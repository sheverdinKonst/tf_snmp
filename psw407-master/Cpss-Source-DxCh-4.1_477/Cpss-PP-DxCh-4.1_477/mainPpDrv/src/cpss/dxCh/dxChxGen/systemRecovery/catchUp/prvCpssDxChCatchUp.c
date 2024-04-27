/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChCatchUp.c
*
* DESCRIPTION:
*       CPSS DxCh CatchUp functions.
*
* FILE REVISION NUMBER:
*       $Revision: 19 $
*******************************************************************************/

#include <cpssCommon/cpssPresteraDefs.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/generic/systemRecovery/cpssGenSystemRecovery.h>
#include <cpss/dxCh/dxChxGen/systemRecovery/catchUp/private/prvCpssDxChCatchUp.h>
#include <cpss/dxCh/dxChxGen/cpssHwInit/cpssDxChHwInit.h>
#include <cpss/dxCh/dxChxGen/bridge/cpssDxChBrgFdb.h>
#include <cpss/dxCh/dxCh3/policer/cpssDxCh3Policer.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChCfg.h>
#include <cpss/generic/port/cpssPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/cpssHwInit/private/prvCpssDxChHwInit.h>
#include <cpss/dxCh/dxChxGen/bridge/private/prvCpssDxChBrgFdbAu.h>
#include <cpss/dxCh/dxChxGen/networkIf/cpssDxChNetIf.h>
#include <cpss/dxCh/dxChxGen/cnc/cpssDxChCnc.h>

PRV_CPSS_DXCH_AUQ_ENABLE_DATA_STC *auqMsgEnableStatus[PRV_CPSS_MAX_PP_DEVICES_CNS] = {NULL};
prvCpssCatchUpFuncPtr  catchUpFuncPtrArray[] = {
                                                prvCpssDxChHwDevNumCatchUp,prvCpssDxChCpuPortModeCatchUp,
                                                prvCpssDxChDevTableCatchUp,prvCpssDxChCpuSdmaPortGroupCatchUp,
                                                prvCpssDxChFdbHashParamsModeCatchUp,prvCpssDxChFdbActionHwDevNumActionHwDevNumMaskCatchUp,
                                                prvCpssDxChPrePendTwoBytesCatchUp, prvCpssDxChPolicerMemorySizeModeCatchUp,
                                                prvCpssDxChSecurBreachPortDropCntrModeAndPortGroupCatchUp,prvCpssDxChPortEgressCntrModeCatchUp,
                                                prvCpssDxChBridgeIngressCntrModeCatchUp,prvCpssDxChPortModeParamsCatchUp,
                                                prvCpssDxChBridgeIngressDropCntrModeCatchUp,NULL
                                                };


GT_STATUS prvCpssDxChNetIfPrePendTwoBytesHeaderFromHwGet
(
    IN  GT_U8        devNum,
    OUT  GT_BOOL    *enablePtr
);

/*******************************************************************************
* prvCpssDxChHwDevNumCatchUp
*
* DESCRIPTION:
*       Synchronize hw device number in software DB by its hw value
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_OUT_OF_RANGE          - on hwDevNum > 31
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChHwDevNumCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc = GT_OK;
    GT_HW_DEV_NUM hwDevNum;
    rc =  cpssDxChCfgHwDevNumGet(devNum,&hwDevNum);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* save HW devNum to the DB */
    PRV_CPSS_HW_DEV_NUM_MAC(devNum) = hwDevNum;
    return rc;
}
/*******************************************************************************
* prvCpssDxChCpuPortModeCatchUp
*
* DESCRIPTION:
*       Synchronize cpu port mode in software DB by its hw value
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_OUT_OF_RANGE          - on hwDevNum > 31
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChCpuPortModeCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc = GT_OK;
    GT_U32 regAddr;
    GT_U32 regValue = 0;

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        /* no such bit as there is no special MAC for CPU */
        regValue = 1;/*sdma*/
    }
    else
    {
        /* use CPU Ethernet port/MII for CPU packets interface and PowerSave = 0*/
        regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.globalControl;
        rc = prvCpssDrvHwPpGetRegField(devNum,regAddr,20,1,&regValue);
        if (rc != GT_OK)
        {
            return rc;
        }
    }

    switch(regValue)
    {
    case 0:
        PRV_CPSS_PP_MAC(devNum)->cpuPortMode = CPSS_NET_CPU_PORT_MODE_MII_E;
        break;
    case 1:
        PRV_CPSS_PP_MAC(devNum)->cpuPortMode = CPSS_NET_CPU_PORT_MODE_SDMA_E;
        break;
    default:
        return GT_FAIL;
    }
    return rc;
}


/*******************************************************************************
* prvCpssDxChCpuSdmaPortGroupCatchUp
*
* DESCRIPTION:
*       Synchronize cpu sdma port group in software DB by its hw value
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_OUT_OF_RANGE          - on hwDevNum > 31
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChCpuSdmaPortGroupCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc;
    GT_U32 regAddr;
    GT_U32 regValue;

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_TRUE)
    {
        if(PRV_CPSS_SIP_5_10_CHECK_MAC(devNum))
        {
            /* Not used any more */
            return GT_OK;
        }
        else
        {
            /* bit 0 - <CpuPortMode> set to 0 'global mode' */
            /* bits 1..4 -  <CpuTargetCore> set to the 'SDMA_PORT_GROUP_ID'  */
            rc = prvCpssHwPpGetRegField(devNum,
                    PRV_DXCH_REG1_UNIT_EGF_EFT_MAC(devNum).global.cpuPortDist,
                    0,5,
                    &regValue);
        }
    }
    else
    {
        regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->txqVer1.egr.global.cpuPortDistribution;
        rc = prvCpssDrvHwPpGetRegField(devNum,regAddr,0,3,&regValue);
    }

    if (rc != GT_OK)
    {
        return rc;
    }

    if ((regValue & 0x1) == 0)
    {
        /* Global CPU port mode */
        /* update device DB */
        PRV_CPSS_PP_MAC(devNum)->netifSdmaPortGroupId = regValue >> 1;
    }
    else
    {
        /* cpss doesn't support Local CPU port mode */
        return GT_FAIL;
    }

    return GT_OK;
}


/*******************************************************************************
* prvCpssDxChDevTableCatchUp
*
* DESCRIPTION:
*       Synchronize device table BMP in software DB by its hw value
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_OUT_OF_RANGE          - on hwDevNum > 31
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChDevTableCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc = GT_OK;
    GT_U32    devTableBmp;

    rc =  cpssDxChBrgFdbDeviceTableGet(devNum,&devTableBmp);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* save device table to the DB */
    PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.devTable = devTableBmp;
    return rc;
}

/*******************************************************************************
* prvCpssDxChFdbHashParamsModeCatchUp
*
* DESCRIPTION:
*       Synchronize fdb hash params in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_OUT_OF_RANGE          - on hwDevNum > 31
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChFdbHashParamsModeCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc = GT_OK;
    CPSS_MAC_VL_ENT vlanMode;
    CPSS_MAC_HASH_FUNC_MODE_ENT hashMode;

    rc =  cpssDxChBrgFdbMacVlanLookupModeGet(devNum,&vlanMode);
    if (rc != GT_OK)
    {
        return rc;
    }
    rc = cpssDxChBrgFdbHashModeGet(devNum,&hashMode);
    if (rc != GT_OK)
    {
        return rc;
    }
     /* Update FDB hash parameters */
    PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.fdbHashParams.vlanMode = vlanMode;
    PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.fdbHashParams.hashMode = hashMode;
    return rc;
}

/*******************************************************************************
* prvCpssDxChFdbActionHwDevNumActionHwDevNumMaskCatchUp
*
* DESCRIPTION:
*       Synchronize Active device number and active device number mask
*       in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChFdbActionHwDevNumActionHwDevNumMaskCatchUp
(
    IN GT_U8    devNum
)
{
    GT_U32   actDev;
    GT_U32   actDevMask;
    GT_STATUS rc = GT_OK;

    rc =  cpssDxChBrgFdbActionActiveDevGet(devNum,&actDev,&actDevMask);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* update data */
    PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.actionHwDevNum = actDev;
    PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.actionHwDevNumMask = actDevMask;
    return rc;
}


/*******************************************************************************
* prvCpssDxChPrePendTwoBytesCatchUp
*
* DESCRIPTION:
*       Synchronize enable/disable pre-pending a two-byte header
*       in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPrePendTwoBytesCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS   rc;
    GT_BOOL  readValue;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    /* get the value from the HW */
    rc = prvCpssDxChNetIfPrePendTwoBytesHeaderFromHwGet(devNum,&readValue);

    if(rc == GT_OK)
    {
        /* save info to the DB */
        PRV_CPSS_DXCH_PP_MAC(devNum)->netIf.prePendTwoBytesHeader = BIT2BOOL_MAC(readValue);
    }

    return rc;
}

/*******************************************************************************
* prvCpssDxChPolicerMemorySizeModeCatchUp
*
* DESCRIPTION:
*       Synchronize Policer Memory Size Mode in software DB by its hw values.
*
* APPLICABLE DEVICES:
*        Lion;  Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*         DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPolicerMemorySizeModeCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS   rc = GT_OK;
    GT_U32      iplr0MemSize;   /* IPLR 0 memory size                */
    GT_U32      iplr1MemSize;   /* IPLR 1 memory size                */
    CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_ENT     mode;

    rc =  cpssDxChPolicerMemorySizeModeGet(devNum,&mode);
    if (rc != GT_OK)
    {
        return rc;
    }
    switch (mode)
    {
        case CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_PLR0_UPPER_PLR1_LOWER_E:
            iplr1MemSize = MIN(256, PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum);
            iplr0MemSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum - iplr1MemSize;
            break;
        case CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_PLR0_UPPER_AND_LOWER_E:
            iplr1MemSize = 0;
            iplr0MemSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum;
            break;
        case CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_PLR1_UPPER_AND_LOWER_E:
            /* Check that the second stage is supported, if not, then memory
               size of the single stage will become 0. */
            if (PRV_CPSS_DXCH_PP_MAC(devNum)->
                fineTuning.featureInfo.iplrSecondStageSupported == GT_FALSE)
            {
                return GT_BAD_PARAM;
            }
            iplr0MemSize = 0;
            iplr1MemSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum;
            break;
        case CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_PLR1_UPPER_PLR0_LOWER_E:
            iplr0MemSize = MIN(256, PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum);
            iplr1MemSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum - iplr0MemSize;
            break;
        case CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_4_E:
            if (PRV_CPSS_DXCH_PP_MAC(devNum)->
                fineTuning.featureInfo.iplrSecondStageSupported == GT_FALSE)
            {
                return GT_BAD_PARAM;
            }
            if (PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT2_E)
            {
                iplr0MemSize = MIN(172, PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum);
                iplr1MemSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum - iplr0MemSize;
                break;
            }
            else
            {
                return GT_BAD_PARAM;
            }
        case CPSS_DXCH_POLICER_MEMORY_CTRL_MODE_5_E:
            if (PRV_CPSS_DXCH_PP_MAC(devNum)->
                fineTuning.featureInfo.iplrSecondStageSupported == GT_FALSE)
            {
                return GT_BAD_PARAM;
            }
            if (PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT2_E)
            {
                iplr1MemSize = MIN(172, PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum);
                iplr0MemSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.policersNum - iplr1MemSize;
                break;
            }
            else
            {
                return GT_BAD_PARAM;
            }
        default:
            return GT_BAD_PARAM;
    }

    /* update memory size in the policer db */
    PRV_CPSS_DXCH_PP_MAC(devNum)->policer.memSize[0] = iplr0MemSize;
    PRV_CPSS_DXCH_PP_MAC(devNum)->policer.memSize[1] = iplr1MemSize;
    return rc;
}

