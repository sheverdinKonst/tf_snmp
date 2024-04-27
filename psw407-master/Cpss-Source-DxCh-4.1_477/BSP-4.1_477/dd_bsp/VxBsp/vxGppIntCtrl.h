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
* vxGppIntCtrl.h - Header File for : GPP Interrupt Controller driver
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __INCvxGppIntCtrlh
#define __INCvxGppIntCtrlh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */


/* typedefs */


/* vxGppIntCtrl.h API list */
void    vxGppIntCtrlInit (void);
STATUS  vxGppIntConnect  (int cause, VOIDFUNCPTR routine,
                         int parameter, int prio);
STATUS  vxGppIntEnable   (int cause);
STATUS  vxGppIntDisable  (int cause);

/* Interrupt Controller's available handlers */
void  gpp7_0Int (UINT32 group);
void  gpp15_8Int (UINT32 group);
void  gpp23_16Int (UINT32 group);
#if defined (MV88F6XXX) 
void  gpp31_24Int (UINT32 group);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCvxGppIntCtrlh */
