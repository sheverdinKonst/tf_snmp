/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahEgress.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 21 $
*
*******************************************************************************/
#ifndef __snetCheetahEgressh
#define __snetCheetahEgressh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SNET_CHT_RX_DESC_CPU_OWN     (0)

#define CFM_OPCODE_BYTE_OFFSET_CNS   (1)

/* Sets the Own bit field of the rx descriptor */
/* Returns / Sets the Own bit field of the rx descriptor.           */
#define SNET_CHT_RX_DESC_GET_OWN_BIT(rxDesc) (((rxDesc)->word1) >> 31)

/* Return the buffer size field from the second word of an Rx desc. */
/* Make sure to set the lower 3 bits to 0.                          */
#define SNET_CHT_RX_DESC_GET_BUFF_SIZE_FIELD(rxDesc)             \
            (((((rxDesc)->word2) >> 3) & 0x7FF) << 3)

#define SNET_CHT_RX_DESC_SET_OWN_BIT(rxDesc, val)                     \
               (SMEM_U32_SET_FIELD((rxDesc)->word1,31,1,val))

#define SNET_CHT_RX_DESC_SET_FIRST_BIT(rxDesc, val)                     \
            (SMEM_U32_SET_FIELD((rxDesc)->word1,27,1,val))

#define SNET_CHT_RX_DESC_SET_LAST_BIT(rxDesc, val)                     \
            (SMEM_U32_SET_FIELD((rxDesc)->word1,26,1,val))

/* Return the byte count field from the second word of an Rx desc.  */
/* Make sure to set the lower 3 bits to 0.                          */
#define SNET_CHT_RX_DESC_SET_BYTE_COUNT_FIELD(rxDesc, val)        \
            (SMEM_U32_SET_FIELD((rxDesc)->word2,16,14,val))

/* Return the EI (enable interrupt) field from the first word of an Rx desc.  */
#define SNET_CHT_RX_DESC_GET_EI_FIELD(rxDesc)        \
             (((rxDesc)->word1  & 0x20000000) >> 29)


/* This is a reserved SrcPort indicating packet was TS on prior device.
  (relevant for XCat devices) */
#define SNET_CHT_PORT_60_CNS                           60

/* Embedded CPU port -- CAPWAP DTLS */
#define SNET_CHT_EMBEDDED_CPU_PORT_CNS                 59

/* OSM special port -- CAPWAP 802.11 "protected" data channel */
/* 7. CAPWAP Data Channel 802.11 Encryption/Decryption on the AC
    In Split MAC mode, in the event that the 802.11 encryption/decryption is
    performed on the AC rather than the WTP, the 802.11 data channel payload
    must be processed by an external CPU prior to ingress processing or
    transmission.
    This functionality is supported using the OSM ingress and egress redirection
    facility.
    7.1 Ingress 802.11 Decryption
        For the ingress case, the incoming packet matches a TTI for the given
        BSSID, and the TTI Action configuration has the <Ingress OSM Redirection>
        field set and the <OSM_CPU_Code> set to user-defined CPU Code.
        The packet is then redirected according to the CPU Code table entry for
        the user-defined CPU code.
        The CPU returns the packet with the 802.11 payload unencrypted,
        specifying in the DSA tag the packets original source port,and a
        special destination port 60 indicating that this packet was OSM
        redirected.
        Note the packet is subject to normal ingress processing as if it was
        received on the original ingress port.
    7.2 Egress 802.11 Decryption
        For the egress case, the outgoing packet tunnel-start entry is
        configured with <Egress OSM Redirect> and the <OSM_CPU_Code> set to the
        user-defined CPU Code.
        The packet is then redirected according to the CPU Code table entry
        for the user-defined CPU code.
        The CPU returns the packet with the encrypted 802.11 payload, specifying
        in the FROM CPU DSA tag the packets egress port.
        The packet is then transmitted as-is on the egress port.
*/
#define SNET_CHT_OSM_PORT_CNS                 0x3C /*60*/

