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
/*
 * vxCntmrIntCtrlInternal.h - it's not interface for timer
 */

#ifndef __INCvxCntmrIntCtrlInternalh
#define __INCvxCntmrIntCtrlInternalh

#if !defined (SYS_CLK_RATE_MIN) || !defined (SYS_CLK_RATE_MAX) || \
    !defined (AUX_CLK_RATE_MIN) || !defined (AUX_CLK_RATE_MAX) || \
    !defined (SYS_TIMER_CLK)    || !defined (AUX_TIMER_CLK)    || \
    !defined (AMBA_RELOAD_TICKS)
#error missing #defines in ambaTimer.c.
#endif

#define INVALID_CNTMR(cntmrNum)    ((cntmrNum) >= MV_CNTMR_MAX_COUNTER)

/*
 * Timers bits
 */
#if defined(MV78XX0)
#define CNTMR_MASK_TC(counter)        ((counter) < 2 ? (BIT1 << (counter)) \
                                                     : (BIT6 << (counter - 2)))
#define CNTMR_MASK_CAUSE_TC(counter)  (BIT0 << (counter + INT_LVL_TIMER0))
#define CPU_MAIN_IRQ_MASK_REG         CPU_INT_MASK_LOW_REG(whoAmI())
#define CPU_MAIN_IRQ_MASK_HIGH_REG    CPU_INT_MASK_HIGH_REG(whoAmI())

#elif defined (MV88F6XXX) || defined (MV88F6183)
#define CNTMR_MASK_TC(counter)        (BIT1 << (counter))
#define CNTMR_MASK_CAUSE_TC(counter)  (BIT0 << INT_LVL_BRIDGE)
#endif

#endif /* __INCvxCntmrIntCtrlInternalh */

