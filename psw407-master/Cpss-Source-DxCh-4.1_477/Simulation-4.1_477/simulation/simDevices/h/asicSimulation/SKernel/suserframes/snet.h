/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snet.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 37 $
*
*******************************************************************************/
#ifndef __sneth
#define __sneth


#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BITS_IN_BYTE 8

/* Convert tick clocks to seconds and nanoseconds */
#define SNET_TOD_CLOCK_FORMAT_MAC(clock, sec, nanosec) \
{ \
    sec = (clock) / 1000; \
    nanosec = ((clock) % 1000) * 1000000; \
}

/*
 * Typedef: struct STRUCT_TX_DESC
 *
 * Description: Includes the PP Tx descriptor fields, to be used for handling
 *              packet transmits.
 *
 * Fields:
 *      word1           - First word of the Tx Descriptor.
 *      word2           - Second word of the Tx Descriptor.
 *      buffPointer     - The physical data-buffer address.
 *      nextDescPointer - The physical address of the next Tx descriptor.
 *
 */
typedef struct _txDesc
{
    /*volatile*/ GT_U32         word1;
    /*volatile*/ GT_U32         word2;

    /*volatile*/ GT_U32         buffPointer;
    /*volatile*/ GT_U32         nextDescPointer;
}SNET_STRUCT_TX_DESC;

/****** SDMA *********************************************/
/* Get / Set the Own bit field of the tx descriptor.    */
#define TX_DESC_GET_OWN_BIT(txDesc)                         \
            (((txDesc)->word1) >> 31)
#define TX_DESC_SET_OWN_BIT(txDesc,val)                     \
            (SMEM_U32_SET_FIELD((txDesc)->word1, 31, 1, val))

/* Get / Set the First bit field of the tx descriptor.  */
#define TX_DESC_GET_FIRST_BIT(txDesc)                       \
            ((((txDesc)->word1) >> 21) & 0x1)
#define TX_DESC_SET_FIRST_BIT(txDesc,val)                   \
            (SMEM_U32_SET_FIELD((txDesc)->word1, 21, 1, val))

/* If set, CRC of the packet should be recalculated on packet transmission */
#define TX_DESC_GET_RECALC_CRC_BIT(txDesc)                  \
            ((((txDesc)->word1) >> 12) & 0x1)

/* Get / Set the Last bit field of the tx descriptor.   */
#define TX_DESC_GET_LAST_BIT(txDesc)                        \
            ((((txDesc)->word1)>> 20) & 0x1)
#define TX_DESC_SET_LAST_BIT(txDesc,val)                    \
            (SMEM_U32_SET_FIELD((txDesc)->word1, 20, 1, val))

/* Get / Set the Int bit field of the tx descriptor.    */
#define TX_DESC_GET_INT_BIT(txDesc)                         \
            (((txDesc)->word1 >> 23) & 0x1)

#define TX_HEADER_SIZE      (16)  /* Bytes    */

/* Tx descriptor related macros.    */
#define TX_DESC_CPU_OWN     (0)
#define TX_DESC_DMA_OWN     (1)

/* Number of Transmit Queues in ASIC */
#define TRANS_QUEUE_MAX_NUMBER                       8

/* get the byte order that the PP should sent to Host CPU
NOTE : the field called 'Rx' but it is from HOST point of view
       so it is actually has impact on simulation that do 'tx to cpu'
*/
#define TX_BYTE_ORDER_MAC(devObjPtr) \
    (devObjPtr->rxByteSwap ? SCIB_DMA_WORDS : SCIB_DMA_BYTE_STREAM)

/* get the byte order that the PP should receive from the Host CPU
NOTE : the field called 'Tx' but it is from HOST point of view
       so it is actually has impact on simulation that do 'rx from cpu'
*/
#define RX_BYTE_ORDER_MAC(devObjPtr) \
    (devObjPtr->txByteSwap ? SCIB_DMA_WORDS : SCIB_DMA_BYTE_STREAM)


#define     SNET_FRAMES_1024_TO_MAX_OCTETS           1024
#define     SNET_FRAMES_512_TO_1023_OCTETS           512
#define     SNET_FRAMES_256_TO_511_OCTETS            256
#define     SNET_FRAMES_128_TO_255_OCTETS            128
#define     SNET_FRAMES_65_TO_127_OCTETS             65
#define     SNET_FRAMES_64_OCTETS                    64

