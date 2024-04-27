/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetXCat.c
*
* DESCRIPTION:
*       This is a external API definition for xCat frame processing.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 68 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SLog/simLog.h>

#define SNET_XCAT_GET_GLOBAL_TPID_MAC(data_ptr, index) \
    SMEM_U32_GET_FIELD((data_ptr)[((index) / 2)], 16 * ((index) % 2), 16)

#define SNET_XCAT_GET_TAG0_PORT_BITMAP_TPID_MAC(data_ptr, port) \
    SMEM_U32_GET_FIELD((data_ptr)[((port) / 2)], 16 * ((port) % 2), 8)

#define SNET_XCAT_GET_TAG1_PORT_BITMAP_TPID_MAC(data_ptr, port) \
    SMEM_U32_GET_FIELD((data_ptr)[((port) / 2)], 8 + (16 * ((port) % 2)), 8)

#define SNET_XCAT_GET_EGR_PORT_TPID_MAC(data_ptr, port) \
    SMEM_U32_GET_FIELD((data_ptr)[((port) / 8)], 3 * ((port) % 8), 3)


#define SNET_XCAT_VLAN_PORT_ISOLATION_MODE_GET_MAC(dev, entry) \
    (SKERNEL_IS_LION_REVISON_B0_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry[8], 23, 2) : \
        SMEM_U32_GET_FIELD(entry[3], 21, 2)

#define SNET_XCAT_GLOBAL_TPID_NUM_CNS                       8

/*******************************************************************************
*   internalVlanTagMatch
*
* DESCRIPTION:
*      Check Ingress Global TPID table for packet ethernet type matching ,
*      and set descrPtr->tpidIndex[tagIndex] with the tpidIndex
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       etherType   - packet ethernet type
*       portTpIdBmp - port TPID bitmap
*       tagIndex    - tag index :
*                       0 - tag 0
*                       1 - tag 1
*                       2.. - extra tags
*       modifyTag0Tag1Indication - indication to modify descrPtr->tpidIndex[tagIndex]
*                       relevant only to tagIndex =  0,1
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* OUTPUTS:
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* RETURN:
*
*******************************************************************************/
static GT_BOOL internalVlanTagMatch
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr ,
    IN    GT_U32                           etherType,
    IN    GT_U32                           portTpIdBmp,
    IN    GT_U32                           tagIndex,
    IN    GT_BOOL                          modifyTag0Tag1Indication,
    INOUT GT_U32                            *tagSizePtr
)
{
    DECLARE_FUNC_NAME(internalVlanTagMatch);

    GT_U32 i;
    GT_U32 globalTpid;              /* global TPID */
    GT_U32 * regPtr;                /* register entry pointer */
    GT_U32 regValue;
    GT_U32 srcPortBpeEnable = 0;        /* Enable BPE for port */
    SKERNEL_PORTS_BMP_STC sourcePortsBmp; /* Source ports bitmap */

    __LOG_PARAM(tagIndex);
    __LOG_PARAM(modifyTag0Tag1Indication);

    SKERNEL_PORTS_BMP_CLEAR_MAC(&sourcePortsBmp);

    if(devObjPtr->support802_1br_PortExtender)
    {
        regPtr = smemMemGet(devObjPtr, SMEM_XCAT3_TTI_SOURCE_PORT_BPE_ENABLE_REG(devObjPtr));
        SKERNEL_FILL_PORTS_BITMAP_MAC(devObjPtr, &sourcePortsBmp, regPtr);
        srcPortBpeEnable  = SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(&sourcePortsBmp , descrPtr->localDevSrcPort);
    }

    regPtr = smemMemGet(devObjPtr, SMEM_XCAT_INGR_GLOBAL_TAG_TPID_REG(devObjPtr));

    __LOG(("look for etherType[0x%4.4x] TPID bitmap[0x%2.2x] \n",
        etherType,portTpIdBmp));

    for (i = 0; i < SNET_XCAT_GLOBAL_TPID_NUM_CNS; i++)
    {
        /* Port TPID bitmap */
        if ((portTpIdBmp >> i) & 0x01)
        {
            globalTpid = SNET_XCAT_GET_GLOBAL_TPID_MAC(regPtr, i);
            if(globalTpid == etherType)
            { /* got match */
                if((modifyTag0Tag1Indication == GT_TRUE) && tagIndex < 2)
                {
                    /* save the index of the matching TPID for tag 0 or tag 1*/
                    __LOG(("save the index[%d] of the matching TPID for tag[%d] \n",
                        i,tagIndex));
                    descrPtr->tpidIndex[tagIndex] = i;

                    if(tagIndex == 1)
                    {
                        __LOG(("Recognized TAG1 'locally' on this device \n"));
                        /* Indicates that the packet inner/outer ethertype matches
                           one of the local dev Ingress port Tag1 TPIDs (regardless to DSA tag info) */
                        /* this field is relevant to sip5 only */
                        descrPtr->tag1LocalDevSrcTagged = 1;
                        __LOG_PARAM(descrPtr->tag1LocalDevSrcTagged);
                    }
                }

                /* check is tag extended */
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && tagSizePtr)
                {
                    GT_U32  numBits = SMEM_CHT_IS_SIP5_15_GET(devObjPtr) ? 2 : 1;

                    smemRegFldGet(devObjPtr, SMEM_LION3_TTI_INGR_TPID_TAG_TYPE_REG(devObjPtr), i*numBits, numBits, &regValue);
                    *tagSizePtr = (regValue == 1) ? 8 :
                                  (regValue == 2) ? 6 :/* case valid only for sip5_15 */
                                  4;
                    __LOG(("tag[%d] : Recognize %d bytes vlan tag \n",
                        tagIndex,
                        *tagSizePtr
                         ));
                }
                else if(devObjPtr->support802_1br_PortExtender && /* device support tag1 recognition as 'ETag' */
                        tagSizePtr &&
                        (srcPortBpeEnable == 1) &&
                        tagIndex == 1)
                {
                    *tagSizePtr = 8;
                    __LOG(("Port Extender (BPE) support : tag[%d] : Considered ETag (8 bytes vlan tag) \n",
                        tagIndex));
                }


                __LOG(("index[%d] hold TPID[0x%4.4x] that MATCH the etherType \n",
                    i,globalTpid));
                return GT_TRUE;
            }
            else
            {
                __LOG(("index[%d] hold TPID[0x%4.4x] that not match \n",
                    i,globalTpid));
            }
        }
    }

    __LOG(("etherType[0x%4.4x] was not matched \n",
        etherType ));

    return GT_FALSE;
}

/*******************************************************************************
*   snetXCatVlanTagMatchWithoutTag0Tag1Classification
*
* DESCRIPTION:
*      Check Ingress Global TPID table for packet ethernet type matching ,
*      without changing descrPtr->tpidIndex[tagIndex] !
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       etherType   - packet ethernet type
*       portTpIdBmp - port TPID bitmap
*       tagIndex    - tag index :
*                       0 - tag 0
*                       1 - tag 1
*                       2.. - extra tags
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* OUTPUTS:
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* RETURN:
*
*******************************************************************************/
GT_BOOL snetXCatVlanTagMatchWithoutTag0Tag1Classification
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr ,
    IN    GT_U32                           etherType,
    IN    GT_U32                           portTpIdBmp,
    IN    GT_U32                           tagIndex,
    INOUT GT_U32                           *tagSizePtr
)
{
    return internalVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp,
            tagIndex, GT_FALSE, tagSizePtr);
}

/*******************************************************************************
*   snetXCatVlanTagMatch
*
* DESCRIPTION:
*      Check Ingress Global TPID table for packet ethernet type matching ,
*      and set descrPtr->tpidIndex[tagIndex] with the tpidIndex
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       etherType   - packet ethernet type
*       portTpIdBmp - port TPID bitmap
*       tagIndex    - tag index :
*                       0 - tag 0
*                       1 - tag 1
*                       2.. - extra tags
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* OUTPUTS:
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* RETURN:
*
*******************************************************************************/
static GT_BOOL snetXCatVlanTagMatch
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr ,
    IN    GT_U32                           etherType,
    IN    GT_U32                           portTpIdBmp,
    IN    GT_U32                           tagIndex,
    INOUT GT_U32                           *tagSizePtr
)
{
    return internalVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp,
            tagIndex, GT_TRUE, tagSizePtr);
}

/*******************************************************************************
*   snetXCatIngressVlanTagClassify
*
* DESCRIPTION:
*       Ingress Tag0 VLAN and Tag1 VLAN classification
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       parseMode - parsing mode : from port/tti /trill..
*
* OUTPUT:
*       inVlanEtherType0Ptr - pointer to Tag0 VLAN EtherType
*       inVlanEtherType1Ptr - pointer to Tag1 VLAN EtherType
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetXCatIngressVlanTagClassify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 ethTypeOffset,
    OUT GT_U32 * inVlanEtherType0Ptr,
    OUT GT_U32 * inVlanEtherType1Ptr,
    IN SNET_CHT_FRAME_PARSE_MODE_ENT   parseMode
)
{
    DECLARE_FUNC_NAME(snetXCatIngressVlanTagClassify);

    GT_U32 regAddress;              /* register address */
    GT_U32 etherType;               /* real frame ethernet type */
    GT_U32 * regPtr;                /* register entry pointer */
    GT_U32 portTpIdBmp0 = 0,portTpIdBmp1 = 0; /* TPID bitmap 0,1 to use */
    GT_BOOL retVal;                 /* return value */
    GT_U32 localPort;               /* local ingress port */
    GT_U32 ii;                      /*iterator*/
    GT_U32  profileForTag[SNET_CHT_TAG_MAX_INDEX_CNS] = {0,0};/* the profiles for the tags 0,1*/
    GT_U32  tagExtendedSize     = 0; /* must be initialized by default, because it is used as INOUT param later */
    GT_U32  marvellTaggedUsed = (descrPtr->marvellTagged && parseMode == SNET_CHT_FRAME_PARSE_MODE_PORT_E) ? 1 : 0;
    GT_BIT  tag1Exists; /* indication that tag1 recognized */

    __LOG_PARAM(marvellTaggedUsed);

    descrPtr->ingressVlanTag0Type =
    descrPtr->ingressVlanTag1Type = SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E;
    if(descrPtr->eArchExtInfo.ttiPreTtiLookupIngressEPortTablePtr)
    {
        /* The default Tag1 VLAN-ID that is assigned in the event that Tag1 is
           not found in the packet.
           Note: previously the Tag1 VLAN-ID was assigned value of 0 if not
           found in the packet */
        descrPtr->vid1 =
            SMEM_LION3_TTI_DEFAULT_EPORT_ENTRY_FIELD_GET(devObjPtr, descrPtr,
                SMEM_LION3_TTI_DEFAULT_E_PORT_TABLE_FIELDS_DEF_TAG1_VLAN_ID);
    }
    else
    {
        descrPtr->vid1 = 0;
    }

    *inVlanEtherType0Ptr =
    *inVlanEtherType1Ptr = 0xFFFFFFFF;

    etherType  = (descrPtr->startFramePtr[ethTypeOffset] << 8) |
                  descrPtr->startFramePtr[ethTypeOffset + 1];

    localPort = descrPtr->localDevSrcPort;

    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(devObjPtr->vplsModeEnable.tti &&
           parseMode != SNET_CHT_FRAME_PARSE_MODE_PORT_E)
        {
            /* Passenger Ingress Tag0/Tag1 VLAN Ethertype Select Address and Data Pointer*/
            __LOG(("Passenger Ingress Tag0/Tag1 VLAN Ethertype Select Address and Data Pointer"));
            regAddress = SMEM_XCAT_PASSENGER_INGR_PORT_TPID_REG(devObjPtr);
        }
        else
        {
            /* Ingress Tag0/Tag1 VLAN Ethertype Select Address and Data Pointer*/
            __LOG(("Ingress Tag0/Tag1 VLAN Ethertype Select Address and Data Pointer"));
            regAddress = SMEM_XCAT_INGR_PORT_TPID_REG(devObjPtr);
        }
        regPtr = smemMemGet(devObjPtr, regAddress);

        /* Tag0 TPID bitmap */
        portTpIdBmp0 =
            SNET_XCAT_GET_TAG0_PORT_BITMAP_TPID_MAC(regPtr, localPort);
        /* Tag1 TPID bitmap */
        portTpIdBmp1 =
            SNET_XCAT_GET_TAG1_PORT_BITMAP_TPID_MAC(regPtr, localPort);
    }
    else
    {
        if(descrPtr->eArchExtInfo.ttiPostTtiLookupIngressEPortTablePtr)
        {
            __LOG(("use TPID profiles from ePort table (post TTI lookup)"));
            /* when we parse the packet AFTER the TTI action --> we have pointer to
                the 'post TTI EPort table' then we use the profile from this table */
            profileForTag[SNET_CHT_TAG_0_INDEX_CNS] =
                SMEM_LION3_TTI_EPORT_ATTRIBUTES_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                    SMEM_LION3_TTI_EPORT_ATTRIBUTES_TABLE_FIELDS_ING_TAG0_TPID_PROFILE);
            profileForTag[SNET_CHT_TAG_1_INDEX_CNS] =
                SMEM_LION3_TTI_EPORT_ATTRIBUTES_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                    SMEM_LION3_TTI_EPORT_ATTRIBUTES_TABLE_FIELDS_ING_TAG1_TPID_PROFILE);
        }
        else if(descrPtr->eArchExtInfo.ttiPreTtiLookupIngressEPortTablePtr)
        {
            __LOG(("use TPID profiles from default-ePort table (pre TTI lookup)"));
            profileForTag[SNET_CHT_TAG_0_INDEX_CNS] =
                SMEM_LION3_TTI_DEFAULT_EPORT_ENTRY_FIELD_GET(devObjPtr, descrPtr,
                    SMEM_LION3_TTI_DEFAULT_E_PORT_TABLE_FIELDS_INGRESS_TAG0_TPID_PROFILE);
            profileForTag[SNET_CHT_TAG_1_INDEX_CNS] =
                SMEM_LION3_TTI_DEFAULT_EPORT_ENTRY_FIELD_GET(devObjPtr, descrPtr,
                    SMEM_LION3_TTI_DEFAULT_E_PORT_TABLE_FIELDS_INGRESS_TAG1_TPID_PROFILE);
        }
        else
        {
            skernelFatalError(" snetXCatIngressVlanTagClassify : neither e-port entry nor default eport entry are valid  \n");
        }

        /* Tag0 TPID bitmap */
        ii = SNET_CHT_TAG_0_INDEX_CNS;
        regAddress = SMEM_LION3_TTI_INGR_TAG_PROFILE_TPID_SELECT_REG(devObjPtr,
                ii,profileForTag[ii]);
        smemRegFldGet(devObjPtr, regAddress , 8*(profileForTag[ii] % 4) , 8 ,&portTpIdBmp0);

        /* Tag1 TPID bitmap */
        ii = SNET_CHT_TAG_1_INDEX_CNS;
        regAddress = SMEM_LION3_TTI_INGR_TAG_PROFILE_TPID_SELECT_REG(devObjPtr,
                ii,profileForTag[ii]);
        smemRegFldGet(devObjPtr, regAddress , 8*(profileForTag[ii] % 4) , 8 ,&portTpIdBmp1);

    }

    if (marvellTaggedUsed)
    {
        if(descrPtr->marvellTaggedExtended < SKERNEL_EXT_DSA_TAG_4_WORDS_E)
        {
            /* Packet arrived FORWARD DSA-tagged */
            if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E || SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr) == 0)
                {
                    __LOG(("Packet arrived FORWARD DSA-tagged"));
                }

                /* Tag0 is DSA-tagged  */
                __LOG(("Tag0 is DSA-tagged"));
                descrPtr->ingressVlanTag0Type = SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;

                /* Check Inner Tag1 match */
                __LOG(("Check Inner Tag1 match"));
                retVal = snetXCatVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp1,
                                                        SNET_CHT_TAG_1_INDEX_CNS, NULL);
                if(retVal)
                {
                    /* Tag0 is DSA-tagged  */
                    descrPtr->ingressVlanTag1Type =
                        SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E;
                    /* Tag1 VLAN ethertype */
                    *inVlanEtherType1Ptr = etherType;
                }
                else
                {
                    descrPtr->ingressVlanTag1Type =
                        SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E;
                }
            }

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr) /* && descrPtr->marvellTaggedExtended < SKERNEL_EXT_DSA_TAG_4_WORDS_E*/)
            {
                /* If 4B/8B DSA
                Desc<Tag0SrcTagged> = DSA<Tag0 SrcTagged>         --> already assigned
                Desc<Tag1SrcTagged> = VLAN1_Exists
                Desc<Tag0IsOuterTag> = 1                         --> done via descrPtr->ingressVlanTag0Type = SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E
                Desc<Tag0 TPID Index> = index of first bit set in Src ePort Tag0 TPID bitmap
                Desc<Tag1 TPID Index> = VLAN1_Exist ? (matched Vlan1 TPID) : 0
                Desc<Tag1 LocalDevSrcTagged> = VLAN1 Exists ? 1 : 0
                */
                descrPtr->tagSrcTagged[SNET_CHT_TAG_1_INDEX_CNS] = TAG1_EXIST_MAC(descrPtr);
                for(ii = 0 ; ii < 8 ; ii++)
                {
                    if(portTpIdBmp0 & (1<<ii))
                    {
                        /*index of first bit set in Src ePort Tag0 TPID bitmap*/
                        descrPtr->tpidIndex[SNET_CHT_TAG_0_INDEX_CNS] = ii;
                        break;
                    }
                }

                descrPtr->tag0IsOuterTag = 1;
            }
        }
        else /*(SMEM_CHT_IS_SIP5_GET(devObjPtr) && descrPtr->marvellTaggedExtended >= SKERNEL_EXT_DSA_TAG_4_WORDS_E)*/
        {
            /* If eDSA
            Desc<Tag0SrcTagged> = eDSA<Tag0SrcTagged>         already assigned
            Desc<Tag1SrcTagged> = eDSA<Tag1SrcTagged>         already assigned
            Desc<Tag0IsOuterTag> = eDSA<Tag0IsOuterTag>       already assigned
            Desc<Tag0 TPID Index> = eDSA<Tag0 TPID Index>     already assigned
            Desc<Tag1 TPID Index> = VLAN1 Exists ? TPID Index matching Tag1 TPID: 0
            Desc<Tag1 LocalDevSrcTagged> = VLAN1 Exists ? 1 : 0
            */

            /* NOTE: the FROM_CPU DSA tag when useVidx = 0 ,
                     not hold info about Tag0SrcTagged,Tag1SrcTagged,Tag0IsOuterTag
                     still it hold <Tag0 TPID Index>
                     but it hold info about how to egress the destination.
            */

            /* Check Inner Tag1 match --> assign descrPtr->tpidIndex[SNET_CHT_TAG_1_INDEX_CNS]*/
            retVal = snetXCatVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp1, SNET_CHT_TAG_1_INDEX_CNS, NULL);

            if(descrPtr->incomingMtagCmd == SKERNEL_MTAG_CMD_TO_CPU_E ||
               descrPtr->incomingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
            {
                descrPtr->tagSrcTagged[SNET_CHT_TAG_1_INDEX_CNS] = retVal; /* not in the eDSA */
                descrPtr->tag0IsOuterTag = 1;/* not in the eDSA */
            }
            else
            if(descrPtr->incomingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E &&
               descrPtr->useVidx == 0)
            {
                /* NOTE: the FROM_CPU DSA tag when useVidx = 0 ,
                         not hold info about Tag0SrcTagged,Tag1SrcTagged,Tag0IsOuterTag
                         still it hold <Tag0 TPID Index>
                         but it hold info about how to egress the destination.
                */
                /* the device should treat packet as if the eDSA hold tag0 (never tag 1) */
                descrPtr->tag0IsOuterTag = 1;/* not in the eDSA */

            }

            tag1Exists = descrPtr->tag1LocalDevSrcTagged;/* not descrPtr->tagSrcTagged[SNET_CHT_TAG_1_INDEX_CNS] */

            if(descrPtr->tagSrcTagged[SNET_CHT_TAG_1_INDEX_CNS] != descrPtr->tag1LocalDevSrcTagged)
            {
                __LOG(("Note : mismatch eDSA<Tag1 Src Tagged> = [%s] but 'locally' tag1 = [%s] \n"
                    ,descrPtr->tagSrcTagged[SNET_CHT_TAG_1_INDEX_CNS] ? "exists" : "not exists"
                    ,descrPtr->tag1LocalDevSrcTagged                  ? "exists" : "not exists"
                    ));
            }

            if(descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] && tag1Exists)
            {
                if(descrPtr->tag0IsOuterTag)
                {
                    descrPtr->ingressVlanTag0Type =
                        SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
                    descrPtr->ingressVlanTag1Type =
                        SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E;
                }
                else
                {
                    descrPtr->ingressVlanTag0Type =
                        SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E;
                    descrPtr->ingressVlanTag1Type =
                        SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
                }
            }
            else if (descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS])
            {
                descrPtr->ingressVlanTag0Type =
                    SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
            }
            else if (tag1Exists)
            {
                descrPtr->ingressVlanTag1Type =
                    SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
            }
            else /* tag 1 in not in the eDSA ... so it must be tag0.
                    fix [JIRA] [CPSS-4289] : When packet is received untagged on the
                    cascade port it doesn't take the DSA tag VID. */
            {
                /* as in other DSA tag formats , we always consider the 'tag0' to
                   exists in the eDSA */
                descrPtr->ingressVlanTag0Type =
                    SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
            }
        }

        return;
    }

    /* Check Non DSA Outer Tag0 match */
    retVal = snetXCatVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp0, SNET_CHT_TAG_0_INDEX_CNS, &tagExtendedSize);
    if(retVal)
    {
        /* Outer EtherType matches any of the port selected Tag0 TPIDs */
        descrPtr->ingressVlanTag0Type = SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
        /* Tag0 VLAN ethertype */
        *inVlanEtherType0Ptr = etherType;

        ethTypeOffset += 4;
        if(tagExtendedSize > 4)
        {
            ethTypeOffset += (tagExtendedSize - 4);

            /* state that the most outer tag is extended */
            descrPtr->firstVlanTagExtendedSize = tagExtendedSize;

            if(devObjPtr->errata.ttiWrongPparsingWhenETagSecondWordEqualToEthertype)
            {
                /* check if conditions of the errata apply */
                etherType = (descrPtr->startFramePtr[ethTypeOffset     - 4] << 8) |
                             descrPtr->startFramePtr[ethTypeOffset + 1 - 4];


                retVal = snetXCatVlanTagMatchWithoutTag0Tag1Classification(devObjPtr, descrPtr, etherType, portTpIdBmp0, SNET_CHT_TAG_0_INDEX_CNS, NULL);
                if(retVal == GT_TRUE)
                {
                    __LOG(("Warning : Errata: Wrong parsing when E-Tag second word equal to ethertype. \n"));
                    __LOG(("Warning : Errata: the E-Tag is parsed as 3 words tag. (instead of 2 words) \n"));

                    descrPtr->firstVlanTagExtendedSize += 4;/* indicate 3 words tag (not only 2 words)*/
                    ethTypeOffset += 4;
                }
            }

        }

        /* Tag1 ethernet type */
        etherType = (descrPtr->startFramePtr[ethTypeOffset] << 8) |
                     descrPtr->startFramePtr[ethTypeOffset + 1];

        /* Check Non DSA Inner Tag1 match */
        retVal = snetXCatVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp1, SNET_CHT_TAG_1_INDEX_CNS, NULL);
        if(retVal)
        {
            /* Inner EtherType matches any of the port selected Tag1 TPIDs */
            descrPtr->ingressVlanTag1Type = SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E;
            /* Tag1 VLAN ethertype */
            *inVlanEtherType1Ptr = etherType;
        }
        else
        {
            descrPtr->ingressVlanTag1Type =
                SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E;
        }
    }
    else /* Check Non DSA Outer Tag1 match */
    {
        /* Check Non DSA Inner Tag1 match */
        retVal = snetXCatVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp1, SNET_CHT_TAG_1_INDEX_CNS, &tagExtendedSize);
        if(retVal)
        {
            /* Outer EtherType matches any of the port selected Tag1 TPIDs */
            descrPtr->ingressVlanTag1Type = SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E;
            /* Tag1 VLAN ethertype */
            *inVlanEtherType1Ptr = etherType;

            ethTypeOffset += 4;
            if(tagExtendedSize > 4)
            {
                ethTypeOffset += (tagExtendedSize-4);

                /* state that the most outer tag is extended */
                descrPtr->firstVlanTagExtendedSize = tagExtendedSize;

                if(devObjPtr->errata.ttiWrongPparsingWhenETagSecondWordEqualToEthertype)
                {
                    /* check if conditions of the errata apply */
                    etherType = (descrPtr->startFramePtr[ethTypeOffset     - 4] << 8) |
                                 descrPtr->startFramePtr[ethTypeOffset + 1 - 4];


                    retVal = snetXCatVlanTagMatchWithoutTag0Tag1Classification(devObjPtr, descrPtr, etherType, portTpIdBmp0, SNET_CHT_TAG_1_INDEX_CNS, NULL);
                    if(retVal == GT_TRUE)
                    {
                        __LOG(("Warning : Errata: Wrong parsing when E-Tag second word equal to ethertype. \n"));
                        __LOG(("Warning : Errata: the E-Tag is parsed as 3 words tag. (instead of 2 words) \n"));

                        descrPtr->firstVlanTagExtendedSize += 4;/* indicate 3 words tag (not only 2 words)*/
                        ethTypeOffset += 4;
                    }
                }
            }


            /* Tag1 ethernet type */
            etherType  = (descrPtr->startFramePtr[ethTypeOffset] << 8) |
                          descrPtr->startFramePtr[ethTypeOffset + 1];

            /* Check Non DSA Outer Tag0 match */
            retVal = snetXCatVlanTagMatch(devObjPtr, descrPtr, etherType, portTpIdBmp0, SNET_CHT_TAG_0_INDEX_CNS, NULL);
            if(retVal)
            {
                /* Inner EtherType matches any of the port selected Tag0 TPIDs */
                descrPtr->ingressVlanTag0Type = SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E;
                /* Tag0 VLAN ethertype */
                *inVlanEtherType0Ptr = etherType;
            }
            else
            {
                descrPtr->ingressVlanTag0Type =
                    SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E;
            }
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Non-DSA
        Desc<Tag0SrcTagged> = VLAN0_Exists
        Desc<Tag1SrcTagged> = VLAN1_Exists
        Desc<Tag0IsOuterTag> = (Tag0 Is outer Tag) ? 1: 0;
        Desc<Tag0 TPID Index> = VLAN0_Exist ? (matched Vlan0 TPID) : 0     --> already done
        Desc<Tag1 TPID Index> = VLAN1_Exist ? (matched Vlan1 TPID) : 0     --> already done
        Desc<Tag1 LocalDevSrcTagged> = VLAN1 Exists ? 1 : 0
        */
        descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] = TAG0_EXIST_MAC(descrPtr);
        descrPtr->tagSrcTagged[SNET_CHT_TAG_1_INDEX_CNS] = TAG1_EXIST_MAC(descrPtr);
        descrPtr->tag0IsOuterTag = (descrPtr->ingressVlanTag0Type == SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E) ? 1 : 0;
    }

}

/*******************************************************************************
*   snetXCatHaEgressTunnelStartTagEtherTypeByTpid
*
* DESCRIPTION:
*       HA - for Egress tunnel start header :  get EtherType according to TPID
*
* INPUTS:
*       devObjPtr    - pointer to device object
*       descrPtr     - pointer to the frame's descriptor.
*       tpId         - TP-ID to select EtherType according to it
*       etherTypePtr - pointer to VLAN Ethertype
*
* RETURN:
*       none
*
*******************************************************************************/
static GT_VOID snetXCatHaEgressTunnelStartTagEtherTypeByTpid
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   tpId,
    INOUT GT_U32 * etherTypePtr
)
{
    DECLARE_FUNC_NAME(snetXCatHaEgressTunnelStartTagEtherTypeByTpid);

    GT_U32 regAddress;
    GT_U32 * regGlobalPtr;
    /* TPID Global Configuration register */
    regAddress = SMEM_XCAT_HA_TS_EGR_GLOBAL_TAG_TPID_REG(devObjPtr);
    regGlobalPtr = smemMemGet(devObjPtr, regAddress);
    *etherTypePtr =
        SNET_XCAT_GET_GLOBAL_TPID_MAC(regGlobalPtr, tpId);

    __LOG(("selected etherType [0x%4.4x] for tpid[%d] \n",
        *etherTypePtr ,
        tpId));
    return ;
}

/*******************************************************************************
*   snetXCatHaEgressTagEtherType
*
* DESCRIPTION:
*       HA - Egress Tag0 VLAN and Tag1 VLAN ethertype assignment
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr        - pointer to the frame's descriptor.
*       egressPort        - egress port (local port - not global)
*       egrTag0EtherTypePtr - pointer to egress Tag0 VLAN Ethertype
*       egrTag1EtherTypePtr - pointer to egress Tag1 VLAN Ethertype
*       tunnelStartEtherTypePtr - pointer to egress Tag of the tunnel start VLAN Ethertype
* RETURN:
*       GT_OK   - on success
*       GT_BAD_PTR - on NULL pointer
*
*******************************************************************************/
GT_STATUS snetXCatHaEgressTagEtherType
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 egressPort,
    OUT GT_U32 * egrTag0EtherTypePtr,
    OUT GT_U32 * egrTag1EtherTypePtr,
    OUT GT_U32 * tunnelStartEtherTypePtr
)
{
    DECLARE_FUNC_NAME(snetXCatHaEgressTagEtherType);

    GT_U32 regAddress;
    GT_U32 * regPtr;
    GT_U32 tpId0 = 0,tpId1 = 0,tsTpId = 0;
    GT_U32         outputPortBit;  /* the bit index for the egress port */
    GT_BOOL        isSecondRegister;/* is the per port config is in second register -- for bmp of ports */
    GT_BOOL         etherType0Retrieved = GT_FALSE;/* did we already Retrieved ethertype 0 */
    GT_BOOL         etherType1Retrieved = GT_FALSE;/* did we already Retrieved ethertype 1 */

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(egrTag0EtherTypePtr);
    ASSERT_PTR(egrTag1EtherTypePtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* TPID select for the tunnel-start header */
        tsTpId =
            SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_TS_HEADER_TPID_INDEX);

        descrPtr->tpidIndexTunnelstart = tsTpId;

        if(0 ==
            SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_KEEP_ORIGINAL_TAG0_TPID))
        {
            /*Egress Tag 0 TPID select - (based on egress ePort) */
            tpId0 =
                SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                    descrPtr,
                    SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_EGRESS_TAG0_TPID_INDEX);

            __LOG(("Egress Tag 0 TPID select[%d] - (based on egress ePort)",
                tpId0));
        }
        else
        {
            tpId0 = descrPtr->tpidIndex[SNET_CHT_TAG_0_INDEX_CNS];
        }

        if(0 ==
            SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_KEEP_ORIGINAL_TAG1_TPID))
        {
            /*Egress Tag 0 TPID select - (based on egress ePort) */
            tpId1 =
                SMEM_LION3_HA_EPORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                    descrPtr,
                    SMEM_LION3_HA_EPORT_TABLE_1_FIELDS_EGRESS_TAG1_TPID_INDEX);

            __LOG(("Egress Tag 1 TPID select[%d] - (based on egress ePort)",
                tpId1));
        }
        else
        {
            tpId1 = descrPtr->tpidIndex[SNET_CHT_TAG_1_INDEX_CNS];
        }

        descrPtr->tpidIndex[SNET_CHT_TAG_0_INDEX_CNS] = tpId0;
        descrPtr->tpidIndex[SNET_CHT_TAG_1_INDEX_CNS] = tpId1;

        __LOG_PARAM(descrPtr->tpidIndex[SNET_CHT_TAG_0_INDEX_CNS]);
        __LOG_PARAM(descrPtr->tpidIndex[SNET_CHT_TAG_1_INDEX_CNS]);
        __LOG_PARAM(descrPtr->tpidIndexTunnelstart);

        snetXCatHaEgressTagEtherTypeByTpid(devObjPtr,descrPtr,tsTpId,tunnelStartEtherTypePtr,NULL);
        snetXCatHaEgressTagEtherTypeByTpid(devObjPtr,descrPtr,tpId0,egrTag0EtherTypePtr,NULL);
        snetXCatHaEgressTagEtherTypeByTpid(devObjPtr,descrPtr,tpId1,egrTag1EtherTypePtr,NULL);

        __LOG_PARAM((*tunnelStartEtherTypePtr));
        __LOG_PARAM((*egrTag0EtherTypePtr));
        __LOG_PARAM((*egrTag1EtherTypePtr));
        return GT_OK;
    }

    snetChtHaPerPortInfoGet(devObjPtr,egressPort,&isSecondRegister,&outputPortBit);

    if(devObjPtr->vplsModeEnable.ha && descrPtr->useVidx == 0 && descrPtr->tunnelStart &&
       descrPtr->vplsInfo.egressTagStateAssigned )
    {
        /* The new 2-bit TPID index from the descriptor is relevant only when
           TagState was assigned by the EQ LPM table (!=0x7)

            Functional explanation:
            The LP Table <TPID Index> is applied to the PW passenger VLAN tags.
            if the egress tag state is taken from the TxQ VLAN table (because the LP table <egress tag state> == Unassigned 0x7),
            then the TPIDs would be based on the egress port rather than the LP Table.
        */

        regAddress = SMEM_XCAT_PASSENGER_TPID_INDEX_SOURCE_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddress);

        if(SMEM_U32_GET_FIELD(*regPtr, (egressPort& 0x1f), 1))
        {
            regAddress = SMEM_XCAT_PASSENGER_TPID_CONFIG_REG(devObjPtr);
            regPtr = smemMemGet(devObjPtr, regAddress);

            *egrTag0EtherTypePtr = snetFieldValueGet(regPtr,
                (descrPtr->egressPassangerTagTpidIndex * 32),16);

            *egrTag1EtherTypePtr = snetFieldValueGet(regPtr,
                (descrPtr->egressPassangerTagTpidIndex * 32) + 16 ,16);

            etherType0Retrieved = GT_TRUE;
            etherType1Retrieved = GT_TRUE;
        }
    }

    if(etherType0Retrieved == GT_FALSE)
    {
        /* Egress Tag0 VLAN Ethertype Select Address and Data Pointer*/
        __LOG(("Egress Tag0 VLAN Ethertype Select Address and Data Pointer"));
        regAddress = SMEM_XCAT_TAG0_EGR_PORT_TPID_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddress);
        tpId0 = SNET_XCAT_GET_EGR_PORT_TPID_MAC(regPtr, outputPortBit);
    }

    if(etherType0Retrieved == GT_FALSE)
    {
        snetXCatHaEgressTagEtherTypeByTpid(devObjPtr,descrPtr,tpId0,egrTag0EtherTypePtr,NULL);
    }

    if(devObjPtr->vplsModeEnable.ha)
    {

        /* Egress VLAN Ethertype for tunnel start tag */
        __LOG(("Egress VLAN Ethertype for tunnel start tag"));
        regAddress = SMEM_XCAT_HA_TS_EGR_PORT_TPID_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddress);
        tsTpId = SNET_XCAT_GET_EGR_PORT_TPID_MAC(regPtr, outputPortBit);

        snetXCatHaEgressTunnelStartTagEtherTypeByTpid(devObjPtr,descrPtr,tsTpId,tunnelStartEtherTypePtr);
    }


    if(etherType1Retrieved == GT_FALSE)
    {
        /* Egress Tag1 VLAN Ethertype Select Address and Data Pointer*/
        regAddress = SMEM_XCAT_TAG1_EGR_PORT_TPID_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddress);
        tpId1 = SNET_XCAT_GET_EGR_PORT_TPID_MAC(regPtr, outputPortBit);
    }

    if(etherType1Retrieved == GT_FALSE)
    {
        snetXCatHaEgressTagEtherTypeByTpid(devObjPtr,descrPtr,tpId1,egrTag1EtherTypePtr,NULL);
    }


    return GT_OK;
}

/*******************************************************************************
*   snetXCatHaEgressTagEtherTypeByTpid
*
* DESCRIPTION:
*       HA - Egress get EtherType according to TPID
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr        - pointer to the frame's descriptor.
*       tpId         - TP-ID to select EtherType according to it
*       etherTypePtr - pointer to VLAN Ethertype
*       tagSizePtr  -  size of tag extended, NULL means not used (sip5 only)
*
* RETURN:
*       none
*
*******************************************************************************/
GT_VOID snetXCatHaEgressTagEtherTypeByTpid
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   tpId,
    INOUT GT_U32 * etherTypePtr,
    INOUT GT_U32 *tagSizePtr
)
{
    DECLARE_FUNC_NAME(snetXCatHaEgressTagEtherTypeByTpid);

    GT_U32 regAddress;
    GT_U32 * regGlobalPtr;
    GT_U32 tmpVal;
    /* TPID Global Configuration register */

    if(tagSizePtr)
    {
        *tagSizePtr = GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddress = SMEM_XCAT_HA_GLOBAL_TAG_TPID_REG(devObjPtr,tpId);
        regGlobalPtr = smemMemGet(devObjPtr, regAddress);
        *etherTypePtr = SMEM_U32_GET_FIELD(regGlobalPtr[0], 0, 16);

        if(tagSizePtr)
        {
            GT_CHAR *tagTypeNames[]= {
                "0 = VLAN TAG: 4B VLAN tag",
                "1 = E TAG: 8B BPE tag",
                "2 = Reserved1 ---> WARNING simulation treat as '4B'",
                "3 = Reserved2 ---> WARNING simulation treat as '4B'"};
            GT_CHAR *tagTypeNames_sip5_15[]= {
                "0 = VLAN TAG: 4B VLAN tag",
                "1 = E TAG: 8B BPE tag",
                "2 = 6-bytes v-Tag",
                "3 = Reserved2 ---> WARNING simulation treat as '4B'"};

            tmpVal = SMEM_U32_GET_FIELD(regGlobalPtr[0], 16, 2);
            if(tmpVal < 3)
            {
                __LOG(("TPID Tag type [%s] \n",
                    SMEM_CHT_IS_SIP5_15_GET(devObjPtr) ?
                        tagTypeNames_sip5_15[tmpVal] :
                        tagTypeNames[tmpVal]

                        ));
            }

            *tagSizePtr = (tmpVal == 1) ? 8 :
                          (SMEM_CHT_IS_SIP5_15_GET(devObjPtr) && (tmpVal == 2)) ? 6 :
                           4;
        }

    }
    else
    {
        regAddress = SMEM_XCAT_HA_GLOBAL_TAG_TPID_REG(devObjPtr,0);
        regGlobalPtr = smemMemGet(devObjPtr, regAddress);
        *etherTypePtr = SNET_XCAT_GET_GLOBAL_TPID_MAC(regGlobalPtr, tpId);
    }

    __LOG(("selected etherType [0x%4.4x] for tpid[%d] \n",
        *etherTypePtr ,
        tpId));

    return ;
}

/*******************************************************************************
*   haPushTag0
*
* DESCRIPTION:
*       push tag 0 into the packet (egress)
*       NOTE: assume that this function is not called when packet came from
*       cascade port and need to egress from network port with 'push'.
*       because this case is treated as 'Outer Tag0, inner Tag1'
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       vlanTag0EtherType - VLAN Tag0 EtherType
*       vlanTag1EtherType - VLAN Tag1 EtherType
*       destVlanTagged  - destination port Tag state.
*
* OUTPUT:
*       tagDataPtr  - DSA/Vlan tag data.
*       tagDataLengthPtr - pointer to DSA/Vlan tag data length.
*
* RETURN:
*       none
*
*******************************************************************************/
static GT_VOID haPushTag0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  vlanTag0EtherType,
    IN GT_U32  vlanTag1EtherType,
    IN DSA_TAG_TYPE_E egrMarvellTag,
    OUT GT_U8 * tagDataPtr,
    INOUT GT_U32 * tagDataLengthPtr
)
{
    DECLARE_FUNC_NAME(haPushTag0);

    GT_32  tagLength;
    GT_32  ii;
    GT_U32  offset;

    if(descrPtr->marvellTagged)
    {
        /* see function comments */
        skernelFatalError(" haPushTag0 : not support call from cascade port \n");
    }

    tagLength = 0;
    offset = 0;

    if(egrMarvellTag == MTAG_TYPE_NONE_E)
    {
        /* add tag 0 according to descriptor values */
        snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                          (GT_U16)descrPtr->eVid,
                          (GT_U8)descrPtr->cfidei,
                          (GT_U16)vlanTag0EtherType, tagDataPtr);
        offset = 4;
    }
    else
    {
        /* the DSA tag already hold outer tag */
        __LOG(("the DSA tag already hold outer tag"));
    }

    if(descrPtr->ingressVlanTag0Type !=
       SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E)
    {
        tagLength += 4;
    }

    if(descrPtr->ingressVlanTag1Type !=
       SKERNEL_FRAME_TR101_VLAN_TAG_NOT_FOUND_E)
    {
        tagLength += 4;
    }

    for(ii = 0 ; ii < tagLength ; ii++)
    {
        tagDataPtr[offset+ii] = descrPtr->afterVlanOrDsaTagPtr[ii - tagLength];
    }

    *tagDataLengthPtr = tagLength + offset;

    return;
}

/*******************************************************************************
*   snetXCatHaEgressTagBuildEtag
*
* DESCRIPTION:
*        IEEE 802.1 header building. (xCat3)
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       descrPtr            - pointer to the frame's descriptor.
*       vpt                 - user priority tag
*       cfiDeiBit           - CFI/DEI bit
*       etherType           - VLAN ethernet type
* OUTPUT
*       eTagData            - tagged data - IEEE 802.1br order.
* RETURN:
*
*******************************************************************************/
static GT_VOID snetXCatHaEgressTagBuildEtag
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN  GT_U8                             e_pcp,
    IN  GT_U8                             e_dei,
    IN  GT_U16                            etherType,
    OUT GT_U8                             eTagData[] /* 8 bytes */
)
{
    DECLARE_FUNC_NAME(snetXCatHaEgressTagBuildEtag);

    GT_BIT IsMC;
    GT_U32  egressETag_GRP = 0;             /* on egress ETag : field GRP */
    GT_U32  egressETag_ingress_E_CID_base;  /* on egress ETag : field <ingress_E_CID_base> */
    GT_U32  egressETag_E_CID_base;          /* on egress ETag : field <E_CID_base> */
    GT_U8   *ingressETagPtr;     /* pointer to the ingress ETag ... if came with it */
    GT_U32  srcPortBpeEnable;     /* is ingress port may handle ETags */
    SKERNEL_PORTS_BMP_STC trgPortsBmp; /* Target ports bitmap */
    GT_U32 * regPtr;                /* register entry pointer */
    GT_U32  ingressETag_grp = 0;     /* field from ingress ETag */
    GT_U32  ingressETag_ECID_base=0;  /* field from ingress ETag */

    ASSERT_PTR(eTagData);

    /* ether type       - bits 63..48 (16 bits) */
    /* E-PCP            - bits 47..45 (3 bits) */
    /* E-DEI            - bit  44     (1 bit) */
    /* Ingress_CID_base - bits 43..32 (12 bits)*/
    /* GRP              - bits 29..28 (2 bits)*/
    /* E-CID_base       - bits 27..16 (12 bits)*/

    /* Get srcPort <BPE enabled> per port */
    regPtr = smemMemGet(devObjPtr, SMEM_XCAT3_HA_PER_PORT_BPE_ENABLE_REG(devObjPtr));
    SKERNEL_FILL_PORTS_BITMAP_MAC(devObjPtr, &trgPortsBmp, regPtr);
    srcPortBpeEnable = SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(&trgPortsBmp,
                            descrPtr->localDevSrcPort);

    __LOG_PARAM(srcPortBpeEnable);

    __LOG_PARAM(descrPtr->haToEpclInfo.bpe_802_1br_ingressInfo.tag1_exists)
    __LOG_PARAM(descrPtr->haToEpclInfo.bpe_802_1br_ingressInfo.tag1_is_outer)

    /* calculate */
    if (descrPtr->haToEpclInfo.bpe_802_1br_ingressInfo.tag1_exists && (descrPtr->firstVlanTagExtendedSize > 4))
    {
        ingressETagPtr = &descrPtr->startFramePtr[12];

        if(descrPtr->haToEpclInfo.bpe_802_1br_ingressInfo.tag1_is_outer)
        {
            __LOG(("ETag on ingress packet: as OUTER tag \n"));

        }
        else
        {
            /* calculate the length of tag0 on the packet */
            if(descrPtr->marvellTagged)
            {
                __LOG(("ETag on ingress packet: as INNER tag (after DSA tag) \n"));
                ingressETagPtr += (descrPtr->marvellTaggedExtended + 1) * 4;
            }
            else
            {
                __LOG(("ETag on ingress packet: as INNER tag \n"));
                ingressETagPtr += 4;
            }
        }


        ingressETag_grp = (ingressETagPtr[4] >> 4) & 3;
        __LOG_PARAM(ingressETag_grp);

        ingressETag_ECID_base = ingressETagPtr[5] |
                               ((ingressETagPtr[4] & 0xF) << 8);
        __LOG_PARAM(ingressETag_ECID_base);
    }
    else
    {
        __LOG(("no ETag on ingress packet \n"));
        ingressETagPtr = NULL;
    }

    /* IsMC= (srcPort<BPEport>=Enabled && tag1_exists && ingressE-Tag<GRP(1:0)> != 0) */
    IsMC = (srcPortBpeEnable && ingressETagPtr && ingressETag_grp) ? 1 : 0;
    /* E-Tag<Ingress_E-CID_base> = (IsMC ? Desc<VID1> : 0)*/
    /* E-Tag<E-CID_base>    = (IsMC     ?   ingressE-Tag<E-CID_base>    : Desc<VID1>)*/
    /* E-Tag<GRP(1:0)>      = (IsMC     ?   ingressE-Tag<GRP(1:0)>      : 0)*/

    if (IsMC == 1)
    {
        egressETag_ingress_E_CID_base = descrPtr->vid1;
        egressETag_E_CID_base = ingressETag_ECID_base;
        egressETag_GRP = ingressETag_grp;
        __LOG(("egress ETag is Multi-destination format \n"));
    }
    else
    {
        egressETag_ingress_E_CID_base = 0;
        egressETag_E_CID_base = descrPtr->vid1;
        egressETag_GRP = 0;
        __LOG(("egress ETag is single-destination format \n"));
    }

    __LOG_PARAM(egressETag_ingress_E_CID_base);
    __LOG_PARAM(egressETag_E_CID_base);
    __LOG_PARAM(egressETag_GRP);


    __LOG_PARAM_WITH_NAME("E-Tag : <E-TPID> 16 bits ",etherType);
    __LOG_PARAM_WITH_NAME("E-Tag : <E-PCP> 3 bits ",e_pcp);
    __LOG_PARAM_WITH_NAME("E-Tag : <E-DEI> 3 bits ",e_dei);
    __LOG_PARAM_WITH_NAME("E-Tag : <ingress_E-CID_base> 12 bits ",egressETag_ingress_E_CID_base);

    __LOG_PARAM_WITH_NAME("E-Tag : <E-CID_base> 12 bits ",egressETag_E_CID_base);
    __LOG_PARAM_WITH_NAME("E-Tag : <GRP> 2 bits ",egressETag_GRP);
    __LOG_PARAM_WITH_NAME("E-Tag : <Re-> 2 reserved bits ",0);
    __LOG_PARAM_WITH_NAME("E-Tag : <ingress_E-CID_ext> and <E-CID_ext> 16 reserved bits ",0);

    eTagData[0] = etherType >> 8;
    eTagData[1] = etherType & 0xff;
    eTagData[2] = (((e_pcp & 0x7) << 5) |
                  ((e_dei & 0x1) << 4) |
                  ((egressETag_ingress_E_CID_base & 0xfff) >> 8));
    eTagData[3] = egressETag_ingress_E_CID_base & 0xff;
    eTagData[4] = (((egressETag_GRP & 0x3) << 4) |
                  ((egressETag_E_CID_base & 0xfff) >> 8));
    eTagData[5] = egressETag_E_CID_base & 0xff;
    eTagData[6] = 0;
    eTagData[7] = 0;

    __LOG(("BPE :"));
    __LOG(("E-Tag BYTES:"));
    simLogDump(devObjPtr, SIM_LOG_INFO_TYPE_PACKET_E, (GT_PTR)eTagData, 8);

    return;
}

/*******************************************************************************
*   snetXCatHaEgressTagBuild
*
* DESCRIPTION:
*       HA - Build Tag0 VLAN and Tag1 VLAN according to Tag state
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor.
*       vlanTag0EtherType - VLAN Tag0 EtherType
*       vlanTag1EtherType - VLAN Tag1 EtherType
*       egrMarvellTag - type of marvell tag(none, standard, extended)
*       destVlanTagged  - destination port Tag state.
*
* OUTPUT:
*       tagDataPtr  - DSA/Vlan tag data.
*       tagDataLengthPtr - pointer to DSA/Vlan tag data length.
*       tag0OffsetInTagPtr - pointer to tag 0 offset from start of the buffer (tagDataPtr)
*       tag1OffsetInTagPtr - pointer to tag 1 offset from start of the buffer (tagDataPtr)
*
* RETURN:
*       GT_OK   - on success
*       GT_BAD_PTR - on NULL pointer
*       GT_BAD_PARAM - on wrong egress Vlan Tag state
*
*******************************************************************************/
GT_STATUS snetXCatHaEgressTagBuild
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32  vlanTag0EtherType,
    IN GT_U32  vlanTag1EtherType,
    IN DSA_TAG_TYPE_E egrMarvellTag,
    IN GT_U32  destVlanTagged,
    OUT GT_U8 * tagDataPtr,
    INOUT GT_U32 * tagDataLengthPtr,
    OUT GT_U32  *tag0OffsetInTagPtr,
    OUT GT_U32  *tag1OffsetInTagPtr
)
{
    DECLARE_FUNC_NAME(snetXCatHaEgressTagBuild);

    GT_U32  tagLength;
    GT_U32  dsaTagLength = 4;
    GT_U32  tmpLength;
    GT_U32  egressTag0Tpid; /* egress tag0 TPID */
    GT_U32  egressTag1Tpid; /* egress tag1 TPID */
    GT_U32  ii; /*iterator*/
    GT_U32  numTagsToAdd; /*number of tags to add*/
    GT_U32  targetPort;
    GT_U32  trgPortBpeEnable = 0;
    GT_BIT  bpeMcGroups = 0;
    GT_U32  * regPtr;
    SKERNEL_PORTS_BMP_STC trgPortsBmp; /* Target ports bitmap */

    ASSERT_PTR(tagDataPtr);
    ASSERT_PTR(tagDataLengthPtr);
    ASSERT_PTR(tag0OffsetInTagPtr);
    ASSERT_PTR(tag1OffsetInTagPtr);

    tagLength = 0;
    tmpLength = 0;

    *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
    if(devObjPtr->support802_1br_PortExtender)
    {
        smemRegFldGet(devObjPtr,
                                SMEM_XCAT3_HA_BPE_INTERNAL_CONFIG_REG (devObjPtr), 0, 1, &bpeMcGroups);

        targetPort = SMEM_CHT_LOCAL_PORT_FROM_GLOBAL_PORT_MAC (devObjPtr, descrPtr->egressPhysicalPortInfo.globalPortIndex);
                /* Get Target Port <BPE enabled> per port */
        regPtr = smemMemGet(devObjPtr, SMEM_XCAT3_HA_PER_PORT_BPE_ENABLE_REG(devObjPtr));
        SKERNEL_FILL_PORTS_BITMAP_MAC(devObjPtr, &trgPortsBmp, regPtr);
        trgPortBpeEnable = SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(&trgPortsBmp, targetPort);

    }

    /* DSA tag build */
    if(egrMarvellTag != MTAG_TYPE_NONE_E)
    {
        dsaTagLength = (egrMarvellTag + 1) * 4;
        /* skip the DSA tag */
        __LOG(("skip the DSA tag"));
        tagDataPtr += dsaTagLength;

        *tag0OffsetInTagPtr = 0;/*tag0 on the DSA tag*/


/* redo DSA switch-case -- label */
reDoDsaSwitchCase_lbl:

        switch(destVlanTagged)
        {
            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E:
                /* tag 0 is on the DSA tag */
                /* add tag 1 */
                if (trgPortBpeEnable)
                {
                    snetXCatHaEgressTagBuildEtag(devObjPtr,descrPtr,
                                      (GT_U8)descrPtr->up1,
                                      (GT_U8)descrPtr->cfidei1,
                                      (GT_U16)vlanTag1EtherType, tagDataPtr);
                    tagLength = 8;
                }
                else
                {
                    snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up1,
                                      (GT_U16)descrPtr->vid1,
                                      (GT_U8)descrPtr->cfidei1,
                                      (GT_U16)vlanTag1EtherType, tagDataPtr);
                    tagLength = 4;
                }
                *tag1OffsetInTagPtr = dsaTagLength;
                break;
            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG1_IN_TAG0_E:
                *tag1OffsetInTagPtr = 0;
                tagLength = 4;
                *tag0OffsetInTagPtr = dsaTagLength;/*tag0 after the DSA tag*/

                /* add tag 0 */
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                                  (GT_U16)descrPtr->eVid,
                                  (GT_U8)descrPtr->cfidei,
                                  (GT_U16)vlanTag0EtherType, tagDataPtr);
                break;

            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_PUSH_TAG0_E:
                if(descrPtr->marvellTagged)
                {
                    destVlanTagged = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E;
                    goto reDoDsaSwitchCase_lbl;
                }
                haPushTag0(devObjPtr,descrPtr,vlanTag0EtherType,vlanTag1EtherType,
                                egrMarvellTag,tagDataPtr,&tmpLength);

                if(descrPtr->ingressVlanTag1Type == SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E)
                {
                    *tag1OffsetInTagPtr = dsaTagLength + 4;
                }
                else if (descrPtr->ingressVlanTag1Type == SKERNEL_FRAME_TR101_VLAN_TAG_OUTER_E)
                {
                    *tag1OffsetInTagPtr = dsaTagLength;
                }

                break;
            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E:
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    *tag0OffsetInTagPtr = SMAIN_NOT_VALID_CNS;/*tag0 NOT on the DSA tag*/
                    *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;/*tag1 NOT on the DSA tag*/
                }
                else
                {
                    *tag0OffsetInTagPtr = 0;/*tag0 on the DSA tag*/
                    *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
                }
                break;
            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E:
                *tag0OffsetInTagPtr = 0;/*tag0 on the DSA tag*/
                *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
                break;
            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E:
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    *tag0OffsetInTagPtr = SMAIN_NOT_VALID_CNS;/*tag0 NOT on the DSA tag*/
                    *tag1OffsetInTagPtr = 0;/*tag1 on the DSA tag*/
                }
                else
                {
                    *tag0OffsetInTagPtr = 0;/*tag0 on the DSA tag*/
                    *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
                }
                break;

            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_POP_OUT_TAG_E:
                /* we pop the out tag , so if we had 2 tags in the ingress and
                   the inner is tag 1 , we need to use tag 1 , otherwise we keep
                   tag 0 info */
                if(descrPtr->ingressVlanTag1Type == SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E)
                {
                    *tag1OffsetInTagPtr = 0;/*tag1 on the DSA tag*/
                    *tag0OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
                }
                else
                {
                    *tag0OffsetInTagPtr = 0;/*tag0 on the DSA tag*/
                    *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
                }
                break;
            case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_DO_NOT_MODIFIED_E:
                /* ! TBD ! */

                break;
            default:
                break;
        }

        tagLength += dsaTagLength;

        *tagDataLengthPtr = tagLength + tmpLength;

        return GT_OK;
    }

/* redo switch-case -- label */
reDoSwitchCase_lbl:

    *tag0OffsetInTagPtr = SMAIN_NOT_VALID_CNS;
    *tag1OffsetInTagPtr = SMAIN_NOT_VALID_CNS;

    switch(destVlanTagged)
    {
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E:
            break;
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E:
            snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                              (GT_U16)descrPtr->eVid,
                              (GT_U8)descrPtr->cfidei,
                              (GT_U16)vlanTag0EtherType, tagDataPtr);
            tagLength = 4;
            *tag0OffsetInTagPtr = 0;
            break;
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E:
            if (trgPortBpeEnable)
            {
                snetXCatHaEgressTagBuildEtag(devObjPtr,descrPtr,
                                  (GT_U8)descrPtr->up1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)vlanTag1EtherType, tagDataPtr);
                tagLength = 8;
            }
            else
            {
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up1,
                                  (GT_U16)descrPtr->vid1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)vlanTag1EtherType, tagDataPtr);
                tagLength = 4;
            }
            *tag1OffsetInTagPtr = 0;
            break;
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E:
            *tag0OffsetInTagPtr = 0;
            *tag1OffsetInTagPtr = 4;
            snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                              (GT_U16)descrPtr->eVid,
                              (GT_U8)descrPtr->cfidei,
                              (GT_U16)vlanTag0EtherType, tagDataPtr);
            if (trgPortBpeEnable)
            {
                snetXCatHaEgressTagBuildEtag(devObjPtr,descrPtr,
                                  (GT_U8)descrPtr->up1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)vlanTag1EtherType, tagDataPtr + 4);
                tagLength = 12;
            }
            else
            {
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up1,
                                  (GT_U16)descrPtr->vid1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)vlanTag1EtherType, tagDataPtr + 4);
                tagLength = 8;
            }
            break;
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG1_IN_TAG0_E:
            *tag1OffsetInTagPtr = 0;
            if (trgPortBpeEnable)
            {
                snetXCatHaEgressTagBuildEtag(devObjPtr,descrPtr,
                                  (GT_U8)descrPtr->up1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)vlanTag1EtherType, tagDataPtr);
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                                  (GT_U16)descrPtr->eVid,
                                  (GT_U8)descrPtr->cfidei,
                                  (GT_U16)vlanTag0EtherType, tagDataPtr + 8);
                *tag0OffsetInTagPtr = 8;
                tagLength = 12;
            }
            else
            {
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up1,
                                  (GT_U16)descrPtr->vid1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)vlanTag1EtherType, tagDataPtr);
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                                  (GT_U16)descrPtr->eVid,
                                  (GT_U8)descrPtr->cfidei,
                                  (GT_U16)vlanTag0EtherType, tagDataPtr + 4);
                *tag0OffsetInTagPtr = 4;
                tagLength = 8;
            }
            break;
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_PUSH_TAG0_E:
            if(descrPtr->marvellTagged)
            {
                destVlanTagged = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E;
                goto reDoSwitchCase_lbl;
            }

            *tag0OffsetInTagPtr = 0;
            haPushTag0(devObjPtr,descrPtr,vlanTag0EtherType,vlanTag1EtherType,
                            egrMarvellTag,tagDataPtr,&tmpLength);

            switch(descrPtr->srcTagState)
            {
                case SKERNEL_FRAME_TR101_VLAN_INGR_TAG_TAG1_E:
                    *tag1OffsetInTagPtr = 4;/*tag 1 after pushed tag 0*/
                    break;
                case SKERNEL_FRAME_TR101_VLAN_INGR_TAG_OUT_TAG0_IN_TAG1_E:
                    *tag1OffsetInTagPtr = 8;/*tag 1 after pushed tag 0 and after another tag 0 */
                    break;
                case SKERNEL_FRAME_TR101_VLAN_INGR_TAG_OUT_TAG1_IN_TAG0_E:
                    *tag1OffsetInTagPtr = 4;/*tag 1 after pushed tag 0*/
                    break;
                default:
                    /*tag 1 not exist on ingress so no exists on egress*/
                    break;
            }
            break;
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_POP_OUT_TAG_E:
            if(descrPtr->marvellTagged)
            {
                destVlanTagged = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E;
                goto reDoSwitchCase_lbl;
            }

            if(devObjPtr->supportEArch == 0)
            {
                egressTag0Tpid = descrPtr->vlanEtherType;
                egressTag1Tpid = descrPtr->vlanEtherType1;
            }
            else
            {
             /* Egress Tag0/Tag1 TPIDs for ePort selected according to
                egress Tag0/Tag1 TPID Index in HA Egress ePort Attribute Table1.
                However, this requires the egress and ingress TPID tables
                to be configured identically */
                egressTag0Tpid = vlanTag0EtherType;
                egressTag1Tpid = vlanTag1EtherType;
            }

            if(descrPtr->ingressVlanTag1Type ==
               SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E)
            {
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up1,
                                  (GT_U16)descrPtr->vid1,
                                  (GT_U8)descrPtr->cfidei1,
                                  (GT_U16)egressTag1Tpid, tagDataPtr);
                tagLength = 4;
                *tag1OffsetInTagPtr = 0;
            }
            else if(descrPtr->ingressVlanTag0Type ==
                    SKERNEL_FRAME_TR101_VLAN_TAG_INNER_E)
            {
                *tag0OffsetInTagPtr = 0;
                snetChtHaEgressTagDataExtGet((GT_U8)descrPtr->up,
                                  (GT_U16)descrPtr->eVid,
                                  (GT_U8)descrPtr->cfidei,
                                  (GT_U16)egressTag0Tpid, tagDataPtr);
                tagLength = 4;
            }
            break;
        default:
            return GT_BAD_PARAM;
    }

    if(devObjPtr->errata.haBuildWrongTagsWhenNumOfTagsToPopIs2 && /* meaning : 2 tags of 4 bytes */
       descrPtr->numOfBytesToPop == 8 /* 8 bytes == 2 tags of 4 bytes */ &&
       /* 2 tags from ingress that need to be 'removed' before build egress tags */
       (TAG0_EXIST_MAC(descrPtr) && TAG1_EXIST_MAC(descrPtr)))
    {

        numTagsToAdd = 2 + (descrPtr->origVlanTagLength / 4);
        /* so the ERROR behavior is to add the original tags after those
           that should have egress due to egress tag state . */
        __LOG(("ERROR : Errata : when numOfTagsToPop == 2 the device add original tags after the expected egress ones (even though should not appear) \n"));
        __LOG(("ERROR : Errata : adding extra [%d] tags (4 bytes each) \n",
            numTagsToAdd));

        tagDataPtr = &tagDataPtr[tagLength + tmpLength];

        /* copy the tags from ingress buffer */
        for(ii = 0 ; ii < (numTagsToAdd*4) ; ii++,tagDataPtr++)
        {
            tagDataPtr[0] =
                descrPtr->origVlanTagPtr[ii];
        }

        /* calculate length of tags that ingress recognized */
        tagLength += ii;
    }

    /* the simulation do the logic at HA (unlike the HW/GM that do it in TTI)
       but we can't change full simulation logic just for this one... */
    if(descrPtr->numOfBytesToPop &&
       descrPtr->popTagsWithoutReparse)
    {
        if(descrPtr->origVlanTagLength)
        {
            /* we will pop tags that are not 'tag0' nor 'tag1' but after then ... */
            __LOG(("Warning : 'Popping tags' which are not 'most outer' but after tag0,1 (because TTI unit did not do 'L2 re-parse' due to no TTI action) \n"));
        }

        /* the TTI did not do re-parse of tag recognition , but still HA need
           to 'jump' to skip some tags */
        if(descrPtr->afterVlanOrDsaTagLen >= descrPtr->numOfBytesToPop)
        {
            __LOG(("pop [%d] bytes due to <numOfBytesToPop> \n",
                descrPtr->numOfBytesToPop));
            descrPtr->afterVlanOrDsaTagLen -= descrPtr->numOfBytesToPop;
            descrPtr->afterVlanOrDsaTagPtr += descrPtr->numOfBytesToPop;
            __LOG_PARAM(descrPtr->afterVlanOrDsaTagLen);
        }
        else
        {
            /* this case seems like simulation bug ?! */
            __LOG(("ERROR: try to pop tag [%d] but 'no more' tags to pop \n"));
        }
    }

    *tagDataLengthPtr = tagLength + tmpLength;


    return GT_OK;
}

/*******************************************************************************
*   snetLion3EgfShtPortIsolationCmdGet
*
* DESCRIPTION:
*        SIP5 :
*        Get the port isolation mode.
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        egressPort      - egress port.
*
* OUTPUTS:
*        None
*
* RETURNS:
*        the port isolation command :
*         0x0 = Disabled;
*         0x1 = L2PortIsoEn;
*         0x2 = L3PortIsoEn;
*         0x3 = AllPortIsoEn;
*
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 snetLion3EgfShtPortIsolationCmdGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32                   egressPort
)
{
    GT_U32 regAddress;  /* Register's address */
    GT_U32 * regPtr ;   /* registers  pointer */
    GT_U32 portIsolationMode;/*Port Isolation Mode*/

    if(descrPtr->useVidx == 0)
    {
        snetChtEgfEgressEPortEntryGet(devObjPtr,descrPtr,egressPort/*global port*/);

        if(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr)
        {
            portIsolationMode = snetFieldValueGet(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr,5,2);

            goto done_lbl;
        }
    }

    regAddress = SMEM_LION3_EGF_SHT_EPORT_PORT_ISOLATION_MODE_REG(devObjPtr);
    /* 2 bits per port */
    regPtr = smemMemGet(devObjPtr, regAddress);
    portIsolationMode = snetFieldValueGet(regPtr, egressPort*2, 2);

done_lbl:

    return portIsolationMode;
}

/*******************************************************************************
*   snetXCatTxQPortIsolationFilters
*
* DESCRIPTION:
*        Port isolation filtering
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        destPorts       - number of egress port.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetXCatTxQPortIsolationFilters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    INOUT GT_U32 destPorts[]
)
{
    DECLARE_FUNC_NAME(snetXCatTxQPortIsolationFilters);

    GT_U32 regAddr;             /* Register address */
    GT_U32 * regPtr;            /* Register pointer */
    GT_U32 fieldVal;            /* Register field value */
    GT_U32 entryIndex = 0;      /* Table entry index */
    GT_U32 port;                /* Port number */
    GT_U32 srcPort;             /* Source port */
    GT_U32 cpuBit;              /* CPU bit index */
    GT_U32 startPort;           /* Start iteration port */
    GT_U32 endPort;             /* End iteration port */
    GT_U32 localPort;           /* local Port number */
    GT_BIT origIsTrunk;         /* Packet's origin - trunk/port */
    GT_BIT isL3Traffic;         /* is packet routed for MPLS switched */
    GT_U32 portNumBits,devNumBits;/* number of bits for port,device in building index to access the table*/
    GT_U32 trunkIdNumBits;/* number of bits for trunkId in building index to access the table*/
    GT_U32 trunkIndexBase;/* trunk index to start access in the table */
    GT_U32 portIsolationVlanCmd;/* port isolation command : disable/L2/L3/L2&L3*/
    SKERNEL_PORTS_BMP_STC l2IsolationPortBmp; /* L2 isolation ports bitmap */
    SKERNEL_PORTS_BMP_STC l3IsolationPortBmp; /* L3 isolation ports bitmap */
    SKERNEL_PORTS_BMP_STC *isolationPortBmpPtr; /* pointer to active isolation ports bitmap */

    startPort = SNET_CHT_EGR_TXQ_START_PORT_MAC(devObjPtr);
    endPort = SNET_CHT_EGR_TXQ_END_PORT_MAC(devObjPtr);

    /* Port Isolation is not applied to TO_CPU, FROM_CPU, and TO_ANALYZER packets */
    if(descrPtr->outGoingMtagCmd != SKERNEL_MTAG_CMD_FORWARD_E)
    {
        return;
    }

    if(devObjPtr->txqRevision != 0)
    {
        regAddr = SMEM_LION_TXQ_EGR_FILTER_GLOBAL_EN_REG(devObjPtr);
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            smemRegGet(devObjPtr, regAddr, &fieldVal);

            /* check the 'filter Enable' and the 'global filter enable' */
            if(SMEM_U32_GET_FIELD(fieldVal,0,1) &&
               SMEM_U32_GET_FIELD(fieldVal,6,1) )
            {
                fieldVal = 1;
            }
            else
            {
                fieldVal = 0;
            }
        }
        else
        {
            smemRegFldGet(devObjPtr, regAddr, 7, 1, &fieldVal);
        }

        if(fieldVal == 0)
        {
            /* Port isolation disabled */
            __LOG(("Port isolation disabled"));
            return;
        }
    }
    else
    {
        regAddr = SMEM_XCAT_TXQ_RESOURCE_SHARE_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 28, 1, &fieldVal);
        if(fieldVal == 0)
        {
            /* Port isolation disabled */
            __LOG(("Port isolation globally disabled"));
            return;
        }
    }

    if(descrPtr->egressVlanInfo.portIsolationVlanCmd == 0)
    {
        __LOG(("Port isolation L2/L3 disabled for the egress vlan[0x%3.3x]",
                descrPtr->eVid));
        return;
    }

    origIsTrunk = descrPtr->origIsTrunk;
    if(devObjPtr->errata.srcTrunkPortIsolationAsSrcPort)
    {
        /* When packet received from local trunk with no DSA tag,
        it will access port isolation tables according to <src port, src dev> */
        origIsTrunk = (descrPtr->marvellTagged == 0) ? 0 : origIsTrunk;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        srcPort = descrPtr->localPortGroupPortAsGlobalDevicePort;/* global port */

        regAddr = SMEM_LION3_EGF_EFT_PORT_ISOLATION_LOOKUP_0_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);

        devNumBits     = snetFieldValueGet(regPtr, 25, 4);
        trunkIdNumBits = snetFieldValueGet(regPtr, 21, 4);
        trunkIndexBase = snetFieldValueGet(regPtr, 0, 21);

        regAddr = SMEM_LION3_EGF_EFT_PORT_ISOLATION_LOOKUP_1_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* Port Isolation on ePorts Enable */
        if(1 == snetFieldValueGet(regPtr, 0, 1))
        {
            /* port isolation is performed based on the source ePort */
            srcPort = descrPtr->eArchExtInfo.localDevSrcEPort;
            __LOG(("port isolation is performed based on the source ePort[0x%4.4x]",
                srcPort));
        }

        portNumBits = snetFieldValueGet(regPtr, 1, 5);
    }
    else
    {
        srcPort = descrPtr->localDevSrcPort;

        devNumBits  = devObjPtr->flexFieldNumBitsSupport.hwDevNum;
        portNumBits = devObjPtr->flexFieldNumBitsSupport.phyPort;
        trunkIdNumBits = devObjPtr->flexFieldNumBitsSupport.trunkId;
        trunkIndexBase = 2048;
    }

    if(descrPtr->marvellTagged)
    {
        srcPort = descrPtr->origSrcEPortOrTrnk;
    }

    /* Packet originated on trunk (on local/remote device) */
    if(origIsTrunk)
    {
        /* the trunk entries starts at the 'trunk base index'  */
        __LOG(("the trunk entries starts at the 'trunk base index'"));
        entryIndex = SMEM_U32_GET_FIELD(descrPtr->origSrcEPortOrTrnk,0,trunkIdNumBits) +
                    trunkIndexBase;
    }
    /* Packet received on local device and port */
    else
    {
        /* build the index */
        /* set the srcPort section */
        SMEM_U32_SET_FIELD(entryIndex,0,portNumBits,srcPort);
        /* set the srcDev section */
        SMEM_U32_SET_FIELD(entryIndex,portNumBits,devNumBits,descrPtr->srcDev);

        if(entryIndex > (127+0x800)/*2K+127*/)
        {
            __LOG(("{dev,port} avoid access to indexes > [%d] values , so 'modulo' from [%d] \n",
                (127+0x800),entryIndex));
        }
        entryIndex %= (127+0x800);
    }

    /* check MPLS LSR case or IP Routed one */
    isL3Traffic = descrPtr->isMplsLsr ? 1 : 0;
    isL3Traffic |= descrPtr->routed;

    /* access L3 Port Isolation table */
    regAddr = SMEM_XCAT_L3_PORT_ISOLATION_REG(devObjPtr, entryIndex);
    regPtr = smemMemGet(devObjPtr, regAddr);
    SKERNEL_FILL_PORTS_BITMAP_MAC(devObjPtr, &l3IsolationPortBmp, regPtr);

    /* access L2 Port Isolation table */
    regAddr = SMEM_XCAT_L2_PORT_ISOLATION_REG(devObjPtr, entryIndex);
    regPtr = smemMemGet(devObjPtr, regAddr);
    SKERNEL_FILL_PORTS_BITMAP_MAC(devObjPtr, &l2IsolationPortBmp, regPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        for (port = startPort,localPort = 0; port < endPort; port++ , localPort++)
        {
            if(destPorts[localPort] == 0)
            {
                /* no need to check port that is not in the 'dest list'*/
                continue;
            }

            portIsolationVlanCmd =
                snetLion3EgfShtPortIsolationCmdGet(devObjPtr,descrPtr,port);

            /* the filter must be enabled on both 'per port' and 'per vlan' */
            portIsolationVlanCmd &= descrPtr->egressVlanInfo.portIsolationVlanCmd;

            if(isL3Traffic && (portIsolationVlanCmd & 2))
            {
                /* traffic is L3 , and Enabled L3 Port Isolation */
                isolationPortBmpPtr = &l3IsolationPortBmp;
            }
            else if ((!isL3Traffic) && (portIsolationVlanCmd & 1))
            {
                /* traffic is not L3 , and Enabled L2 Port Isolation */
                isolationPortBmpPtr = &l2IsolationPortBmp;
            }
            else
            {
                continue;
            }

            if(0 == SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(isolationPortBmpPtr, port))
            {
                __LOG(("port [%d] filtered due to L2/L3 port isolation \n",port));
            }

            destPorts[localPort] &=
                SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(isolationPortBmpPtr, port);
        }
    }
    else
    {
        if(isL3Traffic && (descrPtr->egressVlanInfo.portIsolationVlanCmd & 2))
        {
            /* traffic is L3 , and Enabled L3 Port Isolation */
            isolationPortBmpPtr = &l3IsolationPortBmp;
        }
        else if ((!isL3Traffic) && (descrPtr->egressVlanInfo.portIsolationVlanCmd & 1))
        {
            /* traffic is not L3 , and Enabled L2 Port Isolation */
            isolationPortBmpPtr = &l2IsolationPortBmp;
        }
        else
        {
            return;
        }

        for (port = startPort,localPort = 0; port < endPort; port++ , localPort++)
        {
            if(0 == SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(isolationPortBmpPtr, port))
            {
                __LOG(("port [%d] filtered due to L2/L3 port isolation \n",port));
            }
            /* Port with port isolation index <entryIndex> could be filtered */
            destPorts[localPort] &=
                SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(isolationPortBmpPtr, port);
        }

        if(devObjPtr->txqRevision == 0)
        {
            /* xcat / Lion A */
            cpuBit = 28;
        }
        else
        {
            cpuBit = SNET_CHT_CPU_PORT_CNS;
        }

        if(0 == SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(isolationPortBmpPtr, cpuBit))
        {
            __LOG(("CPU port filtered due to L2/L3 port isolation \n"));
        }

        destPorts[SNET_CHT_CPU_PORT_CNS] &=
            SKERNEL_PORTS_BMP_IS_PORT_SET_MAC(isolationPortBmpPtr, cpuBit);
    }

}

/*******************************************************************************
*  snetXCatEqSniffFromRemoteDevice
*
* DESCRIPTION:
*       Forwarding TO_ANALYZER frames to the Rx/Tx Sniffer.
*
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       rxSniff     - RX/TX mirroring direction
*
* RETURN:
*
* COMMENTS :
*       Supports two modes of forwarding for TO_ANALYZER packets:
*           Hop-by-hop forwarding — in this mode each hop performs a forwarding
*                                   decision based on its local configuration.
*           Source-based forwarding — in this mode the device that triggers the
*                                   mirroring determines the analyzer device and port,
*                                   and as the packet is forwarded to other devices,
*                                   forwarding is based on the decision of the first device.
*                                   In this mode the TO_ANALYZER DSA tag includes the
*                                   analyzer device and port.
*
*******************************************************************************/
GT_VOID snetXCatEqSniffFromRemoteDevice
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL rxSniff
)
{
    GT_U32 trgSniffDev, trgSniffPort;   /* Rx sniffer device an port */
    GT_U32 analyzQosCfgRegData;         /* Ingress and Egress Monitoring to
                                           Analyzer QoS Configuration Register */

    /* Ingress and Egress Monitoring to Analyzer QoS Configuration Register */
    smemRegGet(devObjPtr, SMEM_CHT_INGR_EGR_MON_TO_ANALYZER_QOS_CONF_REG(devObjPtr),
               &analyzQosCfgRegData);

    if(descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_HOP_BY_HOP_E)
    {
        /* Hop-by-hop forwarding mode (Cheetah backwards compatible) */
        skernelFatalError("the hop by hop have no special treatment then on local device .. do not call this function \n");
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Source-based forwarding mode. Port+Device of the analyzer from the DSA tag */
        trgSniffDev     = descrPtr->trgDev;
        trgSniffPort    = descrPtr->eArchExtInfo.trgPhyPort;
    }
    else
    {
        /* Source-based forwarding mode. Port+Device of the analyzer from the DSA tag */
        trgSniffDev = descrPtr->srcDev;
        trgSniffPort = descrPtr->origSrcEPortOrTrnk;
    }

    /* Ingress Analyzer TC */
    descrPtr->tc = SMEM_U32_GET_FIELD(analyzQosCfgRegData, 7, 3);
    /* Ingress Analyzer DP */
    descrPtr->dp = SMEM_U32_GET_FIELD(analyzQosCfgRegData, 5, 2);

    /* Ingress/Egress analyzer */
    descrPtr->rxSniff = (rxSniff) ? 1 : 0;

    /* Send packet to Ingress/Egress analyzer */
    snetChtEqDoTargetSniff(devObjPtr, descrPtr, trgSniffDev, trgSniffPort);
}

/*******************************************************************************
*  snetXCatFdbSrcIdAssign
*
* DESCRIPTION:
*       Source-ID Assignment
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - frame data buffer Id
*       saLookupInfoPtr - pointer to SA lookup info structure
*       daLookupInfoPtr - pointer to DA lookup info structure
*
* RETURN:
*       sstId - FDB based SrcID (value of SMAIN_NOT_VALID_CNS means 'no assignment')
*
* COMMENTS :
*
*******************************************************************************/
GT_U32 snetXCatFdbSrcIdAssign
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH_L2I_SA_LOOKUP_INFO * saLookupInfoPtr,
    IN SNET_CHEETAH_L2I_DA_LOOKUP_INFO * daLookupInfoPtr
)
{
    GT_U32 fldValue;
    GT_U32 fdbBasedSrcID = SMAIN_NOT_VALID_CNS;

    /* FDBbasedSrcID Assign Mode */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* this is the L2I register .
           NOTE that the FDB unit also hold those bits in 17,18 in
           SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG
        */
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr),
                      4, 2, &fldValue);
    }
    else
    {
        smemRegFldGet(devObjPtr, SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr),
                      13, 2, &fldValue);
    }

    switch(fldValue)
    {
        case 1:
            if(daLookupInfoPtr->found)
            {
                /* FDB source ID assignment is enabled for DA-based assignment  */
                fdbBasedSrcID = daLookupInfoPtr->sstId;
            }
            break;
        case 2:
            /* check that SA was not moved. The HW do not assigns source ID for moved SA entries.
               The functional spec does not have such notes. This checked on HW. */
            if(saLookupInfoPtr->found && (saLookupInfoPtr->isMoved == 0))
            {
                /* FDB source ID assignment is enabled for SA-based assignment */
                fdbBasedSrcID = saLookupInfoPtr->sstId;
            }
            break;
        case 0:
            /* FDB source ID assignment is disabled */
        default:
            break; /* no override because sstId already have the needed value */
    }

    return fdbBasedSrcID;
}

/*******************************************************************************
* snetXCatLogicalTargetTblGet
*
* DESCRIPTION:
*        Get Logical Target Mapping Table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*
* OUTPUTS:
*       logTargetDataPtr   - pointer to the logical target mapping table data
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetXCatLogicalTargetTblGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    OUT SNET_XCAT_LOGICAL_PORT_MAPPING_STC  * logTargetDataPtr
)
{
    DECLARE_FUNC_NAME(snetXCatLogicalTargetTblGet);

    GT_U32 *regPtr;                         /* register pointer */
    GT_U32 fieldVal;                        /* register field value */
    GT_U32 regAddr;                         /* register address */
    GT_U32 enable = 1;

    /* Set init value */
    SNET_SET_DEST_INTERFACE_TYPE(logTargetDataPtr->egressIf,
                                 SNET_DST_INTERFACE_PORT_E, enable);

    if(devObjPtr->supportLogicalMapTableInfo.supportFullRange )
    {
        /* Logical Target Mapping Table */
        __LOG(("Logical Target Mapping Table"));
        regAddr = SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_MEM(devObjPtr,
                                                        descrPtr->trgDev,
                                                        descrPtr->trgEPort);
    }
    else
    {
        /* Logical Target Mapping Table */
        __LOG(("Logical Target Mapping Table"));
        regAddr = SMEM_XCAT_LOGICAL_TRG_MAPPING_TBL_MEM(devObjPtr,
                                                       (descrPtr->trgDev - 24),
                                                        descrPtr->trgEPort);
    }
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Use VIDX  */
    __LOG(("Use VIDX"));
    fieldVal = SMEM_U32_GET_FIELD(*regPtr, 0, 1);

    SNET_SET_DEST_INTERFACE_TYPE(logTargetDataPtr->egressIf,
                                 SNET_DST_INTERFACE_VIDX_E, fieldVal);
    if (SNET_GET_DEST_INTERFACE_TYPE(logTargetDataPtr->egressIf,
                                 SNET_DST_INTERFACE_VIDX_E))
    {
        logTargetDataPtr->egressIf.interfaceInfo.vidx =
            (GT_U16) SMEM_U32_GET_FIELD(*regPtr, 1, 12);
    }
    else
    {
        /* Target Is Trunk */
        fieldVal = SMEM_U32_GET_FIELD(*regPtr, 1, 1);
        SNET_SET_DEST_INTERFACE_TYPE(logTargetDataPtr->egressIf,
                                 SNET_DST_INTERFACE_TRUNK_E, fieldVal);

        if (SNET_GET_DEST_INTERFACE_TYPE(logTargetDataPtr->egressIf,
                                     SNET_DST_INTERFACE_TRUNK_E))
        {
            logTargetDataPtr->egressIf.interfaceInfo.trunkId =
                (GT_U8)SMEM_U32_GET_FIELD(*regPtr, 6, 7);
        }
        else
        {

            logTargetDataPtr->egressIf.dstInterface = SNET_DST_INTERFACE_PORT_E;
            logTargetDataPtr->egressIf.interfaceInfo.devPort.port =
                (GT_U8)SMEM_U32_GET_FIELD(*regPtr, 2, 6);
            logTargetDataPtr->egressIf.interfaceInfo.devPort.devNum =
                (GT_U8)SMEM_U32_GET_FIELD(*regPtr, 8, 5);
        }
    }

    /* Tunnel Start */
    logTargetDataPtr->tunnelStart = SMEM_U32_GET_FIELD(*regPtr, 13, 1);
    /* Tunnel Pointer */
    logTargetDataPtr->tunnelPtr = SMEM_U32_GET_FIELD(*regPtr, 14, 12);
    /* Tunnel Start Passenger Type  */
    logTargetDataPtr->tunnelPassengerType = SMEM_U32_GET_FIELD(*regPtr, 26, 1);


    if(devObjPtr->supportLogicalMapTableInfo.tableFormatVersion == 0)
    {
        return;
    }

    logTargetDataPtr->egressPassangerTagTpidIndex = snetFieldValueGet(regPtr,24,2);
    /* <egressPassangerTagTpidIndex> is instead of bit 24..25 that are stolen from the "Tunnel Pointer" */
    SMEM_U32_SET_FIELD(logTargetDataPtr->tunnelPtr,10, 2,0);

    logTargetDataPtr->assignVid0Command = snetFieldValueGet(regPtr,32,1);
    logTargetDataPtr->egressVlanFilteringEnable = snetFieldValueGet(regPtr,27,1);
    logTargetDataPtr->assignedEgressTagStateOnlyIfUnassigned = snetFieldValueGet(regPtr,28,1);
    logTargetDataPtr->egressTagState = snetFieldValueGet(regPtr,29,3);
    logTargetDataPtr->vid0 = snetFieldValueGet(regPtr,33,12);


}

/*******************************************************************************
* snetXCatLogicalTargetTblApply
*
* DESCRIPTION:
*        Apply data entry from Logical Target Mapping Table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       logTargetDataPtr
*                       - pointer to the logical target mapping table data
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetXCatLogicalTargetTblApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr,
    IN SNET_XCAT_LOGICAL_PORT_MAPPING_STC  * logTargetDataPtr
)
{
    DECLARE_FUNC_NAME(snetXCatLogicalTargetTblApply);

    GT_U32  value;/*register value*/

    /* Apply data from destination interface */
    SNET_APPLY_DESTINATION_INTERFACE_MAC(logTargetDataPtr->egressIf, descrPtr);

    /* Indication for Tunnel Start */
    __LOG(("Indication for Tunnel Start[%d]",
                  logTargetDataPtr->tunnelStart));

    /*always take the logical port table entry <Tunnel Start>
        (regardless of whether it is 0 or 1)  and
        override the incoming descriptor<Tunnel Start> value */
    descrPtr->tunnelStart = logTargetDataPtr->tunnelStart;

    if (logTargetDataPtr->tunnelStart)
    {
        descrPtr->tunnelPtr = logTargetDataPtr->tunnelPtr;
        descrPtr->tunnelStartPassengerType = logTargetDataPtr->tunnelPassengerType;
    }

    if(devObjPtr->supportLogicalMapTableInfo.tableFormatVersion == 0)
    {
        return;
    }

    if(logTargetDataPtr->egressVlanFilteringEnable &&
       descrPtr->vplsInfo.targetLogicalPortIsNotVlanMember == 1)
    {
        /* we drop this packet */
        __LOG(("Logical Port Egress VLAN Filtering : assign SOFT DROP"));
        /*assigned a SOFT_DROP packet*/
        snetChtIngressCommandAndCpuCodeResolution(devObjPtr,descrPtr,
                                                  descrPtr->packetCmd,
                                                  SKERNEL_EXT_PKT_CMD_SOFT_DROP_E,
                                                  descrPtr->cpuCode,
                                                  0,/*don't care*/
                                                  SNET_CHEETAH_ENGINE_UNIT_EQ_E,
                                                  GT_TRUE);


        /* Logical Port Egress VLAN Filtering Drop Counter */
        smemRegGet(devObjPtr, SMEM_XCAT_EQ_VLAN_EGRESS_FILTER_COUNTER_REG(devObjPtr) , &value);
        value++;/*value of the counter*/
        smemRegSet(devObjPtr, SMEM_XCAT_EQ_VLAN_EGRESS_FILTER_COUNTER_REG(devObjPtr) , value);
    }

    /* update the descriptor only when <VPLS Mode>==1 */

    if(devObjPtr->vplsModeEnable.eq == 0)
    {
        return;
    }

    /*
    0 = Do not override VID0 assignment
    1 = Always override VID0 assignment
    */
    if(logTargetDataPtr->assignVid0Command == 1) /*egress on PW and need to push/swap VID0*/
    {
        descrPtr->eVid = logTargetDataPtr->vid0;
    }
    else if (descrPtr->overrideVid0WithOrigVid) /*arrived on PW and need to push/Swap VID0*/
    {
        descrPtr->eVid = descrPtr->vid0Or1AfterTti;
    }

    if(logTargetDataPtr->assignedEgressTagStateOnlyIfUnassigned == 0 || /* assign to all */
       descrPtr->vplsInfo.egressTagStateAssigned == 0)/* assign only to those that not assigned */
    {
        /* flag internal for the simulation to remove the use of value '7' from the egressTagState */
        if(logTargetDataPtr->egressTagState == 7)
        {
            descrPtr->vplsInfo.egressTagStateAssigned = 0;
        }
        else
        {
            descrPtr->vplsInfo.egressTagStateAssigned = 1;
        }

        descrPtr->vplsInfo.egressTagState = logTargetDataPtr->egressTagState;
    }

    descrPtr->egressPassangerTagTpidIndex = logTargetDataPtr->egressPassangerTagTpidIndex;

}

/*******************************************************************************
*   snetXCatLogicalTargetMapping
*
* DESCRIPTION:
*       The device supports a generic mechanism that maps a packets
*       logical target to an actual egress interface
*
*       The logical target can me mapped to any of the following new targets:
*           - Single-target {Device, Port}
*           - Single-Target {Device, Port} + Tunnel-Start Pointer
*           - Trunk-ID
*           - Multi-target {VIDX}
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetXCatLogicalTargetMapping
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(snetXCatLogicalTargetMapping);

    GT_U32 *regPtr;                         /* register pointer */
    GT_U32 regAddr;                         /* register address */
    GT_U32 fieldVal;                        /* register field value */
    SNET_XCAT_LOGICAL_PORT_MAPPING_STC  logTargetData; /* logical target mapping table data */
    GT_U32  bitIndex;

    /* Check that target is dev/port but not VIDX or trunk */
    if (descrPtr->useVidx || descrPtr->targetIsTrunk)
    {
        __LOG(("no logical target mapping for trunk or VIDX \n"));
        return;
    }

    regPtr = smemMemGet(devObjPtr, SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr));
    /* Logical Target Mapping Global Enable */
    fieldVal = SMEM_U32_GET_FIELD(*regPtr, 13, 1);
    if(fieldVal == 0)
    {
        __LOG(("no logical target mapping : global disabled \n"));
        return;
    }

    if(devObjPtr->supportLogicalMapTableInfo.supportFullRange )
    {
        if(descrPtr->trgDev > 31 ||   /* device range */
           descrPtr->trgEPort > 63)   /* port range */
        {
            __LOG(("not access the table : the trgDev[%d] > 31 trgEPort[%d] > 63 \n",
                descrPtr->trgDev,
                descrPtr->trgEPort));
            return;
        }
    }
    else
    {
        /* Triggering the Logical Target Mapping Table */
        if((descrPtr->trgDev < 24) || (descrPtr->trgDev > 31) || /* device range */
           (descrPtr->trgEPort > 63))                            /* port range */
        {
            __LOG(("not access the table : the trgDev[%d] > 31 or < 24 or trgEPort[%d] > 63 \n",
                descrPtr->trgDev,
                descrPtr->trgEPort));
            return;
        }
    }

    bitIndex = descrPtr->trgDev >= 24 ?
               (descrPtr->trgDev - 24) : /* bits 0..7*/
               (8 + descrPtr->trgDev);   /* bits 8..31*/

    regAddr = SMEM_XCAT_LOGICAL_TRG_DEV_MAPPING_CONF_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Enable a descrPtr->trgDev to be mapped by the Logical Target Mapping table */
    fieldVal = SMEM_U32_GET_FIELD(*regPtr, bitIndex, 1);
    if(fieldVal == 0)
    {
        return;
    }

    memset(&logTargetData,0,sizeof(logTargetData));

    /* Accessing the Logical Target Mapping Table */
    snetXCatLogicalTargetTblGet(devObjPtr, descrPtr, &logTargetData);
    /* Apply Logical Target Mapping Table data */
    snetXCatLogicalTargetTblApply(devObjPtr, descrPtr, &logTargetData);
}

/*******************************************************************************
*   mirrorAnalyzerIndexSelect
*
* DESCRIPTION:
*       The device supports multiple analyzers.
*       support resolution of analyzer index.
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        newAnalyzerIndex - new analyzer index
*        fromIngress     - from ingress/egress
*                          GT_TRUE - from ingress
*                          GT_FALSE - from egress
* RETURNS:
*
*******************************************************************************/
static GT_VOID mirrorAnalyzerIndexSelect
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr ,
    IN GT_U32                   newAnalyzerIndex,
    IN GT_BOOL                  fromIngress
)
{
    DECLARE_FUNC_NAME(mirrorAnalyzerIndexSelect);
    GT_U32 origIndex;

    if(devObjPtr->supportMultiAnalyzerMirroring == 0)
    {
        /* Does not support multiple analyzer feature */
        return;
    }

    origIndex = descrPtr->analyzerIndex;

    descrPtr->analyzerIndex = MAX(origIndex,newAnalyzerIndex);

    if(origIndex != descrPtr->analyzerIndex)
    {
        __LOG(("new %s analyzer index selected [%d] (was [%d]) \n",
            fromIngress ? "ingress" : "egress" ,
            newAnalyzerIndex,
            origIndex));
    }

}

/*******************************************************************************
*   snetXcatIngressMirrorAnalyzerIndexSelect
*
* DESCRIPTION:
*       The device supports multiple analyzers. If a packet is mirrored by
*       both the port-based ingress mirroring mechanism, and one of the other
*       ingress mirroring mechanisms, the selected analyzer
*       is the one with the higher index in the analyzer table
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        newAnalyzerIndex - new analyzer index
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetXcatIngressMirrorAnalyzerIndexSelect
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr ,
    IN GT_U32                   newAnalyzerIndex
)
{
    mirrorAnalyzerIndexSelect(devObjPtr,descrPtr,newAnalyzerIndex,GT_TRUE);
}

/*******************************************************************************
*   snetXcatEgressMirrorAnalyzerIndexSelect
*
* DESCRIPTION:
*       The device supports multiple analyzers. support Egress selection
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        newAnalyzerIndex - new analyzer index
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetXcatEgressMirrorAnalyzerIndexSelect
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr ,
    IN GT_U32                   newAnalyzerIndex
)
{
    mirrorAnalyzerIndexSelect(devObjPtr,descrPtr,newAnalyzerIndex,GT_FALSE);
}


