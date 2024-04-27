/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
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
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
/*******************************************************************************
* vxCntmrIntCtrl.h - Header File for : Counter/Timer interrupt controller driver
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxCntmrIntCtrlh
#define __INCvxCntmrIntCtrlh

/* includes */


#ifdef __cplusplus
extern "C" {
#endif

/* defines  */
#if !defined (SYS_CLK_RATE_MIN) || !defined (SYS_CLK_RATE_MAX) || \
    !defined (AUX_CLK_RATE_MIN) || !defined (AUX_CLK_RATE_MAX) || \
    !defined (SYS_TIMER_CLK)    || !defined (AUX_TIMER_CLK)    || \
    !defined (AMBA_RELOAD_TICKS)
#error missing #defines in ambaTimer.c.
#endif

/* vxCntmrIntCtrl.h API list */
void   vxCntmrIntCtrlInit(void);
STATUS vxCntmrIntEnable  (MV_U32 cntmrNum);
STATUS vxCntmrIntDisable (MV_U32 cntmrNum);
STATUS vxCntmrIntClear(MV_U32 cntmrNum);

#ifdef __cplusplus
}
#endif

#endif /* __INCvxCntmrIntCtrlh */

