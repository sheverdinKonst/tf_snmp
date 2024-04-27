/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistL2.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/
#ifndef __snetTwistL2h
#define __snetTwistL2h

#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum {
    TWIST_MAC_RANGE_NOT_ACTIVE_E,
    TWIST_MAC_RANGE_FWD_E,
    TWIST_MAC_RANGE_DROP_E,
    TWIST_MAC_RANGE_TRAP_CPU_E
} TWIST_MAC_RANGR_FILTER_E;



/* CPU code is set to Marvell_Tag */
typedef enum
{
    TWIST_CPU_CODE_CONTROL_TO_CPU               = 1,
    TWIST_CPU_CODE_BPDU                         = 16,
    TWIST_CPU_CODE_MAC_ADDR_TRAP                = 17,
    TWIST_CPU_CODE_MAC_RANGE_TRAP               = 20,
    TWIST_CPU_CODE_RX_SNIFF                     = 21,
    TWIST_CPU_CODE_ARP_BROADCAST                = 32,
    TWIST_CPU_CODE_IGMP_PACKET                  = 33,
    TWIST_CPU_CODE_INTERVENTION_MAC_ADDR        = 36,
    TWIST_CPU_CODE_INTERVENTION_LOCK_TO_CPU     = 37,

    /* Inblock Codes */
    TWIST_CPU_CODE_RESRVED_SIP_TRAP             = 128,
    TWIST_CPU_CODE_INTERNAL_SIP_TRAP            = 129,
    TWIST_CPU_CODE_SPOOF_SIP_TRAP               = 130,

    /* TCB codes */
    TWIST_CPU_TCB_TRAP_DEFAULT                  = 132,
    TWIST_CPU_TCB_TRAP_COS                      = 133,
    TWIST_CPU_TCB_KEY_ENTRY                     = 134,
    TWIST_CPU_TCB_TCP_RST_FIN                   = 135,
    TWIST_CPU_TCB_CPU_MIRROR_FIN                = 136,

    /* ipv4 Codes */
    TWIST_CPU_CODE_RESRVED_DA                   = 144,
    TWIST_CPU_CODE_LOCAL_SCOPE                  = 145,
    TWIST_CPU_CODE_INTERNAL_DA                  = 146,
    TWIST_CPU_CODE_TRAP_TTL                     = 147,
    TWIST_CPU_CODE_BAD_CHECKSUM                 = 148,
    TWIST_CPU_CODE_RPF_NOT_PASSED               = 149,
    TWIST_CPU_CODE_OPTION_FAILED                = 150,
    TWIST_CPU_CODE_NEED_REASSRMBLY              = 151,
    TWIST_CPU_CODE_BAD_TUNNEL_HEADER            = 152,
    TWIST_CPU_CODE_IP_HEADER_ERR                = 153,
    TWIST_CPU_CODE_PACKET_TRAPPED               = 154,

    /* MPLS Codes */
    TWIST_CPU_MPLS_MTU_EXCEED                   = 162,
    TWIST_CPU_MPLS_NULL_TTL                     = 171,
    TWIST_CPU_MPLS_NHLFE_CMD_TRAP               = 172,
    TWIST_CPU_MPLS_ILLEGAL_POP                  = 173,
    TWIST_CPU_MPLS_INVALID_IF_ENTRY             = 174
} TWIST_CPU_CODE;

#if 0
typedef enum {
    TWIST_CMD_TO_CPU_E,
    TWIST_CMD_FROM_CPU_E,
    TWIST_CMD_TO_TARGET_SNIFFER_E,
    TWIST_CMD_FORWARD_E
} TAG_COMMAND_E;
#endif

/*******************************************************************************
*   snetTwisttProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
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
void snetTwistProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   snetTwistGetFreeMacEntryAddr
*
* DESCRIPTION:
*        Look up in the MAC table for the first free MAC entry address
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
* OUTPUTS:
*******************************************************************************/
GT_U32 snetTwistGetFreeMacEntryAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid
);

/*******************************************************************************
*   snetTwistTxMacCountUpdate
*
* DESCRIPTION:
*        Global update counters function (MAC MIB Counters + CPU port)
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        port       - counter's port
*
*******************************************************************************/
GT_VOID snetTwistTxMacCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
);

/*******************************************************************************
*   snetTwistVltTables
*
* DESCRIPTION:
*        Get VLAN information (vlan ports, tagged ports, STP state)
* INPUTS:
*       devObjPtr          - pointer to device object.
*       descPtr            - frame descriptor
* OUTPUTS:
*       vlan_ports         - array to the VLAN port members
*       vlan_tagged_ports  - array to the VLAN tagged ports
*       stp_for_ports      - array to the STP ports.
*
*******************************************************************************/
GT_BOOL  snetTwistVltTables
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descPtr,
    OUT GT_U8 vlan_member_ports[],
    OUT GT_U8 vlan_tagged_ports[],
    OUT GT_U8 stp_for_ports[]
);

/*******************************************************************************
*   snetTwistVidxPortsGet
*
* DESCRIPTION:
*        Get VIDX ports information
* INPUTS:
*       vidx          - pointer to device object.
*
* OUTPUTS:
*       vidx_ports    - array of the vidx
*
*******************************************************************************/
GT_BOOL snetTwistVidxPortsGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vidx,
    OUT GT_U8 vidx_ports[]
);

/*******************************************************************************
*   snetTwistGetMacEntry
*
* DESCRIPTION:
*        Get MAC table entry by index in the hash table
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        regAddress  -  MAC entry address.
* OUTPUTS:
*        twistMacEntryPtr - pointer to the MAC table entry or NULL if not found.
*
*******************************************************************************/
GT_VOID snetTwistGetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr,
    IN GT_U32 regAddress,
    OUT SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
);

/*******************************************************************************
*   snetTwistSetMacEntry
*
* DESCRIPTION:
*        Get MAC table entry by index in the hash table
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        regAddress  -  MAC entry address.
* OUTPUTS:
*        twistMacEntryPtr - pointer to the MAC table entry or NULL if not found.
*
*******************************************************************************/
GT_VOID snetTwistSetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddress,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
);


/*******************************************************************************
*   snetTwistFdbAuSend
*
* DESCRIPTION:
*        Send address update message
* INPUTS:
*        devObjPtr  -  pointer to device object.
*        hwData[4]  -  pointer to CPU message
* OUTPUTS:
*        setIntrPtr -  set interrupt flag
*
*
*******************************************************************************/
GT_BOOL snetTwistFdbAuSend
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN GT_U32 hwData[4],
    OUT GT_BOOL  * setIntrPtr
);

/*******************************************************************************
*   snetTwistFindMacEntry
*
* DESCRIPTION:
*        Look up in the MAC table
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
*        macEntryPtr -  mac table entry
*        entryIdx    -  index of found entry
* OUTPUTS:
*        TRUE - MAC entry found, FALSE - MAC entry not found
*
*******************************************************************************/
GT_BOOL snetTwistFindMacEntry
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U8 * macAddrPtr,
    IN  GT_U16 vid,
    OUT SNET_TWIST_MAC_TBL_STC * macEntryPtr,
    OUT GT_U32 * entryIdx
);

/*******************************************************************************
*   snetTwistCreateMessage
*
* DESCRIPTION:
*        Create new MAC address message
* INPUTS:
*        devObjPtr      - pointer to device object.
*        descrPtr       - pointer to the frame's descriptor.
*        sourceMacPtr   - pointer to the source MAC address.
*        newMacEntryPtr - pointer to the MAC entry
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID snetTwistCreateMessage(
    IN           SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN           SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN           GT_U8                   * sourceMacPtr,
    INOUT        SNET_TWIST_MAC_TBL_STC  * newMacEntryPtr
);
/*******************************************************************************
*   snetTwistCreateAAMsg
*
* DESCRIPTION:
*        Set Aged Address Message by free FIFO entry pointer
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        twistMacEntryPtr - MAC entry pointer
*        hwData[4] - New address message data
*
*******************************************************************************/
GT_VOID snetTwistCreateAAMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr,
    OUT GT_U32 hwData[4]
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistL2h */


