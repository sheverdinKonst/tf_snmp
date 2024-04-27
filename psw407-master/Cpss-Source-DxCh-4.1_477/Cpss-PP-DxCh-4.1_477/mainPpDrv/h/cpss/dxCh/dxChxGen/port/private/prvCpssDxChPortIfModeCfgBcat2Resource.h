/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortIfModeCfgBcat2Resource.h
*
* DESCRIPTION:
*       CPSS BC2 implementation for Port interface mode resource configuration.
*
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#ifndef __PRV_CPSS_DXCH_PORT_IF_MODE_CFG_BCAT2_RESOURCE_H
#define __PRV_CPSS_DXCH_PORT_IF_MODE_CFG_BCAT2_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>

/*--------------------------------------------------------------------------------------------------------------------------------
// Packet travel guide : 
//
//  Regular port x with mapping (mac-x,rxdma-x, txdma-x, txq-x,txfifo-x) 
//        packet --> port-rx --> RxDMA -----------------> TxQ -->---------------- TxDMA ----------> TXFIIFO --> port-tx
//                    mac-x     rxdma-x                  txq-x                    txdma-x           txfifo-x     mac-x
//
//                           RXDMA_IF_WIDTH                ?                    TxQ Credits       TXFIFO IF width
//                                                                           TxDMA speed(A0)      Shifter Threshold (A0)
//                                                                            Rate Limit(A0)      Payload StartTrasmThreshold 
//                                                                           Burst Full Limit(B0)      
//                                                                           Burst Amost Full Limit(B0)  
//                                                                           Payload Credits
//                                                                           Headers Credits
//
//  port with TM (mac-x,rxdma-x,TxDMA = TM,TxQ = TM , TxFIFO = TM, Eth-TxFIFO == eth-txfifo-x) 
//
//        packet --> port-rx --> RxDMA ------------------> TxQ ---------> TxDMA ---------------> TxFIFO -------------------> ETH-THFIFO -----------> port-tx
//                    mac-x      rxdma-x                  64(TM)          73(TM)                 73 (TM)                     eth-txfifo-x             mac-x
//                                                                                                                           
//                            RXDMA_IF_WIDTH                            TxQ Credits             TXFIFO IF width               Eth-TXFIFO IF width
//                                                                      TxDMA speed(A0)         Shifter Threshold(A0)         Shifter Threshold (A0)
//                                                                      Rate Limit(A0)          Payload StartTrasmThreshold   Payload StartTrasmThreshold
//                                                                      Burst Full Limit(B0)      
//                                                                      Burst Amost Full Limit(B0)  
//                                                                      Payload Credits
//                                                                      Headers Credits
//
//----------------------------------------------------------------------------------------------------------------------------------
*/

GT_STATUS prvCpssDxChPortBcat2CreditsCheckSet
(
    IN GT_BOOL val
);


#define PRV_CPSS_DXCH_INVALID_RESOURCE_CNS  (GT_U16)(~0)


typedef enum CPSS_DXCH_BC2_PORT_RESOURCES_FLD_ENT
{
     BC2_PORT_FLD_RXDMA_IfWidth  = 0                                   /* rev(A0, B0               ), speed             */
    ,BC2_PORT_FLD_TXDMA_SCDMA_TxQDescriptorCredit                      /* rev(A0, B0               ), speed, core clock */
    ,BC2_PORT_FLD_TXDMA_SCDMA_speed                                    /* rev(A0                   ), speed             */
    ,BC2_PORT_FLD_TXDMA_SCDMA_rateLimitThreshold                       /* rev(A0                   ), speed, core clock */
    ,BC2_PORT_FLD_TXDMA_SCDMA_burstAlmostFullThreshold                 /* rev(    B0               ), speed, core clock */
    ,BC2_PORT_FLD_TXDMA_SCDMA_burstFullThreshold                       /* rev(    B0               ), speed, core clock */
    ,BC2_PORT_FLD_TXDMA_SCDMA_TxFIFOHeaderCreditThreshold              /* rev(A0, B0               ), speed, core clock */ 
    ,BC2_PORT_FLD_TXDMA_SCDMA_TxFIFOPayloadCreditThreshold             /* rev(A0, B0               ), speed, core clock */ 
    ,BC2_PORT_FLD_TXFIFO_IfWidth                                       /* rev(A0, B0               ), speed             */      
    ,BC2_PORT_FLD_TXFIFO_shifterThreshold                              /* rev(A0, B0-ReadOnly      ), speed, core clock */
    ,BC2_PORT_FLD_TXFIFO_payloadThreshold                              /* rev(A0, B0               ), speed, core clock */
    ,BC2_PORT_FLD_Eth_TXFIFO_IfWidth                                   /* rev(A0, B0               ), speed */
    ,BC2_PORT_FLD_Eth_TXFIFO_shifterThreshold                          /* rev(A0,                  ), speed , core clock */
    ,BC2_PORT_FLD_Eth_TXFIFO_payloadThreshold                          /* rev(A0, B0               ), speed , core clock */
    ,BC2_PORT_FLD_FCA_BUS_WIDTH
    ,BC2_PORT_FLD_MAX
    ,BC2_PORT_FLD_INVALID_E = ~0
}CPSS_DXCH_BC2_PORT_RESOURCES_FLD_ENT;


