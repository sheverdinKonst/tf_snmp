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
* mgiEnd.c - Marvell Gigabit Ethernet Controller Network Interface driver
*
*   DESCRIPTION
*       This module implements a Gigabit Ethernet Controller network interface
*       driver. It utilize the Gigabit Ethernet Controller low level driver to
*       introduce VxWorks END interface driver.
*
*       Supported Features:
*       - Zero Copy Buff. This driver implements the Zero Copy Buff
*         methodology. This means no data copy is done during either Rx nor Tx
*         process.
*       - This driver supports Scatter-Gather on TX. When the driver gets a chain
*         of mBlks to transmit it is able to perform Gather-write. it does
*         not need to do any data copying.
*       - Support cached buffers and descriptors for better performance.
*       - This driver utilize the internal device SRAM for descriptor memory
*         area. This reduce access time to the descriptor thus increase
*         overall performance.
*       - This driver supports multicasting.
*       - This driver supports Promiscous or Non-promiscous modes
*       - Easy to use API to manipulate the MAC address (using bootline).
*       NOTE: This deriver does not support polling mode.
*
*       This driver establishes a shared memory for the communication system,
*       which is divided into two parts: The descriptors area (Tx and Rx)
*       and the receive buffer area.
*
*       The receive buffer area is where the device received packet data
*       is stored. This area is curved by the netBufLib to be the END pool
*       of clusters. Those clusters pointers are set to be the Rx descriptors
*       buffers pointers. This means that the received Rx packet is stored
*       directly into the netBufLib clusters with no need to copy
*       data (See netBufLib).
*
*       This driver manages the cluster memory pool by itself. netBuffLib 'get'
*       and 'free' routine are not used in case of cluster return. Release of
*       cluster by upper layers will result calling to local free routine.
*       This saves expensive pool management time.
*
*       The clusters and descriptors must comply to alignment restriction and
*       CPU data cache line alignment restrictions. Thus the cluster and
*       descriptors size are calculate in such way the base address is 32bytes
*       aligned.
*
*       BOARD LAYOUT
*       This device is on-board.  No jumper diagram is necessary.
*
*       EXTERNAL INTERFACE
*
*       The driver provides the standard external interface, required by 
*       vxWorks END driver specification.
*
*       MAC ADDRESS INIT-TIME ASSIGN                                                                                                                           *
*       Default port MAC address can be modified using the system bootline.
*       Assign boot parameter 'other' (o) with user defined MAC address in
*       the following manner:
*       1) In case of running bootrom:
*           - Stop the autoload countdown.
*           - Enter the boot parameter change menu (Type 'c' and then
*             press 'Enter').
*           - At the 'other' parameter type the new MAC address in the
*             following format:
*              <MAC Name> - <byte0:byte1:byte2:byte3:byte4:byte5>.
*             For example, to assign ethernet port 1 with the MAC
*             00:11:22:33:44:55 assign 'other' with the following:
*             other (o):  MAC1-00:11:22:33:44:55
*
*             NOTE: When defining two consecutive MAC addresses, a dash ('-')
*                   character must be placed between the first MAC address
*                   string and the last MAC name. For example to assign
*                   ethernet port 1 with MAC 00:11:22:33:44:55 and
*                   ethernet port 2 with MAC 00:AA:BB:CC:DD:EE assign
*                   'other' with the following:
*                   other (o):  MAC1-00:11:22:33:44:55-MAC2-00:AA:BB:CC:DD:EE
*       2) In case of running stand alone VxWorks image
*           - Run bootChange() routine. This routine introduce the boot
*               parameters change menu.
*           - Change 'other' parameter as defined above.
*           - Reset the system. The new image will bootup using the MAC
*             address you defined.
*
*       SEE ALSO:   ifLib,
*                   endLib,
*                   netBufLib
*                   ethernet module.
*
*******************************************************************************/

#include "mvOs.h"
#include "mvDebug.h"
#include "mv802_3.h"
#include "mvCommon.h"

#include "vxWorks.h"
#include "wdLib.h"
#include "iv.h"
#include "vme.h"
#include "net/mbuf.h"
#include "net/unixLib.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "errno.h"
#include "memLib.h"
#include "intLib.h"
#include "iosLib.h"
#include "errnoLib.h"
#include "vxLib.h"
#include "private/funcBindP.h"

#include "cacheLib.h"
#include "logLib.h"
#include "netLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "sysLib.h"
#include "taskLib.h"

#include "net/systm.h"
#include "sys/times.h"
#if !((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 5))
#include "net/if_subr.h"
#endif
#include "netinet/ip.h"

#include "drv/pci/pciIntLib.h"
#undef  ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"
#include "end.h"
#include "semLib.h"

#define END_MACROS
#include "endLib.h"
#include "lstLib.h"

#include "config.h"

#include "boardEnv/mvBoardEnvLib.h"
#include "eth/mvEth.h"
#include "eth/gbe/mvEthRegs.h"
#include "mgiEnd.h"
#include "ddr2/mvDramIf.h"
#ifdef MV_PPC
#include "mvIntControl.h"
#endif
#if defined (MV78200)
#include "cpu/mvCpu.h"
#include "mv78200/mvSocUnitMap.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Globals */
extern 	char *	sysBootLine; 	/* address of boot line */
	
extern END_TBL_ENTRY    endDevTbl[];

/* Control collection of IF-MIB counters 
 * 0 - Don't collect statistics
 * 1 - Collect statistics in polling mode (if supported)
 * 2 - Collect statistics on per packet base
 */
int     mgiStatsControl = 2;

#ifdef MGI_DEBUG
MV_U32  mgiDebugFlags = MGI_DEBUG_FLAG_STATS;
#endif

#ifdef MGI_DEBUG_TRACE
MV_U8   mgiTrace[MGI_TRACE_ARRAY_SIZE];
int     mgiTraceIdx = 0;
#endif

MV_BOOL   mgiIsFirstTime = MV_TRUE;
MV_U8     mgiUserDefMacAddr[MV_MAC_ADDR_SIZE] = {0x00,0x50,0x43,0x67,0x89,0x00};
MV_U32    mgiMtuSize;
DRV_CTRL* mgiDrvCtrl[BOARD_ETH_PORT_NUM];

int mgiRxQueueDescrNum[MV_ETH_RX_Q_NUM];

int mgiTxQueueDescrNum[MV_ETH_TX_Q_NUM];

/* Forward function declarations */
LOCAL STATUS    mgiNetPoolInit (DRV_CTRL *pDrvCtrl);
LOCAL MV_STATUS mgiEthPortInit(DRV_CTRL *pDrvCtrl);
LOCAL void      mgiRxDone(DRV_CTRL *pDrvCtrl, MV_PKT_INFO *pPktInfo, int queue);
LOCAL void      mgiTxDoneJob (DRV_CTRL *pDrvCtrl, MV_U32 cause);
LOCAL int       mgiTxDone(DRV_CTRL *pDrvCtrl, MV_U32 cause);
void      mgiTxDoneIsr (DRV_CTRL *pDrvCtrl);
void      mgiRxReadyIsr(DRV_CTRL *pDrvCtrl);
LOCAL void      mgiRxReadyJob(DRV_CTRL *pDrvCtrl, MV_U32 cause);
LOCAL  int      mgiRxReady(DRV_CTRL *pDrvCtrl, int rxq, int quota);
LOCAL void      mgiLinkChangeJob(DRV_CTRL *pDrvCtrl, int isLinkUp);
void      mgiMiscIsr(DRV_CTRL *pDrvCtrl);
LOCAL MV_STATUS getBootlineMAC(int portNum, MV_U8 *portMacAddr);
LOCAL char*     macFindInBootline(char *macName);
LOCAL M_BLK_ID  mgiGenerateMblk(DRV_CTRL *pDrvCtrl, MV_PKT_INFO *pPktInfo, int queue);
LOCAL MV_STATUS mgiMblkToPktInfo(DRV_CTRL *pDrvCtrl, M_BLK_ID pMblkPkt, 
                                 MV_PKT_INFO* pPktInfo);
void            mgiPrintMblk( M_BLK_ID pMblk );
void            mgiCheckRxPkt(MV_U8* pData, int size);
void            mgiSetFlags(DRV_CTRL *pDrvCtrl, long oldFlags);
void            mgiCleanup(DRV_CTRL *pDrvCtrl);
MV_STATUS       mgiRxRefill(DRV_CTRL *pDrvCtrl, int queue, int num);
int             mgiFreeRxResources(DRV_CTRL *pDrvCtrl, int queue);
int             mgiFreeTxResources(DRV_CTRL *pDrvCtrl, int queue);




/* END Specific interfaces. */

END_OBJ *       mgiEndLoad (char *initString, void* dummy);
LOCAL STATUS    mgiUnload (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    mgiStart (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    mgiStop (DRV_CTRL *pDrvCtrl);
LOCAL int       mgiIoctl (DRV_CTRL *pDrvCtrl, unsigned int cmd, caddr_t data);
LOCAL STATUS    mgiSend (DRV_CTRL *pDrvCtrl, M_BLK *pMblk);
LOCAL STATUS    mgiMCastAddrAdd (DRV_CTRL *pDrvCtrl, char* pAddress);
LOCAL STATUS    mgiMCastAddrDel (DRV_CTRL *pDrvCtrl, char* pAddress);
LOCAL STATUS    mgiMCastAddrGet (DRV_CTRL *pDrvCtrl, MULTI_TABLE *pTable);
LOCAL STATUS    mgiPollSend (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    mgiPollReceive (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);


/*  Define the device function table.  Static across all driver instances.  */

static NET_FUNCS netFuncs =
{
    (FUNCPTR)mgiStart,          /* Start func.                          */
    (FUNCPTR)mgiStop,           /* Stop func.                           */
    (FUNCPTR)mgiUnload,         /* Unload func.                         */
    (FUNCPTR)mgiIoctl,          /* ioctl func.                          */
    (FUNCPTR)mgiSend,           /* Send func.                           */
    (FUNCPTR)mgiMCastAddrAdd,   /* Multicast add func.                  */
    (FUNCPTR)mgiMCastAddrDel,   /* Multicast delete func.               */
    (FUNCPTR)mgiMCastAddrGet,   /* Multicast get fun.                   */
    (FUNCPTR)mgiPollSend,       /* Polling send func.                   */
    (FUNCPTR)mgiPollReceive,    /* Polling receive func.                */
    endEtherAddressForm,            /* Put address info into a NET_BUFFER.  */
    endEtherPacketDataGet,          /* Get pointer to data in NET_BUFFER.   */
    endEtherPacketAddrGet           /* Get packet addresses.                */
};

static INLINE int mgiRxQueueGet(MV_U32 cause)
{
    int     rxq = MV_ETH_RX_Q_NUM-1;

    for(rxq=(MV_ETH_RX_Q_NUM-1); rxq>0; rxq--)
    {
        if( cause & (1 << rxq) )
            break;
    }
    return rxq;
}

static INLINE int mgiTxQueueGet(M_BLK *pMblkPkt)
{
    MV_802_3_HEADER*    pMacHeader = (MV_802_3_HEADER*)(pMblkPkt->mBlkHdr.mData);
    int                 txq;

    /* Check only IP packets */
    if (pMacHeader->typeOrLen == MV_16BIT_BE(MV_IP_TYPE))
    {
        /* get queue from TOS value */
        struct ip   *pIpHeader;
        char        prio;

        pIpHeader = (struct ip*)(((MV_U8*)pMacHeader) + sizeof(MV_802_3_HEADER));

        /* Map higher values of PRECEDENCE field to existing TX queues */
        prio = pIpHeader->ip_tos >> 5;
        if(prio < (8 - MV_ETH_TX_Q_NUM))
            txq = 0;
        else
            txq = prio - (8 - MV_ETH_TX_Q_NUM);
    }
    else
        txq = ETH_DEF_TXQ;

    return txq;
}

/*******************************************************************************
* mgiMblkClFreeRtn - Free or Reuse Mblk+Clblk+Cluster tuple.
*
* DESCRIPTION:
*       This function replace standard pMblkClFreeRtn from netBufLib.
*       1) If the cluster was used for RX, 
*           - Invalidate used buffer: Maximum of a. number of bytes used for RX,
*                                                b. number of bytes used for TX.
*           - Return Cluster to ethernet RX queue.
*          Else return cluster to netPool of the driver.
*       2) If Cluster can not be freed (clRefCnt > 1)
*           - return only Mlbk;    
*
* INPUT:
*       NET_POOL_ID pNetPool    - pointer to NET_POOL
*       M_BLK_ID    pMblk       - pointer to Mblk+Clblk+Cluster tuple.
*
* RETURN:
*       M_BLK_ID - Pointer to next Mblk if exist.
*
*******************************************************************************/
#ifdef USE_PRIV_NET_POOL_FUNC
POOL_FUNC           mgiPoolFuncTbl;
MV_U32              mgiMblkClFreeCount = 0;
MV_U32              mgiMblkClDoubleFreeCount = 0;
MV_U32              mgiMblkClFreeRtnCount = 0;
MV_U32              mgiMblkFreeCount = 0;
MV_U32              mgiMblkClRefFreeCount = 0;
extern POOL_FUNC*   _pNetPoolFuncTbl;

M_BLK_ID    mgiMblkClFreeRtn(NET_POOL_ID pNetPool, M_BLK_ID pMblk)
{
    M_BLK_ID    pMblkNext = pMblk->mBlkHdr.mNext;

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, 
     ("ENTER mgiMblkClFreeRtn: pNetPool=0x%x\n", (unsigned int)pNetPool) );

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_RX_DONE, 
            ("\n MblkClFreeCount #%u: pNetPool=0x%x, pMblk=0x%x, pMblkNext=0x%x\n",
            mgiMblkClFreeCount, (unsigned int)pNetPool, 
            (unsigned int)pMblk, (unsigned int)pMblk->mBlkHdr.mNext));

    MGI_DEBUG_CODE(MGI_DEBUG_FLAG_RX_DONE|MGI_DEBUG_FLAG_MBLK, 
                   (mgiPrintMblk(pMblk)));

    MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, (mgiMblkClFreeCount++));

    /* free the cluster first if it is attached to the mBlk */
    if( M_HASCL(pMblk) )
    {        
        if(pMblk->pClBlk->clRefCnt == 1)
        {
            if( pMblk->pClBlk->pClFreeRtn == ((FUNCPTR)mgiRxDone) )
            {
                    DRV_CTRL*   pDrvCtrl = (DRV_CTRL*)pMblk->pClBlk->clFreeArg1;
                    MV_PKT_INFO *pktInfo = (MV_PKT_INFO*)pMblk->pClBlk->clFreeArg2;
                    int         queue = (pMblk->pClBlk->clFreeArg3 & 0xffff);

                if( (pMblk->pClBlk->clFreeArg3 & ~0xFFFF) > 0)
                {
                    int         usedSize;

                    MGI_LOCK(pDrvCtrl);
                    usedSize = MV_MAX( pktInfo->pFrags->dataSize, 
                                       (pMblk->mBlkHdr.mLen + (pMblk->mBlkHdr.mData -
                                                    pMblk->pClBlk->clNode.pClBuf)));

                    pktInfo->pktSize = usedSize + _CACHE_ALIGN_SIZE;
                    /*pktInfo.pktSize = (MGI_RX_BUF_DEF_SIZE + _CACHE_ALIGN_SIZE);*/
/*
                    printf("mgi_rx_free_%u: pMblk=%p, pData=%p, size=%d\n", 
                        pDrvCtrl->debugStats.rxDonePktsCount[queue], 
                        pMblk, pMblk->mBlkHdr.mData, pMblk->mBlkHdr.mLen);
*/
                    /* Clear Mblk structure */
                    pMblk->mBlkHdr.mNext    = NULL;
                    pMblk->mBlkHdr.mNextPkt = NULL;
                    MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, 
                            (pDrvCtrl->debugStats.rxDonePktsCount[queue]++));
                    mvEthPortRxDone(pDrvCtrl->pEthPortHndl, queue, pktInfo);
                    pMblk->pClBlk->clFreeArg3 &= 0xFFFF;
                    MGI_UNLOCK(pDrvCtrl);
                }
                else
                {
                    mgiMblkClDoubleFreeCount++;
                    mvOsPrintf("mgi%d: Double free - pMblk=%p, pPktInfo=%p\n", 
                            pDrvCtrl->ethPortNo, pMblk, pktInfo);
                }
            }
            else
            {
                pMblkNext = _pNetPoolFuncTbl->pMblkClFreeRtn(pNetPool, pMblk);
                mgiMblkClFreeRtnCount++;
            }
            pMblk = NULL;
        }
        else
        {
            mgiMblkClRefFreeCount++;
            pMblk->pClBlk->clRefCnt--;
        }
    }
    if(pMblk != NULL)
    {
        /* free the mBlk */
        mgiMblkFreeCount++;
        _pNetPoolFuncTbl->pMblkFreeRtn(pNetPool, pMblk);        
    }
    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE,
           ("EXIT mgiMblkClFreeRtn: pNetPool=0x%x\n", (unsigned int)pNetPool) );

    return (pMblkNext);
}
#endif /* USE_PRIV_NET_POOL_FUNC */



