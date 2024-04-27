/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemOcelot.c
*
* DESCRIPTION:
*       Initialization of a memory space in Ocelot simulation.
*       -- Ocelot devices
*
* FILE REVISION NUMBER:
*       $Revision: 11 $
*
*******************************************************************************/

#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemOcelot.h>



/*bits wo words*/
#define CONVERT_BITS_TO_WORDS(bits) ((bits+31)/32)


/* Register special function index in function array (Bits 20:27)*/
#define     REG_SPEC_FUNC_INDEX                  0xFF00000

/* Global constants definition */
#define SMEM_OCELOT_UNIT_INDEX_FIRST_BIT_CNS               20

static void smemFx950InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemFX950RegsInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr
);

static void * smemOcelotFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

void smemFx950TableMemInit
(
    IN SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr

);

void smemFx950ActiveWriteTable
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,/* contains pointer to the table */
    IN         GT_U32 * inMemPtr
);

static void ocelotWrapperFap20MWriteFunActiveMemory
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,/* contains pointer to the table */
    IN         GT_U32 * inMemPtr
);

static void ocelotWrapperFap20MReadActiveMemory
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);


/* Active memory entry that the Ocelot will wrap the active memory returned from the FAP20M */
static SMEM_ACTIVE_MEM_ENTRY_STC ocelotWrapperFap20M =
{
    0,/*not used*/
    0,/*not used*/
    ocelotWrapperFap20MReadActiveMemory,
    0,/*not used*/
    ocelotWrapperFap20MWriteFunActiveMemory,
    0/*not used*/
};
/*******************************************************************************
* smemFX950RegsInit
*
* DESCRIPTION:
*       Init memory module for a FX950 device.
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
static void smemFX950RegsInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32          address[6];
    GT_U32          i;
    {/*start of unit MG - Management unit (controls access to registers and tables)  */
        {/*start of unit globalConfig */

            devMemInfoPtr->regsAddr.MG.globalConfig.addrCompletion0 = 0x00000000;


            devMemInfoPtr->regsAddr.MG.globalConfig.addrCompletion1 = 0x00000004;


            devMemInfoPtr->regsAddr.MG.globalConfig.addrCompletion2 = 0x00000008;


            devMemInfoPtr->regsAddr.MG.globalConfig.lastReadTimeStamp = 0x00000040;


            devMemInfoPtr->regsAddr.MG.globalConfig.deviceID = 0x0000004c;


            devMemInfoPtr->regsAddr.MG.globalConfig.vendorID = 0x00000050;


            devMemInfoPtr->regsAddr.MG.globalConfig.globalCtrlReg = 0x00000058;


            devMemInfoPtr->regsAddr.MG.globalConfig.mgGlueConfig = 0x00000080;



        }/*end of unit globalConfig */


        {/*start of unit globalInterrupt */
            devMemInfoPtr->regsAddr.MG.globalInterrupt.globalInterruptCause = 0x00000030;


            devMemInfoPtr->regsAddr.MG.globalInterrupt.globalInterruptMask = 0x00000034;


            devMemInfoPtr->regsAddr.MG.globalInterrupt.miscellaneousInterruptCause = 0x00000038;


            devMemInfoPtr->regsAddr.MG.globalInterrupt.miscellaneousInterruptMask = 0x0000003c;



        }/*end of unit globalInterrupt */


        {/*start of unit interruptCoalescingConfig */
            devMemInfoPtr->regsAddr.MG.interruptCoalescingConfig.interruptCoalescingConfig = 0x000000e0;



        }/*end of unit interruptCoalescingConfig */


        {/*start of unit TWSIConfig */
            devMemInfoPtr->regsAddr.MG.TWSIConfig.TWSIGlobalConfig = 0x00000010;


            devMemInfoPtr->regsAddr.MG.TWSIConfig.TWSILastAddr = 0x00000014;


            devMemInfoPtr->regsAddr.MG.TWSIConfig.TWSITimeoutLimit = 0x00000018;


            devMemInfoPtr->regsAddr.MG.TWSIConfig.TWSIStateHistory = 0x00000020;


            devMemInfoPtr->regsAddr.MG.TWSIConfig.TWSIInternalBaudRate = 0x0001000c;



        }/*end of unit TWSIConfig */


        {/*start of unit userDefined */
            devMemInfoPtr->regsAddr.MG.userDefined.userDefinedReg0 = 0x000000f0;


            devMemInfoPtr->regsAddr.MG.userDefined.userDefinedReg1 = 0x000000f4;


            devMemInfoPtr->regsAddr.MG.userDefined.userDefinedReg2 = 0x000000f8;


            devMemInfoPtr->regsAddr.MG.userDefined.userDefinedReg3 = 0x000000fc;



        }/*end of unit userDefined */

    }/*end of unit MG - Management unit (controls access to registers and tables)  */

    {/*start of unit PEX - PCI express  */
        {/*start of unit PEXAddrWindowCtrl */
            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow0Ctrl = 0x00071820;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow0Base = 0x00071824;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow0Remap = 0x0007182c;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow1Ctrl = 0x00071830;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow1Base = 0x00071834;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow1Remap = 0x0007183c;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow2Ctrl = 0x00071840;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow2Base = 0x00071844;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow2Remap = 0x0007184c;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow3Ctrl = 0x00071850;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow3Base = 0x00071854;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow3Remap = 0x0007185c;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow4Ctrl = 0x00071860;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow4Base = 0x00071864;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow4Remap = 0x0007186c;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow5Ctrl = 0x00071880;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow5Base = 0x00071884;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXWindow5Remap = 0x0007188c;


            devMemInfoPtr->regsAddr.PEX.PEXAddrWindowCtrl.PEXDefaultWindowCtrl = 0x000718b0;



        }/*end of unit PEXAddrWindowCtrl */


        {/*start of unit PEXBARCtrl */
            devMemInfoPtr->regsAddr.PEX.PEXBARCtrl.PEXBAR1Ctrl = 0x00071804;


            devMemInfoPtr->regsAddr.PEX.PEXBARCtrl.PEXBAR2Ctrl = 0x00071808;



        }/*end of unit PEXBARCtrl */


        {/*start of unit PEXConfigCyclesGeneration */
            devMemInfoPtr->regsAddr.PEX.PEXConfigCyclesGeneration.PEXConfigAddr = 0x000718f8;


            devMemInfoPtr->regsAddr.PEX.PEXConfigCyclesGeneration.PEXConfigData = 0x000718fc;



        }/*end of unit PEXConfigCyclesGeneration */


        {/*start of unit PEXConfigHeader */
            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXDeviceAndVendorID = 0x00070000;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXCommandAndStatus = 0x00070004;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXClassCodeAndRevisionID = 0x00070008;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBISTHeaderTypeAndCacheLineSize = 0x0007000c;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBAR0Internal = 0x00070010;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBAR0InternalHigh = 0x00070014;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBAR1 = 0x00070018;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBAR1High = 0x0007001c;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBAR2 = 0x00070020;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXBAR2High = 0x00070024;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXSubsystemDeviceAndVendorID = 0x0007002c;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXCapability = 0x00070060;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXDeviceCapabilities = 0x00070064;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXDeviceCtrlStatus = 0x00070068;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXLinkCapabilities = 0x0007006c;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXLinkCtrlStatus = 0x00070070;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXAdvancedErrorReportHeader = 0x00070100;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXUncorrectableErrorStatus = 0x00070104;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXUncorrectableErrorMask = 0x00070108;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXUncorrectableErrorSeverity = 0x0007010c;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXCorrectableErrorStatus = 0x00070110;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXCorrectableErrorMask = 0x00070114;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXAdvancedErrorCapabilityAndCtrl = 0x00070118;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXHeaderLogFirstDWORD = 0x0007011c;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXHeaderLogSecondDWORD = 0x00070120;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXHeaderLogThirdDWORD = 0x00070124;


            devMemInfoPtr->regsAddr.PEX.PEXConfigHeader.PEXHeaderLogFourthDWORD = 0x00070128;



        }/*end of unit PEXConfigHeader */


        {/*start of unit PEXCtrlAndStatus */
            devMemInfoPtr->regsAddr.PEX.PEXCtrlAndStatus.PEXCtrl = 0x00071a00;


            devMemInfoPtr->regsAddr.PEX.PEXCtrlAndStatus.PEXStatus = 0x00071a04;


            devMemInfoPtr->regsAddr.PEX.PEXCtrlAndStatus.PEXCompletionTimeout = 0x00071a10;


            devMemInfoPtr->regsAddr.PEX.PEXCtrlAndStatus.PEXFlowCtrl = 0x00071a20;


            devMemInfoPtr->regsAddr.PEX.PEXCtrlAndStatus.PEXAcknowledgeTimers1X = 0x00071a40;


            devMemInfoPtr->regsAddr.PEX.PEXCtrlAndStatus.PEXTLCtrl = 0x00071ab0;



        }/*end of unit PEXCtrlAndStatus */


        {/*start of unit PEXInterrupt */
            devMemInfoPtr->regsAddr.PEX.PEXInterrupt.PEXInterruptCause = 0x00071900;


            devMemInfoPtr->regsAddr.PEX.PEXInterrupt.PEXInterruptMask = 0x00071910;



        }/*end of unit PEXInterrupt */

    }/*end of unit PEX - PCI express  */

    {/*start of unit ingr - ingress  */
        {/*start of unit choppingConfig */
            devMemInfoPtr->regsAddr.ingr.choppingConfig.choppingSize = 0x00202000;


            devMemInfoPtr->regsAddr.ingr.choppingConfig.choppedPktsCntr = 0x00202004;


            devMemInfoPtr->regsAddr.ingr.choppingConfig.outgoingChunksCntr = 0x00202008;


            {/*0x202010+w*0x4*/
                GT_U32    w;
                for(w = 0 ; w <= 3 ; w++) {
                    devMemInfoPtr->regsAddr.ingr.choppingConfig.targetChoppingEnable[w] =
                        0x202010+w*0x4;
                }/* end of loop w */
            }/*0x202010+w*0x4*/


            devMemInfoPtr->regsAddr.ingr.choppingConfig.choppingEnablers = 0x00202020;


            devMemInfoPtr->regsAddr.ingr.choppingConfig.perTrafficClassChoppingEnablers = 0x00202024;



        }/*end of unit choppingConfig */


        {/*start of unit cntr */
            devMemInfoPtr->regsAddr.ingr.cntr.queueNotValidCntr = 0x00203000;


            {/*0x203004+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.CRCErrorLinkCntr[l] =
                        0x203004+l*0x4;
                }/* end of loop l */
            }/*0x203004+l*0x4*/


            {/*0x20300c+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.LLFCCntr[l] =
                        0x20300c+l*0x4;
                }/* end of loop l */
            }/*0x20300c+l*0x4*/


            {/*0x203014+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkCPUMailCntr[l] =
                        0x203014+l*4;
                }/* end of loop l */
            }/*0x203014+l*4*/


            {/*0x203020+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkPktErrorCntr[l] =
                        0x203020+l*4;
                }/* end of loop l */
            }/*0x203020+l*4*/


            {/*0x203028+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkByteCountErrorCntr[l] =
                        0x203028+l*4;
                }/* end of loop l */
            }/*0x203028+l*4*/


            {/*0x203030+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.UCLinkPktPassCntr[l] =
                        0x203030+l*0x4;
                }/* end of loop l */
            }/*0x203030+l*0x4*/


            {/*0x203038+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.MCLinkPktPassCntr[l] =
                        0x203038+l*0x4;
                }/* end of loop l */
            }/*0x203038+l*0x4*/


            {/*0x203040+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkUcFifoFullDropCntr[l] =
                        0x203040+l*4;
                }/* end of loop l */
            }/*0x203040+l*4*/


            {/*0x203048+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkMcFifoFullDropCntr[l] =
                        0x203048+l*4;
                }/* end of loop l */
            }/*0x203048+l*4*/


            {/*0x203050+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkDescFifoFullDropCntr[l] =
                        0x203050+l*4;
                }/* end of loop l */
            }/*0x203050+l*4*/


            {/*0x203058+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.cntr.linkByteCountToBigDropCntr[l] =
                        0x203058+l*4;
                }/* end of loop l */
            }/*0x203058+l*4*/



        }/*end of unit cntr */


        {/*start of unit globalConfig */
            devMemInfoPtr->regsAddr.ingr.globalConfig.generalConfig = 0x00201000;


            {/*0x201004+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.globalConfig.linkConfig[l] =
                        0x201004+l*0x4;
                }/* end of loop l */
            }/*0x201004+l*0x4*/


            devMemInfoPtr->regsAddr.ingr.globalConfig.MCIDCalculation = 0x0020100c;


            {/*0x201010+w*4*/
                GT_U32    w;
                for(w = 0 ; w <= 3 ; w++) {
                    devMemInfoPtr->regsAddr.ingr.globalConfig.destinationDescType[w] =
                        0x201010+w*4;
                }/* end of loop w */
            }/*0x201010+w*4*/


            devMemInfoPtr->regsAddr.ingr.globalConfig.interruptCauseReg = 0x00201020;


            devMemInfoPtr->regsAddr.ingr.globalConfig.interruptMaskReg = 0x00201024;


            {/*0x201028+l*4*/
                GT_U32    l;
                for(l = 0 ; l <= 1 ; l++) {
                    devMemInfoPtr->regsAddr.ingr.globalConfig.descFIFOThreshold[l] =
                        0x201028+l*4;
                }/* end of loop l */
            }/*0x201028+l*4*/


            {/*0x201030+((l*2)+t)*4*/
                GT_U32    t,l;
                for(t = 0 ; t <= 1 ; t++) {
                    for(l = 0 ; l <= 1 ; l++) {
                        devMemInfoPtr->regsAddr.ingr.globalConfig.dataFIFOThreshold[t][l] =
                            0x201030+((l*2)+t)*4;
                    }/* end of loop l */
                }/* end of loop t */
            }/*0x201030+((l*2)+t)*4*/



        }/*end of unit globalConfig */


        {/*start of unit labelingConfig */
            devMemInfoPtr->regsAddr.ingr.labelingConfig.labelingGlobalConfig = 0x00204000;


            devMemInfoPtr->regsAddr.ingr.labelingConfig.basePerLink = 0x00204004;


            devMemInfoPtr->regsAddr.ingr.labelingConfig.UCPriorityMap = 0x00204008;


            devMemInfoPtr->regsAddr.ingr.labelingConfig.MCPriorityMap = 0x0020400c;


            {/*0x204010+w*4*/
                GT_U32    w;
                for(w = 0 ; w <= 11 ; w++) {
                    devMemInfoPtr->regsAddr.ingr.labelingConfig.ucQOS2PrioMap[w] =
                        0x204010+w*4;
                }/* end of loop w */
            }/*0x204010+w*4*/


            devMemInfoPtr->regsAddr.ingr.labelingConfig.ucQOS2PrioMap12 = 0x00204040;


            {/*0x204044+w*4*/
                GT_U32    w;
                for(w = 0 ; w <= 11 ; w++) {
                    devMemInfoPtr->regsAddr.ingr.labelingConfig.mcQOS2PrioMap[w] =
                        0x204044+w*4;
                }/* end of loop w */
            }/*0x204044+w*4*/


            devMemInfoPtr->regsAddr.ingr.labelingConfig.mcQOS2PrioMap12 = 0x00204074;


            devMemInfoPtr->regsAddr.ingr.labelingConfig.analyzerParameters = 0x00204078;


            devMemInfoPtr->regsAddr.ingr.labelingConfig.mcBase = 0x0020407c;


            devMemInfoPtr->regsAddr.ingr.labelingConfig.DXDPAssignment = 0x00204080;


            {/*0x204084+w*4*/
                GT_U32    w;
                for(w = 0 ; w <= 7 ; w++) {
                    devMemInfoPtr->regsAddr.ingr.labelingConfig.QOS2DpMap[w] =
                        0x204084+w*4;
                }/* end of loop w */
            }/*0x204084+w*4*/



        }/*end of unit labelingConfig */

    }/*end of unit ingr - ingress  */

    {/*start of unit egr - egress  */
        {/*start of unit EDQUnit */
            devMemInfoPtr->regsAddr.egr.EDQUnit.dequeueGlobalConfig = 0x00320000;


            {/*0x320004+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.egr.EDQUnit.linkLevel2SDWRRWeight[n] =
                        0x320004+n*4;
                }/* end of loop n */
            }/*0x320004+n*4*/


            {/*0x320010+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.egr.EDQUnit.linkSchedulerConfig[n] =
                        0x320010+n*4;
                }/* end of loop n */
            }/*0x320010+n*4*/


            {/*0x320018+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.egr.EDQUnit.linkSDWRRWeights[n] =
                        0x320018+n*4;
                }/* end of loop n */
            }/*0x320018+n*4*/



        }/*end of unit EDQUnit */


        {/*start of unit EFCUnit */
            devMemInfoPtr->regsAddr.egr.EFCUnit.flowCtrlGlobalConfig = 0x00304000;


            devMemInfoPtr->regsAddr.egr.EFCUnit.linkLevelFCThresholds = 0x00304004;


            devMemInfoPtr->regsAddr.egr.EFCUnit.linkLevelFCGlobalThreshold = 0x00304008;


            {/*0x304010+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.egr.EFCUnit.linkMCIBEThresholds[n] =
                        0x304010+n*4;
                }/* end of loop n */
            }/*0x304010+n*4*/


            {/*0x304018+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.egr.EFCUnit.linkMCIGuaranteedThresholds[n] =
                        0x304018+n*4;
                }/* end of loop n */
            }/*0x304018+n*4*/



        }/*end of unit EFCUnit */


        {/*start of unit FMUnit */
            devMemInfoPtr->regsAddr.egr.FMUnit.FMDXConfig = 0x00302000;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMTC2TCUcMapTable = 0x00302008;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMTC2TCMcMapTable = 0x0030200c;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMDXUcQosProfile = 0x00302010;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMDXMcQosProfile = 0x00302014;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMOutgoingMH = 0x00302018;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMExtOutgoingMH = 0x0030201c;


            {/*0x302020+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    devMemInfoPtr->regsAddr.egr.FMUnit.FMDXQoS2QoSUcMapTable[n] =
                        0x302020+n*4;
                }/* end of loop n */
            }/*0x302020+n*4*/


            {/*0x302030+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    devMemInfoPtr->regsAddr.egr.FMUnit.FMDXQoS2QoSMcMapTable[n] =
                        0x302030+n*4;
                }/* end of loop n */
            }/*0x302030+n*4*/


            devMemInfoPtr->regsAddr.egr.FMUnit.FMDXTargetDevID = 0x00302040;


            devMemInfoPtr->regsAddr.egr.FMUnit.FMHGLCellTCMapTable = 0x00302044;


            {/*0x302050+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    devMemInfoPtr->regsAddr.egr.FMUnit.FMDXSrcID2SrcIDMapTable[n] =
                        0x302050+n*4;
                }/* end of loop n */
            }/*0x302050+n*4*/



        }/*end of unit FMUnit */


        {/*start of unit globalConfig */
            devMemInfoPtr->regsAddr.egr.globalConfig.egrGlobalConfig = 0x00300000;


            devMemInfoPtr->regsAddr.egr.globalConfig.egrQoSConfig = 0x00300004;



        }/*end of unit globalConfig */


        {/*start of unit PDPUnit */
            devMemInfoPtr->regsAddr.egr.PDPUnit.queueSelectionGlobalConfig = 0x00310000;


            devMemInfoPtr->regsAddr.egr.PDPUnit.CMConstantContextID = 0x00310004;


            devMemInfoPtr->regsAddr.egr.PDPUnit.tailDropLinkThresholds = 0x00310020;


            devMemInfoPtr->regsAddr.egr.PDPUnit.tailDropPktThresholds = 0x00310024;


            devMemInfoPtr->regsAddr.egr.PDPUnit.tailDropTotalThreshold = 0x00310028;


            {/*0x310030+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.egr.PDPUnit.tailDropQueueThresholds[n] =
                        0x310030+n*4;
                }/* end of loop n */
            }/*0x310030+n*4*/


            {/*0x310038+m*4*/
                GT_U32    m;
                for(m = 0 ; m <= 1 ; m++) {
                    devMemInfoPtr->regsAddr.egr.PDPUnit.tailDropQueueThresholds1[m] =
                        0x310038+m*4;
                }/* end of loop m */
            }/*0x310038+m*4*/


            {/*0x310040+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 3 ; n++) {
                    devMemInfoPtr->regsAddr.egr.PDPUnit.qosProfileToPriorityMapTable[n] =
                        0x310040+n*4;
                }/* end of loop n */
            }/*0x310040+n*4*/



        }/*end of unit PDPUnit */


        {/*start of unit WRDMAUnit */
            {/*0x301040+n*4*/
                GT_U32    n;
                for(n = 0 ; n <= 7 ; n++) {
                    devMemInfoPtr->regsAddr.egr.WRDMAUnit.DXQosProfile2DPMapTable[n] =
                        0x301040+n*4;
                }/* end of loop n */
            }/*0x301040+n*4*/



        }/*end of unit WRDMAUnit */

    }/*end of unit egr - egress  */

    {/*start of unit statistics - statistics  */
        {/*start of unit copyOfSERDESConfigRegs */
            devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.analogTestAndTBGCtrlReg = 0x00460000;


            devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.analogReceiverTransmitCtrlReg = 0x00460004;


            devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.analogAllLaneCtrlReg0 = 0x00460008;


            devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.analogAllLaneCtrlReg1 = 0x0046000c;


            devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.VCOCalibrationCtrlReg = 0x00460010;


            devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.SERDESPowerAndResetCtrl = 0x00460014;


            {/*0x460020+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 5 ; l++) {
                    devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.analogLaneTransmitterCtrlReg[l] =
                        0x460020+l*0x4;
                }/* end of loop l */
            }/*0x460020+l*0x4*/


            {/*0x460040+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 5 ; l++) {
                    devMemInfoPtr->regsAddr.statistics.copyOfSERDESConfigRegs.laneVCOCalibrationCtrlReg[l] =
                        0x460040+l*0x4;
                }/* end of loop l */
            }/*0x460040+l*0x4*/



        }/*end of unit copyOfSERDESConfigRegs */


        {/*start of unit debug */
            devMemInfoPtr->regsAddr.statistics.debug.ingrDebugFifoStatus = 0x004e000c;


            devMemInfoPtr->regsAddr.statistics.debug.ingrDebugFifoData = 0x004e0010;


            devMemInfoPtr->regsAddr.statistics.debug.egrDebugFifoStatus = 0x004e0014;


            devMemInfoPtr->regsAddr.statistics.debug.egrDebugFifoData = 0x004e0018;



        }/*end of unit debug */


        {/*start of unit egrConfig */
            devMemInfoPtr->regsAddr.statistics.egrConfig.egrStatisticConfig = 0x00420000;


            devMemInfoPtr->regsAddr.statistics.egrConfig.egrMACParameters = 0x00420004;


            devMemInfoPtr->regsAddr.statistics.egrConfig.egrIncomingEventsCntr = 0x00420020;


            devMemInfoPtr->regsAddr.statistics.egrConfig.egrOutgoingEventsCntr = 0x00420024;


            devMemInfoPtr->regsAddr.statistics.egrConfig.egrDropMsgCntr = 0x0042002c;



        }/*end of unit egrConfig */


        {/*start of unit ingrConfig */
            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrStatisticConfig = 0x00400000;


            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrMACParameters = 0x00400004;


            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrByteCountCompensation = 0x0040000c;


            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrIncomingEventsCntr = 0x00400020;


            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrOutgoingEventsCntr = 0x00400024;


            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrParityErrorCntr = 0x00400028;


            devMemInfoPtr->regsAddr.statistics.ingrConfig.ingrDropMsgCntr = 0x0040002c;



        }/*end of unit ingrConfig */


        {/*start of unit TMMAC */
            devMemInfoPtr->regsAddr.statistics.TMMAC.globalCtrl = 0x00450000;


            devMemInfoPtr->regsAddr.statistics.TMMAC.transmitCellParameters = 0x00450004;


            devMemInfoPtr->regsAddr.statistics.TMMAC.CRCErrorsCntr = 0x00450010;



            {/*0x451000+4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.statistics.TMMAC.laneDropCellsCntr[n] =
                        0x451000+4*n;
                }/* end of loop n */
            }/*0x451000+4*n*/


            {/*0x451010+4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.statistics.TMMAC.laneInterruptCause[n] =
                        0x451010+4*n;
                }/* end of loop n */
            }/*0x451010+4*n*/


            {/*0x451020+4*n*/
                GT_U32    n;
                for(n = 0 ; n <= 1 ; n++) {
                    devMemInfoPtr->regsAddr.statistics.TMMAC.laneInterruptMask[n] =
                        0x451020+4*n;
                }/* end of loop n */
            }/*0x451020+4*n*/



        }/*end of unit TMMAC */

    }/*end of unit statistics - statistics  */

    {/*start of unit flowCtrl - Flow Control  */
        {/*0x500000+offset*0x4*/
            GT_U32    offset;
            for(offset = 0 ; offset <= 7 ; offset++) {
                devMemInfoPtr->regsAddr.flowCtrl.schedulerStatusffset[offset] =
                    0x500000+offset*0x4;
            }/* end of loop offset */
        }/*0x500000+offset*0x4*/


        {/*0x500020+offset*0x4*/
            GT_U32    offset;
            for(offset = 0 ; offset <= 7 ; offset++) {
                devMemInfoPtr->regsAddr.flowCtrl.schedulerDefaultffset[offset] =
                    0x500020+offset*0x4;
            }/* end of loop offset */
        }/*0x500020+offset*0x4*/


        {/*0x500040+offset*0x4*/
            GT_U32    offset;
            for(offset = 0 ; offset <= 7 ; offset++) {
                devMemInfoPtr->regsAddr.flowCtrl.schedulerEnffset[offset] =
                    0x500040+offset*0x4;
            }/* end of loop offset */
        }/*0x500040+offset*0x4*/


        devMemInfoPtr->regsAddr.flowCtrl.inBandFCConfigs = 0x00500060;


        devMemInfoPtr->regsAddr.flowCtrl.calendarConfigs = 0x00500064;


        {/*0x500070+l*0x4*/
            GT_U32    l;
            for(l = 0 ; l <= 1 ; l++) {
                devMemInfoPtr->regsAddr.flowCtrl.FCDropCntr[l] =
                    0x500070+l*0x4;
            }/* end of loop l */
        }/*0x500070+l*0x4*/

    }/*end of unit flowCtrl - Flow Control  */


    {/*start of unit misc - misc  */
        {/*start of unit debugConfig */
            devMemInfoPtr->regsAddr.misc.debugConfig.debugCtrlReg = 0x00600104;



        }/*end of unit debugConfig */


        {/*start of unit globalConfig */
            devMemInfoPtr->regsAddr.misc.globalConfig.sampledAtResetReg0 = 0x00600000;


            devMemInfoPtr->regsAddr.misc.globalConfig.sampledAtResetReg1 = 0x00600004;



        }/*end of unit globalConfig */


        {/*start of unit GPPRegs */
            {/*0x600400+g*4*/
                GT_U32    g;
                for(g = 0 ; g <= 1 ; g++) {
                    devMemInfoPtr->regsAddr.misc.GPPRegs.GPPReg[g] =
                        0x600400+g*4;
                }/* end of loop g */
            }/*0x600400+g*4*/



        }/*end of unit GPPRegs */


        {/*start of unit PLLConfigRegs */
            devMemInfoPtr->regsAddr.misc.PLLConfigRegs.PLLsConfig0Reg = 0x00600200;


            devMemInfoPtr->regsAddr.misc.PLLConfigRegs.PLLsConfig1Reg = 0x00600204;


            devMemInfoPtr->regsAddr.misc.PLLConfigRegs.PLLsConfig2Reg = 0x00600208;



        }/*end of unit PLLConfigRegs */


        {/*start of unit XSMI */
            devMemInfoPtr->regsAddr.misc.XSMI.XSMICtrlReg = 0x00609000;



        }/*end of unit XSMI */

    }/*end of unit misc - misc  */

    {/*start of unit hyperGlink0shared - hyperGlink0 (shared)   */
        {/*start of unit hyperGLinkPortInterrupt */
            devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkPortInterrupt.hyperGLinkMainInterruptCause = 0x00800000;


            devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkPortInterrupt.hyperGLinkMainInterruptMask = 0x00800004;



        }/*end of unit hyperGLinkPortInterrupt */


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkPingCellTx = 0x00800008;


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkPingCellRx = 0x0080000c;


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkMACConfig = 0x00800010;


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkLinkLevelFlowCtrlTxConfig = 0x00800014;


        {/*0x800018+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkLinkLevelFlowCtrlEnable[n] =
                    0x800018+n*4;
            }/* end of loop n */
        }/*0x800018+n*4*/


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkRxFlowCtrlCellsCntr = 0x00800020;


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkTxFlowCtrlCellsCntr = 0x00800024;


        devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkMACDroppedReceivedCellCntrs = 0x00800028;


        {/*0x80002c+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                devMemInfoPtr->regsAddr.hyperGlink0shared.hyperGLinkLinkLevelFlowCtrlStatus[n] =
                    0x80002c+n*4;
            }/* end of loop n */
        }/*0x80002c+n*4*/



    }/*end of unit hyperGlink0shared - hyperGlink0 (shared)   */

    {/*start of unit hyperGStack0shared - hyperG.Stack0 (shared)   */
        {/*start of unit hyperGStackPortsInterrupt */
            devMemInfoPtr->regsAddr.hyperGStack0shared.hyperGStackPortsInterrupt.HGSPortInterruptCause = 0x00900014;


            devMemInfoPtr->regsAddr.hyperGStack0shared.hyperGStackPortsInterrupt.HGSPortInterruptMask = 0x00900018;



        }/*end of unit hyperGStackPortsInterrupt */


        {/*start of unit hyperGStackPortsMACConfig */
            devMemInfoPtr->regsAddr.hyperGStack0shared.hyperGStackPortsMACConfig.portMACCtrlReg0 = 0x00900000;


            devMemInfoPtr->regsAddr.hyperGStack0shared.hyperGStackPortsMACConfig.portMACCtrlReg1 = 0x00900004;


            devMemInfoPtr->regsAddr.hyperGStack0shared.hyperGStackPortsMACConfig.portMACCtrlReg2 = 0x00900008;



        }/*end of unit hyperGStackPortsMACConfig */


        {/*start of unit hyperGStackPortsStatus */
            devMemInfoPtr->regsAddr.hyperGStack0shared.hyperGStackPortsStatus.portStatus = 0x0090000c;



        }/*end of unit hyperGStackPortsStatus */



    }/*end of unit hyperGStack0shared - hyperG.Stack0 (shared)   */

    {/*start of unit convertor0shared - convertor0 (shared)   */
        {/*start of unit SERDESConfigRegs */
            devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.analogTestAndTBGCtrlReg = 0x00a60000;


            devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.analogReceiverTransmitCtrlReg = 0x00a60004;


            devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.analogAllLaneCtrlReg0 = 0x00a60008;


            devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.analogAllLaneCtrlReg1 = 0x00a6000c;


            devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.VCOCalibrationCtrlReg = 0x00a60010;


            devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.SERDESPowerAndResetCtrl = 0x00a60014;


            {/*0xa60020+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 5 ; l++) {
                    devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.analogLaneTransmitterCtrlReg[l] =
                        0xa60020+l*0x4;
                }/* end of loop l */
            }/*0xa60020+l*0x4*/


            {/*0xa60040+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 5 ; l++) {
                    devMemInfoPtr->regsAddr.convertor0shared.SERDESConfigRegs.laneVCOCalibrationCtrlReg[l] =
                        0xa60040+l*0x4;
                }/* end of loop l */
            }/*0xa60040+l*0x4*/



        }/*end of unit SERDESConfigRegs */


        devMemInfoPtr->regsAddr.convertor0shared.convertorPortMACsAndMIBxReg = 0x00a00000;


        devMemInfoPtr->regsAddr.convertor0shared.convertorPortGeneralReg = 0x00a00004;


        devMemInfoPtr->regsAddr.convertor0shared.convertorMainInterruptCauseReg = 0x00a00008;


        devMemInfoPtr->regsAddr.convertor0shared.convertorMainInterruptMaskReg = 0x00a0000c;


        devMemInfoPtr->regsAddr.convertor0shared.sourceAddrMiddle = 0x00a00020;


        devMemInfoPtr->regsAddr.convertor0shared.sourceAddrHigh = 0x00a00024;


        devMemInfoPtr->regsAddr.convertor0shared.convertorSummaryInterruptCauseReg = 0x00a00030;


        devMemInfoPtr->regsAddr.convertor0shared.convertorSummaryInterruptMaskReg = 0x00a00034;


    }/*end of unit convertor0shared - convertor0 (shared)   */

    {/*start of unit genericXPCS0shared - generic XPCS0 (shared)   */
        {/*start of unit globalRegs */
            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.txPktsCntrLSB = 0x00b00030;


            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.txPktsCntrMSB = 0x00b00034;



            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.globalConfig0 = 0x00b00000;


            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.globalConfig1 = 0x00b00004;



            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.globalDeskewErrorCntr = 0x00b00020;



            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.metalFix = 0x00b00040;



            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.globalStatus = 0x00b00010;


            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.globalInterruptCause = 0x00b00014;


            devMemInfoPtr->regsAddr.genericXPCS0shared.globalRegs.globalInterruptMask = 0x00b00018;



        }/*end of unit globalRegs */


        {/*start of lanes units */
            address[0] = 0x00b00050;
            address[1] = 0x00b00094;
            address[2] = 0x00b000d8;
            address[3] = 0x00b0011c;
            address[4] = 0x00b00160;
            address[5] = 0x00b001a4;


            for(i=0; i<6; i++)
            {
                /* Lane Configuration */
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].laneConfig0 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].laneConfig1 = address[i];
                address[i] += 8;

                /* Lane status and interrupt */
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].laneStatus = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].laneInterruptCause = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].laneInterruptMask = address[i];
                address[i] += 4;

                /* Error counters */
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].symbolErrorCntr = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].disparityErrorCntr = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].PRBSErrorCntr = address[i];
                address[i] += 4;

                /* Lane<i> CJRPAT Rx */
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].rxPktsCntrLSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].rxPktsCntrMSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].rxBadPktsCntrLSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].rxBadPktsCntrMSB = address[i];
                address[i] += 4;

                /* Lane<i> Cyclic Data */
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].cyclicDataReg0 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].cyclicDataReg1 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].cyclicDataReg2 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS0shared.laneRegs[i].cyclicDataReg3 = address[i];
                address[i] += 4;
            }

        }/*end of lanes units */

    }/*end of unit genericXPCS0shared - generic XPCS0 (shared)   */

    {/*start of unit hyperGlink1shared - hyperGlink1 (shared)   */
        {/*start of unit hyperGLinkPortInterrupt */
            devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkPortInterrupt.hyperGLinkMainInterruptCause = 0x00c00000;


            devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkPortInterrupt.hyperGLinkMainInterruptMask = 0x00c00004;



        }/*end of unit hyperGLinkPortInterrupt */


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkPingCellTx = 0x00c00008;


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkPingCellRx = 0x00c0000c;


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkMACConfig = 0x00c00010;


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkLinkLevelFlowCtrlTxConfig = 0x00c00014;


        {/*0xc00018+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkLinkLevelFlowCtrlEnable[n] =
                    0xc00018+n*4;
            }/* end of loop n */
        }/*0xc00018+n*4*/


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkRxFlowCtrlCellsCntr = 0x00c00020;


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkTxFlowCtrlCellsCntr = 0x00c00024;


        devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkMACDroppedReceivedCellCntrs = 0x00c00028;


        {/*0xc0002c+n*4*/
            GT_U32    n;
            for(n = 0 ; n <= 1 ; n++) {
                devMemInfoPtr->regsAddr.hyperGlink1shared.hyperGLinkLinkLevelFlowCtrlStatus[n] =
                    0xc0002c+n*4;
            }/* end of loop n */
        }/*0xc0002c+n*4*/


    }/*end of unit hyperGlink1shared - hyperGlink1 (shared)   */

    {/*start of unit hyperGStack1shared - hyperG.Stack1 (shared)   */
        {/*start of unit hyperGStackPortsInterrupt */
            devMemInfoPtr->regsAddr.hyperGStack1shared.hyperGStackPortsInterrupt.HGSPortInterruptCause = 0x00d00014;


            devMemInfoPtr->regsAddr.hyperGStack1shared.hyperGStackPortsInterrupt.HGSPortInterruptMask = 0x00d00018;



        }/*end of unit hyperGStackPortsInterrupt */


        {/*start of unit hyperGStackPortsMACConfig */
            devMemInfoPtr->regsAddr.hyperGStack1shared.hyperGStackPortsMACConfig.portMACCtrlReg0 = 0x00d00000;


            devMemInfoPtr->regsAddr.hyperGStack1shared.hyperGStackPortsMACConfig.portMACCtrlReg1 = 0x00d00004;


            devMemInfoPtr->regsAddr.hyperGStack1shared.hyperGStackPortsMACConfig.portMACCtrlReg2 = 0x00d00008;



        }/*end of unit hyperGStackPortsMACConfig */


        {/*start of unit hyperGStackPortsStatus */
            devMemInfoPtr->regsAddr.hyperGStack1shared.hyperGStackPortsStatus.portStatus = 0x00d0000c;



        }/*end of unit hyperGStackPortsStatus */


    }/*end of unit hyperGStack1shared - hyperG.Stack1 (shared)   */

    {/*start of unit convertor1shared - convertor1 (shared)   */
        {/*start of unit SERDESConfigRegs */
            devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.analogTestAndTBGCtrlReg = 0x00e60000;


            devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.analogReceiverTransmitCtrlReg = 0x00e60004;


            devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.analogAllLaneCtrlReg0 = 0x00e60008;


            devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.analogAllLaneCtrlReg1 = 0x00e6000c;


            devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.VCOCalibrationCtrlReg = 0x00e60010;


            devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.SERDESPowerAndResetCtrl = 0x00e60014;


            {/*0xe60020+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 5 ; l++) {
                    devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.analogLaneTransmitterCtrlReg[l] =
                        0xe60020+l*0x4;
                }/* end of loop l */
            }/*0xe60020+l*0x4*/


            {/*0xe60040+l*0x4*/
                GT_U32    l;
                for(l = 0 ; l <= 5 ; l++) {
                    devMemInfoPtr->regsAddr.convertor1shared.SERDESConfigRegs.laneVCOCalibrationCtrlReg[l] =
                        0xe60040+l*0x4;
                }/* end of loop l */
            }/*0xe60040+l*0x4*/



        }/*end of unit SERDESConfigRegs */


        devMemInfoPtr->regsAddr.convertor1shared.convertorPortMACsAndMIBxReg = 0x00e00000;


        devMemInfoPtr->regsAddr.convertor1shared.convertorPortGeneralReg = 0x00e00004;


        devMemInfoPtr->regsAddr.convertor1shared.convertorMainInterruptCauseReg = 0x00e00008;


        devMemInfoPtr->regsAddr.convertor1shared.convertorMainInterruptMaskReg = 0x00e0000c;


        devMemInfoPtr->regsAddr.convertor1shared.sourceAddrMiddle = 0x00e00020;


        devMemInfoPtr->regsAddr.convertor1shared.sourceAddrHigh = 0x00e00024;


        devMemInfoPtr->regsAddr.convertor1shared.convertorSummaryInterruptCauseReg = 0x00e00030;


        devMemInfoPtr->regsAddr.convertor1shared.convertorSummaryInterruptMaskReg = 0x00e00034;


    }/*end of unit Convertor 1 shared - convertor1 (shared)   */

    {/*start of unit genericXPCS1shared - generic XPCS1 (shared)   */
        {/*start of unit globalRegs */
            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.txPktsCntrLSB = 0x00f00030;


            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.txPktsCntrMSB = 0x00f00034;



            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.globalConfig0 = 0x00f00000;


            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.globalConfig1 = 0x00f00004;



            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.globalDeskewErrorCntr = 0x00f00020;



            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.metalFix = 0x00f00040;



            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.globalStatus = 0x00f00010;


            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.globalInterruptCause = 0x00f00014;


            devMemInfoPtr->regsAddr.genericXPCS1shared.globalRegs.globalInterruptMask = 0x00f00018;



        }/*end of unit globalRegs */

        {/*start of lanes units */
            address[0] = 0x00f00050;
            address[1] = 0x00f00094;
            address[2] = 0x00f000d8;
            address[3] = 0x00f0011c;
            address[4] = 0x00f00160;
            address[5] = 0x00f001a4;

            for(i=0; i<6; i++)
            {
                /* Lane Configuration */
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].laneConfig0 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].laneConfig1 = address[i];
                address[i] += 8;

                /* Lane status and interrupt */
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].laneStatus = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].laneInterruptCause = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].laneInterruptMask = address[i];
                address[i] += 4;

                /* Error counters */
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].symbolErrorCntr = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].disparityErrorCntr = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].PRBSErrorCntr = address[i];
                address[i] += 4;

                /* Lane<i> CJRPAT Rx */
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].rxPktsCntrLSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].rxPktsCntrMSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].rxBadPktsCntrLSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].rxBadPktsCntrMSB = address[i];
                address[i] += 4;

                /* Lane<i> Cyclic Data */
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].cyclicDataReg0 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].cyclicDataReg1 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].cyclicDataReg2 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.genericXPCS1shared.laneRegs[i].cyclicDataReg3 = address[i];
            }
        }/*end of lanes units */


    }/*end of unit genericXPCS1shared - generic XPCS1 (shared)   */

    {/*start of unit ftdll_Calibshared - ftdll_Calib (shared)   */
        {/*start of unit calibrationUnit */
            devMemInfoPtr->regsAddr.ftdll_Calibshared.calibrationUnit.DDRPadsConfig0 = 0x01000100;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.calibrationUnit.DDRPadsConfig1 = 0x01000104;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.calibrationUnit.calibrationGlobalCtrlReg = 0x01000108;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.calibrationUnit.calibrationStatusReg = 0x0100010c;


            {/*0x1000110+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 9 ; n++) {
                    devMemInfoPtr->regsAddr.ftdll_Calibshared.calibrationUnit.grpPadCalibrationCtrlReg[n] =
                        0x1000110+n*0x4;
                }/* end of loop n */
            }/*0x1000110+n*0x4*/



        }/*end of unit calibrationUnit */


        {/*start of unit ftdllUnitRegsDocOnly */
            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLCtrlReg = 0x01000200;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLFilterReg = 0x01000204;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramAddrReg = 0x01000208;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramWriteData0Reg = 0x0100020c;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramWriteData1Reg = 0x01000210;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramWriteData2Reg = 0x01000214;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLDfvReg = 0x01000218;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramReadData0Reg = 0x0100021c;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramReadData1Reg = 0x01000220;


            devMemInfoPtr->regsAddr.ftdll_Calibshared.ftdllUnitRegsDocOnly.FTDLLSramReadData2Reg = 0x01000224;



        }/*end of unit ftdllUnitRegsDocOnly */

    }/*end of unit ftdll_Calibshared - ftdll_Calib (shared)   */

    {/*start of unit ftdll_Calib_1shared - ftdll_Calib 1(shared)   */
        {/*start of unit calibrationUnit */
            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.calibrationUnit.DDRPadsConfig0 = 0x01100100;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.calibrationUnit.DDRPadsConfig1 = 0x01100104;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.calibrationUnit.calibrationGlobalCtrlReg = 0x01100108;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.calibrationUnit.calibrationStatusReg = 0x0110010c;


            {/*0x1000110+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 9 ; n++) {
                    devMemInfoPtr->regsAddr.ftdll_Calib_1shared.calibrationUnit.grpPadCalibrationCtrlReg[n] =
                        0x1100110+n*0x4;
                }/* end of loop n */
            }/*0x1000110+n*0x4*/



        }/*end of unit calibrationUnit */


        {/*start of unit ftdllUnitRegsDocOnly */
            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLCtrlReg = 0x01100200;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLFilterReg = 0x01100204;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramAddrReg = 0x01100208;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramWriteData0Reg = 0x0110020c;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramWriteData1Reg = 0x01100210;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramWriteData2Reg = 0x01100214;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLDfvReg = 0x01100218;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramReadData0Reg = 0x0110021c;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramReadData1Reg = 0x01100220;


            devMemInfoPtr->regsAddr.ftdll_Calib_1shared.ftdllUnitRegsDocOnly.FTDLLSramReadData2Reg = 0x01100224;



        }/*end of unit ftdllUnitRegsDocOnly */

    }/*end of unit ftdll_Calibshared - ftdll_Calib (shared)   */

    {/*start of unit ftdll_Calib_2shared - ftdll_Calib 2(shared)   */
        {/*start of unit calibrationUnit */
            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.calibrationUnit.DDRPadsConfig0 = 0x01200100;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.calibrationUnit.DDRPadsConfig1 = 0x01200104;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.calibrationUnit.calibrationGlobalCtrlReg = 0x01200108;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.calibrationUnit.calibrationStatusReg = 0x0120010c;


            {/*0x1000110+n*0x4*/
                GT_U32    n;
                for(n = 0 ; n <= 9 ; n++) {
                    devMemInfoPtr->regsAddr.ftdll_Calib_2shared.calibrationUnit.grpPadCalibrationCtrlReg[n] =
                        0x1200110+n*0x4;
                }/* end of loop n */
            }/*0x1000110+n*0x4*/



        }/*end of unit calibrationUnit */


        {/*start of unit ftdllUnitRegsDocOnly */
            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLCtrlReg = 0x01200200;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLFilterReg = 0x01200204;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramAddrReg = 0x01200208;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramWriteData0Reg = 0x0120020c;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramWriteData1Reg = 0x01200210;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramWriteData2Reg = 0x01200214;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLDfvReg = 0x01200218;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramReadData0Reg = 0x0120021c;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramReadData1Reg = 0x01200220;


            devMemInfoPtr->regsAddr.ftdll_Calib_2shared.ftdllUnitRegsDocOnly.FTDLLSramReadData2Reg = 0x01200224;



        }/*end of unit ftdllUnitRegsDocOnly */

    }/*end of unit ftdll_Calib_2shared - ftdll_Calib 2(shared)   */

    {/*start of unit BM - BM  */
        {/*start of unit bufferManagementInterrupt */
            devMemInfoPtr->regsAddr.BM.bufferManagementInterrupt.bufferManagementInterruptCauseReg0 = 0x01300040;


            devMemInfoPtr->regsAddr.BM.bufferManagementInterrupt.bufferManagementInterruptMaskReg0 = 0x01300044;



        }/*end of unit bufferManagementInterrupt */


        {/*start of unit buffersManagementAgingConfig */
            devMemInfoPtr->regsAddr.BM.buffersManagementAgingConfig.bufferManagementAgingConfig = 0x0130000c;



        }/*end of unit buffersManagementAgingConfig */


        {/*start of unit buffersManagementGlobalConfig */
            devMemInfoPtr->regsAddr.BM.buffersManagementGlobalConfig.bufferManagementGlobalBuffersLimitsConfig = 0x01300000;


            devMemInfoPtr->regsAddr.BM.buffersManagementGlobalConfig.bufferManagementSharedBuffersConfig = 0x01300014;



        }/*end of unit buffersManagementGlobalConfig */

    }/*end of unit BM - BM  */

    {/*start of unit statisticsPCSshared - statistics PCS (shared)   */
        {/*start of unit globalRegs */
            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.txPktsCntrLSB = 0x01400030;


            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.txPktsCntrMSB = 0x01400034;



            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.globalConfig0 = 0x01400000;


            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.globalConfig1 = 0x01400004;



            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.globalDeskewErrorCntr = 0x01400020;



            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.metalFix = 0x01400040;



            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.globalStatus = 0x01400010;


            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.globalInterruptCause = 0x01400014;


            devMemInfoPtr->regsAddr.statisticsPCSshared.globalRegs.globalInterruptMask = 0x01400018;



        }/*end of unit globalRegs */

        {/*start of lanes units */
            address[0] = 0x01400050;
            address[1] = 0x01400094;
            address[2] = 0x014000d8;
            address[3] = 0x0140011c;
            address[4] = 0x01400160;
            address[5] = 0x014001a4;

            for(i=0; i<6; i++)
            {
                /* Lane Configuration */
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].laneConfig0 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].laneConfig1 = address[i];
                address[i] += 8;

                /* Lane status and interrupt */
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].laneStatus = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].laneInterruptCause = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].laneInterruptMask = address[i];
                address[i] += 4;

                /* Error counters */
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].symbolErrorCntr = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].disparityErrorCntr = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].PRBSErrorCntr = address[i];
                address[i] += 4;

                /* Lane<i> CJRPAT Rx */
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].rxPktsCntrLSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].rxPktsCntrMSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].rxBadPktsCntrLSB = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].rxBadPktsCntrMSB = address[i];
                address[i] += 4;

                /* Lane<i> Cyclic Data */
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].cyclicDataReg0 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].cyclicDataReg1 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].cyclicDataReg2 = address[i];
                address[i] += 4;
                devMemInfoPtr->regsAddr.statisticsPCSshared.laneRegs[i].cyclicDataReg3 = address[i];
            }
        }/*end of lanes units */

    }/*end of unit statisticsPCSshared - statistics PCS (shared)   */

    {/*start of unit ingr - ingress  */
       {/* start new table UC_Config_Table */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.UC_Config_Table.tableName = "UC_Config_Table";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.UC_Config_Table.baseAddr = 0x00205000;
            devMemInfoPtr->tblsAddr.ingr.UC_Config_Table.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.UC_Config_Table.entrySize = 0x00000022;
            devMemInfoPtr->tblsAddr.ingr.UC_Config_Table.lineAddrAlign = 0x00000002;
            devMemInfoPtr->tblsAddr.ingr.UC_Config_Table.numOfEntries = 0x00000080;
/*            MEM_PTR_ALLOC(ingr.UC_Config_Table);*/
        }/*end of table UC_Config_Table */


        {/* start new table CPU_Config_Table */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.CPU_Config_Table.tableName = "CPU_Config_Table";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.CPU_Config_Table.baseAddr = 0x00206000;
            devMemInfoPtr->tblsAddr.ingr.CPU_Config_Table.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.CPU_Config_Table.entrySize = 0x00000010;
            devMemInfoPtr->tblsAddr.ingr.CPU_Config_Table.lineAddrAlign = 0x00000001;
            devMemInfoPtr->tblsAddr.ingr.CPU_Config_Table.numOfEntries = 0x00000100;
            /*MEM_PTR_ALLOC(ingr.CPU_Config_Table);*/
        }/*end of table CPU_Config_Table */


        {/* start new table UC_Link_0_packet_Memory_High */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_High.tableName = "UC_Link_0_packet_Memory_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_High.baseAddr = 0x00210000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_High.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_High.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_High.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_High.numOfEntries = 0x00000190;
        /*    MEM_PTR_ALLOC(ingr.UC_Link_0_packet_Memory_High);*/
        }/*end of table UC_Link_0_packet_Memory_High */


        {/* start new table UC_Link_0_packet_Memory_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_Low.tableName = "UC_Link_0_packet_Memory_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_Low.baseAddr = 0x00214000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_Low.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_Low.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_Low.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_0_packet_Memory_Low.numOfEntries = 0x00000190;
       /*     MEM_PTR_ALLOC(ingr.UC_Link_0_packet_Memory_Low);*/
        }/*end of table UC_Link_0_packet_Memory_Low */


        {/* start new table MC_Link_0_packet_Memory_High */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.MC_Link_0_packet_Memory_High.tableName = "MC_Link_0_packet_Memory_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.MC_Link_0_packet_Memory_High.baseAddr = 0x00218000;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_0_packet_Memory_High.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_0_packet_Memory_High.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_0_packet_Memory_High.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_0_packet_Memory_High.numOfEntries = 0x00000190;
      /*      MEM_PTR_ALLOC(ingr.MC_Link_0_packet_Memory_High);*/
        }/*end of table MC_Link_0_packet_Memory_High */


        {/* start new table MCLink_0_packet_Memory_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.MCLink_0_packet_Memory_Low.tableName = "MCLink_0_packet_Memory_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.MCLink_0_packet_Memory_Low.baseAddr = 0x0021c000;
            devMemInfoPtr->tblsAddr.ingr.MCLink_0_packet_Memory_Low.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.MCLink_0_packet_Memory_Low.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.MCLink_0_packet_Memory_Low.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.MCLink_0_packet_Memory_Low.numOfEntries = 0x00000190;
      /*      MEM_PTR_ALLOC(ingr.MCLink_0_packet_Memory_Low);*/
        }/*end of table MCLink_0_packet_Memory_Low */


        {/* start new table UC_Link_1_packet_Memory_High */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_High.tableName = "UC_Link_1_packet_Memory_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_High.baseAddr = 0x00220000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_High.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_High.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_High.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_High.numOfEntries = 0x00000190;
       /*     MEM_PTR_ALLOC(ingr.UC_Link_1_packet_Memory_High);*/
        }/*end of table UC_Link_1_packet_Memory_High */


        {/* start new table UC_Link_1_packet_Memory_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_Low.tableName = "UC_Link_1_packet_Memory_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_Low.baseAddr = 0x00224000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_Low.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_Low.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_Low.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.UC_Link_1_packet_Memory_Low.numOfEntries = 0x00000190;
      /*      MEM_PTR_ALLOC(ingr.UC_Link_1_packet_Memory_Low);*/
        }/*end of table UC_Link_1_packet_Memory_Low */


        {/* start new table MC_Link_1_packet_Memory_High */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_High.tableName = "MC_Link_1_packet_Memory_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_High.baseAddr = 0x00228000;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_High.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_High.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_High.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_High.numOfEntries = 0x00000190;
      /*      MEM_PTR_ALLOC(ingr.MC_Link_1_packet_Memory_High);*/
        }/*end of table MC_Link_1_packet_Memory_High */


        {/* start new table MC_Link_1_packet_Memory_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_Low.tableName = "MC_Link_1_packet_Memory_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_Low.baseAddr = 0x0022c000;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_Low.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_Low.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_Low.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.MC_Link_1_packet_Memory_Low.numOfEntries = 0x00000190;
       /*     MEM_PTR_ALLOC(ingr.MC_Link_1_packet_Memory_Low);*/
        }/*end of table MC_Link_1_packet_Memory_Low */


        {/* start new table link_0_Desc_Memory_High */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_High.tableName = "link_0_Desc_Memory_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_High.baseAddr = 0x00230000;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_High.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_High.entrySize = 0x000000dc;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_High.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_High.numOfEntries = 0x000000c8;
        /*    MEM_PTR_ALLOC(ingr.link_0_Desc_Memory_High);*/
        }/*end of table link_0_Desc_Memory_High */


        {/* start new table link_0_Desc_Memory_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_Low.tableName = "link_0_Desc_Memory_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_Low.baseAddr = 0x00232000;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_Low.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_Low.entrySize = 0x000000dc;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_Low.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.link_0_Desc_Memory_Low.numOfEntries = 0x000000c8;
       /*     MEM_PTR_ALLOC(ingr.link_0_Desc_Memory_Low);*/
        }/*end of table link_0_Desc_Memory_Low */


        {/* start new table link_1_Desc_Memory_High */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_High.tableName = "link_1_Desc_Memory_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_High.baseAddr = 0x00234000;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_High.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_High.entrySize = 0x000000dc;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_High.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_High.numOfEntries = 0x000000c8;
      /*      MEM_PTR_ALLOC(ingr.link_1_Desc_Memory_High);*/
        }/*end of table link_1_Desc_Memory_High */


        {/* start new table link_1_Desc_Memory_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_Low.tableName = "link_1_Desc_Memory_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_Low.baseAddr = 0x00236000;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_Low.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_Low.entrySize = 0x000000dc;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_Low.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.link_1_Desc_Memory_Low.numOfEntries = 0x000000c8;
    /*         MEM_PTR_ALLOC(ingr.link_1_Desc_Memory_Low);*/
        }/*end of table link_1_Desc_Memory_Low */


        {/* start new table data_Delay_memory */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.data_Delay_memory.tableName = "data_Delay_memory";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.data_Delay_memory.baseAddr = 0x00238000;
            devMemInfoPtr->tblsAddr.ingr.data_Delay_memory.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.data_Delay_memory.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.data_Delay_memory.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.data_Delay_memory.numOfEntries = 0x00000028;
     /*       MEM_PTR_ALLOC(ingr.data_Delay_memory);*/
        }/*end of table data_Delay_memory */


        {/* start new table desc_Delay_memory */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ingr.desc_Delay_memory.tableName = "desc_Delay_memory";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ingr.desc_Delay_memory.baseAddr = 0x00239000;
            devMemInfoPtr->tblsAddr.ingr.desc_Delay_memory.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ingr.desc_Delay_memory.entrySize = 0x00000100;
            devMemInfoPtr->tblsAddr.ingr.desc_Delay_memory.lineAddrAlign = 0x00000008;
            devMemInfoPtr->tblsAddr.ingr.desc_Delay_memory.numOfEntries = 0x00000028;
       /*     MEM_PTR_ALLOC(ingr.desc_Delay_memory);*/
        }/*end of table desc_Delay_memory */

    }/*end of unit ingr - ingress  */

    {/*start of unit egr - egress  */
        {/*start of unit EDQUnit */
            {/* start new table PD_mem */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem.tableName = "PD_mem";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem.baseAddr = 0x00324000;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem.entrySize = 0x00000020;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem.lineAddrAlign = 0x00000001;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem.numOfEntries = 0x000005ff;
          /*      MEM_PTR_ALLOC(egr.EDQUnit.PD_mem);*/
            }/*end of table PD_mem */


            {/* start new table LL0_mem */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL0_mem.tableName = "LL0_mem";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL0_mem.baseAddr = 0x00328000;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL0_mem.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL0_mem.entrySize = 0x0000000c;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL0_mem.lineAddrAlign = 0x00000001;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL0_mem.numOfEntries = 0x00000600;
          /*      MEM_PTR_ALLOC(egr.EDQUnit.LL0_mem);*/
            }/*end of table LL0_mem */


            {/* start new table LL1_mem */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL1_mem.tableName = "LL1_mem";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL1_mem.baseAddr = 0x0032c000;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL1_mem.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL1_mem.entrySize = 0x0000000c;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL1_mem.lineAddrAlign = 0x00000001;
                devMemInfoPtr->tblsAddr.egr.EDQUnit.LL1_mem.numOfEntries = 0x00000600;
          /*      MEM_PTR_ALLOC(egr.EDQUnit.LL1_mem);*/
            }/*end of table LL1_mem */



        }/*end of unit EDQUnit */


        {/*start of unit PDPUnit */
            {/* start new table statistic_Report_Masking_table */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.PDPUnit.statistic_Report_Masking_table.tableName = "statistic_Report_Masking_table";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.PDPUnit.statistic_Report_Masking_table.baseAddr = 0x00311000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.statistic_Report_Masking_table.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.statistic_Report_Masking_table.entrySize = 0x0000000f;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.statistic_Report_Masking_table.lineAddrAlign = 0x00000001;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.statistic_Report_Masking_table.numOfEntries = 0x00000080;
            /*    MEM_PTR_ALLOC(egr.PDPUnit.statistic_Report_Masking_table);*/
            }/*end of table statistic_Report_Masking_table */


            {/* start new table MCDP_table */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.PDPUnit.MCDP_table.tableName = "MCDP_table";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.PDPUnit.MCDP_table.baseAddr = 0x00312000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.MCDP_table.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.MCDP_table.entrySize = 0x00000044;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.MCDP_table.lineAddrAlign = 0x00000004;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.MCDP_table.numOfEntries = 0x00000106;
           /*     MEM_PTR_ALLOC(egr.PDPUnit.MCDP_table);*/
            }/*end of table MCDP_table */


            {/* start new table flowID_2_link_prio_table */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_link_prio_table.tableName = "flowID_2_link_prio_table";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_link_prio_table.baseAddr = 0x00314000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_link_prio_table.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_link_prio_table.entrySize = 0x00000021;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_link_prio_table.lineAddrAlign = 0x00000002;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_link_prio_table.numOfEntries = 0x00000400;
            /*    MEM_PTR_ALLOC(egr.PDPUnit.flowID_2_link_prio_table);*/
            }/*end of table flowID_2_link_prio_table */


            {/* start new table flowID_2_contextID_table */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_contextID_table.tableName = "flowID_2_contextID_table";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_contextID_table.baseAddr = 0x00318000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_contextID_table.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_contextID_table.entrySize = 0x00000069;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_contextID_table.lineAddrAlign = 0x00000004;
                devMemInfoPtr->tblsAddr.egr.PDPUnit.flowID_2_contextID_table.numOfEntries = 0x00000800;
           /*     MEM_PTR_ALLOC(egr.PDPUnit.flowID_2_contextID_table);*/
            }/*end of table flowID_2_contextID_table */



        }/*end of unit PDPUnit */


        {/* start new table pkts_memory_bank0 */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank0.tableName = "pkts_memory_bank0";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank0.baseAddr = 0x00340000;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank0.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank0.entrySize = 0x00000200;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank0.lineAddrAlign = 0x00000010;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank0.numOfEntries = 0x00000c00;
        /*    MEM_PTR_ALLOC(egr.pkts_memory_bank0);*/
        }/*end of table pkts_memory_bank0 */


        {/* start new table pkts_memory_bank1 */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank1.tableName = "pkts_memory_bank1";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank1.baseAddr = 0x00380000;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank1.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank1.entrySize = 0x00000200;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank1.lineAddrAlign = 0x00000010;
            devMemInfoPtr->tblsAddr.egr.pkts_memory_bank1.numOfEntries = 0x00000c00;
        /*    MEM_PTR_ALLOC(egr.pkts_memory_bank1);*/
        }/*end of table pkts_memory_bank1 */

    }/*end of unit egr - egress  */

    {/*start of unit statistics - statistics  */
        {/*start of unit egrConfig */
            {/* start new table egr_Msg_FIFO_High */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High.tableName = "egr_Msg_FIFO_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High.baseAddr = 0x00424000;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High.entrySize = 0x0000003a;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High.lineAddrAlign = 0x00000002;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High.numOfEntries = 0x00000400;
          /*      MEM_PTR_ALLOC(statistics.egrConfig.egr_Msg_FIFO_High);*/
            }/*end of table egr_Msg_FIFO_High */


            {/* start new table egr_Msg_FIFO_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_Low.tableName = "egr_Msg_FIFO_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_Low.baseAddr = 0x00426000;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_Low.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_Low.entrySize = 0x0000003a;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_Low.lineAddrAlign = 0x00000002;
                devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_Low.numOfEntries = 0x00000400;
           /*     MEM_PTR_ALLOC(statistics.egrConfig.egr_Msg_FIFO_Low);*/
            }/*end of table egr_Msg_FIFO_Low */



        }/*end of unit egrConfig */


        {/*start of unit ingrConfig */
            {/* start new table ingr_Msg_FIFO_High */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_High.tableName = "ingr_Msg_FIFO_High";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_High.baseAddr = 0x00408000;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_High.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_High.entrySize = 0x0000003a;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_High.lineAddrAlign = 0x00000002;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_High.numOfEntries = 0x00000800;
           /*     MEM_PTR_ALLOC(statistics.ingrConfig.ingr_Msg_FIFO_High);*/
            }/*end of table ingr_Msg_FIFO_High */


            {/* start new table ingr_Msg_FIFO_Low */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_Low.tableName = "ingr_Msg_FIFO_Low";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_Low.baseAddr = 0x0040c000;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_Low.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_Low.entrySize = 0x0000003a;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_Low.lineAddrAlign = 0x00000002;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Msg_FIFO_Low.numOfEntries = 0x00000800;
           /*     MEM_PTR_ALLOC(statistics.ingrConfig.ingr_Msg_FIFO_Low);*/
            }/*end of table ingr_Msg_FIFO_Low */


            {/* start new table ingr_Chunk_Cntrs */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Chunk_Cntrs.tableName = "ingr_Chunk_Cntrs";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Chunk_Cntrs.baseAddr = 0x00410000;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Chunk_Cntrs.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Chunk_Cntrs.entrySize = 0x00000006;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Chunk_Cntrs.lineAddrAlign = 0x00000001;
                devMemInfoPtr->tblsAddr.statistics.ingrConfig.ingr_Chunk_Cntrs.numOfEntries = 0x00003000;
          /*      MEM_PTR_ALLOC(statistics.ingrConfig.ingr_Chunk_Cntrs);*/
            }/*end of table ingr_Chunk_Cntrs */



        }/*end of unit ingrConfig */



    }/*end of unit statistics - statistics  */

    {/*start of unit misc - misc  */
        {/*start of unit XSMI */
            {/* start new table XSMI_Addr */
#ifdef DEBUG_REG_AND_TBL_NAME
                devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr.tableName = "XSMI_Addr";
#endif /*DEBUG_REG_AND_TBL_NAME*/
                devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr.baseAddr = 0x00609100;
                devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr.memType = 0x00000000;
                devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr.entrySize = 0x00000020;
                devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr.lineAddrAlign = 0x00000001;
                devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr.numOfEntries = 0x00000008;
         /*       MEM_PTR_ALLOC(misc.XSMI.XSMI_Addr);*/
            }/*end of table XSMI_Addr */



        }/*end of unit XSMI */



    }/*end of unit misc - misc  */

    {/*start of unit convertor0shared - convertor0 (shared)   */
        {/* start new table hyperG_Stack_Ports_MAC_MIB_Cntrs */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs.tableName = "hyperG_Stack_Ports_MAC_MIB_Cntrs";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs.baseAddr = 0x00a80000;
            devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs.entrySize = 0x00000020;
            devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs.lineAddrAlign = 0x00000001;
            devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs.numOfEntries = 0x00000040;
        /*    MEM_PTR_ALLOC(convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs);*/
        }/*end of table hyperG_Stack_Ports_MAC_MIB_Cntrs */



    }/*end of unit convertor0shared - convertor0 (shared)   */

    {/*start of unit ftdll_Calibshared - ftdll_Calib (shared)   */
        {/* start new table FTDLL_UNIT */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT.tableName = "FTDLL_UNIT";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT.baseAddr = 0x01000200;
            devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT.entrySize = 0x00000020;
            devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT.lineAddrAlign = 0x00000001;
            devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT.numOfEntries = 0x00000010;
        /*    MEM_PTR_ALLOC(ftdll_Calibshared.FTDLL_UNIT);*/
        }/*end of table FTDLL_UNIT */



    }/*end of unit ftdll_Calibshared - ftdll_Calib (shared)   */

    {/*start of unit ftdll_Calib_1shared - ftdll_Calib_1 (shared)   */
        {/* start new table FTDLL_UNIT */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT.tableName = "FTDLL_UNIT";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT.baseAddr = 0x01100200;
            devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT.entrySize = 0x00000020;
            devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT.lineAddrAlign = 0x00000001;
            devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT.numOfEntries = 0x00000010;
       /*     MEM_PTR_ALLOC(ftdll_Calib_1shared.FTDLL_UNIT);*/
        }/*end of table FTDLL_UNIT */



    }/*end of unit ftdll_Calib_1shared - ftdll_Calib_1 (shared)   */

    {/*start of unit ftdll_Calib_2shared - ftdll_Calib_2 (shared)   */
        {/* start new table FTDLL_UNIT */
#ifdef DEBUG_REG_AND_TBL_NAME
            devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT.tableName = "FTDLL_UNIT";
#endif /*DEBUG_REG_AND_TBL_NAME*/
            devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT.baseAddr = 0x01200200;
            devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT.memType = 0x00000000;
            devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT.entrySize = 0x00000020;
            devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT.lineAddrAlign = 0x00000001;
            devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT.numOfEntries = 0x00000010;
     /*       MEM_PTR_ALLOC(ftdll_Calib_2shared.FTDLL_UNIT);*/
        }/*end of table FTDLL_UNIT */



    }/*end of unit ftdll_Calib_2shared - ftdll_Calib_2 (shared)   */

    smemFx950TableMemInit(devMemInfoPtr);

}

