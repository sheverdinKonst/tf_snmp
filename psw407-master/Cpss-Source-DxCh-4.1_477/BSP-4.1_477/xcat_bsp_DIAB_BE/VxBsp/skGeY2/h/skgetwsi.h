/******************************************************************************
 *
 * Name:	skgetwsi.h
 * Project:	Gigabit Ethernet Adapters, TWSI-Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Special defines for TWSI
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
 * SKGETWSI.H	contains all SK-98xx specific defines for the TWSI handling
 */

#ifndef _INC_SKGETWSI_H_
#define _INC_SKGETWSI_H_

/*
 * Macros to access the B2_I2C_CTRL
 */
#define SK_I2C_CTL(IoC, flag, dev, dev_size, reg, burst) \
	SK_OUT32(IoC, B2_I2C_CTRL,\
		((flag) ? 0x80000000UL : 0L) | \
		(((SK_U32)(reg) << 16) & I2C_ADDR) | \
		(((SK_U32)(dev) << 9) & I2C_DEV_SEL) | \
		((dev_size) & I2C_DEV_SIZE) | \
		((burst) ? I2C_BURST_LEN : 0))

#define SK_I2C_STOP(IoC) {			\
	SK_U32	I2cCtrl;				\
	SK_IN32(IoC, B2_I2C_CTRL, &I2cCtrl);	\
	SK_OUT32(IoC, B2_I2C_CTRL, I2cCtrl | I2C_STOP);	\
}

#define SK_I2C_GET_CTL(IoC, pI2cCtrl)	SK_IN32(IoC, B2_I2C_CTRL, pI2cCtrl)

/*
 * Macros to access the TWSI SW Registers
 */
#define SK_I2C_SET_BIT(IoC, SetBits) {			\
	SK_U8	OrgBits;				\
	SK_IN8(IoC, B2_I2C_SW, &OrgBits);		\
	SK_OUT8(IoC, B2_I2C_SW, OrgBits | (SK_U8)(SetBits));	\
}

#define SK_I2C_CLR_BIT(IoC, ClrBits) {			\
	SK_U8	OrgBits;				\
	SK_IN8(IoC, B2_I2C_SW, &OrgBits);		\
	SK_OUT8(IoC, B2_I2C_SW, OrgBits & ~((SK_U8)(ClrBits)));	\
}

#define SK_I2C_GET_SW(IoC, pI2cSw)	SK_IN8(IoC, B2_I2C_SW, pI2cSw)

/*
 * define the possible sensor states
 */
#define SK_SEN_IDLE		0	/* Idle: sensor not read */
#define SK_SEN_VALUE	1	/* Value Read cycle */
#define SK_SEN_VALEXT	2	/* Extended Value Read cycle */

/*
 * Conversion factor to convert read Voltage sensor to milli Volt
 * Conversion factor to convert read Temperature sensor to 10th degree Celsius
 */
#define SK_LM80_VT_LSB		22	/* 22mV LSB resolution */
#define SK_LM80_TEMP_LSB	10	/* 1 degree LSB resolution */
#define SK_LM80_TEMPEXT_LSB	 5	/* 0.5 degree LSB resolution for ext. val. */

/*
 * formula: counter = (22500*60)/(rpm * divisor * pulses/2)
 * assuming: 6500 rpm, 4 pulses, divisor 1
 */
#define SK_LM80_FAN_FAKTOR	((22500L*60)/(1*2))

/*
 * Define sensor management data
 * Maximum is reached on Genesis copper dual port and Yukon-64
 * Board specific maximum is in pAC->I2c.MaxSens
 */
#define SK_MAX_SENSORS	8	/* maximal no. of installed sensors */
#define SK_MIN_SENSORS	5	/* minimal no. of installed sensors */

/*
 * To watch the state machine (SM) use the timer in two ways
 * instead of one as hitherto
 */
#define SK_TIMER_WATCH_SM		0	/* Watch the SM to finish in a spec. time */
#define SK_TIMER_NEW_GAUGING	1	/* Start a new gauging when timer expires */

/*
 * Defines for the individual thresholds
 */

#define C_PLUS_20		120 / 100
#define C_PLUS_15		115 / 100
#define C_PLUS_10		110 / 100
#define C_PLUS_5		105 / 100
#define C_MINUS_5		 95 / 100
#define C_MINUS_10		 90 / 100
#define C_MINUS_15		 85 / 100

