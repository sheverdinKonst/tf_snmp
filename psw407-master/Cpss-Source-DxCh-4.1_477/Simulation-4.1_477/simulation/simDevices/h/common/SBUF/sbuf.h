/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sbuf.h
*
* DESCRIPTION:
*       This is a API definition for SBuf module of the Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 14 $
*
*******************************************************************************/
#ifndef __sbufh
#define __sbufh

#include <os/simTypes.h>
#include <common/SQue/squeue.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SIM_CAST_BUFF(buf)   ((void*)(SBUF_BUF_ID)(buf))

/* NULL buffer ID in the case of allocation fail */
#define SBUF_BUF_ID_NULL                    NULL

/* typedefs */
typedef enum {
    SBUF_BUF_STATE_FREE_E,
    SBUF_BUF_STATE_BUZY_E
} SBUF_BUF_STATE_ENT;

/*
 * Typedef: GT_RX_CPU_CODE
 *
 * Description: Defines the different CPU codes that indicate the reason for
 *              sending a packet to the CPU.
 *
 * Fields:
 *
 *  Codes set by Ethernet Bridging Engine:
 *      CONTROL                 - Can be initiated only by another CPU.
 *      CONTROL_BPDU            - Control BPDU packet.
 *      CONTROL_DEST_MAC_TRAP   - Destination MAC trapped packet.
 *      CONTROL_SRC_MAC_TRAP    - Source MAC trapped packet.
 *      CONTROL_SRC_DST_MAC_TRAP- Source & Destination MAC trapped packet.
 *      CONTROL_MAC_RANGE_TRAP  - MAC Range Trapped packet.
 *      RX_SNIFFER_TRAP         - Trapped to CPU due to Rx sniffer
 *      INTERVENTION_ARP        - Intervention ARP
 *      INTERVENTION_IGMP       - Intervention IGMP
 *      INTERVENTION_SA         - Intervention Source address.
 *      INTERVENTION_DA         - Intervention Destination address.
 *      INTERVENTION_SA_DA      - Intervention Source and Destination addresses
 *      INTERVENTION_PORT_LOCK  - Intervention - Port Lock to CPU.
 *  Codes set by L3- L7 Engines:
 *      RESERVED_SIP_TRAP       - Trapped Due to Reserved SIP.
 *      INTERNAL_SIP_TRAP       - Trapped Due to Internal SIP.
 *      SIP_SPOOF_TRAP          - Trapped Due to SIP Spoofing
 *  TCB Codes:
 *      DEF_KEY_TRAP            - Trap by default Key Entry, after no match was
 *                                found in the Flow Table.
 *      IP_CLASS_TRAP           - Trap by Ip Classification Tables
 *      CLASS_KEY_TRAP          - Trap by Classifier's Key entry after a match
 *                                was found in the Flow Table.
 *      TCP_RST_FIN_TRAP        - Packet Mirrored to CPU because of TCP Rst_FIN
 *                                trapping.
 *      CLASS_KEY_MIRROR        - Packet Mirrored to CPU because of mirror bit
 *                                in Key entry.
 *      CLASS_MTU_EXCEED        - Classifier MTU exceed when redirecting a packet
 *                                through the classifier.
 *  Ipv4 Codes:
 *      RESERVED_DIP_TRAP       - Reserved DIP Trapping.
 *      MC_BOUNDARY_TRAP        - Multicast Boundary Trapping.
 *      INTERNAL_DIP            - Internal DIP.
 *      IP_ZERO_TTL_TRAP        - Packet was trapped due to TTL = 0 (valid for
 *                                IP header only).
 *      BAD_IP_HDR_CHECKSUM     - Bad IP header Checksum.
 *      RPF_CHECK_FAILED        - Packet did not pass RPF check, need to send
 *                                prune message.
 *      OPTIONS_IN_IP_HDR       - Packet with Options in the IP header.
 *      END_OF_IP_TUNNEL        - Packet which is in the End of an IP tunnel
 *                                sent for reassembly to the CPU.
 *      BAD_TUNNEL_HDR          - Bad tunnel header - Bad GRE header or packet
 *                                need to be fragmented with DF bit set.
 *      IP_HDR_ERROR            - IP header contains Errors - version!= 4,
 *                                Ip header length < 20.
 *      ROUTE_ENTRY_TRAP        - Trap Command was found in the Route Entry.
 *      IP_MTU_EXCEED           - IPv4 MTU exceed.
 *  MPLS Codes:
 *      MPLS_MTU_EXCEED         - MPLS MTU exceed.
 *      MPLS_ZERO_TTL_TRAP      - TTL in the MPLS shim header is 0.
 *      NHLFE_ENTRY_TRAP        - Trap command was found in the NHLFE.
 *      ILLEGAL_POP             - Illegal pop operation was done.
 *      INVALID_MPLS_IF         - Invalid MPLS Interface Entry was fetched.
 *  Egress Pipe Codes:
 *      ETH_BRIDGED_LLT         - Regular packet passed to CPU, Last level
 *                                treated (LLT) = Ethernet Bridged.
 *      IPV4_ROUTED_LLT         - Regular packet passed to CPU, LLT = IPv4
 *                                Routed.
 *      UC_MPLS_LLT             - Regular packet passed to CPU, LLT = Unicast
 *                                MPLS.
 *      MC_MPLS_LLT             - Regular packet passed to CPU, LLT = Multicast
 *                                MPLS (currently not supported).
 *      IPV6_ROUTED_LLT         - Regular packet passed to CPU, LLT = IPv6
 *                                Routed (currently not supported).
 *      L2CE_LLT                - Regular packet passed to CPU, LLT = L2CE.
 *
 */
