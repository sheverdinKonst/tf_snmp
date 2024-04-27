/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbCheetah.h
*
* DESCRIPTION:
*       Cheetah FDB update, Triggered actions messages simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#ifndef __sfdbcheetahh
#define __sfdbcheetahh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <common/Utils/PresteraHash/smacHashDx.h>

typedef enum{
    SFDB_CHT_BANK_COUNTER_ACTION_NONE_E = 0,
    SFDB_CHT_BANK_COUNTER_ACTION_INCREMENT_E = 1,
    SFDB_CHT_BANK_COUNTER_ACTION_DECREMENT_E = 2
}SFDB_CHT_BANK_COUNTER_ACTION_ENT;


typedef enum{
    SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_AU_MSG_E,
    SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_CPU_EXACT_BANK_E,
    SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_PP_AGING_DAEMON_E,
    SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_PP_AUTO_LEARN_E
}SFDB_CHT_BANK_COUNTER_UPDATE_CLIENT_ENT;

/*
 * typedef: structure SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC
 *
 * Description:
 *       Muxed fields from the FDB that depend on :
 *       1. vid1_assignment_mode
 *       2. src_id_length_in_fdb
 *
 *      field with value : SMAIN_NOT_VALID_CNS , means 'not used'
*/
typedef struct{
    GT_U32              srcId;
    GT_U32              udb;
    GT_U32              origVid1;
    GT_U32              daAccessLevel;
    GT_U32              saAccessLevel;
}SFDB_LION3_FDB_SPECIAL_MUXED_FIELDS_STC;

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
void sfdbChtMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * fdbMsgPtr
);

/*******************************************************************************
*   sfdbChtMacTableAging
*
* DESCRIPTION:
*       MAC Table Trigger Action
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - pointer to table action data
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void sfdbChtMacTableTriggerAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * tblActPtr
);


/*******************************************************************************
*   sfdbChtMacTableAutomaticAging
*
* DESCRIPTION:
*       MAC Table automatic aging.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       data_PTR   - pointer to number of records to search
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sfdbChtMacTableAutomaticAging
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * data_PTR
);

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
);

/*******************************************************************************
*   sfdbCht2MacTableUploadAction
*
* DESCRIPTION:
*       MAC Table fdb upload engine.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tblActPtr   - ?????
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void sfdbCht2MacTableUploadAction
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * tblActPtr
);

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
);

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
);

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
);

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
);


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
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sfdbcheetahh */


