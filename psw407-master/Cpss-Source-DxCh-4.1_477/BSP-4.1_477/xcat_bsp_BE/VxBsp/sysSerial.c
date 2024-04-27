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
* sysSerial.c - BSP serial device initialization
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

/* includes */
#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "config.h"
#include "sysLib.h"
#if defined(MV645xx)
#include "db64560.h"
#elif defined(MV646xx)
#include "db64660.h"
#elif defined(MV78XX0) 
#include "db78xx0.h"
#elif defined(MV88F6XXX)
	#include "db88F6281.h"
#elif   defined (MV88F6183)
	#include "db88F6183.h"
#endif
#include "drv/sio/ns16552Sio.h"


#ifdef __cplusplus
extern "C" {
#endif

/* defines  */
#define UART_REG(reg,chan) \
		(devParas[chan].baseAdrs + (reg * RS232_REG_INTERVAL))

/* typedefs */

/* device INIT structs */

typedef struct {
    void  *pVector;		/* Interrupt vector */
    ULONG  baseAdrs;	/* Register base address */
    UINT32 intLevel;	/* Interrupt level */
} NS16550_CHAN_PARAS;

/* locals   */
LOCAL NS16550_CHAN  ns16550Chan[NUM_TTY];

LOCAL NS16550_CHAN_PARAS devParas[] = {
    { RS232_INT_A_VEC, RS232_CHAN_A_BASE, RS232_INT_A_LVL },
    { RS232_INT_B_VEC, RS232_CHAN_B_BASE, RS232_INT_B_LVL }
};
 

/*******************************************************************************
* sysSerialHwInit - initialize the BSP serial devices to a quiescent state
*
* DESCRIPTION:
*       This routine initializes the BSP serial device descriptors and puts the
*       devices in a quiescent state.  It is called from sysHwInit() with
*       interrupts locked.
*
* INPUT:
*       None.
*
* OUTPUT:
*       See description.
*
* RETURN:
*       None.
*
* SEE ALSO:
*       sysHwInit()
*
*******************************************************************************/
void sysSerialHwInit(void)
{
    int i;

    for(i = 0; i < NUM_TTY; i++)
	{
        ns16550Chan[i].regs	 	= (UINT8 *)devParas[i].baseAdrs;
		ns16550Chan[i].level	= devParas[i].intLevel;
		ns16550Chan[i].ier		= 0;
		ns16550Chan[i].lcr		= 0;
#if (((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR < 2)) || (BSP_VER == 5))
		ns16550Chan[i].pad1		= 0;
#endif
		ns16550Chan[i].channelMode 	= 0;
		ns16550Chan[i].regDelta 	= RS232_REG_INTERVAL;
		ns16550Chan[i].baudRate 	= CONSOLE_BAUD_RATE;
		ns16550Chan[i].xtal 		= RS232_CLOCK;
    
		ns16550DevInit(&ns16550Chan[i]);

        /* setup modem control lines */
        *((UINT8*)UART_REG(MCR,i)) = (UINT8)0x8 ;
    
	}

	return;
}

/*******************************************************************************
* sysSerialHwInit2 - connect BSP serial device interrupts
*
* DESCRIPTION:
*       This routine connects the BSP serial device interrupts.
*       It is called from sysHwInit2().
*
*       Serial device interrupts cannot be connected in sysSerialHwInit()
*       because the kernel memory allocator was not initialized at that point,
*       and intConnect() calls malloc().
*
* INPUT:
*       None.
*
* OUTPUT:
*       See description.
*
* RETURN:
*       None.
*
* SEE ALSO:
*       sysHwInit2()
*
*******************************************************************************/
void sysSerialHwInit2 (void)
{
    int i;

    /* connect serial interrupts */
    for ( i = 0; i < NUM_TTY; i++ )
    {      
		intConnect(devParas[i].pVector, ns16550Int, (int)&ns16550Chan[i]);
	    intEnable(devParas[i].intLevel);
	}

}

/*******************************************************************************
* sysSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* DESCRIPTION:
*       This routine gets the SIO_CHAN device associated with a specified serial
*       channel.
*
* INPUT:
*       int channel - Serial channel
*
* OUTPUT:
*       None.
*
* RETURN:
*       A pointer to the SIO_CHAN structure for the channel,
*       ERROR - if the channel is invalid.
*
*******************************************************************************/

SIO_CHAN * sysSerialChanGet(int channel)
{
    if ( (channel < 0) || (channel >= NUM_TTY) )
        return((SIO_CHAN *)ERROR);

    return((SIO_CHAN *)&ns16550Chan[channel]);
}

/******************************************************************************
*
* sysSerialReset - reset the sio devices to a quiet state
*
* Reset all devices to prevent them from generating interrupts.
*
* This is called from sysToMonitor to shutdown the system gracefully before
* transferring to the boot ROMs.
*
* RETURNS: N/A.
*/

void sysSerialReset (void)
{
    int i;

    for (i = 0; i < NUM_TTY; i++)
	{
	    /* disable serial interrupts */

	    intDisable (devParas[i].intLevel);
	}
}

#ifdef __cplusplus
}
#endif

