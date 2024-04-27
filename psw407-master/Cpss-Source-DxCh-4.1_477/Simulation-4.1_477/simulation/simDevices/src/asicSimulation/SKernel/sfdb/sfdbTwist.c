/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbTwist.c
*
* DESCRIPTION:
*       This is a SFDB module of SKernel Twist.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 13 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>

static GT_VOID sfdbTwistNaMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr
);

static GT_VOID sfdbTwistQxMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr,
    IN SFDB_UPDATE_MSG_TYPE    msgType
);

static void sfdbTwistAgingL2TableEntries
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr ,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
)  ;

static void sfdbTwistTransplantL2TableEntries
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr ,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
)  ;



static GT_VOID sfdbTwistFdbAgingProc
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN GT_U32                * tblActPtr,
  IN GT_U32                  action,
  IN GT_U32                  firstEntryIdx,
  IN GT_U32                  numOfEntries
);

/* Defines */
#define SFDB_COPY_BIT_MAC(_src_word, _src_bit_num, _dst_bit_num) \
    ((((_src_word) >> (_src_bit_num))& 0x1) << (_dst_bit_num))


/* Number of words in the MAC entry */
#define SFDB_TWIST_MAC_ENTRY_WORDS_NUM_CNS  4

/* Number of bytes in the MAC entry */
#define SFDB_TWIST_MAC_ENTRY_BYTES_NUM_CNS          \
            (SFDB_TWIST_MAC_ENTRY_WORDS_NUM_CNS * 4)

/* sizeof mac table */
#define SFDB_TWIST_MAC_TABLE_SIZE_CNS    (16 * 1024)

typedef enum {
    SFDB_TWIST_FDB_AGING_REMOVE_E = 0,
    SFDB_TWIST_FDB_AGING_NO_REMOVE_E,
    SFDB_TWIST_FDB_DELETE_E,
    SFDB_TWIST_FDB_TRANS_E
}SFDB_TWIST_ACTION_ENT;

#define DEVICE_IS_REGISTERED(ownDevNum,dev, dev_arr)  \
        (((1 << ((dev) % 32)) & (dev_arr)[(dev) / 32]) || (ownDevNum == dev))

#define CHECK_ACTION_ON_STATIC_ENTRY(twist_mac_entry, changeStaticEn)   \
        if(twist_mac_entry.staticEntry && (!changeStaticEn))            \
        {                                                               \
            continue;                                                   \
        }

/*******************************************************************************
*   sfdbTwistMsgProcess
*
* DESCRIPTION:
*       Process FDB update message.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
*******************************************************************************/
GT_VOID sfdbTwistMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U8                 * fdbMsgPtr
)
{
    GT_U32  * msgWords = (GT_U32 *)fdbMsgPtr; /* pointer to the first word */
    SFDB_UPDATE_MSG_TYPE    msgType;    /* message type value*/

    /* get MSG type: bits 4:6 of first word */
    msgType = (msgWords[0] & 0x70) >> 4;

    switch (msgType)
    {
        case SFDB_UPDATE_MSG_NA_E:
            sfdbTwistNaMsgProcess(deviceObjPtr,msgWords);
            break;
        case SFDB_UPDATE_MSG_QA_E:
        case SFDB_UPDATE_MSG_QI_E:
            sfdbTwistQxMsgProcess(deviceObjPtr,msgWords, msgType);
            break;
        default:
            skernelFatalError("sfdbTwistMsgProcess: wrong message type %d",
                            msgType);
    }
}
/*******************************************************************************
*   sfdbTwistNaMsgProcess
*
* DESCRIPTION:
*       Process New Address FDB update message.
*
* INPUTS:
*       fdbMsgPtr    - pointer to FDB message.
*
* OUTPUTS:
*       macEntryPtr  - pointer to MAC entry.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbTwistNaMsg2MacEntry
(
    IN  GT_U32                * fdbMsgPtr,
    OUT GT_U32                * macEntryPtr
)
{
    GT_U32 i;

    /* reset  entry */
    for (i = 0; i < 4; i++)
        macEntryPtr[i] = 0;

