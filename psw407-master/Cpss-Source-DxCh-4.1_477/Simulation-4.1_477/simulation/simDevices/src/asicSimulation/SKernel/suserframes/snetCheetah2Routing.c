/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah2Routing.c
*
* DESCRIPTION:
*       Layer 3 IP unicast/multicast routing for cht2 asic simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 128 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2Routing.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Routing.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Lpm.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

/* convert SNET_CHT2_ROUTE_ENGINE_CMD_ENT to HW_ROUTER_PACKET_COMMAND_E */
#define GET_HW_ROUTER_PACKET_COMMAND_NAME_MAC(/*IN*/hwRouterPacketCommand,/*OUT*/hwRouterPacketCommandName)      \
    switch(hwRouterPacketCommand)                                                                                \
    {                                                                                                            \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_ROUTE_E                       ,hwRouterPacketCommandName); \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E     ,hwRouterPacketCommandName); \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E                 ,hwRouterPacketCommandName); \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_HARD_DROP_E                   ,hwRouterPacketCommandName); \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E                   ,hwRouterPacketCommandName); \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_BRIDGE_AND_MIRROR_TO_CPU_E    ,hwRouterPacketCommandName); \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_BRIDGE_E                      ,hwRouterPacketCommandName); \
                                                                                                                 \
        SWITCH_CASE_NAME_MAC(HW_ROUTER_PACKET_COMMAND_UNKNOWN_E                     ,hwRouterPacketCommandName); \
        default: hwRouterPacketCommandName = "HW_ROUTER_PACKET_COMMAND_UNKNOWN_E";   break;                      \
    }

#define IS_ROUTER_DROP_MAC(routerCommand)                               \
    (((routerCommand) == HW_ROUTER_PACKET_COMMAND_HARD_DROP_E ||        \
      (routerCommand) == HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E) ? 1 : 0)
/* The Router engine overrides the Bridge CPU code assignment in the following cases:
If the Router engine assigned the TRAP command,
OR
If the Router engine assigned BRIDGE_AND_MIRROR or
ROUTE_AND_MIRROR packet command, and the Bridge engine does NOT
assign the packet command TRAP*/
#define IS_CPU_CODE_NEEDED_MAC(routerCommand,doRout)                       \
    ((((doRout) && (routerCommand) ==  HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E) ||        \
      ((routerCommand) ==  HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E)||                              \
      (((doRout) == 0) && ((routerCommand) ==  HW_ROUTER_PACKET_COMMAND_BRIDGE_AND_MIRROR_TO_CPU_E)))\
      ? 1 :0)

/* is this command NOT (trap or trap_and_mirror) */
#define IS_NOT_ROUTE_AND_NON_ROUTE_AND_MIRROR_MAC(routerHwCommand) \
    (((routerHwCommand) <= HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E ) ? 0 : 1)

/* is this command trap/mirro_to_cpu */
#define IS_TRAP_OR_MIRROR_TO_CPU_MAC(routerHwCommand)              \
    (((routerHwCommand) == HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E || \
      (routerHwCommand) == HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E ||             \
      (routerHwCommand) == HW_ROUTER_PACKET_COMMAND_BRIDGE_AND_MIRROR_TO_CPU_E) ? 1 : 0)

/* number of CPU CODES transmitted from Layer3 Engine */
#define SNET_CHT2_IP_ROUT_CPU_CODES_NUM_CNS                 (20)
/* key size for IPv6 in TCAM search */
#define SNET_CHT2_IPV6_KEY_SIZE_INDEX_CNS                   (5)
/* scope check value                */
#define SNET_CHT2_SCOPE                                     (3)

#define LTT_WORD_0                                          0
#define LTT_WORD_1                                          1
#define LTT_WORD_2                                          2
#define LTT_WORD_3                                          3
#define LTT_WORD_4                                          4

#define ACCESS_MATRIX_FORWARD                               0
#define ACCESS_MATRIX_SOFT_DROP                             1
#define ACCESS_MATRIX_HARD_DROP                             2

/* number exceptions types + access matrix */
#define SNET_CHT2_IP_EXCEPTION_TYPES_NUM_CNS                30/*place hold for future commands */
#define ROUTE_ENTRY_CMD_IDX                                 (5)

/*
    struct : SNET_CHT2_TRANSLATION_TABLE_ACTION_STC

    description : The Lookup translation table  consist of 5K entries
                  The Route Table consists pf 4K Route entries
*/

#define CMD_ROUTE           HW_ROUTER_PACKET_COMMAND_ROUTE_E
#define CMD_ROUTE_MIRR      HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E
#define CMD_TRAP            HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E
#define CMD_HARD_DROP       HW_ROUTER_PACKET_COMMAND_HARD_DROP_E
#define CMD_SOFT_DROP       HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E
#define CMD_BRIDGE_MIRROR   HW_ROUTER_PACKET_COMMAND_BRIDGE_AND_MIRROR_TO_CPU_E
#define CMD_BRIDGE          HW_ROUTER_PACKET_COMMAND_BRIDGE_E

/* indicates whether to use lpm or tcam+ltt */
static GT_BOOL isLpmEnabled = GT_TRUE;
GT_BOOL isLpmEnabled_test = GT_FALSE;


typedef struct {
    GT_U32 bottomEncap;
    GT_U32 vlanId;
    GT_U32 daTag;
    GT_U32 useVidx;
    union {
        GT_U32 vidx;
        struct {
            GT_U32 targetIsTrunk;
            GT_U32 trgPort;
            GT_U32 trgDev;
        } port;
        struct {
            GT_U32 targetIsTrunk;
            GT_U32 trgTrunkId;
            GT_U32 trgTrunkHush ;
        }trunk;
    } target;
} LLL_OUTLIF_STC;


/* Multicast Link List */
typedef struct {
    struct {
        GT_U32  rpf_fail;
        LLL_OUTLIF_STC lll;
        GT_U32 ttlThres;
        GT_U32 excludeSrcVlan;
        GT_U32 last;
    } first_mll;
    struct {
        GT_U32  rpf_fail;
        LLL_OUTLIF_STC lll;
        GT_U32 ttlThres;
        GT_U32 excludeSrcVlan;
        GT_U32 last;
        GT_U32 nextPtr;
    } second_mll;
} SNET_MLL_STC;

/* Converts bridge commands to route commands and checks exception */
#define SNET_CHT2_ROUTE_CMD_RESOLVE_MAC(pkt_cmd, route_cmd, exception, in_out_except) \
    route_cmd = routeEngineCmdArr[(pkt_cmd)]; \
    if (exception != SNET_CHT2_ROUTE_IP_UC_MC_ROUTE_ENTRY_0_3_E && \
       (route_cmd < SNET_CHT2_ROUTE_ENGINE_ROUT_E)) \
        in_out_except = (exception);


static GT_BOOL snetCht2L3iIpv4HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
);

static GT_BOOL  snetCht2L3iIpv4DipDaMisMatch
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
);

static GT_BOOL snetCht2L3iIpv4IllegalAdress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
);

static GT_BOOL snetCht2L3iIpv6DipDaMisMatch
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
);

static GT_BOOL snetCht2L3iIpv6IllegalAdress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfo
);


static GT_VOID snetCht2L3iTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr  ,
    IN GT_U32  *matchIndex
);

static GT_VOID snetCht2L3iFetchRouteEntry
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN  GT_U32                              *matchIndexPtr,
    OUT GT_U32                              *routeIndexPtr,
    OUT SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr
);

static GT_BOOL snetCht2ArpTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_BOOL snetCht2IPv4UCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_BOOL snetCht2IPv4MCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_BOOL snetCht2IPv6MCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetCht3URpfCheck
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN  GT_U32                              *routeIndexPtr,
    IN  SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr,
    IN  GT_U32                               vid1UnicastRoutingEn,
    IN  GT_U32                               nextHopVid1,
    OUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  *cntrlPcktInfo
);

static GT_VOID snetCht2L3iDropCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH2_L3_CNTRL_PACKET_INFO * cntrlPcktInfoPtr
);

static SKERNEL_EXT_PACKET_CMD_ENT cht2ResovedPacketl3intrarouterCmdTable[HW_ROUTER_PACKET_COMMAND_BRIDGE_E+1][HW_ROUTER_PACKET_COMMAND_BRIDGE_E+1]=
{
/*                     CMD_ROUTE       | CMD_ROUTE_MIRR   |CMD_TRAP     |CMD_HARD_DROP| CMD_SOFT_DROP| CMD_BRIDGE_MIRROR|CMD_BRIDGE       */
/*CMD_ROUTE    */    {CMD_ROUTE        , CMD_ROUTE_MIRR   ,CMD_TRAP     ,CMD_HARD_DROP, CMD_SOFT_DROP, CMD_BRIDGE_MIRROR,CMD_BRIDGE       },
/*CMD_ROUTE_MIRR*/   {CMD_ROUTE_MIRR   , CMD_ROUTE_MIRR   ,CMD_TRAP     ,CMD_HARD_DROP, CMD_SOFT_DROP, CMD_BRIDGE_MIRROR,CMD_BRIDGE       },
/*CMD_TRAP     */    {CMD_TRAP         , CMD_TRAP         ,CMD_TRAP     ,CMD_HARD_DROP, CMD_TRAP     , CMD_TRAP         ,CMD_TRAP         },
/*CMD_HARD_DROP*/    {CMD_HARD_DROP    , CMD_HARD_DROP    ,CMD_HARD_DROP,CMD_HARD_DROP, CMD_HARD_DROP, CMD_HARD_DROP    ,CMD_HARD_DROP    },
/*CMD_SOFT_DROP*/    {CMD_SOFT_DROP    , CMD_SOFT_DROP    ,CMD_TRAP     ,CMD_HARD_DROP, CMD_SOFT_DROP, CMD_SOFT_DROP    ,CMD_SOFT_DROP    },
/*CMD_BRIDGE_MIRROR*/{CMD_BRIDGE_MIRROR, CMD_BRIDGE_MIRROR,CMD_TRAP     ,CMD_HARD_DROP, CMD_SOFT_DROP, CMD_BRIDGE_MIRROR,CMD_BRIDGE_MIRROR},
/*CMD_BRIDGE   */    {CMD_BRIDGE       , CMD_BRIDGE       ,CMD_TRAP     ,CMD_HARD_DROP, CMD_SOFT_DROP, CMD_BRIDGE_MIRROR,CMD_BRIDGE       }
};

/* short names for pipe line packet commands */
#define PCK_FORWARD          SKERNEL_EXT_PKT_CMD_FORWARD_E
#define PCK_MIRROR_TO_CPU    SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E
#define PCK_TRAP_TO_CPU      SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E
#define PCK_HARD_DROP        SKERNEL_EXT_PKT_CMD_HARD_DROP_E
#define PCK_SOFT_DROP        SKERNEL_EXT_PKT_CMD_SOFT_DROP_E

/* hold value in bridge style for the matrix of convert router_style_packet_cmd and bridge_style_packet_cmd to bridge_style_packet_cmd */
static SKERNEL_EXT_PACKET_CMD_ENT cht2ResovedPacketl3intrabridgeCmdTable[SKERNEL_EXT_PKT_CMD_SOFT_DROP_E+1][HW_ROUTER_PACKET_COMMAND_BRIDGE_E+1]=
{
/*               CMD_ROUTE          | CMD_ROUTE_MIRR    | CMD_TRAP           |CMD_HARD_DROP| CMD_SOFT_DROP  | CMD_BRIDGE_MIRROR   | CMD_BRIDGE           */
/*forward   */ {PCK_FORWARD         , PCK_MIRROR_TO_CPU ,PCK_TRAP_TO_CPU     ,PCK_HARD_DROP, PCK_SOFT_DROP  , PCK_MIRROR_TO_CPU   , PCK_FORWARD          },
/*mirror    */ {PCK_MIRROR_TO_CPU   , PCK_MIRROR_TO_CPU ,PCK_TRAP_TO_CPU     ,PCK_HARD_DROP, PCK_TRAP_TO_CPU, PCK_MIRROR_TO_CPU   , PCK_MIRROR_TO_CPU    },
/*trap      */ {PCK_TRAP_TO_CPU     , PCK_TRAP_TO_CPU   ,PCK_TRAP_TO_CPU     ,PCK_HARD_DROP, PCK_TRAP_TO_CPU, PCK_TRAP_TO_CPU     , PCK_TRAP_TO_CPU      },
/*hard drop */ {PCK_HARD_DROP       , PCK_HARD_DROP     ,PCK_HARD_DROP       ,PCK_HARD_DROP, PCK_HARD_DROP  , PCK_HARD_DROP       , PCK_HARD_DROP        },
/*soft drop */ {PCK_SOFT_DROP       , PCK_SOFT_DROP     ,PCK_TRAP_TO_CPU     ,PCK_HARD_DROP, PCK_SOFT_DROP  , PCK_SOFT_DROP       , PCK_SOFT_DROP        }
};

#define SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NAME                                  \
     STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_PACKET_CMD                          )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_CPU_CODE_INDEX                      )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX    )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_VID1                       )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_PRECEDENCE              )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_MARKING_EN              )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_INDEX                   )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_DSCP                         )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_UP                           )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_EVLAN                      )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX                            )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_EVIDX                               )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK                     )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_TRUNK_ID                        )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_EPORT                           )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_DEV                             )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DIP_ACCESS_LEVEL                    )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_ACCESS_LEVEL                    )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL                     )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_PTR                             )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_PTR                          )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_RESERVED                            )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TS_IS_NAT                           )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_TYPE                         )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_DEST_SITE_ID                   )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_SCOPE_CHECK                    )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_COUNTER_SET_INDEX                   )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MTU_INDEX                           )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_BC_TRAP_MIRROR_EN               )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE_EN            )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_FILTER_EN                       )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DEC_TTL_OR_HOP_COUNT                )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ICMP_REDIRECT_EXCEP_MIRROR          )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_EVLAN_MRST_ID            )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK_FAIL_CMD      )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_FAIL_CMD_MODE            )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_INTERNAL_MLL_PTR             )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER_MSBITS  )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK               )\
    ,STR(SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER         )


static char * sip5IpvxNextHopFieldsTableNames[
    SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC sip5IpvxNextHopTableFieldsFormat[
    SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_PACKET_CMD                           */
    STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION  */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_CPU_CODE_INDEX                       */
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX     */
    ,STANDARD_FIELD_MAC(3)
        /*enable_vid1_unicast_routing = 1*/
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_VID1                        */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX}

        /*enable_vid1_unicast_routing = 0*/
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_PRECEDENCE               */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX}
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_MARKING_EN               */
    ,    STANDARD_FIELD_MAC(1)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_INDEX                    */
    ,    STANDARD_FIELD_MAC(7)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_DSCP                          */
    ,    STANDARD_FIELD_MAC(2)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_UP                            */
    ,    STANDARD_FIELD_MAC(2)

    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_EVLAN                       */
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX                             */
    ,STANDARD_FIELD_MAC(1)
        /* useVidx = 1 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_EVIDX                                */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX}

        /* useVidx = 0 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK                      */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX}

            /* isTrunk = 1 */
            /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_TRUNK_ID                         */
    ,        {FIELD_SET_IN_RUNTIME_CNS,
             12,
             SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK}

            /* isTrunk = 0 */
            /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_EPORT                            */
    ,        {FIELD_SET_IN_RUNTIME_CNS,
             13,
             SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK}
            /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_DEV                              */
    ,        STANDARD_FIELD_MAC(10)

    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DIP_ACCESS_LEVEL                     */
    ,STANDARD_FIELD_MAC(6)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_ACCESS_LEVEL                     */
    ,STANDARD_FIELD_MAC(6)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL                      */
    ,STANDARD_FIELD_MAC(1)
        /* isTunnel = 0 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_PTR                              */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         17,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL}

        /* isTunnel = 1 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_PTR                           */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         15,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL}
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_RESERVED*/
    ,    STANDARD_FIELD_MAC(1)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TS_IS_NAT                            */
    ,    STANDARD_FIELD_MAC(1)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_TYPE                          */
    ,    STANDARD_FIELD_MAC(1)

    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_DEST_SITE_ID                    */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_SCOPE_CHECK                     */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_COUNTER_SET_INDEX                    */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MTU_INDEX                            */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_BC_TRAP_MIRROR_EN                */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE_EN             */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_FILTER_EN                        */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DEC_TTL_OR_HOP_COUNT                 */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ICMP_REDIRECT_EXCEP_MIRROR           */
    ,STANDARD_FIELD_MAC(1)

    /* ipv6 MC */

    /*MC_Vlan_Check = "bidir_tree_check"*/
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_EVLAN_MRST_ID            */
    ,{FIELD_SET_IN_RUNTIME_CNS,
         6,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_UP}
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK_FAIL_CMD      */
    ,{48,3,0}/*start at bit 48 */
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_FAIL_CMD_MODE            */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_INTERNAL_MLL_PTR             */
    ,{53,16,0}/*start at bit 53 */
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER_MSBITS  */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK               */
    ,{75,2,0}/*start at bit 75 */
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER         */
    ,STANDARD_FIELD_MAC(13)


};
static SNET_ENTRY_FORMAT_TABLE_STC sip5_20IpvxNextHopTableFieldsFormat[
    SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_PACKET_CMD                           */
    STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION  */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_CPU_CODE_INDEX                       */
    ,STANDARD_FIELD_MAC(2)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX     */
    ,STANDARD_FIELD_MAC(3)
        /*enable_vid1_unicast_routing = 1*/
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_VID1                        */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX}

        /*enable_vid1_unicast_routing = 0*/
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_PRECEDENCE               */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX}
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_MARKING_EN               */
    ,    STANDARD_FIELD_MAC(1)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_INDEX                    */
    ,    STANDARD_FIELD_MAC(7)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_DSCP                          */
    ,    STANDARD_FIELD_MAC(2)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_UP                            */
    ,    STANDARD_FIELD_MAC(2)

    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_EVLAN                       */
    ,STANDARD_FIELD_MAC(13)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX                             */
    ,STANDARD_FIELD_MAC(1)
        /* useVidx = 1 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_EVIDX                                */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX}

        /* useVidx = 0 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK                      */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX}

            /* isTrunk = 1 */
            /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_TRUNK_ID                         */
    ,        {FIELD_SET_IN_RUNTIME_CNS,
             12,
             SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK}

            /* isTrunk = 0 */
            /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_EPORT                            */
    ,        {FIELD_SET_IN_RUNTIME_CNS,
             14,/* was 13 in sip 5*/
             SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK}
            /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_DEV                              */
    ,        STANDARD_FIELD_MAC(10)

    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DIP_ACCESS_LEVEL                     */
    ,STANDARD_FIELD_MAC(6)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_ACCESS_LEVEL                     */
    ,STANDARD_FIELD_MAC(6)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL                      */
    ,STANDARD_FIELD_MAC(1)
        /* isTunnel = 0 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_PTR                              */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         18, /*was 17 in sip 5*/
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL}

        /* isTunnel = 1 */
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_PTR                           */
    ,    {FIELD_SET_IN_RUNTIME_CNS,
         16, /*was 15 in sip 5*/
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL}
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_RESERVED*/
    ,    STANDARD_FIELD_MAC(1)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TS_IS_NAT                            */
    ,    STANDARD_FIELD_MAC(1)
        /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_TYPE                          */
    ,    STANDARD_FIELD_MAC(1)

    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_DEST_SITE_ID                    */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_SCOPE_CHECK                     */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_COUNTER_SET_INDEX                    */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MTU_INDEX                            */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_BC_TRAP_MIRROR_EN                */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE_EN             */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_FILTER_EN                        */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DEC_TTL_OR_HOP_COUNT                 */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ICMP_REDIRECT_EXCEP_MIRROR           */
    ,STANDARD_FIELD_MAC(1)

    /* ipv6 MC */

    /*MC_Vlan_Check = "bidir_tree_check"*/
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_EVLAN_MRST_ID            */
    ,{FIELD_SET_IN_RUNTIME_CNS,
         6,
         SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_UP}
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK_FAIL_CMD      */
    ,{48,3,0}/*start at bit 48 */
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_FAIL_CMD_MODE            */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_INTERNAL_MLL_PTR             */
    ,{53,16,0}/*start at bit 53 */
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER_MSBITS  */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK               */
    ,{75,2,0}/*start at bit 75 */
    /*SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER         */
    ,STANDARD_FIELD_MAC(16)

};

/*******************************************************************************
*   snetCht2ArpTrigger
*
* DESCRIPTION:
*    The following trigger checks are made on IPv4 Unicast packets:
*    �  IPv4 GLOBAL bit check
*    �  IPv4
*    �  IPv4
*
* INPUTS:
*       devObjPtr            - (pointer to) device object.
*       descrPtr             - (pointer to) frame data buffer Id
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2ArpTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr

)
{
    GT_U32 fldVal;              /* register field's value */

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E)
    {
     /* Enable router engine services for bridged ARP packets that were assigned
        to a Soft Drop Command prior to the Router engine */
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 27, 1, &fldVal);
        if (fldVal == 0)
        {
            return GT_FALSE;
        }
    }

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
    {
     /* Enable router engine services for bridged ARP packets that were assigned
        to a TRAP Command prior to the Router engine */
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 28, 1, &fldVal);
        if (fldVal == 0)
        {
            return GT_FALSE;
        }
    }

    /* The ARP Broadcast packet was previously assigned a MIRROR or TRAP command */
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
    {
        return GT_FALSE;
    }

    /* Router engine does not process ARP Broadcast packets */
    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr), 30, 2, &fldVal);
    if (fldVal == 0)
    {
        return GT_FALSE;
    }

    /* The ARP Broadcast was not bridged by this device */
    if (descrPtr->bypassBridge == 1)
    {
        return GT_FALSE;
    }

    if (descrPtr->ingressVlanInfo.ipInterfaceEn == GT_FALSE)
    {
        /* IPv4 Control to CPU is disabled */
        return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetCht2IPv4UniTrigger
*
* DESCRIPTION:
*    The following trigger checks are made on IPv4 Unicast packets:
*    �  IPv4 GLOBAL bit check
*    �  IPv4
*    �  IPv4
*
* INPUTS:
*       devObjPtr            - (pointer to) device object.
*       descrPtr             - (pointer to) frame data buffer Id
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2IPv4UCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr

)
{
    GT_BOOL status = GT_TRUE; /*  return value */
    GT_U32 fldVal;           /* register field's value */
    GT_U32 engconfreg;

    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 22, 1, &fldVal);
    status =  !((fldVal == 0)  &&
                (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E));

    if (status)
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 26, 1, &fldVal);
        status = !((fldVal == 0 ) &&
                   (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E));

    }

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*IPv4 UC routing enable*/
        fldVal = lion3IpvxLocalDevSrcEportBitsGet(devObjPtr, descrPtr, IPVX_PER_SRC_PORT_FIELD_IPV4_UC_ROUTING_ENABLE_E);
    }
    else
    {
        smemRegGet(devObjPtr, SMEM_CHT2_L3_UNICAST_CONTROL_IPV4_REG(devObjPtr),&engconfreg);   /* CPU ENABLE */
        /* Check Unicast enable for source Port. */
        if  (descrPtr->localDevSrcPort == SNET_CHT_CPU_PORT_CNS)
        { /* source port from CPU */
            fldVal = SMEM_U32_GET_FIELD(engconfreg,31 ,1);
        }
        else
        { /* Enable on source port */
            fldVal = SMEM_U32_GET_FIELD(engconfreg, descrPtr->localDevSrcPort,1); /*port enable */
        }
    }

    status = (fldVal == 1) ? GT_TRUE : GT_FALSE;

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    /* Enable IPv4 Unicast Routing on the VLAN */
    status = (descrPtr->ingressVlanInfo.ipv4UcRoutEn == 1) ? GT_TRUE : GT_FALSE;

    /* if packet l2 is valid we should check triggering on l2 fields */
    if(descrPtr->l2Valid)
    {
         /* This field includes whole the bridge checks that  */
        if (status && (descrPtr->ipV4ucvlan)  &&
           (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) &&
           ((descrPtr->bridgeUcRoutEn == 1) ||
            ((descrPtr->trgDev == descrPtr->ownDev) &&  (descrPtr->trgEPort == 61)) ||
            (descrPtr->fdbBasedUcRouting == 1)))
        {
            status = GT_TRUE;
        }
        else
        {
            status = GT_FALSE;
        }
    }
    else /* tunnel terminated */
    {
        /*we are here for non ipmc and only for ipv4 -- so no need to check those*/
        if (status && descrPtr->ipV4ucvlan)
        {
            status = GT_TRUE;
        }
        else
        {
            status = GT_FALSE;
        }
    }

    return status;
}

/*******************************************************************************
*   snetLion3FcoeTrigger
*
* DESCRIPTION:
*    The trigger checks are made on FCoE packets
*
* INPUTS:
*       devObjPtr - (pointer to) device object.
*       descrPtr  - (pointer to) frame data buffer Id
*
* RETURNS:
*       status    - GT_TRUE - no exception, GT_FALSE - there is an exception
*
*******************************************************************************/
static GT_BOOL snetLion3FcoeTrigger
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion3FcoeTrigger);

    GT_BOOL status = GT_TRUE; /* return value */
    GT_U32 fldVal;            /* register field's value */

    /* Check SOFT_DROP && <FCoE Soft Drop Router Enable> == 1 */
    smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 0, 1, &fldVal);
    status =  !((fldVal == 0) && (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E));

    /* Check TRAP && <FCoE Trap Router Enable> == 1 */
    if (status)
    {
        __LOG(("SOFT_DROP && <FCoE Soft Drop Router Enable> == 1"));
        smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 1, 1, &fldVal);
        status = !((fldVal == 0 ) &&
                   (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E));
    }

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }
    __LOG(("TRAP && <FCoE Trap Router Enable> == 1"));

    /* Check ePort<FCoE Forwarding Enable> == 1 */
    fldVal = lion3IpvxLocalDevSrcEportBitsGet(devObjPtr, descrPtr, IPVX_PER_SRC_PORT_FIELD_FCOE_FORWARDING_ENABLE_E);

    status = (fldVal == 1) ? GT_TRUE : GT_FALSE;
    if (status == GT_FALSE)
    {
        __LOG(("ePort<FCoE Forwarding Enable> == 0"));
        return GT_FALSE;
    }
    __LOG(("ePort<FCoE Forwarding Enable> == 1"));

    /* Check descr<FCOE Forward Enable> taken from vlan entry */
    status = (descrPtr->fcoeInfo.fcoeFwdEn == 1) ? GT_TRUE : GT_FALSE;
    if (status == GT_FALSE)
    {
        __LOG(("descr<FCOE Forward Enable> == 0"));
        return GT_FALSE;
    }
    __LOG(("descr<FCOE Forward Enable> == 1"));

    /* if packet l2 is valid we should check triggering on l2 fields */
    if(descrPtr->l2Valid)
    {
        __LOG(("check triggering on l2 fields"));

        if (status &&
           (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) &&
           ((descrPtr->bridgeUcRoutEn == 1) ||
           ((descrPtr->trgDev == descrPtr->ownDev) &&  (descrPtr->trgEPort == 61))))
        {
            status = GT_TRUE;
        }
        else
        {
            status = GT_FALSE;
        }
    }

    return status;
}

/*******************************************************************************
*   snetCht2IPv6UCTrigger
*
* DESCRIPTION:
*    The following trigger checks are made on IPv6 Unicast packets:
*       If the packet is native IPv6:  MAC DA is Unicast (MAC DA[40]=0),
*       AND
*       * MAC header <EtherType> = IPv6 (0x86DD),
*       AND
*           * Bridge FDB entry match for the destination lookup is configured with the <DA_ROUTE> bit set
*           OR
*           * Packet <target device> = local device and Packet <target port> = 61 (the virtual router port1)
*       AND
*       * Source port is enabled for IPv6 Unicast routing
*       AND
*       * VLAN-ID assigned to the packet is enabled for IPv6 Unicast routing

*       If the packet is tunnel-terminated:
*       * Passenger packet is IPv6
*       AND
*       * Passenger packet DIP is Unicast (DIP!= FF::/8)
*       AND
*       * VLAN-ID assigned to the packet is enabled for IPv6 Unicast routing
*
* INPUTS:
*       devObjPtr            - (pointer to) device object.
*       descrPtr             - (pointer to) frame data buffer Id
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2IPv6UCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    GT_BOOL status = GT_TRUE; /*  return value */
    GT_U32 fldVal;           /* register field's value */
    GT_U32 engconfreg;       /* register address value */

    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 20, 1, &fldVal);
    status = !((fldVal == 0)  && (descrPtr->packetCmd ==
               SKERNEL_EXT_PKT_CMD_SOFT_DROP_E));

    if (status)
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 24, 1, &fldVal);
        status =   !((fldVal == 0 ) && (descrPtr->packetCmd ==
                    SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E));
    }

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*IPv6 UC routing enable*/
        fldVal = lion3IpvxLocalDevSrcEportBitsGet(devObjPtr, descrPtr, IPVX_PER_SRC_PORT_FIELD_IPV6_UC_ROUTING_ENABLE_E);
    }
    else
    {
        /* Check Unicast enable for source Port. */
        smemRegGet(devObjPtr, SMEM_CHT2_L3_UNICAST_CONTROL_IPV6_REG(devObjPtr),&engconfreg);   /* CPU ENABLE */
        if  (descrPtr->localDevSrcPort == SNET_CHT_CPU_PORT_CNS)
        { /* source port from CPU */
            fldVal = SMEM_U32_GET_FIELD(engconfreg,31 ,1);
        }
        else
        { /* Enable on port */
            fldVal = SMEM_U32_GET_FIELD(engconfreg, descrPtr->localDevSrcPort,1); /*port enable */
        }
    }

    status = (fldVal) ? GT_TRUE : GT_FALSE;

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    /* Enable IPv6 Unicast Routing on the VLAN */
    status = (descrPtr->ingressVlanInfo.ipv6UcRoutEn == 1) ? GT_TRUE : GT_FALSE;

    if(descrPtr->tunnelTerminated)
    {
        /* not need extra checks */
        return status;
    }

    /* This field includes whole the bridge checks that  */
    if (status && (descrPtr->ipV6ucvlan)&&
       (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) &&
       ((descrPtr->bridgeUcRoutEn == 1) ||
       ((descrPtr->trgDev == descrPtr->ownDev) &&  (descrPtr->trgEPort == 61)) ||
            (descrPtr->fdbBasedUcRouting == 1)))
    {
        status = GT_TRUE;
    }
    else
    {
        status = GT_FALSE;
    }

    return status;
}

/*******************************************************************************
*   snetCht2IPv4MCTrigger
*
* DESCRIPTION:
*    The following trigger checks are made on IPv4 Unicast packets:
*       �  IPv4 GLOBAL bit check
*       �  IPv4
*
* INPUTS:
*       devObjPtr            - (pointer to) device object.
*       descrPtr             - (pointer to) frame data buffer Id
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2IPv4MCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr

)
{
    GT_BOOL status = GT_TRUE; /*  return value */
    GT_U32 fldVal;           /* register field's value */
    GT_U32 engconfreg;       /* register address value */

     /* MC soft drop v.4*/
    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 21, 1, &fldVal);
    status = !((fldVal == 0) && (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E));

    if (status ==  GT_TRUE)
    {
        /* MC trap v.4*/
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 25, 1, &fldVal);
        status =   (!((fldVal == 0 ) && (descrPtr->packetCmd ==
                    SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)))? GT_TRUE : GT_FALSE;
    }

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*IPv4 MC routing enable*/
        fldVal = lion3IpvxLocalDevSrcEportBitsGet(devObjPtr, descrPtr,IPVX_PER_SRC_PORT_FIELD_IPV4_MC_ROUTING_ENABLE_E);
    }
    else
    {
        smemRegGet(devObjPtr, SMEM_CHT2_L3_MULTICAST_CONTROL_IPV4_REG(devObjPtr), &engconfreg);
        if  (descrPtr->localDevSrcPort != SNET_CHT_CPU_PORT_CNS)
        {
            fldVal = SMEM_U32_GET_FIELD(engconfreg, descrPtr->localDevSrcPort, 1);
        }
        else
        {
            fldVal = SMEM_U32_GET_FIELD(engconfreg,31 ,1);
        }
    }

    status = (fldVal == 1) ? GT_TRUE : GT_FALSE;

    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    /* Enable IPv4 Multicast Routing on the VLAN */
    status = (descrPtr->ingressVlanInfo.ipv4McRoutEn == 1) ? GT_TRUE : GT_FALSE;

    if(descrPtr->tunnelTerminated)
    {
        /* not need extra checks */
        return status;
    }
     /* This field includes whole the bridge checks that  */
    if (status && descrPtr->ipV4mcvlan)
    {
        status = GT_TRUE;
    }
    else
    {
        status = GT_FALSE;
    }

    return status;
}

/*******************************************************************************
*   snetCht2IPv6MCTrigger
*
* DESCRIPTION:
*    The following trigger checks are made on IPv4 Unicast packets:
*    �  IPv6 GLOBAL bit check
*    �  IPv4
*    �  IPv4
*
* INPUTS:
*       devObjPtr            - (pointer to) device object.
*       descrPtr             - (pointer to) frame data buffer Id
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2IPv6MCTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr

)
{
    GT_BOOL status = GT_TRUE; /*  return value */
    GT_U32 fldVal;           /* register field's value */
    GT_U32 engconfreg;       /* register address value */

    /* MC soft drop v.6  */
    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 19, 1, &fldVal);
    status = !((fldVal == 0) && (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E));
    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

     /* drop v.6 */
    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 23, 1, &fldVal);
    status = !((fldVal == 0) && (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E));
    if (status == GT_FALSE)
    {
        return GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*IPv6 MC routing enable*/
        fldVal = lion3IpvxLocalDevSrcEportBitsGet(devObjPtr, descrPtr,IPVX_PER_SRC_PORT_FIELD_IPV6_MC_ROUTING_ENABLE_E);
    }
    else
    {
        smemRegGet(devObjPtr, SMEM_CHT2_L3_MULTICAST_CONTROL_IPV6_REG(devObjPtr),&engconfreg);
        if  (descrPtr->localDevSrcPort != SNET_CHT_CPU_PORT_CNS )
        {
            fldVal = SMEM_U32_GET_FIELD(engconfreg, descrPtr->localDevSrcPort,1);
        }
        else
        {
            smemRegGet(devObjPtr, SMEM_CHT2_L3_MULTICAST_CONTROL_IPV6_REG(devObjPtr),&engconfreg);
            fldVal = SMEM_U32_GET_FIELD(engconfreg,31 ,1);
        }
    }
    status =  (fldVal == 1) ? GT_TRUE : GT_FALSE;

    /* Enable IPv6 Multicast Routing on the VLAN */
    status =  (descrPtr->ingressVlanInfo.ipv6McRoutEn == 1) ? GT_TRUE : GT_FALSE;

    if(descrPtr->tunnelTerminated)
    {
        /* not need extra checks */
        return status;
    }
    /* This field includes whole the bridge checks that  */
    if (status && descrPtr->ipV6mcvlan)
    {
        status = GT_TRUE;
    }
    else
    {
        status = GT_FALSE;
    }

    return status;
}

/* get the name (string) and value of field */
#define NAME_AND_VALUE_MAC(field)   #field , field

/*******************************************************************************
*   logRouterPacketCommand
*
* DESCRIPTION:
*      take the name and value of the router command and LOG it + command value name
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       nameStr      - name of the command
*       value        - the numeric value of the command
* OUTPUTS:
*       none
*
* RETURN:
*
*
*******************************************************************************/
static void logRouterPacketCommand(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  GT_CHAR*                            nameStr,
    IN  GT_U32                              value
)
{
    GT_CHAR*    commandName;

    GET_HW_ROUTER_PACKET_COMMAND_NAME_MAC(value,commandName);

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%s] = [%s] (%d)\n",
        nameStr,commandName,value));
}


/*******************************************************************************
*   snetCht2L3iFcoeHeaderError
*
* DESCRIPTION:
*      Valid FCoE header
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfo - (pointer to) local frame data buffer Id
*
* RETURN:
*       GT_BOOL - status  GT-true Exception, gt-false There is NO exception
*
*******************************************************************************/
static GT_BOOL snetCht2L3iFcoeHeaderError
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    OUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  *cntrlPcktInfoPtr
)
{
    GT_BOOL status = GT_FALSE;  /* return code */
    GT_U32  fldVal;             /* register field's value */

    if (!descrPtr->fcoeInfo.fcoeLegal)
    {
        status = GT_TRUE;

        /* The command assigned to routed FCoE packets with Bad Header */
        smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 5, 3, &fldVal);

        cntrlPcktInfoPtr->ipHeaderCmd = fldVal;

        logRouterPacketCommand(devObjPtr,
            NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipHeaderCmd));
    }

    return status; /* return code */
}

/*******************************************************************************
*   snetCht2L3iExceptionCheck
*
* DESCRIPTION:
*    The following exception checks are made on IPv4/6 and FCoE packets:
*    �  IPv4/6 Header Error
*    �  IPv4/6 illegal Address
*    �  IPv4/6 DIP/DA mismatch
*    �  FCoE checks
*
* INPUTS:
*       devObjPtr         - (pointer to) device object.
*       descrPtr          - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfoPtr  - (pointer to) local frame data buffer Id
*
* RETURN:
*       GT_BOOL - status   GT_TRUE  - there is an exception
*                          GT_FALSE - there is no exceptions
*
*******************************************************************************/
static GT_BOOL snetCht2L3iExceptionCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr ,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    GT_BOOL status = GT_FALSE; /*  return value */
    GT_U32 header_bridge_fld;   /* field for special service check */

    if (descrPtr->isIPv4)
    {   /* Check IPv4 Header errors */
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 14, 1, &header_bridge_fld);
        if ((descrPtr->ipServiceFlag == GT_TRUE) || (( descrPtr->ipServiceFlag == GT_FALSE && header_bridge_fld)))
        {
            status = snetCht2L3iIpv4HeaderError(devObjPtr, descrPtr,
                                            cntrlPcktInfoPtr);
        }
        /* No Header error detected and for Bridged Unicast IPv4 Packets only IP Header Check required */
        if (!status && descrPtr->brdgUcIpHeaderCheck == GT_FALSE)
        {   /* Check IPv4 MisMatch DIP and its MAC DA errors */
            status = snetCht2L3iIpv4IllegalAdress(devObjPtr, descrPtr,
                                                       cntrlPcktInfoPtr);
            if (!status)  /* Illegal Address not detected */
            {   /* Check IPv4 Illegal Address errors */
                status = snetCht2L3iIpv4DipDaMisMatch(devObjPtr, descrPtr,
                                                  cntrlPcktInfoPtr);
            }
        }
    }
    else if(descrPtr->isFcoe)
    {   /* Check FCoE Header errors */
        smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 22, 1, &header_bridge_fld);
        if ((descrPtr->ipServiceFlag == GT_TRUE) || (( descrPtr->ipServiceFlag == GT_FALSE && header_bridge_fld)))
        {
            status = snetCht2L3iFcoeHeaderError(devObjPtr, descrPtr, cntrlPcktInfoPtr);
        }
    }
    else    /* ipv6 */
    {   /* Check IPv6 Header errors*/
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 15, 1, &header_bridge_fld);
        if ((descrPtr->ipServiceFlag == GT_TRUE) || (( descrPtr->ipServiceFlag == GT_FALSE && header_bridge_fld)))
        {
            status = snetCht2L3iIpv6HeaderError(devObjPtr,  descrPtr,
                                            cntrlPcktInfoPtr);
        }
        /* No Header error detected and for Bridged Unicast IPv6 Packets only IP Header Check required */
        if (!status && descrPtr->brdgUcIpHeaderCheck == GT_FALSE)
        {   /* Check IPv6 Illegal Address errors */
            status = snetCht2L3iIpv6IllegalAdress(devObjPtr, descrPtr ,
                                                  cntrlPcktInfoPtr);
            if (!status)
            {   /* Check IPv6 MisMatch DIP and its MAC DA errors */
                status = snetCht2L3iIpv6DipDaMisMatch(devObjPtr, descrPtr,
                                                        cntrlPcktInfoPtr);
           }
        }
    }

    return status;     /* return code */
}

/*******************************************************************************
*   snetCht2L3iIpv4HeaderError
*
* DESCRIPTION:
*    � Valid IPv4 header<Checksum>
*    � IPv4 header Version = 4
*    � IPv4 header IP Header Length = 5
*    � IPv4 header IP Header Length <= (IPv4 header Total Length / 4)
*    � IPv4 header IP Header Length + packet L3 byte offset + 4
*    � CRC bytes <= packet Layer 2 byte count
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfo - (pointer to) local frame data buffer Id
*       cpuCodes      -  cpu code associated with Header error
*
* RETURN:
*
*       GT_BOOL - status  GT-true Exception, gt-false There is NO exception
*******************************************************************************/
static GT_BOOL snetCht2L3iIpv4HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    GT_BOOL status = GT_FALSE;  /* return code */
    GT_U32 fldVal;              /* register field's value */

    if (descrPtr->ipHeaderError)
    {
        status = GT_TRUE;

        /* The command assigned to routed IPv4 packets with Bad Header */
        if (descrPtr->ipm == 0)  /* Unicast packet */
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr), 15, 3, &fldVal);
        }
        else
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr), 12, 3, &fldVal);
        }

        cntrlPcktInfoPtr->ipHeaderCmd = fldVal;
        logRouterPacketCommand(devObjPtr,
            NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipHeaderCmd));
    }

    return status;   /* return code */
}

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
*       cpuCodes     -  CPU code associated with Header error
*
* RETURN:
*       GT_BOOL - status  GT-true Exception, gt-false There is NO exception
*
*******************************************************************************/
GT_BOOL snetCht2L3iIpv6HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iIpv6HeaderError);

    GT_U32 fldVal;              /* register field's value */
    GT_U32 regAddr;             /* ???? */
    GT_BOOL status = GT_FALSE;  /* return code */

    if (descrPtr->ipHeaderError)
    {
        /* The command assigned to routed IPv6 packets with Bad Header */
        __LOG(("The command assigned to routed IPv6 packets with Bad Header"));
        if (descrPtr->ipm == 0)  /* Unicast packet */
        {
            regAddr = SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr) ;
            smemRegFldGet(devObjPtr, regAddr, 15, 3, &fldVal); /* bit IPv6headererrorEn */
            cntrlPcktInfoPtr->ipHeaderCmd =   fldVal ;
        }
        else
        {
            regAddr = SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr) ;
            smemRegFldGet(devObjPtr, regAddr, 12, 3, &fldVal); /* bit IPv6headererrorEn */
            cntrlPcktInfoPtr->ipHeaderCmd  =  fldVal;
        }

        logRouterPacketCommand(devObjPtr,
            NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipHeaderCmd));

        status = GT_TRUE ;
    }

    return status;       /* return code */
}
/*******************************************************************************
*   snetCht2L3iIpv4IllegalAdress
*
* DESCRIPTION:
*       IPv4 Illegal address such as 0.0.0.0 127.0.0.0
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfo     - (pointer to) local frame data buffer Id
*       cpuCodes     -  cpu code associated with IllegalAdress  error
*
* RETURN:  GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2L3iIpv4IllegalAdress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iIpv4IllegalAdress);

    GT_U32 fldVal;                   /* register field's value */
    GT_U32 destIp;                   /* dest ip address */
    GT_U32 srcIp;                    /* source ip address */
    GT_U32 _8msbOfSip;/*8 MSB of the SIP*/
    GT_U32 _8msbOfDip;/*8 MSB of the DIP*/
    GT_BOOL error = GT_FALSE;

    destIp = descrPtr->dip[0];
    srcIp  = descrPtr->sip[0];
    _8msbOfSip = srcIp>>24;
    _8msbOfDip = destIp>>24;

    if (descrPtr->ipm == 0)
    {
        /*
            An IPv4 SIP is declared illegal if it is equal to:
            - 127.0.0.0/8 (loopback)
            - 224.0.0.0/4 (Multicast class D)
            An IPv4 DIP is declared illegal if it is equal to:
            - 0.0.0.0/32 (Unspecified address)
            - 127.0.0.0/8 (loopback)
        */

        if(_8msbOfSip == 127)/*127.0.0.0/8 (loopback)*/
        {
            error = GT_TRUE;
        }
        else if((_8msbOfSip & 0xf0) == 224 ) /*224.0.0.0/4 (Multicast class D)*/
        {
            error = GT_TRUE;
        }

        if(_8msbOfDip == 127) /*127.0.0.0/8 (loopback)*/
        {
            error = GT_TRUE;
        }
        else if(destIp == 0)/*0.0.0.0/32 (Unspecified address)*/
        {
            error = GT_TRUE;
        }
    }
    else
    {
        /*
            A SIP is declared as Illegal if it equal to:
            - 0.0.0.0
            - 127.0.0.0/8 (loop back)
            - 224.0.0.0/4 (Multicast class D)
        */
        if(_8msbOfSip == 127)/*127.0.0.0/8 (loopback)*/
        {
            error = GT_TRUE;
        }
        else if((_8msbOfSip & 0xf0) == 224 ) /*224.0.0.0/4 (Multicast class D)*/
        {
            error = GT_TRUE;
        }
        else if(srcIp == 0)
        {
            error = GT_TRUE;
        }
    }

    if(error == GT_TRUE)
    {
        __LOG(("Got illegal IP address error"));

        if (descrPtr->ipm == 0)  /* Unicast packet */
        {
            __LOG(("IPv4 UC Illegal Address Command"));
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr), 27, 3, &fldVal);
        }
        else
        {
            __LOG(("IPv4 MC Illegal Address Command"));
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr), 24, 3, &fldVal);
        }

        __LOG(("The command assigned to routed IPv4 packets with Bad Header"));
        cntrlPcktInfoPtr->ipIllegalAddrCmd = fldVal;
        logRouterPacketCommand(devObjPtr, NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipIllegalAddrCmd));

        return GT_TRUE;
    }

    return GT_FALSE;  /* return value */
}

/*******************************************************************************
*   snetCht2L3iIpv6IllegalAdress
*
* DESCRIPTION:
*    IPv6 Illegal address such as ::1/128 FF::0/128
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfoPtr     - (pointer to) local frame data buffer Id
*       cpuCodes     -  cpu code associated with IllegalAdress  error
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2L3iIpv6IllegalAdress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iIpv6IllegalAdress);

    GT_U32 pktCmd;              /* packet command  */
    GT_BOOL dipUnspecified;     /* DIP Unspecified */
    GT_BOOL dipLoopback;        /* DIP Loopback    */
    GT_BOOL sipLoopback;        /* SIP Loopback    */
    GT_BOOL sipMulticast;       /* SIP Multicast   */

    dipUnspecified =
        ((descrPtr->dip[0] == 0) && (descrPtr->dip[1] == 0)
         && (descrPtr->dip[2] == 0) && (descrPtr->dip[3] == 0));

    dipLoopback =
        ((descrPtr->dip[0] == 0) && (descrPtr->dip[1] == 0)
         && (descrPtr->dip[2] == 0) && (descrPtr->dip[3] == 1));

    sipLoopback =
        ((descrPtr->sip[0] == 0) && (descrPtr->sip[1] == 0)
         && (descrPtr->sip[2] == 0) && (descrPtr->sip[3] == 1));

    sipMulticast = ((descrPtr->sip[0] & 0xFF000000) == 0xFF000000);

    if (descrPtr->ipm == 0)
    {
        /* packet with unicast MAC_DA */
        __LOG(("packet with unicast MAC_DA"));
        if (sipLoopback || sipMulticast || dipUnspecified || dipLoopback)
        {
            if(sipLoopback) __LOG_PARAM(sipLoopback);
            if(sipMulticast) __LOG_PARAM(sipMulticast);
            if(dipUnspecified) __LOG_PARAM(dipUnspecified);
            if(dipLoopback) __LOG_PARAM(dipLoopback);

            __LOG(("The command assigned to IPv6 UC Illegal Address"));

            smemRegFldGet(
                devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr),
                27, 3, &pktCmd);
            cntrlPcktInfoPtr->ipIllegalAddrCmd = pktCmd;

            logRouterPacketCommand(devObjPtr,
                NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipIllegalAddrCmd));

            return GT_TRUE ;
        }
    }
    else
    {
        /* packet with multicast MAC_DA */
        __LOG(("packet with multicast MAC_DA"));
        if (sipLoopback || sipMulticast)
        {
            if(sipLoopback) __LOG_PARAM(sipLoopback);
            if(sipMulticast) __LOG_PARAM(sipMulticast);

            __LOG(("The command assigned to IPv6 MC Illegal Address"));

            smemRegFldGet(
                devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr),
                24, 3, &pktCmd);
            cntrlPcktInfoPtr->ipIllegalAddrCmd = pktCmd;

            logRouterPacketCommand(devObjPtr,
                NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipIllegalAddrCmd));

            return GT_TRUE ;
        }
    }

    return GT_FALSE ;
}

/*******************************************************************************
*   snetCht2L3iIpv4DipDaMisMatch
*
* DESCRIPTION:
*
*  IPv4 MisMatch IPv4 dest & is MC in UC packet
*                MC packet MAC DA maps invalid address
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       cntrlPcktInfoPtr     - (pointer to) local frame data buffer Id
*       cpu_codes     -  cpu code associated with DipDaMisMatch error entry
*
* RETURN:GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2L3iIpv4DipDaMisMatch
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iIpv4DipDaMisMatch);

    GT_U32  fldVal;           /* register field's value */
    GT_U32  ipdipVersion;
    GT_U32  ipMacMismatch=0,ipMacMismatch2=0;

    /* The DIP/DA Mismatch Error check is done for valid l2 packets. it is not applied
       to packets that are tunnel-terminated e.g ipv4 over ipv4, as these packets
       will always arrive with the Unicast MAC DA of the router. in eth over ipv4 e.g
       the packet will arrive with valid l2 header and triggering must be checked */
    if(!descrPtr->l2Valid)
    {
        return GT_FALSE;
    }

    ipdipVersion = descrPtr->dip[0] >> 28;
    ipMacMismatch =  (descrPtr->dip[0] & 0x007fffff);
    fldVal =  descrPtr->macDaPtr[3] >> 7;
    if(fldVal)
    {
        __LOG(("MAC_DA bit 23 is 1 --> can't be considered as IPMC \n"));
    }
    ipMacMismatch2 = (descrPtr->macDaPtr[3] & 0x7f) << 16;
    ipMacMismatch2 = ipMacMismatch2 | descrPtr->macDaPtr[4] << 8;
    ipMacMismatch2 = ipMacMismatch2 | descrPtr->macDaPtr[5];

    if ( (descrPtr->ipm == 0 && ipdipVersion == 0xe) /* IPv4 UC mismatch check DIP [31..28] == 0xE */
         || /* IPv4 MC mismatch check */
       ( (descrPtr->ipm) &&
         ((ipdipVersion != 0xe) || (descrPtr->macDaPtr[0] != 0x01) ||
          (descrPtr->macDaPtr[1] != 0x00) || (descrPtr->macDaPtr[2] != 0x5E) ||
         (fldVal != 0) || (ipMacMismatch != ipMacMismatch2) )))
    {
        __LOG(("MisMatch header"));
        if (descrPtr->ipm == 0)  /* Unicast packet */
        {
            __LOG(("IPv4 UC DA Mismatch Command"));
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(devObjPtr), 0, 3, &fldVal);
        }
        else
        {
            __LOG(("IPv4 MC DA Mismatch Command"));
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(devObjPtr), 3, 3, &fldVal);
        }

        cntrlPcktInfoPtr->ipDaMismatchCmd = fldVal;
        logRouterPacketCommand(devObjPtr, NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipDaMismatchCmd));

        return GT_TRUE;
    }

    return GT_FALSE;
}

/*******************************************************************************
*   snetCht2L3iIpv6DipDaMisMatch
*
* DESCRIPTION:
*                IPv6 MisMatch IPv6 dest & is MC in UC packet
*                MC packet MAC DA maps invalid address
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id

* OUTPUTS:
*       cntrlPcktInfoPtr     - (pointer to) local frame data buffer Id
*       cpu_codes     -  cpu code associated with DipDaMisMatch error
*
* RETURN:
*       GT_BOOL - status   GT-true No Exception, gt-false There is an exception ,
*
*******************************************************************************/
static GT_BOOL snetCht2L3iIpv6DipDaMisMatch
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT  SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iIpv6DipDaMisMatch);

    GT_STATUS status = GT_FALSE;
    GT_U32 fldVal;        /* register field's value */
    GT_U32 macValue31_0;          /* MAC address bits 0-31 value  */
    GT_U32 macValue47_32;         /* MAC address bits 32-47 value  */
    GT_BOOL dipIpIsMC = GT_FALSE;

    /* The DIP/DA Mismatch Error check is done for valid l2 packets. it is not applied
       to packets that are tunnel-terminated e.g ipv4 over ipv6, as these packets
       will always arrive with the Unicast MAC DA of the router. in eth over ipv6 e.g
       the packet will arrive with valid l2 header and triggering must be checked */
    if(!descrPtr->l2Valid)
    {
        return GT_FALSE;
    }


    /* Bits [31:0] of the destination MAC address  */
    macValue31_0 = SNET_BUILD_WORD_FROM_BYTES_PTR_MAC(&descrPtr->macDaPtr[2]);
    /* Bits [47:32] of the source MAC address of the reaching packet */
    macValue47_32 = SNET_BUILD_HALF_WORD_FROM_BYTES_PTR_MAC(&descrPtr->macDaPtr[0]);

    if ((descrPtr->dip[0] & 0xff000000) == 0xff000000)
    {
        dipIpIsMC = GT_TRUE;
    }

    if (descrPtr->ipm == 0)
    {/* Unicast packet */
        __LOG(("Unicast packet"));
        if (dipIpIsMC == GT_TRUE)
        {/* The command assigned to routed IPv4 packets with Bad Header */
            __LOG(("The command assigned to routed IPv4 packets with Bad Header"));
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(devObjPtr), 0, 3, &fldVal);
            cntrlPcktInfoPtr->ipHeaderCmd = fldVal;

            logRouterPacketCommand(devObjPtr,
                NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipHeaderCmd));

            status = GT_TRUE;
        }
    }
    else /* Packet is Multicast */
    {
        __LOG(("Packet is Multicast"));
        if((dipIpIsMC == GT_FALSE)              || /* if not multicast */
           ((macValue47_32 & 0xFFFF) != 0x3333) || /* or mac da doesn't match DIP */
           (macValue31_0 != descrPtr->dip[3]))
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(devObjPtr), 3, 3, &fldVal);
            cntrlPcktInfoPtr->ipDaMismatchCmd = fldVal;

            logRouterPacketCommand(devObjPtr,
                NAME_AND_VALUE_MAC(cntrlPcktInfoPtr->ipDaMismatchCmd));

            status = GT_TRUE;
        }
    }


    return status;
}

