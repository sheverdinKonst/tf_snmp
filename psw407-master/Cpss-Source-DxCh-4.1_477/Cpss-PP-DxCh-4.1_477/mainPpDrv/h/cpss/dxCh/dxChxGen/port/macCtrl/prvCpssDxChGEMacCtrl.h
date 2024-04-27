/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChGEMacCtrl.h
*
* DESCRIPTION:
*       bobcat2 GE mac control API
*
* FILE REVISION NUMBER:
*       $Revision: 3$
*******************************************************************************/

#ifndef PRV_CPSS_DXCH_GE_MAC_CTRL_H
#define PRV_CPSS_DXCH_GE_MAC_CTRL_H

#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/config/private/prvCpssConfigTypes.h>
#include <cpss/generic/port/cpssPortCtrl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChGEMacCtrl.h
*
* DESCRIPTION:
*       bobcat2 GE mac control 
*
* FILE REVISION NUMBER:
*       $Revision: 3$
*******************************************************************************/
#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/config/private/prvCpssConfigTypes.h>
#include <cpss/generic/port/cpssPortCtrl.h>


/*---------------------------------------------------------------------------------------------
 * /Cider/EBU/Bobcat2/Bobcat2 {Current}/Switching Core/GOP/<Gige MAC IP> Gige MAC IP Units%g/Tri-Speed Port MAC Configuration/Port MAC Control Register1
 * 15 - 15  Short preable    0x0 -- 8 bytes
 *                           0x1 -- 4 bytes
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
);

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
*       mac      - physical port number
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
    IN GT_PHYSICAL_PORT_NUM     mac,
    IN GT_U32                   length
);

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
);

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
);

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
);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
