/******************************************************************************
 *
 * Name:	skqueue.h
 * Project:	Gigabit Ethernet Adapters, Event Scheduler Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Defines for the Event queue
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
 * SKQUEUE.H	contains all defines and types for the event queue
 */

#ifndef _SKQUEUE_H_
#define _SKQUEUE_H_

/*
 * define the event classes to be served
 */
#define SKGE_DRV	1	/* Driver Event Class */
#define SKGE_RLMT	2	/* RLMT Event Class */
#define SKGE_I2C	3	/* I2C Event Class */
#define SKGE_PNMI	4	/* PNMI Event Class */
#define SKGE_CSUM	5	/* Checksum Event Class */
#define SKGE_HWAC	6	/* Hardware Access Event Class */

#define SKGE_SWT	9	/* Software Timer Event Class */
#define SKGE_LACP	10	/* LACP Aggregation Event Class */
#define SKGE_RSF	11	/* RSF Aggregation Event Class */
#define SKGE_MARKER	12	/* MARKER Aggregation Event Class */
#define SKGE_FD		13	/* FD Distributor Event Class */
#ifdef SK_ASF
#define SKGE_ASF	14	/* ASF Event Class */
#endif

/*
 * define event queue as circular buffer
 */
#define SK_MAX_EVENT	64

/*
 * Parameter union for the Para stuff
 */
typedef	union u_EvPara {
	void	*pParaPtr;	/* Parameter Pointer */
	SK_U64	Para64;		/* Parameter 64bit version */
	SK_U32	Para32[2];	/* Parameter Array of 32bit parameters */
} SK_EVPARA;

/*
 * Event Queue
 *	skqueue.c
 * events are class/value pairs
 *	class	is addressee, e.g. RLMT, PNMI etc.
 *	value	is command, e.g. line state change, ring op change etc.
 */
typedef	struct s_EventElem {
	SK_U32		Class;			/* Event class */
	SK_U32		Event;			/* Event value */
	SK_EVPARA	Para;			/* Event parameter */
} SK_EVENTELEM;

typedef	struct s_Queue {
	SK_EVENTELEM	EvQueue[SK_MAX_EVENT];
	SK_EVENTELEM	*EvPut;
	SK_EVENTELEM	*EvGet;
} SK_QUEUE;

extern void SkEventInit(SK_AC *, SK_IOC, int Level);
extern void SkEventQueue(SK_AC *, SK_U32 Class, SK_U32 Event, SK_EVPARA Para);
extern int  SkEventDispatcher(SK_AC *, SK_IOC);

/* Define Error Numbers and messages */
#define SKERR_Q_E001	(SK_ERRBASE_QUEUE+1)
#define SKERR_Q_E001MSG	"Event queue overflow"
#define SKERR_Q_E002	(SKERR_Q_E001+1)
#define SKERR_Q_E002MSG	"Undefined event class"
#define SKERR_Q_E003	(SKERR_Q_E002+1)
#define SKERR_Q_E003MSG	"Event queued in init level 0"
#define SKERR_Q_E004	(SKERR_Q_E003+1)
#define SKERR_Q_E004MSG	"Error reported from event function (queue blocked)"
#define SKERR_Q_E005	(SKERR_Q_E004+1)
#define SKERR_Q_E005MSG	"Event scheduler called in init level 0 or 1"

#endif /* _SKQUEUE_H_ */

