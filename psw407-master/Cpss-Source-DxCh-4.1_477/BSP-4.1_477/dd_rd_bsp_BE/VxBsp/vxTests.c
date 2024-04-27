 /*******************************************************************************
 *
 *  THIS CODE IS NOT SUPPORTED CODE.  IT IS PROVIDED ON AN AS-IS BASIS.
 *
 *******************************************************************************/

#include "mvOs.h"
#include "mvTypes.h"
#include "mvCommon.h"
#include "mvDebug.h"
#include "mv802_3.h"
#include "mvCommon.h"
#include "mvEth.h"
#include "vxWorks.h"
#include "version.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "socket.h"
#include "netinet/in.h" 
#include "sockLib.h" 
#include "inetLib.h" 
#include "ioLib.h" 
#include "stdio.h"
#include "muxLib.h"
#include "wdLib.h"
#include "logLib.h"
#include "sysLib.h"
#include "mvCntmr.h"
#if defined(MV645xx)
#include "db64560.h"
#elif defined(MV646xx)
#include "db64660.h"
#elif defined (MV78XX0)
#include "db78XX0.h"
#endif
#include "iv.h"
#include "intLib.h"
#include "mvTwsi.h"
#include "mvXorRegs.h"
#include "mgiEnd.h"
#include "etherMultiLib.h"
#include "end.h"
#include "mvEth.h"


char dumpLog[200];
char dumpLog2[200];
char dumpLog3[200];
char dumpLog4[200];

volatile int isInit = 0;
volatile int testNum = 1;

extern void*    vxFindDevCookie(char* pDevName, int unit);
extern STATUS   mgiSetMcastQ(END_OBJ* pEndObj, int queue);
extern STATUS   mgiSetUcastQ(END_OBJ* pEndObj, int queue);
 
unsigned char vxTaskTrace[1024*16];
unsigned long vxTaskSwitchCount = 0;

void    vxTaskSwitchHook(WIND_TCB *pOldTcb, WIND_TCB *pNewTcb)
{
    char tempBuf[200];

    /*printf("%3ld\n", vxTaskSwitchCount);*/
#if (_WRS_VXWORKS_MAJOR == 6)
    sprintf(tempBuf, "%3ld. %s --> %s\n", vxTaskSwitchCount, pOldTcb->objCore.name, pNewTcb->objCore.name);
#else
#ifdef _WRS_VXWORKS_5_X 
    sprintf(tempBuf, "%3ld. %s --> %s\n", vxTaskSwitchCount, pOldTcb->name, pNewTcb->name);
#endif
#endif
    strcat(vxTaskTrace, tempBuf);
    vxTaskSwitchCount++;
}

void    vxTaskTraceStart(void)
{
    if(vxTaskSwitchCount == 0)
    {
        if( taskSwitchHookAdd((FUNCPTR)vxTaskSwitchHook) != OK)
        {
            printf("vxTaskTrace: Failed add taskSwirtch hook\n");
        }
    }
    memset(vxTaskTrace, 0, sizeof(vxTaskTrace));
    vxTaskSwitchCount = 0;
}


STATUS   vxTestStartDriver(char* devName, int unit, MV_BOOL isStart)
{
    void* 	pCookie;
	STATUS	status;

    pCookie = vxFindDevCookie(devName, unit);
    if(pCookie == NULL)
    {
        printf("Device: %s Unit %d not found\n", devName, unit);
        return ERROR;
    }
    if(isStart)
        status = muxDevStart(pCookie);
    else
        status = muxDevStop(pCookie);

    if(status != OK)
        printf("Can't start/stop Device %s%d: status=0x%x\n", devName, unit, status);

    return status;
}

