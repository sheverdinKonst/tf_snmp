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

#include <stdio.h>
#include <string.h>
#include "vxWorks.h"
#include "sysLib.h"
#include "logLib.h"
#include "tickLib.h"
#include "intLib.h" 
#include "config.h"
#include <arch/arm/ivArm.h>
#if 	defined(MV646xx)
	#include "db64660.h"
#elif 	defined(MV78XX0)
	#include "db78xx0.h"
#elif	defined(MV88F6XXX)
	#include "db88F6281.h"
#elif	defined (MV_88F6183)
	#include "db88F6183.h"
#endif

#include <wrn/cci/cci.h>
#include "mvOs.h"
#include "mvSysHwConfig.h"
#include "cesa/mvCesaRegs.h"
#include "cesa/mvCesa.h"
#include "ctrlEnv/sys/mvSysCesa.h"
#include "common/mvDebug.h"
#include "ctrlEnv/sys/mvcpuIf.h"


#define MARVELL_PROVIDER_NAME	"MV_CESA" 
#define	CESA_INTERRUPT_SUPPORT	
#define INTERRUPT_CHECK_READY
#define CESA_READY_TASK_NAME	"tCesa" 
#define CESA_READY_PRIORITY		31


#undef MV_CESA_DEBUG

#ifdef MV_CESA_DEBUG

#define DEBUG_CCI_PROVIDER 		0x0001
#define DEBUG_CIPHER            0x0002
#define DEBUG_HMAC	            0x0004
#define DEBUG_HASH	            0x0008
#define DEBUG_SINGLEPASS		0x0010 
#define DEBUG_DATA				0x1000 

int	mvCesaDebugFlag =   DEBUG_CCI_PROVIDER |
                        DEBUG_HMAC |
                        DEBUG_HASH | 
                        DEBUG_CIPHER |
                        DEBUG_SINGLEPASS;

#define CciDebugPrint(x,y)  if (x & mvCesaDebugFlag)	\
						mvOsPrintf(y);
#define CciDebugPrintArg(x,y,p1,p2,p3,p4,p5,p6)  if (x & mvCesaDebugFlag)	\
						mvOsPrintf(y,p1,p2,p3,p4,p5,p6);
#define CciDebugPrintDATA(x, str,pData,size)  											\
					if ((DEBUG_DATA & mvCesaDebugFlag) && (x & mvCesaDebugFlag)) 		\
					 {                                                                  \
							 mvOsPrintf("%s: offset=%p, sizes=%d\n", str, pData, size); \
							 mvDebugMemDump(pData,size,1);                              \
					 }
#else
#define CciDebugPrint(x,y)  
#define CciDebugPrintArg(x,y,p1,p2,p3,p4,p5,p6)
#define CciDebugPrintDATA(x, p1,p2,p3)
#endif

#define SUPPORT_CIPHER		1
#define SUPPORT_HMAC		2
#define SUPPORT_HASH		4
#define SUPPORT_SINGLEPASS	8

#if ((CCI_MAJOR_VER > 3) || ((CCI_MAJOR_VER == 3) && (CCI_MINOR_VER > 1)))
cci_st mvCci_singlepass_open_session( const CCIContext cciContext ); 
cci_st mvCciSessionHandler_SinglePass( const CCIContext cciContext ); 
cci_st mvCci_singlepass_close_session( const CCIContext cciContext ); 

int	cciAlgorithmSupport = SUPPORT_CIPHER | SUPPORT_HASH| SUPPORT_HMAC | SUPPORT_SINGLEPASS;
#else 
int	cciAlgorithmSupport = SUPPORT_CIPHER | SUPPORT_HMAC;
#endif /* CCI version > 3.1 */
unsigned long       taskId;

#define	SESSION_STATE_ALLOC		0x1234
#define	SESSION_STATE_UPDAT   	0x5678
typedef struct 
{
    MV_16               sessionId;
	MV_U32				SessionStat;
    cci_t               cipherMode;
	cci_t				direction;
	unsigned long 		WaitReadySemId;   
	int					index;

} VX_CCI_CIPHER;

typedef struct 
{
    MV_16               sessionId;
	MV_U32				digestSize;
	MV_32				numUpdate;
	MV_32				numByteAlign;
	MV_U8				*pDigest;
    MV_U8               *pMacData;
    int                 macSize;
    MV_CESA_MBUF        inputMBuf, outputMBuf;
    MV_BUF_INFO         inputBuf[2], outputBuf[2];
    MV_CESA_COMMAND     command;
	unsigned long 		WaitReadySemId;   
} VX_CCI_HMAC;

#define HMAC_ALIGN	8

/*
** --- WindRiver generated security tags references
*/
extern CCISecurityTag *mv_cci_security_funct_1();
extern CCISecurityTag *mv_cci_security_funct_2();
extern CCISecurityTag *mv_cci_security_funct_3();
extern CCISecurityTag *mv_cci_security_funct_4();


cci_st mvCciSessionHandler_CipherOpen( const CCIContext cciContext );
cci_st mvCciSessionHandler_Cipher( const CCIContext cciContext );
cci_st mvCciSessionHandler_CipherClose(const CCIContext cciContext );

cci_st mvCciSessionHandler_Init_Hash( const CCIContext cciContext );
cci_st mvCciSessionHandler_Update_Hash( const CCIContext cciContext );
cci_st mvCciSessionHandler_Final_Hash( const CCIContext cciContext );

cci_st mvCciSessionHandler_Init_Hmac( const CCIContext cciContext );
cci_st mvCciSessionHandler_Update_Hmac( const CCIContext cciContext );
cci_st mvCciSessionHandler_Final_Hmac( const CCIContext cciContext );


unsigned __TASKCONV cesaReadyTask(void* args);
static  	void	cesaTestReadyIsr(void);
void 				mvCesaWaitPreviousCommandCompleted(unsigned long SemId);
MV_STATUS   		mvCesaDoCommand (MV_CESA_COMMAND* pCmd);
void 				CipherClose(VX_CCI_CIPHER *pCipher);
VX_CCI_CIPHER * 	CipherOpen( void);

unsigned long mvCesaSemId;
unsigned long vxCesaWaitSemId;   


/*
** --- WindRiver generated Security table
*/
/*
** --- Security tag for module #1
*/
static CCISecurityTag mv_cesa_security_tag_1[] = {0x24,0x57,0xC3,0x27,0xE1,0x02,0x62,0x02,0x8C,0xF8,
0xFE,0xB2,0x80,0xC4,0xDC,0x6A,0x4E,0xF2,0x5A,0x2E};
CCISecurityTag *mv_cesa_security_funct_1(){ return( mv_cesa_security_tag_1 ); };

/*
** --- Security tag for module #2
*/
static CCISecurityTag mv_cesa_security_tag_2[] = {0x20,0x52,0x02,0x61,0xDF,0xA9,0x68,0x1B,0x6B,0xB8,
0x2D,0xD9,0x9E,0xBB,0xF5,0x52,0x0A,0x4F,0x6F,0x8F};
CCISecurityTag *mv_cesa_security_funct_2(){ return( mv_cesa_security_tag_2 ); };

/*
** --- Security tag for module #3
*/
static CCISecurityTag mv_cesa_security_tag_3[] = {0x39,0x84,0xE7,0x81,0x57,0x4A,0x6C,0x69,0x64,0x36,
0xD6,0xA0,0x39,0x5F,0x90,0x99,0xB1,0x85,0xAC,0x03};
CCISecurityTag *mv_cesa_security_funct_3(){ return( mv_cesa_security_tag_3 ); };

/*
** --- Security tag for module #4
*/
static CCISecurityTag mv_cesa_security_tag_4[] = {0xF6,0xA7,0xB8,0x1D,0x78,0xAD,0x27,0x5C,0x30,0x2A,
0x7F,0xAC,0xFA,0x4D,0x64,0x57,0xB6,0x2E,0xA5,0x56};
CCISecurityTag *mv_cesa_security_funct_4(){ return( mv_cesa_security_tag_4 ); };

/*
** --- Security tag for module #5
*/
static CCISecurityTag mv_cesa_security_tag_5[] = {0xC1,0xE9,0xE7,0x49,0x2A,0x19,0x2C,0x1F,0x8F,0x19,
0xFA,0x2F,0x1F,0xE7,0x45,0xDC,0xE5,0xA2,0x2C,0xFB};
CCISecurityTag *mv_cesa_security_funct_5(){ return( mv_cesa_security_tag_5 ); };

/*
** --- Security tag for module #6
*/
static CCISecurityTag mv_cesa_security_tag_6[] = {0x3C,0xD6,0xBE,0xAE,0x50,0xD5,0x49,0x76,0x85,0x75,
0x29,0xE3,0x4A,0x13,0x04,0x00,0x2F,0x9A,0xB9,0xA2};
CCISecurityTag *mv_cesa_security_funct_6(){ return( mv_cesa_security_tag_6 ); };

extern CCISecurityTag *mv_cesa_security_funct_1();
extern CCISecurityTag *mv_cesa_security_funct_2();
extern CCISecurityTag *mv_cesa_security_funct_3();
extern CCISecurityTag *mv_cesa_security_funct_4();
extern CCISecurityTag *mv_cesa_security_funct_5();
extern CCISecurityTag *mv_cesa_security_funct_6();

/*
** --- WindRiver generated Security table
*/
static void *(security_tag_table)[] =
{
    mv_cesa_security_funct_1,
    mv_cesa_security_funct_2,
    mv_cesa_security_funct_3,
    mv_cesa_security_funct_4,
    mv_cesa_security_funct_5,
    mv_cesa_security_funct_6,
    NULL
};

/*
** --- MV_CESA Provider's Digital signature
*/
static cci_b mv_cesa_provider_signature[] = {
    0x5E,0xE1,0x36,0x17,0xD6,0x69,0x65,0x7B,0x67,0x2F,0x08,0xF4,0x79,0x08,0x44,0xCF, \
    0xDB,0x6A,0x67,0x01,0x12,0xFB,0x80,0xA5,0xBA,0xBA,0xA7,0x5C,0x15,0x60,0x8D,0xD0, \
    0xCE,0xED,0xAB,0xFB,0xD5,0x38,0x39,0x75,0x7A,0x81,0xC5,0x41,0x0C,0x4B,0xD4,0x98, \
    0x15,0x68,0x41,0xF8,0x8F,0xA1,0xB8,0x19,0xDF,0xDF,0xED,0xBF,0xD7,0x4E,0xF9,0x8F, \
    0x49,0x74,0x12,0x1F,0x1C,0xC7,0x1D,0xAE,0x99,0x42,0x1C,0xA6,0xA7,0xF3,0x76,0x6C, \
    0x01,0x46,0xE8,0xF1,0x2B,0x1F,0x38,0x81,0xB5,0x2A,0x6C,0xE1,0x40,0xEC,0x7B,0x2A, \
    0x94,0x80,0x06,0x01,0x4B,0x7F,0xBD,0x85,0x22,0x0F,0xCF,0x23,0xF5,0xC4,0x8B,0xBB, \
    0x8B,0x37,0xB3,0x8C,0x75,0x44,0x3A,0x92,0x4E,0x63,0x93,0xA2,0x96,0x73,0x4B,0x1A, \
    0xA5,0xB3,0xCA,0x56,0x1A,0xCE,0x06,0xF0,0xB3,0x61,0xCB,0xAA,0x27,0x49,0xF2,0x7E, \
    0x14,0x07,0xC4,0x94,0x68,0x7F,0x21,0x2E,0xFB,0xE7,0xAD,0xC8,0xF0,0x5D,0xFE,0x95, \
    0xF5,0x92,0xDE,0xE1,0x03,0x79,0x27,0xA5,0xDA,0x95,0xE1,0xC2,0xC8,0x28,0x45,0xD1, \
    0x0E,0xBB,0x82,0xB1,0x9B,0x19,0x0B,0x46,0x28,0x31,0x7A,0x21,0xF3,0xEA,0xBC,0x2C, \
    0xAF,0x46,0x13,0xF1,0x51,0xBC,0xF3,0x76,0x5E,0x3A,0x22,0xDF,0x59,0xA9,0x7A,0x75, \
    0xFE,0x4A,0x88,0x8E,0xC5,0xA2,0x5B,0x9D,0x89,0x65,0x32,0x8C,0x8D,0x12,0x75,0xF2, \
    0xC8,0xFF,0x38,0x99,0x6C,0x58,0x2D,0xF1,0xE7,0x42,0xD1,0x7B,0x96,0x54,0x13,0xE6, \
    0x3A,0x96,0xEF,0xE6,0x21,0x6A,0x99,0xF9,0x4B,0x7C,0xF9,0xD8,0x36,0x2E,0xE1,0xD7};



/*
** --- WindRiver 2048-bit RSA Public Key components.
*/
static cci_b cci_public_key_e[] = {0x11};
static cci_b cci_public_key_n[] = {
    0xBE,0xA5,0xCC,0x4A,0x37,0x28,0xF5,0x1F,0xF7,0x7F,0x7E,0xD2,0x20,0xFD,0xB2,0x57, \
    0x98,0xCF,0x39,0xCA,0x28,0xE9,0x19,0x4C,0x7C,0x67,0x7B,0xD7,0x1B,0xE0,0x60,0xCB, \
    0x8D,0x32,0x98,0x5D,0xC3,0x18,0x7C,0x35,0xED,0xCB,0x3F,0x44,0x4C,0xFF,0xCA,0xFD, \
    0x0A,0xF7,0x37,0xAC,0x0A,0xC4,0x9F,0xB5,0xF5,0x54,0x3C,0x34,0xAE,0xCD,0xC9,0x70, \
    0x84,0x0D,0xA1,0x84,0xBD,0xE8,0x22,0x46,0xE9,0xE0,0x55,0xDD,0x10,0xA4,0xF1,0x57, \
    0x58,0x89,0x8E,0xF9,0x1E,0xB6,0x60,0x03,0xB0,0xCE,0x11,0x0A,0x14,0xC6,0x00,0xAE, \
    0x1E,0x43,0x00,0x1C,0xC2,0xFA,0x83,0x2D,0xE4,0x6A,0x29,0x3F,0x92,0xAA,0x2B,0xE1, \
    0x21,0x26,0x21,0xF8,0x37,0x87,0xD3,0xC4,0x47,0xFF,0xD5,0x4C,0x1A,0x36,0x7A,0x00, \
    0x7A,0x48,0xED,0xFD,0xC5,0xCC,0x25,0x2C,0xF7,0x72,0xEF,0xED,0x39,0x5D,0xCC,0x85, \
    0xC2,0xEA,0xE6,0x58,0xB2,0x19,0x85,0xB9,0x2F,0xDA,0xEA,0x80,0x55,0xEA,0x5C,0xFA, \
    0xA3,0x5A,0x94,0xE1,0xBD,0xF6,0x57,0x63,0x1C,0x58,0x87,0x43,0x5E,0xEE,0x2B,0xF7, \
    0x6C,0xAD,0x51,0x4D,0x8B,0xC5,0xE2,0x5E,0x68,0x14,0x1C,0x54,0xC0,0x1A,0x29,0xE5, \
    0xC9,0xCB,0xD7,0x7F,0x3E,0xA2,0x55,0x61,0xEB,0xAF,0xD2,0x3D,0x8C,0x71,0xBC,0xC6, \
    0xA3,0xA6,0x58,0x67,0x0C,0x67,0x8A,0x1B,0x88,0x96,0xBB,0x89,0x6F,0x29,0xE4,0x19, \
    0x1E,0xF6,0x6A,0x89,0xC3,0x23,0x59,0xC5,0x57,0xB4,0xD0,0x4A,0x83,0xEE,0x23,0x61, \
    0x34,0x74,0x34,0x74,0x3D,0xC4,0xB0,0x67,0xBC,0x7E,0xE5,0x93,0xA4,0xD4,0x4C,0x1F};


/*
** --- CCI Public key object.
*/
static CCIPublicKey publicKey;

int	delayB4Ready=1;
/*
** --- .
**     Debug/performence/counters .
*/
int	HashInitCounter				= 0;
int	HashUpdateCounter			= 0;
int	HashFinalCounter			= 0;
int	HmacInitCounter				= 0;
int	HmacUpdateCounter			= 0;
int	HmacFinalCounter			= 0;
int	CipherOpenCounter  			= 0;
int	CipherUpdateCounter			= 0;
int	CipherCloseCounter			= 0;
int	cesaTestIsrCounter			= 0;
int cesaSemaWaitCounter 		= 0;
int cesaDoCmdCounter			= 0;
int	cesaSemaWait4busyCounter 	= 0;
int	cesaTestTaskCounter			= 0;
#if ((CCI_MAJOR_VER > 3) || ((CCI_MAJOR_VER == 3) && (CCI_MINOR_VER > 1)))
int SinglePassProcCounter	    = 0;
int SinglePassErrorCounter	    = 0;
int SinglePassOpenCounter		= 0;
int SinglePassCloseCounter		= 0;
int SinglePassEncryptOnly		= 0;
int SinglePassEncryptAuth		= 0;
int SinglePassEncryptAuthBytes	= 0;
int SinglePassDecryptOnly		= 0;
int SinglePassDecryptAuth		= 0;
int SinglePassDecryptAuthBytes	= 0;
#endif /* CCI version > 3.1 */

char *AlgName[]={
	"CCI_CIPHER_DES",											/* 56bit key DES block cipher				*/
	"CCI_CIPHER_2DES",										/* 112bit key DES block cipher              */
	"CCI_CIPHER_3DES",										/* 168bit key TRIPLE-DES block cipher       */
	"CCI_CIPHER_IDEA",										/* IDEA block cipher						*/
	"CCI_CIPHER_DESX",										/* 56bit key DESX block cipher				*/
	"CCI_CIPHER_RC2",											/* RC2 block cipher                         */
	"CCI_CIPHER_RC5",											/* RC5 block cipher                         */
	"CCI_CIPHER_BLOWFISH",									/* BLOWFISH block cipher                    */
	"CCI_CIPHER_SKIPJACK",									/* SKIPJACK block cipher                    */
	"CCI_CIPHER_CAST128",										/* CAST-128 block cipher                    */
	"CCI_CIPHER_AES",											/* AES block cipher                         */
	"CCI_CIPHER_RC4",											/* RC4 stream cipher                        */
	"CCI_CIPHER_A5",											/* A5 stream cipher							*/
	"CCI_CIPHER_LEVIATHON",									/* LEVIATHON stream cipher					*/
	"CCI_CIPHER_SOBER",										/* SOBER stream cipher                      */
	"CCI_CIPHER_SEAL",										/* SEAL stream cipher						*/
	"CCI_CIPHER_FEAL",										/* FEAL block cipher						*/
	"CCI_CIPHER_ICE",											/* ICE 64-bit block cipher					*/
	"CCI_CIPHER_KASUMI",										/* KASUMI 64-bit block cipher				*/
	"CCI_CIPHER_MISTY",										/* MISTY 64-bit block cipher				*/
	"CCI_CIPHER_SAFER",										/* SAFER 64-bit block cipher				*/
	"CCI_CIPHER_SHARK",										/* SHARK 64-bit block cipher				*/
	"CCI_CIPHER_CAMELLIA",									/* CAMELLIA 128-bit block cipher			*/
	"CCI_CIPHER_SEED",										/* SEED 128-bit block cipher				*/
	"CCI_CIPHER_SQUARE",										/* SQUARE 128-bit block cipher				*/
	"CCI_CIPHER_UNICORN",										/* UNICORN 128-bit block cipher				*/
	"CCI_CIPHER_MERCY",										/* MERCY Disk Sector block cipher			*/
	"CCI_CIPHER_PANAMA",										/* PANAMA cipher							*/
	"CCI_CIPHER_NULL",										/* NULL encryption cipher					*/
	"CCI_CIPHER_AESKW",										/* AES Key Wrap Algorithm					*/
	"CCI_CIPHER_RC4TKIP",										/* TKIP using RC4 algorithm					*/
	"CCI_CIPHER_MAX"											/* Max value								*/
};
char *CipherModeName[]= 
{
	"CCI_MODE_NONE",											/* No CIPHER mode needed					*/
	"CCI_MODE_BC",											/* Block Chaining Mode						*/
	"CCI_MODE_PCBC",											/* Propagating Cipher Block Chaining Mode	*/
	"CCI_MODE_CBCC",											/* Cipher Block Chaining Checksum Mode		*/
	"CCI_MODE_OFBNLF",										/* Output-Feedback Nonlinear Fuction Mode	*/
	"CCI_MODE_PBC",											/* Plainttext Block Chaining Mode			*/
	"CCI_MODE_CBC",											/* Cipher Block Chaining Mode				*/
	"CCI_MODE_ECB",											/* Electronic Codebook Mode					*/
	"CCI_MODE_PFB",											/* Plaintext-Feedback Mode					*/
	"CCI_MODE_CFB",											/* Cipher-Feedback Mode						*/
	"CCI_MODE_CFB128",										/* Cipher-Feedback 128-bit mode				*/
	"CCI_MODE_CFB8",											/* Cipher-Feedback 8-bit mode				*/
	"CCI_MODE_CFB1",											/* Cipher-Feedback 1-bit mode				*/
	"CCI_MODE_CTR",											/* AES counter Mode							*/
	"CCI_MODE_OFB",											/* Output-Feedback  Mode					*/
	"CCI_MODE_MAX"											/* Maximum cipher modes						*/
};



/**************************************************************************************
* Method Name: mvCciProviderInit
* Description: Initialize CESA Security Processor
*
* Parameters: CCIProvider - CCI Provider object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise, CCI_FAILURE.
*
**************************************************************************************/
static cci_st mvCciProviderInit( CCIProvider provider )
{
    MV_STATUS       status;
    MV_CPU_DEC_WIN  addrDecWin;
    char*           pSram;
    cci_st          cciStatus = CCI_SUCCESS;

	CciDebugPrint(DEBUG_CCI_PROVIDER,"-------------ENTER mvCciProviderInit -------------\n")
    /*
    ** --- Initialize CESA device.
    */
    if (mvCpuIfTargetWinGet(CRYPT_ENG, &addrDecWin) == MV_OK)
        pSram = (char*)addrDecWin.addrWin.baseLow;
    else
    {
        mvOsPrintf("mvCesaInit: ERR. mvCpuIfTargetWinGet failed\n");
        return CCI_FAILURE;
    }

    status = mvCesaInit(32, 16, pSram,NULL);
    if(status != MV_OK)
    {
        mvOsPrintf("mvCesaInit is Failed: status = 0x%x\n", status);
        /* !!!! Dima cesaTestCleanup();*/
        return CCI_FAILURE;
    }
#ifdef CESA_INTERRUPT_SUPPORT
	status = mvOsTaskCreate(CESA_READY_TASK_NAME,
							CESA_READY_PRIORITY,
							4*1024,
							cesaReadyTask, NULL,&taskId);
    if(status != MV_OK)
    {
        mvOsPrintf("mvCciProviderInit:  mvOsTaskCreate is Failed: status = 0x%x\n", status);
        return CCI_FAILURE;
    }
	
	status = mvOsSemCreate("CESA-ISR",0,1,&vxCesaWaitSemId);
    if(status != MV_OK)
    {
        mvOsPrintf("mvCciProviderInit:  mvOsSemCreate is Failed: status = 0x%x\n", status);
        return CCI_FAILURE;
    }
    MV_REG_WRITE( MV_CESA_ISR_CAUSE_REG, 0);
    MV_REG_WRITE( MV_CESA_ISR_MASK_REG, MV_CESA_CAUSE_ACC_DMA_MASK);

	status = intConnect((VOIDFUNCPTR *)INUM_TO_IVEC(INT_LVL_CESA), cesaTestReadyIsr, (int)NULL);
	if (status != OK)
	{
		mvOsPrintf("CESA: Can't connect CESA (%d) interrupt, status=0x%x \n", INT_LVL_CESA, status);
		/* !!!! Dima cesaTestCleanup();*/
        return CCI_FAILURE;
	}
	status = mvOsSemCreate("CESA-ACTIVE",1,1,&mvCesaSemId );
	if (status != MV_OK)
	{
		mvOsPrintf("cesaTestStart: Can't create semaphore\n");
        return CCI_FAILURE;
	}
	intEnable(INT_LVL_CESA);
#endif

    printf("mvCciProviderInit\n");
    return( cciStatus );
 }

 /**************************************************************************************
 * Method Name: mvCciProviderUninit
 * Description: Shutdown Initialize CESA Security Processor
 *
 * Parameters: CCIProvider - CCI Provider object
 *
 *
 * Returns: CCI_SUCCESS if successful. Otherwise, CCI_FAILURE.
 *
 **************************************************************************************/
 static cci_st mvCciProviderUninit( CCIProvider provider )
 {
    cci_st      cciStatus = CCI_SUCCESS;

    /*
    ** --- Initialize SSP500 device.
    */

    printf("mvCciProviderUninit\n");
    return( cciStatus );
 }



 /**************************************************************************************
 * Method Name: mvExportCCIProvider
 * Description: Provider Export Function for Marvell CESA
 *
 * Parameters: None
 *
 *
 * Returns: CCIProvider - CCI Provider object.
 *
 **************************************************************************************/
 CCIProvider mvExportCCIProvider( void )
 {
    cci_st          cciStatus = CCI_SUCCESS;
    CCIProvider     mvCciProvider = ( CCIProvider ) NULL;
    CCIAlgorithm    algorithm;

    cciStatus = cciProviderCreate( &mvCciProvider, MARVELL_PROVIDER_NAME,
                                    mvCciProviderInit, mvCciProviderUninit );
    if( !CCISUCCESS( cciStatus ) )
    {
        mvOsPrintf("Can't create %s provider: status=%d\n", MARVELL_PROVIDER_NAME, cciStatus);
        return NULL;
    }

        /*
        ** --- Setup public key object...
        */
        cci_pki_create( &publicKey, CCI_DEF_PROVIDER_ID, CCI_RSA_PUBLIC_KEY );
 
        cciPKIKeyCompSet( publicKey, CCI_RSA_PUBLIC_EXPONENT, cci_public_key_e,
                          sizeof(cci_public_key_e));
        cciPKIKeyCompSet( publicKey, CCI_RSA_MODULAS, cci_public_key_n,
                          sizeof(cci_public_key_n));

        /*
        ** --- Register Digital Signature with provider module....
        */
        cciProviderAttrSet( mvCciProvider, CCI_PVR_SECURITY_SIGNATURE,
                            mv_cesa_provider_signature );
        cciProviderAttrSet( mvCciProvider, CCI_PVR_SECURITY_SIGNATURE_LEN, 256 );
        cciProviderAttrSet( mvCciProvider, CCI_PVR_SECURITY_PUBLIC_KEY, publicKey );
        cciProviderAttrSet( mvCciProvider, CCI_PVR_SECURITY_TAG_TABLE,
                            security_tag_table );
	if (SUPPORT_CIPHER & cciAlgorithmSupport)
	{
		/*
		** --- Define AES CIPHER algorithm
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_CIPHER, CCI_CIPHER_AES );
		if ( CCISUCCESS( cciStatus ) )
		{
			/*cciAlgorithmAttrSet( algorithm, CCI_ASYNC_SUPPORT, TRUE );*/
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_OPERATION, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_MODE, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_KEY, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_KEY_LENGTH, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_IV, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_IV_LENGTH, 16 );

			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION,
									mvCciSessionHandler_Cipher );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define AES CIPHER algorithm \n"); 
		}

		/*
		** --- Define DES CIPHER algorithm
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_CIPHER, CCI_CIPHER_DES );
		if ( CCISUCCESS( cciStatus ) )
		{
			/*cciAlgorithmAttrSet( algorithm, CCI_ASYNC_SUPPORT, TRUE );*/
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_OPERATION, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_MODE, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_KEY, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_KEY_LENGTH, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_IV, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_IV_LENGTH, 8 );

			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION, 
									mvCciSessionHandler_Cipher );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define DES CIPHER algorithm \n"); 
		}

		/*
		** --- Define TRIPLE-DES CIPHER algorithm
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_CIPHER, CCI_CIPHER_3DES );
		if ( CCISUCCESS( cciStatus ) )
		{
			/* cciAlgorithmAttrSet( algorithm, CCI_ASYNC_SUPPORT, TRUE );*/
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_OPERATION, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_MODE, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_KEY, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_KEY_LENGTH, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_IV, NULL );
			cciAlgorithmAttrSet( algorithm, CCI_CIPHER_IV_LENGTH, 8 );

			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION,
									mvCciSessionHandler_Cipher );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define TRIPLE-DES CIPHER algorithm \n"); 
		}
	}
	if (SUPPORT_HASH & cciAlgorithmSupport)
	{
		/*
		** --- Define HASH-MD5 algorithm handler
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_HASH, CCI_HASH_MD5 );
		if ( CCISUCCESS( cciStatus ) )
		{
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_INIT,
									mvCciSessionHandler_Init_Hash );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_UPDATE,
									mvCciSessionHandler_Update_Hash );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_FINAL,
									mvCciSessionHandler_Final_Hash );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define HASH-MD5 algorithm \n"); 
		}

		/*
		** --- Define HASH-SHA1 algorithm handler
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_HASH, CCI_HASH_SHA1 );
		if ( CCISUCCESS( cciStatus ) )
		{
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_INIT,
									mvCciSessionHandler_Init_Hash );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_UPDATE,
									mvCciSessionHandler_Update_Hash );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_FINAL,
									mvCciSessionHandler_Final_Hash );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define HASH-SH1 algorithm \n"); 
		}
	}

	if (SUPPORT_HMAC & cciAlgorithmSupport)
	{
		/*
		** --- Define HMAC-MD5 algorithm handler
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_HMAC, CCI_HMAC_MD5 );
		if ( CCISUCCESS( cciStatus ) )
		{
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_INIT,
									mvCciSessionHandler_Init_Hmac );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_UPDATE,
									mvCciSessionHandler_Update_Hmac );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_FINAL,
									mvCciSessionHandler_Final_Hmac );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define HMAC-MD5 algorithm \n"); 
		}
		/*
		** --- Define HMAC-SHA1 algorithm handler
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_HMAC, CCI_HMAC_SHA1 );
		if ( CCISUCCESS( cciStatus ) )
		{
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_INIT,
									mvCciSessionHandler_Init_Hmac );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_UPDATE,
									mvCciSessionHandler_Update_Hmac );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_FINAL,
									mvCciSessionHandler_Final_Hmac );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
			mvOsPrintf("Define HMAC-SH1 algorithm \n"); 
		}
	}
#if ((CCI_MAJOR_VER>3) || ((CCI_MAJOR_VER==3) && (CCI_MINOR_VER > 1)))
	if (SUPPORT_SINGLEPASS & cciAlgorithmSupport)
	{
		/*
		** --- Define IPSEC-SINGLEPASS algorithm handler
		*/
		cciStatus = cciAlgorithmCreate( &algorithm, CCI_CLASS_IPSEC, CCI_IPSEC_SINGLEPASS);
		if( CCISUCCESS( cciStatus ) )
		{
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_OPEN, mvCci_singlepass_open_session );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION, mvCciSessionHandler_SinglePass );
			cciAlgorithmHandlerSet( algorithm, CCI_HANDLER_SESSION_CLOSE, mvCci_singlepass_close_session );
			cciProviderAlgorithmSet( mvCciProvider, algorithm );
            mvOsPrintf("Define CCI_IPSEC_SINGLEPASS \n"); 
		}		 
	}
#endif

    mvOsPrintf("%s Provider exported: mvCciProvider=%p\n", MARVELL_PROVIDER_NAME, mvCciProvider);
    return( mvCciProvider );
}

/*
** --- Security tag for module #1
*/
static CCISecurityTag mv_cci_security_tag_1[] = 
        {0x6C,0xA6,0xD5,0xAF,0xD6,0x15,0x04,0x63,0x69,0x4E,
         0xE7,0x81,0x35,0x86,0x12,0x6A,0x40,0xC5,0xA4,0xF0};

CCISecurityTag *mv_cci_security_funct_1()
{ 
    return( mv_cci_security_tag_1 ); 
};


/**************************************************************************************
* Method Name: CipherOpen
* Description: create and init cipher struct
*
* Parameters: None
*
*
* Returns: cipher struct pointer
*
**************************************************************************************/
VX_CCI_CIPHER * CipherOpen( void)
{
	VX_CCI_CIPHER	*pCipher;

	pCipher = mvOsMalloc(sizeof(VX_CCI_CIPHER));
	if (NULL == pCipher)
	{
        printf("Error: mvCciSessionHandler_Init_Hmac: memory allocation error\n");
		return NULL;
	}
	memset(pCipher,0,sizeof(VX_CCI_CIPHER));
	pCipher->SessionStat = SESSION_STATE_ALLOC;
	pCipher->sessionId=0xff;
	return pCipher;
}

