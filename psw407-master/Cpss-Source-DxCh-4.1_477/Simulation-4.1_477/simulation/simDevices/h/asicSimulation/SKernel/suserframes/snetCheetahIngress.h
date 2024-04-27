/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahIngress.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 22 $
*
*******************************************************************************/
#ifndef __snetCheetahIngressh
#define __snetCheetahIngressh

#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3TTermination.h>
#include <asicSimulation/SKernel/suserframes/snet.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* NULL port */
#define SNET_CHT_NULL_PORT_CNS                          62

/* the types of the DMA clients */
typedef enum{
    SNET_CHT_DMA_CLIENT_PACKET_FROM_CPU_E,
    SNET_CHT_DMA_CLIENT_PACKET_TO_CPU_E,
    SNET_CHT_DMA_CLIENT_AUQ_E,
    SNET_CHT_DMA_CLIENT_FUQ_E,

    SNET_CHT_DMA_CLIENT___LAST___E

}SNET_CHT_DMA_CLIENT_ENT;


/*******************************************************************************
*   snetChtProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
*******************************************************************************/
GT_VOID snetChtProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

/*******************************************************************************
*
* DESCRIPTION:
*       ingress pipe processing after the TTI unit
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtIngressAfterTti
(
        IN SKERNEL_DEVICE_OBJECT * devObjPtr,
        IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*
* snetChtIngressAfterL3IpReplication:
*       ingress pipe processing after the IP MLL (L3 IP replication) unit
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtIngressAfterL3IpReplication
(
        IN SKERNEL_DEVICE_OBJECT * devObjPtr,
        IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);


/*******************************************************************************
*   snetCht3IngressMllSingleMllOutlifSet
*
* DESCRIPTION:
*        set the target outLif into descriptor - for single MLL
*        and Update mll counters
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        singleMllPtr - single mll pointer
* OUTPUTS:
*
*
*******************************************************************************/
void snetCht3IngressMllSingleMllOutlifSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN   SNET_CHT3_SINGLE_MLL_STC * singleMllPtr
);


/*******************************************************************************
*   snetChtPktCmdResolution
*
* DESCRIPTION:
*       resolve from old and current commands the new command
*
* INPUTS:
*       prevCmd --- previous command
*       currCmd — current command
*
* OUTPUTS:
*       none
*
* RETURN:
*       resolved new command
*
* COMMENTS:
*
*        [2] Table 5: cpu code changes conflict resolution  — page 17
*
*******************************************************************************/
extern GT_U32 snetChtPktCmdResolution
(
    IN SKERNEL_EXT_PACKET_CMD_ENT prevCmd,
    IN SKERNEL_EXT_PACKET_CMD_ENT currCmd
);

/*******************************************************************************
*   snetChtIngressCommandAndCpuCodeResolution
*
* DESCRIPTION:
*       1. resolve from old and current commands the new packet command
*       2. resolve from old and new commands the new packet cpu code (relevant
*          to copy that goes to CPU)
*
* INPUTS:
*       devObjPtr       - (pointer to) device object.
*       descrPtr        - (pointer to) frame descriptor
*       prevCmd         - previous command
*       currCmd         — current command
*       prevCpuCode     - previous cpu code
*       currCpuCode     — current cpu code
*       engineUnit      - the engine unit that need resolution with previous engine
*                         (isFirst = GT_TRUE) or need resolution with previous
*                         hit inside the same engine (isFirst = GT_FALSE)
*       isFirst - indication that the resolution is within the same engine or
*                         with previous engine
* OUTPUTS:
*       descrPtr        - (pointer to) frame descriptor
*
* RETURN:
*       none
*
* COMMENTS:
*
*
*
*******************************************************************************/
extern void snetChtIngressCommandAndCpuCodeResolution
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT prevCmd,
    IN SKERNEL_EXT_PACKET_CMD_ENT currCmd,
    IN GT_U32                     prevCpuCode,
    IN GT_U32                     currCpuCode,
    IN SNET_CHEETAH_ENGINE_UNIT_ENT engineUnit,
    IN GT_BOOL                    isFirst
);