#define SNET_GET_NUM_OCTETS_IN_FRAME(size) \
    (size >= SNET_FRAMES_1024_TO_MAX_OCTETS) ? SNET_FRAMES_1024_TO_MAX_OCTETS : \
    (size >= SNET_FRAMES_512_TO_1023_OCTETS) ? SNET_FRAMES_512_TO_1023_OCTETS : \
    (size >= SNET_FRAMES_256_TO_511_OCTETS) ? SNET_FRAMES_256_TO_511_OCTETS : \
    (size >= SNET_FRAMES_128_TO_255_OCTETS) ? SNET_FRAMES_128_TO_255_OCTETS : \
    (size >= SNET_FRAMES_65_TO_127_OCTETS) ? SNET_FRAMES_65_TO_127_OCTETS : \
    (size >= SNET_FRAMES_64_OCTETS) ? SNET_FRAMES_64_OCTETS : 0

/* Threshold between 1024to1518 and 1519toMax MIB counters is variable: */
#define SNET_MIB_CNT_THRESHOLD_1518  1518
#define SNET_MIB_CNT_THRESHOLD_1522  1522

typedef enum {
    SNET_SFLOW_RX = 0,
    SNET_SFLOW_TX
} SNET_SFLOW_DIRECTION_ENT;

/* apply mask on value and key , check if equal */
#define SNET_CHT_MASK_CHECK(value,mask,key) \
    (((value)&(mask)) == ((key)&(mask)))

/* build GT_U32 from pointer to 4 bytes */
#define SNET_BUILD_WORD_FROM_BYTES_PTR_MAC(bytesPtr)\
    ((GT_U32)(((bytesPtr)[0] << 24) | ((bytesPtr)[1]) << 16 | ((bytesPtr)[2]) << 8 | ((bytesPtr)[3] << 0)))

/* build GT_U16 from pointer to 2 bytes */
#define SNET_BUILD_HALF_WORD_FROM_BYTES_PTR_MAC(bytesPtr)\
    ((GT_U16)(((bytesPtr)[0] << 8) | ((bytesPtr)[1]) << 0))

/* Retrieve DSA tag from frame data buffer */
#define DSA_TAG(_frame_data_ptr)\
    (GT_U32)((_frame_data_ptr)[12] << 24 | (_frame_data_ptr)[13] << 16 |\
             (_frame_data_ptr)[14] << 8  | (_frame_data_ptr)[15])


/* build GT_U32 from pointer to 4 bytes */
#define SNET_BUILD_BYTES_FROM_WORD_MAC(word,bytesPtr)\
    bytesPtr[0] = (GT_U8)SMEM_U32_GET_FIELD(word,24,8);\
    bytesPtr[1] = (GT_U8)SMEM_U32_GET_FIELD(word,16,8);\
    bytesPtr[2] = (GT_U8)SMEM_U32_GET_FIELD(word, 8,8);\
    bytesPtr[3] = (GT_U8)SMEM_U32_GET_FIELD(word, 0,8)

#define SNET_GET_PCKT_TAG_ETHER_TYPE_MAC(descr, offset) \
                (((descr)->startFramePtr[(offset)] << 8) | \
                 ((descr)->startFramePtr[(offset) + 1]))

#define SNET_GET_PCKT_TAG_VLAN_ID_MAC(descr, offset) \
                (((descr)->startFramePtr[(offset) + 2] << 8) | \
                 ((descr)->startFramePtr[(offset) + 3])) & 0xfff

#define SNET_GET_PCKT_TAG_UP_MAC(descr, offset) \
                ((descr)->startFramePtr[(offset) + 2]) >> 5

#define SNET_GET_PCKT_TAG_CFI_DEI_MAC(descr, offset) \
                (((descr)->startFramePtr[(offset) + 2] >> 4)) & 0x1

/* indication to set SNET_ENTRY_FORMAT_TABLE_STC::startBit during run time */
#define FIELD_SET_IN_RUNTIME_CNS   0xFFFFFFFF
/* indication in SNET_ENTRY_FORMAT_TABLE_STC::previousFieldType that 'current'
field is consecutive to the previous field */
#define FIELD_CONSECUTIVE_CNS      0xFFFFFFFF

#define STR(strname)    \
    #strname

/*
 * Typedef: struct SNET_ENTRY_FORMAT_TABLE_STC
 *
 * Description: A structure to hold info about field in entry of table
 *
 * Fields:
 *      startBit  - start bit of the field. filled in runtime when value != FIELD_SET_IN_RUNTIME_CNS
 *      numOfBits - number of bits in the field
 *      previousFieldType - 'point' to the previous field for calculation of startBit.
 *                          used when != FIELD_CONSECUTIVE_CNS
 *
 */
typedef struct SNET_ENTRY_FORMAT_TABLE_STCT{
    GT_U32      startBit;
    GT_U32      numOfBits;
    GT_U32      previousFieldType;
}SNET_ENTRY_FORMAT_TABLE_STC;

