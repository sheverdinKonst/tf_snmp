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
* mv88fxx81IntrCtl.c - MV88Fxx81 main interrupt controller driver
*
* DESCRIPTION:
*		This module implements the MV88Fxx81 interrupt controller driver.
*
*       The 88F1181 handles interrupts in two stages. The first stage is 
*		specific unit cause and mask registers, that distinguish between a 
*		specific interrupt events within the unit.
*		Once an interrupt event occurs, its corresponding bit in the unit 
*		cause register is set to 1. If the interrupt is not	masked by the unit 
*		mask register, it is marked in the main interrupt cause register. 
*		The unit local mask register has no affect on the setting of interrupt 
*		bits in the unit local cause register. It only affects the setting of 
*		the interrupt bit in the Main Interrupt Cause register
*       NOTE:
*		The Main Interrupt Cause register bits are Read Only. To clear an 
*		interrupt cause, the software needs to clear the active bit(s) in 
*		the specific unit cause register.
*       
*       This driver assumes that interrupt vector numbers are calculated and
*       not the result of a special cycle on the bus.  Vector numbers are
*       generated by adding the current interrupt level number to
*       AMBA_INT_VEC_BASE to generate a vector number which the architecture
*       level will use to invoke the proper handling routine.  If a different
*       mapping scheme, or a special hardware routine is needed, then the BSP
*       should redefine the macro AMBA_INT_LVL_VEC_MAP(level,vector) to
*       override the version defined in this file.
*       
*       Priorities
*       ==========
*       The order of interrupt level priority is undefined at the architecture
*       level.  In this driver, level 0 is highest and and indicates that all
*       levels are disabled; level <AMBA_INT_NUM_LEVELS> is the lowest and
*       indicates that all levels are enabled.
*       
*       By default, this driver implements a least-significant bit first
*       interrupt priority scheme (this is compatible with earlier versions of
*       this driver) which requires a definition of ambaIntLvlMask (see above).
*       If required, the driver can be compiled to implement a BSP-configurable
*       interrupt priority scheme by #defining AMBA_INT_PRIORITY_MAP.  For this
*       model, the BSP should define an array of int called ambaIntLvlPriMap,
*       each element of which is a bit number to be polled.  The list should be
*       terminated with an entry containing -1.  This list is used in the
*       interrupt handler to check bits in the requested order and is also used
*       to generate a map of interrupt source to new interrupt level such that
*       whilst servicing an interrupt, all interrupts defined by the BSP to be
*       of lower priority than that interrupt are disabled.  Interrupt sources
*       not in the list are serviced after all others in least-significant bit
*       first priority.  (Note that the list is a list of ints rather than
*       bytes because it causes the current compiler to generate faster code.)
*       
*       Note that in this priority system, intLevelSet(n) does not necessarily
*       disable interrupt bit n and all lower-priority ones but uses
*       ambaIntLvlPriMap to determine which interrupts should be masked e.g.
*       if ambaIntLvlPriMap[] contains { 9, 4, 8, 5, -1 }, intLevelSet(0)
*       disables all interrupt bits; intLevelSet(1) enables interrupt bit 9 but
*       disables interrupts 4, 8, 5 and all others not listed; intLevelSet(3)
*       enables interrupt bits 9, 4 and 8 but disables all others.  This
*       enabling of interrupts only occurs if the interrupt has been explicitly
*       enabled via a call to ambaIntLvlEnable().
*       
*		If the list is empty (contains just a terminator) or ambaIntLvlPriMap
*		is declared as an int pointer of value 0 (this is more efficient),
*		interrupts are handled as least-significant bit is highest priority.
*
*       The BSP will initialize this driver in sysHwInit2(), after initializing
*       the main interrupt library, usually intLibInit().  The initialization
*       routine, ambaIntDevInit() will setup the interrupt controller device,
*       it will mask off all individual interrupt sources and then set the
*       interrupt level to enable all interrupts.  See ambaIntDevInit for more
*       information.
*       
*       All of the functions in this library are global.  This allows them to
*       be used by the BSP if it is necessary to create wrapper routines or to
*       incorporate several drivers together as one.
*       
* DEPENDENCIES:
*       None.
*
*******************************************************************************/


#include "vxWorks.h"
#include "config.h"
#include "intLib.h"
#ifdef MV_INCLUDE_INTERRUPT_STATISTIC
#include "expandedBSP/Intrrupt.h"
STATUS	intRoutineGet(UINT32 vector,VOIDFUNCPTR *pRoutine,int *pArgument);

/*  Local */
VOID BSP_IntVecNameGet(UINT32 addr, char *symbName);

#endif

IMPORT int ffsLsb (UINT32);


/* Defines from config.h, or <bsp>.h */

#ifndef	AMBA_INT_NUM_LEVELS
#if defined (MV78XX0)
#define	AMBA_INT_NUM_LEVELS	96
#elif defined (MV88F6XXX) 
#define	AMBA_INT_NUM_LEVELS	64
#elif defined (MV88F6183) 
#define	AMBA_INT_NUM_LEVELS	32
#endif
#endif

#define AMBA_INT_VEC_BASE	(0x0)

/* Convert level number to vector number */

#ifndef AMBA_INT_LVL_VEC_MAP
#define AMBA_INT_LVL_VEC_MAP(level, vector) \
	((vector) = ((level) + AMBA_INT_VEC_BASE))
#endif /* AMBA_INT_LVL_VEC_MAP */


#ifndef	AMBA_INT_PRIORITY_MAP
/* Convert pending register value, to a level number */

#ifndef AMBA_INT_PEND_LVL_MAP
#define AMBA_INT_PEND_LVL_MAP(pendReg, level) \
	((level) = (pendReg))
#endif /* AMBA_INT_PEND_LVL_MAP */
#endif /* AMBA_INT_PRIORITY_MAP */


/* driver constants */

#define AMBA_INT_ALL_ENABLED	(AMBA_INT_NUM_LEVELS)
#define AMBA_INT_ALL_DISABLED	0


/* User - Define Cause bit distribution. */
#define CPU_INT_LOW_CAUSE(cause)      (00 <= cause && cause <= 31)
#define CPU_INT_HIGH_CAUSE(cause)     (32 <= cause && cause <= 63)
#if defined (MV78XX0)
#if defined (MV78100)
#define CPU_IRQ_SELECT_CAUSE_REG	CPU_INT_SELECT_CAUSE_REG(0)
#define CPU_IRQ_LOW_CAUSE_REG		CPU_INT_LOW_REG(0)
#define CPU_IRQ_LOW_MASK_REG 		CPU_INT_MASK_LOW_REG(0)
#define CPU_IRQ_HIGH_MASK_REG		CPU_INT_MASK_HIGH_REG(0)
#define CPU_IRQ_HIGH_CAUSE_REG      CPU_INT_HIGH_REG(0)
#define CPU_IRQ_ERROR_REG			CPU_INT_ERROR_REG(0)
#define CPU_IRQ_ERROR_MASK_REG      CPU_INT_MASK_ERROR_REG(0)
#else
#define CPU_IRQ_SELECT_CAUSE_REG	CPU_INT_SELECT_CAUSE_REG(whoAmI())
#define CPU_IRQ_LOW_CAUSE_REG		CPU_INT_LOW_REG(whoAmI())
#define CPU_IRQ_LOW_MASK_REG 		CPU_INT_MASK_LOW_REG(whoAmI())
#define CPU_IRQ_HIGH_MASK_REG		CPU_INT_MASK_HIGH_REG(whoAmI())
#define CPU_IRQ_HIGH_CAUSE_REG      CPU_INT_HIGH_REG(whoAmI())
#define CPU_IRQ_ERROR_REG			CPU_INT_ERROR_REG(whoAmI())
#define CPU_IRQ_ERROR_MASK_REG      CPU_INT_MASK_ERROR_REG(whoAmI())
#endif

#define CPU_INT_ERROR_CAUSE(cause)      (64 <= cause && cause <= 95)

/* Interrupt Select Cause Register (ISCR) bits */
#define ISCR_SEL_HIGH_CAUSE      	0x40000000 /* Bit 30 in little endian */
#define ISCR_SEL_BOTH_MAIN_HIGH  	0x80000000 /* Bit 31 in little endian */


#define ERROR_REGISTER_BIT	(1 << INT_LVL_ERRSUM)
#elif defined (MV88F6XXX) || defined (MV88F6183) 
#define CPU_IRQ_LOW_CAUSE_REG		CPU_MAIN_INT_CAUSE_REG
#define CPU_IRQ_LOW_MASK_REG 		CPU_MAIN_IRQ_MASK_REG
#define CPU_IRQ_HIGH_CAUSE_REG      CPU_MAIN_INT_CAUSE_HIGH_REG
#define CPU_IRQ_HIGH_MASK_REG		CPU_MAIN_IRQ_MASK_HIGH_REG

#define ERROR_REGISTER_BIT	0

#endif
/* Local data */

/* Current interrupt level setting (ambaIntLvlChg). */

LOCAL UINT32 ambaIntLvlCurrent = AMBA_INT_ALL_DISABLED; /* all levels disabled*/

/*
 * A mask word.  Bits are set in this word when a specific level
 * is enabled. It is used to mask off individual levels that have
 * not been explicitly enabled.
 */

LOCAL UINT32 ambaIntLvlEnabled[3];


#ifdef AMBA_INT_PRIORITY_MAP
/*
 * Controller masks: for each interrupt level, this provides a mask for
 * the controller (see IntLvlChg).
 * Mask is 32 bits * (levels + 1)
 */
LOCAL UINT32 ambaIntLvlMaskLow[1 + AMBA_INT_NUM_LEVELS];
#if AMBA_INT_NUM_LEVELS >= 64
LOCAL UINT32 ambaIntLvlMaskHigh[1 + AMBA_INT_NUM_LEVELS];
#endif