/*******************************************************************************
*   snetChtL2Parsing
*
* DESCRIPTION:
*        L2 header Parsing (vlan tag , ethertype , nested vlan , encapsulation)
*        coming from port interface or comming from tunnel termination interface
*        for Ethernet over MPLS.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*       parseMode - parsing mode : from port/tti /trill..
*       internalTtiInfoPtr -  pointer to internal TTI info
* RETURN:
*
*******************************************************************************/
extern GT_VOID snetChtL2Parsing
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_FRAME_PARSE_MODE_ENT   parseMode,
    IN INTERNAL_TTI_DESC_INFO_STC        * internalTtiInfoPtr
);

/*******************************************************************************
*   snetChtL3L4ProtParsing
*
* DESCRIPTION:
*        Parsing of L3 and L4 protocols header
*
* INPUTS:
*        deviceObj  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        etherType  - packet ethernet type
*        internalTtiInfoPtr - internal tti info
* OUTPUTS:
*        descrPtr   - pointer to the frame's descriptor.
*
*******************************************************************************/
extern GT_VOID snetChtL3L4ProtParsing
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN GT_U32                           etherType,
    IN INTERNAL_TTI_DESC_INFO_STC      *internalTtiInfoPtr
);

/*******************************************************************************
*   snetChtL3L4ProtParsingResetDesc
*
* DESCRIPTION:
*       reset the fields that involved in Parsing of L3 and L4 protocols header
*       this is needed since the TTI need to reparse the L3,L4 when it terminates
*       the tunnel.
*
* INPUTS:
*        deviceObj  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        etherType  - packet ethernet type
*
*******************************************************************************/
extern GT_VOID snetChtL3L4ProtParsingResetDesc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetChtL3L4ProtCopyDesc
*
* DESCRIPTION:
*       copy ONLY L3/L4 relevant fields from source to target descriptor
*
* INPUTS:
*        trgDescrPtr   - pointer to the target frame's descriptor.
*        srcDescrPtr   - pointer to the source frame's descriptor.
*
*******************************************************************************/
extern GT_VOID snetChtL3L4ProtCopyDesc
(
    OUT SKERNEL_FRAME_CHEETAH_DESCR_STC * trgDescrPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC * srcDescrPtr
);

/*******************************************************************************
*   snetChtParsingTrillInnerFrame
*
* DESCRIPTION:
*        L2,3,QOS Parsing for 'Inner frame' of TRILL. need to be done regardless
*        to tunnel termination.
*        the function will save the 'parsing descriptor' in descrPtr->trillInfo.innerFrameDescrPtr
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the descriptor of the original frame (outer frame)
*       internalTtiInfoPtr -  pointer to internal TTI info
*
* RETURN:
*
*******************************************************************************/
extern GT_VOID snetChtParsingTrillInnerFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN INTERNAL_TTI_DESC_INFO_STC        * internalTtiInfoPtr
);

/*******************************************************************************
*   snetChtMplsTransitTunnelsProtParsing
*
* DESCRIPTION:
*        Parsing of transit tunnels
*
* INPUTS:
*        deviceObj  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        pmode      - parsing mode
*        internalTtiInfoPtr - internal tti info
*
*******************************************************************************/
GT_VOID snetChtMplsTransitTunnelsProtParsing
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN LION3_MPLS_TRANSIT_TUNNEL_PARSING_MODE_ENT    pmode,
    IN INTERNAL_TTI_DESC_INFO_STC      *internalTtiInfoPtr
);

/*******************************************************************************
* snetIngressTablesFormatInit
*
* DESCRIPTION:
*        init the format of ingress tables.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetIngressTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);

/*******************************************************************************
*   snetLion3PassengerOuterTagIsTag0_1
*
* DESCRIPTION:
*        set innerTag0Exists,innerPacketTag0Vid,innerPacketTag0CfiDei,innerPacketTag0Up fields in descriptor
*
* INPUTS:
*        deviceObj  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr   - pointer to the frame's descriptor.
*
*******************************************************************************/
void  snetLion3PassengerOuterTagIsTag0_1(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
);

/*******************************************************************************
*   snetLion3IngressMllAccessCheck
*
* DESCRIPTION:
*       Lion3 : check that IP/L2 MLL is not access out of range.
*               generate interrupt in case of access violation.
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*       usedForIp       - GT_TRUE  - used for IP-MLL
*                         GT_FALSE - used for L2-MLL
*       index           - index into the MLL table
* OUTPUT:
*
* RETURN:
*       indication that did error.
*       GT_TRUE  - error (access out of range) , and interrupt was generated.
*                   the MLL memory should NOT be accessed
*       GT_FALSE - no error , can continue MLL processing.
*
* COMMENTS:
*
*******************************************************************************/
GT_BOOL snetLion3IngressMllAccessCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN GT_BOOL      usedForIp,
    IN GT_U32       index
);


