/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************
* pssBspApis.h - bsp APIs
*
* DESCRIPTION:
*       API's supported by BSP.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/

#ifndef __pssBspApisH
#define __pssBspApisH

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mvTypes.h"

#undef IN
#define IN
#undef OUT
#define OUT
#undef INOUT
#define INOUT

/*
 * Typedef: enum BSP_ETH_MRU
 *
 * Description:
 *      This type defines MRU values for CPU RGMII ifaces
 *
 * Note:
 *      The enum has to be compatible with MV_ETH_MRU.
 *
 */
typedef enum
{
    BSP_ETH_MRU_1518 = 1518,
    BSP_ETH_MRU_1522 = 1522,
    BSP_ETH_MRU_1552 = 1552,
    BSP_ETH_MRU_9022 = 9022,
    BSP_ETH_MRU_9192 = 9192,
    BSP_ETH_MRU_9700 = 9700,
    BSP_ETH_MRU_ILLEGAL = 0xFFFFFFFF
} BSP_ETH_MRU;

/*
 * Typedef: enum bspEthNetPortType_ENT
 *
 * Description:
 *      This type defines types of switch ports for BSP ETH driver.
 *
 * Fields:
 *      bspEthNetPortType_cpssOwned_E      - packets forwarded to CPSS
 *      bspEthNetPortType_osOwned_RxLeaveTxNoAddDsa_E   - packets forwarded to OS (w/o DSA)
 *      bspEthNetPortType_osOwned_RxRemoveTxAddDsa_E - packets forwarded to OS (w. DSA)
 *
 * Note:
 *      The enum has to be compatible with MV_NET_OWN.
 *
 */
typedef enum
{
    bspEthNetPortType_cpssOwned_E                   = 0,
    bspEthNetPortType_osOwned_RxLeaveTxNoAddDsa_E   = 1,
    bspEthNetPortType_osOwned_RxRemoveTxAddDsa_E    = 2,
    bspEthNetPortType_numOfTypes
} bspEthNetPortType_ENT;

/*
 * Typedef: enum bspCacheType_ENT
 *
 * Description:
 *             This type defines used cache types
 *
 * Fields:
 *          bspCacheType_InstructionCache_E - cache of commands
 *          bspCacheType_DataCache_E        - cache of data
 *
 * Note:
 *      The enum has to be compatible with MV_MGMT_CACHE_TYPE_ENT.
 *
 */
typedef enum
{
    bspCacheType_InstructionCache_E,
    bspCacheType_DataCache_E
} bspCacheType_ENT;

/*
 * Description: Enumeration For PCI interrupt lines.
 *
 * Enumerations:
 *      bspPciInt_PCI_INT_A_E - PCI INT# A
 *      bspPciInt_PCI_INT_B_ - PCI INT# B
 *      bspPciInt_PCI_INT_C - PCI INT# C
 *      bspPciInt_PCI_INT_D - PCI INT# D
 *
 * Assumption:
 *      This enum should be identical to bspPciInt_PCI_INT.
 */
typedef enum
{
    bspPciInt_PCI_INT_A = 1,
    bspPciInt_PCI_INT_B,
    bspPciInt_PCI_INT_C,
    bspPciInt_PCI_INT_D
} bspPciInt_PCI_INT;

/*
 * Typedef: enum bspSmiAccessMode_ENT
 *
 * Description:
 *             PP SMI access mode.
 *
 * Fields:
 *          bspSmiAccessMode_Direct_E   - direct access mode (single/parallel)
 *          bspSmiAccessMode_inDirect_E - indirect access mode
 *
 * Note:
 *      The enum has to be compatible with MV_MGMT_CACHE_TYPE_ENT.
 *
 */
typedef enum
{
    bspSmiAccessMode_Direct_E,
    bspSmiAccessMode_inDirect_E
} bspSmiAccessMode_ENT;

/*
 * Typedef: enum bspEthTxMode_ENT
 *
 * Description:
 *      Enum describing the TX mode of the driver.
 *
 * Fields:
 *      bspEthTxMode_asynch_E - async TX mode
 *      bspEthTxMode_synch_E  - sync  TX mode
 *
 * Note:
 *      None.
 *
 */
typedef enum
{
    bspEthTxMode_asynch_E = 0,
    bspEthTxMode_synch_E
} bspEthTxMode_ENT;

/*
 * Typedef: enum BSP_ETH_RX_Q
 *
 * Description:
 *      Describes available RX queue for MII interface
 *
 * Fields:
 *
 * Note:
 *      Should be full compatible to MII_ETH_RX_Q_XXX.
 */
typedef enum
{
    BSP_ETH_RX_Q_0 = 0,
    BSP_ETH_RX_Q_1,
    BSP_ETH_RX_Q_2,
    BSP_ETH_RX_Q_3,
    BSP_ETH_RX_Q_4,
    BSP_ETH_RX_Q_5,
    BSP_ETH_RX_Q_6,
    BSP_ETH_RX_Q_7,
    BSP_ETH_RX_Q_NUM,
    BSP_ETH_RX_Q_ALL = 0xFF
} BSP_ETH_RX_Q;

/*
 * Typedef: enum BSP_ETH_TX_Q
 *
 * Description:
 *      Describes available TX queue for MII interface
 *
 * Fields:
 *
 * Note:
 *      Should be full compatible to MII_ETH_TX_Q_XXX.
 */
typedef enum
{
    BSP_ETH_TX_Q_0 = 0,
    BSP_ETH_TX_Q_1,
    BSP_ETH_TX_Q_2,
    BSP_ETH_TX_Q_3,
    BSP_ETH_TX_Q_4,
    BSP_ETH_TX_Q_5,
    BSP_ETH_TX_Q_6,
    BSP_ETH_TX_Q_7,
    BSP_ETH_TX_Q_NUM,
    BSP_ETH_TX_Q_ALL = 0xFF
} BSP_ETH_TX_Q;

/*
 * BSP_ETH_RX_MAPPING (enum)
 *
 * Description:
 *      Enumerates all supported mapping of RX frame to GbE RX queue.
 *
 * Note:
 *      Should be identical to MV_ETH_RX_MAPPING enum
 *
 */
typedef enum
{
    BSP_ETH_RX_DEFAULT_MAP     = 0x0, /* default queue is used */
    BSP_ETH_RX_UP_FIELD_MAP    = 0x1, /* RX Queue = UP[2:0] */
    BSP_ETH_RX_CPU_CODE_MAP    = 0x2, /* CPU_Code2RxQ map <-- DFSMT table */
    BSP_ETH_RX_DEV_PORT_UP_MAP = 0x3  /* RX Queue = */
                              /* (SrcPort==<SPID> & SrcDev = <SDID>,UP[2:1]) */
} BSP_ETH_RX_MAPPING;

/*******************************************************************************
* BSP_RX_ISR_CALLBACK_FUNCPTR
*
* DESCRIPTION:
*       The prototype of the routine to be called on TX_DONE Interrupt
*       The interrupts are NOT masked before calling to callback function.
*       It is the responsibility of the upper layer (CPSS) to implement
*       the interrupt handling policy. Meaning, that the implementer of
*       the callback routine should decide if
*       [1] Layer1 interrupts should be masked during Ready RX queues processing
*           (disabling thus RX_READY interrupts for _ALL_ RX queue)
*       or
*       [2] Layer2 interrupts should be masked for the RX queues represented by
*           rxReadyQBitMask (giving thus opportunity to high-priority RX queues
*           to interrupt processing of low-priority RX queues packets).
*       It is the responsibility of the upper layer (CPSS):
*       [1] Mask/unmask the interrupts through provided API
*       and
*       [2] Acknowledge the interrupts through provided API
*
* INPUTS:
*       rxReadyQBitMask    - bit mask that represents RX_READY queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
typedef MV_VOID (*BSP_RX_ISR_CALLBACK_FUNCPTR)(MV_U32 rxReadyQBitMask);

/*******************************************************************************
* BSP_TX_DONE_ISR_CALLBACK_FUNCPTR
*
* DESCRIPTION:
*       The prototype of the routine to be called on TX_DONE Interrupt
*       The interrupts are NOT masked before calling to callback function.
*       It is the responsibility of the upper layer (CPSS) to implement
*       the interrupt handling policy. Meaning, that the implementer of
*       the callback routine should decide if
*       [1] Layer1 interrupts should be masked during TX_DONE queues processing
*           (disabling thus ALL TX_DONE interrupts for _ALL_ TX queue)
*       or
*       [2] Layer2 interrupts should be masked for the TX queues represented by
*           txDoneQBitMask (giving thus opportunity to high-priority TX queues
*           to interrupt processing of low-priority TX queues packets).
*       It is the responsibility of the upper layer (CPSS):
*       [1] Mask/unmask the interrupts through provided API
*       and
*       [2] Acknowledge the interrupts through provided API
*
* INPUTS:
*       txDoneQBitMask    - bit mask that represents TX_DONE queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       TX_DONE interrupts are masked before calling to callback function.
*       It is the responsibility of the upper layer (CPSS) unmask interrupt.
*
*******************************************************************************/
typedef MV_VOID (*BSP_TX_DONE_ISR_CALLBACK_FUNCPTR)(MV_U32 txDoneQBitMask);

