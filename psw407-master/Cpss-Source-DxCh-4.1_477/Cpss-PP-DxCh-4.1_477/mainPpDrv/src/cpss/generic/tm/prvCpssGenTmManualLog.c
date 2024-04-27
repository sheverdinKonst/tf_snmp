/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssGenTmManualLog.c
*       Manually implemented CPSS Log types wrappers
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <cpss/generic/log/cpssLog.h>
#include <cpss/generic/log/prvCpssLog.h>
#include <cpss/generic/tm/cpssTmPublicDefs.h>


/********* structure fields log functions *********/


void prvCpssLogParamFuncStc_CPSS_TM_TREE_CHANGE_STC_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_START_FIELD_STC_MAC(CPSS_TM_TREE_CHANGE_STC *, valPtr);
    PRV_CPSS_LOG_STC_NUMBER_MAC(valPtr, type);
    PRV_CPSS_LOG_STC_NUMBER_MAC(valPtr, index);
    PRV_CPSS_LOG_STC_NUMBER_MAC(valPtr, old);
    PRV_CPSS_LOG_STC_NUMBER_MAC(valPtr, new);
    PRV_CPSS_LOG_STC_PTR_MAC(valPtr, nextPtr);
    prvCpssLogStcLogEnd(contextLib, logType);
}
