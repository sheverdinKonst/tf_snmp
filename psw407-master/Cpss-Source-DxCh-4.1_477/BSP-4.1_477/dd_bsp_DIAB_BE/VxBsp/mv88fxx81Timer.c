/******************************************************************************* 
*                   Copyright 2004, Marvell Semiconductor Israel LTD.          * 
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
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL LTD (MSIL).                             * 
********************************************************************************
* mv88fxx81Timer.c - Marvell 88fxx81 timer library
*
* DESCRIPTION:
*		This library contains routines to manipulate the timer functions on 
*		Marvell 88fxx81 timer with a mostly board-independent interface. 
*		This driver provides 3 main functions, system clock support, 
*		auxiliary clock support, and timestamp timer support.  
*		The timestamp function is always conditional upon the 
*		INCLUDE_TIMESTAMP macro.                              
*
* DEPENDENCIES:
*		None.
*
*******************************************************************************/

/* includes */
#include "config.h"
#include "mvCntmr.h"
#include "vxCntmrIntCtrl.h"
#include "drv/timer/timerDev.h"
#include "boardenv/mvBoardEnvLib.h"

/* defines */
#ifndef TIMER_INT_ENABLE
#define TIMER_INT_ENABLE(timerNum)  vxCntmrIntEnable(timerNum)
#endif

#ifndef TIMER_INT_DISABLE
#define TIMER_INT_DISABLE(timerNum) vxCntmrIntDisable(timerNum)
#endif	   


/* locals */
LOCAL FUNCPTR sysClkRoutine	= NULL; /* routine to call on clock interrupt */
LOCAL int sysClkArg		= 0;    /* its argument */
LOCAL int sysClkRunning		= FALSE;
LOCAL int sysClkConnected	= FALSE;
LOCAL int sysClkTicksPerSecond	= 60;

LOCAL FUNCPTR sysAuxClkRoutine	= NULL;
LOCAL int sysAuxClkArg		= 0;
LOCAL int sysAuxClkRunning	= FALSE;
LOCAL int sysAuxClkTicksPerSecond = 100;

#ifdef	INCLUDE_TIMESTAMP
LOCAL BOOL	sysTimestampRunning  	= FALSE;   /* timestamp running flag */
#endif /* INCLUDE_TIMESTAMP */

#if !defined (SYS_CLK_RATE_MIN) || !defined (SYS_CLK_RATE_MAX) || \
    !defined (AUX_CLK_RATE_MIN) || !defined (AUX_CLK_RATE_MAX) || \
    !defined (SYS_TIMER_CLK) 	|| !defined (AUX_TIMER_CLK) || \
    !defined (AMBA_RELOAD_TICKS)
#error missing #defines in ambaTimer.c.
#endif

/*******************************************************************************
*
* sysClkInt - interrupt level processing for system clock
*
* This routine handles the system clock interrupt.  It is attached to the
* AHB to MBUS bridge interrupt vector by the routine vxAhb2MbusIntConnect().
*
* RETURNS: N/A.
*/

LOCAL void sysClkInt (void)
    {
    /* No need to acknowledge interrupt. The AHB to MBUS bridge interrupt 	*/
	/* handler automatically acknowledge interrupts							*/

    /* If any routine attached via sysClkConnect(), call it */

    if (sysClkRoutine != NULL)
	(* sysClkRoutine) (sysClkArg);
	    /* Acknowledge interrupts */
	vxCntmrIntClear(SYS_TIMER_NUM);
    }

/*******************************************************************************
*
* sysClkConnect - connect a routine to the system clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* clock interrupt.  It does not enable system clock interrupts.
* Normally it is called from usrRoot() in usrConfig.c to connect
* usrClock() to the system clock interrupt.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
*
* SEE ALSO: intConnect(), usrClock(), sysClkEnable()
*/

STATUS sysClkConnect
    (
    FUNCPTR routine,	/* routine to be called at each clock interrupt */
    int arg		/* argument with which to call routine */
    )
{
    if (sysClkConnected == FALSE)
   	{
    	sysHwInit2 ();	/* XXX for now -- needs to be in usrConfig.c */
    	sysClkConnected = TRUE;
   	}

    sysClkRoutine = NULL; /* ensure routine not called with wrong arg */
    sysClkArg	  = arg;

    if (routine == NULL)
	return OK;

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    /* set b0 so that sysClkConnect() can be used from shell */

    sysClkRoutine = (FUNCPTR)((UINT32)routine | 1);
#else
    sysClkRoutine = routine;
#endif /* CPU_FAMILY == ARM */

    return OK;
}

/*******************************************************************************
*
* sysClkDisable - turn off system clock interrupts
*
* This routine disables system clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysClkEnable()
*/

void sysClkDisable (void)
{
    if (sysClkRunning)
	{
	/* Disable timer. Might as well leave it configured for Periodic mode.  */
	mvCntmrDisable(SYS_TIMER_NUM);

	/* Disable the timer interrupt in the Interrupt Controller */
	vxCntmrIntDisable(SYS_TIMER_NUM);
	
	sysClkRunning = FALSE;
	}
}

/*******************************************************************************
*
* sysClkEnable - turn on system clock interrupts
*
* This routine enables system clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysClkConnect(), sysClkDisable(), sysClkRateSet()
*/

void sysClkEnable (void)
{
    UINT32 tc;
	MV_CNTMR_CTRL cntmrCtrl;	/* Marvell Timer/counter control */

    if (!sysClkRunning)
	{
		/*
		 * Calculate the timer value
		 * Note that it may take some ticks to reload the counter
		 * so counter value = (clock rate / sysClkTicksPerSecond) - num_ticks
		 */		
		tc = (SYS_TIMER_CLK / sysClkTicksPerSecond) - AMBA_RELOAD_TICKS;
	
	
		/* Load Timer Reload value into Timer registers, set up in 			*/
		/* periodic mode and enable timer 		  
		  						*/
		cntmrCtrl.enable     = MV_TRUE;		/* start it 					*/
		cntmrCtrl.autoEnable = MV_TRUE;		/* Work in auto mode - Periodic */
		mvCntmrStart(SYS_TIMER_NUM, tc, &cntmrCtrl);
	
		/* Start the timer */
	
		/* enable clock interrupt in interrupt controller */
		vxCntmrIntEnable(SYS_TIMER_NUM);
		
		sysClkRunning = TRUE;
	}
}

/*******************************************************************************
*
* sysClkRateGet - get the system clock rate
*
* This routine returns the interrupt rate of the system clock.
*
* RETURNS: The number of ticks per second of the system clock.
*
* SEE ALSO: sysClkRateSet(), sysClkEnable()
*/

int sysClkRateGet (void)
    {
    return sysClkTicksPerSecond;
    }

/*******************************************************************************
*
* sysClkRateSet - set the system clock rate
*
* This routine sets the interrupt rate of the system clock.  It does not
* enable system clock interrupts unilaterally, but if the system clock is
* currently enabled, the clock is disabled and then re-enabled with the new
* rate.  Normally it is called by usrRoot() in usrConfig.c.
*
* RETURNS:
* OK, or ERROR if the tick rate is invalid or the timer cannot be set.
*
* SEE ALSO: sysClkRateGet(), sysClkEnable()
*/

STATUS sysClkRateSet
    (
    int ticksPerSecond	    /* number of clock interrupts per second */
    )
    {
    if (ticksPerSecond < SYS_CLK_RATE_MIN || ticksPerSecond > SYS_CLK_RATE_MAX)
	return ERROR;

    sysClkTicksPerSecond = ticksPerSecond;

    if (sysClkRunning)
	{
	sysClkDisable ();
	sysClkEnable ();
	}

    return OK;
    }

/*******************************************************************************
*
* sysAuxClkInt - handle an auxiliary clock interrupt
*
* This routine handles the system clock interrupt.  It is attached to the
* AHB to MBUS bridge interrupt vector by the routine vxAhb2MbusIntConnect().
*
* RETURNS: N/A
*/


void sysAuxClkInt (void)
    {
	/* No need to acknowledge interrupt. The AHB to MBUS bridge interrupt 	*/
	/* handler automatically acknowledge interrupts							*/


    /* If any routine attached via sysAuxClkConnect(), call it */

    if (sysAuxClkRoutine != NULL)
	(*sysAuxClkRoutine) (sysAuxClkArg);
    }

