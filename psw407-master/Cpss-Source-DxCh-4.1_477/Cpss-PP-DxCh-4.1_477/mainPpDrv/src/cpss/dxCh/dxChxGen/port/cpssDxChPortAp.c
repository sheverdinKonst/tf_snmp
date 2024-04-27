/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* cpssGenPortAp.c
*
* DESCRIPTION:
*       CPSS implementation for 802.3ap standard (defines the auto negotiation
*       for backplane Ethernet) configuration and control facility.
*
* FILE REVISION NUMBER:
*       $Revision: 12 $
*******************************************************************************/

/* macro needed to support the call to PRV_CPSS_PHYSICAL_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC */
/* this define must come before include files */
#define PRV_CPSS_PHYSICAL_GLOBAL_PORT_TO_PORT_GROUP_ID_SUPPORTED_FLAG_CNS
#define CPSS_LOG_IN_MODULE_ENABLE
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortLog.h>
#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/config/private/prvCpssConfigTypes.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortAp.h>

#include <common/siliconIf/mvSiliconIf.h>
#include <port/mvHwsPortApInitIf.h>
#include <private/mvPortModeElements.h>

#define AP_DBG_EN
#ifdef AP_DBG_EN
    static GT_U32   allowPrint=0;/* option to disable the print in runtime*/
    #define AP_DBG_PRINT_MAC(x) if(allowPrint) cpssOsPrintSync x

extern GT_U32 apPrintControl(IN GT_U32  allowPrintNew)
{
    GT_U32  oldState = allowPrint;
    allowPrint = allowPrintNew;

    return oldState;
}

static GT_U32   apSemPrintEn = 0; /* option to disable the print in runtime*/
extern GT_U32 apSemPrint(IN GT_U32  allowPrintNew)
{
    GT_U32  oldState = apSemPrintEn;
    apSemPrintEn = allowPrintNew;

    return oldState;
}

#else
    #define AP_DBG_PRINT_MAC(x)
#endif

/* CPSS suggested defaults for AP configuration */
static CPSS_DXCH_PORT_AP_PARAMS_STC prvCpssDxChPortApDefaultParams =
{
    /* fcPause */  GT_TRUE,
    /* fcAsmDir */ CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E,
    /* fecSupported */ GT_TRUE,
    /* fecRequired */ GT_FALSE,
    /* noneceDisable */ GT_TRUE, CPSS_TBD_BOOKMARK_LION2 /* to fix when Z80 algorithm fixed */
    /* laneNum */       0,
    /* modesAdvertiseArr */
    {
        {CPSS_PORT_INTERFACE_MODE_1000BASE_X_E, CPSS_PORT_SPEED_1000_E}
        ,{CPSS_PORT_INTERFACE_MODE_XGMII_E, CPSS_PORT_SPEED_10000_E}
        ,{CPSS_PORT_INTERFACE_MODE_KR_E, CPSS_PORT_SPEED_10000_E}
        ,{CPSS_PORT_INTERFACE_MODE_KR_E, CPSS_PORT_SPEED_40000_E}
        ,{CPSS_PORT_INTERFACE_MODE_REDUCED_10BIT_E, CPSS_PORT_SPEED_NA_E}
    }
};

/*******************************************************************************
* internal_cpssDxChPortApEnableSet
*
* DESCRIPTION:
*       Enable/disable AP engine (loads AP code into shared memory and starts AP
*       engine).
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portGroupsBmp   - bitmap of cores where to run AP engine
*                           (0x7FFFFFF - for ALL)
*       enable  - GT_TRUE  - enable AP on port group
*                 GT_FALSE - disbale
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApEnableSet
(
    IN  GT_U8               devNum,
    IN  GT_PORT_GROUPS_BMP  portGroupsBmp,
    IN  GT_BOOL             enable
)
{
    GT_U32      portGroupId;/* local core number */
    GT_STATUS   rc;         /* return code */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                          | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    AP_DBG_PRINT_MAC(("cpssDxChPortApEnableSet:devNum=%d,portGroupsBmp=%d,enable=%d\n", 
                        devNum, portGroupsBmp, enable));
    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum, portGroupsBmp,
                                                      portGroupId)
    {
        if(enable)
        {
            CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApEngineInit(devNum[%d], portGroupId[%d])", devNum, portGroupId);
            rc = mvHwsApEngineInit(devNum,portGroupId);
        }
        else
        {
			CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApEngineStop(devNum[%d], portGroupId[%d])", devNum, portGroupId);
            rc = mvHwsApEngineStop(devNum,portGroupId);
        }
        if(rc != GT_OK)
        {
			CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
            return rc;
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum, portGroupsBmp,
                                                    portGroupId)

    return GT_OK;
}

/*******************************************************************************
* cpssDxChPortApEnableSet
*
* DESCRIPTION:
*       Enable/disable AP engine (loads AP code into shared memory and starts AP
*       engine).
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portGroupsBmp   - bitmap of cores where to run AP engine
*                           (0x7FFFFFF - for ALL)
*       enable  - GT_TRUE  - enable AP on port group
*                 GT_FALSE - disbale
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS cpssDxChPortApEnableSet
(
    IN  GT_U8               devNum,
    IN  GT_PORT_GROUPS_BMP  portGroupsBmp,
    IN  GT_BOOL             enable
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApEnableSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portGroupsBmp, enable));

    rc = internal_cpssDxChPortApEnableSet(devNum, portGroupsBmp, enable);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portGroupsBmp, enable));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChPortApEnableGet
*
* DESCRIPTION:
*       Get AP engine enabled and functional on port group (local core) status.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portGroupNum - number of port group (local core)
*
* OUTPUTS:
*       enablePtr  - GT_TRUE  - AP enabled and functional on port group
*                    GT_FALSE - disabled or not functional
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port group or device
*       GT_BAD_PTR               - enabledPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApEnableGet
(
    IN  GT_U8   devNum,
    IN  GT_U32  portGroupNum,
    OUT GT_BOOL *enabledPtr
)
{
    GT_STATUS       rc;         /* return code */
    GT_U32 counter1;   /* AP watchdog value first read */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                          | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);
    CPSS_NULL_PTR_CHECK_MAC(enabledPtr);

	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApEngineInitGet(devNum[%d], portGroupNum[%d], *enabledPtr)", devNum, portGroupNum);
    (GT_VOID)mvHwsApEngineInitGet(devNum,portGroupNum,enabledPtr);

    if(GT_FALSE == *enabledPtr)
    {
        AP_DBG_PRINT_MAC(("mvHwsApEngineInitGet:enabled=%d\n", *enabledPtr));
        return GT_OK;
    }
	CPSS_LOG_INFORMATION_MAC("Calling: mvApCheckCounterGet(devNum[%d], portGroupNum[%d], *counter1)", devNum, portGroupNum);
    rc = mvApCheckCounterGet(devNum,portGroupNum,&counter1);
    AP_DBG_PRINT_MAC(("mvApCheckCounterGet_1:rc=%d,counter=%d\n", rc, counter1));
    if(rc != GT_OK)
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
        return rc;
    }

    return GT_OK;
}

/*******************************************************************************
* cpssDxChPortApEnableGet
*
* DESCRIPTION:
*       Get AP engine enabled and functional on port group (local core) status.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portGroupNum - number of port group (local core)
*
* OUTPUTS:
*       enablePtr  - GT_TRUE  - AP enabled and functional on port group
*                    GT_FALSE - disabled or not functional
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port group or device
*       GT_BAD_PTR               - enabledPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS cpssDxChPortApEnableGet
(
    IN  GT_U8   devNum,
    IN  GT_U32  portGroupNum,
    OUT GT_BOOL *enabledPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApEnableGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portGroupNum, enabledPtr));

    rc = internal_cpssDxChPortApEnableGet(devNum, portGroupNum, enabledPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portGroupNum, enabledPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* prvCpssDxChPortApModesVectorBuild
*
* DESCRIPTION:
*       Get array of port modes in CPSS format and build advertisement array in
*       HWS format.
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - port number
*       modesAdvertiseArrPtr - array of advertised modes in CPSS format
*
* OUTPUTS:
*       modesVectorPtr - bitmap of advertised modes in HWS format
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS prvCpssDxChPortApModesVectorBuild
(
    IN  GT_U8                    devNum,
    IN  GT_PHYSICAL_PORT_NUM     portNum,
    IN  CPSS_PORT_MODE_SPEED_STC *modesAdvertiseArrPtr,
    OUT GT_U32                   *modesVectorPtr
)
{
    GT_STATUS   rc; /* return code */
    GT_U32      i;   /* iterator */
    GT_U32      localPort; /* number of port in core */
    GT_U32      macNum;     /* number of used MAC unit */
    GT_U32      pcsNum;     /* number of used PCS unit */
    GT_U32      sdVecSize;         /* size of serdes redundancy array */
    GT_U32      *sdVectorPtr;      /* serdes redundancy array */

    localPort = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

    for(i = 0, *modesVectorPtr = 0; (modesAdvertiseArrPtr[i].ifMode !=
                                            CPSS_PORT_INTERFACE_MODE_REDUCED_10BIT_E)
                                        && (i < CPSS_DXCH_PORT_AP_IF_ARRAY_SIZE_CNS);
         i++)
    {
        switch(modesAdvertiseArrPtr[i].ifMode)
        {
            case CPSS_PORT_INTERFACE_MODE_SGMII_E:
            case CPSS_PORT_INTERFACE_MODE_1000BASE_X_E:
                if(modesAdvertiseArrPtr[i].speed != CPSS_PORT_SPEED_1000_E)
                {
                    return GT_BAD_PARAM;
                }
				CPSS_LOG_INFORMATION_MAC("Calling: hwsPortsParamsCfgGet(devNum[%d], portGroup[%d], portNum[%d], portMode[%s], *macNum, *pcsNum, *sdVecSize, **sdVector)", devNum, 0, localPort, "_1000Base_X");
				rc = hwsPortsParamsCfgGet(devNum, 0, localPort, _1000Base_X, &macNum,
                                          &pcsNum, &sdVecSize, &sdVectorPtr);
                if(rc != GT_OK)
                {
					CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
                    return rc;
                }
                if(NA_NUM == macNum)
                {
					CPSS_LOG_INFORMATION_MAC("Hws return GT_BAD_PARAM");
                    return GT_BAD_PARAM;
                }
                *modesVectorPtr |= _1000Base_KX_Bit0;
                break;
            case CPSS_PORT_INTERFACE_MODE_XGMII_E:
                if(modesAdvertiseArrPtr[i].speed != CPSS_PORT_SPEED_10000_E)
                {
                    return GT_BAD_PARAM;
                }
				CPSS_LOG_INFORMATION_MAC("Calling: hwsPortsParamsCfgGet(devNum[%d], portGroup[%d], portNum[%d], portMode[%s], *macNum, *pcsNum, *sdVecSize, **sdVector)", devNum, 0, localPort, "_10GBase_KX4");
				rc = hwsPortsParamsCfgGet(devNum, 0, localPort, _10GBase_KX4, &macNum,
                                          &pcsNum, &sdVecSize, &sdVectorPtr);
                if(rc != GT_OK)
                {
					CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
                    return rc;
                }
                if(NA_NUM == macNum)
                {
					CPSS_LOG_INFORMATION_MAC("Hws return GT_BAD_PARAM");
                    return GT_BAD_PARAM;
                }
                *modesVectorPtr |= _10GBase_KX4_Bit1;
                break;
            case CPSS_PORT_INTERFACE_MODE_KR_E:
                if(CPSS_PORT_SPEED_10000_E == modesAdvertiseArrPtr[i].speed)
                {
					CPSS_LOG_INFORMATION_MAC("Calling: hwsPortsParamsCfgGet(devNum[%d], portGroup[%d], portNum[%d], portMode[%s], *macNum, *pcsNum, *sdVecSize, **sdVector)", devNum, 0, localPort, "_10GBase_KR");
                    rc = hwsPortsParamsCfgGet(devNum, 0, localPort, _10GBase_KR, &macNum,
                                              &pcsNum, &sdVecSize, &sdVectorPtr);
                    *modesVectorPtr |= _10GBase_KR_Bit2;
                }
                else if(CPSS_PORT_SPEED_40000_E == modesAdvertiseArrPtr[i].speed)
                {
					CPSS_LOG_INFORMATION_MAC("Calling: hwsPortsParamsCfgGet(devNum[%d], portGroup[%d], portNum[%d], portMode[%s], *macNum, *pcsNum, *sdVecSize, **sdVector)", devNum, 0, localPort, "_40GBase_KR");
                    rc = hwsPortsParamsCfgGet(devNum, 0, localPort, _40GBase_KR, &macNum,
                                              &pcsNum, &sdVecSize, &sdVectorPtr);
                    *modesVectorPtr |= _40GBase_KR4_Bit3;
                }
                else
                {
                    return GT_BAD_PARAM;
                }

                if(rc != GT_OK)
                {
					CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
                    return rc;
                }
                if(NA_NUM == macNum)
                {
					CPSS_LOG_INFORMATION_MAC("Hws return GT_BAD_PARAM");
                    return GT_BAD_PARAM;
                }

                break;
            default:
                return GT_BAD_PARAM;
        }
    }

    return GT_OK;
}

/*******************************************************************************
* internal_cpssDxChPortApPortConfigSet
*
* DESCRIPTION:
*       Enable/disable AP process on port.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - number of physical port
*       apEnable    - AP enable/disable on port
*       apParamsPtr - (ptr to) AP parameters for port
*                               (NULL - for CPSS defaults).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   For now allowed negotiation on serdes lanes of port 0-3, because in Lion2
*   just these options are theoreticaly possible.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApPortConfigSet
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    IN  GT_BOOL                         apEnable,
    IN  CPSS_DXCH_PORT_AP_PARAMS_STC    *apParamsPtr
)
{
    GT_STATUS                rc;        /* return code */
    GT_U32                   portGroup; /* local core number */
    GT_U32                   phyPortNum;/* port number in local core */
    MV_HWS_AP_CFG            apCfg;     /* AP parameters of port in HWS format */
    CPSS_DXCH_PORT_AP_PARAMS_STC  *localApParamsPtr;/* temporary pointer to
                                        AP configuration parameters structure */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                          | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);
    PRV_CPSS_DXCH_PHY_PORT_CHECK_MAC(devNum, portNum);

    AP_DBG_PRINT_MAC(("cpssDxChPortApPortConfigSet:devNum=%d,portNum=%d,apEnable=%d\n", 
                        devNum, portNum, apEnable));

    portGroup = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portNum);
    phyPortNum = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

    if(GT_FALSE == apEnable)
    {
		CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApPortStop(devNum[%d], portGroup[%d], phyPortNum[%d], action[%s])", devNum, portGroup, phyPortNum, "PORT_POWER_DOWN");
		rc = mvHwsApPortStop(devNum,portGroup,phyPortNum,PORT_POWER_DOWN);
		if(rc != GT_OK)
		{
			CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
		}
        return rc;
    }

    localApParamsPtr = (apParamsPtr != NULL) ? apParamsPtr :
                                                &prvCpssDxChPortApDefaultParams;

    rc = prvCpssDxChPortApModesVectorBuild(devNum, portNum,
                                           localApParamsPtr->modesAdvertiseArr,
                                           &apCfg.modesVector);
    if(rc != GT_OK)
    {
        return rc;
    }

    if((localApParamsPtr->fcAsmDir != CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E)
        && (localApParamsPtr->fcAsmDir != CPSS_DXCH_PORT_AP_FLOW_CONTROL_ASYMMETRIC_E))
    {
        return GT_BAD_PARAM;
    }

    apCfg.fcAsmDir  = localApParamsPtr->fcAsmDir;
    apCfg.fcPause   = localApParamsPtr->fcPause;
    if((apCfg.modesVector & _10GBase_KR_Bit2) ||
       (apCfg.modesVector & _40GBase_KR4_Bit3))
    {
        apCfg.fecReq    = localApParamsPtr->fecRequired;
        apCfg.fecSup    = localApParamsPtr->fecSupported;
    }
    else
    {
        apCfg.fecReq    = GT_FALSE;
        apCfg.fecSup    = GT_FALSE;
    }
    apCfg.nonceDis  = localApParamsPtr->noneceDisable;
    if(localApParamsPtr->laneNum > 3)
    {
        return GT_BAD_PARAM;
    }
    apCfg.apLaneNum = localApParamsPtr->laneNum;
    apCfg.refClockCfg.refClockSource = PRIMARY_LINE_SRC;

    rc = prvCpssDxChSerdesRefClockTranslateCpss2Hws(devNum,
                                                 &apCfg.refClockCfg.refClockFreq);
    if (rc != GT_OK)
    {
        return rc;
    }

    AP_DBG_PRINT_MAC(("mvHwsApPortStart:devNum=%d,portGroup=%d,phyPortNum=%d,\
laneNum=%d,modesVector=0x%x,fcAsmDir=%d,fcPause=%d,fecReq=%d,fecSup=%d,nonceDis=%d,\
refClock=%d,refClockSource=%d\n",
                      devNum, portGroup, phyPortNum, apCfg.apLaneNum,
                      apCfg.modesVector,apCfg.fcAsmDir,apCfg.fcPause,apCfg.fecReq,
                      apCfg.fecSup, apCfg.nonceDis, apCfg.refClockCfg.refClockFreq,
                      apCfg.refClockCfg.refClockSource));