/* ================= word 0 of MAC entry ====================== */
    /*    1. MAC address lsb */
    macEntryPtr[0] = WORD_FIELD_GET(&fdbMsgPtr[0], 0, 16, 16) << 16;

    /* 2. VID */
    macEntryPtr[0] |= WORD_FIELD_GET(&fdbMsgPtr[2], 0, 0, 12) << 4;

    /* 3. trunk bit */
    macEntryPtr[0] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[2], 14, 3);

    /* 4. aging */
    macEntryPtr[0] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[2], 13, 2);

    /* 5. skip */
    macEntryPtr[0] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[2], 12, 1);

    /* 6. valid bit */
    macEntryPtr[0] |= 1;

/* ================= word 1 of MAC entry ====================== */
    macEntryPtr[1] = fdbMsgPtr[1];

/* ================= word 2 of MAC entry ====================== */
    /* 1.Dev# */
    macEntryPtr[2] = WORD_FIELD_GET(&fdbMsgPtr[2], 0, 16, 7);

    /* 2. Port/Trunk ID */
    macEntryPtr[2] |= WORD_FIELD_GET(&fdbMsgPtr[2], 0, 24, 6) << 8;

    /* 3. VIDX */
    macEntryPtr[2] |= (WORD_FIELD_GET(&fdbMsgPtr[2], 0, 30, 2) |
                       WORD_FIELD_GET(&fdbMsgPtr[3], 0, 0, 12) << 2) << 16;

/* ================= word 3 of MAC entry ====================== */
    /* 1. TCs */
    macEntryPtr[3] = WORD_FIELD_GET(&fdbMsgPtr[3], 0, 12, 3);

    /* 2. TCd */
    macEntryPtr[3] |= WORD_FIELD_GET(&fdbMsgPtr[3], 0, 15, 3) << 4;

    /* 3. St */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 18, 8);

    /* 4. M */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 19, 9);

    /* 5. DA_CMD */
    macEntryPtr[3] |= WORD_FIELD_GET(&fdbMsgPtr[3], 0, 22, 2) << 12;

    /* 6. SA_CMD */
    macEntryPtr[3] |= WORD_FIELD_GET(&fdbMsgPtr[3], 0, 24, 2) << 14;

    /* 7. SA_TC */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 26, 16);

    /* 3. SA_epi */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 27, 17);

    /* 4. DA_TC */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 28, 20);

     /* 3. DA_epi */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 29, 21);

    /* 4. DA_rout */
    macEntryPtr[3] |= SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 30, 22);

}
/*******************************************************************************
*   sfdbTwistNaMsgProcess
*
* DESCRIPTION:
*       Process New Address FDB update message.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       fdbMsgPtr   - pointer to MAC update message.
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
static GT_VOID sfdbTwistNaMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr
)
{
    SGT_MAC_ADDR_TYP  macAddr; /* mac address */
    GT_U16            vid;     /* vid */
    /* MAC entry*/
    GT_U32            macEntry[SFDB_TWIST_MAC_ENTRY_WORDS_NUM_CNS];
    GT_BOOL           macIntrNotify = GT_FALSE; /* notify Mac interrupt flag */
    GT_BOOL           globIntrNotify = GT_FALSE; /* glob interrupt flag */
    GT_U32            fldValue; /* register field value */
    GT_BOOL           isFound;   /* mac entry found */
    GT_U32            entryIdx; /* mac entry index */
    GT_U32            regAddress; /* address of free MAC entry */

    SNET_TWIST_MAC_TBL_STC  twistMacEntryPtr; /* MAC Address Table Entry */

    /* get MAC address and VID from message */
    sfdbMsg2Mac(fdbMsgPtr,&macAddr);
    sfdbMsg2Vid(fdbMsgPtr,&vid);

    /* find MAC entry */
    isFound = snetTwistFindMacEntry(deviceObjPtr, macAddr.bytes, vid,
                                     &twistMacEntryPtr, &entryIdx);
    if(isFound)
    {
        regAddress = MAC_TAB_ENTRY_WORD0_REG + entryIdx * 0x10;
    }
    else
    {
        /* find free place in the MAC table */
        regAddress = snetTwistGetFreeMacEntryAddr(deviceObjPtr,
                                              macAddr.bytes,vid);
    }

    /* convert message to the MAC entry */
    sfdbTwistNaMsg2MacEntry(fdbMsgPtr, macEntry);
    /* write entry to free place */
    if (regAddress != NOT_VALID_ADDR)
    {
        smemMemSet(deviceObjPtr, regAddress, macEntry,
                        SFDB_TWIST_MAC_ENTRY_WORDS_NUM_CNS);
    }

    if (regAddress != NOT_VALID_ADDR)
    {
        /* check that interrupt enabled */
        smemRegFldGet(deviceObjPtr, ETHER_BRDG_INTR_MASK_REG, 4, 1, &fldValue);
        /* set NALearned of Bridge Configuration Interrupt Registers */
        if (fldValue)
        {
            smemRegFldSet(deviceObjPtr, ETHER_BRDG_INTR_REG, 4, 1, 1);
            smemRegFldSet(deviceObjPtr, ETHER_BRDG_INTR_REG, 0, 1, 1);
            macIntrNotify = GT_TRUE;
        }
    }
    else
    {
        /* check that interrupt enabled */
        smemRegFldGet(deviceObjPtr, ETHER_BRDG_INTR_MASK_REG, 5, 1, &fldValue);
        /* set NANotLearned of Bridge Configuration Interrupt Registers */
        if (fldValue)
        {
            smemRegFldSet(deviceObjPtr, ETHER_BRDG_INTR_REG, 5, 1, 1);
            smemRegFldSet(deviceObjPtr, ETHER_BRDG_INTR_REG, 0, 1, 1);
            macIntrNotify = GT_TRUE;
        }
    }

    /* set interrupt AU processed if not masked*/
    smemRegFldGet(deviceObjPtr, MISC_INTR_MASK_REG, 5, 1, &fldValue);
    /* AUQPending Mask */
    if (fldValue) {
        smemRegFldSet(deviceObjPtr, MISC_INTR_CAUSE_REG, 5, 1, 1);
        smemRegFldSet(deviceObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);

        /* PCI interrupt */
        smemPciRegFldGet(deviceObjPtr, PCI_INT_MASK_REG, 16, 1, &fldValue);
        /* Misc interrupt */
        if (fldValue)
        {
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 16, 1, 1);
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }


    /* set global interrupt bits */
    if (macIntrNotify == GT_TRUE)
    {
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(deviceObjPtr, PCI_INT_MASK_REG, 11, 1, &fldValue);
        /* Ethernet BridgeSumInt */
        if (fldValue)
        {
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 11, 1, 1);
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }

    if (globIntrNotify == GT_TRUE)
    {
        scibSetInterrupt(deviceObjPtr->deviceId);
    }
}

