/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion3Tcam.c
*
* DESCRIPTION:
*       SIP5 Lion3 Tcam
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 16 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetLion3Tcam.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypeTcam.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

/* macro to return result of which bits caused the 'NO MATCH' */
#define X_Y_K_FIND_NO_MATCH(x,y,k,mask)  \
    ((~(((~x) & (~k)) | ((~y) & (k)))) & mask )

#define SIP5_10_HIT_SEGMENTS_IN_FLOOR_CNS   6 /* bc2 : 2 , bobk : 6 */
#define MAX_HIT_SEGMENTS_IN_FLOOR_CNS       SIP5_10_HIT_SEGMENTS_IN_FLOOR_CNS

/* size of segments info array :
    in sip5     : 2 segments per floor
    in sip 5_15 : 6 segments per floor
    in sip 5_20 : 2 segments per floor (like sip5)

*/
#define SIP5_TCAM_SEGMENTS_INFO_SIZE_CNS     (MAX_HIT_SEGMENTS_IN_FLOOR_CNS * SIP5_TCAM_MAX_NUM_OF_FLOORS_CNS)

#define TCAM_SEGMENTS_INFO_SIZE_WORDS_CNS   (CONVERT_BITS_TO_WORDS_MAC(SIP5_TCAM_SEGMENTS_INFO_SIZE_CNS))

#define NUM_HIT_SEGMENTS_IN_FLOOR_MAC(_devObjPtr)   (_devObjPtr->tcam_numBanksForHitNumGranularity)

/* number of words in key chunk */
#define SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS     3

/* number of bits in key chunk */
#define SIP5_TCAM_NUM_OF_BITS_IN_KEY_CHUNK_CNS     (32 * \
                SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS)

static GT_CHAR* tcamClientName[SIP5_TCAM_CLIENT_LAST_E + 1] =
{
    STR(SIP5_TCAM_CLIENT_TTI_E   ),
    STR(SIP5_TCAM_CLIENT_IPCL0_E ),
    STR(SIP5_TCAM_CLIENT_IPCL1_E ),
    STR(SIP5_TCAM_CLIENT_IPCL2_E ),
    STR(SIP5_TCAM_CLIENT_EPCL_E  ),

    NULL
};


/*
 * Typedef: struct SIP5_TCAM_SEGMENT_INFO_STC
 *
 * description :
 *          info about the segment in tcam to look into.
 *
 * Fields:
 *         startBankIndex : the index of the start bank (0/6)
 *         floorNum  : floor number (0..11)
 *         hitNum    : hit number   (0..3)
*/
typedef struct{
    GT_U32       startBankIndex;
    GT_U32       floorNum;
    GT_U32       hitNum;
} SIP5_TCAM_SEGMENT_INFO_STC;

#define MAX_HIT_NUM_CNS 4
/*
 * Typedef: struct TCAM_FLOOR_INFO_STC
 *
 * description :
 *          info about the floor.
 *
 * Fields:
 *         validBanksBmp : bmp of valid banks for the lookup
*/
typedef struct{
    GT_U32       validBanksBmp;
}TCAM_FLOOR_INFO_STC;

typedef struct{
    TCAM_FLOOR_INFO_STC floorInfo[SIP5_TCAM_MAX_NUM_OF_FLOORS_CNS];
}TCAM_FLOORS_INFO_STC;

/*******************************************************************************
*   snetLion3TcamGetTcamGroupId
*
* DESCRIPTION:
*       sip5 function that returns tcam group id
*
* INPUTS:
*       devObjPtr   - (pointer to) the device object
*       tcamClient  - tcam client
*
* OUTPUTS:
*       groupId     - group id, valid values 0..4
*
* RETURNS:
*       status - GT_OK        - found,
*                GT_NOT_FOUND - not found
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS snetLion3TcamGetTcamGroupId
(
    IN  SKERNEL_DEVICE_OBJECT  *devObjPtr,
    IN  SIP5_TCAM_CLIENT_ENT    tcamClient,
    OUT GT_U32                 *groupIdPtr
)
{
    GT_STATUS   st = GT_NOT_FOUND;
    GT_U32      isClientEnabled;
    GT_U32      enableCounter = 0;
    GT_U32      group;   /* 5 groups supported, 0..4 */
    GT_U32      regAddr; /* registry address */
    GT_U32      portGroupId = (devObjPtr->portGroupId & 0x3);/* serve 4 port groups */

    /* get groupId */
    for(group = 0; group < SIP5_TCAM_NUM_OF_GROUPS_CNS; group++)
    {
        regAddr = SMEM_LION3_TCAM_GROUP_CLIENT_ENABLE_REG(devObjPtr, group);
        smemRegFldGet(devObjPtr, regAddr,
                      portGroupId*SIP5_TCAM_NUM_OF_GROUPS_CNS + tcamClient,
                      1, &isClientEnabled);

        if (isClientEnabled)
        {
            /* save port group id */
            *groupIdPtr = group;
            enableCounter++;
        }
    }

    switch(enableCounter)
    {
        case 0:
            break;
        case 1:
            st = GT_OK;
            break;
        default:
            skernelFatalError(
              "snetLion3TcamGetTcamGroupId: wrong number of groups id (groupId > 1)\n");
    }

    return st;
}