/*******************************************************************************
* smemFx950TableMemInit
*
* DESCRIPTION:
*       Allocate and init table memory.
*
* INPUTS:
*       devMemInfoPtr   - pointer to device object memory.
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
void smemFx950TableMemInit
(
    IN SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr

)
{
    GT_U32                      numOfTbls;
    GT_U32                      i;
    SMEM_FX950_TABLE_INFO_STC*  tblPtr;

    numOfTbls = sizeof(devMemInfoPtr->tblsAddr)/sizeof(SMEM_FX950_TABLE_INFO_STC);

    for(i=0, tblPtr = &(devMemInfoPtr->tblsAddr.ingr.UC_Config_Table); i<numOfTbls; i++, tblPtr++)
    {
        /* allocate memory for the specific table */
        tblPtr->memPtr = calloc(tblPtr->numOfEntries + 1,tblPtr->lineAddrAlign * 4);

        if (tblPtr->memPtr == 0)
        {
            skernelFatalError("smemFX950Init: allocation error - name\n");
        }

        /* last address in the address space is
           base address PLUS end of last entry based on entry line alignment
          */
        tblPtr->lastAddr =  tblPtr->baseAddr +
                            tblPtr->numOfEntries * tblPtr->lineAddrAlign * 4
                            - 4;
        /* active mem info */
        tblPtr->activeMemInfo.readFun = NULL;
        tblPtr->activeMemInfo.writeFun = smemFx950ActiveWriteTable;
        tblPtr->activeMemInfo.writeFunParam = (GT_UINTPTR)(void*)(&tblPtr);
    }
}

