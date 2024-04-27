/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3CentralizedCnt.c
*
* DESCRIPTION:
*       Cheetah3 Asic Simulation
*       Centralized Counter Unit.
*       Source Code file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 54 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah3.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <common/Utils/Math/sMath.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3CentralizedCnt.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

/* CNC block counters number */
#define MAX_CNC_BLOCKS_CNS                          16

/**
 *  Enum: SMEM_CHT3_CNC_BYTE_CNT_MODE_E
 *
 *  Description:
 *      Byte Count Mode.
 *
 *  Fields:
 *
 *  SMEM_CHT3_CNC_BYTE_CNT_L2_MODE_E - The Byte Count counter counts the entire packet byte count for all packet types.
 *  SMEM_CHT3_CNC_BYTE_CNT_L3_MODE_E - Byte Count counters counts the packet L3 fields (the entire packet minus the L3 offset)
 *  SMEM_CHT3_CNC_BYTE_CNT_PACKETS_MODE_E - Byte Count counters counts packet number
 *
**/
typedef enum {
    SMEM_CHT3_CNC_BYTE_CNT_L2_MODE_E = 0,
    SMEM_CHT3_CNC_BYTE_CNT_L3_MODE_E,
    SMEM_CHT3_CNC_BYTE_CNT_PACKETS_MODE_E
}SMEM_CHT3_CNC_BYTE_CNT_MODE_E;

/**
 *  Structure: SNET_CHT3_CNC_CLIENT_INFO_STC
 *
 *  Description:
 *      CNC client info
 *
 *  Fields:
 *
 *      client      - type of client (unified value)
 *      clientHwBit - the bit index of the client -- explicit HW value
 *
 *      index       - client index
 *      byteCntMode - byte count mode
 *      userDefined - user defined field
 *      rangeBitmap - client range bitmap
 *      cncUnitIndex- the CNC unit index (0/1) (Sip5 devices , legacy device only 1 unit)
 *
 *
**/
typedef struct {
    SNET_CNC_CLIENT_E client;
    GT_U32            clientHwBit;


    GT_U32 index;
    SMEM_CHT3_CNC_BYTE_CNT_MODE_E byteCntMode;
    GT_UINTPTR userDefined;
    GT_U64 rangeBitmap[MAX_CNC_BLOCKS_CNS];

    GT_U32 cncUnitIndex;
    GT_CHAR*     clientsNamePtr;

} SNET_CHT3_CNC_CLIENT_INFO_STC;

#define SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(funcName)     \
GT_STATUS funcName                                      \
(                                                       \
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,               \
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,      \
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr    \
)

static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientL2L3VlanIngrTrigger        );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientPclLookUpIngrTrigger       );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientVlanPassDropIngrTrigger    );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientVlanPassDropEgrTrigger     );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientTxqQueuePassDropEgrTrigger );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientPclEgrTrigger              );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientArpTblTrigger              );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientTunnelStartTrigger         );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetXCatCncClientTunnelTerminatedTrigger    );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientHaTargetEPortTrigger       );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetCht3CncClientIpclSourceEPortTrigger     );
static SNET_CHT3_CNC_CLIENT_TRIG_FUN_MAC(snetLion3CncClientPreEgressPacketTypePassDropTrigger);

/* the "first non-zero bit table" for values 0-15       */
/* the value for 0 is not relevant                      */
/* 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 */
/* 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1,  0,  2,  0,  1,  0  */
/* packed in one 32-bit value                           */
/* bits n,n+1 contain the "first non-zero bit" for n    */
#define PRV_FIRST_NON_ZERO_BIT_CNS 0x12131210

/* CNC block entry size is 2 words */
#define CNC_BLOCK_ENTRY_WORDS_CNS                   2

/* Counters update message was dropped in block<N> due to Rate Limiting FIFO Full */
#define SMEM_CHT3_CNC_BLOCK_RATE_LIMIT_FIFO(block)      0x2000 << block

/* Fast Dump of last triggered block finished */
#define SMEM_CHT3_CNC_DUMP_BLOCK_FINISHED               1 << 25

#define SMEM_CHT3_CNC_BLOCK_WRAP_AROUND(block)          1 << ((block) + 1)

#define SNET_CHT3_NO_CNC_DUMP_BUFFERS_SLEEP_TIME        1

extern GT_BIT  oldWaitDuringSkernelTaskForAuqOrFua;

/*
 * typedef: CNC_COUNTER_FORMAT_ENT
 *
 * Description:
 *      CNC modes of counter formats.
 *
 * Enumerations:
 *   CNC_COUNTER_FORMAT_PKT_29_BC_35_E;
 *          Partitioning of the 64 bits entry is as following:
 *          Packets counter: 29 bits, Byte Count counter: 35 bits
 *   CNC_COUNTER_FORMAT_PKT_27_BC_37_E;
 *          Partitioning of the 64 bits entry is as following:
 *          Packets counter: 27 bits, Byte Count counter: 37 bits
 *   CNC_COUNTER_FORMAT_PKT_37_BC_27_E;
 *          Partitioning of the 64 bits entry is as following:
 *          Packets counter: 37 bits, Byte Count counter: 27 bits
 *
 *   CNC_COUNTER_FORMAT_PKT_64_BC_0_E - sip5_20
 *          PKT_64_BC_0; PKT_64_BC_0; Partitioning of the 64 Entry bits is as follows:
 *           Packet counter: 64 bits
 *           Byte Count counter: 0 bits (No Counting)
 *   CNC_COUNTER_FORMAT_PKT_0_BC_64_E - sip5_20
 *           PKT_0_BC_64; PKT_0_BC_64; Partitioning of the 64 Entry bits is as follows:
 *           Packet counter: 0 bits (No Counting)
 *           Byte Count counter: 64 bits
 * Comments:
 *          CNC_COUNTER_FORMAT_PKT_27_BC_37_E and CNC_COUNTER_FORMAT_PKT_37_BC_27_E
 *          relevant only for Lion and above.
 *          For other devices the mode of CNC_COUNTER_FORMAT_PKT_29_BYTE_35_E is used.
 */
typedef enum
{
    CNC_COUNTER_FORMAT_PKT_29_BC_35_E,
    CNC_COUNTER_FORMAT_PKT_27_BC_37_E,
    CNC_COUNTER_FORMAT_PKT_37_BC_27_E,

    /* new in sip5_20 */
    CNC_COUNTER_FORMAT_PKT_64_BC_0_E,
    CNC_COUNTER_FORMAT_PKT_0_BC_64_E

} CNC_COUNTER_FORMAT_ENT;

/*
 * typedef: struct SNET_CHT3_CNC_COUNTER_STC
 *
 * Description:
 *      The counter entry contents.
 *
 * Fields:
 *      byteCount      - byte count
 *      packetCount    - packets count
 *
 * Comment:
 *      See the possible counter HW formats of Lion and above devices in
 *      SNET_CHT3_CNC_COUNTER_STC_CNC_COUNTER_FORMAT_ENT description.
 *      For DxCh3 and DxChXcat devices the byte counter has 35 bits,
 *      the packets counter has 29 bits
 */
typedef struct
{
    GT_U64     byteCount;
    GT_U64     packetCount;
} SNET_CHT3_CNC_COUNTER_STC;

/*******************************************************************************
*   SNET_CHT3_CNC_CLIENT_TRIG_FUN
*
* DESCRIPTION:
*       CNC client trigger function
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
typedef GT_STATUS (* SNET_CHT3_CNC_CLIENT_TRIG_FUN)
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
);

static GT_STATUS snetCht3CncClientRangeBitmapGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
);

static GT_BOOL snetCht3CncSendMsg2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 block,
    IN GT_U32 * cncDumpPtr,
    INOUT GT_BOOL * doInterruptPtr,
    IN GT_U32   cncUnitIndex
);

static GT_STATUS snetCht3CncBlockWrapStatusSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 block,
    IN GT_U32 index,
    IN GT_U32   cncUnitIndex
);

static GT_VOID snetCht3CncCounterFormatGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN  GT_U32                              block,
    OUT CNC_COUNTER_FORMAT_ENT    *formatPtr,
    IN GT_U32   cncUnitIndex
);

static GT_STATUS snetCht3CncCounterHwRead
(
    IN   CNC_COUNTER_FORMAT_ENT  format,
    IN   GT_U32                            * regPtr,
    OUT  SNET_CHT3_CNC_COUNTER_STC         * cncCounterPtr
);

static GT_STATUS snetCht3CncCounterHwWrite
(
    IN   CNC_COUNTER_FORMAT_ENT  format,
    IN   GT_U32                            * regPtr,
    IN   SNET_CHT3_CNC_COUNTER_STC         * cncCounterPtr
);

static GT_STATUS snetCht3CncCounterWrapAroundCheck
(
    IN   GT_U32 wrapEn,
    IN   CNC_COUNTER_FORMAT_ENT  format,
    IN   SNET_CHT3_CNC_COUNTER_STC         * cncCounterPtr,
    IN   GT_BIT                         do64BitsByteWrapAround
);

static GT_CHAR*     cncClientsNamesArr[SNET_CNC_CLIENT_LAST_E + 1] =
{
    "(cnc)TTI ACTION 0"                                  ,    /* SNET_CNC_CLIENT_TTI_ACTION_0_E,                                  */
    "(cnc)TTI ACTION 1"                                  ,    /* SNET_CNC_CLIENT_TTI_ACTION_1_E,                                  */
    "(cnc)IPCL_LOOKUP 0 ACTION 0"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E,                        */
    "(cnc)IPCL_LOOKUP 0 ACTION 1"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E,                        */
    "(cnc)IPCL_LOOKUP 0 ACTION 2"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E,                        */
    "(cnc)IPCL_LOOKUP 0 ACTION 3"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E,                        */
    "(cnc)IPCL_LOOKUP 1 ACTION 0"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E,                        */
    "(cnc)IPCL_LOOKUP 1 ACTION 1"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E,                        */
    "(cnc)IPCL_LOOKUP 1 ACTION 2"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E,                        */
    "(cnc)IPCL_LOOKUP 1 ACTION 3"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E,                        */
    "(cnc)IPCL_LOOKUP 2 ACTION 0"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E,                        */
    "(cnc)IPCL_LOOKUP 2 ACTION 1"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_1_E,                        */
    "(cnc)IPCL_LOOKUP 2 ACTION 2"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_2_E,                        */
    "(cnc)IPCL_LOOKUP 2 ACTION 3"                        ,    /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E,                        */
    "(cnc)INGRESS VLAN L2 L3"                            ,    /* SNET_CNC_CLIENT_INGRESS_VLAN_L2_L3_E,                            */
    "(cnc)SOURCE EPORT"                                  ,    /* SNET_CNC_CLIENT_SOURCE_EPORT_E,                                  */
    "(cnc)INGRESS VLAN_PASS_DROP"                        ,    /* SNET_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E,                        */
    "(cnc)PACKET TYPE_PASS_DROP"                         ,    /* SNET_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E,                         */
    "(cnc)EGRESS VLAN_PASS_DROP"                         ,    /* SNET_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E,                         */
    "(cnc)EGRESS QUEUE PASS DROP AND QCN PASS DROP"      ,    /* SNET_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E,      */
    "(cnc)ARP INDEX"                                     ,    /* SNET_CNC_CLIENT_ARP_INDEX_E,                                     */
    "(cnc)TUNNEL START INDEX"                            ,    /* SNET_CNC_CLIENT_TUNNEL_START_INDEX_E,                            */
    "(cnc)TARGET EPORT"                                  ,    /* SNET_CNC_CLIENT_TARGET_EPORT_E,                                  */
    "(cnc)EPCL ACTION 0"                                 ,    /* SNET_CNC_CLIENT_EPCL_ACTION_0_E,                                 */
    "(cnc)EPCL ACTION 1"                                 ,    /* SNET_CNC_CLIENT_EPCL_ACTION_1_E,                                 */
    "(cnc)EPCL ACTION 2"                                 ,    /* SNET_CNC_CLIENT_EPCL_ACTION_2_E,                                 */
    "(cnc)EPCL ACTION 3"                                 ,    /* SNET_CNC_CLIENT_EPCL_ACTION_3_E,                                 */
    "(cnc)TRAFFIC MANAGER PASS DROP"                     ,    /* SNET_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E,                     */
    /*sip5_20*/
    "(cnc)TTI ACTION 2"                                  ,    /* SNET_CNC_CLIENT_TTI_ACTION_2_E,                                  */
    "(cnc)TTI ACTION 3"                                  ,    /* SNET_CNC_CLIENT_TTI_ACTION_3_E,                                  */

    NULL
};


/* Trigger function array holds trigger function for all clients */
static SNET_CHT3_CNC_CLIENT_TRIG_FUN gCncClientTrigFuncArr[SNET_CNC_CLIENT_LAST_E] =
{
    snetXCatCncClientTunnelTerminatedTrigger,   /* SNET_CNC_CLIENT_TTI_ACTION_0_E                                 */
    snetXCatCncClientTunnelTerminatedTrigger,   /* SNET_CNC_CLIENT_TTI_ACTION_1_E                                 */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_1_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_2_E                       */
    snetCht3CncClientPclLookUpIngrTrigger,      /* SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E                       */
    snetCht3CncClientL2L3VlanIngrTrigger,       /* SNET_CNC_CLIENT_INGRESS_VLAN_L2_L3_E                           */
    snetCht3CncClientIpclSourceEPortTrigger,    /* SNET_CNC_CLIENT_SOURCE_EPORT_E                                 */
    snetCht3CncClientVlanPassDropIngrTrigger,   /* SNET_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E                       */
    snetLion3CncClientPreEgressPacketTypePassDropTrigger,/* SNET_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E                */
    snetCht3CncClientVlanPassDropEgrTrigger,    /* SNET_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E                        */
    snetCht3CncClientTxqQueuePassDropEgrTrigger,/* SNET_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E     */
    snetCht3CncClientArpTblTrigger,             /* SNET_CNC_CLIENT_ARP_INDEX_E                                    */
    snetCht3CncClientTunnelStartTrigger,        /* SNET_CNC_CLIENT_TUNNEL_START_INDEX_E                           */
    snetCht3CncClientHaTargetEPortTrigger,      /* SNET_CNC_CLIENT_TARGET_EPORT_E                                 */
    snetCht3CncClientPclEgrTrigger,             /* SNET_CNC_CLIENT_EPCL_ACTION_0_E                                */
    snetCht3CncClientPclEgrTrigger,             /* SNET_CNC_CLIENT_EPCL_ACTION_1_E                                */
    snetCht3CncClientPclEgrTrigger,             /* SNET_CNC_CLIENT_EPCL_ACTION_2_E                                */
    snetCht3CncClientPclEgrTrigger,             /* SNET_CNC_CLIENT_EPCL_ACTION_3_E                                */
    NULL,                                       /* SNET_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E                    */
    snetXCatCncClientTunnelTerminatedTrigger,   /* SNET_CNC_CLIENT_TTI_ACTION_2_E                                 */
    snetXCatCncClientTunnelTerminatedTrigger,   /* SNET_CNC_CLIENT_TTI_ACTION_3_E                                 */


};

/*******************************************************************************
*   convertClientToBitIndex
*
* DESCRIPTION:
*       convert enum of SNET_CNC_CLIENT_E to INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT_CNC_CLIENT_E
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       clientUnified  - the unified client
*
* OUTPUTS:
*
*   RETURNS: the bit index in the HW
*
*******************************************************************************/
static INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT_CNC_CLIENT_E convertClientToBitIndex
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CNC_CLIENT_E    clientUnified
)
{
    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* ch3 .. Lion2 */
        switch(clientUnified)
        {
            case SNET_CNC_CLIENT_TTI_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_XCAT_CNC_CLIENT_TUNNEL_TERMINATION_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL0_0_LOOKUP_INGR_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL0_1_LOOKUP_INGR_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL1_LOOKUP_INGR_E;
            case SNET_CNC_CLIENT_INGRESS_VLAN_L2_L3_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_L2_L3_VLAN_INGR_E;
            case SNET_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_VLAN_PASS_DROP_INGR_E;
            case SNET_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_VLAN_PASS_DROP_EGR_E;
            case SNET_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_TXQ_QUEUE_PASS_DROP_EGR_E;
            case SNET_CNC_CLIENT_ARP_INDEX_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_ARP_TBL_E;
            case SNET_CNC_CLIENT_TUNNEL_START_INDEX_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_TUNNEL_START_E;
            case SNET_CNC_CLIENT_EPCL_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_CHT3_CNC_CLIENT_PCL_EGR_E;
            default:
                break;
        }
    }
    else
    {
        /* Lion3,bobcat2*/
        switch(clientUnified)
        {
            case SNET_CNC_CLIENT_TTI_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_0_E;
            case SNET_CNC_CLIENT_TTI_ACTION_1_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_1_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_0_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_1_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_1_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_2_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_2_E;
            case SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E;
            case SNET_CNC_CLIENT_INGRESS_VLAN_L2_L3_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_INGRESS_VLAN_L2_L3_E;
            case SNET_CNC_CLIENT_SOURCE_EPORT_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_SOURCE_EPORT_E;
            case SNET_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E;
            case SNET_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E;
            case SNET_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E;
            case SNET_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EGRESS_QUEUE_PASS_DROP_AND_QCN_PASS_DROP_E;
            case SNET_CNC_CLIENT_ARP_INDEX_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_ARP_INDEX_E;
            case SNET_CNC_CLIENT_TUNNEL_START_INDEX_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TUNNEL_START_INDEX_E;
            case SNET_CNC_CLIENT_TARGET_EPORT_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TARGET_EPORT_E;
            case SNET_CNC_CLIENT_EPCL_ACTION_0_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_0_E;
            case SNET_CNC_CLIENT_EPCL_ACTION_1_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_1_E;
            case SNET_CNC_CLIENT_EPCL_ACTION_2_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_2_E;
            case SNET_CNC_CLIENT_EPCL_ACTION_3_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_EPCL_ACTION_3_E;
            case SNET_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E:
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TRAFFIC_MANAGER_PASS_DROP_E;
            case SNET_CNC_CLIENT_TTI_ACTION_2_E:
                if(!SMEM_CHT_IS_SIP5_20_GET(devObjPtr)) break;
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_2_E;
            case SNET_CNC_CLIENT_TTI_ACTION_3_E:
                if(!SMEM_CHT_IS_SIP5_20_GET(devObjPtr)) break;
                return INTERNAL_SPECIFIC_HW_VALUES__SNET_LION3_CNC_CLIENT_TTI_ACTION_3_E;
            default:
                break;
        }
    }

    skernelFatalError("convertClientToBitIndex : unknown client [%d] \n" , clientUnified);
    return 0;
}



/*******************************************************************************
*   snetCht3CncIncrement
*
* DESCRIPTION:
*       Increment CNC counter - packets and bytes
*
* INPUTS:
*       regPtr              - pointer to CNC counter register data.
*       block               - block index
*       cncFormat           - CNC modes of counter formats
*       cncCounterAddPtr    - pointer to increment CNC counter value
*       wrapEn              - wraparound counter enable/disable
*
* RETURNS:
*       GT_OK           - CNC packet counter successfully incremented
*       GT_FULL         - CNC packet counter is full
*
*******************************************************************************/
static GT_STATUS snetCht3CncIncrement
(
    GT_U32 * regPtr,
    GT_U32 block,
    CNC_COUNTER_FORMAT_ENT cncFormat,
    SNET_CHT3_CNC_COUNTER_STC  * cncCounterAddPtr,
    GT_U32 wrapEn
)
{
    SNET_CHT3_CNC_COUNTER_STC  cncCounter;
    GT_STATUS status = GT_OK;
    GT_BIT      do64BitsByteWrapAround = 0;

    /* Read the 64-bit HW counter to SW format */
    snetCht3CncCounterHwRead(cncFormat, regPtr, &cncCounter);

    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :before update : packetCount.l[0] value [0x%8.8x] \n",cncCounter.packetCount.l[0]));
    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :before update : packetCount.l[1] value [0x%8.8x]\n" ,cncCounter.packetCount.l[1]));
    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :before update : byteCount.l[0] value [0x%8.8x] \n",cncCounter.byteCount.l[0]));
    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :before update : byteCount.l[1] value [0x%8.8x]\n" ,cncCounter.byteCount.l[1]));

    /* test if wraparound is disabled and maximum value already reached */
    status = snetCht3CncCounterWrapAroundCheck(wrapEn, cncFormat, &cncCounter,0);
    /* Maximum value reached */
    if(status != GT_OK)
    {
        /* Wraparound disabled */
        if(wrapEn == 0)
        {
            __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :Wraparound disabled \n"));
            return status;
        }
    }
    /* Increment CNC packet counter */
    cncCounter.packetCount =
        prvSimMathAdd64(cncCounter.packetCount, cncCounterAddPtr->packetCount);

    if((cncFormat == CNC_COUNTER_FORMAT_PKT_0_BC_64_E) &&
        cncCounter.byteCount.l[1] == 0xFFFFFFFF &&
        (0xFFFFFFFF - cncCounter.byteCount.l[0]) < cncCounterAddPtr->byteCount.l[0])
    {
        /* even 64 bits 'bytes' counter will wrap around */
        do64BitsByteWrapAround = 1;
    }

    /* Increment CNC byte counter */
    cncCounter.byteCount =
        prvSimMathAdd64(cncCounter.byteCount, cncCounterAddPtr->byteCount);

    /* Test if wraparound is disabled and packet or maximum CNC byte count value have reached */
    status = snetCht3CncCounterWrapAroundCheck(wrapEn, cncFormat, &cncCounter,do64BitsByteWrapAround);

    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :after update : packetCount.l[0] value [0x%8.8x] \n",cncCounter.packetCount.l[0]));
    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :after update : packetCount.l[1] value [0x%8.8x]\n" ,cncCounter.packetCount.l[1]));
    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :after update : byteCount.l[0] value [0x%8.8x] \n",cncCounter.byteCount.l[0]));
    __LOG_NO_LOCATION_META_DATA(("CNC : increment counter :after update : byteCount.l[1] value [0x%8.8x]\n" ,cncCounter.byteCount.l[1]));

    /* Write the 64-bit HW counter to HW format */
    snetCht3CncCounterHwWrite(cncFormat, regPtr, &cncCounter);

    return status;
}

/*******************************************************************************
*   snetCht3CncBlockReset
*
* DESCRIPTION:
*       Centralize counters block reset
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       block       - block index
*       entryStart  - start entry index
*       entryNum    - number of entry to reset
*       cncUnitIndex- the CNC unit index (0/1) (Sip5 devices)
*
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetCht3CncBlockReset
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 block,
    IN GT_U32 entryStart,
    IN GT_U32 entryNum,
    IN GT_U32 cncUnitIndex
)
{
    GT_U32 regAddr;                 /* register address */
    GT_U32 * regPtr;                /* register data pointer */
    GT_U32 clearByRead;             /* enable setting of a configurable value
                                       to the Counters after read by the CPU */
    GT_U32 entry;                   /* CNC block entry index */
    GT_U32 totalEntries;

    totalEntries = entryStart + entryNum;

    if ( entryNum < 1 ||
         totalEntries > devObjPtr->cncBlockMaxRangeIndex)
    {
        skernelFatalError("Wrong block entry start index or number of entries\n");
        return;
    }

    smemRegFldGet(devObjPtr, SMEM_CHT3_CNC_GLB_CONF_REG(devObjPtr,cncUnitIndex), 1, 1, &clearByRead);
    if (clearByRead == 0)
    {
        return;
    }

    /* pointer to CNC default value */
    regPtr = smemMemGet(devObjPtr, SMEM_CHT3_CNC_ROC_WORD0_REG(devObjPtr,cncUnitIndex));

    /* Get start address of entry in the block */
    regAddr = SMEM_CHT3_CNC_BLOCK_COUNTER_TBL_MEM(devObjPtr,block, entryStart , cncUnitIndex);

    for (entry = entryStart; entry < totalEntries; entry++, regAddr += (CNC_BLOCK_ENTRY_WORDS_CNS*4))
    {
        /* copy from the 'Default value' into the entry */
        smemMemSet(devObjPtr, regAddr, regPtr, CNC_BLOCK_ENTRY_WORDS_CNS);
    }
}


/*******************************************************************************
*   cncByteCountGet
*
* DESCRIPTION:
*       CNC byte count get
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       clientInfoPtr - pointer to CNC client info.
*
* OUTPUTS:
*
* RETURNS:
*       value to be incremented (bytes of L2/L2/packets)
*
*******************************************************************************/
static GT_STATUS cncByteCountGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(cncByteCountGet);

    GT_U32  cncFinalSizeToCount;/* final size to count */
    GT_U32  byteCount,l2HeaderSize,l3ByteCount,egressByteCount;
    GT_U32  dsaBytes;/* number of dsa bytes*/
    GT_BOOL forceIngressByteCountLogic;/* indication to use 'ingress logic' even for egress pipe units */
    GT_BOOL unitIsHa;/* is the client is from HA unit */

    __LOG(("CNC : Start calculate the byte count \n"));

    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        switch(clientInfoPtr->client)
        {
            case SNET_CNC_CLIENT_EPCL_ACTION_0_E:
                /* the EPCL unit never knew the ingress byte count */
                forceIngressByteCountLogic = GT_FALSE;
                break;
            default:
                forceIngressByteCountLogic = GT_TRUE;
                break;
        }
    }
    else
    {
        forceIngressByteCountLogic = GT_FALSE;
    }

    switch(clientInfoPtr->client)
    {
        case SNET_CNC_CLIENT_ARP_INDEX_E :
        case SNET_CNC_CLIENT_TUNNEL_START_INDEX_E :
        case SNET_CNC_CLIENT_TARGET_EPORT_E :
            unitIsHa = GT_TRUE;
            break;
        default:
            unitIsHa = GT_FALSE;
            break;
    }

    if(descrPtr->egressByteCount)
    {
        __LOG_PARAM(descrPtr->haToEpclInfo.paddingZeroForLess64BytesLength);
        __LOG_PARAM(descrPtr->egressByteCount);

        __LOG(("CNC : for Egress Pipe after packet length and format is known \n"));

        if(descrPtr->haToEpclInfo.paddingZeroForLess64BytesLength)
        {
            __LOG(("CNC : use byte count without MAC padding \n"));
        }

        egressByteCount = descrPtr->egressByteCount -
            descrPtr->haToEpclInfo.paddingZeroForLess64BytesLength;

    }
    else
    {
        egressByteCount = 0;

        __LOG(("CNC : for Ingress pipe or Egress Pipe before egress packet length and format is known \n"));
    }

    __LOG_PARAM(forceIngressByteCountLogic);
    __LOG_PARAM(descrPtr->byteCount);
    __LOG_PARAM(descrPtr->tunnelTerminated);
    __LOG_PARAM(descrPtr->tunnelStart);

    if(egressByteCount == 0 ||
       forceIngressByteCountLogic == GT_TRUE)
    {
        if(descrPtr->tunnelTerminated == 0 ||
           clientInfoPtr->client == SNET_CNC_CLIENT_TUNNEL_START_INDEX_E)
        {
            byteCount = descrPtr->byteCount;

            if(clientInfoPtr->client == SNET_CNC_CLIENT_TUNNEL_START_INDEX_E)
            {
                __LOG(("CNC : Tunnel start packet use ingress L2 byte count [%d] (using descrPtr->byteCount) \n",
                    byteCount));
            }
            else
            {
                __LOG(("CNC : non TT (non tunnel terminated) packet use L2 byte count [%d]  \n",
                    byteCount));
            }


            if((clientInfoPtr->client >= SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E ||
                clientInfoPtr->client <= SNET_CNC_CLIENT_IPCL_LOOKUP_2_ACTION_3_E) &&
                descrPtr->marvellTagged &&
                clientInfoPtr->byteCntMode == SMEM_CHT3_CNC_BYTE_CNT_L2_MODE_E)
            {
                dsaBytes = 4 * (descrPtr->marvellTaggedExtended + 1);

                if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    __LOG(("CNC : IPCL : L2 counting mode ignore DSA [%d] bytes  \n",
                        dsaBytes));
                    /* PCL : L2 counting check whether the packet is DSA tagged or not. */
                    byteCount -= dsaBytes;
                }
                else  /* according to ebook this is diff from sip 4.0 */
                {
                    __LOG(("CNC : IPCL : L2 counting mode NOT ignore DSA [%d] bytes \n",
                        dsaBytes));
                }
            }
        }
        else
        {
            byteCount = descrPtr->origByteCount;

            __LOG(("CNC : TT (tunnel terminated) packet use L2 'orig' byte count [%d]  \n",
                byteCount));
        }

        l2HeaderSize = descrPtr->l2HeaderSize;

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && descrPtr->tunnelTerminated &&
            descrPtr->innerPacketType == SKERNEL_INNER_PACKET_TYPE_ETHERNET_WITH_CRC)
        {
            __LOG(("CNC : for diff between L3 and L2 ignore 4 CRC bytes of TT with Ethernet passenger (beside CRC of packet) \n"));
            l2HeaderSize += 4;/* this is the CRC of the passenger */
        }

        l3ByteCount = byteCount - l2HeaderSize;

    }
    else
    {
        /* egress pipe (HA,EPCL) ... we need to use the egress byte count */
        byteCount       = egressByteCount;

        /* calc the L3 mode */

        if(descrPtr->tunnelTerminated || descrPtr->tunnelStart == 0)
        {
            l2HeaderSize = descrPtr->haToEpclInfo.l3StartOffsetPtr -
                           descrPtr->haToEpclInfo.macDaSaPtr;

            if(descrPtr->tunnelTerminated)
            {
                __LOG(("CNC : use L3 of the passenger \n"));
                if(descrPtr->innerPacketType == SKERNEL_INNER_PACKET_TYPE_ETHERNET_WITH_CRC)
                {
                    __LOG(("CNC : L3 of passenger ignore 4 CRC bytes of Ethernet passenger (beside CRC of packet) \n"));
                    l2HeaderSize += 4;/* this is the CRC of the passenger */
                }
            }
            else /* non TS and non TT */
            {
                __LOG(("CNC : use L3 of the packet \n"));
            }

        }
        else /* descrPtr->tunnelStart == 1 && descrPtr->tunnelTerminated == 0 */
        {
            l2HeaderSize = descrPtr->haToEpclInfo.l3StartOffsetPtr - /* the L3 of passenger */
                           descrPtr->haToEpclInfo.tunnelStartL2StartOffsetPtr;/* the start of egress packet */
            __LOG(("CNC : use L3 of the passenger of the TS (tunnel start) \n"));
            /*for TS that is not with ethernet passenger this is NULL */
            /*for TS that is     with ethernet passenger this is the mac da of the passenger */
            /*for non-TS that this is the mac da of the packet */
            if(descrPtr->haToEpclInfo.macDaSaPtr)
            {
                /* the TS with Ethernet passenger ... need to ignore 4 bytes of CRC of passenger */
                __LOG(("CNC : L3 of TS with Ethernet passenger is without CRC of passenger(beside CRC of packet) \n"));
                l2HeaderSize += 4;
            }
        }

        l3ByteCount  = byteCount - l2HeaderSize;

        __LOG(("CNC : egress packet use L2 byte count [%d] (using descrPtr->egressByteCount) \n",
            byteCount));
        if(unitIsHa == GT_TRUE &&
           devObjPtr->errata.cncHaIgnoreCrc4BytesInL2ByteCountMode &&
           descrPtr->haToEpclInfo.paddingZeroForLess64BytesLength == 0)/* if missed padding the CRC is ignored anyway */
        {
            /* BC is 4 bytes less then in egress packet. I assume that CRC is not added to the BC */
            byteCount -= 4;
            __LOG(("CNC : NOTE : ERROR : HA unit ignores 4 bytes of CRC \n"));
        }
    }

    /* For L3 byte count mode the 4 L2 CRC bytes are also subtract */
    l3ByteCount -= 4;

    __LOG_PARAM(byteCount);
    __LOG_PARAM(l3ByteCount);

    /* Calculate packet size according to client byte count mode */
    switch (clientInfoPtr->byteCntMode)
    {
        case SMEM_CHT3_CNC_BYTE_CNT_L2_MODE_E:
            cncFinalSizeToCount = byteCount;
            __LOG(("CNC : byte count L2 size=[%d] \n",
                cncFinalSizeToCount));
            break;
        case SMEM_CHT3_CNC_BYTE_CNT_L3_MODE_E:
            cncFinalSizeToCount = l3ByteCount;
            __LOG(("CNC : byte count L3 size=[%d] \n",
                cncFinalSizeToCount));
            break;
        case SMEM_CHT3_CNC_BYTE_CNT_PACKETS_MODE_E:
            cncFinalSizeToCount = 1;
            __LOG(("Packet count mode \n "));
            break;
        default:
            skernelFatalError("snetCht3CncBlockSet: wrong byte count mode %d\n", clientInfoPtr->byteCntMode);
            return GT_FAIL; /* should never happened*/
    }

    __LOG(("CNC : Ended calculate the byte count [%d] \n",
        cncFinalSizeToCount));

    return cncFinalSizeToCount;

}

/*******************************************************************************
*   snetCht3CncBlockSet
*
* DESCRIPTION:
*       Centralize counters block set
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       clientInfoPtr - pointer to CNC client info.
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK               - counter was successfully set
*       GT_NOT_SUPPORTED    - CNC feature not supported
*       GT_EMPTY            - all ranges for client are zero
*
*******************************************************************************/
static GT_STATUS snetCht3CncBlockSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncBlockSet);

    GT_U32 regAddr;                 /* register address */
    GT_U32 * regPtr;                /* register data pointer */
    GT_U32 clientIndex;             /* CNC client index inside block */
    GT_U32 block;                   /* CNC block index */
    GT_U32 numOfPackets;            /* number of packets to count */
    SNET_CHT3_CNC_COUNTER_STC cncCounter;
                                    /* CNC counter value */
    CNC_COUNTER_FORMAT_ENT cncFormat;
                                    /* CNC counter format */
    GT_U32 size;                    /* frame header size */
                                    /* CNC range bitmap binded to the block */
    GT_STATUS status;
    GT_U32 wrapArroundEn;           /* enable/disable counter wraparound */
    GT_U32 i;                       /* loop index */
    GT_U32 rangeIndex;              /* counter index range */
    GT_U32 cncUnitIndex = clientInfoPtr->cncUnitIndex;/*the CNC unit index (0/1) (Sip5 devices)*/
    SNET_CNC_CLIENT_E client = clientInfoPtr->client;
    GT_CHAR*    clientNamePtr = clientInfoPtr->clientsNamePtr;

    __LOG(("Cnc client name : [%s] \n",
        clientNamePtr));

    /* Get client range bitmaps for all CNC blocks */
    status = snetCht3CncClientRangeBitmapGet(devObjPtr, clientInfoPtr);
    if (status != GT_OK)
    {
        __LOG(("the client not enabled for counting \n"));
        return status;
    }

    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        clientInfoPtr->index = clientInfoPtr->index & 0x3FFF;/*14bits*/
    }
    else
    {
        /* SIP5 supports 16 bits of counters' index */
        clientInfoPtr->index = clientInfoPtr->index & 0xFFFF;/*16bits*/
    }
    __LOG(("selected index[%d] \n",
        clientInfoPtr->index));

    /* Set counter increment to one packet */
    cncCounter.packetCount.l[0] = 1;
    cncCounter.packetCount.l[1] = 0;

    /* Enable Wraparound in the Packet and Byte Count Counters */
    smemRegFldGet(devObjPtr, SMEM_CHT3_CNC_GLB_CONF_REG(devObjPtr,cncUnitIndex), 0, 1, &wrapArroundEn);
    if(wrapArroundEn)
    {
        __LOG(("Enable Wraparound in the Packet and Byte Count Counters \n"));
    }

    /* Get range index for specific client */
    rangeIndex = clientInfoPtr->index / devObjPtr->cncBlockMaxRangeIndex;
    __LOG(("range index is [%d] \n",
        rangeIndex));

    __LOG(("Set client byte count mode [%s] \n",
            ((clientInfoPtr->byteCntMode == SMEM_CHT3_CNC_BYTE_CNT_L2_MODE_E) ?
                    "SMEM_CHT3_CNC_BYTE_CNT_L2_MODE_E" :
                    "SMEM_CHT3_CNC_BYTE_CNT_L3_MODE_E")
        ));

    size = cncByteCountGet(devObjPtr,descrPtr,clientInfoPtr);

    numOfPackets = 1;

    /* for egress vlan client packet number is multiplied */
    if( client == SNET_CNC_CLIENT_EGRESS_VLAN_PASS_DROP_E )
    {
        if( clientInfoPtr->userDefined != 0 )
        {
            numOfPackets = clientInfoPtr->userDefined;
            __LOG(("for egress vlan client packet number is multiplied [%d] \n",
                numOfPackets));
        }
    }

    for (block = 0; block < (GT_U32)CNC_CNT_BLOCKS(devObjPtr); block++)
    {
        /* Range binded to specific CNC block */
        if (0 == snetFieldValueGet(clientInfoPtr->rangeBitmap[block].l, rangeIndex, 1))
        {
            continue;
        }
        /* Get client index inside CNC block */
        clientIndex = clientInfoPtr->index % devObjPtr->cncBlockMaxRangeIndex;
        __LOG(("client index inside CNC block[%d]",
            clientIndex));

        /* Get memory address inside CNC block */
        regAddr = SMEM_CHT3_CNC_BLOCK_COUNTER_TBL_MEM(devObjPtr,block, clientIndex,cncUnitIndex);

        /* Read block data pointer */
        regPtr = smemMemGet(devObjPtr, regAddr);

        CNV_U32_TO_U64(size, cncCounter.byteCount);

        /* Get format of CNC counter */
        snetCht3CncCounterFormatGet(devObjPtr, block, &cncFormat,cncUnitIndex);

        for( i = 0 ; i < numOfPackets ; i++ )
        {
            /* Increment counters field */
            __LOG(("Increment counters field"));
            if (snetCht3CncIncrement(regPtr, block, cncFormat, &cncCounter, wrapArroundEn) == GT_FULL)
            {
                snetCht3CncBlockWrapStatusSet(devObjPtr, block, clientIndex,cncUnitIndex);
                /* if wraparound is disabled no need to count further packets */
                /* since no update to the counter will be done.               */
                if( wrapArroundEn == 0 )
                {
                    break;
                }
            }
        }

        /* Write block data pointer */
        smemMemSet(devObjPtr, regAddr, regPtr, CNC_BLOCK_ENTRY_WORDS_CNS);
    }

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncTrigger
*
* DESCRIPTION:
*       Trigger CNC event for specified client
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       clientInfoPtr - pointer to CNC client info.
*
* OUTPUTS:
*       clientInfoPtr - pointer to CNC client info.
*
* RETURNS:
*       GT_OK               - CNC event triggered
*       GT_NOT_SUPPORTED    - CNC feature not supported
*       GT_FAIL             - CNC event is not triggered
*       GT_BAD_PARAM        - wrong CNC client index
*
*******************************************************************************/
static GT_STATUS snetCht3CncTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncTrigger);

    /* save the bit index */
    clientInfoPtr->clientHwBit = convertClientToBitIndex(devObjPtr,clientInfoPtr->client);

    if(clientInfoPtr->clientHwBit >= 32)
    {
        skernelFatalError("snetCht3CncTrigger: Error calculating bit index \n");
    }
    else
    if (SMEM_U32_GET_FIELD(devObjPtr->cncClientSupportBitmap, clientInfoPtr->clientHwBit, 1) == 0)
    {
        skernelFatalError("snetCht3CncTrigger: CNC client is not supported\n");
    }

    if(gCncClientTrigFuncArr[clientInfoPtr->client] == NULL)
    {
        skernelFatalError("snetCht3CncTrigger: CNC client is not BOUND to function \n");
        return GT_NOT_SUPPORTED;
    }

    __LOG(("Call CNC trigger function for client [%s] \n", clientInfoPtr->clientsNamePtr));

    return gCncClientTrigFuncArr[clientInfoPtr->client](devObjPtr, descrPtr, clientInfoPtr);
}

/*******************************************************************************
*   getFirstBitInValue
*
* DESCRIPTION:
*       get first bit in value
*
* INPUTS:
*       value   - the value to get the first bit
*
* OUTPUTS:
*
* RETURNS:
*       the 'bit index' of the firts bit that is 'set' in 'value'
* COMMENTS:
*  based on prvCpssPpConfigBitmapFirstActiveBitGet
*
*******************************************************************************/
static GT_U32   getFirstBitInValue(IN GT_U32    value)
{
    GT_U32 bmp;       /* bitmap             */
    GT_U32 bmp4;      /* bitmap 4 LSBs      */
    GT_U32 ii;         /* loop index         */
    GT_U32  firstBit;

    bmp = value;

    /* search the first non-zero bit in bitmap */
    /* loop on 8 4-bit in 32-bit bitmap        */
    for (ii = 0; (ii < 8); ii++, bmp >>= 4)
    {
        bmp4 = bmp & 0x0F;

        if (bmp4 == 0)
        {
            /* non-zero bit not found */
            /* search in next 4 bits  */
            continue;
        }

        /* non-zero bit found                                            */
        /* the expression below is the fast version of                   */
        /* (i * 4) + ((PRV_FIRST_NON_ZERO_BIT_CNS >> (bmp4 * 2)) & 0x03) */
        firstBit =
            (ii << 2) + ((PRV_FIRST_NON_ZERO_BIT_CNS >> (bmp4 << 1)) & 0x03);
        return firstBit;
    }

    /* occurs only if bmp == 0 */
    return SMAIN_NOT_VALID_CNS;

}

/*******************************************************************************
*   snetCht3CncFastDumpFuncPtr
*
* DESCRIPTION:
*       Process upload CNC block demanded by CPU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       cncTrigPtr  - pointer to CNC Fast Dump Trigger Register
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetCht3CncFastDumpFuncPtr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * cncTrigPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncFastDumpFuncPtr);

    GT_U32 regAddr;                 /* Register address */
    GT_U32 entry, * entryPtr;       /* CNC entry index and pointer */
    GT_U32 block;                   /* Trigger block index */
    GT_U32 intrNo;                  /* Interrupt cause bit */
    GT_BOOL doInterrupt;            /* No interrupt while message not sent */
    GT_U32 numOfCncInFu;            /* number of CNC counters in single FU message */
    GT_U32 cncUnitIndex;/* the CNC unit index (0/1) (Sip5 devices)*/

    block = getFirstBitInValue(cncTrigPtr[0]);

    if(block == SMAIN_NOT_VALID_CNS ||
       (0 == SMEM_U32_GET_FIELD(devObjPtr->cncClientSupportBitmap, block, 1)))
    {
        /* not triggered any active client */
        return;
    }

    cncUnitIndex = cncTrigPtr[1];

    /* Get entryPtr according to entry index */
    regAddr = SMEM_CHT3_CNC_BLOCK_COUNTER_TBL_MEM(devObjPtr,block, 0,cncUnitIndex);
    /* Get start block data pointer */
    entryPtr = smemMemGet(devObjPtr, regAddr);

    /* Enable first time interrupt */
    doInterrupt = GT_TRUE;

    /* send each time 2 counters regardless to the alignment (as this is the minimal alignment) */
    numOfCncInFu = 2;

    /* DMA message size is 4 words. Iterate memory space with step of 4 words */
    __LOG(("Send CNC messages to FUQ - start \n"));

    entry = devObjPtr->cncUploadInfo.indexInCnc;
    entryPtr += (CNC_BLOCK_ENTRY_WORDS_CNS * numOfCncInFu) * entry;

    for (/**/; entry < devObjPtr->cncBlockMaxRangeIndex / numOfCncInFu; entry++, entryPtr+= CNC_BLOCK_ENTRY_WORDS_CNS * numOfCncInFu)
    {
        /* Send FU message to CPU */
        while(GT_FALSE == snetCht3CncSendMsg2Cpu(devObjPtr, block, entryPtr,
                                                 &doInterrupt,cncUnitIndex))
        {
            /* Wait for SW to free buffers */
            if(oldWaitDuringSkernelTaskForAuqOrFua)
            {
                SIM_OS_MAC(simOsSleep)(SNET_CHT3_NO_CNC_DUMP_BUFFERS_SLEEP_TIME);
            }
            else
            {
                /* the action is broken and will be continued in new message */
                /* save the index */
                devObjPtr->needResendMessage = 1;
                devObjPtr->cncUploadInfo.indexInCnc = entry;
                return;
            }
        }
        /* Enable interrupt when FU message has sent successfully */
        doInterrupt = GT_TRUE;
    }

    devObjPtr->cncUploadInfo.indexInCnc = 0;

    __LOG(("Send CNC messages to FUQ - ended \n"));

    /* Set configurable value to the Counters after read by the CPU */
    __LOG(("Set configurable value to the Counters after read by the CPU"));
    snetCht3CncBlockReset(devObjPtr, block, 0, devObjPtr->cncBlockMaxRangeIndex , cncUnitIndex);

    /* Clear upload message trigger bit when the action is completed  */
    regAddr = SMEM_CHT3_CNC_FAST_DUMP_TRIG_REG(devObjPtr,cncUnitIndex);
    smemRegFldSet(devObjPtr, regAddr, block, 1, 0);


    intrNo = SMEM_CHT3_CNC_DUMP_BLOCK_FINISHED;
    snetChetahDoInterrupt(devObjPtr,
                        SMEM_CHT3_CNC_INTR_CAUSE_REG(devObjPtr,cncUnitIndex),
                        SMEM_CHT3_CNC_INTR_MASK_REG(devObjPtr,cncUnitIndex),
                        intrNo,
                        SMEM_CHT_CNC_ENGINE_INT(devObjPtr,cncUnitIndex));
}

/*******************************************************************************
*   snetCht3SendCncMsg2Cpu
*
* DESCRIPTION:
*       Send FDB Upload message to CPU
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       block               - CNC block index
*       cncDumpPtr          - CNC block pointer
*       doInterruptPtr      - process interrupt flag pointer
*      cncUnitIndex- the CNC unit index (0/1) (Sip5 devices , legacy device only 1 unit)
*
* OUTPUT:
*       doInterruptPtr      - process interrupt flag pointer
*
*******************************************************************************/
static GT_BOOL snetCht3CncSendMsg2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 block,
    IN GT_U32 * cncDumpPtr,
    INOUT GT_BOOL * doInterruptPtr,
    IN GT_U32   cncUnitIndex
)
{
    GT_U32 regAddr;                 /* Register address */
    GT_U32 fldValue;                /* Register's field value */
    GT_BOOL status;                 /* Function return value */
    GT_U32 intrNo;                  /* Interrupt cause bit */

    status = GT_TRUE;

    regAddr = SMEM_CHT_FU_QUE_BASE_ADDR_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 31, 1, &fldValue);

    /* Enable forwarding address update message to a separate queue */
    if (fldValue)
    {
        /* The device initiates a PCI Master transaction for each FU message it
           forwards to the CPU */
        status = snetChtL2iPciFuMsg(devObjPtr, cncDumpPtr,(2 * CNC_BLOCK_ENTRY_WORDS_CNS)/*always 2 CNC counters*/);

        /* if the device started 'soft reset' we not care if NA not reached the CPU
           and we must not 'wait until success' so declare success !!! */
        if(devObjPtr->needToDoSoftReset && status == GT_FALSE)
        {
            status = GT_TRUE;
        }

        if (status == GT_FALSE && (*doInterruptPtr) ==  GT_TRUE)
        {
            intrNo = SMEM_CHT3_CNC_BLOCK_RATE_LIMIT_FIFO(block);
            snetChetahDoInterrupt(devObjPtr,
                                SMEM_CHT3_CNC_INTR_CAUSE_REG(devObjPtr,cncUnitIndex),
                                SMEM_CHT3_CNC_INTR_MASK_REG(devObjPtr,cncUnitIndex),
                                intrNo,
                                SMEM_CHT_CNC_ENGINE_INT(devObjPtr,cncUnitIndex));

            *doInterruptPtr = GT_FALSE;
        }
    }

    return status;
}

/*******************************************************************************
*   snetCht3CncClientRangeBitmapGet
*
* DESCRIPTION:
*       CNC range bitmap for client from CNC block configuration register
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       clientInfoPtr   - pointer to CNC client info.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK           - client range was successfully get
*       GT_EMPTY        - all ranges for client are zero
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientRangeBitmapGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientRangeBitmapGet);

    GT_U32 block, *regPtr;
    GT_BOOL rangeEmpty = GT_TRUE;
    GT_U32 wordBit;
    GT_U32 cncUnitIndex = clientInfoPtr->cncUnitIndex;/*the CNC unit index (0/1) (Sip5 devices)*/
    GT_U32  numBlocks = CNC_CNT_BLOCKS(devObjPtr);
    GT_U32  clientHwBit = clientInfoPtr->clientHwBit;


    __LOG(("check which block enabled for counting (out of [%d] blocks) \n" ,
        numBlocks));

    for (block = 0; block < numBlocks; block++)
    {
        regPtr = smemMemGet(devObjPtr,
                       SMEM_CHT_ANY_CNC_BLOCK_CNF_REG(devObjPtr, block, clientHwBit,cncUnitIndex));

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(0 == SMEM_U32_GET_FIELD(regPtr[0], clientHwBit, 1))
            {
                continue;
            }

            /* 64 ranges of 1K counters.
               The ranges mapped to the block for all clients  */
            smemRegGet(devObjPtr,
                SMEM_LION3_CNC_BLOCK_COUNT_EN_0_REG(devObjPtr,block,cncUnitIndex),
                &clientInfoPtr->rangeBitmap[block].l[0]);
            smemRegGet(devObjPtr,
                SMEM_LION3_CNC_BLOCK_COUNT_EN_1_REG(devObjPtr,block,cncUnitIndex),
                &clientInfoPtr->rangeBitmap[block].l[1]);
        }
        else
        /* Lion B0 and above */
        if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
        {
            /* Lion and above - 64 ranges of 512 counters.
               The ranges mapped to the block for all clients  */
            if(0 == SMEM_U32_GET_FIELD(regPtr[0], clientHwBit, 1))
            {
                continue;
            }
            clientInfoPtr->rangeBitmap[block].l[0] = regPtr[1];
            clientInfoPtr->rangeBitmap[block].l[1] = regPtr[2];
        }
        else
        {
            wordBit = (clientHwBit % 2) * 13;
            /* DxCh3, DxChXCat - 8 ranges of 2048 counters */
            if(0 == SMEM_U32_GET_FIELD(regPtr[0], wordBit, 1))
            {
                continue;
            }
            clientInfoPtr->rangeBitmap[block].l[0] =
                SMEM_U32_GET_FIELD(regPtr[0], wordBit + 1, 12);
        }

        if(clientInfoPtr->rangeBitmap[block].l[0] ||
           clientInfoPtr->rangeBitmap[block].l[1])
        {
            __LOG(("Block [%d] is enabled with ranges bmp [0x%8.8x] [0x%8.8x]\n",
                block,
                clientInfoPtr->rangeBitmap[block].l[0],
                clientInfoPtr->rangeBitmap[block].l[1]));

            rangeEmpty = GT_FALSE;
        }
        else
        {
            __LOG(("Block [%d] is enabled but no range valid (so ignored) \n",
                block));
        }
    }

    return (rangeEmpty == GT_TRUE) ? GT_EMPTY : GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientL2L3VlanIngrTrigger
*
* DESCRIPTION:
*       CNC L2 and L3 VLAN ingress client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* OUTPUTS:
*       clientRange     - CNC range bitmap binded to the block
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientL2L3VlanIngrTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientL2L3VlanIngrTrigger);

    GT_U32 fldValue;
    smemRegFldGet(devObjPtr,
                  SMEM_CHT3_CNC_VLAN_EN_REG(devObjPtr, descrPtr->localDevSrcPort),
                  descrPtr->localDevSrcPort % 32, 1, &fldValue);
    if (fldValue == 0)
    {
        __LOG(("[%s] Counting Disabled 'per port' : ingress physical port disabled for vlan/eport ingress counting \n",
            clientInfoPtr->clientsNamePtr));

        /* ingress physical port not enabled for vlan/eport ingress counting */
        return GT_FAIL;
    }

    clientInfoPtr->index = descrPtr->eVid;
    smemRegFldGet(devObjPtr, SMEM_CHT3_CNC_COUNT_MODE_REG(devObjPtr), 3, 1,
                  &fldValue);

    /* Set byte count mode for L2 and L3 VLAN ingress client */
    clientInfoPtr->byteCntMode = fldValue;

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientPclLookUpIngrTrigger
*
* DESCRIPTION:
*       CNC PCL lookup ingress client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* OUTPUTS:
*       clientRange     - CNC range bitmap binded to the block
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientPclLookUpIngrTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    GT_U32 cntModeBit;
    GT_U32 fldValue;

    clientInfoPtr->index = clientInfoPtr->userDefined;

    /* Count mode bit:  0, 1 or 2 */

    switch(clientInfoPtr->client)
    {
        case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_0_E:
        case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_1_E:
        case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_2_E:
        case SNET_CNC_CLIENT_IPCL_LOOKUP_0_ACTION_3_E:
            /* action 0 */
            cntModeBit = 0;
            break;
        case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_0_E:
        case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_1_E:
        case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_2_E:
        case SNET_CNC_CLIENT_IPCL_LOOKUP_1_ACTION_3_E:
            /* action 1 */
            cntModeBit = 1;
            break;
        default:
            /* action 2 */
            cntModeBit = 2;
            break;
    }

    smemRegFldGet(devObjPtr, SMEM_CHT3_CNC_COUNT_MODE_REG(devObjPtr),
                  cntModeBit, 1, &fldValue);

    /* Set byte count mode for PCL lookup ingress client */
    clientInfoPtr->byteCntMode = fldValue;

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientVlanPassDropIngrTrigger
*
* DESCRIPTION:
*       CNC VLAN pass/drop ingress client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* OUTPUTS:
*       clientRange     - CNC range bitmap binded to the block
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientVlanPassDropIngrTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientVlanPassDropIngrTrigger);

    GT_U32 enable, dropBit, *regPtr;
    GT_U32 cncVlanSelection;/* CNC VLAN Selection */
    GT_U32 vlanId;  /* the vlanId for the CNC indexing */
    GT_U32 vidNumBits;/* number of bits from the vlanId to use */
    GT_U32 cncFromCpuEnOffset; /* offset of the cnc_from_cpu_en bit */
    GT_U32 byteCountModeOffset; /* offset of EQ Clients Byte Count mode */

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
    {
        __LOG(("[%s]Counting Disabled : TO_CPU packets should NOT be counted \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    /* TO_CPU packets should NOT be counted (fix CQ Bugs00124345) */
    if (descrPtr->useVidx == 0 && descrPtr->trgEPort == SNET_CHT_CPU_PORT_CNS)
    {
        __LOG(("[%s]Counting Disabled : forward packets to port 63 (CPU) should NOT be counted \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        cncFromCpuEnOffset = 14;
        byteCountModeOffset = 13;
    }
    else
    {
        cncFromCpuEnOffset = 15;
        byteCountModeOffset = 14;
    }

    regPtr = smemMemGet(devObjPtr, SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr));
    if (descrPtr->marvellTagged &&
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FROM_CPU_E)
    {
        enable = SMEM_U32_GET_FIELD(*regPtr, cncFromCpuEnOffset, 1);
        if (enable == 0)
        {
            __LOG(("[%s]Counting globally Disabled : 'FROM_CPU' Marvell tagged \n",
                clientInfoPtr->clientsNamePtr));
            return GT_FAIL;
        }
    }

    dropBit = clientInfoPtr->userDefined;
    __LOG_PARAM(dropBit);

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.eq)
    {
        /*CNC VLAN Selection*/
        cncVlanSelection = SMEM_U32_GET_FIELD(*regPtr, 17, 2);

        if(cncVlanSelection == 0)
        {
            vlanId = descrPtr->vid0Or1AfterTti;/* OrigVID; The original incoming VID is used as the CNC index */
            vlanId &= 0xFFF;/*12 bits*/

            __LOG(("VLAN Selection mode : OrigVID(12bits); Use the original incoming VID [0x%x] \n",
                vlanId));
        }
        else if(cncVlanSelection == 1)
        {
            vlanId = descrPtr->eVid;
            vlanId &= 0x3FFF;/*14 bits*/

            __LOG(("(ingress)VLAN Selection mode : eVLAN((14bits)); Use the eVLAN [0x%x] \n",
                vlanId));

        }
        else if(cncVlanSelection == 2)
        {
            vlanId = descrPtr->vid1;
            vlanId &= 0xFFF;/*12 bits*/

            __LOG(("(ingress)VLAN Selection mode : Tag1(12bits); Use the Tag1 VID [0x%x] \n",
                vlanId));
        }
        else
        {
            vlanId = 0;/* not supported option*/
            __LOG(("(ingress)VLAN Selection mode : unknown; Use the VID = 0 \n"));
        }

        vidNumBits = 14;
    }
    else
    {
        vlanId = descrPtr->ingressPipeVid;/* The index passed to the bound
                counter blocks is the packet VLAN assignment after the ingress
                Policy engine */
        vidNumBits = 12;

        __LOG(("The VLAN used is the VLAN assignment after the ingress Policy : the CNC index [%d] \n",
            vlanId));
    }

    clientInfoPtr->index = (dropBit << vidNumBits) | SMEM_U32_GET_FIELD(vlanId,0,vidNumBits);

    clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(*regPtr, byteCountModeOffset, 1);

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientVlanPassDropEgrTrigger
*
* DESCRIPTION:
*       CNC VLAN pass/drop egress client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* OUTPUTS:
*       clientRange     - CNC range bitmap binded to the block
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientVlanPassDropEgrTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientVlanPassDropEgrTrigger);

    GT_U32 cncEgressVlanClientCountMode, outPorts, dropBit = 0, *regPtr;
    GT_U32 cncVlanSelection;/* CNC VLAN Selection */
    GT_U32 vlanId;  /* the vlanId for the CNC indexing */
    GT_U32 vidNumBits;/* number of bits from the vlanId to use */

    outPorts = clientInfoPtr->userDefined;
    regPtr = smemMemGet(devObjPtr, SMEM_CHT3_CNC_MODES_REG(devObjPtr));
    if (outPorts == 0)
    {
        __LOG(("No Replication will egress (all replications filtered) \n "));
        /* <CNC Egress VLAN Client Count Mode>
            Selects the type of packets counted by the CNC Egress VLAN Client
           according to the packet's VLAN Egress filtering status.
           0 = CountEgressFilterAndTailDrop
           1 = CountEgressFilterOnly
           2 = CountTailDropOnly
        */

        cncEgressVlanClientCountMode = SMEM_U32_GET_FIELD(*regPtr, 4, 2);

        __LOG_PARAM(cncEgressVlanClientCountMode);

        if (cncEgressVlanClientCountMode == 2 || cncEgressVlanClientCountMode == 3)
        {
            __LOG(("[%s]Counting Disabled : Simulation not Supports 'Tail Drop' so nothing to count \n",
                clientInfoPtr->clientsNamePtr));

            return GT_FAIL;
        }

        dropBit = 1;
    }

    __LOG_PARAM(dropBit);

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.eq)
    {
        /*CNC VLAN Selection*/
        cncVlanSelection = SMEM_U32_GET_FIELD(*regPtr, 10, 2);

        if(cncVlanSelection == 2) /*not same value as in EQ !!!*/
        {
            vlanId = descrPtr->vid0Or1AfterTti;/* OrigVID; The original incoming VID is used as the CNC index */
            vlanId &= 0xFFF;/*12 bits*/

            __LOG(("(egress)VLAN Selection mode : OrigVID(12bits); Use the original incoming VID [0x%x] \n",
                vlanId));
        }
        else if(cncVlanSelection == 0)/*not same value as in EQ !!!*/
        {
            vlanId = descrPtr->eVid;
            vlanId &= 0x3FFF;/*14 bits*/

            __LOG(("(egress)VLAN Selection mode : eVLAN((14bits)); Use the eVLAN [0x%x] \n",
                vlanId));
        }
        else if(cncVlanSelection == 1)/*not same value as in EQ !!!*/
        {
            vlanId = descrPtr->vid1;
            vlanId &= 0xFFF;/*12 bits*/

            __LOG(("(egress)VLAN Selection mode : Tag1(12bits); Use the Tag1 VID [0x%x] \n",
                vlanId));
        }
        else
        {
            vlanId = 0;/* not supported option*/
            __LOG(("(egress)VLAN Selection mode : unknown; Use the VID = 0 \n"));
        }

        vidNumBits = 14;
    }
    else
    {
        vlanId = descrPtr->eVid;
        vidNumBits = 12;
    }


    clientInfoPtr->index = (dropBit << vidNumBits) | SMEM_U32_GET_FIELD(vlanId,0,vidNumBits);
    clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(*regPtr, 2, 1);

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientTxqQueuePassDropEgrTrigger
*
* DESCRIPTION:
*       CNC EQ pass/drop egress client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* OUTPUTS:
*       clientRange     - CNC range bitmap binded to the block
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientTxqQueuePassDropEgrTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientTxqQueuePassDropEgrTrigger);

    GT_U32 egrPort;
    GT_U32 fldValue;                        /* register's field value */
    GT_U32 globalPort;
    GT_U32 cncModeRegAddr;
    GT_U32 tailDrop = 0;/*no tail drop in simulation*/
    GT_U32 isCnMessage;

    egrPort = clientInfoPtr->userDefined;
    cncModeRegAddr = SMEM_CHT3_CNC_MODES_REG(devObjPtr);

    smemRegFldGet(devObjPtr, cncModeRegAddr, 0, 1,
                  &fldValue);
    /* Set client byte count mode for EQ pass/drop egress client */
    clientInfoPtr->byteCntMode = fldValue;

    /* Lion B0 and above */
    if (devObjPtr->txqRevision)
    {
        if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* Lion B0 uses Global port */
            globalPort = SMEM_CHT_GLOBAL_PORT_FROM_LOCAL_PORT_MAC(devObjPtr, egrPort);
            __LOG(("convert local port [%d] to  Global port [%d]",
                egrPort,globalPort));

            egrPort = globalPort;

            egrPort &= 0x3F;/*6 bits*/
        }
        else
        {
            /*egrPort was already set*/
        }

        /* Selects the counting mode of the CNC Egress Queue Client TailDropMode/CNMode */
        smemRegFldGet(devObjPtr, cncModeRegAddr, 8, 1, &fldValue);
        /* TailDropMode */
        if(fldValue == 0)
        {
            __LOG(("used Tail-Drop Counting Mode \n"));
            if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* index bit 10 is 0 at simulation since there are no tail drop */
                clientInfoPtr->index = (descrPtr->dp >> 1) << 11 | (tailDrop << 10) | egrPort << 4 |
                    descrPtr->tc << 1 | (descrPtr->dp & 1);
            }
            else
            {
                /* index bit 13 is 0 at simulation since there are no tail drop */
                clientInfoPtr->index = (tailDrop << 13) | egrPort << 5 |
                    descrPtr->tc << 2 | (descrPtr->dp);
            }
        }
        else
        {
            __LOG(("used CN Counting Mode (NOTE: forced to use packets count mode) \n"));

            __LOG(("Simulation currently not differ between CN and NON-CN messages .. so treat all as 'non CN' \n"));
            isCnMessage = 0;
            __LOG_PARAM(isCnMessage);

            if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* index bit 10 is 0 at simulation since there are no tail drop, bit 0 is 0 - None-CN Message Counter */
                clientInfoPtr->index = (tailDrop << 10) | egrPort << 4 | descrPtr->tc << 1;
            }
            else
            {
                /* index bit 12 is 0 at simulation since there are no tail drop, bit 0 is 0 - None-CN Message Counter */
                clientInfoPtr->index = (tailDrop << 12) | egrPort << 4 | descrPtr->tc << 1;
            }

            if(isCnMessage)
            {
                clientInfoPtr->index |= 1;/*set bit 0*/
            }

            /* the CN counts packets */
            clientInfoPtr->byteCntMode = SMEM_CHT3_CNC_BYTE_CNT_PACKETS_MODE_E;
        }
    }
    else
    {
        /*
        Text from FS BTS xCAT (xCAT) #322
        All the CNC indexes are defined in the functional to be 14 bits, so only bits 13:11 are "0".
        ------------------------------------------------------------------
        In (section 21.5.5.2), figure 88, Egress counters index format dp occupies 1 bit.
        The format should be as below (DP is extended to 2 bits)
        bits 1:0 = dp
        bits 4:2 = tc
        bits 9:5 = port
        bit 10 = drop/pass
        bits 15:11 = "0" */
        /* index bit 10 is 0 at simulation since there are no tail drop */
        clientInfoPtr->index = (egrPort << 5) | (descrPtr->tc << 2) | (descrPtr->dp);
    }

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientPclEgrTrigger
*
* DESCRIPTION:
*       CNC PCL egress client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientPclEgrTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    GT_U32 *regPtr;

    regPtr = smemMemGet(devObjPtr, SMEM_CHT3_EPCL_GLOBAL_REG(devObjPtr));

    clientInfoPtr->index = clientInfoPtr->userDefined;

    if(devObjPtr->supportEArch)
    {
        clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(*regPtr, 10, 1);
    }
    else
    {
        clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(*regPtr, 25, 1);
    }


    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientArpTblTrigger
*
* DESCRIPTION:
*       CNC ARP client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientArpTblTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientArpTblTrigger);

    GT_U32 fldValue;

    if(descrPtr->isNat)
    {
        clientInfoPtr->index = descrPtr->arpPtr;

        __LOG(("The NAT accessed to the ARP/TS table at index [%d] (from  descrPtr->arpPtr) \n" ,
            clientInfoPtr->index));

        /* The CNC clients ARP and NAT are muxed between them
            (since for a given packet the user can access for ARP entry or for NAT entry).
           This offset is added to the NAT when sending the pointer to the CNC. */
        smemRegFldGet(devObjPtr, SMEM_BOBCAT2_HA_NAT_CONFIGURATIONS_REG(devObjPtr), 0, 16,
                      &fldValue);
        __LOG(("This offset is added to the NAT when sending the pointer to the CNC [%d] \n", fldValue));
        clientInfoPtr->index += fldValue;
    }
    else
    {
        clientInfoPtr->index = descrPtr->arpPtr;
    }

    __LOG(("client index[%d] \n", clientInfoPtr->index));

    smemRegFldGet(devObjPtr, SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(devObjPtr), 0, 1,
                  &fldValue);

    __LOG(("Set byte count mode for ARP client [%d] \n", fldValue));
    clientInfoPtr->byteCntMode = fldValue;

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientHaTargetEPortTrigger
*
* DESCRIPTION:
*       CNC HA target EPort client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientHaTargetEPortTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientHaTargetEPortTrigger);

    GT_U32  regValue;

    clientInfoPtr->index = descrPtr->trgEPort;

    if(descrPtr->useVidx == 1 ||
       descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E ||
       descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
    {
        /* no counting */
        __LOG(("[%s]Counting Disabled : due to VIDX / TO_TRG_SNIFFER / FROM_CPU \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    if((0 == SKERNEL_IS_MATCH_DEVICES_MAC(descrPtr->trgDev, descrPtr->ownDev,
                                                  devObjPtr->dualDeviceIdEnable.ha))
            &&
            descrPtr->eArchExtInfo.assignTrgEPortAttributesLocally == 0)
    {
        /* no counting */
        __LOG(("[%s]Counting Disabled : due to 'not own dev' && 'not locally assigned' \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    /*Use Target ePort as CNC Index*/
    clientInfoPtr->index = descrPtr->trgEPort;

    smemRegGet(devObjPtr, SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(devObjPtr),&regValue);
    clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(regValue,2,1);

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientTunnelStartTrigger
*
* DESCRIPTION:
*       CNC tunnel start client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientTunnelStartTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    GT_U32  regValue;

    smemRegGet(devObjPtr, SMEM_CHT3_ROUTE_HA_GLB_CNF_REG(devObjPtr),&regValue);
    clientInfoPtr->index = descrPtr->tunnelPtr;
    clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(regValue,1,1);

    return GT_OK;
}

/*******************************************************************************
*   snetXCatCncClientTunnelTerminatedTrigger
*
* DESCRIPTION:
*       CNC tunnel termination client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetXCatCncClientTunnelTerminatedTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetXCatCncClientTunnelTerminatedTrigger);

    GT_U32 * regPtr;

    regPtr = smemMemGet(devObjPtr, SMEM_XCAT_A1_TTI_UNIT_GLB_CONF_REG(devObjPtr));

    /* Enables counting by the Ingress Counter engine */
    if (SMEM_U32_GET_FIELD(*regPtr, 3, 1) == 0)
    {
        __LOG(("[%s]Counting Globally Disabled \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    clientInfoPtr->index = clientInfoPtr->userDefined;

    /* Byte count mode - determines whether a packet size is determined by its L2 or L3 size */
    clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(*regPtr, 4, 1);

    return GT_OK;
}

/*******************************************************************************
*   snetCht3CncClientIpclSourceEPortTrigger
*
* DESCRIPTION:
*       CNC IPCL source EPort client
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetCht3CncClientIpclSourceEPortTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3CncClientIpclSourceEPortTrigger);

    GT_U32 fldValue;                /* Register's field value */


    /* The IPCL Source ePort counter is only triggered if SrcDev=OwnDev */
    if(descrPtr->srcDevIsOwn == 0)
    {
        __LOG(("[%s]Counting Disabled : source not from 'own device' \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    smemRegFldGet(devObjPtr,
                  SMEM_CHT3_CNC_VLAN_EN_REG(devObjPtr, descrPtr->localDevSrcPort),
                  descrPtr->localDevSrcPort % 32, 1, &fldValue);
    if (fldValue == 0)
    {
        /* per-port enable for VLAN counting is also applied also to the ePort counting */
        /* SAME registers as the 'snetCht3CncClientL2L3VlanIngrTrigger' */
        /* ingress physical port not enabled for vlan/eport ingress counting */
        __LOG(("[%s] Counting Disabled 'per port' : ingress physical port disabled for vlan/eport ingress counting \n",
            clientInfoPtr->clientsNamePtr));
        return GT_FAIL;
    }

    /* <Ingress ePort Count Mode> */
    smemRegFldGet(devObjPtr, SMEM_CHT3_CNC_COUNT_MODE_REG(devObjPtr), 5, 1,
                  &fldValue);

    /* Set byte count mode for L2 and L3 VLAN ingress client */
    clientInfoPtr->byteCntMode = fldValue;
    /*Use source ePort as CNC Index*/
    clientInfoPtr->index = clientInfoPtr->userDefined;

    return GT_OK;
}


/*******************************************************************************
*   snetLion3CncClientPreEgressPacketTypePassDropTrigger
*
* DESCRIPTION:
*       Sip5 only function.
*       This client allows counting packets based on their type;
*       separated counters are held for dropped or passed packets.
*       Counting is performed in the Pre-egress engine after all replications
*       and dropping mechanisms
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       clientInfoPtr   - pointer to CNC client info.
*
* RETURNS:
*       GT_OK           - client is triggered
*       GT_FAIL         - fail in client triggering
*
*******************************************************************************/
static GT_STATUS snetLion3CncClientPreEgressPacketTypePassDropTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT3_CNC_CLIENT_INFO_STC * clientInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLion3CncClientPreEgressPacketTypePassDropTrigger);

    GT_U32 * regPtr;     /* register data pointer */
    GT_BIT  cpuCodeValid;/*is the CPU code valid*/
    GT_U32  baseIndex;
    GT_BIT  dropBit;/*is the packet dropped */
    GT_U32  outGoingMtagCmd = descrPtr->outGoingMtagCmd;
    GT_U32  passDropCncMode;

    dropBit = clientInfoPtr->userDefined;
    __LOG_PARAM(dropBit);

    cpuCodeValid = /* CPU code valid for drops (in sip5) and for TO_CPU */
        (dropBit || outGoingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E)
          ? 1 : 0;

    __LOG_PARAM(cpuCodeValid);

    regPtr = smemMemGet(devObjPtr, SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr));

    /* Defines the operation mode for the CNC Client: Packet Type Drop/Pass Counter
        0 = Code: Use packet CPU/DROP_Code in index to CNC Packet Type Pass/Drop
        1 = Src Port: Use physical source port in index to CNC Packet Type Pass/Drop
    */
    passDropCncMode = SMEM_U32_GET_FIELD(*regPtr, 16, 1);

    __LOG_PARAM(passDropCncMode);

    if((cpuCodeValid == 0) || (passDropCncMode == 1))
    {
        /*Use physical source port in index to CNC Packet Type Pass/Drop*/
        baseIndex = descrPtr->localDevSrcPort;

        __LOG(("Use physical source port [%d] in index to CNC Packet Type Pass/Drop \n",
            baseIndex));
    }
    else
    {
        baseIndex = descrPtr->cpuCode;
        __LOG(("Use packet CPU/DROP_Code [%d] in index to CNC Packet Type Pass/Drop \n",
            baseIndex));
    }

    baseIndex &= 0xFF;

    __LOG_PARAM(baseIndex);

    clientInfoPtr->index =  (outGoingMtagCmd << 9) | (dropBit << 8) | baseIndex;

    /* Set byte count mode for EQ clients */
    clientInfoPtr->byteCntMode = SMEM_U32_GET_FIELD(*regPtr, 13, 1);

    return GT_OK;
}


/*******************************************************************************
*   snetCht3CncCount
*
* DESCRIPTION:
*       Trigger CNC event for specified client and set CNC counter block
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       client          - client ID.
*       userDefined     - user defined parameter
*
* RETURNS:
*       GT_OK               - CNC event triggered
*       GT_NOT_SUPPORTED    - CNC feature not supported
*       GT_FAIL             - CNC event is not triggered
*       GT_BAD_PARAM        - wrong CNC client index
*
*******************************************************************************/
void snetCht3CncCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CNC_CLIENT_E client,
    IN GT_UINTPTR userDefined
)
{
    DECLARE_FUNC_NAME(snetCht3CncCount);

    SNET_CHT3_CNC_CLIENT_INFO_STC clientInfo;   /* client info structure */
    GT_STATUS rc;                               /* return code */
    GT_CHAR*    clientNamePtr;
    GT_U32 cncUnitIndex;
    GT_U32 maxCncUnits;

    memset(&clientInfo, 0, sizeof(clientInfo));

    if (SKERNEL_IS_CHEETAH3_DEV(devObjPtr) == 0)
    {
        return;
    }

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_CNC_E);

    maxCncUnits  = devObjPtr->cncNumOfUnits ? devObjPtr->cncNumOfUnits : 1;

    /* Initialize input parameters of structure */
    clientInfo.client = client;
    clientInfo.userDefined = userDefined;
    clientInfo.clientsNamePtr = ((client < SNET_CNC_CLIENT_LAST_E) ? cncClientsNamesArr[client] : "unknown");

    clientNamePtr = clientInfo.clientsNamePtr;

    for(cncUnitIndex = 0 ; cncUnitIndex < maxCncUnits ; cncUnitIndex++)
    {
        clientInfo.cncUnitIndex = cncUnitIndex;
        __LOG(("Check CNC client triggering [%s] in CNC unit[%d] \n",
            clientNamePtr,cncUnitIndex));
        rc = snetCht3CncTrigger(devObjPtr, descrPtr, &clientInfo);
        if (rc == GT_OK)
        {
            /* Set CNC block for specified client */
            __LOG(("the Client [%s] is enabled in CNC unit[%d] \n",
                clientNamePtr,cncUnitIndex));
            rc = snetCht3CncBlockSet(devObjPtr, descrPtr, &clientInfo);
        }
        else
        {
            __LOG(("the Client [%s] is disabled in CNC unit[%d] \n",
                clientNamePtr,cncUnitIndex));
        }
    }

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___RESTORE_PREVIOUS_UNIT___E);


    return ;
}

/*******************************************************************************
*   snetCht3CncBlockWrapStatusSet
*
* DESCRIPTION:
*       Set CNC Block Wraparound Status Register
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       block           - wraparound counter block.
*       index           - wraparound counter index.
*      cncUnitIndex- the CNC unit index (0/1) (Sip5 devices , legacy device only 1 unit)
*
* RETURNS:
*       GT_OK           - CNC wraparound status table was successfully written
*       GT_FAIL         - CNC wraparound status table write fail
*
*******************************************************************************/
static GT_STATUS snetCht3CncBlockWrapStatusSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 block,
    IN GT_U32 index,
    IN GT_U32   cncUnitIndex
)
{
    GT_U32 regAddr;                 /* Resister address */
    GT_U32 * regPtr;                /* register data pointer */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 entry;                   /* Number of counters in wrap-around status table */
    GT_U32 intrNo;                  /* Interrupt cause bit */
    GT_STATUS status = GT_FAIL;

    /* CNC Block <block> Wraparound Status Register 0 */
    regAddr = SMEM_CHT3_CNC_BLOCK_WRAP_AROUND_STATUS_REG(devObjPtr, block, 0,cncUnitIndex);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* if the index already exists in the Wraparound table do nothing */
    for (entry = 0; entry < 4; entry++)
    {
        /* Block wraparound valid bit 0 */
        fldValue = SMEM_U32_GET_FIELD(regPtr[entry], 0, 1);
        /* Entry is in use */
        if (fldValue == 1)
        {
            if( SMEM_U32_GET_FIELD(regPtr[entry], 1, 12) ==  index)
            {
                return GT_OK;
            }
        }

        /* Block wraparound valid bit 16 */
        fldValue = SMEM_U32_GET_FIELD(regPtr[entry], 16, 1);
        /* Entry is in use */
        if (fldValue == 1)
        {
            if( SMEM_U32_GET_FIELD(regPtr[entry], 17, 12) ==  index)
            {
                return GT_OK;
            }
        }
    }

    for (entry = 0; entry < 4; entry++)
    {
        /* Block wraparound valid bit 0 */
        fldValue = SMEM_U32_GET_FIELD(regPtr[entry], 0, 1);
        /* Entry is not in use */
        if (fldValue == 0)
        {
            /* Set block wraparound index */
            SMEM_U32_SET_FIELD(regPtr[entry], 1, 12, index);
            SMEM_U32_SET_FIELD(regPtr[entry],0,1,1);
            status = GT_OK;
            break;
        }

        /* Block wraparound valid bit 16 */
        fldValue = SMEM_U32_GET_FIELD(regPtr[entry], 16, 1);
        /* Entry is not in use */
        if (fldValue == 0)
        {
            /* Set block wraparound index */
            SMEM_U32_SET_FIELD(regPtr[entry], 17, 12, index);
            SMEM_U32_SET_FIELD(regPtr[entry],16,1,1);
            status = GT_OK;
            break;
        }
    }

    intrNo = SMEM_CHT3_CNC_BLOCK_WRAP_AROUND(block);
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT3_CNC_INTR_CAUSE_REG(devObjPtr,cncUnitIndex),
                          SMEM_CHT3_CNC_INTR_MASK_REG(devObjPtr,cncUnitIndex),
                          intrNo,
                          SMEM_CHT_CNC_ENGINE_INT(devObjPtr,cncUnitIndex));

    return status;
}
/*******************************************************************************
* snetCht3CncCounterFormatGet
*
* DESCRIPTION:
*   The function gets format of CNC counter
*
* APPLICABLE DEVICES:  Lion and above.
*
* INPUTS:
*       devNum          - device number
*       blockNum        - CNC block number
*                         valid range see in datasheet of specific device.
*       cncUnitIndex- the CNC unit index (0/1) (Sip5 devices)
*
* OUTPUTS:
*       formatPtr       - (pointer to) CNC counter format
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetCht3CncCounterFormatGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN  GT_U32                              block,
    OUT CNC_COUNTER_FORMAT_ENT              *formatPtr,
    IN GT_U32   cncUnitIndex
)
{
    GT_U32  regAddr;                /* register address         */
    GT_U32  * regPtr;               /* register's data pointer  */
    GT_U32  fldValue;               /* register's field value   */
    GT_U32  startBit;
    GT_U32  numOfBits = 2;

    *formatPtr = CNC_COUNTER_FORMAT_PKT_29_BC_35_E;
    if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr) == 0)
    {
        /* CNC counter format is not supported */
        return;
    }

    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        numOfBits = 3;
        /*Blocks Counter Entry Mode Register%r*/
        regAddr = SMEM_SIP5_20_CNC_BLOCK_ENTRY_MODE_REG( devObjPtr, block, cncUnitIndex);
        regPtr = smemMemGet(devObjPtr, regAddr);
        startBit = 3* (cncUnitIndex % 8);

    }
    else
    {
        /*CNC Block %n Configuration Register0*/
        regAddr = SMEM_LION_CNC_BLOCK_CNF0_REG( devObjPtr, block, cncUnitIndex);
        regPtr = smemMemGet(devObjPtr, regAddr);

        if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            startBit = 12;
        }
        else
        {
            startBit = 30;
        }
    }

    /*<Block%nCounterEntryMode>*/
    fldValue = SMEM_U32_GET_FIELD(regPtr[0], startBit, numOfBits);
    switch (fldValue)
    {
        default:
            break;
        case 1:
            *formatPtr = CNC_COUNTER_FORMAT_PKT_27_BC_37_E;
            break;
        case 2:
            *formatPtr = CNC_COUNTER_FORMAT_PKT_37_BC_27_E;
            break;
        case 3:
            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                *formatPtr = CNC_COUNTER_FORMAT_PKT_64_BC_0_E;
            }
            break;
        case 4:
            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                *formatPtr = CNC_COUNTER_FORMAT_PKT_0_BC_64_E;
            }
            break;
    }

}

