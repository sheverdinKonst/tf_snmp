/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCht2TunnelStart.c
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Tunnel Start Engine processing for outgoing frame.
*       Source Code file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 56 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SLog/simLog.h>

#define VERSION_IPV4 4
#define VERSION_IPV6 6
#define IPV4_HEADER_LENGHT 5
#define UDP_HEADER_LENGHT  8
#define GRE_BASIC_HEADER_LENGHT  4
#define BYTES_IN_WORD 4

#define SIP5_TSE_FIELD_GET_MAC(field)\
    SMEM_LION3_HA_TUNNEL_START_ENTRY_FIELD_GET(devObjPtr, tunnelStartActionPtr, descrPtr->tunnelPtr, field)

#define SIP5_TSE_PROFILE_FIELD_GET_MAC(field)\
    SMEM_LION3_HA_GENERIC_TS_PROFILE_FIELD_GET(devObjPtr, tsInfoPtr->profileEntryPtr, tsInfoPtr->profileNum, field)


typedef struct{
    GT_U32      dscp;
    GT_BIT      gre; /* indicates whether packet gre */
    GT_BIT      udp; /* indicates whether packet udp */
    GT_U32      ttl;
    GT_U32      protocol; /* "protocol" field in ipv4, in ipv6 named as "next header" */
    GT_U32      sip; /* used for ipv4 only */
    GT_U32      dip; /* used for ipv4 only */

    /* mpls info */
    GT_U32      numOfLabels;
    GT_U32      exp[3];  /* index 0 for exp1  , index 1 for exp2  , index 2 for exp3 */
    GT_U32      label[3];/* index 0 for exp1  , index 1 for exp2  , index 2 for exp3 */
    GT_U32      sBit[3];/* index 0 for exp1  , index 1 for exp2  , index 2 for exp3 */

    /* MacInMac fields */
    /* New fields are not relevant for CH2 / CH3 */
    GT_U32 iSid;                    /* MacInMac inner service identifier      */
    GT_U32 iUp;                     /* inner user priority field in iTag      */
    GT_BIT iDp;                     /* inner drop precedence in the iTag      */
    GT_U32 iTagRes2;                /* 2 reserved bits to be set in the I-Tag */
    GT_BIT iTagRes1;                /* 1 reserved bits to be set in the I-Tag */
    GT_BIT iTagCda;                 /* CDA field to be set in the I-Tag       */

    GT_U32  controlWordIndex;       /*control word index (MPLS)
                    Index to one of 7 Control Word entries
                    0 =  Do not add Control Word
                    1-7 = Add the inner most MPLS label, insert the Control Word from the respective entry in the Control Word Table
                    The CW label will always be placed directly after the last MPLS header of the TS
                    */

    GT_U32  doNotFragmentFlag;/*do not fragment flag - relevant when != 0xFFFFFFFF, ipv4 only */

    /* sip5 only */
    GT_U32   eVlanServiceId;         /* eVLAN <Service-ID> attribute */
    GT_U32   eVlanServiceIdShifted;  /* shifted eVLAN <Service-ID> */
    GT_U32   profileNum;             /* ip profile number */
    GT_U32  *profileEntryPtr;        /* ip profile entry pointer */
    GT_U32   packetTotalLen;         /* packet total length value, can be modified during sip5 generic TS */
    GT_U8   *ipTotalLenFieldInEgressBufPtr;   /* pointer to the ipv4/6 <total length> field within the egress buffer.
                                                Needed for dynamic setting of this field */

}CH2_TS_INFO_STC;


/*******************************************************************************
*   calculateAndAppendPacketTotalLength
*
* DESCRIPTION:
*        Calculates and appends packet total length to egress buffer
*
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        egressPort   - the local egress port (not global port)
*        tsInfoPtr    - TS internal info
*        egrBufPtr    - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* calculateAndAppendPacketTotalLength
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN HA_INTERNAL_INFO_STC            *haInfoPtr,
    IN GT_U32                           egressPort,
    IN CH2_TS_INFO_STC                 *tsInfoPtr,
    IN GT_U8                           *egrBufPtr
)
{
    DECLARE_FUNC_NAME(calculateAndAppendPacketTotalLength);

    GT_U8         tmpBuffer[2];
    GT_U32        fldValue;
    GT_U32        totalLen;     /* the total length of the packet */

    /* add payload length first, this is also known as <Passenger packet byte count> */
    /* NOTE: this calculation NOT considers L2 of "Ethernet passenger", so additional
       manipulation of the <total length> (<payload length> in ipv6) is done in chtHaUnit(...),
       see use of haInfo.tsIpv4HeaderPtr, haInfo.tsIpv6HeaderPtr */

    if(descrPtr->tunnelStartPassengerType == 0 /* passenger is ethernet */ ||
       descrPtr->isIp == 0 || descrPtr->isFcoe )
    {
        totalLen = (descrPtr->byteCount - (descrPtr->l3StartOffsetPtr - descrPtr->origInfoBeforeTunnelTermination.originalL2Ptr));

        /* Ignore devObjPtr->errata.tunnelStartPassengerEthPacketCrcRemoved !!!
           (current tests expect the <total length> to ignore it)
        */
        if(devObjPtr->crcPortsBytesAdd == 0 &&
            haInfoPtr->retainCrc == 0)
        {
            /* remove the 4 bytes of the CRC of the passenger */
            totalLen -= 4;
        }
    }
    else
    {
        /* use the length of the passenger 'ipv4/6 ' , because it may suggest
           length shorter then length till end of packet.

           this is actual HW behavior.
        */
        totalLen = descrPtr->ipxLength;
    }

    if(SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV6_E != descrPtr->tunnelStartType)
    { /* ipv4 code, not relevant for ipv6 */

        /* add ipv4 header length, constant 20 (the size in bytes of the tunneling IPv4 * header) */
        totalLen += IPV4_HEADER_LENGHT*4;
    }


    if(tsInfoPtr->gre)
    {
        /* add the 4 byte of the basic GRE */
        totalLen += GRE_BASIC_HEADER_LENGHT;
    }

    if (devObjPtr->supportTunnelstartIpTotalLengthAddValue)
    {
        /* for Ethernet-over-IPv4/IPv4_GRE packets */
        /* If the option to add offset to <IP Tunnel Total Length> is enabled per port */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fldValue = SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_IP_TUNNEL_LENGTH_OFFSET_ENABLE);
            if(fldValue)
            {
                /* Then add the additional offset */
                smemRegFldGet(devObjPtr, SMEM_LION3_HA_IP_LENGTH_OFFSET_REG(devObjPtr),0, 6 , &fldValue);
                totalLen += fldValue;
            }
        }
        else
        {
            smemRegFldGet(devObjPtr, SMEM_XCAT_HA_TRUNK_ID_FOR_PORT_BASE_REG(devObjPtr,egressPort),28 + (egressPort % 4), 1 , &fldValue);
            if (fldValue != 0)
            {
                /* Then add the additional offset */
                smemRegFldGet(devObjPtr, SMEM_XCAT_HA_CPID_1_REG(devObjPtr),26, 6 , &fldValue);
                totalLen += fldValue;
            }
        }
    }

    /* save packet total len */
    tsInfoPtr->ipTotalLenFieldInEgressBufPtr = egrBufPtr;
    tsInfoPtr->packetTotalLen = totalLen;

    tmpBuffer[0] = (GT_U8)(totalLen >> 8);
    tmpBuffer[1] = (GT_U8)(totalLen);

    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    __LOG(("added total length [0x%2.2x%2.2x] \n", tmpBuffer[0], tmpBuffer[1]));

    return egrBufPtr;
}

/*******************************************************************************
*   ipv4HeaderIdentificationFieldGet
*
* DESCRIPTION:
*        get and update the the value of <TunnelStartFragmentID> that is used for
*        field <Identification> in the IPv4 header .
* INPUTS:
*        devObjPtr    - pointer to device object.
*
* OUTPUTS:
*
* RETURNS: value of <Identification> to be used in the ipv4 that the device
*           build as part of the TS
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32   ipv4HeaderIdentificationFieldGet(
  IN SKERNEL_DEVICE_OBJECT           *devObjPtr
)
{
    DECLARE_FUNC_NAME(ipv4HeaderIdentificationFieldGet);

    GT_U32  tunnelStartFragmentID;
    GT_U32  fldValue;
    GT_U32  regAddr;
    GT_U32  bit_TunnelStartFragmentIDIncEn = SMEM_CHT_IS_SIP5_GET(devObjPtr) ? 16: 31;

    regAddr = SMEM_CHT2_HA_TUNNEL_START_FRAGMENT_ID_CONFIG_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddr, &fldValue);
    /* get <TunnelStartFragmentID> */
    tunnelStartFragmentID = SMEM_U32_GET_FIELD(fldValue,0,16);
    /* check <TunnelStartFragmentIDIncEn > */
    if(SMEM_U32_GET_FIELD(fldValue,bit_TunnelStartFragmentIDIncEn,1))
    {
        smemRegFldSet(devObjPtr, regAddr, 0,16 ,tunnelStartFragmentID + 1);

        __LOG(("Update 'TunnelStartFragmentID' Counter from [%d] \n",
            tunnelStartFragmentID));
    }
    else
    {
        __LOG(("NOT Update 'TunnelStartFragmentID' Counter , use value [%d] \n",
            tunnelStartFragmentID));
    }

    return tunnelStartFragmentID;
}

/*******************************************************************************
*   buildIpv4Header
*
* DESCRIPTION:
*        Create buffer for tunnel start - IPv4 header.
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        egressPort   - the local egress port (not global port)
*        tsInfoPtr    - TS internal info
*        egrBufPtr    - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* buildIpv4Header
(
  IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
  IN HA_INTERNAL_INFO_STC            *haInfoPtr,
  IN GT_U32                           egressPort,
  IN CH2_TS_INFO_STC                 *tsInfoPtr,
  IN GT_U8                           *egrBufPtr
)
{
    DECLARE_FUNC_NAME(buildIpv4Header);

    GT_U8         tmpBuffer[4];
    GT_U32        ipV4Encap;      /* Encapsulated IPv4 Header     */
    GT_U32        tmpWord;

    /* start with the ether type */
    tmpBuffer[0] = 0x08;
    tmpBuffer[1] = 0x00;
    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    __LOG(("added EtherType [0x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1]
        ));

    /* save pointer to the start of the ipv4 */
    haInfoPtr->tsIpv4HeaderPtr = egrBufPtr;

    /* version,bytes,Tos */
    ipV4Encap = 0;
    ipV4Encap = (VERSION_IPV4 << 28);

    ipV4Encap = ipV4Encap | (IPV4_HEADER_LENGHT << 24);
    ipV4Encap = ipV4Encap |(tsInfoPtr->dscp << 18);
    SNET_BUILD_BYTES_FROM_WORD_MAC(ipV4Encap,tmpBuffer);
    MEM_APPEND(egrBufPtr,tmpBuffer, 2);

    __LOG(("added version,bytes,Tos [0x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1]
        ));

    /* packet total length - 16 bits */
    egrBufPtr = calculateAndAppendPacketTotalLength(devObjPtr, descrPtr, haInfoPtr,
                                                    egressPort, tsInfoPtr, egrBufPtr);

    /* set fields of : Identification , flags , fragment offset*/
    /*<Identification>*/
    tmpWord = ipv4HeaderIdentificationFieldGet(devObjPtr) << 16;
    /*<Fragment Offset>=0*/
    if(tsInfoPtr->doNotFragmentFlag == 1)
    {
        tmpWord |= 1 << 14;
        /*tmpBuffer[2] |=  1 << 6;*/
    }

    SNET_BUILD_BYTES_FROM_WORD_MAC(tmpWord,tmpBuffer);

    MEM_APPEND(egrBufPtr,tmpBuffer, 4);


    __LOG(("added Identification , flags , fragment offset [0x%2.2x%2.2x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1],
        tmpBuffer[2],
        tmpBuffer[3]
        ));

    /* TTL and Protocol */
    /* length ip fragment*/
    tmpBuffer[0] = tsInfoPtr->ttl ;
    tmpBuffer[1] = tsInfoPtr->protocol;

    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    __LOG(("added TTL and Protocol [0x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1]
        ));

    /* Set 0 in the checksum */
    __LOG(("Set 0 in the checksum"));
    tmpBuffer[0] = 0 ;
    tmpBuffer[1] = 0;
    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    __LOG(("added the checksum (before re-calc) [0x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1]
        ));

    /* build the SIP */
    __LOG(("build the SIP"));
    SNET_BUILD_BYTES_FROM_WORD_MAC(tsInfoPtr->sip,tmpBuffer);
    MEM_APPEND(egrBufPtr,tmpBuffer, 4);

    __LOG(("added SIP [0x%2.2x%2.2x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1],
        tmpBuffer[2],
        tmpBuffer[3]
        ));

    /* build the DIP */
    __LOG(("build the DIP"));
    SNET_BUILD_BYTES_FROM_WORD_MAC(tsInfoPtr->dip,tmpBuffer);
    MEM_APPEND(egrBufPtr,tmpBuffer, 4);

    __LOG(("added DIP [0x%2.2x%2.2x%2.2x%2.2x] \n",
        tmpBuffer[0],
        tmpBuffer[1],
        tmpBuffer[2],
        tmpBuffer[3]
        ));


    return egrBufPtr;
}