/*******************************************************************************
* smemFX950Init
*
* DESCRIPTION:
*       Init fx950 memory.
*
* INPUTS:
*       devMemInfoPtr   - pointer to device object memory.
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
void smemFX950Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr;


    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO *)calloc(1, sizeof(SMEM_FX950_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemFX950Init: allocation error\n");
    }

    devObjPtr->deviceMemory = devMemInfoPtr;

    /* init regs addresses */
    smemFX950RegsInit(devObjPtr,devMemInfoPtr);

    /* init specific functions array */
    smemFx950InitFuncArray(devObjPtr,devMemInfoPtr);

    /* bind the find memory function */
    devObjPtr->devFindMemFunPtr = (void *)smemOcelotFindMem;
}

/*******************************************************************************
* smemOcelotInitSecondaryFap20M
*
* DESCRIPTION:
*       create and init the secondary device of the ocelot --> FAP20M
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
static void smemOcelotInitSecondaryFap20M
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
)
{
    /* allocate secondary device object -- for the FAP20M */
    deviceObj->numOfCoreDevs    = 1;
    deviceObj->coreDevInfoPtr = calloc(deviceObj->numOfCoreDevs, sizeof(SKERNEL_CORE_DEVICE_INFO_STC));
    if (deviceObj->coreDevInfoPtr == NULL)
    {
        skernelFatalError(" smemOcelotInit: cannot allocate sons info, device %u", deviceObj->deviceId);
    }

    deviceObj->coreDevInfoPtr->devObjPtr = calloc(1, sizeof(SKERNEL_DEVICE_OBJECT));

    if (deviceObj->coreDevInfoPtr->devObjPtr == NULL)
    {
        skernelFatalError(" smemOcelotInit: cannot allocate secondary device %u", deviceObj->deviceId);
    }

    /* set needed values in the secondary device */
    deviceObj->coreDevInfoPtr->devObjPtr->deviceType = SKERNEL_98FX950;
    deviceObj->coreDevInfoPtr->devObjPtr->deviceFamily = SKERNEL_DEVICE_FAMILY(deviceObj->coreDevInfoPtr->devObjPtr);
    deviceObj->coreDevInfoPtr->devObjPtr->deviceHwId = deviceObj->deviceHwId;
    deviceObj->coreDevInfoPtr->devObjPtr->deviceId = deviceObj->deviceId;
}