/*******************************************************************************
* snetCht3CncCounterHwRead
*
* DESCRIPTION:
*   The function read the 64-bit HW counter to SW format.
*
* APPLICABLE DEVICES:  DxCh3 and above.
*
* INPUTS:
*       format        - CNC counter HW format,
*                       relevant only for Lion and above
*       regPtr        - (pointer to) CNC Counter register
*
* OUTPUTS:
*       cncCounterPtr - (pointer to) CNC Counter in SW format
*
* RETURNS:
*       GT_OK          - on success
*       GT_BAD_PARAM   - on wrong parameters
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetCht3CncCounterHwRead
(
    IN   CNC_COUNTER_FORMAT_ENT         format,
    IN   GT_U32                         * regPtr,
    OUT  SNET_CHT3_CNC_COUNTER_STC      * cncCounterPtr
)
{
    switch (format)
    {
        case CNC_COUNTER_FORMAT_PKT_29_BC_35_E:
            /* Packets counter: 29 bits, Byte Count counter: 35 bits */
            cncCounterPtr->packetCount.l[0] =
                (regPtr[0] & 0x1FFFFFFF);
            cncCounterPtr->packetCount.l[1] = 0;
            cncCounterPtr->byteCount.l[0] = regPtr[1];
            cncCounterPtr->byteCount.l[1] =
                ((regPtr[0] >> 29) & 0x07);
            break;

        case CNC_COUNTER_FORMAT_PKT_27_BC_37_E:
            /* Packets counter: 27 bits, Byte Count counter: 37 bits */
            cncCounterPtr->packetCount.l[0] =
                (regPtr[0] & 0x07FFFFFF);
            cncCounterPtr->packetCount.l[1] = 0;
            cncCounterPtr->byteCount.l[0] = regPtr[1];
            cncCounterPtr->byteCount.l[1] =
                ((regPtr[0] >> 27) & 0x1F);
            break;

        case CNC_COUNTER_FORMAT_PKT_37_BC_27_E:
            /* Packets counter: 37 bits, Byte Count counter: 27 bits */
            cncCounterPtr->packetCount.l[0] = regPtr[0];
            cncCounterPtr->packetCount.l[1] =
                ((regPtr[1] >> 27) & 0x1F);
            cncCounterPtr->byteCount.l[0] =
                (regPtr[1] & 0x07FFFFFF);
            cncCounterPtr->byteCount.l[1] = 0;
            break;
        case CNC_COUNTER_FORMAT_PKT_64_BC_0_E:
            /* Packets counter: 64 bits, Byte Count counter: 0 bits */
            cncCounterPtr->packetCount.l[0] = regPtr[0];
            cncCounterPtr->packetCount.l[1] = regPtr[1];
            cncCounterPtr->byteCount.l[0] = 0;
            cncCounterPtr->byteCount.l[1] = 0;
            break;
        case CNC_COUNTER_FORMAT_PKT_0_BC_64_E:
            /* Packets counter: 64 bits, Byte Count counter: 0 bits */
            cncCounterPtr->packetCount.l[0] = 0;
            cncCounterPtr->packetCount.l[1] = 0;
            cncCounterPtr->byteCount.l[0] = regPtr[0];
            cncCounterPtr->byteCount.l[1] = regPtr[1];
            break;
        default: return GT_BAD_PARAM;
    }

    return GT_OK;
}

/*******************************************************************************
* snetCht3CncCounterHwWrite
*
* DESCRIPTION:
*   The function writes the SW counter to HW in 64-bit HW format.
*
* APPLICABLE DEVICES:  DxCh3 and above.
*
* INPUTS:
*       format         - CNC counter HW format,
*                        relevant only for Lion and above
*       cncCounterPtr   - (pointer to) CNC Counter in SW format
*
* OUTPUTS:
*       regPtr   - (pointer to) CNC Counter register
*
* RETURNS:
*       GT_OK          - on success
*       GT_BAD_PARAM   - on wrong parameters
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetCht3CncCounterHwWrite
(
    IN   CNC_COUNTER_FORMAT_ENT         format,
    IN   GT_U32                         * regPtr,
    IN   SNET_CHT3_CNC_COUNTER_STC      * cncCounterPtr
)
{
    switch (format)
    {
        case CNC_COUNTER_FORMAT_PKT_29_BC_35_E:
            /* Packets counter: 29 bits, Byte Count counter: 35 bits */
            regPtr[0] =
                (cncCounterPtr->packetCount.l[0] & 0x1FFFFFFF)
                | (cncCounterPtr->byteCount.l[1] << 29);
            regPtr[1] = cncCounterPtr->byteCount.l[0];
            break;

        case CNC_COUNTER_FORMAT_PKT_27_BC_37_E:
            /* Packets counter: 27 bits, Byte Count counter: 37 bits */
            regPtr[0] =
                (cncCounterPtr->packetCount.l[0] & 0x07FFFFFF)
                | (cncCounterPtr->byteCount.l[1] << 27);
            regPtr[1] = cncCounterPtr->byteCount.l[0];
            break;

        case CNC_COUNTER_FORMAT_PKT_37_BC_27_E:
            /* Packets counter: 37 bits, Byte Count counter: 27 bits */
            regPtr[0] = cncCounterPtr->packetCount.l[0];
            regPtr[1] =
                (cncCounterPtr->byteCount.l[0] & 0x07FFFFFF)
                | (cncCounterPtr->packetCount.l[1] << 27);
            break;

        case CNC_COUNTER_FORMAT_PKT_64_BC_0_E:
            /* Packets counter: 64 bits, Byte Count counter: 0 bits */
            regPtr[0] = cncCounterPtr->packetCount.l[0];
            regPtr[1] = cncCounterPtr->packetCount.l[1];
            break;

        case CNC_COUNTER_FORMAT_PKT_0_BC_64_E:
            /* Packets counter: 0 bits, Byte Count counter: 64 bits */
            regPtr[0] = cncCounterPtr->byteCount.l[0];
            regPtr[1] = cncCounterPtr->byteCount.l[1];
            break;

        default: return GT_BAD_PARAM;
    }

    return GT_OK;
}

/*******************************************************************************
* snetCht3CncCounterWrapAroundCheck
*
* DESCRIPTION:
*   The function checks the 64-bit HW counter wraparound.
*
* APPLICABLE DEVICES:  DxCh3 and above.
*
* INPUTS:
*       wrapEn        - wraparound enabled/disabled
*       format        - CNC counter HW format,
*                       relevant only for Lion and above
*       cncCounterPtr - (pointer to) CNC Counter in SW format
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK         - on success
*       GT_FULL       - counter(s) value has reached it's maximal value
*       GT_BAD_PARAM  - on wrong parameters
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetCht3CncCounterWrapAroundCheck
(
    IN   GT_U32 wrapEn,
    IN   CNC_COUNTER_FORMAT_ENT         format,
    IN   SNET_CHT3_CNC_COUNTER_STC      * cncCounterPtr,
    IN   GT_BIT                         do64BitsByteWrapAround
)
{
    GT_BOOL wrapAround = GT_FALSE;

    switch (format)
    {
        case CNC_COUNTER_FORMAT_PKT_29_BC_35_E:
            /* Packets counter: 29 bits, Byte Count counter: 35 bits */
            if(cncCounterPtr->packetCount.l[0] >= 0x1FFFFFFF)
            {
                /* Wraparound disabled */
                if(wrapEn == 0)
                {
                    cncCounterPtr->packetCount.l[0] = 0x1FFFFFFF;
                }
                wrapAround = GT_TRUE;
            }
            if(cncCounterPtr->byteCount.l[1] > 0x7 ||
              (cncCounterPtr->byteCount.l[0] == 0xFFFFFFFF &&
               cncCounterPtr->byteCount.l[1] == 0x7))
            {
                /* Wraparound disabled */
                if(wrapEn == 0)
                {
                    cncCounterPtr->byteCount.l[0] = 0xFFFFFFFF;
                    cncCounterPtr->byteCount.l[1] = 0x7;
                }
                wrapAround = GT_TRUE;
            }
            break;

        case CNC_COUNTER_FORMAT_PKT_27_BC_37_E:
            /* Packets counter: 27 bits, Byte Count counter: 37 bits */
            if(cncCounterPtr->packetCount.l[0] >= 0x07FFFFFF)
            {
                /* Wraparound disabled */
                if(wrapEn == 0)
                {
                    cncCounterPtr->packetCount.l[0] = 0x07FFFFFF;
                }
                wrapAround = GT_TRUE;
            }
            if(cncCounterPtr->byteCount.l[1] > 0x1F ||
               (cncCounterPtr->byteCount.l[0] == 0xFFFFFFFF &&
                cncCounterPtr->byteCount.l[1] == 0x1F))
            {
                /* Wraparound disabled */
                if(wrapEn == 0)
                {
                    cncCounterPtr->byteCount.l[0] = 0xFFFFFFFF;
                    cncCounterPtr->byteCount.l[1] = 0x1F;
                }
                wrapAround = GT_TRUE;
            }
            break;

        case CNC_COUNTER_FORMAT_PKT_37_BC_27_E:
            /* Packets counter: 37 bits, Byte Count counter: 27 bits */
            if(cncCounterPtr->packetCount.l[1] > 0x1F ||
              (cncCounterPtr->packetCount.l[0] == 0xFFFFFFFF &&
               cncCounterPtr->packetCount.l[1] == 0x1F))
            {
                /* Wraparound disabled */
                if(wrapEn == 0)
                {
                    cncCounterPtr->packetCount.l[0] = 0xFFFFFFFF;
                    cncCounterPtr->packetCount.l[1] = 0x1F;
                }
                wrapAround = GT_TRUE;
            }

            if(cncCounterPtr->byteCount.l[0] >= 0x07FFFFFF)
            {
                /* Wraparound disabled */
                if(wrapEn == 0)
                {
                    cncCounterPtr->byteCount.l[0] = 0x07FFFFFF;
                }
                wrapAround = GT_TRUE;
            }
            break;

        case CNC_COUNTER_FORMAT_PKT_64_BC_0_E:
            /* Packets counter: 64 bits, Byte Count counter: 0 bits */
            if(cncCounterPtr->packetCount.l[0] == 0xFFFFFFFF &&
               cncCounterPtr->packetCount.l[1] == 0xFFFFFFFF)
            {
                wrapAround = GT_TRUE;
            }
            break;

        case CNC_COUNTER_FORMAT_PKT_0_BC_64_E:
            /* Packets counter: 0 bits, Byte Count counter: 64 bits */
            if(do64BitsByteWrapAround)
            {
                if(wrapEn == 0)
                {
                    cncCounterPtr->byteCount.l[0] = 0xFFFFFFFF;
                    cncCounterPtr->byteCount.l[1] = 0xFFFFFFFF;
                }
                wrapAround = GT_TRUE;
            }
            break;

        default:
            return GT_BAD_PARAM;
    }

    return ((wrapAround == GT_TRUE) ? GT_FULL : GT_OK);
}