/* get the actual number of ports of the device */
#define SNET_CHEETAH_MAX_PORT_NUM(dev)   ((dev)->portsNumber)

/* TXQ works in 'Hemisphere' -
   So Global port number is limited to 0..63 .
   the start port of the device/core start with (in context of 'Hemisphere')*/
#define SNET_CHT_EGR_TXQ_START_PORT_MAC(dev) \
       (SMEM_CHT_IS_SIP5_GET(dev) ? 0 :/* start from 0 */ \
       (((dev)->supportTxQGlobalPorts) ? \
        ((SMEM_CHT_GLOBAL_PORT_FROM_LOCAL_PORT_MAC(dev,0)) & 0x3f) : 0))

/* Global port number that the device start with plus total number of ports for this device */
#define SNET_CHT_EGR_TXQ_END_PORT_MAC(dev) \
       (SMEM_CHT_IS_SIP5_GET(dev) ? 256 :/* end at port 255 */     \
        (SNET_CHT_EGR_TXQ_START_PORT_MAC(dev) + SNET_CHEETAH_MAX_PORT_NUM(dev)))

/*
 * Typedef: struct STRUCT_RX_DESC
 *
 * Description: Includes the PP Rx descriptor fields, to be used for handling
 *              received packets.
 *
 * Fields:
 *      word1           - First word of the Rx Descriptor.
 *      word2           - Second word of the Rx Descriptor.
 *      buffPointer     - The physical data-buffer address.
 *      nextDescPointer - The physical address of the next Rx descriptor.
 *
 */
typedef struct
{
    /*volatile*/GT_U32         word1;
    /*volatile*/GT_U32         word2;

    /*volatile*/GT_U32         buffPointer;
    /*volatile*/GT_U32         nextDescPointer;
}SNET_CHT_RX_DESC    ;

typedef enum {
    SNET_CHT_READ_VLT_ACTION = 0,
    SNET_CHT_WRITE_VLT_ACTION
}SNET_CHT_RDWR_VLT_ACTION;

typedef enum {
    SNET_CHT_VLAN_VLT_TABLE = 0,
    SNET_CHT_MCGROUP_VLT_TABLE,
    SNET_CHT_STG_VLT_TABLE
}SNET_CHT_TABLE_VLT_TYPE;

/*  Modes of hashing into the designated trunk table that related to trunk member selection.
    NOTE: Cascade trunk is trunk that pointer by the 'Device map table' as
    destination for remote device.
    As well as multi-destination packets are forwarded according to
    global hashing mode of trunk hash generation.
*/
typedef enum{
    TRUNK_HASH_MODE_USE_PACKET_HASH_E,  /* use packet hash */
    TRUNK_HASH_MODE_USE_GLOBAL_SRC_PORT_HASH_E,/* use global src port for hash */
    TRUNK_HASH_MODE_USE_GLOBAL_DST_PORT_HASH_E, /* use global dst port for hash */
    TRUNK_HASH_MODE_USE_LOCAL_SRC_PORT_HASH_E,/* use global src port for hash , but not trunkId */

    TRUNK_HASH_MODE_USE_MULIT_DESTINATION_HASH_SETTINGS_E/* used for cascade trunk
                                                            that use the values of
                                                            the multi destination and
                                                            not per cascade trunk entry */

}TRUNK_HASH_MODE_ENT;

/*******************************************************************************
*   snetChtEgress
*
* DESCRIPTION:
*        Egress processing main routine
* INPUTS:
*        deviceObj - pointer to device object.
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetChtEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetChtEgressAfterTmUnit
*
* DESCRIPTION:
*       finish processing the packet in the egress pipe after the TM unit
*       finished with the packet.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       dataPtr     - pointer to TM attached info.
*       dataLength  - length of the data
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       this function called in context of smain task but hold buffer with
*       packet + descriptor that where send from the same task but 'long time ago'
*
*******************************************************************************/
GT_VOID snetChtEgressAfterTmUnit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * dataPtr,
    IN GT_U32                  dataLength
);


