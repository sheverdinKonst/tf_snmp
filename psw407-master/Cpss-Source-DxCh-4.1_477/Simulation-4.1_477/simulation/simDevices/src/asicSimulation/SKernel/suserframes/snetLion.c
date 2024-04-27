/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion.c
*
* DESCRIPTION:
*       This is a external API definition for Lion frame processing.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 70 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <common/Utils/Math/sMath.h>
#include <asicSimulation/SKernel/suserframes/snetLion.h>
#include <asicSimulation/SKernel/suserframes/snetXCat.h>
#include <asicSimulation/SLog/simLog.h>

/* Get current TOD clock value */
#define SNET_LION_TOD_CLOCK_GET_MAC(dev, direction) \
    SIM_OS_MAC(simOsTickGet)() - (dev)->todTimeStampClockDiff[(direction)]
#define SNET_LION3_TOD_CLOCK_GET_MAC(dev, group, inst) \
    SIM_OS_MAC(simOsTickGet)() - (dev)->eTodTimeStampClockDiff[(group)][(inst)]

/* Incoming packet from the port group local <port> is subject to multi-port group lookup */
#define SNET_LION_GET_LOCAL_PORT_MULTI_PORT_GROUP_LOOKUP_EN_MAC(data_ptr, port) \
    SMEM_U32_GET_FIELD((data_ptr)[0], (port), 1)

/* Packet entering the device through <port group> has no DA match in this port group
   is redirected to the next ring port, or forwarded normally */
#define SNET_LION_GET_SRC_PORT_GROUP_UNKNOWN_DA_CMD_MAC(data_ptr, port_group) \
    SMEM_U32_GET_FIELD((data_ptr)[0], 16 + (port_group), 1)

/* This flag determines whether the next ring port is a trunk.
   For packets that enter the device through <port group> */
#define SNET_LION_GET_PORT_GROUP_NEXT_RING_IS_TRUNK_MAC(data_ptr, port_group) \
    SMEM_U32_GET_FIELD((data_ptr)[(((port_group) / 2) + 1)], 13 * ((port_group) % 2), 1)

/* The port or trunk number of the next ring interface.
   For packets that enter the device through <port group> */
#define SNET_LION_GET_PORT_GROUP_NEXT_RING_PORT_TRUNK_MAC(data_ptr, port_group) \
    SMEM_U32_GET_FIELD((data_ptr)[(((port_group) / 2) + 1)], ((13 * ((port_group) % 2)) + 1), 7)

/* When multi-port group FDB/TTI is activated, the bits corresponding to ring ports
   should be enabled */
#define SNET_LION_GET_PORT_GROUP_RING_PORT_EN_MAC(data_ptr, port) \
    SMEM_U32_GET_FIELD((data_ptr)[((port) / 32)], ((port) % 32), 1)

/* Mask UnKnown DA Enable Port <Group> */
#define SNET_LION_GET_PORT_GROUP_MASK_UNKNOWN_DA_CMD_MAC(data_ptr, port_group) \
    SMEM_U32_GET_FIELD((data_ptr)[0], 16 + (port_group), 1)

#define SNET_XCAT_TTI_ACTION_MAC(action_ptr) \
    ((SNET_XCAT_TT_ACTION_STC *)action_ptr)

#define SNET_LION_TTI_ACTION_MAC(action_ptr) \
    ((SNET_LION_TT_ACTION_STC *)action_ptr)

/* Array holds the info about the CRC hash input fields */
static CHT_PCL_KEY_FIELDS_INFO_STC lionCrcHashKeyFields[SNET_LION_CRC_HASH_LAST_E]=
{
        { 0  ,15 , GT_FALSE, " SNET_LION_CRC_HASH_L4_TARGET_PORT_E "    },
        {16  ,31 , GT_FALSE, " SNET_LION_CRC_HASH_L4_SOURCE_PORT_E "    },
        {32  ,51 , GT_FALSE, " SNET_LION_CRC_HASH_IPV6_FLOW_E "         },
        {52  ,55 , GT_FALSE, " SNET_LION_CRC_HASH_RESERVED_55_52_E "    },

        {56  ,87  , GT_FALSE, " SNET_LION_CRC_HASH_IP_DIP_3_E "         },
        {88  ,119 , GT_FALSE, " SNET_LION_CRC_HASH_IP_DIP_2_E "         },
        {120 ,151 , GT_FALSE, " SNET_LION_CRC_HASH_IP_DIP_1_E "         },
        {152 ,183 , GT_FALSE, " SNET_LION_CRC_HASH_IP_DIP_0_E "         },

        {184 ,215, GT_FALSE, " SNET_LION_CRC_HASH_IP_SIP_3_E "          },
        {216 ,247, GT_FALSE, " SNET_LION_CRC_HASH_IP_SIP_2_E "          },
        {248 ,279, GT_FALSE, " SNET_LION_CRC_HASH_IP_SIP_1_E "          },
        {280 ,311, GT_FALSE, " SNET_LION_CRC_HASH_IP_SIP_0_E "          },

        {312 ,359, GT_FALSE, " SNET_LION_CRC_HASH_MAC_DA_E "            },
        {360 ,407, GT_FALSE, " SNET_LION_CRC_HASH_MAC_SA_E "            },
        {408 ,427, GT_FALSE, " SNET_LION_CRC_HASH_MPLS_LABEL0_E "       },
        {428 ,431, GT_FALSE, " SNET_LION_CRC_HASH_RESERVED_431_428_E "  },
        {432 ,451, GT_FALSE, " SNET_LION_CRC_HASH_MPLS_LABEL1_E "       },
        {452 ,455, GT_FALSE, " SNET_LION_CRC_HASH_RESERVED_455_452_E "  },
        {456 ,475, GT_FALSE, " SNET_LION_CRC_HASH_MPLS_LABEL2_E "       },
        {476 ,479, GT_FALSE, " SNET_LION_CRC_HASH_RESERVED_479_476_E "  },
        {480 ,487, GT_FALSE, " SNET_LION_CRC_HASH_LOCAL_SOURCE_PORT_E " },
        {488 ,559, GT_FALSE, " SNET_LION_CRC_HASH_UDB_14_TO_22_E "      },

        /* lion3 'overlapping' fields'*/
        { 88 ,183, GT_FALSE, " SNET_LION3_CRC_HASH_UDB_0_TO_11_E "       },
        {216 ,311, GT_FALSE, " SNET_LION3_CRC_HASH_UDB_23_TO_34_E "      },

        /*same as SNET_LION_CRC_HASH_MPLS_LABEL0_E*/
        {408 ,427, GT_FALSE, " SNET_LION3_CRC_HASH_EVID_E "              },
        /*same as SNET_LION_CRC_HASH_MPLS_LABEL1_E*/
        {432 ,451, GT_FALSE, " SNET_LION3_CRC_HASH_ORIG_SRC_EPORT_OR_TRNK_E "    },
        /*same as SNET_LION_CRC_HASH_MPLS_LABEL2_E*/
        {456 ,475, GT_FALSE, " SNET_LION3_CRC_HASH_LOCAL_DEV_SRC_EPORT_E "       }


};

typedef enum {
    LION_PTP_GTS_CAUSE_SUM_INTERRUPT_E = 0,
    LION_PTP_GTS_CPU_ADDR_OUT_RANGE_INTERRUPT_E = 1 << 1,
    LION_PTP_GTS_VALID_ENTRY_INTERRUPT_E = 1 << 2,
    LION_PTP_GTS_GLOBAL_FIFO_FULL_INTERRUPT_E = 1 << 3
}LION_PTP_GTS_INTERRUPT_ENT;

typedef enum {
    LION3_PTP_INGRESS_EXCEPTION_MODE_BASIC_E        = 0,
    LION3_PTP_INGRESS_EXCEPTION_MODE_TC_E           = 1,
    LION3_PTP_INGRESS_EXCEPTION_MODE_PIGGY_BACKED_E = 2,
    LION3_PTP_INGRESS_EXCEPTION_MODE_BC_E           = 3
}LION3_PTP_INGRESS_EXCEPTION_MODE_ENT;

typedef enum {
    LION3_UNREG_IPM_EVIDX_MODE_MODE_ALL_E       = 0,
    LION3_UNREG_IPM_EVIDX_MODE_MODE_IPV4_E      = 1,
    LION3_UNREG_IPM_EVIDX_MODE_MODE_IPV6_E      = 2,
    LION3_UNREG_IPM_EVIDX_MODE_MODE_IPV4_IPV6_E = 3
}LION3_UNREG_IPM_EVIDX_MODE_ENT;

/*******************************************************************************
*   snetLionPclUdbKeyValueGet
*
* DESCRIPTION:
*        Get user defined value by user defined key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*       direction           - ingress or egress direction
*       udbIdx              - UDB index in UDB configuration entry.
*
* OUTPUTS:
*       byteValuePtr        - pointer to UDB value.
*
* RETURN:
*       GT_OK - OK
*       GT_FAIL - Not valid byte
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS snetLionPclUdbKeyValueGet
(
    IN SKERNEL_DEVICE_OBJECT                        * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC              * descrPtr,
    IN SMAIN_DIRECTION_ENT                            direction,
    IN GT_U32                                         udbIdx,
    OUT GT_U8                                       * byteValuePtr
)
{
    DECLARE_FUNC_NAME(snetLionPclUdbKeyValueGet);

    GT_U32  regAddr;            /* register address */
    GT_U32  udbData;            /* 11-bit UDB data */
    GT_U32  fieldValue;           /* field value */
    GT_U32  * regDataPtr;       /* register data pointer */
    GT_U32  userDefinedAnchor;  /* user defined byte Anchor */
    GT_U32  userDefinedOffset;  /* user defined byte offset from Anchor */
    GT_STATUS  rc;
    GT_U32  lineIndex;/* index of line in the "IPCL User Defined Bytes Configuration" */
    GT_U32  startBit;/* start bit in the "IPCL User Defined Bytes Configuration" */

    *byteValuePtr = 0;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr) && direction == SMAIN_DIRECTION_EGRESS_E)
    {
        return lion3EPclUdbKeyValueGet(devObjPtr,descrPtr,udbIdx,byteValuePtr);
    }

    lineIndex = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
            descrPtr->pcl_pcktType_sip5 : descrPtr->pcktType;

    startBit = (udbIdx * 11);

    regAddr = SMEM_XCAT_POLICY_UDB_CONF_MEM_REG(devObjPtr, lineIndex);
    regDataPtr = smemMemGet(devObjPtr, regAddr);

    /* get all 11-bit data related to the udbIdx-th UDB */
    udbData = snetFieldValueGet(regDataPtr, startBit, 11);

    /* bit 0 - valid */
    fieldValue = SMEM_U32_GET_FIELD(udbData, 0, 1);
    if (fieldValue == 0)
    {
        __LOG(("ipclUdbPacketType[%d] , UDB[%d] - not valid (in UDB configuration table)\n",
            lineIndex,udbIdx));
        *byteValuePtr = 0;
        return GT_OK;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Use UDB */
        userDefinedAnchor = SMEM_U32_GET_FIELD(udbData, 1, 3);
        userDefinedOffset = SMEM_U32_GET_FIELD(udbData, 4, 7);

        /* "Enable TCP/UDP Comparators" feature is implemented via the Metadata Anchor In SIP5*/
    }
    else
    {
        /* Enable TCP/UDP Comparators */
        fieldValue = SMEM_U32_GET_FIELD(udbData, 10, 1);
        if (fieldValue)
        {
            /* Replace UDB with TCP/UDP Comparators */
            __LOG(("Replace UDB with TCP/UDP Comparators"));
            rc = snetChtIPclTcpUdpPortRangeCompareGet(devObjPtr, descrPtr,
                                                      byteValuePtr);
            if(rc != GT_OK)
            {
                *byteValuePtr = 0;
            }

            return rc;
        }

        /* Use UDB */
        userDefinedAnchor = SMEM_U32_GET_FIELD(udbData, 1, 2);
        userDefinedOffset = SMEM_U32_GET_FIELD(udbData, 3, 7);
    }

    rc = snetXCatPclUserDefinedByteGet(devObjPtr, descrPtr, userDefinedAnchor,
                                      userDefinedOffset, SNET_UDB_CLIENT_IPCL_E, byteValuePtr);
    if(rc != GT_OK)
    {
        *byteValuePtr = 0;
    }

    return rc;
}

/*******************************************************************************
*   snetLionCrcHashKeyFieldBuildByValue
*
* DESCRIPTION:
*       Inserts data of the field to the specific place in CRC hash key
*
* INPUTS:
*       crcHashKeyPtr   - (pointer to) CRC hash key
*       fldValue        - data of field to insert to key
*       fieldId         - field id
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
static GT_VOID snetLionCrcHashKeyFieldBuildByValue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U8 * crcHashKeyPtr,
    IN GT_U32  fldValue,
    IN SNET_LION_CRC_HASH_INPUT_KEY_FIELDS_ID_ENT  fieldId
)
{
    SNET_CHT_POLICY_KEY_STC pclKey;

    pclKey.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    pclKey.updateOnlyDiff = GT_FALSE;
    pclKey.key.unlimitedBufferPtr = crcHashKeyPtr;
    pclKey.devObjPtr = devObjPtr;

    snetChtPclSrvKeyFieldBuildByValue(&pclKey, fldValue, &lionCrcHashKeyFields[fieldId]);
}

/*******************************************************************************
*   snetLionCrcHashKeyFieldBuildByPointer
*
* DESCRIPTION:
*       Inserts data of the field to the specific place in CRC hash key
*
* INPUTS:
*       crcHashKeyPtr   - (pointer to) CRC hash key
*       fldValue        - data of field to insert to key
*       fieldId         - field id
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
static GT_VOID snetLionCrcHashKeyFieldBuildByPointer
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U8 * crcHashKeyPtr,
    IN GT_U8    * fieldValPtr,
    IN SNET_LION_CRC_HASH_INPUT_KEY_FIELDS_ID_ENT  fieldId
)
{
    SNET_CHT_POLICY_KEY_STC pclKey;

    pclKey.pclKeyFormat = CHT_PCL_KEY_UNLIMITED_SIZE_E;
    pclKey.updateOnlyDiff = GT_FALSE;
    pclKey.key.unlimitedBufferPtr = crcHashKeyPtr;
    pclKey.devObjPtr = devObjPtr;

    snetChtPclSrvKeyFieldBuildByPointer(&pclKey, fieldValPtr, &lionCrcHashKeyFields[fieldId]);
}

/*******************************************************************************
*   snetLionCrcHashKeyFieldBuildByPointerWithMask
*
* DESCRIPTION:
*       Inserts data by of the field to the specific place in CRC hash key using mask
*
* INPUTS:
*       crcHashKeyMaskPtr - pointer to memory of start of mask entry (70 bits)
*       crcHashKeyPtr   - (pointer to) CRC hash key
*       fldValue        - data of field to insert to key
*       fieldId         - field id
*
* OUTPUTS:
*       crcHashKeyPtr - (pointer to) CRC hash key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLionCrcHashKeyFieldBuildByPointerWithMask
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * crcHashKeyMaskPtr,
    INOUT GT_U8 * crcHashKeyPtr,
    IN GT_U8    * fieldValPtr,
    IN SNET_LION_CRC_HASH_INPUT_KEY_FIELDS_ID_ENT  fieldId
)
{
    GT_U32  maskBits;/* bits that represent add/skip of representative byte */
    GT_U32  startByte;/* start byte in key */
    GT_U32  numBytes;/* number of bytes in field */
    GT_U32  ii;/*iterator */

    if(lionCrcHashKeyFields[fieldId].startBitInKey % 8)
    {
        skernelFatalError("snetLionCrcHashKeyFieldBuildByPointerWithMask: must be 8 bits aligned \n");
    }

    startByte = lionCrcHashKeyFields[fieldId].startBitInKey / 8;/* bits to bytes */

    numBytes = (lionCrcHashKeyFields[fieldId].endBitInKey -
                lionCrcHashKeyFields[fieldId].startBitInKey + 7) / 8;/* bits to bytes */

    maskBits = snetFieldValueGet(crcHashKeyMaskPtr, startByte, numBytes);

    snetLionCrcHashKeyFieldBuildByPointer(devObjPtr,crcHashKeyPtr, fieldValPtr,
                                          fieldId);
    for(ii = 0; ii < numBytes; ii++)
    {
        if(0 == SMEM_U32_GET_FIELD(maskBits, ii, 1))
        {
            /* this byte will not be in the hash result */
            crcHashKeyPtr[startByte + ii] =  0;
        }
    }
}

/*******************************************************************************
*   snetLionCrcHashKeyFieldBuildByValueWithMask
*
* DESCRIPTION:
*       Inserts data of the field to the specific place in CRC hash key using mask
*
* INPUTS:
*       crcHashKeyMaskPtr - pointer to memory of start of mask entry (70 bits)
*       crcHashKeyPtr   - (pointer to) CRC hash key
*       fldValue        - data of field to insert to key
*       fieldId         - field id
*
* OUTPUTS:
*       crcHashKeyPtr   - (pointer to) CRC hash key
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLionCrcHashKeyFieldBuildByValueWithMask
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * crcHashKeyMaskPtr,
    INOUT GT_U8 * crcHashKeyPtr,
    IN GT_U32  fldValue,
    IN SNET_LION_CRC_HASH_INPUT_KEY_FIELDS_ID_ENT  fieldId
)
{
    GT_U32  maskBits;/* bits that represent add/skip of representative byte */
    GT_U32  startByte;/* start byte in key */
    GT_U32  numBytes;/* number of bytes in field */
    GT_U32  ii;/*iterator */

    if(lionCrcHashKeyFields[fieldId].startBitInKey % 8)
    {
        skernelFatalError("snetLionCrcHashKeyFieldBuildByValueWithMask: must be 8 bits aligned \n");
    }

    startByte = lionCrcHashKeyFields[fieldId].startBitInKey / 8;/* bits to bytes */

    numBytes = (lionCrcHashKeyFields[fieldId].endBitInKey -
                lionCrcHashKeyFields[fieldId].startBitInKey + 7) / 8;/* bits to bytes */

    maskBits = snetFieldValueGet(crcHashKeyMaskPtr, startByte, numBytes);

    snetLionCrcHashKeyFieldBuildByValue(devObjPtr,crcHashKeyPtr, fldValue, fieldId);

    for(ii = 0 ; ii <numBytes ; ii ++)
    {
        if(0 == SMEM_U32_GET_FIELD(maskBits,ii,1))
        {
            /* this byte will not be in the hash result */
            crcHashKeyPtr[startByte + ii] =  0;
        }
    }
}


