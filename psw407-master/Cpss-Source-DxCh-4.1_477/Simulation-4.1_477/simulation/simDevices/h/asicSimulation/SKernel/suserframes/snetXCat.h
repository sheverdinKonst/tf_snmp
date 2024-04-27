/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetXCat.h
*
* DESCRIPTION:
*       This is a external API definition for snet xCat module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 17 $
*
*******************************************************************************/
#ifndef __snetXCath
#define __snetXCath


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*  Struct : SNET_XCAT_LOGICAL_PORT_MAPPING_STC

    Description:
            Logical Port Mapping Entry used for mapping a packets logical
            target to an actual egress interface.

    egressIf        - Actual egress interface.
    tunnelStart     - Indicates that the packet should be sent on a tunnel.
    tunnelPtr       - Tunnel pointer in case that the packet should be tunneled.
    tunnelPassengerType
                    - Type of passenger packet being to be encapsulated.

new fields for devObjPtr->supportLogicalMapTableInfo.tableFormatVersion = 1
    assignVid0Command :
        0 = Do not override VID0 assignment
        1 = Override VID0 assignment only if packet arrives without Tag0
        2 = Always override VID0 assignment
        3 = Reserved
    egressVlanFilteringEnable :
        If set, apply Egress VLAN filtering according to LP Egress VLAN member table configuration

    assignedEgressTagStateOnlyIfUnassigned:
        Needed to AC->AC traffic.  If packet arrived from PW, then it is already assigned

    egressTagState :
        same enum as VLAN table

    vid0:
        Relevant only if <Assign VID0 Enable> is set. The new VID0 assignment

*/
typedef struct {
    SNET_DST_INTERFACE_STC egressIf;
    GT_U32          tunnelStart;
    GT_U32          tunnelPtr;
    GT_U32          tunnelPassengerType;

    /* new fields for devObjPtr->supportLogicalMapTableInfo.tableFormatVersion = 1 */
    GT_U32  assignVid0Command;        /*1 bit*/
    GT_U32  egressVlanFilteringEnable;/*1 bit*/
    GT_U32  assignedEgressTagStateOnlyIfUnassigned;/*1 bit*/
    GT_U32  egressTagState;/*3 bits*/
    GT_U32  vid0;/*12 bits*/
    GT_U32  egressPassangerTagTpidIndex;/*2 bits*/

}SNET_XCAT_LOGICAL_PORT_MAPPING_STC;

/*******************************************************************************
*   snetXCatIngressVlanTagClassify
*
* DESCRIPTION:
*       Ingress Tag0 VLAN and Tag1 VLAN classification
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*       ethTypeOffset   - ethernet type offset
*       parseMode - parsing mode : from port/tti /trill..
*
* OUTPUT:
*       inVlanEtherType0Ptr - pointer to Tag0 VLAN EtherType
*       inVlanEtherType1Ptr - pointer to Tag1 VLAN EtherType
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetXCatIngressVlanTagClassify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 ethTypeOffset,
    OUT GT_U32 * inVlanEtherType0Ptr,
    OUT GT_U32 * inVlanEtherType1Ptr,
    IN SNET_CHT_FRAME_PARSE_MODE_ENT   parseMode
);

/*******************************************************************************
*   snetXCatHaEgressTagBuild
*
* DESCRIPTION:
*       HA - Build Tag0 VLAN and Tag1 VLAN according to Tag state
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       vlanTag0EtherType - VLAN Tag0 EtherType
*       vlanTag1EtherType - VLAN Tag1 EtherType
*       egrMarvellTag - type of marvell tag(none, standard, extended)
*       destVlanTagged  - destination port Tag state.
*
* OUTPUT:
*       tagDataPtr  - DSA/Vlan tag data.
*       tagDataLengthPtr - pointer to DSA/Vlan tag data length.
*       tag0OffsetInTagPtr - pointer to tag 0 offset from start of the buffer (tagDataPtr)
*       tag1OffsetInTagPtr - pointer to tag 1 offset from start of the buffer (tagDataPtr)
* RETURN:
*       GT_OK   - on success
*       GT_BAD_PTR - on NULL pointer
*       GT_BAD_PARAM - on wrong egress Vlan Tag state
*
*******************************************************************************/
GT_STATUS snetXCatHaEgressTagBuild
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  vlanTag0EtherType,
    IN GT_U32  vlanTag1EtherType,
    IN DSA_TAG_TYPE_E egrMarvellTag,
    IN GT_U32  destVlanTagged,
    OUT GT_U8 * tagDataPtr,
    INOUT GT_U32 * tagDataLengthPtr,
    OUT GT_U32  *tag0OffsetInTagPtr,
    OUT GT_U32  *tag1OffsetInTagPtr
);