/*******************************************************************************
*   snetChtHaPerPortInfoGet
*
* DESCRIPTION:
*        Header Alteration - get indication about the index of per port and use
*                            of second register
*
* INPUTS:
*        deviceObj       -  pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        egressPort      - egress port (local port on multi-port group device)
*                          CPU port is port 63
*
* OUTPUTS:
*        isSecondRegisterPtr  - pointer to 'use the second register'
*        outputPortBitPtr - pointer to the bit index for the egress port
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetChtHaPerPortInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   egressPort,
    OUT GT_BOOL     *isSecondRegisterPtr,
    OUT GT_U32     *outputPortBitPtr
);


/*******************************************************************************
* snetChtEgfEgressEPortEntryGet
*
* DESCRIPTION:
*       function of the EGF unit
*       get pointer to the egress EPort table
*       Set value into descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr
*       Set value into descrPtr->eArchExtInfo.egfQagEgressEPortTablePtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - Cht frame descriptor
*        globalEgressPort - physical egress port -- Global port number !!!
*                    (referred as <LocalDevTrgPhyPort> in documentation ,
*                     but it is not part of the 'descriptor' !)
*
* OUTPUTS:
*
*   RETURN:
*
*
* COMMENTS :
*
*******************************************************************************/
void snetChtEgfEgressEPortEntryGet
(
   IN SKERNEL_DEVICE_OBJECT * devObjPtr,
   IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
   IN GT_U32    egressPort
);

/*******************************************************************************
*   snetChtHaArpTblEntryGet
*
* DESCRIPTION:
*       HA - Get ARP table mac address entry
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryIndex      - pointer to ARP memory table entry.
*
* OUTPUT:
*       arpEntry        - pointer to ARP entry
*
* RETURNS:
*
* COMMENT:
*
*******************************************************************************/
GT_VOID snetChtHaArpTblEntryGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 entryIndex,
    OUT SGT_MAC_ADDR_TYP * arpEntry
);

/*******************************************************************************
*   snetChtHaEgressTagDataExtGet
*
* DESCRIPTION:
*       HA - build VLAN tag info , in network order
*
* INPUTS:
*       vpt                 - user priority tag
*       vid                 - vlan ID
*       cfiDeiBit           - CFI/DEI bit
*       etherType           - VLAN ethernet type
* OUTPUT
*       tagData - tagged data --> in network order
*
*******************************************************************************/
void snetChtHaEgressTagDataExtGet
(
    IN  GT_U8   vpt,
    IN  GT_U16  vid,
    IN  GT_U8   cfiDeiBit,
    IN  GT_U16  etherType,
    OUT GT_U8   tagData[] /* 4 bytes */
);

/*******************************************************************************
*   snetChtHaMacFromMeBuild
*
* DESCRIPTION:
*        HA - get the SRC mac address to use as "mac from me"
* INPUTS:
*        deviceObj       -  pointer to device object.
*        descrPtr        -  pointer to the frame's descriptor.
*       egressPort   - the local egress port (not global port)
*       usedForTunnelStart - indication if used by tunnel start or by the routed packet.
*                           GT_TRUE - used by the TunnelStart
*                           GT_FALSE - used by the routed packet
* OUTPUTS:
*        macAddrPtr       - pointer to the mac address (6 bytes)
*
*
* COMMENTS:
*
*******************************************************************************/
void snetChtHaMacFromMeBuild
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   egressPort,
    IN GT_BOOL  usedForTunnelStart,
    OUT GT_U8   *macAddrPtr
);