/*******************************************************************************
*   snetLion3TcamSegmentsListGet
*
* DESCRIPTION:
*       sip5 function that gets tcam segments list
*
* INPUTS:
*       devObjPtr     - (pointer to) the device object
*       groupId       - tcam group id
*
* OUTPUTS:
*       tcamSegmentsInfoArr  - array of segments info
*       tcamSegmentsNumPtr - (pointer to) number of elements in tcamSegmentsInfoArr
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3TcamSegmentsListGet
(
    IN  SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN  GT_U32                    groupId,
    OUT SIP5_TCAM_SEGMENT_INFO_STC *tcamSegmentsInfoArr,
    OUT GT_U32                   *tcamSegmentsNumPtr
)
{
    GT_U32   floorNum;
    GT_U32   regAddr; /* registry address */
    GT_U32   regValue;
    GT_U32   startBankIndex, index ,step , startBitHitNum;
    GT_U32   numOfActiveFloors;

    regAddr = SMEM_LION3_TCAM_POWER_ON_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 0, 8, &numOfActiveFloors);

    /* clear out values first */
    *tcamSegmentsNumPtr = 0;

    /* get banks info */
    for(floorNum = 0; floorNum < numOfActiveFloors; floorNum++)
    {
        regAddr = SMEM_LION3_TCAM_HIT_NUM_AND_GROUP_SEL_FLOOR_REG(devObjPtr, floorNum);
        smemRegGet(devObjPtr, regAddr, &regValue);

        step = SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS /
                    NUM_HIT_SEGMENTS_IN_FLOOR_MAC(devObjPtr);

        index = 0;

        startBitHitNum = 3 * (SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS / step);

        for(startBankIndex = 0 ;
            startBankIndex < SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS ;
            startBankIndex += step , index++)
        {
            if(SMEM_U32_GET_FIELD(regValue , index * 3 , 3) == groupId)
            {
                /* save floor, bank, hit num */
                tcamSegmentsInfoArr[*tcamSegmentsNumPtr].startBankIndex = startBankIndex;
                tcamSegmentsInfoArr[*tcamSegmentsNumPtr].floorNum = floorNum;
                tcamSegmentsInfoArr[*tcamSegmentsNumPtr].hitNum   = SMEM_U32_GET_FIELD(regValue , startBitHitNum + (index*2) , 2);
                (*tcamSegmentsNumPtr)++;
            }
        }
    }
}

/*******************************************************************************
*   snetLion3TcamGetKeySizeBits
*
* DESCRIPTION:
*       sip5 function that returns size bits (4 bits),
*       these bits must be added to each chunk (bits 0..3)
*
* INPUTS:
*       keySize - key size
*
* OUTPUTS:
*       None
*
* RETURNS:
*       sizeBits value
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 snetLion3TcamGetKeySizeBits
(
    IN  SIP5_TCAM_KEY_SIZE_ENT    keySize
)
{
    GT_U32 sizeBits = 0xFF;

    switch(keySize)
    {
        case SIP5_TCAM_KEY_SIZE_10B_E: sizeBits = 0; break;
        case SIP5_TCAM_KEY_SIZE_20B_E: sizeBits = 1; break;
        case SIP5_TCAM_KEY_SIZE_30B_E: sizeBits = 2; break;
        case SIP5_TCAM_KEY_SIZE_40B_E: sizeBits = 3; break;
        case SIP5_TCAM_KEY_SIZE_50B_E: sizeBits = 4; break;
        case SIP5_TCAM_KEY_SIZE_60B_E: sizeBits = 5; break;
        case SIP5_TCAM_KEY_SIZE_80B_E: sizeBits = 7; break;
        default: skernelFatalError("snetLion3TcamGetKeySizeBits: wrong key given %d\n", keySize);
    }

    return sizeBits;
}

/*******************************************************************************
*   snetLion3TcamPrepareKeyChunksArray
*
* DESCRIPTION:
*       sip5 function that prepares key chunks (84 bits each chunk)
*
* INPUTS:
*       keyArrayPtr    - array of keys bytes
*       keySize        - size of key array
*
* OUTPUTS:
*       chunksArrayPtr - array of key chunks
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3TcamPrepareKeyChunksArray
(
    IN  GT_U32                   *keyArrayPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT    keySize,
    OUT GT_U32                   *chunksArrayPtr
)
{
    GT_U32  chunkNum;
    GT_U32  keyWordsNum;
    GT_U32  sizeBits = snetLion3TcamGetKeySizeBits(keySize);
    GT_U32  valuesArr[SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS]={0};

    /* dump the key that the client give */
    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("dump key size [%d] bytes from client: \n",
        (keySize*10)));

    /* loop of the number of chunks that need to build*/
    for(chunkNum = 0; chunkNum < (GT_U32)keySize; chunkNum++)
    {
        /*each 84 bits start at new 96 bits */

        /* set the 4 bits of the 'sizeBits' as start of the 84 bits data */
        snetFieldValueSet(chunksArrayPtr,
            (chunkNum * SIP5_TCAM_NUM_OF_BITS_IN_KEY_CHUNK_CNS),
            4,
            sizeBits);

        /*continue the chunk with 10bytes (80 bits)*/

        /* first 64 bits */
        for(keyWordsNum = 0;
            keyWordsNum < SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS - 1;/*words0,1*/
            keyWordsNum++)
        {
            valuesArr[keyWordsNum] = snetFieldValueGet(keyArrayPtr,(32*keyWordsNum) + (80*chunkNum),32);
            snetFieldValueSet(chunksArrayPtr,
                4 + (32*keyWordsNum) + (chunkNum * SIP5_TCAM_NUM_OF_BITS_IN_KEY_CHUNK_CNS),
                32,
                valuesArr[keyWordsNum]);
        }

        /* next 16 bits (total of 80 bits)*/
        keyWordsNum = SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS - 1;/*word2*/
        valuesArr[keyWordsNum] = snetFieldValueGet(keyArrayPtr,(32*keyWordsNum) + (80*chunkNum),16);

        snetFieldValueSet(chunksArrayPtr,
            4 + (32*keyWordsNum) + (chunkNum * SIP5_TCAM_NUM_OF_BITS_IN_KEY_CHUNK_CNS),
            16,
            valuesArr[keyWordsNum]);

        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("0x%8.8x 0x%8.8x 0x%4.4x \n",
            valuesArr[0],valuesArr[1],valuesArr[2]));
    }
}