/*******************************************************************************
*   sfdbTwistQxMsgProcess
*
* DESCRIPTION:
*       Process QI and QR FDB update message.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       fdbMsgPtr   - pointer to device object.
*       msgType     - message type
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbTwistQxMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr,
    IN SFDB_UPDATE_MSG_TYPE    msgType
)
{
    SGT_MAC_ADDR_TYP    macAddr; /* mac address */
    GT_U16              vid;     /* vid */
    GT_BOOL             isFound; /* entry is found */
    GT_U32              entryIdx; /* index of found entry */
    GT_BOOL             setMiscIntr; /* set misc interrupt bit */
    GT_U32              fldValue; /* register field value */
    GT_BOOL             auqResult = GT_FALSE;  /* AU message sent result */
    GT_BOOL           macIntrNotify = GT_FALSE; /* notify Mac interrupt flag */
    GT_BOOL           globIntrNotify = GT_FALSE; /* glob interrupt flag */
    SNET_TWIST_MAC_TBL_STC  twistMacEntryPtr; /* MAC Address Table Entry */

    setMiscIntr = GT_FALSE;
    /* there is free place in the FIFO, create reply message */

    /* get MAC address and VID from message */
    sfdbMsg2Mac(fdbMsgPtr, &macAddr);
    sfdbMsg2Vid(fdbMsgPtr, &vid);

    /* find MAC entry */
    isFound =  snetTwistFindMacEntry(deviceObjPtr, macAddr.bytes, vid,
                                     &twistMacEntryPtr, &entryIdx);

    /* the QI not changing the message type */
    if(SMEM_U32_GET_FIELD(fdbMsgPtr[0], 4, 3) == SFDB_UPDATE_MSG_QA_E)
    {
        /* set QR message code */
        SMEM_U32_SET_FIELD(fdbMsgPtr[0], 4, 3, 2);
    }

    if(isFound == GT_FALSE)
    {
        /* clear 15 bit of first byte */
        SMEM_U32_SET_FIELD(fdbMsgPtr[0], 15, 1, 0);
    }
    else
    {
        /* set 15 bit of first byte */
        SMEM_U32_SET_FIELD(fdbMsgPtr[0], 15, 1, 1);

        if (msgType == SFDB_UPDATE_MSG_QI_E)
        {
            /* form reply for QI request */
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 12, 14, entryIdx);
        }
        else
        {
            /* form reply for QA request */
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 0, 12,
                               twistMacEntryPtr.vid);
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 12, 1,
                               twistMacEntryPtr.skipEntry);
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 13, 1,
                               twistMacEntryPtr.aging);
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 14, 1,
                               twistMacEntryPtr.trunk);
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 16, 7,
                               twistMacEntryPtr.dev);
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 24, 6,
                               twistMacEntryPtr.port);
            SMEM_U32_SET_FIELD(fdbMsgPtr[2], 30, 2,
                               twistMacEntryPtr.vidx);

            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 0, 12,
                               twistMacEntryPtr.vidx >> 2);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 12, 1,
                               twistMacEntryPtr.saClass);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 15, 1,
                               twistMacEntryPtr.daClass);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 18, 1,
                               twistMacEntryPtr.staticEntry);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 19, 7,
                               twistMacEntryPtr.multiple);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 22, 1,
                               twistMacEntryPtr.daCmd);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 24, 1,
                               twistMacEntryPtr.saCmd);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 26, 1,
                               twistMacEntryPtr.saClass);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 27, 1,
                               twistMacEntryPtr.saCib);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 28, 1,
                               twistMacEntryPtr.daClass);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 29, 1,
                               twistMacEntryPtr.daCib);
            SMEM_U32_SET_FIELD(fdbMsgPtr[3], 30, 1,
                               twistMacEntryPtr.daRout);

        }
    }

    /* For twist-D,Tiger 2 work around */
    if (SKERNEL_DEVICE_FAMILY_TWISTD(deviceObjPtr->deviceType)||
        SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        GT_U32 hwDataTmp[4];
        /* swap the data */
        hwDataTmp[0] = fdbMsgPtr[2];
        hwDataTmp[1] = fdbMsgPtr[3];
        hwDataTmp[2] = fdbMsgPtr[0];
        hwDataTmp[3] = fdbMsgPtr[1];

        fdbMsgPtr[0] = hwDataTmp[0];
        fdbMsgPtr[1] = hwDataTmp[1];
        fdbMsgPtr[2] = hwDataTmp[2];
        fdbMsgPtr[3] = hwDataTmp[3];
    }


    /* Send AU message to CPU */
    while(auqResult == GT_FALSE)
    {
        auqResult = snetTwistFdbAuSend(deviceObjPtr, fdbMsgPtr, &setMiscIntr);
        /* wait for SW to free buffers */
        SIM_OS_MAC(simOsSleep)(50);
    }

    /* set interrupt AU processed if not masked*/
    smemRegFldGet(deviceObjPtr, MISC_INTR_MASK_REG, 5, 1, &fldValue);
    /* AUQPending Mask */
    if (fldValue) {
        macIntrNotify = GT_TRUE;
        smemRegFldSet(deviceObjPtr, MISC_INTR_CAUSE_REG, 5, 1, 1);
        smemRegFldSet(deviceObjPtr, MISC_INTR_CAUSE_REG, 0, 1, 1);

        /* PCI interrupt */
        smemPciRegFldGet(deviceObjPtr, PCI_INT_MASK_REG, 16, 1, &fldValue);
        /* Misc interrupt */
        if (fldValue)
        {
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 16, 1, 1);
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }

    /* set global interrupt bits */
    if (macIntrNotify == GT_TRUE)
    {
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(deviceObjPtr, PCI_INT_MASK_REG, 11, 1, &fldValue);
        /* Ethernet BridgeSumInt */
        if (fldValue)
        {
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 11, 1, 1);
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            globIntrNotify = GT_TRUE;
        }
    }

    if (globIntrNotify == GT_TRUE)
    {
        scibSetInterrupt(deviceObjPtr->deviceId);
    }

}


/*******************************************************************************
*   sfdbTwistMacTableAging
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
void sfdbTwistMacTableAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32  * macTblActPtr;  /* pointer to MAC table action  */
    GT_U32  agingTime;  /* aging time in milliseconds */
    GT_U32  actionMode;
    GT_U32  trigMode;   /* CPU triggered/automatic aging mode */

    while(1)
    {
        /* check that automatic aging enabled by read TriggerMode field */
        macTblActPtr = smemMemGet(deviceObjPtr, MAC_TABLE_ACTION_0_REG);

        trigMode = /* if TriggerMode != 0 than automatic aging disabled */
            SMEM_U32_GET_FIELD(macTblActPtr[0], 2, 1);

        /* The action is performed via a trigger from the CPU */
        if (trigMode == 1)
        {
            return;
        }

        actionMode = /* Check if action on MAC entries are enabled */
            SMEM_U32_GET_FIELD(macTblActPtr[0], 3, 2);


        sfdbTwistFdbAgingProc(deviceObjPtr, macTblActPtr, actionMode,
                              0, SFDB_TWIST_MAC_TABLE_SIZE_CNS);

        agingTime = /* get aging time in seconds * 10 */
            SMEM_U32_GET_FIELD(macTblActPtr[0], 5, 6);

        if (agingTime == 0)
            agingTime++;

        agingTime *= (10 * 1000); /*10 for the second , *1000 for the mili*/

        SIM_OS_MAC(simOsSleep)(agingTime);
    }
}