/*******************************************************************************
*
* sysAuxClkConnect - connect a routine to the auxiliary clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* auxiliary clock interrupt.  It also connects the clock error interrupt
* service routine.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
*
* SEE ALSO: intConnect(), sysAuxClkEnable()
*/

STATUS sysAuxClkConnect
    (
    FUNCPTR routine,    /* routine called at each aux clock interrupt */
    int arg             /* argument with which to call routine        */
    )
    {
    sysAuxClkRoutine = NULL;	/* ensure routine not called with wrong arg */
    sysAuxClkArg	= arg;

    if (routine == NULL)
	return OK;

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    /* set b0 so that sysAuxClkConnect() can be used from shell */

    sysAuxClkRoutine = (FUNCPTR)((UINT32)routine | 1);
#else
    sysAuxClkRoutine	= routine;
#endif /* CPU_FAMILY == ARM */

    return OK;
    }

/*******************************************************************************
*
* sysAuxClkDisable - turn off auxiliary clock interrupts
*
* This routine disables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkEnable()
*/

void sysAuxClkDisable (void)
{
    if (sysAuxClkRunning)
    {
	
	/* Disable timer. Might as well leave it configured for Periodic mode.  */
	mvCntmrDisable(AUX_TIMER_NUM);
	
	/* Disable the timer interrupt in the Interrupt Controller */
	vxCntmrIntDisable(AUX_TIMER_NUM);

	sysAuxClkRunning = FALSE;
        }
    }

/*******************************************************************************
*
* sysAuxClkEnable - turn on auxiliary clock interrupts
*
* This routine enables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkDisable()
*/

void sysAuxClkEnable (void)
    {
    UINT32 tc;
	MV_CNTMR_CTRL cntmrCtrl;	/* Marvell Timer/counter control */

    if (!sysAuxClkRunning)
	{
		/* Enable Timer/Counter expiration interrupt */
		TIMER_INT_ENABLE(AUX_TIMER_NUM);
        /*
		 * Calculate the timer value
		 * Note that it may take some ticks to reload the counter
		 * so counter value = (clock rate / sysClkTicksPerSecond) - num_ticks
		 */		
		tc = (AUX_TIMER_CLK / sysAuxClkTicksPerSecond) - AMBA_RELOAD_TICKS;

		/* Load Timer Reload value into Timer registers, set up in 			*/
		/* periodic mode and enable timer 									*/
		cntmrCtrl.enable     = MV_TRUE;		/* start it 					*/
		cntmrCtrl.autoEnable = MV_TRUE;		/* Work in auto mode - Periodic */
		mvCntmrStart(AUX_TIMER_NUM, tc, &cntmrCtrl);   
	
		sysAuxClkRunning = TRUE;
	}
}

/*******************************************************************************
*
* sysAuxClkRateGet - get the auxiliary clock rate
*
* This routine returns the interrupt rate of the auxiliary clock.
*
* RETURNS: The number of ticks per second of the auxiliary clock.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateSet()
*/

int sysAuxClkRateGet (void)
    {
    return sysAuxClkTicksPerSecond;
    }

/*******************************************************************************
*
* sysAuxClkRateSet - set the auxiliary clock rate
*
* This routine sets the interrupt rate of the auxiliary clock.  It does
* not enable auxiliary clock interrupts unilaterally, but if the
* auxiliary clock is currently enabled, the clock is disabled and then
* re-enabled with the new rate.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateGet()
*/

STATUS sysAuxClkRateSet
    (
    int ticksPerSecond	    /* number of clock interrupts per second */
    )
    {
    if (ticksPerSecond < AUX_CLK_RATE_MIN || ticksPerSecond > AUX_CLK_RATE_MAX)
	return ERROR;

    sysAuxClkTicksPerSecond = ticksPerSecond;

    if (sysAuxClkRunning)
	{
	sysAuxClkDisable ();
	sysAuxClkEnable ();
	}

    return OK;
    }

#ifdef	INCLUDE_TIMESTAMP

/*******************************************************************************
*
* sysTimestampConnect - connect a user routine to a timestamp timer interrupt
*
* This routine specifies the user interrupt routine to be called at each
* timestamp timer interrupt.
*
* RETURNS: ERROR, always.
*/

STATUS sysTimestampConnect
    (
    FUNCPTR routine,    /* routine called at each timestamp timer interrupt */
    int arg             /* argument with which to call routine */
    )
    {
    return ERROR;
    }

/*******************************************************************************
*
* sysTimestampEnable - enable a timestamp timer interrupt
*
* This routine enables timestamp timer interrupts and resets the counter.
*
* RETURNS: OK, always.
*
* SEE ALSO: sysTimestampDisable()
*/

STATUS sysTimestampEnable (void)
   {
   if (sysTimestampRunning)
      {
      return OK;
      }

   if (!sysClkRunning)          /* timestamp timer is derived from the sysClk */
      return ERROR;

   sysTimestampRunning = TRUE;

   return OK;
   }

/*******************************************************************************
*
* sysTimestampDisable - disable a timestamp timer interrupt
*
* This routine disables timestamp timer interrupts.
*
* RETURNS: OK, always.
*
* SEE ALSO: sysTimestampEnable()
*/

STATUS sysTimestampDisable (void)
    {
    if (sysTimestampRunning)
        sysTimestampRunning = FALSE;

    return OK;
    }

/*******************************************************************************
*
* sysTimestampPeriod - get the period of a timestamp timer
*
* This routine gets the period of the timestamp timer, in ticks.  The
* period, or terminal count, is the number of ticks to which the timestamp
* timer counts before rolling over and restarting the counting process.
*
* RETURNS: The period of the timestamp timer in counter ticks.
*/

UINT32 sysTimestampPeriod (void)
    {
    /*
     * The period of the timestamp depends on the clock rate of the system
     * clock.
     */

    return ((UINT32)(SYS_TIMER_CLK / sysClkTicksPerSecond));
    }

/*******************************************************************************
*
* sysTimestampFreq - get a timestamp timer clock frequency
*
* This routine gets the frequency of the timer clock, in ticks per
* second.  The rate of the timestamp timer is set explicitly by the
* hardware and typically cannot be altered.
*
* NOTE: Because the system clock serves as the timestamp timer,
* the system clock frequency is also the timestamp timer frequency.
*
* RETURNS: The timestamp timer clock frequency, in ticks per second.
*/

UINT32 sysTimestampFreq (void)
    {
    return ((UINT32)SYS_TIMER_CLK);
    }

/*******************************************************************************
*
* sysTimestamp - get a timestamp timer tick count
*
* This routine returns the current value of the timestamp timer tick counter.
* The tick count can be converted to seconds by dividing it by the return of
* sysTimestampFreq().
*
* This routine should be called with interrupts locked.  If interrupts are
* not locked, sysTimestampLock() should be used instead.
*
* RETURNS: The current timestamp timer tick count.
*
* SEE ALSO: sysTimestampFreq(), sysTimestampLock()
*/

UINT32 sysTimestamp (void)
    {
    UINT32 t;

	t = mvCntmrRead(SYS_TIMER_NUM);

#if defined (AMBA_TIMER_VALUE_MASK)
    t &= AMBA_TIMER_VALUE_MASK;
#endif

    return (sysTimestampPeriod() - t);
    }

/*******************************************************************************
*
* sysTimestampLock - lock interrupts and get the timestamp timer tick count
*
* This routine locks interrupts when the tick counter must be stopped
* in order to read it or when two independent counters must be read.
* It then returns the current value of the timestamp timer tick
* counter.
*
* The tick count can be converted to seconds by dividing it by the return of
* sysTimestampFreq().
*
* If interrupts are already locked, sysTimestamp() should be
* used instead.
*
* RETURNS: The current timestamp timer tick count.
*
* SEE ALSO: sysTimestampFreq(), sysTimestamp()
*/

UINT32 sysTimestampLock (void)
    {
    UINT32 t;

	t = mvCntmrRead(SYS_TIMER_NUM);

#if defined (AMBA_TIMER_VALUE_MASK)
    t &= AMBA_TIMER_VALUE_MASK;
#endif

    return (sysTimestampPeriod() - t);
    }

#endif  /* INCLUDE_TIMESTAMP */
