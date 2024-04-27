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
* usrAppInit.c
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

/* includes */
#include "ipProto.h"
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#include <net/if.h>
#else
#include "ifLib.h"
#endif
#include "taskLib.h"
#include "taskHookLib.h"
#include "errnoLib.h"
#include "stdio.h"
#include "end.h"
#include "muxLib.h"
#include "sysLib.h"
#include "bootLib.h"
#include "config.h"

#include "mvCtrlEnvLib.h"
#include "mvCpu.h"
#include "vxPciIntCtrl.h"

#ifdef INCLUDE_FEI_END
#include "drv/End/fei82557End.h"
#endif

#include "version.h"

/* defines  */

/* typedefs */
typedef struct
{
    char*   ifAddr;
    char*   ifBcastAddr;
    MV_U32  ifSubnetMask;
} MV_VXW_IP_ADDR;

/* locals   */
#ifdef INCLUDE_MGI_END
#if USE_MV_VXW_IP_ADDR
static MV_VXW_IP_ADDR  mgiIfAddr[] =
{
    { "192.168.0.1", "192.168.0.255", 0xFFFFFF00 },
    { "192.168.1.1", "192.168.1.255", 0xFFFFFF00 },
    { "192.168.2.1", "192.168.2.255", 0xFFFFFF00 },
    { "192.168.3.1", "192.168.3.255", 0xFFFFFF00 }
};
#endif
#endif /* INCLUDE_MGI_END */

#ifdef INCLUDE_GEI_END
static MV_VXW_IP_ADDR  geiIfAddr[GEI_MAX_UNITS] =
{
    { "192.168.10.1", "192.168.10.255", 0xFFFFFF00 },
    { "192.168.11.1", "192.168.11.255", 0xFFFFFF00 },
    { "192.168.12.1", "192.168.12.255", 0xFFFFFF00 },
    { "192.168.13.1", "192.168.13.255", 0xFFFFFF00 }
};
#endif /* INCLUDE_GEI_END */

#ifdef INCLUDE_SYSKONNECT
static MV_VXW_IP_ADDR  sgiIfAddr[SGI_MAX_UNITS] =
{
    { "192.168.20.1", "192.168.20.255", 0xFFFFFF00 },
    { "192.168.21.1", "192.168.21.255", 0xFFFFFF00 },
    { "192.168.22.1", "192.168.22.255", 0xFFFFFF00 },
    { "192.168.22.1", "192.168.23.255", 0xFFFFFF00 }
};
#endif /* INCLUDE_SYSKONNECT */

#ifdef INCLUDE_FEI_END
static MV_VXW_IP_ADDR  feiIfAddr[GEI_MAX_UNITS] =
{
    { "192.168.30.1", "192.168.30.255", 0xFFFFFF00 },
    { "192.168.31.1", "192.168.31.255", 0xFFFFFF00 },
    { "192.168.32.1", "192.168.32.255", 0xFFFFFF00 },
    { "192.168.33.1", "192.168.33.255", 0xFFFFFF00 }
};
#endif /* INCLUDE_GEI_END */

#ifdef INCLUDE_SWITCH_END
static MV_VXW_IP_ADDR  switchIfAddr[] =
{
    { "192.168.40.1", "192.168.40.255", 0xFFFFFF00 },
};
#endif /* INCLUDE_SWITCH_END */




/* Externals */
void changeProtocolInputQMax(int ipQNewSize, int arpQNewSize);
void changeIfSendQMax(char *name, int len);
#ifdef MV_INCLUDE_CESA

extern void ipsecDemoInclude();
extern void cipherTestsInclude();
#endif


#ifdef INCLUDE_BRIDGING

#ifdef INCLUDE_MGI_END
void    mgiSetDstEndObj(int unit, END_OBJ* pEndObj);
#endif /* INCLUDE_MGI_END */

#ifdef INCLUDE_GEI_END
void    geiSetDstEndObj(int unit, END_OBJ* pEndObj);
#endif /* INCLUDE_GEI_END */

#ifdef INCLUDE_SGI_END
void    skSetDstEndObj(int unit, END_OBJ* pEndObj);
#endif /* INCLUDE_SGI_END */

/* if ifDestName == NULL, use Routing */
void    userIfCfg(char* ifName, int ifUnit,  char* ifDstName, int ifDstUnit)
{
    END_OBJ*    pDstEnd;

    if(ifDstName == NULL)
    {
        /* Routing */
        pDstEnd = NULL;
    }
    else
    {
        pDstEnd = endFindByName(ifDstName, ifDstUnit);
        if(pDstEnd == NULL)
        {
            printf("Destination Interface is not found\n");
            return;
        }
    }

#ifdef INCLUDE_MGI_END
    if( strcmp(ifName, "mgi") == 0)
    {
        mgiSetDstEndObj(ifUnit, pDstEnd);
        return;
    }
#endif /* INCLUDE_MGI_END */

#ifdef INCLUDE_GEI_END
    if(strcmp(ifName, "gei") == 0)
    {
        geiSetDstEndObj(ifUnit, pDstEnd);
        return;
    }
#endif /* INCLUDE_GEI_END */

#ifdef INCLUDE_SGI_END
    if(strcmp(ifName, "sgi") == 0)
    {
        skSetDstEndObj(ifUnit, pDstEnd);
        return;
    }
#endif /* INCLUDE_SGI_END */

    printf("Unexpected ifName\n");
    return;
}
#endif /* INCLUDE_BRIDGING */

void    userCfgShow(void)
{
    char    chipName[50], cpuName[50], boardName[50];

    mvOsPrintf("\nSystem Configuration\n\n");


    mvOsPrintf("BSP Ver: %s%s   Creation date: %s \n",BSP_VERSION,BSP_REV, creationDate);
        mvCtrlNameGet(chipName);
    mvOsPrintf("CHIP   : %s\n", chipName);

    mvCpuNameGet(cpuName);
    mvOsPrintf("CPU    : %s\n", cpuName);

    if(mvBoardNameGet(boardName) == MV_OK)
        mvOsPrintf("BOARD  : %s (0x%x)\n", boardName, mvBoardIdGet());

    mvOsPrintf("CLOCKs : tClk = %4d MHz, sysClk = %4d MHz\n",
                mvBoardTclkGet()/1000000, mvBoardSysClkGet()/1000000);
#if defined(MV646xx)
    mvOsPrintf("         mClk = %4d MHz  pClk   = %4d MHz\n\n",
                           mvBoardMclkGet()/1000000,mvCpuPclkGet()/1000000);
#else
    mvOsPrintf("         pClk = %4d MHz\n\n",mvCpuPclkGet()/1000000);
#endif



#if defined(INCLUDE_MGI_END)

        if (sysBootParams.flags & SYSFLG_VENDOR_0)
        {
                mvOsPrintf("ETH_JUMBO_SUPPORT   = %s\n", "Enabled");
        }
        else
        {
                mvOsPrintf("ETH_JUMBO_SUPPORT   = %s\n", "Disabled");
        }

#if defined(INCLUDE_BRIDGING)
    mvOsPrintf("BRIDGING_SUPPORT    = %s\n", "Enabled");
#else
    mvOsPrintf("BRIDGING_SUPPORT    = %s\n", "Disabled");
#endif /* INCLUDE_BRIDGING */

#if defined(INCLUDE_MULTI_QUEUE)
    mvOsPrintf("MULTI_Q_SUPPORT     = %s\n", "Enabled");
#else
    mvOsPrintf("MULTI_Q_SUPPORT     = %s\n", "Disabled");
#endif /* INCLUDE_MULTI_QUEUE */

#endif /* defined(INCLUDE_MGI_END) */

#if defined(INCLUDE_NAT)
    mvOsPrintf("NAT_SUPPORT         = %s\n", "Enabled");
#else
    mvOsPrintf("NAT_SUPPORT         = %s\n", "Disabled");
#endif /* INCLUDE_NAT */

#if defined(INCLUDE_VFP)
    mvOsPrintf("VFP_SUPPORT         = %s\n", "Enabled");
#else
    mvOsPrintf("VFP_SUPPORT         = %s\n", "Disabled");
#endif /* INCLUDE_NAT */

}

