/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetPuma.c
*
* DESCRIPTION:
*       This is a external API definition for Puma frame processing.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#include <os/simTypesBind.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snetPuma.h>
#include <asicSimulation/SKernel/smem/smemGM.h>
#include <gmSimulation/GM/TrafficAPI.h>
#include <gmSimulation/GM/GMApi.h>


/* WA for dead lock in GM system. the processing of frames to CPU may cause deadlock:
    wa to deadlock in win32 distributed system with GM:
    send high rate of packets to GM that need to go to CPU.
    the GM fill all the chain of descriptors and then 'polling' on descriptor with device ownership.
    but the CPU change give such descriptor because all task are 'locked' by interrupt context.
    and the interrupt context is locked 'read register' on the GM semaphore that was taken by the packet that the GM process.
 */
#ifdef _WIN32  /* only in WIN32 because there are no real task locking on the linux */
GT_U32 waGmFramesSleep = 100;
#else
GT_U32 waGmFramesSleep = 0;
#endif /*_WIN32*/

/* command to fine tunning the needed rate (for debugging) */
GT_U32  simWaGmFramesSleepSet(GT_U32    newValue)
{
    GT_U32  prevValue = waGmFramesSleep;
    waGmFramesSleep = newValue;
    return prevValue;
}


/*******************************************************************************
*   snetPumaProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void snetPumaProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->devFrameProcFuncPtr =      snetPumaProcessFrameFromSlan;
    devObjPtr->devPortLinkUpdateFuncPtr = snetGmLinkStateNotify;
}

/*******************************************************************************
*   snetPumaProcessFrameFromSlan
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame in FA
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
*      The frame can be traffic or message from remote CPU.
*******************************************************************************/
void snetPumaProcessFrameFromSlan
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    /* WA for dead lock in GM system. the processing of frames to CPU may cause deadlock:
        wa to dealock in win32 distributed system with GM:
        send high rate of packets to GM that need to go to CPU.
        the GM fill all the chain of descriptors and then 'polling' on descriptor with device ownership.
        but the CPU change give such descriptor because all task are 'locked' by interrupt context.
        and the interrupt context is locked 'read register' on the GM semaphore that was taken by the packet that the GM process.
     */
    if(waGmFramesSleep && (devObjPtr->deviceFamily != SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY))
    {
        SIM_OS_MAC(simOsSleep)(waGmFramesSleep);
    }
    /*
        with this sleep we allow other tasks to do read/write registers when there
        is no locking of the GM by packets...
    */

    /* while the GM takes it's own semaphore , we must take ours before , so
        we will not get into deadlock when the 'interrupt context' will do 'read register'
        that will do scib lock and then try to take GM semaphore , but if the
        GM is into processing ... deadlock */
    SCIB_SEM_TAKE;

    if(devObjPtr->deviceFamily == SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY)
    {
        GT_U32  pipeId = (srcPort / 16) + devObjPtr->portGroupId;
        GT_U32  localPort = (srcPort % 16);

        /* the Puma3 device has 4 pipes that handle packets but there
           are only 2 MG units . so we have only devObjPtr->portGroupId = 0 and 1 */
        /* but still we need to notify the GM that packet came from
            pipe 0 or pipe 1 or pipe 2 or pipe 3 with local port 0..11 */


        /* multi core device */
        ppSendPacket(SMEM_GM_GET_GM_DEVICE_ID(devObjPtr), pipeId , localPort, (char*)bufferId->actualDataPtr, bufferId->actualDataSize);

    }
    else
    {
        /* other GM devices */
        ppSendPacket(SMEM_GM_GET_GM_DEVICE_ID(devObjPtr),devObjPtr->portGroupId, srcPort, (char*)bufferId->actualDataPtr, bufferId->actualDataSize);
    }

    SCIB_SEM_SIGNAL;

}

/*******************************************************************************
*   snetGmLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       port      - port number.
*       linkState - link state (0 - down, 1 - up)
*
*******************************************************************************/
GT_VOID snetGmLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
)
{
    GT_U32 rc;

    rc = ppSetPortControl(SMEM_GM_GET_GM_DEVICE_ID(devObjPtr), devObjPtr->portGroupId, port, (GT_BOOL)linkState);
    if(rc != 1)
    {
        skernelFatalError(" skernelInit: ppSetPortControl failed");
    }
}

