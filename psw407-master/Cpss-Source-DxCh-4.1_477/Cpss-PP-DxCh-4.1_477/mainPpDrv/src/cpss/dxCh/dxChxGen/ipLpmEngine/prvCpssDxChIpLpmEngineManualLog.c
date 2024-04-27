/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChIpLpmEngineManualLog.c
*       Manually implemented CPSS Log type wrappers
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <cpss/generic/log/cpssLog.h>
#include <cpss/generic/log/prvCpssLog.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/dxCh/dxChxGen/ip/private/prvCpssDxChIpLog.h>
#include <cpss/dxCh/dxChxGen/ipLpmEngine/cpssDxChIpLpmTypes.h>
#include <cpss/dxCh/dxChxGen/ipLpmEngine/private/prvCpssDxChIpLpmEngineLog.h>
#include <cpss/dxCh/dxChxGen/lpm/private/prvCpssDxChLpmLog.h>
#include <cpss/dxCh/dxChxGen/pcl/private/prvCpssDxChPclLog.h>
#include <cpss/generic/log/prvCpssGenLog.h>

/********* enums *********/
/********* structure fields log functions *********/

/********* parameters log functions *********/
void prvCpssLogParamFunc_CPSS_DXCH_IP_LPM_MEMORY_CONFIG_UNT_PTR(
    IN CPSS_LOG_LIB_ENT            contextLib,
    IN CPSS_LOG_TYPE_ENT           logType,
    IN GT_CHAR_PTR                 namePtr,
    IN va_list                   * argsPtr,
    IN GT_BOOL                     skipLog,
    IN PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC   * inOutParamInfoPtr
)
{
    CPSS_DXCH_IP_LPM_SHADOW_TYPE_ENT shadowType;
    PRV_CPSS_LOG_START_PARAM_STC_MAC(CPSS_DXCH_IP_LPM_MEMORY_CONFIG_UNT*, paramVal);

    prvCpssLogStcLogStart(contextLib, logType, namePtr);

    shadowType = (inOutParamInfoPtr->paramKey.paramKeyArr[1] == PRV_CPSS_LOG_PARAM_IN_E) ?
          ((CPSS_DXCH_IP_LPM_SHADOW_TYPE_ENT)inOutParamInfoPtr->paramKey.paramKeyArr[0]):
          ((inOutParamInfoPtr->paramKey.paramKeyArr[0] == 0) ? CPSS_DXCH_IP_LPM_RAM_LION3_SHADOW_E :
              *(CPSS_DXCH_IP_LPM_SHADOW_TYPE_ENT*)inOutParamInfoPtr->paramKey.paramKeyArr[0]);

    if((shadowType == CPSS_DXCH_IP_LPM_TCAM_CHEETAH_SHADOW_E) ||
       (shadowType == CPSS_DXCH_IP_LPM_TCAM_CHEETAH2_SHADOW_E) ||
       (shadowType == CPSS_DXCH_IP_LPM_TCAM_CHEETAH3_SHADOW_E) ||
       (shadowType == CPSS_DXCH_IP_LPM_TCAM_XCAT_SHADOW_E) ||
       (shadowType == CPSS_DXCH_IP_LPM_TCAM_XCAT_POLICY_BASED_ROUTING_SHADOW_E) ||
       (shadowType == CPSS_DXCH_IP_LPM_RAM_LION3_SHADOW_E))
    {
        if (shadowType < CPSS_DXCH_IP_LPM_RAM_LION3_SHADOW_E) 
        {
            PRV_CPSS_LOG_STC_STC_MAC(paramVal, tcamDbCfg, CPSS_DXCH_IP_LPM_TCAM_CONFIG_STC);
        }
        else
        {
            PRV_CPSS_LOG_STC_STC_MAC(paramVal, ramDbCfg, CPSS_DXCH_LPM_RAM_CONFIG_STC);
        }

    }
    prvCpssLogStcLogEnd(contextLib, logType);
}
/********* structure fields log functions *********/
/********* api pre-log functions *********/
void cpssDxChIpLpmDBCreate_preLogic
(
    IN va_list  args,
    OUT PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC * paramDataPtr
)
{
    /* lpmDBId */
    va_arg(args, GT_U32);
    /* shadowType - key for union parsing */
    paramDataPtr->paramKey.paramKeyArr[0] = (GT_UINTPTR)va_arg(args, CPSS_DXCH_IP_LPM_SHADOW_TYPE_ENT);
    paramDataPtr->paramKey.paramKeyArr[1] =  PRV_CPSS_LOG_PARAM_IN_E;
}

void cpssDxChIpLpmDBConfigGet_preLogic
(
    IN va_list  args,
    OUT PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC * paramDataPtr
)
{
    /* lpmDBId */
    va_arg(args, GT_U32);
    /* shadowTypePtr - key for union parsing */
    paramDataPtr->paramKey.paramKeyArr[0] = (GT_UINTPTR)va_arg(args, CPSS_DXCH_IP_LPM_SHADOW_TYPE_ENT *);
    paramDataPtr->paramKey.paramKeyArr[1] =  PRV_CPSS_LOG_PARAM_OUT_E;
}