#if AMBA_INT_NUM_LEVELS == 96
LOCAL UINT32 ambaIntLvlMaskError[1 + AMBA_INT_NUM_LEVELS];
#endif

/*
 * Map of interrupt bit number to level: if bit n is set, ambaIntLvlMap[n]
 * is the interrupt level to change to such that interrupt n and all lower
 * priority ones are disabled.
 */

LOCAL int ambaIntLvlMap[AMBA_INT_NUM_LEVELS ];
#endif /* AMBA_INT_PRIORITY_MAP */

#ifdef MV_INCLUDE_INTERRUPT_STATISTIC
char *IntLvlName[AMBA_INT_NUM_LEVELS]=
{
	"Error Cause register",
	"SPI interrupt       ",
	"TWSI0 interrupt     ",
	"TWSI1 interrupt     ",
	"IDMA0 interrupt     ",
	"IDMA1 interrupt     ",
	"IDMA2 interrupt     ",
	"IDMA3 interrupt     ",
	"Timer0 interrupt    ",
	"Timer1 interrupt    ",
	"Timer2 interrupt    ",
	"Timer3 interrupt    ",
	"UART0 interrupt     ",
	"UART1 interrupt     ",
	"UART2 interrupt     ",
	"UART3 interrupt     ",
	"USB0 interrupt      ",
	"USB1 interrupt      ",
	"USB2 interrupt      ",
	"Crypto engine       ",
	0,
	0,
	"XOR engine 0        ",
	"XOR engine 1        ",
	0,
	0,
	"SATA interrupt      ",
	0,
	0,
	0,
	0,
	0,
	"PEX port0.0         ",
	"PEX port0.1         ",
	"PEX port0.2         ",
	"PEX port0.3         ",
	"PEX port1.0         ",
	"PEX port1.1         ",
	"PEX port1.2         ",
	"PEX port1.3         ",
	"GE Port 0.0         ",
	"GE Port 0.0 Rx      ",
	"GE Port 0.0 Tx      ",
	"GE Port 0.0 misc.   ",
	"GE Port 0.1         ",
	"GE Port 0.1 Rx      ",
	"GE Port 0.1 Tx      ",
	"GE Port 0.1 misc.   ",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	"GPIO[00:07]         ",
	"GPIO[O8:15]         ",
	"GPIO[16:23]         ",
	"GPIO[24:31]         ",
	"Inbound Doorbell    ",
	"Outbound Doorbell   ",
	0,
	0,
	"Crypto error        ",
	"Device bus error    ",
	"DMA error           ",
	"CPU error           ",
	"PEX port0 Error     ",
	"PEX port1 Error     ",
	"GEthernet error     ",
	0,
	"USB error           ",
	"DRAM ECC error      ",
	"XOR engine error    ",
	0,
	0,
	0,
	0,
	"WD Timer interrupt  ",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};
UINT32 IntLvlStatisticCounter[1 + AMBA_INT_NUM_LEVELS];
UINT32 IntLvlStatisticOverFlow[1 + AMBA_INT_NUM_LEVELS];

static INLINE  unsigned long clz(unsigned long val)
{
	unsigned long ret;
	__asm__ __volatile__(
		"clz %0, %1\n"
		: "=r" (ret)
		: "r" (val)); 
	return ret;
}
#endif
/* forward declarations */

STATUS	ambaIntLvlVecChk (int*, int*);
STATUS  ambaIntLvlVecAck (int, int);
int		ambaIntLvlChg (int);
STATUS	ambaIntLvlEnable (int);
STATUS	ambaIntLvlDisable (int);

/*******************************************************************************
*
* ambaIntDevInit - initialize the interrupt controller
*
* This routine will initialize the interrupt controller device, disabling all
* interrupt sources.  It will also connect the device driver specific routines
* into the architecture level hooks.  If the BSP needs to create a wrapper
* routine around any of the arhitecture level routines, it should install the
* pointer to the wrapper routine after calling this routine.
*
* If used with configurable priorities (#define AMBA_INT_PRIORITY_MAP),
* before this routine is called, ambaIntLvlPriMap should be initialised
* as a list of interrupt bits to poll in order of decreasing priority and
* terminated by an entry containing -1.  If ambaIntLvlPriMap is a null
* pointer (or an empty list), the priority scheme used will be
* least-significant bit first.  This is equivalent to the scheme used if
* AMBA_INT_PRIORITY_MAP is not defined but slightly less efficient.
*
* The return value ERROR indicates that the contents of
* ambaIntLvlPriMap (if used) were invalid.
*
* RETURNS: OK or ERROR if ambaIntLvlPriMap invalid.
*/

int ambaIntDevInit (void)
{
	int i;
#ifdef AMBA_INT_PRIORITY_MAP
	int j;
	int level;
	UINT32 bit;

	/* if priorities are supplied, validate the supplied list */
	/* ambaIntLvlMap is array, ==> never NULL
     * if (ambaIntLvlPriMap != 0)
     */
	{
		/* first check the list is terminated (VecChk requires this) */

		for (i = 0; i < AMBA_INT_NUM_LEVELS && 
                    i < NELEMENTS(ambaIntLvlPriMap); ++i)
			if (ambaIntLvlPriMap[i] == -1)
				break;

		if (!(i < AMBA_INT_NUM_LEVELS))
			return ERROR;	/* no terminator */


		/* now check that all are in range and that there are no duplicates */

		for (i = 0; ambaIntLvlPriMap[i] != -1; ++i)
			if (ambaIntLvlPriMap[i] < 0 ||
				ambaIntLvlPriMap[i] >= AMBA_INT_NUM_LEVELS)
			{
				return ERROR;	/* out of range */
			} else
				for (j = i + 1; ambaIntLvlPriMap[j] != -1; ++j)
					if (ambaIntLvlPriMap[j] == ambaIntLvlPriMap[i])
					{
						return ERROR;	/* duplicate */
					}
	}


	/*
	 * Now initialise the mask array.
	 * For each level (in ascending order), the mask is the mask of the
	 * previous level with the bit for the current level set to enable it.
	 */

	ambaIntLvlMaskLow[0]   = 0;	/* mask for level 0 = all disabled */
#if AMBA_INT_NUM_LEVELS >= 64
	ambaIntLvlMaskHigh[0]  = 0;	/* mask for level 0 = all disabled */
#endif
#if AMBA_INT_NUM_LEVELS == 96
	ambaIntLvlMaskError[0] = 0;	/* mask for level 0 = all disabled */
#endif

	/* do the levels for which priority has been specified */

	level = 1;
	/* ambaIntLvlMap is array, ==> never NULL
     * if (ambaIntLvlPriMap != 0)
     */
	{
		for ( ; level <= AMBA_INT_NUM_LEVELS && 
                level <  NELEMENTS(ambaIntLvlPriMap) && 
			((i = ambaIntLvlPriMap[level - 1]) >= 0); ++level)
		{
			/* copy previous level's mask to this one's */

			ambaIntLvlMaskLow[level] = ambaIntLvlMaskLow[level - 1];
#if AMBA_INT_NUM_LEVELS >= 64
			ambaIntLvlMaskHigh[level] = ambaIntLvlMaskHigh[level - 1];
#endif
#if AMBA_INT_NUM_LEVELS == 96
			ambaIntLvlMaskError[level] = ambaIntLvlMaskError[level - 1];
#endif
			if (i<32)
			{
				ambaIntLvlMaskLow[level] |= 1 << i;
			}
#if 	AMBA_INT_NUM_LEVELS >= 64
			else
			{
				if (i<64)
				{
					ambaIntLvlMaskHigh[level] |= 1 << (i-32);
				}
#if 		AMBA_INT_NUM_LEVELS == 96
				else
				{
					ambaIntLvlMaskError[level] |= 1 << (i-64);
				}
#			endif
			}
#		endif
			/*
			 * set index in level map: to disable this interrupt and all
			 * lower-priority ones, select the level one less than this
			 */

            if (i < NELEMENTS(ambaIntLvlPriMap))
			ambaIntLvlMap[i] = level - 1;
		}
	}

	/* do the rest of the levels */

	i = 0;		/* lowest-numbered interrupt bit */

	for ( ; level <= AMBA_INT_NUM_LEVELS; ++level)
	{
		/* copy previous level's mask to this one's */
		ambaIntLvlMaskLow[level] = ambaIntLvlMaskLow[level - 1];
#if AMBA_INT_NUM_LEVELS >= 64
		ambaIntLvlMaskHigh[level] = ambaIntLvlMaskHigh[level - 1];
#endif
#if AMBA_INT_NUM_LEVELS == 96
		ambaIntLvlMaskError[level] = ambaIntLvlMaskError[level - 1];
#endif

		/* try to find a bit that has not yet been set */

		for ( ; ; ++i)
		{
			if (i < 32)
			{
				bit = 1 << i;
				if ((ambaIntLvlMaskLow[level] & bit) == 0)
				{
					/* this bit not set so put it in the mask */
					ambaIntLvlMaskLow[level] |= bit;
					/*
					 * set index in level map: to disable this interrupt and all
					 * lower-priority ones, select the level one less than this
					 */
					ambaIntLvlMap[i] = level - 1;
					break;
				}
			}
#if AMBA_INT_NUM_LEVELS >= 64
			else if (i<64)
			{
				bit = 1 << (i-32);
				if ((ambaIntLvlMaskHigh[level] & bit) == 0)
				{
					/* this bit not set so put it in the mask */
					ambaIntLvlMaskHigh[level] |= bit;
					/*
					 * set index in level map: to disable this interrupt and all
					 * lower-priority ones, select the level one less than this
					 */
					ambaIntLvlMap[i] = level - 1;
					break;
				}
			}
#endif
#if AMBA_INT_NUM_LEVELS == 96
			else 
			{
				bit = 1 << (i-64);
				if ((ambaIntLvlMaskError[level] & bit) == 0)
				{
					/* this bit not set so put it in the mask */
					ambaIntLvlMaskError[level] |= bit;
					/*
					 * set index in level map: to disable this interrupt and all
					 * lower-priority ones, select the level one less than this
					 */
					ambaIntLvlMap[i] = level - 1;
					break;
				}
			}
#endif
		}
	}
#endif

	/* install the driver routines in the architecture hooks */

	sysIntLvlVecChkRtn  = ambaIntLvlVecChk;
	sysIntLvlVecAckRtn  = ambaIntLvlVecAck;
	sysIntLvlChgRtn     = ambaIntLvlChg;
	sysIntLvlEnableRtn  = ambaIntLvlEnable;
	sysIntLvlDisableRtn = ambaIntLvlDisable;

	ambaIntLvlEnabled[0] = 0;	/* all sources disabled */
	ambaIntLvlEnabled[1] = 0;	/* all sources disabled */
#if defined (MV78XX0)
	ambaIntLvlEnabled[2] = 0;	/* all sources disabled */
#endif
	ambaIntLvlChg (AMBA_INT_ALL_ENABLED); /* enable all levels */
#ifdef MV_INCLUDE_INTERRUPT_STATISTIC
	BSP_ClrIntCnt(-1);
#endif

	return OK;
}

/*******************************************************************************
*
* ambaIntLvlVecChk - check for and return any pending interrupts
*
* This routine interrogates the hardware to determine the highest priority
* interrupt pending.  It returns the vector associated with that interrupt, and
* also the interrupt priority level prior to the interrupt (not the
* level of the interrupt).  The current interrupt priority level is then
* raised to the level of the current interrupt so that only higher priority
* interrupts will be accepted until this interrupt is finished.
*
* This routine must be called with CPU interrupts disabled.
*
* The return value ERROR indicates that no pending interrupt was found and
* that the level and vector values were not returned.
*
* Input:
*   int* pLevel,   ptr to receive old interrupt level
*   int* pVector   ptr to receive current interrupt vector
*
* RETURNS: OK or ERROR if no interrupt is pending.
*/

STATUS  ambaIntLvlVecChk(int* pLevel,  int* pVector)
{
	unsigned int causeAndMaskLow;	/* Read relevant cause */
	int newLevel = 0;
#if AMBA_INT_NUM_LEVELS >=64
	unsigned int causeAndMaskHigh;	/* Read relevant cause */
#endif
#if AMBA_INT_NUM_LEVELS == 96
	unsigned int selectCause;
	unsigned int causeAndMaskErr; /* Read relevant cause */
#endif
#ifdef AMBA_INT_PRIORITY_MAP
	UINT32 *priPtr;
	int bitNum;
#endif
#if defined (MV78XX0)
	/* Read pending interrupt register and mask undefined bits */
	/* Read the Select Cause Register */
	selectCause = MV_REG_READ(CPU_IRQ_SELECT_CAUSE_REG);
	if (selectCause & ISCR_SEL_BOTH_MAIN_HIGH)
	{
		/* There are interrupts both in High and Low Cause Register. */
        /* The Select Cause Register represent by Defalut the        */
        /* Low Cause Register. so in the case of interrupsts in both */
        /* of the cause registers, read straight from the High Cause */ 
        /* Register and read the Low from the Select Cause Register  */
		if (selectCause & ISCR_SEL_HIGH_CAUSE)
		{
			causeAndMaskHigh = selectCause;
			causeAndMaskLow = MV_REG_READ(CPU_IRQ_LOW_CAUSE_REG);
		}
		else
		{
			causeAndMaskLow = selectCause;
			causeAndMaskHigh = MV_REG_READ(CPU_IRQ_HIGH_CAUSE_REG);
		}
	} else
	{
		/* There are active Interrups only in the High Cause Register */
		if (selectCause & ISCR_SEL_HIGH_CAUSE)
		{
			causeAndMaskHigh = selectCause;
			causeAndMaskLow  = 0;
		} else
		{
			causeAndMaskHigh = 0;
			causeAndMaskLow = selectCause;
		}
	}
	if (causeAndMaskLow & ERROR_REGISTER_BIT)
	{
		causeAndMaskLow &=~ERROR_REGISTER_BIT;
		causeAndMaskErr = MV_REG_READ(CPU_IRQ_ERROR_REG);
	} else
	{
		causeAndMaskErr = 0;
	}
	if (causeAndMaskLow)
	{
		causeAndMaskLow &= MV_REG_READ(CPU_IRQ_LOW_MASK_REG);
	}
	if (causeAndMaskHigh)
	{
		causeAndMaskHigh &= MV_REG_READ(CPU_IRQ_HIGH_MASK_REG); 

	}
	if (causeAndMaskErr)
	{
		causeAndMaskErr &= MV_REG_READ(CPU_IRQ_ERROR_MASK_REG);
	}
#elif defined (MV88F6XXX) || defined (MV88F6183) 
	causeAndMaskLow = MV_REG_READ(CPU_MAIN_INT_CAUSE_REG); 
#if AMBA_INT_NUM_LEVELS >= 64
	if (causeAndMaskLow & (1<<INT_LVL_HIGHSUM))
	{
		causeAndMaskHigh = MV_REG_READ(CPU_MAIN_INT_CAUSE_HIGH_REG);
		causeAndMaskLow &=~(1<<INT_LVL_HIGHSUM);
	}
	else
	{
		causeAndMaskHigh = 0;
	}
#endif
	if (causeAndMaskLow)
	{
		causeAndMaskLow &= MV_REG_READ(CPU_IRQ_LOW_MASK_REG);
	}
#if AMBA_INT_NUM_LEVELS >= 64
	if (causeAndMaskHigh)
	{
		causeAndMaskHigh &= MV_REG_READ(CPU_IRQ_HIGH_MASK_REG); 
	}
#endif
#endif


	/* If no interrupt is pending, return ERROR */

	if ((causeAndMaskLow == 0) 
#if AMBA_INT_NUM_LEVELS == 96
	   &&  (causeAndMaskErr  == 0) 
#endif
#if AMBA_INT_NUM_LEVELS >=64
	   && (causeAndMaskHigh == 0)
#endif
		)
		return ERROR;


#ifdef AMBA_INT_PRIORITY_MAP
	priPtr = (UINT32 *)ambaIntLvlPriMap;
  /* service interrupts according to priority specifed in map */
  
  while (bitNum = *priPtr++, bitNum != -1)
  {
    /* bitNum = interrupt bit from priority map */
    if (CPU_INT_LOW_CAUSE(bitNum))
    {
      if (causeAndMaskLow & (1 << bitNum))
      {
        newLevel = ambaIntLvlMap[bitNum];
        break;
      }
    } 
#if AMBA_INT_NUM_LEVELS >=64
    else if (CPU_INT_HIGH_CAUSE(bitNum))
    {
      if (causeAndMaskHigh & (1 << (bitNum - 32)))
      {
        newLevel = ambaIntLvlMap[bitNum];
        break;
      }
    } 
#endif
#if AMBA_INT_NUM_LEVELS == 96
    else
    {
      if (CPU_INT_ERROR_CAUSE(bitNum))
      {
        if (causeAndMaskErr & (1 << (bitNum - 64)))
        {
          newLevel = ambaIntLvlMap[bitNum];
          break;
        }
      }
    }
#endif
  }
  
  
	/*
	 * if priority scan didn't find anything, look for any bit set,
	 * starting with the lowest-numbered bit
	 */

	if (bitNum == -1)
	{
		if (causeAndMaskLow)
		{
			bitNum = ffsLsb (causeAndMaskLow) - 1; /* ffsLsb returns numbers from 1, not 0 */
			/* map the interrupting device to an interrupt level number */
		} 
#if AMBA_INT_NUM_LEVELS >= 64
		else if (causeAndMaskHigh)
		{
			bitNum = 32 + (ffsLsb (causeAndMaskHigh) - 1 ); /* ffsLsb returns numbers from 1, not 0 */
			/* map the interrupting device to an interrupt level number */
		}
#endif
#if AMBA_INT_NUM_LEVELS == 96
		else if (causeAndMaskErr)
		{
			bitNum = 64 + (ffsLsb (causeAndMaskErr) - 1 );	/* ffsLsb returns numbers from 1, not 0 */
		}
#endif
		/* map the interrupting device to an interrupt level number */
		newLevel = ambaIntLvlMap[bitNum];
	}

	/* if no interrupt is pending, return ERROR */

	if (bitNum == -1)
		return ERROR;
#else
	/* find first bit set in ISR, starting from lowest-numbered bit */

	while (1)
	{
		if (causeAndMaskLow)
		{
			newLevel = ffsLsb (causeAndMaskLow);
			if (newLevel)
				break;
		}
#if AMBA_INT_NUM_LEVELS >=64
		if (causeAndMaskHigh)
		{
			newLevel = ffsLsb (causeAndMaskHigh);
			if (newLevel)
			{
				newLevel += 32;
				break;
			}
		}
#endif
#if AMBA_INT_NUM_LEVELS == 96
		if (causeAndMaskErr)
		{
			newLevel = ffsLsb (causeAndMaskErr);
			if (newLevel)
			{
				newLevel += 64;
				break;
			}
		}
#endif
		return ERROR;
	}

	--newLevel;		/* ffsLsb returns numbers from 1, not 0 */


	/* map the interrupting device to an interrupt level number */

	AMBA_INT_PEND_LVL_MAP (newLevel, newLevel);
#endif	/* ifdef AMBA_INT_PRIORITY_MAP */


	/* change to new interrupt level, returning previous level to caller */

	*pLevel = ambaIntLvlChg (newLevel);

#ifdef MV_INCLUDE_INTERRUPT_STATISTIC
	IntLvlStatisticCounter[bitNum]++;
	if (0 == IntLvlStatisticCounter[bitNum])
	{
		IntLvlStatisticOverFlow[bitNum]++; 
	}

#endif /* MV_INCLUDE_INTERRUPT_STATISTIC  */

	/* fetch, or compute the interrupt vector number */

#ifdef AMBA_INT_PRIORITY_MAP
	AMBA_INT_LVL_VEC_MAP (bitNum, *pVector);
#else
	AMBA_INT_LVL_VEC_MAP (newLevel, *pVector);
#endif

	return OK;
}

/*******************************************************************************
*
* ambaIntLvlVecAck - acknowledge the current interrupt
*
* Acknowledge the current interrupt cycle.  The level and vector values are
* those generated during the ambaIntLvlVecChk() routine for this interrupt
* cycle.  The basic action is to reset the current interrupt and return
* the interrupt level to its previous setting.  Note that the AMBA interrupt
* controller does not need an acknowledge cycle.
*
* RETURNS: OK or ERROR if a hardware fault is detected.
* ARGSUSED
*/

STATUS  ambaIntLvlVecAck
    (
    int level,	/* old interrupt level to be restored */
    int vector	/* current interrupt vector, if needed */
    )
    {
    /* restore the previous interrupt level */

    ambaIntLvlChg (level);

    return OK;
    }

/*******************************************************************************
*
* ambaIntLvlChg - change the interrupt level value
*
* This routine implements the overall interrupt setting.  All levels
* up to and including the specifed level are disabled.  All levels above
* the specified level will be enabled, but only if they were specifically
* enabled by the ambaIntLvlEnable() routine.
*
* The specific priority level AMBA_INT_NUM_LEVELS is valid and represents
* all levels enabled.
*
* Input:  int level	 new interrupt level
*
*
*
* RETURNS: Previous interrupt level.
*/

