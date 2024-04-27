/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistTrafficCond.c
*
* DESCRIPTION:
*      Traffic condition processing for frame
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 4 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
#include <asicSimulation/SLog/simLog.h>

typedef enum {
    SAMBA_TRAFFIC_COND_INTERRUPT_METER_COUNT_OVERFLOW_E=7,
    SAMBA_TRAFFIC_COND_INTERRUPT_POLICING_COUNT_OVERFLOW_E=8,
    SAMBA_TRAFFIC_COND_INTERRUPT_TCB_COUNT_FULL_E=11,
}SAMBA_TRAFFIC_COND_INTERRUPT_CAUSE_REG_ENT;


typedef struct{
/* word 0 */
    GT_U32     doMetering       :1;
    GT_U32     res              :1;
    GT_U32     remarkCosByDscp  :1;
    GT_U32     enModifyTC       :1;
    GT_U32     remarkCosByUP    :1;
    GT_U32     dropRed          :1;   /*N/A*/
    GT_U32     colorMode        :1;   /*N/A*/
    GT_U32     meterDuMode      :1;
    GT_U32     tickMode         :1;   /*N/A*/
    GT_U32     policerMode      :1;   /*N/A*/
    GT_U32     L0_lsb           :22;  /*N/A*/

/* word 1 */
    GT_U32     L0_msb           :6 ;  /*N/A*/
    GT_U32     L1_lsb           :26;  /*N/A*/

/* word 2 */
    GT_U32     L1_msb           :2 ;  /*N/A*/
    GT_U32     I0               :16;  /*N/A*/
    GT_U32     I1_lsb           :14;  /*N/A*/

/* word 3 */
    GT_U32     I1_msb           :2 ;  /*N/A*/
    GT_U32     L0_large         :12;  /*N/A*/
    GT_U32     L1_large         :12;  /*N/A*/
    GT_U32     enModifyExtCoS   :1;
    GT_U32     counterMode      :2;
    GT_U32     meterManagementCounters:2;
    GT_U32     counterDu        :1;

    union{
        struct{
/* word 4 */
            GT_U32  cdu:26;
            GT_U32  packets_lsb:6;
/* word 5 */
            GT_U32  packets_msb:16;
            GT_U32  tet0_lsb:16;           /*N/A*/
        }both;

        struct{
/* word 4 */
            GT_U32  cdu_lsb;
/* word 5 */
            GT_U32  cdu_msb:16;
            GT_U32  tet0_lsb:16;           /*N/A*/
        }cduOnly;

        struct{
/* word 4 */
            GT_U32  packets_lsb;
/* word 5 */
            GT_U32  packets_msb:16;
            GT_U32  tet0_lsb:16;           /*N/A*/
        }packetsOnly;
    }meterCounters;

/* word 6 */
    GT_U32  tet0_msb:24;                  /*N/A*/
    GT_U32  tet1_lsb:8;                   /*N/A*/
/* word 7 */
    GT_U32  tet1_msb;                     /*N/A*/

    struct{
/* word 8 */
        GT_U32  green_lsb;

/* word 9 */
        GT_U32  green_msb:10;
        GT_U32  yellow_lsb:22;

/* word 10 */
        GT_U32  yellow_msb:20;
        GT_U32  red_lsb:12;

/* word 11 */
        GT_U32  red_msb:30;
        GT_U32  res:2;
    }conformanceCounters;
}SNET_SAMABA_PP_TRAFF_COND_ENTRY_STC;



/*******************************************************************************
*   snetSambaTrafficConditionCauseInterrupt
*
* DESCRIPTION:
*        samba -- do interrupt for a specific cause in the traffic condition
*               interrupt cause register
* INPUTS:
*        deviceObj - pointer to device object.
*        interruptCause  - specific interrupt to set
* OUTPUTS:
*
*
*
*******************************************************************************/
static void snetSambaTrafficConditionCauseInterrupt
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SAMBA_TRAFFIC_COND_INTERRUPT_CAUSE_REG_ENT interruptCause
)
{
    GT_U32  fldValue;
    GT_U32  regVal;
    GT_U32  iterruptEn=GT_FALSE;

    smemRegFldGet(devObjPtr,TRAFFIC_COND_INTERRUPT_MASK_REG,
                    interruptCause,1,&fldValue);

    if(fldValue)
    {
        /* the interrupt is enabled */
        smemRegGet(devObjPtr, TRAFFIC_COND_INTERRUPT_CAUSE_REG,&regVal);
        SMEM_U32_SET_FIELD(regVal,0,1,1);   /* summery bit */
        SMEM_U32_SET_FIELD(regVal,interruptCause,1,1);/* specific interrupt */
        smemRegSet(devObjPtr, TRAFFIC_COND_INTERRUPT_CAUSE_REG,regVal);


        /* PCI Interrupt Summary Mask */
        smemPciRegFldGet(devObjPtr, PCI_INT_MASK_REG, 12, 1, &fldValue);
        if (fldValue) {
            /* Lx SumInt */
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 12, 1, 1);
            /* IntSum */
            smemPciRegFldSet(devObjPtr, PCI_INT_CAUSE_REG, 0, 1, 1);
            iterruptEn = GT_TRUE;
        }
    }

    if(iterruptEn==GT_TRUE)
    {
        scibSetInterrupt(devObjPtr->deviceId);
    }

    return;
}

/*******************************************************************************
*   snetSambaTrafficConditionEntryGetByParams
*
* DESCRIPTION:
*        samba -- get a tc entry from memory and fill the tcEntryPTR
* INPUTS:
*        deviceObj - pointer to device object.
*        tcIndex  - index of the tc entry
*        activateTC - does policy class set activateTC
*        activateCount - does policy class set activateCount
* OUTPUTS:
*        tcEntryPTR - pointer to fill info to
*
*
*******************************************************************************/
static void snetSambaTrafficConditionEntryGetByParams
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32                tcIndex,
    IN    GT_BOOL               activateTC,
    IN    GT_BOOL               activateCount,
    OUT   SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    SNET_SAMABA_PP_TRAFF_COND_ENTRY_STC *tmpTcEntryAsicFormatPTR;
    GT_U32      tcEntryAddress;

    /* get TC table base address */
    smemRegFldGet(devObjPtr,TWIST_TRAFFIC_COND_TABLE_BASE_ADDR_REG,
                0,22,&tcEntryAddress);

    tcEntryAddress <<= 4;

    /* calculate the actual address of entry */
    tcEntryAddress += SNET_SAMBA_INTERNAL_MEM_BASE_ADDRESS +
                      (tcIndex<<4) ;

    tmpTcEntryAsicFormatPTR = smemMemGet(devObjPtr,tcEntryAddress);

    /* convert PP format to generic simulation format */
    tcEntryPTR->doMetering      =  (tmpTcEntryAsicFormatPTR->doMetering &&
                                    activateTC);
    tcEntryPTR->doCounting      =  activateCount;
    tcEntryPTR->remarkCosByDscp =  tmpTcEntryAsicFormatPTR->remarkCosByDscp;
    tcEntryPTR->enModifyTC      =  tmpTcEntryAsicFormatPTR->enModifyTC     ;
    tcEntryPTR->remarkCosByUP   =  tmpTcEntryAsicFormatPTR->remarkCosByUP  ;
    tcEntryPTR->dropRed         =  tmpTcEntryAsicFormatPTR->dropRed        ;
    tcEntryPTR->colorMode       =  tmpTcEntryAsicFormatPTR->colorMode      ;
    tcEntryPTR->meterDuMode     =  tmpTcEntryAsicFormatPTR->meterDuMode    ;
    tcEntryPTR->tickMode        =  tmpTcEntryAsicFormatPTR->tickMode       ;
    tcEntryPTR->policerMode     =  tmpTcEntryAsicFormatPTR->policerMode    ;
    tcEntryPTR->L0.l[0]         =  tmpTcEntryAsicFormatPTR->L0_lsb |
                                   (tmpTcEntryAsicFormatPTR->L0_msb << 22) |
                                   (tmpTcEntryAsicFormatPTR->L0_large << 28) ;
    tcEntryPTR->L0.l[1]         =  tmpTcEntryAsicFormatPTR->L0_large>>4;
    tcEntryPTR->L1.l[0]         =  tmpTcEntryAsicFormatPTR->L1_lsb |
                                   (tmpTcEntryAsicFormatPTR->L1_msb << 26) |
                                   (tmpTcEntryAsicFormatPTR->L1_large << 28) ;
    tcEntryPTR->L1.l[1]         =  tmpTcEntryAsicFormatPTR->L1_large>>4;
    tcEntryPTR->I0              =  (GT_U16)tmpTcEntryAsicFormatPTR->I0             ;
    tcEntryPTR->I1              =  (GT_U16)((tmpTcEntryAsicFormatPTR->I1_msb<<14) |
                                    tmpTcEntryAsicFormatPTR->L1_lsb);
    tcEntryPTR->enModifyExtCoS = tmpTcEntryAsicFormatPTR->enModifyExtCoS;
    tcEntryPTR->counterMode = tmpTcEntryAsicFormatPTR->counterMode;
    tcEntryPTR->counterDu = tmpTcEntryAsicFormatPTR->counterDu;
    tcEntryPTR->meterManagementCounters =
                        tmpTcEntryAsicFormatPTR->meterManagementCounters;

    memset(&tcEntryPTR->meterCounters,0,sizeof(tcEntryPTR->meterCounters));

    switch(tcEntryPTR->counterMode)
    {
        case 0:
        case 3:
            /* cdu and packets */
            tcEntryPTR->meterCounters.cdu.l[0]     =
                tmpTcEntryAsicFormatPTR->meterCounters.both.cdu;
            tcEntryPTR->meterCounters.packets.l[0] =
                (tmpTcEntryAsicFormatPTR->meterCounters.both.packets_msb << 16) |
                 tmpTcEntryAsicFormatPTR->meterCounters.both.packets_lsb ;
            break;
        case 1:
            /* cdu only */
            tcEntryPTR->meterCounters.cdu.l[0] =
                tmpTcEntryAsicFormatPTR->meterCounters.cduOnly.cdu_lsb;
            tcEntryPTR->meterCounters.cdu.l[1] =
                tmpTcEntryAsicFormatPTR->meterCounters.cduOnly.cdu_msb;
            break;
        case 2:
            /* packets only */
            tcEntryPTR->meterCounters.packets.l[0] =
                tmpTcEntryAsicFormatPTR->meterCounters.packetsOnly.packets_lsb;
            tcEntryPTR->meterCounters.packets.l[1] =
                tmpTcEntryAsicFormatPTR->meterCounters.packetsOnly.packets_msb;
            break;
        default:
            skernelFatalError(" snetSambaTrafficConditionEntryGet: not valid mode[%d]",
                            tcEntryPTR->counterMode);
    }
    tcEntryPTR->tet0 = (tmpTcEntryAsicFormatPTR->tet0_msb << 24 )|
                    tmpTcEntryAsicFormatPTR->meterCounters.cduOnly.tet0_lsb ;
    tcEntryPTR->tet1 =
         tmpTcEntryAsicFormatPTR->tet1_lsb |
         (tmpTcEntryAsicFormatPTR->tet1_msb << 8);
    tcEntryPTR->conformanceCounters.green.l[0]  =
        tmpTcEntryAsicFormatPTR->conformanceCounters.green_lsb;
    tcEntryPTR->conformanceCounters.green.l[1]  =
        tmpTcEntryAsicFormatPTR->conformanceCounters.green_msb;
    tcEntryPTR->conformanceCounters.yellow.l[0] =
        tmpTcEntryAsicFormatPTR->conformanceCounters.yellow_lsb |
        tmpTcEntryAsicFormatPTR->conformanceCounters.yellow_msb<<22;
    tcEntryPTR->conformanceCounters.yellow.l[1]  =
        tmpTcEntryAsicFormatPTR->conformanceCounters.yellow_msb>>10;
    tcEntryPTR->conformanceCounters.red.l[0]  =
        tmpTcEntryAsicFormatPTR->conformanceCounters.red_lsb |
        (tmpTcEntryAsicFormatPTR->conformanceCounters.red_msb<<12);
    tcEntryPTR->conformanceCounters.red.l[1]  =
        tmpTcEntryAsicFormatPTR->conformanceCounters.red_msb>>20;

    return;
}

/*******************************************************************************
*   snetSambaTrafficConditionEntryGet
*
* DESCRIPTION:
*        samba -- get a tc entry from memory and fill the tcEntryPTR
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        tcEntryPTR - pointer to fill info to
*
*
*******************************************************************************/
static void snetSambaTrafficConditionEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    snetSambaTrafficConditionEntryGetByParams(devObjPtr,
                                              descrPtr->traffCondOffset,
                                              descrPtr->activateTC,
                                              descrPtr->activateCount,
                                              tcEntryPTR);
}

/*******************************************************************************
*   snetTwistTrafficConditionEntryGet
*
* DESCRIPTION:
*        get a tc entry from memory and fill the tcEntryPTR
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        tcEntryPTR - pointer to fill info to
*
*
*******************************************************************************/
static void snetTwistTrafficConditionEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    DECLARE_FUNC_NAME(snetTwistTrafficConditionEntryGet);

    if (devObjPtr->deviceFamily ==SKERNEL_SAMBA_FAMILY)
    {
        snetSambaTrafficConditionEntryGet(devObjPtr,descrPtr,tcEntryPTR);
    }
    else
    {
        /* not supported yet */
        __LOG(("not supported yet"));
        tcEntryPTR->doMetering =0;
        tcEntryPTR->doCounting =0;
    }

    return;
}

/*******************************************************************************
*   snetTwistTrafficConditionDoMeterCounters
*
* DESCRIPTION:
*        do counting of metering according to tc entry
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        tcEntryPTR - tc entry info
* OUTPUTS:
*        tcEntryPTR - pointer to update counters in
*
*
*******************************************************************************/
static void snetTwistTrafficConditionDoMeterCounters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    DECLARE_FUNC_NAME(snetTwistTrafficConditionDoMeterCounters);

    GT_U32  duNumber ;
    GT_U32  currRegAddr;
    GT_U32  currRegValue;
    GT_U32  tcbRegVal;
    GT_U32  enCounter;

    smemRegGet(devObjPtr, TCB_CONTROL_REG,&tcbRegVal);

    enCounter = SMEM_U32_GET_FIELD(tcbRegVal,4,1);

    if(enCounter==0)
    {
        return;
    }

    duNumber = tcEntryPTR->meterDuMode == 1 ?
               (descrPtr->byteCount /64) :
               descrPtr->byteCount ;

    switch(tcEntryPTR->counterMode)
    {
        case 1:
            /* cdu */
            __LOG(("cdu"));
            tcEntryPTR->meterCounters.cdu.l[0] += duNumber;
            break;
        case 2:
            /* packets */
            __LOG(("packets"));
            tcEntryPTR->meterCounters.packets.l[0]++;
            break;
        case 0:
        case 3:
        default:
            /* both */
            __LOG(("both"));
            tcEntryPTR->meterCounters.cdu.l[0] += duNumber;
            tcEntryPTR->meterCounters.packets.l[0]++;
            break;
    }

    if(tcEntryPTR->meterManagementCounters!=0)
    {
        currRegAddr = ((TRAFFIC_COND_MANAGEMENT_OFFSETT_BETWEEN_SETS_CNS)*
                        tcEntryPTR->meterManagementCounters-1);

        if(descrPtr->dropPrecedence == 0 /*green*/)
        {
            /* green octets */
            __LOG(("green octets"));
            currRegAddr += TRAFFIC_COND_GREEN_OCTET_LOW_REG;
            smemRegGet(devObjPtr,currRegAddr,&currRegValue);
            currRegValue +=descrPtr->byteCount;
            smemRegSet(devObjPtr,currRegAddr,currRegValue);

            /* green packets */
            __LOG(("green packets"));
            currRegAddr -= TRAFFIC_COND_GREEN_OCTET_LOW_REG;
            currRegAddr += TRAFFIC_COND_GREEN_PCKT_REG;
            smemRegGet(devObjPtr,currRegAddr,&currRegValue);
            currRegValue++;
            smemRegSet(devObjPtr,currRegAddr,currRegValue);
        }
        else if(descrPtr->dropPrecedence == 1 /*yellow*/)
        {
            /* yellow octets */
            __LOG(("yellow octets"));
            currRegAddr += TRAFFIC_COND_YELLOW_OCTET_LOW_REG;
            smemRegGet(devObjPtr,currRegAddr,&currRegValue);
            currRegValue +=descrPtr->byteCount;
            smemRegSet(devObjPtr,currRegAddr,currRegValue);

            /* yellow packets */
            __LOG(("yellow packets"));
            currRegAddr -= TRAFFIC_COND_YELLOW_OCTET_LOW_REG;
            currRegAddr += TRAFFIC_COND_YELLOW_PCKT_REG;
            smemRegGet(devObjPtr,currRegAddr,&currRegValue);
            currRegValue++;
            smemRegSet(devObjPtr,currRegAddr,currRegValue);
        }
        else if(descrPtr->dropPrecedence == 2 /*red*/)
        {
            if(descrPtr->tcbDidDrop==GT_FALSE)
            {
                /* red octets */
                __LOG(("red octets"));
                currRegAddr += TRAFFIC_COND_RED_OCTET_LOW_REG;
                smemRegGet(devObjPtr,currRegAddr,&currRegValue);
                currRegValue +=descrPtr->byteCount;
                smemRegSet(devObjPtr,currRegAddr,currRegValue);

                /* red packets */
                __LOG(("red packets"));
                currRegAddr -= TRAFFIC_COND_RED_OCTET_LOW_REG;
                currRegAddr += TRAFFIC_COND_RED_PCKT_REG;
                smemRegGet(devObjPtr,currRegAddr,&currRegValue);
                currRegValue++;
                smemRegSet(devObjPtr,currRegAddr,currRegValue);
            }
        }

        /* discard counters */
        if(descrPtr->tcbDidDrop)
        {
            /* discard octets */
            __LOG(("discard octets"));
            currRegAddr += TRAFFIC_COND_DISCARD_OCTET_LOW_REG;
            smemRegGet(devObjPtr,currRegAddr,&currRegValue);
            currRegValue +=descrPtr->byteCount;
            smemRegSet(devObjPtr,currRegAddr,currRegValue);

            /* discard packets */
            __LOG(("discard packets"));
            currRegAddr -= TRAFFIC_COND_DISCARD_OCTET_LOW_REG;
            currRegAddr += TRAFFIC_COND_DISCARD_PCKT_REG;
            smemRegGet(devObjPtr,currRegAddr,&currRegValue);
            currRegValue++;
            smemRegSet(devObjPtr,currRegAddr,currRegValue);
        }
    }
}

/*******************************************************************************
*   snetSambaTrafficConditionDoMeterRemark
*
* DESCRIPTION:
*        do remarking according to tc entry (CoS assignment)
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        tcEntryPTR - tc entry info
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static void snetSambaTrafficConditionDoMeterRemark
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    DECLARE_FUNC_NAME(snetSambaTrafficConditionDoMeterRemark);

    GT_U32  addressInDscpToCosRemarkTable;
    GT_U32  newCosParameters,tmpRegVal;
    GT_U8   dscp,dp,up,tc,exp;
    GT_U32  addressInTcCosRemarkTable;

    if(descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E ||
       descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
        return;
    }

    if(tcEntryPTR->remarkCosByDscp &&
        (descrPtr->flowTemplate== SKERNEL_TCP_FLOW_E ||
         descrPtr->flowTemplate== SKERNEL_UDP_FLOW_E ||
         descrPtr->flowTemplate== SKERNEL_IP_OTHER_E ||
         descrPtr->flowTemplate== SKERNEL_IP_FRAGMENT_E ||
         descrPtr->flowTemplate== SKERNEL_MPLS_FLOW_E))
    {
        /* get index in the table */
        __LOG(("get index in the table"));
        if(descrPtr->flowTemplate== SKERNEL_MPLS_FLOW_E)
        {
            addressInDscpToCosRemarkTable = 192 + (descrPtr->dropPrecedence*8)+
                                            descrPtr->exp;
        }
        else
        {
            addressInDscpToCosRemarkTable = (descrPtr->dropPrecedence*64) +
                                            descrPtr->dscp;
        }

        addressInDscpToCosRemarkTable*=4;/* multi by entry size */

        /* get new CoS parameters from the Remark table */
        __LOG(("get new CoS parameters from the Remark table"));
        addressInDscpToCosRemarkTable += SAMBA_COS_REMARK_REG;

        smemRegGet(devObjPtr,addressInDscpToCosRemarkTable,&newCosParameters);
    }
    else if(tcEntryPTR->remarkCosByUP)
    {
        /* do remark based on (TC/UP,CL)->CoS */
        __LOG(("do remark based on (TC/UP,CL)->CoS"));

        smemRegGet(devObjPtr, TCB_EXTENDED_CONTROL_REG,&tmpRegVal);

        if(SMEM_U32_GET_FIELD(tmpRegVal,14,1))
        {
            /* remark by UP */
            addressInTcCosRemarkTable = (descrPtr->dropPrecedence*8) +
                                    descrPtr->userPriorityTag ;
        }
        else
        {
            /* remark by TC */
            addressInTcCosRemarkTable = (descrPtr->dropPrecedence*8) +
                                    descrPtr->trafficClass ;
        }

        addressInTcCosRemarkTable*=4;/* multi by entry size */

        /* get new CoS parameters from the Remark table */
        addressInTcCosRemarkTable += (SAMBA_COS_REMARK_REG+228*4);

        smemRegGet(devObjPtr,addressInTcCosRemarkTable,&newCosParameters);
    }
    else
    {
        return;
    }

    dscp= (GT_U8)SMEM_U32_GET_FIELD(newCosParameters,0,6);
    dp=   (GT_U8)SMEM_U32_GET_FIELD(newCosParameters,6,2);
    tc=   (GT_U8)SMEM_U32_GET_FIELD(newCosParameters,8,3);
    up=   (GT_U8)SMEM_U32_GET_FIELD(newCosParameters,11,3);
    exp=  (GT_U8)SMEM_U32_GET_FIELD(newCosParameters,14,3);

    if(tcEntryPTR->enModifyTC)
    {
        descrPtr->trafficClass = tc;
    }

    smemRegGet(devObjPtr, TCB_CONTROL_REG,&tmpRegVal);

    if(SMEM_U32_GET_FIELD(tmpRegVal,31,1))
    {
        /* enable modify DP */
        descrPtr->dropPrecedence = dp;
    }

    if(tcEntryPTR->enModifyExtCoS)
    {
        descrPtr->modifyDscpOrExp = GT_TRUE;
        descrPtr->dscp = dscp;
        descrPtr->exp = exp;

        if(SMEM_U32_GET_FIELD(tmpRegVal,29,1))
        {
            /* enable modify UP for ip traffic */
            descrPtr->userPriorityTag = up;
        }
    }

    if(descrPtr->dropPrecedence==2 && tcEntryPTR->dropRed)/* red */
    {
        /* we need to drop "red" packets */
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        descrPtr->tcbDidDrop = GT_TRUE;
    }
}

