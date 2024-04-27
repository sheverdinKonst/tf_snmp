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
* cpssDxChTmGluePfc.c
*
* DESCRIPTION:
*       Traffic Manager Glue - PFC API implementation.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*******************************************************************************/
#define CPSS_LOG_IN_MODULE_ENABLE
#include <cpss/dxCh/dxChxGen/tmGlue/private/prvCpssDxChTmGlueLog.h>

#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/cos/private/prvCpssDxChCoS.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGluePfc.h>
#include <cpss/dxCh/dxChxGen/tmGlue/private/prvCpssDxChTmGluePfc.h>
#include <cpss/extServices/cpssExtServices.h>

/*******************************************************************************
* internal_cpssDxChTmGluePfcTmTcPort2CNodeSet
*
* DESCRIPTION:
*       Sets C node value for given port and traffic class.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - source physical port number.
*       tc          - traffic class (APPLICABLE RANGES: 0..7).
*       cNodeValue  - C node value (APPLICABLE RANGES: 0...511).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number,
*                                  port or traffic class.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       Specific CNode must have one instance in TcPortToCnode Table,
*       CNodes are switched by the API to have such behaviour.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGluePfcTmTcPort2CNodeSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_U32                   tc,
    IN GT_U32                   cNodeValue
)
{
    GT_STATUS rc;
    PRV_CPSS_DXCH_TM_GLUE_PFC_CNODE_INFO_STC oldCnode;
    PRV_CPSS_DXCH_TM_GLUE_PFC_CNODE_INFO_STC newCnode;

    GT_U32 value;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_TM_GLUE_PFC_PHY_PORT_NUM_CHECK_MAC(portNum);
    PRV_CPSS_DXCH_COS_CHECK_TC_MAC(tc);

    if (cNodeValue >= BIT_9)
    {
        return GT_OUT_OF_RANGE;
    }


    if (prvPfcTmGlueCnodeMappingOnce == GT_TRUE)
    {
        /* look for cNode which exist in [portNum][tc] */
        rc = prvCpssDxChTmGluePfcTmDbGetCNodeByTcPort(devNum, portNum, tc, &value);
        if (rc != GT_OK)
        {
            return rc;
        }
        newCnode.port = portNum;
        newCnode.tc = tc;
        newCnode.cNode = (GT_U16)cNodeValue;


        /* look for cNodeValue [port][tc] location */
        rc = prvCpssDxChTmGluePfcTmDbGetTcPortByCNode(devNum, cNodeValue, &oldCnode.port, &oldCnode.tc);
        if (rc != GT_OK)
        {
            return rc;
        }
        oldCnode.cNode = (GT_U16)value;


        /*   move cNode from [portNum][tc] to oldCnode [port][tc]  */
        rc =  prvCpssDxChWriteTableEntryField(devNum,
                     PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                     oldCnode.port,
                     PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                     oldCnode.tc * 9, 9, value);

        if (rc != GT_OK)
        {
            return rc;
        }
    }

    /* Set TC Port 2 CNode Entry - TC Port 2 CNode */
    rc =  prvCpssDxChWriteTableEntryField(devNum,
                 PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                 portNum,
                 PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                 tc * 9, 9, cNodeValue);

    if (rc != GT_OK)
    {
        return rc;
    }

    /* update Port2CnodeShadow DB */
    if (prvPfcTmGlueCnodeMappingOnce == GT_TRUE)
    {
        /* update DB with switched cNodes */

        /* update Port2CnodeShadow DB */
        rc = prvCpssDxChTmGluePfcTmDbUpdateTcPortCNode(devNum, &oldCnode, &newCnode);
        if (rc != GT_OK)
        {
            return rc;
        }
    }

    return rc;
}

/*******************************************************************************
* cpssDxChTmGluePfcTmTcPort2CNodeSet
*
* DESCRIPTION:
*       Sets C node value for given port and traffic class.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - source physical port number.
*       tc          - traffic class (APPLICABLE RANGES: 0..7).
*       cNodeValue  - C node value (APPLICABLE RANGES: 0...511).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number,
*                                  port or traffic class.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGluePfcTmTcPort2CNodeSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_U32                   tc,
    IN GT_U32                   cNodeValue
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGluePfcTmTcPort2CNodeSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, tc, cNodeValue));

    rc = internal_cpssDxChTmGluePfcTmTcPort2CNodeSet(devNum, portNum, tc, cNodeValue);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, tc, cNodeValue));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGluePfcTmTcPort2CNodeGet
*
* DESCRIPTION:
*       Gets C node for given traffic class and port.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - source physical port number.
*       tc          - traffic class (APPLICABLE RANGES: 0..7).
*
* OUTPUTS:
*       cNodeValuePtr - (pointer to) C node value.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, C node,
*                                  traffic class or port.
*       GT_BAD_PTR               - on NULL pointer
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGluePfcTmTcPort2CNodeGet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_U32                   tc,
    OUT GT_U32                 *cNodeValuePtr
)
{
    GT_U32  value;                  /* HW field value */
    GT_STATUS rc;                   /* return code */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_TM_GLUE_PFC_PHY_PORT_NUM_CHECK_MAC(portNum);
    PRV_CPSS_DXCH_COS_CHECK_TC_MAC(tc);
    CPSS_NULL_PTR_CHECK_MAC(cNodeValuePtr);


    /* Get TC Port 2 CNode Entry - TC Port 2 CNode */
    rc = prvCpssDxChReadTableEntryField(devNum,
                 PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                 portNum,
                 PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                 tc * 9, 9, &value);
    if(rc != GT_OK)
    {
        return rc;
    }

    *cNodeValuePtr = value;

    return GT_OK;
}

/*******************************************************************************
* cpssDxChTmGluePfcTmTcPort2CNodeGet
*
* DESCRIPTION:
*       Gets C node for given traffic class and port.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - source physical port number.
*       tc          - traffic class (APPLICABLE RANGES: 0..7).
*
* OUTPUTS:
*       cNodeValuePtr - (pointer to) C node value.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, C node,
*                                  traffic class or port.
*       GT_BAD_PTR               - on NULL pointer
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGluePfcTmTcPort2CNodeGet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_U32                   tc,
    OUT GT_U32                 *cNodeValuePtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGluePfcTmTcPort2CNodeGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, tc, cNodeValuePtr));

    rc = internal_cpssDxChTmGluePfcTmTcPort2CNodeGet(devNum, portNum, tc, cNodeValuePtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, tc, cNodeValuePtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGluePfcResponseModeSet
*
* DESCRIPTION:
*       Enable/Disable PFC (Priority Flow Control) support with Traffic Manager (TM).
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       responseMode    - PFC response mode.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number or PFC response mode.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGluePfcResponseModeSet
(
    IN GT_U8 devNum,
    IN CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_ENT responseMode
)
{
    GT_STATUS rc;                   /* return code */
    GT_U32 regAddr;                 /* registre address */
    GT_U32 data;                    /* HW data */
    GT_U32 pfcData;                 /* HW data */
    GT_U32 bitIndex;  /* bit index */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);

    switch(responseMode)
    {
        case CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_TM_E:
            /* TM FCU ingress must be enabled if PFC is working using TM */
            data = 0x1;
            pfcData = 0x1;
            break;
        case CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_TXQ_E:
            data = 0x4;
            pfcData = 0x0;
            break;
        default:
            return GT_BAD_PARAM;
    }

    if(PRV_CPSS_SIP_5_10_CHECK_MAC(devNum))
    {
        bitIndex = 12;
    }
    else
    {
        bitIndex = 11;
    }

    regAddr = PRV_DXCH_REG1_UNIT_TXQ_PFC_MAC(devNum).PFCTriggerGlobalConfig;
    rc = prvCpssHwPpSetRegField(devNum, regAddr, bitIndex, 1, pfcData);
    if(GT_OK != rc)
    {
        return rc;
    }

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMFCUGlobalConfigs;
    rc = prvCpssHwPpPortGroupWriteRegBitMask(devNum,
                                             CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                             regAddr, 0x5, data);
    return rc;
}

/*******************************************************************************
* cpssDxChTmGluePfcResponseModeSet
*
* DESCRIPTION:
*       Enable/Disable PFC (Priority Flow Control) support with Traffic Manager (TM).
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum          - device number.
*       responseMode    - PFC response mode.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number or PFC response mode.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGluePfcResponseModeSet
(
    IN GT_U8 devNum,
    IN CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_ENT responseMode
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGluePfcResponseModeSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, responseMode));

    rc = internal_cpssDxChTmGluePfcResponseModeSet(devNum, responseMode);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, responseMode));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGluePfcResponseModeGet
*
* DESCRIPTION:
*       Get PFC (Priority Flow Control) for TM support status.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum  - device number.
*
* OUTPUTS:
*       responseModePtr          - (pointert to) PFC response mode.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number number.
*       GT_BAD_PTR               - on NULL pointer.
*       GT_HW_ERROR              - on hardware error.
*       GT_BAD_STATE             - on wrong response mode.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGluePfcResponseModeGet
(
    IN GT_U8 devNum,
    OUT CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_ENT *responseModePtr
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
    CPSS_NULL_PTR_CHECK_MAC(responseModePtr);

    regAddr = PRV_DXCH_REG1_UNIT_TM_FCU_MAC(devNum).TMFCUGlobalConfigs;
    rc = prvCpssHwPpPortGroupReadRegBitMask(devNum,
                                            CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
                                            regAddr, 0x5, &data);
    if(rc != GT_OK)
    {
        return rc;
    }

    if(data == 0x1)
    {
        *responseModePtr = CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_TM_E;
    }
    else if(data == 0x4)
    {
        *responseModePtr = CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_TXQ_E;
    }
    else
    {
        return GT_BAD_STATE;
    }

    return GT_OK;
}

/*******************************************************************************
* cpssDxChTmGluePfcResponseModeGet
*
* DESCRIPTION:
*       Get PFC (Priority Flow Control) for TM support status.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum  - device number.
*
* OUTPUTS:
*       responseModePtr          - (pointert to) PFC response mode.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number number.
*       GT_BAD_PTR               - on NULL pointer.
*       GT_HW_ERROR              - on hardware error.
*       GT_BAD_STATE             - on wrong response mode.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGluePfcResponseModeGet
(
    IN GT_U8 devNum,
    OUT CPSS_DXCH_TM_GLUE_PFC_RESPONSE_MODE_ENT *responseModePtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGluePfcResponseModeGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, responseModePtr));

    rc = internal_cpssDxChTmGluePfcResponseModeGet(devNum, responseModePtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, responseModePtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGluePfcPortMappingSet
*
* DESCRIPTION:
*       Map physical port to pfc port, used to map physical ports 0..255
*       to pfc ports 0..63.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number. (APPLICABLE RANGES: 0..255). *
*       pfcPortNum  - pfc port number which is used at
*                     cpssDxChTmGluePfcTmTcPort2CNodeSet. (APPLICABLE RANGES: 0..63).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, or port.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGluePfcPortMappingSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_PHYSICAL_PORT_NUM     pfcPortNum
)
{
    GT_STATUS rc;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E| CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    if(!PRV_CPSS_SIP_5_10_CHECK_MAC(devNum))
    {
        return GT_NOT_APPLICABLE_DEVICE;
    }

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_ENHANCED_PHY_PORT_OR_CPU_PORT_CHECK_MAC(devNum, portNum);
    PRV_CPSS_DXCH_TM_GLUE_PFC_PHY_PORT_NUM_CHECK_MAC(pfcPortNum);

    /* Set Physical Port to PFC Port mapping Entry */
    rc =  prvCpssDxChWriteTableEntryField(devNum,
                 PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_PORT_TO_PHYSICAL_PORT_MAPPING_E,
                 portNum / 4,
                 0,
                 (portNum % 4) * 8, 6, pfcPortNum);

        return rc;
}

/*******************************************************************************
* cpssDxChTmGluePfcPortMappingSet
*
* DESCRIPTION:
*       Map physical port to pfc port, used to map physical ports 0..255
*       to pfc ports 0..63.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number. (APPLICABLE RANGES: 0..255). *
*       pfcPortNum  - pfc port number which is used at
*                     cpssDxChTmGluePfcTmTcPort2CNodeSet. (APPLICABLE RANGES: 0..63).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, or port.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGluePfcPortMappingSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_PHYSICAL_PORT_NUM     pfcPortNum
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGluePfcPortMappingSet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, pfcPortNum));

    rc = internal_cpssDxChTmGluePfcPortMappingSet(devNum, portNum, pfcPortNum);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, pfcPortNum));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}

/*******************************************************************************
* internal_cpssDxChTmGluePfcPortMappingGet
*
* DESCRIPTION:
*       Get physical port to pfc port mapping.

*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number. (APPLICABLE RANGES: 0..255).
*
* OUTPUTS:
*       pfcPortNum  - (pointer to) pfc port number which is used at
*                     cpssDxChTmGluePfcTmTcPort2CNodeSet. (APPLICABLE RANGES: 0..63).
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, or port.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS internal_cpssDxChTmGluePfcPortMappingGet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    OUT GT_PHYSICAL_PORT_NUM    *pfcPortNumPtr
)
{
    GT_STATUS rc;                   /* return code */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    if(!PRV_CPSS_SIP_5_10_CHECK_MAC(devNum))
    {
        return GT_NOT_APPLICABLE_DEVICE;
    }

        rc = prvCpssDxChTmGluePfcPortMappingGet(devNum, portNum, pfcPortNumPtr);

    return rc;
}

/*******************************************************************************
* cpssDxChTmGluePfcPortMappingGet
*
* DESCRIPTION:
*       Get physical port to pfc port mapping.

*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number. (APPLICABLE RANGES: 0..255).
*
* OUTPUTS:
*       pfcPortNum  - (pointer to) pfc port number which is used at
*                     cpssDxChTmGluePfcTmTcPort2CNodeSet. (APPLICABLE RANGES: 0..63).
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, or port.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS cpssDxChTmGluePfcPortMappingGet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    OUT GT_PHYSICAL_PORT_NUM    *pfcPortNumPtr
)
{
    GT_STATUS rc;
    CPSS_LOG_FUNC_VARIABLE_DECLARE_MAC(funcId, cpssDxChTmGluePfcPortMappingGet);

    CPSS_API_LOCK_MAC(0,0);
    CPSS_LOG_API_ENTER_MAC((funcId, devNum, portNum, pfcPortNumPtr));

    rc = internal_cpssDxChTmGluePfcPortMappingGet(devNum, portNum, pfcPortNumPtr);

    CPSS_LOG_API_EXIT_MAC(funcId, rc);
    CPSS_APP_SPECIFIC_CB_MAC((funcId, rc, devNum, portNum, pfcPortNumPtr));
    CPSS_API_UNLOCK_MAC(0,0);

    return rc;
}


