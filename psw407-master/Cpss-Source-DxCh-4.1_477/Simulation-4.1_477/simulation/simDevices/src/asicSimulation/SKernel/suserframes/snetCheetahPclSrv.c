/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahPclSrv.c
*
* DESCRIPTION:
*       Cheetah/2 Asic Simulation .
*       Policy Engine Service Routines.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 44 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3TTermination.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypeTcam.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

/* macro check that start bit and end bit supports next:
    1. start bit not bigger the end bit
    2. diff support up to 32 bits
*/

#define CHECK_FIELD_INTEGRITY(startBit,endBit) \
    if(startBit > endBit) skernelFatalError(" CHECK_FIELD_INTEGRITY: start bit[%d] > end bit[%d] \n");\
    if((startBit + 32) < endBit) skernelFatalError(" CHECK_FIELD_INTEGRITY: start bit[%d] + 32 < end bit[%d] \n")

/* TCAM entry valid bit */
#define SNET_PCL_TCAM_VALID_BIT_MAC(dev) \
    ((SKERNEL_IS_XCAT_DEV(dev)) ? 1 : 2)

/***************************************************************************
* snetChtPclSrvTcamBankHit
*
* DESCRIPTION:
*        Lookup for match in specific TCAM bank
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       tcamCommonDataPtr   - pointer to TCAM common data structure.
*       pclKeyPtr           - pointer to PCL key structure
*       currentBank         - the number of bank / segment of key
*       entryIndex          - TCAM entry index
*
*
* OUTPUTS:
*       None.
*
* RETURN:
*       GT_OK           - matching is hit
*       GT_NOT_FOUND    - no matching
*       GT_FAIL         - operation failed
*
* COMMENTS:
*
***************************************************************************/
static GT_STATUS snetChtPclSrvTcamBankHit
(
    IN SKERNEL_DEVICE_OBJECT        *devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC *tcamCommonDataPtr,
    IN SNET_CHT_POLICY_KEY_STC      *pclKeyPtr,
    IN GT_U32                       currentBank,
    IN GT_U32                       entryIndex
);

/*******************************************************************************
*   snetChtPclSrvTcamWordGet
*
* DESCRIPTION:
*       Function gets data and control words from the TCAM by entry/word index
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       tcamCommonDataPtr   - pointer to TCAM common data structure
*       dataType            - TCAM data type (pattern/mask)
*       entryIndex          - TCAM entry index
*       wordIndex           - TCAM word index
*
* OUTPUTS:
*       byteDataPtr             - pointer to TCAM data word pointer
*       byteCtrlPtr             - pointer to TCAM data control word pointer
*
* RETURN:
*       GT_OK   - operation successful
*       GT_FAIL - operation failed
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetChtPclSrvTcamWordGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC * tcamCommonDataPtr,
    IN CHT_PCL_TCAM_DATA_TYPE_ENT dataType,
    IN GT_U32  entryIndex,
    IN GT_U32  wordIndex,
    INOUT GT_U8 * byteDataPtr,
    INOUT GT_U8 * byteCtrlPtr
);

/*******************************************************************************
*   snetChtPclKeyFieldPut
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specified place in key
*       used for "numeric" fields - up to GT_U32
*
* INPUTS:
*       bytesKeyPtr  - (pointer to) current pcl key
*       endBit       - field MSB position
*       startBit     - field LSB position
*       fieldVal     - data of field to insert to key
*
* OUTPUTS:
*       wordsKeyPtr - (pointer to) PCL field
*
* RETURN:
*
* COMMENTS:
*     The simulated key consists of 32 bit words that
*     compared "anded" and "compared" with TCAM read-only
*     pattern and mask direct access registers. There is no difference
*     Big or Little endian CPU executes this code, but for PC
*     the bytes will be seen swapped under debugger
*
*******************************************************************************/
GT_VOID snetChtPclKeyFieldPut
(
    IN SKERNEL_DEVICE_OBJECT   *devObjPtr,
    INOUT GT_U8                   *bytesKeyPtr,
    IN GT_U32                     endBit,
    IN GT_U32                     startBit,
    IN GT_U32                     fieldVal
)
{
    GT_U32  byteIndex;
    GT_U32  bitIndex;/*index of start bit in the start byte*/
    GT_U32  length;/* length of bits to set */
    GT_U32  numBitsInField;/* number of bits in the field */

    CHECK_FIELD_INTEGRITY(startBit,endBit);

    numBitsInField = endBit - startBit + 1;

    byteIndex = startBit / 8;
    bitIndex  = startBit % 8;

    while (numBitsInField)
    {
        length = (numBitsInField < (8 - bitIndex))
                ? numBitsInField : (8 - bitIndex);

        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[0x%2.2x] \n",
                      fieldVal));

        SMEM_U32_SET_FIELD(bytesKeyPtr[byteIndex],/*data*/
                           bitIndex, /*offset*/
                           length, /*length*/
                           fieldVal);/*val*/

        byteIndex++;
        bitIndex = 0;
        numBitsInField -= length;
        fieldVal >>= length;
    }
}

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*       in Little Endian order (PP order)
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC       *pclKeyPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
)
{
    GT_U32  length;/* length of bits to set */
    GT_U32  offset;/* offset where to write in the word */
    GT_U32  numBitsInField;/* number of bits in the field */

    if (pclKeyPtr->updateOnlyDiff == GT_TRUE)
    {
        if (fieldInfoPtr->updateOnSecondCycle == GT_FALSE)
        {
            /* don't update on second cycle */
            return;
        }
    }

    if (fieldValPtr == 0)
    {
        return ;
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%s] startBitInKey[%d] endBitInKey[%d] value -->",
                  fieldInfoPtr->debugName,
                  fieldInfoPtr->startBitInKey,
                  fieldInfoPtr->endBitInKey));

    numBitsInField = fieldInfoPtr->endBitInKey -
                     fieldInfoPtr->startBitInKey +
                     1;
    offset = fieldInfoPtr->startBitInKey;

    while (numBitsInField > 0)
    {
        length = (numBitsInField >= 8) ? 8 : numBitsInField;

        snetChtPclKeyFieldPut(
            pclKeyPtr->devObjPtr,
            SNET_CHT_PCL_KEY_PTR(pclKeyPtr),
            (offset + length - 1),
            offset,
            fieldValPtr[(numBitsInField - 1) / 8]);

        numBitsInField -= length;
        offset += length;
    }


    return;
}

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByU32Pointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*       in Little Endian order (PP order)
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvKeyFieldBuildByU32Pointer
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U32                               *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
)
{
    GT_U32  length;/* length of bits to set */
    GT_U32  offset;/* offset where to write in the word */
    GT_U32  numBitsInField;/* number of bits in the field */
    GT_U32  dataWord;   /* field data's word number */
    GT_U32  fieldVal;   /* field value for specific word */
    GT_U32  dataOffset; /* field data's offset in bits */

    if (pclKeyPtr->updateOnlyDiff == GT_TRUE)
    {
        if (fieldInfoPtr->updateOnSecondCycle == GT_FALSE)
        {
            /* don't update on second cycle */
            return;
        }
    }

    if (fieldValPtr == 0)
    {
        return ;
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%s] startBitInKey[%d] endBitInKey[%d] value -->",
                  fieldInfoPtr->debugName,
                  fieldInfoPtr->startBitInKey,
                  fieldInfoPtr->endBitInKey));

    numBitsInField = fieldInfoPtr->endBitInKey -
                     fieldInfoPtr->startBitInKey +
                     1;
    offset = fieldInfoPtr->startBitInKey;

    /* calculate bit offset for last word in the fieldValPtr */
    dataOffset = 32 - (numBitsInField % 32);

    while (numBitsInField > 0)
    {
        length = (numBitsInField >= 32) ? 32 : numBitsInField;
        dataWord = (numBitsInField - 1) / 32;

        if (dataOffset != 32)
        {
            /* the field has partial last word.
               need to take MSBs of last word shifted to first position.
               And current field's data need to be completed by following
               word shifted to dataOffset */
            fieldVal = (fieldValPtr[dataWord] >> dataOffset);
            if (dataWord)
            {
                SMEM_U32_SET_FIELD( fieldVal, /* output data */
                                    (32 - dataOffset), /* offset*/
                                    dataOffset,        /* length */
                                    fieldValPtr[dataWord - 1] /* input data*/);
            }
        }
        else
        {
            /* the field comprise full words only */
            fieldVal = fieldValPtr[dataWord];
        }

        snetChtPclKeyFieldPut(
            pclKeyPtr->devObjPtr,
            SNET_CHT_PCL_KEY_PTR(pclKeyPtr),
            (offset + length - 1),
            offset,
            fieldVal);

        numBitsInField -= length;
        offset += length;
    }


    return;
}

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByValue
*
* DESCRIPTION:
*       function insert a data of the field to the search key
*       in specific place in key
*       used for "numeric" fields - up to GT_U32
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldVal - data of field to insert to key
*       fieldId -- field id
*       cycleNum -  the number of current cycle
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U32                                fieldVal,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
)
{
    if (pclKeyPtr->updateOnlyDiff == GT_TRUE)
    {
        if (fieldInfoPtr->updateOnSecondCycle == GT_FALSE)
        {
            /* don't update on second cycle */
            return;
        }
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%s] startBitInKey[%d] endBitInKey[%d] value[0x%8.8x]\n",
                  fieldInfoPtr->debugName,
                  fieldInfoPtr->startBitInKey,
                  fieldInfoPtr->endBitInKey,
                  fieldVal & (SMEM_BIT_MASK(fieldInfoPtr->endBitInKey - fieldInfoPtr->startBitInKey+1))
                  ));


    snetChtPclKeyFieldPut(
        pclKeyPtr->devObjPtr,
        SNET_CHT_PCL_KEY_PTR(pclKeyPtr),
        fieldInfoPtr->endBitInKey,
        fieldInfoPtr->startBitInKey,
        fieldVal);

    return;
}

