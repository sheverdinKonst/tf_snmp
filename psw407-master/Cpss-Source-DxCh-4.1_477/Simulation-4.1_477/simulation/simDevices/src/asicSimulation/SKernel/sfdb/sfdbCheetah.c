/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbCheetah.c
*
* DESCRIPTION:
*       Cheetah FDB update, Triggered actions messages simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 67 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <common/SBUF/sbuf.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SLog/simLog.h>

/* indication that the skernel task should wait until the application will reset
   the AUQ for new messages.

   the correct value is 0.

   for debug purposes only I allow to set it to 1 , to reconstruct starvation cases.
*/
GT_BIT  oldWaitDuringSkernelTaskForAuqOrFua = 0;
/* DEBUG function :
    to allow changing oldWaitDuringSkernelTaskForAuqOrFua from
   outside the simulation */
void simulationDebug_oldWaitDuringSkernelTaskForAuqOrFua(IN GT_BIT mode)
{
    oldWaitDuringSkernelTaskForAuqOrFua = mode;
}

static GT_STATUS sfdbChtNaMsgProcess(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * fdbMsgPtr
);

static GT_STATUS sfdbChtQxMsgProcess(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * fdbMsgPtr
);

static GT_STATUS sfdbChtHrMsgProcess(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * fdbMsgPtr
);

static GT_VOID sfdbChtWriteNaMsg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * fdbMsgPtr,
    IN GT_U32                fdbIndex,
    OUT GT_U32               entryOffset
);

static GT_VOID sfdbChtTriggerTa(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * tblActPtr
);

static GT_VOID sfdbChtDoAging(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN GT_U32                * tblActPtr,
  IN GT_U32                  action,
  IN GT_U32                  firstEntryIdx,
  IN GT_U32                  numOfEntries
);

static GT_VOID sfdbChtFdb2AuMsg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  fdbIndex,
    IN GT_U32                * fdbEntryPtr,
    OUT GT_U32               * macUpdMsgPtr
);

typedef enum {
    SFDB_CHEETAH_TRIGGER_AGING_REMOVE_E = 0,
    SFDB_CHEETAH_TRIGGER_AGING_NO_REMOVE_E,
    SFDB_CHEETAH_TRIGGER_DELETE_E,
    SFDB_CHEETAH_TRIGGER_TRANS_E ,
    SFDB_CHEETAH_AUTOMATIC_AGING_REMOVE_E,
    SFDB_CHEETAH_AUTOMATIC_AGING_NO_REMOVE_E,
    SFDB_CHEETAH_TRIGGER_UPLOAD_E
}SFDB_CHEETAH_ACTION_ENT;

#define POWER_2(a) ((a)*(a))
#define _256K   (256*1024)
#define _512K   (512*1024)
#define _1_MILLION  (1000*1000)

#define SFDB_AUTO_AGING_BUFF_SIZE     1000

#define SNET_CHEETAH_NO_FDB_ACTION_BUFFERS_SLEEP_TIME 50

typedef struct{
    /* transplant info : 'old interface' */
    GT_U32  oldPort;
    GT_U32  oldDev;
    GT_U32  oldIsTrunk;
    /* transplant info : 'new interface' */
    GT_U32  newPort;
    GT_U32  newDev;
    GT_U32  newIsTrunk;

    /* all actions (not transplant) info: value */
    GT_U32  actVlan;                /*also relevant for transplant action */
    GT_U32  actIsTrunk;
    GT_U32  actPort;
    GT_U32  actDevice;
    GT_U32  actUserDefined;
    GT_U32  actVID1;

    /* all actions (not transplant) info: mask */
    GT_U32  actVlanMask;            /*also relevant for transplant action */
    GT_U32  actIsTrunkMask;
    GT_U32  actPortMask;
    GT_U32  actDeviceMask;
    GT_U32  actUserDefinedMask;
    GT_U32  actVID1Mask;           /*also relevant for transplant action */


    GT_U32  staticDelEn;                /* Enable static entries deletion */
    GT_U32  staticTransEn;              /* Enable static entries translating */
    GT_U32  removeStaticOnNonExistDev;  /* The device automatically removes
                                        entries associated with devices that are
                                        not defined it the system. In addition
                                        to regular entries associated with
                                        devices that are not defined in the
                                        system, this field enables the removal
                                        of Static entries associated with those
                                        devices.
                                        0 - Static entries associated with non
                                            existing devices are not removed
                                        1- Static entries associated with non
                                        existing devices are removed */
    GT_U32  ageOutAllDevOnTrunk;/* do age on trunk entries that registered on
                                   other devices */
    GT_U32  ageOutAllDevOnNonTrunk;/* do age NOT on trunk entries that
                                      registered on other devices */
    GT_U32  aaAndTaMessageToCpuEn;/* enable/disable AA/TA messages to the CPU */

    GT_U32  routeUcAaAndTaMessageToCpuEn;/* route UC enable/disable AA/TA messages to the CPU */
    GT_U32  routeUcAgingEn;              /* route UC aging enable/disable */
    GT_U32  routeUcTransplantingEn;      /* route UC transplanting enable/disable */
    GT_U32  routeUcDeletingEn;           /* route UC deleting enable/disable */

    GT_U32* deviceTableBmpPtr;/* pointer to the BMP of 'device table'.
                              This device table is used by the aging mechanism
                              to delete all MAC entries associated with devices
                              that are not present in the system (hot removal)*/
    GT_U32  ownDevNum;        /* the 'own devNum' of this device */
    GT_U32  maskAuFuMessageToCpuOnNonLocal;/* Lion B -  When Enabled - AU/FU messages are
                                not sent to the CPU if the MAC entry does NOT reside
                                on the local core, i.e. the entry port[5:4] != LocalCore
                                0x0 = Disable;
                                0x1 = Enable; */
    GT_U32  maskAuFuMessageToCpuOnNonLocal_with3BitsCoreId; /*support mask of 3 bits of coreId with 3 bits from the entry : 2 of port + 1 of devNum */
    GT_U32  srcIdLengthInFdbMode;/* is Max Length SrcID in FDB Enabled ? */

    GT_U32  multiHashEnable;/* SIP5: is the multi hash enabled --->
                            0 - <multi hash enabled> not enabled --> means to use the skip bit
                            1 - <multi hash enabled>     enabled --> means to ignore the skip bit
                            */
/* cq# 150852 */
#define AgeAccordingToConfiguration 0
#define ForceAgeWithoutRemoval      1
    GT_U32  trunksAgingMode;/*  Trunks aging control
                 0x0 = AgeAccordingToConfiguration; Entries associated with Trunks
                    are aged out according to the ActionMode configuration.
                 0x1 = ForceAgeWithoutRemoval; Entries associated with Trunks are
                        aged-without-removal
                        regardless of the dev# they are associated with and
                        regardless of the configuration in the ActionMode configuration
                         */


    GT_U32  McAddrDelMode;/*    Disable removal of Multicast addresses during the aging process.
                                0 = Remove: Remove MC addresses in Aging process
                                1 = DontRemove: Do not remove MC addresses in Aging process*/
    GT_U32  IPMCAddrDelMode;/*This field sets the address deleting mode for IP MC FDB entries
                                NOTE: This is in addition to current configuration <MCAddrDelMode>
                                that controls aging on non IP Multicast entries in FDB.
                                0 = AgeNonStatic: IP MC entries are subjected to aging if not static.
                                1 = NoAge: IP MC entries are not subjected to aging regardless if static.*/
}AGE_DAEMON_ACTION_INFO_STC;


/*******************************************************************************
*   parseAuMsgGetFdbEntryHashInfo
*
* DESCRIPTION:
*       parse the AU message and get the FDB entry hash info .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       msgWordPtr  - pointer to fdb message.
* OUTPUTS:
*       entryInfoPtr    - (pointer to) entry hash info
*
* RETURN:
*       GT_OK           - FDB entry was found
*       GT_NOT_FOUND    - FDB entry was not found and assigned with new address
*       GT_FAIL         - FDB entry was not found and no free address space
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS parseAuMsgGetFdbEntryHashInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr,
    OUT SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    *entryInfoPtr
)
{
    DECLARE_FUNC_NAME(parseAuMsgGetFdbEntryHashInfo);

    GT_U32  fieldValue;
    GT_U32  macAddrWords[2];
    GT_U32  regValue = 0;

    memset(entryInfoPtr, 0, sizeof(SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC));
    entryInfoPtr->isSip5 = SMEM_CHT_IS_SIP5_GET(devObjPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,&regValue);

        /* entry type */
        entryInfoPtr->entryType =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE);
        /* FID */
        entryInfoPtr->fid =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID);

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            /* FDB lookup mode */
            entryInfoPtr->fdbLookupKeyMode =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE);

            /* VID1_assignment_mode */
            fieldValue = SMEM_U32_GET_FIELD(regValue, 27, 1);
            __LOG(("VID1 assignemnet mode: %d\n", fieldValue));
            if (fieldValue)
            {
                /* VID1 used in FDB entry hash entry for double tag FDB Lookup */
                entryInfoPtr->vid1 =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1);
            }
        }
    }
    else
    {
        /* entry type bits [6:4] word0 */
        entryInfoPtr->entryType = SMEM_U32_GET_FIELD(msgWordPtr[3], 19, 2);
        entryInfoPtr->fid = snetFieldValueGet(msgWordPtr,64,12);
    }

    /* save the original fid before any change */
    entryInfoPtr->origFid = entryInfoPtr->fid;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        entryInfoPtr->fid16BitHashEn = SMEM_U32_GET_FIELD(regValue, 4, 1);

        if(entryInfoPtr->fid16BitHashEn == 0)
        {
            entryInfoPtr->fid &= 0xfff;
        }
    }
    else
    {
        entryInfoPtr->fid16BitHashEn = 0;
    }

    switch(entryInfoPtr->entryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* get MAC address from message */
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_MAC_ADDR_GET(devObjPtr,msgWordPtr,
                    macAddrWords);

                entryInfoPtr->info.macInfo.macAddr[5] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 0) ,8);
                entryInfoPtr->info.macInfo.macAddr[4] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 1) ,8);
                entryInfoPtr->info.macInfo.macAddr[3] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 2) ,8);
                entryInfoPtr->info.macInfo.macAddr[2] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 3) ,8);
                entryInfoPtr->info.macInfo.macAddr[1] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 4) ,8);
                entryInfoPtr->info.macInfo.macAddr[0] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 5) ,8);
            }
            else
            {
            /* get MAC address from message */
            entryInfoPtr->info.macInfo.macAddr[5] = (GT_U8)((msgWordPtr[0] >> 16) & 0xff);
            entryInfoPtr->info.macInfo.macAddr[4] = (GT_U8)((msgWordPtr[0] >> 24) & 0xff);
            entryInfoPtr->info.macInfo.macAddr[3] = (GT_U8)(msgWordPtr[1]         & 0xff);
            entryInfoPtr->info.macInfo.macAddr[2] = (GT_U8)((msgWordPtr[1] >> 8)  & 0xff);
            entryInfoPtr->info.macInfo.macAddr[1] = (GT_U8)((msgWordPtr[1] >> 16) & 0xff);
            entryInfoPtr->info.macInfo.macAddr[0] = (GT_U8)((msgWordPtr[1] >> 24) & 0xff);
            }

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                fieldValue = SMEM_U32_GET_FIELD(regValue, 0, 2);

                switch(fieldValue)
                {
                    case 0:
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E;
                        break;
                    case 1:
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_FID_E;
                        break;
                    case 2:
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_MAC_E;
                        break;
                    case 3:
                    default:
                        __LOG(("ERROR : unknown <crcHashUpperBitsMode> value [%d] \n",
                                      fieldValue));
                        entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E;
                        break;
                }

            }
            else
            {
                entryInfoPtr->info.macInfo.crcHashUpperBitsMode = SNET_CHEETAH_FDB_CRC_HASH_UPPER_BITS_MODE_ALL_ZERO_E;
            }

            break;
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:/*IPv4 Multicast address entry (IGMP snooping);*/
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:/*IPv6 Multicast address entry (MLD snooping);*/
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* get SIP,DIP address from message */
                entryInfoPtr->info.ipmcBridge.dip =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DIP);

                entryInfoPtr->info.ipmcBridge.sip =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP);
            }
            else
            {
            entryInfoPtr->info.ipmcBridge.dip =
                SMEM_U32_GET_FIELD(msgWordPtr[0], 16, 16) |
                SMEM_U32_GET_FIELD(msgWordPtr[1], 0, 16) << 16;

            entryInfoPtr->info.ipmcBridge.sip =
                SMEM_U32_GET_FIELD(msgWordPtr[1], 16, 16) |
                SMEM_U32_GET_FIELD(msgWordPtr[3], 0, 12) << 16 |
                SMEM_U32_GET_FIELD(msgWordPtr[3], 27, 4) << 28;
            }
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:
            /* ipv4 dip */
            entryInfoPtr->info.ucRouting.dip[0] =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP);

            /* vrf id */
            entryInfoPtr->info.ucRouting.vrfId =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID);
            break;

        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
            /* fcoe d_id */
            entryInfoPtr->info.ucRouting.dip[0] =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID);

            /* vrf id */
            entryInfoPtr->info.ucRouting.vrfId =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID);
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E:
            __LOG(("ipv6 'not supported' (the device not support 'by message' the ipv6 entries)"));
            /* no break here */
        default:
            return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
*   parseAuMsgGetFdbEntryIndex
*
* DESCRIPTION:
*       parse the AU message and get the FDB entry index + entry's register address .
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       msgWordPtr  - pointer to fdb message.
* OUTPUTS:
*       entryIndexPtr   - index for the FDB found or free entry
*       entryOffsetPtr  - FDB offset within the 'bucket'
*
* RETURN:
*       GT_OK           - FDB entry was found
*       GT_NOT_FOUND    - FDB entry was not found and assigned with new address
*       GT_FAIL         - FDB entry was not found and no free address space
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS parseAuMsgGetFdbEntryIndex
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr,
    OUT GT_U32               * entryIndexPtr,
    OUT GT_U32               * entryOffsetPtr
)
{
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    entryHashInfo;
    GT_STATUS       status;

    status = parseAuMsgGetFdbEntryHashInfo(devObjPtr,msgWordPtr,&entryHashInfo);
    if(status != GT_OK)
    {
        return status;
    }

    return snetChtL2iMacEntryAddress(devObjPtr, &entryHashInfo, entryIndexPtr, entryOffsetPtr);
}


/*******************************************************************************
*   sfdbChtMsgProcess
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
*******************************************************************************/
GT_VOID sfdbChtMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * fdbMsgPtr
)
{
    GT_U32 * msgWordPtr;                /* pointer to the fdb first word */
    SFDB_UPDATE_MSG_TYPE msgType;       /* AU Message Type */
    GT_U32 regEntry = 0;                    /* register entry  */
    GT_U32 fldValue;                    /* register field value */
    GT_STATUS status = GT_TRUE;         /* function return status */

    msgWordPtr = (GT_U32 *)fdbMsgPtr;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* MSG type */
        msgType =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE);
    }
    else
    {
        /* MSG type bits [6:4] word0 */
        msgType = SMEM_U32_GET_FIELD(msgWordPtr[0], 4, 3);
    }

    switch (msgType)
    {
        case SFDB_UPDATE_MSG_NA_E:
            status = sfdbChtNaMsgProcess(devObjPtr, msgWordPtr);
            break;
        case SFDB_UPDATE_MSG_QA_E:
            status = sfdbChtQxMsgProcess(devObjPtr, msgWordPtr);

            if(devObjPtr->needResendMessage)
            {
                /* indication that the message was not processed */
                /* we need to re-try later */
                return;
            }

            break;
        case SFDB_UPDATE_MSG_HR_E:
            if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                skernelFatalError("sfdbChtMsgProcess: wrong message type %d",
                                   msgType);
            }
            /* Hash Request - called by the CPU to calculate all the HASH functions that the devices can generate :
                CRC -
                XOR -
                16 CRC multi-hash -
            */
            status = sfdbChtHrMsgProcess(devObjPtr, msgWordPtr);
            break;

        default:
            skernelFatalError("sfdbChtMsgProcess: wrong message type %d",
                               msgType);
    }

    /* Clear message trigger bit when the action is completed  */
    SMEM_U32_SET_FIELD(regEntry, 0, 1, 0);

    fldValue = (status == GT_FAIL) ? 0 : 1;
    /* AU Message Status */
    SMEM_U32_SET_FIELD(regEntry, 1, 1, fldValue);

    smemRegFldSet(devObjPtr, SMEM_CHT_MSG_FROM_CPU_REG(devObjPtr), 0,2,regEntry);
}

/*******************************************************************************
*   sfdbChtBankCounterActionAutoCalc
*
* DESCRIPTION:
*       SIP5 : auto calculate the increment/decrement/none action on the FDB bank
*           counters. as difference between 'old FDB entry' and the new FDB entry.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       oldIndex    - index of old entry
*       oldEntryPtr - pointer to the 'old' FDB entry (pointer to the FDB before
*                     apply the FDB update)
*       oldIndex    - index of new entry
*       newEntryPtr - pointer to the 'new' FDB entry (pointer to buffer that will
*                     be copied later into the FDB to update the existing entry)
*       client      - triggering client
* OUTPUTS:
*       counterActionPtr - (pointer to) the bank counter action.
*
* RETURNS:
*       None.
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbChtBankCounterActionAutoCalc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   oldIndex,
    IN GT_U32                   *oldEntryPtr,
    IN GT_U32                   newIndex,
    IN GT_U32                   *newEntryPtr,
    OUT SFDB_CHT_BANK_COUNTER_ACTION_ENT     *counterActionPtr,
    IN SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_ENT  client
)
{
    GT_BIT  spUnknown;
    GT_BIT  skip;
    GT_BIT  valid;
    GT_BIT  spUnknown_old;
    GT_BIT  skip_old;
    GT_BIT  valid_old;
    GT_BIT  entryConsideredValid_new;/* new entry considered valid */
    GT_BIT  entryConsideredValid_old;/* old entry considered valid */

    valid_old =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,oldEntryPtr,
            oldIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);
    skip_old =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,oldEntryPtr,
            oldIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP);
    spUnknown_old =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,oldEntryPtr,
            oldIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN);

    valid =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,newEntryPtr,
            newIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);
    skip =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,newEntryPtr,
            newIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP);
    spUnknown =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,newEntryPtr,
            newIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN);

    if(devObjPtr->multiHashEnable == 0)/* skip bit is NOT ignored */
    {
        entryConsideredValid_new = valid     && (skip == 0)       &&   (spUnknown == 0);
        entryConsideredValid_old = valid_old && (skip_old == 0)   &&   (spUnknown_old == 0);
    }
    else/* skip bit is ignored */
    {
        entryConsideredValid_new = valid     && /*(skip == 0)     &&*/ (spUnknown == 0);
        entryConsideredValid_old = valid_old && /*(skip_old == 0) &&*/ (spUnknown_old == 0);
    }


    if(entryConsideredValid_new)
    {
        /* although the validNew is 1 this entry should have been considered as deleted due to the errata */
        if((devObjPtr->fdbRouteUcDeletedEntryFlag==GT_TRUE) &&
           (client  == SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E))

        {
            /* new entry is NOT valid */
            *counterActionPtr = (entryConsideredValid_old) ?
                        SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E :  /*old entry is valid*/
                        SFDB_CHT_BANK_COUNTER_ACTION_NONE_E;        /*old entry is NOT valid*/
        }
        else
        {
            /* new entry is valid */
            *counterActionPtr = (entryConsideredValid_old) ?
                        SFDB_CHT_BANK_COUNTER_ACTION_NONE_E :       /*old entry is valid*/
                        SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E;   /*old entry is NOT valid*/
        }
    }
    else
    {
        /* new entry is NOT valid */
        *counterActionPtr = (entryConsideredValid_old) ?
                        SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E :  /*old entry is valid*/
                        SFDB_CHT_BANK_COUNTER_ACTION_NONE_E;        /*old entry is NOT valid*/
    }


    return;
}


