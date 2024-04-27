/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSambaPolicy.c
*
* DESCRIPTION:
*       Policy Engine of Samba
*       calling PCL Engine and Classifier Engine
*       for applying actions on the descriptor (that will be of the frame)
*
*       implementation according to REG_SIM_POLICY_SAMBA_FRDTD document
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SLog/simLog.h>
/*******************************************************************************
*   snetSambaPolicyFinalActionFieldDecision
*
* DESCRIPTION:
*        The function get decisionMode  and decide which one between pcl/gfc
*        will be in the final policy actions
*
* INPUTS:
*        classifierFound - the action was found in classifier
*        pclFound  - the action was found in pcl
*        decisionMode : the value of the field from the register 0x028001E4 to be tested .
*                 0 - always take from pcl
*                 1- always take from gfc
*                 2- take from the one that was found on , if found on both or on none , pcl
*                     has precedence
*                 3- take from the one that was found on , if found on both or on none , gfc
*                     has precedence
*
* OUTPUTS:
*        takePclPtr - do we need to use the value from pcl actions or
*                     from classifier actions
*
*COMMENTS:
*******************************************************************************/
static void snetSambaPolicyFinalActionFieldDecision(
    IN    GT_BOOL classifierFound,
    IN    GT_BOOL pclFound,
    IN    GT_U32  decisionMode,
    OUT   GT_BOOL *takePclPtr
)
{
    switch (decisionMode)
    {
        case 0:
            * takePclPtr = GT_TRUE; /* take from pcl */
            break;
        case 1:
            * takePclPtr = GT_FALSE; /* take from gfc*/
            break;
        case 2:
            if(classifierFound == pclFound)
            {
                * takePclPtr = GT_TRUE ;/* take from pcl */
            }
            else if (pclFound == GT_TRUE)
            {
                * takePclPtr = GT_TRUE; /* take from pcl */
            }
            else
            {
                * takePclPtr = GT_FALSE;  /* take from gfc*/
            }
            break;
        case 3:
            if(classifierFound == pclFound)
            {
                * takePclPtr = GT_FALSE ;/* take from gfc */
            }
            else if (pclFound == GT_TRUE)
            {
                * takePclPtr = GT_TRUE; /* take from pcl */
            }
            else
            {
                * takePclPtr = GT_FALSE; /* take from gfc*/
            }
            break;
        default:
            skernelFatalError(" snetSambaPolicyFinalActionFieldDecision: "\
                "decisionMode [%d] out of range[%d]",decisionMode,3);
            break;
    }

    return;
}


/*******************************************************************************
*   snetSambaPolicyFinalActionGet
*
* DESCRIPTION:
*        Decide what actions to take from GFC/PCL according to Policy Final
*       Actions Setting Table Register Address 0x028001E4 (table 469 [4] )
*
*       set the FinalActionPtr according to info from classifierPolicyActionPtr
*       and/or pclPolicyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        pclPolicyActionPtr - the actions from the PCL in the policy format
*        classifierPolicyActionPtr - was the classifier found of the default
*                                   actions used (of the template)
*        classifierFound - was the classifier/CoS found entry or
*                             the default entry was taken .
*        pclFoundFirst - was the first pcl found entry or
*                             the default entry was taken .
*        pclFoundSecon - was the second pcl found entry or
*                             the default entry was taken .
*       pclDone - did we enter the pcl engine
*       classifierDone - did we enter the classifiers engine
* OUTPUTS:
*        descrPtr - pointer to the frame's descriptor , with after apply actions
*
*COMMENTS:
*******************************************************************************/
static void snetSambaPolicyFinalActionGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *classifierPolicyActionPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *pclPolicyActionPtr,
    IN    GT_BOOL classifierFound,
    IN    GT_BOOL pclFoundFirst,
    IN    GT_BOOL pclFoundSecond,
    IN    GT_BOOL pclDone,
    IN    GT_BOOL classifierDone,
    OUT   SNET_SAMBA_POLICY_ACTION *finalActionPtr
)
{
    DECLARE_FUNC_NAME(snetSambaPolicyFinalActionGet);

    SNET_SAMBA_POLICY_FINAL_SETTING policyFinalSetting;/* fields from
                                                          register 0x028001E4 */
    GT_U32 overridePclHit ;/*value to decide how to use the pcl matching .*/
    GT_BOOL takePcl ;/*is to take value from pcl actions or from the classifier actions*/
    GT_BOOL pclFound;/*the combination of pcl first,and second matching and
                    the overridePclHit value*/
    GT_BOOL workInSpecialMode = GT_FALSE;/*do we work in special command mode ,
                                           see the cmd field*/
    SNET_SAMBA_POLICY_ACTION *tmpPolicyPtr;

/* macro to use only in this specific function */
#define POLICY_CHOOSE_FINAL_ACTION(takePcl) \
    if(takePcl == 1) { tmpPolicyPtr = pclPolicyActionPtr ; }  \
    else{tmpPolicyPtr = classifierPolicyActionPtr;}

    /* SET overridePclHit ACCORDING to bit [1] in register : 0x028001E8 */
    __LOG(("SET overridePclHit ACCORDING to bit [1] in register : 0x028001E8"));
    smemRegFldGet(devObjPtr, POLICY_CONTROL_REG, 1, 1,&overridePclHit);

    if(overridePclHit == 0)
    {
        pclFound = ( pclDone == 1 &&
                    (pclFoundFirst == 1 || pclFoundSecond == 1 )) ;
    }
    else
    {
        pclFound = pclDone;
    }

    /*SET policyFinalSetting ACCORDING TO register 0x028001E4*/
    smemRegGet(devObjPtr, SAMBA_POLICY_FINAL_ACTION_SETTING_TABLE_REG,
            (void *)&policyFinalSetting);

    /* Cmd */
    if (policyFinalSetting.cmd <= 3)
    {
        snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                        policyFinalSetting.cmd,
                                        &takePcl );
        POLICY_CHOOSE_FINAL_ACTION(takePcl);
        finalActionPtr->cmd = tmpPolicyPtr->cmd;
        finalActionPtr->trapActionSource = tmpPolicyPtr->trapActionSource;
    }
    else if (policyFinalSetting.cmd == 4)
    {
        workInSpecialMode = GT_TRUE; /* special command mode */

        if(pclPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ||
           classifierPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E)
        {
            finalActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ;
        }
        else if (pclPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E  ||
               classifierPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
        {
            finalActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_DROP_E ;
        }
        else
        {
            finalActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_PROCESS_E ;
        }
    }
    else
    {
        skernelFatalError(" snetSambaPolicyFinalActionGet: "\
            "policyFinalSetting.cmd[%d] not allowed",
            policyFinalSetting.cmd);
    }

    /* explore the Cmd further ---  see 1.5.5 [5] */
    if( workInSpecialMode &&
        ((pclPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E  &&
            classifierPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E )
                ||
         (pclPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E  &&
            classifierPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E ))
      )
    {
        /* we take all the actions from pcl action */
        memcpy(finalActionPtr,pclPolicyActionPtr,
                sizeof(SNET_SAMBA_POLICY_ACTION));
        return;
    }
    else if(finalActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ||
            finalActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
    {
        /* take all  commands from the BLOCK that command came from */
        if(pclPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E  ||
           pclPolicyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
        {
            /* we take all the actions from pcl action */
            memcpy(finalActionPtr,pclPolicyActionPtr,
                    sizeof(SNET_SAMBA_POLICY_ACTION));
        }
        else
        {
            /* we take all the actions from classifier action */
            memcpy(finalActionPtr,classifierPolicyActionPtr,
                    sizeof(SNET_SAMBA_POLICY_ACTION));
        }
        return;
    }

    /* mirrorToAnalyzerPort */
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.mirrorToAnalyzerPort,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);

    finalActionPtr->mirrorToAnalyzerPort = tmpPolicyPtr->mirrorToAnalyzerPort;

    /* ActiveTcOrCnt*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.activateTC,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->activateTC  = tmpPolicyPtr->activateTC;
    finalActionPtr->activateCount = tmpPolicyPtr->activateCount ;
    finalActionPtr->tcOrCnt_Ptr = tmpPolicyPtr->tcOrCnt_Ptr;

    descrPtr->activateTC    = finalActionPtr->activateTC;
    descrPtr->activateCount = finalActionPtr->activateCount;
    descrPtr->traffCondOffset = finalActionPtr->tcOrCnt_Ptr;

    /* MarkCmd*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.mark_Cmd,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->mark_Cmd  = tmpPolicyPtr->mark_Cmd;
    /* dscp */
    finalActionPtr->dscp = tmpPolicyPtr->dscp;
    /* marking */
    finalActionPtr->markDscp = tmpPolicyPtr->markDscp;
    finalActionPtr->markTc =  tmpPolicyPtr->markTc;
    finalActionPtr->markUp =  tmpPolicyPtr->markUp;
    finalActionPtr->markDp =  tmpPolicyPtr->markDp;
    /* cos parameters*/
    finalActionPtr->tc = tmpPolicyPtr->tc;
    finalActionPtr->up = tmpPolicyPtr->up;
    finalActionPtr->dp = tmpPolicyPtr->dp;


    /* RedirectFlowCmd*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.redirectFlowCmd,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->redirectFlowCmd = tmpPolicyPtr->redirectFlowCmd;
    finalActionPtr->outlifOrIPv4_Ptr = tmpPolicyPtr->outlifOrIPv4_Ptr;

    /* SetLBH*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.setLBH,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->setLBH = tmpPolicyPtr->setLBH;
    finalActionPtr->LBH = tmpPolicyPtr->LBH;

    /* NestedVlan*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.nestedVlan,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->nestedVlan = tmpPolicyPtr->nestedVlan;

    /* ModifyVlan*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.modifyVlan,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->modifyVlan = tmpPolicyPtr->modifyVlan;
    finalActionPtr->vid = tmpPolicyPtr->vid;

    /* FlowID*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.flowId,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->flowId = tmpPolicyPtr->flowId;

    /* MirrorToCPU*/
    snetSambaPolicyFinalActionFieldDecision(classifierFound, pclFound,
                                    policyFinalSetting.mirrorToCpu,
                                    &takePcl );
    POLICY_CHOOSE_FINAL_ACTION(takePcl);
    finalActionPtr->mirrorToCpu = tmpPolicyPtr->mirrorToCpu;

    return;
#undef POLICY_CHOOSE_FINAL_ACTION
}

/*******************************************************************************
*   snetSambaPolicyMark
*
* DESCRIPTION:
*        The function apply the final marking/remarking of the policy on the
*        packet's descriptor for frames that are not dropped or trapped to cpu
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        finalPolicyActionsPtr - the policy actions to apply
* OUTPUTS:
*        descrPtr - modified descriptor according to the finalPolicyActionsPtr
*
*COMMENTS:
*******************************************************************************/
static void snetSambaPolicyMark
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_POLICY_ACTION *finalPolicyActionsPtr
)
{
    DECLARE_FUNC_NAME(snetSambaPolicyMark);

    GT_U32 dscp ; /* dscp to insert to descriptor */
    GT_U8 dp , tc , up ;/* dropPrec,TrafficClass , userPrio*/
    GT_BOOL doMappingDscpToCoS = GT_FALSE;
    GT_U32 regVal;/* register value for mapping dscp to Cos params */

    dscp = descrPtr->dscp ;
    tc = descrPtr-> trafficClass;
    up = descrPtr-> userPriorityTag;
    dp = descrPtr-> dropPrecedence;

    if(finalPolicyActionsPtr->mark_Cmd == SKERNEL_PCL_COS_ENTRY_E)
    {
        dp = finalPolicyActionsPtr->dp ;
        tc = finalPolicyActionsPtr->tc ;
        up = finalPolicyActionsPtr->up ;
        dscp = finalPolicyActionsPtr->dscp ;
    }
    else if(finalPolicyActionsPtr->mark_Cmd == SKERNEL_PCL_L2_COS_E)
    {
        /* do nothing */
        __LOG(("do nothing"));
    }
    else if (finalPolicyActionsPtr->mark_Cmd ==
                SKERNEL_PCL_DSCP_ENTRY_E)
    {
        dscp = finalPolicyActionsPtr->dscp ;
        doMappingDscpToCoS = GT_TRUE;
    }
    else
    {
        doMappingDscpToCoS = GT_TRUE;
    }

    if (doMappingDscpToCoS)
    {
        /*Use Cos Mapping Reststers in 0x02CA0000 */
        /*GET regVal FROM register at (0x02CA0000 + dscp<<2) ;*/
        smemRegGet(devObjPtr, COS_MARK_REMARK_REG+ (dscp<<2),&regVal);

        dp = (GT_U8)(regVal >> 6 ) & 0x3;
        tc = (GT_U8)(regVal >> 8 ) & 0x7;
        up = (GT_U8)(regVal >> 11 ) & 0x7;
    }

    if(finalPolicyActionsPtr->markDp)
    {
        descrPtr->dropPrecedence = dp ;
    }

    if(finalPolicyActionsPtr->markTc)
    {
        descrPtr->trafficClass = tc ;
    }

    if(finalPolicyActionsPtr->markUp)
    {
        descrPtr->userPriorityTag = up ;
    }

    if(finalPolicyActionsPtr->markDscp)
    {
        descrPtr->modifyDscpOrExp = GT_TRUE;
        descrPtr->dscp = (GT_U8)dscp;
    }

    if (finalPolicyActionsPtr->setLBH)
    {
        /*TBD*/
    }

    if(finalPolicyActionsPtr->nestedVlan)
    {
        /*TBD*/
    }

    if(finalPolicyActionsPtr->modifyVlan)
    {
        descrPtr->vid = finalPolicyActionsPtr->vid ;
    }

    descrPtr->flowId =  finalPolicyActionsPtr->flowId;

    return;
}


/*******************************************************************************
*   snetSambaPolicyFinalActionProcessPacketApply
*
* DESCRIPTION:
*        The function apply the final actions of the policy on the packet's
*       descriptor for frames that are not dropped or trapped to cpu
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        finalPolicyActionsPtr - the policy actions to apply
* OUTPUTS:
*        descrPtr - modified descriptor according to the finalPolicyActionsPtr
*
*COMMENTS:
*******************************************************************************/
static void snetSambaPolicyFinalActionProcessPacketApply
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_POLICY_ACTION *finalPolicyActionsPtr
)
{
    DECLARE_FUNC_NAME(snetSambaPolicyFinalActionProcessPacketApply);

    if(finalPolicyActionsPtr->mirrorToCpu == 1)
    {
        snetTwistTx2Cpu(devObjPtr, descrPtr,
                       TWIST_CPU_TCB_CPU_MIRROR_FIN); /*136*/
    }

    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
    if(finalPolicyActionsPtr->mirrorToAnalyzerPort)
    {
        snetTwistPclRxMirror(devObjPtr, descrPtr);
    }

    if(finalPolicyActionsPtr->redirectFlowCmd == SKERNEL_PCL_IPV4_FRWD_E)
    {
        descrPtr->policySwitchType = POLICY_ROUTE_E ;
        descrPtr->nextLookupPtr = finalPolicyActionsPtr->outlifOrIPv4_Ptr;
    }
    else if (finalPolicyActionsPtr->redirectFlowCmd ==
                SKERNEL_PCL_OUTLIF_FRWD_E)
    {
        descrPtr->policySwitchType = POLICY_SWITCH_E;
        descrPtr->LLL_outLif = finalPolicyActionsPtr->outlifOrIPv4_Ptr;
    }

    /* apply the marking */
    __LOG(("apply the marking"));
    snetSambaPolicyMark(devObjPtr, descrPtr, finalPolicyActionsPtr);

    return;
}


/*******************************************************************************
*   snetSambaPolicyFinalActionApply
*
* DESCRIPTION:
*        The function apply the final actions of the policy on the packet's
*        descriptor
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        finalPolicyActionsPtr - the policy actions to apply
* OUTPUTS:
*        descrPtr - modified descriptor according to the finalPolicyActionsPtr
*
*COMMENTS:
*******************************************************************************/
static void snetSambaPolicyFinalActionApply
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_POLICY_ACTION *finalPolicyActionsPtr
)
{
    DECLARE_FUNC_NAME(snetSambaPolicyFinalActionApply);


    if(finalPolicyActionsPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
    {
        /* instead of calling static snetTwistDropPacket(...) */
        __LOG(("instead of calling static snetTwistDropPacket(...)"));
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        descrPtr->useVidx = 0;
        descrPtr->bits15_2.useVidx_1.vidx = (GT_U16)POLICY_DROP_CODE;
    }
    else if(finalPolicyActionsPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E)
    {
        GT_U32 cpuCode;

        if (finalPolicyActionsPtr->trapActionSource == SNET_POLICY_ACTION_PCE_E
                ||
            finalPolicyActionsPtr->trapActionSource == SNET_POLICY_ACTION_GFC_E)
        {
            cpuCode = CLASS_KEY_TRAP /* 134 */ ;
        }
        else if (finalPolicyActionsPtr->trapActionSource == SNET_POLICY_ACTION_IP_TABLES_E)
        {
            cpuCode = IP_CLASS_TRAP /* 133 */ ;
        }
        else
        {
            cpuCode = DEF_KEY_TRAP/* 132 */ ;
        }

        /* instead of calling static snetTwistTrapPacket(...) */
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        descrPtr->useVidx = 0;
        descrPtr->bits15_2.cmd_trap2cpu.cpuCode = (GT_U16)cpuCode;
    }
    else /* process packet */
    {
        snetSambaPolicyFinalActionProcessPacketApply(devObjPtr,descrPtr,
                                    finalPolicyActionsPtr);
    }

    return;
}


/*******************************************************************************
*   snetSambaPolicyActionsApply
*
* DESCRIPTION:
*        The function modify the frames descriptor according to the actions that
*        will be activated according to the logic of the policy block
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        pclPolicyActionPtr - the actions from the PCL in the policy format
*        classifierPolicyActionPtr - was the classifier found of the default
*                                   actions used (of the template)
*        classifierFound - was the classifier/CoS found entry or
*                             the default entry was taken .
*        pclFoundFirst - was the first pcl found entry or
*                             the default entry was taken .
*        pclFoundSecon - was the second pcl found entry or
*                             the default entry was taken .
*       pclDone - did we enter the pcl engine
*       classifierDone - did we enter the classifiers engine
* OUTPUTS:
*        descrPtr - pointer to the frame's descriptor , with after apply actions
*
*COMMENTS: logic from 1.5.3 [5]
*******************************************************************************/
static void snetSambaPolicyActionsApply
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *classifierPolicyActionPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *pclPolicyActionPtr,
    IN    GT_BOOL classifierFound,
    IN    GT_BOOL pclFoundFirst,
    IN    GT_BOOL pclFoundSecond,
    IN    GT_BOOL pclDone,
    IN    GT_BOOL classifierDone
)
{
    DECLARE_FUNC_NAME(snetSambaPolicyActionsApply);

    SNET_SAMBA_POLICY_ACTION finalPolicyActions;/* final policy actions */

    memset(&finalPolicyActions,0,sizeof(SNET_SAMBA_POLICY_ACTION));

    /* combine the actions from the pcl and the classifier into one final actions*/
    __LOG(("combine the actions from the pcl and the classifier into one final actions"));
    snetSambaPolicyFinalActionGet(devObjPtr, descrPtr, classifierPolicyActionPtr,
                                  pclPolicyActionPtr, classifierFound,
                                  pclFoundFirst, pclFoundSecond,
                                  pclDone, classifierDone,
                                  &finalPolicyActions);

    /* make the necessary changes in the descriptor that apply from the
       final actions*/
    snetSambaPolicyFinalActionApply(devObjPtr, descrPtr, &finalPolicyActions);

    return;
}

/*******************************************************************************
*   snetSambaPolicyEngine
*
* DESCRIPTION:
*        do the policy from the classifier Engine and the pcl Engine.
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
extern void snetSambaPolicyEngine
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSambaPolicyEngine);

    SNET_SAMBA_POLICY_ACTION classifierPolicyAction;/* a structure that hold the
                                info from the GFC (classifier )engine*/
    SNET_SAMBA_POLICY_ACTION pclPolicyAction;/* a structure that hold the
                                info from the PCL engine */
    GT_BOOL classifierFound=GT_FALSE ; /* was the classifier found or used the
                                          default action */
    GT_BOOL pclFoundFirst=GT_FALSE, pclFoundSecond=GT_FALSE; /* was the PCL first,
                                second found or used the default PCL */
    GT_BOOL classifierDone  = GT_FALSE;/* was the classifier engine called */
    GT_BOOL pclDone = GT_FALSE; /* was the pcl engine called*/

    /* set default values to the actions for both Blocks */
    __LOG(("set default values to the actions for both Blocks"));
    snetSambaPolicyDefaultActionsSet(&classifierPolicyAction);
    snetSambaPolicyDefaultActionsSet(&pclPolicyAction);

    if (snetSambaClassificationCheck(devObjPtr, descrPtr))
    {
        snetSambaClassificationEngine(devObjPtr, descrPtr,
                            &classifierPolicyAction,&classifierFound);
        classifierDone  = GT_TRUE;
    }

    if (snetSambaPclCheck(devObjPtr, descrPtr))
    {
        snetSambaPCLEngine(devObjPtr, descrPtr,&pclPolicyAction,
                            &pclFoundFirst, &pclFoundSecond);
        pclDone = GT_TRUE;
    }

    if (pclDone || classifierDone)
    {
        snetSambaPolicyActionsApply(devObjPtr, descrPtr,&classifierPolicyAction,
                &pclPolicyAction, classifierFound, pclFoundFirst, pclFoundSecond,
                pclDone, classifierDone);

        if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
        {
            descrPtr->tcbDidDrop=GT_TRUE;
        }
    }

}


/*******************************************************************************
*   snetSambaPolicyDefaultActionsSet
*
* DESCRIPTION:
*       the function set default values to the input pointer
*
* INPUTS:
*
* OUTPUTS:
*       policyActionPtr - pointer to a structure that hold the actions in
*                       the policy format
*
* RETURNS:
*
* COMMENTS: from 1.6 [5] (reset Value)
*
*
*******************************************************************************/
extern void snetSambaPolicyDefaultActionsSet(
    OUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
)
{
    memset(policyActionPtr,0,sizeof(SNET_SAMBA_POLICY_ACTION));

    policyActionPtr->cmd =  SNET_SAMBA_POLICY_ACTION_CMD_PROCESS_E;
    policyActionPtr->mark_Cmd = SKERNEL_PCL_L2_COS_E ;

    return;
}