/* Temperature sensor */
#define SK_SEN_TEMP_HIGH_ERR	800	/* Temperature High Err  Threshold */
#define SK_SEN_TEMP_HIGH_WARN	700	/* Temperature High Warn Threshold */
#define SK_SEN_TEMP_LOW_WARN	100	/* Temperature Low  Warn Threshold */
#define SK_SEN_TEMP_LOW_ERR		  0	/* Temperature Low  Err  Threshold */

/* VCC which should be 5 V */
#define SK_SEN_PCI_5V_HIGH_ERR		5588	/* Voltage PCI High Err  Threshold */
#define SK_SEN_PCI_5V_HIGH_WARN		5346	/* Voltage PCI High Warn Threshold */
#define SK_SEN_PCI_5V_LOW_WARN		4664	/* Voltage PCI Low  Warn Threshold */
#define SK_SEN_PCI_5V_LOW_ERR		4422	/* Voltage PCI Low  Err  Threshold */

/*
 * VIO may be 5 V or 3.3 V. Initialization takes two parts:
 * 1. Initialize lowest lower limit and highest higher limit.
 * 2. After the first value is read correct the upper or the lower limit to
 *    the appropriate C constant.
 *
 * Warning limits are +-5% of the exepected voltage.
 * Error limits are +-10% of the expected voltage.
 */

/* Bug fix AF: 16.Aug.2001: Correct the init base of LM80 sensor */

#define SK_SEN_PCI_IO_5V_HIGH_ERR	5566	/* + 10% V PCI-IO High Err Threshold */
#define SK_SEN_PCI_IO_5V_HIGH_WARN	5324	/* +  5% V PCI-IO High Warn Threshold */
					/*		5000	mVolt */
#define SK_SEN_PCI_IO_5V_LOW_WARN	4686	/* -  5% V PCI-IO Low Warn Threshold */
#define SK_SEN_PCI_IO_5V_LOW_ERR	4444	/* - 10% V PCI-IO Low Err Threshold */

#define SK_SEN_PCI_IO_RANGE_LIMITER	4000	/* 4000 mV range delimiter */

/* correction values for the second pass */
#define SK_SEN_PCI_IO_3V3_HIGH_ERR	3850	/* + 15% V PCI-IO High Err Threshold */
#define SK_SEN_PCI_IO_3V3_HIGH_WARN	3674	/* + 10% V PCI-IO High Warn Threshold */
					/*		3300	mVolt */
#define SK_SEN_PCI_IO_3V3_LOW_WARN	2926	/* - 10% V PCI-IO Low Warn Threshold */
#define SK_SEN_PCI_IO_3V3_LOW_ERR	2772	/* - 15% V PCI-IO Low Err  Threshold */

/*
 * VDD voltage
 */
#define SK_SEN_VDD_HIGH_ERR		3630	/* Voltage ASIC High Err  Threshold */
#define SK_SEN_VDD_HIGH_WARN	3476	/* Voltage ASIC High Warn Threshold */
#define SK_SEN_VDD_LOW_WARN		3146	/* Voltage ASIC Low  Warn Threshold */
#define SK_SEN_VDD_LOW_ERR		2970	/* Voltage ASIC Low  Err  Threshold */

/*
 * PHY PLL 3V3 voltage
 */
#define SK_SEN_PLL_3V3_HIGH_ERR		3630	/* Voltage PMA High Err  Threshold */
#define SK_SEN_PLL_3V3_HIGH_WARN	3476	/* Voltage PMA High Warn Threshold */
#define SK_SEN_PLL_3V3_LOW_WARN		3146	/* Voltage PMA Low  Warn Threshold */
#define SK_SEN_PLL_3V3_LOW_ERR		2970	/* Voltage PMA Low  Err  Threshold */

/*
 * VAUX (YUKON only)
 */
#define SK_SEN_VAUX_3V3_VAL		3300	/* Voltage VAUX 3.3 Volt */

#define SK_SEN_VAUX_3V3_HIGH_ERR	(SK_I32)(SK_SEN_VAUX_3V3_VAL * C_PLUS_10)
#define SK_SEN_VAUX_3V3_HIGH_WARN	(SK_I32)(SK_SEN_VAUX_3V3_VAL * C_PLUS_5)
#define SK_SEN_VAUX_3V3_LOW_WARN	(SK_I32)(SK_SEN_VAUX_3V3_VAL * C_MINUS_5)
#define SK_SEN_VAUX_3V3_LOW_ERR		(SK_I32)(SK_SEN_VAUX_3V3_VAL * C_MINUS_10)

