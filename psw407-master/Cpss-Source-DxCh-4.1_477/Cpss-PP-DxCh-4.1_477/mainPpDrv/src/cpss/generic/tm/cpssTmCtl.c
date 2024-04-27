/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* cpssTmCtl.c
*
* DESCRIPTION:
*       TM Configuration Library control interface APIs
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#define CPSS_LOG_IN_MODULE_ENABLE
#include <cpss/generic/tm/private/prvCpssGenTmLog.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/extServices/os/gtOs/gtGenTypes.h>
#include <cpss/generic/tm/cpssTmServices.h>
#include <cpss/generic/tm/cpssTmCtl.h>
#include <cpss/generic/config/private/prvCpssConfigTypes.h>
#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/tm/cpssTmPublicDefs.h>
#include <tm_ctl.h>
#include <tm_nodes_read.h>
#include <platform/tm_regs.h>

#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGlue.h>
#include <platform/cpss_tm_rw_registers_proc.h>

#include <cpssDriver/pp/hardware/prvCpssDrvHwCntl.h>
#include <cpssDriver/pp/hardware/cpssDriverPpHw.h>
#include <cpss/generic/cpssHwInit/private/prvCpssHwRegisters.h>

#include <cpss/generic/tm/cpssTmCtl.h>
#include <cpss/generic/tm/prvCpssTmCtl.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>

/* Global LADs parameters Settings*/
static CPSS_TM_CTL_LAD_INF_PARAM_STC tmLadParamsArr[CPSS_TM_CTL_MAX_NUM_OF_LADS_CNS+1] =
{
/*   MinPkgSize      NumOfPagesPerBank      NumOfPkgsPerBank      portChunksEmitPerSel */
    {   0,                  0,                      0,                      0 },
    {   0x10,               0xE38E,                 0xE38E,                 2 },
    {   0x20,               0x7878,                 0x7878,                 4 },
    {   0x30,               0x28EC,                 0x2AAA,                 6 },
    {   0x80,               0x2D000,                0x2D000,                8 },
    {   0x80,               0x31F3,                 0x31F3,                 10}
};

/* Min num Of Lads By CPSS Tm Device */
GT_U8 prvCpssMinDramInfArr[PRV_CPSS_TM_DEV_LAST_E+1] = 
{
    2, /* PRV_CPSS_TM_DEV_BC2_E */
    1, /* PRV_CPSS_TM_DEV_BOBK_CETUS_E  */
    1, /* PRV_CPSS_TM_DEV_BOBK_CAELUM_E */
    0
};

/* Max num Of Lads By CPSS Tm Device */
GT_U8 prvCpssMaxDramInfArr[PRV_CPSS_TM_DEV_LAST_E+1] = 
{
    5, /* PRV_CPSS_TM_DEV_BC2_E */
    1, /* PRV_CPSS_TM_DEV_BOBK_CETUS_E  */
    3, /* PRV_CPSS_TM_DEV_BOBK_CAELUM_E */
    0
};


/* Bobcat2 tree definitions */
/* Max number of nodes supported by HW */
/** Max number of queues supported by HW */
#define TM_MAX_QUEUES   16384
/** Max number of A-nodes supported by HW */
#define TM_MAX_A_NODES  4096
/** Max number of B-nodes supported by HW */
#define TM_MAX_B_NODES  2048
/** Max number of C-nodes supported by HW */
#define TM_MAX_C_NODES  512
/** Max number of Ports supported by HW */
#define TM_MAX_PORTS    192


/* TM HW Params for BC2 */
static PRV_CPSS_TM_HW_PARAMS_STC tmHwParamsBc2 = 
{
    TM_MAX_QUEUES,  /* maxQueues */ 
    TM_MAX_A_NODES, /* maxAnodes */
    TM_MAX_B_NODES, /* maxBnodes */
    TM_MAX_C_NODES, /* maxCnodes */
    TM_MAX_PORTS,   /* maxPorts  */
    4,              /* queuesToAnode  */   
    2,              /* aNodesToBnode  */   
    4,              /* bNodesToCnode  */   
    2,              /* cNodesToPort   */   
    4               /* installedQueuesPerPort */
};

/* TM HW Params for Bobk */
static PRV_CPSS_TM_HW_PARAMS_STC tmHwParamsBobk = 
{
    4096,           /* maxQueues */ 
    2048,           /* maxAnodes */
    1024,           /* maxBnodes */
    TM_MAX_C_NODES, /* maxCnodes */
    TM_MAX_PORTS,   /* maxPorts  */
    2,              /* queuesToAnode  */   
    2,              /* aNodesToBnode  */   
    2,              /* bNodesToCnode  */   
    2,              /* cNodesToPort   */   
    4               /* installedQueuesPerPort */
};

/* Global Parameter- count tmlib init executions */
GT_U8 tmCurrentNum = 0;

#ifdef ASIC_SIMULATION
    #define WAIT_FOR_DEVICE_MAC(mili)
#else /*ASIC_SIMULATION*/
    #define WAIT_FOR_DEVICE_MAC(mili)   \
        cpssOsTimerWkAfter(mili)
#endif /*ASIC_SIMULATION*/


