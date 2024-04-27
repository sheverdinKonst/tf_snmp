/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion2TrafficGenerator.c
*
* DESCRIPTION:
*       This is a external API definition for Lion2 traffic generator module.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetLion2TrafficGenerator.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/smem/smemLion2.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>
#include <asicSimulation/SLog/simLogInfoTypeDevice.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEq.h>

#ifdef _WIN32
extern void SHOSTG_psos_reg_asic_task(void);
#endif /*_WIN32*/

/* Tx SDMA info required for processing by the generic function */
typedef struct{
    GT_U32      txQueue;  /* the TX sdma queue to use */
    SBUF_BUF_ID bufferId; /* Buffer of the message , will be reused for SDMA packet */
}TX_SDMA_GEN_PROCESS_STC;

/* the minimal number of buffers that should be kept , and not allowed for 'SDMA generator'*/
#define SDMA_GEN_MIN_FREE_BUFFERS_CNS   300


/*******************************************************************************
*   snetLion2TgPacketBuild
*
* DESCRIPTION:
*       Build TG packet.
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       trafficGenNumber    - traffic generator number
*       frameDataPtr        - pointer to frame data pointer
*       frameDataSizePtr    - pointer to frame data size
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2TgPacketBuild
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 trafficGenNumber,
    OUT GT_U8  *frameDataPtr,
    OUT GT_U32 *frameDataSizePtr
)
{
    GT_U8 macDa[6], macSa[6];       /* DA and SA MAC's*/
    GT_U8 vlanEtherType[2] = {0x81, 0x00};/* VLAN ethertype */
    GT_U8 vlanTag[2];               /* VLAN tag */
    GT_U8 etherType[2];             /* Packet's ethertype */
    GT_U8 dataPattern[8];           /* Packet's payload */
    GT_U32 regAddr;                 /* Register address */
    GT_U32 * regPtr;                /* Register pointer */
    GT_U32 fieldVal;                /* Register value */
    GT_U32 byte = 0;                /* Byte index */
    GT_U32 packetLength;            /* Packet length in bytes */

    regAddr = SMEM_LION2_TG_MAC_DA_0_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get DA MAC */
    for(byte = 0; byte < 6; byte++)
    {
        macDa[5 - byte] = SMEM_U32_GET_FIELD(regPtr[byte/2], 8 * (byte%2), 8);
    }
    MEM_APPEND(frameDataPtr, macDa, byte);

    regAddr = SMEM_LION2_TG_MAC_SA_0_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get SA MAC */
    for(byte = 0; byte < 6; byte++)
    {
        macSa[5 - byte] = SMEM_U32_GET_FIELD(regPtr[byte/2], 8 * (byte%2), 8);
    }
    MEM_APPEND(frameDataPtr, macSa, byte);

    regAddr = SMEM_LION2_TG_VLAN_TAG_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* VLAN Tag exist */
    if(regPtr[0])
    {
        /* VLAN Ethertype */
        MEM_APPEND(frameDataPtr, vlanEtherType, 2);

        /* Get VLAN Tag */
        for(byte = 0; byte < 2; byte++)
        {
            vlanTag[1 - byte] = SMEM_U32_GET_FIELD(regPtr[0], 8 * (byte%2), 8);
        }
        MEM_APPEND(frameDataPtr, vlanTag, byte);
    }

    regAddr = SMEM_LION2_TG_ETHERTYPE_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get ethertype  */
    for(byte = 0; byte < 2; byte++)
    {
        etherType[1 - byte] = SMEM_U32_GET_FIELD(regPtr[0], 8 * (byte%2), 8);
    }
    MEM_APPEND(frameDataPtr, etherType, byte);

    regAddr = SMEM_LION2_TG_DATA_PATTERN_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Set pointer to data pattern  */
    devObjPtr->trafficGeneratorSupport.
        trafficGenData[trafficGenNumber].dataPatternPtr = frameDataPtr;

    /* Get Data Pattern */
    for(byte = 0; byte < 8; byte++)
    {
        dataPattern[7 - byte] = SMEM_U32_GET_FIELD(regPtr[byte/2], 8 * (byte%2), 8);
    }
    MEM_APPEND(frameDataPtr, dataPattern, byte);

    regAddr = SMEM_LION2_TG_PACKET_LENGTH_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get packet length */
    packetLength = SMEM_U32_GET_FIELD(regPtr[0], 0, 14);
    if(packetLength < 20)
    {
        packetLength = 20;
    }

    regAddr = SMEM_LION2_TG_CONTROL_0_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Data type */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 3, 1);
    if(fieldVal)
    {
        /* Random */
        frameDataPtr = devObjPtr->trafficGeneratorSupport.
            trafficGenData[trafficGenNumber].dataPatternPtr;

        for(byte = 0; byte < 8; byte++)
        {
            frameDataPtr[byte] = rand();
        }
    }

    /* Get length type */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 2, 1);
    if(fieldVal)
    {
        /* Random */
        GT_U32 tempPacketLength = rand();
        packetLength = (tempPacketLength < 20) ? 20 :
            (((tempPacketLength % packetLength) < 20) ? packetLength : (tempPacketLength % packetLength));

        /* Get undersize */
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 9, 1);
        if(fieldVal == 0)
        {
            /* Disable */
            packetLength = (packetLength <= 60) ? 60 : packetLength;
        }
    }
    else
    {
        /* Constant */
    }

    /* Add 4 FCS (frame check sum or CRC) bytes to frame from packet generator */
    *frameDataSizePtr = packetLength + 4;
}

