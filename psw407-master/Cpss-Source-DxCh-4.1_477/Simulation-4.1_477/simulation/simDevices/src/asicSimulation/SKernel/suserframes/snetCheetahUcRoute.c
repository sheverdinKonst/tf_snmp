/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetChtUcRoute.c
*
* DESCRIPTION:
*       Router Engine Processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 20 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahUcRoute.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPcl.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SLog/simLog.h>

static GT_VOID snetChtUcExeptionCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    OUT GT_BIT * exBitPtr
);

static GT_VOID snetChtUcCommandResolve
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_BIT exBit
);

static GT_STATUS snetChtIpv4HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 ucRouteEngCfgReg
);

static GT_STATUS snetChtIpv6HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 ucRouteEngCfgReg
);

static GT_STATUS snetChtIpv4TtlExceeded
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
);

static GT_STATUS snetChtIpv6HopLimitExceeded
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
);

static GT_STATUS snetChtIpv4Options
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
);

static GT_STATUS snetChtIpv6HopByHopOptHeader
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
);

static GT_STATUS snetChtIcmpRedirectError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr
);

#define ASSERT_EXCEPTION(status, except_bit_ptr)\
{                                               \
    *except_bit_ptr = 0;                        \
    if (status == GT_TRUE)                      \
    {                                           \
        *except_bit_ptr = 1;                    \
        return;                                 \
    }                                           \
}
/*******************************************************************************
*   snetChtUcExeptionCheck
*
* DESCRIPTION:
*    The following exception checks are made on IPv4 packets:
*    �  IPv4 Header Error
*    �  IPv4 TTL Exceeded
*    �  IPv4 Options
*
*    The following exception checks are made on IPv6 packets:
*    �  IPv6 Header Error
*    �  IPv6 Hop Limit Exceeded
*    �  IPv6 Hop-by-Hop Options Header
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
* OUTPUTS:
*       exRaisePtr - exception flag raised
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtUcExeptionCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    OUT GT_BIT * exBitPtr
)
{
    GT_U32 * ucEngConfRegPtr;   /* unicast engine configuration register */

    ucEngConfRegPtr = smemMemGet(devObjPtr,
                                 SMEM_CHT_UC_ROUT_ENG_CONF_REG(devObjPtr));

    if(descrPtr->isIPv4)
    {
        ASSERT_EXCEPTION(
            snetChtIpv4HeaderError(devObjPtr, descrPtr, ucEngConfRegPtr[0]),
            exBitPtr );

        ASSERT_EXCEPTION(
            snetChtIpv4TtlExceeded(devObjPtr, descrPtr, nextHopPolicyActionPtr,
            ucEngConfRegPtr[0]),
            exBitPtr );

        ASSERT_EXCEPTION(
            snetChtIpv4Options(devObjPtr, descrPtr, nextHopPolicyActionPtr,
            ucEngConfRegPtr[0]),
            exBitPtr );
    }
    else    /* ipv6 */
    {
        ASSERT_EXCEPTION(
            snetChtIpv6HeaderError(devObjPtr, descrPtr, ucEngConfRegPtr[0]),
            exBitPtr );

        ASSERT_EXCEPTION(
            snetChtIpv6HopLimitExceeded(devObjPtr, descrPtr,
            nextHopPolicyActionPtr, ucEngConfRegPtr[0]),
            exBitPtr );

        ASSERT_EXCEPTION(
            snetChtIpv6HopByHopOptHeader(devObjPtr, descrPtr, nextHopPolicyActionPtr,
            ucEngConfRegPtr[0]),
            exBitPtr );
    }

    ASSERT_EXCEPTION(
        snetChtIcmpRedirectError(devObjPtr, descrPtr, nextHopPolicyActionPtr),
        exBitPtr);
}

/*******************************************************************************
*   snetChtUcCommandResolve
*
* DESCRIPTION:
*    The final packet command resolution is based on the following:
*    �  Bridge engine packet command (must be either FORWARD or MIRROR,
*       otherwise the packet is not eligible for routing.)
*    �  Routing PCL action <Packet Command>
*    �  Exception commands in the event where one or more exceptions are detected.
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
*       exBit        - exception has raised
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtUcCommandResolve
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_BIT exBit
)
{
    DECLARE_FUNC_NAME(snetChtUcCommandResolve);

    GT_U32                      isCpuTrgt = 0;

    /* the UC router glue layer - will cause traffic 'to_cpu' to be with
    the 'routed Vid' and not the original for tagged packet and
    not ingress pipe (until router) for untagged packets */
    descrPtr->useIngressPipeVid = GT_FALSE;

    if (exBit)
    {
        /* Exception command handler */
        __LOG(("Exception command handler"));
        if (descrPtr->cpuCode == SNET_CHT_IPV4_HEADER_ERROR ||
            descrPtr->cpuCode == SNET_CHT_IPV6_HEADER_ERROR ||
            descrPtr->cpuCode == SNET_CHT_IPV4_UC_TTL_EXCEEDED ||
            descrPtr->cpuCode == SNET_CHT_IPV6_UC_HOP_LIMIT_EXCEEDED ||
            descrPtr->cpuCode == SNET_CHT_IPV4_UC_OPTIONS ||
            descrPtr->cpuCode == SNET_CHT_IPV6_UC_HOP_BY_HOP)
        {
            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
            isCpuTrgt = 1;
        }
        else
        if (descrPtr->cpuCode == SNET_CHT_IPV4_UC_ICMP_REDIRECT ||
            descrPtr->cpuCode == SNET_CHT_IPV6_UC_ICMP_REDIRECT)
        {
            descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E;
            isCpuTrgt = 1;
        }
    }
    else
    {
        /* merge packet command with next hop command */
        /* nextHopPolicyActionPtr->fwdCmd; */
        /* descrPtr->packetCmd; */

        /* resolve packet command and CPU code */
        __LOG(("resolve packet command and CPU code"));
        snetChtIngressCommandAndCpuCodeResolution(
            devObjPtr,descrPtr,
            descrPtr->packetCmd,
            nextHopPolicyActionPtr->fwdCmd,
            descrPtr->cpuCode,
            ((descrPtr->isIPv4)?
                SNET_CHT_IPV4_UC_ROUTE0:
                SNET_CHT_IPV6_UC_ROUTE0),
            SNET_CHEETAH_ENGINE_UNIT_ROUTER_E,
            GT_TRUE /*isFirst*/);

        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
        {
            isCpuTrgt = 1;
        }
    }

    /* Ingress mirroring command */
    if(nextHopPolicyActionPtr->mirrorToIngressAnalyzerPort == GT_TRUE)
    {
        descrPtr->rxSniff = 1;
    }

    /* Select mirror analyzer index */
    snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr,
        nextHopPolicyActionPtr->mirrorToIngressAnalyzerPort);

    /* The redirect info */
    __LOG(("The redirect info"));
    descrPtr->useVidx = nextHopPolicyActionPtr->routEgressInterface.useVidx;
    if (descrPtr->useVidx)
    {
        descrPtr->eVidx =
            nextHopPolicyActionPtr->routEgressInterface.vidxInfo.vidx;
    }
    else
    {
        descrPtr->targetIsTrunk =
            nextHopPolicyActionPtr->routEgressInterface.vidxInfo.noVidx.
            targetIsTrunk;

        if (descrPtr->targetIsTrunk)
        {
            descrPtr->trgTrunkId =
                nextHopPolicyActionPtr->routEgressInterface.vidxInfo.noVidx.
                trunkInfo.targetTrunk;

        }
        else
        {
            descrPtr->trgEPort =
                nextHopPolicyActionPtr->routEgressInterface.vidxInfo.noVidx.
                trunkInfo.noTrunk.targetPort;

            descrPtr->trgDev =
                nextHopPolicyActionPtr->routEgressInterface.vidxInfo.noVidx.
                trunkInfo.noTrunk.targetDev;

            if (( descrPtr->trgEPort == SNET_CHT_CPU_PORT_CNS ) && ( !exBit ))
            {
                isCpuTrgt = 1;
                descrPtr->packetCmd =  SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E ;
                descrPtr->cpuCode = SNET_CHT_ROUTED_PACKET_FORWARD ;
            }
        }
    }
    /* End redirect info */

    /* p. 278 : Packets (including routed packets) queues to the CPU are not
                subject to any packet modification other then DSA tagging */
    if (isCpuTrgt)
    {
        return ;
    }
    /* QoS Marking Command */
    if(descrPtr->qosProfilePrecedence == SKERNEL_QOS_PROF_PRECED_SOFT)
    {
        if(nextHopPolicyActionPtr->routQosPrecedence ==
                    (SKERNEL_PRECEDENCE_ORDER_ENT)SKERNEL_QOS_PROF_PRECED_HARD)
        {
            descrPtr->qosProfilePrecedence = SKERNEL_QOS_PROF_PRECED_HARD;
        }

        if(nextHopPolicyActionPtr->routQosProfileMarkingEn == GT_TRUE)
        {
            descrPtr->qos.qosProfile = nextHopPolicyActionPtr->routQosProfile;
        }

        if(nextHopPolicyActionPtr->routModifyDscp == 1)
        {
            descrPtr->modifyDscp = 1;
        }
        else
        if(nextHopPolicyActionPtr->routModifyDscp == 2)
        {
            descrPtr->modifyDscp = 0;
        }

        if(nextHopPolicyActionPtr->routModifyUp == 1)
        {
            descrPtr->modifyUp = 1;
        }
        else
        if(nextHopPolicyActionPtr->routModifyUp == 2)
        {
            descrPtr->modifyUp = 0;
        }
    }

    descrPtr->eVid = nextHopPolicyActionPtr->vid;

    /* TTL decremental */
    descrPtr->decTtl = nextHopPolicyActionPtr->decTtl;

    /* ARP index */
    descrPtr->arpPtr = nextHopPolicyActionPtr->arpDaIndex;

    descrPtr->routed = 1;
    descrPtr->doRouterHa = 1;

    /* Source-Id assignment */
    if(SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        smemRegFldGet(devObjPtr, SMEM_XCAT_UC_ROUT_ENG_CONF_REG(devObjPtr),0,5,&(descrPtr->sstId));
    }

    return;
}

/*******************************************************************************
*   snetChtIpv4HeaderError
*
* DESCRIPTION:
*    � Valid IPv4 header <Checksum>
*    � IPv4 header Version = 4
*    � IPv4 header IP Header Length = 5
*    � IPv4 header IP Header Length <= (IPv4 header Total Length / 4)
*    � IPv4 header IP Header Length + packet L3 byte offset + 4
*      CRC bytes <= packet Layer 2 byte count*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIpv4HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 ucRouteEngCfgReg
)
{
    DECLARE_FUNC_NAME(snetChtIpv4HeaderError);

    GT_STATUS status = GT_OK;
    GT_U32 fldVal;              /* register field's value */

    if (descrPtr->ipHeaderError)
    {
        status = GT_FAIL;
        /* The command assigned to routed IPv4 packets with Bad Header */
        fldVal = SMEM_U32_GET_FIELD(ucRouteEngCfgReg, 16, 1);
        if (fldVal == 1)
        {
            /* IPv4 with bad IP header are Trapped to the CPU */
            __LOG(("IPv4 with bad IP header are Trapped to the CPU"));
            descrPtr->cpuCode = SNET_CHT_IPV4_HEADER_ERROR;
        }
        else
        {
            status = GT_OK;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtIpv6HeaderError
*
* DESCRIPTION:
*    - IPv6 Header IP Version = 6
*    - IPv6 Payload Length + 40 bytes of IPv6 header + packet L3 byte offset +
*        4 CRC bytes <= packet Layer 2 byte count
*
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIpv6HeaderError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 ucRouteEngCfgReg
)
{
    DECLARE_FUNC_NAME(snetChtIpv6HeaderError);

    GT_STATUS status = GT_OK;
    GT_U32 fldVal;              /* register field's value */

    if (descrPtr->ipHeaderError)
    {
        /* The command assigned to routed IPv6 packets with Bad Header */
        fldVal = SMEM_U32_GET_FIELD(ucRouteEngCfgReg, 17, 1);
        if (fldVal == 1)
        {
            /* IPv6 with bad IP header are Trapped to the CPU */
            __LOG(("IPv6 with bad IP header are Trapped to the CPU"));
            descrPtr->cpuCode = SNET_CHT_IPV6_HEADER_ERROR;
        }
        else
        {
            status = GT_OK;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtIpv4TtlExceeded
*
* DESCRIPTION:
*    After the optional decrement, if the IPv4 packet TTL field is either
*   0 or 1 and Route<DecTTL> = 1, a TTL Exceeded exception is detected.
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIpv4TtlExceeded
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
)
{
    DECLARE_FUNC_NAME(snetChtIpv4TtlExceeded);

    GT_STATUS status = GT_OK;
    GT_U8 ttl;                  /* TTL/Hop Limit */
    GT_U32 fldVal;              /* register field's value */

    if (nextHopPolicyActionPtr->bypassTtlOptionCheck == 1)
    {
        return status;
    }

    /* If the TTL/Hop-Limit has reached 0 or TTL/Hop-Limit has reached 1 and decrement
       the TTL/Hop-Limit is enable, a TTL/Hop-Limit Exceeded exception occurs */
    ttl = descrPtr->l3StartOffsetPtr[8];
    if (ttl == 0 || (ttl == 1 && nextHopPolicyActionPtr->decTtl == 1))
    {
        status = GT_FAIL;
        descrPtr->ttl = ttl;
    }

    if (status == GT_FAIL)
    {
        /* The command assigned to routed IPv4 packets with TTL exceeded
           (either received with TTL = 1 and DecTTL = 1 or with TTL = 0) */
        fldVal = SMEM_U32_GET_FIELD(ucRouteEngCfgReg, 18, 1);
        if (fldVal == 1)
        {
            /* IPv4 With TTL exceeded are Trapped to the CPU */
            __LOG(("IPv4 With TTL exceeded are Trapped to the CPU"));
            descrPtr->cpuCode = SNET_CHT_IPV4_UC_TTL_EXCEEDED;
        }
        else
        {
            status = GT_OK;
        }

    }

    return status;
}

/*******************************************************************************
*   snetChtIpv6HopLimitExceeded
*
* DESCRIPTION:
*   If the Router PCL rule action has <TTL/Hop Limit Decrement Disable>=0,
*   the packet Hop Limit is decremented by 1 (but remains at zero if it
*   is already zero).
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIpv6HopLimitExceeded
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
)
{
    DECLARE_FUNC_NAME(snetChtIpv6HopLimitExceeded);

    GT_STATUS status = GT_OK;
    GT_U8 hopLimit;                 /* TTL/Hop Limit */
    GT_U32 fldVal;                  /* register field's value */

    if (nextHopPolicyActionPtr->bypassTtlOptionCheck == 1)
    {
        return status;
    }

    if (nextHopPolicyActionPtr->decTtl == 1)
    {
        /* Time To Live */
        __LOG(("Time To Live"));
        hopLimit = descrPtr->l3StartOffsetPtr[7];
        if (hopLimit == 0 || hopLimit == 1)
        {
            status = GT_FAIL;
        }

        descrPtr->ttl = hopLimit;
    }

    if (status == GT_FAIL)
    {
        /* The command assigned to routed IPv6 packets with Hop Limit exceeded
        (either received with HopLimit = 1 and DecTTL = 1 or with HopLimit = 0) */
        fldVal = SMEM_U32_GET_FIELD(ucRouteEngCfgReg, 19, 1);
        if (fldVal == 1)
        {
            /* IPv6 With Hop Limit exceeded are Trapped to the CPU */
            __LOG(("IPv6 With Hop Limit exceeded are Trapped to the CPU"));
            descrPtr->cpuCode = SNET_CHT_IPV6_UC_HOP_LIMIT_EXCEEDED;
        }
        else
        {
            status = GT_OK;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtIpv4Options
*
* DESCRIPTION:
*   If the IPv4 Header Length is greater than 5, the packet contains IPv4 options.
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIpv4Options
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
)
{
    DECLARE_FUNC_NAME(snetChtIpv4Options);

    GT_STATUS status = GT_OK;
    GT_U32 fldVal;              /* register field's value */
    GT_U32 ipHeaderLength;      /* ip header length */

    if (nextHopPolicyActionPtr->bypassTtlOptionCheck == 1)
    {
        return status;
    }

    ipHeaderLength = descrPtr->l3StartOffsetPtr[0] & 0xF;
    if (ipHeaderLength > 5)
    {
        status = GT_FAIL;
    }

    if (status == GT_FAIL)
    {
        /* The command assigned to routed IPv4 packets with header containing
           option fields*/
        fldVal = SMEM_U32_GET_FIELD(ucRouteEngCfgReg, 20, 1);
        if (fldVal == 1)
        {
            /* IPv4 packets with options are routed Trapped to the CPU. */
            __LOG(("IPv4 packets with options are routed Trapped to the CPU."));
            descrPtr->cpuCode = SNET_CHT_IPV4_UC_OPTIONS;
        }
        else
        {
            status = GT_OK;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtIpv6HopByHopOptHeader
*
* DESCRIPTION:
*   If the IPv6 packet contains a Hop-by-Hop Options header, a Hop-by-Hop
*   Options exception is detected.
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIpv6HopByHopOptHeader
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr,
    IN GT_U32 ucRouteEngCfgReg
)
{
    DECLARE_FUNC_NAME(snetChtIpv6HopByHopOptHeader);

    GT_STATUS status = GT_OK;
    GT_U32 payLoadLength;       /* Payload length */
    GT_U32 fldVal;              /* register field's value */

    if (nextHopPolicyActionPtr->bypassTtlOptionCheck == 1)
    {
        return status;
    }

    payLoadLength = (descrPtr->l3StartOffsetPtr[4] << 8) |
                     descrPtr->l3StartOffsetPtr[5];
    if (payLoadLength == 0)
    {
        status = GT_FAIL;
    }

    if (status == GT_FAIL)
    {
        /* The command assigned to routed IPv6 packets with Hop By Hop header */
        fldVal = SMEM_U32_GET_FIELD(ucRouteEngCfgReg, 21, 1);
        if (fldVal == 1)
        {
            /* IPv6 packets Hop By Hop header are Trapped to the CPU */
            __LOG(("IPv6 packets Hop By Hop header are Trapped to the CPU"));
            descrPtr->cpuCode = SNET_CHT_IPV6_UC_HOP_BY_HOP;
        }
        else
        {
            status = GT_OK;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtIcmpRedirectError
*
* DESCRIPTION:
*   An ICMP Redirect Error message is sent by a router to a host to inform it
*   of a better next hop router on the path to the destination.
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*       ucRouteEngCfgReg - UC engine configuration register entry
*
* OUTPUTS:
*
* RETURN:
*
*******************************************************************************/
static GT_STATUS snetChtIcmpRedirectError
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr
)
{
    GT_STATUS status = GT_OK;

    if (nextHopPolicyActionPtr->vid == descrPtr->eVid)
    {
        if (nextHopPolicyActionPtr->icmpRedirectChk == 1)
        {
            if (descrPtr->isIPv4)
            {
                descrPtr->cpuCode = SNET_CHT_IPV4_UC_ICMP_REDIRECT;
            }
            else
            {
                descrPtr->cpuCode = SNET_CHT_IPV6_UC_ICMP_REDIRECT;
            }

            status = GT_FAIL;
        }
    }

    return status;
}

/*******************************************************************************
*   snetChtNextHopPolicyActionGet
*
* DESCRIPTION:
*   Get Next Hop Policy Action (descrPtr->pclUcNextHopIndex in the TCAM memory)
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
*
* OUTPUTS:
*       nextHopPolicyActionPtr - (pointer to)nexthop policy action entry
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtNextHopPolicyActionGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHT_NEXT_HOP_POLICY_ACTION_STC * nextHopPolicyActionPtr
)
{
    DECLARE_FUNC_NAME(snetChtNextHopPolicyActionGet);

    GT_U32  *actionEntryMemPtr;/* (pointer to) action entry in memory */
    GT_BOOL isXcatFormat;/* is the entry format like xcat or not */
    GT_U32  bitsOffset0;/* number of bits to offset in word 0 */

    if(! SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
    {
        isXcatFormat = GT_FALSE;
        bitsOffset0 = 0;
    }
    else
    {
        isXcatFormat = GT_TRUE;
        bitsOffset0 = 3;/* bits from bit 12 in word 0 are shifted 3 bits */
    }

    ASSERT_PTR(nextHopPolicyActionPtr);

    actionEntryMemPtr = smemMemGet(devObjPtr,
        SMEM_CHT_PCL_ACTION_TBL_MEM(devObjPtr, descrPtr->pclUcNextHopIndex));

    /* word 0 of the action */
    nextHopPolicyActionPtr->matchCounterIndex =
        snetFieldValueGet(actionEntryMemPtr, 26 + bitsOffset0, 5);

    nextHopPolicyActionPtr->matchCounterEn =
        snetFieldValueGet(actionEntryMemPtr, 25 + bitsOffset0, 1);

    nextHopPolicyActionPtr->routModifyUp =
        snetFieldValueGet(actionEntryMemPtr, 23 + bitsOffset0, 2);

    nextHopPolicyActionPtr->routModifyDscp =
        snetFieldValueGet(actionEntryMemPtr, 21 + bitsOffset0, 2);

    nextHopPolicyActionPtr->routQosProfile =
        snetFieldValueGet(actionEntryMemPtr, 14 + bitsOffset0, 7);

    nextHopPolicyActionPtr->routQosProfileMarkingEn =
        snetFieldValueGet(actionEntryMemPtr, 13 + bitsOffset0, 1);

    nextHopPolicyActionPtr->routQosPrecedence =
        snetFieldValueGet(actionEntryMemPtr, 12 + bitsOffset0, 1);

    nextHopPolicyActionPtr->mirrorToIngressAnalyzerPort =
        snetFieldValueGet(actionEntryMemPtr, 11, 1);

    nextHopPolicyActionPtr->arpDaIndex =
        snetFieldValueGet(actionEntryMemPtr, 3, 8);

    nextHopPolicyActionPtr->fwdCmd =
        snetFieldValueGet(actionEntryMemPtr,0,3);

    /* word 1 of the action */
    nextHopPolicyActionPtr->vid =
        snetFieldValueGet(actionEntryMemPtr,32+ 19, 12);

    nextHopPolicyActionPtr->arpDaIndex |=
        (snetFieldValueGet(actionEntryMemPtr,32+ 17, 2) << 8);

    if(snetFieldValueGet(actionEntryMemPtr,32+ 14, 1))/* use vidx */
    {
        /* vidx */
        __LOG(("vidx"));
        nextHopPolicyActionPtr->routEgressInterface.useVidx = GT_TRUE;
        nextHopPolicyActionPtr->routEgressInterface.vidxInfo.vidx =
            (GT_U16)snetFieldValueGet(actionEntryMemPtr,32+2,12);
    }
    else
    {
        /* port/trunk */
        __LOG(("port/trunk"));
        nextHopPolicyActionPtr->routEgressInterface.useVidx = GT_FALSE;

        if(snetFieldValueGet(actionEntryMemPtr,32+ 2, 1))/* targetIsTrunk */
        {
            /* trunk */
            __LOG(("trunk"));
            nextHopPolicyActionPtr->
                routEgressInterface.vidxInfo.noVidx.targetIsTrunk = GT_TRUE;
            nextHopPolicyActionPtr->
            routEgressInterface.vidxInfo.noVidx.trunkInfo.targetTrunk =
                (GT_U16)snetFieldValueGet(actionEntryMemPtr,32+ 3, 7);
        }
        else
        {
            /* port */
            __LOG(("port"));
            nextHopPolicyActionPtr->
                routEgressInterface.vidxInfo.noVidx.targetIsTrunk = GT_FALSE;
            nextHopPolicyActionPtr->
                routEgressInterface.vidxInfo.noVidx.trunkInfo.noTrunk.targetDev =
                (GT_U8)snetFieldValueGet(actionEntryMemPtr,32+ 9, 5);

            nextHopPolicyActionPtr->
            routEgressInterface.vidxInfo.noVidx.trunkInfo.noTrunk.targetPort =
                (GT_U8)snetFieldValueGet(actionEntryMemPtr,32+ 3,6);
        }
    }

    if(isXcatFormat == GT_FALSE)
    {
        nextHopPolicyActionPtr->routRedirectCmd =
            snetFieldValueGet(actionEntryMemPtr,32+ 0, 2);
    }
    else
    {
        nextHopPolicyActionPtr->routRedirectCmd =
            snetFieldValueGet(actionEntryMemPtr,12, 2);
    }

    if(nextHopPolicyActionPtr->routRedirectCmd != 3)
    {
        skernelFatalError("snetChtNextHopPolicyActionGet : PBR command must be 3 to indicate a routed Entry \n");
    }


    /* word 2 of the action */
    nextHopPolicyActionPtr->decTtl =
        snetFieldValueGet(actionEntryMemPtr,64+ 3, 1);

    nextHopPolicyActionPtr->bypassTtlOptionCheck =
        snetFieldValueGet(actionEntryMemPtr,64+ 2, 1);

    nextHopPolicyActionPtr->icmpRedirectChk =
        snetFieldValueGet(actionEntryMemPtr,64+ 1, 1);

    return;
}

/*******************************************************************************
*   snetChtUcRouting
*
* DESCRIPTION:
*        (Cheetah) IPv4 and IPv6 Unicast Routing
*
*
* INPUTS:
*       devObjPtr    - (pointer to) device object.
*       descrPtr     - (pointer to) frame data buffer Id
* OUTPUTS:
*       descrPtr     - (pointer to) updated frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID  snetChtUcRouting
(
    SKERNEL_DEVICE_OBJECT * devObjPtr,
    SKERNEL_FRAME_CHEETAH_DESCR_STC *  descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtUcRouting);

    GT_BIT exceptBit;
    GT_BIT status;
    SNET_CHT_NEXT_HOP_POLICY_ACTION_STC nextHopPolicyAction;

    if(!descrPtr->isIp)
    {
        /* non ipv4/6 packets can not do this 'UC routing' engine */
        __LOG(("non ipv4/6 packets can not do this 'UC routing' engine"));
        return;
    }

    if (descrPtr->isIPv4)
    {
        status = descrPtr->ipV4ucvlan;
    }
    else
    {
        status = descrPtr->ipV6ucvlan;
    }

    if (status &&
        ((descrPtr->bridgeUcRoutEn == 1) ||
         ((descrPtr->trgDev == descrPtr->ownDev) &&  (descrPtr->trgEPort == 61))))
    {
        snetChtNextHopPolicyActionGet(devObjPtr, descrPtr, &nextHopPolicyAction);

        snetChtUcExeptionCheck(devObjPtr, descrPtr, &nextHopPolicyAction, &exceptBit);

        snetChtUcCommandResolve(devObjPtr, descrPtr, &nextHopPolicyAction, exceptBit);
    }
}