/* macro to fill instance of SNET_ENTRY_FORMAT_TABLE_STC with value that good for 'standard' field.
'standard' field is a field that comes after the field that is defined before it in the array of fields

the macro gets the <numOfBits> of the current field.
*/
#define STANDARD_FIELD_MAC(numOfBits)     \
        {FIELD_SET_IN_RUNTIME_CNS,        \
         numOfBits,                       \
         FIELD_CONSECUTIVE_CNS}

/* macro to set <startBit> and <numOfBits>
the macro gets the <numOfBits> of the current field.
*/
#define EXPLICIT_FIELD_MAC(startBit,numOfBits)     \
        {startBit,        \
         numOfBits,                       \
         0/*don't care*/}


/********************************************************************************
 *  SNET_IPCL_LOOKUP_ENT
 *
 *  DESCRIPTION : Enum of the ingress PCL TCAM lookups
 *
********************************************************************************/
typedef enum{
    SNET_IPCL_LOOKUP_0_0_E = 0,
    SNET_IPCL_LOOKUP_0_1_E,
    SNET_IPCL_LOOKUP_1_E
}SNET_IPCL_LOOKUP_ENT;

/*******************************************************************************
*   snetProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   snetFrameProcess
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

/*******************************************************************************
*   snetCpuTxFrameProcess
*
* DESCRIPTION:
*       Process frame sent from CPU: call appropriate Tx function
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*       descrPtr       - pointer to the frame's descriptor.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetCpuTxFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPtr
);


/*******************************************************************************
*   snetLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
*       port            - port number.
*                linkState   - link state (0 - down, 1 - up)
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN GT_U32                           port,
    IN GT_U32                           linkState
);

/*******************************************************************************
*   snetFrameParsing
*
* DESCRIPTION:
*       Parsing the frame, get information from frame and fill descriptor
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*       descrPtr
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetFrameParsing
(
    IN SKERNEL_DEVICE_OBJECT        *     deviceObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC   *     descrPrt
);

/*******************************************************************************
*   snetTagDataGet
*
* DESCRIPTION:
*       Get VLAN tag info
*
* INPUTS:
*       vpt                 - user priority tag
*       vid                 - vlan ID
*       littleEndianOrder   - little endian order or big
* OUTPUT
*       tagData - tagged data
*
*******************************************************************************/
void snetTagDataGet
(
    IN  GT_U8   vpt,
    IN  GT_U16  vid,
    IN  GT_BOOL littleEndianOrder,
    OUT GT_U8   tagData[] /* 4 bytes */
);

/*******************************************************************************
*   snetProcessFrameFromUpLink
*
* DESCRIPTION:
*       Process frame from uplink
*
* INPUTS:
*    devObjPtr
*    descrPtr
* OUTPUT:
* COMMENT:
*         The frame is sent from pp->fa or fa->pp
*******************************************************************************/
void snetProcessFrameFromUpLink
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC     *     descrPtr
);

/*******************************************************************************
*   snetCpuMessageProcess
*
* DESCRIPTION:
*       xbar sends cpu message.
*
* INPUTS:
*    devObjPtr -
*    descrPtr  -
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
void snetCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN GT_U8                      *     descrPtr
);

/*******************************************************************************
*   snetFromCpuDmaProcess
*
* DESCRIPTION:
*     Process transmitted SDMA queue frames
*
* INPUTS:
*    devObjPtr -  pointer to device object
*    bufferId  -  buffer id
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
GT_VOID snetFromCpuDmaProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN SBUF_BUF_ID                      bufferId
);

/*******************************************************************************
*   snetFromEmbeddedCpuProcess
*
* DESCRIPTION:
*     Process transmitted frames from the Embedded CPU to the PP
*
* INPUTS:
*    devObjPtr -  pointer to device object
*    bufferId  -  buffer id
*
* OUTPUT:
*
* COMMENT:
*
*******************************************************************************/
GT_VOID snetFromEmbeddedCpuProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SBUF_BUF_ID                      bufferId
);


/*******************************************************************************
*   snetCncFastDumpUploadAction
*
* DESCRIPTION:
*       Process upload CNC block by CPU demand
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       cncTrigPtr  - pointer to CNC Fast Dump Trigger Register
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetCncFastDumpUploadAction
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                * cncTrigPtr
);

/*******************************************************************************
*   snetUtilGetContinuesValue
*
* DESCRIPTION:
*        the function get value of next continues fields .
* INPUTS:
*        devObjPtr - pointer to device object.
*        startAddress  - need to be register address %4 == 0.
*        sizeofValue - how many bits this continues field is
*        indexOfField - what is the index of the field
* OUTPUTS:
*   none
*
* RETURN : the value of fields
*
*COMMENTS:
*******************************************************************************/
extern GT_U32 snetUtilGetContinuesValue(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 startAddress ,
    IN GT_U32 sizeofValue,
    IN GT_U32 indexOfField
);

