/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbSoho.c
*
* DESCRIPTION:
*       This is a SFDB module of SKernel Soho.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 32 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/sfdb/sfdbSoho.h>
#include <asicSimulation/SKernel/suserframes/snetSoho.h>
#include <asicSimulation/SKernel/sohoCommon/sregSoho.h>
#include <asicSimulation/SKernel/smem/smemSoho.h>

#define SOHO_ALL_DB_NUM_CNS   0xffffffff

/*get the most significant byte from the atu entry */
#define SOHO_MSB_IN_ATU_ENTRY_MAC(atuEntryPtr) \
    (GT_U8)((((GT_U32*)(atuEntryPtr))[1]>>8)  & 0xff)

/* get DB Number from ATU message */
#define SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray)               \
        ( SKERNEL_DEVICE_FAMILY_SOHO_PLUS((devObjPtr)->deviceType) ?       \
          SMEM_U32_GET_FIELD((msgWordsArray)[6], 0, 12) :                  \
            ((SMEM_U32_GET_FIELD((msgWordsArray)[0], 0, 4)) |               \
                ((SKERNEL_DEVICE_FAMILY_SOHO2((devObjPtr)->deviceType) ?     \
                (SMEM_U32_GET_FIELD((msgWordsArray)[5], 12, 4) << 4) : 0))))

#define SOHO_NUM_WORDS_IN_ATU_ACTION_CNS    7

#define SOHO_MAX_ENTRY_STATE_IN_ATU_ENTRY_CNS 0x6


/* ATU Opcodes */
typedef enum {
    ATU_NO_OPERATION_E = 0,
    ATU_FLUSH_MOVE_ALL_E = 1,
    ATU_FLUSH_MOVE_NON_STATIC_E = 2,
    ATU_LOAD_PURGE_ENTRY_E = 3,
    ATU_GET_NEXT_E = 4,
    ATU_FLUSH_MOVE_DB_ALL_E = 5,
    ATU_FLUSH_MOVE_DB_NON_STATIC_E = 6,
    ATU_GET_CLEAR_VIOLATION_E = 7
} SOHO_ATU_OP_E;

static GT_STATUS sfdbSohoAtuEntryFind(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum,
    OUT GT_U32 * entryIdx
);

static void sfdbSohoMoveAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 * msgWordPtr
);

static void sfdbSohoFlushAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   dbNum,
    IN GT_BOOL  onlyDynamic
);

static void sfdbSohoAddAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 * msgWordPtr,
    IN GT_U32 dbNum
);

static void sfdbSohoDeleteAtuEntry (
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum
);

static  GT_STATUS sfdbSohoGetNextAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 * msgWordPtr,
    IN GT_U32 dbNum
);

static void sfdbSohoaMacTableDoAging(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32  agingTime
);


static GT_U32  counter_valid_entry;
static GT_U32  counter_valid_non_static_entry;
static GT_U32  counter_valid_entry_fid;
static GT_U32  counter_valid_non_static_entry_fid;

/*******************************************************************************
*   sfdbSohoMsgProcess
*
* DESCRIPTION:
*       Process FDB update message.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*   *************************************************************************************
*     WORD2   |                     WORD 1            |            WORD 0               *
*             |10     20 19    1615     0           0 |10         0          0          0
*         ftpp| p|   DPV  | ES   |     MAC 16 MSB     |       MAC 32 LSB                |
*   *************************************************************************************
*
*******************************************************************************/
GT_VOID sfdbSohoMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * fdbMsgPtr
)
{
    GT_U16   msgWordsArray[SOHO_NUM_WORDS_IN_ATU_ACTION_CNS];/* pointer to the first word */
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    SOHO_ATU_OP_E atuOp;                    /* ATU opcode */
    GT_U32 intrAtuDone = 0;                 /* interrupts' flag */
    GT_U32 fldValue;                        /* register' field value */
    GT_U32 dbNumber;                        /* ATU MAC address dbase number */
    SGT_MAC_ADDR_UNT macAddr;               /* MAC address from message */
    GT_U8 * macPtr;                         /* pointer to MAC[0:5] */
    GT_U32        ii;
    GT_U16 * wordDataPtr;                   /* violation data pointers */
    GT_U32 * regDataPtr;                    /* register's data pointer */

    for(ii=0;ii<SOHO_NUM_WORDS_IN_ATU_ACTION_CNS;ii++)
    {
        msgWordsArray[ii] = *(GT_U16 *)(fdbMsgPtr+4*ii);
    }

    /*msgWords = (GT_U16 *)fdbMsgPtr;*/
    macPtr = (GT_U8 *)&msgWordsArray[2];

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    atuOp = SMEM_U32_GET_FIELD(msgWordsArray[0], 12, 3);
    if (atuOp == ATU_FLUSH_MOVE_ALL_E)
    {
        /* Entry state */
        fldValue = SMEM_U32_GET_FIELD(msgWordsArray[1], 0, 4);
        if (fldValue == 0xf)
        {
            /* move entries on all dbNums */
            sfdbSohoMoveAtuEntry(devObjPtr, msgWordsArray);
        }
        else
        {
            /* flush all entries on all dbNums */
            sfdbSohoFlushAtuEntry(devObjPtr, SOHO_ALL_DB_NUM_CNS,
                                  GT_FALSE);/*all entries:static,dynamic*/
        }

        intrAtuDone = 1;
    }
    else
    if (atuOp == ATU_FLUSH_MOVE_NON_STATIC_E)
    {
        /* Entry state */
        fldValue = SMEM_U32_GET_FIELD(msgWordsArray[1], 0, 4);
        if (fldValue == 0xf)
        {
            sfdbSohoMoveAtuEntry(devObjPtr, msgWordsArray);
        }
        else
        {
            /* flush only dynamic entries on all dbNums */
            sfdbSohoFlushAtuEntry(devObjPtr, SOHO_ALL_DB_NUM_CNS,
                                  GT_TRUE);/* only dynamic entries */
        }
        intrAtuDone = 1;
    }
    else
    if (atuOp == ATU_LOAD_PURGE_ENTRY_E)
    {
        /* ATU MAC address database number */
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS((devObjPtr)->deviceType))
        { /*db num place is 10 bytes before the place in old SOHOS */
            msgWordsArray[6] = msgWordsArray[6] - 10;
            dbNumber =  SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);
            msgWordsArray[6] = msgWordsArray[6] + 10;
        }
        else
            dbNumber =  SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);

        /* Entry state */
        fldValue = SMEM_U32_GET_FIELD(msgWordsArray[1], 0, 4);
        if (fldValue == 0)
        {
            /* Delete ATU entry from SRAM */
            SOHO_SWAP_MSG_2_MAC_FROM_CPU(macPtr, macAddr);
            sfdbSohoDeleteAtuEntry(devObjPtr, macAddr.bytes, dbNumber);
        }
        else
        {
            /* Add new ATU entry to SRAM */
            sfdbSohoAddAtuEntry(devObjPtr, msgWordsArray, dbNumber);
        }
        intrAtuDone = 1;
    }
    else
    if (atuOp == ATU_GET_NEXT_E)
    {
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS((devObjPtr)->deviceType))
        { /*db num place is 10 bytes before the place in old SOHOS */
            msgWordsArray[6] = msgWordsArray[6] - 10;
            dbNumber =  SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);
            msgWordsArray[6] = msgWordsArray[6] + 10;
        }
        else
        /* ATU MAC address database number */
            dbNumber = SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);

        /* Get next higher MAC address in particular database */
        sfdbSohoGetNextAtuEntry(devObjPtr, msgWordsArray, dbNumber);
        intrAtuDone = 1;
    }
    else if(atuOp == ATU_FLUSH_MOVE_DB_ALL_E)
    {
        /* Entry state */
        fldValue = SMEM_U32_GET_FIELD(msgWordsArray[1], 0, 4);
        if (fldValue == 0xf)
        {
            sfdbSohoMoveAtuEntry(devObjPtr, msgWordsArray);
        }
        else
        {
            /* ATU MAC address database number */
            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS((devObjPtr)->deviceType))
            { /*db num place is 10 bytes before the place in old SOHOS */
                msgWordsArray[6] = msgWordsArray[6] - 10;
                dbNumber =  SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);
                msgWordsArray[6] = msgWordsArray[6] + 10;
            }
            else
                dbNumber = SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);
                /* flush all entries on specific dbNum */
            sfdbSohoFlushAtuEntry(devObjPtr, dbNumber,
                                  GT_FALSE);/*all entries:static,dynamic*/
        }
        intrAtuDone = 1;
    }
    else if(atuOp==ATU_FLUSH_MOVE_DB_NON_STATIC_E)
    {
        /* Entry state */
        fldValue = SMEM_U32_GET_FIELD(msgWordsArray[1], 0, 4);
        if (fldValue == 0xf)
        {
            sfdbSohoMoveAtuEntry(devObjPtr, msgWordsArray);
        }
        else
        {
            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS((devObjPtr)->deviceType))
            { /*db num place is 10 bytes before the place in old SOHOS */
                msgWordsArray[6] = msgWordsArray[6] - 10;
                dbNumber =  SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);
                msgWordsArray[6] = msgWordsArray[6] + 10;
            }
            else
                /* ATU MAC address database number */
                dbNumber = SOHO_DB_NUM_FROM_ATU_OP_MAC(devObjPtr,msgWordsArray);
            /* flush only dynamic entries on specific dbNum */
            sfdbSohoFlushAtuEntry(devObjPtr, dbNumber,
                                  GT_TRUE);/* only dynamic entries */
        }
        intrAtuDone = 1;
    }
    else
    if (atuOp == ATU_GET_CLEAR_VIOLATION_E)
    {
        regDataPtr = smemMemGet(devObjPtr, GLB_ATU_OPER_REG);
        wordDataPtr = memInfoPtr->macDbMem.violationData;

        /*  ATU MAC Address Database Number bits 3:0.               *
         *  On Get/Clear Violation Data ATUOps these                *
         *  bits return the DBNum[3:0] value associated with the    *
         *  ATU violation that was just serviced.                   *
         *  The upper 4-bits of the ATUs DBNum are in bits 15:12   *
         *  of the ATU Control register at offset 0x0A.             */
        regDataPtr[0] = (1<<15) | (wordDataPtr[0] & 0xF);

        /* convert U16 wordDataPtr to U32 regDataPtr */
        for (ii = 1; (ii < 5); ii++)
        {
            regDataPtr[ii] = wordDataPtr[ii] ;
        }
        /* set the next 4 bits of dbNum */
        smemRegFldSet(devObjPtr, GLB_ATU_CTRL_REG, 12,4, (wordDataPtr[0] >> 4));

        if (memInfoPtr->macDbMem.violation == SOHO_MISS_ATU_E)
        {
            /* Set ATU miss violation */
            SMEM_U32_SET_FIELD(regDataPtr[0], 5, 1, 1);
        }
        else
        if (memInfoPtr->macDbMem.violation == SOHO_SRC_ATU_PORT_E)
        {
            SMEM_U32_SET_FIELD(regDataPtr[0], 6, 1, 1);
        }
        else
        if (memInfoPtr->macDbMem.violation == SOHO_FULL_ATU_E)
        {
            SMEM_U32_SET_FIELD(regDataPtr[0], 4, 1, 1);
        }

        intrAtuDone = 1;
        /* The AtuProb bit automatically cleared when all the pending        *
         * ATU Violations have been serviced by the ATU Get/Clear Violation  *
         * Data operation                                                    */
        smemRegFldSet(devObjPtr, GLB_STATUS_REG, 3, 1, 0);
    }
    /* Clear ATU busy */
    smemRegFldSet(devObjPtr, GLB_ATU_OPER_REG, 15, 1, 0);

    if (intrAtuDone == 1)
    {
        /* check that interrupt enabled */
        smemRegFldGet(devObjPtr, GLB_CTRL_REG, 2, 1, &fldValue);
        if (fldValue)
        {
            /* ATU done interrupt */
            smemRegFldSet(devObjPtr, GLB_STATUS_REG, 2, 1, 1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
}

/*******************************************************************************
*   sfdbSohoAddAtuEntry
*
* DESCRIPTION:
*       Add ATU entry to SRAM
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       msgWordPtr  - pointer to ATU message
*       dbNum       - lookup database number
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*
*******************************************************************************/
static void sfdbSohoAddAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 * msgWordPtr,
    IN GT_U32 dbNum
)
{
    GT_U32 atuWord[SOHO_ATU_ENTRY_SIZE_IN_WORDS];/* ATU entry word pointer */
    GT_U32 address = 0;                     /* MAC table memory address */
    SGT_MAC_ADDR_UNT macAddr;               /* MAC address from message */
    GT_U8 * macPtr;                         /* Pointer to MAC address */
    GT_U32  entryIdx;                       /* ATU entry index */

    macPtr = (GT_U8 *)&msgWordPtr[2];
    SOHO_SWAP_MSG_2_MAC_FROM_CPU(macPtr, macAddr);

    /* if entry found then remove it and write the new one ,
       if not found , find the first bin to be free           */
    if (sfdbSohoAtuEntryFind(devObjPtr, macAddr.bytes,dbNum,&entryIdx)
             == GT_NOT_FOUND)
    {
        if (sfdbSohoFreeBinGet(devObjPtr,macAddr.bytes,dbNum,&address)==GT_OK)/*all static*/
            return;
    }
    else
    {
        address = (4 << 28) | (entryIdx << 2) ;
    }

    /* MAC address */
    atuWord[0] = macAddr.bytes[5] |
                 macAddr.bytes[4] << 8 |
                 macAddr.bytes[3] << 16 |
                 macAddr.bytes[2] << 24;
    atuWord[1] = macAddr.bytes[1] |
                 macAddr.bytes[0] << 8;
    /* Entry state */
    atuWord[1] |= (msgWordPtr[1] & 0xf) << 16;
    /* 10/11 ports according to device ... */
    atuWord[1] |= ((msgWordPtr[1] >> 4) & ((1 << devObjPtr->portsNumber)-1)) << 20;

    /* entry's priority */
    if ((devObjPtr->deviceType == SKERNEL_RUBY) ||
        (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType)))
    {
        /* 3 bits priority */
        atuWord[1] |= (((msgWordPtr[0] & (0x1 << 8)) >> 8) << 31); /* 1 lsb */
        atuWord[2] =  (((msgWordPtr[0] & (0x3 << 9)) >> 9) << 0); /* 2 msb */
    }
    else
    {
        /* 2 bits priority */
        atuWord[1] |= (((msgWordPtr[1] & (0x3 << 13)) >> 13) << 30);
        atuWord[2] = 0;
    }
    /* Trunk */
    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        atuWord[2] |= ((msgWordPtr[1] >> 15) & 0x01) << 2;
    }

    /* FID[11], this bit must match FID[11] passed to the ATU for a Hit */
    if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        atuWord[2] |= ((dbNum >> 11) & 0x01) << 3;
    }

    /* Make 32 bit word address */
    smemMemSet(devObjPtr, address, &atuWord[0], SOHO_ATU_ENTRY_SIZE_IN_WORDS);
}

/*******************************************************************************
*   sfdbSohoDeleteAtuEntry
*
* DESCRIPTION:
*       Delete ATU entry in SRAM
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       macAddrPtr  - MAC address pointer being deleted
*       dbNum       - lookup database number
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
static void sfdbSohoDeleteAtuEntry (
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum
)
{
    GT_U32 atuWord[SOHO_ATU_ENTRY_SIZE_IN_WORDS]; /* ATU entry word pointer */
    GT_U32 address = 0;                 /* MAC table memory address */
    GT_U32 entryIdx = 0;                /* ATU entry index */
    GT_STATUS status;                   /* return status */
    GT_U32  ii;

    status = sfdbSohoAtuEntryFind(devObjPtr, macAddrPtr, dbNum, &entryIdx);
    if (status == GT_NOT_FOUND)
    {
        return;
    }

    for(ii=0; ii < SOHO_ATU_ENTRY_SIZE_IN_WORDS; ii++)
    {
        atuWord[ii] = 0;
    }

    /* Make 32 bit word address */
    address = (4 << 28) | (entryIdx<<2) ;
    smemMemSet(devObjPtr, address, &atuWord[0], SOHO_ATU_ENTRY_SIZE_IN_WORDS);
}

/*******************************************************************************
*   sfdbSohoGetNextAtuEntry
*
* DESCRIPTION:
*       Get next ATU entry SRAM , in specific FDB (dbNum)
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       msgWordPtr  - pointer to fdb message
*       dbNum       - Vlan database number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK
*       GT_NOT_FOUND
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS sfdbSohoGetNextAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 * msgWordPtr,
    IN GT_U32 dbNum
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    SGT_MAC_ADDR_UNT macAddr;           /* MAC address from message */
    SGT_MAC_ADDR_UNT lookUpAddr;        /* lookup MAC address  */
    SGT_MAC_ADDR_UNT currAddr;          /* current MAC address  */
    GT_U32 lookUpIdx;                   /* lookup MAC address index */
    GT_U8 * macPtr;                     /* Pointer to MAC address */
    GT_U32 * atuEntryPtr;               /* ATU entry pointer */
    GT_U32 entryState;                  /* ATU entry state */
    GT_U32 fldValue;                    /* register value */
    GT_U32 atuIdx;                      /* ATU database entry index  */
    GT_U32 maxNumEnties;                /* max num entries in ATU */
    GT_U32 maxBucketIdx;                /* max bucket index */
    GT_U32 bucket = 0;                  /* bucket for current mac addr */
    GT_STATUS rc;                       /* return code */
    GT_32 result1, result2;/* NOT SIGNED  results of addresses compare */
    GT_U32 i;                           /* index in memory */
    GT_BOOL fid11;                       /* FID[11] used in OPAL+ for matching */
                                         /* 49th bit in ATU comparison         */

    GT_U32 binmode;                     /* bin selector bits */
    GT_U32 countmode;                   /* bin counter mode what ATU entry in any counter */


    rc = GT_NOT_FOUND;

    if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    { /* at the start of ATU NEXT clear 4 counters */
        counter_valid_entry = 0;
        counter_valid_non_static_entry = 0;
        counter_valid_entry_fid = 0;
        counter_valid_non_static_entry_fid = 0;
    }
    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
    lookUpIdx = 0;
    /* Copy input MAC address for the first time */
    macPtr = (GT_U8 *)&msgWordPtr[2];
    /* Start with MAC address from fdb message */
    SOHO_SWAP_MSG_2_MAC_FROM_CPU(macPtr, macAddr);

    /* set the lookup mac addr to max */
    memset(lookUpAddr.bytes,0xFF,6);

    macAddr.bytes[5]++;
    for(i = 5; i != 0 ; i--)
    {
        if(macAddr.bytes[i] == 0)
        {
            macAddr.bytes[i-1]++;
        }
        else
        {
            break;
        }
    }

    maxNumEnties = memInfoPtr->macDbMem.macTblMemSize /
                                SOHO_ATU_ENTRY_SIZE_IN_WORDS;

    maxBucketIdx = maxNumEnties/SOHO_ATU_BUCKET_SIZE_IN_BINS;

    for (atuIdx = 0; atuIdx < maxNumEnties; atuIdx++)
    {
        i = atuIdx * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
        /* Get ATU database entry  */
        atuEntryPtr = &memInfoPtr->macDbMem.macTblMem[i];
        entryState = SMEM_U32_GET_FIELD(atuEntryPtr[1], 16, 4);
        /* Not empty entry */
        if (entryState)
        {
            macPtr = (GT_U8 *)atuEntryPtr;
            SOHO_MSG_2_MAC(macPtr, currAddr);

            /* calc dbNum of current entry */
            bucket  = snetSohoMacHashCalc(devObjPtr, currAddr.bytes);

            /* calc dbNum of current entry */
            bucket += dbNum & 0x7FF; /* In OPAL+ Take 11bits of DBnum 12bits. */
            bucket %= maxBucketIdx;

            if((atuIdx/SOHO_ATU_BUCKET_SIZE_IN_BINS) != bucket)
            {
                /* this entry not match the needed FDB (dbNum)*/
                continue;
            }

             /* Compare 49th bit with FID[11]. */
            if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {
                fid11 = (dbNum >> 11) & 0x1;     /* and the 12th bit is used for matching */
                if ( fid11 != ((GT_U8) SMEM_U32_GET_FIELD(atuEntryPtr[2], 3, 1)))
                    continue;
            }

            result1 = memcmp(currAddr.bytes, macAddr.bytes, 6);
            result2 = memcmp(currAddr.bytes, lookUpAddr.bytes, 6);
            if (result1 >= 0 && result2 <= 0)
            {
                memcpy(lookUpAddr.bytes, currAddr.bytes, 6);
                lookUpIdx = i;
                /* indicate that at least one entry was found to be
                   next from base */
                rc = GT_OK;
            }
        }
    }

    /* Set ATU registers  */
    if (rc == GT_NOT_FOUND)
    {
        /*  1. we need to set to broadcast mac address */
        fldValue = 0xffff;
        smemMemSet(devObjPtr, GLB_ATU_MAC0_1_REG, &fldValue, 1);
        smemMemSet(devObjPtr, GLB_ATU_MAC2_3_REG, &fldValue, 1);
        smemMemSet(devObjPtr, GLB_ATU_MAC4_5_REG, &fldValue, 1);

        /*  2. we need to set to entryState = 0 */
        fldValue = 0;
        smemMemSet(devObjPtr, GLB_ATU_DATA_REG, &fldValue, 1);

        return GT_NOT_FOUND;
    }

    atuEntryPtr = &memInfoPtr->macDbMem.macTblMem[lookUpIdx];

    fldValue = SMEM_U32_GET_FIELD(atuEntryPtr[1], 16, 16);
    smemMemSet(devObjPtr, GLB_ATU_DATA_REG, &fldValue, 1);

    fldValue = SMEM_U32_GET_FIELD(atuEntryPtr[1], 0, 8) |
               SMEM_U32_GET_FIELD(atuEntryPtr[1], 8, 8) << 8;
    smemMemSet(devObjPtr, GLB_ATU_MAC0_1_REG, &fldValue, 1);

    fldValue = SMEM_U32_GET_FIELD(atuEntryPtr[0], 16, 8) |
               SMEM_U32_GET_FIELD(atuEntryPtr[0], 24, 8) << 8;
    smemMemSet(devObjPtr, GLB_ATU_MAC2_3_REG, &fldValue, 1);

    fldValue = SMEM_U32_GET_FIELD(atuEntryPtr[0], 0, 8) |
               SMEM_U32_GET_FIELD(atuEntryPtr[0], 8, 8) << 8;
    smemMemSet(devObjPtr, GLB_ATU_MAC4_5_REG, &fldValue, 1);

    if ((devObjPtr->deviceType == SKERNEL_RUBY) ||
        SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* the ruby write the priority to the Atu operation register
            bits 8-10 */
        fldValue = ((atuEntryPtr[1] >> 31) | /* one bit */
                   ((atuEntryPtr[2] & 0x3) << 1)); /* two bits */
        smemRegFldSet(devObjPtr, GLB_ATU_OPER_REG, 8, 3, fldValue);
    }

    if(SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* set the trunk bit */
        fldValue = ((atuEntryPtr[2] >> 2) & 0x01);
            smemRegFldSet(devObjPtr, GLB_ATU_DATA_REG, 15, 1, fldValue);
    }

    if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        smemRegFldGet(devObjPtr, GLB2_ATU_STST_REG, 14, 2, &binmode);
        /* Read all entries in ATU table */
        for (atuIdx = 0 + binmode; atuIdx < maxNumEnties; atuIdx ++)
        {
            i = atuIdx * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
            /* Get ATU database entry  */
            atuEntryPtr = &memInfoPtr->macDbMem.macTblMem[i];
            entryState = SMEM_U32_GET_FIELD(atuEntryPtr[1], 16, 4);
            /* Not empty entry */
            if (entryState)
            {   /* calc dbNum of current entry */
                bucket = bucket + (dbNum & 0x7FF); /* In OPAL+ Take 11bits of DBnum 12bits. */
                fid11 = (dbNum >> 11) & 0x1;     /* and the 12th bit is used for matching */


                bucket += dbNum;
                bucket %= maxBucketIdx;
                smemRegFldGet(devObjPtr, GLB2_ATU_STST_REG, 12, 2, &countmode);
                if((atuIdx/SOHO_ATU_BUCKET_SIZE_IN_BINS) != bucket)
                { /* this entry not match the needed FDB (dbNum)*/
                    continue;
                }

                smemRegFldGet(devObjPtr, GLB2_ATU_STST_REG, 12, 2, &countmode);


                switch (countmode)
                {
                    case 0:
                       counter_valid_entry ++;
                    break;

                    case 1:
                       if (entryState <= 7)
                       {
                           counter_valid_non_static_entry ++;
                       }
                    break;

                    case 2:
                       if ((atuIdx >= bucket) && (atuIdx < bucket + 3))
                       {
                           counter_valid_non_static_entry ++;
                       }
                    break;

                    case 3:
                        if ((atuIdx >= bucket) && (atuIdx < bucket + 3) &&
                            (entryState <= 7))
                       {
                           counter_valid_non_static_entry ++;
                       }
                    break;
                }
            }
        }
    }

    return GT_OK;
}