/*******************************************************************************
*   snetTwistTrafficConditionDoMeter
*
* DESCRIPTION:
*        do metering (remarking and meter counting) according to tc entry
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        tcEntryPTR - tc entry info
* OUTPUTS:
*        tcEntryPTR - pointer to update counters in
*
*
*******************************************************************************/
static void snetTwistTrafficConditionDoMeter
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    DECLARE_FUNC_NAME(snetTwistTrafficConditionDoMeter);

    GT_U32  tcbRegVal;
    GT_U32  enPolicing;

    smemRegGet(devObjPtr, TCB_CONTROL_REG,&tcbRegVal);

    enPolicing = SMEM_U32_GET_FIELD(tcbRegVal,3,1);

    if(enPolicing==0)
    {
        /* metering disabled */
        __LOG(("metering disabled"));
        return ;
    }

    /* do remarking (CoS params) */
    __LOG(("do remarking (CoS params)"));
    if(devObjPtr->deviceFamily ==SKERNEL_SAMBA_FAMILY)
    {
        snetSambaTrafficConditionDoMeterRemark(devObjPtr,descrPtr,tcEntryPTR);
    }

    /* do meter counting */
    __LOG(("do meter counting"));
    snetTwistTrafficConditionDoMeterCounters(devObjPtr,descrPtr,tcEntryPTR);
}

/*******************************************************************************
*   snetTwistTrafficConditionDoConformanceCount
*
* DESCRIPTION:
*        do conformance counting according to tc entry
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        tcEntryPTR - tc entry info
* OUTPUTS:
*        tcEntryPTR - pointer to update counters in
*
*
*******************************************************************************/
static void snetTwistTrafficConditionDoConformanceCount
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    GT_U32  duCount;
    GT_U32  tcbRegVal;
    GT_U32  enPolicingCounter;
    GT_U32  *counterPtr;

    smemRegGet(devObjPtr, TCB_CONTROL_REG,&tcbRegVal);

    enPolicingCounter = SMEM_U32_GET_FIELD(tcbRegVal,5,1);

    if(enPolicingCounter==0)
    {
        return;
    }

    duCount = (tcEntryPTR->counterDu==1?
               (descrPtr->byteCount/16):
                descrPtr->byteCount );


    if(descrPtr->dropPrecedence == 0 /*green*/)
    {
        counterPtr = &tcEntryPTR->conformanceCounters.green.l[0];
    }
    else if(descrPtr->dropPrecedence == 1 /*yellow*/)
    {
        counterPtr = &tcEntryPTR->conformanceCounters.yellow.l[0];
    }
    else if(descrPtr->dropPrecedence == 2 /*red*/)
    {
        counterPtr = &tcEntryPTR->conformanceCounters.red.l[0];
    }
    else
    {
        return;
    }

    (*counterPtr) += duCount;
}