/*******************************************************************************
* prvCpssTmCtlGetTmHwParams
*
* DESCRIPTION:
*       Get TM HW params per device family type.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       tmHwParamsPtr     - pointer to PRV_CPSS_TM_HW_PARAMS_STC.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_BAD_PARAM                - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_STATUS prvCpssTmCtlGetTmHwParams
(
    IN GT_U8 devNum,
    OUT PRV_CPSS_TM_HW_PARAMS_STC **tmHwParamsPtr
)
{
    GT_STATUS   rc = GT_OK;

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );
    CPSS_NULL_PTR_CHECK_MAC(tmHwParamsPtr);

    if ((PRV_CPSS_PP_MAC(devNum)->devFamily ==  CPSS_PP_FAMILY_DXCH_BOBCAT2_E) &&
        (PRV_CPSS_PP_MAC(devNum)->devSubFamily == CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E))
    {
        *tmHwParamsPtr = &tmHwParamsBobk;
    }
    else
    {
        *tmHwParamsPtr = &tmHwParamsBc2;
    }

    return rc;
}


/*******************************************************************************
* internal_cpssTmCtlWriteRegister
*
* DESCRIPTION:
*       Write register USED before libInit for cpssTmCtlHwInit when no tm_hndl exist.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum          - Device number.
*       regAddr         - Register address.
*       dataPtr         - Pointer to write data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_FAIL                  - on hardware error.
*       GT_BAD_PARAM             - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*******************************************************************************/
static GT_STATUS internal_cpssTmCtlWriteRegister
(
    IN GT_U8                   devNum,
    IN GT_U64                  regAddr,
    IN GT_U64                  *dataPtr
)
{
    GT_STATUS   rc;
    GT_U32  data;
        GT_U8        burstMode;

        PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );
   /*check whether we are in burst or non burst mode */
    rc = cpssDrvHwPpResetAndInitControllerReadReg(devNum, 0x000F828C ,&data);
        if (GT_OK != rc)   return rc;

    burstMode =  ((data & 0x10)==0);
    /* temporary   : burst mode is suppressed */
    burstMode = 0;

    return  tm_write_register_proc(devNum, burstMode,  regAddr,  dataPtr);

}

/*******************************************************************************
* cpssTmCtlWriteRegister
*
* DESCRIPTION:
*       Write register USED before libInit for cpssTmCtlHwInit when no tm_hndl exist.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum          - Device number.
*       regAddr         - Register address.
*       dataPtr         - Pointer to write data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_FAIL                  - on hardware error.
*       GT_BAD_PARAM             - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*******************************************************************************/
GT_STATUS cpssTmCtlWriteRegister
(
    IN GT_U8                   devNum,
    IN GT_U64                  regAddr,
    IN GT_U64                  *dataPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssTmCtlWriteRegister);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, regAddr, dataPtr));

    rc = internal_cpssTmCtlWriteRegister(devNum, regAddr, dataPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, regAddr, dataPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssTmCtlReadRegister
*
* DESCRIPTION:
*       Read register USED before libInit for cpssTmCtlHwInit when no tm_hndl exist.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum          - Device number.
*       regAddr         - Register address.
*       dataPtr         - Pointer to write data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_FAIL                  - on hardware error.
*       GT_BAD_PARAM             - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*******************************************************************************/
static GT_STATUS internal_cpssTmCtlReadRegister
(
    IN GT_U8                   devNum,
    IN GT_U64                  regAddr,
    IN GT_U64                  *dataPtr
)
{
    GT_STATUS   rc;
    GT_U32      data;
    GT_U8       burstMode;

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );
    /*check whether we are in burst or non burst mode */
    rc = cpssDrvHwPpResetAndInitControllerReadReg(devNum, 0x000F828C ,&data);
        if (GT_OK != rc)
                return rc;

    burstMode =  ((data & 0x10)==0);
/* temporary   : burst mode is suppressed */
burstMode = 0;

        return  tm_read_register_proc(devNum, burstMode,  regAddr,  dataPtr);
}

