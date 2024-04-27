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
*           This library contains routines to manipulate the timer functions on
*           Marvell 88fxx81 timer with a mostly board-independent interface.
*           This driver provides 3 main functions, system clock support,
*           auxiliary clock support, and timestamp timer support.
*           The timestamp function is always conditional upon the
*           INCLUDE_TIMESTAMP macro.
*
* DEPENDENCIES:
*           None.
*
*******************************************************************************/

/* includes */
#include "config.h"
#include "mvCntmr.h"
#include "vxCntmrIntCtrlInternal.h"
#include "vxCntmrIntCtrl.h"
#include "drv/timer/timerDev.h"
#include "boardenv/mvBoardEnvLib.h"
#include "vxGppIntCtrl.h"
#include "mvSysHwConfig.h"

/* defines */
#ifndef TIMER_INT_ENABLE
#define TIMER_INT_ENABLE(timerNum)  vxCntmrIntEnable(timerNum)
#endif

#ifndef TIMER_INT_DISABLE
#define TIMER_INT_DISABLE(timerNum) vxCntmrIntDisable(timerNum)
#endif

/* locals */
LOCAL FUNCPTR sysClkRoutine     = NULL; /* routine to call on clock interrupt */
LOCAL int sysClkArg             = 0;    /* its argument */
LOCAL int sysClkRunning         = FALSE;
LOCAL int sysClkConnected       = FALSE;
LOCAL int sysClkTicksPerSecond  = 60;

#ifdef INCLUDE_AUXCLK
LOCAL FUNCPTR sysAuxClkRoutine  = NULL;
LOCAL int sysAuxClkArg          = 0;
LOCAL int sysAuxClkRunning      = FALSE;
LOCAL int sysAuxClkTicksPerSecond = 100;
#endif

#ifdef INCLUDE_TIMESTAMP
LOCAL BOOL sysTimestampRunning  = FALSE; /* timestamp running flag */
#endif

/*******************************************************************************
* sysClkInt - interrupt level processing for system clock
*
* This routine handles the system clock interrupt.  It is attached to the
* AHB to MBUS bridge interrupt vector by the routine vxAhb2MbusIntConnect().
*
* RETURNS: N/A.
*/
void sysClkInt(void)
{
    if (sysClkRoutine)
        (*sysClkRoutine)(sysClkArg);
    vxCntmrIntClear(SYS_TIMER_NUM);
}

#ifdef MV_SYS_TIMER_THROUGH_GPIO
MV_VOID mvFiqAckGpioInt(MV_U32 gpioNum);

void sysClkIntGpio(void)
{
    if (sysClkRoutine)
        (*sysClkRoutine)(sysClkArg);
    mvFiqAckGpioInt(MV_SYS_TIMER_THROUGH_GPIO_NUM);
}
#endif

/*******************************************************************************
* sysAuxClkInt - handle an auxiliary clock interrupt
*
* This routine handles the system clock interrupt.  It is attached to the
* AHB to MBUS bridge interrupt vector by the routine vxAhb2MbusIntConnect().
* RETURNS: N/A
*/
#ifdef INCLUDE_AUXCLK
void sysAuxClkInt(void)
{
    if (sysAuxClkRoutine)
        (*sysAuxClkRoutine) (sysAuxClkArg);
    vxCntmrIntClear(AUX_TIMER_NUM);
}
#endif

/*******************************************************************************
* sysClkConnect - connect a routine to the system clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* clock interrupt.  It does not enable system clock interrupts.
* Normally it is called from usrRoot() in usrConfig.c to connect
* usrClock() to the system clock interrupt.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
* SEE ALSO: intConnect(), usrClock(), sysClkEnable()
*/
STATUS sysClkConnect(FUNCPTR isrRoutine, int isrArg)
{
    if (sysClkConnected == FALSE)
    {
        sysHwInit2();  /* XXX for now -- needs to be in usrConfig.c */
        sysClkConnected = TRUE;
    }

    if (!isrRoutine)
        return OK;

    sysClkArg = isrArg;

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    /* set b0 so that sysClkConnect() can be used from shell */
    sysClkRoutine = (FUNCPTR)((UINT32)isrRoutine | 1);
#else
    sysClkRoutine = isrRoutine;
#endif /* CPU_FAMILY == ARM */

    return OK;
}