int  ambaIntLvlChg(int level)
{
    int oldLevel;

    oldLevel = ambaIntLvlCurrent;

    if (level >= 0 && level <= AMBA_INT_NUM_LEVELS)
	{
	/* change current interrupt level */
		ambaIntLvlCurrent = level;
	}

    /* Activate the enabled interrupts */
		MV_REG_WRITE (CPU_IRQ_LOW_MASK_REG,
			(ambaIntLvlMaskLow[ambaIntLvlCurrent] & ambaIntLvlEnabled[0]));
#if AMBA_INT_NUM_LEVELS >= 64
		MV_REG_WRITE (CPU_IRQ_HIGH_MASK_REG,
				(ambaIntLvlMaskHigh[ambaIntLvlCurrent] & ambaIntLvlEnabled[1]));
#endif
#if AMBA_INT_NUM_LEVELS == 96
		MV_REG_WRITE (CPU_IRQ_ERROR_MASK_REG,
				(ambaIntLvlMaskError[ambaIntLvlCurrent] & ambaIntLvlEnabled[2]));
#endif
    return oldLevel;
}

/*******************************************************************************
*
* ambaIntLvlEnable - enable a single interrupt level
*
* Enable a specific interrupt level.  The enabled level will be allowed
* to generate an interrupt when the overall interrupt level is set to
* enable interrupts of this priority (as configured by ambaIntLvlPriMap,
* if appropriate).  Without being enabled, the interrupt is blocked
* regardless of the overall interrupt level setting.
*
* Input:
*   int level   level to be enabled
*
* RETURNS: OK or ERROR if the specified level cannot be enabled.
*/

STATUS  ambaIntLvlEnable (int level)
{
    int key;

    if (level < 0 || level >= AMBA_INT_NUM_LEVELS)
		return ERROR;

    /* set bit in enable mask */

    key = intLock ();

	if (level <32)
	{
		ambaIntLvlEnabled[0] |= (1 << level);
	}
	else
	{
		if (level <64)
		{
			ambaIntLvlEnabled[1] |= (1 << (level-32));
		}
#if defined (MV78XX0)
		else
		{
			ambaIntLvlEnabled[2] |= (1 << (level-64));
		}
#endif
	}

    ambaIntLvlChg (-1);	/* reset current mask */
    intUnlock (key);

    return OK;
}

/*******************************************************************************
*
* ambaIntLvlDisable - disable a single interrupt level
*
* Disable a specific interrupt level.  The disabled level is prevented
* from generating an interrupt even if the overall interrupt level is
* set to enable interrupts of this priority (as configured by
* ambaIntLvlPriMap, if appropriate).
*
* Input:
*   int level   level to be disabled
*
* RETURNS: OK or ERROR, if the specified interrupt level cannot
* be disabled.
*/

STATUS  ambaIntLvlDisable(int level)
{
    int key;

    if (level < 0 || level >= AMBA_INT_NUM_LEVELS)
		return ERROR;


    /* clear bit in enable mask */

    key = intLock ();

	if (level <32)
	{
		ambaIntLvlEnabled[0] &= ~(1 << level);
	}
	else
	{
		if (level <64)
		{
			ambaIntLvlEnabled[1] &= ~(1 << (level-32));
		}
#if defined (MV78XX0)
		else
		{
			ambaIntLvlEnabled[2] &= ~(1 << (level-64));
		}
#endif
	}

    ambaIntLvlChg (-1);	/* reset current mask */
    intUnlock (key);

    return OK;
}
#ifdef MV_INCLUDE_INTERRUPT_STATISTIC
/*******************************************************************************
*  BSP_GetIntCnt -  Get interruption statistics counter
*
* DESCRIPTION:
*		Get interruption statistics counter 
*
* INPUT:
*       iVector	interrupt vector
*		pCnt	pointer of counter
*
* OUTPUT:
*       pCnt - interrupt counter 
*
* RETURN:
*       OK.
*******************************************************************************/
INT32 BSP_GetIntCnt (INT32 iVector,  UINT32 * pCnt) 
{
	if (iVector > AMBA_INT_NUM_LEVELS)
	{
		return ERROR;
	}
	*pCnt = IntLvlStatisticCounter[iVector];
	return OK;
}

/*******************************************************************************
*  BSP_ShowIntCnt -  Show statistic counters
*
* DESCRIPTION:
*		Print all interruption statistics counter, 
*		and output the information to control terminal.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*******************************************************************************/
VOID BSP_ShowIntCnt (VOID)  
{
	int Ix;
	for (Ix=0; Ix< AMBA_INT_NUM_LEVELS; Ix++)
	{
		if (NULL == IntLvlName[Ix])
		{
			continue;
		}
		mvOsPrintf("%2d | %s | %6d%c| \n", 
				  Ix,
				  IntLvlName[Ix],
				  IntLvlStatisticCounter [Ix],
				  IntLvlStatisticOverFlow[Ix]?'*':' '); 
	}
	return;
}
/*******************************************************************************
*  BSP_ShowIntVec -  Show interrupt function vector
*
* DESCRIPTION:
*		print all interruption vector functions, and output this information to control terminal 
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK.
*******************************************************************************/
VOID BSP_ShowIntVec (VOID)
{
	int Ix;
    VOIDFUNCPTR pRoutine;
    int Argument;
	char intrName[80];

	mvOsPrintf(" No| Vector Addr| Interrupt Symbol Name      | Argument   | \n");
	mvOsPrintf("---|------------|---------------------------|------------| \n");
	for (Ix=0; Ix< AMBA_INT_NUM_LEVELS; Ix++)
	{
		if (OK == intRoutineGet(Ix,&pRoutine,&Argument))
		{
			BSP_IntVecNameGet((UINT32)*pRoutine, intrName);
			mvOsPrintf("%2d | 0x%08x | %-25s | 0x%08x | \n", 
					  Ix,*pRoutine,intrName, Argument);
		}
	}
	return ;
}


/*******************************************************************************
*  BSP_ClrIntCnt -  Clear  interruption statistics counter
*
* DESCRIPTION:
*		Clear  interruption statistics counter
*
* INPUT:
*       iVector	interrupt vector
*
* OUTPUT:
*       None
*
* RETURN:
*       OK.
*******************************************************************************/
VOID BSP_ClrIntCnt (INT32 iVector)  
{
	if (-1 == iVector)
	{
		memset (IntLvlStatisticCounter ,0,sizeof (IntLvlStatisticCounter ));
		memset (IntLvlStatisticOverFlow,0,sizeof (IntLvlStatisticOverFlow));
		
	}
	else
	{
		IntLvlStatisticCounter [iVector] = 0; 
		IntLvlStatisticOverFlow[iVector] = 0; 
	}
}
#include "sysSymTbl.h"
#include "sysLib.h"

VOID BSP_IntVecNameGet(UINT32 addr, char *symbName)
{
		char 	   *name;       /* pointer to symbol table copy of string */
       SYMBOL_ID   symId;
	   FUNCPTR 	   symbolAddress = 0;

	   *symbName = '\0';	         /* no matching symbol */			
      	if ((_func_symFindSymbol !=(FUNCPTR) NULL) && (sysSymTbl != NULL))
	    {
			if ((* _func_symFindSymbol) (sysSymTbl,  NULL, addr, SYM_MASK_NONE, SYM_MASK_NONE, 
					 &symId) == OK)
	        {
				(* _func_symNameGet) (symId, &name);
				(* _func_symValueGet) (symId, (void **) &symbolAddress); 
				if ((UINT32)symbolAddress == addr)
				{
					strcpy (symbName, name);
				}
				return;
			}
	    }
}
#endif /* MV_INCLUDE_INTERRUPT_STATISTIC */

