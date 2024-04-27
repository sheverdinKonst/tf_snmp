/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetah3MacLookup.c
*
* DESCRIPTION:
*   implement next for cheetah3(plus) :
*       1. Ingress MAC SA Lookup -
*           The Ingress MAC SA is performed in a hash based lookup into
*           a dedicated 32K MAC Address table.
*       2. Egress MAC DA Lookup .
*
*
*
*************************************************************************
*   Note : This lookup is independent of the Bridge engine FDB lookup.  *
*************************************************************************
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3MacLookup.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3TTermination.h>
#include <asicSimulation/SLog/simLog.h>

/* type of RSSI interrupt :
*   RSSI_INTERRUPT_TYPE_GLOBAL_MIN_E  - interrupt due to RSSI min threshold
*   RSSI_INTERRUPT_TYPE_AVERAGE_MIN_E - interrupt due to RSSI min average threshold
*/
typedef enum{
    RSSI_INTERRUPT_TYPE_GLOBAL_MIN_E,
    RSSI_INTERRUPT_TYPE_AVERAGE_MIN_E
}RSSI_INTERRUPT_TYPE_ENT;

/*******************************************************************************
*   snetCht3MacCalcKeyAndMask
*
* DESCRIPTION:
*       Fill MAC entry and mask for MAC Lookup
*
* INPUTS:
*       macAddrPtr      - MAC pointer
*
* OUTPUT:
*       enrtyKeyPtr     - MAC entry search key
*       entryMaskPtr    - MAC entry search mask
*
*******************************************************************************/
extern GT_VOID snetCht3MacCalcKeyAndMask
(
    IN    GT_U8  * macAddrPtr,
    OUT   GT_U32 * enrtyKeyPtr,
    OUT   GT_U32 * entryMaskPtr
)
{
    /* MACAddr[7:0] */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[0],  0, 8, macAddrPtr[5]);
    /* MACAddr[15:8] */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[0],  8, 8, macAddrPtr[4]);
    /* MACAddr[23:16] */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 16, 8, macAddrPtr[3]);
    /* MACAddr[31:24] */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[0], 24, 8, macAddrPtr[2]);

    /* MACAddr Mask[31:0] */
    entryMaskPtr[0] = 0xFFFFFFFF;



    /* MACAddr[39:32] */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 0, 8, macAddrPtr[1]);
    /* MACAddr[47:40] */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 8, 8, macAddrPtr[0]);

    /* MACAddr Mask[37:32] */
    SMEM_U32_SET_FIELD(entryMaskPtr[1], 0, 16, 0xFFFF);


    /* Valid bit */
    SMEM_U32_SET_FIELD(enrtyKeyPtr[1], 16, 1, 1);
    /* Valid bit - mask */
    SMEM_U32_SET_FIELD(entryMaskPtr[1], 16, 1, 1);

}
/*******************************************************************************
*   snetCht3MacLookup
*
* DESCRIPTION:
*       MAC SA/DA LookUp
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*       saLookUp        - GT_TRUE - SA lookup , GT_FALSE - DA lookup
* OUTPUT:
*       hasMatchPtr     - (pointer to) did we have match
*       matchIndexPtr   - (pointer to) index of matched entry
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetCht3MacLookup(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL      saLookUp,
    OUT GT_BOOL *hasMatchPtr,
    OUT GT_U32  *matchIndexPtr
)
{
    DECLARE_FUNC_NAME(snetCht3MacLookup);

    SNET_CHEETAH_FDB_ENTRY_ENT fdbEntryType;    /* fdb entry type */
    GT_U32 hashIndex;               /* hash method calculated index */
    GT_U32 hashType;                /* hash calculation method */
    GT_U32 vlanMode;                /* VLAN mode  */
    GT_U32 bucketSize;              /* number of MAC entries in the bucket */
    GT_U32 macGlobCfgRegData;       /* Global register configuration data */
    GT_U8  *keyMacPtr;
    GT_U32 entry_key[4]={0};         /* array of words to compare with entry -
                                       build according to (macAddr)*/
    GT_U32 entry_mask[4]={0};        /* array of words to apply on entry words
                                       and entry_key */
    GT_U32  *macTblPtr;              /* pointer to start of MAC table */
    GT_U32 entryIndx;               /* entry index in the MAC table */
    GT_U32  ii;                     /* entry index in bucket */
    GT_U32  jj;                     /* word index in entry */
    const GT_U32 entryWidthInWords = 4;
    GT_U32 tblSize = devObjPtr->fdbNumEntries;
    GT_U32 numBitsToUse = SMEM_CHT_FDB_HASH_NUM_BITS(tblSize);

    keyMacPtr = saLookUp == GT_TRUE  ?  descrPtr->macSaPtr : descrPtr->macDaPtr;

    fdbEntryType = SNET_CHEETAH_FDB_ENTRY_MAC_E;

    /* Get data from MAC Table Global Configuration Register */
    __LOG(("Get data from MAC Table Global Configuration Register"));
    smemRegGet(devObjPtr, SMEM_CHT_MAC_TBL_GLB_CONF_REG(devObjPtr), &macGlobCfgRegData);

    hashType = SMEM_U32_GET_FIELD(macGlobCfgRegData, 21, 1);
    vlanMode  = SMEM_U32_GET_FIELD(macGlobCfgRegData, 3, 1);
    bucketSize = 4 * (1 + SMEM_U32_GET_FIELD(macGlobCfgRegData, 0, 3));

    /* Get hash index to start search */
    __LOG(("Get hash index to start search"));
    hashIndex = cheetahMacHashCalc(devObjPtr, hashType, vlanMode, fdbEntryType, keyMacPtr,
                                   0, 0, 0,numBitsToUse);


    /* Calculate entry_key and entry_mask to compare with FDB entry */
    __LOG(("Calculate entry_key and entry_mask to compare with FDB entry"));
    snetCht3MacCalcKeyAndMask(keyMacPtr, entry_key, entry_mask);

    /* Get entryPtr to start of table */
    __LOG(("Get entryPtr to start of table"));
    macTblPtr = smemMemGet(devObjPtr, SMEM_CHT3_CAPWAP_MAC_TABLE_ENTRY_TBL_MEM(0));

    /* Read bucket */
    __LOG(("Read bucket"));
    for (ii = 0; ii < bucketSize; ii++)
    {
        entryIndx = (hashIndex + ii) % tblSize;

        /* compare the entry with next fields : 1. validBit , 2. macAddr */
        __LOG(("compare the entry with next fields : 1. validBit , 2. macAddr"));
        for(jj = 0 ; jj < entryWidthInWords ; jj++)
        {
            if((macTblPtr[(entryIndx * entryWidthInWords) + jj] & entry_mask[jj]) !=
               entry_key[jj])
            {
                /* no match */
                __LOG(("no match"));
                break;
            }
        }

        if(jj == entryWidthInWords)
        {
            /* we have a match */
            __LOG(("we have a match"));
            *hasMatchPtr = GT_TRUE;
            *matchIndexPtr = entryIndx;
            return;
        }
    }

    *hasMatchPtr = GT_FALSE;

    return;
}

/*******************************************************************************
*   snetCht3RssiExceptionInterrupt
*
* DESCRIPTION:
*       Do interrupt due to RSSI exception
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*       rssiInterruptType   - interrupt due to RSSI min threshold or
*                           due to RSSI min average threshold
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetCht3RssiExceptionInterrupt(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN RSSI_INTERRUPT_TYPE_ENT  rssiInterruptType,
    IN GT_U32   rssiValue
)
{
    DECLARE_FUNC_NAME(snetCht3RssiExceptionInterrupt);

    GT_U32  regValue;
    GT_U32  interruptBit;
    GT_U32  *internalMemPtr;

    internalMemPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,
        CHT_INTERNAL_SIMULATION_USE_MEM_CAPWAP_RSSI_EXCEPTION_STATUS_E);

    if((*internalMemPtr) & 1)
    {
        /* set the bit at "used"  */
        (*internalMemPtr) &= (~1);
        /* we can update the RSSI exception */
        __LOG(("we can update the RSSI exception"));
        /* set register 0 */
        __LOG(("set register 0"));
        regValue = (GT_U32)(descrPtr->macSaPtr[0] << 24) |
                   (GT_U32)(descrPtr->macSaPtr[1] << 16) |
                   (GT_U32)(descrPtr->macSaPtr[2] <<  8) |
                   (GT_U32)(descrPtr->macSaPtr[3] <<  0);

        smemRegSet(devObjPtr,SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG,regValue);

        /* set register 1 */
        __LOG(("set register 1"));
        regValue = (GT_U32)((rssiValue & 0xFF) << 16) |
                   (GT_U32)(descrPtr->macSaPtr[4] <<  8) |
                   (GT_U32)(descrPtr->macSaPtr[5] <<  0);

        smemRegSet(devObjPtr,SMEM_CHT3_CAPWAP_RSSI_EXCEPTION_REG + 4,regValue);

    }

    /* use reserved bits : 30,31 */
    __LOG(("use reserved bits : 30,31"));
    interruptBit = (rssiInterruptType == RSSI_INTERRUPT_TYPE_GLOBAL_MIN_E) ?
                        30 : /* global minimum */
                        31 ; /* average minimum */

    /* generate interrupt --- use the bridge cause and mask registers */
    __LOG(("generate interrupt --- use the bridge cause and mask registers"));
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_CHT_BRIDGE_INT_CAUSE_REG(devObjPtr),
                          SMEM_CHT_BRIDGE_INT_MASK_CAUSE_REG(devObjPtr),
                          (1 << interruptBit),
                          SMEM_CHT_L2I_SUM_INT(devObjPtr));
}



/*******************************************************************************
*   snetCht3IngressMacSa
*
* DESCRIPTION:
*       Ingress MAC SA LookUp ,VLAN assignment , RSSI
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetCht3IngressMacSa(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3IngressMacSa);

    GT_U32  index;
    GT_BOOL matchFound;
    GT_U32  *macEntryPtr;              /* pointer to start of MAC table */
    GT_U32  fieldValue;
    SKERNEL_EXT_PACKET_CMD_ENT   packetCmd;
    GT_U32  rssiWeightProfileRegValue;
    GT_U32  weight;/* RSSI weight -- value 0..100 */
    GT_U32  rssiNewValue;/* new calculated RSSI value */

    snetCht3MacLookup(devObjPtr,descrPtr,GT_TRUE,&matchFound,&index);

    if(matchFound == GT_FALSE)
    {
        /* nothing more to do */
        __LOG(("nothing more to do"));
        return;
    }


    /* Get entryPtr to start of table */
    __LOG(("Get entryPtr to start of table"));
    macEntryPtr = smemMemGet(devObjPtr, SMEM_CHT3_CAPWAP_MAC_TABLE_ENTRY_TBL_MEM(index));


    /* the field of packet command */
    __LOG(("the field of packet command"));
    fieldValue = SMEM_U32_GET_FIELD(macEntryPtr[1],17,3);

    switch(fieldValue)
    {
        case 0:
            packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
            break;
        case 1:
            packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
            break;
        case 2:
            packetCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
            break;
        case 3:
            packetCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
            break;
        default:
            packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
            break;
    }

    /* Apply packet new command */
    __LOG(("Apply packet new command"));
    descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd,
                                                  packetCmd);

    if(descrPtr->capwap.action.vlanAssignmentCmd ==
        SNET_CHT3_CAPWAP_VLAN_ASSIGNMENT_BASED_MAC_SA_CNS)
    {
        /* we need to use the VID from this entry */
        __LOG(("we need to use the VID from this entry"));
        descrPtr->eVid = (GT_U16)SMEM_U32_GET_FIELD(macEntryPtr[2],0,12);
    }

    if(descrPtr->capwap.rssiValid == GT_TRUE)
    {
        /* the field of rssiWeightProfile */
        __LOG(("the field of rssiWeightProfile"));
        fieldValue = SMEM_U32_GET_FIELD(macEntryPtr[1],23,2);

        /* a weighted average is maintained for the MAC SA */
        __LOG(("a weighted average is maintained for the MAC SA"));
        smemRegGet(devObjPtr,SMEM_CHT3_CAPWAP_RSSI_WEIGHT_PROFILE_REG(fieldValue),&rssiWeightProfileRegValue);

        /* check that the packet's RSSI is in the threshold limits */
        /* the field of rssiMin */
        fieldValue = SMEM_U32_GET_FIELD(rssiWeightProfileRegValue,7,8);
        if(fieldValue > descrPtr->capwap.rssi)
        {
            /* set interrupt about this state */
            __LOG(("set interrupt about this state"));
            snetCht3RssiExceptionInterrupt(devObjPtr,descrPtr,GT_TRUE,descrPtr->capwap.rssi);
        }

        /* get the weight of the needed profile */
        __LOG(("get the weight of the needed profile"));
        weight = SMEM_U32_GET_FIELD(rssiWeightProfileRegValue, 0 ,7);

        /* the field of rssiAverage */
        __LOG(("the field of rssiAverage"));
        fieldValue = SMEM_U32_GET_FIELD(macEntryPtr[2],24,8);

        /* calculate new RSSI weight value for this entry */
        /* New RSSI = profile<weight> * <prev RSSI> + (1 - profile<weight>) * Pkt<RSSI> */
        rssiNewValue = ((weight * fieldValue) + ((100-weight)*descrPtr->capwap.rssi)) / 100;

        /* modify the RSSI value in the entry */
        /* call smemRegFldSet to allow visualizer to get the update */
        smemRegFldSet(devObjPtr,
                    SMEM_CHT3_CAPWAP_MAC_TABLE_ENTRY_TBL_MEM(index) + 4*2,/* word 2 in entry */
                    24,8,rssiNewValue);


        /* check that the new average RSSI is in the threshold limits */
        /* the field of rssiMinAverage */
        fieldValue = SMEM_U32_GET_FIELD(rssiWeightProfileRegValue,15,8);
        if(fieldValue > rssiNewValue)
        {
            /* set interrupt about this state */
            __LOG(("set interrupt about this state"));
            snetCht3RssiExceptionInterrupt(devObjPtr,descrPtr,GT_FALSE,rssiNewValue);
        }
    }

    /* the field of redirectEnable  */
    __LOG(("the field of redirectEnable"));
    fieldValue = SMEM_U32_GET_FIELD(macEntryPtr[3],0,1);

    if(fieldValue)
    {
        /* redirect */
        __LOG(("redirect"));
        descrPtr->bypassBridge = 1;/* bypass bridge decisions */
        descrPtr->bypassRouter = 1;/* bypass router decisions */

        /* get the virtual{dev,port} -- redirectEgressInterface */
        __LOG(("get the virtual{dev,port} -- redirectEgressInterface"));
        descrPtr->useVidx = 0;
        descrPtr->targetIsTrunk = 0;
        descrPtr->trgDev  = SMEM_U32_GET_FIELD(macEntryPtr[3],1,5);
        descrPtr->trgEPort = SMEM_U32_GET_FIELD(macEntryPtr[3],6,6);
        /* call after setting trgEPort */
        __LOG(("call after setting trgEPort"));
        SNET_E_ARCH_CLEAR_IS_TRG_PHY_PORT_VALID_MAC(devObjPtr,descrPtr,tti);
    }

    return;
}


