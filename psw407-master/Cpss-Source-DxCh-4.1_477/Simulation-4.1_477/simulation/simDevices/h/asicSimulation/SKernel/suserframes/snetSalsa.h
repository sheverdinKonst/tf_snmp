/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSalsa.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __snetSalsah
#define __snetSalsah


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    SALSA_MAC_RANGE_NOT_ACTIVE_E,
    SALSA_MAC_RANGE_FWD_E,
    SALSA_MAC_RANGE_DROP_E,
    SALSA_MAC_RANGE_TRAP_CPU_E
} MAC_RANGR_FILTER_E;

typedef struct {
    SGT_MAC_ADDR_UNT    macAddr;
    GT_U16              vid;
    GT_U16              trunk;
    GT_U16              aging;
    GT_U16              skipEntry;
    GT_U16              validEntry;
    GT_U16              rxSniff;
    GT_U16              saCmd;
    GT_U16              daCmd;
    GT_U16              forceL3Cos;
    GT_U16              multiply;
    GT_U8               staticEntry;
    GT_U16              daPriority;
    GT_U16              saPriority;
    GT_U16              vidx;
    GT_U16              port;
} SNET_SALSA_MAC_TBL_STC;

/* CPU code is set to Marvell_Tag */
typedef enum
{
    SALSA_CPU_CODE_CONTROL_TO_CPU              = 1,
    SALSA_CPU_CODE_BPDU                        = 2,
    SALSA_CPU_CODE_MAC_ADDR_TRAP               = 3,
    SALSA_CPU_CODE_MAC_RANGE_TRAP              = 4,
    SALSA_CPU_CODE_ARP_BROADCAST               = 5,
    SALSA_CPU_CODE_IGMP_PACKET                 = 6,
    SALSA_CPU_CODE_INTERVENTION_MAC_ADDR       = 7,
    SALSA_CPU_CODE_INTERVENTION_LOCK_TO_CPU    = 8,
    SALSA_CPU_CODE_MIRROR_MAC_RANGE_TO_CPU     = 9,
    SALSA_CPU_CODE_MIRROR_LOCK_TO_CPU          = 10
} SALSA_MRVL_TAG_CPU_CODE;

typedef struct {
    GT_U16 vid;
    GT_U16 cpuCode;
    GT_U16 vpt;
    GT_U16 srcIsTrunk;
    GT_U16 rxSniff;
    GT_U16 prio;
    GT_U16 useVidx;
    GT_U16 trgPort;
    GT_U16 trgDev;
    GT_U16 vidx;
    GT_U16 srcPort;
    GT_U16 srcTrunk;
    GT_U16 srcDevice;
    GT_U16 dstTagged;
    GT_U16 srcTagged;
    GT_U16 tagCommand;
}MRVL_TAG;


/*******************************************************************************
*   snetSalsaProcessInit
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
void snetSalsaProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   snetSalsaGetFreeMacEntryAddr
*
* DESCRIPTION:
*        Look up in the MAC table for the first free MAC entry address
* INPUTS:
*        deviceObj   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
* OUTPUTS:
*******************************************************************************/
GT_U32 snetSalsaGetFreeMacEntryAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid
);
/*******************************************************************************
*   snetSalsaFindMacEntry
*
* DESCRIPTION:
*        Look up in the MAC table
* INPUTS:
*        deviceObj   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
* OUTPUTS:
*        entryIdxPtr - index of entry in the table
*        TRUE - MAC entry found, FALSE - MAC entry not found
*
*******************************************************************************/
GT_BOOL snetSalsaFindMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    OUT SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr,
    OUT GT_U32  * entryIdxPtr
);

void snetSalsaTxMirror
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 egressPort
);

void snetSalsaTxMacCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
);

GT_BOOL  snetSalsaVlanPortsGet
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN GT_U32 vid,
    OUT GT_U8 vlan_member_ports[],
    OUT GT_U8 vlan_tagged_ports[],
    OUT GT_U8 stp_for_ports[]
);

GT_BOOL  snetSalsaVlanPortsGet
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN GT_U32 vid,
    OUT GT_U8 vlan_member_ports[],
    OUT GT_U8 vlan_tagged_ports[],
    OUT GT_U8 stp_for_ports[]
);

GT_BOOL snetSalsaVidxPortsGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vidx,
    OUT GT_U8 vidx_ports[]
);

GT_U16 snetSalsaTrunkHash
(
    IN SGT_MAC_ADDR_TYP * macDstPtr,
    IN SGT_MAC_ADDR_TYP * macSrcPtr,
    IN GT_U32 trgPort
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetSalsah */


