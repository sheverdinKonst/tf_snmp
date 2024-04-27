#ifndef __PRV_CPSS_DXCH_PORT_TXQ_HIGH_SPEED_PORTS_DRV_H
#define __PRV_CPSS_DXCH_PORT_TXQ_HIGH_SPEED_PORTS_DRV_H

#include <cpss/generic/cpssTypes.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS        8

/*
#define PRV_CPSS_DXCH_PORT_BC2_A0_HIGH_SPEED_PORT_NUM_CNS 2
#define PRV_CPSS_DXCH_PORT_BC2_B0_HIGH_SPEED_PORT_NUM_CNS 8
*/

/*******************************************************************************
* prvCpssDxChPortDynamicPATxQHighSpeedPortInit
*
* DESCRIPTION:
*       TxQ High Speed ports Init
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2; 
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_FAIL         - on error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPATxQHighSpeedPortInit
(
    IN  GT_U8  devNum    
);

/*******************************************************************************
* prvCpssDxChPortDynamicPATxQHighSpeedPortSet
*
* DESCRIPTION:
*       TxQ assign high speed ports to TxQ ports 
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2; 
*
* INPUTS:
*       devNum   - device number
*       numberOfHighSpeedPorts - number of TxQ ports to configure as High Speed 
*       highSpeedPortsPortArr  - array of ports
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - bad pointer
*       GT_FAIL         - on error
*
* COMMENTS:
*       BOBCAT2 B0: only txq port 64 can be declared as High Speed Port
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPATxQHighSpeedPortSet
(
    IN  GT_U8  devNum,
    IN  GT_U32  highSpeedPortNumber,
    IN  GT_U32  highSpeedPortArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);

/*******************************************************************************
* prvCpssDxChPortDynamicPATxQHighSpeedPortGet
*
* DESCRIPTION:
*       get list of txQ ports that are declared as  high speed
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2;
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       numberOfHighSpeedPortsPtr - number of TxQ ports to configure as High Speed
*       portNumArr                - array of TxQ ports that are declared as high speed
*       highSpeedPortIdxArr       - array high speed port idx assigned to corresponded TxQ port 
*

*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - bad pointer
*       GT_FAIL         - on error
*
* COMMENTS:
*       if highSpeedPortIdxArr is NULL, port indexes are not filled
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPATxQHighSpeedPortGet
(
    IN  GT_U8   devNum,
    OUT GT_U32  *highSpeedPortNumberPtr,
    OUT GT_U32  portNumArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS],
    OUT GT_U32  highSpeedPortIdxArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);

/*******************************************************************************
* prvCpssDxChPortDynamicPATxQHighSpeedPortDumpGet
*
* DESCRIPTION:
*       get dump of LL and DQ unit 
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; Lion2; xCat2;
*
* INPUTS:
*       devNum   - device number
*
* OUTPUTS:
*       txqDqPortNumArr     - array of TxQ ports that are declared as high speed
*       txqLLPortNumArr     - array high speed port idx assigned to corresponded TxQ port 
*
*
* RETURNS:
*       GT_OK           - on success
*       GT_BAD_PARAM    - wrong devNum, portNum
*       GT_BAD_PTR      - bad pointer
*       GT_FAIL         - on error
*
* COMMENTS:
*       if high speed port is not assigned to any port , the corresponding entry is filled by ~0
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortDynamicPATxQHighSpeedPortDumpGet
(
    IN  GT_U8   devNum,
    OUT GT_U32  txqDqPortNumArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS],
    OUT GT_U32  txqLLPortNumArr[PRV_CPSS_DXCH_PORT_HIGH_SPEED_PORT_NUM_CNS]
);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