/*******************************************************************************
*   snetChtPclSrvParseExtraData
*
* DESCRIPTION:
*        Parse the packet for extra data needed for PCL key creation
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       pclExtraDataPtr - (pointer to) pcl extra info (cookie)
*
* RETURN:
*
* COMMENTS:
*         isL2Valid is a field added for cheetah2 . It is relevant with
*           TTI packets.
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvParseExtraData
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    OUT CHT_PCL_EXTRA_PACKET_INFO_STC       * pclExtraDataPtr
)
{
    DECLARE_FUNC_NAME(snetChtPclSrvParseExtraData);

    GT_U32 timeToLive;      /* time to live                           */


    pclExtraDataPtr->isIpHeaderOk = GT_FALSE;
    pclExtraDataPtr->isIpV6EhExists   = GT_FALSE;
    pclExtraDataPtr->isIpV6EhHopByHop = GT_FALSE;
    pclExtraDataPtr->isL4Valid = GT_FALSE;
    pclExtraDataPtr->isIpv4Fragment = GT_FALSE;
    pclExtraDataPtr->isL2Valid = descrPtr->l2Valid ? GT_TRUE : GT_FALSE;
    pclExtraDataPtr->ipv4HeaderInfo = 0;
    timeToLive = descrPtr->ttl;

    if (descrPtr->isIp == GT_FALSE)
    {
        goto exit_function_lbl;
    }
    else if (descrPtr->isIPv4)
    {
        /* fragment offset */
        pclExtraDataPtr->ipv4FragmentOffset = descrPtr->ipv4FragmentOffset;

        /* default */
        pclExtraDataPtr->isIpv4Fragment = GT_FALSE;

        /* "fragment offset" (the first fragment has offset zero) */
        if (pclExtraDataPtr->ipv4FragmentOffset != 0)
        {
            pclExtraDataPtr->isIpv4Fragment = GT_TRUE;
        }
        __LOG(("pclExtraDataPtr->ipv4FragmentOffset[%d], pclExtraDataPtr->isIpv4Fragment[%d]",
                       pclExtraDataPtr->ipv4FragmentOffset,     pclExtraDataPtr->isIpv4Fragment));
    }
    else /* ipv6 */
    {
        pclExtraDataPtr->isIpV6EhExists = descrPtr->isIpV6EhExists;
        pclExtraDataPtr->isIpV6EhHopByHop = descrPtr->isIpV6EhHopByHop;

        __LOG(("pclExtraDataPtr->isIpV6EhHopByHop[%d], pclExtraDataPtr->isIpV6EhExists[%d]",
                       pclExtraDataPtr->isIpV6EhHopByHop,     pclExtraDataPtr->isIpV6EhExists));
    }

    if (timeToLive == 0)
    {
        __LOG(("timeToLive == 0 --> don't calc isIpHeaderOk,isL4Valid"));
        goto exit_function_lbl;
    }

    pclExtraDataPtr->isIpHeaderOk = descrPtr->ipHeaderError ? GT_FALSE : GT_TRUE;

    /* for now I ignore the chance that l4 info is beyond the 128 first bytes */
    pclExtraDataPtr->isL4Valid = descrPtr->isIp == 0 ? GT_FALSE :
                              pclExtraDataPtr->isIpv4Fragment == GT_TRUE ? GT_FALSE :
                              pclExtraDataPtr->isIpHeaderOk == GT_FALSE ? GT_FALSE :
                             (pclExtraDataPtr->isIpV6EhExists == GT_TRUE &&
                              pclExtraDataPtr->isIpV6EhHopByHop == GT_FALSE) ? GT_FALSE : GT_TRUE;

    __LOG(("pclExtraDataPtr->isIpHeaderOk[%d], pclExtraDataPtr->isL4Valid[%d]",
                   pclExtraDataPtr->isIpHeaderOk,     pclExtraDataPtr->isL4Valid));

exit_function_lbl:
    /* store L4 Valid in the descriptor for the following processing */
    descrPtr->l4Valid = (pclExtraDataPtr->isL4Valid == GT_FALSE) ? 0 : 1;

    return;
}