/**************************************************************************************
* Method Name: mvCciSessionHandler_CipherInit
* Description: CESA Session handler for AES, DES and 3DES requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_CipherOpen( const CCIContext cciContext )
{
	CipherOpenCounter++;
	return CCI_SUCCESS;
}


/**************************************************************************************
* Method Name: mvCciSessionHandler_Cipher
* Description: CESA Session handler for AES, DES and 3DES requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_Cipher( const CCIContext cciContext )
{
    cci_st                  cciStatus = CCI_SUCCESS;
    CCI_ALGORITHM_ID        algorithmId = cciCtxAlgorithmId( cciContext );
    MV_CESA_COMMAND         command;
    MV_STATUS               status;
    cci_t                   ivLength;
    void                    *pIvBuffer;
    MV_CESA_MBUF            inputMBuf, outputMBuf;
    MV_BUF_INFO             inputBuf[2], outputBuf[2];
	MV_8					ivTemp[MV_CESA_MAX_IV_LENGTH];
	VX_CCI_CIPHER			*pCipher;

	CipherUpdateCounter++;

	pCipher = CipherOpen();
	if (NULL == pCipher)
	{
		return CCI_FAILURE;
	}
    cciCtxSessionCtxSet(cciContext, (void *)pCipher);    /* save cipher struct pointer ) */

	CciDebugPrintArg(DEBUG_CIPHER, "cipherUpdate(%d) cciContext=0x%x, pCipher=%p  pCipher->sessionId=%d \n",
				   CipherUpdateCounter,(MV_U32)cciContext,(MV_U32)pCipher,pCipher->sessionId,3,4);

	if (pCipher->SessionStat == SESSION_STATE_ALLOC)
	{
		MV_CESA_OPEN_SESSION    openSession;
		cci_t					cryptoKeyLength;
		void                    *pCipherKey = NULL;
		MV_16                   sid;

		pCipher->SessionStat =   SESSION_STATE_UPDAT;
		/*
		** --- Make sure this is an AES, DES or 3DES request...
		*/
		switch (algorithmId)
		{
		case CCI_CIPHER_DES:
			openSession.cryptoAlgorithm = MV_CESA_CRYPTO_DES;
			break;

		case CCI_CIPHER_3DES:
			openSession.cryptoAlgorithm = MV_CESA_CRYPTO_3DES;
			break;

		case CCI_CIPHER_AES:
			openSession.cryptoAlgorithm = MV_CESA_CRYPTO_AES;
			break;

		default:
			mvOsPrintf("Cipher handler: Unexpected crypto algorithm %d\n", algorithmId);            
			return( S_cciLib_INVALID_ALGORITHM_ID );
		}
		status = mvOsSemCreate("CIPHER-ISR",0,1,&pCipher->WaitReadySemId);
		if (status != MV_OK)
		{
			mvOsPrintf("mvCciSessionHandler_Cipher:  mvOsSemCreate is Failed: status = 0x%x\n", status);
			return CCI_FAILURE;
		}

		/*
		** --- The MV_CESA only supports ECB and CBC modes
		*/
		cciCtxAttrGet( cciContext, CCI_CIPHER_MODE, &pCipher->cipherMode);
		
		switch (pCipher->cipherMode)
		{
		case CCI_MODE_ECB:
			openSession.cryptoMode = MV_CESA_CRYPTO_ECB;
			break;

		case CCI_MODE_CBC:
			openSession.cryptoMode = MV_CESA_CRYPTO_CBC;
			break;

		default:
			mvOsPrintf("Cipher handler: Unexpected crypto mode %d\n", pCipher->cipherMode);
			return( S_cciLib_ALGORITHM_NOT_SUPPORTED );
		}
		cciCtxAttrGet( cciContext, CCI_CIPHER_OPERATION, &pCipher->direction );

		switch (pCipher->direction)
		{
		case CCI_ENCRYPT:
			openSession.direction = MV_CESA_DIR_ENCODE;
			break;

		case CCI_DECRYPT:
			openSession.direction = MV_CESA_DIR_DECODE;
			break;

		default:
			mvOsPrintf("Cipher handler: Unexpected crypto direction %d\n", pCipher->direction);
			return S_cciLib_INVALID_ATTRIBUTE_VALUE;
		}

		/*
		** --- Get context attributes...
		*/
		cciCtxAttrGet( cciContext, CCI_CIPHER_KEY_LENGTH, &cryptoKeyLength);
		openSession.cryptoKeyLength = (MV_U8)cryptoKeyLength;

		cciCtxAttrGet( cciContext, CCI_CIPHER_KEY, &pCipherKey );

		CciDebugPrintArg(DEBUG_CIPHER , "mvCipher First update(%d): alg=%d, mode=%d, dir=%d\n",
				   CipherUpdateCounter, algorithmId, pCipher->cipherMode, pCipher->direction,5,6);

		if ( (pCipherKey == NULL) || (openSession.cryptoKeyLength == 0) ||
			 (openSession.cryptoKeyLength > MV_CESA_MAX_CRYPTO_KEY_LENGTH) )
		{
			mvOsPrintf("Cipher handler: Invalid key - %p (%d bytes)\n", 
				   pCipherKey, openSession.cryptoKeyLength);
			return S_cciLib_INVALID_KEY_LENGTH;
		}
		memcpy(openSession.cryptoKey, pCipherKey, openSession.cryptoKeyLength);

		openSession.operation = MV_CESA_CRYPTO_ONLY;

		/* Open session */
		mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
		status = mvCesaSessionOpen(&openSession, &sid);
		pCipher->sessionId = sid;
		mvOsSemSignal(mvCesaSemId);
		if (status != MV_OK)
		{
			mvOsPrintf("Cipher handler: Can't open Session - rc=%d\n", status);
			return S_cciLib_SESSION_OPEN_ERROR;
		}
	}
	if (pCipher->SessionStat != SESSION_STATE_UPDAT)
	{
		mvOsPrintf("update Cipher , pCiphe not initilaize,\n",1,2,3,4,5,6);
		return CCI_FAILURE;
	}

	cciCtxAttrGet( cciContext, CCI_INPUT_BUFFER,  &inputBuf[1].bufVirtPtr);
	cciCtxAttrGet( cciContext, CCI_INPUT_LENGTH,  &inputBuf[1].bufSize);
	cciCtxAttrGet( cciContext, CCI_OUTPUT_BUFFER, &outputBuf[1].bufVirtPtr);
	outputBuf[1].bufSize = inputBuf[1].bufSize;
    inputMBuf.numFrags 	= 1;

    /*
    ** --- If the cipher mode is CBC, make sure we have an init vector and length
    */
    if( pCipher->cipherMode == CCI_MODE_CBC )
    {
        /*
        ** --- Extract ivlength....
        */
        cciStatus = cciCtxAttrGet( cciContext, CCI_CIPHER_IV_LENGTH, &ivLength );
        if( CCISUCCESS( cciStatus ) )
        {
            /*
            ** --- Extract iv buffer...
            */
            cciStatus = cciCtxAttrGet( cciContext, CCI_CIPHER_IV,
                                        &pIvBuffer );
            if( !CCISUCCESS( cciStatus ) )
            {
                /*
                ** --- Tell application we're missing IV buffer
                */
                cciCtxMissingAttrSet( cciContext, CCI_CIPHER_IV );
                mvOsPrintf("CBC mode: IV buffer missing - status=%d\n", cciStatus);

                return( S_cciLib_MISSING_ATTRIBUTE );
            }
			inputMBuf.numFrags 	= 2;
			inputBuf[0].bufSize 		= ivLength;
			inputBuf[0].bufPhysAddr 	= mvOsIoVirtToPhy(NULL,pIvBuffer); 
			inputBuf[0].bufVirtPtr  	= pIvBuffer; 
			outputBuf[0].bufSize 		= ivLength;
			outputBuf[0].bufPhysAddr 	= mvOsIoVirtToPhy(NULL,pIvBuffer);
			outputBuf[0].bufVirtPtr  	= pIvBuffer; 
            /* save IV buffer */
			memcpy(ivTemp,inputBuf[1].bufVirtPtr+ inputBuf[1].bufSize - ivLength,ivLength); 
		}
        else
        {
            /*
            ** --- Tell application we're missing IV length
            */
            cciCtxMissingAttrSet( cciContext, CCI_CIPHER_IV_LENGTH );
            mvOsPrintf("CBC mode: IV length missing - status=%d\n", cciStatus);

            return( S_cciLib_MISSING_ATTRIBUTE );
        }
		CciDebugPrintArg(DEBUG_CIPHER,"CBC mode: ivLength=%d, ivBuffer=%p\n", ivLength, pIvBuffer,3,4,5,6);
    }
	else
	{
		ivLength = 0;
	}
    if (2 == inputMBuf.numFrags)
	{
		inputMBuf.pFrags 	= &inputBuf[0];
		outputMBuf.numFrags = 2;
		outputMBuf.pFrags 	= &outputBuf[0];
	}
	else
	{
		inputMBuf.pFrags 	= &inputBuf[1];
		outputMBuf.numFrags = 1;
		outputMBuf.pFrags 	= &outputBuf[1];
	}

    inputMBuf.mbufSize 	= inputBuf[1].bufSize + ivLength ;
    
    outputMBuf.mbufSize = outputBuf[1].bufSize + ivLength ;
    command.sessionId 	= pCipher->sessionId ;
    command.cryptoOffset= ivLength;
    command.cryptoLength= inputBuf[1].bufSize;
    command.ivFromUser  = MV_TRUE;
    command.ivOffset 	= 0;
	command.pSrc	 	= &inputMBuf;
	command.pDst	 	= &outputMBuf;
	command.pReqPrv		= (void *)pCipher->WaitReadySemId;
	command.skipFlush	= MV_FALSE;

    /*
    ** --- Submit request for processing...
    */
	if (MV_OK != mvCesaDoCommand(&command))
		return S_cciLib_FAILURE;

	/*
    ** --- Check for successful completion...
    */
	mvCesaWaitPreviousCommandCompleted(pCipher->WaitReadySemId);

	if ( pCipher->cipherMode == CCI_MODE_CBC )
	{
		if (pCipher->direction == CCI_ENCRYPT)
		{
			memcpy(pIvBuffer, 
                   outputBuf[1].bufVirtPtr + outputBuf[1].bufSize - ivLength, 
                   ivLength);
		} 
        else
		{
			memcpy(pIvBuffer,ivTemp,ivLength);
		}
	}

	CipherClose(pCipher);
    cciStatus = CCI_SUCCESS;

    return( cciStatus );
}

void CipherClose(VX_CCI_CIPHER *pCipher)
{
	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
	if (mvCesaSessionClose(pCipher->sessionId ) != MV_OK)
	{
		mvOsPrintf("CESA close session failed\n");
	}
	mvOsSemSignal(mvCesaSemId);
	mvOsSemDelete(pCipher->WaitReadySemId);
	mvOsFree(pCipher);
}
																				   

/**************************************************************************************
* Method Name: mvCciSessionHandler_CipherClose
* Description: Close CESA Session handler for AES, DES and 3DES
* requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/

cci_st mvCciSessionHandler_CipherClose( const CCIContext cciContext )
{
	CipherCloseCounter++;
    return( CCI_SUCCESS );
}

/*
** --- Security tag for module #2
*/
static CCISecurityTag mv_cci_security_tag_2[] =
            {0xA8,0x00,0x23,0x87,0x20,0xE8,0xDC,0x20,0x41,0x8E,0x89,0xCE,0xEC,0x7B,0xB4,
             0x3D,0x8A,0x3A,0x38,0xF7};

CCISecurityTag *mv_cci_security_funct_2()   
{ 
    return( mv_cci_security_tag_2 ); 
};


/**************************************************************************************
* Method Name: mvCciSessionHandler_Init_Hash
* Description: CESA 'INIT' Session handler for HASH requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_Init_Hash( const CCIContext cciContext )
{
    cci_st cciStatus = CCI_SUCCESS;
    MV_STATUS               status;
    CCI_ALGORITHM_ID 		algorithmId = cciCtxAlgorithmId( cciContext );
    MV_CESA_OPEN_SESSION    openSession;
    MV_16                   sid;
	VX_CCI_HMAC				*pHmac;

	CciDebugPrint(DEBUG_HASH,"--------------------------------------------------\n");
	CciDebugPrintArg(DEBUG_HASH,"&&& Init_HASH               algorithmId=%d\n",algorithmId,2,3,4,5,6);

	HashInitCounter++;

    /*
    ** --- Make sure this is an MD5 or SHA1 HASH request...
    */
    switch(algorithmId)
    {
	case CCI_HASH_MD5:
		openSession.macMode 	= MV_CESA_MAC_MD5;
		openSession.digestSize 	= MV_CESA_MD5_DIGEST_SIZE;
		break;

	case CCI_HASH_SHA1:
		openSession.macMode 	= MV_CESA_MAC_SHA1;
		openSession.digestSize 	= MV_CESA_SHA1_DIGEST_SIZE;
		break;

	default:
		mvOsPrintf("HASH init handler: Unexpected HASH algorithm %d\n", algorithmId);            
		return( S_cciLib_INVALID_ALGORITHM_ID );
	}
    openSession.operation = MV_CESA_MAC_ONLY;
	openSession.direction = MV_CESA_DIR_ENCODE;
	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
    status = mvCesaSessionOpen(&openSession, &sid);
	mvOsSemSignal(mvCesaSemId);
    /*
    ** --- If the request failed, return request structure back to free list
    */
    if(status != MV_OK)
    {
        mvOsPrintf("HASH init handler: Can't open Session - rc=%d\n", status);
        return CCI_FAILURE;
    }
	pHmac = mvOsMalloc(sizeof(VX_CCI_HMAC));
	if (NULL == pHmac)
	{
        mvOsPrintf("Error: _Init_HASH: memory allocation error\n");
        return CCI_FAILURE;
	}
	memset(pHmac,0,sizeof(VX_CCI_HMAC));
	pHmac->pDigest 		= memalign(32, MV_CESA_MAX_DIGEST_SIZE+8);
	if (NULL == pHmac->pDigest)
	{
        mvOsPrintf("Error: mvCciSessionHandler_Init_Hmac: memory allocation error (pDigest)\n");
        return CCI_FAILURE;
	}
   	pHmac->pMacData = memalign(32, MV_CESA_MAX_PKT_SIZE);
	if (NULL == pHmac->pDigest)
	{
        mvOsPrintf("Error: Init_Hmac: Can't allocate %d bytes for pMacData\n",
                        MV_CESA_MAX_PKT_SIZE);
        return CCI_FAILURE;
	}
    /*mvOsPrintf("Hash_init: pDigest=%p, pMacData=%p\n", pHmac->pDigest, pHmac->pMacData);*/

    pHmac->macSize = 0;
	status = mvOsSemCreate("HMAC-ISR",0,1,&pHmac->WaitReadySemId);
    if(status != MV_OK)
    {
        mvOsPrintf("mvCciProviderInit:  mvOsSemCreate is Failed: status = 0x%x\n", status);
        return CCI_FAILURE;
    }
	pHmac->sessionId 	= sid;
	pHmac->numUpdate	= 0;
	pHmac->digestSize 	= openSession.digestSize;

    cciCtxSessionCtxSet(cciContext, (void *)pHmac);    /* save session ID - (update and final operation) */
	CciDebugPrintArg(DEBUG_HASH,"Init_HASH mvCesaSessionOpen return sid=%d, openSession.digestSize=%d\n",sid,openSession.digestSize,3,4,5,6);
	CciDebugPrintArg(DEBUG_HASH,"Exit _Init_HASH: context=%p\n\n", cciContext,2,3,4,5,6);

    return( cciStatus );
}

/**************************************************************************************
* Method Name: mvCciSessionHandler_Update_Hash
* Description: SSP500 'UPDATE' Session handler for HASH requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_Update_Hash( const CCIContext cciContext )
{
    cci_st cciStatus = CCI_SUCCESS;
    MV_16                   sid;
	VX_CCI_HMAC			*pHmac;
    MV_U8               *pInBuf;
    MV_U32              bufSize;

	pHmac 	= (VX_CCI_HMAC *)cciCtxSessionCtxGet(cciContext);
	sid 	= pHmac->sessionId; 

    cciCtxAttrGet( cciContext, CCI_INPUT_BUFFER,  &pInBuf);
    cciCtxAttrGet( cciContext, CCI_INPUT_LENGTH,  &bufSize);

	CciDebugPrintArg(DEBUG_HASH , "&&& Update_Hash: sid = 0x%x, inBuf=%p (%d), \n",
                sid, pInBuf, bufSize,2,3,4);
	HashUpdateCounter++;
    memcpy((pHmac->pMacData + pHmac->macSize), pInBuf, bufSize);
    pHmac->macSize += bufSize;

    return( cciStatus );
}


/***************************************************************************************
* Method Name: mvCciSessionHandler_Final_Hash
* Description: Marvell CESA 'FINAL' Session handler for HASH requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_Final_Hash( const CCIContext cciContext )
{
    cci_st cciStatus = CCI_SUCCESS;
	VX_CCI_HMAC			*pHmac;
    MV_STATUS           status;
	MV_16					sid;
    cci_b *digest;
    cci_t digestLength;

	HashFinalCounter++;
    pHmac = (VX_CCI_HMAC *)cciCtxSessionCtxGet( cciContext );
	sid = pHmac->sessionId;

    /*
    ** --- Get context attributes...
    */
    cciCtxAttrGet( cciContext, CCI_OUTPUT_BUFFER, &digest);

    cciCtxAttrGet( cciContext, CCI_OUTPUT_LENGTH, &digestLength );
	CciDebugPrintArg(DEBUG_HASH,"\n&&&     Final_Hash: input digest=%p, (%d)\n",digest, digestLength ,2,3,4,5);
	CciDebugPrintArg(DEBUG_HASH,
				   "Final_Hash: pHmac->numByteAlign=%d pHmac->digestSize=%d\n",
					pHmac->numByteAlign, pHmac->digestSize,2,3,4,5);
	pHmac->numByteAlign	= MV_ALIGN_UP(pHmac->macSize, HMAC_ALIGN) - pHmac->macSize;

   	pHmac->inputBuf[0].bufVirtPtr	= pHmac->pMacData;
	pHmac->inputBuf[0].bufSize		= pHmac->macSize;
	pHmac->inputBuf[1].bufVirtPtr	= pHmac->pDigest;
	pHmac->inputBuf[1].bufSize		= pHmac->digestSize + pHmac->numByteAlign;

   	pHmac->outputBuf[0].bufVirtPtr	= pHmac->inputBuf[0].bufVirtPtr;
	pHmac->outputBuf[0].bufSize		= pHmac->inputBuf[0].bufSize;

	pHmac->outputBuf[1].bufVirtPtr	= pHmac->inputBuf[1].bufVirtPtr; 
	pHmac->outputBuf[1].bufSize		= pHmac->inputBuf[1].bufSize;    

   	pHmac->inputMBuf.numFrags  		= 2;
	pHmac->inputMBuf.mbufSize 		= pHmac->inputBuf[0].bufSize + pHmac->inputBuf[1].bufSize;
	pHmac->inputMBuf.pFrags 		= &pHmac->inputBuf[0];

	pHmac->outputMBuf.mbufSize 		= pHmac->outputBuf[0].bufSize + pHmac->outputBuf[1].bufSize; 
	pHmac->outputMBuf.numFrags 		= 2;
	pHmac->outputMBuf.pFrags 		= &pHmac->outputBuf[0];

	pHmac->command.sessionId 		= sid;
	pHmac->command.macOffset   		= 0;
	pHmac->command.macLength   		= pHmac->inputBuf[0].bufSize;
	pHmac->command.ivOffset 		= 0;
	pHmac->command.pSrc	 			= &pHmac->inputMBuf;
	pHmac->command.pDst	 			= &pHmac->outputMBuf;
	pHmac->command.digestOffset		= MV_ALIGN_UP(pHmac->inputBuf[0].bufSize, HMAC_ALIGN);  
	pHmac->command.pReqPrv			= (void *)(pHmac->WaitReadySemId);
	pHmac->command.skipFlush		= MV_FALSE;

	if (MV_OK != mvCesaDoCommand(&pHmac->command))
		return S_cciLib_FAILURE;

    mvCesaWaitPreviousCommandCompleted(pHmac->WaitReadySemId);
    mvOsFree(pHmac->pMacData);
	digestLength = pHmac->digestSize;
	memcpy(digest, pHmac->pDigest + pHmac->numByteAlign, digestLength );
    cciCtxAttrSet( cciContext, CCI_OUTPUT_LENGTH, digestLength  );

	CciDebugPrintArg(DEBUG_HASH, "_Final_Hash digestLength=%d\n",digestLength ,2,3,4,5,6);

	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
	status = mvCesaSessionClose(sid);
	mvOsSemSignal(mvCesaSemId);
	if (status  != MV_OK)
	{
		mvOsPrintf("Final_Hash: close session failed\n");
	}
	mvOsSemDelete(pHmac->WaitReadySemId);
	mvOsFree(pHmac->pDigest);
	mvOsFree(pHmac);
	CciDebugPrintArg (DEBUG_HASH, 
					"&&&-----------------------Exit Final_Hash -------cciContext=%p------------\n", 
					cciContext,2,3,4,5,6);

    return( cciStatus );
}

/*
** --- Security tag for module #3
*/
static CCISecurityTag mv_cci_security_tag_3[] =
        {0x77,0xEC,0x00,0xC7,0x1D,0x6B,0x27,0xB2,0x2B,0x5B,
         0x66,0xA8,0x19,0x6F,0xBC,0x0D,0x5B,0xF8,0x75,0x92};

CCISecurityTag *mv_cci_security_funct_3()
{ 
    return( mv_cci_security_tag_3 ); 
};


/**************************************************************************************
* Method Name: mvCciSessionHandler_Init_Hmac
* Description: Marvell CESA 'INIT' Session handler for HMAC requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
***************************************************************************************/
cci_st mvCciSessionHandler_Init_Hmac( const CCIContext cciContext )
{
    cci_st cciStatus = CCI_SUCCESS;
    MV_STATUS               status;
    CCI_ALGORITHM_ID 		algorithmId = cciCtxAlgorithmId( cciContext );
    void                    *pHmacKey = NULL;
    cci_t                   hmacKeyLength;
    MV_CESA_OPEN_SESSION    openSession;
    MV_16                   sid;
	VX_CCI_HMAC				*pHmac;

	HmacInitCounter++;

    /*
    ** --- Make sure this is an MD5 or SHA1 MAC request...
    */
    switch(algorithmId)
    {
        case CCI_HMAC_MD5:
            openSession.macMode 	= MV_CESA_MAC_HMAC_MD5;
			openSession.digestSize 	= MV_CESA_MD5_DIGEST_SIZE;
            break;

        case CCI_HMAC_SHA1:
            openSession.macMode 	= MV_CESA_MAC_HMAC_SHA1;
            openSession.digestSize 	= MV_CESA_SHA1_DIGEST_SIZE;
            break;

        default:
            mvOsPrintf("HMAC init handler: Unexpected HMAC algorithm %d\n", algorithmId);            
            return( S_cciLib_INVALID_ALGORITHM_ID );
    }
	
    /*
    ** --- Get key attributes...
    */
    cciCtxAttrGet( cciContext, CCI_HMAC_KEY, &pHmacKey );
    cciCtxAttrGet( cciContext, CCI_HMAC_KEY_LENGTH, &hmacKeyLength );
    /*
    ** --- Initialize request...
    */
    openSession.macKeyLength = (MV_U8)hmacKeyLength;
    openSession.operation = MV_CESA_MAC_ONLY;
    memcpy(openSession.macKey, pHmacKey, hmacKeyLength);
	openSession.direction = MV_CESA_DIR_ENCODE;

	CciDebugPrintArg(DEBUG_HMAC, "### Init_Hmac: algorithmId = %d, pHmacKey %p (%d)\n",
			   algorithmId, pHmacKey,hmacKeyLength,4,5,6);

	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
    status = mvCesaSessionOpen(&openSession, &sid);
	mvOsSemSignal(mvCesaSemId);
    /*
    ** --- If the request failed, return request structure back to free list
    */
    if(status != MV_OK)
    {
        mvOsPrintf("HMAC init handler: Can't open Session - rc=%d\n", status);
        return CCI_FAILURE;
    }
	pHmac = mvOsMalloc(sizeof(VX_CCI_HMAC));
	if (NULL == pHmac)
	{
        mvOsPrintf("Error: mvCciSessionHandler_Init_Hmac: memory allocation error\n");
        return CCI_FAILURE;
	}
	memset(pHmac,0,sizeof(VX_CCI_HMAC));
	pHmac->pDigest 		= memalign(32, MV_CESA_MAX_DIGEST_SIZE+8);
	if (NULL == pHmac->pDigest)
	{
        mvOsPrintf("Error: mvCciSessionHandler_Init_Hmac: memory allocation error (pDigest)\n");
        return CCI_FAILURE;
	}
	status = mvOsSemCreate("HMAC-ISR",0,1,&pHmac->WaitReadySemId);
    if(status != MV_OK)
    {
        mvOsPrintf("mvCciProviderInit:  mvOsSemCreate is Failed: status = 0x%x\n", status);
        return CCI_FAILURE;
    }
	pHmac->sessionId 	= sid;
	pHmac->numUpdate	= 0;
	pHmac->digestSize 	= openSession.digestSize;

    cciCtxSessionCtxSet(cciContext, (void *)pHmac);    /* save session ID - (update and final operation) */

    return( cciStatus );
}

/**************************************************************************************
* Method Name: mvCciSessionHandler_Update_Hmac
* Description: SSP500 'UPDATE' Session handler for HMAC requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_Update_Hmac( const CCIContext cciContext )
{
    cci_st cciStatus = CCI_SUCCESS;

    MV_16                   sid;
	VX_CCI_HMAC			*pHmac;

	HmacUpdateCounter++;
    /*
    ** --- Get context attributes...
    */
	pHmac 	= (VX_CCI_HMAC *)cciCtxSessionCtxGet(cciContext);
	sid 	= pHmac->sessionId; 
	if (pHmac->numUpdate >= 1)
	{
		mvOsPrintf(" ****** Update_Hmac: to many update bufferd *******\n");
	   return CCI_FAILURE;
	}
	pHmac->numUpdate++;
    cciCtxAttrGet( cciContext, CCI_INPUT_BUFFER,  &pHmac->inputBuf[0].bufVirtPtr); 
    cciCtxAttrGet( cciContext, CCI_INPUT_LENGTH,  &pHmac->inputBuf[0].bufSize);
	pHmac->inputBuf[0].bufPhysAddr 	= mvOsIoVirtToPhy(NULL, pHmac->inputBuf[0].bufVirtPtr);

	CciDebugPrintArg(DEBUG_HMAC, 
					  "### Update_Hmac: sid = 0x%x, inBuf=%p (%d), \n",
					  sid, pHmac->inputBuf[0].bufVirtPtr, pHmac->inputBuf[0].bufSize,4,5,6);

	pHmac->numByteAlign   			= MV_ALIGN_UP(pHmac->inputBuf[0].bufSize,HMAC_ALIGN) - pHmac->inputBuf[0].bufSize;
	pHmac->inputBuf[1].bufVirtPtr	= pHmac->pDigest;
	pHmac->inputBuf[1].bufSize		= pHmac->digestSize + pHmac->numByteAlign ;
	pHmac->inputBuf[1].bufPhysAddr 	= mvOsIoVirtToPhy(NULL, pHmac->inputBuf[1].bufVirtPtr);

	pHmac->outputBuf[0].bufVirtPtr	= pHmac->inputBuf[0].bufVirtPtr;
	pHmac->outputBuf[0].bufSize		= pHmac->inputBuf[0].bufSize;
	pHmac->outputBuf[0].bufPhysAddr = pHmac->inputBuf[0].bufPhysAddr;

	pHmac->outputBuf[1].bufVirtPtr	= pHmac->inputBuf[1].bufVirtPtr;
	pHmac->outputBuf[1].bufSize		= pHmac->inputBuf[1].bufSize;
	pHmac->outputBuf[1].bufPhysAddr = pHmac->inputBuf[1].bufPhysAddr;


    /*
    ** --- Submit request for processing...
    */
	pHmac->inputMBuf.numFrags  	= 2;
	pHmac->inputMBuf.mbufSize 	= pHmac->inputBuf[0].bufSize+ pHmac->inputBuf[1].bufSize;
	pHmac->inputMBuf.pFrags 	= &pHmac->inputBuf[0];

	pHmac->outputMBuf.numFrags 	= 2;
	pHmac->outputMBuf.mbufSize 	= pHmac->outputBuf[0].bufSize + pHmac->outputBuf[1].bufSize; 
	pHmac->outputMBuf.pFrags 	= &pHmac->outputBuf[0];

	pHmac->command.sessionId 	= sid;
	pHmac->command.macOffset   	= 0;
	pHmac->command.macLength   	= pHmac->inputBuf[0].bufSize;
	pHmac->command.ivOffset 	= 0;
	pHmac->command.pSrc	 		= &pHmac->inputMBuf;
	pHmac->command.pDst	 		= &pHmac->outputMBuf;
	pHmac->command.digestOffset	= MV_ALIGN_UP(pHmac->inputBuf[0].bufSize,HMAC_ALIGN);  
	pHmac->command.pReqPrv		= (void *)pHmac->WaitReadySemId;
	pHmac->command.skipFlush	= MV_FALSE;
	if (MV_OK != mvCesaDoCommand(&pHmac->command))
		return S_cciLib_FAILURE;
    /*
    ** --- Check for successful completion...
    */
	CciDebugPrint(DEBUG_HMAC ,"### Update_Hmac:  Exit OK \n");

	mvCesaWaitPreviousCommandCompleted(pHmac->WaitReadySemId);

    return( cciStatus );
}

