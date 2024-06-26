/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssGenSystemRecoveryLog.c
*       WARNING!!! this is a generated file, please don't edit it manually
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <cpss/generic/log/cpssLog.h>
#include <cpss/generic/log/prvCpssLog.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/generic/log/prvCpssGenCommonTypesLog.h>
#include <cpss/generic/systemRecovery/cpssGenSystemRecovery.h>
#include <cpss/generic/systemRecovery/private/prvCpssGenSystemRecoveryLog.h>


/********* enums *********/

char * prvCpssLogEnum_CPSS_SYSTEM_RECOVERY_STATE_ENT[]  =
{
    "CPSS_SYSTEM_RECOVERY_PREPARATION_STATE_E",
    "CPSS_SYSTEM_RECOVERY_INIT_STATE_E",
    "CPSS_SYSTEM_RECOVERY_COMPLETION_STATE_E",
    "CPSS_SYSTEM_RECOVERY_HW_CATCH_UP_STATE_E"
};
PRV_CPSS_LOG_STC_ENUM_ARRAY_SIZE_MAC(CPSS_SYSTEM_RECOVERY_STATE_ENT);
char * prvCpssLogEnum_CPSS_SYSTEM_RECOVERY_PROCESS_ENT[]  =
{
    "CPSS_SYSTEM_RECOVERY_PROCESS_HSU_E",
    "CPSS_SYSTEM_RECOVERY_PROCESS_FAST_BOOT_E",
    "CPSS_SYSTEM_RECOVERY_PROCESS_HA_E",
    "CPSS_SYSTEM_RECOVERY_PROCESS_NOT_ACTIVE_E"
};
PRV_CPSS_LOG_STC_ENUM_ARRAY_SIZE_MAC(CPSS_SYSTEM_RECOVERY_PROCESS_ENT);


/********* structure fields log functions *********/

void prvCpssLogParamFuncStc_CPSS_SYSTEM_RECOVERY_INFO_STC_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_START_FIELD_STC_MAC(CPSS_SYSTEM_RECOVERY_INFO_STC *, valPtr);
    PRV_CPSS_LOG_STC_ENUM_MAC(valPtr, systemRecoveryState, CPSS_SYSTEM_RECOVERY_STATE_ENT);
    PRV_CPSS_LOG_STC_STC_MAC(valPtr, systemRecoveryMode, CPSS_SYSTEM_RECOVERY_MODE_STC);
    PRV_CPSS_LOG_STC_ENUM_MAC(valPtr, systemRecoveryProcess, CPSS_SYSTEM_RECOVERY_PROCESS_ENT);
    prvCpssLogStcLogEnd(contextLib, logType);
}
void prvCpssLogParamFuncStc_CPSS_SYSTEM_RECOVERY_MODE_STC_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_START_FIELD_STC_MAC(CPSS_SYSTEM_RECOVERY_MODE_STC *, valPtr);
    PRV_CPSS_LOG_STC_BOOL_MAC(valPtr, continuousRx);
    PRV_CPSS_LOG_STC_BOOL_MAC(valPtr, continuousTx);
    PRV_CPSS_LOG_STC_BOOL_MAC(valPtr, continuousAuMessages);
    PRV_CPSS_LOG_STC_BOOL_MAC(valPtr, continuousFuMessages);
    PRV_CPSS_LOG_STC_BOOL_MAC(valPtr, haCpuMemoryAccessBlocked);
    prvCpssLogStcLogEnd(contextLib, logType);
}


/********* parameters log functions *********/

void prvCpssLogParamFunc_CPSS_SYSTEM_RECOVERY_INFO_STC_PTR(
    IN CPSS_LOG_LIB_ENT            contextLib,
    IN CPSS_LOG_TYPE_ENT           logType,
    IN GT_CHAR_PTR                 namePtr,
    IN va_list                   * argsPtr,
    IN GT_BOOL                     skipLog,
    IN PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC   * inOutParamInfoPtr
)
{
    PRV_CPSS_LOG_START_PARAM_STC_MAC(CPSS_SYSTEM_RECOVERY_INFO_STC*, paramVal);
    prvCpssLogParamFuncStc_CPSS_SYSTEM_RECOVERY_INFO_STC_PTR(contextLib, logType, namePtr, paramVal);
}

