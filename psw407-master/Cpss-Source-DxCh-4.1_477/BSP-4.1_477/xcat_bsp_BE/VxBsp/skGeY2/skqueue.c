/******************************************************************************
 *
 * Name:	skqueue.c
 * Project:	Gigabit Ethernet Adapters, Event Scheduler Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Management of an event queue.
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
	"@(#) $Id: skqueue.c,v 2.6 2007/07/17 15:31:56 rschmidt Exp $ (C) Marvell.";
#endif

#include "h/skdrv1st.h"		/* Driver Specific Definitions */
#include "h/skqueue.h"		/* Queue Definitions */
#include "h/skdrv2nd.h"		/* Adapter Control- and Driver specific Def. */

#ifdef __C2MAN__
/*
	Event queue management.

	General Description:

 */
intro()
{}
#endif

/******************************************************************************
 *
 *	SkEventInit() - init event queue management
 *
 * Description:
 *	This function initializes event queue management.
 *	It must be called during Init Level 0.
 *
 * Returns:
 *	nothing
 */
void	SkEventInit(
SK_AC	*pAC,	/* Adapter Context */
SK_IOC	Ioc,	/* I/O Context */
int		Level)	/* Init Level */
{
	switch (Level) {
	case SK_INIT_DATA:
		pAC->Event.EvPut = pAC->Event.EvGet = pAC->Event.EvQueue;
		break;
	default:
		break;
	}
}

/******************************************************************************
 *
 *	SkEventQueue() - add event to queue
 *
 * Description:
 *	This function adds an event to the event queue.
 *	At least Init Level 1 is required to queue events,
 *	but will be scheduled at Init Level 2.
 *
 * returns:
 *	nothing
 */
void	SkEventQueue(
SK_AC		*pAC,	/* Adapter Context */
SK_U32		Class,	/* Event Class */
SK_U32		Event,	/* Event to be queued */
SK_EVPARA	Para)	/* Event Parameter */
{

	if (pAC->GIni.GILevel == SK_INIT_DATA) {
		SK_ERR_LOG(pAC, SK_ERRCL_NORES, SKERR_Q_E003, SKERR_Q_E003MSG);
		return;
	}

	pAC->Event.EvPut->Class = Class;
	pAC->Event.EvPut->Event = Event;
	pAC->Event.EvPut->Para = Para;

	if (++pAC->Event.EvPut == &pAC->Event.EvQueue[SK_MAX_EVENT]) {
		pAC->Event.EvPut = pAC->Event.EvQueue;
	}

	if (pAC->Event.EvPut == pAC->Event.EvGet) {
		SK_ERR_LOG(pAC, SK_ERRCL_NORES, SKERR_Q_E001, SKERR_Q_E001MSG);
	}
}

/******************************************************************************
 *
 *	SkEventDispatcher() - Event Dispatcher
 *
 * Description:
 *	The event dispatcher performs the following operations:
 *		while event queue is not empty
 *			- get event from queue
 *			- send event to state machine
 *
 * CAUTION:
 *	The event functions MUST report an error if performing a reinitialization
 *	of the event queue, e.g. performing Init Level 0..2 while in dispatcher call!
 *	ANY OTHER return value delays scheduling the other events in the queue.
 *	In this case the event blocks the queue until the error condition is cleared!
 *
 * Returns:
 *	The return value error reported by individual event function
 */
int	SkEventDispatcher(
SK_AC	*pAC,	/* Adapter Context */
SK_IOC	Ioc)	/* I/O Context */
{
	SK_EVENTELEM	*pEv;	/* pointer into queue */
	SK_U32			Class;
	int			Rtv;

	if (pAC->GIni.GILevel != SK_INIT_RUN) {
		SK_ERR_LOG(pAC, SK_ERRCL_NORES, SKERR_Q_E005, SKERR_Q_E005MSG);
	}

	pEv = pAC->Event.EvGet;

	SK_DBG_MSG(pAC, SK_DBGMOD_HWM, SK_DBGCAT_QUEUE,
		("Dispatch get 0x%x, put 0x%x\n", pEv, pAC->Event.EvPut));

	while (pEv != pAC->Event.EvPut) {
		SK_DBG_MSG(pAC, SK_DBGMOD_HWM, SK_DBGCAT_QUEUE,
			("Dispatch Class %d, Event %d\n", pEv->Class, pEv->Event));

		switch (Class = pEv->Class) {
#ifndef SK_USE_LAC_EV
#ifndef SK_SLIM
		case SKGE_RLMT:		/* RLMT Event */
			Rtv = SkRlmtEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
		case SKGE_I2C:		/* I2C Event */
			Rtv = SkI2cEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;

#ifdef SK_PNMI_SUPPORT
		case SKGE_PNMI:		/* PNMI Event */
			Rtv = SkPnmiEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
#endif /* SK_PNMI_SUPPORT */

#endif	/* not SK_SLIM */
#endif	/* not SK_USE_LAC_EV */
		case SKGE_DRV:		/* Driver Event */
			Rtv = SkDrvEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
#ifndef SK_USE_SW_TIMER
		case SKGE_HWAC:
			Rtv = SkGeSirqEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
#else /* !SK_USE_SW_TIMER */
		case SKGE_SWT :
			Rtv = SkSwtEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
#endif /* !SK_USE_SW_TIMER */
#if defined(SK_USE_LAC_EV) || defined(SK_LBFO)
		case SKGE_LACP :
			Rtv = SkLacpEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
		case SKGE_RSF :
			Rtv = SkRsfEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
		case SKGE_MARKER :
			Rtv = SkMarkerEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
		case SKGE_FD :
			Rtv = SkFdEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
#endif /* SK_USE_LAC_EV */
#ifdef SK_ASF
		case SKGE_ASF :
			Rtv = SkAsfEvent(pAC,Ioc,pEv->Event,pEv->Para);
			break ;
#endif
#ifdef	SK_USE_CSUM
		case SKGE_CSUM :
			Rtv = SkCsEvent(pAC, Ioc, pEv->Event, pEv->Para);
			break;
#endif	/* SK_USE_CSUM */
		default :

#ifdef SK_PNMI_SUPPORT
			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_Q_E002, SKERR_Q_E002MSG);
#endif /* SK_PNMI_SUPPORT */

			Rtv = 0;
		}

		if (Rtv != 0) {
			/*
			 * Special Case: See CAUTION statement above.
			 * We assume the event queue is reset.
			 */
			if (pAC->Event.EvGet != pAC->Event.EvQueue &&
				pAC->Event.EvGet != pEv) {
				/*
				 * Create an error log entry if the event queue isn't reset.
				 * In this case it may be blocked.
				 */
				SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_Q_E004, SKERR_Q_E004MSG);
			}

			return(Rtv);
		}

		if (++pEv == &pAC->Event.EvQueue[SK_MAX_EVENT]) {

			pEv = pAC->Event.EvQueue;
		}

		/* Renew get: it is used in queue_events to detect overruns */
		pAC->Event.EvGet = pEv;
	}

	return(0);
}

/* End of file */
