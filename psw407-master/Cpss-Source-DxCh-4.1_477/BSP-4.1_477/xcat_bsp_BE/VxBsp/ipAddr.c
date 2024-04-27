
#include <vxWorks.h>
#include <ioLib.h>
#include <stdio.h>
#include <ioctl.h>
#include <errnoLib.h>
#include <m2Lib.h>

#include <net/mbuf.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <inetLib.h>
#include <hostLib.h>
#include <netinet/in_var.h>
#include <sockLib.h>

#include "netinet/in.h"
#include "net/if.h"
#include "netinet/if_ether.h"
#include "sys/ioctl.h"
#include "ioLib.h"
#include "inetLib.h"
#include "string.h"
#include "netinet/in_var.h"
#include "errnoLib.h"
#include "sockLib.h"
#include "hostLib.h"

#include "unistd.h"

#include "m2Lib.h"
#include "arpLib.h"

#include "netLib.h"



LOCAL STATUS ifIoctlCall
	(
	int           code,         /* ioctl code */
	struct ifreq *ifrp          /* pointer to the interface ioctl request */
	)
	{
	int sock;
	int status;

	if ((sock = socket (AF_INET, SOCK_RAW, 0)) < 0)
	return (ERROR);

	status = ioctl (sock, code, (int)ifrp);
	(void)close (sock);

	if (status != 0)
	{
	if (status != ERROR)	/* iosIoctl() can return ERROR */
		(void)errnoSet (status);

	return (ERROR);
	}

	return (OK);
	}

STATUS ifAddrAdd
	(
	char *interfaceName,     /* name of interface to configure */
	char *interfaceAddress,  /* Internet address to assign to interface */
	char *broadcastAddress,  /* broadcast address to assign to interface */
	int   subnetMask         /* subnetMask */
	)
	{
	struct ifaliasreq   ifa;
	struct sockaddr_in *pSin_iaddr = (struct sockaddr_in *)&ifa.ifra_addr;
	struct sockaddr_in *pSin_baddr = (struct sockaddr_in *)&ifa.ifra_broadaddr;
	struct sockaddr_in *pSin_snmsk = (struct sockaddr_in *)&ifa.ifra_mask;

	bzero ((caddr_t) &ifa, sizeof (ifa));

	/* verify Internet address is in correct format */
	if ((pSin_iaddr->sin_addr.s_addr =
			 inet_addr (interfaceAddress)) == ERROR &&
		(pSin_iaddr->sin_addr.s_addr =
			 hostGetByName (interfaceAddress) == ERROR))
		{
		return (ERROR);
		}

	/* verify broadcast address is in correct format */
	if (broadcastAddress != NULL &&
		(pSin_baddr->sin_addr.s_addr =
			 inet_addr (broadcastAddress)) == ERROR &&
		(pSin_baddr->sin_addr.s_addr =
			 hostGetByName (broadcastAddress) == ERROR))
		{
		return (ERROR);
		}

	strncpy (ifa.ifra_name, interfaceName, sizeof (ifa.ifra_name));

	/* for interfaceAddress */
	ifa.ifra_addr.sa_len = sizeof (struct sockaddr_in);
	ifa.ifra_addr.sa_family = AF_INET;

	/* for broadcastAddress */
	if (broadcastAddress != NULL)
		{
		ifa.ifra_broadaddr.sa_len = sizeof (struct sockaddr_in);
		ifa.ifra_broadaddr.sa_family = AF_INET;
		}

	/* for subnetmask */
	if (subnetMask != 0)
	   {
	   ifa.ifra_mask.sa_len = sizeof (struct sockaddr_in);
	   ifa.ifra_mask.sa_family = AF_INET;
	   pSin_snmsk->sin_addr.s_addr = htonl (subnetMask);
	   }

	return (ifIoctlCall (SIOCAIFADDR, (struct ifreq *) &ifa));
	}
