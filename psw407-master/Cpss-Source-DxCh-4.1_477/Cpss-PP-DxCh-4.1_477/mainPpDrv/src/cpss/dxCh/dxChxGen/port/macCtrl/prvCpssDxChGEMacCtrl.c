/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChGEMacCtrl.c
*
* DESCRIPTION:
*       bobcat2 GE mac control 
*
* FILE REVISION NUMBER:
*       $Revision: 4$
*******************************************************************************/

#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/macCtrl/prvCpssDxChGEMacCtrl.h>
#include <cpss/dxCh/dxChxGen/port/macCtrl/prvCpssDxChMacCtrl.h>

/*---------------------------------------------------------------------------------------------
 * /Cider/EBU/Bobcat2/Bobcat2 {Current}/Switching Core/GOP/<Gige MAC IP> Gige MAC IP Units%g/Tri-Speed Port MAC Configuration/Port MAC Control Register1
 * 15 - 15  Short Tx preable    0x0 -- 8 bytes
 *                              0x1 -- 4 bytes
 * /Cider/EBU/Bobcat2/Bobcat2 {Current}/Switching Core/GOP/<Gige MAC IP> Gige MAC IP Units%g/Tri-Speed Port MAC Configuration/Port MAC Control Register3
 * 6-14     IPG 
 *---------------------------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------------------------
 * /Cider/EBU/Bobcat2/Bobcat2 {Current}/Switching Core/GOP/<XLG MAC IP> XLG MAC IP Units%p/Port MAC Control Register5
 * 
 * 
 *  0 -  3  TxIPGLength          minimal vaue is 8 for 10G and 40G
 *  4 -  6  PreambleLengthTx     0 -- 8 bytes 
 *                               1..7 -- 1..7-bytes
 *                               for 10G 4,8 are only allowed 
 *  7 -  9  PreambleLengthRx     0 -- 8 bytes 
 *                               1..7 -- 1..7-bytes
 * 10 - 12  TxNumCrcBytes        legal value 1,2,3,4
 * 13 - 15  RxNumCrcBytes        legal value 1,2,3,4
 *---------------------------------------------------------------------------------------------
 */

/*******************************************************************************
* prvCpssDxChBobcat2PortMacGigaIsSupported
*
* DESCRIPTION:
*       check whether GE mac is supported for specific mac
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*       portNum  - physical port number
*
* OUTPUTS:
*       isSupportedPtr - pointer to is supported
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - on bad ptr
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBobcat2PortMacGigaIsSupported
(
    IN  GT_U8                    devNum,
    IN  GT_PHYSICAL_PORT_NUM     mac,
    OUT GT_BOOL                 *isSupportedPtr
)
{
    GT_U32    regAddr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    PRV_CPSS_DXCH_PORT_MAC_CTRL3_REG_MAC(devNum, mac, PRV_CPSS_PORT_GE_E, &regAddr);

    if(regAddr == PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        *isSupportedPtr = GT_FALSE;
    }
    else
    {
        *isSupportedPtr = GT_TRUE;
    }
    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChBobcat2PortGigaMacIPGLengthSet
*
* DESCRIPTION:
*       GE mac set IPG length
*
* APPLICABLE DEVICES:
*        xCat3; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*       portNum      - physical port number
*       length   - ipg length
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBobcat2PortGigaMacIPGLengthSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN GT_U32                   length
)
{
    GT_STATUS rc;
    GT_U32    regAddr;
    GT_PHYSICAL_PORT_NUM    portMacNum;
    PRV_CPSS_DXCH_PORT_REG_CONFIG_STC   regDataArray[PRV_CPSS_PORT_NOT_APPLICABLE_E];

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    if (length > GIGA_MAC_CTRL3_IPG_MAX_LEN_D)
    {
        return GT_OUT_OF_RANGE;
    }

    rc = prvCpssDxChMacByPhysPortGet(devNum, portNum, &portMacNum);
    if (rc != GT_OK)
    {
        return rc;
    }
    PRV_CPSS_DXCH_PORT_MAC_CTRL3_REG_MAC(devNum, portMacNum, PRV_CPSS_PORT_GE_E, &regAddr);

    if(regAddr == PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        return GT_BAD_PARAM;
    }

    if(prvCpssDxChPortMacConfigurationClear(regDataArray) != GT_OK)
        return GT_INIT_ERROR;

    regDataArray[PRV_CPSS_PORT_GE_E].regAddr = regAddr;
    regDataArray[PRV_CPSS_PORT_GE_E].fieldData = length;
    regDataArray[PRV_CPSS_PORT_GE_E].fieldLength = GIGA_MAC_CTRL3_IPG_LEN_FLD_LEN_D;
    regDataArray[PRV_CPSS_PORT_GE_E].fieldOffset = GIGA_MAC_CTRL3_IPG_LEN_FLD_OFFS_D;
    
    rc = prvCpssDxChPortMacConfiguration(devNum, portNum, regDataArray);

    return rc;
}

/*******************************************************************************
* prvCpssDxChBobcat2PortGigaMacIPGLengthGet
*
* DESCRIPTION:
*       GE mac set IPG length
*
* APPLICABLE DEVICES:
*        xCat3; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*       mac      - physical port number
*
* OUTPUTS:
*       lengthPtr   - pointer to ipg length 
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - on NULL pointer
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBobcat2PortGigaMacIPGLengthGet
(
    IN  GT_U8                    devNum,
    IN  GT_PHYSICAL_PORT_NUM     mac,
    OUT GT_U32                  *lengthPtr
)
{
    GT_STATUS rc;
    GT_U32    regAddr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);
    CPSS_NULL_PTR_CHECK_MAC(lengthPtr);

    *lengthPtr = 0;

    PRV_CPSS_DXCH_PORT_MAC_CTRL3_REG_MAC(devNum, mac, PRV_CPSS_PORT_GE_E, &regAddr);

    if(regAddr == PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        return GT_BAD_PARAM;
    }
    rc = prvCpssHwPpPortGroupGetRegField(devNum, 0, regAddr
                                            ,GIGA_MAC_CTRL3_IPG_LEN_FLD_OFFS_D  
                                            ,GIGA_MAC_CTRL3_IPG_LEN_FLD_LEN_D  
                                            ,lengthPtr);
    if (rc != GT_OK)
    {
        *lengthPtr = 0;
        return rc;
    }
    return GT_OK;
}


/*******************************************************************************
* prvCpssDxChBobcat2PortGigaMacPreambleLengthSet
*
* DESCRIPTION:
*       GE mac set preamble length
*
* APPLICABLE DEVICES:
*        xCat3; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*       portNum      - physical port number
*       direction - RX/TX/BOTH
*       length   - preamble length
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBobcat2PortGigaMacPreambleLengthSet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    IN CPSS_PORT_DIRECTION_ENT  direction,
    IN GT_U32                   length
)
{
    GT_STATUS rc;
    GT_U32    regAddr;
    GT_U32    fldVal;
    GT_PHYSICAL_PORT_NUM    portMacNum;
    PRV_CPSS_DXCH_PORT_REG_CONFIG_STC   regDataArray[PRV_CPSS_PORT_NOT_APPLICABLE_E];

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    if (length != GIGA_MAC_CTRL1_PREAMBLE_LEN_4_D && length != GIGA_MAC_CTRL1_PREAMBLE_LEN_8_D)
    {
        return GT_BAD_PARAM;
    }
    if (direction < CPSS_PORT_DIRECTION_RX_E || direction > CPSS_PORT_DIRECTION_BOTH_E)
    {
        return GT_BAD_PARAM;
    }
    if (direction != CPSS_PORT_DIRECTION_TX_E)
    {
        return GT_BAD_PARAM;
    }

    rc = prvCpssDxChMacByPhysPortGet(devNum, portNum, &portMacNum);
    if (rc != GT_OK)
    {
        return rc;
    }

    PRV_CPSS_DXCH_PORT_MAC_CTRL1_REG_MAC(devNum, portMacNum, PRV_CPSS_PORT_GE_E, &regAddr);

    if(regAddr == PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        return GT_BAD_PARAM;
    }
    if (length == GIGA_MAC_CTRL1_PREAMBLE_LEN_4_D)
    {
        fldVal = 1;
    }
    else
    {
        fldVal = 0;
    }

    if(prvCpssDxChPortMacConfigurationClear(regDataArray) != GT_OK)
        return GT_INIT_ERROR;

    regDataArray[PRV_CPSS_PORT_GE_E].regAddr = regAddr;
    regDataArray[PRV_CPSS_PORT_GE_E].fieldData = fldVal;
    regDataArray[PRV_CPSS_PORT_GE_E].fieldLength = GIGA_MAC_CTRL1_PREAMBLE_LEN_FLD_OFFS_D;
    regDataArray[PRV_CPSS_PORT_GE_E].fieldOffset = GIGA_MAC_CTRL1_PREAMBLE_LEN_FLD_OFFS_D;
    
    rc = prvCpssDxChPortMacConfiguration(devNum, portNum, regDataArray);

    return rc;
}

/*******************************************************************************
* prvCpssDxChBobcat2PortGigaMacPreambleLengthGet
*
* DESCRIPTION:
*       GE mac set preamble length
*
* APPLICABLE DEVICES:
*        xCat3; Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; Lion; Lion2; xCat2.
*
* INPUTS:
*       devNum   - device number
*       mac      - physical port number
*       direction - RX/TX/BOTH
*
* OUTPUTS:
*       lengthPtr  - pointer to preamble length
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - on wrong devNum, portNum
*       GT_BAD_PTR      - on NULL ptr
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBobcat2PortGigaMacPreambleLengthGet
(
    IN  GT_U8                    devNum,
    IN  GT_PHYSICAL_PORT_NUM     mac,
    IN  CPSS_PORT_DIRECTION_ENT  direction,
    OUT GT_U32                   *lengthPtr
)
{
    GT_STATUS rc;
    GT_U32    regAddr;
    GT_U32    fldVal;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);
    CPSS_NULL_PTR_CHECK_MAC(lengthPtr);

    if (direction != CPSS_PORT_DIRECTION_TX_E)
    {
        return GT_BAD_PARAM;
    }

    PRV_CPSS_DXCH_PORT_MAC_CTRL1_REG_MAC(devNum, mac, PRV_CPSS_PORT_GE_E, &regAddr);

    if(regAddr == PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        return GT_BAD_PARAM;
    }
    rc = prvCpssHwPpPortGroupGetRegField(devNum, 0, regAddr
                                            ,GIGA_MAC_CTRL1_PREAMBLE_LEN_FLD_OFFS_D  
                                            ,GIGA_MAC_CTRL1_PREAMBLE_LEN_FLD_LEN_D  
                                            ,/*OUT*/&fldVal);
    if (rc != GT_OK)
    {
        return rc;
    }
    if (fldVal == 1)
    {
        *lengthPtr = GIGA_MAC_CTRL1_PREAMBLE_LEN_4_D;
    }
    else
    {
        *lengthPtr = GIGA_MAC_CTRL1_PREAMBLE_LEN_8_D;
    }
    return GT_OK;
}