/*******************************************************************************
* BSP_RX_CALLBACK_FUNCPTR
*
* DESCRIPTION:
*       The prototype of the RX callback routine to be called
*       for each received packet
*
* INPUTS:
*       buffs     - A list of pointers to the packets segments.
*       buffsLen  - A list of segment length.
*       buffNum   - The number of segment in segment list.
*       rxQ       - the received queue number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE if it has handled the input packet and no further action should
*               be taken with it, or
*       MV_FALSE if it has not handled the input packet and normal processing.
*
* COMMENTS:
*       'buffs[]' itself MUST NOT be stored by the callback function.
*       Its only purpose is to supply pointers to the buffers, which can be
*       used/stored/etc.
*
*******************************************************************************/
typedef MV_STATUS (*BSP_RX_CALLBACK_FUNCPTR)(MV_U8  *buffs[],
                                             MV_U32  buffsLen[],
                                             MV_U32  buffNum,
                                             MV_U32  rxQ);

/*******************************************************************************
* BSP_TX_COMPLETE_CALLBACK_FUNCPTR
*
* DESCRIPTION:
*       The prototype of the routine to be called after a packet was received
*
* INPUTS:
*       buffs     - A list of pointers to the frame buffers.
*       buffNum   - The number of buffers in the frame.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE if it has handled the input packet and no further action should
*               be taken with it, or
*       MV_FALSE if it has not handled the input packet and normal processing.
*
* COMMENTS:
*       'buffs[]' itself MUST NOT be stored by the callback function.
*       Its only purpose is to supply pointers to the buffers, which can be
*       used/stored/etc.
*
*******************************************************************************/
typedef MV_STATUS (*BSP_TX_COMPLETE_CALLBACK_FUNCPTR)(MV_U8  *buffs[],
                                                      MV_U32  buffNum);