typedef enum
{
    CONTROL                     = 1,
    CONTROL_BPDU                = 16,
    CONTROL_DEST_MAC_TRAP       = 17,
    CONTROL_SRC_MAC_TRAP        = 18,
    CONTROL_SRC_DST_MAC_TRAP    = 19,
    CONTROL_MAC_RANGE_TRAP      = 20,
    RX_SNIFFER_TRAP             = 21,
    INTERVENTION_ARP            = 32,
    INTERVENTION_IGMP           = 33,
    INTERVENTION_SA             = 34,
    INTERVENTION_DA             = 35,
    INTERVENTION_SA_DA          = 36,
    INTERVENTION_PORT_LOCK      = 37,
    RESERVED_SIP_TRAP           = 128,
    INTERNAL_SIP_TRAP           = 129,
    SIP_SPOOF_TRAP              = 130,
    DEF_KEY_TRAP                = 132,
    IP_CLASS_TRAP               = 133,
    CLASS_KEY_TRAP              = 134,
    TCP_RST_FIN_TRAP            = 135,
    CLASS_KEY_MIRROR            = 136,
    RESERVED_DIP_TRAP           = 144,
    MC_BOUNDARY_TRAP            = 145,
    INTERNAL_DIP                = 146,
    IP_ZERO_TTL_TRAP            = 147,
    BAD_IP_HDR_CHECKSUM         = 148,
    RPF_CHECK_FAILED            = 149,
    OPTIONS_IN_IP_HDR           = 150,
    END_OF_IP_TUNNEL            = 151,
    BAD_TUNNEL_HDR              = 152,
    IP_HDR_ERROR                = 153,
    ROUTE_ENTRY_TRAP            = 154,
    IP_MTU_EXCEED               = 161,
    MPLS_MTU_EXCEED             = 162,
    CLASS_MTU_EXCEED            = 163,
    MPLS_ZERO_TTL_TRAP          = 171,
    NHLFE_ENTRY_TRAP            = 172,
    ILLEGAL_POP                 = 173,
    INVALID_MPLS_IF             = 174,
    ETH_BRIDGED_LLT             = 248,
    IPV4_ROUTED_LLT             = 249,
    UC_MPLS_LLT                 = 250,
    MC_MPLS_LLT                 = 251,
    IPV6_ROUTED_LLT             = 252,
    L2CE_LLT                    = 253
}GT_RX_CPU_CODE;