/*******************************************************************************
* smemOcelotInit
*
* DESCRIPTION:
*       Init memory module for a Ocelot device.
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
void smemOcelotInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
)
{
    /* create and init the secondary device of the ocelot --> FAP20M */
    smemOcelotInitSecondaryFap20M(deviceObj);

    /* Initialize FAP20M memory */
    smemFAP20MInit(deviceObj->coreDevInfoPtr->devObjPtr);

    /* Initialize FX950 memory*/
    smemFX950Init(deviceObj);
}

/*******************************************************************************
*   smemFX950FindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
static void * smemFX950FindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                     * memPtr;
    SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr;
    GT_32                      index;
    GT_U32                     param;
    SMEM_FX950_TABLE_INFO_STC *specificTblInfoPtr=0;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemFX950FindMem: illegal pointer \n");
    }
    memPtr = 0;
    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    index = (address & REG_SPEC_FUNC_INDEX) >>
                 SMEM_OCELOT_UNIT_INDEX_FIRST_BIT_CNS;

    param   = devMemInfoPtr->specFunTbl[index].specParam;
    memPtr  = devMemInfoPtr->specFunTbl[index].specFun(devObjPtr,
                                                       address,
                                                       memSize,
                                                       param,
                                                       &specificTblInfoPtr);


    /* find active memory entry */
    if (activeMemPtrPtr != NULL && specificTblInfoPtr != NULL)
    {
        *activeMemPtrPtr = (SMEM_ACTIVE_MEM_ENTRY_STC *)&(specificTblInfoPtr->baseAddr);
    }

    return memPtr;
}