/*******************************************************************************
*   sfdbChtNaMsgCheckAndProcessMoveBank
*
* DESCRIPTION:
*       check the NA message and if it is special case "NA move entry bank" then process it.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       msgWordPtr  - pointer MAC update message.
*
* OUTPUTS:
*       naProcessedPtr - (pointer to) indication that the NA was processed.
*                       GT_TRUE - the NA was processed as "NA move entry bank"
*                       GT_FALSE - the NA should be process as regular NA.
*
* RETURNS:
*
* COMMENTS:
*       GT_OK   - success
*       GT_FAIL - failed
*
*******************************************************************************/
static GT_STATUS sfdbChtNaMsgCheckAndProcessMoveBank
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr,
    OUT GT_BOOL              * naProcessedPtr
)
{
    DECLARE_FUNC_NAME(sfdbChtNaMsgCheckAndProcessMoveBank);

    GT_U32 value;        /* temp value */
    GT_U32 oldAddress;   /* old FDB entry address */
    GT_U32 oldFdbIndex;  /* old FDB entry index */
    GT_U32 *oldEntryPtr; /* pointer to the memory of the old entry in the FDB */
    GT_U32 oldFdbEntryType;
    GT_U32  valid;
    SNET_CHEETAH_FDB_ENTRY_ENT entryType;
    GT_U16               fid;
    GT_U8                macAddr[6];
    GT_U32               macAddrWords[2];
    GT_U32               sip=0;
    GT_U32               dip=0;
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC entryHashInfo;
    GT_U32  numValidPlaces;
    GT_U32  entryIndexArr[2];
    GT_U32  newFdbIndex;  /* new FDB entry index */
    GT_U32  newAddress;   /* new FDB entry address */
    GT_U32 *newEntryPtr; /* pointer to the memory of the old entry in the FDB */
    SFDB_CHT_BANK_COUNTER_ACTION_ENT counterAction;

    /* check if the NA massage is of "NA move entry bank" option */
    value =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr, msgWordPtr ,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_ENABLE);

    if(value == 0)
    {
        /* the NA massage is NOT "NA move entry bank" option */
        *naProcessedPtr = GT_FALSE;
        return GT_OK;
    }

    if(devObjPtr->multiHashEnable == 0)
    {
        /* no meaning of this message in <multi hash enable> = 0  */
        __LOG(("ERROR: ALERT: special NA message 'NA move entry bank' , but <multi hash enable> = 0 \n"));

        *naProcessedPtr = GT_TRUE;
        return GT_FAIL;
    }

    /* state that operation recognized as special case "NA move entry bank"
       so the caller function will not try to process it as regular NA message.*/
    *naProcessedPtr = GT_TRUE;

    __LOG(("special NA message 'NA move entry bank' \n"));


    /* 1. get the index of the entry that need to be moved
       2. get the FDB entry
       3. calculate the 16 banks that this entry can be in
       4. update the hash registers about the 16 hash values of the moved entry
       if found new location according to :
            5. copy the FDB entry into new bank which is most populated
                 between available banks (IGNORE the original bank)
                a. update the new bank counters (increment)
            6. invalidate the original entry
                a. update the old bank counters (decrement)
            7. update the <NA entry offset> about the new bank
            8. update <AUMsg StatusOK> = 'success'
            9. update <AU Message From CPU Trigger> = 'done'
            10. generate interrupt
       else (can't find new location that is not the original bank)
            5. update <AUMsg StatusOK> = 'FAILED'
            6. update <AU Message From CPU Trigger> = 'done'
            7. generate interrupt
    */
    /*19 bits supports 512K entries */
    oldFdbIndex =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr, msgWordPtr ,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_NA_MOVE_ENTRY_INDEX);

    oldAddress = SMEM_CHT_MAC_TBL_MEM(devObjPtr, oldFdbIndex);
    oldEntryPtr = smemMemGet(devObjPtr,oldAddress);

    valid =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, oldEntryPtr ,
            oldFdbIndex ,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);


    if(valid == 0)
    {
        /* the index to move is not valid */
        __LOG(("ERROR: ALERT: the entry in index [%d] to move is not valid \n",
                      oldFdbIndex));
        return GT_FAIL;
    }

/*
FDB:
    FDBEntryType    3..4
    VID             5..16

    MACAddr[14:0]   17..31
    MACAddr[39:15]  32..56
    MACAddr[40]     57
    MACAddr[47:41]  58..64

    DIP[31:0]       17..48

    SIP[14:0]       49..63
    SIP[27:15]      64..76
    SIP[28]         90
    SIP[30:29]      94..95
    SIP[31]         96

*/
    oldFdbEntryType =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, oldEntryPtr ,
            oldFdbIndex ,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE);

    entryType = oldFdbEntryType;
    fid =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, oldEntryPtr ,
            oldFdbIndex ,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID);

    /* build key to generate 16 hash functions */
    switch (oldFdbEntryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
            /* get the MAC ADDR words */
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_MAC_ADDR_GET(devObjPtr,
                oldEntryPtr,
                oldFdbIndex,
                macAddrWords);

            macAddr[5] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 0) ,8);
            macAddr[4] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 1) ,8);
            macAddr[3] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 2) ,8);
            macAddr[2] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 3) ,8);
            macAddr[1] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 4) ,8);
            macAddr[0] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 5) ,8);

            __LOG(("NA 'try to move' macAddr[%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x] vid[0x%3.3x] index [%d]\n"
                          ,macAddr[0]
                          ,macAddr[1]
                          ,macAddr[2]
                          ,macAddr[3]
                          ,macAddr[4]
                          ,macAddr[5]
                          ,fid/* vid - 12 bits*/
                          ,oldFdbIndex
                          ));

            break;
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:
            /* get 32 bits of dip */
            dip        =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, oldEntryPtr ,
                    oldFdbIndex ,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DIP);

            /* get 32 bits of sip */
            sip        =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr, oldEntryPtr ,
                    oldFdbIndex ,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP);
            break;
        default:
            /* not implemented + need more then the 2 bits */
            /* the index to move is not valid */
            __LOG(("ERROR: ALERT: the entry in index [%d] entry type not supported [%d] \n",
                          oldFdbIndex,oldFdbEntryType));
            return GT_FAIL;
    }

    fdbEntryHashInfoGet(devObjPtr,entryType,fid,macAddr,sip,dip,&entryHashInfo);

    /* get the 2 places that the old entry can occupy */
    snetChtL2iFdbLookupFor2Places(devObjPtr,&entryHashInfo,&numValidPlaces,entryIndexArr);

    if(numValidPlaces == 0)
    {
        /* should not happen --- other task remove the old entry ? */
        __LOG(("ERROR: ALERT: the entry in index [%d] NOT found any bank to be in ?! \n",
                      oldFdbIndex));
        return GT_FAIL;
    }
    else if (numValidPlaces == 1)
    {
        /* from all 16 places that this old entry could be in , already 15 are occupied
           so we can't move it to another place */
        __LOG(("The entry in index [%d] NOT found other bank to be in , so operation of 'move' failed \n",
                      oldFdbIndex));
        return GT_FAIL;
    }

    /* we need to check which one of the 2 entries is not the original and move the old entry into it */
    if(entryIndexArr[0] == oldFdbIndex)
    {
        /* FDB index in entryIndexArr[0] was more priority then entryIndexArr[1] */
        /* so only if the entryIndexArr[0] is the same as 'old index' we use entryIndexArr[1] */
        newFdbIndex = entryIndexArr[1];
    }
    else
    {
        newFdbIndex = entryIndexArr[0];
    }

    if((newFdbIndex % devObjPtr->fdbNumOfBanks) == (oldFdbIndex % devObjPtr->fdbNumOfBanks))
    {
        skernelFatalError("sfdbChtNaMsgCheckAndProcessMoveBank: the chosen new index[%d] is in the same bank as the old one[%d] ?! \n",
                        oldFdbIndex,newFdbIndex);
    }

    __LOG(("The entry in index [%d] found 2 places (at least) to be in [%d] and [%d] .the chosen index [%d] \n",
                  oldFdbIndex,
                  entryIndexArr[0],
                  entryIndexArr[1],
                  newFdbIndex));

    newAddress = SMEM_CHT_MAC_TBL_MEM(devObjPtr, newFdbIndex);
    newEntryPtr = smemMemGet(devObjPtr,newAddress);

    /* auto calc the counterAction by comparing the 'old' entry with the 'new' entry*/
    sfdbChtBankCounterActionAutoCalc(devObjPtr,
                        newFdbIndex,
                        newEntryPtr,        /* NOT BUG ! param called "oldEntryPtr" but we give it the "newEntryPtr"
                                               the entry before the change in the new index */
                        oldFdbIndex,
                        oldEntryPtr,        /* NOT BUG ! param called "newEntryPtr" but we give it the "oldEntryPtr"
                                                the entry to be into the new index */
                        &counterAction,
                        SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E);

    if(counterAction != SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E)
    {
        /* !!! SHOULD NOT HAPPEN !!!! */
        skernelFatalError("sfdbChtNaMsgCheckAndProcessMoveBank: the 'auto calc' found that the entry in new index[%d] compared to entry in old index[%d] \n"
                        " not considered as 'increment' but as [%d] \n",
                        oldFdbIndex,newFdbIndex,counterAction);
    }

    /* copy from old entry to the new entry ,
       only after calling sfdbChtBankCounterActionAutoCalc */
    smemMemSet(devObjPtr,newAddress,oldEntryPtr,SMEM_CHT_MAC_TABLE_WORDS);

    /* increment counter of the new bank */
    sfdbChtBankCounterAction(devObjPtr, newFdbIndex ,
        SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E,
        SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E);

    /* invalidate the old entry */
    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr, oldEntryPtr ,
        oldFdbIndex ,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID,
        0);

    /* decrement counter of the old bank */
    sfdbChtBankCounterAction(devObjPtr, oldFdbIndex ,
        SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E,
        SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E);

    /* operation succeeded */
    return GT_OK;
}

/*******************************************************************************
*   sfdbChtNaMsgProcess
*
* DESCRIPTION:
*       Process New Address FDB update message.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       msgWordPtr  - pointer MAC update message.
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
static GT_STATUS sfdbChtNaMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr
)
{
    DECLARE_FUNC_NAME(sfdbChtNaMsgProcess);

    GT_U32 fdbIndex;                         /* FDB entry index */
    GT_STATUS status = GT_FAIL;                       /* function return status */
    GT_U32 causBitMap = 0;                  /* Interrupt cause bitmap  */
    GT_U32 entryOffset;                     /* MAC entry offset */
    GT_BOOL naProcessed = GT_FALSE;

    if(devObjPtr->fdbBankMoveMessageSupported)
    {
        /* check if the operation recognized as special case "NA move entry bank"
           so the must not try to process it as regular NA message.*/
        status = sfdbChtNaMsgCheckAndProcessMoveBank(devObjPtr,msgWordPtr,&naProcessed);
    }
    else
    {
        naProcessed = GT_FALSE;
    }

    if(naProcessed == GT_FALSE)
    {
        status = parseAuMsgGetFdbEntryIndex(devObjPtr,msgWordPtr,&fdbIndex, &entryOffset);
        if (status != GT_FAIL)
        {
            /* Write NA to FDB table */
            sfdbChtWriteNaMsg(devObjPtr, msgWordPtr, fdbIndex,entryOffset);
        }
    }

    if (status == GT_FAIL)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* Na Not Learned */
            causBitMap = SMEM_CHT_NA_NOT_LEARN_INT;
            __LOG(("Interrupt: AU_PROC_COMPLETED and NA_NOT_LEARN \n"));
        }
        else
        {
        /* Na Not Learned | Dropped */
        causBitMap = SMEM_CHT_NA_NOT_LEARN_INT | SMEM_CHT_NA_DROPPED_INT;
        __LOG(("Interrupt: AU_PROC_COMPLETED and NA_NOT_LEARN and NA_DROPPED \n"));
    }

    }
    else
    {
        /* A new source MAC Address received was retained */
        causBitMap = SMEM_CHT_NA_LEARN_INT;

        __LOG(("Interrupt: AU_PROC_COMPLETED and NA_LEARN \n"));
    }


    /* AU completed */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        causBitMap |= SMEM_LION3_AU_PROC_COMPLETED_INT ;
    }
    else
    {
    causBitMap |= SMEM_CHT_AU_PROC_COMPLETED_INT;
    }

    /* A new source MAC Address received was retained */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                          SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                          causBitMap,
                          SMEM_CHT_FDB_SUM_INT(devObjPtr));
    return status;
}

/*******************************************************************************
*   sfdbLion3FdbSpecialMuxedFieldsGet
*
* DESCRIPTION:
*       Get Muxed fields from the FDB that depend on :
*       1. vid1_assignment_mode
*       2. src_id_length_in_fdb
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to FDB message.
*       fdbIndex     - fdb index
* OUTPUTS:
*       specialFieldsPtr  - (pointer to) the special fields value from the entry
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID sfdbLion3FdbSpecialMuxedFieldsGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * fdbEntryPtr,
    IN GT_U32                fdbIndex,
    OUT SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC *specialFieldsPtr
)
{
    GT_U32              srcId;
    GT_U32              udb;
    GT_U32              origVid1;
    GT_U32              daAccessLevel;
    GT_U32              saAccessLevel;
    GT_U32              value;
    GT_U32              srcIdLengthInFDB;/* 0- The SrcID filed in FDB is 12b
                                            1- The SrcID field in FDB table is 9b.
                                            SrcID[11:9] are used for extending the user defined bits */
    GT_U32              vid1AssignmentMode;/* 0 - <OrigVID1> is not written in the FDB and is not read from the FDB.
                                                    <SrcID>[8:6] can be used for src-id filtering and
                                                    <SA Security Level> and <DA Security Level> are written/read from the FDB.
                                              1- <OrigVID1> is written in the FDB and read from the FDB as described in
                                                    Section N:1 Mac Based VLAN .
                                                    <SrcID>[8:6], <SA Security Level> and <DA Security Level>
                                                    are read as 0 from the FDB entry.*/
    GT_U32              regValue;

    smemRegGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,&regValue);

    srcIdLengthInFDB   = SMEM_U32_GET_FIELD(regValue,10,1);
    vid1AssignmentMode = SMEM_U32_GET_FIELD(regValue,27,1);

    value =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            fdbIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_5_0);
    srcId = value;

    if(vid1AssignmentMode == 0)
    {
        value =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_8_6);
        SMEM_U32_SET_FIELD(srcId,6,3,value);
    }

    value =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
            fdbIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED);
    udb = value;

    if(srcIdLengthInFDB == 0)
    {
        value =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_11_9);
        SMEM_U32_SET_FIELD(srcId,9,3,value);

        udb &= 0x1F;/* keep only bits 0..4 */
    }

    if(vid1AssignmentMode)
    {
        daAccessLevel = SMAIN_NOT_VALID_CNS;
        saAccessLevel = SMAIN_NOT_VALID_CNS;

        value =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1);
        origVid1 = value;
    }
    else
    {
        origVid1 = SMAIN_NOT_VALID_CNS;

        value =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ACCESS_LEVEL);
        daAccessLevel = value;

        value =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_ACCESS_LEVEL);
        saAccessLevel = value;
    }

    specialFieldsPtr->srcId = srcId;
    specialFieldsPtr->udb = udb;
    specialFieldsPtr->origVid1 = origVid1;
    specialFieldsPtr->daAccessLevel = daAccessLevel;
    specialFieldsPtr->saAccessLevel = saAccessLevel;

    return;
}

/*******************************************************************************
*   sfdbLion3FdbSpecialMuxedFieldsSet
*
* DESCRIPTION:
*       Set Muxed fields from the FDB that depend on :
*       1. vid1_assignment_mode
*       2. src_id_length_in_fdb
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to FDB message.
*       fdbIndex     - fdb index
*       macEntryType - entry type
*       specialFieldsPtr  - (pointer to) the special fields value from the entry
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID sfdbLion3FdbSpecialMuxedFieldsSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * fdbEntryPtr,
    IN GT_U32                fdbIndex,
    IN SNET_CHEETAH_FDB_ENTRY_ENT                macEntryType,
    IN SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC *specialFieldsPtr
)
{
    GT_U32              value;
    GT_U32              srcIdLengthInFDB;/* 0- The SrcID filed in FDB is 12b
                                            1- The SrcID field in FDB table is 9b.
                                            SrcID[11:9] are used for extending the user defined bits */
    GT_U32              vid1AssignmentMode;/* 0 - <OrigVID1> is not written in the FDB and is not read from the FDB.
                                                    <SrcID>[8:6] can be used for src-id filtering and
                                                    <SA Security Level> and <DA Security Level> are written/read from the FDB.
                                              1- <OrigVID1> is written in the FDB and read from the FDB as described in
                                                    Section N:1 Mac Based VLAN .
                                                    <SrcID>[8:6], <SA Security Level> and <DA Security Level>
                                                    are read as 0 from the FDB entry.*/
    GT_U32              regValue;

    smemRegGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,&regValue);

    srcIdLengthInFDB   = SMEM_U32_GET_FIELD(regValue,10,1);
    vid1AssignmentMode = SMEM_U32_GET_FIELD(regValue,27,1);

    if(macEntryType == SNET_CHEETAH_FDB_ENTRY_MAC_E) /* valid when MACEntryType = "MAC" */
    {
        value = specialFieldsPtr->srcId;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
            fdbIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_5_0,
            value);

        if(vid1AssignmentMode == 0)
        {
            value = specialFieldsPtr->srcId >> 6;
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_8_6,
                value);
        }
    }

    /* NOTE: setting the 8 bits of UDB must come before setting of SOURCE_ID_11_9
        to allow SOURCE_ID_11_9 to override the 3 bits ! */
    value = specialFieldsPtr->udb;
    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED,
        value);

    if(macEntryType == SNET_CHEETAH_FDB_ENTRY_MAC_E) /* valid when MACEntryType = "MAC" */
    {
        if(srcIdLengthInFDB == 0)
        {
            value = specialFieldsPtr->srcId >> 9;
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SOURCE_ID_11_9,
                value);
        }
    }

    if(vid1AssignmentMode)
    {
        value = specialFieldsPtr->origVid1;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
            fdbIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_ORIG_VID1,
            value);
    }
    else
    {
        value = specialFieldsPtr->daAccessLevel;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
            fdbIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ACCESS_LEVEL,
            value);

        value = specialFieldsPtr->saAccessLevel;
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,fdbEntryPtr,
            fdbIndex,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_ACCESS_LEVEL,
            value);
    }

    return;
}

/*******************************************************************************
*   sfdbLion3FdbAuMsgSpecialMuxedFieldsGet
*
* DESCRIPTION:
*       Get Muxed fields from the FDB Au Msg that depend on :
*       1. vid1_assignment_mode
*       2. src_id_length_in_fdb
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       auMsgEntryPtr   - pointer to AU MSG message.
* OUTPUTS:
*       specialFieldsPtr  - (pointer to) the special fields value from the entry
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID sfdbLion3FdbAuMsgSpecialMuxedFieldsGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * auMsgEntryPtr,
    OUT SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC *specialFieldsPtr
)
{
    GT_U32              srcId;
    GT_U32              udb;
    GT_U32              origVid1;
    GT_U32              daAccessLevel;
    GT_U32              saAccessLevel;
    GT_U32              value;
    GT_U32              srcIdLengthInFDB;/* 0- The SrcID filed in FDB is 12b
                                            1- The SrcID field in FDB table is 9b.
                                            SrcID[11:9] are used for extending the user defined bits */
    GT_U32              vid1AssignmentMode;/* 0 - <OrigVID1> is not written in the FDB and is not read from the FDB.
                                                    <SrcID>[8:6] can be used for src-id filtering and
                                                    <SA Security Level> and <DA Security Level> are written/read from the FDB.
                                              1- <OrigVID1> is written in the FDB and read from the FDB as described in
                                                    Section N:1 Mac Based VLAN .
                                                    <SrcID>[8:6], <SA Security Level> and <DA Security Level>
                                                    are read as 0 from the FDB entry.*/
    GT_U32              regValue;

    smemRegGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,&regValue);

    srcIdLengthInFDB   = SMEM_U32_GET_FIELD(regValue,10,1);
    vid1AssignmentMode = SMEM_U32_GET_FIELD(regValue,27,1);

    value =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_5_0);
    srcId = value;

    if(vid1AssignmentMode == 0)
    {
        value =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_8_6);
        SMEM_U32_SET_FIELD(srcId,6,3,value);
    }

    value =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED);

    udb = value;

    if(srcIdLengthInFDB == 0)
    {
        value =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_11_9);
        SMEM_U32_SET_FIELD(srcId,9,3,value);

        udb &= 0x1F;/* keep only bits 0..4 */
    }

    if(vid1AssignmentMode)
    {
        daAccessLevel = SMAIN_NOT_VALID_CNS;
        saAccessLevel = SMAIN_NOT_VALID_CNS;

        value =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1);
        origVid1 = value;
    }
    else
    {
        origVid1 = SMAIN_NOT_VALID_CNS;

        value =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ACCESS_LEVEL);
        daAccessLevel = value;

        value =
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_ACCESS_LEVEL);
        saAccessLevel = value;
    }

    specialFieldsPtr->srcId = srcId;
    specialFieldsPtr->udb = udb;
    specialFieldsPtr->origVid1 = origVid1;
    specialFieldsPtr->daAccessLevel = daAccessLevel;
    specialFieldsPtr->saAccessLevel = saAccessLevel;

    return;
}

/*******************************************************************************
*   sfdbLion3FdbAuMsgSpecialMuxedFieldsSet
*
* DESCRIPTION:
*       Set Muxed fields from the FDB Au Msg that depend on :
*       1. vid1_assignment_mode
*       2. src_id_length_in_fdb
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       auMsgEntryPtr  - pointer to FDB AU MSG message.
*       macEntryType - entry type
*       specialFieldsPtr  - (pointer to) the special fields value from the entry
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID sfdbLion3FdbAuMsgSpecialMuxedFieldsSet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * auMsgEntryPtr,
    IN SNET_CHEETAH_FDB_ENTRY_ENT                macEntryType,
    IN SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC *specialFieldsPtr
)
{
    GT_U32              value;
    GT_U32              srcIdLengthInFDB;/* 0- The SrcID filed in FDB is 12b
                                            1- The SrcID field in FDB table is 9b.
                                            SrcID[11:9] are used for extending the user defined bits */
    GT_U32              vid1AssignmentMode;/* 0 - <OrigVID1> is not written in the FDB and is not read from the FDB.
                                                    <SrcID>[8:6] can be used for src-id filtering and
                                                    <SA Security Level> and <DA Security Level> are written/read from the FDB.
                                              1- <OrigVID1> is written in the FDB and read from the FDB as described in
                                                    Section N:1 Mac Based VLAN .
                                                    <SrcID>[8:6], <SA Security Level> and <DA Security Level>
                                                    are read as 0 from the FDB entry.*/
    GT_U32              regValue;

    smemRegGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr) ,&regValue);

    srcIdLengthInFDB   = SMEM_U32_GET_FIELD(regValue,10,1);
    vid1AssignmentMode = SMEM_U32_GET_FIELD(regValue,27,1);

    if(macEntryType == SNET_CHEETAH_FDB_ENTRY_MAC_E) /* valid when MACEntryType = "MAC" */
    {
        value = specialFieldsPtr->srcId;
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_5_0,
            value);

        if(vid1AssignmentMode == 0)
        {
            value = specialFieldsPtr->srcId >> 6;
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_8_6,
                value);
        }
    }

    /* NOTE: setting the 8 bits of UDB must come before setting of SOURCE_ID_11_9
        to allow SOURCE_ID_11_9 to override the 3 bits ! */
    value = specialFieldsPtr->udb;
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_USER_DEFINED,
        value);

    if(macEntryType == SNET_CHEETAH_FDB_ENTRY_MAC_E) /* valid when MACEntryType = "MAC" */
    {
        if(srcIdLengthInFDB == 0)
        {
            value = specialFieldsPtr->srcId >> 9;
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SOURCE_ID_11_9,
                value);
        }
    }

    if(vid1AssignmentMode)
    {
        value = specialFieldsPtr->origVid1;
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ORIG_VID1,
            value);
    }
    else
    {
        if(specialFieldsPtr->daAccessLevel != SMAIN_NOT_VALID_CNS)
        {
            value = specialFieldsPtr->daAccessLevel;
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ACCESS_LEVEL,
                value);
        }

        if(specialFieldsPtr->saAccessLevel != SMAIN_NOT_VALID_CNS)
        {
            value = specialFieldsPtr->saAccessLevel;
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,auMsgEntryPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_ACCESS_LEVEL,
                value);
        }
    }

    return;
}



/*******************************************************************************
*   lion3AuMsgConvertToFdbEntry
*
* DESCRIPTION:
*        Lion3 : Create FDB entry from AU message
*
* INPUTS:
*        devObjPtr          - pointer to device object.
*        fdbIndex           - index in the FDB
*        msgWordPtr         - MAC update message pointer
*
* OUTPUTS:
*        macEntry           - FDB entry pointer
*
*******************************************************************************/
static GT_STATUS lion3AuMsgConvertToFdbEntry
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U32                 fdbIndex,
    IN GT_U32                *msgWordPtr,
    OUT GT_U32               *macEntry
)
{
    DECLARE_FUNC_NAME(lion3AuMsgConvertToFdbEntry);

    GT_U32 isMulticastMac;/* bit 40 of the mac indicated multicast bit */
    GT_U8  macAddr[6];
    GT_U32 macAddrWords[2];
    GT_U32 fid = 0;
    GT_U32 fldValue;                    /* entry field value */
    GT_U32 useVidx;
    GT_U32 isTrunk;
    GT_U32 isTunnelStart;

    SNET_CHEETAH_FDB_ENTRY_ENT                macEntryType;
    SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC specialFields;


    /*************** common fields for all mac entries ********************/

    /* Entry Type */
    macEntryType =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE);

    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE,
        macEntryType);

    /* Valid */
    fldValue =SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                                                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VALID);

    if((fldValue==0) && /* delete entry */
       (devObjPtr->errata.fdbRouteUcDeleteByMsg) &&
       ((macEntryType==SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E)||
        (macEntryType==SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E)))
    {
        /* [JIRA]:[MT-231] [FE-2293984]
           CPU NA message for deleting an entry does not work for UC route entries */
        fldValue = 1;
        devObjPtr->fdbRouteUcDeletedEntryFlag = GT_TRUE;
    }
    else
    {
        devObjPtr->fdbRouteUcDeletedEntryFlag = GT_FALSE;
    }


    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID,
        fldValue);

    /* Skip */
    fldValue =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SKIP);
    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP,
        fldValue);

    /* Age */
    fldValue =
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE);
    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
        fldValue);

    /********************* mac and mc fields: common **********************************/
    switch(macEntryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:

            memset(&specialFields,0,sizeof(SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC));

            /* srcId          */
            /* udb            */
            /* origVid1       */
            /* daAccessLevel  */
            /* saAccessLevel  */
            sfdbLion3FdbAuMsgSpecialMuxedFieldsGet(devObjPtr,msgWordPtr,
                &specialFields);
            sfdbLion3FdbSpecialMuxedFieldsSet(devObjPtr,macEntry,fdbIndex,macEntryType,
                &specialFields);


            /* Vlan ID */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID,
                fldValue);
            fid = fldValue;

            /* Static */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_STATIC);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC,
                fldValue);

            /* DA_CMD */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_CMD);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_CMD,
                fldValue);

            /* Mirror To Analyzer Port */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER,
                fldValue);


            /*DA Lookup Ingress Mirror to Analyzer Enable*/
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_LOOKUP_INGRESS_MIRROR_TO_ANALYZER,
                fldValue);

            /* DA QoS Profile Index */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_QOS_PARAM_SET_IDX,
                fldValue);

            /* SA QoS Profile Index */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_QOS_PARAM_SET_IDX,
                fldValue);

            /* SPUnknown */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SP_UNKNOWN);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN,
                fldValue);

            /* Enable application specific CPU code for this entry */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE,
                fldValue);

            /* DA rote */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DA_ROUTE);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DA_ROUTE,
                fldValue);

            break;

    /********************* routing fields: common **********************************/
        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:
        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:

            /* vrf_id */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID,
                fldValue);


            /* dec ttl */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT,
                fldValue);

            /* bypass ttl */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION_FOR_IPV4_PACKETS,
                fldValue);

            /* Ingress Mirror to Analyzer Index */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX,
                fldValue);

            /* Qos profile marking en */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN,
                fldValue);

            /* Qos profile index */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX,
                fldValue);

            /* Qos profile precedence */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE,
                fldValue);

            /* Modify UP */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_UP);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_UP,
                fldValue);

            /* Modify DSCP */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP,
                fldValue);

            /* Counter Set index */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX,
                fldValue);

            /* Arp bc trap mirror en */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN,
                fldValue);

            /* Dip access level */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL,
                fldValue);

            /* Ecmp */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ICMP_REDIRECT_EXCEP_MIRROR_EN);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ECMP_REDIRECT_EXCEPTION_MIRROR,
                fldValue);

            /* Mtu Index */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MTU_INDEX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MTU_INDEX,
                fldValue);

            /* Use Vidx*/
            useVidx =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX,
                useVidx);

            if(0 == useVidx)
            {
                /* Is trunk*/
                isTrunk =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK,
                    fldValue);

                if(isTrunk)
                {
                    /* Trunk num */
                    fldValue =
                        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TRUNK_NUM);
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID,
                        fldValue);
                }
                else /* not trunk */
                {
                    /* Eport num */
                    fldValue =
                        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM);
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT,
                        fldValue);

                    /* Target Device */
                    fldValue =
                        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE);
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV,
                        fldValue);
                }
            }
            else /* useVidx true */
            {
                /* Evidx */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EVIDX);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_EVIDX,
                    fldValue);
            }

            /* Next Hop Evlan */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN,
                fldValue);

            /* Tunnel Start */
            isTunnelStart =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL,
                isTunnelStart);

            if(isTunnelStart)
            {
                /* Tunnel Type */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE,
                    fldValue);

                /* Tunnel Ptr */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR,
                    fldValue);
            }
            else /* not tunnel */
            {
                /* Arp Ptr */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_PTR);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_PTR,
                    fldValue);
            }

            break;
        default:
            break;
    }


    /************** specific fields (depends on mac entry type) *********************/

    switch(macEntryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:

            /* 48-bit MAC address */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_MAC_ADDR_GET(devObjPtr,msgWordPtr,
                macAddrWords);

            /* set the MAC ADDR words */
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_MAC_ADDR_SET(devObjPtr,
                macEntry,
                fdbIndex,
                macAddrWords);

            macAddr[5] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 0) ,8);
            macAddr[4] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 1) ,8);
            macAddr[3] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 2) ,8);
            macAddr[2] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 3) ,8);
            macAddr[1] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 4) ,8);
            macAddr[0] = (GT_U8)snetFieldValueGet(macAddrWords,(8 * 5) ,8);

            isMulticastMac = (macAddr[0] & 1);/*bit 40 of the MAC address */

            /* SA_CMD */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SA_CMD);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SA_CMD,
                fldValue);

            /* Multiple */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MULTIPLE);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE,
                fldValue);

            if(fldValue || /* multiple*/
               isMulticastMac)/* multicast mac address
                                   last bit in the most significant byte of mac
                                   address (network order)*/
            {
                /* use vidx */

                /* vidx */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX,
                    fldValue);
            }
            else
            {
                /* Trunk */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_TRUNK);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK,
                    fldValue);
                if(fldValue)
                {
                    /* TrunkNum */
                    fldValue =
                        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_TRUNK_NUM);
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM,
                        fldValue);
                }
                else
                {
                    /* PortNum */
                    fldValue =
                        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_EPORT_NUM);
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM,
                        fldValue);
                }

                /* DevID */
                fldValue =
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DEV_ID);
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID,
                    fldValue);
            }

                __LOG(("NA processed into FDB macAddr[%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x] vid[0x%3.3x] index [%d]\n"
                          ,macAddr[0]
                          ,macAddr[1]
                          ,macAddr[2]
                          ,macAddr[3]
                          ,macAddr[4]
                          ,macAddr[5]
                          ,fid
                          ,fdbIndex
                          ));
            break;


        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:
            /* DIP */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DIP);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DIP,
                fldValue);

            /* SIP */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SIP);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SIP,
                fldValue);

            /* VIDX */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX,
                fldValue);
        break;



        /****************** routing specific fields ********************************/
        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:

            /* ipv4 dip */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP,
                fldValue);
            break;

        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:

            /* fcoe d_id */
            fldValue =
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_GET(devObjPtr,msgWordPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID);
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,macEntry,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID,
                fldValue);
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E:
            __LOG(("ipv6 'not supported' (the device not support 'by message' the ipv6 entries)"));
            return GT_FAIL;
        default:
            skernelFatalError("wrong entry type given\n");
            break;
    }

    return GT_OK;

}


/*******************************************************************************
*   lion3FdbChtWriteNaMsg
*
* DESCRIPTION:
*       Process New Address FDB update message and write to fdb table.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to FDB message.
*       fdbIndex     - fdb index
*       entryOffset  - FDB offset within the 'bucket'
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID lion3FdbChtWriteNaMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr,
    IN GT_U32                fdbIndex,
    IN GT_U32                entryOffset
)
{
    DECLARE_FUNC_NAME(lion3FdbChtWriteNaMsg);

    GT_U32 macEntry[SMEM_CHT_MAC_TABLE_WORDS];                 /* MAC entry */
    GT_U32  *preChangedEntryPtr;/* pointer to FDB entry before apply the new info */
    GT_U32  address;/* address of the FDB entry */
    SFDB_CHT_BANK_COUNTER_ACTION_ENT counterAction;

    memset(&macEntry, 0, sizeof(macEntry));

    address = SMEM_CHT_MAC_TBL_MEM(devObjPtr, fdbIndex);
    __LOG_PARAM(address);

    lion3AuMsgConvertToFdbEntry(devObjPtr, fdbIndex, msgWordPtr, macEntry);

    preChangedEntryPtr = smemMemGet(devObjPtr,address);

    /* auto calc the counterAction by comparing the 'old' entry with the 'new' entry*/
    sfdbChtBankCounterActionAutoCalc(devObjPtr,
                    fdbIndex, /*the old index - same as new index because we update the entry */
                        preChangedEntryPtr,/* the old entry */
                    fdbIndex, /* the new index - same as old index because we update the entry */
                        macEntry,          /* the new entry */
                        &counterAction,
                        SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E);

    /*counter action mode*/
    sfdbChtBankCounterAction(devObjPtr, fdbIndex , counterAction,
        SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E);

    /* Write MAC entry into fdb table */
    smemMemSet(devObjPtr, address, macEntry, SMEM_CHT_MAC_TABLE_WORDS);

    /* update <NAEntryOffset> */
    smemRegFldSet(devObjPtr, SMEM_CHT_MSG_FROM_CPU_REG(devObjPtr), 2,5, entryOffset);

}

/*******************************************************************************
*   sfdbChtWriteNaMsg
*
* DESCRIPTION:
*       Process New Address FDB update message and write to fdb table.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to FDB message.
*       fdbIndex     - fdb index
*       entryOffset  - FDB offset within the 'bucket'
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbChtWriteNaMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr,
    IN GT_U32                fdbIndex,
    IN GT_U32                entryOffset
)
{
    GT_U32 fldValue;                    /* entry field value */
    GT_U32 fldEntryType;
    GT_U32 macEntry[SMEM_CHT_MAC_TABLE_WORDS];                 /* MAC entry */
    GT_U32  address;/* address of the FDB entry */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        lion3FdbChtWriteNaMsg(devObjPtr,msgWordPtr,fdbIndex,entryOffset);
        return;
    }

    address = SMEM_CHT_MAC_TBL_MEM(devObjPtr, fdbIndex);

    memset(&macEntry, 0, sizeof(macEntry));

    /* Create MAC table X words entry */
    /* Valid */
    SMEM_U32_SET_FIELD(macEntry[0], 0, 1, 1);

    /* Skip */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 12, 1);
    SMEM_U32_SET_FIELD(macEntry[0], 1, 1, fldValue);

    /* Age */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 13, 1);
    SMEM_U32_SET_FIELD(macEntry[0], 2, 1, fldValue);

    /* Vlan ID */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 0, 12);
    SMEM_U32_SET_FIELD(macEntry[0], 5, 12, fldValue);

    /* Static */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 18, 1);
    SMEM_U32_SET_FIELD(macEntry[2], 25, 1, fldValue);

    /* DA_CMD */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 21, 3);
    SMEM_U32_SET_FIELD(macEntry[2], 27, 3, fldValue);

    /* Mirror To Analyzer Port */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 31, 1);
    SMEM_U32_SET_FIELD(macEntry[3], 9, 1, fldValue);

    /* DA QoS Profile Index */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 15, 3);
    SMEM_U32_SET_FIELD(macEntry[3], 6, 3, fldValue);

    /* SA QoS Profile Index */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 12, 3);
    SMEM_U32_SET_FIELD(macEntry[3], 3, 3, fldValue);

    /* SPUnknown */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 14, 1);
    SMEM_U32_SET_FIELD(macEntry[3], 2, 1, fldValue);

    /* Enable application specific CPU code for this entry */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 29, 1);
    SMEM_U32_SET_FIELD(macEntry[3], 10, 1, fldValue);

    /* DA rote */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 30, 1);
    SMEM_U32_SET_FIELD(macEntry[3], 1, 1, fldValue);

    /* Entry Type */
    fldEntryType = SMEM_U32_GET_FIELD(msgWordPtr[3], 19, 2);
    SMEM_U32_SET_FIELD(macEntry[0], 3, 2, fldEntryType);

    /* The MAC DA Access level for this entry */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[0], 1, 3);
    SMEM_U32_SET_FIELD(macEntry[3], 11, 3, fldValue);

    /* The MAC SA Access level for this entry */
    fldValue = SMEM_U32_GET_FIELD(msgWordPtr[0], 12, 3);
    SMEM_U32_SET_FIELD(macEntry[3], 14, 3, fldValue);

    if (fldEntryType == SNET_CHEETAH_FDB_ENTRY_MAC_E)
    {
        /* Bits 14:0 of the 48-bit MAC address */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[0], 16, 16);
        SMEM_U32_SET_FIELD(macEntry[0], 17, 15, fldValue);
        SMEM_U32_SET_FIELD(macEntry[1], 0, 1, fldValue >> 15);

        /* Bits 46:15 of the 48-bit MAC address */
        fldValue = msgWordPtr[1];
        SMEM_U32_SET_FIELD(macEntry[1], 1, 31, fldValue);
        SMEM_U32_SET_FIELD(macEntry[2], 0, 1, fldValue >> 31);

        /* SA_CMD */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 24, 3);
        SMEM_U32_SET_FIELD(macEntry[2], 30, 2, (fldValue & 0x3));
        SMEM_U32_SET_FIELD(macEntry[3], 0, 1, fldValue >> 2);

        /* Multiple */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 15, 1);
        SMEM_U32_SET_FIELD(macEntry[2], 26, 1, fldValue);

        if(fldValue || /* multiple*/
           SMEM_U32_GET_FIELD(msgWordPtr[1], 24, 1))/* multicast mac address
                                   last bit in the most significant byte of mac
                                   address (network order)*/
        {
            /* use vidx */

            /* vidx */
            fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 17, 12);
            SMEM_U32_SET_FIELD(macEntry[2], 13, 12, fldValue);
        }
        else
        {
            /* Trunk */
            fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 17, 1);
            SMEM_U32_SET_FIELD(macEntry[2], 13, 1, fldValue);
            /* PortNum/TrunkNum */
            fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 18, 7);
            SMEM_U32_SET_FIELD(macEntry[2], 14, 7, fldValue);
            /* User Defined */
            fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 25, 4);
            SMEM_U32_SET_FIELD(macEntry[2], 21, 4, fldValue);
        }

        /* DevID */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 7, 5);
        SMEM_U32_SET_FIELD(macEntry[2], 1, 5, fldValue);

        /* SrcID */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 2, 5);
        SMEM_U32_SET_FIELD(macEntry[2], 6, 5, fldValue);
    }
    else
    {
        /* DIP[15:0] */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[0], 16, 16);
        /* DIP[14:0] */
        SMEM_U32_SET_FIELD(macEntry[0], 17, 15, fldValue);
        /* DIP[15] */
        SMEM_U32_SET_FIELD(macEntry[1], 0, 1, fldValue >> 15);

        /* DIP[31:16] */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[1], 0, 16);
        /* DIP[31:16] */
        SMEM_U32_SET_FIELD(macEntry[1], 1, 16, fldValue);

        /* SIP[15:0] */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[1], 16, 16);
        /* SIP[14:0] */
        SMEM_U32_SET_FIELD(macEntry[1], 17, 15, fldValue);
        /* SIP[15] */
        SMEM_U32_SET_FIELD(macEntry[2], 0, 1, fldValue >> 15);

        /* SIP[27:16] */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 0, 12);
        /* SIP[27:16] */
        SMEM_U32_SET_FIELD(macEntry[2], 1, 12, fldValue);

        /* SIP[31:28] */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[3], 27, 4);
        /* SIP[28] */
        SMEM_U32_SET_FIELD(macEntry[2], 26, 1, fldValue);
        /* SIP[30:29] */
        SMEM_U32_SET_FIELD(macEntry[2], 30, 2, fldValue >> 1);
        /* SIP[31] */
        SMEM_U32_SET_FIELD(macEntry[3], 0, 1, fldValue >> 3);

        /* VIDX */
        fldValue = SMEM_U32_GET_FIELD(msgWordPtr[2], 17, 12);
        SMEM_U32_SET_FIELD(macEntry[2], 13, 12, fldValue);
    }

        /* Write MAC entry into fdb table */
        smemMemSet(devObjPtr, address, macEntry, 4);
    }

/*******************************************************************************
*   sfdbChtMacTableTriggerAction
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
GT_VOID sfdbChtMacTableTriggerAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * tblActPtr
)
{
    GT_U32 * tblActWordPtr;         /* 32 bit data pointer */
    GT_U32  *memPtr;/*pointer to memory*/
    CHT_AUQ_MEM  *  simAuqMem;              /* pointer to AU queue */
    GT_U32          auqSize; /* size of AUQ */
    GT_U32      regAddr;/*register address*/
    GT_U32      ActEn;/*Enables FDB Actions*/
    GT_U32      TriggerMode;
    GT_U32      ActionMode;
    GT_U32      fdbCfgRegAddr;/* FDB Global Configuration register address*/
    GT_U32      auqInMemory;  /* 1 - used PCI accessed memory, 0 - used on Chip queue */

    tblActWordPtr = (GT_U32 *)tblActPtr;

    ActEn = SMEM_U32_GET_FIELD(tblActWordPtr[0], 0, 1);
    if(ActEn == 0)
    {
        /* action disabled by the application */
        return;
    }

    /* Trigger Mode */
    TriggerMode = SMEM_U32_GET_FIELD(tblActWordPtr[0], 2, 1);
    /* Action Mode */
    ActionMode  = SMEM_U32_GET_FIELD(tblActWordPtr[0], 3, 2);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION3_FDB_FDB_ACTION_GENERAL_REG(devObjPtr);
    }
    else
    {
        regAddr = SMEM_CHT_MAC_TBL_ACTION0_REG(devObjPtr);
    }

    fdbCfgRegAddr = SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr);
    smemRegFldGet(devObjPtr, fdbCfgRegAddr, 20, 1, &auqInMemory);

    if ((devObjPtr->isPciCompatible) && (auqInMemory != 0))/* use DMA memory */
    {
        simAuqMem = SMEM_CHT_MAC_AUQ_MEM_GET(devObjPtr);
        auqSize = simAuqMem->auqBaseSize;
        /* don't start the action while the AUQ is full --> 'erraum' */
        if((simAuqMem->auqBaseValid == GT_FALSE || simAuqMem->auqOffset == auqSize)&&
           simAuqMem->auqShadowValid == GT_FALSE)
        {
            memPtr = smemMemGet(devObjPtr, regAddr);

            /* do some waiting so no storming of messages */
            SIM_OS_MAC(simOsSleep)(50);

            /* call function to send message to the 'smain' task again */
            smemChtActiveWriteFdbActionTrigger(devObjPtr,
                    regAddr,  /*address */
                    1,        /*memSize */
                    memPtr,   /*memPtr  */
                    0,        /*param   */
                    memPtr);  /*inMemPtr*/

            return;
        }
    }

    if (TriggerMode)
    {
        /* ActionMode */
        if (ActionMode == SFDB_CHEETAH_TRIGGER_TRANS_E)
        {
            sfdbChtTriggerTa(devObjPtr, tblActWordPtr);
        }
        else
        {
            sfdbChtDoAging(devObjPtr, tblActWordPtr,
                           ActionMode, devObjPtr->fdbAgingDaemonInfo.indexInFdb,
                           devObjPtr->fdbNumEntries - devObjPtr->fdbAgingDaemonInfo.indexInFdb);
        }
    }

    /* Clear Aging Trigger */
    smemRegFldSet(devObjPtr, regAddr, 1, 1, 0);
}

/* open this flag manually to check the behavior of the aging daemon */
/*#define SIMULATION_CHECK_AGING_TIME*/


#ifdef SIMULATION_CHECK_AGING_TIME

typedef GT_STATUS (*CPSS_OS_TIME_RT_FUNC)
(
    OUT GT_U32  *seconds,
    OUT GT_U32  *nanoSeconds
);
extern CPSS_OS_TIME_RT_FUNC        cpssOsTimeRT;

#endif /*SIMULATION_CHECK_AGING_TIME*/

/*******************************************************************************
*   sfdbChtMacTableAutomaticAging
*
* DESCRIPTION:
*       MAC Table Trigger Action
*
* INPUTS:
*       devObjPtr      - pointer to device object.
*       data_PTR       - pointer number of entries to age.
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
GT_VOID sfdbChtMacTableAutomaticAging
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN GT_U8                 * data_PTR
)
{
    GT_U32  fldValue;                /* field value of register */
    GT_U32  * tblActPtr;
    GT_U32  firstEntryIdx;
    GT_U32  numOfEntries;
    GT_U32  * msg_data_PTR = (GT_U32 *)data_PTR;
    GT_U32  regAddr;
    GT_U32  ActionMode;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(data_PTR);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION3_FDB_FDB_ACTION_GENERAL_REG(devObjPtr);
    }
    else
    {
        regAddr = SMEM_CHT_MAC_TBL_ACTION0_REG(devObjPtr);
    }

    /* get mac action register */
    tblActPtr = smemMemGet(devObjPtr, regAddr);

    /* Action Mode */
    ActionMode  = SMEM_U32_GET_FIELD(tblActPtr[0], 3, 2);

    /* action to perform (with or without removal )*/
    if (ActionMode == 0)
    {
      fldValue = SFDB_CHEETAH_AUTOMATIC_AGING_REMOVE_E;
    }
    else
    {
      fldValue = SFDB_CHEETAH_AUTOMATIC_AGING_NO_REMOVE_E;
    }
    /* do aging with or without remove. data_PTR contains the indexes to search*/
    memcpy(&firstEntryIdx , msg_data_PTR , sizeof(GT_U32) );
    msg_data_PTR++;
    memcpy(&numOfEntries , msg_data_PTR , sizeof(GT_U32) );

    sfdbChtDoAging(devObjPtr, tblActPtr, fldValue, firstEntryIdx, numOfEntries);
}

/*******************************************************************************
*   sfdbChtActionInfoGet
*
* DESCRIPTION:
*       MAC Table Triggering Action - transplant address
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - pointer table action data
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbChtActionInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * tblActPtr,
    OUT AGE_DAEMON_ACTION_INFO_STC      *actionInfoPtr
)
{
    GT_U32    portTrunkSize; /* size of portTrunk field in SIP5 and above registers */

    memset(actionInfoPtr,0,sizeof(AGE_DAEMON_ACTION_INFO_STC));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            portTrunkSize = 15;
        }
        else
        {
            portTrunkSize = 13;
        }
        actionInfoPtr->removeStaticOnNonExistDev = 0;/* NOT supported in SIP5 ,
                                                       since no 'device table' */

        actionInfoPtr->staticTransEn = /* Static Address Transplant Enable */
            SMEM_U32_GET_FIELD(tblActPtr[0], 11, 1);

        actionInfoPtr->staticDelEn = /* Enables deleting of static addresses */
            SMEM_U32_GET_FIELD(tblActPtr[0], 12, 2);

        actionInfoPtr->ageOutAllDevOnTrunk =
            SMEM_U32_GET_FIELD(tblActPtr[0], 14, 1);

        actionInfoPtr->ageOutAllDevOnNonTrunk =
            SMEM_U32_GET_FIELD(tblActPtr[0], 15, 1);

        actionInfoPtr->actVlan =   /* Action Active VLAN */
            SMEM_U32_GET_FIELD(tblActPtr[1], 0, 13);

        actionInfoPtr->actVlanMask = /* Action Active VLAN Mask */
            SMEM_U32_GET_FIELD(tblActPtr[1], 16, 13);

        actionInfoPtr->actPort = /* Action Active Port/Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[2], 0, portTrunkSize);

        actionInfoPtr->actIsTrunk = /* Action Active Trunk bit */
            SMEM_U32_GET_FIELD(tblActPtr[2], portTrunkSize, 1);

        actionInfoPtr->actDevice = /* Action Active Device */
            SMEM_U32_GET_FIELD(tblActPtr[2], (portTrunkSize + 1), 10);

        actionInfoPtr->actPortMask = /* Action Active Port/Trunk Mask */
            SMEM_U32_GET_FIELD(tblActPtr[3], 0, portTrunkSize);

        actionInfoPtr->actIsTrunkMask = /* ActTrunkMask */
            SMEM_U32_GET_FIELD(tblActPtr[3], portTrunkSize, 1);

        actionInfoPtr->actDeviceMask = /* Action Active Device Mask */
            SMEM_U32_GET_FIELD(tblActPtr[3], (portTrunkSize + 1), 10);

        actionInfoPtr->actUserDefined =
            SMEM_U32_GET_FIELD(tblActPtr[4], 0, 8);

        actionInfoPtr->actUserDefinedMask =
            SMEM_U32_GET_FIELD(tblActPtr[4], 8, 8);

        SIM_TBD_BOOKMARK /* need to use those fields */
        actionInfoPtr->actVID1 =   /* Action Active VID1 */
            SMEM_U32_GET_FIELD(tblActPtr[5], 0, 12);

        actionInfoPtr->actVID1Mask = /* Action Active VID1 Mask */
            SMEM_U32_GET_FIELD(tblActPtr[5], 12, 12);

        actionInfoPtr->oldPort = /* Old Port/Old Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[6], 0, portTrunkSize);

        actionInfoPtr->oldIsTrunk = /* Old Is Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[6], portTrunkSize, 1);

        actionInfoPtr->oldDev = /* Old Device */
            SMEM_U32_GET_FIELD(tblActPtr[6], (portTrunkSize + 1), 10);

        actionInfoPtr->newPort = /* new Port/Old Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[7], 0, portTrunkSize);

        actionInfoPtr->newIsTrunk = /* new Is Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[7], portTrunkSize, 1);

        actionInfoPtr->newDev = /* new Device */
            SMEM_U32_GET_FIELD(tblActPtr[7], (portTrunkSize + 1), 10);

    }
    else
    {
        /* NOTE : the tblActPtr is pointer to memory in address 0x06000004 */
        /* we need data from registers :
           0x06000004 , 0x06000008 , 0x06000020

           so  tblActPtr[0] is 0x06000004
               tblActPtr[1] is 0x06000008
               tblActPtr[7] is 0x06000020
        */

        actionInfoPtr->staticDelEn = /* Enables deleting of static addresses */
            SMEM_U32_GET_FIELD(tblActPtr[0], 12, 1);

        actionInfoPtr->oldPort = /* Old Port/Old Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[0], 13, 7);

        actionInfoPtr->newPort = /* New Port/New Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[0], 25, 7);

        actionInfoPtr->oldDev = /* Old Device */
            SMEM_U32_GET_FIELD(tblActPtr[0], 20, 5);

        actionInfoPtr->newDev = /* New Device */
            SMEM_U32_GET_FIELD(tblActPtr[1], 0, 5);

        actionInfoPtr->oldIsTrunk = /* Old Is Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[1], 6, 1);

        actionInfoPtr->newIsTrunk = /* New Is Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[1], 5, 1);

        actionInfoPtr->actVlan =   /* Action Active VLAN */
            SMEM_U32_GET_FIELD(tblActPtr[1], 8, 12);

        actionInfoPtr->actVlanMask = /* Action Active VLAN Mask */
            SMEM_U32_GET_FIELD(tblActPtr[1], 20, 12);

        actionInfoPtr->staticTransEn = /* Static Address Transplant Enable */
            SMEM_U32_GET_FIELD(tblActPtr[0], 11, 1);

        actionInfoPtr->actIsTrunk = /* Action Active Trunk bit */
            SMEM_U32_GET_FIELD(tblActPtr[7], 14, 1);

        actionInfoPtr->actIsTrunkMask = /* ActTrunkMask */
            SMEM_U32_GET_FIELD(tblActPtr[7], 15, 1);

        actionInfoPtr->actPort = /* Action Active Port/Trunk */
            SMEM_U32_GET_FIELD(tblActPtr[7], 0, 7);

        actionInfoPtr->actPortMask = /* Action Active Port/Trunk Mask */
            SMEM_U32_GET_FIELD(tblActPtr[7], 7, 7);

        actionInfoPtr->ageOutAllDevOnTrunk =
            SMEM_U32_GET_FIELD(tblActPtr[7], 16, 1);

        actionInfoPtr->ageOutAllDevOnNonTrunk =
            SMEM_U32_GET_FIELD(tblActPtr[7], 17, 1);

        actionInfoPtr->actDevice = /* Action Active Device */
            SMEM_U32_GET_FIELD(tblActPtr[7], 18, 5);

        actionInfoPtr->actDeviceMask = /* Action Active Device Mask */
            SMEM_U32_GET_FIELD(tblActPtr[7], 23, 5);

        actionInfoPtr->removeStaticOnNonExistDev =
            SMEM_U32_GET_FIELD(tblActPtr[7], 28, 1);

        actionInfoPtr->deviceTableBmpPtr = smemMemGet(devObjPtr, SFDB_CHT_DEVICE_TBL_MEM(devObjPtr));
    }

    smemRegFldGet(devObjPtr, SMEM_CHT_GLB_CTRL_REG(devObjPtr), 4,
        devObjPtr->flexFieldNumBitsSupport.hwDevNum,
        &actionInfoPtr->ownDevNum);

    if(devObjPtr->supportMaskAuFuMessageToCpuOnNonLocal)
    {
        smemRegFldGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr) , 26 , 1 , &actionInfoPtr->maskAuFuMessageToCpuOnNonLocal);

        if(actionInfoPtr->maskAuFuMessageToCpuOnNonLocal &&
           devObjPtr->supportMaskAuFuMessageToCpuOnNonLocal_with3BitsCoreId)
        {
            smemRegFldGet(devObjPtr, SMEM_LION2_FDB_METAL_FIX_REG(devObjPtr) , 16 , 1 , &actionInfoPtr->maskAuFuMessageToCpuOnNonLocal_with3BitsCoreId);
        }
    }


    /* AA and TA message to CPU Enable */
    smemRegFldGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr), 19, 1, &actionInfoPtr->aaAndTaMessageToCpuEn);
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegFldGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr),      20, 1, &actionInfoPtr->routeUcAaAndTaMessageToCpuEn);
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr), 21, 1, &actionInfoPtr->routeUcAgingEn);
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr), 22, 1, &actionInfoPtr->routeUcTransplantingEn);
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr), 23, 1, &actionInfoPtr->routeUcDeletingEn);
    }

    /* <TrunksAgingMode> */
    smemRegFldGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr), 8, 1, &actionInfoPtr->trunksAgingMode);

    if(actionInfoPtr->trunksAgingMode == ForceAgeWithoutRemoval)
    {
        /* 1. force aging on trunk regardless to device number */
        actionInfoPtr->ageOutAllDevOnTrunk = 1;
    }

    /**/

    if(devObjPtr->supportEArch)
    {
        /*max Length Src-Id In Fdb Enable*/
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                      10, 1, &actionInfoPtr->srcIdLengthInFdbMode);

        /* get the <multi hash enable> bit*/
        actionInfoPtr->multiHashEnable = devObjPtr->multiHashEnable;

        /*McAddrDelMode*/
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_FDB_ACTION_GENERAL_REG(devObjPtr),
                      19, 1, &actionInfoPtr->McAddrDelMode);

        /*IPMCAddrDelMode*/
        smemRegFldGet(devObjPtr, SMEM_LION3_FDB_GLOBAL_CONFIG_1_REG(devObjPtr),
                      5, 1, &actionInfoPtr->IPMCAddrDelMode);
    }
    else
    {
        /*McAddrDelMode*/
        smemRegFldGet(devObjPtr, SMEM_CHT_MAC_TBL_ACTION2_REG(devObjPtr),
            31, 1, &actionInfoPtr->McAddrDelMode);


        /* this device NOT support to aging/delete of IPMC entry ! */
        actionInfoPtr->IPMCAddrDelMode = 1;
    }

}

/*******************************************************************************
*   sfdbChtFdbActionEntry_invalidate
*
* DESCRIPTION:
*       MAC Table Auto/triggered Action - invalidate the entry
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       entryPtr    - pointer to the FDB entry
*       actionInfoPtr   - (pointer to)common info need for the 'Aging daemon'
*       index    - index in the FDB
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbChtFdbActionEntry_invalidate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 *             entryPtr,
    IN AGE_DAEMON_ACTION_INFO_STC *actionInfoPtr,
    IN GT_U32 index
)
{
    /* 'delete' the entry :
    the SIP5 device with <multiHashEnable> = 1 uses the valid bit --> set to 0
    other devices uses that skip bit --> set to 1.
    */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(actionInfoPtr->multiHashEnable)/*relevant to SIP5*/
        {
            /* the skip bit ignored ... use the valid bit */
            /*valid bit*/
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
                index,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID,
                0);

            /* decrement the bank counter of the entry due to 'removed' entry */
            sfdbChtBankCounterAction(devObjPtr,index,
                SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E,
                SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_PP_AGING_DAEMON_E);
        }
        else
        {
            /*skip bit*/
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
                index,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP,
                1);
        }
    }
    else
    {
        /* skip bit */
        SMEM_U32_SET_FIELD(entryPtr[0], 1, 1, 1);
    }
}



/*******************************************************************************
*   sfdbChtFdbAaAndTaMsgToCpu
*
* DESCRIPTION:
*       FDB: send AA and TA message to cpu
*
* INPUTS:
*       devObjPtr      - pointer to device object.
*       action         - remove or not the aged out entries
*       actionInfoPtr  - (pointer to)common info need for the 'Aging daemon'
*       index          - index in the FDB
*       macEntryType   - type of fdb entry
*       entryPtr       - MAC table entry pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*******************************************************************************/
static GT_BOOL sfdbChtFdbAaAndTaMsgToCpu
(
    IN SKERNEL_DEVICE_OBJECT      *devObjPtr,
    IN SFDB_CHEETAH_ACTION_ENT     action,
    IN AGE_DAEMON_ACTION_INFO_STC *actionInfoPtr,
    IN GT_U32                      index,
    IN SNET_CHEETAH_FDB_ENTRY_ENT  macEntryType,
    IN GT_U32                     *entryPtr
)
{
    GT_U32  macUpdMsg[SMEM_CHT_AU_MSG_WORDS] = {0};  /* MAC update message */
    GT_U32  fldValue  = 0;                           /* register's field value */
    GT_BOOL status    = GT_FALSE;                    /* return status */
    GT_U32  isEnabled = 0;                           /* is feature enabled */
    GT_U32  isRouting = 0;                           /* routing indication */


    switch(macEntryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:

            isEnabled = actionInfoPtr->aaAndTaMessageToCpuEn;
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:
        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E:

            isRouting = 1; /* routing indication */
            isEnabled = actionInfoPtr->routeUcAaAndTaMessageToCpuEn;
            break;

        default:
            skernelFatalError("should never happen\n");
            break;
    }

    if(0 == isEnabled)
    {
        return GT_TRUE;
    }

    /* the UPload uses the AA/TA messaging mechanism so I assume use this bit too */

    /* Create update message    */
    sfdbChtFdb2AuMsg(devObjPtr, index, entryPtr, &macUpdMsg[0]);

    /* Message Type */
    if(action == SFDB_CHEETAH_TRIGGER_TRANS_E)
    {
        fldValue = SFDB_UPDATE_MSG_TA_E;
    }
    else
    if(action == SFDB_CHEETAH_TRIGGER_UPLOAD_E)
    {
        fldValue = SFDB_UPDATE_MSG_FU_E;
    }
    else
    {
       fldValue = SFDB_UPDATE_MSG_AA_E;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Message Type */
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE,
            fldValue);

        /* Entry Index */
        if(isRouting)
        {
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MAC_ADDR_INDEX,
                index);
        }
        else
        {
            /* Entry Index 0..8 */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_8_0,
                index);
            /* Entry Index 9..21 */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_INDEX_20_9,
                index >> 9);
        }
    }
    else
    {
        /* Message Type */
        SMEM_U32_SET_FIELD(macUpdMsg[0], 4, 3, fldValue);

        /* Entry Index */
        SMEM_U32_SET_FIELD(macUpdMsg[0], 7, 8, (index & 0xFF));
        SMEM_U32_SET_FIELD(macUpdMsg[3], 27, 4,((index >> 8) & 0xF));
        SMEM_U32_SET_FIELD(macUpdMsg[3], 0, 2, ((index >> 12) & 0x3));
    }

    /* Send MAC update message to CPU */
    while(GT_FALSE == status)
    {
        if (action == SFDB_CHEETAH_TRIGGER_UPLOAD_E)
        {
            status = snetCht2L2iSendFuMsg2Cpu(devObjPtr, macUpdMsg);
        }
        else
        {
            status = snetChtL2iSendUpdMsg2Cpu(devObjPtr, macUpdMsg);
        }

        if(status == GT_FALSE)
        {
            if(oldWaitDuringSkernelTaskForAuqOrFua)
            {
                /* wait for SW to free buffers */
                SIM_OS_MAC(simOsSleep)(SNET_CHEETAH_NO_FDB_ACTION_BUFFERS_SLEEP_TIME);
                /* do not stack the sKernel task , re-send message to continue from this point */
            }
            else
            {
                /* state that the daemon is on hold */
                devObjPtr->fdbAgingDaemonInfo.daemonOnHold = 1;

                return GT_FALSE;
            }
        }
    }

    return GT_TRUE;
}


/*******************************************************************************
*   sfdbChtFdbActionEntry
*
* DESCRIPTION:
*       MAC Table Auto/triggered Action.
*
* INPUTS:
*       devObjPtr     - pointer to device object.
*       action        - remove or not the aged out entries
*       actionInfoPtr - (pointer to)common info need for the 'Aging daemon'
*       index         - index in the FDB
* OUTPUTS:
*       needBreakPtr - (pointer to) indication that the loop iteration must break
*                   and continue from current index , by new skernel message  .
*
* RETURNS:
*       none
*
* COMMENTS:
*
*
*******************************************************************************/
static void sfdbChtFdbActionEntry
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SFDB_CHEETAH_ACTION_ENT action,
    IN AGE_DAEMON_ACTION_INFO_STC *actionInfoPtr,
    IN GT_U32 index,
    OUT GT_BIT  *needBreakPtr
)
{
    GT_U32 regAddr;                     /* Register address */
    GT_U32 fldValue;                    /* Register's field value */
    GT_U32 * entryPtr;                  /* MAC table entry pointer */
    GT_U32  staticEntry = 0;            /* is current entry static, must be zero by default */
    GT_U32  devNumInEntry;/* the device number of the entry */
    GT_U32  useVidxInEntry;/* the entry is for vidx */
    GT_U32  portInEntry;   /* port number in the entry */
    GT_U32  isLocalDevPort;/*is entry of type 'port' (not vidx/trunk) with 'own dev num'
                            NOTE: this is not 'local port' in terms of 'multi-port groups'
                                  but in terms of 'local to this device'*/
    GT_U32  ageOnAllDevices;/*do age on all entries (trunk and non trunk) that
                                      registered on other devices*/
    GT_U32  udbInEntry;/*user define field in the entry*/
    GT_U32  entry_isTrunk;/* the field isTrunk in the entry */
    GT_U32  entry_portTrunkNum; /* the field portTrunkNum in the entry */
    GT_BIT  isPortValid;/* indication the info is about 'port' (not trunk and not vidx) */

    SNET_CHEETAH_FDB_ENTRY_ENT   macEntryType;
    GT_BIT  treatAsAgeWithoutRemoval;

    GT_U32  isUcRoutingEntry = 0; /* indicates fdb entry type is one of uc routing entries,
                                     must be zero by default */
    GT_U32  isUcIpv6KeyRoutingEntry = 0; /* is Ipv6 UC address entry */

    SMEM_LION3_FDB_FDB_TABLE_FIELDS entryField;
    GT_BIT  isAgeOrDelete = (SFDB_CHEETAH_TRIGGER_UPLOAD_E || SFDB_CHEETAH_TRIGGER_TRANS_E) ? 0  :1;
    GT_BOOL status;/* status of send message to the CPU */

    *needBreakPtr = 0;
    index =  index % devObjPtr->fdbNumEntries;

#ifdef SIMULATION_CHECK_AGING_TIME
    {
        static GT_U32      start_sec  = 0;
        static GT_U32      start_nsec = 0;
        GT_U32      end_sec  = 0;
        GT_U32      end_nsec = 0;
        GT_U32      diff_sec;
        GT_U32      diff_nsec;

        if(cpssOsTimeRT && (devObjPtr->portGroupId == 0))
        {
            if(index == 0)
            {
                cpssOsTimeRT(&start_sec, &start_nsec);
            }
            else if(index == (devObjPtr->fdbNumEntries - 1))
            {
                cpssOsTimeRT(&end_sec, &end_nsec);
                if(end_nsec < start_nsec)
                {
                    end_nsec += 1000000000;
                    end_sec  -= 1;
                }
                diff_sec  = end_sec  - start_sec;
                diff_nsec = end_nsec - start_nsec;

                simGeneralPrintf(" aging time  = %d sec + %d nano \n" , diff_sec , diff_nsec);
            }
        }
    }
#endif /*SIMULATION_CHECK_AGING_TIME*/

    /* Get entryPtr according to entry index */
    regAddr = SMEM_CHT_MAC_TBL_MEM(devObjPtr, index);
    entryPtr = smemMemGet(devObjPtr, regAddr);

    /* Valid */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        fldValue =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
            index,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);
    }
    else
    {
        fldValue = SMEM_U32_GET_FIELD(entryPtr[0], 0, 1);
    }

    if (fldValue == 0)
    {
        return;
    }

    if(actionInfoPtr->multiHashEnable == 0)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* Skip */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
                index,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP);
        }
        else
        {
            /* Skip */
            fldValue = SMEM_U32_GET_FIELD(entryPtr[0], 1, 1);
        }

        if (fldValue)
        {
            return;
        }
    }

    /* mac entry type */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        macEntryType =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
            index,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE);
    }
    else
    {
        macEntryType = SMEM_U32_GET_FIELD(entryPtr[0], 3, 2);
    }

    if (SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E == macEntryType ||
        SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E == macEntryType)
    {
        if (action == SFDB_CHEETAH_TRIGGER_UPLOAD_E)
        {
            status = sfdbChtFdbAaAndTaMsgToCpu(devObjPtr, action, actionInfoPtr, index, macEntryType, entryPtr);
            if(status == GT_FALSE)
            {
                *needBreakPtr = 1;
            }
            return ;
        }

        if (action == SFDB_CHEETAH_TRIGGER_TRANS_E)
        {
            /* the IPMC entries are NOT subject transplant */
            return;
        }

        if(actionInfoPtr->IPMCAddrDelMode == 1)
        {
            /* the ipmc entries are NOT subject to next operations */
            return;
        }

        /* NOTE: non sip5 device can not reach here ! */

        staticEntry =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
            index,
            SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC);

        if(action != SFDB_CHEETAH_TRIGGER_DELETE_E)
        {
            if(staticEntry)
            {
                /* aging is not subject on static entries */
                return;
            }

            /* age  bit */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
                    index,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE);
            if (fldValue)
            {
                /* entry is not aged out.
                   reset age bit and continue with next entry. */
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
                    index,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
                    0);

                return ;
            }
        }
        else /*SFDB_CHEETAH_TRIGGER_DELETE_E*/
        {
            if (actionInfoPtr->staticDelEn == 0 && staticEntry)
            {
                /* Skip entries when delete static entries disable */
                return;
            }
        }

        /* Triggered address deleting */
        sfdbChtFdbActionEntry_invalidate(devObjPtr,entryPtr,actionInfoPtr,index);

        /* Write aged/deleted entry */
        smemMemSet(devObjPtr, regAddr, entryPtr, SMEM_CHT_MAC_TABLE_WORDS);
        status = sfdbChtFdbAaAndTaMsgToCpu(devObjPtr, action, actionInfoPtr, index, macEntryType, entryPtr);
        if(status == GT_FALSE)
        {
            *needBreakPtr = 1;
        }
    }
    else if (SNET_CHEETAH_FDB_ENTRY_MAC_E != macEntryType)
    {
        isUcRoutingEntry = 1;

        if (SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E == macEntryType)
        {
            isUcIpv6KeyRoutingEntry = 1;
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX :
                                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE ;
        useVidxInEntry =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr, index, entryField);

        if(0 == isUcRoutingEntry) /* The Static bit is not supported for FDB-based host route entries */
        {
            staticEntry =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
                index,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC);
        }

        entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV :
                                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID ;
        devNumInEntry =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr, index, entryField);
    }
    else
    {
        useVidxInEntry = SMEM_U32_GET_FIELD(entryPtr[2],26,1);
        staticEntry = SMEM_U32_GET_FIELD(entryPtr[2], 25, 1);
        devNumInEntry = SMEM_U32_GET_FIELD(entryPtr[2], 1, 5);

        /* check that the device in the entry exists */
        /* NOTE : even multicast entries are checked */
        if(actionInfoPtr->ownDevNum != devNumInEntry && /* not own dev num */
            /*Indexing to it is performed using the lower 5 bits of descriptor<DevNum>*/
          (snetFieldValueGet(actionInfoPtr->deviceTableBmpPtr,(devNumInEntry & 0x1f),1) == 0))
        {
            if(actionInfoPtr->removeStaticOnNonExistDev || staticEntry == 0)
            {
                /* delete the entry */
                    sfdbChtFdbActionEntry_invalidate(devObjPtr,entryPtr,actionInfoPtr,index);

                /* DO NOT SEND AA message to CPU !!! */
                return;
            }
        }
    }

    if(0 == isUcRoutingEntry) /* fid filtering is not relevant for fdb routing */
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* FID */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
                index,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID);
        }
        else
        {
            /* Vlan ID */
            fldValue = SMEM_U32_GET_FIELD(entryPtr[0], 5, 12);
        }

        if ((fldValue & actionInfoPtr->actVlanMask) != actionInfoPtr->actVlan)
        {
            return;
        }
    }

    if(useVidxInEntry && actionInfoPtr->McAddrDelMode == 1 && isAgeOrDelete)
    {
        /* the MC entries are NOT subject to aging/delete */
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* trunk  */
        entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK :
                                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK ;
        entry_isTrunk =
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr, index, entryField);

        if(entry_isTrunk)/*port*/
        {
            /* trunk num */
            entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID :
                                            SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM ;
            entry_portTrunkNum =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr, index, entryField);
        }
        else
        {
            /* eport num */
            entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT :
                                            SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM ;
            entry_portTrunkNum =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr, index, entryField);
        }
    }
    else
    {
        /* Trunk */
        entry_isTrunk = SMEM_U32_GET_FIELD(entryPtr[2], 13, 1);
        /* Port Num/Trunk Num */
        entry_portTrunkNum = SMEM_U32_GET_FIELD(entryPtr[2], 14, 7);
    }

    if(actionInfoPtr->trunksAgingMode == ForceAgeWithoutRemoval &&
           (useVidxInEntry == 0 && entry_isTrunk == 1)/*trunk entry*/)
    {
        /* cq# 150852 */
        /* this entry associated with trunk , but trunk aging mode forced to
            'Age without removal'*/
        treatAsAgeWithoutRemoval = 1;
    }
    else
    {
        treatAsAgeWithoutRemoval = 0;
    }


    if(action == SFDB_CHEETAH_TRIGGER_TRANS_E)
    {
        if ((SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E == macEntryType)||
            (SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E == macEntryType))
        {
            /* Ipv6 Uc Data/Key entry is not subject to transplanting */
            return;
        }

        if(isUcRoutingEntry && 0 == actionInfoPtr->routeUcTransplantingEn)
        {
            /* uc routing entry: transplanting disabled */
            return;
        }

        /* trunk/port masking done only on unicast entries */
        if(useVidxInEntry == 1)
        {
            return;
        }

        if (actionInfoPtr->staticTransEn == 0 && staticEntry == 1)
        {
            /* Skip entries when transplant static entries disable */
            return;
        }
        /* DevID */
        if (devNumInEntry != actionInfoPtr->oldDev)
        {
            return;
        }

        /* Trunk */
        fldValue = entry_isTrunk;

        isLocalDevPort = 1 - fldValue;
        isPortValid = isLocalDevPort;
        if (fldValue != actionInfoPtr->oldIsTrunk)
        {
            return;
        }

        /* Port Num/Trunk Num */
        fldValue = entry_portTrunkNum;

        if (fldValue != actionInfoPtr->oldPort)
        {
            return;
        }

        if(actionInfoPtr->ownDevNum != devNumInEntry)
        {
            isLocalDevPort = 0;
        }

        /* Lookup entry is match */

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* New device */
            entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV :
                                            SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID ;
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
            index,
            entryField,
            actionInfoPtr->newDev);

            /* New Is Trunk */
            entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK :
                                            SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_TRUNK ;
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
            index,
            entryField,
            actionInfoPtr->newIsTrunk);

            if(actionInfoPtr->newIsTrunk)
            {
                /* New trunk */
                entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID :
                                                SMEM_LION3_FDB_FDB_TABLE_FIELDS_TRUNK_NUM ;
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
                index,
                entryField,
                actionInfoPtr->newPort);
            }
            else
            {
                /* New port */
                entryField = isUcRoutingEntry ? SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT :
                                                SMEM_LION3_FDB_FDB_TABLE_FIELDS_EPORT_NUM ;
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
                index,
                entryField,
                actionInfoPtr->newPort);
            }
        }
        else
        {
            /* New device */
            SMEM_U32_SET_FIELD(entryPtr[2], 1, 5, actionInfoPtr->newDev);
            /* New Is Trunk */
            SMEM_U32_SET_FIELD(entryPtr[2], 13, 1, actionInfoPtr->newIsTrunk);
            /* New port */
            SMEM_U32_SET_FIELD(entryPtr[2], 14, 7, actionInfoPtr->newPort);
        }
    }
    else
    {
        if(isUcRoutingEntry && 0 == actionInfoPtr->routeUcAgingEn)
        {
            if (action == SFDB_CHEETAH_TRIGGER_AGING_REMOVE_E    ||
                action == SFDB_CHEETAH_TRIGGER_AGING_NO_REMOVE_E ||
                action == SFDB_CHEETAH_AUTOMATIC_AGING_REMOVE_E  ||
                action == SFDB_CHEETAH_AUTOMATIC_AGING_NO_REMOVE_E)
            {
                /* uc routing entry: aging disabled */
                return;
            }
        }

        if (/*!(SMEM_CHT_IS_SIP5_10_GET(devObjPtr)) &&*/
            (SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E == macEntryType)&&
            (action != SFDB_CHEETAH_TRIGGER_DELETE_E))
        {
            /* Ipv6 Uc Data entry is not subject to aging */
            /* in SIP_5_10 do aging on Data entry and not on the Key */
            return;
        }

        /* for BobCat2 B0 and above
           In case of IPv6 key or data, in order for an entry to be subject to aging
           and delete all the following should be set to 0:
           actPortMask, actIsTrunkMask, actDeviceMask.  */
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr)&&
           ((SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E == macEntryType)||
            (SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E == macEntryType)))
        {
            if ((actionInfoPtr->actDeviceMask!=0)||
                (actionInfoPtr->actIsTrunkMask!=0)||
                (actionInfoPtr->actPortMask!=0))
            {
                /* the ipv6 entry is not subject to aging */
                return;
            }
        }

        if (action == SFDB_CHEETAH_TRIGGER_UPLOAD_E)
        {
            ageOnAllDevices = 1;
        }
        else
        {
            ageOnAllDevices = actionInfoPtr->ageOutAllDevOnNonTrunk;
        }

       isLocalDevPort = 0;
       isPortValid = 0;

       /* trunk/port masking done only on unicast entries */
       if(useVidxInEntry == 0)
       {
           /* Address associated  with trunk */
           fldValue = entry_isTrunk;

           isLocalDevPort = 1 - fldValue;

            if ((isUcIpv6KeyRoutingEntry==0) &&
                ((fldValue & actionInfoPtr->actIsTrunkMask) != actionInfoPtr->actIsTrunk))
            {
                return;
            }

            isPortValid = isLocalDevPort;

            if (action == SFDB_CHEETAH_TRIGGER_UPLOAD_E)
            {
                ageOnAllDevices = fldValue ?
                                  actionInfoPtr->ageOutAllDevOnTrunk :
                                  1;
            }
            else
            {
                ageOnAllDevices =  fldValue ?
                                   actionInfoPtr->ageOutAllDevOnTrunk :
                                   actionInfoPtr->ageOutAllDevOnNonTrunk;
            }

            /* Port Number/ Trunk Number */
            fldValue = entry_portTrunkNum;

            if ((isUcIpv6KeyRoutingEntry==0) &&
                ((fldValue & actionInfoPtr->actPortMask) != actionInfoPtr->actPort))
            {
                return;
            }
        }
        else /* 'vidx' entries */
        {
            if(devObjPtr->errata.fdbAgingDaemonVidxEntries)
            {
                /* aging daemon skip fdb entries of fdb<FDBEntryType> MAC with fdb<multiple> = 1 (vidx entries) ,
                   unless (<ActDevMask > & <ActDev>) = 0 (from action 1,2 registers ) */
                if ((isUcIpv6KeyRoutingEntry==0) &&
                    ((actionInfoPtr->actDeviceMask & actionInfoPtr->actDevice) != 0))
                {
                    /* don't age entries that are not registered on this device */
                    return;
                }
            }
        }

        if(actionInfoPtr->ownDevNum != devNumInEntry)
        {
            isLocalDevPort = 0;
        }

        /* Device number */
        if (ageOnAllDevices == 0)
        {
            if ((isUcIpv6KeyRoutingEntry==0) &&
                ((devNumInEntry & actionInfoPtr->actDeviceMask) != actionInfoPtr->actDevice))
            {
                /* don't age entries that are not registered on this device */
                return;
            }
        }

        if (isUcRoutingEntry)
        {
            /* UDB not relevant for uc routing: do nothing */
        }
        else if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* UDB  */
            udbInEntry =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
                index,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_USER_DEFINED);

            if(actionInfoPtr->srcIdLengthInFdbMode == 0)
            {
                /* the 3 MSB of the UDB used in srcId ! */
                udbInEntry &= 0x1F;
            }

            if ((udbInEntry  & actionInfoPtr->actUserDefinedMask) != actionInfoPtr->actUserDefined)
            {
                /* don't age entries that are not match on 'user defined' */
                return;
            }
        }
    }/*transplant*/


    /* NOTE that :
      1. multicast entries can be aged if not static entries !
      2. multicast entries are subject to "triggered delete" just like other
         entries.
    */

    /* Look up entry is match */
    if (action == SFDB_CHEETAH_TRIGGER_DELETE_E)
    {
        if(isUcRoutingEntry && 0 == actionInfoPtr->routeUcDeletingEn)
        {
            /* uc routing entry: deleting disabled */
            return;
        }

        if (actionInfoPtr->staticDelEn == 0 && staticEntry)
        {
            /* Skip entries when delete static entries disable */
            return;
        }
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if (actionInfoPtr->staticDelEn == 2 && !staticEntry)
            {
                /* Skip dynamic entries when static only delete is enabled */
                return;
            }
        }

        if(treatAsAgeWithoutRemoval == 0)
        {
            /* cq# 150852 */
            /* Triggered address deleting */
            sfdbChtFdbActionEntry_invalidate(devObjPtr,entryPtr,actionInfoPtr,index);
        }
    }
    else if (action != SFDB_CHEETAH_TRIGGER_TRANS_E &&
              action != SFDB_CHEETAH_TRIGGER_UPLOAD_E)
    {
        if (staticEntry)
        {
            /* don't age static entries */
            return;
        }

        /* Triggered address aging without removal of aged out entries */
        /* Triggered address aging with removal of aged out entries */
        /* Automatic address aging without removal of aged out entries */
        /* Automatic address aging with removal of aged out entries */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* age  bit */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,entryPtr,
                    index,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE);
            if (fldValue)
            {
                /* entry is not aged out.
                   reset age bit and continue with next entry. */
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_SET(devObjPtr,entryPtr,
                    index,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE,
                    0);

                return ;
            }

        }
        else
        {
            if (SMEM_U32_GET_FIELD(entryPtr[0], 2, 1))
            {
                /* entry is not aged out.
                   reset age bit and continue with next entry. */
                SMEM_U32_SET_FIELD(entryPtr[0], 2, 1, 0);
                return ;
            }
        }

        /* Triggered address aging with removal of aged out entries */
        /* Automatic address aging with removal of aged out entries */
        if ((action == SFDB_CHEETAH_AUTOMATIC_AGING_REMOVE_E) ||
            (action == SFDB_CHEETAH_TRIGGER_AGING_REMOVE_E))
        {
            if(isUcRoutingEntry && actionInfoPtr->routeUcAgingEn)
            {
                /* do not remove the entry, only age_without_removal
                   is supported in case of UC routing entries*/
            }
            else if(treatAsAgeWithoutRemoval == 0)
            {
                /* cq# 150852 */
                /* the entry is aged out - set the skip bit */
                sfdbChtFdbActionEntry_invalidate(devObjPtr,entryPtr,actionInfoPtr,index);
            }
        }
    }

    if (action != SFDB_CHEETAH_TRIGGER_UPLOAD_E)/* not update the table */
    {
        if(devObjPtr->supportEArch)
        {
            /* Write aged/transplanted entry */
            smemMemSet(devObjPtr, regAddr, entryPtr, SMEM_CHT_MAC_TABLE_WORDS);
        }
        else
        {
            /* Write aged/transplanted entry */
            smemMemSet(devObjPtr, regAddr, entryPtr, 4);
        }
    }

    if(devObjPtr->supportEArch == 0)
    {
        portInEntry = entry_portTrunkNum;

        if(isPortValid && /* indication that info is about 'port' */
           actionInfoPtr->maskAuFuMessageToCpuOnNonLocal_with3BitsCoreId)
        {
            if((actionInfoPtr->ownDevNum & 0xFFFFFFFE) == (devNumInEntry & 0xFFFFFFFE))
            {
                /* local port to 1 of my hemispheres */

                GT_U32   devLsBit = devNumInEntry & 1;
                GT_U32   entryPortGroupId = ((portInEntry >> 4) & 3) | (devLsBit << 2);

                if(entryPortGroupId != (devObjPtr->portGroupId & 7))
                {
                    /* checking all bits of the codeId */

                    /* AU/FU messages are filtered by HW .
                        port checked according to 3 bits comparison as follows: {device[0],port[5:4]} vs pipe_id[2:0]. */
                    return;
                }

            }
        }
        else
        if(devObjPtr->portGroupSharedDevObjPtr && actionInfoPtr->maskAuFuMessageToCpuOnNonLocal &&
           isLocalDevPort && (((portInEntry >> 4) & 3) != (devObjPtr->portGroupId & 3)))
        {
            /*When Enabled - AU/FU messages are filtered by HW if the MAC entry
               does NOT reside on the local port group, i.e. the entry port[5:4] != port group */
            return;
        }
    }

    status = sfdbChtFdbAaAndTaMsgToCpu(devObjPtr, action, actionInfoPtr, index, macEntryType, entryPtr);
    if(status == GT_FALSE)
    {
        *needBreakPtr = 1;
    }

}

/*******************************************************************************
*   sfdbChtTriggerTa
*
* DESCRIPTION:
*       MAC Table Triggering Action - transplant address
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - pointer table action data
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbChtTriggerTa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * tblActPtr
)
{
    DECLARE_FUNC_NAME(sfdbChtTriggerTa);

    GT_U32 entry;                       /* MAC table entry index */
    AGE_DAEMON_ACTION_INFO_STC actionInfo;/* common info need for the 'Aging daemon' */
    GT_BIT  needBreak = 0;
    /* get common action info */
    sfdbChtActionInfoGet(devObjPtr,tblActPtr,&actionInfo);

    entry = devObjPtr->fdbAgingDaemonInfo.indexInFdb;

    for (/**/; entry < devObjPtr->fdbNumEntries; entry++)
    {
        sfdbChtFdbActionEntry(devObjPtr,SFDB_CHEETAH_TRIGGER_TRANS_E,&actionInfo,entry,&needBreak);
        if(needBreak)
        {
            /* the action is broken and will be continued in new message */
            /* save the index */
            devObjPtr->needResendMessage = 1;
            devObjPtr->fdbAgingDaemonInfo.indexInFdb = entry;
            return;
        }
        /* state that the daemon is NOT on hold */
        devObjPtr->fdbAgingDaemonInfo.daemonOnHold = 0;
    }

    devObjPtr->fdbAgingDaemonInfo.indexInFdb = 0;

    __LOG(("Interrupt: AGE_VIA_TRIGGER_END \n"));

    /* Processing of an AU Message received by the device is completed */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                          SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                          SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                            SMEM_LION3_AGE_VIA_TRIGGER_END_INT :
                          SMEM_CHT_AGE_VIA_TRIGGER_END_INT,
                          SMEM_CHT_FDB_SUM_INT(devObjPtr));
}

/*******************************************************************************
*   sfdbChtDoAging
*
* DESCRIPTION:
*       MAC Table Triggering Action - delete address, aging without removal
        of aged out entries, triggered aging with removal of aged out entries.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - action mode being performed on the entries
*       action      - remove or not the aged out entries
*       firstEntryIdx    - index to the first enrty .
*       numOfEntries     - number of entries to be searched.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID sfdbChtDoAging
(
  IN SKERNEL_DEVICE_OBJECT * devObjPtr,
  IN GT_U32                * tblActPtr,
  IN GT_U32                  action,
  IN GT_U32                  firstEntryIdx,
  IN GT_U32                  numOfEntries
)
{
    DECLARE_FUNC_NAME(sfdbChtDoAging);

    GT_U32 entry;                       /* MAC table entry index */
    AGE_DAEMON_ACTION_INFO_STC actionInfo;/* common info need for the 'Aging daemon' */
    GT_BIT  needBreak = 0;

    /* get common action info */
    sfdbChtActionInfoGet(devObjPtr,tblActPtr,&actionInfo);

    firstEntryIdx = devObjPtr->fdbAgingDaemonInfo.indexInFdb;

    for (entry = firstEntryIdx;
         entry < firstEntryIdx + numOfEntries ;
         entry++)
    {
        sfdbChtFdbActionEntry(devObjPtr,action,&actionInfo,entry,&needBreak);
        if(needBreak)
        {
            /* the action is broken and will be continued in new message */
            /* save the index */
            devObjPtr->needResendMessage = 1;
            devObjPtr->fdbAgingDaemonInfo.indexInFdb = entry;
            return;
        }
        /* state that the daemon is NOT on hold */
        devObjPtr->fdbAgingDaemonInfo.daemonOnHold = 0;
    }

    devObjPtr->fdbAgingDaemonInfo.indexInFdb = entry % devObjPtr->fdbNumEntries;

    /* The aging cycle that was initiated by a CPU trigger has ended */
    if ( (action == SFDB_CHEETAH_TRIGGER_AGING_REMOVE_E ) ||
         (action == SFDB_CHEETAH_TRIGGER_AGING_NO_REMOVE_E) ||
         (action == SFDB_CHEETAH_TRIGGER_DELETE_E))
    {
        __LOG(("Interrupt: AGE_VIA_TRIGGER_END \n"));

        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                              SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                                SMEM_LION3_AGE_VIA_TRIGGER_END_INT :
                              SMEM_CHT_AGE_VIA_TRIGGER_END_INT,
                              SMEM_CHT_FDB_SUM_INT(devObjPtr));
    }
}

/*******************************************************************************
*   sfdbChtQxMsgProcess
*
* DESCRIPTION:
*       Query Address Entry Query Response.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       msgWordPtr  - pointer to fdb message.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS sfdbChtQxMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr
)
{
    DECLARE_FUNC_NAME(sfdbChtQxMsgProcess);

    GT_U32 fdbIndex;                         /* FDB entry index */
    GT_STATUS  status;                      /* function return status */
    GT_U32 fldValue;                        /* Register's field value */
    GT_U32 * entryPtr;                      /* MAC table entry pointer */
    GT_U32 macUpdMsg[SMEM_CHT_AU_MSG_WORDS];                    /* MAC update message */
    GT_U32 entryOffset;                     /* MAC address offset */
    GT_U32  fdbMsgId;                       /* FDB Message ID */

    status = parseAuMsgGetFdbEntryIndex(devObjPtr,msgWordPtr,&fdbIndex, &entryOffset);

    if (status == GT_OK)
    {
        /* entry found */

        /* Get FDB entry */
        entryPtr = smemMemGet(devObjPtr, SMEM_CHT_MAC_TBL_MEM(devObjPtr, fdbIndex));

        /* Create update message */
        sfdbChtFdb2AuMsg(devObjPtr, fdbIndex, entryPtr, &macUpdMsg[0]);
    }
    else
    {
        /* entry was not found , need to use the info from the original AU message */
        memcpy(&macUpdMsg[0],msgWordPtr,4*SMEM_CHT_AU_MSG_WORDS);

        /* so we take the mac,vlan (or IPs,vlan) */
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Message ID always 0x0 */
        fldValue =  0;
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MESSAGE_ID,
            fldValue);

        /* Message Type */
        fldValue = SFDB_UPDATE_MSG_QR_E;
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MSG_TYPE,
            fldValue);

        /* Entry was found */
        fldValue = (status == GT_OK) ? 1 : 0;
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_ENTRY_FOUND,
            fldValue);

        fldValue = entryOffset;
        /* MAC address offset */
        SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsg,
            SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MAC_ADDR_OFFSET,
            fldValue);
    }
    else
    {
        /* Message ID for Cht is 0x2 and for Cht2 is 0x0 (ignore DASecurity Level)*/
        fdbMsgId =  (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr)) ? 0 : 2;
        SMEM_U32_SET_FIELD(macUpdMsg[0], 0, 4, fdbMsgId);

        /* Message Type */
        fldValue = SFDB_UPDATE_MSG_QR_E;
        SMEM_U32_SET_FIELD(macUpdMsg[0], 4, 3, fldValue);

        /* Entry was found */
        fldValue = (status == GT_OK) ? 1 : 0;
        SMEM_U32_SET_FIELD(macUpdMsg[0], 15, 1, fldValue);

        fldValue = entryOffset;
        /* MAC address offset */
        SMEM_U32_SET_FIELD(macUpdMsg[0], 7, 5, fldValue);
    }

    /* Send MAC update message to CPU */
    while(GT_FALSE == snetChtL2iSendUpdMsg2Cpu(devObjPtr, &macUpdMsg[0]))
    {
        if(oldWaitDuringSkernelTaskForAuqOrFua)
        {
            /* wait for SW to free buffers */
            SIM_OS_MAC(simOsSleep)(SNET_CHEETAH_NO_FDB_ACTION_BUFFERS_SLEEP_TIME);
        }
        else
        {
            devObjPtr->needResendMessage = 1;
            return status;/* this status is not relevant because ,
                             caller will not use it */
        }
    }

    __LOG(("Interrupt: AU_PROC_COMPLETED \n"));

    /* Processing of an AU Message received by the device is completed */
    snetChetahDoInterrupt(devObjPtr,
                      SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                      SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                      SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                        SMEM_LION3_AU_PROC_COMPLETED_INT :
                      SMEM_CHT_AU_PROC_COMPLETED_INT,
                      SMEM_CHT_FDB_SUM_INT(devObjPtr));

    return status;
}

/*******************************************************************************
*   lion3FdbEntryConvertToAuMsg
*
* DESCRIPTION:
*        Lion3 : Create AU message from FDB entry
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        fdbIndex           - index inf the FDB
*        fdbEntryPtr        -  FDB entry pointer
*        macUpdMsgPtr       -  MAC update message
*******************************************************************************/
static GT_VOID lion3FdbEntryConvertToAuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  fdbIndex,
    IN GT_U32                * fdbEntryPtr,
    OUT GT_U32               * macUpdMsgPtr
)
{
    GT_U32                                  fldValue;        /* Word's field value */
    GT_U32                                  valueArr[4] = {0};
    SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC specialFields;
    GT_U32                                  macAddrWords[2];
    SNET_CHEETAH_FDB_ENTRY_ENT              macEntryType;

    ASSERT_PTR(fdbEntryPtr);
    ASSERT_PTR(macUpdMsgPtr);

    memset(macUpdMsgPtr, 0, 4 * SMEM_CHT_AU_MSG_WORDS);
    memset(&specialFields,0,sizeof(SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC));

    /* fdb entry type */
    macEntryType = SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_FDB_ENTRY_TYPE);

    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FDB_ENTRY_TYPE,
        macEntryType);


    switch(macEntryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_MAC_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV4_IPMC_BRIDGING_E:/*IPv4 Multicast address entry (IGMP snooping);*/
        case SNET_CHEETAH_FDB_ENTRY_IPV6_IPMC_BRIDGING_E:/*IPv6 Multicast address entry (MLD snooping);*/

            /* srcId          */
            /* udb            */
            /* origVid1       */
            /* daAccessLevel  */
            /* saAccessLevel  */
            sfdbLion3FdbSpecialMuxedFieldsGet(devObjPtr,fdbEntryPtr,fdbIndex,
                &specialFields);
            sfdbLion3FdbAuMsgSpecialMuxedFieldsSet(devObjPtr,macUpdMsgPtr,macEntryType,
                &specialFields);

            /* MAC ADDR */
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_MAC_ADDR_GET(devObjPtr,
                fdbEntryPtr,
                fdbIndex,
                macAddrWords);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_MAC_ADDR_SET(devObjPtr,
                macUpdMsgPtr,
                macAddrWords);

            /* VID */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_FID);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_FID,
                fldValue);

            /* Multiple */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_MULTIPLE);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MULTIPLE,
                fldValue);

            /* SPUnknown */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_SP_UNKNOWN);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SP_UNKNOWN,
                fldValue);

            /* VIDX/Port */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_VIDX);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VIDX,
                fldValue);

            /* add application specific code */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_APP_SPECIFIC_CPU_CODE,
                fldValue);

            /* DevNum - the device number associated with this message */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_DEV_ID);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_DEV_ID,
                fldValue);

            /* FDB Entry Static Bit */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_IS_STATIC);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_IS_STATIC,
                fldValue);

            break;

        /*****************************************************************/
        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:

            /* ipv4 dip */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV4_DIP);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV4_DIP,
                fldValue);
            break;

        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
            /* fcoe d_id */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_FCOE_D_ID,
                fldValue);
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_KEY_E:

            /* ipv6 dip */
            SMEM_LION3_FDB_FDB_ENTRY_FIELD_IPV6_DIP_GET(devObjPtr,
                fdbEntryPtr,
                fdbIndex,
                valueArr);

            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_0,
                valueArr[0]);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_1,
                valueArr[1]);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_2,
                valueArr[2]);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DIP_3,
                valueArr[3]);

            /* NH_DATA_BANK_NUM */
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NH_DATA_BANK_NUM,
                fdbIndex % devObjPtr->fdbNumOfBanks );
            break;

        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E:
            break;

        default:
            skernelFatalError("wrong entry type given\n");
            break;
    }

    /* MessageID Always set to 0x0 */
    fldValue = 0;
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_MESSAGE_ID,
        fldValue);

    /* Age */
    fldValue =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_AGE);
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_AGE,
        fldValue);

    /* Skip */
    fldValue =
        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
        fdbIndex,
        SMEM_LION3_FDB_FDB_TABLE_FIELDS_SKIP);
    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_SKIP,
        fldValue);


    /* FDB ipv4 / fcoe / ipv6 data  UC Routing common fields */
    switch(macEntryType)
    {
        case SNET_CHEETAH_FDB_ENTRY_IPV4_UC_ROUTING_E:
        case SNET_CHEETAH_FDB_ENTRY_FCOE_UC_ROUTING_E:
        case SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E:

            /* valid */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_VALID);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_VALID,
                fldValue);

            /* vrf_id */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_VRF_ID);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_VRF_ID,
                fldValue);

           if(macEntryType == SNET_CHEETAH_FDB_ENTRY_IPV6_UC_ROUTING_DATA_E)
            {
                /* ipv6 scope check */
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_SCOPE_CHECK,
                    fldValue);

                /* ipv6 dst site id */
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_IPV6_DEST_SITE_ID);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IPV6_DST_SITE_ID,
                    fldValue);
            }

            /* dec ttl */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DEC_TTL_OR_HOP_COUNT,
                fldValue);

            /* bypass ttl */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION_FOR_IPV4_PACKETS);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_BYPASS_TTL_OPTIONS_OR_HOP_EXTENSION,
                fldValue);

            /* Ingress Mirror to Analyzer Index */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_INGRESS_MIRROR_TO_ANALYZER_INDEX,
                fldValue);

            /* Qos profile marking en */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_MARKING_EN,
                fldValue);

            /* Qos profile index */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_INDEX,
                fldValue);

            /* Qos profile precedence */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_QOS_PROFILE_PRECEDENCE,
                fldValue);

            /* Modify UP */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_UP);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_UP,
                fldValue);

            /* Modify DSCP */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MODIFY_DSCP,
                fldValue);

            /* Counter Set index */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_COUNTER_SET_INDEX,
                fldValue);

            /* Arp bc trap mirror en */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_BC_TRAP_MIRROR_EN,
                fldValue);

            /* Dip access level */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_DIP_ACCESS_LEVEL,
                fldValue);

            /* Ecmp */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ECMP_REDIRECT_EXCEPTION_MIRROR);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ICMP_REDIRECT_EXCEP_MIRROR_EN,
                fldValue);

            /* Mtu Index */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_MTU_INDEX);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_MTU_INDEX,
                fldValue);

            /* Use Vidx*/
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_USE_VIDX);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_USE_VIDX,
                fldValue);

            if(0 == fldValue) /* useVidx = false*/
            {
                /* Is trunk*/
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_IS_TRUNK);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_IS_TRUNK,
                    fldValue);

                if(fldValue)/*IsTrunk=true*/
                {
                    /* Trunk num */
                    fldValue =
                        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_TRUNK_ID);
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TRUNK_NUM,
                        fldValue);
                }
                else /* not trunk */
                {
                    /* Target Device */
                    fldValue =
                        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_DEV);
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TARGET_DEVICE,
                        fldValue);

                    /* Eport num */
                    fldValue =
                        SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                        fdbIndex,
                        SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TRG_EPORT);
                    SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                        SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EPORT_NUM,
                        fldValue);
                }
            }
            else /* useVidx true */
            {
                /* Evidx */
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_EVIDX);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_EVIDX,
                    fldValue);
            }

            /* Next Hop Evlan */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_NEXT_HOP_EVLAN,
                fldValue);

            /* Tunnel Start */
            fldValue =
                SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                fdbIndex,
                SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_START_OF_TUNNEL);
            SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_START,
                fldValue);

            if(fldValue)/* tunnelStart=true */
            {
                /* Tunnel Type */
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_TYPE,
                    fldValue);

                /* Tunnel Ptr */
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_TUNNEL_PTR,
                    fldValue);
            }
            else /* not tunnel */
            {
                /* Arp Ptr */
                fldValue =
                    SMEM_LION3_FDB_FDB_ENTRY_FIELD_GET(devObjPtr,fdbEntryPtr,
                    fdbIndex,
                    SMEM_LION3_FDB_FDB_TABLE_FIELDS_UC_ROUTE_ARP_PTR);
                SMEM_LION3_FDB_AU_MSG_ENTRY_FIELD_SET(devObjPtr,macUpdMsgPtr,
                    SMEM_LION3_FDB_AU_MSG_TABLE_FIELDS_UC_ROUTE_ARP_PTR,
                    fldValue);
            }

            break;

        default:
            break;
    }

}


/*******************************************************************************
*   sfdbChtFdb2AuMsg
*
* DESCRIPTION:
*        Create AU message from FDB entry
* INPUTS:
*        devObjPtr          -  pointer to device object.
*        fdbIndex           - index inf the FDB
*        fdbEntryPtr        -  FDB entry pointer
*        macUpdMsgPtr       -  MAC update message
*******************************************************************************/
static GT_VOID sfdbChtFdb2AuMsg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  fdbIndex,
    IN GT_U32                * fdbEntryPtr,
    OUT GT_U32               * macUpdMsgPtr
)
{
    GT_U32 fldValue;                        /* Word's field value */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        lion3FdbEntryConvertToAuMsg(devObjPtr,fdbIndex,fdbEntryPtr,macUpdMsgPtr);

        return;
    }

    ASSERT_PTR(macUpdMsgPtr);


    memset(macUpdMsgPtr, 0, 4 * SMEM_CHT_AU_MSG_WORDS);

    if (SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {   /* MessageID Always set to 0x2 , on cht */
        SMEM_U32_SET_FIELD(macUpdMsgPtr[0], 0, 4, 2);
    }
    else
    {   /* MessageID Always set to 0x0 , on cht2 */
        SMEM_U32_SET_FIELD(macUpdMsgPtr[0], 0, 1, 0);
        /* DA security level */
        fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[3], 11, 3);
        SMEM_U32_SET_FIELD(macUpdMsgPtr[0], 1, 3, fldValue);
        /* SA security level */
        fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[3], 14, 3);
        SMEM_U32_SET_FIELD(macUpdMsgPtr[0], 12, 3, fldValue);
    }
    /* MacAddr[14:0] - the lower 16 bits of the mac addr*/
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[0], 17, 15);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[0], 16, 15, fldValue);
    /* MacAddr[15] - continue of the lower 16 bits lower */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[1], 0, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[0], 31, 1, fldValue);

    /* MacAddr[46:16] , the upper 32 bits of the mac addr */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[1], 1, 31);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[1], 0, 31, fldValue);
    /* MacAddr[47] , continue with the upper 32 bits of the mac addr */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 0, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[1], 31, 1, fldValue);

    /* VID */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[0], 5, 12);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 0, 12, fldValue);

    /* Multiple */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 26, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 15, 1, fldValue);

    /* SPUnknown */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[3], 2, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 14, 1, fldValue);

    /* Age */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[0], 2, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 13, 1, fldValue);

    /* Skip */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[0], 1, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 12, 1, fldValue);

    /* VIDX/Port */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 13, 12);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 17, 12, fldValue);
    if (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
    {   /* add application specific code */
        fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[3], 10, 1);
        SMEM_U32_SET_FIELD(macUpdMsgPtr[2], 29, 1, fldValue);
    }

    /* SrcID , the source id associated with this message */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 6, 5);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[3], 2, 5, fldValue);
    /* DevNum - the device number associated with this message */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 1, 5);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[3], 7, 5, fldValue);

    /* FDB Entry Static Bit */
    fldValue = SMEM_U32_GET_FIELD(fdbEntryPtr[2], 25, 1);
    SMEM_U32_SET_FIELD(macUpdMsgPtr[3], 18, 1, fldValue);

}

/* use value 0 to keep using the value from the register */
GT_U32  numOfEntries_debug = 256;
/* debug function */
GT_STATUS sfdbChtAutoAgingNumOfEntries_debug(GT_U32 new_numOfEntries_debug)
{
    numOfEntries_debug = new_numOfEntries_debug;

    return GT_OK;
}

