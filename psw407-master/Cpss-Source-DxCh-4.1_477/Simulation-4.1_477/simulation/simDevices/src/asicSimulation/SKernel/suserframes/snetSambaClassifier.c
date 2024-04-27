/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSambaPolicy.c
*
* DESCRIPTION:
*       classifier Engine of Samba
*
*       for applying actions on the descriptor (that will be of the frame)
*
*       implementation according to REG_SIM_POLICY_SAMBA_FRDTD document
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SLog/simLog.h>


/* Cyclic shift left of dword x on ch positions */
#define SIMULATION_CSH32(x, ch) ( ((x) << (ch)) | ((x) >> (32-(ch))) )

/* Flips byte x*/
#define SIMULATION_FLIP8(x) \
    (  (((x)>>7) & 0x1)  | (((x)>>5) & 0x02) | (((x)>>3) & 0x04)| \
       (((x)>>1) & 0x08) | (((x)<<1) & 0x10) | (((x)<<3) & 0x20)| \
       (((x)<<5) & 0x40) | (((x)<<7) & 0x80) )

/* Flips word x*/
#define SIMULATION_FLIP32(x) \
    ( (SIMULATION_FLIP8(x & 0xFF) << 24) | (SIMULATION_FLIP8((x>>8) & 0xFF) << 16) | \
      (SIMULATION_FLIP8((x>>16) & 0xFF) << 8) | (SIMULATION_FLIP8((x>>24) & 0xFF) ) )


typedef enum {
    GFC_TABLE,
    COS_TABLE,
    DEFAULT_TABLE,
}CLASSIFIER_TABLE_ENT;


static GT_U32 snetPolicyCrc8Bytes(
    IN GT_U32 data[2],
    IN GT_U32 initCrc
);

extern GT_U32 snetUtilGetContinuesValue(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 startAddress ,/* need to be register address %4 == 0 */
    IN GT_U32 sizeofValue,/* how many bits this continues field is */
    IN GT_U32 indexOfField/* what is the index of the field */
)
{
    /* the field bits that can be spread on 2 registers max */
    GT_U32  tmpAddress;
    GT_U32  regVal[2];
    GT_U32  mask[2],offset[2];
    GT_U32  indexOfStartingBit;
    GT_U32  baseMask= (1<<sizeofValue) - 1 ;
    GT_U32  finalInfo;

    if(sizeofValue > 32)
    {
        skernelFatalError(" snetUtilGetContinuesValue: not supported sizeofValue > 32");
    }


    indexOfStartingBit = sizeofValue * indexOfField;

    tmpAddress = startAddress + (indexOfStartingBit/32)*4;

    offset[0] = indexOfStartingBit%32;
    mask[0]   = baseMask << offset[0];

    smemRegGet(devObjPtr,tmpAddress,&regVal[0]);

    finalInfo = 0;

    if((offset[0] + sizeofValue) > 32)
    {
        /* numExtraBits hods the number of bits need to be read from the next
           register */
        GT_U32  numExtraBits = (sizeofValue + offset[0]) - 32;

        offset[1] = sizeofValue - numExtraBits ;
        /* mask[1] has the first numExtraBits bits on */
        mask[1] = ((1<<numExtraBits)-1);
        smemRegGet(devObjPtr,tmpAddress+4,&regVal[1]);

        finalInfo |= (regVal[1] & mask[1]) << offset[1];
    }

    finalInfo |= (regVal[0]&mask[0]) >> offset[0];

    return finalInfo;

}


/*******************************************************************************
*   snetSambaClassificationCheck
*
* DESCRIPTION:
*        check if need to enter the classifier
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*  Return: GT_TRUE if need to enter the classifier
*
*COMMENTS: Logic From 12.2  [1]
*******************************************************************************/
extern GT_BOOL snetSambaClassificationCheck
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    if (descrPtr->doClassify == 0 ||
        descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E ||
        descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
        return GT_FALSE;
    }


    return GT_TRUE;
}


/*******************************************************************************
*   snetSambaClassificationKeyCreate
*
* DESCRIPTION:
*        create key for generic flow classification
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        searchKeyPtr - pointer to the search key .
*
* COMMENT:
*        from 12.5.1.1 [1]
*******************************************************************************/
extern GT_VOID snetSambaClassificationKeyCreate
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS]
)
{
    GT_U32 frameOffsetsNum , offset ; /* number of offsets for key will got from
                                        frame */
    SNET_TWIST_TEMPLATE_STC template; /* template for frame */
    GT_U32 enhancedIncludeFlowTemplate ; /* do we need samba extra info
                                                    on the key */
    GT_U32 l4HeaderSize;  /* Size of TCP/UDP header */
    GT_U32 includeInlifOrVidMode ;/* include inlif/vid mode (at end of key or
                                     in bytes 15,16 [14,15])*/

    memset(searchKeyPtr,0,SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS);
    snetTwistGetFlowTemplate(devObjPtr,descrPtr,&template);

    smemRegFldGet(devObjPtr, INBLOCK_CONTROL_REG, 24, 1,
        &includeInlifOrVidMode);

    if(devObjPtr->deviceFamily != SKERNEL_SAMBA_FAMILY)
    {
        template.includeVid = 0;
        includeInlifOrVidMode = 0;
    }

    frameOffsetsNum = template.keySize ;/* already include the inlif/templateId */

    if (frameOffsetsNum > SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS)
    {
        skernelFatalError(" snetSambaClassificationKeyCreate:"\
            " frameOffsetsNum[%d] over limit[%d] ",
            frameOffsetsNum,SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS);
    }

    if(template.includeLif ==1 && template.includeVid == 1)
    {
        skernelFatalError(" snetSambaClassificationKeyCreate: "\
            " template : includeLif cant be with includeVid ");
    }

    if((includeInlifOrVidMode == 0) &&
       (template.includeLif ==1 || template.includeVid == 1))
    {
        frameOffsetsNum -= 2;
    }

    if(template.incFlowTemp == 1)
    {
        frameOffsetsNum --;
    }

    for(offset = 0; offset < frameOffsetsNum; offset++)
    {
        if (template.offsetValid[offset])
        {
            if (template.offsetType[offset] == SKERNEL_PCL_ABS_E ||
                template.offsetType[offset] == SKERNEL_PCL_MAC_HEADER_E ||
                template.offsetType[offset] == SKERNEL_PCL_MPLS_HEADER_E)
            {
                searchKeyPtr[offset] = *(descrPtr->dstMacPtr +
                    template.offsetValue[offset]);
            }
            else if (template.offsetType[offset] == SKERNEL_PCL_L3_HEADER_E)
            {
                if (descrPtr->ipHeaderPtr == NULL)
                {
                    continue;
                }
                searchKeyPtr[offset] = *(descrPtr->ipHeaderPtr +
                    template.offsetValue[offset]);
            }
            else if (template.offsetType[offset] == SKERNEL_PCL_L4_HEADER_E)
            {
                if (descrPtr->ipHeaderPtr == NULL || descrPtr->l4HeaderPtr == NULL)
                {
                    continue;
                }
                searchKeyPtr[offset] = *(descrPtr->l4HeaderPtr +
                    template.offsetValue[offset]);
            }
            else if (template.offsetType[offset] == SKERNEL_PCL_L5_HEADER_E)
            {
                if (descrPtr->ipHeaderPtr == NULL || descrPtr->l4HeaderPtr == NULL)
                {
                    continue;
                }

                if (descrPtr->flowTemplate == SKERNEL_TCP_FLOW_E)
                {
                    l4HeaderSize = TCP_HEADER_SIZE;
                }
                else if (descrPtr->flowTemplate == SKERNEL_UDP_FLOW_E)
                {
                    l4HeaderSize = UDP_HEADER_SIZE;
                }
                else
                {
                    continue;
                }
                searchKeyPtr[offset] = *(descrPtr->l4HeaderPtr + l4HeaderSize +
                    template.offsetValue[offset]);
            }
            else
            {
                continue;
            }

            searchKeyPtr[offset] &=  template.byteMask[offset];
        }
    }

    if(includeInlifOrVidMode==0)
    {
        if(template.includeLif ==1)
        {
            searchKeyPtr[offset++] = descrPtr->inLifNumber >> 8;
            searchKeyPtr[offset++] = descrPtr->inLifNumber & 0xff;
        }
        else if(template.includeVid == 1)
        {
            /* this is the correct way ---  not in net work order !!! */
            searchKeyPtr[offset++] = descrPtr->vid & 0xff;
            searchKeyPtr[offset++] = ((descrPtr->vid >> 8) & 0x0f) |
                                     ((descrPtr->userPriorityTag & 0x7)<<5)   ;
        }
    }
    else /* includeInlifOrVidMode == 1 */
    {
        if(template.includeLif ==1)
        {
            searchKeyPtr[14] = descrPtr->inLifNumber >> 8;
            searchKeyPtr[15] = descrPtr->inLifNumber & 0xff;
        }
        else if(template.includeVid == 1)
        {
            /* this is the correct way ---  not in net work order !!! */
            searchKeyPtr[14] = descrPtr->vid & 0xff;
            searchKeyPtr[15] = ((descrPtr->vid >> 8) & 0x0f) |
                               ((descrPtr->userPriorityTag & 0x7)<<5)   ;
        }
    }

    if(template.incFlowTemp == 1)
    {
        smemRegFldGet(devObjPtr, INBLOCK_CONTROL_REG, 23, 1,
                        &enhancedIncludeFlowTemplate);

        searchKeyPtr[offset++] = (descrPtr->majorTemplate << 3) |
                                 (descrPtr->flowTemplate) ;

        if(enhancedIncludeFlowTemplate == 1)
        {
            searchKeyPtr[offset++] |= (descrPtr->doRout <<7            ) |
                                      (descrPtr->macDaLookupResult <<6 ) |
                                      (descrPtr->macDaType <<4         ) ;
        }
    }

    return;
}


/*******************************************************************************
*   snetSambaGfcFlowXorHash
*
* DESCRIPTION:
*        function calculate hashAdrress from the XOR Hashing  of the template .
*        the code is basically taken from the function : coreFlowXorHash
*        of PSS CORE
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        tempAddress - address to get the info on hashing of the bytes of the flow
*        searchKeyPtr - pointer to the search key
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       basic hashAddr that is result of the Xor Hashing
*
* COMMENT:
*     code from coreFlowXorHash(...)
*******************************************************************************/
static GT_U32 snetSambaGfcFlowXorHash
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32               tempAddress,
    IN GT_U8                searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    IN GT_U32               byteNum
)
{
    GT_U32  fieldValue;
    GT_32   flip,hashMask,csh;
    GT_U8   ii;
    GT_U32  res,tmp;

    res = 0;
    for (ii=0; ii< byteNum ;ii++)
    {
        /* 1.every byte configuration has 22 bits
           2.every 5 byte configuration has new base address that is
              4 words after the previous base address */
        if(ii!=0 && (ii%5)==0)
        {
            tempAddress += 4*sizeof(GT_U32);/* the next 4 words */
        }

        fieldValue = snetUtilGetContinuesValue(devObjPtr,tempAddress,22,(ii%5));

        csh = (fieldValue >> 0 ) & 0x1f;/*5bits*/
        flip = (fieldValue >> 5 ) & 0x01;/*1 bit*/
        hashMask = (fieldValue >> 6 ) & 0xff;/*8bits*/

        tmp = SIMULATION_CSH32( (GT_U32)( hashMask & searchKeyPtr[ii] ),
                                csh);

        if(flip == 1)
        {
            tmp = SIMULATION_FLIP32(tmp) ;
        }

        res = res^tmp;
    }

    return res;
}



/*******************************************************************************
*   snetSambaGfcFlowCrcHash
*
* DESCRIPTION:
*        function calculate hashAdrress from the CRC Hashing of the template .
*        the code is basically taken from the function : coreFlowCrcHash
*        of PSS CORE
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        tempAddress - address to get the info on hashing of the bytes of the flow
*        searchKeyPtr - pointer to the search key
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       basic hashAddr that is result of the CRC Hashing
*
* COMMENT:
*     code from coreFlowCrcHash(...)
*******************************************************************************/
static GT_U32 snetSambaGfcFlowCrcHash
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32               tempAddress,
    IN GT_U8                searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    IN GT_U32               byteNum
)
{
    GT_U32  fieldValue;
    GT_32   hashMask;
    GT_U8   paddData[24];
    GT_U8   oldByteNum=(GT_U8)byteNum;
    GT_U32  crcAccum = 0xFFFFFFFF;
    GT_U32  ii,jj;
    GT_U32  byteArray8[2];

    if((byteNum & 0x7) != 0)
    {
        byteNum = 8*(1 + (byteNum >> 3));
    }

    memset(paddData,0,sizeof(GT_U8) * byteNum);
    memcpy(paddData,searchKeyPtr,sizeof(GT_U8) * oldByteNum);

    /*Apply the byte mask*/
    for (ii=0; ii< byteNum;ii++)
    {
        /* 1.every byte configuration has 22 bits
           2.every 5 byte configuration has new base address that is
              4 words after the previous base address */
        if(ii!=0 && (ii%5)==0)
        {
            tempAddress += 4*sizeof(GT_U32);/* the next 4 words */
        }

        fieldValue = snetUtilGetContinuesValue(devObjPtr,tempAddress,22,(ii%5));

        hashMask = (fieldValue >> 6 ) & 0xff;/*8bits*/

        paddData[ii] &= hashMask;
    }

    for(jj=0; jj < (byteNum >> 3); jj ++)
    {
        ii = (byteNum >> 3) - jj - 1;
        byteArray8[0] =  paddData[ii << 3] | (paddData[(ii << 3)+1] << 8) |
               (paddData[(ii << 3)+2] << 16) | (paddData[(ii << 3)+3] << 24);
        byteArray8[1] =  paddData[(ii << 3)+4] | (paddData[(ii << 3)+5] << 8) |
               (paddData[(ii << 3)+6] << 16) | (paddData[(ii << 3)+7] << 24);

        crcAccum = snetPolicyCrc8Bytes(byteArray8, crcAccum);
    }

    return crcAccum;
}

/*******************************************************************************
*   snetSambaGfcFlowHashAddrGet
*
* DESCRIPTION:
*        function calculate hashAdrress from the hash parameters of the template .
*        the code is basically taken from the function : coreFlowHash
*        of PSS CORE
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
*        templIndex - index of template (major <<3 |minor)
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       hashAddr from where to get hash entry .
*
* COMMENT:
*
*******************************************************************************/
extern GT_U32 snetSambaGfcFlowHashAddrGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_FLOW_TEMPLATE_HASH  *hashInfoPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    IN    GT_U32                   templIndex
)
{
    DECLARE_FUNC_NAME(snetSambaGfcFlowHashAddrGet);

    GT_U32  hashAddr;
    GT_U32  byteNum ;
    GT_U32  fieldValue;
    GT_U32  tempAddress;/*address to get the info on hashing of the bytes of
                        the flow*/
    GT_U32  tempValue;


    tempAddress = FLOW_TEMPLATE_HASH_SELECT_TABLE_REG +
        SNET_SAMBA_FLOW_TEMPLATE_HASH_SELECT_ENTRY_SIZE_IN_BYTES*templIndex;

    /* every template has 8 bits */
    __LOG(("every template has 8 bits"));
    fieldValue = snetUtilGetContinuesValue(devObjPtr,
            FLOW_TEMPLATE_CONF_REG,8,templIndex);

    byteNum = (fieldValue >> 0) & 0x1f ;/* 5 bits */

    if(byteNum > SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS)
    {
        skernelFatalError(" snetSambaGfcFlowHashAddrGet: byteNum[%d] overflow [%d] ",
                            byteNum,SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS);
    }

    if(hashInfoPtr->hashType == SNET_POLICY_FLOW_XOR_HASH)
    {
        tempValue = snetSambaGfcFlowXorHash(devObjPtr,tempAddress,
                                            searchKeyPtr,byteNum);
    }
    else
    {
        tempValue = snetSambaGfcFlowCrcHash(devObjPtr,tempAddress,
                                            searchKeyPtr,byteNum);
    }

    tempValue = SIMULATION_CSH32(tempValue,hashInfoPtr->csh * 4);

    tempValue = tempValue & ( (1<<(hashInfoPtr->adWidth+1)) - 1);

    hashAddr = tempValue * sizeof(SNET_SAMBA_CLASSIFIER_HASH_ENTRY);

    return hashAddr;
}

/*******************************************************************************
* snetSambeGfcDiffBitVal
*
* DESCRIPTION:
*       Compute value of key on diff bits.
*
* INPUTS:
*   col - start offset.
*   len - number of bits.
*   pattern - flow pattern.
*
* OUTPUTS:
*   val - computed value.
*
* RETURNS:
*   val - computed value.
*
* COMMENTS:
*       the function is COPIED from the coreDiffBitVal(...) implementation
*******************************************************************************/
extern GT_U8 snetSambeGfcDiffBitVal
(
    GT_U8 col,
    GT_U8 len,
    GT_U8 pattern[]
)
{
    GT_U8  byteOffset; /* byte from key indexed by diff bits */
    GT_U8  bitOffset;  /* bit in the byte that value by diff bits started with*/
    GT_U8  val;

    byteOffset = col >> 3;
    bitOffset =  col & 0x7;

    if( (byteOffset) == ((col + len) >> 3) )
    {
        /*all diff bits in the same byte*/
       val = (pattern[byteOffset] >> bitOffset) &
                ((1 << len) - 1);
    }
    else
    {
        /*Value from byteOffset - least significant*/
        val = (pattern[byteOffset] >> bitOffset) &
                 ((1 << (8 - bitOffset) ) - 1) ;

        /*Value from byteOffset + 1  - most significant*/
        val |= ( pattern[byteOffset+1] &
                 ((1 << (len - (8 - bitOffset)) ) - 1) ) <<
                  (8-bitOffset);

    }

    return val;
}

/*******************************************************************************
*   snetSambaGfcFlowVlTriSearch
*
* DESCRIPTION:
*        The function get the hashEntryInfoPtr where there is collision and
*        look for the next entry address until no collisions or entry no valid
*        if found valid entry it set validVlTrieEntryPtrPtr to point to the entry
*         in the vl-trie
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
*        hashEntryInfoPtr - the pointer to the first entry after the hashing
*                           algorithm found that the entry has collision .
*
* OUTPUTS:
*        validVlTrieEntryPtr_Ptr - pointer to pointer pointer to the entry in
*                                  the vl-trie that holds the info to the floe entry
*
* RETURNS:
*       GT_TRUE - if entry found in the Vl-tri
*       GT_FALSE - otherwise
* COMMENT:
*       logic taken from 5.3 [2] called Search VL-trie algorithm.
*       And 5.8.4 [2] called Decode Entry Algorithm
*       non recursive implementation
*
*******************************************************************************/
extern GT_BOOL snetSambaGfcFlowVlTriSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_CLASSIFIER_HASH_ENTRY *hashEntryInfoPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    OUT   SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY **validVlTrieEntryPtr_Ptr
)
{
    DECLARE_FUNC_NAME(snetSambaGfcFlowVlTriSearch);

    SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY *vlTrieEntryPtr =NULL;/* current pointer
                                                  o entry in the vl-trie tree*/
    GT_U32  nextAddress;
    GT_U8   offset;
    GT_U8   range;
    GT_U32  groupIndex;/* the index of the vl-tri in the array of leaf */
    GT_U32  vlTriDepth = 0;


    if(!hashEntryInfoPtr->valid || !hashEntryInfoPtr->col)
    {
        skernelFatalError(" snetSambaGfcFlowVlTriSearch: hash entry must have "\
                            " collision");
    }


    nextAddress = (hashEntryInfoPtr->addressLsb << 5) ; /* 21 bits */

    if(hashEntryInfoPtr->Offset0_or_addressMsb == 0xff)
    {
        /* collision offset is not valid */
        __LOG(("collision offset is not valid"));
        groupIndex = 0;
    }
    else
    {
        /* the offset to the group 0 in the key that cause a collision in the
        hash function */
        offset = (GT_U8)hashEntryInfoPtr->Offset0_or_addressMsb;

        range = (GT_U8)hashEntryInfoPtr->colRange0 + 1 ;
        if((offset+range)>=(SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8))
        {
            skernelFatalError(" snetSambaGfcFlowVlTriSearch: offset[%d] + range[%d]"\
                                " out of max size [%d]",
                                offset,
                                range,
                                (SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8));
        }

        groupIndex = snetSambeGfcDiffBitVal(offset,range,searchKeyPtr);
    }

    /* update the address to include the offset due to group index */
    __LOG(("update the address to include the offset due to group index"));
    nextAddress +=
        groupIndex * sizeof(SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY);

    while(GT_TRUE)
    {
        if(vlTriDepth++ > 30)
        {
            skernelFatalError(" snetSambaGfcFlowVlTriSearch: vlTriDepth[%d] too big",
                                vlTriDepth);
        }
        /* the vl tri is in the sram */
        nextAddress |= SNET_SAMBA_INTERNAL_MEM_BASE_ADDRESS;

        /* get the next/first vl-trie entry for the next address */
        vlTrieEntryPtr = smemMemGet(devObjPtr,nextAddress);

        if (vlTrieEntryPtr->valid == 0 )
        {
            return GT_FALSE ;
        }

        if(vlTrieEntryPtr->col == 0)
        {
            break;
        }

        nextAddress = (vlTrieEntryPtr->addressLsb << 4) ; /* 22 bits */

        if(vlTrieEntryPtr->Offset0_or_addressMsb == 0xff)
        {
            /* collision offset is not valid */
            groupIndex = 0;
        }
        else
        {
            /* the offset to the group 0 in the key that cause a collision in the
            hash function */
            offset = (GT_U8)vlTrieEntryPtr->Offset0_or_addressMsb;

            range = (GT_U8)vlTrieEntryPtr->colRange0 + 1 ;
            if((offset+range)>=(SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8))
            {
                skernelFatalError(" snetSambaGfcFlowVlTriSearch: offset[%d] + range[%d]"\
                                    " out of max size [%d]",
                                    offset,
                                    range,
                                    (SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8));
            }

            groupIndex = snetSambeGfcDiffBitVal(offset,range,searchKeyPtr);
        }

        /* update the address to include the offset due to group index */
        __LOG(("update the address to include the offset due to group index"));
        nextAddress +=
            groupIndex * sizeof(SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY);

    }

    /* we have no collision on the last search
       so we need to return the pointer*/

    *validVlTrieEntryPtr_Ptr = vlTrieEntryPtr;

    return GT_TRUE;
}



/*******************************************************************************
*   snetSambaClassifierConvertToPolicy
*
* DESCRIPTION:
*        the function convert info form address keyTableAddr to the format of
*        policy into the policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        keyTableAddr  -- address where the flow entry exist
*        policyActionPtr - hold reset values of this info
*        searchKeyPtr - pointer to the search key
*        maxGroupIndex - the number of flows to check for match in flow tale
*        flowEntrySize - the size of flow entry
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*        GT_TRUE -  if flow entry matched
*        GT_FALSE - if flow entry not matched
* COMMENT:
*       logic from 1.5.6 - 1.5.9  [5]
*******************************************************************************/
static GT_BOOL snetSambaClassifierConvertToPolicy
(
    IN  SKERNEL_DEVICE_OBJECT *   devObjPtr,
    IN  GT_U32                    keyTableAddr,
    OUT SNET_SAMBA_POLICY_ACTION *policyActionPtr,
    IN  GT_U8                  * searchKeyPtr,
    IN  GT_U32                   maxGroupIndex,
    IN  GT_U32                   flowEntrySize
)
{
    DECLARE_FUNC_NAME(snetSambaClassifierConvertToPolicy);

    SNET_SAMBA_CLASSIFIER_ACTION classifierActions; /*format from Table 6 (page 13) [6]*/
    GT_U32 enableMultiplexedTcPointerMode ; /* bit [0] in register 0x028001E8*/
    GT_U8   *memoryPtr;
    GT_BOOL useTc=GT_FALSE ;
    GT_BOOL flowPattentMatched = GT_FALSE;

    smemRegFldGet(devObjPtr, POLICY_CONTROL_REG, 0, 1,
        &enableMultiplexedTcPointerMode);

    memoryPtr = smemMemGet(devObjPtr,keyTableAddr);

    /* find the matching pattern */
    __LOG(("find the matching pattern"));
    while(GT_TRUE)
    {
        /* check that the entry is valid */
        if(memoryPtr[0]&0x01)
        {
            if(0==memcmp(memoryPtr+12,searchKeyPtr,(flowEntrySize-12)))
            {
                flowPattentMatched = GT_TRUE ;
                break;
            }
        }

        /* decrement the num of searches */
        if(0==maxGroupIndex)
        {
            /* no more search to do */
            break;
        }

        maxGroupIndex--;
        memoryPtr+=flowEntrySize ;
    }

    if(flowPattentMatched == GT_FALSE)
    {
        return GT_FALSE;
    }

    memoryPtr ++; /* the actions start at the second byte of the flow entry */
    /*SET classifierActions ACCORDING to memory in keyTableAddr*/
    memcpy(&classifierActions,memoryPtr,sizeof(classifierActions));

    /* Cmd --- according to SNET_POLICY_CMD_ENT */
    if (classifierActions.word0.fields.cmd == SKERNEL_PCL_DROP_E /* 0 */)
    {
        policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_DROP_E; /* 0 */
    }
    else if( classifierActions.word0.fields.cmd == SKERNEL_PCL_TRAP_E /* 2 */)
    {
        policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E; /* 1 */
    }
    else /*classifierActions.word0.fields.cmd == 1 or 3 */
    {
        policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_PROCESS_E ; /* 2 */
    }

    /* mapping GFC Activate TC/ Cnt to policy */
    if (classifierActions.word0.fields.cmd ==
            SKERNEL_PCL_PASS_SEND_TC_E)/* pass and tc */
    {
        policyActionPtr->tcOrCnt_Ptr =
            (GT_U16)classifierActions.word0.fields.tc_PtrOrCosParam;
        useTc = GT_TRUE;
    }

    policyActionPtr->activateTC = policyActionPtr->activateCount = useTc;

    policyActionPtr->dscp=
        (GT_U8)classifierActions.word0.fields.dscp;
    policyActionPtr->mark_Cmd =
        (GT_U8)classifierActions.word0.fields.mark_Cmd;
    policyActionPtr->redirectFlowCmd =
        (GT_U8)classifierActions.word0.fields.redirectFlowCmd;
    policyActionPtr->mirrorToCpu =
        (GT_U8)classifierActions.word0.fields.mirrorToCpu;

    policyActionPtr->outlifOrIPv4_Ptr =
        (classifierActions.word1.noMultiplexedCosRedirectOutLif.outlifOrIPv4_Ptr_msb<<1) |
        classifierActions.word0.fields.outlifOrIPv4_Ptr_lsb;

    /* mapping GFC cos parameters and cmdMark bits to policy */
    if (enableMultiplexedTcPointerMode == 1)
    {
        if(useTc == GT_FALSE)
        {
            policyActionPtr->dp =
                (GT_U8)(classifierActions.word0.fields.tc_PtrOrCosParam) & 0x3 ;
            policyActionPtr->tc =
                (GT_U8)(classifierActions.word0.fields.tc_PtrOrCosParam >> 2) & 0x7 ;
            policyActionPtr->up =
                (GT_U8)(classifierActions.word0.fields.tc_PtrOrCosParam >> 5) & 0x7 ;
            policyActionPtr->markTc = 1;/*see 5.2 [6]*/
            policyActionPtr->markDp = 1;
            policyActionPtr->markUp = 1;
        }

        policyActionPtr->markDscp = 1;
    }
    else
    {
        if(classifierActions.word0.fields.redirectFlowCmd ==
            SKERNEL_PCL_OUTLIF_FRWD_E)
        {
            policyActionPtr->outlifOrIPv4_Ptr |=
                classifierActions.word2.multiplexedCos.linkLayerOutLif << 20;

            policyActionPtr->dp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.cosDp ;
            policyActionPtr->tc =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.cosTc ;
            policyActionPtr->up =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.cosUp ;
            policyActionPtr->markTc =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.markTc ;
            policyActionPtr->markDp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.markDp ;
            policyActionPtr->markUp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.markUp ;
            policyActionPtr->markDscp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectOutLif.markDscp ;
        }
        else
        {
            policyActionPtr->dp =
                (GT_U8)classifierActions.word1.noMultiplexedCosRedirectNoOutLif.cosDp ;
            policyActionPtr->tc =
                (GT_U8)classifierActions.word1.noMultiplexedCosRedirectNoOutLif.cosTc ;
            policyActionPtr->up =
                (GT_U8)classifierActions.word1.noMultiplexedCosRedirectNoOutLif.cosUp ;
            policyActionPtr->markTc =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectNoOutLif.markTc ;
            policyActionPtr->markDp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectNoOutLif.markDp ;
            policyActionPtr->markUp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectNoOutLif.markUp ;
            policyActionPtr->markDscp =
                (GT_U8)classifierActions.word2.noMultiplexedCosRedirectNoOutLif.markDscp ;
        }
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetSambaGfcSearch
*
* DESCRIPTION:
*        search in GFC for classification actions , fill the info in policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       GT_TRUE - if entry found in the GFC
*       GT_FALSE - otherwise
* COMMENT:
*
*******************************************************************************/
static GT_BOOL snetSambaGfcSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
)
{
    DECLARE_FUNC_NAME(snetSambaGfcSearch);

    GT_U32 tempIndex; /* index of the template.*/
    GT_U32 BaseAddress ; /* This field shows the base address to be added to the
                            final hash value (after csh and AdWidth) in 128
                            entries resolution.*/
    GT_U32 hashAddr ; /*the address in the hash table where the hash entry
                        should exist.*/
    GT_U32 keyTableAddr ; /*the address where the actions exist*/
    GT_U8   offset;
    GT_U8   range;
    GT_U32  groupIndex;/* the index of the vl-tri in the array of leaf */
    GT_U32  flowEntrySize;/* the size in bytes of the flow entry */
    GT_U32  regValue;/* value of register*/
    GT_U32  byteNum ;
    GT_U32  fieldValue;
    GT_U32  keyTableSize;/* the key table size in bytes */
    GT_BOOL finalMatch;/* final matching in the flow table*/

    SNET_SAMBA_FLOW_TEMPLATE_HASH  hashInfo;/*see registers 0x02C28000-0x02C2803C*/
    SNET_SAMBA_CLASSIFIER_HASH_ENTRY *hashEntryInfoPtr;/* table info from 4.7 [2] */
    SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY *validVlTrieEntryPtr;/* table info from 4.7 [2]*/

    tempIndex = (descrPtr->majorTemplate <<3) | descrPtr->flowTemplate;

    smemRegGet(devObjPtr, FLOW_TEMPLATE_HASH_CONFIGURATION_REG+tempIndex*4,
                (void *)&hashInfo);

    /* get base address for the hashAdresses of that template */
    __LOG(("get base address for the hashAdresses of that template"));
    BaseAddress = hashInfo.baseAddress<<10 ;/*22 bits for base address*/

    /*Compute Hash value*/
    hashAddr = snetSambaGfcFlowHashAddrGet(devObjPtr, descrPtr,
                                           &hashInfo , searchKeyPtr , tempIndex);

    hashAddr += BaseAddress;

    /* base address for the flow dram or the internal registers memory */
    hashAddr |= SNET_SAMBA_CLASSIFIER_FLOW_BASE_ADDRESS_MAC(devObjPtr);

    hashEntryInfoPtr = smemMemGet(devObjPtr,hashAddr);

    if (hashEntryInfoPtr->valid == 0)
    {
        return GT_FALSE ;
    }

    if(hashEntryInfoPtr->col == 1)
    {
        /* the entry has collisions */
        if(GT_FALSE == snetSambaGfcFlowVlTriSearch(devObjPtr, descrPtr,
                                            hashEntryInfoPtr,searchKeyPtr,
                                            &validVlTrieEntryPtr))
        {
            return GT_FALSE;
        }

        /* the entry is valid with no collisions */
/*        keyTableAddr = (validVlTrieEntryPtr->addressLsb>>1);*/
        keyTableAddr = ((validVlTrieEntryPtr->addressLsb>>1) |
          ((validVlTrieEntryPtr->Offset0_or_addressMsb << 21)))<<3;

        /* the offset to the group 0 in the key that cause a collision in the
        hash function */
        offset = (GT_U8)validVlTrieEntryPtr->offset1;
        range = (GT_U8)validVlTrieEntryPtr->colRange1 + 1 ;
    }
    else
    {
        /* the entry is valid with no collisions */
        __LOG(("the entry is valid with no collisions"));
        keyTableAddr = (hashEntryInfoPtr->addressLsb |
                        (hashEntryInfoPtr->Offset0_or_addressMsb <<21)) << 3;

        /* the offset to the group 0 in the key that cause a collision in the
        hash function */
        offset = (GT_U8)hashEntryInfoPtr->offset1;
        range = (GT_U8)hashEntryInfoPtr->colRange1 + 1 ;
    }



    if( offset == 0xFF )
    {
        groupIndex = 0;
    }
    else
    {
        if((offset+range)>=(SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8))
        {
            skernelFatalError(" snetSambaGfcFlowVlTriSearch: offset[%d] + range[%d]"\
                              " out of max size [%d]",
                              offset,
                              range,
                              (SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8));
        }
        groupIndex = snetSambeGfcDiffBitVal(offset,range,searchKeyPtr);
    }

    /* every template has 8 bits */
    fieldValue = snetUtilGetContinuesValue(devObjPtr,
            FLOW_TEMPLATE_CONF_REG,8,tempIndex);

    /* get the size of the search key size */
    byteNum = (fieldValue >> 0) & 0x1f ;/* 5 bits */

    /* get the size of the flow entry */
    flowEntrySize =
        SNET_SAMBA_CLASSIFIER_FLOW_ENTRY_NUM_OF_BYTES_MAC(devObjPtr,byteNum);

    /*keyTableAddr += groupIndex * flowEntrySize;*/

    /* check validation with the size of the flow table size */
    smemRegGet(devObjPtr, SAMBA_KEY_TABLE_SIZE_REG,&regValue);

    keyTableSize = SNET_SAMBA_CLASSIFIER_FLOW_KEY_TABLE_SIZE(devObjPtr,regValue);

    /* it seems that keyTableAddr > keyTableSize */
    keyTableAddr %= keyTableSize;

    if( keyTableSize < (keyTableAddr + flowEntrySize))
    {
        /* the index is out the flow table range */
        skernelFatalError(" snetSambaGfcSearch: the address is out the flow table range"\
        " (regValue[0x%8.8x] & 0x03fffffc) < (keyTableAddr[0x%8.8x] + flowEntrySize[0x%8.8x])" ,
        regValue,keyTableAddr,flowEntrySize);
    }

    /* get the base address of the flow table*/
    smemRegGet(devObjPtr, SAMBA_KEY_TABLE_BASE_ADDRESS_REG,&regValue);


    keyTableAddr +=
        SNET_SAMBA_CLASSIFIER_FLOW_KEY_TABLE_ADDRESS(devObjPtr,regValue);
            /* 29 bits -- but in 4 bytes resolution (1 word )*/

    /* base address for the flow dram or the internal registers memory */
    keyTableAddr |= SNET_SAMBA_CLASSIFIER_FLOW_BASE_ADDRESS_MAC(devObjPtr);

    /* FILL policyActionPtr from the flow entry info in address KeyTableAddr  */
    finalMatch = snetSambaClassifierConvertToPolicy(devObjPtr,keyTableAddr,
                                        policyActionPtr,
                                        searchKeyPtr,
                                        groupIndex,
                                        flowEntrySize);

    return finalMatch;
}

/*******************************************************************************
*   snetSambaPolicyConvertTwistFormat2SambaFormat
*
* DESCRIPTION:
*        convert policy stc from twist to samba
*
* INPUTS:
*        twistPolicyActionPtr - the values as twist simulation holds it
*        sambaPolicyActionPtr - hold reset values of this info
* OUTPUTS:
*        sambaPolicyActionPtr - the info in the Samba's simulation format
*
* RETURNS:
*
*
* COMMENT:
*
*******************************************************************************/
static void snetSambaPolicyConvertTwistFormat2SambaFormat(
    IN SNET_POLICY_ACTION_STC *twistPolicyActionPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *sambaPolicyActionPtr
)
{
    if (twistPolicyActionPtr->cmd == SKERNEL_PCL_DROP_E /* 0 */)
    {
        sambaPolicyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_DROP_E; /* 0 */
    }
    else if(twistPolicyActionPtr->cmd == SKERNEL_PCL_TRAP_E /* 2 */)
    {
        sambaPolicyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ;/* 1 */
    }
    else /*classifierActions.word0.fields.cmd == 1 or 3 */
    {
        sambaPolicyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_PROCESS_E ; /* 2 */
    }

    /* mapping GFC Activate TC/ Cnt to policy */
    if (twistPolicyActionPtr->cmd ==
            SKERNEL_PCL_PASS_SEND_TC_E)/* pass and tc */
    {
        sambaPolicyActionPtr->tcOrCnt_Ptr = (GT_U16)twistPolicyActionPtr->tcCosParam;
    }
    else
    {
        sambaPolicyActionPtr->dp =
            (GT_U8)(twistPolicyActionPtr->tcCosParam) & 0x3 ;
        sambaPolicyActionPtr->tc =
            (GT_U8)(twistPolicyActionPtr->tcCosParam >> 2) & 0x7 ;
        sambaPolicyActionPtr->up =
            (GT_U8)(twistPolicyActionPtr->tcCosParam >> 5) & 0x7 ;
        sambaPolicyActionPtr->markTc = 1;/*see 5.2 [6]*/
        sambaPolicyActionPtr->markDp = 1;
        sambaPolicyActionPtr->markUp = 1;
        sambaPolicyActionPtr->markDscp = 1;
    }

    sambaPolicyActionPtr->mirrorToAnalyzerPort = (GT_U8)twistPolicyActionPtr->analyzMirror;
    sambaPolicyActionPtr->mark_Cmd = (GT_U8)twistPolicyActionPtr->markCmd;
    sambaPolicyActionPtr->dscp = (GT_U8)twistPolicyActionPtr->dscp;
    sambaPolicyActionPtr->mirrorToCpu = (GT_U8)twistPolicyActionPtr->cpuMirror;
}


/*******************************************************************************
*   snetSambaIpFlowGet
*
* DESCRIPTION:
*        search in CoS for classification actions , fill the info in policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       GT_TRUE - if entry found in the Cos
*       GT_FALSE - otherwise
* COMMENT:
*       use twist function
*******************************************************************************/
static GT_BOOL snetSambaIpFlowGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
)
{
    SNET_POLICY_ACTION_STC twistPolicyActionInfo;

    if(GT_FALSE == snetTwistCosTablesLookup(devObjPtr,descrPtr,
                    &twistPolicyActionInfo))
    {
        return GT_FALSE;
    }

    /* convert SNET_POLICY_ACTION_STC to SNET_SAMBA_POLICY_ACTION */
    snetSambaPolicyConvertTwistFormat2SambaFormat(&twistPolicyActionInfo,
        policyActionPtr);

    return GT_TRUE;
}



/*******************************************************************************
*   snetSambaDefaultTemplateFlowGet
*
* DESCRIPTION:
*        get template default classification actions ,
*        fill the info in policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       GT_TRUE - if entry found in the Cos
*       GT_FALSE - otherwise
* COMMENT:
*       use twist function
*******************************************************************************/
static void snetSambaDefaultTemplateFlowGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
)
{
    DECLARE_FUNC_NAME(snetSambaDefaultTemplateFlowGet);

    SNET_POLICY_ACTION_STC twistPolicyActionInfo;

    snetTwisDefTemplActionGet(devObjPtr,descrPtr,&twistPolicyActionInfo);

    /* convert SNET_POLICY_ACTION_STC to SNET_SAMBA_POLICY_ACTION */
    __LOG(("convert SNET_POLICY_ACTION_STC to SNET_SAMBA_POLICY_ACTION"));
    snetSambaPolicyConvertTwistFormat2SambaFormat(&twistPolicyActionInfo,
        policyActionPtr);

    return ;
}


/*******************************************************************************
*   snetSambaClassificationSearch
*
* DESCRIPTION:
*        search for classification actions , fill the info in policyAction
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*        classifierFoundPtr - pointer to return if the gfc/ip found entry or
*                             the default entry was taken .
*
* COMMENT:
*        from 12.4 [1]
*******************************************************************************/
static void snetSambaClassificationSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr,
    OUT   GT_BOOL *classifierFoundPtr
)
{
    GT_BOOL doIp = GT_FALSE; /* need to do IP classification */
    GT_BOOL doClassifier = GT_FALSE;/* need to do GFC */
    GT_BOOL entryFound = GT_FALSE;
    GT_U32 lookUpMode;

    smemRegFldGet(devObjPtr, TCB_CONTROL_REG, 1, 2,&lookUpMode);

    if( lookUpMode == 0)
    {
        doClassifier = GT_TRUE;
    }
    else if( lookUpMode == 1)
    {
        doClassifier = GT_TRUE;
        doIp = GT_TRUE;
    }

    if(doClassifier)
    {
        entryFound = snetSambaGfcSearch(devObjPtr, descrPtr,searchKeyPtr,
                                        policyActionPtr);
        policyActionPtr->trapActionSource = SNET_POLICY_ACTION_GFC_E;
    }

    if(entryFound == GT_FALSE && doIp)
    {
        entryFound = snetSambaIpFlowGet(devObjPtr, descrPtr,policyActionPtr);
        policyActionPtr->trapActionSource = SNET_POLICY_ACTION_IP_TABLES_E;
    }

    *classifierFoundPtr = entryFound;

    if(entryFound == GT_FALSE)
    {
        snetSambaDefaultTemplateFlowGet(devObjPtr, descrPtr,policyActionPtr);
        policyActionPtr->trapActionSource = SNET_POLICY_ACTION_TEMPLATE_E;
    }

    return ;
}

/*******************************************************************************
*   snetSambaClassificationEngine
*
* DESCRIPTION:
*        the function do the classifier engine to get an action info
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        classifierPolicyActionPtr - hold reset values of this info
* OUTPUTS:
*        classifierPolicyActionPtr - pointer to where to put the actions info
*        classifierFoundPtr - pointer to return if the gfc/ip found entry or
*                             the default entry was taken .
*
*******************************************************************************/
extern void snetSambaClassificationEngine
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *classifierPolicyActionPtr,
    OUT   GT_BOOL *classifierFoundPtr
)
{
    GT_U8  searchKeyArray[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS];

    snetSambaClassificationKeyCreate(devObjPtr, descrPtr, searchKeyArray);

    snetSambaClassificationSearch(devObjPtr, descrPtr,searchKeyArray,
        classifierPolicyActionPtr,classifierFoundPtr);

    return;
}


/*******************************************************************************
* snetPolicyCrc8Bytes
*
* DESCRIPTION:
*       Calculates CRC 8 bytes hash on given key.
*
* INPUTS:
*       data -    the key pattern.
*       initCrs - Init value of CRC.
*
* OUTPUTS:
*       Hash value.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*******************************************************************************/
static GT_U32 snetPolicyCrc8Bytes
(
    IN GT_U32 data[2],
    IN GT_U32 initCrc
)
{
    GT_U8   i;
    GT_U8   bitArray[64];
    GT_U8   initCrcBits[32];
    GT_U8   tempCrcVal[32];
    GT_U32  result;

    for(i = 0; i < 64; i++)
    {
        bitArray[i] = (GT_U8)(data[i >> 5] >> (i & 0x1f)) & 0x1;
    }

    for(i = 0; i < 32; i++)
    {
        initCrcBits[i] = (GT_U8)(initCrc >> i) & 0x1;
    }

    tempCrcVal[0] = (bitArray[63] ^ bitArray[61] ^ bitArray[60] ^
                     bitArray[58] ^ bitArray[55] ^ bitArray[54] ^
                     bitArray[53] ^ bitArray[50] ^ bitArray[48] ^
                     bitArray[47] ^ bitArray[45] ^ bitArray[44] ^
                     bitArray[37] ^ bitArray[34] ^ bitArray[32] ^
                     bitArray[31] ^ bitArray[30] ^ bitArray[29] ^
                     bitArray[28] ^ bitArray[26] ^ bitArray[25] ^
                     bitArray[24] ^ bitArray[16] ^ bitArray[12] ^
                     bitArray[10] ^ bitArray[9] ^ bitArray[6] ^
                     bitArray[0] ^ initCrcBits[0] ^ initCrcBits[2] ^
                     initCrcBits[5] ^ initCrcBits[12] ^ initCrcBits[13] ^
                     initCrcBits[15] ^ initCrcBits[16] ^ initCrcBits[18] ^
                     initCrcBits[21] ^ initCrcBits[22] ^ initCrcBits[23] ^
                     initCrcBits[26] ^ initCrcBits[28] ^
                     initCrcBits[29] ^ initCrcBits[31]);

    tempCrcVal[1] = (bitArray[63] ^ bitArray[62] ^ bitArray[60] ^
                     bitArray[59] ^ bitArray[58] ^ bitArray[56] ^
                     bitArray[53] ^ bitArray[51] ^ bitArray[50] ^
                     bitArray[49] ^ bitArray[47] ^ bitArray[46] ^
                     bitArray[44] ^ bitArray[38] ^ bitArray[37] ^
                     bitArray[35] ^ bitArray[34] ^ bitArray[33] ^
                     bitArray[28] ^ bitArray[27] ^ bitArray[24] ^
                     bitArray[17] ^ bitArray[16] ^ bitArray[13] ^
                     bitArray[12] ^ bitArray[11] ^ bitArray[9] ^
                     bitArray[7] ^ bitArray[6] ^ bitArray[1] ^
                     bitArray[0] ^  initCrcBits[1] ^ initCrcBits[2] ^
                     initCrcBits[3] ^ initCrcBits[5] ^ initCrcBits[6] ^
                     initCrcBits[12] ^ initCrcBits[14] ^ initCrcBits[15]
                     ^ initCrcBits[17] ^ initCrcBits[18] ^ initCrcBits[19] ^
                     initCrcBits[21] ^ initCrcBits[24] ^ initCrcBits[26] ^
                     initCrcBits[27] ^ initCrcBits[28] ^ initCrcBits[30] ^
                     initCrcBits[31] );

    tempCrcVal[2] = (bitArray[59] ^ bitArray[58] ^ bitArray[57] ^
                     bitArray[55] ^ bitArray[53] ^ bitArray[52] ^
                     bitArray[51] ^ bitArray[44] ^ bitArray[39] ^
                     bitArray[38] ^ bitArray[37] ^ bitArray[36] ^
                     bitArray[35] ^ bitArray[32] ^ bitArray[31] ^
                     bitArray[30] ^ bitArray[26] ^ bitArray[24] ^
                     bitArray[18] ^ bitArray[17] ^ bitArray[16] ^
                     bitArray[14] ^ bitArray[13] ^ bitArray[9] ^
                     bitArray[8] ^ bitArray[7] ^ bitArray[6] ^
                     bitArray[2] ^ bitArray[1] ^ bitArray[0] ^
                     initCrcBits[0] ^ initCrcBits[3] ^ initCrcBits[4] ^
                     initCrcBits[5] ^
                     initCrcBits[6] ^ initCrcBits[7] ^ initCrcBits[12] ^
                     initCrcBits[19] ^ initCrcBits[20] ^ initCrcBits[21] ^
                     initCrcBits[23] ^ initCrcBits[25] ^ initCrcBits[26] ^
                     initCrcBits[27]);

    tempCrcVal[3] = (bitArray[60] ^ bitArray[59] ^ bitArray[58] ^
                     bitArray[56] ^ bitArray[54] ^ bitArray[53] ^
                     bitArray[52] ^ bitArray[45] ^ bitArray[40] ^
                     bitArray[39] ^ bitArray[38] ^ bitArray[37] ^
                     bitArray[36] ^ bitArray[33] ^ bitArray[32] ^
                     bitArray[31] ^ bitArray[27] ^ bitArray[25] ^
                     bitArray[19] ^ bitArray[18] ^ bitArray[17] ^
                     bitArray[15] ^ bitArray[14] ^ bitArray[10] ^
                     bitArray[9] ^ bitArray[8] ^ bitArray[7] ^
                     bitArray[3] ^ bitArray[2] ^ bitArray[1] ^
                     initCrcBits[0] ^ initCrcBits[1] ^ initCrcBits[4] ^
                     initCrcBits[5] ^ initCrcBits[6] ^ initCrcBits[7] ^
                     initCrcBits[8] ^ initCrcBits[13] ^ initCrcBits[20] ^
                     initCrcBits[21] ^ initCrcBits[22] ^ initCrcBits[24] ^
                     initCrcBits[26] ^ initCrcBits[27] ^ initCrcBits[28]);

    tempCrcVal[4] = (bitArray[63] ^ bitArray[59] ^ bitArray[58] ^
                     bitArray[57] ^ bitArray[50] ^ bitArray[48] ^
                     bitArray[47] ^ bitArray[46] ^ bitArray[45] ^
                     bitArray[44] ^ bitArray[41] ^ bitArray[40] ^
                     bitArray[39] ^ bitArray[38] ^ bitArray[33] ^
                     bitArray[31] ^ bitArray[30] ^ bitArray[29] ^
                     bitArray[25] ^ bitArray[24] ^ bitArray[20] ^
                     bitArray[19] ^ bitArray[18] ^ bitArray[15] ^
                     bitArray[12] ^ bitArray[11] ^ bitArray[8] ^
                     bitArray[6] ^ bitArray[4] ^ bitArray[3] ^
                     bitArray[2] ^ bitArray[0] ^ initCrcBits[1] ^
                     initCrcBits[6] ^ initCrcBits[7] ^ initCrcBits[8] ^
                     initCrcBits[9] ^
                     initCrcBits[12] ^ initCrcBits[13] ^ initCrcBits[14] ^
                     initCrcBits[15] ^ initCrcBits[16] ^ initCrcBits[18] ^
                     initCrcBits[25] ^ initCrcBits[26] ^ initCrcBits[27] ^
                     initCrcBits[31]);

    tempCrcVal[5] = (bitArray[63] ^ bitArray[61] ^ bitArray[59] ^
                     bitArray[55] ^ bitArray[54] ^ bitArray[53] ^
                     bitArray[51] ^ bitArray[50] ^ bitArray[49] ^
                     bitArray[46] ^ bitArray[44] ^ bitArray[42] ^
                     bitArray[41] ^ bitArray[40] ^ bitArray[39] ^
                     bitArray[37] ^ bitArray[29] ^ bitArray[28] ^
                     bitArray[24] ^ bitArray[21] ^ bitArray[20] ^
                     bitArray[19] ^ bitArray[13] ^ bitArray[10] ^
                     bitArray[7] ^ bitArray[6] ^ bitArray[5] ^
                     bitArray[4] ^ bitArray[3] ^ bitArray[1] ^
                     bitArray[0] ^ initCrcBits[5] ^ initCrcBits[7] ^
                     initCrcBits[8] ^ initCrcBits[9] ^ initCrcBits[10] ^
                     initCrcBits[12] ^ initCrcBits[14] ^ initCrcBits[17] ^
                     initCrcBits[18] ^ initCrcBits[19] ^ initCrcBits[21] ^
                     initCrcBits[22] ^ initCrcBits[23] ^ initCrcBits[27] ^
                     initCrcBits[29] ^ initCrcBits[31]);

    tempCrcVal[6] = (bitArray[62] ^ bitArray[60] ^ bitArray[56] ^
                     bitArray[55] ^ bitArray[54] ^ bitArray[52] ^
                     bitArray[51] ^ bitArray[50] ^ bitArray[47] ^
                     bitArray[45] ^ bitArray[43] ^ bitArray[42] ^
                     bitArray[41] ^ bitArray[40] ^ bitArray[38] ^
                     bitArray[30] ^ bitArray[29] ^ bitArray[25] ^
                     bitArray[22] ^ bitArray[21] ^ bitArray[20] ^
                     bitArray[14] ^ bitArray[11] ^ bitArray[8] ^
                     bitArray[7] ^ bitArray[6] ^ bitArray[5] ^
                     bitArray[4] ^ bitArray[2] ^ bitArray[1] ^
                     initCrcBits[6] ^ initCrcBits[8] ^ initCrcBits[9] ^
                     initCrcBits[10] ^ initCrcBits[11] ^ initCrcBits[13] ^
                     initCrcBits[15] ^ initCrcBits[18] ^ initCrcBits[19] ^
                     initCrcBits[20] ^ initCrcBits[22] ^ initCrcBits[23] ^
                     initCrcBits[24] ^ initCrcBits[28] ^ initCrcBits[30]);


    tempCrcVal[7] = (bitArray[60] ^ bitArray[58] ^ bitArray[57] ^
                     bitArray[56] ^ bitArray[54] ^ bitArray[52] ^
                     bitArray[51] ^ bitArray[50] ^ bitArray[47] ^
                     bitArray[46] ^ bitArray[45] ^ bitArray[43] ^
                     bitArray[42] ^ bitArray[41] ^ bitArray[39] ^
                     bitArray[37] ^ bitArray[34] ^ bitArray[32] ^
                     bitArray[29] ^ bitArray[28] ^ bitArray[25] ^
                     bitArray[24] ^ bitArray[23] ^ bitArray[22] ^
                     bitArray[21] ^ bitArray[16] ^ bitArray[15] ^
                     bitArray[10] ^ bitArray[8] ^ bitArray[7] ^
                     bitArray[5] ^ bitArray[3] ^ bitArray[2] ^
                     bitArray[0] ^ initCrcBits[0] ^ initCrcBits[2] ^
                     initCrcBits[5] ^ initCrcBits[7] ^ initCrcBits[9] ^
                     initCrcBits[10] ^ initCrcBits[11] ^ initCrcBits[13] ^
                     initCrcBits[14] ^ initCrcBits[15] ^ initCrcBits[18] ^
                     initCrcBits[19] ^ initCrcBits[20] ^ initCrcBits[22] ^
                     initCrcBits[24] ^ initCrcBits[25] ^ initCrcBits[26] ^
                     initCrcBits[28]);


    tempCrcVal[8] = (bitArray[63] ^ bitArray[60] ^ bitArray[59] ^
                     bitArray[57] ^ bitArray[54] ^ bitArray[52] ^
                     bitArray[51] ^ bitArray[50] ^ bitArray[46] ^
                     bitArray[45] ^ bitArray[43] ^ bitArray[42] ^
                     bitArray[40] ^ bitArray[38] ^ bitArray[37] ^
                     bitArray[35] ^ bitArray[34] ^ bitArray[33] ^
                     bitArray[32] ^ bitArray[31] ^ bitArray[28] ^
                     bitArray[23] ^ bitArray[22] ^ bitArray[17] ^
                     bitArray[12] ^ bitArray[11] ^ bitArray[10] ^
                     bitArray[8] ^ bitArray[4] ^ bitArray[3] ^
                     bitArray[1] ^ bitArray[0] ^ initCrcBits[0] ^
                     initCrcBits[1] ^ initCrcBits[2] ^ initCrcBits[3] ^
                     initCrcBits[5] ^ initCrcBits[6] ^ initCrcBits[8] ^
                     initCrcBits[10] ^ initCrcBits[11] ^ initCrcBits[13] ^
                     initCrcBits[14] ^ initCrcBits[18] ^ initCrcBits[19] ^
                     initCrcBits[20] ^ initCrcBits[22] ^ initCrcBits[25] ^
                     initCrcBits[27] ^ initCrcBits[28] ^ initCrcBits[31]);



    tempCrcVal[9] = (bitArray[61] ^ bitArray[60] ^ bitArray[58] ^
                     bitArray[55] ^ bitArray[53] ^ bitArray[52] ^
                     bitArray[51] ^ bitArray[47] ^ bitArray[46] ^
                     bitArray[44] ^ bitArray[43] ^ bitArray[41] ^
                     bitArray[39] ^ bitArray[38] ^ bitArray[36] ^
                     bitArray[35] ^ bitArray[34] ^ bitArray[33] ^
                     bitArray[32] ^ bitArray[29] ^ bitArray[24] ^
                     bitArray[23] ^ bitArray[18] ^ bitArray[13] ^
                     bitArray[12] ^ bitArray[11] ^ bitArray[9] ^
                     bitArray[5] ^ bitArray[4] ^ bitArray[2] ^
                     bitArray[1] ^ initCrcBits[0] ^ initCrcBits[1] ^
                     initCrcBits[2] ^ initCrcBits[3] ^ initCrcBits[4] ^
                     initCrcBits[6] ^ initCrcBits[7] ^ initCrcBits[9] ^
                     initCrcBits[11] ^ initCrcBits[12] ^ initCrcBits[14] ^
                     initCrcBits[15] ^ initCrcBits[19] ^ initCrcBits[20] ^
                     initCrcBits[21] ^ initCrcBits[23] ^ initCrcBits[26] ^
                     initCrcBits[28] ^ initCrcBits[29]);

    tempCrcVal[10] = (bitArray[63] ^ bitArray[62] ^ bitArray[60] ^
                      bitArray[59] ^ bitArray[58] ^ bitArray[56] ^
                      bitArray[55] ^ bitArray[52] ^ bitArray[50] ^
                      bitArray[42] ^ bitArray[40] ^ bitArray[39] ^
                      bitArray[36] ^ bitArray[35] ^ bitArray[33] ^
                      bitArray[32] ^ bitArray[31] ^ bitArray[29] ^
                      bitArray[28] ^ bitArray[26] ^ bitArray[19] ^
                      bitArray[16] ^ bitArray[14] ^ bitArray[13] ^
                      bitArray[9] ^ bitArray[5] ^ bitArray[3] ^
                      bitArray[2] ^ bitArray[0] ^ initCrcBits[0] ^
                      initCrcBits[1] ^ initCrcBits[3] ^ initCrcBits[4] ^
                      initCrcBits[7] ^ initCrcBits[8] ^ initCrcBits[10] ^
                      initCrcBits[18] ^ initCrcBits[20] ^ initCrcBits[23] ^
                      initCrcBits[24] ^ initCrcBits[26] ^ initCrcBits[27] ^
                      initCrcBits[28] ^ initCrcBits[30] ^ initCrcBits[31]);


    tempCrcVal[11] = (bitArray[59] ^ bitArray[58] ^ bitArray[57] ^
                      bitArray[56] ^ bitArray[55] ^ bitArray[54] ^
                      bitArray[51] ^ bitArray[50] ^ bitArray[48] ^
                      bitArray[47] ^ bitArray[45] ^ bitArray[44] ^
                      bitArray[43] ^ bitArray[41] ^ bitArray[40] ^
                      bitArray[36] ^ bitArray[33] ^ bitArray[31] ^
                      bitArray[28] ^ bitArray[27] ^ bitArray[26] ^
                      bitArray[25] ^ bitArray[24] ^ bitArray[20] ^
                      bitArray[17] ^ bitArray[16] ^ bitArray[15] ^
                      bitArray[14] ^ bitArray[12] ^ bitArray[9] ^
                      bitArray[4] ^ bitArray[3] ^ bitArray[1] ^
                      bitArray[0] ^ initCrcBits[1] ^ initCrcBits[4] ^
                      initCrcBits[8] ^ initCrcBits[9] ^ initCrcBits[11] ^
                      initCrcBits[12] ^ initCrcBits[13] ^ initCrcBits[15] ^
                      initCrcBits[16] ^ initCrcBits[18] ^ initCrcBits[19] ^
                      initCrcBits[22] ^ initCrcBits[23] ^ initCrcBits[24] ^
                      initCrcBits[25] ^ initCrcBits[26] ^ initCrcBits[27]);


    tempCrcVal[12] = (bitArray[63] ^ bitArray[61] ^ bitArray[59] ^
                      bitArray[57] ^ bitArray[56] ^ bitArray[54] ^
                      bitArray[53] ^  bitArray[52] ^ bitArray[51] ^
                      bitArray[50] ^ bitArray[49] ^  bitArray[47] ^
                      bitArray[46] ^ bitArray[42] ^ bitArray[41] ^
                      bitArray[31] ^ bitArray[30] ^ bitArray[27] ^
                      bitArray[24] ^  bitArray[21] ^ bitArray[18] ^
                      bitArray[17] ^ bitArray[15] ^  bitArray[13] ^
                      bitArray[12] ^ bitArray[9] ^ bitArray[6] ^
                      bitArray[5] ^ bitArray[4] ^ bitArray[2] ^
                      bitArray[1] ^  bitArray[0] ^ initCrcBits[9] ^
                      initCrcBits[10] ^ initCrcBits[14] ^  initCrcBits[15] ^
                      initCrcBits[17] ^ initCrcBits[18] ^ initCrcBits[19] ^
                      initCrcBits[20] ^ initCrcBits[21] ^ initCrcBits[22] ^
                      initCrcBits[24] ^  initCrcBits[25] ^ initCrcBits[27] ^
                      initCrcBits[29] ^  initCrcBits[31]);

    tempCrcVal[13] = (bitArray[62] ^ bitArray[60] ^ bitArray[58] ^
                      bitArray[57] ^ bitArray[55] ^ bitArray[54] ^
                      bitArray[53] ^ bitArray[52] ^ bitArray[51] ^
                      bitArray[50] ^ bitArray[48] ^ bitArray[47] ^
                      bitArray[43] ^ bitArray[42] ^ bitArray[32] ^
                      bitArray[31] ^ bitArray[28] ^ bitArray[25] ^
                      bitArray[22] ^ bitArray[19] ^ bitArray[18] ^
                      bitArray[16] ^ bitArray[14] ^ bitArray[13] ^
                      bitArray[10] ^ bitArray[7] ^ bitArray[6] ^
                      bitArray[5] ^ bitArray[3] ^ bitArray[2] ^
                      bitArray[1] ^ initCrcBits[0] ^ initCrcBits[10] ^
                      initCrcBits[11] ^ initCrcBits[15] ^ initCrcBits[16] ^
                      initCrcBits[18] ^ initCrcBits[19] ^ initCrcBits[20] ^
                      initCrcBits[21] ^ initCrcBits[22] ^ initCrcBits[23] ^
                      initCrcBits[25] ^ initCrcBits[26] ^ initCrcBits[28] ^
                      initCrcBits[30]);


    tempCrcVal[14] = (bitArray[63] ^ bitArray[61] ^ bitArray[59] ^
                      bitArray[58] ^ bitArray[56] ^ bitArray[55] ^
                      bitArray[54] ^  bitArray[53] ^ bitArray[52] ^
                      bitArray[51] ^ bitArray[49] ^  bitArray[48] ^
                      bitArray[44] ^ bitArray[43] ^ bitArray[33] ^
                      bitArray[32] ^ bitArray[29] ^ bitArray[26] ^
                      bitArray[23] ^  bitArray[20] ^ bitArray[19] ^
                      bitArray[17] ^ bitArray[15] ^  bitArray[14] ^
                      bitArray[11] ^ bitArray[8] ^ bitArray[7] ^
                      bitArray[6] ^ bitArray[4] ^ bitArray[3] ^
                      bitArray[2] ^  initCrcBits[0] ^ initCrcBits[1] ^
                      initCrcBits[11] ^ initCrcBits[12] ^  initCrcBits[16] ^
                      initCrcBits[17] ^ initCrcBits[19] ^ initCrcBits[20] ^
                      initCrcBits[21] ^ initCrcBits[22] ^ initCrcBits[23] ^
                      initCrcBits[24] ^ initCrcBits[26] ^ initCrcBits[27] ^
                      initCrcBits[29] ^ initCrcBits[31]);


    tempCrcVal[15] = (bitArray[62] ^ bitArray[60] ^ bitArray[59] ^
                      bitArray[57] ^ bitArray[56] ^ bitArray[55] ^
                      bitArray[54] ^ bitArray[53] ^ bitArray[52] ^
                      bitArray[50] ^ bitArray[49] ^ bitArray[45] ^
                      bitArray[44] ^ bitArray[34] ^ bitArray[33] ^
                      bitArray[30] ^ bitArray[27] ^ bitArray[24] ^
                      bitArray[21] ^ bitArray[20] ^ bitArray[18] ^
                      bitArray[16] ^ bitArray[15] ^ bitArray[12] ^
                      bitArray[9] ^ bitArray[8] ^ bitArray[7] ^
                      bitArray[5] ^ bitArray[4] ^ bitArray[3] ^
                      initCrcBits[1] ^ initCrcBits[2] ^ initCrcBits[12] ^
                      initCrcBits[13] ^ initCrcBits[17] ^ initCrcBits[18] ^
                      initCrcBits[20] ^ initCrcBits[21] ^ initCrcBits[22] ^
                      initCrcBits[23] ^ initCrcBits[24] ^ initCrcBits[25] ^
                      initCrcBits[27] ^ initCrcBits[28] ^ initCrcBits[30]);


    tempCrcVal[16] = (bitArray[57] ^ bitArray[56] ^ bitArray[51] ^
                      bitArray[48] ^ bitArray[47] ^ bitArray[46] ^
                      bitArray[44] ^ bitArray[37] ^ bitArray[35] ^
                      bitArray[32] ^ bitArray[30] ^ bitArray[29] ^
                      bitArray[26] ^ bitArray[24] ^ bitArray[22] ^
                      bitArray[21] ^ bitArray[19] ^ bitArray[17] ^
                      bitArray[13] ^ bitArray[12] ^ bitArray[8] ^
                      bitArray[5] ^ bitArray[4] ^ bitArray[0] ^
                      initCrcBits[0] ^ initCrcBits[3] ^ initCrcBits[5] ^
                      initCrcBits[12] ^ initCrcBits[14] ^ initCrcBits[15] ^
                      initCrcBits[16] ^ initCrcBits[19] ^ initCrcBits[24] ^
                      initCrcBits[25]);


    tempCrcVal[17] = (bitArray[58] ^ bitArray[57] ^ bitArray[52] ^
                      bitArray[49] ^ bitArray[48] ^ bitArray[47] ^
                      bitArray[45] ^ bitArray[38] ^ bitArray[36] ^
                      bitArray[33] ^ bitArray[31] ^ bitArray[30] ^
                      bitArray[27] ^ bitArray[25] ^ bitArray[23] ^
                      bitArray[22] ^ bitArray[20] ^ bitArray[18] ^
                      bitArray[14] ^ bitArray[13] ^ bitArray[9] ^
                      bitArray[6] ^ bitArray[5] ^ bitArray[1] ^
                      initCrcBits[1] ^ initCrcBits[4] ^ initCrcBits[6] ^
                      initCrcBits[13] ^ initCrcBits[15] ^ initCrcBits[16] ^
                      initCrcBits[17] ^ initCrcBits[20] ^ initCrcBits[25] ^
                      initCrcBits[26]);

    tempCrcVal[18] = (bitArray[59] ^ bitArray[58] ^ bitArray[53] ^
                      bitArray[50] ^ bitArray[49] ^ bitArray[48] ^
                      bitArray[46] ^ bitArray[39] ^ bitArray[37] ^
                      bitArray[34] ^ bitArray[32] ^ bitArray[31] ^
                      bitArray[28] ^ bitArray[26] ^ bitArray[24] ^
                      bitArray[23] ^ bitArray[21] ^ bitArray[19] ^
                      bitArray[15] ^ bitArray[14] ^ bitArray[10] ^
                      bitArray[7] ^ bitArray[6] ^ bitArray[2] ^
                      initCrcBits[0] ^ initCrcBits[2] ^ initCrcBits[5] ^
                      initCrcBits[7] ^ initCrcBits[14] ^ initCrcBits[16] ^
                      initCrcBits[17] ^ initCrcBits[18] ^ initCrcBits[21] ^
                      initCrcBits[26] ^ initCrcBits[27]);


    tempCrcVal[19] = (bitArray[60] ^ bitArray[59] ^ bitArray[54] ^
                      bitArray[51] ^ bitArray[50] ^ bitArray[49] ^
                      bitArray[47] ^ bitArray[40] ^ bitArray[38] ^
                      bitArray[35] ^ bitArray[33] ^ bitArray[32] ^
                      bitArray[29] ^ bitArray[27] ^ bitArray[25] ^
                      bitArray[24] ^ bitArray[22] ^ bitArray[20] ^
                      bitArray[16] ^ bitArray[15] ^ bitArray[11] ^
                      bitArray[8] ^ bitArray[7] ^ bitArray[3] ^
                      initCrcBits[0] ^ initCrcBits[1] ^ initCrcBits[3] ^
                      initCrcBits[6] ^ initCrcBits[8] ^ initCrcBits[15] ^
                      initCrcBits[17] ^ initCrcBits[18] ^ initCrcBits[19] ^
                      initCrcBits[22] ^ initCrcBits[27] ^ initCrcBits[28]);

    tempCrcVal[20] = (bitArray[61] ^ bitArray[60] ^ bitArray[55] ^
                      bitArray[52] ^ bitArray[51] ^ bitArray[50] ^
                      bitArray[48] ^ bitArray[41] ^ bitArray[39] ^
                      bitArray[36] ^ bitArray[34] ^ bitArray[33] ^
                      bitArray[30] ^ bitArray[28] ^ bitArray[26] ^
                      bitArray[25] ^ bitArray[23] ^ bitArray[21] ^
                      bitArray[17] ^ bitArray[16] ^ bitArray[12] ^
                      bitArray[9] ^ bitArray[8] ^ bitArray[4] ^
                      initCrcBits[1] ^ initCrcBits[2] ^ initCrcBits[4] ^
                      initCrcBits[7] ^ initCrcBits[9] ^ initCrcBits[16] ^
                      initCrcBits[18] ^ initCrcBits[19] ^ initCrcBits[20] ^
                      initCrcBits[23] ^ initCrcBits[28] ^ initCrcBits[29]);

    tempCrcVal[21] = (bitArray[62] ^ bitArray[61] ^ bitArray[56] ^
                      bitArray[53] ^ bitArray[52] ^ bitArray[51] ^
                      bitArray[49] ^ bitArray[42] ^ bitArray[40] ^
                      bitArray[37] ^ bitArray[35] ^ bitArray[34] ^
                      bitArray[31] ^ bitArray[29] ^ bitArray[27] ^
                      bitArray[26] ^ bitArray[24] ^ bitArray[22] ^
                      bitArray[18] ^ bitArray[17] ^ bitArray[13] ^
                      bitArray[10] ^ bitArray[9] ^ bitArray[5] ^
                      initCrcBits[2] ^ initCrcBits[3] ^ initCrcBits[5] ^
                      initCrcBits[8] ^ initCrcBits[10] ^ initCrcBits[17] ^
                      initCrcBits[19] ^ initCrcBits[20] ^ initCrcBits[21] ^
                      initCrcBits[24] ^ initCrcBits[29] ^ initCrcBits[30]);


    tempCrcVal[22] = (bitArray[62] ^ bitArray[61] ^ bitArray[60] ^
                      bitArray[58] ^ bitArray[57] ^ bitArray[55] ^
                      bitArray[52] ^ bitArray[48] ^ bitArray[47] ^
                      bitArray[45] ^ bitArray[44] ^ bitArray[43] ^
                      bitArray[41] ^ bitArray[38] ^ bitArray[37] ^
                      bitArray[36] ^ bitArray[35] ^ bitArray[34] ^
                      bitArray[31] ^ bitArray[29] ^ bitArray[27] ^
                      bitArray[26] ^ bitArray[24] ^ bitArray[23] ^
                      bitArray[19] ^ bitArray[18] ^ bitArray[16] ^
                      bitArray[14] ^ bitArray[12] ^ bitArray[11] ^
                      bitArray[9] ^ bitArray[0] ^ initCrcBits[2] ^
                      initCrcBits[3] ^ initCrcBits[4] ^ initCrcBits[5] ^
                      initCrcBits[6] ^ initCrcBits[9] ^ initCrcBits[11] ^
                      initCrcBits[12] ^ initCrcBits[13] ^ initCrcBits[15] ^
                      initCrcBits[16] ^ initCrcBits[20] ^ initCrcBits[23] ^
                      initCrcBits[25] ^ initCrcBits[26] ^ initCrcBits[28] ^
                      initCrcBits[29] ^ initCrcBits[30]);

    tempCrcVal[23] = (bitArray[62] ^ bitArray[60] ^ bitArray[59] ^
                      bitArray[56] ^ bitArray[55] ^ bitArray[54] ^
                      bitArray[50] ^ bitArray[49] ^ bitArray[47] ^
                      bitArray[46] ^ bitArray[42] ^ bitArray[39] ^
                      bitArray[38] ^ bitArray[36] ^ bitArray[35] ^
                      bitArray[34] ^ bitArray[31] ^ bitArray[29] ^
                      bitArray[27] ^ bitArray[26] ^ bitArray[20] ^
                      bitArray[19] ^ bitArray[17] ^ bitArray[16] ^
                      bitArray[15] ^ bitArray[13] ^ bitArray[9] ^
                      bitArray[6] ^ bitArray[1] ^ bitArray[0] ^
                      initCrcBits[2] ^ initCrcBits[3] ^ initCrcBits[4] ^
                      initCrcBits[6] ^ initCrcBits[7] ^ initCrcBits[10] ^
                      initCrcBits[14] ^ initCrcBits[15] ^ initCrcBits[17] ^
                      initCrcBits[18] ^ initCrcBits[22] ^ initCrcBits[23] ^
                      initCrcBits[24] ^ initCrcBits[27] ^ initCrcBits[28] ^
                      initCrcBits[30]);

    tempCrcVal[24] = (bitArray[63] ^ bitArray[61] ^ bitArray[60] ^
                      bitArray[57] ^ bitArray[56] ^ bitArray[55] ^
                      bitArray[51] ^ bitArray[50] ^ bitArray[48] ^
                      bitArray[47] ^ bitArray[43] ^ bitArray[40] ^
                      bitArray[39] ^ bitArray[37] ^ bitArray[36] ^
                      bitArray[35] ^ bitArray[32] ^ bitArray[30] ^
                      bitArray[28] ^ bitArray[27] ^ bitArray[21] ^
                      bitArray[20] ^ bitArray[18] ^ bitArray[17] ^
                      bitArray[16] ^ bitArray[14] ^ bitArray[10] ^
                      bitArray[7] ^ bitArray[2] ^ bitArray[1] ^
                      initCrcBits[0] ^ initCrcBits[3] ^ initCrcBits[4] ^
                      initCrcBits[5] ^ initCrcBits[7] ^ initCrcBits[8] ^
                      initCrcBits[11] ^ initCrcBits[15] ^ initCrcBits[16] ^
                      initCrcBits[18] ^ initCrcBits[19] ^ initCrcBits[23] ^
                      initCrcBits[24] ^ initCrcBits[25] ^ initCrcBits[28] ^
                      initCrcBits[29] ^ initCrcBits[31]);


    tempCrcVal[25] = (bitArray[62] ^ bitArray[61] ^
                      bitArray[58] ^  bitArray[57] ^ bitArray[56] ^
                      bitArray[52] ^ bitArray[51] ^  bitArray[49] ^
                      bitArray[48] ^ bitArray[44] ^ bitArray[41] ^
                      bitArray[40] ^ bitArray[38] ^ bitArray[37] ^
                      bitArray[36] ^  bitArray[33] ^ bitArray[31] ^
                      bitArray[29] ^ bitArray[28] ^  bitArray[22] ^
                      bitArray[21] ^ bitArray[19] ^ bitArray[18] ^
                      bitArray[17] ^ bitArray[15] ^ bitArray[11] ^
                      bitArray[8] ^ bitArray[3] ^ bitArray[2] ^
                      initCrcBits[1] ^ initCrcBits[4] ^ initCrcBits[5] ^
                      initCrcBits[6] ^ initCrcBits[8] ^ initCrcBits[9] ^
                      initCrcBits[12] ^ initCrcBits[16] ^ initCrcBits[17] ^
                      initCrcBits[19] ^ initCrcBits[20] ^ initCrcBits[24] ^
                      initCrcBits[25] ^ initCrcBits[26] ^ initCrcBits[29] ^
                      initCrcBits[30]);


    tempCrcVal[26] = (bitArray[62] ^ bitArray[61] ^ bitArray[60] ^
                      bitArray[59] ^ bitArray[57] ^ bitArray[55] ^
                      bitArray[54] ^ bitArray[52] ^ bitArray[49] ^
                      bitArray[48] ^ bitArray[47] ^ bitArray[44] ^
                      bitArray[42] ^ bitArray[41] ^ bitArray[39] ^
                      bitArray[38] ^ bitArray[31] ^ bitArray[28] ^
                      bitArray[26] ^ bitArray[25] ^ bitArray[24] ^
                      bitArray[23] ^ bitArray[22] ^ bitArray[20] ^
                      bitArray[19] ^ bitArray[18] ^ bitArray[10] ^
                      bitArray[6] ^ bitArray[4] ^ bitArray[3] ^
                      bitArray[0] ^ initCrcBits[6] ^ initCrcBits[7] ^
                      initCrcBits[9] ^ initCrcBits[10] ^ initCrcBits[12] ^
                      initCrcBits[15] ^ initCrcBits[16] ^ initCrcBits[17] ^
                      initCrcBits[20] ^ initCrcBits[22] ^ initCrcBits[23] ^
                      initCrcBits[25] ^ initCrcBits[27] ^ initCrcBits[28] ^
                      initCrcBits[29] ^ initCrcBits[30]);

    tempCrcVal[27] = (bitArray[63] ^ bitArray[62] ^ bitArray[61] ^
                      bitArray[60] ^ bitArray[58] ^ bitArray[56] ^
                      bitArray[55] ^ bitArray[53] ^ bitArray[50] ^
                      bitArray[49] ^ bitArray[48] ^ bitArray[45] ^
                      bitArray[43] ^ bitArray[42] ^ bitArray[40] ^
                      bitArray[39] ^ bitArray[32] ^ bitArray[29] ^
                      bitArray[27] ^ bitArray[26] ^ bitArray[25] ^
                      bitArray[24] ^ bitArray[23] ^ bitArray[21] ^
                      bitArray[20] ^ bitArray[19] ^ bitArray[11] ^
                      bitArray[7] ^ bitArray[5] ^ bitArray[4] ^
                      bitArray[1] ^ initCrcBits[0] ^ initCrcBits[7] ^
                      initCrcBits[8] ^ initCrcBits[10] ^ initCrcBits[11] ^
                      initCrcBits[13] ^ initCrcBits[16] ^ initCrcBits[17] ^
                      initCrcBits[18] ^ initCrcBits[21] ^ initCrcBits[23] ^
                      initCrcBits[24] ^ initCrcBits[26] ^ initCrcBits[28] ^
                      initCrcBits[29] ^ initCrcBits[30] ^ initCrcBits[31]);


    tempCrcVal[28] = (bitArray[63] ^ bitArray[62] ^ bitArray[61] ^
                      bitArray[59] ^ bitArray[57] ^ bitArray[56] ^
                      bitArray[54] ^ bitArray[51] ^ bitArray[50] ^
                      bitArray[49] ^ bitArray[46] ^ bitArray[44] ^
                      bitArray[43] ^ bitArray[41] ^ bitArray[40] ^
                      bitArray[33] ^ bitArray[30] ^ bitArray[28] ^
                      bitArray[27] ^ bitArray[26] ^ bitArray[25] ^
                      bitArray[24] ^ bitArray[22] ^ bitArray[21] ^
                      bitArray[20] ^ bitArray[12] ^ bitArray[8] ^
                      bitArray[6] ^ bitArray[5] ^ bitArray[2] ^
                      initCrcBits[1] ^ initCrcBits[8] ^ initCrcBits[9] ^
                      initCrcBits[11] ^ initCrcBits[12] ^ initCrcBits[14] ^
                      initCrcBits[17] ^ initCrcBits[18] ^ initCrcBits[19] ^
                      initCrcBits[22] ^ initCrcBits[24] ^ initCrcBits[25] ^
                      initCrcBits[27] ^ initCrcBits[29] ^ initCrcBits[30] ^
                      initCrcBits[31]);

    tempCrcVal[29] = (bitArray[63] ^ bitArray[62] ^ bitArray[60] ^
                      bitArray[58] ^ bitArray[57] ^ bitArray[55] ^
                      bitArray[52] ^ bitArray[51] ^ bitArray[50] ^
                      bitArray[47] ^ bitArray[45] ^ bitArray[44] ^
                      bitArray[42] ^ bitArray[41] ^ bitArray[34] ^
                      bitArray[31] ^ bitArray[29] ^ bitArray[28] ^
                      bitArray[27] ^ bitArray[26] ^ bitArray[25] ^
                      bitArray[23] ^ bitArray[22] ^ bitArray[21] ^
                      bitArray[13] ^ bitArray[9] ^ bitArray[7] ^
                      bitArray[6] ^ bitArray[3] ^ initCrcBits[2] ^
                      initCrcBits[9] ^ initCrcBits[10] ^ initCrcBits[12] ^
                      initCrcBits[13] ^ initCrcBits[15] ^ initCrcBits[18] ^
                      initCrcBits[19] ^ initCrcBits[20] ^ initCrcBits[23] ^
                      initCrcBits[25] ^ initCrcBits[26] ^ initCrcBits[28] ^
                      initCrcBits[30] ^ initCrcBits[31]);

    tempCrcVal[30] = (bitArray[63] ^ bitArray[61] ^ bitArray[59] ^
                      bitArray[58] ^ bitArray[56] ^ bitArray[53] ^
                      bitArray[52] ^ bitArray[51] ^ bitArray[48] ^
                      bitArray[46] ^ bitArray[45] ^ bitArray[43] ^
                      bitArray[42] ^ bitArray[35] ^ bitArray[32] ^
                      bitArray[30] ^ bitArray[29] ^ bitArray[28] ^
                      bitArray[27] ^ bitArray[26] ^ bitArray[24] ^
                      bitArray[23] ^ bitArray[22] ^ bitArray[14] ^
                      bitArray[10] ^ bitArray[8] ^ bitArray[7] ^
                      bitArray[4] ^ initCrcBits[0] ^ initCrcBits[3] ^
                      initCrcBits[10] ^ initCrcBits[11] ^ initCrcBits[13] ^
                      initCrcBits[14] ^ initCrcBits[16] ^ initCrcBits[19] ^
                      initCrcBits[20] ^ initCrcBits[21] ^ initCrcBits[24] ^
                      initCrcBits[26] ^ initCrcBits[27] ^ initCrcBits[29] ^
                      initCrcBits[31]);

    tempCrcVal[31] = (bitArray[62] ^ bitArray[60] ^ bitArray[59] ^
                      bitArray[57] ^ bitArray[54] ^ bitArray[53] ^
                      bitArray[52] ^ bitArray[49] ^ bitArray[47] ^
                      bitArray[46] ^ bitArray[44] ^ bitArray[43] ^
                      bitArray[36] ^ bitArray[33] ^ bitArray[31] ^
                      bitArray[30] ^ bitArray[29] ^ bitArray[28] ^
                      bitArray[27] ^ bitArray[25] ^ bitArray[24] ^
                      bitArray[23] ^ bitArray[15] ^ bitArray[11] ^
                      bitArray[9] ^ bitArray[8] ^ bitArray[5] ^
                      initCrcBits[1] ^ initCrcBits[4] ^ initCrcBits[11] ^
                      initCrcBits[12] ^ initCrcBits[14] ^ initCrcBits[15] ^
                      initCrcBits[17] ^ initCrcBits[20] ^ initCrcBits[21] ^
                      initCrcBits[22] ^ initCrcBits[25] ^ initCrcBits[27] ^
                      initCrcBits[28] ^ initCrcBits[30]);



    result = 0;

    for (i = 0; i < 32; i++)
    {
        result = result | (tempCrcVal[i] << i);
    }

    return result;
}