/*******************************************************************************
*   snetChtPclUserDefinedByteGet
*
* DESCRIPTION:
*       function get from the packet the user defined byte info
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       userDefinedAnchor - user defined byte Anchor
*       userDefinedOffset - user defined byte offset from Anchor
* OUTPUTS:
*       userDefinedByteValuePtr - (pointer to) the user defined byte value
*
* RETURN:
*       GT_OK              Operation succeeded
*       GT_FAIL            Operation failed
*
* COMMENTS:
*       [1] 8.2.2.2 parser -- page 81
*       [1] 8.5.2.3 User-Defined Bytes -- page 99
*
*******************************************************************************/
extern GT_STATUS snetChtPclUserDefinedByteGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  userDefinedAnchor,
    IN GT_U32  userDefinedOffset,
    OUT GT_U8  *userDefinedByteValuePtr
)
{
    GT_U8   *tmpAnchorBytePacketPtr;/* pointer to anchor byte in the packet */
    GT_U32  indexInPacket = 0;

    switch (userDefinedAnchor)
    {
        case 0:
            tmpAnchorBytePacketPtr = descrPtr->macDaPtr;
            indexInPacket = descrPtr->macDaPtr - descrPtr->startFramePtr;
            break;
        case 1:
            tmpAnchorBytePacketPtr = descrPtr->l3StartOffsetPtr;
            indexInPacket = descrPtr->l2HeaderSize;
            break;
        case 2:
            tmpAnchorBytePacketPtr = descrPtr->l4StartOffsetPtr;
            indexInPacket = descrPtr->l23HeaderSize;
            break;
        case 3:
            tmpAnchorBytePacketPtr = NULL;/* need to be pointer to Start of
                                             IPv6 extension header.*/
            break;
        default:
            return GT_FAIL;
    }

    if (tmpAnchorBytePacketPtr == NULL)
    {
        *userDefinedByteValuePtr = 0;
    }
    else
    {
        indexInPacket += userDefinedOffset;

        /* check limitation from start of the packet */
        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("check limitation from start of the packet \n"));
        if(indexInPacket >= SNET_CHT_PCL_MAX_BYTE_INDEX_CNS)
        {
            if(descrPtr->capwap.doLookup == GT_FALSE)
            {
                return GT_FAIL;
            }
            else if(indexInPacket >= SNET_CHT_PCL_MAX_BYTE_INDEX_EXTENDED_CNS)
            {
                return GT_FAIL;
            }
        }

        if (indexInPacket >= descrPtr->byteCount)
        {
            *userDefinedByteValuePtr = 0;
            return GT_OK;
        }


        *userDefinedByteValuePtr = tmpAnchorBytePacketPtr[userDefinedOffset];
    }

    return GT_OK;
}

/*******************************************************************************
*   snetChtIPclTcpUdpPortRangeCompareGet
*
* DESCRIPTION:
*        Build TCP/UDP comparator value for PCL search key.
*
* INPUTS:
*       regAddr            - register address of the table
*       srcPktIPPort       - source port.
*       dstPktIPPort       - destination port.
*
* OUTPUTS:
*       byteCompareVal    - pointer to the TCP/UDP port comparator value.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS tcpUdpPortRangeCompareGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32      regAddr,
    IN GT_U32      srcPktIPPort,
    IN GT_U32      dstPktIPPort,
    OUT GT_U8      * byteCompareVal
)
{
    DECLARE_FUNC_NAME(tcpUdpPortRangeCompareGet);

    GT_U32 *    layer4CompareValPtr; /* pointer to the tcp or udp compare table*/
    GT_U32      ii =0 ; /* index for the loop */
    GT_U32      pktIPPort; /* source ip packet port */
    GT_U32      compIPPort; /* destination ip packet port */
    GT_U32      l4Operator; /* operator*/
    GT_U8       bytePortCompareVal = 0 ;
    GT_U32      res1; /* comparison result */

    layer4CompareValPtr = smemMemGet(devObjPtr, regAddr);
    /* build the byteCompareVal from the ingress TCP Port Range     *
     *   comparator<n> configuration Register                       */
    for (ii = 0 ; ii < SNET_CHT_TCP_UDP_PORT_COMP_RANGE_CNS;
          ++ii, ++layer4CompareValPtr)
    {
        if (((*layer4CompareValPtr >> 0x12) & 0x1) == 0x0)
        {/* source IP port*/
            __LOG(("source IP port[%4.4x]",srcPktIPPort));
            pktIPPort = srcPktIPPort ;
        }
        else
        {/* destination IP port*/
            __LOG(("destination IP port[%4.4x]",dstPktIPPort));
            pktIPPort = dstPktIPPort;
        }

        compIPPort =  *layer4CompareValPtr & 0xFFFF; /* port to be compared to */
        l4Operator = (*layer4CompareValPtr >> 0x10) & 0x3; /* operator */
        res1 = 0 ;

        switch (l4Operator)
        {
            case CHT_PCL_IP_PORT_OPERATOR_INVALID_E:
                __LOG(("l4Operator = INVALID"));
                break;

            case CHT_PCL_IP_PORT_OPERATOR_GREATER_OR_EQUAL_E:
                __LOG(("l4Operator = GREATER_OR_EQUAL"));
                res1 = (pktIPPort >= compIPPort )? 1 : 0;
                break;

            case CHT_PCL_IP_PORT_OPERATOR_LESS_OR_EQUAL_E:
                __LOG(("l4Operator = LESS_OR_EQUAL"));
                res1 = (pktIPPort <= compIPPort )? 1 : 0;
                break;

            case CHT_PCL_IP_PORT_OPERATOR_NOT_EQUAL_E:
                __LOG(("l4Operator = NOT_EQUAL"));
                res1 = (pktIPPort != compIPPort )? 1 : 0;
                break;
        }
        bytePortCompareVal |= (res1 << ii);
    } /* ii */

    *byteCompareVal = bytePortCompareVal;

    return GT_OK;

}

/*******************************************************************************
*   snetChtIPclTcpUdpPortRangeCompareGet
*
* DESCRIPTION:
*        Build TCP/UDP comparator value for PCL search key.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*
* OUTPUTS:
*       byteCompareVal    - pointer to the TCP/UDP port comparator value.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_STATUS snetChtIPclTcpUdpPortRangeCompareGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    OUT GT_U8                           * byteCompareVal
)
{
    DECLARE_FUNC_NAME(snetChtIPclTcpUdpPortRangeCompareGet);

    GT_U32      regAddr;       /* registry address */

    /* Don't compute value for packets with IPv4/IPv6 header error */
    if (descrPtr->ipHeaderError)
    {
        __LOG(("Don't compute value for packets with IPv4/IPv6 header error"));
        return GT_FAIL;
    }

    if (descrPtr->ipProt == SNET_TCP_PROT_E)
    {
        __LOG(("use TCP"));
        regAddr = SMEM_CHT_PCL_TCP_PORT_RANGE_COMPARE_REG(devObjPtr);
    }
    else if (descrPtr->udpCompatible)
    {
        __LOG(("use UDP"));
        regAddr = SMEM_CHT_PCL_UDP_PORT_RANGE_COMPARE_REG(devObjPtr);
    }
    else
    {
        __LOG(("unknown protocol[0x%4.4x]",descrPtr->ipProt));
        return GT_FAIL ;
    }

    return tcpUdpPortRangeCompareGet(devObjPtr,regAddr,
                descrPtr->l4SrcPort,
                descrPtr->l4DstPort,
                byteCompareVal);
}

/*******************************************************************************
*   snetCht2EPclTcpUdpPortRangeCompareGet
*
* DESCRIPTION:
*        Build TCP/UDP comparator value for EPCL search key - EPCL - from ch2
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*
* OUTPUTS:
*       byteCompareVal    - pointer to the TCP/UDP port comparator value.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_STATUS snetCht2EPclTcpUdpPortRangeCompareGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    OUT GT_U8                           * byteCompareVal
)
{
    DECLARE_FUNC_NAME(snetCht2EPclTcpUdpPortRangeCompareGet);

    GT_U32      regAddr;       /* registry address */

    /* Don't compute value for packets with IPv4/IPv6 header error */
    if (descrPtr->ipHeaderError)
    {
        __LOG(("Don't compute value for packets with IPv4/IPv6 header error"));
        return GT_FAIL;
    }

    if (descrPtr->ipProt == SNET_TCP_PROT_E)
    {
        __LOG(("use TCP"));
        regAddr = SMEM_CHT2_EPCL_TCP_PORT_RANGE_COMPARE_REG(devObjPtr);
    }
    else if (descrPtr->udpCompatible)
    {
        __LOG(("use UDP"));
        regAddr = SMEM_CHT2_EPCL_UDP_PORT_RANGE_COMPARE_REG(devObjPtr);
    }
    else
    {
        __LOG(("unknown protocol[0x%4.4x]",descrPtr->ipProt));
        return GT_FAIL ;
    }

    return tcpUdpPortRangeCompareGet(devObjPtr,regAddr,
                descrPtr->l4SrcPort,
                descrPtr->l4DstPort,
                byteCompareVal);
}