/*******************************************************************************
*   snetLion2TgPacketModify
*
* DESCRIPTION:
*       Modify packet according to TG mode.
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       trafficGenNumber    - traffic generator number
*       frameDataPtr        - pointer to frame data pointer
*       frameDataSizePtr    - pointer to frame data size
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2TgPacketModify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 trafficGenNumber,
    OUT GT_U8  *frameDataPtr,
    OUT GT_U32 *frameDataSizePtr
)
{
    GT_U32 regAddr;
    GT_U32 * regPtr;
    GT_U32 fieldVal;
    GT_U32 byte;

    regAddr = SMEM_LION2_TG_CONTROL_0_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 7, 1);
    if(fieldVal)
    {
        /* Increment MAC DA */
        byte = 5;

        frameDataPtr[byte]++;
        while(1)
        {
            if ((frameDataPtr[byte] != 0) || (byte == 0))
            {
                break;
            }

            byte--;
            frameDataPtr[byte]++;
        }
    }

    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 3, 1);
    if(fieldVal)
    {
        /* Data type - Random*/
        frameDataPtr = devObjPtr->trafficGeneratorSupport.
            trafficGenData[trafficGenNumber].dataPatternPtr;

        for(byte = 0; byte < 8; byte++)
        {
            frameDataPtr[byte] = rand();
        }
    }

}

/*******************************************************************************
*   snetLion2TgTx2Port
*
* DESCRIPTION:
*       Send traffic to Tx port.
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       port                - number of egress port
*       frameDataPtr        - pointer to frame data pointer
*       frameDataSize       - frame data size
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2TgTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 port,
    IN GT_U8  *frameDataPtr,
    IN GT_U32 frameDataSize
)
{
    SKERNEL_FRAME_CHEETAH_DESCR_STC descriptor;     /* packet descriptor */
    SKERNEL_DEVICE_OBJECT           ingressDevObj;  /* device object */

    memset(&descriptor, 0, sizeof(SKERNEL_FRAME_CHEETAH_DESCR_STC));
    memset(&ingressDevObj, 0, sizeof(SKERNEL_DEVICE_OBJECT));

    /* Set calculate packet SRC */
    descriptor.calcCrc = GT_TRUE;
    /* Set ingress port group ID */
    descriptor.ingressDevObjPtr = &ingressDevObj;
    descriptor.ingressDevObjPtr->portGroupId = devObjPtr->portGroupId;
    /* Set packet MAC DA type */
    if (SGT_MAC_ADDR_IS_BCST(frameDataPtr)) {
        descriptor.macDaType = SKERNEL_BROADCAST_MAC_E;
    }
    else
    if (SGT_MAC_ADDR_IS_MCST(frameDataPtr)) {
        descriptor.macDaType = SKERNEL_MULTICAST_MAC_E;
    }
    else
    {
        descriptor.macDaType = SKERNEL_UNICAST_MAC_E;
    }
    /* Set egress buffer size */
    descriptor.egressByteCount = frameDataSize;

    snetChtTx2Port(devObjPtr, &descriptor, port, frameDataPtr, frameDataSize);
}
/*******************************************************************************
*   snetLion2TgContinuousTrafficSend
*
* DESCRIPTION:
*       Continuous packet transmission routine.
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       trafficGenNumber    - traffic generator number
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2TgContinuousTrafficSend
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 trafficGenNumber
)
{
    GT_U32 regAddr;
    GT_U32 * regPtr;
    GT_U32 fieldVal;
    GT_U32 port;                        /* Transmittion port */
    GT_U8  *frameDataPtr;               /* Pointer to frame data */
    GT_U32 frameDataSize = 0;           /* Frame data size */
    GT_U32 interFrameGap = 0;           /* Idle time in milliseconds between packets */

    /* Pointer to frame buffer */
    frameDataPtr = devObjPtr->egressBuffer1;

    /* Build traffic from registers data */
    snetLion2TgPacketBuild(devObjPtr, trafficGenNumber,
                           frameDataPtr, &frameDataSize);

    regAddr = SMEM_LION2_TG_FRAME_GAP_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get idle time in bytes between packets */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 0, 16);
    /* Convert interframe gap to miliseconds */
    interFrameGap = (fieldVal / SGT_MAX_BYTES_PER_SECOND_CNS) * 1000;

    regAddr = SMEM_LION2_TG_CONTROL_0_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get connected port */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 10, 3);
    /* Tx port for port group = TG number * 4(ports in TG) + TG connected port number */
    port = fieldVal + (4 * trafficGenNumber);
    while(1)
    {
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 5, 1);
        if(fieldVal)
        {
            /* Clear stop trigger bit */
            SMEM_U32_SET_FIELD(regPtr[0], 5, 1, 0);
            break;
        }
        /* Send traffic to Tx port */
        snetLion2TgTx2Port(devObjPtr, port, frameDataPtr, frameDataSize);
        /* Modify packet if need */
        snetLion2TgPacketModify(devObjPtr, trafficGenNumber, frameDataPtr, &frameDataSize);

        devObjPtr = skernelSleep(devObjPtr,interFrameGap);
    }
}

/*******************************************************************************
*   snetLion2TgBurstSend
*
* DESCRIPTION:
*       Burst packet transmission routine.
*
* INPUTS:
*       devObjPtr           - pointer to device object
*       trafficGenNumber    - traffic generator number
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion2TgBurstSend
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 trafficGenNumber
)
{
    GT_U32 regAddr;                     /* Register address */
    GT_U32 * regPtr;                    /* Register pointer */
    GT_U32 fieldVal;                    /* Register's field value */
    GT_U32 port;                        /* Transmittion port */
    GT_U32 burstCount;                  /* Number of burst packets to send */
    GT_U32 packet;                      /* Packet counter */
    GT_U32 multiplier;                  /* Multiplier value */
    GT_U32 interFrameGap = 0;           /* Idle time in milliseconds between packets */
    GT_U8  *frameDataPtr;               /* Pointer to frame data */
    GT_U32 frameDataSize = 0;           /* Frame data size */

    /* Get pointer to frame buffer */
    frameDataPtr = devObjPtr->egressBuffer1;

    /* Build traffic from registers data */
    snetLion2TgPacketBuild(devObjPtr, trafficGenNumber,
                           frameDataPtr, &frameDataSize);

    regAddr = SMEM_LION2_TG_PACKET_COUNT_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get packet count to send in single burst mode */
    burstCount = SMEM_U32_GET_FIELD(regPtr[0], 0, 13);

    /* Get packet count multiplier */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 13, 3);
    switch(fieldVal)
    {
        case 1:
            multiplier = 0x100;
            break;
        case 2:
            multiplier = 0x200;
            break;
        case 3:
            multiplier = 0x400;
            break;
        case 4:
            multiplier = 0x1000;
            break;
        case 5:
            multiplier = 0x10000;
            break;
        case 6:
            multiplier = 0x100000;
            break;
        case 7:
            multiplier = 0x1000000;
            break;
        default:
            multiplier = 1;
    }

    /* Calculate burst count */
    burstCount *= multiplier;


    regAddr = SMEM_LION2_TG_FRAME_GAP_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get idle time in bytes between packets */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 0, 16);
    /* Convert interframe gap to miliseconds */
    interFrameGap = (fieldVal / SGT_MAX_BYTES_PER_SECOND_CNS) * 1000;

    regAddr = SMEM_LION2_TG_CONTROL_0_REG(devObjPtr, trafficGenNumber);
    regPtr = smemMemGet(devObjPtr, regAddr);

    /* Get connected port */
    fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 10, 3);

    /* Tx port for port group = TG number * 4(ports in TG) + TG connected port number */
    port = fieldVal + (4 * trafficGenNumber);

    for(packet = 0; packet < burstCount; packet++)
    {
        /* Stop trigger */
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 5, 1);
        if(fieldVal)
        {
            /* Clear stop trigger bit */
            SMEM_U32_SET_FIELD(regPtr[0], 5, 1, 0);
            break;
        }

        /* Send traffic to Tx port */
        snetLion2TgTx2Port(devObjPtr, port, frameDataPtr, frameDataSize);
        /* Modify packet if need */
        snetLion2TgPacketModify(devObjPtr, trafficGenNumber, frameDataPtr, &frameDataSize);

        devObjPtr = skernelSleep(devObjPtr,interFrameGap);
    }

    /* Burst transmit done */
    SMEM_U32_SET_FIELD(regPtr[0], 6, 1, 1);
}

/*******************************************************************************
*   snetLion2TgPacketGenerator
*
* DESCRIPTION:
*       Packet generator main process.
*
* INPUTS:
*       tgDataPtr  - pointer to traffic generator data.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLion2TgPacketGenerator
(
    IN SKERNEL_TRAFFIC_GEN_STC * tgDataPtr
)
{
    GT_U32 regAddr;
    GT_U32 * regPtr;
    GT_U32 fieldVal;

    while(1)
    {
        tgDataPtr->deviceObjPtr = skernelSleep(tgDataPtr->deviceObjPtr,1000);

        regAddr = SMEM_LION2_TG_CONTROL_0_REG(tgDataPtr->deviceObjPtr,
                                              tgDataPtr->trafficGenNumber);
        regPtr = smemMemGet(tgDataPtr->deviceObjPtr, regAddr);

        /* Enable TG on port */
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 0, 1);
        if(fieldVal == 0)
        {
            continue;
        }

        /* Start trigger */
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 4, 1);
        if(fieldVal == 0)
        {
            continue;
        }
        /* Clear start trigger bit */
        SMEM_U32_SET_FIELD(regPtr[0], 4, 1, 0);

        /* Burst type */
        fieldVal = SMEM_U32_GET_FIELD(regPtr[0], 1, 1);
        if(fieldVal == 0)
        {
            /* Continuous */
            snetLion2TgContinuousTrafficSend(tgDataPtr->deviceObjPtr,
                                             tgDataPtr->trafficGenNumber);
        }
        else
        {
            /* Single burst */
            snetLion2TgBurstSend(tgDataPtr->deviceObjPtr,
                                 tgDataPtr->trafficGenNumber);
        }
    }
}

/*******************************************************************************
*   snetLion3TxSdmaCountControl
*
* DESCRIPTION:
*       Tx SDMA Packet Count Control.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to descriptor
*       txQue           - TxQ number
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID snetLion3TxSdmaCountControl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr,
    IN GT_U32 txQue
)
{
    GT_U32 *txSdmaPktCountCfgQueRegPtr; /* Pointer to register value */
    GT_U32 startReset;                  /* Packet Counter Enabler */
    GT_U32 packetMaxLimit;              /* Max number of packets allowed to be sent from queue */
    GT_U32 packetCounterStatus;         /* Status of packet counter */
    SKERNEL_SDMA_TRANSMIT_DATA_STC * sdmaDataPtr;

    DECLARE_FUNC_NAME(snetLion3TxSdmaCountControl);

    sdmaDataPtr = &devObjPtr->sdmaTransmitInfo.sdmaTransmitData[txQue];

    /* Tx SDMA Packet Generator Config Queue */
    txSdmaPktCountCfgQueRegPtr =
        smemMemGet(devObjPtr, SMEM_CHT_PACKET_COUNT_CONFIG_QUEUE_REG(devObjPtr, txQue));

    /* Packet Counter Enabler */
    startReset =
        SMEM_U32_GET_FIELD(*txSdmaPktCountCfgQueRegPtr, 0, 1);

    if(startReset == 0)
    {
        /* Reset the counter */
        sdmaDataPtr->txQueueCounterReg = 0;
        /* Packet Counter has been terminated by the CPU */
        __LOG(("Packet Counter has been terminated by the CPU"));
        packetCounterStatus = 3;
    }
    else
    {
        /* The max number of packets allowed to be sent from a specific queue */
        packetMaxLimit =
            SMEM_U32_GET_FIELD(*txSdmaPktCountCfgQueRegPtr, 1, 29);

        /* Increment counter */
        sdmaDataPtr->txQueueCounterReg++;

        /* Packet Counter is in progress (is enabled and has not yet reached its configurable limit) */
        __LOG(("Packet Counter is in progress. TxQ counter is %d\n",
               sdmaDataPtr->txQueueCounterReg));

        packetCounterStatus = 1;

        if(packetMaxLimit == sdmaDataPtr->txQueueCounterReg)
        {
            /* Transmit SDMA Queue Command Register - disable queue*/
            sdmaDataPtr->txQueueEn = GT_FALSE;
            /* Clear queue enable bit */
            smemRegFldSet(devObjPtr,
                          SMEM_CHT_TX_SDMA_QUE_CMD_REG(devObjPtr),
                          txQue, 1, 0);

            /* Reset the counter */
            sdmaDataPtr->txQueueCounterReg = 0;

            /* Packet Counter has reached its configurable limit */
            __LOG(("Packet Counter has reached its configurable limit [0x%x] --> so disabling the queue[%d]   \n",
                   packetMaxLimit,txQue));

            packetCounterStatus = 2;
            /* Generate interrupt Tx DMA stopped processing the queue */
            snetChetahDoInterrupt(devObjPtr,
                                  SMEM_CHT_TX_SDMA_INT_CAUSE_REG(devObjPtr),
                                  SMEM_CHT_TX_SDMA_INT_MASK_REG(devObjPtr),
                                  SMEM_CHT_TX_ERROR_INT(txQue),
                                  SMEM_CHT_TX_SDMA_SUM_INT(devObjPtr));
        }
    }
    /* Packet Counter Status */
    SMEM_U32_SET_FIELD(*txSdmaPktCountCfgQueRegPtr, 30, 2, packetCounterStatus);
}

/*******************************************************************************
*   snetLion3TxSdmaGenProcess
*
* DESCRIPTION:
*       Read SDMA descriptor from specific Tx queue and process packet
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       dataPtr         - pointer to OAM keepalive aging info
*       dataSize        - data size
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       The function is called in context of "skernel task"
*       to process "tx from cpu" for specific queue.
*
*******************************************************************************/
static GT_VOID snetLion3TxSdmaGenProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * dataPtr,
    IN GT_U32                  dataSize
)
{
    DECLARE_FUNC_NAME(snetLion3TxSdmaGenProcess);

    GT_U32 txQueue;                     /* Keepalive transmission queue */
    SKERNEL_FRAME_CHEETAH_DESCR_STC * descrPtr;
    GT_BOOL isLastPacket;
    TX_SDMA_GEN_PROCESS_STC txSdmaGenProcessInfo;

    /* Copy TxQ number from buffer */
    memcpy(&txSdmaGenProcessInfo, (GT_U32*)dataPtr, sizeof(txSdmaGenProcessInfo));

    txQueue = txSdmaGenProcessInfo.txQueue;

    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(
        SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SDMA_QUEUE_DAEMON_E,
        &devObjPtr->task_sdmaPerQueueCookieInfo[txQueue].generic);

    /*state from cpu command ()*/
    simLogPacketFrameCommandSet(SIM_LOG_FRAME_COMMAND_TYPE_FROM_CPU_E);
    __LOG(("Start process SDMA TX queue[%d] \n", txQueue));

    if(devObjPtr->sdmaTransmitInfo.sdmaTransmitData[txQueue].txQueueEn == GT_TRUE)
    {
        descrPtr = snetChtEqDuplicateDescr(devObjPtr, NULL);
        /* save the ingress device*/
        descrPtr->ingressDevObjPtr = devObjPtr;

        /*bind the descriptor with the buffer */
        descrPtr->frameBuf = txSdmaGenProcessInfo.bufferId;

        /* Transmit SDMA packet to TxQ */
        snetChtFromCpuDmaTxQueue(devObjPtr, descrPtr, txQueue, &isLastPacket);

        /* SDMA Tx Count Control */
        snetLion3TxSdmaCountControl(devObjPtr, descrPtr, txQueue);
         __LOG(("Ended process SDMA TX queue[%d] \n", txQueue));
    }
    else
    {
        /* message that was put in the queue when queue was enabled ... */
        /* but configuration changed from that point */
    }

    /* decrement the number of packets in the system */
    skernelNumOfPacketsInTheSystemSet(GT_FALSE);
}

/*******************************************************************************
*   snetLion3SdmaTaskPerQueue
*
* DESCRIPTION:
*       SDMA Message Transmission task.
*
* INPUTS:
*       sdmaTxDataPtr  - Pointer to Tx SDMA data
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetLion3SdmaTaskPerQueue
(
    SKERNEL_SDMA_TRANSMIT_DATA_STC  *sdmaTxDataPtr
)
{
    GT_U8 * dataPtr;                    /* Pointer to the data in the buffer */
    GT_U32 dataSize;                    /* Data size */
    GT_U32 txQueue;                     /* Keepalive transmission queue */
                                        /* Pointer to keepalive descriptor */
    GT_U32 * txSdmaPktGenCfgRegPtr;     /* Pointer to register entry value */
    GT_U32 pktInterEn;                  /* Packet Interval Counter feature is enabled */
    GT_U32 pktInterCount;               /* Minimal time between each 2 adjacent packet from queue */
    GT_U32 bypassOwnerEn;               /* Bypass owner bit is enabled */

    SIM_OS_TASK_PURPOSE_TYPE_ENT taskType;
    SBUF_BUF_ID bufferId;               /* Buffer */
    GENERIC_MSG_FUNC txSdmaGenProc;     /* Tx SDMA process function */
    SKERNEL_SDMA_TRANSMIT_DATA_STC * sdmaDataPtr;
    SKERNEL_DEVICE_OBJECT * devObjPtr;
    TX_SDMA_GEN_PROCESS_STC txSdmaGenProcessInfo;
    GT_U32 noBuffersAllocationCounter;


#ifdef _WIN32
    /* call SHOST to register the application task in the asic task table*/
    SHOSTG_psos_reg_asic_task();