/*******************************************************************************
* cpssTmCtlReadRegister
*
* DESCRIPTION:
*       Read register USED before libInit for cpssTmCtlHwInit when no tm_hndl exist.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum          - Device number.
*       regAddr         - Register address.
*       dataPtr         - Pointer to write data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_FAIL                  - on hardware error.
*       GT_BAD_PARAM             - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*******************************************************************************/
GT_STATUS cpssTmCtlReadRegister
(
    IN GT_U8                   devNum,
    IN GT_U64                  regAddr,
    IN GT_U64                  *dataPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssTmCtlReadRegister);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, regAddr, dataPtr));

    rc = internal_cpssTmCtlReadRegister(devNum, regAddr, dataPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, regAddr, dataPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssTmInit
*
* DESCRIPTION:
*       Initialize the TM configuration library.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*       tmLibInitParamsPtr- (pointer of) CPSS_TM_LIB_INIT_PARAMS_STC.
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_STATUS internal_cpssTmInit
(
    IN GT_U8 devNum,
    IN CPSS_TM_LIB_INIT_PARAMS_STC *tmLibInitParamsPtr
)
{
    int         ret = 0;
    GT_STATUS   rc = GT_OK;

    struct tm_lib_init_params   tm_lib_init_params = {0};
	struct tm_tree_structure    tm_tree_params;
    PRV_CPSS_TM_HW_PARAMS_STC   *tmHwParamsPtr;


    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );

    if (PRV_CPSS_PP_MAC(devNum)->tmInfo.tmHandle != NULL)
    {
        TM_DBG_INFO(("---- cpssTmInit:tm hndl already exist and tm_lib was invoked, ignoring ...\n"));
        return GT_OK;
    }
    CPSS_NULL_PTR_CHECK_MAC(tmLibInitParamsPtr);


    if(PRV_CPSS_DXCH_PP_MAC(devNum)->fineTuning.featureInfo.TmSupported == GT_FALSE)
    {
        TM_DBG_INFO(("TM is not supported\n"));
        return GT_NOT_SUPPORTED;
    }

    cpssOsMemSet(&PRV_CPSS_PP_MAC(devNum)->tmInfo, 0, sizeof(PRV_CPSS_GEN_TM_DB_STC));
    cpssTmToCpssErrCodesInit();
    init_tm_alias_struct();

    tm_lib_init_params.tm_mtu = tmLibInitParamsPtr->tmMtu;
    /* more init params here */

    rc = prvCpssTmCtlGetTmHwParams(devNum, &tmHwParamsPtr);
    if (rc != GT_OK)
    {
        TM_DBG_INFO(("prvCpssTmCtlGetTmHwParams has failed\n"));
        return rc;
    }
   
	tm_tree_params.numOfQueues = tmHwParamsPtr->maxQueues;
	tm_tree_params.numOfAnodes = tmHwParamsPtr->maxAnodes;
	tm_tree_params.numOfBnodes = tmHwParamsPtr->maxBnodes;
	tm_tree_params.numOfCnodes = tmHwParamsPtr->maxCnodes;
	tm_tree_params.numOfPorts  = tmHwParamsPtr->maxPorts;

	tm_tree_params.queuesToAnode = tmHwParamsPtr->queuesToAnode;
	tm_tree_params.aNodesToBnode = tmHwParamsPtr->aNodesToBnode;
	tm_tree_params.bNodesToCnode = tmHwParamsPtr->bNodesToCnode;
	tm_tree_params.cNodesToPort  = tmHwParamsPtr->cNodesToPort;
	tm_tree_params.installedQueuesPerPort = tmHwParamsPtr->installedQueuesPerPort;

    /* TM Initialization */
    ret = tm_lib_open_ext(devNum, &tm_tree_params,  &tm_lib_init_params, &PRV_CPSS_PP_MAC(devNum)->tmInfo.tmHandle);
    PRV_CPSS_PP_MAC(devNum)->tmInfo.isInitilized = GT_TRUE;

    /* convert from BC2 devFamily/SubFamily to internal TM Devs Ids */
    PRV_CPSS_PP_MAC(devNum)->tmInfo.prvCpssTmDevId = prvCpssTmGetInternalDevId(devNum);

    if (PRV_CPSS_PP_MAC(devNum)->tmInfo.prvCpssTmDevId == PRV_CPSS_TM_DEV_LAST_E)
        rc = GT_NOT_SUPPORTED;   
    else
        rc = XEL_TO_CPSS_ERR_CODE(ret);

    /* Increase number of ready TM devices */
    if (rc == GT_OK)
    {
        tmCurrentNum++;
        rc = cpssDxChTmGlueInit(devNum);
    }
    else
    {
        PRV_CPSS_PP_MAC(devNum)->tmInfo.isInitilized = GT_FALSE;
    }

    return rc;
}

/*******************************************************************************
* cpssTmInit
*
* DESCRIPTION:
*       Initialize the TM configuration library.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
GT_STATUS cpssTmInit
(
    IN GT_U8 devNum
)
{
    GT_STATUS rc;
    CPSS_TM_LIB_INIT_PARAMS_STC tmLibInitParams;

    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssTmInit);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum));

    cpssOsMemSet(&tmLibInitParams, 0, sizeof(tmLibInitParams));
    rc = internal_cpssTmInit(devNum, &tmLibInitParams);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum));
    CPSS_API_UNLOCK_MAC(0,0);
    return rc;
}
/*******************************************************************************
* cpssTmInitExt
*
* DESCRIPTION:
*       Customize Initialize of the TM configuration library.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*       tmLibInitParamsPtr- (pointer of) TM LIB initialize parameters.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
GT_STATUS cpssTmInitExt
(
    IN GT_U8 devNum,
    IN CPSS_TM_LIB_INIT_PARAMS_STC *tmLibInitParamsPtr
)
{
        GT_STATUS rc;
        CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssTmInitExt);

        CPSS_API_LOCK_MAC(0,0);
        CPSS_LOG_API_ENTER_MAC((funcId, devNum, tmLibInitParamsPtr));

        rc = internal_cpssTmInit(devNum, tmLibInitParamsPtr);

        CPSS_LOG_API_EXIT_MAC(funcId, rc);
        CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum));
        CPSS_API_UNLOCK_MAC(0,0);
    return rc;
}

/*******************************************************************************
* internal_cpssTmClose
*
* DESCRIPTION:
*       Close the TM configuration library.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_STATUS internal_cpssTmClose
(
    IN GT_U8 devNum
)
{
    int         ret = 0;
    GT_STATUS   rc = GT_OK;

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );

    if (PRV_CPSS_PP_MAC(devNum)->tmInfo.tmHandle == NULL)
    {
        TM_DBG_INFO(("---- cpssTmCtlLibClose: tm_hndl is NULL ... "));
        return GT_FAIL;
    }

    /** Close TM configuration library and free parameters */
    ret = tm_lib_close(PRV_CPSS_PP_MAC(devNum)->tmInfo.tmHandle);
    rc = XEL_TO_CPSS_ERR_CODE(ret);
    if (rc != GT_OK)
        return rc;

    tmCurrentNum--;

    cpssOsMemSet(&PRV_CPSS_PP_MAC(devNum)->tmInfo, 0, sizeof(PRV_CPSS_GEN_TM_DB_STC));

    return rc;
}

/*******************************************************************************
* cpssTmClose
*
* DESCRIPTION:
*       Close the TM configuration library.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
GT_STATUS cpssTmClose
(
    IN GT_U8 devNum
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssTmClose);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum));

    rc = internal_cpssTmClose(devNum);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}










/*******************************************************************************
* internal_cpssTmTreeParamsGet
*
* DESCRIPTION:
*       Get TM Tree Hw properties.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       tmTreeParamsPtr  - pointer to CPSS_TM_TREE_PARAMS_STC.    
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_BAD_PARAM                - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_STATUS internal_cpssTmTreeParamsGet
(
    IN GT_U8 devNum,
    OUT CPSS_TM_TREE_PARAMS_STC *tmTreeParamsPtr
)
{
    GT_STATUS   rc = GT_OK;
    PRV_CPSS_TM_HW_PARAMS_STC *tmHwParamsPtr;

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );

    CPSS_NULL_PTR_CHECK_MAC(tmTreeParamsPtr);

    rc = prvCpssTmCtlGetTmHwParams(devNum, &tmHwParamsPtr);
    if (rc != GT_OK)
        return rc;

    tmTreeParamsPtr->maxQueues = tmHwParamsPtr->maxQueues;  
    tmTreeParamsPtr->maxAnodes = tmHwParamsPtr->maxAnodes;  
    tmTreeParamsPtr->maxBnodes = tmHwParamsPtr->maxBnodes;  
    tmTreeParamsPtr->maxCnodes = tmHwParamsPtr->maxCnodes;  
    tmTreeParamsPtr->maxPorts  = tmHwParamsPtr->maxPorts;   

    return rc;
}

/*******************************************************************************
* cpssTmTreeParamsGet
*
* DESCRIPTION:
*       Get TM Tree Hw properties.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       tmTreeParamsPtr  - pointer to CPSS_TM_TREE_PARAMS_STC.    
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_BAD_PARAM                - on wrong device number or wrong parameter value.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*
* COMMENTS:
*       None.
*******************************************************************************/
GT_STATUS cpssTmTreeParamsGet
(
    IN GT_U8 devNum,
    OUT CPSS_TM_TREE_PARAMS_STC *tmTreeParamsPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssTmTreeParamsGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, tmTreeParamsPtr));

    rc = internal_cpssTmTreeParamsGet(devNum, tmTreeParamsPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, tmTreeParamsPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* prvCpssTmCtlGlobalParamsInit
*
* DESCRIPTION:
*       Initialize the TM global params HW configuration.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*       GT_BAD_STATE                - on burst mode or init doesnt succeed
*
* COMMENTS:
*       None.
*******************************************************************************/
GT_STATUS prvCpssTmCtlGlobalParamsInit
(
    IN GT_U8                   devNum
)
{
    GT_U64      writeData;
    GT_U64      regAddr;
    GT_U32      regData;
    GT_STATUS   rc = GT_OK;

    /*cpssOsPrintf("\n ***********GLOBAL*********** \n ");*/

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
        CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                                           | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );

    /* check if we are in non burst mode*/
    rc = cpssDrvHwPpResetAndInitControllerReadReg(devNum, 0x000F828C ,&regData);

#if 0
    #ifdef GM_USED
    regData = 0x10;
    #endif /*GT_USED*/

    if (!(regData & 0x10))
    {
        rc = GT_BAD_STATE; 
    }

    if (rc)
    {
        return rc;
    }
#endif

    /*Write_QMR_offset_Phase_0*/
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFF8,0xFFFFFFFF ,0x8000);
    if (rc)
    {
        return rc;
    }

    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFFC,0xFFFFFFFF ,0x0000123);
    if (rc)
    {
        return rc;
    }

    /*Write__to_OOR_Phase_4()*/
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001020,0xFFFFFFFF ,3);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001024,0xFFFFFFFF ,0x123);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001028,0xFFFFFFFF ,3);
    if (rc) 
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C00102C,0xFFFFFFFF ,0x00000123);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001010,0xFFFFFFFF ,3);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0, 0x0C001014,0xFFFFFFFF ,0x00000123);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001018,0xFFFFFFFF , 3);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C00101C,0xFFFFFFFF ,0x00000123);
    if (rc)
    {
        return rc;
    }


    /*Write__to_QMR_exception_mask_Phase_6*/
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C000020,0xFFFFFFFF ,0x2E4FF83F);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C000024,0xFFFFFFFF ,0xA0088931);
    if (rc)
    {
        return rc;
    }

    /*BAP_DRAM_Read_Control_FIFOs_update
    The BP thresholds currently are Set=8, Clr=7. The correct values are Set=6, Clr=5.
    bit 0-4 clr back pressure threshold, bits 32-36 set back pressure threshold*/
    /*write to all BAPs*/
    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x20;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x28;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x30;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x38;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x40;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x48;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);

    if (rc)
    {
        return rc;
    }
    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x50;

    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }
    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x58;
    rc = cpssTmCtlWriteRegister(devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x60;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    writeData.l[0]=0x00000005;
    writeData.l[1]=0x6;
    regAddr.l[0]= 0x00000318;
    regAddr.l[1]= 0x68;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    /*Write_QMR_offset_Phase_0*/
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFF8,0xFFFFFFFF ,0x8000);
    if (rc)
    {
        return rc;
    }
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFFC,0xFFFFFFFF ,0x0000123);
    if (rc)
    {
        return rc;
    }

    /*Internal_init___control_reg15_bit_0___0___Phase_7*/
    rc= cpssDrvHwPpResetAndInitControllerWriteReg (devNum,0x000F828C ,0x19F0);
    if (rc)
    {
        return rc;
    }

    /*Write amount of ports - 192*/
    writeData.l[0]=0xC0;
    writeData.l[1]=0;
    regAddr.l[0]= 0x00003000;
    regAddr.l[1]= 0x8;
    rc = cpssTmCtlWriteRegister( devNum, regAddr, &writeData);
    if (rc)
    {
        return rc;
    }

    return rc;
}

