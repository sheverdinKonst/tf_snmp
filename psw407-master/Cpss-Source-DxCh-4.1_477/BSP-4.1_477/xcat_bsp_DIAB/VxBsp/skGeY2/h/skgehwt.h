/******************************************************************************
 *
 * Name:	skhwt.h
 * Project:	Gigabit Ethernet Adapters, Event Scheduler Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Defines for the hardware timer functions
 *
 ******************************************************************************/

/******************************************************************************
 *
 *	LICENSE:
 *	(C)Copyright Marvell,
 *	All Rights Reserved
 *	
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MARVELL
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *	
 *	This Module contains Proprietary Information of Marvell
 *	and should be treated as Confidential.
 *	
 *	The information in this file is provided for the exclusive use of
 *	the licensees of Marvell.
 *	Such users have the right to use, modify, and incorporate this code
 *	into products for purposes authorized by the license agreement
 *	provided they include this notice and the associated copyright notice
 *	with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 *	/LICENSE
 *
 ******************************************************************************/

/*
 * SKGEHWT.H	contains all defines and types for the timer functions
 */

#ifndef _SKGEHWT_H_
#define _SKGEHWT_H_

/*
 * SK Hardware Timer
 * - needed wherever the HWT module is used
 * - use in Adapters context name pAC->Hwt
 */
typedef	struct s_Hwt {
	SK_U32		TStart;		/* HWT start */
	SK_U32		TStop;		/* HWT stop */
	SK_BOOL		TActive;	/* HWT: flag : active/inactive */
} SK_HWT;

extern void SkHwtInit(SK_AC *pAC, SK_IOC IoC);
extern void SkHwtStart(SK_AC *pAC, SK_IOC IoC, SK_U32 Time);
extern void SkHwtStop(SK_AC *pAC, SK_IOC IoC);
extern SK_U32 SkHwtRead(SK_AC *pAC, SK_IOC IoC);
extern void SkHwtIsr(SK_AC *pAC, SK_IOC IoC);
#endif /* _SKGEHWT_H_ */