STATUS   vxTestSetUcast(char* devName, int unit, char* macAddrStr, int queue)
{
    MV_U8       macAddr[MV_MAC_ADDR_SIZE];
    END_OBJ*    pEndObj;
    void*       pCookie;
    STATUS      status;

    pCookie = vxFindDevCookie(devName, unit);
    pEndObj = endFindByName(devName, unit);
    if( (pCookie == NULL) || (pEndObj == NULL) )
    {
        printf("Device: %s Unit %d not found\n", devName, unit);
        return ERROR;
    }
    if( (queue < 0) || (queue >= MV_ETH_RX_Q_NUM) )
    {
        printf("Invalid Queue number\n");
        return ERROR;
    }
    mvMacStrToHex(macAddrStr, &macAddr[0]);


    mgiSetUcastQ(pEndObj, queue);
    status = muxIoctl(pCookie, EIOCSADDR, macAddr);

    return status;
}

STATUS   vxTestSetMcast(char* devName, int unit, char* macAddrStr, int queue)
{
    void*   pCookie;
    END_OBJ*    pEndObj;
	STATUS	status;
    MV_U8   macAddr[MV_MAC_ADDR_SIZE];

    pCookie = vxFindDevCookie(devName, unit);
    pEndObj = endFindByName(devName, unit);
    if( (pCookie == NULL) || (pEndObj == NULL) )
    {
        printf("Device: %s Unit %d not found\n", devName, unit);
        return ERROR;
    }
    mvMacStrToHex(macAddrStr, &macAddr[0]);
    if(queue >= 0)
    {
        mgiSetMcastQ(pEndObj, queue);
        status = muxMCastAddrAdd(pCookie, &macAddr[0]);
    }
    else
    {
        status = muxMCastAddrDel(pCookie, &macAddr[0]);
    }

    if(status != OK)
        printf("Can't add/delete Mcast (%s) to device %s%d: status=0x%x\n", 
                macAddrStr, devName, unit, status);

    return status;
}
/*----------------------------------------------------------------------------*/

/******************************************************************************/
/************               UDP and TCP tests             *********************/
/******************************************************************************/

unsigned long	    vxTestTaskId = 0;
int                 vxTestBufSize = 0;
int					vxTestSockBufSize = 0;
int                 vxTestRcvPort = 0;
unsigned long       vxTestTotalSize = 0;
unsigned long       vxTestRcvSize = 0;
unsigned long       vxTestRateSize = 0;
int					vxTestMode = 0;
MV_BOOL             vxTestIsForever = MV_FALSE;
int 	            vxTestWdTime = 60;
WDOG_ID	            vxTestWd = NULL;
int                 vxTestRcvSock = 0;
int                 vxTestListenSock = 0;
char                vxTestRcvAddr[50];
char                vxTestMcastAddr[50];


void    vxTestSetWdTime(int sec)
{
    vxTestWdTime = sec;
}

void    vxTestPrintRate(void)
{
    unsigned long mbits, rate, remainder;

    if( (vxTestWd==NULL) || (vxTestWdTime == 0) )
        return;

    if(vxTestRateSize > 0)
    {
        mbits = vxTestRateSize/(1024*1024/8);
        rate = mbits/vxTestWdTime;
        remainder = ((mbits % vxTestWdTime)*10)/vxTestWdTime;

 	    logMsg("Rcv Rate: %3d.%d Mbits/sec\n", rate, remainder, 0, 0, 0, 0);
	    vxTestRateSize = 0;
	}
    else
	{
	    logMsg("No bytes read in the last %d seconds.\n", vxTestWdTime, 0, 0, 0, 0, 0);
	}
    wdStart(vxTestWd, sysClkRateGet()*vxTestWdTime, (FUNCPTR)vxTestPrintRate, 0);
}