/*******************************************************************************
*   snetFieldValueGet
*
* DESCRIPTION:
*        get the value of field (up to 32 bits) that located in any start bit in
*       memory
* INPUTS:
*        startMemPtr - pointer to memory
*        startBit  - start bit of field (0..)
*        numBits   - number of bits of field (0..31)
* OUTPUTS:
*
* COMMENTS:
*
*******************************************************************************/
GT_U32  snetFieldValueGet(
    IN GT_U32                  *startMemPtr,
    IN GT_U32                  startBit,
    IN GT_U32                  numBits
);

/*******************************************************************************
*   snetFieldValueSet
*
* DESCRIPTION:
*        set the value to field (up to 32 bits) that located in any start bit in
*       memory
* INPUTS:
*        startMemPtr - pointer to memory
*        startBit  - start bit of field (0..)
*        numBits   - number of bits of field (0..31)
*        value     - value to write to
* OUTPUTS:
*
* COMMENTS:
*
*******************************************************************************/
void  snetFieldValueSet(
    IN GT_U32                  *startMemPtr,
    IN GT_U32                  startBit,
    IN GT_U32                  numBits,
    IN GT_U32                  value
);


/*******************************************************************************
*   ipV4CheckSumCalc
*
* DESCRIPTION:
*        Perform ones-complement sum , and ones-complement on the final sum-word.
*        The function can be used to make checksum for various protocols.
* INPUTS:
*        msgPtr - pointer to IP header.
*        msgSize  - IP header length.
* OUTPUTS:
*
* COMMENTS:
*        1. If there's a field CHECKSUM within the input-buffer
*           it supposed to be zero before calling this function.
*
*        2. The input buffer is supposed to be in network byte-order.
*******************************************************************************/
extern GT_U32 ipV4CheckSumCalc
(
    IN GT_U8 *pMsg,
    IN GT_U16 lMsg
);

/*******************************************************************************
* snetFieldFromEntry_GT_U32_Get
*
* DESCRIPTION:
*        Get GT_U32 value of a field from the table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       entryPtr        - pointer to memory.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       entryIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
*       fieldIndex      - the index of the field (used as index in fieldsInfoArr[] and in fieldsNamesArr[])
*
* OUTPUTS:
*       None.
*
* RETURN:
*       the value of the field
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 snetFieldFromEntry_GT_U32_Get(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN GT_U32                           *entryPtr,
    IN char *                           tableName,
    IN GT_U32                           entryIndex,
    IN SNET_ENTRY_FORMAT_TABLE_STC      fieldsInfoArr[],
    IN char *                           fieldsNamesArr[],
    IN GT_U32                           fieldIndex
);

/*******************************************************************************
* snetFieldFromEntry_subField_Get
*
* DESCRIPTION:
*        Get sub field (offset and num of bits) from a 'parent' field from the table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       entryPtr        - pointer to memory.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       entryIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
*       parentFieldIndex - the index of the 'parent' field (used as index in fieldsInfoArr[] and in fieldsNamesArr[])
*       subFieldOffset  - bit offset from start of the parent field.
*       subFieldNumOfBits - number of bits of the sub field.
*
* OUTPUTS:
*       None.
*
* RETURN:
*       the value of the field
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 snetFieldFromEntry_subField_Get(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN GT_U32                           *entryPtr,
    IN char *                           tableName,
    IN GT_U32                           entryIndex,
    IN SNET_ENTRY_FORMAT_TABLE_STC      fieldsInfoArr[],
    IN char *                           fieldsNamesArr[],
    IN GT_U32                           parentFieldIndex,
    IN GT_U32                           subFieldOffset,
    IN GT_U32                           subFieldNumOfBits
);

/*******************************************************************************
* snetFieldFromEntry_Any_Get
*
* DESCRIPTION:
*        Get (any length) value of a field from the table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       entryPtr        - pointer to memory.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       entryIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
*       fieldIndex      - the index of the field (used as index in fieldsInfoArr[] and in fieldsNamesArr[])
*
* OUTPUTS:
*       valueArr[]      - the array of GT_U32 that hold the value of the field.
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
void snetFieldFromEntry_Any_Get(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN GT_U32                           *entryPtr,
    IN char *                           tableName,
    IN GT_U32                           entryIndex,
    IN SNET_ENTRY_FORMAT_TABLE_STC      fieldsInfoArr[],
    IN char *                           fieldsNamesArr[],
    IN GT_U32                           fieldIndex,
    OUT GT_U32                          valueArr[]
);


/*******************************************************************************
* snetFieldFromEntry_GT_U32_Set
*
* DESCRIPTION:
*        Set GT_U32 value into a field in the table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       entryPtr        - pointer to memory.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       entryIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
*       fieldIndex      - the index of the field (used as index in fieldsInfoArr[] and in fieldsNamesArr[])
*       value           - the value to set to the field (the value is 'masked'
*                         according to the actual length of the field )
*
* OUTPUTS:
*       None.
*
* RETURN:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
void snetFieldFromEntry_GT_U32_Set(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN GT_U32                           *entryPtr,
    IN char *                           tableName,
    IN GT_U32                           entryIndex,
    IN SNET_ENTRY_FORMAT_TABLE_STC      fieldsInfoArr[],
    IN char *                           fieldsNamesArr[],
    IN GT_U32                           fieldIndex,
    IN GT_U32                           value
);

/*******************************************************************************
* snetFieldFromEntry_subField_Set
*
* DESCRIPTION:
*        Set value to sub field (offset and num of bits) from a 'parent' field from the table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       entryPtr        - pointer to memory.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       entryIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
*       parentFieldIndex - the index of the 'parent' field (used as index in fieldsInfoArr[] and in fieldsNamesArr[])
*       subFieldOffset  - bit offset from start of the parent field.
*       subFieldNumOfBits - number of bits of the sub field.
*       value           - the value to set to the sub field (the value is 'masked'
*                         according to subFieldNumOfBits )
*
* OUTPUTS:
*       None.
*
* RETURN:
*
* COMMENTS:
*
*******************************************************************************/
void snetFieldFromEntry_subField_Set(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN GT_U32                           *entryPtr,
    IN char *                           tableName,
    IN GT_U32                           entryIndex,
    IN SNET_ENTRY_FORMAT_TABLE_STC      fieldsInfoArr[],
    IN char *                           fieldsNamesArr[],
    IN GT_U32                           parentFieldIndex,
    IN GT_U32                           subFieldOffset,
    IN GT_U32                           subFieldNumOfBits,
    IN GT_U32                           value
);

