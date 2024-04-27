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
*       pcl Engine of Samba
*
*       for applying actions on the descriptor (that will be of the frame)
*
*       implementation according to REG_SIM_POLICY_SAMBA_FRDTD document
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SLog/simLog.h>



/*******************************************************************************
*   snetSambaPclCheck
*
* DESCRIPTION:
*        check if need to enter the Pcl
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*  Return: GT_TRUE if need to enter the Pcl
*
*COMMENTS: Logic From 13.3.1  [1]
*******************************************************************************/
extern GT_BOOL snetSambaPclCheck
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 bypassPcl; ;/* do we need to bypass the pcl engine */

    if (descrPtr->doPcl == 0 ||
        descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E ||
        descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
        return GT_FALSE;
    }

    smemRegFldGet(devObjPtr, SAMBA_PCL_CONTROL_REG, 1, 1,&bypassPcl);

    if(bypassPcl)
    {
        return GT_FALSE;
    }

    return GT_TRUE;
}
/*******************************************************************************
*   snetSambaPclKeyCreateBytes17and18
*
* DESCRIPTION:
*        function fill bytes 17,18 (index 16,17) in the searchKeyArray
*        according to frame's descriptor and the pclId
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        pclId     - the pclId to insert to the key search
*        doByte18  - if GT_FALSE we modify only bite17
* OUTPUTS:
*        searchKeyPtr - pointer to the search key .
*
* COMMENT:
*
*******************************************************************************/
static void snetSambaPclKeyCreateBytes17and18
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_TWIST_PCL_KEY_SIZE_CNS],
    IN    GT_U32                   pclId,
    IN    GT_BOOL                  doByte18
)
{
    DECLARE_FUNC_NAME(snetSambaPclKeyCreateBytes17and18);

    SNET_SAMBA_BYTE_17 *byte17Ptr; /* byte17 info --   structure from 1.6.1.3 [3] */
    GT_U32 enabledMacDaTypeAtt ;/*do we use mac da type attribute to the key */

    smemRegFldGet(devObjPtr, SAMBA_PCL_CONTROL_REG, 3, 1,&enabledMacDaTypeAtt);

    byte17Ptr = (SNET_SAMBA_BYTE_17*)&( searchKeyPtr[16]);
    byte17Ptr->nacDATypeAttEn.route = descrPtr->doRout;
    byte17Ptr->nacDATypeAttEn.foundDa = descrPtr->macDaLookupResult;

    if(enabledMacDaTypeAtt)
    {
        /* the byte17Ptr->pclId  is always bits 2:7 */
        __LOG(("the byte17Ptr->pclId  is always bits 2:7"));
        byte17Ptr->nacDATypeAttEn.pclId = (GT_U8)(pclId & 0x0f);
        byte17Ptr->nacDATypeAttEn.macDaType = descrPtr->macDaType;
    }
    else
    {
        byte17Ptr->nacDATypeAttDis.pclId = (GT_U8)(pclId & 0x3f);
    }

    if(doByte18)
    {
        if(descrPtr->flowTemplate != SKERNEL_IPV6_E)
        {
            searchKeyPtr[17] = 1 << descrPtr->flowTemplate ;
        }
        else
        {
            searchKeyPtr[17] = 1 << (descrPtr->flowTemplate - 1);
        }
    }

    return;
}


/*******************************************************************************
*   snetSambaPclKeyCreate
*
* DESCRIPTION:
*        create key for PCL
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        pclId     - the pclId to insert to the key search
* OUTPUTS:
*        searchKeyPtr - pointer to the search key .
*
* COMMENT:
*
*******************************************************************************/
static void snetSambaPclKeyCreate
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_TWIST_PCL_KEY_SIZE_CNS],
    IN    GT_U32                   pclId
)
{
    DECLARE_FUNC_NAME(snetSambaPclKeyCreate);

    GT_BOOL doByte18 = GT_FALSE;
    /* let the engine of the twist build the search key
        we will modify it after that ! */
    snetTwistPclKeyCreate(devObjPtr,descrPtr,searchKeyPtr);

    /* modify byte 17 -- to suite Samba */
    __LOG(("modify byte 17 -- to suite Samba"));
    snetSambaPclKeyCreateBytes17and18(devObjPtr,descrPtr,searchKeyPtr,
                                      pclId,doByte18);

    return;
}


/*******************************************************************************
*   snetSambaPclConvertPceToPolicy
*
* DESCRIPTION:
*        function convert the format of action from SNET_SAMBA_PCE_ACTION to the
*        format of SNET_SAMBA_POLICY_ACTION
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        pceActionPtr - actions in the pce format
* OUTPUTS:
*        policyActionPtr - actions in the policy format .
*
* COMMENT:
*
*******************************************************************************/
static void snetSambaPclConvertPceToPolicy
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_PCE_ACTION *pceActionPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
)
{

    /* the first 66 bits from pceActionPtr are striate forward */
    policyActionPtr->cmd = (GT_U8)pceActionPtr->cmd ;
    policyActionPtr->mirrorToAnalyzerPort = (GT_U8)pceActionPtr->mirrorToAnalyzerPort  ;
    policyActionPtr->activateTC = (GT_U8)pceActionPtr->activateTC;
    policyActionPtr->activateCount = (GT_U8)pceActionPtr->activateCount ;
    policyActionPtr->mark_Cmd = (GT_U8)pceActionPtr->mark_Cmd ;
    policyActionPtr->dscp = (GT_U8)pceActionPtr->dscp ;
    policyActionPtr->tcOrCnt_Ptr = (GT_U16)pceActionPtr->tcOrCnt_Ptr ;
    policyActionPtr->redirectFlowCmd = (GT_U8)pceActionPtr->redirectFlowCmd ;
    policyActionPtr->outlifOrIPv4_Ptr=(pceActionPtr->outlifOrIPv4_Ptr_msb << 1)|
                                       pceActionPtr->outlifOrIPv4_Ptr_lsb ;
    policyActionPtr->setLBH = (GT_U8)pceActionPtr->setLBH ;
    policyActionPtr->LBH = (GT_U8)pceActionPtr->LBH ;
    policyActionPtr->nestedVlan = (GT_U8)pceActionPtr->word2.markCmdDscpCos.nestedVlan  ;
    policyActionPtr->modifyVlan = (GT_U8)pceActionPtr->word2.markCmdDscpCos.modifyVlan  ;

    /* end of the first 66 bits */

    if(pceActionPtr->mark_Cmd == SKERNEL_PCL_COS_ENTRY_E )/* Mark DSCP CoS */
    {
        policyActionPtr->tc =
            (GT_U8)pceActionPtr->word2.markCmdDscpCos.cosTc;/*bits [74:76] in pceActionPtr */
        policyActionPtr->up =
            (GT_U8)pceActionPtr->word2.markCmdDscpCos.cosUp;/*bits [77:79] in pceActionPtr */
        policyActionPtr->dp =
            (GT_U8)pceActionPtr->word2.markCmdDscpCos.cosDp;/*bits [80:81] in pceActionPtr */

        if (policyActionPtr->modifyVlan)
        {
            policyActionPtr->vid = (GT_U16)
                pceActionPtr->word2.markCmdDscpCos.vidOrFlowId;/* bits [66:77] in pceActionPtr */
        }
        else
        {
            policyActionPtr->flowId = (GT_U16)
                pceActionPtr->word2.markCmdDscpCos.vidOrFlowId ; /* bits [66:73] in pceActionPtr */
        }
    }
    else if (policyActionPtr->modifyVlan)
    {
        policyActionPtr->vid = (GT_U16)
            pceActionPtr->word2.markCmdNotDscpCos_modifyVlan.vid;/* bits [66:77] in pceActionPtr */
    }
    else
    {
        policyActionPtr->flowId = (GT_U16)
            pceActionPtr->word2.markCmdNotDscpCos_noModifyVlan.flowId ; /* bits [66:74] in pceActionPtr */
    }

    /* COPY rest of the bits [83:88] FROM pceActionPtr */
    policyActionPtr->mirrorToCpu = (GT_U8)pceActionPtr->word2.markCmdDscpCos.mirrorToCpu;
    policyActionPtr->markDscp = (GT_U8)pceActionPtr->word2.markCmdDscpCos.markDscp;
    policyActionPtr->markTc = (GT_U8)pceActionPtr->word2.markCmdDscpCos.markTc;
    policyActionPtr->markUp = (GT_U8)pceActionPtr->word2.markCmdDscpCos.markUp;
    policyActionPtr->markDp = (GT_U8)pceActionPtr->word2.markCmdDscpCos.markDp;

    return;
}



/*******************************************************************************
*   snetSambaPclSearch
*
* DESCRIPTION:
*        search for PCL actions , fill the info in actionInfoPtr.
*        If there is no match in the PCE table then the default actions are
*        taken from registers :
*        PCL PCE default 0-2 in (3 registers)
*        addresses 0x028001A0 , 0x028001A4 , 0x028001A8 .
*
*        This "simulate" the TCAM search .
*        Tables 466-468 [4] show how to address the pce entries and actions
*
*        See table 466 [4]
*        The base address of the TCAM's data (values) is : 0x02e28000 - 0x02e2fffc
*        1024 entries of 32 bytes (8 words) only first 5 words of entry are used .
*
*        See table 467 [4]
*        The base address of the TCAM's masks  is : 0x02e20000 - 0x02e27ffc
*        1024 entries of 32 bytes (8 words) only first 5 words of entry are used .
*
*        See table 468 [4]
*        The base address of the PCE table (actions)  is :0x02e3400-0x02e37ffc
*        1024 entries of 16 bytes (4 words) only first 3 words of entry are used .
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        actionInfoPtr     - hold reset values of this info
*        searchKeyPtr - pointer to the search key .
* OUTPUTS:
*        actionInfoPtr - will be filled with action info no mater what
*                        (maybe with default).
*        pclFoundPtr   - if pcl was found (not as default)
* COMMENT:
*
*******************************************************************************/
static void snetSambaPclSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U8                    searchKeyPtr[SNET_TWIST_PCL_KEY_SIZE_CNS],
    INOUT SNET_SAMBA_POLICY_ACTION *actionInfoPtr,
    OUT   GT_BOOL                  *pclFoundPtr

)
{
    DECLARE_FUNC_NAME(snetSambaPclSearch);

    GT_U8   sambaSearchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS];
    GT_U32  pceIndex;/*index in the pce table*/
    GT_U32  wordIndex; /*index of the compared word*/
    GT_U32  *pceValueEntryPtr;/*pointer to the tcam data memory for the current entry*/
    GT_U32  *pceMaskEntryPtr;/*pointer to the tcam mask memory for the current entry */
    GT_U32  *key32Ptr;/*the search key as GT_U32 pointer (not GT_U8*) */
    SNET_SAMBA_PCE_ACTION *pceActionPtr;/* pointer to the actions of the matched flow */

    memset(sambaSearchKeyPtr, 0, SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS);
    memcpy(sambaSearchKeyPtr, searchKeyPtr, SNET_TWIST_PCL_KEY_SIZE_CNS);

    /* pointer to the start of the tcam data memory */
    pceValueEntryPtr = smemMemGet(devObjPtr,SAMBA_TCAM_MEMORY_DATA_START);
    /* pointer to the start of the tcam mask memory */
    pceMaskEntryPtr  = smemMemGet(devObjPtr,SAMBA_TCAM_MEMORY_MASK_START);

    key32Ptr = (GT_U32*)sambaSearchKeyPtr;

    for(pceIndex = 0 ; pceIndex < 1024 ;
        pceIndex++,pceValueEntryPtr+=8,pceMaskEntryPtr+=8)
    {
        for(wordIndex=0;wordIndex<5;wordIndex++)
        {
            if(pceValueEntryPtr[wordIndex] !=
               (pceMaskEntryPtr[wordIndex] &  key32Ptr[wordIndex]))
            {
                break;
            }
        }

        if(wordIndex == 5)
        {
            /* we found a match */
            __LOG(("we found a match"));
            break;
        }
    }

    if(pceIndex == 1024)
    {
        /* we didn't found a match in the pce table */
        *pclFoundPtr = GT_FALSE;

        /* the actions from the default pce actions registers */
        pceActionPtr = smemMemGet(devObjPtr,SAMBA_PCL_PCE_DEFAULT_0_REG);
    }
    else
    {
        /* each entry in the actions is 4 words */
        GT_U32  tmpMemAddr = (SAMBA_PCE_TABLE_START +  pceIndex * 4 *sizeof(GT_32));

        *pclFoundPtr = GT_TRUE;
        pceActionPtr = smemMemGet(devObjPtr,tmpMemAddr);
    }

    /* convert the pce from memory format to the policy format */
    snetSambaPclConvertPceToPolicy(devObjPtr, descrPtr, pceActionPtr,
                                    actionInfoPtr);

    return;
}


/*******************************************************************************
*   snetSambaPclReplcePclId
*
* DESCRIPTION:
*        replace the pcl id in the byte 17 of the key (index 16)
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        pclId     - the pclId to insert to the key search
* OUTPUTS:
*        searchKeyPtr - pointer to the search key .
*
* COMMENT:
*
*******************************************************************************/
static void snetSambaPclReplcePclId
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_TWIST_PCL_KEY_SIZE_CNS],
    IN    GT_U32                   pclId
)
{
    DECLARE_FUNC_NAME(snetSambaPclReplcePclId);

    GT_BOOL doByte18 = GT_FALSE;
    /* replace the pclId in byte 17 */
    __LOG(("replace the pclId in byte 17"));
    snetSambaPclKeyCreateBytes17and18(devObjPtr,descrPtr,searchKeyPtr,
                                      pclId,doByte18);
}


/*******************************************************************************
*   snetSambaPclCombineActions
*
* DESCRIPTION:
*       the function combine the 2 action received by the 2 pce's searching
*       into one action , according to the setting in the PCL Action to
*       Lookup Mapping Register in address : 0x028001E0
*
*       the registers format will be into : SNET_SAMBA_ACTION2LOOKUP
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
*        firstActionInfoPtr - action info from the first pcl
*        secondActionInfoPtr - action info from the first pcl
* OUTPUTS:
*        policyActionPtr - the COMBINED action in the policy format
*
* COMMENT:
*
*******************************************************************************/
static void snetSambaPclCombineActions
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_POLICY_ACTION *firstActionInfoPtr,
    IN    SNET_SAMBA_POLICY_ACTION *secondActionInfoPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
)
{
    SNET_SAMBA_ACTION2LOOKUP decisionLookupInfo;
    GT_BOOL workInSpecialMode = GT_FALSE;/*do we work in special command mode ,
                                               see the CmdMap field */
    SNET_SAMBA_POLICY_ACTION *tmpPolicyPtr;
/* macro to use only in this specific function */
#define POLICY_CHOOSE_ACTION(mode) \
    if(mode == 0) { tmpPolicyPtr = firstActionInfoPtr ; }  \
    else{tmpPolicyPtr = secondActionInfoPtr;}

    smemRegGet(devObjPtr, SAMBA_PCL_ACTION_TO_LOOKUP_MAPPING_REG,
        (void *)&decisionLookupInfo);

    /* CmdMap */
    if(decisionLookupInfo.cmdMap == 0)
    {
        policyActionPtr->cmd = firstActionInfoPtr->cmd;
    }
    else if (decisionLookupInfo.cmdMap == 1)
    {
        policyActionPtr->cmd = secondActionInfoPtr->cmd;
    }
    else
    {
        workInSpecialMode = GT_TRUE; /* special command mode */
        if(firstActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ||
           secondActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E)
        {
            policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ;
        }
        else if(firstActionInfoPtr->cmd ==SNET_SAMBA_POLICY_ACTION_CMD_DROP_E ||
               secondActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
        {
            policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_DROP_E ;
        }
        else
        {
            policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_PROCESS_E ;
        }
    }

    /* explore the Cmd further ---  see 16.4.5 [3] */
    if (workInSpecialMode &&
        (( firstActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E  &&
           secondActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E )
                        ||
         ( firstActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E  &&
            secondActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E ))
        )
    {
        /* we take all the actions from first action */
        /* COPY firstActionInfoPtr TO  policyActionPtr */

        memcpy(policyActionPtr,firstActionInfoPtr,
                sizeof(SNET_SAMBA_POLICY_ACTION));
        return;
    }
    else if(policyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E ||
               policyActionPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
    {
        /* take all  commands from the PCE that command cam from */
        if (firstActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E  ||
            firstActionInfoPtr->cmd == SNET_SAMBA_POLICY_ACTION_CMD_DROP_E)
        {
            /* we take all the actions from first action */
            /*COPY firstActionInfoPtr TO  policyActionPtr*/

            memcpy(policyActionPtr,firstActionInfoPtr,
                    sizeof(SNET_SAMBA_POLICY_ACTION));
            return;
        }
        else
        {
            /* we take all the actions from second action */
            /*COPY secondActionInfoPtr TO  policyActionPtr*/
            memcpy(policyActionPtr,secondActionInfoPtr,
                    sizeof(SNET_SAMBA_POLICY_ACTION));
            return;
        }
    }
    /* MirrorToAnalyzerPortMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.mirrorToAnalyzerPortMap);

    policyActionPtr->mirrorToAnalyzerPort = tmpPolicyPtr->mirrorToAnalyzerPort ;

    /* ActiveTcOrCntMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.activateTCAndContMap);

    policyActionPtr->activateTC  = tmpPolicyPtr->activateTC;
    policyActionPtr->activateCount = tmpPolicyPtr->activateCount;
    policyActionPtr->tcOrCnt_Ptr = tmpPolicyPtr->tcOrCnt_Ptr;

    /* MarkCmdMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.mark_CmdMap);

    policyActionPtr->mark_Cmd = tmpPolicyPtr->mark_Cmd;
    /* dscp */
    policyActionPtr->dscp = tmpPolicyPtr->dscp;
    /* marking */
    policyActionPtr->markDscp = tmpPolicyPtr->markDscp;
    policyActionPtr->markTc = tmpPolicyPtr->markTc;
    policyActionPtr->markUp = tmpPolicyPtr->markUp;
    /* cos parameters */
    policyActionPtr->tc = tmpPolicyPtr->tc;
    policyActionPtr->up = tmpPolicyPtr->up;
    policyActionPtr->dp = tmpPolicyPtr->dp;

    /* RedirectFlowCmdMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.redirectFlowCmdMap);

    policyActionPtr->redirectFlowCmd= tmpPolicyPtr->redirectFlowCmd;
    policyActionPtr->outlifOrIPv4_Ptr = tmpPolicyPtr->outlifOrIPv4_Ptr;

    /* SetLBHMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.setLBHMap);

    policyActionPtr->setLBH = tmpPolicyPtr->setLBH;
    policyActionPtr->LBH = tmpPolicyPtr->LBH;

    /* NestedVlanMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.nestedVlanMap);

    policyActionPtr->nestedVlan = tmpPolicyPtr->nestedVlan;

    /* ModifyVlanMap */
    POLICY_CHOOSE_ACTION( decisionLookupInfo.modifyVlanMap );

    policyActionPtr->modifyVlan = tmpPolicyPtr->modifyVlan;
    policyActionPtr->vid = tmpPolicyPtr->vid;

    /* FlowIDMap */
    POLICY_CHOOSE_ACTION(decisionLookupInfo.flowIdMap);


    if(tmpPolicyPtr->modifyVlan == 1)
    {
        policyActionPtr->flowId = 0;
    }
    else if(tmpPolicyPtr->mark_Cmd == SKERNEL_PCL_COS_ENTRY_E) /*mark DSCP_CoS */
    {
        /*The bit [8] is used for the CoS*/
        policyActionPtr->flowId = tmpPolicyPtr->flowId & 0xff;
    }
    else
    {
        policyActionPtr->flowId = tmpPolicyPtr->flowId;
    }

    /* MirrorToCPUMap */
    POLICY_CHOOSE_ACTION( decisionLookupInfo.mirrorToCpuMap)

    policyActionPtr->mirrorToCpu = tmpPolicyPtr->mirrorToCpu;


    return ;
#undef POLICY_CHOOSE_ACTION
}

/*******************************************************************************
*   snetSambaPCLEngine
*
* DESCRIPTION:
*        the function do the pcl engine to get an action info
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*        pclFoundFirstPtr - pointer to return if the first pcl found entry or
*                             the default entry was taken .
*        pclFoundSecondPtr - pointer to return if the second pcl found entry or
*                             the default entry was taken .
*
*COMMENTS: Logic From 1.2 [3]
*******************************************************************************/
extern void snetSambaPCLEngine
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr,
    OUT   GT_BOOL *pclFoundFirstPtr,
    OUT   GT_BOOL *pclFoundSecondPtr
)
{
    SNET_SAMBA_POLICY_ACTION firstActionInfo;/* action info from the first pcl */
    SNET_SAMBA_POLICY_ACTION secondActionInfo;/*action info from the first pcl*/
    GT_U8   searchKeyArray[SNET_TWIST_PCL_KEY_SIZE_CNS];
    GT_U32  dropPcl ; /* do we set drop action no mater what */

    smemRegFldGet(devObjPtr, SAMBA_PCL_CONTROL_REG, 2, 1,&dropPcl);

    if (dropPcl)
    {
        *pclFoundFirstPtr = GT_FALSE;
        *pclFoundSecondPtr = GT_FALSE;
        policyActionPtr->cmd = SNET_SAMBA_POLICY_ACTION_CMD_DROP_E;
        policyActionPtr->trapActionSource = SNET_POLICY_ACTION_PCE_E;
        return;
    }

    /* set default values to the actions for both Blocks */
    snetSambaPolicyDefaultActionsSet(&firstActionInfo);
    snetSambaPolicyDefaultActionsSet(&secondActionInfo);

    /* create key search for the first search */
    snetSambaPclKeyCreate(devObjPtr,descrPtr,searchKeyArray,descrPtr->pclId0);

    /* do the first search */
    snetSambaPclSearch(devObjPtr,descrPtr,searchKeyArray,
                    &firstActionInfo,pclFoundFirstPtr);

    /* modify the first key search to suite the second search */
    snetSambaPclReplcePclId(devObjPtr,descrPtr,searchKeyArray,descrPtr->pclId1);

    /* do the second search */
    snetSambaPclSearch(devObjPtr,descrPtr,searchKeyArray,
                    &secondActionInfo,pclFoundSecondPtr);

    /* combine the 2 actions into one action */
    snetSambaPclCombineActions(devObjPtr,descrPtr,
                               &firstActionInfo,&secondActionInfo,
                               policyActionPtr);

    /*this line need to be the after the call to snetSambaPclCombineActions */
    policyActionPtr->trapActionSource = SNET_POLICY_ACTION_PCE_E;

    return;
}