CPSS_TBD_BOOKMARK_LION2 /* for now optimization algorithms cause AP excide 500 mSec */
    rc = cpssDxChPortSerdesAutoTuneOptAlgSet(devNum, portNum,
                                             CPSS_PORT_SERDES_TRAINING_OPTIMISATION_NONE_E);
    if (rc != GT_OK)
    {
        return rc;
    }
	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApPortStart(devNum[%d], portGroup[%d], phyPortNum[%d], apCfg{apLaneNum[%d], modesVector[%d], fcPause[%d], fcAsmDir[%d], fecSup[%d], fecReq[%d], nonceDis[%d], refClockCfg{refClockFreq[%d], refClockSource[%d]}})", 
						devNum, portGroup, phyPortNum, apCfg.apLaneNum,apCfg.modesVector,apCfg.fcPause,apCfg.fcAsmDir,apCfg.fecSup,apCfg.fecReq, apCfg.nonceDis, apCfg.refClockCfg.refClockFreq,apCfg.refClockCfg.refClockSource);

	rc =  mvHwsApPortStart(devNum,
                            portGroup,
                            phyPortNum,
                            &apCfg);
	if(rc != GT_OK)
	{
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
	}
	return rc;
}

/*******************************************************************************
* cpssDxChPortApPortConfigSet
*
* DESCRIPTION:
*       Enable/disable AP process on port.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - number of physical port
*       apEnable    - AP enable/disable on port
*       apParamsPtr - (ptr to) AP parameters for port
*                               (NULL - for CPSS defaults).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   For now allowed negotiation on serdes lanes of port 0-3, because in Lion2
*   just these options are theoreticaly possible.
*
*******************************************************************************/
GT_STATUS cpssDxChPortApPortConfigSet
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    IN  GT_BOOL                         apEnable,
    IN  CPSS_DXCH_PORT_AP_PARAMS_STC    *apParamsPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApPortConfigSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, apEnable, apParamsPtr));

    rc = internal_cpssDxChPortApPortConfigSet(devNum, portNum, apEnable, apParamsPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, apEnable, apParamsPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChPortApPortConfigGet
*
* DESCRIPTION:
*       Get AP configuration of port.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum     - physical device number
*       portNum    - physical port number
*
* OUTPUTS:
*       apEnablePtr - AP enable/disable on port
*       apParamsPtr - (ptr to) AP parameters of port
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_BAD_PTR               - apEnablePtr or apParamsPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApPortConfigGet
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    OUT GT_BOOL                         *apEnablePtr,
    OUT CPSS_DXCH_PORT_AP_PARAMS_STC    *apParamsPtr
)
{
    GT_STATUS       rc;         /* return code */
    MV_HWS_AP_CFG   apCfg;      /* AP configuration parameters */
    GT_U32          portGroup;  /* local core number */
    GT_U32          phyPortNum; /* port number in local core */
    GT_U32          i;          /* iterator */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                          | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);
    PRV_CPSS_DXCH_PHY_PORT_CHECK_MAC(devNum, portNum);
    CPSS_NULL_PTR_CHECK_MAC(apEnablePtr);
    CPSS_NULL_PTR_CHECK_MAC(apParamsPtr);

    portGroup = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portNum);
    phyPortNum = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApEngineInitGet(devNum[%d], portGroup[%d], *apEnablePtr)", devNum, portGroup);
    (GT_VOID)mvHwsApEngineInitGet(devNum,portGroup,apEnablePtr);
    if(GT_FALSE == *apEnablePtr)
    {
        return GT_OK;
    }
	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApPortConfigGet(devNum[%d], portGroup[%d], phyPortNum[%d] *apEnablePtr, *apCfg)", devNum, portGroup, phyPortNum);
    rc = mvHwsApPortConfigGet(devNum,portGroup,phyPortNum,apEnablePtr,&apCfg);
    AP_DBG_PRINT_MAC(("mvHwsApPortConfigGet:rc=%d,devNum=%d,portGroup=%d,phyPortNum=%d,\
apLaneNum=%d,modesVector=0x%x,fcAsmDir=%d,fcPause=%d,fecReq=%d,fecSup=%d,nonceDis=%d,\
refClockFreq=%d,refClockSource=%d,apEnable=%d\n",
                      rc, devNum, portGroup, phyPortNum, apCfg.apLaneNum,
                      apCfg.modesVector,apCfg.fcAsmDir,apCfg.fcPause,apCfg.fecReq,
                      apCfg.fecSup, apCfg.nonceDis, apCfg.refClockCfg.refClockFreq,
                      apCfg.refClockCfg.refClockSource, *apEnablePtr));
    if(rc != GT_OK)
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
        return rc;
    }

    apParamsPtr->fcAsmDir = apCfg.fcAsmDir;
    apParamsPtr->fcPause = apCfg.fcPause;
    apParamsPtr->fecRequired = apCfg.fecReq;
    apParamsPtr->fecSupported = apCfg.fecSup;
    apParamsPtr->laneNum = apCfg.apLaneNum;
    apParamsPtr->noneceDisable = apCfg.nonceDis;

    i = 0;
    if(apCfg.modesVector & _10GBase_KX4_Bit1)
    {
        apParamsPtr->modesAdvertiseArr[i].ifMode =
            CPSS_PORT_INTERFACE_MODE_XGMII_E;
        apParamsPtr->modesAdvertiseArr[i++].speed = CPSS_PORT_SPEED_10000_E;
    }

    if(apCfg.modesVector & _1000Base_KX_Bit0)
    {
        apParamsPtr->modesAdvertiseArr[i].ifMode =
            CPSS_PORT_INTERFACE_MODE_1000BASE_X_E;
        apParamsPtr->modesAdvertiseArr[i++].speed = CPSS_PORT_SPEED_1000_E;
    }

    if(apCfg.modesVector & _10GBase_KR_Bit2)
    {
        apParamsPtr->modesAdvertiseArr[i].ifMode =
            CPSS_PORT_INTERFACE_MODE_KR_E;
        apParamsPtr->modesAdvertiseArr[i++].speed = CPSS_PORT_SPEED_10000_E;
    }

    if(apCfg.modesVector & _40GBase_KR4_Bit3)
    {
        apParamsPtr->modesAdvertiseArr[i].ifMode =
            CPSS_PORT_INTERFACE_MODE_KR_E;
        apParamsPtr->modesAdvertiseArr[i++].speed = CPSS_PORT_SPEED_40000_E;
    }

    for(;i<CPSS_DXCH_PORT_AP_IF_ARRAY_SIZE_CNS;i++)
    {
        apParamsPtr->modesAdvertiseArr[i].ifMode =
            CPSS_PORT_INTERFACE_MODE_REDUCED_10BIT_E;
        apParamsPtr->modesAdvertiseArr[i].speed = CPSS_PORT_SPEED_NA_E;
    }

    return GT_OK;
}