/*******************************************************************************
*   smemOcelotFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
static void * smemOcelotFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                     * memPtr;
    GT_32                      index;

    index = (address & REG_SPEC_FUNC_INDEX) >>
                 SMEM_OCELOT_UNIT_INDEX_FIRST_BIT_CNS;

    /* FAP20M memory */
    if (index == 1)
    {
        address &= 0xFFFFF; /* address in FAP20M memory (after reducing the offset of the unit)*/
        memPtr = smemFAP20MReg(devObjPtr->coreDevInfoPtr->devObjPtr,accessType,address,memSize,activeMemPtrPtr);

        if(activeMemPtrPtr != NULL && ((*activeMemPtrPtr) != NULL))
        {
            /* we need to save the active memory info that the FAP20M device returned
            because we need to wrap the call to it */
            devObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr = *activeMemPtrPtr;

            /* so give the caller the wrapper function */
            *activeMemPtrPtr = &ocelotWrapperFap20M;
        }
    }
    else
    /* FX950 memory*/
    {
        memPtr = smemFX950FindMem(devObjPtr,accessType,address,memSize,activeMemPtrPtr);
    }
    return memPtr;
}

/*******************************************************************************
*   smemFx950FatalError
*
* DESCRIPTION:
*       function for non-bound memories
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950FatalError(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    skernelFatalError("smemOcelotFatalError: illegal function pointer\n");
    *specificTblInfoPtrPtr = NULL;

    return NULL;
}

/*******************************************************************************
*   smemFx950MemFindInUnit
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.- in specific unit
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       unitRegistersPtr - pointer to unit's registers
*       unitRegistersNum - number of registers in unit
*       unitTablesPtr - pointer to unit's tables info
*       unitTablesNum -  number of registers in unit
*       address     - address of memory(register or table).
*       memSize     - size of memory
*       doFatalError  - do fatal error if memory not found in the unit
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950MemFindInUnit
(
    IN SKERNEL_DEVICE_OBJECT *      devObjPtr,
    IN GT_U32                       *unitRegistersPtr,
    IN GT_U32                       unitRegistersNum,
    IN SMEM_FX950_TABLE_INFO_STC    *unitTablesPtr,
    IN GT_U32                       unitTablesNum,
    IN GT_U32                       address,
    IN GT_U32                       memSize,
    IN GT_UINTPTR                       param,
    IN GT_BOOL                      doFatalError,
    OUT SMEM_FX950_TABLE_INFO_STC   **specificTblInfoPtrPtr
)
{
    GT_U32                  index,ii;
    GT_U32                  *regValuePtr=NULL;/*registers values*/
    SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* start by looking in the registers */
    for(index = 0 ; index < unitRegistersNum ; index++ , unitRegistersPtr++)
    {
        /*check for this address -- for match*/
        if(unitRegistersPtr[0] == address)
        {
            regValuePtr = (GT_U32 *)(&devMemInfoPtr->regsValues) + (unitRegistersPtr - (GT_U32 *)(&devMemInfoPtr->regsAddr));

            /*check that the next addresses are valid*/
            for(ii=1 ; ii < memSize ; ii++,unitRegistersPtr++)
            {
                if((unitRegistersPtr[0] + 4) != unitRegistersPtr[1])
                {
                    skernelFatalError("smemFx950MemFindInUnit: wrong address or memSize\n");
                }
            }

            return regValuePtr;
        }
    }

    /* look for the address in the 'Tables' */
    for(ii = 0 ; ii < unitTablesNum ; ii++, unitTablesPtr++)
    {
        if(unitTablesPtr->baseAddr <= address &&
           unitTablesPtr->lastAddr >= (address + memSize * 4 - 4))
        {
            index = (address - unitTablesPtr->baseAddr)/4;

            *specificTblInfoPtrPtr = (SMEM_FX950_TABLE_INFO_STC *)&(unitTablesPtr->memPtr[index]);
            return (void *)*specificTblInfoPtrPtr;
        }
    }

    if(doFatalError == GT_TRUE)
    {
        /* the address not found in the unit */
        skernelFatalError("smemFx950MemFindInUnit: the address not found in the unit\n");
    }

    return NULL;
}

/* set pointer to registers addresses and number */
#define SMEM_FX950_REGS_MAC(unit,_regAddrPtr,_numRegs)      \
    _regAddrPtr = (void*)&devMemInfoPtr->regsAddr.unit;     \
    _numRegs = sizeof(devMemInfoPtr->regsAddr.unit) / sizeof(GT_U32)

/* set pointer to tables info and number */
#define SMEM_FX950_TBLS_MAC(unit,_tblPtr,_numTbls)      \
    _tblPtr = (void*)&devMemInfoPtr->tblsAddr.unit;     \
    _numTbls = sizeof(devMemInfoPtr->tblsAddr.unit) / sizeof(SMEM_FX950_TABLE_INFO_STC)

/*******************************************************************************
* smemFx950MG
*
* DESCRIPTION:
*       MG registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*       specificTblInfoPtr - pointer to the table
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950MG
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;


    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(MG,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(deviceObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_FALSE, NULL);
}

/*******************************************************************************
* smemFx950PEX
*
* DESCRIPTION:
*       PEX registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*       specificTblInfoPtr - pointer to the table
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950PEX
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(PEX,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(deviceObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE,NULL);
}

/*******************************************************************************
* smemFx950Ingress
*
* DESCRIPTION:
*       Ingress registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Ingress
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(ingr,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(deviceObjPtr,regAddrPtr,numRegs, &(devMemInfoPtr->tblsAddr.ingr.UC_Config_Table),16, address,memSize,param,GT_TRUE, specificTblInfoPtrPtr);
}

/*******************************************************************************
* smemFx950Egress
*
* DESCRIPTION:
*       Egress registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Egress
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(egr,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(deviceObjPtr,regAddrPtr,numRegs, &(devMemInfoPtr->tblsAddr.egr.EDQUnit.PD_mem),9,
                                  address,memSize,param,GT_TRUE,specificTblInfoPtrPtr);

}

/*******************************************************************************
* smemFx950Statistics
*
* DESCRIPTION:
*       Statistics registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Statistics
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(statistics,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(deviceObjPtr,regAddrPtr,numRegs,&(devMemInfoPtr->tblsAddr.statistics.egrConfig.egr_Msg_FIFO_High),5,
                                  address,memSize,param,GT_TRUE,specificTblInfoPtrPtr);
}

/*******************************************************************************
* smemFx950FlowCtrl
*
* DESCRIPTION:
*       Flow control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950FlowCtrl
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(flowCtrl,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE,NULL);
}

/*******************************************************************************
* smemFx950Misc
*
* DESCRIPTION:
*       Misc control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Misc
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(misc,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,&(devMemInfoPtr->tblsAddr.misc.XSMI.XSMI_Addr),1,
                                  address,memSize,param,GT_TRUE, specificTblInfoPtrPtr);
}

/*******************************************************************************
* smemFx950HyperGlink0
*
* DESCRIPTION:
*       HyperGlink0 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950HyperGlink0
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(hyperGlink0shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950HyperGStack0
*
* DESCRIPTION:
*       HyperGStack0 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950HyperGStack0
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(hyperGStack0shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950Convertor0
*
* DESCRIPTION:
*       Convertor0 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Convertor0
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(convertor0shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,&(devMemInfoPtr->tblsAddr.convertor0shared.hyperG_Stack_Ports_MAC_MIB_Cntrs),1,
                                  address,memSize,param,GT_TRUE, specificTblInfoPtrPtr);
}


/*******************************************************************************
* smemFx950GenXPCS0
*
* DESCRIPTION:
*       GenXPCS0 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950GenXPCS0
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(genericXPCS0shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950HyperGlink1
*
* DESCRIPTION:
*       HyperGlink1 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950HyperGlink1
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(hyperGlink1shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950HyperGStack1
*
* DESCRIPTION:
*       HyperGStack1 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950HyperGStack1
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(hyperGStack1shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950Convertor1
*
* DESCRIPTION:
*       Convertor1 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*       specificTblInfoPtr - pointer to the table
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Convertor1
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(convertor1shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE,NULL);
}


/*******************************************************************************
* smemFx950GenXPCS1
*
* DESCRIPTION:
*       GenXPCS1 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950GenXPCS1
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(genericXPCS1shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950FtdllCalib
*
* DESCRIPTION:
*       FtdllCalib control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950FtdllCalib
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(ftdll_Calibshared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,&(devMemInfoPtr->tblsAddr.ftdll_Calibshared.FTDLL_UNIT),1,
                                  address,memSize,param,GT_TRUE, specificTblInfoPtrPtr);
}

/*******************************************************************************
* smemFx950FtdllCalib_1
*
* DESCRIPTION:
*       FtdllCalib_1 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950FtdllCalib_1
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(ftdll_Calib_1shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,&(devMemInfoPtr->tblsAddr.ftdll_Calib_1shared.FTDLL_UNIT),1,
                                  address,memSize,param,GT_TRUE, specificTblInfoPtrPtr);
}


/*******************************************************************************
* smemFx950FtdllCalib_2
*
* DESCRIPTION:
*       FtdllCalib_2 control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950FtdllCalib_2
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(ftdll_Calib_2shared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,&(devMemInfoPtr->tblsAddr.ftdll_Calib_2shared.FTDLL_UNIT),1,
                                  address,memSize,param,GT_TRUE, specificTblInfoPtrPtr);
}


/*******************************************************************************
* smemFx950BM
*
* DESCRIPTION:
*       BM control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950BM
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(BM,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0,address,memSize,param,GT_TRUE, NULL);
}


/*******************************************************************************
* smemFx950StatisticsPCS
*
* DESCRIPTION:
*       StatisticsPCS control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950StatisticsPCS
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *regAddrPtr;/*registers addresses*/
    GT_U32                      numRegs;/*number of registers in the unit*/
    SMEM_FX950_DEV_MEM_INFO     *devMemInfoPtr;

    devMemInfoPtr = (SMEM_FX950_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    SMEM_FX950_REGS_MAC(statisticsPCSshared,regAddrPtr,numRegs);

    return smemFx950MemFindInUnit(devObjPtr,regAddrPtr,numRegs,NULL,0, address,memSize,param,GT_TRUE, NULL);
}

/*******************************************************************************
* smemFx950Unit0
*
* DESCRIPTION:
*       Unit0 (MG + PEX) control registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*       specificTblInfoPtr - pointer to the table
*
* OUTPUTS:
*       specificTblInfoPtrPtr - (pointer to) pointer to table info --> in case of access to
*                           table memory that uses active memory
*                           when (*specificTblInfoPtrPtr)== NULL after return from function
*                           means that there is no table accessing
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFx950Unit0
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32 address,
    IN         GT_U32 memSize,
    IN         GT_UINTPTR param,
    OUT        SMEM_FX950_TABLE_INFO_STC **specificTblInfoPtrPtr
)
{
    GT_U32                      *res;
    res = smemFx950MG(devObjPtr,address,memSize,param, specificTblInfoPtrPtr);
    if (res == NULL)
    {
        res = smemFx950PEX(devObjPtr,address,memSize,param, specificTblInfoPtrPtr);
    }
    return res;
}

/*******************************************************************************
*   smemFx950InitMemArray
*
* DESCRIPTION:
*       Init specific functions array.
*
* INPUTS:
*       devMemInfoPtr   - pointer to device memory object.
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
static void smemFx950InitFuncArray
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_FX950_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++)
    {
        devMemInfoPtr->specFunTbl[i].specFun    = smemFx950FatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemFx950Unit0;
    /* Unit 1 contains fap 20m memory*/
    devMemInfoPtr->specFunTbl[2].specFun        = smemFx950Ingress;
    devMemInfoPtr->specFunTbl[3].specFun        = smemFx950Egress;
    devMemInfoPtr->specFunTbl[4].specFun        = smemFx950Statistics;
    devMemInfoPtr->specFunTbl[5].specFun        = smemFx950FlowCtrl;
    devMemInfoPtr->specFunTbl[6].specFun        = smemFx950Misc;

    devMemInfoPtr->specFunTbl[8].specFun        = smemFx950HyperGlink0;
    devMemInfoPtr->specFunTbl[9].specFun        = smemFx950HyperGStack0;
    devMemInfoPtr->specFunTbl[10].specFun       = smemFx950Convertor0;
    devMemInfoPtr->specFunTbl[11].specFun       = smemFx950GenXPCS0;
    devMemInfoPtr->specFunTbl[12].specFun       = smemFx950HyperGlink1;
    devMemInfoPtr->specFunTbl[13].specFun       = smemFx950HyperGStack1;
    devMemInfoPtr->specFunTbl[14].specFun       = smemFx950Convertor1;
    devMemInfoPtr->specFunTbl[15].specFun       = smemFx950GenXPCS1;
    devMemInfoPtr->specFunTbl[16].specFun       = smemFx950FtdllCalib;
    devMemInfoPtr->specFunTbl[17].specFun       = smemFx950FtdllCalib_1;
    devMemInfoPtr->specFunTbl[18].specFun       = smemFx950FtdllCalib_2;
    devMemInfoPtr->specFunTbl[19].specFun       = smemFx950BM;
    devMemInfoPtr->specFunTbl[20].specFun       = smemFx950StatisticsPCS;

}

