/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSalsa.c
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 44 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/suserframes/snetSalsa.h>
#include <asicSimulation/SKernel/smem/smemSalsa.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <common/Utils/PresteraHash/smacHashDx.h>
#include <asicSimulation/SKernel/salsaCommon/sregSalsa.h>
#include <asicSimulation/SKernel/sstack/sstackSalsa.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/sfdb/sfdbSalsa.h>
#include <asicSimulation/SLog/simLog.h>

#define SNET_SALSA_DEFAULT_SA_DA_TC_CNS 0xff

extern GT_VOID snetTwistMacRangeFilterGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetSalsaRxMacProcess
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaRxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static GT_BOOL snetSalsaTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    IN GT_U8 tagged
);
static void snetSalsaTx2Device
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 targetPort,
    IN GT_U32 targetDevice
);
static void snetSalsaTx2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaVlanTcAssign
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static GT_BOOL snetPortProtocolVidGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 etherType,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaControlFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaMacRange
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaFdbLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetMacRangeFilterGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_U32 snetSalsaMacHashCalc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    IN GT_U8 isVlanUsed
);
static void snetGetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr,
    IN GT_U32 regAddress,
    OUT SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr
);
static void snetSetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddress,
    IN SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr
);

static  void snetSalsaLockPort
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static  void snetSalsaIngressFilter
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static  void snetSalsaL2Decision
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetCreateMarvellTag
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32_PTR mrvlTagPtr
);
static void snetSalsaFrwrdUcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void  snetSalsaFwdUcastDest
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void snetSalsaFrwrdMcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void  snetSalsaFwdMcastDest
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);
static void  snetSalsaTrapPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 cpuCode
);
static void  snetSalsaDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);
static void  snetSalsaDrop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
}
static void  snetSalsaSecurityDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);
static void  snetSalsaIngressDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);
static void  snetSalsaRegularDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
);

static GT_BOOL snetSalsaVidxPortsInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT GT_U8 vidx_ports[]
);

static void snetSalsaLearnSa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetSetFdbFifoEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 * fifoPtr,
    IN SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr
);

static void snetSalsaMacMibCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
);

static void snetSalsaCpuCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
);

static GT_BOOL snetSalsaSflowControl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_SFLOW_DIRECTION_ENT flowDirect,
    IN GT_U32 sflowPort,
    IN GT_U8 * dataPtr,
    IN GT_U32 dataLen
);

static GT_VOID snetSalsaFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

static GT_VOID snetSalsaLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN GT_U32                           port,
    IN GT_U32                           linkState
);

typedef struct {
    GT_U32 startOffset;
    GT_U32 endOffset;
    /*    GT_U32      stepSize;*/
} PORT_COUNTERS;

/* Calculating Port Counter Addresses */
static PORT_COUNTERS portsCounters[24] = {
    {0x04010000, 0x0401007C},{0x04010080, 0x040100FC},{0x04010100, 0x0401017C},
    {0x04010180, 0x040101FC},{0x04010200, 0x0401027C},{0x04010280, 0x040102FC},
    {0x04810000, 0x0481007C},{0x04810080, 0x048100FC},{0x04810100, 0x0481017C},
    {0x04810180, 0x048101FC},{0x04810280, 0x048102FC},{0x04810300, 0x0481037C},
    {0x05010000, 0x0501007C},{0x05010080, 0x050100FC},{0x05010100, 0x0501017C},
    {0x05010180, 0x050101FC},{0x05010200, 0x0501027C},{0x05010280, 0x050102FC},
    {0x05810000, 0x0581007C},{0x05810080, 0x058100FC},{0x05810100, 0x0581017C},
    {0x05810180, 0x058101FC},{0x05810200, 0x0581027C},{0x05810280, 0x058102FC}
};

/*******************************************************************************
*   snetSalsaFindMacEntry
*
* DESCRIPTION:
*        Look up in the MAC table
* INPUTS:
*        devObjPtr   -  pointer to device object.
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
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 hashIdx;
    GT_U32 hashBucketSize;
    SGT_MAC_ADDR_UNT macAddr;
    GT_U8 isVlanUsed;
    GT_U16 i;
    GT_U32 workHashBucketSize;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(macAddrPtr);

    smemRegFldGet(devObjPtr, MAC_TABLE_CTRL_REG, 4, 1, &fldValue);
    /* VlanLookup-Mode */
    isVlanUsed = (GT_U8)fldValue;
    hashIdx = snetSalsaMacHashCalc(devObjPtr, macAddrPtr, vid, isVlanUsed);

    smemRegFldGet(devObjPtr, MAC_TABLE_CTRL_REG, 0, 3, &fldValue);
    /* NumOfEntries */
    workHashBucketSize = hashBucketSize = (fldValue + 1) * 2;
    if(hashBucketSize + hashIdx > 0x2000)
    {
        workHashBucketSize = 0x2000 - hashIdx;
    }
    for (i = 0; i < workHashBucketSize; i++) {
        regAddress = MAC_TAB_ENTRY_WORD0_REG + (hashIdx + i)* 0x10;
        snetGetMacEntry(devObjPtr, regAddress, salsaMacEntryPtr);

        if (salsaMacEntryPtr->validEntry && salsaMacEntryPtr->skipEntry == 0) {
            memcpy(macAddr.bytes, macAddrPtr, 6);
            if (SGT_MAC_ADDR_ARE_EQUAL(&macAddr, &salsaMacEntryPtr->macAddr)) {
                if ((isVlanUsed && salsaMacEntryPtr->vid == vid) || !isVlanUsed)
                {
                    /* entry found */
                    if (entryIdxPtr != NULL)
                        *entryIdxPtr = hashIdx + i;

                    return GT_TRUE;
                }
            }
        }
    }
    return GT_FALSE;
}

/*******************************************************************************
*   snetSalsaTrunkTableReg
*
* DESCRIPTION:
*
* INPUTS:
*       trunkId    - trunk id
* OUTPUTS:
*       address - address of the 32 bits Trunk Members register
*       Trunk <port> membership first bit
*
*******************************************************************************/
static GT_U32 snetSalsaTrunkTableReg
(
    IN GT_U32 trunkId,
    OUT GT_U32 * address
)
{
    GT_U32      firstBit = 0;

    if (trunkId >= 1 && trunkId < 9) {
        firstBit = ((trunkId - 1) * 4);
        *address = 0x02040170;
    }
    if (trunkId >= 9 && trunkId < 17) {
        firstBit = ((trunkId - 9) * 4);
        *address = 0x02040174;
    }
    if (trunkId >= 17 && trunkId < 25) {
        firstBit = ((trunkId - 17) * 4);
        *address = 0x02040178;
    }
    if (trunkId >= 25 && trunkId < 32) {
        firstBit = ((trunkId - 25) * 4);
        *address = 0x0204017C;
    }

    return firstBit;
}
/*******************************************************************************
*   snetSalsaInternVtablePortOffset
*
* DESCRIPTION:
*
* INPUTS:
*       entryIdx     - VID entry index
*       port    - port for the VLAN
* OUTPUTS:
*       address - address of the 32 bits VID entry
*       Port<n>member field's first bit index
*
*******************************************************************************/
static GT_U32 snetSalsaInternVtablePortOffset
(
    IN GT_U32 entryIdx,
    IN GT_U32 port,
    OUT GT_U32 * address
)
{
    GT_U32 firstBit = 0;

    INTERNAL_VLAN_TABLE(entryIdx, address);
    if (port < 4) {
        firstBit = 24 + (port * 2);
    }
    else
    if (port >= 4 && port < 20) {
        firstBit = (port - 4) * 2;
        *address += 0x4;

    }
    else
    if (port >= 20 && port < 24) {
        firstBit = (port - 20) * 2;
        *address += 0x8;
    }

    return firstBit;
}

/*******************************************************************************
*   snetSalsaSpanStatePortOffset
*
* DESCRIPTION:
*
* INPUTS:
*       stgId   - STG group entry
*       port    - port for the VLAN
* OUTPUTS:
*       address - address of the 32 bits VID entry
*       Port<n>member field's first bit index
*
*******************************************************************************/
static GT_U32 snetSalsaSpanStatePortOffset
(
    IN GT_U32 stgId,
    IN GT_U32 port,
    OUT GT_U32 * address
)
{
    GT_U32 firstBit = 0;
    SPAN_STATE_TABLE(stgId, address);
    if (port < 16) {
        firstBit = port * 2;
    }
    else
    if (port >= 16 && port < 24) {
        firstBit = (port - 16) * 2;
        *address += 0x4;
    }

    return firstBit;
}
/*******************************************************************************
*   snetCreateMarvellTag
*
* DESCRIPTION:
*       Get Marvell tag from descriptor
*
* INPUTS:
*       descrPtr  - description pointer
*
* OUTPUTS:
*       Returns 32 bits Marvell TAG
*
*******************************************************************************/
static void snetCreateMarvellTag
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32_PTR mrvlTagPtr
)
{
    GT_U32 mrvlTag;
    GT_U32 mrvlTagCmd = 0;

    ASSERT_PTR(descrPtr);
    ASSERT_PTR(mrvlTagPtr);

    if (descrPtr->rxSniffed || descrPtr->txSniffed) {
        mrvlTagCmd = TAG_CMD_TO_TARGET_SNIFFER_E;
    }
    else
    {
        if (descrPtr->pktCmd == 0) {
        mrvlTagCmd = TAG_CMD_FORWARD_E;
        }
        else
        if (descrPtr->pktCmd == 2) {
            mrvlTagCmd = TAG_CMD_TO_CPU_E;
        }
    }

    mrvlTag = mrvlTagCmd << 30;

    if (mrvlTagCmd == TAG_CMD_TO_TARGET_SNIFFER_E ||
        mrvlTagCmd == TAG_CMD_FORWARD_E) {
        /* Src Dev */
        mrvlTag |= ((descrPtr->srcDevice & 0x1F) << 24);
    }
    /* Tag_Command */
    if (mrvlTagCmd == TAG_CMD_FORWARD_E) {
        /* Src_Tagged */
        mrvlTag |= ((descrPtr->srcVlanTagged & 0x01) << 29);

        if (descrPtr->srcTrunkId == 0) {
            /* Src_Port */
            mrvlTag |= ((descrPtr->srcPort & 0x1F) << 19);
        }
        else {
            /* Src_Trunk */
            mrvlTag |= ((descrPtr->srcTrunkId & 0x1F) << 19);
            mrvlTag |= (1 << 18);
        }
    }

    if (mrvlTagCmd == TAG_CMD_TO_TARGET_SNIFFER_E) {
        /* rx_sniff */
        mrvlTag |= ((descrPtr->rxSniffed & 0x01) << 18);
    }
    if (mrvlTagCmd == TAG_CMD_TO_CPU_E) {
        /* Src_Tagged */
        mrvlTag |= ((descrPtr->srcVlanTagged & 0x01) << 29);
        /* Src_Port */
        mrvlTag |= ((descrPtr->srcPort & 0x1F) << 19);
        /* CPU_Code[3:1] */
        mrvlTag |= ((descrPtr->bits15_2.cmd_trap2cpu.cpuCode & 0x0E) << 15);
        /* CPU_Code[0] */
        mrvlTag |= ((descrPtr->bits15_2.cmd_trap2cpu.cpuCode & 0x01) << 12);
    }

    /* add vlan tag with vpt */
    mrvlTag |= ((descrPtr->vid & 0xfff) |
                ((descrPtr->userPriorityTag & 0x7) << 13));

    *mrvlTagPtr = SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(mrvlTag);
}

/*******************************************************************************
*   snetSalsaPortProtocolCoS
*
* DESCRIPTION:
*        set cos by Port Protocol or Default TC config
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaPortProtocolTc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaPortProtocolTc);


    GT_U32 fldValue;
    GT_U32 regAddress;

    if (descrPtr->portProtMatch)
    {
        descrPtr->trafficClass = descrPtr->portProtTc;
    }
    else
    {
        BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);

        /* TC is mapped to DefaultTrafficClass */
        __LOG(("TC is mapped to DefaultTrafficClass"));
        smemRegFldGet(devObjPtr, regAddress, 6, 2, &fldValue);
        descrPtr->trafficClass = (GT_U8)fldValue;
    }
}
/*******************************************************************************
*   snetSalsaCoS
*
* DESCRIPTION:
*        CoS processing for frame
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaCoS
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaCoS);

    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U32 regAddress;
    GT_U8  dscpBitToStart;
    GT_U32 brgPortCntrlRegData;
    GT_U32 forceL3CosRegData = 0;
    GT_U8  frameDscp = 0;
    GT_BOOL forceL3Cos = GT_FALSE;   /* DA or SA match and
                                        ForceL3 is set in the DA or SA entry
                                        and packet IP*/
    GT_BOOL defaultPortUp = GT_FALSE;   /* Get UP from default value for port */
    GT_BOOL untagUpFromTc = GT_FALSE;   /* Get UP from TC to UP map */


    /* DSCP Assignment */

    /* TBD: Port is cascading or Basic Mode is enabled */

    if ((descrPtr->saDaTc != SNET_SALSA_DEFAULT_SA_DA_TC_CNS)
        && (descrPtr->forceL3Cos == 1)
        && (descrPtr->ipHeaderPtr))
    {
        /* DA or SA are found and ForceL3Cos is true and packet is IPv4
           Assign DSCP to ForceL3DSCP Register value */
        smemRegGet(devObjPtr, FORCE_L3_COS_REG, &forceL3CosRegData);
        descrPtr->dscp = (GT_U8)((forceL3CosRegData >> 8) & 0x3f);
        descrPtr->modifyDscpOrExp = 1;
        forceL3Cos = GT_TRUE;
    }

    if (descrPtr->ipHeaderPtr)
    {
        /* get DSCP from frame */
        __LOG(("get DSCP from frame"));
        frameDscp = (*(descrPtr->ipHeaderPtr + 1)) >> 2;
    }


    /* Traffic Class Assignment for Switched Packets */

    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegGet(devObjPtr, regAddress, &brgPortCntrlRegData);
    /* Override traffic class */
    if (((brgPortCntrlRegData >> 5) & 0x1) == 0)
    {
        /* DA or SA are found and ForceL3Cos is true and packet is IPv4
           Assign Traffic Class ForceL3TC Register value */
        if (forceL3Cos == GT_TRUE)
        {
           descrPtr->trafficClass = (GT_U8)((forceL3CosRegData >> 2) & 0x3);
        }
        else
        {
            /* MapDSCP2TC bit is enabled and packet is IPv4 */
            if (((brgPortCntrlRegData >> 27) & 0x1) && (descrPtr->ipHeaderPtr))
            {
                /* DSCP0, DSCP1 -> 0x02040200, DSCP2, DSCP3 -> 0x02040204 */
                /* calculating dscp bit to start read */
                if ((frameDscp % 2) == 0)
                {
                    dscpBitToStart = 2;
                }
                else
                {
                    dscpBitToStart = 10;
                }
                DSCP_TO_COS_REG(frameDscp, &regAddress);
                smemRegFldGet(devObjPtr, regAddress, dscpBitToStart, 2,
                                                                    &fldValue);
                descrPtr->trafficClass = (GT_U8)fldValue;
            }
            else
            {
                /* packet is tagged and MapUP2TC bit is enabled */
                if (((brgPortCntrlRegData >> 30) & 0x1) &&
                     descrPtr->srcVlanTagged )
                {
                    /* calculating start bit in UP2TC Register */
                    fldFirstBit = UP2TC_BIT(descrPtr->userPriorityTag);

                    /* UP2TCSel */
                    if (((brgPortCntrlRegData >> 26) & 0x1) == 0)
                    {
                        /* UP2TC Register 0 */
                        smemRegFldGet(devObjPtr, UP2TC_REG0,
                                      fldFirstBit, 2, &fldValue);
                    }
                    else
                    {
                        /* UP2TC Register 1 */
                        smemRegFldGet(devObjPtr, UP2TC_REG1,
                                      fldFirstBit, 2, &fldValue);

                    }
                    descrPtr->trafficClass = (GT_U8)fldValue;
                }

                /* DA or SA are found and Layer 2 Ingress Control
                   DiscardMacTC = 0 */
                else
                {
                    smemRegFldGet(devObjPtr, L2_INGRESS_CNT_REG, 22,
                                  1, &fldValue);
                    if ((descrPtr->saDaTc != SNET_SALSA_DEFAULT_SA_DA_TC_CNS)
                        && (fldValue == 0))
                    {
                        descrPtr->trafficClass = (GT_U8)descrPtr->saDaTc;
                    }
                    else
                    {
                        snetSalsaPortProtocolTc(devObjPtr, descrPtr);
                    }
                }
            }
        }
    }
    else
    {
        snetSalsaPortProtocolTc(devObjPtr, descrPtr);
    }

    /* User Priority Assignment */

    /* TBD: Port is cascading */

    if (descrPtr->srcVlanTagged)
        return;

    if (SKERNEL_DEVICE_FAMILY_SALSA_A2(devObjPtr->deviceType))
    {
        if (descrPtr->saDaTc != SNET_SALSA_DEFAULT_SA_DA_TC_CNS)
        {   /* DA or SA are found */

            if (forceL3Cos == GT_TRUE)
            {
                /* ForceL3Cos is true and Packet is IPv4/6 */
                /* Assign Traffic Class ForceL3DSCP Register value */
                descrPtr->userPriorityTag =
                    (GT_U8)((forceL3CosRegData >> 5) & 0x7);
            }
            else
            {
                /* Check mapDscp2UP field of bridge port control reg for IP */
                if ((descrPtr->ipHeaderPtr) &&
                    ((brgPortCntrlRegData >> 28) & 0x1))
                {/* Packet is IPv4/6 and MapDSCP2UP is enabled */
                    /* calculating dscp bit to start read */
                    if ((frameDscp % 2) == 0)
                    {
                        dscpBitToStart = 5;
                    }
                    else
                    {
                        dscpBitToStart = 13;
                    }
                    /* DSCP0, DSCP1 -> 0x02040200, DSCP2, DSCP3 -> 0x02040204 */
                    DSCP_TO_COS_REG(frameDscp, &regAddress);
                    smemRegFldGet(devObjPtr, regAddress,
                                  dscpBitToStart, 3, &fldValue);

                    descrPtr->userPriorityTag = (GT_U8)fldValue;
                }
                else
                {
                    /* UP is taken from default VPT*/
                    defaultPortUp = GT_TRUE;
                }
            }
        }
        else
        {
            /* UP is taken from default VPT*/
            defaultPortUp = GT_TRUE;
        }
    }
    else
    {
        untagUpFromTc = GT_TRUE;
    }

    if (defaultPortUp == GT_TRUE)
    {
        smemRegFldGet(devObjPtr, L2_INGRESS_CNT_REG, 18,
                      1, &fldValue);

        if (fldValue == 0)
        {
            /* VPT is set according to Bridge Port<n> ControlRegister */
            /* DefaultVPT[0] */
            descrPtr->userPriorityTag =
                (GT_U8)((brgPortCntrlRegData >> 8) & 0x1);

            /* DefaultVPT[1] */
            descrPtr->userPriorityTag |=
                (GT_U8)(((brgPortCntrlRegData >> 12) & 0x1) << 1);

            /* DefaultVPT[2] */
            descrPtr->userPriorityTag |=
                (GT_U8)(((brgPortCntrlRegData >> 19) & 0x1) << 2);
        }
        else
          untagUpFromTc = GT_TRUE;

    }

    if(untagUpFromTc == GT_TRUE)
    {

        /* get VPT from traffic class */
        fldFirstBit = (descrPtr->trafficClass & 0x3) * 3;
        smemRegFldGet(devObjPtr, TC_TO_VPT_REG, fldFirstBit, 3, &fldValue);
        descrPtr->userPriorityTag = (GT_U8)fldValue;
    }


}
/*******************************************************************************
*   snetSalsaProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
*******************************************************************************/
void snetSalsaProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaProcessInit);

    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_FRAME_DESCR_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("smemSalsaInit: allocation error\n");
    }

    /* initiation of internal twist functions */
    __LOG(("initiation of internal twist functions"));
    devObjPtr->devFrameProcFuncPtr = snetSalsaFrameProcess;
    devObjPtr->devPortLinkUpdateFuncPtr = snetSalsaLinkStateNotify;
    devObjPtr->devFdbMsgProcFuncPtr = sfdbSalsaMsgProcess;
    devObjPtr->devMacTblAgingProcFuncPtr = sfdbSalsaMacTableAging;
}

/*******************************************************************************
*   snetSalsaFrameProcess
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
*******************************************************************************/
static GT_VOID snetSalsaFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID             bufferId,
    IN GT_U32                  srcPort
)
{
    DECLARE_FUNC_NAME(snetSalsaFrameProcess);

    GT_BOOL retVal;

    SKERNEL_FRAME_DESCR_STC * descrPtr; /* pointer to the frame's descriptor */

    descrPtr = (SKERNEL_FRAME_DESCR_STC * )devObjPtr->descriptorPtr;
    memset(descrPtr, 0, sizeof(SKERNEL_FRAME_DESCR_STC));

    descrPtr->frameBuf = bufferId;
    descrPtr->srcPort = srcPort;
    descrPtr->byteCount = (GT_U16)bufferId->actualDataSize;

    /* Parsing the frame, get information from frame and fill descriptor */
    __LOG(("Parsing the frame, get information from frame and fill descriptor"));
    snetFrameParsing(devObjPtr, descrPtr);
    /* Rx MAC layer processing of the Salsa  */
    retVal = snetSalsaRxMacProcess(devObjPtr, descrPtr);
    if (retVal == GT_FALSE)
        return;

    snetSalsaRxMirror(devObjPtr, descrPtr);
    snetSalsaVlanTcAssign(devObjPtr, descrPtr);
    snetSalsaControlFrame(devObjPtr, descrPtr);
    snetSalsaMacRange(devObjPtr, descrPtr);
    snetSalsaFdbLookUp(devObjPtr, descrPtr);
    snetSalsaLockPort(devObjPtr, descrPtr);
    snetSalsaIngressFilter(devObjPtr, descrPtr);
    snetSalsaCoS(devObjPtr, descrPtr);
    snetSalsaL2Decision(devObjPtr, descrPtr);
    snetSalsaEgress(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetSalsaLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       port            - port number.
*       linkState   - link state (0 - down, 1 - up)
*
*******************************************************************************/
static void snetSalsaLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
)
{
    DECLARE_FUNC_NAME(snetSalsaLinkStateNotify);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;

    ASSERT_PTR(devObjPtr);

    MAC_STATUS_REG(port, &regAddress);
    smemRegFldSet(devObjPtr, regAddress, 0, 1, linkState);

    GOP_INTR_MASK_REG(port, &regAddress);
    fldFirstBit = INTR_CAUSE_LNK_STAT_CHANGED_BIT(port);
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
    /* Link status changed on port bit (interrupt mask register) */
    __LOG(("Link status changed on port bit (interrupt mask register)"));
    if (fldValue != 1)
        return;

    GOP_INTR_CAUSE_REG(port, &regAddress);
    /* Link status changed on port (interrupt cause register) */
    fldFirstBit = INTR_CAUSE_LNK_STAT_CHANGED_BIT(port);
    smemRegFldSet(devObjPtr, regAddress, fldFirstBit, 1, 1);
    /* Sum of all GOP interrupts */
    smemRegFldSet(devObjPtr, regAddress, 0, 1, fldValue);

    /* GOP(n) Interrupt Cause register0 SUM bit */
    fldFirstBit = GLB_INTR_CAUSE_GOP_SUM0(port);
    smemRegFldGet(devObjPtr, GLOBAL_INT_MASK_REG, fldFirstBit, 1, &fldValue);
    if (fldValue != 1)
        return;
    /* GOP(n) Interrupt cause register0 SUM bit */
    smemRegFldSet(devObjPtr, GLOBAL_INT_CAUSE_REG, fldFirstBit, 1, 1);
    /* General Interrupt SUM bit */
    smemRegFldSet(devObjPtr, GLOBAL_INT_CAUSE_REG, 0, 1, 1);

    /* Call interrupt */
    scibSetInterrupt(devObjPtr->deviceId);
}

/*******************************************************************************
*   snetSalsaRxMacProcess
*
* DESCRIPTION:
*       Rx MAC layer processing of the Salsa
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURNS:
*       GT_TRUE
*       GT_FALSE
*
*******************************************************************************/
static GT_BOOL snetSalsaRxMacProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaRxMacProcess);

    GT_U32 regAddress;              /* register's address */
    GT_U32 fldValue;                /* register field's value */
    GT_U8 cascadingPort;            /* cascading port */

    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 0, 1, &fldValue);
    /* Device Enable */
    __LOG(("Device Enable"));
    if (fldValue != 1)
        return GT_FALSE;

    if (descrPtr->srcPort > 24){
        regAddress = CPU_PORT_CONTROL_REG;
    }
    else {
        MAC_CONTROL_REG(descrPtr->srcPort, &regAddress);
    }
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* This bit is set to enable the port */
    if (fldValue != 1)
        return GT_FALSE;

    smemRegFldGet(devObjPtr, regAddress, 12, 1, &fldValue);
    /* CascadingPort */
    cascadingPort = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 4, 5, &fldValue);
    /* DeviceID[4:0] */
    descrPtr->srcDevice = (GT_U8)fldValue;

    snetSalsaTxMacCountUpdate(devObjPtr, descrPtr, descrPtr->srcPort);

    if (cascadingPort == 1) {
        return sstackSalsaFrameProcess(devObjPtr, descrPtr);
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetSalsaRxMirror
*
* DESCRIPTION:
*       Make RX mirroring
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaRxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaRxMirror);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_BOOL sflowStat;
    GT_U8 * dataPtr;
    GT_U32 dataLen;
    GT_U32 targetPort;
    GT_U32 targetDevice;

    dataPtr = descrPtr->frameBuf->actualDataPtr;
    dataLen = descrPtr->byteCount;

    sflowStat = snetSalsaSflowControl(devObjPtr, descrPtr, SNET_SFLOW_RX,
                                      descrPtr->srcPort, dataPtr, dataLen);
    if (sflowStat == GT_TRUE)
    {
        /* restore rxSniffed bit */
        __LOG(("restore rxSniffed bit"));
        descrPtr->rxSniffed = 0;
        return;
    }
    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 24, 1, &fldValue);
    /* If sniffer bit disable */
    if (fldValue != 1)
        return;

    descrPtr->rxSniffed = 1;
    /* Statistic Sniffing and Monitoring Traffic Class Conf Register */
    smemRegFldGet(devObjPtr, STATISTIC_SNIF_MONITOR_TC_REG, 2, 11, &fldValue);
    /* no packets are duplicated to the Tx or Tx monitor */
    if (fldValue == 0)
        return;

    smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 3, 5, &fldValue);
    /* The device number on which the Rx monitor resides */
    targetDevice = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 11, 5, &fldValue);
    /* The port number on which the Rx monitor resides */
    targetPort = (GT_U8)fldValue;


    smemRegFldGet(devObjPtr, STATISTIC_SNIF_MONITOR_TC_REG, 13, 1, &fldValue);
    /* When desc is rx_sniff, if set_orig_tc =1 pass the original desc tc.
    If not, pass the rx_sniff_prio.*/
    if (fldValue == 0)
    {
        smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 17, 2, &fldValue);
        /* TC2Target RxSniff */
        descrPtr->trafficClass = (GT_U8)fldValue;
    }

    /* Send packet */
    snetSalsaTx2Device(devObjPtr, descrPtr, dataPtr, dataLen, targetPort,
                       targetDevice);

    /* restore rxSniffed bit */
    descrPtr->rxSniffed = 0;

}

/*******************************************************************************
*   snetSalsaTxMirror
*
* DESCRIPTION:
*       Make TX mirroring
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       dataPtr     - pointer to the frame's data.
*       dataLen     - length of the frame's data.
*       egressPort  - egress port, which should be checked for Tx mirroring.
*
*******************************************************************************/
void snetSalsaTxMirror
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * dataPtr,
    IN GT_U32 dataLen,
    IN GT_U32 egressPort
)
{
    DECLARE_FUNC_NAME(snetSalsaTxMirror);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 trgDev;
    GT_U32 trgPort;
    GT_BOOL sflowStat;

    sflowStat = snetSalsaSflowControl(devObjPtr, descrPtr, SNET_SFLOW_TX,
                                      egressPort, dataPtr, dataLen);
    if (sflowStat == GT_TRUE)
    {
        return;
    }

    TRANSMIT_CONF_REG(egressPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 10, 1, &fldValue);
    /* Enable Egress traffic mirroring on this port (Tx sniffer enable)*/
    __LOG(("Enable Egress traffic mirroring on this port (Tx sniffer enable)"));
    if (fldValue != 1)
        return;

    smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 0, 5, &fldValue);
    /* Indicates the device number of the destination Tx sniffer */
    trgDev = fldValue;
    smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 7, 5, &fldValue);
    /* Indicates the port number of the destination Tx sniffer */
    trgPort = fldValue;
    descrPtr->txSniffed = 1;
    snetSalsaTx2Device(devObjPtr, descrPtr, dataPtr, dataLen, trgPort, trgDev);
}

/*******************************************************************************
*   snetSalsaTxMacCountUpdate
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
void snetSalsaTxMacCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 port
)
{
    if (port > 24) {
        snetSalsaCpuCountUpdate(devObjPtr, descrPtr, port);
    }
    else {
        snetSalsaMacMibCountUpdate(devObjPtr, descrPtr, port);
    }

}

/*******************************************************************************
*   snetSalsaCpuCountUpdate
*
* DESCRIPTION:
*        CPU Port Threshold and MIB Count Enable Registers
* INPUTS:
*        devObjPtr  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        port       - CPU  port
*
*******************************************************************************/
static void snetSalsaCpuCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
)
{
    DECLARE_FUNC_NAME(snetSalsaCpuCountUpdate);

    GT_U32 regAddress;
    GT_U32 fldValue;

    smemRegFldGet(devObjPtr, CPU_PORT_THLD_MIB_CNT_REG, 13, 1, &fldValue);
    /* CPUCountEn */
    __LOG(("CPUCountEn"));
    if (fldValue != 1)
        return;

    regAddress = CPU_PORT_GOOD_FRM_CNT_REG;
    smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
    /* GoodFrames ReceivedCnt */
    smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

    regAddress = CPU_PORT_GOOD_OCT_CNT_REG;
    smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
    /* GoodOctets ReceivedCnt */
    smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
}
/*******************************************************************************
*   snetSalsaMacMibCountUpdate
*
* DESCRIPTION:
*        Update Tx MAC counters
* INPUTS:
*        devObjPtr  - pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
*        port       - MIB counter's port
*
*******************************************************************************/
static void snetSalsaMacMibCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;

    MIB_COUNT_CONTROL_REG0(egressPort, &regAddress);
    fldFirstBit = PORT_COUNT_UPDATE(egressPort);
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
    /* Port<n>_CountEn */
    if (fldValue != 1)
        return;

    switch (descrPtr->macDaType) {
        case SKERNEL_MULTICAST_MAC_E:
            regAddress = portsCounters[egressPort].startOffset + 0x1C;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        case SKERNEL_BROADCAST_MAC_E:
            regAddress = portsCounters[egressPort].startOffset + 0x18;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        }
        switch (SNET_GET_NUM_OCTETS_IN_FRAME(descrPtr->byteCount)) {
        case SNET_FRAMES_1024_TO_MAX_OCTETS:
            regAddress = portsCounters[egressPort].startOffset + 0x34;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        case SNET_FRAMES_512_TO_1023_OCTETS:
            regAddress = portsCounters[egressPort].startOffset + 0x30;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        case SNET_FRAMES_256_TO_511_OCTETS:
            regAddress = portsCounters[egressPort].startOffset + 0x2C;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        case SNET_FRAMES_128_TO_255_OCTETS:
            regAddress = portsCounters[egressPort].startOffset + 0x28;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        case SNET_FRAMES_65_TO_127_OCTETS:
            regAddress = portsCounters[egressPort].startOffset + 0x24;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        case SNET_FRAMES_64_OCTETS:
            regAddress = portsCounters[egressPort].startOffset + 0x20;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
            break;
        }

        /* Good frames sent */
        regAddress = portsCounters[egressPort].startOffset + 0x40;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

}

/*******************************************************************************
*   snetSalsaTx2Device
*
* DESCRIPTION:
*
* INPUTS:
*        devObjPtr   - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        frameDataPtr - pointer to the frame's data.
*        frameDataLength - length of the frame's data.
*        targetPort - egress port.
*        targetDevice - egress device ID.
* OUTPUTS:
*       Returns octets number in frame
*
*******************************************************************************/
static void snetSalsaTx2Device
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8 * frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 targetPort,
    IN GT_U32 targetDevice
)
{
    DECLARE_FUNC_NAME(snetSalsaTx2Device);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 ownDevice;
    GT_U32 fldFirstBit;
    GT_U32 mrvlTag;
    GT_U32 egressBufLen, copySize;
    GT_U32 cascadePort;
    GT_U8_PTR egrBufPtr;
    GT_U8_PTR dataPtr;
    GT_U8_PTR mrvlTagPtr;
    GT_U8 vlan_member_ports[32];
    GT_U8 vlan_tagged_ports[32];
    GT_U8 stp_for_ports[32];


    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 4, 5, &fldValue);
    /* Device ID of 98DX240/160 Managed PP within a Prestera chipset*/
    ownDevice = fldValue;
    if (targetDevice == ownDevice){
        /* Get VLAN ports info */
        snetSalsaVlanPortsGet(devObjPtr, descrPtr->vid, vlan_member_ports,
                              vlan_tagged_ports, stp_for_ports);
        snetSalsaTx2Port(devObjPtr, descrPtr, targetPort,
                              vlan_tagged_ports[targetPort]);
    }
    else {
        TRG_DEV_CASCADE_PORT_REG(targetDevice, &regAddress);
        fldFirstBit = CASCADE_PORT(targetDevice);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 5, &fldValue);
        /* The local port through which packets destined to device are to be
           transmitted */
        cascadePort = fldValue;
        egrBufPtr = devObjPtr->egressBuffer1;
        dataPtr = frameDataPtr;
        copySize = 2 * SGT_MAC_ADDR_BYTES;

        /* Append DA and SA to the egress buffer */
        MEM_APPEND(egrBufPtr, dataPtr, copySize);
        dataPtr += copySize;

        if (cascadePort) {
            /* add marvell tag anyway */
            __LOG(("add marvell tag anyway"));
            snetCreateMarvellTag(descrPtr, &mrvlTag);
            mrvlTagPtr = (GT_U8_PTR) &mrvlTag;
            copySize = sizeof(mrvlTag);
            /* Copy Marvell tag */
            MEM_APPEND(egrBufPtr, mrvlTagPtr, copySize);

            if (descrPtr->srcVlanTagged == 1) {
                /* Skip VLAN tag */
                dataPtr += 4;
            }
        }

        copySize = frameDataLength - (dataPtr - frameDataPtr);

        /* Copy the rest of the frame data */
        MEM_APPEND(egrBufPtr, dataPtr, copySize);

        snetSalsaTxMacCountUpdate(devObjPtr, descrPtr, targetPort);

        egressBufLen = egrBufPtr - devObjPtr->egressBuffer1;
        smainFrame2PortSend(devObjPtr,
                            cascadePort,
                            devObjPtr->egressBuffer1,
                            egressBufLen,
                            GT_FALSE);
    }
}

/*******************************************************************************
*   snetSalsaVlanTcAssign
*
* DESCRIPTION:
*        Assign VLAN and Traffic Class
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaVlanTcAssign
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaVlanTcAssign);

    GT_32 tagData;
    GT_32 * tagDataPtr;
    SBUF_BUF_ID frameBuf;
    GT_U16 etherType;
    GT_U16 tagControl;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U8 * dataPtr;
    GT_U8 * etherType2Ptr;
    GT_U16 etherType2;
    GT_BOOL result;

    frameBuf = descrPtr->frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    tagDataPtr = &tagData;
    dataPtr += 2 * SGT_MAC_ADDR_BYTES;
    MEM_APPEND(tagDataPtr, dataPtr, 4);
    etherType = SGT_LIB_SWAP16_MAC(tagData);
    if (etherType == 0x8100) {
        tagControl = (GT_U16) SGT_LIB_SWAP16_MAC(tagData >> 16);
        descrPtr->srcVlanTagged = 1;
        descrPtr->vid = tagControl & 0xFFF;
        descrPtr->userPriorityTag = (tagControl >> 12) & 0x7;
        descrPtr->inputIncapsulation = 0;
        etherType2Ptr =(frameBuf->actualDataPtr + 12 + 4);
    }
    else {
        descrPtr->inputIncapsulation = 1;
        etherType2Ptr =(frameBuf->actualDataPtr + 12);
    }

    /* set L3 and L4 headers */
    __LOG(("set L3 and L4 headers"));
    etherType2 = (etherType2Ptr[0] << 8) | etherType2Ptr[1];
    if (etherType2 == 0x0800) {
        GT_U32 headerSize;
        descrPtr->ipHeaderPtr = ((GT_U8 *)etherType2Ptr) + 2;
        headerSize = (descrPtr->ipHeaderPtr[0] & 0x0f) * 4;
        /* Calculate IP_OPTIONS size and SET */
        descrPtr->l4HeaderPtr = descrPtr->ipHeaderPtr + headerSize;
    }
    else if((etherType2 == 0x0806))
    {
        descrPtr->frameType = SKERNEL_ARP_E;
    }

    if (descrPtr->srcVlanTagged != 1)
    {
       /* frame is untagged */
        __LOG(("frame is untagged"));

        /* Bridge Port<n> Control Register */
        BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 18, 1, &fldValue);

        /* ProtBased VLANEn */
        result = GT_FALSE;
        if (fldValue == 1)
        {
            result = snetPortProtocolVidGet(devObjPtr, etherType, descrPtr);
        }

        /* port based vlans based VLAN */
        if (result != GT_TRUE)
        {
            /* get PVID register's address */
            regAddress = 0x02000004 + (descrPtr->srcPort / 2) * 0x2000;

            /* For  odd ports PVID is in bits 16 : 27.
               For even ports PVID is in bits 0 : 11. */
            fldFirstBit = (descrPtr->srcPort % 2) ? 16 : 0;

            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 12, &fldValue);

            /* Vid */
            descrPtr->vid = (GT_U16)fldValue;

            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 6, 2, &fldValue);
            /* DefaultTraffic-Class */
            descrPtr->trafficClass = (GT_U8)fldValue;

        }
    }
}

/*******************************************************************************
*   snetPortProtocolVidGet
*
* DESCRIPTION:
*        Get port protocol VID
* INPUTS:
*        devObjPtr -  pointer to the device object.
*        descrPtr  -  pointer to the frame's descriptor.
*        etherType - ethernet type
* RETURN: True - classified or False not classified
*
*******************************************************************************/
static GT_BOOL snetPortProtocolVidGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 etherType,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetPortProtocolVidGet);

    GT_U32 fldValue;
    GT_U32 prot;
    GT_U32 srcPort;
    GT_U32 regAddress;
    GT_BOOL result = GT_FALSE;

    srcPort = descrPtr->srcPort;

    for (prot = 0; prot < 4; prot++) {
        smemRegFldGet(devObjPtr, PROTOCOL_ENCAP0_REG, 0, 1, &fldValue);
        /* EtherType0 EthernetEn */
        __LOG(("EtherType0 EthernetEn"));
        if (fldValue == 0)
            continue;

        switch (prot) {
        case 0:
            smemRegFldGet(devObjPtr, PROTOCOL0_REG, 0, 16, &fldValue);
            /* EtherType0 */
            if (etherType != fldValue)
                break;
            PROTOCOL_VID0_REG(srcPort, &regAddress);
            smemRegGet(devObjPtr, regAddress, &fldValue);
            /* Vid assigned to a packet with user defined protocol 0 */
            descrPtr->vid = (GT_U16)(fldValue & 0xfff);

            /* for future TrafficClass assignment to a packet
                with user defined protocol 0 */
            descrPtr->portProtTc = (GT_U8)((fldValue >> 12) & 0x3);
            descrPtr->portProtMatch = GT_TRUE;
            return GT_TRUE;
        case 1:
            smemRegFldGet(devObjPtr, PROTOCOL0_REG, 16, 16, &fldValue);
            /* EtherType1 */
            if (etherType != fldValue)
                break;
            PROTOCOL_VID0_REG(srcPort, &regAddress);
            smemRegGet(devObjPtr, regAddress, &fldValue);
            /* Vid assigned to a packet with user defined protocol 1 */
            descrPtr->vid = (GT_U16)((fldValue >> 16) & 0xfff);

            /* for future TrafficClass assignment to a packet
                with user defined protocol 1 */
            descrPtr->portProtTc = (GT_U8)((fldValue >> 28) & 0x3);
            descrPtr->portProtMatch = GT_TRUE;
            return GT_TRUE;
        case 2:
            smemRegFldGet(devObjPtr, PROTOCOL1_REG, 0, 16, &fldValue);
            /* EtherType2 */
            if (etherType != fldValue)
                break;

            PROTOCOL_VID1_REG(srcPort, &regAddress);
            smemRegGet(devObjPtr, regAddress, &fldValue);
            /* Vid assigned to a packet with user defined protocol 2 */
            descrPtr->vid = (GT_U16)(fldValue & 0xfff);

            /* for future TrafficClass assignment to a packet
                with user defined protocol 2 */
            descrPtr->portProtTc = (GT_U8)((fldValue >> 12) & 0x3);
            descrPtr->portProtMatch = GT_TRUE;
            return GT_TRUE;
        case 3:
            /* EtherType3 */
            smemRegFldGet(devObjPtr, PROTOCOL1_REG, 16, 16, &fldValue);
            if (etherType != fldValue)
                break;

            PROTOCOL_VID1_REG(srcPort, &regAddress);
            smemRegGet(devObjPtr, regAddress, &fldValue);
            /* Vid assigned to a packet with user defined protocol 3 */
            descrPtr->vid = (GT_U16)((fldValue >> 16) & 0xfff);

            /* for future TrafficClass assignment to a packet
                with user defined protocol 3 */
            descrPtr->portProtTc = (GT_U8)((fldValue >> 28) & 0x3);
            descrPtr->portProtMatch = GT_TRUE;
            return GT_TRUE;
        }
    }

    return result;
}

/*******************************************************************************
*   snetMacRangeFilterGet
*
* DESCRIPTION:
*        Get MAC range filter
* INPUTS:
*        devObjPtr -  pointer to the device object.
*        descrPtr  -  pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetMacRangeFilterGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetMacRangeFilterGet);

    /* let twist-D alike do the hard work */
    __LOG(("let twist-D alike do the hard work"));
    snetTwistMacRangeFilterGet(devObjPtr,descrPtr);
}

/*******************************************************************************
*   snetSalsaTx2Port
*
* DESCRIPTION:
*        Forward frame to target port
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        egressPort -   number of egress port.
*        tagged  -      send frame with tag.
*
*
*******************************************************************************/
static GT_BOOL snetSalsaTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    IN GT_U8 tagged
)
{
    DECLARE_FUNC_NAME(snetSalsaTx2Port);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U8_PTR egrBufPtr;
    GT_U8_PTR dataPtr;
    GT_U32 egressBufLen, copySize;
    GT_U32 mrvlTag;
    SBUF_BUF_ID frameBuf;
    GT_U32 cascadField;
    GT_U8 tagData[4];
    GT_U32 add2bytes;

    if (egressPort > 24) {
        regAddress = CPU_PORT_CONTROL_REG;
    }
    else {
        MAC_CONTROL_REG(egressPort, &regAddress);
    }
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* Mac control PortEn register field */
    __LOG(("Mac control PortEn register field"));
    if (fldValue == 0)
        return GT_TRUE;

    if ((descrPtr->srcTrunkId == 0 &&
         egressPort == descrPtr->srcPort &&
         descrPtr->srcDevice == devObjPtr->deviceId) ||

        (descrPtr->srcTrunkId &&
         descrPtr->useVidx == 0 &&
         descrPtr->bits15_2.useVidx_0.targetIsTrunk &&
         descrPtr->srcTrunkId == descrPtr->bits15_2.useVidx_0.targedPort)
         )
    {
        /* Bridge Port<n> Control Register */
        __LOG(("Bridge Port<n> Control Register"));
        BRDG_PORT0_CTRL_REG(egressPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 21, 1, &fldValue);
        /* The packet is allowed to be L2 switched back to its local ports */
        if (fldValue != 1)
            return GT_FALSE;
    }

    smemRegFldGet(devObjPtr, regAddress, 12, 1, &fldValue);
    /* Mac control CascadingPort register field */
    cascadField = fldValue;
    egrBufPtr = devObjPtr->egressBuffer;
    frameBuf = descrPtr->frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    copySize = 2 * SGT_MAC_ADDR_BYTES;

    if (egressPort > 24) {
        /* add 2 bytes if need when send to cpu */
        smemRegFldGet(devObjPtr, regAddress, 31, 1, &add2bytes);
        if (add2bytes)
        {
            devObjPtr->egressBuffer[0] = devObjPtr->egressBuffer[1] = 0;
            egrBufPtr += 2;
        }
    }

    /* Append DA and SA to the egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr += copySize;

    if (cascadField == 1) {
        /* add Marvell tag anyway */
        snetCreateMarvellTag(descrPtr, &mrvlTag);
        copySize = sizeof(mrvlTag);
        /* Copy Marvell tag */
        MEM_APPEND(egrBufPtr, ((GT_U8_PTR)&mrvlTag), copySize);

        /* skip VLAN tag if need it */
        if (descrPtr->srcVlanTagged == 1) {
            dataPtr += 4;
        }
    }
    else
    {
        if (tagged) {
            if (descrPtr->srcVlanTagged == 0) {
                /* Create and insert VLAN tag */
                snetTagDataGet(descrPtr->userPriorityTag, descrPtr->vid,GT_FALSE, tagData );
                copySize = sizeof(tagData);
                /* Copy VLAN tag */
                MEM_APPEND(egrBufPtr, tagData, copySize);
            }
        }
        else {
            if (descrPtr->srcVlanTagged == 1)
            {
                /* skip VLAN tag bytes */
                dataPtr += 4;
            }
        }
    }

    copySize = frameBuf->actualDataSize - (dataPtr - frameBuf->actualDataPtr);

    /* Copy the rest of the frame data */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;
    snetSalsaTxMirror(devObjPtr, descrPtr, devObjPtr->egressBuffer,
                      egressBufLen, egressPort);

    snetSalsaTxMacCountUpdate(devObjPtr, descrPtr, egressPort);

    smainFrame2PortSend(devObjPtr,
                        egressPort,
                        devObjPtr->egressBuffer,
                        egressBufLen,
                        GT_FALSE);

    return GT_TRUE;
}

/*******************************************************************************
*   snetSalsaTx2Cpu
*
* DESCRIPTION:
*        Processing of trapped to CPU frames
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*******************************************************************************/
static void snetSalsaTx2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 cpuPort;
    GT_U32 mrvlTag;
    GT_U32 egressBufLen, copySize;
    GT_U8 macControlEn;
    GT_U8 cascadPort;
    GT_U8 * egrBufPtr;
    GT_U8 * dataPtr, * mrvlTagPtr;
    SBUF_BUF_ID frameBuf;

    smemRegFldGet(devObjPtr, TRANS_QUEUE_CONTROL_REG, 4, 5, &fldValue);
    /* CPU PortNum */
    cpuPort = fldValue;

    regAddress = CPU_PORT_CONTROL_REG;
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* PortEn */
    macControlEn = (GT_U8)fldValue;
    if (macControlEn == 0)
        return;

    smemRegFldGet(devObjPtr, regAddress, 12, 1, &fldValue);
    /* CascadingPort */
    cascadPort = (GT_U8)fldValue;
    frameBuf = descrPtr->frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    egrBufPtr = devObjPtr->egressBuffer;
    mrvlTag = 0;
    if (cascadPort == 1) {
        copySize = 2 * SGT_MAC_ADDR_BYTES;
        /* Append DA and SA to the egress buffer */
        MEM_APPEND(egrBufPtr, dataPtr, copySize);
        dataPtr += copySize;

        snetCreateMarvellTag(descrPtr, &mrvlTag);
        mrvlTagPtr = (GT_U8_PTR) &mrvlTag;
        copySize = sizeof(mrvlTag);
        /* Copy Marvell tag */
        MEM_APPEND(egrBufPtr, mrvlTagPtr, copySize);

        if (descrPtr->srcVlanTagged == 1) {
            /* Skip VLAN tag */
            dataPtr += 4;
        }

        copySize = frameBuf->actualDataSize -
                   (dataPtr - frameBuf->actualDataPtr);

        /* Copy the rest of the frame data */
        MEM_APPEND(egrBufPtr, dataPtr, copySize);
    }
    else {
        /* COPY all from  descrPtr->frameBuf to the devObjPtr->egressBuffe */
        copySize = descrPtr->frameBuf->actualDataSize;
        MEM_APPEND(egrBufPtr, dataPtr, copySize);
    }

    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    snetSalsaTxMirror(devObjPtr, descrPtr, devObjPtr->egressBuffer,
                      egressBufLen, cpuPort);

    snetSalsaTxMacCountUpdate(devObjPtr, descrPtr, cpuPort);

    smainFrame2PortSend(devObjPtr,
                        cpuPort,
                        devObjPtr->egressBuffer,
                        egressBufLen,
                        GT_FALSE);
}
/*******************************************************************************
*   snetSalsaControlFrame
*
* DESCRIPTION:
*        Control frame's processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaControlFrame
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;

    if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E) {
        if (SGT_MAC_ADDR_IS_BPDU(descrPtr->dstMacPtr)) {
            descrPtr->frameType = SKERNEL_BPDU_E;
            descrPtr->controlTrap = 1;
            descrPtr->pbduTrap = 1;
        }
    }
    else {
        if (descrPtr->frameType == SKERNEL_ARP_E) {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 13, 1, &fldValue);
            /* TrapARPEn */
            if (fldValue == 1) {
                descrPtr->arpTrap = 1;
            }
        }
        else {
            GT_U8 retVal;

            if (descrPtr->ipHeaderPtr == NULL)
                return;

            SGT_IP_ADDR_IS_IGMP(descrPtr->ipHeaderPtr, &retVal);

            if (retVal == GT_TRUE) {
                descrPtr->frameType = SKERNEL_IGMP_E;
                /* Bridge Port<n> Control Register */
                BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
                smemRegFldGet(devObjPtr, regAddress, 14, 1, &fldValue);
                /* TrapIGMPEn */
                if (fldValue == 1) {
                    descrPtr->igmpTrap = 1;
                }
            }
        }
    }
}

/*******************************************************************************
*   snetSalsaMacRange
*
* DESCRIPTION:
*        MAC range filtering
* INPUTS:
*        devObjPtr -  pointer to device object.
*        descrPtr  -  pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -  pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaMacRange
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    if (descrPtr->controlTrap == 1)
        return;

        /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 10, 1, &fldValue);
    /* MACRange FilterEn */
    if (fldValue == 0)
        return;

    snetMacRangeFilterGet(devObjPtr, descrPtr);
}


/*******************************************************************************
*   snetSalsaGetPrivEdgeVLANEnable
*
* DESCRIPTION:
*        Checks if PVLAN enabled on port and sets target info accordingly.
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaGetPrivEdgeVLANEnable
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaGetPrivEdgeVLANEnable);

    GT_U32 regAddress;
    GT_U32 fldValue;

    if(descrPtr->srcPort > 24)
    {
        /* feature not works on CPU port */
        __LOG(("feature not works on CPU port"));
        return;
    }

    smemRegFldGet(devObjPtr, L2_INGRESS_CNT_REG, 30, 1, &fldValue);
    /* Check if PrivEdgeVLAN is enabled in L2_Ingress Control Register */
    if (fldValue == 1)
    {
        PVLAN_PORT0_REG(descrPtr->srcPort, &regAddress);
        smemRegGet(devObjPtr, regAddress, &fldValue);
        if (fldValue & 0x1)
        {
            /* PVLAN of the SA port is enabled*/
            descrPtr->privEdgeVlanEn = GT_TRUE;
            descrPtr->useVidx = 0;

            descrPtr->bits15_2.useVidx_0.targedDevice =
                                            (GT_U8)((fldValue >> 8) & 0x1f);
            descrPtr->bits15_2.useVidx_0.targedPort =
                                            (GT_U8)((fldValue >> 2) & 0x1f);

            if ((fldValue >> 1) & 0x1)
            {
                /* The target of the private Vlan Edge is Trunk */
                descrPtr->bits15_2.useVidx_0.targetIsTrunk = 1;
            }
            else
            {
                descrPtr->bits15_2.useVidx_0.targetIsTrunk = 0;
            }
        }
    }
}

/*******************************************************************************
*   snetSalsaFdbLookUp
*
* DESCRIPTION:
*        Look up in the bridge fdb
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  -    pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaFdbLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 srcPort;
    GT_U8 * srcMacPtr;
    SNET_SALSA_MAC_TBL_STC macEntry;
    GT_BOOL found;
    GT_U16 vid;

    srcPort = descrPtr->srcPort;
    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 5, &fldValue);
    /* TrunkGroupId */
    if (fldValue)
        descrPtr->srcTrunkId = (GT_U16)fldValue;

    srcMacPtr = SRC_MAC_FROM_DSCR(descrPtr);
    vid = descrPtr->vid;

    /* initializing Cos fields */
    descrPtr->saDaTc = SNET_SALSA_DEFAULT_SA_DA_TC_CNS;

    found = snetSalsaFindMacEntry(devObjPtr, srcMacPtr, vid, &macEntry, NULL);
    if (found) {
        descrPtr->macSaLookupResult = 1;
        descrPtr->saLookUpCmd = (GT_U8)macEntry.saCmd;
        descrPtr->forceL3Cos = (GT_U8)macEntry.forceL3Cos;
        descrPtr->saDaTc = (GT_U8)macEntry.saPriority;
        macEntry.aging = 1;
        if (macEntry.trunk == 1) {
            if (macEntry.trunk != descrPtr->srcTrunkId) {
                if (macEntry.staticEntry == 1)
                    descrPtr->l2Move = 2;
                else
                    descrPtr->l2Move = 1;
            }
        }
        else {
            if (macEntry.port != descrPtr->srcPort) {
                if (macEntry.staticEntry == 1)
                    descrPtr->l2Move = 2;
                else
                    descrPtr->l2Move = 1;
            }
        }
    }
    if (descrPtr->controlTrap == 1)
        return;


    /* destination MAC lookup */
    found = snetSalsaFindMacEntry(devObjPtr, descrPtr->dstMacPtr, vid,
                                 &macEntry, NULL);
    if (found) {
        if (descrPtr->saDaTc == SNET_SALSA_DEFAULT_SA_DA_TC_CNS)
        {
            descrPtr->saDaTc = (GT_U8)macEntry.daPriority;
        }
        else
        {
            descrPtr->saDaTc = MAX(descrPtr->saDaTc, macEntry.daPriority);
        }

        /* if forceL3Cos bit wasn't enabled in SA */
        if (descrPtr->forceL3Cos == 0x0)
        {
            descrPtr->forceL3Cos = (GT_U8)macEntry.forceL3Cos;
        }

        descrPtr->daLookUpCmd = (GT_U8)macEntry.daCmd;
        descrPtr->macDaLookupResult = 1;
        if (macEntry.daCmd == 0) {
            if ((macEntry.multiply && macEntry.staticEntry
                && descrPtr->macDaType == SKERNEL_UNICAST_MAC_E)
                        ||
                descrPtr->macDaType != SKERNEL_UNICAST_MAC_E)
            {
                descrPtr->useVidx = 1;
                descrPtr->bits15_2.useVidx_1.vidx = macEntry.vidx;
            }
            else
            {
                if (macEntry.trunk == 1) {
                    descrPtr->bits15_2.useVidx_0.targetIsTrunk = 1;
                    descrPtr->bits15_2.useVidx_0.targedPort =
                                                        (GT_U8)macEntry.port;
                }
                else {
                    descrPtr->bits15_2.useVidx_0.targetIsTrunk = 0;
                    descrPtr->bits15_2.useVidx_0.targedPort =
                                                        (GT_U8)macEntry.port;
                }
            }
        }
    }
    /* Checking if Private Edge Vlan is enabled */
    snetSalsaGetPrivEdgeVLANEnable(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetSalsaMacHashCalc
*
* DESCRIPTION:
*        Calculates the hash index for the mac address table
* INPUTS:
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
*        isVlanUsed  -  use vlan tag.
* RETURNS:
*        The hash index
*******************************************************************************/
static GT_U32 snetSalsaMacHashCalc
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U16 vid,
    IN GT_U8 isVlanUsed
)
{
    GT_ETHERADDR addr;
    GT_U32 hash;
    GT_U8 retVal;

    ASSERT_PTR(macAddrPtr);

    memcpy(addr.arEther, macAddrPtr, 6);

    if (SKERNEL_DEVICE_FAMILY_SALSA_A1(devObjPtr->deviceType))
    {
        retVal = salsaMacHashCalc(&addr, vid, isVlanUsed, &hash);
    }
    else
    {
        retVal = salsa2MacHashCalc(&addr, vid, isVlanUsed, &hash);
    }
    if (GT_BAD_PARAM == retVal)
        skernelFatalError("snetSalsaMacHashCalc : bad parameters\n");

    return hash;
}

/*******************************************************************************
*   snetSalsaGetFreeMacEntryAddr
*
* DESCRIPTION:
*        Look up in the MAC table for the first free MAC entry address
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        macAddrPtr  -  pointer to the first byte of MAC address.
*        vid         -  vlan tag.
* OUTPUTS:
*******************************************************************************/
GT_U32 snetSalsaGetFreeMacEntryAddr
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U8 * macAddrPtr,
    IN  GT_U16 vid
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 hashIdx;
    GT_U32 hashBucketSize;
    GT_U8 isVlanUsed;
    GT_U16 i;
    GT_U32 workHashBucketSize;

    SNET_SALSA_MAC_TBL_STC salsaMacEntryPtr;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(macAddrPtr);

    smemRegFldGet(devObjPtr, MAC_TABLE_CTRL_REG, 4, 1, &fldValue);
    /* VlanLookup-Mode */
    isVlanUsed = (GT_U8)fldValue;
    hashIdx = snetSalsaMacHashCalc(devObjPtr, macAddrPtr, vid, isVlanUsed);

    smemRegFldGet(devObjPtr, MAC_TABLE_CTRL_REG, 0, 3, &fldValue);
    /* NumOfEntries */
    workHashBucketSize = hashBucketSize = (fldValue + 1) * 2;
    if(hashBucketSize + hashIdx > 0x2000)
    {
        workHashBucketSize = 0x2000 - hashIdx;
    }
    for (i = 0; i < workHashBucketSize; i++) {
        regAddress = MAC_TAB_ENTRY_WORD0_REG + (hashIdx + i) * 0x10;
        snetGetMacEntry(devObjPtr, regAddress, &salsaMacEntryPtr);
        if (salsaMacEntryPtr.validEntry == 0 || salsaMacEntryPtr.skipEntry)
            return regAddress;
    }

    return NOT_VALID_ADDR;
}

/*******************************************************************************
* snetSetMacEntry
*
* DESCRIPTION:
*        Set MAC table new entry by free MAC address
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        regAddress  -  MAC entry address.
*        salsaMacEntryPtr - MAC entry pointer
*******************************************************************************/
static void snetSetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddress,
    IN SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr
)
{
    GT_U32 hwData[3];       /* 3 words of mac address hw entry  */

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(salsaMacEntryPtr);

    memset(hwData, 0, sizeof(hwData));
    /* MAC table entry word0 */
    hwData[0] =
       (salsaMacEntryPtr->validEntry |
        salsaMacEntryPtr->skipEntry << 1 |
        salsaMacEntryPtr->aging << 2 |
        salsaMacEntryPtr->trunk << 3 |
        salsaMacEntryPtr->vid << 4 |
        SGT_LIB_SWAP16_MAC(salsaMacEntryPtr->macAddr.w.hword2) << 16);


    /* MAC table entry word1 */
    hwData[1] =
       SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(salsaMacEntryPtr->macAddr.w.word1);

    /* MAC table entry word2 */
    hwData[2] =
       (salsaMacEntryPtr->port |
        salsaMacEntryPtr->vidx << 5 |
        salsaMacEntryPtr->saPriority << 14 |
        salsaMacEntryPtr->daPriority << 16 |
        salsaMacEntryPtr->staticEntry << 18 |
        salsaMacEntryPtr->multiply << 19 |
        salsaMacEntryPtr->forceL3Cos << 20 |
        salsaMacEntryPtr->daCmd << 21 |
        salsaMacEntryPtr->saCmd << 23 |
        salsaMacEntryPtr->rxSniff << 25);

    smemMemSet(devObjPtr, regAddress, hwData, 3);
}
/*******************************************************************************
*   snetSetFdbFifoEntry
*
* DESCRIPTION:
*        Set MAC Update Message by free FIFO entry pointer
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr    -  descriptor
*        freeFifoEntryPtr  -  FIFI entry pointer.
*        salsaMacEntryPtr - MAC entry pointer
*
*******************************************************************************/
static void snetSetFdbFifoEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 * fifoPtr,
    IN SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr
)
{
    GT_U32 hwData[4];       /* 4 words of of 32 bits message */
    GT_U32 messageType;

    if (descrPtr->islockPort)
    {
        /* SA message */
        messageType = 5;
    }
    else
    {
        /* NA message */
        messageType = 0;
    }

    memset(hwData, 0, sizeof(hwData));
    hwData[0] =
       (2 | /* Must be 2 */
        messageType << 4 | /* message type */
/*        0 << 15 | */  /* Entry was not found */
        SGT_LIB_SWAP16_MAC(salsaMacEntryPtr->macAddr.w.hword2) << 16 );

    hwData[1] =
       SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(salsaMacEntryPtr->macAddr.w.word1);

    hwData[2] =
       (salsaMacEntryPtr->vid |
        salsaMacEntryPtr->skipEntry << 12 |
        salsaMacEntryPtr->aging << 13 |
        salsaMacEntryPtr->trunk << 14 |
        salsaMacEntryPtr->port << 24 |
        salsaMacEntryPtr->vidx << 30);

    hwData[3] =
       (salsaMacEntryPtr->vidx |
/*        0 << 7 | */
        salsaMacEntryPtr->saPriority << 12 |
/*        0 << 14 | */
        salsaMacEntryPtr->daPriority << 15 |
        salsaMacEntryPtr->daCmd << 22 |
        salsaMacEntryPtr->saCmd << 24 |
        salsaMacEntryPtr->rxSniff << 27);

        memcpy(fifoPtr, hwData, sizeof(hwData));
}

/*******************************************************************************
*   snetGetMacEntry
*
* DESCRIPTION:
*        Get MAC table entry by index in the hash table
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        regAddress  -  MAC entry address.
* OUTPUTS:
*        salsaMacEntryPtr - pointer to the MAC table entry or NULL if not found.
*
*******************************************************************************/
static void snetGetMacEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddress,
    OUT SNET_SALSA_MAC_TBL_STC * salsaMacEntryPtr
)
{
    GT_U32 fldValue;
    GT_U32 * macEntry;

    macEntry = smemMemGet(devObjPtr, regAddress);

    memset(salsaMacEntryPtr, 0, sizeof(SNET_SALSA_MAC_TBL_STC));

    /* Get mac address*/
    sfdbMsg2Mac(macEntry, &salsaMacEntryPtr->macAddr);

    /* MAC table entry word0 */
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* Valid/Invalid entry */
    salsaMacEntryPtr->validEntry = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress, 1, 1, &fldValue);
    /* Skip (used to delete this entry) */
    salsaMacEntryPtr->skipEntry = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress, 2, 1, &fldValue);
    /* Aging - This bit is used for the Aging process. */
    salsaMacEntryPtr->aging = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress, 3, 1, &fldValue);
    /* Aging - This bit is used for the Aging process. */
    salsaMacEntryPtr->trunk = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress, 4, 12, &fldValue);
    /* VLAN id - holds the VLAN identification number. */
    salsaMacEntryPtr->vid = (GT_U16)fldValue;

    /* MAC table entry word2 */
    smemRegFldGet(devObjPtr, regAddress + 0x8, 0, 5, &fldValue);
    /* port# / trunk# */
    salsaMacEntryPtr->port = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 5, 9, &fldValue);
    /* Multicast group table index. */
    salsaMacEntryPtr->vidx = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 14, 2, &fldValue);
    /* SA priority */
    salsaMacEntryPtr->saPriority = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 16, 2, &fldValue);
    /* DA priority */
    salsaMacEntryPtr->daPriority = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 18, 1, &fldValue);
    /* Static entry */
    salsaMacEntryPtr->staticEntry = (GT_U8)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 19, 1, &fldValue);
    /* Multiply */
    salsaMacEntryPtr->multiply = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 20, 1, &fldValue);
    /* Forces L3 class of service assignment */
    salsaMacEntryPtr->forceL3Cos = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 21, 1, &fldValue);
    /* DA_CMD */
    salsaMacEntryPtr->daCmd = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 23, 1, &fldValue);
    /* SA_CMD */
    salsaMacEntryPtr->saCmd = (GT_U16)fldValue;
    smemRegFldGet(devObjPtr, regAddress + 0x8, 25, 1, &fldValue);
    /* Rx_Sniff */
    salsaMacEntryPtr->rxSniff = (GT_U16)fldValue;
}

/*******************************************************************************
*   snetSalsaLockPort
*
* DESCRIPTION:
*        Lock port processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static  void snetSalsaLockPort
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 15, 1, &fldValue);
    /* Locked Port */
    descrPtr->islockPort = (GT_U8)fldValue;
    descrPtr->learnEn = (GT_U8)fldValue^1;

    if (descrPtr->learnEn == 1)
        return;

    if (descrPtr->macSaLookupResult == 0) {
        smemRegFldGet(devObjPtr, regAddress, 16, 2, &fldValue);
        /* LockedPort CMD */
        if (fldValue != 0) {
            descrPtr->lockPortCmd = (GT_U8)fldValue;
        }
    }
    else
    if (descrPtr->l2Move != 0)
    {
        smemRegFldGet(devObjPtr, regAddress, 16, 2, &fldValue);
        /* LockedPort CMD */
        if (fldValue != 0) {
            descrPtr->lockPortCmd = (GT_U8)fldValue;
        }
    }
}

/*******************************************************************************
*   snetSalsaIngressFilter
*
* DESCRIPTION:
*        STP and ingress filter processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static  void snetSalsaIngressFilter
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSalsaIngressFilter);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 fldFirstBit;
    GT_U32 vlanValid;
    GT_U32 intVlanIdx;
    GT_U32 stgId = 0;
    GT_U32 srcPort;
    GT_U32 stpState;
    GT_U32 vidEntry;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    vidEntry = descrPtr->vid / 2;
    /* VLAN Translation Table (VLT) */
    VLAN_TRANS_TABLE_REG(vidEntry, &regAddress);
    fldFirstBit = VID_VALID(descrPtr->vid);
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
    /* Valid bit for each VLAN Entry (for Ingress filtering) */
    vlanValid = fldValue;

    fldFirstBit = VID_INTERNAL_VLAN_ENTRY_PTR(descrPtr->vid);
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 8, &fldValue);
    /* Pointer to the Internal VLAN Entry */
    intVlanIdx = fldValue;

    srcPort = descrPtr->srcPort;

    if (vlanValid == 1) {
        /* Internal VLAN Table */
        __LOG(("Internal VLAN Table"));
        INTERNAL_VLAN_TABLE(intVlanIdx, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
        /* VLAN table valid bit */
        vlanValid = fldValue;
        smemRegFldGet(devObjPtr, regAddress, 15, 6, &fldValue);
        /* Pointer to the Span State table */
        stgId = fldValue;
        smemRegFldGet(devObjPtr, regAddress, 1, 1, &fldValue);
        /* learn_en */
        descrPtr->learnEn = (GT_U8)fldValue;
    }
    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 9, 1, &fldValue);
    /* Secure VLAN */
    if (fldValue == 1) { /* do ingress filtering check */
        if (vlanValid == 1) {
            fldFirstBit =
            INTERNAL_VTABLE_PORT_OFFSET(intVlanIdx, srcPort, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* Port<n> member of this VLAN */
            if (fldValue == 0) {
               descrPtr->ingressFilterOut = 1;
               /*return;* don't return get stp state anyway */
            }
        }
        else {
            descrPtr->ingressFilterOut = 1;
            /*return;** don't return get stp state anyway */
        }
    }
    if (vlanValid == 1) {
        fldFirstBit = SPAN_STATE_TABLE_PORT(stgId, srcPort, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 2, &fldValue);
        /* port0_span_state */
        stpState = fldValue;
    }
    else {
        stpState = 3;
    }

    if (stpState == 1) {
        descrPtr->learnEn = 0;
    }

    descrPtr->stpState = (GT_U8)stpState;
}
/*******************************************************************************
*   snetSalsaL2Decision
*
* DESCRIPTION:
*        snetSalsaL2Decision
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static  void snetSalsaL2Decision
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 dropCode;
    GT_U32 cpuCode;
    GT_U32 securityDrop;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    dropCode = 0;
    securityDrop = 0;

    if(descrPtr->pbduTrap == 1)
    {
        if (descrPtr->stpState == 0)
        {
            /* Check STP state - if disabled than process BPDU as multicast */
            descrPtr->pbduTrap = 0; /* don't trap the packet to cpu */
            descrPtr->controlTrap = 0;
        }
    }

    if (descrPtr->pbduTrap == 1) {
        cpuCode = SALSA_CPU_CODE_BPDU;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->daLookUpCmd == 3 && descrPtr->saLookUpCmd != 3) {
        cpuCode = SALSA_CPU_CODE_MAC_ADDR_TRAP;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->daLookUpCmd != 3 && descrPtr->saLookUpCmd == 3) {
        cpuCode = SALSA_CPU_CODE_MAC_ADDR_TRAP;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    if (descrPtr->daLookUpCmd == 3 && descrPtr->saLookUpCmd == 3) {
        cpuCode = SALSA_CPU_CODE_MAC_ADDR_TRAP;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->macRangeCmd == 3) {
        cpuCode = SALSA_CPU_CODE_MAC_RANGE_TRAP;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->daLookUpCmd == 1 &&
        descrPtr->saLookUpCmd == 1 &&
        descrPtr->lockPortCmd == 1 &&
        descrPtr->macRangeCmd == 1 &&
        descrPtr->invalidSaDrop == 1)
    {
        securityDrop = 1;
    }

    if (securityDrop == 1) {
        dropCode = 2;
        snetSalsaSecurityDropPacket(descrPtr, dropCode);
        return;
    }
    else
    if (descrPtr->ingressFilterOut == 1) {
        dropCode = 3;
        snetSalsaIngressDropPacket(descrPtr, dropCode);
        return;
    }
    else if (1 == descrPtr->stpState)
    {   /* STP state is: Blocking and listening, drop the packet */
        dropCode = 4;
        snetSalsaRegularDropPacket(descrPtr, dropCode);
        return;
    }
    else if (2 == descrPtr->stpState)
    {   /* STP state is: Learning, learn the SA and drop the packet */
        dropCode = 4;
        snetSalsaRegularDropPacket(descrPtr, dropCode);
        snetSalsaLearnSa(devObjPtr, descrPtr);
        return;
    }
    else
    if (descrPtr->arpTrap == 1) {
        if (descrPtr->macDaType == SKERNEL_BROADCAST_MAC_E ||
            descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E)
            cpuCode = SALSA_CPU_CODE_ARP_BROADCAST;
        else
            cpuCode = SALSA_CPU_CODE_CONTROL_TO_CPU;

        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->igmpTrap == 1) {
        cpuCode = SALSA_CPU_CODE_IGMP_PACKET;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->saLookUpCmd == 2 && descrPtr->daLookUpCmd != 2) {
        cpuCode = SALSA_CPU_CODE_INTERVENTION_MAC_ADDR;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->saLookUpCmd != 2 && descrPtr->daLookUpCmd == 2) {
        cpuCode = SALSA_CPU_CODE_INTERVENTION_MAC_ADDR;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->saLookUpCmd == 2 && descrPtr->daLookUpCmd == 2) {
        cpuCode = SALSA_CPU_CODE_INTERVENTION_MAC_ADDR;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }
    else
    if (descrPtr->lockPortCmd == 2) {
        cpuCode = SALSA_CPU_CODE_INTERVENTION_LOCK_TO_CPU;
        snetSalsaTrapPacket(descrPtr, cpuCode);
        return;
    }

    /* Regular forward frames */
    if (descrPtr->macDaLookupResult == 1 &&
        descrPtr->useVidx == 0) {
        snetSalsaFwdUcastDest(descrPtr);
        snetSalsaLearnSa(devObjPtr, descrPtr);
    }
    else {
        if (descrPtr->privEdgeVlanEn == GT_TRUE)
        {
            snetSalsaFwdUcastDest(descrPtr);
        }
        else
        {
            snetSalsaFwdMcastDest(descrPtr);
        }
        snetSalsaLearnSa(devObjPtr, descrPtr);
    }
}

/*******************************************************************************
*   snetSalsaFwdUcastDest
*
* DESCRIPTION:
*        Forward unicast packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaFwdUcastDest
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 trgPort;
    GT_U16 numOfOnes;
    GT_U8 * dataPtr;
    GT_U8 * macAddrPtr;
    SGT_MAC_ADDR_TYP macDst;
    SGT_MAC_ADDR_TYP macSrc;

    ASSERT_PTR(descrPtr);

    macAddrPtr = macDst.bytes;
    dataPtr = DST_MAC_FROM_DSCR(descrPtr);
    MEM_APPEND(macAddrPtr, dataPtr, SGT_MAC_ADDR_BYTES);

    macAddrPtr = macSrc.bytes;
    dataPtr = SRC_MAC_FROM_DSCR(descrPtr);
    MEM_APPEND(macAddrPtr, dataPtr, SGT_MAC_ADDR_BYTES);

    descrPtr->pktCmd = 0;
    descrPtr->useVidx = 0;
    descrPtr->mdb = 0;

    if (descrPtr->bits15_2.useVidx_0.targetIsTrunk) {
        trgPort =  descrPtr->bits15_2.useVidx_0.targedPort;
        numOfOnes = snetSalsaTrunkHash(&macDst, &macSrc, trgPort);
        descrPtr->bits15_2.useVidx_0.trunkHash = (GT_U8)numOfOnes;
    }
}

/*******************************************************************************
*   snetSalsaTrunkHash
*
* DESCRIPTION:
*        Hash for trunk ports
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_U16 snetSalsaTrunkHash
(
    IN SGT_MAC_ADDR_TYP * macDstPtr,
    IN SGT_MAC_ADDR_TYP * macSrcPtr,
    IN GT_U32 trunkId
)
{
    GT_U32 portsInTrunk;
    GT_U32 address;
    GT_U8 firstBit;
    GT_U16 numOfOnes;
    GT_U16 i;

    numOfOnes = 0;
    portsInTrunk = TRUNK_TABLE_REG(trunkId, &address);
    macDstPtr->bytes[5] ^= macSrcPtr->bytes[5];
    firstBit = (GT_U8)(47 - portsInTrunk) + 1;
    for(i = firstBit%8; i < 8; i++) {
        if (macDstPtr->bytes[5] & (1 << i)) {
            numOfOnes++;
        }
    }
    if (portsInTrunk == numOfOnes) {
        numOfOnes = 0;
        for(i = 5; i < 8; i++) {
            if (macSrcPtr->bytes[5] & (1 << i)) {
                numOfOnes++;
            }
        }
        for(i = 0; i < 8; i++) {
            if (macSrcPtr->bytes[5] & (1 << i)) {
                numOfOnes++;
            }
        }
    }
    return numOfOnes;
}
/*******************************************************************************
*   snetSalsaFwdMcastDest
*
* DESCRIPTION:
*        Forward multicast packet
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaFwdMcastDest
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    descrPtr->pktCmd = 0;
    descrPtr->useVidx = 1;
    descrPtr->mdb = 1;
}
/*******************************************************************************
*   snetSalsaDropPacket
*
* DESCRIPTION:
*        Drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
)
{

    descrPtr->pktCmd = 1;
    descrPtr->useVidx = 0;
    descrPtr->bits15_2.useVidx_1.vidx = (GT_U16)dropCode;
}
/*******************************************************************************
*   snetSalsaTrapPacket
*
* DESCRIPTION:
*        Drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        cpuCode  -  drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaTrapPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 cpuCode
)
{
    descrPtr->pktCmd = 2;
    descrPtr->useVidx = 0;
    descrPtr->bits15_2.cmd_trap2cpu.cpuCode = (GT_U16)cpuCode;
}
/*******************************************************************************
*   snetSalsaSecurityDropPacket
*
* DESCRIPTION:
*        Security drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaSecurityDropPacket
(
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 dropCode
)
{
    snetSalsaDropPacket(descrPtr, dropCode);
}

/*******************************************************************************
*   snetSalsaIngressDropPacket
*
* DESCRIPTION:
*        Security drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaIngressDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
)
{
    snetSalsaDropPacket(descrPtr, dropCode);
}
/*******************************************************************************
*   snetSalsaRegularDropPacket
*
* DESCRIPTION:
*        Regular drop packet
* INPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*        dropCode  - drop code
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void  snetSalsaRegularDropPacket
(
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 dropCode
)
{
    snetSalsaDropPacket(descrPtr, dropCode);
}

/*******************************************************************************
*   snetSalsaEgress
*
* DESCRIPTION:
*        Egress frame processing dispatcher.
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    if (descrPtr->pktCmd == 0) {
        if (descrPtr->useVidx == 0) {
            snetSalsaFrwrdUcast(devObjPtr, descrPtr);
        }
        else {
            snetSalsaFrwrdMcast(devObjPtr, descrPtr);
        }
    }
    else
    if (descrPtr->pktCmd == 1) {
        snetSalsaDrop(devObjPtr, descrPtr);
    }
    else {
        snetSalsaTx2Cpu(devObjPtr, descrPtr);
    }
}
/*******************************************************************************
*   snetSalsaFrwrdUcast
*
* DESCRIPTION:
*        Forward unicast known frames
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaFrwrdUcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U8 hashVal;
    GT_U8 targetPort;
    GT_U8 targetTrunk;
    GT_U32 bitToStartInTrunkTableReg;
    GT_U32 bitToStartInTrunkMembersReg;
    GT_U32 memberIdInTrunk;
    GT_U32 fldValue;
    GT_U8 tagged;
    GT_U8 vlan_member_ports[32];
    GT_U8 vlan_tagged_ports[32];
    GT_U8 stp_for_ports[32];

    if (descrPtr->bits15_2.useVidx_0.targetIsTrunk == 1) {
        hashVal = descrPtr->bits15_2.useVidx_0.trunkHash;

        targetTrunk = descrPtr->bits15_2.useVidx_0.targedPort;

        bitToStartInTrunkTableReg = TRUNK_TABLE_REG(targetTrunk, &regAddress);

         /* getting the number of the members in the Trunk(n)
            n is according to Trunk ID */
        smemRegFldGet(devObjPtr, regAddress, bitToStartInTrunkTableReg,
                      4, &fldValue);

        /* calculating the memberID in Trunk(n) */
        memberIdInTrunk = (GT_U8)(hashVal % fldValue);

       /* Trunk Members Table Register(n) 0 <= n < 32,
        for each Trunk there 4 registers,
        each register contains two members data */
        regAddress = 0x06000100 + (0x10 * (targetTrunk - 1))
                     + (memberIdInTrunk / 2) * 0x04;

        /* calculating the bit to start from in Trunk Table Register(n) */
        if ((memberIdInTrunk % 2) == 1)
        {
            bitToStartInTrunkMembersReg = 10;
        }
        else
        {
            bitToStartInTrunkMembersReg = 0;
        }

        /* getting Trunk member for Trunk(n) n=TrunkID
           this member is going to be the destination port */
        smemRegFldGet(devObjPtr, regAddress, bitToStartInTrunkMembersReg,
                      5, &fldValue);

        targetPort = (GT_U8)fldValue;

    }
    else {
        targetPort = descrPtr->bits15_2.useVidx_0.targedPort;
    }

    /* Get VLAN ports info */
    snetSalsaVlanPortsGet(devObjPtr, descrPtr->vid, vlan_member_ports,
                          vlan_tagged_ports, stp_for_ports);

    smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 5, 1, &fldValue);
    /* BridgedUC EgressFilter En*/
    if (fldValue == 1) {
        if (vlan_member_ports[targetPort] == 0) {
            snetSalsaDrop(devObjPtr, descrPtr);
            return;
        }
    }
    if (stp_for_ports[targetPort] != 3 && stp_for_ports[targetPort] != 0) {
        snetSalsaDrop(devObjPtr, descrPtr);
        return;
    }
    tagged = vlan_tagged_ports[targetPort];
    /* Forward frame to target port */
    snetSalsaTx2Port(devObjPtr, descrPtr, targetPort, tagged);
}

/*******************************************************************************
*   snetSalsaFrwrdMcast
*
* DESCRIPTION:
*        Forward broadcast, unknown unicast and multicast frames
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
static void snetSalsaFrwrdMcast
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 firstBit;
    GT_U32 fldValue;
    GT_U32 mcastTrunkBmp;
    GT_U8 tagged;
    GT_U8 i;
    GT_U8 unicastCmd;
    GT_U8 cpuBcFilterEn;
    GT_U32 nonTrunkMem;
    GT_U8 hashVal;
    GT_U8 trunkGroupId;
    GT_U8 vlan_member_ports[32];
    GT_U8 vlan_tagged_ports[32];
    GT_U8 stp_for_ports[32];
    GT_U8 vidx_ports[32];

    /* Get VLAN ports info */
    snetSalsaVlanPortsGet(devObjPtr, descrPtr->vid, vlan_member_ports,
                          vlan_tagged_ports, stp_for_ports );

    unicastCmd = 0;

    /* UCAST */
    if (descrPtr->macDaType == SKERNEL_UNICAST_MAC_E) {
        for (i = 0; i < 24; i++) {
            firstBit = PORT_FILTER_UNK(i);
            smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, firstBit, 1, &fldValue);
            /* PortNFilterUnk */
            if (fldValue)
                vlan_member_ports[i] = 0;
        }
        smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 1, 2, &fldValue);
        /* Unknown UnicastCmd */
        unicastCmd = (GT_U8)fldValue;
    }
    else
    if (descrPtr->macDaLookupResult == 0) {
        if (descrPtr->macDaType == SKERNEL_MULTICAST_MAC_E) {
            smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 3, 2, &fldValue);
            /* Unreg UnicastCmd */
            unicastCmd = (GT_U8)fldValue;
        }
        else {
            smemRegFldGet(devObjPtr, EGRESS_BRIDGING_REG, 0, 1, &fldValue);
            /* CPUBC FilterEn */
            cpuBcFilterEn = (GT_U8)fldValue;
            if (cpuBcFilterEn &&
                descrPtr->macDaType == SKERNEL_BROADCAST_ARP_E) {
                vlan_member_ports[31] = 0;
            }
        }
    }
    else {
        memset(vidx_ports, 0, sizeof(vidx_ports));
        snetSalsaVidxPortsInfoGet(devObjPtr, descrPtr, vidx_ports);
        for (i = 0; i < 32; i++)
            vlan_member_ports[i] &= vidx_ports[i];
    }

    if (unicastCmd == 1) {
        vlan_member_ports[31] = 1;
    }
    else
    if (unicastCmd == 2) {
        vlan_member_ports[31] = 1;
    }
    else
    if (unicastCmd == 3) {
        vlan_member_ports[31] = 0;
    }

    for (i = 0; i < 32; i++) {
        if (stp_for_ports[i] != SKERNEL_STP_DISABLED_E &&
            stp_for_ports[i] != SKERNEL_STP_FORWARD_E) {
            vlan_member_ports[i] = 0;
        }
    }
    if (descrPtr->srcTrunkId > 0) {
        TRUNK_NON_TRUNK_MEMBER_REG(descrPtr->srcTrunkId, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 24, &fldValue);
        /* Trunk<n> NonTrunkMem */
        nonTrunkMem = fldValue;
        for (i = 0; i < 24; i++) {
            if (((nonTrunkMem >> i) & 0x01) == 0) {
                vlan_member_ports[i] = 0;
            }
        }
    }
    hashVal = descrPtr->bits15_2.useVidx_0.trunkHash;
    TRUNK_DESIGN_PORTS_HASH_REG(hashVal, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 24, &fldValue);
    /* DesPortTrunk */
    mcastTrunkBmp = fldValue;
    for (i = 0; i < 24; i++) {
        if (vlan_member_ports[i] == 0)
            continue;

        if ((stp_for_ports[i] == SKERNEL_STP_FORWARD_E) ||
            (stp_for_ports[i] == SKERNEL_STP_DISABLED_E)) {
            /* Bridge Port<n> Control Register */
            BRDG_PORT0_CTRL_REG(i, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, 0, 5, &fldValue);
            /* TrunkGroupId */
            trunkGroupId = (GT_U8)fldValue;
            if ((trunkGroupId > 0) && ((mcastTrunkBmp >> i) & 0x01) == 0) {
                vlan_member_ports[i] = 0;
            }
        }
    }
    for (i = 0; i < 32; i++) {
        if (vlan_member_ports[i] == 1) {
            tagged = vlan_tagged_ports[i];
            snetSalsaTx2Port(devObjPtr, descrPtr, i, tagged);
        }
    }
}
/*******************************************************************************
*   snetSalsaLearnSa
*
* DESCRIPTION:
*        Make RX mirroring
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetSalsaLearnSa
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 fldValue;
    GT_U32 * freeFifoEntryPtr = NULL;
    GT_U32 freeMacEntryAddr = 0xBAD0ADD0;
    GT_U32 fifoSize;
    GT_U32 * fifoBufferPrt = NULL;
    GT_U32 msgEnable;
    GT_U32 setIntr = 0;
    GT_U32 i;
    GT_U8 * srcMacPtr;

    SNET_SALSA_MAC_TBL_STC newMacEntry;
    SALSA_DEV_MEM_INFO   * memInfoPtr;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    if (descrPtr->macSaLookupResult == 1)
        return;

    smemRegFldGet(devObjPtr, FIFO_TO_CPU_CONTROL_REG, 1, 4, &fldValue);

    /* CPUFifo-treshold */
    fifoSize = fldValue;
    smemRegFldGet(devObjPtr, MAC_TABLE_CTRL_REG, 9, 1, &fldValue);
    /* ForwMACUpdToCPU */
    msgEnable = fldValue;

    memInfoPtr =  (SALSA_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
    fifoBufferPrt = memInfoPtr->macFbdMem.macUpdFifoRegs;
    for (i = 0; i < fifoSize; i++) {
        if (*(fifoBufferPrt + 4*i) != 0xffffffff)
            continue;
        freeFifoEntryPtr =  (fifoBufferPrt + 4*i);
        break;
    }

    if (freeFifoEntryPtr == NULL && msgEnable == 1) {
        smemRegFldGet(devObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 16, 1, &fldValue);
        /* MacTable CauseReg Mask */
        if (fldValue) {
            smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 16, 1, 1);
            smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
            setIntr = 1;
        }
    }

    /* Create new MAC entry */
    memset(&newMacEntry, 0, sizeof(SNET_SALSA_MAC_TBL_STC));
    srcMacPtr = SRC_MAC_FROM_DSCR(descrPtr);
    memcpy(newMacEntry.macAddr.bytes, srcMacPtr, SGT_MAC_ADDR_BYTES);
    newMacEntry.vid = descrPtr->vid;
    if (descrPtr->srcTrunkId) {
        newMacEntry.trunk = 1;
        newMacEntry.port = descrPtr->srcTrunkId;
    }
    else
        newMacEntry.port = (GT_U16)descrPtr->srcPort;

    newMacEntry.aging = 1;
    newMacEntry.validEntry = 1;

    if (descrPtr->learnEn == 1) {
        freeMacEntryAddr =
        snetSalsaGetFreeMacEntryAddr(devObjPtr, srcMacPtr, descrPtr->vid);
        if (freeMacEntryAddr == NOT_VALID_ADDR ||
            freeFifoEntryPtr == NULL) {
            smemRegFldGet(devObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 3, 1, &fldValue);
            /* NaNotLearned Mask */
            if (fldValue) {
                smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 3, 1, 1);
                smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
                setIntr = 1;
            }
        }
        else
        if (freeMacEntryAddr != NOT_VALID_ADDR
            && freeFifoEntryPtr != NULL) {

            /* FILL *freeMacEntryAddr */
            snetSetMacEntry(devObjPtr, freeMacEntryAddr, &newMacEntry);

            smemRegFldGet(devObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 2, 1, &fldValue);
            /* NaLearned Mask*/
            if (fldValue) {
                smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 2, 1, 1);
                smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
                setIntr = 1;
            }
        }
    }

    if (freeFifoEntryPtr != NULL) {
        /*    FILL *freeFifoEntryPtr. */
        snetSetFdbFifoEntry(devObjPtr, descrPtr,freeFifoEntryPtr, &newMacEntry);

        smemRegFldGet(devObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 10, 1, &fldValue);
        /* Message2CPUReady Mask*/
        if (fldValue) {
            smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 10, 1, 1);
            smemRegFldSet(devObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
            setIntr = 1;
        }
    }

    /* Call interrupt */
    if (setIntr == 1)
    {
        /* Global Interrupt Mask Register */
        smemRegFldGet(devObjPtr, GLOBAL_INT_MASK_REG, 6, 1, &fldValue);
        /* MTIntSumRO mask*/
        if (fldValue) {
            smemRegFldSet(devObjPtr, GLOBAL_INT_CAUSE_REG, 6, 1, 1);
            smemRegFldSet(devObjPtr, GLOBAL_INT_CAUSE_REG, 0, 1, 1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
}

/*******************************************************************************
*   snetSalsaVidxPortsInfoGet
*
* DESCRIPTION:
*        Get vidx ports info ()
* INPUTS:
*       devObjPtr          - pointer to device object.
*       descrPtr           - pointer to the frame's descriptor.
* OUTPUTS:
*       vidx_ports         - array of the vidx
*
*******************************************************************************/
static GT_BOOL snetSalsaVidxPortsInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT GT_U8 vidx_ports[]
)
{
    GT_U32 vidx;
    GT_BOOL ret = GT_FALSE;

    vidx = descrPtr->bits15_2.useVidx_1.vidx;
    ret = snetSalsaVidxPortsGet(devObjPtr, vidx, vidx_ports);

    return ret;
}

/*******************************************************************************
*   snetSalsaVidxPortsGet
*
* DESCRIPTION:
*        Get vidx ports info ()
* INPUTS:
*       vidx          - pointer to device object.
*
* OUTPUTS:
*       vidx_ports    - array of the vidx
*
*******************************************************************************/
GT_BOOL snetSalsaVidxPortsGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vidx,
    OUT GT_U8 vidx_ports[]
)
{
    GT_U32 regAddress;
    GT_U32 fldFirstBit;
    GT_U32 fldValue;
    GT_U8 i;

    if (vidx < 256) {
        /* Internal VLAN Table */
        INTERNAL_VLAN_TABLE(vidx, &regAddress);

        /* valid bit */
        smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
        if (fldValue == 0)
            return GT_FALSE;

        /* cpu_member */
        smemRegFldGet(devObjPtr, regAddress, 4, 1, &fldValue);
        vidx_ports[31] = (GT_U8)fldValue;

        for (i = 0; i < 24; i++) {
            fldFirstBit = INTERNAL_VTABLE_PORT_OFFSET(vidx, i, &regAddress);
            smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
            /* port<i>member */
            vidx_ports[i] = (GT_U8)fldValue;
        }
    }
    else
    {
        vidx %= 256;
        MCAST_GROUPS_TABLE(vidx, &regAddress);
        for (i = 0; i < 24; i++) {
            smemRegFldGet(devObjPtr, regAddress, i + 1, 1, &fldValue);
            /* port<i>_member */
            vidx_ports[i] = (GT_U8)fldValue;
        }
        smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
        /* cpu_member */
        vidx_ports[31] = (GT_U8)fldValue;
    }

    return GT_TRUE;
}
/*******************************************************************************
*   snetSalsaVlanPortsGet
*
* DESCRIPTION:
*        Get VLAN ports info ()
* INPUTS:
*       devObjPtr          - pointer to device object.
*       vid                - VLAN id
* OUTPUTS:
*       vlan_ports         - array to the VLAN port members
*       vlan_tagged_ports  - array to the VLAN tagged ports
*       stp_for_ports      - array to the STP ports.
*
*******************************************************************************/
GT_BOOL  snetSalsaVlanPortsGet
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN GT_U32 vid,
    OUT GT_U8 vlan_member_ports[],
    OUT GT_U8 vlan_tagged_ports[],
    OUT GT_U8 stp_for_ports[]
)
{
    GT_U32 regAddress;
    GT_U32 fldFirstBit;
    GT_U32 fldValue;
    GT_U32 intVlanIdx;
    GT_U32 vlanValid;
    GT_U32 stgId;
    GT_U32 vidEntry;
    GT_U8 i;

    memset(vlan_member_ports, 0, 32 * sizeof(GT_U8));
    memset(vlan_tagged_ports, 0, 32 * sizeof(GT_U8));
    memset(stp_for_ports, 0, 32 * sizeof(GT_U8));

    vidEntry = vid / 2;
    /* VLAN Translation Table (VLT) */
    VLAN_TRANS_TABLE_REG(vidEntry, &regAddress);
    fldFirstBit = VID_VALID(vid);
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
    /* Valid bit for each VLAN Entry (for Ingress filtering) */
    vlanValid = fldValue;
    if (vlanValid == 0)
        return GT_FALSE;

    fldFirstBit = VID_INTERNAL_VLAN_ENTRY_PTR(vid);
    smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 8, &fldValue);
    /* Pointer to the Internal VLAN Entry */
    intVlanIdx = fldValue;

    /* Internal VLAN Table */
    INTERNAL_VLAN_TABLE(intVlanIdx, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 1, &fldValue);
    /* VLAN table valid bit */
    vlanValid = fldValue;

    if (vlanValid == 0)
        return GT_FALSE;

    smemRegFldGet(devObjPtr, regAddress, 15, 6, &fldValue);
    /* Pointer to the Span State table */
    stgId = fldValue;

    for (i = 0; i < 24; i++) {
        fldFirstBit = INTERNAL_VTABLE_PORT_OFFSET(intVlanIdx, i, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 1, &fldValue);
        /* port<i>member */
        vlan_member_ports[i] = (GT_U8)fldValue;

        smemRegFldGet(devObjPtr, regAddress, fldFirstBit + 1, 1, &fldValue);
        /* port<i>tag */
        vlan_tagged_ports[i] = (GT_U8)fldValue;

        fldFirstBit = SPAN_STATE_TABLE_PORT(stgId, i, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, fldFirstBit, 2, &fldValue);
        stp_for_ports[i] = (GT_U8)fldValue;
    }

    INTERNAL_VLAN_TABLE(intVlanIdx, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 4, 1, &fldValue);
    /* cpu_member */
    vlan_member_ports[31] = (GT_U8)fldValue;

    return GT_TRUE;
}

/*******************************************************************************
*   snetSalsa2Sflow
*
* DESCRIPTION:
*         SFlow processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -  pointer to the frame's descriptor.
*        flowDirect  - packet flow direction RX/TX
* OUTPUTS:
*
* RETURN:
*        TRUE - frame processed by sflow engine, FALSE - not processed
*
*
*******************************************************************************/
static GT_BOOL snetSalsaSflowControl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN SNET_SFLOW_DIRECTION_ENT flowDirect,
    IN GT_U32 sflowPort,
    IN GT_U8 * dataPtr,
    IN GT_U32 dataLen
)
{
    GT_U32 sFlowTbsPort;
    GT_U32 sniffTargetPort, sniffTargetDevice;
    GT_U8 sFlowEn, sFlowSniffEn;
    GT_U32 fldValue;

    if (SKERNEL_DEVICE_FAMILY_SALSA_A2(devObjPtr->deviceType) == 0)
    {
        return GT_FALSE;
    }

    if (flowDirect == SNET_SFLOW_RX)
    {
        smemRegFldGet(devObjPtr, RX_TIME_BASED_SAMPLE_REG, 1, 1, &fldValue);
        /* TBSEn */
        sFlowEn = (GT_U8)fldValue;
        if (sFlowEn == 0)
        {
            return GT_FALSE;
        }
        smemRegFldGet(devObjPtr, RX_TIME_BASED_SAMPLE_REG, 0, 1, &fldValue);
        /* TBSTrigger */
        sFlowSniffEn = (GT_U8)fldValue;
        if (sFlowSniffEn == 0)
        {
            return GT_FALSE;
        }
        smemRegFldGet(devObjPtr, RX_TIME_BASED_SAMPLE_REG, 2, 5, &fldValue);
        /* TBSPort Number */
        sFlowTbsPort = fldValue;
        if (sflowPort != sFlowTbsPort)
        {
            return GT_FALSE;
        }

        smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 3, 5, &fldValue);
        /* RxSniffTarget Device */
        sniffTargetDevice = fldValue;
        smemRegFldGet(devObjPtr, INGR_MIRR_MONITOR_REG, 11, 5, &fldValue);
        /* RxSniffTarget Port */
        sniffTargetPort = fldValue;

        descrPtr->rxSniffed = 1;
        /* Send packet */
        snetSalsaTx2Device(devObjPtr, descrPtr, dataPtr, dataLen,
                           sniffTargetPort, sniffTargetDevice);
        /* Zero TBSTrigger */
        smemRegFldSet(devObjPtr, RX_TIME_BASED_SAMPLE_REG, 0, 1, 0);
    }
    else
    {
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 6, 1, &fldValue);
        /* TBSEn */
        sFlowEn = (GT_U8)fldValue;
        if (sFlowEn == 0)
        {
            return GT_FALSE;
        }
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 5, 1, &fldValue);
        /* TBSTrigger */
        sFlowSniffEn = (GT_U8)fldValue;
        if (sFlowSniffEn == 0)
        {
            return GT_FALSE;
        }
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 19, 5, &fldValue);
        /* TBSPort Number */
        sFlowTbsPort = fldValue;
        if (sflowPort != sFlowTbsPort)
        {
            return GT_FALSE;
        }
        /* Get target sniffer Device/Port */
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 0, 5, &fldValue);
        /* TxSniffDest Dev */
        sniffTargetDevice = fldValue;
        smemRegFldGet(devObjPtr, TRANSMIT_SNIF_REG, 7, 5, &fldValue);
        /* TxSniffDest Port */
        sniffTargetPort = fldValue;

        descrPtr->rxSniffed = 1;
        /* Send packet */
        snetSalsaTx2Device(devObjPtr, descrPtr, dataPtr, dataLen,
                           sniffTargetPort, sniffTargetDevice);
        /* Zero TBSTrigger */
        smemRegFldSet(devObjPtr, TRANSMIT_SNIF_REG, 5, 1, 0);
    }

    return GT_TRUE;
}