/*******************************************************************************
* cpssDxChPortApPortConfigGet
*
* DESCRIPTION:
*       Get AP configuration of port.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum     - physical device number
*       portNum    - physical port number
*
* OUTPUTS:
*       apEnablePtr - AP enable/disable on port
*       apParamsPtr - (ptr to) AP parameters of port
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_BAD_PTR               - apEnablePtr or apParamsPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS cpssDxChPortApPortConfigGet
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    OUT GT_BOOL                         *apEnablePtr,
    OUT CPSS_DXCH_PORT_AP_PARAMS_STC    *apParamsPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApPortConfigGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, apEnablePtr, apParamsPtr));

    rc = internal_cpssDxChPortApPortConfigGet(devNum, portNum, apEnablePtr, apParamsPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, apEnablePtr, apParamsPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChPortApPortStatusGet
*
* DESCRIPTION:
*       Get status of AP on port.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - physical port number
*       apStatusPtr - (ptr to) AP parameters for port
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_BAD_PTR               - apStatusPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_INITIALIZED       - AP engine not run
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApPortStatusGet
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    OUT CPSS_DXCH_PORT_AP_STATUS_STC   *apStatusPtr
)
{
    GT_STATUS   rc; /* return status */
    GT_U32                   portGroup;
    GT_U32                   apPortNum;
    MV_HWS_AP_PORT_STATUS    apResult;

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                          | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);
    PRV_CPSS_DXCH_PHY_PORT_CHECK_MAC(devNum, portNum);
    CPSS_NULL_PTR_CHECK_MAC(apStatusPtr);

    portGroup = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portNum);
    apPortNum = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApPortStatusGet(devNum[%d], portGroup[%d], apPortNum[%d], *apResult)", devNum, portGroup, apPortNum);
    rc = mvHwsApPortStatusGet(devNum, portGroup, apPortNum, &apResult);
    if(rc != GT_OK)
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
        return rc;
    }

    AP_DBG_PRINT_MAC(("cpssDxChPortApPortStatusGet:mvHwsApPortStatusGet:portGroup=%d,apPortNum=%d,\
apResult:apLaneNum=%d,hcdFound=%d,hcdLinkStatus=%d,\
hcdFecEn=%d,hcdFcPause=%d,hcdFcAsmDir=%d,postApPortMode=%d,\
postApPortNum=%d,preApPortNum=%d\n",
                      portGroup, apPortNum,
                      apResult.apLaneNum,
                      apResult.hcdResult.hcdFound,
                      apResult.hcdResult.hcdLinkStatus,
                      apResult.hcdResult.hcdFecEn,
                      apResult.hcdResult.hcdFcRxPauseEn,
                      apResult.hcdResult.hcdFcTxPauseEn,
                      apResult.postApPortMode,
                      apResult.postApPortNum,
                      apResult.preApPortNum));

    cpssOsMemSet(apStatusPtr, 0, sizeof(CPSS_DXCH_PORT_AP_STATUS_STC));
    if((apStatusPtr->hcdFound = apResult.hcdResult.hcdFound) != GT_TRUE)
    {/* nothing interesting any more */
        return GT_OK;
    }

    apStatusPtr->fecEnabled = apResult.hcdResult.hcdFecEn;

    switch(apResult.postApPortMode)
    {
        case _1000Base_X:
            apStatusPtr->portMode.ifMode = CPSS_PORT_INTERFACE_MODE_1000BASE_X_E;
            apStatusPtr->portMode.speed = CPSS_PORT_SPEED_1000_E;
            break;
        case _10GBase_KX4:
            apStatusPtr->portMode.ifMode = CPSS_PORT_INTERFACE_MODE_XGMII_E;
            apStatusPtr->portMode.speed = CPSS_PORT_SPEED_10000_E;
            break;
        case _10GBase_KR:
            apStatusPtr->portMode.ifMode = CPSS_PORT_INTERFACE_MODE_KR_E;
            apStatusPtr->portMode.speed = CPSS_PORT_SPEED_10000_E;
            break;
        case _40GBase_KR:
            apStatusPtr->portMode.ifMode = CPSS_PORT_INTERFACE_MODE_KR_E;
            apStatusPtr->portMode.speed = CPSS_PORT_SPEED_40000_E;
            break;
        case _10GBase_SR_LR:
            apStatusPtr->portMode.ifMode = CPSS_PORT_INTERFACE_MODE_SR_LR_E;
            apStatusPtr->portMode.speed = CPSS_PORT_SPEED_10000_E;
            break;
        case _40GBase_SR_LR:
            apStatusPtr->portMode.ifMode = CPSS_PORT_INTERFACE_MODE_SR_LR_E;
            apStatusPtr->portMode.speed = CPSS_PORT_SPEED_40000_E;
            break;
        default:
            return GT_NOT_SUPPORTED;
    }

    apStatusPtr->postApPortNum  = apResult.postApPortNum;
    apStatusPtr->fcTxPauseEn    = apResult.hcdResult.hcdFcTxPauseEn;
    apStatusPtr->fcRxPauseEn        = apResult.hcdResult.hcdFcRxPauseEn;

    AP_DBG_PRINT_MAC(("apStatusPtr:fcTxPauseEn=%d,fcRxPauseEn=%d,fecEnabled=%d,hcdFound=%d,\
ifMode=%d,speed=%d,postApPortNum=%d\n",
                      apStatusPtr->fcTxPauseEn,apStatusPtr->fcRxPauseEn,apStatusPtr->
                      fecEnabled,apStatusPtr->hcdFound,
                      apStatusPtr->portMode.ifMode,apStatusPtr->portMode.speed,
                      apStatusPtr->postApPortNum));

    return GT_OK;
}

/*******************************************************************************
* cpssDxChPortApPortStatusGet
*
* DESCRIPTION:
*       Get status of AP on port.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - physical port number
*       apStatusPtr - (ptr to) AP parameters for port
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_BAD_PTR               - apStatusPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_INITIALIZED       - AP engine not run
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS cpssDxChPortApPortStatusGet
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    OUT CPSS_DXCH_PORT_AP_STATUS_STC   *apStatusPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApPortStatusGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, apStatusPtr));

    rc = internal_cpssDxChPortApPortStatusGet(devNum, portNum, apStatusPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, apStatusPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChPortApResolvedPortsBmpGet
*
* DESCRIPTION:
*       Get bitmap of ports on port group (local core) where AP process finished
*       with agreed for both sides resolution
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portGroupNum - number of port group (local core)
*
* OUTPUTS:
*       apResolvedPortsBmpPtr  - 1's set for ports of local core where AP
*                                   resolution acheaved
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port group or device
*       GT_BAD_PTR               - apResolvedPortsBmpPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApResolvedPortsBmpGet
(
    IN  GT_U8   devNum,
    IN  GT_U32  portGroupNum,
    OUT GT_U32  *apResolvedPortsBmpPtr
)
{
    GT_STATUS   rc;         /* return code */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                          | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);
    CPSS_NULL_PTR_CHECK_MAC(apResolvedPortsBmpPtr);

	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApPortResolutionMaskGet(devNum[%d], portGroupNum[%d], *apResolvedPortsBmpPtr)", devNum, portGroupNum);
    rc = mvHwsApPortResolutionMaskGet(devNum,portGroupNum,apResolvedPortsBmpPtr);
    if(rc != GT_OK)
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
        AP_DBG_PRINT_MAC(("mvHwsApPortResolutionMaskGet:portGroupNum=%d,rc=%d\n",
                          portGroupNum, rc));
        return rc;
    }

    AP_DBG_PRINT_MAC(("cpssDxChPortApResolvedPortsBmpGet:devNum=%d,portGroupNum=%d,apResolvedPortsBmp=0x%x\n",
                      devNum, portGroupNum, *apResolvedPortsBmpPtr));

    return GT_OK;
}

/*******************************************************************************
* cpssDxChPortApResolvedPortsBmpGet
*
* DESCRIPTION:
*       Get bitmap of ports on port group (local core) where AP process finished
*       with agreed for both sides resolution
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portGroupNum - number of port group (local core)
*
* OUTPUTS:
*       apResolvedPortsBmpPtr  - 1's set for ports of local core where AP
*                                   resolution acheaved
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port group or device
*       GT_BAD_PTR               - apResolvedPortsBmpPtr is NULL
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS cpssDxChPortApResolvedPortsBmpGet
(
    IN  GT_U8   devNum,
    IN  GT_U32  portGroupNum,
    OUT GT_U32  *apResolvedPortsBmpPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApResolvedPortsBmpGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portGroupNum, apResolvedPortsBmpPtr));

    rc = internal_cpssDxChPortApResolvedPortsBmpGet(devNum, portGroupNum, apResolvedPortsBmpPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portGroupNum, apResolvedPortsBmpPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChPortApSetActiveMode
*
* DESCRIPTION:
*       Update port's AP active lanes according to new interface.
*
* APPLICABLE DEVICES:
*        Lion2; Lion3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; xCat2.
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - number of physical port
*       ifMode    - interface mode
*       speed     - port data speed
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApSetActiveMode
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    IN  CPSS_PORT_INTERFACE_MODE_ENT    ifMode,
    IN  CPSS_PORT_SPEED_ENT             speed
)
{
    GT_STATUS               rc;        /* return code */
    GT_U32                  portGroup; /* local core number */
    GT_U32                  phyPortNum;/* port number in local core */
    MV_HWS_PORT_STANDARD    portMode;  /* port i/f mode and speed translated to
                                            BlackBox enum */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                          | CPSS_LION_E | CPSS_XCAT2_E);
    PRV_CPSS_DXCH_PHY_PORT_CHECK_MAC(devNum, portNum);

    AP_DBG_PRINT_MAC(("cpssDxChPortApSetActiveMode:devNum=%d,portNum=%d,ifMode=%d,speed=%d\n", 
                        devNum, portNum, ifMode, speed));

    rc = prvCpssLion2CpssIfModeToHwsTranslate(ifMode, speed, &portMode);
    if(rc != GT_OK)
    {
        AP_DBG_PRINT_MAC(("cpssDxChPortApSetActiveMode(%d,%d,%d,%d):prvCpssLion2CpssIfModeToHwsTranslate:rc=%d\n",
                          devNum, portNum, ifMode, speed, rc));
        return rc;
    }

    portGroup = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portNum);
    phyPortNum = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

	CPSS_LOG_INFORMATION_MAC("Calling: mvHwsApPortSetActiveLanes(devNum[%d], portGroup[%d], phyPortNum[%d], portMode[%d])", devNum, portGroup, phyPortNum, portMode);
    rc = mvHwsApPortSetActiveLanes(devNum, portGroup, phyPortNum, portMode);
	if(rc != GT_OK)
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
	}
    AP_DBG_PRINT_MAC(("mvHwsApPortSetActiveLanes(%d,%d,%d,%d):rc=%d\n",
                      devNum, portGroup, phyPortNum, portMode, rc));

    return rc;
}

/*******************************************************************************
* cpssDxChPortApSetActiveMode
*
* DESCRIPTION:
*       Update port's AP active lanes according to new interface.
*
* APPLICABLE DEVICES:
*        Lion2; Lion3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; xCat2.
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - number of physical port
*       ifMode    - interface mode
*       speed     - port data speed
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS cpssDxChPortApSetActiveMode
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    IN  CPSS_PORT_INTERFACE_MODE_ENT    ifMode,
    IN  CPSS_PORT_SPEED_ENT             speed
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApSetActiveMode);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, ifMode, speed));

    rc = internal_cpssDxChPortApSetActiveMode(devNum, portNum, ifMode, speed);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, ifMode, speed));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}
/*******************************************************************************
* internal_cpssDxChPortApLock
*
* DESCRIPTION:
*       Acquires lock so host and AP machine won't access the same 
*		resource at the same time.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - physical port number
*
* OUTPUTS:
*       statePtr    - (ptr to) port state:
*                           GT_TRUE - locked by HOST - can be configured
*                           GT_FALSE - locked by AP processor - access forbidden
*
* RETURNS:
*       GT_OK                - on success - port not in use by AP processor
*       GT_BAD_PARAM         - on wrong port number or device
*       GT_NOT_INITIALIZED   - AP engine or library not initialized
*       GT_BAD_PTR           - statePtr is NULL
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApLock
(
    IN  GT_U8                   devNum,
    IN  GT_PHYSICAL_PORT_NUM    portNum,
    OUT GT_BOOL                 *statePtr
)
{
    GT_STATUS   rc;         /* return code */
	GT_U32	    portGroup;  /* local core number */
	GT_U32	    phyPortNum; /* number of port in local core */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                          | CPSS_LION_E | CPSS_XCAT2_E);
    PRV_CPSS_DXCH_PHY_PORT_CHECK_MAC(devNum, portNum);
    CPSS_NULL_PTR_CHECK_MAC(statePtr);

    if(apSemPrintEn)
    {
        cpssOsPrintf("cpssDxChPortApLock:devNum=%d,portNum=%d,", 
                            devNum, portNum);
    }

    portGroup = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portNum);
    phyPortNum = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

	CPSS_LOG_INFORMATION_MAC("Calling: mvApLockGet(devNum[%d], portGroup[%d], phyPortNum[%d])", devNum, portGroup, phyPortNum);
    rc = mvApLockGet(devNum, portGroup, phyPortNum);
    *statePtr = (GT_OK == rc) ? GT_TRUE : GT_FALSE;
    if(apSemPrintEn)
    {
        cpssOsPrintf("state=%d\n", *statePtr);
    }
    if((GT_OK == rc) || (GT_NO_RESOURCE == rc))
    {
        return GT_OK;
    }
    else
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
        return rc;
    }
}

/*******************************************************************************
* cpssDxChPortApLock
*
* DESCRIPTION:
*       Acquires lock so host and AP machine won't access the same 
*		resource at the same time.
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - physical port number
*
* OUTPUTS:
*       statePtr    - (ptr to) port state:
*                           GT_TRUE - locked by HOST - can be configured
*                           GT_FALSE - locked by AP processor - access forbidden
*
* RETURNS:
*       GT_OK                - on success - port not in use by AP processor
*       GT_BAD_PARAM         - on wrong port number or device
*       GT_NOT_INITIALIZED   - AP engine or library not initialized
*       GT_BAD_PTR           - statePtr is NULL
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS cpssDxChPortApLock
(
    IN  GT_U8                   devNum,
    IN  GT_PHYSICAL_PORT_NUM    portNum,
    OUT GT_BOOL                 *statePtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApLock);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, statePtr));

    rc = internal_cpssDxChPortApLock(devNum, portNum, statePtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, statePtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}


/*******************************************************************************
* internal_cpssDxChPortApUnLock
*
* DESCRIPTION:
*       Releases the synchronization lock (between Host and AP machine).
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - physical port number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                - on success - port not in use by AP processor
*       GT_BAD_PARAM         - on wrong port number or device
*       GT_NOT_INITIALIZED   - AP engine or library not initialized
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChPortApUnLock
(
    IN  GT_U8                   devNum,
    IN  GT_PHYSICAL_PORT_NUM    portNum
)
{
	GT_STATUS	rc;
	GT_U32	    portGroup;  /* local core number */
	GT_U32	    phyPortNum; /* number of port in local core */

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                          | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                          | CPSS_LION_E | CPSS_XCAT2_E);
    PRV_CPSS_DXCH_PHY_PORT_CHECK_MAC(devNum, portNum);

    portGroup = PRV_CPSS_GLOBAL_PORT_TO_PORT_GROUP_ID_CONVERT_MAC(devNum, portNum);
    phyPortNum = PRV_CPSS_GLOBAL_PORT_TO_LOCAL_PORT_CONVERT_MAC(devNum, portNum);

    if(apSemPrintEn)
    {
        cpssOsPrintf("cpssDxChPortApUnLock:devNum=%d,portNum=%d\n", 
                            devNum, portNum);
    }
	CPSS_LOG_INFORMATION_MAC("Calling: mvApLockRelease(devNum[%d], portGroup[%d], phyPortNum[%d])", devNum, portGroup, phyPortNum);
    rc = mvApLockRelease(devNum, portGroup, phyPortNum);
	if(rc != GT_OK)
    {
		CPSS_LOG_INFORMATION_MAC("Hws return code is %d", rc);
    }
	return rc;
}

/*******************************************************************************
* cpssDxChPortApUnLock
*
* DESCRIPTION:
*       Releases the synchronization lock (between Host and AP machine).
*
* APPLICABLE DEVICES:
*        Lion2; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
*
* INPUTS:
*       devNum    - physical device number
*       portNum   - physical port number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                - on success - port not in use by AP processor
*       GT_BAD_PARAM         - on wrong port number or device
*       GT_NOT_INITIALIZED   - AP engine or library not initialized
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS cpssDxChPortApUnLock
(
    IN  GT_U8                   devNum,
    IN  GT_PHYSICAL_PORT_NUM    portNum
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChPortApUnLock);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum));

    rc = internal_cpssDxChPortApUnLock(devNum, portNum);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}


