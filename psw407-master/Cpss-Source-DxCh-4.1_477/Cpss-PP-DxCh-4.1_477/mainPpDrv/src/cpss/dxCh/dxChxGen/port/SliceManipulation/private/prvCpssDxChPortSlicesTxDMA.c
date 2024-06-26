/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortSlicesTxDMA.c
*
* DESCRIPTION:
*       Tx DMA CTU Unit Slices Manipulation Interface
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*******************************************************************************/

#include <cpss/generic/cpssTypes.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h> 
#include <cpss/dxCh/dxChxGen/port/SliceManipulation/private/prvCpssDxChPortSlicesTxDMA.h>

/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMASlicesNumSet
*
* DESCRIPTION:
*      The function sets slice number used in Pizza Arbiter for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMASlicesNumSet
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
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    /* When 0 all the slots are in use */
    if (slicesNum > TxDMA_TOTAL_SLICES_CNS)
    {
        return GT_OUT_OF_RANGE;
    }
    if (slicesNum == TxDMA_TOTAL_SLICES_CNS)
    {
        slicesNum2Set = 0;
    }
    else
    {
        slicesNum2Set = slicesNum;
    }

    /* number of slices in RX DMA pizza arbiter and 
       enable strict priority for CPU (allocate slice for it)
       problem to do on all cores because anyway CPU connected just to 0 */
    fieldData = slicesNum2Set;
    if (enableStrictPriority4CPU)
    {
        fieldData |= TxDMA_CTRL_STRICT_PRIORITY_BIT_CNS; /* set bit of strict priority for CPU */
    }

    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.txdmaPZCtrlReg;
    rc = prvCpssHwPpPortGroupSetRegField(devNum, portGroupId, regAddr,
                                            0, 6, fieldData);
    return rc;
}

/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMASlicesNumGet
*
* DESCRIPTION:
*      The function get slice number used in Pizza Arbiter and strict priority state bit 
*      for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMASlicesNumGet
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
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    if (NULL == slicesNumPtr)
    {
        return GT_BAD_PTR;
    }
    if (NULL == enableStrictPriority4CPUPtr)
    {
        return GT_BAD_PTR;
    }

    /* number of slices in TX DMA pizza arbiter */
    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.txdmaPZCtrlReg;
    rc = prvCpssHwPpPortGroupGetRegField(devNum, portGroupId, regAddr,
                                            0, 6, &fieldData);

    if (rc != GT_OK)
    {
        return rc;
    }

    *slicesNumPtr = fieldData & TxDMA_CTRL_SLICE_NUM_BITS_CNS;
    if (*slicesNumPtr == 0)
    {
        *slicesNumPtr = TxDMA_TOTAL_SLICES_CNS;
    }
    *enableStrictPriority4CPUPtr = (fieldData & TxDMA_CTRL_STRICT_PRIORITY_BIT_CNS) == TxDMA_CTRL_STRICT_PRIORITY_BIT_CNS;
    return rc;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMASliceOccupy
*
* DESCRIPTION:
*      The function occupy the slice (i.e. set the spicific slice 
*      be asigned to specific port for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMASliceOccupy
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId,
    IN GT_PHYSICAL_PORT_NUM portId
)
{
    GT_STATUS rc;
    /* GT_U32 regAddr;
    GT_U32 fieldData; */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    rc = prvLion2PortPizzaArbiterTxDMASliceStateSet(devNum,portGroupId,sliceId,GT_TRUE,portId);
    if (GT_OK != rc)
    {
        return rc;
    }
    return GT_OK;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMASliceRelease
*
* DESCRIPTION:
*      The function release the slice (i.e. set the spicific slice 
*      to be disable to any port) for BM unit)
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMASliceRelease
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId
)
{
    GT_STATUS rc;
    /* GT_U32    regAddr;
    GT_U32    fieldData; */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    rc = prvLion2PortPizzaArbiterTxDMASliceStateSet(devNum,portGroupId,sliceId,GT_FALSE,0);
    if (GT_OK != rc)
    {
        return rc;
    }
    return GT_OK;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMASliceStateSet
*
* DESCRIPTION:
*      The function sets the state of the slice (i.e. if slace is disable or enable
*      and if it enbaled to which port it is assigned) for TXDMA unit.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
*
* INPUTS:
*       devNum      - device number
*       portGroupId - port group id
*       sliceId     - slice to be released
*       isEnabled   - is slace enabled
*       portId      - pord id slace is enabled to
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
GT_STATUS prvLion2PortPizzaArbiterTxDMASliceStateSet
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId,
    IN GT_BOOL              isEnabled, 
    IN GT_PHYSICAL_PORT_NUM portId
)
{
    GT_STATUS rc;
    GT_U32 regAddr;
    GT_U32 fieldData;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    if (sliceId >= TxDMA_TOTAL_SLICES_CNS) 
    {
        return GT_OUT_OF_RANGE;
    }

    /* field is 4 bit length */
    if (portId >= PRV_CPSS_GEN_PP_MAX_PORTS_IN_PORT_GROUP_CNS)
    {
        return GT_OUT_OF_RANGE;
    }

    fieldData = portId; /* map current slot to this port */
    if (GT_TRUE == isEnabled)
    {
        fieldData |= TxDMA_SLICE_ENABLE_BIT_CNS; /* (1<<4);   *//* enable slot mapping by arbiter */
    }
    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.txdmaPZSliceMapReg[sliceId/4];
    rc = prvCpssHwPpPortGroupSetRegField(devNum, portGroupId, regAddr,
                                            (sliceId%4)*5, 5, fieldData);
    if (GT_OK != rc)
    {
        return rc;
    }
    return GT_OK;
}



/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMASliceGetState
*
* DESCRIPTION:
*      The function gets the state of the slice (i.e. if slace is disable or enable
*      and if it enbaled to which port it is assigned) for BM unit.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMASliceGetState
(
    IN  GT_U8  devNum,
    IN  GT_U32 portGroupId,
    IN  GT_U32 sliceId,
    OUT GT_BOOL              *isOccupiedPtr, 
    OUT GT_PHYSICAL_PORT_NUM *portNumPtr
)
{
    GT_STATUS rc;
    GT_U32 regAddr;
    GT_U32 fieldData;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    if (NULL == isOccupiedPtr) 
    {
        return GT_BAD_PTR;
    }
    if (NULL == portNumPtr) 
    {
        return GT_BAD_PTR;
    }
    if (sliceId >= TxDMA_TOTAL_SLICES_CNS) 
    {
        return GT_OUT_OF_RANGE;
    }

    *isOccupiedPtr = GT_FALSE;
    *portNumPtr    = 0xFF;

    regAddr = PRV_CPSS_DXCH_DEV_REGS_MAC(devNum)->bufferMng.txdmaPZSliceMapReg[sliceId/4];
    rc = prvCpssHwPpPortGroupGetRegField(devNum, portGroupId, regAddr,
                                            (sliceId%4)*5, 5, &fieldData);
    if (rc != GT_OK) 
    {
        return rc;
    }
    *isOccupiedPtr = (GT_BOOL)((fieldData & TxDMA_SLICE_ENABLE_BIT_CNS) == TxDMA_SLICE_ENABLE_BIT_CNS);
    *portNumPtr    = fieldData  & TxDMA_SLICE_PORT_BITS_CNS;
    return GT_OK;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMAInit
*
* DESCRIPTION:
*      Init TxDMA unit (sets number of slice to use , strict priority to CPU bit 
*      and disables all slices of the device.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMAInit
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
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    rc = prvLion2PortPizzaArbiterTxDMASlicesNumSet(/*IN*/devNum,/*IN*/portGroupId,
                                                   /*IN*/sliceNum2Init,
                                                   /*IN*/GT_FALSE);
    if(rc != GT_OK)
    {
        return rc;
    }
    for (sliceId = 0 ; sliceId < TxDMA_TOTAL_SLICES_CNS; sliceId++) 
    {
        rc = prvLion2PortPizzaArbiterTxDMASliceRelease(devNum,portGroupId,sliceId);
        if(rc != GT_OK)
        {
            return rc;
        }
    }
    return rc;
}

/*--------------------------------------------------*/
/*  Patch 360 Mhz 12 Slice                          */
/*--------------------------------------------------*/


/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_ConvertSliceIdx
*
* DESCRIPTION:
*      The function compute new slice position for TxDMA pizza 360 Mhz 12 Slices
*      be asigned to specific port for BM unit
*
* APPLICABLE DEVICES:
*        supported : Lion2 A0 B0 
*
* NOT APPLICABLE DEVICES:
*
* INPUTS:
*       sliceIdx     - slice to be occupied
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_U32 prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_ConvertSliceIdx
(
    IN GT_U32 sliceIdx
)
{
    GT_U32 newSliceIdx;

    newSliceIdx = (sliceIdx/3) * 4 + (sliceIdx%3);
    return newSliceIdx;
}


/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_SliceOccupy
*
* DESCRIPTION:
*      The function occupy the slice (i.e. set the spicific slice 
*      be asigned to specific port for BM unit
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_SliceOccupy
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId,
    IN GT_PHYSICAL_PORT_NUM portId
)
{
    GT_U32 newSliceId;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    newSliceId = prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_ConvertSliceIdx(sliceId);
    return prvLion2PortPizzaArbiterTxDMASliceOccupy(devNum,portGroupId,newSliceId, portId);

}

/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_SliceRelease
*
* DESCRIPTION:
*      The function release the slice (i.e. set the spicific slice 
*      to be disable to any port) for BM unit)
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_SliceRelease
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceId
)
{
    GT_U32 newSliceId;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);


    newSliceId = prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_ConvertSliceIdx(sliceId);
    return prvLion2PortPizzaArbiterTxDMASliceRelease(devNum,portGroupId,newSliceId);
}