/*******************************************************************************
*   snetSambaTrafficConditionWriteBackByParams
*
* DESCRIPTION:
*        write back to the memory the values of the tc entry
*        we need to update the entries counters
* INPUTS:
*        deviceObj - pointer to device object.
*        tcIndex  - index of the tc entry
*        tcEntryPTR - tc entry info
*        doResetCounters - do reset on the counters specified by
*                       doResetCountersFlag
*        doResetCountersFlag - counters reset flags
*
*
*******************************************************************************/
static void snetSambaTrafficConditionWriteBackByParams
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32                  tcIndex,
    IN    SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR,
    IN    GT_BOOL                 doResetCounters,
    IN    GT_U32                  doResetCountersFlag
)
{
    SNET_SAMABA_PP_TRAFF_COND_ENTRY_STC *tmpTcEntryAsicFormatPTR;
    GT_U32      tcEntryAddress;
    GT_U32      pksCountLimit,cduCountLimit;
    GT_BOOL     overFlowPktCountLimit = GT_FALSE,overFlowCduCountLimit = GT_FALSE;
    GT_U32      policyCountLimit;
    GT_BOOL     overFlowPolicyCountLimit[3]={GT_FALSE,GT_FALSE,GT_FALSE};
    GT_U32      tmpRegVal;
    GT_BOOL     didModifyEntry=GT_FALSE ;

    /* get TC table base address */
    smemRegFldGet(devObjPtr,TWIST_TRAFFIC_COND_TABLE_BASE_ADDR_REG,
                0,22,&tcEntryAddress);

    tcEntryAddress <<= 4;

    /* calculate the actual address of entry */
    tcEntryAddress += SNET_SAMBA_INTERNAL_MEM_BASE_ADDRESS +
                      (tcIndex<<4) ;

    tmpTcEntryAsicFormatPTR = smemMemGet(devObjPtr,tcEntryAddress);

    /* convert generic simulation format to PP format */

    if(tcEntryPTR->doMetering || doResetCounters )
    {
        didModifyEntry = GT_TRUE;
        switch(tcEntryPTR->counterMode)
        {
            case 0:
            case 3:
                /* cdu and packets */

                smemRegFldGet(devObjPtr,TCB_PACKET_COUNT_LIMIT_REG,
                            0,22,&pksCountLimit);

                smemRegFldGet(devObjPtr,TCB_CDU_COUNT_LIMIT_REG,
                            0,26,&cduCountLimit);

                /* check alarm counters limitations */
                if(cduCountLimit <
                    (tcEntryPTR->meterCounters.cdu.l[0] & 0x3ffffff))/*26bits*/
                {
                    /* over flow */
                    overFlowCduCountLimit = GT_TRUE;
                }
                else
                {
                    tmpTcEntryAsicFormatPTR->meterCounters.both.cdu =
                        tcEntryPTR->meterCounters.cdu.l[0] & 0x3ffffff;/*26bits*/
                }

                if(pksCountLimit <
                    (tcEntryPTR->meterCounters.packets.l[0] & 0x3fffff))/*22bits*/
                {
                    /* over flow */
                    overFlowPktCountLimit = GT_TRUE;
                }
                else
                {
                    tmpTcEntryAsicFormatPTR->meterCounters.both.packets_lsb =
                        tcEntryPTR->meterCounters.packets.l[0] & 0x3f;/* 6 bits*/
                    tmpTcEntryAsicFormatPTR->meterCounters.both.packets_msb =
                        (tcEntryPTR->meterCounters.packets.l[0]>>6) & 0xffff;/* 16 bits*/
                }
                break;
            case 1:
                /* cdu only */
                smemRegFldGet(devObjPtr,SAMBA_TCB_LARGE_COUNT_LIMIT_MSB_REG,
                            0,16,&cduCountLimit);

                if(cduCountLimit==0)
                {
                    smemRegGet(devObjPtr,SAMBA_TCB_CDU_LARGE_COUNT_LIMIT_LSB_REG,
                                &cduCountLimit);
                }
                else
                {
                    cduCountLimit = 0xffffffff;/*support only 32 bits*/
                }

                if(cduCountLimit <
                    tcEntryPTR->meterCounters.cdu.l[0])
                {
                    /* over flow */
                    overFlowCduCountLimit = GT_TRUE;
                }
                else
                {
                    tmpTcEntryAsicFormatPTR->meterCounters.cduOnly.cdu_lsb =
                        tcEntryPTR->meterCounters.cdu.l[0];
                }
                break;
            case 2:
                /* packets only */
                smemRegFldGet(devObjPtr,SAMBA_TCB_LARGE_COUNT_LIMIT_MSB_REG,
                            16,16,&pksCountLimit);

                if(pksCountLimit==0)
                {
                    smemRegGet(devObjPtr,SAMBA_TCB_CDU_LARGE_COUNT_LIMIT_LSB_REG,
                                &pksCountLimit);
                }
                else
                {
                    pksCountLimit = 0xffffffff;/*support only 32 bits*/
                }

                if(pksCountLimit <
                    tcEntryPTR->meterCounters.packets.l[0])
                {
                    /* over flow */
                    overFlowPktCountLimit = GT_TRUE;
                }
                else
                {
                    tmpTcEntryAsicFormatPTR->meterCounters.packetsOnly.packets_lsb =
                        tcEntryPTR->meterCounters.packets.l[0];
                }
                break;
            default:
                skernelFatalError(" snetSambaTrafficConditionWriteBack: not valid mode[%d]",
                                tcEntryPTR->counterMode);
        }

        if(overFlowPktCountLimit || overFlowCduCountLimit)
        {
            /* handle over flow */
            didModifyEntry = GT_FALSE;

            smemRegGet(devObjPtr,TCB_COUNTER_ALARM_REG,&tmpRegVal);

            if(SMEM_U32_GET_FIELD(tmpRegVal,0,1)==0)
            {
                SMEM_U32_SET_FIELD(tmpRegVal,0,1,1);
                SMEM_U32_SET_FIELD(tmpRegVal,1,1,overFlowCduCountLimit);
                SMEM_U32_SET_FIELD(tmpRegVal,2,1,overFlowPktCountLimit);
                SMEM_U32_SET_FIELD(tmpRegVal,3,16,tcIndex);
                smemRegSet(devObjPtr,TCB_COUNTER_ALARM_REG,tmpRegVal);

                /*** do interrupt *****/
                snetSambaTrafficConditionCauseInterrupt(devObjPtr,
                    SAMBA_TRAFFIC_COND_INTERRUPT_METER_COUNT_OVERFLOW_E);
            }
        }
    }

    if(tcEntryPTR->doCounting || (doResetCounters && doResetCountersFlag==1))
    {
        didModifyEntry = GT_TRUE;

        smemRegGet(devObjPtr,TCB_POLICING_COUNT_LIMIT_HI_REG,
                    &policyCountLimit);

        if(policyCountLimit==0)
        {
            smemRegGet(devObjPtr,TCB_POLICING_COUNT_LIMIT_LOW_REG,
                        &policyCountLimit);
        }
        else
        {
            policyCountLimit = 0xffffffff;/*support 32 bits*/
        }

        if(policyCountLimit < tcEntryPTR->conformanceCounters.green.l[0])
        {
            overFlowPolicyCountLimit[0]=GT_TRUE ;
        }
        else
        {
            tmpTcEntryAsicFormatPTR->conformanceCounters.green_lsb =
                tcEntryPTR->conformanceCounters.green.l[0];/* 32 bits */
            tmpTcEntryAsicFormatPTR->conformanceCounters.green_msb =
                tcEntryPTR->conformanceCounters.green.l[1]&0x3ff;/*10bits*/
        }

        if(policyCountLimit < tcEntryPTR->conformanceCounters.yellow.l[0])
        {
            overFlowPolicyCountLimit[1]=GT_TRUE ;
        }
        else
        {
            tmpTcEntryAsicFormatPTR->conformanceCounters.yellow_lsb=
                (tcEntryPTR->conformanceCounters.yellow.l[0]&0x3fffff);/* 22 bits */
            tmpTcEntryAsicFormatPTR->conformanceCounters.yellow_msb=
                ((tcEntryPTR->conformanceCounters.yellow.l[0]>>22)&0x3ff)|/* 10 bits */
                ((tcEntryPTR->conformanceCounters.yellow.l[1]&0x3ff)<<10);/* 10 bits */
        }

        if(policyCountLimit < tcEntryPTR->conformanceCounters.red.l[0])
        {
            overFlowPolicyCountLimit[2]=GT_TRUE ;
        }
        else
        {
            tmpTcEntryAsicFormatPTR->conformanceCounters.red_lsb=
                tcEntryPTR->conformanceCounters.red.l[0]&0xfff;/*12 bits*/
            tmpTcEntryAsicFormatPTR->conformanceCounters.red_msb=
                ((tcEntryPTR->conformanceCounters.red.l[0]>>12)&0xfffff)|/*20 bits*/
                ((tcEntryPTR->conformanceCounters.red.l[1]&0x3ff)<<20);/*10 bits*/
        }

        if(overFlowPolicyCountLimit[0] ||
           overFlowPolicyCountLimit[1] ||
           overFlowPolicyCountLimit[2] )
        {
            /* handle over flow */
            didModifyEntry = GT_FALSE;

            smemRegGet(devObjPtr,TCB_POLICING_COUNTER_ALARM_REG,&tmpRegVal);

            if(SMEM_U32_GET_FIELD(tmpRegVal,0,1)==0)
            {
                SMEM_U32_SET_FIELD(tmpRegVal,0,1,1);
                SMEM_U32_SET_FIELD(tmpRegVal,1,1,overFlowPolicyCountLimit[2]);
                SMEM_U32_SET_FIELD(tmpRegVal,2,1,overFlowPolicyCountLimit[1]);
                SMEM_U32_SET_FIELD(tmpRegVal,3,1,overFlowPolicyCountLimit[0]);
                SMEM_U32_SET_FIELD(tmpRegVal,4,16,tcIndex);
                smemRegSet(devObjPtr,TCB_POLICING_COUNTER_ALARM_REG,tmpRegVal);

                /*** do interrupt *****/
                snetSambaTrafficConditionCauseInterrupt(devObjPtr,
                    SAMBA_TRAFFIC_COND_INTERRUPT_POLICING_COUNT_OVERFLOW_E);
            }
        }
    }

    if(didModifyEntry==GT_TRUE)
    {
        /* we need to update the visualizer */
        smemMemSet(devObjPtr,
                    tcEntryAddress+0x10,/* skip to the start of counters */
                    (GT_U32*)((GT_U8*)tmpTcEntryAsicFormatPTR+0x10),
                    (0x20/4));/* size of counters (in words) */
    }


    return;
}


/*******************************************************************************
*   snetSambaTrafficConditionWriteBack
*
* DESCRIPTION:
*        write back to the memory the values of the tc entry
*        we need to update the entries counters
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        tcEntryPTR - tc entry info
*
*
*******************************************************************************/
static void snetSambaTrafficConditionWriteBack
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    snetSambaTrafficConditionWriteBackByParams(devObjPtr,
                                               descrPtr->traffCondOffset,
                                               tcEntryPTR,
                                               GT_FALSE,
                                               0);
}

/*******************************************************************************
*   snetTwistTrafficConditionWriteBack
*
* DESCRIPTION:
*        write back to the memory the values of the tc entry
*        we need to update the entries counters
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        tcEntryPTR - tc entry info
*
*
*******************************************************************************/
static void snetTwistTrafficConditionWriteBack
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC *tcEntryPTR
)
{
    DECLARE_FUNC_NAME(snetTwistTrafficConditionWriteBack);

    if (devObjPtr->deviceFamily ==SKERNEL_SAMBA_FAMILY)
    {
        snetSambaTrafficConditionWriteBack(devObjPtr,descrPtr,tcEntryPTR);
    }
    else
    {
        /* not supported yet */
        __LOG(("not supported yet"));
    }
}

/*******************************************************************************
*   snetTwistTrafficConditionDoGlobalCounters
*
* DESCRIPTION:
*        do counting of Rx traffic to the TCB and discard traffic by TCB
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*
*******************************************************************************/
static void snetTwistTrafficConditionDoGlobalCounters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistTrafficConditionDoGlobalCounters);

    GT_U32  tmpRegVal;

    /* rx packets */
    smemRegGet(devObjPtr,TCB_GLOBAL_RECEIVED_PACKETS_REG,&tmpRegVal);
    tmpRegVal++;
    smemRegSet(devObjPtr,TCB_GLOBAL_RECEIVED_PACKETS_REG,tmpRegVal);

    /* rx octets */
    smemRegGet(devObjPtr,TCB_GLOBAL_RECEIVED_OCTETS_LOW_REG,&tmpRegVal);
    tmpRegVal+=descrPtr->byteCount;
    smemRegSet(devObjPtr,TCB_GLOBAL_RECEIVED_OCTETS_LOW_REG,tmpRegVal);

    if(descrPtr->tcbDidDrop)
    {
        /* dropped packets */
        __LOG(("dropped packets"));
        smemRegGet(devObjPtr,TCB_GLOBAL_DROPPED_PACKETS_REG,&tmpRegVal);
        tmpRegVal++;
        smemRegSet(devObjPtr,TCB_GLOBAL_DROPPED_PACKETS_REG,tmpRegVal);

        /* dropped octets */
        __LOG(("dropped octets"));
        smemRegGet(devObjPtr,TCB_GLOBAL_DROPPED_OCTETS_LOW_REG,&tmpRegVal);
        tmpRegVal+=descrPtr->byteCount;
        smemRegSet(devObjPtr,TCB_GLOBAL_DROPPED_OCTETS_LOW_REG,tmpRegVal);
    }

}

/*******************************************************************************
*   snetTwistTrafficConditionProcess
*
* DESCRIPTION:
*        make traffic condition processing
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        GT_TRUE   - continue frame processing
*        GT_FALSE  - stop frame processing
*******************************************************************************/
extern GT_BOOL snetTwistTrafficConditionProcess
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistTrafficConditionProcess);

    SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC tcEntry;/* shadow entry in generic format */

    if(descrPtr->activateTC || descrPtr->activateCount)
    {
        /* get the TC entry */
        __LOG(("get the TC entry"));
        snetTwistTrafficConditionEntryGet(devObjPtr,descrPtr,&tcEntry);

        if(tcEntry.doMetering)
        {
            /* do remarking and meter counting */
            __LOG(("do remarking and meter counting"));
            snetTwistTrafficConditionDoMeter(devObjPtr,descrPtr,&tcEntry);
        }

        if(tcEntry.doCounting)
        {
            /* do conformance counting */
            __LOG(("do conformance counting"));
            snetTwistTrafficConditionDoConformanceCount(devObjPtr,descrPtr,&tcEntry);
        }

        if(tcEntry.doMetering || tcEntry.doCounting)
        {
            /* update the Tc entry (in the memory) about all the counters that
               where changed */
            snetTwistTrafficConditionWriteBack(devObjPtr,descrPtr,&tcEntry);
        }
    }

    /* update global counters */
    __LOG(("update global counters"));
    snetTwistTrafficConditionDoGlobalCounters(devObjPtr,descrPtr);

    return GT_TRUE;
}


/*******************************************************************************
*   snetSambaTrafficConditionWriteGenxsReg
*
* DESCRIPTION:
*        in the samba you can set action of reset Tc entry counters and the "old"
*       values are going to the genxs global counters
* INPUTS:
*        deviceObj - pointer to device object.
*        tcIndex   - index of tcEntry
* OUTPUTS:
*        None
* COMMENT:
*       fatal error on error
*
*******************************************************************************/
extern GT_VOID snetSambaTrafficConditionWriteGenxsReg
(
    IN    SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN    GT_U32                tcIndex,
    IN    GT_U32                resetContersFlag
)
{
    SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC tcEntry;
    GT_U32      tmpWord1,tmpWord2;

    /* get the specific TC entry */
    snetSambaTrafficConditionEntryGetByParams(deviceObjPtr,
                                              tcIndex,
                                              GT_FALSE,/* not relevant */
                                              GT_FALSE,/* not relevant */
                                              &tcEntry);

    switch(tcEntry.counterMode)
    {
        case 1:
            /* cdu only */
            /* 32 lsb */
            tmpWord1 = tcEntry.meterCounters.cdu.l[0];
            /* 16 msb */
            tmpWord2 = tcEntry.meterCounters.cdu.l[1] & 0xffff;
            break;
        case 2:
            /* packets only */
            /* 32 lsb */
            tmpWord1 = tcEntry.meterCounters.packets.l[0];
            /* 16 msb */
            tmpWord2 = tcEntry.meterCounters.packets.l[1] & 0xffff;
            break;
        case 0:
        case 3:
        default:
            /* 26 cdu , 6 lsb packets */
            tmpWord1 = ((tcEntry.meterCounters.packets.l[0]&0x3f)<<26) | /*6*/
                        (tcEntry.meterCounters.cdu.l[0]&0x3ffffff);/*26*/
            /* 16 msb packets */
            tmpWord2 = (tcEntry.meterCounters.packets.l[0]>>6) & 0xffff;
            /* both packets and cdu */
            break;
    }

    /* read and reset cdu and packets counter */
    smemRegSet(deviceObjPtr,SAMBA_TCB_READ_METER_COUNTER_1_REG,
               tmpWord1);
    smemRegSet(deviceObjPtr,SAMBA_TCB_READ_METER_COUNTER_2_REG,
               tmpWord2);
    tcEntry.meterCounters.cdu.l[0]=0;
    tcEntry.meterCounters.cdu.l[1]=0;
    tcEntry.meterCounters.packets.l[0]=0;
    tcEntry.meterCounters.packets.l[1]=0;

    if(resetContersFlag==1)
    {
        /* also read and reset policing counters */

        /* 32 lsb from green */
        smemRegSet(deviceObjPtr,SAMBA_TCB_READ_POLICY_COUNTER_1_REG,
                   tcEntry.conformanceCounters.green.l[0]);

        /* 10 msb from green
           22 lsb from yellow */
        smemRegSet(deviceObjPtr,SAMBA_TCB_READ_POLICY_COUNTER_2_REG,
                   (tcEntry.conformanceCounters.green.l[1] &0x3ff) |/*10*/
                   ((tcEntry.conformanceCounters.yellow.l[0]&0x3fffff)<<10));/*22*/


        /* 10 msb from yellow_0
           10 lsb from yellow_1
           12 lsb from red 1
           */
        smemRegSet(deviceObjPtr,SAMBA_TCB_READ_POLICY_COUNTER_3_REG,
                   ((tcEntry.conformanceCounters.yellow.l[0]>>22) &0x3ff) |/*10*/
                   ((tcEntry.conformanceCounters.yellow.l[1]&0x3ff)<<10) |/*10*/
                   ((tcEntry.conformanceCounters.red.l[0]&0xfff )<<20));/*12*/

        /* 20 msb from red 0
           10 lsb from red 1
        */
        smemRegSet(deviceObjPtr,SAMBA_TCB_READ_POLICY_COUNTER_4_REG,
                   ((tcEntry.conformanceCounters.red.l[0]>>12)&0xfffff)|/*20*/
                   ((tcEntry.conformanceCounters.red.l[1]&0x3ff)<<20));/*10*/


        tcEntry.conformanceCounters.green.l[0]=0;
        tcEntry.conformanceCounters.green.l[1]=0;

        tcEntry.conformanceCounters.yellow.l[0]=0;
        tcEntry.conformanceCounters.yellow.l[1]=0;
        tcEntry.conformanceCounters.red.l[0]=0;
        tcEntry.conformanceCounters.red.l[1]=0;
    }


    /* write the values back to the TC entry */
    snetSambaTrafficConditionWriteBackByParams(deviceObjPtr,tcIndex,
                                &tcEntry,GT_TRUE,resetContersFlag);

    /* set bit 31 so PSS will know that the action of "reset" the Traffic
       condition counters is done */
    smemRegFldSet(deviceObjPtr, SAMBA_TCB_GENXS_REG, 31, 1, 1);
    return;
}



