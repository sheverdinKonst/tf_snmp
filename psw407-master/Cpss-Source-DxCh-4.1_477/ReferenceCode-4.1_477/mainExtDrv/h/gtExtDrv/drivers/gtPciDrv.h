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
* gtPciDrv.h
*
* DESCRIPTION:
*       Includes PCI functions wrappers, for PCI device discovering and
*       configuration read/write operation.
*
* DEPENDENCIES:
*       -   System controller.
*       -   BSP.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/
#ifndef __gtPciDrvh
#define __gtPciDrvh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gtExtDrv/os/extDrvOs.h>

/*
 * typedef: enum GT_PCI_INT
 *
 * Description: Enumeration For PCI interrupt lines.
 *
 * Enumerations:
 *      GT_PCI_INT_A - PCI INT# A
 *      GT_PCI_INT_B - PCI INT# B
 *      GT_PCI_INT_C - PCI INT# C
 *      GT_PCI_INT_D - PCI INT# D
 */
typedef enum
{
    GT_PCI_INT_A = 1,
    GT_PCI_INT_B,
    GT_PCI_INT_C,
    GT_PCI_INT_D
}GT_PCI_INT;

/*
 * Typedef: struct GT_EXT_DRV_PCI_MAP_STC
 *
 * Description: The structure reflect PCI mapping for device
 *
 * Fields:
 *      regs    - registers
 *          regs.base - virtual address of registers
 *          regs.size - size of register access window
 *      config  - pci config
 *          config.base - virtual address of pci config registers
 *          config.size - size of register access window
 *      dfx  - pci dfx
 *          dfx.base - virtual address of dfx registers or 0
 *          dfx.size - size of register access window
 *
 *
 */
typedef struct {
    struct {
        GT_UINTPTR  base;
        GT_UINTPTR  size;
    } regs,config,dfx;
} GT_EXT_DRV_PCI_MAP_STC;

/*******************************************************************************
* extDrvPciConfigWriteReg
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
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciConfigWriteReg
(
    IN  GT_U32  busNo,
    IN  GT_U32  devSel,
    IN  GT_U32  funcNo,
    IN  GT_U32  regAddr,
    IN  GT_U32  data
);



/*******************************************************************************
* extDrvPciConfigReadReg
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
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciConfigReadReg
(
    IN  GT_U32  busNo,
    IN  GT_U32  devSel,
    IN  GT_U32  funcNo,
    IN  GT_U32  regAddr,
    OUT GT_U32  *data
);

/*******************************************************************************
* extDrvInitDrv
*
* DESCRIPTION:
*       Open mvPP device,
*       If first client initialize it (set virtual addresses for userspace)
*       Map all to userspace
*
* INPUTS:
*       None
*
* OUTPUTS:
*       isFirstClient
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*       Linux only
*
*******************************************************************************/
GT_STATUS extDrvInitDrv
(
    OUT GT_BOOL  *isFirstClient
);

/*******************************************************************************
* extDrvPciMap
*
* DESCRIPTION:
*       This routine maps PP registers and PCI registers into userspace
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regsSize - registers size (64M for PCI)
*
* OUTPUTS:
*       mapPtr   - The pointer to all mappings
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciMap
(
    IN  GT_U32                  busNo,
    IN  GT_U32                  devSel,
    IN  GT_U32                  funcNo,
    IN  GT_UINTPTR              regsSize,
    OUT GT_EXT_DRV_PCI_MAP_STC  *mapPtr
);

/*******************************************************************************
* extDrvPciFindDev
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
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciFindDev
(
    IN  GT_U16  vendorId,
    IN  GT_U16  devId,
    IN  GT_U32  instance,
    OUT GT_U32  *busNo,
    OUT GT_U32  *devSel,
    OUT GT_U32  *funcNo
);



/*******************************************************************************
* extDrvGetPciIntVec
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
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS extDrvGetPciIntVec
(
    IN  GT_PCI_INT  pciInt,
    OUT void        **intVec
);

/*******************************************************************************
* extDrvGetIntMask
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
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
GT_STATUS extDrvGetIntMask
(
    IN  GT_PCI_INT  pciInt,
    OUT GT_UINTPTR  *intMask
);

/*******************************************************************************
* extDrvPcieGetInterruptNumber
*
* DESCRIPTION:
*       This routine returns interrupt number for PCIe device
*
* INPUTS:
*       busNo      - PCI bus number.
*       devSel     - the device devSel.
*       funcNo     - function number.
*
* OUTPUTS:
*       intNumPtr  - Interrupt number value
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPcieGetInterruptNumber
(
    IN  GT_U32      busNo,
    IN  GT_U32      devSel,
    IN  GT_U32      funcNo,
    OUT GT_U32     *intNumPtr
);

/*******************************************************************************
* extDrvEnableCombinedWrites
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
*       enWrCombine - GT_TRUE enables write requests combining.
*       enRdCombine - GT_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on sucess,
*       GT_NOT_SUPPORTED    - if the controller does not support this feature,
*       GT_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
GT_STATUS extDrvEnableCombinedPciAccess
(
    IN  GT_BOOL     enWrCombine,
    IN  GT_BOOL     enRdCombine
);

/*******************************************************************************
* extDrvPciDoubleWrite
*
* DESCRIPTION:
*        This routine will write a 64-bit data  to given address
*
* INPUTS:
*        address - address to write to
*       word1 - the first half of double word to write (MSW)
*       word2 - the second half of double word to write (LSW)
*
* OUTPUTS:
*      none
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciDoubleWrite
(
    IN  GT_U32 address,
    IN  GT_U32 word1, 
    IN  GT_U32 word2
);

/*******************************************************************************
* extDrvPciDoubleRead
*
* DESCRIPTION:
*        This routine will read a 64-bit data  from given address
*
* INPUTS:
*        address - address to read from
*
* OUTPUTS:
*       dataPtr     -  pointer to the received data.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciDoubleRead
(
    IN  GT_U32  address,
    OUT GT_U64  *dataPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __gtPciDrvh */


