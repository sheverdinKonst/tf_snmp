/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSoho.h
*
* DESCRIPTION:
*       This is a external API definition for SNetSoho module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 20 $
*
*******************************************************************************/
#ifndef __snetSohoh
#define __snetSohoh


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define     SOHO_ATU_ENTRY_SIZE_IN_WORDS        3
#define     SOHO_ATU_BUCKET_SIZE_IN_BINS        4

#define SOHO_MSG_2_MAC(_msg_ptr, _mac_addr)                             \
{                                                                       \
    _mac_addr.bytes[5] = (GT_U8)((((GT_U32*)_msg_ptr)[0]>>0)  & 0xff);  \
    _mac_addr.bytes[4] = (GT_U8)((((GT_U32*)_msg_ptr)[0]>>8)  & 0xff);  \
    _mac_addr.bytes[3] = (GT_U8)((((GT_U32*)_msg_ptr)[0]>>16) & 0xff);  \
    _mac_addr.bytes[2] = (GT_U8)((((GT_U32*)_msg_ptr)[0]>>24) & 0xff);  \
    _mac_addr.bytes[1] = (GT_U8)((((GT_U32*)_msg_ptr)[1]>>0)  & 0xff);  \
    _mac_addr.bytes[0] = (GT_U8)((((GT_U32*)_msg_ptr)[1]>>8)  & 0xff);  \
}

#if !defined(BE_HOST) /* i.e. simulation running on little endian CPU - LE_HOST */

/* use this macro when the msg come from the CPU */
#define SOHO_SWAP_MSG_2_MAC_FROM_CPU(_msg_ptr, _mac_addr)               \
{                                                                       \
    _mac_addr.bytes[1] = (GT_U8)(((GT_U8*)_msg_ptr)[0]);                \
    _mac_addr.bytes[0] = (GT_U8)(((GT_U8*)_msg_ptr)[1]);                \
    _mac_addr.bytes[3] = (GT_U8)(((GT_U8*)_msg_ptr)[2]);                \
    _mac_addr.bytes[2] = (GT_U8)(((GT_U8*)_msg_ptr)[3]);                \
    _mac_addr.bytes[5] = (GT_U8)(((GT_U8*)_msg_ptr)[4]);                \
    _mac_addr.bytes[4] = (GT_U8)(((GT_U8*)_msg_ptr)[5]);                \
}

#else /* BE_HOST */

/* use this macro when the msg come from the CPU */
#define SOHO_SWAP_MSG_2_MAC_FROM_CPU(_msg_ptr, _mac_addr)               \
{                                                                       \
    _mac_addr.bytes[0] = (GT_U8)(((GT_U8*)_msg_ptr)[0]);                \
    _mac_addr.bytes[1] = (GT_U8)(((GT_U8*)_msg_ptr)[1]);                \
    _mac_addr.bytes[2] = (GT_U8)(((GT_U8*)_msg_ptr)[2]);                \
    _mac_addr.bytes[3] = (GT_U8)(((GT_U8*)_msg_ptr)[3]);                \
    _mac_addr.bytes[4] = (GT_U8)(((GT_U8*)_msg_ptr)[4]);                \
    _mac_addr.bytes[5] = (GT_U8)(((GT_U8*)_msg_ptr)[5]);                \
}

#endif

/* End of request in RMU packet */
#define REQ_END_LIST(_frame_data_ptr,ii)\
    (GT_U32)((_frame_data_ptr)[24 +ii*4] << 24 | (_frame_data_ptr)[25 +ii*4] << 16 | \
             (_frame_data_ptr)[26 +ii*4] << 8  | (_frame_data_ptr)[27 +ii*4] )

#define SOHO_MEMBER_TAG_2_VTU_VECTOR(_port_tag, _port_vector)           \
{                                                                       \
    GT_U32 i;                                                           \
    for (i = 0; i < SOHO_PORTS_NUMBER; i++)                             \
    {                                                                   \
         _port_vector = ((_port_tag[i] & 0x3) != 3) ?                   \
                                _port_vector | (1 << i) :               \
                                _port_vector & ~(1 << i);               \
    }                                                                   \
}

typedef enum {
    SNET_SOHO_DISABLED_STATE=0,
    SNET_SOHO_BLOCKING_LISTENING_STATE,
    SNET_SOHO_LEARNING_STATE,
    SNET_SOHO_FORWARD_STATE,
}SOHO_PORT_STATE_ENT;


/* SOHO_PORT_8021Q_MODE_ENT enum of port 802.1q mode */
/*
 *  SNET_SOHO_8021Q_MODE_DISABLE        - use port based vlan only
 *  SNET_SOHO_8021Q_MODE_FALLBACK       - enable 802.1q for this port.
 *  SNET_SOHO_8021Q_MODE_CHECK          - enable 802.1q for this port.
                                          discard frame if its vid is
                                          not contained in the vtu.
 *  SNET_SOHO_8021Q_MODE_SECURE         - enable 802.1q for this port.
                                          discard frame if its vid is
                                          not contained in the vtu.
*/
typedef enum {
    SNET_SOHO_8021Q_MODE_DISABLE=0,
    SNET_SOHO_8021Q_MODE_FALLBACK,
    SNET_SOHO_8021Q_MODE_CHECK,
    SNET_SOHO_8021Q_MODE_SECURE,
}SOHO_PORT_8021Q_MODE_ENT;

/* SOHO_PORT_TAG_EGRESS_MODE_ENT port egress mode */
/*
 *  SNET_SOHO_EGRESS_MODE_UNMODIFIED    - unmodified mode.
 *  SNET_SOHO_EGRESS_MODE_UNTAGGED_STATE- untagged mode.
 *  SNET_SOHO_LEARNING_STATE            - tagged mode.
 *  SNET_SOHO_FORWARD_STATE             - double tag mode.
*/
typedef enum {
    SNET_SOHO_EGRESS_MODE_UNMODIFIED=0,
    SNET_SOHO_EGRESS_MODE_UNTAGGED_STATE,
    SNET_SOHO_EGRESS_MODE_TAGGED_STATE,
    SNET_SOHO_EGRESS_MODE_DOUBLE_TAG_STATE,
}SOHO_PORT_TAG_EGRESS_MODE_ENT;



/* OPAL_REQUEST_REMOTE_CODE_ENT  */
/*
 *  SNET_OPAL_GETID  - request for getting response format and product/rev num
 *  SNET_OPAL_DUMP_ATU_STATE
 *  SNET_OPAL_DUMP_MIB            
 *  SNET_OPAL_READ_WRITE_STATE             
*/
typedef enum {
    SNET_OPAL_GETID=0,
    SNET_OPAL_DUMP_ATU_STATE= 0x1000,
    SNET_OPAL_DUMP_MIB= 0x1020,
    SNET_OPAL_READ_WRITE_STATE= 0x2000,
}OPAL_REQUEST_REMOTE_C0DE_ENT;

/* SOHO_8021Q_VTU_TAG_MODE_ENT VTU tag mode  */
/*
 *  SNET_SOHO_8021Q_UNMODIFIED_STATE    - unmodified mode.
 *  SNET_SOHO_8021Q_UNTAGGED_STATE      - untagged mode.
 *  SNET_SOHO_8021Q_TAGGED_STATE        - tagged mode.
 *  SNET_SOHO_8021Q_NOTMEMBER_STATE     - the port in not member in vtu.
*/
typedef enum {
    SNET_SOHO_8021Q_UNMODIFIED_STATE=0,
    SNET_SOHO_8021Q_UNTAGGED_STATE,
    SNET_SOHO_8021Q_TAGGED_STATE,
    SNET_SOHO_8021Q_NOTMEMBER_STATE
}SOHO_8021Q_VTU_TAG_MODE_ENT;

/* SOHO_8021Q_VTU_TAG_MODE_ENT VTU tag mode  */
/*
 * tag_mode        - port egress mode.
 * state           - port state mode.
 * cascaded_port   - tagged mode.
 * num             - the port in not member in vtu.
 * vtu_tag_mode    - tag mode from the VTU table.
 * p_8021q_mode    - 802.1q port mode
*/
typedef struct {
    SOHO_PORT_TAG_EGRESS_MODE_ENT  tag_mode;
    SOHO_PORT_STATE_ENT            state;
    GT_U8                          num;
    SOHO_8021Q_VTU_TAG_MODE_ENT    vtu_tag_mode;
    SOHO_PORT_8021Q_MODE_ENT       p_8021q_mode;
}SOHO_PORT_INFO_STC;

/* Format of the ATU Database */
/*
 *  macAddr         - 48-bit MAC address field
 *  entryState      - 4-bit entry state field
 *  atuData         - 10-bit ATU data field
 *  priority        - 2-bit priority field
 *  fid             - 1- FID[11], this bit must match FID[11] passed to the ATU for a Hit.
 *                       i.e., a 49-bit compare is done (along with 48 bit of MAC addr).
*/
typedef struct {
    SGT_MAC_ADDR_UNT    macAddr;
    GT_U8               entryState;
    GT_U16              atuData;
    GT_U8               priority;
    GT_U8               trunk;
    GT_U8               fid;
} SNET_SOHO_ATU_STC;


/* Format of the VTU Database */
/*
 *  dbNum           - database number
 *  vid             - VLAN ID
 *  pri             - priority
 *  portsMap        - ports member tag info and state.
 *  vidPolicy       - vid policy map
 *  usepri          - overrided priority bit 
 *  sid             - port state information data base
 * valid            - valid entry VLAN
*/
typedef struct {
    GT_U32              dbNum;
    GT_U32              vid;
    GT_U32              pri;
    GT_U8               portsMap[11];
    GT_U32              vidPolicy;
    GT_U32              usepri;
    GT_U32              sid;
    GT_U32              valid;
} SNET_SOHO_VTU_STC;

/* Mirror flags for rx/tx and arp */
/*
 * monFwd   - Monitor with normal frame
 * inMon    - Tx sniffer with rxSnif = 1
 * outMon   - Tx sniffer with txSnif = 1
 * arpMonitorDest - ARP monitor destination port
*/
typedef struct {
    GT_U32              monFwd;
    GT_U32              inMon;
    GT_U32              outMon;
    GT_U32              arpMonitorDest;
} SNET_SOHO_MIRROR_STC;

/*******************************************************************************
*   snetSohoProcessInit
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
void snetSohoProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   snetSohoCreateMarvellTag
*
* DESCRIPTION:
*       Get Marvell tag from descriptor
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr  - description pointer
*
* OUTPUTS:
*       Returns 32 bits Marvell TAG
*
*******************************************************************************/
GT_U32 snetSohoCreateMarvellTag
(
    IN SKERNEL_DEVICE_OBJECT           * devObjPtr,
    INOUT SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetSohoMacHashCalc
*
* DESCRIPTION:
*        Calculates the hash index for the mac address table
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       macAddrPtr      -  pointer to the first byte of MAC address.
*
* RETURNS:
*        The hash index
*******************************************************************************/
GT_U32 snetSohoMacHashCalc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetSohoh */


