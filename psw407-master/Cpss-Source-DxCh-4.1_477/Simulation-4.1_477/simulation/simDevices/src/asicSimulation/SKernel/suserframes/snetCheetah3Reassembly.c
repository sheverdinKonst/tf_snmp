/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Reassembly.c
*
* DESCRIPTION:
*       This is the implementation for the Reassembly engine for cheetah
*       module of SKernel.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <common/Utils/PresteraHash/smacHashDx.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Reassembly.h>
#include <asicSimulation/SLog/simLog.h>

/* see value in function cheetahMacHashCalc(...)
   for the case of : CRC + IPV4 IGMP
*/
#define HAHS_CRC_VALUE      0x0101

#define HASH_TYPE        ((HAHS_CRC_VALUE) >> 8)
#define FDB_ENTRY_TYPE   ((HAHS_CRC_VALUE) & 0xff)
#define VLAN_LOOKUP_MODE 0xffffffff /*don't care*/
#define MAC_ADDR_PTR       NULL     /*don't care*/
#define VLAN_ID(fragmentId)     fragmentId

/* we need 2<<NUM_BITS_TO_USE == SNET_CHT3_CAPWAP_REASSEMBLY_NUM_ENTRIES */
#define NUM_BITS_TO_USE     11

/* the lookup not found free index for new entry */
#define LOOKUP_INDEX_NOT_FOUND_NO_FREE_SPACE    0xFFFFFFFF

/* number of lookups before 'not found' */
#define LOOKUP_LEN  4

/* new/fresh entry in the reassembly table */
#define NEW_FRESH_ENTRY     1
/* old aged entry in the reassembly table */
#define OLD_AGED_ENTRY      0


#define NEW_DESC_PTR_MAC(devObjPtr,descriptorIndex)\
    &(((SKERNEL_FRAME_CHEETAH_DESCR_STC*)(devObjPtr)->descriptorPtr)[(devObjPtr)->capwapReassemblyFirstDescriptorIndex + (descriptorIndex)])

/*******************************************************************************
*   snetCht3ReassemblyCapwap
*
* DESCRIPTION:
*        CAPWAP Reassembly engine
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       keyPtr       - pointer to the key of the reassembly/fragment
*
*
* OUTPUTS:
*       foundPtr - (pointer to) key was found/not
*       indexPtr - (pointer to) index of free/found entry
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void snetCht3ReassemblyCapwapLookup
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_CAPWAP_REASSEMBLY_KEY_STC *  keyPtr,
    OUT GT_BOOL         *foundPtr,
    OUT GT_U32          *indexPtr
)
{
    GT_U32  startIndex;
    GT_U32  ii;
    GT_BOOL firstFreeSet = GT_FALSE;

    startIndex = cheetahMacHashCalc(devObjPtr,
                        HASH_TYPE,
                        VLAN_LOOKUP_MODE,
                        FDB_ENTRY_TYPE,
                        MAC_ADDR_PTR,
                        VLAN_ID(keyPtr->fragmentId),
                        keyPtr->isIpv4 ? keyPtr->srcIp[0] : keyPtr->srcIp[3] ,
                        keyPtr->isIpv4 ? keyPtr->dstIp[0] : keyPtr->dstIp[3] ,
                        NUM_BITS_TO_USE);

    /* Set init value */
    *indexPtr = 0;
    /* from this index start to look for a match/not found , up to LOOKUP_LEN indexes */
    for(ii = startIndex; ii < (startIndex + LOOKUP_LEN) ; ii++)
    {
        if(devObjPtr->capwapReassemblyTablePtr[ii].isUsed == 0)
        {
            if(firstFreeSet == GT_FALSE)
            {
                *indexPtr = ii;/* this index is free and will be used as new entry
                                  if needed */
                firstFreeSet = GT_TRUE;
            }
        }
        else if(devObjPtr->capwapReassemblyTablePtr[ii].isUsed == 1 &&
            (0 == memcmp(&devObjPtr->capwapReassemblyTablePtr[ii].key,
                keyPtr,
                sizeof(SKERNEL_CAPWAP_REASSEMBLY_KEY_STC))))
        {
            /* match found */
            *foundPtr = GT_TRUE;
            *indexPtr = ii;
            return;
        }
    }

    if(firstFreeSet == GT_FALSE)
    {
        /* the lookup not found free index , and no free space */
        *indexPtr = LOOKUP_INDEX_NOT_FOUND_NO_FREE_SPACE;
    }

    *foundPtr = GT_FALSE;

    return;
}

/*******************************************************************************
*   snetCht3ReassemblyCapwapFreeDescriptorGet
*
* DESCRIPTION:
*        CAPWAP Reassembly engine - get free descriptor to copy .
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*
*
* OUTPUTS:
*       indexPtr - (pointer to) index of free descriptor
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void snetCht3ReassemblyCapwapFreeDescriptorGet
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    OUT GT_U32          *indexPtr
)
{
    GT_U32  ii,jj;
    GT_U32  index;
    *indexPtr = LOOKUP_INDEX_NOT_FOUND_NO_FREE_SPACE;

    for(ii = 0 ;  ii < SNET_CHT3_CAPWAP_REASSEMBLY_NUM_ENTRIES_CNS ; ii++)
    {
        index = (ii*32);
        if(devObjPtr->capwapReassemblyDescriptorsUsedBmpPtr[ii] == 0xFFFFFFFF)
        {
            continue;
        }

        for(jj = 0 ; jj < 32; jj++,index++)
        {
            if(0 == SMAIN_INDEX_IN_BMP_ARRAY_GET_MAC(devObjPtr->capwapReassemblyDescriptorsUsedBmpPtr,index))
            {
                /* found index */
                *indexPtr = index;
                return;
            }
        }

        /* should not happen */
        break;
    }

    /* no free space ??? */
    return;
}

/*******************************************************************************
*   snetCht3ReassemblyCapwap
*
* DESCRIPTION:
*        CAPWAP Reassembly engine
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*       keyPtr       - pointer to the key of the reassembly/fragment
*
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_TRUE  - when this frame complete the assembly of waiting frames
*       GT_FALSE - when this frame is not the last needed fragment
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL snetCht3ReassemblyCapwap
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN SKERNEL_CAPWAP_REASSEMBLY_KEY_STC *  keyPtr
)
{
    DECLARE_FUNC_NAME(snetCht3ReassemblyCapwap);

    GT_BOOL found;/* was key found */
    GT_U32  index;/* index of the found entry or index for new entry */
    GT_U32  descriptorIndex;/* index of the descriptor that will hold the copy of current frame descriptor */

    if(keyPtr->fragmentId >= SNET_CHT3_CAPWAP_REASSEMBLY_NUM_ENTRIES_CNS)
    {
        skernelFatalError("snetCht3ReassemblyCapwap: fragment id[%d] >= max num entries [%d]\n",
            keyPtr->fragmentId,SNET_CHT3_CAPWAP_REASSEMBLY_NUM_ENTRIES_CNS);
    }
    else if(keyPtr->fragmentId > 0x0FFF)
    {
        /* if more then 12 bits -- maybe needed new hashing method */
        skernelFatalError("snetCht3ReassemblyCapwap: implementation use 12 bits as vid in hashing, but fragment Id[%d]>0xfff\n",
            keyPtr->fragmentId);
    }

    /* look for existing key , or new place for it */
    __LOG(("look for existing key , or new place for it"));
    snetCht3ReassemblyCapwapLookup(devObjPtr,keyPtr,&found,&index);

    if(found == GT_TRUE)
    {
        /* key already exists */
        __LOG(("key already exists"));

        if(devObjPtr->capwapReassemblyTablePtr[index].frameBufferPtr == NULL)
        {
            /* get free descriptor index to hold the copy */
            __LOG(("get free descriptor index to hold the copy"));
            snetCht3ReassemblyCapwapFreeDescriptorGet(devObjPtr,&descriptorIndex);
            if(descriptorIndex == LOOKUP_INDEX_NOT_FOUND_NO_FREE_SPACE)
            {
                /* can't hold this fragment */
                __LOG(("can't hold this fragment"));
                skernelFatalError("snetCht3ReassemblyCapwap: no free descriptors to copy to");
            }

            /* take new descriptor */
            __LOG(("take new descriptor"));
            SMAIN_INDEX_IN_BMP_ARRAY_SET_MAC(devObjPtr->capwapReassemblyDescriptorsUsedBmpPtr,descriptorIndex);

            /* copy current descriptor to new descriptor */
            __LOG(("copy current descriptor to new descriptor"));
            *(NEW_DESC_PTR_MAC(devObjPtr,descriptorIndex)) = *descrPtr;

            /* update the buffer and descriptor pointers */
            __LOG(("update the buffer and descriptor pointers"));
            devObjPtr->capwapReassemblyTablePtr[index].descriptorPtr = NEW_DESC_PTR_MAC(devObjPtr,descriptorIndex);
            devObjPtr->capwapReassemblyTablePtr[index].frameBufferPtr = descrPtr->frameBuf;
        }

        /* update the age value */
        __LOG(("update the age value"));
        devObjPtr->capwapReassemblyTablePtr[index].ageValue = NEW_FRESH_ENTRY;

        /* we have nothing to do with the new frame ! */
        __LOG(("we have nothing to do with the new frame !"));
        return GT_FALSE;
    }
    else if(LOOKUP_INDEX_NOT_FOUND_NO_FREE_SPACE == index)
    {
        /* do nothing with this frame */
        __LOG(("do nothing with this frame"));
        return GT_FALSE;
    }
    else
    {
        /* get free descriptor index to hold the copy */
        __LOG(("get free descriptor index to hold the copy"));
        snetCht3ReassemblyCapwapFreeDescriptorGet(devObjPtr,&descriptorIndex);
        if(descriptorIndex == LOOKUP_INDEX_NOT_FOUND_NO_FREE_SPACE)
        {
            /* can't hold this fragment */
            __LOG(("can't hold this fragment"));
            skernelFatalError("snetCht3ReassemblyCapwap: no free descriptors to copy to");
        }

        /* take new descriptor */
        __LOG(("take new descriptor"));
        SMAIN_INDEX_IN_BMP_ARRAY_SET_MAC(devObjPtr->capwapReassemblyDescriptorsUsedBmpPtr,descriptorIndex);

        /* copy current descriptor to new descriptor */
        __LOG(("copy current descriptor to new descriptor"));
        *(NEW_DESC_PTR_MAC(devObjPtr,descriptorIndex)) = *descrPtr;

        /* tell the allocator of this buffer , that we take full responsibility
           to free the buffer and the descriptor ! */
        descrPtr->frameBuf->freeState = SBUF_BUFFER_STATE_OTHER_WILL_FREE_E;

        /* set the entry as used with the needed info */
        devObjPtr->capwapReassemblyTablePtr[index].key = *keyPtr;
        devObjPtr->capwapReassemblyTablePtr[index].isUsed = 1;
        devObjPtr->capwapReassemblyTablePtr[index].ageValue = NEW_FRESH_ENTRY;
        devObjPtr->capwapReassemblyTablePtr[index].descriptorPtr = NEW_DESC_PTR_MAC(devObjPtr,descriptorIndex);
        devObjPtr->capwapReassemblyTablePtr[index].frameBufferPtr = descrPtr->frameBuf;
    }

    skernelFatalError("snetCht3ReassemblyCapwap: implementation not competed\n");

    return GT_FALSE;
}


