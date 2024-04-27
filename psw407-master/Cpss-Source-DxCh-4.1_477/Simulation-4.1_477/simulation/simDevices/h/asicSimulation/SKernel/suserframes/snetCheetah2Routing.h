/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahRouting.h
*
* DESCRIPTION:
*       This is a external API definition for Router Engine Processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 16 $
*
*******************************************************************************/
#ifndef __snetCheetah2Routingh
#define __snetCheetah2Routingh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* TCAM_SEARCH value                */
#define SNET_CHT2_TCAM_SEARCH                               (2)

/* no match indicator during routing key matching checking */
#define SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS                (0xffffffff)

/* match fdb routing indicator during FDB routing key matching checking */
#define SNET_LION3_FDB_ROUT_MATCH_INDICATION_ONLY_CNS       (0)

/* number of capacity in ipv4 */
#define SNET_CHT2_IP_ROUT_ECMP                              (0)

/* SNET_RPF_CHECK_MODE_ENT
    description :
        unicast Reverse Path Forwarding check mode

    values:
        SNET_RPF_DISABLED_E     - uRPF check disabled
        SNET_RPF_VLAN_BASED_E   - Vlan-based uRPF check
        SNET_RPF_PORT_BASED_E   - Port-based uRPF check
        SNET_RPF_LOOSE_E        - Loose uRPF check
*/
typedef enum{
    SNET_RPF_DISABLED_E = 0,
    SNET_RPF_VLAN_BASED_E = 1,
    SNET_RPF_PORT_BASED_E = 2,
    SNET_RPF_LOOSE_E = 3
}SNET_RPF_CHECK_MODE_ENT;

/**
 * SNET_ROUTE_SECURITY_CHECKS_INFO_STC
 *
 * DESCRIPTION: IP security checks info
 *
 *  FIELDS:
 *       sipFromEcmpOrQosBlock        - source IP address is associated with ECMP/QOS block
 *       unicastSipSaCheck            - indicates whether to make SipSAcheck
 *       ipv6MulticastGroupScopeLevel - ipv6 multicast value for scope check
 *       mcRpfFailCmdFromMll          - is the RPF fail command taken from the MLL
 *                                      mcRpfFailCmdFromMll indicates how the RPF Fail command is selected.
 *                                      GT_TRUE:Use the RPF Fail Command in the MLL entry
 *                                              whose VLAN matches the packet VLAN
 *                                      GT_FALSE:Use the Multicast Route entry
 *
 *       sipLttNumberOfPaths          - used for uRPF check
 *       sipLttBaseRouteEntryIndex    - used for uRPF check
 *       rpfCheckMode                 - whether to make rpfCheck
 */
typedef struct{
    GT_U32                  sipFromEcmpOrQosBlock;
    GT_U32                  unicastSipSaCheck;
    GT_U32                  ipv6MulticastGroupScopeLevel;
    GT_BOOL                 mcRpfFailCmdFromMll;
    GT_U32                  sipNumberOfPaths;
    GT_U32                  sipBaseRouteEntryIndex;
    SNET_RPF_CHECK_MODE_ENT rpfCheckMode;
}SNET_ROUTE_SECURITY_CHECKS_INFO_STC;

/*******************************************************************************
*   snetLion3RoutingLpmEnable_test
*
* DESCRIPTION:
*       Enables simulation lpm test
*       When enable is true - it will run UT tgfBasicIpv4UcRouting
*       with lpm engine and not tcam.
*
*       Only UT tgfBasicIpv4UcRouting in lion3 supported.
*
*       Usage:
*               cpssInitSystem 28,1,0
*               snetLion3RoutingLpmEnable_test 1
*               utfTestsRun ".tgfBasicIpv4UcRouting",1,1
*
* INPUTS:
*       enable - indicates whether lpm test enabled or not
*
* RETURN: status - GT_OK
*
*******************************************************************************/
GT_STATUS  snetLion3RoutingLpmEnable_test
(
    IN GT_BOOL enable
);

/*******************************************************************************
*   snetCht2Routing
*
* DESCRIPTION:
*        (Cheetah) IPv4 and IPv6 Unicast/Multicast Routing
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
* OUTPUTS:
*       descrPtr     - (pointer to) updated frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID  snetCht2Routing(
    SKERNEL_DEVICE_OBJECT * devObjPtr,
    SKERNEL_FRAME_CHEETAH_DESCR_STC *  descrPtr
);

/* HW values for packet command
 0x0 = Route; Route; Packet is routed .
 0x1 = RouteAndMirrorToCPU; RouteAndMirrorToCPU
 0x2 = TrapToCPU; TrapToCPU; Packet is trapped with CPU code set to ???.
 0x3 = HardDrop; HardDrop
 0x4 = SoftDrop; SoftDrop
 0x5 = BridgeAndMirrorToCPU; BridgeAndMirrorToCPU; Packet is only bridged and mirrored to the CPU, with CPU code set to ???.
 0x6 = BridgeOnly; BridgeOnly; Packet is not routed

*/
typedef enum{
    HW_ROUTER_PACKET_COMMAND_ROUTE_E                        = 0 ,
    HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E      = 1 ,
    HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E                  = 2 ,
    HW_ROUTER_PACKET_COMMAND_HARD_DROP_E                    = 3 ,
    HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E                    = 4 ,
    HW_ROUTER_PACKET_COMMAND_BRIDGE_AND_MIRROR_TO_CPU_E     = 5 ,
    HW_ROUTER_PACKET_COMMAND_BRIDGE_E                       = 6 ,

    HW_ROUTER_PACKET_COMMAND_UNKNOWN_E                      = 7

}HW_ROUTER_PACKET_COMMAND_E;


typedef struct {
    HW_ROUTER_PACKET_COMMAND_E      ipHeaderCmd;            /* Command for ipv4/6 header error*/
    HW_ROUTER_PACKET_COMMAND_E      ipIllegalAddrCmd;       /* Command for iv4_6 illegal address error.*/
    HW_ROUTER_PACKET_COMMAND_E      ipDaMismatchCmd;        /* Command for ipv4_6 DA mismatch*/
    HW_ROUTER_PACKET_COMMAND_E      ipSaMismatchCmd;        /* Command for ipv4_6 SA mismatch*/
    HW_ROUTER_PACKET_COMMAND_E      ipvUcRpfRailCmd;        /* Command for ipv4_6 rpf fail.*/
    HW_ROUTER_PACKET_COMMAND_E      ipvMcRpfFailCmd;        /* Command for ipv4_6 rpf fail.*/
    HW_ROUTER_PACKET_COMMAND_E      ipvRouteEntry03Cmd;     /* Command for route entry.*/
    HW_ROUTER_PACKET_COMMAND_E      ipvHopLimitExceededFailCmd; /* Command for ipv4_6 rpf fail*/
    HW_ROUTER_PACKET_COMMAND_E      ipvRouteMtuExcceededFailCmd;/* Command for ipv4_6 rpf fail*/
    HW_ROUTER_PACKET_COMMAND_E      ipvOptionHbhOptionFailCmd;  /* Command for ipv4 options exists and for ipv6 HBH and non-HBH .*/
    HW_ROUTER_PACKET_COMMAND_E      ipvIcmpRedirectFailCmd;     /* Command for ipv4_6 rpf fail*/
    HW_ROUTER_PACKET_COMMAND_E      ipv6ScopeFailCmd;       /* Command for ipv4_6 rpf fail*/
    HW_ROUTER_PACKET_COMMAND_E      ipvAccessCmd;           /* Command for ipv4_6 rpf fail*/
    HW_ROUTER_PACKET_COMMAND_E      ipvAllZeroCmd;          /* Command for ipv4_6 rpf fail*/
    HW_ROUTER_PACKET_COMMAND_E      ipvSipFilterCmd;        /* Command for ipv4_6 sip filter fail*/

    GT_BIT      isRoutEntryForwardToCpu;/* is the rout entry forward to CPU */


    SKERNEL_EXT_PACKET_CMD_ENT      bridgePacketCmd;/* bridge packet command (prior to Router engine) */

    HW_ROUTER_PACKET_COMMAND_E      ipvFinalPacketCmd;  /* final packet command of the Router (before merge with the bridge command) */

    GT_BIT                          isMcRpfFail;        /* is MC RPF eVid check failed (regardless to packet command) */
    GT_BIT                          isMcRpfBidirFail;   /* is MC RPF bidirectional tree check failed (regardless to packet command) */

    HW_ROUTER_PACKET_COMMAND_E      ipvExceptionsPacketCmd;  /* summary of packet command of all Router 'exceptions'  */

    HW_ROUTER_PACKET_COMMAND_E      ipvNextHopPacketCmd;  /* summary of NEXT HOP packet command :
                                        ipvRouteEntry03Cmd + isRoutEntryForwardToCpu */


}SNET_CHEETAH2_L3_CNTRL_PACKET_INFO;

typedef enum {
    SKERNEL_ARP_MIRROR = 1,
    SKERNEL_ARP_TRAP,
}SKERNEL_SPECIAL_SERVICES_ARP_PACKET;

typedef struct{
    GT_U32  lttRouteType;        /* Defines the route type ECMP QOS */
    GT_U32  lttNumberOfPaths;   /* The number of Route paths bound to lookup */
    GT_U32  lttRouteEntryIndex; /* 0 or index for to block of continuous entries */
    GT_U32  lttUnicastRPFCheckEnable; /* If enabled the Unicast RPF CHECK performs*/
    GT_U32  lttUnicastSipSaEnable;   /* If enabled the MAC SIP/SA performs */
    GT_U32  lttIPv6GroupScopeLevel; /* IpV6 Scope checking */
}SNET_CHT2_L3_TRANSLATION_TABLE_STC;


/*******************************************************************************
*   snetCht2L3iIpv6HeaderError
*
* DESCRIPTION:
*    - IPv6 Header IP Version = 6
*    - IPv6 Payload Length + 40 bytes of IPv6 header + packet L3 byte offset +
*        4 CRC bytes <= packet Layer 2 byte count
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfo     - (pointer to) local frame data buffer Id
*       cpuCodes     -  cpu code associated with Header error
*
* RETURN:
*       GT_BOOL  - Status.
*       GT_TRUE  - No Exception.
*       GT_FALSE - There is an exception.
*
*******************************************************************************/
GT_BOOL snetCht2L3iIpv6HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
);

/*******************************************************************************
*   snetCht2L3iCounters
*
* DESCRIPTION:
*       Update Bridge counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*
*
*
*******************************************************************************/
GT_VOID snetCht2L3iCounters(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH2_L3_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN GT_U32 * routeIndexPtr

);

typedef enum{
    IPVX_PER_SRC_PORT_FIELD_ROUTER_PER_EPORT_SIP_SA_CHECK_ENABLE_E,
    IPVX_PER_SRC_PORT_FIELD_FCOE_FORWARDING_ENABLE_E,
    IPVX_PER_SRC_PORT_FIELD_IPV4_UC_ROUTING_ENABLE_E,
    IPVX_PER_SRC_PORT_FIELD_IPV4_MC_ROUTING_ENABLE_E,
    IPVX_PER_SRC_PORT_FIELD_IPV6_UC_ROUTING_ENABLE_E,
    IPVX_PER_SRC_PORT_FIELD_IPV6_MC_ROUTING_ENABLE_E,
    IPVX_PER_SRC_PORT_FIELD_DISABLE_SIP_LOOKUP_E,
}IPVX_PER_SRC_PORT_FIELDS_ENT;

/*******************************************************************************
*   lion3IpvxLocalDevSrcEportBitsGet
*
* DESCRIPTION:
*        get the value relate to the <localDevSrcEPort> from the IPvX ePort table
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*                   hold : descrPtr->eArchExtInfo.localDevSrcEPort ,descrPtr->eArchExtInfo.ipvxIngressEPortTablePtr
*        startBit  - start bit of field (0..)
* OUTPUTS:
*
* COMMENTS:
*
*******************************************************************************/
GT_U32  lion3IpvxLocalDevSrcEportBitsGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN IPVX_PER_SRC_PORT_FIELDS_ENT   fieldName
);


/*******************************************************************************
* snetIpvxTablesFormatInit
*
* DESCRIPTION:
*        init the format of IPvx tables.
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
void snetIpvxTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetCheetah2Routing */