/*******************************************************************************
*   buildMimHeader
*
* DESCRIPTION:
*        Create buffer for tunnel start - MIM header.
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        tsInfoPtr    - TS internal info
*        egrBufPtr    - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* buildMimHeader
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN CH2_TS_INFO_STC       *tsInfoPtr,
  IN GT_U8                 *egrBufPtr
)
{
    DECLARE_FUNC_NAME(buildMimHeader);

    GT_U32        iTag,tmpVal;
    GT_U8         tmpBuffer[4];

    /* I-Ethertype */
    smemRegFldGet(devObjPtr,SMEM_XCAT_HA_I_ETHERTYPE_REG(devObjPtr), 0, 16, &tmpVal);
    tmpBuffer[0] = (GT_U8)(tmpVal >> 8);
    tmpBuffer[1] = (GT_U8)(tmpVal >> 0);
    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    /* I-UP,I-DP,RES,I-SID */
    iTag  = tsInfoPtr->iUp << 29;
    iTag |= (tsInfoPtr->iDp << 28);
    iTag |= (tsInfoPtr->iTagCda << 27);
    iTag |= (tsInfoPtr->iTagRes1 << 26);
    iTag |= (tsInfoPtr->iTagRes2 << 24);
    iTag |= (tsInfoPtr->iSid);
    SNET_BUILD_BYTES_FROM_WORD_MAC(iTag,tmpBuffer);
    MEM_APPEND(egrBufPtr,tmpBuffer, 4);

    __LOG(("added MIM header [0x%8.8x] \n",
        iTag));

    return egrBufPtr;
}

/*******************************************************************************
*   buildGreBytes
*
* DESCRIPTION:
*        Create buffer for tunnel start - GRE bytes. (4 or 8 bytes)
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        egrBufPtr    - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* buildGreBytes
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN HA_INTERNAL_INFO_STC*     haInfoPtr,
  IN GT_U8                 *egrBufPtr,
  IN GT_U32                *tunnelStartActionPtr
)
{
    DECLARE_FUNC_NAME(buildGreBytes);

    GT_U8         tmpBuffer[4];
    GT_U32        tmpVal;
    GT_U32        greWord,greKey,secondGreWord;
    GT_U32        doSecondWord;
    GT_U32        fieldValue;
    GT_U32        greKeyMode = 0;
    GT_U32        isIpGenericKey;
    GT_U32        expectedGreProtocol;

    /*
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |C|       Reserved0       | Ver |         Protocol Type         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |      Checksum (optional)      |       Reserved1 (Optional)    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


    C (Checksum Present) -- This field is always set to zero.
        The device never adds a checksum to the GRE header.
    Reserved0 --  This field is always set to zero.
    Ver -- This field is always set to zero.
    Protocol Type
            If the passenger is IPv4 packet,the GRE protocol is set to 0x0800.
            If the passenger is IPv6 packet,the GRE protocol is set to 0x86DD.
            If the passenger is Ethernet packet (ip or non ip), the GRE protocol is set to 0x6558. Do not verify or test.
    */

    isIpGenericKey = (descrPtr->tunnelStartType == SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV4_E ||
                      descrPtr->tunnelStartType == SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV6_E ) ? 1 : 0;
    if(tunnelStartActionPtr == NULL)
    {
        /* just to avoid static code analyzer to warn about NULL port accessing
           in MACRO : SIP5_TSE_FIELD_GET_MAC */
        isIpGenericKey = 0;
    }

    greWord = 0;
    fieldValue = 0;

    if(isIpGenericKey)
    {
        greWord |= SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_GRE_FLAGS_AND_VERSION) << 16;
    }

    if(isIpGenericKey && SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {

        /* (different from sip5) : using the <GRE protocol> regardless to passenger type */
        fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_GRE_PROTOCOL);

        if(descrPtr->tunnelStartPassengerType && descrPtr->isIp && descrPtr->isFcoe == 0)
        {
            if(descrPtr->isIPv4)
            {
                expectedGreProtocol = SKERNEL_L3_PROT_TYPE_IPV4_E;
            }
            else
            {
                expectedGreProtocol = SKERNEL_L3_PROT_TYPE_IPV6_E;
            }

            if(expectedGreProtocol != fieldValue)
            {
                __LOG(("GRE protocol : Warning: got [0x%4.4x] from TS<GRE protocol> field but suspected as configuration ERROR , that should be [0x%4.4x] \n",
                    fieldValue,expectedGreProtocol));
            }
        }

        __LOG(("GRE protocol : use TS <GRE protocol> [0x%4.4x] (regardless to passenger type) \n" ,
            fieldValue));

    }
    else
    if (descrPtr->tunnelStartPassengerType == 0)
    {
        /* Ethernet */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(isIpGenericKey)
            {
                /* take from the TS entry */
                fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_GRE_PROTOCOL);

                __LOG(("GRE protocol : for 'ethernet' passenger use TS <GRE protocol> [0x%4.4x] \n" ,
                    fieldValue));
            }
            else
            {
                /* take from global configuration */

                /* Ethernet over GRE Protocol */
                smemRegFldGet(devObjPtr,SMEM_LION3_HA_ETHERNET_OVER_GRE_PROTOCOL_TYPE_REG(devObjPtr),
                    0, 16, &tmpVal);
                fieldValue = tmpVal & 0xFFFF;

                __LOG(("GRE protocol : for IP 'ethernet' passenger 'global configuration' <GRE protocol> [0x%4.4x] \n" ,
                    fieldValue));
            }
        }
        else
        {
            /* take from 'hard coded' value */
            fieldValue = 0x6558;
            __LOG(("GRE protocol : take from 'hard coded' <GRE protocol> [0x%4.4x] \n" ,
                fieldValue));
        }
    }
    else if(descrPtr->isIp)
    {
        if (descrPtr->isIPv4 == 0)/*Ipv6*/
        {
            fieldValue = SKERNEL_L3_PROT_TYPE_IPV6_E;
        }
        else                      /*Ipv4*/
        {
            fieldValue = SKERNEL_L3_PROT_TYPE_IPV4_E;
        }
        __LOG(("GRE protocol : for IP 'ethernet other' passenger use implicit <GRE protocol> [0x%4.4x] , according to ipv4/ipv6 passenger \n" ,
            fieldValue));
    }

    greWord |= fieldValue;

    SNET_BUILD_BYTES_FROM_WORD_MAC(greWord,tmpBuffer );
    MEM_APPEND(egrBufPtr, tmpBuffer, 4);

    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* not supporting 8 bytes GRE */
        doSecondWord = 0;
    }
    else if(tunnelStartActionPtr)
    {
        /* should not happen ... just to avoid warnings */
        doSecondWord = 0;
    }
    else
    {
        greKeyMode = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_GRE_KEY_MODE);
        /*
            0x0 = 4B GRE Header; Use 4B GRE Header;
            0x1 = 8B GRE with TSE key; GRE Key is taken from TS entry;
            0x2 = 8B GRE with hash key; GRE Key is assigned from the packet hash value;
            0x3 = HP GRE Key mode; HP GRE Key Mode;
        */

        if(greKeyMode == 0)/*0x0 = 4B GRE Header; Use 4B GRE Header;*/
        {
            /* only 1 word GRE */
            doSecondWord = 0;
        }
        else
        {
            doSecondWord = 1;
        }
    }

    if(doSecondWord)
    {
        __LOG(("added GRE first word [0x%8.8x] \n",
            greWord));
    }
    else
    {
        __LOG(("added GRE word [0x%8.8x] \n",
            greWord));

        return egrBufPtr;
    }

    /* ePort <TS Extension> */
    tmpVal = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                        descrPtr,
                        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_TS_EXT);

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_IPV4_GRE_KEY_1);
    greKey = fieldValue << 16;

    greKey |= SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_IPV4_GRE_KEY_0);


    if(greKeyMode == 1)/*0x1 = 8B GRE with TSE key; GRE Key is taken from TS entry;*/
    {
        secondGreWord = greKey;
    }
    else
    if(greKeyMode == 2)/*0x2 = 8B GRE with hash key; GRE Key is assigned from the packet hash value;*/
    {
        secondGreWord = descrPtr->pktHash;
    }
    else              /*0x3 = HP GRE Key mode; HP GRE Key Mode;*/
    {
        /*
         GRE Key[11:0] = egress packet is tagged ? outer tag VID : desc<eVLAN[11:0]>
         GRE Key[12] = TS<GREKey[12]>
         GRE Key[15:13] = desc<Tag0UP>
         GRE Key[29:16] = TS<GREKey[29:16]>
         GRE Key[30] = TS<GREKey[30]>
         GRE Key[31] = desc<Tag0SrcTag>
        */
        secondGreWord = haInfoPtr->tsVlanTagInfo.vlanTagged ?
            haInfoPtr->tsVlanTagInfo.vlanId :
            descrPtr->eVid;

        SMEM_U32_SET_FIELD(secondGreWord,12,1,
            SMEM_U32_GET_FIELD(greKey,12,1));

        SMEM_U32_SET_FIELD(secondGreWord,13,3,descrPtr->up);

        SMEM_U32_SET_FIELD(secondGreWord,16,15,
            SMEM_U32_GET_FIELD(greKey,16,15));

        SMEM_U32_SET_FIELD(secondGreWord,31,1,TAG0_EXIST_MAC(descrPtr));
    }

    __LOG(("added GRE second word  [0x%8.8x] \n",
        secondGreWord));

    SNET_BUILD_BYTES_FROM_WORD_MAC(secondGreWord,tmpBuffer );
    MEM_APPEND(egrBufPtr, tmpBuffer, 4);

    return egrBufPtr;
}