/*******************************************************************************
* prvCpssTmCtlLadInit
*
* DESCRIPTION:
*       configure the number of lads in TM HW.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*       numOfLad          - Nubmer of lads.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*       GT_BAD_STATE                - on burst mode or init doesnt succeed
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_STATUS prvCpssTmCtlLadInit
(
    IN GT_U8                   devNum,
    IN GT_U8                   numOfLad
)
{
    GT_U64 regData;
    GT_U64 writeReg;
    GT_U64      writeData;
    GT_U32      i;
    GT_STATUS   rc = GT_OK;

    /*cpssOsPrintf("\n ***********LAD*********** %d \n ",numOfLad);*/
    PRV_CPSS_TM_CTL_CHECK_DRAM_INF_NUM_MAC(devNum, numOfLad);

    writeData.l[1]=0;

    /*number of lads*/
    /*read*/
    rc = cpssTmCtlReadRegister(devNum, TM.QMgr.NumOfLADs, &regData);
    if (rc) {
        return rc;
    }

    writeData.l[0]= numOfLad;
    /*write*/
    rc = cpssTmCtlWriteRegister( devNum, TM.QMgr.NumOfLADs, &writeData);
    if (rc) {
        return rc;
    }

    /* set crc enable */
    writeData.l[0]= 1;
    rc = cpssTmCtlWriteRegister( devNum, TM.QMgr.CRC16En, &writeData);
    if (rc) {
        return rc;
    }

    /* write minPkgSize to HW */
    writeData.l[0] = tmLadParamsArr[numOfLad].minPkgSize;
    rc = cpssTmCtlWriteRegister(devNum, TM.QMgr.MinPkgSize, &writeData);
    if (rc) {
        return rc;
    }

    /* write numOfPagesPerBank to HW */
    writeData.l[0] = tmLadParamsArr[numOfLad].pagesPerBank;

    rc = cpssTmCtlWriteRegister( devNum,  TM.QMgr.NumOfPagesPerBank, &writeData);
    if (rc) {
        return rc;
    }

    /* write numOfPkgesPerBank to HW */
    writeData.l[0] = tmLadParamsArr[numOfLad].pkgesPerBank;
    rc = cpssTmCtlWriteRegister( devNum,  TM.QMgr.NumOfPkgsPerBank, &writeData);
    if (rc) {
        return rc;
    }

    /* write PortChunksEmitPerSel to HW */
    writeData.l[0] = tmLadParamsArr[numOfLad].portChunksEmitPerSel;
    for (i=0; i< 192; i++)
        {
        writeReg.l[0] = TM.QMgr.PortChunksEmitPerSel.l[0] + (i*8);
        writeReg.l[1] = TM.QMgr.PortChunksEmitPerSel.l[1];

        rc = cpssTmCtlWriteRegister( devNum,  writeReg, &writeData);
        if (rc)
                {
            return rc;
        }
    }

    return rc;
}


/*******************************************************************************
* prvCpssTmCtlDramFrequencyInit
*
* DESCRIPTION:
*       configure the frequency in BAP registers.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*       GT_BAD_STATE                - on burst mode or init doesnt succeed
*
* COMMENTS:
*       None.
*******************************************************************************/
        /* macro for writing data to register  - useful in following function*/
        #define WRITE_DATA_TO_REGISTER(register_address, mask, value)        \
                        /*read register content */\