/*** PCI ***/
/*******************************************************************************
* bspPciConfigWriteReg
*
* DESCRIPTION:
*       This routine write register to the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*       data     - data to write.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigWriteReg
(
    IN  MV_U32  busNo,
    IN  MV_U32  devSel,
    IN  MV_U32  funcNo,
    IN  MV_U32  regAddr,
    IN  MV_U32  data
);

/*******************************************************************************
* bspPciConfigReadReg
*
* DESCRIPTION:
*       This routine read register from the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*
* OUTPUTS:
*       data     - the read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigReadReg
(
    IN  MV_U32  busNo,
    IN  MV_U32  devSel,
    IN  MV_U32  funcNo,
    IN  MV_U32  regAddr,
    OUT MV_U32  *data
);

/*******************************************************************************
* bspPciFindDev
*
* DESCRIPTION:
*       This routine returns the next instance of the given device (defined by
*       vendorId & devId).
*
* INPUTS:
*       vendorId - The device vendor Id.
*       devId    - The device Id.
*       instance - The requested device instance.
*
* OUTPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciFindDev
(
    IN  MV_U16  vendorId,
    IN  MV_U16  devId,
    IN  MV_U32  instance,
    OUT MV_U32  *busNo,
    OUT MV_U32  *devSel,
    OUT MV_U32  *funcNo
);

/*******************************************************************************
* bspPciGetIntVec
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intVec - PCI interrupt vector.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspPciGetIntVec
(
    IN  bspPciInt_PCI_INT  pciInt,
    OUT void               **intVec
);

/*******************************************************************************
* bspPciGetIntMask
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intMask - PCI interrupt mask.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
MV_STATUS bspPciGetIntMask
(
    IN  bspPciInt_PCI_INT  pciInt,
    OUT MV_U32             *intMask
);

/*******************************************************************************
* bspPciEnableCombinedAccess
*
* DESCRIPTION:
*       This function enables / disables the Pci writes / reads combining
*       feature.
*       Some system controllers support combining memory writes / reads. When a
*       long burst write / read is required and combining is enabled, the master
*       combines consecutive write / read transactions, if possible, and
*       performs one burst on the Pci instead of two. (see comments)
*
* INPUTS:
*       enWrCombine - MV_TRUE enables write requests combining.
*       enRdCombine - MV_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on sucess,
*       MV_NOT_SUPPORTED    - if the controller does not support this feature,
*       MV_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
MV_STATUS bspPciEnableCombinedAccess
(
    IN  MV_BOOL     enWrCombine,
    IN  MV_BOOL     enRdCombine
);

/*** reset ***/
/*******************************************************************************
* bspResetInit
*
* DESCRIPTION:
*       This routine calls in init to do system init config for reset.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspResetInit
(
    MV_VOID
);


/*******************************************************************************
* bspReset
*
* DESCRIPTION:
*       This routine calls to reset of CPU.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspReset
(
    MV_VOID
);

/*** cache ***/
/*******************************************************************************
* bspCacheFlush
*
* DESCRIPTION:
*       Flush to RAM content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block (in bytes)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspCacheFlush
(
    IN bspCacheType_ENT         cacheType,
    IN void                     *address_PTR,
    IN size_t                   size
);

/*******************************************************************************
* bspCacheInvalidate
*
* DESCRIPTION:
*       Invalidate current content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspCacheInvalidate
(
    IN bspCacheType_ENT         cacheType,
    IN void                     *address_PTR,
    IN size_t                   size
);

/*** DMA ***/
/*******************************************************************************
* bspDmaWrite
*
* DESCRIPTION:
*       Write a given buffer to the given address using the Dma.
*
* INPUTS:
*       address     - The destination address to write to.
*       buffer      - The buffer to be written.
*       length      - Length of buffer in words.
*       burstLimit  - Number of words to be written on each burst.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
MV_STATUS bspDmaWrite
(
    IN  MV_U32  address,
    IN  MV_U32  *buffer,
    IN  MV_U32  length,
    IN  MV_U32  burstLimit
);

/*******************************************************************************
* bspDmaRead
*
* DESCRIPTION:
*       Read a memory block from a given address.
*
* INPUTS:
*       address     - The address to read from.
*       length      - Length of the memory block to read (in words).
*       burstLimit  - Number of words to be read on each burst.
*
* OUTPUTS:
*       buffer  - The read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
MV_STATUS bspDmaRead
(
    IN  MV_U32  address,
    IN  MV_U32  length,
    IN  MV_U32  burstLimit,
    OUT MV_U32  *buffer
);

/*** PCI ***/
/*******************************************************************************
* bspPciConfigWriteReg
*
* DESCRIPTION:
*       This routine write register to the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*       data     - data to write.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigWriteReg
(
    IN  MV_U32  busNo,
    IN  MV_U32  devSel,
    IN  MV_U32  funcNo,
    IN  MV_U32  regAddr,
    IN  MV_U32  data
);

/*******************************************************************************
* bspPciConfigReadReg
*
* DESCRIPTION:
*       This routine read register from the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*
* OUTPUTS:
*       data     - the read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigReadReg
(
    IN  MV_U32  busNo,
    IN  MV_U32  devSel,
    IN  MV_U32  funcNo,
    IN  MV_U32  regAddr,
    OUT MV_U32  *data
);

/*******************************************************************************
* bspPciFindDev
*
* DESCRIPTION:
*       This routine returns the next instance of the given device (defined by
*       vendorId & devId).
*
* INPUTS:
*       vendorId - The device vendor Id.
*       devId    - The device Id.
*       instance - The requested device instance.
*
* OUTPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciFindDev
(
    IN  MV_U16  vendorId,
    IN  MV_U16  devId,
    IN  MV_U32  instance,
    OUT MV_U32  *busNo,
    OUT MV_U32  *devSel,
    OUT MV_U32  *funcNo
);

/*******************************************************************************
* bspPciGetIntVec
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intVec - PCI interrupt vector.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspPciGetIntVec
(
    IN  bspPciInt_PCI_INT  pciInt,
    OUT void               **intVec
);

/*******************************************************************************
* bspPciGetIntMask
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intMask - PCI interrupt mask.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
MV_STATUS bspPciGetIntMask
(
    IN  bspPciInt_PCI_INT  pciInt,
    OUT MV_U32             *intMask
);

/*******************************************************************************
* bspPciEnableCombinedAccess
*
* DESCRIPTION:
*       This function enables / disables the Pci writes / reads combining
*       feature.
*       Some system controllers support combining memory writes / reads. When a
*       long burst write / read is required and combining is enabled, the master
*       combines consecutive write / read transactions, if possible, and
*       performs one burst on the Pci instead of two. (see comments)
*
* INPUTS:
*       enWrCombine - MV_TRUE enables write requests combining.
*       enRdCombine - MV_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on sucess,
*       MV_NOT_SUPPORTED    - if the controller does not support this feature,
*       MV_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
MV_STATUS bspPciEnableCombinedAccess
(
    IN  MV_BOOL     enWrCombine,
    IN  MV_BOOL     enRdCombine
);


/*** SMI ***/
/*******************************************************************************
* bspSmiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       smiAccessMode - direct/indirect mode
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiInitDriver
(
    bspSmiAccessMode_ENT  *smiAccessMode
);

/*******************************************************************************
* bspSmiReadReg
*
* DESCRIPTION:
*       Reads a register from SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*		actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*
* OUTPUTS:
*       valuePtr     - Data read from register.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiReadReg
(
    IN  MV_U32  devSlvId,
    IN  MV_U32  actSmiAddr,
    IN  MV_U32  regAddr,
    OUT MV_U32 *valuePtr
);

/*******************************************************************************
* bspSmiWriteReg
*
* DESCRIPTION:
*       Writes a register to an SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*		actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*       value   - data to be written.
*
* OUTPUTS:
*        None,
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiWriteReg
(
    IN MV_U32 devSlvId,
    IN MV_U32 actSmiAddr,
    IN MV_U32 regAddr,
    IN MV_U32 value
);


/*** TWSI ***/
/*******************************************************************************
* bspTwsiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiInitDriver
(
    MV_VOID
);

/*******************************************************************************
* bspTwsiWaitNotBusy
*
* DESCRIPTION:
*       Wait for TWSI interface not BUSY
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiWaitNotBusy(MV_VOID);

/*******************************************************************************
* bspTwsiMasterReadTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiMasterReadTrans
(
    IN MV_U8           devId,       /* I2c slave ID                              */
    IN MV_U8           *pData,      /* Pointer to array of chars (address / data)*/
    IN MV_U8           len,         /* pData array size (in chars).              */
    IN MV_BOOL         stop         /* Indicates if stop bit is needed in the end  */
);