/*******************************************************************************
* prvCpssDxChSecurBreachPortDropCntrModeAndPortGroupCatchUp
*
* DESCRIPTION:
*       Synchronize Secure Breach Port Drop Counter Mode in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChSecurBreachPortDropCntrModeAndPortGroupCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS   rc = GT_OK;             /* return code */
    GT_U32      regAddr;        /* hw register address */
    GT_U32      portGroupId;         /*the port group Id - support multi-port-groups device */
    GT_U32      regValue = 0;
    GT_U32      fieldOffset;            /* The start bit number in the register */
    GT_U32      fieldLength;            /* The number of bits to be written to register */

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        regAddr = PRV_DXCH_REG1_UNIT_L2I_MAC(devNum).
                    bridgeEngineConfig.bridgeGlobalConfig1;
        fieldOffset = 17;
    }
    else
    {
    /* get address of Bridge Configuration Register1 */
    regAddr =
        PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bridgeRegs.bridgeGlobalConfigRegArray[1];
        fieldOffset = 19;
    }

    /* called without portGroupId , loop done inside the driver */
    rc = prvCpssDrvHwPpGetRegField(devNum, regAddr, fieldOffset, 1, &regValue);
    if (rc != GT_OK)
        return rc;
    if (regValue  & 0x1)
    {
        PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.securBreachDropCounterInfo.counterMode = CPSS_BRG_SECUR_BREACH_DROP_COUNT_VLAN_E;
        PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.securBreachDropCounterInfo.portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;
    }
    else
    {
        PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.securBreachDropCounterInfo.counterMode = CPSS_BRG_SECUR_BREACH_DROP_COUNT_PORT_E;

        if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
        {
            regAddr = PRV_DXCH_REG1_UNIT_L2I_MAC(devNum).
                        bridgeEngineConfig.bridgeSecurityBreachDropCntrCfg0;
            fieldOffset = 0;
            fieldLength = 13;
        }
        else
        {
            fieldOffset = 1;
            fieldLength = 6;
        }

        PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
        {
            rc = prvCpssDrvHwPpPortGroupGetRegField(devNum, portGroupId,regAddr, fieldOffset, fieldLength,&regValue);
            if(rc != GT_OK)
            {
                return rc;
            }
            if (regValue != PRV_CPSS_DXCH_NULL_PORT_NUM_CNS)
            {
                PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.securBreachDropCounterInfo.portGroupId = portGroupId;
                break;
            }
        }
        PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

    }
    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChPortEgressCntrModeCatchUp
*
* DESCRIPTION:
*       Synchronize Port Egress Counters Mode in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortEgressCntrModeCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS   rc = GT_OK;
    GT_U32 regAddr;
    GT_U32 value;
    GT_U32  i;
    GT_U32  cntrSetNum;
    GT_U32 portValue = 0;
    GT_U32 globalPort = 0;
    GT_U32 portGroupId = 0;
    GT_U32 txqNum = 0;
    GT_U32 *regValuePtr;
    txqNum = PRV_CPSS_DXCH_PP_HW_INFO_TXQ_UNITS_NUM_MAC(devNum);
    regValuePtr = (GT_U32 *)cpssOsMalloc(sizeof(GT_U32) * txqNum);
    if (regValuePtr == NULL)
    {
        return GT_OUT_OF_CPU_MEM;
    }

    cpssOsMemSet(regValuePtr,0,sizeof(GT_U32)* txqNum);
    for (cntrSetNum = 0; cntrSetNum < 2; cntrSetNum++)
    {
        if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
        {
            CPSS_TBD_BOOKMARK_EARCH
            /* Bobcat2 and Lion3 implementation should be added */

            return GT_NOT_IMPLEMENTED;
        }
        else if(0 == PRV_CPSS_DXCH_PP_HW_INFO_TXQ_REV_1_OR_ABOVE_MAC(devNum))
        {
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->
                        egrTxQConf.txQCountSet[cntrSetNum].txQConfig;
        }
        else
        {
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->
                        txqVer1.queue.peripheralAccess.egressMibCounterSet.config[cntrSetNum];
        }
        for (i =0; i < txqNum; i++)
        {
            rc = prvCpssHwPpPortGroupReadRegister(devNum,
                   i*PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS,
                   regAddr, &value);
            if(rc != GT_OK)
            {
                cpssOsFree(regValuePtr);
                return rc;
            }
            regValuePtr[i] = value;
        }
        /* check if all values are the same */
        for (i = 0; i < txqNum; i++ )
        {
            if (regValuePtr[0] != regValuePtr[i])
            {
                break;
            }
        }
        if ((i == txqNum) && (txqNum > 1))
        {
            /* it means all values are the same */
            PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                portEgressCntrModeInfo[cntrSetNum].portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;

        }
        else
            if ((txqNum == 1)&&(regValuePtr[0] & 0x1) == 0x0)
            {
                PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                    portEgressCntrModeInfo[cntrSetNum].portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;
            }
            else
            {
                for (i = 0; i < txqNum; i++ )
                {
                    portValue = (regValuePtr[i]>>4) & 0x3f;
                    if (portValue != PRV_CPSS_DXCH_NULL_PORT_NUM_CNS)
                    {
                        /* check if it is CPU port */
                        if ((portValue & 0xf) == 0xf)
                        {

                            PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                                portEgressCntrModeInfo[cntrSetNum].portGroupId = PRV_CPSS_NETIF_SDMA_PORT_GROUP_ID_MAC(devNum);
                        }
                        else
                        {
                            /* convert port value from port local HEM to global*/
                            globalPort = PRV_CPSS_DXCH_HEM_LOCAL_TO_GLOBAL_PORT(devNum,i*PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS,portValue);
                             /* convert the 'Physical port' to portGroupId,local port -- supporting multi-port-groups device */
                            portGroupId = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, globalPort);
                            PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                                portEgressCntrModeInfo[cntrSetNum].portGroupId = portGroupId;
                        }
                        break;
                    }
                }
                if (i == txqNum)
                {
                    /* it means that all port values are PRV_CPSS_DXCH_NULL_PORT_NUM_CNS*/
                    return GT_BAD_STATE;
                }
            }
    }
    cpssOsFree(regValuePtr);
    return rc;
}

/*******************************************************************************
* prvCpssDxChBridgeIngressCntrModeCatchUp
*
* DESCRIPTION:
*       Synchronize Bridge Ingress Counters Mode in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBridgeIngressCntrModeCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS   rc = GT_OK;
    GT_U32 regAddr;
    GT_U32 value = 0;
    GT_U32  portGroupId;
    GT_U32  i = 0;
    GT_U32  entriesCounter = 0;
    GT_U32 cntrSetNum = 0;
    GT_U32 regValue[8][2] = {{0,0}};
    for (cntrSetNum = 0; cntrSetNum < 2; cntrSetNum++)
    {
        entriesCounter = 0;

        if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_TRUE)
        {
            regAddr = PRV_DXCH_REG1_UNIT_L2I_MAC(devNum).
                        layer2BridgeMIBCntrs.cntrsSetConfig0[cntrSetNum];
        }
        else
        {
        regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bridgeRegs.
                                             brgCntrSet[cntrSetNum].cntrSetCfg;
        }
        /* loop on all port groups :
            on the port group that 'own' the port , set the needed configuration
            on other port groups put 'NULL port'
        */
        PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
        {
            rc = prvCpssDrvHwPpPortGroupReadRegister(devNum, portGroupId,regAddr,&value);
            if(rc != GT_OK)
            {
                return rc;
            }
            regValue[entriesCounter][0] = value;
            regValue[entriesCounter][1] = portGroupId;
            entriesCounter++;
        }
        PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

        /* check if all values are the same */
        for (i = 0; i < entriesCounter; i++ )
        {
            if (regValue[0][0] != regValue[i][0])
            {
                break;
            }
        }

        if ((i == entriesCounter) && (entriesCounter > 1))
        {
             /* it means all values are the same */
                PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                    bridgeIngressCntrMode[cntrSetNum].portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;
        }
        else
            if ((entriesCounter == 1) && ((regValue[0][0] & 0x3) == 0x1))
            {
                PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                    bridgeIngressCntrMode[cntrSetNum].portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;
            }
            else
            {
                for (i = 0; i < entriesCounter; i++ )
                {
                    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_ENABLED_MAC(devNum) == GT_TRUE)
                    {
                        regValue[i][0] = (value >> 2) & (BIT_13 - 1);
                    }
                    else
                    {
                    regValue[i][0] = (value >> 2) & 0x3f;
                    }

                    if (regValue[i][0] != PRV_CPSS_DXCH_NULL_PORT_NUM_CNS)
                    {
                        PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.
                                bridgeIngressCntrMode[cntrSetNum].portGroupId = regValue[i][1];
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
    }

    return rc;
}

/*******************************************************************************
* prvCpssDxChBridgeIngressDropCntrModeCatchUp
*
* DESCRIPTION:
*       Synchronize  Ingress Drop Counters Mode in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBridgeIngressDropCntrModeCatchUp
(
    IN GT_U8    devNum
)
{
    GT_STATUS   rc = GT_OK;
    GT_U32 regAddr;
    GT_U32 value;
    GT_U32  portGroupId;
    GT_U32  i = 0;
    GT_U32  entriesCounter = 0;
    GT_U32 regValue[8][2] = {{0,0}};
    entriesCounter = 0;

    if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
    {
        regAddr = PRV_DXCH_REG1_UNIT_EQ_MAC(devNum).ingrDropCntr.ingrDropCntrConfig;
    }
    else
    {
    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.eqBlkCfgRegs.ingressDropCntrConfReg;
    }

    /* loop on all port groups :
    */
    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    {
        rc = prvCpssDrvHwPpPortGroupReadRegister(devNum, portGroupId,regAddr,&value);
        if(rc != GT_OK)
        {
            return rc;
        }
        regValue[entriesCounter][0] = value;
        regValue[entriesCounter][1] = portGroupId;
        entriesCounter++;
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

    /* check if all values are the same */
    for (i = 0; i < entriesCounter; i++ )
    {
        if (regValue[0][0] != regValue[i][0])
        {
            break;
        }
    }
    if ((i == entriesCounter) && (entriesCounter > 1))
    {
         /* it means all values are the same */
           PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.cfgIngressDropCntrMode.portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;
    }
    else
        if ((entriesCounter == 1) && ((regValue[0][0] & 0x3) != 0x2))
        {
            /* it is not port mode */
            PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.cfgIngressDropCntrMode.portGroupId = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;
        }
        else
        {
            for (i = 0; i < entriesCounter; i++ )
            {
                if(PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_MAC(devNum) == GT_TRUE)
                {
                    regValue[i][0] =  (regValue[i][0] >> 2) & 0x1fff;
                }
                else
                {
                regValue[i][0] =  (regValue[i][0] >> 2) & 0xfff;
                }

                if (regValue[i][0] != PRV_CPSS_DXCH_NULL_PORT_NUM_CNS)
                {
                    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.cfgIngressDropCntrMode.portGroupId = regValue[i][1];
                    break;
                }
                else
                {
                    continue;
                }
            }
        }
    return rc;
}



/*******************************************************************************
* prvCpssDxChPortModeParamsCatchUp
*
* DESCRIPTION:
*       Synchronize Port Mode parameters in software DB by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortModeParamsCatchUp
(
    IN GT_U8    devNum
)
{
    GT_PHYSICAL_PORT_NUM port;
    GT_STATUS rc = GT_OK;
    CPSS_PORT_INTERFACE_MODE_ENT   ifMode;
    CPSS_PORT_SPEED_ENT            speed;
    /* loop over all GE and FE ports */
    for(port = 0; port < PRV_CPSS_PP_MAC(devNum)->numOfPorts; port++)
    {
        /* skip not existed ports */
        if (! PRV_CPSS_PHY_PORT_IS_EXIST_MAC(devNum, port))
            continue;

        rc =  prvCpssDxChPortInterfaceModeHwGet(devNum, port, &ifMode);
        if (rc != GT_OK)
        {
            return rc;
        }

        /* portType updated inside prvCpssDxChPortInterfaceModeHwGet */
        PRV_CPSS_PP_MAC(devNum)->phyPortInfoArray[port].portIfMode = ifMode;

        rc = prvCpssDxChPortSpeedHwGet(devNum, port, &speed);
        if (rc != GT_OK)
        {
            return rc;
        }
        PRV_CPSS_PP_MAC(devNum)->phyPortInfoArray[port].portSpeed = speed;
        if((CPSS_PORT_SPEED_2500_E == speed) &&
            (CPSS_PORT_INTERFACE_MODE_1000BASE_X_E == ifMode))
        {/* SGMII 2.5G in HW implemented as 1000BaseX */
            PRV_CPSS_PP_MAC(devNum)->phyPortInfoArray[port].portIfMode =
                                            CPSS_PORT_INTERFACE_MODE_SGMII_E;
        }

        if (CPSS_SYSTEM_RECOVERY_PROCESS_FAST_BOOT_E == systemRecoveryInfo.systemRecoveryProcess)
        {
            if((CPSS_PP_FAMILY_DXCH_LION2_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
                || (CPSS_PP_FAMILY_DXCH_LION3_E == PRV_CPSS_PP_MAC(devNum)->devFamily))
            {
                if((ifMode != CPSS_PORT_INTERFACE_MODE_NA_E) &&
                   (speed != CPSS_PORT_SPEED_NA_E))
                {
                    rc = prvCpssDxChLion2PortTypeSet(devNum, port, ifMode, speed);
                    if (rc != GT_OK)
                    {
                        return rc;
                    }
                }
                /* else leave cpss default defined in hwPpPhase1Part1 */
            }
            else
            {
                if(ifMode != CPSS_PORT_INTERFACE_MODE_NA_E)
                {
                    prvCpssDxChPortTypeSet(devNum, port, ifMode, speed);
                }
            }
        }

        /* update addresses of mac registers accordingly to used MAC 1G/XG/XLG */
        rc = prvCpssDxChHwRegAddrPortMacUpdate(devNum, port, ifMode);
        if (rc != GT_OK)
        {
            return rc;
        }
    }
    return GT_OK;
}


/************************************************************************
* dxChAuFuPtrUpdate
*
* DESCRIPTION:
*       The function scan the AU/FU queues and update AU/FU software queue pointer.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum            - the device number on which AU are counted
*       portGroupId       - the portGroupId - for multi-port-groups support
*       queueType         - AUQ or FUQ. FUQ valid for DxCh2 and above.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChAuFuPtrUpdate
(
    IN  GT_U8                         devNum,
    IN  GT_U32                        portGroupId,
    IN  MESSAGE_QUEUE_ENT             queueType
)
{
    PRV_CPSS_AU_DESC_STC        *descPtr;          /*pointer to the current descriptor*/
    PRV_CPSS_AU_DESC_STC        *descBlockPtr;     /* AU descriptors block */
    PRV_CPSS_AU_DESC_CTRL_STC   *descCtrlPtr;      /* pointer to the descriptors DB of the device */
    PRV_CPSS_AU_DESC_CTRL_STC   *descCtrl1Ptr = 0; /* pointer to the descriptors DB
                                                      for additional primary AUQ of the device */
    GT_U32                      ii;                /* iterator */
    GT_U32                      auMessageNumBytes;
    GT_U32                      auMessageNumWords;
    GT_U32                      numberOfQueues = 1; /* number of queues */
    GT_U32                      currentQueue;       /* iterator */
    GT_BOOL                     useDoubleAuq;       /* support configuration of two AUQ memory regions */
    PRV_CPSS_AU_DESC_EXT_8_STC  *descExtPtr;        /*pointer to the current descriptor*/
     GT_U32                     *auMemPtr = NULL;   /* pointer to start of current message */
    CPSS_MAC_ENTRY_EXT_KEY_STC  macEntry;
    GT_U32                      portGroupsBmp;
    GT_U32                      qa_counter;
    GT_PORT_GROUPS_BMP          completedPortGroupsBmp = 0;
    GT_PORT_GROUPS_BMP          succeededPortGroupsBmp = 0;
    CPSS_MAC_UPDATE_MSG_EXT_STC auFuMessage;
    GT_STATUS rc = GT_OK;
    GT_32           intKey = 0;
    CPSS_SYSTEM_RECOVERY_INFO_STC tempSystemRecoveryInfo;
    tempSystemRecoveryInfo.systemRecoveryState = systemRecoveryInfo.systemRecoveryState;

    cpssOsMemSet(&macEntry,0,sizeof(CPSS_MAC_ENTRY_EXT_KEY_STC));
    cpssOsMemSet(&auFuMessage,0,sizeof(CPSS_MAC_UPDATE_MSG_EXT_STC));
    macEntry.entryType                      = CPSS_MAC_ENTRY_EXT_TYPE_MAC_ADDR_E;
    macEntry.key.macVlan.vlanId             = 0;
    macEntry.key.macVlan.macAddr.arEther[0] = 0x0;
    macEntry.key.macVlan.macAddr.arEther[1] = 0x1A;
    macEntry.key.macVlan.macAddr.arEther[2] = 0xFF;
    macEntry.key.macVlan.macAddr.arEther[3] = 0xFF;
    macEntry.key.macVlan.macAddr.arEther[4] = 0xFF;
    macEntry.key.macVlan.macAddr.arEther[5] = 0xFF;

    auMessageNumWords = PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.auMessageNumOfWords;
    auMessageNumBytes = 4 * auMessageNumWords;

    useDoubleAuq = PRV_CPSS_DXCH_PP_MAC(devNum)->moduleCfg.useDoubleAuq;

    switch (queueType)
    {
    case MESSAGE_QUEUE_PRIMARY_FUQ_E:
        descCtrlPtr = &(PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl[portGroupId]);
        break;
    case MESSAGE_QUEUE_PRIMARY_AUQ_E:
        descCtrlPtr = (PRV_CPSS_AUQ_INDEX_MAC(devNum, portGroupId) == 0) ?
            &(PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl[portGroupId]) :
            &(PRV_CPSS_PP_MAC(devNum)->intCtrl.au1DescCtrl[portGroupId]);

        if (useDoubleAuq == GT_TRUE)
        {
            numberOfQueues = 2;
            descCtrl1Ptr = (PRV_CPSS_AUQ_INDEX_MAC(devNum, portGroupId) == 1) ?
                &(PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl[portGroupId]) :
                &(PRV_CPSS_PP_MAC(devNum)->intCtrl.au1DescCtrl[portGroupId]);
        }
        break;
    case MESSAGE_QUEUE_SECONDARY_AUQ_E:
        descCtrlPtr = (PRV_CPSS_SECONDARY_AUQ_INDEX_MAC(devNum, portGroupId) == 0) ?
            &(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAuDescCtrl[portGroupId]) :
            &(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAu1DescCtrl[portGroupId]);
        if (useDoubleAuq == GT_TRUE)
        {
            numberOfQueues = 2;
            descCtrl1Ptr = (PRV_CPSS_SECONDARY_AUQ_INDEX_MAC(devNum, portGroupId) == 1) ?
                &(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAuDescCtrl[portGroupId]) :
                &(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAu1DescCtrl[portGroupId]);
        }
        break;
    default:
        return GT_BAD_PARAM;
    }
    /* get address of AU descriptors block */
    descBlockPtr = (PRV_CPSS_AU_DESC_STC*)(descCtrlPtr->blockAddr);

    /* pointer to the current descriptor */
    descPtr = &(descBlockPtr[descCtrlPtr->currDescIdx]);
    cpssExtDrvSetIntLockUnlock(CPSS_OS_INTR_MODE_LOCK_E, &intKey);
    for(currentQueue = 0; currentQueue < numberOfQueues; currentQueue++)
    {
        /* Second iteration for additional AUQ */
        if(currentQueue == 1)
        {
             descCtrlPtr = descCtrl1Ptr;
             descPtr = &(descBlockPtr[descCtrlPtr->currDescIdx]);
        }
        for (ii = descCtrlPtr->currDescIdx; ii < descCtrlPtr->blockSize; ii++ ,descPtr++)
        {
            if(!(AU_DESC_IS_NOT_VALID(descPtr)))
            {
                /* no more not valid descriptors */
                break;
            }
            /* increment software descriptor pointer*/
             descCtrlPtr->currDescIdx =
                (( descCtrlPtr->currDescIdx + 1) %  descCtrlPtr->blockSize);
        }
        if (ii == descCtrlPtr->blockSize)
        {
            descCtrlPtr->currDescIdx = 0;
            descPtr = &(descBlockPtr[descCtrlPtr->currDescIdx]);
            if (queueType == MESSAGE_QUEUE_PRIMARY_AUQ_E)
            {
                /*In this case there is no new real message in the queue. In order to understand where software pointer is*/
                /* quary is sent from cpu to pp and pp will reply to cpu with quary response and this response should be found*/
                /* in AUQ*/
                portGroupsBmp = 0;
                portGroupsBmp = portGroupsBmp | (1<<portGroupId);

                qa_counter = 0;
                systemRecoveryInfo.systemRecoveryState = CPSS_SYSTEM_RECOVERY_COMPLETION_STATE_E;
                do
                {
                    rc =  cpssDxChBrgFdbPortGroupQaSend( devNum, portGroupsBmp,&macEntry);
                     if(rc != GT_OK)
                     {
                #ifdef ASIC_SIMULATION
                         cpssOsTimerWkAfter(1);
                #endif
                         qa_counter++;
                         if(qa_counter > 20)
                         {
                             cpssExtDrvSetIntLockUnlock(CPSS_OS_INTR_MODE_UNLOCK_E, &intKey);
                             systemRecoveryInfo.systemRecoveryState = tempSystemRecoveryInfo.systemRecoveryState;
                             return rc;
                         }
                     }
                 } while (rc != GT_OK);
                 systemRecoveryInfo.systemRecoveryState = tempSystemRecoveryInfo.systemRecoveryState;
                 /* verify that action is completed */
                 do
                 {
                     rc = cpssDxChBrgFdbPortGroupFromCpuAuMsgStatusGet(devNum,portGroupsBmp,&completedPortGroupsBmp,
                                                                       &succeededPortGroupsBmp);
                     if(rc != GT_OK)
                     {
                         cpssExtDrvSetIntLockUnlock(CPSS_OS_INTR_MODE_UNLOCK_E, &intKey);
                         return rc;
                     }
                 }
                 while ((completedPortGroupsBmp & portGroupsBmp)!= portGroupsBmp);
                 /* now perform search again */
                 for (ii = descCtrlPtr->currDescIdx; ii < descCtrlPtr->blockSize; ii++ ,descPtr++)
                 {
                     if(!(AU_DESC_IS_NOT_VALID(descPtr)))
                     {
                         /* no more not valid descriptors */
                         break;
                     }
                     /* increment software descriptor pointer*/
                      descCtrlPtr->currDescIdx =
                         (( descCtrlPtr->currDescIdx + 1) %  descCtrlPtr->blockSize);
                 }

                 if (ii == descCtrlPtr->blockSize)
                 {
                     descCtrlPtr->currDescIdx = 0;
                 }
                 else
                 {
                     /* entry was found */
                     /* check that this entry is QR and delete it*/
                     /* the pointer to start of 'next message to handle'  */
                     auMemPtr = (GT_U32 *)(descCtrlPtr->blockAddr + (auMessageNumBytes * descCtrlPtr->currDescIdx));
                     descExtPtr= (PRV_CPSS_AU_DESC_EXT_8_STC*)auMemPtr;
                     rc = auDesc2UpdMsg(devNum, portGroupId ,descExtPtr , GT_TRUE, &auFuMessage);
                     if(rc != GT_OK)
                     {
                         return rc;
                     }
                     if ( (auFuMessage.macEntry.key.entryType == CPSS_MAC_ENTRY_EXT_TYPE_MAC_ADDR_E)        &&
                          (0 == cpssOsMemCmp((GT_VOID*)&auFuMessage.macEntry.key.key.macVlan.macAddr,
                                             (GT_VOID*)&macEntry.key.macVlan.macAddr,
                                             sizeof (GT_ETHERADDR)))                                        &&
                          (auFuMessage.macEntry.key.key.macVlan.vlanId  == macEntry.key.macVlan.vlanId) )
                     {
                         AU_DESC_RESET_MAC(descPtr);
                         descCtrlPtr->currDescIdx++;
                     }
                 }
            }
        }
    }
    cpssExtDrvSetIntLockUnlock(CPSS_OS_INTR_MODE_UNLOCK_E, &intKey);
    return GT_OK;
}


/*******************************************************************************
* dxChHaAuFuSameMemCatchUp
*
* DESCRIPTION:
*       Synchronize  software DB AU/FU pointers by its hw values
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChHaAuFuSameMemCatchUp
(
    IN GT_U8    devNum
)
{
    MESSAGE_QUEUE_ENT             queueType;
    GT_U32                        portGroupId;
    GT_STATUS                     rc = GT_OK;

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    {
        if(PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl[portGroupId].blockAddr != 0)
        {
            /* handle AUQs for this portGroup */
            queueType = MESSAGE_QUEUE_PRIMARY_AUQ_E;
            rc = dxChAuFuPtrUpdate(devNum, portGroupId, queueType);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
        if(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAuDescCtrl[portGroupId].blockAddr != 0)
        {
            /* handle secondary AUQs for this portGroup */
            queueType = MESSAGE_QUEUE_SECONDARY_AUQ_E;
            rc = dxChAuFuPtrUpdate(devNum,portGroupId, queueType);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
        if(PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl[portGroupId].blockAddr != 0)
        {
            /* handle FUQs for this portGroup */
            queueType = MESSAGE_QUEUE_PRIMARY_FUQ_E;
            rc = dxChAuFuPtrUpdate(devNum,portGroupId, queueType);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

    return GT_OK;
}
/*******************************************************************************
* prvCpssDxChSystemRecoveryCatchUpSameMemoryAuFuHandle
*
* DESCRIPTION:
*       Synchronize AUQ/FUQ software pointers by its hw values.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       This function could be used only if application guarantees constant
*       surviving cpu restart memory for AUQ/FUQ allocation.
*
*******************************************************************************/
GT_STATUS prvCpssDxChSystemRecoveryCatchUpSameMemoryAuFuHandle
(
    IN GT_U8    devNum
)
{
    PRV_CPSS_DXCH_MODULE_CONFIG_STC *moduleCfgPtr;/* pointer to the module
                                                configure of the PP's database*/
    GT_U32 portGroupId;
    GT_STATUS rc = GT_OK;
    GT_UINTPTR                  phyAddr;/* The physical address of the AU block*/
    GT_UINTPTR virtAddr;
    GT_U32 queueSize;
    GT_U32 regAddr;
    GT_U32 auMessageNumBytes; /* number of bytes in AU/FU message */
    GT_U32 auqTotalSize = 0;      /* auq total size of all port groups*/
    GT_U32 fuqTotalSize = 0;      /* fuq total size of all port groups*/


    PRV_CPSS_AU_DESC_CTRL_STC    *auDescCtrlPtr = NULL;

    /* Configure the module configruation struct.   */
    moduleCfgPtr = &(PRV_CPSS_DXCH_PP_MAC(devNum)->moduleCfg);
    if (moduleCfgPtr->useDoubleAuq == GT_TRUE)
    {
        return GT_NOT_SUPPORTED;
    }
    auMessageNumBytes = 4 * PRV_CPSS_DXCH_PP_MAC(devNum)->bridge.auMessageNumOfWords;
    /* restore AUQ/FUQ DB */
    cpssOsMemSet(PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl,0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl));
    cpssOsMemSet(PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl,0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl));
    cpssOsMemSet(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAuDescCtrl,0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAuDescCtrl));
    cpssOsMemSet(&(PRV_CPSS_PP_MAC(devNum)->intCtrl.auqDeadLockWa),0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.auqDeadLockWa));
    cpssOsMemSet(PRV_CPSS_PP_MAC(devNum)->intCtrl.activeAuqIndex,0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.activeAuqIndex));
    cpssOsMemSet(PRV_CPSS_PP_MAC(devNum)->intCtrl.au1DescCtrl,0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.au1DescCtrl));
    cpssOsMemSet(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAu1DescCtrl,0,sizeof(PRV_CPSS_PP_MAC(devNum)->intCtrl.secondaryAu1DescCtrl));

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    {
        auDescCtrlPtr = &(PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl[portGroupId]);
        regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.auQBaseAddr;

        rc = prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,(GT_U32*)&phyAddr);
        if (rc != GT_OK)
        {
            return rc;
        }
        cpssOsPhy2Virt(phyAddr,&virtAddr);
        auDescCtrlPtr->blockAddr = virtAddr;
        regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.auQControl;
        rc = prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,&queueSize);
        if (rc != GT_OK)
        {
            return rc;
        }
        auDescCtrlPtr->blockSize = queueSize;
        auqTotalSize += queueSize;
        auDescCtrlPtr->currDescIdx = 0;
        auDescCtrlPtr->unreadCncCounters = 0;

        if (moduleCfgPtr->fuqUseSeparate == GT_TRUE)
        {
            auDescCtrlPtr = &(PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl[portGroupId]);
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQBaseAddr;
            rc = prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,(GT_U32*)&phyAddr);
            if (rc != GT_OK)
            {
                return rc;
            }
            cpssOsPhy2Virt(phyAddr,&virtAddr);
            auDescCtrlPtr->blockAddr = virtAddr;
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQControl;
            rc = prvCpssDrvHwPpPortGroupGetRegField(devNum,portGroupId,regAddr,0,31,&queueSize);/*num of entries in the block*/
            if (rc != GT_OK)
            {
                return rc;
            }
            auDescCtrlPtr->blockSize = queueSize;
            fuqTotalSize += queueSize;
            auDescCtrlPtr->currDescIdx = 0;
            auDescCtrlPtr->unreadCncCounters = 0;
        }
        /* Set primary AUQ index */
        PRV_CPSS_PP_MAC(devNum)->intCtrl.activeAuqIndex[portGroupId] = 0;
        /* Set primary AUQ init state - 'FULL';
        When WA triggered for the first time - all primary AUQs are full */
        PRV_CPSS_PP_MAC(devNum)->intCtrl.auqDeadLockWa[portGroupId].primaryState =
            PRV_CPSS_AUQ_STATE_ALL_FULL_E;
        /* Set secondary AUQ index */
        PRV_CPSS_PP_MAC(devNum)->intCtrl.auqDeadLockWa[portGroupId].activeSecondaryAuqIndex = 0;
        /* Set secondary AUQ state - 'EMPTY' */
        PRV_CPSS_PP_MAC(devNum)->intCtrl.auqDeadLockWa[portGroupId].secondaryState =
            PRV_CPSS_AUQ_STATE_ALL_EMPTY_E;
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

    moduleCfgPtr->auCfg.auDescBlock = (GT_U8*)PRV_CPSS_PP_MAC(devNum)->intCtrl.auDescCtrl[0].blockAddr;
    moduleCfgPtr->auCfg.auDescBlockSize = auqTotalSize * auMessageNumBytes;
    if (moduleCfgPtr->fuqUseSeparate == GT_TRUE)
    {
        moduleCfgPtr->fuCfg.fuDescBlock = (GT_U8*)PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl[0].blockAddr;
        moduleCfgPtr->fuCfg.fuDescBlockSize = fuqTotalSize * auMessageNumBytes;
    }

    /* perform auq/fuq sw pointer tuning */
    rc = dxChHaAuFuSameMemCatchUp(devNum);
    return rc;
}


/*******************************************************************************
* prvCpssDxChSystemRecoveryCatchUpDiffMemoryFuHandle
*
* DESCRIPTION:
*       Synchronize FUQ software pointers by its hw values.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       This function could be used only if application couldn't guarantee constant
*       surviving cpu restart memory for AUQ/FUQ allocation.
*
*******************************************************************************/
GT_STATUS prvCpssDxChSystemRecoveryCatchUpDiffMemoryFuHandle
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc = GT_OK;
    PRV_CPSS_DXCH_MODULE_CONFIG_STC *moduleCfgPtr;
    GT_U32                          regAddr;
    GT_U32                          portGroupId;

    moduleCfgPtr = &(PRV_CPSS_DXCH_PP_MAC(devNum)->moduleCfg);
    if (moduleCfgPtr->fuqUseSeparate == GT_FALSE)
    {
        return GT_OK;
    }
    /* After this action on chip FIFO contents (if at all) is transferred */
    /* into FUQ defined during cpss init                                  */
    /* now sinchronization hw and sw FUQ pointers is required */

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    {
        if(PRV_CPSS_PP_MAC(devNum)->intCtrl.fuDescCtrl[portGroupId].blockAddr != 0)
        {
            /* Enable FUQ for each portgroup  */
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQControl;
            rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,31,1,1);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
        else
        {
            return GT_FAIL;
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

    return GT_OK;
}
/*******************************************************************************
* prvCpssDxChSystemRecoveryCatchUpHandle
*
* DESCRIPTION:
*       Perform synchronization of hardware data and software DB after special init sequence.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssDxChSystemRecoveryCatchUpHandle
(
   GT_VOID
)
{
    GT_STATUS rc = GT_OK;
    GT_U8 devNum;
    GT_U32 i = 0;
    GT_U8 rxQueue = 0;
    CPSS_SYSTEM_RECOVERY_INFO_STC   tempSystemRecoveryInfo = systemRecoveryInfo;
    for (devNum = 0; devNum < PRV_CPSS_MAX_PP_DEVICES_CNS; devNum++)
    {
        if ( (prvCpssPpConfig[devNum] == NULL) ||
             (PRV_CPSS_DXCH_FAMILY_CHECK_MAC(devNum) == 0) )
        {
            continue;
        }
        if (PRV_CPSS_DXCH_LION_FAMILY_CHECK_MAC(devNum))
        {
            while (catchUpFuncPtrArray[i] != NULL)
            {
                /* perform catch up*/
                rc = (*catchUpFuncPtrArray[i])(devNum);
                if (rc != GT_OK)
                {
                    return rc;
                }
                i++;
            }
        }
        if (systemRecoveryInfo.systemRecoveryMode.continuousAuMessages == GT_TRUE)
        {
            /* HA mode - application provide the same memory for AUQ -- handle AUQ pointer*/
            rc = prvCpssDxChSystemRecoveryCatchUpSameMemoryAuFuHandle(devNum);
            if (rc != GT_OK)
            {
                return rc;
            }
        }

        if(systemRecoveryInfo.systemRecoveryMode.haCpuMemoryAccessBlocked == GT_TRUE)
        {
            /*HA mode - application can't guarantee the same memory for AUQ */
            /* during cpss init stage special AUQ WA was done and AUQ was disable for messages.*/
            systemRecoveryInfo.systemRecoveryState = CPSS_SYSTEM_RECOVERY_COMPLETION_STATE_E;
            if (systemRecoveryInfo.systemRecoveryMode.continuousFuMessages == GT_FALSE)
            {
                /*Restore FUQ enable status. In this case hw should be written */

                rc = prvCpssDxChSystemRecoveryCatchUpDiffMemoryFuHandle(devNum);
                if (rc != GT_OK)
                {
                    systemRecoveryInfo.systemRecoveryState = tempSystemRecoveryInfo.systemRecoveryState;
                    return rc;
                }
            }

            if (systemRecoveryInfo.systemRecoveryMode.continuousAuMessages == GT_FALSE)
            {
                /*Restore AUQ enable status. In this case hw should be written */
                rc =  prvCpssDxChSystemRecoveryCatchUpDiffMemoryAuHandle(devNum);
                if (rc != GT_OK)
                {
                    systemRecoveryInfo.systemRecoveryState = tempSystemRecoveryInfo.systemRecoveryState;
                    return rc;
                }
            }

            /* enable RxSDMA queues */
            /* write Rx SDMA Queues Reg - Enable Rx SDMA Queues */
            if (systemRecoveryInfo.systemRecoveryMode.continuousRx == GT_FALSE)
            {
                for(rxQueue = 0; rxQueue < NUM_OF_RX_QUEUES; rxQueue++)
                {
                    /* Enable Rx SDMA Queue */
                    rc = cpssDxChNetIfSdmaRxQueueEnable(devNum, rxQueue, GT_TRUE);
                    if(rc != GT_OK)
                    {
                        systemRecoveryInfo.systemRecoveryState = tempSystemRecoveryInfo.systemRecoveryState;
                        return rc;
                    }
                }
            }
            systemRecoveryInfo.systemRecoveryState = tempSystemRecoveryInfo.systemRecoveryState;
        }
    }
    return rc;
}


/*******************************************************************************
* dxChEnableFdbUploadActionAndSaveFuqCurrentStatus
*
* DESCRIPTION:
*       This function configure FDB upload action for specific entry
*       and save current FUQ action status.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum        - device number.
*
* OUTPUTS:
*       actionDataPtr - pointer to action data.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_FAIL                  - on error.
*       GT_HW_ERROR              - on hardware error.
*       GT_BAD_PARAM             - on bad device.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChEnableFdbUploadActionAndSaveFuqCurrentStatus
(
    IN  GT_U8                             devNum,
    OUT PRV_CPSS_DXCH_FUQ_ACTION_DATA_STC *actionDataPtr
)
{
    GT_STATUS rc = GT_OK;

    rc = cpssDxChBrgFdbUploadEnableGet(devNum,&actionDataPtr->fdbUploadState);
    if(rc != GT_OK)
    {
        return rc;
    }

    /* Enable/Disable reading FDB entries via AU messages to the CPU*/
    rc =  cpssDxChBrgFdbUploadEnableSet(devNum, GT_TRUE);
    if(rc != GT_OK)
    {
        return rc;
    }

    /* configure FDB to upload only this specific entry*/

    /* save vid and vid mask */
    rc =  cpssDxChBrgFdbActionActiveVlanGet(devNum,&actionDataPtr->currentVid,
                                            &actionDataPtr->currentVidMask);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* set current vid and vid mask */
    rc =  cpssDxChBrgFdbActionActiveVlanSet(devNum,9,0xfff);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* save current action dev */
    rc =  cpssDxChBrgFdbActionActiveDevGet(devNum,&actionDataPtr->actDev,
                                           &actionDataPtr->actDevMask);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* set new action device */
    rc = cpssDxChBrgFdbActionActiveDevSet(devNum,30,0x1f);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* save current action interface */
    rc = cpssDxChBrgFdbActionActiveInterfaceGet(devNum,&actionDataPtr->actIsTrunk,&actionDataPtr->actIsTrunkMask,
                                                &actionDataPtr->actTrunkPort,&actionDataPtr->actTrunkPortMask);

    if (rc != GT_OK)
    {
        return rc;
    }
    /* set new action interface */
    rc =  cpssDxChBrgFdbActionActiveInterfaceSet(devNum,0,0,62,0x3f);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* save action trigger mode */
    rc = cpssDxChBrgFdbMacTriggerModeGet(devNum,&actionDataPtr->triggerMode);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* set new action trigger mode */
    rc = cpssDxChBrgFdbMacTriggerModeSet(devNum,CPSS_ACT_TRIG_E);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* save action mode */
    rc =  cpssDxChBrgFdbActionModeGet(devNum,&actionDataPtr->actionMode);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* set fdb upload action mode */
    rc = cpssDxChBrgFdbActionModeSet(devNum,CPSS_FDB_ACTION_AGE_WITHOUT_REMOVAL_E);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* save action enable state*/
    rc = cpssDxChBrgFdbActionsEnableGet(devNum,&actionDataPtr->actionEnable);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* set action enable set */
    rc = cpssDxChBrgFdbActionsEnableSet(devNum,GT_TRUE);
    if (rc != GT_OK)
    {
        return rc;
    }
    if(CPSS_PP_FAMILY_DXCH_LION2_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        /* save maskAuFuMsg2CpuOnNonLocal state*/
        rc = prvCpssDxChBrgFdbAuFuMessageToCpuOnNonLocalMaskEnableGet(devNum,&actionDataPtr->maskAuFuMsg2CpuOnNonLocal);
        if (rc != GT_OK)
        {
            return rc;
        }
        /* set maskAuFuMsg2CpuOnNonLocal state*/
        rc = prvCpssDxChBrgFdbAuFuMessageToCpuOnNonLocalMaskEnableSet(devNum,GT_FALSE);
    }

    return rc;
}

/*******************************************************************************
* dxChRestoreCurrentFdbActionStatus
*
* DESCRIPTION:
*       This function restore FDB action data and apply it on the device.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum        - device number
*       actionDataPtr - pointer to action data
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChRestoreCurrentFdbActionStatus
(
    IN GT_U8    devNum,
    IN PRV_CPSS_DXCH_FUQ_ACTION_DATA_STC *actionDataPtr
)
{
    GT_STATUS rc = GT_OK;

    rc =  cpssDxChBrgFdbActionActiveVlanSet(devNum,actionDataPtr->currentVid,
                                            actionDataPtr->currentVidMask);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* restore saved action device */
    rc =  cpssDxChBrgFdbActionActiveDevSet(devNum,actionDataPtr->actDev,
                                           actionDataPtr->actDevMask);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* restore saved action interface */
    rc = cpssDxChBrgFdbActionActiveInterfaceSet(devNum,actionDataPtr->actIsTrunk,
                                                actionDataPtr->actIsTrunkMask,
                                                actionDataPtr->actTrunkPort,
                                                actionDataPtr->actTrunkPortMask);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* restore saved action trigger mode */
    rc = cpssDxChBrgFdbMacTriggerModeSet(devNum,actionDataPtr->triggerMode);
    if (rc != GT_OK)
    {
        return rc;
    }

    /* restore  saved action mode */
    rc = cpssDxChBrgFdbActionModeSet(devNum,actionDataPtr->actionMode);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* restore saved action enable/disable mode */
    rc = cpssDxChBrgFdbActionsEnableSet(devNum,actionDataPtr->actionEnable);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* restore fdbUploadState enable/disable state */
    rc = cpssDxChBrgFdbUploadEnableSet(devNum,actionDataPtr->fdbUploadState);
    if (rc != GT_OK)
    {
        return rc;
    }
    if(CPSS_PP_FAMILY_DXCH_LION2_E == PRV_CPSS_PP_MAC(devNum)->devFamily)
    {
        /* restore  maskAuFuMsg2CpuOnNonLocal state*/
        rc = prvCpssDxChBrgFdbAuFuMessageToCpuOnNonLocalMaskEnableSet(devNum,actionDataPtr->maskAuFuMsg2CpuOnNonLocal);
    }

    return rc;
}