/*
 * typedef: enum GT_PCK_ENCAP
 *
 * Description: Defines the different transmitted packet encapsulations.
 *
 * Fields:
 *      GT_REGULAR_PCKT  - Non - control packet.
 *      GT_CONTROL_PCKT  - Control packet.
 *
 * Comments:
 *      Non-control packets are subject to egress bridge filtering due
 *      to VLAN egress filtering or Spanning Tree filtering.
 *
 *      Control packets are not subject to bridge egress filtering.
 */
typedef enum
{
    GT_REGULAR_PCKT  = 0,
    GT_CONTROL_PCKT  = 7
}GT_PCKT_ENCAP;

/*
 * Typedef: enum GT_MAC_TYPE
 *
 * Description: Defines the different Mac-Da types of a transmitted packet.
 *
 * Fields:
 *      UNICAST_MAC         - MAC_DA[0] = 1'b0
 *      MULTICAST_MAC       - MAC_DA[0] = 1'b1
 *      BROADCAST_MAC       - MAC_DA = 0xFFFFFFFF
 *
 */
typedef enum
{
    UNICAST_MAC,
    MULTICAST_MAC,
    BROADCAST_MAC
}GT_MAC_TYPE;


typedef struct {
    GT_U32               local_ingress_port;
    GT_U32               ingress_is_trunk;
    GT_U32               ingress_trunk; /* 0 based trunk id*/
    GT_BOOL              from_stack_port;
    GT_U32               actual_ingress_port_trunk;
    GT_U32               actual_ingress_device;
    GT_BOOL              is_tagged;
    GT_RX_CPU_CODE       opcode;
    GT_U16               vlan_id;
    GT_U8                user_priority;
    GT_U8                rxQueue;
    GT_U32               frameType ;/*one of SCOR_FRAME_TYPE_ENT*/
}SAPI_RX_PACKET_DESC_STC ;


/*
 * Typedef: struct SAPI_TX_PACKET_DESC_STC
 *
 * Description: Includes needed parameters for enqueuing a packet to the PP's
 *              Tx queues.
 *
 * Fields:
 *      userPrioTag - User priority attached to the tag header.
 *      packetTagged - Packet is tagged.
 *      packetEncap - The outgoing packet encapsulation.
 *      sendTagged  - need to send the packet as tagged (not used when useVidx =1).
 *      dropPrecedence - The packet's drop precedence.
 *      recalcCrc   - GT_TRUE the PP should add CRC to the transmitted packet,
 *                    GT_FALSE leave packet unchanged.
 *      vid         - Packet vid.
 *      macDaType   - Type of MAC-DA (valid only if packetEncap = ETHERNET).
 *      txQueue     - The tx Queue number to transmit the packet on.
 *      txDevice    -  the device from which to send the packet this device will
 *                     send to the tgtDev,tgtPort or to the vidx
 *      cookie      - The user's data to be returned to the tapi Tx End handling
 *                    function.
 *
 *      useVidx     - Use / Don't use the Vidx field for forwarding the packet.
 *      dest        - Packet destination:
 *                      if(useVidx == 1)
 *                          Transmit according to vidx.
 *                      else
 *                          Transmit according to tgtDev & tgtPort.
 *
 */
typedef union {
    GT_U16       vidx;
    struct{
        GT_U8    tgtDev;
        GT_U8    tgtPort; /* 0 based port number --- never trunk */
    }devPort;
}SAPI_VIDX_UNT;

typedef struct {
    GT_U8           userPrioTag;
    GT_BOOL         packetTagged;
    GT_BOOL         sendTagged;
    GT_PCKT_ENCAP   packetEncap;
    GT_U8           dropPrecedence;
    GT_BOOL         recalcCrc;
    GT_U16          vid;
    GT_MAC_TYPE     macDaType;
    GT_U8           txQueue;
    GT_U8           txDevice;

    /* Internal Control Data    */
    GT_PTR          cookie;

    GT_BOOL         useVidx;
    SAPI_VIDX_UNT   dest;

}SAPI_TX_PACKET_DESC_STC ;

typedef struct{
    GT_BOOL     uplinkPort;/* uplink port has numbers of its own  */
    GT_U8       srcPort; /* the number of port --
                            can be the number of upLink port (not regular port)*/
}SAPI_SLAN_DESC_STC;

/*
 * Typedef: enum SBUF_FA_FORWARD_TYPE_ENT
 *
 * Description:
 *      FORWARDING to TARGET types.
 *
 * Fields:
 *
 *   SBUF_FA_FORWARD_TYPE_NONE_E
 *   SBUF_FA_FORWARD_TYPE_UC_FRAME_E
 *   SBUF_FA_FORWARD_TYPE_MC_FRAME_E
 *   SBUF_FA_FORWARD_TYPE_BC_FRAME_E
 *  SBUF_FA_FORWARD_TYPE_EGRESS_FPORT_E
 * Comments:
 */
typedef enum {
      SBUF_FA_FORWARD_TYPE_NONE_E,
      SBUF_FA_FORWARD_TYPE_UC_FRAME_E,  /* PP<->PP or PP<-FA->PP */
      SBUF_FA_FORWARD_TYPE_MC_FRAME_E,  /* PP<->PP or PP<-FA->PP */
      SBUF_FA_FORWARD_TYPE_BC_FRAME_E,  /* PP<->PP or PP<-FA->PP */
      SBUF_FA_FORWARD_TYPE_EGRESS_FPORT_E,
      SBUF_FA_FORWARD_TYPE_EGRESS_CPU_E,/* pp<->CC or CPU<-FA->CPU */
      SBUF_FA_FORWARD_TYPE_LAST_E
} SBUF_FA_FORWARD_TYPE_ENT ;

/*
 * Typedef: struct SBUF_FA_BURST_HDR_STC
 *
 * Description:
 *      Burst header .
 *
 * Fields:
 *
 *      target ; target type dependent format
 *                for unicast: <deviceId, port, LBH>
 *                for multicast: mcGroupId
 *                for broadcast: vlanId
 *                egressFport - used for Cpu Mail and PCS PING
 *                for others - not used
 *      targetType - value from SBUF_FA_TARGET_TYPE_ENT
 *      ingressFport - written by SLAN handler, to pass info to scorFa
 *
 * Comments:
 */
typedef struct {
    union {
        GT_U32 mcGroupId ; /* Mc */
        GT_U32 vlanId ; /* Bc, not supported by FA/XBAR, may by supported by PP */
        GT_U32 egressFport ;
        struct {
          GT_U8 deviceId ;
          GT_U8 port ;
          GT_U8 LBH ; /* load balance hash 0-3 */
          GT_U8 reserved;
        } ucTarget ;
    } target ; /* target type dependent format */
    SBUF_FA_FORWARD_TYPE_ENT targetType ;
    GT_U32 ingressFport ;
} SAPI_FA_FORWARD_DESC_STC ;

/* data send on the uplink from/to the PP */
typedef enum {
    SAPI_UPLINK_DATA_TYPE_FRAME_E,
    SAPI_UPLINK_DATA_TYPE_FDB_UPDATE_E
}SAPI_UPLINK_DATA_TYPE_ENT;

typedef struct{
    GT_MAC_TYPE     macDaType;
    GT_BOOL         useVidx;
    SAPI_VIDX_UNT   dest;
    GT_BOOL         doTrapToMaster;
}SAPI_UPLINK_FRAME_STC;

typedef struct{
    GT_U32      reserved;
}SAPI_UPLINK_FDB_UPDATE_STC;

typedef struct{
    SAPI_UPLINK_DATA_TYPE_ENT   type;
    union{
        SAPI_UPLINK_FRAME_STC      frame;
        SAPI_UPLINK_FDB_UPDATE_STC fdbUpdate;
    }info;
}SAPI_UPLINK_DATA_STC;

typedef enum {
    SBUF_USER_INFO_TYPE_SAPI_TX_E=1,
    SBUF_USER_INFO_TYPE_SAPI_RX_E,
    SBUF_USER_INFO_TYPE_SAPI_LINK_CHANGE_E,
    SBUF_USER_INFO_TYPE_SAPI_FDB_MSG_E,
    SBUF_USER_INFO_TYPE_SAPI_RW_REQUEST_E,
    SBUF_USER_INFO_TYPE_SAPI_FA_FORWARD_E,     /* for FA support */
    SAPI_USER_INFO_TYPE_SAPI_UPLINK_DATA_E,
    SAPI_USER_INFO_TYPE_SAPI_MAC_ACTION_ENDED_E,
    SAPI_USER_INFO_TYPE_SAPI_TX_SDMA_QUE_BMP_E
}SBUF_USER_INFO_TYPE_ENT;

typedef struct{
    SBUF_USER_INFO_TYPE_ENT type;
    union{
        SAPI_TX_PACKET_DESC_STC sapiTxPacket;/*scib to scor*/
        SAPI_RX_PACKET_DESC_STC sapiRxPacket;/*scor to scib*/
        SAPI_SLAN_DESC_STC      sapiSlanInfo;/*slan to scor*/
        SAPI_FA_FORWARD_DESC_STC sapiFaForward;/* scor<->scorFa<->scor
                                                or scor<->scor all via upLink*/
        SAPI_UPLINK_DATA_STC    sapiUplinkDataInfo;/* scor<->scorFa<->scor
                                                or scor<->scor all via upLink*/
        GT_U32      txSdmaQueueBmp;           /* transmit SDMA queue bitmap */
    }data;

    union{
        GT_U32      baseAddress;/* scor / scorFa send to scib */
        void*       devObj_PTR; /* scib send to scor/scor FA  */
    }target;
}SBUF_USER_INFO_STC;

/* Maximal dataSize in the buffer, max size of ingress and egress packets */
#define SBUF_DATA_SIZE_CNS                  12000

/*
 * Typedef: enum SBUF_BUFFER_FREE_STATE_ENT
 *
 * Description:
 *      values that the allocator of the buffer should consider
 *
 * Fields:
 *      SBUF_BUFFER_STATE_ALLOCATOR_CAN_FREE_E - the allocator of the buffer can
 *              free the buffer , because the buffer not used by other context.
 *      SBUF_BUFFER_STATE_OTHER_WILL_FREE_E - the allocator of the buffer can NOT
 *              free the buffer , because the buffer is used by other context.
 *              the other context will free the buffer.
 *
 */
typedef enum{
    SBUF_BUFFER_STATE_ALLOCATOR_CAN_FREE_E = 0,
    SBUF_BUFFER_STATE_OTHER_WILL_FREE_E
}SBUF_BUFFER_FREE_STATE_ENT;

/*
 * Typedef: struct SBUF_BUF_STC
 *
 * Description:
 *      Describe the buffer in the simulation.
 *
 * Fields:
 *      magic           : Magic number for consistence check.
 *      nextBufPtr      : Pointer to the next buffer in the pool or queue.
 *      state           : Buffers' state.
 *      freeState       : Buffers' state, when need to free allocated buffer.
 *      dataType        : Type of buffer's data.
 *      srcType         : Type of buffer's source.
 *      srcData         : Source specific data.
 *      actualDataPtr   : Pointer to the actual basic data at data section.
 *      actualDataSize  : Size of the actual data.
 *      userInfo        : User information.
 *      data            : Buffer's data memory - dynamic allocated buffer to hold info/packet .
 *                        when pool created.
 * Comments:
 */
typedef struct SBUF_BUF_STCT {
/* first fields bust be the same fields as in : SIM_BUFFER_STC */
    GT_U32                  magic;
    struct SBUF_BUF_STCT *  nextBufPtr;
    SBUF_BUF_STATE_ENT      state;
    SBUF_BUFFER_FREE_STATE_ENT freeState;
    GT_U32                  dataType;
    GT_U32                  srcType;
    GT_U32                  srcData;
    GT_U8                *  actualDataPtr;
    GT_U32                  actualDataSize;
    SBUF_USER_INFO_STC      userInfo;
    GT_U8                   *data;/* dynamic allocated buffer to hold info/packet*/
} SBUF_BUF_STC;

/* buffer ID typedef */
typedef SBUF_BUF_STC  * SBUF_BUF_ID;


/* Pool ID typedef */
typedef  void * SBUF_POOL_ID;

/* API functions */

/*******************************************************************************
*   sbufInit
*
* DESCRIPTION:
*       Initialize internal structures for pools and buffers management.
*
* INPUTS:
*       maxPoolsNum - maximal number of buffer pools.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       In the case of memory lack the function aborts application
*
*******************************************************************************/
void sbufInit
(
    IN  GT_U32              maxPoolsNum
);

/*******************************************************************************
*   sbufPoolCreate
*
* DESCRIPTION:
*       Create new buffers pool.
*
* INPUTS:
*       poolSize - number of buffers in a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*         SBUF_POOL_ID - new pool ID
*
* COMMENTS:
*       In the case of memory lack the function aborts application.
*
*******************************************************************************/
SBUF_POOL_ID sbufPoolCreate
(
    IN  GT_U32              poolSize
);

/*******************************************************************************
*   sbufPoolCreateWithBufferSize
*
* DESCRIPTION:
*       Create new buffers pool.
*
* INPUTS:
*       poolSize - number of buffers in a pool.
*       bufferSize - size in bytes of each buffer
* OUTPUTS:
*       None.
*
* RETURNS:
*         SBUF_POOL_ID - new pool ID
*
* COMMENTS:
*       In the case of memory lack the function aborts application.
*
*******************************************************************************/
SBUF_POOL_ID sbufPoolCreateWithBufferSize
(
    IN  GT_U32              poolSize,
    IN  GT_U32              bufferSize
);