/*******************************************************************************
*   sfdbTwistAgingL2TableEntries
*
* DESCRIPTION:
*       Age out MAC table entries.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*       twistMacEntry  - Mac entry to be aged.
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbTwistAgingL2TableEntries
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr ,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
)
{
    GT_U32                  hwData[4];
    GT_BOOL                 auqResult = GT_FALSE;
    GT_BOOL                 setMiscIntr = GT_FALSE;
    GT_U32                  fldValue;
    GT_BOOL                 setPciIntr  = GT_FALSE;

    snetTwistCreateAAMsg(deviceObjPtr,twistMacEntryPtr,hwData);
    /* Send AA message to CPU */
    while(auqResult == GT_FALSE)
    {
        auqResult = snetTwistFdbAuSend(deviceObjPtr, hwData, &setMiscIntr);
        /* wait for SW to free buffers */
        SIM_OS_MAC(simOsSleep)(50);
    }
    /* Call interrupt */
    if (setMiscIntr == GT_TRUE)
    {
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(deviceObjPtr, PCI_INT_MASK_REG, 16, 1, &fldValue);
        if (fldValue)
        {
            /* Ethernet BridgeSumInt */
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 16, 1, 1);
            /* IntSum */
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            setPciIntr = GT_TRUE;
        }
    }

    if (setPciIntr == GT_TRUE)
    {
        scibSetInterrupt(deviceObjPtr->deviceId);
    }
}


/*******************************************************************************
*   sfdbTwistTransplantL2TableEntries
*
* DESCRIPTION:
*       send transplant messages to CPU .
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*       twistMacEntry  - Mac entry to be aged.
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbTwistTransplantL2TableEntries
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr ,
    IN SNET_TWIST_MAC_TBL_STC * twistMacEntryPtr
)
{
    GT_U32                  hwData[4];
    GT_BOOL                 auqResult = GT_FALSE;
    GT_BOOL                 setMiscIntr = GT_FALSE;
    GT_U32                  fldValue;
    GT_BOOL                 setPciIntr  = GT_FALSE;

    snetTwistCreateAAMsg(deviceObjPtr,twistMacEntryPtr,hwData);
    /* update the AA to TA message */

    /* For twist-D,Tiger 2 work around */
    if (SKERNEL_DEVICE_FAMILY_TWISTD(deviceObjPtr->deviceType)||
        SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        /* Swapped data  */
        hwData[2] &= ~ (7<<4);
        hwData[2] |= ~ (4<<4);
    }
    else
    {
        hwData[0] &= ~ (7<<4);
        hwData[0] |= ~ (4<<4);
    }

    /* Send AA message to CPU */
    while(auqResult == GT_FALSE)
    {
        auqResult = snetTwistFdbAuSend(deviceObjPtr, hwData, &setMiscIntr);
        /* wait for SW to free buffers */
        SIM_OS_MAC(simOsSleep)(50);
    }
    /* Call interrupt */
    if (setMiscIntr == GT_TRUE)
    {
        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(deviceObjPtr, PCI_INT_MASK_REG, 16, 1, &fldValue);
        if (fldValue)
        {
            /* Ethernet BridgeSumInt */
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 16, 1, 1);
            /* IntSum */
            smemPciRegFldSet(deviceObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            setPciIntr = GT_TRUE;
        }
    }

    if (setPciIntr == GT_TRUE)
    {
        scibSetInterrupt(deviceObjPtr->deviceId);
    }
}

/*******************************************************************************
*   sfdbTwistFdbAgingProc
*
* DESCRIPTION:
*       MAC Table Triggering Action - delete address, aging without removal
        of aged out entries, triggered aging with removal of aged out entries.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - pointer to MAC Table Action register
*       action      - action mode being performed on the entries
*       firstEntryIdx - index to the first enrty .
*       numOfEntries  - number of entries to be searched.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbTwistFdbAgingProc
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN GT_U32                * tblActPtr,
  IN GT_U32                  action,
  IN GT_U32                  firstEntryIdx,
  IN GT_U32                  numOfEntries
)
{
    GT_U32 * devBmpPtr;                     /* Device bitmap */
    SNET_TWIST_MAC_TBL_STC  twistMacEntry;  /* MAC table entry pointer */
    GT_U32 actVlan, actVlanMask;            /* Action Active VLAN/VLAN mask*/
    GT_U32 staticDelEn = 0;                 /* Enables deleting of static addr */
    GT_U32 staticTransEn = 0;               /* Enables transplanting of static addr */
    GT_U32 oldPort = 0, oldDev = 0;         /* Old device and port value */
    GT_U32 newPort = 0, newDev = 0;         /* New device and port value */
    GT_U32 trigMode;                        /* Triggered mode */
    GT_U32 macEntryAddr;                    /* address of first word of Mac Entry */
    GT_U32 entryIdx;                        /* index of MAC ent*/
    GT_U32 interruptNo;/* interrupt bit number in the "trigger action" */
    GT_BOOL aaMessadgeNeeded;/* need to send AA message */
    GT_BOOL taMessadgeNeeded;/* need to send TA message */
    GT_U32  ownDevNum;

    smemRegFldGet(devObjPtr, GLOBAL_CONTROL_REG, 8, 7, &ownDevNum);

    actVlan = /* Action Active VLAN */
        SMEM_U32_GET_FIELD(tblActPtr[1], 8, 12);

    actVlanMask = /* Action Active VLAN Mask */
        SMEM_U32_GET_FIELD(tblActPtr[1], 20, 12);

    trigMode = /* Triggered/automatic mode */
        SMEM_U32_GET_FIELD(tblActPtr[0], 2, 1);

    if (trigMode)
    {
        staticDelEn = /* Enables deleting of static addresses */
            SMEM_U32_GET_FIELD(tblActPtr[0], 12, 1);

        staticTransEn = /* Static Address Transplant */
            SMEM_U32_GET_FIELD(tblActPtr[0], 11, 1);

        oldPort = /* Transplant action Old-port */
            SMEM_U32_GET_FIELD(tblActPtr[0], 13, 6);

        oldDev = /* Transplant action Old-Device number */
            SMEM_U32_GET_FIELD(tblActPtr[0], 19, 7);

        newPort = /* Transplant action New Port */
            SMEM_U32_GET_FIELD(tblActPtr[0], 26, 6);

        newDev = /* Transplant action New Device */
            SMEM_U32_GET_FIELD(tblActPtr[1], 0, 7);
    }

    devBmpPtr = smemMemGet(devObjPtr, DEV_TBL_REG0);

    for (entryIdx = firstEntryIdx;
         entryIdx < firstEntryIdx + numOfEntries ;
         entryIdx++)
    {
        macEntryAddr = MAC_TAB_ENTRY_WORD0_REG +
                        SFDB_TWIST_MAC_ENTRY_BYTES_NUM_CNS * entryIdx;

        snetTwistGetMacEntry(devObjPtr, macEntryAddr , &twistMacEntry );

        /*****************************/
        /** check if entry is valid **/
        /*****************************/
        if (twistMacEntry.validEntry == 0)
        {
            continue;
        }

        /******************************/
        /** check if skip is enabled **/
        /******************************/
        if (twistMacEntry.skipEntry)
        {
            continue;
        }

        /* delete silently - unknown device , regardless to current action !!!
           regardless to current vlanMask */
        if (! DEVICE_IS_REGISTERED(ownDevNum,twistMacEntry.dev, devBmpPtr))
        {
            twistMacEntry.skipEntry = 1;
            /* in this case the device not send AA message to the CPU !!! */
            /* just Update the MAC table entry */
            snetTwistSetMacEntry(devObjPtr, macEntryAddr, &twistMacEntry);
            continue;
        }

        /******************************/
        /** check if vlan masked     **/
        /******************************/
        if ((twistMacEntry.vid & actVlanMask) != actVlan)
        {
            continue;
        }

        aaMessadgeNeeded = GT_FALSE;
        taMessadgeNeeded = GT_FALSE;

        /* Aging with removal of aged out entries (Automatic & CPU Triggered
           action)*/
        if (action == SFDB_TWIST_FDB_AGING_REMOVE_E)
        {
            if(twistMacEntry.staticEntry)
            {
                continue;
            }

            /* Triggered address aging with removal of aged out entries */
            /* Automatic address aging with removal of aged out entries */
            if (twistMacEntry.aging)
            {
                twistMacEntry.aging = 0;
            }
            else
            {
                twistMacEntry.skipEntry = 1;
                aaMessadgeNeeded = GT_TRUE;
            }
        }
        else
        /* Aging without removal of aged-out entries (Automatic & CPU Triggered
           action) */
        if (action == SFDB_TWIST_FDB_AGING_NO_REMOVE_E)
        {
            if(twistMacEntry.staticEntry)
            {
                continue;
            }
            /* Triggered address aging without removal of aged out entries */
            /* Automatic address aging without removal of aged out entries */
            if (twistMacEntry.aging)
            {
                twistMacEntry.aging = 0;
            }
            else
            {
                aaMessadgeNeeded = GT_TRUE;
            }
        }
        else
        /* Address deleting (CPU Triggered action only) */
        if (action == SFDB_TWIST_FDB_DELETE_E)
        {
            CHECK_ACTION_ON_STATIC_ENTRY(twistMacEntry, staticDelEn);
            /* skip */
            twistMacEntry.skipEntry = 1;
            aaMessadgeNeeded = GT_TRUE;
        }
        /* Address transplanting (CPU Triggered action only) */
        else
        {
            CHECK_ACTION_ON_STATIC_ENTRY(twistMacEntry, staticTransEn);

            if (twistMacEntry.dev != oldDev ||
                twistMacEntry.port != oldPort)
            {
                continue;
            }

            /* Transplant new dev/port */
            twistMacEntry.dev = (GT_U16)newDev;
            twistMacEntry.port = newPort;

            taMessadgeNeeded = GT_TRUE;
        }

        /* Set MAC table entry */
        snetTwistSetMacEntry(devObjPtr, macEntryAddr, &twistMacEntry);

        if(aaMessadgeNeeded == GT_TRUE)
        {
            /* send Age out MAC table entries */
            sfdbTwistAgingL2TableEntries(devObjPtr, &twistMacEntry);
        }
        else if(taMessadgeNeeded == GT_TRUE)
        {
            /* send transplant MAC table entries */
            sfdbTwistTransplantL2TableEntries(devObjPtr, &twistMacEntry);
        }
    }


    /* The aging cycle that was initiated by a CPU trigger has ended */
    if (trigMode)
    {
        interruptNo = AGE_VIA_TRIGGER_END_INT ;
        sTwistDoInterrupt(devObjPtr,
                              ETHER_BRDG_INTR_REG,
                              ETHER_BRDG_INTR_MASK_REG,
                              interruptNo,
                              FDB_SUM_INT);
    }


}
/*******************************************************************************
*   sfdbTwistMacTableTriggerAction
*
* DESCRIPTION:
*       MAC Table Trigger Action
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - pointer table action data
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
void sfdbTwistMacTableTriggerAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * tblActPtr
)
{
    GT_U32  fldValue;               /* field value of register */
    GT_U32 * tblActWordPtr;         /* 32 bit data pointer */

    tblActWordPtr = (GT_U32 *)tblActPtr;

    /* Trigger Mode */
    fldValue = SMEM_U32_GET_FIELD(tblActWordPtr[0], 2, 1);
    if (fldValue)
    {
        /* ActionMode */
        fldValue = SMEM_U32_GET_FIELD(tblActWordPtr[0], 3, 2);

        sfdbTwistFdbAgingProc(devObjPtr, tblActWordPtr,
                              fldValue, 0, SFDB_TWIST_MAC_TABLE_SIZE_CNS);
    }
    /* Clear Aging Trigger */
    smemRegFldSet(devObjPtr, MAC_TABLE_ACTION_0_REG, 1, 1, 0);
}