#define SK_SEN_VAUX_RANGE_LIMITER	1000	/* 1000 mV range delimiter */

/*
 * PHY 2V5 voltage
 */
#define SK_SEN_PHY_2V5_VAL		2500	/* Voltage PHY 2.5 Volt */

#define SK_SEN_PHY_2V5_HIGH_ERR		(SK_I32)(SK_SEN_PHY_2V5_VAL * C_PLUS_10)
#define SK_SEN_PHY_2V5_HIGH_WARN	(SK_I32)(SK_SEN_PHY_2V5_VAL * C_PLUS_5)
#define SK_SEN_PHY_2V5_LOW_WARN		(SK_I32)(SK_SEN_PHY_2V5_VAL * C_MINUS_5)
#define SK_SEN_PHY_2V5_LOW_ERR		(SK_I32)(SK_SEN_PHY_2V5_VAL * C_MINUS_10)

/*
 * ASIC Core 1V5 voltage (YUKON only)
 */
#define SK_SEN_CORE_1V5_VAL		1500	/* Voltage ASIC Core 1.5 Volt */

#define SK_SEN_CORE_1V5_HIGH_ERR	(SK_I32)(SK_SEN_CORE_1V5_VAL * C_PLUS_10)
#define SK_SEN_CORE_1V5_HIGH_WARN	(SK_I32)(SK_SEN_CORE_1V5_VAL * C_PLUS_5)
#define SK_SEN_CORE_1V5_LOW_WARN	(SK_I32)(SK_SEN_CORE_1V5_VAL * C_MINUS_5)
#define SK_SEN_CORE_1V5_LOW_ERR		(SK_I32)(SK_SEN_CORE_1V5_VAL * C_MINUS_10)

/*
 * ASIC Core 1V2 (1V3) voltage (YUKON-2 only)
 */
#define SK_SEN_CORE_1V2_VAL		1200	/* Voltage ASIC Core 1.2 Volt */

#define SK_SEN_CORE_1V2_HIGH_ERR	(SK_I32)(SK_SEN_CORE_1V2_VAL * C_PLUS_20)
#define SK_SEN_CORE_1V2_HIGH_WARN	(SK_I32)(SK_SEN_CORE_1V2_VAL * C_PLUS_15)
#define SK_SEN_CORE_1V2_LOW_WARN	(SK_I32)(SK_SEN_CORE_1V2_VAL * C_MINUS_5)
#define SK_SEN_CORE_1V2_LOW_ERR		(SK_I32)(SK_SEN_CORE_1V2_VAL * C_MINUS_10)

#define SK_SEN_CORE_1V3_VAL		1300	/* Voltage ASIC Core 1.3 Volt */

#define SK_SEN_CORE_1V3_HIGH_ERR	(SK_I32)(SK_SEN_CORE_1V3_VAL * C_PLUS_15)
#define SK_SEN_CORE_1V3_HIGH_WARN	(SK_I32)(SK_SEN_CORE_1V3_VAL * C_PLUS_10)
#define SK_SEN_CORE_1V3_LOW_WARN	(SK_I32)(SK_SEN_CORE_1V3_VAL * C_MINUS_5)
#define SK_SEN_CORE_1V3_LOW_ERR		(SK_I32)(SK_SEN_CORE_1V3_VAL * C_MINUS_10)

/*
 * FAN 1 speed
 */
/* assuming: 6500rpm +-15%, 4 pulses,
 * warning at:	80 %
 * error at:	70 %
 * no upper limit
 */
#define SK_SEN_FAN_HIGH_ERR		20000	/* FAN Speed High Err Threshold */
#define SK_SEN_FAN_HIGH_WARN	20000	/* FAN Speed High Warn Threshold */
#define SK_SEN_FAN_LOW_WARN		 5200	/* FAN Speed Low Warn Threshold */
#define SK_SEN_FAN_LOW_ERR		 4550	/* FAN Speed Low Err Threshold */

/*
 * Some Voltages need dynamic thresholds
 */
#define SK_SEN_DYN_INIT_NONE		 0  /* No dynamic init of thresholds */
#define SK_SEN_DYN_INIT_PCI_IO		10  /* Init PCI-IO with new thresholds */
#define SK_SEN_DYN_INIT_VAUX		11  /* Init VAUX with new thresholds */

extern	int SkLm80ReadSensor(SK_AC *pAC, SK_IOC IoC, SK_SENSOR *pSen);
#endif	/* n_INC_SKGETWSI_H */