/*******************************************************************************
*   snetCht2HaTunnelStartL3
*
* DESCRIPTION:
*        Create buffer for tunnel start - L3 info
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        egressPort   - the local egress port (not global port)
*        egrBufPtr    - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* snetCht2HaTunnelStartL3
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN HA_INTERNAL_INFO_STC*     haInfoPtr,
  IN GT_U32                 egressPort,
  IN CH2_TS_INFO_STC       *tsInfoPtr,
  IN GT_U8                 *egrBufPtr
)
{
    DECLARE_FUNC_NAME(snetCht2HaTunnelStartL3);

    GT_U32        mplsEncap;      /* Encapsulated MPLS Header     */
    GT_U32        regAddress;     /* register address             */
    GT_U8         tmpBuffer[4];
    GT_U32        fldValue;
    GT_U32        ii;/* iterator*/

    if (descrPtr->tunnelStartType == SKERNEL_FRAME_TUNNEL_START_TYPE_MPLS_E)
    {
        if (SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
        {
            /* Read the Ethertype used to identify MPLS Unicast packets */
            regAddress = SMEM_XCAT_HA_MPLS_ETHERTYPES_REG(devObjPtr);
            smemRegFldGet(devObjPtr, regAddress, 0, 16, & fldValue);
            tmpBuffer[0] = (GT_U8)(fldValue >> 8);
            tmpBuffer[1] = (GT_U8)(fldValue);
        }
        else
        {
            /* start with the ether type */
            __LOG(("start with the ether type"));
            tmpBuffer[0] = 0x88;
            tmpBuffer[1] = 0x47;
        }

        MEM_APPEND(egrBufPtr, tmpBuffer, 2);

        for(ii = tsInfoPtr->numOfLabels + 1; ii > 0 ; ii--)
        {
            mplsEncap = 0;
            /* TTL */
            SMEM_U32_SET_FIELD(mplsEncap,0,8,tsInfoPtr->ttl);
            /* s (sBit)  */
            SMEM_U32_SET_FIELD(mplsEncap,8,1,tsInfoPtr->sBit[ii-1]);
            /* EXP */
            SMEM_U32_SET_FIELD(mplsEncap,9,3,tsInfoPtr->exp[ii-1]);
            /* label */
            SMEM_U32_SET_FIELD(mplsEncap,12,20,tsInfoPtr->label[ii-1]);

            SNET_BUILD_BYTES_FROM_WORD_MAC(mplsEncap,tmpBuffer );
            MEM_APPEND(egrBufPtr, tmpBuffer, 4);
        }

        if(tsInfoPtr->controlWordIndex)
        {
            /* Set HA descriptor fields relevant to MPLS tunnel start */
            descrPtr->tsEgressMplsControlWordExist = 1;/* not really used */

            /* place the relevant <Control Word<n>> directly after the last MPLS header of the TS*/
            __LOG(("place the relevant <Control Word<n>> directly after the last MPLS header of the TS"));

            /* Control Word */
            regAddress = SMEM_XCAT_HA_TS_MPLS_CONTROL_WORD_REG(devObjPtr,
                            tsInfoPtr->controlWordIndex);/* NOT need to do (tsInfoPtr->controlWordIndex-1) */

            smemRegGet(devObjPtr, regAddress, & fldValue);

            SNET_BUILD_BYTES_FROM_WORD_MAC(fldValue,tmpBuffer );
            MEM_APPEND(egrBufPtr, tmpBuffer, 4);
        }

    }
    else if(descrPtr->tunnelStartType == SKERNEL_FRAME_TUNNEL_START_TYPE_IPV4_E)
    {
        /* build the ipv4 header */
        egrBufPtr = buildIpv4Header(devObjPtr,descrPtr, haInfoPtr, egressPort, tsInfoPtr, egrBufPtr);

        if (tsInfoPtr->gre)
        {
            /* Create and insert GRE encapsulation */
            egrBufPtr = buildGreBytes(devObjPtr,descrPtr,haInfoPtr,egrBufPtr,NULL);
        }
    }/* end of ipv4/GRE*/
    else if (descrPtr->tunnelStartType == SKERNEL_FRAME_TUNNEL_START_TYPE_MIM_E)
    {
        /* build the MIM header */
        egrBufPtr = buildMimHeader(devObjPtr,descrPtr,
                tsInfoPtr,egrBufPtr);
    }

    /* return the updated pointer */
    return egrBufPtr;
}

/*******************************************************************************
*   lion3BuildMplsPwLabel
*
* DESCRIPTION:
*         build  label of MPLS - 'PW label'
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        egrBufPtr    - pointer to the buffer where to put the data
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8 *lion3BuildMplsPwLabel
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN GT_U8                 *egrBufPtr,
  IN GT_BIT                 sBit
)
{
    DECLARE_FUNC_NAME(lion3BuildMplsPwLabel);

    GT_U8   tmpBuffer[4];
    GT_U32  fieldValue;
    GT_U32  mplsEncap;

    __LOG(("use sBit[%d]\n",
        sBit));

    mplsEncap = 0;
    /* TTL */
    fieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
        descrPtr,
        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_MPLS_PW_LABEL_TTL);
    SMEM_U32_SET_FIELD(mplsEncap,0,8,fieldValue);
    /* s (sBit)  */
    SMEM_U32_SET_FIELD(mplsEncap,8,1,sBit);
    /* EXP */
    fieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
        descrPtr,
        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_MPLS_PW_LABEL_EXP);
    SMEM_U32_SET_FIELD(mplsEncap,9,3,fieldValue);
    /* label */
    fieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
        descrPtr,
        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_MPLS_PW_LABEL);
    SMEM_U32_SET_FIELD(mplsEncap,12,20,fieldValue);

    /* PUSH the TRGePort<Label>, <EXP>, S=1, <TTL> onto the packet
        BEFORE pushing the label(s) from the TS entry */
    SNET_BUILD_BYTES_FROM_WORD_MAC(mplsEncap,tmpBuffer );
    MEM_APPEND(egrBufPtr, tmpBuffer, 4);

    __LOG(("Added PW Label [0x%8.8x] \n",
        mplsEncap));

    return egrBufPtr;
}

/*******************************************************************************
*   lion3BuildMplsFlowLabel
*
* DESCRIPTION:
*         build  label of MPLS - 'Flow label'
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        egrBufPtr    - pointer to the buffer where to put the data
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8 *lion3BuildMplsFlowLabel
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN GT_U8                 *egrBufPtr,
  IN GT_U32                 sBit
)
{
    DECLARE_FUNC_NAME(lion3BuildMplsFlowLabel);

    GT_U8   tmpBuffer[4];
    GT_U32  fieldValue;
    GT_U32  mplsEncap = 0;
    GT_U32  regAddress;

    __LOG(("use sBit[%d]\n",
        sBit));

    /*
        The Flow Label is set as follows:
        Label[19:0] = inDesc<Hash>[11:0].
            If Label[19:0] is in MPLS reserved label range (0-15) set Label[19] to 1
            --> purpose is to ensure that the flow label is never reserved label 0-15
        TTL = global<MPLS Flow Label TTL>
        EXP = 0
    */

    regAddress = SMEM_LION2_HA_GLOBAL_CONF1_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddress, &fieldValue);
    /* TTL */
    SMEM_U32_SET_FIELD(mplsEncap,0,8,
        SMEM_U32_GET_FIELD(fieldValue,1,8));

    /* s (sBit) */
    SMEM_U32_SET_FIELD(mplsEncap,8,1,sBit);

    /* EXP */
    fieldValue = 0;
    SMEM_U32_SET_FIELD(mplsEncap,9,3,fieldValue);
    /* label */
    fieldValue = descrPtr->pktHash < 16 ?
                 descrPtr->pktHash | (1 << 19) :
                 descrPtr->pktHash ;

    SMEM_U32_SET_FIELD(mplsEncap,12,20,fieldValue);

    SNET_BUILD_BYTES_FROM_WORD_MAC(mplsEncap,tmpBuffer );
    MEM_APPEND(egrBufPtr, tmpBuffer, 4);

    __LOG(("Added FLOW Label [0x%8.8x] \n",
        mplsEncap));

    return egrBufPtr;
}
/*******************************************************************************
*   lion3BuildMplsLabel
*
* DESCRIPTION:
*         build one label of MPLs
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        tunnelStartActionPtr - pointer to the entry in the TS table
*        labelIndex - the index of the label (0,1,2)
*        ttl - to be used in the label
*        sBit - the sBit to use
*        pushEliAndElAfterLabel - entropy label indication
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8 *lion3BuildMplsLabel
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN GT_U8                 *egrBufPtr,
  IN GT_U32                *tunnelStartActionPtr,
  IN GT_U32                 labelIndex,
  IN GT_U32                 ttl,
  IN GT_U32                 sBit,
  IN GT_BIT                 pushEliAndElAfterLabel
)
{
    DECLARE_FUNC_NAME(lion3BuildMplsLabel);

    GT_U8         tmpBuffer[4];
    GT_U32 fieldValue;
    GT_U32  mplsEncap;         /* Encapsulated MPLS Header     */
    GT_U32  label;
    GT_U32  exp;
    GT_U32  hash = 0;

    SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_ENT  labelArr[]={
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_LABEL1,
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_LABEL2,
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_LABEL3};
    SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_ENT  expMarkingModeArr[]={
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_EXP1_MARKING_MODE,
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_EXP2_MARKING_MODE,
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_EXP3_MARKING_MODE};
    SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_ENT  expArr[]={
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_EXP1,
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_EXP2,
        SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_EXP3};

    if(labelIndex >= 3)
    {
        /* not supported */
        return egrBufPtr;
    }

    __LOG(("use sBit[%d]\n",
        sBit));

    label = SIP5_TSE_FIELD_GET_MAC(labelArr[labelIndex]);

    fieldValue = SIP5_TSE_FIELD_GET_MAC(expMarkingModeArr[labelIndex]);

    if(fieldValue)

    {

        exp = SIP5_TSE_FIELD_GET_MAC(expArr[labelIndex]);

    }
    else
    {

        smemRegFldGet(devObjPtr,
                      SMEM_LION2_HA_QOS_PROFILE_TO_EXP_TBL_MEM(devObjPtr,descrPtr->qos.qosProfile),
                      0,3,
                      &exp);
    }


    mplsEncap = 0;
    /* TTL */
    SMEM_U32_SET_FIELD(mplsEncap,0,8,ttl);
    /* s (sBit)  */
    if (pushEliAndElAfterLabel == 1)
    {
        SMEM_U32_SET_FIELD(mplsEncap,8,1,0);
    }
    else 
    {
        SMEM_U32_SET_FIELD(mplsEncap,8,1,sBit);
    }
    /* EXP */
    SMEM_U32_SET_FIELD(mplsEncap,9,3,exp);
    /* label */
    SMEM_U32_SET_FIELD(mplsEncap,12,20,label);

    __LOG(("Added Mpls Label [0x%8.8x] \n",
        mplsEncap));

    SNET_BUILD_BYTES_FROM_WORD_MAC(mplsEncap,tmpBuffer );
    MEM_APPEND(egrBufPtr, tmpBuffer, 4);       
    
    
    /* push entropy label */
    if (pushEliAndElAfterLabel == 1)
    {
        __LOG(("pushEliAndElAfterLabel = %d \n",
            pushEliAndElAfterLabel)); 
        
        hash = descrPtr->pktHash;
        
        /* add ELI label */
        mplsEncap = 0;
        /* ELI TTL */
        SMEM_U32_SET_FIELD(mplsEncap,0,8,ttl);
        /* ELI s (sBit)  */
        SMEM_U32_SET_FIELD(mplsEncap,8,1,0);
        /* ELI EXP */
        SMEM_U32_SET_FIELD(mplsEncap,9,3,exp);
        /* ELI label */
        SMEM_U32_SET_FIELD(mplsEncap,12,20,7);

        __LOG(("Added ELI Label [0x%8.8x] \n",
            mplsEncap));

        SNET_BUILD_BYTES_FROM_WORD_MAC(mplsEncap,tmpBuffer );
        MEM_APPEND(egrBufPtr, tmpBuffer, 4);

        /* add EL label */
        mplsEncap = 0;        
        /* EL TTL */
        SMEM_U32_SET_FIELD(mplsEncap,0,8,0);
        /* EL s (sBit)  */
        SMEM_U32_SET_FIELD(mplsEncap,8,1,sBit);
        /* EL EXP */
        SMEM_U32_SET_FIELD(mplsEncap,9,3,0);
        /* EL label */
        SMEM_U32_SET_FIELD(mplsEncap,12,20,hash);

        __LOG(("Added EL Label [0x%8.8x] \n",
            mplsEncap));

        SNET_BUILD_BYTES_FROM_WORD_MAC(mplsEncap,tmpBuffer );
        MEM_APPEND(egrBufPtr, tmpBuffer, 4);

    }
    return egrBufPtr;
}

/*******************************************************************************
*   lion3HaTunnelStartL3_MPLS
*
* DESCRIPTION:
*        Create buffer for tunnel start - MPLS info
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        tunnelStartActionPtr - pointer to the entry in the TS table
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* lion3HaTunnelStartL3_MPLS
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN HA_INTERNAL_INFO_STC*     haInfoPtr,
  IN GT_U32                *tunnelStartActionPtr
)
{
    DECLARE_FUNC_NAME(lion3HaTunnelStartL3_MPLS);

    GT_U8         tmpBuffer[4];
    GT_U8  *egrBufPtr = descrPtr->tunnelStartRestOfHeaderInfoPtr;/*buffer to put the TS L3 header + ethertype */
    GT_U32 fieldValue;
    GT_U32  ttl;/* The TTL field */
    GT_U32  numOfLabels;/* number of total labels : 1,2,3,4,5 */
    GT_U32  numOfTsLabels;/* number of TS labels : 1,2,3 */
    GT_U32  currentLabelIndex;/* current label index : 0,1,2 */
    GT_U32  swapTtlMode;/*Determines how the TTL is assigned to MPLS labels.
            Note: This field is valid only when TS<TTL>=0.
             0x0 = Incoming TTL; Label<TTL> is taken from the incoming packet, according to the MPLS incoming TTL assignment flow (see "TTL Assignment For Incoming MPLS Label" in the functional spec).;
             0x1 = Popped TTL;     Relevant only when the incoming packet is MPLS, and the MPLS Command = Pop<n>, Swap, or Pop_Swap. Label<TTL> is taken from the popped/swapped MPLS label (if more than one label is popped/swapped - refers to the inner label from the popped/swapped labels).;
             0x2 = Dec Popped TTL; Relevant only when the incoming packet is MPLS, and the MPLS Command = Pop<n>, Swap, or Pop_Swap. Label<TTL> is taken from the popped/swapped MPLS label (if more than one label is popped/swapped - refers to the inner label from the popped/swapped labels), and then decremented by 1.;
             0x3 = Popped Outer TTL; Relevant when the MPLS Command = Pop2 or Pop_Swap. The TTL is taken from the Outer label of the incoming packet.;
     */
    GT_BIT  Pop_or_Swap;
    GT_U32  startBit;
    GT_U32  regAddress;     /* register address             */
    GT_BIT  mplsPwLabel;
    GT_BIT  mplsFlowLabel;
    GT_BIT  sBit;
    GT_BIT  autoSetSBitEnable_global;/* global setting - sBit auto set */
    GT_BIT  setSBit;/* TS setting - sBit set enable */
    GT_BIT  forceSBitZero;/* to we force S bit zero to all labels */
    GT_BOOL  decTtl;/*indication to decrement the used TTL*/
    GT_BIT  pushEliAndElAfterLabel[3]; /*indication whether to push Entropy Label Indicator (ELI) and Entropy Label (EL) after label1, label2, label3*/

    ttl = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_TTL);

    if(ttl == 0)
    {
        swapTtlMode = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_SWAP_TTL_MODE);

        /*Pop<n>, Swap, or Pop_Swap*/
        Pop_or_Swap = descrPtr->mplsCommand != SKERNEL_XCAT_TTI_MPLS_NOP_E ? 1 : 0;

        if(Pop_or_Swap)
        {
            switch(swapTtlMode)
            {
                case 0:
                    ttl = descrPtr->ttl;
                    decTtl = descrPtr->decTtl;
                    break;
                case 1:
                    /* was set by the TTI code to inner label */
                    ttl = descrPtr->origTunnelTtl;
                    decTtl = GT_FALSE;/* force no decrement (regardless to descrPtr->decTtl) */
                    break;
                case 2:
                    /* was set by the TTI code to inner label */
                    ttl = descrPtr->origTunnelTtl;
                    decTtl = GT_TRUE;/* force decrement (regardless to descrPtr->decTtl)  */
                    break;
                case 3:
                default:
                    ttl = descrPtr->ttl1;  /*the Outer label of the incoming packet*/
                    decTtl = GT_TRUE;/* force decrement (regardless to descrPtr->decTtl)  */
                    break;
            }
        }
        else
        {
            if(swapTtlMode != 0)
            {
                __LOG(("Configuration ERROR: swapTtlMode[%d] is for LSR operations only  \n"));
                decTtl = GT_FALSE;
            }
            else
            {
                decTtl = descrPtr->decTtl;
            }
            ttl = descrPtr->ttl;
        }

        if ((decTtl == GT_TRUE) && (ttl != 0))
        {
            ttl--;
        }
    }

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_NUMBER_OF_MPLS_LABELS);
    numOfTsLabels = fieldValue + 1;

    if(numOfTsLabels == 4)
    {
        __LOG((" -- ERROR -- not support TS of 4 labels \n"));
        numOfTsLabels = 3;
    }

    numOfLabels = numOfTsLabels;

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_MPLS_MC_UPSTREAM_ASSIGNED_LABEL);
    startBit = fieldValue * 16;

    smemRegFldGet(devObjPtr,
        SMEM_XCAT_HA_MPLS_ETHERTYPES_REG(devObjPtr),
            startBit, 16, & fieldValue);
    tmpBuffer[0] = (GT_U8)(fieldValue >> 8);
    tmpBuffer[1] = (GT_U8)(fieldValue);
    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    __LOG(("Added EtherType [0x%4.4x] \n",
        fieldValue
        ));

    /*MPLS label push enable*/
    fieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
        descrPtr,
        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_MPLS_PW_LABEL_PUSH_ENABLE);
    mplsPwLabel = fieldValue;
    numOfLabels += mplsPwLabel;/* indication that another label added */

    /*MPLS FLOW label push enable*/
    fieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
        descrPtr,
        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_PUSH_MPLS_FLOW_LABEL_ENABLE);
    mplsFlowLabel = fieldValue;
    numOfLabels += mplsFlowLabel;/* indication that another label added */

    /* set descriptor TS relevant field */
    /* in this function the numOfLabels is 1 based and not 0 based */
    descrPtr->tsEgressMplsNumOfLabels = numOfLabels - 1;

    regAddress = SMEM_LION2_HA_GLOBAL_CONF1_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddress, &fieldValue);
    /* autoSetSBitEnable_global */
    autoSetSBitEnable_global = SMEM_U32_GET_FIELD(fieldValue,0,1);

    forceSBitZero = 0;

    if(autoSetSBitEnable_global)
    {
        if(descrPtr->tunnelTerminated &&
           descrPtr->innerPacketType ==  SKERNEL_INNER_PACKET_TYPE_MPLS)
        {
            forceSBitZero = 1;
        }
    }

    setSBit = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_SET_S_BIT);
    
    if(SMEM_CHT_IS_SIP5_15_GET(devObjPtr))
    {
        /* indication whether to use entropy labels */
        pushEliAndElAfterLabel[0] = SIP5_TSE_FIELD_GET_MAC(SMEM_SIP5_15_HA_TUNNEL_START_TABLE_FIELDS_PUSH_ELI_AND_EL_AFTER_LABEL_1);
        pushEliAndElAfterLabel[1] = SIP5_TSE_FIELD_GET_MAC(SMEM_SIP5_15_HA_TUNNEL_START_TABLE_FIELDS_PUSH_ELI_AND_EL_AFTER_LABEL_2);
        pushEliAndElAfterLabel[2] = SIP5_TSE_FIELD_GET_MAC(SMEM_SIP5_15_HA_TUNNEL_START_TABLE_FIELDS_PUSH_ELI_AND_EL_AFTER_LABEL_3);
    }
    else
    {
      pushEliAndElAfterLabel[0] = pushEliAndElAfterLabel[1] = pushEliAndElAfterLabel[2] = 0;
    }
    
    for(currentLabelIndex = numOfTsLabels ; currentLabelIndex > 0 ; currentLabelIndex--)
    {
        if(forceSBitZero)
        {
            /* indication that even that we are 'most inner' there are additional labels that we are not aware of */
            sBit = 0;
        }
        else
        if( (currentLabelIndex == 1) && (numOfTsLabels == numOfLabels))/* most inner label */
        {
            /* for auto set most inner label to sBit = 1.*/
            /* for no auto set sBit = 0 on all labels */
            sBit = (autoSetSBitEnable_global || setSBit) ? 1 : 0;
        }
        else
        {
            /* not bottom of stack */
            sBit = 0;
        }

        egrBufPtr = lion3BuildMplsLabel(devObjPtr,descrPtr,
            egrBufPtr,tunnelStartActionPtr,currentLabelIndex-1,
            ttl,sBit,pushEliAndElAfterLabel[currentLabelIndex-1]);
    }

    if(mplsPwLabel)
    {
        if(forceSBitZero)
        {
            /* indication that even that we are 'most inner' there are additional labels that we are not aware of */
            sBit = 0;
        }
        else
        if(mplsFlowLabel)
        {
            /* not most inner */
            sBit = 0;
        }
        else
        {
            sBit = 1;
        }

        /* add 'PW label' */
        egrBufPtr = lion3BuildMplsPwLabel(devObjPtr,descrPtr,egrBufPtr,sBit);
    }

    if(mplsFlowLabel)
    {
        if(forceSBitZero)
        {
            /* indication that even that we are 'most inner' there are additional labels that we are not aware of */
            sBit = 0;
        }
        else
        {
            sBit = 1;
        }
        /* add 'Flow label' */
        egrBufPtr = lion3BuildMplsFlowLabel(devObjPtr,descrPtr,egrBufPtr,sBit);
    }

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_PW_CONTROL_INDEX);
    if(fieldValue)
    {
        /* Set HA descriptor fields relevant to MPLS tunnel start */
        descrPtr->tsEgressMplsControlWordExist = 1;

        /*Insert PW Control word (indexed by this field) to the MPLS tunnel*/
        regAddress = SMEM_XCAT_HA_TS_MPLS_CONTROL_WORD_REG(devObjPtr,
                        fieldValue);/* NOT need to do (fieldValue - 1) */

        smemRegGet(devObjPtr, regAddress, &fieldValue);

        __LOG(("Added PW Control word [0x%8.8x] \n",
            fieldValue));

        SNET_BUILD_BYTES_FROM_WORD_MAC(fieldValue,tmpBuffer );
        MEM_APPEND(egrBufPtr, tmpBuffer, 4);
    }

    return egrBufPtr;
}

/*******************************************************************************
*   lion3HaTunnelStartL3_IPv4
*
* DESCRIPTION:
*        Create buffer for tunnel start - IPv4 info (legacy key)
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        egressPort   - the local egress port (not global port)
*        tunnelStartActionPtr - pointer to the entry in the TS table
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* lion3HaTunnelStartL3_IPv4
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN HA_INTERNAL_INFO_STC*     haInfoPtr,
  IN GT_U32                 egressPort,
  IN GT_U32                *tunnelStartActionPtr
)
{
    DECLARE_FUNC_NAME(lion3HaTunnelStartL3_IPv4);

    GT_U8  *egrBufPtr = descrPtr->tunnelStartRestOfHeaderInfoPtr;/*buffer to put the TS L3 header + ethertype */
    GT_U32 fieldValue;
    CH2_TS_INFO_STC         tsInfo;
    GT_BIT greEnable;
    GT_BIT isIpv6Tunnel;
    GT_BIT getDipFromEntry = 1;

    isIpv6Tunnel = ((descrPtr->tunnelStartPassengerType != 0) && (descrPtr->isIp) && (descrPtr->isIPv4 == 0));

    memset(&tsInfo,0,sizeof(tsInfo));

    greEnable = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_IPV4_GRE_ENABLE);

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DSCP_MARKING_MODE);

    if(fieldValue)
    {
        tsInfo.dscp = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DSCP);
        __LOG(("TunnelStart : Assign DSCP [%d] according to the TS entry DSCP \n",
            tsInfo.dscp));
    }
    else
    {
        if(haInfoPtr->tunnelAssigningMode)
        {
            /*Assign DSCP / UP according to the Qos Mapped values*/
            tsInfo.dscp = descrPtr->qosMappedDscp;

            __LOG(("TunnelStart : Assign DSCP [%d] according to the Qos Mapped values \n",
                tsInfo.dscp));
        }
        else
        {
            /*Assign DSCP / UP to the tunnel according to the passenger fields*/
            tsInfo.dscp = descrPtr->dscp;
            __LOG(("TunnelStart : Assign DSCP [%d] according to the passenger fields \n",
                tsInfo.dscp));
        }
    }

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_TTL);

    tsInfo.ttl = fieldValue ?
                 fieldValue :
                 descrPtr->ttl;

    if ((fieldValue == 0) && (descrPtr->decTtl == GT_TRUE) && (descrPtr->ttl != 0))
    {
        tsInfo.ttl--;
    }

    if (greEnable == 0)
    {
        if (descrPtr->isIPv4 == 0)
        {
            tsInfo.protocol = 41;
        }
        else
        {
            tsInfo.protocol = 4;
        }
    }
    else
    {
        tsInfo.protocol = 47;
    }

    tsInfo.doNotFragmentFlag = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DF_FLAG);

    tsInfo.sip = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_SIP);

    if(isIpv6Tunnel)
    {
        fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_AUTO_TUNNEL_FLAG);

        if(fieldValue)
        {
            fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_AUTO_TUNNEL_OFFSET);

            /*The offset, in bytes, within the IPv6 DIP header field from which the IPv4 header DIP is extracted*/
            tsInfo.dip = snetFieldValueGet(descrPtr->dip,(8 *fieldValue),32);

            getDipFromEntry = 0;/* do not use the DIP from the TS entry */
        }
    }

    if(getDipFromEntry)
    {
        tsInfo.dip = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DIP);
    }

    /* build the ipv4 header */
    egrBufPtr = buildIpv4Header(devObjPtr,descrPtr, haInfoPtr,egressPort, &tsInfo,egrBufPtr);

    if(greEnable)
    {
        /* Create and insert GRE encapsulation - 4/8 bytes */
        egrBufPtr = buildGreBytes(devObjPtr,descrPtr,haInfoPtr,egrBufPtr,tunnelStartActionPtr);
    }

    return egrBufPtr;
}


