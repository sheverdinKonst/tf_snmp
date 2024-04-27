/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snet.c
*
* DESCRIPTION:
*       This is a external API definition for snet module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 54 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetFap.h>
#include <asicSimulation/SKernel/suserframes/snetFe.h>
#include <asicSimulation/SKernel/suserframes/snetFaFox.h>
#include <asicSimulation/SKernel/suserframes/snetCapuera.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetSalsa.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/suserframes/snetTigerL2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahIngress.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/suserframes/snetPuma.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snetSoho.h>
#include <asicSimulation/SKernel/sEmbeddedCpu/sEmbeddedCpu.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/smem/smemPhy.h>
#include <asicSimulation/SKernel/smem/smemMacsec.h>
#include <asicSimulation/SLog/simLog.h>

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
)
{
    GT_BIT  initDone = 0;
    if (!deviceObjPtr)
    {
        skernelFatalError("snetProcessInit : illegal pointer\n");
    }

    /* check device type (including FA) and call device/FA specific init */
    switch(deviceObjPtr->deviceFamily)
    {
        case SKERNEL_CHEETAH_1_FAMILY:
        case SKERNEL_CHEETAH_2_FAMILY:
        case SKERNEL_CHEETAH_3_FAMILY:
        case SKERNEL_XCAT_FAMILY:
        case SKERNEL_XCAT3_FAMILY:
        case SKERNEL_XCAT2_FAMILY:
        case SKERNEL_LION_PORT_GROUP_FAMILY:
        case SKERNEL_LION2_PORT_GROUP_FAMILY:
        case SKERNEL_LION3_PORT_GROUP_FAMILY:
        case SKERNEL_PUMA_FAMILY:
        case SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY:
        case SKERNEL_BOBCAT2_FAMILY:
        case SKERNEL_BOBK_CAELUM_FAMILY:
        case SKERNEL_BOBK_CETUS_FAMILY:
        case SKERNEL_BOBCAT3_FAMILY:
            break;

        case SKERNEL_SALSA_FAMILY:
            snetSalsaProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_TIGER_FAMILY:
            snetTigerProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_TWIST_D_FAMILY:
        case SKERNEL_SAMBA_FAMILY:
        case SKERNEL_TWIST_C_FAMILY:
            snetTwistProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_SOHO_FAMILY:
            snetSohoProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_FA_FOX_FAMILY:
            snetFaFoxProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_LION_PORT_GROUP_SHARED_FAMILY:
        case SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY:
        case SKERNEL_LION3_PORT_GROUP_SHARED_FAMILY:
        case SKERNEL_COM_MODULE_FAMILY:
        case SKERNEL_PHY_SHELL_FAMILY:
        case SKERNEL_PUMA3_SHARED_FAMILY:
            /* no direct packet processing */
            initDone = 1;
            break;
        case SKERNEL_XBAR_CAPOEIRA_FAMILY:
            snetCapoeiraProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_FAP_DUNE_FAMILY:
            if (deviceObjPtr->deviceType != SKERNEL_98FX950)
            {
                snetFapProcessInit(deviceObjPtr);
            }
            initDone = 1;
            break;
        case SKERNEL_FE_DUNE_FAMILY:
            snetFeProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_EMBEDDED_CPU_FAMILY:
            snetEmbeddedCpuProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_PHY_CORE_FAMILY:
            smemPhyProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        case SKERNEL_MACSEC_FAMILY:
            smemMacsecProcessInit(deviceObjPtr);
            initDone = 1;
            break;
        default:
            skernelFatalError(" smemInit: not valid mode[%d]",
                                deviceObjPtr->deviceFamily);
        break;
    }

    if(initDone == 0)
    {
        /* the 'regular' devices split to 2 types :
            1. the non GM device,
            2. the GM devices
        */

        if(deviceObjPtr->gmDeviceType == GOLDEN_MODEL)
        {
            /* the device is 'GM device' */
            snetPumaProcessInit(deviceObjPtr);
        }
        else
        {
            /* the device is 'not GM device' */
            snetChtProcessInit(deviceObjPtr);
        }
    }


}

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
)
{
    SNET_DEV_FRAME_PROC_FUN  frameProcFunc;

    frameProcFunc = devObjPtr->devFrameProcFuncPtr;
    if (frameProcFunc)
    {
        frameProcFunc(devObjPtr, bufferId, srcPort);
    }
}