/*******************************************************************************
* bspTwsiMasterWriteTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiMasterWriteTrans
(
    IN MV_U8           devId,       /* I2c slave ID                              */
    IN MV_U8           *pData,      /* Pointer to array of chars (address / data)*/
    IN MV_U8           len,         /* pData array size (in chars).              */
    IN MV_BOOL         stop         /* Indicates if stop bit is needed in the end  */
);


/*** Dragonite Driver ***/
#ifdef MV_INCLUDE_DRAGONITE
MV_STATUS bspDragoniteSWDownload(const MV_VOID *src, MV_U32 size);
MV_STATUS bspDragoniteEnableSet(MV_BOOL enableFlag);
MV_STATUS bspDragoniteInit(MV_VOID);
MV_STATUS bspDragoniteSharedMemWrite(MV_U32 offset, const MV_VOID *buffer,
                                     MV_U32 length);
MV_STATUS bspDragoniteSharedMemRead (MV_U32 offset, MV_VOID *buff,
                                     MV_U32 length);
MV_STATUS bspDragoniteSharedMemBaseAddrGet(MV_U32 *sharedMemP);
MV_STATUS bspDragoniteGetIntVec(MV_U32 *intVec);
#endif /* MV_INCLUDE_DRAGONITE */


/*** Ethernet Driver ***/
/*******************************************************************************
* bspEthDisableDsrMode
*
* DESCRIPTION:
*       Disables the DSR mode for PSS BSP Ethernet driver (the old approach,
*       the additional tasks are created by PSS BSP to handle RX/TX).
*       THIS FUNCTION SHOULD BE CALLED FIRST! (Before calling to bspEthInit()).
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspEthDisableDsrMode(MV_VOID);

/*******************************************************************************
* bspEthRegisterRxIsrCb
*
* DESCRIPTION:
*       Registers RX ISR callback function which is called when GbE sets
*       unmasked RX Interrupt in Main Interrupt Cause Low Register
*       of KW CPU.
*
* INPUTS:
*       rxReadyIsrCb   - Pointer to RX ISR callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspEthRegisterRxIsrCb(BSP_RX_ISR_CALLBACK_FUNCPTR rxReadyIsrCb);

/*******************************************************************************
* bspEthRegisterTxDoneIsrCb
*
* DESCRIPTION:
*       Registers TX_DONE ISR callback function which is called when GbE sets
*       unmasked TX_DONE Interrupt in Main Interrupt Cause Low Register
*       of internal CPU.
*
* INPUTS:
*       txDoneIsrCb   - Pointer to TX_DONE ISR callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspEthRegisterTxDoneIsrCb(BSP_TX_DONE_ISR_CALLBACK_FUNCPTR txDoneIsrCb);

/*******************************************************************************
* bspEthRxReadyQGet
*
* DESCRIPTION:
*       Checks what RX queue are ready (have arrived packets).
*       This function does NOT acknowledges pending interrupts, transferring
*       thus this responsibility to the caller.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Bit mask representing the ready RX queues.
*       This mask should be handled through appropriate API.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 bspEthRxReadyQGet(MV_VOID);

/*******************************************************************************
* bspEthTxDoneQGet
*
* DESCRIPTION:
*       Checks what TX queue finished the transmission.
*       This function does NOT acknowledges pending interrupts, transferring
*       thus this responsibility to the caller.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Bit mask representing the TX_DONE queues.
*       This mask should be handled through appropriate API.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 bspEthTxDoneQGet(MV_VOID);

/*******************************************************************************
* bspBitMaskFfs
*
* DESCRIPTION:
*       Process bit mask and returns the position of the first
*       (least significant) bit set in the provided mask.
*       Bits are numbered starting from 1, starting at the right-most (least sig-
*       nificant) bit.  A return value of zero from any of these functions means
*       that the argument was zero.
*
* INPUTS:
*       bitmask    - bit mask (e.g. may represent ready RX/TX queues)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The position of the first (least significant) bit set.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 bspBitMaskFfs(MV_U32 bitMask);

/*******************************************************************************
* bspEthInit
*
* DESCRIPTION:
*       Init the ethernet HW and HAL
*
* INPUTS:
*       port   - eth port number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspEthInit(MV_U8 port);

/*******************************************************************************
* bspEthPortRxInit
*
* DESCRIPTION: Init the ethernet port Rx interface
*
* INPUTS:
*       poolSize       - buffer pool size
*       pPool          - the address of the pool
*       buffSize       - the buffer requested size
*       pBuffNum       - # of requested buffs, and actual buffers created
*       hdrOffset      - packet header offset size
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortRxInit(IN   MV_U32   rxBuffsBulkSize,
                           IN   MV_U8   *rxBuffsBulk,
                           IN   MV_U32   buffSize,
                           OUT  MV_U32  *pActualBuffNum,
                           IN   MV_U32   hdrOffset,
                           IN   MV_U32   numOfRxQueues,
                           IN   MV_U32   rxQbufPercent[]);

/*******************************************************************************
* bspEthPortTxInit
*
* DESCRIPTION:
*       Init the ethernet port Tx interface
*
* INPUTS:
*       txDescNumPerQ - number of requested descriptors per TX queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortTxInit(MV_U32 txDescNumPerQ);

/*******************************************************************************
* bspEthPortEnable
*
* DESCRIPTION:
*       Enable the ethernet port interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortEnable(MV_VOID);

/*******************************************************************************
* bspEthPortDisable
*
* DESCRIPTION: Disable the ethernet port interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortDisable(MV_VOID);

/*******************************************************************************
* bspEthRxBuffSizeGet
*
* DESCRIPTION: Gets RX buffer size for RMGII-1 iface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_U32 - RX buffer size.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_U32 bspEthRxBuffSizeGet(MV_VOID);

/*******************************************************************************
* bspEthMruGet
*
* DESCRIPTION: Gets configured MRU from MII port.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Enum representing the configured MRU value.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
BSP_ETH_MRU bspEthMruGet(MV_VOID);

/*******************************************************************************
* bspEthMruSet
*
* DESCRIPTION: Sets MRU for MII port.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful.
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthMruSet(BSP_ETH_MRU mru);

/*******************************************************************************
* bspEthPortTxModeSet
*
* DESCRIPTION: Set the ethernet port tx mode
*
* INPUTS:
*       if txMode == bspEthTxMode_asynch_E
*           don't wait for TX done - free packet when interrupt received
*       if txMode == bspEthTxMode_synch_E
*           wait to TX done and free packet immediately
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful
*       MV_NOT_SUPPORTED if input is wrong
*       MV_FAIL if bspTxModeSetOn is zero
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortTxModeSet (bspEthTxMode_ENT txMode);

/*******************************************************************************
* bspEthPortTx
*
* DESCRIPTION:
*       This function is called to send a frame through MII port (PP_CPU_Port)
*       using the default TX queue.
*
* INPUTS:
*       buffs     - A list of pointers to the frame buffers
*       buffsLen  - A list of buffer lengths
*       buffNum   - The number of buffers in frame
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortTx(MV_U8 *buffs[], MV_U32 buffsLen[], MV_U32 buffNum);

/*******************************************************************************
* bspSendPkt
*
* DESCRIPTION:
*       This function is called to send a frame through MII port (PP_CPU_Port)
*       using the the provided TX queue. The frame is represented by MV_GND_PKT_INFO
*       and array of MV_BUF_INFO.
*
* INPUTS:
*       pktInfoP  - represents frame (possibly Scatter-Gather)
*       txQ       - TX queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspSendPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ);

/*******************************************************************************
* bspEthPortTxQueue
*
* DESCRIPTION:
*       This function is called to send a frame through MII port (PP_CPU_Port)
*       using some specific TX queue.
*
* INPUTS:
*       buffs         - A list of pointers to the packets buffs.
*       buffsLen      - A list of segment length.
*       buffNum       - The number of segment in segment list.
*       txQ           - The TX queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortTxQueue(MV_U8 *buffs[],
                            MV_U32 buffsLen[],
                            MV_U32 buffNum,
                            MV_U32 txQ);

/*******************************************************************************
* bspGetPkt
*
* DESCRIPTION:
*       This function is called when a packet has received.
*
* INPUTS:
*       rxQ       - RX Queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_GND_PKT_INFO *bspGetPkt(MV_U32 rxQ);

/*******************************************************************************
* bspGetTxDonePkt
*
* DESCRIPTION:
*       This function is called to send a frame through MII port (PP_CPU_Port)
*       using the the provided TX queue. The frame is represented by MV_GND_PKT_INFO
*       and array of MV_BUF_INFO.
*
* INPUTS:
*       txQ       - TX queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to MV_GND_PKT_INFO that represents the transmitted packet.
*       NULL if nothing has been transmitted.
*
* COMMENTS:
*       [1] The TX buffers are owned by upper layer (CPSS), but MV_GND_PKT_INFO pool
*       TX purposes are owned by PSS BSP, hence, this function (bspGetTxDonePkt)
*       has to be called by upper layer to return the MV_GND_PKT_INFO to TX pool of
*       PSS BSP.
*       [2] The transmitted packet is represented by the next structure:
*       (MV_GND_PKT_INFO?MV_BUF_INFO[]). Note MV_BUF_INFO[] is an array. Its length
*       as the maximum number of buffer for Scatter-Gather frame.
*
*******************************************************************************/
MV_GND_PKT_INFO *bspGetTxDonePkt(MV_U32 txQ);

/*******************************************************************************
* bspEthRxPktFree
*
* DESCRIPTION:
*       This function is called by upper layer (CPSS) after corresponding
*       bspGetPkt(). Its aim is to return the linked list structure
*       (MV_GND_PKT_INFO?MV_BUF_INFO?buffer)?next?... that represents the packet
*       to the GbE RX SDMA descriptors.
*
* INPUTS:
*       pktInfoP  - represents frame (possibly Scatter-Gather).
*       rxQ       - RX queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthRxPktFree(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ);

/*******************************************************************************
* bspEthRxPacketFree
*
* DESCRIPTION:
*       This routine frees the received Rx buffer.
*
* INPUTS:
*       buffs     - A list of pointers to the packets buffs.
*       buffNum   - The number of segment in segment list.
*       rxQ       - Receive queue number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthRxPacketFree(MV_U8 *buffs[], MV_U32 buffNum, MV_U32 rxQ);

/*******************************************************************************
* bspEthFreeTxPkt
*
* DESCRIPTION:
*       This function is called by upper layer (CPSS) from context/task that
*       handles ASYNCHRONOUSLY sent packet. For each ASYNCHRONOUSLY sent packet
*       upper layer should call the appropriate PSS BSP API to retrieve that
*       packet in order to reclaim ownership on transmitted buffers. The sent
*       packet is returned to upper layer (CPSS) in the form of MV_GND_PKT_INFO.
*       This MV_GND_PKT_INFO should be returned to its pool by calling bspEthFreeTxPkt.
*
* INPUTS:
*       pktInfoP  - represents frame. Has structure: MV_GND_PKT_INFO->MV_BUF_INFO[].
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthFreeTxPkt(MV_GND_PKT_INFO *pktInfoP);

/*******************************************************************************
* bspMiiTxDoneIntMask
*
* DESCRIPTION:
*       Masks specific TX_DONE interrupts on MII port (Layer2 interrupts).
*
* INPUTS:
*       txQBitMaks   - bit mask representing RX queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspMiiTxDoneIntMask(MV_U32 txQBitMask);

/*******************************************************************************
* bspMiiTxDoneIntUnmask
*
* DESCRIPTION:
*       Unmasks specific TX_DONE interrupts on MII port (Layer2 interrupts).
*
* INPUTS:
*       txQBitMaks   - bit mask representing RX queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspMiiTxDoneIntUnmask(MV_U32 txQBitMask);

/*******************************************************************************
* bspMiiRxReadyIntMask
*
* DESCRIPTION:
*       Masks specific RX_READY interrupts on MII port (Layer2 interrupts).
*
* INPUTS:
*       rxQBitMaks   - bit mask representing RX queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspMiiRxReadyIntMask(MV_U32 rxQBitMask);

/*******************************************************************************
* bspMiiRxReadyIntUnmask
*
* DESCRIPTION:
*       Unmasks specific RX_READY interrupts on MII port (Layer2 interrupts).
*
* INPUTS:
*       rxQBitMaks   - bit mask representing RX queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspMiiRxReadyIntUnmask(MV_U32 rxQBitMask);

/*******************************************************************************
* bspMiiRxReadyIntAck
*
* DESCRIPTION:
*       Acknowledges specific RX_READY interrupts on MII port (Layer2 interrupts).
*
* INPUTS:
*       rxQBitMaks   - bit mask representing RX queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspMiiRxReadyIntAck(MV_U32 rxQBitMask);

/*******************************************************************************
* bspMiiTxDoneIntAck
*
* DESCRIPTION:
*       Acknowledges specific TX_DONE interrupts on MII port (Layer2 interrupts).
*
* INPUTS:
*       txQBitMaks   - bit mask representing RX queues
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_VOID bspMiiTxDoneIntAck(MV_U32 txQBitMask);

/*******************************************************************************
* bspGetPkt
*
* DESCRIPTION:
*       This function should be called to retrieve the arrived frame.
*
* INPUTS:
*       rxQ       - RX queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Linked list of MV_GND_PKT_INFO representing the arrived frame.
*       If the frame is Scatter-Gather, then linked list has two or more nodes.
*       Otherwise, one node.
*       If no arrived packets, NULL is returned.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_GND_PKT_INFO *bspGetPkt(MV_U32 rxQ);

/*******************************************************************************
* bspEthInputHookAdd
*
* DESCRIPTION:
*       This bind the user Rx callback
*
* INPUTS:
*       userRxFunc - the user Rx callback function that is called for every
*                    arrived frame
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthInputHookAdd(BSP_RX_CALLBACK_FUNCPTR userRxFunc);

/*******************************************************************************
* bspEthTxCompleteHookAdd
*
* DESCRIPTION:
*       This bind the user Tx complete callback
*
* INPUTS:
*       userTxFunc - the user Tx callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthTxCompleteHookAdd(BSP_TX_COMPLETE_CALLBACK_FUNCPTR userTxFunc);

/*******************************************************************************
* bspEthCpuCodeToQueue
*
* DESCRIPTION:
*       Binds DSA CPU code to RX queue.
*
* INPUTS:
*       dsaCpuCode - DSA CPU code
*       rxQueue    -  rx queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK if successful, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthCpuCodeToQueue(MV_U32 dsaCpuCode, MV_U32 rxQueue);

/*******************************************************************************
* bspEthPrePendTwoBytesHeaderSet
*
* DESCRIPTION:
*       Enables/Disable pre-pending a two-byte header to all frames forwarded
*       to upper layer (CPSS)
*
* INPUTS:
*       enable = GT_TRUE
*           Two-byte header is pre-pended to frames forwarded to upper layer
*       enable = GT_FALSE
*           Two-byte header is not pre-pended to frames forwarded to upper layer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK if successful, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPrePendTwoBytesHeaderSet(MV_BOOL enable);

/*******************************************************************************
* bspEthPrePendTwoBytesHeaderGet
*
* DESCRIPTION:
*       Retrievs the status of:
*       Enables/Disable pre-pending a two-byte header to all frames forwarded
*       to upper layer (CPSS)
*
* INPUTS:
*       enable = GT_TRUE
*           Two-byte header is pre-pended to frames forwarded to upper layer
*       enable = GT_FALSE
*           Two-byte header is not pre-pended to frames forwarded to upper layer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK if successful, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPrePendTwoBytesHeaderGet(MV_BOOL *pEnable);

/*******************************************************************************
* bspIntConnect
*
* DESCRIPTION:
*       Connect a specified C routine to a specified interrupt vector.
*
* INPUTS:
*       vector    - interrupt vector number to attach to
*       routine   - routine to be called
*       parameter - parameter to be passed to routine
*
* OUTPUTS:
*       None
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntConnect(IN  MV_U32           irqValue,
                        IN  MV_VOIDFUNCPTR   routine,
                        IN  MV_U32           parameter);

/*******************************************************************************
* bspIntEnable
*
* DESCRIPTION:
*       Enable corresponding interrupt bits
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntEnable(MV_U32 irqValue);

/*******************************************************************************
* bspIntDisable
*
* DESCRIPTION:
*       Disable corresponding interrupt bits.
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntDisable(MV_U32 irqValue);

/*******************************************************************************
* bspEthMuxEnable
*
* DESCRIPTION: Enable/disable MUX functionality.
*              When disabled all packets will be sent directly to CPSS.
*
* INPUTS:
*       MV_BOOL enable – when MV_TRUE MUX will be enabled, MV_FALSE – disabled.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthMuxEnable(MV_BOOL enable);

/*******************************************************************************
* bspEthMuxSet
*
* DESCRIPTION: Set bitmap of PP ports. All packets from those ports will be sent to
*              the Linux kernel by MUX. If portsBmp will be equal to zero,
*              all traffic will be sent to CPSS again.
*
* INPUTS:
*       MV_U32 portsBmp – ports bit map.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthMuxSet(MV_U32 portNum, bspEthNetPortType_ENT portType);

/*******************************************************************************
* bspEthMuxGet
*
* DESCRIPTION: Get bitmap of PP ports. All packets from those ports are sent to
*              the Linux kernel by MUX.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       MV_U32 *portsBmp – ports bit map.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*******************************************************************************/
bspEthNetPortType_ENT bspEthMuxGet(MV_U32 portNum,
                                   bspEthNetPortType_ENT *portTypeP);

/*******************************************************************************
* bspCacheDmaMalloc
*
* DESCRIPTION:
*       Allocate a cache free area for DMA devices.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       virtual address of area
*       NULL - per failure to allocate space
*
* COMMENTS:
*       None
*
*******************************************************************************/
void *bspCacheDmaMalloc(IN size_t bytes);

/*******************************************************************************
* bspCacheDmaFree
*
* DESCRIPTION:
*       free a cache free area back to pool.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspCacheDmaFree(void * pBuf);

/*******************************************************************************
* bspHsuMalloc
*
* DESCRIPTION:
*       Allocate a free area for HSU usae.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to allocated data per success
*       NULL - per failure to allocate space
*
* COMMENTS:
*       None
*
*******************************************************************************/
void *bspHsuMalloc(IN size_t bytes);

/*******************************************************************************
* bspHsuFree
*
* DESCRIPTION:
*       free a hsu area back to pool.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspHsuFree(void * pBuf);

/*******************************************************************************
* bspWarmRestart
*
* DESCRIPTION:
*       This routine performs warm restart.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/

MV_STATUS bspWarmRestart
(
    MV_VOID
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pssBspApisH */
