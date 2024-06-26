/******************************************************************************* 
*                   Copyright 2003, Marvell Semiconductor Israel LTD.          * 
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      * 
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  * 
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        * 
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     * 
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       * 
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   * 
*                                                                              * 
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, * 
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    * 
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  * 
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL LTD (MSIL).                             * 
********************************************************************************
* sysPciIntCtrl.h - Header File for : System Interrupt controller driver
*
* DESCRIPTION:
*		None.
*
* DEPENDENCIES:
*		None.
*
*******************************************************************************/

#ifndef __INCsysPciIntCtrlh
#define __INCsysPciIntCtrlh

/* includes */


/* defines  */


/* typedefs */


/* sysPciIntCtrl.c API list */
STATUS sysPciIntConnect(int irqValue, VOIDFUNCPTR routine, int parameter);
STATUS sysPciIntEnable (int irqValue);
STATUS sysPciIntDisable(int irqValue);


#endif /* __INCsysPciIntCtrlh */

