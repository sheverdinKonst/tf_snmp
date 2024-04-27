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
 *
 *  THIS CODE IS NOT SUPPORTED CODE.  IT IS PROVIDED ON AN AS-IS BASIS.
 *
 *******************************************************************************/
 
#include "vxWorks.h"
#include "taskLib.h"
#include "ifLib.h"
#include "ipProto.h" 
#include "etherLib.h"
#include "stdio.h"
#include "net/if.h"
#include "net/unixLib.h"
#include "netBufLib.h"
#include "end.h"
#include "muxLib.h"
#include "netShow.h"
#include "stdio.h"
#include "private/muxLibP.h"



IMPORT struct ifqueue ipintrq;
IMPORT struct ifqueue arpintrq;

IMPORT void netPoolShow (NET_POOL_ID pNetPool);

extern int          ipMaxUnits;

/******************************************************************************
*
* changeProtocolInputQMax - Change IP and ARP input queue maximum length
*                       Default configuration sets these values to 50 packets each.
* 
*/
void changeProtocolInputQMax
    (
    int ipQNewSize,   /* Default is 50 */
    int arpQNewSize   /* Default is 50 */
    )
    {
    int s;
   
    printf("IP input maximum queue length = %d\n", ipintrq.ifq_maxlen);
    printf("Arp input maximum queue length = %d\n", arpintrq.ifq_maxlen);
    
    s = splnet();
    arpintrq.ifq_maxlen = arpQNewSize;
    ipintrq.ifq_maxlen = ipQNewSize;
    splx(s);

    printf("New IP input maximum queue length = %d\n", ipintrq.ifq_maxlen);
    printf("New Arp input maximum queue length = %d\n", arpintrq.ifq_maxlen);

    return;

    }


void protocolQValuesShow (void)
    {
   
    printf("IP receive queue max size = %d\n", ipintrq.ifq_maxlen);
    printf("IP receive queue drops = %d\n\n", ipintrq.ifq_drops);
    printf("ARP receive queue max size = %d\n", arpintrq.ifq_maxlen);
    printf("ARP receive queue drops = %d\n", arpintrq.ifq_drops);


    return;

    }


/******************************************************************************
*
* ifQValuesShow - Show number of times packets were dropped because interface send
*                was full.
*
* name
*     Expects the interface name plus the unit number: dc0, fei0 
* 
*/

void ifQValuesShow
    (
    char *name
    )
    {

    struct ifnet *ifp;

    ifp = ifunit(name);

    if (ifp == NULL)
        {
        printf("Could not find %s interface\n", name);
        return;
        }
    printf("%s drops = %d queue length = %d max_len = %d \n", name, ifp->if_snd.ifq_drops,
            ifp->if_snd.ifq_len, ifp->if_snd.ifq_maxlen);
    return;
    }


/******************************************************************************
*
* changeSendQueueLength - Change interface send queue length.
*
* name
*     Expects the interface name plus the unit number: dc0, fei0 
*
* length
*     New length.   Default configuration sets this value to 50.
*/

void changeIfSendQMax
    (
    char *name,
    int len
    )
    {

    struct ifnet *ifp;
    int s;

    ifp = ifunit(name);

    if (ifp == NULL)
        {
        printf("Could not find %s interface\n", name);
        return;
        }

    printf("%s previous queue length was %d\n", name, ifp->if_snd.ifq_maxlen);
    s = splnet();
    ifp->if_snd.ifq_maxlen = len;
    splx(s);
    printf("Queue length set at: %d\n", ifp->if_snd.ifq_maxlen);
    return;
    }



/******************************************************************************
*
* endMibShow - Show the contents of the MIB2 table for end drivers.
*
* Output errors can be errors while transmitting or dropped packets due to
* driver's out of buffers error condition.  Input errors can be packets received
* with errors or packets dropped due to driver's out of buffers error condition.
* See endPoolShow below. 
* 
*/
void endMibShow
    (
    char * devName,
    int unit
    )
    {

    END_OBJ * pEnd;

    if ((pEnd = endFindByName (devName, unit)) == NULL)
        {
        printf ("Could not find device %s\n", devName);
        return;
        }

    printf("\tMib table statistics for %s%d:\n\n", devName, unit);
    printf("Output Octets               = %lu\n", pEnd->mib2Tbl.ifOutOctets);
    printf("Output Errors               = %lu\n", pEnd->mib2Tbl.ifOutErrors);
    printf("Output Discards             = %lu\n", pEnd->mib2Tbl.ifOutDiscards);
    printf("Output Unicast Packets      = %lu\n", pEnd->mib2Tbl.ifOutUcastPkts);
    printf("Output Non-Unicast Packets  = %lu\n", pEnd->mib2Tbl.ifOutNUcastPkts);
    printf("\n");
    printf("Input Octets                = %lu\n", pEnd->mib2Tbl.ifInOctets);
    printf("Input Errors                = %lu\n", pEnd->mib2Tbl.ifInErrors);
    printf("Input Discards              = %lu\n", pEnd->mib2Tbl.ifInDiscards);
    printf("Input Unicast Packets       = %lu\n", pEnd->mib2Tbl.ifInUcastPkts);
    printf("Input Non-Unicast Packets   = %lu\n", pEnd->mib2Tbl.ifInNUcastPkts);
    printf("Input Unknown protocols     = %lu\n", pEnd->mib2Tbl.ifInUnknownProtos);

    return;
}

/******************************************************************************
*
* endPoolShow - Show the status of the private memory pool for end drivers.
*
* The total number of clusters is usually equal to the sum of the number of transmit,
* receive and loan buffer descriptors that a driver is initialized with.
* See the External Interface section for the driver's library description in the
* vxWorks Reference manual (i.e dec21x40End), the driver's header file, and configNet.h
* for the default values.
*
* If the number of free buffers is 0, the device will be unable to either transmit 
* (Mib 2 output errors will increase)  or receive packets (The Mib table input errors
*  statistics will increase).
*
* Why would the free pool drop to 0?  Most common reason is task not reading a socket.
* Drivers lend buffers to the stack which are not freed until the socket is read.
*
*/

void endPoolShow
    (
    char * devName,  /* The inteface name: "dc", "ln" ...*/
    int unit         /* the unit number: usually 0 */
    )
    {
    END_OBJ * pEnd;

    if ((pEnd = endFindByName (devName, unit)) != NULL)
        netPoolShow (pEnd->pNetPool);
    else
        printf ("Could not find device %s\n", devName);
    return;
    }


void* vxFindDevCookie(char* pDevName, int unit)
{
    END_OBJ * pEnd;
    IP_DRV_CTRL * pDrvCtrl;
    int i;

    pEnd = endFindByName(pDevName, unit);
    if (pEnd == NULL) 
    {
        printf("Device: %s Unit %d not found\n",pDevName,unit);
        return NULL;
    }

    for (i = 0; i < ipMaxUnits; i++) 
    {
        pDrvCtrl = &ipDrvCtrl[i];

        if (PCOOKIE_TO_ENDOBJ (pDrvCtrl->pIpCookie) == pEnd)
            return(pDrvCtrl->pIpCookie);
    }

    printf ("Could not find pCookie for Device %s Unit %d\n", pDevName, unit);
    return NULL;
}

