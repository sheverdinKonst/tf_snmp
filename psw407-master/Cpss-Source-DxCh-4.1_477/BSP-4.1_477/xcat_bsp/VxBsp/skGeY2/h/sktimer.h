/******************************************************************************
 *
 * Name:	sktimer.h
 * Project:	Gigabit Ethernet Adapters, Event Scheduler Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Defines for the timer functions
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
 * SKTIMER.H	contains all defines and types for the timer functions
 */

#ifndef _SKTIMER_H_
#define _SKTIMER_H_

#include "h/skqueue.h"

/*
 * SK timer
 * - needed wherever a timer is used. Put this in your data structure
 *   wherever you want.
 */
typedef struct s_Timer SK_TIMER;

struct s_Timer {
	SK_TIMER	*TmNext;	/* linked list */
	SK_U32		TmClass;	/* Timer Event class */
	SK_U32		TmEvent;	/* Timer Event value */
	SK_EVPARA	TmPara;		/* Timer Event parameter */
	SK_U32		TmDelta;	/* delta time */
	SK_BOOL		TmActive;	/* flag: active/inactive */
};

/*
 * Timer control struct.
 * - use in Adapters context name pAC->Tim
 */
typedef struct s_TimCtrl {
	SK_TIMER	*StQueue;	/* Head of Timer queue */
} SK_TIMCTRL;

extern void SkTimerInit(SK_AC *pAC, SK_IOC IoC, int Level);
extern void SkTimerStop(SK_AC *pAC, SK_IOC IoC, SK_TIMER *pTimer);
extern void SkTimerStart(SK_AC *pAC, SK_IOC IoC, SK_TIMER *pTimer,
	SK_U32 Time, SK_U32 Class, SK_U32 Event, SK_EVPARA Para);
extern void SkTimerDone(SK_AC *pAC, SK_IOC IoC);

#endif /* _SKTIMER_H_ */