/*******************************************************************************
* mgiEndLoad - Load END device driver to MUX.
*
* DESCRIPTION:
*       This routine initializes both, driver and device to an operational
*       state using device specific parameters specified by <initString>.
*
*       The <initString> includes single parameter <portNum> - interface and
*       port number.
*
*       The <portMacAddr> is a MAC address to assign the Ethernet port.
*       At first driver look for its MAC address in the Boot line.
*       If address is not found, driver use default one "mgiUserDefMacAddr",
*       when "portNum" added to the last byte of MAC address.
*
*       - Allocate END_OBJ structure.
*       - Get "unit" from "InitString".
*       - Get "macAddr" from BootLine or build default.
*       - Create NetPool and allocate Mblk, Clblk and Clusters for RX,
*       accordingly with RX queue size (MGI_NUM_OF_RX_DESCR)
*       - Init END_OBJ and MIB structures
*       - Mark the device ready 
*
* INPUT:
*       char *initString            Parameter string
*
* RETURN:
*       An END object pointer, or NULL on error.
*
*******************************************************************************/
END_OBJ* mgiEndLoad(char *initString, void* dummy)
{
    DRV_CTRL *  pDrvCtrl;                   /* Pointer to DRV_CTRL structure */
    char        *tok, *holder = NULL;   
    int         ethPort, i;

    if (initString == NULL)
        return (NULL);

    if(mgiIsFirstTime)
    {
        int portCount, numOfPorts, queue;

		if (sysBootParams.flags & SYSFLG_VENDOR_0)
		{
			mgiMtuSize = 9180;
		}
		else
		{
			mgiMtuSize = ETHERMTU;
		}

        mvDebugModuleSetFlags(MV_MODULE_MGI, MGI_DEBUG_FLAG_INIT 
                                            | MGI_DEBUG_FLAG_ERR 
                                            | MGI_DEBUG_FLAG_STATS
                                           /* | MGI_DEBUG_FLAG_RX        */
                                           /* | MGI_DEBUG_FLAG_TX          */
                                           /* | MGI_DEBUG_FLAG_RX_DONE    */
                                           /* | MGI_DEBUG_FLAG_TX_DONE    */
                             );
                     
        for (portCount=0; portCount<BOARD_ETH_PORT_NUM; portCount++)
        {
            mgiDrvCtrl[portCount] = NULL;
        }

        /* Update "endDevTbl" table in the file "configNet.h" */
        i = 0; 
        portCount = 0;
        numOfPorts = mvCtrlEthMaxPortGet();
        while( (portCount < numOfPorts) && 
               (endDevTbl[i].endLoadFunc != END_TBL_END))
        {
            if((FUNCPTR)endDevTbl[i].endLoadFunc == (FUNCPTR)mgiEndLoad)
            {
                MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_INIT, 
                                ("Enable endDevTbl #%d entry\n", i) );
#if defined (MV78200)
				if (mvSocUnitIsMappedToThisCpu(GIGA0+portCount) == FALSE)
				{
					endDevTbl[i].processed = TRUE;
				}
				else
#endif
				{
					endDevTbl[i].processed = FALSE;
				}
                portCount++;
            }
            i++;
        }
        for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
        {
            if (queue == ETH_DEF_RXQ)
                mgiRxQueueDescrNum[queue] = ETH_NUM_OF_RX_DESCR;
            else
                mgiRxQueueDescrNum[queue] = ETH_NUM_OF_RX_DESCR/2;
        }
        for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
            mgiTxQueueDescrNum[queue] = ETH_NUM_OF_TX_DESCR;

        mgiIsFirstTime = MV_FALSE;
    }

    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, 
                     ("ENTER EndLoad: InitString (0x%x) = %s\n", 
                        (unsigned int)initString, initString));

    if (initString[0] == 0)
    {
        bcopy ((char *)DEV_NAME, (void *)initString, DEV_NAME_LEN);
        return (NULL);
    }

    /* Get Interface index from init string (in configNet.h )*/
    tok = strtok_r(initString, ":", &holder);
    if (tok == NULL)
    {
        printf("Invalid Init String to mgiEndLoad(). \n");
        return NULL;
    }
    ethPort = atoi(tok);
#if defined (MV78200)
		if (mvSocUnitIsMappedToThisCpu(GIGA0+ethPort) == FALSE)
		{
			printf("mgi port%d is assined to cpu%d \n",ethPort,whoAmI()?0:1);
			return NULL;
		}
#endif
    /* Sanity check the unit number */
    if( (ethPort < 0) || (ethPort >= mvCtrlEthMaxPortGet() ) ||
		(MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, ethPort))) {
		return NULL;

	}

    /* Allocate the device structure */
    pDrvCtrl = (DRV_CTRL*)mvOsMalloc (sizeof (DRV_CTRL));
    if (pDrvCtrl == NULL)
        return (NULL);

    memset(pDrvCtrl, 0, sizeof (DRV_CTRL));

#ifdef MGI_DEBUG
    {
        int queue;

        /* Allocate RX and TX distribution arrays for Debug only */
        pDrvCtrl->rxPktsDist = mvOsMalloc(sizeof(MV_U32) * (MGI_PORT_RX_MAX_QUOTA+1));
        if(pDrvCtrl->rxPktsDist == NULL)
        {
            mvOsPrintf("mgi%d: Can't allocate %d bytes for rxPktsDist\n", 
                        sizeof(MV_U32) * (MGI_PORT_RX_MAX_QUOTA+1));
            return (NULL);
        }
        memset(pDrvCtrl->rxPktsDist, 0, sizeof(MV_U32)*(MGI_PORT_RX_MAX_QUOTA+1));

        pDrvCtrl->txDonePktsDistSize = 1;
        for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
        {
            pDrvCtrl->txDonePktsDistSize += mgiTxQueueDescrNum[queue];        
        }
        pDrvCtrl->txDonePktsDist = mvOsMalloc( sizeof(MV_U32)*pDrvCtrl->txDonePktsDistSize);
        if(pDrvCtrl->txDonePktsDist == NULL)
        {
            mvOsPrintf("mgi%d: Can't allocate %d bytes for txDonePktsDist\n", 
                        sizeof(MV_U32)*pDrvCtrl->txDonePktsDistSize);
            return (NULL);
        }
        memset(pDrvCtrl->txDonePktsDist, 0, sizeof(MV_U32)*pDrvCtrl->txDonePktsDistSize); 
    }
#endif /* MGI_DEBUG */

    pDrvCtrl->ethPortNo = pDrvCtrl->unit = ethPort;
    pDrvCtrl->rxUcastQ = ETH_DEF_RXQ;
    pDrvCtrl->rxMcastQ = ETH_DEF_RXQ;
    pDrvCtrl->portRxQuota = MGI_PORT_RX_QUOTA;

    /* Parse MAC address if there is a user defined MAC */
    memcpy(pDrvCtrl->macAddr, mgiUserDefMacAddr, sizeof(mgiUserDefMacAddr));
    if( getBootlineMAC(pDrvCtrl->ethPortNo, pDrvCtrl->macAddr) != MV_OK)
    {
        memcpy(pDrvCtrl->macAddr, mgiUserDefMacAddr, MV_MAC_ADDR_SIZE);
        pDrvCtrl->macAddr[5] += pDrvCtrl->ethPortNo;
    }

#ifdef MGI_SEMAPHORE
    /* Create Mutual exclusion semaphore */
    pDrvCtrl->semId = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    if(pDrvCtrl->semId == NULL)
        goto errorExit;
#endif /* MGI_SEMAPHORE */

    /* Memory initialization */
    if (mgiNetPoolInit(pDrvCtrl) == ERROR)
        goto errorExit;

    /* Callout to perform init */
    if(mgiEthPortInit (pDrvCtrl) != MV_OK)
        goto errorExit;     

    pDrvCtrl->attached = MV_TRUE;
    pDrvCtrl->started = MV_FALSE;
    pDrvCtrl->isUp = MV_FALSE;
    mgiDrvCtrl[pDrvCtrl->ethPortNo] = pDrvCtrl;

	/* allocate and initialization PKT_INFO structure */
    pDrvCtrl->txPktInfo = mvOsMalloc(ETH_NUM_OF_TX_DESCR*sizeof(MV_PKT_INFO*)); 
	if(pDrvCtrl->txPktInfo == NULL)
	{
		mvOsPrintf("mgi%d: Can't alloc %d bytes for txPktInfo array\n", 
				   pDrvCtrl->ethPortNo,
				   ETH_NUM_OF_TX_DESCR*sizeof(MV_PKT_INFO*));
		goto errorExit;
	}
	for(i=0; i<ETH_NUM_OF_TX_DESCR; i++)
	{
		pDrvCtrl->txPktInfo[i] = mvOsMalloc(sizeof(MV_PKT_INFO));
		if(pDrvCtrl->txPktInfo[i] == NULL)
		{
			mvOsPrintf("mgi%d: Can't alloc MV_PKT_INFO for %d descr\n", 
					   pDrvCtrl->ethPortNo, i);
			goto errorExit;
		}
		memset(pDrvCtrl->txPktInfo[i], 0, sizeof(MV_PKT_INFO));
		pDrvCtrl->txPktInfo[i]->ownerId = (char)-1; 

		pDrvCtrl->txPktInfo[i]->pFrags = mvOsMalloc(sizeof(MV_BUF_INFO)*(MGI_MAX_BUF_PER_PKT + MV_ETH_EXTRA_FRAGS_NUM));
		if(pDrvCtrl->txPktInfo[i]->pFrags == NULL)
		{
			mvOsPrintf("mgi%d: Can't alloc %d bytes for MV_BUF_INFO array for %d descr\n", 
					   pDrvCtrl->ethPortNo,
					   (int)(sizeof(MV_BUF_INFO)*(MGI_MAX_BUF_PER_PKT + MV_ETH_EXTRA_FRAGS_NUM)), i);
			goto errorExit;
		}
		memset(pDrvCtrl->txPktInfo[i]->pFrags, 0, sizeof(MV_BUF_INFO)*(MGI_MAX_BUF_PER_PKT + MV_ETH_EXTRA_FRAGS_NUM));
	}
	pDrvCtrl->txPktInfoIdx = 0;
	pDrvCtrl->tx_count = 0;

/*	*******************	*/

    /* endObj initializations */
    if (END_OBJ_INIT (&pDrvCtrl->endObj, (DEV_OBJ*)NULL,
              DEV_NAME, pDrvCtrl->unit, &netFuncs,
              "MV-643xx Ethernet Enhanced Network Driver") == ERROR)
        goto errorExit;

#ifdef INCLUDE_MGI_END_2233_SUPPORT
    pDrvCtrl->endObj.pMib2Tbl = m2IfAlloc(M2_ifType_ethernet_csmacd, 
                                   (UINT8*)pDrvCtrl->macAddr, MV_MAC_ADDR_SIZE,
									mgiMtuSize, MGI_1GBS, DEV_NAME, pDrvCtrl->unit);
    if(pDrvCtrl->endObj.pMib2Tbl == NULL)
    {
        printf("mgiEndLoad: MIB-II initialization FAILED\n"); 
        goto errorExit;
    }
    m2IfPktCountRtnInstall(pDrvCtrl->endObj.pMib2Tbl, m2If8023PacketCount);
    bcopy( (char*)&pDrvCtrl->endObj.pMib2Tbl->m2Data.mibIfTbl,
           (char*)&pDrvCtrl->endObj.mib2Tbl, sizeof(M2_INTERFACETBL) );
#else
    if (END_MIB_INIT (&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd,
              (u_char *)pDrvCtrl->macAddr, MV_MAC_ADDR_SIZE,
			  mgiMtuSize, MGI_1GBS) == ERROR)
    {
        goto errorExit;
    }
#endif /* INCLUDE_MGI_END_2233_SUPPORT */

#if defined(MGI_STATS_POLLING) 
    mvEthMibCountersClear(pDrvCtrl->pEthPortHndl);
    pDrvCtrl->inPackets = 0;
    pDrvCtrl->inOctets = 0; 
    bzero ((char *)&pDrvCtrl->endStatsCounters, sizeof(END_IFCOUNTERS));

    pDrvCtrl->endStatsConf.ifPollInterval = 5 * sysClkRateGet(); /* 5 sec */
    pDrvCtrl->endStatsConf.ifEndObj = &pDrvCtrl->endObj;
    pDrvCtrl->endStatsConf.ifWatchdog = NULL;
    pDrvCtrl->endStatsConf.ifValidCounters = (END_IFINUCASTPKTS_VALID      | 
                                              END_IFINMULTICASTPKTS_VALID  | 
                                              END_IFINBROADCASTPKTS_VALID  |
                                              END_IFINOCTETS_VALID         | 
                                              END_IFOUTOCTETS_VALID        | 
                                              END_IFOUTUCASTPKTS_VALID     |
                                              END_IFOUTMULTICASTPKTS_VALID | 
                                              END_IFOUTBROADCASTPKTS_VALID);
#endif /* MGI_STATS_POLLING */

    /* Mark the device ready */
    END_OBJ_READY (&pDrvCtrl->endObj,
             IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST | MGI_END_MIB_2233);

    mvOsPrintf("%s%d Interface Loaded\n", DEV_NAME, pDrvCtrl->unit);

    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, ("EXIT EndLoad: pEndObj=0x%x\n", 
                    (unsigned int)&pDrvCtrl->endObj) ); 

    return (&pDrvCtrl->endObj);

errorExit:

    printf("ERROR!!! EndLoad: failed\n"); 
    mgiCleanup(pDrvCtrl);
    return NULL;
}

/*******************************************************************************
* mgiUnload - unload a driver from the system
*
* DESCRIPTION:
*       This function unloads the ethernet port END driver from the MUX.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       Driver memory allocated is freed.
*       Driver is disconnected from the MUX.
*
* RETURN:
*       OK.
*
*******************************************************************************/
LOCAL STATUS mgiUnload(DRV_CTRL *pDrvCtrl)
{
    STATUS  status;
    int     unit = pDrvCtrl->unit;

    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, ("ENTER EndUnoad: pDrvCtrl=0x%x\n", 
                                           (unsigned int)pDrvCtrl)); 

    if(pDrvCtrl->started)
    {
        status = mgiStop(pDrvCtrl);
        if(status != OK)
        {
            printf("MGI: can't stop %s%d interface\n", DEV_NAME, pDrvCtrl->unit);
            return status;
        }
    }

    pDrvCtrl->attached = FALSE;

    /* Free lists */
    END_OBJECT_UNLOAD (&pDrvCtrl->endObj);
    mgiCleanup(pDrvCtrl);

    mvOsPrintf("%s%d Interface Unloaded\n", DEV_NAME, unit);

    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, ("EXIT EndUnoad\n")); 
    return (OK);
}


/*******************************************************************************
* mgiNetPoolInit - Initialize memory.
*
* DESCRIPTION:
*       This function allocates the necessary memory space for the Tx/Rx
*       descriptors as well as Rx buffers. Both Tx/Rx descriptor memory space
*       and Rx buffers allocated using the malloc() routine which defines this
*       memory space cacheable as the low level driver supports cacheable
*       descriptors. This is done to allow better packet process performance.
*       The function assigns the Rx buffer memory space to be the network pool
*       cluster memory space which means that the network pool clusters and the
*       Gigabit Ethernet Controller buffers are the same. This allow the zero
*       copy in Rx where the Rx data is placed into the network pool clusters
*       with no copying.
*       This function also makes sure the descriptor and cluster addresses are
*       cache line size aligned in order to avoid data loss when performing
*       data cache flush or invalidate.
*
*       After memory allocation the driver creates and initialize
*           - netBufLib.
*               The pool of mBlk and clBlk is created.
*               The pool of clusters is defined to be the Rx buffers.
*               This way the driver can practice the Zero Copy Buff methodology.
*           - Tx/Rx descriptors.
*               Call low level routine to create Tx descriptor data structure.
*               Call low level routine to create Rx descriptor data structure.
*               Assign each Rx descriptor with netBufLib cluster.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       Driver memory is ready for Rx and Tx operation.
*
* RETURN:
*       OK if output succeeded.
*       ERROR if - driver failed to allocate one of the address spaces.
*                - <memBase> parameter was not NONE.
*                - failure to initialize netBufLib pools.
*                - fail to retrieve cluster from pool for all Rx descriptors.
*
*******************************************************************************/
static STATUS mgiNetPoolInit(DRV_CTRL *pDrvCtrl)
{
    M_CL_CONFIG mgiMclBlkConfig;
    POOL_FUNC*  pNetPoolFunc = NULL;
    CL_DESC     mgiClDescTbl[1];
    int         queue;
    int         mgiClDescTblNumEnt = (NELEMENTS (mgiClDescTbl));

    /* Initialize the netPool */
    if ((pDrvCtrl->endObj.pNetPool = mvOsMalloc (sizeof (NET_POOL))) == NULL)
        return (ERROR);

    memset(pDrvCtrl->endObj.pNetPool, 0, sizeof (NET_POOL));
    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_INIT, 
                    ("MGI: %d bytes allocated for NET_POOL, addr=0x%x\n", 
                    sizeof(NET_POOL), (unsigned int)pDrvCtrl->endObj.pNetPool));

    /* Calculate number of mBlks, clBlks and Clusters for all queues */
    mgiMclBlkConfig.mBlkNum = 0;
    mgiMclBlkConfig.clBlkNum = 0;
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        mgiMclBlkConfig.mBlkNum += mgiRxQueueDescrNum[queue];        
    }
    mgiMclBlkConfig.mBlkNum += CL_LOAN_NUM;
    mgiMclBlkConfig.clBlkNum = mgiMclBlkConfig.mBlkNum;
    mgiMclBlkConfig.mBlkNum = mgiMclBlkConfig.mBlkNum*4;

    /* Get memory for mBlks */
    mgiMclBlkConfig.memSize = (
                    (mgiMclBlkConfig.mBlkNum  * (M_BLK_SZ + 4))  +
                    (mgiMclBlkConfig.clBlkNum * CL_BLK_SZ + 4));

    pDrvCtrl->pMblkBase = mvOsMalloc(mgiMclBlkConfig.memSize + 4);
    if(pDrvCtrl->pMblkBase == NULL)
    {
        mvOsPrintf("MGI: Failed to allocate %d bytes for mBlk and clBlk mem\n",
                            mgiMclBlkConfig.memSize+4);
        return (ERROR);
    }
    memset (pDrvCtrl->pMblkBase, 0, (mgiMclBlkConfig.memSize+4) );
    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_INIT, 
                 ("MGI: %d bytes allocated for %d mBlk + %d clBlk: Addr=0x%x\n",
                  mgiMclBlkConfig.memSize+4, mgiMclBlkConfig.mBlkNum,
                  mgiMclBlkConfig.clBlkNum, (unsigned int)pDrvCtrl->pMblkBase));

    /* Align address */
    mgiMclBlkConfig.memArea =(void*)MV_ALIGN_UP((MV_U32)pDrvCtrl->pMblkBase, 4);

    /* Allocate memory for Rx clusters (including loan) */
    mgiClDescTbl[0].clNum = mgiMclBlkConfig.clBlkNum;
    mgiClDescTbl[0].clSize = CL_BUFF_SIZE;
    mgiClDescTbl[0].memSize = (mgiClDescTbl[0].clNum * 
                  (mgiClDescTbl[0].clSize + CL_OVERHEAD));

#if defined(MV_UNCACHED_RX_BUFFERS)
      pDrvCtrl->pClsBase = mvOsIoUncachedMalloc(NULL, 
                                 (mgiClDescTbl[0].memSize + 0x20), NULL,NULL);
#else
      pDrvCtrl->pClsBase = mvOsIoCachedMalloc(NULL, 
                                (mgiClDescTbl[0].memSize + 0x20), NULL,NULL);
#endif /* MV_UNCACHED_RX_BUFFERS */

    if(pDrvCtrl->pClsBase == NULL)
    {
        mvOsPrintf("MGI: Failed to allocate %d bytes for RX Clusters\n",
                            mgiClDescTbl[0].memSize + 0x20);
        return (ERROR);
    }
    memset (pDrvCtrl->pClsBase, 0, (mgiClDescTbl[0].memSize+0x20) );
    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_INIT, 
                     ("MGI: %d bytes allocated for RX buffers: Addr=0x%x\n",
                     mgiClDescTbl[0].memSize + 0x20, 
                      (unsigned int)pDrvCtrl->pClsBase));

    /* Align cluster so the cluser data poiner will be cache line aligned */
    mgiClDescTbl[0].memArea = (char*) ((MV_U32)pDrvCtrl->pClsBase +
                   (0xc - ((MV_U32)pDrvCtrl->pClsBase & 0xF)));
    
    mgiClDescTbl[0].memArea=(char*)((MV_U32)mgiClDescTbl[0].memArea|0x10);

#ifdef USE_PRIV_NET_POOL_FUNC
    /* Replace only one function in POOL_FUNC table */
    memcpy(&mgiPoolFuncTbl, _pNetPoolFuncTbl, sizeof(POOL_FUNC));
    mgiPoolFuncTbl.pMblkClFreeRtn = mgiMblkClFreeRtn;
    pNetPoolFunc = &mgiPoolFuncTbl;
#endif /* USE_PRIV_NET_POOL_FUNC */

    /* Init the mem pool */
    if (netPoolInit (pDrvCtrl->endObj.pNetPool, &mgiMclBlkConfig,
             &mgiClDescTbl[0], mgiClDescTblNumEnt, pNetPoolFunc) == ERROR)
    {
        printf("netPoolInit failed\n");
        return (ERROR);
    }

    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool,
                                                CL_BUFF_SIZE, FALSE)) == NULL)
    {
        printf("netClPoolIdGet failed\n");
        return (ERROR);
    }
    return OK;
}

/* Init ethernet port attached for this interface */
LOCAL MV_STATUS mgiEthPortInit(DRV_CTRL *pDrvCtrl)
{
    MV_ETH_PORT_INIT        portInit;
 
    memset(&portInit, 0, sizeof(MV_ETH_PORT_INIT));

    portInit.rxDefQ = ETH_DEF_RXQ;
    portInit.maxRxPktSize = MGI_RX_BUF_DEF_SIZE;
    
    memcpy(portInit.rxDescrNum,  mgiRxQueueDescrNum, sizeof(mgiRxQueueDescrNum)); 

    memcpy(portInit.txDescrNum,  mgiTxQueueDescrNum, sizeof(mgiTxQueueDescrNum)); 
                                         
    /* Callout to perform init */
    pDrvCtrl->pEthPortHndl = mvEthPortInit(pDrvCtrl->ethPortNo, &portInit);
    if(pDrvCtrl->pEthPortHndl == NULL)
    {
        mvOsPrintf("mgi: Can't init Eth port #%d\n", pDrvCtrl->ethPortNo);
        return (MV_FAIL);
    }

    mvOsPrintf("Ethernet port #%d initialized\n", pDrvCtrl->ethPortNo);

    return MV_OK;
}


/*******************************************************************************
* mgiStart - Start the device
*
* DESCRIPTION:
*       This routine prepare the ethernet port and system for operation:
*       1) Connects the driver ISR.
*       2) Enables interrupts.
*       3) Start the Gigabit ethernet port using the low level driver.
*       4) Marks the interface as active.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       see description.
*
* RETURN:
*       OK always.
*
*******************************************************************************/
LOCAL STATUS mgiStart(DRV_CTRL *pDrvCtrl)
{
    STATUS          status;
    MV_STATUS       mvStatus;
    END_ERR         endErr;
    int             queue, i;

    endErr.pMesg = NULL;
    endErr.pSpare = NULL;

    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, ("ENTER mgiStart ifId=%d\n", 
                        pDrvCtrl->unit));

    /* Must have been attached */
    if (!pDrvCtrl->attached)
    {
        mvOsPrintf("%s%d Interface is not loaded\n", DEV_NAME, pDrvCtrl->unit);
        return (ERROR);
    }

    if (pDrvCtrl->started)
    {
        mvOsPrintf("%s%d Interface is already started\n", 
                   DEV_NAME, pDrvCtrl->unit);
        return (OK);
    }

    /* Set some flags to default values */
    pDrvCtrl->txStall = FALSE;

    /* Fill gigaEth RX queue with RX buffers */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        mgiRxRefill(pDrvCtrl, queue, mgiRxQueueDescrNum[queue]);
    }

    /* Set MAC address */
    if( mvEthMacAddrSet(pDrvCtrl->pEthPortHndl, pDrvCtrl->macAddr, 
                                            ETH_DEF_RXQ) != MV_OK ) 
    {
        printf("mgi_%d: ethSetMacAddr failed\n", pDrvCtrl->ethPortNo);
        return (MV_FAIL);
    }

    status = intConnect(INUM_TO_IVEC(INT_LVL_GBE_PORT_RX(pDrvCtrl->ethPortNo)), 
                        mgiRxReadyIsr, (int)pDrvCtrl);
    if (status != OK)
    {
        mvOsPrintf("%s%d: Can't connect to RxReady (%d) interrupt, status=0x%x \n", 
                    DEV_NAME, pDrvCtrl->ethPortNo, 
                    INT_LVL_GBE_PORT_RX(pDrvCtrl->ethPortNo), status);
        return (status);
    }

    status = intConnect(INUM_TO_IVEC(INT_LVL_GBE_PORT_TX(pDrvCtrl->ethPortNo)), 
                        mgiTxDoneIsr, (int)pDrvCtrl);
    if (status != OK)
    {
        mvOsPrintf("%s%d: Can't connect to TxDone (%d) interrupt, status=0x%x\n", 
                    DEV_NAME, pDrvCtrl->ethPortNo, 
                    INT_LVL_GBE_PORT_TX(pDrvCtrl->ethPortNo), status);
        return (status);
    }

    status = intConnect(INUM_TO_IVEC(INT_LVL_GBE_PORT_MISC(pDrvCtrl->ethPortNo)), 
                        mgiMiscIsr, (int)pDrvCtrl);
    if (status != OK)
    {
        mvOsPrintf("%s%d: Can't connect to Misc (%d) interrupt, status=0x%x\n", 
                    DEV_NAME, pDrvCtrl->ethPortNo, 
                    INT_LVL_GBE_PORT_MISC(pDrvCtrl->ethPortNo), status);
        return (status);
    }

    intEnable(INT_LVL_GBE_PORT_RX(pDrvCtrl->ethPortNo));
    intEnable(INT_LVL_GBE_PORT_TX(pDrvCtrl->ethPortNo));
    intEnable(INT_LVL_GBE_PORT_MISC(pDrvCtrl->ethPortNo));

    pDrvCtrl->started = MV_TRUE;
    mvOsPrintf("%s%d Interface Started\n", DEV_NAME, pDrvCtrl->unit);

    mvEthRxCoalSet(pDrvCtrl->pEthPortHndl, MGI_RX_COAL_VALUE);

    mvEthTxCoalSet(pDrvCtrl->pEthPortHndl, MGI_TX_COAL_VALUE);

    MV_REG_WRITE( ETH_INTR_MASK_REG(pDrvCtrl->ethPortNo), MGI_RX_READY_MASK);
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(pDrvCtrl->ethPortNo), 
                    MGI_TX_DONE_MASK | MGI_MISC_MASK);

    /* Mark the interface as up */
    END_FLAGS_SET (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));
	i = 0;
	mvStatus = mvEthPortEnable(pDrvCtrl->pEthPortHndl);
	while ((mvStatus != MV_OK) && (i < 10))
	{
		mvOsDelay(50);
		i++;
		mvStatus = mvEthPortEnable(pDrvCtrl->pEthPortHndl);
	}
    if(mvStatus == MV_OK)
    {
        int                 speed = 0;
        MV_ETH_PORT_STATUS  portStatus;

        mvEthStatusGet(pDrvCtrl->pEthPortHndl, &portStatus);

        if(portStatus.speed == MV_ETH_SPEED_10)
            speed = 10;
        else if(portStatus.speed == MV_ETH_SPEED_100)
            speed = 100;
        else if(portStatus.speed == MV_ETH_SPEED_1000)
            speed = 1000;

        endErr.errCode = END_ERR_UP;
        muxError(&pDrvCtrl->endObj, &endErr);

        pDrvCtrl->isUp = MV_TRUE;

        mvOsPrintf("%s%d Interface - Link Up, %d %s\n", 
                DEV_NAME, pDrvCtrl->unit, speed,
                (portStatus.duplex == MV_ETH_DUPLEX_HALF) ? "Half" : "Full");
    }
    else
    {

        endErr.errCode = END_ERR_DOWN;
        muxError(&pDrvCtrl->endObj, &endErr);

       pDrvCtrl->isUp = MV_FALSE;
    }
    
    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, ("EXIT mgiStart: ifId=%d\n", 
                        pDrvCtrl->unit));

    return (OK);
}

/*******************************************************************************
* mgiStop - Stop the END device activity
*
* DESCRIPTION:
*       This routine marks the interface as inactive, disables interrupts and
*       resets the Gigabit Ethernet Controller port. It brings down the
*       interface to a non-operational state.
*       To bring the interface back up, mgiStart() must be called.
*
* INPUT:
*       None.
*
* OUTPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* RETURN:
*       OK, always.
*
*******************************************************************************/
LOCAL STATUS mgiStop(DRV_CTRL* pDrvCtrl)
{
    ETHER_MULTI*    pEtherMulti;
    STATUS          retVal;
    END_ERR         endErr;
    int             queue;

    if(pDrvCtrl == NULL)
        return ERROR;

    MGI_LOCK(pDrvCtrl);

    endErr.pMesg = NULL;
    endErr.pSpare = NULL;

    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TRACE, 
                    ("ENTER mgiStop: pDrvCtrl=0x%x, ifId=%d\n", 
                    (unsigned int)pDrvCtrl, pDrvCtrl->unit));

    if(!pDrvCtrl->started)
    {
        mvOsPrintf("%s%d Interface is not started\n", DEV_NAME, pDrvCtrl->unit);
        MGI_UNLOCK(pDrvCtrl);

        return (OK);
    }

    pDrvCtrl->started = MV_FALSE;
    pDrvCtrl->isUp = MV_FALSE;

    /* Mark the interface as down */
    END_FLAGS_CLR (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

    endErr.errCode = END_ERR_DOWN;
    muxError(&pDrvCtrl->endObj, &endErr);


    /* Mask gigaEth interrupts and clear cause registers */
    MV_REG_WRITE( ETH_INTR_MASK_REG(pDrvCtrl->ethPortNo), 0);
    MV_REG_WRITE( ETH_INTR_CAUSE_REG(pDrvCtrl->ethPortNo), 0);
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(pDrvCtrl->ethPortNo), 0);
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo), 0);

    /* Stop RX, TX and disable the Ethernet port */
    mvEthPortDisable(pDrvCtrl->pEthPortHndl);

    mvOsPrintf("%s%d Interface Down\n", DEV_NAME, pDrvCtrl->unit);

    intDisable(INT_LVL_GBE_PORT_RX(pDrvCtrl->ethPortNo));
    intDisable(INT_LVL_GBE_PORT_TX(pDrvCtrl->ethPortNo));
    intDisable(INT_LVL_GBE_PORT_MISC(pDrvCtrl->ethPortNo)); 
#ifdef MV_PPC
    mvIntDisconnect(INT_LVL_GBE_PORT_RX(pDrvCtrl->ethPortNo));
    mvIntDisconnect(INT_LVL_GBE_PORT_TX(pDrvCtrl->ethPortNo));
    mvIntDisconnect(INT_LVL_GBE_PORT_MISC(pDrvCtrl->ethPortNo)); 
#endif

    /* Free all TX resources */
    for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
    {
        mgiFreeTxResources(pDrvCtrl, queue);
    }

    /* Free all RX resources */
    for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
    {
        mgiFreeRxResources(pDrvCtrl, queue);
    }

    /* Delete all existing Multicast addresses from END_OBJECT database */
    while(pDrvCtrl->endObj.nMulti > 0)
    {
        pEtherMulti = END_MULTI_LST_FIRST(&pDrvCtrl->endObj);
        do
        {
            retVal = etherMultiDel (&pDrvCtrl->endObj.multiList, 
                                    &pEtherMulti->addr[0]);
        }while( retVal != ENETRESET);
        pDrvCtrl->endObj.nMulti--;
    }
    mvEthDefaultsSet(pDrvCtrl->pEthPortHndl);

    mvOsPrintf("%s%d Interface Stopped\n", DEV_NAME, pDrvCtrl->unit);

    MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_TRACE, ("EXIT mgiStop: ifId=%d\n", 
                        pDrvCtrl->unit));

    MGI_UNLOCK(pDrvCtrl);

    return OK;
}