/*******************************************************************************
*   sTwistDoInterrupt
*
* DESCRIPTION:
*       Set Twist interrupt
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       causeRegAddr        - Interrupt Cause Register
*       causeMaskRegAddr    - Interrupt Cause Register Mask
*       causeBitNum         - Interrupt Cause Register Bit
*       globalBitNum        - Global Interrupt Cause Register Bit
*
*       based on function snetChetahDoInterrupt(...)
*******************************************************************************/
GT_VOID sTwistDoInterrupt
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 causeRegAddr,
    IN GT_U32 causeMaskRegAddr,
    IN GT_U32 causeBitBmp,
    IN GT_U32 globalBitBmp
)
{
    GT_U32 causeRegVal;             /* Cause register value */
    GT_U32 causeRegMask;            /* Cause register mask */
    GT_BOOL setIntr = GT_FALSE;     /* Set interrupt */
    GT_BOOL setGlbIntr = GT_FALSE;  /* Set interrupt in global register */

    /* read interrupt cause data */
    smemRegGet(devObjPtr, causeRegAddr, &causeRegVal);

    /* read interrupt mask data */
    smemRegGet(devObjPtr, causeMaskRegAddr, &causeRegMask);

    /* set cause bit in the data for interrupt cause */
    causeRegVal |= causeBitBmp;

    /* if mask is set for bitmap, set summary bit in data for interrupt cause */
    if (causeBitBmp & causeRegMask)
    {
        SMEM_U32_SET_FIELD(causeRegVal, 0, 1, 1);
        setIntr = GT_TRUE;
    }

    smemRegSet(devObjPtr, causeRegAddr, causeRegVal);

    if (setIntr == GT_TRUE)
    {
        /* PCI Interrupt Cause Register */
        smemPciRegFldGet(devObjPtr, PCI_INT_CAUSE_REG,
                         0, 32, &causeRegVal);

        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG,
                         0, 32, &causeRegMask);

        /* set cause bit in the data for interrupt cause */
        causeRegVal |= globalBitBmp;

        /* if mask is set for bitmap, set summary bit in data for interrupt cause */
        if (globalBitBmp & causeRegMask)
        {
            /* IntSum */
            SMEM_U32_SET_FIELD(causeRegVal, 0, 1, 1);
            setGlbIntr = GT_TRUE;
        }

        smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG,
                         0, 32, causeRegVal);
    }

    if (setGlbIntr)
    {
        scibSetInterrupt(devObjPtr->deviceId);
    }
}

/*******************************************************************************
*   snetCheetahInterruptsMaskChanged
*
* DESCRIPTION:
*       handle interrupt mask registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       causeRegAddr   - cause register address
*       maskRegAddr    - mask  register address
*       intRegBit      - interrupt bit in the global cause register
*       currentCauseRegVal - current cause register values
*       lastMaskRegVal - last mask register values
*       newMaskRegVal  - new mask register values
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
extern void snetTigerInterruptsMaskChanged(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  causeRegAddr,
    IN GT_U32  maskRegAddr,
    IN GT_U32  intRegBit,
    IN GT_U32  currentCauseRegVal,
    IN GT_U32  lastMaskRegVal,
    IN GT_U32  newMaskRegVal
)
{
    GT_U32  diffCause;

    diffCause = ((newMaskRegVal & 0xFFFFFFFE) & ~lastMaskRegVal) & currentCauseRegVal;

    /* check if there is a reason to do interrupt */
    if(diffCause)
    {
        sTwistDoInterrupt(devObjPtr,causeRegAddr,
                              maskRegAddr,diffCause,intRegBit);
    }

    return;
}