/*******************************************************************************
* dxChAuqStatusMemoryFree
*
* DESCRIPTION:
*       This function free previously allocated AUQ status memory.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Lion3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum       - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID dxChAuqStatusMemoryFree
(
    IN GT_U8    devNum
)
{
    if (auqMsgEnableStatus[devNum]->naToCpuPerPortPtr != NULL)
    {
        cpssOsFree(auqMsgEnableStatus[devNum]->naToCpuPerPortPtr);
        auqMsgEnableStatus[devNum]->naToCpuPerPortPtr = NULL;
    }
    if (auqMsgEnableStatus[devNum]->naStormPreventPortPtr != NULL)
    {
        cpssOsFree(auqMsgEnableStatus[devNum]->naStormPreventPortPtr);
        auqMsgEnableStatus[devNum]->naStormPreventPortPtr = NULL;
    }
    if (auqMsgEnableStatus[devNum] != NULL)
    {
        cpssOsFree(auqMsgEnableStatus[devNum]);
        auqMsgEnableStatus[devNum] = NULL;
    }
}

/*******************************************************************************
* dxChRestoreAuqCurrentStatus
*
* DESCRIPTION:
*       This function retieve  AUQ enable/disable message status
*       and apply it on the device.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum       - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChRestoreAuqCurrentStatus
(
    IN GT_U8    devNum
)
{
    GT_U32 i = 0;
    GT_STATUS rc = GT_OK;

    /*Restore sending NA update messages to the CPU per port*/

    for(i=0; i < PRV_CPSS_PP_MAC(devNum)->numOfPorts; i++)
    {
        /* skip not existed ports */
        if (! PRV_CPSS_PHY_PORT_IS_EXIST_MAC(devNum, i))
            continue;

        rc =  cpssDxChBrgFdbNaToCpuPerPortSet(devNum,(GT_U8)i,auqMsgEnableStatus[devNum]->naToCpuPerPortPtr[i]);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
        rc =  cpssDxChBrgFdbNaStormPreventSet(devNum,(GT_U8)i,auqMsgEnableStatus[devNum]->naStormPreventPortPtr[i]);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
    }

    /*Restore sending NA messages to the CPU indicating that the device
    cannot learn a new SA. */

    rc = cpssDxChBrgFdbNaMsgOnChainTooLongSet(devNum,auqMsgEnableStatus[devNum]->naToCpuLearnFail);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }
    /* Restore the status of Tag1 VLAN Id assignment in vid1 field of the NA AU
      message */
    if (PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT2_E)
    {
        rc = cpssDxChBrgFdbNaMsgVid1EnableSet(devNum,auqMsgEnableStatus[devNum]->naTag1VLANassignment);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
    }

    /* restore sending to CPU status of AA and TA messages*/
    rc = cpssDxChBrgFdbAAandTAToCpuSet(devNum,auqMsgEnableStatus[devNum]->aaTaToCpu);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }
    /* restore Sp AA message to CPU status*/
    rc =  cpssDxChBrgFdbSpAaMsgToCpuSet(devNum,auqMsgEnableStatus[devNum]->spAaMsgToCpu);
    dxChAuqStatusMemoryFree(devNum);

    return rc;
}

/*******************************************************************************
* dxChAuqFillByQuery
*
* DESCRIPTION:
*       The function fills AUQ and return the queueu state full/not full.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum        - the device number.
*       portGroupsBmp - bitmap of Port Groups.
*       macEntryPtr   - pointer to mac entry.
*
* OUTPUTS:
*       isAuqFullPtr - (pointer to) AUQ status:
*                       GT_TRUE - AUQ is full.
*                       GT_FALSE - otherwisw.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on bad devNum or portGroupsBmp or queueType.
*       GT_BAD_PTR               - one of the parameters is NULL pointer.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChAuqFillByQuery
(
    IN GT_U8                       devNum,
    IN  GT_PORT_GROUPS_BMP         portGroupsBmp,
    IN CPSS_MAC_ENTRY_EXT_KEY_STC  *macEntryPtr,
    OUT GT_BOOL                    *isAuqFullPtr
)
{
    GT_U32              qa_counter             = 0;
    GT_BOOL             auqIsFull              = GT_FALSE;
    GT_PORT_GROUPS_BMP  isFullPortGroupsBmp    = 0;
    GT_STATUS           rc                     = GT_OK;
    GT_PORT_GROUPS_BMP  completedPortGroupsBmp = 0;
    GT_PORT_GROUPS_BMP  succeededPortGroupsBmp = 0;

    while (auqIsFull == GT_FALSE)
    {
        /* check if AUQ full bit is set */
        rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_AU_E,&isFullPortGroupsBmp);
        if (rc != GT_OK)
        {
            return rc;
        }
        if((isFullPortGroupsBmp & portGroupsBmp)== portGroupsBmp)
        {
            /* queue is full */
            *isAuqFullPtr = GT_TRUE;
            return rc;
        }

        /* send quary */
        qa_counter = 0;
        do
        {
            rc =  cpssDxChBrgFdbPortGroupQaSend( devNum, portGroupsBmp, macEntryPtr);
            if(rc != GT_OK)
            {
    #ifdef ASIC_SIMULATION
                cpssOsTimerWkAfter(1);
    #endif
                qa_counter++;
                if(qa_counter > 20)
                {
                    return rc;
                }
            }
        } while (rc != GT_OK);

        /* verify that action is completed */
        completedPortGroupsBmp = 0;
        while ((completedPortGroupsBmp & portGroupsBmp)!= portGroupsBmp)
        {
            rc = cpssDxChBrgFdbPortGroupFromCpuAuMsgStatusGet(devNum,portGroupsBmp,&completedPortGroupsBmp,
                                                              &succeededPortGroupsBmp);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }
    return rc;
}



/*******************************************************************************
* dxChDisableAuqAndSaveAuqCurrentStatus
*
* DESCRIPTION:
*       This function disable AUQ for messages and save current AUQ messages enable status.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum       - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChDisableAuqAndSaveAuqCurrentStatus
(
    IN GT_U8    devNum
)
{
    GT_U32 i = 0;
    GT_STATUS rc = GT_OK;
    /* alocate memory for given device */
    auqMsgEnableStatus[devNum] = (PRV_CPSS_DXCH_AUQ_ENABLE_DATA_STC *)cpssOsMalloc(sizeof(PRV_CPSS_DXCH_AUQ_ENABLE_DATA_STC));
    if (auqMsgEnableStatus[devNum] == NULL)
    {
        return GT_OUT_OF_CPU_MEM;
    }
    cpssOsMemSet(auqMsgEnableStatus[devNum],0,sizeof(auqMsgEnableStatus));
    auqMsgEnableStatus[devNum]->naToCpuPerPortPtr = (GT_BOOL*)cpssOsMalloc(sizeof(GT_BOOL)* PRV_CPSS_PP_MAC(devNum)->numOfPorts);
    if (auqMsgEnableStatus[devNum]->naToCpuPerPortPtr == NULL)
    {
        dxChAuqStatusMemoryFree(devNum);
        return GT_OUT_OF_CPU_MEM;
    }
    cpssOsMemSet(auqMsgEnableStatus[devNum]->naToCpuPerPortPtr,0,sizeof(GT_BOOL)* PRV_CPSS_PP_MAC(devNum)->numOfPorts);
    auqMsgEnableStatus[devNum]->naStormPreventPortPtr = (GT_BOOL*)cpssOsMalloc(sizeof(GT_BOOL)* PRV_CPSS_PP_MAC(devNum)->numOfPorts);
    if (auqMsgEnableStatus[devNum]->naStormPreventPortPtr == NULL)
    {
        dxChAuqStatusMemoryFree(devNum);
        return GT_OUT_OF_CPU_MEM;
    }
    cpssOsMemSet(auqMsgEnableStatus[devNum]->naStormPreventPortPtr,0,sizeof(GT_BOOL)* PRV_CPSS_PP_MAC(devNum)->numOfPorts);
    /*Disable sending NA update messages to the CPU per port*/

    for(i=0; i < PRV_CPSS_PP_MAC(devNum)->numOfPorts; i++)
    {
        /* skip not existed ports */
        if (! PRV_CPSS_PHY_PORT_IS_EXIST_MAC(devNum, i))
            continue;

        /* at first save staus per port*/
        rc = cpssDxChBrgFdbNaToCpuPerPortGet(devNum,(GT_U8)i,&(auqMsgEnableStatus[devNum]->naToCpuPerPortPtr[i]));
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }

        rc =  cpssDxChBrgFdbNaStormPreventGet(devNum, (GT_U8)i,&(auqMsgEnableStatus[devNum]->naStormPreventPortPtr[i]));
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }

        rc =  cpssDxChBrgFdbNaToCpuPerPortSet(devNum,(GT_U8)i,GT_FALSE);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
        rc =  cpssDxChBrgFdbNaStormPreventSet(devNum, (GT_U8)i,GT_FALSE);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
    }

    /* save status (enabled/disabled) of sending NA messages to the CPU
       indicating that the device cannot learn a new SA */
    rc = cpssDxChBrgFdbNaMsgOnChainTooLongGet(devNum,&auqMsgEnableStatus[devNum]->naToCpuLearnFail);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }
    /* disable sending NA messages to the CPU
       indicating that the device cannot learn a new SA */
    rc = cpssDxChBrgFdbNaMsgOnChainTooLongSet(devNum,GT_FALSE);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }
    /* Get the status of Tag1 VLAN Id assignment in vid1 field of the NA AU
      message */
    if (PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_XCAT2_E)
    {
        rc = cpssDxChBrgFdbNaMsgVid1EnableGet(devNum,&auqMsgEnableStatus[devNum]->naTag1VLANassignment);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
        /* Disable sending of Tag1 VLAN Id assignment in vid1 field of the NA AU
          message */
        rc = cpssDxChBrgFdbNaMsgVid1EnableSet(devNum,GT_FALSE);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            return rc;
        }
    }

    /* save status of AA and AT messages */
    rc = cpssDxChBrgFdbAAandTAToCpuGet(devNum,&auqMsgEnableStatus[devNum]->aaTaToCpu);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }

   /* Disable AA and AT messages */
    rc =  cpssDxChBrgFdbAAandTAToCpuSet(devNum,GT_FALSE);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }

    /* save Sp AA message to CPU status*/
    rc =  cpssDxChBrgFdbSpAaMsgToCpuGet(devNum,&auqMsgEnableStatus[devNum]->spAaMsgToCpu);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }

    /* Disable sending AA messages to the CPU indicating that the
    device aged-out storm prevention FDB entry */
    rc =  cpssDxChBrgFdbSpAaMsgToCpuSet(devNum,GT_FALSE);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        return rc;
    }
    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChHaAuqNonContinuesMsgModeHandle
*
* DESCRIPTION:
*       This function performs AUQ workaround after HA event. It makes PP to consider that
*       queue is full and to be ready for reprogramming.
*       The workaround should be used when application can't guarantee the same memory
*       allocated for AUQ before and after HA event.
*       Before calling this function application should disable access of device to CPU memory.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum       - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChHaAuqNonContinuesMsgModeHandle
(
    IN GT_U8    devNum
)
{
    GT_STATUS                        rc;
    CPSS_MAC_ENTRY_EXT_KEY_STC       macEntry;
    GT_U32                           portGroupId;
    GT_PORT_GROUPS_BMP               portGroupsBmp = 0;
    GT_PORT_GROUPS_BMP               isFullPortGroupsBmp = 0;
    GT_BOOL                          auqIsFull = GT_FALSE;
    GT_U32                           regAddr = 0;
    GT_UINTPTR                       phyAddr;

    if((PRV_CPSS_HW_IF_PCI_COMPATIBLE_MAC(devNum)) &&
       (PRV_CPSS_DXCH_PP_MAC(devNum)->errata.info_PRV_CPSS_DXCH_XCAT_FDB_AU_FIFO_CORRUPT_WA_E.
        enabled == GT_FALSE))
    {
        /* disable AUQ for messages and save current AUQ enable status for given device */
        rc = prvCpssDxChDisableAuqAndSaveAuqCurrentStatus(devNum);
        if (rc != GT_OK)
        {
            return rc;
        }
        cpssOsMemSet(&macEntry,0,sizeof(CPSS_MAC_ENTRY_EXT_KEY_STC));
        macEntry.entryType                      = CPSS_MAC_ENTRY_EXT_TYPE_MAC_ADDR_E;
        macEntry.key.macVlan.vlanId             = 0;
        macEntry.key.macVlan.macAddr.arEther[0] = 0x0;
        macEntry.key.macVlan.macAddr.arEther[1] = 0x1A;
        macEntry.key.macVlan.macAddr.arEther[2] = 0xFF;
        macEntry.key.macVlan.macAddr.arEther[3] = 0xFF;
        macEntry.key.macVlan.macAddr.arEther[4] = 0xFF;
        macEntry.key.macVlan.macAddr.arEther[5] = 0xFF;

        PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
        {
            portGroupsBmp = (1 << portGroupId);
            auqIsFull = GT_FALSE;
            /* first check if AUQ is full*/
            rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_AU_E,&isFullPortGroupsBmp);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                return rc;
            }
            if((isFullPortGroupsBmp & portGroupsBmp)== portGroupsBmp)
            {
                /* WA sarts with AUQ full. In this case FIFO should be handled as well. */
                /* For this reason new AUQ of 64 messages size is defined. If there is  */
                /* something in FIFO it would be splashed into the new queue. After that*/
                /* this queue would be filled till the end by QR messages               */

                /* define queue size */
                regAddr =PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.auQControl;
                rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,0,31,64);
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    return rc;
                }
                regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.auQBaseAddr;
                /* read physical AUQ address from PP */
                rc =  prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,(GT_U32*)(&phyAddr));
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    return rc;
                }
                /* define queue base address */
                rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,regAddr,(GT_U32)phyAddr);
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    return rc;
                }

                /* now fill the FIFO queue */
                rc = dxChAuqFillByQuery(devNum,portGroupsBmp,&macEntry,&auqIsFull);
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    return rc;
                }
                if (auqIsFull == GT_TRUE)
                {
                    /* handle another port group */
                    continue;
                }
                else
                {
                    /* didn't succeed to fill AUQ */
                    dxChAuqStatusMemoryFree(devNum);
                    return GT_FAIL;
                }
            }
            /* now fill the queue */
            rc = dxChAuqFillByQuery(devNum,portGroupsBmp,&macEntry,&auqIsFull);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                return rc;
            }
            if (auqIsFull == GT_TRUE)
            {
                /* handle another port group */
                continue;
            }
            else
            {
                /* didn't succeed to fill AUQ */
                dxChAuqStatusMemoryFree(devNum);
                return GT_FAIL;
            }
        }
        PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    }
    return GT_OK;
}

/*******************************************************************************
* dxChScanFdbAndAddEntries
*
* DESCRIPTION:
*       This function scan FDB for valid entries and add special entires in order to
*       perform FUQ WA (making FUQ FULL) by optimal manner.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum               - device number.
*       portGroupsBmp        - bitmap of Port Groups.
*       fdbNumOfEntries      - number of entries in FDB
*       fuqSizeInEntries     - number of entries in current fuq
*       fdbEntryPtr          - pointer to special fdb entry
*
*
* OUTPUTS:
*       deleteEntryPtr       - pointer to boolean array contained indexes of added entries
*                              that should be deleted later.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChScanFdbAndAddEntries
(
    IN  GT_U8                         devNum,
    IN  GT_PORT_GROUPS_BMP            portGroupsBmp,
    IN  GT_U32                        fdbNumOfEntries,
    IN  GT_U32                        fuqSizeInEntries,
    IN  CPSS_MAC_ENTRY_EXT_STC        *fdbEntryPtr,
    OUT GT_BOOL                       *deleteEntryPtr
)
{
    GT_STATUS       rc = GT_OK;
    GT_U32          numberOfFdbEntriesToAdd = 0;
    GT_BOOL         valid;
    GT_U32          index = 0;
    GT_BOOL         skip = GT_FALSE;

    /* Initialize numberOfFdbEntriesToAdd to the the maximum of FU queue size and FDB size. */
    if (fuqSizeInEntries > fdbNumOfEntries)
    {
        numberOfFdbEntriesToAdd = fdbNumOfEntries;
    }
    else
    {
        numberOfFdbEntriesToAdd = fuqSizeInEntries;
    }
    /* First scan of the FDB: find how many entries to add to the FDB */
    for (index = 0; index < fdbNumOfEntries; index++)
    {
        /* call cpss api function */
        rc = cpssDxChBrgFdbPortGroupMacEntryStatusGet(devNum, portGroupsBmp, index, &valid, &skip);
        if (rc != GT_OK)
        {
            cpssOsFree(deleteEntryPtr);
            return rc;
        }
        if ((valid == GT_TRUE) && (skip == GT_FALSE))
        {
            numberOfFdbEntriesToAdd--;
        }
        if (numberOfFdbEntriesToAdd == 0)
        {
            break;
        }
    }
    /* Second scan of the FDB: add entries */
    for (index = 0; index < fdbNumOfEntries; index++)
    {
        if (numberOfFdbEntriesToAdd == 0)
        {
            break;
        }

        rc = cpssDxChBrgFdbPortGroupMacEntryStatusGet(devNum,portGroupsBmp, index, &valid, &skip);
        if (rc != GT_OK)
        {
            cpssOsFree(deleteEntryPtr);
            return rc;
        }
        if ((valid == GT_FALSE) || (skip == GT_TRUE))
        {
            /* write the entry to the FDB */
            rc = cpssDxChBrgFdbPortGroupMacEntryWrite(devNum, portGroupsBmp, index, GT_FALSE, fdbEntryPtr);
            if (rc != GT_OK)
            {
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
            deleteEntryPtr[index] = GT_TRUE;
            numberOfFdbEntriesToAdd--;
        }
    }
    return GT_OK;
}



/*******************************************************************************
* dxChFuqFillByUploadAction
*
* DESCRIPTION:
*       The function fills FUQ and return the queueu state full/not full.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum        - the device number.
*       portGroupsBmp - bitmap of Port Groups.
*       macEntryPtr   - pointer to mac entry.
*
* OUTPUTS:
*       isFuqFullPtr - (pointer to) FUQ status:
*                       GT_TRUE - FUQ is full.
*                       GT_FALSE - otherwisw.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on bad devNum or portGroupsBmp or queueType.
*       GT_BAD_PTR               - one of the parameters is NULL pointer.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS dxChFuqFillByUploadAction
(
    IN GT_U8                       devNum,
    IN  GT_PORT_GROUPS_BMP         portGroupsBmp,
    OUT GT_BOOL                    *isFuqFullPtr
)
{
    GT_STATUS rc = GT_OK;
    GT_PORT_GROUPS_BMP isFullPortGroupsBmp;
    GT_BOOL fuqIsFull = GT_FALSE;
    GT_BOOL actFinished = GT_FALSE;
    GT_U32 trigCounter = 0;

    rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_FU_E,&isFullPortGroupsBmp);
    if (rc != GT_OK)
    {
        return rc;
    }
    if((isFullPortGroupsBmp & portGroupsBmp)== portGroupsBmp)
    {
        fuqIsFull = GT_TRUE;
    }
    else
    {
        fuqIsFull = GT_FALSE;
    }
    /* fill all FUQs on the device*/
    while (fuqIsFull == GT_FALSE)
    {
        /*  force the upload trigger */
        rc =  cpssDxChBrgFdbMacTriggerToggle(devNum);
        if(rc != GT_OK)
        {
            return rc;
        }
        /* verify that action is completed */
        actFinished = GT_FALSE;
        trigCounter = 0;
        while (actFinished == GT_FALSE)
        {
            rc = cpssDxChBrgFdbTrigActionStatusGet(devNum,&actFinished);
            if(rc != GT_OK)
            {
                return rc;
            }
            if (actFinished == GT_FALSE)
            {
                trigCounter++;
            }
            else
            {
                trigCounter = 0;
                break;
            }
            if (trigCounter > 500)
            {
                rc =  prvCpssDrvHwPpSetRegField(devNum,
                                                PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bridgeRegs.macTblAction0,
                                                1, 1, 0);
                if (rc != GT_OK)
                {
                    return rc;
                }
            }
        }
        rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_FU_E,&isFullPortGroupsBmp);
        if (rc != GT_OK)
        {
            return rc;
        }
        if((isFullPortGroupsBmp & portGroupsBmp)== portGroupsBmp)
        {
            fuqIsFull = GT_TRUE;
        }
        else
        {
            fuqIsFull = GT_FALSE;
        }
    }
    *isFuqFullPtr = GT_TRUE;
    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChHaFuqNonContinuesMsgModeHandle
*
* DESCRIPTION:
*       This function performs FUQ workaround after HA event. It makes PP to consider that
*       queue is full and to be ready for reprogramming.
*       The workaround should be used when application can't guarantee the same memory
*       allocated for FUQ before and after HA event.
*       Before calling this function application should disable access of device to CPU memory.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum       - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChHaFuqNonContinuesMsgModeHandle
(
    IN GT_U8    devNum
)
{
    GT_STATUS                         rc = GT_OK;
    GT_U32                            regAddr;
    GT_BOOL                           actionCompleted = GT_FALSE;
    GT_U32                            inProcessBlocksBmp = 0;
    GT_U32                            inProcessBlocksBmp1 = 0;
    GT_U32                            fdbNumOfEntries;
    GT_BOOL                           *deleteEntryPtr = NULL;
    CPSS_MAC_ENTRY_EXT_STC            fdbEntry;
    GT_PORT_GROUPS_BMP                isFullPortGroupsBmp = 0;
    GT_PORT_GROUPS_BMP                portGroupsBmp = 0;
    GT_UINTPTR                        phyAddr;
    GT_BOOL                           fuqIsFull = GT_FALSE;
    GT_U32                            i = 0;
    GT_U32                            fuqSize = 0;
    PRV_CPSS_DXCH_FUQ_ACTION_DATA_STC actionData;
    GT_U32                            portGroupId = 0;
    GT_BOOL                           cncUploadIsHandled = GT_FALSE;

    cpssOsMemSet(&actionData,0,sizeof(PRV_CPSS_DXCH_FUQ_ACTION_DATA_STC));
    fdbNumOfEntries = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.fdb;
    /* Get address of FDB Action0 register */
    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bridgeRegs.macTblAction0;

    /* check that there is not not-finished FDB upload */
    rc = cpssDxChBrgFdbTrigActionStatusGet(devNum, &actionCompleted);
    if (rc != GT_OK)
    {
        return rc;
    }
    if(actionCompleted == GT_FALSE)
    {
        /* clear the trigger */
        rc = prvCpssDrvHwPpSetRegField(devNum, regAddr, 1, 1, 0);
        if (rc != GT_OK)
        {
            return rc;
        }
    }
    /* check there are no CNC blocks yet being uploaded */
    rc = cpssDxChCncBlockUploadInProcessGet(devNum,&inProcessBlocksBmp);
    if (rc != GT_OK)
    {
        return rc;
    }
    if (inProcessBlocksBmp != 0)
    {
        /* if CNC upload is under way let it be finished */
        cpssOsTimerWkAfter(10);
    }
    inProcessBlocksBmp = 0;
    deleteEntryPtr = (GT_BOOL *)cpssOsMalloc(sizeof(GT_BOOL) * fdbNumOfEntries);
    if (deleteEntryPtr == NULL)
    {
        return GT_OUT_OF_CPU_MEM;
    }

    /* disable AUQ for messages and save current AUQ enable status for given device */
    rc = prvCpssDxChDisableAuqAndSaveAuqCurrentStatus(devNum);
    if (rc != GT_OK)
    {
        return rc;
    }
    /* configure FDB upload action for specific entry and save current FUQ action status */
    rc = dxChEnableFdbUploadActionAndSaveFuqCurrentStatus(devNum,&actionData);
    if (rc != GT_OK)
    {
        dxChAuqStatusMemoryFree(devNum);
        cpssOsFree(deleteEntryPtr);
        return rc;
    }

    cpssOsMemSet(&fdbEntry,0,sizeof(CPSS_MAC_ENTRY_EXT_STC));

    /* fill very specific fdb entry  */
    fdbEntry.key.entryType = CPSS_MAC_ENTRY_EXT_TYPE_MAC_ADDR_E;
    fdbEntry.key.key.macVlan.vlanId = 9;
    fdbEntry.key.key.macVlan.macAddr.arEther[0] = 0;
    fdbEntry.key.key.macVlan.macAddr.arEther[1] = 0x15;
    fdbEntry.key.key.macVlan.macAddr.arEther[2] = 0x14;
    fdbEntry.key.key.macVlan.macAddr.arEther[3] = 0x13;
    fdbEntry.key.key.macVlan.macAddr.arEther[4] = 0x12;
    fdbEntry.key.key.macVlan.macAddr.arEther[5] = 0x11;
    fdbEntry.dstInterface.devPort.hwDevNum = 30;
    fdbEntry.dstInterface.devPort.portNum = 62;
    fdbEntry.dstInterface.type = CPSS_INTERFACE_PORT_E;
    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    {
        cpssOsMemSet(deleteEntryPtr,0,sizeof(GT_BOOL)* fdbNumOfEntries);
        portGroupsBmp = (1 << portGroupId);
        /* first check if AUQ is full*/
        rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_AU_E,&isFullPortGroupsBmp);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            cpssOsFree(deleteEntryPtr);
            return rc;
        }
        if((isFullPortGroupsBmp & portGroupsBmp)== portGroupsBmp)
        {
            /* WA sarts with AUQ full. In this case FIFO can be full as well. */
            /* For this reason new AUQ of 64 messages size is defined. If there is  */
            /* something in FIFO it would be splashed into the new queue.         */

            /* define queue size */
            regAddr =PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.auQControl;
            rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,0,31,64);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.auQBaseAddr;
            /* read physical AUQ address from PP */
            rc =  prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,(GT_U32*)(&phyAddr));
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
            /* define queue base address */
            rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,regAddr,(GT_U32)phyAddr);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
        }

        /* check if FUQ is full */
        rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_FU_E,&isFullPortGroupsBmp);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            cpssOsFree(deleteEntryPtr);
            return rc;
        }

        if((isFullPortGroupsBmp & portGroupsBmp)== portGroupsBmp)
        {
            /* WA sarts with FUQ full. In this case CNC upload action check is performed. */
            /* If upload action is ongoing lets it be finished by defining new fuq with   */
            /* CNC block size. (Asumtion is CNC upload command is given for different CNC */
            /* blocks sequentially  - one in a time).This process is proceeded until CNC  */
            /* is not finished.                                                           */
            /* If CNC upload is not a factor, FIFO should be handled as well.             */
            /* For this reason new FUQ of 64 messages size is defined. If there is        */
            /* something in FIFO it would be splashed into the new queue. After that      */
            /* this queue would be filled till the end by FDB upload action               */

            /* reset FDB action trigger if any */
            rc =  prvCpssDrvHwPpSetRegField(devNum,
                                            PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bridgeRegs.macTblAction0,
                                            1, 1, 0);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
            /* check if CNC upload takes place */
            rc =  cpssDxChCncPortGroupBlockUploadInProcessGet(devNum, portGroupsBmp, &inProcessBlocksBmp);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
            if (inProcessBlocksBmp == 0)
            {
                /* cnc was not triggered and queue is full. The FIFO contents is unknown */
                /* In order to avoid CNC entries in FIFO define new queue by FIFO size   */
                fuqSize = 64;
                regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQControl;
                rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,0,30,fuqSize);
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    cpssOsFree(deleteEntryPtr);
                    return rc;
                }
                regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQBaseAddr;
                /* read physical FUQ address from PP */
                rc =  prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,(GT_U32*)(&phyAddr));
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    cpssOsFree(deleteEntryPtr);
                    return rc;
                }
                /* define queue base address */
                rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,regAddr,(GT_U32)phyAddr);
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    cpssOsFree(deleteEntryPtr);
                    return rc;
                }
            }
            else
            {
                cncUploadIsHandled = GT_TRUE;
                while (inProcessBlocksBmp != 0)
                {
                    /* only one block can be uploaded in given time */
                    inProcessBlocksBmp1 = inProcessBlocksBmp;
                    isFullPortGroupsBmp = 0;
                    rc = cpssDxChBrgFdbPortGroupQueueFullGet(devNum,portGroupsBmp,CPSS_DXCH_FDB_QUEUE_TYPE_FU_E,&isFullPortGroupsBmp);
                    if (rc != GT_OK)
                    {
                        dxChAuqStatusMemoryFree(devNum);
                        cpssOsFree(deleteEntryPtr);
                        return rc;
                    }
                    if((isFullPortGroupsBmp & portGroupsBmp)!= portGroupsBmp)
                    {
                        /* queue is not full and cnc dump is not finished */
                        dxChAuqStatusMemoryFree(devNum);
                        cpssOsFree(deleteEntryPtr);
                        return GT_FAIL;
                    }
                    fuqSize = PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.tableSize.cncBlockNumEntries + 64;/* cnc block size + FIFO */
                    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQControl;
                    rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,0,30,fuqSize);
                    if (rc != GT_OK)
                    {
                        dxChAuqStatusMemoryFree(devNum);
                        cpssOsFree(deleteEntryPtr);
                        return rc;
                    }
                    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQBaseAddr;
                    /* read physical FUQ address from PP */
                    rc =  prvCpssDrvHwPpPortGroupReadRegister(devNum,portGroupId,regAddr,(GT_U32*)(&phyAddr));
                    if (rc != GT_OK)
                    {
                        dxChAuqStatusMemoryFree(devNum);
                        cpssOsFree(deleteEntryPtr);
                        return rc;
                    }
                    /* define queue base address */
                    rc = prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,regAddr,(GT_U32)phyAddr);
                    if (rc != GT_OK)
                    {
                        dxChAuqStatusMemoryFree(devNum);
                        cpssOsFree(deleteEntryPtr);
                        return rc;
                    }
                    i = 0;
                    while (inProcessBlocksBmp == inProcessBlocksBmp1)
                    {
                        i++;
                        if (i > 1000)
                        {
                            dxChAuqStatusMemoryFree(devNum);
                            cpssOsFree(deleteEntryPtr);
                            return GT_FAIL;
                        }
                        /* check if CNC upload takes place */
                        rc =  cpssDxChCncPortGroupBlockUploadInProcessGet(devNum, portGroupsBmp, &inProcessBlocksBmp);
                        if (rc != GT_OK)
                        {
                            dxChAuqStatusMemoryFree(devNum);
                            cpssOsFree(deleteEntryPtr);
                            return rc;
                        }
                    }
                }
            }
        }
        else
        {
            /* fuq is not full*/
            /* get current fuq size */
            regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQControl;
            rc = prvCpssDrvHwPpPortGroupGetRegField(devNum,portGroupId,regAddr,0,30,&fuqSize);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
        }
        /* in this point queue is not full : or from begining or new queue size 64  was defined  or  */
        /* new queue CNC size was defined.                                                           */
        /* now fill the current fuq by means of fdb upload action                                    */
        if (cncUploadIsHandled == GT_TRUE)
        {
            /* cnc upload was already handled */
            rc =  cpssDxChCncPortGroupBlockUploadInProcessGet(devNum, portGroupsBmp, &inProcessBlocksBmp);
            if (rc != GT_OK)
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return rc;
            }
            if (inProcessBlocksBmp == 0)
            {
                /* set FDB to be the Queue owner */
                regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.globalControl;
                rc = prvCpssDrvHwPpPortGroupSetRegField(devNum, portGroupId,regAddr, 14, 1, 1);
                if (rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    cpssOsFree(deleteEntryPtr);
                    return rc;
                }
            }
            else
            {
                dxChAuqStatusMemoryFree(devNum);
                cpssOsFree(deleteEntryPtr);
                return GT_FAIL;
            }

        }
        rc = dxChScanFdbAndAddEntries(devNum,portGroupsBmp,fdbNumOfEntries,fuqSize,&fdbEntry,deleteEntryPtr);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            cpssOsFree(deleteEntryPtr);
            return rc;
        }
        rc =  dxChFuqFillByUploadAction(devNum, portGroupsBmp,&fuqIsFull);
        if (rc != GT_OK)
        {
            dxChAuqStatusMemoryFree(devNum);
            cpssOsFree(deleteEntryPtr);
            return rc;
        }
        if (fuqIsFull == GT_FALSE)
        {
            /* queue is still not full*/
            dxChAuqStatusMemoryFree(devNum);
            cpssOsFree(deleteEntryPtr);
            return GT_FAIL;
        }

         /* Now queue is full . Restore FDB configuration */
        for (i = 0; i < fdbNumOfEntries; i++)
        {
            if (deleteEntryPtr[i] == GT_TRUE)
            {
                rc = prvCpssDxChPortGroupWriteTableEntryField(devNum,
                                                              portGroupId,
                                                              PRV_CPSS_DXCH_TABLE_FDB_E,
                                                              i,
                                                              0,
                                                              1,
                                                              1,
                                                              1);
                if(rc != GT_OK)
                {
                    dxChAuqStatusMemoryFree(devNum);
                    cpssOsFree(deleteEntryPtr);
                    return rc;
                }
            }
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    cpssOsFree(deleteEntryPtr);
    /* restore configuration */
    rc =  dxChRestoreCurrentFdbActionStatus(devNum, &actionData);
    if (rc != GT_OK)
    {
        return rc;
    }
    rc =  prvCpssDxChRestoreAuqCurrentStatus(devNum);
    if (rc != GT_OK)
    {
        return rc;
    }

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)
    {
        /* Disable FUQ for each portgroup to prevent splashing FIFO into new defined queue */
        /* Further in the catch up stage fuq would be reenabled                            */
        regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->globalRegs.fuQControl;
        rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,31,1,0);
        if (rc != GT_OK)
        {
            return rc;
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_MAC(devNum,portGroupId)

    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChSystemRecoveryCatchUpDiffMemoryAuHandle
*
* DESCRIPTION:
*       Synchronize AUQ software pointers by its hw values.
*
* APPLICABLE DEVICES:
*         Lion; Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PARAM             - on bad device
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       This function could be used only if application couldn't guarantee constant
*       surviving cpu restart memory for AUQ/FUQ allocation.
*
*******************************************************************************/
GT_STATUS prvCpssDxChSystemRecoveryCatchUpDiffMemoryAuHandle
(
    IN GT_U8    devNum
)
{
    GT_STATUS rc = GT_OK;
    rc =  prvCpssDxChRestoreAuqCurrentStatus(devNum);
    return rc;
}


