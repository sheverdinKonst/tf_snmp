/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssGenPpTypesManualLog.c
*       Manually implemented CPSS Log type wrappers
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <cpss/generic/log/cpssLog.h>
#include <cpss/generic/log/prvCpssLog.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/generic/cpssCommonDefs.h>
#include <cpss/generic/cpssTypes.h>
#include <cpss/generic/log/prvCpssGenCommonTypesLog.h>
#include <cpss/generic/log/prvCpssGenPpTypesLog.h>


void prvCpssLogParamFuncStc_CPSS_INTERFACE_INFO_STC_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_START_FIELD_STC_MAC(CPSS_INTERFACE_INFO_STC *, stcPtr);

    PRV_CPSS_LOG_STC_ENUM_MAC(stcPtr, type, CPSS_INTERFACE_TYPE_ENT);

    switch(PRV_CPSS_LOG_STC_FIELD_VAL_MAC(stcPtr, type))
    {
        case CPSS_INTERFACE_PORT_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, devPort.hwDevNum);
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, devPort.portNum);
            break;

        case CPSS_INTERFACE_TRUNK_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, trunkId);
            break;

        case CPSS_INTERFACE_VIDX_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, vidx);
            break;

        case CPSS_INTERFACE_VID_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, vlanId);
            break;

        case CPSS_INTERFACE_DEVICE_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, hwDevNum);
            break;

        case CPSS_INTERFACE_FABRIC_VIDX_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, fabricVidx);
            break;

        case CPSS_INTERFACE_INDEX_E:
            PRV_CPSS_LOG_STC_NUMBER_MAC(stcPtr, index);
            break;

        default:
            break;
    }

    prvCpssLogStcLogEnd(contextLib, logType);
}

void prvCpssLogParamFuncStc_GT_IPV6ADDR_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_START_FIELD_STC_MAC(GT_IPV6ADDR *, valPtr);
    prvCpssLogStcIpV6(contextLib,logType,namePtr, valPtr->arIP);

}

void prvCpssLogParamFuncStc_GT_ETHERADDR_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_SET_FIELD_VAL_PTR_MAC(GT_ETHERADDR *, valPtr);
    prvCpssLogStcMac(contextLib, logType, namePtr, valPtr->arEther);
}
void prvCpssLogParamFuncStc_GT_IPADDR_PTR
(
    IN CPSS_LOG_LIB_ENT         contextLib,
    IN CPSS_LOG_TYPE_ENT        logType,
    IN GT_CHAR_PTR              namePtr,
    IN void                   * fieldPtr
)
{
    PRV_CPSS_LOG_SET_FIELD_VAL_PTR_MAC(GT_IPADDR *, valPtr);
    prvCpssLogStcIpV4(contextLib,logType,namePtr, valPtr->arIP);
}

void prvCpssLogParamFunc_GT_PORT_GROUPS_BMP(
    IN CPSS_LOG_LIB_ENT            contextLib,
    IN CPSS_LOG_TYPE_ENT           logType,
    IN GT_CHAR_PTR                 namePtr,
    IN va_list                   * argsPtr,
    IN GT_BOOL                     skipLog,
    IN PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC   * inOutParamInfoPtr
)
{
    inOutParamInfoPtr->formatPtr = "%s = 0x%04x\n";
    prvCpssLogParamFunc_GT_U32(contextLib, logType, namePtr,argsPtr,skipLog,inOutParamInfoPtr);
}
