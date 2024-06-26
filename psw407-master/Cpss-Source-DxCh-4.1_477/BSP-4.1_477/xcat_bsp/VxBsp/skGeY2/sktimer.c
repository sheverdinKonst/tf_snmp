/******************************************************************************
 *
 * Name:	sktimer.c
 * Project:	Gigabit Ethernet Adapters, Event Scheduler Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	High level timer functions.
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
 *	Event queue and dispatcher
 */
#if (defined(DEBUG) || ((!defined(LINT)) && (!defined(SK_SLIM))))
static const char SysKonnectFileId[] =
	"@(#) $Id: sktimer.c,v 2.4 2007/07/31 13:18:42 rschmidt Exp $ (C) Marvell.";
#endif

#include "h/skdrv1st.h"		/* Driver Specific Definitions */
#include "h/skdrv2nd.h"		/* Adapter Control- and Driver specific Def. */

#ifdef __C2MAN__
/*
	Event queue management.

	General Description:

 */
intro()
{}
#endif


/* Forward declaration */
static void timer_done(SK_AC *pAC, SK_IOC IoC, SK_BOOL Restart);


/*
 * Inits the software timer
 *
 * needs to be called during Init level 1.
 */
void SkTimerInit(
SK_AC	*pAC,		/* Adapter Context */
SK_IOC	IoC,		/* I/O Context */
int		Level)		/* Init Level */
{
	switch (Level) {
	case SK_INIT_DATA:
		pAC->Tim.StQueue = 0;
		break;
	case SK_INIT_IO:
		SkHwtInit(pAC, IoC);
		SkTimerDone(pAC, IoC);
		break;
	default:
		break;
	}
}

/*
 * Stops a high level timer
 * - If a timer is not in the queue the function returns normally, too.
 */
void SkTimerStop(
SK_AC		*pAC,		/* Adapter Context */
SK_IOC		IoC,		/* I/O Context */
SK_TIMER	*pTimer)	/* Timer Pointer to be stopped */
{
	SK_TIMER	**ppTimPrev;
	SK_TIMER	*pTm;

	/* remove timer from queue */
	pTimer->TmActive = SK_FALSE;

	if (pAC->Tim.StQueue == pTimer && !pTimer->TmNext) {

		SkHwtStop(pAC, IoC);
	}

	for (ppTimPrev = &pAC->Tim.StQueue; (pTm = *ppTimPrev);
		ppTimPrev = &pTm->TmNext ) {

		if (pTm == pTimer) {
			/*
			 * Timer found in queue
			 * - dequeue it
			 * - correct delta of the next timer
			 */
			*ppTimPrev = pTm->TmNext;

			if (pTm->TmNext) {
				/* correct delta of next timer in queue */
				pTm->TmNext->TmDelta += pTm->TmDelta;
			}
			return;
		}
	}
}

/*
 * Start a high level software timer
 */
void SkTimerStart(
SK_AC		*pAC,		/* Adapter Context */
SK_IOC		IoC,		/* I/O Context */
SK_TIMER	*pTimer,	/* Timer Pointer to be started */
SK_U32		Time,		/* Time Value (in microsec.) */
SK_U32		Class,		/* Event Class for this timer */
SK_U32		Event,		/* Event Value for this timer */
SK_EVPARA	Para)		/* Event Parameter for this timer */
{
	SK_TIMER	**ppTimPrev;
	SK_TIMER	*pTm;
	SK_U32		Delta;

	SkTimerStop(pAC, IoC, pTimer);

	pTimer->TmClass = Class;
	pTimer->TmEvent = Event;
	pTimer->TmPara = Para;
	pTimer->TmActive = SK_TRUE;

	if (!pAC->Tim.StQueue) {
		/* first Timer to be started */
		pAC->Tim.StQueue = pTimer;
		pTimer->TmNext = 0;
		pTimer->TmDelta = Time;

		SkHwtStart(pAC, IoC, Time);

		return;
	}

	/* timer correction */
	timer_done(pAC, IoC, SK_FALSE);

	/* find position in queue */
	Delta = 0;
	for (ppTimPrev = &pAC->Tim.StQueue; (pTm = *ppTimPrev);
		ppTimPrev = &pTm->TmNext ) {

		if (Delta + pTm->TmDelta > Time) {
			/* the timer needs to be inserted here */
			break;
		}
		Delta += pTm->TmDelta;
	}

	/* insert in queue */
	*ppTimPrev = pTimer;
	pTimer->TmNext = pTm;
	pTimer->TmDelta = Time - Delta;

	if (pTm) {
		/* there is a next timer:  correct its Delta value */
		pTm->TmDelta -= pTimer->TmDelta;
	}

	/* restart with first */
	SkHwtStart(pAC, IoC, pAC->Tim.StQueue->TmDelta);
}


void SkTimerDone(
SK_AC	*pAC,		/* Adapter Context */
SK_IOC	IoC)		/* I/O Context */
{
	timer_done(pAC, IoC, SK_TRUE);
}


static void timer_done(
SK_AC	*pAC,		/* Adapter Context */
SK_IOC	IoC,		/* I/O Context */
SK_BOOL	Restart)	/* Do we need to restart the Hardware timer ? */
{
	SK_U32		Delta;
	SK_TIMER	*pTm;
	SK_TIMER	*pTComp;	/* Timer completed now */
	SK_TIMER	**ppLast;	/* Next field of Last timer to be deq */
	int		Done = 0;

	Delta = SkHwtRead(pAC, IoC);

	ppLast = &pAC->Tim.StQueue;
	pTm = pAC->Tim.StQueue;

	while (pTm && !Done) {
		if (Delta >= pTm->TmDelta) {
			/* Timer ran out */
			pTm->TmActive = SK_FALSE;
			Delta -= pTm->TmDelta;
			ppLast = &pTm->TmNext;
			pTm = pTm->TmNext;
		}
		else {
			/* We found the first timer that did not run out */
			pTm->TmDelta -= Delta;
			Delta = 0;
			Done = 1;
		}
	}

	*ppLast = 0;
	/*
	 * pTm points to the first Timer that did not run out.
	 * StQueue points to the first Timer that run out.
	 */

	for (pTComp = pAC->Tim.StQueue; pTComp; pTComp = pTComp->TmNext) {

		SkEventQueue(pAC,pTComp->TmClass, pTComp->TmEvent, pTComp->TmPara);
	}

	/* Set head of timer queue to the first timer that did not run out */
	pAC->Tim.StQueue = pTm;

	if (Restart && pAC->Tim.StQueue) {
		/* Restart HW timer */
		SkHwtStart(pAC, IoC, pAC->Tim.StQueue->TmDelta);
	}
}

/* End of file */