/*******************************************************************************
*   snetXCatIPclBuildBaseCommonKey
*
* DESCRIPTION:
*       Build first common 42 bits of PCL search key.
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to frame data buffer Id.
*
* OUTPUTS:
*       crcHashKeyPtr        - pointer to the CRC hash key.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLionCrcHashBuildKey
(
    IN SKERNEL_DEVICE_OBJECT    * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC  * descrPtr,
    OUT GT_U8   * crcHashKeyPtr
)
{
    DECLARE_FUNC_NAME(snetLionCrcHashBuildKey);

    GT_U32 regAddr;                     /* Register's address */
    GT_U32 *regPtr;                     /* Register entry pointer */
    GT_U32 fieldVal;                    /* Register's field value */
    GT_U32 regValue;                    /* Register's value */
    GT_U32 entryIndex;                  /* Table entry index */
    GT_U32 * crcHashKeyMaskPtr;         /* CRC hash key mask pointer */
    GT_U8 tmpFieldVal[50];              /* UDB bytes array */
    GT_U32 l4SrcPort;                   /* L4 Source Port */
    GT_U32 l4DstPort;                   /* L4 Destination Port */
    GT_U32 i;
    GT_U32 hashFlowIDEnable,hashLocalSourceEPortEnable,hashOriginalSourceEPortEnable;/*global hash modes*/
    GT_U32 hashIPv6UseFullAddrEnable,hashEVlanEnable;/*global hash modes*/
    GT_U32 hashEVlanForce=0,hashOriginalSourceEPortForce=0,hashLocalSourceEPortForce=0,hashUdbsInsteadOfIpv6Force=0;
    GT_U32 symmetricMac,symmetricIpv4,symmetricIpv6,symmetricL4Port;/*per hash mask entry hash modes*/
    GT_U32 udbId;/* udb id */
    GT_U8  symmetricTempArr[12];/*array for symmetric hash*/
    static GT_U8 symmetricZeroTempArr[12] = {0,0,0,0,0,0,0,0,0,0,0,0};/*12 bytes for ZERO array */
    GT_U32  byteIndex;
    GT_U32  packetType;/* packet type */
    GT_U32  maxIpv6Iterator;
    GT_BIT  isIpv6;

    isIpv6 = ((descrPtr->isIp == 0) || descrPtr->isIPv4 || descrPtr->isFcoe) ? 0 : 1;

    packetType = SMEM_CHT_IS_SIP5_GET(devObjPtr) ?
            descrPtr->pcl_pcktType_sip5 :
            descrPtr->pcktType ;

                /* Hash mask selection from the TTI is enabled */
    if (descrPtr->ttiHashMaskIndex)
    {
        /* TTI based mask selection */
        entryIndex = descrPtr->ttiHashMaskIndex;
        __LOG(("TTI based mask selection index[%d] \n",
            entryIndex));
    }
    else
    {
        if(descrPtr->eArchExtInfo.ttiPostTtiLookupIngressEPortTablePtr)
        {
            fieldVal =
                SMEM_LION3_TTI_EPORT_ATTRIBUTES_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                    SMEM_LION3_TTI_EPORT_ATTRIBUTES_TABLE_FIELDS_OVERRIDE_MASK_HASH_EN);
            /* Override Mask Hash Enable */
            if(fieldVal)
            {
                /* Port based mask selection */
                entryIndex =
                    SMEM_LION3_TTI_EPORT_ATTRIBUTES_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                        SMEM_LION3_TTI_EPORT_ATTRIBUTES_TABLE_FIELDS_HASH_MASK_INDEX);

                __LOG(("Port based mask selection index[%d] \n",
                    entryIndex));
            }
            else
            {
                /* Packet type based mask selection */
                entryIndex = 16/*4 bits*/ + packetType;
                __LOG(("Packet type based mask selection index[%d] \n",
                    entryIndex));
            }
        }
        else
        {
            regAddr =
                SMEM_CHT_PORT_VLAN_QOS_CONFIG_TBL_MEM(devObjPtr, descrPtr->localDevSrcPort);
            regPtr = smemMemGet(devObjPtr, regAddr);

            fieldVal = SMEM_U32_GET_FIELD(regPtr[2], 2, 1);
            /* Override Mask Hash Enable */
            if(fieldVal)
            {
                /* Port based mask selection */
                entryIndex = SMEM_U32_GET_FIELD(regPtr[2], 3, 4);
                __LOG(("Port based mask selection index[%d] \n",
                    entryIndex));
            }
            else
            {
                /* Packet type based mask selection */
                entryIndex = 16 + packetType;
                __LOG(("Packet type based mask selection index[%d] \n",
                    entryIndex));
            }
        }
    }

    __LOG(("Start Calculating the 70 bytes that needed for the CRC (6/16/32) modes \n"));

    regAddr = SMEM_LION_PCL_CRC_HASH_MASK_TBL_MEM(devObjPtr, entryIndex);
    crcHashKeyMaskPtr = smemMemGet(devObjPtr, regAddr);

    if(simLogIsOpen())
    {
        __LOG(("next bytes are used by the CRC : \n"));
        /* state which of the 70 bytes are used (represented by bytes)*/
        for(i = 0 ; i < SNET_LION_CRC_HEADER_HASH_BYTES_CNS ; i++)
        {
            if(snetFieldValueGet(crcHashKeyMaskPtr,i,1))
            {
                __LOG(("%d ,"
                ,i));
            }
        }
        __LOG(("\n"));
    }


    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION_PCL_CRC_HASH_CONF_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr , &regValue);
        /*Determines whether the 70B hash key includes the FlowID, or the UDB21-UDB22*/
        hashFlowIDEnable = SMEM_U32_GET_FIELD(regValue, 2, 1);
        /*Determines whether the 70B hash key in CRC-based hash uses the original source ePort field or MPLS Label 1.*/
        hashOriginalSourceEPortEnable = SMEM_U32_GET_FIELD(regValue, 4, 1);
        /*Determines whether the 70B hash key in CRC-based hash uses the local source ePort (i.e., the source ePort in the current device) or MPLS Label 2.*/
        hashLocalSourceEPortEnable = SMEM_U32_GET_FIELD(regValue, 5, 1);
        /*Determines whether the 70B hash key includes the IPv6 full addresses.
        When disabled, the hash key includes the 32 lsbits of the IP addresses, and additional user defined bytes.*/
        hashIPv6UseFullAddrEnable = SMEM_U32_GET_FIELD(regValue, 6, 1);
        /*Determines whether the 70B hash key includes the eVLAN, or the MPLS Label 0*/
        hashEVlanEnable = SMEM_U32_GET_FIELD(regValue, 7, 1);

        /*Enables overriding of MAC DA and MAC SA fields in the key.
        This action is performed prior to masking of the fields
         0x0 = Disable; Disable; Key contents is not changed;
         0x1 = Enable Symmetric MAC; Enable_Symmetric_MAC;
            The MAC DA field in the key will contain the byte by byte sum of MAC DA and MAC SA.
            The MAC SA field in the key will contain 0.; */
        symmetricMac = snetFieldValueGet(crcHashKeyMaskPtr,70,1);
        /*Enables overriding of IP DIP and IP SIP fields in the key.
            This action is performed prior to masking of the fields.
             0x0 = Disable; Disable; Key contents is not changed;
             0x1 = Enable Symmetric IP; Enable_Symmetric_IP;
             The 4 LSB of the IP DIP field in the key will contain the byte by byte sum of the 4 LSB IP DIP and IP SIP.
             The 4 LSB of the IP SIP field in the key will contain 0.; */
        symmetricIpv4 = snetFieldValueGet(crcHashKeyMaskPtr,71,1);
        /*Enables overriding of IP DIP and IP SIP fields in the key.
            This action is performed prior to masking of the fields
             0x0 = Disable; Disable; Key contents is not changed;
             0x1 = Enable Symmetric IP; Enable_Symmetric_IP;
             The 12 MSB of the IP DIP field in the key will contain the byte by byte sum of the 12 MSB IP DIP and IP SIP.
             The 12 MSB IP of the SIP field in the key will contain 0; */
        symmetricIpv6 = snetFieldValueGet(crcHashKeyMaskPtr,72,1);
        /*Enables overriding of L4 Target Port and L4 Source Port fields in the key.
            This action is performed prior to masking of the fields
             0x0 = Disable; Disable; Key contents is not changed;
             0x1 = Enable Symmetric Port; Enable_Symmetric_Port;
             The L4 Target Port field in the key will contain the byte by byte sum of L4 Target Port and L4 Source Port.
             The L4 Source Port field in the key will contain 0; */
        symmetricL4Port = snetFieldValueGet(crcHashKeyMaskPtr,73,1);


        __LOG_PARAM(hashFlowIDEnable);
        __LOG_PARAM(hashOriginalSourceEPortEnable);
        __LOG_PARAM(hashLocalSourceEPortEnable);
        __LOG_PARAM(hashIPv6UseFullAddrEnable);
        __LOG_PARAM(hashEVlanEnable);
        __LOG_PARAM(symmetricMac);
        __LOG_PARAM(symmetricIpv4);
        __LOG_PARAM(symmetricIpv6);
        __LOG_PARAM(symmetricL4Port);

        if(descrPtr->mpls == 0)
        {
            __LOG(("for non-MPLS use EVLAN ,OriginalSourceEPort ,LocalSourceEPort \n"));
            hashEVlanForce = 1;
            hashOriginalSourceEPortForce = 1;
            hashLocalSourceEPortForce = 1;
        }

        if(isIpv6 == 0)
        {
            __LOG(("for non-IPv6 use 24UDBs \n"));
            hashUdbsInsteadOfIpv6Force = 1;
        }
    }
    else
    {
        /* making the code generic */
        hashFlowIDEnable = 0;
        hashOriginalSourceEPortEnable = 0;
        hashLocalSourceEPortEnable = 0;
        hashIPv6UseFullAddrEnable = 1;
        hashEVlanEnable = 0;
        symmetricMac = 0;
        symmetricIpv4 = 0;
        symmetricIpv6 = 0;
        symmetricL4Port = 0;
    }


    if(symmetricMac)
    {
        for(byteIndex=0 ; byteIndex < 6 ; byteIndex++)
        {
            symmetricTempArr[byteIndex] = descrPtr->macDaPtr[byteIndex] + descrPtr->macSaPtr[byteIndex];
        }

        /* MAC DA field in the key will contain the byte by byte sum of MAC DA and MAC SA */
        snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                      symmetricTempArr,
                                              SNET_LION_CRC_HASH_MAC_DA_E);
        /* The MAC SA field in the key will contain 0 */
        snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr,crcHashKeyPtr,
                                                      symmetricZeroTempArr,
                                              SNET_LION_CRC_HASH_MAC_SA_E);
    }
    else
    {
        /* MAC DA */
        snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                      descrPtr->macDaPtr,
                                              SNET_LION_CRC_HASH_MAC_DA_E);
        /* MAC SA */
        snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr,crcHashKeyPtr,
                                                      descrPtr->macSaPtr,
                                              SNET_LION_CRC_HASH_MAC_SA_E);
    }
    /* Local Source Port */
    snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,crcHashKeyPtr,
                                                descrPtr->localDevSrcPort,
                                        SNET_LION_CRC_HASH_LOCAL_SOURCE_PORT_E);


    for(i = 0,udbId = 14; i < 9; i++,udbId++)
    {
        if(hashFlowIDEnable)
        {
            if(udbId == 21)
            {
                tmpFieldVal[8 - i] = (GT_U8)descrPtr->flowId;
                __LOG(("use descrPtr->flowId 8 LSBits [0x%x] instead of UDB21 \n",
                    tmpFieldVal[8 - i]));
                continue;
            }
            else if (udbId == 22)
            {
                tmpFieldVal[8 - i] = (GT_U8)(descrPtr->flowId >> 8);
                __LOG(("use descrPtr->flowId MSBits [0x%x] instead of UDB22 \n",
                    tmpFieldVal[8 - i]));
                continue;
            }
        }

        snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, SMAIN_DIRECTION_INGRESS_E,
                                  udbId, &tmpFieldVal[8 - i]);
    }

    snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr,crcHashKeyPtr, tmpFieldVal,
                                          SNET_LION_CRC_HASH_UDB_14_TO_22_E);


    if(descrPtr->isIp || descrPtr->isFcoe)
    {
        if(descrPtr->isIp == 0/*fcoe*/ ||
           descrPtr->isIPv4 == 1/*ipv4*/)
        {
            if(symmetricIpv4)
            {
                for(byteIndex=0 ; byteIndex < 4 ; byteIndex++)
                {
                    symmetricTempArr[byteIndex] =
                            (GT_U8)(descrPtr->dip[0] >> 8*(byteIndex)) +
                            (GT_U8)(descrPtr->sip[0] >> 8*(byteIndex)) ;
                }

                /* DIP[0] */
                __LOG(("DIP[0] (fcoe/ipv4) \n"));
                snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                            symmetricTempArr,
                                                            SNET_LION_CRC_HASH_IP_DIP_3_E);
                /* SIP[0] */
                __LOG(("SIP[0] (fcoe/ipv4) \n"));
                snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                            symmetricZeroTempArr,
                                                            SNET_LION_CRC_HASH_IP_SIP_3_E);
            }
            else
            {
                /* DIP[0] */
                __LOG(("DIP[0] (fcoe/ipv4) \n"));
                snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                            descrPtr->dip[0],
                                                            SNET_LION_CRC_HASH_IP_DIP_3_E);
                /* SIP[0] */
                __LOG(("SIP[0] (fcoe/ipv4) \n"));
                snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                            descrPtr->sip[0],
                                                            SNET_LION_CRC_HASH_IP_SIP_3_E);
            }
        }
        else/*ipv6*/
        {
            if(hashIPv6UseFullAddrEnable)
            {
                maxIpv6Iterator = 4;
            }
            else
            {
                maxIpv6Iterator = 1;
            }

            if(symmetricIpv6)
            {
                for(i = 0; i < maxIpv6Iterator ; i++)
                {
                    /* IPv6 DIP */
                    for(byteIndex=0 ; byteIndex < 4 ; byteIndex++)
                    {
                        symmetricTempArr[byteIndex] =
                                (GT_U8)(descrPtr->dip[3-i] >> 8*(byteIndex)) +
                                (GT_U8)(descrPtr->sip[3-i] >> 8*(byteIndex)) ;
                    }

                    /* DIP[0] */
                    __LOG(("DIP[%d] (ipv6) \n",
                        i));
                    snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                                symmetricTempArr,
                                                                SNET_LION_CRC_HASH_IP_DIP_3_E + i);
                    /* SIP[0] */
                    __LOG(("SIP[%d] (ipv6) \n",
                        i));
                    snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                                symmetricZeroTempArr,
                                                                SNET_LION_CRC_HASH_IP_SIP_3_E + i);
                }
            }
            else
            {
                for(i = 0; i < maxIpv6Iterator ; i++)
                {
                    /* IPv6 DIP */
                    __LOG(("DIP[%d] (ipv6) \n",
                        i));
                    snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                                crcHashKeyPtr,
                                                                descrPtr->dip[3-i],
                                                                SNET_LION_CRC_HASH_IP_DIP_3_E + i);
                    /* IPv6 SIP */
                    __LOG(("SIP[%d] (ipv6) \n",
                        i));
                    snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                                crcHashKeyPtr,
                                                                descrPtr->sip[3-i],
                                                                SNET_LION_CRC_HASH_IP_SIP_3_E + i);
                }
            }

            snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                        crcHashKeyPtr, descrPtr->flowLabel,
                                                SNET_LION_CRC_HASH_IPV6_FLOW_E);

        }

        if (descrPtr->ipProt == SNET_TCP_PROT_E ||
            descrPtr->udpCompatible)
       {
            if(symmetricL4Port)
            {
                for(byteIndex=0 ; byteIndex < 2 ; byteIndex++)
                {
                    symmetricTempArr[byteIndex] =
                            (GT_U8)(descrPtr->l4DstPort >> 8*(byteIndex)) +
                            (GT_U8)(descrPtr->l4SrcPort >> 8*(byteIndex)) ;
                }

                /* Target port */
                __LOG(("L4 Target port \n"));
                snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                            symmetricTempArr,
                                                            SNET_LION_CRC_HASH_L4_TARGET_PORT_E);
                /* Source port */
                __LOG(("L4 Source port \n"));
                snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                            symmetricZeroTempArr,
                                                            SNET_LION_CRC_HASH_L4_SOURCE_PORT_E);
            }
            else
            {
                if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
                {
                    /* the issue of the swapped fields was fixed ! */
                    l4DstPort = descrPtr->l4DstPort;
                    l4SrcPort = descrPtr->l4SrcPort;
                }
                else
                {
                    /* NOTE: the next 2 fields found to be swapped , and not according to
                       the functional spec */
                    l4SrcPort = descrPtr->l4DstPort;
                    l4DstPort = descrPtr->l4SrcPort;
                }


                /* Target port */
                __LOG(("L4 Target port \n"));
                snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                            crcHashKeyPtr,
                                                            l4DstPort,
                                                            SNET_LION_CRC_HASH_L4_TARGET_PORT_E);
                /* Source port */
                __LOG(("L4 Source port \n"));
                snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                            crcHashKeyPtr,
                                                            l4SrcPort,
                                                            SNET_LION_CRC_HASH_L4_SOURCE_PORT_E);
            }
       }

       /* the device wrongly use bytes in TCP/UDP Source/Destination ports
         offsets for hash */
       if (devObjPtr->errata.crcTrunkHashL4InfoInIpv4Other &&
           (packetType == CHT_PACKET_CLASSIFICATION_TYPE_IPV4_OTHER_E) &&
            descrPtr->l4StartOffsetPtr)
       {
            /* NOTE: the next 2 fields found to be swapped , and not according to
               the functional spec */
           l4DstPort = (descrPtr->l4StartOffsetPtr[0] << 8) |
                        (descrPtr->l4StartOffsetPtr[1]);

           l4SrcPort = (descrPtr->l4StartOffsetPtr[2] << 8) |
                        (descrPtr->l4StartOffsetPtr[3]);

           /* Target port */
           __LOG(("IPv4 Other Target port"));
           snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                       crcHashKeyPtr,
                                                       l4DstPort,
                                                       SNET_LION_CRC_HASH_L4_TARGET_PORT_E);
           /* Source port */
           __LOG(("IPv4 Other Source port"));
           snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr,
                                                       crcHashKeyPtr,
                                                       l4SrcPort,
                                                       SNET_LION_CRC_HASH_L4_SOURCE_PORT_E);
       }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        if(hashIPv6UseFullAddrEnable == 0 || hashUdbsInsteadOfIpv6Force)
        {
            for(i = 0,udbId = 0; i < 12; i++,udbId++)
            {
                snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, SMAIN_DIRECTION_INGRESS_E,
                                          udbId, &tmpFieldVal[11 - i]);
            }

            snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr,crcHashKeyPtr, tmpFieldVal,
                                                  SNET_LION3_CRC_HASH_UDB_0_TO_11_E);


            for(i = 0,udbId = 23; i < 12; i++,udbId++)
            {
                snetLionPclUdbKeyValueGet(devObjPtr, descrPtr, SMAIN_DIRECTION_INGRESS_E,
                                          udbId, &tmpFieldVal[11 - i]);
            }

            snetLionCrcHashKeyFieldBuildByPointerWithMask(devObjPtr,crcHashKeyMaskPtr,crcHashKeyPtr, tmpFieldVal,
                                                  SNET_LION3_CRC_HASH_UDB_23_TO_34_E);
        }
    }


    if(hashEVlanEnable || hashEVlanForce)
    {
        __LOG(("use descrPtr->eVid instead of descrPtr->label1 \n"));
        /* <eVLAN> */
        snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                descrPtr->eVid,
                                                SNET_LION3_CRC_HASH_EVID_E);
    }
    else
    if(descrPtr->mpls)
    {
        /* Outer most label extracted from MPLS header */
        snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                descrPtr->label1,
                                                SNET_LION_CRC_HASH_MPLS_LABEL0_E);
    }

    if(hashOriginalSourceEPortEnable || hashOriginalSourceEPortForce)
    {
        /* <original source ePort field> */
        snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                descrPtr->origSrcEPortOrTrnk,
                                                SNET_LION3_CRC_HASH_ORIG_SRC_EPORT_OR_TRNK_E);
    }
    else
    if(descrPtr->mpls)
    {
        /* Second label extracted from MPLS header */
        snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                descrPtr->label2,
                                                SNET_LION_CRC_HASH_MPLS_LABEL1_E);
    }

    if(hashLocalSourceEPortEnable || hashLocalSourceEPortForce)
    {
        /* <local source ePort> */
        snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                descrPtr->eArchExtInfo.localDevSrcEPort,
                                                SNET_LION3_CRC_HASH_LOCAL_DEV_SRC_EPORT_E);
    }
    else
    if(descrPtr->mpls)
    {
        /* Third label extracted from MPLS header. */
        snetLionCrcHashKeyFieldBuildByValueWithMask(devObjPtr,crcHashKeyMaskPtr, crcHashKeyPtr,
                                                descrPtr->label3,
                                                SNET_LION_CRC_HASH_MPLS_LABEL2_E);
    }

}

/*******************************************************************************
*   snetLionEqInterPortGroupRingFrwrd
*
* DESCRIPTION:
*       Perform Inter-Port Group Ring Forwarding
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*
* COMMENTS:
*       A packet is subject to inter-port group ring forwarding
*       if ALL the following conditions are TRUE:
*       - The "Multi-port group Lookup" feature is globally enabled
*       - The Bridge FDB DA lookup did not find a match
*       - If the packet is ingressed on a local port group ring port,
*         the ingress port is configured as a multi-port group ring port.
*         OR
*       - If the packet is ingressed on a local port group network port,
*         the ingress port is enabled for multi-port group lookup.
*
*******************************************************************************/
GT_VOID snetLionEqInterPortGroupRingFrwrd
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionEqInterPortGroupRingFrwrd);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldVal;                    /* register's field value */
    GT_U32 ingressPortEligibleForMultiLookup;/*port Eligible for multi-lookups */

    if(descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_FORWARD_E &&
       descrPtr->packetCmd != SKERNEL_EXT_PKT_CMD_MIRROR_TO_CPU_E)
    {
        return;
    }

    /* MAC table DA lookup success */
    if (descrPtr->egressFilterRegistered)
    {
        /* No port group multi lookup */
        __LOG(("No port group multi lookup"));
        return;
    }

    if(descrPtr->routed)
    {
        return;
    }


    /* Multi-Port Group Lookup0 Registers */
    regAddr = SMEM_LION_MULTI_PORT_GROUP_LOOKUP0_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* If the packet is ingressed on a local port group network port */
    fieldVal =
        SNET_LION_GET_LOCAL_PORT_MULTI_PORT_GROUP_LOOKUP_EN_MAC(regPtr,
                                                                descrPtr->localDevSrcPort);
    ingressPortEligibleForMultiLookup = fieldVal;

    /* Ingress port is disabled for multi-port group lookup */
    __LOG(("Ingress port is disabled for multi-port group lookup"));
    if(ingressPortEligibleForMultiLookup == 0)
    {
        /* No port group multi lookup */
        __LOG(("No port group multi lookup"));
        return;
    }

    /* Packet forwarding decision  */
    fieldVal =
        SNET_LION_GET_SRC_PORT_GROUP_UNKNOWN_DA_CMD_MAC(regPtr, descrPtr->srcCoreId);

    if(fieldVal == 0)
    {
        /* NO_REDIRECT - Last port group in the source-port group path has reached */
        __LOG(("NO_REDIRECT - Last port group in the source-port group path has reached"));
        return;
    }

    descrPtr->useVidx = 0;/* no more 'flooding' */

    /* Next ring port is a trunk/port */
    descrPtr->targetIsTrunk =
        SNET_LION_GET_PORT_GROUP_NEXT_RING_IS_TRUNK_MAC(regPtr, descrPtr->srcCoreId);

    if (descrPtr->targetIsTrunk)
    {
        /* Trunk number of the next ring interface */
        descrPtr->trgTrunkId =
            SNET_LION_GET_PORT_GROUP_NEXT_RING_PORT_TRUNK_MAC(regPtr, descrPtr->srcCoreId);
    }
    else
    {
        /* Port number of the next ring interface */
        descrPtr->trgEPort =
            SNET_LION_GET_PORT_GROUP_NEXT_RING_PORT_TRUNK_MAC(regPtr, descrPtr->srcCoreId);

        /* the lion-b not using the target device from the register */
        descrPtr->trgDev  = descrPtr->ownDev;
    }
}

/*******************************************************************************
*   snetLionL2iPortGroupMaskUnknownDaEnable
*
* DESCRIPTION:
*       Apply/Deny VLAN Unknown/Unregistered commands to unknown/unregistered packets.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
* OUTPUT:
*
* RETURN:
*       GT_TRUE - apply VLAN Unknown/Unregistered commands to unknown/unregistered packets
*       GT_FALSE - deny VLAN Unknown/Unregistered commands to unknown/unregistered packets
*
* COMMENTS:
*       The Bridge engine in each port group supports a set of per-VLAN
*       Unknown/Unregistered commands that can override the default flooding behavior.
*       However, these commands are only applied to unknown/unregistered packets
*       if the bridge engine resides in the last port group of the respective ring.
*
*******************************************************************************/
GT_BOOL snetLionL2iPortGroupMaskUnknownDaEnable
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionL2iPortGroupMaskUnknownDaEnable);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldVal;                    /* register's field value */

    regAddr = SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);
    /* Mask UnKnown DA Enable Port <Group> */
    fieldVal =
        SNET_LION_GET_PORT_GROUP_MASK_UNKNOWN_DA_CMD_MAC(regPtr, devObjPtr->portGroupId);
    if(fieldVal == 0)
    {
        __LOG(("It is the last port group in the inter-port group ring \n"));
        return GT_TRUE;
    }
    else
    {
        __LOG(("It is NOT the last port group in the inter-port group ring \n"));
        return GT_FALSE;
    }
}

/*******************************************************************************
*   snetLionIngressSourcePortGroupIdGet
*
* DESCRIPTION:
*       Assign descriptor source port group ID - port group from which
*       the packet ingress the device
*
* INPUTS:
*       devObjPtr    - pointer to device object
*       descrPtr     - pointer to the frame's descriptor
* OUTPUT:
*
* COMMENTS:
*       Source port group ID may be either the local port group Id (when ingress port is not ring port),
*       or a remote port group Id (when ingress port is ring port and value parsed from the DSA)
*
*       The function should be called straight after DSA tag parsing.
*
*******************************************************************************/
GT_VOID snetLionIngressSourcePortGroupIdGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionIngressSourcePortGroupIdGet);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldVal;                    /* register's field value */

    /* TTI Internal configurations */
    regPtr = smemMemGet(devObjPtr, SMEM_LION_TTI_INTERNAL_CONF_REG(devObjPtr));
    /* Ring Ports enabled ?*/
    if(SMEM_U32_GET_FIELD(regPtr[0], 2, 1))
    {
        if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.tti)
        {
            fieldVal = SMEM_LION3_TTI_PHYSICAL_PORT_ENTRY_FIELD_GET(devObjPtr, descrPtr,
                    SMEM_LION3_TTI_PHYSICAL_PORT_TABLE_FIELDS_PORT_IS_RING_CORE_PORT);
        }
        else
        {
            /* TTI Port Group Ring Port Enable Configuration Register */
            __LOG(("TTI Port Group Ring Port Enable Configuration Register"));
            regAddr = SMEM_LION_TTI_PORT_GROUP_RING_PORT_ENABLE_REG(devObjPtr);
            regPtr = smemMemGet(devObjPtr, regAddr);

            /* Whether the ingress port group's local port is a port group ring enable */
            fieldVal =
                SNET_LION_GET_PORT_GROUP_RING_PORT_EN_MAC(regPtr, descrPtr->localDevSrcPort);
        }

        descrPtr->portIsRingCorePort = fieldVal;
    }

    if (descrPtr->marvellTaggedExtended == SKERNEL_EXT_DSA_TAG_4_WORDS_E &&
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E &&
        descrPtr->portIsRingCorePort)
    {
        /* eDSA<TrgPort>[3:0] */
        descrPtr->srcCoreId =  descrPtr->eArchExtInfo.trgPhyPort & 0xf;
    }
    else
    if (descrPtr->marvellTaggedExtended == SKERNEL_EXT_DSA_TAG_2_WORDS_E &&
        descrPtr->packetCmd == SKERNEL_EXT_PKT_CMD_FORWARD_E &&
        descrPtr->portIsRingCorePort)
    {
        /* Ingress port is local port group ring port from the DSA tag:
           <routed> and <egress filtered registered> bits */
        descrPtr->srcCoreId =
            (descrPtr->routed | descrPtr->egressFilterRegistered << 1);

        if(SKERNEL_IS_LION2_DEV(devObjPtr))
        {
            /* take bit 2 of the core ID from the DSA tag */
            __LOG(("take bit 2 of the core ID from the DSA tag"));
            descrPtr->srcCoreId |= descrPtr->dsaCoreIdBit2 << 2;
        }

        descrPtr->egressFilterRegistered = 0; /* clear bit for further use */
        descrPtr->routed = 0;  /* clear bit for further use */
    }
}

/*******************************************************************************
*   snetLionHaEgressMarvellTagSourcePortGroupId
*
* DESCRIPTION:
*       HA - Overrides DSA tag <routed> and <egress filtered registered> bits with
*       Source port group ID.
*
* INPUTS:
*       devObjPtr   - pointer to device object
*       descrPtr    - pointer to the frame's descriptor
*       egressPort  - cascade port to which traffic forwarded
*       mrvlTagPtr  - pointer to marvell tag
*
* OUTPUT:
*       mrvlTagPtr  - pointer to marvell tag
*
* COMMENTS:
*
*       Source port group ID may be either the local port group Id (when ingress port is not ring port),
*       or a remote port group Id (when ingress port is ring port and value parsed from the DSA)
*       It is passed in the DSA tag <routed> and <egress filtered registered> bits.
*
*       The function should be called straight after Marvell Tag creation.
*******************************************************************************/
GT_VOID snetLionHaEgressMarvellTagSourcePortGroupId
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN  GT_U32  egressPort,
    INOUT GT_U8 * mrvlTagPtr
)
{
    DECLARE_FUNC_NAME(snetLionHaEgressMarvellTagSourcePortGroupId);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldVal;                    /* tag's field value */

    if (descrPtr->outGoingMtagCmd != SKERNEL_MTAG_CMD_FORWARD_E)
    {
        return;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* no multi-core lookup support ! */
        return;
    }

    /* HA Port Group Ring Configuration Register */
    regAddr = SMEM_LION_HA_PORT_GROUP_RING_CONF_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Whether the port group's cascade port is port group ring enable */
    fieldVal = SNET_LION_GET_PORT_GROUP_RING_PORT_EN_MAC(regPtr, egressPort);

    if(fieldVal == 0)
    {
        /* No Inter-port group ring forwarding */
        __LOG(("No Inter-port group ring forwarding"));
        return;
    }

    /* Clear bit 25,28 that used for : 25- Routed and 28- EgressFilter Registered;
       We need to set the descrPtr->srcCoreId  into those 2 bits */
    mrvlTagPtr[4] &= ~((1 << 1) | (1 << 4));

    /* Source port group ID - override any previous decision */
    fieldVal = descrPtr->srcCoreId & 0x1;
    mrvlTagPtr[4] |= fieldVal << 1;     /* DSA word[1], bit[25]&0x1*/

    fieldVal = (descrPtr->srcCoreId >> 1) & 0x1;
    mrvlTagPtr[4] |= fieldVal << 4;     /* DSA word[1], bit[28]&0x1*/

    if(SKERNEL_IS_LION2_DEV(devObjPtr))
    {
        /* Clear bit 17 - reserved;
           We need to set the descrPtr->srcCoreId bit 2 this bit */
        mrvlTagPtr[1] &= ~(1 << 1);

        fieldVal = (descrPtr->srcCoreId >> 2) & 0x1;
        mrvlTagPtr[1] |= fieldVal << 1;     /* DSA word[0], bit[17] */
    }

    __LOG(("Do Inter-port group ring forwarding \n"));
}

/*******************************************************************************
*   snetLionL2iGetIngressVlanInfo
*
* DESCRIPTION:
*       Get Ingress VLAN Info (devObjPtr->txqRevision != 0)
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr     - pointer to the frame's descriptor.
*
* OUTPUT:
*       vlanInfoPtr  - pointer to VLAN info structure
*
*
*******************************************************************************/
GT_VOID snetLionL2iGetIngressVlanInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_CHEETAH_L2I_VLAN_INFO * vlanInfoPtr
)
{
    DECLARE_FUNC_NAME(snetLionL2iGetIngressVlanInfo);

    GT_U32 regAddr;                 /* Register address */
    GT_U32 *regPtr;                 /* Register data pointer */
    GT_U32 fldBit;                  /* Register's field offset */
    GT_U32 * stgEntryPtr;           /* STG table entry pointer pointer */
    GT_U32 * vlanEntryPtr;          /* VLAN table entry pointer pointer */
    SKERNEL_EXT_PACKET_CMD_ENT newCmd;/* new packet command*/
    GT_U32  bridgeVid;      /* Vlan Id for get info (descrPtr->eVid or "1" in Vlan unaware mode) */
    GT_U32  indexingVid;    /* membership & STP table can be accesses according to vid1 */
    GT_U32  vlanPortIndex;/* port index in the vlan entry  */
    GT_U32  stgPortIndex;/* port index in the STG entry */
    GT_U32  stgIndex;/* the STG index from the vlan entry */
    LION3_UNREG_IPM_EVIDX_MODE_ENT unregIpmEvidxMode; /* unregistered IPM eVidx Mode */
    GT_U32  unregIpmEvidx; /* unregistered IPM eVidx*/

    vlanPortIndex = descrPtr->localPortGroupPortAsGlobalDevicePort;
    stgPortIndex  = descrPtr->localPortGroupPortAsGlobalDevicePort;

    if(!devObjPtr->supportEArch)
    {
        vlanPortIndex &= 0x3f;/* the vlan entry support 64 ports  */
        stgPortIndex  &= 0x3f;/* the STG  entry support 64 ports  */
    }

    /* Clear vlan entry structure */
    memset(vlanInfoPtr, 0 , sizeof(SNET_CHEETAH_L2I_VLAN_INFO));

    /* BasicMode */
    if (descrPtr->basicMode)
    {
        /* 802.1d bridge */
        bridgeVid = 1;
    }
    else
    {
        bridgeVid = descrPtr->eVid;
    }

    /* Get VLAN entry according to vid */
    regAddr = SMEM_CHT_VLAN_TBL_MEM(devObjPtr, bridgeVid);
    vlanEntryPtr = smemMemGet(devObjPtr, regAddr);

    /* VLAN validation */
    /* 1 - VLAN valid bit is check */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr), 2, 1, &fldBit);
    }
    else
    {
        smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF0_REG(devObjPtr), 4, 1, &fldBit);
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        vlanInfoPtr->valid =  (fldBit == 0) ?
            1 :
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VALID);

        vlanInfoPtr->unknownIsNonSecurityEvent =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NEW_SRC_ADDR_IS_NOT_SECURITY_BREACH);

        indexingVid = bridgeVid;
        if( 0 == descrPtr->basicMode )
        {
            /* <UseVid1ForPortMembership> */
            smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr), 26, 1, &fldBit);
            if( fldBit )
            {
                indexingVid = descrPtr->vid1;
                __LOG(("Accessing ingress eVlan membership table with vid1 [%d] indexing\n" , indexingVid));
            }
            else
            {
                __LOG(("Accessing ingress eVlan membership table with eVlan [%d] indexing\n" , indexingVid));
            }
        }

        if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
        {
            /* 2 vlans of 256 ports in single LINE */
            vlanPortIndex += 256 * (indexingVid & 1);
            indexingVid = indexingVid / 2 ;
        }

        __LOG(("NOTE: accessing to 'Ingress Bridge Port Membership Table' line number according to index : [%d] , for port membership bit index [%d] \n",
            indexingVid,
            vlanPortIndex));

        regAddr = SMEM_LION2_BRIDGE_INGRESS_PORT_MEMBERSHIP_TBL_MEM(devObjPtr,indexingVid);
        regPtr = smemMemGet(devObjPtr, regAddr);

        vlanInfoPtr->portIsMember = snetFieldValueGet(regPtr,vlanPortIndex,1);
        __LOG(("vlanInfoPtr->portIsMember = [%d] \n" , vlanInfoPtr->portIsMember));


        vlanInfoPtr->unregNonIpMcastCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IP_MULTICAST_CMD);
        vlanInfoPtr->unregIPv4McastCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_MULTICAST_CMD);
        vlanInfoPtr->unregIPv6McastCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV6_MULTICAST_CMD);
        vlanInfoPtr->unknownUcastCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKNOWN_UNICAST_CMD);
        vlanInfoPtr->igmpTrapEnable =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IGMP_TO_CPU_EN);
        vlanInfoPtr->ipv4IpmBrgMode =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_MODE);
        vlanInfoPtr->ipv6IpmBrgMode =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_MODE);
        vlanInfoPtr->analyzerIndex =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MIRROR_TO_INGRESS_ANALYZER);
        vlanInfoPtr->icmpIpv6TrapMirror =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_ICMP_TO_CPU_EN);
        vlanInfoPtr->ipInterfaceEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_CONTROL_TO_CPU_EN);
        vlanInfoPtr->ipv4IpmBrgEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_IPM_BRIDGING_EN);
        vlanInfoPtr->ipv6IpmBrgEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_IPM_BRIDGING_EN);
        vlanInfoPtr->unregIp4BcastCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_IPV4_BC_CMD);
        vlanInfoPtr->unregNonIp4BcastCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREGISTERED_NON_IPV4_BC_CMD);
        vlanInfoPtr->ipv4UcRoutEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_UNICAST_ROUTE_EN);
        vlanInfoPtr->ipv4McRoutEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MULTICAST_ROUTE_EN);
        vlanInfoPtr->ipv6UcRoutEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_UNICAST_ROUTE_EN);
        vlanInfoPtr->ipv6McRoutEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MULTICAST_ROUTE_EN);
        vlanInfoPtr->ipV6SiteID =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_SITEID);
        vlanInfoPtr->autoLearnDisable =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_AUTO_LEARN_DIS);
        vlanInfoPtr->naMsgToCpuEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_NA_MSG_TO_CPU_EN);
        vlanInfoPtr->mruIndex =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_MRU_INDEX);
        vlanInfoPtr->bcUdpTrapMirrorEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_BC_UDP_TRAP_MIRROR_EN);
        vlanInfoPtr->ipV6InterfaceEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_CONTROL_TO_CPU_EN);
        vlanInfoPtr->floodVidx =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_EVIDX);
        /* eVIDX for unregisterged IPM */
        if( (descrPtr->ipm == 1) && (descrPtr->isIp == 1) && (descrPtr->egressFilterRegistered == 0) )
        {
            unregIpmEvidx =
                SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                    SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX);
            if( devObjPtr->errata.unregIpmEvidxValue0 || (unregIpmEvidx != 0) )
            {
                unregIpmEvidxMode =
                    SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                        SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNREG_IPM_EVIDX_MODE);

                __LOG(("unregIpmEvidxMode = [%d], unregIpmEvidx = [%d] \n" ,
                       unregIpmEvidxMode, unregIpmEvidx));

                switch(unregIpmEvidxMode)
                {
                    case LION3_UNREG_IPM_EVIDX_MODE_MODE_ALL_E:
                        vlanInfoPtr->floodVidx = unregIpmEvidx;
                        break;
                    case LION3_UNREG_IPM_EVIDX_MODE_MODE_IPV4_E:
                        if( descrPtr->isIPv4 == 1 ) /* IPv4 */
                        {
                            vlanInfoPtr->floodVidx = unregIpmEvidx;
                        }
                        break;
                    case LION3_UNREG_IPM_EVIDX_MODE_MODE_IPV6_E:
                        if( descrPtr->isIPv4 == 0 ) /* IPv6 */
                        {
                            vlanInfoPtr->floodVidx = unregIpmEvidx;
                        }
                        break;
                    case LION3_UNREG_IPM_EVIDX_MODE_MODE_IPV4_IPV6_E:
                        if( descrPtr->isIPv4 == 1 ) /* IPv4 */
                        {
                            vlanInfoPtr->floodVidx = unregIpmEvidx;
                        }
                        else /* Ipv6 */
                        {
                            vlanInfoPtr->floodVidx = unregIpmEvidx+1;
                        }
                        break;
                    default:
                        break;
                }

            }
            else
            {
                __LOG(("unregIpmEvidx = 0 \n"));
            }
        }

        vlanInfoPtr->vrfId  =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_VRF_ID);
        vlanInfoPtr->ucLocalEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UC_LOCAL_EN);
        vlanInfoPtr->floodVidxMode =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FLOOD_VIDX_MODE);

        indexingVid = bridgeVid;
        if( 0 == descrPtr->basicMode )
        {
            /* <UseVid1ForSSGIndex> */
            smemRegFldGet(devObjPtr, SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr), 27, 1, &fldBit);
            if( fldBit )
            {
                indexingVid = descrPtr->vid1;
                __LOG(("Accessing ingress eVlan span state group index table with vid1 [%d] indexing\n" , indexingVid));
            }
            else
            {
                __LOG(("Accessing ingress eVlan span state group index table with eVlan [%d] indexing\n" , indexingVid));
            }
        }

        /* SpanState-GroupIndex */
        regAddr = SMEM_LION2_BRIDGE_INGRESS_SPAN_STATE_GROUP_INDEX_TBL_MEM(devObjPtr, indexingVid);
        regPtr = smemMemGet(devObjPtr, regAddr);

        stgIndex = snetFieldValueGet(regPtr, 0, 12);
        __LOG(("stgIndex = [%d] \n" , stgIndex));


        vlanInfoPtr->ipv4McBcMirrorToAnalyzerIndex =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV4_MC_BC_TO_MIRROR_ANLYZER_IDX);
        vlanInfoPtr->ipv6McMirrorToAnalyzerIndex =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_IPV6_MC_TO_MIRROR_ANALYZER_IDX);
        vlanInfoPtr->fid =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FID);
        vlanInfoPtr->unknownSaCmd =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_UNKOWN_MAC_SA_CMD);
        descrPtr->fcoeInfo.fcoeFwdEn =
            SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FCOE_FORWARDING_EN);

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
        {
            vlanInfoPtr->fdbLookupKeyMode =
                SMEM_LION3_L2I_INGRESS_VLAN_ENTRY_FIELD_GET(devObjPtr,descrPtr,vlanEntryPtr,bridgeVid,
                    SMEM_LION3_L2I_INGRESS_VLAN_TABLE_FIELDS_FDB_LOOKUP_KEY_MODE);
        }
    }
    else
    {
        vlanInfoPtr->valid = (fldBit == 0) ?
            1 : snetFieldValueGet(vlanEntryPtr, 0, 1);

        /* get all the next fields from the vlan even if vlan not valid */

        /* SET vlanInfoPtr according to VLAN entry and STG entry */
        vlanInfoPtr->unknownIsNonSecurityEvent =
            snetFieldValueGet(vlanEntryPtr, 1, 1);

        /* Get membership of source port */
        vlanInfoPtr->portIsMember =
            snetFieldValueGet(vlanEntryPtr, 2 + vlanPortIndex, 1);

        vlanInfoPtr->unregNonIpMcastCmd =
            snetFieldValueGet(vlanEntryPtr, 66, 3);
        vlanInfoPtr->unregIPv4McastCmd =
            snetFieldValueGet(vlanEntryPtr, 69, 3);
        vlanInfoPtr->unregIPv6McastCmd =
            snetFieldValueGet(vlanEntryPtr, 72, 3);
        vlanInfoPtr->unknownUcastCmd =
            snetFieldValueGet(vlanEntryPtr, 75, 3);
        vlanInfoPtr->igmpTrapEnable =
            snetFieldValueGet(vlanEntryPtr, 78, 1);
        vlanInfoPtr->ipv4IpmBrgMode =
            snetFieldValueGet(vlanEntryPtr, 79, 1);
        vlanInfoPtr->ipv6IpmBrgMode =
            snetFieldValueGet(vlanEntryPtr, 80, 1);
        vlanInfoPtr->ingressMirror =
            snetFieldValueGet(vlanEntryPtr, 81, 1);
        vlanInfoPtr->icmpIpv6TrapMirror =
            snetFieldValueGet(vlanEntryPtr, 82, 1);
        vlanInfoPtr->ipInterfaceEn =
            snetFieldValueGet(vlanEntryPtr, 83, 1);
        vlanInfoPtr->ipv4IpmBrgEn =
            snetFieldValueGet(vlanEntryPtr, 84, 1);
        vlanInfoPtr->ipv6IpmBrgEn =
            snetFieldValueGet(vlanEntryPtr, 85, 1);
        vlanInfoPtr->unregIp4BcastCmd =
            snetFieldValueGet(vlanEntryPtr, 86, 3);
        vlanInfoPtr->unregNonIp4BcastCmd =
            snetFieldValueGet(vlanEntryPtr, 89, 3);
        vlanInfoPtr->ipv4UcRoutEn =
            snetFieldValueGet(vlanEntryPtr, 92, 1);
        vlanInfoPtr->ipv4McRoutEn =
            snetFieldValueGet(vlanEntryPtr, 93, 1);
        vlanInfoPtr->ipv6UcRoutEn =
            snetFieldValueGet(vlanEntryPtr, 94, 1);
        vlanInfoPtr->ipv6McRoutEn =
            snetFieldValueGet(vlanEntryPtr, 95, 1);
        vlanInfoPtr->ipV6SiteID =
            snetFieldValueGet(vlanEntryPtr, 96, 1);
        vlanInfoPtr->autoLearnDisable =
            snetFieldValueGet(vlanEntryPtr, 97, 1);
        vlanInfoPtr->naMsgToCpuEn =
            snetFieldValueGet(vlanEntryPtr, 98, 1);
        vlanInfoPtr->mruIndex =
            snetFieldValueGet(vlanEntryPtr, 99, 3);
        vlanInfoPtr->bcUdpTrapMirrorEn =
            snetFieldValueGet(vlanEntryPtr, 102, 1);
        vlanInfoPtr->ipV6InterfaceEn =
            snetFieldValueGet(vlanEntryPtr, 103, 1);
        vlanInfoPtr->floodVidx =
            snetFieldValueGet(vlanEntryPtr, 104, 12);

        vlanInfoPtr->vrfId  =
            snetFieldValueGet(vlanEntryPtr, 116, 12);

        vlanInfoPtr->ucLocalEn =
            snetFieldValueGet(vlanEntryPtr, 128, 1);
        vlanInfoPtr->floodVidxMode =
            snetFieldValueGet(vlanEntryPtr, 129, 1);

        /* SpanState-GroupIndex */
        stgIndex = snetFieldValueGet(vlanEntryPtr, 130, 8);

        vlanInfoPtr->unknownSaCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E; /* to make code generic */
    }

    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        /* 2 STGs of 256 ports (2 bits per port) in single LINE */
        stgPortIndex += 256 * (stgIndex & 1);
        stgIndex = stgIndex / 2 ;
    }

    __LOG(("NOTE: accessing to 'Ingress Bridge Spanning Tree State Table' line number according to index : [%d] , for STP of port index [%d] \n",
        stgIndex,
        stgPortIndex));

    /* Get STG entry according to VLAN entry */
    regAddr = SMEM_CHT_STP_TBL_MEM(devObjPtr, stgIndex);
    stgEntryPtr = smemMemGet(devObjPtr, regAddr);

    /* Get STP state  of source port */
    vlanInfoPtr->spanState =
        snetFieldValueGet(stgEntryPtr, 2 * stgPortIndex, 2);

    if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
    {
        /*Ingress ePort STP state Mode*/
        fldBit =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_STP_STATE_MODE);
        if(fldBit == 1)
        {
            /*Ingress ePort spanning tree state*/
            fldBit =
                SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                    SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_EPORT_SPANNING_TREE_STATE);
            vlanInfoPtr->spanState = fldBit;
        }
    }


    if (descrPtr->vrfId == 0)
    {   /* use Bridge VRF-ID Table to vid */
        __LOG(("Use Bridge VRF-ID [%d] Table to vid",
            vlanInfoPtr->vrfId));
        descrPtr->vrfId = vlanInfoPtr->vrfId;
    }

    if(vlanInfoPtr->autoLearnDisable == 0)
    {
        /* when auto learn enabled this command is not relevant */
        vlanInfoPtr->unknownSaCmd = SKERNEL_EXT_PKT_CMD_FORWARD_E;
    }

    if(descrPtr->eArchExtInfo.bridgeIngressEPortTablePtr)
    {
        /*Ingress Port Unknown UC filter command*/
        newCmd =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNKNOWN_UC_FILTER_CMD);
        vlanInfoPtr->unknownUcastCmd =
            snetChtPktCmdResolution(vlanInfoPtr->unknownUcastCmd , newCmd);

        /*Ingress port unregistered MC filter command*/
        newCmd =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_UNREG_MC_FILTER_CMD);
        vlanInfoPtr->unregIPv4McastCmd =
            snetChtPktCmdResolution(vlanInfoPtr->unregIPv4McastCmd , newCmd);
        vlanInfoPtr->unregNonIpMcastCmd =
            snetChtPktCmdResolution(vlanInfoPtr->unregNonIpMcastCmd , newCmd);
        vlanInfoPtr->unregIPv6McastCmd =
            snetChtPktCmdResolution(vlanInfoPtr->unregIPv6McastCmd , newCmd);

        /*Ingress Port BC filter command*/
        newCmd =
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_INGRESS_PORT_BC_FILTER_CMD);
        vlanInfoPtr->unregIp4BcastCmd =
            snetChtPktCmdResolution(vlanInfoPtr->unregIp4BcastCmd , newCmd);
        vlanInfoPtr->unregNonIp4BcastCmd =
            snetChtPktCmdResolution(vlanInfoPtr->unregNonIp4BcastCmd , newCmd);

        /*IPv4 control TRAP enable*/
        vlanInfoPtr->ipInterfaceEn |=
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV4_CONTROL_TRAP_EN);
        /*IPv6 control TRAP enable*/
        vlanInfoPtr->ipV6InterfaceEn |=
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_IPV6_CONTROL_TRAP_EN);
        /*BC UDP Trap/Mirror enable*/
        vlanInfoPtr->bcUdpTrapMirrorEn |=
            SMEM_LION3_L2I_EPORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_L2I_EPORT_TABLE_FIELDS_BC_UDP_TRAP_OR_MIRROR_EN);
    }
}


/*******************************************************************************
*   snetLion3EgfShtStpStateGet
*
* DESCRIPTION:
*        SIP5 :
*        Get STP state for the egress port
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        stgEntryPtr     - pointer to the STG table (pointed by eVlan by STG index )
*        egressPort      - egress port.
*
* OUTPUTS:
*        None
*
* RETURNS:
*       SKERNEL_STP_BLOCK_LISTEN_E - the EGF will     filter the port
*       SKERNEL_STP_FORWARD_E      - the EGF will NOT filter this port
*
* COMMENTS:
*
*******************************************************************************/
static SKERNEL_STP_ENT snetLion3EgfShtStpStateGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32                   *stgEntryPtr,
    IN GT_U32                   egressPort
)
{
    DECLARE_FUNC_NAME(snetLion3EgfShtStpStateGet);

    GT_U32 regAddress;      /* Register's address */
    GT_U32 * eportStpStateModePtr ;   /* sip5 : Eport Stp State Mode  pointer */
    GT_U32 * eportStpStatePtr ;       /* sip5 : Eport Stp State pointer */
    GT_BIT  stpState; /* the stp state is 1 bit !!! with meaning of :
                              0x0 = NotMember; ;
                              0x1 = Member*/
    GT_BIT getStpStateFromEPort;
    GT_BIT usedEportInfo = 0;

    if(descrPtr->useVidx == 0)
    {
        snetChtEgfEgressEPortEntryGet(devObjPtr,descrPtr,egressPort/*global port*/);

        if(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr)
        {
            /* STP State Mode */
            if(snetFieldValueGet(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr,8,1))
            {
                usedEportInfo = 1;
                /* use STP state from the ePort table */
                stpState = snetFieldValueGet(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr,7,1);
                goto done_lbl;
            }
        }
    }

    regAddress = SMEM_LION3_EGF_SHT_EPORT_STP_STATE_MODE_REG(devObjPtr);
    eportStpStateModePtr = smemMemGet(devObjPtr, regAddress);

    getStpStateFromEPort = snetFieldValueGet(eportStpStateModePtr,egressPort,1);

    if(getStpStateFromEPort == 0)
    {
        /* Get STP port state - according to STG from the eVlan table */
        stpState = snetFieldValueGet(stgEntryPtr, egressPort, 1);
    }
    else
    {
        regAddress = SMEM_LION3_EGF_SHT_EPORT_STP_STATE_REG(devObjPtr);
        eportStpStatePtr = smemMemGet(devObjPtr, regAddress);

        /* ePort STP State (not from the ePort table) but from the registers set */
        stpState = snetFieldValueGet(eportStpStatePtr,egressPort,1);
    }

done_lbl:
    if (stpState == 0)
    {
        /* NOTE that the port is STP blocked */
        if(usedEportInfo)
        {
            __LOG(("the target port [%d] used 'per eport' STP state --> blocking mode (replication is filtered) \n",
               egressPort));
        }
        else
        {
            __LOG(("the target port [%d] used STP state from the STG table --> blocking mode (replication is filtered) \n",
               egressPort));
        }
    }

    /* convert the STP state to 2 bits value to support generic code */
    return stpState ? SKERNEL_STP_FORWARD_E : SKERNEL_STP_BLOCK_LISTEN_E;
}

/*******************************************************************************
*   snetLion3EgfShtStpStateGet
*
* DESCRIPTION:
*        SIP5 :
*        Get if 'vlan filter' enabled on the port
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        egressPort      - egress port.
*
* OUTPUTS:
*        None
*
* RETURNS:
*       GT_TRUE     - the EGF will     filter the port
*       GT_FALSE    - the EGF will NOT filter this port
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 snetLion3EgfShtVlanFilterGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32                   egressPort
)
{
    GT_U32 regAddress;      /* Register's address */
    GT_U32 *regPtr;
    GT_U32 filter;

    if(descrPtr->useVidx == 0)
    {
        snetChtEgfEgressEPortEntryGet(devObjPtr,descrPtr,egressPort/*global port*/);

        if(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr)
        {
            /* use the ePort table */
            filter = snetFieldValueGet(descrPtr->eArchExtInfo.egfShtEgressEPortTablePtr,4,1);
            goto done_lbl;
        }
    }

    regAddress = SMEM_LION3_EGF_SHT_VLAN_FILTERING_ENABLE_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddress);

    filter = snetFieldValueGet(regPtr,egressPort,1);

done_lbl:
    return filter;
}

/*******************************************************************************
*   snetLionTxQGetEgressVidxInfo
*
* DESCRIPTION:
*        Get egress VLAN information and STP
* INPUTS:
*        devObjPtr       - pointer to device object.
*        descrPtr        - pointer to the frame's descriptor.
*        packetType      - type of packet.
*
* OUTPUTS:
*        destPorts       - destination ports.
*        destVlanTagged  - destination vlan tagged.
*        stpVector       - stp ports vector.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetLionTxQGetEgressVidxInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SKERNEL_EGR_PACKET_TYPE_ENT packetType,
    OUT GT_U32 destPorts[],
    OUT GT_U8 destVlanTagged[],
    OUT SKERNEL_STP_ENT stpVector[]
)
{
    DECLARE_FUNC_NAME(snetLionTxQGetEgressVidxInfo);

    GT_U32 checkVidxGroup;  /* vidx checking */
    GT_U32 regAddress;      /* Register's address */
    GT_U32 *regValuePtr;    /* Register's value pointer */
    GT_U32 fldValue;        /* Register's field value */
    GT_U32 entryIndex;      /* Table entry index */
    GT_U32 port;            /* Port number */
    GT_U32 * stgEntryPtr;   /* STG table entry pointer pointer */
    GT_U32 * vlanEntryPtr;  /* VLAN table entry pointer pointer */
    GT_U32 * mcastEntryPtr; /* Multicast table entry pointer pointer */
    GT_U32 localPort;       /* local Port number */
    GT_U32 startPort;       /* Start iteration port */
    GT_U32  endPort;        /* End iteration port */
    GT_U32  vidx = 0;/* vidx to use*/
    GT_U32  vid;    /* Vlan Id to use (descrPtr->eVid or "1" in Vlan unaware mode) */
    GT_U32  indexingVidForVlanMembers;    /* vlan membership table can be accesses according to vid1 */
    GT_U32  indexingVidForStp;    /* STP table can be accesses according to vid1 */
    GT_U32  vlanPortIndex;/* port index in the vlan entry */
    GT_U32  nonVlanPortIndex;/* port index in the STG/vidx entry */
    GT_U32  vlanPortIndexMask = 0x3F;/* mask to apply on port index */
    GT_U32  egressEvlanFilteringEnabled;/*Egress eVLAN Filtering - per port */
    GT_U32  vlanEgressFilterEn;/*Egress VLAN Filtering*/
    GT_U32  vidxEgressFilterEn ;/*Egress VIDX Filtering*/
    GT_U32  spanStateEgressFilterEn ;/*Egress STP Filtering*/
    GT_U32  bypassVlanMembers = 0;/* do we ignore the vlan members */
    GT_U32  shtGlobalConfRegVal;/* value of SHT Global Configurations register */

    /* Current port group start and end port number */
    startPort = SNET_CHT_EGR_TXQ_START_PORT_MAC(devObjPtr);
    endPort = SNET_CHT_EGR_TXQ_END_PORT_MAC(devObjPtr);

    __LOG_PARAM(descrPtr->basicMode);
    /* support 802.1d bridge (Vlan-unaware mode) */
    vid = descrPtr->basicMode ? 1 : descrPtr->eVid;
    indexingVidForVlanMembers = vid;
    indexingVidForStp = vid;

    if(devObjPtr->supportEArch)
    {
        smemRegGet(devObjPtr, SMEM_LION_TXQ_SHT_GLOBAL_CONF_REG(devObjPtr), &shtGlobalConfRegVal);

        if(0 == descrPtr->basicMode)
        {
            /* <Use VID1 for Egress Port Membership> */
            if(SMEM_U32_GET_FIELD(shtGlobalConfRegVal, 7, 1))
            {
                indexingVidForVlanMembers = descrPtr->vid1;
                __LOG(("Accessing egress eVlan membership table with vid1 [%d] indexing\n" , indexingVidForVlanMembers));
            }
            else
            {
                __LOG(("Accessing egress eVlan membership table with eVlan [%d] indexing\n" , indexingVidForVlanMembers));
            }

            /* <Use VID1 for Egress Span State> */
            if(SMEM_U32_GET_FIELD(shtGlobalConfRegVal, 6, 1))
            {
                indexingVidForStp = descrPtr->vid1;
                __LOG(("Accessing egress eVlan span state group index table with vid1 [%d] indexing\n" , indexingVidForStp));
            }
            else
            {
                __LOG(("Accessing egress eVlan span state group index table with eVlan [%d] indexing\n" , indexingVidForStp));
            }
        }
        else
        {
            __LOG(("in basic mode access vlan members and STG only according to vlan = 1 (default vlan) \n"));
        }

        vlanPortIndexMask = 0xFFFFFFFF;/*all ports supported from start to end */

        bypassVlanMembers = 1;

        if(descrPtr->pktIsLooped == 0)
        {
            switch(packetType)
            {
                case SKERNEL_EGR_PACKET_BRG_UCAST_E:
                    /*<BridgedUCEgressFilterEn>*/
                    fldValue = SMEM_U32_GET_FIELD(shtGlobalConfRegVal, 0, 1);
                    __LOG(("<BridgedUCEgressFilterEn> = %d \n" ,
                        fldValue));
                    if(fldValue)
                    {
                        bypassVlanMembers = 0;
                    }
                    else
                    {
                        __LOG(("NOTE: BRG_UCAST that not subject to egress vlan members filter \n"));
                    }
                    break;
                case SKERNEL_EGR_PACKET_ROUT_UCAST_E:
                    /*<RoutedUCEgressFilterEn >*/
                    fldValue = SMEM_U32_GET_FIELD(shtGlobalConfRegVal, 1, 1);
                    __LOG(("<RoutedUCEgressFilterEn > = %d \n" ,
                        fldValue));
                    if(fldValue)
                    {
                        bypassVlanMembers = 0;
                    }
                    else
                    {
                        __LOG(("NOTE: ROUT_UCAST that not subject to egress vlan members filter \n"));
                    }
                    break;
                case SKERNEL_EGR_PACKET_CNTRL_UCAST_E:
                     __LOG(("NOTE: CNTRL_UCAST not subject to egress vlan members filter \n"));
                    break;
                default:
                     bypassVlanMembers = 0;
                    break;
            }
        }

        /*<Enable independent non-flood VIDX>*/
        fldValue = SMEM_U32_GET_FIELD(shtGlobalConfRegVal, 4, 1);

        __LOG(("<Enable independent non-flood VIDX> = %d \n" ,
            fldValue));

        if(fldValue &&
           (descrPtr->useVidx == 1) &&
           (descrPtr->eVidx != 0xFFF))
        {
            __LOG(("(flood to vidx) bypass vlan members filter due to <Enable independent non-flood VIDX> == 1 \n"));
            bypassVlanMembers = 1;
        }

        /*<Control_from_CPU_Egress_Filter_En>*/
        fldValue = SMEM_U32_GET_FIELD(shtGlobalConfRegVal, 14, 1);

        __LOG(("<Control_from_CPU_Egress_Filter_En> = %d \n" ,
            fldValue));

        if((fldValue == 0) &&
           (packetType == SKERNEL_EGR_PACKET_CNTRL_MCAST_E) &&
           (descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FROM_CPU_E) &&
           descrPtr->useVidx)
        {
            __LOG(("(multicast control from cpu) bypass vlan members filter due to <Control_from_CPU_Egress_Filter_En> == 0 \n"));
            bypassVlanMembers = 1;
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr) &&
           descrPtr->outGoingMtagCmd == SKERNEL_MTAG_CMD_FORWARD_E &&
           descrPtr->useVidx &&
           descrPtr->tunnelStart)
        {
            smemRegGet(devObjPtr, SMEM_LION_TXQ_EGR_FILTER_GLOBAL_EN_REG(devObjPtr),&fldValue);

            /* check the 'Tunnel Start Egress Filter Enable' and the 'global filter enable' */
            if(SMEM_U32_GET_FIELD(fldValue,0,1) &&
               SMEM_U32_GET_FIELD(fldValue,7,1))
            {
                /* we allow 'Tunnel Start Egress Filter Enable' */
                __LOG(("allow vlan filtering on 'Tunnel Start' (single and multi-destination) \n"));
            }
            else
            {
                /* Filter Tunnel Start Multicast */
                regAddress = SMEM_LION3_EGF_EFT_EGR_FILTERS_GLOBAL_REG(devObjPtr);
                smemRegFldGet(devObjPtr, regAddress, 10, 1, &fldValue);
                __LOG(("<Filter Tunnel Start Multicast> = %d \n" ,
                    fldValue));

                if(descrPtr->useVidx == 0 || fldValue == 0)
                {
                    if(descrPtr->useVidx)
                    {
                        __LOG(("(flood to vidx) bypass vlan members filter due to <Filter Tunnel Start Multicast> == 0 \n"));
                    }
                    else
                    {
                        __LOG(("Single destination TS : bypass vlan members filter due to <Tunnel Start Egress Filter Enable> == 0 \n"));
                    }
                    bypassVlanMembers = 1;

                    if(descrPtr->useVidx && descrPtr->eArchExtInfo.vidx == 0xFFF)
                    {
                        __LOG(("WARNING : potential configuration error ! flood is probably done to ALL ports with 'link up' ... not only to 'vlan members' (of vlan[%d]) \n",
                            indexingVidForVlanMembers));
                    }
                }
            }
        }

        if(bypassVlanMembers)
        {
            __LOG(("NOTE: Due to bypass vlan members filter : consider that all [%d] ports as members of vlan[%d] !!! \n",
                endPort,indexingVidForVlanMembers));
        }

        regAddress = SMEM_LION3_EGF_SHT_EGR_FILTERS_ENABLE_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddress,&fldValue);


        /*SHTGlobalEgressFiltersEn */
        if(SMEM_U32_GET_FIELD(fldValue, 0, 1))
        {
            vlanEgressFilterEn       = SMEM_U32_GET_FIELD(fldValue, 1, 1);
            vidxEgressFilterEn       = SMEM_U32_GET_FIELD(fldValue, 2, 1);
            spanStateEgressFilterEn  = SMEM_U32_GET_FIELD(fldValue, 3, 1);
        }
        else
        {
            vlanEgressFilterEn       = 0;
            vidxEgressFilterEn       = 0;
            spanStateEgressFilterEn  = 0;
        }
    }
    else
    {
        vlanEgressFilterEn       = 1;/* make generic code */
        vidxEgressFilterEn       = 1;/* make generic code */
        spanStateEgressFilterEn  = 1;/* make generic code */
    }

    nonVlanPortIndex = startPort;/* save value before apply vlanPortIndexMask */

    startPort &= vlanPortIndexMask;
    endPort   &= vlanPortIndexMask;

    /* Get egress VLAN entry according to vid */
    regAddress = SMEM_LION_EGR_VLAN_TBL_MEM_REG(devObjPtr, indexingVidForVlanMembers);
    vlanEntryPtr = smemMemGet(devObjPtr, regAddress);

    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        fldValue = 1;/* Vlan entry is always valid . (valid bit was removed) */
    }
    else
    {
        /* VLAN validation */
        fldValue = snetFieldValueGet(vlanEntryPtr, 0, 1);
    }
    /* Check validity of VLAN */
    if (fldValue == 0)
    {
        __LOG(("NOTE: Egress VLAN[%d] is not valid , so considered as if no ports to send to ! \n",
                      vid));
        return ;
    }

    __LOG(("Egress VLAN[%d] is valid \n",
                  vid));

    /* Get egress STP entry */
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddress = SMEM_LION2_EGF_QAG_EGRESS_EVLAN_SPANNING_TBL_MEM(devObjPtr, indexingVidForStp);
        smemRegFldGet(devObjPtr, regAddress , 0, 12, &entryIndex);

        __LOG(("Egress STP index from Spanning Table = [%d] \n" , entryIndex));
    }
    else
    {
        entryIndex = snetFieldValueGet(vlanEntryPtr, 254, 8);
        __LOG(("Egress STP index from Egress Vlan Table = [%d] \n" , entryIndex));
    }

    regAddress = SMEM_LION_EGR_STP_TBL_MEM_REG(devObjPtr, entryIndex);
    stgEntryPtr = smemMemGet(devObjPtr, regAddress);

    checkVidxGroup = 0;
    /* 1. check if packet is multicast , if set only vidx group is considered */
    if (descrPtr->useVidx)
    {
        if(devObjPtr->supportEArch)
        {
            /* This table is indexed by the InDesc<VIDX> (not by the eVidx). */
            vidx = descrPtr->eArchExtInfo.vidx;
        }
        else
        {
            /* NOTE: this is not 'bug' that the 'Non supportEArch' uses the field : eVidx.
                because this field 'replaces' the 'legacy' <vidx> field */
            vidx = descrPtr->eVidx;
        }

        checkVidxGroup = 1;
    }
    else if(descrPtr->eVidx == 0xFFF)
    {
        vidx = 0xFFF;

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* the behavior changed ! , the single destination not care about the VIDX !!!! */
        }
        else
        {
            /* the VIDX 0xFFF is checked on single AND multi destination traffic !!! */
            checkVidxGroup = 1;
        }
    }

    if (checkVidxGroup)
    {
        regAddress = SMEM_CHT_MCST_TBL_MEM(devObjPtr, vidx);
        mcastEntryPtr = smemMemGet(devObjPtr, regAddress);
    }
    else
    {
        mcastEntryPtr = NULL;
    }

    if(simLogIsOpen())
    {
        __LOG_PARAM(checkVidxGroup);
        if(checkVidxGroup)
        {
            __LOG_PARAM(vidx);
        }
        __LOG_PARAM(bypassVlanMembers);
        __LOG_PARAM(vlanEgressFilterEn);
        __LOG_PARAM(vidxEgressFilterEn);
        __LOG_PARAM(spanStateEgressFilterEn);
        __LOG_PARAM(startPort);
        __LOG_PARAM(endPort);
        __LOG_PARAM(indexingVidForVlanMembers);
        __LOG_PARAM(indexingVidForStp);
    }

    for (port = startPort,localPort = 0; port < endPort; port++ , localPort++,nonVlanPortIndex++)
    {
        vlanPortIndex = port;

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* call snetChtEgfEgressEPortEntryGet to get
                assignment of descrPtr->eArchExtInfo.egfShtEgressClass */
            snetChtEgfEgressEPortEntryGet(devObjPtr,descrPtr,port/*global port*/);

            if(descrPtr->eArchExtInfo.egfShtEgressClass != SHT_PACKET_CLASS_3_E)
            {
                egressEvlanFilteringEnabled =
                    snetLion3EgfShtVlanFilterGet(devObjPtr,descrPtr,port);
            }
            else /* SHT_PACKET_CLASS_3_E */
            {
                egressEvlanFilteringEnabled = 1;
            }
        }
        else
        {
            /* make generic code */
            egressEvlanFilteringEnabled = 1;
        }

        if(bypassVlanMembers || (vlanEgressFilterEn == 0))
        {
            destPorts[localPort] = 1; /* ALL ports considered vlan members !!!! */
        }
        else if(egressEvlanFilteringEnabled == 0)
        {
            destPorts[localPort] = 1; /* ALL ports considered vlan members !!!! */
        }
        else
        {
            destPorts[localPort] =
                SMEM_CHT_IS_SIP5_20_GET(devObjPtr) ?
                snetFieldValueGet(vlanEntryPtr, vlanPortIndex, 1) :
                snetFieldValueGet(vlanEntryPtr, 1 + vlanPortIndex, 1);
        }

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(destPorts[localPort] == 0 &&
               descrPtr->eArchExtInfo.egfShtEgressClass != SHT_PACKET_CLASS_3_E)
            {
                /* check if doing 'Egress eVLAN Filtering' */
                egressEvlanFilteringEnabled =
                    snetLion3EgfShtVlanFilterGet(devObjPtr,descrPtr,port);

                if(egressEvlanFilteringEnabled == 0)
                {
                    /* we NOT filter the port . */
                    destPorts[localPort] = 1;
                }
            }

            /* the egress tagging mode is not in the eVlan table , but in other
               table that is accessed in later stage per egress port */

            if(spanStateEgressFilterEn)
            {
                /* Get STP port state */
                stpVector[localPort] =
                    snetLion3EgfShtStpStateGet(devObjPtr,descrPtr,stgEntryPtr,port);
            }
            else
            {
                /* we not filter ports due to STP */
                stpVector[localPort] = SKERNEL_STP_FORWARD_E;
            }
        }
        else
        {
            destVlanTagged[localPort] =
                snetFieldValueGet(vlanEntryPtr, 65 + 3 * vlanPortIndex, 3);

            /* Get STP port state */
            stpVector[localPort] =
                snetFieldValueGet(stgEntryPtr, 2 * nonVlanPortIndex, 2);
        }

        if(vidxEgressFilterEn == 0)
        {
            /* we not filter ports from the list due to VIDX ! */
        }
        else
        if (checkVidxGroup)
        {
            /* Get VIDX membership and mask it with vlan membership */
            destPorts[localPort] &=
                snetFieldValueGet(mcastEntryPtr, nonVlanPortIndex, 1);
        }
    }/* end loop on ports */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddress = SMEM_LION2_EGF_QAG_EGRESS_EVLAN_ATTRIBUTE_TBL_MEM(devObjPtr, vid);
        regValuePtr = smemMemGet(devObjPtr, regAddress);

        /* cpu port is like any other port */

        descrPtr->egressVlanInfo.mcLocalEn =
            snetFieldValueGet(regValuePtr, 0, 1);
        descrPtr->egressVlanInfo.portIsolationVlanCmd =
            snetFieldValueGet(regValuePtr, 1, 2);

        if( SKERNEL_MTAG_CMD_TO_TRG_SNIFFER_E != descrPtr->outGoingMtagCmd)
        {
            /*Mirror to Analyzer Index_0_2*/
            fldValue = snetFieldValueGet(regValuePtr, 3 , 3);
            if(fldValue)/*NOTE: allowed on all traffic types (not only on forward) */
            {
                __LOG(("vlan [%d] egress Mirror to Analyzer Index[%d] \n",
                    vid,fldValue));
                __LOG(("NOTE: still it maybe ignored if 'vlan egress Mirror' not enabled on the egress physical port \n"));
            }

            snetXcatEgressMirrorAnalyzerIndexSelect(devObjPtr,descrPtr,fldValue);
        }
        else
        {
            __LOG(("vlan egress mirroring not relevant to 'analyzer' replications \n"));
        }

        /* no 'relay port' because the EGF 'see' all 256 ports and not need to
           'jump' to next hemisphere */
    }
    else
    {
        /* cpu port handling */
        localPort = SNET_CHT_CPU_PORT_CNS;
        port = SNET_CHT_CPU_PORT_CNS;
        nonVlanPortIndex = port;
        vlanPortIndex = port & vlanPortIndexMask;

        if(descrPtr->useVidx ||
           devObjPtr->portGroupSharedDevObjPtr == NULL ||
           SMEM_CHT_PORT_GROUP_ID_MASK_CORE_MAC(devObjPtr, devObjPtr->portGroupId, devObjPtr->dualDeviceIdEnable.txq) == 0)
        {
            destPorts[localPort] =
                snetFieldValueGet(vlanEntryPtr, 1 + vlanPortIndex, 1);

            if (checkVidxGroup)
            {
                /* Get VIDX membership */
                __LOG(("Get VIDX membership"));
                destPorts[localPort] &=
                    snetFieldValueGet(mcastEntryPtr, nonVlanPortIndex, 1);
            }
        }
        else
        {
            /* avoid the flooding to CPU from all port groups */
            destPorts[localPort] = 0;
        }

        descrPtr->egressVlanInfo.mcLocalEn =
            snetFieldValueGet(vlanEntryPtr, 262, 1);
        descrPtr->egressVlanInfo.portIsolationVlanCmd =
            snetFieldValueGet(vlanEntryPtr, 263, 2);

        if(devObjPtr->numOfTxqUnits >= 2)
        {
             /* get the relayed port number */
            smemRegFldGet(devObjPtr, SMEM_LION2_TXQ_RELAYED_PORT_NUMBER_REG(devObjPtr), 0, 6, &port);

            nonVlanPortIndex = port;
            vlanPortIndex = port & vlanPortIndexMask;

            /* when bit is 0 means that ' relayed port' allow flood to next TXQ unit */
            descrPtr->floodToNextTxq = (snetFieldValueGet(vlanEntryPtr, 1 + port, 1) == 0) ?/* vlan entry */
                                        1 : 0;

            if(mcastEntryPtr)
            {
                descrPtr->floodToNextTxq &= (snetFieldValueGet(mcastEntryPtr,    nonVlanPortIndex, 1) == 0) ?  /* vidx entry */
                                        1 : 0;
            }
        }
    }
}

/*******************************************************************************
* snetLionTxQGetTrunkDesignatedPorts
*
* DESCRIPTION:
*        Trunk designated ports bitmap
*
* INPUTS:
*        devObjPtr             - pointer to device object.
*        descrPtr                 - pointer to the frame's descriptor.
*        trunkHashMode            - cascade trunk mode
* OUTPUTS:
*        designatedPortsBmpPtr  - pointer to designated port bitmap
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLionTxQGetTrunkDesignatedPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN TRUNK_HASH_MODE_ENT    trunkHashMode,
    OUT SKERNEL_PORTS_BMP_STC * designatedPortsBmpPtr
)
{
    DECLARE_FUNC_NAME(snetLionTxQGetTrunkDesignatedPorts);

    GT_U32 * regPtr;            /* Pointer to register data  */
    GT_U32 regValue;            /* register value  */
    GT_U32 regAddress;          /* Register address */
    GT_U32 fldValue;            /* Register field value */
    GT_U32 entry = 0;               /* Calculated entry number */
    GT_U32  ii;/*iterator*/
    GT_U16 swappedVidx;/*swapped  VIDX
                {VIDX[0], VIDX[1], VIDX[2], VIDX[3], VIDX[4], VIDX[5] ,
                 VIDX[6], VIDX[7], VIDX[8], VIDX[9], VIDX[10], VIDX[11]}
                 */
    GT_U32  pktHash;/* the relevant bits from the packet hash */
    GT_U32  multicastTrunkHashMode;/*Defines how the designated member table is accessed for multi-destination packets,
                i.e., how the table index is chosen.
                Mode 0x3 is suitable for MC load balancing in a stacking system
                because it guarantees that the same hash function is calculated
                in all devices*/
    GT_U32 splitDesignatedTableEnable; /* indication of split UC,MC designated table */


    ASSERT_PTR(designatedPortsBmpPtr);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddress = SMEM_LION3_EGF_EFT_EGR_FILTERS_GLOBAL_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddress,&regValue);
        multicastTrunkHashMode =  SMEM_U32_GET_FIELD(regValue, 2, 2);
        /*Hash Bits Selection*/
        /* This field selects which bits from the packet hash will be used when
          accessing the designated members table*/
        fldValue = SMEM_U32_GET_FIELD(regValue, 14, 1);

        if(fldValue)
        {
            /*Use bits 11:6 from the packet hash*/
            pktHash = (descrPtr->pktHash >> 6) & 0x3f;
            __LOG(("Use bits 11:6 from the packet hash [%d] \n",
                pktHash));
        }
        else
        {
            /*Use bits 5:0 from the packet hash*/
            pktHash = descrPtr->pktHash & 0x3f;
            __LOG(("Use bits 5:0 from the packet hash [%d] \n",
                pktHash));
        }
    }
    else
    {
        regAddress = SMEM_LION_TXQ_EGR_FILTER_GLOBAL_EN_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddress,&regValue);
        multicastTrunkHashMode =  SMEM_U32_GET_FIELD(regValue, 5, 2);

        pktHash = descrPtr->pktHash & 0x3f;
        __LOG(("Use bits 5:0 from the packet hash [%d] \n",
            pktHash));
    }


    switch(multicastTrunkHashMode)
    {
        case 0:
            /* Use the trunk hash computed at the beginning of the pipe */
            entry = pktHash;
            __LOG(("Use the trunk hash computed at the beginning of the pipe [%d] \n",
                entry));
            break;
        case 2:
            /* The index is based on the computed trunk hash, XORed with the VID assigned to the packet */
            entry = SMEM_U32_GET_FIELD(descrPtr->eVid,0,6) ^
                    SMEM_U32_GET_FIELD(descrPtr->eVid,6,6) ^
                    (pktHash);

            __LOG(("The index is based on the computed trunk hash, XORed with the VID assigned to the packet [%d] \n",
                entry));
            break;
        case 3:
            /* build field of swapped bits of the VIDX */
            swappedVidx = 0;
            for(ii = 0 ; ii < 12 ; ii++)
            {
                if(SMEM_U32_GET_FIELD(descrPtr->eVidx,ii,1))
                {
                    SMEM_U32_SET_FIELD(swappedVidx,(11 - ii),1,1);
                }
            }
            /* The index is based on the original source port/trunk
               (in the case of trunk: the 6 least significant bits of the trunk) */
            entry =
                    SMEM_U32_GET_FIELD(swappedVidx,0,6) ^ /*<{VIDX[6], VIDX[7], VIDX[8], VIDX[9], VIDX[10], VIDX[11]}>*/
                    SMEM_U32_GET_FIELD(swappedVidx,6,6) ^ /*<{VIDX[0], VIDX[1], VIDX[2], VIDX[3], VIDX[4], VIDX[5]}*/
                    SMEM_U32_GET_FIELD(descrPtr->eVid,0,6) ^
                    SMEM_U32_GET_FIELD(descrPtr->eVid,6,6) ^
                    SMEM_U32_GET_FIELD(descrPtr->origSrcEPortOrTrnk,0,6);

            if(devObjPtr->designatedPortVersion)
            {
                /* Extended logic reflects increasing in the number of ports for Lion2 */
                __LOG(("Extended logic reflects increasing in the number of ports for Lion2"));
                entry ^= descrPtr->srcDev;
            }

            __LOG(("The index is based on the original source port/trunk (in the case of trunk: the 6 least significant bits of the trunk) [%d] \n",
                entry));

            break;
        case 1:
            entry = 0;
            __LOG(("use first entry [%d] \n",
                entry));
            goto useIndex_lbl;/* also for single destination */
        default:
            /* can't get here */
            break;
    }

    if(descrPtr->useVidx == 0 /*cascade trunk*/ &&
        trunkHashMode != TRUNK_HASH_MODE_USE_MULIT_DESTINATION_HASH_SETTINGS_E)
    {
        switch(trunkHashMode)
        {
            case TRUNK_HASH_MODE_USE_PACKET_HASH_E:
                entry = pktHash;
                __LOG(("single destination (from cascade trunk)- mode %s - use index[%d] \n",
                    "USE_PACKET_HASH",
                    entry));
                break;
            case TRUNK_HASH_MODE_USE_GLOBAL_SRC_PORT_HASH_E:
                entry =  descrPtr->origSrcEPortOrTrnk;  /* same for orig port/trunk */
                __LOG(("single destination (from cascade trunk)- mode %s - use index[%d] \n",
                    "USE_GLOBAL_SRC_PORT_HASH",
                    entry));
                break;
            case TRUNK_HASH_MODE_USE_GLOBAL_DST_PORT_HASH_E:
                entry =  descrPtr->trgEPort;
                __LOG(("single destination (from cascade trunk)- mode %s - use index[%d] \n",
                    "USE_GLOBAL_DST_PORT_HASH",
                    entry));
                break;
            case TRUNK_HASH_MODE_USE_LOCAL_SRC_PORT_HASH_E:
                entry =  descrPtr->localDevSrcPort; /* use 'global' port of the local device */
                __LOG(("single destination (from cascade trunk)- mode %s - use index[%d] \n",
                    "USE_LOCAL_SRC_PORT_HASH",
                    entry));
                break;
            default:
                break;
        }
    }


useIndex_lbl:
    entry &= 0x3f;

    if (devObjPtr->supportSplitDesignatedTrunkTable)
    {
        smemRegFldGet(devObjPtr,
            SMEM_LION_TXQ_SHT_GLOBAL_CONF_REG (devObjPtr), 5, 1, &splitDesignatedTableEnable);

        __LOG_PARAM(splitDesignatedTableEnable);

        if(splitDesignatedTableEnable && descrPtr->useVidx)
        {
            __LOG(("Use 'dedicated' MC designated table (high 64 entries) \n"));
            entry += 64;
        }
        else
        if(splitDesignatedTableEnable)
        {
            __LOG(("Use 'dedicated' UC designated table (low 64 entries) \n"));
        }
        else
        {
            __LOG(("Use 'shared' UC,MC designated table (legacy mode - 64 entries) \n"));
        }
    }
    else
    {
        __LOG(("Use 'shared' UC,MC designated table \n"));
    }

    __LOG(("final index[%d] to the designated port table \n",
        entry));


    regAddress = SMEM_LION_TXQ_DESIGNATED_PORT_TBL_MAC(devObjPtr, entry);
    regPtr = smemMemGet(devObjPtr, regAddress);

    /* Bitmap[64] of designated trunk ports */
    SKERNEL_FILL_PORTS_BITMAP_MAC(devObjPtr, designatedPortsBmpPtr, regPtr);
}


/*******************************************************************************
* devMapTableAddrConstruct
*
* DESCRIPTION:
*           The device map table is accessed according to the <DevMapTableAddrConstruct> register.
*           Get index into 'Device map table'
*
*
* INPUTS:
*           devObjPtr       - pointer to device object.
*           descrPtr        - pointer to the frame's descriptor.
*           trgDev          - target device
*           trgPort         - target port
*           srcDev          - source device
*           srcPort         - source port
* OUTPUTS:
*
* RETURNS:
*           Device Map Entry index
*           value - SMAIN_NOT_VALID_CNS --> meaning : indication that the device
*                                                     map table will not be accessed
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 devMapTableAddrConstruct
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 trgDev,
    IN GT_U32 trgPort,
    IN GT_U32 srcDev,
    IN GT_U32 srcPort
)
{
    DECLARE_FUNC_NAME(devMapTableAddrConstruct);

    GT_U32 regAddr;             /* Register address */
    GT_U32 regValue;            /* Register value */
    GT_U32 entryIndex;          /* Table entry index */
    GT_U32 profile;     /* This value indicates the ID of the profile associated with the port,
                           the profile select the type of index which is used to access
                           the device map table and the offset which is used*/
    GT_U32 portForProfile;/*the port that is used for the profile */
    GT_U32 devMapOffset;/*Indicates the offset used when accessing the device map table.
                        The index to access the device map table will be based on DevMapTableAddrConstruct + DevMapOffset*/
    GT_U32  devMapEn;/*Enables target device mapping to target port/trunk.*/
    GT_U32  devMapTableAddrConstruct_mode;

    if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
    {
        portForProfile = descrPtr->localDevSrcPort / 8;/* 8 ports in register */
        regAddr = SMEM_LION3_EGF_EFT_PORT_ADDRESS_CONSTRUCT_MODE_REG(devObjPtr,portForProfile);
        smemRegGet(devObjPtr, regAddr,&regValue);
        /* 8 ports in register */
        profile = SMEM_U32_GET_FIELD(regValue, 3* (descrPtr->localDevSrcPort % 8), 3);
    }
    else
    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        portForProfile = descrPtr->localDevSrcPort;
        regAddr = SMEM_LION3_EGF_EFT_PORT_ADDRESS_CONSTRUCT_MODE_REG(devObjPtr,portForProfile);
        smemRegGet(devObjPtr, regAddr,&regValue);

        profile = SMEM_U32_GET_FIELD(regValue, 0, 3);
    }
    else
    {
        /* next are to allow 'generic code' */
        profile = 0;/* not care */
    }

    regAddr = SMEM_LION_TXQ_DEV_MAP_MEM_CONSTRUCT_REG(devObjPtr,profile);
    smemRegGet(devObjPtr, regAddr,&regValue);

    devMapTableAddrConstruct_mode = SMEM_U32_GET_FIELD(regValue, 0, 4);

    __LOG_PARAM(devMapTableAddrConstruct_mode);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        devMapEn = SMEM_U32_GET_FIELD(regValue, 4, 1);
        if(devMapEn == 0)
        {
            /* indication that the device map table will not be accessed */
            return SMAIN_NOT_VALID_CNS;
        }

        devMapOffset = SMEM_U32_GET_FIELD(regValue, 5, 12);
    }
    else
    {
        /* next are to allow 'generic code' */
        devMapOffset = 0;
    }

    entryIndex = 0;

    switch(devMapTableAddrConstruct_mode)
    {
        case 0:
            entryIndex = trgDev;
            __LOG_PARAM_WITH_NAME("mode 0 : trgDev" , entryIndex);
            break;
        case 1:
            entryIndex = trgDev << 6 | (trgPort & 0x3f);
            __LOG_PARAM_WITH_NAME("mode 1 : trgDev << 6 | (trgPort & 0x3f)" , entryIndex);
            break;
        case 2:
            entryIndex = trgDev << 5 | (srcDev & 0x1f);
            __LOG_PARAM_WITH_NAME("mode 2 : trgDev << 5 | (srcDev & 0x1f" , entryIndex);
            break;
        case 3:
            entryIndex = trgDev << 6 | (srcPort & 0x3f);
            __LOG_PARAM_WITH_NAME("mode 3 : trgDev << 6 | (srcPort & 0x3f)" , entryIndex);
            break;
        case 4:
            entryIndex = trgDev << 6 |
                         (srcDev & 0x7) << 3 | (srcPort & 0x7);
            __LOG_PARAM_WITH_NAME("mode 4 : trgDev << 6 | (srcDev & 0x7) << 3 | (srcPort & 0x7)" , entryIndex);
            break;
        case 5:
            entryIndex = trgDev << 6 |
                         (srcDev & 0xf) << 2 | (srcPort & 0x3);
            __LOG_PARAM_WITH_NAME("mode 5 : trgDev << 6 | (srcDev & 0xf) << 2 | (srcPort & 0x3)" , entryIndex);
            break;
        case 6:
            entryIndex = trgDev << 6 |
                         (srcDev & 0x1f) << 1 | (srcPort & 0x1);
            __LOG_PARAM_WITH_NAME("mode 6 : trgDev << 6 | (srcDev & 0x1f) << 1 | (srcPort & 0x1)" , entryIndex);
            break;
        case 7:
            entryIndex = trgDev << 6 |
                         (srcDev & 0x3) << 4 | (srcPort & 0xf);
            __LOG_PARAM_WITH_NAME("mode 7 : trgDev << 6 | (srcDev & 0x3) << 4 | (srcPort & 0xf)" , entryIndex);
            break;
        case 8:
            entryIndex = trgDev << 6 |
                         (descrPtr->srcDev & 0x1) << 5 | (srcPort & 0x1f);
            __LOG_PARAM_WITH_NAME("mode 8 : trgDev << 6 | (descrPtr->srcDev & 0x1) << 5 | (srcPort & 0x1f)" , entryIndex);
            break;
        case 9: /*{<TRGePort>[11:0]}*/
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                entryIndex = trgPort;
                __LOG_PARAM_WITH_NAME("mode 9 : <TRGePort>[11:0]" , entryIndex);
            }
            else
            {
                __LOG_PARAM_WITH_NAME("mode %d : not supported" , devMapTableAddrConstruct_mode);
            }
            break;
        case 10: /*{<TrgDev>[3:0], <LocalDevSrcPort>[7:0]}*/
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                entryIndex = trgDev << 8 | (descrPtr->localDevSrcPort & 0xFF);
                __LOG_PARAM_WITH_NAME("mode 10 : <TrgDev>[3:0], <LocalDevSrcPort>[7:0]" , entryIndex);
            }
            else
            {
                __LOG_PARAM_WITH_NAME("mode %d : not supported" , devMapTableAddrConstruct_mode);
            }
            break;
        case 11: /*{<TrgDev>[5:0], <Hash>[5:0]}*/
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                entryIndex = trgDev << 6 | (descrPtr->pktHash & 0x3F);
                __LOG_PARAM_WITH_NAME("mode 11 : <TrgDev>[5:0], <Hash>[5:0]" , entryIndex);
            }
            else
            {
                __LOG_PARAM_WITH_NAME("mode %d : not supported" , devMapTableAddrConstruct_mode);
            }
            break;
        case 12: /*{<TrgDev>[5:0], <Hash>[11:6]}*/
            if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
            {
                entryIndex = trgDev << 6 | ((descrPtr->pktHash >> 6) & 0x3F);
                __LOG_PARAM_WITH_NAME("mode 12 : <TrgDev>[5:0], <Hash>[11:6]" , entryIndex);
            }
            else
            {
                __LOG_PARAM_WITH_NAME("mode %d : not supported" , devMapTableAddrConstruct_mode);
            }
            break;
        default:
            __LOG_PARAM_WITH_NAME("mode %d : not supported" , devMapTableAddrConstruct_mode);
            break;
    }

    entryIndex += devMapOffset;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* trim the result into 12 bits */
        entryIndex &= 0xFFF;/*12 bits*/
    }

    __LOG_PARAM_WITH_NAME("access device map table at index : ", entryIndex);

    return entryIndex;
}


/*******************************************************************************
* snetLionTxQGetDeviceMapTableAddress
*
* DESCRIPTION:
*           Get table entry address from Device Map table
*           according to source/target device and source/target port
*
* INPUTS:
*           devObjPtr       - pointer to device object.
*           descrPtr        - pointer to the frame's descriptor.
*           trgDev          - target device
*           trgPort         - target port
*           srcDev          - source device
*           srcPort         - source port
* OUTPUTS:
*
* RETURNS:
*           Device Map Entry address
*           value - SMAIN_NOT_VALID_CNS --> meaning : indication that the device
*                                                     map table will not be accessed
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 snetLionTxQGetDeviceMapTableAddress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 trgDev,
    IN GT_U32 trgPort,
    IN GT_U32 srcDev,
    IN GT_U32 srcPort
)
{
    GT_U32 entryIndex = devMapTableAddrConstruct(devObjPtr,descrPtr,trgDev,trgPort,srcDev,srcPort);

    if(entryIndex == SMAIN_NOT_VALID_CNS)
    {
        /* indication that the device map table will not be accessed */
        return SMAIN_NOT_VALID_CNS;
    }

    return SMEM_CHT_DEVICE_MAP_TABLE_ENTRY(devObjPtr, entryIndex);
}

/*******************************************************************************
*   snetLionCrcBasedTrunkHash
*
* DESCRIPTION:
*        CRC Based Hash Index Generation Procedure
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*       descrPtr            - pointer to the frame's descriptor.
*
* RETURN:
*
********************************************************************************/
GT_VOID snetLionCrcBasedTrunkHash
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionCrcBasedTrunkHash);

    GT_U32 regAddr;                     /* Register's address */
    GT_U32 *regPtr;                     /* Register entry pointer */
    GT_U32 fieldVal;                    /* Register's field value */
    GT_U32 crcSeed;                     /* The seed used by the CRC computation */
    GT_U32 crcValue;                    /* the CRC computation */
    GT_U32 hashVal;                     /* Hashing trunk value */
    GT_U32 entryIndex;                  /* Hash table index */
    GT_U8 crcHashKey[SNET_LION_CRC_HEADER_HASH_BYTES_CNS];
                                        /* Packet Header Fields (70 bytes) */
    GT_U32 saltHashKey[(SNET_LION_CRC_HEADER_HASH_BYTES_CNS + 3) / 4];
                                        /* 'Salt' for hash bytes (support 70 bytes) */
    GT_U32 hashFunctionSelection;/*global hash modes*/
    GT_U32 ii,iiMax;
    GT_U32 carry;/*value of bit8 when add 2 'bytes' */
    GT_U32 tempValue;

    memset(crcHashKey, 0, sizeof(crcHashKey));

    regAddr = SMEM_LION_PCL_CRC_HASH_CONF_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Trunk Hash Mode */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 0, 1);
    if(fieldVal == 0)
    {
        /* "Simple" Trunk Hash Mode */
        __LOG(("'Simple' Trunk Hash Mode (so use XOR hash value [0x%x]) \n",
            descrPtr->pktHash));
        return;
    }

    /* Build hash input fields -
    Packet Header Fields (70 bytes) and Packet Fields Mask (70 bits) */
    snetLionCrcHashBuildKey(devObjPtr, descrPtr, crcHashKey);

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /*This field selects the size of the hash function which will be used
         0x0 = Compatible; Compatible; A 6-bit hash function is used
         0x1 = Enhanced; Enhanced; A 32-bit hash function is used        */
        hashFunctionSelection = SMEM_U32_GET_FIELD(regPtr[0], 3, 1);
    }
    else
    {
        hashFunctionSelection = 0;
    }

    if(hashFunctionSelection == 1)
    {
        /* Enhanced Hash Computation Block */
        __LOG(("Enhanced Hash Computation Block (CRC_32) \n"));

        /* 20 registers with 28 bits in each = 560 bits = 70 bytes */
        iiMax = 20;
        for(ii = 0 ; ii < iiMax; ii++)
        {
            regAddr = SMEM_LION3_PCL_HASH_CRC_32_SALT_REG(devObjPtr, ii);
            smemRegGet(devObjPtr, regAddr,&fieldVal);

            /* build the words for the 70 bytes of the 'salt'*/
            snetFieldValueSet(saltHashKey,28*ii,28,fieldVal);
        }

        carry = 0;
        fieldVal = 0;
        tempValue = 0;
        /*The 70 bytes <Salt> is added to the key byte by byte*/
        for(ii = 0 ; ii < SNET_LION_CRC_HEADER_HASH_BYTES_CNS; ii++)
        {
            fieldVal = snetFieldValueGet(saltHashKey,8*ii,8);
            /* add the byte value + salt + carry from previous byte */
            tempValue = crcHashKey[ii] + fieldVal + carry;

            /* take 8 bits to new value */
            crcHashKey[ii] +=  (GT_U8)tempValue;
            /* save bit 8 as carry for next byte */
            carry = (tempValue >> 8) & 1;
        }

        /* CRC 32 Seed */
        regAddr = SMEM_LION3_PCL_HASH_CRC_32_SEED_REG(devObjPtr);
        smemRegGet(devObjPtr, regAddr,&crcSeed);
        /* Calculate CRC 32 bit for input data */
        crcValue = simCalcHashFor70BytesCrc32(crcSeed, crcHashKey,
                                             SNET_LION_CRC_HEADER_HASH_BYTES_CNS);
        /* the CRC value is the hash value (all 32 bits) */
        hashVal = crcValue;
    }
    else
    {
        /* CRC Hash Sub-Mode */
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 1, 1);
        if(fieldVal == 0)
        {
            __LOG(("CRC 16 Hash Mode"));
            /* CRC 16 Seed */
            crcSeed = SMEM_U32_GET_FIELD(regPtr[0], 16, 16);
            /* Calculate CRC 16 bit for input data */
            crcValue = simCalcHashFor70BytesCrc16((GT_U16)crcSeed, crcHashKey,
                                                 SNET_LION_CRC_HEADER_HASH_BYTES_CNS);

            /* CRC-16 + Pearson based hash sub-mode */
            regAddr = SMEM_LION_PCL_PEARSON_HASH_TBL_MEM(devObjPtr, 0);
            regPtr = smemMemGet(devObjPtr, regAddr);

            entryIndex = SMEM_U32_GET_FIELD(crcValue,0,6);
            /* A = Table[CRC[5:0]] */
            hashVal = snetFieldValueGet(regPtr, 8 * entryIndex, 6);

            entryIndex = hashVal ^ SMEM_U32_GET_FIELD(crcValue,6,6);
            /* B = Table[A XOR CRC[11:6]] */
            hashVal = snetFieldValueGet(regPtr, 8 * entryIndex, 6);

            entryIndex = hashVal ^ SMEM_U32_GET_FIELD(crcValue,12,4);
            /* PearsonHash = Table[B XOR {00,CRC[15:12]}] */
            hashVal = snetFieldValueGet(regPtr, 8 * entryIndex, 6);
        }
        else
        {
            /* CRC-6 hash sub-mode */
            __LOG(("CRC-6 hash mode"));
            crcSeed = SMEM_U32_GET_FIELD(regPtr[0], 8, 6);

            /* Calculate CRC 6 bit for input data */
            crcValue = simCalcHashFor70BytesCrc6((GT_U8)crcSeed, crcHashKey,
                                                SNET_LION_CRC_HEADER_HASH_BYTES_CNS);
            hashVal = crcValue;
        }
    }

    descrPtr->pktHash = hashVal;

    __LOG(("the CRC hash value is [0x%x] \n",
        descrPtr->pktHash));

    return;
}

/*******************************************************************************
* snetLionCutThroughTrigger
*
* DESCRIPTION:
*        Cut-Through Mode Triggering
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*
* OUTPUT:
*       descrPtr        - pointer to frame data buffer Id.
*
* RETURN:
*
* COMMENTS:
*       Cut-through mode can be enabled per source port and priority.
*       The priority in this context is the user priority field in the VLAN tag:
*           - A packet is identified as VLAN tagged for cut-through purposes
*           if its Ethertype (TPID) is equal to one of two configurable VLAN Ethertypes.
*           - For cascade ports, the user priority is taken from the DSA tag.
*
*
*******************************************************************************/
GT_VOID snetLionCutThroughTrigger
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionCutThroughTrigger);

    GT_U32 regAddr;                     /* Register's address */
    GT_U32 *regPtr;                     /* Register entry pointer */
    GT_U32 fieldVal;                    /* Register's field value */
    GT_U32 rxDmaLocalPort;              /* local port for the rxDMA unit registers */
    GT_U32 enableForUntagged;           /* CT enabled for untagged */
    GT_U32  unitIndex = SMEM_DUAL_UNIT_DMA_UNIT_INDEX_GET(devObjPtr,descrPtr->ingressRxDmaPortNumber);

    rxDmaLocalPort = descrPtr->ingressRxDmaPortNumber;

    regAddr = SMEM_LION_RXDMA_CT_PACKET_IDENTIFY_REG(devObjPtr,rxDmaLocalPort);
    smemRegFldGet(devObjPtr, regAddr, 0, 11, &fieldVal);
    if (fieldVal == 0 || (descrPtr->byteCount <= (fieldVal - 1) * 8))
    {
        /* Packet size less then Minimal Cut-Through Byte Count */
        __LOG(("Cut Through forwarding disabled -  Packet size less then Minimal Cut-Through Byte Count[%d]",descrPtr->byteCount));
        return;
    }

    if(! SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION_RXDMA_CT_ENABLE_PER_PORT_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], rxDmaLocalPort, 1);
        /* Enable Cut Through forwarding for untagged packets received on source port */
        enableForUntagged = SMEM_U32_GET_FIELD(regPtr[0], 16 + rxDmaLocalPort, 1);
    }
    else
    {
        regAddr = SMEM_LION3_RXDMA_SCDMA_CONFIG_0_REG(devObjPtr,rxDmaLocalPort);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /*Cut Through Enable*/
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 7, 1);
        /*Cut Through Untagged Enable*/
        enableForUntagged = SMEM_U32_GET_FIELD(regPtr[0], 8 , 1);
    }

    if(fieldVal == 0)
    {
        /* Cut Through forwarding disabled -  on source rxDmaLocalPort */
        __LOG(("Cut Through forwarding disabled -  on source rxDmaLocalPort[%d]",rxDmaLocalPort));
        return;
    }

    if (descrPtr->tagSrcTagged[SNET_CHT_TAG_0_INDEX_CNS] || descrPtr->marvellTagged)
    {

        if(descrPtr->marvellTagged == 0)
        {
            regAddr = SMEM_LION_RXDMA_CT_ETHERTYPE_REG(devObjPtr,unitIndex);
            /* CT VLAN EtherType 0 */
            __LOG(("CT VLAN EtherType 0"));
            smemRegGet(devObjPtr, regAddr,&fieldVal);
            /* VLAN Tags ethertypes not match */
            if(descrPtr->vlanEtherType != SMEM_U32_GET_FIELD(fieldVal,0,16) &&
               descrPtr->vlanEtherType != SMEM_U32_GET_FIELD(fieldVal,16,16))
            {
                /* Cut Through forwarding disabled - VLAN Tags ethertypes not match */
                __LOG(("Cut Through forwarding disabled  - ethertype [0x%4.4x] not match",descrPtr->vlanEtherType));
                return;
            }
        }

        regAddr = SMEM_LION_RXDMA_CT_UP_REG(devObjPtr,unitIndex);
        /* UP Cut Through Enable  */
        smemRegFldGet(devObjPtr, regAddr, descrPtr->up, 1, &fieldVal);
        if(fieldVal == 0)
        {
            /* Cut Through forwarding disabled - for up */
            __LOG(("Cut Through forwarding disabled  - for up[%d]",descrPtr->up));
            return;
        }
    }
    else
    {
        if(enableForUntagged == 0)
        {
            /* Cut Through forwarding disabled - for untagged */
            __LOG(("Cut Through forwarding disabled  - for untagged"));
            return;
        }
    }

    __LOG(("Cut Through forwarding Enabled -  on source rxDmaLocalPort[%d]",rxDmaLocalPort));

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_CHT_BRDG_GLB_CONF1_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 7, 1, &fieldVal);
    }
    else
    {
        regAddr = SMEM_CHT_BRIDGE_GLOBAL_CONF2_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 20, 1, &fieldVal);
    }

    if(fieldVal)
    {
        /* Bypass Router and Policer */
        __LOG(("Bypass Router and Policer"));

        descrPtr->bypassRouterAndPolicer = 1;
    }

    /* Set value of desc<BC> associated to Cut Through packet */
    regAddr = SMEM_LION_MG_CT_GLOBAL_CONF_REG(devObjPtr);
    smemRegFldGet(devObjPtr, regAddr, 0, 14, &fieldVal);
    descrPtr->byteCount = fieldVal;

    __LOG(("new byte count[%d]",descrPtr->byteCount));

}

/*******************************************************************************
* snetLionHaKeepVlan1Check
*
* DESCRIPTION:
*        Ha - check for forcing 'keeping VLAN1' in the packet even if need to
*        egress without tag1 (when ingress with tag1)
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*       egressPort      - egress port
*
* OUTPUT:
*       none
*
* RETURN:
*
* COMMENTS:
*       Enable keeping VLAN1 in the packet, for packets received with VLAN1 and
*       even-though the tag-state of this {egress-port, VLAN0} is configured in
*       the VLAN-table to {untagged} or {VLAN0}.
*
*******************************************************************************/
GT_VOID snetLionHaKeepVlan1Check
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr,
    IN GT_U32   egressPort
)
{
    DECLARE_FUNC_NAME(snetLionHaKeepVlan1Check);

    GT_U32 regAddr;                     /* Register's address */
    GT_U32 fieldVal;                    /* Register's field value */
    GT_U32 fieldOffset;                 /* Register's field offset */
    SKERNEL_FRAME_TR101_VLAN_EGR_TAG_STATE_ENT  newEgressTagState;

    /* Keep Vlan1 is supported in Lion and above devices */
    if (devObjPtr->supportKeepVlan1 == 0)
    {
        return;
    }

    switch(descrPtr->passangerTr101EgressVlanTagMode)
    {
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E:
        case SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E:
            /* originally wanted to egress without tag 1*/
            __LOG(("originally wanted to egress without tag 1 , egress port[%d] \n",
                egressPort));
            break;
        default:
            return;
    }

    switch(descrPtr->srcTagState)
    {
        case SKERNEL_FRAME_TR101_VLAN_INGR_TAG_UNTAGGED_E:
        case SKERNEL_FRAME_TR101_VLAN_INGR_TAG_TAG0_E:
            /* packet NOT came with Tag 1 */
            __LOG(("packet NOT came with Tag 1 , egress port[%d] \n",
                egressPort));
            return ;
        default:
            /* packet came with Tag 1 */
            __LOG(("packet came with Tag 1 , egress port[%d] \n",
                egressPort));
            break;
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.ha)
    {
        /* TC 0..7 keep VLAN1 enable */
        fieldVal =
            SMEM_LION3_HA_PHYSICAL_PORT_1_ENTRY_FIELD_AUTO_GET(devObjPtr,
                descrPtr,
                SMEM_LION3_HA_PHYSICAL_PORT_TABLE_1_FIELDS_PER_UP0_KEEP_VLAN1_ENABLE);

        /* get the proper bit */
        fieldVal = SMEM_U32_GET_FIELD(fieldVal,descrPtr->up,1);
    }
    else
    {
        /* Keep VLAN1 <p> Register where p(0-15) represents port div 4 */
        regAddr =
            SMEM_LION_HA_UP0_PORT_KEEP_VLAN1_TBL_MEM(devObjPtr, (egressPort / 4));

        /* Port<p*4+n> UP0 Keep VLAN1 were p(0-15) represents port div 4 and n - port mod 4*/
        fieldOffset = (egressPort % 4) * 8 + descrPtr->up;
        smemRegFldGet(devObjPtr, regAddr, fieldOffset, 1, &fieldVal);
    }

    /* Tag state assigned by VLT is preserved */
    if(fieldVal == 0)
    {
        __LOG(("not forcing to 'Keep VLAN1' on UP[%d] egress port[%d] \n",
            descrPtr->up,egressPort));
        return;
    }

    if(descrPtr->passangerTr101EgressVlanTagMode == SKERNEL_FRAME_TR101_VLAN_EGR_TAG_UNTAGGED_E)
    {
        __LOG(("Keep tag1 : State that should egress vlan tag state should be 'TAG1'  \n"));
        newEgressTagState = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG1_E;
    }
    else  /*descrPtr->passangerTr101EgressVlanTagMode == SKERNEL_FRAME_TR101_VLAN_EGR_TAG_TAG0_E*/
    {
        __LOG(("Keep tag1 : State that should egress vlan tag state should be 'OUT_TAG0_IN_TAG1'  \n"));
        newEgressTagState = SKERNEL_FRAME_TR101_VLAN_EGR_TAG_OUT_TAG0_IN_TAG1_E;
    }


    descrPtr->passangerTr101EgressVlanTagMode =  newEgressTagState;
}

