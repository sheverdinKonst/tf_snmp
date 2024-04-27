/*******************************************************************************
*              Copyright 2001, GALILEO TECHNOLOGY, LTD.
*
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL. NO RIGHTS ARE GRANTED
* HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT OF MARVELL OR ANY THIRD
* PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE DISCRETION TO REQUEST THAT THIS
* CODE BE IMMEDIATELY RETURNED TO MARVELL. THIS CODE IS PROVIDED "AS IS".
* MARVELL MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS
* ACCURACY, COMPLETENESS OR PERFORMANCE. MARVELL COMPRISES MARVELL TECHNOLOGY
* GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, MARVELL INTERNATIONAL LTD. (MIL),
* MARVELL TECHNOLOGY, INC. (MTI), MARVELL SEMICONDUCTOR, INC. (MSI), MARVELL
* ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. (MJKK), GALILEO TECHNOLOGY LTD. (GTL)
* AND GALILEO TECHNOLOGY, INC. (GTI).
********************************************************************************
* smainSwitchFabric.c
*
* DESCRIPTION:
*       This module is Switch Fabric API and FA/XBAR main tasks functions
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <common/SBUF/sbuf.h>
#include <common/SQue/squeue.h>

/**************************** defines *****************************************/

#define SMAIN_SWITCH_FABRIC_FAFOX_INTERNAL_FPORT_CNS        0x4

static GT_SEM smainSwitchFabricInitSem;

/*******************************************************************************
*   smainSwitchFabricDevObjGetByUplinkId
*
* DESCRIPTION:
*       Get device object from the number of the upLinkId
*
* INPUTS:
*       upLinkId        - The ID of the uplink.
*       srcDeviceObjPtr - pointer to the source device object
*
* OUTPUTS:
*       destDeviceObjPtr - destination device object.
*
* RETURNS:
*       void
*
* COMMENTS:
*      if fa/fap with device is 2 is connected to the pp with device id 0 ,
*      the number of the uplink is determined by the pp id
*      (0 in the above example).
*
*******************************************************************************/
void smainSwitchFabricDevObjGetByUplinkId
(
    IN    GT_U32                    upLinkId,
    OUT   SKERNEL_DEVICE_OBJECT     **destDeviceObjPtr
)
{
    if (upLinkId >= SMAIN_MAX_NUM_OF_DEVICES_CNS)
    {
        *destDeviceObjPtr = NULL;
        return;
    }

    *destDeviceObjPtr = smainDeviceObjects[upLinkId] ;
}


/*******************************************************************************
*   smainSwitchFabricFaTask
*
* DESCRIPTION:
*       fabric adapter main device task.
*
* INPUTS:
*       devObjPtr - pointer to the device object for task.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       This is the main task for the fabric adapter .
*       Packet from pp or from slan or from CPU are handled at this function.
*
*******************************************************************************/
void smainSwitchFabricFaTask(
        IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    SBUF_BUF_ID             bufferId; /* buffer id */
    GT_U8                   *dataPtr; /* pointer to the start of buffer's data*/
    GT_U32                  dataSize; /* size of buffer's data */
    SKERNEL_UPLINK_DESC_STC uplinkDesc;  /* uplink descriptor*/


    /* notify that task starts to work       */
    SIM_OS_MAC(simOsSemSignal)(smainSwitchFabricInitSem) ;

    while(1)
    {
        /* get buffer */
        bufferId = SIM_CAST_BUFF(squeBufGet(deviceObjPtr->queueId));

        /* process a buffer from SLAN */
        if (bufferId->srcType == SMAIN_SRC_TYPE_SLAN_E)/* frame from slan*/
        {/* can be pcs pings , mb or frames from uplink , slan->fa */
           memset(&uplinkDesc, 0, sizeof(uplinkDesc));
           uplinkDesc.data.PpPacket.source.frameBuf = bufferId;
           uplinkDesc.data.PpPacket.source.srcData  = bufferId->srcData;
           uplinkDesc.data.PpPacket.source.ingressFport =  bufferId->srcData;
           /* process frame */
           snetFrameProcess(deviceObjPtr, bufferId, bufferId->srcData);
        }

        else if (bufferId->srcType == SMAIN_SRC_TYPE_CPU_E)/* cpu->fa */
        { /* cpu message */
           sbufDataGet(bufferId,&dataPtr,&dataSize);
           snetCpuMessageProcess(deviceObjPtr,dataPtr);
        }

        else if (bufferId->srcType == SMAIN_SRC_TYPE_UPLINK_E)/* pp->fa , pp->pp*/
        {
           memset(&uplinkDesc, 0, sizeof(SKERNEL_UPLINK_DESC_STC));
           sbufDataGet(bufferId,&dataPtr,&dataSize);
           memcpy(&uplinkDesc,dataPtr,sizeof(SKERNEL_UPLINK_DESC_STC));
           uplinkDesc.data.PpPacket.source.frameBuf = bufferId;
           uplinkDesc.data.PpPacket.source.frameBuf->actualDataPtr = bufferId->actualDataPtr;
           uplinkDesc.data.PpPacket.source.ingressFport=SMAIN_SWITCH_FABRIC_FAFOX_INTERNAL_FPORT_CNS;
           snetProcessFrameFromUpLink(deviceObjPtr,&uplinkDesc);
        }

        /* free buffer */
        sbufFree(deviceObjPtr->bufPool, bufferId);
    }
}

/*******************************************************************************
*   smainSwitchFabricFrame2UplinkSend
*
* DESCRIPTION:
*        transmit frame to uplink
*
* INPUTS:
*        deviceObj   -  pointer to device object.
*        UplinkDesc  -  pointer to the uplink descriptor.
*
* OUTPUT:
*
* RETURNS:
*       GT_OK   - successful
*       GT_FAIL - failure
*
* COMMENTS:
*        the function is used by the fa/fap device and by the pp device objects.
*
*******************************************************************************/
GT_STATUS smainSwitchFabricFrame2UplinkSend
(
    IN SKERNEL_DEVICE_OBJECT    * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC  * UplinkDesc
)
{
    SBUF_BUF_ID                     bufferId;    /* buffer */
    SKERNEL_DEVICE_OBJECT           *dest_deviceObj_PTR=NULL;
    const GT_U16                    uplinkDescrSize=sizeof(SKERNEL_UPLINK_DESC_STC);
    SBUF_BUF_STC                    *dstBuf_PTR;

    smainSwitchFabricDevObjGetByUplinkId(devObjPtr->uplink.partnerDeviceID ,
                                        &dest_deviceObj_PTR);

    if(dest_deviceObj_PTR->bufPool == NULL)
    {
        /* buffer pool of the target device is not initialized */
        skernelFatalError("smainSwitchFabricFrame2UplinkSend: bufPool is not \
                         initialized yet. uplinkID=%d",                      \
                         dest_deviceObj_PTR->uplink.partnerDeviceID);
    }

    /* allocate buffer */
    /* get the buffer and put it in the queue */
    bufferId = sbufAlloc(dest_deviceObj_PTR->bufPool,
                         UplinkDesc->data.PpPacket.source.byteCount+uplinkDescrSize);
    if (bufferId == NULL)
    {
        printf(" smainSwitchFabricFrame2UplinkSend : no buffers for process \n");
        return GT_FAIL;
    }

    dstBuf_PTR = (SBUF_BUF_STC *) bufferId;
    dstBuf_PTR->srcType = SMAIN_SRC_TYPE_UPLINK_E;
    dstBuf_PTR->srcData = devObjPtr->uplink.partnerDeviceID;
    dstBuf_PTR->dataType = SMAIN_MSG_TYPE_FRAME_E;

    memcpy(dstBuf_PTR->actualDataPtr,UplinkDesc,uplinkDescrSize);

    /* then copy the frame itself */
    memcpy(dstBuf_PTR->actualDataPtr+uplinkDescrSize,
           UplinkDesc->data.PpPacket.source.frameBuf->actualDataPtr,
           UplinkDesc->data.PpPacket.source.frameBuf->actualDataSize);

    /* get the buffer and put it in the queue */
    sbufDataSet(bufferId, dstBuf_PTR->actualDataPtr,
                UplinkDesc->data.PpPacket.source.byteCount+uplinkDescrSize);

    /* put buffer on the queue */
    squeBufPut(dest_deviceObj_PTR->queueId ,SIM_CAST_BUFF(bufferId));

    return GT_OK;
}

/*******************************************************************************
*   smainSwitchFabricInit
*
* DESCRIPTION:
*    Initiate the smain switch fabric module .
*
* INPUTS:
*       devObjPtr -  pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The function is called by smainSwitchFabricInit function (smain module).
*
*******************************************************************************/
void smainSwitchFabricInit
(
    SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_TASK_HANDLE          taskHandl = NULL;        /* task handle */
    GT_TASK_PRIORITY_ENT    taskPriority = GT_TASK_PRIORITY_HIGHEST;

    /* create semaphore for init process */
    smainSwitchFabricInitSem = SIM_OS_MAC(simOsSemCreate)(0,1);
    if (smainSwitchFabricInitSem == (GT_SEM)0)
    {
        skernelFatalError(" smainSwitchFabricInit: cannot create semaphore");
    }

    taskHandl = SIM_OS_MAC(simOsTaskCreate)(
                     taskPriority,
                     (unsigned (__TASKCONV *)(void*))smainSwitchFabricFaTask,
                     (void *) deviceObjPtr);
    if (taskHandl == NULL)
    {
        skernelFatalError(" smainSwitchFabricInit: cannot create main task for"\
                          " device %u",deviceObjPtr->deviceId );
    }

    /* notify that task of fap starts to work */
    SIM_OS_MAC(simOsSemWait)(smainSwitchFabricInitSem,SIM_OS_WAIT_FOREVER);
}