/*******************************************************************************
*   sfdbSohoFlushAtuEntry
*
* DESCRIPTION:
*       0 entire Atu entries that match :
*       1. specific dbNum/all
*       2. dynamic entries/all
* INPUTS:
*       devObjPtr   - device object PTR.
*       dbNum    - specific dbNum or all (SOHO_ALL_DB_NUM_CNS)
*       onlyDynamic - only dynamic/all
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbSohoFlushAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   dbNum,
    IN GT_BOOL  onlyDynamic
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    GT_U32 * atuEntryPtr;               /* ATU entry pointer */
    GT_U32 entryState;                  /* ATU entry state */
    GT_U32 atuIdx;                      /* ATU database entry index  */
    GT_U32 maxNumEnties;                /* max num entries in ATU */
    GT_U32 maxBucketIdx;                /* max bucket index */
    GT_U32 bucket;                      /* bucket for current mac addr */
    GT_U32 ii;                          /* index in memory */
    GT_BOOL currentIsDynamic;           /* current ATU entry is dynamic */
    SGT_MAC_ADDR_UNT currAddr;          /* current MAC address  */
    GT_U8 * macPtr;                     /* Pointer to MAC address */
    GT_U32  emptyAtuEntryPtr[SOHO_ATU_ENTRY_SIZE_IN_WORDS]={0,0,0};
    GT_U32  address;                    /* address to be modified */

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    maxNumEnties = memInfoPtr->macDbMem.macTblMemSize /
                            SOHO_ATU_ENTRY_SIZE_IN_WORDS;

    maxBucketIdx = maxNumEnties/SOHO_ATU_BUCKET_SIZE_IN_BINS;

    for (atuIdx = 0; atuIdx < maxNumEnties; atuIdx++)
    {
        ii = atuIdx * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
        /* Get ATU database entry  */
        atuEntryPtr = &memInfoPtr->macDbMem.macTblMem[ii];

        entryState = SMEM_U32_GET_FIELD(atuEntryPtr[1], 16, 4);

        currentIsDynamic = ((SOHO_MSB_IN_ATU_ENTRY_MAC(atuEntryPtr) & 0x1) == 0x01) ?
                                GT_FALSE/* mcmac address */:
                                (entryState<=7)?
                                GT_TRUE:/* dynamic unicast address */
                                GT_FALSE/* static unicast address */;

        if(onlyDynamic && (currentIsDynamic==GT_FALSE))
        {
            /* we need to modify only dynamic entries ,
               and this entry is not dynamic */
            continue;
        }

        /* Not empty entry */
        if (entryState)
        {
            if(dbNum!=SOHO_ALL_DB_NUM_CNS)
            {
                macPtr = (GT_U8 *)atuEntryPtr;
                SOHO_MSG_2_MAC(macPtr, currAddr);

                /* calc dbNum of current entry */
                bucket  = snetSohoMacHashCalc(devObjPtr, currAddr.bytes);
                if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                    bucket = bucket + (dbNum & 0x7FF); /* In OPAL+ Take 11bits of DBnum 12bits. */
                else
                    bucket += dbNum;

                bucket %= maxBucketIdx;

                if((atuIdx/SOHO_ATU_BUCKET_SIZE_IN_BINS) != bucket)
                {
                    /* this entry not match the needed FDB (dbNum)*/
                    continue;
                }
            }
        }
        else
        {
            /*check if entry has info*/

            /* I want to reduce the num of massages to Visualizer */

            if(0==memcmp(atuEntryPtr,emptyAtuEntryPtr,
                SOHO_ATU_ENTRY_SIZE_IN_WORDS*4))
            {
                /* we DON'T need to reset this entry */
                continue;
            }
        }

        /* if we are here we need to reset the entry */


        /* the next actions will also send visualizer the update */
        /* calc the address */
        address = (4 << 28) | (ii<<2) ;
        smemMemSet(devObjPtr, address, &emptyAtuEntryPtr[0],
                    SOHO_ATU_ENTRY_SIZE_IN_WORDS);
    }

}