/*                if (freqInitSkipParam==i++) continue; */\
                        rc = cpssTmCtlReadRegister( devNum,  register_address, &regData); \
                        if (rc) return rc; \
                        writeData.l[0]=(regData.l[0] & (~mask)) + (value & mask);\
                        /*write to register */\
                        rc = cpssTmCtlWriteRegister( devNum,  register_address, &writeData); \
                        if (rc) return rc;\
                        /* test for write  - uncomment if necessary \
                        rc = cpssTmCtlReadRegister( devNum,  register_address, &regData); \
                        if (rc) return rc; \
                        if ((regData.l[0]!= writeData.l[0]) || (regData.l[1]!= writeData.l[1] ) )\
                        {\
                                cpssOsPrintf("\n %s  %d :\n write data test failed for register 0x%08X  %08X\n", __FILE__, __LINE__ , register_address.l[1],register_address.l[0]);\
cpssOsPrintf("write data : 0x%08X  %08X \n", writeData.l[1], writeData.l[0]);\
cpssOsPrintf("read  data : 0x%08X  %08X \n", regData.l[1], regData.l[0]);\
                               return GT_BAD_PARAM;\
                        }\
                        */
        /* macro for read register content         - for debug purposes */
        #define READ_REGISTER_CONTENT(register_address, mask, value)        \
                        rc = cpssTmCtlReadRegister( devNum,  register_address, &regData); \
                        if (rc)  \
                        {\
                                cpssOsPrintf("\n %s  %d :  read failed for register %x\n ", __FILE__, __LINE__ , register_address);\
                                return GT_BAD_PARAM;\
                        }\
                        cpssOsPrintf("\n read register : "#value" =%d (0x%x)\n ", regData.l[0], regData.l[0]);\


#define PRV_CPSS_DFX_SERVER_SAR2_REG_ADDR_CNS       0x000f8204
#define PRV_CPSS_DFX_SERVER_SAR2_REG_PLL2_TM_OFFS_CNS  15
#define PRV_CPSS_DFX_SERVER_SAR2_REG_PLL2_TM_LEN_CNS   3

GT_STATUS prvCpssDxChTMFreqGet
(
    IN  GT_U8    devNum,
    OUT GT_U32 * tmFreqPtr
)
{
    GT_U32 tmFreq;
    GT_STATUS rc;
    GT_U32    sar2_reg_data;
    GT_U32    tmMask;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    rc = cpssDrvHwPpResetAndInitControllerReadReg(devNum, PRV_CPSS_DFX_SERVER_SAR2_REG_ADDR_CNS, &sar2_reg_data);
    if (GT_OK != rc)
    {
        return GT_BAD_PARAM;
    }


    tmMask = (1<<PRV_CPSS_DFX_SERVER_SAR2_REG_PLL2_TM_LEN_CNS)-1;
    tmFreq = (sar2_reg_data >>= PRV_CPSS_DFX_SERVER_SAR2_REG_PLL2_TM_OFFS_CNS) & tmMask;        /* shift right 15 bits         */

    #ifdef ASIC_SIMULATION
            tmFreq = 2;/*933_MHZ*/
    #endif /*ASIC_SIMULATION*/

    if (tmFreqPtr == NULL)
    {
        return GT_BAD_PTR;
    }
    *tmFreqPtr = tmFreq;

    return GT_OK;
}

GT_STATUS prvCpssTmCtlDramFrequencyInit
(
    IN GT_U8                   devNum,
    IN GT_U8                   numOfBap
)
{
    GT_STATUS   rc = GT_OK;
    GT_U32      uTRC;
    GT_U32      uTFAW;
    GT_U32      uTRRD;
    GT_U32      uTRD_AP_TO_ACT;
    GT_U32      uTWR_AP_TO_ACT;
    GT_U32      uRD_2_WR_NOPS;
    GT_U32      uWR_2_RD_NOPS;
    GT_U32      uAFTER_REFR_NOPS;
    GT_U32      uREF_INT;

    IN CPSS_DRAM_FREQUENCY_ENT                   frequency;

    GT_U64 regData;
    GT_U64 writeData;
        int j;

    GT_U32        tmFreq;

    /* Read TM frequency */
    rc = prvCpssDxChTMFreqGet(devNum,&tmFreq);
    if (rc != GT_OK)
    {
        return rc;
    }

    switch(tmFreq)
    {
        case 2:
            frequency=CPSS_DRAM_FREQ_933_MHZ_E;
            break;
        case 3:
            frequency=CPSS_DRAM_FREQ_667_MHZ_E;
            break;
        case 1:
            frequency=CPSS_DRAM_FREQ_800_MHZ_E;
            break;
        case 0:
        default:
            cpssOsPrintf("DRam frequency: failed to define system frequency, exiting...\n");
            return GT_BAD_PARAM;
        }

    /*cpssOsPrintf("DRam frequency: %d (read from registry)\n ", frequency);*/

    switch (frequency)
    {

        case CPSS_DRAM_FREQ_667_MHZ_E:
        {
            uTRC  = 33;
            uTFAW = 30;
            uTRRD =  5;
            uTRD_AP_TO_ACT  = 22;
            uTWR_AP_TO_ACT  = 38;
            uRD_2_WR_NOPS   = 2;
            uWR_2_RD_NOPS   = 4;
            uAFTER_REFR_NOPS= 50;
            uREF_INT        = 2600;
            break;
        }
        case CPSS_DRAM_FREQ_800_MHZ_E:
        {
            uTRC  = 39;
            uTFAW = 32;
            uTRRD =  6;
            uTRD_AP_TO_ACT   = 27;
            uTWR_AP_TO_ACT   = 45;
            uRD_2_WR_NOPS    = 3;
            uWR_2_RD_NOPS    = 5;
            uAFTER_REFR_NOPS = 52;
            uREF_INT         = 3120;
            break;
        }
        case CPSS_DRAM_FREQ_933_MHZ_E:
        {
            uTRC  = 46;
            uTFAW = 33;
            uTRRD =  6;
            uTRD_AP_TO_ACT  = 33;
            uTWR_AP_TO_ACT  = 53;
            uRD_2_WR_NOPS   = 3;
            uWR_2_RD_NOPS   = 6;
            uAFTER_REFR_NOPS= 61;
            uREF_INT        = 3645;
            break;
        }
        default:
            cpssOsPrintf("\n undefined system frequency for initializing lads\n ");
            return GT_BAD_PARAM;
        break;
    }

    /* write registers*/
    writeData.l[1]=0;
    for (j = 0; j < numOfBap; j++)
    {
        WRITE_DATA_TO_REGISTER(TM.BAP[j].tRC,            0x3F,  uTRC)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].tFAW,           0x7F,  uTFAW)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].tRRD,           0x7,   uTRRD)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].RdApToActDelay, 0x3F,  uTRD_AP_TO_ACT)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].WrApToActDelay, 0x3F,  uTWR_AP_TO_ACT)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].Read2WriteNOPs, 0xF,   uRD_2_WR_NOPS)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].Write2ReadNOPs, 0xF,   uWR_2_RD_NOPS)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].AfterRefNOPs,   0x7F,  uAFTER_REFR_NOPS)
        WRITE_DATA_TO_REGISTER(TM.BAP[j].RefInterval,    0x3FFF,uREF_INT)
    }

    /*cpssOsPrintf("\n ***********Update default MAxRead/Write/Panic BAP parameters...\n ");*/
    for (j = 0; j < numOfBap; j++)
    {
        writeData.l[1]=0;
                /* Max Reads */
                writeData.l[0]=80;
                rc = cpssTmCtlWriteRegister( devNum,  TM.BAP[j].MaxReads, &writeData);
        if (rc) return rc;
                /* Max Reads (Write ) Panic*/
                writeData.l[0]=80;
                rc = cpssTmCtlWriteRegister( devNum,  TM.BAP[j].MaxReadsPanic, &writeData);
        if (rc) return rc;
                /* Max Writes */
                writeData.l[0]=96;
                rc = cpssTmCtlWriteRegister( devNum,  TM.BAP[j].MaxWrites, &writeData);
        if (rc) return rc;
                /* Max Writes(write) Panic */
                writeData.l[0]=96;
                rc = cpssTmCtlWriteRegister( devNum,  TM.BAP[j].MaxWritesPanic, &writeData);
        if (rc) return rc;
                /* Write (Data FIFOs) Panic Threshold */
                writeData.l[0]=8;
                rc = cpssTmCtlWriteRegister( devNum,  TM.BAP[j].WritePanicThresh, &writeData);
        if (rc) return rc;
                /* DRAM Request Shaping Parameters */
                writeData.l[0]=0xC6;       /* N parameter = 0xc6 , bits 0::13  */
                writeData.l[0]+=(1 << 16); /* K parameter = 1,     bits 16::29 */
                                writeData.l[1]=0;          /* L parameter = 0,     bits 32::45 */
                rc = cpssTmCtlWriteRegister( devNum,  TM.BAP[j].ReqShpPrms, &writeData);
        if (rc) return rc;

        }
    return rc;
}


/*******************************************************************************
* internal_cpssTmCtlHwInit
*
* DESCRIPTION:
*       Initialize the TM HW configuration.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*       xCat; xCat3; Lion; xCat2; DxCh1; DxCh1_Diamond; DxCh2; DxCh3; Lion2; Bobcat3; Puma2; Puma3; ExMx.
*
* INPUTS:
*       devNum            - Device number.
*       numOfLad            - Nubmer of lads.
*       frequency            - support 667 or 933 MHZ.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                       - on success.
*       GT_FAIL                     - on hardware error.
*       GT_NO_RESOURCE              - on out of memory space.
*       GT_NOT_APPLICABLE_DEVICE    - on not applicable device.
*       GT_BAD_STATE                - on burst mode or init doesnt succeed
*
* COMMENTS:
*       None.
*******************************************************************************/
GT_STATUS prvCpssTmCtlHwInit
(
    IN GT_U8                   devNum,
    IN GT_U8                   numOfLad
)
{

    GT_STATUS   rc = GT_OK;

    GT_U32      i;
    GT_U64      writeData;
    GT_U8       numOfBap;

    init_tm_alias_struct();

    numOfBap = numOfLad * 2;

    rc= prvCpssTmCtlDramFrequencyInit(devNum, numOfBap);
    if (rc)
    {
        cpssOsPrintf("%s %d  ERROR :rc=%d\n ",__FILE__, __LINE__ , rc);
        return rc;
    }


    rc= prvCpssTmCtlLadInit(devNum, numOfLad);
    if (rc)
    {
        cpssOsPrintf("%s %d  ERROR : rc=%d\n ",__FILE__, __LINE__ , rc);
        return rc;
    }

    rc= prvCpssTmCtlGlobalParamsInit(devNum);
    if (rc)
    {
        cpssOsPrintf("%s %d  ERROR : rc=%d\n ",__FILE__, __LINE__ , rc);
        return rc;
    }

    /*cpssOsPrintf("\n ***********HW INIT*********** \n ");*/

    WAIT_FOR_DEVICE_MAC(10*SLEEP_MS);

    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFF8,0xFFFFFFFF ,0x8000);
    if (rc)
    {
        cpssOsPrintf("%s %d  ERROR : rc=%d\n ",__FILE__, __LINE__ , rc);
        return rc;
    }

    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFFC,0xFFFFFFFF ,0x0000123);
    if (rc)
    {
        cpssOsPrintf("%s %d  ERROR : rc=%d\n ",__FILE__, __LINE__ , rc);
        return rc;
    }

    /*Write__to_start_external_init_Phase_8*/
    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001080,0xFFFFFFFF ,1);
    if (rc)
    {
        return rc;
    }

    rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0x0C001084,0xFFFFFFFF ,291);
    if (rc)
    {
        cpssOsPrintf("%s %d  ERROR : rc=%d\n ",__FILE__, __LINE__ , rc);
        return rc;
    }

   /*read_external_init_done_Phase_10*/
   for (i=0; i<WAIT_LOOP; i++)
   {
        WAIT_FOR_DEVICE_MAC(SLEEP_MS);

        rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFF8,0xFFFFFFFF ,0x8000);
        if (rc)
        {
            return rc;
        }
        rc= cpssDrvPpHwRegBitMaskWrite (devNum,0,0xC0FFFFC,0xFFFFFFFF ,0x0000123);
        if (rc)
        {
            return rc;
        }
        rc = prvCpssHwPpReadRegister(devNum, 0xC001088, &(writeData.l[0])); /* LSB */
        if (GT_OK != rc)
        {
            return rc;
        }
        rc = prvCpssHwPpReadRegister(devNum, 0xC00108C, &(writeData.l[1])); /* MSB */
        if (GT_OK != rc)
        {
            return rc;
        }
        #ifdef ASIC_SIMULATION
            /* no need more loops */
            writeData.l[0] |= 0x1;
        #endif /*ASIC_SIMULATION*/

        if (writeData.l[0] & 0x1)
        {
            break;
        }
    }
/* cpssOsPrintf("%s %d  rc=%d\n ",__FILE__, __LINE__ ,  rc); */

   if (i == WAIT_LOOP)
   {
       rc = GT_BAD_STATE;
   }

   return rc;
}


/*******************************************************************************
* cpssTmCtlLadParamsSet
*
* DESCRIPTION:
*       Set TM LAD parameters to its DB when neccessary to overwrite
*       its default configuration, parameters are used from DB
*       at cpssTmCtlLadInit.
*
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum       - physical device number.
*       numOfLads    - number of LADs.
*       ladParamsPtr - (pointer of) CPSS_TM_CTL_LAD_INF_PARAM_STC.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong device or configuration parameters
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS cpssTmCtlLadParamsSet
(
    IN  GT_U8                           devNum,
    IN  GT_U8                           numOfLads,
    IN  CPSS_TM_CTL_LAD_INF_PARAM_STC   *ladParamsPtr
)
{
    GT_STATUS rc = GT_OK;
    (void)devNum;
    /* 
    function is relevant before cpssInitSystem, next checks are ignored:
     
    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
    CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                               | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );

    PRV_CPSS_TM_CTL_CHECK_DRAM_INF_NUM_MAC(devNum, numOfLads);
    */

    CPSS_NULL_PTR_CHECK_MAC(ladParamsPtr);

    if (numOfLads > CPSS_TM_CTL_MAX_NUM_OF_LADS_CNS)
        return GT_BAD_VALUE;

    if (ladParamsPtr->minPkgSize != 0)
        tmLadParamsArr[numOfLads].minPkgSize = ladParamsPtr->minPkgSize;

    if (ladParamsPtr->pagesPerBank != 0)
        tmLadParamsArr[numOfLads].pagesPerBank = ladParamsPtr->pagesPerBank;

    if (ladParamsPtr->pkgesPerBank != 0)
        tmLadParamsArr[numOfLads].pkgesPerBank = ladParamsPtr->pkgesPerBank;

    if (ladParamsPtr->portChunksEmitPerSel != 0)
        tmLadParamsArr[numOfLads].portChunksEmitPerSel = ladParamsPtr->portChunksEmitPerSel;

    return rc;
}


/*******************************************************************************
* cpssTmCtlLadParamsGet
*
* DESCRIPTION:
*       Get TM LAD parameters from its DB.
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2;
*
* INPUTS:
*       devNum       - physical device number.
*       numOfLads    - number of LADs.
*       ladParamsPtr - (pointer of) CPSS_TM_CTL_LAD_INF_PARAM_STC.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong device or configuration parameters
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS cpssTmCtlLadParamsGet
(
    IN  GT_U8                         devNum,
    IN  GT_U8                         numOfLads,
    OUT CPSS_TM_CTL_LAD_INF_PARAM_STC *ladParamsPtr
)
{
    GT_STATUS   rc = GT_OK;
    (void)devNum;

    /* 
    function is relevant before cpssInitSystem, next checks are ignored:

    PRV_CPSS_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
    CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E | CPSS_LION_E | CPSS_XCAT2_E
                               | CPSS_LION2_E | CPSS_BOBCAT3_E | CPSS_PUMA_E | CPSS_PUMA3_E  );

    PRV_CPSS_TM_CTL_CHECK_DRAM_INF_NUM_MAC(devNum, numOfLads);
    */

    CPSS_NULL_PTR_CHECK_MAC(ladParamsPtr);

    if (numOfLads > CPSS_TM_CTL_MAX_NUM_OF_LADS_CNS)
        return GT_BAD_VALUE;

    *ladParamsPtr = tmLadParamsArr[numOfLads];
    return rc;
}