/******************************************************************************
*
* usrAppInit - initialize the users application
*/
#ifndef PSS_MODE
#if (_WRS_VXWORKS_MAJOR == 6)
void mvUserAppInit(void)
#elif defined (PSS_MODE)
void mvUserAppInit(void)
#else
void userAppInit(void)
#endif
{
#ifdef INCLUDE_SWITCH_END
    int             i;
#endif

#if defined(INCLUDE_GEI_END) || defined(INCLUDE_MGI_END) || defined(MV_BSP_BOOT_PRINTS_ENABLE)
    char            ifName[10];
#endif

#if defined(INCLUDE_MGI_END) || defined(MV_BSP_BOOT_PRINTS_ENABLE)
    STATUS          status;
#endif
#if defined(INCLUDE_MGI_END) && defined(MV_BSP_BOOT_PRINTS_ENABLE)
    int             mgiIfNum;
#endif
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
    u_int if_index = 0;
#else
    #if defined(INCLUDE_MGI_END) || defined(MV_BSP_BOOT_PRINTS_ENABLE) || defined(INCLUDE_SWITCH_END)
    struct ifnet*   ifp;
    #endif
#endif
    taskDelay(20);

#if defined(INCLUDE_MGI_END) && defined(MV_BSP_BOOT_PRINTS_ENABLE)
    mgiIfNum = mvCtrlEthMaxPortGet();

    for(i=0; i<mgiIfNum; i++)
    {
        if (MV_FALSE == mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, i))
                continue;
        sprintf(ifName, "mgi%d", i);
        if( muxDevExists("mgi", i) )
        {
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
                    if_index = if_nametoindex(ifName);
                        printf("\n\nifName %s if_index %d\n\n",ifName,if_index);

            if(if_index != 0)
#else
            ifp = ifunit(ifName);
                        if (ifp == NULL)
#endif
            {
                status = ipAttach(i, "mgi");
                if(status == OK)
                {
                    status = ifAddrAdd(ifName, mgiIfAddr[i].ifAddr,
                                               mgiIfAddr[i].ifBcastAddr,
                                               mgiIfAddr[i].ifSubnetMask);
                    if(status == ERROR)
                        printf("Can't Set IP address to <%s> interface\n", ifName);
                }
                else
                    printf("Can't Attach <%s> interface\n", ifName);
            }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#else
            changeIfSendQMax(ifName, 256);
#endif
        }
    }
#endif /* INCLUDE_MGI_END */

#ifdef INCLUDE_GEI_END
    for(i=0; i<GEI_MAX_UNITS; i++)
    {
        sprintf(ifName, "gei%d", i);
        if( muxDevExists("gei", i) )
        {
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
                    if_index = if_nametoindex(ifName);
            if(if_index != 0)
#else
            ifp = ifunit(ifName);
            if (ifp == NULL)
#endif
            {
                status = ipAttach(i, "gei");
                if(status == OK)
                {
                    status = ifAddrAdd(ifName, geiIfAddr[i].ifAddr,
                                               geiIfAddr[i].ifBcastAddr,
                                               geiIfAddr[i].ifSubnetMask);
                    if(status == ERROR)
                        printf("Can't Set IP address to <%s> interface\n", ifName);
                }
                else
                    printf("Can't Attach <%s> interface\n", ifName);
            }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#else
            changeIfSendQMax(ifName, 256);
#endif
        }
    }
#endif /* INCLUDE_GEI_END */

#if defined(INCLUDE_SYSKONNECT)
    for(i=0; i<SGI_MAX_UNITS; i++)
    {
        sprintf(ifName, "sgi%d", i);
        if( muxDevExists("sgi", i) )
        {
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
                    if_index = if_nametoindex(ifName);
            if(if_index != 0)
#else
            ifp = ifunit(ifName);
            if (ifp == NULL)
#endif
            {
                status = ipAttach(i, "sgi");
                if(status == OK)
                {
                    status = ifAddrAdd(ifName, sgiIfAddr[i].ifAddr,
                                               sgiIfAddr[i].ifBcastAddr,
                                               sgiIfAddr[i].ifSubnetMask);
                    if(status == ERROR)
                        printf("Can't Set IP address to <%s> interface\n", ifName);
                }
                else
                    printf("Can't Attach <%s> interface\n", ifName);
            }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#else
            changeIfSendQMax(ifName, 256);
#endif
        }
    }
#endif /* INCLUDE_SYSKONNECT */

#ifdef INCLUDE_SWITCH_END
    for(i=0; i<1; i++)
    {
        sprintf(ifName, "switch%d", i);
        if( muxDevExists("switch", i) )
        {
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
                    if_index = if_nametoindex(ifName);
            if(if_index != 0)
#else
            ifp = ifunit(ifName);
            if (ifp == NULL)
#endif
            {
                status = ipAttach(i, "switch");
                if(status == OK)
                {
                    status = ifAddrAdd(ifName, switchIfAddr[i].ifAddr,
                                               switchIfAddr[i].ifBcastAddr,
                                               switchIfAddr[i].ifSubnetMask);
                    if(status == ERROR)
                        printf("Can't Set IP address to <%s> interface\n", ifName);
                }
                else
                    printf("Can't Attach <%s> interface\n", ifName);
            }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#else
            changeIfSendQMax(ifName, 256);
#endif
        }
    }
#endif /* INCLUDE_SWITCH_END */


#ifdef INCLUDE_FEI_END

    for(i=0; i<FEI_MAX_UNITS; i++)
    {
        sprintf(ifName, "fei%d", i);
        if( muxDevExists("fei", i) )
        {
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
                    if_index = if_nametoindex(ifName);
            if(if_index != 0)
#else
            ifp = ifunit(ifName);
            if (ifp == NULL)
#endif
            {
                status = ipAttach(i, "fei");
                if(status == OK)
                {
                    status = ifAddrAdd(ifName, feiIfAddr[i].ifAddr,
                                               feiIfAddr[i].ifBcastAddr,
                                               feiIfAddr[i].ifSubnetMask);
                    if(status == ERROR)
                        printf("Can't Set IP address to <%s> interface\n", ifName);
                }
                else
                    printf("Can't Attach <%s> interface\n", ifName);
            }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#else
            changeIfSendQMax(ifName, 256);
#endif
        }
    }
#endif /* INCLUDE_GEI_END */

#ifdef INCLUDE_NETWORK
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 4)
#else
    changeProtocolInputQMax(512, 512);
#endif
#endif

#if defined(INCLUDE_NAT)
#if (_WRS_VXWORKS_MAJOR == 6)
/*
   TODO: porting the new vxWorks API

   */
#else
    natcfg();
    if( natInit() == ERROR )
        printf("NAT initialization failed!\n");
    /* NAT FTP ALG: */
    if( natFtpInit(21) != OK )
        printf("NAT FTP ALG initialization failed!\n");
#endif /* (_WRS_VXWORKS_MAJOR == 6)   */
#endif /* INCLUDE_NAT */
#ifdef MV_BSP_BOOT_PRINTS_ENABLE
    userCfgShow();
#endif

#ifdef MV_INCLUDE_CESA

    ipsecDemoInclude();

    cipherTestsInclude();

#endif

#ifdef MV_BSP_BOOT_PRINTS_ENABLE
    mvChipFeaturesPrint();
    switchGenPrint();
    mvGndPrint();

    taskDelay(2);
#endif

    return;
}
#endif