/**************************************************************************************
* Method Name: mvCciSessionHandler_Final_Hmac
* Description: SSP500 'FINAL' Session handler for HMAC requests
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
cci_st mvCciSessionHandler_Final_Hmac( const CCIContext cciContext )
{
    cci_st 				cciStatus = CCI_SUCCESS;
    MV_STATUS           status;
	VX_CCI_HMAC		*pHmac;
	MV_16				sid;
    cci_b 				*digest;
    cci_t 				digestLength;

	HmacFinalCounter++;
    pHmac = (VX_CCI_HMAC *)cciCtxSessionCtxGet( cciContext );
	sid = pHmac->sessionId;

    /*
    ** --- Get context attributes...
    */
    cciCtxAttrGet( cciContext, CCI_OUTPUT_BUFFER, &digest);
    cciCtxAttrGet( cciContext, CCI_OUTPUT_LENGTH, &digestLength );
	CciDebugPrintArg(DEBUG_HMAC,
					 "### _Final_Hmac input digestLength=%d\n",digestLength,2,3,4,5,6);
	CciDebugPrintDATA(DEBUG_HMAC, "digest: ",digest,pHmac->digestSize);
	CciDebugPrintArg(DEBUG_HMAC,
					 "### _Final_Hmac output pHmac->numByteAlign=%d pHmac->digestSize=%d\n",
					 pHmac->numByteAlign, pHmac->digestSize,3,4,5,6);
    CciDebugPrintDATA(DEBUG_HMAC,"pHmac->pDigest:", 
					  pHmac->pDigest,pHmac->numByteAlign+pHmac->digestSize);

	digestLength = pHmac->digestSize;
	memcpy(digest,pHmac->pDigest+pHmac->numByteAlign,digestLength );
    cciCtxAttrSet( cciContext, CCI_OUTPUT_LENGTH, digestLength  );

	CciDebugPrintArg(DEBUG_HMAC,
					 "### _Final_Hmac digestLength=%d\n",digestLength,2,3,4,5,6 );
	CciDebugPrintDATA(DEBUG_HMAC,"pHmac->pDigest: ", pHmac->pDigest,pHmac->numByteAlign+pHmac->digestSize);
	CciDebugPrintDATA(DEBUG_HMAC,"digest",digest,pHmac->digestSize);

	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
	status = mvCesaSessionClose(sid);
	mvOsSemSignal(mvCesaSemId);
	if (status  != MV_OK)
	{
		mvOsPrintf("mvCciSessionHandler_Final_Hmac: close session failed\n");
	}
	mvOsSemDelete(pHmac->WaitReadySemId);
	mvOsFree(pHmac->pDigest);
	mvOsFree(pHmac);
    return( cciStatus );
}
/**************************************************************************************
* Method Name: mvCesaWaitPreviousCommandCompleted
* Description: wait for previous CESA command to be completed
*
* Parameters: CCIContext object
*
*
* Returns: CCI_SUCCESS if successful. Otherwise error.
*
**************************************************************************************/
void mvCesaWaitPreviousCommandCompleted(unsigned long SemId)
{
    MV_STATUS               status;

	cesaSemaWaitCounter++;
	status = mvOsSemWait(SemId, 1000);
	if (status != MV_OK)
	{
		mvOsPrintf("WaitCompleted ERROR: status=%d, tid=0x%x, semId=0x%x\n", 
                    status, taskIdSelf(), SemId);
	}
}

MV_STATUS   mvCesaDoCommand (MV_CESA_COMMAND* pCmd)
{
    MV_STATUS      	rc;

	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
	rc = mvCesaAction(pCmd);
	mvOsSemSignal(mvCesaSemId);

	cesaDoCmdCounter++;
	if(rc == MV_NO_RESOURCE)
	{
		mvOsPrintf("mvCesaAction NO resource: rc=%d\n", rc);
		taskDelay(100);
		mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
		rc = mvCesaAction(pCmd);
		mvOsSemSignal(mvCesaSemId);
		if(rc == MV_NO_RESOURCE)
		{
			mvOsPrintf("CESA Test timeout: \n");
			return MV_TIMEOUT;
		}
	}
    /*
    ** --- If the request failed, return request structure back to free pool...
    */
	if( (rc != MV_OK) && (rc != MV_NO_MORE) )
	{
		mvOsPrintf("mvCesaAction failed: rc=%d\n", rc);
		return MV_FAIL;
	}
	return MV_OK;
}

void mvCciCountersShow (int clear)
{
	mvOsPrintf("Hash Init Counter        = %d\n", HashInitCounter    );
	mvOsPrintf("Hash Update Counter      = %d\n", HashUpdateCounter  ); 
	mvOsPrintf("Hash Final Counter       = %d\n", HashFinalCounter   ); 
	mvOsPrintf("Hmac Init Counter        = %d\n", HmacInitCounter    ); 
	mvOsPrintf("Hmac Update Counter      = %d\n", HmacUpdateCounter  ); 
	mvOsPrintf("Hmac Final Counter       = %d\n", HmacFinalCounter   ); 
	mvOsPrintf("Cipher Open Counter      = %d\n", CipherOpenCounter); 
	mvOsPrintf("Cipher Update Counter    = %d\n", CipherUpdateCounter); 
	mvOsPrintf("Cipher Close Counter     = %d\n", CipherCloseCounter );  
	mvOsPrintf("Cesa Do CMD Counter      = %d\n", cesaDoCmdCounter   );  
	mvOsPrintf("Cesa Wait Counter        = %d\n", cesaSemaWaitCounter);  
	mvOsPrintf("Cesa Isr  Counter        = %d\n", cesaTestIsrCounter );  
	mvOsPrintf("Cesa Task  Counter       = %d\n", cesaTestTaskCounter );  
	mvOsPrintf("Cesa BUSY Counter        = %d\n", cesaSemaWait4busyCounter);
	if ((clear == 123) || (clear == 0x123))
	{
		HashInitCounter    = HashUpdateCounter  = HashFinalCounter   =
		HmacInitCounter    = HmacUpdateCounter  = HmacFinalCounter   =
		CipherOpenCounter  = CipherUpdateCounter= CipherCloseCounter =
		cesaDoCmdCounter   = cesaSemaWaitCounter =
		cesaTestIsrCounter = cesaTestTaskCounter= cesaSemaWait4busyCounter = 0;
	}
#if ((CCI_MAJOR_VER > 3) || ((CCI_MAJOR_VER == 3) && (CCI_MINOR_VER > 1)))
	mvOsPrintf("SinglePass Proc Counter  = %d\n", SinglePassProcCounter);
    mvOsPrintf("SinglePass Error Counter = %d\n", SinglePassErrorCounter);
	mvOsPrintf("SinglePass Open Counter  = %d\n", SinglePassOpenCounter );
	mvOsPrintf("SinglePass Close Counter = %d\n", SinglePassCloseCounter);
	mvOsPrintf("SinglePass EncryptOnly Counter  = %d\n", SinglePassEncryptOnly);
	mvOsPrintf("SinglePass EncryptAuth Counter  = %d\n", SinglePassEncryptAuth);
    mvOsPrintf("SinglePass EncryptAuth Bytes    = %d\n", SinglePassEncryptAuthBytes);
	mvOsPrintf("SinglePass DecryptOnly Counter  = %d\n", SinglePassDecryptOnly);
	mvOsPrintf("SinglePass DecryptAuth Counter  = %d\n", SinglePassDecryptAuth);
    mvOsPrintf("SinglePass DecryptAuth Bytes    = %d\n", SinglePassDecryptAuthBytes);
	if ((clear == 123) || (clear == 0x123))
	{
        SinglePassProcCounter = 0;
        SinglePassErrorCounter = 0;
        SinglePassOpenCounter  = 0;
        SinglePassCloseCounter = 0;
        SinglePassEncryptOnly = 0;
        SinglePassEncryptAuth = 0;
        SinglePassDecryptOnly = 0;
        SinglePassDecryptAuth = 0;
        SinglePassEncryptAuthBytes = 0;
        SinglePassDecryptAuthBytes = 0;
	}
#endif /* CCI version > 3.1 */
}


static  void   cesaTestReadyIsr(void)
{
    MV_U32          cause;

    cesaTestIsrCounter++;
    /* Clear cause register */
    cause = MV_REG_READ(MV_CESA_ISR_CAUSE_REG);
    if( (cause & MV_CESA_CAUSE_ACC_DMA_ALL_MASK) == 0)
    {
        logMsg("cesaTestReadyIsr: cause=0x%x!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", cause,2,3,4,5,6);
        return;
    }
    MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG, 0);
	mvOsSemSignal(vxCesaWaitSemId);
}


unsigned __TASKCONV cesaReadyTask(void* args)
{
    MV_STATUS               status;
	MV_CESA_RESULT			result;
	while(1)
	{
		mvOsSemWait(vxCesaWaitSemId,MV_OS_WAIT_FOREVER);
		cesaTestTaskCounter++;
        mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
		status = mvCesaReadyGet(&result);
        mvOsSemSignal(mvCesaSemId);
		if(status != MV_OK)
		{
			cesaSemaWait4busyCounter++;
			if(status != MV_BUSY)
			{
				mvOsPrintf(" ********cesaReadyTask:  mvCesaReadyGet return error %d *******\n",status,2,3,4,5,6);
			}
		}
		else
		{
			mvOsSemSignal((unsigned long)result.pReqPrv); 
		}
	}
}
#if ((CCI_MAJOR_VER > 3) || ((CCI_MAJOR_VER == 3) && (CCI_MINOR_VER > 1)))

#define SESSION_ID_NOT_INITIALIZE 	0xFFF

typedef struct 
{
    short               cipherSid;
    short               hmacSid;
    short               combiSid;
	unsigned long 		WaitReadySemId;   

	MV_U32				digestSize;
    MV_CESA_MBUF        inputMBuf, outputMBuf;
    MV_CESA_COMMAND     command;

} VX_CCI_SINGLE_PASS;
#undef	INCLUDE_VXW_CCI_DEFAULT
#ifdef	INCLUDE_VXW_CCI_DEFAULT
#include "vxCci_DefaultSinglePass.c"
#endif
cci_st mvCciSessionHandler_SinglePass( const CCIContext cciContext )
{
	cci_st cciStatus = CCI_SUCCESS;
#ifdef	INCLUDE_VXW_CCI_DEFAULT
	SinglePassProcCounter++;
	CciDebugPrintArg(DEBUG_SINGLEPASS, "ENTER SinglePass Handle: cciContext=%p  No=%d\n", 
                    cciContext,SinglePassProcCounter,3,4,5,6);
	cciStatus = __ccip_default_singlepass_session(cciContext);	
#else
    MV_STATUS               status;
    MV_BUF_INFO             inputBuf[4], outputBuf[4];
    MV_CESA_OPEN_SESSION    openSession;
    MV_CESA_MBUF        	inputMBuf, outputMBuf;
    MV_CESA_COMMAND     	command;
	VX_CCI_SINGLE_PASS		*pSinglePass;
	MV_16					sid;
    int                     buf;
	MV_8					ivTemp[MV_CESA_MAX_IV_LENGTH];

	cci_b *inputBuffer, *outputBuffer, *iv, *key;
	cci_t direction, keyLength, ivLength, cipherMode, inputLength, *outputLength;
	cci_b *digest, *hmacBuffer, *hmacKey;
	cci_t hmacLength, hmacKeyLength, *digestLength, truncateLength = 0;
	CCI_ALGORITHM_ID cipherAlgId, hmacAlgId;

	SinglePassProcCounter++;

	pSinglePass 	= (VX_CCI_SINGLE_PASS *)cciCtxSessionCtxGet(cciContext);

	CciDebugPrintArg(DEBUG_SINGLEPASS, 
        "ENTER SinglePass Handle: cciContext=%p  No=%d\n", 
            cciContext,SinglePassProcCounter,3,4,5,6);

	/*
	** --- Get cipher attributes
	*/
	cciCtxAttrGet( cciContext, CCI_IPSEC_OPERATION, &direction );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER, &cipherAlgId );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_KEY, &key );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_KEY_LENGTH, &keyLength );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_MODE, &cipherMode );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_IV, &iv );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_IV_LENGTH, &ivLength );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_INPUT_BUFFER, &inputBuffer );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_INPUT_LENGTH, &inputLength );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_OUTPUT_BUFFER, &outputBuffer );
	cciCtxAttrGet( cciContext, CCI_IPSEC_CIPHER_OUTPUT_LENGTH, &outputLength );

    if( (inputLength != *outputLength) || (inputBuffer != outputBuffer) )
    {
  	    mvOsPrintf("\t%p ->cipherInBuf,  inputLength  =%d\n", inputBuffer, inputLength);
	    mvOsPrintf("\t%p ->cipherOutBuf, outputLength =%p (%d)\n", 
                            outputBuffer, outputLength, *outputLength);
    }

	/*
	** --- Get hashing attributes
	*/
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC, &hmacAlgId );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_KEY, &hmacKey );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_KEY_LENGTH, &hmacKeyLength );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_INPUT_BUFFER, &hmacBuffer );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_BUFFER_LENGTH, &hmacLength );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_TRUNCATE_LENGTH, &truncateLength );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_DIGEST, &digest );
	cciCtxAttrGet( cciContext, CCI_IPSEC_HMAC_DIGEST_LENGTH, &digestLength );

#ifdef MV_CESA_DEBUG
    if(mvCesaDebugFlag & DEBUG_SINGLEPASS)
    {
        cci_t               ipsecMode, ipsecProto;

        cciCtxAttrGet( cciContext, CCI_IPSEC_MODE,	   &ipsecMode);
        cciCtxAttrGet( cciContext, CCI_IPSEC_PROTOCOL, &ipsecProto);		

        mvOsPrintf("mode=%d, proto=%d, dir=%s, %s (%d), %s (%d), %d (%d), hmacLen=%d\n", 
			       ipsecMode, ipsecProto, (direction == CCI_ENCRYPT) ? "ENCRYPT":"DECRYPT",
			       AlgName[cipherAlgId], cipherAlgId, CipherModeName[cipherMode], cipherMode,
                   hmacAlgId, hmacAlgId, truncateLength);

	    mvOsPrintf("\t%p ->ciperIV,      ivLength     =%d\n", iv, ivLength);
	    mvOsPrintf("\t%p ->cipherKey,    keyLength    =%d\n", key, keyLength);
	    mvOsPrintf("\t%p ->cipherInBuf,  inputLength  =%d\n", inputBuffer, inputLength);
	    mvOsPrintf("\t%p ->cipherOutBuf, outputLength =%p (%d)\n", 
                            outputBuffer, outputLength, *outputLength);

	mvOsPrintf("\t%p ->hmacKey, hmacKeyLength=%d \n", hmacKey,hmacKeyLength);
	    mvOsPrintf("\t%p ->hmacBuffer,   hmacLength   =%d\n",hmacBuffer, hmacLength);
	    mvOsPrintf("\t%p ->hmacdigest,   digestLength =%p (%d)\n", digest, digestLength, *digestLength);
    }
#endif /* MV_CESA_DEBUG */
/*——————————————————————————————————————————————————————————————————————————
	**--- Determine HMAC algorithm to use */
    openSession.digestSize = 0;
    if( cciCtxIsAttrSet( cciContext, CCI_IPSEC_HMAC_TRUNCATE_LENGTH) )
        openSession.digestSize = truncateLength;
	switch(hmacAlgId)
	{
		case CCI_HMAC_MD5:
			openSession.macMode     = MV_CESA_MAC_HMAC_MD5;
            if(openSession.digestSize == 0)
			openSession.digestSize 	= MV_CESA_MD5_DIGEST_SIZE;
			break;

		case CCI_HMAC_SHA1:
			openSession.macMode     = MV_CESA_MAC_HMAC_SHA1;
            if(openSession.digestSize == 0)
			openSession.digestSize 	= MV_CESA_SHA1_DIGEST_SIZE;
			break;

      case CCI_HMAC_NONE:
			openSession.macMode     = MV_CESA_MAC_NULL;
			openSession.digestSize 	= 0;
			break;

		default:
			mvOsPrintf("HMAC init handler: Unexpected HMAC algorithm %d\n", hmacAlgId);            
			return( CCI_ALGORITHM_NOT_SUPPORTED );
	}
/*——————————————————————————————————————————————————————————————————————————
    ** --- The MV_CESA only supports ECB and CBC modes --- **/
	switch (cipherMode)
	{
	case CCI_MODE_ECB:
		openSession.cryptoMode = MV_CESA_CRYPTO_ECB;
		break;

	case CCI_MODE_CBC:
		openSession.cryptoMode = MV_CESA_CRYPTO_CBC;
		break;

	default:
		mvOsPrintf("SinglePass handler: Unexpected crypto mode %d\n", cipherMode);
		return( CCI_CIPHER_MODE_NOT_SUPPORTED );
	}
/*——————————————————————————————————————————————————————————————————————————*/
	switch (cipherAlgId)
	{
	case CCI_CIPHER_DES:
		openSession.cryptoAlgorithm = MV_CESA_CRYPTO_DES;
		break;

	case CCI_CIPHER_3DES:
		openSession.cryptoAlgorithm = MV_CESA_CRYPTO_3DES;
		break;

	case CCI_CIPHER_AES:
		openSession.cryptoAlgorithm = MV_CESA_CRYPTO_AES;
		break;

	default:
		mvOsPrintf("SinglePass handler: Unexpected crypto algorithm %d\n", cipherAlgId);            
		return( CCI_ALGORITHM_NOT_SUPPORTED );
	}
/*——————————————————————————————————————————————————————————————————————————*/
	if ( direction == CCI_ENCRYPT )
	{
		openSession.direction = MV_CESA_DIR_ENCODE;
		
        if (hmacAlgId == CCI_HMAC_NONE)
		{
			openSession.operation = MV_CESA_CRYPTO_ONLY;
			if (SESSION_ID_NOT_INITIALIZE == pSinglePass->cipherSid)
			{
				openSession.cryptoKeyLength = keyLength;
				memcpy(openSession.cryptoKey, key, keyLength);
				mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
				status = mvCesaSessionOpen(&openSession, &pSinglePass->cipherSid);
				mvOsSemSignal(mvCesaSemId);
				/*
				** --- If the request failed, return request structure back to free list
				*/
#ifdef MV_CESA_DEBUG
                mvOsPrintf("**** Open Encode CRYPTO only Session: context=%p, pSinglePass=%p, sid=%d\n",
                            cciContext, pSinglePass, pSinglePass->cipherSid);
#endif
				if(status != MV_OK)
				{
					mvOsPrintf("SinglePass handler: Can't open Session - rc=%d\n", status);
					return CCI_FAILURE;
				}
			}
			sid = pSinglePass->cipherSid;
            CciDebugPrintArg(DEBUG_SINGLEPASS, 
                "----- Encrypt MV_CESA_CRYPTO_ONLY: sid=%d -------\n", sid,2,3,4,5,6);
		}
        else if(cipherAlgId == CCI_CIPHER_NULL)
        {
            mvOsPrintf("Encrypt MV_CESA_MAC_ONLY: NOT supported !!!!!\n");
            return( CCI_ALGORITHM_NOT_SUPPORTED );
        }
        else
        {
			openSession.operation = MV_CESA_CRYPTO_THEN_MAC;
			if (SESSION_ID_NOT_INITIALIZE == pSinglePass->combiSid)
			{
				openSession.cryptoKeyLength = keyLength;
				memcpy(openSession.cryptoKey, key, keyLength);
				openSession.macKeyLength = (MV_U8)hmacKeyLength;
				memcpy(openSession.macKey, hmacKey, hmacKeyLength);
				mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
				status = mvCesaSessionOpen(&openSession, &pSinglePass->combiSid);
				mvOsSemSignal(mvCesaSemId);
				/*
				** --- If the request failed, return request structure back to free list
				*/
#ifdef MV_CESA_DEBUG
                mvOsPrintf("**** Open Encode Combi Session: context=%p, pSinglePass=%p, sid=%d\n",
                            cciContext, pSinglePass, pSinglePass->combiSid);
#endif
				if(status != MV_OK)
				{
					mvOsPrintf("SinglePass handler: Can't open Session - rc=%d\n", status);
					return CCI_FAILURE;
				}
			}
			sid = pSinglePass->combiSid;
			pSinglePass->digestSize	= openSession.digestSize;
            CciDebugPrintArg(DEBUG_SINGLEPASS, 
                    "----- Encrypt MV_CESA_CRYPTO_THEN_MAC: sid=%d -------\n", sid,2,3,4,5,6);
        }

        buf = 0;
        inputMBuf.mbufSize = outputMBuf.mbufSize = 0;
	    inputMBuf.pFrags 	= &inputBuf[buf];
		outputMBuf.pFrags 	= &outputBuf[buf];

		if (hmacAlgId == CCI_HMAC_NONE)
		{
            SinglePassEncryptOnly++;
/*
            mvOsPrintf("EncryptOnly: mode=%d, iv=%p (%d), input=%p (%d), output=%p (%p=%d)\n",
                        cipherMode, iv, ivLength, inputBuffer, inputLength, 
                        outputBuffer, outputLength, *outputLength);
*/
            /* Encrypt Data only */
		if( cipherMode == CCI_MODE_CBC )
		{
			    inputBuf[buf].bufSize 	 = outputBuf[buf].bufSize    = ivLength;
			    inputBuf[buf].bufVirtPtr = outputBuf[buf].bufVirtPtr = iv; 
                inputMBuf.mbufSize = outputMBuf.mbufSize += ivLength;
                buf++;
		    }

   		    outputBuf[buf].bufSize = inputBuf[buf].bufSize = inputLength;
	        outputBuf[buf].bufVirtPtr = outputBuffer;
            inputBuf[buf].bufVirtPtr = inputBuffer; 
            inputMBuf.mbufSize = outputMBuf.mbufSize += inputLength;
            buf++;

            command.ivOffset     = 0; 
   		    command.cryptoOffset = ivLength;
		}
		else
		{
            SinglePassEncryptAuth++;
            SinglePassEncryptAuthBytes += hmacLength + inputLength + pSinglePass->digestSize;
            /* ESP Encryption + Authentication */
            if( cipherMode != CCI_MODE_CBC )
            {
                mvOsPrintf("ESP Encrypt: cipherMode (%d) != CCI_MODE_CBC!!!!!\n", cipherMode);
                return CCI_FAILURE;
            }
            if( memcmp(hmacBuffer+8, iv, ivLength) != 0)
            {
                mvOsPrintf("IV in the ESP data is not set\n");
                mvOsPrintf("hmacBuffer=%p, hmacLength=%d\n", hmacBuffer, hmacLength);
                mvDebugMemDump(hmacBuffer, hmacLength, 1);                    
                mvOsPrintf("IV=%p, ivLength=%d\n", hmacBuffer, ivLength);
                mvDebugMemDump(hmacBuffer, ivLength, 1);                
				return CCI_FAILURE;
            }
            if( ((hmacBuffer + hmacLength) != inputBuffer) ||
                ((inputBuffer + inputLength) != digest) )
            {
                mvOsPrintf("ESP Encrypt: buffer is not continious !!!!!\n");
                return CCI_FAILURE;
            }
			inputBuf[buf].bufVirtPtr  	= hmacBuffer; 
			inputBuf[buf].bufSize 		= hmacLength + inputLength + pSinglePass->digestSize;
			outputBuf[buf].bufVirtPtr  	= hmacBuffer; 
			outputBuf[buf].bufSize 		= hmacLength + inputLength + pSinglePass->digestSize;
            inputMBuf.mbufSize = outputMBuf.mbufSize = 
                        hmacLength + inputLength + pSinglePass->digestSize;
            buf++;

			command.macOffset   		= 0;
			command.macLength   		= hmacLength + inputLength;
            command.digestOffset        = hmacLength + inputLength;

            command.ivOffset            = 8;
   		    command.cryptoOffset        = hmacLength;
		}
		inputMBuf.numFrags = outputMBuf.numFrags = buf;

        command.ivFromUser	= TRUE;
		command.cryptoLength    = inputLength;
		command.sessionId	= sid;
		command.pSrc	 	= &inputMBuf;
		command.pDst	 	= &outputMBuf;
		command.pReqPrv		= (void *)pSinglePass->WaitReadySemId;
		command.skipFlush	= MV_FALSE;
		/*
		** --- Submit request for processing...
		*/
		if (MV_OK != mvCesaDoCommand(&command))
			return CCI_FAILURE;

		/*
		** --- Check for successful completion...
		*/
		mvCesaWaitPreviousCommandCompleted(pSinglePass->WaitReadySemId);

		if ( cipherMode == CCI_MODE_CBC )
		{
            /* Update IV value with last data block */
            memcpy(iv, outputBuffer + inputLength - ivLength, ivLength);
		}
        if(hmacAlgId != CCI_HMAC_NONE)
        {
            /* Digest is already updated */
		    *digestLength = pSinglePass->digestSize;
	    }
    }
	else
	{
		/*——————————————————————————————————————————————————————————————————————————*/
		openSession.direction = MV_CESA_DIR_DECODE;
    
		if ( hmacAlgId != CCI_HMAC_NONE )
		{
			if (SESSION_ID_NOT_INITIALIZE == pSinglePass->combiSid)
			{
				openSession.operation 	 = MV_CESA_MAC_THEN_CRYPTO;
				openSession.macKeyLength = (MV_U8)hmacKeyLength;
				memcpy(openSession.macKey, hmacKey, hmacKeyLength);
				openSession.cryptoKeyLength = keyLength;
				memcpy(openSession.cryptoKey, key, keyLength);
				mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
				status = mvCesaSessionOpen(&openSession, &pSinglePass->combiSid);
				mvOsSemSignal(mvCesaSemId);
				/*
				** --- If the request failed, return request structure back to free list
				*/
#ifdef MV_CESA_DEBUG
                mvOsPrintf("**** Open Decode Combi Session: context=%p, pSinglePass=%p, sid=%d\n",
                            cciContext, pSinglePass, pSinglePass->combiSid);
#endif
				if(status != MV_OK)
				{
					mvOsPrintf("SinglePass handler: Can't open Session - rc=%d\n", status);
					return CCI_FAILURE;
				}
			}
			sid = pSinglePass->combiSid;
            CciDebugPrintArg(DEBUG_SINGLEPASS, 
                    "----- Decrypt MV_CESA_MAC_THEN_CRYPTO: sid=%d -------\n", sid,2,3,4,5,6);
		}
		else
		{
			if (SESSION_ID_NOT_INITIALIZE == pSinglePass->cipherSid)
			{
				openSession.operation 	 = MV_CESA_CRYPTO_ONLY;
				openSession.macKeyLength = 0;
				openSession.cryptoKeyLength = keyLength;
				memcpy(openSession.cryptoKey, key, keyLength);
				mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
				status = mvCesaSessionOpen(&openSession, &pSinglePass->cipherSid);
				mvOsSemSignal(mvCesaSemId);
				/*
				** --- If the request failed, return request structure back to free list
				*/
#ifdef MV_CESA_DEBUG
                mvOsPrintf("**** Open Decode CRYPTO only Session: context=%p, pSinglePass=%p, sid=%d\n",
                            cciContext, pSinglePass, pSinglePass->cipherSid);
#endif
				if(status != MV_OK)
				{
					mvOsPrintf("SinglePass handler: Can't open Session - rc=%d\n", status);
					return CCI_FAILURE;
				}
			}
			sid = pSinglePass->cipherSid;
            CciDebugPrintArg(DEBUG_SINGLEPASS, 
                    "----- Decrypt MV_CESA_CRYPTO_ONLY: sid=%d -------\n", sid,2,3,4,5,6);
		}
		pSinglePass->digestSize	= openSession.digestSize;

        buf = 0;
        inputMBuf.mbufSize = outputMBuf.mbufSize = 0;
	    inputMBuf.pFrags 	= &inputBuf[buf];
		outputMBuf.pFrags 	= &outputBuf[buf];

		if (hmacAlgId == CCI_HMAC_NONE)
		{
            SinglePassDecryptOnly++;
            /* Decrypt Data only */
		if( cipherMode == CCI_MODE_CBC )
		{
			    inputBuf[buf].bufSize 	 = outputBuf[buf].bufSize    = ivLength;
			    inputBuf[buf].bufVirtPtr = outputBuf[buf].bufVirtPtr = iv; 
                inputMBuf.mbufSize = outputMBuf.mbufSize += ivLength;
                buf++;
		    }

   		    outputBuf[buf].bufSize = inputBuf[buf].bufSize = inputLength;
	        outputBuf[buf].bufVirtPtr = outputBuffer;
            inputBuf[buf].bufVirtPtr = inputBuffer; 

            inputMBuf.mbufSize = outputMBuf.mbufSize += inputLength;
            buf++;

            command.ivOffset     = 0; 
   		    command.cryptoOffset = ivLength;
		}
		else
		{
            /* ESP Decryption + Authentication */
            SinglePassDecryptAuth++;
            SinglePassDecryptAuthBytes += hmacLength + inputLength + pSinglePass->digestSize;
            if( memcmp(hmacBuffer+8, iv, ivLength) != 0)
            {
                mvOsPrintf("IV in the ESP data is not set\n");
                mvOsPrintf("hmacBuffer=%p, hmacLength=%d\n", hmacBuffer, hmacLength);
                mvDebugMemDump(hmacBuffer, hmacLength, 1);                    
                mvOsPrintf("IV=%p, ivLength=%d\n", hmacBuffer, ivLength);
                mvDebugMemDump(hmacBuffer, ivLength, 1);                
				return CCI_FAILURE;
            }
            if( ((hmacBuffer + hmacLength) != inputBuffer) ||
                ((inputBuffer + inputLength) != digest) )
            {
                mvOsPrintf("ESP Encrypt: buffer is not continious !!!!!\n");
                return CCI_FAILURE;
            }
            if(hmacLength != (ivLength + 8))
            {
                mvOsPrintf("ESP Decrypt+Auth: hmacLength (%d) != ivLength (%d) + 8\n",
                            hmacLength, ivLength);
                return CCI_FAILURE;
            }
/*
            mvOsPrintf("DecryptAuth: iv=%p (%d), hmac=%p (%d), in=%p (%d), out=%p (%p=%d), dig=%p\n",
                        iv, ivLength, hmacBuffer, hmacLength, inputBuffer, inputLength, 
                        outputBuffer, outputLength, *outputLength, digest);
*/
			inputBuf[buf].bufVirtPtr  	= hmacBuffer; 
			inputBuf[buf].bufSize 		= hmacLength + inputLength + pSinglePass->digestSize;
			outputBuf[buf].bufVirtPtr  	= hmacBuffer; 
			outputBuf[buf].bufSize 		= hmacLength + inputLength + pSinglePass->digestSize;
            inputMBuf.mbufSize = outputMBuf.mbufSize = 
                        hmacLength + inputLength + pSinglePass->digestSize;
            buf++;

			command.macOffset   		= 0;
			command.macLength   		= hmacLength + inputLength;
            command.digestOffset        = hmacLength + inputLength;

            command.ivOffset            = 8;
   		    command.cryptoOffset        = hmacLength;
		}
		inputMBuf.numFrags = outputMBuf.numFrags = buf;

        /* Save last block of Cipher data for next IV */
		if ( cipherMode == CCI_MODE_CBC )
            memcpy(ivTemp, inputBuffer + inputLength - ivLength, ivLength);

        command.ivFromUser		= TRUE;
		command.cryptoLength    = inputLength;
		command.sessionId 	= sid;
		command.pSrc	 	= &inputMBuf;
		command.pDst	 	= &outputMBuf;
		command.pReqPrv		= (void *)pSinglePass->WaitReadySemId;
		command.skipFlush	= MV_FALSE;

		if (MV_OK != mvCesaDoCommand(&command))
			return CCI_FAILURE;

		/*
		** --- Check for successful completion...
		*/
		mvCesaWaitPreviousCommandCompleted(pSinglePass->WaitReadySemId);
        if(hmacAlgId != CCI_HMAC_NONE)
        {
		*digestLength = pSinglePass->digestSize;
        }
		
		if ( cipherMode == CCI_MODE_CBC )
		{
			memcpy(iv,ivTemp,ivLength);
		}
	}  /* ( direction == CCI_ENCRYPT )    */

    *outputLength = inputLength;

#endif
  	CciDebugPrintArg(DEBUG_SINGLEPASS, "EXIT SinglePass Handle: cciContext=%p  \n", 
                    cciContext, 2, 3,4,5,6);
    
    if(cciStatus != CCI_SUCCESS)
        SinglePassErrorCounter++;

    return( cciStatus ); 
}

cci_st mvCci_singlepass_open_session ( const CCIContext cciContext )
{
#ifdef	INCLUDE_VXW_CCI_DEFAULT
	ccip_default_singlepass_open_session(cciContext ) ;
#else
	VX_CCI_SINGLE_PASS		*pSinglePass;
    MV_STATUS       		status;

	pSinglePass = mvOsMalloc(sizeof(VX_CCI_SINGLE_PASS));
	if (NULL == pSinglePass)
	{
        mvOsPrintf("Error: mvCci_singlepass_open_session: memory allocation error\n");
		return CCI_FAILURE;
	}

	memset(pSinglePass,0,sizeof(VX_CCI_SINGLE_PASS));

	status = mvOsSemCreate("SinglePass",0,1,&pSinglePass->WaitReadySemId);
	if (status != MV_OK)
	{
		mvOsPrintf("mvCci_singlepass_open_session:  mvOsSemCreate is Failed: status = 0x%x\n", status);
		return CCI_FAILURE;
	}
	pSinglePass->cipherSid = SESSION_ID_NOT_INITIALIZE;
	pSinglePass->hmacSid   = SESSION_ID_NOT_INITIALIZE;
	pSinglePass->combiSid  = SESSION_ID_NOT_INITIALIZE;

    /* save cipher struct pointer ) */
    cciCtxSessionCtxSet(cciContext, (void *)pSinglePass);    
#endif
	SinglePassOpenCounter++;
	CciDebugPrintArg(DEBUG_SINGLEPASS, 
        "+++++++ SinglePass: Open: cciContext=%p\n", cciContext,2,3,4,5,6);
	return( CCI_SUCCESS );
}
/*************************************************************************************************
* Method Name: mvCci_singlepass_close_session
* Description: Free SinglePass context
*
* Parameters:
* 
*  < cciContext > - CCI crypto request object.
* 
* Returns: cci_st - CCI_STATUS status identifier.
* 
* NOTES: None
*
**************************************************************************************************/
cci_st mvCci_singlepass_close_session( const CCIContext cciContext )  
{
#ifdef	INCLUDE_VXW_CCI_DEFAULT
	ccip_default_singlepass_close_session( cciContext );
#else
	VX_CCI_SINGLE_PASS		*pSinglePass;

	pSinglePass 	= (VX_CCI_SINGLE_PASS *)cciCtxSessionCtxGet(cciContext);
	mvOsSemWait(mvCesaSemId,MV_OS_WAIT_FOREVER);
	if (SESSION_ID_NOT_INITIALIZE != pSinglePass->combiSid)
	{
		if (mvCesaSessionClose(pSinglePass->combiSid) != MV_OK)
		{
			mvOsPrintf("mvCci_singlepass_close_session: combiSid=%d close session failed\n",
                        pSinglePass->combiSid);
		}
	}
	if (SESSION_ID_NOT_INITIALIZE != pSinglePass->cipherSid)
	{
		if (mvCesaSessionClose(pSinglePass->cipherSid) != MV_OK)
		{
			mvOsPrintf("mvCci_singlepass_close_session: cipherSid=%d close session failed\n",
                            pSinglePass->cipherSid);
		}
	}
    cciCtxSessionCtxSet(cciContext, (void *)NULL);
	mvOsSemSignal(mvCesaSemId);
	mvOsSemDelete(pSinglePass->WaitReadySemId);
	mvOsFree(pSinglePass);
#endif
	SinglePassCloseCounter++; 
	CciDebugPrintArg(DEBUG_SINGLEPASS, 
                    "-------- SinglePass: Close: cciContext=%p\n", cciContext,2,3,4,5,6);
	return( CCI_SUCCESS );
}
#endif /* CCI version > 3.1 */