/*******************************************************************************
*   snetCht3EgressMacDa
*
* DESCRIPTION:
*       Egress MAC DA LookUp
*
* INPUTS:
*       devObjPtr       - pointer to device object
*       descrPtr        - pointer to frame descriptor
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetCht3EgressMacDa(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetCht3EgressMacDa);

    GT_U32  index;
    GT_BOOL matchFound;
    GT_U32  *macEntryPtr;              /* pointer to start of MAC table */
    GT_U32  fieldValue;
    SKERNEL_EXT_PACKET_CMD_ENT   packetCmd;

    snetCht3MacLookup(devObjPtr,descrPtr,GT_FALSE,&matchFound,&index);

    if(matchFound == GT_FALSE)
    {
        /* nothing more to do */
        __LOG(("nothing more to do"));
        return;
    }

    /* Get entryPtr to start of table */
    __LOG(("Get entryPtr to start of table"));
    macEntryPtr = smemMemGet(devObjPtr, SMEM_CHT3_CAPWAP_MAC_TABLE_ENTRY_TBL_MEM(index));


    /* the field of packet command */
    fieldValue = SMEM_U32_GET_FIELD(macEntryPtr[1],21,3);/*DA packetCommand*/

    switch(fieldValue)
    {
        case 0:
            packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
            break;
        case 1:
            packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
            break;
        case 2:
            packetCmd = SKERNEL_EXT_PKT_CMD_SOFT_DROP_E;
            break;
        case 3:
            packetCmd = SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E;
            break;
        default:
            packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
            break;
    }

    /* Apply packet new command */
    __LOG(("Apply packet new command"));
    descrPtr->packetCmd = snetChtPktCmdResolution(descrPtr->packetCmd,
                                                  packetCmd);

    fieldValue = SMEM_U32_GET_FIELD(macEntryPtr[1],26,1);/*useThisEntryVlanId*/

    if(fieldValue)
    {
        /* we need to use the VID from this entry */
        __LOG(("we need to use the VID from this entry"));
        descrPtr->eVid = (GT_U16)SMEM_U32_GET_FIELD(macEntryPtr[2],12,12);/*daVlanId*/
    }

    descrPtr->capwap.egressInfo.daLookUpMatched = 1;
    descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressEnable = SMEM_U32_GET_FIELD(macEntryPtr[3],12,1);/*da802dot11eEgressEnable*/
    descrPtr->capwap.egressInfo.tsOrDa802dot11eEgressMappingTableProfile = SMEM_U32_GET_FIELD(macEntryPtr[3],13,2);/*da802dot11eEgressMappingTableProfile*/

    return;
}

