/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortSlicesRxDMACTU.c
*
* DESCRIPTION:
*       Rx DMA CTU Unit Slices Manipulation Interface
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*******************************************************************************/


#include <cpss/generic/cpssTypes.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h> 
#include <cpss/dxCh/dxChxGen/port/SliceManipulation/private/prvCpssDxChPortSlicesRxDMACTU.h>

/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUSlicesNumSet
*
* DESCRIPTION:
*      The function sets slice number used in Pizza Arbiter for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*       slicesNum   - slices number to be set in pizza arbiter
*       enableStrictPriority4CPU - strict priority to CPU bit shall be set or not
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_HW_ERROR     - hw error
*       GT_OUT_OF_RANGE - number of slice to be set is greater than available
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUSlicesNumSet
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 slicesNum,
    IN GT_BOOL enableStrictPriority4CPU
)
{
    GT_STATUS   rc;                 /* return code */
    GT_U32      regAddr;            /* register's address */
    GT_U32      slicesNum2Set;
    GT_U32      fieldData;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    /* When 0 all the slots are in use */
    if (slicesNum > RxDMACTU_TOTAL_SLICES_CNS)
    {
        return GT_OUT_OF_RANGE;
    }
    if (slicesNum == RxDMACTU_TOTAL_SLICES_CNS)
    {
        slicesNum2Set = 0;
    }
    else
    {
        slicesNum2Set = slicesNum;
    }

    fieldData = slicesNum2Set;
    if (enableStrictPriority4CPU)
    {
        fieldData |= RxDMACTU_CTRL_STRICT_PRIORITY_BIT_CNS; /* set bit of strict priority for CPU */
    }
    
    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.rxdmaCTUCtrlReg;
    rc = prvCpssHwPpPortGroupSetRegField(devNum, portGroupId, regAddr,
                                            0, 6, fieldData);
    return rc;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUSlicesNumGet
*
* DESCRIPTION:
*      The function get slice number used in Pizza Arbiter and strict priority state bit 
*      for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*
* OUTPUTS:
*       slicesNumPtr   - place to store slices number used in pizza arbiter
*       enableStrictPriority4CPUPtr - where to store strict priority to CPU bit 
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_BAD_PTR      - bad slicesNumPtr, enableStrictPriority4CPUPtr
*       GT_HW_ERROR     - hw error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUSlicesNumGet
(
    IN  GT_U8   devNum,
    IN  GT_U32  portGroupId,
    OUT GT_U32  *slicesNumPtr,
    OUT GT_BOOL *enableStrictPriority4CPUPtr
)
{
    GT_STATUS   rc;                 /* return code */
    GT_U32      regAddr;            /* register's address */
    GT_U32      fieldData;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    if (NULL == slicesNumPtr)
    {
        return GT_BAD_PTR;
    }
    if (NULL == enableStrictPriority4CPUPtr)
    {
        return GT_BAD_PTR;
    }


    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.rxdmaCTUCtrlReg;
    rc = prvCpssHwPpPortGroupGetRegField(devNum, portGroupId, regAddr,
                                            0, 6, /*OUT*/&fieldData);
    if (rc != GT_OK)
    {
        return rc;
    }

    *slicesNumPtr = fieldData & RxDMACTU_CTRL_SLICE_NUM_BITS_CNS;
    if (*slicesNumPtr == 0)
    {
        *slicesNumPtr = RxDMACTU_TOTAL_SLICES_CNS;
    }
    *enableStrictPriority4CPUPtr = (fieldData & RxDMACTU_CTRL_STRICT_PRIORITY_BIT_CNS) == RxDMACTU_CTRL_STRICT_PRIORITY_BIT_CNS;
    return rc;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUSliceOccupy
*
* DESCRIPTION:
*      The function occupy the slice (i.e. set the spicific slice 
*      be asigned to specific port for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*       sliceId     - slice to be occupied
*       portId      - local port id (inside portGroup) to which slice is assigned
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_OUT_OF_RANGE - sliceId or portId are greater than available
*       GT_HW_ERROR     - hw error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUSliceOccupy
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId,
    IN GT_PHYSICAL_PORT_NUM portId
)
{
    GT_STATUS   rc;
    /* GT_U32      regAddr;
    GT_U32      fieldData;         */
    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    rc = prvLion2PortPizzaArbiterRxDMACTUSliceStateSet(devNum,portGroupId,sliceId,GT_TRUE,portId);
    if (GT_OK != rc)
    {
        return rc;
    }
    return GT_OK;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUSliceRelease
*
* DESCRIPTION:
*      The function release the slice (i.e. set the spicific slice 
*      to be disable to any port) for BM unit)
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*       sliceId     - slice to be released
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_OUT_OF_RANGE - sliceId or portId are greater than available
*       GT_HW_ERROR     - hw error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUSliceRelease
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId
)
{
    GT_STATUS rc;
    /* GT_U32 	  regAddr;
    GT_U32    fieldData; */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    rc = prvLion2PortPizzaArbiterRxDMACTUSliceStateSet(devNum,portGroupId,sliceId,GT_FALSE,0);
    if (GT_OK != rc)
    {
        return rc;
    }
    return GT_OK;
}

/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUSliceStateSet
*
* DESCRIPTION:
*      The function occupy the slice (i.e. set the spicific slice 
*      be asigned to specific port for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*       sliceId     - slice to be released
*       isEnabled   - is slice enabled 
*       portId      - port Num slice is assigned to 
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_OUT_OF_RANGE - sliceId are greater than available
*       GT_BAD_PTR      - isOccupiedPtr or portNumPtr are NULL
*       GT_HW_ERROR     - hw error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUSliceStateSet
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   sliceId,
    IN GT_BOOL  isEnabled, 
    IN GT_PHYSICAL_PORT_NUM portId
)
{
    GT_STATUS   rc;
    GT_U32      regAddr;
    GT_U32      fieldData;          /* value to write to register */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    if (sliceId >= RxDMACTU_TOTAL_SLICES_CNS) 
    {
        return GT_OUT_OF_RANGE;
    }

    /* field is 4 bit length */
    if (portId >= PRV_CPSS_GEN_PP_MAX_PORTS_IN_PORT_GROUP_CNS)
    {
        return GT_OUT_OF_RANGE;
    }

    fieldData = portId;                       /* map current slot to this port */
    if (GT_TRUE == isEnabled)
    {
        fieldData |= RxDMACTU_SLICE_ENABLE_BIT_CNS;   /* enable slot mapping by arbiter */
    }
    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.rxdmaCTUSliceMapReg[sliceId/4];
    rc = prvCpssHwPpPortGroupSetRegField(devNum, portGroupId, regAddr,
                                            (sliceId%4)*5, 5, fieldData);
    if (GT_OK != rc)
    {
        return rc;
    }
    return GT_OK;
}



/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUSliceGetState
*
* DESCRIPTION:
*      The function gets the state of the slice (i.e. if slace is disable or enable
*      and if it enbaled to which port it is assigned) for BM unit.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*       sliceId     - slice to be released
*
* OUTPUTS:
*       isOccupiedPtr - place to store whether slice is occupied
*        portNumPtr   - plavce to store to which port slice is assigned
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_OUT_OF_RANGE - sliceId are greater than available
*       GT_BAD_PTR      - isOccupiedPtr or portNumPtr are NULL
*       GT_HW_ERROR     - hw error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUSliceGetState
(
    IN  GT_U8  devNum,
    IN  GT_U32 portGroupId,
    IN  GT_U32 sliceId,
    OUT GT_BOOL              *isOccupiedPtr, 
    OUT GT_PHYSICAL_PORT_NUM *portNumPtr
)
{
    GT_STATUS   rc;
    GT_U32      regAddr;
    GT_U32      fieldData;          /* value to write to register */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);
    if (NULL == isOccupiedPtr)
    {
        return GT_BAD_PTR;
    }

    if (NULL == portNumPtr) 
    {
        return GT_BAD_PTR;
    }
    if (sliceId >= RxDMACTU_TOTAL_SLICES_CNS) 
    {
        return GT_OUT_OF_RANGE;
    }


    *isOccupiedPtr = GT_FALSE;
    *portNumPtr    = 0xFF;

    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.rxdmaCTUSliceMapReg[sliceId/4];
    rc = prvCpssHwPpPortGroupGetRegField(devNum, portGroupId, regAddr,
                                            (sliceId%4)*5, 5, &fieldData);

    if (rc != GT_OK) 
    {
        return rc;
    }
    *isOccupiedPtr = (GT_BOOL)((fieldData & RxDMACTU_SLICE_ENABLE_BIT_CNS) == RxDMACTU_SLICE_ENABLE_BIT_CNS);
    *portNumPtr    =  fieldData & RxDMACTU_SLICE_PORT_BITS_CNS;

    return GT_OK;

}

/*******************************************************************************
* prvLion2PortPizzaArbiterRxDMACTUInit
*
* DESCRIPTION:
*      Init BM unit (sets number of slice to use , strict priority to CPU bit 
*      and disables all slices of the device.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2.
*
* INPUTS:
*       devNum        - device number
*       portGroupId   - port group id
*       sliceNum2Init - number of slices to be used 
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success,
*       GT_BAD_PARAM    - bad devNum, portGroupId
*       GT_OUT_OF_RANGE - sliceNum2Init is greater than available
*       GT_HW_ERROR     - hw error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvLion2PortPizzaArbiterRxDMACTUInit
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceNum2Init
)
{
    GT_STATUS rc;
    GT_U32    sliceId;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E);

    rc = prvLion2PortPizzaArbiterRxDMACTUSlicesNumSet(/*IN*/devNum,/*IN*/portGroupId,
                                                     /*IN*/sliceNum2Init,
                                                     /*IN*/GT_FALSE);
    if(rc != GT_OK)
    {
        return rc;
    }
    for (sliceId = 0 ; sliceId < RxDMACTU_TOTAL_SLICES_CNS; sliceId++) 
    {
        rc = prvLion2PortPizzaArbiterRxDMACTUSliceRelease(devNum,portGroupId,sliceId);
        if(rc != GT_OK)
        {
            return rc;
        }
    }
    return rc;
}

