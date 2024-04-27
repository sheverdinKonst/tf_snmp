/******************************************************************************
 *
 * Name:	sktypes.h
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Define data types for Linux
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
 
#ifndef __INC_SKTYPES_H
#define __INC_SKTYPES_H

#define SK_I8   char    			/* 8 bits (1 byte) signed       */
#define SK_U8   unsigned char    	/* 8 bits (1 byte) unsigned     */
#define SK_I16  short    			/* 16 bits (2 bytes) signed     */
#define SK_U16  unsigned short    	/* 16 bits (2 bytes) unsigned   */
#define SK_I32  int				    /* 32 bits (4 bytes) signed     */
#define SK_U32  unsigned int    	/* 32 bits (4 bytes) unsigned   */
#define SK_I64  long long		    /* 64 bits (8 bytes) signed     */
#define SK_U64  unsigned long long  /* 64 bits (8 bytes) unsigned   */

#define SK_UPTR	unsigned long  /* casting pointer <-> integral */

#define SK_BOOL   SK_U8
#define SK_FALSE  0
#define SK_TRUE   (!SK_FALSE)

#endif	/* __INC_SKTYPES_H */

/*******************************************************************************
 *
 * End of file
 *
 ******************************************************************************/
