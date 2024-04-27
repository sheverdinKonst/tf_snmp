/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sEmbeddedCpu.c
*
* DESCRIPTION:
*       This is a implementation of the "embedded CPU" module of SKernel.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/

#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/sEmbeddedCpu/sEmbeddedCpu.h>


#define DSA_TAG_START_OFFSET 12 /*6+6 -- first byte of Extended DSA tag */

#define CPU_CODE_OFFSET 19 /*6+6+7 -- last byte of Extended DSA tag */

/* this value should be read from a register of this embedded CPU , but for now
   I define it "hard coded" --

   the CPU code of the traffic that the PP send to the external CPU via the
   embedded CPU
*/
#define USER_DEFINED_CPU_CODE_SRC_IS_PP             253
/* this value should be read from a register of this embedded CPU , but for now
   I define it "hard coded" --

   the CPU code of the traffic that the external CPU send to the PP via the
   embedded CPU
*/
#define USER_DEFINED_CPU_CODE_SRC_IS_EXTERNAL_CPU   254
/* this value should be read from a register of this embedded CPU , but for now
   I define it "hard coded" --

   the CPU code of the traffic that the embedded CPU send to the external CPU for
   traffic that came from PP (replace the CPU code of
    USER_DEFINED_CPU_CODE_SRC_IS_PP that is on the packet)
*/
#define USER_DEFINED_CPU_CODE_SRC_IS_EMBEDDED_CPU   255



/*******************************************************************************
*   snetEmbeddedCpuProcessInit
*
* DESCRIPTION:
*       Init the module.
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
*
*******************************************************************************/
void snetEmbeddedCpuProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    return;
}

/*******************************************************************************
*   sEmbeddedCpuTxFrame
*
* DESCRIPTION:
*        Receive frame from the PP (on the DMA) of the Embedded CPU.
* INPUTS:
*        devObjPtr  - pointer to the Embedded CPU device object.
*        bufferId   - frame data buffer Id
*        frameData  - pointer to the data
*        frameSize  - data size
*        cameFromExternalCpu - distinguish between from external CPU and from PP
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
******************************************************************************/
static void sEmbeddedCpuTxFrame
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SBUF_BUF_ID    bufferId,
    IN GT_U8        * frameData,
    IN GT_U32         frameSize,
    IN GT_BOOL        cameFromExternalCpu
)
{
    GT_U32  word1 = 0;/* word1 of the 'FROM_CPU' to build */
    GT_BIT  dropOnSource,packetIsLooped; /*bit in DSA tag */

    if(cameFromExternalCpu == GT_TRUE)
    {
        /* came from the external CPU going to the PP */

        /*********************************/
        /* modify the DSA tag CPU format */
        /* from "TO_CPU" to "FROM_CPU"   */
        /*********************************/


        /* convert WORD0 from "TO_CPU" to "FROM_CPU" */
        /* all bit are the same except for bits 16..18 */

        frameData[DSA_TAG_START_OFFSET+0] |= (GT_U8)(1<<6);/* 31..30 set the 'FROM_CPU' */

        /* bit 16 - CFI    --> set according to CFI in the 'TO_CPU' */
        /* bit 17 - TC[0]  --> set TC to 7 --> so bit is 1 */
        /* bit 18 - isVidx --> must set to 0 */

        frameData[DSA_TAG_START_OFFSET+1] &= (GT_U8)(~(0x7));/* clear the 3 bits */
        frameData[DSA_TAG_START_OFFSET+1] |= (GT_U8)(1<<1);/* set the TC[0]*/
        if(frameData[DSA_TAG_START_OFFSET+4] & (GT_U8)(1<<6))
        {
            frameData[DSA_TAG_START_OFFSET+1] |= 1;/* set the CFI */
        }

        /* convert WORD1 from "TO_CPU" to "FROM_CPU" */
        /* all bit are the same except for bits 16..18 */

        /* get bits from the word1 of the 'to_cpu'*/

        dropOnSource   = (frameData[DSA_TAG_START_OFFSET+4] & (1<<5)) ? 1 :0;/*bit 29*/
        packetIsLooped = (frameData[DSA_TAG_START_OFFSET+4] & (1<<4)) ? 1 :0;/*bit 28*/

        /* build the DSA WORD 1 */
        /* bits 31-28 stay 0 */
        SMEM_U32_SET_FIELD(word1 , 27, 1, 1);/*TC[2] */
        SMEM_U32_SET_FIELD(word1 , 26, 1, dropOnSource);/*dropOnSource */
        SMEM_U32_SET_FIELD(word1 , 25, 1, packetIsLooped);/*packetIsLooped */
        /* bits 24-15 stay 0 */
        SMEM_U32_SET_FIELD(word1 , 14, 1, 1);/*TC[1] */
        /* bits 13-0 stay 0 */

        /* convert word to bytes */
        frameData[DSA_TAG_START_OFFSET+4] = (GT_U8)SMEM_U32_GET_FIELD(word1,24,8);
        frameData[DSA_TAG_START_OFFSET+5] = (GT_U8)SMEM_U32_GET_FIELD(word1,16,8);
        frameData[DSA_TAG_START_OFFSET+6] = (GT_U8)SMEM_U32_GET_FIELD(word1, 8,8);
        frameData[DSA_TAG_START_OFFSET+7] = (GT_U8)SMEM_U32_GET_FIELD(word1, 0,8);
    }
    else
    {
        /* came from the PP going to the external CPU */

        /*************************************/
        /* modify the DSA tag CPU code value */
        /*************************************/
        frameData[CPU_CODE_OFFSET] = USER_DEFINED_CPU_CODE_SRC_IS_EMBEDDED_CPU;
    }

    /**************************************************/
    /* call to the coupled device to handle the frame */
    /**************************************************/
    snetFromEmbeddedCpuProcess(devObjPtr->embeddedCpuInfo.coupledDevice.parentPpDevicePtr,/*the embedded CPU device has pointer to it's PP*/
                     bufferId);

}

/*******************************************************************************
*   sEmbeddedCpuRxFrame
*
* DESCRIPTION:
*        Receive frame from the PP (on the DMA) of the Embedded CPU.
* INPUTS:
*        embeddedDevObjPtr  - pointer to the Embedded CPU device object.
*        bufferId   - frame data buffer Id
*        frameData  - pointer to the data
*        frameSize  - data size
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
******************************************************************************/
void sEmbeddedCpuRxFrame
(
    IN void         * embeddedDevObjPtr,
    IN SBUF_BUF_ID    bufferId,
    IN GT_U8        * frameData,
    IN GT_U32         frameSize
)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr = embeddedDevObjPtr;
    GT_BOOL        cameFromExternalCpu; /*distinguish between from external CPU and from PP*/

    if(0 == SKERNEL_DEVICE_FAMILY_EMBEDDED_CPU(devObjPtr->deviceType))
    {
        skernelFatalError("sEmbeddedCpuRxFrame: this device is not Embedded CPU device... \n");
    }

    /* check the incoming CPU code to see the source of the packet , in order to
       know the needed treatment*/
    switch(frameData[CPU_CODE_OFFSET])
    {
        case USER_DEFINED_CPU_CODE_SRC_IS_PP:
            cameFromExternalCpu = GT_FALSE;
            break;
        case USER_DEFINED_CPU_CODE_SRC_IS_EXTERNAL_CPU:
            cameFromExternalCpu = GT_TRUE;
            break;
        default:
            skernelFatalError("sEmbeddedCpuRxFrame: unknown CPU code in embedded CPU \n");
            return;
    }

    if(cameFromExternalCpu == GT_FALSE)
    {
        /* we need to decrypt the DTLS data */

        /* not supported in simulation ... */
    }
    else
    {
        /* we need to encrypt and add the DTLS data */

        /* not supported in simulation ... */
    }

    /***************************************************************/
    /* TX the frame --- since we not do DTLS encryption/decryption */
    /***************************************************************/
    sEmbeddedCpuTxFrame(devObjPtr,bufferId,frameData,frameSize,cameFromExternalCpu);

    return;
}

/*******************************************************************************
*   smemEmbeddedCpuFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       accessType  - Read/Write operation
*       address     - address of memory(register or table).
*       memsize     - size of memory
*
* OUTPUTS:
*     activeMemPtrPtr - pointer to the active memory entry or NULL if not exist.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static void * smemEmbeddedCpuFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    static GT_U32       stub_mem[100]={0};

    /* STUB implementation */
    *activeMemPtrPtr = NULL;

    return &stub_mem[0];
}

/*******************************************************************************
*   smemEmbeddedCpuInit
*
* DESCRIPTION:
*       Init memory module for an embedded CPU device.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
void smemEmbeddedCpuInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    deviceObjPtr->devFindMemFunPtr = (void *)smemEmbeddedCpuFindMem;

    return;
}