/*******************************************************************************
*   snetChtPclSrvKeyWordGet
*
* DESCRIPTION:
*       Function gets word from the search key according to TCAM word index
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       tcamCommonDataPtr   - pointer to TCAM common data structure
*       pclKeyPtr           - pointer to current pcl key
*       wordIndex           - TCAM word index
*
* OUTPUTS:
*       bytePtr             - pointer to PCL key word pointer
*
* RETURN:
*       GT_OK   - operation successful
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS snetChtPclSrvKeyWordGet
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC *  tcamCommonDataPtr,
    IN SNET_CHT_POLICY_KEY_STC *pclKeyPtr,
    IN GT_U32  wordIndex,
    INOUT GT_U8 *  bytePtr
)
{
    GT_U8 * bytesKeyPtr;            /* PCL key pointer */
    GT_U32  byteOffset;             /* PCL key byte offset */
    GT_U32  bitOffset;              /* PCL key bit offset */
    GT_U32  wordBytes;              /* Number of bytes in TCAM word */
    GT_U32  data[2];                /* tmp data to copy from key - 64 bits (before shift bits)*/
    GT_U32  result[2] = { 0, 0 };   /* result data (after shift bits) */
    GT_U32  val;                    /* temporary field value */
    GT_U32  bankWordIndex;          /* wordIndex in current bank */
    GT_U32  wordReference, keyPart, keyStartBit, bitsLeft;

    ASSERT_PTR(bytePtr);

    bytesKeyPtr = SNET_CHT_PCL_KEY_PTR(pclKeyPtr);


    if (devObjPtr->pclTcamFormatVersion >= 1)   /* xCat2 TCAM format*/
    {
        /* bankWordIndex (0..3) -- wordIndex in calling function (snetChtPclSrvTcamBankHit) */
        bankWordIndex = wordIndex % tcamCommonDataPtr->bankWordsNum;
        wordReference = wordIndex - bankWordIndex;  /* see snetChtPclSrvTcamBankHit */
        /* part of key:  0 - first 26 bytes of key   (std, ext or triple)
                         1 - second 26 bytes of key  (ext or triple)
                         2 - third 26 bytes of key   (triple) */
        keyPart = wordReference / tcamCommonDataPtr->bankWordsNum;


        if (bankWordIndex == 0) /* 1st word in bank - contain 2 Control bits */
        {
            /* 1) tcam internal key bits 0-1: set by key size: (1-std, 2-ext or 3-triple) */
            result[0] = SNET_CHT_SRV_TCAM_CTRL_COMP_MODE(pclKeyPtr->pclKeyFormat);

            /* 2) tcamDataBits 2-51 = read from key (length = 50 bits) */
            keyStartBit = keyPart*206;   /* key bits  0-49 (std), or 206-255 (ext) */

                  /* copy 8 bytes (with wanted 50 bits) from key into data */
            byteOffset = keyStartBit / 8;
            bitOffset = keyStartBit % 8;
            data[0] =
                bytesKeyPtr[byteOffset+0] << 0 |
                bytesKeyPtr[byteOffset+1] << 8 |
                bytesKeyPtr[byteOffset+2] << 16|
                bytesKeyPtr[byteOffset+3] << 24 ;
            data[1] =
                bytesKeyPtr[byteOffset+4] << 0 |
                bytesKeyPtr[byteOffset+5] << 8 |
                bytesKeyPtr[byteOffset+6] << 16|
                bytesKeyPtr[byteOffset+7] << 24 ;

                /* copy bits in 2 steps: 1) bits from data[0];  2) rest bits from data[1] */
            val = SMEM_U32_GET_FIELD(data[0],  bitOffset, 32-bitOffset);
            snetFieldValueSet(result, 2, 32-bitOffset, val);

            /* bitsLeft = 52 - 2 - (32-bitOffset) */
            bitsLeft = tcamCommonDataPtr->bankWordDataBits - 2 - (32-bitOffset); /* rest of bits in data[1] */
            val = SMEM_U32_GET_FIELD(data[1],  0, bitsLeft);
            snetFieldValueSet(result, 2 + 32-bitOffset, bitsLeft, val);
        }
        else
        {   /* bankWordIndex 1,2,3 - only data bits (no control bits) */
            /* result tcamDataBits 0-51 = read from key (starting from keyStartBit, length = 52 bits) */
            keyStartBit = keyPart*206 + bankWordIndex*52 - 2;  /* 1) keyBits 50-101; 2) keyBits = 102-153; 3) 154-205; */

            /* copy 8 bytes (with wanted 52 bits) from key into data */
            byteOffset = keyStartBit / 8;
            bitOffset = keyStartBit % 8;
            data[0] =
                bytesKeyPtr[byteOffset+0] << 0 |
                bytesKeyPtr[byteOffset+1] << 8 |
                bytesKeyPtr[byteOffset+2] << 16|
                bytesKeyPtr[byteOffset+3] << 24 ;
            data[1] =
                bytesKeyPtr[byteOffset+4] << 0 |
                bytesKeyPtr[byteOffset+5] << 8 |
                bytesKeyPtr[byteOffset+6] << 16|
                bytesKeyPtr[byteOffset+7] << 24 ;
            /* copy bits in 2 steps: 1) bits from data[0];  2) rest bits from data[1] */
            val = SMEM_U32_GET_FIELD(data[0],  bitOffset, 32-bitOffset);
            snetFieldValueSet(result, 0, 32-bitOffset, val);

            bitsLeft = tcamCommonDataPtr->bankWordDataBits - (32-bitOffset); /* rest of bits in data[1] */
            val = SMEM_U32_GET_FIELD(data[1],  0, bitsLeft);
            snetFieldValueSet(result, 32-bitOffset, bitsLeft, val);
        }

        /* Copy data to output buffer */
        memcpy(bytePtr, result, 7);
    }
    else
    {
        /* Ch3, xCat, Lion TCAM format */
    /* Calculate PCL key word offset in bytes */
    wordBytes = tcamCommonDataPtr->bankWordDataBits / 8;
    byteOffset = wordIndex * wordBytes;

    /* Copy data to output buffer */
    memcpy(bytePtr, bytesKeyPtr + byteOffset, wordBytes);
    }

    return GT_OK;
}