unsigned __TASKCONV vxTestUdpRcvTask(void* args)
{
    struct sockaddr_in  fromAddr;  
    struct sockaddr_in  sin;  
    struct ip_mreq      ipMreq;  
    int                 recvLen, fromLen; 
    char *              recvBuf = NULL;

    vxTestRcvSock = 0;

    if(vxTestBufSize == 0)
    {
        vxTestBufSize = 2000;
    }
    if(vxTestRcvPort == 0)
        vxTestRcvPort = 4001;

    vxTestRcvSock = socket(AF_INET, SOCK_DGRAM, 0);
    if(vxTestRcvSock < 0)  
    { 
        printf (" cannot open UDP recv socket\n");  
        return 1; 
    } 

    memset((char*)&sin, 0, sizeof (sin)); 
    memset((char*)&fromAddr, 0, sizeof(fromAddr)); 
    fromLen = sizeof(fromAddr);  

    sin.sin_len = (u_char) sizeof(sin);  
    sin.sin_family = AF_INET;  
    sin.sin_port = htons (vxTestRcvPort); 
    sin.sin_addr.s_addr = INADDR_ANY;  

    /* bind a port number to the socket */ 
    if( bind(vxTestRcvSock, (struct sockaddr*)&sin, sizeof(sin)) != 0) 
    { 
        printf (" cannot bind UDP recv socket\n");  
        goto cleanUp; 
    } 

    /* allocate buffer to receive message */
    recvBuf = malloc(vxTestBufSize);
    if(recvBuf == NULL)
    {
        printf("Can't allocate %d bytes for RCV buffer\n", vxTestBufSize);  
        goto cleanUp; 
    }

    if(vxTestMcastAddr[0] != '\0')
    {
        /* initialize the multicast address to join */ 
        ipMreq.imr_multiaddr.s_addr = inet_addr (vxTestMcastAddr);           
        if(vxTestRcvAddr[0] != '\0')
            ipMreq.imr_interface.s_addr = inet_addr (vxTestRcvAddr); 
        else
            ipMreq.imr_interface.s_addr = INADDR_ANY;

        /* set the socket option to join the MULTICAST group */ 
        if (setsockopt (vxTestRcvSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                        (char *)&ipMreq,  sizeof (ipMreq)) < 0) 
        {  
            printf ("setsockopt IP_ADD_MEMBERSHIP error:\n");  
            goto cleanUp; 
        }
    }

    vxTestWd = wdCreate();
    if(vxTestWd == NULL)
    {
        printf ("Can't create watchdog\n");  
        goto cleanUp; 
    }

    /* Start watchdog after a minute*/
    wdStart(vxTestWd, sysClkRateGet () * vxTestWdTime, (FUNCPTR)vxTestPrintRate, 0);

    /* Wait for message */
    if(vxTestIsForever)
        printf("Waiting on UDP socket to receive data FOREVER\n");
    else
        printf("Waiting on UDP socket to receive %d bytes of data\n", (int)vxTestTotalSize);

	vxTestRcvSize = 0;
    vxTestRateSize = 0;
    while( (vxTestIsForever) || (vxTestRcvSize < vxTestTotalSize) )
    {
        if ((recvLen = recvfrom(vxTestRcvSock, recvBuf, vxTestBufSize, 0,  
                            (struct sockaddr *)&fromAddr, &fromLen)) < 0) 
        { 
            printf ("recvfrom UDP socet error:\n");  
            break;
        }
        else        
        {
            if(vxTestMode != 0)
            {
                printf("\t %d bytes received from: IP=%s, port=%d\n", 
                        recvLen, inet_ntoa(fromAddr.sin_addr), ntohs(fromAddr.sin_port));
                if(vxTestMode == 2)
                {
                    if (recvLen > 0)
                    {
                        mvDebugMemDump(recvBuf, recvLen, 1);
                    }
                }
            }
        }
        vxTestRcvSize += recvLen;
        vxTestRateSize += recvLen;
    }
    printf("Total: %ld bytes received on UDP socket from: IP=%s, port=%d\n", 
           vxTestRcvSize, inet_ntoa(fromAddr.sin_addr), ntohs(fromAddr.sin_port));

    if(vxTestMcastAddr[0] != '\0')
    {
        /* set the socket option to leave the MULTICAST group */ 
        if( setsockopt(vxTestRcvSock, IPPROTO_IP, IP_DROP_MEMBERSHIP,  
                        (char *)&ipMreq,  sizeof (ipMreq)) < 0)  
        {
            printf ("setsockopt IP_DROP_MEMBERSHIP error:\n");  
        }
    }

cleanUp: 
    if (vxTestRcvSock > 0) 
    {
        close(vxTestRcvSock); 
        vxTestRcvSock = 0;
    }

    if(recvBuf != NULL)
    {
        free (recvBuf); 
        recvBuf = NULL;
    }

    if(vxTestWd != NULL)
    {
        wdDelete(vxTestWd);
        vxTestWd = NULL;
    }
    vxTestTaskId = 0;
	return 0;
}

unsigned __TASKCONV vxTestTcpRcvTask(void* args)
{
    struct sockaddr_in	serverAddr; /* server's address */
    struct sockaddr_in  clientAddr; /* client's address */
    int	                addrLen, numRead;
    char                *recvBuf = NULL;

    vxTestRcvSock = vxTestListenSock = 0;

    if(vxTestRcvPort == 0)
        vxTestRcvPort = 7000;

    if(vxTestSockBufSize == 0)
        vxTestSockBufSize = 16*1024;

    if(vxTestBufSize == 0)
        vxTestBufSize = 2000;

    /* allocate buffer to receive message */
    recvBuf = malloc(vxTestBufSize);
    if(recvBuf == NULL)
    {
        printf("Can't allocate %d bytes for RCV buffer\n", vxTestBufSize);  
        vxTestTaskId = 0;
        return 1;
    }

    memset((char*)&serverAddr, 0, sizeof(serverAddr) ); 
    memset((char*)&clientAddr, 0, sizeof(clientAddr) ); 

    vxTestListenSock = socket (AF_INET, SOCK_STREAM, 0);
    if(vxTestListenSock < 0)  
    { 
        printf ("vxTest: Can't open TCP socket\n");  
        goto cleanUp; 
    } 

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port	  = htons (vxTestRcvPort);

    /* bind a port number to the socket */ 
    if( bind(vxTestListenSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) 
    { 
        printf ("vxTest: Can't bind TCP socket\n");  
        goto cleanUp; 
    } 
    if (listen (vxTestListenSock, 2) < 0)
    {
    	printf("vxTest: listen on TCP socket is failed\n");
        goto cleanUp; 
    }
    addrLen = sizeof (clientAddr);
    printf("Wait for TCP connection on %d port\n", vxTestRcvPort);
    vxTestRcvSock = accept (vxTestListenSock, (struct sockaddr *) &clientAddr, &addrLen);
    if (vxTestRcvSock < 0)
    {
        printf ("vxTest: accept on TCP socket failed\n");
        goto cleanUp; 
    }
    printf("TCP connection with IP=%s, port=%d established\n", 
                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    /* maximum size of socket-level receive buffer */
    if (setsockopt (vxTestRcvSock, SOL_SOCKET, SO_RCVBUF,
                    (char *)&vxTestSockBufSize, sizeof (vxTestSockBufSize)) < 0)
    {
	    printf("setsockopt SO_RCVBUF for TCP socket failed\n");
        goto cleanUp; 
    }

    vxTestWd = wdCreate();
    if(vxTestWd == NULL)
    {
        printf ("Can't create watchdog\n");  
        goto cleanUp; 
    }
    
    /* Start watchdog after a minute*/
    wdStart(vxTestWd, sysClkRateGet () * vxTestWdTime, (FUNCPTR)vxTestPrintRate, 0);

    if(vxTestIsForever)
        printf("Waiting on TCP socket to receive data FOREVER\n");
    else
        printf("Waiting on TCP socket to receive %d bytes of data\n", (int)vxTestTotalSize);

	vxTestRcvSize = 0;
    vxTestRateSize = 0;
    while( (vxTestIsForever) || (vxTestRcvSize < vxTestTotalSize) )
    {
	    numRead = 0;
	    if ((numRead = recv(vxTestRcvSock, recvBuf, vxTestBufSize, 0)) <= 0)
	    {
	        printf("Read on TCP socket failed\n");
	        break;
	    }
        else
        {
            if(vxTestMode != 0)
            {
                printf("\t %d bytes received from IP=%s, port=%d\n", 
                        numRead, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port)); 
                if(vxTestMode == 2)
                {
                    if (numRead > 0)
                    {
                        mvDebugMemDump(recvBuf, numRead, 1);
                    }
                }
            }
        }
	    vxTestRcvSize += numRead;      
        vxTestRateSize += numRead;
    }
    printf("Total: %ld bytes received on TCP socket\n", vxTestRcvSize);

cleanUp: 
    if (vxTestRcvSock > 0) 
    {
        close (vxTestRcvSock); 
        vxTestRcvSock = 0;
    }

    if (vxTestListenSock > 0) 
    {
        close (vxTestListenSock); 
        vxTestListenSock = 0;
    }

    if((recvBuf != NULL)) 
    {
        free (recvBuf); 
        recvBuf = NULL;
    }

    if(vxTestWd != NULL)
    {
        wdDelete(vxTestWd);
        vxTestWd = NULL;
    }
    vxTestTaskId = 0;
	return 0;
}

void        vxTestTcpRcv(unsigned short rcvPort, int totalSize,
                         int mode, int bufSize,  int socketBufSize)
{
	long	rc;

    if(vxTestTaskId != 0)
    {
        mvOsPrintf("TEST_T task already created\n");
        return;
    }
    vxTestBufSize = bufSize;
    vxTestTotalSize = totalSize;
    vxTestRcvPort = rcvPort;
    vxTestSockBufSize = socketBufSize;
    vxTestMode = mode;

    if(totalSize == 0)
        vxTestIsForever = MV_TRUE;
    else
        vxTestIsForever = MV_FALSE;

	rc = mvOsTaskCreate("TEST_T", 10, 4*1024, vxTestTcpRcvTask, NULL, &vxTestTaskId);
	if (rc != MV_OK) 
	{
		mvOsPrintf("Can't create TEST_T task, rc = %ld\n", rc);
	}
}


void        vxTestUdpRcv(unsigned short rcvPort, int totalSize, int mode, 
                         int bufSize, char* rcvAddr, char* mcastAddr)
{
	long	rc;

    vxTestBufSize = bufSize;
    vxTestTotalSize = totalSize;
    vxTestRcvPort = rcvPort;
    vxTestMode = mode;

    if(vxTestTaskId != 0)
    {
        mvOsPrintf("TEST_T task already created\n");
        return;
    }

    if(totalSize == 0)
        vxTestIsForever = MV_TRUE;
    else
        vxTestIsForever = MV_FALSE;

    if(rcvAddr != NULL)
        strcpy(vxTestRcvAddr, rcvAddr);
    else
        vxTestRcvAddr[0] = '\0';
    if(mcastAddr != NULL)
        strcpy(vxTestMcastAddr, mcastAddr);
    else
        vxTestMcastAddr[0] = '\0';

    rc = mvOsTaskCreate("TEST_T", 10, 4*1024, vxTestUdpRcvTask, NULL, &vxTestTaskId);
	if (rc != MV_OK) 
	{
		mvOsPrintf("Can't create TEST_T task, rc = %ld\n", rc);
	}
}

void    vxTestStopRcv(void)
{
    volatile int tmpSock;

    tmpSock = vxTestRcvSock;
    vxTestRcvSock = 0;
    if (tmpSock > 0) 
    {
        close (tmpSock); 
        return;
    }

    tmpSock = vxTestListenSock;
    vxTestListenSock = 0;
    if (tmpSock > 0) 
    {
        close(tmpSock); 
    }
}

void	vxTestStopAll(void)
{
	STATUS   	status;
	int			i;

	for(i=0; i<3; i++)
	{
		status = vxTestStartDriver("mgi", i, MV_FALSE);
		if(status != OK)
			mvOsPrintf("Can't stop <mgi%d> interface\n", i);
	}
}
extern int  _pingTxLen;
STATUS ping(char *  host,       /* host to ping */
            int     numPackets, /* number of packets to receive */
            ulong_t     options     /* option flags */
           );


int  vxPing(char* host, int size, int num, int prio)
{
    _pingTxLen = size;

    return taskSpawn ("tPing", prio, 0, 16*1024, (FUNCPTR)ping,
                      (int)host, num, 4, 0, 0, 0, 0, 0, 0, 0);

}

#ifdef MV_IPM_ENABLE

void prio0Isr()
{
    volatile unsigned int cnt = 0;
    mvBoardDebug7Seg(0);

    strcat(dumpLog,"Start  priority 0 ISR\n");

     /* clean TWSI interrupt */
     MV_REG_BIT_RESET(TWSI_CONTROL_REG,TWSI_CONTROL_INT_ENA); 
   
    /* Generate priority 1 interrupt */
    MV_REG_WRITE(CNTMR_TC_REG(1),BIT0);
    MV_REG_BIT_SET(CNTMR_MASK_REG,BIT1);
    MV_REG_BIT_RESET(CNTMR_CTRL_REG,CNTMR_CTRL_MODE(1)); 
    MV_REG_BIT_SET(CNTMR_CTRL_REG,CNTMR_CTRL_EN(1)); 
                
    while(cnt++ < 0x100000);                
                               
     strcat(dumpLog," Finish priority 0 ISR\n");
     testNum = 9;

}

void prio1Isr()
{
    volatile unsigned int cnt1 = 0;

    mvBoardDebug7Seg(1);

    if(testNum == 1)
    {
      strcat(dumpLog," Start  priority 1 ISR\n");
      
      /* Generate priority 2 interrupt */  
      MV_REG_BYTE_WRITE(0x12104,0xf);
      MV_REG_BYTE_WRITE(0x12100,3);  
    }
    else if(testNum == 2)
    {
        strcat(dumpLog2," Start  priority 1 ISR test 2\n");
    }
    else if(testNum == 3)
    {
        strcat(dumpLog3,"Start  priority 1 ISR test 3\n");
         /* Generate priority 1 interrupt */
        MV_REG_WRITE(CNTMR_TC_REG(3),BIT0);
        MV_REG_BIT_SET(CNTMR_MASK_REG,BIT3);
        MV_REG_BIT_RESET(CNTMR_CTRL_REG,CNTMR_CTRL_MODE(3)); 
        MV_REG_BIT_SET(CNTMR_CTRL_REG,CNTMR_CTRL_EN(3));

    }
    
    while(cnt1++ < 0x10000);
  
    MV_REG_BIT_RESET(CNTMR_CAUSE_REG,BIT1);
    
    if(testNum == 1)
    {
       strcat(dumpLog," Finish priority 1 ISR\n");
    }
    else if(testNum == 2)
    {
      strcat(dumpLog2," Finish priority 1 ISR test 2\n");
      testNum = 8;
    }
    else if(testNum == 3)
    {
      strcat(dumpLog3," Finish priority 1 ISR test 3\n");
    }
  
}

void prio2Isr()
{
    volatile unsigned int cnt2 = 0;
     
    mvBoardDebug7Seg(2);
    if(testNum == 4)
    {
        strcat(dumpLog4,"Start  priority 2 ISR test 4\n");
        /* Generate priority 1 interrupt */
        MV_REG_WRITE(CNTMR_TC_REG(3),BIT0);
        MV_REG_BIT_SET(CNTMR_MASK_REG,BIT3);
        MV_REG_BIT_RESET(CNTMR_CTRL_REG,CNTMR_CTRL_MODE(3)); 
        MV_REG_BIT_SET(CNTMR_CTRL_REG,CNTMR_CTRL_EN(3));  
    }
    else
    {
        strcat(dumpLog," Start  priority 2 ISR\n");
        /* Generate priority 3 interrupt */
        MV_REG_WRITE(IDMA_BASE_ADDR_REG(0),0);
        MV_REG_BIT_SET(IDMA_CTRL_LOW_REG(0), ICCLR_CHAN_ENABLE);
    }
    while(cnt2++ < 0x10000);
    
    MV_REG_BYTE_WRITE(0x12104,0);
    if(testNum == 4)  
        strcat(dumpLog4," Finish priority 2 ISR test 4\n");
     else
        strcat(dumpLog," Finish priority 2 ISR\n");
  
}

void prio3Isr()
{
   
    strcat(dumpLog," Start  priority 3 ISR\n");
    mvBoardDebug7Seg(3);

    MV_REG_WRITE(IDMA_CAUSE_REG, 0);
    MV_REG_BIT_RESET(IDMA_CTRL_LOW_REG(0), ICCLR_CHAN_ENABLE);
     
    strcat(dumpLog," Finish priority 3 ISR\n");
}

void prio1Isr2()
{
    volatile unsigned int cnt4 = 0;

    if(testNum == 2)
       strcat(dumpLog2,"Start  priority 1 ISR 2 test 2\n");
    else if(testNum == 3)
       strcat(dumpLog3," Start  priority 1 ISR 2 test 3\n");
    else  if(testNum == 4)
       strcat(dumpLog4," Start  priority 1 ISR 2 test 4\n");
   
    mvBoardDebug7Seg(4);
     if(testNum == 2)
     {
     
     /* Generate priority 1 interrupt */
      MV_REG_WRITE(CNTMR_TC_REG(1),BIT0);
      MV_REG_BIT_SET(CNTMR_MASK_REG,BIT1);
      MV_REG_BIT_RESET(CNTMR_CTRL_REG,CNTMR_CTRL_MODE(1)); 
      MV_REG_BIT_SET(CNTMR_CTRL_REG,CNTMR_CTRL_EN(1));
     }

       while(cnt4++ < 0x10000);
     MV_REG_BIT_RESET(CNTMR_CAUSE_REG,BIT3);
     if(testNum == 2)
       strcat(dumpLog2," Finish priority 1 ISR 2 test 2\n");
     else if(testNum == 3)
     {
       strcat(dumpLog3," Finish priority 1 ISR 2 test 3\n");
       testNum = 7;
     }
     else if(testNum == 4)
       strcat(dumpLog4," Finish priority 1 ISR 2 test 4\n");

}


void ipmTest1()
{
     unsigned int temp;
     
     testNum = 1;
     mvBoardDebug7Seg(5);
     if(!isInit)
     {
        /* Hooking new ISRs */
        /*Priority 0 -  Connect TWSI handler in main cause register */
        intConnect(INT_VEC_TWSI ,prio0Isr, 0);
        intEnable(INT_LVL_TWSI);

        /*Priority 1 -  Connect TIMER1 handler in main cause register */
        mvIntDisconnect(INT_LVL_TIMER1);
        intConnect(INT_VEC_TIMER1 ,prio1Isr, 0);
        intEnable(INT_LVL_TIMER1);

        /*Priority 1 -  Connect TIMER3 handler in main cause register */
        intConnect(INT_VEC_TIMER3 ,prio1Isr2, 0);
        intEnable(INT_LVL_TIMER3);

        /*Priority 2 -  Connect UART handler in main cause register */
        mvIntDisconnect(INT_LVL_UART1);     
        intConnect(INT_VEC_UART1 ,prio2Isr, 0);
        intEnable(INT_LVL_UART1);

        /*Priority 3 -  Connect IDMA handler in main cause register */
        mvIntDisconnect(INT_LVL_DMA_ERR); 
        intConnect(INT_VEC_DMA_ERR ,prio3Isr, 0);
        intEnable(INT_LVL_DMA_ERR);
     }
    /* Generate priority 0 interrupt */
    MV_REG_BIT_RESET(TWSI_CONTROL_REG,TWSI_CONTROL_START_BIT);
    MV_REG_BIT_SET(TWSI_CONTROL_REG,TWSI_CONTROL_INT_ENA);
    temp = MV_REG_READ(TWSI_CONTROL_REG);
    MV_REG_WRITE(TWSI_CONTROL_REG, temp | TWSI_CONTROL_START_BIT);
    
    return;

}

void ipmTest2()
{
    testNum = 2;
    mvBoardDebug7Seg(6);
     /* Generate priority 1 interrupt */
    MV_REG_WRITE(CNTMR_TC_REG(3),BIT0);
    MV_REG_BIT_SET(CNTMR_MASK_REG,BIT3);
    MV_REG_BIT_RESET(CNTMR_CTRL_REG,CNTMR_CTRL_MODE(3)); 
    MV_REG_BIT_SET(CNTMR_CTRL_REG,CNTMR_CTRL_EN(3));

    return;

}

void ipmTest3()
{
    testNum = 3;
    mvBoardDebug7Seg(7);
     /* Generate priority 1 interrupt */
    MV_REG_WRITE(CNTMR_TC_REG(1),BIT0);
    MV_REG_BIT_SET(CNTMR_MASK_REG,BIT1);
    MV_REG_BIT_RESET(CNTMR_CTRL_REG,CNTMR_CTRL_MODE(1)); 
    MV_REG_BIT_SET(CNTMR_CTRL_REG,CNTMR_CTRL_EN(1));

    return;

}

void ipmTest4()
{
    testNum = 4;
    mvBoardDebug7Seg(8);
   /* Generate priority 2 interrupt */  
      MV_REG_BYTE_WRITE(0x12104,0xf);
      MV_REG_BYTE_WRITE(0x12100,3); 

    return;

}



void ipmTest()
{
    unsigned int stopInt;

    testNum = 1;

    memset(dumpLog,'\0',200);
    memset(dumpLog2,'\0',200);
    memset(dumpLog3,'\0',200);
    memset(dumpLog4,'\0',200);

      /* Shutdown GBe0 & UART0  */
    intDisable(INT_LVL_UART0);
    intDisable(INT_LVL_GBE0_RX);
      
    stopInt = 0;
    while(stopInt++ < 0x100000);
    
    /* TEST 1 */
     ipmTest1();
     while(testNum != 9);
    
    /* TEST 2 */
     ipmTest2();
     while(testNum != 8);

   /* TEST 3 */
     ipmTest3();
     while(testNum != 7);
    
     /* TEST 4 */
     ipmTest4();


    /* reenable UART0 */
    intEnable(INT_LVL_UART0);   

    /* print test results */
    mvOsPrintf("Starting ipmTest....\n\n");
    mvOsPrintf("Test 1 - Nesting 4 interrupts....\n");
    mvOsPrintf("Sequence Events Test 1:\n %s \n",dumpLog);
    mvOsPrintf("Test 2 - non subsequent interrupts(prio 1)....\n");
    mvOsPrintf("Sequence Events Test 2:\n %s \n",dumpLog2); 
    mvOsPrintf("Test 3 - subsequent interrupts(prio 1)....\n");
    mvOsPrintf("Sequence Events Test 3:\n %s \n",dumpLog3);
    mvOsPrintf("Test 4 - prio 2 then prio 1....\n");
    mvOsPrintf("Sequence Events Test 4:\n %s \n",dumpLog4); 

 
    mvOsPrintf("ipmTest....Done\n");

    if(!isInit)
        isInit = 1;

   
    /* reenable GBe0 */
    intEnable(INT_LVL_GBE0_RX);

}

#endif /* MV_IPM_ENABLE */


void ethPortPowerDownTest(int port)
{
    void* 	pCookie;
   STATUS	status;
   char*    devName = "mgi";


    pCookie = vxFindDevCookie(devName, port);
    if(pCookie == NULL)
    {
        mvOsPrintf("Device: %s Unit %d not found\n", devName, port);
        return;
    }
   
    status = muxDevStop(pCookie);

    if(status != OK)
    {
      mvOsPrintf("Can't stop Device %s%d: status=0x%x\n", devName, port, status);
      return;
    }
    
      
    mvEthPortPowerDown(port);

    return;

 }

void ethPortPowerUpTest(int port)
{
    void* 	pCookie;
    STATUS	status;
    char*   devName = "mgi";

                              
    mvEthPortPowerUp(port);
   
    pCookie = vxFindDevCookie(devName, port);
    if(pCookie == NULL)
    {
        mvOsPrintf("Device: %s Unit %d not found\n", devName, port);
        return;
    }
   
    status = muxDevStart(pCookie);
    
    if(status != OK)
      mvOsPrintf("Can't start Device %s%d: status=0x%x\n", devName, port, status);
                 
    return;
}