typedef struct 
{
    CPSS_DXCH_BC2_PORT_RESOURCES_FLD_ENT fldId;
    GT_U32                         fldArrIdx;
    GT_U32                         fldVal;
}prvCpssDxChBcat2PortResourse_IdVal_STC;


typedef struct 
{
    GT_U32 fldN;
    prvCpssDxChBcat2PortResourse_IdVal_STC arr[BC2_PORT_FLD_MAX];
}CPSS_DXCH_BCAT2_PORT_RESOURCE_LIST_STC;


typedef struct
{
    GT_U32  rxdmaScdmaIncomingBusWidth;
    GT_U32  txdmaTxQCreditValue;
    GT_U32  txdmaBurstAlmostFullThreshold;
    GT_U32  txdmaBBurstFullThreshold;
    GT_U32  txdmaTxfifoHeaderCounterThresholdScdma;
    GT_U32  txdmaTxfifoPayloadCounterThresholdScdma;
    GT_U32  txfifoScdmaPayloadThreshold;
    GT_U32  txfifoScdmaShiftersOutgoingBusWidth;
    GT_U32  ethTxfifoOutgoingBusWidth;
    GT_U32  ethTxfifoScdmaPayloadThreshold;
}PRV_CPSS_DXCH_BC2_PORT_RESOURCE_STC;



typedef enum 
{
    PRV_CPSS_RXDMA_IfWidth_64_E = 0,
    PRV_CPSS_RXDMA_IfWidth_256_E = 2,
    PRV_CPSS_RXDMA_IfWidth_512_E = 3,
    PRV_CPSS_RXDMA_IfWidth_MAX_E
}PRV_CPSS_RXDMA_IfWidth_ENT;


typedef enum 
{
    PRV_CPSS_TxFIFO_OutGoungBusWidth_1B_E = 0,
    PRV_CPSS_TxFIFO_OutGoungBusWidth_8B_E = 3,
    PRV_CPSS_TxFIFO_OutGoungBusWidth_32B_E = 5,
    PRV_CPSS_TxFIFO_OutGoungBusWidth_64B_E = 6,
    PRV_CPSS_TxFIFO_OutGoungBusWidth_MAX_E
}PRV_CPSS_TxFIFO_OutGoungBusWidth_ENT;

typedef enum 
{
    PRV_CPSS_EthTxFIFO_OutGoungBusWidth_1B_E = 0,
    PRV_CPSS_EthTxFIFO_OutGoungBusWidth_8B_E = 3,
    PRV_CPSS_EthTxFIFO_OutGoungBusWidth_32B_E = 5,
    PRV_CPSS_EthTxFIFO_OutGoungBusWidth_64B_E = 6,
    PRV_CPSS_EthTxFIFO_OutGoungBusWidth_MAX_E
}PRV_CPSS_EthTxFIFO_OutGoungBusWidth_ENT;


/*******************************************************************************
* prvCpssDxChBcat2PortResourcesInit
*
* DESCRIPTION:
*       Initialize data structure for port resource allocation
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum      - physical device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_BAD_PARAM      - on wrong port number or device
*       GT_HW_ERROR       - on hardware error
*       GT_NOT_SUPPORTED  - on not supported interface for given port
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBcat2PortResourcesInit
(
    IN    GT_U8                   devNum
);


/*******************************************************************************
* prvCpssDxChBcat2PortResourcesConfig
*
* DESCRIPTION:
*       Allocate/free resources of port per it's current status/interface.
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - physical port number
*       ifMode      - interface mode
*       speed       - port data speed
*       allocate    - allocate/free resources:
*                       GT_TRUE - up;
*                       GT_FALSE - down;
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_BAD_PARAM      - on wrong port number or device
*       GT_HW_ERROR       - on hardware error
*       GT_NOT_SUPPORTED  - on not supported interface for given port
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChBcat2PortResourcesConfig
(
    IN  GT_U8                           devNum,
    IN  GT_PHYSICAL_PORT_NUM            portNum,
    IN  CPSS_PORT_INTERFACE_MODE_ENT    ifMode,
    IN  CPSS_PORT_SPEED_ENT             speed,
    IN  GT_BOOL                         allocate
);

/*******************************************************************************
* prvCpssDxChBcat2PortResoursesStateGet
*
* DESCRIPTION:
*       read port resources 
*
* APPLICABLE DEVICES:
*        Bobcat2; Caelum; Bobcat3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum      - physical device number
*       portNum     - physical port number
*
* OUTPUTS:
*       resPtr      - pointer to list of resources
*
* RETURNS:
*       GT_OK             - on success
*       GT_BAD_PARAM      - on wrong port number or device
*       GT_BAD_PTR        - on bad ptr
*       GT_HW_ERROR       - on hardware error
*       GT_NOT_SUPPORTED  - on not supported revision
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       just revion B0 and above are supported
*******************************************************************************/
GT_STATUS prvCpssDxChBcat2PortResoursesStateGet
(
    IN  GT_U8                                devNum,
    IN  GT_PHYSICAL_PORT_NUM                 portNum,
   OUT  CPSS_DXCH_BCAT2_PORT_RESOURCE_LIST_STC  *resPtr
);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