/*******************************************************************************
*   snetChtPclSrvTcamWordGet
*
* DESCRIPTION:
*       Function gets data and control words from the TCAM by word index
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       tcamCommonDataPtr   - pointer to TCAM common data structure
*       dataType            - TCAM data type (pattern/mask)
*       entryIndex          - TCAM entry index
*       wordIndex           - TCAM word index
*
* OUTPUTS:
*       byteDataPtr             - pointer to TCAM data word pointer
*       byteCtrlPtr             - pointer to TCAM data control word pointer
*
* RETURN:
*       GT_OK   - operation successful
*       GT_FAIL - operation failed
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetChtPclSrvTcamWordGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC * tcamCommonDataPtr,
    IN CHT_PCL_TCAM_DATA_TYPE_ENT dataType,
    IN GT_U32  entryIndex,
    IN GT_U32  wordIndex,
    INOUT GT_U8 * byteDataPtr,
    INOUT GT_U8 * byteCtrlPtr
)
{
    GT_U32 regPatternData = 0;
    GT_U32 regCtrlData = 0;

    ASSERT_PTR(byteDataPtr);
    ASSERT_PTR(byteCtrlPtr);

    /* Get pattern and control data from TCAM memory */
    SNET_CHT_SRV_TCAM_MEM_GET(tcamCommonDataPtr, dataType,
                              entryIndex, wordIndex,
                              regPatternData, regCtrlData);


    /* Copy pattern data to output buffers */
    if (tcamCommonDataPtr->bankWordDataBits > 32)
    {
        /* 52 data bits in total, copy bits 48-51: */
        byteDataPtr[6] = (GT_U8)SMEM_U32_GET_FIELD(regCtrlData, 16, 4);
        /* Upper 16 bits of the 48-bit word */
        byteDataPtr[5] = (GT_U8)SMEM_U32_GET_FIELD(regCtrlData, 8, 8);
        byteDataPtr[4] = (GT_U8)SMEM_U32_GET_FIELD(regCtrlData, 0, 8);
        /* Lower 32 bits of the 48-bit word */
        byteDataPtr[3] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData, 24, 8);
        byteDataPtr[2] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData, 16, 8);
        byteDataPtr[1] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData,  8, 8);
        byteDataPtr[0] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData,  0, 8);
    }
    else
    {
        byteDataPtr[3] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData, 24, 8);
        byteDataPtr[2] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData, 16, 8);
        byteDataPtr[1] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData,  8, 8);
        byteDataPtr[0] = (GT_U8)SMEM_U32_GET_FIELD(regPatternData,  0, 8);
    }

    if (devObjPtr->pclTcamFormatVersion == 0)
    {
    /* Copy control data to output buffers */
    byteCtrlPtr[0] = (GT_U8)SMEM_U32_GET_FIELD(regCtrlData, 16,
                                               tcamCommonDataPtr->bankWordCtrlBits);
    }

    return GT_OK;
}

/***************************************************************************
* snetChtPclSrvTcamLookUpPrvNextSeg
*
* DESCRIPTION:
*        TCAM Lookup Iteration Next tested short segment of Rule (current or next)
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       bankEntriesNum      - amount of entries in one TCAM bank
*       pclKeyFormat        - PCL key format
*       currMatchStatus     - status of lookup on current index(relevant to other the short keys)
*       tcamIndexPtr        - index of TCAM rule to check
*
* OUTPUTS:
*       tcamIndexPtr        - index of NEXT TCAM rule to check
*
* RETURN:
*       GT_OK   - operation successful
*
* COMMENTS:
*
*   logic:
*       for sort key the indexes are 0..4x-1 in steps of 1
*       for extended key the indexes of start rule will be 0..x-1 , 2x..3x-1
*               the second segment indexes are in x..2x-1 , 3x..4x-1
*       for triple key the indexes are 0..x-1 in steps of 1
*               the second segment indexes are in x..2x-1
*               the third segment indexes are in 2x..3x-1
*
*
***************************************************************************/
static GT_STATUS snetChtPclSrvTcamLookUpPrvNextSeg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC *tcamCommonDataPtr,
    IN CHT_PCL_KEY_FORMAT_ENT         pclKeyFormat,
    IN GT_STATUS                     currMatchStatus,
    INOUT GT_U32                      *tcamIndexPtr
)
{
    GT_U32    currentBank = ((*tcamIndexPtr) / tcamCommonDataPtr->bankEntriesNum);/* the current bank we query */
    GT_U32    pclMode = SNET_CHT_SRV_TCAM_CTRL_COMP_MODE(pclKeyFormat);
    GT_U32    segmentInRule;/* segment index in rule */

    if((*tcamIndexPtr) == END_OF_TABLE)
    {
        skernelFatalError("netChtPclSrvTcamLookUpPrvNextSeg: table overflow\n");
        return GT_OK;
    }

    segmentInRule = currentBank % pclMode;/* for extended and for triple key*/

    switch (pclKeyFormat)
    {
        case CHT_PCL_KEY_REGULAR_E:
            (*tcamIndexPtr) ++;
            return GT_OK;
        case CHT_PCL_KEY_EXTENDED_E:
            break;
        case CHT_PCL_KEY_TRIPLE_E:
            if(currentBank == pclMode)
            {
                /* the triple can't use it's next segment */
                /* so jump to end of table */
                (*tcamIndexPtr) = END_OF_TABLE;
                return GT_OK;
            }
            break;
        default:
            /* not supported key */
            (*tcamIndexPtr) = END_OF_TABLE;
            return GT_OK;
    }

    /* in more then single segment we may find "no match" in the starting segments,
       so no need to continue query this rule...*/
    if(currMatchStatus != GT_OK)
    {
        /* we need to jump to next rule */

        /* go back to start index of current rule */
        (*tcamIndexPtr) -= tcamCommonDataPtr->bankEntriesNum * segmentInRule;

        /* jump to next rule */
        (*tcamIndexPtr) ++;

        if(0 == ((*tcamIndexPtr) % tcamCommonDataPtr->bankEntriesNum))
        {
            /* jump to start of the next valid bank for this type of lookup */
            (*tcamIndexPtr) = (currentBank - segmentInRule + pclMode) *
                              tcamCommonDataPtr->bankEntriesNum;
        }

        return GT_OK;
    }

    segmentInRule++;

    if(segmentInRule == pclMode)
    {
        /* should not happen , because the value of previousMatchStatus != GT_OK
        and segmentInRule > 0 (before ++)

        meaning that there was match on last segment and still we look for next ?
        */
        skernelFatalError("netChtPclSrvTcamLookUpPrvNextSeg: no more looking should be done\n");
        return GT_OK;
    }

    (*tcamIndexPtr) += tcamCommonDataPtr->bankEntriesNum;

    return GT_OK;
}

/***************************************************************************
* snetXCatPclSrvTcamLookUpPrvNextSeg
*
* DESCRIPTION:
*       TCAM Lookup Iteration Next tested short segment of Rule (current or next)
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       bankEntriesNum      - amount of entries in one TCAM bank
*       pclKeyFormat        - PCL key format
*       currMatchStatus     - status of lookup on current index(relevant to other the short keys)
*       tcamIndexPtr        - index of TCAM rule to check
*
* OUTPUTS:
*       tcamIndexPtr        - index of NEXT TCAM rule to check
*
* RETURN:
*       GT_OK - operation successful
*
* COMMENTS:
*
*
*
***************************************************************************/
static GT_STATUS snetXCatPclSrvTcamLookUpPrvNextSeg
(
    IN SKERNEL_DEVICE_OBJECT        *devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC *tcamCommonDataPtr,
    IN CHT_PCL_KEY_FORMAT_ENT        pclKeyFormat,
    IN GT_STATUS                     currMatchStatus,
    INOUT GT_U32                    *tcamIndexPtr
)
{
    /* the row in XCAT is 4 consecutive  standard rules */
    /* as 0,1,2,3 (row0) or as 100,101,102,103 (row25) */
    /* The bank i.e. the relative index in the row     */
    GT_U32  currentBank = ((*tcamIndexPtr) % tcamCommonDataPtr->bankNumbers);/* the current bank we query */
    GT_U32  pclRuleSize = SNET_CHT_SRV_TCAM_CTRL_COMP_MODE(pclKeyFormat);
    GT_U32  segmentInRule;  /* segment index in rule */
    GT_U32  indexStep;      /* increment index according to matching state */

    if((*tcamIndexPtr) == END_OF_TABLE)
    {
        skernelFatalError("snetXCatPclSrvTcamLookUpPrvNextSeg: table overflow\n");
        return GT_OK;
    }

    /* 0 - Standard key; 0, 1 - Extended key; 0, 1, 2 - Triple key */
    segmentInRule = currentBank % pclRuleSize;

    if(currMatchStatus != GT_OK)
    {
        /* go back to start index of current rule */
        (*tcamIndexPtr) -= segmentInRule;
    }

    /* Switch index to the next bank if current match found */
    indexStep = 1;

    switch (pclKeyFormat)
    {
        case CHT_PCL_KEY_REGULAR_E:
            /* If segment doesn't match rule, next bank == next rule */
            break;
        case CHT_PCL_KEY_EXTENDED_E:
            if(currMatchStatus != GT_OK)
            {
                /* Switch index to the next rule (Bank0 based) */
                indexStep = 2;
            }
            break;
        case CHT_PCL_KEY_TRIPLE_E:
            if(currMatchStatus != GT_OK)
            {
                /* Switch index to the next rule (Bank0 based) */
                indexStep = 4;
            }
            break;
        default:
            /* Not supported key */
            (*tcamIndexPtr) = END_OF_TABLE;
            return GT_OK;
    }

    (*tcamIndexPtr)+= indexStep;

    return GT_OK;
}

/***************************************************************************
* snetChtPclSrvTcamLookUp
*
* DESCRIPTION:
*        TCAM lookup
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       tcamCommonDataPtr   - pointer to TCAM common data structure
*       pclKeyPtr           - pointer to PCL key .
*
* OUTPUTS:
*       matchIndexPtr   - pointer to the matching index.
*
* RETURN:
*       GT_OK   - operation successful
*       GT_FAIL - operation failed
*       GT_OUT_OF_RANGE - index is out of range
*
* COMMENTS:
*       Policy Tcam Table :
*       The policy TCAM holds rules data + mask and used for ingress and egress PCL.
*
*
***************************************************************************/
extern GT_STATUS snetChtPclSrvTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC * tcamCommonDataPtr,
    IN SNET_CHT_POLICY_KEY_STC * pclKeyPtr,
    OUT GT_U32 * matchIndexPtr
)
{
    DECLARE_FUNC_NAME(snetChtPclSrvTcamLookUp);

    GT_U32  maxEntries;         /* TCAM  maximal entries */
    GT_U32  tcamIndex = 0;      /* Index to the TCAM rule */
    GT_U32  bankMatchPattern =0;/* TCAM bank match pattern */
    GT_U32  bankMatchMask = 0;      /* TCAM bank match mask */
    GT_STATUS status;           /* Return status */
    GT_U32  bank;

    ASSERT_PTR(pclKeyPtr);

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_TCAM_E);

    status = GT_NOT_FOUND;

    *matchIndexPtr = END_OF_TABLE;

    /* Total memory entries */
    maxEntries = tcamCommonDataPtr->bankNumbers *
                 tcamCommonDataPtr->bankEntriesNum;

    /* Set start pointer offsets for TCAM pattern/mask data and control */
    SNET_CHT_SRV_TCAM_MEM_INIT(devObjPtr, tcamCommonDataPtr);

    /* Lookup all TCAM entries */
    while (tcamIndex < maxEntries)
    {
        if(SKERNEL_IS_XCAT_DEV(devObjPtr))
        {
            /* the row in XCAT is 4 consecutive standard rules */
            /* as 0,1,2,3 (row0) or as 100,101,102,103 (row25) */
            /* The bank i.e. the relative index in the row     */
            bank = (tcamIndex % tcamCommonDataPtr->bankNumbers);
        }
        else
        {
            bank = (tcamIndex / tcamCommonDataPtr->bankEntriesNum);
        }

        /* For each entry in TCAM set initial pattern and mask  */
        SNET_CHT_SRV_TCAM_PATTERN_MASK_INIT(pclKeyPtr, bank,
                                            bankMatchPattern, bankMatchMask);

        /* Check matching for current bank */
        status = snetChtPclSrvTcamBankHit(
            devObjPtr, tcamCommonDataPtr,
            pclKeyPtr, bank, tcamIndex);

        /* Bank entry match found */
        if (status == GT_OK)
        {
            if(*matchIndexPtr == END_OF_TABLE)
            {
                *matchIndexPtr = tcamIndex;
            }

            /* Set bank pattern and compare with mask */
            SNET_CHT_SRV_TCAM_PATTERN_MASK_CHECK(bank,
                                                 bankMatchPattern,
                                                 bankMatchMask,status)

            /* log tcam info */
            __LOG_TCAM(("bankMatchPattern: %d, bankMatchMask: %d\n",
                           bankMatchPattern, bankMatchMask));

            if(status == GT_OK)
            {
                __LOG_TCAM(("rule matched\n"));
                /* the rule regular/extended/triple was matched */
                /* calc start index */


                break;
            }

            __LOG_TCAM(("rule not matched\n"));
            status = GT_OK;
        }
        else
        {
            *matchIndexPtr = END_OF_TABLE;
        }

        /* Move to next TCAM entry */
        if(SKERNEL_IS_XCAT_DEV(devObjPtr))
        {
            snetXCatPclSrvTcamLookUpPrvNextSeg(
                devObjPtr,
                tcamCommonDataPtr,
                pclKeyPtr->pclKeyFormat,
                status,&tcamIndex);
        }
        else
        {
            snetChtPclSrvTcamLookUpPrvNextSeg(
                devObjPtr,
                tcamCommonDataPtr,
                pclKeyPtr->pclKeyFormat,
                status,&tcamIndex);
        }
    }

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___RESTORE_PREVIOUS_UNIT___E);

    return status;
}

/***************************************************************************
* snetChtPclSrvTcamBankHit
*
* DESCRIPTION:
*        Lookup for match in specific TCAM bank
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       tcamCommonDataPtr   - pointer to TCAM common data structure.
*       pclKeyPtr           - pointer to PCL key structure
*       currentBank         - the number of bank / segment of key
*       entryIndex          - TCAM entry index
*
*
* OUTPUTS:
*       None.
*
* RETURN:
*       GT_OK           - matching is hit
*       GT_NOT_FOUND    - no matching
*       GT_FAIL         - operation failed
*
* COMMENTS:
*
***************************************************************************/
static GT_STATUS snetChtPclSrvTcamBankHit
(
    IN SKERNEL_DEVICE_OBJECT        *devObjPtr,
    IN CHT_PCL_TCAM_COMMON_DATA_STC *tcamCommonDataPtr,
    IN SNET_CHT_POLICY_KEY_STC      *pclKeyPtr,
    IN GT_U32                       currentBank,
    IN GT_U32                       entryIndex
)
{
    DECLARE_FUNC_NAME(snetChtPclSrvTcamBankHit);

    GT_U32  wordIndex;          /* Index to the word in the TCAM rule */
    GT_U8   pclTcamPattern[7];  /* PCL TCAM pattern (up to 52 bits) */
    GT_U8   pclTcamPatternCtrl; /* PCL TCAM pattern control */
    GT_U8   pclTcamMask[7];     /* PCL TCAM mask */
    GT_U8   pclTcamMaskCtrl;    /* PCL TCAM mask control */
    GT_U8   pclKeyData[7];      /* PCL search key data */
    GT_U8   ctrlSearchVal;      /* Data field value */
    GT_BOOL result;             /* Return value result for compare operation */
    GT_STATUS status;           /* Return status */
    GT_U32  wordReference;/* the index of word reference (in global line looking)*/
    GT_U32  pclMode = SNET_CHT_SRV_TCAM_CTRL_COMP_MODE(pclKeyPtr->pclKeyFormat);
    GT_U32  fieldVal;

    wordReference = (currentBank % pclMode) * tcamCommonDataPtr->bankWordsNum ;/* for extended and for triple key*/

    if (devObjPtr->pclTcamFormatVersion >= 1)
    {
        /* The key will hold  the control bits value !!! */
        ctrlSearchVal = 0;/* not used */
    }
    else
    {
        fieldVal = SNET_PCL_TCAM_VALID_BIT_MAC(devObjPtr);
        /* Valid control bit + compare mode control bits 18:19 */
        ctrlSearchVal = (GT_U8)(fieldVal | (pclMode << 2));
    }

    for (wordIndex = 0; wordIndex < tcamCommonDataPtr->bankWordsNum; wordIndex++)
    {
        /* Read TCAM pattern and pattern control words */
        status = snetChtPclSrvTcamWordGet(devObjPtr, tcamCommonDataPtr,
                                          CHT_PCL_TCAM_PATTERN_E,
                                          entryIndex, wordIndex,
                                          pclTcamPattern, &pclTcamPatternCtrl);
        if (status != GT_OK)
        {
            return status;
        }

        /* Read TCAM mask and mask control words */
        status = snetChtPclSrvTcamWordGet(devObjPtr, tcamCommonDataPtr,
                                          CHT_PCL_TCAM_MASK_E,
                                          entryIndex, wordIndex,
                                          pclTcamMask, &pclTcamMaskCtrl);
        if (status != GT_OK)
        {
            return status;
        }

        /* compare Control bits */
        if (devObjPtr->pclTcamFormatVersion == 0)
        {
            /* Compare valid bit and compare mode bits */
            result = tcamCommonDataPtr->tcamCompareFuncPtr(&pclTcamPatternCtrl,
                                                           &pclTcamMaskCtrl,
                                                           &ctrlSearchVal,
                                                           tcamCommonDataPtr->bankWordCtrlBits,
                                                           entryIndex,
                                                           currentBank,
                                                           wordIndex);
            if (result == GT_FALSE)
            {
                break;  /* Control bits do not match */
            }
        }

        /* Read PCL search key word */
        snetChtPclSrvKeyWordGet(devObjPtr, tcamCommonDataPtr, pclKeyPtr,
                wordReference + wordIndex, pclKeyData);

        /* Compare TCAM word pattern and mask and PCL key word */
        result = tcamCommonDataPtr->tcamCompareFuncPtr(pclTcamPattern,
                                                       pclTcamMask,
                                                       pclKeyData,
                                                       tcamCommonDataPtr->bankWordDataBits,
                                                       entryIndex,
                                                       currentBank,
                                                       wordIndex
                                                       );
        if (result == GT_FALSE)
        {
            break;
        }
    }

    /* Match all words in PCL search key */
    if (wordIndex == tcamCommonDataPtr->bankWordsNum)
    {
        /* log tcam entry and word */
        __LOG_TCAM(("key matched\n"));
        __LOG_TCAM(("entry: %d, bank: %d, word: %d\n", entryIndex, currentBank, wordIndex));

        return GT_OK;
    }

    return GT_NOT_FOUND;
}

/*******************************************************************************
*   snetChtPclKeyFieldPutNetworkOrder
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specified place in key
*       used for "numeric" fields - up to GT_U32
*
*       build it network order
*
* INPUTS:
*       bytesKeyPtr  - (pointer to) current pcl key
*       endBit       - field MSB position
*       startBit     - field LSB position
*       fieldVal     - data of field to insert to key
*
* OUTPUTS:
*       wordsKeyPtr - (pointer to) PCL field
*
* RETURN:
*
* COMMENTS:
*     The simulated key consists of 32 bit words that
*     compared "anded" and "compared" with TCAM read-only
*     pattern and mask direct access registers. There is no difference
*     Big or Little endian CPU executes this code, but for PC
*     the bytes will be seen swapped under debugger
*
*******************************************************************************/
static GT_VOID snetChtPclKeyFieldPutNetworkOrder
(
    IN SKERNEL_DEVICE_OBJECT   *devObjPtr,
    INOUT GT_U8                   *bytesKeyPtr,
    IN GT_U32                     endBit,
    IN GT_U32                     startBit,
    IN GT_U32                     fieldVal
)
{
    GT_U32  byteIndex;
    GT_U32  bitIndex;/*index of start bit in the start byte*/
    GT_U32  length;/* length of bits to set */
    GT_U32  numBitsInField;/* number of bits in the field */

    CHECK_FIELD_INTEGRITY(startBit,endBit);

    numBitsInField = endBit - startBit + 1;

    byteIndex = endBit / 8;
    bitIndex  = 7 - (endBit % 8);

    while (numBitsInField)
    {
        length = (numBitsInField < (8 - bitIndex))
                ? numBitsInField : (8 - bitIndex);

        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[0x%2.2x] \n",
                      fieldVal));

        SMEM_U32_SET_FIELD(bytesKeyPtr[byteIndex],/*data*/
                           bitIndex, /*offset*/
                           length, /*length*/
                           fieldVal);/*val*/

        byteIndex--;
        bitIndex = 0;
        numBitsInField -= length;
        fieldVal >>= length;
    }
}

/*******************************************************************************
*   snetChtPclSrvKeyFieldBuildByPointerNetworkOrder
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*       in network order
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvKeyFieldBuildByPointerNetworkOrder
(
    INOUT SNET_CHT_POLICY_KEY_STC       *pclKeyPtr,
    IN GT_U8                                *fieldValPtr,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
)
{
    GT_U32  length;/* length of bits to set */
    GT_U32  offset;/* offset where to write in the word */
    GT_U32  numBitsInField;/* number of bits in the field */

    if (pclKeyPtr->updateOnlyDiff == GT_TRUE)
    {
        if (fieldInfoPtr->updateOnSecondCycle == GT_FALSE)
        {
            /* don't update on second cycle */
            return;
        }
    }

    if (fieldValPtr == 0)
    {
        return ;
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%s] startBitInKey[%d] endBitInKey[%d] value -->",
                  fieldInfoPtr->debugName,
                  fieldInfoPtr->startBitInKey,
                  fieldInfoPtr->endBitInKey));

    numBitsInField = fieldInfoPtr->endBitInKey -
                     fieldInfoPtr->startBitInKey +
                     1;
    offset = fieldInfoPtr->startBitInKey;

    while (numBitsInField > 0)
    {
        length = (numBitsInField >= 8) ? 8 : numBitsInField;

        snetChtPclKeyFieldPutNetworkOrder(
            pclKeyPtr->devObjPtr,
            SNET_CHT_PCL_KEY_PTR(pclKeyPtr),
            (offset + length - 1),
            offset,
            fieldValPtr[(offset-fieldInfoPtr->startBitInKey) / 8]);

        numBitsInField -= length;
        offset += length;
    }


    return;
}

/*******************************************************************************
*   snetChtPclKeyFieldBuildByValue
*
* DESCRIPTION:
*       function inserts data of the field to the search key
*       in specific place in key
*       used for "numeric" fields - up to GT_U32
*       in network order
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldVal - data of field to insert to key
*       fieldId -- field id
*       cycleNum -  the number of current cycle
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
extern GT_VOID snetChtPclSrvKeyFieldBuildByValueNetworkOrder
(
    INOUT SNET_CHT_POLICY_KEY_STC           *pclKeyPtr,
    IN GT_U32                                fieldVal,
    IN CHT_PCL_KEY_FIELDS_INFO_STC          *fieldInfoPtr
)
{
    if (pclKeyPtr->updateOnlyDiff == GT_TRUE)
    {
        if (fieldInfoPtr->updateOnSecondCycle == GT_FALSE)
        {
            /* don't update on second cycle */
            return;
        }
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%s] startBitInKey[%d] endBitInKey[%d] value[0x%8.8x] \n",
                  fieldInfoPtr->debugName,
                  fieldInfoPtr->startBitInKey,
                  fieldInfoPtr->endBitInKey,
                  fieldVal));

    snetChtPclKeyFieldPutNetworkOrder(
        pclKeyPtr->devObjPtr,
        SNET_CHT_PCL_KEY_PTR(pclKeyPtr),
        fieldInfoPtr->endBitInKey,
        fieldInfoPtr->startBitInKey,
        fieldVal);

    return;
}


/*******************************************************************************
*   snetChtPclSrvGenericTcamLookup
*
* DESCRIPTION:
*       generic function that do lookup in tcam for given key and return the
*       matching index.
*
* INPUTS:
*       devObjPtr - (pointer to) the device object
*       lookUpKeyPtr - (pointer to) current lookup key
*
* OUTPUTS:
*       matchIndexPtr - (pointer to) the match index
*                   if no match - return value SNET_CHT_POLICY_NO_MATCH_INDEX_CNS
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetChtPclSrvGenericTcamLookup
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_DEVICE_TCAM_INFO_STC *tcamInfoPtr,
    IN SNET_CHT_POLICY_KEY_STC *lookUpKeyPtr,
    OUT GT_U32  *matchIndexPtr
)
{
    DECLARE_FUNC_NAME(snetChtPclSrvGenericTcamLookup);

    GT_U32   maxEntries;  /* max number of entries in the TCAM */
    GT_U32   entryIndex;  /* Index to the TCAM line */
    GT_U32  *xdataPtr;   /* pointer to current TCAM data X entry */
    GT_U32  *ydataPtr;   /* pointer to current TCAM data Y entry */
    GT_U32  comparedWordIndex;/* index of the compared word */
    GT_U32  numWordsToCompare = (tcamInfoPtr->numBits + 31)/32;/* number of words to compare  */
    GT_U32  numBitsLastWord = tcamInfoPtr->numBits & 0x1f;/* number of bits in last word*/
    GT_U32  lastWordComparedMask = SMEM_U32_GET_FIELD(0xFFFFFFFF,0,numBitsLastWord);/*mask on last word comparing*/
    GT_U32  bankIndex;/*index of current bank*/
    GT_U32  numBanksToCompare = tcamInfoPtr->numberOfBanks;/* number of bank to compare*/
    GT_U32  lookUpKeyWord;/* lookup key word - build from the key by the CallBack function*/
    GT_U32  compareMask;/* compare mask for current word*/
    GT_U32  nextRuleOffset;/* offset in words from X of rule of entry A to X of entry A+1 (same for Y) */
    GT_U32  tempVal = 0;

    /* mark index as No Match */
    *matchIndexPtr = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;

    maxEntries = tcamInfoPtr->numEntries;
    nextRuleOffset = (tcamInfoPtr->entryWidth * 2);

    /*  Get pointer to Tcam X data entry */
    xdataPtr = smemMemGet(devObjPtr, tcamInfoPtr->xMemoryAddress);
    /*  Get pointer to Tcam Y data entry */
    ydataPtr = smemMemGet(devObjPtr, tcamInfoPtr->yMemoryAddress);

    for (entryIndex = 0 ; entryIndex <  maxEntries; entryIndex++ ,
        xdataPtr += nextRuleOffset,
        ydataPtr += nextRuleOffset)
    {
        for(bankIndex = 0 ; bankIndex < numBanksToCompare; bankIndex++)
        {
            for(comparedWordIndex = 0 ; comparedWordIndex < numWordsToCompare ;comparedWordIndex++)
            {
                lookUpKeyWord = tcamInfoPtr->lookUpKeyWordBuildPtr(devObjPtr,tcamInfoPtr,
                                lookUpKeyPtr,bankIndex,comparedWordIndex);

                compareMask = (comparedWordIndex != numWordsToCompare - 1) ? 0xFFFFFFFF:/* not last word*/
                             lastWordComparedMask;   /*last word*/

                tempVal = ((~ydataPtr[(bankIndex*numWordsToCompare) + comparedWordIndex] &  lookUpKeyWord) |
                           (~xdataPtr[(bankIndex*numWordsToCompare) + comparedWordIndex] & ~lookUpKeyWord)) & compareMask;

                if(tempVal != (compareMask & 0xFFFFFFFF))
                {
                    /* log tcam entry and bank */
                    __LOG_TCAM(("no match \n"));
                    __LOG_TCAM(("entry index: %d, bank index: %d\n", entryIndex, bankIndex));
                    simLogTcamBitsCausedNoMatchInTheEntry(devObjPtr, tempVal);

                    /* no match */
                    break;
                }
            }

            if(comparedWordIndex != numWordsToCompare)
            {
                /* no match */
                __LOG_TCAM(("no match \n"));
                break;
            }
        }

        if(bankIndex == numBanksToCompare)
        {
            /* match*/
            __LOG_TCAM(("match\nentry index: %d, bank index: %d\n", entryIndex, bankIndex));

            *matchIndexPtr = entryIndex;
            return;
        }
    }

    /* no entry did match*/
    return;
}

/*******************************************************************************
* snetLion2TtiTrillAdjacencyLookUpKeyWordBuild
*
* DESCRIPTION:
*       call back function for tcamInfoPtr->lookUpKeyWordBuildPtr
*        Lion2 TRILL tcam build of lookup key word according to the key and the
*        word index to build
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       ttiKeyTypePtr  - pointer to tti key type
*
*
* RETURN:
*       GT_BOOL TRUE DOING T.T false
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 snetLion2TtiTrillAdjacencyLookUpKeyWordBuild
(
    IN SKERNEL_DEVICE_OBJECT        *devObjPtr,
    IN SKERNEL_DEVICE_TCAM_INFO_STC  *tcamInfoPtr,
    IN SNET_CHT_POLICY_KEY_STC *lookUpKeyPtr,
    IN GT_U32 bankIndex,
    IN GT_U32 wordIndex
)
{
    GT_U32 lookUpKeyWord = 0;/* the word value to return from the function */
    GT_U32 startByteIndex = 0;/* start byte index in the key for building the word */
    GT_U32 controlBitsValue = 0;/*extra control bits value if needed */
    GT_U32 mask = 0;/* mask for the current word from the key */


    /* the key is split on 2 banks */
    switch(bankIndex)
    {
        case 0:
            startByteIndex = 0;
            break;
        case 1:
            startByteIndex = 6;/* the first 6 bytes in bank 0*/
            break;
        default:
            skernelFatalError("snetLion2TtiTrillAdjacencyLookUpKeyWordBuild: bankIndex out of range\n");
    }

    /* this tcam is 52bits : so only 2 words */
    switch(wordIndex)
    {
        case 0:
            startByteIndex += 0;
            mask = 0xFFFFFFFF;
            break;
        case 1:
            startByteIndex += 4;/*the first 4 bytes in word 0*/
            if(bankIndex == 0)
            {
                mask = 0x0000FFFF;/* 48 bits data (16+32) */
            }
            else
            {
                mask = 0x0003FFFF;/* 50 bits data (18+32) */
            }
            controlBitsValue = 1 << (51 - 32);/*valid bit 51*/
            break;
        default:
            skernelFatalError("snetLion2TtiTrillAdjacencyLookUpKeyWordBuild: wordIndex out of range\n");
    }

    lookUpKeyWord = (lookUpKeyPtr->key.regular[startByteIndex+0] << 0 ) |
                    (lookUpKeyPtr->key.regular[startByteIndex+1] << 8 ) |
                    (lookUpKeyPtr->key.regular[startByteIndex+2] << 16) |
                    (lookUpKeyPtr->key.regular[startByteIndex+3] << 24) ;

    lookUpKeyWord &= mask;
    lookUpKeyWord |= controlBitsValue;


    return lookUpKeyWord;
}