/*******************************************************************************
*   sfdbSohoMoveAtuEntry
*
* DESCRIPTION:
*       Reassign all valid entries associated with on port (fromPort)
*       and move association to another port (toPort)
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       msgWordPtr  - pointer to fdb message
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
static void sfdbSohoMoveAtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U16 * msgWordPtr
)
{
    GT_U32 fromPort, toPort;                /* Ports to Move/Remove */
    GT_U32 fromPortBmp, toPortBmp;          /* port converted bitmap */
    GT_U32 * atuEntryPtr;                   /* ATU entry pointer */
    GT_U32 atuIdx;                          /* ATU database entry index  */
    GT_U32 atuSize;                         /* ATU database size  */
    SOHO_ATU_OP_E atuOp;                    /* ATU opcode */
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    GT_U32  address;                       /* address to be modified */
    GT_U32  currentPortsBmp;          /* bmp of ports current in the ATU entry*/
    GT_BOOL currentIsDynamic;           /* current ATU entry is dynamic */
    GT_U32 entryState;                  /* ATU entry state */
    GT_U32 ports;                       /* device ports number */

    atuOp = SMEM_U32_GET_FIELD(msgWordPtr[0], 12, 3);

    fromPortBmp = toPortBmp = 0;
    /* FromPort */
    fromPort = SMEM_U32_GET_FIELD(msgWordPtr[1], 4, 4);
    /* ToPort */
    toPort = SMEM_U32_GET_FIELD(msgWordPtr[1], 8, 4);

    /* Convert ports to it's bitmap */
    fromPortBmp =   1 << fromPort;
    toPortBmp =     1 << toPort;

    /* support 10/11 bits */
    toPortBmp   &= ((1 << devObjPtr->portsNumber)-1);
    fromPortBmp &= ((1 << devObjPtr->portsNumber)-1);
    ports = (GT_U16)devObjPtr->portsNumber;

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
    atuSize = memInfoPtr->macDbMem.macTblMemSize /
                            SOHO_ATU_ENTRY_SIZE_IN_WORDS;

    for (atuIdx = 0; atuIdx < atuSize; atuIdx++)
    {
        GT_U32 i = atuIdx * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
        atuEntryPtr = &memInfoPtr->macDbMem.macTblMem[i];

        entryState = SMEM_U32_GET_FIELD(atuEntryPtr[1], 16, 4);

        currentIsDynamic = ((SOHO_MSB_IN_ATU_ENTRY_MAC(atuEntryPtr) & 0x1) == 0x01) ?
                                GT_FALSE/* mcmac address */:
                               (entryState<=7)?
                                GT_TRUE:/* dynamic unicast address */
                                GT_FALSE/* static unicast address */;

        if((atuOp == ATU_FLUSH_MOVE_NON_STATIC_E) &&
           (currentIsDynamic==GT_FALSE))
        {
            /* we need to modify only dynamic entries ,
               and this entry is not dynamic */
            continue;
        }

        currentPortsBmp = SMEM_U32_GET_FIELD(atuEntryPtr[1], 20, ports);
        if (0==(currentPortsBmp & fromPortBmp))
        {
            /* the "from port" is not member in this entry */
            continue;
        }

        /* remove the "from port" from the current bmp */
        currentPortsBmp &= ~fromPortBmp;

        if (toPort != 0xf)
        {
            /* add the "to port" to current bmp */
            currentPortsBmp |= toPortBmp;
        }

        /* update the current bmp */
        SMEM_U32_SET_FIELD(atuEntryPtr[1], 20, 10, currentPortsBmp);

        /* calc the address */
        address = (4 << 28) | (i<<2) ;
        /* the next actions will also send visualizer the update */
        address += 0x4;
        smemMemSet(devObjPtr, address, &atuEntryPtr[1], 1);
    }
}
/*******************************************************************************
*   sfdbSohoAtuEntryFind
*
* DESCRIPTION:
*       Find ATU database entry in the SRAM
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       macAddrPtr      - pointer to 48-bit MAC address
*                dbNum           - db Number
*
* OUTPUTS:
*       entryIdxPtr     - ATU database entry index pointer
*
*
* RETURNS:
*       GT_OK
*       GT_NOT_FOUND
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS  sfdbSohoAtuEntryFind
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum,
    OUT GT_U32 * entryIdxPtr
)
{
    GT_U32 bucket;                       /* bucket address */
    SOHO_DEV_MEM_INFO * memInfoPtr;      /* device's memory pointer */
    GT_U32 maxBucketIdx;                 /* max bucket index */
    SMEM_REGISTER * macTblMemPtr;        /* ATU table memory pointer */
    GT_U32 i;
    GT_U32 * atuWordPtr;                 /* ATU word pointer */
    SGT_MAC_ADDR_UNT macAddr1, macAddr2; /* MAC addresses being compared */
    GT_U32  entryState;                  /* ATU entry state */
    GT_U32  fid11;                       /* FID[11] used in OPAL+ for matching */

    ASSERT_PTR(macAddrPtr);

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    maxBucketIdx = memInfoPtr->macDbMem.macTblMemSize /
                   (SOHO_ATU_ENTRY_SIZE_IN_WORDS*SOHO_ATU_BUCKET_SIZE_IN_BINS);

    /* Compute the bucket address */
    bucket = snetSohoMacHashCalc(devObjPtr, macAddrPtr);

    /* Support per VLAN database */
    bucket = bucket + (dbNum & 0x7FF); /* In OPAL+ Take 11bits of DBnum 12bits. */
    fid11 = (dbNum >> 11) & 0x1;     /* and the 12th bit is used for matching */

    bucket %= maxBucketIdx;

    bucket *= (SOHO_ATU_ENTRY_SIZE_IN_WORDS * SOHO_ATU_BUCKET_SIZE_IN_BINS);
    macTblMemPtr = &memInfoPtr->macDbMem.macTblMem[bucket];
    memcpy(macAddr1.bytes, macAddrPtr, 6);
    /* Read bins at the bucket address */
    for (i = 0; i < SOHO_ATU_BUCKET_SIZE_IN_BINS; i++)
    {
        GT_U32 bin = i * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
        atuWordPtr = &macTblMemPtr[bin];
        entryState = SMEM_U32_GET_FIELD(atuWordPtr[1], 16, 4);
        /* Empty entry */
        if (entryState == 0)
        {
            continue;
        }

        SOHO_MSG_2_MAC(atuWordPtr, macAddr2);

        /* Compare 49th bit with FID[11]. */
        if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            if ( fid11 != (SMEM_U32_GET_FIELD(atuWordPtr[2], 3, 1)))
                continue;
        }

        if (SGT_MAC_ADDR_ARE_EQUAL(&macAddr1, &macAddr2))
        {
            /* ATU entry found */
            *entryIdxPtr = (bucket + bin);
            return GT_OK;
        }

    }
    /* ATU entry was not found */
    *entryIdxPtr = bucket;

    return GT_NOT_FOUND;
}

/*******************************************************************************
*   sfdbSohoAtuEntryGet
*
* DESCRIPTION:
*       Get ATU database entry in the SRAM
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       macAddrPtr      - pointer to 48-bit MAC address
*       dbNum           - Vlan database number
*
* OUTPUTS:
*       atuEntryPtr     - ATU database entry pointer
*
* RETURNS:
*       GT_OK           - ATU database entry found
*       GT_NOT_FOUND    - ATU database entry not found
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS sfdbSohoAtuEntryAddress (
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum,
    OUT GT_U32 * address
)
{
    GT_STATUS status;                       /* return status */
    GT_U32  entryIdx;                       /* ATU entry index */

    ASSERT_PTR(macAddrPtr);
    ASSERT_PTR(address);

    *address = 0;
    status = sfdbSohoAtuEntryFind(devObjPtr, macAddrPtr, dbNum, &entryIdx);

    if (status == GT_OK)
    {
        /* Make 32 bit word address */
        *address = (4 << 28) | (entryIdx << 2);
    }

    return status;
}

/*******************************************************************************
*   sfdbSohoFreeBinGet
*
* DESCRIPTION:
*       Check if all bins in the bucket are static
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       macAddrPtr      - pointer to 48-bit MAC address
*       dbNum           - Vlan database number
*       regAddrPtr      - pointer to ATU entry address in SRAM
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - all entries static
*       GT_NOT_FOUND    - not all entries static
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS sfdbSohoFreeBinGet(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum,
    OUT GT_U32 * regAddrPtr
)
{
    OUT GT_U32 entryIdx;                /* ATU database entry index pointer */
    GT_U32 entryState;                  /* ATU entry state */
    GT_STATUS status;                   /* return status */
    SMEM_REGISTER * macTblMemPtr;       /* ATU table memory pointer */
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    GT_U32 * atuWordPtr;                /* ATU word pointer */
    GT_U8 allStatic = 0;                /* number of static entries */
    GT_U8 i;
    SGT_MAC_ADDR_UNT macAddr;           /* MAC address from ATU entry */

    ASSERT_PTR(regAddrPtr);

    status = sfdbSohoAtuEntryFind(devObjPtr, macAddrPtr, dbNum, &entryIdx);
    if (status == GT_OK)
    {
        return GT_FALSE;
    }

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
    macTblMemPtr = &memInfoPtr->macDbMem.macTblMem[entryIdx];
    /* Loop all bins  */
    for (i = 0; i < SOHO_ATU_BUCKET_SIZE_IN_BINS; i++)
    {
        GT_U32 bin = i * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
        atuWordPtr = &macTblMemPtr[bin];
        entryState = SMEM_U32_GET_FIELD(atuWordPtr[1], 16, 4);
        /* Empty empty */
        if (entryState == 0)
        {
            /* Make 32 bit word address */
            *regAddrPtr = (4 << 28) | ((entryIdx + bin) << 2);
            break;
        }

        if (entryState == 0xf || entryState == 0xe)
        {
            allStatic++;
        }
        else
        {
            SOHO_MSG_2_MAC(atuWordPtr, macAddr);
            if (SGT_MAC_ADDR_IS_MCST(&macAddr))
            {
                allStatic++;
            }
        }
    }

    return (allStatic == 3) ? GT_OK : GT_NOT_FOUND;
}
/*******************************************************************************
*   sfdbSohoMacTableAging
*
* DESCRIPTION:
*       Age out MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sfdbSohoMacTableAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32  fldValue;                   /* field value of register */
    GT_U32  agingTime;                  /* aging time in milliseconds */

    while(1)
    {
        smemRegFldGet(deviceObjPtr, GLB_ATU_CTRL_REG, 4, 8, &fldValue);

        /* AgeTime */
        if (fldValue == 0)
        {
            return;
        }

        /* Address age time/# of Entries in milliseconds*/
        agingTime = (fldValue * 15 * 1000);

        sfdbSohoaMacTableDoAging(deviceObjPtr, agingTime);
        SIM_OS_MAC(simOsSleep)(agingTime / SOHO_MAX_ENTRY_STATE_IN_ATU_ENTRY_CNS);
    }

}
/*******************************************************************************
*   sfdbSohoaMacTableDoAging
*
* DESCRIPTION:
*       Age out MAC table entries.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       agingTime   -
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbSohoaMacTableDoAging
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  agingTime
)
{
    GT_U32 entryState;                  /* ATU entry state */
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    GT_U32 * atuWordPtr;                /* ATU word pointer */
    GT_U32 i;
    GT_U32 maxEntryIdx;                 /* max ATU entry index */
    GT_U32 entryIdx;                    /* current ATU entry index */
    GT_U32 regAddrPtr;                  /* ATU entry address */
    GT_U32 dbNumber;                    /* ATU entry db number */
    GT_U32 port;
    /*GT_U32 atuData = 0; */              /* Destination port vector/TrunkID */
    GT_U32 atuAgeInt;                   /* interrupt on aged out */
    GT_U32 holdAt1;                     /* hold atu entry at entry state 1 */
    GT_U32 messagePort;                 /* index for whole marvell tag ports except cpu port not network port*/
    GT_BIT dpv=0, dpvMember;
    GT_U16 * wordDataPtr;



    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    maxEntryIdx = memInfoPtr->macDbMem.macTblMemSize /
                                    (SOHO_ATU_ENTRY_SIZE_IN_WORDS);

    /* Loop all ATU entries */
    for (i = 0; i < maxEntryIdx; i++)
    {
        entryIdx = i * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
        atuWordPtr = &memInfoPtr->macDbMem.macTblMem[entryIdx];
        entryState = SMEM_U32_GET_FIELD(atuWordPtr[1], 16, 4);

        /* The Aging need to skip :
           1. multicast entries ,
           2. unicast entries with state > 7 ,
           3. non-used entries with state = 0 */
        if (entryState == 0 || entryState > 7 ||
            SMEM_U32_GET_FIELD(atuWordPtr[1],8,1))
        {
            continue;
        }

        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            if (entryState == 1)
            {
                 /* Update entry with new Entry_State value */
                if (SMEM_U32_GET_FIELD(atuWordPtr[2],2,1) == 0) /* Not Trunk*/
                {
                    dpv = SMEM_U32_GET_FIELD(atuWordPtr[1], 20, 11);
                }
                else
                {
                    /*dpv = trunkTable[SMEM_U32_GET_FIELD(atuWordPtr[1], 20, 4)];*/
                }

                for(port = 0 ; port < 11 ; port++)
                {
                    dpvMember =  SMEM_U32_GET_FIELD(dpv, port, 1);
                    regAddrPtr = PORT_ASSOC_VECTOR_REG + (port * 0x10000);
                    smemRegFldGet(devObjPtr, regAddrPtr, 14, 1, &atuAgeInt);
                    regAddrPtr = PORT_CTRL1_REG + (port * 0x10000);
                    smemRegFldGet(devObjPtr, regAddrPtr, 15, 1, &messagePort);


                    wordDataPtr = memInfoPtr->macDbMem.violationData;
                    memset(wordDataPtr, 0, 5 * sizeof(GT_U16));

                    /* DBNUM in ATU table 12 bits */
                    smemRegFldGet(devObjPtr, GLB_ATU_FID_REG, 0, 12, &dbNumber);

                    wordDataPtr[0] = (GT_U16)dbNumber;

                    wordDataPtr[1] = (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[1], 20+port, 1);

                    /* MAC in ATU table 16 bits */
                    wordDataPtr[2] = (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[0], 0, 16);

                    wordDataPtr[3] = (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[0], 16, 16);

                    wordDataPtr[4] = (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[1], 0, 16);


                     /* set ATU Violation - no matter if interrupt enabled,
                     status register is updated

                     smemRegFldSet(devObjPtr, GLB_STATUS_REG, 3, 1, 1);
                      check that interrupt enabled
                     smemRegFldGet(devObjPtr, GLB_CTRL_REG, 3, 1, &fldValue); */



                    if ((dpvMember == 1) && (messagePort == 0) && (atuAgeInt == 1) )
                    {
                        /* Generate interrupt on aged mac, and quit the loop. */
                         scibSetInterrupt(devObjPtr->deviceId);
                         break;
                    }

                }

                for(port = 0 ; port < 11 ; port++)
                {
                    dpvMember =  SMEM_U32_GET_FIELD(dpv, port, 1);
                    regAddrPtr = PORT_ASSOC_VECTOR_REG + (port * 0x10000);
                    smemRegFldGet(devObjPtr, regAddrPtr, 15, 1, &holdAt1);
                    if (holdAt1)
                    {
                        /* Increment here and the lines after this will decrement it to 1. */
                        entryState++;
                        break;
                    }

                }
            }
        }
        /* Update entry with new Entry_State value */
        SMEM_U32_SET_FIELD(atuWordPtr[1], 16, 4, --entryState);

        /* The next actions will also send visualizer the update */
        regAddrPtr = (4 << 28) | (entryIdx << 2);
        regAddrPtr += 0x4;
        smemMemSet(devObjPtr, regAddrPtr, &atuWordPtr[1], 1);
    }
}

