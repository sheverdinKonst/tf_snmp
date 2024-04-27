/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
********************************************************************************
* appDemoDxChDmaUtils.h
*
* DESCRIPTION:
*       App demo DxCh appdemo DMA API header.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __appDemoDxChDmaUtilsh
#define __appDemoDxChDmaUtilsh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
* appDemoDxChMacCounterGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Gets Ethernet MAC counter /  MAC Captured counter for a
*       specified port on specified device.
*
* INPUTS:
*       devNum         - physical device number
*       portNum        - physical port number,
*                        CPU port if getFromCapture is GT_FALSE
*       cntrName       - specific counter name
*       getFromCapture -  GT_TRUE -  Gets the captured Ethernet MAC counter
*                         GT_FALSE - Gets the Ethernet MAC counter
*
* OUTPUTS:
*       cntrValuePtr - (pointer to) current counter value.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*       GT_BAD_PARAM - on wrong port number, device or counter name
*       GT_HW_ERROR  - on hardware error
*       GT_BAD_PTR   - one of the parameters is NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*     The 10G MAC MIB counters are 64-bit wide.
*     Not supported counters: CPSS_BAD_PKTS_RCV_E, CPSS_UNRECOG_MAC_CNTR_RCV_E,
*     CPSS_BadFC_RCV_E, CPSS_GOOD_PKTS_RCV_E, CPSS_GOOD_PKTS_SENT_E.
*
*     This function is written for pexOnly mode --> these is no register initialization
*     All register addresses are hard coded
*******************************************************************************/
GT_STATUS appDemoDxChMacCounterGet_HardCodedRegAddr
(
    IN  GT_U8                       devNum,
    IN  GT_PHYSICAL_PORT_NUM        portNum,
    IN  CPSS_PORT_MAC_COUNTERS_ENT  cntrName,
    OUT GT_U64                      *cntrValuePtr
);

/*******************************************************************************
* appDemoDxChCfgIngressDropCntrGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Get the Ingress Drop Counter value.
*
* INPUTS:
*       devNum      - Device number.
*
* OUTPUTS:
*       counterPtr  - (pointer to) Ingress Drop Counter value
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_HW_ERROR              - on hardware error.
*       GT_BAD_PARAM             - on wrong devNum.
*       GT_BAD_PTR               - on NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
GT_STATUS appDemoDxChCfgIngressDropCntrGet_HardCodedRegAddr
(
    IN  GT_U8       devNum,
    OUT GT_U32      *counterPtr
);

/*******************************************************************************
* appDemoDxChMirrorRxPortSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Sets a specific port to be Rx mirrored port.
*
* INPUTS:
*       devNum         - the device number
*       mirrPort       - port number, CPU port supported.
*       enable         - enable/disable Rx mirror on this port
*                        GT_TRUE - Rx mirroring enabled, packets
*                                  received on a mirrPort are
*                                  mirrored to Rx analyzer.
*                        GT_FALSE - Rx mirroring disabled.
*       index          - Analyzer destination interface index. (APPLICABLE RANGES: 0..6)
*                        Supported for xCat and above device.
*                        Used only if forwarding mode to analyzer is Source-based.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device, mirrPort.
*       GT_OUT_OF_RANGE          - index is out of range.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
GT_STATUS appDemoDxChMirrorRxPortSet_HardCodedRegAddr
(
    IN  GT_U8           devNum,
    IN  GT_PORT_NUM     mirrPort,
    IN  GT_BOOL         enable,
    IN  GT_U32          index
);

/*******************************************************************************
* setOwnDevice_HardCodedRegAddr
*
* DESCRIPTION:
*       Sets the device Device_ID within a Prestera chipset.
*
* INPUTS:
*       devNum         - the device number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device, mirrPort.
*       GT_OUT_OF_RANGE          - index is out of range.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*******************************************************************************/
GT_STATUS setOwnDevice_HardCodedRegAddr
(
    IN  GT_U8           devNum
);

/*******************************************************************************
* appDemoDxChMirrorAnalyzerInterfaceSet_HardCodedRegAddr
*
* DESCRIPTION:
*       This function sets analyzer interface.
*
* INPUTS:
*      devNum         - device number.
*      index          - index of analyzer interface. (APPLICABLE RANGES: 0..6)
*      interfacePtr   - Pointer to analyzer interface.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device, index, interface type.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_BAD_PTR               - on NULL pointer
*       GT_OUT_OF_RANGE          - on wrong port or device number in interfacePtr.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChMirrorAnalyzerInterfaceSet_HardCodedRegAddr
(
    IN GT_U8     devNum,
    IN GT_U32    index,
    IN CPSS_DXCH_MIRROR_ANALYZER_INTERFACE_STC   *interfacePtr
);

/*******************************************************************************
* appDemoDxChMirrorRxGlobalAnalyzerInterfaceIndexSet_HardCodedRegAddr
*
* DESCRIPTION:
*       This function sets analyzer interface index, used for ingress
*       mirroring from all engines except
*       port-mirroring source-based-forwarding mode.
*       (Port-Based hop-by-hop mode, Policy-Based, VLAN-Based,
*        FDB-Based, Router-Based).
*       If a packet is mirrored by both the port-based ingress mirroring,
*       and one of the other ingress mirroring, the selected analyzer is
*       the one with the higher index.
*
* INPUTS:
*      devNum    - device number.
*      enable    - global enable/disable mirroring for
*                  Port-Based hop-by-hop mode, Policy-Based,
*                  VLAN-Based, FDB-Based, Router-Based.
*                  - GT_TRUE - enable mirroring.
*                  - GT_FALSE - No mirroring.
*      index     - Analyzer destination interface index. (APPLICABLE RANGES: 0..6)
*
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - wrong device.
*       GT_OUT_OF_RANGE          - index is out of range.
*       GT_HW_ERROR              - on writing to HW error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChMirrorRxGlobalAnalyzerInterfaceIndexSet_HardCodedRegAddr
(
    IN GT_U8     devNum,
    IN GT_BOOL   enable,
    IN GT_U32    index
);

/*******************************************************************************
* appDemoDxChIngressCscdPortSet_HardCodedRegAddr
*
* DESCRIPTION:
*           Set ingress port as cascaded/non-cascade .
*           (effect all traffic on ingress pipe only)
*
* INPUTS:
*       devNum   - physical device number
*       portNum  - The physical port number.
*                   NOTE: this port is not covered and is NOT checked for 'mapping'
*       portType - cascade  type regular/extended DSA tag port or network port
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChIngressCscdPortSet_HardCodedRegAddr
(
    IN GT_U8                        devNum,
    IN GT_PHYSICAL_PORT_NUM         portNum,
    IN CPSS_CSCD_PORT_TYPE_ENT      portType
);

/*******************************************************************************
* appDemoDxChPortEgressCntrsGet_HardCodedRegAddr
*
* DESCRIPTION:
*       Gets a egress counters from specific counter-set.
*
* INPUTS:
*       devNum        - physical device number
*       cntrSetNum - counter set number : 0, 1
*
* OUTPUTS:
*       egrCntrPtr - (pointer to) structure of egress counters current values.
*
* RETURNS:
*       GT_OK                    - on success
*       GT_FAIL                  - on error
*       GT_BAD_PARAM             - on wrong port number or device
*       GT_HW_ERROR              - on hardware error
*       GT_BAD_PTR               - one of the parameters is NULL pointer
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChPortEgressCntrsGet_HardCodedRegAddr
(
    IN  GT_U8                       devNum,
    IN  GT_U8                       cntrSetNum,
    OUT CPSS_PORT_EGRESS_CNTR_STC   *egrCntrPtr
);

/*******************************************************************************
* appDemoDxChSdmaTxPacketSend_HardCodedRegAddr
*
* DESCRIPTION:
*       This function sends a single packet.
*       The packet is sent through interface port type to port given by
*       dstPortNum with VLAN vid.
*
* INPUTS:
*       isPacketWithVlanTag - indication that the packetDataPtr[12..15] hold vlan tag.
*       vid                 - VLAN ID. (used when isPacketWithVlanTag ==GT_FALSE,
*                             otherwise taken from packetDataPtr[14..15])
*       packetPtr           - pointer to the packet data and length in bytes.
*       dstPortNum          - Destination port number.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChSdmaTxPacketSend_HardCodedRegAddr
(
    IN GT_U8    devNum,
    IN GT_BOOL  isPacketWithVlanTag,
    IN GT_U8    vid,
    IN TGF_PACKET_PAYLOAD_STC  *packetPtr,
    IN GT_U32   dstPortNum
);

/*******************************************************************************
* appDemoDxChPortForceLinkPassEnableSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Enable/disable Force Link Pass on specified port on specified device.
*
* INPUTS:
*       devNum   - physical device number
*       portNum  - physical port number (or CPU port)
*       state    - GT_TRUE for force link pass, GT_FALSE otherwise
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
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChPortForceLinkPassEnableSet_HardCodedRegAddr
(
    IN  GT_U8     devNum,
    IN  GT_PHYSICAL_PORT_NUM     portNum,
    IN  GT_BOOL   state
);

/*******************************************************************************
* appDemoDxChPortLoopbackModeEnableSet_HardCodedRegAddr
*
* DESCRIPTION:
*       Set port in 'loopback' mode
*
* INPUTS:
*       portInterfacePtr - (pointer to) port interface
*       enable           - enable / disable (loopback/no loopback)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK        - on success
*       GT_FAIL      - on error
*       GT_HW_ERROR  - on hardware error
*       GT_BAD_PARAM - wrong interface
*       GT_BAD_PTR   - on NULL pointer
*
* COMMENTS:
*   This function is written for pexOnly mode --> these is no register initialization
*   All register addresses are hard coded
*
*******************************************************************************/
GT_STATUS appDemoDxChPortLoopbackModeEnableSet_HardCodedRegAddr
(
    IN CPSS_INTERFACE_INFO_STC  *portInterfacePtr,
    IN GT_BOOL                   enable
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __appDemoDxChDmaUtilsh */