/*******************************************************************************
*   snetXCatHaEgressTagEtherType
*
* DESCRIPTION:
*       HA - Egress Tag0 VLAN and Tag1 VLAN ethertype assignment
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr        - pointer to the frame's descriptor.
*       egressPort        - egress port (local port - not global)
*       egrTag0EtherTypePtr - pointer to egress Tag0 VLAN Ethertype
*       egrTag1EtherTypePtr - pointer to egress Tag1 VLAN Ethertype
*       tunnelStartEtherTypePtr - pointer to egress Tag of the tunnel start VLAN Ethertype
* RETURN:
*       GT_OK   - on success
*       GT_BAD_PTR - on NULL pointer
*
*******************************************************************************/
GT_STATUS snetXCatHaEgressTagEtherType
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    OUT GT_U32 * egrTag0EtherTypePtr,
    OUT GT_U32 * egrTag1EtherTypePtr,
    OUT GT_U32 * tunnelStartEtherTypePtr
);

/*******************************************************************************
*   snetXCatHaEgressTagEtherTypeByTpid
*
* DESCRIPTION:
*       HA - Egress get EtherType according to TPID
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr        - pointer to the frame's descriptor.
*       tpId         - TP-ID to select EtherType according to it
*       etherTypePtr - pointer to VLAN Ethertype
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* RETURN:
*       none
*
*******************************************************************************/
GT_VOID snetXCatHaEgressTagEtherTypeByTpid
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   tpId,
    INOUT GT_U32 * etherTypePtr,
    INOUT GT_U32 *tagSizePtr
);


/*******************************************************************************
*   snetXCatTxQPortIsolationFilters
*
* DESCRIPTION:
*        Port isolation filtering
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        destPorts       - number of egress port.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetXCatTxQPortIsolationFilters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT GT_U32 destPorts[]
);


/*******************************************************************************
*  snetXCatEqSniffFromRemoteDevice
*
* DESCRIPTION:
*        Forwarding TO_ANALYZER frames to the Rx/Tx Sniffer.
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       rxSniff     - RX/TX mirroring direction
*
* RETURN:
*
* COMMENTS :
*       Supports two modes of forwarding for TO_ANALYZER packets:
*           Hop-by-hop forwarding — in this mode each hop performs a forwarding
*                                   decision based on its local configuration.
*           Source-based forwarding — in this mode the device that triggers the
*                                   mirroring determines the analyzer device and port,
*                                   and as the packet is forwarded to other devices,
*                                   forwarding is based on the decision of the first device.
*                                   In this mode the TO_ANALYZER DSA tag includes the
*                                   analyzer device and port.
*
*******************************************************************************/
GT_VOID snetXCatEqSniffFromRemoteDevice
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL rxSniff
);

/*******************************************************************************
*  snetXCatFdbSrcIdAssign
*
* DESCRIPTION:
*       Source-ID Assignment
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       saLookupInfoPtr - pointer to SA lookup info structure
*       daLookupInfoPtr - pointer to DA lookup info structure
*
* RETURN:
*       sstId - FDB based SrcID (value of SMAIN_NOT_VALID_CNS means 'no assignment')
*
* COMMENTS :
*
*******************************************************************************/
GT_U32 snetXCatFdbSrcIdAssign
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr
);

/*******************************************************************************
*   snetXCatLogicalTargetMapping
*
* DESCRIPTION:
*       The device supports a generic mechanism that maps a packets
*       logical target to an actual egress interface
*
*       The logical target can me mapped to any of the following new targets:
*           - Single-target {Device, Port}
*           - Single-Target {Device, Port} + Tunnel-Start Pointer
*           - Trunk-ID
*           - Multi-target {VIDX}
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetXCatLogicalTargetMapping
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
);

/*******************************************************************************
*   snetXcatIngressMirrorAnalyzerIndexSelect
*
* DESCRIPTION:
*       The device supports multiple analyzers. If a packet is mirrored by
*       both the port-based ingress mirroring mechanism, and one of the other
*       ingress mirroring mechanisms, the selected analyzer
*       is the one with the higher index in the analyzer table
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        newAnalyzerIndex - new analyzer index
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetXcatIngressMirrorAnalyzerIndexSelect
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr ,
    IN GT_U32                   newAnalyzerIndex
);

/*******************************************************************************
*   snetXcatEgressMirrorAnalyzerIndexSelect
*
* DESCRIPTION:
*       The device supports multiple analyzers. support Egress selection
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        newAnalyzerIndex - new analyzer index
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetXcatEgressMirrorAnalyzerIndexSelect
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr ,
    IN GT_U32                   newAnalyzerIndex
);

/*******************************************************************************
*   snetXCatVlanTagMatchWithoutTag0Tag1Classification
*
* DESCRIPTION:
*      Check Ingress Global TPID table for packet ethernet type matching ,
*      without changing descrPtr->tpidIndex[tagIndex] !
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       etherType   - packet ethernet type
*       portTpIdBmp - port TPID bitmap
*       tagIndex    - tag index :
*                       0 - tag 0
*                       1 - tag 1
*                       2.. - extra tags
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* OUTPUTS:
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* RETURN:
*
*******************************************************************************/
GT_BOOL snetXCatVlanTagMatchWithoutTag0Tag1Classification
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr ,
    IN    GT_U32                           etherType,
    IN    GT_U32                           portTpIdBmp,
    IN    GT_U32                           tagIndex,
    INOUT GT_U32                           *tagSizePtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetXCath */


