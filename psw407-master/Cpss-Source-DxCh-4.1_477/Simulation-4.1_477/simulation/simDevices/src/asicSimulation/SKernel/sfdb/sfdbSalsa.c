/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbSalsa.c
*
* DESCRIPTION:
*       This is a SFDB module of SKernel Salsa.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/suserframes/snetSalsa.h>
#include <asicSimulation/SKernel/salsaCommon/sregSalsa.h>
#include <asicSimulation/SKernel/smem/smemSalsa.h>

static void sfdbSalsaNaMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr
);

static void sfdbSalsaQxMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr,
    IN SFDB_UPDATE_MSG_TYPE    msgType
);

static void sfdbSalsaMacTableDoAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32  skipAgeOut
);

/* Defines */
#define SFDB_COPY_BIT_MAC(_src_word, _src_bit_num, _dst_bit_num) \
    ((((_src_word) >> (_src_bit_num))& 0x1) << (_dst_bit_num))

/* Number of words in the MAC entry */
#define SFDB_SALSA_MAC_ENTRY_WORDS_NUM_CNS  4

/* Number of bytes in the MAC entry */
#define SFDB_SALSA_MAC_ENTRY_BYTES_NUM_CNS          \
            (SFDB_SALSA_MAC_ENTRY_WORDS_NUM_CNS * 4)

/* sizeof mac table */
#define SFDB_SALSA_MAC_TABLE_SIZE_CNS       8192

/*******************************************************************************
*   sfdbSalsaMsgProcess
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
void sfdbSalsaMsgProcess
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
            sfdbSalsaNaMsgProcess(deviceObjPtr,msgWords);
            break;
        case SFDB_UPDATE_MSG_QA_E:
        case SFDB_UPDATE_MSG_QI_E:
            sfdbSalsaQxMsgProcess(deviceObjPtr,msgWords, msgType);
            break;
        default:
            skernelFatalError("sfdbSalsaMsgProcess: wrong message type %d",
                            msgType);
    }
}
/*******************************************************************************
*   sfdbSalsaMacTableAging
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
void sfdbSalsaMacTableAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32  regValue; /* field value of register */
    GT_U32  agingTime;  /* aging time in milliseconds */
    GT_U32  skipAgeOut;

    while(1)
    {
        /* check that automatic aging enabled by read TriggerMode field */
        smemRegGet(deviceObjPtr, MAC_TABLE_ACTION_0_REG, &regValue);
        /* if TriggerMode != 0 than automatic aging disabled */
        if ((regValue >> 2) & 0x01)
            return;

        /* get age out type - with remove or just skip setting */
        skipAgeOut = (regValue >> 3) & 0x01;

        sfdbSalsaMacTableDoAging(deviceObjPtr, skipAgeOut);

        /* get aging time in seconds * 10 */
        agingTime = ((regValue >> 5) & 0x3f);
        if (agingTime == 0)
            agingTime++;

        agingTime *= 10 * 1000;
        /*Sleep(agingTime);*/
        SIM_OS_MAC(simOsSleep)(agingTime);
    }

}

/*******************************************************************************
*   sfdbSalsaNaMsgProcess
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
static void sfdbSalsaNaMsg2MacEntry
(
    IN  GT_U32                * fdbMsgPtr,
    OUT GT_U32                * macEntryPtr
)
{
    GT_32 i;

    /* reset  entry */
    for (i = 0; i < 4; i++)
        macEntryPtr[i] = 0;

/* ================= word 0 of MAC entry ====================== */
    /*    1. MAC address lsb */
    macEntryPtr[0] = fdbMsgPtr[0] & 0xffff0000;

    /* 2. VID */
    macEntryPtr[0] |= (fdbMsgPtr[2] & 0xfff) << 4;

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

    /* 1. rxSniff */
    macEntryPtr[2] = SFDB_COPY_BIT_MAC(fdbMsgPtr[3], 27, 25);

    /* 2. SA_CMD, DA_CMD, Forse l3 cos */
    macEntryPtr[2] |= ((fdbMsgPtr[3] & 0x03E00000) >> 1);

    /* 3. Multiple and static */
    macEntryPtr[2] |= (fdbMsgPtr[3] & 0x000C0000);

    /* 4. DA_TC */
    macEntryPtr[2] |= ((fdbMsgPtr[3] & 0x00018000) << 1);

    /* 5. SA_TC */
    macEntryPtr[2] |= ((fdbMsgPtr[3] & 0x00003000) << 2);

    /* 6. VIDX */
    macEntryPtr[2] |= (((fdbMsgPtr[3] & 0x0000007F) << 2 |
                       ((fdbMsgPtr[2] >> 30) & 0x3)) << 5);

    /* 7. Port/Trunk ID */
    macEntryPtr[2] |= ((fdbMsgPtr[2] & 0x1F000000) >> 24);


}
/*******************************************************************************
*   sfdbSalsaNaMsgProcess
*
* DESCRIPTION:
*       Process New Address FDB update message.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       fdbMsgPtr   - pointer MAC update message.
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
static void sfdbSalsaNaMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr
)
{
    SGT_MAC_ADDR_TYP  macAddr; /* mac address */
    GT_U16            vid;     /* vid */
                      /* MAC entry*/
    GT_U32            macEntry[SFDB_SALSA_MAC_ENTRY_WORDS_NUM_CNS];
    GT_U32            entryAddr; /* address of free MAC entry */
    GT_U32            regData; /* register's data */
    GT_BOOL           isFound;   /* mac entry found */
    GT_U32            macIntMask; /* MAC interrupt mask register */
    GT_BOOL           macIntrNotify = GT_FALSE; /* notify interrupt flag */
    GT_U32            entryIdx; /* mac entry index */

    SNET_SALSA_MAC_TBL_STC  salsaMacEntryPtr; /* MAC Address Table Entry */

    /* get MAC address and VID from message */
    sfdbMsg2Mac(fdbMsgPtr,&macAddr);
    sfdbMsg2Vid(fdbMsgPtr,&vid);

    /* find MAC entry */
    isFound = snetSalsaFindMacEntry(deviceObjPtr, macAddr.bytes, vid,
                                     &salsaMacEntryPtr, &entryIdx);
    if(isFound)
    {
        entryAddr = MAC_TAB_ENTRY_WORD0_REG + entryIdx * 0x10;
    }
    else
    {
        /* find free place in the MAC table */
        entryAddr = snetSalsaGetFreeMacEntryAddr(deviceObjPtr,
                    macAddr.bytes,vid);
    }

    /* convert message to the MAC entry */
    sfdbSalsaNaMsg2MacEntry(fdbMsgPtr, macEntry);

    /* write entry to free place */
    if (entryAddr != NOT_VALID_ADDR)
    {
        smemMemSet(deviceObjPtr, entryAddr, macEntry,
                        SFDB_SALSA_MAC_ENTRY_WORDS_NUM_CNS);
    }

    /* get interrupt mask register*/
    smemRegGet(deviceObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, &macIntMask);

    if (entryAddr != NOT_VALID_ADDR)
    {   /* set NAfromCPULearned of MAC Table Interrupt Cause Register */
        /* check that interrupt enabled */
        if (macIntMask & 0x2000)
        {
            regData = 0x2001;
            smemRegSet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, regData);
            macIntrNotify = GT_TRUE;
        }
    }
    else
    {   /* set NAfromCPUDropped of MAC Table Interrupt Cause Register  */
        /* check that interrupt enabled */
        if (macIntMask & 0x4000)
        {
            regData = 0x4001;
            smemRegSet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, regData);
            macIntrNotify = GT_TRUE;
        }
    }

    /* set UpdateFromCPUDone of MAC Table Interrupt Cause Register  */
    if (macIntMask & 0x200)
    {
        /* get register */
        smemRegGet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, &regData);
        regData |= 0x201;

        /* set register */
        smemRegSet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, regData);
        macIntrNotify = GT_TRUE;
    }

    /* set global interrupt bits */
    if (macIntrNotify == GT_TRUE)
    {
        /* get global interrupt mask */
        smemRegGet(deviceObjPtr,GLOBAL_INT_MASK_REG, &regData);
        if (regData & 0x40)
        {
            /* Set global interrupts bits */
            regData = 0x41;
            smemRegSet(deviceObjPtr,GLOBAL_INT_CAUSE_REG, regData);
            /* Set interrupt */
            scibSetInterrupt(deviceObjPtr->deviceId);
        }
    }
}

/*******************************************************************************
*   sfdbSalsaQxMsgProcess
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
static void sfdbSalsaQxMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * fdbMsgPtr,
    IN SFDB_UPDATE_MSG_TYPE    msgType
)
{
    SGT_MAC_ADDR_TYP  macAddr; /* mac address */
    GT_U16            vid;     /* vid */

    GT_U32          * freeFifoEntryPtr = NULL;
    GT_U32            fifoSize;
    GT_U32          * fifoBufferPrt;
    GT_U32            msgEnable;
    SALSA_DEV_MEM_INFO   * memInfoPtr;

    SNET_SALSA_MAC_TBL_STC salsaMacEntry;
    GT_U32            entryIdx; /* index of found entry */
    GT_BOOL           isFound; /* entry is found */
    GT_U32            regData; /* register's data */
    GT_U32            macIntMask = 0; /* MAC interrupt mask register */
    GT_BOOL           macIntrNotify = GT_FALSE; /* notify interrupt flag */
    GT_U32            i;

    /* check that message to CPU is enabled */
    smemRegFldGet(deviceObjPtr, MAC_TABLE_CTRL_REG, 9, 1, &msgEnable);
    if (msgEnable == 1)
    {
        /* find free place in the FIFO */
        smemRegFldGet(deviceObjPtr, FIFO_TO_CPU_CONTROL_REG, 1, 4, &fifoSize);

        memInfoPtr =  (SALSA_DEV_MEM_INFO *)(deviceObjPtr->deviceMemory);
        fifoBufferPrt = memInfoPtr->macFbdMem.macUpdFifoRegs;
        for (i = 0; i < fifoSize; i++) {
            if (*(fifoBufferPrt + 4*i) != 0xffffffff)
                continue;
            freeFifoEntryPtr =  (fifoBufferPrt + 4*i);
            break;
        }

        /* get interrupt mask register*/
        smemRegGet(deviceObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, &macIntMask);

        if (freeFifoEntryPtr == NULL) {

            /* FIFO to CPU Full interrupt */
            if (macIntMask & (1 << 16)) {
                smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 16, 1, 1);
                smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
                macIntrNotify = GT_TRUE;
            }
        }
        else
        {   /* there is free place in the FIFO, create reply message */

            /* get MAC address and VID from message */
            sfdbMsg2Mac(fdbMsgPtr,&macAddr);
            sfdbMsg2Vid(fdbMsgPtr,&vid);

            /* find MAC entry */
            isFound =  snetSalsaFindMacEntry(deviceObjPtr, macAddr.bytes, vid,
                                             &salsaMacEntry, &entryIdx);

            /* set QR message code */
            fdbMsgPtr[0] &= ~(3 << 4);
            fdbMsgPtr[0] |=  (2 << 4);

            if(isFound == GT_FALSE)
            {
                /* clear 15 bit of first byte */
                fdbMsgPtr[0] &= ~(1 << 15);
            }
            else
            {
                /* set 15 bit of first byte */
                fdbMsgPtr[0] |= (1 << 15);

                if (msgType == SFDB_UPDATE_MSG_QI_E)
                {
                    /* form reply for QI request */
                    fdbMsgPtr[2] &= ~(0xffffffff << 12);
                    fdbMsgPtr[2] |=  (entryIdx << 12);
                }
                else
                {
                    /* form reply for QA request */
                    fdbMsgPtr[2] =
                       (salsaMacEntry.vid |
                        salsaMacEntry.skipEntry << 12 |
                        salsaMacEntry.aging << 13 |
                        salsaMacEntry.trunk << 14 |
                        salsaMacEntry.port << 24 |
                        salsaMacEntry.vidx << 30);

                    fdbMsgPtr[3] =
                       (salsaMacEntry.vidx |
                        salsaMacEntry.saPriority << 12 |
                        salsaMacEntry.daPriority << 15 |
                        salsaMacEntry.daCmd << 22 |
                        salsaMacEntry.saCmd << 24 |
                        salsaMacEntry.rxSniff << 27);
                }
            }

            /* copy message to the FIFO */
            memcpy(freeFifoEntryPtr, fdbMsgPtr, 16);
        }

        /* set message to CPU ready */
        if (macIntMask & (1 << 10)) {
            /* get register */
            smemRegGet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, &regData);
            regData |= ((1 << 10) | 1);

            /* set register */
            smemRegSet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, regData);
            macIntrNotify = GT_TRUE;
        }
    }

    /* set UpdateFromCPUDone of MAC Table Interrupt Cause Register  */
    if (macIntMask & 0x200)
    {
        /* get register */
        smemRegGet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, &regData);
        regData |= 0x201;

        /* set register */
        smemRegSet(deviceObjPtr,MAC_TAB_INTR_CAUSE_REG, regData);
        macIntrNotify = GT_TRUE;
    }

    /* set global interrupt bits */
    if (macIntrNotify == GT_TRUE)
    {
        /* get global interrupt mask */
        smemRegGet(deviceObjPtr,GLOBAL_INT_MASK_REG, &regData);
        if (regData & 0x40)
        {
            /* Set global interrupts bits */
            regData = 0x41;
            smemRegSet(deviceObjPtr,GLOBAL_INT_CAUSE_REG, regData);
            /* Set interrupt */
            scibSetInterrupt(deviceObjPtr->deviceId);
        }
    }

}
/*******************************************************************************
*   sfdbSalsaMac2AAMsg
*
* DESCRIPTION:
*       Fill AA FIFO message from MAC table entry.
*
* INPUTS:
*       macEntryPtr  - pointer to MAC entry.
*
* OUTPUTS:
*       fdbMsgPtr    - pointer to FDB message.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbSalsaMac2AAMsg
(
    IN   GT_U32                * macEntryPtr,
    IN   GT_U32                  ownDeviceId,
    OUT  GT_U32                * fdbMsgPtr
)
{

/* ================= word 0  ====================== */
    /* message id and AA message type */
    fdbMsgPtr[0] = 0x32;

    /*    MAC address lsb */
    fdbMsgPtr[0] |= macEntryPtr[0] & 0xffff0000;

/* ================= word 1  ========================*/
    fdbMsgPtr[1] = macEntryPtr[1];

/* ================= word 2  ========================*/

    /*  VID */
    fdbMsgPtr[2] = (macEntryPtr[0] >> 4) & 0xfff;

    /* skip age and trunk bits */
    fdbMsgPtr[2] |= (macEntryPtr[0] & 0xe) << 12;

    /* ownDevNum */
    fdbMsgPtr[2] |= ownDeviceId << 16;

    /* port trunk number */
    fdbMsgPtr[2] |= (macEntryPtr[2] & 0x1f) << 24;

    /* vidx [1:0] */
    fdbMsgPtr[2] |= ((macEntryPtr[2] >> 5) & 0x3) >> 30;

/* ================= word 3  ========================*/

    /* vidx [8:2] */
    fdbMsgPtr[3] = (macEntryPtr[2] >> 7) & 0x7f;

    /* SA_TC */
    fdbMsgPtr[3] |= ((macEntryPtr[2] >> 14) & 0x3) << 12;

    /* DA_TC */
    fdbMsgPtr[3] |= ((macEntryPtr[2] >> 16) & 0x3) << 15;

    /* static and multiple */
    fdbMsgPtr[3] |= ((macEntryPtr[2] >> 18) & 0x3) << 18;

    /* force l3 Cos, da-cmd, sa-cmd */
    fdbMsgPtr[3] |= ((macEntryPtr[2] >> 20) & 0x1f) << 21;

    /* rx snif */
    fdbMsgPtr[3] |= SFDB_COPY_BIT_MAC(macEntryPtr[2], 25, 27);

}
/*******************************************************************************
*   sfdbSalsaMacAAUpdateSend
*
* DESCRIPTION:
*       Send AA update message to CPU.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       macEntryAddr - address of first word of Mac Entry
*       macEntryPtr - pointer to the MAC entry
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/

static void sfdbSalsaMacAAUpdateSend
(
    IN SKERNEL_DEVICE_OBJECT *  deviceObjPtr,
    IN GT_U32                   macEntryAddr,
    IN GT_U32_PTR               macEntryPtr
)
{
    GT_U32 * freeFifoEntryPtr = NULL; /* pointer to the free FIFO entry */
    GT_U32 fifoSize, fldValue;
    GT_U32 * fifoBufferPrt = NULL;
    GT_U32 msgEnable;
    GT_U32 setIntr = 0;
    GT_U32 i;
    SALSA_DEV_MEM_INFO   * memInfoPtr;

    /* check that FDB messaging enabled - get ForwMACUpdToCPU */
    smemRegFldGet(deviceObjPtr, MAC_TABLE_CTRL_REG, 9, 1, &msgEnable);
    if (!msgEnable)
        return;

    /* get FIFO size CPUFifo-treshold */
    smemRegFldGet(deviceObjPtr, FIFO_TO_CPU_CONTROL_REG, 1, 4, &fifoSize);

    memInfoPtr =  (SALSA_DEV_MEM_INFO *)(deviceObjPtr->deviceMemory);
    fifoBufferPrt = memInfoPtr->macFbdMem.macUpdFifoRegs;

    for (i = 0; i < fifoSize; i++) {
        if (*(fifoBufferPrt + 4*i) != 0xffffffff)
            continue;
        freeFifoEntryPtr =  (fifoBufferPrt + 4*i);
        break;
    }

    /* set interrupt if no free FIFO buffers */
    if (freeFifoEntryPtr == NULL) {
        smemRegFldGet(deviceObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 16, 1, &fldValue);
        /* MacTable CauseReg Mask */
        if (fldValue) {
            smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 16, 1, 1);
            smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
            setIntr = 1;
        }
    }
    else
    {
        /* get device ID */
        smemRegFldGet(deviceObjPtr, GLOBAL_CONTROL_REG, 4, 5, &fldValue);

        /* fill fifoEntry */
        sfdbSalsaMac2AAMsg(macEntryPtr, fldValue, freeFifoEntryPtr);

        /* set age out interrupt bit */
        smemRegFldGet(deviceObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 15, 1, &fldValue);
        /* MacTable CauseReg Mask */
        if (fldValue) {
            smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 15, 1, 1);
            smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
            setIntr = 1;
        }

        /* set ready Msg interrupt */
        smemRegFldGet(deviceObjPtr, MAC_TAB_INTR_CAUSE_MASK_REG, 10, 1, &fldValue);
        /* MacTable CauseReg Mask */
        if (fldValue) {
            smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 10, 1, 1);
            smemRegFldSet(deviceObjPtr, MAC_TAB_INTR_CAUSE_REG, 0, 1, 1);
            setIntr = 1;
        }
    }

    /* Call interrupt */
    if (setIntr == 1)
    {
        /* Global Interrupt Mask Register */
        smemRegFldGet(deviceObjPtr, GLOBAL_INT_MASK_REG, 6, 1, &fldValue);
        /* MTIntSumRO mask*/
        if (fldValue) {
            smemRegFldSet(deviceObjPtr, GLOBAL_INT_CAUSE_REG, 6, 1, 1);
            smemRegFldSet(deviceObjPtr, GLOBAL_INT_CAUSE_REG, 0, 1, 1);
            scibSetInterrupt(deviceObjPtr->deviceId);
        }
    }

}
/*******************************************************************************
*   sfdbSalsaMacTableDoAging
*
* DESCRIPTION:
*       Age out MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       skipAgeOut - 1 means set "skip" bit othervise reset "valid" bit
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/

static void sfdbSalsaMacTableDoAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32  skipAgeOut
)
{
    GT_U32   macEntryAddr;    /* address of first word of Mac Entry */
    GT_U32 * macEntryPtr;     /* pointer to the MAC entry */
    GT_U32   entryIdx;        /* index of MAC entry */
    GT_U32   entryWord0;
    GT_U32   entryWord2;

    for (entryIdx = 0; entryIdx < SFDB_SALSA_MAC_TABLE_SIZE_CNS; entryIdx++)
    {
        macEntryAddr = MAC_TAB_ENTRY_WORD0_REG +
                        SFDB_SALSA_MAC_ENTRY_BYTES_NUM_CNS * entryIdx;

        /* get mac table entry */
        macEntryPtr = smemMemGet(deviceObjPtr,macEntryAddr);
        entryWord0 = *macEntryPtr;
        entryWord2 =  *(macEntryPtr + 2);
        /* check entry's validity */
        if ((entryWord0 & 0x3) != 1)
            continue;

        /* check static bit */
        if(entryWord2 >> 18)
            continue;

        /* check aging bit */
        if ((entryWord0 & 0x4) == 1)
        {
            /* young entry just reset aging bit */
            entryWord0 &= ~0x4;
            smemMemSet(deviceObjPtr,macEntryAddr,&entryWord0, 1);
        }
        else
        {
            if (skipAgeOut)
            {
                entryWord0 |= 0x2;
            }
            else
            {
                entryWord0 &= 0xfffffffe;
            }

            smemMemSet(deviceObjPtr,macEntryAddr,&entryWord0, 1);

            /* entry should be aged out */
            sfdbSalsaMacAAUpdateSend(deviceObjPtr, macEntryAddr, macEntryPtr);
        }
    }
}
