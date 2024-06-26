/******************************************************************************
 *
 * Name:	skgedrv.h
 * Project:	Gigabit Ethernet Adapters, Common Modules
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Interface with the driver
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

#ifndef __INC_SKGEDRV_H_
#define __INC_SKGEDRV_H_

/* defines ********************************************************************/

/*
 * Define the driver events.
 * Usually the events are defined by the destination module.
 * In case of the driver we put the definition of the events here.
 */
#define SK_DRV_PORT_RESET		 1	/* The port needs to be reset */
#define SK_DRV_NET_UP			 2	/* The net is operational */
#define SK_DRV_NET_DOWN			 3	/* The net is down */
#define SK_DRV_SWITCH_SOFT		 4	/* Ports switch with both links connected */
#define SK_DRV_SWITCH_HARD		 5	/* Port switch due to link failure */
#define SK_DRV_RLMT_SEND		 6	/* Send a RLMT packet */
#define SK_DRV_ADAP_FAIL		 7	/* The whole adapter fails */
#define SK_DRV_PORT_FAIL		 8	/* One port fails */
#define SK_DRV_SWITCH_INTERN	 9	/* Port switch by the driver itself */
#define SK_DRV_POWER_DOWN		10	/* Power down mode */
#define SK_DRV_TIMER			11	/* Timer for free use */
#ifdef SK_NO_RLMT
#define SK_DRV_LINK_UP			12	/* Link Up event for driver */
#define SK_DRV_LINK_DOWN		13	/* Link Down event for driver */
#endif
#define SK_DRV_DOWNSHIFT_DET	14	/* Downshift 4-Pair / 2-Pair (YUKON only) */
#define SK_DRV_RX_OVERFLOW		15	/* Receive Overflow */
#define SK_DRV_LIPA_NOT_AN_ABLE	16	/* Link Partner not Auto-Negotiation able */
#define SK_DRV_PEX_LINK_WIDTH	17	/* PEX negotiated Link width not maximum */
#define SK_DRV_TX_UNDERRUN		18	/* Transmit Underrun */

#define SK_DRV_PRIVATE_BASE		100	/* Base for driver private events */
#endif /* __INC_SKGEDRV_H_ */