/*******************************************************************************
*   snetCncFastDumpUploadAction
*
* DESCRIPTION:
*       Process upload CNC block demanded by CPU
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
    IN GT_U32                 * cncTrigPtr
)
{
    ASSERT_PTR(deviceObjPtr);

    if (deviceObjPtr->devCncFastDumpFuncPtr == NULL)
        return ;

    deviceObjPtr->devCncFastDumpFuncPtr(deviceObjPtr, cncTrigPtr);
}

/*******************************************************************************
*   snetCpuTxFrameProcess
*
* DESCRIPTION:
*       Process frame sent from CPU: call apropriate Tx function
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
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPtr
)
{
    DECLARE_FUNC_NAME(snetCpuTxFrameProcess);


    /* check device type and call device specific tx frame from CPU func */
    __LOG(("check device type and call device specific tx frame from CPU func"));
    switch(devObjPtr->deviceFamily)
    {
        case SKERNEL_SAMBA_FAMILY:
        case SKERNEL_TWIST_D_FAMILY:
        case SKERNEL_TIGER_FAMILY:
        case SKERNEL_TWIST_C_FAMILY:
              snetTwistCpuTxFrameProcess(devObjPtr, descrPtr);
            break;
        default:
            skernelFatalError("snetCpuFrameProcess: \
                              devObjPtr->deviceFamily[%d] not supported",
                              devObjPtr->deviceFamily);
            break;
    }
}

/*******************************************************************************
*   snetProcessFrameFromUpLink
*
* DESCRIPTION:
*       Process frame sent from uplink: call apropriate Tx function
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
void snetProcessFrameFromUpLink
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC    *     descrPtr
)
{
    DECLARE_FUNC_NAME(snetProcessFrameFromUpLink);

    /* check device type and call device specific tx frame from CPU func */
    __LOG(("check device type and call device specific tx frame from CPU func"));
    switch(devObjPtr->deviceFamily)
    {
        case SKERNEL_TWIST_D_FAMILY:
        case SKERNEL_TIGER_FAMILY:
        case SKERNEL_TWIST_C_FAMILY:
        case SKERNEL_SAMBA_FAMILY:
            snetTwistProcessFrameFromUpLink(devObjPtr,descrPtr);
            break;
        case SKERNEL_FA_FOX_FAMILY:
            snetFaFoxProcessFrameFromUplink(devObjPtr,descrPtr);
            break;
        case SKERNEL_FAP_DUNE_FAMILY:
            snetFapProcessFrameFromUpLink(devObjPtr,descrPtr);
            break;
        default:
            skernelFatalError("snetProcessFrameFromUpLink: \
                              devObjPtr->deviceFamily[%d] not supported",
                              devObjPtr->deviceFamily);
            break;
    }
}

/*******************************************************************************
* snetCpuMessageProcess
*
* DESCRIPTION:
*       send message of mailbox or pcs ping or reachablity message from cpu
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*       dataPtr        - pointer to the message content .
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
void snetCpuMessageProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN GT_U8                      *     dataPtr
)
{
    /* check device type and call device specific tx frame from CPU func */
    switch(devObjPtr->deviceFamily)
    {
        case SKERNEL_FA_FOX_FAMILY:
            snetFoxCpuMessageProcess(devObjPtr, dataPtr);
            break;
        case SKERNEL_XBAR_CAPOEIRA_FAMILY:
            snetCapoeiraCpuMessageProcess(devObjPtr, dataPtr);
            break;
        case SKERNEL_FAP_DUNE_FAMILY:
            snetFapCpuMessageProcess(devObjPtr, dataPtr);
            break;
        default:
            skernelFatalError("snetCpuMessageProcess: \
                              devObjPtr->deviceFamily[%d] not supported",
                              devObjPtr->deviceFamily);
            break;
    }
}

/*******************************************************************************
*   snetLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       deviceObjPtr - pointer to device object.
*       port            - port number.
*       linkState   - link state (0 - down, 1 - up)
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
)
{
    ASSERT_PTR(deviceObjPtr);

    if (deviceObjPtr->devPortLinkUpdateFuncPtr == NULL)
        return ;

    /* register the notification in the LOGGER engine*/
    simLogLinkStateNotify(deviceObjPtr,port,linkState);

    deviceObjPtr->devPortLinkUpdateFuncPtr(deviceObjPtr, port, linkState);
}

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
    IN SKERNEL_DEVICE_OBJECT        *     devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC   *     descrPtr
)
{
    DECLARE_FUNC_NAME(snetFrameParsing);

    SBUF_BUF_STC   * frameBufPtr;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    frameBufPtr = descrPtr->frameBuf;
    /* Set byte count from actual buffer's length */
    __LOG(("Set byte count from actual buffer's length"));
    descrPtr->byteCount = (GT_U16)frameBufPtr->actualDataSize;
    /* Set destination MAC pointer */
    __LOG(("Set destination MAC pointer"));
    descrPtr->dstMacPtr = frameBufPtr->actualDataPtr;
    /* Fill MAC data type of descriptor */
    __LOG(("Fill MAC data type of descriptor"));
    if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr)) {
        descrPtr->macDaType = SKERNEL_BROADCAST_MAC_E;
    }
    else
    if (SGT_MAC_ADDR_IS_MCST(descrPtr->dstMacPtr)) {
        descrPtr->macDaType = SKERNEL_MULTICAST_MAC_E;
    }
    else
    {
        descrPtr->macDaType = SKERNEL_UNICAST_MAC_E;
    }

    /* the broadcast ARP will be checked when parsing the frame's protocol */
    __LOG(("the broadcast ARP will be checked when parsing the frame's protocol"));

    return;
}
#define     SNET_TAG_PROTOCOL_ID_CNS            0x8100
/*******************************************************************************
*   snetTagDataGet
*
* DESCRIPTION:
*       Get VLAN tag info
*
* INPUTS:
*       descrPtr - description pointer.
*
*******************************************************************************/
void snetTagDataGet
(
    IN  GT_U8   vpt,
    IN  GT_U16  vid,
    IN  GT_BOOL littleEndianOrder,
    OUT GT_U8   tagData[] /* 4 bytes */
)
{
    ASSERT_PTR(tagData);

    if (littleEndianOrder) {
        /* form ieee802.1q tag - little endian order */
        tagData[3] = SNET_TAG_PROTOCOL_ID_CNS >> 8;
        tagData[2] = SNET_TAG_PROTOCOL_ID_CNS & 0xff;

        /* lsb of tag control */
        tagData[1] = ((vid & 0xf00) >> 8) | ((vpt & 0x7) << 5);
        tagData[0] = vid & 0xff; /* msb of tag control */
    }
    else /* big endian */
    {
        /* form ieee802.1q tag - BIG endian order */
        tagData[0] = SNET_TAG_PROTOCOL_ID_CNS >> 8;
        tagData[1] = SNET_TAG_PROTOCOL_ID_CNS & 0xff;

        /* msb of tag control */
        tagData[2] = ((vid & 0xf00) >> 8) | ((vpt & 0x7) << 5);
        tagData[3] = vid & 0xff; /* lsb of tag control */
    }

    return;
}


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
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    SBUF_BUF_ID                         bufferId
)
{
    SNET_DEV_FROM_CPU_DMA_PROC_FUN  fromCpuDmaProcFuncPtr;

    fromCpuDmaProcFuncPtr = devObjPtr->devFromCpuDmaFuncPtr;
    if (fromCpuDmaProcFuncPtr)
    {
        fromCpuDmaProcFuncPtr(devObjPtr, bufferId);
    }
}

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
    SBUF_BUF_ID                         bufferId
)
{
    if (devObjPtr->devFromEmbeddedCpuFuncPtr)
    {
        devObjPtr->devFromEmbeddedCpuFuncPtr(devObjPtr, bufferId);
    }
}

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
)
{
    GT_U32  actualStartWord = startBit >> 5;/*/32*/
    GT_U32  actualStartBit  = startBit & 0x1f;/*%32*/
    GT_U32  actualValue;
    GT_U32  workValue;
    GT_U32  numBitsFirst;
    GT_U32  numBitsLeft;

    ASSERT_PTR(startMemPtr);

    if (numBits > 32)
    {
        skernelFatalError(" snetFieldValueGet: oversize numBits[%d] > 32 \n", numBits);
    }

    if ((actualStartBit + numBits) <= 32)
    {
        numBitsFirst = numBits;
        numBitsLeft  = 0;
    }
    else
    {
        numBitsFirst = 32 - actualStartBit;
        numBitsLeft  = numBits - numBitsFirst;
    }

    actualValue = SMEM_U32_GET_FIELD(
        startMemPtr[actualStartWord], actualStartBit, numBitsFirst);

    if (numBitsLeft > 0)
    {
        /* retrieve the rest of the value from the second word */
        workValue = SMEM_U32_GET_FIELD(
            startMemPtr[actualStartWord + 1], 0, numBitsLeft);

        /* place it to the high bits of the result */
        actualValue |= (workValue << numBitsFirst);
    }

    return actualValue;
}


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
)
{
    GT_U32  actualStartWord = startBit >> 5;/*/32*/
    GT_U32  actualStartBit  = startBit & 0x1f;/*%32*/
    GT_U32  numBitsFirst;
    GT_U32  numBitsLeft;

    ASSERT_PTR(startMemPtr);

    if (numBits > 32)
    {
        skernelFatalError(" snetFieldValueSet: oversize numBits[%d] > 32 \n", numBits);
    }

    if ((actualStartBit + numBits) <= 32)
    {
        numBitsFirst = numBits;
        numBitsLeft  = 0;
    }
    else
    {
        numBitsFirst = 32 - actualStartBit;
        numBitsLeft  = numBits - numBitsFirst;
    }

    SMEM_U32_SET_FIELD(
        startMemPtr[actualStartWord], actualStartBit, numBitsFirst,value);

    if (numBitsLeft > 0)
    {
        /* place rest of value to the high bits of the result */
        SMEM_U32_SET_FIELD(
            startMemPtr[actualStartWord + 1], 0, numBitsLeft,(value>>numBitsFirst));
    }

    return ;
}

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
)
{
    GT_U32  value;

    ASSERT_PTR(fieldsInfoArr);

    value = snetFieldValueGet(entryPtr,fieldsInfoArr[fieldIndex].startBit,fieldsInfoArr[fieldIndex].numOfBits);

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("table[%s] entryIndex[%d] fieldName[%s] value[0x%x]",
                  tableName ? tableName : "unknown",
                  entryIndex,
                  fieldsNamesArr ? fieldsNamesArr[fieldIndex] : "unknown",
                  value));


    return value;
}

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
)
{
    GT_U32  value;

    ASSERT_PTR(fieldsInfoArr);

    value = snetFieldValueGet(entryPtr,fieldsInfoArr[parentFieldIndex].startBit + subFieldOffset, subFieldNumOfBits);

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("table[%s] entryIndex[%d] fieldName[%s] subFieldOffset[%d] subFieldNumOfBits[%d] value[0x%x]",
                  tableName ? tableName : "unknown",
                  entryIndex,
                  fieldsNamesArr ? fieldsNamesArr[parentFieldIndex] : "unknown",
                  subFieldOffset,
                  subFieldNumOfBits,
                  value));


    return value;
}


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
)
{
    GT_U32  numOfWords;
    GT_U32  ii;
    GT_U32  subFieldOffset;
    GT_U32  subFieldNumOfBits;

    ASSERT_PTR(fieldsInfoArr);
    ASSERT_PTR(valueArr);

    numOfWords = CONVERT_BITS_TO_WORDS_MAC(fieldsInfoArr[fieldIndex].numOfBits);

    if(numOfWords <= 1)
    {
        valueArr[0] = snetFieldFromEntry_GT_U32_Get(devObjPtr,entryPtr,tableName,entryIndex,fieldsInfoArr,fieldsNamesArr,fieldIndex);
        return;
    }

    /* break the field into 'sub fields' of 32 bits
       to use snetFieldFromEntry_subField_Get */

    subFieldOffset = 0;
    subFieldNumOfBits = 32;
    for(ii = 0; ii < (numOfWords - 1); ii++,subFieldOffset += 32)
    {
        valueArr[ii] = snetFieldFromEntry_subField_Get(devObjPtr,entryPtr,tableName,
                entryIndex,fieldsInfoArr,fieldsNamesArr,
                fieldIndex,subFieldOffset,subFieldNumOfBits);
    }

    subFieldNumOfBits = fieldsInfoArr[fieldIndex].numOfBits & 0x1f;/* %32 */
    if(subFieldNumOfBits == 0)
    {
        /* the last word is 32 bits */
        subFieldNumOfBits = 32;
    }

    /* get the last word */
    valueArr[ii] = snetFieldFromEntry_subField_Get(devObjPtr,entryPtr,tableName,
            entryIndex,fieldsInfoArr,fieldsNamesArr,
            fieldIndex,subFieldOffset,subFieldNumOfBits);

    return;
}


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
)
{
    ASSERT_PTR(fieldsInfoArr);

    snetFieldValueSet(entryPtr,fieldsInfoArr[fieldIndex].startBit,fieldsInfoArr[fieldIndex].numOfBits,value);

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("table[%s] entryIndex[%d] fieldName[%s] value[0x%x]",
                  tableName ? tableName : "unknown",
                  entryIndex,
                  fieldsNamesArr ? fieldsNamesArr[fieldIndex] : "unknown",
                  value));


    return ;
}

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
)
{
    ASSERT_PTR(fieldsInfoArr);

    snetFieldValueSet(entryPtr,fieldsInfoArr[parentFieldIndex].startBit + subFieldOffset,subFieldNumOfBits,value);

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("table[%s] entryIndex[%d] fieldName[%s] subFieldOffset[%d] subFieldNumOfBits[%d] value[0x%x]",
                  tableName ? tableName : "unknown",
                  entryIndex,
                  fieldsNamesArr ? fieldsNamesArr[parentFieldIndex] : "unknown",
                  subFieldOffset,
                  subFieldNumOfBits,
                  value));


    return ;
}

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
)
{
    GT_U32  numOfWords;
    GT_U32  ii;
    GT_U32  subFieldOffset;
    GT_U32  subFieldNumOfBits;

    ASSERT_PTR(fieldsInfoArr);
    ASSERT_PTR(valueArr);

    numOfWords = CONVERT_BITS_TO_WORDS_MAC(fieldsInfoArr[fieldIndex].numOfBits);

    if(numOfWords <= 1)
    {
        snetFieldFromEntry_GT_U32_Set(devObjPtr,entryPtr,tableName,entryIndex,fieldsInfoArr,fieldsNamesArr,fieldIndex,valueArr[0]);
        return;
    }

    /* break the field into 'sub fields' of 32 bits
       to use snetFieldFromEntry_subField_Set */

    subFieldOffset = 0;
    subFieldNumOfBits = 32;
    for(ii = 0; ii < (numOfWords - 1); ii++,subFieldOffset += 32)
    {
        snetFieldFromEntry_subField_Set(devObjPtr,entryPtr,tableName,
                entryIndex,fieldsInfoArr,fieldsNamesArr,
                fieldIndex,subFieldOffset,subFieldNumOfBits,
                valueArr[ii]);
    }

    subFieldNumOfBits = fieldsInfoArr[fieldIndex].numOfBits & 0x1f;/* %32 */
    if(subFieldNumOfBits == 0)
    {
        /* the last word is 32 bits */
        subFieldNumOfBits = 32;
    }

    /* set the last word */
    snetFieldFromEntry_subField_Set(devObjPtr,entryPtr,tableName,
            entryIndex,fieldsInfoArr,fieldsNamesArr,
            fieldIndex,subFieldOffset,subFieldNumOfBits,
            valueArr[ii]);

    return;
}

typedef struct{
    char *                           tableName;      /*table name (string)       --> can be NULL  --> for dump to LOG purpose only.*/
    GT_U32                           numOfFields;    /*the number of elements in in fieldsInfoArr[] and in fieldsNamesArr[].       */
    SNET_ENTRY_FORMAT_TABLE_STC     *fieldsInfoPtr;  /*array of fields info                                                        */
    char *                          *fieldsNamesPtr; /*array of fields names     --> can be NULL  --> for dump to LOG purpose only.*/
    GT_U32                           numBitsUsed;    /* number of bits in the entry */
}TABLE_AND_FIELDS_INFO_STC;

#define MAX_TABLES_CNS   512
static TABLE_AND_FIELDS_INFO_STC  tablesInfoArr[MAX_TABLES_CNS];
static GT_U32   lastTableIndex = 0;

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
)
{
    GT_U32                      ii;
    SNET_ENTRY_FORMAT_TABLE_STC *currentFieldInfoPtr;
    SNET_ENTRY_FORMAT_TABLE_STC *prevFieldInfoPtr;
    GT_U32                      prevIndex;

    ASSERT_PTR(fieldsInfoArr);

    /* look for this table in the DB (by it's name) */
    for( ii = 0 ; ii < lastTableIndex ; ii++)
    {
        if(0 == strcmp(tablesInfoArr[ii].tableName,tableName))
        {
            /* table already registered */
            /* table names should be unique (currently not supported 'per device') */
            return;
        }
    }

    if(lastTableIndex == (MAX_TABLES_CNS - 1))
    {
        skernelFatalError("snetFillFieldsStartBitInfo : reached max number of tables ... need to enlarge MAX_TABLES_CNS \n");
        return;
    }


    simLogMessage(SIM_LOG_FUNC_NAME_MAC(snetFillFieldsStartBitInfo), devObjPtr, SIM_LOG_INFO_TYPE_DEVICE_E,
                  "table[%s] , numOfFields[%d]",
                  tableName ? tableName : "unknown",
                  numOfFields);


    for( ii = 0 ; ii < numOfFields ; ii++)
    {
        currentFieldInfoPtr = &fieldsInfoArr[ii];

        prevIndex = currentFieldInfoPtr->previousFieldType;

        if(currentFieldInfoPtr->startBit == FIELD_SET_IN_RUNTIME_CNS)
        {
            if(ii == 0)
            {
                /* first field got no options other then to start in bit 0 */
                currentFieldInfoPtr->startBit = 0;
            }
            else /* use the previous field info */
            {
                if(prevIndex == FIELD_CONSECUTIVE_CNS)
                {
                    /* this field is consecutive to the previous field */
                    prevIndex = ii-1;
                }
                else
                {
                    /* this field come after other previous field */
                }

                prevFieldInfoPtr = &fieldsInfoArr[prevIndex];
                currentFieldInfoPtr->startBit = prevFieldInfoPtr->startBit + prevFieldInfoPtr->numOfBits;
            }
        }
        else
        {
            /* no need to calculate the start bit -- it is FORCED by the entry format */
        }

        simLogMessage(SIM_LOG_FUNC_NAME_MAC(snetFillFieldsStartBitInfo), devObjPtr, SIM_LOG_INFO_TYPE_DEVICE_E,
                      "index[%d] fieldName[%s] startBit[%d] numOfBits[%d]",
                      ii,
                      fieldsNamesArr ? fieldsNamesArr[ii] : "unknown",
                      currentFieldInfoPtr->startBit,
                      currentFieldInfoPtr->numOfBits
                      );

        if(prevIndex != FIELD_CONSECUTIVE_CNS)
        {
            prevFieldInfoPtr = &fieldsInfoArr[prevIndex];

            simLogMessage(SIM_LOG_FUNC_NAME_MAC(snetFillFieldsStartBitInfo), devObjPtr, SIM_LOG_INFO_TYPE_DEVICE_E,
                          "     previous_index[%d] previous_fieldName[%s] previous_startBit[%d] previous_numOfBits[%d]",
                          prevIndex,
                          fieldsNamesArr ? fieldsNamesArr[prevIndex] : "unknown",
                          prevFieldInfoPtr->startBit,
                          prevFieldInfoPtr->numOfBits
                          );

        }
    }

    /* save the table into DB */
    tablesInfoArr[lastTableIndex].tableName      = tableName;
    tablesInfoArr[lastTableIndex].numOfFields    = numOfFields;
    tablesInfoArr[lastTableIndex].fieldsInfoPtr  = fieldsInfoArr;
    tablesInfoArr[lastTableIndex].fieldsNamesPtr = fieldsNamesArr;

    tablesInfoArr[lastTableIndex].numBitsUsed = 0;
    for( ii = 0 ; ii < numOfFields ; ii++)
    {
        currentFieldInfoPtr = &fieldsInfoArr[ii];

        if(tablesInfoArr[lastTableIndex].numBitsUsed <
            (currentFieldInfoPtr->startBit + currentFieldInfoPtr->numOfBits))
        {
            tablesInfoArr[lastTableIndex].numBitsUsed =
                (currentFieldInfoPtr->startBit + currentFieldInfoPtr->numOfBits);
        }
    }

    lastTableIndex ++;
}

static GT_U32 numOfCharsInPrefixNameGet(
    IN TABLE_AND_FIELDS_INFO_STC   *currentTableInfoPtr/* current table info */
)
{
    GT_U32  ii,jj;
    static char tempFieldsPrefix[256];
    GT_U32 numOfCharsInPrefixName = (GT_U32)strlen(currentTableInfoPtr->fieldsNamesPtr[0]);

    for( ii = 1 ; ii < currentTableInfoPtr->numOfFields ; ii++)
    {
        for(jj = numOfCharsInPrefixName ; jj > 5 ; jj--)
        {
            if(0 == strncmp(currentTableInfoPtr->fieldsNamesPtr[0] ,
                            currentTableInfoPtr->fieldsNamesPtr[ii] ,
                            jj))
            {
                /* fount new shorter prefix */
                break;
            }
        }

        if(jj == 5)
        {
            /* no need to bother and look for prefix ... it is not exists */
            numOfCharsInPrefixName = 0;
            break;
        }
        else
        {
            numOfCharsInPrefixName = jj;
        }
    }

    if(numOfCharsInPrefixName >= 256)
    {
        numOfCharsInPrefixName = 255;
    }

    strncpy(tempFieldsPrefix,
           currentTableInfoPtr->fieldsNamesPtr[0],
           numOfCharsInPrefixName);

    tempFieldsPrefix[numOfCharsInPrefixName] = 0;/* string terminator*/

    simGeneralPrintf(" removing [%s] prefix from fields names \n",
        tempFieldsPrefix);

    return numOfCharsInPrefixName;
}



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
)
{
    GT_U32      ii;
    TABLE_AND_FIELDS_INFO_STC   *currentTableInfoPtr;/* current table info */
    SNET_ENTRY_FORMAT_TABLE_STC *currentFieldInfoPtr;/* current field info */
    char                        *currentFieldNamePtr;/* current field name */
    GT_U32      lenOfTableName;
    GT_U32      currentTableIndex;
    GT_U32      numTablesPrinted = 0;
    GT_U32      numOfCharsInPrefixName;/* the names of the fields are most likely to have 'same prefix' .
                                        calculate the 'prefix length' */

    devObjPtr = devObjPtr;/*currently not used*/

    if(tableName == NULL)
    {
        lenOfTableName = 0;
        simGeneralPrintf("snetPrintFieldsInfo : print ALL tables \n");
    }
    else
    {
        simGeneralPrintf("snetPrintFieldsInfo : print table(s) [%s] \n", tableName);
        lenOfTableName = (GT_U32)strlen(tableName);
    }

    currentTableIndex = 0 ;

    /* this while allow */
    while(currentTableIndex < lastTableIndex)
    {
        /* look for this table in the DB (by it's name) */
        for( /*continue*/ ; currentTableIndex < lastTableIndex ; currentTableIndex++)
        {
            if(tableName == NULL)
            {
                /* print all tables */
                break;
            }

            if(0 == strncmp(tablesInfoArr[currentTableIndex].tableName,tableName,lenOfTableName))
            {
                /* table found */
                /* table names should be unique (currently not supported 'per device') */
                break;
            }
        }

        if(currentTableIndex == lastTableIndex)
        {
            break;
        }

        currentTableInfoPtr = &tablesInfoArr[currentTableIndex];
        /* update the 'current index' for the next use */
        currentTableIndex++;

        if(numTablesPrinted)
        {
            simGeneralPrintf("--- end table --- [%d] \n", numTablesPrinted);
            simGeneralPrintf("\n\n\n\n");
            simGeneralPrintf("printing table [%d] \n", (numTablesPrinted + 1));
        }

        simGeneralPrintf("table[%s] with numBitsUsed[%d] \n",
                      currentTableInfoPtr->tableName,
                      currentTableInfoPtr->numBitsUsed);

        numOfCharsInPrefixName = numOfCharsInPrefixNameGet(currentTableInfoPtr);

        simGeneralPrintf("index  startBit  numOfBits  fieldName \n");

        currentFieldInfoPtr = &currentTableInfoPtr->fieldsInfoPtr[0];
        for( ii = 0 ; ii < currentTableInfoPtr->numOfFields ; ii++,
            currentFieldInfoPtr++)
        {
            currentFieldNamePtr = currentTableInfoPtr->fieldsNamesPtr[ii];

            simGeneralPrintf("%d \t %d \t %d \t %s \n",
                          ii,
                          currentFieldInfoPtr->startBit,
                          currentFieldInfoPtr->numOfBits,
                          &currentFieldNamePtr[numOfCharsInPrefixName]
                          );
        }

        simGeneralPrintf(" --- end fields ---\n");

        numTablesPrinted++;
    }

    if(numTablesPrinted == 0)
    {
        simGeneralPrintf("table[%s] not found \n",
            tableName ? tableName : "NULL");
    }
    else if(numTablesPrinted == 1)
    {
        simGeneralPrintf(" --- end table (single table) ---\n");
    }
    else
    {
        simGeneralPrintf("--- end table --- [%d] \n", numTablesPrinted);
        simGeneralPrintf("\n\n\n\n");
        simGeneralPrintf("printed [%d] tables \n", numTablesPrinted);
    }

    return;
}

/*******************************************************************************
*   skernelDebugFreeBuffersNumPrint
*
* DESCRIPTION:
*       debug function to print the table(s) fields format.
*
* INPUTS:
*       devNum - device number as stated in the INI file.
*       tableName       - table name (string)
*                           NOTE:
*                           1. can be 'prefix of name' for multi tables !!!
*                           2. when NULL .. print ALL tables !!!
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void snetPrintFieldsInfo_debug(
    IN GT_U32                           devNum,
    IN char *                           tableName
)
{
    SKERNEL_DEVICE_OBJECT* deviceObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SKERNEL_DEVICE_OBJECT* currDeviceObjPtr;
    GT_U32  dev;

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        simGeneralPrintf(" multi-core device [%d] \n",devNum);
        for(dev = 0 ; dev < deviceObjPtr->numOfCoreDevs ; dev++)
        {
            currDeviceObjPtr = deviceObjPtr->coreDevInfoPtr[dev].devObjPtr;
            snetPrintFieldsInfo(currDeviceObjPtr,tableName);
        }
    }
    else
    {
        snetPrintFieldsInfo(deviceObjPtr,tableName);
    }
}

static void printFullMemEntry(
    IN SKERNEL_DEVICE_OBJECT            *devObjPtr,
    IN TABLE_AND_FIELDS_INFO_STC        *currentTableInfoPtr,
    IN GT_U32                           *entryPtr
)
{
    GT_U32  ii,jj;
    static GT_U32  valueArr[128];
    GT_U32  numOfWords;
    char*   valueFormats[] = {
         "[%d]\t\t"                                  /*0 - for up to 8  bits field */
        ,"[0x%4.4x]\t"                             /*1 - for up to 16 bits field */
        ,"[0x%6.6x]\t"                             /*2 - for up to 24 bits field */
        ,"[0x%8.8x]\t"                               /*3 - for up to 32 bits field */
        ,"[%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x] \t " /*4 - for MAC_ADDR field */
        ,"[%s]\t\t"                                  /*5 - for '1 bit' field */
        };
    char*   currentValueFormatPtr;
    GT_U32      numOfCharsInPrefixName;/* the names of the fields are most likely to have 'same prefix' .
                                        calculate the 'prefix length' */

    numOfCharsInPrefixName = numOfCharsInPrefixNameGet(currentTableInfoPtr);

    for(ii = 0 ; ii < currentTableInfoPtr->numOfFields; ii++)
    {
        snetFieldFromEntry_Any_Get(devObjPtr,entryPtr,
            currentTableInfoPtr->tableName,
            0 , /*entryIndex -- unknown*/
            currentTableInfoPtr->fieldsInfoPtr,
            currentTableInfoPtr->fieldsNamesPtr,
            ii,/*fieldIndex*/
            valueArr);

        numOfWords = (currentTableInfoPtr->fieldsInfoPtr[ii].numOfBits + 31) / 32;

        if(numOfWords == 1)
        {
            if(currentTableInfoPtr->fieldsInfoPtr[ii].numOfBits == 1)
            {
                currentValueFormatPtr = /* ON/OFF field */ valueFormats[5];

                /*print the field value according to it's format */
                simGeneralPrintf(currentValueFormatPtr ,
                    valueArr[0] ? "ON" : "OFF");
            }
            else
            {
                if(currentTableInfoPtr->fieldsInfoPtr[ii].numOfBits <= 8)
                {
                    currentValueFormatPtr = /* decimal value */ valueFormats[0];
                }
                else
                if(currentTableInfoPtr->fieldsInfoPtr[ii].numOfBits <= 16)
                {
                    currentValueFormatPtr = /* hex value */ valueFormats[1];
                }
                else
                if(currentTableInfoPtr->fieldsInfoPtr[ii].numOfBits <= 24)
                {
                    currentValueFormatPtr = /* hex value */ valueFormats[2];
                }
                else/* up to 32*/
                {
                    currentValueFormatPtr = /* hex value */ valueFormats[3];
                }

                /*print the field value according to it's format */
                simGeneralPrintf(currentValueFormatPtr ,
                    valueArr[0]);
            }
        }
        else
        if(currentTableInfoPtr->fieldsInfoPtr[ii].numOfBits == 48)
        {
            currentValueFormatPtr = /* mac address */ valueFormats[4];
            /* mac address */
            simGeneralPrintf(currentValueFormatPtr
                ,(GT_U8)(valueArr[1] >>  8)
                ,(GT_U8)(valueArr[1] >>  0)
                ,(GT_U8)(valueArr[0] >> 24)
                ,(GT_U8)(valueArr[0] >> 16)
                ,(GT_U8)(valueArr[0] >>  8)
                ,(GT_U8)(valueArr[0] >>  0)
                );
        }
        else
        {
            currentValueFormatPtr = /* hex value */ valueFormats[3];

            for(jj = 0 ; jj < numOfWords ; jj++)
            {
                /*print the field value according to it's format */
                simGeneralPrintf(currentValueFormatPtr ,
                    valueArr[jj]);
            }
        }
        simGeneralPrintf("[%s] \n",
            &((currentTableInfoPtr->fieldsNamesPtr[ii])[numOfCharsInPrefixName]));
    }

    simGeneralPrintf("\n\n\n\n");

}

