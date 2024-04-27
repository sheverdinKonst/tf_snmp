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
* vxEthErrIntCtrl.h - Header File for : Ethernet Unit interrupt controller
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxEthErrIntCtrllh
#define __INCvxEthErrIntCtrllh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* ETH_ERR_CAUSE is an enumerator that describes the ethernet unit errors.  */
/* the enumerator describes the error cause bits positions in the ethernet  */
/* cause register.                                                          */

typedef enum _ethErrCause
{
    ETH_ERR_CAUSE_START= -1, /* Cause Start boundry                        	  */
	ETH_ERR_SUMMARY		,	/* Ethernet Unit Interrupt Summary				  */
	ETH_PARITY_ERR		,	/* Parity Error									  */
	ETH_ADDR_VIOLATION	,	/* Address Violation							  */
	ETH_ADDR_NO_MATCH	,	/* Address No Match								  */
    ETH_SMI_DONE		,	/* SMI Command Done								  */
	ETH_COUNT_WA		,	/* Counters Wrap Around Indication				  */
	ETH_BIT6_RESERVED	,	/* ETH unit error cause bit 6 reserved			  */
	ETH_INTER_ADDR_ERR	,	/* There is an access to an illegal offset of
                               the internal registers.						  */
	ETH_BIT8_RESERVED	,	/* ETH unit error cause bit 8 reserved			  */
	ETH_PORT0_DP_ERR	,	/* Port 0 Internal data path parity error 		  */
	ETH_PORT1_DP_ERR	,	/* Port 1 Internal data path parity error 		  */
	ETH_PORT2_DP_ERR	,	/* Port 2 Internal data path parity error 		  */
	ETH_TOP_DP_ERR		,	/* G TOP Internal data path parity error 		  */
	ETH_ERR_CAUSE_END   /* Cause End boundry                              	  */
} ETH_ERR_CAUSE;

#define ETH_PORT_DP_ERR(portNum)	(ETH_PORT0_DP_ERR + portNum)	

/* vxEthErrIntCtrl.h API list */
void   vxEthErrIntCtrlInit(void);
STATUS vxEthErrIntConnect (ETH_ERR_CAUSE cause, VOIDFUNCPTR routine, 
						   int parameter, int prio);
STATUS vxEthErrIntEnable  (int port, ETH_ERR_CAUSE cause);
STATUS vxEthErrIntDisable (int port, ETH_ERR_CAUSE cause);
STATUS vxEthErrIntClear	  (int port, ETH_ERR_CAUSE cause);

void ethErrIntHandler(void);       /* Eth error events handler */

#ifdef __cplusplus
}
#endif

#endif /* __INCvxEthErrIntCtrllh */