/*******************************************************************************
* snetLionTxqDeviceMapTableAccessCheck
*
* DESCRIPTION:
*           Check if need to access the Device map table.
*
* INPUTS:
*           devObjPtr       - pointer to device object.
*           descrPtr        - pointer to the frame's descriptor.
*           srcPort         - the source port
*                               Global for port group shared device.
*                               Local(the same value as localDevSrcPort)
*                               for non-port group device.
*           trgDev          - the target device
*           trgPort         - the target port
* OUTPUTS:
*           none
* RETURNS:
*           GT_TRUE     - device map need to be accessed
*           GT_FALSE    - device map NOT need to be accessed
* COMMENTS:
*           from the FS:
*           The device map table can also be accessed when the target device is
*           the local device.
*           This capability is useful in some modular systems, where the device
*           is used in the line card (see Figure 164).
*           In some cases it is desirable for incoming traffic from the network
*           ports to be forwarded through the fabric when the target device is
*           the local device. This is done by performing a device map lookup for
*           the incoming upstream traffic.
*           Device map lookup for the local device is based on a per-source-port
*           and per-destination-port configuration. The device map table is thus
*           accessed if one of the following conditions holds:
*           1. The target device is not the local device.
*           2. The target device is the local device and both of the following
*              conditions hold:
*              A. The local source port is enabled for device map lookup for
*                 local device according to the per-source-port configuration.
*              B. The target1 port is enabled for device map lookup for local
*                 device according to the per-destination-port configuration.
*
*******************************************************************************/
GT_BOOL snetLionTxqDeviceMapTableAccessCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 srcPort,
    IN GT_U32 trgDev,
    IN GT_U32 trgPort
)
{
    GT_U32 regAddr;             /* Register address */
    GT_U32 * regPtr;            /* Register data pointer */
    GT_U32 fldValue;            /* Register field value */
    GT_U32 globalSrcPort;       /* global source port */
    GT_U32 txqUnitSrcPort;      /* Source port number in TxQ unit */
    GT_U32 egrUnitMgPortGroup;  /* managment port group for EGR unit */
    GT_U32 egrSubUnit;          /* EGR subunit either 0 or 1 */
    SKERNEL_DEVICE_OBJECT * orig_devObjPtr = devObjPtr;/* original device  */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* set the device that should handle the EGF */
        devObjPtr = orig_devObjPtr;
        egrSubUnit = 0;/* ignored anyway*/
    }
    else
    {
        /* calculate EGR units bitmap:
         Tagret ports define hemisphere and source ports define EGR unit
         within hemisphere:
         EGR0 - EGR7 (hemisphere #0) used for target ports 0..63
         EGR8 - EGR15 (hemisphere #1) used for target ports 64..127.
         Source port's port group define EGR unit withing hemisphere as next:
         - hemisphere #0 - EGR unit = source port group;
         - hemisphere #1 - EGR unit = source port group + 8
         Each managment port group is responsible for two EGRs:
         MG0 - ERG0-1, MG1 - ERG2-3, MG3 - ERG4-5,.. MG7 - ERG14-15,
         srcPort is global one in the scope of ingress hemisphere (values 0..63).
         use ingess device info to get source hemisphere number. */
        globalSrcPort = ((descrPtr->ingressDevObjPtr->portGroupId >> 2) << 6) | srcPort;
        egrUnitMgPortGroup = descrPtr->txqId * 4 + (globalSrcPort >> 5);
        /* set the device that should handle the EGF */
        devObjPtr =  (orig_devObjPtr->portGroupSharedDevObjPtr) ?
                      orig_devObjPtr->portGroupSharedDevObjPtr->coreDevInfoPtr[egrUnitMgPortGroup].devObjPtr :
                      orig_devObjPtr;
        egrSubUnit = (srcPort >> 4) & 1;
    }

    /* The target device is the local device (own device)*/
    if (SKERNEL_IS_MATCH_DEVICES_MAC(trgDev, descrPtr->ownDev,
                                     devObjPtr->dualDeviceIdEnable.txq))
    {
        if(!devObjPtr->supportDevMapTableOnOwnDev)
        {
            return GT_FALSE;
        }

        regAddr = SMEM_LION_TXQ_LOCAL_SRC_PORT_MAP_OWN_DEV_EN_REG(devObjPtr, egrSubUnit);
        regPtr = smemMemGet(devObjPtr, regAddr);

        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* no converts needed - this is global port 0..255 */
            txqUnitSrcPort = srcPort;
        }
        else
        {

            /* Lion2 device with two hemisphere special treatment.
               Registers in hemisphere#0 are used as next:
                - bits 0..63   - global ports 0..63 and global ports 64..127, ignore MSB

               Registers in hemisphere#1 are used as next:
                - bits 64..127 - global ports 0..63 and global ports 64..127, ignore MSB
            */
            txqUnitSrcPort = (descrPtr->txqId == 0) ? (srcPort & 0x3f) : ((srcPort & 0x3f) + 64);
        }

        fldValue = snetFieldValueGet(regPtr, txqUnitSrcPort, 1);
        if(fldValue == 0)
        {
            /* The local source port is disabled for device map lookup for local device
            according to the per-source-port configuration. */
            return GT_FALSE;
        }

        regAddr = SMEM_LION_TXQ_LOCAL_TRG_PORT_MAP_OWN_DEV_EN_REG(devObjPtr, egrSubUnit);
        regPtr = smemMemGet(devObjPtr, regAddr);

        fldValue = snetFieldValueGet(regPtr,trgPort,1);
        if(fldValue == 0)
        {
            /* The target port is disabled for device map lookup for local device
            according to the per-destination-port configuration. */
            return GT_FALSE;
        }
    }

    return GT_TRUE;
}


/*******************************************************************************
* snetLionResourceHistogramCount
*
* DESCRIPTION:
*       Gathering information about the resource utilization of the Transmit
*       Queues
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to frame data buffer Id.
*
* OUTPUT:
*
* RETURN:
*
* COMMENTS:
*       For every successful packet enqueue, if the Global Descriptors Counter
*       exceeds the Threshold(n), the Histogram Counter(n) is incremented by 1.
*
*
*******************************************************************************/
GT_VOID snetLionResourceHistogramCount
(
    IN SKERNEL_DEVICE_OBJECT                * devObjPtr,
    INOUT SKERNEL_FRAME_CHEETAH_DESCR_STC   * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionResourceHistogramCount);

    GT_U32 regAddr;                     /* Register's address */
    GT_U32 fieldVal;                    /* Register's field value */
    GT_U32 *regHistTreshPtr;            /* Resource Histogram threshold entry pointer */
    GT_U32 *regHistCountPtr;            /* Resource Histogram counter entry pointer */
    GT_U32 globalDescrCount;            /* Global descriptors counter */
    GT_U32 limitIndex;                  /* Histogram limit index */
    GT_U32 wordIndex;
    GT_U32 fieldOffset;

    /* The Resource Histogram is not supported */
    if (devObjPtr->supportResourceHistogram == 0)
    {
        return;
    }

    /* Total Desc Counter Register */
    regAddr = SMEM_LION_TXQ_TOTAL_DESC_COUNT_REG(devObjPtr);
    /* Number of descriptors queued in all of the devices transmit queues */
    smemRegFldGet(devObjPtr, regAddr, 0, 14, &globalDescrCount);
    if(globalDescrCount != 0)
    {
        /* Decrement descriptors count for calculation, because one descriptor is in use! */
        __LOG(("Decrement descriptors count from [0x%x] \n",globalDescrCount));
        globalDescrCount--;
    }
    else
    {
        __LOG(("not decrementing count on current 'last' descriptor (counter already 0)\n"));
    }

    /* Resource Histogram Limit Register 1 */
    regAddr = SMEM_LION_TXQ_RES_HIST_LIMIT_1_REG(devObjPtr);
    regHistTreshPtr = smemMemGet(devObjPtr, regAddr);

    /* Resource Histogram Counter <n> Register (n=03) */
    regAddr = SMEM_LION_TXQ_RES_HIST_COUNT_REG(devObjPtr);
    regHistCountPtr = smemMemGet(devObjPtr, regAddr);

    for(limitIndex = 0; limitIndex < 4; limitIndex++)
    {
        wordIndex = limitIndex / 2;
        fieldOffset = (limitIndex % 2) * 14;
        /* Resource Histogram Limit <limitIndex> */
        fieldVal =
            SMEM_U32_GET_FIELD(regHistTreshPtr[wordIndex], fieldOffset, 14);

        if(globalDescrCount > fieldVal)
        {
            __LOG(("Increment Resource Histogram Counter[%d] from [0x%x] \n",
                limitIndex,
                regHistCountPtr[limitIndex]));

            regHistCountPtr[limitIndex] += 1;
            regHistCountPtr[limitIndex] &= 0x3FFF;/* 14 bits counter */
        }
    }
}

/*******************************************************************************
*   snetLionPtpMessageParsing
*
* DESCRIPTION:
*        Parsing of PTP message header
*
* INPUTS:
*        devObjPtr  - pointer to device object
*        descrPtr   - pointer to the frame's descriptor
*
* OUTPUT:
*        gtsEntryPtr - pointer to timestamp entry
*
*******************************************************************************/
static GT_VOID snetLionPtpMessageParsing
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    OUT SNET_LION_GTS_ENTRY_STC * gtsEntryPtr
)
{
    DECLARE_FUNC_NAME(snetLionPtpMessageParsing);

    #define PTP_HEADER_BYTES    32      /* PTP header length in bytes */

    GT_U8 * ptpMessageHeaderPtr;
    GT_U32 ptpMessageHeaderOffset;

    if(descrPtr->udpCompatible
       && descrPtr->l4StartOffsetPtr != NULL)
    {
        /* PTP over User Datagram Protocol (UDP) over IPv4 or IPv6 */
        __LOG(("PTP over User Datagram Protocol (UDP) over IPv4 or IPv6"));
        ptpMessageHeaderPtr = &descrPtr->l4StartOffsetPtr[8];
    }
    else
    {
        /* PTP over IEEE 802.3 / Ethernet */
        __LOG(("PTP over IEEE 802.3 / Ethernet"));
        ptpMessageHeaderPtr = &descrPtr->afterVlanOrDsaTagPtr[2];
    }
    /* Calculate PTP message offset */
    __LOG(("Calculate PTP message offset"));
    ptpMessageHeaderOffset = ptpMessageHeaderPtr - descrPtr->startFramePtr;
    if(ptpMessageHeaderOffset + PTP_HEADER_BYTES > descrPtr->byteCount)
    {
        /* PTP message length exceeds packet length */
        __LOG(("PTP message length exceeds packet length"));
        memset(gtsEntryPtr, 0, sizeof(SNET_LION_GTS_ENTRY_STC));
        return;
    }

    gtsEntryPtr->msgType = ptpMessageHeaderPtr[0] & 0xf;
    gtsEntryPtr->transportSpecific = ptpMessageHeaderPtr[0] >> 4;
    gtsEntryPtr->seqID = ptpMessageHeaderPtr[30] << 8 | ptpMessageHeaderPtr[31];

    __LOG_PARAM(gtsEntryPtr->msgType);
    __LOG_PARAM(gtsEntryPtr->transportSpecific);
    __LOG_PARAM(gtsEntryPtr->seqID);
}

/*******************************************************************************
*   snetLionPtpMessageCheck
*
* DESCRIPTION:
*        PTP Packet Identification
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*       GT_TRUE     - packet is PTP message
*       GT_FALSE    - packet is not PTP message
*
*******************************************************************************/
static GT_BOOL snetLionPtpMessageCheck
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction
)
{
    DECLARE_FUNC_NAME(snetLionPtpMessageCheck);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_BIT ptpOverUdpEnable = 0;        /* PTP over UDP enabling flag - sip5 relevant only*/
    GT_BIT ptpOverEthernetEnable = 0;   /* PTP over Ethernet enabling flag - sip5 relevant only*/
    GT_BIT udpCompatible;               /* UDP over IPvX indication */

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION3_TTI_PTP_CONFIG_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);

        ptpOverUdpEnable = SMEM_U32_GET_FIELD(*regPtr, 1,  1);
        ptpOverEthernetEnable = SMEM_U32_GET_FIELD(*regPtr, 0,  1);
    }
    else
    {
        if(descrPtr->mpls)
        {
            /* The MPLS packet could not be identified as a PTP packet */
            __LOG(("The MPLS packet could not be identified as a PTP packet"));
            return GT_FALSE;
        }

        if(descrPtr->ingressTunnelInfo.innerFrameDescrPtr)
        {
            /* Not tunnel terminated tunnels could not be identified as a PTP packet */
            __LOG(("Not tunnel terminated tunnels could not be identified as a PTP packet"));
            return GT_FALSE;
        }
    }

    SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,udpCompatible);

    if(udpCompatible)
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(ptpOverUdpEnable != 1)
            {
                /* PTP over UDP not enabled */
                __LOG(("PTP over UDP not enabled"));
                return GT_FALSE;
            }

            regAddr = SMEM_LION3_TTI_PTP_OVER_UDP_DESTINATION_PORTS_REG(devObjPtr);
        }
        else
        {
            regAddr = SMEM_LION_TTI_PTP_OVER_UDP_DEST_PORT_REG(devObjPtr);
        }

        regPtr = smemMemGet(devObjPtr, regAddr);

        if(descrPtr->l4DstPort == SMEM_U32_GET_FIELD(regPtr[0], 0, 16) ||
           descrPtr->l4DstPort == SMEM_U32_GET_FIELD(regPtr[0], 16, 16))
        {
            /* PTP packet received */
            __LOG(("PTP packet received"));
            return GT_TRUE;
        }
    }
    else
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            if(ptpOverEthernetEnable != 1)
            {
                /* PTP over Ethernet not enabled */
                __LOG(("PTP over Ethernet not enabled"));
                return GT_FALSE;
            }
            regAddr = SMEM_LION3_TTI_PTP_ETHERTYPES_REG(devObjPtr);
        }
        else
        {
            regAddr = SMEM_LION_TTI_PTP_ETHERTYPES_REG(devObjPtr);
        }

        regPtr = smemMemGet(devObjPtr, regAddr);

        if(descrPtr->etherTypeOrSsapDsap == SMEM_U32_GET_FIELD(regPtr[0], 0, 16) ||
           descrPtr->etherTypeOrSsapDsap == SMEM_U32_GET_FIELD(regPtr[0], 16, 16))
        {
            /* PTP packet received */
            __LOG(("PTP packet received"));
            return GT_TRUE;
        }
    }

    return GT_FALSE;
}

/*******************************************************************************
*   snetLion3PtpExceptionHandling
*
* DESCRIPTION:
*        PTP Exception Counting & Command
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id.
*
*******************************************************************************/
static GT_VOID snetLion3PtpExceptionHandling
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion3PtpExceptionHandling);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fldValue;                    /* register field value */
    SKERNEL_EXT_PACKET_CMD_ENT  currentPacketCmd;
    GT_U32  currentCpuCode;
    GT_U32  ptpIntBit;/*interrupt bit for the PTP in the register*/

    if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr))
    {
        ptpIntBit = 3;
    }
    else
    {
        ptpIntBit = 6;
    }

    /* Generate PTP Exception interrupt */
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_LION3_TTI_INTERRUPT_CAUSE_REG(devObjPtr),
                          SMEM_LION3_TTI_INTERRUPT_MASK_REG(devObjPtr),
                          (1 << ptpIntBit),
                          (GT_U32)SMEM_LION3_TTI_SUM_INT(devObjPtr));

    descrPtr->isPtpException = 1;

    /* PTP Exception Counter increment */
    regAddr = SMEM_LION3_TTI_PTP_EXCEPTIONS_COUNT_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);
    fldValue = SMEM_U32_GET_FIELD(*regPtr, 0,  8);
    if(fldValue == 0xFF)
    {
        fldValue = 0;
    }
    else
    {
        fldValue++;
    }
    smemRegFldSet(devObjPtr, regAddr, 0, 8, fldValue);

    regAddr = SMEM_LION3_TTI_PTP_EXCEPTIONS_AND_CPU_CODE_CONFIG_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if(SMEM_U32_GET_FIELD(*regPtr, 0,  1))
    {
        /* PTP Exception command assignment & Resolution */

        if( descrPtr->packetCmd >= SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E )
        {
            /* No command resolution will be done if packet command is not within the basic 5 commands */
            __LOG(("No command resolution will be done if packet command is not within the basic 5 commands"));
            return;
        }

        currentPacketCmd = SMEM_U32_GET_FIELD(*regPtr, 1,  3);
        currentCpuCode = SMEM_U32_GET_FIELD(*regPtr, 4,  8);

        /* Resolve packet command and CPU code */
        snetChtIngressCommandAndCpuCodeResolution(devObjPtr, descrPtr,
                                                  descrPtr->packetCmd,
                                                  currentPacketCmd,
                                                  descrPtr->cpuCode,
                                                  currentCpuCode,
                                                  SNET_CHEETAH_ENGINE_UNIT_TTI_E,
                                                  GT_TRUE);

    }
}

/*******************************************************************************
*   snetLion3PtpExtendedExceptionChecking
*
* DESCRIPTION:
*        PTP Exception Checking beyond the Basic mode
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id.
*       ptpMessageHeaderOffset - PTP header offset.
*
* RETURN:
*       GT_TRUE     - no PTP exception occured.
*       GT_FALSE    - PTP exception occured.
*
*******************************************************************************/
static GT_BOOL snetLion3PtpExtendedExceptionChecking
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U8 ptpMessageHeaderOffset
)
{
    DECLARE_FUNC_NAME(snetLion3PtpExtendedExceptionChecking);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldData;                   /* register field value */

    #define BUFFER_HEADER_BYTES    128

    regAddr =
        SMEM_LION3_TTI_PTP_EXCEPTION_CHECK_MODE_DOMAIN_REG(devObjPtr,descrPtr->ptpDomain);
    regPtr = smemMemGet(devObjPtr, regAddr);

    fieldData =
        SMEM_U32_GET_FIELD(*regPtr, (descrPtr->ptpGtsInfo.gtsEntry.msgType * 2),  2);

    if( fieldData == LION3_PTP_INGRESS_EXCEPTION_MODE_BC_E )
    {
        if( ((descrPtr->ptpGtsInfo.gtsEntry.ptpVersion == 2) &&
            ((ptpMessageHeaderOffset + 44) > BUFFER_HEADER_BYTES )) ||
            ((descrPtr->ptpGtsInfo.gtsEntry.ptpVersion == 1) &&
            ((ptpMessageHeaderOffset + 48) > BUFFER_HEADER_BYTES )) )
        {
            /* PTP Exception due to Boundary Clock mode */
            __LOG(("PTP Exception due to Boundary Clock mode"));
            return GT_FALSE;
        }
    }
    else if( fieldData == LION3_PTP_INGRESS_EXCEPTION_MODE_PIGGY_BACKED_E )
    {
        if( (descrPtr->ptpGtsInfo.gtsEntry.ptpVersion == 2) &&
            ((ptpMessageHeaderOffset + 20) > BUFFER_HEADER_BYTES) )
        {
            /* PTP Exception due to PiggyBacked mode */
            __LOG(("PTP Exception due to PiggyBacked mode"));
            return GT_FALSE;
        }
    }
    else if( fieldData == LION3_PTP_INGRESS_EXCEPTION_MODE_TC_E )
    {
        if( (descrPtr->ptpGtsInfo.gtsEntry.ptpVersion == 2) &&
            ((ptpMessageHeaderOffset + 16) > BUFFER_HEADER_BYTES) )
        {
            /* PTP Exception due to Transparent Clock mode */
            __LOG(("PTP Exception due to Transparent Clock mode"));
            return GT_FALSE;
        }
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetLion3PtpMessageParsing
*
* DESCRIPTION:
*        PTP Packet Parsing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id.
*       srcPort     - pecifies the local source port.
*
* RETURN:
*       GT_TRUE     - PTP message has been triggered
*       GT_FALSE    - PTP message has not been triggered
*
*******************************************************************************/
static GT_BOOL snetLion3PtpMessageParsing
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 srcPort
)
{
    DECLARE_FUNC_NAME(snetLion3PtpMessageParsing);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 regData;                     /* register value */

    SNET_LION_GTS_ENTRY_STC * gtsEntryPtr = &descrPtr->ptpGtsInfo.gtsEntry;

    GT_U8 *ptpMessageHeaderPtr;
    GT_U8  ptpMessageHeaderOffset;

    GT_BIT udpCompatible;               /* UDP over IPvX indication */
    GT_BIT isIPv4;                      /* IPv4 or IPv6 indication */
    GT_U32 pktDomain;                   /* packet domain entry */
    GT_U32 domainN;                     /* loop iterator */
    GT_U32 ii;
    GT_BOOL V1SubdomainMatch;           /* indication of V1 subdomain match to entry */
    GT_BOOL retVal;                     /* return status*/
    GT_BIT  piggybacked_timestamp_enable;

    if(descrPtr->isPtp)
    {
        if( ((descrPtr->ptpTriggerType == SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_IPV4_UDP_E) ||
             (descrPtr->ptpTriggerType == SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_IPV6_UDP_E)) &&
              (descrPtr->l4Valid == 0))
        {
            /* PTP identification in the TTI Action as PTP over UDP but L4Valid is not valid */
            __LOG(("PTP identification in the TTI Action as PTP over UDP but L4Valid is not valid"));
            snetLion3PtpExceptionHandling(devObjPtr,descrPtr);
            return GT_FALSE;
        }

        /* PTP header based on identification in the TTI Action */
        __LOG(("PTP header based on identification in the TTI Action"));
        ptpMessageHeaderPtr = descrPtr->l3StartOffsetPtr + descrPtr->ptpOffset;
    }
    else
    {
        SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,udpCompatible);

        if(udpCompatible
           && descrPtr->l4StartOffsetPtr != NULL)
        {
            if(descrPtr->l4Valid == 0)
            {
                /* PTP identification as PTP over UDP but L4Valid is not valid */
                __LOG(("PTP identification as PTP over UDP but L4Valid is not valid"));
                snetLion3PtpExceptionHandling(devObjPtr,descrPtr);
                return GT_FALSE;
            }

            ptpMessageHeaderPtr = &descrPtr->l4StartOffsetPtr[8];

            SKERNEL_CHT_DESC_INNER_FRAME_FIELD_GET_MAC(descrPtr,isIPv4);
            if(isIPv4)
            {
                /* PTP over User Datagram Protocol (UDP) over IPv4 */
                __LOG(("PTP over User Datagram Protocol (UDP) over IPv4 "));
                descrPtr->ptpTriggerType = SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_IPV4_UDP_E;
            }
            else
            {
                /* PTP over User Datagram Protocol (UDP) over IPv6 */
                __LOG(("PTP over User Datagram Protocol (UDP) over IPv6 "));
                descrPtr->ptpTriggerType = SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_IPV6_UDP_E;
            }
        }
        else
        {
            /* PTP over IEEE 802.3 / Ethernet */
            __LOG(("PTP over IEEE 802.3 / Ethernet"));
            ptpMessageHeaderPtr = &descrPtr->afterVlanOrDsaTagPtr[2];
            descrPtr->ptpTriggerType = SKERNEL_PTP_TRIGGER_TYPE_PTP_OVER_L2_E;
        }
    }

    descrPtr->ptpGtsInfo.ptpMessageHeaderPtr = ptpMessageHeaderPtr;
    ptpMessageHeaderOffset = ptpMessageHeaderPtr - descrPtr->startFramePtr;

    /* PTP version check */
    gtsEntryPtr->ptpVersion = ptpMessageHeaderPtr[1] & 0xf;

    if((gtsEntryPtr->ptpVersion) != 1 && (gtsEntryPtr->ptpVersion != 2))
    {
        regAddr = SMEM_LION3_TTI_PTP_EXCEPTIONS_AND_CPU_CODE_CONFIG_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);

        if(SMEM_U32_GET_FIELD(*regPtr, 22,  1))
        {
            /* PTP Exception due to version field not 1 or 2 */
            __LOG(("PTP Exception due to version field not 1 or 2"));
            snetLion3PtpExceptionHandling(devObjPtr,descrPtr);
        }

        /* PTP version field is not 1 or 2 */
        __LOG(("PTP version field is not 1 or 2"));

        return GT_FALSE;
    }

    descrPtr->isPtp = 1;
    descrPtr->ptpGtsInfo.ptpPacketTriggered = 1;

    /* Extracting PTP Header Fields */

    if( gtsEntryPtr->ptpVersion == 2 )
    {
        if((GT_U32)(ptpMessageHeaderOffset + 5) > descrPtr->byteCount)
        {
            /* PTP Exception due to first 5 bytes of PTPv2 header missing */
            __LOG(("PTP Exception due to first 5 bytes of PTPv2 header missing"));
            snetLion3PtpExceptionHandling(devObjPtr,descrPtr);

            return GT_FALSE;
        }

        /* PTPv2 extracting */
        __LOG(("PTPv2 extracting"));
        gtsEntryPtr->msgType = ptpMessageHeaderPtr[0] & 0xf;
        gtsEntryPtr->transportSpecific = ptpMessageHeaderPtr[0] >> 4;
        gtsEntryPtr->V2DomainNumber = ptpMessageHeaderPtr[4];

        __LOG_PARAM(gtsEntryPtr->msgType);
        __LOG_PARAM(gtsEntryPtr->transportSpecific);
        __LOG_PARAM(gtsEntryPtr->V2DomainNumber);
    }
    else /* PTPv1 */
    {
        if((GT_U32)(ptpMessageHeaderOffset + 21) > descrPtr->byteCount)
        {
            /* PTP Exception due to first 21 bytes of PTPv1 header missing */
            __LOG(("PTP Exception due to first 5 bytes of PTPv2 header missing"));
            snetLion3PtpExceptionHandling(devObjPtr,descrPtr);

            return GT_FALSE;
        }
        /* PTPv1 extracting */
        __LOG(("PTPv1 extracting"));
        gtsEntryPtr->msgType = ptpMessageHeaderPtr[20] & 0xf;
        gtsEntryPtr->transportSpecific = 0;
        gtsEntryPtr->V1Subdomain[0] = SNET_BUILD_WORD_FROM_BYTES_PTR_MAC((ptpMessageHeaderPtr+16));
        gtsEntryPtr->V1Subdomain[1] = SNET_BUILD_WORD_FROM_BYTES_PTR_MAC((ptpMessageHeaderPtr+12));
        gtsEntryPtr->V1Subdomain[2] = SNET_BUILD_WORD_FROM_BYTES_PTR_MAC((ptpMessageHeaderPtr+8));
        gtsEntryPtr->V1Subdomain[3] = SNET_BUILD_WORD_FROM_BYTES_PTR_MAC((ptpMessageHeaderPtr+4));

        __LOG_PARAM(gtsEntryPtr->msgType);
        __LOG_PARAM(gtsEntryPtr->transportSpecific);
        __LOG_PARAM(gtsEntryPtr->V1Subdomain[0]);
        __LOG_PARAM(gtsEntryPtr->V1Subdomain[1]);
        __LOG_PARAM(gtsEntryPtr->V1Subdomain[2]);
        __LOG_PARAM(gtsEntryPtr->V1Subdomain[3]);
    }

    gtsEntryPtr->seqID = SNET_BUILD_HALF_WORD_FROM_BYTES_PTR_MAC((ptpMessageHeaderPtr+30));
    __LOG_PARAM(gtsEntryPtr->msgType);

    /* Timestamp Tagged Update */
    if((gtsEntryPtr->ptpVersion == 2) &&
       (descrPtr->timestampTagged[SMAIN_DIRECTION_INGRESS_E] ==
        SKERNEL_TIMESTAMP_TAG_TYPE_UNTAGGED_E))
    {
        if(descrPtr->eArchExtInfo.ttiPhysicalPort2AttributePtr)
        {
            piggybacked_timestamp_enable = SMEM_SIP5_20_TTI_PHYSICAL_PORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                SMEM_LION3_TTI_PHYSICAL_PORT_2_TABLE_FIELD_PIGGYBACKED_TIMESTAMP_ENABLE);
        }
        else
        {
            regAddr = SMEM_LION3_TTI_PTP_PIGGY_BACK_TS_CONFIG_REG(devObjPtr,(srcPort/32));
            regPtr = smemMemGet(devObjPtr, regAddr);
            piggybacked_timestamp_enable = SMEM_U32_GET_FIELD(*regPtr, (srcPort%32),  1);
        }

        if(piggybacked_timestamp_enable)
        {
            /* Timestamp Tag type replaced to PiggyBacked */
            __LOG(("Timestamp Tag type replaced to PiggyBacked \n"));
            descrPtr->timestampTagged[SMAIN_DIRECTION_INGRESS_E] =
                SKERNEL_TIMESTAMP_TAG_TYPE_PIGGY_BACKED_E;
        }
    }

    if( (descrPtr->timestampTagged[SMAIN_DIRECTION_INGRESS_E] ==
         SKERNEL_TIMESTAMP_TAG_TYPE_HYBRID_E) ||
        (descrPtr->timestampTagged[SMAIN_DIRECTION_INGRESS_E] ==
         SKERNEL_TIMESTAMP_TAG_TYPE_PIGGY_BACKED_E) )
    {
        /* Hybrid\Piggybacked timestamp tag info in <reserved> */
        descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].timestamp.nanoSecondTimer =
            SNET_BUILD_WORD_FROM_BYTES_PTR_MAC((ptpMessageHeaderPtr+16));
        /* The 2 lsbits are U & T */
        descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].U =
            SMEM_U32_GET_FIELD(descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].timestamp.nanoSecondTimer, 0,  1);
        descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].T =
            SMEM_U32_GET_FIELD(descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].timestamp.nanoSecondTimer, 1,  1);
        descrPtr->timestampTagInfo[SMAIN_DIRECTION_INGRESS_E].timestamp.nanoSecondTimer &= 0xFFFFFFFC;
    }

    /* PTP Domain Resolution */

    regAddr = SMEM_LION3_TTI_PTP_CONFIG_REG(devObjPtr);
    regPtr = smemMemGet(devObjPtr, regAddr);
    regData = *regPtr;

    pktDomain = 4; /* default assignment */
    for(domainN = 0; domainN < 4; domainN++)
    {
        if(gtsEntryPtr->ptpVersion == 1)
        {
            if( SMEM_U32_GET_FIELD(regData, ((domainN+1)*2),  2) == 1 )
            {
                V1SubdomainMatch = GT_TRUE;
                for(ii = 0 ; ii < 4; ii++)
                {
                    regAddr = SMEM_LION3_TTI_PTP_1588_V1_DOMAIN_NUMBER_REG(devObjPtr,domainN,ii);
                    regPtr = smemMemGet(devObjPtr, regAddr);
                    if(*regPtr != gtsEntryPtr->V1Subdomain[ii])
                    {
                        V1SubdomainMatch = GT_FALSE;
                    }
                }
                if(V1SubdomainMatch == GT_TRUE)
                {
                    /* PTP Domain Resolution found match for PTPv1 */
                    __LOG(("PTP Domain Resolution found match for PTPv1"));
                    pktDomain = domainN;
                }
            }
        }
        else /* gtsEntryPtr->ptpVersion == 2 */
        {
            if( SMEM_U32_GET_FIELD(regData, ((domainN+1)*2),  2) == 2 )
            {
                regAddr = SMEM_LION3_TTI_PTP_1588_V2_DOMAIN_NUMBER_REG(devObjPtr);
                regPtr = smemMemGet(devObjPtr, regAddr);
                if(SMEM_U32_GET_FIELD(*regPtr, (domainN*8) ,  8) == gtsEntryPtr->V2DomainNumber)
                {
                    /* PTP Domain Resolution found match for PTPv2 */
                    __LOG(("PTP Domain Resolution found match for PTPv2"));
                    pktDomain = domainN;
                }
            }
        }
    }

    descrPtr->ptpDomain = pktDomain;

    /* PTP "Extended" Exception Checking */
    retVal = snetLion3PtpExtendedExceptionChecking(devObjPtr, descrPtr, ptpMessageHeaderOffset);
    if (retVal == GT_FALSE)
    {
        /* PTP Exception due to mode different then basic mode */
        __LOG(("PTP Exception due to mode different then basic mode"));
        snetLion3PtpExceptionHandling(devObjPtr,descrPtr);
    }

    /* Due to "extended" exception no timestamping should be done */
    if (retVal == GT_FALSE)
    {
        return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetLion3PtpTodGetTimeCounter
*
* DESCRIPTION:
*        Get the TOD content
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1
*
* OUTPUT:
*       timeCounterPtr - (pointer to) the TOD contetnt
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetLion3PtpTodGetTimeCounter
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst,
    OUT SNET_TOD_TIMER_STC *timeCounterPtr
)
{
    GT_U32 clocks;              /* TOD value in clocks */
    GT_U32 seconds;             /* TOD seconds */
    GT_U32 nanoSeconds;         /* TOD nanoseconds */
    GT_U64 seconds64;           /* 64 bits seconds value */

    /* Current TOD value */
    clocks = SNET_LION3_TOD_CLOCK_GET_MAC(devObjPtr, taiGroup, taiInst);
    /* Convert tick clocks to seconds and nanoseconds */
    SNET_TOD_CLOCK_FORMAT_MAC(clocks, seconds, nanoSeconds);

    *timeCounterPtr = devObjPtr->eTod[taiGroup][taiInst];
    timeCounterPtr->nanoSecondTimer += nanoSeconds;

    /* Convert timer data to 64 bit value*/
    seconds64.l[0] = seconds;
    seconds64.l[1] = 0;
    timeCounterPtr->secondTimer =
        prvSimMathAdd64(timeCounterPtr->secondTimer, seconds64);
}

/*******************************************************************************
*   snetLion3TimestampInfo
*
* DESCRIPTION:
*        Get ingress tiemstamp info
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id.
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetLion3TimestampInfo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLion3TimestampInfo);
    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 taiGroup;                    /* the TAI group used for that port */

    if(descrPtr->ingressGopPortNumber == SMAIN_NOT_VALID_CNS)
    {
        __LOG(("The ingress port [%d] not hold valid GOP port number , so no timestamp support \n" ,
            descrPtr->localDevSrcPort));

        return;
    }

    if(devObjPtr->supportSingleTai == 0)
    {
        taiGroup = SNET_LION3_PORT_NUM_TO_TOD_GROUP_CONVERT_MAC(devObjPtr ,descrPtr->ingressGopPortNumber);
        /* PTP TAI Select */
        regAddr = SMEM_LION3_GOP_PTP_GENERAL_CTRL_REG(devObjPtr, descrPtr->ingressGopPortNumber);
        regPtr = smemMemGet(devObjPtr, regAddr);

        descrPtr->ptpTaiSelect = SMEM_U32_GET_FIELD(*regPtr, 5,  1);
    }
    else
    {
        taiGroup = 0;
        descrPtr->ptpTaiSelect = 0;
    }

    /* Timestamp */
    snetLion3PtpTodGetTimeCounter(devObjPtr,
                                  taiGroup,
                                  descrPtr->ptpTaiSelect,
                                  &descrPtr->timestamp[SMAIN_DIRECTION_INGRESS_E]);
}

/*******************************************************************************
*   snetLionPtpMessageTrigger
*
* DESCRIPTION:
*        PTP Packet triggering
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       srcTrgPort  - For ingress timestamping this fields specifies the local source port,
*                     while for egress timestamping this field specifies the local target port.
*       direction   - ingress or egress direction
*
* RETURN:
*       GT_TRUE     - PTP message has been triggered
*       GT_FALSE    - PTP message has not been triggered
*
*******************************************************************************/
static GT_BOOL snetLionPtpMessageTrigger
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 srcTrgPort,
    IN SMAIN_DIRECTION_ENT direction
)
{
    DECLARE_FUNC_NAME(snetLionPtpMessageTrigger);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldVal;                    /* register's field value */
    GT_U32 fieldOffset;                 /* register's field offset */
    GT_STATUS retVal;
    GT_U32 globalRegValue;              /* global register value */
    SNET_LION_GTS_ENTRY_STC * gtsEntryPtr = &descrPtr->ptpGtsInfo.gtsEntry;

    if( direction == SMAIN_DIRECTION_EGRESS_E )
    {
        descrPtr->ptpGtsInfo.ptpPacketTriggered = 0;
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        snetLion3TimestampInfo(devObjPtr, descrPtr);
    }

    if (descrPtr->ptpGtsInfo.ptpPacketTriggered == 0)
    {
        retVal = snetLionPtpMessageCheck(devObjPtr, descrPtr, direction);
        if(retVal == GT_FALSE)
        {
            /* Packet not recognized as PTP message */
            __LOG(("Packet not recognized as PTP message"));
            return GT_FALSE;
        }
    }

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        retVal = snetLion3PtpMessageParsing(devObjPtr, descrPtr, srcTrgPort);
        if(retVal == GT_FALSE)
        {
            /* PTP was not found suitable for parsing or timestamping*/
            __LOG(("PTP was not found suitable for parsing or timestamping"));
            return GT_FALSE;
        }

        return GT_TRUE;
    }

    descrPtr->ptpGtsInfo.ptpPacketTriggered = 1;

    /* Get data from PTP packet header */
    snetLionPtpMessageParsing(devObjPtr, descrPtr, gtsEntryPtr);

    /* Global Configurations */
    regAddr = SMEM_LION_GTS_SIP_GLOBAL_REG(devObjPtr, direction);
    smemRegGet(devObjPtr, regAddr, &globalRegValue);

    /* Check global configurations */
    if(descrPtr->udpCompatible)
    {
        if(descrPtr->isIPv4)
        {
            /* PTP over User Datagram Protocol (UDP) over IPv4 disabled */
            if (snetFieldValueGet(&globalRegValue, 1, 1) == 0)
            {
                __LOG(("PTP over User Datagram Protocol (UDP) over IPv4 disabled"));
                return GT_FALSE;
            }
        }
        else
        {
            /* PTP over User Datagram Protocol (UDP) over IPv6 disabled */
            if (snetFieldValueGet(&globalRegValue, 2, 1) == 0)
            {
                __LOG(("PTP over User Datagram Protocol (UDP) over IPv6 disabled"));
                return GT_FALSE;
            }
        }
    }
    else
    {
        /* PTP over IEEE 802.3 / Ethernet disabled */
        if (snetFieldValueGet(&globalRegValue, 0, 1) == 0)
        {
            __LOG(("PTP over IEEE 802.3 / Ethernet disabled"));
            return GT_FALSE;
        }
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.tti &&
        direction == SMAIN_DIRECTION_INGRESS_E)
    {
        /* the PTP is in the GTS and not in the TTI ... use the GTS registers like in Lion2 */
        if(srcTrgPort != SNET_CHT_CPU_PORT_CNS)
        {
            srcTrgPort &= 0xf;
        }
    }

    if(devObjPtr->supportEArch && devObjPtr->unitEArchEnable.ha &&
        direction == SMAIN_DIRECTION_EGRESS_E)
    {
        /* <PTP_TIMESTAMP_TAG_MODE> */
        fieldVal =
            SMEM_LION3_HA_PHYSICAL_PORT_2_ENTRY_FIELD_GET(devObjPtr,
                descrPtr->eArchExtInfo.haEgressPhyPort2TablePtr,
                srcTrgPort,
                SMEM_LION3_HA_PHYSICAL_PORT_TABLE_2_FIELDS_PTP_TIMESTAMP_TAG_MODE);

    }
    else
    {
        /* Check is port enabled for PTP timestamping */
        regAddr = SMEM_LION_GTS_TIME_STAMP_PORT_EN_REG(devObjPtr, direction);
        regPtr = smemMemGet(devObjPtr, regAddr);
        fieldVal = snetFieldValueGet(regPtr, srcTrgPort, 1);
    }

    if(fieldVal == 0)
    {
        /* This port is disabled for PTP timestamping */
        __LOG(("This port [%d] is disabled for PTP timestamping \n",
            srcTrgPort));
        return GT_FALSE;
    }

    /* Check is timestamping enabled for the specific messageType and
    transportSpecific of the packet */
    regAddr = SMEM_LION_GTS_TIME_STAMP_EN_REG(devObjPtr, direction);
    regPtr = smemMemGet(devObjPtr, regAddr);

    fieldOffset = gtsEntryPtr->msgType + 16;
    fieldVal = snetFieldValueGet(regPtr, fieldOffset, 1);
    if(fieldVal == 0)
    {
        /* PTP packet with this message type is disabled for timestamping */
        __LOG(("PTP packet with this message type is disabled for timestamping \n"));
        return GT_FALSE;
    }

    fieldOffset = gtsEntryPtr->transportSpecific;
    fieldVal = snetFieldValueGet(regPtr, fieldOffset, 1);
    if(fieldVal == 0)
    {
        /* Enable transportSpecific Check */
        if(snetFieldValueGet(&globalRegValue, 3, 1))
        {
            /* PTP packet with this transport specific type is disabled for timestamping */
            __LOG(("PTP packet with this transport specific type is disabled for timestamping"));
            return GT_FALSE;
        }
    }

    gtsEntryPtr->srcTrgPort = srcTrgPort;
    __LOG_PARAM(gtsEntryPtr->srcTrgPort);
    /* Current packet is PTP triggered */
    __LOG(("port [%d] : Current packet is PTP triggered \n",
        srcTrgPort));

    return GT_TRUE;
}

/*******************************************************************************
*   snetLionPtpTimestampMessageInterrupt
*
* DESCRIPTION:
*        PTP Timestamp Queue interrupts
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       direction   - ingress or egress direction
*       interruptEvent - cause interrupt event
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetLionPtpTimestampMessageInterrupt
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN SNET_LION_PTP_GTS_INTERRUPT_EVENTS_ENT interruptEvent
)
{
    DECLARE_FUNC_NAME(snetLionPtpTimestampMessageInterrupt);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 globalIntBmp;                /* global interrupt cause bitmap */
    GT_U32 causeIntBmp;                 /* cause interrupt cause bitmap */
    GT_U32 ptpIntBit = 3;/*interrupt bit for the PTP in the summary register*/

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* we should not get here */
        /*
            NOTE: function is NOT for SIP5 devices !!!

            function called only from snetLionSendPtpMessageToCpu(...)

            function snetLionSendPtpMessageToCpu called only for NON sip5 devices.
        */
        skernelFatalError("snetLionPtpTimestampMessageInterrupt: not for SIP5 devices \n");
    }


    if(direction == SMAIN_DIRECTION_INGRESS_E)
    {
        regAddr = SMEM_LION_TTI_SUM_CAUSE_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* TTI cause bit */
        globalIntBmp = 1 << devObjPtr->globalInterruptCauseRegister.tti;
    }
    else
    {
        regAddr = SMEM_LION_HEP_INTR_CAUSE_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);
        /* HA cause bit */
        globalIntBmp = 1 << devObjPtr->globalInterruptCauseRegister.ha;
    }

    /* ITS/ETS Summary Cause Bit*/
    SMEM_U32_SET_FIELD(regPtr[0], ptpIntBit, 1, 1);
    /* SUM Cause Bit */
    SMEM_U32_SET_FIELD(regPtr[0], 0, 1, 1);

    /* ITS/ETS Summary Mask Bit*/
    if (SMEM_U32_GET_FIELD(regPtr[1], ptpIntBit, 1) == 0)
    {
        /* Interrupt masked */
        __LOG(("Interrupt masked"));
        return;
    }

    if(interruptEvent == SNET_LION_PTP_GTS_INTERRUPT_GLOBAL_FIFO_FULL_E)
    {
        /* Global FIFO Full */
        causeIntBmp = LION_PTP_GTS_GLOBAL_FIFO_FULL_INTERRUPT_E;
    }
    else
    {
        /* Valid Timestamp Entry */
        causeIntBmp = LION_PTP_GTS_VALID_ENTRY_INTERRUPT_E;
    }


    /* generate interrupt */
    __LOG(("generate interrupt"));
    snetChetahDoInterrupt(devObjPtr,
                          SMEM_LION_GTS_INTERRUPT_CAUSE_REG(devObjPtr, direction),
                          SMEM_LION_GTS_INTERRUPT_MASK_REG(devObjPtr, direction),
                          causeIntBmp,
                          globalIntBmp);

}

/*******************************************************************************
*   snetLionSendPtpMessageToCpu
*
* DESCRIPTION:
*       Send PTP message to CPU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       tsEntryPtr  - pointer timestamp PTP message
*       direction   - ingress or egress direction

* RETURN:
*       GT_OK       - PTP message has been successfully sent to CPU
*       GT_FAIL     - PTP message sending to CPU failed
*
*******************************************************************************/
static GT_STATUS snetLionSendPtpMessageToCpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN SMAIN_DIRECTION_ENT direction
)
{
    DECLARE_FUNC_NAME(snetLionSendPtpMessageToCpu);

    GT_STATUS status;                       /* function return status */
    GT_U32 * fifoBufferPtr;                 /* fifo memory pointer */
    GT_U32 * freeFifoEntryPtr = 0;          /* pointer to free fifo entry */
    GT_U32 i;                               /* fifo index index */

    /* Set pointer to the common memory */
    SMEM_CHT_DEV_COMMON_MEM_INFO  * memInfoPtr =
        (SMEM_CHT_DEV_COMMON_MEM_INFO *)(devObjPtr->deviceMemory);

    SNET_LION_GTS_ENTRY_STC * gtsEntryPtr = &descrPtr->ptpGtsInfo.gtsEntry;

    if(direction == SMAIN_DIRECTION_INGRESS_E)
    {
        fifoBufferPtr = &memInfoPtr->gtsIngressFifoMem.gtsFifoRegs[0];
    }
    else
    {
        fifoBufferPtr = &memInfoPtr->gtsEgressFifoMem.gtsFifoRegs[0];
    }


    for (i = 0; i < GTS_FIFO_REGS_NUM; i += GTS_PTP_MSG_WORDS)
    {
        if (fifoBufferPtr[i] != 0xffffffff)
        {
            continue;
        }

        freeFifoEntryPtr = &fifoBufferPtr[i];
        break;
    }

    if (freeFifoEntryPtr == 0)
    {
        /* PTP timestamp messages FIFO to CPU has exceeded its threshold */
        __LOG(("PTP timestamp messages FIFO to CPU has exceeded its threshold"));
        snetLionPtpTimestampMessageInterrupt(devObjPtr, descrPtr, direction,
                                             SNET_LION_PTP_GTS_INTERRUPT_GLOBAL_FIFO_FULL_E);
        status = GT_FAIL;
    }
    else
    {
        /* Put new message into FIFO */
        __LOG(("Put new message into FIFO"));
        SMEM_U32_SET_FIELD(freeFifoEntryPtr[0], 0,  6, gtsEntryPtr->srcTrgPort);
        SMEM_U32_SET_FIELD(freeFifoEntryPtr[0], 6,  4, gtsEntryPtr->msgType);
        SMEM_U32_SET_FIELD(freeFifoEntryPtr[0],10, 16, gtsEntryPtr->seqID);
        freeFifoEntryPtr[1] = descrPtr->packetTimestamp;

        /* Message to CPU is ready in the FIFO */
        __LOG(("Message to CPU is ready in the FIFO"));
        snetLionPtpTimestampMessageInterrupt(devObjPtr, descrPtr, direction,
                                             SNET_LION_PTP_GTS_INTERRUPT_VALID_ENTRY_E);
        status = GT_OK;
    }

    return status;
}

/*******************************************************************************
*   snetLionPtpIngressTimestampProcess
*
* DESCRIPTION:
*        Ingress PTP Timestamp Processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionPtpIngressTimestampProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionPtpIngressTimestampProcess);

    GT_BOOL retVal;

    if(devObjPtr->supportPtp == 0)
    {
        /* Device doesn't support PTP message processing */
        __LOG(("Device doesn't support PTP message processing"));
        return;
    }

   retVal = snetLionPtpMessageTrigger(devObjPtr, descrPtr,
                                       descrPtr->localDevSrcPort,
                                       SMAIN_DIRECTION_INGRESS_E);
    if(retVal == GT_FALSE)
    {
        /* PTP Message is not triggered */
        __LOG(("PTP Message is not triggered"));
        return;
    }

    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        /* Enqueue PTP message in a timestamp queue */
        __LOG(("Enqueue PTP message in a timestamp queue"));
        snetLionSendPtpMessageToCpu(devObjPtr, descrPtr, SMAIN_DIRECTION_INGRESS_E);
    }
}

/*******************************************************************************
*   snetLionHaPtpEgressTimestampProcess
*
* DESCRIPTION:
*        Ha - Egress PTP Timestamp Processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*       egressPort  - egress port
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionHaPtpEgressTimestampProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 egressPort
)
{
    DECLARE_FUNC_NAME(snetLionHaPtpEgressTimestampProcess);

    GT_BOOL retVal;
    SNET_LION_GTS_ENTRY_STC * gtsEntryPtr = &descrPtr->ptpGtsInfo.gtsEntry;

    if(devObjPtr->supportPtp == 0)
    {
        /* Device doesn't support PTP message processing */
        __LOG(("Device doesn't support PTP message processing"));
        return;
    }

    /* non SIP5 PTP processing */
    if(!SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        retVal = snetLionPtpMessageTrigger(devObjPtr, descrPtr, egressPort,
                                           SMAIN_DIRECTION_EGRESS_E);
        if(retVal == GT_FALSE)
        {
            /* PTP Message is not triggered */
            __LOG(("PTP Message is not triggered"));
            return;
        }

        /* Update packet's timestamp on egress */
        descrPtr->packetTimestamp = SIM_OS_MAC(simOsTickGet)();

        /* Convert egress local port to global port on egress PTP timestamp process */
        gtsEntryPtr->srcTrgPort =
                SMEM_CHT_GLOBAL_PORT_FROM_LOCAL_PORT_MAC(devObjPtr, egressPort);
        __LOG_PARAM(gtsEntryPtr->srcTrgPort);

        /* Enqueue PTP message in a timestamp queue */
        snetLionSendPtpMessageToCpu(devObjPtr, descrPtr, SMAIN_DIRECTION_EGRESS_E);
    }
}

/*******************************************************************************
*   snetLionPtpCommandResolution
*
* DESCRIPTION:
*        PTP Trapping/Mirroring to the CPU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - frame data buffer Id
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionPtpCommandResolution
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetLionPtpCommandResolution);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 *regPtr;                     /* register entry pointer */
    GT_U32 fieldVal;                    /* register's field value */
    GT_U32 ptpConfData[2];              /* PTP Packet Command Configuration */
    GT_U32 startBit;
    SKERNEL_EXT_PACKET_CMD_ENT  currentPacketCmd;
    GT_U32  currentCpuCode;

    SNET_LION_GTS_ENTRY_STC * gtsEntryPtr;

    if(devObjPtr->supportPtp == 0)
    {
        /* Device doesn't support PTP message processing */
        __LOG(("Device doesn't support PTP message processing"));
        return;
    }

    if (descrPtr->ptpGtsInfo.ptpPacketTriggered == 0)
    {
        /* Packet is not PTP message type or not triggered by ingress/egress PTP mechanism */
        __LOG(("Packet is not PTP message type or not triggered by ingress/egress PTP mechanism"));
        return;
    }

    if( descrPtr->packetCmd >= SKERNEL_EXT_PKT_CMD_TO_TRG_SNIFFER_E )
    {
        /* No command resolution will be done if packet command is not within the basic 5 commands */
        __LOG(("No command resolution will be done if packet command is not within the basic 5 commands"));
        return;
    }

    gtsEntryPtr = &descrPtr->ptpGtsInfo.gtsEntry;

    if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
    {
        regAddr = SMEM_LION3_TTI_PTP_EXCEPTIONS_AND_CPU_CODE_CONFIG_REG(devObjPtr);
        regPtr = smemMemGet(devObjPtr, regAddr);

        /* PTP CPU Code */
        currentCpuCode = SMEM_U32_GET_FIELD(*regPtr, 14,  8);

        if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
        {
            fieldVal = SMEM_SIP5_20_TTI_PHYSICAL_PORT_ENTRY_FIELD_GET(devObjPtr,descrPtr,
                (SMEM_LION3_TTI_PHYSICAL_PORT_2_TABLE_FIELD_MESSAGE_TYPE_00_DOMAIN_X_PTP_PACKET_COMMAND + gtsEntryPtr->msgType));

            startBit = descrPtr->ptpDomain * 3;
            /* PTP Message Type <n>, domain number <d> Packet Command */
            currentPacketCmd = SMEM_U32_GET_FIELD(fieldVal, startBit, 3);
        }
        else
        {
            regAddr = SMEM_LION3_TTI_PTP_PACKET_CMD_TBL_MEM(devObjPtr,(descrPtr->localDevSrcPort));
            regPtr = smemMemGet(devObjPtr, regAddr);

            startBit = (gtsEntryPtr->msgType * 0x10) + (descrPtr->ptpDomain * 3);

            /* PTP Message Type <n>, domain number <d> Packet Command */
            currentPacketCmd = snetFieldValueGet(regPtr, startBit, 3);
        }
    }
    else
    {
        /* Read two control configuration registers */
        regAddr = SMEM_LION_TTI_PTP_PACKET_CMD_CONF0_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 0, 32, &ptpConfData[0]);
        regAddr = SMEM_LION_TTI_PTP_PACKET_CMD_CONF1_REG(devObjPtr);
        smemRegFldGet(devObjPtr, regAddr, 0, 32, &ptpConfData[1]);

        startBit = gtsEntryPtr->msgType * 3 + ((gtsEntryPtr->msgType > 9) ? 2 : 0) ;

        /* PTP Message Type <n> Packet Command, where 0 <= n <=15  */
        currentPacketCmd = snetFieldValueGet(ptpConfData, startBit, 3);

        /* PTP CPU Code Base */
        fieldVal = snetFieldValueGet(&ptpConfData[1], 18, 8);
        currentCpuCode = gtsEntryPtr->msgType + fieldVal;
    }

    /* Resolve packet command and CPU code */
    snetChtIngressCommandAndCpuCodeResolution(devObjPtr, descrPtr,
                                              descrPtr->packetCmd,
                                              currentPacketCmd,
                                              descrPtr->cpuCode,
                                              currentCpuCode,
                                              SNET_CHEETAH_ENGINE_UNIT_TTI_E,
                                              GT_TRUE);
}

/*******************************************************************************
*   snetLionPtpTodCounterRead
*
* DESCRIPTION:
*        Read TOD Counter to internal structure
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*       regDataPtr  - pointer to registers data
*
* OUTPUT:
*       todTimerPtr - TOD counter event type
*
* RETURN:
*
*
*******************************************************************************/
static GT_VOID snetLionPtpTodCounterRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN GT_U32 * regDataPtr,
    OUT SNET_TOD_TIMER_STC * todTimerPtr
)
{
    GT_U32 clocks;                          /* TOD value in clocks */
    GT_U32 seconds;                         /* TOD seconds */
    GT_U32 nanoSeconds;                     /* TOD nanoseconds */
    GT_U64 seconds64;                       /* 64 bits seconds value */

    /* Current TOD value */
    clocks = SNET_LION_TOD_CLOCK_GET_MAC(devObjPtr, direction);
    /* Convert tick clocks to seconds and nanoseconds */
    SNET_TOD_CLOCK_FORMAT_MAC(clocks, seconds, nanoSeconds);

    /* Nano timer */
    todTimerPtr->nanoSecondTimer = regDataPtr[0] + nanoSeconds;
    /* Convert TOD second timer data to 64 bit value */
    todTimerPtr->secondTimer.l[0] = regDataPtr[1];
    todTimerPtr->secondTimer.l[1] = regDataPtr[2];
    /* Convert timer data to 64 bit value*/
    seconds64.l[0] = seconds;
    seconds64.l[1] = 0;
    todTimerPtr->secondTimer =
        prvSimMathAdd64(todTimerPtr->secondTimer, seconds64);
}

/*******************************************************************************
*   snetLionPtpTodCounterWrite
*
* DESCRIPTION:
*        Write TOD Counter data
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*       regDataPtr  - pointer to registers data
*       todTimerPtr - TOD counter event type
*
* OUTPUT:
*       regDataPtr  - pointer to registers data
*
* RETURN:
*
*
*******************************************************************************/
static GT_VOID snetLionPtpTodCounterWrite
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    INOUT GT_U32 * regDataPtr,
    IN SNET_TOD_TIMER_STC * todTimerPtr
)
{
    /* Update TOD counter register */
    regDataPtr[0] = todTimerPtr->nanoSecondTimer;
    regDataPtr[1] = todTimerPtr->secondTimer.l[0];
    regDataPtr[2] = todTimerPtr->secondTimer.l[1];
}

/*******************************************************************************
*   snetLionPtpTodCounterApply
*
* DESCRIPTION:
*        TOD Counter Functions
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*       todEvent    - TOD counter event type
*
* RETURN:
*
* COMMENTS: The TOD counter supports four types of time-driven and time-setting
*           functions  Update, Increment, Capture, and Generate.
*           These functions use a shadow register, which has exactly the same format
*           as the TOD counter
*
*******************************************************************************/
GT_VOID snetLionPtpTodCounterApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    IN SNET_LION_PTP_TOD_EVENT_ENT todEvent
)
{
    GT_U32 regAddr;                         /* register address */
    GT_U32 * regCopyFromPtr;                /* register pointer */
    GT_U32 * regCopyToPtr;                  /* register pointer */
    SNET_TOD_TIMER_STC todTimer;            /* TOD timer */
    GT_U64 seconds64;                       /* 64 bits seconds value */

    switch(todEvent)
    {
        /* Copies the value from the TOD counter shadow to the TOD counter register */
        case SNET_LION_PTP_TOD_UPDATE_E:
            regAddr =
                SMEM_LION_GTS_TOD_SHADOW_COUNT_NANOSEC_REG(devObjPtr, direction);
            regCopyFromPtr = smemMemGet(devObjPtr, regAddr);

            regAddr = SMEM_LION_GTS_TOD_COUNT_NANOSEC_REG(devObjPtr, direction);
            regCopyToPtr = smemMemGet(devObjPtr, regAddr);
            /* Copy IPFix timer data: nano timer, second LSb timer, second MSb timer */
            memcpy(regCopyToPtr, regCopyFromPtr, 3 * sizeof(GT_U32));

            break;
        /* Adds the value of the TOD counter shadow to the TOD counter register */
        case SNET_LION_PTP_TOD_INCREMENT_E:
            regAddr =
                SMEM_LION_GTS_TOD_SHADOW_COUNT_NANOSEC_REG(devObjPtr, direction);
            regCopyFromPtr = smemMemGet(devObjPtr, regAddr);

            regAddr = SMEM_LION_GTS_TOD_COUNT_NANOSEC_REG(devObjPtr, direction);
            regCopyToPtr = smemMemGet(devObjPtr, regAddr);

            /* Read TOD counter value first */
            snetLionPtpTodCounterRead(devObjPtr, direction, regCopyToPtr, &todTimer);

            todTimer.nanoSecondTimer += regCopyFromPtr[0];
            /* Convert shadow second timer data to 64 bit value */
            seconds64.l[0] = regCopyFromPtr[1];
            seconds64.l[1] = regCopyFromPtr[2];

            todTimer.secondTimer =
                prvSimMathAdd64(todTimer.secondTimer, seconds64);
            /* Write TOD counter */
            snetLionPtpTodCounterWrite(devObjPtr, direction, regCopyToPtr, &todTimer);

            break;
        /* Copies the value of the TOD counter to the TOD counter shadow register */
        case SNET_LION_PTP_TOD_CAPTURE_E:
            regAddr = SMEM_LION_GTS_TOD_COUNT_NANOSEC_REG(devObjPtr, direction);
            regCopyFromPtr = smemMemGet(devObjPtr, regAddr);

            regAddr = SMEM_LION_GTS_TOD_SHADOW_COUNT_NANOSEC_REG(devObjPtr, direction);
            regCopyToPtr = smemMemGet(devObjPtr, regAddr);

            /* Read TOD counter value first */
            snetLionPtpTodCounterRead(devObjPtr, direction, regCopyFromPtr, &todTimer);
            /* Write TOD shadow counter */
            snetLionPtpTodCounterWrite(devObjPtr, direction, regCopyToPtr, &todTimer);

            break;
        case SNET_LION_PTP_TOD_GENERATE_E:
            /* Simulation currently does not support this hardware generated event */
        default:
            break;
    }
}

/*******************************************************************************
*   snetLionPtpTodCounterSecondsRead
*
* DESCRIPTION:
*        Read TOD counter seconds
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*
* OUTPUT:
*       seconds     - TOD counter seconds
*
* RETURN:
*
*
*******************************************************************************/
GT_VOID snetLionPtpTodCounterSecondsRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    OUT GT_U64 * seconds64
)
{
    GT_U32 regAddr;                         /* register address */
    GT_U32 * regPtr;                        /* register pointer */
    SNET_TOD_TIMER_STC todTimer;            /* TOD timer */

    regAddr = SMEM_LION_GTS_TOD_COUNT_NANOSEC_REG(devObjPtr, direction);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Read TOD counter value */
    snetLionPtpTodCounterRead(devObjPtr, direction, regPtr, &todTimer);

    seconds64->l[0] = todTimer.secondTimer.l[0];
    seconds64->l[1] = todTimer.secondTimer.l[1];
}

/*******************************************************************************
*   snetLionPtpTodCounterNanosecondsRead
*
* DESCRIPTION:
*        Read TOD counter nanoseconds
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       direction   - ingress or egress direction
*
* OUTPUT:
*       nanoseconds - TOD counter nanoseconds
*
* RETURN:
*
*******************************************************************************/
GT_VOID snetLionPtpTodCounterNanosecondsRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMAIN_DIRECTION_ENT direction,
    OUT GT_U32 * nanoseconds
)
{
    GT_U32 regAddr;                         /* register address */
    GT_U32 * regPtr;                        /* register pointer */
    SNET_TOD_TIMER_STC todTimer;            /* TOD timer */

    regAddr = SMEM_LION_GTS_TOD_COUNT_NANOSEC_REG(devObjPtr, direction);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Read TOD counter value */
    snetLionPtpTodCounterRead(devObjPtr, direction, regPtr, &todTimer);

    *nanoseconds = todTimer.nanoSecondTimer;
}

/*******************************************************************************
*   snetLion3PtpTodGetLoadValue
*
* DESCRIPTION:
*        Get the timer format of Time Load Value
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1
*
* OUTPUT:
*       timeLoadValuePtr - (pointer to) Load Time value
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetLion3PtpTodGetLoadValue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst,
    OUT SNET_TOD_TIMER_STC *timeLoadValuePtr
)
{
    DECLARE_FUNC_NAME(snetLion3PtpTodGetLoadValue);
    GT_U32 regAddr;         /* register address */
    GT_U32 *regPtr;         /* register pointer */

    if ((devObjPtr->supportSingleTai) && ((taiGroup != 0) || (taiInst != 0)))
    {
        __LOG(("single TAI unit specified by not null taiGroup or taiInst \n"));
        taiGroup = 0;
        taiInst  = 0;
    }

    /* seconds */
    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_SEC_LOW_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->secondTimer.s[0] = SMEM_U32_GET_FIELD(*regPtr, 0,  16);

    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_SEC_MED_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->secondTimer.s[1] = SMEM_U32_GET_FIELD(*regPtr, 0,  16);

    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_SEC_HIGH_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->secondTimer.l[1] = SMEM_U32_GET_FIELD(*regPtr, 0,  16);

    /* nanoseconds */
    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_NANO_LOW_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->nanoSecondTimer = SMEM_U32_GET_FIELD(*regPtr, 0,  16);

    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_NANO_HIGH_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->nanoSecondTimer += (SMEM_U32_GET_FIELD(*regPtr, 0,  16) << 16);

    /* fractional nanoseconds */
    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_FRAC_LOW_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->fractionalNanoSecondTimer = SMEM_U32_GET_FIELD(*regPtr, 0,  16);


    regAddr = SMEM_LION3_GOP_TAI_LOAD_VALUE_FRAC_HIGH_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    timeLoadValuePtr->fractionalNanoSecondTimer += (SMEM_U32_GET_FIELD(*regPtr, 0,  16) << 16);
}

/*******************************************************************************
*   snetLion3PtpTodCaptureToReg
*
* DESCRIPTION:
*        Capture operation of TOD to capture registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1
*       timeCapture - captured time from TOD
*       captureSet  - capture registers set to write to, 0 or 1
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetLion3PtpTodCaptureToReg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst,
    IN SNET_TOD_TIMER_STC timeCapture,
    IN GT_U32 captureSet
)
{
    DECLARE_FUNC_NAME(snetLion3PtpTodCaptureToReg);
    GT_U32 regAddr;                         /* register address */
    GT_U32 *regPtr;                         /* register pointer */

    if ((devObjPtr->supportSingleTai) && ((taiGroup != 0) || (taiInst != 0)))
    {
        __LOG(("single TAI unit specified by not null taiGroup or taiInst \n"));
        taiGroup = 0;
        taiInst  = 0;
    }

    /* seconds */
    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_SEC_HIGH_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = timeCapture.secondTimer.s[2];

    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_SEC_MED_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = timeCapture.secondTimer.s[1];

    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_SEC_LOW_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = timeCapture.secondTimer.s[0];

    /* nanoseconds */
    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_NANO_HIGH_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = SMEM_U32_GET_FIELD(timeCapture.nanoSecondTimer, 16, 16);

    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_NANO_LOW_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = SMEM_U32_GET_FIELD(timeCapture.nanoSecondTimer, 0, 16);

    /* fractional nanoseconds */
    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_FRAC_HIGH_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = SMEM_U32_GET_FIELD(timeCapture.nanoSecondTimer, 16, 16);

    regAddr =
        SMEM_LION3_GOP_TAI_CAPTURE_VALUE_FRAC_LOW_REG(devObjPtr,taiGroup,taiInst,captureSet);
    regPtr = smemMemGet(devObjPtr, regAddr);

    *regPtr = SMEM_U32_GET_FIELD(timeCapture.nanoSecondTimer, 0, 16);

    /* set valid bit */
    regAddr = SMEM_LION3_GOP_TAI_CAPTURE_STATUS_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    SMEM_U32_SET_FIELD(*regPtr, captureSet, 1, 1);
}

/*******************************************************************************
*   snetLion3PtpTodCapture
*
* DESCRIPTION:
*        Capture operation of TOD
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1
*
* RETURN:
*
*******************************************************************************/
static GT_VOID snetLion3PtpTodCapture
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst
)
{
    DECLARE_FUNC_NAME(snetLion3PtpTodCapture);
    GT_U32 regAddr;                         /* register address */
    GT_U32 *regPtr;                         /* register pointer */
    SNET_TOD_TIMER_STC timeCapture;         /* time capture values */

    if ((devObjPtr->supportSingleTai) && ((taiGroup != 0) || (taiInst != 0)))
    {
        __LOG(("single TAI unit specified by not null taiGroup or taiInst \n"));
        taiGroup = 0;
        taiInst  = 0;
    }

    snetLion3PtpTodGetTimeCounter(devObjPtr, taiGroup, taiInst, &timeCapture);

    regAddr = SMEM_LION3_GOP_TAI_CAPTURE_STATUS_REG(devObjPtr,taiGroup,taiInst);
    regPtr = smemMemGet(devObjPtr, regAddr);

    if( SMEM_U32_GET_FIELD(*regPtr, 0, 1) == 0 )
    {
        snetLion3PtpTodCaptureToReg(devObjPtr, taiGroup, taiInst, timeCapture, 0);
    }
    else if ( SMEM_U32_GET_FIELD(*regPtr, 1, 1) == 0)
    {
        snetLion3PtpTodCaptureToReg(devObjPtr, taiGroup, taiInst, timeCapture, 1);
    }
    else
    {
        regAddr =
            SMEM_LION3_GOP_TAI_FUNC_CONFIG_0_REG(devObjPtr,taiGroup,taiInst);
        regPtr = smemMemGet(devObjPtr, regAddr);

        if( SMEM_U32_GET_FIELD(*regPtr, 6, 1) == 1 )
        {
            snetLion3PtpTodCaptureToReg(devObjPtr, taiGroup, taiInst, timeCapture, 0);
        }
    }
}

/*******************************************************************************
*   snetLion3PtpTodCounterApply
*
* DESCRIPTION:
*        TOD Counter Functions
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       todEvent    - TOD counter event type
*       taiGroup    - The TAI selected group - one of 9
*       taiInst     - within the selected group - TAI0 or TAI1

*
* RETURN:
*
* COMMENTS:
*           The functions are based on shadow\capture registers which have the
*           same format as the TOD counter
*
*******************************************************************************/
GT_VOID snetLion3PtpTodCounterApply
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SNET_LION3_PTP_TOD_FUNC_ENT todEvent,
    IN GT_U32 taiGroup,
    IN GT_U32 taiInst
)
{
    DECLARE_FUNC_NAME(snetLion3PtpTodCounterApply);
    GT_U32 regAddr;                         /* register address */
    GT_U32 *regPtr;                         /* register pointer */
    SNET_TOD_TIMER_STC timeLoadValue;       /* shadow value for operation */
    SNET_TOD_TIMER_STC localTimeValue;      /* auxilery timer structure */

    if ((devObjPtr->supportSingleTai) && ((taiGroup != 0) || (taiInst != 0)))
    {
        __LOG(("single TAI unit specified by not null taiGroup or taiInst \n"));
        taiGroup = 0;
        taiInst  = 0;
    }

    memset(&timeLoadValue, 0 , sizeof(SNET_TOD_TIMER_STC));
    if( (todEvent != SNET_LION3_PTP_TOD_CAPTURE_E) &&
        (todEvent != SNET_LION3_PTP_TOD_NOP_E) )
    {
        /* Load Value register data is not relevat to Capture or NOP */
        snetLion3PtpTodGetLoadValue(devObjPtr, taiGroup, taiInst, &timeLoadValue);
    }

    switch(todEvent)
    {
        /* Copies the value from the shadow timer to the timer. */
        case SNET_LION3_PTP_TOD_UPDATE_E:
            devObjPtr->eTod[taiGroup][taiInst] = timeLoadValue;
            break;
        /* Copies the value from the shadow timer to the fractional nanosecond drift. */
        case SNET_LION3_PTP_TOD_FREQ_UPDATE_E:
            regAddr =
                SMEM_LION3_GOP_TAI_DFIFT_ADJUST_CONFIG_LOW_REG(devObjPtr,taiGroup,taiInst);
            regPtr = smemMemGet(devObjPtr, regAddr);
            *regPtr =
                SMEM_U32_GET_FIELD(timeLoadValue.fractionalNanoSecondTimer, 0, 16);

            regAddr =
                SMEM_LION3_GOP_TAI_DFIFT_ADJUST_CONFIG_HIGH_REG(devObjPtr,taiGroup,taiInst);
            regPtr = smemMemGet(devObjPtr, regAddr);
            *regPtr =
                SMEM_U32_GET_FIELD(timeLoadValue.fractionalNanoSecondTimer, 16, 16);

            break;
        /* Adds or graceful increment the value of the shadow timer to the timer. */
        /* simulation support both operation the same way. */
        case SNET_LION3_PTP_TOD_INCREMENT_E:
        case SNET_LION3_PTP_TOD_GRACEFUL_INC_E:
        /* Subtracts or graceful decrement the value of the shadow timer to the timer. */
        /* simulation support both operation the same way. */
        case SNET_LION3_PTP_TOD_DECREMENT_E:
        case SNET_LION3_PTP_TOD_GRACEFUL_DEC_E:
            snetLion3PtpTodGetTimeCounter(devObjPtr,
                                          taiGroup,
                                          taiInst,
                                          &localTimeValue);

            devObjPtr->eTod[taiGroup][taiInst] = localTimeValue;

            if( (todEvent == SNET_LION3_PTP_TOD_INCREMENT_E) ||
                (todEvent == SNET_LION3_PTP_TOD_GRACEFUL_INC_E) )
            {
                /* increment function */
                devObjPtr->eTod[taiGroup][taiInst].nanoSecondTimer +=
                                            timeLoadValue.nanoSecondTimer;
            }
            else
            {
                /* decrement function */
                devObjPtr->eTod[taiGroup][taiInst].nanoSecondTimer -=
                                            timeLoadValue.nanoSecondTimer;
            }

            if( (todEvent == SNET_LION3_PTP_TOD_INCREMENT_E) ||
                (todEvent == SNET_LION3_PTP_TOD_GRACEFUL_INC_E) )
            {
                /* increment function */
                devObjPtr->eTod[taiGroup][taiInst].secondTimer =
                    prvSimMathAdd64(devObjPtr->eTod[taiGroup][taiInst].secondTimer,
                                timeLoadValue.secondTimer);
            }
            else
            {
                /* decrement function */
                devObjPtr->eTod[taiGroup][taiInst].secondTimer =
                    prvSimMathSub64(devObjPtr->eTod[taiGroup][taiInst].secondTimer,
                                timeLoadValue.secondTimer);
            }

            break;
        /* Copies the value of the timer to the capture area. */
        case SNET_LION3_PTP_TOD_CAPTURE_E:
            snetLion3PtpTodCapture(devObjPtr, taiGroup, taiInst);
            break;
        /* No operation is performed. */
        case SNET_LION3_PTP_TOD_NOP_E:
        default:
            break;
    }

    if( (todEvent == SNET_LION3_PTP_TOD_UPDATE_E) ||
        (todEvent == SNET_LION3_PTP_TOD_INCREMENT_E) ||
        (todEvent == SNET_LION3_PTP_TOD_DECREMENT_E) )
    {
        /* Time Update Counter */
        regAddr =
                SMEM_LION3_GOP_TAI_UPDATE_COUNTER_REG(devObjPtr,taiGroup,taiInst);
        regPtr = smemMemGet(devObjPtr, regAddr);
        *regPtr += 1;
    }
}