/*******************************************************************************
* snetFieldFromEntry_Any_Set
*
* DESCRIPTION:
*        Set (any length) value of a field from the table entry.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       entryPtr        - pointer to memory.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       entryIndex      - index of the entry in the table            --> for dump to LOG purpose only.
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
*       fieldIndex      - the index of the field (used as index in fieldsInfoArr[] and in fieldsNamesArr[])
*       valueArr[]      - the array of GT_U32 that hold the value of the field.
*
* OUTPUTS:
*       None
*
* RETURN:
*       None
*
* COMMENTS:
*
*******************************************************************************/
void snetFieldFromEntry_Any_Set(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN GT_U32                           *entryPtr,
    IN char *                           tableName,
    IN GT_U32                           entryIndex,
    IN SNET_ENTRY_FORMAT_TABLE_STC      fieldsInfoArr[],
    IN char *                           fieldsNamesArr[],
    IN GT_U32                           fieldIndex,
    IN GT_U32                           valueArr[]
);



/*******************************************************************************
* snetFillFieldsStartBitInfo
*
* DESCRIPTION:
*        Fill during init the 'start bit' of the fields in the table format.
*
* INPUTS:
*       devObjPtr       - pointer to device object. --> can be NULL --> for dump to LOG purpose only.
*       tableName       - table name (string)       --> can be NULL  --> for dump to LOG purpose only.
*       numOfFields     - the number of elements in in fieldsInfoArr[] and in fieldsNamesArr[].
*       fieldsInfoArr   - array of fields info
*       fieldsNamesArr  - array of fields names     --> can be NULL  --> for dump to LOG purpose only.
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
void snetFillFieldsStartBitInfo(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN char *                           tableName,
    IN GT_U32                           numOfFields,
    INOUT SNET_ENTRY_FORMAT_TABLE_STC   fieldsInfoArr[],
    IN char *                           fieldsNamesArr[]
);

/*******************************************************************************
* snetPrintFieldsInfo
*
* DESCRIPTION:
*        print the info about the fields in the table format.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       tableName       - table name (string)
*                           NOTE:
*                           1. can be 'prefix of name' for multi tables !!!
*                           2. when NULL .. print ALL tables !!!
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
void snetPrintFieldsInfo(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN char *                           tableName
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sneth */