/*******************************************************************************
*   snetPrintFieldsInfoForSpecificMemoryEntry_debug
*
* DESCRIPTION:
*       debug function to print the content of specific entry in memory according
*       to known format.
*
* INPUTS:
*       devNum - device number as stated in the INI file.
*       tableName       - table name (string)
*                           NOTE:
*                           1. can be 'prefix of name' for multi tables !!!
*                           2. when NULL .. print ALL tables !!!
*       startAddress - start address of the entry in the memory
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void snetPrintFieldsInfoForSpecificMemoryEntry_debug(
    IN GT_U32                           devNum,
    IN char *                           tableName,
    IN GT_U32                           startAddress
)
{
    SKERNEL_DEVICE_OBJECT* deviceObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SKERNEL_DEVICE_OBJECT* currDeviceObjPtr;
    GT_U32  dev;
    GT_U32 ii;
    GT_U32  *entryPtr;

    if(tableName == NULL)
    {
        simGeneralPrintf(" table name 'NULL' \n");
        return ;
    }

    /* look for this table in the DB (by it's name) */
    for( ii = 0 ; ii < lastTableIndex ; ii++)
    {
        if(0 == strcmp(tablesInfoArr[ii].tableName,tableName))
        {
            /* table already registered */
            /* table names should be unique (currently not supported 'per device') */
            break;
        }
    }

    if(ii == lastTableIndex)
    {
        simGeneralPrintf(" table name [%s] not found  \n",devNum);
        return ;
    }

    simGeneralPrintf("print from table [%s] entry starts at address [0x%8.8x] with numBitsUsed[%d] \n",
        tablesInfoArr[ii].tableName,
        startAddress,
        tablesInfoArr[ii].numBitsUsed);

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        simGeneralPrintf(" multi-core device [%d] \n",devNum);
        for(dev = 0 ; dev < deviceObjPtr->numOfCoreDevs ; dev++)
        {
            currDeviceObjPtr = deviceObjPtr->coreDevInfoPtr[dev].devObjPtr;

            if(GT_FALSE ==
                smemIsDeviceMemoryOwner(currDeviceObjPtr,startAddress))
            {
                simGeneralPrintf("skip core[%d] , because not owner of this memory \n" , dev);
                /* the device is not the owner of this memory */
                continue;
            }

            simGeneralPrintf("Print the format for core[%d] \n" , dev);

            entryPtr = smemMemGet(currDeviceObjPtr , startAddress);

            printFullMemEntry(currDeviceObjPtr,&tablesInfoArr[ii],entryPtr);
        }
    }
    else
    {
        entryPtr = smemMemGet(deviceObjPtr , startAddress);
        printFullMemEntry(deviceObjPtr,&tablesInfoArr[ii],entryPtr);
    }
}