/*******************************************************************************
* sysAuxClkConnect - connect a routine to the auxiliary clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* auxiliary clock interrupt. It also connects the clock error interrupt
* service routine.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
* SEE ALSO: intConnect(), sysAuxClkEnable()
*/
#ifdef INCLUDE_AUXCLK
STATUS sysAuxClkConnect(FUNCPTR isrRoutine, int isrArg)
{
    if (!isrRoutine)
        return OK;

    sysAuxClkArg = isrArg;

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    /* set b0 so that sysAuxClkConnect() can be used from shell */
    sysAuxClkRoutine    = (FUNCPTR)((UINT32)isrRoutine | 1);
#else
    sysAuxClkRoutine    = isrRoutine;
#endif

    return OK;
}
#endif

/*******************************************************************************
* sysClkDisable - turn off system clock interrupts
*
* This routine disables system clock interrupts.
*
* RETURNS: N/A
* SEE ALSO: sysClkEnable()
*/
void sysClkDisable(void)
{
    if (sysClkRunning)
    {
        mvCntmrDisable(SYS_TIMER_NUM);
        vxCntmrIntDisable(SYS_TIMER_NUM);
        sysClkRunning = FALSE;
    }
}

/*******************************************************************************
* sysAuxClkDisable - turn off auxiliary clock interrupts
* This routine disables auxiliary clock interrupts.
*
* RETURNS: N/A
* SEE ALSO: sysAuxClkEnable()
*/
#ifdef INCLUDE_AUXCLK
void sysAuxClkDisable(void)
{
    if (sysAuxClkRunning)
    {
        mvCntmrDisable(AUX_TIMER_NUM);
        vxCntmrIntDisable(AUX_TIMER_NUM);
        sysAuxClkRunning = FALSE;
    }
}
#endif

/*******************************************************************************
* sysClkEnable - turn on system clock interrupts
*
* This routine enables system clock interrupts.
*
* RETURNS: N/A
* SEE ALSO: sysClkConnect(), sysClkDisable(), sysClkRateSet()
*/
void sysClkEnable(void)
{
    UINT32 tc;
    MV_CNTMR_CTRL cntmrCtrl;

    if (!sysClkRunning)
    {
        /* it may take some ticks to reload the counter
         * so counter value = (clock rate / sysClkTicksPerSecond) - num_ticks
         */
        tc = (SYS_TIMER_CLK / sysClkTicksPerSecond) - AMBA_RELOAD_TICKS;
        cntmrCtrl.enable     = MV_TRUE;
        cntmrCtrl.autoEnable = MV_TRUE;
        mvCntmrStart(SYS_TIMER_NUM, tc, &cntmrCtrl);
        vxCntmrIntEnable(SYS_TIMER_NUM);
        sysClkRunning        = TRUE;
    }
}

/*******************************************************************************
* sysAuxClkEnable - turn on auxiliary clock interrupts
* This routine enables auxiliary clock interrupts.
*
* RETURNS: N/A
* SEE ALSO: sysAuxClkDisable()
*/
#ifdef INCLUDE_AUXCLK
void sysAuxClkEnable(void)
{
    UINT32 tc;
    MV_CNTMR_CTRL cntmrCtrl;

    if (!sysAuxClkRunning)
    {
        /* it may take some ticks to reload the counter
         * so counter value = (clock rate / sysClkTicksPerSecond) - num_ticks
         */
        tc = (AUX_TIMER_CLK / sysAuxClkTicksPerSecond) - AMBA_RELOAD_TICKS;
        cntmrCtrl.enable     = MV_TRUE;
        cntmrCtrl.autoEnable = MV_TRUE;
        mvCntmrStart(AUX_TIMER_NUM, tc, &cntmrCtrl);
        vxCntmrIntEnable(AUX_TIMER_NUM);
        sysAuxClkRunning     = TRUE;
    }
}
#endif

/*******************************************************************************
* sysClkRateGet - get the system clock rate
*
* This routine returns the interrupt rate of the system clock.
*
* RETURNS: The number of ticks per second of the system clock.
* SEE ALSO: sysClkRateSet(), sysClkEnable()
*/
int sysClkRateGet(void)
{
    return sysClkTicksPerSecond;
}

/*******************************************************************************
* sysAuxClkRateGet - get the auxiliary clock rate
*
* This routine returns the interrupt rate of the auxiliary clock.
*
* RETURNS: The number of ticks per second of the auxiliary clock.
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateSet()
*/
#ifdef INCLUDE_AUXCLK
int sysAuxClkRateGet(void)
{
    return sysAuxClkTicksPerSecond;
}
#endif

/*******************************************************************************
* sysClkRateSet - set the system clock rate
*
* This routine sets the interrupt rate of the system clock.  It does not
* enable system clock interrupts unilaterally, but if the system clock is
* currently enabled, the clock is disabled and then re-enabled with the new
* rate. Normally it is called by usrRoot() in usrConfig.c.
*
* RETURNS:
* OK, or ERROR if the tick rate is invalid or the timer cannot be set.
* SEE ALSO: sysClkRateGet(), sysClkEnable()
*/
STATUS sysClkRateSet(int ticksPerSecond)
{
    if (ticksPerSecond < SYS_CLK_RATE_MIN || ticksPerSecond > SYS_CLK_RATE_MAX)
        return ERROR;

    sysClkTicksPerSecond = ticksPerSecond;

    if (sysClkRunning)
    {
        sysClkDisable();
        sysClkEnable();
    }

    return OK;
}

/*******************************************************************************
* sysAuxClkRateSet - set the auxiliary clock rate
*
* This routine sets the interrupt rate of the auxiliary clock.  It does
* not enable auxiliary clock interrupts unilaterally, but if the
* auxiliary clock is currently enabled, the clock is disabled and then
* re-enabled with the new rate.
*
* RETURNS: OK or ERROR.
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateGet()
*/
#ifdef INCLUDE_AUXCLK
STATUS sysAuxClkRateSet(int ticksPerSecond)
{
    if (ticksPerSecond < AUX_CLK_RATE_MIN || ticksPerSecond > AUX_CLK_RATE_MAX)
        return ERROR;

    sysAuxClkTicksPerSecond = ticksPerSecond;

    if (sysAuxClkRunning)
    {
        sysAuxClkDisable();
        sysAuxClkEnable();
    }

    return OK;
}
#endif

#ifdef INCLUDE_TIMESTAMP
/*******************************************************************************
* sysTimestampConnect - connect a user routine to a timestamp timer interrupt
*
* This routine specifies the user interrupt routine to be called at each
* timestamp timer interrupt.
*
* RETURNS: ERROR, always.
*/
STATUS sysTimestampConnect(FUNCPTR isrRoutine, int isrArg)
{
    return ERROR;
}

/*******************************************************************************
* sysTimestampEnable - enable a timestamp timer interrupt
*
* This routine enables timestamp timer interrupts and resets the counter.
*
* RETURNS: OK, always.
* SEE ALSO: sysTimestampDisable()
*/
STATUS sysTimestampEnable(void)
{
   if (sysTimestampRunning)
      return OK;

   if (!sysClkRunning)
      return ERROR;

   sysTimestampRunning = TRUE;
   return OK;
}

/*******************************************************************************
* sysTimestampDisable - disable a timestamp timer interrupt
*
* This routine disables timestamp timer interrupts.
* RETURNS: OK, always.
* SEE ALSO: sysTimestampEnable()
*/
STATUS sysTimestampDisable(void)
{
    if (sysTimestampRunning)
        sysTimestampRunning = FALSE;
    return OK;
}

/*******************************************************************************
* sysTimestampPeriod - get the period of a timestamp timer
*
* This routine gets the period of the timestamp timer, in ticks.  The
* period, or terminal count, is the number of ticks to which the timestamp
* timer counts before rolling over and restarting the counting process.
*
* RETURNS: The period of the timestamp timer in counter ticks.
*/
UINT32 sysTimestampPeriod(void)
{
    return ((UINT32)(SYS_TIMER_CLK / sysClkTicksPerSecond));
}

/*******************************************************************************
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
UINT32 sysTimestampFreq(void)
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
* SEE ALSO: sysTimestampFreq(), sysTimestampLock()
*/
UINT32 sysTimestamp(void)
{
    UINT32 t = mvCntmrRead(SYS_TIMER_NUM);

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
* SEE ALSO: sysTimestampFreq(), sysTimestamp()
*/
UINT32 sysTimestampLock(void)
{
    UINT32 t = mvCntmrRead(SYS_TIMER_NUM);

#if defined (AMBA_TIMER_VALUE_MASK)
    t &= AMBA_TIMER_VALUE_MASK;
#endif

    return (sysTimestampPeriod() - t);
}

#endif /* INCLUDE_TIMESTAMP */

void usrAuxClockDummy()
{
    /*
     * do nothing
     */
}

MV_U32 G_bridgeIntIrqCnt = 0;
MV_U32 G_bridgeIntIrqTimer0Cnt = 0;
MV_U32 G_bridgeIntIrqTimer1Cnt = 0;

#ifdef MV_SYS_TIMER_THROUGH_GPIO
MV_VOID mvEmulateGpioInt(MV_U32 gpioNum);

MV_U32 G_bridgeIntFiqCnt = 0;
MV_U32 G_bridgeIntFiqTimer0Cnt = 0;
MV_U32 G_bridgeIntFiqTimer1Cnt = 0;

MV_VOID mvbridgeIntFiqCntsPrint(MV_VOID)
{
    mvOsPrintf("bridgeIntFiq() entered %d times.\n", G_bridgeIntFiqCnt);
    mvOsPrintf("\tTimer0 handled %d times.\n",       G_bridgeIntFiqTimer0Cnt);
    mvOsPrintf("\tTimer1 handled %d times.\n",       G_bridgeIntFiqTimer1Cnt);
    mvOsPrintf("\n");
    mvOsPrintf("bridgeInt() entered %d times.\n",    G_bridgeIntIrqCnt);
    mvOsPrintf("\tTimer0 handled %d times.\n",       G_bridgeIntIrqTimer0Cnt);
    mvOsPrintf("\tTimer1 handled %d times.\n",       G_bridgeIntIrqTimer1Cnt);
    mvOsPrintf("\n\n");
}

MV_VOID mvbridgeIntFiqCntsInit(MV_VOID)
{
    G_bridgeIntFiqCnt = 0;
    G_bridgeIntFiqTimer0Cnt = 0;
    G_bridgeIntFiqTimer1Cnt = 0;
    G_bridgeIntIrqCnt = 0;
    G_bridgeIntIrqTimer0Cnt = 0;
    G_bridgeIntIrqTimer1Cnt = 0;
}

void bridgeIntFiq(void)
{
    MV_U32 cause = MV_REG_READ(CPU_AHB_MBUS_CAUSE_INT_REG);
    MV_REG_WRITE(CPU_AHB_MBUS_CAUSE_INT_REG, ~cause);

    G_bridgeIntFiqCnt++;

    if (cause & CNTMR_MASK_TC(SYS_TIMER_NUM))
    {
        G_bridgeIntFiqTimer0Cnt++;
        vxCntmrIntClear(SYS_TIMER_NUM);
        mvEmulateGpioInt(MV_SYS_TIMER_THROUGH_GPIO_NUM);
    }
#ifdef INCLUDE_AUXCLK
    else if (cause & CNTMR_MASK_TC(AUX_TIMER_NUM))
    {
        G_bridgeIntFiqTimer1Cnt++;
        sysAuxClkInt();
    }
#endif
}
#endif

void bridgeInt(void)
{
    MV_U32 cause = MV_REG_READ(CPU_AHB_MBUS_CAUSE_INT_REG);
    MV_REG_WRITE(CPU_AHB_MBUS_CAUSE_INT_REG, ~cause);

    G_bridgeIntIrqCnt++;

    if (cause & CNTMR_MASK_TC(SYS_TIMER_NUM))
    {
        G_bridgeIntIrqTimer0Cnt++;
        sysClkInt();
    }
#ifdef INCLUDE_AUXCLK
    else if (cause & CNTMR_MASK_TC(AUX_TIMER_NUM))
    {
        G_bridgeIntIrqTimer1Cnt++;
        sysAuxClkInt();
    }
#endif
}