/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_SliceGetState
*
* DESCRIPTION:
*      The function gets the state of the slice (i.e. if slace is disable or enable
*      and if it enbaled to which port it is assigned) for BM unit.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_SliceGetState
(
    IN  GT_U8  devNum,
    IN  GT_U32 portGroupId,
    IN  GT_U32 sliceId,
    OUT GT_BOOL              *isOccupiedPtr, 
    OUT GT_PHYSICAL_PORT_NUM *portNumPtr
)
{
    GT_U32 newSliceId;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);


    newSliceId = prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_ConvertSliceIdx(sliceId);
    return prvLion2PortPizzaArbiterTxDMASliceGetState(devNum,portGroupId,newSliceId,/*OUT*/isOccupiedPtr,portNumPtr); 
}

/*******************************************************************************
* prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_Init
*
* DESCRIPTION:
*      Init BM unit (sets number of slice to use , strict priority to CPU bit 
*      and disables all slices of the device.
*
* APPLICABLE DEVICES:
*        Lion2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Bobcat2; Caelum; Bobcat3.
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
GT_STATUS prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_Init
(
    IN GT_U8  devNum,
    IN GT_U32 portGroupId,
    IN GT_U32 sliceNum2Init
)
{
    GT_STATUS rc;
    GT_U32    sliceId;
    GT_U32    newSliceNum2Init;
    GT_PHYSICAL_PORT_NUM portNum;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E 
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E 
                                            | CPSS_LION_E | CPSS_XCAT2_E | CPSS_BOBCAT2_E | CPSS_CAELUM_E | CPSS_BOBCAT3_E);

    newSliceNum2Init = prvLion2PortPizzaArbiterTxDMA_360Mhz_12Slices_ConvertSliceIdx(sliceNum2Init);


    rc = prvLion2PortPizzaArbiterTxDMAInit(devNum,portGroupId,newSliceNum2Init);
    if(rc != GT_OK)
    {
        return rc;
    }
    /*---------------------------------------------------------------------------*/
    /* each 4-th slice beginning from 3 shall be assigned to port 9 and disabled */
    /*---------------------------------------------------------------------------*/
    portNum = 9; 
    for (sliceId = 3 ; sliceId < newSliceNum2Init; sliceId+=4) 
    {
        rc = prvLion2PortPizzaArbiterTxDMASliceStateSet(devNum,portGroupId,sliceId,GT_FALSE,portNum);
        if (GT_OK != rc)
        {
            return rc;
        }
    }
    return rc;
}

