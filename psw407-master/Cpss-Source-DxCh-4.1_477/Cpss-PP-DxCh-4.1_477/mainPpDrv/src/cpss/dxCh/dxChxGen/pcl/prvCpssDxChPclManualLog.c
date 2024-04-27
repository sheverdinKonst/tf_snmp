/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPclManualLog.c
*       Manually implemented CPSS Log type wrappers
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <cpss/generic/log/cpssLog.h>
#include <cpss/generic/log/prvCpssLog.h>
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/dxCh/dxChxGen/pcl/cpssDxChPcl.h>
#include <cpss/dxCh/dxChxGen/pcl/private/prvCpssDxChPclLog.h>
#include <cpss/generic/log/prvCpssGenLog.h>
#include <cpss/generic/networkIf/private/prvCpssGenNetworkIfLog.h>
#include <cpss/generic/pcl/private/prvCpssGenPclLog.h>


/********* enums *********/
/********* structure fields log functions *********/
/********* parameters log functions *********/
void prvCpssLogParamFunc_CPSS_DXCH_PCL_RULE_FORMAT_UNT_PTR(
    IN CPSS_LOG_LIB_ENT            contextLib,
    IN CPSS_LOG_TYPE_ENT           logType,
    IN GT_CHAR_PTR                 namePtr,
    IN va_list                   * argsPtr,
    IN GT_BOOL                     skipLog,
    IN PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC   * inOutParamInfoPtr
)
{
    CPSS_DXCH_PCL_RULE_FORMAT_TYPE_ENT ruleFormat;

    PRV_CPSS_LOG_START_PARAM_STC_MAC(CPSS_DXCH_PCL_RULE_FORMAT_UNT *, stcPtr);
    prvCpssLogStcLogStart(contextLib, logType, namePtr);

    ruleFormat = (CPSS_DXCH_PCL_RULE_FORMAT_TYPE_ENT)inOutParamInfoPtr->paramKey.paramKeyArr[0];

    switch (ruleFormat)
    {
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STD_NOT_IP_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleStdNotIp, CPSS_DXCH_PCL_RULE_FORMAT_STD_NOT_IP_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STD_IP_L2_QOS_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleStdIpL2Qos, CPSS_DXCH_PCL_RULE_FORMAT_STD_IP_L2_QOS_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STD_IPV4_L4_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleStdIpv4L4, CPSS_DXCH_PCL_RULE_FORMAT_STD_IPV4_L4_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STD_IPV6_DIP_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleStdIpv6Dip, CPSS_DXCH_PCL_RULE_FORMAT_STD_IPV6_DIP_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_EXT_NOT_IPV6_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleExtNotIpv6, CPSS_DXCH_PCL_RULE_FORMAT_EXT_NOT_IPV6_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_EXT_IPV6_L2_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleExtIpv6L2, CPSS_DXCH_PCL_RULE_FORMAT_EXT_IPV6_L2_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_EXT_IPV6_L4_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleExtIpv6L4, CPSS_DXCH_PCL_RULE_FORMAT_EXT_IPV6_L4_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_STD_NOT_IP_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrStdNotIp, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_STD_NOT_IP_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_STD_IP_L2_QOS_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrStdIpL2Qos, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_STD_IP_L2_QOS_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_STD_IPV4_L4_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrStdIpv4L4, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_STD_IPV4_L4_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_NOT_IPV6_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrExtNotIpv6, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_NOT_IPV6_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_IPV6_L2_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrExtIpv6L2, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_IPV6_L2_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_IPV6_L4_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrExtIpv6L4, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_IPV6_L4_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STD_UDB_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleIngrStdUdb, CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STANDARD_UDB_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_EXT_UDB_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleIngrExtUdb, CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_EXTENDED_UDB_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_STD_IPV4_ROUTED_ACL_QOS_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleStdIpv4RoutedAclQos, CPSS_DXCH_PCL_RULE_FORMAT_STD_IPV4_ROUTED_ACL_QOS_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_EXT_IPV4_PORT_VLAN_QOS_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleExtIpv4PortVlanQos, CPSS_DXCH_PCL_RULE_FORMAT_EXT_IPV4_PORT_VLAN_QOS_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_ULTRA_IPV6_PORT_VLAN_QOS_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleUltraIpv6PortVlanQos, CPSS_DXCH_PCL_RULE_FORMAT_ULTRA_IPV6_PORT_VLAN_QOS_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_ULTRA_IPV6_ROUTED_ACL_QOS_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleUltraIpv6RoutedAclQos, CPSS_DXCH_PCL_RULE_FORMAT_ULTRA_IPV6_ROUTED_ACL_QOS_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_IPV4_RACL_VACL_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrExtIpv4RaclVacl, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_EXT_IPV4_RACL_VACL_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_ULTRA_IPV6_RACL_VACL_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrUltraIpv6RaclVacl, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_ULTRA_IPV6_RACL_VACL_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_10_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_20_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_30_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_40_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_50_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_60_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleIngrUdbOnly, CPSS_DXCH_PCL_RULE_FORMAT_INGRESS_UDB_ONLY_STC);
        break;
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_10_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_20_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_30_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_40_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_50_E:
    case CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_60_E:
        PRV_CPSS_LOG_STC_STC_MAC(stcPtr, ruleEgrUdbOnly, CPSS_DXCH_PCL_RULE_FORMAT_EGRESS_UDB_ONLY_STC);
        break;
    default:
        break;
    }

    prvCpssLogStcLogEnd(contextLib, logType);
}

/********* api pre-log functions *********/
void cpssDxChPclRuleSet_preLogic
(
    IN va_list  args,
    OUT PRV_CPSS_LOG_PARAM_ENTRY_INFO_STC * paramDataPtr
)
{
    /* devNum */
    va_arg(args, GT_32);
    /* ruleFormat */
    paramDataPtr->paramKey.paramKeyArr[0] = (CPSS_DXCH_PCL_RULE_FORMAT_TYPE_ENT)va_arg(args, CPSS_DXCH_PCL_RULE_FORMAT_TYPE_ENT);
}