#endif /*_WIN32*/

    txQueue = sdmaTxDataPtr->txQueue;
    devObjPtr = sdmaTxDataPtr->devObjPtr;

    /* set task type - only after SHOSTG_psos_reg_asic_task */
    taskType = SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SDMA_QUEUE_DAEMON_E;
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(taskType,
        &devObjPtr->task_sdmaPerQueueCookieInfo[txQueue].generic);

    /* state from cpu command */
    simLogPacketFrameCommandSet(SIM_LOG_FRAME_COMMAND_TYPE_FROM_CPU_E);


    /* Bind Tx SDMA tramsmit generic function */
    txSdmaGenProc = snetLion3TxSdmaGenProcess;

    noBuffersAllocationCounter = 0;

    /* Send packets that are in this queue periodically: <Tx SDMA Delay Interval> */
    while (1)
    {
        /* support possible change of the device during 'soft reset' */
        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(
            SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SDMA_QUEUE_DAEMON_E,
            &devObjPtr->task_sdmaPerQueueCookieInfo[txQueue].generic);

        sdmaDataPtr = &devObjPtr->sdmaTransmitInfo.sdmaTransmitData[txQueue];
        /* Tx SDMA Packet Generator Config Queue */
        txSdmaPktGenCfgRegPtr = &sdmaDataPtr->txQueuePacketGenCfgReg;
        bypassOwnerEn = SMEM_U32_GET_FIELD(*txSdmaPktGenCfgRegPtr, 0, 1);

        /* Packet generator or TxQ is disabled */
        if(bypassOwnerEn == 0 || sdmaDataPtr->txQueueEn == GT_FALSE)
        {
            /* the task was 'paused' so we wait until 'resumed' */
            devObjPtr = skernelSleep(devObjPtr,500);
            continue;
        }

        /* Get buffer -- allocate size for max supported frame size */
        /* limit allocation to check that we still have enough buffers for proper system operations */
        bufferId = sbufAllocWithProtectedAmount(devObjPtr->bufPool, SBUF_DATA_SIZE_CNS/*max*/ , SDMA_GEN_MIN_FREE_BUFFERS_CNS);
        if (bufferId == NULL)
        {
            /* Alert no buffers allocation printout only after 100 allocation failures */
            noBuffersAllocationCounter++;
            if( 0 == noBuffersAllocationCounter%100 )
            {
                simWarningPrintf("snetLion3SdmaTaskPerQueue: no buffers for Tx SDMA traffic generator \n");
            }
            devObjPtr = skernelSleep(devObjPtr,100);   /* currently no buffers for the Tx SDMA traffic generator.
                                   sleep for 100 miliseconds and then try again ,
                                   maybe there will be buffers free then */
            continue;
        }

        txSdmaGenProcessInfo.bufferId = bufferId;
        txSdmaGenProcessInfo.txQueue = txQueue;

        /* increment the number of packets in the system */
        skernelNumOfPacketsInTheSystemSet(GT_TRUE);

       /* Get actual data pointer */
        sbufDataGet(bufferId, (GT_U8**)&dataPtr, &dataSize);

        /* Put the name of the function into the message */
        memcpy(dataPtr, &txSdmaGenProc, sizeof(GENERIC_MSG_FUNC));
        dataPtr+=sizeof(GENERIC_MSG_FUNC);

        /* Copy info to buffer */
        memcpy(dataPtr, &txSdmaGenProcessInfo, sizeof(txSdmaGenProcessInfo));
        dataPtr+=sizeof(txSdmaGenProcessInfo);

        /* Set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* Set message type of buffer */
        bufferId->dataType = SMAIN_MSG_TYPE_GENERIC_FUNCTION_E;

        /* Put buffer to queue */
        squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));

        /* Packet Interval Counter Enable */
        pktInterEn = SMEM_U32_GET_FIELD(*txSdmaPktGenCfgRegPtr, 1, 1);

        if(pktInterEn)
        {
            /* Packet Interval Counter */
            pktInterCount = SMEM_U32_GET_FIELD(*txSdmaPktGenCfgRegPtr, 2, 30);

            /* Convert nanoseconds in granularity of 40 ns to miliseconds */
            devObjPtr = skernelSleep(devObjPtr,(pktInterCount * 40) / 1000000);
        }
    }
}