/*******************************************************************************
*   sfdbChtAutoAging
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
void sfdbChtAutoAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32  regValue;                 /* register data */
    GT_U32  fldValue;                 /* field value of register */
    GT_U32  agingTime;                /* aging time in milliseconds */
    GT_U32  ActionTimer;              /* field <ActionTimer> in HW */
    GT_U32 entryIndex = 0 ;
    GT_U32 numOfEntries = 0;
    SBUF_BUF_ID bufferId;             /* buffer */
    GT_U32 * dataPtr;                /* pointer to the data in the buffer */
    GT_U32 dataSize;                  /* data size */
    GT_U32 timeOut;
    GT_U32 totalEntries;
    GT_U8  *dataU8Ptr;                   /* pointer to the data in the buffer */
    GT_U32  regAddr;

    GT_U32  coreClk;        /* the core clock of the device */
    GT_U32  fdbBaseCoreClock; /* FDB usage of the reference core clock for the device */
    GT_U32  fdbAgingFactor; /* ch3 - 16 sec , ch1,2,xcat,2,lion,2 = 10 sec, bobcat2,lion3 */
    GT_U32  FDBAgingWindowSizeRegAddr;/* FDB Aging Window Size Register Address */
    GT_U32  currentSleepTime = 0;

    /* Sleep to finish init of the simulation */
    /* (try to) create some kind of divert between the different port groups */
    deviceObjPtr = skernelSleep(deviceObjPtr,5000 + (deviceObjPtr->portGroupId * 111));

    if(SMEM_CHT_IS_SIP5_GET(deviceObjPtr))
    {
        regAddr = SMEM_LION3_FDB_FDB_ACTION_GENERAL_REG(deviceObjPtr);
        FDBAgingWindowSizeRegAddr = SMEM_LION3_FDB_AGING_WINDOW_SIZE_REG(deviceObjPtr);
    }
    else
    {
        regAddr = SMEM_CHT_MAC_TBL_ACTION0_REG(deviceObjPtr);
        FDBAgingWindowSizeRegAddr = 0;/* not used */
    }

    do
    {
        if(deviceObjPtr->portGroupSharedDevObjPtr)
        {
            coreClk = deviceObjPtr->portGroupSharedDevObjPtr->coreClk;
        }
        else
        {
            coreClk = deviceObjPtr->coreClk;
        }

        if(coreClk == 0)
        {
            /* device is not initialized yet -- try again later ... */
            deviceObjPtr = skernelSleep(deviceObjPtr,100);
        }
    }while(coreClk == 0); /* we wait while init part 2 is done ... we need fdbBaseCoreClock,fdbAgingFactor*/

    if(deviceObjPtr->portGroupSharedDevObjPtr)
    {
        fdbBaseCoreClock  = deviceObjPtr->portGroupSharedDevObjPtr->fdbBaseCoreClock;
        fdbAgingFactor    = deviceObjPtr->portGroupSharedDevObjPtr->fdbAgingFactor  ;
    }
    else
    {
        fdbBaseCoreClock  = deviceObjPtr->fdbBaseCoreClock;
        fdbAgingFactor    = deviceObjPtr->fdbAgingFactor  ;
    }

    while(1)
    {
        if(currentSleepTime)
        {
            /* the sleep function may replace the device object !!! due to soft reset */
            deviceObjPtr = skernelSleep(deviceObjPtr,currentSleepTime);

            /* set task type - only after SHOSTG_psos_reg_asic_task */
            /* support possible change of the device during 'soft reset' */
            SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(
                SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E,
                &deviceObjPtr->task_sagingCookieInfo.generic);
        }

        /* check that automatic aging enabled and active by read automatic field */
        smemRegGet(deviceObjPtr, regAddr, &regValue);
        /*ActEn */
        fldValue = SMEM_U32_GET_FIELD(regValue, 0, 1);
        if(fldValue == 0)
        {
            /* daemon is not active -- try again later ... */
            currentSleepTime = 1000;
            continue;
        }
        /*AgingTrigger */
        fldValue = SMEM_U32_GET_FIELD(regValue, 1, 1);
        if(fldValue)
        {
            /* daemon is BUSY -- try again later ... */
            currentSleepTime = 1000;
            continue;
        }
        /*TriggerMode */
        fldValue = SMEM_U32_GET_FIELD(regValue, 2, 1);
        if(fldValue)
        {
            /* trigger aging */
            currentSleepTime = 1000;
            /* automatic aging is currently disabled
                                   sleep for 2 seconds and then check again if
                                   automatic aging is enabled again */
            continue;
        }
        /*ActionMode */
        fldValue = SMEM_U32_GET_FIELD(regValue, 3, 2);

        /* if TriggerMode > 1 than automatic aging disabled */
        if (fldValue > 1)
        {
            currentSleepTime = 1000;
            /* automatic aging is currently disabled
                                   sleep for 2 seconds and then check again if
                                   automatic aging is enabled again */
            continue;
        }

        numOfEntries = numOfEntries_debug;

        if(deviceObjPtr->fdbAgingDaemonInfo.daemonOnHold)
        {
            currentSleepTime = 100;
            /* the daemon is on hold (AUQ is full) */
            continue;
        }

        /* Get buffer */
        bufferId = sbufAlloc(deviceObjPtr->bufPool, SFDB_AUTO_AGING_BUFF_SIZE);
        if (bufferId == NULL)
        {
            simWarningPrintf(" sfdbChtAutoAging: no buffers for automatic aging \n");
            currentSleepTime = 1000;
            /* currently no buffers for aging
                                   sleep for 2 seconds and then try again ,
                                   maybe there will be buffers free then */
            continue;
        }

        /* Get actual data pointer */
        sbufDataGet(bufferId, (GT_U8**)&dataU8Ptr, &dataSize);
        dataPtr = (GT_U32*)dataU8Ptr;


        /* copy MAC table action 1 word to buffer */
        memcpy(dataPtr, &entryIndex , sizeof(GT_U32) );
        dataPtr++;
        memcpy(dataPtr, &numOfEntries , sizeof(GT_U32) );

        /* set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* set message type of buffer */
        bufferId->dataType = SMAIN_CPU_FDB_AUTO_AGING_E;

        /* put buffer to queue */
        squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(bufferId));

        totalEntries =  entryIndex + numOfEntries;
        entryIndex = totalEntries % deviceObjPtr->fdbNumEntries ;
        /* get aging time in seconds * 10 */
        /* ActionTimer  */
        ActionTimer = SMEM_U32_GET_FIELD(regValue, 5, 6);
        if (ActionTimer == 0)
            ActionTimer++;

        if(FDBAgingWindowSizeRegAddr)
        {
            /*aging_end_counter_val*/
            smemRegGet(deviceObjPtr, FDBAgingWindowSizeRegAddr, &regValue);

            if(SMEM_CHT_IS_SIP5_10_GET(deviceObjPtr))
            {
                fdbAgingFactor = _512K;
            }
            else
            {
                fdbAgingFactor = (_256K / POWER_2(_256K/deviceObjPtr->fdbNumEntries));
            }

            /* result in milliseconds */
            /* NOTE: order of dividing is important to not overflow the GT_U32 calculations */
            agingTime = (((ActionTimer * regValue)/coreClk)  * fdbAgingFactor) / (_1_MILLION / 1000/*milliseconds*/);
        }
        else
        {
            /* in milliseconds */
            agingTime = ((fdbAgingFactor * 1000/*milliseconds*/) * ActionTimer * fdbBaseCoreClock) /
                        coreClk;
        }

        timeOut = (agingTime * numOfEntries) / deviceObjPtr->fdbNumEntries ;

        currentSleepTime = timeOut;
    }
}

/*******************************************************************************
*   sfdbCht2MacTableUploadAction
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
GT_VOID sfdbCht2MacTableUploadAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * tblActPtr
)
{
    DECLARE_FUNC_NAME(sfdbCht2MacTableUploadAction);

    GT_U32 entry;                       /* MAC table entry index */
    GT_U32  *tblDataMemPtr = (GT_U32*)tblActPtr;
    AGE_DAEMON_ACTION_INFO_STC actionInfo;/* common info need for the 'Aging daemon' */
    GT_U32  regAddr;
    GT_BIT  needBreak = 0;

    /* get common action info */
    sfdbChtActionInfoGet(devObjPtr,tblDataMemPtr,&actionInfo);

    entry = devObjPtr->fdbAgingDaemonInfo.indexInFdb;

    for (/**/; entry < devObjPtr->fdbNumEntries; entry++)
    {
        sfdbChtFdbActionEntry(devObjPtr,SFDB_CHEETAH_TRIGGER_UPLOAD_E,&actionInfo,entry,&needBreak);
        if(needBreak)
        {
            /* the action is broken and will be continued in new message */
            /* save the index */
            devObjPtr->needResendMessage = 1;
            devObjPtr->fdbAgingDaemonInfo.indexInFdb = entry;
            return;
        }
        /* state that the daemon is NOT on hold */
        devObjPtr->fdbAgingDaemonInfo.daemonOnHold = 0;
    }

    devObjPtr->fdbAgingDaemonInfo.indexInFdb = 0;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION3_FDB_FDB_ACTION_GENERAL_REG(devObjPtr);
    }
    else
    {
        regAddr = SMEM_CHT_MAC_TBL_ACTION0_REG(devObjPtr);
    }

    /* Clear Aging Trigger */
    smemRegFldSet(devObjPtr, regAddr, 1, 1, 0);

    __LOG(("Interrupt: AGE_VIA_TRIGGER_END \n"));

    snetChetahDoInterrupt(devObjPtr,
                        SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                        SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                        SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                          SMEM_LION3_AGE_VIA_TRIGGER_END_INT :
                        SMEM_CHT_AGE_VIA_TRIGGER_END_INT,
                        SMEM_CHT_FDB_SUM_INT(devObjPtr));
}

/*******************************************************************************
* sfdbChtHashResultsRegistersUpdate
*
* DESCRIPTION:
*       SIP5 : update the hash result registers that relate to last AU message
*       from the CPU to the PP.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       calculatedHashArr[] - multi hash results :
*                           array of calculated hash by the different functions
*                           NULL means --> ignore the multi hash values
*       xorResult   -   xor result.
*       crcResult   -   CRC result.
*
* OUTPUTS:
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void sfdbChtHashResultsRegistersUpdate
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32       calculatedHashArr[FDB_MAX_BANKS_CNS],
    IN GT_U32       xorResult,
    IN GT_U32       crcResult
)
{
    GT_U32 regAddr;                         /* register address */
    GT_U32 regValue;
    GT_U32 ii;

    regValue = 0;

    if(calculatedHashArr)
    {
        for(ii = 0 ; ii < devObjPtr->fdbNumOfBanks; ii++)
        {
            SMEM_U32_SET_FIELD(regValue,16*(ii & 1),16,calculatedHashArr[ii]);

            if((ii & 1) == 1)
            {
                regAddr = SMEM_LION3_FDB_MULTI_HASH_CRC_RESULT_REG(devObjPtr,(ii / 2));
                smemRegSet(devObjPtr,regAddr,regValue);

                regValue = 0;
            }
        }
    }

    regAddr = SMEM_LION3_FDB_NON_MULTI_HASH_CRC_RESULT_REG(devObjPtr);
    smemRegSet(devObjPtr,regAddr,crcResult);

    regAddr = SMEM_LION3_FDB_NON_MULTI_HASH_XOR_HASH_REG(devObjPtr);
    smemRegSet(devObjPtr,regAddr,xorResult);

    return;
}

/*******************************************************************************
*   sfdbChtHrMsgProcess
*
* DESCRIPTION:
*       SIP5 : Hash Request (HR) - called by the CPU to calculate all the HASH
*               functions that the devices can generate :
*                CRC , XOR , 16 CRC multi-hash.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       msgWordPtr  - pointer to fdb message.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_STATUS sfdbChtHrMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                * msgWordPtr
)
{
    DECLARE_FUNC_NAME(sfdbChtHrMsgProcess);

    GT_U32 tblSize = devObjPtr->fdbNumEntries;
    GT_U32 numBitsToUse = SMEM_CHT_FDB_HASH_NUM_BITS(tblSize);
    GT_U32 calculatedHashArr[FDB_MAX_BANKS_CNS];
    enum{legacy_CRC = 1,legacy_XOR = 0, new_multi__CRC=0xFFF};
    GT_U32  vlanMode;
    GT_U32 macGlobCfgRegData;       /* Global register configuration data */
    SNET_CHEETAH_FDB_ENTRY_HASH_INFO_STC    entryHashInfo;
    GT_STATUS       status;
    GT_U32       xorResult;
    GT_U32       crcResult;

    status = parseAuMsgGetFdbEntryHashInfo(devObjPtr,msgWordPtr,&entryHashInfo);
    if(status != GT_OK)
    {
        return status;
    }

    /* Get data from MAC Table Global Configuration Register */
    smemRegGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr), &macGlobCfgRegData);

    vlanMode  = SMEM_U32_GET_FIELD(macGlobCfgRegData, 3, 1);

    /* get multi hash CRC calculation */
    sip5MacHashCalcMultiHash(new_multi__CRC, vlanMode, &entryHashInfo,numBitsToUse,
                devObjPtr->fdbNumOfBanks,calculatedHashArr);

    /* get legacy (single) hash CRC calculation */
    crcResult = cheetahMacHashCalcByStc(legacy_CRC, vlanMode, &entryHashInfo,numBitsToUse);
    /* the results come from the function as 'FDB index' */
    crcResult /= 4;

    /* get legacy (single) hash XOR calculation */
    xorResult = cheetahMacHashCalcByStc(legacy_XOR, vlanMode, &entryHashInfo,numBitsToUse);
    /* the results come from the function as 'FDB index' */
    xorResult /= 4;

    /* write the results to the registers */
    sfdbChtHashResultsRegistersUpdate(devObjPtr,calculatedHashArr,xorResult,crcResult);

    __LOG(("Interrupt: AU_PROC_COMPLETED \n"));

    /* Processing of an AU Message received by the device is completed */
    snetChetahDoInterrupt(devObjPtr,
                      SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                      SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                      SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
                        SMEM_LION3_AU_PROC_COMPLETED_INT :
                      SMEM_CHT_AU_PROC_COMPLETED_INT,
                      SMEM_CHT_FDB_SUM_INT(devObjPtr));

    return GT_OK;
}

/*******************************************************************************
*   sfdbChtBankCounterAction
*
* DESCRIPTION:
*       SIP5 : the AU message from the CPU need to be checked if to increment/decrement
*              the counters of the relevant FDB bank
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbIndex    - FDB index that relate to the entry that may have bank counter action
*       counterAction - the bank counter action.
*       client      - triggering client
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
* COMMENTS:
*
*
*******************************************************************************/
void sfdbChtBankCounterAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                   fdbIndex,
    IN SFDB_CHT_BANK_COUNTER_ACTION_ENT     counterAction,
    IN SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_ENT  client
)
{
    DECLARE_FUNC_NAME(sfdbChtBankCounterAction);

    GT_U32  bankIndex; /*index of the bank --> calculated from fdbIndex */
    GT_CHAR *actionName;/*string (name) of action type */
    GT_CHAR *clientName;/*string (name) of client type */
    GT_BIT  overflowError = 0; /*indication for error - type overrun */
    GT_BIT  underflowError = 0;/*indication for error - type underrun */
    GT_U32  bankRankIndex;/* the rank of the bank */
    GT_U32  tmpBankIndex;/* temp - bank index */
    GT_U32  tmpCounter;  /* temp - bank counter */
    GT_U32  origBankRankIndex;/* the original rank of the bank */
    GT_U32  ii;/*iterator*/

    if((devObjPtr->multiHashEnable == 0) &&
       (SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_EXACT_BANK_E != client))
    {
        /* There is no bank management when working in none MULTI hash mode.
           Management works always by CPU direct update regardless of hash mode. */
        return;
    }

    switch(counterAction)
    {
        case SFDB_CHT_BANK_COUNTER_ACTION_NONE_E:
            return ;
        case SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E:
            actionName = "INCREMENT";
            break;
        case SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E:
            actionName = "DECREMENT";
            break;
        default:
            return ;
    }

    switch(client)
    {
        case SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E:
            clientName = "CPU_AU_MSG";
            break;
        case SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_EXACT_BANK_E:
            clientName = "CPU_EXACT_BANK";
            break;
        case SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_PP_AGING_DAEMON_E:
            clientName = "PP_AGING_DAEMON";
            break;
        case SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_PP_AUTO_LEARN_E:
            clientName = "PP_AUTO_LEARN";
            break;
        default:
            skernelFatalError("sfdbChtBankCounterAction: not supported client[%d] \n",client);
            return;
    }

    /* calculate the bank from the FDB index */
    bankIndex = fdbIndex % devObjPtr->fdbNumOfBanks;

    SCIB_SEM_TAKE;
    while(devObjPtr->fdbBankCounterInCpuPossess)
    {
        SCIB_SEM_SIGNAL;
        /* wait for CPU to release the counters */
        SIM_OS_MAC(simOsSleep)(50);
        SCIB_SEM_TAKE;

        if(devObjPtr->needToDoSoftReset)
        {
            /* we can not allow the thread to be stuck here */
            break;
        }
    }

    devObjPtr->fdbBankCounterUsed = 1;
    SCIB_SEM_SIGNAL;

    bankRankIndex = SMAIN_NOT_VALID_CNS;
    for( ii = 0 ; ii < devObjPtr->fdbNumOfBanks ; ii++)
    {
        if((*devObjPtr->fdbBanksRankArr[ii]) == bankIndex)
        {
            bankRankIndex = ii;
            break;
        }
    }

    origBankRankIndex = bankRankIndex;

    if(counterAction == SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E)
    {
        if((*devObjPtr->fdbBanksCounterArr[bankIndex]) >= ((devObjPtr->fdbMaxNumEntries / devObjPtr->fdbNumOfBanks)-1))
        {
            overflowError = 1;
        }
        else
        {
            (*devObjPtr->fdbBanksCounterArr[bankIndex])++;

            /* re-rank the counters */
            /* check if counter's rank need to be 'upgrated' (swap with higher rank) */
            if((bankRankIndex != SMAIN_NOT_VALID_CNS) && (bankRankIndex != 0))
            {
                for( ii = (bankRankIndex - 1) ; /*break condition is not here*/ ; ii--)
                {
                    /* bank index of 'better ranked' bank */
                    tmpBankIndex = (*devObjPtr->fdbBanksRankArr[ii]);
                    /* the counter of the 'better ranked' bank */
                    tmpCounter = (*devObjPtr->fdbBanksCounterArr[tmpBankIndex]);

                    if((*devObjPtr->fdbBanksCounterArr[bankIndex]) > tmpCounter)
                    {
                        /*the counter of 'this' bank is higher then the counter of bank ranked higher*/
                        /*swap the ranks*/
                        (*devObjPtr->fdbBanksRankArr[ii]) = bankIndex;
                        (*devObjPtr->fdbBanksRankArr[bankRankIndex]) = tmpBankIndex;
                        bankRankIndex = ii;
                    }
                    else
                    {
                        /* no need to continue the search for better rank */
                        break;
                    }

                    if(ii == 0)
                    {
                        /* no more swaps can be */
                        break;
                    }
                }
            }/*rank check*/
        }
    }
    else
    {
        if((*devObjPtr->fdbBanksCounterArr[bankIndex]) == 0)
        {
            underflowError = 1;
        }
        else
        {
            (*devObjPtr->fdbBanksCounterArr[bankIndex])--;

            /* re-rank the counters */
            /* check if counter's rank need to be 'downgrated' (swap with lower rank) */
            if(bankRankIndex != SMAIN_NOT_VALID_CNS)
            {
                for( ii = bankRankIndex + 1 ; ii < devObjPtr->fdbNumOfBanks ; ii++)
                {
                    /* bank index of 'lower ranked' bank */
                    tmpBankIndex = (*devObjPtr->fdbBanksRankArr[ii]);
                    /* the counter of the 'lower ranked' bank */
                    tmpCounter = (*devObjPtr->fdbBanksCounterArr[tmpBankIndex]);

                    if((*devObjPtr->fdbBanksCounterArr[bankIndex]) < tmpCounter)
                    {
                        /*the counter of 'this' bank is lower then the counter of bank ranked lower */
                        /*swap the ranks*/
                        (*devObjPtr->fdbBanksRankArr[ii]) = bankIndex;
                        (*devObjPtr->fdbBanksRankArr[bankRankIndex]) = tmpBankIndex;
                        bankRankIndex = ii;
                    }
                    else
                    {
                        /* no need to continue the search for lower rank */
                        break;
                    }
                }
            }/*rank check*/
        }

    }

    devObjPtr->fdbBankCounterUsed = 0;


    if(overflowError || underflowError)
    {
        __LOG(("Interrupt: BLC (bank learn counters) overflow/underflow \n"));

        /* generate BLC (bank learn counters) overflow/underflow interrupt */
        snetChetahDoInterrupt(devObjPtr,
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_REG(devObjPtr),
                              SMEM_CHT_MAC_TBL_INTR_CAUSE_MASK_REG(devObjPtr),
                              SMEM_LION3_BLC_OVERFLOW_INT,
                              SMEM_CHT_FDB_SUM_INT(devObjPtr));
    }

    if(simLogIsOpen())
    {
        if(overflowError)
        {
            __LOG(("client[%s] counterAction[%s] for bank[%d] OVERFLOW ERROR ,already at value[%d] \n",
                          clientName,actionName,bankIndex,
                          (*devObjPtr->fdbBanksCounterArr[bankIndex])));
        }
        else if(underflowError)
        {
            __LOG(("client[%s] counterAction[%s] for bank[%d] UNDERFLOW ERROR ,already at value 0 \n",
                          clientName,actionName,bankIndex));
        }
        else
        {
            __LOG(("client[%s] counterAction[%s] for bank[%d] new value[%d] \n"
                          "bank rank changed from [%d] to [%d] \n",
                          clientName,actionName,bankIndex,
                          (*devObjPtr->fdbBanksCounterArr[bankIndex]),
                          origBankRankIndex , bankRankIndex));
        }
    }

    return ;
}

