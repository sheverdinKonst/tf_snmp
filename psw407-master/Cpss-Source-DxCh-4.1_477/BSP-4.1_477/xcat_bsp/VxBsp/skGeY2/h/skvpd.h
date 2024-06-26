/******************************************************************************
 *
 * Name:	skvpd.h
 * Project:	Gigabit Ethernet Adapters, VPD-Module
 * Version:	$Revision: 1 $
 * Date:	$Date: 12/04/08 3:10p $
 * Purpose:	Defines and Macros for VPD handling
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
 * skvpd.h	contains Diagnostic specific defines for VPD handling
 */

#ifndef __INC_SKVPD_H_
#define __INC_SKVPD_H_

/*
 * Define Resource Type Identifiers and VPD keywords
 */
#define RES_ID		0x82	/* Resource Type ID String (Product Name) */
#define RES_VPD_R	0x90	/* start of VPD read only area */
#define RES_VPD_W	0x91	/* start of VPD read/write area */
#define RES_END		0x78	/* Resource Type End Tag */

#ifndef VPD_NAME
#define VPD_NAME	"Name"	/* Product Name, VPD name of RES_ID */
#endif	/* VPD_NAME */
#define VPD_PN		"PN"	/* Adapter Part Number */
#define VPD_EC		"EC"	/* Adapter Engineering Level */
#define VPD_MN		"MN"	/* Manufacture ID */
#define VPD_SN		"SN"	/* Serial Number */
#define VPD_CP		"CP"	/* Extended Capability */
#define VPD_RV		"RV"	/* Checksum and Reserved */
#define VPD_YA		"YA"	/* Asset Tag Identifier */
#define VPD_VL		"VL"	/* First Error Log Message (SK specific) */
#define VPD_VF		"VF"	/* Second Error Log Message (SK specific) */
#define VPD_VB		"VB"	/* Boot Agent ROM Configuration (SK specific) */
#define VPD_VE		"VE"	/* EFI UNDI Configuration (SK specific) */
#define VPD_RW		"RW"	/* Remaining Read / Write Area */

/* 'type' values for vpd_setup_para() */
#define VPD_RO_KEY	1	/* RO keys are "PN", "EC", "MN", "SN", "RV" */
#define VPD_RW_KEY	2	/* RW keys are "Yx", "Vx", and "RW" */

/* 'op' values for vpd_setup_para() */
#define ADD_KEY		1	/* add the key at the pos "RV" or "RW" */
#define OWR_KEY		2	/* overwrite key if already exists */

/*
 * Define READ and WRITE Constants.
 */

#define VPD_DEV_ID_GENESIS	0x4300

#define VPD_SIZE_YUKON		256
#define VPD_SIZE_GENESIS	512
#define VPD_SIZE			512
#define VPD_READ	0x0000
#define VPD_WRITE	0x8000

#define VPD_STOP(pAC,IoC)	VPD_OUT16(pAC,IoC,PCI_VPD_ADR_REG,VPD_WRITE)

#define VPD_GET_RES_LEN(p)	((unsigned int)\
					(*(SK_U8 *)&(p)[1]) |\
					((*(SK_U8 *)&(p)[2]) << 8))
#define VPD_GET_VPD_LEN(p)	((unsigned int)(*(SK_U8 *)&(p)[2]))
#define VPD_GET_VAL(p)		((char *)&(p)[3])

#define VPD_MAX_LEN	50

#define VPD_SPI_ADDR	0x1f700UL

/* VPD status */
	/* bit 7..2 reserved */
#ifndef SK_SLIM
#define VPD_NO_RW	(1<<1)	/* No buffer available for VPD R/W data */
#endif	/* SK_SLIM */
#define VPD_VALID	(1<<0)	/* VPD data buffer, vpd_free_ro, */
							/* and vpd_free_rw valid	 */

/*
 * VPD structs
 */
typedef	struct s_vpd_status {
	unsigned short	Align01;			/* Alignment */
	unsigned short	vpd_status;			/* VPD status, description see above */
	int				vpd_free_ro;		/* unused bytes in read only area */
	int				vpd_free_rw;		/* bytes available in read/write area */
} SK_VPD_STATUS;

typedef	struct s_vpd {
	SK_VPD_STATUS	v;					/* VPD status structure */
	char			vpd_buf[VPD_SIZE];	/* VPD buffer */
	int				rom_size;			/* VPD ROM size from PCI_OUR_REG_2 */
	int				vpd_size;			/* saved VPD-size */
	SK_BOOL			CfgInSpi;			/* config data is stored in SPI flash */
} SK_VPD;

typedef	struct s_vpd_para {
	unsigned int	p_len;	/* parameter length */
	char			*p_val;	/* points to the value */
} SK_VPD_PARA;

/*
 * structure of Large Resource Type Identifiers
 */

/* was removed because of alignment problems */

/*
 * structure of VPD keywords
 */
typedef	struct s_vpd_key {
	char			p_key[2];	/* 2 bytes ID string */
	unsigned char	p_len;		/* 1 byte length */
	char			p_val;		/* start of the value string */
} SK_VPD_KEY;


/*
 * System specific VPD macros
 */
#ifndef SK_DIAG
#ifndef VPD_DO_IO
#define VPD_OUT8(pAC,IoC,Addr,Val)	(void)SkPciWriteCfgByte(pAC,Addr,Val)
#define VPD_OUT16(pAC,IoC,Addr,Val)	(void)SkPciWriteCfgWord(pAC,Addr,Val)
#define VPD_OUT32(pAC,IoC,Addr,Val)	(void)SkPciWriteCfgDWord(pAC,Addr,Val)
#define VPD_IN8(pAC,IoC,Addr,pVal)	(void)SkPciReadCfgByte(pAC,Addr,pVal)
#define VPD_IN16(pAC,IoC,Addr,pVal)	(void)SkPciReadCfgWord(pAC,Addr,pVal)
#define VPD_IN32(pAC,IoC,Addr,pVal)	(void)SkPciReadCfgDWord(pAC,Addr,pVal)
#else	/* VPD_DO_IO */
#define VPD_OUT8(pAC,IoC,Addr,Val)	SK_OUT8(IoC,PCI_C(pAC,Addr),Val)
#define VPD_OUT16(pAC,IoC,Addr,Val)	SK_OUT16(IoC,PCI_C(pAC,Addr),Val)
#define VPD_OUT32(pAC,IoC,Addr,Val)	SK_OUT32(IoC,PCI_C(pAC,Addr),Val)
#define VPD_IN8(pAC,IoC,Addr,pVal)	SK_IN8(IoC,PCI_C(pAC,Addr),pVal)
#define VPD_IN16(pAC,IoC,Addr,pVal)	SK_IN16(IoC,PCI_C(pAC,Addr),pVal)
#define VPD_IN32(pAC,IoC,Addr,pVal)	SK_IN32(IoC,PCI_C(pAC,Addr),pVal)
#endif	/* VPD_DO_IO */
#else	/* SK_DIAG */
#define VPD_OUT8(pAC,Ioc,Addr,Val) {			\
		if ((pAC)->DgT.DgUseCfgCycle)			\
			SkPciWriteCfgByte(pAC,Addr,Val);	\
		else									\
			SK_OUT8(pAC,PCI_C(pAC,Addr),Val);	\
		}
#define VPD_OUT16(pAC,Ioc,Addr,Val) {			\
		if ((pAC)->DgT.DgUseCfgCycle)			\
			SkPciWriteCfgWord(pAC,Addr,Val);	\
		else						\
			SK_OUT16(pAC,PCI_C(pAC,Addr),Val);	\
		}
#define VPD_OUT32(pAC,Ioc,Addr,Val) {			\
		if ((pAC)->DgT.DgUseCfgCycle)			\
			SkPciWriteCfgDWord(pAC,Addr,Val);	\
		else						\
			SK_OUT32(pAC,PCI_C(pAC,Addr),Val);	\
		}
#define VPD_IN8(pAC,Ioc,Addr,pVal) {			\
		if ((pAC)->DgT.DgUseCfgCycle)			\
			SkPciReadCfgByte(pAC,Addr,pVal);	\
		else						\
			SK_IN8(pAC,PCI_C(pAC,Addr),pVal);	\
		}
#define VPD_IN16(pAC,Ioc,Addr,pVal) {			\
		if ((pAC)->DgT.DgUseCfgCycle)			\
			SkPciReadCfgWord(pAC,Addr,pVal);	\
		else						\
			SK_IN16(pAC,PCI_C(pAC,Addr),pVal);	\
		}
#define VPD_IN32(pAC,Ioc,Addr,pVal) {			\
		if ((pAC)->DgT.DgUseCfgCycle)			\
			SkPciReadCfgDWord(pAC,Addr,pVal);	\
		else						\
			SK_IN32(pAC,PCI_C(pAC,Addr),pVal);	\
		}
#endif /* SK_DIAG */

/* function prototypes ********************************************************/

#ifndef	SK_KR_PROTO
#ifdef SK_DIAG
extern SK_U32	VpdReadDWord(
	SK_AC		*pAC,
	SK_IOC		IoC,
	int			addr);
#endif /* SK_DIAG */

extern int	VpdSetupPara(
	SK_AC		*pAC,
	const char	*key,
	const char	*buf,
	int			len,
	int			type,
	int			op);

extern SK_VPD_STATUS	*VpdStat(
	SK_AC		*pAC,
	SK_IOC		IoC);

extern int	VpdKeys(
	SK_AC		*pAC,
	SK_IOC		IoC,
	char		*buf,
	int			*len,
	int			*elements);

extern int	VpdRead(
	SK_AC		*pAC,
	SK_IOC		IoC,
	const char	*key,
	char		*buf,
	int			*len);

extern SK_BOOL	VpdMayWrite(
	char		*key);

extern int	VpdWrite(
	SK_AC		*pAC,
	SK_IOC		IoC,
	const char	*key,
	const char	*buf);

extern int	VpdDelete(
	SK_AC		*pAC,
	SK_IOC		IoC,
	char		*key);

extern int	VpdUpdate(
	SK_AC		*pAC,
	SK_IOC		IoC);

extern void	VpdErrLog(
	SK_AC		*pAC,
	SK_IOC		IoC,
	char		*msg);

int VpdInit(
	SK_AC		*pAC,
	SK_IOC		IoC);

extern int	VpdReadBlock(
	SK_AC		*pAC,
	SK_IOC		IoC,
	char		*buf,
	int			addr,
	int			len);

extern int	VpdWriteBlock(
	SK_AC		*pAC,
	SK_IOC		IoC,
	char		*buf,
	int			addr,
	int			len);

#else	/* SK_KR_PROTO */
extern SK_U32	VpdReadDWord();
extern int	VpdSetupPara();
extern SK_VPD_STATUS	*VpdStat();
extern int	VpdKeys();
extern int	VpdRead();
extern SK_BOOL	VpdMayWrite();
extern int	VpdWrite();
extern int	VpdDelete();
extern int	VpdUpdate();
extern void	VpdErrLog();
#endif	/* SK_KR_PROTO */

#endif	/* __INC_SKVPD_H_ */