/*******************************************************************************
* mgiSend - Send an Ethernet packet
*
* DESCRIPTION:
*       This routine takes a M_BLK and sends off the data using the low level
*       API. The buffer must already have the addressing information properly
*       installed in it. This is done by a higher layer.
*       The routine calls low level Tx routine for each mblk possibly reside
*       in the M_BLK struct passed as a parameter.
*       Note: The routine has to be familiar with Tx command status field of
*       the low-level Tx descriptor in order to signal the low level driver on
*       the location of the transmitted buffer in the packet (for packet
*       spanned over multiple buffers). This way the routine supports
*       Scatter-Gather. When the driver gets a chain of mBlks it is able to
*       perform Gather-write where it does not need to do any data copying.
*
*       muxSend() calls this routine each time it wants to send a packet.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*       M_BLK *     pMblk        pointer to the mBlk/cluster pair
*
* OUTPUT:
*       The packet described by pMblk is sent to the Ethernet low level driver
*       for transmission.
*
* RETURN:
*       OK, END_ERR_BLOCK in case there are no Tx descriptors available.
*
*******************************************************************************/
LOCAL STATUS mgiSend(DRV_CTRL *pDrvCtrl, M_BLK *pMblkPkt)
{
    MV_U32          pktCounter=0;
    MV_STATUS       status;
    STATUS          vxStatus = OK;
    MV_PKT_INFO     *pktInfo;
    int             queue;

    /* interlock with txDone */
    MGI_LOCK(pDrvCtrl);

    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TRACE, 
                     ("ENTER mgiSend: ifId=%d\n", pDrvCtrl->unit) );

    MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, (pDrvCtrl->debugStats.txCount++));

    MGI_TRACE_ADD(MGI_TRACE_TX_ENTER);

    if(pDrvCtrl->isUp == MV_FALSE)
    {
        netMblkClChainFree( pMblkPkt );
        vxStatus = ERROR;
        pDrvCtrl->debugStats.txFailedCount++;
        pMblkPkt = NULL;
    }
    /* for all concatinated Packets */
    while( (pMblkPkt != NULL) && 
           (pMblkPkt->mBlkHdr.mLen > 0) && 
           (pMblkPkt->mBlkHdr.mData != NULL) )
    {       
      	pktInfo = pDrvCtrl->txPktInfo[pDrvCtrl->txPktInfoIdx++];
	    if(pDrvCtrl->txPktInfoIdx == ETH_NUM_OF_TX_DESCR)
		    pDrvCtrl->txPktInfoIdx = 0;

        pktInfo->status = 0;

        status = mgiMblkToPktInfo(pDrvCtrl, pMblkPkt, pktInfo);
        if(status == MV_OK)
        {   
            MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TX, 
                   ("\n mgiTx%d #%d: pMblkPkt=0x%x, pData=0x%x, pktSize=%d, bufNum=%d\n", 
                    pDrvCtrl->unit, pDrvCtrl->debugStats.txCount, (unsigned int)pMblkPkt, 
                    (unsigned int)pMblkPkt->mBlkHdr.mData,
                    pktInfo->pktSize, pktInfo->numFrags) );

            MGI_DEBUG_CODE( (MGI_DEBUG_FLAG_MBLK | MGI_DEBUG_FLAG_TX), 
                        (mgiPrintMblk(pMblkPkt)));
#ifdef TX_CSUM_OFFLOAD
            {
                MV_802_3_HEADER*    pMacHeader = (MV_802_3_HEADER*)(pMblkPkt->mBlkHdr.mData);

                /* if HW is suppose to offload layer4 checksum, set some bits in the first buf_info command */
                if (pMacHeader->typeOrLen == MV_16BIT_BE(MV_IP_TYPE))
                {
                    struct ip *pIpHeader;
                    /* Check only IP packets */
                    pIpHeader = (struct ip*)(((MV_U8*)pMacHeader) + sizeof(MV_802_3_HEADER));

                    /* Check only UDP packets */
                    if (IPPROTO_UDP == pIpHeader->ip_p)
                    {
                        pktInfo->status = 
                        ETH_TX_IP_NO_FRAG  |               /* we do not handle fragmented IP packets. add check inside iph!! */
                        ((pIpHeader->ip_hl) << ETH_TX_IP_HEADER_LEN_OFFSET) |                            /* 32bit units */
                        ETH_TX_L4_UDP_TYPE |                /* UDP packet */
                        ETH_TX_GENERATE_L4_CHKSUM_MASK |   /* generate layer4 csum command */
                        ETH_TX_GENERATE_IP_CHKSUM_BIT;     /* generate IP csum (already done?) */
                    } else
                    {
                        pktInfo->status = 0x5 << ETH_TX_IP_HEADER_LEN_OFFSET; /* Errata BTS #50 */
                    }
                }
            }
#endif /* TX_CSUM_OFFLOAD  */

            if(mgiStatsControl == 2)
            {
            MGI_OUT_COUNTERS_UPDATE(&pDrvCtrl->endObj, pMblkPkt);
            }

            /* send packet */
            MGI_DEBUG_CODE((MGI_DEBUG_FLAG_DUMP | MGI_DEBUG_FLAG_TX), 
                (mvDebugPrintPktInfo(pktInfo, 64, 1)) );

#if (MV_ETH_TX_Q_NUM > 1)
            queue = mgiTxQueueGet(pMblkPkt);
#else       
            queue = ETH_DEF_TXQ;
#endif /* (MV_ETH_TX_Q_NUM > 1) */

            MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, (pDrvCtrl->debugStats.txPktsCount[queue]++));

            status = mvEthPortSgTx(pDrvCtrl->pEthPortHndl, 
                                    queue, pktInfo);
        }
        if(status == MV_OK) 
        {
            /* move to next packet */
            pMblkPkt = pMblkPkt->mBlkHdr.mNextPkt;
            pktCounter++;
        }
        else 
        {
            /* back pressure the stack */
            MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TX, 
                    ("mgiTx: stack back pressure: ifId=%d\n", pDrvCtrl->unit));

            if(pDrvCtrl->txPktInfoIdx == 0)
            {
                pDrvCtrl->txPktInfoIdx = ETH_NUM_OF_TX_DESCR-1;
            }
            else
	        {
                pDrvCtrl->txPktInfoIdx--;
	        }

            pDrvCtrl->debugStats.stackBP++;
            pDrvCtrl->debugStats.txFailedCount++;
            pDrvCtrl->txStall = MV_TRUE;
            vxStatus = END_ERR_BLOCK;
            break;  /* exit loop */
        }
    }

#ifdef MGI_DEBUG
    if(pktCounter > pDrvCtrl->debugStats.txMaxPktsCount)
        pDrvCtrl->debugStats.txMaxPktsCount = pktCounter;
#endif /* MGI_DEBUG */

    MGI_TRACE_ADD(MGI_TRACE_TX_EXIT);

    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TRACE, ("EXIT mgiSend: ifId=%d, status=%d\n", 
                    pDrvCtrl->unit, vxStatus) );

    /* Interlock with txRsrcReturn */
    MGI_UNLOCK(pDrvCtrl);

    return (vxStatus);
}

/*******************************************************************************
* mgiMiscIsr - Tx interrupt handler.
*
* DESCRIPTION:
*       This routine is the interrupt handler of all events except RX and TX.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*
* RETURN:
*       None.
*
*******************************************************************************/
void mgiMiscIsr(DRV_CTRL *pDrvCtrl)
{    
    volatile MV_U32     cause;
    STATUS              rc;
    MV_ETH_PORT_STATUS  portStatus;

    pDrvCtrl->debugStats.miscIsrCount++;

    /* Read cause  */
    cause = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo));

    if( cause & (1 << ETH_CAUSE_LINK_STATE_CHANGE_BIT) )
    {
    mvEthStatusGet(pDrvCtrl->pEthPortHndl, &portStatus);

    /* schedule the TxDoneJob in netTask context */
    rc = netJobAdd( (FUNCPTR)mgiLinkChangeJob, (int)pDrvCtrl, 
                                               (int)portStatus.isLinkUp, 0, 0, 0 );
    if(rc != OK)
        pDrvCtrl->debugStats.netJobAddError++;
    }
    /* Clear Misc bits */
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo), ~cause);
}

/* Function */
LOCAL void mgiLinkChangeJob(DRV_CTRL *pDrvCtrl, int isLinkUp)
{
    END_ERR             endErr;
    int                 queue;
    MV_ETH_PORT_STATUS  portStatus;

    MGI_LOCK(pDrvCtrl);

    endErr.pMesg = NULL;
    endErr.pSpare = NULL;

    mvEthStatusGet(pDrvCtrl->pEthPortHndl, &portStatus);

    if(portStatus.isLinkUp != (MV_BOOL)isLinkUp)
    {
        pDrvCtrl->debugStats.linkMissCount++;
    }

    if((MV_BOOL)isLinkUp == MV_TRUE)
    {
        int speed = 0;
        
        pDrvCtrl->debugStats.linkUpCount++;

        if(portStatus.speed == MV_ETH_SPEED_10)
            speed = 10;
        else if(portStatus.speed == MV_ETH_SPEED_100)
            speed = 100;
        else if(portStatus.speed == MV_ETH_SPEED_1000)
            speed = 1000;

        mvEthPortUp(pDrvCtrl->pEthPortHndl);

        endErr.errCode = END_ERR_UP;
        muxError(&pDrvCtrl->endObj, &endErr);

        pDrvCtrl->isUp = MV_TRUE;

        mvOsPrintf("%s%d Interface - Link Up, %d %s\n", 
                DEV_NAME, pDrvCtrl->unit, speed,
                (portStatus.duplex == MV_ETH_DUPLEX_HALF) ? "Half" : "Full");
    }
    else
    {
        pDrvCtrl->debugStats.linkDownCount++;

        endErr.errCode = END_ERR_DOWN;
        muxError(&pDrvCtrl->endObj, &endErr);

        pDrvCtrl->isUp = MV_FALSE;

        mvEthPortDown(pDrvCtrl->pEthPortHndl);

        /* Free all TX resources */
        for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
        {
            mgiFreeTxResources(pDrvCtrl, queue);
        }
        /* Fix RX queues: */
        for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
        {
            int num;

            num = mgiFreeRxResources(pDrvCtrl, queue);
            mgiRxRefill(pDrvCtrl, queue, num);            
        }
        mvOsPrintf("%s%d Interface - Link Down\n", DEV_NAME, pDrvCtrl->unit);
    }
    MGI_UNLOCK(pDrvCtrl);
}



/*******************************************************************************
* mgiTxDoneIsr - Tx interrupt handler.
*
* DESCRIPTION:
*       This routine is the Tx interrupt handler. When this routine is called
*       (by the Ethernet interrupt controller handler) the Tx interrupt event
*       are already acknowledged, so that the device will de-assert its
*       interrupt signal.
*       The amount of work done here is kept to a minimum; the bulk of the
*       work is deferred to the netTask.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       Activating netJobAdd to registrate the Tx events.
*
* RETURN:
*       None.
*
*******************************************************************************/
void mgiTxDoneIsr(DRV_CTRL *pDrvCtrl)
{
    STATUS          rc;
    volatile MV_U32 cause;

    pDrvCtrl->debugStats.txDoneIsrCount++;
 
    MGI_TRACE_ADD(MGI_TRACE_TX_DONE_ISR);

    /* Read cause to start Coalescing mechanism */
    cause = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo));

    /* Disable TxDone interrupt (all interrupts in Isr Ext register) */
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(pDrvCtrl->ethPortNo), MGI_MISC_MASK);
    
    /* schedule the TxDoneJob in netTask context */
    rc = netJobAdd( (FUNCPTR)mgiTxDoneJob, (int)pDrvCtrl, cause, 0, 0, 0 );
    if(rc != OK)
        pDrvCtrl->debugStats.netJobAddError++;

    /*logMsg("mgiTxDoneIsr: cause=0x%x\n", cause, 0, 0, 0, 0, 0);*/

    /* Clear bits for TX_DONE */
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo), ~cause);
}

/*******************************************************************************
* mgiRxReadyIsr - Rx interrupt handler.
*
* DESCRIPTION:
*       This routine is the Rx interrupt handler. When this routine is called
*       (by the Ethernet interrupt controller handler) the Rx interrupt event
*       are already acknowledged, so that the device will de-assert its
*       interrupt signal.
*       The amount of work done here is kept to a minimum; the bulk of the
*       work is deferred to the netTask.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       Activating netJobAdd to registrate the Rx events.
*
* RETURN:
*       None.
*
*******************************************************************************/
void mgiRxReadyIsr(DRV_CTRL *pDrvCtrl)
{
    STATUS          rc;
    volatile MV_U32 cause;

    cause = MV_REG_READ( ETH_INTR_CAUSE_REG(pDrvCtrl->ethPortNo));

    /* Disable RX interrupts */
    MV_REG_WRITE( ETH_INTR_MASK_REG(pDrvCtrl->ethPortNo), 0);

    /* Clear bits for RX */
    MV_REG_WRITE( ETH_INTR_CAUSE_REG(pDrvCtrl->ethPortNo), ~cause);

    pDrvCtrl->debugStats.rxReadyIsrCount++;

    MGI_TRACE_ADD(MGI_TRACE_RX_READY_ISR);

    /* schedule the RxReadyJob in netTask context */
    rc = netJobAdd( (FUNCPTR)mgiRxReadyJob, (int)pDrvCtrl, cause, 0, 0, 0 );
    if(rc != OK)
        pDrvCtrl->debugStats.netJobAddError++;
}
/***********************************************************/

#ifdef RX_CSUM_OFFLOAD
static MV_STATUS mgiRx_csum_offload(MV_PKT_INFO *pkt_info)
{
    if( (pkt_info->pFrags->dataSize > ETH_CSUM_MIN_BYTE_COUNT)   && /* Minimum        */
        (pkt_info->status & ETH_RX_IP_FRAME_TYPE_MASK) && /* IPv4 packet    */
        (pkt_info->status & ETH_RX_IP_HEADER_OK_MASK)  && /* IP header OK   */
        (!(pkt_info->fragIP))                          && /* non frag IP    */
/*  for TCP packet add this lines      
    (!(pkt_info->status & ETH_RX_L4_OTHER_TYPE))   &&  L4 is TCP/UDP  */
        ((pkt_info->status & ETH_RX_L4_UDP_TYPE))   &&    /* L4 UDP         */
        (pkt_info->status & ETH_RX_L4_CHECKSUM_OK_MASK) ) /* L4 checksum OK */
            return MV_OK;

    if(!(pkt_info->pFrags->dataSize > ETH_CSUM_MIN_BYTE_COUNT))
        MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX, ("Byte count smaller than %d\n", ETH_CSUM_MIN_BYTE_COUNT) );
    if(!(pkt_info->status & ETH_RX_IP_FRAME_TYPE_MASK))
        MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX, ("Unknown L3 protocol\n") );
    if(!(pkt_info->status & ETH_RX_IP_HEADER_OK_MASK))
        MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX, ("Bad IP csum\n") );
    if(pkt_info->fragIP)
        MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX, ("Fragmented IP\n") );
    if(pkt_info->status & ETH_RX_L4_OTHER_TYPE)
        MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX, ("Unknown L4 protocol\n") );
    if(!(pkt_info->status & ETH_RX_L4_CHECKSUM_OK_MASK))
        MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX, ("Bad L4 csum\n") );

    return MV_FAIL;
}
#endif

/***********************************************************/

LOCAL  int mgiRxReady(DRV_CTRL *pDrvCtrl, int queue, int quota)
{
    M_BLK_ID    pMblk   = NULL;
    int         count = 0;
    MV_PKT_INFO *pktInfo;

    /* While there are RFDs to process */
    while( count < quota)
    {
        /* Get the packet from device */
        pktInfo = mvEthPortRx(pDrvCtrl->pEthPortHndl, queue);
        if(pktInfo == NULL)
        {
            /* no more rx packets ready */
            break;
        }
        /* packet received */

            /* if no errors indicated in cmdSts, and Mblk is    */
            /* succesfully generated with BufInfo parameters    */
		if (pktInfo->status & (ETH_ERROR_SUMMARY_MASK))
		{
			MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_ERR, 
							 ("mgiRx: cmdSts Error: ifId=%d, cmdSts=0x%x, dataSize=%d\n", 
							  pDrvCtrl->unit, (unsigned int)pktInfo->status, pktInfo->pFrags->dataSize));

			MGI_IN_ERRORS_ADD(&pDrvCtrl->endObj, 1);
			mvEthPortRxDone(pDrvCtrl->pEthPortHndl, queue, pktInfo);
			continue;
		}
        pMblk = (M_BLK_ID)pktInfo->osInfo;

        mvOsCacheUnmap(NULL, (void *)mvOsIoVirtToPhy(NULL, pMblk->pClBlk->clNode.pClBuf), 
                            pktInfo->pFrags->dataSize);

        pMblk->mBlkHdr.mData = pMblk->pClBlk->clNode.pClBuf + ETH_MV_HEADER_SIZE;

		/* mvOsCacheLineInv(NULL,pMblk->pClBlk->clNode.pClBuf); 				*/
		/* cacheInvalidate(DATA_CACHE, pMblk->mBlkHdr.mData, pktInfo.pktSize);  */


            MGI_DEBUG_CODE(MGI_DEBUG_FLAG_PKT, 
            mgiCheckRxPkt(pMblk->mBlkHdr.mData, pktInfo->pFrags->dataSize));
         
        pMblk->mBlkHdr.mLen = pktInfo->pFrags->dataSize - ETH_MV_HEADER_SIZE;
            pMblk->mBlkPktHdr.len = pMblk->mBlkHdr.mLen;
            pMblk->mBlkHdr.mNext = NULL;
        pMblk->pClBlk->clFreeArg3 |= (pMblk->mBlkHdr.mLen << 16);

            /* update MIB counters */
        if(mgiStatsControl == 2)
        {
            MGI_IN_COUNTERS_UPDATE(&pDrvCtrl->endObj, pMblk);
        }
        else
        {
            pMblk->mBlkHdr.mFlags = M_EXT | M_PKTHDR;

            if(mgiStatsControl == 1)
            {
            pDrvCtrl->inPackets++;
            pDrvCtrl->inOctets += pMblk->mBlkPktHdr.len;
            }
        }
/*
        printf("\n mgi_rx_%d: pktInfo=%p, pMblk=%p, pData=%p, size=%d\n", 
                   pDrvCtrl->debugStats.rxPktsCount[queue], pktInfo,
                       pMblk, pMblk->mBlkHdr.mData, pMblk->mBlkHdr.mLen);
        mgiPrintMblk(pMblk);
            mvDebugMemDump(pMblk->mBlkHdr.mData, 64, 1);
            printf("\n");
*/
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_RX, 
            ("\n mgiRx #%d: pktSize=%d, pData=0x%x, pMblk=0x%x\n", 
            pDrvCtrl->debugStats.rxReadyJobCount, pktInfo->pFrags->dataSize, 
                (unsigned int)pMblk->mBlkHdr.mData, (unsigned int)pMblk));

            MGI_DEBUG_CODE((MGI_DEBUG_FLAG_DUMP | MGI_DEBUG_FLAG_RX), 
                    (mvDebugMemDump(pMblk->mBlkHdr.mData, 64, 1)) );

            MGI_DEBUG_CODE((MGI_DEBUG_FLAG_MBLK | MGI_DEBUG_FLAG_RX), 
                    (mgiPrintMblk(pMblk)));

#ifdef RX_CSUM_OFFLOAD
        /* checksum offload */
            if( mgiRx_csum_offload( &pktInfo ) == MV_OK ) 
            {
                /* Here should be the code to inform the TCPIP stack for fragmented packet 
                to be check the checksum by UDP stack. - meanwhile not supported.   */
            }
#endif
                
            /* deliver the received Packet up to the stack */
#ifdef MGI_SEMAPHORE
            semGive(pDrvCtrl->semId);
#endif /* MGI_SEMAPHORE */

#ifdef INCLUDE_BRIDGING
            if(pDrvCtrl->pDstEnd != NULL)
            {
                pDrvCtrl->pDstEnd->pFuncTable->send(pDrvCtrl->pDstEnd, pMblk);
            }
            else
#endif /* INCLUDE_BRIDGING */

#ifdef MGI_DEBUG
            {
                STATUS      vxStatus;

                errnoSet(0);
                vxStatus = pDrvCtrl->endObj.receiveRtn(&pDrvCtrl->endObj, pMblk, 
                                                        NULL, NULL, NULL, NULL);
                if( (vxStatus != OK) || (errnoGet() != 0) )
                {
                    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_RX | MGI_DEBUG_FLAG_ERR, 
                           ("END_RCV_RTN_CALL failed: Status=0x%x, errno=0x%x\n",
                           vxStatus, errnoGet()) );
                }
            }
#else
            END_RCV_RTN_CALL(&pDrvCtrl->endObj, pMblk);
#endif /* MGI_DEBUG */

#ifdef MGI_SEMAPHORE
            semTake(pDrvCtrl->semId, WAIT_FOREVER);
#endif /* MGI_SEMAPHORE */

            MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, (pDrvCtrl->debugStats.rxPktsCount[queue]++));         
        count++;
    }       

    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_RX,
               ("mgiRx for IfId_%d complete, %d Packet(s) received\n\n", 
                pDrvCtrl->unit, (int)count) );

    return count;
}

/*******************************************************************************
* mgiRxReadyJob - Service task-level interrupts for receive frames.
*
* DESCRIPTION:
*       This routine runs in netTask's context.  The ISR scheduled
*       this routine so that it could handle receive packets at task level.
*       First the routine tries to retrieve network pool elements in order to
*       assemble mBlk-clBlk-cluster tuple. After retrieving those elements the
*       routine calls the low level API to get a Rx buffer. This buffer is
*       created as a network pool cluster. After connecting all three parts of
*       the tupple, the mBlk pointer is passed to the MUX.
*       This routine is active as long as there are Rx frames to process.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       Using network pool mBlk and clBlk, Rx information is passed to
*       the MUX receive routine.   the MUX recieve routine.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL void mgiRxReadyJob(DRV_CTRL *pDrvCtrl, MV_U32 cause)
{
    int         pktCounter = 0;
    STATUS  rc;
    
    MGI_LOCK(pDrvCtrl);

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, ("ENTER mgiRxReadyJob: ifId=%d, cause=0x%x\n", 
                                        pDrvCtrl->unit, cause) );
    MGI_TRACE_ADD(MGI_TRACE_RX_READY_JOB_ENTER);
    pDrvCtrl->debugStats.rxReadyJobCount++;         

#if (MV_ETH_RX_Q_NUM > 1)
    while(MV_TRUE)
    {
        int rxq;

        if(cause == 0)
            break;

        rxq = mgiRxQueueGet(cause);

        pktCounter += mgiRxReady(pDrvCtrl, rxq, pDrvCtrl->portRxQuota - pktCounter);
        if(pktCounter < pDrvCtrl->portRxQuota)
            cause &= ~(ETH_CAUSE_RX_READY_MASK(rxq));
        else
            break;
    }
#else
    pktCounter = mgiRxReady(pDrvCtrl, ETH_DEF_RXQ, pDrvCtrl->portRxQuota);
#endif /* (MV_ETH_RX_Q_NUM > 1) */

    MGI_TRACE_ADD(pktCounter);
    MGI_TRACE_ADD(MGI_TRACE_RX_READY_JOB_EXIT);

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, ("EXIT mgiRxReadyJob: ifId=%d\n", 
                                        pDrvCtrl->unit) );

#ifdef MGI_DEBUG
    pDrvCtrl->rxPktsDist[pktCounter]++;
#endif /* MGI_DEBUG */
        
    if(pktCounter < pDrvCtrl->portRxQuota)
    {
        /* Re-enable RxReady interrupt */
        MV_REG_WRITE( ETH_INTR_MASK_REG(pDrvCtrl->ethPortNo), MGI_RX_READY_MASK);
    }
    else
    {
        /* Read cause once more */
        cause |= MV_REG_READ( ETH_INTR_CAUSE_REG(pDrvCtrl->ethPortNo));
        cause &= MGI_RX_READY_MASK;
        MV_REG_WRITE( ETH_INTR_CAUSE_REG(pDrvCtrl->ethPortNo), ~cause);

        /* schedule the RxReadyJob in netTask context */
        rc = netJobAdd( (FUNCPTR)mgiRxReadyJob, (int)pDrvCtrl, cause, 0, 0, 0 );
        if(rc != OK)
            pDrvCtrl->debugStats.netJobAddError++;
    }

    MGI_UNLOCK(pDrvCtrl);

    return;
}

/*******************************************************************************
* mgiRxDone - Return a Receive Frame back on the Receive Queue.
*
* DESCRIPTION:
*       This routine returns the Rx resource back to the low level driver
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*       ETH_RX_DESC *   pRxDesc  pointer to a RFD
*
* OUTPUT:
*       Rx resource is cache invalidate and returned to low level driver.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL void mgiRxDone(DRV_CTRL *pDrvCtrl, MV_PKT_INFO *pPktInfo, int queue)
{
    M_BLK_ID    pMblk   = NULL;

    MGI_LOCK(pDrvCtrl);

    if( (queue & ~0xffff) == 0)
    {
        mvOsPrintf("mgi%d: quueue=0x%x, mgiRxDone Double free - pMblk=%p, pPktInfo=%p\n", 
                    pDrvCtrl->ethPortNo, queue, pMblk, pPktInfo);
        MGI_UNLOCK(pDrvCtrl);
        return;
    }
    queue &= 0xffff;

    MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, (pDrvCtrl->debugStats.rxDoneCount++));

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, 
                    ("ENTER mgiRxDone: ifId=%d\n", pDrvCtrl->unit) );

    MGI_TRACE_ADD(MGI_TRACE_RX_DONE_ENTER);

    pMblk = mgiGenerateMblk(pDrvCtrl, pPktInfo, queue);
    if(pMblk == NULL)
    {
        mvOsPrintf("mgiRxDone failed: Can't get Mblk+Clblk\n");
        MGI_UNLOCK(pDrvCtrl);
        return;
    }
    pPktInfo->osInfo = (MV_U32)pMblk;

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_RX_DONE, 
            ("\n mgiRxDone #%d: pMblk=0x%x, pPktInfo=0x%x, queue=%d\n", 
            pDrvCtrl->debugStats.rxDoneCount, (unsigned int)pMblk, 
            (unsigned int)pPktInfo, queue));

    /* Invalidate the cluster current information. */
    pPktInfo->pktSize = MV_MAX(pPktInfo->pFrags->dataSize, (pMblk->mBlkHdr.mLen + 
                                (pMblk->mBlkHdr.mData - pMblk->pClBlk->clNode.pClBuf)));    

    /*pktInfo.pktSize = (MGI_RX_BUF_DEF_SIZE + _CACHE_ALIGN_SIZE);*/
    pPktInfo->pktSize += _CACHE_ALIGN_SIZE;
    pMblk->mBlkHdr.mNext    = NULL;
    pMblk->mBlkHdr.mNextPkt = NULL;

    MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, 
                            (pDrvCtrl->debugStats.rxDonePktsCount[queue]++));

    mvEthPortRxDone(pDrvCtrl->pEthPortHndl, queue, pPktInfo);

    MGI_TRACE_ADD(MGI_TRACE_RX_DONE_EXIT);

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, 
                    ("EXIT mgiRxDone: ifId=%d\n", pDrvCtrl->unit) );

    MGI_UNLOCK(pDrvCtrl);
    return;
}

/* Free all already sent buffers */
LOCAL int mgiTxDone(DRV_CTRL *pDrvCtrl, MV_U32 cause)
{
    MV_PKT_INFO *pktInfo;
    MV_U32      pktCounter;
    M_BLK_ID    pMblk;
    int         queue;

    pktCounter = 0;
    queue = 0;
    while(cause != 0)
    {

#if (MV_ETH_TX_Q_NUM > 1)
        while( (cause & ETH_CAUSE_TX_BUF_MASK(queue)) == 0)
        {
            queue++;
        }
#else
        queue = ETH_DEF_TXQ;
        cause = 0;
#endif /* (MV_ETH_TX_Q_NUM > 1) */

        while(MV_TRUE)
        {
            pktInfo = mvEthPortTxDone(pDrvCtrl->pEthPortHndl, queue);
            if(pktInfo == NULL)
            {
                cause &= ~(ETH_CAUSE_TX_BUF_MASK(queue));
                queue++;
                break;
            }
            MGI_DEBUG_CODE(MGI_DEBUG_FLAG_STATS, (pDrvCtrl->debugStats.txDonePktsCount[queue]++));

            /* soon notify upper protocols CFDs are available */
            if (pDrvCtrl->txStall)
            {
                MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TX, 
                            ("mgiTxDone: stack resume: ifId=%d\n",
                                pDrvCtrl->unit));

                pDrvCtrl->debugStats.stackResume++;

                netJobAdd((FUNCPTR)muxTxRestart, (int)&pDrvCtrl->endObj, 0,0,0,0);

                pDrvCtrl->txStall = MV_FALSE;
            }

            if(pktInfo->status & (ETH_ERROR_SUMMARY_MASK))
            {
                MGI_OUT_ERRORS_ADD(&pDrvCtrl->endObj, 1);
            }

            pMblk = (M_BLK_ID)pktInfo->osInfo;

            MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TX_DONE, 
                     ("mgiTxDone%d %d: queue=%d, cause=0x%x, pktSize=%d\n",
                     pDrvCtrl->unit, pDrvCtrl->debugStats.txDoneJobCount,
                     queue, cause, pMblk->mBlkPktHdr.len));

            /* Release the mBlk chain of the transmitted packet */
            if(pMblk != NULL)
            {
                netMblkClChainFree(pMblk);
                pktCounter++;
            }
        }
    }
    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TX_DONE, 
               ("mgiTxDone: ifId=%d, %d Packet(s) released\n\n", 
               pDrvCtrl->unit, (int)pktCounter) );

#ifdef MGI_DEBUG
    pDrvCtrl->txDonePktsDist[pktCounter]++;
#endif /* MGI_DEBUG */

    return pktCounter;
}

/*******************************************************************************
* mgiTxDoneJob - Free used Tx descriptors and mBlks.
*
* DESCRIPTION:
*       This routine runs in netTask's context.  The ISR scheduled this
*       routine so that it could handle Tx packet resource release at task
*       level.
*       This routine free used Tx descriptors as well as mBlks. mBlks to
*       release are located in the PKT_INFO returnInfo field. The mgiSend
*       routine places the mBlk pointer in that field only in case the Tx desc
*       is a packet last buffer.
*       In case the Tx process was in 'stall' situation, the routine returns
*       the Tx process to normal and notified the upper layers that the 'stall'
*       situation is over.
*       This routine is active as long as there are Tx resources to release.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl       pointer to DRV_CTRL structure
*
* OUTPUT:
*       Return Tx resources to low level driver and release mBlk struct.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL void mgiTxDoneJob(DRV_CTRL *pDrvCtrl, MV_U32 cause)
{
    int pktCounter;

    /* Interlock with mgiSend */
    MGI_LOCK(pDrvCtrl);

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, ("ENTER mgiTxDoneJob: ifId=%d, cause=0x%x\n", 
                                pDrvCtrl->unit, cause) );

    pDrvCtrl->debugStats.txDoneJobCount++;

    MGI_TRACE_ADD(MGI_TRACE_TX_DONE_JOB_ENTER);

#if (MV_ETH_TX_Q_NUM > 1)
    /* Read cause once more */
    cause |= MV_REG_READ( ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo));
    MGI_DEBUG_PRINT( MGI_DEBUG_FLAG_TX_DONE, 
               ("mgiTxDone: ifId=%d, cause = 0x%x\n\n", 
               pDrvCtrl->unit, cause) );
    cause &= MGI_TX_DONE_MASK;
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(pDrvCtrl->ethPortNo), ~cause);
#endif /* (MV_ETH_TX_Q_NUM > 1) */

    pktCounter = mgiTxDone(pDrvCtrl, cause);

    MGI_TRACE_ADD(pktCounter);
    MGI_TRACE_ADD(MGI_TRACE_TX_DONE_JOB_EXIT);

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_TRACE, ("EXIT mgiTxDoneJob\n") );

    /* Re-enable TxDone interrupt !!! Remember reenable MISC interrupts too*/
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(pDrvCtrl->ethPortNo), 
                                (MGI_TX_DONE_MASK | MGI_MISC_MASK));

    MGI_UNLOCK(pDrvCtrl);
}


MV_STATUS    mgiRxRefill(DRV_CTRL *pDrvCtrl, int queue, int num)
{
    MV_U8*          pBufferPtr;
    MV_BUF_INFO     *pBufInfo;
    MV_PKT_INFO     *pPktInfo;
    M_BLK_ID        pMblk   = NULL;
    MV_STATUS       status;
    int             count;

    count = mvEthRxResourceGet(pDrvCtrl->pEthPortHndl, queue);
    /* Connect netpool clusters to desc buffer pointer using Rx cluster  */
    /* release routine. Those clusters never returns !!                  */
    while(count < num)
    {
        if ((pBufferPtr = (MV_U8*)netClusterGet(pDrvCtrl->endObj.pNetPool,
                                           pDrvCtrl->pClPoolId)) == NULL)
        {
            printf("netClusterGet failed\n");
            return MV_NO_RESOURCE;
        }
        /*mvOsPrintf("RxRefill: %d of %d, pBuf=%p\n", count, num, pBufferPtr);*/

        pPktInfo = mvOsMalloc(sizeof(MV_PKT_INFO));
        if(pPktInfo == NULL)
        {
            mvOsPrintf("mgi%d: Can't allocate %d bytes for MV_PKT_INFO\n",
					   pDrvCtrl->ethPortNo,
                       sizeof(MV_PKT_INFO));
            return MV_NO_RESOURCE;
        }

        pBufInfo = mvOsMalloc(sizeof(MV_BUF_INFO));
        if(pBufInfo == NULL)
        {
            mvOsPrintf("mgi%d: Can't allocate %d bytes for MV_BUF_INFO\n",
					   pDrvCtrl->ethPortNo,
                       sizeof(MV_BUF_INFO));
            mvOsFree(pPktInfo);
            return MV_NO_RESOURCE;
        }
        pPktInfo->pFrags = pBufInfo;

        pBufInfo->bufVirtPtr = pBufferPtr;
        pBufInfo->bufPhysAddr = mvOsIoVirtToPhy(NULL, pBufInfo->bufVirtPtr);
        pBufInfo->bufSize 	 = MGI_RX_BUF_DEF_SIZE;
        pBufInfo->dataSize 	 = 0;
        pPktInfo->pktSize 	 = pBufInfo->bufSize; /* how much to invalidate */

        pMblk = mgiGenerateMblk(pDrvCtrl, pPktInfo, queue);
        if(pMblk == NULL)
        {
            mvOsPrintf("%s%d Error: Can't generate Mblk+Clblk+Cluster tuple\n",
                        DEV_NAME, pDrvCtrl->unit);

            mvOsFree(pBufInfo);
            mvOsFree(pPktInfo);

            return MV_NO_RESOURCE;
        }
        pPktInfo->osInfo 	 = (MV_U32)pMblk;

        status = mvEthPortRxDone(pDrvCtrl->pEthPortHndl, queue, pPktInfo);
        if(status != MV_OK)
        {
            if(status != MV_FULL)
            {
                /* Can't return last buffer. Free it back to netPool */
                pMblk->pClBlk->pClFreeRtn = NULL;
                pMblk->pClBlk->clFreeArg1 = 0;
                pMblk->pClBlk->clFreeArg2 = 0;
                pMblk->pClBlk->clFreeArg3 = 0;
				mvOsFree(pBufInfo);
				mvOsFree(pPktInfo);
                netMblkClChainFree(pMblk);
            }
            return status;
        }
        count++;
    }    
    return MV_OK;
}

/*******************************************************************************
* mgiGenerateMblk - Allocate and set a new tuple M_BLK-CL_CLK-Cluster
*
* DESCRIPTION:
*       The routine performs the following:
*           - allocates M_BLK from the Interface pool
*           - allocates CL_BLK from the Interface pool
*           - initializes CL_CLK with the mgiRxDone Release function and 
*                       its input parameters.
*           - M_BLK and CL_BLK attachment
*           - initialises M_BLK parameters
*       If any of the steps is failed, the function cleans the previous steps.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl  - pointer to Driver Control structure
*
* OUTPUT:
*       New tuple M_BLK-CL_CLK-Cluster is ready
*
* RETURN:
*       M_BLK_ID - pointer to the generated M_BLK
*       NULL - on failure
*
*******************************************************************************/
static M_BLK_ID     mgiGenerateMblk(DRV_CTRL *pDrvCtrl, MV_PKT_INFO* pPktInfo, int rxQueue)
{
    END_OBJ*    pEndObj = &pDrvCtrl->endObj;
    NET_POOL_ID pNetPool = pEndObj->pNetPool;
    M_BLK_ID    pMblk = NULL;
    CL_BLK_ID   pClBlk = NULL;

    /* allocate MBlk */
    if( !(pMblk = netMblkGet( pNetPool, M_DONTWAIT, MT_DATA )) )
    {
        printf("netMblkGet failed\n");
        goto Failure;
    }

    /* allocate ClBlk */
    if( !(pClBlk = netClBlkGet( pNetPool, M_DONTWAIT )) )
    {
        printf("netClBlkGet failed\n");
        goto Failure;
    }

    /* associate ClBlk with actual data and release information */
    if( !netClBlkJoin( pClBlk, (char *)pPktInfo->pFrags->bufVirtPtr, CL_BUFF_SIZE, 
                        (FUNCPTR)mgiRxDone, (int)pDrvCtrl, (int)pPktInfo, rxQueue) )
    {
        printf("netClBlkJoin failed\n");
        goto Failure;
    }
            
    /* associate Mblk with ClBlk */
    if( !(netMblkClJoin( pMblk, pClBlk )) )
    {
        printf("netMblkClJoin failed\n");
        goto Failure;
    }

    return pMblk;

Failure:
    pDrvCtrl->debugStats.genMblkError++;
    if( pMblk )
        netMblkFree( pNetPool, pMblk );
    if( pClBlk )
        netClBlkFree( pNetPool, pClBlk );
    return NULL;
}

/* Convert Mblk-Clblk-Cluster list to BUF_INFO-Buffer list */
LOCAL MV_STATUS mgiMblkToPktInfo(DRV_CTRL *pDrvCtrl, M_BLK_ID pMblkPkt, MV_PKT_INFO* pPktInfo)
{
    M_BLK_ID    pMblk = NULL;
    MV_BUF_INFO *pBufInfo = pPktInfo->pFrags;
    int         bufCount = 0;
    
    pMblk = pMblkPkt;

    while(pMblk && pMblk->mBlkHdr.mLen && pMblk->mBlkHdr.mData)
    {
        if(bufCount >= MGI_MAX_BUF_PER_PKT)
        {
            /* !!! Exception */
            mvOsPrintf("MblkPkt has too much clusters: max = %d\n",
                    MGI_MAX_BUF_PER_PKT);
            return MV_FAIL;
        }
        /* set a BufInfo */
        pBufInfo[bufCount].bufVirtPtr = (MV_U8*)pMblk->mBlkHdr.mData;
        pBufInfo[bufCount].dataSize = pMblk->mBlkHdr.mLen;
    
        /* move to next Mblk */
        pMblk = pMblk->mBlkHdr.mNext;

        /* move to next BufInfo */
        bufCount++;
    }
#ifdef MGI_DEBUG
    if(bufCount > pDrvCtrl->debugStats.txMaxBufCount)
        pDrvCtrl->debugStats.txMaxBufCount = bufCount;
#endif /* MGI_DEBUG */

    /* Store pointer to Mblk structure in MV_PKT_INFO */
    pPktInfo->osInfo = (MV_ULONG)pMblkPkt;
    pPktInfo->numFrags = bufCount;
    pPktInfo->pktSize = pMblkPkt->mBlkPktHdr.len;

    return MV_OK;
}

#if defined(MGI_STATS_POLLING) 
int     mgiStatsDump(DRV_CTRL *pDrvCtrl)
{
    END_IFCOUNTERS *    pEndStatsCounters;
    UINT32              high32;

    pEndStatsCounters = &pDrvCtrl->endStatsCounters;

    /* Get number of RX'ed octets */
    pEndStatsCounters->ifInOctets = pDrvCtrl->inOctets;
    pDrvCtrl->inOctets = 0;

    /* Get number of TX'ed octets */
    pEndStatsCounters->ifOutOctets = mvEthMibCounterRead (pDrvCtrl->pEthPortHndl, 
                        ETH_MIB_GOOD_OCTETS_SENT_LOW, &high32);
    pEndStatsCounters->ifOutOctets |= (unsigned long long)high32 << 32;

    /* Get RX'ed unicasts, broadcasts, multicasts */
    pEndStatsCounters->ifInBroadcastPkts = mvEthMibCounterRead(pDrvCtrl->pEthPortHndl, 
                                        ETH_MIB_BROADCAST_FRAMES_RECEIVED, NULL);
    
    pEndStatsCounters->ifInMulticastPkts = mvEthMibCounterRead(pDrvCtrl->pEthPortHndl, 
                                        ETH_MIB_MULTICAST_FRAMES_RECEIVED, NULL);

    if(pDrvCtrl->inPackets > 
        (pEndStatsCounters->ifInMulticastPkts + pEndStatsCounters->ifInBroadcastPkts))
    {
        pEndStatsCounters->ifInUcastPkts = pDrvCtrl->inPackets -
                                    (pEndStatsCounters->ifInMulticastPkts +
                                     pEndStatsCounters->ifInBroadcastPkts);
    }
    else
        pEndStatsCounters->ifInUcastPkts = 0;

    pDrvCtrl->inPackets = 0;

    /* Get TX'ed unicasts, broadcasts, multicasts */
    pEndStatsCounters->ifOutBroadcastPkts = mvEthMibCounterRead(pDrvCtrl->pEthPortHndl, 
                                        ETH_MIB_BROADCAST_FRAMES_SENT, NULL);

    pEndStatsCounters->ifOutMulticastPkts = mvEthMibCounterRead(pDrvCtrl->pEthPortHndl, 
                                        ETH_MIB_MULTICAST_FRAMES_SENT, NULL);

    pEndStatsCounters->ifOutUcastPkts = mvEthMibCounterRead(pDrvCtrl->pEthPortHndl, 
                                        ETH_MIB_GOOD_FRAMES_SENT, NULL);

    pEndStatsCounters->ifOutUcastPkts -= (pEndStatsCounters->ifOutMulticastPkts +
                                     pEndStatsCounters->ifOutBroadcastPkts);

    return OK;
}
#endif /* MGI_STATS_POLLING */

/*******************************************************************************
* mgiIoctl - Interface ioctl procedure
*
* DESCRIPTION:
*       Process an interface ioctl request.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl      pointer to DRV_CTRL structure
*       int            cmd      command to process
*       caddr_t       data      pointer to data
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK, or ERROR if the config command failed.
*
*******************************************************************************/
static int mgiIoctl(DRV_CTRL *pDrvCtrl, unsigned int cmd, caddr_t data)
{
    int         error = OK;
    long        value;
    END_OBJ *   pEndObj = &pDrvCtrl->endObj;

    switch (cmd)
    {

        case EIOCGNPT:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                     ("Ioctl: ifId=0x%x cmd=EIOCGNPT(0x%x), data=0x%x\n",
                     pDrvCtrl->unit, cmd, (int)data));
            error = EINVAL;
            break;

        case EIOCGNAME:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                     ("Ioctl: ifId=0x%x cmd=EIOCGNAME(0x%x), data=0x%x\n",
                     pDrvCtrl->unit, cmd, (unsigned int)data));
            strcpy((char*)data, END_DEV_NAME(pDrvCtrl->endObj));
            break;
         
        case EIOCGHDRLEN:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                     ("Ioctl: ifId=0x%x cmd=EIOCGHDRLEN(0x%x), data=0x%x\n",
                     pDrvCtrl->unit, cmd, (unsigned int)data));
            *(long *)data =  SIZEOF_ETHERHEADER;
            break;

        case EIOCSADDR:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                     ("Ioctl: ifId=0x%x cmd=EIOCSADDR(0x%x), data=0x%x\n",
                     pDrvCtrl->unit, cmd, (unsigned int)data));
            
            if (data == NULL)
                error = EINVAL;
            else
            {
                /* Copy and install the new address */
                bcopy ((char *) data,
					   (char *) MGI_HADDR (&pDrvCtrl->endObj),
                       MGI_HADDR_LEN(&pDrvCtrl->endObj));
        
                mvEthMacAddrSet(pDrvCtrl->pEthPortHndl, (MV_U8*)data, pDrvCtrl->rxUcastQ);
            }
            break;

        case EIOCGADDR:                      
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                     ("Ioctl: ifId=0x%x, cmd=EIOCGADDR(0x%x), data=0x%x\n",
                     pDrvCtrl->unit, cmd, (int)data) );
            if (data == NULL)
                error = EINVAL;
            else
                bcopy ((char *) MGI_HADDR (&pDrvCtrl->endObj),
                       (char *) data,
                       MGI_HADDR_LEN (&pDrvCtrl->endObj));
            break;

        case EIOCSFLAGS:
            {
                long oldFlags = END_FLAGS_GET(pEndObj);

                MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                    ("Ioctl: ifId=%d, cmd=EIOCSFLAGS(0x%x), data=0x%08lx, oldFlags=0x%08lx\n",
                    pDrvCtrl->unit, cmd, (long)data, oldFlags) );

                value = (long) data;
                if (value < 0)
                {
                    value = -value;
                    value--;
                    END_FLAGS_CLR (pEndObj, value);
                }
                else
                    END_FLAGS_SET(pEndObj, value);

                MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, 
                        ("value=0x%08lx, oldFlags=0x%08lx, newFlags=0x%08lx\n", 
                        value, oldFlags, END_FLAGS_GET(pEndObj) )); 
                mgiSetFlags(pDrvCtrl, oldFlags);
            }
            break;

        case EIOCGFLAGS:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, 
                            ("Ioctl: ifId=0x%x, cmd=EIOCGFLAGS(0x%x), data=0x%x\n",
                              pDrvCtrl->unit, cmd, (int)data) );

            if (data == NULL)
                error = EINVAL;
            else
                *(long *)data = END_FLAGS_GET(pEndObj);

            break;

        case EIOCMULTIADD:            
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                      ("Ioctl: ifId=0x%x, cmd=EIOCMULTIADD(0x%x), data=0x%x\n",
                        pDrvCtrl->unit, cmd, (int)data) );

            error = mgiMCastAddrAdd (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIDEL:            
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                      ("Ioctl: ifId=0x%x, cmd=EIOCMULTIDEL(0x%x), data=0x%x\n",
                        pDrvCtrl->unit, cmd, (int)data) );

            error = mgiMCastAddrDel (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIGET:            
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                      ("Ioctl: ifId=0x%x, cmd=EIOCMULTIGET(0x%x), data=0x%x\n",
                      pDrvCtrl->unit, cmd, (int)data) );

            error = mgiMCastAddrGet (pDrvCtrl, (MULTI_TABLE *) data);
            break;

        case EIOCPOLLSTART:            
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                    ("Ioctl: ifId=0x%x, cmd=EIOCPOLLSTART(0x%x), data=0x%x\n",
                    pDrvCtrl->unit, cmd, (int)data) );

            error = EINVAL; /* No support for Polling operation mode */
            break;

        case EIOCPOLLSTOP:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                    ("Ioctl: ifId=0x%x, cmd=EIOCPOLLSTART(0x%x), data=0x%x\n",
                    pDrvCtrl->unit, cmd, (int)data) );
            error = EINVAL; /* No support for Polling operation mode */
            break;

        case EIOCGMIB2:              
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                    ("Ioctl: ifId=0x%x, cmd=EIOCGMIB2(0x%x), data=0x%x\n",
                        pDrvCtrl->unit, cmd, (int)data) );

            if (data == NULL)
                error = EINVAL;
            else
                bcopy ((char *)&pEndObj->mib2Tbl, (char *) data,
                                                    sizeof (pEndObj->mib2Tbl));
            break;

#ifdef INCLUDE_MGI_END_2233_SUPPORT
        case EIOCGMIB2233:              
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL,
                    ("Ioctl: ifId=0x%x, cmd=EIOCGMIB2233(0x%x), data=0x%x\n",
                        pDrvCtrl->unit, cmd, (int)data) );
            if( (data == NULL) || (pEndObj->pMib2Tbl == NULL) )
                error = EINVAL;
            else
                *((M2_ID**)data) = pEndObj->pMib2Tbl;
            break;
#endif /* INCLUDE_MGI_END_2233_SUPPORT */

#if defined(MGI_STATS_POLLING)
        case EIOCGPOLLCONF:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, 
                            ("Ioctl: ifId=0x%x, cmd=EIOCGPOLLCONF(0x%x), data=0x%x\n",
                              pDrvCtrl->unit, cmd, (int)data) );

            if ((data == NULL))
                error = EINVAL;
            else
                *((END_IFDRVCONF **)data) = &pDrvCtrl->endStatsConf;
            break;

        case EIOCGPOLLSTATS:
            if( (data == NULL) || (mgiStatsControl != 1) )
                error = EINVAL;
            else
                {
                error = mgiStatsDump(pDrvCtrl);
                if (error == OK)
                    *((END_IFCOUNTERS **)data) = &pDrvCtrl->endStatsCounters;
                }
            break;
#endif /* MGI_STATS_POLLING */

        default:
            MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, 
                    ("Ioctl: UNKNOWN COMMAND: ifId=0x%x, cmd=0x%x, data=0x%x!! \n", 
                    pDrvCtrl->unit, cmd, (int)data) );
            error = EINVAL;
    }

    return (error);
}

/* Free all resources allocated for this interface */
void     mgiCleanup(DRV_CTRL *pDrvCtrl)
{
    int     portNo = pDrvCtrl->ethPortNo;
    STATUS  osStatus;
    int     i;

    /* Free allocated memory if necessary */
    if (pDrvCtrl->pClsBase != NULL)
    {
#if defined(MV_UNCACHED_RX_BUFFERS)
        mvOsIoUncachedFree(NULL, 0, 0, pDrvCtrl->pClsBase);
#else /* Normal case: Buffers in cached SDRAM */
        mvOsIoCachedFree(NULL, 0, 0, pDrvCtrl->pClsBase,0);
#endif /* MV_UNCACHED_RX_BUFFERS */
    }

    if(pDrvCtrl->pMblkBase != NULL)
        mvOsFree(pDrvCtrl->pMblkBase);

    if(pDrvCtrl->endObj.pNetPool != NULL)
    {
        osStatus = netPoolDelete(pDrvCtrl->endObj.pNetPool);
        if(osStatus != OK)
        {
            mvOsPrintf("mgiCleanup: Can't Delete netPool: osStatus=0x%x\n", osStatus);
        }
        mvOsFree(pDrvCtrl->endObj.pNetPool);
    }
	for(i=0; i<ETH_NUM_OF_TX_DESCR; i++)
	{
		if(pDrvCtrl->txPktInfo[i] != NULL)
		{
			if(pDrvCtrl->txPktInfo[i]->pFrags != NULL)
				mvOsFree(pDrvCtrl->txPktInfo[i]->pFrags);

			mvOsFree(pDrvCtrl->txPktInfo[i]);
		}
	}         
	if(pDrvCtrl->txPktInfo != NULL)
	{
		mvOsFree(pDrvCtrl->txPktInfo);
	}


#ifdef INCLUDE_MGI_END_2233_SUPPORT
    if(pDrvCtrl->endObj.pMib2Tbl != NULL)
    {
        m2IfFree(pDrvCtrl->endObj.pMib2Tbl);
        pDrvCtrl->endObj.pMib2Tbl = NULL;
    }
#endif /* INCLUDE_MGI_END_2233_SUPPORT */

    mvOsFree(pDrvCtrl);

    mgiDrvCtrl[portNo] = NULL;
}

void     mgiSetFlags(DRV_CTRL *pDrvCtrl, long oldFlags)
{
    STATUS          status;
    ETHER_MULTI*    pEtherMulti;
    long            newFlags = END_FLAGS_GET(&pDrvCtrl->endObj);
    
    if( ((oldFlags & IFF_PROMISC) != 0) && 
        ((newFlags & IFF_PROMISC) == 0) )
    {
        /* Clear IFF_PROMISC flag */
        mvEthRxFilterModeSet(pDrvCtrl->pEthPortHndl, MV_FALSE);
        /* Restore Unicast address */
        mvEthMacAddrSet(pDrvCtrl->pEthPortHndl, pDrvCtrl->macAddr, pDrvCtrl->rxUcastQ);

        /* Add all existing Multicast addresses to Hardware !!!!*/
        pEtherMulti = END_MULTI_LST_FIRST(&pDrvCtrl->endObj);
        while(pEtherMulti != NULL)
        {
            mvEthMcastAddrSet(pDrvCtrl->pEthPortHndl, 
                            (MV_U8*)&pEtherMulti->addr[0], pDrvCtrl->rxMcastQ);
            pEtherMulti = END_MULTI_LST_NEXT(pEtherMulti);
        }
        mvOsPrintf("%s%d Interface: Setting promiscuous mode OFF!\n", 
                    DEV_NAME, pDrvCtrl->unit);
    }
    if( ((oldFlags & IFF_PROMISC) == 0) && 
        ((newFlags & IFF_PROMISC) != 0) )
    {
        /* Set IFF_PROMISC flag */
        MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_IOCTL, 
                        ("mgiSetFlags: Setting promiscuous mode ON!\n"));
        mvEthRxFilterModeSet(pDrvCtrl->pEthPortHndl, MV_TRUE);
        mvOsPrintf("%s%d Interface: Setting promiscuous mode ON!\n", 
                    DEV_NAME, pDrvCtrl->unit);
    }

    if( ((oldFlags & IFF_UP) != 0) && 
        ((newFlags & IFF_UP) == 0) )
    {
        /* Clear IFF_UP flag */
/*
        ethPortDisable(pDrvCtrl->pEthPortHndl);
        mvOsPrintf("%s%d Interface Down\n", DEV_NAME, pDrvCtrl->unit);
*/
        status = mgiStop(pDrvCtrl);
        if(status != OK)
        {
            printf("MGI: can't stop %s%d interface\n", 
                    DEV_NAME, pDrvCtrl->unit);
        }
    }    
    if( ((oldFlags & IFF_UP) == 0) && 
        ((newFlags & IFF_UP) != 0) )
    {
        /* Set IFF_UP flag */
/*
        ethPortEnable(pDrvCtrl->pEthPortHndl);
        mvOsPrintf("%s%d Interface Up\n", DEV_NAME, pDrvCtrl->unit);
*/
        status = mgiStart(pDrvCtrl);
        if(status != OK)
        {
            printf("MGI: can't start %s%d interface\n", 
                    DEV_NAME, pDrvCtrl->unit);
        }
    }
}

int mgiFreeTxResources(DRV_CTRL *pDrvCtrl, int queue)
{
    int             counter = 0;
    MV_PKT_INFO     *pktInfo;
    M_BLK_ID        pMblk   = NULL;

    while(MV_TRUE)
    {
        pktInfo = mvEthPortForceTxDone(pDrvCtrl->pEthPortHndl, queue);
        if(pktInfo == NULL)
            break;

        counter++;
        pMblk = (M_BLK_ID)pktInfo->osInfo;
        /* Release the mBlk chain of the transmitted packet */
        if(pMblk != NULL)
        {
            if( pMblk->pClBlk->pClFreeRtn == ((FUNCPTR)mgiRxDone) )
            {
                /* This Mblk belongs to one of our mgi interfaces*/
                if( ((DRV_CTRL*)(pMblk->pClBlk->clFreeArg1))->started == MV_FALSE)
                {
                    /* mgi intefrace is stopped, so return buffer to pool */
                    pMblk->pClBlk->pClFreeRtn = NULL;
                    pMblk->pClBlk->clFreeArg1 = 0;
                    pMblk->pClBlk->clFreeArg2 = 0;
                    pMblk->pClBlk->clFreeArg3 = 0;                    
                }
            }
            MGI_DEBUG_CODE(MGI_DEBUG_FLAG_INIT | MGI_DEBUG_FLAG_MBLK, 
                   (mgiPrintMblk(pMblk)));
            netMblkClChainFree(pMblk);
        }
    }    
    if(counter > 0)
        MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_INIT, 
                    ("mgi%d: Free %d TX Mblk+ClBlk+Buffer tuples\n", 
                        pDrvCtrl->unit, counter));
    return counter;
}

int mgiFreeRxResources(DRV_CTRL *pDrvCtrl, int queue)
{
    int             counter = 0;
    MV_PKT_INFO     *pktInfo;
    M_BLK_ID        pMblk   = NULL;

    while(MV_TRUE)
    {
        pktInfo = mvEthPortForceRx(pDrvCtrl->pEthPortHndl, queue);
        if(pktInfo == NULL)
            break;

        counter++;
        pMblk = (M_BLK_ID)pktInfo->osInfo;
        /* Release the mBlk chain to driver NetPool */
        if(pMblk != NULL)
        {
            if( pMblk->pClBlk->pClFreeRtn == ((FUNCPTR)mgiRxDone) )
            {
                pMblk->pClBlk->pClFreeRtn = NULL;
                pMblk->pClBlk->clFreeArg1 = 0;
                pMblk->pClBlk->clFreeArg2 = 0;
                pMblk->pClBlk->clFreeArg3 = 0;
            }
            MGI_DEBUG_CODE(MGI_DEBUG_FLAG_INIT | MGI_DEBUG_FLAG_MBLK, 
                   (mgiPrintMblk(pMblk)));

            netMblkClChainFree(pMblk);
            mvOsFree(pktInfo->pFrags);
            mvOsFree(pktInfo);
        }
    }    
    if(counter > 0) 
        MGI_DEBUG_PRINT (MGI_DEBUG_FLAG_INIT, 
                    ("mgi%d: Free %d RX Mblk+ClBlk+Buffer tuples\n", 
                        pDrvCtrl->unit, counter));

    return counter;
}

/*******************************************************************************
* mgiSetUcastQ - Define which Queue will be used for Unicast packets.
*
* DESCRIPTION:
*
* INPUT:
*       DRV_CTRL*   pDrvCtrl    - Pointer to DRV_CTRL structure
*       int         queue       - Queue for Unicast
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
STATUS  mgiSetUcastQ(DRV_CTRL *pDrvCtrl, int queue)
{
    pDrvCtrl->rxUcastQ = queue;
    return OK;
}

/*******************************************************************************
* mgiSetMcastQ - Define which Queue will be used for Multicast packets.
*
* DESCRIPTION:
*
* INPUT:
*       DRV_CTRL*   pDrvCtrl    - Pointer to DRV_CTRL structure
*       int         queue       - Queue for Multicast
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
STATUS  mgiSetMcastQ(DRV_CTRL *pDrvCtrl, int queue)
{
    pDrvCtrl->rxMcastQ = queue;
    return OK;
}

/*******************************************************************************
* mgiRxQuotaSet - Set Rx quota for specific interface.
*
* DESCRIPTION:
*
* INPUT:
*       DRV_CTRL*   pDrvCtrl    - Pointer to DRV_CTRL structure
*       int         rxQuota     - Quota for RX processing
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
STATUS  mgiRxQuotaSet(DRV_CTRL *pDrvCtrl, int rxQuota)
{
    if( (rxQuota <= 0) || (rxQuota > MGI_PORT_RX_MAX_QUOTA) )
    {
        mvOsPrintf("mgi%d Error: rxQuota must be from 1 to %d\n", 
                    pDrvCtrl->unit, MGI_PORT_RX_MAX_QUOTA);
        return ERROR;
    }
    pDrvCtrl->portRxQuota = rxQuota;
    return OK;
}

/*******************************************************************************
* mgiMCastAddrAdd - Add a multicast address for the device.
*
* DESCRIPTION:
*       This routine adds a multicast address to whatever the driver is
*       already listening for.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl      pointer to DRV_CTRL structure
*       char *      pAddr       address to be added
*
* OUTPUT:
*       Multicast address will be accepted.
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
LOCAL STATUS mgiMCastAddrAdd(DRV_CTRL *pDrvCtrl, char *pAddr)
{
    int     retVal;

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, 
                    ("mgiMCastAddrAdd ifId=%d: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    pDrvCtrl->unit, pAddr[0]&0xFF, pAddr[1]&0xFF, pAddr[2]&0xFF,
                    pAddr[3]&0xFF, pAddr[4]&0xFF, pAddr[5]&0xFF));

    retVal = etherMultiAdd (&pDrvCtrl->endObj.multiList, pAddr);
    if (retVal == ENETRESET)
    {
        pDrvCtrl->endObj.nMulti++;

        if (pDrvCtrl->endObj.nMulti > N_MCAST)
        {
            etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);
            pDrvCtrl->endObj.nMulti--;
        }
        else
        {
            /* If Promiscous mode don't access Hardware */
            if( END_FLAGS_ISSET(IFF_PROMISC) == MV_FALSE)            
                mvEthMcastAddrSet(pDrvCtrl->pEthPortHndl, (MV_U8 *)pAddr, pDrvCtrl->rxMcastQ);
        }
        retVal = OK;
    }

    return ((retVal == OK) ? OK : ERROR);

}

/*******************************************************************************
* mgiMCastAddrDel - Delete a multicast address for the device.
*
* DESCRIPTION:
*       This routine deletes a multicast address from the current list of
*       multicast addresses.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl      pointer to DRV_CTRL structure
*       char *      pAddr       address to be added
*
* OUTPUT:
*       Multicast address will be rejected.
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
LOCAL STATUS mgiMCastAddrDel(DRV_CTRL *pDrvCtrl, char *pAddr)
{
    int     retVal;

    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, 
                    ("mgiMCastAddrDel ifId=%d: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    pDrvCtrl->unit, pAddr[0]&0xFF, pAddr[1]&0xFF, pAddr[2]&0xFF,
                    pAddr[3]&0xFF, pAddr[4]&0xFF, pAddr[5]&0xFF));

    retVal = etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);
    if (retVal == ENETRESET)
    {
        pDrvCtrl->endObj.nMulti--;

        /* If Promiscous mode don't access Hardware */
        if( END_FLAGS_ISSET(IFF_PROMISC) == MV_FALSE)            
            mvEthMcastAddrSet(pDrvCtrl->pEthPortHndl, (MV_U8 *)pAddr, -1);

        retVal = OK;
    }
    return ((retVal == OK) ? OK : ERROR);
}

/*******************************************************************************
* mgiMCastAddrGet - Get the current multicast address list.
*
* DESCRIPTION:
*       This routine returns the current multicast address list in <pTable>.
*
* INPUT:
*       DRV_CTRL *pDrvCtrl      pointer to DRV_CTRL structure
*       MULTI_TABLE *pTable     table into which to copy addresses
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK or ERROR.
*
*******************************************************************************/
LOCAL STATUS mgiMCastAddrGet(DRV_CTRL *pDrvCtrl, MULTI_TABLE *pTable)
{
    MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_IOCTL, ("mgiMCastAddrGet\n"));

    return (etherMultiGet (&pDrvCtrl->endObj.multiList, pTable));
}

/*******************************************************************************
* mgiPollSend - Transmit a packet in polled mode.
*
* DESCRIPTION:
*       This routine is called by a user to try and send a packet on the
*       device. It sends a packet directly on the network, without having to
*       go through the normal process of queuing a packet on an output queue
*       and the waiting for the device to decide to transmit it.
*
*       This routine should not call any kernel functions.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       DRV_CTRL *pDrvCtrl      pointer to DRV_CTRL structure
*       M_BLK_ID    pMblk       pointer to the mBlk/cluster pair
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK or EAGAIN.
*
*******************************************************************************/
LOCAL STATUS mgiPollSend(DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk)
{
    /* No support for Poll mode */
    return (OK);
}

/*******************************************************************************
* mgiPollReceive - Receive a packet in polled mode.
*
* DESCRIPTION:
*       This routine is called by a user to try and get a packet from the
*       device. It returns EAGAIN if no packet is available. The caller must
*       supply a M_BLK_ID with enough space to contain the received packet. If
*       enough buffer is not available then EAGAIN is returned.
*
*       This routine should not call any kernel functions.
*       NOTE: This routine is not supported in this version !!
*
* INPUT:
*       DRV_CTRL *pDrvCtrl      pointer to DRV_CTRL structure
*       M_BLK_ID    pMblk       pointer to the mBlk/cluster pair
*
* OUTPUT:
*       None.
*
* RETURN:
*       OK or EAGAIN.
*
*******************************************************************************/
LOCAL STATUS mgiPollReceive(DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk)
{
    /* No support for Poll mode */
    return (OK);
}

/*******************************************************************************
* getBootlineMAC - Get the user defined MAC if defined.
*
* DESCRIPTION:
*       This function checks the bootline to see if there is a user defined
*       MAC address for the given port.
*
* INPUT:
*       int portNum       -  Port number.
*       char *portMacAddr -  Pointer to returned char array.
*
* OUTPUT:
*       In case the MAC is valid the given char array is filled with the MAC
*       address values. In case the MAC is not valid or the MAC for that port
*       is not found the function returns. In this case the passed array
*       (portMacArray) is not modified.
*
* RETURN:
*       None.
*
*******************************************************************************/
LOCAL MV_STATUS getBootlineMAC(int portNum, MV_U8 *portMacAddr)
{
    char*   macBootlineStr;
    char    macName[5];

    sprintf (macName, "%s%d-", "MAC", portNum);
    macBootlineStr = macFindInBootline(macName);
    if(macBootlineStr == NULL)
    {
        MGI_DEBUG_PRINT(MGI_DEBUG_FLAG_INIT, 
                        ("Did not find MAC name in bootline! \n") );
        return MV_NOT_FOUND;
    }
    mvMacStrToHex(macBootlineStr, portMacAddr);

    return MV_OK;
}


/*******************************************************************************
* macFindInBootline - Search the bootline for a given MAC address.
*
* DESCRIPTION:
*       This function search the bootline looking for MAC address described by
*       the given parameter. For example for port0 the given parameter will be
*       "MAC0-". If this string is found within the bootline the function
*       returns a pointer to the actual MAC address following the MAC name.
*
* INPUT:
*       char *macName - MAC name. For example port 0 MAC name is "MAC0-".
*
* OUTPUT:
*       None.
*
* RETURN:
*       - Pointer to the MAC string in the bootline.
*       - NULL if the MAC name was not found.
*
*******************************************************************************/
LOCAL char* macFindInBootline(char *macName)
{
    char    *subStr;        /* Bootline sub string that holds the MAC name */

    subStr = strstr(sysBootLine, macName);
    if (subStr == NULL)
        return NULL;
    else
        return (subStr + strlen(macName));
}

#ifdef INCLUDE_BRIDGING
void    mgiSetDstEndObj(int unit, END_OBJ* pEndObj)
{
    DRV_CTRL*       pDrvCtrl = mgiDrvCtrl[unit];

    if(pDrvCtrl == NULL)
    {
        printf("%s%d: No such interface\n", DEV_NAME, unit);
        return;
    }
    pDrvCtrl->pDstEnd = pEndObj;
}
#endif /* INCLUDE_BRIDGING */

/******************************************************************************/
/*************                Debug functions                ******************/
/******************************************************************************/

/* Print Multicast table of the port */
void    mgiPrintMcastTable(int port)
{
    int             i, multiCount;
    ETHER_MULTI*    pEtherMulti;
    DRV_CTRL*       pDrvCtrl = mgiDrvCtrl[port];
    char            macStr[MV_MAC_STR_SIZE];

    if(pDrvCtrl == NULL)
        return;

    multiCount = END_MULTI_LST_CNT(&pDrvCtrl->endObj);

    mvOsPrintf("\n\t ethPort #%d: %d Multicast addresses\n\n", 
                port, multiCount);
    pEtherMulti = END_MULTI_LST_FIRST(&pDrvCtrl->endObj);
    i = 0;
    while(pEtherMulti != NULL)
    {
        mvMacHexToStr((MV_U8*)&pEtherMulti->addr[0], macStr);
        mvOsPrintf("%d. MAC = %s, refcount = %ld\n", 
                    i, macStr, pEtherMulti->refcount);
        pEtherMulti = END_MULTI_LST_NEXT(pEtherMulti);
        i++;
    }
}

/* Print Mblk structure */
void    mgiPrintMblk( M_BLK_ID pMblk )
{
    M_BLK_ID    pMblkPkt = pMblk;

    while(pMblkPkt)
    {

        mvOsPrintf("\n Start of Packet: pMblkPkt=0x%x, pktLen=%d\n", 
                (unsigned int)pMblkPkt, pMblkPkt->mBlkPktHdr.len);

        while( pMblk )
        {
            mvOsPrintf("pNetPool=0x%x, pMblk: 0x%x:\n", 
                (unsigned int)pMblk->pClBlk->pNetPool, (unsigned int)pMblk);

            mvOsPrintf("pMblk->mBlkHdr.mData         = 0x%x\n",       
                (unsigned int)pMblk->mBlkHdr.mData);
            mvOsPrintf("pMblk->mBlkHdr.mFlags        = 0x%x\n",     
                (unsigned int)pMblk->mBlkHdr.mFlags);
            mvOsPrintf("pMblk->mBlkHdr.mLen          = %d\n",       
                pMblk->mBlkHdr.mLen);
            mvOsPrintf("pMblk->mBlkHdr.mNext         = 0x%x\n",       
                (unsigned int)pMblk->mBlkHdr.mNext);
            mvOsPrintf("pMblk->mBlkHdr.mNextPkt      = 0x%x\n",       
                (unsigned int)pMblk->mBlkHdr.mNextPkt);
            mvOsPrintf("pMblk->mBlkHdr.mType         = 0x%x\n",     
                (unsigned int)pMblk->mBlkHdr.mType);
            mvOsPrintf("pMblk->pClBlk->clNode.pClBuf = 0x%x\n",       
                (unsigned int)pMblk->pClBlk->clNode.pClBuf);
            mvOsPrintf("pMblk->pClBlk->clSize        = %d\n",       
                pMblk->pClBlk->clSize);
            mvOsPrintf("pMblk->pClBlk->clRefCnt      = %d\n",       
                pMblk->pClBlk->clRefCnt);
            mvOsPrintf("pMblk->pClBlk->pClFreeRtn    = 0x%x\n",       
                (unsigned int)pMblk->pClBlk->pClFreeRtn);
            mvOsPrintf("pMblk->pClBlk->clFreeArg1    = 0x%x\n",       
                (unsigned int)pMblk->pClBlk->clFreeArg1);
            mvOsPrintf("pMblk->pClBlk->clFreeArg2    = 0x%x\n",       
                (unsigned int)pMblk->pClBlk->clFreeArg2);
            mvOsPrintf("pMblk->pClBlk->clFreeArg3    = 0x%x\n\n",     
                pMblk->pClBlk->clFreeArg3);

            pMblk = pMblk->mBlkHdr.mNext;
            if((pMblk == NULL) || (pMblk->mBlkHdr.mLen == 0))
                break;
        }
        pMblkPkt = pMblkPkt->mBlkHdr.mNextPkt;
    }
    mvOsPrintf("\n");
}

/* Print information general for all ports */
void    mgiStatus(void)
{
#ifdef USE_PRIV_NET_POOL_FUNC
    mvOsPrintf("MblkClFree=%u, MblkClFreeRtn=%u, MblkFree=%u, DoubleFree=%u, Ref=%u\n", 
                mgiMblkClFreeCount, mgiMblkClFreeRtnCount, 
                mgiMblkFreeCount, mgiMblkClDoubleFreeCount, mgiMblkClRefFreeCount);
#endif
}

/* Print MGI Port status and statistics */
void    mgiPortStatus(int port)
{
    DRV_CTRL*   pDrvCtrl;

    mvOsPrintf("\t MGI port #%d Status\n\n", port);

    pDrvCtrl = mgiDrvCtrl[port];
    if(pDrvCtrl == NULL)
    {
        mvOsPrintf("mgiPort_#d is not exist\n", port);
        return;
    }
    mvOsPrintf("Interface %s%d State: %s %s %s\n", DEV_NAME, pDrvCtrl->unit,
                pDrvCtrl->attached ? "Loaded" : "Unloaded", 
                pDrvCtrl->started ? "+ Started" : "+ Stopped",
                pDrvCtrl->isUp ? "+ Up" : "+ Down");

    mvOsPrintf("\t pEndObj=0x%x, txSem=0x%x, pNetPool=0x%x, pMblkBase=0x%x, flags=0x%08lx\n", 
                (unsigned int)&pDrvCtrl->endObj, (unsigned int)pDrvCtrl->semId, 
                (unsigned int)pDrvCtrl->endObj.pNetPool, (unsigned int)pDrvCtrl->pMblkBase, 
                (unsigned long)END_FLAGS_GET(&pDrvCtrl->endObj));

    mvOsPrintf("\t ethPortNo=%d, ethPortHndl=0x%x, rxQuota=%d\n", 
               pDrvCtrl->ethPortNo, (unsigned int)pDrvCtrl->pEthPortHndl,
               pDrvCtrl->portRxQuota);

    mvOsPrintf("StackBP=%u, StackResume=%u\n", 
                pDrvCtrl->debugStats.stackBP, pDrvCtrl->debugStats.stackResume);

    mvOsPrintf("TxFailed=%u, RxFailed=%u, GenMblkError=%u, NetJobAddError=%u\n",
            pDrvCtrl->debugStats.txFailedCount, pDrvCtrl->debugStats.rxFailedCount, 
            pDrvCtrl->debugStats.genMblkError, pDrvCtrl->debugStats.netJobAddError);

    mvOsPrintf("MiscIsr=%u, linkUp=%u, linkDown=%u, linkMiss=%u\n",
                pDrvCtrl->debugStats.miscIsrCount, pDrvCtrl->debugStats.linkUpCount, 
                pDrvCtrl->debugStats.linkDownCount, pDrvCtrl->debugStats.linkMissCount);

        mvOsPrintf("RxReadyIsr=%u, TxDoneIsr=%u\n",
                    pDrvCtrl->debugStats.rxReadyIsrCount, pDrvCtrl->debugStats.txDoneIsrCount);
                    
    mvOsPrintf("RxReadyJob=%u, TxDoneJob=%u\n", 
                    pDrvCtrl->debugStats.rxReadyJobCount, pDrvCtrl->debugStats.txDoneJobCount);
#ifdef MGI_DEBUG
    {
        int   i, queue;

        mvOsPrintf("\n");

        mvOsPrintf("RxDone=%u\n", pDrvCtrl->debugStats.rxDoneCount);
        for(queue=0; queue<MV_ETH_RX_Q_NUM; queue++)
        {
            mvOsPrintf("rxQ_%d: rxPkts=%u, rxDonePkts=%u\n",
                    queue, pDrvCtrl->debugStats.rxPktsCount[queue], 
                    pDrvCtrl->debugStats.rxDonePktsCount[queue]);
        }
        mvOsPrintf("\n");
        mvOsPrintf("TxPkts=%u, TxMaxPkts=%u, TxMaxBuf=%d\n", 
                    pDrvCtrl->debugStats.txCount, pDrvCtrl->debugStats.txMaxPktsCount, 
                    pDrvCtrl->debugStats.txMaxBufCount);

        for(queue=0; queue<MV_ETH_TX_Q_NUM; queue++)
        {
            mvOsPrintf("txQ_%d: txPkts=%u, txDonePkts=%u\n",
                    queue, pDrvCtrl->debugStats.txPktsCount[queue], 
                    pDrvCtrl->debugStats.txDonePktsCount[queue]);
        }
        mvOsPrintf("\n");

        for(i=0; i<=MGI_PORT_RX_MAX_QUOTA; i++)
        {
            if(pDrvCtrl->rxPktsDist[i] != 0)
                mvOsPrintf("%d RxPkts - %u times\n", i, pDrvCtrl->rxPktsDist[i]);
        }

        mvOsPrintf("\n");
        for(i=0; i<pDrvCtrl->txDonePktsDistSize; i++)
        {
            if(pDrvCtrl->txDonePktsDist[i] != 0)
                mvOsPrintf("%d TxDonePkts - %d times\n", i, pDrvCtrl->txDonePktsDist[i]);
        }
    }
#endif /* MGI_DEBUG */
}

#ifdef MGI_DEBUG
void    mgiPortIntrCntrGet(int port,MV_U32 *pTxDoneIsrCount, MV_U32 *pRxDoneIsrCount )
{
    DRV_CTRL*   pDrvCtrl;
    pDrvCtrl = mgiDrvCtrl[port];
    if(pDrvCtrl == NULL)
    {
        mvOsPrintf("mgiPort_#d is not exist\n", port);
        return;
    }
    *pTxDoneIsrCount = pDrvCtrl->debugStats.txDoneIsrCount;
    *pRxDoneIsrCount  = pDrvCtrl->debugStats.rxReadyIsrCount;
}
#endif /* MGI_DEBUG */

/* Clear MGI status and statistics */
void mgiClearStatus(void)
{
#ifdef MGI_DEBUG
    int i=0;
#endif /* MGI_DEBUG */
    int port;

    for(port=0; port<BOARD_ETH_PORT_NUM; port++)
    {
        if(mgiDrvCtrl[port] != NULL)
            memset(&mgiDrvCtrl[port]->debugStats, 0, sizeof(mgiDrvCtrl[port]->debugStats));

#ifdef MGI_DEBUG
        for(i=0; i<=MGI_PORT_RX_MAX_QUOTA; i++)
            mgiDrvCtrl[port]->rxPktsDist[i] = 0;

        for(i=0; i<mgiDrvCtrl[port]->txDonePktsDistSize; i++)
            mgiDrvCtrl[port]->txDonePktsDist[i] = 0;
#endif /* MGI_DEBUG */

    }
#ifdef USE_PRIV_NET_POOL_FUNC
    mgiMblkClFreeCount = mgiMblkClDoubleFreeCount = 0;
    mgiMblkClFreeRtnCount = mgiMblkFreeCount = mgiMblkClRefFreeCount = 0;
#endif

#ifdef MGI_DEBUG_TRACE
    {
        int i;

        for(i=0; i<MGI_TRACE_ARRAY_SIZE; i++)
        {
            mgiTrace[i] = 0xFF;
        }
        mgiTraceIdx = 0;
    }
#endif /* MGI_DEBUG_TRACE */
}

#ifdef MGI_DEBUG

int     mgiCheckMode = 0;
int     mgiCheckOffset = 0;
int     mgiCheckSize  = 0;
/* not used 
MV_U8   mgiCheckPattern[MGI_RX_BUF_DEF_SIZE];
*/ 
MV_U16  mgiIpIdentifier = 0;
MV_U16  mgiIpFragmentOffset = 0;
#define MV_ICMP_PROTOCOL_ID     1

/* Print IP header */
void    mgiPrintIpHeader(MV_IP_HEADER*  pIpHeader) 
{
    mvOsPrintf("IP header: totSize=0x%04x, id=0x%04x, fragCtrl=0x%04x, prot=0x%02x\n", 
                pIpHeader->totalLength, pIpHeader->identifier, 
                pIpHeader->fragmentCtrl, pIpHeader->protocol);
}

/* Check and print received ICMP packets */
void    mgiCheckRxPkt(MV_U8* pData, int size)
{
    MV_802_3_HEADER*    pMacHeader = (MV_802_3_HEADER*)pData;
    MV_IP_HEADER*       pIpHeader;

    if(mgiCheckMode == 1)
    {
        /* Check constant pattern 
        if( memcmp(mgiCheckPattern, pData+mgiCheckOffset, mgiCheckSize) != 0)
        {
            printf("mgiRx: Check pattern error: checkOffset=%d, checkSize=%d\n",
                        mgiCheckOffset, mgiCheckSize);
            printf("\t checkPattern\n");
            mvDebugMemDump(mgiCheckPattern, mgiCheckSize, 1);
            printf("\t Received packet\n");
            mvDebugMemDump(pData, size, 1);                        
        }
		*/
    }
    else
    {
        /* Check identifier and fragment offset of IP packets */
        if(pMacHeader->typeOrLen == MV_16BIT_BE(MV_IP_TYPE))
        {
            /* Check only IP packets */
            pIpHeader = (MV_IP_HEADER*)(((MV_U8*)pMacHeader) + sizeof(MV_802_3_HEADER));


            /* Check only ICMP packets */
            if(pIpHeader->protocol == MV_ICMP_PROTOCOL_ID)
            {
                mgiPrintIpHeader(pIpHeader);
                /* For all fragments should be the same identification */
                if( (pIpHeader->fragmentCtrl & 0x1FFF) == 0)
                {
                    /* First fragment. Check Identifier is great than previous */
                    if(pIpHeader->identifier <= mgiIpIdentifier)
                    {
                        mvOsPrintf("IP id out of order: last=0x%04x >= recv=0x%04x\n",
                                    mgiIpIdentifier, pIpHeader->identifier);
                    }
                    else
                    {
                        mgiIpIdentifier = pIpHeader->identifier;
                        mgiIpFragmentOffset = 
                            (pIpHeader->totalLength - sizeof(MV_IP_HEADER))/8;
                    }
                }
                else
                {
                    /* Fragment: check Identifier */
                    if(pIpHeader->identifier != mgiIpIdentifier)
                    {
                        mvOsPrintf("Wrong IP id for the fragment: exp=0x%04x != recv=0x%04x\n",
                                    mgiIpIdentifier, pIpHeader->identifier);
                    }
                    /* Check Fragment offset */
                    if(mgiIpFragmentOffset != (pIpHeader->fragmentCtrl & 0x1FFF))
                    {
                        mvOsPrintf("Wrong IP fragOffset: exp=0x%04x != recv=0x%04x\n",
                                    mgiIpFragmentOffset, (pIpHeader->fragmentCtrl & 0x1FFF));
                    }
                    mgiIpFragmentOffset += (pIpHeader->totalLength - sizeof(MV_IP_HEADER))/8;

                    if( (pIpHeader->fragmentCtrl & 0xE000) == 0x2000)
                    {

                        /* Not last fragment must be 1500 bytes */
                        if(pIpHeader->totalLength != 1500)
                        {
                            mvOsPrintf("Wrong IP totalLength: received=0x%04x != 1500\n",
                                    pIpHeader->totalLength);
                        }
                    }
                }
            }
        }        
    }
}
#endif /* MGI_DEBUG */

#ifdef __cplusplus
}
#endif
