/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3Routing.c
*
* DESCRIPTION:
*       Layer 3 IP unicast/multicast routing for cht3 asic simulation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 43 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3Routing.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypeTcam.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>


/* number of entries in one line */
#define XCAT_IP_ROUT_NUM_ENTRIES_IN_LINE_CNS 4

/* valid bit offset in control is at location: CH3 - 17, XCAT - 16  */
#define SNET_CONTROL_VALID_BIT_OFFSET_CNS(dev) \
                (SKERNEL_IS_XCAT_DEV(dev) ?    \
                 16:                           \
                 17)

/*******************************************************************************
* snetCht3L3iTcamLookUp
*
* DESCRIPTION:
*        Tcam lookup for IPv4/6 address .
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*
* OUTPUTS:
*
*       matchIndexPtr   - pointer to the matching index.
*
* RETURN:
*
* COMMENTS:
*       Router Tcam Table:
*       CH3:  TCAM holds up to 20K entries of 32 bits for IPv4
*                           or 5K entries of 128 bits for IPv6.
*       XCAT: TCAM holds up to 13K entries of 32 bits for IPv4
*                           or 3.25K entries of 128 bits for IPv6.
*******************************************************************************/
GT_VOID snetCht3L3iTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr  ,
    OUT GT_U32 *matchIndex
)
{
    DECLARE_FUNC_NAME(snetCht3L3iTcamLookUp);

    GT_U32   maxEntries;               /* CH3:  5K entries for extended IPv6 ,20K for IPv4*/
                                       /* XCAT: 3.25K entries for extended IPv6, 13K for IPv4*/
    GT_U32   entryIndex;               /* Index to the TCAM line                */
    GT_U32   bankIndex;                /* Index to the word in the TCAM rule    */
    GT_U32   idx_match;                /* Index for DIP or SIP search           */
    GT_U32   *xdataPtr, *xDataTmpPtr;  /* pointer to routing TCAM data X entry  */
    GT_U32   *xctrlPtr, *xCtrlTmpPtr;  /* pointer to routing TCAM ctrl X entry  */
    GT_U32   *ydataPtr, *yDataTmpPtr;  /* pointer to routing TCAM data Y entry  */
    GT_U32   *yctrlPtr, *yCtrlTmpPtr;  /* pointer to routing TCAM ctrl Y entry  */
    GT_U32   compModeData;             /* compare mode bit in the tcam rule     */
    GT_U32   ipAddr32Bits ;            /* routing tcam search routing 32 lower bits   */
    GT_U32   ipAddr16Bits ;            /* routing tcam search routing 32 lower bits   */
    GT_U32   entryIndexstart;          /* start entry range in TCAM table       */
    GT_U32   entryIndexlast;           /* end entry range in TCAM table         */
    GT_U32   keySize = 4;              /* tt tcam size of TCAM words            */
    GT_U32   xcompModeData;            /* compare mode bit in the tcam rule     */
    GT_U32   ycompModeData;            /* compare mode bit in the tcam rule     */
    GT_U32   valid;                    /* compare mode bit in the tcam rule     */
    GT_BOOL  match[4];
    GT_U32   gArray;                   /* G index Array r for multicast search  */
    GT_BIT   isIPv4Compatible;         /* Lookup IPv4 or IPv6 address */
    GT_U32   indexToRowColumnFactor;   /* factor to convert index to row,column (not used for ch3) */

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_TCAM_E);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        indexToRowColumnFactor = 5;
    }
    else
    {
        /* NOT used for cheetah3 */
        indexToRowColumnFactor = XCAT_IP_ROUT_NUM_ENTRIES_IN_LINE_CNS;
    }

    entryIndexstart = 0;

    /* use IPv4 address for IPv4 and ARP packets -- fix CQ#90052 */
    /* fcoe key the same as ipv4 UC */
    isIPv4Compatible = (descrPtr->isIPv4 || descrPtr->arp || descrPtr->isFcoe) ? 1 : 0;

    for (idx_match = 0 ; idx_match <  SNET_CHT2_TCAM_SEARCH;  ++idx_match)
    {
        if (isIPv4Compatible)
        {
            maxEntries = devObjPtr->routeTcamInfo.numEntriesIpv4 ;
            entryIndexlast = maxEntries;
        }
        else  /* IPv6 */
        {
            maxEntries = devObjPtr->routeTcamInfo.numEntriesIpv6 ;

            if (SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
            {
                entryIndexlast = maxEntries;
            }
            else
            {
                entryIndexlast = maxEntries*4;
            }
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            maxEntries = (maxEntries * indexToRowColumnFactor) / 4;
            if (isIPv4Compatible)
            {
                entryIndexlast = maxEntries;
            }
            else
            {
                entryIndexlast = maxEntries*4;
            }
        }

        /* initialize the matchIndexPtr */
        matchIndex[idx_match] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;

        /*  Get pointer to Tcam data entry */
        xDataTmpPtr = smemMemGet(devObjPtr,SSMEM_CHT3_TCAM_X_DATA_TBL_MEM(devObjPtr));

        /*  Get pointer to Tcam control entry */
        xCtrlTmpPtr = smemMemGet(devObjPtr,SSMEM_CHT3_TCAM_ROUTER_TT_X_CONTROL_TBL_MEM(devObjPtr));

        /*  Get pointer to Tcam data mask entry */
        yDataTmpPtr = smemMemGet(devObjPtr,SSMEM_CHT3_TCAM_Y_DATA_TBL_MEM(devObjPtr));

        /*  Get pointer to Tcam control mask entry */
        yCtrlTmpPtr = smemMemGet(devObjPtr,SSMEM_CHT3_TCAM_ROUTER_TT_Y_CONTROL_TBL_MEM(devObjPtr));

        for (entryIndex = entryIndexstart ; entryIndex <  entryIndexlast;  ++entryIndex)
        {
            /* Calculate the appropriate address for this entry. */
            xdataPtr = xDataTmpPtr + entryIndex * devObjPtr->routeTcamInfo.entryWidth;
            xctrlPtr = xCtrlTmpPtr + entryIndex * devObjPtr->routeTcamInfo.entryWidth;
            ydataPtr = yDataTmpPtr + entryIndex * devObjPtr->routeTcamInfo.entryWidth;
            yctrlPtr = yCtrlTmpPtr + entryIndex * devObjPtr->routeTcamInfo.entryWidth;

            ycompModeData =  (yctrlPtr[0] & (3<<18)) >>18; /* get compmode */
            xcompModeData =   (xctrlPtr[0] & (3<<18)) >>18; /* get compmode */
            valid = ( ((xctrlPtr[0] >> SNET_CONTROL_VALID_BIT_OFFSET_CNS(devObjPtr)) & 1) &
                    ((~yctrlPtr[0] >> SNET_CONTROL_VALID_BIT_OFFSET_CNS(devObjPtr)) & 1)  );

            /* for ipv4 should be '0' */
            compModeData = ((xcompModeData) & (~ycompModeData));

            if (isIPv4Compatible == 0)
            {

                for (bankIndex = 0 ; bankIndex <  keySize  ; ++bankIndex)
                {
                    match[bankIndex] = GT_FALSE;

                    /* calculate 48 bits for word 0,1,2,3 */
                    ycompModeData =  (yctrlPtr[0] & (3<<18)) >>18; /* get compmode */
                    xcompModeData =   (xctrlPtr[0] & (3<<18)) >>18; /* get compmode */
                    valid = ( (( xctrlPtr[0] >> SNET_CONTROL_VALID_BIT_OFFSET_CNS(devObjPtr)) & 1) &
                            ((~yctrlPtr[0] >> SNET_CONTROL_VALID_BIT_OFFSET_CNS(devObjPtr)) & 1) );

                    /* for ipv6 compModeData should be '1' */
                    if ( !valid || !compModeData)
                      break;

                    /* log tcam entry and bank */
                    __LOG_TCAM(("entry index: %d, bank index: %d\n", entryIndex, bankIndex));

                    /* log tcam content */
                    __LOG_TCAM(("xdataPtr: 0x%08X, ydataPtr: 0x%08X, xctrlPtr: 0x%08X, xctrlPtr: 0x%08X\n",
                                   ~xdataPtr[0], ~ydataPtr[0], ~xctrlPtr[0], ~yctrlPtr[0]));

                    if (bankIndex == 0)
                    {
                      /* calculate 48 bits for word1 from the ipv6 search key */
                      ipAddr16Bits = (idx_match) ? (descrPtr->sip[2] & 0xFFFF): (descrPtr->dip[2]& 0xFFFF) ;
                      ipAddr32Bits = (idx_match) ? descrPtr->sip[3] : descrPtr->dip[3] ;
                    }
                    else if (bankIndex == 1)
                    {
                      ipAddr32Bits  = (((idx_match) ? (descrPtr->sip[2] & 0xFFFF0000) : (descrPtr->dip[2] & 0xFFFF0000)) >> 16);
                      ipAddr32Bits |= (((idx_match) ? (descrPtr->sip[1] & 0xFFFF)     : (descrPtr->dip[1] & 0xFFFF   )) << 16);
                      ipAddr16Bits  = (((idx_match) ? (descrPtr->sip[1] & 0xFFFF0000) : (descrPtr->dip[1] & 0xFFFF0000)) >> 16);
                    }
                    else if (bankIndex == 2)
                    {
                      /* calculate 48 bits for word1 from the ipv6 search key */
                      if ((idx_match == 1) && (descrPtr->ipm == 1))
                      {   /* multicast for (G,S) search */
                          if (SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
                          {
                              /* G index Row r */
                              ipAddr16Bits =  (matchIndex[0] % devObjPtr->routeTcamInfo.offsetBetweenBulks) & 0x1FFF; /* 13 bits */
                              /* G index Array a */
                              gArray       =  (matchIndex[0] / devObjPtr->routeTcamInfo.offsetBetweenBulks) & 0x3;    /* 2 bits */
                          }
                          else
                          {
                              /* G index Row r */
                              ipAddr16Bits =  (matchIndex[0] / indexToRowColumnFactor) & 0x1FFF; /* 13 bits */
                              /* G index Array a */
                              gArray       =  (matchIndex[0] % indexToRowColumnFactor) & 0x3;    /* 2 bits */
                          }

                          ipAddr16Bits =  (ipAddr16Bits | gArray << 13);
                          ipAddr16Bits =  (ipAddr16Bits | 1 << 15);
                      }
                      else
                      {
                          ipAddr16Bits =  descrPtr->vrfId;
                      }

                      ipAddr32Bits =(idx_match) ? descrPtr->sip[0] : descrPtr->dip[0] ;
                    }
                    else
                    {
                      ipAddr32Bits = 0;
                      ipAddr16Bits = 0;
                    }

                    /* lookup in TCAM */
                    if  ((((~ydataPtr[0] & ipAddr32Bits) | (~xdataPtr[0] & ~ipAddr32Bits)) == 0xFFFFFFFF)
                      &&
                      ((((~yctrlPtr[0] & ipAddr16Bits) | (~xctrlPtr[0] & ~ipAddr16Bits)) & 0xFFFF) == 0xFFFF))

                    {
                        /* log tcam info */
                        __LOG_TCAM(("tcam matched \n"));

                      /* Get to the next bank for this entry*/
                        __LOG(("Get to the next bank for this entry"));

                      xdataPtr = xdataPtr + devObjPtr->routeTcamInfo.bankWidth / 4;
                      xctrlPtr = xctrlPtr + devObjPtr->routeTcamInfo.bankWidth / 4;
                      ydataPtr = ydataPtr + devObjPtr->routeTcamInfo.bankWidth / 4;
                      yctrlPtr = yctrlPtr + devObjPtr->routeTcamInfo.bankWidth / 4;

                      /* got a match - continue to next bank */
                      __LOG(("got a match - continue to next bank"));
                      match[bankIndex] = GT_TRUE;

                      continue;
                    }
                    else
                    {
                        /* log tcam info */
                        simLogTcamTTNotMatch(devObjPtr, ipAddr16Bits, ipAddr32Bits,
                                             xdataPtr, ydataPtr, xctrlPtr, yctrlPtr);

                      /* no match in one of the banks */
                      break;
                    }
                } /*  find an entry    */

                /* if there was a match in all banks */
                if ((match[0] == GT_TRUE) &&
                    (match[1] == GT_TRUE) &&
                    (match[2] == GT_TRUE) &&
                    (match[3] == GT_TRUE))
                {
                     matchIndex[idx_match] = entryIndex;
                     break;
                }

                if (SKERNEL_IS_XCAT_DEV(devObjPtr))
                {
                    /* the entries order in bank0 for ipv6 are enrty0, entry4 ...*/
                    /* the loop will add additional 1 to the entryIndex */
                    entryIndex += 3;
                }
            } /* for loop word index ipv6 search */
            else /* ipv4 packet */
            {    /* descrPtr->vrfId  and descrPtr->dip */
               if ((compModeData==0) && (valid) )/* for ipv4 compModeData should be '0' */
               {

                   ipAddr32Bits = (idx_match) ? descrPtr->sip[0] : descrPtr->dip[0] ;
                   if ((idx_match == 1) && (descrPtr->ipm == 1))
                   {   /* multicast for (G,S) search */
                       if (SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
                          {
                              /* G index Row r */
                              ipAddr16Bits =  (matchIndex[0] % devObjPtr->routeTcamInfo.offsetBetweenBulks) & 0x1FFF; /* 13 bits */
                              /* G index Array a */
                              gArray       =  (matchIndex[0] / devObjPtr->routeTcamInfo.offsetBetweenBulks) & 0x3;    /* 2 bits */
                          }
                          else
                          {
                              /* G index Row r */
                              ipAddr16Bits =  (matchIndex[0] / indexToRowColumnFactor) & 0x1FFF; /* 13 bits */
                              /* G index Array a */
                              gArray       =  (matchIndex[0] % indexToRowColumnFactor) & 0x3;    /* 2 bits */
                          }

                       ipAddr16Bits =  (ipAddr16Bits | gArray << 13);
                       ipAddr16Bits =  (ipAddr16Bits | 1 << 15);
                   }
                   else
                   {
                       ipAddr16Bits =  descrPtr->vrfId;
                   }
                   if ((((~ydataPtr[0] & ipAddr32Bits) | (~xdataPtr[0] & ~ipAddr32Bits)) == 0xFFFFFFFF)
                         &&
                       ((((~yctrlPtr[0] & ipAddr16Bits) | (~xctrlPtr[0] & ~ipAddr16Bits)) & 0xFFFF) == 0xFFFF))

                   {
                       matchIndex[idx_match] = entryIndex;
                       entryIndex = maxEntries;
                       break ;
                   }
               }
            }

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* this code is common for IPv4 and for IPv6 lookups */
                if((entryIndex % indexToRowColumnFactor) == 3)
                {
                    /*we are at end of 4'th bank */
                    /* support skip of the 5'th bank (that is used only for the TTI lookup) */
                    entryIndex += 1;
                }
            }
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        GT_U32  row,column,newIndex;

        for (idx_match = 0 ; idx_match <  SNET_CHT2_TCAM_SEARCH;  ++idx_match)
        {
            if(matchIndex[idx_match] == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
            {
                /* 'no match' indication -- must not convert this value */
                continue;
            }

            row    = matchIndex[idx_match] / indexToRowColumnFactor;
            column = matchIndex[idx_match] % indexToRowColumnFactor;
            newIndex = (4 * row) + column;

            matchIndex[idx_match] = newIndex;

            /* convert 5 banks index into 4 banks index (that 'skip' one bank) */
            __LOG_TCAM(("convert 5 banks index into 4 banks index (that 'skip' one bank)\n"
                          "from index[%d] to [%d] \n", matchIndex[idx_match], newIndex));
        }
    }

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___RESTORE_PREVIOUS_UNIT___E);
}

/*******************************************************************************
*   snetCht3UpdateRpfCheckMode
*
* DESCRIPTION:
*       updates rpf check mode (relevant for cheetah 3 and above)
*
* INPUTS:
*       devObjPtr        - pointer to device object.
*       descrPtr         - pointer to the frame's descriptor.
*       rpfCheckModePtr  - weather to make rpfCheck
*
* OUTPUTS:
*       rpfCheckModePtr  - weather to make rpfCheck
*
*******************************************************************************/
GT_VOID snetCht3UpdateRpfCheckMode
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    INOUT SNET_RPF_CHECK_MODE_ENT         *unicastRpfCheckModePtr
)
{
    GT_U32  fldValue = 0;  /* field value in QoS register */
    GT_U32  *memPtr;       /* pointer to memory */

    /* If LTT<UC RPF Check Enable> is disabled,
       then read per-Vlan uRPF mode configuration (otherwise it is ignored
       and Vlan-based uRPF is performed) */
    if  ((devObjPtr->ipvxSupport.supportPerVlanURpfMode) &&
         (*unicastRpfCheckModePtr == SNET_RPF_DISABLED_E) )
    {
        if(descrPtr->eArchExtInfo.ipvxIngressEVlanTablePtr)
        {
            /*per-eVLAN uRPF mode*/
            fldValue = snetFieldValueGet(descrPtr->eArchExtInfo.ipvxIngressEVlanTablePtr , 0 , 2);
        }
        else
        {
            memPtr = smemMemGet(devObjPtr, SMEM_LION_ROUTER_PER_VLAN_URPF_MODE_TBL_MEM(devObjPtr));
            /* 2 bits per vlan */
            /* Get per-Vlan uRPF check mode */
            fldValue = snetFieldValueGet(memPtr , descrPtr->eVid * 2 , 2);
        }

        switch(fldValue)
        {
            case 0:
                *unicastRpfCheckModePtr = SNET_RPF_DISABLED_E;
                break;
            case 1:
                *unicastRpfCheckModePtr = SNET_RPF_VLAN_BASED_E;
                break;
            case 2:
                *unicastRpfCheckModePtr = SNET_RPF_PORT_BASED_E;
                break;
            case 3:
            default:
                *unicastRpfCheckModePtr = SNET_RPF_LOOSE_E;
                break;
        }
    }
}

/*******************************************************************************
*   snetCht3UpdateSipSaCheckEnable
*
* DESCRIPTION:
*       updates sip sa check enable (relevant for cheetah 3 and above)
*
* INPUTS:
*       devObjPtr            - pointer to device object.
*       descrPtr             - pointer to the frame's descriptor.
*       unicastSipSaCheckPtr - weather to make sip sa check
*
* OUTPUTS:
*       unicastSipSaCheckPtr  - weather to make sip sa check
*
*******************************************************************************/
GT_VOID snetCht3UpdateSipSaCheckEnable
(
    IN    SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    INOUT GT_U32                          *unicastSipSaCheckPtr
)
{
    DECLARE_FUNC_NAME(snetCht3UpdateSipSaCheckEnable);

    GT_U32  fldValue = 0;  /* field value in QoS register */
    GT_U32  bitIndex;      /* index of bit */

    if(devObjPtr->ipvxSupport.supportPerPortSipSaCheckEnable)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /*Router per-ePort SIP-SA check enable*/
            __LOG(("Router per-ePort SIP-SA check enable"));
            fldValue = lion3IpvxLocalDevSrcEportBitsGet(devObjPtr,descrPtr, IPVX_PER_SRC_PORT_FIELD_ROUTER_PER_EPORT_SIP_SA_CHECK_ENABLE_E);
        }
        else
        {
            /* Check SIP/SA enabled for source Port. */
            __LOG(("Check SIP/SA enabled for source Port."));
            bitIndex = (descrPtr->localDevSrcPort == SNET_CHT_CPU_PORT_CNS)
                        ? 31 : descrPtr->localDevSrcPort;

            bitIndex &= 0x1f;/*patch for bobcat2*/
            /* Read register field */
            smemRegFldGet(devObjPtr, SMEM_LION_ROUTER_PER_PORT_SIPSA_REG(devObjPtr),
                                                        bitIndex, 1, &fldValue);
        }

        if(fldValue)
        {
           *unicastSipSaCheckPtr = 1;
        }
    }
}

/*******************************************************************************
* snetCht3L3iFetchRouteEntry
*
* DESCRIPTION:
*       Fetch the lookup translation table entry  .
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       matchIndexPtr   - pointer to the matching index.
*
* OUTPUTS:
*       routeIndexPtr         - pointer to the matching table.
*       ipSecurChecksInfoPtr  - routing security checks information
*
* RETURN:
*
* COMMENTS:
*       Router LTT Table:
*       CH3:  LTT holds up to 5K lines.
*       XCAT: LTT holds up to 3.25K lines.
*
*******************************************************************************/
GT_VOID snetCht3L3iFetchRouteEntry
(
    IN  SKERNEL_DEVICE_OBJECT               *devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC     *descrPtr,
    IN  GT_U32                              *matchIndexPtr,
    OUT GT_U32                              *routeIndexPtr,
    OUT SNET_ROUTE_SECURITY_CHECKS_INFO_STC *ipSecurChecksInfoPtr
)
{
    DECLARE_FUNC_NAME(snetCht3L3iFetchRouteEntry);

    SNET_CHT2_L3_TRANSLATION_TABLE_STC lttEntryData;
    GT_U32  * lttEntryPtr;          /* (pointer to) action entry in memory */
    GT_U32  idx_match;              /* Index of tcam match  */
    GT_U32  matchInxByK[SNET_CHT2_TCAM_SEARCH];         /* index in  translationDataPtr table */
    GT_U32  regAddr[SNET_CHT2_TCAM_SEARCH];             /* Register address */
    GT_U32  offsetInxByK[SNET_CHT2_TCAM_SEARCH];        /* offset address index for QoS register */
    GT_U32  qosProfEntryInx;        /* base address index for QoS register */
    GT_U32  qosEntryOffSet;         /* offset index for QoS register */
    GT_U32  fldValue = 0;           /* field value in QoS register */

    for (idx_match = 0; idx_match < SNET_CHT2_TCAM_SEARCH;++ idx_match)
    {
        /* in case of multicast */
        __LOG(("in case of multicast"));
        if (descrPtr->ipm == 1)
        {
            /* differentiate between (G,*) and (G,S) cases */
            __LOG(("differentiate between (G,*) and (G,S) cases"));
            if (matchIndexPtr[1] != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
            {
                /* multicast packet (G,S) */
                __LOG(("multicast packet (G,S)"));
                if (idx_match == 0)
                {
                    /* no need to handle (G,*) case */
                    __LOG(("no need to handle (G,*) case"));
                    continue;
                }
                routeIndexPtr[0] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;
            }
            else
            {
                /*  multicast packet (G,*) */
                __LOG(("multicast packet (G,*)"));
                if (idx_match == 1)
                {
                    /* no need to handle (G,S) case */
                    __LOG(("no need to handle (G,S) case"));
                    continue;
                }
                routeIndexPtr[1] = SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS;
            }
        }

        if (matchIndexPtr[idx_match] <= devObjPtr->routeTcamInfo.numEntriesIpv4)
        {
            if (SKERNEL_IS_CHEETAH3_ONLY_DEV(devObjPtr))
            {
                matchInxByK[idx_match]  = matchIndexPtr[idx_match] / devObjPtr->routeTcamInfo.offsetBetweenBulks;
                offsetInxByK[idx_match] =  matchIndexPtr[idx_match] % devObjPtr->routeTcamInfo.offsetBetweenBulks;
                regAddr[idx_match] = SMEM_CHT3_TUNNEL_ACTION_TBL_MEM(devObjPtr,offsetInxByK[idx_match]) +
                devObjPtr->routeTcamInfo.numEntriesIpv4 * matchInxByK[idx_match];
            }
            else
            {
                /*we want to get to row and column*/
                __LOG(("we want to get to row and column"));
                regAddr[idx_match] = SMEM_CHT3_TUNNEL_ACTION_WORD_TBL_MEM(devObjPtr,matchIndexPtr[idx_match]/4,matchIndexPtr[idx_match]%4);
            }

            lttEntryPtr  = smemMemGet(devObjPtr, regAddr[idx_match]);

            /* Fill the structure of the Look Translation Table */
            __LOG(("Fill the structure of the Look Translation Table"));
            if (devObjPtr->ipvxSupport.lttMaxNumberOfPaths == SKERNEL_LTT_MAX_NUMBER_OF_PATHS_8_E)
            {
                lttEntryData.lttRouteType = SMEM_U32_GET_FIELD(lttEntryPtr[0], 0, 1);
                lttEntryData.lttNumberOfPaths = SMEM_U32_GET_FIELD(lttEntryPtr[0], 1, 3);
                lttEntryData.lttUnicastRPFCheckEnable = SMEM_U32_GET_FIELD(lttEntryPtr[0], 4, 1);
                lttEntryData.lttIPv6GroupScopeLevel = SMEM_U32_GET_FIELD(lttEntryPtr[0], 5, 2);
                lttEntryData.lttUnicastSipSaEnable = SMEM_U32_GET_FIELD(lttEntryPtr[0], 7, 1);
                lttEntryData.lttRouteEntryIndex = SMEM_U32_GET_FIELD(lttEntryPtr[0], 8, 13);
            }
            else
            {
                /* LTT_MAX_NUMBER_OF_PATHS_64_E */
                __LOG(("LTT_MAX_NUMBER_OF_PATHS_64_E"));
                lttEntryData.lttRouteType = SMEM_U32_GET_FIELD(lttEntryPtr[0], 0, 1);
                lttEntryData.lttNumberOfPaths = SMEM_U32_GET_FIELD(lttEntryPtr[0], 1, 6);
                lttEntryData.lttUnicastRPFCheckEnable = SMEM_U32_GET_FIELD(lttEntryPtr[0], 7, 1);
                lttEntryData.lttIPv6GroupScopeLevel = SMEM_U32_GET_FIELD(lttEntryPtr[0], 8, 2);
                lttEntryData.lttUnicastSipSaEnable = SMEM_U32_GET_FIELD(lttEntryPtr[0], 10, 1);
                lttEntryData.lttRouteEntryIndex = SMEM_U32_GET_FIELD(lttEntryPtr[0], 11, 13);
            }

            if (lttEntryData.lttRouteType == SNET_CHT2_IP_ROUT_ECMP)
            {

                /* Calculating Route Entry index in ECMP mode */
                __LOG(("Calculating Route Entry index in ECMP mode"));
                if (devObjPtr->ipvxSupport.ecmpIndexFormula == SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_0_E)
                {
                    routeIndexPtr[idx_match] = lttEntryData.lttRouteEntryIndex +
                            (descrPtr->pktHash % (lttEntryData.lttNumberOfPaths +1));
                }
                else if (devObjPtr->ipvxSupport.ecmpIndexFormula == SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E)
                {
                    /* ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E */
                    __LOG(("ECMP_ROUTE_INDEX_FORMULA_TYPE_1_E"));
                    routeIndexPtr[idx_match] = lttEntryData.lttRouteEntryIndex +
                        ((descrPtr->pktHash * (lttEntryData.lttNumberOfPaths + 1)) / 64);
                }
                else /*if(devObjPtr->ipvxSupport.ecmpIndexFormula == SKERNEL_ECMP_ROUTE_INDEX_FORMULA_TYPE_2_E)*/
                {
                    /* get the ECMP offset */
                    __LOG(("get the ECMP offset"));
                    snetChtEqHashIndexResolution(devObjPtr,descrPtr,
                        (lttEntryData.lttNumberOfPaths +1), 0/*ecmp random en*/,
                        &routeIndexPtr[idx_match],
                        SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L3_ECMP_E,
                        NULL,NULL);

                    /* add the offset to the final index */
                    __LOG(("add the offset to the final index"));
                    routeIndexPtr[idx_match] += lttEntryData.lttRouteEntryIndex;
                }
            }
            else
            {    /* Calculating Route Entry index in QoS mode */
                __LOG(("Calculating Route Entry index in QoS mode"));
                 if (devObjPtr->ipvxSupport.qosIndexFormula == SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_0_E)
                 {
                     qosProfEntryInx = descrPtr->qos.qosProfile / 8;
                     qosEntryOffSet = descrPtr->qos.qosProfile % 8;
                     regAddr[idx_match] = SMEM_CHT3_QOS_ROUTING_TBL_MEM(devObjPtr, qosProfEntryInx);
                     smemRegFldGet(devObjPtr, regAddr[idx_match], 4 * qosEntryOffSet , 3, &fldValue);
                     routeIndexPtr[idx_match] =  lttEntryData.lttRouteEntryIndex  +
                                      (fldValue % (lttEntryData.lttNumberOfPaths + 1));
                 }
                 else
                 {   /*SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E*/
                     __LOG(("SKERNEL_QOS_ROUTE_INDEX_FORMULA_TYPE_1_E"));
                     routeIndexPtr[idx_match] =  lttEntryData.lttRouteEntryIndex  +
                                      ((fldValue * (lttEntryData.lttNumberOfPaths + 1)) / 8);
                 }
            }

            if (idx_match == 1 )
            {
                __LOG(("fill security checks info - relevant only for SIP search"));
                ipSecurChecksInfoPtr->rpfCheckMode = (lttEntryData.lttUnicastRPFCheckEnable) ?
                                                          SNET_RPF_VLAN_BASED_E : SNET_RPF_DISABLED_E;
                ipSecurChecksInfoPtr->sipNumberOfPaths       = lttEntryData.lttNumberOfPaths;
                ipSecurChecksInfoPtr->sipBaseRouteEntryIndex = lttEntryData.lttRouteEntryIndex;
                ipSecurChecksInfoPtr->unicastSipSaCheck      = lttEntryData.lttUnicastSipSaEnable;
            }

            ipSecurChecksInfoPtr->ipv6MulticastGroupScopeLevel = lttEntryData.lttIPv6GroupScopeLevel;
        }
    }
}

/*******************************************************************************
*   snetChtIpLogFlow
*
* DESCRIPTION:
*       Log the SIP,DIP match
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       sipIndex      - what SA lookup index found
*       dipIndex      - what DA lookup index found
*
* OUTPUT:
*
*
*
*******************************************************************************/
static GT_VOID snetChtIpLogFlow
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   sipIndex,
    IN GT_U32   dipIndex
)
{
    DECLARE_FUNC_NAME(snetChtIpLogFlow);

    GT_BIT  enabled;/* is feature enabled */
    GT_U32  tmpMask,tmpValue,tmpKey;
    GT_U32  regVal;
    GT_U32  sipLogAddresses[]={
        0x02800d20,
        0x02800d30,
        0x02800d5c
    };
    GT_U32  dipLogAddresses[]={
        0x02800d00,
        0x02800d10,
        0x02800d50
    };
    GT_U32 *addrPtr;
    GT_U32 *resultPtr;
    GT_U32  index;
    GT_U32  sipResult = 0;
    GT_U32  dipResult = 0;
    GT_U32  ii;
    GT_U32  *ipAddrPtr;
    GT_U32  kk,kkMax;
    GT_U32  regAddr;
    GT_U32  *nextHopMemPtr;

    if(sipIndex == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS &&
       dipIndex == SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
    {
        return;
    }

    smemRegFldGet(devObjPtr,SMEM_CHT2_ROUTER_ADDITIONAL_CONTROL_REG(devObjPtr),6,1,&enabled);

    if(enabled == 0)
    {
        /* the feature not enabled */
        __LOG(("the feature not enabled"));
        return;
    }

    if(sipIndex != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
    {
        sipResult = 1;
    }

    if(dipIndex != SNET_CHT2_IP_ROUT_NO_MATCH_INDEX_CNS)
    {
        dipResult = 1;
    }

    /* check L4 dest port */
    __LOG(("check L4 dest port"));
    smemRegGet(devObjPtr,SMEM_CHT3_IP_HIT_LOG_LAYER4_DESTINATION_PORT_REG(devObjPtr),&regVal);
    tmpMask  = SMEM_U32_GET_FIELD(regVal,16,16);
    tmpValue = SMEM_U32_GET_FIELD(regVal, 0,16);

    tmpKey = descrPtr->l4DstPort;
    if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
    {
        /* no match */
        __LOG(("no match"));
        return;
    }

    /* check L4 src port */
    __LOG(("check L4 src port"));
    smemRegGet(devObjPtr,SMEM_CHT3_IP_HIT_LOG_LAYER4_SOURCE_PORT_PORT_REG(devObjPtr),&regVal);
    tmpMask  = SMEM_U32_GET_FIELD(regVal,16,16);
    tmpValue = SMEM_U32_GET_FIELD(regVal, 0,16);

    tmpKey = descrPtr->l4SrcPort;
    if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
    {
        /* no match */
        __LOG(("no match"));
        return;
    }

    /* check IP protocol */
    __LOG(("check IP protocol"));
    smemRegGet(devObjPtr,SMEM_CHT3_IP_HIT_LOG_PROTOCOL_REG(devObjPtr),&regVal);
    tmpMask  = SMEM_U32_GET_FIELD(regVal, 8,8);
    tmpValue = SMEM_U32_GET_FIELD(regVal, 0,8);

    tmpKey = descrPtr->ipProt;
    if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
    {
        /* no match */
        __LOG(("no match"));
        return;
    }


    kkMax = descrPtr->isIPv4 ? 1 /*ipv4*/: 4/*Ipv6*/;

    for(ii = 0 ; ii < 2 ; ii++)
    {
        if(ii == 0)
        {
            if(sipResult)
            {
                addrPtr = sipLogAddresses;
                ipAddrPtr = &descrPtr->sip[0];
                resultPtr = &sipResult;
                index = sipIndex;
            }
            else
            {
                continue;
            }
        }
        else /* ii == 1 */
        {
            if(dipResult)
            {
                addrPtr = dipLogAddresses;
                ipAddrPtr = &descrPtr->dip[0];
                resultPtr = &dipResult;
                index = dipIndex;
            }
            else
            {
                continue;
            }
        }

        for(kk = 0 ; kk < kkMax ; kk++)
        {
            tmpKey = ipAddrPtr[kk];
            smemRegGet(devObjPtr,addrPtr[0] + (4*kk) ,&regVal);/* ip addr */
            tmpValue  = regVal;
            smemRegGet(devObjPtr,addrPtr[1] + (4*kk) ,&regVal);/* ip addr mask */
            tmpMask  = regVal;

            if(0 == SNET_CHT_MASK_CHECK(tmpValue,tmpMask,tmpKey))
            {
                /* no match */
                *resultPtr = 0;
                break;
            }
        }

        if(*resultPtr)
        {
            /* override previous values */
            __LOG(("override previous values"));
            regAddr = SMEM_CHT3_NEXT_HOP_ENTRY_TBL_MEM(index);
            nextHopMemPtr = smemMemGet(devObjPtr, regAddr);

            for( kk = 0 ; kk < 3 ; kk++)
            {
                smemRegSet(devObjPtr,addrPtr[2] + kk * 4, nextHopMemPtr[kk]);/* 3 result registers */
            }
        }
    }

}

/*******************************************************************************
*   snetCht3L3iCounters
*
* DESCRIPTION:
*       Update Bridge counters
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       cntrlPcktInfoPtr    - pointer to control packet info structure
*
*
*
*******************************************************************************/
GT_VOID snetCht3L3iCounters
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SNET_CHEETAH2_L3_CNTRL_PACKET_INFO * cntrlPcktInfoPtr ,
    IN GT_U32 * routeIndexPtr
)
{
    DECLARE_FUNC_NAME(snetCht3L3iCounters);

    /* call the log flow */
    __LOG(("call the log flow"));
    snetChtIpLogFlow(devObjPtr,descrPtr,routeIndexPtr[1]/*sip*/,routeIndexPtr[0]/*dip*/);

     /* Update router counters */
    __LOG(("Update router counters"));
    snetCht2L3iCounters(devObjPtr, descrPtr, cntrlPcktInfoPtr,
                            routeIndexPtr);

}