/*******************************************************************************
*   lion3HaTunnelStartL3_MIM
*
* DESCRIPTION:
*        Create buffer for tunnel start - MIM info
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        tunnelStartActionPtr - pointer to the entry in the TS table
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* lion3HaTunnelStartL3_MIM
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN HA_INTERNAL_INFO_STC*     haInfoPtr,
  IN GT_U32                *tunnelStartActionPtr
)
{
    DECLARE_FUNC_NAME(lion3HaTunnelStartL3_MIM);

    GT_U8  *egrBufPtr = descrPtr->tunnelStartRestOfHeaderInfoPtr;/*buffer to put the TS L3 header + ethertype */
    GT_U32 fieldValue;
    CH2_TS_INFO_STC         tsInfo;

    memset(&tsInfo,0,sizeof(tsInfo));

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_UP_MARKING_MODE);

    if(fieldValue)
    {
        tsInfo.iUp = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_UP);
    }
    else
    {
        tsInfo.iUp  = descrPtr->iUp;
    }

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_DEI_MARKING_MODE);

    if(fieldValue)
    {
        tsInfo.iDp = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_DEI);
    }
    else
    {
        tsInfo.iDp  = descrPtr->iDp;
    }

    tsInfo.iTagCda = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_TAG_NCA);
    tsInfo.iTagRes1 = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_TAG_RES1);
    tsInfo.iTagRes2 = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_TAG_RES2);

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_SID_ASSIGN_MODE);
    if(fieldValue)
    {
        /* get from the vlan table */
        fieldValue =
            snetFieldValueGet(descrPtr->eArchExtInfo.haEgressVlanTablePtr,12,24);
        tsInfo.iSid = fieldValue;
    }
    else
    {
        tsInfo.iSid = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_I_SID);
    }

    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_B_DA_ASSIGN_MODE);

    if(fieldValue)
    {
        /*  B_DA[47:24] = TsEntry<B-DA[47:24]
            B_DA[23:0]  = I-SID */

        /* we need to update the 3 LSBytes that already set by
           snetLion3HaTunnelStart(...) */
        descrPtr->tunnelStartMacInfoPtr[3] = (GT_U8)(tsInfo.iSid >> 16);
        descrPtr->tunnelStartMacInfoPtr[4] = (GT_U8)(tsInfo.iSid >>  8);
        descrPtr->tunnelStartMacInfoPtr[5] = (GT_U8)(tsInfo.iSid >>  0);

        __LOG(("update the DA (B-DA) 3 LSBytes to [0x%2.2x%2.2x%2.2x] \n",
            descrPtr->tunnelStartMacInfoPtr[3],
            descrPtr->tunnelStartMacInfoPtr[4],
            descrPtr->tunnelStartMacInfoPtr[5]
            ));
    }

    /* build the MIM header */
    egrBufPtr = buildMimHeader(devObjPtr,descrPtr,&tsInfo,egrBufPtr);

    return egrBufPtr;
}


/*******************************************************************************
*   lion3HaTunnelStartL3_TRILL
*
* DESCRIPTION:
*        Create buffer for tunnel start - TRILL info
* INPUTS:
*        devObjPtr    - pointer to device object.
*        descrPtr     - pointer to the frame's descriptor.
*        haInfoPtr    - ha internal info
*        tunnelStartActionPtr - pointer to the entry in the TS table
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* lion3HaTunnelStartL3_TRILL
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
  IN HA_INTERNAL_INFO_STC*     haInfoPtr,
  IN GT_U32                *tunnelStartActionPtr
)
{
    DECLARE_FUNC_NAME(lion3HaTunnelStartL3_TRILL);

    GT_U8         tmpBuffer[4];
    GT_U32 trillHeaderWords[2];
    GT_U32 tsIndex = descrPtr->tunnelPtr;
    GT_U8  *egrBufPtr = descrPtr->tunnelStartRestOfHeaderInfoPtr;/*buffer to put the TS L3 header + ethertype */
    GT_U32 tmpVal;
    GT_U32 fieldValue;
    GT_U32 trillEtherType;

    descrPtr->tunnelStartTrillTransit = 0;

    SMEM_LION3_HA_TUNNEL_START_ENTRY_FIELD_TRILL_HEADER_GET(devObjPtr,
        tunnelStartActionPtr,
        tsIndex,
        trillHeaderWords);

    /*TRILL Interface Enable*/
    fieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
            descrPtr,
            SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_TRILL_INTERFACE_ENABLE);

    if(fieldValue == 0)
    {
        __LOG(("-- ERROR --- TRILL Interface NOT Enabled \n"));
        return egrBufPtr;
    }

    __LOG(("TRILL Interface Enabled \n"));
    smemRegGet(devObjPtr, SMEM_LION2_HA_TRILL_ETHERTYPE_CONFIG_REG(devObjPtr),&fieldValue);
    /*TRILL EtherType*/
    trillEtherType = SMEM_U32_GET_FIELD(fieldValue,0,16);
    if(descrPtr->origInfoBeforeTunnelTermination.origEtherType == trillEtherType)
    {
        __LOG(("TRILL 'transit' packet \n"));
        /*
            Trigger DoRouteHA processing:
            strip L2 header
            Overwrite TRILL header <Hop Count> with desc<TTL>
            Use TS L2 fields to build new L2 header (ignore TS <TRILL header>)
        */
        descrPtr->tunnelStartTrillTransit = 1;

        /* this is TRILL packet that ingress and now need to Egress , with
           'Outer frame L2' modifications (and hopCount update) */

        /* the simulation code in HA expect the TS to cover the 'after vlan tag' including 'ethertype' */
        MEM_APPEND(egrBufPtr,
            descrPtr->afterVlanOrDsaTagPtr,
            descrPtr->afterVlanOrDsaTagLen);
        /* no more to do ... let the HA build the rest of the packet ... */
        /*we only took MAC SA,DA from the TS into the buffer and other L2
        into the descriptor */

    }
    else
    {
        /*Use TS for Ether-over-TRILL TS encapsulation*/
        __LOG(("Ethernet packet encapsulation onto TRILL packet \n"));
        descrPtr->tunnelStartTrillTransit = 0;

        /* EtherType of TRILL */
        tmpVal = trillEtherType;
        tmpBuffer[0] = (GT_U8)(tmpVal >> 8);
        tmpBuffer[1] = (GT_U8)(tmpVal >> 0);
        MEM_APPEND(egrBufPtr, tmpBuffer, 2);

        /* TRILL header */
        tmpVal = trillHeaderWords[1];
        tmpBuffer[0] = (GT_U8)(tmpVal >> 8);
        tmpBuffer[1] = (GT_U8)(tmpVal >> 0);
        MEM_APPEND(egrBufPtr, tmpBuffer, 2);

        tmpVal = trillHeaderWords[0];
        tmpBuffer[0] = (GT_U8)(tmpVal >> 24);
        tmpBuffer[1] = (GT_U8)(tmpVal >> 16);
        tmpBuffer[2] = (GT_U8)(tmpVal >>  8);
        tmpBuffer[3] = (GT_U8)(tmpVal >>  0);
        MEM_APPEND(egrBufPtr, tmpBuffer, 4);
    }

    return egrBufPtr;
}

/*******************************************************************************
*   snetLion3AppendIpProfile
*
* DESCRIPTION:
*        Appends buffer for IP profile to IP header.
* INPUTS:
*        devObjPtr       - pointer to device object
*        descrPtr        - pointer to the frame's descriptor
*        tsInfoPtr       - ts internal info
*        egrBufPtr       - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* snetLion3AppendIpProfile
(
  IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
  IN CH2_TS_INFO_STC                 *tsInfoPtr,
  IN GT_U8                           *egrBufPtr
)
{
    DECLARE_FUNC_NAME(snetLion3AppendIpProfile);

    GT_U32   byteValue;
    GT_U32   byteNum;
    GT_U32   bitValue = 0;
    GT_U32   bitNum;
    GT_U32   tmpShift;
    GT_U32   tmpFieldValue;
    GT_U32   fieldValue;        /* template byte bit field value */
    GT_U32   shiftedPacketHash; /* 16 bits packet shifted hash */

    GT_U32   ipTemplateSize = SIP5_TSE_PROFILE_FIELD_GET_MAC(SMEM_LION3_HA_GENERIC_TS_PROFILE_TABLE_FIELDS_TEMPLATE_SIZE);
    GT_U32   hashShiftLeft  = SIP5_TSE_PROFILE_FIELD_GET_MAC(SMEM_LION3_HA_GENERIC_TS_PROFILE_TABLE_FIELDS_HASH_CIRCULAR_SHIFT_LEFT);

    /* check ip template size */
    if (0 == ipTemplateSize)
    {
        __LOG(("snetLion3AppendIpProfile: no profile bytes added \n"));
        return egrBufPtr; /* nothing to add, return the same ptr */
    }

    if (ipTemplateSize > 4)
    {
        skernelFatalError("Got wrong ip template size %x\n", ipTemplateSize);
    }

    /* convert ip templates size to number of bytes in the template */
    ipTemplateSize *= 4; /* value is in 4 bytes resolution */

    /* add udp length */
    tsInfoPtr->packetTotalLen += ipTemplateSize;

    /* calculate shifted packet hash (16 bits) */
    shiftedPacketHash = (descrPtr->pktHash << hashShiftLeft | descrPtr->pktHash >> (16-hashShiftLeft));

    /* get profile and append it to the buffer */
    for(byteNum = 0; byteNum < ipTemplateSize; byteNum++)
    {
        byteValue = 0;

        /* get byte value */
        for(bitNum = 0; bitNum < BITS_IN_BYTE; bitNum++)
        {
            /* get field value - template byte bit value*/
            fieldValue = SMEM_LION3_HA_GENERIC_TS_PROFILE_TEMPLATE_FIELD_GET(devObjPtr,
                              tsInfoPtr->profileEntryPtr, tsInfoPtr->profileNum, byteNum, bitNum);

            if(fieldValue > 0xA)
            {
                skernelFatalError("Got wrong template byte bit value %x\n", fieldValue);
            }

            /* get bit value */
            switch(fieldValue)
            {
                case 0:
                case 1:
                    bitValue = fieldValue;
                    break;

                case 2:
                case 3:
                case 4:
                case 5:
                    /* total num of bits to shift */
                    tmpShift = (fieldValue - 2/*start position in switch*/) * BITS_IN_BYTE + bitNum;

                    /* ePort <TS Extension> */
                    tmpFieldValue = SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                                        descrPtr,
                                        SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_TS_EXT);

                    bitValue = (tmpFieldValue >> tmpShift) & 1;
                    break;

                case 6:
                case 7:
                case 8:
                    /* total num of bits to shift */
                    tmpShift = (fieldValue - 6/*start position in switch*/) * BITS_IN_BYTE + bitNum;

                    bitValue = (tsInfoPtr->eVlanServiceId >> tmpShift) & 1;
                    break;

                case 9:
                case 10:
                    /* total num of bits to shift */
                    tmpShift = (fieldValue - 9/*start position in switch*/) * BITS_IN_BYTE + bitNum;

                    bitValue = (shiftedPacketHash >> tmpShift) & 1;
                    break;
                default:
                    skernelFatalError("Got wrong template byte bit value: %x\n", fieldValue);
                    break;
            }

            /* set bit value to the byte */
            byteValue |= bitValue << bitNum;
        }

        /* append byte to the buffer */
        MEM_APPEND(egrBufPtr, &byteValue, 1);
    }

    return egrBufPtr;
}



/*******************************************************************************
*   lion3BuildIpv6Header
*
* DESCRIPTION:
*        Build TS ipv6 header
*
* INPUTS:
*        devObjPtr            - pointer to device object.
*        descrPtr             - pointer to the frame's descriptor.
*        haInfoPtr            - ha internal info
*        egressPort           - the local egress port (not global port)
*        tunnelStartActionPtr - pointer to the entry in the TS table
*        tsInfoPtr            - ts internal info
*        egrBufPtr            - pointer to the buffer where to put the tunnel start
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* lion3BuildIpv6Header
(
  IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
  IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
  IN    HA_INTERNAL_INFO_STC            *haInfoPtr,
  IN    GT_U32                           egressPort,
  IN    GT_U32                          *tunnelStartActionPtr,
  IN    CH2_TS_INFO_STC                 *tsInfoPtr,
  IN    GT_U8                           *egrBufPtr
)
{
    DECLARE_FUNC_NAME(lion3BuildIpv6Header);

    GT_U32  i;
    GT_U32  dipMode;
    GT_U32  tmpWords[4] = {0}; /* used for getting sip/dip */
    GT_U8   tmpBuffer[BYTES_IN_WORD] = {0};
    GT_U32  flowLabelMode;
    GT_U32  tmpWord;

    /* start with the ether type */
    tmpBuffer[0] = 0x86;
    tmpBuffer[1] = 0xDD;
    MEM_APPEND(egrBufPtr, tmpBuffer, 2);

    __LOG(("added EtherType [0x%2.2x%2.2x] \n", tmpBuffer[0], tmpBuffer[1]));

    /* save pointer to the start of the ipv6 */
    haInfoPtr->tsIpv6HeaderPtr = egrBufPtr;

    /* flow label (based on flow label mode) */
    flowLabelMode = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_FLOW_LABEL_MODE);
    tmpWord = flowLabelMode ? descrPtr->pktHash : 0;

    /* version (always 6) */
    tmpWord |= (VERSION_IPV6 << 28);

    /* traffic class (dscp)  */
    tmpWord |= (tsInfoPtr->dscp << 20);

    SNET_BUILD_BYTES_FROM_WORD_MAC(tmpWord, tmpBuffer);
    MEM_APPEND(egrBufPtr, tmpBuffer, 4);

    /* payload length - 16 bits */
    egrBufPtr = calculateAndAppendPacketTotalLength(devObjPtr, descrPtr, haInfoPtr,
                                                    egressPort, tsInfoPtr, egrBufPtr);

    /* next header (protocol) - 8 bits*/
    tmpBuffer[0] = (GT_U8)tsInfoPtr->protocol;
    MEM_APPEND(egrBufPtr, tmpBuffer, 1);

    /* hop limit (ttl) - 8 bits */
    tmpBuffer[0] = (GT_U8)tsInfoPtr->ttl;
    MEM_APPEND(egrBufPtr, tmpBuffer, 1);

    /* ipv6 sip */
    SMEM_LION3_HA_TUNNEL_START_ENTRY_FIELD_SIP_IPV6_GET(devObjPtr,
                tunnelStartActionPtr, descrPtr->tunnelPtr, tmpWords);
    for(i = 16; i != 0 ; i--)
    {
        tmpBuffer[0] = (GT_U8)snetFieldValueGet(tmpWords, (8 * (i-1)), 8);
        MEM_APPEND(egrBufPtr, tmpBuffer, 1);
    }

    /* ipv6 dip */
    SMEM_LION3_HA_TUNNEL_START_ENTRY_FIELD_DIP_IPV6_GET(devObjPtr,
                tunnelStartActionPtr, descrPtr->tunnelPtr, tmpWords);

    /* dip mode: x ls bits are taken from the Shifted eVLAN <Service-ID> attribute */
    dipMode = SIP5_TSE_PROFILE_FIELD_GET_MAC(SMEM_LION3_HA_GENERIC_TS_PROFILE_TABLE_FIELDS_PROFILE_DIP_MODE);

    if(dipMode)
    {
        /* apply DIP mode */
        tmpWords[0] &= snetFieldValueGet(&tsInfoPtr->eVlanServiceIdShifted, 0, dipMode);
    }

    for(i = 16; i != 0 ; i--)
    {
        tmpBuffer[0] = (GT_U8)snetFieldValueGet(tmpWords, (8 * (i-1)), 8);

        MEM_APPEND(egrBufPtr, tmpBuffer, 1);
    }

    /* return the updated pointer */
    return egrBufPtr;
}

/*******************************************************************************
*   lion3HaTunnelStartL3_GenericIP
*
* DESCRIPTION:
*        Creates buffer for tunnel start - generic IPv4 info (legacy key)
*
* INPUTS:
*        devObjPtr            - pointer to device object.
*        descrPtr             - pointer to the frame's descriptor.
*        haInfoPtr            - ha internal info
*        egressPort           - the local egress port (not global port)
*        tunnelStartActionPtr - pointer to the entry in the TS table
*
* OUTPUTS:
*
* RETURNS: GT_U8* - pointer to the end of the tunnel start
*
* COMMENTS:
*
*******************************************************************************/
static GT_U8* lion3HaTunnelStartL3_GenericIP
(
  IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
  IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
  IN HA_INTERNAL_INFO_STC            *haInfoPtr,
  IN GT_U32                           egressPort,
  IN GT_U32                          *tunnelStartActionPtr
)
{
    DECLARE_FUNC_NAME(lion3HaTunnelStartL3_GenericIP);

    GT_U8           *egrBufPtr                = descrPtr->tunnelStartRestOfHeaderInfoPtr;/*buffer to put the TS L3 header + ethertype */
    GT_U8            tmpBuffer[BYTES_IN_WORD] = {0};
    GT_U32           shiftValue;
    GT_U8           *udpLengthPtr = NULL;
    CH2_TS_INFO_STC  tsInfo                   = {0};
    CH2_TS_INFO_STC *tsInfoPtr                = &tsInfo; /* used for SIP5_TSE_PROFILE_FIELD_GET_MAC */
    GT_U32  fieldValue;

    /* save ip profile entry */
    tsInfo.profileNum      = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_PROFILE_NUMBER);
    tsInfo.profileEntryPtr = smemMemGet(devObjPtr,
                        SMEM_LION3_HA_TUNNEL_START_GENERIC_IP_PROFILE_TBL_MEM(devObjPtr, tsInfo.profileNum));


    /* eVlan <Service ID> */
    tsInfo.eVlanServiceId = snetFieldValueGet(descrPtr->eArchExtInfo.haEgressVlanTablePtr,12,24);

    /* The number of bits to circular shift left the eVLAN attribute <Service-ID> value. */
    shiftValue = SIP5_TSE_PROFILE_FIELD_GET_MAC(SMEM_LION3_HA_GENERIC_TS_PROFILE_TABLE_FIELDS_SERVICE_ID_CIRCULAR_SHIFT_SIZE);

    /* The result is the "Shifted Service-ID" value. Range: 0 - 23 Values 24 -31 are reserved */
    tsInfo.eVlanServiceIdShifted = (tsInfo.eVlanServiceId << shiftValue) |
                                   (tsInfo.eVlanServiceId >> (32/*bits in word*/ - shiftValue));


    fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DSCP_MARKING_MODE);
    /* get dscp value, also named as traffic class in ipv6 */
    if(fieldValue)
    {
        tsInfo.dscp = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DSCP);
        __LOG(("TunnelStart : Assign DSCP [%d] according to the TS entry DSCP \n",
            tsInfo.dscp));
    }
    else
    {
        if(haInfoPtr->tunnelAssigningMode)
        {
            /*Assign DSCP / UP according to the Qos Mapped values*/
            tsInfo.dscp = descrPtr->qosMappedDscp;

            __LOG(("TunnelStart : Assign DSCP [%d] according to the Qos Mapped values \n",
                tsInfo.dscp));
        }
        else
        {
            /*Assign DSCP / UP to the tunnel according to the passenger fields*/
            tsInfo.dscp = descrPtr->dscp;
            __LOG(("TunnelStart : Assign DSCP [%d] according to the passenger fields \n",
                tsInfo.dscp));
        }
    }

    {/* ttl */
        GT_U32 ttl = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_TTL);
        tsInfo.ttl = ttl ? ttl : descrPtr->ttl;
        if ((ttl == 0) && (descrPtr->decTtl == GT_TRUE) && (descrPtr->ttl != 0))
        {
            tsInfo.ttl--;
        }
    }

    switch(SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_PROTOCOL))
    {
        case 0: /* ip */
             tsInfo.protocol = descrPtr->isIp == 0 ? 0 :
                        (descrPtr->isIPv4 ? 4 : 41) ;
            break;
        case 1: /* gre */
            tsInfo.protocol = 47;
            tsInfo.gre = 1;
            break;
        case 2: /* udp */
            tsInfo.protocol = 17;
            tsInfo.udp = 1;
            break;
        case 3: /* hp gre - internal only, not supported */
        default:
            skernelFatalError("Not supported ts protocol\n");
            break;
    }

    /******************************************************************/
    if(SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV6_E != descrPtr->tunnelStartType)
    { /* ipv4 code, not relevant for ipv6 */

        /* prepare ipv4 specific fields */
        tsInfo.doNotFragmentFlag = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DF_FLAG);

        tsInfo.sip = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_SIP);

        if( (descrPtr->tunnelStartPassengerType && descrPtr->isIp && !descrPtr->isIPv4) &&    /* is ipv6 tunnel */
            SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_AUTO_TUNNEL_FLAG)) /* and auto tunnel flag is set */
        {
            /*The offset, in bytes, within the IPv6 DIP header field from which the IPv4 header DIP is extracted*/
            tsInfo.dip = snetFieldValueGet(descrPtr->dip,
                       (8*SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_AUTO_TUNNEL_OFFSET)), 32);
        }
        else
        { /* use the DIP from the TS entry */
            tsInfo.dip = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_DIP);
        }

        /* build the ipv4 header */
        egrBufPtr = buildIpv4Header(devObjPtr, descrPtr, haInfoPtr, egressPort, &tsInfo, egrBufPtr);
    }
    else
    { /* ipv6 */
        egrBufPtr = lion3BuildIpv6Header(devObjPtr, descrPtr, haInfoPtr, egressPort,
                                         tunnelStartActionPtr, &tsInfo, egrBufPtr);
    }


    /******************************************************************/
    if(tsInfo.gre)
    {

        /* Create and insert GRE encapsulation - 4/8 bytes */
        egrBufPtr = buildGreBytes(devObjPtr,descrPtr,haInfoPtr,egrBufPtr,tunnelStartActionPtr);

        /* gre header lentgh (4 bytes) already handled inside build ip header functions,
           no need to update */
    }

    /******************************************************************/
    if(tsInfo.udp)
    {
        /* add udp header */
        GT_U32 tmpWord    = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_UDP_DST_PORT);

        GT_U32 udpSrcMode = SIP5_TSE_PROFILE_FIELD_GET_MAC(SMEM_LION3_HA_GENERIC_TS_PROFILE_TABLE_FIELDS_UDP_SOURCE_PORT_MODE);

        GT_U32 udpSrcPort = udpSrcMode ? descrPtr->pktHash :
                                  SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_UDP_SRC_PORT);
        tmpWord |= udpSrcPort << 16;

        /* save pointer to the start of the UDP */
        haInfoPtr->tsUdpHeaderPtr = egrBufPtr;

        SNET_BUILD_BYTES_FROM_WORD_MAC(tmpWord, tmpBuffer);
        MEM_APPEND(egrBufPtr, tmpBuffer, BYTES_IN_WORD);

        /* udp length, will be set after ip profiles */
        /* save pointer to udp length field */
        udpLengthPtr = egrBufPtr;

        /* udp length set to 0 and udp checksum is always 0 in sip5 */
        SNET_BUILD_BYTES_FROM_WORD_MAC(0, tmpBuffer);
        MEM_APPEND(egrBufPtr, tmpBuffer, BYTES_IN_WORD);

        /* add udp len */
        tsInfoPtr->packetTotalLen += UDP_HEADER_LENGHT;
    }

    /******************************************************************/
    if(tsInfo.gre || tsInfo.udp)
    {
        /* add ip profile template data */
        egrBufPtr = snetLion3AppendIpProfile(devObjPtr, descrPtr, &tsInfo, egrBufPtr);
    }

    /******************************************************************/
    /* update packet total length (ipv4) or payload length (ipv6) */
    tsInfoPtr->ipTotalLenFieldInEgressBufPtr[0] = tsInfoPtr->packetTotalLen >> 8;
    tsInfoPtr->ipTotalLenFieldInEgressBufPtr[1] = tsInfoPtr->packetTotalLen;

    __LOG(("updated total length [0x%2.2x%2.2x] \n",
                tsInfoPtr->ipTotalLenFieldInEgressBufPtr[0],
                tsInfoPtr->ipTotalLenFieldInEgressBufPtr[1]));

    /* update udp length */
    if(tsInfo.udp)
    {
        GT_U32 udpLength = tsInfoPtr->packetTotalLen;

        if(SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV6_E != descrPtr->tunnelStartType)
        { /* ipv4 code, not relevant for ipv6 */
            udpLength -= IPV4_HEADER_LENGHT*4;
        }

        udpLengthPtr[0] = udpLength >> 8;
        udpLengthPtr[1] = udpLength;
    }

    return egrBufPtr;
}


/*******************************************************************************
*   snetLion3HaTunnelStart
*
* DESCRIPTION:
*        tunnel start for SIP5
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       haInfoPtr    - ha internal info
*       egressPort   - the local egress port (not global port)
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*       haInfoPtr    - ha internal info
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLion3HaTunnelStart
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT HA_INTERNAL_INFO_STC*     haInfoPtr,
    IN GT_U32   egressPort
)
{
    DECLARE_FUNC_NAME(snetLion3HaTunnelStart);

    GT_U32  regAddr;                /* field for register address             */
    GT_U32 *tunnelStartActionPtr;      /* pointer to the entry in the TS table */
    GT_U8  *origEgressPtr;
    GT_U8  *nextBufferPtr;
    GT_U32 fieldValue;
    GT_U32 tsIndex = descrPtr->tunnelPtr;
    GT_U32  macAddrWords[2];

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        /* fix for sip5_10 : HA-3340 : "Incorrect assignment of tunnel's UP" */
        regAddr = SMEM_LION2_HA_GLOBAL_CONF1_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr,12,1,&haInfoPtr->tunnelAssigningMode);
    }
    else
    {
        /*Assign DSCP / UP to the tunnel according to the passenger fields*/
        haInfoPtr->tunnelAssigningMode = 0;
    }

    __LOG_PARAM(haInfoPtr->tunnelAssigningMode);

    /* start with the Ethernet header */
    origEgressPtr = devObjPtr->egressBuffer1;

    descrPtr->tunnelStartMacInfoPtr = origEgressPtr;
    descrPtr->tunnelStartMacInfoLen = 12;/* mac sa,da */

    nextBufferPtr = origEgressPtr;

    /* pointer to the 'memory line' of the TS entry */
    tunnelStartActionPtr = smemMemGet(devObjPtr,
        SMEM_CHT2_TUNNEL_START_TBL_MEM(devObjPtr,tsIndex));
    if(tsIndex & 1)
    {
        /* the odd entries are in the second half of the entry (offset of 192 bits = 6 words) */
        tunnelStartActionPtr += 6;
    }

    descrPtr->tunnelStartType = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_TUNNEL_TYPE);

    haInfoPtr->tsVlanTagInfo.vlanTagged = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_TAG_ENABLE);

    if(haInfoPtr->tsVlanTagInfo.vlanTagged)
    {
        fieldValue = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_UP_MARKING_MODE);

        if(fieldValue)
        {
            haInfoPtr->tsVlanTagInfo.up = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_UP);
            __LOG(("TunnelStart : Assign UP [%d] according to the TS entry UP \n",
                haInfoPtr->tsVlanTagInfo.up));
        }
        else
        {
            if(haInfoPtr->tunnelAssigningMode)
            {
                /*Assign DSCP / UP according to the Qos Mapped values*/
                haInfoPtr->tsVlanTagInfo.up = descrPtr->qosMappedUp;
                __LOG(("TunnelStart : Assign UP [%d] according to the Qos Mapped values \n",
                    haInfoPtr->tsVlanTagInfo.up));
            }
            else
            {
                /*Assign DSCP / UP to the tunnel according to the passenger fields*/
                haInfoPtr->tsVlanTagInfo.up = descrPtr->up;
                __LOG(("TunnelStart : Assign UP [%d] according to the passenger fields \n",
                    haInfoPtr->tsVlanTagInfo.up));
            }
        }

        haInfoPtr->tsVlanTagInfo.cfi = descrPtr->cfidei;/*Reserved (was CFI - this is obsolete - CFI taken from descriptor)*/

        haInfoPtr->tsVlanTagInfo.vlanId = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_VID);
    }

    /* get the mac DA */
    SMEM_LION3_HA_TUNNEL_START_ENTRY_FIELD_NEXT_HOP_MAC_ADDR_GET(devObjPtr,
            tunnelStartActionPtr,
            tsIndex,
            macAddrWords);

    nextBufferPtr[0] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 5) ,8);
    nextBufferPtr[1] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 4) ,8);
    nextBufferPtr[2] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 3) ,8);
    nextBufferPtr[3] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 2) ,8);
    nextBufferPtr[4] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 1) ,8);
    nextBufferPtr[5] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 0) ,8);

    __LOG(("Set DA 6 Bytes to [0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x] \n",
        nextBufferPtr[0],
        nextBufferPtr[1],
        nextBufferPtr[2],
        nextBufferPtr[3],
        nextBufferPtr[4],
        nextBufferPtr[5]
        ));

    /* calculate the mac SA */
    snetChtHaMacFromMeBuild(devObjPtr,descrPtr,egressPort,GT_TRUE,&nextBufferPtr[6]);

    __LOG(("Set SA 6 Bytes to [0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x] \n",
        nextBufferPtr[6],
        nextBufferPtr[7],
        nextBufferPtr[8],
        nextBufferPtr[9],
        nextBufferPtr[10],
        nextBufferPtr[11]
        ));

    __LOG(("end of build of L2 for the tunnel start (without vlan tag) \n"));

    /* set point to the end of Mac info */
    descrPtr->tunnelStartRestOfHeaderInfoPtr = descrPtr->tunnelStartMacInfoPtr + descrPtr->tunnelStartMacInfoLen;

    if(haInfoPtr->tunnelStartPassengerType != 0)
    {
        haInfoPtr->retainCrc = 0;
    }
    else
    {
        haInfoPtr->retainCrc = SIP5_TSE_FIELD_GET_MAC(SMEM_LION3_HA_TUNNEL_START_TABLE_FIELDS_RETAIN_INNER_CRC);
    }

    switch(descrPtr->tunnelStartType)
    {
        case SKERNEL_FRAME_TUNNEL_START_TYPE_MPLS_E    :
            nextBufferPtr = lion3HaTunnelStartL3_MPLS(devObjPtr,
                        descrPtr,
                        haInfoPtr,
                        tunnelStartActionPtr);
            break;
        case SKERNEL_FRAME_TUNNEL_START_TYPE_IPV4_E    : /* to delete when generic ipv4 cpss support will be done */
            nextBufferPtr = lion3HaTunnelStartL3_IPv4(devObjPtr,
                        descrPtr,
                        haInfoPtr,
                        egressPort,
                        tunnelStartActionPtr);
            break;
        case SKERNEL_FRAME_TUNNEL_START_TYPE_MIM_E     :
            nextBufferPtr = lion3HaTunnelStartL3_MIM(devObjPtr,
                        descrPtr,
                        haInfoPtr,
                        tunnelStartActionPtr);
            break;
        case SKERNEL_FRAME_TUNNEL_START_TYPE_TRILL_E   :
            nextBufferPtr = lion3HaTunnelStartL3_TRILL(devObjPtr,
                        descrPtr,
                        haInfoPtr,
                        tunnelStartActionPtr);
            break;
        case SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV4_E:
        case SKERNEL_FRAME_TUNNEL_START_TYPE_GENERIC_IPV6_E:
            /* same call for both ipv4 and ipv6, differences resolved inside func */
            nextBufferPtr = lion3HaTunnelStartL3_GenericIP(devObjPtr,
                        descrPtr,
                        haInfoPtr,
                        egressPort,
                        tunnelStartActionPtr);
            break;
        default:
            skernelFatalError("Not supported tunnel start type: %d\n", descrPtr->tunnelStartType);
            break;
    }

    /* set the length of the L3 info -- this include the EtherType !! */
    descrPtr->tunnelStartRestOfHeaderInfoLen = nextBufferPtr - descrPtr->tunnelStartRestOfHeaderInfoPtr;


    __LOG(("end of build of L3 for the tunnel start \n"));
}

/*******************************************************************************
*   snetCht2HaTunnelStart
*
* DESCRIPTION:
*        T.S Engine processing for outgoing frame on Cheetah2
*        asic simulation.
*        T.S processing , T.S assignment
*        actions to descriptor processing
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       haInfoPtr    - ha internal info
*       egressPort   - the local egress port (not global port)
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*       haInfoPtr    - ha internal info
*
* RETURN:
*
*       T.S has only one lookup cycle.
*       D.17.2.1: T.S Registers
*******************************************************************************/
GT_VOID snetCht2HaTunnelStart
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT HA_INTERNAL_INFO_STC*     haInfoPtr,
    IN GT_U32   egressPort
)
{
    DECLARE_FUNC_NAME(snetCht2HaTunnelStart);


    GT_U32 *tunnelStartAction;      /* Address of next hop entry             */
    GT_U32  regAddr;                /* field for register address             */
    GT_U32  tunnelType;             /* identify the tunnel type 0 IPv4 1 MPLS */
    GT_U32  tagEnable;              /* if set packet with vlan                */
    GT_U32  vId;                    /* vid in Tunnel Start                    */
    GT_U32  up_mode;                /* up marking mode                        */
    GT_U32  up;                     /* up value                               */
    GT_U32  dscp_mode;              /* dscp marking mode                      */
    GT_U32  dscp;                   /* dscp value                             */
    /*GT_U32  dfflag; */   /* not used warning */  /* don't fragment flag                    */
    GT_U32  gRE = 0;                /* if set tunnel header is GRE-IPv4       */
    GT_U32  ttl;                    /* The tunnel header TTL field            */
    GT_U32  cfi;                    /* CFI assigned to Tunnel Start packets   */
    GT_U32  autoflag;               /* for IPv6 set ipv4 derived from packet  */
    GT_U32  autoOffset;             /* for IPv6 set flag offset calculate DIP */
    GT_U32  label1;                 /* MPLS label1                            */
    GT_U32  numOfLabels;            /* num of MPLS labels                     */
    GT_U32  exp1;                   /* MPLS mode set set EXP to QoS           */
    GT_U32  exp_mode1;              /* MPLS mode                              */
    GT_U32  label2;                 /* MPLS label2                            */
    GT_U32  exp2;                   /* MPLS mode set set EXP to QoS           */
    GT_U32  exp_mode2;              /* MPLS mode                              */
    GT_U32  label3;                 /* MPLS label3                            */
    GT_U32  exp3;                   /* MPLS mode set set EXP to QoS           */
    GT_U32  exp_mode3;              /* MPLS mode                              */
    GT_U32 ipv6_v4_offset;          /* pointer to ARP table entry             */
    GT_U32 ipv6_v4_value;           /* pointer to ARP table entry             */
    GT_BIT setSBit = 1;             /* MPLS Label S bit                       */
    GT_U32 swapTtlMode;             /* swap TTL mode; Relevant when <ttl>=0   */
    GT_U32 iSid;                    /* MacInMac inner service identifier      */
    GT_BIT iUpMarkMode;             /* determine assignment of iUp            */
    GT_U32 iUp;                     /* inner user priority field in iTag      */
    GT_BIT iDp;                     /* inner drop precedence in the iTag      */
    GT_BIT iDpMarkMode;             /* determine assignment of iDp            */
    GT_U32 iTagRes2;                /* 2 reserved bits to be set in the I-Tag */
    GT_BIT iTagRes1;                /* 1 reserved bits to be set in the I-Tag */
    GT_BIT iTagCda;                 /* CDA field to be set in the I-Tag       */
    GT_U32 tunnelStartPointer;         /* tunnel start index */
    GT_BIT  retainCrc = 0;/*Relevant when the passenger packet is Ethernet.
                        Determines whether the Ethernet passenger packet 4-bytes of CRC should be retained.
                        NOTE: When the passenger packet CRC field is retained in the packet it is unmodified,
                              even if the passenger packet was changed by the device.
                              Therefore, when this field is set to 1 and the passenger packet is modified,
                              the retained CRC transmitted in the packet does not reflect the changes made
                              to the passenger packet.
                              0x0 = Remove;
                                    The Ethernet passenger packet 4-bytes of CRC are removed and the tunneled
                                    packet is transmitted with a newly generated CRC for the entire packet.
                              0x1 = Don't Remove; The Ethernet passenger packet 4-bytes of CRC are not removed,
                                    nor modified, and the tunneled packet is transmitted with two CRC fields,
                                    the passenger packet original, unmodified CRC field and the newly generated
                                    CRC for the entire packet.*/

    GT_U8                   *origEgressPtr;
    GT_U8                   *nextBufferPtr;
    CH2_TS_INFO_STC         tsInfo;

    tsInfo.controlWordIndex = 0;/* to support common code */

    /* start with the Ethernet header */
    origEgressPtr = devObjPtr->egressBuffer1;

    descrPtr->tunnelStartMacInfoPtr = origEgressPtr;
    descrPtr->tunnelStartMacInfoLen = 12;/* mac sa,da */

    nextBufferPtr = origEgressPtr;

    tunnelStartPointer = descrPtr->tunnelPtr;

    regAddr = SMEM_CHT2_TUNNEL_START_TBL_MEM(devObjPtr, tunnelStartPointer);
    tunnelStartAction = smemMemGet(devObjPtr, regAddr);

    tunnelType =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 0, 2));
    up_mode    =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 2, 1));
    up         =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 3, 3));
    tagEnable  =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 6, 1));
    vId        =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 7, 12));
    ttl        =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 19, 8));
    cfi        =  (SMEM_U32_GET_FIELD(tunnelStartAction[0], 27, 1));

    if (tunnelType == SKERNEL_FRAME_TUNNEL_START_TYPE_MIM_E && !SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        return;
    }

    /* only fill the VLAN tagging info , and let the "egress HA" to build it
       because the frame may egress from a DSA tagged port , and we not want to
       Query the vlanEtherType (0x8100) from here, and duplicate code...
    */

    haInfoPtr->tsVlanTagInfo.vlanTagged = tagEnable;

    if (tagEnable)
    {
        haInfoPtr->tsVlanTagInfo.vlanId = vId;
        haInfoPtr->tsVlanTagInfo.up     = (up_mode == 1) ? up :  descrPtr->up;
        haInfoPtr->tsVlanTagInfo.cfi    = cfi;
    }

    /* mac DA */
    nextBufferPtr[0] = (GT_U8)SMEM_U32_GET_FIELD(tunnelStartAction[2], 8, 8);
    nextBufferPtr[1] = (GT_U8)SMEM_U32_GET_FIELD(tunnelStartAction[2], 0, 8);
    nextBufferPtr[2] = (GT_U8)SMEM_U32_GET_FIELD(tunnelStartAction[1], 24, 8);
    nextBufferPtr[3] = (GT_U8)SMEM_U32_GET_FIELD(tunnelStartAction[1], 16, 8);
    nextBufferPtr[4] = (GT_U8)SMEM_U32_GET_FIELD(tunnelStartAction[1], 8, 8);
    nextBufferPtr[5] = (GT_U8)SMEM_U32_GET_FIELD(tunnelStartAction[1], 0, 8);

    /* calculate the mac SA */
    __LOG(("calculate the mac SA"));
    snetChtHaMacFromMeBuild(devObjPtr,descrPtr,egressPort,GT_TRUE,&nextBufferPtr[6]);

    /* build the L3 info */
    __LOG(("build the L3 info"));
    memset(&tsInfo,0,sizeof(tsInfo));

    tsInfo.ttl = ttl ? ttl : descrPtr->ttl;

    descrPtr->tunnelStartType = tunnelType;

    __LOG(("IPv4 Tunneling"));
    switch(descrPtr->tunnelStartType)
    {   /* IPv4 Tunneling */
        case SKERNEL_FRAME_TUNNEL_START_TYPE_IPV4_E:
        {
            if(devObjPtr->supportTunnelStartEthOverIpv4)
            {
                retainCrc    =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 25, 1));
            }
            /*dfflag       =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 13, 1));*/
            dscp_mode    =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 12, 1));
            dscp         =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 6, 6));
            gRE          =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 5, 1));
            autoflag     =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 4, 1));

            tsInfo.dscp = (dscp_mode == 1) ? dscp : descrPtr->dscp;
            tsInfo.gre = gRE;

            if ((descrPtr->isIPv4 == 0) && (autoflag == 1))
            { /* IPv6 only */
                __LOG(("IPv6 only"));

                autoOffset =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 0, 4));
                ipv6_v4_offset= autoOffset%4;
                ipv6_v4_value = autoOffset/4;

                tsInfo.dip = (((descrPtr->dip[ipv6_v4_value]) << 8*ipv6_v4_offset) |
                              (descrPtr->dip[ipv6_v4_value+1]  >> 8*(4-ipv6_v4_offset)));

            }
            else
            {

                tsInfo.dip  = (SMEM_U32_GET_FIELD(tunnelStartAction[4],  0, 8) << 0);
                tsInfo.dip |= (SMEM_U32_GET_FIELD(tunnelStartAction[4],  8, 8) << 8);
                tsInfo.dip |= (SMEM_U32_GET_FIELD(tunnelStartAction[4], 16, 8) << 16);
                tsInfo.dip |= (SMEM_U32_GET_FIELD(tunnelStartAction[4], 24, 8) << 24);


            }

            tsInfo.sip  = (SMEM_U32_GET_FIELD(tunnelStartAction[5],  0, 8) << 0);
            tsInfo.sip |= (SMEM_U32_GET_FIELD(tunnelStartAction[5],  8, 8) << 8);
            tsInfo.sip |= (SMEM_U32_GET_FIELD(tunnelStartAction[5], 16, 8) << 16);
            tsInfo.sip |= (SMEM_U32_GET_FIELD(tunnelStartAction[5], 24, 8) << 24);


            if (gRE == 0)
            {
                if (descrPtr->isIPv4 == 0)
                {
                    tsInfo.protocol = 41;
                }
                else
                {
                    tsInfo.protocol = 4;
                }
            }
            else
            {
                tsInfo.protocol = 47;
            }

        }
        break;

        case SKERNEL_FRAME_TUNNEL_START_TYPE_MPLS_E:
        {/* MPLS Tunneling */
            __LOG(("MPLS Tunneling"));
            numOfLabels  =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 24, 1));
            retainCrc    =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 25, 1));

            if (SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
            {
                numOfLabels |=  ((SMEM_U32_GET_FIELD(tunnelStartAction[3], 26, 1)) << 1);
                setSBit      =   (SMEM_U32_GET_FIELD(tunnelStartAction[3], 27, 1));
                swapTtlMode  =   (SMEM_U32_GET_FIELD(tunnelStartAction[3], 28, 2));
                if (ttl == 0)
                {
                    /* Refers to the TTL of the outmost label before adding the new labels.
                       and the TTI Action <MPLS Command> = Push */
                    if (swapTtlMode == 0 && descrPtr->mplsCommand == SKERNEL_XCAT_TTI_MPLS_PUSH_E)
                    {
                        /* nothing to do - already assigned the value form descrPtr->ttl */
                        __LOG(("nothing to do - already assigned the value form descrPtr->ttl"));
                    }
                    /* Relevant only when TTI Action <MPLS Command> = Swap.
                       Tunnel<TTL> Refers to the TTL of the swapped MPLS label. */
                    if(descrPtr->mplsCommand == SKERNEL_XCAT_TTI_MPLS_SWAP_E ||
                       descrPtr->mplsCommand == SKERNEL_XCAT_TTI_MPLS_PUSH_E)
                    {
                        if (swapTtlMode == 1 || swapTtlMode == 2)
                        {
                            tsInfo.ttl = descrPtr->origTunnelTtl;
                            if (swapTtlMode == 2 )
                            {
                                tsInfo.ttl --;
                            }
                        }
                    }
                }
            }

            /* (inner) label is always added */
            __LOG(("(inner) label is always added"));
            exp_mode1    =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 23, 1));
            exp1         =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 20, 3));
            label1       =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 0, 20));
            tsInfo.numOfLabels = numOfLabels;
            tsInfo.label[0] = label1;
            tsInfo.exp[0] = (exp_mode1 == 1) ? exp1 : descrPtr->exp1;
            tsInfo.sBit[0] = (setSBit == 1) ? 1 : 0;

            /* set descriptor TS relevant field */
            descrPtr->tsEgressMplsNumOfLabels = numOfLabels;/* not really used */

            if (numOfLabels >= 1)
            {
                 exp_mode2   =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 23, 1));
                 exp2        =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 20, 3));
                 label2      =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 0, 20));
                 tsInfo.label[1] = label2;
                 tsInfo.exp[1] = (exp_mode2 == 1) ? exp2 : descrPtr->exp2;
                 tsInfo.sBit[1] = 0;
            }

            if(devObjPtr->supportVpls && devObjPtr->vplsModeEnable.ha)
            {
                tsInfo.controlWordIndex =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 24, 3));
            }


            if (numOfLabels >= 2)
            {
                exp_mode3   =  (SMEM_U32_GET_FIELD(tunnelStartAction[5], 23, 1));
                exp3        =  (SMEM_U32_GET_FIELD(tunnelStartAction[5], 20, 3));
                label3      =  (SMEM_U32_GET_FIELD(tunnelStartAction[5], 0, 20));
                tsInfo.label[2] = label3;
                tsInfo.exp[2] = (exp_mode3 == 1) ? exp3 : descrPtr->exp3;
                tsInfo.sBit[2] = 0;
            }
        }
        break;

        case SKERNEL_FRAME_TUNNEL_START_TYPE_MIM_E:
        { /* MacInMac */
            __LOG(("MacInMac"));
            iSid       =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 0, 23));
            iUpMarkMode=  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 24, 1));
            retainCrc  =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 25, 1));
            iUp        =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 26, 3));
            iDp        =  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 29, 1));
            iDpMarkMode=  (SMEM_U32_GET_FIELD(tunnelStartAction[3], 30, 1));
            iTagRes2   =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 0, 2));
            iTagRes1   =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 2, 1));
            iTagCda    =  (SMEM_U32_GET_FIELD(tunnelStartAction[4], 3, 1));

            tsInfo.iSid = iSid;
            tsInfo.iUp  = (iUpMarkMode == 1) ? iUp : descrPtr->iUp;
            tsInfo.iDp  = (iDpMarkMode == 1) ? iDp : descrPtr->iDp;
            tsInfo.iTagRes2 = iTagRes2;
            tsInfo.iTagRes1 = iTagRes1;
            tsInfo.iTagCda  = iTagCda;
        }
        break;

        default:
            return;
    }

    /* tunnelStartPassengerType: 0x0 = Ethernet, 0x1 = Other */
    if(retainCrc == 1 &&
       haInfoPtr->tunnelStartPassengerType == 0)
    {
        haInfoPtr->retainCrc = 1;
    }

    __LOG(("haInfoPtr->retainCrc = %d \n",
        haInfoPtr->retainCrc));

    /* set point to the end of Mac info */
    __LOG(("set point to the end of Mac info"));
    descrPtr->tunnelStartRestOfHeaderInfoPtr = descrPtr->tunnelStartMacInfoPtr + descrPtr->tunnelStartMacInfoLen;

    nextBufferPtr = snetCht2HaTunnelStartL3(devObjPtr,descrPtr,haInfoPtr,egressPort,&tsInfo,
                descrPtr->tunnelStartRestOfHeaderInfoPtr);

    /* set the length of the L3 info -- this include the EtherType !! */
    descrPtr->tunnelStartRestOfHeaderInfoLen = nextBufferPtr - descrPtr->tunnelStartRestOfHeaderInfoPtr;
    __LOG(("set the length [%d] of the L3 info -- this include the EtherType !!",
        descrPtr->tunnelStartRestOfHeaderInfoLen));

}


