/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simLogInfoTypePacket.c
*
* DESCRIPTION:
*       simulation logger packet functions
*
* FILE REVISION NUMBER:
*       $Revision: 61 $
*
*******************************************************************************/

#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>
#include <asicSimulation/SLog/simLogInfoTypeDevice.h>

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SCIB/scib.h>
/*#include <asicSimulation/SInit/sinit.h>*/

static GT_VOID internalLogPacketDump
(
    IN SKERNEL_DEVICE_OBJECT        const *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC const *descrPtr,
    IN GT_BOOL                      ingressDirection,
    IN GT_U32                       portNum,
    IN GT_U8                        *startFramePtr,
    IN GT_U32                       byteCount
);


/*Gets 'cookiePtr' of the thread*/
extern GT_PTR simOsTaskOwnTaskCookieGet(GT_VOID);

/* __LOG() for parameter with it's value */
#define __LOG_PARAM_NO_LOCATION_META_DATA(param)                \
    __LOG_NO_LOCATION_META_DATA(("[%s] = [0x%x] \n",            \
        #param ,/* name of the parameter (field name)   */      \
        (param)))/*value of the parameter (field value) */


/*
 * macro to compare packet descriptor
 */
#define SIM_LOG_PACKET_COMPARE_MAC(field)                                   \
    {                                                                       \
        if ( memcmp(&old->field, &new->field, sizeof(old->field)) != 0 )    \
        {                                                                   \
            if( GT_FALSE == *descrChangedPtr )                              \
                *descrChangedPtr = GT_TRUE;                                 \
            __LOG_NO_LOCATION_META_DATA(("0x%8.8x 0x%8.8x %s\n", old->field, new->field , #field)); \
        }                                                                   \
    }

#define SIM_LOG_PACKET_COMPARE_MAC____GT_U64(x) \
    SIM_LOG_PACKET_COMPARE_MAC(x.l[0]);         \
    SIM_LOG_PACKET_COMPARE_MAC(x.l[1])

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_TOD_TIMER_STC(x)     \
    SIM_LOG_PACKET_COMPARE_MAC(x.fractionalNanoSecondTimer);    \
    SIM_LOG_PACKET_COMPARE_MAC(x.nanoSecondTimer);              \
    SIM_LOG_PACKET_COMPARE_MAC____GT_U64(x.secondTimer)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_TIMESTAMP_TAG_INFO_STC(x)    \
    SIM_LOG_PACKET_COMPARE_MAC(x.U);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.T);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.OffsetProfile);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.OE);                                   \
    SIM_LOG_PACKET_COMPARE_MAC____SNET_TOD_TIMER_STC(x.timestamp)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_TIMESTAMP_ACTION_INFO_STC(x) \
    SIM_LOG_PACKET_COMPARE_MAC(timestampActionInfo.action);             \
    SIM_LOG_PACKET_COMPARE_MAC(timestampActionInfo.packetFormat);       \
    SIM_LOG_PACKET_COMPARE_MAC(timestampActionInfo.transportType);      \
    SIM_LOG_PACKET_COMPARE_MAC(timestampActionInfo.offset);             \
    SIM_LOG_PACKET_COMPARE_MAC(timestampActionInfo.ptpMessageType);     \
    SIM_LOG_PACKET_COMPARE_MAC(timestampActionInfo.ptpDomain)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC(x)    \
    SIM_LOG_PACKET_COMPARE_MAC(x.drop);                                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyUp);                              \
    SIM_LOG_PACKET_COMPARE_MAC(x.up);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyDscp);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.dscp);                                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyVid);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.vid);                                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyExp);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.exp);                                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyTc);                              \
    SIM_LOG_PACKET_COMPARE_MAC(x.tc);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyDp);                              \
    SIM_LOG_PACKET_COMPARE_MAC(x.dp);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.vlan1Cmd);                              \
    SIM_LOG_PACKET_COMPARE_MAC(x.vid1);                                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyUp1);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.up1)

#define SIM_LOG_PACKET_COMPARE_MAC____SKERNEL_E_ARCH_EXT_INFO_STC(x)            \
    SIM_LOG_PACKET_COMPARE_MAC(x.vidx);                                         \
    SIM_LOG_PACKET_COMPARE_MAC(x.trgPhyPort);                                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.isTrgPhyPortValid);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.defaultSrcEPort);                              \
    SIM_LOG_PACKET_COMPARE_MAC(x.localDevSrcEPort);                             \
                                                                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.toTargetSniffInfo.sniffUseVidx);               \
    SIM_LOG_PACKET_COMPARE_MAC(x.toTargetSniffInfo.sniffEVidx);                 \
    SIM_LOG_PACKET_COMPARE_MAC(x.toTargetSniffInfo.sniffTrgEPort);              \
    SIM_LOG_PACKET_COMPARE_MAC(x.toTargetSniffInfo.sniffVidx);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.toTargetSniffInfo.sniffisTrgPhyPortValid);     \
                                                                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.srcTrgEPort);                                  \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.ttiPhysicalPortAttributePtr);*/              \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.ttiPreTtiLookupIngressEPortTablePtr);*/      \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.ttiPostTtiLookupIngressEPortTablePtr);*/     \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.bridgeIngressEPortTablePtr);*/               \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.bridgeIngressTrunkTablePtr);*/               \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.ipvxIngressEPortTablePtr);*/                 \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.ipvxIngressEVlanTablePtr);*/                 \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.eqIngressEPortTablePtr);*/                   \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.txqEgressEPortTablePtr);*/                   \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.haEgressEPortAtt1TablePtr);*/                \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.haEgressEPortAtt2TablePtr);*/                \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.haEgressPhyPortTablePtr);*/                  \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.epclEgressEPortTablePtr);*/                  \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.egfShtEgressClass); this changed per port in bmp of ports so don't care in compare !!! */\
                                                                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.haEgressEPortAtt1Table_index);                 \
    SIM_LOG_PACKET_COMPARE_MAC(x.haEgressEPortAtt2Table_index);                 \
                                                                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.eqInfo.eqIngressEPort);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.eqInfo.IN_descTrgIsTrunk);                     \
    SIM_LOG_PACKET_COMPARE_MAC(x.eqInfo.IN_descTrgEPort);                       \
                                                                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.fromCpu.excludedIsPhyPort);                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.assignTrgEPortAttributesLocally);              \
                                                                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.tunnelStart);                           \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.tunnelStartPointer);                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.ePortModifyMacSa);                      \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.ePortModifyMacDa);                      \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.arpPointer);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.doubleTagToCpu);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.forceNewDsaFwdFromCpu);                 \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.rxTrappedOrMonitored);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.dsaReplacesVlan);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.stripL2);                               \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.tpidIndex_atStartOfHa[SNET_CHT_TAG_0_INDEX_CNS]);\
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.tpidIndex_atStartOfHa[SNET_CHT_TAG_1_INDEX_CNS]);\
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.dsa_forword.dsa_tag0_Src_Tagged);       \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.dsa_forword.dsa_tag0_is_outer_tag);     \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.dsa_forword.dsa_tag1_Src_Tagged);       \
    SIM_LOG_PACKET_COMPARE_MAC(x.haInfo.dsa_forword.dsa_tpid_index);            \
    SIM_LOG_PACKET_COMPARE_MAC(x.packetIsTT);                                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.origSrcPhyIsTrunk);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.origSrcPhyPortTrunk);                          \
    SIM_LOG_PACKET_COMPARE_MAC(x.phySrcMcFilterEn);

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_PER_PROTOCOL_INFO_STC(x) \
    SIM_LOG_PACKET_COMPARE_MAC(x.portProtMatch);                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.portProtMatchedMemoryPointer)

#define SIM_LOG_PACKET_COMPARE_MAC____SKERNEL_HA_TO_EPCL_INFO_STC(x)    \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.macDaSaPtr);*/                       \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.outerVlanTagPtr);*/                  \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.timeStampTagPtr);*/                  \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.vlanTag0Ptr);*/                      \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.vlanTag1Ptr);*/                      \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.payloadPtr);*/                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.xponChanges);                          \
    SIM_LOG_PACKET_COMPARE_MAC(x.xPonVid);                              \
    SIM_LOG_PACKET_COMPARE_MAC(x.modifyVid);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.outerVid);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.prePendLength);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.evbBpeRspanTagSize);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.qos.qosMapTableIndex);                 \
    SIM_LOG_PACKET_COMPARE_MAC(x.qos.egressTcDpMapEn);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.qos.egressUpMapEn);                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.qos.egressDscpMapEn);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.qos.egressExpMapEn);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.qos.egressDpToCfiMapEn);               \
    SIM_LOG_PACKET_COMPARE_MAC(x.epclKeyVid);


#define SIM_LOG_PACKET_COMPARE_MAC____SNET_TRILL_INFO_STC(x)        \
    SIM_LOG_PACKET_COMPARE_MAC(x.trillCpuCodeBase);                 \
    SIM_LOG_PACKET_COMPARE_MAC(x.trillMcDescriptorInstance);        \
    SIM_LOG_PACKET_COMPARE_MAC(x.V);                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.M);                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.opLength);                         \
    SIM_LOG_PACKET_COMPARE_MAC(x.hopCount);                         \
    SIM_LOG_PACKET_COMPARE_MAC(x.eRbid);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.iRbid);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.CHbH);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.CItE)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_EGRESS_PHYSICAL_PORT_INFO_STC(x) \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.txDmaDevObjPtr);*/                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.txDmaMacPort);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.localPortIndex);                           \
    SIM_LOG_PACKET_COMPARE_MAC(x.globalPortIndex)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_INGRESS_DSA_STC(x)   \
    SIM_LOG_PACKET_COMPARE_MAC(x.origIsTrunk);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.origSrcEPortOrTrnk);           \
    SIM_LOG_PACKET_COMPARE_MAC(x.srcDev);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.qosProfile);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.srcId);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.dsaWords[0]);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.dsaWords[1]);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.dsaWords[2]);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.dsaWords[3]);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.fromCpu_egressFilterEn)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_INGRESS_TUNNEL_INFO_STC(x) \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.innerFrameDescrPtr);*/             \
    SIM_LOG_PACKET_COMPARE_MAC(x.innerPacketL2FieldsAreValid);        \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.innerMacDaPtr);*/                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.innerTag0Exists);                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.innerPacketTag0Vid)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_LION_PTP_GTS_INFO_STC(x)     \
    SIM_LOG_PACKET_COMPARE_MAC(x.ptpPacketTriggered);                   \
    /*SIM_LOG_PACKET_COMPARE_MAC(x.ptpMessageHeaderPtr);*/              \
    SIM_LOG_PACKET_COMPARE_MAC____SNET_LION_GTS_ENTRY_STC(x.gtsEntry)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_LION_GTS_ENTRY_STC(x)    \
    SIM_LOG_PACKET_COMPARE_MAC(x.ptpVersion);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.seqID);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.msgType);                          \
    SIM_LOG_PACKET_COMPARE_MAC(x.transportSpecific);                \
    SIM_LOG_PACKET_COMPARE_MAC(x.srcTrgPort);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.V2DomainNumber);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.V1Subdomain[0]);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.V1Subdomain[1]);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.V1Subdomain[2]);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.V1Subdomain[3]);

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_L2I_VLAN_INFO(x)         \
    SIM_LOG_PACKET_COMPARE_MAC(x.valid);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.unknownIsNonSecurityEvent);                \
    SIM_LOG_PACKET_COMPARE_MAC(x.portIsMember);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.spanState);                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.unregNonIpMcastCmd);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.unregIPv4McastCmd);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.unregIPv6McastCmd);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.unknownUcastCmd);                          \
    SIM_LOG_PACKET_COMPARE_MAC(x.unregNonIp4BcastCmd);                      \
    SIM_LOG_PACKET_COMPARE_MAC(x.unregIp4BcastCmd);                         \
    SIM_LOG_PACKET_COMPARE_MAC(x.igmpTrapEnable);                           \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv4IpmBrgEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv4IpmBrgMode);                           \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv6IpmBrgEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv6IpmBrgMode);                           \
    SIM_LOG_PACKET_COMPARE_MAC(x.ingressMirror);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.icmpIpv6TrapMirror);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipInterfaceEn);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv4UcRoutEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv6UcRoutEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.mruIndex);                                 \
    SIM_LOG_PACKET_COMPARE_MAC(x.bcUdpTrapMirrorEn);                        \
    SIM_LOG_PACKET_COMPARE_MAC(x.autoLearnDisable);                         \
    SIM_LOG_PACKET_COMPARE_MAC(x.naMsgToCpuEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipV6InterfaceEn);                          \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipV6SiteID);                               \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv4McRoutEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv6McRoutEn);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.floodVidxMode);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.floodVidx);                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.ucLocalEn);                                \
    SIM_LOG_PACKET_COMPARE_MAC(x.vrfId);                                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.unknownSaCmd);                             \
    SIM_LOG_PACKET_COMPARE_MAC(x.analyzerIndex);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv4McBcMirrorToAnalyzerIndex);            \
    SIM_LOG_PACKET_COMPARE_MAC(x.ipv6McMirrorToAnalyzerIndex);              \
    SIM_LOG_PACKET_COMPARE_MAC(x.fid);

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_EGR_VLAN_INFO(x)     \
    SIM_LOG_PACKET_COMPARE_MAC(x.mcLocalEn);                            \
    SIM_LOG_PACKET_COMPARE_MAC(x.portIsolationVlanCmd)

#define SIM_LOG_PACKET_COMPARE_MAC____SNET_QOS_INFO_STC(x)          \
    SIM_LOG_PACKET_COMPARE_MAC(x.qosProfile);                     \
    SIM_LOG_PACKET_COMPARE_MAC(x.ingressExtendedMode);            \
    SIM_LOG_PACKET_COMPARE_MAC(x.egressExtendedMode);             \
    SIM_LOG_PACKET_COMPARE_MAC(x.fromCpuQos.contolTc);            \
    SIM_LOG_PACKET_COMPARE_MAC(x.fromCpuQos.fromCpuTc);           \
    SIM_LOG_PACKET_COMPARE_MAC(x.fromCpuQos.fromCpuDp)

#define SIM_LOG_PACKET_COMPARE_MAC____SKERNEL_OAM_INFO_STC(x)   \
    SIM_LOG_PACKET_COMPARE_MAC(x.oamProfile);                   \
    SIM_LOG_PACKET_COMPARE_MAC(x.opCode);                       \
    SIM_LOG_PACKET_COMPARE_MAC(x.megLevel);                     \
    SIM_LOG_PACKET_COMPARE_MAC(x.oamProcessEnable);             \
    SIM_LOG_PACKET_COMPARE_MAC(x.oamEgressProcessEnable);       \
    SIM_LOG_PACKET_COMPARE_MAC(x.lmCounterCaptureEnable);       \
    SIM_LOG_PACKET_COMPARE_MAC(x.lmCounterInsertEnable);        \
    SIM_LOG_PACKET_COMPARE_MAC(x.lmCounter);                    \
    SIM_LOG_PACKET_COMPARE_MAC(x.timeStampEnable);              \
    SIM_LOG_PACKET_COMPARE_MAC(x.timeStampTagged);              \
    SIM_LOG_PACKET_COMPARE_MAC(x.offsetIndex);                  \
    SIM_LOG_PACKET_COMPARE_MAC____GT_U64(x.hashIndex);          \
    SIM_LOG_PACKET_COMPARE_MAC(x.oamTxPeriod);                  \
    SIM_LOG_PACKET_COMPARE_MAC(x.oamRdi)

static GT_VOID simLogPacketDescrCompareFields
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *old,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *new,
    OUT GT_BOOL                         *descrChangedPtr
);

/* compare IP addr */
static GT_BOOL simLogPacketDescrIpCompare
(
    IN SKERNEL_DEVICE_OBJECT const *devObjPtr,
    IN GT_U32                const *old,
    IN GT_U32                const *new,
    IN GT_CHAR               const *name
)
{
    GT_U32  ii;
    GT_U32  oldIpAddr;
    GT_U32  newIpAddr;
    GT_BOOL changed = GT_FALSE;

    for(ii = 0 ; ii < 4 ; ii++)
    {
        oldIpAddr = old[ii];
        newIpAddr = new[ii];


        if(oldIpAddr == newIpAddr)
        {
            continue;
        }

        changed = GT_TRUE;

        __LOG_NO_LOCATION_META_DATA((
           "%03d.%03d.%03d.%03d   %03d.%03d.%03d.%03d   %s[%d]\n",
           (oldIpAddr >> 24) & 0xff, (oldIpAddr >> 16) & 0xff, (oldIpAddr >> 8) & 0xff, (oldIpAddr & 0xff),
           (newIpAddr >> 24) & 0xff, (newIpAddr >> 16) & 0xff, (newIpAddr >> 8) & 0xff, (newIpAddr & 0xff),
           name , ii));
    }

    return changed;
}

/* compare mac addr */
static GT_BOOL simLogPacketDescrMacCompare
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN GT_U8                 *old,
    IN GT_U8                 *new,
    IN GT_CHAR               *name
)
{
    GT_U8   tmpMacDa[6];
    GT_U8   *tmpMacDaPtr;

    if(new == NULL)
    {
        return GT_FALSE;
    }

    if(old)
    {
        tmpMacDaPtr = old;
    }
    else
    {
        /* allow to compare 'old' that was not exists before last 'save' of 'old' */
        tmpMacDaPtr = &tmpMacDa[0];
        memset(tmpMacDa, 0, sizeof(tmpMacDa));
    }

    if ( (memcmp(tmpMacDaPtr, new, 6) ) != 0)
    {
        __LOG_NO_LOCATION_META_DATA((
              "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x  "
              "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x  "
              "%s\n",
               tmpMacDaPtr[0], tmpMacDaPtr[1], tmpMacDaPtr[2], tmpMacDaPtr[3], tmpMacDaPtr[4], tmpMacDaPtr[5],
               new[0], new[1], new[2], new[3], new[4], new[5],
               name));

        return GT_TRUE;
    }

    return GT_FALSE;
}

/* print packetCmd enum value */
extern GT_VOID simLogPacketDescrPacketCmdDump
(
    IN SKERNEL_DEVICE_OBJECT const *devObjPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT packetCmd
)
{
    GT_CHAR *str = NULL;

    switch(packetCmd)
    {
        case SKERNEL_EXT_PKT_CMD_FORWARD_E:
            str = "CMD_FORWARD";
            break;
        case SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E:
            str = "CMD_MIRROR_TO_CPU";
            break;
        case SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E:
            str = "CMD_TRAP_TO_CPU";
            break;
        case SKERNEL_EXT_PKT_CMD_HARD_DROP_E:
            str = "CMD_HARD_DROP";
            break;
        case SKERNEL_EXT_PKT_CMD_SOFT_DROP_E:
            str = "CMD_SOFT_DROP";
            break;
        case SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E:
            str = "CMD_TO_TRG_SNIFFER";
            break;
        case SKERNEL_EXT_PKT_CMD_FROM_CSCD_TO_CPU_E:
            str = "CMD_FROM_CSCD_TO_CPU";
            break;
        case SKERNEL_EXT_PKT_CMD_FROM_CPU_E:
            str = "CMD_FROM_CPU";
            break;
        default:
            skernelFatalError("simLogPacketDescrPacketCmdDump: unknown packetCmd\n");
    }

    __LOG_NO_LOCATION_META_DATA(("%s ", str));
}

/* compare packetCmd */
static GT_BOOL simLogPacketDescrPacketCmdCompare
(
    IN SKERNEL_DEVICE_OBJECT      const *devObjPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT const old,
    IN SKERNEL_EXT_PACKET_CMD_ENT const new
)
{
    if (old != new)
    {
        simLogPacketDescrPacketCmdDump(devObjPtr, old);
        simLogPacketDescrPacketCmdDump(devObjPtr, new);
        __LOG_NO_LOCATION_META_DATA(("packetCmd \n"));
        return GT_TRUE;
    }
    return GT_FALSE;
}

/* print cpu code enum value */
extern GT_VOID simLogPacketDescrCpuCodeDump
(
    IN SKERNEL_DEVICE_OBJECT const *devObjPtr,
    IN SNET_CHEETAH_CPU_CODE_ENT    cpuCode
)
{
    GT_CHAR *str;

    switch(cpuCode)
    {
        SWITCH_CASE_NAME_MAC(SNET_CHT_BPDU_TRAP                                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_MAC_TABLE_ENTRY_TRAP_MIRROR                        ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_MAC_SA_MOVED                        ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_ARP_BCAST_TRAP_MIRROR                              ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_IGMP_TRAP_MIRROR                              ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_SA_IS_DA                            ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_TRAP                ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_SIP_IS_DIP                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRG_LEARN_DIS_UNK_SRC_MAC_ADDR_MIRROR              ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_UDP_SPORT_IS_DPORT              ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_WITH_FIN_WITHOUT_ACK      ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IEEE_RES_MCAST_ADDR_TRAP_MIRROR                    ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_ICMP_TRAP_MIRROR                              ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_WITHOUT_FULL_HEADER             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_IPV6_LINK_LOCAL_MCAST_DIP_TRAP_MIRROR         ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_RIPV1_MIRROR                                  ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_NEIGHBOR_SOLICITATION_TRAP_MIRROR             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_BCAST_TRAP_MIRROR                             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_NON_IPV4_BCAST_TRAP_MIRROR                         ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_CISCO_CONTROL_MCAST_MAC_ADDR_TRAP_MIRROR           ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRG_NON_IPV4_IPV6_UNREG_MCAST_TRAP_MIRROR          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRG_IPV4_UNREG_MCAST_TRAP_MIRROR                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRG_IPV6_UNREG_MCAST_TRAP_MIRROR                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRG_UNKN_UCAST_TRAP_MIRROR                         ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_1                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_2                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IEEE_RES_MC_ADDR_TRAP_MIRROR_3                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_UDP_BC_TRAP_MIRROR0                                ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_UDP_BC_TRAP_MIRROR1                                ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_UDP_BC_TRAP_MIRROR2                                ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_UDP_BC_TRAP_MIRROR3                                ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_SEC_AUTO_LEARN_UNK_SRC_TRAP                        ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_VLAN_NOT_VALID                      ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_PORT_NOT_VLAN_MEM                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_VLAN_RANGE                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_STATIC_ADDR_MOVED                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_MAC_SPOOF                           ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_ARP_SA_MISMATCH                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_SYN_WITH_DATA                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_OVER_MC_BC                      ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_FRAGMENT_ICMP                       ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_ZERO                      ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_FIN_URG_PSH               ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_FIN                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_FLAGS_SYN_RST                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_TCP_UDP_SRC_DEST_ZERO               ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_BRIDGE_ACCESS_MATRIX                ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_ACCEPT_FRAME_TYPE                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_VLAN_MRU                            ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_RATE_LIMITING                       ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_LOCAL_PORT                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_IPMC                                ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_NON_IPMC                            ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_DSA_TAG_LOCAL_DEV                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_L2I_INVALID_SA                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_LION3_CPU_CODE_IPV4_6_MC_BIDIR_RPF_FAIL                ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_ROUTED_PACKET_FORWARD                              ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_BRIDGED_PACKET_FORWARD                             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_INGRESS_MIRRORED_TO_ANALYZER                       ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_EGRESS_MIRRORED_TO_ANALYZER                        ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_MAIL_FROM_NEIGHBOR_CPU                             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_CPU_TO_CPU                                         ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_EGRESS_SAMPLED                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_INGRESS_SAMPLED                                    ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_TT_HEADER_ERROR                               ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_TT_OPTION_FRAG_ERROR                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_TT_UNSUP_GRE_ERROR                            ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_EQ_BAD_ANALYZER_INDEX_DROP_ERROR                   ,str);
        SWITCH_CASE_NAME_MAC(SNET_XCAT_MPLS_TT_HEADER_CHECK_ERROR                        ,str);
        SWITCH_CASE_NAME_MAC(SNET_XCAT_MPLS_TT_LSR_TTL_EXCEEDED                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_XCAT_OAM_PDU_TRAP                                      ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_TTL_EXCEEDED                               ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_MTU_EXCEEDED                                ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_HOP_LIMIT_EXCEEDED                         ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_ILLEGAL_ADDR_ERROR                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_HEADER_ERROR                                  ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_DIP_DA_MISMATCH                             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_HEADER_ERROR                                  ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_SIP_SA_MISMATCH                             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_OPTIONS                                    ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_NON_HBH_OPTIONS                               ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_HOP_BY_HOP                                 ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_SIP_FILTERING                               ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_SIP_IS_ZERO                                 ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_ACCESS_MATRIX                                      ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_FCOE_EXCEPTION                                     ,str);
      /*SWITCH_CASE_NAME_MAC(SNET_CHT_FCOE_DIP_LOOKUP_NOT_FOUND                          ,str);*/
        SWITCH_CASE_NAME_MAC(SNET_CHT_FCOE_SIP_LOOKUP_NOT_FOUND                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_SCOPE                                         ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_ROUTE0                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_ROUTE1                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_ROUTE2                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_ROUTE3                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_MC_ROUTE0                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_MC_ROUTE1                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_MC_ROUTE2                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_MC_ROUTE3                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_ROUTE0                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_ROUTE1                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_ROUTE2                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_ROUTE3                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_MC_ROUTE0                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_MC_ROUTE1                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_MC_ROUTE2                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_MC_ROUTE3                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_UC_RPF_FAIL                                 ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_MC_RPF_FAIL                                 ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_6_MC_MLL_RPF_FAIL                             ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_ARP_BC_TO_ME                                       ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV4_UC_ICMP_REDIRECT                              ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_UC_ICMP_REDIRECT                              ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_TT_HEADER_ERROR                               ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_CAPWAP_802_11_MGMT_EXCEPTION                       ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_IPV6_TT_OPTION_FRAG_ERROR                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_CAPWAP_FRAGMENT_EXCEPTION                          ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_ARP_REPLY_TO_ME                                    ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_CPU_TO_ALL_CPUS                                    ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_TCP_SYN_TO_CPU                                     ,str);
        SWITCH_CASE_NAME_MAC(SNET_CHT_PACKET_TO_VIRTUAL_ROUTER_PORT                      ,str);
        default:
            str = "unknown";
            break;
    }

    __LOG_NO_LOCATION_META_DATA(("%s(0x%x) ", str,cpuCode));
}

/* compare packetCmd */
static GT_BOOL simLogPacketDescrCpuCodeCompare
(
    IN SKERNEL_DEVICE_OBJECT      const *devObjPtr,
    IN SNET_CHEETAH_CPU_CODE_ENT const old,
    IN SNET_CHEETAH_CPU_CODE_ENT const new
)
{
    if (old == new)
    {
        return GT_FALSE;
    }

    simLogPacketDescrCpuCodeDump(devObjPtr, old);
    simLogPacketDescrCpuCodeDump(devObjPtr, new);
    __LOG_NO_LOCATION_META_DATA(("cpuCode \n"));
    return GT_TRUE;
}

/*******************************************************************************
* simLogPacketDescrFrameDump
*
* DESCRIPTION:
*       log frame dump
*
* INPUTS:
*       devObjPtr -  pointer to device object
*       descr     -  packet descriptor pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID simLogPacketDescrFrameDump
(
    IN SKERNEL_DEVICE_OBJECT           const *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC const *descrPtr
)
{
    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    internalLogPacketDump(devObjPtr,
        descrPtr,
        GT_TRUE ,/* direction : ingress */
        descrPtr->localDevSrcPort,
        descrPtr->startFramePtr,
        descrPtr->byteCount);
}

/*******************************************************************************
* internalLogPacketDump
*
* DESCRIPTION:
*       log frame dump
*
* INPUTS:
*       devObjPtr -  pointer to device object
*       descr     -  packet descriptor pointer --> CAN be NULL !!!
*       ingressDirection - indication for direction:
*                   GT_TRUE - ingress
*                   GT_FALSE - egress
*       portNum - port number of ingress/egress
*       startFramePtr -  start of packet
*       byteCount - number of bytes to dump
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID internalLogPacketDump
(
    IN SKERNEL_DEVICE_OBJECT        const *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC const *descrPtr,
    IN GT_BOOL                      ingressDirection,
    IN GT_U32                       portNum,
    IN GT_U8                        *startFramePtr,
    IN GT_U32                       byteCount
)
{
    GT_BIT  packetFromSlan;
    GT_CHAR* ingressSlanName;

    GT_U32  maxByteCount = 256;

    if(!simLogIsOpen())
    {
        return ;
    }

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(startFramePtr);

    /* lock the waiting operations */
    scibAccessLock();


    if(descrPtr &&
       ingressDirection == GT_TRUE &&
       descrPtr->frameBuf->srcType == SMAIN_SRC_TYPE_SLAN_E)
    {
        /* should get here only once per packet that actually cam from slan */
        /* handle of packets from slan bound to 'cascade port' ---> should use 'internal ports' and not 'slan' */

        packetFromSlan = 1;
    }
    else
    {
        packetFromSlan = 0;
    }

    if(packetFromSlan)
    {
        if(portNum < devObjPtr->numSlans)
        {
            ingressSlanName = &devObjPtr->portSlanInfo[portNum].slanName[0];
        }
        else
        {
            ingressSlanName = NULL;
        }

        /* we got packet from 'outside' the device */
        __LOG_NO_LOCATION_META_DATA((
            SIM_LOG_INGRESS_PACKET_FROM_SLAN_STR "%s] \n"
            "dump packet : device[%d] core[%d] %s port [%d] byteCount[%d]\n",
            (ingressSlanName ? ingressSlanName : "unknown") ,
            devObjPtr->deviceId,
            devObjPtr->portGroupId,
            ingressDirection == GT_TRUE ? "ingress" : "egress" ,
            portNum,
            byteCount));
    }
    else
    {
        __LOG_NO_LOCATION_META_DATA((
            "dump packet : device[%d] core[%d] %s port [%d] byteCount[%d]\n",
            devObjPtr->deviceId,
            devObjPtr->portGroupId,
            ingressDirection == GT_TRUE ? "ingress" : "egress" ,
            portNum,
            byteCount));
    }

    if(byteCount > maxByteCount && (packetFromSlan == 0))
    {
        __LOG_NO_LOCATION_META_DATA((
            "dump only first %d bytes not[%d]\n",
            maxByteCount, byteCount));

        byteCount = maxByteCount;
    }
    else
    {
        __LOG_NO_LOCATION_META_DATA(("\n"));
    }

    simLogDump(devObjPtr, SIM_LOG_INFO_TYPE_PACKET_E,
               (GT_PTR)startFramePtr, byteCount);
    __LOG_NO_LOCATION_META_DATA((SIM_LOG_END_OF_PACKET_DUMP_STR " \n"));

    /* UnLock the waiting operations */
    scibAccessUnlock();
}

/*******************************************************************************
* simLogPacketDump
*
* DESCRIPTION:
*       log frame dump
*
* INPUTS:
*       devObjPtr -  pointer to device object
*       ingressDirection - indication for direction:
*                   GT_TRUE - ingress
*                   GT_FALSE - egress
*       portNum - port number of ingress/egress
*       startFramePtr -  start of packet
*       byteCount - number of bytes to dump
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID simLogPacketDump
(
    IN SKERNEL_DEVICE_OBJECT        const *devObjPtr,
    IN GT_BOOL                      ingressDirection,
    IN GT_U32                       portNum,
    IN GT_U8                        *startFramePtr,
    IN GT_U32                       byteCount
)
{
    internalLogPacketDump(devObjPtr,NULL,ingressDirection,portNum,startFramePtr,byteCount);
}

/*  check if the device if filtered from the LOG ...
    in this case do not put any descriptor change indication */
static GT_BOOL isDevFiltered(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr
)
{
    GT_U32  i;
    GT_U32      deviceId = devObjPtr->deviceId;
    GT_U32      portGroupId = devObjPtr->portGroupId;
    /* dev - port group filter */
    for(i = 0; i != SIM_LOG_MAX_DF; i++)
    {
        /* search filter in array */
        if( simLogDevPortGroupFilters[i].filterId &&
           (simLogDevPortGroupFilters[i].devNum    == deviceId) &&
           (simLogDevPortGroupFilters[i].portGroup == portGroupId) )
        {
            /* filter message */
            return GT_TRUE;
        }
    }

    return GT_FALSE;
}

/*******************************************************************************
* descrCompare
*
* DESCRIPTION:
*       log changes between saved packet descriptor and given
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       descTypeName - pointer to 'descriptor type' name
*       old       - old packet descriptor pointer
*       new       - new packet descriptor pointer
*       funcName  - pointer to function name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       did the compare found changes.
*       GT_TRUE  - the compare found changes.
*       GT_FALSE - the compare NOT found changes.
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL descrCompare
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN GT_CHAR                         *descTypeName,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *old,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *new,
    IN GT_CHAR                         *funcName
)
{
    GT_BOOL descrChanged = GT_FALSE;

    if(descTypeName)
    {
        __LOG_NO_LOCATION_META_DATA(("**************** %30s packet %s descr changes:\n", funcName,descTypeName));
    }
    else
    {
        __LOG_NO_LOCATION_META_DATA(("**************** %30s packet descr changes:\n", funcName));
    }

    simLogPacketDescrCompareFields(devObjPtr, old, new, &descrChanged);

    if( GT_FALSE == descrChanged )
    {
        __LOG_NO_LOCATION_META_DATA(("NONE\n"));
    }
    __LOG_NO_LOCATION_META_DATA(("\n"));

    return descrChanged;
}


/*******************************************************************************
* stateImportantInfo
*
* DESCRIPTION:
*       add to log important info that we would like to see beside 'Changed values' in the descriptor.
*       this info keep us 'focused' on destination of the descriptor:
*       target {port,device / trunkid / vidx} , packet command , vid , is_vidx ,
*       is_trunk , trunkid
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       old       - old packet descriptor pointer
*       new       - new packet descriptor pointer
*       funcName  - pointer to function name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       warning: function uses lock the waiting operations
*
*******************************************************************************/
static GT_VOID stateImportantInfo(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    if(descrPtr->packetTimestamp == 0)
    {
        /* at this stage there is no target info and not full source info ... */
        return;
    }

    __LOG_NO_LOCATION_META_DATA(("^^^^^^^ Start of Current basic descriptor parameters \n"));

    {
        __LOG_NO_LOCATION_META_DATA(("packetCmd:"));
        simLogPacketDescrPacketCmdDump(devObjPtr,descrPtr->packetCmd);
        __LOG_NO_LOCATION_META_DATA(("\n"));
    }

    __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eVid);

    __LOG_NO_LOCATION_META_DATA(("target"));
    if(descrPtr->useVidx == 0)
    {
        if(descrPtr->targetIsTrunk == 0)
        {
            __LOG_NO_LOCATION_META_DATA((" {port,device}: \n"));
            __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->trgEPort);
            __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->trgDev);

            if(devObjPtr->supportEArch)
            {
                __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eArchExtInfo.isTrgPhyPortValid);
                if(descrPtr->eArchExtInfo.isTrgPhyPortValid)
                {
                    __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eArchExtInfo.trgPhyPort);
                }
            }
        }
        else
        {
            __LOG_NO_LOCATION_META_DATA((" trunk: \n"));
            __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->trgTrunkId);
        }
    }
    else
    {
        __LOG_NO_LOCATION_META_DATA((" VIDX: \n"));
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eVidx);
        if(devObjPtr->supportEArch)
        {
            __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eArchExtInfo.vidx);
        }
    }

    if(descrPtr->bypassBridge)
    {
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->bypassBridge);
    }

    if(descrPtr->bypassIngressPipe)
    {
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->bypassIngressPipe);
    }

    if(descrPtr->tunnelStart)
    {
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->tunnelStartType);
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->tunnelStartPassengerType);
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->tunnelPtr);
    }

    if(descrPtr->arpPtr)
    {
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->arpPtr);
    }

    /*__LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->egressPhysicalPortInfo.localPortIndex);*/

    __LOG_NO_LOCATION_META_DATA(("source info:\n"));

    __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->localDevSrcPort);
    __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->origIsTrunk);
    __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->origSrcEPortOrTrnk);
    __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->srcDev);

    if(devObjPtr->portGroupSharedDevObjPtr)
    {
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->srcCoreId);
    }

    if(devObjPtr->supportEArch)
    {
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eArchExtInfo.localDevSrcEPort);
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eArchExtInfo.origSrcPhyIsTrunk);
        __LOG_PARAM_NO_LOCATION_META_DATA(descrPtr->eArchExtInfo.origSrcPhyPortTrunk);
    }

    __LOG_NO_LOCATION_META_DATA(("End of descriptor parameters \n"));
}

/*******************************************************************************
* simLogPacketDescrCompare
*
* DESCRIPTION:
*       log changes between saved packet descriptor and given
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       old       - old packet descriptor pointer
*       new       - new packet descriptor pointer
*       funcName  - pointer to function name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       warning: function uses lock the waiting operations
*
*******************************************************************************/
GT_VOID simLogPacketDescrCompare
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *old,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *new,
    IN GT_CHAR                         *funcName
)
{
    GT_BOOL descrChanged;
    GT_BOOL descrInternalChanged;

    if(!simLogIsOpen())
    {
        return ;
    }

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(old);
    ASSERT_PTR(new);
    ASSERT_PTR(funcName);

    if(isDevFiltered(devObjPtr))
    {
        /* the device/core need to be filtered */
        return;
    }

    /* lock the waiting operations */
    scibAccessLock();

    /* save port group id to log */
    simLogDevDescrPortGroupId(devObjPtr);

    /* compare the regular descriptor */
    descrChanged = descrCompare(devObjPtr, NULL, old, new, funcName);

    if(descrChanged == GT_TRUE)
    {
        stateImportantInfo(devObjPtr,new);
    }

    /* check if need to compare the inner descriptor */
    if(new->ingressTunnelInfo.innerFrameDescrPtr)
    {
        SKERNEL_FRAME_CHEETAH_DESCR_STC tempInnerFrameDescr;
        SKERNEL_FRAME_CHEETAH_DESCR_STC *tempInnerFrameDescrPtr;

        if(old->ingressTunnelInfo.innerFrameDescrPtr)
        {
            tempInnerFrameDescrPtr = old->ingressTunnelInfo.innerFrameDescrPtr;
        }
        else
        {
            /* allow to compare 'old' that was not exists before last 'save' of 'old' */
            tempInnerFrameDescrPtr = &tempInnerFrameDescr;
            memset(&tempInnerFrameDescr,0,sizeof(tempInnerFrameDescr));
        }

        /* compare the inner descriptor */
        descrInternalChanged = descrCompare(devObjPtr, "--INNER--",
                    tempInnerFrameDescrPtr,/*old*/
                    new->ingressTunnelInfo.innerFrameDescrPtr,/*new*/
                    funcName);
        if(descrInternalChanged == GT_TRUE)
        {
            stateImportantInfo(devObjPtr,new->ingressTunnelInfo.innerFrameDescrPtr);
        }
    }


    /* UnLock the waiting operations */
    scibAccessUnlock();
}

/*******************************************************************************
* simLogPortsBmpCompare
*
* DESCRIPTION:
*       ports BMP changes between saved one (old) and given one (new)
*       dump to log the DIFF
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       fileNameString - the name (string) of the calling file
*       functionNameString - the name (string) of the calling function
*       variableNamePtr - the name (string) of the variable
*       oldArr       - old ports BMP pointer (pointer to the actual words)
*       newArr       - new ports BMP pointer (pointer to the actual words)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogPortsBmpCompare
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN GT_CHAR                         *fileNameString,
    IN GT_CHAR                         *functionNameString,
    IN GT_CHAR                         *variableNamePtr,
    IN GT_U32                           *oldArr,
    IN GT_U32                           *newArr
)
{
    GT_U32  ii;
    GT_U32 diff_added[SKERNEL_CHEETAH_EGRESS_PORTS_BMP_NUM_WORDS_CNS];
    GT_U32 diff_removed[SKERNEL_CHEETAH_EGRESS_PORTS_BMP_NUM_WORDS_CNS];
    GT_U32 didDiff_added,didDiff_removed;

    if(!simLogIsOpen())
    {
        return ;
    }

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(oldArr);
    ASSERT_PTR(newArr);
    ASSERT_PTR(variableNamePtr);

    didDiff_added = 0;
    didDiff_removed = 0;

    for(ii = 0 ; ii < SKERNEL_CHEETAH_EGRESS_PORTS_BMP_NUM_WORDS_CNS ; ii++)
    {
        diff_added[ii]       = newArr[ii] & ~oldArr[ii];/* in 'new' but not in 'old' */
        diff_removed[ii]     = oldArr[ii] & ~newArr[ii];/* in 'old' but not in 'new' */
        didDiff_added     |= diff_added[ii];
        didDiff_removed |= diff_removed[ii];
    }

    if(didDiff_added || didDiff_removed)
    {
        simLogMessage(functionNameString, fileNameString, 0, devObjPtr, SIM_LOG_INFO_TYPE_PACKET_E,
                      "The Ports BMP [%s] changed to: \n"
                      "0x%8.8x (255..224) "
                      "0x%8.8x (223..192) "
                      "0x%8.8x (191..160) "
                      "0x%8.8x (159..128) \n"
                      "0x%8.8x (127..96) "
                      "0x%8.8x (95..64) "
                      "0x%8.8x (63..32) "
                      "0x%8.8x (31..0) \n"
                      ,
                      variableNamePtr,
                      newArr[7],
                      newArr[6],
                      newArr[5],
                      newArr[4],
                      newArr[3],
                      newArr[2],
                      newArr[1],
                      newArr[0]
                      );

        if(didDiff_added)
        {
            simLogMessage(functionNameString, fileNameString, 0, devObjPtr, SIM_LOG_INFO_TYPE_PACKET_E,
                          "The BMP of ADDED Ports are: \n"
                          "0x%8.8x (255..224) "
                          "0x%8.8x (223..192) "
                          "0x%8.8x (191..160) "
                          "0x%8.8x (159..128) \n"
                          "0x%8.8x (127..96) "
                          "0x%8.8x (95..64) "
                          "0x%8.8x (63..32) "
                          "0x%8.8x (31..0) \n"
                          ,
                          diff_added[7],
                          diff_added[6],
                          diff_added[5],
                          diff_added[4],
                          diff_added[3],
                          diff_added[2],
                          diff_added[1],
                          diff_added[0]
                          );
        }

        if(didDiff_removed)
        {
            simLogMessage(functionNameString, fileNameString, 0, devObjPtr, SIM_LOG_INFO_TYPE_PACKET_E,
                          "The BMP of REMOVED Ports are: \n"
                          "0x%8.8x (255..224) "
                          "0x%8.8x (223..192) "
                          "0x%8.8x (191..160) "
                          "0x%8.8x (159..128) \n"
                          "0x%8.8x (127..96) "
                          "0x%8.8x (95..64) "
                          "0x%8.8x (63..32) "
                          "0x%8.8x (31..0) \n"
                          ,
                          diff_removed[7],
                          diff_removed[6],
                          diff_removed[5],
                          diff_removed[4],
                          diff_removed[3],
                          diff_removed[2],
                          diff_removed[1],
                          diff_removed[0]
                          );
        }
    }
    else
    {
        simLogMessage(functionNameString, fileNameString, 0, devObjPtr, SIM_LOG_INFO_TYPE_PACKET_E,
                      "The Ports BMP [%s] NOT changed \n"
                      ,variableNamePtr);
    }


}



/*******************************************************************************
* simLogPacketDescrCompareFields
*
* DESCRIPTION:
*       log changes between saved packet descriptor and given (fields)
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       old             - old packet descriptor pointer
*       new             - new packet descriptor pointer
*       descrChangedPtr - indication that descriptor was changed
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID simLogPacketDescrCompareFields
(
    IN  SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *old,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC *new,
    OUT GT_BOOL                         *descrChangedPtr
)
{
    SIM_LOG_PACKET_COMPARE_MAC(frameId);

    SIM_LOG_PACKET_COMPARE_MAC(byteCount);
    SIM_LOG_PACKET_COMPARE_MAC(origByteCount);
    SIM_LOG_PACKET_COMPARE_MAC(localDevSrcPort);
    SIM_LOG_PACKET_COMPARE_MAC(localDevSrcPort_fromRxDma_forBma);
    SIM_LOG_PACKET_COMPARE_MAC(ingressGopPortNumber);
    SIM_LOG_PACKET_COMPARE_MAC(ingressRxDmaPortNumber);
    SIM_LOG_PACKET_COMPARE_MAC(marvellTagged);
    SIM_LOG_PACKET_COMPARE_MAC(incomingMtagCmd);
    SIM_LOG_PACKET_COMPARE_MAC(ownDev);
    SIM_LOG_PACKET_COMPARE_MAC(macDaType);

    /* compare source mac */
    if( simLogPacketDescrMacCompare(devObjPtr, old->macSaPtr, new->macSaPtr, "macSaPtr") )
    {
        *descrChangedPtr = GT_TRUE;
    }
    /* compare dest mac */
    if( simLogPacketDescrMacCompare(devObjPtr, old->macDaPtr, new->macDaPtr, "macDaPtr") )
    {
        *descrChangedPtr = GT_TRUE;
    }

    /*SIM_LOG_PACKET_COMPARE_MAC(payloadPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(payloadLength);
    /*SIM_LOG_PACKET_COMPARE_MAC(origVlanTagPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(origVlanTagLength);
    SIM_LOG_PACKET_COMPARE_MAC(origSrcTagged);
/*    SIM_LOG_PACKET_COMPARE_MAC(srcTagged);  replaced with tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] */
    SIM_LOG_PACKET_COMPARE_MAC(srcTagState);
    SIM_LOG_PACKET_COMPARE_MAC(origSrcTagState);
    SIM_LOG_PACKET_COMPARE_MAC(srcPriorityTagged);
    SIM_LOG_PACKET_COMPARE_MAC(nestedVlanAccessPort);
    SIM_LOG_PACKET_COMPARE_MAC(marvellTaggedExtended);
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.origMplsOuterLabel);
    /*SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.originalL2Ptr);*/
    /*SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.originalL3Ptr);*/
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.origEtherType);
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.l2Encaps);
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.arp);
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.mpls);
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.isIp);
    SIM_LOG_PACKET_COMPARE_MAC(origInfoBeforeTunnelTermination.isIPv4);
    /*SIM_LOG_PACKET_COMPARE_MAC(l3StartOffsetPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(l2HeaderSize);
    /*SIM_LOG_PACKET_COMPARE_MAC(l4StartOffsetPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(l23HeaderSize);
    /*SIM_LOG_PACKET_COMPARE_MAC(afterVlanOrDsaTagPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(afterVlanOrDsaTagLen);
    SIM_LOG_PACKET_COMPARE_MAC(l2Valid);
    SIM_LOG_PACKET_COMPARE_MAC(l4Valid);
    SIM_LOG_PACKET_COMPARE_MAC(l3NotValid);
    SIM_LOG_PACKET_COMPARE_MAC(modifyUp);
    SIM_LOG_PACKET_COMPARE_MAC(up);
    SIM_LOG_PACKET_COMPARE_MAC(qosMappedUp);
    SIM_LOG_PACKET_COMPARE_MAC(qosMappedDscp);
    SIM_LOG_PACKET_COMPARE_MAC(modifyDscp);

    SIM_LOG_PACKET_COMPARE_MAC____SNET_QOS_INFO_STC(qos);

    SIM_LOG_PACKET_COMPARE_MAC(egrMarvellTagType);
    SIM_LOG_PACKET_COMPARE_MAC(egress_dsaWords[0]);
    SIM_LOG_PACKET_COMPARE_MAC(egress_dsaWords[1]);
    SIM_LOG_PACKET_COMPARE_MAC(egress_dsaWords[2]);
    SIM_LOG_PACKET_COMPARE_MAC(egress_dsaWords[3]);
    SIM_LOG_PACKET_COMPARE_MAC(egressTagged);
    SIM_LOG_PACKET_COMPARE_MAC(trgTagged);
    SIM_LOG_PACKET_COMPARE_MAC(preserveVid);
    SIM_LOG_PACKET_COMPARE_MAC(vidModified);
    SIM_LOG_PACKET_COMPARE_MAC(eVid);
    SIM_LOG_PACKET_COMPARE_MAC(vlanEtherType);
    SIM_LOG_PACKET_COMPARE_MAC(basicMode);
    SIM_LOG_PACKET_COMPARE_MAC(vlanTag802dot1dInfo.vid);
    SIM_LOG_PACKET_COMPARE_MAC(vlanTag802dot1dInfo.cfi);
    SIM_LOG_PACKET_COMPARE_MAC(vlanTag802dot1dInfo.vpt);
    SIM_LOG_PACKET_COMPARE_MAC(vid0Or1AfterTti);
    SIM_LOG_PACKET_COMPARE_MAC(localDevSrcTrunkId);
    SIM_LOG_PACKET_COMPARE_MAC(srcDevIsOwn);
    SIM_LOG_PACKET_COMPARE_MAC(egressFilterRegistered);
    SIM_LOG_PACKET_COMPARE_MAC(sstId);
    SIM_LOG_PACKET_COMPARE_MAC(sstIdPrecedence);

    /* compare packetCmd */
    if( simLogPacketDescrPacketCmdCompare(devObjPtr, old->packetCmd, new->packetCmd) )
    {
        *descrChangedPtr = GT_TRUE;
    }

    SIM_LOG_PACKET_COMPARE_MAC(rxSniff);
    SIM_LOG_PACKET_COMPARE_MAC(mirroringMode);
    SIM_LOG_PACKET_COMPARE_MAC(analyzerIndex);
    SIM_LOG_PACKET_COMPARE_MAC(mirrorType);
    SIM_LOG_PACKET_COMPARE_MAC(analyzerVlanTagAdd);
    SIM_LOG_PACKET_COMPARE_MAC(analyzerKeepVlanTag);
    SIM_LOG_PACKET_COMPARE_MAC(isIp);
    SIM_LOG_PACKET_COMPARE_MAC(useVidx);
    SIM_LOG_PACKET_COMPARE_MAC(eVidx);
    SIM_LOG_PACKET_COMPARE_MAC(targetIsTrunk);
    SIM_LOG_PACKET_COMPARE_MAC(trgEPort);
    SIM_LOG_PACKET_COMPARE_MAC(trgDev);
    SIM_LOG_PACKET_COMPARE_MAC(trgTrunkId);

    /* compare packetCmd */
    if( simLogPacketDescrCpuCodeCompare(devObjPtr, old->cpuCode, new->cpuCode) )
    {
        *descrChangedPtr = GT_TRUE;
    }

    SIM_LOG_PACKET_COMPARE_MAC(routed);
    SIM_LOG_PACKET_COMPARE_MAC(origIsTrunk);
    SIM_LOG_PACKET_COMPARE_MAC(origSrcEPortOrTrnk);
    SIM_LOG_PACKET_COMPARE_MAC(egressFilterEn);
    SIM_LOG_PACKET_COMPARE_MAC(excludeIsTrunk);
    SIM_LOG_PACKET_COMPARE_MAC(excludedTrunk);
    SIM_LOG_PACKET_COMPARE_MAC(excludedDevice);
    SIM_LOG_PACKET_COMPARE_MAC(excludedPort);
    SIM_LOG_PACKET_COMPARE_MAC(mailBoxToNeighborCPU);
    SIM_LOG_PACKET_COMPARE_MAC(mirrorToAllCpus);
    SIM_LOG_PACKET_COMPARE_MAC(policerEn);
    SIM_LOG_PACKET_COMPARE_MAC(policerCounterEn);
    SIM_LOG_PACKET_COMPARE_MAC(policerPtr);
    SIM_LOG_PACKET_COMPARE_MAC(policerCntPtr);
    /*SIM_LOG_PACKET_COMPARE_MAC(policerCycle); field internal to the policer units ... don't let is be shown*/
    SIM_LOG_PACKET_COMPARE_MAC(policerEgressEn);
    SIM_LOG_PACKET_COMPARE_MAC(policerEgressCntEn);
    SIM_LOG_PACKET_COMPARE_MAC(policerTriggerMode);
    SIM_LOG_PACKET_COMPARE_MAC(policerEArchPointer);
    SIM_LOG_PACKET_COMPARE_MAC(policerEArchCounterEnabled);
    SIM_LOG_PACKET_COMPARE_MAC(policerActuallAccessedIndex);
    SIM_LOG_PACKET_COMPARE_MAC(countingActuallAccessedIndex);
    SIM_LOG_PACKET_COMPARE_MAC(pktHash);
    /*SIM_LOG_PACKET_COMPARE_MAC(pktHashForIpcl); field internal to the IPCL unit ... don't let is be shown */
    SIM_LOG_PACKET_COMPARE_MAC(qosProfilePrecedence);
    SIM_LOG_PACKET_COMPARE_MAC(ipHeaderError);
    SIM_LOG_PACKET_COMPARE_MAC(ipTtiHeaderError);
    SIM_LOG_PACKET_COMPARE_MAC(greHeaderError);
    SIM_LOG_PACKET_COMPARE_MAC(isIPv4);
    SIM_LOG_PACKET_COMPARE_MAC(isNat);

    /* compare source IP */
    if( simLogPacketDescrIpCompare(devObjPtr, old->sip, new->sip, "sip") )
    {
        *descrChangedPtr = GT_TRUE;
    }
    /* compare dest IP */
    if( simLogPacketDescrIpCompare(devObjPtr, old->dip, new->dip, "dip") )
    {
        *descrChangedPtr = GT_TRUE;
    }

    SIM_LOG_PACKET_COMPARE_MAC(bypassBridge);
    SIM_LOG_PACKET_COMPARE_MAC(ipm);
    SIM_LOG_PACKET_COMPARE_MAC(arp);
    SIM_LOG_PACKET_COMPARE_MAC(srcDev);
    SIM_LOG_PACKET_COMPARE_MAC(igmpQuery);
    SIM_LOG_PACKET_COMPARE_MAC(igmpNonQuery);
    SIM_LOG_PACKET_COMPARE_MAC(portVlanSel);
    SIM_LOG_PACKET_COMPARE_MAC(solicitationMcastMsg);
    SIM_LOG_PACKET_COMPARE_MAC(ipv4Icmp);
    SIM_LOG_PACKET_COMPARE_MAC(ipv6Icmp);
    SIM_LOG_PACKET_COMPARE_MAC(ipv6IcmpType);
    SIM_LOG_PACKET_COMPARE_MAC(isIpV6EhExists);
    SIM_LOG_PACKET_COMPARE_MAC(isIpV6EhHopByHop);
    SIM_LOG_PACKET_COMPARE_MAC(isIpv6Mld);
    SIM_LOG_PACKET_COMPARE_MAC(ipXMcLinkLocalProt);
    SIM_LOG_PACKET_COMPARE_MAC(ipv4Ripv1);
    SIM_LOG_PACKET_COMPARE_MAC(l2Encaps);
    SIM_LOG_PACKET_COMPARE_MAC(etherTypeOrSsapDsap);
    SIM_LOG_PACKET_COMPARE_MAC(dscp);
    SIM_LOG_PACKET_COMPARE_MAC(ipProt);
    SIM_LOG_PACKET_COMPARE_MAC(ipv4DontFragmentBit);
    SIM_LOG_PACKET_COMPARE_MAC(ipv4HeaderOptionsExists);
    SIM_LOG_PACKET_COMPARE_MAC(ipv4FragmentOffset);

    /* ingressVlanInfo */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_L2I_VLAN_INFO(ingressVlanInfo);

    /* egressVlanInfo */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_EGR_VLAN_INFO(egressVlanInfo);

    SIM_LOG_PACKET_COMPARE_MAC(outGoingMtagCmd);
    SIM_LOG_PACKET_COMPARE_MAC(tc);
    SIM_LOG_PACKET_COMPARE_MAC(dp);
    SIM_LOG_PACKET_COMPARE_MAC(meterDp);
    SIM_LOG_PACKET_COMPARE_MAC(truncated);
    SIM_LOG_PACKET_COMPARE_MAC(cpuTrgDev);
    SIM_LOG_PACKET_COMPARE_MAC(srcTrg);
    SIM_LOG_PACKET_COMPARE_MAC(srcTaggedTrgTagged);
    SIM_LOG_PACKET_COMPARE_MAC(srcTrgPhysicalPort);
    SIM_LOG_PACKET_COMPARE_MAC(srcTrgDev);
    SIM_LOG_PACKET_COMPARE_MAC(sniffTrgDev);
    SIM_LOG_PACKET_COMPARE_MAC(sniffTrgPort);
    SIM_LOG_PACKET_COMPARE_MAC(txSampled);
    SIM_LOG_PACKET_COMPARE_MAC(txMirrorDone);
    SIM_LOG_PACKET_COMPARE_MAC(egressTrgPort);
    SIM_LOG_PACKET_COMPARE_MAC(bridgeUcRoutEn);
    SIM_LOG_PACKET_COMPARE_MAC(bridgeMcRoutEn);
    SIM_LOG_PACKET_COMPARE_MAC(appSpecCpuCode);
    SIM_LOG_PACKET_COMPARE_MAC(pclUcNextHopIndex);
    SIM_LOG_PACKET_COMPARE_MAC(doRouterHa);
    SIM_LOG_PACKET_COMPARE_MAC(flowLabel);
    SIM_LOG_PACKET_COMPARE_MAC(decTtl);
    SIM_LOG_PACKET_COMPARE_MAC(ttl);
    SIM_LOG_PACKET_COMPARE_MAC(ttl1);
    SIM_LOG_PACKET_COMPARE_MAC(ttl2);
    SIM_LOG_PACKET_COMPARE_MAC(ttl3);
    SIM_LOG_PACKET_COMPARE_MAC(ttl4);
    SIM_LOG_PACKET_COMPARE_MAC(arpPtr);
    SIM_LOG_PACKET_COMPARE_MAC(modifyMacDa);
    SIM_LOG_PACKET_COMPARE_MAC(modifyMacSa);
    SIM_LOG_PACKET_COMPARE_MAC(macDaFound);
    SIM_LOG_PACKET_COMPARE_MAC(udpBcCpuCodeInx);
    SIM_LOG_PACKET_COMPARE_MAC(saAccessLevel);
    SIM_LOG_PACKET_COMPARE_MAC(daAccessLevel);
    SIM_LOG_PACKET_COMPARE_MAC(validMll);
    SIM_LOG_PACKET_COMPARE_MAC(mll);
    SIM_LOG_PACKET_COMPARE_MAC(mllSelector);
    SIM_LOG_PACKET_COMPARE_MAC(mllexternal);
    SIM_LOG_PACKET_COMPARE_MAC(ipV4ucvlan);
    SIM_LOG_PACKET_COMPARE_MAC(ipV4mcvlan);
    SIM_LOG_PACKET_COMPARE_MAC(ipV6ucvlan);
    SIM_LOG_PACKET_COMPARE_MAC(ipV6mcvlan);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelStart);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelStartType);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelStartPassengerType);
    SIM_LOG_PACKET_COMPARE_MAC(numOfLabels);
    SIM_LOG_PACKET_COMPARE_MAC(bypassRouter);
    SIM_LOG_PACKET_COMPARE_MAC(bypassIngressPipe);
    SIM_LOG_PACKET_COMPARE_MAC(mpls);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelPtr);
    SIM_LOG_PACKET_COMPARE_MAC(label1);
    SIM_LOG_PACKET_COMPARE_MAC(exp1);
    SIM_LOG_PACKET_COMPARE_MAC(label2);
    SIM_LOG_PACKET_COMPARE_MAC(exp2);
    SIM_LOG_PACKET_COMPARE_MAC(label3);
    SIM_LOG_PACKET_COMPARE_MAC(exp3);
    SIM_LOG_PACKET_COMPARE_MAC(label4);
    SIM_LOG_PACKET_COMPARE_MAC(exp4);
    SIM_LOG_PACKET_COMPARE_MAC(setSBit);
    SIM_LOG_PACKET_COMPARE_MAC(mplsCommand);
    SIM_LOG_PACKET_COMPARE_MAC(tsEgressMplsControlWordExist);
    SIM_LOG_PACKET_COMPARE_MAC(tsEgressMplsNumOfLabels);
    SIM_LOG_PACKET_COMPARE_MAC(origTunnelTtl);
    SIM_LOG_PACKET_COMPARE_MAC(mim);
    SIM_LOG_PACKET_COMPARE_MAC(iUp);
    SIM_LOG_PACKET_COMPARE_MAC(iDp);
    SIM_LOG_PACKET_COMPARE_MAC(iRes1);
    SIM_LOG_PACKET_COMPARE_MAC(iRes2);
    SIM_LOG_PACKET_COMPARE_MAC(iSid);
    SIM_LOG_PACKET_COMPARE_MAC(policyOnPortEn);
    SIM_LOG_PACKET_COMPARE_MAC(pclLookUpMode[0]);
    SIM_LOG_PACKET_COMPARE_MAC(pclLookUpMode[1]);
    SIM_LOG_PACKET_COMPARE_MAC(pclLookUpMode[2]);
    SIM_LOG_PACKET_COMPARE_MAC(ipclProfileIndex);
    SIM_LOG_PACKET_COMPARE_MAC(mac2me);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelTerminated);
    SIM_LOG_PACKET_COMPARE_MAC(pceRoutLttIdx);
    SIM_LOG_PACKET_COMPARE_MAC(vrfId);
    SIM_LOG_PACKET_COMPARE_MAC(VntL2Echo);
    SIM_LOG_PACKET_COMPARE_MAC(ActionStop);
    /*SIM_LOG_PACKET_COMPARE_MAC(l3StartOfPassenger);*/
    SIM_LOG_PACKET_COMPARE_MAC(passengerLength);
    SIM_LOG_PACKET_COMPARE_MAC(protOverMpls);
    SIM_LOG_PACKET_COMPARE_MAC(cfidei);
    SIM_LOG_PACKET_COMPARE_MAC(up1);
    SIM_LOG_PACKET_COMPARE_MAC(cfidei1);
    SIM_LOG_PACKET_COMPARE_MAC(vid1);
    SIM_LOG_PACKET_COMPARE_MAC(originalVid1);
    SIM_LOG_PACKET_COMPARE_MAC(vlanEtherType1);
    SIM_LOG_PACKET_COMPARE_MAC(trustTag1Qos);
    SIM_LOG_PACKET_COMPARE_MAC(oam);
    SIM_LOG_PACKET_COMPARE_MAC(cfm);
    SIM_LOG_PACKET_COMPARE_MAC(pktIsLooped);
    SIM_LOG_PACKET_COMPARE_MAC(dropOnSource);
    SIM_LOG_PACKET_COMPARE_MAC(l4SrcPort);
    SIM_LOG_PACKET_COMPARE_MAC(l4DstPort);
    SIM_LOG_PACKET_COMPARE_MAC(udpCompatible);
    SIM_LOG_PACKET_COMPARE_MAC(cpuTrgPort);

    /* capwap stc */
    SIM_LOG_PACKET_COMPARE_MAC(capwap);

    SIM_LOG_PACKET_COMPARE_MAC(ieeeReservedMcastCmdProfile);
    SIM_LOG_PACKET_COMPARE_MAC(mllPtrForWlanBridging);
    SIM_LOG_PACKET_COMPARE_MAC(passengerTag);
    SIM_LOG_PACKET_COMPARE_MAC(validSrcIdBmp);
    SIM_LOG_PACKET_COMPARE_MAC(srcIdBmp);
    SIM_LOG_PACKET_COMPARE_MAC(ignoreQosIndexFromDsaTag);
    SIM_LOG_PACKET_COMPARE_MAC(egressOsmRedirect);
    /*SIM_LOG_PACKET_COMPARE_MAC(tunnelStartMacInfoPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(tunnelStartMacInfoLen);
    /*SIM_LOG_PACKET_COMPARE_MAC(tunnelStartRestOfHeaderInfoPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(tunnelStartRestOfHeaderInfoLen);
    SIM_LOG_PACKET_COMPARE_MAC(ethernetOverXPassengerTagMode);
    SIM_LOG_PACKET_COMPARE_MAC(ethernetOverXPassengerTagged);
    SIM_LOG_PACKET_COMPARE_MAC(ttRouterLTT);
    SIM_LOG_PACKET_COMPARE_MAC(udb[0]);
    SIM_LOG_PACKET_COMPARE_MAC(udb[1]);
    SIM_LOG_PACKET_COMPARE_MAC(udb[2]);
    SIM_LOG_PACKET_COMPARE_MAC(ipServiceFlag);
    SIM_LOG_PACKET_COMPARE_MAC(brdgUcIpHeaderCheck);
    SIM_LOG_PACKET_COMPARE_MAC(useIngressPipeVid);
    SIM_LOG_PACKET_COMPARE_MAC(ingressPipeVid);
    SIM_LOG_PACKET_COMPARE_MAC(egressByteCount);
    SIM_LOG_PACKET_COMPARE_MAC(ingressVlanTag0Type);
    SIM_LOG_PACKET_COMPARE_MAC(ingressVlanTag1Type);
    /*SIM_LOG_PACKET_COMPARE_MAC(tag0Ptr);*/
    /*SIM_LOG_PACKET_COMPARE_MAC(tag1Ptr);*/
    SIM_LOG_PACKET_COMPARE_MAC(tpidIndex[0]);
    SIM_LOG_PACKET_COMPARE_MAC(tpidIndex[1]);
    SIM_LOG_PACKET_COMPARE_MAC(tpidIndexTunnelstart);
    SIM_LOG_PACKET_COMPARE_MAC(tagSrcTagged[0]);
    SIM_LOG_PACKET_COMPARE_MAC(tagSrcTagged[1]);
    SIM_LOG_PACKET_COMPARE_MAC(tag1LocalDevSrcTagged);
    SIM_LOG_PACKET_COMPARE_MAC(tr101ModeEn);
    SIM_LOG_PACKET_COMPARE_MAC(localPortGroupPortAsGlobalDevicePort);
    SIM_LOG_PACKET_COMPARE_MAC(localPortTrunkAsGlobalPortTrunk);
    SIM_LOG_PACKET_COMPARE_MAC(egressOnIngressPortGroup);
    /*SIM_LOG_PACKET_COMPARE_MAC(ipFixTimeStampValue);*/

    /* epclAction stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC(epclAction);
    /* eplrAction stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC(eplrAction);

    SIM_LOG_PACKET_COMPARE_MAC(portIsRingCorePort);
    SIM_LOG_PACKET_COMPARE_MAC(srcCoreId);
    SIM_LOG_PACKET_COMPARE_MAC(ttiHashMaskIndex);
    SIM_LOG_PACKET_COMPARE_MAC(forceNewDsaToCpu);
    SIM_LOG_PACKET_COMPARE_MAC(bypassRouterAndPolicer);
    /*SIM_LOG_PACKET_COMPARE_MAC(packetTimestamp);*/

    /* ptpGtsInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_LION_PTP_GTS_INFO_STC(ptpGtsInfo);

    /* ipfixErrataData stc */
    SIM_LOG_PACKET_COMPARE_MAC(ipfixErrataData);

    /* eArchExtInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SKERNEL_E_ARCH_EXT_INFO_STC(eArchExtInfo);

    SIM_LOG_PACKET_COMPARE_MAC(flowId);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelTerminationOffset);

    /* perProtocolInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_PER_PROTOCOL_INFO_STC(perProtocolInfo);

    SIM_LOG_PACKET_COMPARE_MAC(txqToEq);
    /*SIM_LOG_PACKET_COMPARE_MAC(origDescrPtr);*/

    /* haToEpclInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SKERNEL_HA_TO_EPCL_INFO_STC(haToEpclInfo);

    /*SIM_LOG_PACKET_COMPARE_MAC(ingressDevObjPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(extraSrcPortBits);
    SIM_LOG_PACKET_COMPARE_MAC(txqId);
    SIM_LOG_PACKET_COMPARE_MAC(forceToCpuTrgPortOnHemisphare0);
    SIM_LOG_PACKET_COMPARE_MAC(floodToNextTxq);
    SIM_LOG_PACKET_COMPARE_MAC(dsaCoreIdBit2);
    SIM_LOG_PACKET_COMPARE_MAC(isTrillEtherType);
    SIM_LOG_PACKET_COMPARE_MAC(trillEngineTriggered);
    SIM_LOG_PACKET_COMPARE_MAC(tunnelStartTrillTransit);

    /* trillInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_TRILL_INFO_STC(trillInfo);

    /******** ingressTunnelInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_INGRESS_TUNNEL_INFO_STC(ingressTunnelInfo);

    /* compare innerMacDa */
    if(new->ingressTunnelInfo.innerMacDaPtr)
    {
        GT_U8   *tmpInnerMacDaPtr;

        if(old)
        {
            tmpInnerMacDaPtr = old->ingressTunnelInfo.innerMacDaPtr;
        }
        else
        {
            /* allow to compare 'old' that was not exists before last 'save' of 'old' */
            tmpInnerMacDaPtr = 0;
        }

        if(simLogPacketDescrMacCompare(devObjPtr,
                    tmpInnerMacDaPtr,/*old*/
                    new->ingressTunnelInfo.innerMacDaPtr,/*new*/
                    "ingressTunnelInfo.innerMacDaPtr") )
        {
            *descrChangedPtr = GT_TRUE;
        }
    }


    SIM_LOG_PACKET_COMPARE_MAC(ttiLookupMatch);
    SIM_LOG_PACKET_COMPARE_MAC(centralizedChassisModeEn);
    SIM_LOG_PACKET_COMPARE_MAC(trustQosMappingTableIndex);
    SIM_LOG_PACKET_COMPARE_MAC(up2QosProfileMappingMode);
    SIM_LOG_PACKET_COMPARE_MAC(numOfBytesToPop);
    SIM_LOG_PACKET_COMPARE_MAC(popTagsWithoutReparse);
    SIM_LOG_PACKET_COMPARE_MAC(didPacketParseFromEngine[SNET_CHT_FRAME_PARSE_MODE_PORT_E]);
    SIM_LOG_PACKET_COMPARE_MAC(didPacketParseFromEngine[SNET_CHT_FRAME_PARSE_MODE_TRILL_E]);
    SIM_LOG_PACKET_COMPARE_MAC(didPacketParseFromEngine[SNET_CHT_FRAME_PARSE_MODE_FROM_TTI_PASSENGER_E]);
    SIM_LOG_PACKET_COMPARE_MAC(calcCrc);

    /* oamInfo stc */
    SIM_LOG_PACKET_COMPARE_MAC____SKERNEL_OAM_INFO_STC(oamInfo);


    SIM_LOG_PACKET_COMPARE_MAC(isFromSdma);

    /* fcoe */
    SIM_LOG_PACKET_COMPARE_MAC(isFcoe);
    SIM_LOG_PACKET_COMPARE_MAC(fcoeInfo.fcoeLegal);
    SIM_LOG_PACKET_COMPARE_MAC(fcoeInfo.fcoeFwdEn);

    SIM_LOG_PACKET_COMPARE_MAC(pcktType);
    SIM_LOG_PACKET_COMPARE_MAC(tti_pcktType_sip5);
    SIM_LOG_PACKET_COMPARE_MAC(pcl_pcktType_sip5);
    SIM_LOG_PACKET_COMPARE_MAC(epcl_pcktType_sip5);
    SIM_LOG_PACKET_COMPARE_MAC(isMultiTargetReplication);
    SIM_LOG_PACKET_COMPARE_MAC(isMultiTarget);

    SIM_LOG_PACKET_COMPARE_MAC____SNET_INGRESS_DSA_STC(ingressDsa);

    SIM_LOG_PACKET_COMPARE_MAC(ipTTKeyProtocol);
    SIM_LOG_PACKET_COMPARE_MAC(pclId);
    SIM_LOG_PACKET_COMPARE_MAC(rxEnableProtectionSwitching);
    SIM_LOG_PACKET_COMPARE_MAC(rxIsProtectionPath);
    SIM_LOG_PACKET_COMPARE_MAC(channelTypeToOpcodeMappingEn);
    SIM_LOG_PACKET_COMPARE_MAC(channelTypeProfile);
    SIM_LOG_PACKET_COMPARE_MAC(ipclUdbConfigurationTableUdeIndex);
    SIM_LOG_PACKET_COMPARE_MAC(enableL3L4ParsingOverMpls);
    SIM_LOG_PACKET_COMPARE_MAC(ipxLength);
    SIM_LOG_PACKET_COMPARE_MAC(ipxHeaderLength);

    /*SIM_LOG_PACKET_COMPARE_MAC(bmpOfHemisphereMapperPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(egressPacketType);
    SIM_LOG_PACKET_COMPARE_MAC(innerPacketType);
    SIM_LOG_PACKET_COMPARE_MAC(isMplsLsr);

    SIM_LOG_PACKET_COMPARE_MAC____SNET_EGRESS_PHYSICAL_PORT_INFO_STC(egressPhysicalPortInfo);

    SIM_LOG_PACKET_COMPARE_MAC(useArpForMacSa);
    if(new->useArpForMacSa)
    {
        if(simLogPacketDescrMacCompare(devObjPtr,
                    old->newMacSa,/*old*/
                    new->newMacSa,/*new*/
                    "newMacSa") )
        {
            *descrChangedPtr = GT_TRUE;
        }
    }



    SIM_LOG_PACKET_COMPARE_MAC(pclAssignedSstId);
    SIM_LOG_PACKET_COMPARE_MAC(pclSrcIdMask);
    SIM_LOG_PACKET_COMPARE_MAC(cwFirstNibble);
    SIM_LOG_PACKET_COMPARE_MAC(pclRedirectCmd);
    SIM_LOG_PACKET_COMPARE_MAC(ttiRedirectCmd);
    SIM_LOG_PACKET_COMPARE_MAC(fdbBasedUcRouting);
    SIM_LOG_PACKET_COMPARE_MAC(tag0IsOuterTag);

    /* haAction stc */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_CHEETAH_AFTER_HA_COMMON_ACTION_STC(haAction);

    SIM_LOG_PACKET_COMPARE_MAC(timestampTagged[0]);
    SIM_LOG_PACKET_COMPARE_MAC(timestampTagged[1]);

    /* timestampTagInfo */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_TIMESTAMP_TAG_INFO_STC(timestampTagInfo[0]);
    SIM_LOG_PACKET_COMPARE_MAC____SNET_TIMESTAMP_TAG_INFO_STC(timestampTagInfo[1]);

    /* timestampActionInfo */
    SIM_LOG_PACKET_COMPARE_MAC____SNET_TIMESTAMP_ACTION_INFO_STC(timestampActionInfo);

    /* timestamp */
/*    SIM_LOG_PACKET_COMPARE_MAC____SNET_TOD_TIMER_STC(timestamp[0]);*/
/*    SIM_LOG_PACKET_COMPARE_MAC____SNET_TOD_TIMER_STC(timestamp[1]);*/

    SIM_LOG_PACKET_COMPARE_MAC(ptpUField);
    SIM_LOG_PACKET_COMPARE_MAC(ptpTaiSelect);
    SIM_LOG_PACKET_COMPARE_MAC(ptpEgressTaiSel);
    SIM_LOG_PACKET_COMPARE_MAC(ptpIsTimestampLocal);
    SIM_LOG_PACKET_COMPARE_MAC(isPtp);
    SIM_LOG_PACKET_COMPARE_MAC(isPtpException);
    SIM_LOG_PACKET_COMPARE_MAC(ptpTriggerType);
    SIM_LOG_PACKET_COMPARE_MAC(ptpOffset);
    SIM_LOG_PACKET_COMPARE_MAC(ptpDomain);
    SIM_LOG_PACKET_COMPARE_MAC(ptpActionIsLocal);
    SIM_LOG_PACKET_COMPARE_MAC(ptpUdpChecksumMode);
    SIM_LOG_PACKET_COMPARE_MAC(vplsInfo);
    SIM_LOG_PACKET_COMPARE_MAC(overrideVid0WithOrigVid);
    SIM_LOG_PACKET_COMPARE_MAC(bridgeToMllInfo.virtualSrcPort);
    SIM_LOG_PACKET_COMPARE_MAC(bridgeToMllInfo.virtualSrcDev);
    SIM_LOG_PACKET_COMPARE_MAC(egressPassangerTagTpidIndex);
    SIM_LOG_PACKET_COMPARE_MAC(numberOfSubscribers);
    SIM_LOG_PACKET_COMPARE_MAC(mplsLsrOffset);
    SIM_LOG_PACKET_COMPARE_MAC(origMacSaPtr);
    SIM_LOG_PACKET_COMPARE_MAC(ttiMetadataReady);
    /*SIM_LOG_PACKET_COMPARE_MAC(ttiMetadataInfo); this is huge array */
    SIM_LOG_PACKET_COMPARE_MAC(ipclMetadataReady);
    /*SIM_LOG_PACKET_COMPARE_MAC(ipclMetadataInfo); this is huge array */
    SIM_LOG_PACKET_COMPARE_MAC(epclMetadataReady);
    /*SIM_LOG_PACKET_COMPARE_MAC(epclMetadataInfo); this is huge array */
    /*SIM_LOG_PACKET_COMPARE_MAC(pclExtraDataPtr);->pointer */
    SIM_LOG_PACKET_COMPARE_MAC(firstVlanTagExtendedSize);
    SIM_LOG_PACKET_COMPARE_MAC(tmQueueId);
    SIM_LOG_PACKET_COMPARE_MAC(copyReserved);
    SIM_LOG_PACKET_COMPARE_MAC(greHeaderSize);
    SIM_LOG_PACKET_COMPARE_MAC(tcpFlags);
    SIM_LOG_PACKET_COMPARE_MAC(tcpSyn);
    SIM_LOG_PACKET_COMPARE_MAC(tcpSynWithData);
    SIM_LOG_PACKET_COMPARE_MAC(ecnCapable);
    SIM_LOG_PACKET_COMPARE_MAC(qcnRx);
    SIM_LOG_PACKET_COMPARE_MAC(origRxQcnPrio);
    SIM_LOG_PACKET_COMPARE_MAC(outerPacketType);
    SIM_LOG_PACKET_COMPARE_MAC(tr101EgressVlanTagMode);
    SIM_LOG_PACKET_COMPARE_MAC(passangerTr101EgressVlanTagMode);
    SIM_LOG_PACKET_COMPARE_MAC(queue_dp);
    SIM_LOG_PACKET_COMPARE_MAC(queue_priority);
    /*SIM_LOG_PACKET_COMPARE_MAC(frameBuf);*/
    /*SIM_LOG_PACKET_COMPARE_MAC(txDmaDevObjPtr);*/
    /*SIM_LOG_PACKET_COMPARE_MAC(innerFrameDescrPtr);*/
    SIM_LOG_PACKET_COMPARE_MAC(bmpsEqUnitsGotTxqMirror);
    SIM_LOG_PACKET_COMPARE_MAC(isCpuUseSdma);
    SIM_LOG_PACKET_COMPARE_MAC(trafficManagerEnabled);
    SIM_LOG_PACKET_COMPARE_MAC(trafficManagerTc);
    SIM_LOG_PACKET_COMPARE_MAC(trafficManagerFinalPort);
    SIM_LOG_PACKET_COMPARE_MAC(trafficManagerCos);
    SIM_LOG_PACKET_COMPARE_MAC(trafficManagerColor);
}


/*******************************************************************************
* simLogPacketFrameMyLogInfoGet
*
* DESCRIPTION:
*       Get the pointer to the log info of current thread (own thread)
*
* INPUTS:
*       None.
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to the log info of current thread (own thread)
*       (may be NULL)
*
* COMMENTS:
*
*
*******************************************************************************/
SIM_LOG_FRAME_INFO_STC * simLogPacketFrameMyLogInfoGet(
    GT_VOID
)
{
    SKERNEL_TASK_COOKIE_INFO_STC  *myTaskInfoPtr =
        simOsTaskOwnTaskCookieGet();

    if(myTaskInfoPtr == NULL)
    {
        /* task not registered yet this info ... */
        return NULL;
    }

    if(myTaskInfoPtr->generic.additionalInfo == GT_FALSE)
    {
        /* task not bound additional info that the LOG can use */
        return NULL;
    }

    if(myTaskInfoPtr->logInfoPtr == NULL)
    {
        /* first time for this task */
        myTaskInfoPtr->logInfoPtr = calloc(1 , sizeof(SIM_LOG_FRAME_INFO_STC));
    }

    return myTaskInfoPtr->logInfoPtr;
}