/*******************************************************************************
*  smemFx950ActiveWriteTable
*
* DESCRIPTION:
*      Write interrupts cause registers by read/write mask.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - memory size to be writen.
*       memPtr      - Pointer to the register's memory in the simulation.
*       writeMask   - 32 bits mask of writable bits.
*       inMemPtr    - Pointer to the memory to set register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemFx950ActiveWriteTable
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,/* contains pointer to the table */
    IN         GT_U32 * inMemPtr
)
{
    SMEM_FX950_TABLE_INFO_STC   *tblPtr; /* pointer to the table */
    GT_U32                      wordIndex;/* index of word in the table */
    GT_U32                      numOfEntryWords;/* number of words in the entry*/
    GT_U32                      *bufferPtr; /* pointer to table buffer --
                                              the entry is written there until the last word of the entry is written*/
    GT_U32                      numOfEntriesToWrite;/* number of entries that are ready to write to the table */
    GT_U32                      entryWordIndex;/* index of word in the entry*/
    GT_U32                      *startEntryPtr;/* pointer to the first entry that is ready to be written to the table*/
    GT_U32                      i,j;
    GT_U32                      numOfTailBufferWords;/* number of words to write to the buffer (not "ready" entry) */


    tblPtr = (SMEM_FX950_TABLE_INFO_STC*)param;

    /* pointer to buffer is after the last entry */
    bufferPtr = tblPtr->memPtr + (tblPtr->lastAddr - tblPtr->baseAddr) + 4;
    /* tblPtr->entrySize is in bits*/
    numOfEntryWords = CONVERT_BITS_TO_WORDS(tblPtr->entrySize);
    wordIndex = (address - tblPtr->baseAddr)/4;

    entryWordIndex = wordIndex % numOfEntryWords;
    numOfEntriesToWrite = (entryWordIndex + memSize/4) / numOfEntryWords;
    startEntryPtr = tblPtr->memPtr + (wordIndex - entryWordIndex)*4;

    /*if there are entries to write to the table - first we copy the buffer first*/
    if (numOfEntriesToWrite)
    {
        /* copy buffer to the table -- doesn't include the new data*/
        for(j=0; j<numOfEntryWords; j++, startEntryPtr++, bufferPtr++)
        {
            *startEntryPtr = *bufferPtr;
        }
    }

    /* to overwrite the old parts of the buffer with the new data */
    startEntryPtr = tblPtr->memPtr + wordIndex*4;

    /* write to the table number of entries that are ready */
    for(i=0; i<numOfEntriesToWrite; i++)
    {
        /* write one entry to the table*/
        for(j=0; j<numOfEntryWords; j++, startEntryPtr++, inMemPtr++)
        {
            *startEntryPtr = *inMemPtr;
        }
    }

    /* if there were no entries to write to the table, we write to the buffer
    start of the input data (may start at the middle of the entry)*/
    if (numOfEntriesToWrite == 0)
    {
        bufferPtr += entryWordIndex;
    }

    /* tail of entry that is not ready (last word not written) written to the buffer */
    if(memSize/4 - numOfEntriesToWrite*numOfEntryWords > 0)
    {
        numOfTailBufferWords = memSize/4 - numOfEntriesToWrite*numOfEntryWords;
        /* write to the buffer*/
        for(j=0; j<numOfTailBufferWords; j++, bufferPtr++, inMemPtr++)
        {
            *bufferPtr = *inMemPtr;
        }
    }
}


/*******************************************************************************
*  ocelotWrapperFap20MWriteFunActiveMemory
*
* DESCRIPTION:
*      write active memory for Ocelot to Wrap the FAP20M.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - memory size to be writen.
*       memPtr      - Pointer to the register's memory in the simulation.
*       writeMask   - 32 bits mask of writable bits.
*       inMemPtr    - Pointer to the memory to set register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void ocelotWrapperFap20MWriteFunActiveMemory
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,/* contains pointer to the table */
    IN         GT_U32 * inMemPtr
)
{
    if(devObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr == NULL ||
       devObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr->writeFun == NULL)
    {
        return;
    }

    /* call the secondary device write active memory function */
    devObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr->writeFun(/* the secondary device last write active memory pointer*/
        devObjPtr->coreDevInfoPtr->devObjPtr,/* the secondary device pointer*/
        address & 0xFFFFF,/* the FAP addresses are 20 bits long */
        memSize,
        memPtr,
        devObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr->writeFunParam,/* the secondary device last write active memory parameter */
        inMemPtr);
}


/*******************************************************************************
*  ocelotWrapperFap20MReadActiveMemory
*
* DESCRIPTION:
*      read active memory for Ocelot to Wrap the FAP20M.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - pointer to the register's memory in the simulation.
*       param       - extra parameter
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void ocelotWrapperFap20MReadActiveMemory
(
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    if(deviceObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr == NULL ||
       deviceObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr->readFun == NULL)
    {
        return;
    }

    /* call the secondary device read active memory function */
    deviceObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr->readFun(/* the secondary device last read active memory pointer*/
        deviceObjPtr->coreDevInfoPtr->devObjPtr,/* the secondary device pointer*/
        address & 0xFFFFF,/* the FAP addresses are 20 bits long */
        memSize,
        memPtr,
        deviceObjPtr->coreDevInfoPtr->currentActiveMemoryEntryPtr->readFunParam,/* the secondary device last read active memory parameter */
        outMemPtr);
}