/*******************************************************************************
*   snetChtHaMain
*
* DESCRIPTION:
*        HA - main HA unit logic
*
* INPUTS:
*        deviceObj       -  pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        destPorts       - number of egress port.
*        destVlanTagged  - send frame with tag.
*
* OUTPUTS:
*        frameDataPtr    - pointer to the frame.
*        frameDataSize   - size of frame.
*        isPciSdma       - if egress port is CPU enable/disable the PCI SDMA.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetChtHaMain
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   egressPort,
    IN GT_U8    destVlanTagged,
    OUT GT_U8 **frameDataPtrPtr,
    OUT GT_U32 *frameDataSize,
    OUT GT_BOOL *isPciSdma
);

/*******************************************************************************
* snetChtEgressDev_afterEnqueFromTqxDq
*
* DESCRIPTION:
*        Final Egress proceeding for descriptor that was stack in the queue of
*        TXQ_DQ port
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - Cht frame descriptor
*        egressPort - egress port
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS :
*
*******************************************************************************/
GT_VOID snetChtEgressDev_afterEnqueFromTqxDq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   egressPort,
    IN GT_U32   destVlanTagged
);

/*******************************************************************************
*   snetChtTxqDqPerPortInfoGet
*
* DESCRIPTION:
*        Txq.dq - get indication about the index of per port and use of second register
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        egressPort      - egress port (local port on multi-port group device)
*                          CPU port is port 63
*
* OUTPUTS:
*        isSecondRegisterPtr  - pointer to 'use the second register'
*        outputPortBitPtr - pointer to the bit index for the egress port
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetChtTxqDqPerPortInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN GT_U32   egressPort,
    OUT GT_BOOL     *isSecondRegisterPtr,
    OUT GT_U32     *outputPortBitPtr
);

/*******************************************************************************
* snetChtTx2Port
*
* DESCRIPTION:
*        Forward frame to target port
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        egressPort -   number of egress port.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_BOOL snetChtTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    IN GT_U8 *frameDataPtr,
    IN GT_U32 FrameDataSize
);

/*******************************************************************************
* snetChtEgressGetPortsBmpFromMem
*
* DESCRIPTION:
*        Egress Pipe : build bmp of ports from the pointer to the memory (registers/table entry)
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        memPtr    - pointer to the memory
* OUTPUTS:
*        portsBmpPtr - pointer to the ports bmp
* COMMENTS :
*
*******************************************************************************/
GT_VOID snetChtEgressGetPortsBmpFromMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                 *memPtr,
    OUT SKERNEL_PORTS_BMP_STC *portsBmpPtr
);

/*******************************************************************************
* snetChtEgressPortsBmpOperators
*
* DESCRIPTION:
*        Egress Pipe :
*           BMP operators :
*                operator1 - operator on bmp 1
*                operator2 - operator on bmp 2
*                resultBmpPtr - the bmp for the result of the operators.
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        operator1 - operator on bmp1
*        bmp1Ptr   - pointer to bmp 1
*        operator2 - operator on bmp2
*        bmp2Ptr   - pointer to bmp 2
* OUTPUTS:
*        resultBmpPtr - pointer to the ports bmp that is the 'result'
* COMMENTS :
*
*******************************************************************************/
GT_VOID snetChtEgressPortsBmpOperators
(
    IN SKERNEL_DEVICE_OBJECT        * devObjPtr,
    IN SKERNEL_BITWISE_OPERATOR_ENT operator1,
    IN SKERNEL_PORTS_BMP_STC        *bmp1Ptr,
    IN SKERNEL_BITWISE_OPERATOR_ENT operator2,
    IN SKERNEL_PORTS_BMP_STC        *bmp2Ptr,
    OUT SKERNEL_PORTS_BMP_STC       *resultBmpPtr
);

/*******************************************************************************
* snetChtEgressPortsBmpIsEmpty
*
* DESCRIPTION:
*        Egress Pipe : is BMP empty
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        bmpPtr   - pointer to bmp to check
* OUTPUTS:
*        None
* RETURNS:
*       1 - empty
*       0 - not empty
* COMMENTS :
*
*******************************************************************************/
GT_BIT snetChtEgressPortsBmpIsEmpty
(
    IN SKERNEL_DEVICE_OBJECT        * devObjPtr,
    IN SKERNEL_PORTS_BMP_STC        *bmpPtr
);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetahEgressh */


