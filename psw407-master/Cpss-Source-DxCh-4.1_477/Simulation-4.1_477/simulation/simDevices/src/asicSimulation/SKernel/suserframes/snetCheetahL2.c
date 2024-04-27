/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahL2.c
*
* DESCRIPTION:
*       Layer 2 Bridge Engine Frame Processing Simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 160 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/smem/smemCheetah2.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>
/* MAC table entry in bytes */
#define SMEM_CHT_MAC_ENTRY_BYTES            \
    (SMEM_CHT_MAC_TABLE_WORDS * sizeof(GT_U32))
#include <common/Utils/Math/sMath.h>

#define SNET_CHT_VLAN_PORT_MEMBER_BIT(port) \
    ((port) < 4 ) ? (24 + ((port) * 2)) :   \
    ((port) < 20) ? (((port) -  4) * 2) :   \
    ((port) < 26) ? (((port) - 20) * 2) : 8

#define SNET_CHT2_TCP_SANITY_CHECK_CMD(confVal,command) \
        *command = (confVal) ? \
            SKERNEL_EXT_PKT_CMD_HARD_DROP_E : \
            SKERNEL_EXT_PKT_CMD_FORWARD_E

#define SNET_CHT_VLAN_PORT_MEMBER_WORD(port)\
    ((port) < 4 ) ?  0 :                    \
    ((port) < 20) ?  1 :                    \
    ((port) < 26) ?  2 : 3

/* Number of packet commands used in Phase1 command resolution  */
#define MAX_PCKT_COMMANDS           35
/* Number of CPU codes used in Phase1 CPU code resolution */
#define MAX_CPU_CODES               20
/* SIP5 : Number of packet commands used in Phase1 command resolution  */
#define SIP5_MAX_PCKT_COMMANDS           60
/*ch1..Lion2: CPU code names */
static GT_CHAR *cpuCodeReasonName[MAX_CPU_CODES]={
              "igmpCmd","solicitationMcastCmd","icmpV6Cmd","bpduTrapCmd","arpBcastCmd",
              "ieeePcktCmd","ipXMcLinkLocalProtCmd","ripV1Cmd","ciscoPcktCmd",
              "saCmd","daCmd","unknownSaCmd","secAutoLearnCmd","udpBcDestPortCmd",
              "","","","","",""};
/*sip5: CPU code names */
static GT_CHAR *sip5CpuCodeReasonName[SIP5_MAX_PCKT_COMMANDS];
static GT_U32   sip5CpuCodes[SIP5_MAX_PCKT_COMMANDS];    /* CPU code */
static GT_BOOL  sip5SecurityBreachEvent[SIP5_MAX_PCKT_COMMANDS]; /* indication that the event is in the security breach list */

/* sip5 :  macro to build the packet command + cpu code+ cpu code name */
#define SIP5_L2I_COMMAND_AND_CPU_CODE(_packetCmd,_cpuCode,_isSecurityBreach,_index)  \
        sip5Commands[_index] = _packetCmd;                         \
        sip5CpuCodes[_index] = _cpuCode;                           \
        sip5SecurityBreachEvent[_index] = _isSecurityBreach;       \
        sip5CpuCodeReasonName[_index] = #_packetCmd " = " #_cpuCode \

typedef enum {
    SNET_CHEETAH_IGMP_TRAP_MODE_E = 0,
    SNET_CHEETAH_IGMP_SNOOP_MODE_E,
    SNET_CHEETAH_IGMP_MIRROR_MODE_E
} SNET_CHEETAH_IGMP_MODE_ENT;

#define DUMMY_HASH_VALUE_CNS    0xDEADBEEF

/* check if command is drop soft/hard */
#define IS_DROP_COMMAND_MAC(command) \
    (((command) == SKERNEL_EXT_PKT_CMD_HARD_DROP_E || (command) == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ? 1 : 0)

#define IS_HARD_DROP_COMMAND_MAC(command) \
    (((command) == SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ? 1 : 0)

#define IS_SOFT_DROP_COMMAND_MAC(command) \
    (((command) == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ? 1 : 0)

#define IS_TRAP_COMMAND_MAC(command) \
    (((command) == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ? 1 : 0)

#define GT_HW_SIM_MAC_LOW32(macAddr)                \
                (macAddr[5] |          \
                (macAddr[4] << 8) |    \
                (macAddr[3] << 16) |   \
                (macAddr[2] << 24))

#define GT_HW_SIM_MAC_HIGH16(macAddr)           \
        (macAddr[1] | (macAddr[0] << 8))


/*******************************************************************************
*   enum SNET_CHEETAH_BREACH_CODE_ENT
*
*   Description: Enumerator of the packets drop reasons to be counted
*                in Bridge Filter Counter
*
*   Comments:   This enumerator is used for Cheetah only devices
*
*******************************************************************************/
typedef enum {
    /* Cheetah */
    SNET_CHEETAH_BREACH_MAC_DROP_E = 1,
    SNET_CHEETAH_BREACH_INVALID_SA_E = 2,
    SNET_CHEETAH_BREACH_INVALID_VLAN_E = 3,
    SNET_CHEETAH_BREACH_PORT_NOT_VLAN_MEMBER_E = 4,
    SNET_CHEETAH_BREACH_VLAN_RANGE_DROP_E = 5,
    SNET_CHEETAH_BREACH_STATIC_MOVED_IS_SECURITY_BREACH_E = 6,
    SNET_CHEETAH_BREACH_NEW_ADDR_IS_SECURITY_BREACH_E = 7,
    SNET_CHEETAH_BREACH_VLAN_NA_IS_SECURITY_BREACH = 8
} SNET_CHEETAH_BREACH_CODE_ENT;

/*******************************************************************************
*   enum SNET_CHEETAH2_BREACH_CODE_ENT
*
*   Description: Enumerator of the packets drop reasons to be counted
*                in Bridge Filter Counter
*
*   Comments:   This enumerator is used for Cheetah2 and above devices
*
*******************************************************************************/
typedef enum {
    /* Cheetah2 */
    SNET_CHEETAH2_BREACH_RESERVED_E = 0 ,
    SNET_CHEETAH2_BREACH_MAC_DROP_E = 1,
    SNET_CHEETAH2_BREACH_UNKN_SA_E = 2,
    SNET_CHEETAH2_BREACH_INVALID_SA_E = 3,
    SNET_CHEETAH2_BREACH_INVALID_VLAN_E = 4,
    SNET_CHEETAH2_BREACH_PORT_NOT_VLAN_MEMBER_E = 5,
    SNET_CHEETAH2_BREACH_VLAN_RANGE_DROP_E = 6,
    SNET_CHEETAH2_BREACH_STATIC_MOVED_IS_SECURITY_BREACH_E = 7,
    SNET_CHEETAH2_BREACH_ARP_SA_MISMATCH_BREACH_E = 8,
    SNET_CHEETAH2_BREACH_TCP_SYN_BREACH_E = 9,
    SNET_CHEETAH2_BREACH_TCP_OVER_MC_BREACH_E = 10,
    SNET_CHEETAH2_BREACH_BRIDGE_ACCESS_MATRIX_BREACH_E = 11,
    SNET_CHEETAH2_BREACH_SECURE_AUTOMATIC_LEARNING_BREACH_E = 12,
    SNET_CHEETAH2_BREACH_ACCEPTABLE_FRAME_TYPE_BREACH_E = 13,
    SNET_CHEETAH2_BREACH_FRAGMENTED_ICMP_BREACH_E = 14,
    SNET_CHEETAH2_BREACH_TCP_FLAGS_ZERO_BREACH_E = 15,
    SNET_CHEETAH2_BREACH_TCP_FLAGS_FIN_URG_PSH_BREACH_E = 16,
    SNET_CHEETAH2_BREACH_TCP_FLAGS_SYN_FIN_BREACH_E = 17,
    SNET_CHEETAH2_BREACH_TCP_FLAGS_SYN_RST_BREACH_E = 18,
    SNET_CHEETAH2_BREACH_TCP_UDP_DEST_PORT_IS_0_BREACH_E = 19,
    SNET_CHEETAH2_BREACH_VLAN_MRU_DROP_E = 20,
    SNET_CHEETAH2_BRG_RATE_LIMIT_DROP_E = 21,
    SNET_CHEETAH2_BRG_DROP_CNTR_LOCAL_PORT_E = 22,
    SNET_CHEETAH2_BRG_DROP_CNTR_SPAN_TREE_PORT_ST_E = 23,
    SNET_CHEETAH2_BREACH_IP_MULTICAST_DROP_BREACH_E = 24,
    SNET_CHEETAH2_BRG_DROP_CNTR_NON_IP_MC_E = 25,
    SNET_CHEETAH2_BRG_RESERVED_26_E = 26,
    SNET_CHEETAH2_BRG_IEEE_DROP_E = 27,
    SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_NON_IPM_MC_E = 28,
    SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_IPV6_MC_E = 29,
    SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_IPV4_MC_E = 30,
    SNET_CHEETAH2_BRG_DROP_CNTR_UNKNOWN_L2_UC_E = 31,
    SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_IPV4_BC_E = 32,
    SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_NON_IPV4_BC_E = 33,
    /* Cheetah 3 */
    SNET_CHEETAH3_BREACH_AUTO_LEARN_NO_RELEARN_BREACH_E = 34
}SNET_CHEETAH2_BREACH_CODE_ENT;

/* SIP5 : security breach events */
typedef enum{
    SNET_LION3_L2I_SECURITY_BREACH_FDB_entry_command                    = 0x1     ,
    SNET_LION3_L2I_SECURITY_BREACH_PortNewAddress                                 ,
    SNET_LION3_L2I_SECURITY_BREACH_InvalidMACSourceAddress                        ,
    SNET_LION3_L2I_SECURITY_BREACH_VLANNotValid                                   ,
    SNET_LION3_L2I_SECURITY_BREACH_PortNotMemberInVLAN                            ,
    SNET_LION3_L2I_SECURITY_BREACH_VLANRangeDrop                                  ,
    SNET_LION3_L2I_SECURITY_BREACH_MovedStaticAddres                              ,
    SNET_LION3_L2I_SECURITY_BREACH_ARPSAMismatchSecurity                          ,
    SNET_LION3_L2I_SECURITY_BREACH_TCPSYNwithDataSecurity                         ,
    SNET_LION3_L2I_SECURITY_BREACH_TCPoverMCSecurity                              ,
    SNET_LION3_L2I_SECURITY_BREACH_BridgeAccessMatrixSecurity                     ,
    SNET_LION3_L2I_SECURITY_BREACH_SecureAutomaticLearning                        ,
    SNET_LION3_L2I_SECURITY_BREACH_AcceptableFrameType                            ,
    SNET_LION3_L2I_SECURITY_BREACH_FragmentedICMPSecurity                         ,
    SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsZeroSecurity                           ,
    SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsFINURGandPSHSet                        ,
    SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsSYNandFINSet                           ,
    SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsSYNandRSTSet                           ,
    SNET_LION3_L2I_SECURITY_BREACH_TCP_UDPSourceorDestinationPortisZero           ,
    SNET_LION3_L2I_SECURITY_BREACH_VLANNewAddress                                 ,
    SNET_LION3_L2I_SECURITY_BREACH_MacSpoofProtection                             ,
    SNET_LION3_L2I_SECURITY_BREACH_MacSaMoved                                     ,
    SNET_LION3_L2I_SECURITY_BREACH_MacSaIsDa                                      ,
    SNET_LION3_L2I_SECURITY_BREACH_SipIsDip                                       ,
    SNET_LION3_L2I_SECURITY_BREACH_TcpUdpSportIsDport                             ,
    SNET_LION3_L2I_SECURITY_BREACH_TcpFlagsWithFinWithoutAck                      ,
    SNET_LION3_L2I_SECURITY_BREACH_TcpWithoutFullHeader
}SNET_LION3_L2I_SECURITY_BREACH_ENT;

/*******************************************************************************
*   enum SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT
*
*   Description: Enumerator of bypass bridge modes
*
*   SNET_CHEETA_L2I_BYPASS_BRIDGE_NONE_E - bridge engine is not bypassed.
*   SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E -
*                           bridge engine is bypassed except the SA learning
*                           which is allowed.
*   SNET_CHEETA_L2I_BYPASS_BRIDGE_ONLY_FORWARD_DECISION_E -
*                           only the forwarding decision of the bridge engine
*                           is bypassed (SIP5).
*
*******************************************************************************/
typedef enum{
    SNET_CHEETA_L2I_BYPASS_BRIDGE_NONE_E,
    SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E,
    SNET_CHEETA_L2I_BYPASS_BRIDGE_ONLY_FORWARD_DECISION_E
}
SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT;

static GT_CHAR* sip5SecurityBreachNames[] = {
 STR(reserved                                    )
,STR(FDB_entry_command                           )
,STR(PortNewAddress                              )
,STR(InvalidMACSourceAddress                     )
,STR(VLANNotValid                                )
,STR(PortNotMemberInVLAN                         )
,STR(VLANRangeDrop                               )
,STR(MovedStaticAddres                           )
,STR(ARPSAMismatchSecurity                       )
,STR(TCPSYNwithDataSecurity                      )
,STR(TCPoverMCSecurity                           )
,STR(BridgeAccessMatrixSecurity                  )
,STR(SecureAutomaticLearning                     )
,STR(AcceptableFrameType                         )
,STR(FragmentedICMPSecurity                      )
,STR(TCPFlagsZeroSecurity                        )
,STR(TCPFlagsFINURGandPSHSet                     )
,STR(TCPFlagsSYNandFINSet                        )
,STR(TCPFlagsSYNandRSTSet                        )
,STR(TCP_UDPSourceorDestinationPortisZero        )
,STR(VLANNewAddress                              )
,STR(MacSpoofProtection                          )
,STR(MacSaMoved                                  )
,STR(MacSaIsDa                                   )
,STR(SipIsDip                                    )
,STR(TcpUdpSportIsDport                          )
,STR(TcpFlagsWithFinWithoutAck                   )
,STR(TcpWithoutFullHeader                        )
};

#define SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NAME                                     \
     STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VALID                                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NEW_SRC_ADDR_IS_NOT_SECURITY_BREACH    )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IP_MULTICAST_CMD      )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_MULTICAST_CMD        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV6_MULTICAST_CMD        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKNOWN_UNICAST_CMD                    )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IGMP_TO_CPU_EN                        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_MODE                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_MODE                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MIRROR_TO_INGRESS_ANALYZER             )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_ICMP_TO_CPU_EN                        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_CONTROL_TO_CPU_EN                     )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_EN                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_EN                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_BC_CMD               )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IPV4_BC_CMD           )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_UNICAST_ROUTE_EN                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MULTICAST_ROUTE_EN                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_UNICAST_ROUTE_EN                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MULTICAST_ROUTE_EN                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_SITEID                            )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_AUTO_LEARN_DIS                         )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NA_MSG_TO_CPU_EN                           )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MRU_INDEX                              )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_BC_UDP_TRAP_MIRROR_EN                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_CONTROL_TO_CPU_EN                     )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_EVIDX                            )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VRF_ID                                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UC_LOCAL_EN                            )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_VIDX_MODE                        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MC_BC_TO_MIRROR_ANLYZER_IDX       )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MC_TO_MIRROR_ANALYZER_IDX         )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FID                                    )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKOWN_MAC_SA_CMD                      )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FCOE_FORWARDING_EN                     )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX_MODE                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX                        )

char * lion3L2iIngressVlanFieldsTableNames[SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NAME};

SNET_ENTRY_FORMAT_TABLE_STC lion3L2iIngressVlanTableFieldsFormat[SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VALID                                */
     STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NEW_SRC_ADDR_IS_NOT_SECURITY_BREACH  */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IP_MULTICAST_CMD    */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_MULTICAST_CMD      */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV6_MULTICAST_CMD      */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKNOWN_UNICAST_CMD                  */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IGMP_TO_CPU_EN                      */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_MODE               */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_MODE               */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MIRROR_TO_INGRESS_ANALYZER           */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_ICMP_TO_CPU_EN                      */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_CONTROL_TO_CPU_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_EN                 */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_EN                 */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_BC_CMD             */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IPV4_BC_CMD         */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_UNICAST_ROUTE_EN                */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MULTICAST_ROUTE_EN              */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_UNICAST_ROUTE_EN                */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MULTICAST_ROUTE_EN              */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_SITEID                          */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_AUTO_LEARN_DIS                       */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NA_MSG_TO_CPU_EN                         */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MRU_INDEX                            */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_BC_UDP_TRAP_MIRROR_EN                */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_CONTROL_TO_CPU_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_EVIDX                          */
    ,STANDARD_FIELD_MAC(16)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VRF_ID                               */
    ,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UC_LOCAL_EN                          */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_VIDX_MODE                      */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MC_BC_TO_MIRROR_ANLYZER_IDX     */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MC_TO_MIRROR_ANALYZER_IDX       */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FID                                  */
    ,STANDARD_FIELD_MAC(13)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKOWN_MAC_SA_CMD                    */
    ,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FCOE_FORWARDING_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX_MODE                 */
    ,STANDARD_FIELD_MAC(2)
/*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX                      */
    ,STANDARD_FIELD_MAC(16)
};

#define SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NAME                                     \
      STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_MSG_TO_CPU_EN                      )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_AUTO_LEARN_DIS                        )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_STORM_PREV_EN                      )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NEW_SRC_ADDR_SECURITY_BREACH          )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_VLAN_INGRESS_FILTERING                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ACCEPT_FRAME_TYPE                     )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UC_LOCAL_CMD                          )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UNKNOWN_SRC_ADDR_CMD                  )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_EN                         )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_VLAN_IS_TRUNK                    )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_EPORT_TRUNK_NUM        )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_DEV                    )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ALL_PKT_TO_PVLAN_UPLINK_EN            )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IGMP_TRAP_EN                          )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_BC_TRAP_EN                        )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ON_TRAP_IEEE_RSRV_MC         )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IEEE_RSVD_MC_TABLE_SEL                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_SOURCE_ID_FORCE_MODE             )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_SRC_ID                                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_MAC_SA_MIS_DROP_EN                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNKNOWN_UC_FILTER_CMD    )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNREG_MC_FILTER_CMD      )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_BC_FILTER_CMD            )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_STP_STATE_MODE          )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_SPANNING_TREE_STATE     )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV4_CONTROL_TRAP_EN                  )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV6_CONTROL_TRAP_EN                  )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_BC_UDP_TRAP_OR_MIRROR_EN              )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ORIG_TAG1_VID                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV4_ROUTING_EN                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV6_ROUTING_EN                )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_FCOE_ROUTING_EN                   )\
     ,STR(SMEM_LION3_L2I_EPORT_TABLE_FIELDS_MOVED_MAC_SA_CMD                      )

char * lion3L2iEPortFieldsTableNames[SMEM_LION3_L2I_EPORT_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NAME};

SNET_ENTRY_FORMAT_TABLE_STC lion3L2iEPortTableFieldsFormat[SMEM_LION3_L2I_EPORT_TABLE_FIELDS___LAST_VALUE___E] =
{
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_MSG_TO_CPU_EN                         */
     STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_AUTO_LEARN_DIS                           */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_STORM_PREV_EN                         */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NEW_SRC_ADDR_SECURITY_BREACH             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_VLAN_INGRESS_FILTERING                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ACCEPT_FRAME_TYPE                        */
    ,STANDARD_FIELD_MAC(2)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UC_LOCAL_CMD                             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UNKNOWN_SRC_ADDR_CMD                     */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_EN                            */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_VLAN_IS_TRUNK                       */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_EPORT_TRUNK_NUM           */
    ,STANDARD_FIELD_MAC(13)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_DEV                       */
    ,STANDARD_FIELD_MAC(10)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ALL_PKT_TO_PVLAN_UPLINK_EN               */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IGMP_TRAP_EN                             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_BC_TRAP_EN                           */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ON_TRAP_IEEE_RSRV_MC            */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IEEE_RSVD_MC_TABLE_SEL                   */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_SOURCE_ID_FORCE_MODE                */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_SRC_ID                                   */
    ,STANDARD_FIELD_MAC(12)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_MAC_SA_MIS_DROP_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNKNOWN_UC_FILTER_CMD       */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNREG_MC_FILTER_CMD         */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_BC_FILTER_CMD               */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_STP_STATE_MODE             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_SPANNING_TREE_STATE        */
    ,STANDARD_FIELD_MAC(2)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV4_CONTROL_TRAP_EN                     */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV6_CONTROL_TRAP_EN                     */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_BC_UDP_TRAP_OR_MIRROR_EN                 */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ORIG_TAG1_VID                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV4_ROUTING_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV6_ROUTING_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_FCOE_ROUTING_EN                      */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_MOVED_MAC_SA_CMD                         */
    ,STANDARD_FIELD_MAC(3)
};

SNET_ENTRY_FORMAT_TABLE_STC sip5_20L2iEPortTableFieldsFormat[SMEM_LION3_L2I_EPORT_TABLE_FIELDS___LAST_VALUE___E] =
{
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_MSG_TO_CPU_EN                         */
     STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_AUTO_LEARN_DIS                           */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_STORM_PREV_EN                         */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NEW_SRC_ADDR_SECURITY_BREACH             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_VLAN_INGRESS_FILTERING                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ACCEPT_FRAME_TYPE                        */
    ,STANDARD_FIELD_MAC(2)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UC_LOCAL_CMD                             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UNKNOWN_SRC_ADDR_CMD                     */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_EN                            */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_VLAN_IS_TRUNK                       */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_EPORT_TRUNK_NUM           */
    ,STANDARD_FIELD_MAC(14)/* was 13 in sip5 */
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_DEV                       */
    ,STANDARD_FIELD_MAC(10)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ALL_PKT_TO_PVLAN_UPLINK_EN               */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IGMP_TRAP_EN                             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_BC_TRAP_EN                           */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ON_TRAP_IEEE_RSRV_MC            */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IEEE_RSVD_MC_TABLE_SEL                   */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_SOURCE_ID_FORCE_MODE                */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_SRC_ID                                   */
    ,STANDARD_FIELD_MAC(12)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_MAC_SA_MIS_DROP_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNKNOWN_UC_FILTER_CMD       */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNREG_MC_FILTER_CMD         */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_BC_FILTER_CMD               */
    ,STANDARD_FIELD_MAC(3)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_STP_STATE_MODE             */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_SPANNING_TREE_STATE        */
    ,STANDARD_FIELD_MAC(2)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV4_CONTROL_TRAP_EN                     */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV6_CONTROL_TRAP_EN                     */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_BC_UDP_TRAP_OR_MIRROR_EN                 */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ORIG_TAG1_VID                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV4_ROUTING_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV6_ROUTING_EN                   */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_FCOE_ROUTING_EN                      */
    ,STANDARD_FIELD_MAC(1)
/*   SMEM_LION3_L2I_EPORT_TABLE_FIELDS_MOVED_MAC_SA_CMD                         */
    ,STANDARD_FIELD_MAC(3)
};


#define SMEM_LION3_FDB_FDB_TABLE_FIELDS_NAME                                     \
      STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID                                 )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP                                  )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE                                   )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE                        )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID                                   )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_MAC_ADDR                              )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID                                )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_5_0                         )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK                              )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DIP                                   )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP                                   )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX                                  )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM                             )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM                             )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED                          )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_RESERVED_99_103                       )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_11_9                        )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ACCESS_LEVEL                       )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_ACCESS_LEVEL                       )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_8_6                         )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_RESERVED_116_118                      )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1                             )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC                             )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE                              )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_CMD                                )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_CMD                                )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ROUTE                              )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN                            )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX                  )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX                  )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE                 )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER  )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER  )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID                                               )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK                                     )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DEST_SITE_ID                                    )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID                                            )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP                                             )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT                                 )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION_FOR_IPV4_PACKETS )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX                     )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN                               )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX                                    )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE                               )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_UP                                            )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP                                          )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX                                    )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN                                )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL                                     )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ECMP_REDIRECT_EXCEPTION_MIRROR                       )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MTU_INDEX                                            )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX                                             )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK                                         )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID                                         )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT                                            )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_EVIDX                                                )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV                                              )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN                                       )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL                                      )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE                                          )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR                                     )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_PTR                                        )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DIP               )\
     ,STR(SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM       )\
     ,STR(SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT            )


static char * lion3FdbFdbFieldsTableNames[SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_FDB_FDB_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3FdbFdbTableFieldsFormat[SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID */
    STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP  */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE   */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE   */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID              */
    ,STANDARD_FIELD_MAC(13)

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_MAC_ADDR     */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         48,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID}
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID       */
        ,STANDARD_FIELD_MAC(10)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_5_0*/
        ,STANDARD_FIELD_MAC(6)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK     */
        ,STANDARD_FIELD_MAC(1)

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DIP          */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         32,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID}
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP          */
        ,STANDARD_FIELD_MAC(32)


    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX         */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP}

    /*        SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM      */
            ,{FIELD_SET_IN_RUNTIME_CNS,
             12,
             SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK}

    /*        SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM      */
            ,{FIELD_SET_IN_RUNTIME_CNS,
             13,
             SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK}



    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED       */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         8,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX}


    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_RESERVED_99_103    */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         5,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX}

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_11_9     */
        ,STANDARD_FIELD_MAC(3)



    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ACCESS_LEVEL    */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         3,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED}
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_ACCESS_LEVEL    */
        ,STANDARD_FIELD_MAC(3)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_8_6      */
        ,STANDARD_FIELD_MAC(3)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_RESERVED_116_118   */
        ,STANDARD_FIELD_MAC(3)

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1          */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED}

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC              */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1}
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE               */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_CMD                 */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_CMD                 */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ROUTE               */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN             */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX   */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX   */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE  */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER   */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER   */
    ,STANDARD_FIELD_MAC(1)


    /********************** ipv4 and fcoe routing fields **********************/

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DEST_SITE_ID*/
        ,STANDARD_FIELD_MAC(1)


        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         24,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         32,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID}


        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION_FOR_IPV4_PACKETS*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX*/
        ,STANDARD_FIELD_MAC(3)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX*/
        ,STANDARD_FIELD_MAC(7)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_UP*/
        ,STANDARD_FIELD_MAC(2)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP*/
        ,STANDARD_FIELD_MAC(2)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX*/
        ,STANDARD_FIELD_MAC(3)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL*/
        ,STANDARD_FIELD_MAC(6)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ECMP_REDIRECT_EXCEPTION_MIRROR*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MTU_INDEX*/
        ,STANDARD_FIELD_MAC(3)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX*/
        ,STANDARD_FIELD_MAC(1)

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID*/
        ,STANDARD_FIELD_MAC(12)

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         13,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_EVIDX*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         10,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN*/
        ,STANDARD_FIELD_MAC(13)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE*/
        ,STANDARD_FIELD_MAC(1)

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         15,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_PTR*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         17,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL}


    /******************** ipv6 routing fields ********************/

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DIP*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     128,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE}

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM*/
    ,STANDARD_FIELD_MAC(4)

    /* SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR}

};

#define SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NAME                                         \
      STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MESSAGE_ID                                )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE                                  )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE                            )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VALID                                     )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SKIP                                      )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE                                       )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0                        )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_CHAIN_TOO_LONG                            )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_OFFSET                           )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ENTRY_FOUND                               )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR                                  )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DEV_ID                                    )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_5_0                             )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DIP                                       )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP                                       )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID                                       )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX                                      )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK                                  )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_EPORT_NUM                                 )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_TRUNK_NUM                                 )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED                              )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_RESERVED_109_113                          )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_11_9                            )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1                                 )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ACCESS_LEVEL                           )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_ACCESS_LEVEL                           )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_8_6                             )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE                     )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SP_UNKNOWN                                )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SEARCH_TYPE                               )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_20_9                       )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MULTIPLE                                  )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ROUTE                                  )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX                      )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX                      )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_STATIC                                 )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_CMD                                    )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_CMD                                    )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER      )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER      )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE                       )\
                                                                                        \
        /* additions for the  format of : MAC NA moved update Message */                \
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_UP0                              )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_IS_MOVED                         )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK                     )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_EPORT                        )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_TRUNK_NUM                    )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_DEVICE                       )\
     ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_SRC_ID                       )\
                                                                                        \
    /* additional fields : for FUTURE support of 'NA move Entry' */                     \
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_ENABLE                       )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_INDEX                        )\
                                                                                        \
 /* fields for FDB Routing */                                                           \
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX                         )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID                                 )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK                       )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID                              )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP                               )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DST_SITE_ID                       )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT                   )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION    )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX       )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN                 )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX                      )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE                 )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_UP                              )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP                            )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX                      )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN                  )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL                       )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ICMP_REDIRECT_EXCEP_MIRROR_EN          )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MTU_INDEX                              )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX                               )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK                               )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EVIDX                                  )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE                          )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TRUNK_NUM                              )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM                              )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN                         )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START                           )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE                            )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_PTR                                )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR                             )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_0                             )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_1                             )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_2                             )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_3                             )\
    ,STR(SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM                       )\
    ,STR(SMEM_SIP5_10_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT                            )


static char * lion3FdbAuMsgFieldsTableNames[SMEM_SIP5_10_FDB_AU_MSG_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3FdbAuMsgTableFieldsFormat[SMEM_SIP5_10_FDB_AU_MSG_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MESSAGE_ID                               */
STANDARD_FIELD_MAC(1)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE                                 */
,STANDARD_FIELD_MAC(3)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE                           */
,STANDARD_FIELD_MAC(3)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VALID                                    */
,STANDARD_FIELD_MAC(1)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SKIP                                     */
,STANDARD_FIELD_MAC(1)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE                                      */
,STANDARD_FIELD_MAC(1)


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0                   */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     9,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE}


/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_CHAIN_TOO_LONG                      */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE}

/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_OFFSET                     */
,     STANDARD_FIELD_MAC(5)
/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ENTRY_FOUND                         */
,     STANDARD_FIELD_MAC(1)


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR                             */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     48,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0}

/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DEV_ID                              */
,     STANDARD_FIELD_MAC(10)
/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_5_0                       */
,     STANDARD_FIELD_MAC(6)


/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DIP                                 */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     32,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0}

/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP                                 */
,     STANDARD_FIELD_MAC(32)


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID                                      */
,{FIELD_SET_IN_RUNTIME_CNS,
 13,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX                                 */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     16,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK                             */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID}

/*        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_EPORT_NUM                        */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         13,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK}

/*        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_TRUNK_NUM                        */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED                         */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         8,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX}

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_RESERVED_111_113                     */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         5,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX}

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_11_9                       */
,    STANDARD_FIELD_MAC(3)


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1                            */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED}

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ACCESS_LEVEL                      */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         3,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED}
/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_ACCESS_LEVEL                      */
 ,   STANDARD_FIELD_MAC(3)
/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_8_6                        */
,    STANDARD_FIELD_MAC(3)


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE                    */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SP_UNKNOWN                               */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SEARCH_TYPE                              */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_20_9                      */
,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MULTIPLE                                 */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ROUTE                                 */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX                     */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX                     */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_STATIC                                */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_CMD                                   */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_CMD                                   */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER     */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER     */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE     */
,STANDARD_FIELD_MAC(1)

/* fields for the format of : MAC NA moved update Message */

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_UP0                             */
,{FIELD_SET_IN_RUNTIME_CNS,
 3,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_OFFSET}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_IS_MOVED                        */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SEARCH_TYPE}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK                    */
,STANDARD_FIELD_MAC(1)

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_EPORT                   */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     13,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_TRUNK_NUM               */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK}


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_DEVICE                      */
,{FIELD_SET_IN_RUNTIME_CNS,
 10,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_EPORT}


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_SRC_ID                      */
,STANDARD_FIELD_MAC(12)


    /* additional fields : for FUTURE support of 'NA move Entry' */

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_ENABLE                     */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_INDEX                      */
,STANDARD_FIELD_MAC(19)


 /* fields for FDB Routing */

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX                 */
,{FIELD_SET_IN_RUNTIME_CNS,
 21,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID                 */
,{FIELD_SET_IN_RUNTIME_CNS,
 12,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK                      */
,STANDARD_FIELD_MAC(1)

 /* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID*/
,{FIELD_SET_IN_RUNTIME_CNS,
 24,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP              */
,{FIELD_SET_IN_RUNTIME_CNS,
 32,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DST_SITE_ID */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION*/
,STANDARD_FIELD_MAC(1)

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX*/
,STANDARD_FIELD_MAC(3)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX*/
,STANDARD_FIELD_MAC(7)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_UP*/
,STANDARD_FIELD_MAC(2)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP*/
,STANDARD_FIELD_MAC(2)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX*/
,STANDARD_FIELD_MAC(3)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL*/
,STANDARD_FIELD_MAC(6)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ICMP_REDIRECT_EXCEP_MIRROR_EN*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MTU_INDEX*/
,STANDARD_FIELD_MAC(3)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK*/
,STANDARD_FIELD_MAC(1)

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EVIDX */
,{FIELD_SET_IN_RUNTIME_CNS,
 16,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE */
,{FIELD_SET_IN_RUNTIME_CNS,
 10,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TRUNK_NUM */
,{FIELD_SET_IN_RUNTIME_CNS,
12,
SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM */
,{FIELD_SET_IN_RUNTIME_CNS,
 13,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN */
,{FIELD_SET_IN_RUNTIME_CNS,
 13,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_PTR*/
,{FIELD_SET_IN_RUNTIME_CNS,
 17,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START}
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR*/
,{FIELD_SET_IN_RUNTIME_CNS,
 15,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE}


/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_0*/
,{FIELD_SET_IN_RUNTIME_CNS,
 32,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX}
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_1*/
,STANDARD_FIELD_MAC(32)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_2*/
,STANDARD_FIELD_MAC(32)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_3*/
,STANDARD_FIELD_MAC(32)

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM*/
,STANDARD_FIELD_MAC(4)

/* SMEM_SIP5_10_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR}


};

/* BOBCAT2 B0 TABLES - Start */
#define SMEM_BOBCAT2_B0_L2I_INGRESS_VLAN_TABLE_FIELDS_NAME                               \
     STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VALID                                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NEW_SRC_ADDR_IS_NOT_SECURITY_BREACH   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IP_MULTICAST_CMD     )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_MULTICAST_CMD       )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV6_MULTICAST_CMD       )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKNOWN_UNICAST_CMD                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IGMP_TO_CPU_EN                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_MODE                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_MODE                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MIRROR_TO_INGRESS_ANALYZER            )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_ICMP_TO_CPU_EN                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_CONTROL_TO_CPU_EN                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_EN                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_EN                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_BC_CMD              )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IPV4_BC_CMD          )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_UNICAST_ROUTE_EN                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MULTICAST_ROUTE_EN               )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_UNICAST_ROUTE_EN                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MULTICAST_ROUTE_EN               )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_SITEID                           )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_AUTO_LEARN_DIS                        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NA_MSG_TO_CPU_EN                      )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MRU_INDEX                             )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_BC_UDP_TRAP_MIRROR_EN                 )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_CONTROL_TO_CPU_EN                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_EVIDX                           )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VRF_ID                                )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UC_LOCAL_EN                           )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_VIDX_MODE                       )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MC_BC_TO_MIRROR_ANLYZER_IDX      )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MC_TO_MIRROR_ANALYZER_IDX        )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FID                                   )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKOWN_MAC_SA_CMD                     )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FCOE_FORWARDING_EN                    )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX_MODE                  )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX                       )\
    ,STR(SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE                   )

char * bobcat2B0L2iIngressVlanTableFieldsNames[
    SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_BOBCAT2_B0_L2I_INGRESS_VLAN_TABLE_FIELDS_NAME};

SNET_ENTRY_FORMAT_TABLE_STC bobcat2B0L2iIngressVlanTableFieldsFormat[
    SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VALID */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NEW_SRC_ADDR_IS_NOT_SECURITY_BREACH */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IP_MULTICAST_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_MULTICAST_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV6_MULTICAST_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKNOWN_UNICAST_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IGMP_TO_CPU_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_MODE */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_MODE */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MIRROR_TO_INGRESS_ANALYZER */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_ICMP_TO_CPU_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_CONTROL_TO_CPU_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_BC_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IPV4_BC_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_UNICAST_ROUTE_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MULTICAST_ROUTE_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_UNICAST_ROUTE_EN_E */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MULTICAST_ROUTE_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_SITEID */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_AUTO_LEARN_DIS */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NA_MSG_TO_CPU_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MRU_INDEX */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_BC_UDP_TRAP_MIRROR_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_CONTROL_TO_CPU_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_EVIDX */
    STANDARD_FIELD_MAC(16),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VRF_ID */
    STANDARD_FIELD_MAC(12),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UC_LOCAL_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_VIDX_MODE */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MC_BC_TO_MIRROR_ANLYZER_IDX */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MC_TO_MIRROR_ANALYZER_IDX */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FID */
    STANDARD_FIELD_MAC(13),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKOWN_MAC_SA_CMD */
    STANDARD_FIELD_MAC(3),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FCOE_FORWARDING_EN */
    STANDARD_FIELD_MAC(1),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX_MODE */
    STANDARD_FIELD_MAC(2),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX */
    STANDARD_FIELD_MAC(16),
    /*SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE */
    STANDARD_FIELD_MAC(1)
};

/* BOBCAT2 B0 TABLES - End */

/* sip5_20 tables : */

static SNET_ENTRY_FORMAT_TABLE_STC sip5_20FdbFdbTableFieldsFormat[SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS___LAST_VALUE___E] =
{
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID */
    STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP  */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE   */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE   */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID              */
    ,STANDARD_FIELD_MAC(13)

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_MAC_ADDR     */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         48,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID}
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID       */
        ,STANDARD_FIELD_MAC(10)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_5_0*/
        ,STANDARD_FIELD_MAC(6)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK     */
        ,STANDARD_FIELD_MAC(1)

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DIP          */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         32,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID}
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP          */
        ,STANDARD_FIELD_MAC(32)


    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX         */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP}

    /*        SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM      */
            ,{FIELD_SET_IN_RUNTIME_CNS,
             12,
             SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK}

    /*        SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM      */
            ,{FIELD_SET_IN_RUNTIME_CNS,
             14,/*was 13 in sip5 */
             SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK}



    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED       */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         8,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX}


    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_RESERVED_99_103    */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         5,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX}

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_11_9     */
        ,STANDARD_FIELD_MAC(3)



    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ACCESS_LEVEL    */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         3,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED}
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_ACCESS_LEVEL    */
        ,STANDARD_FIELD_MAC(3)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_8_6      */
        ,STANDARD_FIELD_MAC(3)
    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_RESERVED_116_118   */
        ,STANDARD_FIELD_MAC(3)

    /*    SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1          */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED}

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC              */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1}
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE               */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_CMD                 */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_CMD                 */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ROUTE               */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN             */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX   */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX   */
    ,STANDARD_FIELD_MAC(3)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE  */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER   */
    ,STANDARD_FIELD_MAC(1)
    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER   */
    ,STANDARD_FIELD_MAC(1)


    /********************** ipv4 and fcoe routing fields **********************/

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DEST_SITE_ID*/
        ,STANDARD_FIELD_MAC(1)


        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         24,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         32,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID}


        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         1,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION_FOR_IPV4_PACKETS*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX*/
        ,STANDARD_FIELD_MAC(3)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX*/
        ,STANDARD_FIELD_MAC(7)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_UP*/
        ,STANDARD_FIELD_MAC(2)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP*/
        ,STANDARD_FIELD_MAC(2)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX*/
        ,STANDARD_FIELD_MAC(3)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL*/
        ,STANDARD_FIELD_MAC(6)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ECMP_REDIRECT_EXCEPTION_MIRROR*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MTU_INDEX*/
        ,STANDARD_FIELD_MAC(3)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX*/
        ,STANDARD_FIELD_MAC(1)

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID*/
        ,STANDARD_FIELD_MAC(12)

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         14,/*was 13 in sip5 */
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_EVIDX*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         16,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         10,
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN*/
        ,STANDARD_FIELD_MAC(13)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL*/
        ,STANDARD_FIELD_MAC(1)
        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE*/
        ,STANDARD_FIELD_MAC(1)

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         16,/*was 15 in sip5*/
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE}

        /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_PTR*/
        ,{FIELD_SET_IN_RUNTIME_CNS,
         18,/*was 17 in sip5*/
         SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL}


    /******************** ipv6 routing fields ********************/

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DIP*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     128,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE}

    /*SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM*/
    ,STANDARD_FIELD_MAC(4)

    /* SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR}

};

static SNET_ENTRY_FORMAT_TABLE_STC sip5_20FdbAuMsgTableFieldsFormat[SMEM_SIP5_10_FDB_AU_MSG_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MESSAGE_ID                               */
STANDARD_FIELD_MAC(1)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE                                 */
,STANDARD_FIELD_MAC(3)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE                           */
,STANDARD_FIELD_MAC(3)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VALID                                    */
,STANDARD_FIELD_MAC(1)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SKIP                                     */
,STANDARD_FIELD_MAC(1)

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE                                      */
,STANDARD_FIELD_MAC(1)


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0                   */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     9,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE}


/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_CHAIN_TOO_LONG                      */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE}

/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_OFFSET                     */
,     STANDARD_FIELD_MAC(5)
/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ENTRY_FOUND                         */
,     STANDARD_FIELD_MAC(1)


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR                             */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     48,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0}

/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DEV_ID                              */
,     STANDARD_FIELD_MAC(10)
/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_5_0                       */
,     STANDARD_FIELD_MAC(6)


/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DIP                                 */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     32,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0}

/*     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP                                 */
,     STANDARD_FIELD_MAC(32)


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID                                      */
,{FIELD_SET_IN_RUNTIME_CNS,
 13,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX                                 */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     16,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK                             */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     1,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID}

/*        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_EPORT_NUM                        */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         14,/*was 13 in sip5 */
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK}

/*        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_TRUNK_NUM                        */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED                         */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         8,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX}

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_RESERVED_111_113                     */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         5,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX}

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_11_9                       */
,    STANDARD_FIELD_MAC(3)


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1                            */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         12,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED}

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ACCESS_LEVEL                      */
        ,{FIELD_SET_IN_RUNTIME_CNS,
         3,
         SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED}
/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_ACCESS_LEVEL                      */
 ,   STANDARD_FIELD_MAC(3)
/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_8_6                        */
,    STANDARD_FIELD_MAC(3)


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE                    */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SP_UNKNOWN                               */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SEARCH_TYPE                              */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_20_9                      */
,STANDARD_FIELD_MAC(12)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MULTIPLE                                 */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ROUTE                                 */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX                     */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX                     */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_STATIC                                */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_CMD                                   */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_CMD                                   */
,STANDARD_FIELD_MAC(3)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER     */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER     */
,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE     */
,STANDARD_FIELD_MAC(1)

/* fields for the format of : MAC NA moved update Message */

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_UP0                             */
,{FIELD_SET_IN_RUNTIME_CNS,
 3,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_OFFSET}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_IS_MOVED                        */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SEARCH_TYPE}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK                    */
,STANDARD_FIELD_MAC(1)

/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_EPORT                   */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     14,/*was 13 in sip5 */
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK}


/*    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_TRUNK_NUM               */
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK}


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_DEVICE                      */
,{FIELD_SET_IN_RUNTIME_CNS,
 10,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_EPORT}


/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_SRC_ID                      */
,STANDARD_FIELD_MAC(12)


    /* additional fields : for FUTURE support of 'NA move Entry' */

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_ENABLE                     */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_INDEX                      */
,STANDARD_FIELD_MAC(19)


 /* fields for FDB Routing */

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX                 */
,{FIELD_SET_IN_RUNTIME_CNS,
 21,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID                 */
,{FIELD_SET_IN_RUNTIME_CNS,
 12,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK                      */
,STANDARD_FIELD_MAC(1)

 /* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID*/
,{FIELD_SET_IN_RUNTIME_CNS,
 24,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID}

/*SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP              */
,{FIELD_SET_IN_RUNTIME_CNS,
 32,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DST_SITE_ID */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION*/
,STANDARD_FIELD_MAC(1)

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX*/
,STANDARD_FIELD_MAC(3)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX*/
,STANDARD_FIELD_MAC(7)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_UP*/
,STANDARD_FIELD_MAC(2)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP*/
,STANDARD_FIELD_MAC(2)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX*/
,STANDARD_FIELD_MAC(3)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL*/
,STANDARD_FIELD_MAC(6)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ICMP_REDIRECT_EXCEP_MIRROR_EN*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MTU_INDEX*/
,STANDARD_FIELD_MAC(3)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK*/
,STANDARD_FIELD_MAC(1)

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EVIDX */
,{FIELD_SET_IN_RUNTIME_CNS,
 16,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE */
,{FIELD_SET_IN_RUNTIME_CNS,
 10,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TRUNK_NUM */
,{FIELD_SET_IN_RUNTIME_CNS,
12,
SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM */
,{FIELD_SET_IN_RUNTIME_CNS,
 14,/*was 13 in sip5 */
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN */
,{FIELD_SET_IN_RUNTIME_CNS,
 13,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM}

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE*/
,STANDARD_FIELD_MAC(1)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_PTR*/
,{FIELD_SET_IN_RUNTIME_CNS,
 18,/*was 17 in sip5 */
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START}
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR*/
,{FIELD_SET_IN_RUNTIME_CNS,
 16,/*was 15 in sip5*/
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE}


/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_0*/
,{FIELD_SET_IN_RUNTIME_CNS,
 32,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX}
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_1*/
,STANDARD_FIELD_MAC(32)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_2*/
,STANDARD_FIELD_MAC(32)
/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_3*/
,STANDARD_FIELD_MAC(32)

/* SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM*/
,STANDARD_FIELD_MAC(4)

/* SMEM_SIP5_10_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT */
,{FIELD_SET_IN_RUNTIME_CNS,
 1,
 SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR}

};


typedef enum {
    SNET_CHEETAH_COUNT_ALL_E = 0,
    SNET_CHEETAH_COUNT_RCV_PORT_E,
    SNET_CHEETAH_COUNT_RCV_VLAN_E,
    SNET_CHEETAH_COUNT_RCV_PORT_VLAN_E
} SNET_CHEETAH_COUNTER_SET_MODE_STC;

static GT_VOID snetChtSaLearningEArchNonVidxInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr
);

static GT_VOID snetChtL2iGetVlanInfo(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr
);

static GT_VOID snetChtL2iSetVlanInfo(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr
);

static GT_VOID snetChtL2iSaLookUp(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    OUT SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr
);

static GT_VOID snetChtL2iDaLookUp(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    OUT SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr
);

static GT_VOID snetChtL2iCntrlAnalyze(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    INOUT SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr
);

static GT_VOID snetChtL2iPortRateLimit(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    OUT SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iFilters(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfo,
    OUT SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr,
    IN SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT   bypassBridge
);

static GT_VOID snetChtL2iPhase1Decision(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfo,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr,
    IN SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT   bypassBridge
);

static GT_VOID snetChtL2iPhase2Decision(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iQoS(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iSecurity(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iCounterSecurityBreach(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtL2iCounters(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT preL2iPacketCmd,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iSaLearning(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr
);

static GT_BOOL snetChtL2iSendNa2Cpu(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL sendFailedNa2Cpu,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr
);

static GT_BOOL snetChtL2iFdbLookup(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    IN GT_BOOL  cpuMessageProcessing,
    IN GT_BOOL  needEmptyIndex,
    OUT GT_U32 ** fdbEntryPtrPtr,
    OUT GT_U32 *  entryIndexPtr,
    OUT GT_U32 *  entryOffsetPtr,
    OUT GT_BOOL * sendFailedNa2CpuPtr,
    INOUT GT_U32   *numValidPlacesPtr
);

static GT_VOID snetChetL2iCalcFdbKeyAndMask(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vlanMode,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    OUT GT_U32 * enrtyKeyPtr,
    OUT GT_U32 * entryMaskPtr
);

static GT_BOOL snetChtL2iFifoAuMsg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U32 * macUpdMsgPtr
);

static GT_BOOL snetChtL2iPciAuMsg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U32 * macUpdMsgPtr
);

static GT_VOID snetChtCreateAuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 * macUpdMsgPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr
);

static GT_VOID snetChtL2iAutoLearnFdbEntry(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN GT_U32 spUnknown,
    IN GT_U32 entryIndex,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr,
    IN GT_BOOL  entryAlreadyExists
);

static GT_VOID snetCht2L2iSecurity(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetCheetahL2iSecurity(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetCheetahL2UcLocalSwitchFilter(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    OUT SNET_CHEETAH_L2I_FILTERS_INFO * filterInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr
);

static GT_VOID sip5L2iSecurity(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    INOUT SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iPortRateLimitDropCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID snetChtL2iSecurityBreachStatusUpdate
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
);

static GT_VOID fdbEntryCommonHashInfoGet
(
    IN      SKERNEL_DEVICE_OBJECT                 *devObjPtr,
    INOUT   SKERNEL_FRAME_CHEETAH_DESCR_STC       *descrPtr,
    IN      SNET_CHEETAH_L2I_VLAN_INFO            *vlanInfoPtr,
    IN      SNET_CHEETAH_L2I_LOOKUP_MODE_ENT      lookUpMode,
    OUT     SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC   *entryInfoPtr
);


/*******************************************************************************
*   snetChtL2iTriggerCheck
*
* DESCRIPTION:
*       Bridge Engine processing of frames - trigger check
* The Bridge engine processing is divided into three categories:
*        1.FDB source address (SA) lookup and processing
*        2.FDB destination address (DA)-based forwarding and filtering
*        3.Control traffic trapping/mirroring to the CPU
*
*      The following types of packets are not subject to any Bridge engine processing:
*        1.A packet that is assigned a HARD DROP by the TTI action entry or the Ingress Policy action.
*        2.A packet that arrives DSA-tagged with a command of TO_ANALYZER, TO_CPU, or FROM_CPU.
*        3.A packet that is assigned <Bypass Ingress Pipe> by TTI Action or by Ingress Policy Action
*
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*       GT_TRUE  - the packet can do bridge
*       GT_FALSE - the packet NOT allowed to enter the bridge.
*
*******************************************************************************/
static GT_BOOL snetChtL2iTriggerCheck
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iTriggerCheck);


    if( descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E )
    {
        if(descrPtr->cpuCode == SNET_LION3_CPU_CODE_L2I_DSA_TAG_LOCAL_DEV)
        {
            /* came to bridge with other then 'hard drop' but filtered by 'local dev filter'.
            this is special filter that works almost on all types of packets */
            __LOG(("End Bridge : Got HARD DROP by 'local dev filter' \n"));
        }
        else
        {
            /* came to bridge already with hard drop */
            __LOG(("SKIP BRIDGE : incoming command 'HARD DROP' \n"));
        }
        return GT_FALSE;
    }
    else if(descrPtr->packetCmd >= SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)/* control from DSA */
    {
        __LOG(("SKIP BRIDGE : DSA-tagged with a command of TO_ANALYZER, TO_CPU, or FROM_CPU \n"));
        return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   isBypassBridge
*
* DESCRIPTION:
*       Bridge Engine processing of frames
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*       None
*
*******************************************************************************/
static SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT isBypassBridge(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(isBypassBridge);

    SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT bIsBypassBridge;

    GT_BIT uDescInBypassIngressPipe = 0;
    GT_BIT uDescInBypassBridge = 0;
    GT_BIT uDescInRouted = 0;
    GT_BIT uRoutedBridgeBypassEn = 0;
    GT_U32 uBridgeBypassMode = 0;

    uDescInBypassIngressPipe = descrPtr->bypassIngressPipe;
    uDescInBypassBridge = descrPtr->bypassBridge;
    uDescInRouted = descrPtr->routed;

    if(devObjPtr->supportEArch)
    {
        /*routed_bridge_by_pass_en*/
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr),13,1, &uRoutedBridgeBypassEn);
        /*bridge_bypass_mode*/
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr),14,1, &uBridgeBypassMode);
    }
    else
    {
        /*routed_bridge_by_pass_en*/
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr),24,1, &uRoutedBridgeBypassEn);
        /* emulate sip5 with this value for legacy behavior  */
        uBridgeBypassMode = 0;
    }

    __LOG_PARAM(uDescInBypassIngressPipe);
    __LOG_PARAM(uDescInBypassBridge);
    __LOG_PARAM(uDescInRouted);
    __LOG_PARAM(uRoutedBridgeBypassEn);
    __LOG_PARAM(uBridgeBypassMode);

    if((uDescInBypassIngressPipe || uDescInBypassBridge) && uBridgeBypassMode)/* sip 5*/
    {
        bIsBypassBridge = SNET_CHEETA_L2I_BYPASS_BRIDGE_ONLY_FORWARD_DECISION_E;
        __LOG(("NOTE: Bridge Bypass only forwarding decision (allow bridge filters) \n"));
    }
    else
    if ((uDescInBypassBridge && uBridgeBypassMode == 0) ||
        (uDescInRouted       && uRoutedBridgeBypassEn ) ||
         uDescInBypassIngressPipe)
    {
        __LOG(("NOTE: Bridge engine is going to be bypassed (except SA learning) \n"));

        if(uDescInBypassIngressPipe)
        {
            __LOG(("Bridge Bypass due to 'bypass Ingress Pipe' \n"));
        }
        else
        if(uDescInBypassBridge == 1 && uBridgeBypassMode == 0)
        {
            __LOG(("Bridge Bypass due to 'bypass bridge' (SA learning only) \n"));
        }
        else if((uDescInRouted == 1) && (uRoutedBridgeBypassEn == 1))
        {
            __LOG(("Bridge Bypass due to 'bypass bridge for routed packets' \n"));
        }

        bIsBypassBridge = SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E;

    }
    else
    {
        bIsBypassBridge = SNET_CHEETA_L2I_BYPASS_BRIDGE_NONE_E;
        __LOG(("No Bridge Bypass\n"));
    }

    return bIsBypassBridge;
}



/*******************************************************************************
*   snetChtL2i
*
* DESCRIPTION:
*       Bridge Engine processing of frames
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*       None
*
*******************************************************************************/
GT_VOID snetChtL2i
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2i);

    GT_U32  index,regAddress,regVal;
    SNET_CHEETAH_L2I_SA_LOOKUP_INFO saLookupInfo;    /* SA lookup results */
    SNET_CHEETAH_L2I_DA_LOOKUP_INFO daLookupInfo;    /* DA lookup results */
    SNET_CHEETAH_L2I_CNTRL_PACKET_INFO cntrlPcktInfo;/* Packet command  */
    SNET_CHEETAH_L2I_FILTERS_INFO filtersInfo;       /* Packet filtering info */
    SKERNEL_EXT_PACKET_CMD_ENT preL2iPacketCmd;      /* Extended Packet command */
    GT_U32 globalEPortVal;/*Value for Global ePort. Refer to Global ePort logic below*/
    GT_U32 globalEPortMask;/*"0" masks the corresponding bit in the <GLOBALePortVal>*/
    SNET_CHEETAH_L2I_LEARN_INFO_STC   learnInfo;/* info for SA learning and UC local switching filtering */
    SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT bypassBridge; /* Bridge Bypass indication and mode */
    GT_BIT  ignoreTriggerCheck = 0; /* indication that need to skip trigger check */
    GT_BIT  controlDsaSubjectToL2iFilters = 0;/* indication that we handle erratum that
                                              control packets subject to bridge processing
                                              as if 'bypassBridge' is set */
    SKERNEL_FRAME_CHEETAH_DESCR_STC *origDescrPtr = NULL;/* pointer to original descriptor */
    GT_U32  DSATag_SrcDevIsLocal_FilterDis;
    GT_U32 brgGlobCntrlReg1Data;    /* bridge global control register1 data */
    GT_BIT dropOnSrcDevIsLocal = 0;

    memset(&cntrlPcktInfo,0,sizeof(cntrlPcktInfo));

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_L2I_E);

    __LOG(("Start 'bridge' \n"));

    /*
        special filter that is done also when 'bypass bridge' is set.
        but ignored when 'bypass ingress pipe' is set.
    */
    /* Global configuration register1 data */
    smemRegGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr), &brgGlobCntrlReg1Data);
    DSATag_SrcDevIsLocal_FilterDis = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 31, 1);
    if(DSATag_SrcDevIsLocal_FilterDis == 0 && /* filter not disabled  */
       descrPtr->bypassIngressPipe == 0 && /*not bypass ingress pipe  */
       descrPtr->marvellTagged &&           /* came from cascade port */
       descrPtr->srcDevIsOwn &&             /* came from 'own dev'    */
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CPU_E && /* not 'from_cpu' DSA tag */
       descrPtr->pktIsLooped == 0)          /* packet was not looped */
    {
        dropOnSrcDevIsLocal = 1;

        if(descrPtr->packetCmd >= SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
        {
            __LOG(("NOTE: Filter packets that origin in this device and came back from cascade port --> to prevent loops (assign HARD DROP) \n"));
            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
            descrPtr->cpuCode   = SNET_LION3_CPU_CODE_L2I_DSA_TAG_LOCAL_DEV;
        }
        else
        {
            /* let the forward/mirror to CPU packet to do other bridge operations too
                like SAlearning ...

                we will assign it the drop later
            */
        }
    }
    else
    {
        if(descrPtr->marvellTagged &&           /* came from cascade port */
           descrPtr->srcDevIsOwn &&               /* came from 'own dev'    */
           descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_HARD_DROP_E) /* not already hard drop */
        {
            __LOG(("Packet is recognized to 'return to src device' but was not filtered : \n"));
            if(descrPtr->bypassIngressPipe)
            {
                __LOG(("because: bypassIngressPipe = 1 \n"));
            }

            if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FROM_CPU_E)
            {
                __LOG(("because: packetCmd = 'FROM_CPU' \n"));
            }

            if(descrPtr->pktIsLooped)
            {
                /* support for 'Fast Stack Fail over Recovery' */
                __LOG(("because: pktIsLooped = 1 \n"));
            }

            if(DSATag_SrcDevIsLocal_FilterDis)
            {
                __LOG(("because: DSATag_SrcDevIsLocal_FilterDis = 1 (the filter is disabled !) \n"));
            }
        }
    }

    if(devObjPtr->errata.l2iFromCpuSubjectToL2iFiltersBridgeBypassModeIsBypassForwardingDecisionOnly &&
       descrPtr->packetCmd >= SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)/* control from DSA */
    {
        /* check if those control packets may still be subject to filters in the L2i */

        /* <bridge bypass mode> */
        if(SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data,14,1))
        {
            __LOG(("WARNING: (potential) ERROR : (although should not be) The control packets with DSA != FORWARD is subject to 'L2i filters' when <bridge bypass mode> = 'Bypass Forwarding Decision only' \n"));
            ignoreTriggerCheck = 1;
            controlDsaSubjectToL2iFilters = 1;
            __LOG_PARAM(ignoreTriggerCheck);
            __LOG_PARAM(controlDsaSubjectToL2iFilters);

            /* duplicate descriptor because we not want to impact original descriptor */
            origDescrPtr = descrPtr;
            descrPtr = snetChtEqDuplicateDescr(devObjPtr,descrPtr);

            /*******************************************************************************/
            /* we will restore all original value at end of bridge , but will keep 'drops' */
            /*******************************************************************************/

            __LOG(("treat packet as 'FORWARD' with bypassBridge = 1 and bypassIngressPipe = 1 \n"));
            /* make it 'ordinarily' packet */
            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
            /* make is 'bypass' */
            /*descrPtr->bypassBridge = 1; --> already set by TTI unit */
            /* not allow SA learning */
            descrPtr->bypassIngressPipe = 1;
        }
    }

    /*The Bridge engine processing is divided into three categories:
        1.FDB source address (SA) lookup and processing
        2.FDB destination address (DA)-based forwarding and filtering
        3.Control traffic trapping/mirroring to the CPU

      The following types of packets are not subject to any Bridge engine processing:
        1.A packet that is assigned a HARD DROP by the TTI action entry or the Ingress Policy action.
        2.A packet that arrives DSA-tagged with a command of TO_ANALYZER, TO_CPU, or FROM_CPU.
        3.A packet that is assigned <Bypass Ingress Pipe> by TTI Action or by Ingress Policy Action
      */
    if(ignoreTriggerCheck == 1)
    {
        __LOG(("WARNING : (although should not do) ALLOW packet to do 'bridge' \n"));
    }
    else
    if(GT_FALSE == snetChtL2iTriggerCheck(devObjPtr,descrPtr))
    {
        __LOG(("SKIP BRIDGE : End 'bridge' \n"));
        return;
    }

    memset(&learnInfo,0,sizeof(learnInfo));

    learnInfo.srcEPortIsGlobal = 0;

    descrPtr->bridgeToMllInfo.virtualSrcPort =  descrPtr->trgEPort;
    descrPtr->bridgeToMllInfo.virtualSrcDev =  descrPtr->trgDev;


    /* source virtual port support */
    if  (descrPtr->pclRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E ||
         descrPtr->ttiRedirectCmd == PCL_TTI_ACTION_REDIRECT_CMD_ASSIGN_SOURCE_LOGICAL_PORT_E)
    {
        /* take the 'SOURCE' info from the TARGET info */
        __LOG(("PCL/TTI 'asign source logical port' --> take the 'SOURCE' info from the TARGET info"));
        learnInfo.FDB_isTrunk = descrPtr->targetIsTrunk;
        if(descrPtr->targetIsTrunk)
        {
            learnInfo.FDB_port_trunk = descrPtr->trgTrunkId;
            learnInfo.FDB_devNum = descrPtr->srcDev;
        }
        else
        {
            learnInfo.FDB_port_trunk = descrPtr->trgEPort;
            learnInfo.FDB_devNum = descrPtr->trgDev;
        }
    }
    else
    {
        learnInfo.FDB_port_trunk = descrPtr->localPortTrunkAsGlobalPortTrunk;
        learnInfo.FDB_isTrunk = descrPtr->origIsTrunk;
        learnInfo.FDB_devNum = descrPtr->srcDev;
    }

    learnInfo.FDB_coreId = (descrPtr->localPortTrunkAsGlobalPortTrunk >> 4);

    if(devObjPtr->supportEArch)
    {
        if(devObjPtr->unitEArchEnable.bridge)
        {
            if(descrPtr->portIsRingCorePort)
            {
                /* The concept of the multi-core FDB (almost identical to LionB)
                   is that ingress processing is performed by the ingress pipe of
                   the initial core.
                   When <ePort & eVLAN> == enabled:
                   At the ingress core all ePort attribute tables are indexed with
                   the <LocalDevSRCePort>, including EQ source ePort table
                   (holding mostly forwarding restrictions).
                   If the FDB DA was not found in the ingress core, the packet is
                   sent in a loop to the following cores for additional FDB DA
                   lookups until the DA lookup is matched (or it is an unknown packet
                   if not found on all cores).
                   But, on each core that is not the ingress core, we perform
                   ingress pipe processing only in the bridge.
                   So the bridge on all non-ingress cores must use attribute
                   assignment based on the original source ePort that was assigned,
                   and not on the core port that the packet was received from previous core.
                   This is why when the packet is received from a ring core port,
                   we use the <OrigSRCePort> for indexing the ingress ePort
                   attribute table only in the bridge.*/
                index = descrPtr->origSrcEPortOrTrnk;
            }
            else
            {
                index = descrPtr->eArchExtInfo.localDevSrcEPort;
            }
            /* Bridge - Ingress ePort Table */
            __LOG(("Bridge - access the Ingress ePort Table , index[%d]",index));
            regAddress = SMEM_LION2_BRIDGE_INGRESS_EPORT_ATTRIBUTE_TBL_MEM(devObjPtr,index);
            descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr = smemMemGet(devObjPtr, regAddress);
        }

        regAddress = SMEM_LION3_L2I_BRIDGE_GLOBAL_E_PORT_VALUE_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddress, &regVal);
        globalEPortVal = SMEM_U32_GET_FIELD(regVal,0,13);

        regAddress = SMEM_LION3_L2I_BRIDGE_GLOBAL_E_PORT_MASK_REG(devObjPtr);;
        smemRegGet(devObjPtr, regAddress, &regVal);
        globalEPortMask = SMEM_U32_GET_FIELD(regVal,0,13);

        if(devObjPtr->unitEArchEnable.bridge && learnInfo.FDB_isTrunk == 0 &&
            ((learnInfo.FDB_port_trunk & globalEPortMask) == globalEPortVal))
        {
            learnInfo.srcEPortIsGlobal = 1;
        }

        __LOG_PARAM(learnInfo.srcEPortIsGlobal);

        snetChtSaLearningEArchNonVidxInfo(devObjPtr,descrPtr,&learnInfo);

    }/*eArch*/

    bypassBridge = isBypassBridge(devObjPtr,descrPtr);

    preL2iPacketCmd = descrPtr->packetCmd;

    /* Get VLAN Info for localDevSrcPort */
    snetChtL2iGetVlanInfo(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo);

    /* Set VLAN Info into descriptor */
    snetChtL2iSetVlanInfo(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo);

    /* SA lookup */
    __LOG(("do SA lookup \n"));
    snetChtL2iSaLookUp(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo,&saLookupInfo,&learnInfo);

    if((bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_NONE_E) ||
       (bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_ONLY_FORWARD_DECISION_E))
    {
        /* DA lookup */
        __LOG(("do DA lookup \n"));
        snetChtL2iDaLookUp(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &daLookupInfo);

        /* Control packets analyze */
        __LOG(("Do Control packets analyze \n"));
        snetChtL2iCntrlAnalyze(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &cntrlPcktInfo);

        /* Ingress filters */
        __LOG(("Get Ingress filters info \n"));
        snetChtL2iFilters(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &saLookupInfo,
                          &daLookupInfo, &cntrlPcktInfo,  &filtersInfo,&learnInfo,
                          bypassBridge);

        /* Ingress port storm rate limiting */
        __LOG(("Get Ingress port storm rate limiting info \n"));
        snetChtL2iPortRateLimit(devObjPtr, descrPtr, &daLookupInfo, &filtersInfo);

        /* Do bridging decision */
        __LOG(("Do bridging decision: phase 1 \n"));
        snetChtL2iPhase1Decision(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &saLookupInfo,
                                 &daLookupInfo, &cntrlPcktInfo,  &filtersInfo, bypassBridge);

        if (descrPtr->egressFilterRegistered == 0)
        {
            __LOG(("Handle Unknown or unregistered packet (descrPtr->egressFilterRegistered == 0) \n"
                          "Do bridging decision: phase 2 \n"));
            /* unknown or unregistered */
            snetChtL2iPhase2Decision(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo,
                                     &filtersInfo);
        }

        /* L2i QoS processing */
        __LOG(("L2i QoS processing \n"));
        snetChtL2iQoS(devObjPtr, descrPtr, &saLookupInfo, &daLookupInfo,
                      &filtersInfo);

        /* Security Breach */
        __LOG(("Check Security Breach \n"));
        snetCheetahL2iSecurity(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &saLookupInfo,
                                &daLookupInfo, &filtersInfo);

        /* Update bridge counters */
        __LOG(("Update bridge counters \n"));
        snetChtL2iCounters(devObjPtr, descrPtr, preL2iPacketCmd,
                           &descrPtr->ingressVlanInfo, &saLookupInfo, &daLookupInfo,
                           &cntrlPcktInfo, &filtersInfo);

        if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr) &&
           filtersInfo.isSecurityBreach)
        {
            /* check to see if need to update the Security Breach status registers */
            snetChtL2iSecurityBreachStatusUpdate( devObjPtr,descrPtr,
                &filtersInfo);
        }
    }
    else /*bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E*/
    {
        __LOG(("bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E (no : DA lookup,Qos,security breach,counters processing)\n"));

        /* Ingress filters */
        __LOG(("Get Ingress filters info \n"));
        snetChtL2iFilters(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &saLookupInfo,
                          &daLookupInfo, &cntrlPcktInfo,  &filtersInfo,&learnInfo,
                          bypassBridge);
    }

    if(dropOnSrcDevIsLocal)
    {
        __LOG(("NOTE: Filter packets that origin in this device and came back from cascade port --> to prevent loops (assign HARD DROP) \n"));
        /* need to do bridge drop counters */
        descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
        descrPtr->cpuCode   = SNET_LION3_CPU_CODE_L2I_DSA_TAG_LOCAL_DEV;
    }

    if(devObjPtr->supportVpls && descrPtr->egressFilterRegistered == 1 /* unknown / registered */)
    {
        __LOG(("VPLS mode and descrPtr->egressFilterRegistered == 1\n"));

        regAddress = SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddress, &regVal);
        /*Meter only flooded traffic*/
        if(SMEM_U32_GET_FIELD(regVal,16,1))/*Only flooded traffic out of the traffic flows that are enabled for metering will be metered*/
        {
            __LOG(("Meter only flooded traffic , set descrPtr->policerEn = 0 \n"));
            descrPtr->policerEn = 0;
        }

        /*Bill only flooded traffic*/
        if(SMEM_U32_GET_FIELD(regVal,18,1))/*Only flooded traffic out of the traffic flows that are enabled for billing will be counted */
        {
            __LOG(("Bill only flooded traffic , set descrPtr->policerCounterEn = 0 \n"));
            descrPtr->policerCounterEn = 0;
        }
    }

    if(controlDsaSubjectToL2iFilters && origDescrPtr)
    {
        if(IS_DROP_COMMAND_MAC(descrPtr->packetCmd))
        {
            __LOG(("ERROR : (although should not) The control packet got dropped ('HARD DROP') by the bridge \n"));
            origDescrPtr->packetCmd = descrPtr->packetCmd;
            origDescrPtr->cpuCode   = descrPtr->cpuCode;
        }
        else
        {
            __LOG(("NOTE : The control packet was not dropped by the bridge \n"));
        }

        /* restore original descriptor */
        descrPtr = origDescrPtr;

        __LOG(("End 'bridge' \n"));
        return;
    }

    if (descrPtr->packetCmd > SKERNEL_EXT_PKT_CMD_SOFT_DROP_E)
    {
        __LOG(("no SA learning due to descrPtr->packetCmd[%d] > SKERNEL_EXT_PKT_CMD_SOFT_DROP_E \n"
                  "End 'Bridge' \n",
                  descrPtr->packetCmd));
        return;
    }

    if (descrPtr->bypassIngressPipe == GT_FALSE)
    {
        /* the diff from descrPtr->bypassBridge is that no SA learning */
        /* Learning Mac source address*/
        __LOG(("Do SA Learning \n"));
        snetChtL2iSaLearning(devObjPtr, descrPtr, &descrPtr->ingressVlanInfo, &saLookupInfo,
                             &cntrlPcktInfo,  &filtersInfo,&learnInfo);
    }
    else
    {
        __LOG(("no SA learning due to descrPtr->bypassIngressPipe = 1 \n"));
    }

    __LOG(("End 'bridge' \n"));
}

/*******************************************************************************
*   snetChtL2iGetVlanInfo
*
* DESCRIPTION:
*       Get VLAN info
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUT:
*       vlanInfoPtr  - pointer to VLAN info structure
*
*
*******************************************************************************/
static GT_VOID snetChtL2iGetVlanInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iGetVlanInfo);

    GT_U32 regAddr;                 /* Register address */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 fldBit = 1;                  /* Register's field offset */
    GT_U32 * stgEntryPtr;           /* STG table entry pointer pointer */
    GT_U32 * vlanEntryPtr;          /* VLAN table entry pointer pointer */
    GT_U32 wordOffset;              /* Word index */
    GT_U32 bridgeVid;               /* Vlan Id for get info (descrPtr->eVid or "1" in Vlan unaware mode) */

    if(devObjPtr->txqRevision != 0)
    {
        snetLionL2iGetIngressVlanInfo(devObjPtr, descrPtr, vlanInfoPtr);
        return;
    }

    /* BasicMode */
    if (descrPtr->basicMode)
    {
        /* 802.1d bridge */
        bridgeVid = 1;
    }
    else
    {
        bridgeVid = descrPtr->eVid;
    }

    __LOG(("bridgeVid[%d] \n",bridgeVid));

    /* clear vlan entry structure */
    memset(vlanInfoPtr, 0 , sizeof(SNET_CHEETAH_L2I_VLAN_INFO));

    /* Get VLAN entry according to vid */
    regAddr = SMEM_CHT_VLAN_TBL_MEM(devObjPtr, bridgeVid);
    vlanEntryPtr = smemMemGet(devObjPtr, regAddr);

    /* VLAN validation */
    if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {    /* 1 - VLAN valid bit is check */
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr), 4, 1, &fldBit);
    }
    else
    {
        /* on cheetah always valid bit is checked */
        fldBit = 1;
    }

    vlanInfoPtr->valid = (fldBit == 0) ? 1 : SMEM_U32_GET_FIELD(vlanEntryPtr[0], 0, 1);

    /* get all the next fields from the vlan even if vlan not valid */
    /* fix to bug #90867 */

    /* SpanState-GroupIndex */
    fldValue = SMEM_U32_GET_FIELD(vlanEntryPtr[2], 24, 8);

    /* Get STG entry according to VLAN entry */
    regAddr = SMEM_CHT_STP_TBL_MEM(devObjPtr, fldValue);
    stgEntryPtr = smemMemGet(devObjPtr, regAddr);

    /* Get STP state  of source port */
    wordOffset = (2 * descrPtr->localDevSrcPort ) >> 5;     /* / 32 */
    fldBit = (2 * descrPtr->localDevSrcPort ) & 0x1f;       /* % 32 */

    vlanInfoPtr->spanState =
        SMEM_U32_GET_FIELD(stgEntryPtr[wordOffset], fldBit, 2);

    /* SET vlanInfoPtr according to VLAN entry and STG entry */
    vlanInfoPtr->unknownIsNonSecurityEvent =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 1, 1);
    vlanInfoPtr->unregNonIpMcastCmd =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 3, 3);
    vlanInfoPtr->unregIPv4McastCmd =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 6, 3);
    vlanInfoPtr->unregIPv6McastCmd =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 9, 3);
    vlanInfoPtr->unknownUcastCmd =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 12, 3);
    vlanInfoPtr->ipv4UcRoutEn =
        SMEM_U32_GET_FIELD(vlanEntryPtr[2], 14, 1);
    vlanInfoPtr->ipv6UcRoutEn =
        SMEM_U32_GET_FIELD(vlanEntryPtr[2], 16, 1);
    vlanInfoPtr->unregNonIp4BcastCmd =
        SMEM_U32_GET_FIELD(vlanEntryPtr[2], 21, 3);
    vlanInfoPtr->unregIp4BcastCmd =
        SMEM_U32_GET_FIELD(vlanEntryPtr[2], 18, 3);
    vlanInfoPtr->igmpTrapEnable =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 15, 1);
    vlanInfoPtr->ipv4IpmBrgEn =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 21, 1);
    vlanInfoPtr->ipv4IpmBrgMode =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 19, 1);
    vlanInfoPtr->ipv6IpmBrgEn =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 22, 1);
    vlanInfoPtr->ipv6IpmBrgMode =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 20, 1);
    vlanInfoPtr->ingressMirror =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 16, 1);
    vlanInfoPtr->icmpIpv6TrapMirror =
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 17, 1);
    vlanInfoPtr->ipInterfaceEn = /* for Cht2 - only ipv4 interface */
        SMEM_U32_GET_FIELD(vlanEntryPtr[0], 18, 1);
    vlanInfoPtr->naMsgToCpuEn = 1;
    /* Cht2 fields only , for cht all the fields are zero including autoLearnDisable*/
    if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {
        vlanInfoPtr->bcUdpTrapMirrorEn = SMEM_U32_GET_FIELD(vlanEntryPtr[3], 6, 1);
        vlanInfoPtr->autoLearnDisable = SMEM_U32_GET_FIELD(vlanEntryPtr[3], 0, 1);
        vlanInfoPtr->mruIndex = SMEM_U32_GET_FIELD(vlanEntryPtr[3], 2, 3);
        vlanInfoPtr->naMsgToCpuEn = SMEM_U32_GET_FIELD(vlanEntryPtr[3], 1, 1);
        vlanInfoPtr->ipV6InterfaceEn = SMEM_U32_GET_FIELD(vlanEntryPtr[3], 7, 1);
        vlanInfoPtr->ipV6SiteID = SMEM_U32_GET_FIELD(vlanEntryPtr[0], 23, 1);
        vlanInfoPtr->ipv4McRoutEn = SMEM_U32_GET_FIELD(vlanEntryPtr[2], 15, 1);
        vlanInfoPtr->ipv6McRoutEn = SMEM_U32_GET_FIELD(vlanEntryPtr[2], 17, 1);
    }

    /* Get membership of source port */
    wordOffset = SNET_CHT_VLAN_PORT_MEMBER_WORD(descrPtr->localDevSrcPort);
    fldBit  = SNET_CHT_VLAN_PORT_MEMBER_BIT(descrPtr->localDevSrcPort);
    vlanInfoPtr->portIsMember =
        SMEM_U32_GET_FIELD(vlanEntryPtr[wordOffset], fldBit, 1);

    vlanInfoPtr->floodVidx = 0xFFF;/* to make code generic */
    vlanInfoPtr->ucLocalEn = 1;    /* to make code generic */
    vlanInfoPtr->unknownSaCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E; /* to make code generic */

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr) ||     /* xcat A1*/
       SKERNEL_DEVICE_FAMILY_LION_PORT_GROUP_ONLY(devObjPtr)) /* lion */
    {
        vlanInfoPtr->floodVidxMode =
            SMEM_U32_GET_FIELD(vlanEntryPtr[5], 18, 1);/*178*/
        vlanInfoPtr->ucLocalEn =
            SMEM_U32_GET_FIELD(vlanEntryPtr[5], 17, 1);/*177*/
        vlanInfoPtr->floodVidx =
            SMEM_U32_GET_FIELD(vlanEntryPtr[3], 8, 12);/*104..115*/
    }

    /* VRF ID per VLAN. xCat2 does not support VRF ID tables */
    if (descrPtr->vrfId == 0 && SKERNEL_IS_CHEETAH3_DEV(devObjPtr) &&
        (!SKERNEL_DEVICE_FAMILY_XCAT2_ONLY(devObjPtr)))
    {   /* Get Bridge VRF-ID Table to vid */
        regAddr = SMEM_CHT3_VRFID_TBL_MEM(devObjPtr, bridgeVid);
        smemRegFldGet(devObjPtr, regAddr, 0, 12, &vlanInfoPtr->vrfId);

        descrPtr->vrfId = vlanInfoPtr->vrfId;
    }

}

/*******************************************************************************
*   snetChtL2iSetVlanInfo
*
* DESCRIPTION:
*       Set VLAN info to the descriptor
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       vlanInfoPtr  - pointer to VLAN info structure
*
* OUTPUT:
*
*
*******************************************************************************/
static GT_VOID snetChtL2iSetVlanInfo(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iSetVlanInfo);

    /* NEXT is done regardless to bypass bridge mode ! */

    /* Triggering  unicast VLAN routing */
    if (vlanInfoPtr->ipv4UcRoutEn == 1)
    {
        descrPtr->ipV4ucvlan = 1;
        __LOG_PARAM(descrPtr->ipV4ucvlan);
    }

    if (vlanInfoPtr->ipv6UcRoutEn == 1)
    {
        descrPtr->ipV6ucvlan = 1;
        __LOG_PARAM(descrPtr->ipV6ucvlan);
    }

    /* Triggering  multicast VLAN routing */
    if (vlanInfoPtr->ipv4McRoutEn == 1)
    {
        descrPtr->ipV4mcvlan = 1;
        __LOG_PARAM(descrPtr->ipV4mcvlan);
    }

    if (vlanInfoPtr->ipv6McRoutEn == 1)
    {
        descrPtr->ipV6mcvlan = 1;
        __LOG_PARAM(descrPtr->ipV6mcvlan);
    }


    return;
}


/*******************************************************************************
*   snetChtL2iLogFlow
*
* DESCRIPTION:
*       Log the SA,DA match
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       saFound      - was the SA lookup find sa
*       saIndex      - what SA lookup index found
*       daFound      - was the DA lookup find da
*       daIndex      - what DA lookup index found
*
* OUTPUT:
*
*
*
*******************************************************************************/
static GT_VOID snetChtL2iLogFlow
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL  saFound,
    IN GT_U32   saIndex,
    IN GT_BOOL  daFound,
    IN GT_U32   daIndex
)
{
    DECLARE_FUNC_NAME(snetChtL2iLogFlow);

    GT_BIT  enabled;/* is feature enabled */
    GT_U32  tmpMask,tmpValue,tmpKey;
    GT_U32  regVal;
    GT_U32  saLogAddresses[5];
    GT_U32  daLogAddresses[5];
    GT_U32 *addrPtr;
    GT_U8*  macAddrPtr;
    GT_U32 *resultPtr;
    GT_U32  index;
    GT_U32  saResult = 0;
    GT_U32  daResult = 0;
    GT_U32  ii;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* not supported in SIP5 */
        return;
    }

    __LOG(("check if need to update log flow counters \n"));

    ii=0;
    saLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_SA_LOW_REG(devObjPtr);
    saLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_SA_HIGH_REG(devObjPtr);
    saLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_SA_LOW_MASK_REG(devObjPtr);
    saLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_SA_HIGH_MASK_REG(devObjPtr);
    saLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_MAC_SA_RESULT_REG(devObjPtr);

    ii=0;
    daLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_DA_LOW_REG(devObjPtr);
    daLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_DA_HIGH_REG(devObjPtr);
    daLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_DA_LOW_MASK_REG(devObjPtr);
    daLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_DA_HIGH_MASK_REG(devObjPtr);
    daLogAddresses[ii++] = SMEM_CHT3_INGRESS_LOG_MAC_DA_RESULT_REG(devObjPtr);

    if(saFound == GT_FALSE &&  daFound == GT_FALSE)
    {
        __LOG(("the L2 LOG FLOW not valid for saFound == GT_FALSE &&  daFound == GT_FALSE \n"));
        return;
    }

    smemRegFldGet(devObjPtr, SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr), 8, 1,
                  &enabled);

    if(enabled == 0)
    {
        /* the feature not enabled */
        __LOG(("the L2 LOG FLOW not enabled \n"));
        return;
    }

    if(saFound == GT_TRUE)
    {
        saResult = 1;
    }

    if(daFound == GT_TRUE)
    {
        daResult = 1;
    }

    /* check ether type */
    smemRegGet(devObjPtr,SMEM_CHT3_INGRESS_LOG_ETHER_TYPE_REG(devObjPtr),&regVal);

    tmpMask  = SMEM_U32_GET_FIELD(regVal,16,16);
    tmpValue = SMEM_U32_GET_FIELD(regVal, 0,16);

    tmpKey = descrPtr->etherTypeOrSsapDsap;

    if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
    {
        /* no match */
        __LOG(("no match"));
        return;
    }

    for(ii = 0 ; ii < 2 ; ii++)
    {
        if(ii == 0)
        {
            if(saResult)
            {
                addrPtr = saLogAddresses;
                macAddrPtr = descrPtr->macSaPtr;
                resultPtr = &saResult;
                index = saIndex;
            }
            else
            {
                continue;
            }
        }
        else /* ii == 1 */
        {
            if(daResult)
            {
                addrPtr = daLogAddresses;
                macAddrPtr = descrPtr->macDaPtr;
                resultPtr = &daResult;
                index = daIndex;
            }
            else
            {
                continue;
            }
        }

        tmpKey = macAddrPtr[2] << 24 |
                 macAddrPtr[3] << 16 |
                 macAddrPtr[4] <<  8 |
                 macAddrPtr[5] <<  0 ;
        smemRegGet(devObjPtr,addrPtr[0],&regVal);/* mac low */
        tmpValue  = regVal;
        smemRegGet(devObjPtr,addrPtr[2],&regVal);/* mac low mask */
        tmpMask  = regVal;

        if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
        {
            /* no match */
            __LOG(("no match"));
            *resultPtr = 0;
        }

        if(*resultPtr)
        {
            tmpKey = macAddrPtr[0] <<  8 |
                     macAddrPtr[1] <<  0 ;
            smemRegGet(devObjPtr,addrPtr[1],&regVal);/* mac high */
            tmpValue = SMEM_U32_GET_FIELD(regVal, 0,16);
            smemRegGet(devObjPtr,addrPtr[3],&regVal);/* mac high mask */
            tmpMask  = SMEM_U32_GET_FIELD(regVal, 0,16);
            if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
            {
                /* no match */
                __LOG(("no match"));
                *resultPtr = 0;
            }
        }

        if(*resultPtr)
        {
            /* override previous values */
            SMEM_U32_SET_FIELD(regVal,0,15,index);
            SMEM_U32_SET_FIELD(regVal,31,1,1);/* valid bit*/

            smemRegSet(devObjPtr,addrPtr[4],regVal);/* result register */
        }
    }

}

/*******************************************************************************
*   lion3ParseFdbRouteEntry
*
* DESCRIPTION:
*       Parses fdb routing entry to the struct
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       fdbEntryPtr  - pointer to the fdb route entry
*       entryIndex   - entry index
*
* OUTPUT:
*       fdbRouteEntryInfoPtr - pointer fdb route entry struct
*
*******************************************************************************/
static GT_VOID lion3ParseFdbRouteEntry
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN    GT_U32                          *fdbEntryPtr,
    IN    GT_U32                           entryIndex,
    OUT   SNET_LION3_FDB_ROUTE_ENTRY_INFO *fdbRouteEntryInfoPtr
)
{
#define GET_VALUE(structFieldName, tableFieldName)\
    fdbRouteEntryInfoPtr->structFieldName = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr, entryIndex, tableFieldName)

    GET_VALUE(ipv6ScopeCheck, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK);
    GET_VALUE(ipv6DestSiteId, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DEST_SITE_ID);
    GET_VALUE(decTtlOrHopCount, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT);
    GET_VALUE(bypassTtlOptionsOrHopExtensionForIpv4Packets,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION_FOR_IPV4_PACKETS);
    GET_VALUE(ingressMirrorToAnalyzerIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX);
    GET_VALUE(qosProfileMarkingEn, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN);
    GET_VALUE(qosProfileIndex, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX);
    GET_VALUE(qosProfilePrecedence, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE);
    GET_VALUE(modifyUp, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_UP);
    GET_VALUE(modifyDscp, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP);
    GET_VALUE(counterSetIndex, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX);
    GET_VALUE(arpBcTrapMirrorEn, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN);
    GET_VALUE(dipAccessLevel, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL);
    GET_VALUE(ecmpRedirectExceptionMirror, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ECMP_REDIRECT_EXCEPTION_MIRROR);
    GET_VALUE(mtuIndex, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MTU_INDEX);
    GET_VALUE(useVidx, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX);
    GET_VALUE(trgIsTrunk, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK);
    GET_VALUE(trgTrunkId, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID);
    GET_VALUE(trgEport, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT);
    GET_VALUE(evidx, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_EVIDX);
    GET_VALUE(trgDev, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV);
    GET_VALUE(nextHopEvlan, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN);
    GET_VALUE(startOfTunnel, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL);
    GET_VALUE(tunnelType, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE);
    GET_VALUE(tunnelPtrValid, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR);
    GET_VALUE(arpPtrValid, SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_PTR);
    GET_VALUE(tsIsNat, SMEM_SIP5_10_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TS_IS_NAT);

#undef GET_VALUE
}


static GT_BOOL fdbDipLookup
(
    IN  SKERNEL_DEVICE_OBJECT                 *devObjPtr,
    IN  SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC  *entryInfoPtr,
    OUT GT_U32                               **fdbEntryPtrPtr,
    OUT GT_U32                                *entryIndexPtr
)
{
    GT_U32  dummyEntryOffsetArr[2];

    return snetChtL2iFdbLookup(
        devObjPtr,
        entryInfoPtr,
        GT_FALSE/*cpuMessageProcessing*/,
        GT_FALSE/*needEmptyIndex*/,
        fdbEntryPtrPtr,/* for ipv6 this is to get the entry of the data .
                          else it is the entry */
        entryIndexPtr,/* for ipv6 first index is 'address' second index is 'data'
                        else it is single index of the entry */
        &dummyEntryOffsetArr[0],/* entryOffsetPtr */
        NULL,/*sendFailedNa2CpuPtr*/
        NULL/*numValidPlacesPtr*/
        );
}

/*******************************************************************************
*   snetChtL2iFdbDipLookUp
*
* DESCRIPTION:
*       Performs FDB Routing lookup
*
* INPUTS:
*       devObjPtr        - pointer to device object.
*       descrPtr         - pointer to the frame's descriptor.
*
* OUTPUTS:
*       descrPtr         - pointer to the frame's descriptor.
*       dipLookupInfoPtr - (pointer to) dip lookup info
*
*******************************************************************************/
GT_VOID snetChtL2iFdbDipLookUp
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    OUT   SNET_LION3_FDB_ROUTE_ENTRY_INFO *dipLookupInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iFdbDipLookUp);

    GT_U32                               *fdbEntryPtr;   /* fdb entry pointer */
    GT_BOOL                               fdbFound;      /* fdb entry was found */
    GT_U32                                entryIndex[2] = {0}; /* fdb entry memory index */
    GT_U32                                dataIndex;
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC  entryHashInfo;
    SNET_CHEETAH_FDB_ENTRY_ENT            lookupType;

    memset(dipLookupInfoPtr, 0, sizeof(SNET_LION3_FDB_ROUTE_ENTRY_INFO));

    lookupType = descrPtr->isIp == 0 ? SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
                 descrPtr->isIPv4 ? SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E :
                 SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E;

    /* build the lookup key */
    entryHashInfo.isSip5 = SMEM_CHT_IS_SIP5_GET(devObjPtr);
    entryHashInfo.entryType        = lookupType;
    entryHashInfo.info.ucRouting.vrfId  = descrPtr->vrfId;
    entryHashInfo.info.ucRouting.dip[0] = descrPtr->dip[0];
    entryHashInfo.info.ucRouting.dip[1] = descrPtr->dip[1];
    entryHashInfo.info.ucRouting.dip[2] = descrPtr->dip[2];
    entryHashInfo.info.ucRouting.dip[3] = descrPtr->dip[3];

    /* Lookup for DIP */
    fdbFound = fdbDipLookup(devObjPtr, &entryHashInfo, &fdbEntryPtr, &entryIndex[0]);

    if(lookupType != SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E)
    {
        dataIndex = entryIndex[0];

        __LOG(("FDB DIP [%d.%d.%d.%d] vrfId [0x%3.3x] [%s found] FDB index is[0x%8.8x] \n",
                      (descrPtr->dip[0] >> 24) & 0xFF,
                      (descrPtr->dip[0] >> 16) & 0xFF,
                      (descrPtr->dip[0] >>  8) & 0xFF,
                      (descrPtr->dip[0] >>  0) & 0xFF,
                      descrPtr->vrfId,
                      fdbFound ? "" : "not",
                      entryIndex[0]));
    }
    else
    {
        __LOG(("FDB ipv6 DIP [0x %8.8x.%8.8x.%8.8x.%8.8x] vrfId [0x%3.3x] [%s found] FDB index for address is[0x%8.8x] ,index for data [0x%8.8x] \n",
                      descrPtr->dip[0] ,
                      descrPtr->dip[1] ,
                      descrPtr->dip[2] ,
                      descrPtr->dip[3] ,
                      descrPtr->vrfId,
                      fdbFound ? "" : "not",
                      entryIndex[0],
                      entryIndex[1]
                      ));

        /* data index is the second index*/
        dataIndex = entryIndex[1];

    }

    dipLookupInfoPtr->isValid = fdbFound;

    if(dipLookupInfoPtr->isValid == 0)
    {
        return;
    }

    /* do fdb route entry parsing */
    lion3ParseFdbRouteEntry(devObjPtr, descrPtr, fdbEntryPtr, dataIndex, dipLookupInfoPtr);


    /* get the packet command associated with the entry (in sip5 due to lack of
       bits in the FDB entry those 3 bits come from 'global register') */
    smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,
                        28 , 3 , &dipLookupInfoPtr->nextHop_hwRouterPacketCommand);

    __LOG_PARAM(dipLookupInfoPtr->nextHop_hwRouterPacketCommand);
}

/*******************************************************************************
*   fdbEntryHashInfoGet
*
* DESCRIPTION:
*       get the FDB entry hash info .
*
* INPUTS:
*       devObjPtr     - pointer to device object.
*       entryType     - entry type
*       fid           - FID
*       macAddr       - mac address
*       sip           - sip
*       dip           - dip
* OUTPUTS:
*       entryInfoPtr  - (pointer to) entry hash info
*
* RETURN:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS fdbEntryHashInfoGet
(
    IN SKERNEL_DEVICE_OBJECT      *devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_ENT  entryType,
    IN GT_U16                      fid,
    IN GT_U8                       macAddr[],
    IN GT_U32                      sip,
    IN GT_U32                      dip,
    OUT SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    *entryInfoPtr
)
{
    DECLARE_FUNC_NAME(fdbEntryHashInfoGet);

    GT_U32  fieldValue;

    entryInfoPtr->isSip5 = SMEM_CHT_IS_SIP5_GET(devObjPtr);

    /* MSG type bits [6:4] word0 */
    entryInfoPtr->entryType = entryType;
    entryInfoPtr->fid = fid;

    /* save the original fid before any change */
    entryInfoPtr->origFid = entryInfoPtr->fid;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,
                            4 , 1 , &entryInfoPtr->fid16BitHashEn);
        if(entryInfoPtr->fid16BitHashEn == 0)
        {
            entryInfoPtr->fid &= 0xfff;
        }
    }
    else
    {
        entryInfoPtr->fid16BitHashEn = 0;
    }

    switch(entryInfoPtr->entryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
            /* get MAC address from message */
            entryInfoPtr->info.macInfo.macAddr[5] = macAddr[5];
            entryInfoPtr->info.macInfo.macAddr[4] = macAddr[4];
            entryInfoPtr->info.macInfo.macAddr[3] = macAddr[3];
            entryInfoPtr->info.macInfo.macAddr[2] = macAddr[2];
            entryInfoPtr->info.macInfo.macAddr[1] = macAddr[1];
            entryInfoPtr->info.macInfo.macAddr[0] = macAddr[0];

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                      0, 2, &fieldValue);

                switch(fieldValue)
                {
                    case 0:
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E;
                        break;
                    case 1:
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_FID_E;
                        break;
                    case 2:
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_MAC_E;
                        break;
                    case 3:
                    default:
                        __LOG(("ERROR : unknown <crcHashUpperBitsMode> value [%d] \n",
                                      fieldValue));
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E;
                        break;
                }

            }
            else
            {
                entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E;
            }

            break;
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:/*IPv4 Multicast address entry (IGMP snooping);*/
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:/*IPv6 Multicast address entry (MLD snooping);*/
            entryInfoPtr->info.ipmcBridge.dip = dip;
            entryInfoPtr->info.ipmcBridge.sip = sip;
            break;
        default:
            return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
*   snetChtL2iSaLookUp
*
* DESCRIPTION:
*       Perform MAC SA lookup and get results
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       vlanInfoPtr  - VLAN and STG info pointer
*       learnInfoPtr  - (pointer to) FDB extra info
*
* OUTPUT:
*       saLookupInfoPtr  - pointer SA lookup info structure
*
*
*******************************************************************************/
static GT_VOID snetChtL2iSaLookUp
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    OUT SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iSaLookUp);

    GT_U32 * fdbEntryPtr;                       /* fdb entry pointer */
    GT_BOOL fdbFound;                           /* fdb entry was found */
    GT_U32 entryIndex[2];                       /* fdb entry memory index */
    GT_U32 entryOffset;                         /* MAC address offset */
    GT_U32 fldValue;                            /* register field value */
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    entryHashInfo;
    SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC specialFields;


    memset(saLookupInfoPtr, 0, sizeof(SNET_CHEETAH_L2I_SA_LOOKUP_INFO));

    fdbEntryCommonHashInfoGet(devObjPtr,descrPtr,vlanInfoPtr, SNET_CHEETAH_L2I_LOOKUP_MODE_SA_E, &entryHashInfo);

    /* Lookup for SA */
    fdbFound = snetChtL2iFdbLookup(devObjPtr, &entryHashInfo,GT_FALSE/*from PP*/,GT_TRUE,
                                   &fdbEntryPtr, &entryIndex[0], &entryOffset,
                                   &saLookupInfoPtr->sendFailedNa2Cpu,
                                   NULL);

    learnInfoPtr->FDB_fid     = entryHashInfo.fid;

    __LOG(("mac SA [%s found] FDB index is[0x%8.8x] \n",
                  fdbFound ? "" : "not",
                  entryIndex[0]));

    /* entry index - valid also when 'SA not found' ,
       but will hold the index where need to be learned */
    saLookupInfoPtr->entryIndex = entryIndex[0];

    if (fdbFound == GT_FALSE)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            smemRegFldGet(devObjPtr,SMEM_LION3_L2I_BRIDGE_ACCESS_MATRIX_DEFAULT_REG(devObjPtr),0,3,&descrPtr->saAccessLevel);
        }
        else
        {
            smemRegFldGet(devObjPtr,SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr),0,3,&descrPtr->saAccessLevel);
        }

        __LOG(("use saAccessLevel[%d] , for not found SA \n",
                      descrPtr->saAccessLevel));
        return;
    }

    /* FDB entry found */
    saLookupInfoPtr->found = 1;
    saLookupInfoPtr->valid = 1;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        memset(&specialFields,0,sizeof(SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC));

        sfdbLion3FdbSpecialMuxedFieldsGet(devObjPtr,fdbEntryPtr,entryIndex[0],
            &specialFields);

        /* Set AGE bit */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
            1);

        saLookupInfoPtr->saCmd =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_CMD);

        saLookupInfoPtr->sstId = specialFields.srcId;

        saLookupInfoPtr->isStatic =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC);

        /* SP Unknown */
        saLookupInfoPtr->stormPrevent =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN);

        /* SAQoS Profile Index */
        saLookupInfoPtr->saQosProfile =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX);

        /* Mirror To AnalyzerPort */
        saLookupInfoPtr->rxSniff =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER);

        /* SA security level */
        if(specialFields.saAccessLevel != SMAIN_NOT_VALID_CNS)
        {
            descrPtr->saAccessLevel = specialFields.saAccessLevel;
        }
        else
        {
            descrPtr->saAccessLevel = 0;
        }

        /* Trunk */
        learnInfoPtr->oldInfo.FDB_isTrunk =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK);

        if(learnInfoPtr->oldInfo.FDB_isTrunk)
        {
            /* Trunk */
            learnInfoPtr->oldInfo.FDB_port_trunk =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                entryIndex[0],
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM);
        }
        else
        {
            /* port number */
            learnInfoPtr->oldInfo.FDB_port_trunk =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                entryIndex[0],
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM);
        }

        /* devId */
        learnInfoPtr->oldInfo.FDB_devNum =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID);

    }
    else
    {
        /* Set AGE bit */
        SMEM_U32_SET_FIELD(fdbEntryPtr[0], 2, 1, 1);

        /* SA_CMD[1:0] */
        saLookupInfoPtr->saCmd =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 30, 2) |
           (SMEM_U32_GET_FIELD(fdbEntryPtr[3], 0, 1) << 2);

        /* SrcID */
        saLookupInfoPtr->sstId =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 6, 5);
        /* Static */
        saLookupInfoPtr->isStatic =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 25, 1);
        /* SP Unknown */
        saLookupInfoPtr->stormPrevent =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 2, 1);
        /* SAQoS Profile Index */
        saLookupInfoPtr->saQosProfile =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 3, 3);
        /* Mirror To AnalyzerPort */
        saLookupInfoPtr->rxSniff =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 9, 1);
        /* SA security level */
        descrPtr->saAccessLevel =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 14, 3);

        /* Trunk */
        learnInfoPtr->oldInfo.FDB_isTrunk = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 13, 1);
        /* Trunk or port number */
        learnInfoPtr->oldInfo.FDB_port_trunk =  SMEM_U32_GET_FIELD(fdbEntryPtr[2], 14, 7);
        /* devId */
        learnInfoPtr->oldInfo.FDB_devNum = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 1, 5);
    }

    if(learnInfoPtr->FDB_isTrunk    == learnInfoPtr->oldInfo.FDB_isTrunk && /* both of the same type */
       learnInfoPtr->FDB_port_trunk == learnInfoPtr->oldInfo.FDB_port_trunk) /* both the same value */
    {
        if(learnInfoPtr->oldInfo.FDB_isTrunk == 0 && learnInfoPtr->srcEPortIsGlobal == 0)
        {
            /* the device number need also to match */
            if(learnInfoPtr->FDB_devNum != learnInfoPtr->oldInfo.FDB_devNum)
            {
                __LOG(("SA 'moved' due to different devices : FDB[%d] new[%d] \n",
                              learnInfoPtr->oldInfo.FDB_devNum,
                              learnInfoPtr->FDB_devNum));

                saLookupInfoPtr->isMoved = 1;
            }
        }
    }
    else
    {
        __LOG(("SA 'moved' due to different trunk/port : FDB.isTrunk[%d] new.isTrunk[%d] \n"
                      "FDB.portTrunk[%d] new.portTrunk[%d] \n",
                      learnInfoPtr->oldInfo.FDB_isTrunk,
                      learnInfoPtr->FDB_isTrunk,
                      learnInfoPtr->oldInfo.FDB_port_trunk,
                      learnInfoPtr->FDB_port_trunk
                      ));
        saLookupInfoPtr->isMoved = 1;
    }

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        /* Source address is not moved and not a new one.
           FDB entry configured as Non Static*/
        if (saLookupInfoPtr->isMoved == 0 &&
            saLookupInfoPtr->isStatic == 0)
        {
            /* Source-ID check */
            if (descrPtr->sstId != saLookupInfoPtr->sstId)
            {
                /* Enables check of SourceID to the FDB lookup match criteria */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    smemRegFldGet(devObjPtr,
                                  SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                                  19, 1, &fldValue);
                }
                else
                {
                    smemRegFldGet(devObjPtr,
                                  SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr),
                                  12, 1, &fldValue);
                }

                if(fldValue)
                {
                    __LOG(("SA 'moved' due to different sstId : FDB.sstId[%d] new.sstId[%d] \n",
                                  saLookupInfoPtr->sstId,
                                  descrPtr->sstId
                                  ));


                    saLookupInfoPtr->isMoved = 1;
                }
            }
        }
    }

    learnInfoPtr->oldInfo.FDB_srcId = saLookupInfoPtr->sstId;

    learnInfoPtr->FDB_isMoved = saLookupInfoPtr->isMoved;
}

/*******************************************************************************
*   isSip5FdbRoutingTriggered
*
* DESCRIPTION:
*       Checks sip5 fdb routing trigger and init  descrPtr->fdbBasedUcRouting
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID isSip5FdbRoutingTriggered
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
)
{
    DECLARE_FUNC_NAME(isSip5FdbRoutingTriggered);

    GT_U32 fldValue = 0;       /* entry field value */

    /* Logic for FDB Based UC Routing indication */
    if( (descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr) && descrPtr->mac2me)
    {
        if( descrPtr->isIp )
        {
            if( descrPtr->isIPv4 )
            {
                /* IPv4 */
                if( descrPtr->ingressVlanInfo.ipv4UcRoutEn )
                {
                    fldValue =
                        SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                             SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV4_ROUTING_EN);
                }
            }
            else /* IPv6 */
            {
                if( descrPtr->ingressVlanInfo.ipv6UcRoutEn )
                {
                    fldValue =
                        SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                             SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_UC_IPV6_ROUTING_EN);
                }
            }

        }
        else if( descrPtr->isFcoe )
        {
            /* FCoE */
            if( descrPtr->fcoeInfo.fcoeFwdEn )
            {
                fldValue =
                    SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                         SMEM_LION3_L2I_EPORT_TABLE_FIELDS_FDB_FCOE_ROUTING_EN);
            }
        }

        descrPtr->fdbBasedUcRouting = fldValue;

    }

    __LOG_PARAM(descrPtr->fdbBasedUcRouting);
}


/*******************************************************************************
*   getDaLookupEntryHashInfo
*
* DESCRIPTION:
*       prepares DA lookup entry hash info
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       vlanInfoPtr  - VLAN and STG info pointer
* OUTPUT:
*       entryHashInfoPtr  - pointer SA lookup info structure
*
*
*******************************************************************************/
static GT_VOID getDaLookupEntryHashInfo
(
    IN    SKERNEL_DEVICE_OBJECT                 *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC       *descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO            *vlanInfoPtr,
    OUT   SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC  *entryHashInfoPtr
)
{
    DECLARE_FUNC_NAME(getDaLookupEntryHashInfo);

    GT_U32  sip = 0;   /* Source IP address */
    GT_U32  dip = 0;   /* Destination IP address */
    GT_U16  fid;       /* forwarding ID */
    GT_U8   ipByte;    /* ip byte value */
    GT_U32  byte;      /* ip byte number */
    GT_U32 *regPtr;    /* register's entry pointer */
    GT_U32  fldValue;  /* entry field value */
    GT_U8  *macDaPtr = descrPtr->macDaPtr; /* pointer to packet's MAC DA  */
    SNET_CHEETAH_FDB_ENTRY_ENT  entryType = SNET_CHEETAH_FDB_ENTRY_MAC_E; /* default value */
    GT_BIT  isIpv4McValidMacDaRange = (macDaPtr[0] == 0x01 &&
                                       macDaPtr[1] == 0x00 &&
                                       macDaPtr[2] == 0x5e &&
                                       macDaPtr[3] <= 0x7f) ? 1 : 0; /* is MAC DA in range 01:00:5E:00:00:00 - 01:00:5E:7F:FF:FF */
    GT_BIT  isIpv6McValidMacDaRange = (macDaPtr[0] == 0x33 &&
                                       macDaPtr[1] == 0x33) ? 1 : 0; /* is MAC DA is 33:33:xx:xx:xx:xx */


    __LOG_PARAM(isIpv4McValidMacDaRange);
    __LOG_PARAM(isIpv6McValidMacDaRange);

    /* do bridging */
    if (descrPtr->ipm &&
        descrPtr->isIPv4 && vlanInfoPtr->ipv4IpmBrgEn &&
        isIpv4McValidMacDaRange)
    {
        __LOG(("IPv4 IPM bridging"));

        entryType = SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E;
        dip = descrPtr->dip[0];
        sip = descrPtr->sip[0];

        /* if search mode is DIP only, set SIP to 0 as required */
        if (vlanInfoPtr->ipv4IpmBrgMode == SNET_CHEETAH_IPM_SEARCH_DIP_E)
        {
            sip = 0;
        }
    }
    else
    if(descrPtr->ipm &&
       descrPtr->isIPv4 == 0 && vlanInfoPtr->ipv6IpmBrgEn &&
       isIpv6McValidMacDaRange)
    {
        __LOG(("IPv6 IPM bridging"));
        entryType = SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E;

        /* map IPv6 SIP/DIP to 4 bytes */
        regPtr = smemMemGet(devObjPtr, SMEM_CHT_IPV6_MC_BRDG_BYTE_SEL_REG(devObjPtr));
        for (byte = 0; byte < 4; byte++)
        {
            /* IPv6DIPByte0-3 */
            fldValue = SMEM_U32_GET_FIELD(*regPtr, 4 * byte, 4);
            ipByte = (descrPtr->dip[(0xf - fldValue) / 4] >> (8 * (fldValue % 4))) & 0xff;
            dip |= ipByte << (8 * byte);

            /* IPv6SIPByte0-3 */
            fldValue = SMEM_U32_GET_FIELD(*regPtr, 16 + (4 * byte), 4);
            ipByte = (descrPtr->sip[(0xf - fldValue) / 4] >> (8 * (fldValue % 4))) & 0xff;
            sip |= ipByte << (8 * byte);
        }
        /* if search mode is DIP only, set SIP to 0 as required */
        if (vlanInfoPtr->ipv6IpmBrgMode == SNET_CHEETAH_IPM_SEARCH_DIP_E)
        {
            sip = 0;
        }
    }
    else
    {
        __LOG(("DA mac addr based (non IPv4/6 IPM bridging) \n"));
        entryType = SNET_CHEETAH_FDB_ENTRY_MAC_E;
    }

    fid = (GT_U16)(devObjPtr->supportFid ? vlanInfoPtr->fid : descrPtr->eVid);

    fdbEntryHashInfoGet(devObjPtr,entryType,
                fid,descrPtr->macDaPtr,
                sip, dip,
                entryHashInfoPtr);
}

/*******************************************************************************
*   fdbEntryCommonHashInfoGet
*
* DESCRIPTION:
*       get the FDB entry hash info for all FDB lookup modes.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       vlanInfoPtr  - VLAN and STG info pointer
*       lookUpMode   - SA/DA lookup mode
* OUTPUT:
*       fdbEntryHashInfoPtr  - pointer FDB lookup info structure
*
* RETURN:
*       GT_OK
*       GT_FAIL
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID fdbEntryCommonHashInfoGet
(
    IN      SKERNEL_DEVICE_OBJECT                 *devObjPtr,
    INOUT   SKERNEL_FRAME_CHEETAH_DESCR_STC       *descrPtr,
    IN      SNET_CHEETAH_L2I_VLAN_INFO            *vlanInfoPtr,
    IN      SNET_CHEETAH_L2I_LOOKUP_MODE_ENT      lookUpMode,
    OUT     SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC   *fdbEntryHashInfoPtr
)
{
    DECLARE_FUNC_NAME(fdbEntryCommonHashInfoGet);

    GT_U16 fid;                                 /* forwarding ID */

    memset(fdbEntryHashInfoPtr, 0, sizeof(SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC));
    fdbEntryHashInfoPtr->isSip5 = SMEM_CHT_IS_SIP5_GET(devObjPtr);

    switch(lookUpMode)
    {
        case SNET_CHEETAH_L2I_LOOKUP_MODE_SA_E:
            fid = (GT_U16)(devObjPtr->supportFid ? vlanInfoPtr->fid : descrPtr->eVid);
            fdbEntryHashInfoGet(devObjPtr,SNET_CHEETAH_FDB_ENTRY_MAC_E,
                        fid,descrPtr->macSaPtr,
                        0,0,
                        fdbEntryHashInfoPtr);
            break;
        case SNET_CHEETAH_L2I_LOOKUP_MODE_DA_E:
            getDaLookupEntryHashInfo(devObjPtr,descrPtr,vlanInfoPtr,fdbEntryHashInfoPtr);
            break;

        default:
            skernelFatalError("fdbEntryCommonHashInfoGet: wrong lookup mode\n");
            return;
    }

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        __LOG(("Check FDB lookup key mode for SIP5_10 device\n"));
        if (vlanInfoPtr->fdbLookupKeyMode == 1)
        {
            __LOG(("Double tag FDB Lookup mode: assign hash VID1 by descrPtr->vid1[%d]\n", descrPtr->vid1));
            fdbEntryHashInfoPtr->vid1 = descrPtr->vid1;
            fdbEntryHashInfoPtr->fdbLookupKeyMode = 1;
        }
        else
        {
            __LOG(("Single tag FDB Lookup mode: hash VID1 = 0"));
            fdbEntryHashInfoPtr->vid1 = 0;
            fdbEntryHashInfoPtr->fdbLookupKeyMode = 0;
        }
    }
}

/*******************************************************************************
*   bridgeDaAccessLevelGet
*
* DESCRIPTION:
*       Gets bridge da access level
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*       descrPtr     - pointer to the frame's descriptor.
*******************************************************************************/
static GT_VOID bridgeDaAccessLevelGet
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr
)
{
    DECLARE_FUNC_NAME(bridgeDaAccessLevelGet);

    GT_U32 regAddr = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                        SMEM_LION3_L2I_BRIDGE_ACCESS_MATRIX_DEFAULT_REG(devObjPtr) :
                        SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr);

    smemRegFldGet(devObjPtr, regAddr, 4,3, &descrPtr->daAccessLevel);

    __LOG(("Set DA resource group for cheetah2 bridge access matrix:descrPtr->daAccessLevel [%d] \n",
                  descrPtr->daAccessLevel ));
}

/*******************************************************************************
*   snetChtL2iDaLookUp
*
* DESCRIPTION:
*       Perform MAC DA lookup and get results
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       vlanInfoPtr  - VLAN and STG info pointer
* OUTPUT:
*       descrPtr         - pointer to the frame's descriptor.
*       daLookupInfoPtr  - pointer SA lookup info structure
*
*
*******************************************************************************/
static GT_VOID snetChtL2iDaLookUp
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    OUT SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iDaLookUp);

    GT_U32 * fdbEntryPtr;                       /* fdb entry pointer */
    GT_BOOL fdbFound;                           /* fdb entry was found */
    GT_U32 entryIndex[2];                          /* fdb entry memory index */
    GT_U32 entryOffset;                         /* MAC address offset */
    GT_U32 fldValue;                            /* entry field value */
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    entryHashInfo;
    GT_BIT  spUnknown = 0;/* SP (storm prevention indication) unknown */
    SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC specialFields;

    memset(daLookupInfoPtr, 0, sizeof(SNET_CHEETAH_L2I_DA_LOOKUP_INFO));

    /* check Sip5 Fdb Routing Trigger */
    isSip5FdbRoutingTriggered(devObjPtr, descrPtr);

    if(descrPtr->fdbBasedUcRouting)
    {
        __LOG(("due to expected DIP lookup by the Router the Bridge no do mac DA lookup,"\
                " so no descriptor modifications associated with the mac DA lookup\n"));
          return;
    }

    bridgeDaAccessLevelGet(devObjPtr, descrPtr);

    /* Get entry hash info to struct */
    fdbEntryCommonHashInfoGet(devObjPtr,descrPtr,vlanInfoPtr, SNET_CHEETAH_L2I_LOOKUP_MODE_DA_E, &entryHashInfo);

    /* Lookup for DA */
    fdbFound = snetChtL2iFdbLookup(devObjPtr, &entryHashInfo,GT_FALSE/*from PP*/,GT_FALSE,
                                   &fdbEntryPtr, &entryIndex[0], &entryOffset,
                                   NULL,
                                   NULL);

    daLookupInfoPtr->found = fdbFound;
    daLookupInfoPtr->entryIndex = entryIndex[0];
    descrPtr->macDaFound = fdbFound; /* for cheetah2 egress pcl tcam key */

    /* SP Unknown */
    if(daLookupInfoPtr->found)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            spUnknown =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                entryIndex[0],
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN);
        }
        else
        {
            spUnknown = SMEM_U32_GET_FIELD(fdbEntryPtr[3], 2, 1);
        }
    }

    __LOG(("mac DA [%s found] FDB index is[0x%8.8x] SP [%d]\n",
                  fdbFound ? "" : "not",
                  entryIndex[0],
                  spUnknown));

    if (daLookupInfoPtr->found == 0 || /* the entry not found */
        spUnknown)                     /* or entry used for SP only , so can't use it for DA entry */
    {
        /* VIDX */
        daLookupInfoPtr->useVidx = 1;

        /* this is FLOOD indication */
        return;
    }

    memset(&specialFields,0,sizeof(SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        sfdbLion3FdbSpecialMuxedFieldsGet(devObjPtr,fdbEntryPtr,entryIndex[0],
            &specialFields);
    }


    /*the entry was found and it is valid to be used */

    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* Enables destination address-based refresh :
               UC, MC */
            smemRegFldGet(devObjPtr,
                          SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                          (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) ? 6 : 7,
                          1, &fldValue);
        }
        else
        {
            /* Enables destination address-based refresh */
            smemRegFldGet(devObjPtr,
                          SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr),
                          11, 1, &fldValue);
        }

        if(fldValue)
        {
            /* Set AGE bit */
            __LOG(("REFRESH of aging bit enabled for the DA entry (set age bit)\n"));
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
                    entryIndex[0],
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
                    1);
            }
            else
            {
                SMEM_U32_SET_FIELD(fdbEntryPtr[0], 2, 1, 1);
            }
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            daLookupInfoPtr->sstId = specialFields.srcId;
        }
        else
        {
            /* SrcID */
            daLookupInfoPtr->sstId = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 6, 5);
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* DA_CMD */
        daLookupInfoPtr->daCmd =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_CMD);

        /* Static */
        daLookupInfoPtr->isStatic =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC);
        /* Indicates that this MAC Address is associated with the Router MAC */
        daLookupInfoPtr->daRoute =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ROUTE);
        /* DA QoSProfile Index */
        daLookupInfoPtr->daQosProfile =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX);
        /* Mirror To AnalyzerPort */
        daLookupInfoPtr->rxSniff =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER);
        /* Application Specific CPU Code */
        daLookupInfoPtr->appSpecCpuCode =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE);

        /* DevID */
        daLookupInfoPtr->devNum =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID);

        daLookupInfoPtr->multiTrg =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            entryIndex[0],
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE);

        /* Multiple */
        if ((entryHashInfo.entryType == SNET_CHEETAH_FDB_ENTRY_MAC_E) &&
            (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) &&
            (daLookupInfoPtr->multiTrg == 0))
        {
            daLookupInfoPtr->targed.ucast.isTrunk =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                entryIndex[0],
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK);
            if (daLookupInfoPtr->targed.ucast.isTrunk)
            {
                daLookupInfoPtr->targed.ucast.trunkId =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    entryIndex[0],
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM);
            }
            else
            {
                daLookupInfoPtr->targed.ucast.portNum =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    entryIndex[0],
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM);
            }
            /* VIDX */
            daLookupInfoPtr->useVidx = 0;

            /* da security level */
            if(specialFields.daAccessLevel != SMAIN_NOT_VALID_CNS)
            {
                descrPtr->daAccessLevel = specialFields.daAccessLevel;
            }
            else
            {
                descrPtr->daAccessLevel = 0;
            }
        }
        else
        {
            /* VIDX */
            daLookupInfoPtr->useVidx = 1;

            daLookupInfoPtr->targed.vidx =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                entryIndex[0],
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX);
        }

        /*<OrigVID1>*/
        if(specialFields.origVid1 != SMAIN_NOT_VALID_CNS)
        {
            daLookupInfoPtr->vid1 = specialFields.origVid1;
        }
        else
        {
            daLookupInfoPtr->vid1 = 0;
        }
    }
    else
    {
        /* DA_CMD */
        daLookupInfoPtr->daCmd =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 27, 3);
        /* Static */
        daLookupInfoPtr->isStatic =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 25, 1);
        /* Indicates that this MAC Address is associated with the Router MAC */
        daLookupInfoPtr->daRoute =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 1, 1);
        /* DA QoSProfile Index */
        daLookupInfoPtr->daQosProfile =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 6, 3);
        /* Mirror To AnalyzerPort */
        daLookupInfoPtr->rxSniff =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 9, 1);
        /* Application Specific CPU Code */
        daLookupInfoPtr->appSpecCpuCode =
            SMEM_U32_GET_FIELD(fdbEntryPtr[3], 10, 1);

        /* DevID */
        daLookupInfoPtr->devNum =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 1, 5);

        daLookupInfoPtr->multiTrg =
            SMEM_U32_GET_FIELD(fdbEntryPtr[2], 26, 1);

        /* Multiple */
        if ((entryHashInfo.entryType == SNET_CHEETAH_FDB_ENTRY_MAC_E) &&
            (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) &&
            (daLookupInfoPtr->multiTrg == 0))
        {
            daLookupInfoPtr->targed.ucast.isTrunk =
                SMEM_U32_GET_FIELD(fdbEntryPtr[2], 13, 1);
            if (daLookupInfoPtr->targed.ucast.isTrunk)
            {
                daLookupInfoPtr->targed.ucast.trunkId =
                    SMEM_U32_GET_FIELD(fdbEntryPtr[2], 14, 7);
            }
            else
            {
                daLookupInfoPtr->targed.ucast.portNum =
                    SMEM_U32_GET_FIELD(fdbEntryPtr[2], 14, 7);
            }
            /* VIDX */
            daLookupInfoPtr->useVidx = 0;

            /* da security level */
            descrPtr->daAccessLevel =
                    SMEM_U32_GET_FIELD(fdbEntryPtr[3], 11, 3);
        }
        else
        {
            /* VIDX */
            daLookupInfoPtr->useVidx = 1;

            daLookupInfoPtr->targed.vidx =
                SMEM_U32_GET_FIELD(fdbEntryPtr[2], 13, 12);
        }
    }
}

/*******************************************************************************
*   snetChtL2iCntrlAnalyze
*
* DESCRIPTION:
*       Analyze control packets to Trap or Mirror to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       cntrlPcktInfoPtr    - pointer to control packet info structure
* OUTPUT:
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*
*
*******************************************************************************/
static GT_VOID snetChtL2iCntrlAnalyze
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    INOUT SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iCntrlAnalyze);

    GT_U32 prot;                    /* protocol number */
    GT_U32 brgGlobCntrlReg0Data;    /* global configuration register data */
    GT_U32 icmpType;                /* ICMP type */
    GT_U32 igmpTrapEn;              /* IGMP trap enable */
    GT_U32 icmpTrapEn;              /* ICMP trap enable */
    GT_U32 arpBcTrapEnable;         /* arp bc trap enable */
    GT_U32 fldValue;                /* entry field value */
    GT_U32 fldBit;                  /* entry field offset */
    GT_U32 regAddr;                 /* register address */
    GT_U32 * regPtr;                /* register's entry pointer */
    GT_U32 i;                       /* array index */
    GT_U32 destUdpPort;             /* UDP destination port */
    GT_U32 entInx;
    /* Packet Command Conversion Tables */
    GT_U32 CNTRL_CMD_2_PCK_CMD[4] = {0, 1, 2, 0};
    GT_U32 IEEE_CMD_2_PCK_CMD[4] =  {0, 1, 2, 4};
    GT_BIT  customBpduTrapEn = 1;
    GT_BIT ipv6InterfaceEn = 0;
    GT_U32 ieeeReservedMcastCmdProfile;/* IEEE Reserved Multicast table set */
    GT_U32 portIndex;               /* port, or CPU port index */
    GT_U32 index;                   /* array index */
    GT_U32 fieldSize;               /* register field size */

    memset(cntrlPcktInfoPtr, 0, sizeof(*cntrlPcktInfoPtr));

    if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
    {
        /*IEEE Reserved MC table select*/
        ieeeReservedMcastCmdProfile =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IEEE_RSVD_MC_TABLE_SEL);
    }
    else if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        portIndex = IS_CHT_CPU_PORT(descrPtr->localDevSrcPort) ? 31 : descrPtr->localDevSrcPort;
        regAddr =
            SMEM_XCAT_IEEE_TBL_SELECT_REG_MAC(devObjPtr,
                                              portIndex);

        fldBit = 2 * (portIndex % 16);
        smemRegFldGet(devObjPtr, regAddr, fldBit, 2, &ieeeReservedMcastCmdProfile);
    }
    else
    {
        /* single table supported */
        ieeeReservedMcastCmdProfile = 0;
    }

    __LOG(("IEEE Reserved MC table selected [%d] \n",ieeeReservedMcastCmdProfile));

    /* Global configuration register0 data */
    smemRegGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr),&brgGlobCntrlReg0Data);

    if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
    {
        /* ARP BC Trap Enable */
        arpBcTrapEnable =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_BC_TRAP_EN);
        /* IGMP Trap Enable */
        igmpTrapEn =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IGMP_TRAP_EN);
        /* Enable ICMP Trap */
        icmpTrapEn = 0;/* field not exists any more ! */

    }
    else
    {
        /* Ingress Port<n> Bridge Configuration Register0 Data */
        regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF0_REG(devObjPtr, descrPtr->localDevSrcPort);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* ARP BC Trap Enable */
        arpBcTrapEnable = SMEM_U32_GET_FIELD(regPtr[0], 13, 1);
        /* IGMP Trap Enable  */
        igmpTrapEn = SMEM_U32_GET_FIELD(regPtr[0], 12, 1);
        /* Enable ICMP Trap */
        icmpTrapEn = SMEM_U32_GET_FIELD(regPtr[0], 14, 1);
    }

    __LOG(("arpBcTrapEnable[%d], igmpTrapEn[%d], icmpTrapEn[%d] \n",
                  arpBcTrapEnable,igmpTrapEn,icmpTrapEn));

    if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
    {
        if (SGT_MAC_ADDR_IS_MGMT(descrPtr->macDaPtr))
        {
            prot = descrPtr->macDaPtr[5];

            /* Reserved mcast */
            __LOG(("Reserved mcast (01:80:C2:00:00:[%2.2x]) \n",
                          prot));

            /* Customer BPDU Trap Enable GET.
               In XCat BPDU trapping/mirroring done explicitly
               according to the IEEE Reserved MC command table */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                customBpduTrapEn = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 10, 1);
            }
            else
            if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
            {
                customBpduTrapEn = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 21, 1);
            }
            else
            {
                customBpduTrapEn = 0;
            }

            if(customBpduTrapEn)
            {
                __LOG(("customBpduTrapEn[%d] \n",
                              customBpduTrapEn));
            }

            if ((prot == 0) && (customBpduTrapEn))
            {
                if (vlanInfoPtr->spanState != SKERNEL_STP_DISABLED_E)
                {
                    __LOG(("assign bpduTrapCmd[TRAP] due to : (prot == 0) && (customBpduTrapEn) && vlanInfoPtr->spanState[%d] != SKERNEL_STP_DISABLED_E \n",
                                  vlanInfoPtr->spanState));

                    cntrlPcktInfoPtr->bpduTrapCmd =
                        SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                }
                return;
            }
            else
            {
                /* IEEEReserved Multicast ToCPUEn */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 3, 1);
                }
                else
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 5, 1);
                }
                __LOG(("IEEEReserved Multicast ToCPUEn[%d] \n" ,
                              fldValue));
                if (fldValue)
                {
                    /* IEEE Reserved Multicast Configuration Register<n> */
                    if(descrPtr->ieeeReservedMcastCmdProfile == 1)
                    {
                        /* use the secondary set of registers */
                        __LOG(("use the secondary set of registers"));
                        regAddr = SMEM_CHT3_IEEE_RSRV_MCST_CONF_SECONDARY_TBL_MEM(prot);
                    }
                    else
                    {
                        regAddr =
                            SMEM_CHT_IEEE_RSRV_MCST_CONF_TBL_MEM(devObjPtr, prot, ieeeReservedMcastCmdProfile);
                    }

                    regPtr = smemMemGet(devObjPtr, regAddr);
                    fldBit = 2 * (prot % 16);

                    /* IEEEReserved Multicast Command */
                    fldValue = SMEM_U32_GET_FIELD(*regPtr, fldBit, 2);

                    /* Convert CMD from reg to SW CMD */
                    cntrlPcktInfoPtr->ieeePcktCmd =
                        IEEE_CMD_2_PCK_CMD[fldValue];
                    __LOG(("IEEEReserved Multicast Command[%d]",cntrlPcktInfoPtr->ieeePcktCmd));

                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_IEEE_RESERVED_MC_REGISTERED_REG(devObjPtr, prot);

                        fldBit = prot % 32;
                        smemRegFldGet(devObjPtr, regAddr, fldBit, 1, &fldValue);

                        if(fldValue)
                        {
                            __LOG((" Treat As Registered; Treat packets that are mirrored or forwarded by this entry as registered."));
                            descrPtr->egressFilterRegistered = 1;
                        }
                    }
                }
                return;
            }
        }
        else
        {
            if ((descrPtr->macDaPtr[0] == 0x01) &&
                (descrPtr->macDaPtr[1] == 0x00) &&
                (descrPtr->macDaPtr[2] == 0x0c))
            {
                /* Cisco L2 Protocol */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);
                    smemRegFldGet(devObjPtr, regAddr, 18, 3, &fldValue);
                    cntrlPcktInfoPtr->ciscoPcktCmd = fldValue;
                }
                else
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 9, 2);
                    cntrlPcktInfoPtr->ciscoPcktCmd = CNTRL_CMD_2_PCK_CMD[fldValue];
                }

                __LOG(("Detected Cisco L2 Protocol, use command[%d] \n",
                              cntrlPcktInfoPtr->ciscoPcktCmd));
                return;
            }
        }
    }
    else
    {
        if (descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E)
        {
            /* ARP BC Trap Enable - Cht/Cht2 */
            if (arpBcTrapEnable || vlanInfoPtr->ipInterfaceEn)
            {   /* if the arp bc is configured on vlan */
                if (vlanInfoPtr->ipInterfaceEn)
                {
                    /* ARPBC Cmd - configured per vlan*/
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 21, 3, &fldValue);
                        cntrlPcktInfoPtr->arpBcastCmd = fldValue;
                    }
                    else
                    {
                        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 7, 2);
                        cntrlPcktInfoPtr->arpBcastCmd = CNTRL_CMD_2_PCK_CMD[fldValue];
                    }

                    __LOG(("ARP BC Cmd - configured per vlan , with command[%d] \n",
                                  cntrlPcktInfoPtr->arpBcastCmd));
                }
                else
                {
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 24, 3, &fldValue);
                        cntrlPcktInfoPtr->arpBcastCmd = fldValue;

                    __LOG(("ARP BC Cmd - with command[%d] \n",
                                  cntrlPcktInfoPtr->arpBcastCmd));
                    }
                    else
                    {
                        cntrlPcktInfoPtr->arpBcastCmd =
                            SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                        __LOG(("ARP BC Cmd - 'hard coded' , with command[%d](TRAP) \n",
                                      cntrlPcktInfoPtr->arpBcastCmd));
                    }
                }
                return;
            }
        }
    }
    /* Rip1 */
    if (descrPtr->ipv4Ripv1)
    {
        if(vlanInfoPtr->ipInterfaceEn)
        {
            /* 12.10.8.4 Enable mirroring of IPv4 RIPv1 control messages to the CPU */
            /* IPv4RIPv1 MirrorEn , Cht/Cht2 */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 15, 3, &fldValue);
                cntrlPcktInfoPtr->ripV1Cmd = fldValue;

                __LOG(("IPv4 RIPv1 - command[%d] \n",
                              cntrlPcktInfoPtr->ripV1Cmd));
            }
            else
            {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 28, 1);

                __LOG(("IPv4RIPv1 : MirrorEn[%d] , vlanInfoPtr->ipInterfaceEn[%d] \n",
                      fldValue,
                      vlanInfoPtr->ipInterfaceEn));

                if (fldValue)
                {
                    cntrlPcktInfoPtr->ripV1Cmd = SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E;

                    __LOG(("IPv4RIPv1 MirrorEn - 'hard coded' , with command[%d](mirror to cpu) \n",
                                  cntrlPcktInfoPtr->ripV1Cmd));
                }
            }
        }
        return;
    }
    else
    {
        if (descrPtr->igmpQuery || descrPtr->igmpNonQuery)
        {
            __LOG(("igmpQuery[%d] igmpNonQuery[%d] igmpTrapEn(on port)[%d] \n",
                          descrPtr->igmpQuery , descrPtr->igmpNonQuery , igmpTrapEn));

            /* IGMP Trap Enable Per Port */
            if (igmpTrapEn)
            {
                /* If IGMP trapping enabled per port, all IGMP packets trapped to CPU */
                cntrlPcktInfoPtr->igmpCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                __LOG(("IGMP trapping - 'hard coded' , with command[%d](TRAP) \n",
                              cntrlPcktInfoPtr->igmpCmd));
                return;
            }

            if (vlanInfoPtr->igmpTrapEnable)
            {
                /* IGMP Mode */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 6, 2);
                }
                else
                {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 12, 2);
                }
                /* Trap all */
                if (fldValue == SNET_CHEETAH_IGMP_TRAP_MODE_E)
                {
                    cntrlPcktInfoPtr->igmpCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                    __LOG(("IGMP mode : TRAP ALL - 'hard coded' , with command[%d](TRAP) \n",
                                  cntrlPcktInfoPtr->igmpCmd));
                }
                else
                /* Snoop mode */
                if (fldValue == SNET_CHEETAH_IGMP_SNOOP_MODE_E)
                {
                    if (descrPtr->igmpQuery)
                    {
                        cntrlPcktInfoPtr->igmpCmd =
                            SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E;
                        __LOG(("IGMP mode : SNOOP , igmp query - 'hard coded' , with command[%d](mirror to cpu) \n",
                                      cntrlPcktInfoPtr->igmpCmd));
                    }
                    else
                    {
                        cntrlPcktInfoPtr->igmpCmd =
                            SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                        __LOG(("IGMP mode : SNOOP - 'hard coded' , with command[%d](TRAP) \n",
                                      cntrlPcktInfoPtr->igmpCmd));
                    }
                }
                else
                /* Router mode */
                if (fldValue == SNET_CHEETAH_IGMP_MIRROR_MODE_E)
                {
                    cntrlPcktInfoPtr->igmpCmd = SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E;
                    __LOG(("IGMP mode : Router - 'hard coded' , with command[%d](mirror to cpu) \n",
                                  cntrlPcktInfoPtr->igmpCmd));
                }

                return;
            }
        }

        /* Enable ICMP Trap */
        __LOG(("icmpTrapEn[%d] , vlanInfoPtr->icmpIpv6TrapMirror[%d] , descrPtr->ipv6Icmp[%d] \n",
                      icmpTrapEn ,vlanInfoPtr->icmpIpv6TrapMirror , descrPtr->ipv6Icmp));
        if ((icmpTrapEn || vlanInfoPtr->icmpIpv6TrapMirror) &&
            descrPtr->ipv6Icmp)
        {
            __LOG(("icmp for ipv6 \n"));

            for (i = 0; i < 8; i++)
            {
                regAddr = SMEM_CHT_IPV6_ICMP_MSG_TYPE_CONF_REG(devObjPtr, i);
                fldBit = 8 * (i % 4);
                smemRegFldGet(devObjPtr, regAddr, fldBit, 8, &fldValue);
                /* ICMPMsg-Type */
                __LOG(("from index[%d] read ICMPMsg-Type[0x%x] \n",
                              i,fldValue));
                icmpType = fldValue;
                if (icmpType != descrPtr->ipv6IcmpType)
                {
                    continue;
                }

                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    smemRegFldGet(devObjPtr, SMEM_CHT_IPV6_ICMP_CMD_REG(devObjPtr),
                                  3 * i, 3, &fldValue);
                    /* ICMP Command */
                    cntrlPcktInfoPtr->icmpV6Cmd = fldValue;

                    __LOG(("ICMP ipv6 : command [%d] \n",
                                  cntrlPcktInfoPtr->icmpV6Cmd));
                }
                else
                {
                    if (icmpTrapEn)
                    {
                        cntrlPcktInfoPtr->icmpV6Cmd =
                            SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;

                        __LOG(("match type : ICMP ipv6 : icmpTrapEn - 'hard coded' , with command[%d](TRAP) \n",
                                      cntrlPcktInfoPtr->icmpV6Cmd));
                    }
                    else
                    {
                        smemRegFldGet(devObjPtr, SMEM_CHT_IPV6_ICMP_CMD_REG(devObjPtr),
                                      2 * i, 2, &fldValue);

                        /* ICMP Command */
                        cntrlPcktInfoPtr->icmpV6Cmd = (fldValue != 3) ?
                                    fldValue :
                                    SKERNEL_EXT_PKT_CMD_FORWARD_E;/* value 3 is not defined ! */

                        __LOG(("match type : ICMP ipv6 : icmpTrapEn = 0 , with command[%d] \n",
                                      cntrlPcktInfoPtr->icmpV6Cmd));
                    }
                }
                break;
            }/* loop on i */

            /*return; --> need to allow to go to global IPv6 Link Local Mirror Enable  */
        }
        else
        {
            /* 12.10.7 Cheetah2 : UDP Broadcast Mirror/Trap (UDP Relay)    */
            if ((descrPtr->isIp) && (descrPtr->udpCompatible) &&
                (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E) &&
                (vlanInfoPtr->bcUdpTrapMirrorEn))
            {
                __LOG(("isIp && udpCompatible && macDaType=='BC' && bcUdpTrapMirrorEn \n"
                              "look descrPtr->l4DstPort[0x%4.4x] to find match in UDP Broadcast Destination Port<n> Configuration Table\n",
                              descrPtr->l4DstPort));

                destUdpPort = descrPtr->l4DstPort;
                for(entInx = 0 ; entInx <= 11 ; ++entInx)
                {
                    regAddr = SMEM_CHT2_UDP_BROADCAST_DEST_PORT_TBL_MEM(devObjPtr,entInx) ;
                    smemRegGet(devObjPtr,   regAddr, &fldValue);

                    if(0 == SMEM_U32_GET_FIELD(fldValue,16,1))
                    {
                        __LOG(("udp bc destination entry [%d] not valid \n",
                            entInx));
                        continue;
                    }

                    if ((fldValue & 0xffff) != destUdpPort)
                    {
                        __LOG(("udp bc destination entry [%d] not match the port[0x%4.4x] L4 Port \n",
                            entInx,
                            (fldValue & 0xffff)));
                        continue;
                    }

                    cntrlPcktInfoPtr->udpBcDestPortCmd =
                        (0 == SMEM_U32_GET_FIELD(fldValue,17,1)) ?
                          SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E:
                                 SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E ;
                    descrPtr->udpBcCpuCodeInx = SMEM_U32_GET_FIELD(fldValue,18,2);

                    __LOG(("udpBcDestPortCmd matched index[%d]: use command[%d] (hard coded : trap/mirror) and cpuCode offset[%d] \n",
                           entInx,
                           cntrlPcktInfoPtr->udpBcDestPortCmd,
                           descrPtr->udpBcCpuCodeInx));
                }
            }
        }

        ipv6InterfaceEn = (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr)) ?
                            vlanInfoPtr->ipV6InterfaceEn :
                            vlanInfoPtr->ipInterfaceEn ;

        __LOG(("ipv6InterfaceEn[%d] \n",
                      ipv6InterfaceEn));

        if (ipv6InterfaceEn && descrPtr->solicitationMcastMsg)
        {
            /* 12.10.8.2 IPv6 Neighbor Solicited NodeCmd */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 12, 3, &fldValue);
                cntrlPcktInfoPtr->solicitationMcastCmd = fldValue;
            }
            else
            {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 1, 2);
                cntrlPcktInfoPtr->solicitationMcastCmd = CNTRL_CMD_2_PCK_CMD[fldValue];
            }

            __LOG(("IPv6 Neighbor Solicited NodeCmd  : use command[%d] \n",
                          cntrlPcktInfoPtr->solicitationMcastCmd));
            return ;
        }

        if (descrPtr->ipXMcLinkLocalProt == 0)
        {
            __LOG(("descrPtr->ipXMcLinkLocalProt == 0 --> do not apply command \n"));
             return ;
        }
        /* ipv4 or ipv6 protocols */
        if (descrPtr->isIPv4)
        {
            /* global IPv4 LinkLocal Mirror Enable , Cht/Cht2 */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 12, 1);
            }
            else
            {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 23, 1);
            }
            __LOG(("global IPv4 LinkLocal Mirror Enable[%d] \n",
                          fldValue));

            if (fldValue && vlanInfoPtr->ipInterfaceEn)
            {
                prot = descrPtr->dip[0] & 0xff;
                if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
                {
                    index = prot / 8;
                    fldBit = 3 * (prot % 8);
                    fieldSize = 2;
                }
                else
                {
                    fldBit = prot % 32;
                    index = prot / 32;
                    fieldSize = 1;
                }
                regAddr = SMEM_CHT_IPV4_MCST_LINK_LOCAL_CONF_REG(devObjPtr, index);

                /* specific protocol IPv4 Multicast Link Local Mirror Enable */
                smemRegFldGet(devObjPtr, regAddr, fldBit, fieldSize, &fldValue);

                __LOG(("specific protocol IPv4 Multicast Link Local Mirror Enable[%d] for (descrPtr->dip[0] & 0xff) = [0x%2.2x]  \n",
                              fldValue,
                              (descrPtr->dip[0] & 0xff)));
            }
            else
            {
                /* IPv4 LinkLocal Mirror disabled */
                fldValue = 0;
            }
        }
        else /* ipv6*/
        {
            /* global IPv6 Link Local Mirror Enable  , Cht/Cht2 */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 5, 1);
            }
            else
            {
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 11, 1);
            }

            __LOG(("global IPv6 LinkLocal Mirror Enable[%d] \n",
                          fldValue));

            if (fldValue && ipv6InterfaceEn)
            {
                prot = descrPtr->dip[3] & 0xff;
                if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
                {
                    index = prot / 8;
                    fldBit = 3 * (prot % 8);
                    fieldSize = 2;
                }
                else
                {
                    fldBit = prot % 32;
                    index = prot / 32;
                    fieldSize = 1;
                }

                regAddr = SMEM_CHT_IPV6_MCST_LINK_LOCAL_CONF_REG(devObjPtr, index);
                /* specific protocol IPv6 Multicast Link Local Mirror Enable */
                smemRegFldGet(devObjPtr, regAddr, fldBit, fieldSize, &fldValue);

                __LOG(("specific protocol IPv6 Multicast Link Local Mirror Enable[%d] for (descrPtr->dip[3] & 0xff) = [0x%2.2x]  \n",
                              fldValue,
                              (descrPtr->dip[3] & 0xff)));
            }
            else
            {
                /* IPv6 LinkLocal Mirror disabled */
                fldValue = 0;
            }
        }
        /* Multicast Link Local Mirror Enable */
        if (fldValue)
        {
            cntrlPcktInfoPtr->ipXMcLinkLocalProtCmd =
                SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E;

            __LOG(("Multicast Link Local Mirror Enable - 'hard coded' use command[%d](mirror to cpu) \n",
                          cntrlPcktInfoPtr->ipXMcLinkLocalProtCmd));

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                if(descrPtr->isIPv4)
                {
                    prot = descrPtr->dip[0] & 0xff;
                    regAddr = SMEM_LION3_L2I_IPV4_MC_LINK_LOCAL_REGISTERED_REG(devObjPtr, prot);
                }
                else  /*ipv6*/
                {
                    prot = descrPtr->dip[3] & 0xff;
                    regAddr = SMEM_LION3_L2I_IPV6_MC_LINK_LOCAL_REGISTERED_REG(devObjPtr, prot);
                }

                fldBit = prot % 32;
                smemRegFldGet(devObjPtr, regAddr, fldBit, 1, &fldValue);

                if(fldValue)
                {
                    __LOG((" Treat As Registered; Treat packets that are mirrored or forwarded by this entry as registered."));
                    descrPtr->egressFilterRegistered = 1;
                }
            }
        }

    }
}

/*******************************************************************************
*   snetChtL2iFilters
*
* DESCRIPTION:
*       L2 filters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       learnInfoPtr  - (pointer to) FDB extra info
*       bypassBridge        - bypass bridge
*                             NOTE: At least the SST-ID assignment needed
*                                   even when bypassBridge == 1
* OUTPUT:
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iFilters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    OUT SNET_CHEETAH_L2I_FILTERS_INFO * filterInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr,
    IN SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT   bypassBridge
)
{
    DECLARE_FUNC_NAME(snetChtL2iFilters);

    GT_U32 *regPtr; /* pointer to register memory*/
    GT_U32 brgGlobCntrlReg0Data;    /* bridge global control register0 data */
    GT_U32 brgGlobCntrlReg1Data;    /* bridge global control register1 data */
    GT_U32 brgGlobCntrlReg2Data;    /* bridge global control register2 data */
    GT_BOOL isIpmMac;               /* IPM Mac Multicast flag */
    GT_U32 fldValue;                /* entry field value */
    GT_U32 fldBit;                  /* entry field offset */
    GT_U32 regAddr;                 /* register address */
    GT_U32 secureAutoLearnEn;       /* secure Automatic Enable */
    GT_U32 secureAutoLearnCmd;      /* secure Automatic command */
    GT_U32 srcTcpPort ;             /* TCP source port */
    GT_U32 dstTcpPort;              /* TCP destination port */
    GT_U32 mruEntryIndex;           /* VLAN MRU Profile index */
    GT_U32 fragment;                /* datagram contains additional fragments */
    GT_U32 ipvxHeadLen;             /* IPV4/6 header length */
    GT_U32 tcpDataOffset;           /* TCP data offset */
    GT_U32 ipvxTotalLen;            /* IPV4/6 total length */
    GT_U32  allowFdbSstIdAssign;/* allow FDB SST assign */
    GT_U32  portSstId;/* the port's SSTID */
    GT_U32  portSstIdMode;
    GT_U32  preBridgeSstIdMask;/* sstId mask that the units before the L2I (TTI,PCL) assign*/
    GT_U32  bridgeSstId;/* sstId that the bridge may assign*/
    GT_U32  fdbBasedSrcID;/*sstId that the FDB may assign*/
    GT_U32  origSstId;/* sstId of the incoming descriptor*/
    GT_U32  autoLearnDis;
    GT_U32  naMsgToCpuEn;
    GT_U32  naIsSecurityBreach;
    GT_U32  naStormPreventEn;
    GT_U32  unknownSaCmd;
    GT_U32  unicastLocalEnable;
    GT_U32  ingressVlanFilter;
    GT_U32  privateVlanEnable;
    GT_U32  pvlanTrgIsTrunk;
    GT_U32  pvlanPortOrTrunkId;
    GT_U32  pvlanDevNum;
    GT_U32  acceptableFrameType;
    GT_U32  arpMacSsMismatchDropEn;
    GT_U32  learnPriority_packet;/*Mac Spoof Protection - learning priority between 'uplink' and the 'user ports' - of the packet */
    GT_U32  userGroup_packet;/*Mac Spoof Protection - User ports are associated with an ePort group - of the packet*/
    GT_U32  learnPriority_saFdb;/*Mac Spoof Protection - learning priority between 'uplink' and the 'user ports'  - of SA in the FDB */
    GT_U32  userGroup_saFdb;/*Mac Spoof Protection - User ports are associated with an ePort group  - of SA in the FDB*/
    GT_U32  *learnPrioTablePtr;/*pointer to the entry in : eport/Trunk learn prio Table - for SA info in the FDB*/
    GT_U32  index;

    memset(filterInfoPtr, 0, sizeof(*filterInfoPtr));

    /* Global configuration register0 data */
    smemRegGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr), &brgGlobCntrlReg0Data);

    /* Global configuration register1 data */
    smemRegGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr), &brgGlobCntrlReg1Data);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        brgGlobCntrlReg2Data = 0;/* not used ! */
    }
    else
    {
        /* Global configuration register2 data */
        smemRegGet(devObjPtr, SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr),&brgGlobCntrlReg2Data);
    }

    if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
    {
        /*Port source-ID force mode*/
        portSstIdMode =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_SOURCE_ID_FORCE_MODE);
        /* port default sstId */
        portSstId =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_SRC_ID);
        /*Auto-learning disable*/
        autoLearnDis =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_AUTO_LEARN_DIS);
        /*NA to CPU enable*/
        naMsgToCpuEn =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_MSG_TO_CPU_EN);
        /* New Source Address Is Security Breach */
        naIsSecurityBreach =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NEW_SRC_ADDR_SECURITY_BREACH);
        /* New Address Storm Prevention Enable */
        naStormPreventEn =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_NA_STORM_PREV_EN);
        /* Unknown Source Address Cmd */
        unknownSaCmd =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UNKNOWN_SRC_ADDR_CMD);
        /* Unicast Local Enable */
        unicastLocalEnable =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_UC_LOCAL_CMD);
        /* Ingress VLAN filter*/
        ingressVlanFilter =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_VLAN_INGRESS_FILTERING);
        /* Private VLAN Enable */
        privateVlanEnable =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_EN);
        /* Port PVLAN Trunk */
        pvlanTrgIsTrunk =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_VLAN_IS_TRUNK);
        pvlanPortOrTrunkId =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_EPORT_TRUNK_NUM);
        /* Port PVLAN target device */
        pvlanDevNum =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_PORT_PVLAN_TRG_DEV);
        /* Acceptable frame type */
        acceptableFrameType =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ACCEPT_FRAME_TYPE);
        /*arpMacSsMismatchDropEn*/
        arpMacSsMismatchDropEn =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_ARP_MAC_SA_MIS_DROP_EN);

        if(0 == SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 23, 1))
        {
            /* global configuration <ARP SA Mismatch Enable> */
            arpMacSsMismatchDropEn = 0;
        }

    }
    else
    {
        if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
        {
            /* in xcat A1 --> force sstId*/
            /* in ch2,3 --> SA FDB/port */
            smemRegFldGet(devObjPtr, SMEM_CHT2_SRC_ID_ASSIGN_MOD_REG(devObjPtr),
                            descrPtr->localDevSrcPort % 32, 1, &portSstIdMode) ;
        }
        else
        {
            /* in ch1 (global not per port)--> SA FDB/port */
            portSstIdMode = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 4, 1);
        }

        /* Ingress Port<n> Bridge Configuration Register0 Data */
        regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF0_REG(devObjPtr, descrPtr->localDevSrcPort);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* port default sstId */
        portSstId = SMEM_U32_GET_FIELD(regPtr[0], 6, 5);
        /*Auto-learning disable*/
        autoLearnDis = SMEM_U32_GET_FIELD(regPtr[0], 15, 1);
        /*NA to CPU enable*/
        naMsgToCpuEn = SMEM_U32_GET_FIELD(regPtr[0], 0, 1);
        /* New Source Address Is Security Breach */
        naIsSecurityBreach = SMEM_U32_GET_FIELD(regPtr[0], 20, 1);
        /* New Address Storm Prevention Enable */
        naStormPreventEn = SMEM_U32_GET_FIELD(regPtr[0], 19, 1);
        /* Unknown Source Address Cmd */
        unknownSaCmd = SMEM_U32_GET_FIELD(regPtr[0], 16, 3);
        /* Unicast Local Enable */
        unicastLocalEnable = SMEM_U32_GET_FIELD(regPtr[0], 1, 1);
        /* Ingress VLAN filter*/
        ingressVlanFilter = SMEM_U32_GET_FIELD(regPtr[0], 21, 1);
        /* Private VLAN Enable */
        privateVlanEnable = SMEM_U32_GET_FIELD(regPtr[0], 23, 1);
        /* Port PVLAN Trunk */
        pvlanTrgIsTrunk = SMEM_U32_GET_FIELD(regPtr[0], 24, 1);
        /* PortPVLAN Target Port/PortPVLAN Target Trunk */
        pvlanPortOrTrunkId = SMEM_U32_GET_FIELD(regPtr[0], 25, 7);

        /* Ingress Port<n> Bridge Configuration Register1 Data */
        regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF1_REG(devObjPtr, descrPtr->localDevSrcPort);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* Port PVLAN target device */
        pvlanDevNum = SMEM_U32_GET_FIELD(regPtr[0], 0, 5);
        /* Acceptable frame type */
        acceptableFrameType = SMEM_U32_GET_FIELD(regPtr[0], 21, 2);

        /*arpMacSsMismatchDropEn*/
        arpMacSsMismatchDropEn = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 16, 1);
    }

    __LOG(("portSstIdMode[%d],portSstId[%d],autoLearnDis[%d],naMsgToCpuEn[%d],\n"
        "naIsSecurityBreach[%d],naStormPreventEn[%d],unknownSaCmd[%d],unicastLocalEnable[%d],\n"
        "ingressVlanFilter[%d],privateVlanEnable[%d],pvlanTrgIsTrunk[%d],pvlanPortOrTrunkId[%d],\n"
        "pvlanDevNum[%d],acceptableFrameType[%d],arpMacSsMismatchDropEn[%d]\n"
        ,portSstIdMode,portSstId,autoLearnDis,naMsgToCpuEn
        ,naIsSecurityBreach,naStormPreventEn,unknownSaCmd,unicastLocalEnable
        ,ingressVlanFilter,privateVlanEnable,pvlanTrgIsTrunk,pvlanPortOrTrunkId
        ,pvlanDevNum,acceptableFrameType,arpMacSsMismatchDropEn
        ));

    if(devObjPtr->supportVpls && devObjPtr->vplsModeEnable.bridge &&
       descrPtr->vplsInfo.unknownSaCmdAssigned)
    {
        /* the device did assignment from the PCL or the TTI of descrPtr->vplsInfo.unknownSaCmd */
        unknownSaCmd = descrPtr->vplsInfo.unknownSaCmd;
        __LOG(("VPLS mode : for descrPtr->vplsInfo.unknownSaCmdAssigned , use unknownSaCmd[%d] from descriptor \n",
            descrPtr->vplsInfo.unknownSaCmd));
    }


    /* Auto Learning is PERFORMED only if it is enabled on BOTH in-port and VLAN */
    filterInfoPtr->autoLearnEn = autoLearnDis ? 0 :/* auto lean disabled by the port */
                                 vlanInfoPtr->autoLearnDisable ? 0 : /* auto lean disabled by the vlan */
                                 1;

    __LOG(("filterInfoPtr->autoLearnEn[%d] , autoLearnDis[%d] , vlanInfoPtr->autoLearnDisable[%d]\n",
        filterInfoPtr->autoLearnEn,autoLearnDis,vlanInfoPtr->autoLearnDisable));

    if(descrPtr->capwap.validAction == GT_TRUE)
    {
        /* set the auto learn according to the CAPWAP action (CAPWAP interface) */
        filterInfoPtr->autoLearnEn = descrPtr->capwap.action.enableBridgeAutolearn;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        secureAutoLearnEn = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 9, 1);
        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 24, 3, &fldValue);
        secureAutoLearnCmd = fldValue;
    }
    else
    {
        secureAutoLearnEn  = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 18, 1);
        secureAutoLearnCmd = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 19, 2);
    }

    if ((!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr)) && filterInfoPtr->autoLearnEn)
    {   /* if the packet is not found or it is found but moved on dev/port */
        if (!saLookupInfoPtr->found || (saLookupInfoPtr->found && saLookupInfoPtr->isMoved))
        {   /* 12.5.7.3 Secure Automatic Source MAC Learning.
                read the secure automatic learning enable bit */
            __LOG(("Secure Automatic Source MAC Learning: secureAutoLearnEn[%d] \n"
                "saLookupInfoPtr->found[%d] , saLookupInfoPtr->isMoved[%d] \n",
                secureAutoLearnEn,
                saLookupInfoPtr->found,saLookupInfoPtr->isMoved));

            if (secureAutoLearnEn)
            {
                if (!saLookupInfoPtr->isMoved)
                {
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        filterInfoPtr->secAutoLearnCmd = secureAutoLearnCmd;
                    }
                    else
                    {
                        switch (secureAutoLearnCmd)
                    {
                        case 0:
                            filterInfoPtr->secAutoLearnCmd =
                                SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
                            break;
                        case 1:
                            filterInfoPtr->secAutoLearnCmd =
                                SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
                            break;
                        case 2:
                            filterInfoPtr->secAutoLearnCmd =
                                SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
                            break;
                        default:
                                skernelFatalError("Unsupported security auto-learning command\n");
                        }
                    }

                    __LOG(("Secure Automatic Source MAC Learning : assign command for 'Non moved SA' :secAutoLearnCmd[%d] \n",
                        filterInfoPtr->secAutoLearnCmd));
                }
            }
        }
    }

    if (SKERNEL_IS_CHEETAH3_DEV(devObjPtr) && filterInfoPtr->autoLearnEn)
    {
        if (saLookupInfoPtr->found && saLookupInfoPtr->isMoved && secureAutoLearnEn)
        {
            /* Enable/disable the Auto-Learn with No-Relearn feature */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                filterInfoPtr->autoLearnNoRelearnCmd = secureAutoLearnCmd;
                __LOG(("Auto-Learn with No-Relearn : assign command [%d] \n",
                    filterInfoPtr->autoLearnNoRelearnCmd));
            }
            else
            {
                filterInfoPtr->autoLearnNoRelearnCmd =
                    SKERNEL_EXT_PKT_CMD_HARD_DROP_E;

                __LOG(("Auto-Learn with No-Relearn : assign command [%d](hard drop) \n",
                    filterInfoPtr->autoLearnNoRelearnCmd));
            }
        }
    }

    /* 12.5.7.2 NA Message to CPU is enable on both VLAN and port.*/
    filterInfoPtr->naMsgToCpuEn =
        naMsgToCpuEn & vlanInfoPtr->naMsgToCpuEn;

    __LOG(("filterInfoPtr->naMsgToCpuEn[%d] , naMsgToCpuEn[%d] , vlanInfoPtr->naMsgToCpuEn[%d]\n",
        filterInfoPtr->naMsgToCpuEn,naMsgToCpuEn,vlanInfoPtr->naMsgToCpuEn));

    if(descrPtr->capwap.validAction == GT_TRUE)
    {
        /* set the enable msg to CPU according to the CAPWAP action (CAPWAP interface) */
        __LOG(("set the enable msg to CPU according to the CAPWAP action (CAPWAP interface)"));
        filterInfoPtr->naMsgToCpuEn = descrPtr->capwap.action.enableNaToCpu;
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.bridge)
    {
        if(descrPtr->portIsRingCorePort)
        {
            /* EnLearnOnRingPort */
            filterInfoPtr->autoLearnEn  =
                SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 16, 1);

            filterInfoPtr->naMsgToCpuEn = filterInfoPtr->autoLearnEn;

            __LOG(("<EnLearnOnRingPort> :from global configuration : filterInfoPtr->autoLearnEn[%d] , filterInfoPtr->naMsgToCpuEn[%d]\n",
                filterInfoPtr->autoLearnEn,filterInfoPtr->naMsgToCpuEn));

        }
    }

    /* Control learning / Locked port */
    if ( (saLookupInfoPtr->found == 0) ||
         ( (saLookupInfoPtr->isMoved == 1)  &&
            (saLookupInfoPtr->isStatic == 0) ) )
    {
        if (!filterInfoPtr->autoLearnEn)/* learning is controlled - no automatic*/
        {
            /* New Source Address Is Security Breach */
            filterInfoPtr->naIsSecurityBreach = naIsSecurityBreach;
            /* New Address Storm Prevention Enable */
            filterInfoPtr->naStormPreventEn = naStormPreventEn;
            /* Unknown Source Address Cmd */
            filterInfoPtr->unknownSaCmd = snetChtPktCmdResolution(unknownSaCmd,
                                                                  vlanInfoPtr->unknownSaCmd);

            __LOG(("Control learning / Locked port :use naIsSecurityBreach[%d] , naStormPreventEn[%d] unknownSaCmd[%d]  \n",
                naIsSecurityBreach,naStormPreventEn,unknownSaCmd));
        }
    }

    if((portSstIdMode == 0) &&
       (bypassBridge != SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E))
    {
        allowFdbSstIdAssign = 1;
    }
    else
    {
        allowFdbSstIdAssign = 0;
    }

    __LOG(("L2i - SST assignment : allowFdbSstIdAssign[%d] , descrPtr->sstIdPrecedence[%d] \n",
        allowFdbSstIdAssign,
        descrPtr->sstIdPrecedence));

    /************************/
    /* L2i - SST assignment */
    /************************/
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        preBridgeSstIdMask = descrPtr->pclAssignedSstId;
    }
    else
    {
        if(descrPtr->sstIdPrecedence == SKERNEL_PRECEDENCE_ORDER_SOFT)
        {
            preBridgeSstIdMask = 0;/* the units before the L2I NOT force the value */
        }
        else
        {
            preBridgeSstIdMask = 0xFFFFFFFF;/* the units before the L2I force the value */
        }
    }

    if(preBridgeSstIdMask == 0 || SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        fdbBasedSrcID = SMAIN_NOT_VALID_CNS;
        if(portSstIdMode == 1 && SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
        {
            __LOG(("L2i - SST assignment : not allow the bridge to use the values from th FDB  \n"));
            /* force sstId of the port */
            /* not allow the bridge to use the values from th FDB */
            allowFdbSstIdAssign = 0;
        }

        if(allowFdbSstIdAssign)
        {
            if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
            {
                fdbBasedSrcID = snetXCatFdbSrcIdAssign(devObjPtr, descrPtr,
                                       saLookupInfoPtr,
                                       daLookupInfoPtr);
            }
            else
            {
                /* check that SA was not moved. The HW do not assigns source ID for moved SA entries.
                   The functional spec does not have such notes. This checked on HW. */
                if(saLookupInfoPtr->found  && (saLookupInfoPtr->isMoved == 0))
                {
                    fdbBasedSrcID = saLookupInfoPtr->sstId;
                }
            }
        }

        if(fdbBasedSrcID != SMAIN_NOT_VALID_CNS)
        {
            __LOG(("L2i - Use FDB based sstId [0x%x] \n",
                fdbBasedSrcID));
            bridgeSstId = fdbBasedSrcID;
        }
        else
        {
            __LOG(("L2i - Use port default sstId [0x%x] \n",
                portSstId));
            bridgeSstId = portSstId;
        }

        origSstId = descrPtr->sstId;
        /* save the bits that set by previous units */
        descrPtr->sstId &= preBridgeSstIdMask;
        /* set the bits that can be set by the bridge*/
        descrPtr->sstId |= (bridgeSstId & (~preBridgeSstIdMask));

        if(origSstId != descrPtr->sstId)
        {
            __LOG(("L2i - SST assignment : sstId changed from [0x%x] to [0x%x] \n",
                origSstId,descrPtr->sstId));
        }
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.bridge &&
       saLookupInfoPtr->isMoved && saLookupInfoPtr->found)
    {
        if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
        {
            /* Moved MAC SA command */
            filterInfoPtr->movedMacCmd =
                SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                           SMEM_LION3_L2I_EPORT_TABLE_FIELDS_MOVED_MAC_SA_CMD);
            __LOG(("Moved MAC Command : assign command[%d] \n",
                            filterInfoPtr->movedMacCmd));
        }

        /* Mac Spoof Protection */
        __LOG(("Mac Spoof Protection \n"));

        fldBit = learnInfoPtr->FDB_port_trunk & 0x7;
        index = learnInfoPtr->FDB_port_trunk >> 3;
        /* get value relate to the ingress packet */
        if(descrPtr->origIsTrunk)
        {
            /* Bridge - Ingress trunk learn prio Table */
            regAddr = SMEM_LION3_BRIDGE_INGRESS_TRUNK_TBL_MEM(devObjPtr,index);
            learnPrioTablePtr = smemMemGet(devObjPtr, regAddr);
            learnPriority_packet    = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 0, 1);
            userGroup_packet        = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 1, 4);
        }
        else
        {
            /* Bridge - Ingress EPort learn prio Table */
            regAddr = SMEM_LION3_BRIDGE_INGRESS_EPORT_LEARN_PRIO_TBL_MEM(devObjPtr,index);
            learnPrioTablePtr = smemMemGet(devObjPtr, regAddr);
            learnPriority_packet    = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 0, 1);
            userGroup_packet        = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 1, 4);
        }

        fldBit = learnInfoPtr->oldInfo.FDB_port_trunk & 0x7;
        index = learnInfoPtr->oldInfo.FDB_port_trunk >> 3;
        /* get values relate to the entry that exists in the FDB */
        if(learnInfoPtr->oldInfo.FDB_isTrunk)
        {
            /* Bridge - Ingress trunk learn prio Table */
            regAddr = SMEM_LION3_BRIDGE_INGRESS_TRUNK_TBL_MEM(devObjPtr,index);
            learnPrioTablePtr = smemMemGet(devObjPtr, regAddr);
            learnPriority_saFdb    = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 0, 1);
            userGroup_saFdb        = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 1, 4);
        }
        else
        {
            /* Bridge - Ingress ePort learn prio Table */
            regAddr = SMEM_LION3_BRIDGE_INGRESS_EPORT_LEARN_PRIO_TBL_MEM(devObjPtr,index);
            learnPrioTablePtr = smemMemGet(devObjPtr, regAddr);
            learnPriority_saFdb    = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 0, 1);
            userGroup_saFdb        = snetFieldValueGet(learnPrioTablePtr,(fldBit * 5) + 1, 4);
        }


        if(learnPriority_saFdb == 0 && learnPriority_packet == 1)
        {
            /*Treat packet like normal new/moved SA address*/
            __LOG(("Treat packet like normal new/moved SA address\n"));
        }
        else
        if(learnPriority_saFdb == 1 && learnPriority_packet == 0)
        {
            filterInfoPtr->isMacSpoofProtectionEvent = 1;
            __LOG(("MAC Spoof Protection event: From High to Low\n"));
        }
        else /* the same priority ...check if in the same group */
        {
            if(userGroup_saFdb == userGroup_packet)
            {
                /*Treat packet like normal/moved address*/
                __LOG(("Treat packet like normal/moved address\n"));
            }
            else
            {
                filterInfoPtr->isMacSpoofProtectionEvent = 1;
                __LOG(("MAC Spoof Protection event: Different User Group\n"));
            }
        }

        if( 1 == filterInfoPtr->isMacSpoofProtectionEvent )
        {
            /* Mac Spoof Protection Event Is Security Breach */
            fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 15, 1);

            if(fldValue)
            {
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 18, 3, &fldValue);
                filterInfoPtr->macSpoofProtectionCmd = fldValue;

                __LOG(("MAC Spoof Protection Command : assign command[%d] \n",
                          filterInfoPtr->macSpoofProtectionCmd));
            }
        }
    }

    if (bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E)
    {
        __LOG(("Done due to bypassBridge = SNET_CHEETA_L2I_BYPASS_BRIDGE_ALLOW_ONLY_SA_LEARNING_E \n"));
        return;
    }

    /* PVE assignment should be done before all bridge filters */

    /* Global Enable for Private Edge VLAN */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 14, 1);
    }
    else
    {
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 30, 1);
    }

    __LOG(("Global Enable for Private Edge VLAN [%d] \n",
        fldValue));

    if (fldValue)
    {
        /* Private VLAN Enable */
        __LOG(("(per port) Private VLAN Enable [%d] \n",
            privateVlanEnable));

        fldValue = privateVlanEnable;
        if (fldValue)
        {
            filterInfoPtr->pvlanEn = 1;

            /* Port PVLAN Trunk */
            filterInfoPtr->pvlanTrgIsTrunk = pvlanTrgIsTrunk;

            /* PortPVLAN Target Port/PortPVLAN Target Trunk */
            filterInfoPtr->pvlanPortOrTrunkId = pvlanPortOrTrunkId;

            /* PortPVLAN Target device */
            filterInfoPtr->pvlanDevNum = pvlanDevNum;

            __LOG(("get PVE info:pvlanTrgIsTrunk[%d],pvlanPortOrTrunkId[%d],pvlanDevNum[%d] \n",
                pvlanTrgIsTrunk,pvlanPortOrTrunkId,pvlanDevNum));

        }
    }

    /* Unicast Local Enable */
    fldValue = unicastLocalEnable;
    if(descrPtr->capwap.validAction == GT_TRUE)
    {
        /* set the unicast local switching according to the CAPWAP action (CAPWAP interface) */
        __LOG(("set the unicast local switching according to the CAPWAP action (CAPWAP interface)"));
        fldValue = descrPtr->capwap.action.enableLocalSwitching;
    }
    /* Fix to bug #117286
           Local switching of Known Unicast Packets is permitted by the bridge engine
           only if this feature is enabled in BOTH the ingress port AND the VLAN table entry
           for the VLAN-ID assigned to the packet.
           Routed packets are not subject to local switching configuration constraints
       Fix bug #131662
            PVE forwarding decision is not subject to UC local switching filtering.
            PVE sets the Bridge engine forwarding destination according to the
            ingress port configuration, regardless of the outcome
            of the FDB destination lookup
    */
    if ((fldValue == 0 || (descrPtr->ingressVlanInfo.ucLocalEn == 0)) &&
        daLookupInfoPtr->found &&
        daLookupInfoPtr->useVidx == 0 &&
        daLookupInfoPtr->daRoute == 0 &&
        filterInfoPtr->pvlanEn == 0)
    {
        __LOG(("CHECK for Local switching of Known Unicast Packets.\n"));

        snetCheetahL2UcLocalSwitchFilter(devObjPtr, descrPtr,
                                         daLookupInfoPtr, filterInfoPtr,learnInfoPtr);
    }

    /* Invalid VLAN filter */
    if (vlanInfoPtr->valid == 0)
    {
        /* VLAN Not Valid Drop Mode */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 3, 3, &fldValue);
            filterInfoPtr->invldVlanCmd = fldValue;

            __LOG(("Invalid VLAN Command : assign command[%d] \n",
                          filterInfoPtr->invldVlanCmd));
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 30, 1);
            filterInfoPtr->invldVlanCmd = (fldValue) ?
                SKERNEL_EXT_PKT_CMD_HARD_DROP_E : SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

            __LOG(("Invalid VLAN filter : assign command[%d](soft/hard DROP) \n",
                          filterInfoPtr->invldVlanCmd));
        }
    }

    /* Ingress VLAN filter*/
    fldValue = ingressVlanFilter;

    /* Secure VLAN Enable */
    if (fldValue && vlanInfoPtr->portIsMember == 0)
    {
        /* VLAN Ingress Drop Mode */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 6, 3, &fldValue);
            filterInfoPtr->inVlanFilterCmd = fldValue;

            __LOG(("Ingress VLAN filter (port not member) : assign command[%d] \n",
                          filterInfoPtr->inVlanFilterCmd));
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 26, 1);
            filterInfoPtr->inVlanFilterCmd = (fldValue) ?
                SKERNEL_EXT_PKT_CMD_HARD_DROP_E : SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

            __LOG(("Ingress VLAN filter (port not member) : assign command[%d](soft/hard DROP) \n",
                          filterInfoPtr->inVlanFilterCmd));
        }
    }

    regAddr = SMEM_CHT_INGR_VLAN_RANGE_CONF_REG(devObjPtr);
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Ingress VLAN Range0 */
        smemRegFldGet(devObjPtr, regAddr, 0, 13, &fldValue);
    }
    else
    {
        /* Ingress VLAN Range0 or 1 */
        fldBit = 12 * descrPtr->portVlanSel;
        smemRegFldGet(devObjPtr, regAddr, fldBit, 12, &fldValue);
    }

    if (fldValue < descrPtr->eVid)
    {
        /* VLAN Range Drop Mode */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 9, 3, &fldValue);
            filterInfoPtr->vlanRangeCmd = fldValue;

            __LOG(("VLAN Range filter : assign command[%d] \n",
                          filterInfoPtr->vlanRangeCmd));
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 27, 1);
            __LOG(("VLAN Range Drop Mode"));
            filterInfoPtr->vlanRangeCmd = (fldValue) ?
                SKERNEL_EXT_PKT_CMD_HARD_DROP_E : SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

            __LOG(("VLAN Range filter : assign command[%d](soft/hard DROP) \n",
                          filterInfoPtr->vlanRangeCmd));
        }
    }

    /* cht2 - extended filters */
    if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {
        /* 12.3.1.3 Acceptable frame type */
        fldValue = acceptableFrameType;
        if (((fldValue == 1) && (descrPtr->origSrcTagged == 0)) ||
            ((fldValue == 2) && (descrPtr->origSrcTagged == 1)))
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 27, 3, &fldValue);
                filterInfoPtr->frameTypeCmd = fldValue;

                __LOG(("Acceptable frame type : assign command[%d] \n",
                              filterInfoPtr->frameTypeCmd));
            }
            else
            {
                filterInfoPtr->frameTypeCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ;

                __LOG(("Acceptable frame type filter: assign command[%d](soft DROP) \n",
                              filterInfoPtr->frameTypeCmd));
            }

        }

        /* 12.3.6 MRU per VLAN checking - no security breach event */
        mruEntryIndex = vlanInfoPtr->mruIndex / 2;
        smemRegGet(devObjPtr,
                    SMEM_CHT2_MRU_PROFILE_REG(devObjPtr, mruEntryIndex),
                    &fldValue);
        /* take bits 0..13 or 16..29 */
        fldValue = SMEM_U32_GET_FIELD(fldValue , (vlanInfoPtr->mruIndex & 1) * 16 , 14);

        if (fldValue < descrPtr->byteCount)
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 12, 3, &fldValue);
                filterInfoPtr->vlanMruCmd = fldValue;

                __LOG(("MRU per VLAN checking : assign command[%d] \n",
                              filterInfoPtr->vlanMruCmd));
            }
            else
            {
                filterInfoPtr->vlanMruCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;

                __LOG(("MRU per VLAN checking filter : assign command[%d](hard DROP) \n",
                              filterInfoPtr->vlanMruCmd));
            }
        }

        /* 12.7.4 : ARP MAC SA Mismatch , test is mac SA matches ARP payload */
        if (descrPtr->arp && arpMacSsMismatchDropEn)
        {
            if (!SGT_MAC_ADDR_ARE_EQUAL(descrPtr->macSaPtr,
                                        &descrPtr->l3StartOffsetPtr[8] ))
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
                    smemRegFldGet(devObjPtr, regAddr, 21, 3, &fldValue);
                    filterInfoPtr->arpMacSaMismatchCmd = fldValue;

                    __LOG(("ARP MAC SA Mismatch filter : assign command[%d] \n",
                                  filterInfoPtr->arpMacSaMismatchCmd));
                }
                else
                {
                    filterInfoPtr->arpMacSaMismatchCmd = (arpMacSsMismatchDropEn) ?
                         SKERNEL_EXT_PKT_CMD_HARD_DROP_E :
                         SKERNEL_EXT_PKT_CMD_FORWARD_E;

                    __LOG(("ARP MAC SA Mismatch filter : assign command[%d](hard DROP/frward) \n",
                                  filterInfoPtr->arpMacSaMismatchCmd));
                }
            }
        }
    }

    /* Invalid multicast SA filter */
    if (descrPtr->macSaPtr[0] & 0x01)
    {
        filterInfoPtr->invalidSa = 1;
        /* Drop Invalid multicast SA */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 0, 3, &fldValue);
            filterInfoPtr->invalidSaCmd = fldValue;

            __LOG(("Invalid SA Command : assign command[%d] \n",
                          filterInfoPtr->invalidSaCmd));
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 27, 1);

            if (fldValue)
            {
                /* Invalid multicast SA Drop Mode */
                if (SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 0, 1);
                }
                else
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 25, 1);
                }

                filterInfoPtr->invalidSaCmd = (fldValue) ?
                    SKERNEL_EXT_PKT_CMD_HARD_DROP_E :
                    SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

                __LOG(("Invalid SA Drop filter : assign command[%d](hard/soft DROP) \n",
                              filterInfoPtr->invalidSaCmd));
            }
            else
            {
                __LOG(("Invalid SA filter : NOT filtering \n"));
            }
        }

    }
    /* MAC address checks */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Check if SA is 0.
           Muticast SA check and SA == 0 check are mutual exclusive.
           Therefore invalidSaCmd is used to handle SA == 0 and Muticast SA checks */
        if (SGT_MAC_ADDR_IS_NULL(descrPtr->macSaPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_2_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 0, 3, &fldValue);
            filterInfoPtr->invalidSaCmd = fldValue;
            __LOG(("SA is 0 command : assign command[%d] \n",
                          filterInfoPtr->invalidSaCmd));
        }

        /*check if MAC SA is equal to MAC DA*/
        if (SGT_MAC_ADDR_ARE_EQUAL(descrPtr->macSaPtr, descrPtr->macDaPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_2_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr, 3, 3, &fldValue);
            filterInfoPtr->saEqualDaCmd = fldValue;

            __LOG(("SA is DA command : assign command[%d] \n",
                          filterInfoPtr->saEqualDaCmd));
        }
    }
    /* STP Ingress filter */
    if (cntrlPcktInfoPtr->bpduTrapCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       (vlanInfoPtr->spanState == SKERNEL_STP_BLOCK_LISTEN_E ||
        vlanInfoPtr->spanState == SKERNEL_STP_LEARN_E))
    {
        filterInfoPtr->stpCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
        __LOG(("STP Ingress filter : assign command[%d](soft DROP) \n",
                      filterInfoPtr->stpCmd));
    }

    /* IPM Mac Multicast filter */
    if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
    {
        __LOG(("IPM Mac Multicast filter."));

        /* check reserved mac DA addresses of IPV4 -- regardless to IPv4 recognition (0x0800) */
        filterInfoPtr->mcMacIsIpmV4 =
            SGT_MAC_ADDR_IS_IPMV4(descrPtr->macDaPtr);
        if(filterInfoPtr->mcMacIsIpmV4)
        {
            filterInfoPtr->mcMacIsIpmV6 = 0;
            isIpmMac = 1;
        }
        else /* check for IPV6 reserved mac DA addresses -- regardless to IPv6 recognition (0x86DD)*/
        {
            filterInfoPtr->mcMacIsIpmV6 = ((descrPtr->macDaPtr[0] == 0x33 &&
                                            descrPtr->macDaPtr[1] == 0x33)) ? 1 : 0;
            isIpmMac = filterInfoPtr->mcMacIsIpmV6;
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);

            if(isIpmMac == GT_FALSE)
            {
                smemRegFldGet(devObjPtr, regAddr, 9, 3, &fldValue);
                filterInfoPtr->ipmNonIpmDaCmd = fldValue;

                __LOG(("IP Multicast Not In IANA Range Command : assign command[%d] \n",
                              filterInfoPtr->ipmNonIpmDaCmd));
            }
            else
            {
                smemRegFldGet(devObjPtr, regAddr, 6, 3, &fldValue);
                filterInfoPtr->ipmIpmDaCmd = fldValue;

                __LOG(("IP Multicast In IANA Range Command : assign command[%d] \n",
                              filterInfoPtr->ipmIpmDaCmd));
            }
        }
        else
        {
            if(isIpmMac == GT_FALSE)
            {
                /* Drop Non IP Multicast Enable */
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 26, 1);
                if (fldValue)
                {
                    /* IP Multicast Not In IANA Range Drop Mode */
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 29, 1);
                    filterInfoPtr->ipmNonIpmDaCmd = (fldValue) ?
                        SKERNEL_EXT_PKT_CMD_HARD_DROP_E :
                        SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

                    __LOG(("IP Multicast Not In IANA Range Drop filter : assign command[%d](soft/hard DROP) \n",
                                  filterInfoPtr->ipmNonIpmDaCmd));
                }
            }
            else /*isIpmMac == GT_TRUE*/
            {
                /* Drop IP Multicast Enable */
                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 25, 1);
                if (fldValue)
                {
                    /* IP Multicast In IANA Range Drop Mode */
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 28, 1);
                    filterInfoPtr->ipmNonIpmDaCmd = (fldValue) ?
                        SKERNEL_EXT_PKT_CMD_HARD_DROP_E :
                        SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

                    __LOG(("IP Multicast In IANA Range Drop filter : assign command[%d](soft/hard DROP) \n",
                                  filterInfoPtr->ipmNonIpmDaCmd));
                }
            }
        }
    }

    /* Move Static Address Is Security Breach */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 8, 1);
    }
    else
    {
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 14, 1);
    }

    if (
        saLookupInfoPtr->found &&
        saLookupInfoPtr->isStatic &&
        saLookupInfoPtr->isMoved)
    {
        if(fldValue)
        {
            __LOG(("Considering the Moved Static Address as Security Breach \n"));
        }
        else
        {
            __LOG(("The Moved Static Address IS not Security Breach \n"));
        }

        if(fldValue ||
           devObjPtr->errata.l2iApplyMoveStaticCommandRegrdlessToSecurityBreach)
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /*<Move Static Command >*/
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_0_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 15, 3, &fldValue);
                filterInfoPtr->staticMovedCmd = fldValue;

                if(fldValue == 0)/*due to devObjPtr->errata.l2iApplyMoveStaticCommandRegrdlessToSecurityBreach*/
                {
                    __LOG(("Move Static Command : assign command[%d] applied even when <MoveStatic Address Is SecurityBreach> = 0 \n",
                                  filterInfoPtr->staticMovedCmd));
                }
                else
                {
                    __LOG(("Move Static Command : assign command[%d] \n",
                                  filterInfoPtr->staticMovedCmd));
                }
            }
            else
            {
                /* Move Static Drop Mode */

                fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 15, 1);
                filterInfoPtr->staticMovedCmd = (fldValue) ?
                    SKERNEL_EXT_PKT_CMD_HARD_DROP_E : SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
                __LOG(("Move Static Drop  filter : assign command[%d](soft/hard DROP) \n",
                              filterInfoPtr->staticMovedCmd));
            }
        }
    }

    /* MAC QoS Conflict Mode */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        filterInfoPtr->macQosConflictMode =
            SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 11, 1);
    }
    else
    {
        filterInfoPtr->macQosConflictMode =
            SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 22, 1);
    }

    if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {
        /* 12.7.1. cht2 - Bridge Access Matrix */
        regAddr = SMEM_CHT2_BRIDGE_MATRIX_ACCESS_LINE_REG(devObjPtr, descrPtr->saAccessLevel);
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* sip5 support 'regulare' 3 bits of command */
            smemRegFldGet(devObjPtr,regAddr,(3 * descrPtr->daAccessLevel),3,&fldValue);
            filterInfoPtr->accessCmd = fldValue;

            __LOG(("Bridge Access Matrix filter : assign command[%d] \n",
                          filterInfoPtr->accessCmd));
        }
        else
        {
            smemRegFldGet(devObjPtr,regAddr,(2 * descrPtr->daAccessLevel),2,&fldValue);
            filterInfoPtr->accessCmd = (fldValue == 1) ?
                                        SKERNEL_EXT_PKT_CMD_SOFT_DROP_E :
                                        (fldValue == 2) ?
                                        SKERNEL_EXT_PKT_CMD_HARD_DROP_E :
                                        SKERNEL_EXT_PKT_CMD_FORWARD_E;

            __LOG(("Bridge Access Matrix filter : assign command[%d](soft/hard DROP/forward) \n",
                          filterInfoPtr->accessCmd));
        }

        /* 12.7.2.2 cht2 - TCP over MAC Multicast/Broadcast */
        if (descrPtr->isIp)
        {
            if ( descrPtr->ipProt == SNET_TCP_PROT_E &&
                 descrPtr->macDaType > SKERNEL_UNICAST_MAC_E )
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                    smemRegFldGet(devObjPtr, regAddr, 15, 3, &fldValue);
                    filterInfoPtr->tcpOverMacMcCmd = fldValue;

                    __LOG(("TCP over MAC Multicast/Broadcast Command : assign command[%d] \n",
                                  filterInfoPtr->tcpOverMacMcCmd));
                }
                else
                {
                    fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 17, 1);
                    filterInfoPtr->tcpOverMacMcCmd = (fldValue == 1) ?
                        SKERNEL_EXT_PKT_CMD_HARD_DROP_E : SKERNEL_EXT_PKT_CMD_FORWARD_E;

                    __LOG(("TCP over MAC Multicast/Broadcast filter : assign command[%d](hard DROP/forward) \n",
                                  filterInfoPtr->tcpOverMacMcCmd));
                }
            }
        }



        fragment = (descrPtr->l3StartOffsetPtr[6] & (1<<5)) == 1 ?
                GT_TRUE :/* fragment */
        /* 13 bits of FragmentOffset in the ip header */
            (((descrPtr->l3StartOffsetPtr[6] & 0x1f)<< 8 |
             (descrPtr->l3StartOffsetPtr[7])) == 0) ?
                GT_FALSE :/* not fragment */
                GT_TRUE;/* fragment */

        if (fragment &&  descrPtr->ipv4Icmp)
        {   /* 12.7.2.5 ICMP FRAGMENTATION flags are zero */

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_1_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddr, 0, 3, &fldValue);
                filterInfoPtr->icmpFragCmd = fldValue;

                __LOG(("ICMP FRAGMENTATION Command : assign command[%d] \n",
                              filterInfoPtr->icmpFragCmd));
            }
            else
            {
                SNET_CHT2_TCP_SANITY_CHECK_CMD(
                    SMEM_U32_GET_FIELD(brgGlobCntrlReg2Data, 5, 1),
                    &filterInfoPtr->icmpFragCmd);

                __LOG(("ICMP FRAGMENTATION filter : assign command[%d](hard DROP/forward) \n",
                              filterInfoPtr->icmpFragCmd));
            }
        }

        if (descrPtr->isIp)
        {
            if(descrPtr->ipProt == SNET_TCP_PROT_E
               && descrPtr->l4StartOffsetPtr != NULL)
            {
                fldValue = descrPtr->l4StartOffsetPtr[13];
                if (fldValue == SNET_CHEETAH2_TCP_ZERO_FLAGS_E)
                {   /* 12.7.2.5 TCP flags are zero */
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 12, 3, &fldValue);
                        filterInfoPtr->tcpZeroFlagsCmd = fldValue;

                        __LOG(("TCP flags are zero Command : assign command[%d] \n",
                                      filterInfoPtr->tcpZeroFlagsCmd));
                    }
                    else
                    {
                        SNET_CHT2_TCP_SANITY_CHECK_CMD(
                                SMEM_U32_GET_FIELD(brgGlobCntrlReg2Data, 4, 1),
                                &filterInfoPtr->tcpZeroFlagsCmd);

                        __LOG(("TCP flags are zero filter : assign command[%d](hard DROP/forward) \n",
                                      filterInfoPtr->tcpZeroFlagsCmd));
                    }
                }
                else if (fldValue == SNET_CHEETAH2_TCP_FIN_URG_PSH_FLAGS_E)
                {   /* 12.7.2.4 TCP flags with FIN-URG-PSH */
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 9, 3, &fldValue);
                        filterInfoPtr->tcpFinUrgPsgFlagsCmd = fldValue;

                        __LOG(("TCP flags with FIN-URG-PSH Command : assign command[%d] \n",
                                      filterInfoPtr->tcpFinUrgPsgFlagsCmd));
                    }
                    else
                    {
                        SNET_CHT2_TCP_SANITY_CHECK_CMD(
                                SMEM_U32_GET_FIELD(brgGlobCntrlReg2Data, 3, 1) ,
                                &filterInfoPtr->tcpFinUrgPsgFlagsCmd);

                        __LOG(("TCP flags with FIN-URG-PSH filter : assign command[%d](hard DROP/forward) \n",
                                      filterInfoPtr->tcpFinUrgPsgFlagsCmd));
                    }

                }
                else if (fldValue == SNET_CHEETAH2_TCP_SYN_FIN_FLAGS_E)
                {   /* 12.7.2.5 TCP flags with SYN-FIN */
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 6, 3, &fldValue);
                        filterInfoPtr->tcpSynFinFlagsCmd = fldValue;

                        __LOG(("TCP flags with SYN-FIN Command : assign command[%d] \n",
                                      filterInfoPtr->tcpSynFinFlagsCmd));
                    }
                    else
                    {
                        SNET_CHT2_TCP_SANITY_CHECK_CMD(
                                SMEM_U32_GET_FIELD(brgGlobCntrlReg2Data, 2, 1) ,
                                &filterInfoPtr->tcpSynFinFlagsCmd);

                        __LOG(("TCP flags with SYN-FIN filter : assign command[%d](hard DROP/forward) \n",
                                      filterInfoPtr->tcpSynFinFlagsCmd));
                    }
                }
                else if (fldValue == SNET_CHEETAH2_TCP_SYN_RST_FLAGS_E)
                {   /* 12.7.2.6 TCP flags with SYN-RST */
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 3, 3, &fldValue);
                        filterInfoPtr->tcpSynRstFlagsCmd = fldValue;

                        __LOG(("TCP flags with SYN-RST Command : assign command[%d] \n",
                                      filterInfoPtr->tcpSynRstFlagsCmd));
                    }
                    else
                    {
                        SNET_CHT2_TCP_SANITY_CHECK_CMD(
                                SMEM_U32_GET_FIELD(brgGlobCntrlReg2Data, 1, 1) ,
                                &filterInfoPtr->tcpSynRstFlagsCmd);

                        __LOG(("TCP flags with SYN-RST filter : assign command[%d](hard DROP/forward) \n",
                                      filterInfoPtr->tcpSynRstFlagsCmd));
                    }
                }

                if ((fldValue == SNET_CHEETAH2_TCP_SYN_FLAGS_E ||
                    fldValue == SNET_CHEETAH2_TCP_SYN_ACK_FLAGS_E)
                    && descrPtr->l4StartOffsetPtr && descrPtr->l3StartOffsetPtr)
                {
                    /* TCP SYN Packet with Data */
                    __LOG(("TCP SYN Packet with Data.\n"));

                    /* For IPv4, a TCP SYN packet with data is identified if
                      the following is true:
                       (IPv4<IHL>*4 + TCP<Data Offset>*4) < (IPv4<Total Length>)
                      For IPv6, a TCP SYN packet with data is identified if
                      the following is true:
                       (TCP<Data offset>*4) < (IPv6<payload length>) */

                    ipvxHeadLen = (descrPtr->isIPv4) ?
                        (descrPtr->l3StartOffsetPtr[0] & 0xf) * 4 : 0;

                    ipvxTotalLen = (descrPtr->isIPv4) ?
                        descrPtr->l3StartOffsetPtr[2] << 8 | descrPtr->l3StartOffsetPtr[3] :
                        descrPtr->l3StartOffsetPtr[4] << 8 | descrPtr->l3StartOffsetPtr[5];

                    /* take TCP offset only for L4 Valid packets */
                    tcpDataOffset = (descrPtr->l4Valid) ?
                        ((descrPtr->l4StartOffsetPtr[12] >> 4) & 0xf) * 4  :
                        0;

                    if ((ipvxHeadLen + tcpDataOffset) < ipvxTotalLen)
                    {
                        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                        {
                            regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                            smemRegFldGet(devObjPtr, regAddr, 18, 3, &fldValue);
                            filterInfoPtr->tcpSynWithData = fldValue;

                            __LOG(("TCP SYN Packet with Data Command : assign command[%d] \n",
                                          filterInfoPtr->tcpSynWithData));
                        }
                        else
                        {
                            SNET_CHT2_TCP_SANITY_CHECK_CMD(
                                SMEM_U32_GET_FIELD(brgGlobCntrlReg0Data, 29, 1),
                                &filterInfoPtr->tcpSynWithData);

                            __LOG(("TCP SYN Packet with Data filter : assign command[%d](hard DROP/forward) \n",
                                          filterInfoPtr->tcpSynWithData));
                        }
                    }
                }
            }
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* Check if SIP address is equal to DIP address for IPv4 and IPv6 packets */
                if ((descrPtr->isIPv4 && (descrPtr->sip[0] == descrPtr->dip[0])) ||
                    (!(descrPtr->isIPv4) && (memcmp(descrPtr->sip, descrPtr->dip, sizeof(descrPtr->sip)) == 0)))
                {
                    regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_2_REG(devObjPtr);
                    smemRegFldGet(devObjPtr, regAddr, 6, 3, &fldValue);
                    filterInfoPtr->sipIsDipCmd = fldValue;

                    __LOG(("SIP is DIP command : assign command[%d] \n",
                                 filterInfoPtr->sipIsDipCmd));
                }
            }
            /* TCP and UPP common checks - TCP/UDP Port is Zero*/
            if(descrPtr->ipProt == SNET_TCP_PROT_E ||
               descrPtr->udpCompatible)
            {
                srcTcpPort = descrPtr->l4SrcPort;
                dstTcpPort = descrPtr->l4DstPort;
                if (srcTcpPort == 0 || dstTcpPort == 0)
                {   /* 12.7.2.7 TCP port is Zero */
                    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                    {
                        regAddr = SMEM_LION3_L2I_BRIDGE_COMMAND_CONFIG_3_REG(devObjPtr);
                        smemRegFldGet(devObjPtr, regAddr, 0, 3, &fldValue);
                        filterInfoPtr->tcpPortZeroFlagsCmd = fldValue;

                        __LOG(("TCP SYN Packet with Data Command : assign command[%d] \n",
                                      filterInfoPtr->tcpPortZeroFlagsCmd));
                    }
                    else
                    {
                        SNET_CHT2_TCP_SANITY_CHECK_CMD(
                            SMEM_U32_GET_FIELD(brgGlobCntrlReg2Data, 0, 1),
                            &filterInfoPtr->tcpPortZeroFlagsCmd);

                        __LOG(("TCP port is Zero filter : assign command[%d](hard DROP/forward) \n",
                                      filterInfoPtr->tcpPortZeroFlagsCmd));
                    }
                }
            }
        }
    }
}

/*******************************************************************************
*   snetChtL2iFloodVidxAssign
*
* DESCRIPTION:
*       Phase 1 L2i decision - do flood vidx assign
*                       called when need to get VIDX for flooding
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetChtL2iFloodVidxAssign
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iFloodVidxAssign);

    if(devObjPtr->supportVlanEntryVidxFloodMode)
    {
        /* the device supports the flood vidx mode */
        if(descrPtr->ingressVlanInfo.floodVidxMode ||
           (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E))
        {
            /* uses the entry's VIDX */
            descrPtr->eVidx = descrPtr->ingressVlanInfo.floodVidx;
            __LOG(("uses the entry's VIDX [0x%4.4x] \n",
                          descrPtr->eVidx));
            return;
        }
    }

    /* uses the 0xFFF */
    descrPtr->eVidx = 0xFFF;

    __LOG(("uses VIDX 0xFFF \n"));

    return;
}

/*******************************************************************************
*   sip5L2iCommandAndCpuCodeResolution
*
* DESCRIPTION:
*       SIP5 : L2i Command And Cpu Code Resolution
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID sip5L2iCommandAndCpuCodeResolution
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(sip5L2iCommandAndCpuCodeResolution);

    SKERNEL_EXT_PACKET_CMD_ENT sip5Commands[SIP5_MAX_PCKT_COMMANDS];    /* extended packet command */
    GT_U32 IEEE_CPU_INDEX_2_CODE[4] =  {SNET_CHT_IEEE_RES_MCAST_ADDR_TRAP_MIRROR,
                                        SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_1,
                                        SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_2,
                                        SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_3};
    GT_U32 IPVX_LINK_LOCAL_INDEX_2_CODE[4] = {SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR,
                                              SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR_1,
                                              SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR_2,
                                              SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR_3};
    GT_U32 ii;                                   /* array index */
    GT_U32 iiMax;                                /* max array index */
    GT_U32 fldValue;                            /* entry field value */
    GT_U32 regAddr;                             /* register address */
    GT_U32 fldOffset;                           /* entry field offset */
    GT_U32 protocol;                            /* IEEE protocol */
    GT_BOOL hardDropEventOccured;               /* flag indicating that event which caused hard drop occured */
    GT_BOOL staticMovedIsSecurityBreach;         /* mac Sa Moved Is Security Breach*/

    /* combine the 2 commands */
    cntrlPcktInfoPtr->bpduTrapCmd = snetChtPktCmdResolution(cntrlPcktInfoPtr->bpduTrapCmd,
                        filtersInfoPtr->stpCmd);
    ii = 0;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->movedMacCmd,SNET_LION3_CPU_CODE_L2I_MAC_SA_MOVED,GT_TRUE,ii);/*1*/
    ii++;
    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->macSpoofProtectionCmd,SNET_LION3_CPU_CODE_L2I_MAC_SPOOF,GT_TRUE,ii);/*2*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->igmpCmd,SNET_CHT_IPV4_IGMP_TRAP_MIRROR,GT_FALSE,ii);/*3*/
    ii++;
    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->solicitationMcastCmd,SNET_CHT_IPV6_NEIGHBOR_SOLICITATION_TRAP_MIRROR,GT_FALSE,ii);/*4*/
    ii++;
    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->icmpV6Cmd,SNET_CHT_IPV6_ICMP_TRAP_MIRROR,GT_FALSE,ii);/*5*/
    ii++;
    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->bpduTrapCmd,SNET_CHT_BPDU_TRAP,GT_FALSE,ii);/*6*/
    ii++;
    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->arpBcastCmd,SNET_CHT_ARP_BCAST_TRAP_MIRROR,GT_FALSE,ii);/*7*/
    ii++;

    {
        /* IEEE Reserved mcast protocol */
        protocol = descrPtr->macDaPtr[5];

        /* IEEE Reserved Multicast CPU Index */
        regAddr = SMEM_CHT_IEEE_RESERV_MC_CPU_INDEX_TBL_MEM(devObjPtr,
                                                            protocol);

        fldOffset = 2 * protocol % 16;
        smemRegFldGet(devObjPtr, regAddr, fldOffset, 2, &fldValue);
        SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->ieeePcktCmd,IEEE_CPU_INDEX_2_CODE[fldValue],GT_FALSE,ii);/*8..11*/
        ii++;
    }

    {
        /* IPv4/IPv6 Multicast Link-Local CPU Code Index <%t> <%n> */
        if(descrPtr->isIp)
        {
            if(descrPtr->isIPv4)
            {
                protocol = descrPtr->dip[0] & 0xff;
                regAddr = SMEM_LION3_L2I_IPV4_MC_LINK_LOCAL_CPU_CODE_INDEX_REG(devObjPtr,protocol);
            }
            else /* ipv6*/
            {
                protocol = descrPtr->dip[3] & 0xff;
                regAddr = SMEM_LION3_L2I_IPV6_MC_LINK_LOCAL_CPU_CODE_INDEX_REG(devObjPtr,protocol);
            }

            fldOffset = 2 * (protocol % 16);
            smemRegFldGet(devObjPtr, regAddr, fldOffset, 2, &fldValue);
        }
        else
        {
            fldValue = 0;
        }
        SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->ipXMcLinkLocalProtCmd,IPVX_LINK_LOCAL_INDEX_2_CODE[fldValue],GT_FALSE,ii);/*12..15*/
        ii++;
    }

    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->ripV1Cmd,SNET_CHT_IPV4_RIPV1_MIRROR,GT_FALSE,ii);/*16*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->ciscoPcktCmd,SNET_CHT_CISCO_CONTROL_MCAST_MAC_ADDR_TRAP_MIRROR,GT_FALSE,ii);/*17*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(saLookupInfoPtr->saCmd,SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR,GT_TRUE,ii);/*18*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(daLookupInfoPtr->daCmd,SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR,GT_TRUE,ii);/*18*/
    ii++;

    if (filtersInfoPtr->unknownSaCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->unknownSaCmd,SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_TRAP,GT_TRUE,ii);/*19*/
        ii++;
    }
    else
    {
        SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->unknownSaCmd,SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_MIRROR,GT_FALSE,ii);/*20*/
        ii++;
    }

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->secAutoLearnCmd,SNET_CHT_SEC_AUTO_LEARN_UNK_SRC_TRAP,GT_TRUE,ii);/*21*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(cntrlPcktInfoPtr->udpBcDestPortCmd,SNET_CHT_UDP_BC_TRAP_MIRROR0 + descrPtr->udpBcCpuCodeInx,GT_FALSE,ii);/*22..25*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->invalidSaCmd,SNET_LION3_CPU_CODE_L2I_INVALID_SA,GT_TRUE,ii);/*26*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->saEqualDaCmd,SNET_LION3_CPU_CODE_L2I_SA_IS_DA,GT_TRUE,ii);/*27*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->invldVlanCmd,SNET_LION3_CPU_CODE_L2I_VLAN_NOT_VALID,GT_TRUE,ii);/*28*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->inVlanFilterCmd,SNET_LION3_CPU_CODE_L2I_PORT_NOT_VLAN_MEM,GT_TRUE,ii);/*29*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->vlanRangeCmd,SNET_LION3_CPU_CODE_L2I_VLAN_RANGE,GT_TRUE,ii);/*30*/
    ii++;

    staticMovedIsSecurityBreach = GT_TRUE;

    if(devObjPtr->errata.l2iApplyMoveStaticCommandRegrdlessToSecurityBreach &&
        saLookupInfoPtr->found &&
        saLookupInfoPtr->isStatic &&
        saLookupInfoPtr->isMoved)
    {
        /*Move Static Address Is Security Breach*/
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr),8,1,&fldValue);
        if(fldValue == 0)
        {
            /* the command filtersInfoPtr->staticMovedCmd was set even though
              <Move Static Address Is Security Breach> == 0 , so we need to treat
              as 'NOT security breach'*/
            staticMovedIsSecurityBreach = GT_FALSE;
        }
    }

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->staticMovedCmd,SNET_LION3_CPU_CODE_L2I_STATIC_ADDR_MOVED,staticMovedIsSecurityBreach,ii);/*31*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->arpMacSaMismatchCmd,SNET_LION3_CPU_CODE_L2I_ARP_SA_MISMATCH,GT_TRUE,ii);/*32*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpSynWithData,SNET_LION3_CPU_CODE_L2I_TCP_SYN_WITH_DATA,GT_TRUE,ii);/*33*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpOverMacMcCmd,SNET_LION3_CPU_CODE_L2I_TCP_OVER_MC_BC,GT_TRUE,ii);/*34*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->accessCmd,SNET_LION3_CPU_CODE_L2I_BRIDGE_ACCESS_MATRIX,GT_TRUE,ii);/*35*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->frameTypeCmd,SNET_LION3_CPU_CODE_L2I_ACCEPT_FRAME_TYPE,GT_TRUE,ii);/*36*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->icmpFragCmd,SNET_LION3_CPU_CODE_L2I_FRAGMENT_ICMP,GT_TRUE,ii);/*37*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpZeroFlagsCmd,SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_ZERO,GT_TRUE,ii);/*38*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpFinUrgPsgFlagsCmd,SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_FIN_URG_PSH,GT_TRUE,ii);/*39*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpSynFinFlagsCmd,SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_FIN,GT_TRUE,ii);/*40*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpSynRstFlagsCmd,SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_RST,GT_TRUE,ii);/*41*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->tcpPortZeroFlagsCmd,SNET_LION3_CPU_CODE_L2I_TCP_UDP_SRC_DEST_ZERO,GT_TRUE,ii);/*42*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->sipIsDipCmd,SNET_LION3_CPU_CODE_L2I_SIP_IS_DIP,GT_TRUE,ii);/*43*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->vlanMruCmd,SNET_LION3_CPU_CODE_L2I_VLAN_MRU,GT_FALSE,ii);/*47*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->rateLimitCmd,SNET_LION3_CPU_CODE_L2I_RATE_LIMITING,GT_FALSE,ii);/*48*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->localPortCmd,SNET_LION3_CPU_CODE_L2I_LOCAL_PORT,GT_FALSE,ii);/*49*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->ipmIpmDaCmd,SNET_LION3_CPU_CODE_L2I_IPMC,GT_FALSE,ii);/*50*/
    ii++;

    SIP5_L2I_COMMAND_AND_CPU_CODE(filtersInfoPtr->ipmNonIpmDaCmd,SNET_LION3_CPU_CODE_L2I_NON_IPMC,GT_FALSE,ii);/*51*/
    ii++;

    iiMax = ii;
    /* command resolution + CPU/DROP code resolution */
    /* NOTE: DO REVERSE on the 'priority' to support the logic of 'CPU code'/'drop code' resolution */

    hardDropEventOccured = GT_FALSE;

    for (ii = iiMax; ii ; ii--)
    {
        if(IS_HARD_DROP_COMMAND_MAC(sip5Commands[ii-1]))
        {
            filtersInfoPtr->dropCounterMode = sip5CpuCodes[ii-1];
            hardDropEventOccured = GT_TRUE;
            if( GT_TRUE == sip5SecurityBreachEvent[ii-1] )
            {
                filtersInfoPtr->securityBreachMode = sip5CpuCodes[ii-1];
            }
        }
        else if (IS_SOFT_DROP_COMMAND_MAC(sip5Commands[ii-1]))
        {
            /* the bridge had reason to SOFT_DROP .
               NOTE:
                even if final command is 'trap' but the bridge did 'soft drop'
                --> it will be counter in drop counters.
                If the bridge already had reason for HARD_DROP, then the
                SOFT_DROP reason is of no interest.
            */
            if( GT_FALSE == hardDropEventOccured )
            {
                filtersInfoPtr->dropCounterMode = sip5CpuCodes[ii-1];
            }

            if( GT_TRUE == sip5SecurityBreachEvent[ii-1] )
            {
                filtersInfoPtr->securityBreachMode = sip5CpuCodes[ii-1];
            }
        }
        else if (IS_TRAP_COMMAND_MAC(sip5Commands[ii-1]))
        {
            if( (SNET_LION3_CPU_CODE_L2I_MAC_SA_MOVED == sip5CpuCodes[ii-1]) ||
                (SNET_LION3_CPU_CODE_L2I_MAC_SPOOF == sip5CpuCodes[ii-1]) )
            {
                filtersInfoPtr->securityBreachMode = sip5CpuCodes[ii-1];
            }
        }

        __LOG(("dropCounterMode : [%d], securityBreachMode : [%d]\n",
           filtersInfoPtr->dropCounterMode,
           filtersInfoPtr->securityBreachMode));

        snetChtIngressCommandAndCpuCodeResolution(devObjPtr,descrPtr,
                descrPtr->packetCmd,/* previous command */
                sip5Commands[ii-1], /* command to resolve */
                descrPtr->cpuCode,/* CPU code command */
                sip5CpuCodes[ii-1], /* new CPU code */
                SNET_CHEETAH_ENGINE_UNIT_L2I_E,
                GT_FALSE);
    }

    __LOG(("new packet command : [%d]\n",
                  descrPtr->packetCmd));

}

/*******************************************************************************
*   snetChtL2iCommandAndCpuCodeResolution
*
* DESCRIPTION:
*       L2i Command And Cpu Code Resolution
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iCommandAndCpuCodeResolution
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iCommandAndCpuCodeResolution);

    SKERNEL_EXT_PACKET_CMD_ENT commands[MAX_PCKT_COMMANDS];    /* extended packet command */
    GT_U32 cpuCodes[MAX_CPU_CODES];                 /* CPU code */
    GT_U32 IEEE_CPU_INDEX_2_CODE[4] =  {SNET_CHT_IEEE_RES_MCAST_ADDR_TRAP_MIRROR,
                                        SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_1,
                                        SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_2,
                                        SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_3};
    GT_U32 ii;                                   /* array index */
    SKERNEL_EXT_PACKET_CMD_ENT newCmd;          /* new packet command */
    GT_U32 fldValue;                            /* entry field value */
    GT_U32 regAddr;                             /* register address */
    GT_U32 fldOffset;                           /* entry field offset */
    GT_U32 protocol;                            /* IEEE protocol */

    newCmd = descrPtr->packetCmd;

    commands[0] = cntrlPcktInfoPtr->bpduTrapCmd;
    commands[1] = saLookupInfoPtr->saCmd;
    commands[2] = daLookupInfoPtr->daCmd;
    commands[3] = cntrlPcktInfoPtr->arpBcastCmd;
    commands[4] = cntrlPcktInfoPtr->igmpCmd;
    commands[5] = filtersInfoPtr->unknownSaCmd;
    commands[6] = cntrlPcktInfoPtr->ieeePcktCmd;
    commands[7] = cntrlPcktInfoPtr->icmpV6Cmd;
    commands[8] = cntrlPcktInfoPtr->solicitationMcastCmd;
    commands[9] = cntrlPcktInfoPtr->ciscoPcktCmd;
    commands[10] = filtersInfoPtr->localPortCmd;
    commands[11] = filtersInfoPtr->stpCmd;
    commands[12] = filtersInfoPtr->ipmNonIpmDaCmd;
    commands[13] = filtersInfoPtr->vlanRangeCmd;
    commands[14] = filtersInfoPtr->inVlanFilterCmd;
    commands[15] = filtersInfoPtr->invalidSaCmd;
    commands[16] = filtersInfoPtr->staticMovedCmd;
    commands[17] = filtersInfoPtr->invldVlanCmd;
    commands[18] = cntrlPcktInfoPtr->ripV1Cmd;
    commands[19] = cntrlPcktInfoPtr->udpBcDestPortCmd;
    commands[20] = cntrlPcktInfoPtr->ipXMcLinkLocalProtCmd;
    commands[21] = filtersInfoPtr->frameTypeCmd;
    commands[22] = filtersInfoPtr->vlanMruCmd;
    commands[23] = filtersInfoPtr->arpMacSaMismatchCmd;
    commands[24] = filtersInfoPtr->secAutoLearnCmd;
    commands[25] = filtersInfoPtr->tcpSynWithData;
    commands[26] = filtersInfoPtr->tcpZeroFlagsCmd;
    commands[27] = filtersInfoPtr->tcpFinUrgPsgFlagsCmd;
    commands[28] = filtersInfoPtr->tcpSynFinFlagsCmd;
    commands[29] = filtersInfoPtr->tcpSynRstFlagsCmd;
    commands[30] = filtersInfoPtr->tcpPortZeroFlagsCmd;
    commands[31] = filtersInfoPtr->tcpOverMacMcCmd;
    commands[32] = filtersInfoPtr->accessCmd;
    commands[33] = filtersInfoPtr->icmpFragCmd;
    commands[34] = filtersInfoPtr->rateLimitCmd;

    if(descrPtr->capwap.validAction == GT_TRUE)
    {
        if(descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E)
        {
            commands[3] = descrPtr->capwap.action.arpCmd;
        }

        if(saLookupInfoPtr->found == 0)
        {
            commands[5] = descrPtr->capwap.action.unknownSaCmd;
        }
    }

    for (ii = 0; ii < MAX_PCKT_COMMANDS; ii++)
    {
        newCmd = snetChtPktCmdResolution(newCmd, commands[ii]);
    }

    descrPtr->packetCmd = newCmd;

    __LOG(("new packet command : [%d]\n",
                  descrPtr->packetCmd));

    if(newCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E ||
       newCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        ii = 0;

        /* resolve cpu code */
        commands[ii] = cntrlPcktInfoPtr->igmpCmd;
        cpuCodes[ii] = SNET_CHT_IPV4_IGMP_TRAP_MIRROR;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->solicitationMcastCmd;
        cpuCodes[ii] = SNET_CHT_IPV6_NEIGHBOR_SOLICITATION_TRAP_MIRROR;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->icmpV6Cmd;
        cpuCodes[ii] = SNET_CHT_IPV6_ICMP_TRAP_MIRROR;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->bpduTrapCmd;
        cpuCodes[ii] = SNET_CHT_BPDU_TRAP;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->arpBcastCmd;
        cpuCodes[ii] = SNET_CHT_ARP_BCAST_TRAP_MIRROR;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->ieeePcktCmd;
        if(SKERNEL_DEVICE_FAMILY_CHEETAH_1_ONLY(devObjPtr))
        {
            /* cheetah1 device not support different cpu codes by index */
            cpuCodes[ii] = SNET_CHT_IEEE_RES_MCAST_ADDR_TRAP_MIRROR;
        }
        else
        {
            /* IEEE Reserved mcast protocol */
            protocol = descrPtr->macDaPtr[5];

            /* IEEE Reserved Multicast CPU Index */
            regAddr = SMEM_CHT_IEEE_RESERV_MC_CPU_INDEX_TBL_MEM(devObjPtr,
                                                                protocol);

            fldOffset = 2 * protocol % 16;
            smemRegFldGet(devObjPtr, regAddr, fldOffset, 2, &fldValue);
            cpuCodes[ii] = IEEE_CPU_INDEX_2_CODE[fldValue];
        }
        ii++;

        commands[ii] = cntrlPcktInfoPtr->ipXMcLinkLocalProtCmd;
        cpuCodes[ii] = SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->ripV1Cmd;
        cpuCodes[ii] = SNET_CHT_IPV4_RIPV1_MIRROR;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->ciscoPcktCmd;
        cpuCodes[ii] = SNET_CHT_CISCO_CONTROL_MCAST_MAC_ADDR_TRAP_MIRROR;
        ii++;

        commands[ii] = saLookupInfoPtr->saCmd;
        cpuCodes[ii] = SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR;
        ii++;

        commands[ii] = daLookupInfoPtr->daCmd;
        cpuCodes[ii] = SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR;
        ii++;

        commands[ii] = filtersInfoPtr->unknownSaCmd;
        if (filtersInfoPtr->unknownSaCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
        {
            cpuCodes[ii] = SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_TRAP;
        }
        else
        {
            cpuCodes[ii] = SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_MIRROR;
        }
        ii++;

        commands[ii] = filtersInfoPtr->secAutoLearnCmd;
        cpuCodes[ii] = SNET_CHT_SEC_AUTO_LEARN_UNK_SRC_TRAP;
        ii++;

        commands[ii] = cntrlPcktInfoPtr->udpBcDestPortCmd;
        cpuCodes[ii] = SNET_CHT_UDP_BC_TRAP_MIRROR0 + descrPtr->udpBcCpuCodeInx;
        ii++;

        for (ii = 0; ii < MAX_CPU_CODES; ii++)
        {
            if (commands[ii] == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E ||
                commands[ii] == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
            {
                descrPtr->cpuCode = cpuCodes[ii];

                __LOG(("Cpu code taken from [%s] ,with value [%d] \n",
                              cpuCodeReasonName[ii],
                              descrPtr->cpuCode));

                break;
            }
        }
    }
}

/*******************************************************************************
*   snetChtL2iPhase1Decision
*
* DESCRIPTION:
*       Phase 1 L2i decision
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       filtersInfoPtr      - filters info structure pointer
*       bypassBridge        - bridge bypass indication and mode
*
*******************************************************************************/
static GT_VOID snetChtL2iPhase1Decision
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr,
    IN SNET_CHEETA_L2I_BYPASS_BRIDGE_ENT   bypassBridge
)
{
    DECLARE_FUNC_NAME(snetChtL2iPhase1Decision);


    GT_U32 fldValue;                            /* entry field value */
    GT_U32 regAddr;                             /* register address */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        sip5L2iCommandAndCpuCodeResolution(devObjPtr,descrPtr,
            vlanInfoPtr,saLookupInfoPtr,daLookupInfoPtr,
            cntrlPcktInfoPtr,filtersInfoPtr);

        if( (devObjPtr->errata.noTrafficToCpuOnBridgeBypassForwardingDecisionOnly) &&
            (SNET_CHEETA_L2I_BYPASS_BRIDGE_ONLY_FORWARD_DECISION_E == bypassBridge) &&
            ((SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E == descrPtr->packetCmd) ||
             (SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E == descrPtr->packetCmd)) )
        {
            /* No traffic to CPU in Bridge Bypass Forwrading Decision Only mode. */
            __LOG(("No traffic to CPU in Bridge Bypass Forwrading Decision Only mode\n"));

            if( SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E == descrPtr->packetCmd )
            {
                __LOG(("Packet comand changed from TRAP to FORWARD - not the expected behaviour\n"));
                __LOG(("ePort for forwarding is [%d]", descrPtr->trgEPort));
            }

            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
        }

        if(descrPtr->cpuCode == SNET_LION3_CPU_CODE_L2I_RATE_LIMITING)
        {
            /* the counting of drops due to rate limit should be done only if
               the actual drop reason was 'rate limit' , and this is according
               to the 'priorities' of the drop reasons */
            snetChtL2iPortRateLimitDropCount(devObjPtr,descrPtr,filtersInfoPtr);
        }

    }
    else
    {
        snetChtL2iCommandAndCpuCodeResolution(devObjPtr,descrPtr,
            vlanInfoPtr,saLookupInfoPtr,daLookupInfoPtr,
            cntrlPcktInfoPtr,filtersInfoPtr);
    }

    if(devObjPtr->supportEArch)
    {
        /************************/
        /*Mirroring from the FDB*/
        /************************/

        regAddr = SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr, &fldValue);

        if(saLookupInfoPtr->rxSniff)
        {
            fldValue = SMEM_U32_GET_FIELD(fldValue,17,3);

            /* SA Lookup Analyzer Index */
            __LOG(("SA Lookup Analyzer Index [%d] \n",
                          fldValue));

            snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
                fldValue);
        }

        if(daLookupInfoPtr->rxSniff)
        {
            fldValue = SMEM_U32_GET_FIELD(fldValue,20,3);

            __LOG(("DA Lookup Analyzer Index [%d] \n",
                          fldValue));

            /* DA Lookup Analyzer Index */
            snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
                fldValue);
        }

        /**********************************/
        /*Mirroring from the ingress vlan */
        /**********************************/
        if(descrPtr->isIp)
        {
            if(descrPtr->isIPv4)
            {
                if( SKERNEL_MULTICAST_MAC_E == descrPtr->macDaType ||
                    SKERNEL_BROADCAST_MAC_E == descrPtr->macDaType )
                {
                    __LOG(("vlan - IPv4 MC/BC Mirror to Analyzer Index [%d] \n",
                                  vlanInfoPtr->ipv4McBcMirrorToAnalyzerIndex));

                    /* vlan - IPv4 MC/BC Mirror to Analyzer Index */
                    snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
                        vlanInfoPtr->ipv4McBcMirrorToAnalyzerIndex);
                }
            }
            else /* ipv6 */
            {
                if(descrPtr->ipm)
                {
                    __LOG(("vlan - IPv6 MC Mirror to Analyzer Index [%d] \n",
                                  vlanInfoPtr->ipv6McMirrorToAnalyzerIndex));

                    /* vlan - IPv6 MC Mirror to Analyzer Index */
                    snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
                        vlanInfoPtr->ipv6McMirrorToAnalyzerIndex);
                }
            }
        }

        __LOG(("vlan Analyzer Index [%d] \n",
                      vlanInfoPtr->analyzerIndex));

        /* vlan Analyzer Index */
        snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
            vlanInfoPtr->analyzerIndex);

    }
    else
    if (vlanInfoPtr->ingressMirror ||
        saLookupInfoPtr->rxSniff || daLookupInfoPtr->rxSniff)
    {
        __LOG(("ingress mirror to analyzer due to one of: SA mirror[%d] DA mirror [%d] vlan mirror[%d] \n",
                      saLookupInfoPtr->rxSniff,
                      daLookupInfoPtr->rxSniff,
                      vlanInfoPtr->ingressMirror));

        descrPtr->rxSniff = 1;
    }

    /* Application-specific CPU Code assignment mechanism */
    if (daLookupInfoPtr->appSpecCpuCode == 1)
    {
        descrPtr->appSpecCpuCode = 1;
        __LOG(("Application-specific CPU Code assignment mechanism \n"));
    }

    if(daLookupInfoPtr->found)
    {
        __LOG(("When MAC DA found set descrPtr->egressFilterRegistered = 1 \n"));
        descrPtr->egressFilterRegistered = 1;
    }

    /* snetChtL2iPhase1Desicion_2 */
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        /* support multi-core FDB lookups */
        /*last core should behave like regular device*/
        if(devObjPtr->supportMultiPortGroupFdbLookUpMode &&
           (GT_FALSE == snetLionL2iPortGroupMaskUnknownDaEnable(devObjPtr, descrPtr)) )
        {
            /*non last core*/
            descrPtr->egressFilterRegistered = daLookupInfoPtr->found;
            __LOG(("egressFilterRegistered set according to 'mac da found' in multi-core FDB lookup on 'non last core' : egressFilterRegistered[%d] \n",
                          descrPtr->egressFilterRegistered));

        }
        else
        /* egressFilterRegistered it reached from the DSA tag when exists */
        if((descrPtr->marvellTagged && descrPtr->marvellTaggedExtended == SKERNEL_EXT_DSA_TAG_1_WORDS_E))
        {
            descrPtr->egressFilterRegistered = daLookupInfoPtr->found;

            __LOG(("egressFilterRegistered not exists in the '1 word' DSA tag so use 'mac da found' : egressFilterRegistered[%d] \n",
                          descrPtr->egressFilterRegistered));
        }

        if (filtersInfoPtr->pvlanEn == 0)
        {
            if( bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_NONE_E )
            {
                /* PVLAN disabled */
                descrPtr->useVidx = daLookupInfoPtr->useVidx;
                if (daLookupInfoPtr->found)
                {
                    if (descrPtr->useVidx)
                    {
                        if (descrPtr->basicMode)
                        {
                            __LOG(("In VLAN-Unaware Mode implicitly assign VIDX Mcast group , get the flood VIDX \n"));

                            /* In VLAN-Unaware Mode implicitly assign VIDX Mcast group */
                            /* get the flood VIDX */
                            snetChtL2iFloodVidxAssign(devObjPtr,descrPtr);
                        }
                        else
                        {
                            descrPtr->eVidx = daLookupInfoPtr->targed.vidx;
                        }

                        __LOG(("use fdb target USE_VIDX , vidx[0x%x] \n",
                               descrPtr->eVidx));
                    }
                    else
                    {
                        descrPtr->targetIsTrunk = daLookupInfoPtr->targed.ucast.isTrunk;
                        if (descrPtr->targetIsTrunk)
                        {
                            /* Trunk */
                            __LOG(("Trunk"));
                            descrPtr->trgTrunkId =
                                daLookupInfoPtr->targed.ucast.trunkId;

                            __LOG(("use fdb target trunk , trunk[0x%x] \n",
                                   descrPtr->trgTrunkId));
                        }
                        else
                        {
                            /* Port */
                            descrPtr->trgEPort = daLookupInfoPtr->targed.ucast.portNum;
                            /* call after setting trgEPort */
                            SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,bridge);
                            descrPtr->trgDev = daLookupInfoPtr->devNum;

                            __LOG(("use fdb target dev,port , device[0x%x] ,port[0x%x]\n",
                                   descrPtr->trgDev,
                                   descrPtr->trgEPort));
                        }
                    }

                    if(devObjPtr->supportEArch)
                    {
                        if(devObjPtr->unitEArchEnable.bridge || descrPtr->useVidx)
                        {
                            descrPtr->eArchExtInfo.isTrgPhyPortValid = 0;
                        }
                        else
                        {
                            descrPtr->eArchExtInfo.isTrgPhyPortValid = 1;
                        }

                        __LOG(("descrPtr->eArchExtInfo.isTrgPhyPortValid[%d]\n",
                               descrPtr->eArchExtInfo.isTrgPhyPortValid));
                    }
                }
                else
                {
                    /* get the flood VIDX */
                    snetChtL2iFloodVidxAssign(devObjPtr,descrPtr);

                    __LOG(("DA not found use flood VIDX[0x%x] \n",
                           descrPtr->eVidx));
                }
            }
            else /* bypassBridge == SNET_CHEETA_L2I_BYPASS_BRIDGE_ONLY_FORWARD_DECISION_E */
            {
                /* It is strange or even not appropriate behavior to override the value of     */
                /* <egressFilterRegistered> field when we are not those who set the forwarding */
                /* decision. A more expected behavior might have been not changing this field, */
                /* BUT that is the behavior of the actual device. */
                descrPtr->egressFilterRegistered = 1;
            }

            if (daLookupInfoPtr->found)
            {
                if(devObjPtr->supportEArch)
                {
                    /* VID1 assignment mode */
                    smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                                  27, 1, &fldValue);
                    if(fldValue)
                    {
                        __LOG(("VID1 assignment mode enables writing to desc<Original VID1> from FDB"));

                        /*<FDB <VID1>>*/
                        if(daLookupInfoPtr->vid1)
                        {
                            descrPtr->vid1 =  daLookupInfoPtr->vid1;

                            __LOG(("descrPtr->vid1 = FDB VID1[%d]\n",
                                   descrPtr->vid1));
                        }
                    }
                }
            }

        }
        else
        {
            /* PVLAN enabled */
            descrPtr->egressFilterRegistered = 1;
            descrPtr->targetIsTrunk = filtersInfoPtr->pvlanTrgIsTrunk;
            descrPtr->useVidx = 0;/*clear 'flood' indication that may be*/
            if (descrPtr->targetIsTrunk)
            {
                descrPtr->trgTrunkId = filtersInfoPtr->pvlanPortOrTrunkId;

                __LOG(("use PVE target trunk , trunk[0x%x] \n",
                              descrPtr->trgTrunkId));
            }
            else
            {
                descrPtr->trgEPort = filtersInfoPtr->pvlanPortOrTrunkId;
                /* call after setting trgEPort */
                SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,bridge);
                /* Port PVLAN Target Device */
                descrPtr->trgDev = filtersInfoPtr->pvlanDevNum;

                __LOG(("use PVE target dev,port , device[0x%x] ,port[0x%x]\n",
                              descrPtr->trgDev,
                              descrPtr->trgEPort));
            }
        }
        /* Triggering unicast routing */
        if (daLookupInfoPtr->daRoute == 1)
        {
            descrPtr->bridgeUcRoutEn = 1;
        }

        /* Triggering multicast routing */
        if (daLookupInfoPtr->daRoute == 1)
        {
            descrPtr->bridgeMcRoutEn = 1;
        }

    }
    else
    {
        /* egressFilterRegistered it reached from the DSA tag when exists */
        if((descrPtr->marvellTagged && descrPtr->marvellTaggedExtended == SKERNEL_EXT_DSA_TAG_1_WORDS_E) ||
           (descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
            descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CPU_E))
        {
            descrPtr->egressFilterRegistered = 1;

            __LOG(("egressFilterRegistered it reached from the DSA tag when exists\n"));
        }
    }
}

/*******************************************************************************
*   snetChtL2iPhase2Decision
*
* DESCRIPTION:
*       Phase 2 L2i decision - unknown/unregistered filtering
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iPhase2Decision
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iPhase2Decision);

    GT_BOOL retVal;                 /* return value */

    /* Multi-Port Group FDB Lookup support */
    if(devObjPtr->supportMultiPortGroupFdbLookUpMode)
    {
        retVal = snetLionL2iPortGroupMaskUnknownDaEnable(devObjPtr, descrPtr);
        /* Commands applied to unknown/unregistered packets in the "last port group" of the respective ring */
        if(retVal == GT_FALSE)
        {
            if(devObjPtr->supportEArch)
            {
                /* The bridge must assign desc<use_VIDX> = 0 to these packets to
                   avoid any duplication by the L2MLL unit.
                   (The EQ changes the packet command from flood to forward however
                    the EQ is located after the L2MLL unit).*/
                descrPtr->useVidx = 0;
            }
            return;
        }
    }

    if (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E)
    {
        /* unknown unicast */
        __LOG(("command resolution with: unknown unicast command[%d] \n",
                      vlanInfoPtr->unknownUcastCmd));
        descrPtr->packetCmd =
            snetChtPktCmdResolution(descrPtr->packetCmd,
                                    vlanInfoPtr->unknownUcastCmd);

        /* 12.13.2 Per-Egress port Unknown Unicast Filter */
        filtersInfoPtr->unknUcastCmd = descrPtr->packetCmd;

        if ((vlanInfoPtr->unknownUcastCmd ==
                SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ||
            (vlanInfoPtr->unknownUcastCmd ==
                SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
        {
            descrPtr->cpuCode = SNET_CHT_BRG_UNKN_UCAST_TRAP_MIRROR;

            __LOG(("CPU code from unknown unicast command[%d] \n",
                          descrPtr->cpuCode));
        }
        return;
    }

    if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
    {

        if (filtersInfoPtr->mcMacIsIpmV4)
        {
            /* unregistered IPv4 mcast */
            __LOG(("command resolution with: unregistered IPv4 mcast[%d] \n",
                          vlanInfoPtr->unregIPv4McastCmd));

            descrPtr->packetCmd =
                snetChtPktCmdResolution(descrPtr->packetCmd,
                                        vlanInfoPtr->unregIPv4McastCmd);
            /* 12.13.1.3 Per-VLAN Unregistered IPv4 Multicast Filtering */
            filtersInfoPtr->uregL2Ipv4McCmd = descrPtr->packetCmd;

            if ((vlanInfoPtr->unregIPv4McastCmd ==
                    SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ||
                (vlanInfoPtr->unregIPv4McastCmd ==
                    SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
            {
                descrPtr->cpuCode = SNET_CHT_BRG_IPV4_UNREG_MCAST_TRAP_MIRROR;

                __LOG(("CPU code from unregistered IPv4 mcast[%d] \n",
                              descrPtr->cpuCode));
            }
            return;
        }
        if (filtersInfoPtr->mcMacIsIpmV6 == 0)
        {
            /* non IP unregistered mcast */
            __LOG(("command resolution with: non IP unregistered mcast[%d] \n",
                          vlanInfoPtr->unregNonIpMcastCmd));
            descrPtr->packetCmd =
                snetChtPktCmdResolution(descrPtr->packetCmd,
                                        vlanInfoPtr->unregNonIpMcastCmd);
            /* 12.13.1.2 Per-VLAN Unregistered Non-IPv4/6 Multicast Filtering */
            filtersInfoPtr->uregL2NonIpmMcCmd = descrPtr->packetCmd;

            if ((vlanInfoPtr->unregNonIpMcastCmd ==
                    SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ||
                (vlanInfoPtr->unregNonIpMcastCmd ==
                    SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
            {
                descrPtr->cpuCode =
                    SNET_CHT_BRG_NON_IPV4_IPV6_UNREG_MCAST_TRAP_MIRROR;

                __LOG(("CPU code from non IP unregistered mcast[%d] \n",
                              descrPtr->cpuCode));
            }
            return;
        }

        /* unregistered IPv6 mcast */
        __LOG(("command resolution with: unregistered IPv6 mcast[%d] \n",
                      vlanInfoPtr->unregIPv6McastCmd));

        descrPtr->packetCmd =
            snetChtPktCmdResolution(descrPtr->packetCmd,
                                    vlanInfoPtr->unregIPv6McastCmd);
        /* 12.13.1.4 Per-VLAN Unregistered IPv6 Multicast Filtering */
        filtersInfoPtr->uregL2Ipv6McCmd = descrPtr->packetCmd;

        if ((vlanInfoPtr->unregIPv6McastCmd ==
                SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ||
            (vlanInfoPtr->unregIPv6McastCmd ==
                SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
        {
            descrPtr->cpuCode = SNET_CHT_BRG_IPV6_UNREG_MCAST_TRAP_MIRROR;

            __LOG(("CPU code from unregistered IPv6 mcast[%d] \n",
                          descrPtr->cpuCode));
        }
        return;
    }

    if ((descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) ||
         descrPtr->isIPv4)
    {
        if (descrPtr->ipv4Ripv1 == 1)
        {   /* RIP packet is not a subject to UnRegister BroadCast */
            __LOG(("RIP packet is not a subject to UnRegister BroadCast \n"));
            return;
        }

        __LOG(("command resolution with: Unregistered IPv4 Broadcast Filtering[%d] \n",
                      vlanInfoPtr->unregIp4BcastCmd));

        descrPtr->packetCmd =
            snetChtPktCmdResolution(descrPtr->packetCmd,
                                    vlanInfoPtr->unregIp4BcastCmd);
        /* 12.13.1.5 Per-VLAN Unregistered IPv4 Broadcast Filtering */
        filtersInfoPtr->uregL2Ipv4BcCmd = descrPtr->packetCmd;

        if ((vlanInfoPtr->unregIp4BcastCmd ==
                SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ||
            (vlanInfoPtr->unregIp4BcastCmd ==
                SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
        {
            descrPtr->cpuCode = SNET_CHT_IPV4_BCAST_TRAP_MIRROR;

            __LOG(("CPU code from Unregistered IPv4 Broadcast Filtering[%d] \n",
                          descrPtr->cpuCode));
        }
        return;
    }

    __LOG(("command resolution with: Unregistered non-IPv4 Broadcast Filtering[%d] \n",
                  vlanInfoPtr->unregNonIp4BcastCmd));

    descrPtr->packetCmd =
        snetChtPktCmdResolution(descrPtr->packetCmd,
                                vlanInfoPtr->unregNonIp4BcastCmd);
    /* 12.13.1.6 Per-VLAN Unregistered non-IPv4 Broadcast Filtering */
    filtersInfoPtr->uregL2NonIpv4BcCmd = descrPtr->packetCmd;

    if ((vlanInfoPtr->unregNonIp4BcastCmd ==
            SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E) ||
        (vlanInfoPtr->unregNonIp4BcastCmd ==
            SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E))
    {
        descrPtr->cpuCode = SNET_CHT_NON_IPV4_BCAST_TRAP_MIRROR;

        __LOG(("CPU code from Unregistered non-IPv4 Broadcast Filtering[%d] \n",
                      descrPtr->cpuCode));
    }
}

/*******************************************************************************
*   snetChtL2iQoS
*
* DESCRIPTION:
*       MAC base QoS processing
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iQoS
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iQoS);

    GT_U32 isSaQos = GT_FALSE;              /* SA QoS priority used */
    GT_U32 isDaQos = GT_FALSE;              /* DA QoS priority used */
    GT_U32  macQosTblIdx;                   /* MAC QoS table index */
    GT_U32 * regPtr;                        /* Register entry pointer */
    GT_U32 regAddr;                         /* Register address */
    GT_U32 modifyAttr;

    if (descrPtr->qosProfilePrecedence ==
        SKERNEL_QOS_PROF_PRECED_HARD)
    {
        /* previous QoS cannot be changed */
        __LOG(("previous QoS cannot be changed - due to 'hard' precedence \n"));
        return;
    }

    if (saLookupInfoPtr->found &&
        saLookupInfoPtr->saQosProfile)
    {
        /* SA QoS is used */
        isSaQos = GT_TRUE;
    }
    if (daLookupInfoPtr->found &&
        daLookupInfoPtr->daQosProfile)
    {
        /* DA QoS is used */
        isDaQos = GT_TRUE;
    }
    if (isDaQos == GT_FALSE && isSaQos == GT_FALSE)
    {
        /* No MAC QoS */
        __LOG(("No MAC SA/DA QoS assignment \n"));
        return;
    }

    if (isDaQos == GT_FALSE)
    {
        /* use SA QoS */
        macQosTblIdx = saLookupInfoPtr->saQosProfile;
        __LOG(("use SA QoS (DA QoS not valid) ,macQosTblIdx[%d] \n",
                      macQosTblIdx));
    }
    else
    if (isSaQos == GT_FALSE)
    {
        /* use DA QoS */
        macQosTblIdx = daLookupInfoPtr->daQosProfile;
        __LOG(("use DA QoS (SA QoS not valid) ,macQosTblIdx[%d] \n",
                      macQosTblIdx));
    }
    else
    {
        macQosTblIdx = (filtersInfoPtr->macQosConflictMode) ?
            daLookupInfoPtr->daQosProfile : saLookupInfoPtr->saQosProfile;
        __LOG(("both SA QoS and DA QoS are valid , but use [%s],macQosTblIdx[%d] \n",
                      (filtersInfoPtr->macQosConflictMode) ? "DA" : "SA",
                      macQosTblIdx));
    }

    /* MAC QoS Table Entry<n> (1<=n<8) */
    regAddr = SMEM_CHT_MAC_QOS_TBL_ENTRY_REG(devObjPtr, macQosTblIdx - 1);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* MAC QoS Profile */
        descrPtr->qos.qosProfile = SMEM_U32_GET_FIELD(*regPtr, 0, 10);
        modifyAttr =  SMEM_U32_GET_FIELD(*regPtr, 10, 2);
        /* Enable the modification of the UP field */
        descrPtr->modifyUp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 : descrPtr->modifyUp;
        /* Enable the modification of the DSCP field */
        modifyAttr =  SMEM_U32_GET_FIELD(*regPtr, 12, 2);
        descrPtr->modifyDscp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 : descrPtr->modifyDscp;
        /* Bridge Marking of the QoSProfile Precedence */
        descrPtr->qosProfilePrecedence = SMEM_U32_GET_FIELD(*regPtr, 14, 1);
    }
    else
    {
        /* MAC QoS Profile */
        descrPtr->qos.qosProfile = SMEM_U32_GET_FIELD(*regPtr, 0, 7);
        /* Enable the modification of the UP field */
        modifyAttr =  SMEM_U32_GET_FIELD(*regPtr, 7, 2);
        descrPtr->modifyUp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 : descrPtr->modifyUp;
        /* Enable the modification of the DSCP field */
        modifyAttr =  SMEM_U32_GET_FIELD(*regPtr, 9, 2);
        descrPtr->modifyDscp = (modifyAttr == 1) ? 1 : (modifyAttr == 2) ? 0 : descrPtr->modifyDscp;
        /* Bridge Marking of the QoSProfile Precedence */
        descrPtr->qosProfilePrecedence = SMEM_U32_GET_FIELD(*regPtr, 11, 1);
    }

}

/*******************************************************************************
*   snetCheetahL2iSecurity
*
* DESCRIPTION:
*       Security breach processing
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetCheetahL2iSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        sip5L2iSecurity(devObjPtr, descrPtr, vlanInfoPtr, filtersInfoPtr);
    }
    else if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {
        snetCht2L2iSecurity(devObjPtr, descrPtr, vlanInfoPtr, saLookupInfoPtr,
                           daLookupInfoPtr, filtersInfoPtr);
    }
    else
    {
        snetChtL2iSecurity(devObjPtr, descrPtr, vlanInfoPtr, saLookupInfoPtr,
                           daLookupInfoPtr, filtersInfoPtr) ;
    }
}

/*******************************************************************************
*   snetChtL2iSecurity
*
* DESCRIPTION:
*       Security breach processing for cheetah2 asic simulation
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iSecurity);

    GT_U32 breachCode = 0;                  /* security breach type */
    GT_U32 breachStatusInUse;               /* breach status in use flag */
    GT_U32 regValue;                        /* Register's value  */
    GT_U32 fldValue;                        /* Register's field value */
    GT_U32 * secBreachStatRegPtr;           /* Security breach status register*/
    GT_CHAR *securityBreachName = "";

    if (filtersInfoPtr->vlanRangeCmd)
    {
        breachCode = SNET_CHEETAH_BREACH_VLAN_RANGE_DROP_E;
        securityBreachName = " vlanRangeCmd ";
    }
    else
    if (filtersInfoPtr->inVlanFilterCmd)
    {
        breachCode = SNET_CHEETAH_BREACH_PORT_NOT_VLAN_MEMBER_E;
        securityBreachName = " inVlanFilterCmd ";
    }
    else
    if (filtersInfoPtr->invalidSaCmd)
    {
        breachCode = SNET_CHEETAH_BREACH_INVALID_SA_E;
        securityBreachName = " invalidSaCmd ";
    }
    else
    if ((saLookupInfoPtr->saCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ||
        (saLookupInfoPtr->saCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
        (daLookupInfoPtr->daCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ||
        (daLookupInfoPtr->daCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E))
    {
        breachCode = SNET_CHEETAH_BREACH_MAC_DROP_E;
        securityBreachName = " saCmd/daCmd ";
    }
    else
    if (filtersInfoPtr->staticMovedCmd)
    {
        breachCode = SNET_CHEETAH_BREACH_STATIC_MOVED_IS_SECURITY_BREACH_E;
        securityBreachName = " staticMovedCmd ";
    }
    else
    if (filtersInfoPtr->unknownSaCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
        filtersInfoPtr->unknownSaCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_UNKN_SA_E;

        if (filtersInfoPtr->naIsSecurityBreach)
        {
            breachCode = SNET_CHEETAH_BREACH_NEW_ADDR_IS_SECURITY_BREACH_E;
            securityBreachName = " unknownSaCmd ";
        }
    }
    else
    if (filtersInfoPtr->invldVlanCmd)
    {
        breachCode = SNET_CHEETAH_BREACH_INVALID_VLAN_E;
        securityBreachName = " invldVlanCmd ";
    }
    else
    if (saLookupInfoPtr->valid == 0 &&
        vlanInfoPtr->unknownIsNonSecurityEvent == 0)
    {
        breachCode = SNET_CHEETAH_BREACH_VLAN_NA_IS_SECURITY_BREACH;
        securityBreachName = " NA_IS_SECURITY_BREACH ";
    }

    if (breachCode == 0)
    {
        __LOG(("no security breach detected \n"));
        return;
    }

    __LOG(("security breach detected due to [%s]\n",
                  securityBreachName));

    /* this frame make security breach */
    filtersInfoPtr->isSecurityBreach = 1;

    /* convert CH security breach code to drop counter mode that is equal CH2 breach values */
    switch(breachCode)
    {
        case SNET_CHEETAH_BREACH_MAC_DROP_E:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_MAC_DROP_E;
            break;
        case SNET_CHEETAH_BREACH_INVALID_SA_E:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_INVALID_SA_E;
            break;
        case SNET_CHEETAH_BREACH_INVALID_VLAN_E:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_INVALID_VLAN_E;
            break;
        case SNET_CHEETAH_BREACH_PORT_NOT_VLAN_MEMBER_E:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_PORT_NOT_VLAN_MEMBER_E;
            break;
        case SNET_CHEETAH_BREACH_VLAN_RANGE_DROP_E:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_VLAN_RANGE_DROP_E;
            break;
        case SNET_CHEETAH_BREACH_STATIC_MOVED_IS_SECURITY_BREACH_E:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_STATIC_MOVED_IS_SECURITY_BREACH_E;
            break;
        case SNET_CHEETAH_BREACH_NEW_ADDR_IS_SECURITY_BREACH_E:
        case SNET_CHEETAH_BREACH_VLAN_NA_IS_SECURITY_BREACH:
            filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_UNKN_SA_E;
            break;
        default:
            filtersInfoPtr->dropCounterMode = breachCode;
            break;
    }

    /* Updates Bridge Drop Counter and Security Breach Drop Counters */
    snetChtL2iCounterSecurityBreach(devObjPtr, descrPtr);

    /* check status of breach event */
    secBreachStatRegPtr =
        smemMemGet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT0_REG(devObjPtr));
    breachStatusInUse = secBreachStatRegPtr[0];

    if (breachStatusInUse == GT_TRUE)
    {
        return;
    }

    /* Set value of SECURE_BREACH_STATUS_REG == GT_TRUE */
    secBreachStatRegPtr[0] = 1;

    /* Bits [31:0] of the source MAC address of the breaching packet */
    regValue = descrPtr->macDaPtr[5] |
              (descrPtr->macDaPtr[4] << 8) |
              (descrPtr->macDaPtr[3] << 16) |
              (descrPtr->macDaPtr[2] << 24);

    smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT0_REG(devObjPtr), regValue);

    /* Bits [47:32] of the source MAC address of the breaching packet */
    regValue = descrPtr->macDaPtr[1] | (descrPtr->macDaPtr[0] << 8);

    smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT1_REG(devObjPtr), regValue);

    fldValue = descrPtr->origSrcEPortOrTrnk;
    /* Security Breach Port */
    SMEM_U32_SET_FIELD(regValue, 16, 6, fldValue);

    fldValue = descrPtr->eVid;
    /* Security Breach VID */
    SMEM_U32_SET_FIELD(regValue, 4, 12, fldValue);

    fldValue = breachCode;
    /*Breach Code - bits[0:3] */
    SMEM_U32_SET_FIELD(regValue, 0, 4, fldValue) ;

    smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT2_REG(devObjPtr), regValue);

    __LOG(("Interrupt: Security Breach \n"));

    /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_BRIDGE_INT_CAUSE_REG(devObjPtr),
                          SMEM_CHT_BRIDGE_INT_MASK_CAUSE_REG(devObjPtr),
                          SMEM_CHT_UPD_SEC_BRICH_INT,
                          SMEM_CHT_L2I_SUM_INT(devObjPtr));
}

/*******************************************************************************
*   snetChtL2iCounterSecurityBreach
*
* DESCRIPTION:
*       Updates Bridge Drop Counter and Security Breach Drop Counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetChtL2iCounterSecurityBreach
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iCounterSecurityBreach);

    GT_U32 regAddr;                         /* Register address */
    GT_U32 regValue;                        /* Register's value  */
    GT_U32 fldValue;                        /* Register's field value */
    GT_U32 securBrchPort;                   /* Security breach port */
    GT_U32 securBrchVlan;                   /* Security breach VLAN */
    GT_U32 brgGlobCntrlReg1Data;    /* bridge global control register1 data */

    /* Update Global Security Breach Drop Counter */
    smemRegGet(devObjPtr, SMEM_CHT_GLB_SECUR_BRCH_DROP_CNT_REG(devObjPtr), &regValue);
    smemRegSet(devObjPtr, SMEM_CHT_GLB_SECUR_BRCH_DROP_CNT_REG(devObjPtr), ++regValue);

    __LOG(("Update Global Security Breach Drop Counter: increment by 1 from [%d] \n",
              (regValue-1)));

    /* Global configuration register1 data */
    smemRegGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr), &brgGlobCntrlReg1Data);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 17, 1);
    }
    else
    {
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 19, 1);
    }
    /* Port/VLAN Security Breach-Drop Counter Mode */
    if (!fldValue)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            smemRegGet(devObjPtr,
                SMEM_LION3_L2I_BRIDGE_SECURITY_BREACH_DROP_CNTR_CFG_0_REG(devObjPtr),
                &regValue);
            /*Security Breach DropCntEport*/
            securBrchPort = SMEM_U32_GET_FIELD(regValue, 0, 13);
        }
        else
        {
        /* Drop Counter Port */
            securBrchPort = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 1, 6);
        }

        if (securBrchPort == descrPtr->origSrcEPortOrTrnk)
        {
            /* Update Port Security Breach Drop Counter */
            regAddr = SMEM_CHT_PORT_VLAN_SECUR_BRCH_DROP_CNT_REG(devObjPtr);
            smemRegGet(devObjPtr, regAddr, &regValue);
            smemRegSet(devObjPtr, regAddr, ++regValue);

            __LOG(("Update Port[%d] Security Breach Drop Counter: increment by 1 from [%d]\n",
                          securBrchPort,(regValue-1)));
        }
        else
        {
            __LOG(("securBrchPort[%d] != descrPtr->origSrcEPortOrTrnk[%d]\n",
                          securBrchPort,descrPtr->origSrcEPortOrTrnk));
        }
    }
    else
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            smemRegGet(devObjPtr,
                SMEM_LION3_L2I_BRIDGE_SECURITY_BREACH_DROP_CNTR_CFG_1_REG(devObjPtr),
                &regValue);
            /*Security Breach DropCntEvlan*/
            securBrchVlan = SMEM_U32_GET_FIELD(regValue, 0, 13);
        }
        else
        {
            /* Drop Counter Vlan */
            securBrchVlan = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 7, 12);
        }

        if (securBrchVlan == descrPtr->eVid)
        {
            /* Update Port/VLAN Security Breach Drop Counter */
            __LOG(("Update Port/VLAN Security Breach Drop Counter"));
            regAddr = SMEM_CHT_PORT_VLAN_SECUR_BRCH_DROP_CNT_REG(devObjPtr);
            smemRegGet(devObjPtr, regAddr, &regValue);
            smemRegSet(devObjPtr, regAddr, ++regValue);

            __LOG(("Update VLAN[%d] Security Breach Drop Counter: increment by 1 from [%d]\n",
                          securBrchVlan,(regValue-1)));
        }
        else
        {
            __LOG(("securBrchVlan[%d] != descrPtr->eVid[%d]\n",
                          securBrchVlan,descrPtr->eVid));
        }
    }
}

/*******************************************************************************
*   snetCht2L2iSecurity
*
* DESCRIPTION:
*       Security breach processing for cheetah2 only
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       filtersInfoPtr      - filters info structure pointer
*
* COMMENTS:
*       The event is invoked according to the register
*       (Security Breach Status Register2)
*******************************************************************************/
static GT_VOID snetCht2L2iSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L2iSecurity);

    GT_U32 breachCode = 0;                  /* security breach type */
    GT_CHAR *securityBreachName = "";

    if ((saLookupInfoPtr->saCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ||
        (saLookupInfoPtr->saCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
        (daLookupInfoPtr->daCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ||
        (daLookupInfoPtr->daCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E))
    {
        breachCode = SNET_CHEETAH2_BREACH_MAC_DROP_E;
        securityBreachName = " saCmd/daCmd ";
    }
    else
    if (filtersInfoPtr->unknownSaCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
         filtersInfoPtr->unknownSaCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_UNKN_SA_E;

        if (filtersInfoPtr->naIsSecurityBreach ||
            vlanInfoPtr->unknownIsNonSecurityEvent == 0)
        {
            breachCode = SNET_CHEETAH2_BREACH_UNKN_SA_E;
            securityBreachName = " unknownSaCmd ";
        }
    }
    else
    if (filtersInfoPtr->invalidSaCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_INVALID_SA_E;
        securityBreachName = " invalidSaCmd ";
    }
    else
    if (filtersInfoPtr->invldVlanCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_INVALID_VLAN_E;
        securityBreachName = " invldVlanCmd ";
    }
    else
    if (filtersInfoPtr->inVlanFilterCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_PORT_NOT_VLAN_MEMBER_E;
        securityBreachName = " inVlanFilterCmd ";
    }
    else
    if (filtersInfoPtr->vlanRangeCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_VLAN_RANGE_DROP_E;
        securityBreachName = " vlanRangeCmd ";
    }
    else
    if (filtersInfoPtr->staticMovedCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_STATIC_MOVED_IS_SECURITY_BREACH_E;
        securityBreachName = " staticMovedCmd ";
    }
    else if (filtersInfoPtr->arpMacSaMismatchCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_ARP_SA_MISMATCH_BREACH_E;
        securityBreachName = " arpMacSaMismatchCmd ";
    }
    else if (filtersInfoPtr->tcpSynWithData)
    {
        filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_TCP_SYN_BREACH_E;
    }
    else if (filtersInfoPtr->tcpOverMacMcCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_TCP_OVER_MC_BREACH_E;
        securityBreachName = " tcpOverMacMcCmd ";
    }
    else if (filtersInfoPtr->accessCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_BRIDGE_ACCESS_MATRIX_BREACH_E;
        securityBreachName = " accessCmd ";
    }
    else if (filtersInfoPtr->secAutoLearnCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_SECURE_AUTOMATIC_LEARNING_BREACH_E;
        securityBreachName = " secAutoLearnCmd ";
    }
    else if (filtersInfoPtr->icmpFragCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BREACH_FRAGMENTED_ICMP_BREACH_E;
    }
    else if (filtersInfoPtr->frameTypeCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_ACCEPTABLE_FRAME_TYPE_BREACH_E;
        securityBreachName = " frameTypeCmd ";
    }
    else if (filtersInfoPtr->tcpZeroFlagsCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_TCP_FLAGS_ZERO_BREACH_E;
        securityBreachName = " tcpZeroFlagsCmd ";
    }
    else if (filtersInfoPtr->tcpFinUrgPsgFlagsCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_TCP_FLAGS_FIN_URG_PSH_BREACH_E;
        securityBreachName = " tcpFinUrgPsgFlagsCmd ";
    }
    else if (filtersInfoPtr->tcpSynFinFlagsCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_TCP_FLAGS_SYN_FIN_BREACH_E;
        securityBreachName = " tcpSynFinFlagsCmd ";
    }
    else if (filtersInfoPtr->tcpSynRstFlagsCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_TCP_FLAGS_SYN_RST_BREACH_E;
        securityBreachName = " tcpSynRstFlagsCmd ";
    }
    else if (filtersInfoPtr->tcpPortZeroFlagsCmd)
    {
        breachCode = SNET_CHEETAH2_BREACH_TCP_UDP_DEST_PORT_IS_0_BREACH_E;
        securityBreachName = " tcpPortZeroFlagsCmd ";
    }
    else if (filtersInfoPtr->vlanMruCmd)
    {
        filtersInfoPtr->dropCounterMode = SNET_CHEETAH2_BREACH_VLAN_MRU_DROP_E;
    }
    else if (filtersInfoPtr->autoLearnNoRelearnCmd)
    {
        breachCode = SNET_CHEETAH3_BREACH_AUTO_LEARN_NO_RELEARN_BREACH_E;
        securityBreachName = " autoLearnNoRelearnCmd ";
    }
    else if (filtersInfoPtr->ipmNonIpmDaCmd)
    {
        if (filtersInfoPtr->mcMacIsIpmV4 || filtersInfoPtr->mcMacIsIpmV6)
        {
            filtersInfoPtr->dropCounterMode =
                SNET_CHEETAH2_BREACH_IP_MULTICAST_DROP_BREACH_E;
        }
        else
        {
            filtersInfoPtr->dropCounterMode =
                SNET_CHEETAH2_BRG_DROP_CNTR_NON_IP_MC_E;
        }
    }
    else if (filtersInfoPtr->localPortCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_LOCAL_PORT_E;
    }
    else if (filtersInfoPtr->stpCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_SPAN_TREE_PORT_ST_E;
    }
    else if (filtersInfoPtr->uregL2NonIpmMcCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_NON_IPM_MC_E;
    }
    else if (filtersInfoPtr->uregL2Ipv6McCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_IPV6_MC_E;
    }
    else if (filtersInfoPtr->uregL2Ipv4McCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_IPV4_MC_E;
    }
    else if (filtersInfoPtr->unknUcastCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_UNKNOWN_L2_UC_E;

    }
    else if (filtersInfoPtr->uregL2Ipv4BcCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_IPV4_BC_E;
    }
    else if (filtersInfoPtr->uregL2NonIpv4BcCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_DROP_CNTR_UREG_L2_NON_IPV4_BC_E;
    }
    else if(filtersInfoPtr->rateLimitCmd)
    {
        filtersInfoPtr->dropCounterMode =
            SNET_CHEETAH2_BRG_RATE_LIMIT_DROP_E;
    }

    if (breachCode == 0)
    {
        __LOG(("no security breach detected \n"));
        return;
    }

    __LOG(("security breach detected due to [%s]\n",
                  securityBreachName));

    /* this frame make security breach */
    filtersInfoPtr->isSecurityBreach = 1;
    /* drop counter mode code */
    filtersInfoPtr->dropCounterMode = breachCode;

    return;
}

/*******************************************************************************
*   snetChtL2iSecurityBreachStatusUpdate
*
* DESCRIPTION:
*       Security breach last event setting.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       filtersInfoPtr      - filters info structure pointer
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetChtL2iSecurityBreachStatusUpdate
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iSecurityBreachStatusUpdate);

    GT_U32 breachStatusInUse;               /* breach status in use flag */
    GT_U32 regValue;                        /* Register's value  */
    GT_U32  *internalMemPtr;                /* pointer to internal memory */

    GT_U32 breachCode ; /* security breach type */

    breachCode = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                 filtersInfoPtr->sip5SecurityBreachDropMode:/* sip 5 security reason */
                 filtersInfoPtr->dropCounterMode ;

    /* check status of breach event - in the simulation internal register*/
    internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
        CHT_INTERNAL_SIMULATION_USE_MEM_SECURITY_BREACH_STATUS_E);

    /*bit 0 - indication that the registers of 'Security breach status' ready for the device to update
                value 0 - the CPU did not read the values yet , so the device can not update it.
                value 1 - the CPU read the values , so the device can update it.*/
    breachStatusInUse = SMEM_U32_GET_FIELD(internalMemPtr[0],0,1) ? GT_FALSE : GT_TRUE;

    if (breachStatusInUse == GT_TRUE)
    {
        return;
    }

    /* Bits [31:0] of the source MAC address of the breaching packet */
    regValue = descrPtr->macSaPtr[5] |
              (descrPtr->macSaPtr[4] << 8) |
              (descrPtr->macSaPtr[3] << 16) |
              (descrPtr->macSaPtr[2] << 24);

    smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT0_REG(devObjPtr), regValue);

    /* Bits [47:32] of the source MAC address of the breaching packet */
    regValue = descrPtr->macSaPtr[1] | (descrPtr->macSaPtr[0] << 8);

    smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT1_REG(devObjPtr), regValue);


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Security Breach Port */
        /* bits 0..12*/
        regValue = SMEM_U32_GET_FIELD(descrPtr->origSrcEPortOrTrnk, 0, 13);

        /*Breach Code - bits[22:26] */
        SMEM_U32_SET_FIELD(regValue, 22, 5, breachCode) ;

        smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT2_REG(devObjPtr), regValue);

        /* bits 0..12*/
        regValue = SMEM_U32_GET_FIELD(descrPtr->eVid,0,13);
        smemRegSet(devObjPtr, SMEM_LION3_L2I_SECURITY_BREACH_STATUS_3_REG(devObjPtr), regValue);
    }
    else
    {
        /* Security Breach Port */
        /* bits 16..21*/
        regValue = SMEM_U32_GET_FIELD(descrPtr->origSrcEPortOrTrnk, 0, 6) << 16;

        /* Security Breach VID */
        SMEM_U32_SET_FIELD(regValue, 4, 12, descrPtr->eVid);

        /*Breach Code - bits[22:26] */
        SMEM_U32_SET_FIELD(regValue, 22, 5, breachCode) ;

        smemRegSet(devObjPtr, SMEM_CHT_SECUR_BRCH_STAT2_REG(devObjPtr), regValue);
    }

    /* state that the registers are updated and CPU not read it yet
       (after updating the registers and before generate the interrupt) */
    SMEM_U32_SET_FIELD(internalMemPtr[0],0,1,0);

    __LOG(("Interrupt: Security Breach code [%d] , port[%d] , vlan[%d] \n",
            breachCode,
            descrPtr->origSrcEPortOrTrnk,
            descrPtr->eVid));

    /* Generate interrupt  "Bridge Interrupt Cause Register" bit 28 */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_BRIDGE_INT_CAUSE_REG(devObjPtr),
                          SMEM_CHT_BRIDGE_INT_MASK_CAUSE_REG(devObjPtr),
                          SMEM_CHT_UPD_SEC_BRICH_INT,
                          SMEM_CHT_L2I_SUM_INT(devObjPtr));
}

/*******************************************************************************
*   sip5L2iSecurity
*
* DESCRIPTION:
*       Sip5 : Security breach processing
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       filtersInfoPtr      - filters info structure pointer
*
* OUTPUT:
*       descrPtr            - pointer to the frame's descriptor.
*        filtersInfoPtr      - filters info structure pointer
*
*******************************************************************************/
static GT_VOID sip5L2iSecurity(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN    SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    INOUT SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(sip5L2iSecurity);

    switch(filtersInfoPtr->securityBreachMode)
    {
        case SNET_LION3_CPU_CODE_L2I_MAC_SA_MOVED:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_MacSaMoved;
            break;
        case SNET_LION3_CPU_CODE_L2I_MAC_SPOOF:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_MacSpoofProtection;
            break;
        case SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_FDB_entry_command;
            break;
        case SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_TRAP:
            filtersInfoPtr->sip5SecurityBreachDropMode = vlanInfoPtr->autoLearnDisable == 0 ?
                SNET_LION3_L2I_SECURITY_BREACH_PortNewAddress :/*port controlled learn */
                SNET_LION3_L2I_SECURITY_BREACH_VLANNewAddress; /*vlan controlled learn*/
            break;
        case SNET_CHT_SEC_AUTO_LEARN_UNK_SRC_TRAP:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_SecureAutomaticLearning;
            break;
        case SNET_LION3_CPU_CODE_L2I_INVALID_SA:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_InvalidMACSourceAddress;
            break;
        case SNET_LION3_CPU_CODE_L2I_SA_IS_DA:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_MacSaIsDa;
            break;
        case SNET_LION3_CPU_CODE_L2I_VLAN_NOT_VALID:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_VLANNotValid;
            break;
        case SNET_LION3_CPU_CODE_L2I_PORT_NOT_VLAN_MEM:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_PortNotMemberInVLAN;
            break;
        case SNET_LION3_CPU_CODE_L2I_VLAN_RANGE:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_VLANRangeDrop;
            break;
        case SNET_LION3_CPU_CODE_L2I_STATIC_ADDR_MOVED:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_MovedStaticAddres;
            break;
        case SNET_LION3_CPU_CODE_L2I_ARP_SA_MISMATCH:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_ARPSAMismatchSecurity;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_SYN_WITH_DATA:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCPSYNwithDataSecurity;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_OVER_MC_BC:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCPoverMCSecurity;
            break;
        case SNET_LION3_CPU_CODE_L2I_BRIDGE_ACCESS_MATRIX:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_BridgeAccessMatrixSecurity;
            break;
        case SNET_LION3_CPU_CODE_L2I_ACCEPT_FRAME_TYPE:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_AcceptableFrameType;
            break;
        case SNET_LION3_CPU_CODE_L2I_FRAGMENT_ICMP:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_FragmentedICMPSecurity;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_ZERO:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsZeroSecurity;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_FIN_URG_PSH:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsFINURGandPSHSet;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_FIN:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsSYNandFINSet;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_RST:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCPFlagsSYNandRSTSet;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_UDP_SRC_DEST_ZERO:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TCP_UDPSourceorDestinationPortisZero;
            break;
        case SNET_LION3_CPU_CODE_L2I_SIP_IS_DIP:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_SipIsDip;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_UDP_SPORT_IS_DPORT:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TcpUdpSportIsDport;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_WITH_FIN_WITHOUT_ACK:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TcpFlagsWithFinWithoutAck;
            break;
        case SNET_LION3_CPU_CODE_L2I_TCP_WITHOUT_FULL_HEADER:
            filtersInfoPtr->sip5SecurityBreachDropMode = SNET_LION3_L2I_SECURITY_BREACH_TcpWithoutFullHeader;
            break;
        default:
            if(filtersInfoPtr->dropCounterMode)
            {
            /* this frame make NO security breach */
            __LOG(("no security breach detected \n"));
            }
            else
            {
                /* this frame was not dropped */
                __LOG(("the bridge not 'drop' , so also no security breach detected \n"));
            }
            return;
    }

    /* set as security breach */
    filtersInfoPtr->isSecurityBreach = 1;

    __LOG(("security breach detected : id[%d] , name [%s] \n",
            filtersInfoPtr->sip5SecurityBreachDropMode,
            sip5SecurityBreachNames[filtersInfoPtr->sip5SecurityBreachDropMode]));

    return;
}

/*******************************************************************************
*   snetChtL2iCounterHostCounters
*
* DESCRIPTION:
*       Update Bridge counter host counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iCounterHostCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iCounterHostCounters);

    GT_BOOL macSaMatch = GT_FALSE;  /* DA matching */
    GT_BOOL macDaMatch = GT_FALSE;  /* SA matching */
    GT_U32 * regPtr;                /* Register entry pointer */
    GT_U8 monitorMacSa[6];          /* SA monitoring address */
    GT_U8 monitorMacDa[6];          /* DA monitoring address */
    GT_U8 * macAddrPtr;             /* pointer to MAC address */

    /* Get monitorMacSa and monitorMacDa from MAC Address Count0-2 Register */
    regPtr = smemMemGet(devObjPtr, SMEM_CHT_MAC_ADDR_CNT0_REG(devObjPtr));

    monitorMacDa[2] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr, 24, 8);
    monitorMacDa[3] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr, 16, 8);
    monitorMacDa[4] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,  8, 8);
    monitorMacDa[5] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,  0, 8);

    regPtr = smemMemGet(devObjPtr, SMEM_CHT_MAC_ADDR_CNT1_REG(devObjPtr));

    monitorMacSa[4] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,  24, 8);
    monitorMacSa[5] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,  16, 8);
    monitorMacDa[0] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,   8, 8);
    monitorMacDa[1] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,   0, 8);

    regPtr = smemMemGet(devObjPtr, SMEM_CHT_MAC_ADDR_CNT2_REG(devObjPtr));

    monitorMacSa[0] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,  24, 8);
    monitorMacSa[1] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,  16, 8);
    monitorMacSa[2] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,   8, 8);
    monitorMacSa[3] = (GT_U8)SMEM_U32_GET_FIELD(*regPtr,   0, 8);

    macAddrPtr = &monitorMacSa[0];
    if (SGT_MAC_ADDR_ARE_EQUAL(descrPtr->macSaPtr, macAddrPtr))
    {
        macSaMatch = GT_TRUE;
    }

    macAddrPtr = &monitorMacDa[0];
    if (SGT_MAC_ADDR_ARE_EQUAL(descrPtr->macDaPtr, macAddrPtr))
    {
        macDaMatch = GT_TRUE;
    }

    if (macSaMatch == GT_TRUE)
    {
        regPtr = smemMemGet(devObjPtr, SMEM_CHT_HOST_OUT_PKT_CNT_REG(devObjPtr));
        (*regPtr)++;

        __LOG(("increment (by 1) HOST_OUT_PKT counters from[%d] \n",
                      (*regPtr)-1));

        if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E)
        {
            regPtr = smemMemGet(devObjPtr, SMEM_CHT_HOST_OUT_MCST_PKT_CNT_REG(devObjPtr));
            (*regPtr)++;

            __LOG(("increment (by 1) HOST_OUT_MCST_PKT counters from[%d] \n",
                          (*regPtr)-1));

        }
        else
        if (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E  ||
                descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E )
        {
            regPtr = smemMemGet(devObjPtr, SMEM_CHT_HOST_OUT_BCST_PKT_CNT_REG(devObjPtr));
            (*regPtr)++;

            __LOG(("increment (by 1) HOST_OUT_BCST_PKT counters from[%d] \n",
                          (*regPtr)-1));
        }

    }

    if (macDaMatch == GT_TRUE)
    {
        regPtr = smemMemGet(devObjPtr, SMEM_CHT_HOST_IN_PKT_CNT_REG(devObjPtr));
        (*regPtr)++;

        __LOG(("increment (by 1) HOST_IN_PKT counters from[%d] \n",
                      (*regPtr)-1));
    }

    if (macDaMatch == GT_TRUE &&
        macSaMatch == GT_TRUE)
    {
        regPtr = smemMemGet(devObjPtr, SMEM_CHT_MATRIX_SRC_DST_PKT_CNT_REG(devObjPtr));
        (*regPtr)++;

        __LOG(("increment (by 1) _MATRIX_SRC_DST_PKT counters from[%d] \n",
                      (*regPtr)-1));
    }
}


/*******************************************************************************
*   snetLion3L2iCounterSets
*
* DESCRIPTION:
*       SIP5 : Update Bridge counter sets 0,1
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetLion3L2iCounterSets
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT preL2iPacketCmd,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion3L2iCounterSets);

    GT_BOOL doCount;                /* increment counter enable */
    GT_U32  regAddr0,regAddr1;      /* Register address */
    GT_U32  regVal0,regVal1;        /* Register value */
    GT_U32  filterRegAddr;          /* address for the filtered counters register */
    GT_U32  incomingPacketCountAddr;/* address for the Incoming Packet Count Register */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 ii;                       /* counters set index */
    GT_U32  port,vlan;/* port and vlan for counting */
    enum FILTER_MODE_ENT{
        NO_FILTER_E,
        VLAN_FILTER_E,
        SECURITY_FILTER_E,
        OTHER_FILTER_E
    }filterMode;
    GT_CHAR*    filterNameArr[]={
        STR(NO_FILTER_E),
        STR(VLAN_FILTER_E),
        STR(SECURITY_FILTER_E),
        STR(OTHER_FILTER_E)
    };
    SNET_CHEETAH_COUNTER_SET_MODE_STC  counterMode;

    /* check that the drop is due to L2i mechanism */
    if (filtersInfoPtr->dropCounterMode)
    {
        if (IS_DROP_COMMAND_MAC(filtersInfoPtr->inVlanFilterCmd) ||
            IS_DROP_COMMAND_MAC(filtersInfoPtr->vlanRangeCmd   ) ||
            IS_DROP_COMMAND_MAC(filtersInfoPtr->invldVlanCmd   ) ||
            IS_DROP_COMMAND_MAC(filtersInfoPtr->vlanMruCmd     ) )
        {
            filterMode = VLAN_FILTER_E;
        }
        else
        if (filtersInfoPtr->isSecurityBreach)
        {
            filterMode = SECURITY_FILTER_E;
        }
        else
        {
            filterMode = OTHER_FILTER_E;
        }
    }
    else
    {
        filterMode = NO_FILTER_E;
    }

    for (ii = 0; ii < 2; ii++)
    {
        if(ii == 0)
        {
            regAddr0 = SMEM_LION3_L2I_CNTRS_SET_0_CONFIG_0_REG(devObjPtr);
            regAddr1 = SMEM_LION3_L2I_CNTRS_SET_0_CONFIG_1_REG(devObjPtr);
            incomingPacketCountAddr = SMEM_CHT_SET0_IN_PKT_CNT_REG(devObjPtr);
            switch(filterMode)
            {
                case VLAN_FILTER_E:
                    filterRegAddr = SMEM_CHT_SET0_VLAN_IN_FILTER_CNT_REG(devObjPtr);
                    break;
                case SECURITY_FILTER_E:
                    filterRegAddr = SMEM_CHT_SET0_SECUR_FILTER_CNT_REG(devObjPtr);
                    break;
                case OTHER_FILTER_E:
                    filterRegAddr = SMEM_CHT_SET0_BRDG_FILTER_CNT_REG(devObjPtr);
                    break;
                default:
                    filterRegAddr = SMAIN_NOT_VALID_CNS;
                    break;
            }
        }
        else  /*i==1*/
        {
            regAddr0 = SMEM_LION3_L2I_CNTRS_SET_1_CONFIG_0_REG(devObjPtr);
            regAddr1 = SMEM_LION3_L2I_CNTRS_SET_1_CONFIG_1_REG(devObjPtr);
            incomingPacketCountAddr = SMEM_LION3_L2I_SET1_IN_PKT_CNT_REG(devObjPtr);
            switch(filterMode)
            {
                case VLAN_FILTER_E:
                    filterRegAddr = SMEM_LION3_L2I_SET1_VLAN_IN_FILTER_CNT_REG(devObjPtr);
                    break;
                case SECURITY_FILTER_E:
                    filterRegAddr = SMEM_LION3_L2I_SET1_SECUR_FILTER_CNT_REG(devObjPtr);
                    break;
                case OTHER_FILTER_E:
                    filterRegAddr = SMEM_LION3_L2I_SET1_BRDG_FILTER_CNT_REG(devObjPtr);
                    break;
                default:
                    filterRegAddr = SMAIN_NOT_VALID_CNS;
                    break;
            }
        }
        smemRegGet(devObjPtr,regAddr0,&regVal0);
        smemRegGet(devObjPtr,regAddr1,&regVal1);

        counterMode = SMEM_U32_GET_FIELD(regVal0, 0, 2);
        port = SMEM_U32_GET_FIELD(regVal0, 2, 13);
        vlan = SMEM_U32_GET_FIELD(regVal1, 0, 13);

        /* Set<i> Count Mode */
        switch(counterMode)
        {
            case SNET_CHEETAH_COUNT_ALL_E:
                doCount = GT_TRUE;
                break;
            case SNET_CHEETAH_COUNT_RCV_PORT_E:
                doCount = (port == descrPtr->eArchExtInfo.localDevSrcEPort) ? GT_TRUE : GT_FALSE;
                if(doCount == GT_FALSE)
                {
                    __LOG(("NOT count : counter set[%d] : not match port[%d]with descrPtr->eArchExtInfo.localDevSrcEPort[%d] \n",
                                  ii, port, descrPtr->eArchExtInfo.localDevSrcEPort));
                }
                break;
            case SNET_CHEETAH_COUNT_RCV_VLAN_E:
                doCount = (vlan == descrPtr->eVid) ? GT_TRUE : GT_FALSE;
                if(doCount == GT_FALSE)
                {
                    __LOG(("NOT count : counter set[%d] : not match vlan[%d] with descrPtr->eVid[%d] \n",
                                  ii, vlan, descrPtr->eVid));
                }
                break;
            default:/*SNET_CHEETAH_COUNT_RCV_PORT_VLAN_E*/
                doCount = (port == descrPtr->eArchExtInfo.localDevSrcEPort &&
                           vlan == descrPtr->eVid)  ? GT_TRUE : GT_FALSE;
                if(doCount == GT_FALSE)
                {
                    __LOG(("NOT count : counter set[%d] : not match port[%d] and vlan[%d] with descrPtr->eArchExtInfo.localDevSrcEPort[%d] and descrPtr->eVid[%d] \n",
                                  ii, port, vlan , descrPtr->eArchExtInfo.localDevSrcEPort , descrPtr->eVid));
                }
                break;
        }

        if (doCount == GT_FALSE)
        {
            continue;
        }

        /* Incoming Packet Count Register */
        smemRegGet(devObjPtr, incomingPacketCountAddr, &fldValue);
        smemRegSet(devObjPtr, incomingPacketCountAddr, ++fldValue);

        __LOG(("increment (by 1) Incoming Packet Count (set[%d]) counters from[%d] \n",
                      ii,
                      fldValue-1));

        if(filterRegAddr == SMAIN_NOT_VALID_CNS)
        {
            __LOG(("counter set[%d] packet not filtered by bridge \n",
                          ii));
            continue;
        }

        smemRegGet(devObjPtr, filterRegAddr, &fldValue);
        smemRegSet(devObjPtr, filterRegAddr, ++fldValue);
        __LOG(("[%s] filter counter : increment (by 1) (set[%d]) counters from[%d] \n",
                      filterNameArr[filterMode],
                      ii,
                      fldValue-1));

    }/*ii*/

    return;
}

/*******************************************************************************
*   snetChtL2iCounterSets
*
* DESCRIPTION:
*       Update Bridge counter sets 0,1
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
*******************************************************************************/
static GT_VOID snetChtL2iCounterSets
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT preL2iPacketCmd,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iCounterSets);

    GT_BOOL vlanDrop = GT_FALSE;    /* VLAN ingress filtered packets drop */
    GT_BOOL securDrop = GT_FALSE;   /* security breach filtered packets drop */
    GT_BOOL bridgeDrop = GT_FALSE;  /* bridge filtered drop */
    GT_U32  countCfgReg;            /* counter configuration register */
    GT_BOOL doCount;                /* increment counter enable */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 * regPtr;                /* Register entry pointer */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 i;                       /* counters set index */
    GT_U32  port,vlan;/* port and vlan for counting */
    GT_U32  portToMatch;/* port to match*/

    /* check need for 'drop counters' of the L2i , check that the drop is due to L2i mechanism */
    if ((preL2iPacketCmd != SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) &&
        (preL2iPacketCmd != SKERNEL_EXT_PKT_CMD_HARD_DROP_E) &&
        ((descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ||
         (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
         (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)))
    {
        if ((saLookupInfoPtr->saCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (daLookupInfoPtr->daCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->invalidSaCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->staticMovedCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->unknownSaCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->frameTypeCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->arpMacSaMismatchCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->secAutoLearnCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E))
        {
            securDrop = GT_TRUE;
        }

        if ((filtersInfoPtr->inVlanFilterCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->vlanRangeCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->invldVlanCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->vlanMruCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) )
        {
            vlanDrop = GT_TRUE;
        }

        if ((filtersInfoPtr->localPortCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->stpCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E) ||
            (filtersInfoPtr->ipmNonIpmDaCmd >= SKERNEL_EXT_PKT_CMD_HARD_DROP_E))
        {
            bridgeDrop = GT_TRUE;
        }

        if (vlanDrop == GT_FALSE && securDrop == GT_FALSE &&
            descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
        {
            bridgeDrop = GT_TRUE;
        }
    }

    for (i = 0; i < 2; i++)
    {
        regAddr = (SMEM_CHT_CNT_SET0_CONF_REG(devObjPtr) + (0x14 * i));
        regPtr = smemMemGet(devObjPtr, regAddr);
        countCfgReg = *regPtr;

        fldValue = SMEM_U32_GET_FIELD(countCfgReg, 0, 2);
        port = SMEM_U32_GET_FIELD(countCfgReg, 2, 6);
        vlan = SMEM_U32_GET_FIELD(countCfgReg, 8, 12);

        portToMatch = descrPtr->localDevSrcPort;

        /* reset doCount variable */
        doCount = GT_FALSE;

        /* Set<i> Count Mode */
        if (fldValue == SNET_CHEETAH_COUNT_ALL_E)
        {
            doCount = GT_TRUE;
        }
        else
        if (fldValue == SNET_CHEETAH_COUNT_RCV_PORT_E)
        {
            /* Set<i>Port */
            if (port == portToMatch)
            {
                doCount = GT_TRUE;
            }
            else
            {
                __LOG(("NOT count : counter set[%d] : not match port[%d]with descrPtr->localDevSrcPort[%d] \n",
                              i, port, portToMatch));
            }
        }
        else
        if (fldValue == SNET_CHEETAH_COUNT_RCV_VLAN_E)
        {
            /* Set<i>Vid */
            if (vlan == descrPtr->eVid)
            {
                doCount = GT_TRUE;
            }
            else
            {
                __LOG(("NOT count : counter set[%d] : not match vlan[%d] with descrPtr->eVid[%d] \n",
                              i, vlan, descrPtr->eVid));
            }
        }
        else
        if (fldValue == SNET_CHEETAH_COUNT_RCV_PORT_VLAN_E)
        {
            if (port == portToMatch && vlan == descrPtr->eVid)
            {
                doCount = GT_TRUE;
            }
            else
            {
                __LOG(("NOT count : counter set[%d] : not match port[%d] and vlan[%d] with descrPtr->localDevSrcPort[%d] and descrPtr->eVid[%d] \n",
                              i, port, vlan , portToMatch , descrPtr->eVid));
            }
        }

        if (doCount == GT_FALSE)
        {
            continue;
        }

        /* Incoming Packet Count Register */
        regAddr = (SMEM_CHT_SET0_IN_PKT_CNT_REG(devObjPtr) + (0x14 * i));
        smemRegGet(devObjPtr, regAddr, &fldValue);
        smemRegSet(devObjPtr, regAddr, ++fldValue);

        __LOG(("increment (by 1) Incoming Packet Count (set[%d]) counters from[%d] \n",
                      i,fldValue-1));

        if (vlanDrop == GT_TRUE)
        {
            /* VLAN Ingress Filtered Packet Count Register */
            regAddr = (SMEM_CHT_SET0_VLAN_IN_FILTER_CNT_REG(devObjPtr) + (0x14 * i));
            smemRegGet(devObjPtr, regAddr, &fldValue);
            smemRegSet(devObjPtr, regAddr, ++fldValue);

            __LOG(("increment (by 1) VLAN Ingress Filtered Packet Count (set[%d]) counters from[%d] \n",
                          i,fldValue-1));
        }
        else /* security breach exclude vlan drops */
        if (securDrop == GT_TRUE)
        {
            /* Security Filtered Packet Count Register */
            regAddr = (SMEM_CHT_SET0_SECUR_FILTER_CNT_REG(devObjPtr) + (0x14 * i));
            smemRegGet(devObjPtr, regAddr, &fldValue);
            smemRegSet(devObjPtr, regAddr, ++fldValue);

            __LOG(("increment (by 1) Security Filtered Packet Count (set[%d]) counters from[%d] \n",
                          i,fldValue-1));
        }
        else /* filtered frames exclude vlan drops and security breach */
        if (bridgeDrop)
        {
            /* Bridge Filtered Packet Count Register */
            regAddr = (SMEM_CHT_SET0_BRDG_FILTER_CNT_REG(devObjPtr) + (0x14 * i));
            smemRegGet(devObjPtr, regAddr, &fldValue);
            smemRegSet(devObjPtr, regAddr, ++fldValue);

            __LOG(("increment (by 1) Bridge Filtered Packet Count (set[%d]) counters from[%d] \n",
                          i,fldValue-1));
        }
    }
}

/*******************************************************************************
*   snetChtL2iCounterDropMode
*
* DESCRIPTION:
*       Update Bridge counter of drop mode . count drops according to drop mode
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       filtersInfoPtr      - filters info structure pointer
*
*******************************************************************************/
static GT_VOID snetChtL2iCounterDropMode
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iCounterDropMode);

    GT_U32  brgGlobCntrlReg1Data;          /* Bridge Global Configuration Register1 */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 filteredCntBits;         /* amount of bits in filterd packets counter */

    if(filtersInfoPtr->dropCounterMode == 0)
    {
        return;
    }

    /* Update bridge filter counters */
    smemRegGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr), &brgGlobCntrlReg1Data);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Bridge Drop Counter Mode */
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 18, 8);
    }
    else
    if (SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
    {
        /* Bridge Drop Counter Mode[5:0] */
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 20, 6);
    }
    else
    {
        /* Bridge Drop Counter Mode[4:0] */
        fldValue = SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 20, 5);
        if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
        {
            /* Bridge Drop Counter Mode[5] */
            fldValue |= SMEM_U32_GET_FIELD(brgGlobCntrlReg1Data, 0, 1) << 5;
        }
    }

    if (fldValue == 0 || (fldValue == filtersInfoPtr->dropCounterMode))
    {
        if (SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            filteredCntBits = 16;
        }
        else
        {
            /* Although Cider describes this counter as 16 bits */
            filteredCntBits = 32;
        }

        /* Update Bridge Filter Counter */
        smemRegFldGet(
            devObjPtr, SMEM_CHT_BRDG_FILTER_CNT_REG(devObjPtr),
            0, filteredCntBits, &fldValue);
        smemRegFldSet(
            devObjPtr, SMEM_CHT_BRDG_FILTER_CNT_REG(devObjPtr),
            0, filteredCntBits, ++fldValue);

        __LOG(("increment (by 1) Update Bridge Filter  counters from[%d] \n",
                      fldValue-1));
    }
    else
    {
        __LOG(("the drop reason [%d] is not triggered for counting (only[%d]counted) \n",
                      filtersInfoPtr->dropCounterMode,
                      fldValue));
    }

}

/*******************************************************************************
*   snetChtL2iCounters
*
* DESCRIPTION:
*       Update Bridge counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       preL2iPacketCmd     - pointer to extended packet commands
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       daLookupInfoPtr     - pointer to DA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       filtersInfoPtr      - filters info structure pointer
*
*
*******************************************************************************/
static GT_VOID snetChtL2iCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT preL2iPacketCmd,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{

    if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr) &&
       filtersInfoPtr->isSecurityBreach)/**/
    {
        /* Updates Bridge Drop Counter and Security Breach Drop Counters */
        snetChtL2iCounterSecurityBreach(devObjPtr, descrPtr);
    }

    /* do bridge host counters */
    snetChtL2iCounterHostCounters(devObjPtr,descrPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* process counter sets 0,1 */
        snetLion3L2iCounterSets(devObjPtr,descrPtr,preL2iPacketCmd,saLookupInfoPtr,daLookupInfoPtr,filtersInfoPtr);
    }
    else
    {
        /* process counter sets 0,1 */
        snetChtL2iCounterSets(devObjPtr,descrPtr,preL2iPacketCmd,saLookupInfoPtr,daLookupInfoPtr,filtersInfoPtr);
    }

    /* count drops according to drop mode */
    snetChtL2iCounterDropMode(devObjPtr,descrPtr,filtersInfoPtr);

    /* log flow counters update */
    snetChtL2iLogFlow(devObjPtr,descrPtr,
                        saLookupInfoPtr->found ? GT_TRUE : GT_FALSE,
                        saLookupInfoPtr->entryIndex,
                        daLookupInfoPtr->found ? GT_TRUE : GT_FALSE,
                        daLookupInfoPtr->entryIndex);

}

/*******************************************************************************
*   snetChtSaLearningEArchNonVidxInfo
*
* DESCRIPTION:
*        get info for NON-VIDX learning - on EArch supporting device
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        descrPtr           -  descriptor
*
* OUTPUTS:
*       learnInfoPtr  - (pointer to) FDB extra info
*
*******************************************************************************/
static GT_VOID snetChtSaLearningEArchNonVidxInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr
)
{


    GT_U32 FDB_port_trunk; /*port/trunkid to the FDB*/
    GT_BIT FDB_isTrunk;    /*isTunk to the FDB      */
    GT_U32 FDB_devNum;     /*devNum to the FDB      */
    GT_U32 FDB_coreId;     /*coreId to the FDB      */

    /*
        always learn the orig information. Note that this
        modification is possible as orig info is always valid, even if packet is received without marvell tag, or
        with marvell tag from CC enabled port.
        Following is the modified learning logic:
        FDB<IsTrunk> = <OrigIsTrunk>
        FDB<ePort/trunk> = InDesc<OrigSRCePort/Trunk>
        FDB<Device> = (SRCePortIsGlobal = 1) ? OwnDev : InDesc<OrigSrcDev>
    */

    FDB_isTrunk = descrPtr->origIsTrunk;
    FDB_port_trunk = descrPtr->origSrcEPortOrTrnk;

    if(learnInfoPtr->srcEPortIsGlobal)
    {
        FDB_devNum = descrPtr->ownDev;
        if(descrPtr->ownDev != descrPtr->srcDev)
        {
            __LOG_NO_LOCATION_META_DATA(("NOTE: FDB learn of source Global ePort is on 'ownDev' [0x%x] (and not srcDev from the DSA [0x%x]) \n",
                descrPtr->ownDev , descrPtr->srcDev));
        }
    }
    else
    {
        FDB_devNum = descrPtr->srcDev;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* the FDB not care about coreId , and it can't not get it from the 'port number' ... */
        /* so not used in sip5 !!! */
        FDB_coreId = SMAIN_NOT_VALID_CNS;
    }
    else
    if(devObjPtr->portGroupSharedDevObjPtr && /* multi-core device*/
       descrPtr->marvellTagged == 0 &&  /* non-DSA tag */
       FDB_isTrunk == 0)                /* non trunk */
    {
        /* multi port group device */
        FDB_coreId = devObjPtr->portGroupId;
    }
    else /*not multi-core device or trunk */
    {
        /* 3 MSB of port/trunk */
        FDB_coreId = (FDB_port_trunk >> 4);
    }/*not multi-core device or trunk or dsa */

    learnInfoPtr->FDB_port_trunk = FDB_port_trunk;
    learnInfoPtr->FDB_isTrunk    = FDB_isTrunk;
    learnInfoPtr->FDB_devNum     = FDB_devNum;
    learnInfoPtr->FDB_coreId     = FDB_coreId;

    __LOG_PARAM_NO_LOCATION_META_DATA(FDB_port_trunk);
    __LOG_PARAM_NO_LOCATION_META_DATA(FDB_isTrunk);
    __LOG_PARAM_NO_LOCATION_META_DATA(FDB_devNum);
    __LOG_PARAM_NO_LOCATION_META_DATA(FDB_coreId);

    return;
}

/*******************************************************************************
*   snetChtL2iSaLearning
*
* DESCRIPTION:
*       SA Learning
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       vlanInfoPtr         - VLAN and STG info pointer
*       saLookupInfoPtr     - pointer to SA lookup info structure
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*       filtersInfoPtr      - filters info structure pointer
*       learnInfoPtr  - (pointer to) FDB extra info
*
*
*******************************************************************************/
static GT_VOID snetChtL2iSaLearning
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_CNTRL_PACKET_INFO * cntrlPcktInfoPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iSaLearning);

    GT_U32 entryIndex = 0;      /* index of MAC table entry to write new
                                   or update old */
    GT_BOOL updateFdb;          /* GT_TRUE - FDB should be updated */
    GT_BOOL freeFound = GT_FALSE; /* GT_TRUE - free place in FDB found */
    GT_BOOL sendFailedNa2Cpu;   /* GT_TRUE - send failed to learn
                                   New Addr Message to CPU */
    GT_BOOL na2CpuWasSent;      /* GT_TRUE - NA was successfully sent,
                                   GT_FALSE not sent */
    GT_BOOL learnFailed;        /* GT_TRUE - auto-learning was failed because
                                   internal problems,   GT_FALSE was learned */
    GT_U32 spUnknown;           /* storm prevention */

    GT_U32 regAddr;             /* Register address */
    GT_U32 regVal;              /* Register entry value */
    GT_U32 causBitMap=0;        /* Interrupt cause bitmap  */
    GT_U32 fieldVal;            /* Register field value */
    GT_U32 securAccessLevel;    /* FDB entry security access level */
    GT_BOOL naMsgToCpuEn;/*do we send NA to the CPU */

    __LOG_PARAM(filtersInfoPtr->isSecurityBreach);
    __LOG_PARAM(saLookupInfoPtr->valid);
    __LOG_PARAM(saLookupInfoPtr->isMoved);
    __LOG_PARAM(saLookupInfoPtr->isStatic);
    __LOG_PARAM(vlanInfoPtr->spanState);
    __LOG_PARAM(filtersInfoPtr->isMacSpoofProtectionEvent);
    __LOG_PARAM(filtersInfoPtr->movedMacCmd);

    if(descrPtr->l2Valid == 0)
    {
        /* No Learning */
        __LOG(("No learning is performed: MAC SA and MAC DA are not valid when l2Valid=false\n"));
        return;
    }
    if (filtersInfoPtr->isSecurityBreach || (saLookupInfoPtr->valid &&
        (saLookupInfoPtr->isMoved == 0 || saLookupInfoPtr->isStatic)) ||
        (vlanInfoPtr->spanState == SKERNEL_STP_BLOCK_LISTEN_E) ||
        (filtersInfoPtr->isMacSpoofProtectionEvent) ||
        (filtersInfoPtr->movedMacCmd >= SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E))
    {
        /* No Learning and no NA Messages */
        __LOG(("No Learning and no NA Messages : due to one of :\n"));

        __LOG_PARAM(filtersInfoPtr->isSecurityBreach);
        __LOG_PARAM(saLookupInfoPtr->valid &&
            (saLookupInfoPtr->isMoved == 0 || saLookupInfoPtr->isStatic));
        __LOG_PARAM(vlanInfoPtr->spanState == SKERNEL_STP_BLOCK_LISTEN_E);
        __LOG_PARAM(filtersInfoPtr->movedMacCmd >= SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E);

        return;
    }

    if (SKERNEL_IS_XCAT2_DEV(devObjPtr))
    {

        if (cntrlPcktInfoPtr->ieeePcktCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
        {
            if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
            {
                /*<EnLearnOnTrapIEEEReservedMC> */
                fieldVal =
                    SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                        SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ON_TRAP_IEEE_RSRV_MC);
            }
            else
            {
                regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF1_REG(devObjPtr, descrPtr->localDevSrcPort);
                smemRegFldGet(devObjPtr, regAddr, 25, 1, &fieldVal);
            }

            /* Enable Learn On Trap IEEE Reserved MC */
            if (fieldVal == 0)
            {
                /* No learning is performed for IEEE packets when command is trapped.
                (When a packet is TRAPPED for any other reason, learning will still be performed) */
                __LOG(("No learning is performed for IEEE packets when command is trapped \n"
                              "(When a packet is TRAPPED for any other reason, learning will still be performed) \n"));
                return;
            }
        }
    }

    sendFailedNa2Cpu = GT_FALSE;
    na2CpuWasSent = GT_TRUE;
    learnFailed = GT_FALSE;

    spUnknown = (filtersInfoPtr->naStormPreventEn &
                 !filtersInfoPtr->autoLearnEn);

    if(spUnknown == 1 &&
       saLookupInfoPtr->valid &&
       saLookupInfoPtr->stormPrevent &&
       saLookupInfoPtr->isMoved == 0)
    {
        /* storm prevention of NA to the CPU for this message */
        naMsgToCpuEn = GT_FALSE;

        __LOG(("naMsgToCpuEn = 0 ,do not send NA to CPU , since the SA found as SP unknown and prevention in enabled \n"));
    }
    else
    {
        naMsgToCpuEn = filtersInfoPtr->naMsgToCpuEn;

        __LOG(("naMsgToCpuEn[%d] according to filter info \n",
                      naMsgToCpuEn));
    }


    if (filtersInfoPtr->autoLearnEn ||   /* automatic learning is enabled */
       spUnknown) /*auto learn of SP for 'Controlled learning'*/
    {
        __LOG(("FDB should be updated , due to : automatic learning is enabled[%d] or auto learn of SP[%d] for 'Controlled learning' \n",
                      filtersInfoPtr->autoLearnEn,spUnknown));

        /* FDB should be updated */
        updateFdb = GT_TRUE;
        if (saLookupInfoPtr->valid == 0)
        {
            entryIndex = saLookupInfoPtr->entryIndex;
            freeFound = (saLookupInfoPtr->entryIndex == SMAIN_NOT_VALID_CNS) ? GT_FALSE : GT_TRUE;
            sendFailedNa2Cpu = saLookupInfoPtr->sendFailedNa2Cpu;

            if(freeFound == GT_TRUE)
            {
                /* Find free place for new FDB */
                __LOG(("Find free place for new FDB - index [%d] \n",entryIndex));
            }
            else
            {
                /* Find free place for new FDB */
                __LOG(("NOT found free place for new FDB entry \n"));
            }
        }
        else
        {
            /* Update old entry */
            __LOG(("Update existing entry - index [%d] \n",saLookupInfoPtr->entryIndex));
            entryIndex  = saLookupInfoPtr->entryIndex;
            freeFound = GT_TRUE;
        }
    }
    else
    {
        updateFdb = GT_FALSE;
    }

    /* Assign value for Tag1 VID field of FDB. Value is used for both NA message
       and FDB update. Value assigned only for control learning.
       In control learning: descrPtr->vid1 for Bobcat2 B0 and above and 0 for other devices. */
    if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
    {
        if (filtersInfoPtr->autoLearnEn)
        {
            /* VID1 assignment mode */
            smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                          27, 1, &fieldVal);
            if(fieldVal)
            {
                __LOG(("VID1 assignment mode enables desc<VID1> written to FDB"));

                /*Enable Learning Of Tag1 VID*/
                regVal =
                    SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                         SMEM_LION3_L2I_EPORT_TABLE_FIELDS_EN_LEARN_ORIG_TAG1_VID);

                if(regVal || vlanInfoPtr->fdbLookupKeyMode == 1)
                {
                    /*desc<VID1>*/
                    __LOG(("Enable Learning Of Tag1 VID: use descrPtr->vid1 = [%d]",descrPtr->vid1));
                    learnInfoPtr->FDB_origVid1 = descrPtr->vid1;
                }
            }
        }
        else
        {
            if (devObjPtr->errata.fdbNaMsgVid1Assign)
            {
                __LOG(("Control learning assigns Tag1 VID to be 0 in FDB"));
            }
            else
            {
                __LOG(("Enable Learning Of Tag1 VID: use descrPtr->vid1 = [%d]",descrPtr->vid1));
                learnInfoPtr->FDB_origVid1 = descrPtr->vid1;
            }
        }

        /* relevant to auto learn and for controlled learn */
        /* see functions: sfdbLion3FdbSpecialMuxedFieldsSet,sfdbLion3FdbSpecialMuxedFieldsGet
            that handles the muxed SRC_ID and the UDB */
        learnInfoPtr->FDB_UDB = descrPtr->localPortGroupPortAsGlobalDevicePort;
        __LOG_PARAM(learnInfoPtr->FDB_UDB);
    }

    if (filtersInfoPtr->autoLearnEn && freeFound == GT_FALSE)
    {
        __LOG(("autoLearnEn && BUT NOT FOUND free place \n"));

        learnFailed = GT_TRUE;
        updateFdb = GT_FALSE;
    }
    else
    if (naMsgToCpuEn)/*send message to the CPU*/
    {
        if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
        {
            /* update access matrix security levels according
               to auto learning defaults like in the snetChtL2iAutoLearnFdbEntry.
               "When source MAC address auto-learning is enabled
               (Section 12.4.7.2 "Automatic Source MAC Address Learning"),
               a new address is learned with a <MAC-DA Security Level> and
               <MAC-SA Security Level> based on global configurable default
               values."  */

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* DA security access Level */
                smemRegFldGet(devObjPtr,SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr),
                                                                 4,3,&securAccessLevel);
                descrPtr->daAccessLevel = securAccessLevel;

                /* SA security access Level */
                smemRegFldGet(devObjPtr,SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr),
                                                                 0,3,&securAccessLevel);
                descrPtr->saAccessLevel = securAccessLevel;
            }
            else
            {
                /* DA security access Level */
                smemRegFldGet(devObjPtr,SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr),
                                                                 12,3,&securAccessLevel);
                descrPtr->daAccessLevel = securAccessLevel;

                /* SA security access Level */
                smemRegFldGet(devObjPtr,SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr),
                                                                 8,3,&securAccessLevel);
                descrPtr->saAccessLevel = securAccessLevel;
            }

            __LOG(("DA security access Level[%d] , SA security access Level[%d] \n",
                          descrPtr->daAccessLevel,descrPtr->saAccessLevel));
        }

        __LOG(("try to send NA message to the CPU \n"));

        /* Sends NA message to CPU */
        na2CpuWasSent =
            snetChtL2iSendNa2Cpu(devObjPtr, descrPtr, sendFailedNa2Cpu,learnInfoPtr);

        if(na2CpuWasSent == GT_FALSE)
        {
            __LOG(("failed to send the NA to the CPU \n"));
        }

        if (updateFdb == GT_TRUE && na2CpuWasSent == GT_FALSE)
        {
            /* No FDB update */
            __LOG(("No FDB update , because message could not reach CPU \n"));
            learnFailed = GT_TRUE;
            updateFdb = GT_FALSE;
        }
    }

    if (updateFdb == GT_TRUE && freeFound == GT_TRUE)
    {
        __LOG(("allow FDB update , and have free index to update \n"));

        learnInfoPtr->FDB_srcId = descrPtr->sstId;

        /* Write entry to FDB */
        snetChtL2iAutoLearnFdbEntry(devObjPtr, descrPtr, vlanInfoPtr,spUnknown, entryIndex,learnInfoPtr,saLookupInfoPtr->valid);
    }
    else
    if (learnFailed == GT_TRUE)
    {
        /* increment counter - Learned Entry Discards Count Register */
        regAddr = SMEM_CHT_LEARN_ENTRY_DISCARD_CNT_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr, &regVal);
        smemRegSet(devObjPtr, regAddr, ++regVal);

        __LOG(("FDB update is not performed , increment counter from [%d] \n",
                      regVal-1));

        causBitMap |= SMEM_CHT_NA_NOT_LEARN_INT;

        if (freeFound == GT_FALSE && updateFdb == GT_TRUE)
        {
            causBitMap |= SMEM_CHT_NUM_OF_HOPS_EXP_INT;
        }

        __LOG(("Interrupt: New Address Not Learned \n"));

        /* New Address Not Learned */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                              causBitMap, SMEM_CHT_FDB_SUM_INT(devObjPtr));
    }
}

/*******************************************************************************
*   snetChtL2iSendUpdMsg2Cpu
*
* DESCRIPTION:
*       Send MAC Update message to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macUpdMsgPtr     - MAC update message
*
*
*******************************************************************************/
GT_BOOL snetChtL2iSendUpdMsg2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * macUpdMsgPtr
)
{
    GT_BOOL status;       /* function return value */
    GT_U32  auMsgsToCpuIf;  /* 1 - used PCI accessed memory, 0 - used on Chip queue */
    GT_U32  fieldOffset;  /* field offset */
    GT_U32  fdbCfgRegAddr; /* FDB Global Configuration register address */

#if 0 /*already protected by "devObjPtr->portGroupSharedDevObjPtr->fullPacketWalkThroughProtectMutex" */
    /* NOTE: the SCIB_SEM_TAKE here cause 'deadlock' when working with 'simLogToRuntime' */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* the AUQ is single instance.
           so we need to synch the accessing to it's resources */
        SCIB_SEM_TAKE;
    }
#endif /*0*/

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        fieldOffset = 21;
    }
    else
    {
        fieldOffset = 20;
    }

    fdbCfgRegAddr = SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr);
    smemRegFldGet(devObjPtr, fdbCfgRegAddr, fieldOffset, 1, &auMsgsToCpuIf);

    /* PCI Master Transaction initiated by the device or by a register of
       the AU messages FIFO */
    if (auMsgsToCpuIf)
    {
        /* The device initiates a PCI Master transaction for each AU message it
           forwards to the CPU */

        status = snetChtL2iPciAuMsg(devObjPtr, macUpdMsgPtr);
    }
    else
    {
        /* AU Messages to the CPU are stored in the AU messages FIFO and
           must be read by the CPU via a register read access */

        status = snetChtL2iFifoAuMsg(devObjPtr, macUpdMsgPtr);
    }

    /* if the device started 'soft reset' we not care if NA not reached the CPU
       and we must not 'wait until success' so declare success !!! */
    if(devObjPtr->needToDoSoftReset && status == GT_FALSE)
    {
        status = GT_TRUE;
    }


#if 0 /*already protected by "devObjPtr->portGroupSharedDevObjPtr->fullPacketWalkThroughProtectMutex" */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        SCIB_SEM_SIGNAL;
    }
#endif /*0*/

    return status;
}

/*******************************************************************************
*   snetCht2L2iSendFuMsg2Cpu
*
* DESCRIPTION:
*       Send FDB Upload message to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macFuMsgPtr         - FDB Upload message
*
*
*******************************************************************************/
GT_BOOL snetCht2L2iSendFuMsg2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * macFuMsgPtr
)
{
    DECLARE_FUNC_NAME(snetCht2L2iSendFuMsg2Cpu);

    GT_U32 regAddr;                 /* Register address */
    GT_U32 fldValue;                /* Register's field value */
    GT_BOOL status;                 /* function return value */


    regAddr = SMEM_CHT_FU_QUE_BASE_ADDR_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 31, 1, &fldValue);


    /* Enable forwarding address update message with type = FDB to a separate q*/
    if (fldValue)
    {
        __LOG(("FUQ use separate Queue then the AUQ \n"));
        /* The device initiates a PCI Master transaction for each FU message it
           forwards to the CPU */
        status = snetChtL2iPciFuMsg(devObjPtr, macFuMsgPtr,devObjPtr->numOfWordsInAuMsg);
    }
    else
    {
        __LOG(("FUQ use the same Queue as the AUQ \n"));
        /* FU queue is disabled and Address Update with FDB (FU) are
            queued to the Address Update message queue */
        status = snetChtL2iSendUpdMsg2Cpu(devObjPtr, macFuMsgPtr);
    }

    /* if the device started 'soft reset' we not care if NA not reached the CPU
       and we must not 'wait until success' so declare success !!! */
    if(devObjPtr->needToDoSoftReset && status == GT_FALSE)
    {
        status = GT_TRUE;
    }

    return status;
}

/*******************************************************************************
*   snetChtL2iSendNa2Cpu
*
* DESCRIPTION:
*       Send New Address message to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       sendFailedNa2Cpu    - hash chain over
*       learnInfoPtr  - (pointer to) FDB extra info
*
*******************************************************************************/
static GT_BOOL snetChtL2iSendNa2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL sendFailedNa2Cpu,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iSendNa2Cpu);

    GT_BOOL status;                 /* function return value */
    GT_U32 macUpdateMsg[SMEM_CHT_AU_MSG_WORDS];/* message send to CPU */

    __LOG(("create MAC update message\n"));

    snetChtCreateAuMsg(devObjPtr, descrPtr, &macUpdateMsg[0],learnInfoPtr);

    /* send MAC update message to CPU */
    __LOG(("send MAC update message to CPU \n"));
    status = snetChtL2iSendUpdMsg2Cpu(devObjPtr, &macUpdateMsg[0]);

    return status;
}

/*******************************************************************************
*   snetChtL2iFifoAuMsg
*
* DESCRIPTION:
*       Send New Address message to CPU (AU messages FIFO)
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       macUpdMsgPtr        - pointer to MAC update message
*
* RETURN:
*       GT_TRUE if find free place in FIFO, FALSE otherwise
*
*******************************************************************************/
static GT_BOOL snetChtL2iFifoAuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U32 * macUpdMsgPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iFifoAuMsg);

    GT_BOOL status = GT_FALSE;          /* function return status */
    GT_U32 * fifoBufferPtr;         /* fifo memory pointer */
    GT_U32 fifoSize;                    /* fifo memory size */
    GT_U32 regAddr;                     /* Register address */
    GT_U32 fldValue;                    /* Register's field value */
    GT_U32 i;                           /* fifo index index */
    CHT_AUQ_FIFO_MEM    *auqFifoInfoPtr;
    GT_U32  numOfEntries;/* number of entries that can occupy the on chip fifo */

    auqFifoInfoPtr = SMEM_CHT_MAC_AUQ_FIFO_MEM_GET(devObjPtr);
    fifoBufferPtr = &auqFifoInfoPtr->macUpdFifoRegs[0];

    regAddr = SMEM_CHT_AU_FIFO_TO_CPU_CONF_REG(devObjPtr);
    /* CPUFifo-threshold */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegFldGet(devObjPtr, regAddr, 0, 7, &fldValue);
    }
    else
    {
        smemRegFldGet(devObjPtr, regAddr, 0, 5, &fldValue);
    }

    __LOG(("CPUFifo-threshold [%d]",fldValue));

    /* The fldValue is maximum number of MAC Update message 8-byte words in the
       MAC Update message FIFO to the CPU. Convert value to 4-byte words. */
    fifoSize = fldValue * 2;

    /* calculate the number of AU messages that can be in the 'on chip' fifo */
    numOfEntries = fifoSize / devObjPtr->numOfWordsInAuMsg;

    /* protect FIFO management */
    SCIB_SEM_TAKE;
    for (i = 0; i < numOfEntries; i++, fifoBufferPtr+= devObjPtr->numOfWordsInAuMsg)
    {
        /* check only the 'FIRST word' of the message */
        if ((*fifoBufferPtr) != SMEM_CHEETAH_INVALID_MAC_MSG_CNS)
        {
            continue;
        }

        memcpy(fifoBufferPtr, macUpdMsgPtr, 4*devObjPtr->numOfWordsInAuMsg);
        break;
    }
    SCIB_SEM_SIGNAL;

    if (i == numOfEntries)
    {
        /* indication that can't put the NA into the on chip fifo */

       /* MAC update messages FIFO to CPU has exceeded its threshold */
        __LOG(("Interrupt: Fifo Full \n"
                      "MAC update messages FIFO to CPU has exceeded its threshold \n"));

        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                              SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                                SMEM_LION3_AU_FIFO_FULL_INT :
                                SMEM_CHT_AU_FIFO_FULL_INT,
                              SMEM_CHT_FDB_SUM_INT(devObjPtr));

        status = GT_FALSE;
    }
    else
    {
        /* Put new message in FIFO */
        __LOG(("Interrupt: Message to CPU is ready in the FIFO \n"));

        /* Message to CPU is ready in the FIFO */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                              SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                                SMEM_LION3_AU_MSG_TO_CPU_READY_INT :
                                SMEM_CHT_AU_MSG_TO_CPU_READY_INT,
                              SMEM_CHT_FDB_SUM_INT(devObjPtr));
        status = GT_TRUE;
    }

    return status;
}

/*******************************************************************************
*   checkFdbIpv6DataMatch
*
* DESCRIPTION:
*       Perform FDB Lookup: check ipv6 'data' match
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryInfoPtr    - pointer to entry hash info
*       fdbEntryPtrPtr  - pointer to FDB 'key' entry pointer
*       entryIndexPtr   - ipv6 FDB entry index array
*       calculatedHashArr - calculated hash array
*
* OUTPUT:
*       entryIndexPtr   - ipv6 FDB entry index array
*
* RETURN:
*       GT_TRUE         - FDB entry found
*       GT_FALSE        - FDB entry not found
*
*******************************************************************************/
static GT_BOOL checkFdbIpv6DataMatch
(
    IN    SKERNEL_DEVICE_OBJECT                 *devObjPtr,
    IN    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC  *entryInfoPtr,
    IN    GT_U32                               **fdbEntryPtrPtr,
    INOUT GT_U32                                *entryIndexPtr,
    IN    GT_U32                                *calculatedHashArr
)
{
    DECLARE_FUNC_NAME(checkFdbIpv6DataMatch);

    GT_U32  regAddr;
    GT_U32  nhDataBunkNum;
    GT_U32  validBit;
    GT_U32  dataVrfId;
    GT_U32  dataEntryIndx;
    GT_U32  *dataEntryPtr;
    SNET_CHEETAH_FDB_ENTRY_ENT dataEntryType;


    nhDataBunkNum = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, *fdbEntryPtrPtr, *entryIndexPtr,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM);

    dataEntryIndx = calculatedHashArr[nhDataBunkNum];

    __LOG_PARAM(dataEntryIndx);

    /* convert the hash index into FDB index */
    dataEntryIndx *= (devObjPtr->fdbNumOfBanks);
    dataEntryIndx += nhDataBunkNum;

    __LOG_PARAM(dataEntryIndx);


    __LOG(("Get entryPtr according to entryIndx[0x%8.8x]", dataEntryIndx));
    regAddr = SMEM_CHT_MAC_TBL_MEM(devObjPtr, dataEntryIndx);
    dataEntryPtr = smemMemGet(devObjPtr, regAddr);


    validBit = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, dataEntryPtr, dataEntryIndx,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);

    if(0 == validBit)
    {
        __LOG(("Got not valid entry\n"));
        return GT_FALSE;
    }

    dataEntryType = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, dataEntryPtr, dataEntryIndx,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE);

    if(dataEntryType != SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E)
    {
        __LOG(("Got not data entry type\n"));
        return GT_FALSE;
    }

    dataVrfId = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, dataEntryPtr, dataEntryIndx,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID);

    if(dataVrfId != entryInfoPtr->info.ucRouting.vrfId)
    {
        __LOG(("vrfId not match\n"));
        return GT_FALSE;
    }


    __LOG(("Got ipv6 {key,data} match\n"));
    *fdbEntryPtrPtr = dataEntryPtr;
    entryIndexPtr[1] = dataEntryIndx;

    return GT_TRUE;
}

/*******************************************************************************
*   snetChtL2iFdbLookupRefreshUcDip
*
* DESCRIPTION:
*       SIP5 : check if need to refresh the age bit in the UC entry.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       entryType   - FDB entry type
*       entryPtr    - pointer to fdb entry
*       entryIndx   - index of the fdb entry (for LOG purposes)
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetChtL2iFdbLookupRefreshUcDip
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_ENT   entryType,
    IN GT_U32                * entryPtr,
    IN GT_U32                  entryIndx
)
{
    DECLARE_FUNC_NAME(snetChtL2iFdbLookupRefreshUcDip);

    GT_U32  routeUcRefreshEn;

    if(entryType == SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E)
    {
        /* should not get here */
        return;
    }

    /* check if need to 'refresh' the age bit (set it to 1) */
    /* for ipv6 entry : this need to be done on the 'ipv6 key' entry and not on the data entry */
    smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr), 20, 1, &routeUcRefreshEn);

    __LOG_PARAM(routeUcRefreshEn);

    if(routeUcRefreshEn == 0)
    {
        return;
    }

    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr, entryPtr,
        entryIndx ,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
        1);
}


/*******************************************************************************
*   snetChtL2iFdbLookup
*
* DESCRIPTION:
*       Perform FDB Lookup
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryInfoPtr    - (pointer to) entry hash info
*       cpuMessageProcessing - indication the processing is from the CPU (AU message from CPU)
*       needEmptyIndex  - indication that need to calculate free index in case
*                         of entry 'not found'
*       numValidPlacesPtr - when NULL indicate to not be used .
*                               and the caller need single FDB hash valid(not more)
*                           when not NULL the caller need 2 FDB hash values
*                           valid only when <multiHashEnable> = 1 and needEmptyIndex == GT_TRUE
*
* OUTPUT:
*       fdbEntryPtrPtr  - Pointer to FDB entry pointer
*       entryIndexPtr   - FDB entry index
*       entryOffsetPtr  - MAC address offset
*       sendFailedNa2CpuPtr - send NA 2 CPU if hash chain over (used only for SA learning)
*       numValidPlacesPtr  - (pointer to) indication of how many indexes valid in
*                           entryIndexPtr[]
*                            the value it 0 or 1 or 2
*
* RETURN:
*       GT_TRUE         - FDB entry found
*       GT_FALSE        - FDB entry not found
*
*******************************************************************************/
static GT_BOOL snetChtL2iFdbLookup
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    IN GT_BOOL  cpuMessageProcessing,
    IN GT_BOOL  needEmptyIndex,
    OUT GT_U32 ** fdbEntryPtrPtr,
    OUT GT_U32 *  entryIndexPtr,
    OUT GT_U32 *  entryOffsetPtr,
    OUT GT_BOOL * sendFailedNa2CpuPtr,
    INOUT GT_U32   *numValidPlacesPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iFdbLookup);

    GT_U32 regAddr;                 /* Register address */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 macGlobCfgRegData;       /* Global register configuration data */
    GT_U32 macGlob1CfgRegData;      /* Global 1 register configuration data */
    GT_U32 hashType;                /* CRC hash and XOR based FDB hash function*/
    GT_U32 vlanMode;                /* VLANLookup Mode */
    GT_U32 bucketSize;              /* Number of entries in to be lookup */
    GT_U32 hashIndex;               /* Hush value */
    GT_U32 entryIndx;
    GT_U32 entry_key[SMEM_CHT_MAC_TABLE_WORDS];
                                    /* array of words to compare with entry -
                                       build according to (macAddr, fid) or
                                       (fid,sip,dip) */
    GT_U32 entry_mask[SMEM_CHT_MAC_TABLE_WORDS];
                                    /* array of words to apply on entry words
                                       and entry_key */
    GT_U32 *entryPtr;               /* pointer to the FDB entry */
    GT_U32 i;                       /* bucket index */
    GT_U32  jj;     /*iterator*/

    GT_U32 tblSize = devObjPtr->fdbNumEntries;
    GT_U32 numBitsToUse = SMEM_CHT_FDB_HASH_NUM_BITS(tblSize);
    GT_U32  multiHashEnable;/* SIP5: is the multi hash enabled */
    GT_U32 hashFunctionIndex; /* the hash function index 0..15*/
    GT_U32 hashFunctionIndex_max; /* the maximum hash functions that the device use*/
    GT_U32 stepBetweenEntries;/* steps between lookup entries */
    GT_U32 fdbBanksIsFreeArr[FDB_MAX_BANKS_CNS];/* indication per bank that there is 'free' index for learn */
    GT_U32 fdbBanksIndexArr[FDB_MAX_BANKS_CNS];/* the index per bank of the 'free' index for learn */
    GT_U32 bestBankIndex;/*the index of the best FDB bank */
    GT_U32 secondBestBankIndex;/*the index of the 'second' best FDB bank */
    GT_BIT didAnyHit = 0;/* indication for any hit */
    GT_U32 firstHitBankIndex = 0;/* index of the bank that did first hit */
    GT_U32 calculatedHashArr[FDB_MAX_BANKS_CNS];
    GT_U32  tmpHashXor,tmpHashCrc;
    GT_BIT wordAnyHit = 0;/* indication for any word hit */
    GT_U32 isDipLookup = 0;


    /* Initialize values by zeroes */
    if(fdbEntryPtrPtr)
    {
        *fdbEntryPtrPtr = 0;
    }
    *entryIndexPtr = SMAIN_NOT_VALID_CNS;/* indication that no empty place found */
    *entryOffsetPtr = 0;

    /* Get data from MAC Table Global Configuration Register */
    smemRegGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr), &macGlobCfgRegData);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr), &macGlob1CfgRegData);
        hashType = SMEM_U32_GET_FIELD(macGlob1CfgRegData, 2, 1);
    }
    else
    {
        hashType = SMEM_U32_GET_FIELD(macGlobCfgRegData, 21, 1);
    }

    vlanMode  = SMEM_U32_GET_FIELD(macGlobCfgRegData, 3, 1);

    __LOG(("hashType[%d],vlanMode[%d] \n",
                  hashType,vlanMode));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* get the <multi hash enable> bit*/
        switch(entryInfoPtr->entryType)
        {
            case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
            case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E :
            case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:
                isDipLookup = 1;
                break;
            default:
                isDipLookup = 0;
                break;
        }
        /* get the <multi hash enable> bit*/
        if(isDipLookup && (devObjPtr->multiHashEnable == 0))
        {
            __LOG(("WARNING : Doing DIP lookup , but the <multi hash enable> is 'disabled' \n"));
            __LOG(("WARNING : the DIP lookup is using 'multi hash' regardless to <multi hash enable> \n"));
        }

        multiHashEnable = isDipLookup ? 1 : devObjPtr->multiHashEnable;

    }
    else
    {
        multiHashEnable = 0;
    }

    if(multiHashEnable)
    {
        bucketSize =  SMEM_U32_GET_FIELD(macGlobCfgRegData, 0, 3) ? 2 : 1;

        hashFunctionIndex = 0;
        hashFunctionIndex_max = devObjPtr->fdbNumOfBanks;
        hashIndex = 0;
        stepBetweenEntries = devObjPtr->fdbNumOfBanks;

        __LOG(("<multi hash enable> , bucketSize[%d] , hashFunctionIndex_max[%d] \n",
                      bucketSize,hashFunctionIndex_max));

        sip5MacHashCalcMultiHash(hashType, vlanMode, entryInfoPtr,numBitsToUse,
                    devObjPtr->fdbNumOfBanks,calculatedHashArr);
    }
    else
    {
        bucketSize = 4 + (4 * SMEM_U32_GET_FIELD(macGlobCfgRegData, 0, 3));
        hashFunctionIndex_max = 1;
        stepBetweenEntries = 1;

        __LOG(("(single hash) bucketSize[%d] \n",
                      bucketSize));

        /* Get hash index to start search */
        hashIndex = cheetahMacHashCalcByStc(hashType, vlanMode, entryInfoPtr,numBitsToUse);
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && cpuMessageProcessing == GT_TRUE)
    {
        /* update the hash result registers. */
        /* NOTE: update the registers but give valid values only to those calculations
                that actually already calculated

           Assuming that when working in : XOR --> CPU not care about CRC,CRC_multi_hash
           Assuming that when working in : CRC --> CPU not care about XOR,CRC_multi_hash
           Assuming that when working in : CRC_multi_hash --> CPU not care about XOR,CRC
        */

        if(multiHashEnable == 0)/* legacy mode */
        {
            if(hashType == 1)
            {
                tmpHashXor = DUMMY_HASH_VALUE_CNS;
                tmpHashCrc = hashIndex / 4;/* CRC result - convert from FDB index to hash result */
            }
            else
            {
                tmpHashCrc = DUMMY_HASH_VALUE_CNS;
                tmpHashXor = hashIndex / 4;/* XOR result - convert from FDB index to hash result */
            }
        }
        else /* multi hash mode */
        {
            tmpHashXor = DUMMY_HASH_VALUE_CNS;
            tmpHashCrc = DUMMY_HASH_VALUE_CNS;
        }

        /* write the results to the registers */
        sfdbChtHashResultsRegistersUpdate(devObjPtr,
            multiHashEnable ? calculatedHashArr : NULL,/* multi hash results */
            tmpHashXor,
            tmpHashCrc);
    }

    /* Calculate entry_key and entry_mask to compare with FDB entry */
    snetChetL2iCalcFdbKeyAndMask(devObjPtr,vlanMode, entryInfoPtr,
                                    entry_key, entry_mask);

    for(hashFunctionIndex = 0 ;
        hashFunctionIndex < hashFunctionIndex_max ;
        hashFunctionIndex ++)
    {
        if(multiHashEnable)
        {
            hashIndex = calculatedHashArr[hashFunctionIndex];
            /* convert the hash index into FDB index */
            hashIndex *= (devObjPtr->fdbNumOfBanks);
            hashIndex += hashFunctionIndex;

            fdbBanksIndexArr[hashFunctionIndex] = hashIndex;

            __LOG(("FDB hash [0x%8.8x] for hashFunctionIndex[%d] \n",hashIndex,hashFunctionIndex));
        }

        fdbBanksIsFreeArr[hashFunctionIndex] = 0;
        /* Read bucket */
        for (i = 0; i < bucketSize; i++)
        {
            entryIndx = (hashIndex + (stepBetweenEntries * i)) % tblSize;

            /* Get entryPtr according to entryIndx */
            __LOG(("Get entryPtr according to entryIndx[0x%8.8x]",entryIndx));

            regAddr = SMEM_CHT_MAC_TBL_MEM(devObjPtr, entryIndx);

            entryPtr = smemMemGet(devObjPtr, regAddr);

            /* Valid bit */
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                fldValue = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, entryPtr ,entryIndx ,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);
            }
            else
            {
                fldValue = SMEM_U32_GET_FIELD(entryPtr[0], 0, 1);
            }

            if (fldValue == 0)
            {
                *entryIndexPtr = entryIndx;
                if(fdbEntryPtrPtr)
                {
                    *fdbEntryPtrPtr = entryPtr;
                }

                fdbBanksIsFreeArr[hashFunctionIndex] = 1;

                /* first not valid */
                if(sendFailedNa2CpuPtr)
                {
                    *sendFailedNa2CpuPtr = GT_FALSE;
                }

                if(didAnyHit == 0)
                {
                    didAnyHit = 1;
                    firstHitBankIndex = hashFunctionIndex;
                }

                if(multiHashEnable)
                {
                    /* we break this 'chain' but continue to next bank */
                    break;
                }
                else
                {
                    return GT_FALSE;/*entry not found , but got index for learn */
                }
            }
            else
            {
                __LOG(("Got valid entry with index [0x%8.8x]\n",entryIndx));

            }

            if(multiHashEnable == 0)
            {
                /* the skip bit is ignored when multiHashEnable = 1 */

                /*  Skip bit */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    fldValue =
                        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, entryPtr ,
                            entryIndx ,
                            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP);
                }
                else
                {
                    fldValue = SMEM_U32_GET_FIELD(entryPtr[0], 1, 1);
                }

                if (fldValue)
                {
                    if((*entryIndexPtr)  == SMAIN_NOT_VALID_CNS)
                    {
                        /* update only for the first skip */
                        *entryIndexPtr = entryIndx;
                    }
                    continue;
                }
            }

            /* For all words of entry compare it */
            for(jj = 0 ; jj < SMEM_CHT_MAC_TABLE_WORDS ; jj++)
            {
                __LOG(("Check match in word %d", jj));
                if((entryPtr[jj] & entry_mask[jj]) != (entry_key[jj] & entry_mask[jj]))
                {
                    if(wordAnyHit)
                    {
                        __LOG(("No match in word %d", jj));
                        __LOG_PARAM(entryPtr[jj]);
                        __LOG_PARAM(entry_key[jj]);
                        __LOG_PARAM(entry_mask[jj]);

                        __LOG_PARAM(entryPtr[jj] & entry_mask[jj]);
                        __LOG_PARAM(entry_key[jj] & entry_mask[jj]);
                    }

                    break;
                }
                else
                {
                    wordAnyHit = 1;

                    __LOG(("Got match in word %d", jj));
                    if(0 != entry_mask[jj])
                    {
                        __LOG_PARAM(entryPtr[jj]);
                        __LOG_PARAM(entry_key[jj]);
                        __LOG_PARAM(entry_mask[jj]);

                        __LOG_PARAM(entryPtr[jj] & entry_mask[jj]);
                        __LOG_PARAM(entry_key[jj] & entry_mask[jj]);
                    }
                }
            }

            if(jj == SMEM_CHT_MAC_TABLE_WORDS)
            {
                /* Entry found */
                if(fdbEntryPtrPtr)
                {
                    *fdbEntryPtrPtr = entryPtr;
                }
                *entryIndexPtr = entryIndx;

                /* when multiHashEnable == 0 -->
                    (*entryOffsetPtr) = (1*i) + 0;
                    meaning:
                    *entryOffsetPtr = i; --> previous code that was here.
                */
                (*entryOffsetPtr) = (stepBetweenEntries * i) + hashFunctionIndex;

                if(numValidPlacesPtr && multiHashEnable)
                {
                    /* we need more then this single entry */
                    if(didAnyHit == 0)
                    {
                        didAnyHit = 1;
                        firstHitBankIndex = hashFunctionIndex;
                    }
                    /*continue to next bank*/
                    break;
                }

                if(entryInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E)
                {
                    __LOG(("got ipv6 'key' entry match, check associated ipv6 data entry for vrfId match \n"));
                    if(GT_TRUE == checkFdbIpv6DataMatch(devObjPtr, entryInfoPtr, fdbEntryPtrPtr, entryIndexPtr, calculatedHashArr))
                    {
                        __LOG(("ipv6: got vrfId match\n"));
                        snetChtL2iFdbLookupRefreshUcDip(devObjPtr,
                                                        entryInfoPtr->entryType,/*'ipv6 key'*/
                                                        entryPtr,               /*'ipv6 key'*/
                                                        entryIndx);             /*'ipv6 key'*/
                        return GT_TRUE;
                    }
                    else
                    {
                        __LOG(("Got miss-match in vrfId or ipv6 data entry not valid \n"));
                        /*continue to next bank*/
                        break;
                    }
                }
                else
                if (isDipLookup)
                {
                    /*ipv4/fcoe*/
                    snetChtL2iFdbLookupRefreshUcDip(devObjPtr,
                                                    entryInfoPtr->entryType,
                                                    entryPtr,
                                                    entryIndx);

                    __LOG(("Got match UC ipv4/fcoe \n"));
                    return GT_TRUE;
                }
                else
                {
                    __LOG(("Got match (not uc routing)\n"));
                    return GT_TRUE;
                }
            }
        }
    }

    if(didAnyHit && multiHashEnable && (needEmptyIndex == GT_TRUE))
    {
        bestBankIndex = firstHitBankIndex;
        secondBestBankIndex = SMAIN_NOT_VALID_CNS;

        SCIB_SEM_TAKE;
        while(devObjPtr->fdbBankCounterInCpuPossess)
        {
            SCIB_SEM_SIGNAL;
            /* wait for CPU to release the counters */
            SIM_OS_MAC(simOsSleep)(50);
            SCIB_SEM_TAKE;
            if(devObjPtr->needToDoSoftReset)
            {
                break;
            }
        }

        devObjPtr->fdbBankCounterUsed = 1;
        SCIB_SEM_SIGNAL;

        for(hashFunctionIndex = firstHitBankIndex + 1;
            hashFunctionIndex < hashFunctionIndex_max ;
            hashFunctionIndex ++)
        {
            if(fdbBanksIsFreeArr[hashFunctionIndex])
            {
                if((*devObjPtr->fdbBanksCounterArr[hashFunctionIndex]) >
                   (*devObjPtr->fdbBanksCounterArr[bestBankIndex]))
                {
                    /*save the 'second' best too */
                    secondBestBankIndex = bestBankIndex;
                    /* When multi hash mode is enabled Learning is performed in the most populated bank */
                    bestBankIndex = hashFunctionIndex;
                }
                else if (secondBestBankIndex == SMAIN_NOT_VALID_CNS)
                {
                    /* this is new second best */
                    secondBestBankIndex = hashFunctionIndex;
                }
                else if ((*devObjPtr->fdbBanksCounterArr[hashFunctionIndex]) >
                         (*devObjPtr->fdbBanksCounterArr[secondBestBankIndex]))
                {
                    /* this is new second best */
                    secondBestBankIndex = hashFunctionIndex;
                }
            }
        }

        *entryIndexPtr = fdbBanksIndexArr[bestBankIndex];

        if(numValidPlacesPtr)
        {
            if(secondBestBankIndex == SMAIN_NOT_VALID_CNS)
            {
                *numValidPlacesPtr = 1;
            }
            else
            {
                *numValidPlacesPtr = 2;
                entryIndexPtr[1] = fdbBanksIndexArr[secondBestBankIndex];
            }
        }

        devObjPtr->fdbBankCounterUsed = 0;

        /* Get entryPtr according to entryIndx */
        __LOG(("entry index [0x%8.8x] taken from bank[%d]",
                      fdbBanksIndexArr[bestBankIndex], bestBankIndex));

        if(sendFailedNa2CpuPtr)
        {
            *sendFailedNa2CpuPtr = GT_FALSE;
        }

        return GT_FALSE;/*entry not found , but got index for learn */
    }

    if(numValidPlacesPtr)
    {
        *numValidPlacesPtr = 0;
    }


    if(sendFailedNa2CpuPtr)
    {
        /* Device cannot learn this MAC. Sending NA messages to the CPU */
        fldValue = SMEM_U32_GET_FIELD(macGlobCfgRegData, 4, 1);
        *sendFailedNa2CpuPtr = (fldValue) ? GT_TRUE :  GT_FALSE;

        __LOG(("Sending NA messages to the CPU is enabled[%d] \n",
                      fldValue));

    }

    return GT_FALSE;     /*entry not found , and got NO index for learn */
}

/*******************************************************************************
*   snetChtL2iFdbLookupFor2Places
*
* DESCRIPTION:
*       find in the FDB 2 places for the needed entry info
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryInfoPtr    - (pointer to) entry hash info
* OUTPUT:
*       numValidPlacesPtr  - (pointer to) indication of how many indexes valid in
*                           entryIndexPtr[]
*                            the value it 0 or 1 or 2
*       entryIndexPtr[2]   - FDB entry index 0,1
*
* RETURN:
*       None
*
*******************************************************************************/
void snetChtL2iFdbLookupFor2Places
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    IN GT_U32   *numValidPlacesPtr,
    OUT GT_U32   entryIndexPtr[2]
)
{
    GT_U32  entryOffset; /* DUMMY not used */

    snetChtL2iFdbLookup(
            devObjPtr,
            entryInfoPtr,
            GT_TRUE,  /*cpuMessageProcessing,*/
            GT_TRUE,  /*needEmptyIndex*/
            NULL,     /*fdbEntryPtrPtr*/
            entryIndexPtr, /*entryIndexPtr*/
            &entryOffset, /*entryOffsetPtr*/
            NULL,          /*sendFailedNa2CpuPtr*/
            numValidPlacesPtr
    );

    return;
}

/*******************************************************************************
*   lion3L2iCalcFdbKeyAndMaskForField
*
* DESCRIPTION:
*       SIP5 : Fill field of MAC entry and mask for FDB Lookup
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       enrtyKeyPtr     - MAC entry search key
*       entryMaskPtr    - MAC entry search mask
*       fieldName       - field offset
*       valueArr[]      - value array to set to field
*
* OUTPUT:
*       enrtyKeyPtr     - (updated) MAC entry search key
*       entryMaskPtr    - (updated)MAC entry search mask
*
*******************************************************************************/
static GT_VOID lion3L2iCalcFdbKeyAndMaskForField
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT   GT_U32 * enrtyKeyPtr,
    INOUT   GT_U32 * entryMaskPtr,
    IN  GT_U32         fieldName,
    IN  GT_U32         valueArr[4]
)
{
    static GT_U32  maskArr[4] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
    static GT_U32  dummyFdbIndex = 0xFFFFFFFF;

    if(fieldName == SMEM_LION3_FDB_FDB_TABLE_FIELDS_MAC_ADDR)
    { /* MAC ADDR : 48 bits */

        /* value */
        /* set the MAC ADDR words */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_MAC_ADDR_SET(devObjPtr,
            enrtyKeyPtr,
            dummyFdbIndex,
            valueArr);

        /* mask */
        /* set the MAC ADDR words */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_MAC_ADDR_SET(devObjPtr,
            entryMaskPtr,
            dummyFdbIndex,
            maskArr);
    }
    else if(fieldName == SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DIP)
    { /* ipv6 dip: 128 bits */

        /* value */
        /* set the IPV6 DIP words */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_IPV6_DIP_SET(devObjPtr,
            enrtyKeyPtr,
            dummyFdbIndex,
            valueArr);

        /* mask */
        /* set the IPV6 DIP words */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_IPV6_DIP_SET(devObjPtr,
            entryMaskPtr,
            dummyFdbIndex,
            maskArr);
    }
    else
    {
        /* value */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr, enrtyKeyPtr ,dummyFdbIndex,
            fieldName, valueArr[0]);

        /* mask */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr, entryMaskPtr ,dummyFdbIndex,
            fieldName, maskArr[0]);
    }


}

/*******************************************************************************
*   snetChetL2iCalcFdbKeyAndMask
*
* DESCRIPTION:
*       Fill MAC entry and mask for FDB Lookup
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       vlanMode        - vlan mode.
*       fdbEntryType    - FDB entry type
*       macAddrPtr      - MAC pointer
*       fid             - forwarding ID
*       sourceIpAddr    - Source IP pointer
*       destIpAddr      - Destination IP pointer
*
* OUTPUT:
*       enrtyKeyPtr     - MAC entry search key
*       entryMaskPtr    - MAC entry search mask
*
*******************************************************************************/
static GT_VOID snetChetL2iCalcFdbKeyAndMask
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 vlanMode,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    OUT   GT_U32 * enrtyKeyPtr,
    OUT   GT_U32 * entryMaskPtr
)
{
    DECLARE_FUNC_NAME(snetChetL2iCalcFdbKeyAndMask);

    GT_U32  mask = 0xFFFFFFFF;
    GT_U32  valueArr[4] = {0};

    memset(enrtyKeyPtr, 0, SMEM_CHT_MAC_ENTRY_BYTES);
    memset(entryMaskPtr, 0, SMEM_CHT_MAC_ENTRY_BYTES);

    if (entryInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E ||
        entryInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E     ||
        entryInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E  )
    {
        /* VRF ID : value + mask */
        valueArr[0] = entryInfoPtr->info.ucRouting.vrfId;
        lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr, entryMaskPtr,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID,
            valueArr);
    }
    else if (entryInfoPtr->entryType == SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E)
    {
        /* fid not relevant for ipv6 key: do nothing */
        __LOG(("got ipv6 key entry type \n"));
    }
    else if (vlanMode)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* VLAN : value + mask */
            valueArr[0] = entryInfoPtr->origFid;/* must use ORIGINAL regardless to fid16BitHashEn */
            lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr ,entryMaskPtr,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID,
                valueArr);
        }
        else
        {
            /* VLAN */
            SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 5, 12, entryInfoPtr->origFid);
            /* VLAN Mask */
            SMEM_U32_SET_FIELD(entryMaskPtr[0], 5, 12, mask);
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* MACEntryType : value + mask */
        valueArr[0] = entryInfoPtr->entryType;
        lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr ,entryMaskPtr,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE,
            valueArr);
    }
    else
    {
        /* MACEntryType */
        SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 3, 2, entryInfoPtr->entryType );
        /* MACEntryType Mask */
        SMEM_U32_SET_FIELD(entryMaskPtr[0], 3, 2, mask);
    }

    switch(entryInfoPtr->entryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* MACAddr : value + mask */
                valueArr[0] = GT_HW_SIM_MAC_LOW32(entryInfoPtr->info.macInfo.macAddr);
                valueArr[1] = GT_HW_SIM_MAC_HIGH16(entryInfoPtr->info.macInfo.macAddr);
                lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr ,entryMaskPtr,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_MAC_ADDR,
                    valueArr);
            }
            else
            {
                /* MACAddr[7:0] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 17, 8, entryInfoPtr->info.macInfo.macAddr[5]);
                /* MACAddr[14:8] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 25, 7, entryInfoPtr->info.macInfo.macAddr[4]);
                /* MACAddr Mask[14:0] */
                SMEM_U32_SET_FIELD(entryMaskPtr[0], 17, 15, mask);

                /* MACAddr[15] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 0, 1, entryInfoPtr->info.macInfo.macAddr[4] >> 7);
                /* MACAddr[23:16] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 1, 8, entryInfoPtr->info.macInfo.macAddr[3]);
                /* MACAddr[31:24] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 9, 8, entryInfoPtr->info.macInfo.macAddr[2]);
                /* MACAddr[39:32] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 17, 8, entryInfoPtr->info.macInfo.macAddr[1]);
                /* MACAddr[46:40] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 25, 7, entryInfoPtr->info.macInfo.macAddr[0]);

                /* MACAddr Mask[46:15] */
                entryMaskPtr[1] = mask;


                /* MACAddr [47] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[2], 0, 1, entryInfoPtr->info.macInfo.macAddr[0] >> 7);

                /* MACAddr Mask[47] */
                SMEM_U32_SET_FIELD(entryMaskPtr[2], 0, 1, mask);
            }
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* DIP : value + mask */
                valueArr[0] = entryInfoPtr->info.ipmcBridge.dip;
                lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr ,entryMaskPtr,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DIP,
                    valueArr);
                /* SIP : value + mask */
                valueArr[0] = entryInfoPtr->info.ipmcBridge.sip;
                lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr ,entryMaskPtr,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP,
                    valueArr);
            }
            else
            {
                /* DIP[14:0] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 17, 15, entryInfoPtr->info.ipmcBridge.dip );
                /* DIP Mask[14:0] */
                SMEM_U32_SET_FIELD(entryMaskPtr[0], 17, 15, mask);

                /* DIP[31:15] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 0, 17, entryInfoPtr->info.ipmcBridge.dip >> 15);
                /* DIP Mask[31:15] */
                SMEM_U32_SET_FIELD(entryMaskPtr[1], 0, 17, mask);

                /* SIP[14:0] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 17, 15, entryInfoPtr->info.ipmcBridge.sip);
                /* SIP Mask[14:0] */
                SMEM_U32_SET_FIELD(entryMaskPtr[1], 17, 15, mask);

                /* SIP[27:15] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[2], 0, 13, entryInfoPtr->info.ipmcBridge.sip >> 15);
                /* SIP Mask[14:0] */
                SMEM_U32_SET_FIELD(entryMaskPtr[2], 0, 13, mask);

                /* SIP[28] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[2], 26, 1, entryInfoPtr->info.ipmcBridge.sip >> 28);
                /* SIP Mask[28] */
                SMEM_U32_SET_FIELD(entryMaskPtr[2], 26, 1, mask);

                /* SIP[30:29] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[2], 30, 2, entryInfoPtr->info.ipmcBridge.sip >> 29);
                /* SIP Mask[30:29] */
                SMEM_U32_SET_FIELD(entryMaskPtr[2], 30, 2, mask);

                /* SIP[31] */
                SMEM_U32_SET_FIELD(enrtyKeyPtr[3], 0, 1, entryInfoPtr->info.ipmcBridge.sip >> 31);
                /* SIP Mask[31] */
                SMEM_U32_SET_FIELD(entryMaskPtr[3], 0, 1, mask);
            }
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:
            /* IPV6 DIP : value + mask */
            valueArr[0] = entryInfoPtr->info.ucRouting.dip[3];
            valueArr[1] = entryInfoPtr->info.ucRouting.dip[2];
            valueArr[2] = entryInfoPtr->info.ucRouting.dip[1];
            valueArr[3] = entryInfoPtr->info.ucRouting.dip[0];

            lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr, entryMaskPtr,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DIP,
                valueArr);

            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:
        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
            /* IPV4 DIP/ FCOE D_ID : value + mask */
            valueArr[0] = entryInfoPtr->info.ucRouting.dip[0];

            lion3L2iCalcFdbKeyAndMaskForField(devObjPtr, enrtyKeyPtr, entryMaskPtr,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP,
                valueArr);
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E:
            /* do nothing, vrf id aleady in the key/mask */
            break;

        default:
            __LOG(("ERROR: wrong entry type given, this should never happen \n"));
            return ;
    }
}

/*******************************************************************************
*   lion3CreateAuMsg
*
* DESCRIPTION:
*        Lion3 : Create MAC update message
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        descrPtr           -  descriptor
*        macUpdMsgPtr       -  FIFO entry pointer.
*       learnInfoPtr  - (pointer to) FDB extra info
*       newAddressMessageEn  - new Address Message Enable
*
*******************************************************************************/
static GT_VOID lion3CreateAuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 * macUpdMsgPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr,
    IN GT_U32 newAddressMessageEn
)
{
    DECLARE_FUNC_NAME(lion3CreateAuMsg);

    GT_U32               macAddrWords[2];
    SNET_CHEETAH_FDB_ENTRY_ENT                macEntryType;
    SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC specialFields;

    memset(&specialFields,0,sizeof(SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC));

    if (newAddressMessageEn)
    {
        __LOG(("New address message used \n"));
    }
    else
    {
        __LOG(("Old address message used \n"));
    }

    macEntryType = SNET_CHEETAH_FDB_ENTRY_MAC_E;

    /* MessageID Always set to 0x0  */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MESSAGE_ID,
        0);

    /* MsgType */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE,
        SFDB_UPDATE_MSG_NA_E);

    /* entryType */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE,
        macEntryType);

    /* MacAddr */
    macAddrWords[0] = GT_HW_SIM_MAC_LOW32(descrPtr->macSaPtr);
    macAddrWords[1] = GT_HW_SIM_MAC_HIGH16(descrPtr->macSaPtr);

    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_MAC_ADDR_SET(devObjPtr,macUpdMsgPtr,
        macAddrWords);

    /* FID */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID,
        learnInfoPtr->FDB_fid);

    /* Age */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE,
        1);
    /* isTrunk */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK,
        learnInfoPtr->FDB_isTrunk);
    /* Port/Trunk */
    if(learnInfoPtr->FDB_isTrunk)
    {
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_TRUNK_NUM,
            learnInfoPtr->FDB_port_trunk);
    }
    else
    {
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_EPORT_NUM,
            learnInfoPtr->FDB_port_trunk);
    }

    specialFields.srcId = descrPtr->sstId;
    specialFields.udb = learnInfoPtr->FDB_UDB;

    /* the FDB_origVid1 is set only for auto-learning.
       It's 0 for control learning */
    specialFields.origVid1 = learnInfoPtr->FDB_origVid1;

    if (newAddressMessageEn)
    {
        /* in this format those fields not exists */
        specialFields.daAccessLevel = SMAIN_NOT_VALID_CNS;
        specialFields.saAccessLevel = SMAIN_NOT_VALID_CNS;
    }
    else
    {
        specialFields.daAccessLevel = descrPtr->daAccessLevel;
        specialFields.saAccessLevel = descrPtr->saAccessLevel;
    }

    sfdbLion3FdbAuMsgSpecialMuxedFieldsSet(devObjPtr,macUpdMsgPtr,macEntryType,
        &specialFields);

    /* DevNum */
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DEV_ID,
        learnInfoPtr->FDB_devNum);

    /* New address message enable */
    if (newAddressMessageEn)
    {
        /* UP0 */
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_UP0,
            descrPtr->up);

        /* IsMoved */
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_IS_MOVED,
            learnInfoPtr->FDB_isMoved);

        if (learnInfoPtr->FDB_isMoved)
        {
            /*   OLD_IS_TRUNK */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_IS_TRUNK,
                learnInfoPtr->oldInfo.FDB_isTrunk);

            if(learnInfoPtr->oldInfo.FDB_isTrunk)
            {
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_TRUNK_NUM,
                    learnInfoPtr->oldInfo.FDB_port_trunk);
            }
            else
            {
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_EPORT,
                    learnInfoPtr->oldInfo.FDB_port_trunk);
            }

            /* OldSrcID */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_SRC_ID,
                learnInfoPtr->oldInfo.FDB_srcId);

            /* OldDevice */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVED_OLD_DEVICE,
                learnInfoPtr->oldInfo.FDB_devNum);
        }
    }
}



/*******************************************************************************
*   snetChtCreateAuMsg
*
* DESCRIPTION:
*        Create MAC update message
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        descrPtr           -  descriptor
*        macUpdMsgPtr       -  FIFO entry pointer.
*       learnInfoPtr  - (pointer to) FDB extra info
*
*******************************************************************************/
static GT_VOID snetChtCreateAuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT GT_U32 * macUpdMsgPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtCreateAuMsg);

    GT_U32 newAddressMessageEn = 0;/*new Address Message Enable */
    GT_U32 vid1;/* vid1 field in the AU message */

    if (devObjPtr->supportFdbNewNaToCpuMsgFormat)
    {
        smemRegFldGet(devObjPtr,
                      SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr), 28, 1, &newAddressMessageEn);
    }

    /* reset the FIFO buffer */
    memset(macUpdMsgPtr,0,4*devObjPtr->numWordsAlignmentInAuMsg);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        lion3CreateAuMsg(devObjPtr,descrPtr,macUpdMsgPtr,learnInfoPtr,newAddressMessageEn);
        return;
    }

    /* AU MessageID Always set to 0x2 */
    if (SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {   /* 1:0 MessageID Always set to 0x2 , on cht */
        snetFieldValueSet(macUpdMsgPtr, 0, 4, 2);
    }
    else
    {   /* 0:0 MessageID Always set to 0x0 , on cht2 and above */
        snetFieldValueSet(macUpdMsgPtr, 0, 1, 0);
        /* 2:1 DA security access level */
        snetFieldValueSet(macUpdMsgPtr, 1, 2, descrPtr->daAccessLevel);
        /* 14:12 SA security group level */
        snetFieldValueSet(macUpdMsgPtr, 12, 3, descrPtr->saAccessLevel);

    }

    /* MsgType -- bits [4,5] -- NA (0x0) */

    /* 31:16 MacAddr[15:0] */
    snetFieldValueSet(macUpdMsgPtr, 16, 8, descrPtr->macSaPtr[5]);
    snetFieldValueSet(macUpdMsgPtr, 24, 8, descrPtr->macSaPtr[4]);

    /* 63:32 MacAddr[47:16] */
    snetFieldValueSet(macUpdMsgPtr, 32, 8, descrPtr->macSaPtr[3]);
    snetFieldValueSet(macUpdMsgPtr, 40, 8, descrPtr->macSaPtr[2]);
    snetFieldValueSet(macUpdMsgPtr, 48, 8, descrPtr->macSaPtr[1]);
    snetFieldValueSet(macUpdMsgPtr, 56, 8, descrPtr->macSaPtr[0]);

    /* 75:64 VID */
    snetFieldValueSet(macUpdMsgPtr, 64, 12, learnInfoPtr->FDB_fid);
    /* 77:77  Age */
    snetFieldValueSet(macUpdMsgPtr, 77, 1, 1);
    /* 81:81 Origin Port/Trunk */
    snetFieldValueSet(macUpdMsgPtr, 81, 1, learnInfoPtr->FDB_isTrunk);
    /* 88:82 Port/Trunk */
    snetFieldValueSet(macUpdMsgPtr, 82, 7, learnInfoPtr->FDB_port_trunk);
    /* 102:98 SrcID */
    snetFieldValueSet(macUpdMsgPtr, 98, 5, descrPtr->sstId);
    /* 107:103 DevNum */
    snetFieldValueSet(macUpdMsgPtr, 103, 5, learnInfoPtr->FDB_devNum);

    /* New address message enable */
    if (newAddressMessageEn)
    {
        __LOG(("New address message enabled \n"));

        vid1 = descrPtr->vid0Or1AfterTti; /* this is <Orig Tag VID> that application
            should have set configuration top hold 'tag1' and not 'tag0' .
            see the configuration in :
            -- Orig VID Mode --  (xcat2 and above)
            see bit 2 in SMEM_XCAT_A1_TTI_UNIT_GLB_CONF_REG(..)
            */

        /* VID1[1:0] */
        snetFieldValueSet(macUpdMsgPtr, 1, 2,
                          SMEM_U32_GET_FIELD(vid1, 0, 2));
        /* VID1[4:2] */
        snetFieldValueSet(macUpdMsgPtr, 12, 3,
                          SMEM_U32_GET_FIELD(vid1, 2, 3));
        /* 126:123  VID1[8:5] */
        snetFieldValueSet(macUpdMsgPtr, 123, 4,
                          SMEM_U32_GET_FIELD(vid1, 5, 4));
        /* 97:96    VID1[10:9] */
        snetFieldValueSet(macUpdMsgPtr, 96, 2,
                          SMEM_U32_GET_FIELD(vid1, 9, 2));
        /* 15:15    VID1[11] */
        snetFieldValueSet(macUpdMsgPtr, 15, 1,
                          SMEM_U32_GET_FIELD(vid1, 11, 1));
        /* 110:108  UP0[2:0] */
        snetFieldValueSet(macUpdMsgPtr, 108, 3, descrPtr->up);

        /* 80:80   IsMoved */
        snetFieldValueSet(macUpdMsgPtr, 80, 1, learnInfoPtr->FDB_isMoved);

        if (learnInfoPtr->FDB_isMoved)
        {
            /* 3:3  OldPortOldTrunk[0] */
            snetFieldValueSet(macUpdMsgPtr, 3, 1,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_port_trunk, 0, 1));
            /* 79:79   OldPortOldTrunk[1] */
            snetFieldValueSet(macUpdMsgPtr, 79, 1,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_port_trunk, 1, 1));
            /* 77:77   OldPortOldTrunk[2] */
            snetFieldValueSet(macUpdMsgPtr, 77, 1,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_port_trunk, 2, 1));
            /* 76:76   OldPortOldTrunk[3] */
            snetFieldValueSet(macUpdMsgPtr, 76, 1,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_port_trunk, 3, 1));
            /* 122:120  OldPortOldTrunk[6:4] */
            snetFieldValueSet(macUpdMsgPtr, 120, 3,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_port_trunk, 4, 3));
            /* 127:127  OldSrcID[0] */
            snetFieldValueSet(macUpdMsgPtr, 127, 1,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_srcId, 0, 1));
            /* 119:117  OldSrcID[3:1] */
            snetFieldValueSet(macUpdMsgPtr, 117, 3,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_srcId, 1, 3));
            /* 94:94    OldSrcID[4] */
            snetFieldValueSet(macUpdMsgPtr, 94, 1,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_srcId, 4, 1));
            /* 116:115  OldDevice[1:0] */
            snetFieldValueSet(macUpdMsgPtr, 115, 2,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_devNum, 0, 2));
            /* 113:111  OldDevice[4:2] */
            snetFieldValueSet(macUpdMsgPtr, 111, 3,
                              SMEM_U32_GET_FIELD(learnInfoPtr->oldInfo.FDB_devNum, 2, 3));
            /* 114:114  OldIsTrunk */
            snetFieldValueSet(macUpdMsgPtr, 114, 1, learnInfoPtr->oldInfo.FDB_isTrunk);
        }
    }

}

/*******************************************************************************
*   snetChtL2iPciAuMsg
*
* DESCRIPTION:
*        PCI Master write to host memory Address Update Queue (AUQ)
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        macUpdMsgPtr       -  pointer to MAC Update Message
*
*******************************************************************************/
static GT_BOOL snetChtL2iPciAuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U32 * macUpdMsgPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iPciAuMsg);

    GT_U32 auqSize;                         /* address update queue in bytes */
    GT_U32 * auqMemPtr;                     /* pointer to AUQ queue */
    GT_BOOL status = GT_FALSE;              /* function return value */
    CHT_AUQ_MEM  *  simAuqMem;              /* pointer to AU queue */

    simAuqMem = SMEM_CHT_MAC_AUQ_MEM_GET(devObjPtr);

    /* Check validity of AUQ base segment */
    if (simAuqMem->auqBaseValid == GT_FALSE)
    {
        __LOG(("simAuqMem->auqBaseValid == GT_FALSE : Notify, device can't send this message \n"));

        /* Notify, device can't send this message */
        return GT_FALSE;
    }

    /* AUQSize */
    auqSize = simAuqMem->auqBaseSize;
    if (simAuqMem->auqOffset < auqSize)
    {
        auqMemPtr = (GT_U32*)((GT_UINTPTR)simAuqMem->auqBase +
                    (simAuqMem->auqOffset * (4*devObjPtr->numWordsAlignmentInAuMsg)));

        /*memcpy(auqMemPtr, macUpdMsgPtr, SMEM_CHT_AU_MSG_BYTES);*/
        /* write data into the DMA */
        __LOG(("write data into the DMA \n"));
        snetChtPerformScibDmaWrite(SNET_CHT_DMA_CLIENT_AUQ_E,
            devObjPtr->deviceId,(GT_U32)((GT_UINTPTR)auqMemPtr),devObjPtr->numWordsAlignmentInAuMsg, macUpdMsgPtr,SCIB_DMA_WORDS);

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr)||
           SKERNEL_XCAT3_FAMILY == devObjPtr->deviceFamily)
        {
            GT_U32  AU_Addr_msg_size;
            GT_U32  numWordsData = devObjPtr->numOfWordsInAuMsg;

            smemRegFldGet(devObjPtr, SMEM_LION3_MG_AUQ_GENERAL_CONFIG_REG(devObjPtr), 17, 1, &AU_Addr_msg_size);

            if((numWordsData == 4)/* 2*64 bits */ && AU_Addr_msg_size == 1 /*size_of_3 (3*64 bits) */ )
            {
                /* it is wrong configuration that WILL cause DMA overflow */
                skernelFatalError("snetChtL2iPciAuMsg: the AUQ Message that comes is size_of_2 (2*64 bits) but <AU_Addr_msg_size>  set to size_of_3 (3*64 bits)  \n");
            }
            else if((numWordsData == 6)/* 3*64 bits */ && AU_Addr_msg_size == 0 /*size_of_2 (2*64 bits) */ )
            {
                /* it is wrong configuration that may cause less messages into the FUQ then 'expected' */
                skernelFatalError("snetChtL2iPciAuMsg: the AUQ Message Message that comes is size_of_3 (3*64 bits) but <AU_Addr_msg_size>  set to size_of_2 (2*64 bits)  \n");
            }
        }

        simAuqMem->auqOffset++;

        if (simAuqMem->auqOffset == auqSize)
        {
            /* this is AUQ Full case */
            simAuqMem->auqBaseValid = GT_FALSE;

            /* Check for AUQ shadow before new AU message interrupt */
            if (simAuqMem->auqShadowValid)
            {
                /* Use AUQ shadow for the new upcoming AU messages */
                simAuqMem->auqBase = simAuqMem->auqShadow;
                simAuqMem->auqBaseSize = simAuqMem->auqShadowSize;
                simAuqMem->auqBaseValid = GT_TRUE;
                simAuqMem->auqOffset = 0;

                /* Invalidate the fact that there is another "queue", since we now use it */
                simAuqMem->auqShadowValid = GT_FALSE;
            }
            else
            {
                /* indicate that AUQ is full */
                if (SKERNEL_IS_CHEETAH3_DEV(devObjPtr))
                {
                    /* <GeneralAUQFull> */
                    smemRegFldSet(devObjPtr, SMEM_CHT_AUQ_CTRL_REG(devObjPtr), 30, 1, 1);
                }

                __LOG(("Interrupt: MISCELLANEOUS AU FULL \n"));

                /* The Address Update Queue in the CPU Memory is full */
                snetChetahDoInterrupt(devObjPtr,
                                      SMEM_CHT_MISC_INTR_CAUSE_REG(devObjPtr),
                                      SMEM_CHT_MISC_INTR_MASK_REG(devObjPtr),
                                      SMEM_CHT_MISCELLANEOUS_AU_FULL_INT,
                                      SMEM_CHT_MISCELLANEOUS_SUM_INT(devObjPtr));
            }


        }

        __LOG(("Interrupt: MISCELLANEOUS AU MSG PENDING \n"));
        /* AU message posted in the Address Update Queue in the CPU memory */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MISC_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MISC_INTR_MASK_REG(devObjPtr),
                              SMEM_CHT_MISCELLANEOUS_AU_MSG_PENDING_INT,
                              SMEM_CHT_MISCELLANEOUS_SUM_INT(devObjPtr));
        status = GT_TRUE;
    }
    else
    {
        skernelFatalError("snetChtL2iPciAuMsg: bad state\n");
    }

    return status;
}

/*******************************************************************************
*   snetCheetahFuMsgWrite
*
* DESCRIPTION:
*        Write FDB Upload Queue (FUQ) to host memory
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        fuMsgPtr           -  pointer to FDB/CNC Update Message
*
*******************************************************************************/
static GT_VOID snetCheetahFuMsgWrite
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * fuMsgPtr,
    IN GT_U32   numWordsData
)
{
    DECLARE_FUNC_NAME(snetCheetahFuMsgWrite);

    GT_UINTPTR  fuqMemPtrOffset;               /* new pointer offset to FU queue */
    CHT2_FUQ_MEM  * fuqMemPtr;              /* FDB memory upload pointer */
    GT_U32  FU_Addr_msg_size;
    GT_U32  auq_msg_size3_align;
    GT_U32  alignNumBytes;
    GT_U32  currentOffsetInBytes;

    fuqMemPtr = SMEM_CHT_MAC_FUQ_MEM_GET(devObjPtr);

    currentOffsetInBytes =  fuqMemPtr->fuqOffset;

    fuqMemPtrOffset = (GT_UINTPTR)(fuqMemPtr->fuqBase + currentOffsetInBytes);

    __LOG(("write data into the DMA \n"));

    snetChtPerformScibDmaWrite(SNET_CHT_DMA_CLIENT_FUQ_E,
        devObjPtr->deviceId,(GT_U32)((GT_UINTPTR)fuqMemPtrOffset), numWordsData, fuMsgPtr,SCIB_DMA_WORDS);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr)||
       SKERNEL_XCAT3_FAMILY == devObjPtr->deviceFamily)
    {
        smemRegFldGet(devObjPtr, SMEM_LION3_MG_AUQ_GENERAL_CONFIG_REG(devObjPtr), 18, 1, &FU_Addr_msg_size);

        if((numWordsData == 4)/* 2*64 bits */ && FU_Addr_msg_size == 1 /*size_of_3 (3*64 bits) */ )
        {
            /* it is wrong configuration that WILL cause DMA overflow */
            skernelFatalError("snetCheetahFuMsgWrite: the FU/CNC Message that comes is size_of_2 (2*64 bits) but <FU_Addr_msg_size>  set to size_of_3 (3*64 bits)  \n");
        }
        else if((numWordsData == 6)/* 3*64 bits */ && FU_Addr_msg_size == 0 /*size_of_2 (2*64 bits) */ )
        {
            /* it is wrong configuration that may cause less messages into the FUQ then 'expected' */
            skernelFatalError("snetCheetahFuMsgWrite: the FU/CNC Message that comes is size_of_3 (3*64 bits) but <FU_Addr_msg_size>  set to size_of_2 (2*64 bits)  \n");
        }


        if(FU_Addr_msg_size == 0)
        {
            /* message size type is 16 bytes and aligned on 16 bytes */
            alignNumBytes = 16;
            fuqMemPtr->fuqNumMessages++;/* another message was put into the queue */
        }
        else
        {
            smemRegFldGet(devObjPtr, SMEM_LION3_MG_AUQ_GENERAL_CONFIG_REG(devObjPtr), 16, 1, &auq_msg_size3_align);
            /* message size is 24 bytes and alignment is 24 or 32 bytes  */
            alignNumBytes = (auq_msg_size3_align == 0) ? 24 : 32;

            fuqMemPtr->fuqNumMessages++;/* another message was put into the queue */
        }

        if(numWordsData > (alignNumBytes / 4))
        {
            __LOG(("WARNING : FU message / CNC counters need [%d] words to write , but alignment is only [%d] words \n",
                numWordsData ,
                (alignNumBytes / 4)));
        }

    }
    else
    {
        alignNumBytes = 16;
        fuqMemPtr->fuqNumMessages++;/* another message was put into the queue */
    }

    fuqMemPtr->fuqOffset += alignNumBytes;

    __LOG(("next FUQ message will start [%d] bytes from start of current message that was with [%d] bytes length \n" ,
        alignNumBytes,
        (numWordsData * 4)));

    __LOG(("Interrupt: MISCELLANEOUS FU MSG PENDING \n"));

    /* FU message posted in the Address Update Queue in the CPU memory */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_MISC_INTR_CAUSE_REG(devObjPtr),
                          SMEM_CHT_MISC_INTR_MASK_REG(devObjPtr),
                          SMEM_CHT2_MISCELLANEOUS_FU_MSG_PENDING_INT,
                          SMEM_CHT_MISCELLANEOUS_SUM_INT(devObjPtr));
    return;
}

/*******************************************************************************
*   snetChtL2iPciFuMsg
*
* DESCRIPTION:
*        PCI Master write to host memory FDB Upload Queue (FUQ)
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        fuMsgSizePtr       -  pointer to FDB/CNC Update Message
*
*******************************************************************************/
GT_BOOL snetChtL2iPciFuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * fuMsgPtr,
    IN GT_U32   numWordsData
)
{
    DECLARE_FUNC_NAME(snetChtL2iPciFuMsg);

    GT_U32 fuqSize;                         /* address update queue in bytes */
    GT_U32 fldValue;                        /* Register's field value */
    GT_BOOL status = GT_TRUE;               /* function return value */
    CHT2_FUQ_MEM  * fuqMemPtr;              /* FDB memory upload pointer */

    fuqMemPtr = SMEM_CHT_MAC_FUQ_MEM_GET(devObjPtr);

    /* Check validity of FUQ base segment */
    if (fuqMemPtr->fuqBaseValid == GT_FALSE)
    {
        __LOG(("fuqMemPtr->fuqBaseValid == GT_FALSE : Notify, device can't send this message \n"));

        /* Notify, device can't send this message */
        return GT_FALSE;
    }

    smemRegFldGet(devObjPtr, SMEM_CHT_FU_QUE_BASE_ADDR_REG(devObjPtr), 0, 30, &fldValue);

    /* FU queue size */
    fuqSize = fldValue;

    if (fuqMemPtr->fuqNumMessages < fuqSize)
    {
        /* Write FU message to DMA queue */
        snetCheetahFuMsgWrite(devObjPtr, fuMsgPtr,numWordsData);
    }
    else
    {
        fuqMemPtr->fuqBaseValid = GT_FALSE;

        __LOG(("Interrupt: MISCELLANEOUS FU FULL \n"));

        /* The Address Update Queue in the CPU Memory is full */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MISC_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MISC_INTR_MASK_REG(devObjPtr),
                              SMEM_CHT2_MISCELLANEOUS_FU_FULL_INT,
                              SMEM_CHT_MISCELLANEOUS_SUM_INT(devObjPtr));

        if (fuqMemPtr->fuqShadowValid)
        {
            fuqMemPtr->fuqBase = fuqMemPtr->fuqShadow;
            fuqMemPtr->fuqOffset = 0;
            fuqMemPtr->fuqNumMessages = 0;
            fuqMemPtr->fuqBaseValid = GT_TRUE;

            /* Invalidate the fact that there is another "queue", since we now use it */
            fuqMemPtr->fuqShadowValid = GT_FALSE;

            /* Write FU message to DMA queue */
            snetCheetahFuMsgWrite(devObjPtr, fuMsgPtr,numWordsData);
        }
        else
        {
            if (SKERNEL_IS_CHEETAH3_DEV(devObjPtr))
            {
                /* Set Queue Full bit (bit 30) to one */
                smemRegFldSet(devObjPtr, SMEM_CHT_FU_QUE_BASE_ADDR_REG(devObjPtr), 30, 1, 1);
            }

            status = GT_FALSE;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtL2iAutoLearnFdbEntry
*
* DESCRIPTION:
*        Auto learn new / update existing FDB entry.
*
* INPUTS:
*        devObjPtr          - pointer to device object.
*        descrPtr           - descriptor
*        spUnknown;         - storm prevention
*        entryIndex         - fdb index entry in MAC table
*        learnInfoPtr       - (pointer to) FDB extra info
*        entryAlreadyExists - the entry already exists and we only update it.
*                           in case that entry is updated but there was no 'moved'
*                           we not send interrupt to the CPU.
*******************************************************************************/
static GT_VOID snetChtL2iAutoLearnFdbEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr,
    IN GT_U32 spUnknown,
    IN GT_U32 entryIndex,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC * learnInfoPtr,
    IN GT_BOOL  entryAlreadyExists
)
{
    DECLARE_FUNC_NAME(snetChtL2iAutoLearnFdbEntry);

    GT_U32 macTblReg;               /* MAC table entry address */
    GT_U32 hwData[SMEM_CHT_MAC_TABLE_WORDS];/* 5 words of mac address hw entry  */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 da_securAccessLevel=0,sa_securAccessLevel=0;/* DA/SA security access level */
    SNET_CHEETAH_FDB_ENTRY_ENT                macEntryType;
    SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC specialFields;
    GT_U32               macAddrWords[2];

    __LOG(("Write To FDB index[%d] spUnknown[%d] entryAlreadyExists[%d] \n",
                  entryIndex,
                  spUnknown,
                  entryAlreadyExists));

    /* Get entryPtr according to entryIndx */
    macTblReg = SMEM_CHT_MAC_TBL_MEM(devObjPtr, entryIndex);

    memset(hwData, 0, SMEM_CHT_MAC_ENTRY_BYTES);

    regAddr = SMEM_CHT2_SECURITY_LEVEL_CONF_REG(devObjPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
         /* DA security access Level */
        smemRegFldGet(devObjPtr,regAddr,4,3,&da_securAccessLevel);
        /* SA security access Level */
        smemRegFldGet(devObjPtr,regAddr,0,3,&sa_securAccessLevel);
    }
    else
    if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {
         /* DA security access Level */
        smemRegFldGet(devObjPtr,regAddr,12,3,&da_securAccessLevel);

        /* SA security access Level */
        smemRegFldGet(devObjPtr,regAddr,8,3,&sa_securAccessLevel);
    }
    else /* dummy - not used */
    {
        da_securAccessLevel = 0;
        sa_securAccessLevel = 0;
    }

    __LOG(("Auto learn : DA security access Level[%d] \n",da_securAccessLevel));
    __LOG(("Auto learn : SA security access Level[%d] \n",sa_securAccessLevel));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {

        macEntryType = SNET_CHEETAH_FDB_ENTRY_MAC_E;

        memset(&specialFields,0,sizeof(SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC));

        /* SrcID */
        fldValue = learnInfoPtr->FDB_srcId;
        specialFields.srcId = fldValue;

        /* User Defined */ /*udb*/
        fldValue = learnInfoPtr->FDB_UDB;
        specialFields.udb = fldValue;

        fldValue = learnInfoPtr->FDB_origVid1;
        specialFields.origVid1 = fldValue;

        /* The MAC DA Access level for this entry */
        fldValue = da_securAccessLevel;
        specialFields.daAccessLevel = fldValue;

        /* The MAC SA Access level for this entry */
        fldValue = sa_securAccessLevel;
        specialFields.saAccessLevel = fldValue;

        sfdbLion3FdbSpecialMuxedFieldsSet(devObjPtr,hwData,entryIndex,macEntryType,
            &specialFields);

        /* valid fdb entry */
        fldValue = 1;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
            entryIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID,
            fldValue);

        /* make aging in two age-pass - for auto-learned should be 1 */
        fldValue = 1;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
            entryIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
            fldValue);
        /* FID */
        fldValue = vlanInfoPtr->fid;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
            entryIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID,
            fldValue);

        /* MAC address */
        macAddrWords[0] = GT_HW_SIM_MAC_LOW32(descrPtr->macSaPtr);
        macAddrWords[1] = GT_HW_SIM_MAC_HIGH16(descrPtr->macSaPtr);

        /* set the MAC ADDR words */
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_MAC_ADDR_SET(devObjPtr,
            hwData,
            entryIndex,
            macAddrWords);

        /* dev id */
        fldValue = learnInfoPtr->FDB_devNum;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
            entryIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID,
            fldValue);

        /* is trunk */
        fldValue = learnInfoPtr->FDB_isTrunk;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
            entryIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK,
            fldValue);

        /* PortNum/TrunkNum */
        if(learnInfoPtr->FDB_isTrunk)
        {
            fldValue = learnInfoPtr->FDB_port_trunk;
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
                entryIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM,
                fldValue);
        }
        else
        {
            fldValue = learnInfoPtr->FDB_port_trunk;
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
                entryIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM,
                fldValue);
        }

        /*spUnknown*/
        fldValue = spUnknown;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,hwData,
            entryIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN,
            fldValue);

        smemMemSet(devObjPtr, macTblReg, hwData, 5);
    }
    else
    {
        /* MAC table entry word0 */
        SMEM_U32_SET_FIELD(hwData[0], 0, 1, 1); /* valid fdb entry */

        SMEM_U32_SET_FIELD(hwData[0], 2, 1, 1); /* make aging in two age-pass - for auto-learned should be 1 */

        SMEM_U32_SET_FIELD(hwData[0], 5, 12,    /* VLAN id*/
                           descrPtr->eVid);
        SMEM_U32_SET_FIELD(hwData[0], 17, 8,    /* Bits 7:0 of the MAC address */
                           descrPtr->macSaPtr[5]);
        SMEM_U32_SET_FIELD(hwData[0], 25, 7,    /* Bits 14:8 of the MAC address */
                           descrPtr->macSaPtr[4]);


        /* MAC table entry word1 */
        SMEM_U32_SET_FIELD(hwData[1], 0, 1,     /* Bits 15 of the MAC address */
                           (descrPtr->macSaPtr[4] >> 0x7));
        SMEM_U32_SET_FIELD(hwData[1], 1, 8,     /* Bits 23:16 of the MAC address */
                           (descrPtr->macSaPtr[3]));
        SMEM_U32_SET_FIELD(hwData[1], 9, 8,    /* Bits 31:24 of the MAC address */
                           (descrPtr->macSaPtr[2]));
        SMEM_U32_SET_FIELD(hwData[1], 17, 8,   /* Bits 39:32 of the MAC address */
                           (descrPtr->macSaPtr[1]));
        SMEM_U32_SET_FIELD(hwData[1], 25, 7,   /* Bits 46:40 of the MAC address */
                           (descrPtr->macSaPtr[0]));


        /* MAC table entry word2 */
        SMEM_U32_SET_FIELD(hwData[2], 0, 1,     /* Bits 47 of the MAC address */
                           (descrPtr->macSaPtr[0] >> 0x7));
        SMEM_U32_SET_FIELD(hwData[2], 1, 5,     /* The source device number */
                           learnInfoPtr->FDB_devNum);
        SMEM_U32_SET_FIELD(hwData[2], 6, 5,     /* Source-ID */
                           learnInfoPtr->FDB_srcId);

        SMEM_U32_SET_FIELD(hwData[2], 13, 1,    /* Trunk */
                           learnInfoPtr->FDB_isTrunk);
        SMEM_U32_SET_FIELD(hwData[2], 14, 4, learnInfoPtr->FDB_port_trunk); /* PortNum/Trunk/Num */
        SMEM_U32_SET_FIELD(hwData[2], 18, 3, learnInfoPtr->FDB_coreId); /* coreId */

        /* MAC table entry word3 */
        SMEM_U32_SET_FIELD(hwData[3], 2, 1,    /* Storm Prevent */
                           spUnknown);

        if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
        {
             /* DA security access Level */
            SMEM_U32_SET_FIELD(hwData[3], 11, 3, da_securAccessLevel);

            /* SA security access Level */
            SMEM_U32_SET_FIELD(hwData[3], 14, 3, sa_securAccessLevel);
        }

        smemMemSet(devObjPtr, macTblReg, hwData, 4);
    }


    /* A new source MAC address received was retained */
    if(learnInfoPtr->FDB_isMoved ||
       entryAlreadyExists == GT_FALSE)
    {
        /* send interrupt to the CPU only when we add new entry or when 'Station movement' */
        __LOG(("Interrupt: NA LEARN \n"));

        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                              SMEM_CHT_NA_LEARN_INT,
                              SMEM_CHT_FDB_SUM_INT(devObjPtr));
    }

    if(devObjPtr->supportEArch &&
       entryAlreadyExists == GT_FALSE &&
       spUnknown == 0)
    {
        __LOG(("call to update the FDB bank counters \n"));

        sfdbChtBankCounterAction(devObjPtr,entryIndex,
                        SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E,
                        SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_PP_AUTO_LEARN_E);
    }


}

/*******************************************************************************
*   getInterruptCauseRegAddr
*
* DESCRIPTION:
*       get the cause register address from DB of interrupts.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       currDbPtr           - the entry in the DB
*
*   RETURNS:
*       regAddr
*
*******************************************************************************/
GT_U32 getInterruptCauseRegAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr
)
{
    GT_U32  currCauseRegAddr;

    if(currDbPtr == NULL)
    {
        return SMAIN_NOT_VALID_CNS;
    }

    switch(currDbPtr->causeReg.registersDbType)
    {
        case SKERNEL_REGISTERS_DB_TYPE__LAST___E:
            currCauseRegAddr = SMAIN_NOT_VALID_CNS;
            break;
        case SKERNEL_REGISTERS_DB_TYPE_SIP5_E:
            currCauseRegAddr = *((GT_U32*)(((char*)SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)) + currDbPtr->causeReg.registerOffsetInDb));
            break;
        case SKERNEL_REGISTERS_DB_TYPE_LEGACY_E:
            currCauseRegAddr = *((GT_U32*)(((char*)SMEM_CHT_MAC_REG_DB_GET(devObjPtr)) + currDbPtr->causeReg.registerOffsetInDb));
            break;
        case SKERNEL_REGISTERS_DB_TYPE_NOT_VALID_E:
            currCauseRegAddr = SMAIN_NOT_VALID_CNS;
            break;
        default:
            currCauseRegAddr = SMAIN_NOT_VALID_CNS;
            break;
    }

    return currCauseRegAddr;
}

/*******************************************************************************
*   getInterruptMaskRegAddr
*
* DESCRIPTION:
*       get the mask register address from DB of interrupts.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       currDbPtr           - the entry in the DB
*
*   RETURNS:
*       regAddr
*
*******************************************************************************/
GT_U32 getInterruptMaskRegAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr
)
{
    GT_U32  currMaskRegAddr;

    if(currDbPtr == NULL)
    {
        return SMAIN_NOT_VALID_CNS;
    }

    switch(currDbPtr->maskReg.registersDbType)
    {
        case SKERNEL_REGISTERS_DB_TYPE__LAST___E:
            /* should not happen */
            currMaskRegAddr = SMAIN_NOT_VALID_CNS;
            break;
        case SKERNEL_REGISTERS_DB_TYPE_SIP5_E:
            currMaskRegAddr = *((GT_U32*)(((char*)SMEM_CHT_MAC_REG_DB_SIP5_GET(devObjPtr)) + currDbPtr->maskReg.registerOffsetInDb));
            break;
        case SKERNEL_REGISTERS_DB_TYPE_LEGACY_E:
            currMaskRegAddr = *((GT_U32*)(((char*)SMEM_CHT_MAC_REG_DB_GET(devObjPtr)) + currDbPtr->maskReg.registerOffsetInDb));
            break;
        case SKERNEL_REGISTERS_DB_TYPE_NOT_VALID_E:
            currMaskRegAddr = SMAIN_NOT_VALID_CNS;/*Allow the mask be not valid ... meaning that there is no mask and cause always valid */
            break;
        default:
            currMaskRegAddr = SMAIN_NOT_VALID_CNS;
            break;
     }

    return currMaskRegAddr;
}

/*******************************************************************************
*   lookForInterruptBasicInfoByCauseReg
*
* DESCRIPTION:
*       look for match in the DB for cause register.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       currDbPtr           - the place to check
*
*   RETURNS:
*       GT_TRUE  - register was found
*       GT_FALSE - register was NOT found
*
*******************************************************************************/
GT_BOOL lookForInterruptBasicInfoByCauseReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr
)
{
    GT_U32  currCauseRegAddr;
    GT_U32  currMaskRegAddr;

    currCauseRegAddr = getInterruptCauseRegAddr(devObjPtr,currDbPtr);

    if(currCauseRegAddr != causeRegAddr)
    {
        return GT_FALSE;
    }

    currMaskRegAddr = getInterruptMaskRegAddr(devObjPtr,currDbPtr);

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("causeRegAddr[0x%8.8x] found in interrupts tree as [%s] \n"
                                                 "maskRegAddr[0x%8.8x] as [%s] \n",
        causeRegAddr ,
        currDbPtr->causeReg.registerName ? currDbPtr->causeReg.registerName : "??",
        currMaskRegAddr ,
        currDbPtr->maskReg.registerName ? currDbPtr->maskReg.registerName : "??"));

    return GT_TRUE;

}

/*******************************************************************************
*   doInterruptsSubTree
*
* DESCRIPTION:
*       handle a sub tree in the interrupts DB.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       currDbPtr           - the place to check
*       causeBitBmp         - bmp of the bits that need to be set/clear in the cause register
*       isReadAction        - GT_TRUE  - read action by the CPU (the device will clear interrupts if needed)
*                             GT_FALSE - the device sets interrupts if needed
*   OUTPUT:
*
*   RETURNS:
*       None
*
*******************************************************************************/
void doInterruptsSubTree
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr,
    IN GT_U32 causeBitBmp,
    IN GT_BOOL  isReadAction
)
{
    GT_U32   causeRegAddr;
    GT_U32   maskRegAddr;
    GT_U32   causeRegVal;             /* Cause register value */
    GT_U32   causeRegMask;            /* Cause register mask */
    static GT_U32   globalCauseRegAddr = 0x00000030;/* global cause register address */
    GT_U32   causeBitBmpToMyFather;
    SKERNEL_INTERRUPT_REG_INFO_STC  *myFather_currDbPtr;
    SKERNEL_DEVICE_OBJECT * origDevObjPtr;/* orig device */
    GT_BIT  updateSummaryAndFather;
    GT_U32  ii,iiMax;/* iterator and it's max value */

    causeRegAddr = getInterruptCauseRegAddr(devObjPtr,currDbPtr);
    maskRegAddr  = getInterruptMaskRegAddr(devObjPtr,currDbPtr);

    if(devObjPtr->devMemUnitMemoryGetPtr)
    {
        /* protect the interrupt tree between the cores that access to shared units */
        SCIB_SEM_TAKE;

        /* check if need to trigger the cause register interrupt on other device.
        if yes then also the ISR should triggered on the other device */
        origDevObjPtr = devObjPtr;
        devObjPtr =
            devObjPtr->devMemUnitMemoryGetPtr(origDevObjPtr,causeRegAddr);

        if(origDevObjPtr != devObjPtr)
        {
            /* the port group changed */
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("due to 'shared unit'(reg addr[0x%8.8x]) the port group changed from [%d] to [%d]\n"
                          ,causeRegAddr
                          ,origDevObjPtr->portGroupId
                          ,devObjPtr->portGroupId
                          ));

        }
    }

    /* read interrupt cause data */
    smemRegGet(devObjPtr, causeRegAddr, &causeRegVal);

    if(maskRegAddr == SMAIN_NOT_VALID_CNS)
    {
        /*Allow the mask be not valid ... meaning that there is no mask and cause always valid */
        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("Allow the mask be not valid ... meaning that there is no mask and cause always valid \n"));
        causeRegMask = 0xFFFFFFFF;
    }
    else
    {
        /* read interrupt mask data */
        smemRegGet(devObjPtr, maskRegAddr, &causeRegMask);
    }

    if(isReadAction == GT_TRUE)
    {
        /* clear from the cause the needed bits */
        causeRegVal &= ~causeBitBmp;

        updateSummaryAndFather = (causeRegVal & 0xFFFFFFFE) ? 0 : 1;
    }
    else
    {
        /* set cause bit in the data for interrupt cause */
        causeRegVal |= causeBitBmp;
        updateSummaryAndFather = (causeBitBmp & causeRegMask) ? 1 : 0;
    }

    /* if mask is set for bitmap, set summary bit in data for interrupt cause */
    if (updateSummaryAndFather)
    {
        if(isReadAction == GT_TRUE)
        {
            /* the register hold no more reasons to be with interrupts indications */
            causeRegVal = 0;
        }
        else
        {
            SMEM_U32_SET_FIELD(causeRegVal, 0, 1, 1);
        }

        smemRegSet(devObjPtr, causeRegAddr, causeRegVal);

        iiMax = currDbPtr->isSecondFatherExists ? 2 : 1;

        for(ii = 0 ; ii < iiMax ; ii++)
        {
            if(ii == 0 )
            {
                /* my representative bit in my fathers register */
                causeBitBmpToMyFather = (1 << currDbPtr->myFatherInfo.myBitIndex);

                /* we are allowed to go to the 'father' and set our representative bit in it */
                myFather_currDbPtr = currDbPtr->myFatherInfo.interruptPtr;
            }
            else /* support second father */
            {
                /* my representative bit in my second fathers register */
                causeBitBmpToMyFather = (1 << currDbPtr->myFatherInfo_2.myBitIndex);

                /* we are allowed to go to the 'second father' and set our representative bit in it */
                myFather_currDbPtr = currDbPtr->myFatherInfo_2.interruptPtr;
            }

            if(myFather_currDbPtr == NULL)
            {
                /* we should be at the 'global' register */
                if(globalCauseRegAddr == causeRegAddr)
                {
                    if(isReadAction == GT_TRUE)
                    {
                        /* we are done ... no more to do */
                    }
                    else
                    {
                        /* set the global interrupt too */
                        snetChtPerformScibSetInterrupt(devObjPtr->deviceId);
                    }
                }
            }
            else
            {
                /* let my father handle it's predecessors */
                doInterruptsSubTree(devObjPtr,myFather_currDbPtr,causeBitBmpToMyFather,isReadAction);
            }
        }
    }
    else
    {
        if(isReadAction == GT_TRUE)
        {
            /* the register hold other reasons to be 'triggered' from other cause registers ... */
            /* so clear only the needed bits in it and do not clear it's summary and it's father */
        }

        smemRegSet(devObjPtr, causeRegAddr, causeRegVal);
    }

    if(devObjPtr->devMemUnitMemoryGetPtr)
    {
        SCIB_SEM_SIGNAL;
    }

    return;
}


/*******************************************************************************
*   snetChetahDoInterruptFromTree
*
* DESCRIPTION:
*       Set cheetah interrupt according to the 'tree' of the interrupts
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeBitNum         - Interrupt Cause Register Bit
*       setInterrupt        - set the interrupt (or not)
*
*   RETURNS:
*       GT_TRUE - register was found and treated
*       GT_FALSE - register was NOT found and therefore not treated
*
*******************************************************************************/
GT_BOOL snetChetahDoInterruptFromTree
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeBitBmp
)
{
    SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr;
    GT_BOOL    found;

    currDbPtr = devObjPtr->myInterruptsDbPtr;

    while(currDbPtr &&
          currDbPtr->causeReg.registersDbType != SKERNEL_REGISTERS_DB_TYPE__LAST___E)
    {
        /* check the specific line */
        found = lookForInterruptBasicInfoByCauseReg(devObjPtr,causeRegAddr,currDbPtr);

        if(found == GT_TRUE)
        {
            /* start handling the leaf with all it's predecessors */
            doInterruptsSubTree(devObjPtr,currDbPtr,causeBitBmp,GT_FALSE);

            return GT_TRUE;
        }

        currDbPtr++;
    }

    return GT_FALSE;
}

/*******************************************************************************
*   snetChetahReadCauseInterruptFromTree
*
* DESCRIPTION:
*       Read may causing 'clear' of my father's cause register
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       bmpOfBitsToClear    - bmp of bits to clear in the register
*   RETURNS:
*       GT_TRUE - register was found and treated
*       GT_FALSE - register was NOT found and therefore not treated
*
*******************************************************************************/
GT_BOOL snetChetahReadCauseInterruptFromTree
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 bmpOfBitsToClear
)
{
    SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr;
    GT_BOOL    found;

    currDbPtr = devObjPtr->myInterruptsDbPtr;

    while(currDbPtr &&
          currDbPtr->causeReg.registersDbType != SKERNEL_REGISTERS_DB_TYPE__LAST___E)
    {
        /* check the specific line */
        found = lookForInterruptBasicInfoByCauseReg(devObjPtr,causeRegAddr,currDbPtr);

        if(found == GT_TRUE)
        {
            /* start handling the leaf with all it's predecessors */
            doInterruptsSubTree(devObjPtr,currDbPtr,bmpOfBitsToClear,GT_TRUE);

            return GT_TRUE;
        }

        currDbPtr++;
    }

    return GT_FALSE;
}


/*******************************************************************************
*   snetChetahInterruptsTreeDump
*
* DESCRIPTION:
*       dump the interrupt tree of a device
*
* INPUTS:
*       devNum - device number as stated in the INI file.
*
*   RETURNS:
*       GT_TRUE - register was found and treated
*       GT_FALSE - register was NOT found and therefore not treated
*
*******************************************************************************/
static void interruptsTreeDump
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SKERNEL_INTERRUPT_REG_INFO_STC  *currDbPtr;
    GT_U32   causeRegAddr;
    GT_U32   maskRegAddr;
    GT_U32   causeBitToMyFather;
    SKERNEL_INTERRUPT_REG_INFO_STC  *myFather_currDbPtr;
    GT_U32   myFather_causeRegAddr;
    GT_U32   myFather_maskRegAddr;
    GT_U32  ii,iiMax;/* iterator and it's max value */

    currDbPtr = devObjPtr->myInterruptsDbPtr;

    while(currDbPtr &&
          currDbPtr->causeReg.registersDbType != SKERNEL_REGISTERS_DB_TYPE__LAST___E)
    {
        causeRegAddr = getInterruptCauseRegAddr(devObjPtr,currDbPtr);
        maskRegAddr  = getInterruptMaskRegAddr(devObjPtr,currDbPtr);

        iiMax = currDbPtr->isSecondFatherExists ? 2 : 1;
        if(currDbPtr->isSecondFatherExists)
        {
            simGeneralPrintf("-- NOTE: This node with 2 fathers  -- \n");
        }

        for(ii = 0 ; ii < iiMax ; ii++)
        {
            if(ii == 0 )
            {
                /* my representative bit in my fathers register */
                causeBitToMyFather = currDbPtr->myFatherInfo.myBitIndex;

                /* we are allowed to go to the 'father' and set our representative bit in it */
                myFather_currDbPtr = currDbPtr->myFatherInfo.interruptPtr;
            }
            else /* support second father */
            {
                /* my representative bit in my second fathers register */
                causeBitToMyFather = currDbPtr->myFatherInfo_2.myBitIndex;

                /* we are allowed to go to the 'second father' and set our representative bit in it */
                myFather_currDbPtr = currDbPtr->myFatherInfo_2.interruptPtr;
            }

            myFather_causeRegAddr = getInterruptCauseRegAddr(devObjPtr,myFather_currDbPtr);
            myFather_maskRegAddr  = getInterruptMaskRegAddr(devObjPtr,myFather_currDbPtr);

            simGeneralPrintf("causeRegAddr[0x%8.8x] as [%s] \n"
                     "maskRegAddr[0x%8.8x] as [%s] \n"
                     "causeBitToMyFather[%d] \n"
                     "myFather_currDbPtr->causeRegAddr[0x%8.8x] as [%s] \n"
                     "myFather_currDbPtr->maskRegAddr[0x%8.8x] as [%s] \n"
                     ,
                /* info from me */
                causeRegAddr ,
                currDbPtr->causeReg.registerName ? currDbPtr->causeReg.registerName : "??",
                maskRegAddr ,
                currDbPtr->maskReg.registerName ? currDbPtr->maskReg.registerName : "??",
                /* info from my direct father */
                myFather_currDbPtr ? causeBitToMyFather : 0,
                myFather_currDbPtr ? myFather_causeRegAddr : 0,
                myFather_currDbPtr ? (myFather_currDbPtr->causeReg.registerName ? myFather_currDbPtr->causeReg.registerName : "??") : "no father",
                myFather_currDbPtr ? myFather_maskRegAddr : 0,
                myFather_currDbPtr ? (myFather_currDbPtr->maskReg.registerName  ? myFather_currDbPtr->maskReg.registerName  : "??") : "no father"

                );
        }

        currDbPtr++;
    }

    return;
}

/*******************************************************************************
*   snetChetahInterruptsTreeDump
*
* DESCRIPTION:
*       dump the interrupt tree of a device
*
* INPUTS:
*       devNum - device number as stated in the INI file.
*
*   RETURNS:
*       GT_TRUE - register was found and treated
*       GT_FALSE - register was NOT found and therefore not treated
*
*******************************************************************************/
void snetChetahInterruptsTreeDump
(
    IN  GT_U32    devNum
)
{
    SKERNEL_DEVICE_OBJECT* deviceObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SKERNEL_DEVICE_OBJECT* currDeviceObjPtr;
    GT_U32  dev;

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        simGeneralPrintf(" multi-core device [%d] \n",devNum);
        for(dev = 0 ; dev < deviceObjPtr->numOfCoreDevs ; dev++)
        {
            currDeviceObjPtr = deviceObjPtr->coreDevInfoPtr[dev].devObjPtr;

            if(currDeviceObjPtr->myInterruptsDbPtr == NULL)
            {
                simGeneralPrintf("the device not supports interrupts tree \n");
                return;
            }

            simGeneralPrintf("\n\n ===interrupts tree : Start  ====\n\n");

            interruptsTreeDump(currDeviceObjPtr);

            simGeneralPrintf("\n\n ===interrupts tree : Done  ====\n\n");
        }
    }
    else
    {
        if(deviceObjPtr->myInterruptsDbPtr == NULL)
        {
            simGeneralPrintf("the device not supports interrupts tree \n");
            return;
        }

        simGeneralPrintf("\n\n ===interrupts tree : Start  ====\n\n");

        interruptsTreeDump(deviceObjPtr);

        simGeneralPrintf("\n\n ===interrupts tree : Done  ====\n\n");

    }


}

/*******************************************************************************
*   snetChetahDoInterruptLimited
*
* DESCRIPTION:
*       Set cheetah interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeMaskRegAddr    - Interrupt Cause Register Mask
*       causeBitNum         - Interrupt Cause Register Bit
*       globalBitNum        - Global Interrupt Cause Register Bit
*       setInterrupt        - set the interrupt (or not)
*
*******************************************************************************/
GT_VOID snetChetahDoInterruptLimited
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeMaskRegAddr,
    IN GT_U32 causeBitBmp,
    IN GT_U32 globalBitBmp,
    IN GT_U32 setInterrupt
)
{
    DECLARE_FUNC_NAME(snetChetahDoInterruptLimited);

    GT_U32 causeRegVal;             /* Cause register value */
    GT_U32 causeRegMask;            /* Cause register mask */
    GT_BOOL setIntr = GT_FALSE;     /* Set interrupt */
    GT_BOOL setGlbIntr = GT_FALSE;  /* Set interrupt in global register */
    SKERNEL_DEVICE_OBJECT * origDevObjPtr;/* orig device */
    GT_BOOL found;

    if(devObjPtr->myInterruptsDbPtr)
    {
        /* the device hold tree of interrupts , let it find the register in the tree */
        /* it may not be found due to 'intermediate' development time that not all
           tree supported fully */
        found = snetChetahDoInterruptFromTree(devObjPtr,causeRegAddr,causeBitBmp);

        if(found == GT_TRUE)
        {
            return;
        }
    }

    if(devObjPtr->devMemUnitMemoryGetPtr)
    {
        /* protect the interrupt tree between the cores that access to shared units */
        SCIB_SEM_TAKE;

        /* check if need to trigger the cause register interrupt on other device.
        if yes then also the ISR should triggered on the other device */
        origDevObjPtr = devObjPtr;
        devObjPtr =
            devObjPtr->devMemUnitMemoryGetPtr(origDevObjPtr,causeRegAddr);

        if(origDevObjPtr != devObjPtr)
        {
            /* the port group changed */
            __LOG(("due to 'shared unit'(reg addr[0x%8.8x]) the port group changed from [%d] to [%d]\n"
                          ,causeRegAddr
                          ,origDevObjPtr->portGroupId
                          ,devObjPtr->portGroupId
                          ));

        }
    }

    /* read interrupt cause data */
    smemRegGet(devObjPtr, causeRegAddr, &causeRegVal);

    /* read interrupt mask data */
    smemRegGet(devObjPtr, causeMaskRegAddr, &causeRegMask);

    /* set cause bit in the data for interrupt cause */
    causeRegVal = causeRegVal | causeBitBmp;

    /* if mask is set for bitmap, set summary bit in data for interrupt cause */
    if (causeBitBmp & causeRegMask)
    {
        SMEM_U32_SET_FIELD(causeRegVal, 0, 1, 1);
        setIntr = GT_TRUE;
    }

    smemRegSet(devObjPtr, causeRegAddr, causeRegVal);

    if (setIntr == GT_TRUE)
    {
        /* Global Interrupt Cause Register */
        smemRegGet(devObjPtr, SMEM_CHT_GLB_INT_CAUSE_REG(devObjPtr), &causeRegVal);

        /* Global Interrupt Summary Mask */
        smemRegGet(devObjPtr, SMEM_CHT_GLB_INT_MASK_REG(devObjPtr), &causeRegMask);

        /* set cause bit in the data for interrupt cause */
        causeRegVal = causeRegVal | globalBitBmp;

        /* if mask is set for bitmap, set summary bit in data for interrupt cause */
        if (globalBitBmp & causeRegMask)
        {
            /* IntSum */
            SMEM_U32_SET_FIELD(causeRegVal, 0, 1, 1);
            setGlbIntr = GT_TRUE;
        }

        smemRegSet(devObjPtr, SMEM_CHT_GLB_INT_CAUSE_REG(devObjPtr), causeRegVal);
    }

    if (setGlbIntr && setInterrupt)
    {
        snetChtPerformScibSetInterrupt(devObjPtr->deviceId);
    }

    __LOG(("Global Interrupt [%s generated] , causeRegAddr[0x%8.8x] causeMaskRegAddr[0x%8.8x] causeBitBmp[0x%8.8x]\n"
                  ,(setGlbIntr && setInterrupt) ? "" : "not"
                  ,causeRegAddr
                  ,causeMaskRegAddr
                  ,causeBitBmp
                  ));

    if(devObjPtr->devMemUnitMemoryGetPtr)
    {
        SCIB_SEM_SIGNAL;
    }

}

/*******************************************************************************
*   snetChetahDoInterrupt
*
* DESCRIPTION:
*       Set cheetah interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeMaskRegAddr    - Interrupt Cause Register Mask
*       causeBitNum         - Interrupt Cause Register Bit
*       globalBitNum        - Global Interrupt Cause Register Bit
*
*
*******************************************************************************/
GT_VOID snetChetahDoInterrupt
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeMaskRegAddr,
    IN GT_U32 causeBitBmp,
    IN GT_U32 globalBitBmp
)
{
    snetChetahDoInterruptLimited(devObjPtr,causeRegAddr,causeMaskRegAddr,
                causeBitBmp,globalBitBmp,1);
}
/*******************************************************************************
*   snetChtL2iMacEntryAddress
*
* DESCRIPTION:
*       Perform FDB Lookup and if success return FDB entry address
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       entryInfoPtr    - (pointer to) entry hash info
*
* OUTPUT:
*       entryIndexPtr   - index for the FDB found or free entry
*       entryOffsetPtr  - FDB offset within the 'bucket'
*
* RETURN:
*       GT_OK           - FDB entry was found
*       GT_NOT_FOUND    - FDB entry was not found and assigned with new address
*       GT_FAIL         - FDB entry was not found and no free address space
*
*******************************************************************************/
GT_STATUS snetChtL2iMacEntryAddress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC *entryInfoPtr,
    OUT GT_U32 * entryIndexPtr,
    OUT GT_U32 * entryOffsetPtr
)
{
    GT_STATUS status;                        /* function return status */
    GT_BOOL retVal;                          /* function return value */
    GT_U32  indexArr[2];

    ASSERT_PTR(entryInfoPtr);
    ASSERT_PTR(entryIndexPtr);
    ASSERT_PTR(entryOffsetPtr);

    /* Lookup for existence */
    retVal = snetChtL2iFdbLookup(devObjPtr, entryInfoPtr,GT_TRUE/*from CPU*/,GT_TRUE,
                                 NULL, &indexArr[0], entryOffsetPtr,
                                 NULL,
                                 NULL);
    *entryIndexPtr = indexArr[0];
    if (retVal)
    {
        /* entry was found */
        status = GT_OK;
    }
    else if((*entryIndexPtr) != SMAIN_NOT_VALID_CNS)
    {
        /* entry was not found , but we got index to learn it */
        status = GT_NOT_FOUND;
    }
    else
    {
        /* entry was not found , but we got NO index to learn it */
        *entryIndexPtr = 0;
        status = GT_FAIL;
    }

    return status;
}

/*******************************************************************************
*   snetCheetahInterruptsMaskChanged
*
* DESCRIPTION:
*       handle interrupt mask registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       causeRegAddr   - cause register address
*       maskRegAddr    - mask  register address
*       intRegBit      - interrupt bit in the global cause register
*       currentCauseRegVal - current cause register values
*       lastMaskRegVal - last mask register values
*       newMaskRegVal  - new mask register values
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
extern void snetCheetahInterruptsMaskChanged(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  causeRegAddr,
    IN GT_U32  maskRegAddr,
    IN GT_U32  intRegBit,
    IN GT_U32  currentCauseRegVal,
    IN GT_U32  lastMaskRegVal,
    IN GT_U32  newMaskRegVal
)
{
    GT_U32  diffCause;

    diffCause = ((newMaskRegVal & 0xFFFFFFFE) & ~lastMaskRegVal) & currentCauseRegVal;

    /* check if there is a reason to do interrupt */
    if(diffCause)
    {
        snetChetahDoInterrupt(devObjPtr,causeRegAddr,
                              maskRegAddr,diffCause,intRegBit);
    }

    return;
}

/*******************************************************************************
*   snetCheetahL2UcLocalSwitchFilter
*
* DESCRIPTION:
*        Local Switching of Known Unicast Packets filtering
* INPUTS:
*        devObjPtr      - pointer to device object.
*        descrPtr       - pointer to the frame's descriptor.
*        packetType     - type of packet.
*        destPorts      - number of egress port.
*       learnInfoPtr  - (pointer to) FDB extra info
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*   For XCat devices and above, local switching is permitted by the bridge
*   engine only if this feature is enabled in BOTH the ingress port AND VLAN;
*   otherwise the packet is assigned a SOFT DROP command and counted by the
*   Bridge Drop Counter
*
*******************************************************************************/
static GT_VOID snetCheetahL2UcLocalSwitchFilter
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    OUT SNET_CHEETAH_L2I_FILTERS_INFO * filterInfoPtr,
    IN SNET_CHEETAH_L2I_LEARN_INFO_STC   *learnInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCheetahL2UcLocalSwitchFilter);

    GT_U32  origSrcPortOrTrnk = learnInfoPtr->FDB_port_trunk;/* source port/trunk for filtering */
    GT_U32  origSrcPort = descrPtr->localPortGroupPortAsGlobalDevicePort;/* source port for filtering */

    if(learnInfoPtr->srcEPortIsGlobal)
    {
        if (daLookupInfoPtr->targed.ucast.isTrunk == 0 &&
            daLookupInfoPtr->targed.ucast.portNum == origSrcPortOrTrnk)
        {
            filterInfoPtr->localPortCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

            __LOG(("Local Switching filter 'GLOBAL EPort' [0x%x] : assign command[%d](soft DROP) \n",
                          origSrcPortOrTrnk,
                          filterInfoPtr->localPortCmd));
        }
    }
    else if(daLookupInfoPtr->targed.ucast.isTrunk == learnInfoPtr->FDB_isTrunk)
    {
        /*came from trunk , go to trunk
            OR
          came from port , go to port
        */

        if (daLookupInfoPtr->targed.ucast.isTrunk)
        {
            /*came from trunk , go to trunk */
            if (daLookupInfoPtr->targed.ucast.trunkId ==
                descrPtr->origSrcEPortOrTrnk)
            {
                filterInfoPtr->localPortCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

                __LOG(("Local Switching filter 'trunk' [0x%x] : assign command[%d](soft DROP) \n",
                              descrPtr->origSrcEPortOrTrnk,
                              filterInfoPtr->localPortCmd));
            }
        }
        else
        {
            /*came from port , go to port */
            if ((daLookupInfoPtr->devNum == descrPtr->srcDev) &&
                (daLookupInfoPtr->targed.ucast.portNum == origSrcPortOrTrnk))
            {
                filterInfoPtr->localPortCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

                __LOG(("Local Switching filter 'port' [0x%x] device[0x%x] : assign command[%d](soft DROP) \n",
                              origSrcPortOrTrnk,
                              daLookupInfoPtr->devNum,
                              filterInfoPtr->localPortCmd));
            }
        }
    }
    else if(daLookupInfoPtr->targed.ucast.isTrunk == 0 &&
            daLookupInfoPtr->devNum == descrPtr->ownDev &&
            daLookupInfoPtr->targed.ucast.portNum == origSrcPort)
    {
        /*came from trunk , go to port */

        /* packet came from trunk but destined to port , the src port */
        filterInfoPtr->localPortCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;

        __LOG(("Local Switching filter : packet came from trunk but destined to port , the src port :\n"
                      "'port' [0x%x] device[0x%x] in trunk[0x%x]: assign command[%d](soft DROP) \n",
                      origSrcPort,
                      daLookupInfoPtr->devNum,
                      origSrcPortOrTrnk,
                      filterInfoPtr->localPortCmd));
    }
}

/*******************************************************************************
* snetChtPortSpeedGet
*
* DESCRIPTION:
*       Gets speed for specified port on specified device.
*
* APPLICABLE DEVICES:  All DxCh Devices.
*
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       portNum     - physical port number (or CPU port)
*
* OUTPUTS:
*       speedPtr - pointer to actual port speed
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetChtL2iPortSpeedGet
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr,
    IN  GT_U32                  portNum,
    OUT SKERNEL_PORT_SPEED_ENT  *speedPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iPortSpeedGet);

    GT_U32 regAddr;                             /* register address */
    GT_U32 fldValue;                            /* register field value */

    if((portNum != SNET_CHT_CPU_PORT_CNS) && IS_CHT_HYPER_GIGA_PORT(devObjPtr, portNum))
    {
        __LOG(( "port[%d] SPEED_10000 \n",portNum));
        *speedPtr = SKERNEL_PORT_SPEED_10000_E;
    }
    else
    {
        regAddr = SMEM_CHT_PORT_STATUS0_REG(devObjPtr, portNum);
        smemRegGet(devObjPtr, regAddr, &fldValue);

        /* bit 1 in the register */
        if (SMEM_U32_GET_FIELD(fldValue,1,1))
        {
            __LOG(( "port[%d] SPEED_1000 \n",portNum));
            *speedPtr = SKERNEL_PORT_SPEED_10000_E;
        }
        else
        {
            /* bit 2 in the register */
            if(SMEM_U32_GET_FIELD(fldValue,2,1))
            {
                __LOG(( "port[%d] SPEED_100 \n",portNum));
                *speedPtr = SKERNEL_PORT_SPEED_100_E;
            }
            else
            {
                __LOG(( "port[%d] SPEED_10 \n",portNum));
                *speedPtr = SKERNEL_PORT_SPEED_10_E;
            }
        }
    }
}

/*******************************************************************************
* snetChtL2iPortRateLimitDropCount
*
* DESCRIPTION:
*       Gets speed for specified port on specified device.
*
* APPLICABLE DEVICES:  All DxCh Devices.
*
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       portNum     - physical port number (or CPU port)
*
* OUTPUTS:
*       speedPtr - pointer to actual port speed
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetChtL2iPortRateLimitDropCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iPortRateLimitDropCount);

    GT_U32 localSrcPort;                        /* local source port */
    GT_U32 fldValue;                            /* register field value */
    GT_U32 regAddr;                             /* register address */
    GT_U32 ingressRateLimtConfReg1;             /* ingress rate configuration register data 1*/
    GT_U64 rateLimitCounter;                    /* ingress rate limit forwarded/dropped counter  */
    GT_U64 increment;
    GT_U32 bitOffset;                           /* register bit offset */

    localSrcPort = descrPtr->localDevSrcPort;

    if(filtersInfoPtr->rateLimitCmd != SKERNEL_EXT_PKT_CMD_HARD_DROP_E &&
       filtersInfoPtr->rateLimitCmd != SKERNEL_EXT_PKT_CMD_SOFT_DROP_E )
    {
        __LOG(("rate limit command is not SOFT/HARD DROP , so no counting \n",
            localSrcPort));
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION2_BRIDGE_INGRESS_PHYSICAL_PORT_TBL_MEM(devObjPtr,localSrcPort);
        smemRegFldGet(devObjPtr, regAddr, 4, 1, &fldValue);
    }
    else
    {
        regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF0_REG(devObjPtr, localSrcPort);
        smemRegFldGet(devObjPtr, regAddr, 11, 1, &fldValue);
    }

    /*RateLimitDropCountEn*/
    if(fldValue == 0)
    {
        __LOG(("port[%d] : Ingress RateLimit drops are NOT enabled for counting \n",
            localSrcPort));
        return;
    }

    __LOG(("port[%d] : Ingress RateLimit drops are enabled for counting \n",
        localSrcPort));

    /* Enable a port to increment the global drop counter */
    regAddr = SMEM_CHT_INGR_RATE_LIMIT_DROP_CNT_0_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr,&rateLimitCounter.l[0]);

    regAddr = SMEM_CHT_INGR_RATE_LIMIT_DROP_CNT_1_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr,&rateLimitCounter.l[1]);

    rateLimitCounter.l[1] &= 0xFF;/*support only 8 bits (40-32=8)*/

    regAddr = SMEM_CHT_INGR_RATE_LIMIT_CONF1_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr,&ingressRateLimtConfReg1);

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        bitOffset = 24;
    }
    else
    {
        bitOffset = 12;
    }

    if(SMEM_U32_GET_FIELD(ingressRateLimtConfReg1, bitOffset, 1))
    {
        /* Ingress RateLimit Mode */
        /* Packet increment */
        increment.l[0] = 1;
        __LOG(("counter mode : packets (increment by 1) \n"));
    }
    else
    {
        /* Bytes increment*/
        increment.l[0] = descrPtr->byteCount;
        __LOG(("counter mode : bytes (increment by %d) \n",
            descrPtr->byteCount));
    }
    rateLimitCounter = prvSimMathAdd64(rateLimitCounter, increment);
    rateLimitCounter.l[1] &= 0xFF;/*support only 8 bits (40-32=8)*/
    /* Set drop port counter */
    regAddr = SMEM_CHT_INGR_RATE_LIMIT_DROP_CNT_0_REG(devObjPtr);
    smemRegSet(devObjPtr, regAddr,rateLimitCounter.l[0]);

    regAddr = SMEM_CHT_INGR_RATE_LIMIT_DROP_CNT_1_REG(devObjPtr);
    smemRegSet(devObjPtr, regAddr,rateLimitCounter.l[1]);

    __LOG(("new counters value (40 bits) = HIGH[0x%2.2x] LOW[0x%8.8x]  \n",
        rateLimitCounter.l[1],
        rateLimitCounter.l[0]));


    return;
}


/*******************************************************************************
*   snetChtL2iPortRateLimit
*
* DESCRIPTION:
*       Protect ingress port from excessive ingress rates of known UC, unknown UC,
*       MC, BC and TCP SYN packets
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       daLookupInfoPtr - pointer to DA lookup info structure
*
* OUTPUTS:
*       descrPtr        - pointer to the frame's descriptor.
*       filtersInfoPtr      - filters info structure pointer
* RETURNS:
*
* COMMENTS:
*       Port rate limit counter resets to 0 at the beginning of each window.
*       During the time window and according to the setting of <UnkUc RateLimEn>,
*       <UcRateLimEn>, <McRateLimEn> and <BcRateLimEn>,
*       this counter counts all bytes/packets received on this port that are subject
*       to ingress rate limiting.
*       When this counter reaches <IngressLimit>, it stops
*       and all packets subject to ingress rate limiting that are received thereafter
*       (until the time window ends) are dropped.
*       In Simulation, time window uses time stamp of the incoming packet.
*       Additional speed configuration for ingress port rate limit calculation
*       is implemented in XCAT A2 metal fix.
*       Relevant only for network ports
*
*******************************************************************************/
static GT_VOID snetChtL2iPortRateLimit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr,
    OUT SNET_CHEETAH_L2I_FILTERS_INFO * filtersInfoPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iPortRateLimit);

    GT_U32 regAddr;                             /* register address */
    GT_U32 * regPtr;                            /* pointer to register data */
    GT_U32 ingressRateLimtConfReg1;             /* ingress rate configuration register data 1*/
    GT_U32 fldValue;                            /* register field value */
    GT_U32 localSrcPort;                        /* local source port */
    GT_U32 currentTimeWindow;                   /* current time window */
    GT_U64 rateLimitCounter;                    /* ingress rate limit forwarded/dropped counter  */
    GT_U32 bitOffset;                           /* register bit offset */

    enum RATE_LIMIT_TYPE_ENT{
        TCP_SYN_E,
        REGISTERED_MC,
        ALL_BC,
        UNREGISTERED_MC,
        UNKNOWN_UC,
        KNOWN_UC
    }rateLimitType;   /* type of traffic for rate limit */
    GT_U32  startBit;
    enum REG_ID_ENT{
        REG_ID_ING_PORT_RATE_LIMIT_CONFIG_0_E,
        REG_ID_ING_PORT_RATE_LIMIT_CONFIG_1_E
    }regId;/* NON sip5 : type of register */

    localSrcPort = descrPtr->localDevSrcPort;

    if(devObjPtr->notSupportIngressPortRateLimit)
    {
        /* Device not support rate limiting */
        return;
    }


    if(localSrcPort == SNET_CHT_CPU_PORT_CNS)
    {
        /* Not relevant for CPU port */
        __LOG(("Not relevant for CPU port"));
        return;
    }

    /* TCP SYN packet */
    if (descrPtr->isIp && descrPtr->ipProt == SNET_TCP_PROT_E &&
       (descrPtr->l4StartOffsetPtr != NULL) &&
           (descrPtr->l4StartOffsetPtr[13] == SNET_CHEETAH2_TCP_SYN_FLAGS_E ||
            descrPtr->l4StartOffsetPtr[13] == SNET_CHEETAH2_TCP_SYN_ACK_FLAGS_E))
    {
        __LOG(("TCP SYN \n"));
        rateLimitType = TCP_SYN_E;
    }
    else
    {
        if(daLookupInfoPtr->found)
        {
            switch(descrPtr->macDaType)
            {
                case SKERNEL_UNICAST_MAC_E:
                    __LOG(("KNOWN_UC \n"));
                    rateLimitType = KNOWN_UC;
                    break;
                case SKERNEL_MULTICAST_MAC_E:
                    __LOG(("REGISTERED_MC \n"));
                    rateLimitType = REGISTERED_MC;
                    break;
                case SKERNEL_BROADCAST_MAC_E:
                case SKERNEL_BROADCAST_ARP_E:
                    __LOG(("ALL_BC (registered) \n"));
                    rateLimitType = ALL_BC;
                    break;
                default:
                    /*should not happen*/
                    __LOG(("ERROR \n"));
                    return;
            }
        }
        else
        {
            switch(descrPtr->macDaType)
            {
                case SKERNEL_UNICAST_MAC_E:
                    __LOG(("UNKNOWN_UC \n"));
                    rateLimitType = UNKNOWN_UC;
                    break;
                case SKERNEL_MULTICAST_MAC_E:
                    __LOG(("UNREGISTERED_MC \n"));
                    rateLimitType = UNREGISTERED_MC;
                    break;
                case SKERNEL_BROADCAST_MAC_E:
                case SKERNEL_BROADCAST_ARP_E:
                    __LOG(("ALL_BC (unregistered) \n"));
                    rateLimitType = ALL_BC;
                    break;
                default:
                    /*should not happen*/
                    __LOG(("ERROR \n"));
                    return;
            }
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        switch(rateLimitType)
        {
            case TCP_SYN_E:
                startBit = 5;
                break;
            case REGISTERED_MC:
                startBit = 1;
                break;
            case ALL_BC:
                startBit = 0;
                break;
            case UNKNOWN_UC:
                startBit = 3;
                break;
            case UNREGISTERED_MC:
                startBit = 6;
                break;
            case KNOWN_UC:
                startBit = 2;
                break;
            default:
                /* should not happen */
                startBit = 32;
                break;
        }

        regAddr = SMEM_LION2_BRIDGE_INGRESS_PHYSICAL_PORT_TBL_MEM(devObjPtr,localSrcPort);
    }
    else
    {
        regId = REG_ID_ING_PORT_RATE_LIMIT_CONFIG_0_E;
        switch(rateLimitType)
        {
            case TCP_SYN_E:
                regId = REG_ID_ING_PORT_RATE_LIMIT_CONFIG_1_E;
                startBit = 23;
                break;
            case REGISTERED_MC:
                startBit = 3;
                break;
            case ALL_BC:
                startBit = 2;
                break;
            case UNKNOWN_UC:
                startBit = 5;
                break;
            case UNREGISTERED_MC:
                regId = REG_ID_ING_PORT_RATE_LIMIT_CONFIG_1_E;
                startBit = 24;
                break;
            case KNOWN_UC:
                startBit = 4;
                break;
            default:
                /* should not happen */
                startBit = 32;
                break;
        }
        if(regId == REG_ID_ING_PORT_RATE_LIMIT_CONFIG_0_E)
        {
            regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF0_REG(devObjPtr, localSrcPort);
        }
        else
        {
            regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF1_REG(devObjPtr, localSrcPort);
        }
    }

    smemRegFldGet(devObjPtr, regAddr, startBit, 1, &fldValue);

    if(fldValue == 0)
    {
        /* Rate limiting disabled */
        __LOG(("Port[%d] Rate limiting disabled \n",localSrcPort));
        return;
    }

    __LOG(("Port[%d] Rate limiting enabled \n",localSrcPort));

    regAddr = SMEM_CHT_INGR_RATE_LIMIT_CONF1_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr,&ingressRateLimtConfReg1);

    /* Ports current time window */
    snetChtL2iPortCurrentTimeWindowGet(devObjPtr, localSrcPort, &currentTimeWindow);


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION2_BRIDGE_INGRESS_PHYSICAL_PORT_RATE_LIMIT_COUNTERS_TBL_MEM(devObjPtr,localSrcPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
    }
    else
    {
        regAddr = SMEM_CHT_INGR_PORT_RATE_LIMIT_CNT_REG(devObjPtr, localSrcPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
    }

    if (currentTimeWindow > devObjPtr->portsArr[localSrcPort].portCurrentTimeWindow)
    {
        /* Current time window exceeds time window for the port */
        __LOG(("Current time window exceeds time window for the port \n"
               "Assign new port current window for forwarded packets \n"));

        rateLimitCounter.l[0] = 0;
        /* Assign new port current window for forwarded packets */
        devObjPtr->portsArr[localSrcPort].portCurrentTimeWindow = currentTimeWindow;

    }
    else
    {
        rateLimitCounter.l[0] = SMEM_U32_GET_FIELD(regPtr[0], 0, 22);
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            bitOffset = 24;
        }
        else
        {
            bitOffset = 12;
        }
        /* Ingress RateLimit Mode */
        if(SMEM_U32_GET_FIELD(ingressRateLimtConfReg1, bitOffset, 1))
        {
            __LOG(("Ingress RateLimit packet Mode \n"));
            rateLimitCounter.l[0]++;
        }
        else
        {
            __LOG(("Ingress RateLimit bytes Mode \n"));
            rateLimitCounter.l[0] += descrPtr->byteCount;
        }
    }

    /* Set internal port counter */
    SMEM_U32_SET_FIELD(regPtr[0], 0, 22, rateLimitCounter.l[0]);

    /* <IngressLimit> */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION2_BRIDGE_INGRESS_PHYSICAL_PORT_TBL_MEM(devObjPtr,localSrcPort);
        regPtr = smemMemGet(devObjPtr, regAddr);

        fldValue = snetFieldValueGet(regPtr, 7, 22);
    }
    else
    {
        regAddr = SMEM_CHT_INGR_PORT_BRDG_CONF1_REG(devObjPtr, localSrcPort);
        smemRegFldGet(devObjPtr, regAddr, 5, 16, &fldValue);
    }

    /* Incremented port counter exceeds the configured port limit */
    if(rateLimitCounter.l[0] > fldValue)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                bitOffset = 25;
            }
            else
            {
                bitOffset = 13;
            }

            filtersInfoPtr->rateLimitCmd =
                SMEM_U32_GET_FIELD(ingressRateLimtConfReg1, bitOffset, 3);
            __LOG(("IngressRate LimitDropMode - command[%d] \n",
                          filtersInfoPtr->rateLimitCmd));

            /* the counting of drops due to rate limit should be done only if
               the actual drop reason was 'rate limit' , and this is according
               to the 'priorities' of the drop reasons */

            /* so counting will be called from snetChtL2iPhase1Decision(...) */
        }
        else
        {
            /* IngressRate LimitDropMode */
            if(SMEM_U32_GET_FIELD(ingressRateLimtConfReg1, 13, 1))
            {
                filtersInfoPtr->rateLimitCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
                __LOG(("IngressRate LimitDropMode - command[%d] (hard coded - HARD DROP) \n",
                              filtersInfoPtr->rateLimitCmd));
            }
            else
            {
                filtersInfoPtr->rateLimitCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
                __LOG(("IngressRate LimitDropMode - command[%d] (hard coded - SOFT DROP) \n",
                              filtersInfoPtr->rateLimitCmd));
            }

            /* count the drops */
            snetChtL2iPortRateLimitDropCount(devObjPtr,descrPtr,filtersInfoPtr);
        }
    }
}

/*******************************************************************************
* snetChtL2iCurrentTimeWindowGet
*
* DESCRIPTION:
*       Gets port current time window.
*
* APPLICABLE DEVICES:  All DxCh Devices.
*
*
* INPUTS:
*       devObjPtr - pointer to device object
*       localSrcPort - rate limiting ingress port
*
* OUTPUTS:
*       currentTimeWindowPtr - current time window
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetChtL2iPortCurrentTimeWindowGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 localSrcPort,
    OUT GT_U32 * currentTimeWindowPtr
)
{
    DECLARE_FUNC_NAME(snetChtL2iPortCurrentTimeWindowGet);

    GT_U32 rateLimitWindow;                     /* ingress rate limiting time window assigned to ports */
    SKERNEL_PORT_SPEED_ENT speed;               /* port's speed */
    GT_U32 regAddr;                             /* register address */
    GT_U32 fldValue;                            /* register field value */

    ASSERT_PTR(currentTimeWindowPtr);

    *currentTimeWindowPtr = 0;

    if(devObjPtr->notSupportIngressPortRateLimit)
    {
        /* Device not support rate limiting */
        return;
    }

    snetChtL2iPortSpeedGet(devObjPtr, localSrcPort, &speed);

    switch(speed)
    {
        case SKERNEL_PORT_SPEED_10_E:
            regAddr = SMEM_CHT_INGR_RATE_LIMIT_CONF0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr , 0, 12, &fldValue);
            break;
        case SKERNEL_PORT_SPEED_100_E:
            regAddr = SMEM_CHT_INGR_RATE_LIMIT_CONF0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr , 12, 9, &fldValue);
            break;
        case SKERNEL_PORT_SPEED_1000_E:
            regAddr = SMEM_CHT_INGR_RATE_LIMIT_CONF0_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr , 21, 6, &fldValue);
            break;
        case SKERNEL_PORT_SPEED_10000_E:
            regAddr = SMEM_CHT_INGR_RATE_LIMIT_CONF1_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddr , 0, 12, &fldValue);
            break;
        default:
            skernelFatalError("snetChtL2iCurrentTimeWindowGet: invalid port speed\n");
            return;
    }

    /* The granularity of this field is 256 uSec */
    rateLimitWindow = (fldValue * 256) / 1000;
    if(rateLimitWindow)
    {
        /* Port current time window in milliseconds */
        *currentTimeWindowPtr = (SIM_OS_MAC(simOsTickGet)() / rateLimitWindow);
    }

    __LOG(("Port [%d] current time window in milliseconds[0x%8.8x] \n",
                  localSrcPort,
                  (*currentTimeWindowPtr)));
}

/*******************************************************************************
* snetL2iTablesFormatInit
*
* DESCRIPTION:
*        init the format of L2i tables.
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
void snetL2iTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
)
{
    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EVLAN_E,
            bobcat2B0L2iIngressVlanTableFieldsFormat, bobcat2B0L2iIngressVlanTableFieldsNames);
    }
    else
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EVLAN_E,
            lion3L2iIngressVlanTableFieldsFormat, lion3L2iIngressVlanFieldsTableNames);

    }



    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EPORT_E,
            sip5_20L2iEPortTableFieldsFormat, lion3L2iEPortFieldsTableNames);

        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_FDB_E,
            sip5_20FdbFdbTableFieldsFormat, lion3FdbFdbFieldsTableNames);

        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_FDB_AU_MSG_E,
            sip5_20FdbAuMsgTableFieldsFormat, lion3FdbAuMsgFieldsTableNames);
    }
    else
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_BRIDGE_INGRESS_EPORT_E,
            lion3L2iEPortTableFieldsFormat, lion3L2iEPortFieldsTableNames);

        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_FDB_E,
            lion3FdbFdbTableFieldsFormat, lion3FdbFdbFieldsTableNames);

        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_FDB_AU_MSG_E,
            lion3FdbAuMsgTableFieldsFormat, lion3FdbAuMsgFieldsTableNames);
    }


}