/*******************************************************************************
*   sbufPoolFree
*
* DESCRIPTION:
*       Free buffers memory.
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufPoolFree
(
    IN  SBUF_POOL_ID    poolId
);
/*******************************************************************************
*   sbufAlloc
*
* DESCRIPTION:
*       Allocate buffer.
*
* INPUTS:
*       poolId   - id of a pool.
*       dataSize - size of the data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          SBUF_BUF_ID - buffer id if exist.
*          SBUF_BUF_ID_NULL - if no free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
SBUF_BUF_ID sbufAlloc
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize
);

/*******************************************************************************
*   sbufAllocWithProtectedAmount
*
* DESCRIPTION:
*       Allocate buffer , but only if there are enough free buffers after the alloc.
*
* INPUTS:
*       poolId - id of a pool.
*       dataSize - size for the alloc.
*       protectedAmount - number of free buffers that must be still in the pool
*                   (after alloc of 'this' one)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          SBUF_BUF_ID - buffer id if exist.
*          SBUF_BUF_ID_NULL - if no free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
SBUF_BUF_ID sbufAllocWithProtectedAmount
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize,
    IN  GT_U32          protectedAmount
);

/*******************************************************************************
*   sbufFree
*
* DESCRIPTION:
*       Free buffer.
*
* INPUTS:
*       poolId - id of a pool.
*       bufId - id of buffer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufFree
(
    IN  SBUF_POOL_ID    poolId,
    IN  SBUF_BUF_ID     bufId
);
/*******************************************************************************
*   sbufDataGet
*
* DESCRIPTION:
*       Get pointer on the first byte of data in the buffer
*       and actual size of data.
*
* INPUTS:
*       bufId       - id of buffer
*
* OUTPUTS:
*       dataPrtPtr  - pointer to actual data of buffer
*       dataSizePrt - actual data size of a buffer
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufDataGet
(
    IN  SBUF_BUF_ID     bufId,
    OUT GT_U8   **      dataPrtPtr,
    OUT GT_U32  *       dataSizePrt
);
/*******************************************************************************
*   sbufDataSet
*
* DESCRIPTION:
*       Set pointer to the start of data and new data size.
*
* INPUTS:
*       bufId    - id of buffer
*       dataPrt  - pointer to actual data of buffer
*       dataSize - actual data size of a buffer
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufDataSet
(
    IN  SBUF_BUF_ID     bufId,
    IN  GT_U8   *       dataPrt,
    IN  GT_U32          dataSize
);


/*******************************************************************************
*   sbufGetBufIdByData
*
* DESCRIPTION:
*       Get buff ID of buffer , by the pointer to any place inside the buffer
*
* INPUTS:
*       poolId - id of a pool.
*       dataPtr  - pointer to actual data of buffer
*
* OUTPUTS:
*
* RETURNS:
*      buff_id - the ID of the buffer that the data is in .
*                NULL if dataPtr is not in a buffer
* COMMENTS:
*
*       dataPtr - the pointer must point to data in the buffer
*
*******************************************************************************/
SBUF_BUF_ID sbufGetBufIdByData(
    IN  SBUF_POOL_ID    poolId,
    IN GT_U8   *        dataPtr
);

/*******************************************************************************
*   sbufFreeBuffersNumGet
*
* DESCRIPTION:
*       get the number of free buffers.
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          the number of free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 sbufFreeBuffersNumGet
(
    IN  SBUF_POOL_ID    poolId
);

/*******************************************************************************
*   sbufAllocatedBuffersNumGet
*
* DESCRIPTION:
*       get the number of allocated buffers.
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          the number of allocated buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 sbufAllocatedBuffersNumGet
(
    IN  SBUF_POOL_ID    poolId
);

/*******************************************************************************
*   sbufPoolSuspend
*
* DESCRIPTION:
*       suspend a pool (for any next alloc)
*
* INPUTS:
*       poolId   - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufPoolSuspend
(
    IN  SBUF_POOL_ID    poolId
);
/*******************************************************************************
*   sbufPoolResume
*
* DESCRIPTION:
*       Resume a pool that was suspended by sbufAllocAndPoolSuspend or by sbufPoolSuspend
*
* INPUTS:
*       poolId   - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufPoolResume
(
    IN  SBUF_POOL_ID    poolId
);

/*******************************************************************************
*   sbufAllocAndPoolSuspend
*
* DESCRIPTION:
*       Allocate buffer and then 'suspend' the pool (for any next alloc).
*
* INPUTS:
*       poolId   - id of a pool.
*       dataSize - size of the data.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*          SBUF_BUF_ID - buffer id if exist.
*          SBUF_BUF_ID_NULL - if no free buffers.
*
* COMMENTS:
*
*
*******************************************************************************/
SBUF_BUF_ID sbufAllocAndPoolSuspend
(
    IN  SBUF_POOL_ID    poolId,
    IN  GT_U32          dataSize
);

/*******************************************************************************
*   sbufPoolFlush
*
* DESCRIPTION:
*       flush all buffers in a pool (state that all buffers are free).
*       NOTE: operation valid only if pool is suspended !!!
*
* INPUTS:
*       poolId - id of a pool.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void sbufPoolFlush
(
    IN  SBUF_POOL_ID    poolId
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sbufh */