/*******************************************************************************
*   snetLion3TcamCompareKeyChunk
*
* DESCRIPTION:
*       sip5 function that compares key chunk to tcam memory
*
* INPUTS:
*       devObjPtr         - (pointer to) the device object
*       keyChunksArrayPtr - array of keys chunks
*       chunkIdx          - index of chunk in the key
*       xdataPtr          - pointer to x entry
*       ydataPtr          - pointer to y entry
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_BOOL           - whether chunk found (GT_TRUE) or not (GT_FALSE)
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL snetLion3TcamCompareKeyChunk
(
    IN  SKERNEL_DEVICE_OBJECT       *devObjPtr,
    IN  GT_U32                      *keyChunksArrayPtr,
    IN  GT_U32                       chunkIdx,
    IN  GT_U32                      *xdataPtr,
    IN  GT_U32                      *ydataPtr
)
{
    GT_U32  index;                  /* word index */
    GT_U32  numWords = (84 / 32);   /* Number of words to be compared */
    GT_BOOL result;                 /* Compare result status */
    GT_U32  lastWordMask = SMEM_BIT_MASK((84%32));/* mask for remaining bits in last word */
    GT_U32  *keyPtr = &keyChunksArrayPtr[chunkIdx * SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS];

    /* compare the first 2 full words */
    for (index = 0; index < numWords; index++)
    {
        result = CH3_TCAM_X_Y_K_MATCH(xdataPtr[index],ydataPtr[index],keyPtr[index],0xffffffff);
        if (result == 0)
        {
            return GT_FALSE;
        }
    }

    /* compare the next word , only 20 bits */
    result = CH3_TCAM_X_Y_K_MATCH(xdataPtr[index],ydataPtr[index],keyPtr[index],lastWordMask);
    if (result == 0)
    {
        return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetLion3TcamSegmentsIsHit
*
* DESCRIPTION:
*       sip5 function that does lookup on single segment (returns hit)
*
* INPUTS:
*       devObjPtr         - (pointer to) the device object
*       floorNum          - floor number
*       validBanksBmp     - valid banks for lookup
*       validStartBanksBmp - banks that lookup can start from
*       keyChunksArrayPtr - array with key chunks
*       keySize           - size of the key
* OUTPUTS:
*       actionInxPtr      - (pointer to) action match index
*
* RETURNS:
*       GT_BOOL           - whether hit found (GT_TRUE) or not (GT_FALSE)
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL snetLion3TcamCheckIsHit
(
    IN  SKERNEL_DEVICE_OBJECT       *devObjPtr,
    IN  GT_U32                       floorNum,
    IN  GT_U32                       validBanksBmp,
    IN  GT_U32                       validStartBanksBmp,
    IN  GT_U32                      *keyChunksArrayPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT       keySize,
    OUT GT_U32                      *actionIndexPtr
)
{
    DECLARE_FUNC_NAME(snetLion3TcamCheckIsHit);

    GT_BOOL matchFound = GT_FALSE;
    GT_U32  entryIdx;/*current entry index in the segment*/
    GT_U32  bankIdx; /*current bank index in the segment*/
    GT_U32  numOfBankHits;/*number of hits on consecutive banks */
    GT_U32  numOfBanksToHit;/*number of consecutive banks that must hit to achieve 'HIT' */
    GT_U32  bankIndexOfCurrSequenceBanks;/*bank index of current sequence banks that 'HIT' */
    GT_U32  actionIndex;/*index of the action*/
    GT_U32  globalXLineNumber = 0;/* global X line number */
    GT_U32  *xDataPtr, *yDataPtr;/*pointer to the memory of X and Y in current bank,line*/
    GT_U32  *base_xDataPtr, *base_yDataPtr;/*pointer to the start of the TCAM memory of X and Y*/
    GT_U32  numWordsOffsetBetweenXandYInTcam;/*num words Offset Between X and Y lines*/
    GT_U32  logIsOpen = simLogIsOpen();
    GT_U32  cpssCurrentIndex = 0;/* the CPSS index of the current entry  . this for 'helpful' info to the LOG */
    GT_U32  cpssActionIndex = 0; /* the CPSS index of the current action . this for 'helpful' info to the LOG */
    GT_U32  keyStartIndex = 0;
    GT_U32  ii;
    GT_U32  globalBitIndexForLog;
    GT_U32  x,y,k;

    numOfBanksToHit = keySize;

    __LOG(("Start TCAM lookup on single segment : floorNum[%d] ,validBanksBmp[0x%x],validStartBanksBmp[0x%x],numOfBanksToHit[%d]  \n",
        floorNum , validBanksBmp,validStartBanksBmp , numOfBanksToHit));

    /*Entry Number = {floor_num[3:0], array_addr[7:0], bank[3:0], XY}*/
    SMEM_U32_SET_FIELD(globalXLineNumber,0, 1,0);/*XY*/
    SMEM_U32_SET_FIELD(globalXLineNumber,13,4,floorNum);/*floor_num[3:0]*/


    /*
        for 2 purposes access only once the memory to get the pointer
        1. to not record into LOG every access to the TCAM ... to reduce size of LOG
            ... to reduce time for LOG creation ... to reduce 'not helpful' info in the LOG
        2. to speed up performance.
    */

    /*  Get Tcam X data entry */
    base_xDataPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_MEMORY_TBL_MEM(devObjPtr, 0));

    /*  Get Tcam Y data entry */
    base_yDataPtr = smemMemGet(devObjPtr, SMEM_LION3_TCAM_MEMORY_TBL_MEM(devObjPtr, 1));

    numWordsOffsetBetweenXandYInTcam = (base_yDataPtr - base_xDataPtr);

    for(entryIdx = 0 ; entryIdx < SIP5_TCAM_NUM_OF_X_LINES_IN_BANK_CNS ; entryIdx++)
    {
        /* for this line init that not started hits on the banks */
        numOfBankHits = 0;

        SMEM_U32_SET_FIELD(globalXLineNumber,5, 8,entryIdx);/*array_addr[7:0]*/

        bankIndexOfCurrSequenceBanks = 0;

        for(bankIdx = 0; bankIdx < SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS; bankIdx++)
        {
            /* check if we are with 'no match' yet */
            if(numOfBankHits == 0)
            {
                /*jump to the next bank that may start sequence*/
                for(/*bankIdx*/;bankIdx < SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS; bankIdx++)
                {
                    if(validStartBanksBmp & (1 << bankIdx))
                    {
                        /* found valid bank to start new sequence */
                        break;
                    }
                }

                if(bankIdx == SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS)
                {
                    /* not found new bank to start lookup in current line */
                    break;
                }

                /* save the bank index for the next sequence */
                bankIndexOfCurrSequenceBanks = bankIdx;
            }

            /* check that the bank valid for the lookup */
            if(0 == (validBanksBmp & (1 << bankIdx)))
            {
                if(numOfBankHits)
                {
                    __LOG(("Warning : The non valid bankIdx[%d] broke the lookup sequence \n",bankIdx));
                }

                goto bankNoMatch_lbl;
            }

            SMEM_U32_SET_FIELD(globalXLineNumber,1, 4,bankIdx);/*bank[3:0]*/

            xDataPtr = base_xDataPtr + (numWordsOffsetBetweenXandYInTcam * globalXLineNumber);
            yDataPtr = xDataPtr + numWordsOffsetBetweenXandYInTcam;

            if(logIsOpen)
            {
                /*  For the Application the relation between floor_Id, entry_Id , bank_index and global_Index are:
                    cpssCurrentIndex = (floor_Id * (12*256)) + (entry_Id * 12) + bank_index
                    the actual index that the CPSS will write is:
                    hw_global_Index = (floor_Id * (16*256)) + (entry_Id * 16) + bank_index
                */
                cpssCurrentIndex = (floorNum*256*12) + (entryIdx * 12) + bankIdx;
            }


            if(GT_TRUE ==
               snetLion3TcamCompareKeyChunk(devObjPtr, keyChunksArrayPtr, numOfBankHits, xDataPtr, yDataPtr) )
            {
                /* match on current bank */
                numOfBankHits++;

                if(numOfBankHits == numOfBanksToHit)
                { /* found a match after all the needed consecutive banks matched */

                    /*action index is according to the bank <bankIndexOfCurrSequenceBanks> value */
                    actionIndex = globalXLineNumber;
                    SMEM_U32_SET_FIELD(actionIndex,1, 4, bankIndexOfCurrSequenceBanks);/*bank[3:0]*/
                    actionIndex >>= 1;/*action index is 1/2 from the corresponding X entry*/

                    *actionIndexPtr = actionIndex;

                    __LOG(("TCAM match found (found HIT): floor[%d] entryIdx[%d] bank[%d] -  actionIndex [0x%x]([%d]) \n",
                        floorNum , entryIdx , bankIndexOfCurrSequenceBanks ,
                        actionIndex, actionIndex));

                    cpssActionIndex = (floorNum*256*12) + (entryIdx * 12) + bankIndexOfCurrSequenceBanks;

                    __LOG(("NOTE: in terms of CPSS the cpssActionIndex [0x%x] ([%d]) \n",
                        cpssActionIndex,cpssActionIndex));


                    __LOG(("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n"));
                    matchFound = GT_TRUE;
                    goto exitCleanly_lbl;
                }
                else
                if (logIsOpen)
                {
                    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("[%d] matched banks (out of [%d]) \n",
                        numOfBankHits,
                        numOfBanksToHit));
                }

                /* we have match on this bank but need more banks to hit ... */
                continue;
            }
            else
            if (logIsOpen)
            {
                if(numOfBankHits == 0)
                {
                    if(((xDataPtr[0] & yDataPtr[0]) & 0xF))
                    {
                        /* the first 4 bits are the size , if one of those 4 bits is 1 in X and in Y --> 'NEVER match' */
                        /* we not want to give any log indication for those not valid entries */
                        goto bankNoMatch_lbl;
                    }
                }

                scibAccessLock();

                __LOG(("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n"));

                __LOG(("Floor[%d] entryIdx[%d] bank[%d] \n",
                    floorNum , entryIdx , bankIndexOfCurrSequenceBanks));

                __LOG(("NOTE: in terms of CPSS the 'no match' cpssCurrentIndex [0x%x]([%d]) \n",
                    cpssCurrentIndex,cpssCurrentIndex));

                /* need to explain why the entry not match */
                if(numOfBankHits)
                {
                    __LOG_NO_LOCATION_META_DATA(("the no match happen after [%d] matched banks (out of [%d]) \n",
                        numOfBankHits,
                        numOfBanksToHit));
                }

                __LOG_PARAM_NO_LOCATION_META_DATA(((globalXLineNumber/2)));

                keyStartIndex = SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS * numOfBankHits;

                /* check the key size */
                x = snetFieldValueGet(xDataPtr,0,4);
                y = snetFieldValueGet(yDataPtr,0,4);
                k = snetFieldValueGet(keyChunksArrayPtr,(keyStartIndex*32)+0,4);

                if(X_Y_K_FIND_NO_MATCH(x,y,k,0xF))
                {
                    /* the 'size' do not match */
                    __LOG(("the 4 bits of the 'key size' do not match : X[0x%x],Y[0x%x],K[0x%x] \n",
                        x,y,k));
                }
                else
                {
                    __LOG_NO_LOCATION_META_DATA(("X,Y,K (84 bits include the 'key size'(0..3))- explain reason for no match in this entry: \n"));
                    __LOG_NO_LOCATION_META_DATA(("format:[31..0]  [63..32]   [83..64] \n"));
                    __LOG_NO_LOCATION_META_DATA(("X [%8.8x][%8.8x][%5.5x]\n",
                        xDataPtr[0],xDataPtr[1],xDataPtr[2]&0x000FFFFF));
                    __LOG_NO_LOCATION_META_DATA(("Y [%8.8x][%8.8x][%5.5x]\n",
                        yDataPtr[0],yDataPtr[1],yDataPtr[2]&0x000FFFFF));
                    __LOG_NO_LOCATION_META_DATA(("K [%8.8x][%8.8x][%5.5x]\n",
                        keyChunksArrayPtr[keyStartIndex+0],keyChunksArrayPtr[keyStartIndex+1],keyChunksArrayPtr[keyStartIndex+2]&0x000FFFFF));
                    /* print which bits caused the NO match */
                    __LOG_NO_LOCATION_META_DATA(("bits that caused no match [%8.8x][%8.8x][%5.5x] \n",
                        X_Y_K_FIND_NO_MATCH(xDataPtr[0],yDataPtr[0],keyChunksArrayPtr[keyStartIndex+0],0xFFFFFFFF),/*32 bits*/
                        X_Y_K_FIND_NO_MATCH(xDataPtr[1],yDataPtr[1],keyChunksArrayPtr[keyStartIndex+1],0xFFFFFFFF),/*32 bits*/
                        X_Y_K_FIND_NO_MATCH(xDataPtr[2],yDataPtr[2],keyChunksArrayPtr[keyStartIndex+2],0x000FFFFF) /*20 bits --> total of 32+32+20 = 84 */
                    ));

                    /* analyze the bits that are 'NO match' */
                    __LOG_NO_LOCATION_META_DATA(("analyze the GLOBAL bits that are 'NO match' \n"));
                    __LOG_NO_LOCATION_META_DATA(("the global index is in terms of the FS that describes the TTI/PCL key \n"));

                    /* the global index is in terms of the FS that describes the TTI/PCL key */
                    globalBitIndexForLog = (80*numOfBankHits);

                    __LOG_NO_LOCATION_META_DATA(("Bits:"));
                    /*start from bit 4 because the no match is not from there ! */
                    /*and the (globalBitIndexForLog++) must not be done for bits 0..3 ! */
                    for(ii = 4 ; ii < 84 ; ii++ , globalBitIndexForLog++)
                    {
                        x = snetFieldValueGet(xDataPtr,ii,1);
                        y = snetFieldValueGet(yDataPtr,ii,1);
                        k = snetFieldValueGet(keyChunksArrayPtr,(keyStartIndex*32)+ii,1);

                        if(X_Y_K_FIND_NO_MATCH(x,y,k,1))
                        {
                            __LOG_NO_LOCATION_META_DATA(("%d,",
                                globalBitIndexForLog));
                        }
                    }
                    __LOG_NO_LOCATION_META_DATA((". \n End of not matched Bits \n"));
                }

                __LOG(("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n"));
                scibAccessUnlock();

            }

bankNoMatch_lbl:
            /*reset the number of hits*/
            numOfBankHits = 0;
        }
    }

exitCleanly_lbl:

    __LOG(("Ended TCAM lookup on this single segment %s match \n",
        (matchFound == GT_TRUE ? "with" : "WITHOUT")));

    return matchFound;
}

/*******************************************************************************
*   snetLion3TcamSegmentsListIsHit
*
* DESCRIPTION:
*       sip5 function that does lookup on segments in the list (returns hit)
*
* INPUTS:
*       devObjPtr         - (pointer to) the device object
*       hitNum            - current hit number to check
*       tcamSegmentsInfoArr - segments info
*       tcamSegmentsNum     - size of banks info array
*       keyChunksArrayPtr - array with key chunks
*       keySize           - size of the key
*
* OUTPUTS:
*       actionInxPtr      - (pointer to) action match index.
*
* RETURNS:
*       GT_BOOL           - whether hit found (GT_TRUE) or not (GT_FALSE)
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL snetLion3TcamSegmentsListIsHit
(
    IN  SKERNEL_DEVICE_OBJECT       *devObjPtr,
    IN  GT_U32                       hitNum,
    IN  SIP5_TCAM_SEGMENT_INFO_STC  *tcamSegmentsInfoArr,
    IN  GT_U32                       tcamSegmentsNum,
    IN  GT_U32                      *keyChunksArrayPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT       keySize,
    IN  GT_U32                      validBanksStartIndexBmp,
    OUT GT_U32                      *actionIndexPtr
)
{
    DECLARE_FUNC_NAME(snetLion3TcamSegmentsListIsHit);

    TCAM_FLOORS_INFO_STC    floorsInfo;
    GT_U32  floorNum;
    GT_U32  ii;
    GT_U32  isFirstValidFloor = 1;
    GT_U32  numBanksInHitSegment;

    *actionIndexPtr = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;

    numBanksInHitSegment = SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS / NUM_HIT_SEGMENTS_IN_FLOOR_MAC(devObjPtr);

    memset(&floorsInfo,0,sizeof(floorsInfo));
    /* build valid banks per floor */
    for(ii = 0 ; ii < tcamSegmentsNum ; ii++)
    {
        if(hitNum != tcamSegmentsInfoArr[ii].hitNum)
        {
            continue;
        }

        floorNum = tcamSegmentsInfoArr[ii].floorNum;

        floorsInfo.floorInfo[floorNum].validBanksBmp |=
            SMEM_BIT_MASK(numBanksInHitSegment)
                << tcamSegmentsInfoArr[ii].startBankIndex;
    }

    for(floorNum = 0 ; floorNum < SIP5_TCAM_MAX_NUM_OF_FLOORS_CNS ; floorNum ++)
    {
        if(floorsInfo.floorInfo[floorNum].validBanksBmp == 0)
        {
            /* the floor not valid or not relevant for current hitNum */
            continue;
        }

        if(isFirstValidFloor)
        {
            isFirstValidFloor = 0;

            /* print only if any of the floors bound to this hitNum */
            __LOG_PARAM(hitNum);
        }

        __LOG_PARAM(floorNum);
        __LOG_PARAM(floorsInfo.floorInfo[floorNum].validBanksBmp);

        if(GT_TRUE ==
            snetLion3TcamCheckIsHit(devObjPtr,
                    floorNum,
                    floorsInfo.floorInfo[floorNum].validBanksBmp,
                    validBanksStartIndexBmp,
                    keyChunksArrayPtr,
                    keySize,
                    actionIndexPtr))
        {
            /* found a match */
            return GT_TRUE;
        }
    }

    return GT_FALSE;
}

/*******************************************************************************
*   tcamvalidBanksStartIndexBmpGet
*
* DESCRIPTION:
*       function that get valid bank indexes for start of lookup.
*
* INPUTS:
*       devObjPtr         - (pointer to) the device object
*       keySize           - size of the key
*
* OUTPUTS:
*       validBanksStartIndexBmpPtr - (pointer to )valid bank indexes bitmap for start of lookup.
*
* RETURNS:
*       none.
*
* COMMENTS:
*
*******************************************************************************/
static void tcamvalidBanksStartIndexBmpGet
(
    IN  SKERNEL_DEVICE_OBJECT       *devObjPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT       keySize,

    OUT  GT_U32                      *validBanksStartIndexBmpPtr
)
{
    DECLARE_FUNC_NAME(tcamvalidBanksStartIndexBmpGet);

    GT_U32  validBanksStartIndexBmp;
    GT_U32  regValue;

    /*set validity checks and valid banks that must be power up*/
    switch(keySize)
    {
        case SIP5_TCAM_KEY_SIZE_80B_E:
            validBanksStartIndexBmp = 1<<0;
            break;
        case SIP5_TCAM_KEY_SIZE_60B_E:
        case SIP5_TCAM_KEY_SIZE_50B_E:
        case SIP5_TCAM_KEY_SIZE_40B_E:
            validBanksStartIndexBmp = 1<<0 | 1<<6;
            break;
        case SIP5_TCAM_KEY_SIZE_30B_E:
            validBanksStartIndexBmp = 1<<0 | 1<<3 | 1<<6 | 1<<9;
            break;
        case SIP5_TCAM_KEY_SIZE_20B_E:
            validBanksStartIndexBmp = 1<<0 | 1<<2 | 1<<4 | 1<<6 | 1<<8 | 1<<10;
            break;
        default:
        case SIP5_TCAM_KEY_SIZE_10B_E:
            validBanksStartIndexBmp = 0xFFF;
            break;
    }

    if (SMEM_CHT_IS_SIP5_15_GET(devObjPtr) &&
        (keySize == SIP5_TCAM_KEY_SIZE_40B_E ||
         keySize == SIP5_TCAM_KEY_SIZE_30B_E))
    {
        smemRegGet(devObjPtr, SMEM_LION3_TCAM_GLOBAL_REG(devObjPtr), &regValue);

        if (keySize == SIP5_TCAM_KEY_SIZE_40B_E)
        {
            /*
                Controls the key expend during 40-Byte Key.

                 0x0 = Mode0; Mode0; 40B key starts at Bank0, Bank6.; BC2 Compatible Mode
                 0x1 = Mode1; Mode1; 40B key starts at Bank0, Bank4, Bank8.; Better Utilization Mode
            */
            if(0 == SMEM_U32_GET_FIELD(regValue , 5 ,1))
            {
                __LOG(("40-Byte Key : mode : 40B key starts at Bank0, Bank6.; BC2 Compatible Mode"));
            }
            else
            {
                __LOG(("40-Byte Key : mode : 40B key starts at Bank0, Bank4, Bank8.; Better Utilization Mode"));

                validBanksStartIndexBmp = 1<<0 | 1<<4 | 1<<8;
            }
        }
        else
        {
            /*
                Controls the key expend during 30-Byte Key.

                 0x0 = Mode0; Mode0; 30B key starts at Bank0, Bank3, Bank6, Bank9.
                 0x1 = Mode1; Mode1; 30B key starts at Bank0, Bank4, Bank8.
            */
            if(0 == SMEM_U32_GET_FIELD(regValue , 4 ,1))
            {
                __LOG(("30-Byte Key : mode : 30B key starts at Bank0, Bank3, Bank6, Bank9"));
            }
            else
            {
                __LOG(("30-Byte Key : mode : 30B key starts at Bank0, Bank4, Bank8"));

                validBanksStartIndexBmp = 1<<0 | 1<<4 | 1<<8;
            }
        }
    }

    *validBanksStartIndexBmpPtr = validBanksStartIndexBmp;

    return;
}

/*******************************************************************************
*   snetLion3TcamDoLookup
*
* DESCRIPTION:
*       sip5 function that does sequential lookup on all banks
*
* INPUTS:
*       devObjPtr         - (pointer to) the device object
*       tcamSegmentsInfoArr - segments info
*       tcamSegmentsNum     - size of banks info array
*       keyChunksArrayPtr - array with key chunks
*       keySize           - size of the key
*
* OUTPUTS:
*       resultPtr         - array of action matched indexes
*
* RETURNS:
*       number of results
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 snetLion3TcamDoLookup
(
    IN  SKERNEL_DEVICE_OBJECT       *devObjPtr,
    IN  SIP5_TCAM_SEGMENT_INFO_STC  *tcamSegmentsInfoArr,
    IN  GT_U32                       tcamSegmentsNum,
    IN  GT_U32                      *keyChunksArrayPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT       keySize,
    OUT GT_U32                       resultArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS]
)
{
    DECLARE_FUNC_NAME(snetLion3TcamDoLookup);

    GT_BOOL gotHit = GT_FALSE;
    GT_U32  hitNum;
    GT_U32  matchIdx;
    GT_U32  hitsCounter = 0;
    GT_U32  validBanksStartIndexBmp;

    /* get valid bmp of banks that can start lookup for the 'keySize' */
    tcamvalidBanksStartIndexBmpGet(devObjPtr,keySize,
            &validBanksStartIndexBmp);

    __LOG_PARAM(validBanksStartIndexBmp);

    for(hitNum = 0; hitNum < SIP5_TCAM_MAX_NUM_OF_HITS_CNS; hitNum++)
    {
        __LOG(("start lookup for hitNum [%d] \n", hitNum));

        gotHit = snetLion3TcamSegmentsListIsHit(devObjPtr, hitNum, tcamSegmentsInfoArr,
                            tcamSegmentsNum, keyChunksArrayPtr, keySize, validBanksStartIndexBmp ,
                            &matchIdx);
        if(gotHit)
        {
            resultArr[hitNum] = matchIdx;
            hitsCounter++;
            __LOG(("for hitNum [%d]: got hit: matchIdx [%d] \n", hitNum,matchIdx));
        }
        else
        {
            /*init for the caller that no match for this lookup*/
            resultArr[hitNum] = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS;
            __LOG(("for hitNum [%d]: NO hit \n", hitNum));
        }
    }

    return hitsCounter;
}

/*******************************************************************************
*   sip5TcamConvertPclKeyFormatToKeySize
*
* DESCRIPTION:
*       sip5 function that do convertation of old tcam key format to sip5 key size
*
* INPUTS:
*       keyFormat     - format of the key
*
* OUTPUTS:
*       sip5KeySizePtr   - sip5 key size
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID sip5TcamConvertPclKeyFormatToKeySize
(
    IN  CHT_PCL_KEY_FORMAT_ENT    keyFormat,
    OUT SIP5_TCAM_KEY_SIZE_ENT   *sip5KeySizePtr
)
{
    switch(keyFormat)
    {
        case CHT_PCL_KEY_10B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_10B_E;
            break;

        case CHT_PCL_KEY_20B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_20B_E;
            break;

        case CHT_PCL_KEY_REGULAR_E:
        case CHT_PCL_KEY_30B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_30B_E;
            break;

        case CHT_PCL_KEY_40B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_40B_E;
            break;

        case CHT_PCL_KEY_50B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_50B_E;
            break;

        case CHT_PCL_KEY_EXTENDED_E:
        case CHT_PCL_KEY_60B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_60B_E;
            break;

        case CHT_PCL_KEY_TRIPLE_E:
        case CHT_PCL_KEY_80B_E:
            *sip5KeySizePtr = SIP5_TCAM_KEY_SIZE_80B_E;
            break;

        case CHT_PCL_KEY_UNLIMITED_SIZE_E:
        default:
            skernelFatalError("Wrong key format given\n");
    }
}

/*******************************************************************************
*   sip5TcamLookup
*
* DESCRIPTION:
*       sip5 function that do lookup in tcam for given key and fill the
*       results array
*
* INPUTS:
*       devObjPtr     - (pointer to) the device object
*       tcamClient    - tcam client
*       keyArrayPtr   - key array (size up to 80 bytes)
*       keySize       - size of the key
*
* OUTPUTS:
*       resultPtr     - array of action table results, up to
*                       SIP5_TCAM_MAX_NUM_OF_HITS_CNS results are supported
*
* RETURNS:
*       number of hits found
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 sip5TcamLookup
(
    IN  SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN  SIP5_TCAM_CLIENT_ENT      tcamClient,
    IN  GT_U32                   *keyArrayPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT    keySize,
    OUT GT_U32                    resultArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS]
)
{
    DECLARE_FUNC_NAME(sip5TcamLookup);

    GT_STATUS   st; /* return status */
    GT_U32      groupId = 0;
    GT_U32      tcamSegmentsNum;/* number of tcam segments (half floors) */
    GT_U32      numOfHits;
    GT_U32      keyChunksArr[8/*chunks*/ * SIP5_TCAM_NUM_OF_WORDS_IN_KEY_CHUNK_CNS];
    SIP5_TCAM_SEGMENT_INFO_STC tcamSegmentsInfoArr[SIP5_TCAM_SEGMENTS_INFO_SIZE_CNS];
    GT_CHAR*    clientNamePtr = ((tcamClient < SIP5_TCAM_CLIENT_LAST_E) ? tcamClientName[tcamClient] : "unknown");
    GT_U32      ii;

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_TCAM_E);

    __LOG(("Start Tcam check for [%s] \n",
        clientNamePtr));

    /* get group id */
    st = snetLion3TcamGetTcamGroupId(devObjPtr, tcamClient, &groupId);
    if(GT_OK != st)
    {
        __LOG(("client [%s] disabled",
            clientNamePtr));
        numOfHits = 0;
        goto exitCleanly_lbl;
    }

    /* get segments array info */
    snetLion3TcamSegmentsListGet(devObjPtr, groupId, tcamSegmentsInfoArr, &tcamSegmentsNum);

    if( 0 == tcamSegmentsNum )
    {
        __LOG(("client [%s]: there are no relevant banks found \n",
            clientNamePtr));
        numOfHits = 0;
        goto exitCleanly_lbl;
    }
    else
    {
        __LOG(("client [%s]: will look in next 'start bank indexes' : \n"
            "index  | floorNum    |    startBankIndex |  hitNum \n"
            "-------------------------------------------------- \n",
            clientNamePtr));

        for(ii = 0 ; ii < tcamSegmentsNum ; ii++ )
        {
            __LOG(("%2.2d   |  %2.2d      |       %2.2d      |     %d \n",
                         ii     ,
                         tcamSegmentsInfoArr[ii].floorNum,
                         tcamSegmentsInfoArr[ii].startBankIndex,
                         tcamSegmentsInfoArr[ii].hitNum));
        }
    }

    /* build the array of keys + 'key size' per block */
    snetLion3TcamPrepareKeyChunksArray(keyArrayPtr, keySize, keyChunksArr);

    /* sequential lookup on all segments */
    numOfHits = snetLion3TcamDoLookup(devObjPtr, tcamSegmentsInfoArr, tcamSegmentsNum,
                                      keyChunksArr, keySize, resultArr);

   __LOG_PARAM(resultArr[0]);
   __LOG_PARAM(resultArr[1]);
   __LOG_PARAM(resultArr[2]);
   __LOG_PARAM(resultArr[3]);

exitCleanly_lbl:
    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___RESTORE_PREVIOUS_UNIT___E);

    return numOfHits;
}

/*******************************************************************************
*   snetSip5PclTcamLookup
*
* DESCRIPTION:
*       sip5 function that do pcl lookup in tcam
*
* INPUTS:
*       devObjPtr     - (pointer to) the device object
*       iPclTcamClient    - tcam client
*       u32keyArrayPtr- key array (GT_U32)
*       keyFormat     - format of the key
*
* OUTPUTS:
*       matchIdxPtr   - (pointer to) match indexes array
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetSip5PclTcamLookup
(
    IN  SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN  SIP5_TCAM_CLIENT_ENT      iPclTcamClient,
    IN  GT_U32                   *u32keyArrayPtr,
    IN  CHT_PCL_KEY_FORMAT_ENT    keyFormat,
    OUT GT_U32                   *matchIndexPtr
)
{
    SIP5_TCAM_KEY_SIZE_ENT   sip5KeySize;
    GT_U32                   resultArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS];
    GT_U32                   ii;
    GT_U32                   numOfHits;

    /* convert old key format to new key size */
    sip5TcamConvertPclKeyFormatToKeySize(keyFormat, &sip5KeySize);

    /* search the key */
    numOfHits = sip5TcamLookup(devObjPtr, iPclTcamClient,
                   u32keyArrayPtr, sip5KeySize, resultArr);

    if (numOfHits)
    {
        for (ii = 0; ii < SIP5_TCAM_MAX_NUM_OF_HITS_CNS; ii++) 
        {
           matchIndexPtr[ii] = resultArr[ii];
        }
    }
}