/*******************************************************************************
* snetXcatGetGreEtherTypes
*
* DESCRIPTION:
*       check if GRE over IPv4/6, and get the 'GRE ethertypes'
*
* INPUTS:
*       devObjPtr    - pointer to device object
*       descrPtr     - pointer to frame's descriptor
*
* OUTPUTS:
*       greEtherTypePtr  - gre etherType, relevant if function returns GT_TRUE(can be NULL)
*       gre0EtherTypePtr - gre 0 etherType, GRE protocols that are recognized as Ethernet-over-GRE(can be NULL)
*       gre1EtherTypePtr - gre 1 etherType, GRE protocols that are recognized as Ethernet-over-GRE(can be NULL)
*
* RETURN:
*       GT_BOOL      - is gre or not
*
* COMMENTS:
*
*******************************************************************************/
GT_BOOL snetXcatGetGreEtherTypes
(
    IN    SKERNEL_DEVICE_OBJECT             *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   *descrPtr,
    OUT   GT_U32                            *greEtherTypePtr,
    OUT   GT_U32                            *gre1EtherTypePtr,
    OUT   GT_U32                            *gre2EtherTypePtr
);

/*******************************************************************************
* snetLion3IngressReassignSrcEPort
*
* DESCRIPTION:
*        TRILL/TTI/PCL reassign new src EPort
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame descriptor
*       clientNamePtr - the name of the client (for the LOGGER)
*       newSrcEPort - the new srcEPort
* OUTPUTS:
*       descrPtr     - pointer to frame descriptor
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLion3IngressReassignSrcEPort
(
    IN    SKERNEL_DEVICE_OBJECT             *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   *descrPtr,
    IN    GT_CHAR*                          clientNamePtr,
    IN    GT_U32                            newSrcEPort
);

/*******************************************************************************
* snetChtPerformScibDmaRead
*
* DESCRIPTION:
*        wrap the scibDmaRead to allow the LOG parser to emulate the DMA:
*      write to HOST CPU DMA memory function.
*      Asic is calling this function to write DMA.
*
* INPUTS:
*       clientName - the DMA client name
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - the number of words/bytes (according to dataIsWords)
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       memPtr     - (pointer to) PP's memory in which HOST CPU memory will be
*                    copied.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetChtPerformScibDmaRead
(
    IN SNET_CHT_DMA_CLIENT_ENT clientName,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*  snetChtPerformScibDmaWrite
*
* DESCRIPTION:
*      wrap the scibDmaWrite to allow the LOG parser to emulate the DMA:
*      write to HOST CPU DMA memory function.
*      Asic is calling this function to write DMA.
* INPUTS:
*       clientName - the DMA client name
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - number of words of ASIC memory to write .
*       memPtr     - (pointer to) data to write to HOST CPU memory.
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetChtPerformScibDmaWrite
(
    IN SNET_CHT_DMA_CLIENT_ENT clientName,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*   scibSetInterrupt
*
* DESCRIPTION:
*       wrap the scibSetInterrupt to allow the LOG parser to emulate the interrupt:
*       Generate interrupt for SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void snetChtPerformScibSetInterrupt
(
    IN  GT_U32        deviceId
);

/*******************************************************************************
*   snetChtFromCpuDmaTxQueue
*
* DESCRIPTION:
*       Process transmitted frames per single SDMA queue
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to descriptor
*       txQue           - TxQ number
*
* OUTPUT:
*       isLastPacketPtr - more packets in next SDMA descriptors
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtFromCpuDmaTxQueue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 txQue,
    OUT GT_BOOL *isLastPacketPtr
);

/*******************************************************************************
*   snetBobcat2EeeProcess
*
* DESCRIPTION:
*       process EEE
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macPort             - the mac port number.
*       direction           - ingress egress direction
*
*******************************************************************************/
void snetBobcat2EeeProcess(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   macPort,
    IN SMAIN_DIRECTION_ENT      direction
);
/*******************************************************************************
*   snetBobcat2EeeCheckInterrupts
*
* DESCRIPTION:
*       check for EEE interrupts
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macPort             - the mac port number.
*
*******************************************************************************/
void snetBobcat2EeeCheckInterrupts(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   macPort
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahIngressh */


