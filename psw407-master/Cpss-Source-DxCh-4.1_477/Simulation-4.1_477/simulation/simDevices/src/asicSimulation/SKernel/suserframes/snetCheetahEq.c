/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetCheetahEq.c
*
* DESCRIPTION:
*
*       Enqueueing module processing
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 107 $
*
*******************************************************************************/
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SKernel/suserframes/snetCheetah3CentralizedCnt.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

#define SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_NAME                   \
     STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_MTU_INDEX            )\
    ,STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_IS_TRUNK      )\
    ,STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX             )\
    ,STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_TRUNK         )\
    ,STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_VIDX          )\
    ,STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_DEVICE        )\
    ,STR(SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_PHYSICAL_PORT )

static char * lion3EqE2PhyFieldsTableNames[SMEM_LION3_EQ_E2PHY_TABLE_FIELDS___LAST_VALUE___E] =
    {SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_NAME};

static SNET_ENTRY_FORMAT_TABLE_STC lion3EqE2PhyTableFieldsFormat[SMEM_LION3_EQ_E2PHY_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_MTU_INDEX*/
    STANDARD_FIELD_MAC(2)
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_IS_TRUNK*/
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX*/
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_TRUNK*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX}
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_VIDX*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX}
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_DEVICE*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     10,
     SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX}
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_PHYSICAL_PORT*/
    ,STANDARD_FIELD_MAC(8)
};

static SNET_ENTRY_FORMAT_TABLE_STC sip5_20EqE2PhyTableFieldsFormat[SMEM_LION3_EQ_E2PHY_TABLE_FIELDS___LAST_VALUE___E] =
{
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_MTU_INDEX*/
    STANDARD_FIELD_MAC(2)
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_IS_TRUNK*/
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX*/
    ,STANDARD_FIELD_MAC(1)
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_TRUNK*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX}
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_VIDX*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     12,
     SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX}
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_DEVICE*/
    ,{FIELD_SET_IN_RUNTIME_CNS,
     10,
     SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX}
/*SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_PHYSICAL_PORT*/
    ,STANDARD_FIELD_MAC(9) /* was 8 in sip 5 */
};

/* global frame Id */
static GT_U32  globalFrameId = 0;


/* Static declaration */
static GT_BOOL snetChtEqStc(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDoStc(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDuplicateRxSniff(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_BOOL snetChtEqStatSniff(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDoRxSniff(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL doStatSniff
);

static GT_VOID snetChtEqDoToCpu(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDoFromCpu(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDoFrwd(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDuplicateToCpu(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqDoTrgSniff(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqTxSniffer(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL packetFromIngress
);

static GT_VOID snetChtEqDuplicateStc(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqAppSpecCpuCodeAssign(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);


static GT_VOID snetChtVirtualPortMapping(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqCncCount(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqIngressDropCount(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
);

static GT_VOID snetChtEqSniffToTrgSniff(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 sniffTrgPort,
    IN GT_U32 sniffTrgDev
);

#define SNET_CHT_INGR_ANALYZER_TARGET_PORT_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry, 7, 6) : \
        SMEM_U32_GET_FIELD(entry, 16, 6)

#define SNET_CHT_INGR_ANALYZER_TARGET_DEV_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry, 2, 5) : \
        SMEM_U32_GET_FIELD(entry, 11, 5)

#define SNET_CHT_EGR_ANALYZER_TARGET_PORT_GET_MAC(dev, entry) \
    (SKERNEL_IS_XCAT_REVISON_A1_DEV(dev)) ? \
        SMEM_U32_GET_FIELD(entry, 7, 6) : \
        SMEM_U32_GET_FIELD(entry, 5, 6)

/*check if allow to change fields of src/trg :
<srcTrgDev>
<srcTrgPhysicalPort>
<srcTrgEPort>
*/
#define ALLOW_MODIFY_SRC_TRG_INFO_MAC(dev,desc) \
    (((dev)->supportEArch == 0 || (desc)->marvellTagged == 0) ? 1 : 0)

/*******************************************************************************
*   eqPrepareEgress
*
* DESCRIPTION:
*       prepare for TXQ - egress processing
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID eqPrepareEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(eqPrepareEgress);

    GT_U32 fldValue;                /* Register's field value */
    GT_U32 startBit , endBit;
    GT_U32 numOfBits;
    GT_U32 origPacketHash;
    GT_U32 regAddr;

    SIM_LOG_PACKET_DESCR_SAVE

    if(devObjPtr->vplsModeEnable.eq)
    {
        descrPtr->tunnelPtr &= 0x3FF;/*limited to 10 bits*/
        descrPtr->arpPtr    &= 0x3FF;/*limited to 10 bits*/
    }

    if(devObjPtr->supportEArch)
    {
        /* EQ2EGF Hash Bits Selection mechanism */
        regAddr = SMEM_LION3_EQ_TO_EGF_HASH_BIT_SELECTION_CONFIG_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr,&fldValue);
        startBit = SMEM_U32_GET_FIELD(fldValue,0,5);
        endBit   = SMEM_U32_GET_FIELD(fldValue,5,5);

        if(endBit >= startBit)
        {
            numOfBits = (endBit - startBit) + 1;

            /* the egress pipe not support more then 12 bits ! */
            if(numOfBits > 12)
            {
                skernelFatalError("eqPrepareEgress: the start bit[%d] and end bit [%d] , make [%d] bits that is more then 12 bits \n",
                    startBit,endBit, numOfBits);
            }
        }
        else
        {
            numOfBits = 0;

            skernelFatalError("eqPrepareEgress: the start bit[%d] > end bit [%d] \n",
                startBit,endBit);
        }
    }
    else
    {
        startBit = 0;
        numOfBits = 6;
    }

    /* the packet hash from the EQ to TXQ is only 6 bits width */
    __LOG(("the packet hash from the EQ to TXQ start at bit [%d] and is [%d] bits width \n",
        startBit,numOfBits));

    origPacketHash = descrPtr->pktHash;

    descrPtr->pktHash  = SMEM_U32_GET_FIELD(descrPtr->pktHash,startBit,numOfBits);

    if(origPacketHash != descrPtr->pktHash)
    {
        __LOG(("Hash value for egress pipe is [0x%x] different from Hash value for ingress pipe[0x%x] \n",
            descrPtr->pktHash,
            origPacketHash));
    }

    if(descrPtr->useVidx)
    {
        __LOG(("EQ to Egress : 'flood' (multi destination) to %s [0x%4.4x] (eVidx [0x%4.4x]) \n",
            ((devObjPtr->supportEArch && descrPtr->eArchExtInfo.vidx == 0xFFF) ||
                descrPtr->eVidx == 0xFFF) ? "Vlan" : "Vidx" ,
            ((devObjPtr->supportEArch && descrPtr->eArchExtInfo.vidx == 0xFFF) ||
                descrPtr->eVidx == 0xFFF) ? descrPtr->eVid :
                                                devObjPtr->supportEArch ? descrPtr->eArchExtInfo.vidx :
                                                descrPtr->eVidx,
            descrPtr->eVidx
            ));
    }
    else
    {
        __LOG(("EQ to Egress : single destination to device [0x%2.2x] ePort [0x%3.3x] phyPort[0x%2.2x] \n",
            descrPtr->trgDev,
            descrPtr->trgEPort,
            devObjPtr->supportEArch ?
                (descrPtr->eArchExtInfo.isTrgPhyPortValid ?
                    descrPtr->eArchExtInfo.trgPhyPort :
                    0xFFFFFFFF) :/*phyPort not valid*/
                descrPtr->trgEPort/* non eArch */
                ));
    }


    SIM_LOG_PACKET_DESCR_COMPARE("eqPrepareEgress");

    /* CNC : count the 'sending out of the EQ' */
    /* count the passing of the packet */
    __LOG(("CNC : check if this passing of EQ to Egress Pipe should be counted \n"));
    snetChtEqCncCount(devObjPtr, descrPtr);

    /* Call egress processing */
    __LOG(("Call egress processing"));
    snetChtEgress(devObjPtr, descrPtr);

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_EQ_E);

    /* restore packet hash ... just in case needed in the ingress pipe again */
    descrPtr->pktHash = origPacketHash;
    __LOG(("restore descrPtr->pktHash[0x%x] ... just in case needed in the ingress pipe again\n",
        descrPtr->pktHash));
}


/*******************************************************************************
*   snetChtEqHashIndexResolution
*
* DESCRIPTION:
*        EQ/IPvX - Hash Index Resolution (for eArch)
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*       numOfMembers - number of members to select index from.
*           this number may be different for each packet.
*           In ePort ECMP this number specifies the number of ECMP members,
*           in the range 1-64.
*           In Trunk member selection the possible values of #members is 1-8.
*       randomEcmpPathEnable  -      Random ECMP Path Enable
*       instanceType - instance type : trunk/ecmp .
*       selectedEPortPtr  - (pointer to) the primary EPort (for
*           SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L2_ECMP_E)
*
* OUTPUTS:
*       newHashIndexPtr - pointer to the new hash index to be used by the calling
*           engine.
*           the selected member. In ePort ECMP this is a number in the range 0-63,
*           while in trunk member selection it is a number in the range 0-7
*           NOTE: this value should not modify the descriptor value.
*       selectedDevNumPtr - (pointer to) the selected devNum field
*       selectedEPortPtr  - (pointer to) the selected EPort field
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetChtEqHashIndexResolution
(
    IN SKERNEL_DEVICE_OBJECT                          *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC                *descrPtr,
    IN GT_U32                                          numOfMembers,
    IN GT_U32                                          randomEcmpPathEnable,
    OUT GT_U32                                        *newHashIndexPtr,
    IN SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_ENT  instanceType,
    OUT GT_U32                                        *selectedDevNumPtr,
    INOUT GT_U32                                      *selectedEPortPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqHashIndexResolution);

    GT_U32 fldValue;                /* Register's field value */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 *regPtr;                 /* register pointer */
    GT_U32 *lttMemPtr;              /* LTT table entry pointer */
    GT_U32 numRightShiftBits;       /* number of bit to do right shift */
    GT_U32 hashBitSelectionConfigRegAddr = 0;/* hash Bit Selection Configuration Register Address */
    GT_U32 firstBit,lastBit;/*first bit and last bit to be used from the hash value*/
    GT_U32 ecmpIndex = 0;/* index in the ECMP table */
    GT_U32 ecmpMemberIndex;/* the index of the member from start of ECMP section in the table */
    GT_U32 lttIndex;/* index in the LTT */
    GT_U32 hashIndex = descrPtr->pktHash;
    GT_U32 globalTrgEPort;
    GT_U32 width_l2_ecmp_start_index;/*width of field L2_ECMP_Start_Index*/
    GT_U32 width_target_eport_phy_port; /*width of field Target_ePort_phy_port */

    if(instanceType == SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_TRUNK_E)
    {
        __LOG(("start calc trunk [%d] to get physical port member \n",
            descrPtr->trgTrunkId));

        regAddr = SMEM_LION2_EQ_TRUNK_LTT_TBL_MEM(devObjPtr,descrPtr->trgTrunkId);
        lttMemPtr = smemMemGet(devObjPtr, regAddr);
        hashBitSelectionConfigRegAddr = SMEM_LION2_EQ_TRUNK_HASH_BIT_SELECTION_CONFIG_REG(devObjPtr);
    }
    else if (instanceType == SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L2_ECMP_E)
    {
        globalTrgEPort = (*selectedEPortPtr);

        __LOG(("start calc L2 ECMP for global ePort [0x%x] to get secondary EPort \n",
                (*selectedEPortPtr)));

        /* get l2 ECMP Base ePort */
        regAddr = SMEM_LION2_EQ_L2_ECMP_INDEX_BASE_EPORT_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            fldValue = SMEM_U32_GET_FIELD(*regPtr,0,15);
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(*regPtr,0,13);
        }

        if(globalTrgEPort > fldValue)
        {
            __LOG((" the global port above 'l2 ECMP Base ePort'[0x%x] \n",
                fldValue));
            lttIndex = (*selectedEPortPtr) - fldValue;
        }
        else
        {
            __LOG((" the global port below or equal 'l2 ECMP Base ePort'[0x%x] so 'sticks to zero' \n",
                fldValue));
            /* sticks to zero */
            lttIndex = 0;
        }

        __LOG_PARAM(lttIndex);

        regAddr = SMEM_LION2_EQ_L2_ECMP_LTT_TBL_MEM(devObjPtr,lttIndex);
        lttMemPtr = smemMemGet(devObjPtr, regAddr);

        hashBitSelectionConfigRegAddr = SMEM_LION2_EQ_EPORT_ECMP_HASH_BIT_SELECTION_CONFIG_REG(devObjPtr);
    }
    else /*SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L3_ECMP_E*/
    {
        /* for sip5: lpm used instead ltt */
        lttMemPtr = NULL;
    }

    if(lttMemPtr)
    {
        /*ECMP Enable bit */
        if( 0 == snetFieldValueGet(lttMemPtr,0,1))
        {
            /* ecmp disabled */
            __LOG(("ecmp disabled , set target to be 'NULL port' (on local device) \n"));

            if(selectedDevNumPtr)
            {
                *selectedDevNumPtr = descrPtr->ownDev;
            }

            if(selectedEPortPtr)
            {
                *selectedEPortPtr = SNET_CHT_NULL_PORT_CNS;
            }

            return;
        }

        if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
        {
            width_l2_ecmp_start_index = 14;
        }
        else
        {
            width_l2_ecmp_start_index = 13;
        }
        /* L2 ECMP Start Index */
        ecmpIndex = snetFieldValueGet(lttMemPtr,2,width_l2_ecmp_start_index);
        __LOG_PARAM(ecmpIndex);

        /* Number of L2 ECMP Paths */
        numOfMembers = 1 + snetFieldValueGet(lttMemPtr,2+width_l2_ecmp_start_index,12);

        /* Random ECMP Path Enable */
        randomEcmpPathEnable = snetFieldValueGet(lttMemPtr,1,1);

        regPtr = smemMemGet(devObjPtr, hashBitSelectionConfigRegAddr);
        firstBit =  snetFieldValueGet(regPtr,0,5);
        lastBit  =  snetFieldValueGet(regPtr,5,5);

        regAddr = SMEM_LION3_EQ_L2_ECMP_TRUNK_LFSR_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr , 1,5, &numRightShiftBits);
    }
    else
    {
        /* sip5 ecmp l3 case */
        regPtr = smemMemGet(devObjPtr, SMEM_LION3_ECMP_CONFIG_REG(devObjPtr));
        firstBit          = snetFieldValueGet(regPtr,1,5);
        lastBit           = snetFieldValueGet(regPtr,6,5);
        numRightShiftBits = snetFieldValueGet(regPtr,12,5);
    }

    __LOG_PARAM(numOfMembers);
    __LOG_PARAM(randomEcmpPathEnable);
    __LOG_PARAM(firstBit);
    __LOG_PARAM(lastBit);
    __LOG_PARAM(numRightShiftBits);


    if(randomEcmpPathEnable)
    {
        /* the rand gives 15 bits value , but we need 16 bits value .
           so we add up 2 values .
           we NOT use rand function as the PP , to simplify implementation */
        hashIndex = rand() + rand();

        hashIndex &= 0xFFFF;

        __LOG(("use random hash [0x%4.4x] \n",hashIndex));
    }
    else
    {
        hashIndex = descrPtr->pktHash;
        __LOG(("use packet hash [0x%4.4x] \n",hashIndex));
    }

    if(firstBit && ((lastBit + 1)  < firstBit))
    {
        __LOG(("ERROR : lastBit[%d] less then firstBit[%d] \n",
            lastBit ,
            firstBit));

        lastBit = firstBit - 1;
    }

    hashIndex = SMEM_U32_GET_FIELD(hashIndex,firstBit,((lastBit+1) -firstBit));

    __LOG(("final packet hash [0x%4.4x] after using bits[%d to %d] \n",
        hashIndex,firstBit,lastBit));

    /*  The multiplication result is shifted right according to the number in this field.
        This number should be equal to the number of bits in the input hash.
        For example, if the input hash is 6 bits (and zero padding), then this field should be equal to 6.
        The default is 16 shifts, since the assumption is that the hash is 16 bits long.
    */
    __LOG(("numRightShiftBits [%d] from register \n",numRightShiftBits));

    if(numRightShiftBits > 16)
    {
        __LOG((" ERROR : numRightShiftBits should NOT be more then 16 \n"));
    }

    ecmpMemberIndex = (hashIndex * numOfMembers) >> numRightShiftBits;

    __LOG(("ecmpMemberIndex [0x%x] \n",ecmpMemberIndex));

    /* protection for out of boundary */
    if(ecmpMemberIndex >= numOfMembers)
    {
        /* NOTE: do not fatal error on this case since there could be some
            kind of WA that actual members are beyond the 'numOfMembers'

            so just add LOG indication for 'strange' configuration of numRightShiftBits
        */

        __LOG(("ERROR : the calculated ecmpMemberIndex [%d] is >= then numOfMembers[%d] ...so access violation  \n",
               "I hope you know what you are doing !! numRightShiftBits[%d]\n",
               ecmpMemberIndex,
               numOfMembers,
               numRightShiftBits));
    }


    if(lttMemPtr)
    {
        /* add the offset to the final index */
        ecmpIndex += ecmpMemberIndex;

        __LOG(("final ecmpIndex [0x%x] \n",ecmpIndex));

        regAddr = SMEM_LION2_EQ_L2_ECMP_TBL_MEM(devObjPtr,ecmpIndex);
        regPtr = smemMemGet(devObjPtr, regAddr);

        if(selectedDevNumPtr)
        {
            /*target device*/
            *selectedDevNumPtr =  snetFieldValueGet(regPtr,0,10);

            __LOG(("from ECMP table : target device [0x%x] \n",*selectedDevNumPtr));
        }

        if(selectedEPortPtr)
        {
            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                width_target_eport_phy_port = 14;
            }
            else
            {
                width_target_eport_phy_port = 13;
            }

            /*target port*/
            *selectedEPortPtr =  snetFieldValueGet(regPtr,10,width_target_eport_phy_port);

            __LOG(("from ECMP table : target port [0x%x] \n",*selectedEPortPtr));
        }
    }

    *newHashIndexPtr = ecmpMemberIndex;
}

/*******************************************************************************
*   trunkHashIndexGet
*
* DESCRIPTION:
*       trunk hash index - for single destination
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       trunkId             - trunk Id
*       pktHash             - packet hash value
* OUTPUTS:
*      trunkHashPtr         - pointer to trunk hash index
*
*******************************************************************************/
static GT_VOID trunkHashIndexGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   trunkId,
    IN GT_U32   pktHash,
    OUT GT_U32  *trunkHashPtr
)
{
    GT_U32 regAddr;                 /* Register address */
    GT_U32 fldValue;                /* Register's field value */
    GT_U32 fldBit;                  /* Register's field value */

    /* register address */
    regAddr = SMEM_CHT_TRUNK_MEM_NUM_TBL_MEM(devObjPtr, trunkId);
    /* start field bit */
    fldBit = SMEM_CHT_TRUNK_MEM_START_FIELD(trunkId);

    smemRegFldGet(devObjPtr, regAddr, fldBit, 4, &fldValue);

    /* Number of members in Trunk # trunkId */
    if (fldValue > 1)
    {
        *trunkHashPtr = pktHash % fldValue;
    }
    else
    {
        *trunkHashPtr = 0;
    }
}

/*******************************************************************************
*   trunkMemberGet
*
* DESCRIPTION:
*       select trunk member {port,device}- for single destination
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*       trunkId             - trunk Id
*       trunkHash           - trunk hash index
* OUTPUTS:
*
*******************************************************************************/
static GT_VOID trunkMemberGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32   trunkId
)
{
    DECLARE_FUNC_NAME(trunkMemberGet);

    GT_U32 trunkTableEntry;
    GT_U32 trunkHash;
    GT_U32 regAddr;                     /* register's address */
    GT_U32 trgPort;
    GT_U32 trgDevNum;
    GT_U32    dummy;            /* ecmp offset from start index */
    GT_U32 trgTrunkId = descrPtr->trgTrunkId;/*the trunkId , saved for use at end of function */
    GT_U32 trgPhyPort;/*target physical port*/

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* get the selected member */
        snetChtEqHashIndexResolution(devObjPtr,descrPtr, 0, 0, &dummy,
            SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_TRUNK_E,
            &trgDevNum,
            &trgPort);

        descrPtr->eArchExtInfo.isTrgPhyPortValid = 1;
        trgPhyPort = trgPort & 0xFF;
        descrPtr->eArchExtInfo.trgPhyPort = trgPhyPort;
        /*NOTE: InDesc<TargetIsTrunk> refers to the incoming descriptor to the E2PHY
        table. The idea is that if the descriptor to the E2PHY has this bit set, then it does not contain a
        target ePort. In this case the EQ assigns a target ePort that is equal to the physical port in the
        result of the trunk member table lookup.*/
        descrPtr->trgEPort =
            (descrPtr->eArchExtInfo.eqInfo.IN_descTrgIsTrunk) ?
                trgPort :                            /* use field from the entry */
                descrPtr->eArchExtInfo.eqInfo.IN_descTrgEPort; /* this is 'ePort that used as trunk' */

        descrPtr->trgDev = trgDevNum;

    }
    else
    {
        trunkHashIndexGet(devObjPtr,descrPtr,trgTrunkId,
                            descrPtr->pktHash,&trunkHash);

        regAddr = SMEM_CHT_TRUNK_TBL_MEM(devObjPtr,
                                          trunkHash,
                                          trunkId);/*trunk id*/

        /* Trunk table Trunk<n> Member<i> Entry (1<=n<128, 0<=i<8) */
        smemRegGet(devObjPtr, regAddr, &trunkTableEntry);

        /* Trunk<n> Member<i> PortNum */
        descrPtr->trgEPort = SMEM_U32_GET_FIELD(trunkTableEntry, 0, 6);
        trgPhyPort = descrPtr->trgEPort;

        /* Trunk<n> Member<i> DevNum */
        descrPtr->trgDev = SMEM_U32_GET_FIELD(trunkTableEntry, 6, 5);
    }
    /* we selected a trunk member so now reset the indication of 'target is trunk' */
    descrPtr->targetIsTrunk = 0;

    __LOG(("for trunk[%d] , selected : target device [%d] target physical port[%d] \n",
        trgTrunkId,
        descrPtr->trgDev,
        trgPhyPort)); /* This is the physical port (not EPort)*/

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        __LOG(("for trunk[%d] : set isTrgPhyPortValid = 1 , trgEPort[%x] \n",
            trgTrunkId,
            descrPtr->trgEPort)); /* This is the EPort port (not physical)*/
    }


    return;
}

/*******************************************************************************
*   eqMirrorAnalyzerHopByHop
*
* DESCRIPTION:
*       EQ analyzer for HOP-by-HOP system
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqMirrorAnalyzerHopByHop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32               analyzerPortGlobalConfig,
    IN GT_BIT               isRxMirror
)
{
    DECLARE_FUNC_NAME(eqMirrorAnalyzerHopByHop);

    GT_U32    bad_analyzer_index_drop_code;/*Bad Analyzer Index Drop Code*/
    GT_U32    global_Mirror_Index;

    if(isRxMirror)
    {
        /* Global Ingress Mirror Index */
        global_Mirror_Index = SMEM_U32_GET_FIELD(analyzerPortGlobalConfig, 2, 3);
    }
    else
    {
        /* Global Egress Mirror Index */
        global_Mirror_Index = SMEM_U32_GET_FIELD(analyzerPortGlobalConfig, 5, 3);
    }

    __LOG_PARAM(isRxMirror);
    __LOG_PARAM(global_Mirror_Index);

    if(descrPtr->analyzerIndex && global_Mirror_Index == 0)
    {
        __LOG(("WARNING: In HOP-BY-HOPE 'global index' 0 force NO mirroring although the ingress pipe hold reason for mirroring \n"));
    }
    else
    {
        __LOG(("In HOP-BY-HOPE use only the analyzer index given by 'global index' [%d] \n",
            global_Mirror_Index));
    }

    descrPtr->analyzerIndex = global_Mirror_Index;

    if(descrPtr->analyzerIndex == 0 &&
       descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
    {
        /* we are not in the ingress device , but we have no 'target'
           information where is the analyzer */
        __LOG(("WARNING : In HOP-BY-HOPE when we are not in the ingress device , when 'global index' 0 meaning no 'target' information about analyzer port \n"));

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            bad_analyzer_index_drop_code = SMEM_U32_GET_FIELD(analyzerPortGlobalConfig, 8, 8);
        }
        else
        {
            bad_analyzer_index_drop_code = SNET_CHT_EQ_BAD_ANALYZER_INDEX_DROP_ERROR; /* 0x64 = 78 */
        }

        __LOG_PARAM((bad_analyzer_index_drop_code));

        descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_HARD_DROP_E;
        descrPtr->cpuCode = bad_analyzer_index_drop_code;

        __LOG(("WARNING : packet will get HARD_DROP with drop reason[%d] \n",
            bad_analyzer_index_drop_code));
    }
}


/*******************************************************************************
*   eqIngressMirrorAnalyzer
*
* DESCRIPTION:
*       EQ ingress analyzer stage
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqIngressMirrorAnalyzer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(eqIngressMirrorAnalyzer);

    GT_U32    regAddress;           /* register address */
    GT_U32    fieldVal;             /* field value */
    GT_U32    *regPtr;              /* pointer to register data */
    GT_U32    localDevSrcPort;      /* local ingress physical port on the device */
    GT_U32    analyzerPortGlobalConfig;/* register value of "analyzer Port Global Config"*/
    GT_BIT    doPerEPort = 0;/* do mirroring according to per EPort setting */
    GT_BIT    doPerPhysicalPort = 0;/* do mirroring according to per physical port setting */
    GT_BIT    doGlobalMirror = 0;/* do mirroring according to global setting */
    GT_U32    step;                 /* register fields step */

    regAddress = SMEM_XCAT_ANALYZER_PORT_GLOBAL_CONF_REG(devObjPtr);
    smemRegGet(devObjPtr, regAddress,&analyzerPortGlobalConfig);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*Mirroring Mode*/
        fieldVal = SMEM_U32_GET_FIELD(analyzerPortGlobalConfig, 0, 2);

        if(fieldVal == 0)
        {
            descrPtr->mirroringMode = SKERNEL_MIRROR_MODE_END_TO_END_E;
        }
        else if(fieldVal == 1)
        {
            descrPtr->mirroringMode = SKERNEL_MIRROR_MODE_HOP_BY_HOP_E;
        }
        else if(fieldVal == 2)
        {
            descrPtr->mirroringMode = SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E;
        }
        else
        {
            skernelFatalError("eqIngressMirrorAnalyzer: unknown mirror mode ! \n");
        }
    }
    else
    {
        /* Rx from local dev and rx/tx from DSA mirroring uses this bit on the ingress */
        fieldVal = SMEM_U32_GET_FIELD(analyzerPortGlobalConfig, 0, 1);

        /* Source-based forwarding mode. */
        descrPtr->mirroringMode = fieldVal ?
                            SKERNEL_MIRROR_MODE_HOP_BY_HOP_E : /* Hop-by-hop forwarding mode */
                            SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E; /* Source-based forwarding mode */
    }

    __LOG(("mirroring mode : %s \n",
            descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_HOP_BY_HOP_E ? "HOP_BY_HOP" :
            descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E ? "SRC_BASED_OVERRIDE_SRC_TRG" :
            descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_END_TO_END_E ? "END_TO_END_E" :
            "unknown"
        ));

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)/* not with DSA 'to analyzer' */
    {
        if(descrPtr->mirroringMode != SKERNEL_MIRROR_MODE_HOP_BY_HOP_E) /* Source-based forwarding mode */
        {
            doPerPhysicalPort = 1;
        }
        else /* Hop-by-hop forwarding mode. */
        /* The global index for mirroring is used only when there is a reason
           for rx mirroring */
        if(descrPtr->analyzerIndex || descrPtr->rxSniff)
        {
            doGlobalMirror = 1;
            /* Ingress mirroring can be either port-based, or can be triggered from any of the ingress engines
               This bit specifies the analyzer index used for ingress mirroring from all engines except port-mirroring (Bridge, VLAN). */
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            doPerEPort = 1;
            doPerPhysicalPort = 1;
        }
    }
    else /* DSA 'to analyzer' */
    {
        if(descrPtr->mirroringMode != SKERNEL_MIRROR_MODE_HOP_BY_HOP_E)
        {
            if(descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E)
            {
                descrPtr->trgEPort     = descrPtr->eArchExtInfo.srcTrgEPort;
                descrPtr->trgDev       = descrPtr->srcTrgDev;

                __LOG(("take <descrPtr->trgEPort>[0x%x] from <srcTrgEPort> \n" ,
                    descrPtr->trgEPort));
                __LOG(("take <descrPtr->trgDev>[0x%x] from <srcTrgDev> \n" ,
                    descrPtr->trgDev));
            }

            /* Source-based and End-to-end forwarding modes use Ingress DSA
               information for analyzer port. */
            return;
        }

        if(descrPtr->rxSniff == 0)
        {
            /* not RX mirroring but TX mirroring */
            /* no more to do here */
            return;
        }
    }

    /* get per EPort index */
    if((doPerEPort == 1) &&
        descrPtr->eArchExtInfo.eqIngressEPortTablePtr)
    {
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            step = 7;
        }
        else
        {
            step = 8;
        }

        /*Ingress Mirror Index*/
        fieldVal =
            snetFieldValueGet(descrPtr->eArchExtInfo.eqIngressEPortTablePtr,
                4 + ((descrPtr->eArchExtInfo.eqInfo.eqIngressEPort & 0x3) * step),
                3);

        if(fieldVal)
        {
            __LOG(("Ingress EPort Mirror Index %d \n",fieldVal));
        }

        snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr,descrPtr,fieldVal);
    }

    /* get per physical port index */
    if(doPerPhysicalPort)
    {
        /* Mirrored by the port-based ingress mirroring mechanism */
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            localDevSrcPort = descrPtr->localDevSrcPort;

            regAddress = SMEM_LION2_EQ_PHYSICAL_PORT_INGRESS_MIRROR_INDEX_TBL_MEM(devObjPtr, localDevSrcPort);
            regPtr = smemMemGet(devObjPtr, regAddress);
            /* Per-port Ingress Mirror Index - 8 ports in entry , 3 bits in steps of 4 */
            fieldVal =
                SMEM_U32_GET_FIELD(*regPtr, ((localDevSrcPort % 8) * 4), 3);
        }
        else
        {
            if(descrPtr->localDevSrcPort == SNET_CHT_CPU_PORT_CNS)
            {
                /* CPU port located after the physical 28 ports for all devices
                supported multi analyzer mirroring */
                localDevSrcPort = 28;
            }
            else
            {
                /* Non CPU port */
                localDevSrcPort = descrPtr->localDevSrcPort;
            }

            regAddress = SMEM_XCAT_INGR_MIRROR_INDEX_REG(devObjPtr, localDevSrcPort);
            regPtr = smemMemGet(devObjPtr, regAddress);
            /* Per-port Ingress Mirror Index */
            fieldVal =
                SMEM_U32_GET_FIELD(*regPtr, ((localDevSrcPort % 10) * 3), 3);
        }

        if(fieldVal)
        {
            __LOG(("per physical port [%d] : mirror index [%d] \n",
                descrPtr->localDevSrcPort,
                fieldVal));
        }

        snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr, descrPtr, fieldVal);
        /* mark that there is reason for Rx mirroring .
        Index 0 - no mirroring for the ingress port,
        otherwise mirroring is performed for the ingress port. */
        descrPtr->rxSniff = (descrPtr->analyzerIndex == 0) ? 0 : 1;
    }

    if(descrPtr->analyzerIndex || descrPtr->rxSniff)
    {
        doGlobalMirror = 1;
    }

    /* get global index */
    if(doGlobalMirror)
    {
        /* The global index for mirroring is used only when there is a reason
           for rx mirroring */

        /* Global Ingress Mirror Index */
        fieldVal = SMEM_U32_GET_FIELD(analyzerPortGlobalConfig, 2, 3);
        if(fieldVal)
        {
            __LOG(("Global Ingress Mirror Index : mirror index [%d] \n",
                fieldVal));
        }

        if(descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_HOP_BY_HOP_E)
        {
            eqMirrorAnalyzerHopByHop(devObjPtr,descrPtr,analyzerPortGlobalConfig,GT_TRUE/*RX mirror*/);
        }
        else/* non HOP-by-HOP */
        {
            snetXcatIngressMirrorAnalyzerIndexSelect(devObjPtr,descrPtr,fieldVal);
        }

        if (descrPtr->analyzerIndex == 0)
        {
            /* Index 0x0 stands for No Mirroring */
            __LOG(("No ingress Mirroring \n"));
            descrPtr->rxSniff = 0;
        }
        else
        {
            __LOG(("Final Ingress Mirror Index : mirror index [%d] \n",
                descrPtr->analyzerIndex));

            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* for the sake of the rest of using <rxSniff> set it when descrPtr->analyzerIndex != 0 */
                descrPtr->rxSniff = 1;
            }
        }
    }

}

/*******************************************************************************
*   eqL2EcmpSubUnit
*
* DESCRIPTION:
*        EQ - l2Ecmp sub-block processing
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqL2EcmpSubUnit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(eqL2EcmpSubUnit);

    GT_U32    regAddress;           /* register address */
    GT_U32    *regPtr;              /* pointer to register data */
    GT_U32    *trgEPortPtr;         /* pointer to the target eport , as we may need to modify it's value ...*/
    GT_U32    *isTrgPhyPortValidPtr;   /* pointer to the target EPort valid , as we may need to modify it's value ...*/
    GT_U32    *trgDevPtr;           /* pointer to the target device , as we may need to modify it's value ...*/
    GT_U32    useVidx;              /*use vidx*/
    GT_U32    targetIsTrunk;        /* target is trunk */
    GT_U32    ecmpEPortMask,ecmpEPortValue;/* mask and value for l2-ecmp eports */
    GT_U32     dummy;
    GT_U32     trgEPort;
    GT_U32    eport_ecmp_lookup_enable;

    regAddress = SMEM_LION3_EQ_EPORT_GLOBAL_CONFIG_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddress ,0,1,&eport_ecmp_lookup_enable);
    if(0 == eport_ecmp_lookup_enable)
    {
        __LOG(("ePORT ECMP lookup globally disabled (L2ECMP) \n"));
        return;
    }

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CPU_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
    {
        __LOG(("Global EPort not relevant to this packet command \n"));
        return;
    }

    useVidx = descrPtr->useVidx;
    targetIsTrunk = descrPtr->targetIsTrunk;
    trgEPortPtr = &descrPtr->trgEPort;
    isTrgPhyPortValidPtr = &descrPtr->eArchExtInfo.isTrgPhyPortValid;
    trgDevPtr = &descrPtr->trgDev;

    if(useVidx == 1 || targetIsTrunk == 1)
    {
        __LOG(("Global EPort not relevant to target trunk / vidx \n"));
        return;
    }

    /* get ecmpEPortValue */
    regAddress = SMEM_LION2_EQ_L2_ECMP_EPORT_VALUE_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddress);
    ecmpEPortValue = SMEM_U32_GET_FIELD(*regPtr,0,13);

    /* get ecmpEPortMask */
    regAddress = SMEM_LION2_EQ_L2_ECMP_EPORT_MASK_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddress);
    ecmpEPortMask = SMEM_U32_GET_FIELD(*regPtr,0,13);

    trgEPort = *trgEPortPtr;

    __LOG_PARAM(ecmpEPortValue);
    __LOG_PARAM(ecmpEPortMask);
    __LOG_PARAM(trgEPort);

    if((trgEPort & ecmpEPortMask) != ecmpEPortValue)
    {
        /* the target eport is not considered as primary ECMP eport */
        __LOG(("the target eport is not considered as primary ECMP eport (NOT global ePort) \n"));
        return;
    }

    /* get the ECMP offset */
    snetChtEqHashIndexResolution(devObjPtr,descrPtr, 0, 0, &dummy,
        SNET_CHT_EQ_HASH_INDEX_RESOLUTION_INSTANCE_L2_ECMP_E,
        trgDevPtr,trgEPortPtr);

    *isTrgPhyPortValidPtr = 0;

    __LOG(("(secondary ePort) : target device [%d] target port[%d] \n",
        *trgDevPtr,
        *trgEPortPtr));

}

/*******************************************************************************
* eqE2PhyMtuCheck
*
* DESCRIPTION:
*        EQ - E2PHY - target ePort MTU check sub-block processing
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqE2PhyMtuCheck
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *descrPtr,
    IN GT_U32                           mtuProfileIndex
)
{
    DECLARE_FUNC_NAME(eqE2PhyMtuCheck);

    GT_U32     isMtuCheckEnabled;
    GT_U32     byteCountMode;
    GT_U32     regAddr;
    GT_U32     mtuSizeConfig, packetSize;
    GT_U32     cpuCode;
    GT_U32     exceptionCmd;
    GT_U32     exceededCntr;

    /* check if MTU feature is enabled*/
    regAddr = SMEM_LION3_EQ_PRE_EGRESS_ENGINE_MTU_GLOBAL_CONFIG_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 0, 1, &isMtuCheckEnabled);
    if (isMtuCheckEnabled == 0)
    {
        /* feature disabled - nothing to be done */
        __LOG(("EQ - target ePort MTU check is globally disabled\n"));
        return;
    }
    __LOG(("do MTU check - for mtuProfileIndex= [%d]\n", mtuProfileIndex));

    /* read the MTU size per profile */
    regAddr = SMEM_LION3_EQ_PRE_EGRESS_MTU_SIZE_PER_PROFILE_REG(devObjPtr, mtuProfileIndex);
    smemRegFldGet(devObjPtr, regAddr, 0, 14, &mtuSizeConfig);
    __LOG(("got relevant MTU size: [%d] \n", mtuSizeConfig));

    /* read the byte mode count from register*/
    regAddr = SMEM_LION3_EQ_PRE_EGRESS_ENGINE_MTU_GLOBAL_CONFIG_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 1, 1, &byteCountMode);

    packetSize = descrPtr->byteCount;
    if (byteCountMode == 0)
    {   /* L3 byte count */
        __LOG(("L3 byte count \n"));
        packetSize -= descrPtr->l2HeaderSize - 4;  /*remove L2 header and 4 for CRC */
    }
    else
    {   /* L2 mode byte count*/
        __LOG(("L2 byte count \n"));
        if (descrPtr->marvellTagged && descrPtr-> tunnelTerminated == 0)
        {
            /* decrement the bytes of the DSA tag */
            __LOG(("Decrement the bytes of the DSA tag (remove [%d] bytes) \n",
                ((descrPtr->marvellTaggedExtended + 1)*4)));
            packetSize -= (descrPtr->marvellTaggedExtended + 1)*4;
            if(descrPtr->origSrcTagged)
            {   /* add the bytes of the original tag (that used in first word of the DSA tag) */
                __LOG(("Add the 4 bytes of the original tag (that used in first word of the DSA tag)\n"));
                packetSize += 4;
            }
        }
    }
    __LOG(("got packet size calc: [%d] to compare with MTU size [%d] \n", packetSize, mtuSizeConfig));

    if (packetSize > mtuSizeConfig)
    {
        __LOG(("there is MTU exception \n"));
        /* read and modify exception command and CPU code*/
        regAddr = SMEM_LION3_EQ_PRE_EGRESS_ENGINE_MTU_GLOBAL_CONFIG_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 2, 11, &exceptionCmd);
        cpuCode = exceptionCmd >> 3;
        exceptionCmd &= 0x07;
        __LOG(("new exceptionCmd: [%d]   new CPUcode: [%d]\n",
               exceptionCmd, cpuCode));
        snetChtIngressCommandAndCpuCodeResolution(devObjPtr, descrPtr,
                                                  descrPtr->packetCmd,
                                                  exceptionCmd,
                                                  descrPtr->cpuCode,
                                                  cpuCode,
                                                  SNET_CHEETAH_ENGINE_UNIT_EQ_E,
                                                  GT_FALSE);
        /* update the exceeded packet counter - read/increment/write */
        regAddr = SMEM_LION3_EQ_PRE_EGRESS_ENGINE_MTU_EXCEED_CNTR_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 0, 32, &exceededCntr);
        exceededCntr++;
        smemRegFldSet(devObjPtr, regAddr, 0, 32, exceededCntr);
        __LOG(("Update exceeded MTU counters from: [%d] \n", (exceededCntr - 1)));
    }
}

/*******************************************************************************
*   eqE2PhyTableAccess
*
* DESCRIPTION:
*        EQ - E2Phy (ePort to Physical port mapping table) sub-block processing
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        tableIndex      - E2PHY table index
*
* OUTPUTS:
*        trgPortPtr        - pointer to the target port , as we may need to modify it's value ...
*        trgDevPtr         - pointer to the target device , as we may need to modify it's value ...
*        trgUseVidxPtr     - pointer to the target use vidx , as we may need to modify it's value ...
*        trgEVidxPtr       - pointer to the target EVidx , as we may need to modify it's value ...
*        trgVidxPtr        - pointer to the target vidx , as we may need to modify it's value ...
*        isTrgPhyPortValidPtr - pointer to the target is valid port  , as we may need to modify it's value ...
*        targetIsTrunkPtr  - pointer to the target is trunk , as we may need to modify it's value ...
*        targetTrunkIdPtr  - pointer to the target trunkId , as we may need to modify it's value ...
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqE2PhyTableAccess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32     tableIndex,
    OUT GT_U32    *trgPortPtr,
    OUT GT_U32    *trgDevPtr,
    OUT GT_U32    *trgUseVidxPtr,
    OUT GT_U32    *trgEVidxPtr,
    OUT GT_U32    *trgVidxPtr,
    OUT GT_U32    *isTrgPhyPortValidPtr,
    OUT GT_U32    *targetIsTrunkPtr,
    OUT GT_U32    *targetTrunkIdPtr,
    OUT GT_U32    *mtuProfileIndexPtr
)
{
    GT_U32    regAddress;           /* register address */
    GT_U32    *regPtr;              /* pointer to register data */
    GT_U32    entryTargetIsTrunk,entryUseVidx;/* fields from the table entry*/

    regAddress = SMEM_LION2_EQ_E2PHY_TBL_MEM(devObjPtr,tableIndex);
    regPtr = smemMemGet(devObjPtr, regAddress);

    entryTargetIsTrunk =
        snetFieldFromEntry_GT_U32_Get(devObjPtr,
            regPtr,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
            tableIndex,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
            SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_IS_TRUNK);

    entryUseVidx =
        snetFieldFromEntry_GT_U32_Get(devObjPtr,
            regPtr,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
            tableIndex,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
            SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_USE_VIDX);

    if ((entryTargetIsTrunk == 0) && (entryUseVidx == 0))
    {
        *trgPortPtr =
            snetFieldFromEntry_GT_U32_Get(devObjPtr,
                regPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
                tableIndex,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
                SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_PHYSICAL_PORT);
        *trgDevPtr =
            snetFieldFromEntry_GT_U32_Get(devObjPtr,
                regPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
                tableIndex,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
                SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_DEVICE);
        *isTrgPhyPortValidPtr = 1;
        *trgUseVidxPtr = 0;
        *targetIsTrunkPtr = 0;
    }
    else if ((entryTargetIsTrunk == 1) && (entryUseVidx == 0))
    {
        *targetTrunkIdPtr =
            snetFieldFromEntry_GT_U32_Get(devObjPtr,
                regPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
                tableIndex,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
                SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_TRUNK);
        *isTrgPhyPortValidPtr = 1;
        *trgUseVidxPtr = 0;
        *targetIsTrunkPtr = 1;
    }
    else if ((entryTargetIsTrunk == 0) && (entryUseVidx == 1))
    {
        *trgVidxPtr =
            snetFieldFromEntry_GT_U32_Get(devObjPtr,
                regPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
                tableIndex,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
                devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
                SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_TARGET_VIDX);
        *trgUseVidxPtr = 1;
        *targetIsTrunkPtr = 0;
    }

    *mtuProfileIndexPtr =
        snetFieldFromEntry_GT_U32_Get(devObjPtr,
            regPtr,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].formatNamePtr,
            tableIndex,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsInfoPtr,
            devObjPtr->tableFormatInfo[SKERNEL_TABLE_FORMAT_E2PHY_E].fieldsNamePtr,
            SMEM_LION3_EQ_E2PHY_TABLE_FIELDS_MTU_INDEX);
}


/*******************************************************************************
*   eqE2PhySubUnit
*
* DESCRIPTION:
*        EQ - E2Phy (ePort to Physical port mapping table) sub-block processing
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        isAnalyzerPort  - access for Analyzer port
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqE2PhySubUnit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL      isAnalyzerPort
)
{
    DECLARE_FUNC_NAME(eqE2PhySubUnit);

    GT_U32    trgEPort;             /* the target eport*/
    GT_U32    *isTrgPhyPortValidPtr;   /* pointer to the target EPort valid , as we may need to modify it's value ...*/
    GT_U32    *trgUseVidxPtr;       /* pointer to the target use vidx , as we may need to modify it's value ...*/
    GT_U32    *trgVidxPtr;          /* pointer to the target vidx , as we may need to modify it's value ...*/
    GT_U32    *trgEVidxPtr;         /* pointer to the target EVidx , as we may need to modify it's value ...*/
    GT_U32    *targetIsTrunkPtr;    /* pointer to the target is trunk , as we may need to modify it's value ...*/
    GT_U32    *targetTrunkIdPtr;    /* pointer to the target trunkId  , as we may need to modify it's value ...*/
    GT_U32    *trgDevPtr;           /* pointer to the target device , as we may need to modify it's value ...*/
    GT_U32    *trgPortPtr;          /* pointer to the target port , as we may need to modify it's value ...*/
    GT_U32    mtuProfileIndex;      /* the MTU profile index */


    /* save value of the 'IN descriptor'*/
    descrPtr->eArchExtInfo.eqInfo.IN_descTrgIsTrunk = descrPtr->targetIsTrunk;
    descrPtr->eArchExtInfo.eqInfo.IN_descTrgEPort = descrPtr->trgEPort;

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CPU_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
    {
        __LOG(("E2PHY not accessed due to descrPtr->packetCmd[%d] \n",
            descrPtr->packetCmd));
        return;
    }

    if(SKERNEL_IS_MATCH_DEVICES_MAC(descrPtr->trgDev, descrPtr->ownDev,
                                    devObjPtr->dualDeviceIdEnable.eq) == 0)
    {
        __LOG(("E2PHY not accessed due to not 'own' device \n"));
        return;
    }


    trgUseVidxPtr = &descrPtr->useVidx;
    targetIsTrunkPtr = &descrPtr->targetIsTrunk;
    targetTrunkIdPtr = &descrPtr->trgTrunkId;
    trgEPort = descrPtr->trgEPort;
    isTrgPhyPortValidPtr = &descrPtr->eArchExtInfo.isTrgPhyPortValid;
    trgDevPtr = &descrPtr->trgDev;
    trgPortPtr = &descrPtr->eArchExtInfo.trgPhyPort;
    trgVidxPtr = &descrPtr->eArchExtInfo.vidx;
    trgEVidxPtr = &descrPtr->eVidx;
    mtuProfileIndex = 0;

    if((*trgUseVidxPtr) == 1 ||
       (*targetIsTrunkPtr) == 1 ||
       (*isTrgPhyPortValidPtr) == 1)
    {
        __LOG(("E2PHY not accessed due to useVidx[%d],isTrunk[%d], isTrgPhyPortValid[%d] \n",
            (*trgUseVidxPtr),
            (*targetIsTrunkPtr),
            (*isTrgPhyPortValidPtr)
            ));
        return;
    }

    /* NOTE:
        It is assumed that the application configures entries
            60, 61, 62, 63 to map the ePort the corresponding physical port,
            i.e., 60-->60, 61-->61, 62-->62, 63-->63.
    */
#if 1   /* neet to check that CPSS initialize 60..63 accordingly before mask the code */
    if(SMEM_CHT_PORT_IS_SPECIAL_MAC(devObjPtr,trgEPort))
    {
        /*  Entries 62 and 63 in the E2PHY table are never accessed.
            It is assumed that the application configures entries 60 and 61 to
            map the ePort the corresponding physical port, i.e., 60-->60, 61-->61 */

        __LOG(("E2PHY not accessed for trgEPort[%d] \n",
            trgEPort));
        return;
    }
#endif/*0*/


    if(devObjPtr->errata.eqNotAccessE2phyForSniffedOnDropDuplication &&
       descrPtr->eArchExtInfo.eqInfo.IN_descDrop && (isAnalyzerPort == GT_TRUE))
    {
        __LOG(("E2PHY not accessed for the Sniffed-on-Drop duplication \n"));

        __LOG(("Assign NULL port to get drop down the pipe \n"));
        descrPtr->trgEPort = SNET_CHT_NULL_PORT_CNS;/* WA to get drop outside the EQ unit */
        __LOG_PARAM((descrPtr->trgEPort));

        return;
    }

    eqE2PhyTableAccess(devObjPtr, descrPtr,
                        trgEPort,
                        trgPortPtr,
                        trgDevPtr,
                        trgUseVidxPtr,
                        trgVidxPtr,
                        trgEVidxPtr,
                        isTrgPhyPortValidPtr,
                        targetIsTrunkPtr,
                        targetTrunkIdPtr,
                        &mtuProfileIndex);

    /* add support for target ePort MTU check enable feature */
    eqE2PhyMtuCheck (devObjPtr, descrPtr, mtuProfileIndex);

}



/*******************************************************************************
*   eqEgressVlanFilteringForLogicalPorts
*
* DESCRIPTION:
*       The mechanism is used for egress VLAN filtering in a logical port level,
*       unlike the TXQ egress-filtering which is done on a physical port level
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID eqEgressVlanFilteringForLogicalPorts(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    GT_U32    regAddress;           /* register address */
    GT_U32    fieldVal;             /* field value */
    GT_U32    *regPtr;              /* pointer to register data */
    GT_U32    index;
    GT_U32    xCoordinate;
    GT_U32    yCoordinate;
    GT_U32    vidIndex;/* vid index that represent the VID */
    GT_U32    logicalPortIndex;/* the logical port index */

    if(devObjPtr->supportLogicalMapTableInfo.supportEgressVlanFiltering == 0)
    {
        return;
    }

    if(descrPtr->useVidx == 1 ||
       descrPtr->targetIsTrunk == 1)
    {
        return;
    }

    logicalPortIndex =  (SMEM_U32_GET_FIELD(descrPtr->trgDev,0,5) << 6) | SMEM_U32_GET_FIELD(descrPtr->trgEPort,0,6);


    /* VLAN mapping table */
    index = descrPtr->vid0Or1AfterTti;
    regAddress = SMEM_XCAT_EQ_VLAN_MAPPING_TABLE_TBL_MEM(devObjPtr,index);
    regPtr = smemMemGet(devObjPtr, regAddress);
    vidIndex = SMEM_U32_GET_FIELD(regPtr[0],0,12);


    /* LP Egress VLAN member access mode */
    regAddress = SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddress);
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 21 , 3);

    /*
        Selects the mode in which the LP Egress VLAN member table is accessed,
        according to the amount of VIDs and LP in the system.
        0 = For {4K VID, 32 LP}
            X = {VID_IDX[11:6]}, Y = {VID_IDX[5:0], LP_IDX[4:0]}
        1 = For {2K VID, 64 LP}
            X ={VID_IDX[10:5]}, Y = {VID_IDX[4:0],LP_IDX[5:0]}
        2 = For {1K VID, 128 LP}
            X ={VID_IDX[9:4]}, Y = {VID_IDX[3:0],LP_IDX[6:0]}
        3 = For {512 VID, 256 LP}
            X = {VID_IDX[8:3]}, Y = {VID_IDX[2:0],LP_IDX[7:0]}
        4 = For {256 VID, 512 LP}
            X = {VID_IDX[7:2]}, Y = {VID_IDX[1:0],LP_IDX[8:0]}
        5 = For {128 VID, 1K LP}
            X ={VID_IDX[6:1]}, Y = {VID_IDX[0],LP_IDX[9:0]}
        6 = Reserved For {64 VID, 2K LP}
            X ={VID_IDX[5:0]}, Y = {LP_IDX[10:0]}
        7 = Reserved For {32 VID, 4K LP}
            X ={VID_IDX[4:0],LP_IDX[11]}, Y = {LP_IDX[10:0]}
    */
    switch(fieldVal)
    {
        case 0:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,6,6);
            yCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,6) << 5) | SMEM_U32_GET_FIELD(logicalPortIndex,0,5);
            break;
        case 1:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,5,6);
            yCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,5) << 6) | SMEM_U32_GET_FIELD(logicalPortIndex,0,6);
            break;
        case 2:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,4,6);
            yCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,4) << 7) | SMEM_U32_GET_FIELD(logicalPortIndex,0,7);
            break;
        case 3:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,3,6);
            yCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,3) << 8) | SMEM_U32_GET_FIELD(logicalPortIndex,0,8);
            break;
        case 4:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,2,6);
            yCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,2) << 9) | SMEM_U32_GET_FIELD(logicalPortIndex,0,9);
            break;
        case 5:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,1,6);
            yCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,1) << 10) | SMEM_U32_GET_FIELD(logicalPortIndex,0,10);
            break;
        case 6:
            xCoordinate =  SMEM_U32_GET_FIELD(vidIndex,0,6);
            yCoordinate =  SMEM_U32_GET_FIELD(logicalPortIndex,0,11);
            break;
        case 7:
            xCoordinate =  (SMEM_U32_GET_FIELD(vidIndex,0,5) << 1) | SMEM_U32_GET_FIELD(logicalPortIndex,11,1);
            yCoordinate =   SMEM_U32_GET_FIELD(logicalPortIndex,0,10);
            break;
        default:/* should not happen */
            return;
    }

    /*
        <--           x, 2^6 bits                  -->
        -----------------------------------------------
        |                                             |    /\
        |                                             |     |
        |                  x,y                        |
        |                                             |    y,  2^11 bits
        |                                             |
        |                                             |      |
        |                                             |     \/
        -----------------------------------------------
    */

    index = yCoordinate;

    /*LP Egress VLAN member table*/
    regAddress = SMEM_XCAT_EQ_LOGICAL_PORT_EGRESS_VLAN_MEMBER_TBL_MEM(devObjPtr,index);
    regPtr = smemMemGet(devObjPtr, regAddress);
    fieldVal = snetFieldValueGet(regPtr, xCoordinate, 1);

    /* save the info for the logical port table parsing */
    descrPtr->vplsInfo.targetLogicalPortIsNotVlanMember = fieldVal ? 0 : 1;

    return;
}


/*******************************************************************************
*  snetChtEqNullPortCheck
*
* DESCRIPTION:
*        check if target port is 'NULL' port .
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*       portToCheck  - relevant only when != SMAIN_NOT_VALID_CNS
*                      relevant only for NON sip5.
*
* RETURN:
*       indication that target is NULL port.
*       GT_TRUE  - target port recognized as NULL port.
*       GT_FALSE - target port NOT recognized as NULL port.
*
*
*******************************************************************************/
static GT_BOOL snetChtEqNullPortCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32                   portToCheck
)
{
    GT_U32  targetPort;

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        return GT_FALSE;
    }

    if(descrPtr->useVidx == 1)
    {
        return GT_FALSE;
    }

    if(descrPtr->targetIsTrunk == 1)
    {
        return GT_FALSE;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
       if((descrPtr->eArchExtInfo.isTrgPhyPortValid == 0) &&
          (descrPtr->trgEPort != SNET_CHT_NULL_PORT_CNS))
        {
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("Target eport[0x%x] is not NULL port \n",
                descrPtr->trgEPort));

            return GT_FALSE;
        }

        targetPort = descrPtr->trgEPort;

        if(descrPtr->eArchExtInfo.isTrgPhyPortValid == 1)
        {
            if(descrPtr->eArchExtInfo.trgPhyPort != SNET_CHT_NULL_PORT_CNS)
            {
                __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("Target physical port[%d] is not NULL port \n",
                    descrPtr->eArchExtInfo.trgPhyPort));

                return GT_FALSE;
            }

            targetPort = descrPtr->eArchExtInfo.trgPhyPort;
        }
    }
    else /* non SIP5 */
    {
        targetPort = (portToCheck == SMAIN_NOT_VALID_CNS) ?
                    descrPtr->trgEPort :
                    portToCheck;

        if(targetPort != SNET_CHT_NULL_PORT_CNS)
        {
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("Target port[%d] is not NULL port \n",
                targetPort));
            return GT_FALSE;
        }
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK((
        "Target port[0x%x] is NULL port so do command resolution with SOFT drop \n",
        targetPort));

    /* do command resolution with SOFT drop */
    descrPtr->packetCmd =
        snetChtPktCmdResolution(descrPtr->packetCmd,
                                SKERNEL_EXT_PKT_CMD_SOFT_DROP_E);

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* No drop code is assigned when packet is dropped because of NULL port */
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("NOTE:no explicit drop code assigned for drops due to NULL port (keep previous) \n"));
        }
    }
    else
    {
        /* do not modify the cpuCode of 'MIRROR_TO_CPU' that became 'TO_CPU' */
    }

    return GT_TRUE;
}

/*******************************************************************************
* snetLion3GetLocStatus
*
* DESCRIPTION:
*           sip5: returns loc bit status for given ePort
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       direction    - direction ingress(RX)/egress(TX) - relevant to sip5_20
*       ePort        - ePort number
*
* OUTPUTS:
*
* RETURN:
*       isLocSet     - is loc bit set
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 snetLion3GetLocStatus
(
    IN SKERNEL_DEVICE_OBJECT   *devObjPtr,
    IN SMAIN_DIRECTION_ENT     direction,
    IN GT_U32                   ePort
)
{
    DECLARE_FUNC_NAME(snetLion3GetLocStatus);

    GT_U32    regAddr;
    GT_U32    locEntryIndex;
    GT_U32    isLocSet;
    GT_U32  numBits = SMEM_CHT_IS_SIP5_20_GET(devObjPtr) ? 13 : 11;

    __LOG_PARAM(ePort);

    /* get LOC entry index */
    regAddr = SMEM_LION3_EPORT_TO_LOC_MAPPING_TBL_MEM(devObjPtr, ePort);

    /* each entry hold info about 'locEntryIndex' for 2 eports in bits 0..10 , 11..21 */
    smemRegFldGet(devObjPtr, regAddr, numBits*(ePort & 1), numBits, &locEntryIndex);

    __LOG(("LOC entry index: [%d]\n", locEntryIndex));
    if((! SMEM_CHT_IS_SIP5_20_GET(devObjPtr)) || direction == SMAIN_DIRECTION_INGRESS_E)
    {
        regAddr = SMEM_LION2_OAM_PROTECTION_LOC_STATUS_TBL_MEM(devObjPtr, locEntryIndex);
        if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
        {
            __LOG(("access 'RX protection LOC table' index [0x%x] for eport[0x%x] \n",
                locEntryIndex,ePort));
        }
    }
    else
    {
        regAddr = SMEM_SIP5_20_EQ_OAM_PROTECTION_LOC_STATUS_TBL_MEM(devObjPtr, locEntryIndex);
        __LOG(("access 'TX protection LOC table' index [0x%x] for eport[0x%x] \n",
            locEntryIndex,ePort));
    }

    /* each entry hold info for 32 'isLocSet' */
    smemRegFldGet(devObjPtr, regAddr, (locEntryIndex%32), 1, &isLocSet);

    __LOG(("LOC bit: [%d]\n", isLocSet));

    return isLocSet;
}

/*******************************************************************************
* snetLion3RxProtectedSwitching
*
* DESCRIPTION:
*           Rx protected switching processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3RxProtectedSwitching
(
    IN    SKERNEL_DEVICE_OBJECT             *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   *descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion3RxProtectedSwitching);

    GT_U32                     regAddr;
    GT_U32                     fldVal,fldVal1;
    GT_U32                     isLocSet;
    GT_U32                     currentCpuCode;
    SKERNEL_EXT_PACKET_CMD_ENT currentPacketCmd;

    smemRegFldGet(devObjPtr, SMEM_LION3_EQ_EPORT_GLOBAL_CONFIG_REG(devObjPtr),1,1,&fldVal);

    if( ! fldVal)
    {
        __LOG(("Protection Switching globally disabled\n"));
        return;
    }

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        /* check mismatch in configuration - when the internal bit was not set */
        smemRegFldGet(devObjPtr, SMEM_LION3_EQ_EPORT_GLOBAL_CONFIG_REG(devObjPtr),2,1,&fldVal1);
        if( ! fldVal1)
        {
            __LOG(("Protection Switching Stage globally disabled on other flag \n"));
            return;
        }
    }

    if( ! descrPtr->rxEnableProtectionSwitching)
    {
        __LOG(("Rx Protection Switching disabled\n"));
        return;
    }

    isLocSet = snetLion3GetLocStatus(devObjPtr, SMAIN_DIRECTION_INGRESS_E ,descrPtr->origSrcEPortOrTrnk);

    if( (!isLocSet &&  descrPtr->rxIsProtectionPath) ||
        ( isLocSet && !descrPtr->rxIsProtectionPath) )
    {
        __LOG(("Assign the <Protection Switching Exception Packet Command>\n"));

        regAddr = SMEM_LION3_EQ_EPORT_PROTECTION_SWITCHING_RX_EXCEPTION_CONFIG_REG(devObjPtr);

        smemRegFldGet(devObjPtr, regAddr, 0, 8, &currentCpuCode);
        smemRegFldGet(devObjPtr, regAddr, 8, 3, &fldVal);
        currentPacketCmd = fldVal;

        /* Resolve packet command and CPU code */
        snetChtIngressCommandAndCpuCodeResolution(devObjPtr, descrPtr,
                                                  descrPtr->packetCmd,
                                                  currentPacketCmd,
                                                  descrPtr->cpuCode,
                                                  currentCpuCode,
                                                  SNET_CHEETAH_ENGINE_UNIT_EQ_E,
                                                  GT_FALSE);
    }
}

/*******************************************************************************
* snetLion3TxProtectedSwitchingOneToOneArch
*
* DESCRIPTION:
*           Tx protected switching processing: 1:1 architecture
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to frame data
*
* OUTPUTS:
*       descrPtr     - pointer to updated frame data
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3TxProtectedSwitchingOneToOneArch
(
    IN    SKERNEL_DEVICE_OBJECT             *devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   *descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion3TxProtectedSwitchingOneToOneArch);

    GT_U32    regAddr;
    GT_U32    fldVal, fldVal1;

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CPU_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
    {
        __LOG(("Tx Protected Switching not accessed due to descrPtr->packetCmd[%d] \n",
            descrPtr->packetCmd));
        return;
    }

    if(SKERNEL_IS_MATCH_DEVICES_MAC(descrPtr->trgDev, descrPtr->ownDev,
                                    devObjPtr->dualDeviceIdEnable.eq) == 0)
    {
        __LOG(("Tx Protected Switching not accessed due to not 'own' device \n"));
        return;
    }

    if ((descrPtr->useVidx) == 1 ||
       (descrPtr->targetIsTrunk) == 1 ||
       (descrPtr->eArchExtInfo.isTrgPhyPortValid) == 1)
    {
        __LOG(("Tx Protected Switching not accessed due to useVidx[%d],isTrunk[%d], isTrgPhyPortValid[%d]\n",
            (descrPtr->useVidx),
            (descrPtr->targetIsTrunk),
            (descrPtr->eArchExtInfo.isTrgPhyPortValid)
            ));
        return;
    }


    smemRegFldGet(devObjPtr, SMEM_LION3_EQ_EPORT_GLOBAL_CONFIG_REG(devObjPtr),1,1,&fldVal);

    if( ! fldVal)
    {
        __LOG(("Protection Switching globally disabled\n"));
        return;
    }

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        /* check mismatch in configuration - when the internal bit was not set */
        smemRegFldGet(devObjPtr, SMEM_LION3_EQ_EPORT_GLOBAL_CONFIG_REG(devObjPtr),2,1,&fldVal1);
        if( ! fldVal1)
        {
            __LOG(("Protection Switching Stage globally disabled on other flag \n"));
            return;
        }
    }


    regAddr = SMEM_LION3_TX_PROTECTION_SWITCHING_TBL_MEM(devObjPtr, descrPtr->trgEPort);
    smemRegFldGet(devObjPtr, regAddr, descrPtr->trgEPort % 32, 1, &fldVal);

    if( ! fldVal)
    {
        __LOG(("Tx Protection Switching disabled for target port: [%d]\n", descrPtr->trgEPort));
        return;
    }

    if( snetLion3GetLocStatus(devObjPtr, SMAIN_DIRECTION_EGRESS_E ,descrPtr->trgEPort) )
    {
        __LOG(("the target ePort LSB is toggled, and the packet is sent over the protection path\n"));
        descrPtr->trgEPort ^= 1;

        __LOG_PARAM(descrPtr->trgEPort);
    }
}


/*******************************************************************************
*  snetChtEq
*
* DESCRIPTION:
*        EQ - block processing
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetChtEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEq);

    GT_BOOL   doStc;                /* sampling to CPU */
    GT_U32    regAddress;           /* register address */
    GT_U32 qosProfileEntry;         /* QOS Profile entry */

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT_EQ_E);

    /* EQ - Ingress ePort Table */
    __LOG(("start EQ \n"));

    SIM_LOG_PACKET_DESCR_SAVE

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FROM_CPU_E)
    {
        regAddress = SMEM_CHT_QOS_TBL_MEM(devObjPtr, descrPtr->qos.qosProfile);
        /* QoSProfile to QoS table Entry */
        smemRegGet(devObjPtr, regAddress, &qosProfileEntry);

        /* fix of EQ-491 : QoS-Profile <UP> is not delivered to the egress pipe */
        /* this value will be used by the SIP5_10 device .. in the HA for tunnel start */
        /* the UP from qos profile entry */
        descrPtr->qosMappedUp = SMEM_U32_GET_FIELD(qosProfileEntry, 6, 3);
        /* the DSCP from qos profile entry */
        descrPtr->qosMappedDscp = SMEM_U32_GET_FIELD(qosProfileEntry, 0, 6);
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.eq)
    {
        /* EQ - Ingress ePort Table */
        descrPtr->eArchExtInfo.eqInfo.eqIngressEPort = descrPtr->eArchExtInfo.localDevSrcEPort;
        regAddress = SMEM_LION2_EQ_INGRESS_EPORT_TBL_MEM(devObjPtr,
            descrPtr->eArchExtInfo.eqInfo.eqIngressEPort);

        descrPtr->eArchExtInfo.eqIngressEPortTablePtr = smemMemGet(devObjPtr, regAddress);
    }

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        /* this field used for all devices (not only eArch) */
        descrPtr->eArchExtInfo.eqInfo.IN_descDrop = 1;
    }
    else
    {
        descrPtr->eArchExtInfo.eqInfo.IN_descDrop = 0;
    }

    if(descrPtr->eArchExtInfo.eqInfo.IN_descDrop &&
       descrPtr->marvellTagged &&
       descrPtr->incomingMtagCmd == SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E)
    {
        __LOG(("packet came with DSA tag to analyzer ... but was dropped during the ingress pipe \n"
           " , we can not let it got to analyzer \n"));
        descrPtr->rxSniff = 0;
        descrPtr->analyzerIndex = 0;
    }


    /* for simulation do NULL port check here (before any EQ modifications),
       and we will need it too after trunk and analyzer port

       Call it after setting of descrPtr->eArchExtInfo.eqInfo.IN_descDrop
    */
    snetChtEqNullPortCheck(devObjPtr,descrPtr,SMAIN_NOT_VALID_CNS);

    if (devObjPtr->supportLogicalTargetMapping)
    {
        /*The mechanism is used for egress VLAN filtering in a logical port level,
            unlike the TXQ egress-filtering which is done on a physical port level*/
        eqEgressVlanFilteringForLogicalPorts(devObjPtr, descrPtr);
    }


    if(devObjPtr->supportMultiAnalyzerMirroring)
    {
        /* finalize the <analyzerIndex> and <rxSniff> issues for ingress mirroring */
        __LOG(("finalize the <analyzerIndex> and <rxSniff> issues for ingress mirroring"));
        eqIngressMirrorAnalyzer(devObjPtr, descrPtr);
    }

    if(devObjPtr->supportL2Ecmp &&
       devObjPtr->supportEArch &&
       devObjPtr->unitEArchEnable.eq)
    {
        /*
        Equal Cost Multi-Path (ECMP) enables traffic to be load balanced among several paths. The L2
        ECMP block provides ePort-based load balancing functionality.
        .. Primary ePort - this ePort number represents the ECMP group, i.e., the primary port represents
        a group of ports that lead to a common destination. Traffic that is sent to the primary port is load
        balanced among the ePorts in this group.
        .. Secondary ePorts - the ePorts that are represented by the primary ePort are called secondary
        ePorts.
        Traffic that is sent to an L2 ECMP group typically undergoes the following walk-through:
        1. The ingress pipe assigns a target primary ePort.
        2. The packet arrives to the L2 ECMP block in the EQ. The L2 ECMP block maps the primary
            ePort to one of its secondary ePorts, by using a load balancing hash.
        3. The secondary ePort is used on the target device to access the E2PHY table.
        */
        eqL2EcmpSubUnit(devObjPtr, descrPtr);
    }

    if(devObjPtr->supportEArch &&
       devObjPtr->unitEArchEnable.eq)
    {
        /* Rx Protected Switching processing*/
        snetLion3RxProtectedSwitching(devObjPtr, descrPtr);

        /* Tx Protected Switching - 1:1 architecture processing */
        snetLion3TxProtectedSwitchingOneToOneArch(devObjPtr, descrPtr);

        /* The ePort to Physical port mapping table (E2PHY for short) maps a target
            ePort to a physical egress interface.
           The egress interface consists of a physical target port, VIDX, or trunk.
           It may also include a tunnel pointer, or a pointer to an ARP entry.
        */
        eqE2PhySubUnit(devObjPtr, descrPtr, GT_FALSE);
    }


    /* Multi-Port Group FDB Lookup support */
    if(devObjPtr->supportMultiPortGroupFdbLookUpMode)
    {
        /* Redirect packet to the ring port in the next port group */
        __LOG(("Check if need to Redirect packet to the ring port in the next port group \n"));
        snetLionEqInterPortGroupRingFrwrd(devObjPtr, descrPtr);
    }

    /* Check Sampling to CPU */
    __LOG(("Check Sampling to CPU \n"));
    doStc = snetChtEqStc(devObjPtr, descrPtr);
    __LOG(("Sampling to CPU %s \n" ,
        (doStc == GT_TRUE) ? "Enabled" : "Disabled"));

    /* Drop packet command */
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
       /* CNC Trigger : count the drop */
        __LOG(("CNC : check if this DROP of EQ should be counted \n"));
        snetChtEqCncCount(devObjPtr, descrPtr);

        if(doStc == GT_FALSE)
        {
            /* If a packets copy was NOT mirrored to CPU by the STC mechanism and
            the original packet was dropped in ingress, the packet is counted in this counter */
            snetChtEqIngressDropCount(devObjPtr, descrPtr);
        }

        if (doStc == GT_FALSE && descrPtr->rxSniff == 0)
        {
            __LOG(("packet was dropped without STC and without rxSniff \n"));

            __LOG(("EQ - ended \n"));

            return;
        }

        __LOG(("Start check STC and mirror to analyzer for soft/hard drop \n"));

        /* To enable sampling to CPU and mirroring to analyzer port */
        SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION("snetChtEq - soft/hard drop");

        descrPtr->packetCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;

        if (doStc == GT_TRUE)
        {
            /* Forward STC in the original descriptor */
            __LOG(("Forward STC in the original descriptor"));
            snetChtEqDoStc(devObjPtr, descrPtr);
        }

        if (descrPtr->rxSniff)
        {
            if(doStc == GT_TRUE)
            {
                /* Send to Rx Sniffer in the duplicated descriptor */
                __LOG(("Send to Rx Sniffer in the duplicated descriptor"));
                snetChtEqDuplicateRxSniff(devObjPtr, descrPtr);
            }
            else
            {
                /* Forward Rx sniffed packet in the original descriptor */
                __LOG(("Forward Rx sniffed packet in the original descriptor"));
                snetChtEqDoRxSniff(devObjPtr, descrPtr, GT_TRUE);
            }
        }

        SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION("snetChtEq : snetChtEq - soft/hard drop");

        __LOG(("EQ - ended \n"));
        return;
    }

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEq : part 1 - ");

    SIM_LOG_PACKET_DESCR_SAVE

    if(devObjPtr->supportTunnelInterface == GT_TRUE)
    {
        /* The Pre-Egress stage, at the end of the ingress pipeline processing ,
           is responsible for mapping the packet target virtual interface to a physical
           interface. */
        snetChtVirtualPortMapping(devObjPtr, descrPtr);
    }

    if (devObjPtr->supportLogicalTargetMapping)
    {
        /* Logical Target Mapping */
        __LOG(("Logical Target Mapping"));
        snetXCatLogicalTargetMapping(devObjPtr, descrPtr);
    }

    if (descrPtr->useVidx == 0 && descrPtr->targetIsTrunk)
    {
        /* convert target trunk to target port */
        trunkMemberGet(devObjPtr,descrPtr,
                        descrPtr->trgTrunkId);
    }

    /* for simulation do NULL port check here (after trunk member selection again),
       and we will need it too after analyzer port */
    snetChtEqNullPortCheck(devObjPtr,descrPtr,SMAIN_NOT_VALID_CNS);

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEq : part 2 - ");

    /* Forward packet command */
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        /* Forward packet in the original descriptor */
        __LOG(("Forward packet [%s] in the original descriptor",
            descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E ?
                "FORWARD" : "MIRROR_TO_CPU (the forward part)"
                ));
        snetChtEqDoFrwd(devObjPtr, descrPtr);
    }
    else if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FROM_CPU_E)
    {
        /* handle packet in the original descriptor */
        __LOG(("Process packet 'FROM_CPU' (in the original descriptor) \n"));
        snetChtEqDoFromCpu(devObjPtr, descrPtr);
    }

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        process_EQ_DROP_lbl:
        /* CNC : client type */

       /* CNC Trigger : count the drop */
        __LOG(("CNC : check if this DROP of EQ should be counted \n"));
        snetChtEqCncCount(devObjPtr, descrPtr);

        /* the function snetChtEqDoFrwd , changed the command of the frame */
        __LOG(("packet command is [%s] DROP  , no more processing \n",
            descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E ? "HARD" : "SOFT"));

        __LOG(("EQ - ended \n"));
        return ;
    }
    else
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        /* Send to CPU in the duplicated descriptor */
        __LOG(("Send to CPU the 'MIRROR_TO_CPU' copy (in duplicated descriptor) \n"));
        snetChtEqDuplicateToCpu(devObjPtr, descrPtr);
    }
    else
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TRAP_TO_CPU_E)
    {
        /* Send to CPU  in the original descriptor */
        __LOG(("Send to CPU the 'TRAP_TO_CPU' (in original descriptor) \n"));
        snetChtEqDoToCpu(devObjPtr, descrPtr);
    }
    else
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
    {
        /* Send to sniffer in the original descriptor */
        __LOG(("Send to sniffer the 'TO_TRG_SNIFFER' (in original descriptor) \n"));
        snetChtEqDoTrgSniff(devObjPtr, descrPtr);

        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
        {
            /* packet assigned HARD DROP and was not sent to analyzer */
            goto process_EQ_DROP_lbl;
        }
    }
    else
    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FROM_CSCD_TO_CPU_E)
    {
        /* Send to CPU  in the original descriptor */
        __LOG(("Send to CPU the 'FROM_CSCD_TO_CPU' (in the original descriptor) \n"));
        snetChtEqDoToCpu(devObjPtr, descrPtr);
    }

    /* Do sampling to CPU */
    if (doStc == GT_TRUE)
    {
        /* Do STC in the duplicated descriptor */
        __LOG(("Do sampling to CPU:Do STC (in duplicated descriptor) "));
        snetChtEqDuplicateStc(devObjPtr, descrPtr);
    }

    /* Do rx sniffing */
    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E && /* when packet is from DSA to sniffer .. we already sent it to the analyzer */
       descrPtr->rxSniff)
    {
        /* Send to Rx Sniffer in the duplicated descriptor */
        __LOG(("Do Rx sniffing (in duplicated descriptor)"));
        snetChtEqDuplicateRxSniff(devObjPtr, descrPtr);
    }

    __LOG(("EQ - ended \n"));
    return;
}


/*******************************************************************************
*  snetChtEqStc
*
* DESCRIPTION:
*        Ingress Sampling To CPU
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
static GT_BOOL snetChtEqStc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqStc);

    GT_U32 ingrStcCntrlRegData;     /* Ingress STC Configuration Register */
    GT_U32 ingressSTCCnt,newIngressSTCCnt; /* Countdown port counter */
    GT_BOOL  doStc = GT_FALSE;      /* Do ingress sampling to CPU */
    GT_U32 fieldVal;                /* Register field value */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 * regPtr;                /* Register entry pointer */
    GT_U32 port;                    /* Ingress port */
    GT_U32 bitIndex;                /* bit index of the interrupt */
    GT_U32  newLimValRdy;           /*Port<n>Ingress STC New Limit Value Ready*/
    GT_U32  reloadMode;             /*reload mode*/
    GT_BOOL loadNewValue = GT_FALSE;/*load new value */
    GT_U32  ingressSTCLimit;        /* the limit written in the entry */
    GT_BIT  supportImmidiateLoad ;/* new limit is loaded immediately into Port<n>IngressSTCCnt ,
                                    as opposed to the legacy behavior where the new limit is
                                    only loaded when the counter reaches 0. */
    GT_BIT  supportInterrupt;/* indication that support interrupt. sip5 not do interrupt in continues mode */

    supportImmidiateLoad = SMEM_CHT_IS_SIP5_GET(devObjPtr) ? 1 : 0;

    port = descrPtr->localDevSrcPort;

    /* Ingress STC Configuration Register */
    smemRegGet(devObjPtr, SMEM_CHT_INGRESS_STC_CONF_REG(devObjPtr), &ingrStcCntrlRegData);

    /* Ingress STC Enable */
    fieldVal = SMEM_U32_GET_FIELD(ingrStcCntrlRegData, 2, 1);
    if (fieldVal == 0)
    {
        __LOG(("ingress STC globally disabled \n"));
        return doStc;
    }

    if(0 == SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(port == SNET_CHT_CPU_PORT_CNS)
        {
            __LOG(("ingress STC not relevant to CPU port (only to physical ports) \n"));
            return doStc;
        }
    }

    if (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E ||
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E)
    {
        /* Ingress STC Count Mode :
            NOTE: called 'packet mode' but it is actually ' sampling mode' :
            0 - All packets received without any MAC-level error are subject to ingress STC.
            1 - Only non-dropped packets are subject to ingress STC.

            */
        fieldVal = SMEM_U32_GET_FIELD(ingrStcCntrlRegData, 0, 1);
        if (fieldVal)
        {
            __LOG(("Ingress STC Count Mode 'only Non-dropped' --> ignore packets that were assigned soft/hard drop \n"));
            return doStc;
        }
        else
        {
            __LOG(("Ingress STC Count Mode 'All' -->  Allow packets that were assigned soft/hard drop \n"));
        }
    }

    /* Port<n>Ingress STC Table Entry Address */
    regAddr = SMEM_CHT_INGR_STC_TBL_MEM(devObjPtr, port);
    regPtr = smemMemGet(devObjPtr, regAddr);

    ingressSTCLimit = snetFieldValueGet(regPtr, 0, 30);
    __LOG_PARAM(ingressSTCLimit);
    /* simulate PP behavior - no stc when IngressSTCLimit == 1 (not according the functional spec) */
    if(ingressSTCLimit == 1)
    {
        __LOG(("when limit is 1 --> behave as 'STC disabled' on port [%d] \n",
            port));
        return doStc;
    }

    /* Ingress STC Reload Mode */
    reloadMode = SMEM_U32_GET_FIELD(ingrStcCntrlRegData, 1, 1);

    supportInterrupt = (SMEM_CHT_IS_SIP5_GET(devObjPtr) && (reloadMode == 0)) ? 0 : 1;


    /* Port<n>Ingress STC New Limit Value Ready */
    newLimValRdy = snetFieldValueGet(regPtr, 30, 1);
    __LOG_PARAM(newLimValRdy);

    if(supportImmidiateLoad && newLimValRdy)
    {
        /* new limit is loaded immediately into Port<n>IngressSTCCnt , as opposed
           to the legacy behavior where the new limit is only loaded when
           the counter reaches 0.
        */

        /* use the limit value --> 'loaded immediately' */
        __LOG(("'new' ingressSTCLimit [0x%8.8x] is loaded immediately into ingressSTCCnt (not wait for counter reaches 0) \n",
            ingressSTCLimit));
        loadNewValue = GT_TRUE;
        ingressSTCCnt = ingressSTCLimit;/* initialized only so no compiler warning */
    }
    else
    {
        /* Port<n>Ingress STC Counter */
        ingressSTCCnt = snetFieldValueGet(regPtr, 32, 30);
        __LOG_PARAM(ingressSTCCnt);

        if (ingressSTCCnt)
        {
            if ((--ingressSTCCnt) == 0)
            {
                /* in this case we not reload new value ! */
                __LOG(("The ingressSTCCnt decremented to 0 (but only next packet will 'reload' it's value) \n"));
                doStc = GT_TRUE;
            }
        }
        else /*ingressSTCCnt == 0*/
        {
            if(newLimValRdy)
            {
                __LOG(("The ingressSTCCnt is 0 and newLimValRdy = 1 --> load new limit \n"));

                /* about : <newLimValRdy> : when the countdown counter at 0 , but the <newLimValRdy> is 1 ,
                   this is the time to load the new counter from word0 into word1 */
                loadNewValue = GT_TRUE;
            }
            else if(reloadMode == 0)/*continues*/
            {
                __LOG(("The ingressSTCCnt is 0 and in continue mode ignore that newLimValRdy = 0 --> load new limit \n"));
                loadNewValue = GT_TRUE;
            }
            else
            {
                __LOG(("The ingressSTCCnt is 0 and in trigger mode and newLimValRdy = 0 --> NOT load new limit \n"));
            }
        }
    }

    if(loadNewValue == GT_TRUE)
    {
        /* get the new count Port<n>Ingress STC Limit */
        newIngressSTCCnt = ingressSTCLimit;
        if(newIngressSTCCnt)
        {
            /* the STC on the port was disabled */
            /* the device load the new value and also decrement (due to current packet)*/
            newIngressSTCCnt--;

            if(supportInterrupt == 0)
            {
                /* SIP5 device do not generate interrupts in continuous mode */
                __LOG(("Load new limit but without generate interrupt \n"));
            }
            else
            if(devObjPtr->myInterruptsDbPtr) /* code for BC2 */
            {
                bitIndex = (port & 0xF) + 1;
                snetChetahDoInterrupt(devObjPtr,
                                      SMEM_CHT_INGRESS_STC_INT_CAUSE_REG(devObjPtr,port),
                                      SMEM_CHT_INGRESS_STC_INT_MASK_REG(devObjPtr,port),
                                      (1 << bitIndex),/* bmp of the bitIndex in SMEM_CHT_INGRESS_STC_INT_CAUSE_REG() */
                                      SMEM_CHT_PRE_EGR_ENGINE_INT(devObjPtr));
            }
            else
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))/* dummy code for Lion3 ! (not BC2) */
                {
                    port &= 0xF; /* the CPSS still not supports interrupts properly - so limit all interrupts to single register 0 */

                    /* Interrupt Cause summary register STC bit */
                    smemRegFldSet(devObjPtr, SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_CAUSE_REG(devObjPtr), (port / 16) + 1, 1, 1);
                    smemRegFldGet(devObjPtr, SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_MASK_REG(devObjPtr), (port / 16) + 1, 1, &fieldVal);
                    if(fieldVal)
                    {
                        /* Sum of all STC SUMMARY interrupts */
                        __LOG(("Sum of all STC SUMMARY interrupts \n"));
                        smemRegFldSet(devObjPtr, SMEM_LION3_EQ_INGRESS_STC_INT_SUMMARY_CAUSE_REG(devObjPtr), 0, 1, 1);
                    }

                    bitIndex = (port & 0xF) + 1;
                }
                else
                {
                    bitIndex = port + 1;
                }

                if(SKERNEL_DEVICE_FAMILY_CHEETAH_1_ONLY(devObjPtr))
                {
                    /* Generate interrupt in EQ register */
                    __LOG(("Generate interrupt in EQ register \n"));
                    snetChetahDoInterruptLimited(devObjPtr,
                                          SMEM_CHT_INGRESS_STC_INT_CAUSE_REG(devObjPtr, port),
                                          SMEM_CHT_INGRESS_STC_INT_MASK_REG(devObjPtr, port),
                                          (1 << bitIndex),
                                          SMEM_CHT_PRE_EGR_ENGINE_INT(devObjPtr),
                                          1);
                }
                else
                {
                    /* Interrupt Cause register STC bit */
                    smemRegFldSet(devObjPtr, SMEM_CHT_INGRESS_STC_INT_CAUSE_REG(devObjPtr,port), bitIndex, 1, 1);
                    /* Get summary bit */
                    smemRegFldGet(devObjPtr, SMEM_CHT_INGRESS_STC_INT_MASK_REG(devObjPtr,port), bitIndex, 1, &fieldVal);
                    if(fieldVal)
                    {
                        /* Sum of all STC interrupts */
                        __LOG(("Sum of all STC interrupts \n"));
                        smemRegFldSet(devObjPtr, SMEM_CHT_INGRESS_STC_INT_CAUSE_REG(devObjPtr,port), 0, 1, 1);
                    }
                    /* Generate interrupt in EQ register */
                    __LOG(("Generate interrupt in EQ register \n"));
                    snetChetahDoInterruptLimited(devObjPtr,
                                          SMEM_CHT_EQ_INT_CAUSE_REG(devObjPtr),
                                          SMEM_CHT_EQ_INT_MASK_REG(devObjPtr),
                                          SMEM_CHT_ING_STC_SUM_INT(devObjPtr),
                                          SMEM_CHT_PRE_EGR_ENGINE_INT(devObjPtr),
                                          fieldVal);
                }
            }
        }

        /* since we reload new limit -->
           need to update IngressSTCNewLimValRdy regardless to trigger/continuous mode */
        snetFieldValueSet(regPtr, 30, 1, 0);
    }
    else
    {
        newIngressSTCCnt = ingressSTCCnt;
    }

    /* Set Port<n>Ingress STC Counter */
    snetFieldValueSet(regPtr, 32, 30, newIngressSTCCnt);

    smemMemSet(devObjPtr, regAddr, regPtr, 3);

    __LOG((" final ingress STC decision : %s DO STC \n",
           doStc ? "" : "NOT"));

    return doStc;
}

/*******************************************************************************
*  snetChtEqDoStc
*
* DESCRIPTION:
*        Forward STC in the original descriptor
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqDoStc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDoStc);

    GT_U32          stcSampleCnt;  /*This counter counts the number of packets sampled to the CPU.*/
    GT_U32          * regPtr;      /* Register entry pointer */
    GT_U32          regAddr;       /* register's address */

    descrPtr->cpuCode = SNET_CHT_INGRESS_SAMPLED;

    regAddr = SMEM_CHT_INGR_STC_TBL_MEM(devObjPtr, descrPtr->localDevSrcPort);

    /*get pointer to the actual table memory - direct access*/
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Port<n>STCSampledPktCntr */
    stcSampleCnt = SMEM_U32_GET_FIELD(regPtr[2], 0, 16);

    /* increment counter*/
    stcSampleCnt++;

    /* write to the actual table memory - direct access */
    SMEM_U32_SET_FIELD(regPtr[2], 0, 16, stcSampleCnt);

    /* Send to CPU */
    __LOG(("Send to CPU"));
    snetChtEqDoToCpu(devObjPtr, descrPtr);
}

/*******************************************************************************
*  snetChtEqDuplicateRxSniff
*
* DESCRIPTION:
*        Send frame to the Rx Sniffer by the duplicated descriptor
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqDuplicateRxSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDuplicateRxSniff);

    GT_BOOL passStatSniff;                          /* packet may be send to
                                                       Rx Sniffer */
    SKERNEL_FRAME_CHEETAH_DESCR_STC * newDescrPtr;  /* pointer to descriptor */

    /* Check statistical sniffing */
    __LOG(("Check statistical sniffing"));
    passStatSniff  = snetChtEqStatSniff(devObjPtr, descrPtr);
    if (passStatSniff == GT_FALSE)
    {
        return;
    }

    /* Get pointer to the duplicated descriptor */
    newDescrPtr = snetChtEqDuplicateDescr(devObjPtr, descrPtr);
    /* Send to Rx Sniffer with new descriptor */
    __LOG(("Send to Rx Sniffer with new descriptor"));
    snetChtEqDoRxSniff(devObjPtr, newDescrPtr, GT_FALSE);
}

/*******************************************************************************
*  snetChtEqStatSniff
*
* DESCRIPTION:
*        Rx  Statistical Sniffer
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*       GT_TRUE     - packet may be send to Rx Sniffer
*       GT_FALSE    - packet could not be send to Rx Sniffer
*******************************************************************************/
static GT_BOOL snetChtEqStatSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqStatSniff);

    GT_U32 statSnifCfgRegData;      /* Statistic Sniffing Configuration Register
                                       data */
    GT_U32 statRatio;               /* Statistic mirroring to analyzer port
                                       statistical ratio configuration */
    GT_BOOL passStatSniff = GT_TRUE; /* Pass statistic sniffing */

    GT_U32 fieldVal;                /* Register field value */

    GT_U32  * rxSniffPtr;

    rxSniffPtr = CHT_INTERNAL_MEM_PTR(devObjPtr,CHT_INTERNAL_SIMULATION_USE_MEM_RX_STATISTICAL_SNIFF_COUNTER_E);

    smemRegGet(devObjPtr, SMEM_CHT_STAT_SNIF_CONF_REG(devObjPtr), &statSnifCfgRegData);

    /* Ingress Statistic Mirroring To Analyzer Port Enable */
    fieldVal = SMEM_U32_GET_FIELD(statSnifCfgRegData, 11, 1);
    if (fieldVal == 0)
    {
        /* If disabled, every packet should be mirrored to ingress analyzer */
        __LOG(("If disabled, every packet should be mirrored to ingress analyzer"));
        return GT_TRUE;
    }

    /* Ingress Statistic Mirroring To Analyzer Port Ratio */
    statRatio = SMEM_U32_GET_FIELD(statSnifCfgRegData, 0, 11);

    (*rxSniffPtr)++;

    if (statRatio)
    {
        if ((*rxSniffPtr) % statRatio)
        {
            /* no rx sniff */
            passStatSniff = GT_FALSE;
        }
    }
    else
    {
        passStatSniff = GT_FALSE;
    }

    return passStatSniff;
}

/*******************************************************************************
*  snetChtEqDuplicateDescr
*
* DESCRIPTION:
*        Duplicate Cheetah's descriptor
*        do fatal error if no available descriptor !
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*       SKERNEL_FRAME_CHEETAH_DESCR_STC *
*                   - pointer to the duplicated descriptor of the Cheetah
*
*******************************************************************************/
SKERNEL_FRAME_CHEETAH_DESCR_STC * snetChtEqDuplicateDescr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDuplicateDescr);

    SKERNEL_FRAME_CHEETAH_DESCR_STC *  newDescrPtr = NULL;


    if(descrPtr == NULL)
    {
        devObjPtr->descrFreeIndx = 0;
    }

    while(1)
    {
        if (devObjPtr->descrFreeIndx >= devObjPtr->descrNumber)
        {
            /* not enough descriptors ?! */
            skernelFatalError("snetChtEqDuplicateDescr: not enough descriptors ?! \n");
        }

        /* Get pointer to the free descriptor */
        newDescrPtr = ((SKERNEL_FRAME_CHEETAH_DESCR_STC*)devObjPtr->descriptorPtr)
                        + devObjPtr->descrFreeIndx;

        /* Increment free pointer and copy old descriptor to the new one */
        devObjPtr->descrFreeIndx++;

        if(newDescrPtr->numberOfSubscribers == 0)
        {
            /*found descriptor that is not subscribed to (or used by) other operations */
            break;
        }
    }

    if(descrPtr == NULL)
    {
        __LOG(("Create new descriptor \n"));
        memset(newDescrPtr, 0, sizeof(SKERNEL_FRAME_CHEETAH_DESCR_STC));

        /* protect the global resource 'globalFrameId' */
        SCIB_SEM_TAKE;
        newDescrPtr->frameId = globalFrameId++;
        SCIB_SEM_SIGNAL;
    }
    else
    { /* descrPtr != NULL */
        __LOG(("Duplicate given descriptor \n"));
        memcpy(newDescrPtr, descrPtr, sizeof(SKERNEL_FRAME_CHEETAH_DESCR_STC));

        /* support duplication of inner descriptor */
        if(descrPtr->ingressTunnelInfo.innerFrameDescrPtr)
        {
            __LOG(("Duplicate descriptor for the inner descriptor \n"));

            newDescrPtr->ingressTunnelInfo.innerFrameDescrPtr = /*the duplication*/
                snetChtEqDuplicateDescr(devObjPtr,
                    descrPtr->ingressTunnelInfo.innerFrameDescrPtr);/*the original inner descriptor*/
        }
    }

    return newDescrPtr;
}

/*******************************************************************************
*  snetChtEqSniffInfoGet
*
* DESCRIPTION:
*        get sniffer info
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       rxSniff     - rx/tx sniff :
*                   GT_TRUE - rx
*                   GT_FALSE - tx
*                   NOTE: the rx sniff supports trunk , but the tx not !
* OUTPUTS:
*       monitorTargetDevicePtr - pointer to the target device number for rx/tx monitoring
*       monitorTargetPortPtr   - pointer to the target port   number for rx/tx monitoring
* RETURN:
*       GT_TRUE - the packet can to be mirrored
*       GT_FALSE - the packet must not be mirrored
* COMMENT :
*
*
*******************************************************************************/
static GT_BOOL snetChtEqSniffInfoGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL  rxSniff,
    OUT GT_U32  *monitorTargetDevicePtr,
    OUT GT_U32  *monitorTargetPortPtr

)
{
    DECLARE_FUNC_NAME(snetChtEqSniffInfoGet);

    GT_U32 sniffDev, sniffPort;         /* sniffer device and port */
    GT_U32 regAddr;                     /* Register address */
    GT_U32 analyzCfgRegData;            /* Analyzer Port Configuration Register */
    GT_BIT  mirrorOnDrop;/* mirror on drop */
    GT_BOOL isNullPort;/* is the target analyzer port is NULL port */
    GT_U32  portToCheckNullPort;

    /*  Selection of one of the seven analyzers on a per-port base */
    if(devObjPtr->supportMultiAnalyzerMirroring)
    {
        regAddr = SMEM_XCAT_MIRROR_INTERFACE_PARAM_REG(devObjPtr,
                                                      (descrPtr->analyzerIndex - 1));
        smemRegGet(devObjPtr, regAddr, &analyzCfgRegData);

        if(((rxSniff == GT_TRUE) && descrPtr->eArchExtInfo.eqInfo.IN_descDrop) ||
            ((rxSniff == GT_FALSE) &&
             (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E ||
              descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E)))
        {
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                mirrorOnDrop = SMEM_U32_GET_FIELD(analyzCfgRegData,25 ,1);
            }
            else
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                mirrorOnDrop = SMEM_U32_GET_FIELD(analyzCfgRegData,23 ,1);
            }
            else
            {
                mirrorOnDrop = SMEM_U32_GET_FIELD(analyzCfgRegData,20 ,1);
            }

            /*Mirror On Drop */
            if(0 == mirrorOnDrop)
            {
                /* don't mirror the packet */
                __LOG(("Don't mirror the packet : because packet is dropped \n"));
                return GT_FALSE;
            }
        }
    }
    else
    {
        /* Get analyzCfgRegData from Analyzer Port Configuration Register */
        smemRegGet(devObjPtr, SMEM_CHT_ANALYZER_PORT_CONF_REG(devObjPtr), &analyzCfgRegData);
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            /* Ingress Analyzer Port */
            sniffPort = SMEM_U32_GET_FIELD(analyzCfgRegData,10,15);
        }
        else
        {
            /* Ingress Analyzer Port */
            sniffPort = SMEM_U32_GET_FIELD(analyzCfgRegData,10,13);
        }
        /* Ingress Analyzer Device */
        sniffDev  = SMEM_U32_GET_FIELD(analyzCfgRegData, 0,10);
    }
    else
    {
        /* Ingress Analyzer Port */
        sniffPort = SNET_CHT_INGR_ANALYZER_TARGET_PORT_GET_MAC(devObjPtr,
                                                                 analyzCfgRegData);
        /* Ingress Analyzer Device */
        sniffDev = SNET_CHT_INGR_ANALYZER_TARGET_DEV_GET_MAC(devObjPtr,
                                                               analyzCfgRegData);
    }

    __LOG(("%s Analyzer : device[%d] port[%d]",
            rxSniff == GT_TRUE ? "Ingress" : "Egress" ,
            sniffDev,
            sniffPort));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr) &&
            rxSniff == GT_FALSE)
        {
            /* save the ORIGINAL target info */
            if(descrPtr->useVidx)
            {
                descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->egressTrgPort;
            }
            else
            {
                descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->trgEPort;
            }

            descrPtr->srcTrgPhysicalPort = descrPtr->egressTrgPort;
        }

        descrPtr->useVidx = 0;
        descrPtr->targetIsTrunk = 0;
        descrPtr->trgEPort = sniffPort;
        descrPtr->eArchExtInfo.isTrgPhyPortValid = 0;
        descrPtr->trgDev = sniffDev;

        if(descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E &&
           descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
        {
            __LOG(("e2phy already accessed at start of EQ unit \n"));
        }
        else
        {
            isNullPort = snetChtEqNullPortCheck(devObjPtr,descrPtr,sniffPort);
            if(isNullPort == GT_TRUE)
            {
                __LOG(("WARNING : the target EPORT of the Analyzer is 'NULL port'(%d) --> so IGNORE request to sent to analyzer index [%d] \n",
                    SNET_CHT_NULL_PORT_CNS,
                    descrPtr->analyzerIndex));

                return GT_FALSE;
            }

            __LOG(("Access the E2PHY for the analyzer port \n"));
            eqE2PhySubUnit(devObjPtr, descrPtr, GT_TRUE);
        }

        if(descrPtr->useVidx)
        {
            descrPtr->trgDev = descrPtr->ownDev;
            descrPtr->eArchExtInfo.vidx = descrPtr->eVidx;
        }
        else if (descrPtr->targetIsTrunk)
        {
            /* convert target trunk to target port */
            trunkMemberGet(devObjPtr, descrPtr, 0/* not used */);
        }

        /*update the info */
        sniffPort = descrPtr->trgEPort;
        sniffDev  = descrPtr->trgDev;

        __LOG(("after E2PHY for the %s Analyzer : device[%d] port[%d]",
            rxSniff == GT_TRUE ? "Ingress" : "Egress" ,
            sniffDev,
            sniffPort));

        descrPtr->eArchExtInfo.toTargetSniffInfo.sniffUseVidx = descrPtr->useVidx;
        descrPtr->eArchExtInfo.toTargetSniffInfo.sniffisTrgPhyPortValid = descrPtr->eArchExtInfo.isTrgPhyPortValid;
        descrPtr->eArchExtInfo.toTargetSniffInfo.sniffTrgEPort = sniffPort;

        portToCheckNullPort = SMAIN_NOT_VALID_CNS;/* don't care for sip5 */
    }
    else
    {
        portToCheckNullPort = sniffPort;
    }

    /* for simulation do NULL port after analyzer port */
    isNullPort = snetChtEqNullPortCheck(devObjPtr,descrPtr,portToCheckNullPort);
    if(isNullPort == GT_TRUE)
    {
        __LOG(("WARNING : the target of the Analyzer is 'NULL port'(%d) --> so IGNORE request to sent to analyzer index [%d] \n",
            SNET_CHT_NULL_PORT_CNS,
            (devObjPtr->supportMultiAnalyzerMirroring ? descrPtr->analyzerIndex : 1)));

        return GT_FALSE;
    }

    *monitorTargetDevicePtr = sniffDev;
    *monitorTargetPortPtr   = sniffPort;

    return GT_TRUE;
}
/*******************************************************************************
*  snetChtEqDoRxSniff
*
* DESCRIPTION:
*        Send frame to the Rx Sniffer
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       doStatSniff - do statistical sniffing
*
* RETURN:
*
* COMMENT :
*
*
*******************************************************************************/
static GT_VOID snetChtEqDoRxSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL doStatSniff
)
{
    DECLARE_FUNC_NAME(snetChtEqDoRxSniff);

    GT_BOOL passStatSniff;              /* packet may be send to Rx Sniffer */
    GT_U32 rxSniffDev, rxSniffPort;     /* Rx sniffer device an port */
    GT_U32 analyzQosCfgRegData;         /* Ingress and Egress Monitoring to
                                           Analyzer QoS Configuration Register */

    if (doStatSniff == GT_TRUE)
    {
        /* Check statistical sniffing */
        __LOG(("Check statistical sniffing"));
        passStatSniff  = snetChtEqStatSniff(devObjPtr, descrPtr);
        if (passStatSniff == GT_FALSE)
        {
            return;
        }
    }

    SIM_LOG_PACKET_DESCR_SAVE

    /* Ingress and Egress Monitoring to Analyzer QoS Configuration Register */
    smemRegGet(devObjPtr, SMEM_CHT_INGR_EGR_MON_TO_ANALYZER_QOS_CONF_REG(devObjPtr),
               &analyzQosCfgRegData);

    /* Ingress Analyzer TC */
    descrPtr->tc = SMEM_U32_GET_FIELD(analyzQosCfgRegData, 7, 3);
    /* Ingress Analyzer DP */
    descrPtr->dp = SMEM_U32_GET_FIELD(analyzQosCfgRegData, 5, 2);

    /*  Selection of one of the seven analyzers on a per-port base */
    if(devObjPtr->supportMultiAnalyzerMirroring)
    {
        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E &&
           descrPtr->mirroringMode != SKERNEL_MIRROR_MODE_HOP_BY_HOP_E)
        {
            snetXCatEqSniffFromRemoteDevice(devObjPtr, descrPtr, GT_TRUE );
            return;
        }

        /* Index 0x0 stands for No Mirroring */
        if(descrPtr->analyzerIndex == 0)
        {
            __LOG(("No reason for RX Mirroring \n"));
            return;
        }
    }

    /* get sniffer info */
    __LOG(("get RX sniffer info \n"));
    if(GT_FALSE ==
        snetChtEqSniffInfoGet(devObjPtr, descrPtr, GT_TRUE , &rxSniffDev, &rxSniffPort))
    {
        return;
    }

    /* Ingress analyzer */
    descrPtr->rxSniff = 1;

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEqDoRxSniff");

    /* Send packet to Ingress analyzer */
    __LOG(("Send packet to Ingress analyzer \n"));
    snetChtEqDoTargetSniff(devObjPtr, descrPtr, rxSniffDev, rxSniffPort);
}

/*******************************************************************************
*  snetChtEqDoTargetSniff
*
* DESCRIPTION:
*        Send Rx/Tx sniffed frame to Target Sniffer
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       rxSniffDev  - Rx sniffer device
*       rxSniffPort - Rx sniffer port
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetChtEqDoTargetSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 trgSniffDev,
    IN GT_U32 trgSniffPort
)
{
    DECLARE_FUNC_NAME(snetChtEqDoTargetSniff);

    GT_U32 fieldVal;                    /* Registers field value */
    GT_U32 bitOffset;                   /* first bit of register field */
    GT_U32 step;                        /* register fields step */

    if (trgSniffPort == SNET_CHT_CPU_PORT_CNS)
    {
        descrPtr->modifyUp = 0;
        descrPtr->modifyDscp = 0;
        descrPtr->cpuCode = (descrPtr->rxSniff) ?
            SNET_CHT_INGRESS_MIRRORED_TO_ANALYZER :
            SNET_CHT_EGRESS_MIRRORED_TO_ANALYZER;

        /* Send to CPU */
        __LOG(("MIRRORING to CPU \n"));
        snetChtEqDoToCpu(devObjPtr, descrPtr);
    }
    else
    {
        /* Ingress analyzer */
        if(descrPtr->rxSniff)
        {
            bitOffset = descrPtr->localDevSrcPort;
            /* TO_ANALYZER ingress forwarding restriction conf. reg. 0x0b020004 */
            if ((SNET_CHT_CPU_PORT_CNS != bitOffset) && (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr)))
            {
                if(descrPtr->eArchExtInfo.eqIngressEPortTablePtr)
                {
                    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
                    {
                        step = 7;
                    }
                    else
                    {
                        step = 8;
                    }
                    /*To Analyzer Forward Enable*/
                    fieldVal =
                        snetFieldValueGet(descrPtr->eArchExtInfo.eqIngressEPortTablePtr,
                            2 + ((descrPtr->eArchExtInfo.eqInfo.eqIngressEPort & 0x3) * step),
                            1);
                }
                else
                {
                    smemRegFldGet(devObjPtr,
                                  SMEM_CHT2_TO_ANALYZER_INGR_FORWARD_RESTICT_REG(devObjPtr),
                                  bitOffset, 1, &fieldVal);
                }

                if (fieldVal == 0)
                {   /* update the relevant counter and exit the function */

                    __LOG((" NOTICE : The ingress port [%d] is filtering is ingress ALL 'sniffer' traffic  \n",
                        descrPtr->localDevSrcPort));


                    smemRegGet(devObjPtr, SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(devObjPtr),
                               &fieldVal);
                    fieldVal++;
                    smemRegSet(devObjPtr, SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(devObjPtr),
                               fieldVal);

                    __LOG(("Update forward restricted Counter Register by 1 from [%d] \n",
                        (fieldVal-1)));
                    return ;
                }
            }
        }

        /* SIP5 devices support VIDX analyzers and descrPtr->useVidx is already
           set according to analyzer's ePort.
           Older devices supports only single destination analyzers. */
        if (devObjPtr->supportEArch == 0)
        {
            descrPtr->useVidx = 0; /* single destination treatment */
        }

        /* Send Rx/Tx sniffed frame to Target Sniffer */
        __LOG(("Send Rx/Tx sniffed frame to Target Sniffer"));
        snetChtEqSniffToTrgSniff(devObjPtr, descrPtr, trgSniffPort, trgSniffDev);
    }
}

/*******************************************************************************
*  snetChtEqDoToCpuGeneric
*
* DESCRIPTION:
*        To CPU frame processing - called from ingress and egress pipe lines
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       callToTxq - after changing descriptor do we allow to call 'TXQ'
* RETURN:
*
* COMMENT:
*       Port Ingress/Egress Forwarding Restrictions
*
*       NOTE: when called from the egress pipe line we not call the egress pipe
*             line from this function , otherwise we do
*******************************************************************************/
static GT_VOID snetChtEqDoToCpuGeneric
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL  callToTxq
)
{
    DECLARE_FUNC_NAME(snetChtEqDoToCpuGeneric);

    GT_U32 cpuCodeTableEntry;       /* data of CPU code table's entry */
    GT_U32 statRateLimitThresh;     /* data of Stat rate limit table's entry */
    GT_U32 randomValue;             /* random value from 0 up to 0xFFFF FFFF */
    GT_U32 qosProfileEntry;         /* QOS Profile entry */
    GT_U32 cpuTrgDev;               /* device number to send to CPU */
    GT_U32 fieldVal;                /* Register field value */
    GT_U32 fieldBit;                /* Register field offset */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 bitOffset;               /* first bit of register field */
    GT_U32 tmpValue;                /* temp value */
    GT_U32 step;                    /* register fields step */
    GT_U32 goToLocalCpu;            /* 1 - go to local CPU, 0 - go to remote CPU */
    GT_U32 tableIndex;

    snetChtEqAppSpecCpuCodeAssign(devObjPtr, descrPtr);

    /* TO_CPU Ingress Forwarding Restriction Configuration Register 0x0b020000*/
    bitOffset = descrPtr->localDevSrcPort;
    if(((SNET_CHT_CPU_PORT_CNS == bitOffset) && SKERNEL_IS_CHEETAH3_DEV(devObjPtr)) ||
       ((SNET_CHT_CPU_PORT_CNS != bitOffset) && (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))))
    {
        if (SNET_CHT_CPU_PORT_CNS == bitOffset)
        {
            bitOffset = 28;
        }

        if(descrPtr->eArchExtInfo.eqIngressEPortTablePtr)
        {
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                step = 7;
            }
            else
            {
                step = 8;
            }

            /*To CPU Forward Enable*/
            fieldVal =
                snetFieldValueGet(descrPtr->eArchExtInfo.eqIngressEPortTablePtr,
                    0 + ((descrPtr->eArchExtInfo.eqInfo.eqIngressEPort & 0x3) * step)
                    ,1);
        }
        else
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_TO_CPU_INGR_FORWARD_RESTICT_REG(devObjPtr),
                            bitOffset, 1, &fieldVal);
        }

        if (fieldVal == 0)
        {   /* update the relevant counter and exit the function */
            __LOG((" NOTICE : The ingress port [%d] is filtering is ingress ALL 'TO_CPU' traffic  \n",
                descrPtr->localDevSrcPort));

            smemRegGet(devObjPtr, SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(devObjPtr),&fieldVal);
            fieldVal++;
            smemRegSet(devObjPtr, SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(devObjPtr), fieldVal);

            __LOG(("Update forward restricted Counter Register by 1 from [%d] \n",
                (fieldVal-1)));
            return ;
        }
    }

    SIM_LOG_PACKET_DESCR_SAVE

    regAddr = SMEM_CHT_CPU_CODE_TBL_MEM(devObjPtr, descrPtr->cpuCode);

    /* Read Cpu Code Table Entry */
    smemRegGet(devObjPtr, regAddr, &cpuCodeTableEntry);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Get statistical rate limit threshold and random value */
        tableIndex = SMEM_U32_GET_FIELD(cpuCodeTableEntry, 16, 8);
    }
    else
    {
        /* Get statistical rate limit threshold and random value */
        tableIndex = SMEM_U32_GET_FIELD(cpuCodeTableEntry, 6, 5);
    }

    smemRegGet(devObjPtr,
               SMEM_CHT_STAT_RATE_TBL_MEM(devObjPtr, tableIndex),
               &statRateLimitThresh);

    randomValue = rand();

    if (randomValue >= statRateLimitThresh)
    {
        /* Not send to CPU */
        __LOG(("Not send to CPU, randomValue[0x%x] >= statRateLimitThresh[0x%x] \n",
               randomValue,statRateLimitThresh));
        return;
    }

    if (descrPtr->cpuCode == SNET_CHT_CPU_TO_CPU)
    {
        descrPtr->modifyUp = 0;
        descrPtr->modifyDscp = 0;
        descrPtr->tc = descrPtr->qos.fromCpuQos.fromCpuTc;
        descrPtr->dp = descrPtr->qos.fromCpuQos.fromCpuDp;
    }
    else
    {
        regAddr = SMEM_CHT_QOS_TBL_MEM(devObjPtr, descrPtr->qos.qosProfile);

        /* QoSProfile to QoS table Entry */
        smemRegGet(devObjPtr, regAddr, &qosProfileEntry);

        /* CPU Code TC */
        descrPtr->tc = SMEM_U32_GET_FIELD(cpuCodeTableEntry, 0, 3);

        /* CPU Code DP */
        descrPtr->dp = SMEM_U32_GET_FIELD(cpuCodeTableEntry, 3, 2);

        /* QoS Profile DSCP */
        descrPtr->dscp = SMEM_U32_GET_FIELD(qosProfileEntry, 0, 6);

        if(descrPtr->txqToEq == 0)/* logic from verifier function SipEqEvtDecisions::SetModifyUP() */
        {
            /* fix code according to : JIRA : CPSS-4210 :
                Traffic to CPU that is VLAN and UP were modified -
                don't perform UP modification (only VLAN) */
            __LOG(("TO_CPU not allow to modify the UP from the 'ingress pipe' , so set descrPtr->modifyUp = 0 \n"));
            descrPtr->modifyUp = 0;
        }

        if (descrPtr->modifyUp)
        {
            /* QoS Profile UP */
            descrPtr->up = SMEM_U32_GET_FIELD(qosProfileEntry, 6, 3);
        }
    }

    /* CPU Code Target Device Index */
    fieldVal = SMEM_U32_GET_FIELD(cpuCodeTableEntry, 11, 3);
    if (fieldVal)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            fieldBit = (fieldVal & 1) * 10;

            switch(fieldVal)
            {
                case 0:/* never happen value 0 !!! */
                case 1:
                    regAddr = SMEM_CHT_TRG_DEV_CONF0_REG(devObjPtr);
                    /* device 1 is in bits 0:9 */
                    fieldBit = 0;
                    break;
                case 2:
                case 3:
                    regAddr = SMEM_CHT_TRG_DEV_CONF1_REG(devObjPtr);
                    break;
                case 4:
                case 5:
                    regAddr = SMEM_LION2_EQ_TRG_DEV_CONF2_REG(devObjPtr);
                    break;
                case 6:
                case 7:
                default:/*default will not happen on 3 bits value*/
                    regAddr = SMEM_LION2_EQ_TRG_DEV_CONF3_REG(devObjPtr);
                    break;
            }

            /* The target device for packets forwarded to CPU */
            smemRegFldGet(devObjPtr, regAddr, fieldBit, 10, &cpuTrgDev);
        }
        else
        {

            /* CPU Target Device Configuration Register0/1 */
            regAddr =   (fieldVal < 5) ? SMEM_CHT_TRG_DEV_CONF0_REG(devObjPtr)
                                       : SMEM_CHT_TRG_DEV_CONF1_REG(devObjPtr);

            fieldBit =  (fieldVal < 5) ? (5 * (fieldVal - 1))
                                       : (5 * (fieldVal % 5));

            /* The target device for packets forwarded to CPU */
            smemRegFldGet(devObjPtr, regAddr, fieldBit, 5, &cpuTrgDev);
        }

        if (cpuTrgDev == descrPtr->ownDev)
        {
            goToLocalCpu = 1;
        }
        else
        {
            goToLocalCpu = 0;
            descrPtr->cpuTrgDev = cpuTrgDev;
        }
    }
    else
    {
        goToLocalCpu = 1;
    }

    if (goToLocalCpu)
    {
        descrPtr->cpuTrgDev = descrPtr->ownDev;
        /* get srcTrgEPort info before modify descrPtr->trgEPort .
           This info needed for egress mirror and egress STC */
        if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr))
        {
            descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->trgEPort;
        }
        descrPtr->trgEPort = SNET_CHT_CPU_PORT_CNS;
        if(devObjPtr->numOfTxqUnits >= 2)
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* in SIP5 the CPU port is determined in the EGF (egress) ,
                and not need to FORCE it to any hemisphere */
                descrPtr->forceToCpuTrgPortOnHemisphare0 = 0;
            }
            else
            {
                /* CPU Port Mode */
                smemRegFldGet(devObjPtr, SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr), 17 , 1, &tmpValue);

                /* check if force the 'to cpu' traffic to go to CPU from hemisphere 0 */
                descrPtr->forceToCpuTrgPortOnHemisphare0 = (1 - tmpValue);
            }
        }
        __LOG(("Send TO_CPU to local CPU"));
    }
    else
    {
        __LOG(("Send TO_CPU to remote CPU"));
    }

    descrPtr->outGoingMtagCmd = SKERNEL_MTAG_CMD_TO_CPU_E;
    /* CPU Code Truncated */
    descrPtr->truncated = SMEM_U32_GET_FIELD(cpuCodeTableEntry, 5, 1);

    if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr))
    {
        descrPtr->srcTrgDev = descrPtr->ownDev;
    }

    if (descrPtr->cpuCode == SNET_CHT_EGRESS_SAMPLED ||
        descrPtr->cpuCode == SNET_CHT_EGRESS_MIRRORED_TO_ANALYZER)
    {
        descrPtr->srcTrg = 1;
        descrPtr->srcTaggedTrgTagged = descrPtr->trgTagged;
        if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr))
        {
            descrPtr->srcTrgPhysicalPort = descrPtr->egressTrgPort;
            /* trgEport info --> already set before assign descrPtr->trgEPort */
            /*descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->trgEPort;*/
        }
    }
    else
    {
        descrPtr->srcTrg = 0;
        descrPtr->srcTaggedTrgTagged = descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS];

        if(devObjPtr->supportEArch &&
            descrPtr->marvellTagged == 1)
        {
            if((descrPtr->incomingMtagCmd != SKERNEL_MTAG_CMD_TO_CPU_E) &&
               (descrPtr->incomingMtagCmd != SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E))
            {
                /* the FORWARD DSA and the FROM_CPU not have <srcTrgEPort> field ,
                   so we need to use original packet's source ePort / trunk, depending on Desc<OrigIsTrunk>.
                */
                descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->origSrcEPortOrTrnk;

                descrPtr->srcTrgDev = descrPtr->srcDev;
            }
        }
        else
        if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr))
        {
            /* srcEport info (the srcEport muxed with the trunkId) */
            descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->origSrcEPortOrTrnk;

            if (descrPtr->mailBoxToNeighborCPU)
            {
                descrPtr->srcTrgPhysicalPort = descrPtr->localPortGroupPortAsGlobalDevicePort;/*global port*/
            }
            else
            {
                descrPtr->srcTrgDev = descrPtr->srcDev;
                descrPtr->srcTrgPhysicalPort = ( descrPtr->origIsTrunk == 0 ) ?
                                            descrPtr->localPortTrunkAsGlobalPortTrunk :
                                            descrPtr->localPortGroupPortAsGlobalDevicePort ; /*global port*/
            }
        }


        if(descrPtr->rxSniff &&
            descrPtr->cpuCode == SNET_CHT_INGRESS_MIRRORED_TO_ANALYZER &&
            descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E &&
            devObjPtr->errata.eqToCpuForRxAnalyzerSrcBasedMirrorTrgInfoInsteadOfSrcInfo)
        {
            /* cpu port which is rx analyzer port in MirroringMode == SRC_FWD_OVERRIDE_MIRRORING_MODE
                get 'target port,device' instead of 'source port,device' */
            descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->trgEPort;
            descrPtr->srcTrgDev                = descrPtr->trgDev;
            descrPtr->srcTrgPhysicalPort       = descrPtr->eArchExtInfo.trgPhyPort;

            __LOG(("WARNING : ERRATA : cpu port which is rx analyzer port in MirroringMode == SRC_FWD_OVERRIDE_MIRRORING_MODE \n"
                "get DSA tag info 'target port,device' instead of 'source port,device' \n"));
        }
    }



    descrPtr->useVidx = 0;/* single destination treatment */

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEqDoToCpuGeneric");

    if(callToTxq == GT_TRUE)
    {
        /* Call egress processing */
        __LOG(("Call egress processing"));
        eqPrepareEgress(devObjPtr, descrPtr);
    }
}

/*******************************************************************************
*  snetChtEqDoToCpu
*
* DESCRIPTION:
*        To CPU frame processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
* COMMENT:
*       Cht2 : 8.2.5 Port Ingress/Egress Forwarding Restrictions
*
*******************************************************************************/
static GT_VOID snetChtEqDoToCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    snetChtEqDoToCpuGeneric(devObjPtr,descrPtr,GT_TRUE/* call the TXQ*/);
}

/*******************************************************************************
*  snetChtEqDoToCpuNoCallToTxq
*
* DESCRIPTION:
*        To CPU frame handling from the egress pipe line - don't call the TXQ.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
* COMMENT:
*
*
*******************************************************************************/
extern GT_VOID snetChtEqDoToCpuNoCallToTxq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    snetChtEqDoToCpuGeneric(devObjPtr,descrPtr,GT_FALSE/* DONT call the TXQ*/);
}

/*******************************************************************************
*  snetChtEqDoFromCpu
*
* DESCRIPTION:
*        handle 'FROM CPU' packet in the original descriptor
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
* COMMENT :
*
*******************************************************************************/
static GT_VOID snetChtEqDoFromCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDoFromCpu);

    SIM_LOG_PACKET_DESCR_SAVE

    if(descrPtr->useVidx == 0)
    {
        if (descrPtr->mailBoxToNeighborCPU &&
            descrPtr->localDevSrcPort != SNET_CHT_CPU_PORT_CNS)
        {
            descrPtr->cpuCode = SNET_CHT_MAIL_FROM_NEIGHBOR_CPU;

            SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION("snetChtEqDoFromCpu");

            /* Send to CPU */
            __LOG(("Send to CPU"));
            snetChtEqDoToCpu(devObjPtr, descrPtr);

            return;
        }
        if (SKERNEL_IS_MATCH_DEVICES_MAC(descrPtr->trgDev, descrPtr->ownDev,
                                         devObjPtr->dualDeviceIdEnable.eq) &&
            descrPtr->trgEPort == SNET_CHT_CPU_PORT_CNS)
        {
            descrPtr->cpuCode = SNET_CHT_CPU_TO_CPU;

            SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION("snetChtEqDoFromCpu");

            /* Send to CPU */
            __LOG(("Send to CPU"));
            snetChtEqDoToCpu(devObjPtr, descrPtr);

            return;
        }
    }
    else
    {
        /* useVidx = 1*/
        if(descrPtr->mirrorToAllCpus &&
           descrPtr->localDevSrcPort != SNET_CHT_CPU_PORT_CNS)
        {
            descrPtr->cpuCode = SNET_CHT_MAIL_FROM_NEIGHBOR_CPU;

            SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION("snetChtEqDoFromCpu");

            /* Send to CPU */
            __LOG(("Send to CPU"));
            snetChtEqDoToCpu(devObjPtr, descrPtr);

            return;
        }
    }


    descrPtr->outGoingMtagCmd = SKERNEL_MTAG_CMD_FROM_CPU_E;
    descrPtr->modifyUp = 0;
    descrPtr->modifyDscp = 0;
    descrPtr->tc = descrPtr->qos.fromCpuQos.fromCpuTc;
    descrPtr->dp = descrPtr->qos.fromCpuQos.fromCpuDp;

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEqDoFromCpu");

    /* Call egress processing */
    __LOG(("Call egress processing"));
    eqPrepareEgress(devObjPtr, descrPtr);

    return;
}
/*******************************************************************************
*  snetChtEqDoFrwd
*
* DESCRIPTION:
*        Forward packet in the original descriptor
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
* COMMENT : Cht2 , 8.2.5 : Port Ingress/Egress Forwarding Restriction
*
*******************************************************************************/
static GT_VOID snetChtEqDoFrwd
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDoFrwd);

    GT_U32 qosProfileEntry;             /* QOS Profile entry */
    GT_U32 regAddr;                     /* Register address */
    GT_U32 fldVal;
    GT_U32 bitOffset;                   /* first bit of register field */
    GT_U32 step;                        /* register fields step */

    /* TO NETWORK Ingress Forwarding Restriction conf. reg. 0x0b020004
       (SNET_CHT_CPU_PORT_CNS should not be checked by this filter) */
    bitOffset = descrPtr->localDevSrcPort;
    if ((SNET_CHT_CPU_PORT_CNS != bitOffset) && (!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr)))
    {
        if(descrPtr->eArchExtInfo.eqIngressEPortTablePtr)
        {
            if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
            {
                step = 7;
            }
            else
            {
                step = 8;
            }

            /*To Network Forward Enable*/
            fldVal =
                snetFieldValueGet(descrPtr->eArchExtInfo.eqIngressEPortTablePtr,
                    1 + ((descrPtr->eArchExtInfo.eqInfo.eqIngressEPort & 0x3) * step)
                    ,1);
        }
        else
        {
            smemRegFldGet(devObjPtr, SMEM_CHT2_TO_NW_INGR_FORWARD_RESTICT_REG(devObjPtr),
                                                        bitOffset, 1, &fldVal);
        }

        if (fldVal == 0)
        {   /* update the relevant counter and exit the function */
            __LOG((" NOTICE : The ingress port [%d] is filtering is ingress ALL 'forwar' traffic  \n",
                descrPtr->localDevSrcPort));

            smemRegGet(devObjPtr, SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(devObjPtr), &fldVal);
            fldVal++;
            smemRegSet(devObjPtr, SMEM_CHT2_INGR_FORWARD_RESTICT_COUNTER_REG(devObjPtr), fldVal);

            __LOG(("Update forward restricted Counter Register by 1 from [%d] \n",
                (fldVal-1)));

            /* if packet command was SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E
               then only the 'copy to CPU' will be sent to CPU by
               snetChtEqDuplicateToCpu(...) */
            return ;
        }
    }

    SIM_LOG_PACKET_DESCR_SAVE

    regAddr = SMEM_CHT_QOS_TBL_MEM(devObjPtr, descrPtr->qos.qosProfile);

    /* QoSProfile to QoS table Entry */
    smemRegGet(devObjPtr, regAddr, &qosProfileEntry);

    if (descrPtr->modifyUp)
    {
        /* QoS Profile UP */
        descrPtr->up = SMEM_U32_GET_FIELD(qosProfileEntry, 6, 3);
    }

    if (descrPtr->modifyDscp)/* modify DSCP / EXP */
    {
        /* QoS Profile DSCP */
        descrPtr->dscp = SMEM_U32_GET_FIELD(qosProfileEntry, 0, 6);

        /* the EXP is modified in the HA ! */
    }

    if(descrPtr->qos.ingressExtendedMode)
    {
        __LOG(("sip5: EQ extraction of TC,DP from QoS table is skipped in extended QoS mode (taken from qosProfileBits)"));

        /* Traffic Class */
        descrPtr->tc = SMEM_U32_GET_FIELD(descrPtr->qos.qosProfile, 2, 3);

        /* Drop Precedence */
        descrPtr->dp = SMEM_U32_GET_FIELD(descrPtr->qos.qosProfile, 0, 2);

    }
    else
    {
        /* Traffic Class */
        descrPtr->tc = SMEM_U32_GET_FIELD(qosProfileEntry, 11, 3);

        /* Drop Precedence */
        descrPtr->dp = SMEM_U32_GET_FIELD(qosProfileEntry, 9, 2);
    }

    if (descrPtr->useVidx == 0)
    {
        /* The destination device number is ignored when the destination port is 63.
           The TO_CPU packet is sent to the target device specified in the CPU Code table */
        if (descrPtr->trgEPort == SNET_CHT_CPU_PORT_CNS)
        {
            descrPtr->cpuCode = SNET_CHT_BRIDGED_PACKET_FORWARD;

            SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION("snetChtEqDoFrwd");

            /* Send to CPU */
            __LOG(("Send to CPU"));
            snetChtEqDoToCpu(devObjPtr, descrPtr);

            return;
        }
    }

    descrPtr->outGoingMtagCmd = SKERNEL_MTAG_CMD_FORWARD_E;

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEqDoFrwd");

    /*Call egress processing */
    __LOG(("Call egress processing"));
    eqPrepareEgress(devObjPtr, descrPtr);

    return;
}

/*******************************************************************************
*  snetChtEqDuplicateToCpu
*
* DESCRIPTION:
*        Duplicate descriptor and send to CPU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqDuplicateToCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDuplicateToCpu);

    SKERNEL_FRAME_CHEETAH_DESCR_STC * newDescrPtr; /* Pointer to new descriptor*/

    /* Get pointer to the duplicated descriptor */
    newDescrPtr = snetChtEqDuplicateDescr(devObjPtr, descrPtr);
    /* Send to CPU with new descriptor */
    __LOG(("Send to CPU with the Duplicated descriptor \n"));
    snetChtEqDoToCpu(devObjPtr, newDescrPtr);
}

/*******************************************************************************
*  snetChtEqDoTrgSniff
*
* DESCRIPTION:
*        Send to sniffer in the original descriptor
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqDoTrgSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDoTrgSniff);

    if (descrPtr->rxSniff)
    {
        /* Send to Rx Sniffer */
        __LOG(("Send to Rx Sniffer"));
        snetChtEqDoRxSniff(devObjPtr, descrPtr, GT_TRUE);
    }
    else
    {
        /* Process Tx mirroring */
        __LOG(("Process Tx mirroring"));
        snetChtEqTxSniffer(devObjPtr, descrPtr, GT_TRUE);
    }
}

/*******************************************************************************
*  snetChtEqTxSniffer
*
* DESCRIPTION:
*        Send to Tx Sniffer
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       packetFromIngress   - packet origin
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqTxSniffer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_BOOL packetFromIngress
)
{
    DECLARE_FUNC_NAME(snetChtEqTxSniffer);

    GT_U32 txSniffDev, txSniffPort; /* Target sniffer device/port */
    GT_U32 egrMirrCfgRegData;       /* Egress Monitoring Enable Configuration
                                       Register */
    GT_U32 analyzQosCfgRegData;     /* Ingress and Egress Monitoring to
                                       Analyzer QoS Configuration Register */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 * regPtr;                /* Register data pointer */
    GT_U32 fieldVal;                /* Register field */
    GT_U32 fldOffset;               /* Register field offset */
    GT_U32  cpuPort;
    GT_BIT  mdb = 0;                /* mdb bit */
    GT_U32  egressTrgPort = descrPtr->egressTrgPort;/*the port we do egress mirror to*/
    GT_BIT  txMirrorDone = (descrPtr->bmpsEqUnitsGotTxqMirror & (1 << devObjPtr->portGroupId)) ? 1 : 0;

    /* the descrPtr->analyzerIndex may already hold info from the TXQ !*/
    if(devObjPtr->dualDeviceIdEnable.eq)
    {
        SMEM_U32_SET_FIELD(egressTrgPort,6,1,
            SMEM_U32_GET_FIELD(descrPtr->ownDev,0,1));
    }

    if (txMirrorDone && (descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_HOP_BY_HOP_E))
    {
        /* Only in Source-based forwarding mode packet could  be mirrored
           more than once */

        __LOG(("in HOP-By-HOP can't mirror more than once on the same EQ unit \n"));

        return;
    }

    if(devObjPtr->supportMultiAnalyzerMirroring)
    {
        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E &&
           descrPtr->mirroringMode != SKERNEL_MIRROR_MODE_HOP_BY_HOP_E)
        {
            snetXCatEqSniffFromRemoteDevice(devObjPtr, descrPtr, GT_FALSE);
            return;
        }

        /* Source-based forwarding mode. */
        if(descrPtr->mirroringMode != SKERNEL_MIRROR_MODE_HOP_BY_HOP_E)
        {
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                /* in SIP5 the EQ not hold the Egress mirror index info */
                fieldVal = 0;
            }
            else
            {
                /* Mirrored by the port-based egress mirroring mechanism */
                regAddr = SMEM_XCAT_EGR_MIRROR_INDEX_REG(devObjPtr,
                                                         egressTrgPort);
                regPtr = smemMemGet(devObjPtr, regAddr);
                if(egressTrgPort == SNET_CHT_CPU_PORT_CNS && skernelIsXcatOnlyDev(devObjPtr))
                {
                    fldOffset = 24;
                }
                else
                {
                    fldOffset = ((egressTrgPort & 0x3f) % 10) * 3;
                }

                fieldVal = SMEM_U32_GET_FIELD(*regPtr, fldOffset, 3);
            }
        }
        /* Hop-by-hop forwarding mode. */
        else
        {
            regAddr = SMEM_XCAT_ANALYZER_PORT_GLOBAL_CONF_REG(devObjPtr);
            regPtr = smemMemGet(devObjPtr, regAddr);

            eqMirrorAnalyzerHopByHop(devObjPtr,descrPtr,(*regPtr),GT_FALSE/*TX mirror*/);
            /* index calculated in eqMirrorAnalyzerHopByHop */
            fieldVal = descrPtr->analyzerIndex;

            if(fieldVal == 0)
            {
                /* in Hop-By-Hop the global configuration must be != 0 to enable mirroring */
                /* so index 0 --> no mirroring */
                return;
            }
        }

        snetXcatEgressMirrorAnalyzerIndexSelect(devObjPtr,descrPtr,fieldVal);

        /* Index 0x0 stands for No Mirroring */
        if(descrPtr->analyzerIndex == 0)
        {
            return;
        }
    }

    SIM_LOG_PACKET_DESCR_SAVE

    /* Ingress and Egress Monitoring to Analyzer QoS Configuration Register */
    smemRegGet(devObjPtr, SMEM_CHT_INGR_EGR_MON_TO_ANALYZER_QOS_CONF_REG(devObjPtr),
               &analyzQosCfgRegData);
    /* Egress Analyzer TC */
    descrPtr->tc = SMEM_U32_GET_FIELD(analyzQosCfgRegData, 2, 3);
    /* Egress Analyzer DP */
    descrPtr->dp = SMEM_U32_GET_FIELD(analyzQosCfgRegData, 0, 2);

    /* get sniffer info */
    if(GT_FALSE ==
        snetChtEqSniffInfoGet(devObjPtr, descrPtr, GT_FALSE , &txSniffDev, &txSniffPort))
    {
        return;
    }

    if (packetFromIngress == GT_FALSE)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /*
                EQ Egress Mirroring Trigger Removal
                The EQ logic that modifies the MDB bit according to egress mirroring configuration is obsolete.
                The following legacy configurations are also obsolete:
                <Port<0+r*32>EgressMonitorEn> in <Egress Monitoring Enable Configuration r Register>
                <CascadeEgressMonitorEnable> in <Cascade Egress Monitoring Enable Configuration Register>
            */
        }
        else
        {
            if(egressTrgPort == SNET_CHT_CPU_PORT_CNS &&
               (devObjPtr->supportEqEgressMonitoringNumPorts <= 32))
            {
                if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
                {
                    cpuPort = 28;
                }
                else /* Cheetah and Cheetah+ devices */
                {
                    cpuPort = 27;
                }

                /* Egress Monitoring Enable Configuration Register */
                smemRegGet(devObjPtr, SMEM_CHT_EGR_MON_EN_CONF_REG(devObjPtr), &egrMirrCfgRegData);

                /* Port Egress Monitor Enable */
                mdb = SMEM_U32_GET_FIELD(egrMirrCfgRegData,
                                              cpuPort, 1);
            }
            else
            {
                if(egressTrgPort < devObjPtr->supportEqEgressMonitoringNumPorts)
                {
                    /* Egress Monitoring Enable Configuration Register by index*/
                    smemRegGet(devObjPtr, SMEM_LION_EGR_MON_EN_CONF_REG_BY_INDEX_REG(devObjPtr,
                        (egressTrgPort >> 5)),
                        &egrMirrCfgRegData);

                    /* Port Egress Monitor Enable */
                    mdb = SMEM_U32_GET_FIELD(egrMirrCfgRegData,
                                                  (egressTrgPort & 0x1f), 1);
                }
            }

            if(mdb == 0)
            {
                /* the mdb must be sync with the TXQ mirroring !
                    The HW will support the mirroring , But the buffer management
                    of the HW may get corrupted due to wrong number of pointers
                    to the buffers.

                    So we do FATAL ERROR
                */

                skernelFatalError("snetChtEqTxSniffer: egress mirroring in EQ not sync with TXQ  \n");
            }

            if (0 == SKERNEL_IS_MATCH_DEVICES_MAC(txSniffDev, descrPtr->ownDev,
                                             devObjPtr->dualDeviceIdEnable.eq))
            {
                if(SKERNEL_IS_LION_REVISON_B0_DEV(devObjPtr))
                {
                    regAddr = SMEM_LION_EQ_CASCADE_EGRESS_MONITORING_ENABLE_CONFIGURATION_REG(devObjPtr);
                    smemRegFldGet(devObjPtr, regAddr, 0 ,1 , &fieldVal);
                }
                else
                {
                    /* Cascade Egress Monitor En */
                    if(!SKERNEL_IS_CHEETAH1_ONLY_DEV(devObjPtr))
                    {
                        fldOffset = 29;
                    }
                    else /* Cheetah and Cheetah+ devices */
                    {
                        fldOffset = 28;
                    }

                    /* Egress Monitoring Enable Configuration Register */
                    smemRegGet(devObjPtr, SMEM_CHT_EGR_MON_EN_CONF_REG(devObjPtr), &egrMirrCfgRegData);

                    fieldVal = SMEM_U32_GET_FIELD(egrMirrCfgRegData, fldOffset, 1);
                }

                if (fieldVal == 0 &&
                    descrPtr->macDaType == SKERNEL_UNICAST_MAC_E)
                {
                    /* Mirror to cascade disabled */
                    __LOG(("Mirror to cascade disabled"));
                    return;
                }
            }
        }
    }

    /* Egress analyzer */
    descrPtr->rxSniff = 0;

    SIM_LOG_PACKET_DESCR_COMPARE("snetChtEqTxSniffer");

    /* Send packet to Egress analyzer */
    __LOG(("Send packet to Egress analyzer"));
    snetChtEqDoTargetSniff(devObjPtr, descrPtr, txSniffDev, txSniffPort);

    /* Packet has mirrored */
    /*descrPtr->txMirrorDone = 1;*/
    __LOG(("descrPtr->txMirrorDone = 1 \n"));


    /* bmp of egress EQ units that got mirror:
       Lion3 : Egress mirror sent to egress core, this was done to reduce design complexity.
       Lion1/2 : Egress mirror sent to ingress core.
    */
    descrPtr->bmpsEqUnitsGotTxqMirror |= (1 << devObjPtr->portGroupId);
}

/*******************************************************************************
*  snetChtEqDuplicateStc
*
* DESCRIPTION:
*        Duplicate descriptor and send to CPU with STC code
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqDuplicateStc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqDuplicateStc);

    SKERNEL_FRAME_CHEETAH_DESCR_STC * newDescrPtr;  /* pointer to descriptor */

    /* Get pointer to the duplicated descriptor */
    newDescrPtr = snetChtEqDuplicateDescr(devObjPtr, descrPtr);

    /* Send to CPU with STC code and new descriptor */
    __LOG(("Send to CPU with STC code and new descriptor"));
    snetChtEqDoStc(devObjPtr, newDescrPtr);
}

/*******************************************************************************
*  snetChtEqSniffToTrgSniff
*
* DESCRIPTION:
*        Send frame to the Target Sniffer
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - frame data buffer Id
*       sniffTrgPort        - target sniffer port
*       sniffTrgDev         - target sniffer device
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetChtEqSniffToTrgSniff
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 sniffTrgPort,
    IN GT_U32 sniffTrgDev
)
{
    DECLARE_FUNC_NAME(snetChtEqSniffToTrgSniff);

    if (descrPtr->rxSniff)
    {
        __LOG(("Ingress analyzer \n"));
    }
    else
    {
        __LOG(("Egress analyzer \n"));
    }

    descrPtr->outGoingMtagCmd = SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E;
    descrPtr->modifyUp = 0;
    descrPtr->modifyDscp = 0;

    if(descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_END_TO_END_E)
    {
        if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
        {
            /* packet came from remote device that triggered the mirroring */

            /*Desc<SrcTrgDev>, <SrcTrgPhysicalPort>, <SrcTRGePort> fields are not used or changed.*/


            /* values for legacy in previous devices , but for sip5 should not be use ! */
            descrPtr->sniffTrgDev = descrPtr->trgDev;
            descrPtr->sniffTrgPort = descrPtr->eArchExtInfo.trgPhyPort;
        }
        else
        {
            /* this device triggered the mirroring */

            /*Assign <SrcTrgDev>, <SrcTrgPhysicalPort>, <SrcTRGePort> fields based on the
                source/target information, independent of the sniffer target fields.*/
            if (descrPtr->rxSniff)
            {
                descrPtr->srcTaggedTrgTagged = descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS];
                descrPtr->srcTrgDev = descrPtr->srcDev;
                descrPtr->srcTrgPhysicalPort = descrPtr->localDevSrcPort;
                descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->origSrcEPortOrTrnk;
            }
            else
            {
                descrPtr->srcTaggedTrgTagged = descrPtr->trgTagged;
                descrPtr->srcTrgDev = descrPtr->ownDev;
                /*descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->trgEPort; --> already set*/
                /*descrPtr->srcTrgPhysicalPort = descrPtr->egressTrgPort; --> already set */
            }

            descrPtr->sniffTrgDev = sniffTrgDev;
            descrPtr->sniffTrgPort = sniffTrgPort;
        }

    }
    else
    {
        descrPtr->sniffTrgDev = sniffTrgDev;
        descrPtr->sniffTrgPort = sniffTrgPort;

        if(devObjPtr->supportEArch &&
           descrPtr->mirroringMode == SKERNEL_MIRROR_MODE_SRC_BASED_OVERRIDE_SRC_TRG_E)
        {
            if(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
            {
                /* packet came from remote device that triggered the mirroring */
                /*
                    Desc<TRGePort>      =<SrcTRGePort>
                    Desc<TrgDev>        =<SrcTrgDev>
                    Desc<TrgPhyPort>    =<Src Trg Phy Port>
                    Desc<IsTrgPhyPortValid> = 0
                    Desc<UseVIDX>       =0
                    Desc<eVIDX>         =0
                 */
#if 0 /* already got E2PHY proper mapping ! */
                descrPtr->trgEPort     = descrPtr->eArchExtInfo.srcTrgEPort;
                descrPtr->trgDev       = descrPtr->srcTrgDev;
                descrPtr->eArchExtInfo.trgPhyPort = descrPtr->srcTrgPhysicalPort;
                descrPtr->eArchExtInfo.isTrgPhyPortValid = 0;
                descrPtr->useVidx = 0;
                descrPtr->eVidx = 0;
#endif /*0 */
                /* values for legacy in previous devices , but for sip5 should not be use ! */
                descrPtr->sniffTrgPort = descrPtr->srcTrgPhysicalPort;
                descrPtr->sniffTrgDev  = descrPtr->srcTrgDev;

                /* obsolete parameters -- not to be used ! */
                {
                    descrPtr->eArchExtInfo.toTargetSniffInfo.sniffTrgEPort = descrPtr->trgEPort;
                    descrPtr->eArchExtInfo.toTargetSniffInfo.sniffisTrgPhyPortValid = descrPtr->eArchExtInfo.isTrgPhyPortValid;
                    descrPtr->eArchExtInfo.toTargetSniffInfo.sniffUseVidx = descrPtr->useVidx;
                    descrPtr->eArchExtInfo.toTargetSniffInfo.sniffEVidx   = descrPtr->eVidx;
                }
            }
            else
            {
                /* this device triggered the mirroring */
                /* Desc<SrcTrgDev> = Desc<SniffTrgDev>
                   Desc<SrcTrgPhysicalPort> = Desc<SniffTrgPhyPort>
                   Desc<SrcTRGePort> = Desc<SniffTRGePort>.
                */
                descrPtr->srcTrgDev = sniffTrgDev;
                descrPtr->srcTrgPhysicalPort = sniffTrgPort;
                /*descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->eArchExtInfo.toTargetSniffInfo.sniffTrgEPort; --> already set*/

                if(descrPtr->rxSniff)
                {
                    /* descrPtr->eArchExtInfo.srcTrgEPort was not initialized */
                    descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->trgEPort;
                }
            }
        }
        else
        {
            if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr))
            {
                descrPtr->srcTrgDev = descrPtr->ownDev;
            }

            if(devObjPtr->supportEArch)
            {
                /* fix CQ#153715 */
                /*SKERNEL_MIRROR_MODE_HOP_BY_HOP_E*/
                if (descrPtr->rxSniff)
                {
                    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
                    {
                        descrPtr->srcTrgDev = descrPtr->srcDev;
                        descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->origSrcEPortOrTrnk;
                    }
                    else
                    {
                        /* already have it from DSA tag */
                    }
                }
                else
                {
                    /* the trg info need to set regardless to DSA / not , because
                        HOP by HOP set target per local device configuration */
                    /*already set
                        descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->eArchExtInfo.toTargetSniffInfo.sniffTrgEPort;*/

                    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E)
                    {
                        /* fix CQ#154086 */
                        descrPtr->srcTrgDev          = descrPtr->ownDev;
                        descrPtr->srcTrgPhysicalPort = descrPtr->egressTrgPort;

                        if(descrPtr->origDescrPtr == NULL)
                        {
                            skernelFatalError("snetChtEqSniffToTrgSniff: NULL origDescrPtr \n");
                        }

                        /* use original info because those values already changed at this stage */
                        if(descrPtr->origDescrPtr->useVidx ||
                           descrPtr->origDescrPtr->trgDev != descrPtr->ownDev)
                        {
                            /* fix CQ#154108 */
                            /* Egress mirroring/sampling:
                                if the original packet's target device is the local device,
                                this field contains the target ePort of the original packet,
                                otherwise, this field contains
                                the local device target phy port in the device that
                                triggered the mirroring.*/
                            descrPtr->eArchExtInfo.srcTrgEPort = descrPtr->egressTrgPort;
                        }
                    }
                    else
                    {
                        /* already have it from DSA tag */
                    }
                }
            }
        }

        if(ALLOW_MODIFY_SRC_TRG_INFO_MAC(devObjPtr,descrPtr))
        {
            /* Ingress analyzer */
            if (descrPtr->rxSniff)
            {
                if(devObjPtr->supportEArch == 0)
                {
                    descrPtr->srcTrgPhysicalPort = descrPtr->localPortTrunkAsGlobalPortTrunk;
                }
                else
                {
                    /* we need the physical port */
                    descrPtr->srcTrgPhysicalPort = descrPtr->localDevSrcPort;
                }
            }
            else
            {
                descrPtr->srcTrgPhysicalPort = descrPtr->egressTrgPort;
            }
        }
    }

    if (descrPtr->rxSniff)
    {
        descrPtr->srcTaggedTrgTagged = descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS];
    }
    else
    {
        descrPtr->srcTaggedTrgTagged = descrPtr->trgTagged;
    }

    __LOG_PARAM(descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E);
    __LOG_PARAM(descrPtr->rxSniff);
    __LOG_PARAM(descrPtr->sniffTrgDev);
    __LOG_PARAM(descrPtr->sniffTrgPort);
    __LOG_PARAM(descrPtr->srcTrgDev);
    __LOG_PARAM(descrPtr->eArchExtInfo.srcTrgEPort);
    __LOG_PARAM(descrPtr->srcTrgPhysicalPort);
    __LOG_PARAM(descrPtr->srcTaggedTrgTagged);
    __LOG_PARAM(descrPtr->trgEPort);

    /* Call egress processing */
    __LOG(("Call egress processing"));
    eqPrepareEgress(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetChtEqTxMirror
*
* DESCRIPTION:
*        Send to Tx Sniffer or To CPU by Egress STC. Called from TxQ unit
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
GT_VOID snetChtEqTxMirror
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqTxMirror);

    GT_U32    regAddress;           /* register address */

    /* Egress pipe VID modification is in use */
    descrPtr->useIngressPipeVid = GT_FALSE;

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.eq)
    {
        /* EQ - Ingress ePort Table */
        __LOG(("EQ - Ingress ePort Table"));
        /* take index of the 'Orig egress port' */
        descrPtr->eArchExtInfo.eqInfo.eqIngressEPort = descrPtr->trgEPort;
        regAddress = SMEM_LION2_EQ_INGRESS_EPORT_TBL_MEM(devObjPtr,
            descrPtr->eArchExtInfo.eqInfo.eqIngressEPort);

        descrPtr->eArchExtInfo.eqIngressEPortTablePtr = smemMemGet(devObjPtr, regAddress);
    }

    if (descrPtr->txSampled)
    {
        descrPtr->cpuCode = SNET_CHT_EGRESS_SAMPLED;

        /* Send to CPU */
        __LOG(("Send to CPU"));
        snetChtEqDoToCpu(devObjPtr, descrPtr);
    }
    else
    {
        /* Process Tx mirroring */
        __LOG(("Process Tx mirroring"));
        snetChtEqTxSniffer(devObjPtr, descrPtr, GT_FALSE);
    }

    return;
}

/*******************************************************************************
*   snetChtEqAppSpecCpuCodeAssign
*
* DESCRIPTION:
*
*       Provides an additional application-specific
*       CPU Code assignment mechanism, which overrides the CPU Code
*       assigned by the Bridge or Router engine.
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID snetChtEqAppSpecCpuCodeAssign
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    GT_U32 fldVal, fldVal1;                 /* Register field value */
    GT_U32 fldOffset;                       /* Register field offset */
    GT_U32 entry;                           /* Table entry index */
    GT_U32 regAddr;                         /* Register address */
    GT_U32 regData;                         /* Register data */
    GT_U32 destPort;                        /* Destination TCP port */

    /* Application-specific CPU Code assignment mechanism disabled */
    if (descrPtr->appSpecCpuCode == 0)
    {
        return;
    }

    /* IF the packet is IPv4/6 and L4 is TCP SYN packet
       and global <SYN CPU Code Enable> is set */
    if (descrPtr->isIp)
    {
        if (descrPtr->ipProt == SNET_TCP_PROT_E
            && descrPtr->l4StartOffsetPtr != NULL)
        {
            /* Packet is a TCP SYN */
            if (descrPtr->tcpSyn)
            {
                /* Enable application-specific CPU Code for SYN packets forwarded to the CPU */
                smemRegFldGet(devObjPtr, SMEM_CHT_PRE_EGR_GLB_CONF_REG(devObjPtr),
                              12, 1, &fldVal);
                if (fldVal)
                {
                    descrPtr->cpuCode = SNET_CHT_TCP_SYN_TO_CPU;
                    return;
                }
            }
        }

        if ((descrPtr->ipProt == SNET_TCP_PROT_E ||
            descrPtr->udpCompatible)
            && descrPtr->l4StartOffsetPtr != NULL)
        {
            /* Get the destination IP port from the l4 header */
            destPort = (descrPtr->l4StartOffsetPtr[2] << 8) |
                       (descrPtr->l4StartOffsetPtr[3]);

            for (entry = 0; entry < 16; entry++)
            {
                regAddr =
                    SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_WORD0_TBL_MEM(devObjPtr,
                                                                   entry);
                smemRegGet(devObjPtr, regAddr, &regData);
                /* Minimum destination port for this Entry */
                fldVal = SMEM_U32_GET_FIELD(regData, 0, 16);

                /* Maximum destination port for this Entry */
                fldVal1 = SMEM_U32_GET_FIELD(regData, 16, 16);

                if (destPort < fldVal || destPort > fldVal1)
                {
                    continue;
                }

                regAddr =
                    SMEM_CHT_TCP_UDP_DST_PORT_RANGE_CPU_CODE_WORD1_TBL_MEM(devObjPtr,
                                                                   entry);
                smemRegGet(devObjPtr, regAddr, &regData);

                /* Range TCPUDP Valid */
                fldVal = SMEM_U32_GET_FIELD(regData, 8, 2);
                if ( fldVal == 0 ||
                    (fldVal == 1 && descrPtr->udpCompatible == 0) ||
                    (fldVal == 2 && descrPtr->ipProt != SNET_TCP_PROT_E) )
                {
                    continue;
                }

                /* Range Unicast Multicast Valid */
                fldVal = SMEM_U32_GET_FIELD(regData, 10, 2);
                if ((fldVal == 0 && descrPtr->useVidx == 1) ||
                    (fldVal == 1 && descrPtr->useVidx == 0) || fldVal == 3)
                {
                    continue;
                }

                /* The CPU code assigned to packets matching this range. */
                descrPtr->cpuCode = SMEM_U32_GET_FIELD(regData, 0, 8);

                return;
            }
        }

        regAddr = SMEM_CHT_IP_PROT_CPU_CODE_VALID_CONF_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 0, 8, &fldVal1);

        /* IP Protocol-Based CPU Code match */
        for (entry = 0; entry < 8; entry++)
        {
            /* A Valid bit per IP protocol configured in the IP Protocol CPU Code Entry */
            fldVal = SMEM_U32_GET_FIELD(fldVal1, entry, 1);
            if (fldVal)
            {
                regAddr = SMEM_CHT_IP_PROT_CPU_CODE_ENTRY_TBL_MEM(devObjPtr, entry);
                smemRegGet(devObjPtr, regAddr, &regData);

                /* Entry IP Protocol */
                fldOffset = 16 * (entry % 2);
                fldVal = SMEM_U32_GET_FIELD(regData, fldOffset, 8);
                if (fldVal != descrPtr->ipProt)
                {
                    continue;
                }

                /* CPU code assigned to packet matching */
                fldOffset += 8;
                descrPtr->cpuCode = SMEM_U32_GET_FIELD(regData, fldOffset, 8);

                return;
            }
        }
    }
    else if (descrPtr->arp)
    {
        descrPtr->cpuCode = SNET_CHT_ARP_REPLY_TO_ME;
    }
}

/*******************************************************************************
*   snetChtVirtualPortMapping
*
* DESCRIPTION:
*       The Pre-Egress stage, at the end of the ingress pipeline processing ,
*       is responsible for mapping the packet target virtual port to a physical
*       interface
*
*      The virtual port mapping table is as follows:
*      Target virtual {DevID, Port} => Tunnel-Start Pointer +
*                                      Physical {DevID, Port}/Trunk
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS:
*
*******************************************************************************/
static GT_VOID snetChtVirtualPortMapping(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtVirtualPortMapping);

    GT_U32 virtDevBmp;     /* BMP of virtual devices */
    GT_U32  regAddr;         /* address in mapping table */
    GT_U32  mapRegVal;     /* value of the mapping entry (single register)*/

    if(descrPtr->useVidx == 1 ||
       descrPtr->targetIsTrunk == 1)
    {
        /* destination is trunk/vlan/vidx */
        __LOG(("destination is trunk/vlan/vidx"));
        return;
    }

    /* check if the destination device is virtual device */
    smemRegGet(devObjPtr,SMEM_CHT3_VIRTUAL_DEVICES_BMP_REG(devObjPtr),&virtDevBmp);

    if((virtDevBmp & (1 << descrPtr->trgDev)) == 0)
    {
        /* target device is not virtual */
        __LOG(("target device is not virtual"));
        return;
    }

    /* get index to access to HW mapping */
    regAddr = SMEM_CHT3_VIRTUAL_DEV_PORT_MAP_TO_TS_AND_PHY_DEV_PORT_TRUNK_REG(descrPtr->trgDev,descrPtr->trgEPort);

    smemRegGet(devObjPtr,regAddr,&mapRegVal);

    descrPtr->useVidx = SMEM_U32_GET_FIELD(mapRegVal,0,  1);
    if(descrPtr->useVidx == 0)
    {
        descrPtr->targetIsTrunk  = SMEM_U32_GET_FIELD(mapRegVal,1,  1);
        if(descrPtr->targetIsTrunk)
        {
            descrPtr->trgTrunkId = SMEM_U32_GET_FIELD(mapRegVal,6,  7);
        }
        else
        {
            descrPtr->trgEPort = SMEM_U32_GET_FIELD(mapRegVal,2,  6);
            descrPtr->trgDev  = SMEM_U32_GET_FIELD(mapRegVal,8,  5);
        }
    }
    else
    {
        descrPtr->eVidx = SMEM_U32_GET_FIELD(mapRegVal,1,  12);
    }

    descrPtr->tunnelStart   = SMEM_U32_GET_FIELD(mapRegVal, 13, 1);
    descrPtr->tunnelPtr = SMEM_U32_GET_FIELD(mapRegVal, 14, 13);

    /*srcId filter bitmap --used for "HOT spot applications and Private WLANs" */
    descrPtr->validSrcIdBmp = 1;
    smemRegGet(devObjPtr,regAddr + 4,&descrPtr->srcIdBmp);
}

/*******************************************************************************
* snetChtEqIngressDropCount
*
* DESCRIPTION:
*        Updates Ingress Drop counter
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS: None.
*
*******************************************************************************/
static GT_VOID snetChtEqIngressDropCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqIngressDropCount);

    GT_U32  regAddr;                    /* Register address */
    GT_U32  *regPtr;                    /* Register pointer */
    GT_U32  fldValue;                   /* Register's field value */

    GT_U32  mode;                       /* Ingress Drop Counter configuration mode */
    GT_U32  portOrVlan;                 /* Ingress Drop Counter configuration port/vlan value */
    GT_U32  portToMatch;                /* port to match */
    GT_BOOL updateCounter = GT_FALSE;  /* GT_TRUE - if need to update Ingress Drop Counter */

    /* Check Ingress Drop Counter Configuration Register */
    regAddr = SMEM_CHT_INGR_DROP_CNT_CONFIG_REG(devObjPtr);
    regPtr  = smemMemGet(devObjPtr, regAddr);

    mode = SMEM_U32_GET_FIELD(regPtr[0], 0, 2);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            if(mode == 2) /* port */
            {
                portOrVlan = SMEM_U32_GET_FIELD(regPtr[0], 2, 15);
            }
            else
            {
                portOrVlan = SMEM_U32_GET_FIELD(regPtr[0], 2, 13);
            }
        }
        else
        {
            portOrVlan = SMEM_U32_GET_FIELD(regPtr[0], 2, 13);
        }
    }
    else
    {
        portOrVlan = SMEM_U32_GET_FIELD(regPtr[0], 2, 12);
    }

    switch(mode)
    {
        case 0: /* all */
            __LOG(("update for ALL \n"));
            updateCounter = GT_TRUE;
            break;
        case 1: /* vlan */
            __LOG(("update for VLAN = [0x%x] \n",
                portOrVlan));

            __LOG_PARAM(descrPtr->eVid);
            if(descrPtr->eVid == portOrVlan)
            {
                updateCounter = GT_TRUE;
            }
            break;
        case 2: /* port */
            portToMatch = (devObjPtr->supportEArch && devObjPtr->unitEArchEnable.eq) ?
                                descrPtr->eArchExtInfo.localDevSrcEPort :
                                descrPtr->localDevSrcPort;

            __LOG(("update for port = [0x%x] \n",
                portOrVlan));
            __LOG_PARAM(portToMatch);

            if(portToMatch == portOrVlan)
            {
                updateCounter = GT_TRUE;
            }
            break;
        default:
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                __LOG(("update only for EQ dropped packets \n"));
                __LOG_PARAM(descrPtr->eArchExtInfo.eqInfo.IN_descDrop);
                if(descrPtr->eArchExtInfo.eqInfo.IN_descDrop == 0)
                {
                    /* counts only EQ dropped packets */
                    updateCounter = GT_TRUE;
                }
            }

            break;
    }

    if(updateCounter)
    {
        /* Update Ingress Drop Counter Register */
        regAddr = SMEM_CHT_INGR_DROP_CNT_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr, &fldValue);
        __LOG(("Update Ingress Drop Counter Register by 1 from [%d] \n",
            fldValue));
        smemRegSet(devObjPtr, regAddr, ++fldValue);
    }
    else
    {
        __LOG(("NOT Updating Ingress Drop Counter Register \n"));
    }
}

/*******************************************************************************
*   snetChtEqCncCount
*
* DESCRIPTION:
*       CNC triggering for Ingress VLAN Pass/Drop client (CH3 relevant only)
*
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*
* RETURNS: None.
*
*******************************************************************************/
static GT_VOID snetChtEqCncCount(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetChtEqCncCount);
    GT_BIT  dropBit;/*is the packet dropped */
    GT_U32  outGoingMtagCmd;
    GT_BIT  eqDropOnNullPort = 0;

    if (0 == SKERNEL_IS_CHEETAH3_DEV(devObjPtr))
    {
        /* the device not supports CNC counters */
        return;
    }

    /* NOTE: (from EQ designer)
        the ingress VLAN CNC counting logic and expanding it also to the Pass / Drop client.
        The ingress VLAN CNC is performed only on Ingress packets.
    */

    if(descrPtr->txqToEq)
    {
        /* the client count only from ingress Pipe */
        __LOG(("The EQ CNC clients not count from egress Pipe \n"));

        return;
    }

    if(descrPtr->isEqCncOrigCounted)
    {
        __LOG(("The EQ CNC clients not count replications by EQ unit \n"));
        return;
    }

    /* state that from now it is replication ... HOPE that always 'The original is sent first' ... */
    __LOG(("This accessing to EQ CNC clients considered 'original' packet and will be counted (replications (mirror/STC) will not be) \n"));
    descrPtr->isEqCncOrigCounted = 1;

    dropBit = (descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_HARD_DROP_E ||
               descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_SOFT_DROP_E) ? 1 : 0;

    if(descrPtr->useVidx == 0 && descrPtr->targetIsTrunk == 0)
    {
        if((descrPtr->eArchExtInfo.isTrgPhyPortValid == 0) && (descrPtr->trgEPort == SNET_CHT_NULL_PORT_CNS))
        {
            __LOG(("Target eport is NULL port (considered as 'drop') \n"));
            eqDropOnNullPort = 1;
        }
        else
        if((descrPtr->eArchExtInfo.isTrgPhyPortValid == 1) && (descrPtr->eArchExtInfo.trgPhyPort == SNET_CHT_NULL_PORT_CNS))
        {
            __LOG(("Target eport is NULL port (considered as 'drop') \n"));
            eqDropOnNullPort = 1;
        }
    }

    if(dropBit)
    {
        if(eqDropOnNullPort == 0)
        {
            /* drop that came to EQ unit */
            outGoingMtagCmd = SKERNEL_MTAG_CMD_FORWARD_E;
            __LOG(("Dropped packet (that forwarded not to NULL port) use outGoingMtagCmd = SKERNEL_MTAG_CMD_FORWARD_E \n"));
        }
        else
        {
            /* drop that the EQ unit generated due to NULL port */

            /* the DROP is due to NULL port (of forward) , but only 'forward' replication can get here anyway */
            outGoingMtagCmd = SKERNEL_MTAG_CMD_FORWARD_E;
            __LOG(("Dropped packet that forwarded to NULL port \n"));
        }

        descrPtr->outGoingMtagCmd = outGoingMtagCmd;
    }
    else
    {
        outGoingMtagCmd = descrPtr->outGoingMtagCmd;

        if(eqDropOnNullPort)
        {
            dropBit = 1;
            __LOG(("Dropped packet that send to NULL port \n"));
        }
        else
        {
            __LOG(("Non-dropped packet \n"));
        }
    }

    __LOG_PARAM(eqDropOnNullPort);
    __LOG_PARAM(dropBit);
    __LOG_PARAM(outGoingMtagCmd);


    /* Ingress VLAN Pass/Drop CNC Trigger */
    __LOG(("check for CNC trigger client : Ingress VLAN Pass/Drop \n"));
    snetCht3CncCount(devObjPtr, descrPtr,
             SNET_CNC_CLIENT_INGRESS_VLAN_PASS_DROP_E,
             dropBit); /*give the CNC unit indication about pass/drop */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* PACKET TYPE Pass/Drop CNC Trigger */
        __LOG(("check for CNC trigger client : PACKET TYPE Pass/Drop \n"));
        snetCht3CncCount(devObjPtr, descrPtr,
                SNET_CNC_CLIENT_PACKET_TYPE_PASS_DROP_E,
                dropBit); /*give the CNC unit indication about pass/drop */
    }
}

/*******************************************************************************
* snetEqTablesFormatInit
*
* DESCRIPTION:
*        init the format of EQ tables.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetEqTablesFormatInit(
    IN SKERNEL_DEVICE_OBJECT            * devObjPtr
)
{

    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_E2PHY_E,
            sip5_20EqE2PhyTableFieldsFormat, lion3EqE2PhyFieldsTableNames);
    }
    else
    {
        LION3_TABLES_FORMAT_INIT_MAC(
            devObjPtr, SKERNEL_TABLE_FORMAT_E2PHY_E,
            lion3EqE2PhyTableFieldsFormat, lion3EqE2PhyFieldsTableNames);
    }
}