/*******************************************************************************
*   snetCht2L3iTriggerCheck
*
* DESCRIPTION:
*    The following trigger checks are made on both IPv4/v6 packets:
*
*    .  RouterEngineEn   - global trigger enable
*    �  IPv4/v6 Unicast/Multicast  SOFT DROP
*    �  IPv4/v6 Unicast/Multicast  TRAP packet
*    �  IPv4/6  Unicast/Multicast Routing per port
*    �  IPv4/6  Unicast Bridge Enable local port 61
*    �  IPv4/6  Multicast local port 61
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame descriptor.
*
* OUTPUTS:
*       descrPtr     - pointer to frame descriptor.
*
* RETURNS:
*       status       -  flag raised
*
*******************************************************************************/
static GT_BOOL  snetCht2L3iTriggerCheck
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iTriggerCheck);

    GT_U32    fldVal;               /* register field's value */
    GT_BOOL   status = GT_TRUE;     /* return value fail except routing */
    GT_CHAR*  checkFailedPrefixStr = "Router Trigger check failed due to";

    if(descrPtr->ttiRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E &&
        0 == SKERNEL_IS_CHEETAH2_ONLY_DEV(devObjPtr) &&
        ! SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        return GT_TRUE;
    }

    /* Routing Global enable check */
    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 6, 1, &fldVal);
    if (fldVal == 0)
    {
        __LOG(("%s routing is Globally Disabled \n",
            checkFailedPrefixStr));
        return GT_FALSE;
    }

    __LOG(("Check if packet policy switched"));
    if  (descrPtr->pclRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_OUT_IF_E)
    {
        __LOG(("%s policy redirect \n",
            checkFailedPrefixStr));
        return GT_FALSE;
    }

    /* Check if packet policy redirected to Route LTT entry */
    if(descrPtr->pclRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E ||
        (SMEM_CHT_IS_SIP5_GET(devObjPtr) &&
         descrPtr->ttiRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E))
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr), 7, 1, &fldVal);
            if(fldVal)
            {
                __LOG(("sip5: Enable bypassing router trigger requirement check for PBR packets"));
                descrPtr->ipServiceFlag = GT_TRUE;
                return GT_TRUE;
            }
        }
        else
        {
            __LOG(("PBR Bypass Router Trigger Requirements Enable\n"));
            return GT_TRUE;
        }
    }

    /* If from cascade port (Marvell tagged packet),
     then command must be forward and packet is not routed in other device.  */
    if( (descrPtr->marvellTagged == 1) &&
        ((descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E) ||
         (descrPtr->routed == 1)))
    {
        __LOG(("%s cascade port and 'not forward' or 'forward and rout(from DSA)' \n",
            checkFailedPrefixStr));

        return GT_FALSE;
    }

    if(descrPtr->isIp)
    {
        if(descrPtr->ipm == 0)
        {   /*Unicast packet*/
            __LOG(("IP Unicast packet"));
            if(descrPtr->isIPv4)
            {
                status = snetCht2IPv4UCTrigger(devObjPtr, descrPtr);
                descrPtr->ipServiceFlag = status;

                if(status == GT_FALSE)
                {
                    __LOG(("%s snetCht2IPv4UCTrigger \n",
                        checkFailedPrefixStr));
                }
            }
            else    /* ipv6 */
            {
                status = snetCht2IPv6UCTrigger(devObjPtr, descrPtr);
                descrPtr->ipServiceFlag = status;

                if(status == GT_FALSE)
                {
                    __LOG(("%s snetCht2IPv6UCTrigger \n",
                        checkFailedPrefixStr));
                }
            }
            /* Packet has not been triggered by Router engine */
            if(status == GT_FALSE)
            {
                __LOG(("IP Header Check for Bridged Unicast IPv4/6 Packets"));
                descrPtr->brdgUcIpHeaderCheck = GT_TRUE;
            }
        }
        else
        {    /* Multicast packet   */
            __LOG(("IP Multicast packet"));
            if(descrPtr->isIPv4)
            {
                status = snetCht2IPv4MCTrigger(devObjPtr, descrPtr);

                if(status == GT_FALSE)
                {
                    __LOG(("%s snetCht2IPv4MCTrigger \n",
                        checkFailedPrefixStr));
                }
            }   /* end of version 4 packet**/
            else    /* ipv6 */
            {
                status = snetCht2IPv6MCTrigger(devObjPtr, descrPtr);

                if(status == GT_FALSE)
                {
                    __LOG(("%s snetCht2IPv6MCTrigger \n",
                        checkFailedPrefixStr));
                }
            }
        }
    }
    else if (descrPtr->arp)
    {
        __LOG(("ARP packet \n"));
        status = snetCht2ArpTrigger(devObjPtr, descrPtr);
        if(status == GT_FALSE)
        {
            __LOG(("%s snetCht2ArpTrigger \n",
                checkFailedPrefixStr));
        }
    }
    else if (descrPtr->isFcoe && (descrPtr->ipm == 0) )
    { /*FCoE and Unicast packet*/
        __LOG(("Unicast FCoE \n"));

        status = snetLion3FcoeTrigger(devObjPtr, descrPtr);

        if(status == GT_FALSE)
        {
            __LOG(("%s snetLion3FcoeTrigger \n",
                checkFailedPrefixStr));
        }

        descrPtr->ipServiceFlag = status;
        if(status == GT_FALSE)
        {
            __LOG(("IP Header Check for Bridged Unicast FCOE Packets"));
            descrPtr->brdgUcIpHeaderCheck = GT_TRUE;
        }
    }
    else
    { /* isip = 0, isFcoe = 0 and not arp */
        __LOG(("%s isIp = 0, isFcoe = 0 and not arp \n",
            checkFailedPrefixStr));
         return GT_FALSE;
    }

    if (status)
    {
        __LOG(("When packet triggered by router engine clear <Application Specific CPU Code> flag \n"));
        descrPtr->appSpecCpuCode = 0;
    }

    __LOG_PARAM(descrPtr->appSpecCpuCode);

    return status;
}

/*******************************************************************************
* snetCht2L3iTcamLookUp
*
* DESCRIPTION:
*        Tcam lookup for IPv4/6 address .
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*
* OUTPUTS:
*
*       matchIndexPtr   - pointer to the matching index.
*
* RETURN:
*
* COMMENTS:
*       C.12.8.4  Router Tcam Table :   TCAM holds 5119 entries
*                 of 32 bits for IPv4 or 128 bits for IPv6.
*******************************************************************************/
static GT_VOID snetCht2L3iTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr  ,
    OUT GT_U32 *matchIndex
)
{
    DECLARE_FUNC_NAME(snetCht2L3iTcamLookUp);

    GT_U32   maxEntries;  /* 1024 entries for extended IPv6 , 5120 for IPv4*/
    GT_U32   entryIndex;  /* Index to the TCAM line */
    GT_U32   wordIndex;   /* Index to the word in the TCAM rule */
    GT_U32   idx_match;   /* Index for DIP or SIP search        */
    GT_U32 * routingTcamEntryDataPtr; /* pointer to routing TCAM data entry  */
    GT_U32 * routingTcamEntryCtrlPtr; /* pointer to routing TCAM ctrl entry  */
    GT_U32 * routingTcamEntryDataMaskPtr; /* pointer to routing TCAM data mask entry */
    GT_U32 * routingTcamEntryCtrlMaskPtr; /* pointer to routing TCAM ctrl mask entry  */
    GT_U32   compModeData; /* compare mode bit in the tcam rule */
    GT_U32   routingTcamData32Bits;/* routing Tcam data 32 lower bits   */
    GT_U32   ipAddr32Bits ; /* routing tcam search routing 16 lower bits   */
    GT_U32  *dataPtr;                 /* pointer to routing TCAM data entry */
    GT_U32  *dataMaskPtr;             /* pointer to routing TCAM ctrl entry  */
    GT_U32  *ctrlPtr;                 /* pointer to routing TCAM data mask entry */
    GT_U32  *ctrlMaskPtr;             /* pointer to routing TCAM ctrl mask entry  */
    GT_U32  entryIndexstart;          /* start entry range in TCAM table     */
    GT_U32  entryIndexlast;           /* end entry range in TCAM table     */
    GT_U32  ctrlIndex;                /* for M.C search index of destination */
    GT_U32  ctrlIndexmask;            /* for M.C search mask index of destination*/
    GT_U32  sourceIndex = 0;          /* for M.C search source index of Group */

    for (idx_match = 0 ; idx_match <  SNET_CHT2_TCAM_SEARCH;  ++idx_match)
    {
        if (descrPtr->isIPv4)
        {
            maxEntries = devObjPtr->routeTcamInfo.numEntriesIpv4/*SNET_CHT2_IP_ROUT_TCAM_5120_IPV4_CNS*/ ;
        }
        else  /* IPv6 */
        {
            maxEntries = devObjPtr->routeTcamInfo.numEntriesIpv6/*SNET_CHT2_IP_ROUT_TCAM_1024_IPV6_CNS*/  ;
        }

        /* initialize the matchIndexPtr */
        matchIndex[idx_match] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;

        /*  Get pointer to Tcam data entry */
        routingTcamEntryDataPtr = smemMemGet(devObjPtr, SMEM_CHT2_ROUTING_TCAM_DATA_TBL_MEM);

        /*  Get pointer to Tcam control entry */
        routingTcamEntryCtrlPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_CTRL_TBL_MEM);

        /*  Get pointer to Tcam data mask entry */
        routingTcamEntryDataMaskPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_DATA_MASK_TBL_MEM);

        /*  Get pointer to Tcam control mask entry */
        routingTcamEntryCtrlMaskPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_CTRL_MASK_TBL_MEM);



/*        if ((idx_match == 0) || (descrPtr->isIPv4 == 0) || (descrPtr->ipm == 0))
        {  IPv4 destination ip or IPv6 packet or Unicast packet*/
            entryIndexstart = 0;
            entryIndexlast = maxEntries;
 /*       }
        else
        {
            entryIndexstart = 0x400;
            entryIndexlast = 0xC00;
        } */
        for (entryIndex = entryIndexstart ; entryIndex <  entryIndexlast;  ++entryIndex,
           routingTcamEntryDataPtr += devObjPtr->routeTcamInfo.entryWidth,/*SNET_CHT2_IP_ROUT_TCAM_ENTRY_WIDTH_CNS*/
           routingTcamEntryCtrlPtr += devObjPtr->routeTcamInfo.entryWidth,/*SNET_CHT2_IP_ROUT_TCAM_ENTRY_WIDTH_CNS*/
           routingTcamEntryDataMaskPtr += devObjPtr->routeTcamInfo.entryWidth,/*SNET_CHT2_IP_ROUT_TCAM_ENTRY_WIDTH_CNS*/
           routingTcamEntryCtrlMaskPtr += devObjPtr->routeTcamInfo.entryWidth/*SNET_CHT2_IP_ROUT_TCAM_ENTRY_WIDTH_CNS*/
           )
        {
            dataPtr = routingTcamEntryDataPtr;
            ctrlPtr = routingTcamEntryCtrlPtr;
            dataMaskPtr = routingTcamEntryDataMaskPtr;
            ctrlMaskPtr = routingTcamEntryCtrlMaskPtr;

            if ((ctrlPtr[0] & (1<<17)) & (ctrlMaskPtr[0] & (1<<17)))/* check validity of entry */
            {
                compModeData =  (ctrlPtr[0] & (3<<18)) >>18; /* get comp mode */
                if (descrPtr->isIPv4 == 0)
                {
                     for (wordIndex = 0 ;
                         wordIndex < SNET_CHT2_IPV6_KEY_SIZE_INDEX_CNS;
                         ++wordIndex)
                    {
                        if (wordIndex == 2)  /* line is not containing relevant data for key */
                        {   /* IPv6 entry not Tunnel termination b47 array 2 */
                            __LOG(("IPv6 entry not Tunnel termination b47 array 2"));
                            if ((ctrlPtr[0] & (1<<15) ) & (ctrlMaskPtr[0] & (1<<15)))
                            {
                                break;
                            }
                        }

                        else if ((descrPtr->ipm) && (wordIndex == 1) && (idx_match == 1))
                        {
                            ctrlIndex = (SMEM_U32_GET_FIELD(ctrlPtr[0], 0, 14));
                            ctrlIndexmask = (SMEM_U32_GET_FIELD(ctrlMaskPtr[0], 0, 14));
                            sourceIndex = (ctrlIndex & ctrlIndexmask);
                        }

                        else if (compModeData==1) /* for ipv6 compModeData should be '1' */
                        {
                            /* calculate 32 bits for word 0 , word1, word3 and word4 */
                            __LOG(("calculate 32 bits for word 0 , word1, word3 and word4"));
                            routingTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);
                            if  (idx_match)
                            {
                                 ipAddr32Bits = (wordIndex < 2) ?
                                            descrPtr->sip[3-wordIndex]& dataMaskPtr[0] :
                                            descrPtr->sip[4-wordIndex] & dataMaskPtr[0];
                            }

                            else
                            {
                                 ipAddr32Bits = (wordIndex < 2) ?
                                            descrPtr->dip[3-wordIndex] & dataMaskPtr[0]:
                                            descrPtr->dip[4-wordIndex]& dataMaskPtr[0];
                            }
                            if (((routingTcamData32Bits != ipAddr32Bits)&& (descrPtr->ipm == 0)) ||
                                ((routingTcamData32Bits != ipAddr32Bits) && (descrPtr->ipm == 1)
                                 && (idx_match == 0))                                             ||
                                 ((routingTcamData32Bits != ipAddr32Bits) && (descrPtr->ipm == 1)
                                 && (wordIndex == 1) && (matchIndex[0] == sourceIndex))           ||
                                 ((routingTcamData32Bits != ipAddr32Bits) && (descrPtr->ipm == 1)
                                 && (wordIndex != 1) ))

                            {
                                break;
                            }
                            if (wordIndex == 4)
                            {
                                matchIndex[idx_match] = entryIndex;
                                entryIndex = maxEntries;
                                break ;
                            }
                        }

                        dataPtr += devObjPtr->routeTcamInfo.offsetBetweenBulks;  /* points to the next field in TCAM data entry  */
                        ctrlPtr += devObjPtr->routeTcamInfo.offsetBetweenBulks;  /* points to the next field in TCAM ctrl entry  */
                        dataMaskPtr += devObjPtr->routeTcamInfo.offsetBetweenBulks; /* pointer to routing TCAM data mask entry  */
                        ctrlMaskPtr += devObjPtr->routeTcamInfo.offsetBetweenBulks; /* pointer to routing TCAM ctrl mask entry  */
                    }

/*                    if ((*matchIndex) !=  SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
                        break;*/
                } /* for loop word index ipv6 search */
                else /* ipv4 packet */
                {
                   if (compModeData==0) /* for ipv4 compModeData should be '0' */
                   {
                       routingTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);
                       if (descrPtr->ipm)
                       {
                           ctrlIndex = (SMEM_U32_GET_FIELD(ctrlPtr[0], 0, 14));
                           ctrlIndexmask = (SMEM_U32_GET_FIELD(ctrlMaskPtr[0], 0, 14));
                           sourceIndex = (ctrlIndex & ctrlIndexmask);
                       }

                       ipAddr32Bits = (idx_match) ?
                                descrPtr->sip[0]& dataMaskPtr[0] :
                                descrPtr->dip[0]& dataMaskPtr[0] ;
                       if (((routingTcamData32Bits == ipAddr32Bits) && (descrPtr->ipm == 0)) ||
                           ((routingTcamData32Bits == ipAddr32Bits) && (descrPtr->ipm == 1)
                            && (idx_match == 0))                                             ||
                            ((routingTcamData32Bits == ipAddr32Bits) && (descrPtr->ipm == 1)
                            && (idx_match == 1) && (matchIndex[0] == sourceIndex)))
                       {
                           matchIndex[idx_match] = entryIndex;
                           entryIndex = maxEntries;
                           break ;
                       }
                   }
                }
            }/* valid entry */
       } /*  find an entry     */
    }
}

/*******************************************************************************
* snetCht2L3iFetchRouteEntry
*
* DESCRIPTION:
*       Fetch the lookup translation table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       matchIndexPtr   - pointer to the matching index.
*
* OUTPUTS:
*       routeIndexPtr        - pointer to the matching table.
*       ipSecurChecksInfoPtr - routing security checks information
*
* RETURN:
*
* COMMENTS:
*       C.12.9.2  Route Table : The policy TCAM holds 4096 lines
*
*******************************************************************************/
static GT_VOID snetCht2L3iFetchRouteEntry
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN  GT_U32                              *matchIndexPtr,
    OUT GT_U32                              *routeIndexPtr,
    OUT SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iFetchRouteEntry);

    SNET_CHT2_L3_TRANSLATION_TABLE_STC lttEntryData;
    GT_U32  * lttEntryPtr;          /* (pointer to) action entry in memory */
    GT_U32  matchInxByK[SNET_CHT2_TCAM_SEARCH];         /* index in  translationDataPtr table */
    GT_U32  regAddr[SNET_CHT2_TCAM_SEARCH];             /* Register address */
    GT_U32  idx_match;              /* Index of tcam match  */
    GT_U32  offsetInxByK[SNET_CHT2_TCAM_SEARCH];        /* offset address index for QoS register */
    GT_U32  qosProfEntryInx;        /* base address index for QoS register */
    GT_U32  qosEntryOffSet;         /* offset index for QoS register */
    GT_U32  fldValue = 0;           /* field value in QoS register */
    GT_U32  fldValue2;              /* field value in QoS register */

    for (idx_match = 0; idx_match < SNET_CHT2_TCAM_SEARCH;++ idx_match)
    {
        matchInxByK[idx_match]  = matchIndexPtr[idx_match] / 0x400;
        offsetInxByK[idx_match] =  matchIndexPtr[idx_match] % 0x400;
        regAddr[idx_match] = SMEM_CHT2_IP_ROUTE_ENTRY_TBL_MEM(devObjPtr,offsetInxByK[idx_match]);
        lttEntryPtr  = smemMemGet(devObjPtr, regAddr[idx_match]);

        switch (matchInxByK[idx_match])
        {
            case LTT_WORD_0:
                /* Read word 0 from the action table entry */
                __LOG(("Read word 0 from the action table entry"));
                 fldValue =  SMEM_U32_GET_FIELD(lttEntryPtr[0] , 0, 22);
                break;

            case LTT_WORD_1:
                fldValue2 =  SMEM_U32_GET_FIELD(lttEntryPtr[0] , 22,10);
                fldValue =  SMEM_U32_GET_FIELD (lttEntryPtr[1] , 0,12) <<10 |fldValue2;
                break;

            case LTT_WORD_2:
                fldValue2 =  SMEM_U32_GET_FIELD(lttEntryPtr[1] , 12,20) ;
                fldValue =   SMEM_U32_GET_FIELD(lttEntryPtr[2] , 0,2) <<20 |fldValue2 ;
                break;

            case LTT_WORD_3:
                fldValue =  SMEM_U32_GET_FIELD(lttEntryPtr[2] , 2,22) ;
                break;

            case LTT_WORD_4:
                fldValue2 =  SMEM_U32_GET_FIELD(lttEntryPtr[2] , 24,8);
                fldValue =   SMEM_U32_GET_FIELD(lttEntryPtr[3], 0,14)<<8 |fldValue2;
                break;
        }

        /* Fill the structure of the Look Translation Table */
        __LOG(("Fill the structure of the Look Translation Table"));

        lttEntryData.lttRouteType = SMEM_U32_GET_FIELD(fldValue, 0, 1);
        lttEntryData.lttNumberOfPaths = SMEM_U32_GET_FIELD(fldValue, 1, 3);
        lttEntryData.lttUnicastRPFCheckEnable = SMEM_U32_GET_FIELD(fldValue, 4, 1);
        lttEntryData.lttIPv6GroupScopeLevel = SMEM_U32_GET_FIELD(fldValue, 5, 2);
        lttEntryData.lttUnicastSipSaEnable = SMEM_U32_GET_FIELD(fldValue, 7, 1);
        lttEntryData.lttRouteEntryIndex = SMEM_U32_GET_FIELD(fldValue, 8, 13);


        if (lttEntryData.lttRouteType == SNET_CHT2_IP_ROUT_ECMP)
        {    /* Calculating Route Entry index in ECMP mode */
            __LOG(("Calculating Route Entry index in ECMP mode"));
            routeIndexPtr[idx_match] = lttEntryData.lttRouteEntryIndex +
                      (descrPtr->pktHash % (lttEntryData.lttNumberOfPaths +1));
        }
        else
        {    /* Calculating Route Entry index in QoS mode */
            __LOG(("Calculating Route Entry index in QoS mode"));
            qosProfEntryInx = descrPtr->qos.qosProfile / 8;
            qosEntryOffSet = descrPtr->qos.qosProfile % 8;
            regAddr[idx_match] = SMEM_CHT2_QOS_ROUTING_REG(devObjPtr, qosProfEntryInx);
            fldValue = SMEM_U32_GET_FIELD(regAddr[idx_match], 4 * qosEntryOffSet , 3);
            routeIndexPtr[idx_match] =  lttEntryData.lttRouteEntryIndex  +
                               (fldValue % (lttEntryData.lttNumberOfPaths + 1));
        }

        if (idx_match == 1)
        {
            __LOG(("fill security checks info - relevant only for SIP search"));
            ipSecurChecksInfoPtr->rpfCheckMode = (lttEntryData.lttUnicastRPFCheckEnable) ?
                                                      SNET_RPF_VLAN_BASED_E : SNET_RPF_DISABLED_E;
            ipSecurChecksInfoPtr->sipNumberOfPaths       = lttEntryData.lttNumberOfPaths;
            ipSecurChecksInfoPtr->sipBaseRouteEntryIndex = lttEntryData.lttRouteEntryIndex;
            ipSecurChecksInfoPtr->unicastSipSaCheck      = lttEntryData.lttUnicastSipSaEnable;
         }

         ipSecurChecksInfoPtr->ipv6MulticastGroupScopeLevel = lttEntryData.lttIPv6GroupScopeLevel;
    }
}


/*******************************************************************************
*   nextHop_processArp
*
* DESCRIPTION:
*       Next hop processing: arp
*
* INPUTS:
*       devObjPtr               - (pointer to) device object.
*       descrPtr                - (pointer to) frame data buffer Id
*       arpBcMirrTrapEn         - arpBcMirrTrapEn
*       localDescrPtr           - (pointer to) local frame data buffer Id
*
* OUTPUTS:
*       descrPtr                - (pointer to) frame data buffer Id
*
*******************************************************************************/
static GT_VOID nextHop_processArp
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN    GT_U32                           arpBcMirrTrapEn,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *localDescrPtr
)
{
    DECLARE_FUNC_NAME(nextHop_processArp);

    GT_U32  regVal;                        /* reg value */
    GT_BOOL arpBcTrapMirror = GT_FALSE;    /* ARP Broadcast Mirroring/Trap based on ARP DIP */

    if (arpBcMirrTrapEn)
    {
        smemRegGet(devObjPtr,SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr),&regVal);
        switch(SMEM_U32_GET_FIELD(regVal, 30, 2))
        {
            case SKERNEL_ARP_TRAP:
                __LOG(("IPv4 with ARP broadcast Trapped to the CPU"));
                descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                arpBcTrapMirror = GT_TRUE;
                break;

            case SKERNEL_ARP_MIRROR:
                __LOG(("IPv4 with ARP broadcast are Mirrored to the CPU"));
                descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ;
                arpBcTrapMirror = GT_TRUE;
                break;

            default:
                __LOG(("IPv4 with ARP broadcast are forwarded \n"));
                break;
        }
    }

    if(arpBcTrapMirror)
    {
        __LOG(("ARP Broadcast mirroring/trapping based on the DIP is triggered"));
        descrPtr->cpuCode        = SNET_CHT_ARP_BC_TO_ME;
        descrPtr->appSpecCpuCode = localDescrPtr->appSpecCpuCode;
        __LOG_PARAM(descrPtr->packetCmd);
        __LOG_PARAM(descrPtr->cpuCode);
    }
    else
    {
        __LOG(("Unicast IPv4/6 Packet ARP packet"));
    }

    __LOG(("ARP packets not forwarded according to the NH entry \n"));
}


/*******************************************************************************
*   nextHop_processQos
*
* DESCRIPTION:
*       Next hop processing: qos remarking
*
* INPUTS:
*       devObjPtr               - (pointer to) device object.
*       descrPtr                - (pointer to) frame data buffer Id
*       primaryNextHopEntryPtr  - (pointer to) primary next hop entry
*       localDescrPtr           - (pointer to) local frame data buffer Id
*       fdbDipLookupInfoPtr     - pointer to fdb dip lookup info
*
* OUTPUTS:
*       descrPtr                - (pointer to) frame data buffer Id
*       localDescrPtr           - (pointer to) local frame data buffer Id
*
*******************************************************************************/
static GT_VOID nextHop_processQos
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN    GT_U32                          *primaryNextHopEntryPtr,
    IN    GT_U32                           nextHopIndex,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *localDescrPtr,
    IN    SNET_LION3_FDB_ROUTE_ENTRY_INFO *fdbDipLookupInfoPtr
)
{
    DECLARE_FUNC_NAME(nextHop_processQos);

    GT_U32 qosProfileMarkingEn;
    GT_U32 modifyDscpMode;
    GT_U32 modifyUpMode;
    GT_U32  nextHopFieldName;

    __LOG(("Qos reMarking Command"));
    __LOG_PARAM(descrPtr->qosProfilePrecedence == SKERNEL_QOS_PROF_PRECED_SOFT);

    if(descrPtr->qosProfilePrecedence == SKERNEL_QOS_PROF_PRECED_SOFT)
    {
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_PRECEDENCE;

        localDescrPtr->qosProfilePrecedence =
            fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->qosProfilePrecedence :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 7, 1);

        __LOG_PARAM(localDescrPtr->qosProfilePrecedence);

        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_MARKING_EN;
        qosProfileMarkingEn =
            fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->qosProfileMarkingEn :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 8, 1);

        __LOG(("Qos Profile Marking En[%d] \n", qosProfileMarkingEn));

        if (qosProfileMarkingEn)
        {
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_QOS_PROFILE_INDEX;
            localDescrPtr->qos.qosProfile =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->qosProfileIndex :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 9, 7);
            __LOG_PARAM(localDescrPtr->qos.qosProfile);
        }

        if (SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_DSCP;
            modifyDscpMode =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->modifyDscp:
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName);
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MODIFY_UP;
            modifyUpMode   =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->modifyUp:
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName);
        }
        else
        {
            modifyDscpMode = (SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 16, 2));
            modifyUpMode = (SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 18, 2));
        }

        __LOG(("Modify DSCP[%d] , Modify UP[%d] \n", modifyDscpMode, modifyUpMode));

        if (modifyDscpMode == 1)
        {
            localDescrPtr->modifyDscp = 1;
        }
        else if(modifyDscpMode == 2)
        {
            localDescrPtr->modifyDscp = 0;
        }

        __LOG_PARAM(localDescrPtr->modifyDscp);


        if(modifyUpMode == 1)
        {
            localDescrPtr->modifyUp = 1;
        }
        else if(modifyUpMode == 2)
        {
            localDescrPtr->modifyUp = 0;
        }

        __LOG_PARAM(localDescrPtr->modifyUp);
    }
    else
    {
        __LOG(("NOT allowed to change QOS parameters due to HARD precedence (from previous engine) \n"));
    }
}


/*******************************************************************************
*   nextHop_mcRpfCheckFailed
*
* DESCRIPTION:
*       Next hop processing: MC RPF check failed
*
* INPUTS:
*       devObjPtr               - (pointer to) device object.
*       descrPtr                - (pointer to) frame data buffer Id
*       primaryNextHopEntryPtr  - (pointer to) primary next hop entry
*       cntrlPcktInfo           - (pointer to) control packet info
*
* OUTPUTS:
*       descrPtr                - (pointer to) frame data buffer Id
*       mcRpfFailCmdFromMllPtr  - (pointer to) if there is an RPF Failure exception,
*                             mcRpfFailCmdFromMll indicates how the
*                             RPF Fail command is selected.
*                             GT_TRUE:Use the RPF Fail Command in the MLL entry
*                                     whose VLAN matches the packet VLAN
*                             GT_FALSE:Use the Multicast Route entry
*
*******************************************************************************/
static GT_VOID nextHop_mcRpfCheckFailed
(
    IN    SKERNEL_DEVICE_OBJECT              *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC    *descrPtr,
    IN    GT_U32                             *primaryNextHopEntryPtr,
    IN    GT_U32                              nextHopIndex,
    IN    SNET_CHEETAH2_L3_CNTRL_PACKET_INFO *cntrlPcktInfo,
    OUT   GT_BOOL                            *mcRpfFailCmdFromMllPtr
)
{
    DECLARE_FUNC_NAME(nextHop_mcRpfCheckFailed);

    GT_U32 mcRpfFailCmdMode;  /* Define where the command is to be taken in case of a failure in the Multicast RPF check. */
    GT_U32  nextHopFieldName;

    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK_FAIL_CMD;
    cntrlPcktInfo->ipvMcRpfFailCmd = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName) :
                SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 14, 3);

    logRouterPacketCommand(devObjPtr,
        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvMcRpfFailCmd));


    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_FAIL_CMD_MODE;
    mcRpfFailCmdMode = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName) :
                SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 17, 1);

    if (mcRpfFailCmdMode == 0)
    {
        __LOG(("0 = MCRPFFailCmd: Multicast RPF check fail command is set according to <MCRPF FailCmd>."));
        *mcRpfFailCmdFromMllPtr = GT_FALSE;
    }
    else
    {
        __LOG(("1 = MLLEntry: Multicast RPF check fail command is set according to matching MLL Entry <MLLRPFFail Cmd[2:0]>."));

        __LOG(("send cpu codes according to mll"));

        /* If none of the MLL entries have a VLAN that matches the
           packet VLAN, then use the Multicast Route entry RPF Fail Command */
        *mcRpfFailCmdFromMllPtr = GT_TRUE;
    }

    __LOG_PARAM((*mcRpfFailCmdFromMllPtr));
}


/*******************************************************************************
*   snetCht2SipFiltering
*
* DESCRIPTION:
*       cht2 and above sip filtering
*
* INPUTS:
*       devObjPtr             - (pointer to) device object.
*       descrPtr              - (pointer to) frame data buffer Id
*       ipSecurChecksInfoPtr  - routing security checks information
*       primaryNextHopEntryPtr- primary (DIP or SIP for ipmc) associated next hop entry
*
* OUTPUTS:
*       cntrlPcktInfo         - (pointer to) control packet info
*
*******************************************************************************/
static GT_VOID snetCht2SipFiltering
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN  SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr,
    IN  GT_U32                              *primaryNextHopEntryPtr,
    IN    GT_U32                           nextHopIndex,
    OUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  *cntrlPcktInfo
)
{
    DECLARE_FUNC_NAME(snetCht2SipFiltering);

    GT_U32 fldVal;
    GT_U32  nextHopFieldName;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && ipSecurChecksInfoPtr->sipFromEcmpOrQosBlock)
    {
        __LOG(("SIP5: Unicast IPv4/6 SIP Filtering is not supported for ecmp/qos block"));
        return;
    }

    __LOG(("Unicast IPv4/6 SIP Filtering"));

    if (descrPtr->isIPv4)
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 16, 1, &fldVal);
    }
    else if(descrPtr->isFcoe)
    {
        smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 4, 1, &fldVal);
    }
    else
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 18, 1, &fldVal);
    }

    __LOG_PARAM(descrPtr->ipServiceFlag);
    __LOG_PARAM(fldVal);

    if((descrPtr->ipServiceFlag == GT_TRUE) ||
       (descrPtr->ipServiceFlag == GT_FALSE && fldVal))
    {
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_FILTER_EN;
        fldVal = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr,nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 17, 1);

        __LOG(("ipvSipFilterCmd is %s drop \n", fldVal ? "hard" : " NOT to"));

        if (fldVal)
        {
            cntrlPcktInfo->ipvSipFilterCmd =  HW_ROUTER_PACKET_COMMAND_HARD_DROP_E ;
            logRouterPacketCommand(devObjPtr, NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvSipFilterCmd));
        }
    }
}

/*******************************************************************************
*   snetCht2AccessMatrixFiltering
*
* DESCRIPTION:
*       cht2 and above:  access matrix based filtering
*
* INPUTS:
*       devObjPtr             - (pointer to) device object.
*       descrPtr              - (pointer to) frame data buffer Id
*       ipSecurChecksInfoPtr  - routing security checks information
*       sipNextHopEntryPtr    - SIP associated next hop entry
*       sipNextHopIndex        - the index in the NH table
*       dipAccessLevel        - dip access level
*
* OUTPUTS:
*       cntrlPcktInfo         - (pointer to) control packet info
*
*******************************************************************************/
static GT_VOID snetCht2AccessMatrixFiltering
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN  SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr,
    IN  GT_U32                              *sipNextHopEntryPtr,
    IN  GT_U32                               sipNextHopIndex,
    IN  GT_U32                               dipAccessLevel,
    OUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  *cntrlPcktInfo
)
{
    DECLARE_FUNC_NAME(snetCht2AccessMatrixFiltering);

    GT_U32 fldVal;
    GT_U32 sipAccessLevel;
    GT_U32  nextHopFieldName;

    ASSERT_PTR(sipNextHopEntryPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr)             &&
       ipSecurChecksInfoPtr->sipFromEcmpOrQosBlock &&
       ipSecurChecksInfoPtr->rpfCheckMode == SNET_RPF_VLAN_BASED_E)
    {
        __LOG(("SIP5: for ecmp/qos block sip access level taken from global configuration"));

        if(descrPtr->isIPv4)
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(devObjPtr), 22, 6, &sipAccessLevel);
        }
        else if(descrPtr->isFcoe)
        {
            smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 23, 6, &sipAccessLevel);
        }
        else
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(devObjPtr), 23, 6, &sipAccessLevel);
        }
    }
    else
    {
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_SIP_ACCESS_LEVEL;
        sipAccessLevel = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,sipNextHopEntryPtr,sipNextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(sipNextHopEntryPtr[2], 14, 3);
    }
    __LOG(("Unicast IPv4/6 Packet SIP Access level[%d] \n", sipAccessLevel));


    smemRegFldGet(devObjPtr, SMEM_CHT2_ROUTER_ACCESS_MATRIX_REG(devObjPtr, sipAccessLevel),
                         2 * dipAccessLevel, 2, &fldVal);
    switch (fldVal)
    {
        case ACCESS_MATRIX_SOFT_DROP:
            __LOG(("ACCESS_MATRIX_SOFT_DROP \n"));
             cntrlPcktInfo->ipvAccessCmd = HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E;
            break;
        case ACCESS_MATRIX_HARD_DROP:
            __LOG(("ACCESS_MATRIX_HARD_DROP \n"));
             cntrlPcktInfo->ipvAccessCmd = HW_ROUTER_PACKET_COMMAND_HARD_DROP_E;
            break;
        default:
            __LOG(("ACCESS_MATRIX_FORWARD \n"));
            break;
    }

    logRouterPacketCommand(devObjPtr, NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvAccessCmd));
}

/*******************************************************************************
*   snetCht2L3iGetNextHop
*
* DESCRIPTION:
*       Apply routing actions on the frames descriptor
*
* INPUTS:
*       devObjPtr             - pointer to device object.
*       descrPtr              - pointer to frame data buffer Id
*       routeIndexPtr         - pointer to the matching table.
*       fdbDipLookupInfoPtr   - pointer to fdb dip lookup info
*       ipSecurChecksInfoPtr  - pointer to routing security checks information
*
* OUTPUTS:
*       ipSecurChecksInfoPtr  - pointer to routing security checks information
*       cntrlPcktInfo         - pointer to control packet info
*       localDescrPtr         - pointer to local router engine next hop information
*       mll_selectorPtr       - pointer to selector internal external mll for ipv6 MC
*       isSipLookupPerformed  - is SIP Lookup performed
*
* RETURN:
*
* COMMENTS:
*       [1] 8.6.2 Applying the Action on the Packet
*
*******************************************************************************/
static GT_VOID snetCht2L3iGetNextHop
(
    IN    SKERNEL_DEVICE_OBJECT               *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN    GT_U32                              *routeIndexPtr,
    IN    SNET_LION3_FDB_ROUTE_ENTRY_INFO     *fdbDipLookupInfoPtr,
    INOUT SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr,
    OUT   SNET_CHEETAH2_L3_CNTRL_PACKET_INFO  *cntrlPcktInfo,
    OUT   SKERNEL_FRAME_CHEETAH_DESCR_STC     *localDescrPtr,
    OUT   GT_U32                              *mll_selectorPtr,
    OUT   GT_BOOL                             isSipLookupPerformed
)
{
    DECLARE_FUNC_NAME(snetCht2L3iGetNextHop);

    GT_U32 *primaryNextHopEntryPtr = NULL; /* primary (DIP or SIP for ipmc) associated next hop entry*/
    GT_U32 *sipNextHopEntryPtr     = NULL; /* SIP associated next hop entry */
    GT_U32 fldVal;                  /* field value */
    GT_U32 fldValue1;               /* field value */
    GT_U32 fldValue2;               /* field value */
    GT_U32 ipHeaderTotalLength;     /* ip header total/payload length test */
    GT_U32 idx_scope;               /* index scope check */
    GT_U32 idx_scope_ext;           /* index external scope check */
    GT_U32 dip_scope;               /* scope level found for destination IP*/
    GT_U32 sip_scope;               /* scope level found for source IP*/
    GT_U32 regAddr;                 /* register address value */
    GT_U32 sipAddr;                 /* register address value rpf check*/
    GT_U32 scopeReg;                /* scope register address value */
    GT_U32 scope_value;             /* scope value         */
    GT_U32 cpuCodeIndex;            /* cpu code index */
    GT_U32 bypassTtlCheck=GT_TRUE;  /* bypass ttl check   */
    GT_U32 icmpEn;                  /* enable ICMP redirect mirroring */
    GT_U32 icmpPacketCmd;           /* packet cmd */
    GT_U32 nextHopVid;              /* next hop vid */
    SGT_MAC_ADDR_TYP  monitorMacSa; /* MAC SA value        */
    GT_U32  sip_check_fld;          /* field for sip special service check */
    GT_U32  num_of_scope_check;     /* number of SCOPE checking */
    GT_U32 matchInxAgeTable;        /* index in  Age Bits  table */
    GT_U32 offsetInxAgeTable;       /* offset in  AgeBits  table */
    GT_U32 arpEntryIndex;           /* ARP entry index */
    GT_U32 idx_match;               /* 0 - for unicast search 1 for multicast*/
    GT_U32  offset = 0;             /* if borderCrossed==GT_FALSE the offset=4, if borderCrossed==GT_TRUE the offset=0*/
    GT_U32  entryVid;               /* vid from the entry : for UC - nextHopVid , for MC - MC RPF Vid */
    GT_BIT  isIpv6 = (descrPtr->isIPv4 || descrPtr->isFcoe) ? 0 : 1;
    GT_U32  vid1UnicastRoutingEn = 0;  /* Enable VID1 unicast routing */
    GT_U32  nextHopVid1 = 0;               /*SIP5: next hop vid 1 */
    GT_U32  mll_selector;/*value from the register of selector internal/external mll for ipv6 MC*/
    GT_U32 dipAccessLevel;
    GT_BOOL isEntryRouteCmd = GT_FALSE;
    GT_U32  nextHopFieldName;
    GT_U32  nextHopIndex;

    __LOG_PARAM(routeIndexPtr[0]);
    __LOG_PARAM(routeIndexPtr[1]);

    if (0 == fdbDipLookupInfoPtr->isValid)
    {
        if ((descrPtr->ipm == 1)  && (routeIndexPtr[1] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS))
        {
            __LOG(("multicast SIP lookup packet use NH entry in index[%d]",routeIndexPtr[1]));
            idx_match = 1;
        }
        else
        {
            __LOG(("DIP lookup packet use NH entry in index[%d]",routeIndexPtr[0]));
            idx_match = 0;
        }

        nextHopIndex = routeIndexPtr[idx_match];
        regAddr = SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(devObjPtr,nextHopIndex);
        primaryNextHopEntryPtr = smemMemGet(devObjPtr, regAddr);
    }
    else
    {
        idx_match = 0;
        nextHopIndex = 0;
    }

    /* Enable Application specific CPU Code assignment */
    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE_EN;
    localDescrPtr->appSpecCpuCode =
            fdbDipLookupInfoPtr->isValid ? 0 :
            SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
            SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
            SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 10, 1);

    __LOG_PARAM(localDescrPtr->appSpecCpuCode);

    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_BC_TRAP_MIRROR_EN;
    if (descrPtr->arp)
    {
        GT_U32  arpBcMirrTrapEn =
                fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->arpBcTrapMirrorEn :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 9, 1);

        __LOG(("ARP BC trap/mirror enable = [%d] in NHE entry\n", arpBcMirrTrapEn));

        nextHop_processArp(devObjPtr, descrPtr, arpBcMirrTrapEn, localDescrPtr);
        return;
    }

    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_EVLAN;
    entryVid =
                fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->nextHopEvlan :
                   SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                   SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                   SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 20, 12);

    __LOG_PARAM(entryVid);

       /* temporary assign hard drop for localDescrPtr->packetCmd , until we  have
       command resolution , because 'localDescrPtr->packetCmd' is
       'packet command' (bridge style) , but the register hold 'router enginge' style
       */
    __LOG(("temporary assign hard drop for localDescrPtr->packetCmd \n"));
    localDescrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;

    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_PACKET_CMD;
    /* packet command assignment */
    cntrlPcktInfo->ipvRouteEntry03Cmd =
                fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->nextHop_hwRouterPacketCommand :
                   SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                   SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                   SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 0, 3);
    logRouterPacketCommand(devObjPtr,
        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvRouteEntry03Cmd));

    isEntryRouteCmd = ((cntrlPcktInfo->ipvRouteEntry03Cmd == HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E) ||
                      (cntrlPcktInfo->ipvRouteEntry03Cmd == HW_ROUTER_PACKET_COMMAND_ROUTE_E));
    if((descrPtr->ipm == 0) && isEntryRouteCmd)
    {
        nextHopVid = entryVid;
        /* Router Source VID assignment */
        localDescrPtr->eVid = nextHopVid;
        __LOG_PARAM(localDescrPtr->eVid);
    }

    /* CPU code assignment */
    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_CPU_CODE_INDEX;
    cpuCodeIndex =
                    fdbDipLookupInfoPtr->isValid ? 0 :
                   SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                   SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 4, 2) ;
    if (isIpv6 == 0)
    {
       localDescrPtr->cpuCode = (descrPtr->ipm) ?
                       SNET_CHT_IPV4_MC_ROUTE0 + cpuCodeIndex :
                       SNET_CHT_IPV4_UC_ROUTE0 + cpuCodeIndex;
    }
    else   /* ipv6 packet */
    {
       localDescrPtr->cpuCode = (descrPtr->ipm) ?
                       SNET_CHT_IPV6_MC_ROUTE0 + cpuCodeIndex :
                       SNET_CHT_IPV6_UC_ROUTE0 + cpuCodeIndex;
    }

    __LOG_PARAM(localDescrPtr->cpuCode);

    if(0 == fdbDipLookupInfoPtr->isValid && SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Get "Enable VID1 unicast routing" */
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 7, 1, &vid1UnicastRoutingEn);

        if(vid1UnicastRoutingEn)
        {
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_NEXT_HOP_VID1;

            nextHopVid1 =
                   SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                   SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    snetFieldValueGet(primaryNextHopEntryPtr, 9, 12);
            localDescrPtr->vid1 = nextHopVid1;
            __LOG_PARAM(nextHopVid1);
        }
    }

    if(isEntryRouteCmd)
    {
        if (descrPtr->ipm == 0)
        {
            __LOG(("IPv4/6 Unicast Egress Interface"));

            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_USE_VIDX;

            /*  The redirect info */
            localDescrPtr->useVidx =
                         fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->useVidx :
                         SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                         SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                         SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 0, 1);
            __LOG_PARAM(localDescrPtr->useVidx);

            if (localDescrPtr->useVidx)
            {
                nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_EVIDX;
                localDescrPtr->eVidx =
                        fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->evidx :
                        SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                        SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 1, 13);
                __LOG_PARAM(localDescrPtr->eVidx);
            }
            else
            {
                nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TARGET_IS_TRUNK;
                localDescrPtr->targetIsTrunk =
                        fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->trgIsTrunk :
                        SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                        SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 1, 1);
                __LOG_PARAM(localDescrPtr->targetIsTrunk);

                if (localDescrPtr->targetIsTrunk)
                {
                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_TRUNK_ID;
                    localDescrPtr->trgTrunkId =
                            fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->trgTrunkId :
                            SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                            SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                            SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 6, 7);
                    __LOG_PARAM(localDescrPtr->trgTrunkId);
                }
                else
                {
                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_EPORT;
                    localDescrPtr->trgEPort =
                            fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->trgEport :
                            SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                            SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                            SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 2, 6);
                    __LOG_PARAM(localDescrPtr->trgEPort);

                    /* call after setting trgEPort */
                    SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,localDescrPtr,ipvx);

                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TRG_DEV;
                    localDescrPtr->trgDev =
                            fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->trgDev :
                            SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                            SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                            SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 8, 5);
                    __LOG_PARAM(localDescrPtr->trgDev);

                    if ( localDescrPtr->trgEPort == SNET_CHT_CPU_PORT_CNS )
                    {
                        cntrlPcktInfo->isRoutEntryForwardToCpu = 1;
                        __LOG_PARAM(cntrlPcktInfo->isRoutEntryForwardToCpu);
                    }
                }
           }

            /* indication for Tunnel Start */
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_START_OF_TUNNEL;
            localDescrPtr->tunnelStart =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->startOfTunnel :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 17, 1);
            __LOG_PARAM(localDescrPtr->tunnelStart);

            if (localDescrPtr->tunnelStart)
            {
                /* check isNat */
                if((devObjPtr->supportNat44 && localDescrPtr->isIPv4) || /* bc2 b0,bobK, unicast only, ipv4 packet */
                   (devObjPtr->supportNat66 && isIpv6)) /* bobK, unicast only, ipv6 packet */
                {
                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TS_IS_NAT;
                    localDescrPtr->isNat =
                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName);
                    if(localDescrPtr->isNat)
                    {
                        /* Note: The combination inDesc<TunnelStart> = 1 and inDesc<DoNAT> ==1 is not supported */
                        localDescrPtr->tunnelStart = 0;

                        __LOG(("The 'tunnelStart' is set to 0 due to NAT entry \n"));
                        __LOG(("NAT is triggered \n"));
                    }
                }
            }

            if(localDescrPtr->tunnelStart)
            {
                nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_PTR;
                __LOG(("indicate this action is a TunnelStart \n"));
                localDescrPtr->tunnelPtr =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->tunnelPtrValid :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 19, 12);
                __LOG_PARAM(localDescrPtr->tunnelPtr);

                if (SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_TYPE;
                     /* <tunnel type> although file is 'reserved' still the device look at it */
                    localDescrPtr->tunnelStartPassengerType =
                        fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->tunnelType :
                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName);
                }
                else
                {
                     /* for IP packets routed to tunnel start interface, the tunnel
                       passenger type is always OTHER (meaning IP or MPLS) */
                    localDescrPtr->tunnelStartPassengerType = 1;
                }
                __LOG_PARAM(localDescrPtr->tunnelStartPassengerType);
            }
            else
            if(localDescrPtr->isNat == 1)
            {
                nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_TUNNEL_PTR;/* tunnel Ptr hold the NAT Ptr */
                __LOG(("indicate this action is a NAT \n"));
                localDescrPtr->arpPtr =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->tunnelPtrValid :
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName);
            }
            else
            {
                nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_PTR;
                __LOG(("indicate this action is a ARP \n"));
                localDescrPtr->arpPtr =
                        fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->arpPtrValid :
                        SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                        SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 19, 13);
                __LOG_PARAM(localDescrPtr->arpPtr);
            }
        }
        else
        {
            if(isIpv6)
            {
                __LOG(("IPv6 Multicast MLL Interface"));

                /* ipv6 'external MLL' pointer */
                nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER;
                localDescrPtr->mllexternal = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                                              SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                                              SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 0, 13);
                if( SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_MC_EXTERNAL_MLL_POINTER_MSBITS;
                    /* additional 3 bits */
                    fldVal =
                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName);
                    localDescrPtr->mllexternal |= fldVal << 13;
                }

                __LOG_PARAM(localDescrPtr->mllexternal);
            }
            else
            {
                __LOG(("IPv4 Multicast MLL Interface"));
            }

            /* ipv4 mll or ipv6 'internal MLL' pointer */
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_INTERNAL_MLL_PTR;
            localDescrPtr->mll = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                             SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                             SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 19, 13);
            __LOG_PARAM(localDescrPtr->mll);
        }

        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DEC_TTL_OR_HOP_COUNT;
        descrPtr->decTtl =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->decTtlOrHopCount :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 18, 1);

        localDescrPtr->decTtl = descrPtr->decTtl;
        __LOG_PARAM(localDescrPtr->decTtl);

        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION;
        bypassTtlCheck = fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->bypassTtlOptionsOrHopExtensionForIpv4Packets :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    (SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 3, 1));

        __LOG_PARAM(bypassTtlCheck);

        /*****  bypass TTL v.4 Hop Limit decrement **********/
        if (bypassTtlCheck == 0 && descrPtr->isFcoe == 0)
        {
            if (descrPtr->isIPv4)
            {   /* TTL decrement */
                localDescrPtr->ttl = descrPtr->l3StartOffsetPtr[8];
                __LOG(("TTL taken from the packet \n"));
                regAddr = SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr);
            }
            else
            {   /* Hop by hop decrement */
                localDescrPtr->ttl = descrPtr->l3StartOffsetPtr[7];
                __LOG(("Hop by hop taken from the packet \n"));
                regAddr = SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr);
            }

            __LOG_PARAM(localDescrPtr->ttl);

            /* If the TTL/Hop-Limit has reached 0 or TTL/Hop-Limit has reached 1
               and decrement the TTL/Hop-Limit is enable, a TTL/Hop-Limit Exceeded exception occurs */
            if ((localDescrPtr->ttl == 0) || ((localDescrPtr->ttl == 1) && (descrPtr->decTtl)))
            {
                smemRegFldGet(devObjPtr, regAddr, 21, 3, &fldVal);
                cntrlPcktInfo->ipvHopLimitExceededFailCmd = fldVal;
                logRouterPacketCommand(devObjPtr,
                    NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvHopLimitExceededFailCmd));
            }
        }
    }

    /* Ingress Mirror To Analyzer Index */
    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_INGRESS_MIRROR_TO_ANALYZER_INDEX;
    fldVal =
            fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->ingressMirrorToAnalyzerIndex :
                SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 6, 1);

    if (fldVal)
    {
        localDescrPtr->rxSniff = 1;

        __LOG(("Next Hop mirror analyzer index \n"));
        snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, localDescrPtr, fldVal);
    }
    __LOG_PARAM(localDescrPtr->rxSniff);

    if(0 == fdbDipLookupInfoPtr->isValid)
    {
        /* ICMP redirect command */
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ICMP_REDIRECT_EXCEP_MIRROR;
        icmpEn = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                 SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                 SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 2, 1);
        __LOG_PARAM(icmpEn);

        if (icmpEn)
        {
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_PACKET_CMD;
            icmpPacketCmd  =
                SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                (SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[0], 0, 3)); /*packet cmd */
            if (descrPtr->eVid ==  entryVid &&
                icmpPacketCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E)
            {
                cntrlPcktInfo->ipvIcmpRedirectFailCmd = HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E;
                logRouterPacketCommand(devObjPtr,
                    NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvIcmpRedirectFailCmd));
            }
        }
    }

    if(!descrPtr->ipm)  /* unicast packet */
    {
        if(isSipLookupPerformed == GT_TRUE)
        {
            GT_U32 sipNextHopIndex = 0;

            if(routeIndexPtr[1] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
            {
                if(fdbDipLookupInfoPtr->isValid)
                {
                    skernelFatalError("fdb routing and sip no match - should never happen \n");
                }
                else
                {
                    sipNextHopIndex = routeIndexPtr[0];
                }
            }
            else
            {
                sipNextHopIndex = routeIndexPtr[1];
            }

            __LOG(("unicast packet"));

            sipAddr = SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(devObjPtr, sipNextHopIndex);

            __LOG_PARAM(sipAddr);

            sipNextHopEntryPtr = smemMemGet(devObjPtr, sipAddr);


            if(fdbDipLookupInfoPtr->isValid)
            {
                /* ???  fdb lookup dip entry has no field ipvSipFilterCmd */
            }
            else
            {
                __LOG(("Unicast SIP Filtering"));
                snetCht2SipFiltering(devObjPtr, descrPtr, ipSecurChecksInfoPtr,
                                        primaryNextHopEntryPtr, nextHopIndex, cntrlPcktInfo);
            }

            __LOG(("Unicast Access Matrix Based Filtering"));
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_DIP_ACCESS_LEVEL;
            dipAccessLevel =
                    fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->dipAccessLevel :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 11, 3);
            __LOG(("Unicast IPv4/6 Packet DIP Access level[%d] \n", dipAccessLevel));


            snetCht2AccessMatrixFiltering(devObjPtr, descrPtr, ipSecurChecksInfoPtr,
                            sipNextHopEntryPtr, sipNextHopIndex , dipAccessLevel, cntrlPcktInfo);


            __LOG(("Unicast IPv4/6 Packet RPF check"));
            snetCht3URpfCheck(devObjPtr, descrPtr, routeIndexPtr, ipSecurChecksInfoPtr,
                                        vid1UnicastRoutingEn, nextHopVid1, cntrlPcktInfo);


            if(fdbDipLookupInfoPtr->isValid)
            {
                __LOG(("SIP/SA check not relevant for fdb routing\n"));
            }
            else
            {
                __LOG(("start SIP/SA check"));
                if (descrPtr->isIPv4)
                {
                    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 11, 1, &sip_check_fld);
                }
                else if(descrPtr->isFcoe)
                {
                    smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 3, 1, &sip_check_fld);
                }
                else
                {
                    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 13, 1, &sip_check_fld);
                }

                __LOG_PARAM(sip_check_fld);
                __LOG_PARAM(descrPtr->l2Valid);

                if ((ipSecurChecksInfoPtr->unicastSipSaCheck && descrPtr->l2Valid)  &&
                    ((descrPtr->ipServiceFlag == GT_TRUE) || (( descrPtr->ipServiceFlag == GT_FALSE && sip_check_fld))))
                {
                    nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_ARP_PTR;
                    arpEntryIndex = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                                        SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                                        SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[1], 19, 13);

                    __LOG(("Get ARP address for SA mismatch check"));
                    snetChtHaArpTblEntryGet(devObjPtr, arpEntryIndex, &monitorMacSa);

                    if (!SGT_MAC_ADDR_ARE_EQUAL(monitorMacSa.bytes, descrPtr->macSaPtr))
                    { /* The command assigned to routed IPv6 packets  */
                        __LOG(("SA mismatch detected \n"));
                        regAddr = (isIpv6 == 0) ?
                               SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(devObjPtr):
                               SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(devObjPtr);
                        smemRegGet(devObjPtr,regAddr,&fldVal);
                        cntrlPcktInfo->ipSaMismatchCmd = SMEM_U32_GET_FIELD(fldVal, 13, 3);
                        logRouterPacketCommand(devObjPtr,
                            NAME_AND_VALUE_MAC(cntrlPcktInfo->ipSaMismatchCmd));
                    }
                } /* End  SIP/SA   CHECK  */
            }
        }
    } /* Unicast IPv4/6 checks   */
    else
    {/* Multicast IPv4/6 Packet RPF check */
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_ING_VLAN_CHECK;
        fldVal = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                 SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                 SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 18, 1);

        __LOG(("Multicast IPv4/6 Packet RPF check enabled[%d] \n",
            fldVal));

        if(1 == fldVal && (descrPtr->eVid != entryVid))
        {
            __LOG_PARAM(descrPtr->eVid != entryVid);

            cntrlPcktInfo->isMcRpfFail = 1;

            nextHop_mcRpfCheckFailed(devObjPtr, descrPtr, primaryNextHopEntryPtr, nextHopIndex,
                                     cntrlPcktInfo, &ipSecurChecksInfoPtr->mcRpfFailCmdFromMll);
        }/* check eVid */
        else
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && (2 == fldVal) )
        {
            __LOG(("Bidirectional Tree Check\n"));
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MC_RPF_EVLAN_MRST_ID;
            fldVal =
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                         SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                         snetFieldValueGet(primaryNextHopEntryPtr, 22, 6);
            __LOG(("Multicast Routing Shared Tree index: %d\n", fldVal));

            if(descrPtr->eArchExtInfo.ipvxIngressEVlanTablePtr)
            {
                fldVal = snetFieldValueGet(descrPtr->eArchExtInfo.ipvxIngressEVlanTablePtr, 2+fldVal/*shared tree index*/, 1);

                if(0 == fldVal)
                {
                    __LOG(("Multicast Routing Shared Tree Status - BLOCKED\n"));

                    cntrlPcktInfo->isMcRpfBidirFail = 1;

                    nextHop_mcRpfCheckFailed(devObjPtr, descrPtr, primaryNextHopEntryPtr, nextHopIndex,
                                             cntrlPcktInfo, &ipSecurChecksInfoPtr->mcRpfFailCmdFromMll);
                }
            }
        }/* End  RPF   CHECK  */
    }

    if(descrPtr->isFcoe == 0)
    {
        /* IPv4/6 MTU check */
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_MTU_INDEX;
        fldVal = fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->mtuIndex :
                 SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                 SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                 SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 6, 3);
        __LOG(("IPv4/6 Packet MTU profile index[%d] \n",
            fldVal));

        /* IPv4/6 Packet MTU profile index */
        regAddr = SMEM_CHT2_ROUTER_MTU_CONFIGURATION_REG(devObjPtr, (fldVal / 2));
        smemRegFldGet(devObjPtr, regAddr, 14 * (fldVal % 2), 14, &fldValue2);
        ipHeaderTotalLength = descrPtr->ipxLength;

        __LOG(("the MAX MTU is[%d] and actual 'ip Header Total Length' is[%d] \n",
            fldValue2,ipHeaderTotalLength));

        if (fldValue2 < ipHeaderTotalLength) /* fetch route table entry */
        {
            /* The command assigned to routed IPv4/6 packets */
            __LOG(("MTU exceeded for ipv%d %s with <DF>=%d \n",
                (descrPtr->isIPv4 ? 4 : 6),
                (descrPtr->ipm ? "MC" : "UC") ,
                (descrPtr->ipv4DontFragmentBit)));

            /* The command assigned to routed IPv4/6 packets */
            if(SKERNEL_IS_CHEETAH3_DEV(devObjPtr) &&
               descrPtr->isIPv4 && descrPtr->ipv4DontFragmentBit)
            {
                regAddr = SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(devObjPtr);
                smemRegFldGet(devObjPtr,regAddr, (descrPtr->ipm ? 16 : 19), 3, &fldVal);
            }
            else
            {
                regAddr = descrPtr->isIPv4 ?
                                SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr) :
                                SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr);

                smemRegFldGet(devObjPtr,regAddr, (descrPtr->ipm ? 0 : 3), 3, &fldVal);
            }
            cntrlPcktInfo->ipvRouteMtuExcceededFailCmd = fldVal;
            logRouterPacketCommand(devObjPtr,
                NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvRouteMtuExcceededFailCmd));
        } /* End ... MTU ... CHECK */
    }
    else
    {
        __LOG(("MTU check not relevant to FCOE \n"));
    }

    if(isIpv6)/*ipv6*/
    {
        /*  IPv6 SCOPE check */
        nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_SCOPE_CHECK;
        fldVal =
                fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->ipv6ScopeCheck :
                    SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                    SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 1, 1);
        __LOG(("IPv6 Packet scope enabled[%d] \n",
            fldVal));

        /*  IPv6 Packet scope index*/
        /* scope check (not relevant for fcoe) */
        if (fldVal)
        {
            __LOG(("IPv6 Packet scope index \n"));
            if (descrPtr->ipm)
            {
                num_of_scope_check = 1;
                dip_scope = ipSecurChecksInfoPtr->ipv6MulticastGroupScopeLevel;
                sip_scope = SNET_CHT2_SCOPE;
            }
            else
            {
                num_of_scope_check = 2;
                dip_scope = SNET_CHT2_SCOPE;
                sip_scope = SNET_CHT2_SCOPE;
            }

            __LOG_PARAM(num_of_scope_check);
            __LOG_PARAM(dip_scope);
            __LOG_PARAM(sip_scope);

            for (idx_scope_ext = 0 ; idx_scope_ext < num_of_scope_check ; idx_scope_ext++)
            {
                for (idx_scope = 0 ; idx_scope <= SNET_CHT2_SCOPE ; idx_scope++)
                {
                    regAddr = SMEM_CHT2_ROUTER_SCOPE_PREFIX_TBL_MEM(devObjPtr,idx_scope);
                    smemRegGet(devObjPtr,regAddr,&fldVal);
                    scopeReg = SMEM_CHT2_ROUTER_UNICAST_SCOPE_LEVEL_TBL_MEM(devObjPtr,idx_scope);

                    fldValue1 = SMEM_U32_GET_FIELD(fldVal,0,16);  /* scope prefix      */
                    fldValue2 = SMEM_U32_GET_FIELD(fldVal,16,16); /* scope prefix mask */
                    scope_value = idx_scope_ext ?
                                       (descrPtr->dip[0] >> 16):
                                       (descrPtr->sip[0] >> 16);

                    __LOG_PARAM(idx_scope_ext);
                    __LOG_PARAM(idx_scope);
                    __LOG_PARAM(fldValue1);
                    __LOG_PARAM(fldValue2);
                    __LOG_PARAM(scope_value);

                    if ((scope_value & fldValue2) == fldValue1)
                    {
                        smemRegFldGet(devObjPtr, scopeReg, 0, 2, &fldValue2);
                        __LOG_PARAM(fldValue2);
                        if (idx_scope_ext)
                            dip_scope = fldValue2;
                        else
                            sip_scope = fldValue2;
                        break;
                    }
                }
            }

            /*  IPv6 Dest Site ID  */
            nextHopFieldName = SMEM_SIP5_IPVX_NEXT_HOP_TABLE_FIELDS_IPV6_DEST_SITE_ID;
            fldValue2 =
                fdbDipLookupInfoPtr->isValid ? fdbDipLookupInfoPtr->ipv6DestSiteId :
                SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                SMEM_SIP5_IPVX_NEXT_HOP_FIELD_GET(devObjPtr,primaryNextHopEntryPtr, nextHopIndex,nextHopFieldName) :
                SMEM_U32_GET_FIELD(primaryNextHopEntryPtr[2], 0, 1);

            /* Get source Site ID from VLAN entry according to eVid */

            /* if the border was not crossed the offset=4,
               if the border was crossed the offset=0 */
            if(descrPtr->ingressVlanInfo.ipV6SiteID == fldValue2)
            {
                __LOG(("source Site ID == Dest Site ID"));
                offset = 4;
            }
            else
            {
                offset = 0;
            }

            regAddr = descrPtr->ipm ?
                          SMEM_CHT2_ROUTER_MULTICAST_SCOPE_COMMAND_TBL_MEM(devObjPtr,sip_scope):
                          SMEM_CHT2_ROUTER_UNICAST_SCOPE_COMMAND_TBL_MEM(devObjPtr,sip_scope);

            smemRegFldGet(devObjPtr, regAddr, dip_scope * 8 + offset, 3, &fldVal);
            cntrlPcktInfo->ipv6ScopeFailCmd = fldVal;

            logRouterPacketCommand(devObjPtr,
                NAME_AND_VALUE_MAC(cntrlPcktInfo->ipv6ScopeFailCmd));

            if (descrPtr->ipm)
            {/* bit check for  external internal mll */
                smemRegFldGet(devObjPtr, regAddr, dip_scope * 8 + offset + 3, 1, &mll_selector);
                if(mll_selector)
                {
                    __LOG(("MLL selector : external MLL (use mllexternal) \n"));
                    *mll_selectorPtr = SKERNEL_MLL_SELECT_EXTERNAL_E;
                }
                else
                {
                    __LOG(("MLL selector : internal MLL (use mll) \n"));
                    *mll_selectorPtr = SKERNEL_MLL_SELECT_INTERNAL_E;
                }
            }

        } /* End scope   CHECK */
    }

    if(isEntryRouteCmd)
    {
        /* Router Source ID assignment */
        if (descrPtr->ipm == 0) /*  unicast packet */
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* Router Global Control1, 19:8 RouterUCSourceID */
                smemRegFldGet(devObjPtr, SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr),8,12,&fldVal);
            }
            else
            {
                smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr),1,5,&fldVal);
            }

            localDescrPtr->sstId = fldVal;
            __LOG_PARAM(localDescrPtr->sstId);
        }
    }

    __LOG(("IPv4/6 All Zero SIP check"));
    smemRegGet(devObjPtr, SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr), &fldVal);
    if (isIpv6 == 0)
    {
        __LOG_PARAM(descrPtr->sip[0]);
        if (descrPtr->sip[0] == 0)   /*  IP ADDRESS 0.0.0.0 */
        {
            if (descrPtr->ipm == 0) /*  unicast packet */
            {
                if (SMEM_U32_GET_FIELD(fldVal, 3, 1))
                {
                    cntrlPcktInfo->ipvAllZeroCmd = HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E ;
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvAllZeroCmd));
                }
            }
            else
            {
                if (SMEM_U32_GET_FIELD(fldVal, 4, 1))
                {
                    cntrlPcktInfo->ipvAllZeroCmd  =  HW_ROUTER_PACKET_COMMAND_BRIDGE_E;
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvAllZeroCmd));
                }
            }
        }
    }
    else /*ipv6*/
    {
        __LOG_PARAM(descrPtr->sip[0]);
        __LOG_PARAM(descrPtr->sip[1]);
        __LOG_PARAM(descrPtr->sip[2]);
        __LOG_PARAM(descrPtr->sip[3]);
        if  ((descrPtr->sip[0] == 0 ) &&
            (descrPtr->sip[1] == 0 ) &&
            (descrPtr->sip[2] == 0 ) &&
            (descrPtr->sip[3] == 0 ))
        {
            if (descrPtr->ipm == 0) /*  unicast packet */
            {
                if ((SMEM_U32_GET_FIELD(fldVal, 1, 1)))
                {
                    cntrlPcktInfo->ipvAllZeroCmd =  HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E ;
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvAllZeroCmd));
                }
            }
            else
            {
                 if ((SMEM_U32_GET_FIELD(fldVal, 2, 1)))
                {
                    cntrlPcktInfo->ipvAllZeroCmd =  HW_ROUTER_PACKET_COMMAND_SOFT_DROP_E ;
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvAllZeroCmd));
                }
            }
        }
    }

    /* Process Qos fields or Vid1 */
    if(vid1UnicastRoutingEn)
    {
        __LOG(("vid1UnicastRoutingEn : in this mode there are NO QOS changes because VID1 muxed with of qos fields \n"));
    }
    else
    {
        nextHop_processQos(devObjPtr, descrPtr, primaryNextHopEntryPtr, nextHopIndex, localDescrPtr, fdbDipLookupInfoPtr);
    }

    if(isEntryRouteCmd)
    {
        if(isIpv6 == 0)
        {
            __LOG(("for ipv4 : Bypass TTL and exception check Enabled [%d]",
                bypassTtlCheck));
        }
        else /*ipv6*/
        {
            __LOG(("for ipv6 : Bypass of Hop Limit, Hop-by-Hop header exception check Enabled [%d]",
                bypassTtlCheck));
        }
    }

    if(isEntryRouteCmd)
    {
        if(bypassTtlCheck == GT_FALSE)/* no bypass */
        {
            regAddr = (isIpv6 == 0) ?
                        SMEM_CHT2_L3_ROUTE_CONTROL0_IPV4_REG(devObjPtr) :
                        SMEM_CHT2_L3_ROUTE_CONTROL0_IPV6_REG(devObjPtr);

            if (descrPtr->ipm)
            {
                smemRegFldGet(devObjPtr,regAddr, 6, 3, &fldVal);
            }
            else
            {
                smemRegFldGet(devObjPtr,regAddr, 9, 3, &fldVal);
            }

            if(isIpv6 == 0)
            {
                /*The command assigned to IPv4 UC/MC packets that are to be routed and
                contain an IPv4 header with options and for which the TTL and
                Options check is not bypassed (Route Entry <Bypass TTLand OptionsCheck> = 0)

                IPv4 UC Options Command 9
                IPv4 MC Options Command 6
                */
                __LOG_PARAM(descrPtr->ipv4HeaderOptionsExists);

                if(descrPtr->ipv4HeaderOptionsExists)
                {
                    cntrlPcktInfo->ipvOptionHbhOptionFailCmd =  fldVal;
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvOptionHbhOptionFailCmd));
                }
            }
            else
            {
                /*The command assigned to IPv6 UC/MC packets that are to be routed,
                contain a Hop-By-Hop Options header,and a TTL and
                Options check is not bypassed (Route Entry<Bypass TTLand OptionsCheck> = 0).

                IPv6 UC HBH Options Command 9
                IPv6 MC HBH Options Command 6
                */
                __LOG_PARAM(descrPtr->ipv4HeaderOptionsExists);
                __LOG_PARAM(descrPtr->isIpV6EhExists);

                if(descrPtr->isIpV6EhHopByHop)
                {
                    cntrlPcktInfo->ipvOptionHbhOptionFailCmd =  fldVal;
                    /*  HOP by HOP Header Exception */
                    __LOG(("HOP by HOP Header Exception Command[%d] \n",
                        cntrlPcktInfo->ipvOptionHbhOptionFailCmd));
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvOptionHbhOptionFailCmd));
                }
                else if (!descrPtr->isIpV6EhHopByHop && descrPtr->isIpV6EhExists)
                {
                    /* NON HOP BY HOP */
                    regAddr = SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(devObjPtr);

                    smemRegGet(devObjPtr, regAddr, &fldValue2);
                    cntrlPcktInfo->ipvOptionHbhOptionFailCmd =
                                        (descrPtr->ipm) ?
                                SMEM_U32_GET_FIELD(fldValue2, 20, 3) :
                                SMEM_U32_GET_FIELD(fldValue2, 17, 3);
                    /* Non HOP by HOP Header Exception */
                    __LOG(("Non HOP by HOP Header Exception Command[%d] \n",
                        cntrlPcktInfo->ipvOptionHbhOptionFailCmd));
                    logRouterPacketCommand(devObjPtr,
                        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvOptionHbhOptionFailCmd));
                }
            }
        }
    }

    /* Route Refresh Aging */
    smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 0, 1, &fldVal);
    __LOG(("Route Refresh Aging En [%d]\n",
        fldVal));
    if (fldVal)
    {
      /* For L3 multicast routing, two Lookups are performed, so we need
         to refresh AGE bit for route entry associated with S entry */
        if (routeIndexPtr[0] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
        {
            matchInxAgeTable = routeIndexPtr[0] / 32;
            offsetInxAgeTable = routeIndexPtr[0] % 32;

            regAddr = SMEM_CHT2_ROUTER_NEXT_HOP_AGE_BITS_TBL_MEM(devObjPtr,
                                                             matchInxAgeTable);
            /* Set AGE bit */
            __LOG(("refresh AGE bit for route entry associated with DIP entry \n"));
            smemRegFldSet(devObjPtr, regAddr, offsetInxAgeTable, 1, 1);
        }

        if (routeIndexPtr[1] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
        {
            matchInxAgeTable = routeIndexPtr[1] / 32;
            offsetInxAgeTable = routeIndexPtr[1] % 32;

            regAddr = SMEM_CHT2_ROUTER_NEXT_HOP_AGE_BITS_TBL_MEM(devObjPtr,
                                                             matchInxAgeTable);
            /* Set AGE bit */
            __LOG(("refresh AGE bit for route entry associated with SIP entry \n"));
            smemRegFldSet(devObjPtr, regAddr, offsetInxAgeTable, 1, 1);
        }
    }

    localDescrPtr->routed = 1;
    localDescrPtr->doRouterHa = 1;

    __LOG_PARAM(localDescrPtr->routed);
    __LOG_PARAM(localDescrPtr->doRouterHa);
}

/*******************************************************************************
*   snetCht2IntraRouterPktCmdResolution
*
* DESCRIPTION:
*       resolve from old and current router packet command the new router packet command
*
* INPUTS:
*       prevCmd - previous router packet command
*       currCmd - current router packet  command
*
* OUTPUTS:
*       none
*
* RETURN:
*       resolved new router packet command
*
* COMMENTS:
*
*******************************************************************************/
static HW_ROUTER_PACKET_COMMAND_E snetCht2IntraRouterPktCmdResolution
(
    IN HW_ROUTER_PACKET_COMMAND_E prevCmd,
    IN HW_ROUTER_PACKET_COMMAND_E currCmd
)
{
    if(prevCmd > HW_ROUTER_PACKET_COMMAND_BRIDGE_E ||
       currCmd > HW_ROUTER_PACKET_COMMAND_BRIDGE_E)
    {
        /* fatal error */
        skernelFatalError("snetCht2IntraRouterPktCmdResolution: bad parameters \n");
    }

    return  cht2ResovedPacketl3intrarouterCmdTable[prevCmd][currCmd];
}


/*******************************************************************************
*   snetCht2InterBridgeRouterPktCmdResolution
*
* DESCRIPTION:
*       resolve from bridge packet command and router packet command the
*       new packet command in bridge style
*
* INPUTS:
*       bridgeCmd - bridge packet command
*       routerCmd - router packet command
*
* OUTPUTS:
*       none
*
* RETURN:
*       new packet command in bridge style
*
* COMMENTS:
*
*******************************************************************************/
static SKERNEL_EXT_PACKET_CMD_ENT snetCht2InterBridgeRouterPktCmdResolution
(
    IN SKERNEL_EXT_PACKET_CMD_ENT bridgeCmd,
    IN HW_ROUTER_PACKET_COMMAND_E routerCmd
)
{
    if(bridgeCmd > SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
       routerCmd > HW_ROUTER_PACKET_COMMAND_BRIDGE_E)
    {
        /* fatal error */
        skernelFatalError("snetCht2InterBridgeRouterPktCmdResolution: bad parameters \n");
    }
    return  cht2ResovedPacketl3intrabridgeCmdTable[bridgeCmd][routerCmd];
}

/*******************************************************************************
*   snetCht2L3iCommandResolve
*
* DESCRIPTION:
*    DESCRIPTION
*       Resolve intra router  decision  & Bridge Router decision
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       localdescrPtr       - pointer to local router engine next hop information
*       mcRpfFailCmdFromMll - if there is an RPF Failure exception,mcRpfFailCmdFromMll
*                             indicates how the RPF Fail command is selected.
*                             GT_TRUE:Use the RPF Fail Command in the MLL entry
*                                     whose VLAN matches the packet VLAN
*                             GT_FALSE:Use the Multicast Route entry
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetCht2L3iCommandResolve
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr ,
    IN SNET_CHEETAH2_L3_CNTRL_PACKET_INFO   * cntrlPcktInfo,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * localdescrPtr,
    IN GT_BOOL                              mcRpfFailCmdFromMll
)
{
    DECLARE_FUNC_NAME(snetCht2L3iCommandResolve);

    HW_ROUTER_PACKET_COMMAND_E commands[SNET_CHT2_IP_EXCEPTION_TYPES_NUM_CNS]; /* HW router packet command */
    HW_ROUTER_PACKET_COMMAND_E routeCmd;    /* router decision command */
    SKERNEL_EXT_PACKET_CMD_ENT combinedRouterBridgePacketCmd; /*combined bridge command*/
    GT_U32                     bridgeCpuCode;   /*bridge cpu code */
    GT_BOOL                    modifyOpCode;    /*do we allow router to modify CPU op code */
    GT_U32 i;                                   /* array index */
    GT_BIT  doRout;/* indication that we will rout or bridge */
    GT_U32  index;/*index in commands[] */

    /* No further routing process for ARP broadcast packets */
    if (descrPtr->arp)
    {
        __LOG(("No further routing process for ARP broadcast packets"));
     /* The Router engine does not perform any other processing on the ARP Broadcast
        packet other than an address lookup and possibly mirroring or trapping the packet to the CPU */
        return;
    }

    cntrlPcktInfo->bridgePacketCmd = descrPtr->packetCmd;
    __LOG(("bridgePacketCmd: "));
    simLogPacketDescrPacketCmdDump(devObjPtr,cntrlPcktInfo->bridgePacketCmd);
    __LOG(("\n"));


    index = 0;
    commands[index++] = cntrlPcktInfo->ipHeaderCmd;
    commands[index++] = cntrlPcktInfo->ipDaMismatchCmd;
    commands[index++] = cntrlPcktInfo->ipIllegalAddrCmd;
    commands[index++] = cntrlPcktInfo->ipSaMismatchCmd;
    commands[index++] = cntrlPcktInfo->ipvUcRpfRailCmd;
    commands[index++] = cntrlPcktInfo->ipvMcRpfFailCmd;
    commands[index++] = cntrlPcktInfo->ipvHopLimitExceededFailCmd;
    commands[index++] = cntrlPcktInfo->ipvRouteMtuExcceededFailCmd;
    commands[index++] = cntrlPcktInfo->ipvOptionHbhOptionFailCmd;
    commands[index++] = cntrlPcktInfo->ipvIcmpRedirectFailCmd;
    commands[index++] = cntrlPcktInfo->ipv6ScopeFailCmd;
    commands[index++] = cntrlPcktInfo->ipvAccessCmd;
    commands[index++] = cntrlPcktInfo->ipvAllZeroCmd;
    commands[index++] = cntrlPcktInfo->ipvSipFilterCmd;

    routeCmd = commands[0];
    /* Intra-Router Engine Command and CPU Code Resolution */
    __LOG(("Start Intra-Router Engine packet Command \n"));
    for (i = 1; i < index; i++)
    {
        routeCmd = snetCht2IntraRouterPktCmdResolution(routeCmd, commands[i]);
    }

    /* Save the 'exceptions' router packet command resolution - before resolution with the 'route entry' */
    cntrlPcktInfo->ipvExceptionsPacketCmd = routeCmd;
    logRouterPacketCommand(devObjPtr,
        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvExceptionsPacketCmd));
    /* calculate the next hop packet command + 'isRoutEntryForwardToCpu' */
    cntrlPcktInfo->ipvNextHopPacketCmd = cntrlPcktInfo->ipvRouteEntry03Cmd;
    if(cntrlPcktInfo->isRoutEntryForwardToCpu)
    {
        cntrlPcktInfo->ipvNextHopPacketCmd =
            snetCht2IntraRouterPktCmdResolution(cntrlPcktInfo->ipvNextHopPacketCmd,
                HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E);
    }

    logRouterPacketCommand(devObjPtr,
        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvNextHopPacketCmd));

    /* do resolution with the route entry after all the exceptions */
    routeCmd = snetCht2IntraRouterPktCmdResolution(routeCmd, cntrlPcktInfo->ipvNextHopPacketCmd);

    /* Save the (final) router Engine packet command resolution (before resolution with bridge) */
    cntrlPcktInfo->ipvFinalPacketCmd = routeCmd;
    logRouterPacketCommand(devObjPtr,
        NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvFinalPacketCmd));

    __LOG(("Finished Intra-Router Engine packet Command \n"));

    modifyOpCode = GT_FALSE;
    /* The Router engine can override the Bridge CPU code assignment */
    if(cntrlPcktInfo->ipvFinalPacketCmd == HW_ROUTER_PACKET_COMMAND_TRAP_TO_CPU_E)
    {
        modifyOpCode = GT_TRUE;
    }
    else
    if(cntrlPcktInfo->bridgePacketCmd   != SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E &&
      (cntrlPcktInfo->ipvFinalPacketCmd == HW_ROUTER_PACKET_COMMAND_BRIDGE_AND_MIRROR_TO_CPU_E ||
       cntrlPcktInfo->ipvFinalPacketCmd == HW_ROUTER_PACKET_COMMAND_ROUTE_AND_MIRROR_TO_CPU_E))
    {
        modifyOpCode = GT_TRUE;
    }

    if(modifyOpCode)
    {
        __LOG(("The Router engine will override the Bridge CPU code assignment \n"));
    }
    else
    {
        __LOG(("The Router engine may not override the Bridge CPU code assignment \n"));
    }


    /* Resolution between bridge and router */
    combinedRouterBridgePacketCmd = snetCht2InterBridgeRouterPktCmdResolution(
        cntrlPcktInfo->bridgePacketCmd,
        cntrlPcktInfo->ipvFinalPacketCmd);

    __LOG(("Resolution between bridge and router: "));
    simLogPacketDescrPacketCmdDump(devObjPtr,combinedRouterBridgePacketCmd);
    __LOG(("\n"));

    doRout =
        IS_NOT_ROUTE_AND_NON_ROUTE_AND_MIRROR_MAC(cntrlPcktInfo->ipvExceptionsPacketCmd) ?
        0 : 1;

    if (doRout) /* route / route_and_mirror_to_cpu */
    {
        /* Packets are routed */
        __LOG(("Packets are to be routed, take all decisions from the Next hop \n"));

        /* Save bridge CPU code before router decisions */
        bridgeCpuCode = descrPtr->cpuCode;
        memcpy(descrPtr, localdescrPtr, sizeof(SKERNEL_FRAME_CHEETAH_DESCR_STC));
        if(modifyOpCode == GT_FALSE)
        {
            /* No CPU code modification */
            descrPtr->cpuCode = bridgeCpuCode;
            __LOG(("No CPU code modification - keep CPU code of the 'bridge' [%d] \n",
                bridgeCpuCode));
        }
    }

    /* Set packet command after the resolution */
    __LOG(("Set packet command after the resolution"));
    descrPtr->packetCmd = combinedRouterBridgePacketCmd;
    /* Override application specific CPU code with new one from next hop entry  */
    descrPtr->appSpecCpuCode = localdescrPtr->appSpecCpuCode;
    __LOG_PARAM(descrPtr->appSpecCpuCode);

    if(modifyOpCode == GT_FALSE)
    {
        return;
    }

    /* CPU code resolution */
    __LOG(("CPU code resolution by the router \n"));
/*
1. IPV4_HEADER_ERROR, IPV6_HEADER_ERROR
2. IPV4/6_DIP_DA_MISMATCH
3. IPV4/6_ILLEGAL_ADDR_ERROR
4. IPV4/6_SIP_IS_ZERO (sip5)
5. IPv4/6_UC_SIP_SA_MISMATCH
6. IPV4/6_UC_RPF_FAIL
7. IPv4/6_UC_DIP_NOT_FOUND (sip5)
8. IPv4/6_UC_SIP_NOT_FOUND (sip5)
9. IPv4/6_MC_DIP_NOT_FOUND (sip5 , {*,G})
10. ARP_BC_TO_ME (sip5)
11. IPV4_UC_ROUTE_ENTRY_0 through IPV4_UC_ROUTE_ENTRY_3
   IPV4_MC_ROUTE_ENTRY_0 through IPV4_MC_ROUTE_ENTRY_3
   IPV6_UC_ROUTE_ENTRY_0 through IPV6_UC_ROUTE_ENTRY_3
   IPV6_MC_ROUTE_ENTRY_0 through IPV4_MC_ROUTE_ENTRY_3
12. IPV4/6_MC_ROUTE_RPF_FAIL ,
    IPV4/6_MC_ROUTE_BIDIR_FAIL (sip5)
13. IPv4/6_MC_DIP_NOT_FOUND (sip5 , {S,G})
14. IPV6_HOPLIMIT_EXCEEDED, IPV4_TTL_EXCEEDED
15. IPV4/6_MTU_EXCEEDED
16. IPV4_OPTIONS, IPV6_HBH_OPTIONS, IPv6_NON_HBH_OPTION
17. IPV4_UC_ICMP_REDIRECT, IPV6_UC_ICMP_REDIRECT
18. IPV6_SCOPE
19. IPV4/6_SIP_FILTERING (sip5)
20. ACCESS_MATRIX (sip5)
*/
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipHeaderCmd,doRout))
    {
        descrPtr->cpuCode = (descrPtr->isIPv4 == 0) ?
            SNET_CHT_IPV6_HEADER_ERROR :
            SNET_CHT_IPV4_HEADER_ERROR;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipDaMismatchCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_DIP_DA_MISMATCH; ;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipIllegalAddrCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_ILLEGAL_ADDR_ERROR;
    }
    else
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) &&
       IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvAllZeroCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_SIP_IS_ZERO;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipSaMismatchCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_SIP_SA_MISMATCH;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvUcRpfRailCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_UC_RPF_FAIL;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvNextHopPacketCmd,doRout))
    {
        if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvRouteEntry03Cmd,doRout))
        {
            /* the trap / mirror to CPU comes from the next hope */
           /* use cpuCode from the Next Hop */
            descrPtr->cpuCode = localdescrPtr->cpuCode;
        }
        else /* got the trap from cntrlPcktInfo->isRoutEntryForwardToCpu */
        {
            /* this is 'implicit' TRAP , that use different CPU_CODE from the one in the next hop */
            descrPtr->cpuCode = SNET_CHT_ROUTED_PACKET_FORWARD ;
        }
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvMcRpfFailCmd,doRout))
    {
        if(mcRpfFailCmdFromMll == GT_TRUE)
        {
            descrPtr->cpuCode = SNET_CHT_IPV4_6_MC_MLL_RPF_FAIL;
        }
        else
        {
            descrPtr->cpuCode = SNET_CHT_IPV4_6_MC_RPF_FAIL;

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && cntrlPcktInfo->isMcRpfBidirFail)
            {
                descrPtr->cpuCode = SNET_LION3_CPU_CODE_IPV4_6_MC_BIDIR_RPF_FAIL;
            }
        }
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvHopLimitExceededFailCmd,doRout))
    {
        descrPtr->cpuCode = (descrPtr->isIPv4 == 0) ?
           SNET_CHT_IPV6_UC_HOP_LIMIT_EXCEEDED :
           SNET_CHT_IPV4_UC_TTL_EXCEEDED;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvRouteMtuExcceededFailCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_MTU_EXCEEDED;
    }
    else
    if (IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvOptionHbhOptionFailCmd,doRout))
    {
        descrPtr->cpuCode = (descrPtr->isIPv4 == 0) ?
           SNET_CHT_IPV6_UC_HOP_BY_HOP :
           SNET_CHT_IPV4_UC_OPTIONS ;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvIcmpRedirectFailCmd,doRout))
    {
        /* this case is not considered 'exception' by macro
           SNET_CHT2_ROUTE_CMD_RESOLVE_MAC */
        descrPtr->cpuCode = (descrPtr->isIPv4 == 0) ?
           SNET_CHT_IPV6_UC_ICMP_REDIRECT :
           SNET_CHT_IPV4_UC_ICMP_REDIRECT;
    }
    else
    if(IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipv6ScopeFailCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV6_SCOPE;
    }
    else
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) &&
       IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvSipFilterCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_IPV4_6_SIP_FILTERING;
    }
    else
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) &&
       IS_CPU_CODE_NEEDED_MAC(cntrlPcktInfo->ipvAccessCmd,doRout))
    {
        descrPtr->cpuCode = SNET_CHT_ACCESS_MATRIX;
    }

    __LOG(("descrPtr->cpuCode:"));
    simLogPacketDescrCpuCodeDump(devObjPtr,descrPtr->cpuCode);
}


/*******************************************************************************
* snetCht2L3iGetRpfFailCmdFromMll
*
* DESCRIPTION:
*       Get the RFP Fail Command from the MLL entry whose VLAN matches the
*       packet VLAN
*
* INPUTS:
*       devObjPtr           - (pointer to) device object.
*       descrPtr            - (pointer to) frame data buffer Id
*       mll_selector        - selector internal external mll for ipv6 MC
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*
* OUTPUTS:
*       None
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL snetCht2L3iGetRpfFailCmdFromMll
(
    IN SKERNEL_DEVICE_OBJECT                    * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC          * descrPtr,
    IN GT_U32                                   mll_selector,
    INOUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO    *cntrlPcktInfo
)
{
    DECLARE_FUNC_NAME(snetCht2L3iGetRpfFailCmdFromMll);

    GT_U32 mllIndex;/* MLL index */

    SNET_CHT3_DOUBLE_MLL_STC mll;/* current MLL pair entry (hold 2 single MLLs)*/
    SNET_CHT3_SINGLE_MLL_STC *singleMllPtr; /* pointer to the info about single
                                            MLL (first/second half of the pair)*/
    GT_U32  ii;/* iterator */
    GT_BIT  isIpv6 = (descrPtr->isIPv4 || descrPtr->isFcoe) ? 0 : 1;

    /* get IPMC routing parameters */
    __LOG(("get IPMC routing parameters"));
    if (isIpv6 && (mll_selector == SKERNEL_MLL_SELECT_EXTERNAL_E))
    {
        mllIndex = descrPtr->mllexternal;
    }
    else
    {
        mllIndex = descrPtr->mll;
    }

    /* find the MLL with the same VLAN as the packet VLAN */
    __LOG(("find the MLL with the same VLAN as the packet VLAN"));

    while(1)
    {
        snetCht3ReadMllEntry(devObjPtr, mllIndex, &mll);

        /* check the 2 MLL sections */
        __LOG(("check the 2 MLL sections"));
        for(ii = 0 ; ii < 2 ; ii++)
        {
            if(ii == 0)
            {
                singleMllPtr = &mll.first_mll;
            }
            else
            {
                singleMllPtr = &mll.second_mll;
            }

            if (singleMllPtr->vid == descrPtr->eVid)
            {
                switch(singleMllPtr->rpf_fail_cmd)
                {
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                        cntrlPcktInfo->ipvMcRpfFailCmd = singleMllPtr->rpf_fail_cmd;
                        logRouterPacketCommand(devObjPtr,
                            NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvMcRpfFailCmd));
                        return GT_TRUE;

                    default:
                        /* reserved values for this option */
                        __LOG(("reserved values for this option"));
                        return GT_FALSE;
                }
            }
        }

        if(mll.nextPtr == 0)
        {
            /* NOTE : we should not get here because the second section of MLL
                should have been set to singleMllPtr->last = 0 */
            return GT_FALSE;
        }
        /* update the address for the next MLL */
        __LOG(("update the address for the next MLL"));
        mllIndex = mll.nextPtr;
    }
}

typedef enum{
    COUNTER_SET_COUTER_TYPE_inUcMcPacketCounter_E,
    COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedExceptionPacketCounter_E,
    COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedNonExceptionPacketCounter_E,
    COUNTER_SET_COUTER_TYPE_inUcMcTrappedMirroredPacketCounter_E,
    COUNTER_SET_COUTER_TYPE_mcRpfFailPacketCounter_E,
    COUNTER_SET_COUTER_TYPE_outUnicastPacketCounter_E
}COUNTER_SET_COUTER_TYPE_ENT;

/*******************************************************************************
*   routerEngineCounterSetCounters
*
* DESCRIPTION:
*       Update IP counters of specific set for specific counter type for UC/MC
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       counterSetId        - 'counter set' id
*       isMc                - is counted for MC/UC
*       counterType         - the counter type
*
*
*******************************************************************************/
static GT_VOID routerEngineCounterSetCounters
(
    IN SKERNEL_DEVICE_OBJECT        * devObjPtr,
    IN GT_U32                       counterSetId,
    IN GT_BOOL                      isMc,
    IN COUNTER_SET_COUTER_TYPE_ENT  counterType
)
{
    GT_U32 regAddr;                 /* Register address                            */
    GT_U32 * regPtr;                /* Register entry pointer                      */
    GT_CHAR *mcName;
    GT_CHAR *counterTypeName;

    if(isMc == GT_FALSE)
    {
        mcName = "unicast";
        switch(counterType)
        {
            case COUNTER_SET_COUTER_TYPE_inUcMcPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_UNICAST_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedExceptionPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_UNICAST_EXPECTED_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedNonExceptionPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_NON_ROUTED_UNICAST_NON_EXPECTED_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_inUcMcTrappedMirroredPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_UNICAST_TRAPPED_MIRRORED_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_outUnicastPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_OUT_PACKET_REG(devObjPtr,counterSetId);
                break;
            default:
                __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("unknown counterType[%d] \n",
                    counterType));
                return;
        }
    }
    else
    {
        mcName = "multicast";

        switch(counterType)
        {
            case COUNTER_SET_COUTER_TYPE_inUcMcPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_MULTICAST_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedExceptionPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_MULTICAST_EXPECTED_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedNonExceptionPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_NON_ROUTED_MULTICAST_NON_EXPECTED_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_inUcMcTrappedMirroredPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_MULTICAST_TRAPPED_MIRRORED_PACKET_REG(devObjPtr,counterSetId);
                break;
            case COUNTER_SET_COUTER_TYPE_mcRpfFailPacketCounter_E:
                regAddr =
                    SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_RPF_FAIL_MULTICAST_PACKET_REG(devObjPtr,counterSetId);
                break;
            default:
                __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("unknown counterType[%d] \n",
                    counterType));
                return;
        }
    }

    regPtr = smemMemGet(devObjPtr,regAddr);

    switch(counterType)
    {
        SWITCH_CASE_NAME_MAC(COUNTER_SET_COUTER_TYPE_inUcMcPacketCounter_E                      ,counterTypeName);
        SWITCH_CASE_NAME_MAC(COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedExceptionPacketCounter_E    ,counterTypeName);
        SWITCH_CASE_NAME_MAC(COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedNonExceptionPacketCounter_E ,counterTypeName);
        SWITCH_CASE_NAME_MAC(COUNTER_SET_COUTER_TYPE_inUcMcTrappedMirroredPacketCounter_E       ,counterTypeName);
        SWITCH_CASE_NAME_MAC(COUNTER_SET_COUTER_TYPE_mcRpfFailPacketCounter_E                   ,counterTypeName);
        SWITCH_CASE_NAME_MAC(COUNTER_SET_COUTER_TYPE_outUnicastPacketCounter_E                  ,counterTypeName);
        default:
            counterTypeName = "unknown";
            break;
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("increment %s %s Counter[%d] from [%d] \n",
        mcName,counterTypeName,counterSetId,(*regPtr)));

    (*regPtr)++;

}


/*******************************************************************************
*   snetCht2L3iCounters
*
* DESCRIPTION:
*       Update IP counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*
*
*
*******************************************************************************/
GT_VOID snetCht2L3iCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH2_L3_CNTRL_PACKET_INFO * cntrlPcktInfoPtr ,
    IN GT_U32 * routeIndexPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iCounters);

    GT_U32 regAddr = SMAIN_NOT_VALID_CNS; /* Register address (init with not valid to avoid compiler warnings) */
    GT_U32 counterSet;              /* Register's field value                      */
    GT_U32 cntsetBindmode;          /* counting bind mode :
            0x0 = Interface counter: Counts all packets bounded to an interface according to the configuration of this table.
            0x1 = Route Entry counter: Counts all packets with NHE <CntSetIndex> matching this set number    */
    GT_U32 nextHopVid;              /* vlan read from next hop compared with eVid interface*/
    GT_U32 cntset_port_trunk_mode;  /* port trunk mode for interface bind          */
    GT_U32 cntset_port_trunk;       /* port trunk value for interface bind         */
    GT_U32 cntset_ip_mode;          /* counting mode interface route entry         */
    GT_U32 cntset_vlan_mode;        /* counting mode interface route entry         */
    GT_U32 cntset_vid;              /* vlan value  for interface bind              */
    GT_U32 cntset_dev;              /* device value  for interface bind            */
    GT_U32 *nextHopeEntryPtr;       /* pointer to the relevant NH entry */
    GT_U32 ii;                      /* Index for choosing counter mode             */
    GT_BOOL isCountVid;             /* whether packet's eVid is matching this counterset             */
    GT_BOOL isCountProtocol;        /* whether packet's protocol is matching this counterset        */
    GT_BOOL isCountIf;              /* whether packet's out interface is matching this counterset   */
    GT_BOOL isIncomingPacket = GT_FALSE; /* whether the packet was matched on expected incoming port */
    GT_U32  value;                  /* register value */
    GT_BIT  inUcMcNonRoutedExceptionPacketCounter;
    GT_BIT  inUcMcNonRoutedNonExceptionPacketCounter;
    GT_BIT  inUcMcTrappedMirroredPacketCounter;
    GT_BIT  mcRpfFailPacketCounter;
    GT_BIT  outUnicastPacketCounter;
    GT_BIT  allowCount;
    GT_BOOL isMc = descrPtr->ipm ? GT_TRUE : GT_FALSE;
    GT_BOOL isValidNextHop = GT_TRUE;/* indication that we have valid next hop */

    /*************************** DROP COUNTER REGISTERS *******************/
    /* count drops regardless to TCAM/LPM match */
    snetCht2L3iDropCounters(devObjPtr,descrPtr,cntrlPcktInfoPtr);

    if( (routeIndexPtr[0] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS) &&
        (routeIndexPtr[1] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS) )
    { /* No TCAM match */
        if (SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            __LOG(("No LPM match (so no valid next hop) \n"));
        }
        else
        {
            __LOG(("No TCAM match (so no valid next hop) \n"));
        }

        isValidNextHop = GT_FALSE;
        /* still allow possible count on source interface */
    }

    if(isValidNextHop == GT_TRUE)
    {

        /* multicast packet (G,*) - take DIP route entry */
        if ((descrPtr->ipm == 1) &&    /* multicast packet (G,S) - take SIP route entry */
            (routeIndexPtr[1] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS))
        {
            __LOG(("multicast packet (G,S) - take SIP route entry \n"));
            regAddr = SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(devObjPtr,routeIndexPtr[1]);
        }
        else
        /* for unicast packet                               */
        if (routeIndexPtr[0] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
        {
            __LOG(("for unicast packet - take DIP route entry \n"));
            regAddr = SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(devObjPtr,routeIndexPtr[0]);
        }
        else
        {
            __LOG(("No valid NH entry \n"));
            isValidNextHop = GT_FALSE;
            /* still allow possible count on source interface */
        }
    }


    if(isValidNextHop == GT_TRUE)
    {
        nextHopeEntryPtr = smemMemGet(devObjPtr, regAddr);

        if (SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            nextHopVid = snetFieldValueGet(nextHopeEntryPtr, 22, 13);
        }
        else
        {
            nextHopVid = (SMEM_U32_GET_FIELD(nextHopeEntryPtr[0], 20, 12));
        }

        __LOG_PARAM(nextHopVid);

        /* Counter Set 0..3 is bonded */
        if (SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            counterSet = snetFieldValueGet(nextHopeEntryPtr, 93, 3);
        }
        else
        {
            counterSet = (SMEM_U32_GET_FIELD(nextHopeEntryPtr[2], 3, 3));
        }

        if(counterSet > 3)
        {
            __LOG(("Counter Set [%d] considered as 'not bound'(still allow possible count on source interface) \n",
                counterSet));
            /* still allow possible count on source interface */
        }

    }
    else
    {
        __LOG(("Next Hop not valid , still allow possible count on source interface \n"));
        /* value to guaranty that counter set that is bound to nexthop will have
           'no match' so will not count  */
        counterSet = SMAIN_NOT_VALID_CNS;
        nextHopVid = SMAIN_NOT_VALID_CNS;
    }


    __LOG_PARAM(counterSet);
    __LOG_PARAM(nextHopVid);
    /* In Non-Routed Exception Packet Counter
        The number of ingress packets that had one of the
        following exceptions whose associated command != ROUTE or
        ROUTE_AND_MIRROR:
    */
    inUcMcNonRoutedExceptionPacketCounter =
        IS_NOT_ROUTE_AND_NON_ROUTE_AND_MIRROR_MAC(cntrlPcktInfoPtr->ipvExceptionsPacketCmd) ?
            1 : 0;
    __LOG_PARAM(inUcMcNonRoutedExceptionPacketCounter);

    if(inUcMcNonRoutedExceptionPacketCounter == 0)
    {
        /*
            The number of ingress  packets that were NOT routed for
            a reason other than an exception, i.e.,:
            - Route entry <command> != ROUTE or
            ROUTE_AND_MIRROR
            AND/OR
            Packet was assigned a packet command of TRAP or
            SOFT_DROP prior to the Router engine
            AND
            - The packet did NOT have any Router engine exception
            whose associated command != ROUTE or
            ROUTE_AND_MIRROR
        */
        inUcMcNonRoutedNonExceptionPacketCounter =
            IS_NOT_ROUTE_AND_NON_ROUTE_AND_MIRROR_MAC(cntrlPcktInfoPtr->ipvNextHopPacketCmd) ?
                1 : 0;

        if(cntrlPcktInfoPtr->bridgePacketCmd > SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
        {
            inUcMcNonRoutedNonExceptionPacketCounter = 0;
        }
    }
    else
    {
        inUcMcNonRoutedNonExceptionPacketCounter = 0;
    }
    __LOG_PARAM(inUcMcNonRoutedNonExceptionPacketCounter);

    /*
        The number of packets that are trapped or mirrored to
        the CPU due to command assigned by the Router engine (i.e. the
        Route entry command or an exception command).
    */
    inUcMcTrappedMirroredPacketCounter = 0;
    if(IS_TRAP_OR_MIRROR_TO_CPU_MAC(cntrlPcktInfoPtr->ipvFinalPacketCmd))
    {
        if((descrPtr->cpuCode == SNET_CHT_IPV4_6_MC_MLL_RPF_FAIL) ||
           (descrPtr->cpuCode == SNET_CHT_IPV4_6_MC_RPF_FAIL)     ||
           (descrPtr->cpuCode == SNET_LION3_CPU_CODE_IPV4_6_MC_BIDIR_RPF_FAIL))
        {
            __LOG(("Note: since MC RPF check failed - inUcMcTrappedMirroredPacketCounter is not incremented \n"));
        }
        else
        {
            inUcMcTrappedMirroredPacketCounter = 1;
        }
    }
    __LOG_PARAM(inUcMcTrappedMirroredPacketCounter);

    /*
        The number of packets that have a Multicast RPF Failure exception
    */
    if(cntrlPcktInfoPtr->isMcRpfFail || cntrlPcktInfoPtr->isMcRpfBidirFail)
    {
        mcRpfFailPacketCounter = 1;
    }
    else
    {
        mcRpfFailPacketCounter = 0;
    }
    __LOG_PARAM(mcRpfFailPacketCounter);

    /*
        If the counter-set is bound to one or more Route Entries, this
        counter reflects the number of Unicast packets whose final
        Router command resolution is either ROUTE or
        ROUTE_AND_MIRROR.
        If the counter-set is bound to an interface, this counter reflects
        the number of Unicast packets routed out the configured
        interface
    */
    outUnicastPacketCounter =
        IS_NOT_ROUTE_AND_NON_ROUTE_AND_MIRROR_MAC(cntrlPcktInfoPtr->ipvFinalPacketCmd) ?
            0 : 1;
    __LOG_PARAM(outUnicastPacketCounter);

    for(ii=0; ii<4; ii++)
    {
        /*we need to remove statement it is incoming packet from previous counter*/
        isIncomingPacket = GT_FALSE;

        /* Counter Set <n> */
        __LOG(("Start checking Counter Set <%d> \n",
            ii));
        __LOG_PARAM(ii);
        regAddr = SMEM_CHT2_ROUTER_MANAGEMENT_COUNTER_CONFIGURATION_REG(devObjPtr, ii);
        smemRegGet(devObjPtr , regAddr, &value);

        cntset_port_trunk_mode = SMEM_U32_GET_FIELD(value, 30 , 2 );
        __LOG_PARAM(cntset_port_trunk_mode);
        cntsetBindmode            = SMEM_U32_GET_FIELD(value, 29 , 1 );
        __LOG_PARAM(cntsetBindmode);

        if( !SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            cntset_ip_mode         = SMEM_U32_GET_FIELD(value, 27 , 2 );
            cntset_vlan_mode       = SMEM_U32_GET_FIELD(value, 26 , 1 );
            cntset_vid             = SMEM_U32_GET_FIELD(value, 14 , 12);
            cntset_dev             = SMEM_U32_GET_FIELD(value, 8  , 5 );
            cntset_port_trunk      = SMEM_U32_GET_FIELD(value, 0  , 8 );
        }
        else
        { /* sip5 new ipvx mode */

            /* get counter config 0 values */
            cntset_ip_mode   = SMEM_U32_GET_FIELD(value, 26,  3);
            cntset_vlan_mode = SMEM_U32_GET_FIELD(value, 25,  1);

            cntset_vid       = SMEM_U32_GET_FIELD(value,  0, 13);

            /* get counter config 1 values */
            regAddr = SMEM_LION2_IPVX_ROUTER_EXTRA_MANAGEMENT_COUNTER_CONFIGURATION_REG(devObjPtr, ii);
            smemRegGet(devObjPtr , regAddr, &value);

            if (SMEM_CHT_IS_SIP5_15_GET(devObjPtr) == 0)
            {
                cntset_dev        = SMEM_U32_GET_FIELD(value, 13, 10);
                cntset_port_trunk = SMEM_U32_GET_FIELD(value, 0 , 13);
            }
            else
            {
                cntset_dev        = SMEM_U32_GET_FIELD(value, 15, 10);
                cntset_port_trunk = SMEM_U32_GET_FIELD(value, 0 , 15);
            }
        }
        __LOG_PARAM(cntset_ip_mode);
        __LOG_PARAM(cntset_vlan_mode);
        __LOG_PARAM(cntset_vid);
        __LOG_PARAM(cntset_dev);
        __LOG_PARAM(cntset_port_trunk);


        __LOG_PARAM(descrPtr->useVidx == 1);
        if(descrPtr->useVidx == 1)
        {
            __LOG_PARAM(descrPtr->srcDev);
            __LOG_PARAM(descrPtr->localDevSrcPort);
            __LOG_PARAM(descrPtr->localDevSrcTrunkId);

            __LOG_PARAM((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcPort));
            __LOG_PARAM((cntset_port_trunk_mode == 2) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcTrunkId));

            isCountIf = GT_TRUE;
            __LOG_PARAM(isCountIf);
            if (/*in mc we only need to know if it is incoming packet, in uc it is different (look bellow)*/
                ((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcPort)) ||
                ((cntset_port_trunk_mode == 2) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcTrunkId))
               )
            {
                isIncomingPacket = GT_TRUE;
                __LOG_PARAM(isIncomingPacket);
            }
        }
        else
        {
            __LOG_PARAM(cntset_port_trunk_mode == 0);
            __LOG_PARAM((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->trgDev) && (cntset_port_trunk == descrPtr->trgEPort));
            __LOG_PARAM((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcPort));
            __LOG_PARAM((cntset_port_trunk_mode == 2) && (descrPtr->targetIsTrunk) && (cntset_port_trunk == descrPtr->trgTrunkId));
            __LOG_PARAM((cntset_port_trunk_mode == 2) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcTrunkId));

            if (
                (cntset_port_trunk_mode == 0) ||    /* disregard port/trunk */
                                                    /* counts tx traffic trg port + dev*/
                ((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->trgDev) && (cntset_port_trunk == descrPtr->trgEPort)) ||
                                                    /* counts rx traffic src port + dev*/
                ((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcPort)) ||
                                                    /* counts tx traffic on trunk */
                ((cntset_port_trunk_mode == 2) && (descrPtr->targetIsTrunk) && (cntset_port_trunk == descrPtr->trgTrunkId)) ||
                                                    /* counts rx traffic on trunk */
                ((cntset_port_trunk_mode == 2) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcTrunkId))
               )
            {
                isCountIf = GT_TRUE;
                __LOG_PARAM(isCountIf);
                if ( /*we should count the packet as incoming packet if it was received from expected port+dev/trunk*/
                    ((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->trgDev) && (cntset_port_trunk == descrPtr->trgEPort)) ||
                    ((cntset_port_trunk_mode == 2) && (descrPtr->targetIsTrunk) && (cntset_port_trunk == descrPtr->trgTrunkId))
                   )
                {
                    isIncomingPacket = GT_FALSE;
                    __LOG_PARAM(isIncomingPacket);
                }
                else if (/*we should count the packet as outgoing packet if it was targeted to expected port+dev/trunk*/
                         ((cntset_port_trunk_mode == 1) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcPort)) ||
                         ((cntset_port_trunk_mode == 2) && (cntset_dev == descrPtr->srcDev) && (cntset_port_trunk == descrPtr->localDevSrcTrunkId))
                        )
                {
                    isIncomingPacket = GT_TRUE;
                    __LOG_PARAM(isIncomingPacket);
                }

            }
            else
            {
                isCountIf = GT_FALSE;
                __LOG_PARAM(isCountIf);
            }
        }

        __LOG_PARAM(cntset_vlan_mode == 0);
        __LOG_PARAM(nextHopVid == cntset_vid);

        if (
            (cntset_vlan_mode == 0 ) ||             /* disregard eVid        */
            (nextHopVid == cntset_vid)              /* counts specific eVid  */
           )
        {
            /* we don't set isIncomingPacket = GT_FALSE because it is by default
               GT_FALSE and we don't want to override the port/trunk decision*/
            isCountVid = GT_TRUE;
        }
        else
        {
            __LOG_PARAM(descrPtr->ingressPipeVid);
            __LOG_PARAM(descrPtr->ingressPipeVid == cntset_vid);

            if (descrPtr->ingressPipeVid == cntset_vid)
            {
                isIncomingPacket = GT_TRUE;
                isCountVid = GT_TRUE;
                __LOG_PARAM(isIncomingPacket);
            }
            else
            {
                isCountVid = GT_FALSE;
            }
            __LOG_PARAM(isCountVid);
        }

        __LOG_PARAM(descrPtr->isIPv4);
        __LOG_PARAM(descrPtr->isFcoe);
        __LOG_PARAM(cntset_ip_mode  == 0);
        __LOG_PARAM((cntset_ip_mode == 1) && (descrPtr->isIPv4 == 1));
        __LOG_PARAM((cntset_ip_mode == 2) && (descrPtr->isIPv4 == 0));
        __LOG_PARAM(SMEM_CHT_IS_SIP5_GET(devObjPtr) && (cntset_ip_mode == 3) && descrPtr->isFcoe);

        if (
            (cntset_ip_mode  == 0) ||                             /* disregard protocol   */
            ((cntset_ip_mode == 1) && (descrPtr->isIPv4 == 1)) || /* counts IPv4 packets  */
            ((cntset_ip_mode == 2) && (descrPtr->isIPv4 == 0)) || /* counts IPv6 packets  */
            (SMEM_CHT_IS_SIP5_GET(devObjPtr) && (cntset_ip_mode == 3) && descrPtr->isFcoe) /* counts FCoE packets  */
           )
        {
            isCountProtocol = GT_TRUE;
        }
        else
        {
            isCountProtocol = GT_FALSE;
        }
        __LOG_PARAM(isCountProtocol);

        __LOG_PARAM((ii == counterSet) && (cntsetBindmode == 1));
        __LOG_PARAM((cntsetBindmode == 0) && (isCountIf == GT_TRUE) & (isCountProtocol == GT_TRUE) && (isCountVid == GT_TRUE));

        /* Address associated with DIP search */
        if (
                                                    /* bound to specific route entry */
            ((ii == counterSet) && (cntsetBindmode == 1)) ||
                                                    /* bound to specific interface   */
            ((cntsetBindmode == 0) && (isCountIf == GT_TRUE) & (isCountProtocol == GT_TRUE) && (isCountVid == GT_TRUE))
           )
        {

            if((ii == counterSet) && (cntsetBindmode == 1))
            {
                __LOG(("counter set[%d] bound to specific route entry \n",
                    ii));
            }
            else
            {
                __LOG(("counter set[%d] bound to specific interface \n",
                    ii));
            }

            allowCount = 0;

            __LOG_PARAM(isCountIf == GT_TRUE || isCountVid == GT_TRUE);
            if (isCountIf == GT_TRUE || isCountVid == GT_TRUE)
            {
                __LOG_PARAM(isCountIf == GT_TRUE && (cntset_port_trunk_mode == 1 || cntset_port_trunk_mode == 2));
                __LOG_PARAM(isCountVid == GT_TRUE && cntset_vlan_mode !=0);
                /*in case we are counting per port or trunk we need to see this is incoming*/
                if ((isCountIf == GT_TRUE && (cntset_port_trunk_mode == 1 || cntset_port_trunk_mode == 2)) ||
                    (isCountVid == GT_TRUE && cntset_vlan_mode !=0))
                {
                    if (isIncomingPacket == GT_TRUE)
                    {
                        allowCount = 1;
                    }
                }/*if this is disregard port/trunk/vlan modes we count the packet anyhow*/
                else
                {
                    allowCount = 1;
                }
            }/*if this is not if/vlan we count the packet as it is bounded to route entry*/
            else
            {
                allowCount = 1;
            }

            if(allowCount)
            {
                routerEngineCounterSetCounters(devObjPtr,ii,isMc,
                    COUNTER_SET_COUTER_TYPE_inUcMcPacketCounter_E);

                if (inUcMcNonRoutedExceptionPacketCounter)
                {
                    routerEngineCounterSetCounters(devObjPtr,ii,isMc,
                        COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedExceptionPacketCounter_E);
                }

                if (inUcMcNonRoutedNonExceptionPacketCounter)
                {
                    routerEngineCounterSetCounters(devObjPtr,ii,isMc,
                        COUNTER_SET_COUTER_TYPE_inUcMcNonRoutedNonExceptionPacketCounter_E);
                }

                if(inUcMcTrappedMirroredPacketCounter)
                {
                    routerEngineCounterSetCounters(devObjPtr,ii,isMc,
                        COUNTER_SET_COUTER_TYPE_inUcMcTrappedMirroredPacketCounter_E);
                }

                if (mcRpfFailPacketCounter)
                {
                    routerEngineCounterSetCounters(devObjPtr,ii,isMc,
                        COUNTER_SET_COUTER_TYPE_mcRpfFailPacketCounter_E);
                }
            }

            if(descrPtr->ipm == 0 && outUnicastPacketCounter)
            {
                allowCount = 0;
                __LOG_PARAM(isCountIf == GT_TRUE || isCountVid == GT_TRUE);

                if (isCountIf == GT_TRUE || isCountVid == GT_TRUE)
                {
                    __LOG_PARAM(isCountIf == GT_TRUE && (cntset_port_trunk_mode == 1 || cntset_port_trunk_mode == 2));
                    __LOG_PARAM(isCountVid == GT_TRUE && cntset_vlan_mode !=0);
                    /*in case we are counting per port or trunk we need to see this is outgoing*/
                    if ((isCountIf == GT_TRUE && (cntset_port_trunk_mode == 1 || cntset_port_trunk_mode == 2)) ||
                        (isCountVid == GT_TRUE && cntset_vlan_mode !=0))
                    {
                        if (isIncomingPacket == GT_FALSE)
                        {
                            allowCount = 1;
                        }
                    }
                    else /*in all other cases when we are in interface mode we always count the packet*/
                    {
                        allowCount = 1;
                    }

                }
                else
                {
                    allowCount = 1;
                }

                if(allowCount)
                {
                    routerEngineCounterSetCounters(devObjPtr,ii,isMc,
                        COUNTER_SET_COUTER_TYPE_outUnicastPacketCounter_E);
                }
                allowCount = 0;
            }
        }

        __LOG(("End of Counter Set <%d> \n",
            ii));
    } /* End of ii loop */
}


/*******************************************************************************
*   snetCht2L3iDropCounters
*
* DESCRIPTION:
*       Update IP drop counters (if needed)
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*
*
*
*******************************************************************************/
static GT_VOID snetCht2L3iDropCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH2_L3_CNTRL_PACKET_INFO * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L3iDropCounters);
    GT_CHAR* dropNamePtr;
    GT_BIT  doCount;
    GT_U32  value,cntset_mode;
    GT_U32 * regPtr;                /* Register entry pointer                      */


    if (0 == IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvFinalPacketCmd))
    {

        __LOG(("not dropped by the IPvX unit \n"));
        return;
    }


    regPtr = smemMemGet(devObjPtr, SMEM_CHT2_ROUTER_DROP_COUNTER_REG(devObjPtr));

    smemRegGet(devObjPtr,SMEM_CHT2_ROUTER_DROP_COUNTER_CONFIGURATION_REG(devObjPtr),&value);
    cntset_mode = SMEM_U32_GET_FIELD(value, 0, 4);

    __LOG(("cntset_mode = [%d] \n",
        cntset_mode));

    doCount = 0;
    switch (cntset_mode)
    {
        case 0:
            __LOG(("update on any of the drop reasons \n"));
            doCount = 1;
            break;
        case 1:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipHeaderCmd))
                doCount = 1;
            break;
        case 2:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipDaMismatchCmd))
                doCount = 1;
            break;
        case 3:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipIllegalAddrCmd))
                doCount = 1;
            break;
        case 4:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvAllZeroCmd))
                doCount = 1;
            break;
        case 5:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipSaMismatchCmd))
                doCount = 1;
            break;
        case 6:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvUcRpfRailCmd))
                doCount = 1;
                break;
        case 7:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvRouteEntry03Cmd))
                doCount = 1;
                break;
        case 8:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvMcRpfFailCmd))
                doCount = 1;
                break;
        case 9:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvHopLimitExceededFailCmd))
                doCount = 1;
                break;
        case 10:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvRouteMtuExcceededFailCmd))
                doCount = 1;
                break;
        case 11:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvOptionHbhOptionFailCmd))
                doCount = 1;
                break;
        case 13:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipv6ScopeFailCmd))
                doCount = 1;
            break;
        case 14: /*UnicastPacketSIPFilter; Count packets dropped due to Unicast Packet SIP Filter*/
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvSipFilterCmd))
                doCount = 1;
                break;
        case 15:
            if (IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvAccessCmd))
                doCount = 1;
            break;
        default:
            /* case 12 : not defined in HW. */
            /* No current support for this field in simulation */
            __LOG(("case [%d] :No current support for this field in simulation \n",
                cntset_mode));
            break;
    }

    dropNamePtr =
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipHeaderCmd                 )   ?  "ipHeaderCmd"                 :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipDaMismatchCmd             )   ?  "ipDaMismatchCmd"             :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipIllegalAddrCmd            )   ?  "ipIllegalAddrCmd"            :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvAllZeroCmd               )   ?  "ipvAllZeroCmd"               :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipSaMismatchCmd             )   ?  "ipSaMismatchCmd"             :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvUcRpfRailCmd             )   ?  "ipvUcRpfRailCmd"             :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvRouteEntry03Cmd          )   ?  "ipvRouteEntry03Cmd"          :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvMcRpfFailCmd             )   ?  "ipvMcRpfFailCmd"             :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvHopLimitExceededFailCmd  )   ?  "ipvHopLimitExceededFailCmd"  :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvRouteMtuExcceededFailCmd )   ?  "ipvRouteMtuExcceededFailCmd" :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvOptionHbhOptionFailCmd   )   ?  "ipvOptionHbhOptionFailCmd"   :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipv6ScopeFailCmd            )   ?  "ipv6ScopeFailCmd"            :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvSipFilterCmd             )   ?  "ipvSipFilterCmd"             :
            IS_ROUTER_DROP_MAC(cntrlPcktInfoPtr->ipvAccessCmd                )   ?  "ipvAccessCmd"                :
            "unknown reason";

    if(doCount)
    {
        __LOG(("increment drop counter from[%d] , drop due to [%s] \n",
            *regPtr,
            dropNamePtr));

        (*regPtr)++;
    }
    else
    {
        __LOG(("NOT counted as drop although have drop reason due to [%s] \n",
            dropNamePtr));
    }

}



/*******************************************************************************
*   getLttMatchIndex
*
* DESCRIPTION:
*       gets ltt match index (relevant for tcam and ltt)
*
* INPUTS:
*       devObjPtr    - pointer to device object
*       descrPtr     - pointer to the frame's descriptor

* OUTPUTS:
*       matchIndexPtr - pointer to the match index
*
* RETURNS:
*       GT_TRUE       - the match index was found
*       GT_FALSE      - the match index was not found
*
*******************************************************************************/
static GT_BOOL getLttMatchIndex
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    OUT GT_U32                          *matchIndexPtr
)
{
    DECLARE_FUNC_NAME(getLttMatchIndex);

    GT_U32                  routerLTTIndex = 0;

    if(descrPtr->pclRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E || /* redirect to LTT */
       descrPtr->ttiRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E)
    {
        if(descrPtr->pclRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E)
        {
            __LOG(("PCL redirection to Router LTT Entry"));
            matchIndexPtr[0] = descrPtr->pceRoutLttIdx;
            matchIndexPtr[1] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;
        }
        else /*descrPtr->ttiRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_LTT_ROUTER_E*/
        {
            __LOG(("TTI redirection to Router LTT Entry"));
            matchIndexPtr[0] = descrPtr->ttRouterLTT;
            matchIndexPtr[1] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;
        }

        if(devObjPtr->supportEArch)
        {
            /* no manipulation needed */
        }
        else
        /* FEr#2018: Limited number of Policy-based routes */
        if(devObjPtr->errata.lttRedirectIndexError)
        {
            matchIndexPtr[0] *= 4; /* multiple by 4 --> actually gives value in 'first column' only */
        }
        else
        {
            if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
            {
                /* The Formula for calculating the LTT entry in the Hw is as follow:
                routerLTTIndex[11:0] << 2 + routerLTTIndex[14:13]
                (Note: bit 12 not used) */
                routerLTTIndex = matchIndexPtr[0];
                matchIndexPtr[0] = ((routerLTTIndex & 0xFFF) << 2) |
                            ((routerLTTIndex >> 13) & 0x3);
            }
        }
        return GT_TRUE;
    }
    return GT_FALSE;
}


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
)
{
    if(enable)
    {
        isLpmEnabled      = GT_TRUE;
        isLpmEnabled_test = GT_TRUE;
    }

    return GT_OK;
}


/*******************************************************************************
*   snetCht2RoutingCheckMatches
*
* DESCRIPTION:
*       Router Engine processing: check the matches (and logs the checks)
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       isSipLookupPerformed - indicates whether sip lookup performed
*       matchIndexArr - matches results
*
* OUTPUTS:
*       None
*
*******************************************************************************/
static GT_BOOL snetCht2RoutingCheckMatches
(
    IN SKERNEL_DEVICE_OBJECT              *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC    *descrPtr,
    IN GT_U32                              isSipLookupPerformed,
    IN GT_U32                             *matchIndexArr
)
{
    DECLARE_FUNC_NAME(snetCht2RoutingCheckMatches);

    GT_U32   sipMatched;  /*indication that SIP lookup performed and have matched*/

    if(matchIndexArr[0] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
    {
        __LOG(("DIP lookup match , index[0x%8.8x] \n",matchIndexArr[0]));
    }
    else
    {
        __LOG(("DIP lookup NOT match \n"));
    }

    if(isSipLookupPerformed && /* sip lookup performed */
            matchIndexArr[1] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS && /* got sip match */
           ! descrPtr->ipm ) /* unicast only */
    {
        __LOG(("SIP lookup match , index[0x%8.8x] \n",matchIndexArr[1]));
        sipMatched = 1;
    }
    else
    {
        sipMatched = 0;
        if(isSipLookupPerformed == 0)
        {
            __LOG(("SIP lookup NOT performed \n"));
        }
        else
        if(descrPtr->ipm)
        {
            __LOG(("SIP lookup NOT performed for IPM packets \n"));
        }
        else
        {
            __LOG(("SIP lookup NOT match \n"));
        }
    }

    if(sipMatched == 0 &&
       matchIndexArr[0] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
    {
        __LOG(("neither DIP nor SIP matched lookup \n"));
        return GT_FALSE;
    }

    if( !descrPtr->ipm &&
       matchIndexArr[0] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
    {
        __LOG(("unicast and DIP not matched \n"));
        return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetCht2RoutingAfterTriggerChecksAndNoExceptions
*
* DESCRIPTION:
*       Router Engine processing of frames after routing trigger checks,
*       and no exceptions
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*       localDescrPtr      - local router engine next hop information
*       translationDataPtr - pointer to Route next hop
*       mllSelectorPtr     - selector internal external mll for ipv6 MC
*       cntrlPcktInfoPtr   - local router enginePacket command
*
*******************************************************************************/
static GT_VOID snetCht2RoutingAfterTriggerChecksAndNoExceptions
(
    IN    SKERNEL_DEVICE_OBJECT              *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC    *descrPtr,
    OUT   SKERNEL_FRAME_CHEETAH_DESCR_STC    *localDescrPtr,
    OUT   GT_U32                             *translationDataPtr,
    INOUT GT_U32                             *mllSelectorPtr,
    INOUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO *cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2RoutingAfterTriggerChecksAndNoExceptions);

    SNET_LION3_FDB_ROUTE_ENTRY_INFO fdbDipLookupInfo = {0};

    GT_U32   matchIndexArr[SNET_CHT2_TCAM_SEARCH];
    GT_U32   isSipLookupPerformed = 1; /* enabled by default*/

    SNET_ROUTE_SECURITY_CHECKS_INFO_STC ipSecurChecksInfo = {0};

    __LOG(("Check DIP and SIP matching"));

    matchIndexArr[0] = translationDataPtr[0] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;
    matchIndexArr[1] = translationDataPtr[1] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;
    ipSecurChecksInfo.sipBaseRouteEntryIndex = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;

    if (devObjPtr->supportEArch && isLpmEnabled)
    {
        if(isLpmEnabled_test)
        {
            __LOG(("sip5 lpm test enabled, default lpm memory values will be modified"));

            snetLion3LpmPrefixAdd_test(devObjPtr, descrPtr);
        }

        /* do lpm search */
        isSipLookupPerformed = snetLion3Lpm(devObjPtr, descrPtr, matchIndexArr, &ipSecurChecksInfo);
        __LOG_PARAM(isSipLookupPerformed);

        if(descrPtr->fdbBasedUcRouting)
        {
            snetChtL2iFdbDipLookUp(devObjPtr, descrPtr, &fdbDipLookupInfo);

            __LOG_PARAM(fdbDipLookupInfo.isValid);

            if(fdbDipLookupInfo.isValid)
            {
                /* get the packet command associated with the entry (in sip5 due to lack of
                   bits in the FDB entry those 3 bits come from 'global register') */
                smemRegFldGet(devObjPtr, SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr),
                                    28, 3, &fdbDipLookupInfo.nextHop_hwRouterPacketCommand);

                __LOG_PARAM(fdbDipLookupInfo.nextHop_hwRouterPacketCommand);

                __LOG_PARAM(fdbDipLookupInfo.ipv6ScopeCheck);
                __LOG_PARAM(fdbDipLookupInfo.ipv6DestSiteId);
                __LOG_PARAM(fdbDipLookupInfo.decTtlOrHopCount);
                __LOG_PARAM(fdbDipLookupInfo.bypassTtlOptionsOrHopExtensionForIpv4Packets);
                __LOG_PARAM(fdbDipLookupInfo.ingressMirrorToAnalyzerIndex);
                __LOG_PARAM(fdbDipLookupInfo.qosProfileMarkingEn);
                __LOG_PARAM(fdbDipLookupInfo.qosProfileIndex);
                __LOG_PARAM(fdbDipLookupInfo.qosProfilePrecedence);
                __LOG_PARAM(fdbDipLookupInfo.modifyUp);
                __LOG_PARAM(fdbDipLookupInfo.modifyDscp);
                __LOG_PARAM(fdbDipLookupInfo.counterSetIndex);
                __LOG_PARAM(fdbDipLookupInfo.arpBcTrapMirrorEn);
                __LOG_PARAM(fdbDipLookupInfo.dipAccessLevel);
                __LOG_PARAM(fdbDipLookupInfo.ecmpRedirectExceptionMirror);
                __LOG_PARAM(fdbDipLookupInfo.mtuIndex);
                __LOG_PARAM(fdbDipLookupInfo.useVidx);
                __LOG_PARAM(fdbDipLookupInfo.trgIsTrunk);
                __LOG_PARAM(fdbDipLookupInfo.trgTrunkId);
                __LOG_PARAM(fdbDipLookupInfo.trgEport);
                __LOG_PARAM(fdbDipLookupInfo.evidx);
                __LOG_PARAM(fdbDipLookupInfo.trgDev);
                __LOG_PARAM(fdbDipLookupInfo.nextHopEvlan);
                __LOG_PARAM(fdbDipLookupInfo.startOfTunnel);
                __LOG_PARAM(fdbDipLookupInfo.tunnelType);
                __LOG_PARAM(fdbDipLookupInfo.tunnelPtrValid);
                __LOG_PARAM(fdbDipLookupInfo.arpPtrValid);

                __LOG(("sip5 fdb routing match: using indication in match array with SNET_LION3_FDB_ROUT_MATCH_INDICATION_ONLY_CNS\n"));
                __LOG(("used in pair with fdbDipLookupInfo.isValid indication\n"));
                matchIndexArr[0] = SNET_LION3_FDB_ROUT_MATCH_INDICATION_ONLY_CNS;
            }
            else
            {
                __LOG(("FDB did DIP lookup but not matched\n"));
            }
        }
    }
    else if ( GT_FALSE == getLttMatchIndex(devObjPtr, descrPtr, matchIndexArr) )
    {
        /* tcam+ltt */
        if(SKERNEL_IS_CHEETAH2_ONLY_DEV(devObjPtr))
        {
            snetCht2L3iTcamLookUp(devObjPtr, descrPtr, matchIndexArr);
        }
        else
        {
            snetCht3L3iTcamLookUp(devObjPtr, descrPtr, matchIndexArr);
        }
    }

    if(GT_FALSE == snetCht2RoutingCheckMatches(devObjPtr, descrPtr, isSipLookupPerformed, matchIndexArr))
    {
            __LOG(("DIP or SIP doesn't match"));
            return;
    }

    if(devObjPtr->supportEArch && isLpmEnabled)
    { /* sip 5 lpm */
        /* nothing to do, fetching Route entry logic inside snetLion3Lpm */
        translationDataPtr[0] = matchIndexArr[0];
        translationDataPtr[1] = matchIndexArr[1];
    }
    else
    { /* tcam+ltt */

        __LOG(("read the action from the routing action table"));

        if(SKERNEL_IS_CHEETAH2_ONLY_DEV(devObjPtr))
        {
            snetCht2L3iFetchRouteEntry(devObjPtr, descrPtr, matchIndexArr, translationDataPtr, &ipSecurChecksInfo);
        }
        else
        {
            snetCht3L3iFetchRouteEntry(devObjPtr, descrPtr, matchIndexArr, translationDataPtr, &ipSecurChecksInfo);
        }
    }

    /* update ch3 and above params (if nessesary) */
    if(SKERNEL_IS_CHEETAH3_DEV(devObjPtr))
    {
        snetCht3UpdateRpfCheckMode(devObjPtr, descrPtr, &ipSecurChecksInfo.rpfCheckMode);
        snetCht3UpdateSipSaCheckEnable(devObjPtr, descrPtr, &ipSecurChecksInfo.unicastSipSaCheck);
    }

    __LOG(("apply the action"));
    snetCht2L3iGetNextHop(devObjPtr, descrPtr, translationDataPtr, &fdbDipLookupInfo,
                          &ipSecurChecksInfo, cntrlPcktInfoPtr, localDescrPtr, mllSelectorPtr,isSipLookupPerformed);/* OUT */

    __LOG(("find the RPF fail command according to the MLL"));
    if(ipSecurChecksInfo.mcRpfFailCmdFromMll==GT_TRUE)
    {
        __LOG(("get indication that indeed the RPF fail command taken from the MLL"));
        ipSecurChecksInfo.mcRpfFailCmdFromMll = snetCht2L3iGetRpfFailCmdFromMll(devObjPtr, localDescrPtr,
                                                    *mllSelectorPtr, cntrlPcktInfoPtr);
    }

    __LOG(("Routing decision"));
    snetCht2L3iCommandResolve(devObjPtr, descrPtr, cntrlPcktInfoPtr, localDescrPtr,
                                            ipSecurChecksInfo.mcRpfFailCmdFromMll);
}

/*******************************************************************************
*   snetCht2RoutingAfterTriggerChecks
*
* DESCRIPTION:
*       Router Engine processing of frames after routing trigger checks
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*       descrPtr     - pointer to the frame's descriptor.
*       localDescrPtr - local router engine next hop information
*
*******************************************************************************/
static GT_VOID snetCht2RoutingAfterTriggerChecks
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    OUT   SKERNEL_FRAME_CHEETAH_DESCR_STC *localDescrPtr
)
{
    DECLARE_FUNC_NAME(snetCht2RoutingAfterTriggerChecks);

    GT_U32  translationDataPtr[SNET_CHT2_TCAM_SEARCH];/* pointer to Route next hop */
    GT_U32  mllSelector = SKERNEL_MLL_SELECT_EXTERNAL_E;/* selector internal/external mll for ipv6 MC */
    SNET_CHEETAH2_L3_CNTRL_PACKET_INFO cntrlPcktInfo; /* local router enginePacket command  */

    memset(&cntrlPcktInfo, 0, sizeof(SNET_CHEETAH2_L3_CNTRL_PACKET_INFO));

    translationDataPtr[0] =
    translationDataPtr[1] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;

    if  ( !snetCht2L3iExceptionCheck(devObjPtr, descrPtr, &cntrlPcktInfo) )
    { /* there were no exceptions */
        __LOG(("no 'Pre-Route Exception' detected \n"));
        snetCht2RoutingAfterTriggerChecksAndNoExceptions(devObjPtr, descrPtr, localDescrPtr,
                                            translationDataPtr, &mllSelector, &cntrlPcktInfo);
    }
    else
    { /* got exception */
        /* Routing decision */
        __LOG(("FAILED on 'Pre-Route Exception' Checking (so no Next Hop search) \n"));
        snetCht2L3iCommandResolve(devObjPtr, descrPtr, &cntrlPcktInfo, localDescrPtr, GT_FALSE);
    }

     /* Update router counters */
    __LOG(("Update router counters"));
    if(SKERNEL_IS_CHEETAH2_ONLY_DEV(devObjPtr))
    {
        snetCht2L3iCounters(devObjPtr, descrPtr, &cntrlPcktInfo, translationDataPtr);
    }
    else
    {
        snetCht3L3iCounters(devObjPtr, descrPtr, &cntrlPcktInfo, translationDataPtr);
    }

    __LOG_PARAM(descrPtr->doRouterHa);
    __LOG_PARAM(descrPtr->ipm);

    if(descrPtr->doRouterHa == 1 && descrPtr->ipm == 1)
    {
        if (0 == IS_NOT_ROUTE_AND_NON_ROUTE_AND_MIRROR_MAC(cntrlPcktInfo.ipvFinalPacketCmd))
        {
            /* IPMC Replications (for routed packets) */
            descrPtr->mllSelector = mllSelector;
            descrPtr->validMll = 1;

            __LOG_PARAM(descrPtr->mllSelector);
            __LOG_PARAM(descrPtr->validMll);
        }
        else
        {
            __LOG(("the Router command is not 'ROUTE' and not 'ROUTE_AND_MIRROR' , so no MLL accessing \n"));
        }
    }
}


/*******************************************************************************
*   snetCht2Routing
*
* DESCRIPTION:
*       Router Engine processing of frames
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUTS:
*       descrPtr     - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetCht2Routing
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht2Routing);

    SKERNEL_FRAME_CHEETAH_DESCR_STC  localDescr; /* local router engine next hop information */

    if(descrPtr->bypassRouterAndPolicer)
    {
        __LOG(("do not enter the Router Unit due to 'bypass router and policer' "));
        return;
    }

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E) /* Bridge Packet command decision */
    {
        __LOG(("do not enter the Router Unit due to 'hard drop' from the bridge \n"));
        return;
    }

    if(descrPtr->pclRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_PBR_OR_VPLS_E)
    {
        /* should not happen because the router mode not know to handle this mode */
        __LOG(("should not happen because the router mode not know to handle this mode"));
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* IPvX - Ingress EPort Table */
        descrPtr->eArchExtInfo.ipvxIngressEPortTablePtr =
            smemMemGet(devObjPtr,
                       SMEM_LION2_IPVX_INGRESS_EPORT_TBL_MEM(devObjPtr,
                                    descrPtr->eArchExtInfo.localDevSrcEPort));
        /* IPvX - Ingress EVlan Table */
        descrPtr->eArchExtInfo.ipvxIngressEVlanTablePtr =
            smemMemGet(devObjPtr,
                       SMEM_LION2_IPVX_INGRESS_EVLAN_TBL_MEM(devObjPtr,
                                    descrPtr->eVid));
    }

    memcpy(&localDescr, descrPtr, sizeof(SKERNEL_FRAME_CHEETAH_DESCR_STC));

    if ( snetCht2L3iTriggerCheck(devObjPtr, descrPtr) )/* add check for descrPtr->isIp */
    {
        __LOG(("PASSED the Router Trigger checks \n"));
        snetCht2RoutingAfterTriggerChecks(devObjPtr, descrPtr, &localDescr);
    }
    else
    {
        __LOG(("FAILED the Router Trigger checks \n"));
        __LOG_PARAM(descrPtr->brdgUcIpHeaderCheck);

        if(descrPtr->brdgUcIpHeaderCheck)
        {
            SNET_CHEETAH2_L3_CNTRL_PACKET_INFO cntrlPcktInfo; /* local router enginePacket command  */

            memset(&cntrlPcktInfo, 0, sizeof(SNET_CHEETAH2_L3_CNTRL_PACKET_INFO));

            /* IP Header Check for Bridged Unicast IPv4/6 Packets */
            __LOG(("IP Header Check for Bridged Unicast IPv4/6 Packets"));
            snetCht2L3iExceptionCheck(devObjPtr, descrPtr, &cntrlPcktInfo);

            /* Routing decision */
            __LOG(("Routing decision"));
            snetCht2L3iCommandResolve(devObjPtr, descrPtr, &cntrlPcktInfo, &localDescr, GT_FALSE);
        }
    }
}


/*******************************************************************************
*   snetCht3URpfCheck
*
* DESCRIPTION:
*       Unicast IPv4/6 Packet RPF check
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       routeIndexPtr  - pointer to the matching table.
*                            routeIndexPtr[0] - index of DIP route entry
*                            routeIndexPtr[1] - index of SIP route entry
*       ipSecurChecksInfoPtr - routing security checks information
*       vid1UnicastRoutingEn - unicast vid1 routing enabled
*       nextHopVid1          - next hop vid1 (sip5 only)
*
* OUTPUTS:
*       cntrlPcktInfo       - (pointer to) control packet info
*
*******************************************************************************/
static GT_VOID snetCht3URpfCheck
(
    IN  SKERNEL_DEVICE_OBJECT                     *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC           *descrPtr,
    IN  GT_U32                                    *routeIndexPtr,
    IN  SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr,
    IN  GT_U32                                     vid1UnicastRoutingEn,
    IN  GT_U32                                     nextHopVid1,
    OUT SNET_CHEETAH2_L3_CNTRL_PACKET_INFO        *cntrlPcktInfo
)
{
    DECLARE_FUNC_NAME(snetCht3URpfCheck);

    GT_U32  *nextHopIpActionPtr;       /*Address of next hop entry*/
    GT_U32  *rpfNextHopEntryPtr;     /*Address of last NextHop entry for uRPF check in ECMP block*/
    GT_U32  fldVal;                  /* field value */
    GT_U32  regAddr;                 /* register address value */
    GT_U32  sipAddr;                 /* register address value rpf check*/
    GT_U32  rpfNextHopEntryAddr;     /* register address for last NextHop entry for uRPF check in ECMP block */
    GT_U32  rpf_check_fld;           /* field for rpf special service check */
    GT_U32  nextHopVid;
    GT_U32  sipUseVidx;
    GT_U32  sipTargetIsTrunk;
    GT_U32  sipTrgPort;
    GT_U32  sipTrgTrunkId;
    GT_U32  sipTrgDev;
    GT_U32  sipPacketCmd;
    GT_BOOL rpfCheckFail = GT_FALSE; /* RPF check failed flag */
    GT_U32  ucRpfEcmpCheckEn;        /* if Vlan-based RPF is enabled for ECMP/QoS block of entries */
    GT_BOOL vlanFound;
    GT_U32  ii;

    /* Unicast IPv4/6 Packet RPF check   */

    if (ipSecurChecksInfoPtr->rpfCheckMode == SNET_RPF_DISABLED_E)
    {
        __LOG(("Unicast IPv4/6 Packet RPF check"));
        return ;
    }

    /* Enable/disabled unicast RPF check for Bridged IPv4/IPv6 Unicast packets */
    if (descrPtr->isIPv4)
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 8, 1, &rpf_check_fld);
    }
    else if (descrPtr->isFcoe)
    {
        smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 2, 1, &rpf_check_fld);
    }
    else
    {
        smemRegFldGet(devObjPtr, SMEM_CHT2_L3_ROUTE_GLB_CONTROL_REG(devObjPtr), 10, 1, &rpf_check_fld);
    }

    if (!((descrPtr->ipServiceFlag == GT_TRUE) ||
       ( descrPtr->ipServiceFlag == GT_FALSE && rpf_check_fld)))
    {
        return ;
    }

    /* packet SIP should have match in route table */
    if (routeIndexPtr[1] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS )
    {
        __LOG(("packet SIP should have match in route table"));
        return ;
    }

    /* fetch route table entry  */
    __LOG(("fetch route table entry"));
    sipAddr = SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(devObjPtr,routeIndexPtr[1]);
    nextHopIpActionPtr = smemMemGet(devObjPtr, sipAddr);

    if (ipSecurChecksInfoPtr->rpfCheckMode == SNET_RPF_VLAN_BASED_E)
    {
        nextHopVid = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    snetFieldValueGet(nextHopIpActionPtr, 22, 13) :
                    snetFieldValueGet(nextHopIpActionPtr, 20, 12);

        if (ipSecurChecksInfoPtr->sipNumberOfPaths == 0)
        {
            if (descrPtr->eVid != nextHopVid)
            {
                rpfCheckFail = GT_TRUE;
                __LOG(("sipLttNumberOfPaths == 0 : rpf Check Fail : due to eVid mismatch : descrPtr->eVid[0x%x] != nextHopVid[0x%x] \n",
                    descrPtr->eVid, nextHopVid));
            }
            /* SIP5 : option to check VID1 too*/
            else if(vid1UnicastRoutingEn && (descrPtr->vid1 != nextHopVid1))
            {
                rpfCheckFail = GT_TRUE;
                __LOG(("sipLttNumberOfPaths == 0 : rpf Check Fail : due to VID1 mismatch : descrPtr->vid1[0x%x] != nextHopVid1[0x%x] \n",
                    descrPtr->vid1, nextHopVid1));
            }
        }
        else
        if ( (ipSecurChecksInfoPtr->sipNumberOfPaths <= 7) &&
             (ipSecurChecksInfoPtr->sipBaseRouteEntryIndex != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS) &&
             (SKERNEL_IS_CHEETAH3_DEV(devObjPtr)) )
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr), 5, 1, &ucRpfEcmpCheckEn);
            if (ucRpfEcmpCheckEn == 1)
            {
                /* fetch last NextHop entry for uRPF check in ECMP block, which contain VLAN-ID's  */
                __LOG(("fetch last NextHop entry for uRPF check in ECMP block, which contain VLAN-ID's"));
                rpfNextHopEntryAddr =
                    SMEM_CHT2_NEXT_HOP_ENTRY_TBL_MEM(devObjPtr,
                        (ipSecurChecksInfoPtr->sipBaseRouteEntryIndex + ipSecurChecksInfoPtr->sipNumberOfPaths + 1));
                rpfNextHopEntryPtr = smemMemGet(devObjPtr, rpfNextHopEntryAddr);

                /* at least one Vlan-ID should match packet VID to pass RPF check */
                __LOG(("at least one Vlan-ID should match packet VID to pass RPF check"));
                vlanFound = GT_FALSE;
                for (ii = 0; ii <= ipSecurChecksInfoPtr->sipNumberOfPaths; ii++)
                {
                    nextHopVid = snetFieldValueGet(rpfNextHopEntryPtr, ii*12, 12);
                    if (nextHopVid == descrPtr->eVid)
                    {
                        if(vid1UnicastRoutingEn && (descrPtr->vid1 != nextHopVid1))
                        {
                            __LOG(("eVid matched but VID1 mismatch : descrPtr->vid1[0x%x] != nextHopVid1[0x%x] \n",
                                    descrPtr->vid1, nextHopVid1));
                            continue;
                        }

                        vlanFound = GT_TRUE;
                        __LOG_PARAM(vlanFound);
                        break;
                    }
                }

                if (vlanFound == GT_FALSE)
                {
                    rpfCheckFail = GT_TRUE;
                }
            }
        }
    } /* End  VLAN-Based RPF  CHECK   */


    /*  Port-based RPF check    */
    __LOG(("Port-based RPF check"));
    if ( (devObjPtr->ipvxSupport.supportPerVlanURpfMode) &&
         (ipSecurChecksInfoPtr->rpfCheckMode == SNET_RPF_PORT_BASED_E) &&
         (ipSecurChecksInfoPtr->sipNumberOfPaths == 0) )
    {
        sipUseVidx = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    snetFieldValueGet(nextHopIpActionPtr, 35, 1) :
                    snetFieldValueGet(nextHopIpActionPtr, 32, 1);
        sipTargetIsTrunk = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    snetFieldValueGet(nextHopIpActionPtr, 36, 1) :
                    snetFieldValueGet(nextHopIpActionPtr, 33, 1);
        if (sipUseVidx == 0)  /* packet forwarded to trunk or port */
        {
            __LOG(("packet forwarded to trunk or port"));
            if (descrPtr->origIsTrunk)  /* packet src is trunk */
            {
                __LOG(("packet src is trunk "));
                sipTrgTrunkId = snetFieldValueGet(nextHopIpActionPtr, 38, 7);
                __LOG_PARAM(sipTrgTrunkId);
                if (!((sipTargetIsTrunk == 1) &&
                      (descrPtr->origSrcEPortOrTrnk == sipTrgTrunkId)))
                {
                    rpfCheckFail = GT_TRUE;
                }
            }
            else
            {
                __LOG(("packet src is port "));
                sipTrgPort = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    snetFieldValueGet(nextHopIpActionPtr, 37, 13) :
                    snetFieldValueGet(nextHopIpActionPtr, 34, 6);
                __LOG_PARAM(sipTrgPort);
                sipTrgDev = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                    snetFieldValueGet(nextHopIpActionPtr, 50, 10) :
                    snetFieldValueGet(nextHopIpActionPtr, 40, 5);
                __LOG_PARAM(sipTrgDev);
                if (!((sipTargetIsTrunk == 0) &&
                    (sipTrgDev == descrPtr->srcDev) &&
                    (sipTrgPort == descrPtr->origSrcEPortOrTrnk)))
                {
                    rpfCheckFail = GT_TRUE;
                }
            }
        }
    } /* End  PORT-BASED RPF   CHECK   */


    /* LOOSE uRPF check    */
    __LOG(("LOOSE uRPF check"));
    if ( (devObjPtr->ipvxSupport.supportPerVlanURpfMode) &&
         (ipSecurChecksInfoPtr->rpfCheckMode == SNET_RPF_LOOSE_E) &&
         (ipSecurChecksInfoPtr->sipNumberOfPaths == 0) )
    {
        sipPacketCmd = snetFieldValueGet(nextHopIpActionPtr, 0, 3);
        if ((sipPacketCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E) &&
            (sipPacketCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
        {
            rpfCheckFail = GT_TRUE;
        }
    } /* End  LOOSE uRPF   CHECK   */


    if (rpfCheckFail)
    {
        __LOG(("rpfCheckFail = GT_TRUE"));
        if(descrPtr->isFcoe)
        {
            smemRegFldGet(devObjPtr, SMEM_LION3_IPVX_FCOE_GLOBAL_CONFIG_REG(devObjPtr), 19, 3, &fldVal);
        }
        else
        {
            /* The command assigned to routed IPv4/IPv6 packets    */
            __LOG(("The command assigned to routed IPv4/IPv6 packets"));
            regAddr = (descrPtr->isIPv4) ?
                SMEM_CHT2_L3_ROUTE_CONTROL1_IPV4_REG(devObjPtr):
                SMEM_CHT2_L3_ROUTE_CONTROL1_IPV6_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 9, 3, &fldVal);
        }

        cntrlPcktInfo->ipvUcRpfRailCmd = fldVal;
        logRouterPacketCommand(devObjPtr,
            NAME_AND_VALUE_MAC(cntrlPcktInfo->ipvUcRpfRailCmd));
    }
}
/*******************************************************************************
*   snetLion3TcamTest
*
* DESCRIPTION:
*       sip5 function that does test lookup, checks match (1 hit)
*
*       Usage:
*               cpssInitSystem 28,1,0
*               snetLion3TcamTest 1
*
* INPUTS:
*       deviceId - the simulation device Id (1 should be used)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK    - test passes well
*       GT_FAIL  - test failed
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS snetLion3TcamTest
(
    IN  GT_U32      deviceId
)
{
#define SIP5_TCAM_TEST_NUM_OF_TESTS_CNS  1000

    SKERNEL_DEVICE_OBJECT    *devObjPtr;

    GT_U32  *xDataPtr, *yDataPtr; /* pointer to the memory of X, Y lines */
    GT_U32   keyArrayPtr[SIP5_TCAM_MAX_SIZE_OF_KEY_ARRAY_CNS] = {0};
    GT_U32   resultArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS];

    GT_U32   line = 0, index, keyByte, keyBytePosition, keyByteNum;
    GT_U32   numOfHits, sizeBits, keyAlign = 0;
    GT_U32   floorNum, entryNum, bankNum, tmpBankNum, testNum, totalTestNum = 0, keyWordCounter;

    SIP5_TCAM_KEY_SIZE_ENT keySize = SIP5_TCAM_KEY_SIZE_80B_E;

    GT_BOOL  isTestMustFail = GT_FALSE;

    /* get device pointer */
    devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);

    /* Configure group0 to list of all segments (24 half floors) */
    for(floorNum = 0; floorNum < SIP5_TCAM_MAX_NUM_OF_FLOORS_CNS; floorNum++)
    {
        GT_U32 regAddr = SMEM_LION3_TCAM_HIT_NUM_AND_GROUP_SEL_FLOOR_REG(devObjPtr, floorNum);
        smemRegFldSet(devObjPtr, regAddr, 0, 3, 0);
        smemRegFldSet(devObjPtr, regAddr, 3, 3, 0);
    }

    for(testNum = 0; testNum < SIP5_TCAM_TEST_NUM_OF_TESTS_CNS; testNum++)
    {
        for(keySize = SIP5_TCAM_KEY_SIZE_10B_E; keySize < SIP5_TCAM_KEY_SIZE_LAST_E; keySize++)
        {
            if(keySize == 7)
            { /* non existent key size */
                continue;
            }

            isTestMustFail = GT_FALSE;

            totalTestNum++;

            /* 4 lsb bits mean key size  */
            sizeBits = snetLion3TcamGetKeySizeBits(keySize);

            /* get random line */
            floorNum = rand() % SIP5_TCAM_MAX_NUM_OF_FLOORS_CNS;
            entryNum = rand() % SIP5_TCAM_NUM_OF_X_LINES_IN_BANK_CNS;
            bankNum  = rand() % SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS;

            /* init line num */
            SMEM_U32_SET_FIELD(line,13, 4, floorNum);/*floor_num[3:0] */
            SMEM_U32_SET_FIELD(line, 5, 8, entryNum);/*array_addr[7:0]*/
            SMEM_U32_SET_FIELD(line, 1, 4, bankNum); /*bank[3:0]*/

            index = line / 2;

            /* check keys limitations */
            if(SIP5_TCAM_KEY_SIZE_10B_E != keySize) /* not relevant for 10b keys */
            {
                switch(keySize)
                {
                    case SIP5_TCAM_KEY_SIZE_20B_E: keyAlign =  2; break;
                    case SIP5_TCAM_KEY_SIZE_30B_E: keyAlign =  3; break;
                    case SIP5_TCAM_KEY_SIZE_40B_E:
                    case SIP5_TCAM_KEY_SIZE_50B_E:
                    case SIP5_TCAM_KEY_SIZE_60B_E: keyAlign =  6; break;
                    case SIP5_TCAM_KEY_SIZE_80B_E: keyAlign = 12; break;
                    default: skernelFatalError("wrong key size given\n");
                }

                if(0 != ((index%16) % keyAlign) )
                {
                    /* check no hit */
                    isTestMustFail = GT_TRUE;
                }
            }

            /* calculate key byte using line number */
            keyByte = index % 256 /* one byte can fit 256 values */;
            /* set last byte of the key to keyByte value */
            keyBytePosition = ((GT_U32)keySize * 10) - 1;

            /* set the key bytes */

            for(keyByteNum = 0; keyByteNum <= keyBytePosition; keyByteNum++)
            {
                snetFieldValueSet(keyArrayPtr, keyByteNum * BITS_IN_BYTE, 1, 1);
            }

            /* set byte value to last key byte (in the last word) */
            snetFieldValueSet(keyArrayPtr, keyBytePosition * BITS_IN_BYTE, BITS_IN_BYTE, keyByte);

            /* put key to the memory */
            for(tmpBankNum = bankNum; tmpBankNum < ((GT_U32)keySize + bankNum); tmpBankNum++)
            {
                SMEM_U32_SET_FIELD(line, 1, 4, tmpBankNum); /*bank[3:0]*/
                xDataPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_MEMORY_TBL_MEM(devObjPtr, line));
                yDataPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_MEMORY_TBL_MEM(devObjPtr, line+1));

                /* set size bits for each chunk */
                *xDataPtr = sizeBits;

                for(keyWordCounter = 0; keyWordCounter < 2; keyWordCounter++)
                {
                    /* Prepare x words */
                    for(keyByteNum = 0; keyByteNum < 4; keyByteNum++)
                    {
                        *(xDataPtr+keyWordCounter) |= (1 << (BITS_IN_BYTE*keyByteNum+4)); /*4 bits were used for size*/
                    }

                    /* Prepare y words */
                    *(yDataPtr+keyWordCounter) = ~*(xDataPtr+keyWordCounter);
                }

                if( (tmpBankNum - bankNum + 1) == (GT_U32)keySize  )
                {
                    /* set last key byte (10th byte) to the memory */
                    *(xDataPtr+2) |= (1 << 4);           /* 9th byte = 1 */
                    *(xDataPtr+2) |= (keyByte << (BITS_IN_BYTE+4)); /* 4 bits were used for size */
                    *(yDataPtr+2) = ~*(xDataPtr+2);
                }
            }

            /* search the key */
            numOfHits = sip5TcamLookup(devObjPtr, SIP5_TCAM_CLIENT_IPCL0_E, keyArrayPtr, keySize, resultArr);

            if(isTestMustFail && numOfHits)
            {
                simGeneralPrintf("test must fail, but got hit: %d \n", numOfHits);
                return GT_FAIL;
            }
            else if (isTestMustFail && !numOfHits)
            {
                /* no hits - expected behavior, do nothing */
            }
            else
            {
                /* check test result */
                switch(numOfHits)
                {
                    default:
                        simGeneralPrintf("got more hits than expected: %d \n", numOfHits);
                        return GT_FAIL;
                    case 0:
                        simGeneralPrintf("no hits found, test failed\n");

                        /* debug info */
                        simGeneralPrintf("testNum : %d \n", testNum);
                        simGeneralPrintf("floorNum: %d \n", floorNum);
                        simGeneralPrintf("entryNum: %d \n", entryNum);
                        simGeneralPrintf("bankNum : %d \n", bankNum);
                        simGeneralPrintf("line    : %d \n", line);
                        simGeneralPrintf("keyByte : %d \n", keyByte);
                        simGeneralPrintf("keyBytePosition : %d \n", keyBytePosition);
                        simGeneralPrintf("\n");

                        return GT_FAIL;
                    case 1:
                        if( !(SMEM_U32_GET_FIELD(resultArr[0], 12, 4) == floorNum) )
                        {
                            simGeneralPrintf("floorNum doesn't match\n");
                            return GT_FAIL;
                        }
                        if( !(SMEM_U32_GET_FIELD(resultArr[0], 4, 8) == entryNum) )
                        {
                            simGeneralPrintf("entryNum doesn't match\n");
                            return GT_FAIL;
                        }
                        if( !(SMEM_U32_GET_FIELD(resultArr[0], 0, 4) == bankNum) )
                        {
                            simGeneralPrintf("bankNum doesn't match\n");
                            return GT_FAIL;
                        }
                }
            }

            /* restore configuration */
            for(tmpBankNum = bankNum; tmpBankNum < ((GT_U32)keySize + bankNum); tmpBankNum++)
            {
                SMEM_U32_SET_FIELD(line, 1, 4, tmpBankNum); /*bank[3:0]*/
                xDataPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_MEMORY_TBL_MEM(devObjPtr, line));
                yDataPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_MEMORY_TBL_MEM(devObjPtr, line+1));

                /* restore size bits for each chunk */
                *xDataPtr = 0xf;
                *yDataPtr = 0xf;

                if( (tmpBankNum - bankNum + 1) == (GT_U32)keySize  )
                {
                    /* restore last key byte (10th byte) to the memory */
                    *(xDataPtr+2) = 0;
                    *(yDataPtr+2) = 0;
                }
            }
        } /* key size iteration */
    } /* tests iteration */

    if(SIP5_TCAM_TEST_NUM_OF_TESTS_CNS*7 /*key sizes */ == totalTestNum)
    {
        simGeneralPrintf("\nAll tests passed, \ntotal num of tcam tests: %d\n\n", totalTestNum);
        return GT_OK;
    }

    return GT_FAIL;

#undef SIP5_TCAM_TEST_NUM_OF_TESTS_CNS

}

/*******************************************************************************
*   lion3IpvxLocalDevSrcEportBitsGet
*
* DESCRIPTION:
*        get the value relate to the <localDevSrcEPort> from the IPvX ePort table
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*                   hold : descrPtr->eArchExtInfo.localDevSrcEPort ,descrPtr->eArchExtInfo.ipvxIngressEPortTablePtr
*        fieldName  - field name
*        numBits   - number of bits of field (0..31)
* OUTPUTS:
*
* COMMENTS:
*
*******************************************************************************/
GT_U32  lion3IpvxLocalDevSrcEportBitsGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN IPVX_PER_SRC_PORT_FIELDS_ENT fieldName
)
{
    GT_U32  finalStartBit;
    GT_U32 value;

    GT_U32  startBit_sip5[] = {
        0,/*IPVX_PER_SRC_PORT_FIELD_ROUTER_PER_EPORT_SIP_SA_CHECK_ENABLE_E,*/
        1,/*IPVX_PER_SRC_PORT_FIELD_FCOE_FORWARDING_ENABLE_E,*/
        2,/*IPVX_PER_SRC_PORT_FIELD_IPV4_UC_ROUTING_ENABLE_E,*/
        3,/*IPVX_PER_SRC_PORT_FIELD_IPV4_MC_ROUTING_ENABLE_E,*/
        4,/*IPVX_PER_SRC_PORT_FIELD_IPV6_UC_ROUTING_ENABLE_E,*/
        5,/*IPVX_PER_SRC_PORT_FIELD_IPV6_MC_ROUTING_ENABLE_E,*/
        7/*IPVX_PER_SRC_PORT_FIELD_DISABLE_SIP_LOOKUP_E*/};
    GT_U32  startBit_sip5_10[] = {
        0,/*IPVX_PER_SRC_PORT_FIELD_ROUTER_PER_EPORT_SIP_SA_CHECK_ENABLE_E,*/
        1,/*IPVX_PER_SRC_PORT_FIELD_FCOE_FORWARDING_ENABLE_E,*/
        2,/*IPVX_PER_SRC_PORT_FIELD_IPV4_UC_ROUTING_ENABLE_E,*/
        3,/*IPVX_PER_SRC_PORT_FIELD_IPV4_MC_ROUTING_ENABLE_E,*/
        4,/*IPVX_PER_SRC_PORT_FIELD_IPV6_UC_ROUTING_ENABLE_E,*/
        5,/*IPVX_PER_SRC_PORT_FIELD_IPV6_MC_ROUTING_ENABLE_E,*/
        6/*IPVX_PER_SRC_PORT_FIELD_DISABLE_SIP_LOOKUP_E*/};
    char * fieldNames[] = {
        STR(IPVX_PER_SRC_PORT_FIELD_ROUTER_PER_EPORT_SIP_SA_CHECK_ENABLE_E),
        STR(IPVX_PER_SRC_PORT_FIELD_FCOE_FORWARDING_ENABLE_E),
        STR(IPVX_PER_SRC_PORT_FIELD_IPV4_UC_ROUTING_ENABLE_E),
        STR(IPVX_PER_SRC_PORT_FIELD_IPV4_MC_ROUTING_ENABLE_E),
        STR(IPVX_PER_SRC_PORT_FIELD_IPV6_UC_ROUTING_ENABLE_E),
        STR(IPVX_PER_SRC_PORT_FIELD_IPV6_MC_ROUTING_ENABLE_E),
        STR(IPVX_PER_SRC_PORT_FIELD_DISABLE_SIP_LOOKUP_E)
    };
    GT_U32 *startBitPtr;
    GT_U32  numBits = 1;/* all those are 1 bit size */
    GT_U32  numBitsPerEPort;

    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        startBitPtr = &startBit_sip5_10[0];
    }
    else
    {
        startBitPtr = &startBit_sip5[0];
    }
    /* length according to the last bit */
    numBitsPerEPort = startBitPtr[IPVX_PER_SRC_PORT_FIELD_DISABLE_SIP_LOOKUP_E] + 1;
    /* 16 eports per entry , the 7 fields comes per eport 16 time in the entry */
    finalStartBit = startBitPtr[fieldName] + (descrPtr->eArchExtInfo.localDevSrcEPort % 16) * numBitsPerEPort;

    value = snetFieldValueGet(descrPtr->eArchExtInfo.ipvxIngressEPortTablePtr,
                        finalStartBit,numBits);

    __LOG_NO_LOCATION_META_DATA(("Got field [%s] with value [%d] for src eport [0x%x] \n",
        fieldNames[fieldName],
        value,
        descrPtr->eArchExtInfo.localDevSrcEPort));

    return value;
}

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
)
{
    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_IPVX_ROUTER_NEXT_HOP_E,
            sip5_20IpvxNextHopTableFieldsFormat, sip5IpvxNextHopFieldsTableNames);
    }
    else
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_IPVX_ROUTER_NEXT_HOP_E,
            sip5IpvxNextHopTableFieldsFormat, sip5IpvxNextHopFieldsTableNames);
    }
}

