/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah2TT.c
*
* DESCRIPTION:
*       Cheetah2 Asic Simulation .
*       Egress Policy Engine processing for outgoing frame.
*       Source Code file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 20 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah2TTermination.h>
#include <asicSimulation/SLog/simLog.h>

/* number of compares to be done on Layer 4 ports */
#define SNET_CHT2_L4_TCP_PORT_COMP_RANGE_INDEX_CNS      (0x8)
/* the range of address between one word to another */
#define CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS                 (0x10 / 4)

/* array that holds the info about the fields */
static CHT_PCL_KEY_FIELDS_INFO_STC cht2TTKeyFieldsData[CHT2_TT_KEY_FIELDS_ID_LAST_E]=
{
    {0  ,6  ,GT_TRUE," CHT2_TT_SOURCE_PORT_TRUNK_E "  },
    {7  ,7  ,GT_TRUE," CHT2_TT_SRC_IS_TRUNK_E "  },
    {8  ,12 ,GT_TRUE," CHT2_TT_SRC_DEV_E "  },
    {13 ,13 ,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_13_E "  },
    {14 ,25 ,GT_TRUE," CHT2_TT_FIELDS_ID_VID_E "  },
    {26 ,73 ,GT_TRUE," CHT2_TT_MAC_DA_ID_UP_E "  },
    {74 ,74 ,GT_TRUE," CHT2_TT_TUNNEL_KEY_TYPE_E "  },
    {75 ,75 ,GT_TRUE," CHT2_TT_IPV4_OVER_IPV4_E "  },
    {76 ,76 ,GT_TRUE," CHT2_TT_IPV6_OVER_IPV4_E "  },
    {77 ,77 ,GT_TRUE," CHT2_TT_IPV4_OVER_IPV4_GRE_E"  },
    {78 ,78 ,GT_TRUE," CHT2_TT_IPV6_OVER_IPV4_GRE_E "  },
    {79 ,79 ,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_79_E "  },
    {80 ,111,GT_TRUE," CHT2_TT_SIP_ADDRESS "  },
    {112,126,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_112_126_E "  },
    {127,127,GT_TRUE," CHT2_TT_KEY_TUNNEL_TERMINATION_KEY "  },
    {128,159,GT_TRUE," CHT2_TT_DIP_ADDRESS "},
    {160,126,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_160_191_E "},
    {75 ,75 ,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_75_E "  },
    {76 ,78 ,GT_TRUE," CHT2_TT_EXP2_IN_LABEL2_E "  },
    {79 ,79 ,GT_TRUE," CHT2_TT_SBIT_IN_LABEL2_E "  },
    {80 ,99 ,GT_TRUE," CHT2_TT_LABEL2_E "  },
    {100,102,GT_TRUE," CHT2_TT_EXP1_IN_LABEL1_E "  },
    {103,103,GT_TRUE," CHT2_TT_SBIT_IN_LABEL1_E "  },
    {104,123,GT_TRUE," CHT2_TT_LABEL1_E "  },
    {124,126,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_124_126_E "},
    {128,191,GT_TRUE," CHT2_TT_KEY_FIELDS_ID_RESERVED_128_191_E "}
};
/*static GT_VOID snetCht2TTConfigPtrGet
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC      * descrPtr ,
    IN GT_U32                                 port ,
    OUT SNET_CHT2_EPCL_LOOKUP_CONFIG_STC    * lookupConfPtr
); */



static GT_BOOL snetCht2TTTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);


/*******************************************************************************
*   snetCht2EPclKeyFieldBuildByPointer
*
* DESCRIPTION:
*       function insert data of field to the search key in specific place in key
*
* INPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*       fieldValPtr - (pointer to) data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2TTKeyFieldBuildByPointer
(
    INOUT SNET_CHT_POLICY_KEY_STC       *ttKeyPtr,
    IN GT_U8                            *fieldValPtr,
    IN CHT2_TT_KEY_FIELDS_ID_ENT         fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht2TTKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByPointer(ttKeyPtr, fieldValPtr, fieldInfoPtr);

    return;
}

/*******************************************************************************
*   snetCht2TTKeyFieldBuildByValue
*
* DESCRIPTION:
*       function insert data of the field to the search key
*       in specific place in epcl search key
*
* INPUTS:
*       pclKeyPtr - (pointer to) current epcl key
*       fieldVal - data of field to insert to key
*       fieldId -- field id
*
* OUTPUTS:
*       pclKeyPtr - (pointer to) current pcl key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2TTKeyFieldBuildByValue
(
    INOUT SNET_CHT_POLICY_KEY_STC           *ttKeyPtr,
    IN GT_U32                               fieldVal,
    IN CHT2_TT_KEY_FIELDS_ID_ENT            fieldId
)
{
    CHT_PCL_KEY_FIELDS_INFO_STC *fieldInfoPtr = &cht2TTKeyFieldsData[fieldId];

    snetChtPclSrvKeyFieldBuildByValue(ttKeyPtr, fieldVal, fieldInfoPtr);

    return;
}

/*******************************************************************************
* snetCht2TTActionApply
*
* DESCRIPTION:
*        Apply the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       actionInfoPtr   - pointer to the ePCL action entry.
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*       C.12.14  - T.T action table and Policy TCAM access control
*                   Registers
*
*******************************************************************************/
GT_VOID snetCht2TTActionApply
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr  ,
    IN SNET_CHT2_TT_ACTION_STC              * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht2TTActionApply);

    GT_U32  address;
    GT_U32  regValue;


    descrPtr->packetCmd = actionDataPtr->fwdCmd;

    if ((descrPtr->packetCmd == (SKERNEL_EXT_PACKET_CMD_ENT)SKERNEL_CHT2_TT_CMD_MIRROR_E)||
        (descrPtr->packetCmd == (SKERNEL_EXT_PACKET_CMD_ENT)SKERNEL_CHT2_TT_CMD_TRAP_E))
    {
        descrPtr->cpuCode = actionDataPtr->ttCpuCode;
    }

    /* Ingress mirroring command */
    if(actionDataPtr->ttMirrorAnlze == GT_TRUE)
    {
        descrPtr->rxSniff = 1;
    }

    /* Policer info */
    if(descrPtr->policerEn == 0)
    {
        descrPtr->policerEn = actionDataPtr->policerEn ;

        if(descrPtr->policerEn == 1)
        {
            descrPtr->policerPtr = actionDataPtr->policerIndex;
        }
    }

    /*  The redirect info */
    if (SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->tunnelIf,
                                     SNET_DST_INTERFACE_VIDX_E))
    {   /* useVidx */
        __LOG(("useVidx"));
        descrPtr->eVidx = actionDataPtr->tunnelIf.interfaceInfo.vidx;
    }
    else
    {
        if (SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->tunnelIf,
                                         SNET_DST_INTERFACE_TRUNK_E))
        {
            descrPtr->trgTrunkId =  actionDataPtr->tunnelIf.interfaceInfo.trunkId;
        }
        else
        {
            descrPtr->trgEPort = /* target port  */
                actionDataPtr->tunnelIf.interfaceInfo.devPort.port;

            /* call after setting trgEPort */
            __LOG(("call after setting trgEPort"));
            SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,bridge);

            descrPtr->trgDev = /* target dev  */
                actionDataPtr->tunnelIf.interfaceInfo.devPort.devNum;
        }
    }
    /* indication for Tunnel Start */
    if ((actionDataPtr->ttRedirectCmd == 1) &&
        (actionDataPtr->tunnelStart == 1))
    {
        __LOG(("indication for Tunnel Start"));
        descrPtr->tunnelStart = GT_TRUE;
        descrPtr->tunnelPtr = actionDataPtr->tunnelPtr;
    }

    /* update the matching counter if enabled */
    __LOG(("update the matching counter if enabled"));
    if (actionDataPtr->matchCounterEn)
    {
        address = SMEM_CHT2_PCL_RULE_MATCH_REG(devObjPtr) +
                        (actionDataPtr->matchCounterIndex * 0x4);
        smemRegGet(devObjPtr, address, &regValue);
        ++regValue;
        smemRegSet(devObjPtr, address, regValue);
    }


    /* QoS Marking Command */
    __LOG(("QoS Marking Command"));
    if(descrPtr->qosProfilePrecedence == SKERNEL_QOS_PROF_PRECED_SOFT)
    {
        if(actionDataPtr->ttQoSPrecedence == SKERNEL_QOS_PROF_PRECED_HARD)
        {
            descrPtr->qosProfilePrecedence = SKERNEL_QOS_PROF_PRECED_HARD;
        }

        if(actionDataPtr->ttQoSMode == 4)
        { /* 4 untrust mode */
            __LOG(("4 untrust mode"));
            descrPtr->qos.qosProfile = actionDataPtr->ttQoSProfile;
        }

        if(actionDataPtr->ttRemapDscp == 1)
        {
            descrPtr->modifyDscp = 1;
        }
        else if(actionDataPtr->ttRemapDscp == 2)
        {
            descrPtr->modifyDscp = 0;
        }

        if(actionDataPtr->ttModifyUp == 1)
        {
            descrPtr->modifyUp = 1;
        }
        else if(actionDataPtr->ttModifyUp == 2)
        {
            descrPtr->modifyUp = 0;
        }
    }

    /*  VID re-assignment , only for tagged packets */
    __LOG(("VID re-assignment , only for tagged packets"));
    if (actionDataPtr->ttVidPrecedence == 1)
    {
        if (descrPtr->trgTagged == 1)
        {
            descrPtr->eVid = actionDataPtr->ttVid;
            descrPtr->vidModified = 1;
        }
    }

    return ;
}

/*******************************************************************************
* snetCht2TTActionGet
*
* DESCRIPTION:
*        Get the action entry from the policy action table.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       matchIndex      - index to the action table .
*
* OUTPUTS:
*       actionInfoPtr   - pointer to the TT action.
*
* RETURN:
*
* COMMENTS:
*       D.12.8.3 - The tunnel Engine maintains an 1024 entries table , corresponding
*                  to the 1024 rules that may be defined in the TCAM lookup
*                   structure.
*
*******************************************************************************/
GT_VOID snetCht2TTActionGet
(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr  ,
    IN GT_U32                             matchIndex,
    OUT SNET_CHT2_TT_ACTION_STC         * actionDataPtr
)
{
    DECLARE_FUNC_NAME(snetCht2TTActionGet);

    GT_U32  tblAddr;
    GT_U32  * actionEntryDataPtr;
    GT_BOOL enable;

    tblAddr = SMEM_CHT3_TUNNEL_ACTION_TBL_MEM(devObjPtr, matchIndex) ;
    actionEntryDataPtr = smemMemGet(devObjPtr,tblAddr);

    /* Read word 0 from the action table entry */
    __LOG(("Read word 0 from the action table entry"));
    actionDataPtr->matchCounterIndex = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],26,4);
    actionDataPtr->matchCounterEn    = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],25,1);
    actionDataPtr->ttModifyUp        = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],23,2);
    actionDataPtr->ttModifyDscp      = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],21,2);
    actionDataPtr->ttQoSProfile      = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],14,8);
    actionDataPtr->ttQoSPrecedence   = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],12,1);
    actionDataPtr->ttMirrorAnlze     = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],11,1);
    actionDataPtr->ttCpuCode         = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],3,8);
    actionDataPtr->fwdCmd            = SMEM_U32_GET_FIELD(actionEntryDataPtr[0],0,3);

    /* Read word 1 from the action table entry */
    __LOG(("Read word 1 from the action table entry"));
    actionDataPtr->ttRemapDscp       = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],31,1);
    actionDataPtr->ttVid             = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],19,12);
    actionDataPtr->ttNestedVidEn     = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],16,1);
    actionDataPtr->ttVidPrecedence   = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],15,1);
    enable = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],14,1);
    SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->tunnelIf,
                                 SNET_DST_INTERFACE_VIDX_E, enable);
    if (SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->tunnelIf,
                                     SNET_DST_INTERFACE_VIDX_E))
    {
        actionDataPtr->tunnelIf.interfaceInfo.vidx =
            (GT_U16)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],2,12);
    }
    else
    {
        enable = (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],2,1);
        SNET_SET_DEST_INTERFACE_TYPE(actionDataPtr->tunnelIf,
                                     SNET_DST_INTERFACE_TRUNK_E, enable);

        if (SNET_GET_DEST_INTERFACE_TYPE(actionDataPtr->tunnelIf,
                                         SNET_DST_INTERFACE_TRUNK_E))
        {
            actionDataPtr->tunnelIf.interfaceInfo.trunkId =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],3,7);
        }
        else
        {
            actionDataPtr->tunnelIf.interfaceInfo.devPort.port =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],3,6);
            actionDataPtr->tunnelIf.interfaceInfo.devPort.devNum =
                    (GT_U8)SMEM_U32_GET_FIELD(actionEntryDataPtr[1],9,5);
        }
    }
    actionDataPtr->ttRedirectCmd = SMEM_U32_GET_FIELD(actionEntryDataPtr[1],0,2);
    /* Read word 2 from the action table entry */
    __LOG(("Read word 2 from the action table entry"));

    actionDataPtr->ttUp           = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],29,3);
    actionDataPtr->ttTtl          = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],27,1);
    actionDataPtr->ttPassType     = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],25,2);
    actionDataPtr->ttQoSMode      = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],22,3);
    actionDataPtr->tunnelPtr      = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],11,10);
    actionDataPtr->tunnelStart    = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],10,1);
    actionDataPtr->policerIndex   = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],1,8);
    actionDataPtr->policerEn      = SMEM_U32_GET_FIELD(actionEntryDataPtr[2],0,1);

    return ;
}


/*******************************************************************************
* snetCht2TTTcamLookUp
*
* DESCRIPTION:
*        Tcam lookup for  T.T ENTRY
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame descriptor.
*       pclKeyPtr       - pointer to PCL key .
*
* OUTPUTS:
*       matchIndexPtr   - pointer to the matching index.
* RETURN:
*
* COMMENTS:
*       C.12.8.4  Router Tcam Table :   TCAM holds 1024 entries
*                 of 32 bits for IPv4 or 128 bits for IPv6.Table 593
*******************************************************************************/
static GT_VOID snetCht2TTTcamLookUp
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr  ,
    IN SNET_CHT_POLICY_KEY_STC    * ttKeyPtr,
    OUT GT_U32 *  matchIndexPtr
)
{
    DECLARE_FUNC_NAME(snetCht2TTTcamLookUp);




    GT_U32   maxEntries;  /* 1024 entries for tunnel */
    GT_U32   entryIndex;  /* Index to the TCAM line */
    GT_U32   wordIndex;   /* Index to the word in the TCAM rule */
    GT_U32 * routingTcamEntryDataPtr; /* pointer to routing TCAM data entry  */
    GT_U32 * routingTcamEntryCtrlPtr; /* pointer to routing TCAM ctrl entry  */
    GT_U32 * routingTcamEntryDataMaskPtr; /* pointer to routing TCAM data mask entry */
    GT_U32 * routingTcamEntryCtrlMaskPtr; /* pointer to routing TCAM ctrl mask entry  */
    GT_U32   compModeData;             /* compare mode bit in the tcam rule */
    GT_U32  *dataPtr;                 /* pointer to routing TCAM data entry */
    GT_U32  *dataMaskPtr;             /* pointer to routing TCAM ctrl entry  */
    GT_U32  *ctrlPtr;                 /* pointer to routing TCAM data mask entry */
    GT_U32  *ctrlMaskPtr;             /* pointer to routing TCAM ctrl mask entry  */
    GT_U32  *ttSearchKeyPtr;          /* pointer to tt tcam search0 key */
    GT_U32   ttTcamData32Bits;        /* tt Tcam data 32 lower bits   */
    GT_U32   ttTcamData16Bits;        /* tt Tcam data 16 upper bits   */
    GT_U32   ttSearchKey16Bits ;      /* tt tcam search pcl 32 lower bits   */
    GT_U32   ttSearchKey32Bits ;      /* tt tcam search pcl 16 lower bits   */
    GT_U32   keySize = 5;             /* tt tcam size of TCAM words   */


    maxEntries = SNET_CHT2_TUNNEL_TCAM_1024_CNS  ;

    ttSearchKeyPtr = ttKeyPtr->key.data;

    /*  Get pointer to Tcam data entry */
    routingTcamEntryDataPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_DATA_TBL_MEM);

    /*  Get pointer to Tcam control entry */
    routingTcamEntryCtrlPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_CTRL_TBL_MEM);

    /*  Get pointer to Tcam data mask entry */
    routingTcamEntryDataMaskPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_DATA_MASK_TBL_MEM);

    /*  Get pointer to Tcam control mask entry */
    routingTcamEntryCtrlMaskPtr = smemMemGet(devObjPtr,SMEM_CHT2_ROUTING_TCAM_CTRL_MASK_TBL_MEM);



    for (entryIndex = 0 ; entryIndex <  maxEntries;  ++entryIndex,
        routingTcamEntryDataPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS,
        routingTcamEntryCtrlPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS,
        routingTcamEntryDataMaskPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS,
        routingTcamEntryCtrlMaskPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS
        )
    {

        dataPtr = routingTcamEntryDataPtr;
        ctrlPtr = routingTcamEntryCtrlPtr;
        dataMaskPtr = routingTcamEntryDataMaskPtr;
        ctrlMaskPtr = routingTcamEntryCtrlMaskPtr;

        compModeData =  (ctrlPtr[0] & (3<<18)) >>18; /* get compmode */

        if ((ctrlPtr[0] & (1<<17)) & (ctrlMaskPtr[0] & (1<<17)) &&
          /* for tunnel compModeData should be '1' */
        (compModeData==1))/* check validity of entry */
        {

        for (wordIndex = 0 ; wordIndex <  keySize  ; ++wordIndex,
                                dataPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS,
                                ctrlPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS,
                                dataMaskPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS,
                                ctrlMaskPtr += CHT_TUNNEL_TCAM_ENTRY_WIDTH_CNS
            )
        {



            if  (wordIndex == 1)
            {
                /* calculate 48 bits for word1 */
                __LOG(("calculate 48 bits for word1"));
                ttTcamData16Bits = (SMEM_U32_GET_FIELD(dataPtr[1],0,15) &
                                       SMEM_U32_GET_FIELD(dataMaskPtr[1],0,15));
                ttTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);

                /* calculate 48 bits for word1 from the TUNNEL search key */
                __LOG(("calculate 48 bits for word1 from the TUNNEL search key"));
                ttSearchKey16Bits = SMEM_U32_GET_FIELD(ttSearchKeyPtr[1],0,15);
                ttSearchKey32Bits = ttSearchKeyPtr[0];
                ttSearchKeyPtr++;

                ttSearchKey16Bits &= SMEM_U32_GET_FIELD(dataMaskPtr[1],0,15);
                ttSearchKey32Bits &= dataMaskPtr[0];
            }
            else if  (wordIndex == 2)
            {
                /* calculate 48 bits for word2 */
                __LOG(("calculate 48 bits for word2"));
                ttTcamData16Bits = (SMEM_U32_GET_FIELD(dataPtr[1],0,15) &
                                        SMEM_U32_GET_FIELD(dataMaskPtr[1],0,15));
                ttTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);

                /* calculate 48 bits for word2 from the TUNNEL search key */
                __LOG(("calculate 48 bits for word2 from the TUNNEL search key"));
                ttSearchKey32Bits = SMEM_U32_GET_FIELD(ttSearchKeyPtr[0],16,16);
                ttSearchKey32Bits |= SMEM_U32_GET_FIELD(ttSearchKeyPtr[1],0,16) << 16;
                ttSearchKey16Bits = SMEM_U32_GET_FIELD(ttSearchKeyPtr[1],16,16);
                ttSearchKeyPtr += 2;

                ttSearchKey16Bits &= SMEM_U32_GET_FIELD(dataMaskPtr[1],0,15);
                ttSearchKey32Bits &= dataMaskPtr[0];
            }
            else
            {
                /* 16 extended bits for word1-2 are zero */
                __LOG(("16 extended bits for word1-2 are zero"));
                ttSearchKey16Bits = 0;
                ttTcamData16Bits = 0;

                /* calculate 32 bits for word 0 , word3 and word4 */
                __LOG(("calculate 32 bits for word 0 , word3 and word4"));
                ttTcamData32Bits = (dataPtr[0] & dataMaskPtr[0]);
                ttSearchKey32Bits = ttSearchKeyPtr[0];
                ttSearchKeyPtr++;

                ttSearchKey32Bits &= dataMaskPtr[0];
            }

            /* lookup in TCAM */
            __LOG(("lookup in TCAM"));
            if ((ttTcamData16Bits != ttSearchKey16Bits) ||
                (ttTcamData32Bits != ttSearchKey32Bits))
                break;

            if (wordIndex == keySize)
            {
                *matchIndexPtr = entryIndex;
                break;
            }
        }
        }
    } /*  find an entry     */

}


/*******************************************************************************
* snetCht2TTCreateKey
*
* DESCRIPTION:
*        Create TT  search tcam key.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to frame data buffer Id.
*
* OUTPUT:
*       TTKeyPtr    - pointer to T.T key structure.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetCht2TTCreateKey
(
    IN SKERNEL_DEVICE_OBJECT                *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   *descrPtr ,
    OUT SNET_CHT_POLICY_KEY_STC             *ttKeyPtr

)
{
    GT_U32     resrvdVal = 0;   /* reserved value */
    GT_U32     resrvd1   = 1;   /* reserved value must set 1*/
    GT_U32     protocol;

    snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->trgDev, /* bits[0..6] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
    snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->eVid ,
                                        CHT2_TT_SRC_IS_TRUNK_E);/* [7] src is trunk */
    snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->eVid ,/* [8..12] src is dev */
                                        CHT2_TT_SRC_DEV_E);
    snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal ,/* [13] reserved */
                                        CHT2_TT_KEY_FIELDS_ID_RESERVED_13_E);
    snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->eVid  ,/* [14..25] eVid */
                                        CHT2_TT_FIELDS_ID_VID_E);
    snetCht2TTKeyFieldBuildByPointer(ttKeyPtr, descrPtr->macDaPtr ,/* [26..73] MAC DA */
                                        CHT2_TT_MAC_DA_ID_UP_E);


    if ((descrPtr->isIp == 1)  && (descrPtr->isIPv4))
    {

        protocol = descrPtr->l3StartOffsetPtr[6];

        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[74] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        if (protocol == 4)
        {
            snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1, /* bits[75] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        }
        else
            snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[75] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        if (protocol == 41)
        {
            snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1, /* bits[76] Valid */
                                          CHT2_TT_SOURCE_PORT_TRUNK_E);
        }
        else
            snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[76] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        if (protocol == 47)
        {
            snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1, /* bits[77] Valid */
                                         CHT2_TT_SOURCE_PORT_TRUNK_E);
        }
        else
            snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[77] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[78] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[79] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->sip[0] ,/* [80..111] src is dev */
                                        CHT2_TT_SRC_DEV_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal ,/* [112..126] reserved */
                                        CHT2_TT_KEY_FIELDS_ID_RESERVED_112_126_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1 ,/* [127] reserved */
                                        CHT2_TT_KEY_TUNNEL_TERMINATION_KEY);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->dip[0]  ,/* [128..159] dip */
                                        CHT2_TT_FIELDS_ID_VID_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal         ,/* [160..191] reserved */
                                        CHT2_TT_MAC_DA_ID_UP_E);
    }
    else if (descrPtr->mpls == 1)
    {
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1, /* bits[74] Valid */
                                        CHT2_TT_SOURCE_PORT_TRUNK_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal, /* bits[75] Valid */
                                        CHT2_TT_KEY_FIELDS_ID_RESERVED_75_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->exp2 ,
                                        CHT2_TT_EXP2_IN_LABEL2_E);/* [76..78] src is trunk */
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1 ,/* [79] src is dev */
                                        CHT2_TT_SRC_DEV_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->label2 ,          /* [99..80] reserved */
                                        CHT2_TT_LABEL2_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->exp1 ,
                                        CHT2_TT_EXP1_IN_LABEL1_E);/* [102..100] src is trunk */
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1 ,/* [103] src is dev */
                                        CHT2_TT_SBIT_IN_LABEL2_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, descrPtr->label1 ,          /* [123..104] reserved */
                                        CHT2_TT_LABEL1_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1, /* bits[126..124] Valid */
                                        CHT2_TT_SBIT_IN_LABEL1_E);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvd1 ,            /* [127] reserved */
                                        CHT2_TT_KEY_TUNNEL_TERMINATION_KEY);
        snetCht2TTKeyFieldBuildByValue(ttKeyPtr, resrvdVal         ,  /* [128..191]  reserved*/
                                        CHT2_TT_MAC_DA_ID_UP_E);

    }

}



/*******************************************************************************
* snetCht2TTIPv4ExceptionCheck
*
* DESCRIPTION:
*        IPv4 tunnels and MPLS tunnels have separate T.T trigger
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*
*
* RETURN:
*       GT_BOOL TRUE DOING T.T false
*
* COMMENTS:
*         13.4.3 page 358
*
*******************************************************************************/
GT_BOOL snetCht2TTIPv4ExceptionCheck
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr,
    IN INTERNAL_TTI_DESC_INFO_STCT_PTR      internalTtiInfoPtr

)
{
    DECLARE_FUNC_NAME(snetCht2TTIPv4ExceptionCheck);


    GT_BOOL status = GT_TRUE;  /* return code */
    GT_U32 fldVal;              /* register field's value */
    GT_U32 startBit;            /* start bit in the register */
    GT_U32 cpuCode;             /*cpu code*/
    GT_BIT ipHeaderError = internalTtiInfoPtr ?
        internalTtiInfoPtr->preTunnelTerminationInfo.ipHeaderError :
                                           descrPtr->ipHeaderError;
    GT_BIT ipTtiHeaderError = internalTtiInfoPtr ?
        internalTtiInfoPtr->preTunnelTerminationInfo.ipTtiHeaderError :
                                           descrPtr->ipTtiHeaderError;


    if (ipHeaderError ||  /* ip header error */
        ipTtiHeaderError) /* or extra TTI Ip header Error*/
    {
        status = GT_FALSE;

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(ipHeaderError)
            {
                __LOG(("IPv4 TTI Header Error Command \n"));
                startBit = 0;
                cpuCode = SNET_CHT_IPV4_TT_HEADER_ERROR;
            }
            else /*ipTtiHeaderError*/
            {
                __LOG(("IPv4 TTI SIP Address Error Command \n"));
                startBit = 9;
                /* get the CPU code from the register */
                smemRegFldGet(devObjPtr,SMEM_LION3_TTI_LOOKUP_IP_EXCEPTION_CODES_0_REG(devObjPtr) , 0, 8, &fldVal);

                cpuCode = fldVal;
            }

            smemRegFldGet(devObjPtr,SMEM_LION3_TTI_LOOKUP_IP_EXCEPTION_COMMANDS_REG(devObjPtr) , startBit, 3, &fldVal);

            snetChtIngressCommandAndCpuCodeResolution(devObjPtr,descrPtr,
                                                      descrPtr->packetCmd,
                                                      fldVal,
                                                      descrPtr->cpuCode,
                                                      cpuCode,
                                                      SNET_CHEETAH_ENGINE_UNIT_TTI_E,
                                                      GT_FALSE);

            return (descrPtr->packetCmd > SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E) ?
                    GT_FALSE :
                    GT_TRUE;
        }

        if (SKERNEL_IS_XCAT_REVISON_A1_DEV(devObjPtr))
        {
            smemRegFldGet(devObjPtr,SMEM_XCAT_TTI_ENGINE_CONFIG_REG(devObjPtr) , 4, 1, &fldVal);
        }
        else
        {
            smemRegFldGet(devObjPtr,SMEM_CHT_PCL_GLOBAL_REG(devObjPtr), 19, 1, &fldVal);
        }
        if (fldVal == 1)
        {
            descrPtr->packetCmd = SKERNEL_CHT2_TT_CMD_HARD_DROP_E;
        }
        else
        {
            descrPtr->packetCmd = SKERNEL_CHT2_TT_CMD_TRAP_E;
            descrPtr->cpuCode = SNET_CHT_IPV4_TT_HEADER_ERROR;

        }
    }

    return   status;

}

/*******************************************************************************
* snetCht2TTTriggeringCheck
*
* DESCRIPTION:
*        IPv4 tunnels and MPLS tunnels have separate T.T trigger
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*
* RETURN:
*       GT_BOOL TRUE DOING T.T false
*
* COMMENTS:
*         13.4.1 page 355
*
*******************************************************************************/
static GT_BOOL snetCht2TTTriggeringCheck
(
    IN SKERNEL_DEVICE_OBJECT             *  devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC   *  descrPtr

)
{

    GT_U32  protocol;               /* protocol in IP header */
    GT_U32  regAddress;             /* register entry       */
    GT_U32  *portVlanCfgEntryPtr;   /* table entry pointer  */
    GT_U32  ipV4ttEn;               /* Enable IPv4 TT action for packet rx in port  */
    GT_U32  mplsEn;                 /* Enable mpls TT action for packet rx in port  */



    regAddress = SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(devObjPtr,
                                            descrPtr->localDevSrcPort);
    portVlanCfgEntryPtr = smemMemGet(devObjPtr, regAddress);

    if (descrPtr->isIp == 1)
    {   /* T.T is disabled -  */
        protocol = descrPtr->l3StartOffsetPtr[9];
        /* IPv4 TT Enable bit */
        ipV4ttEn = SMEM_U32_GET_FIELD(portVlanCfgEntryPtr[1], 17, 1);
        if (((protocol  == 4) || (protocol  == 47) || (protocol  == 41)) &&
            (ipV4ttEn == 1))

        {
            return GT_TRUE;
        }
    }
    else if (descrPtr->mpls == 1)
    {
        mplsEn = SMEM_U32_GET_FIELD(portVlanCfgEntryPtr[1], 18, 1);
        if (mplsEn)
        {
            return GT_TRUE;
        }

    }


    return GT_FALSE;

}

/*******************************************************************************
* snetCht2TTermination
*
* DESCRIPTION:
*        T.T Engine processing for outgoing frame on Cheetah2
*        asic simulation.
*        T.T processing , T.T assignment ,key forming , 1 Lookup ,
*        actions to descriptor processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data buffer Id
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data buffer Id
*
* RETURN:
*
* COMMENTS:
*   T.T has only one lookup cycle.
*   D.12.8.3 : T.T Registers
*
*******************************************************************************/
GT_VOID snetCht2TTermination
(
    IN    SKERNEL_DEVICE_OBJECT             * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht2TTermination);


    GT_BOOL                         status =  GT_TRUE;                                /* enable searching the TCAM  */
    SNET_CHT_POLICY_KEY_STC         ttKey;                     /* TT key structure   */
    /* CHT2_TT_KEY_TYPE_ENT            keyType; */                                       /* tcam search key type       */
    GT_U32                          matchIndex = SNET_CHT_POLICY_NO_MATCH_INDEX_CNS; /* index to the matching rule */
    SNET_CHT2_TT_ACTION_STC         actionData;

    /* Get the T.T  (Table 103 104 : T.T classification Table 13.4.2) */
    status = snetCht2TTTriggeringCheck(devObjPtr,descrPtr);
    if (status == GT_FALSE) /* =0 , means T.T engine is disabled */
    {
        return ;
    }

    /* create T.T tcam search key */
    __LOG(("create T.T tcam search key"));
    ttKey.devObjPtr = devObjPtr;
    snetCht2TTCreateKey(devObjPtr, descrPtr , &ttKey);

    /* search key in T.T Tcam */
    __LOG(("search key in T.T Tcam"));
    snetCht2TTTcamLookUp(devObjPtr , descrPtr , &ttKey , &matchIndex);

    if (descrPtr->isIPv4 == 0)
    {   /* T.T IPv4 Exception check */
        __LOG(("T.T IPv4 Exception check"));
        status = snetCht2TTIPv4ExceptionCheck(devObjPtr, descrPtr,NULL);

    }
    if  (status == GT_TRUE)
    {   /* read and apply T.T action */
        __LOG(("read and apply T.T action"));
        if (matchIndex != SNET_CHT2_TUNNEL_NO_MATCH_INDEX_CNS)
        {   /* read the action from the action table */
            __LOG(("read the action from the action table"));
            snetCht2TTActionGet(devObjPtr, descrPtr , matchIndex , &actionData);

            /* apply the matching action */
            __LOG(("apply the matching action"));
            snetCht2TTActionApply(devObjPtr, descrPtr , &actionData);
        }
    }
    return;

}

