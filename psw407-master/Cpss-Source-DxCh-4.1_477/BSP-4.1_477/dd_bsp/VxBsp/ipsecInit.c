/*---------------------

Copyright (C) Wind River Systems, Inc. All rights are reserved.

Redistribution and use of the software in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  1. Redistributions in source code form must retain all copyright notices
     and a copy of this license.
  2. No right is granted herein to use the Wind River name, trademarks, trade dress or
     other company identifiers (except in connection with required copyright notices).
  3. THIS SOFTWARE IS PROVIDED BY WIND RIVER SYSTEMS "AS IS" AND ANY EXPRESS OR
     IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT,
     ARE DISCLAIMED. IN NO EVENT SHALL WIND RIVER SYSTEMS BE LIABLE FOR ANY
     DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
     INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
     ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
     THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

End of License.

---------------------*/
/*
modification history
--------------------
01a,28sep04,rlm  Fixes to instructions which allow proper building of
                 vxWorks and vxWorks.st images from command line.
*/

/****************************************************************
description:
-----------

This source file may be used to demonstrate IPsec/IKE between
two targets.

This file can be used with project-facility builds as well as
command-line builds.

This file was tested with PNE 2.0.

NOTE:
The IP default addresses used in this test may be redefined
from the compiler command-line if desired; see Step 5 below.
The current defaults are:
        - 192.168.1.1 = board1
        - 192.168.1.2 = board2

-----

The following steps describe how to build ipsecInit.c using the
command-line build from the BSP directory.  You can instead use the
Tornado IDE and simply include the ipsecInit.c file in your project
and then rebuild. (Step by step instructions are not included here.
For instructions using the IDE, see the IPSec&IKE user's guide).

Command-line build instructions
-------------------------------

Step 1: Add this source file to the BSP folder.
------  (e.g. C:\t221_pne\target\config\wrPpmc824x\ )

Step 2: Adjust config.h to include the following defines:
------

--- begin ---
#define INCLUDE_SHELL
#define INCLUDE_DEBUG
#define INCLUDE_LOADER
#define INCLUDE_UNLOADER
#define INCLUDE_SYM_TBL

#define STANDALONE_NET
#define INCLUDE_NETWORK
#define INCLUDE_NET_INIT

#define  INCLUDE_PING
#undef  INCLUDE_FASTUDP
#define INCLUDE_ZBUF_SOCK

#define  INCLUDE_SHOW_ROUTINES
#define  INCLUDE_NET_SHOW
#undef  INCLUDE_PCI_CFGSHOW

#if (_WRS_VXWORKS_MAJOR >= 6)
#define INCLUDE_RTP
#define INCLUDE_RTP_HOOKS
#define INCLUDE_SYSCALL_HOOKS
#define INCLUDE_SHARED_DATA
#define INCLUDE_SC_SYSCTL
#define INCLUDE_SYSCTL
#endif
--- end ---

Step 3: Create usrAppInit.c
------
  In the BSP directory, create a file named usrAppInit.c, containing
  a function named usrAppInit() which forces the demonstration code
  to be linked into the vxWorks image. Example:

--- begin ---
void ipsecDemoInclude(void);

void usrAppInit (void)
    {
    ipsecDemoInclude();
    }
--- end ---


Step 4: Adjust the BSP Makefile so that this object is linked
------  to the vxWorks image:

      MACH_EXTRA = ipsecInit.o usrAppInit.o

Step 5: Rebuild the vxWorks image:
------
 C:> cd t221_pne\host\x86-win32\bin
 C:\t221_pne\host\x86-win32\bin> torvars
 C:\t221_pne\host\x86-win32\bin> cd \
 C:> cd t221_pne\target\config\wrPpmc824x

 C:\t221_pne\target\config\wrPpmc824x> make CPU=PPC603 TOOL=gnu clean
 C:\t221_pne\target\config\wrPpmc824x> make CPU=PPC603 TOOL=gnu vxWorks.st
   (for default IP addresses of 192.168.1.1, 192.168.1.2)

   OR to change the IP addresses of the 2 targets you can either edit the
   example's source code (search for "TARGET-SPECIFIC NETWORK INFORMATION" section, below)
   or specify the new IPs on the command line as follows:
   
 C:\t221_pne\target\config\wrPpmc824x> make CPU=PPC603 TOOL=gnu vxWorks.st \
                        "ADDED_CFLAGS+=-DGW_1_ADDR=\"\"\"192.168.1.3\"\"\" -DGW_2_ADDR=\"\"\"192.168.1.4\"\"\""
   (to set explicit addresses for your existing network)
   (Yes, you need all those \" quotes!)

  NOTE: for the new v6-capable stack (whether in v4-only or v4v6-mode) you
  must define the proper network device as well as the v4 or v6 address for
  each host.

  Example (using devices 'dc0' and 'lnPci0' for targets 1 and 2):

 C:\t221_pne\target\config\wrPpmc824x> make CPU=PPC603 TOOL=gnu vxWorks.st \
                        "ADDED_CFLAGS+=-DINET6 -DGW_1_ADDR=\"\"\"192.168.1.3\"\"\" -DGW_2_ADDR=\"\"\"192.168.1.4\"\"\" -DGW_1_IF=\"\"\"dc0\"\"\" -DGW_2_IF=\"\"\"lnPci0\"\"\""

Step 6a (IKE Tunnel Test): From the target shell, execute the following:
-------
For board 1:
-> ipsecIKETunnelInit(1)

For board 2:
-> ipsecIKETunnelInit(2)

Step 6b (MKM Transport, NULL encryption Test): From the target shell,
------- execute the following:

For board 1:
-> ipsecMKMNullTransportInit(1)

For board 2:
-> ipsecMKMNullTransportInit(2)


ADDITIONAL CAVEATS:
 - White space is not permitted in any of the initialization strings
   for IPSEC/IKE
 - demonstration tasks must be started at a high priority.
   They should not be spawned using the sp() API.

****************************************************************/
/***************************************************************
 *  TARGET-SPECIFIC NETWORK INFORMATION
 ***************************************************************/
#define GW_1_IF   	"egiga1"
#define GW_1_ADDR 	"192.168.1.1" 

#define GW_2_IF   	"egiga1" 
#define GW_2_ADDR 	"192.168.1.5" 


#ifndef GW_1_IF
/*#define GW_1_IF "dc0"*/
#define GW_1_IF "fei0"
#endif
#ifndef GW_2_IF
/*#define GW_2_IF "lnPci0"*/
#define GW_2_IF "fei0"
#endif

#if !defined(INET6)
#warning -- IPv4 addresses assigned for IPsec demo --
#ifndef GW_1_ADDR
/*#define GW_1_ADDR "192.168.1.1"*/
#define GW_1_ADDR "10.2.0.22"
#endif

#ifndef GW_2_ADDR
/*#define GW_2_ADDR "192.168.1.2"*/
#define GW_2_ADDR "10.2.0.150"
#endif

#else
#warning -- IPv6 addresses assigned for IPsec demo --
#ifndef GW_1_ADDR
#define GW_1_ADDR "3ffe::1"
#endif

#ifndef GW_2_ADDR
#define GW_2_ADDR "3ffe::2"
#endif
#endif /* !defined(INET6) */


/***************************************************************
 *  INCLUDE FILES
 ***************************************************************/
#include <vxWorks.h>
#include <stdio.h>
#include <unistd.h>
#include <taskLib.h>
#include "mvOs.h"
#include "wrn/cci/cci.h"

#ifndef CCI_MAJOR_VER
#define CCI_MAJOR_VER	3
#define CCI_MINOR_VER	0
#endif 

#ifndef CCI_TASK_PRIORITY
#define CCI_TASK_PRIORITY 75
#endif

#ifndef SHARED_REGION_SIZE
#define SHARED_REGION_SIZE 4096
#endif 

#define IPSEC_AUTOSTART			0
#define IPSEC_NET_JOB_MAX		100

#define INCLUDE_CCI_COREAPI

/* CCI - Common Crypto Interface */
#if ((CCI_MAJOR_VER>3) || ((CCI_MAJOR_VER==3) && (CCI_MINOR_VER > 1)))
#define INCLUDE_CCI_PROVIDERS

  /* Define what CCI provider we want to use (software default here) */
  #define INCLUDE_CCI_DEFAULT_PROVIDER

/* Define what CCI algorithms we want for IPsec */
#define INCLUDE_CCI_IMPORT_AES
#define INCLUDE_CCI_IMPORT_AESKW
#define INCLUDE_CCI_IMPORT_DES
#define INCLUDE_CCI_IMPORT_RC4
#define INCLUDE_CCI_IMPORT_RC4TKIP
#define INCLUDE_CCI_IMPORT_NULL
#define INCLUDE_CCI_IMPORT_PRNG
#define INCLUDE_CCI_IMPORT_HASH_CRC32
#define INCLUDE_CCI_IMPORT_HASH_MD2
#define INCLUDE_CCI_IMPORT_HASH_MD4
#define INCLUDE_CCI_IMPORT_HASH_MD5
#define INCLUDE_CCI_IMPORT_HASH_SHA1
#define INCLUDE_CCI_IMPORT_HASH_SHA224
#define INCLUDE_CCI_IMPORT_HASH_SHA256
#define INCLUDE_CCI_IMPORT_HASH_SHA384
#define INCLUDE_CCI_IMPORT_HASH_SHA512
#define INCLUDE_CCI_IMPORT_HASH_RIP160
#define INCLUDE_CCI_IMPORT_HASH_RIP128
#define INCLUDE_CCI_IMPORT_HMAC_MD4
#define INCLUDE_CCI_IMPORT_HMAC_MD5
#define INCLUDE_CCI_IMPORT_HMAC_SHA1
#define INCLUDE_CCI_IMPORT_HMAC_SHA224
#define INCLUDE_CCI_IMPORT_HMAC_SHA256
#define INCLUDE_CCI_IMPORT_HMAC_SHA384
#define INCLUDE_CCI_IMPORT_HMAC_SHA512
#define INCLUDE_CCI_IMPORT_HMAC_RIP160
#define INCLUDE_CCI_IMPORT_HMAC_RIP128
#define INCLUDE_CCI_IMPORT_HMAC_AESXCBC
#define INCLUDE_CCI_IMPORT_PUBLICKEY_RSA
#define INCLUDE_CCI_IMPORT_IPSEC_SINGLEPASS
#define INCLUDE_CCI_IMPORT_INTEGER
#define INCLUDE_CCI_IMPORT_CAST
#define INCLUDE_CCI_IMPORT_BLOWFISH
#endif /* CCI version > 3.1 */

/* Define IP-Security features we want to include */
#define INCLUDE_IP_SECURITY_PROTOCOL
#define INCLUDE_INTERNET_KEY_EXCHANGE
#define INCLUDE_MKM
#define INCLUDE_SECURITY_ASSOCIATION_DATABASE
#define INCLUDE_SECURITY_POLICY_DATABASE

#if !defined(PRJ_BUILD)
#ifdef INCLUDE_SECURITY_POLICY_DATABASE
#include "wrn/spd/spdInit.h"
#include "../comps/src/net/usrNetSpdInit.c"
#endif /* INCLUDE_SECURITY_POLICY_DATABASE */
#ifdef INCLUDE_SECURITY_ASSOCIATION_DATABASE
#include "wrn/sadb/sadbInit.h"
#include "../comps/src/net/usrNetSadbInit.c"
#endif /* INCLUDE_SECURITY_ASSOCIATION_DATABASE */
#ifdef INCLUDE_CCI_COREAPI
#include "../comps/src/net/usrNetCciInit.c"
#endif /* INCLUDE_CCI_COREAPI */
#ifdef INCLUDE_INTERNET_KEY_EXCHANGE
#include "wrn/ike/ikeInit.h"
#include "../comps/src/net/usrNetIkeInit.c"
#endif /* INCLUDE_INTERNET_KEY_EXCHANGE */
#ifdef INCLUDE_IP_SECURITY_PROTOCOL
#include "wrn/ipsec/ipsecInit.h"
#include "../comps/src/net/usrNetIpsecInit.c"
#endif /* INCLUDE_IP_SECURITY_PROTOCOL */
#ifdef INCLUDE_MKM
#include "wrn/ipsec/mkmInit.h"
#include "../comps/src/net/usrNetMkmInit.c"
#endif /* INCLUDE_MKM */
#endif /* #if !defined(PRJ_BUILD) */
/***************************************************************
 *  DEFINES
 ***************************************************************/
#ifdef INCLUDE_SECURITY_POLICY_DATABASE
#define INCLUDE_IPV4
#define INCLUDE_RWLIB
#define INCLUDE_SECURITY_ASSOCIATION_DATABASE
#endif /* INCLUDE_SECURITY_POLICY_DATABASE */

#ifdef INCLUDE_SECURITY_ASSOCIATION_DATABASE
#define INCLUDE_CCI_COREAPP
#endif /* INCLUDE_SECURITY_ASSOCIATION_DATABASE */
#define IPSEC_DEMO_DEBUG

/***************************************************************
 *  MACROS
 ***************************************************************/
#ifdef IPSEC_DEMO_DEBUG
#define IPSEC_DEMO_PRINTF(X) printf(X)
#else
#define IPSEC_DEMO_PRINTF(X)
#endif

/*
 * Convenience macro to make error checking look tidy.
 * SIDE EFFECT: This macro may return(-1) out of current func!!
 */
#define CATCH_ERR(var, str) \
    if(var != OK) \
      { \
      IPSEC_DEMO_PRINTF(str); \
      return(-1); \
      }

/***************************************************************
 *  EXTERNAL PROTOTYPES
 ***************************************************************/

void   ipsecShowVer (void); 

STATUS ikeAddPeerAuth ( char *configString );
STATUS ikeSetIfAddr(char* configuration_string);
STATUS ikeSetXform(char* configuration_string);
STATUS ikeSetProp(char* configuration_string);
STATUS ikeSetPropAttrib(char* configuration_string);

STATUS spdSetAHXform(char* p_configuration_string);
STATUS spdSetProp(char* p_configuration_string);
STATUS spdSetPropAttrib(char* p_configuration_string);
STATUS spdSetESPXform(char* p_configuration_string);
STATUS spdSetSA(char* p_configuration_string);
STATUS spdAddTunnel(char* p_configuration_string);
STATUS spdAddBypass(char* p_configuration_string);
STATUS spdAddTransport(char* p_configuration_string);

STATUS mkmAddTransport(char* p_configuration_string);
STATUS mkmSetInboundESP(char* p_configuration_string);
STATUS mkmSetOutboundESP(char* p_configuration_string);
STATUS mkmCommitAll(void);

STATUS ipsecShow (void);
STATUS ipsecShowIf (void);
STATUS ipsecAttachIf(char* cptrAddrString);

CCIProvider mvExportCCIProvider( void );

/***************************************************************
 *  INTERNAL PROTOTYPES
 ***************************************************************/
/* ipsecDemoInclude() is a dummy function which should be called
 * to coerce the linker into including all functions in this
 * module.
 */
void ipsecDemoInclude(void);

/* setup 1:
 * -IKE Tunnel mode. Phase I using 3DES/SHA1,
 *  Phase II using AH+ESP with 3DES/MD5
 */
STATUS ipsecIKETunnelInit(int boardNum);
STATUS ipsecIKETunnelPhaseOneInit(int boardNum);
STATUS ipsecIKETunnelPhaseTwoInit(int boardNum);

/* setup 2:
 * -MKM Transport with NULL encryption.
 */
STATUS ipsecMKMNullTransportInit(int boardNum);

/***************************************************************
 *  INTERNAL VARIABLES
 ***************************************************************/
/***************************************************************
 *  FUNCTIONS
 ***************************************************************/

void ipsecDemoInclude()
    {
    /* Do nothing */
    }

extern int	cciAlgorithmSupport;

/***************************************************************
 *  ipsecComponentsInit() - start up all IPsec/IKE modules
 ***************************************************************/
STATUS ipsecComponentsInit(void)
{
    STATUS status;


#ifdef INCLUDE_CCI_COREAPP
    /* Initialize the library and force it to be linked */
    status = usrCciInit();
    CATCH_ERR(status,"usrCciInit() failed...\n");

    /* Default Software Crypto Provider  */
    /* 0 changed to 1 by sjk - to force software */
    printf("Load default provider\n");
    status = usrCciLoadProvider (CCI_DEFAULT_PROVIDER, 1);
    CATCH_ERR(status,"usrCciLoadProvider() failed...\n");
	if (cciAlgorithmSupport != 0)
	{
		printf("Load mvExportCCIProvider\n");
		status = usrCciLoadProvider (mvExportCCIProvider, 1);
		if(status != OK)
			printf("usrCciLoadProvider() failed. Status=%d\n", status);
	}
#endif /* INCLUDE_CCI_COREAPP */

#ifdef INCLUDE_IP_SECURITY_PROTOCOL
    /* provide authentication and encryption of ip datagrams. */
    status = usrIpsecInit(1);
    CATCH_ERR(status,"usrIpsecInit() failed...\n");
#endif /* INCLUDE_IP_SECURITY_PROTOCOL */


#ifdef INCLUDE_INTERNET_KEY_EXCHANGE
    /* dynamic keying for security association */
    status = usrIkeInit(1);
    CATCH_ERR(status,"usrIkeInit() failed...\n");
#endif /* INCLUDE_INTERNET_KEY_EXCHANGE */

#ifdef INCLUDE_MKM
    /* manual keying for security association */
    status = usrMkmInit(1);
    CATCH_ERR(status,"usrMkmInit() failed...\n");
#endif /* INCLUDE_MKM */
	ipsecShowVer();
    return(OK);
  } /* end ipsecComponentsInit() */



/***************************************************************
 *  ipsecIKETunnelInit() - set up IPsec for an IKE tunnel
 *
 *  INPUT: int boardNum - 1 or 2, defines which endpoint this is
 *
 *  RETURN: STATUS - success code (0 for OK or -1 for failure)
 ***************************************************************/
STATUS ipsecIKETunnelInit(int boardNum)
    {
    STATUS status;

    if( (boardNum != 1) && (boardNum != 2) )
      {
      IPSEC_DEMO_PRINTF("ipsecIKETunnelInit() -> invalid board number...\n");
      IPSEC_DEMO_PRINTF("usage: ipsecIKETunnelInit(boardNum);\n");
      return(-1);
      }

#ifdef INET6
    /*
     * Give interface a known IPv6 address for tests.
     */
    if( boardNum == 1 )
      status = ifconfig(GW_1_IF " inet6 " GW_1_ADDR);
    else
      status = ifconfig(GW_2_IF " inet6 " GW_2_ADDR);
    CATCH_ERR(status,"ifconfig() failed...\n");

    taskDelay(100); /* give time for address re-configuration */
#endif /* INET6 */

#if !defined(PRJ_BUILD)
    status = ipsecComponentsInit();
    CATCH_ERR(status,"ipsecComponentsInit() failed...\n");
#endif /* PRJ_BUILD */

    /*
     * Set up IKE Phase I proposals for communication with peer
     */
    status = ipsecIKETunnelPhaseOneInit(boardNum);
    CATCH_ERR(status,"ipsecIKETunnelPhaseOneInit() failed...\n");

    /*
     * Set up phase II policies
     */
    status = ipsecIKETunnelPhaseTwoInit(boardNum);
    CATCH_ERR(status,"ipsecIKETunnelPhaseTwoInit() failed...\n");

    /*
     * That's it! If all went well above we should be able to 'ping'
     * the other host over the encrypted channel.
     */

#ifdef IPSEC_DEMO_DEBUG
    printf ("\nIP-SEC demo: initialization completed. Try pinging other peer.\n");
    ipsecShow();
    ipsecShowIf();
#endif

    return(OK);
    } /* end ipsecIKETunnelInit() */



/***************************************************************
 *  ipsecIKETunnelPhaseOneInit()
 ***************************************************************/
STATUS ipsecIKETunnelPhaseOneInit(int boardNum)
{
  STATUS status;

  /* Tell IKE to listen for exchanges on this interface */
  if (boardNum == 1)
  {
    status = ikeSetIfAddr(GW_1_ADDR);
    CATCH_ERR(status,"ikeSetIfAddr() failed (Host 1)...\n");
  }
  else
  {
    status = ikeSetIfAddr(GW_2_ADDR);
    CATCH_ERR(status,"ikeSetIfAddr() failed (Host 2)...\n");
  }

  /*
   * Define Phase I (IKE) proposals
   * 
   * Phase I (IKE) proposals are built in the following manner:
   * 
   * 1. Define a phase I transform               (ikeSetXform())
   * 2. Bind transform to a new phase I proposal (ikeSetProp())
   * 3. Define properties for that proposal
   *    (DH group, lifetime)                     (ikeSetPropAttrib())
   * 4. Define a peer for IKE communication
   *    using phase I proposal                   (ikeAddPeerAuth())
   */
  
  /* 1): Define new IKE transform: 3DES/SHA1. Call it "ph1_3des_sha" */
  status = ikeSetXform("ph1_3des_sha,3DES,SHA");
  CATCH_ERR(status,"ikeSetXform() failed...\n");
  
  /* 2): Add transform to new IKE proposal, called "ph1_g1_1" */
  status = ikeSetProp("ph1_g1_1,ph1_3des_sha" );
  CATCH_ERR(status,"ikeSetProp() failed...\n");
  
  /* 3): Set IKE proposal properties for "ph1_g1_1" */
  status = ikeSetPropAttrib ("ph1_g1_1,DHGROUP,G1,LIFETIME,28880,UNITOFTIME,SECS");
  CATCH_ERR(status,"ikeSetPropAttrib() failed...\n");
  
  if (boardNum == 1)
  {
    /* 4): Set IKE peer, perfect-forward secrecy mode and pre-shared key */
#if ((CCI_MAJOR_VER>3) || ((CCI_MAJOR_VER==3) && (CCI_MINOR_VER > 1)))
    status = ikeAddPeerAuth ( GW_2_ADDR "," GW_1_ADDR ",ph1_g1_1,PSK,itsasecret");
#else
    status = ikeAddPeerAuth ( GW_2_ADDR "," GW_1_ADDR ",ph1_g1_1,NOPFS,PSK,itsasecret");
#endif
    CATCH_ERR(status,"ikeAddPeerAuth() failed (Host 1)...\n");
  }
  else
  {
    /* 4): Set IKE peer, perfect-forward secrecy mode and pre-shared key */
#if ((CCI_MAJOR_VER>3) || ((CCI_MAJOR_VER==3) && (CCI_MINOR_VER > 1)))
	  status = ikeAddPeerAuth ( GW_1_ADDR "," GW_2_ADDR ",ph1_g1_1,,PSK,itsasecret");
#else
	  status = ikeAddPeerAuth ( GW_1_ADDR "," GW_2_ADDR ",ph1_g1_1,NOPFS,PSK,itsasecret");
#endif
    CATCH_ERR(status,"ikeAddPeerAuth() failed (Host 2)...\n");
  } /* end if(boardNum) */

  return(OK);
} /* end ipsecIKETunnelPhaseOneInit() */



/***************************************************************
 *  ipsecIKETunnelPhaseTwoInit()
 ***************************************************************/
STATUS ipsecIKETunnelPhaseTwoInit(int boardNum)
{
  STATUS status;

  /* Define phase II proposals
   *
   * Phase II proposals are built in the following manner:
   * 
   * 1) Defining one or more AH or ESP transforms    (spdSetAHXform or spdSetESPXform)
   * 2) Binding that transform to a proposal         (spdSetProp)
   * 3) Setting attributes for that proposal         (spdSetPropAttrib)
   * 4) Creating the Security Association (SA)       (spdSetSA)
   *    proposal out of previously-defined transform
   *    proposals
   * 5) Using the SA to set up a tunnel or transport (spdAddTransport or spdAddTunnel)
   *    Security Policy Database (SPD) entry.
   */
  
  /* 1): Define an AH transform using MD5 authentication and call it "ah_md5_1" */
  status = spdSetAHXform("ah_md5_1,AHMD5");
  CATCH_ERR(status,"spdSetAHXform() failed...\n");
  
  /* 2): Bind transform "ah_md5_1" to proposal "prop_ahmd5" */
  status = spdSetProp("prop_ahmd5,ah_md5_1");
  CATCH_ERR(status,"spdSetProp() AH failed...\n");
  
  /*
   * 3): Set attributes (encapsulation type = TUNNEL, DH group 1, hard and soft
   * lifetimes) for "prop_ahmd5"
   */
  status = spdSetPropAttrib("prop_ahmd5,DHGROUP,G1,ENCAP,TUNNEL,HARDLIFETIME,1800,SOFTLIFETIME,1500");
  CATCH_ERR(status,"spdSetPropAttrib() AH failed...\n");
  
  /* Identical process for creating an ESP transform */
  /*
   * 1): Define an ESP transform using MD5 authentication and 3DES encryption,
   * called "esp_3des_md5_1"
   */
  status = spdSetESPXform("esp_3des_md5_1,ESP3DES,MD5");
  CATCH_ERR(status,"spdSetESPXform() failed...\n");
  
  /* 2): Bind transform "esp_3des_md5_1" to proposal "prop_esp3desmd5" */
  status = spdSetProp("prop_esp3desmd5,esp_3des_md5_1");
  CATCH_ERR(status,"spdSetProp() ESP failed...\n");
  
  /*
   * 3): Set attributes (encapsulation type = TUNNEL, DH group 1, hard and soft
   * lifetimes) for "prop_ahmd5"
   * (NOTE: These had better match the AH proposal if you want to use them together!)
   */
  status = spdSetPropAttrib("prop_esp3desmd5,DHGROUP,G1,ENCAP,TUNNEL,HARDLIFETIME,1800,SOFTLIFETIME,1500");
  CATCH_ERR(status,"spdSetPropAttrib() ESP failed...\n");
  
  /*
   * 4): Now we define a Security Association that requires AH + ESP as defined
   * above, called "sa_ahesp_md5_3desmd5"
   * (Here we are saying that AH + ESP are BOTH required, as we have set both
   * proposal numbers to 1. If this were an SA requesting one OR the
   * other, we would set them as proposals 1 and 2, respectively.)
   *
   * ====================================================================
   * NOTE: For examples of more complex IKE policies (with SAs containing
   *       multiple proposals), see usrNetIkeInit.c and usrNetSpdInit.c
   *       in $WIND_BASE/target/config/comps/src/net
   * ====================================================================
   */
  status = spdSetSA("sa_ahesp_md5_3desmd5,prop_ahmd5,1,prop_esp3desmd5,1");
  CATCH_ERR(status,"spdSetSA() failed...\n");
  
  if (boardNum == 1)
  {
    /* HOST1 configuration for IPv4 */
    IPSEC_DEMO_PRINTF ("\nIP-SEC demo: Configuring HOST#1 (of 2) with IPv4\n");
    
    /*
     * 5): Finally we can add the SA to the Security Policy Database (SPD).
     * Since we defined both proposals above to use TUNNEL encapsulation the
     * call must be to spdAddTunnel().
     */
    status = spdAddTunnel("ANY," GW_2_ADDR "," GW_1_ADDR ",OUT,PACKET,IKE,sa_ahesp_md5_3desmd5," GW_2_ADDR);
    CATCH_ERR(status,"spdAddTunnel() failed (Host 1)...\n");
    
    /* Set bypass policy for IKE TCP port (500) */
    status = spdAddBypass("17/500/500," GW_2_ADDR "," GW_1_ADDR ",OUT,MIRRORED");
    CATCH_ERR(status,"spdAddBypass() failed (Host 1)...\n");
    
    /* Set bypass policy for IPSEC-ESP protocol */
    status = spdAddBypass("50," GW_2_ADDR "," GW_1_ADDR ",OUT,MIRRORED");
    CATCH_ERR(status,"spdAddBypass() failed (Host 1)...\n");
    
    /* Set bypass policy for IPSEC-AH protocol */
    status = spdAddBypass("51," GW_2_ADDR "," GW_1_ADDR ",OUT,MIRRORED");
    CATCH_ERR(status,"spdAddBypass() failed (Host 1)...\n");

    /* Attach IPSEC to END device. NOTE that this can be done
     * before setting up the above policies.
     */
    status = ipsecAttachIf (GW_1_ADDR);
    CATCH_ERR(status,"ipsecAttachIf() failed (Host 1)...\n");
  }
  else
  {
    /* HOST2 configuration for IPv4 */
    IPSEC_DEMO_PRINTF ("\nIP-SEC demo: Configuring HOST#2 (of 2) with IPv4\n");
    
    /*
     * 5): Finally we can add the SA to the Security Policy Database (SPD).
     * Since we defined both proposals above to use TUNNEL encapsulation the
     * call must be to spdAddTunnel().
     */
    status = spdAddTunnel("ANY," GW_1_ADDR "," GW_2_ADDR ",OUT,PACKET,IKE,sa_ahesp_md5_3desmd5," GW_1_ADDR);
    CATCH_ERR(status,"spdAddTunnel() failed (Host 2)...\n");
    
    /* Set bypass policy for IKE TCP port (500) */
    status = spdAddBypass("17/500/500," GW_1_ADDR "," GW_2_ADDR ",OUT,MIRRORED");
    CATCH_ERR(status,"spdAddBypass() failed (Host 2)...\n");
    
    /* Set bypass policy for IPSEC-ESP protocol */
    status = spdAddBypass("50," GW_1_ADDR "," GW_2_ADDR ",OUT,MIRRORED");
    CATCH_ERR(status,"spdAddBypass() failed (Host 2)...\n");
    
    /* Set bypass policy for IPSEC-AH protocol */
    status = spdAddBypass("51," GW_1_ADDR "," GW_2_ADDR ",OUT,MIRRORED");
    CATCH_ERR(status,"spdAddBypass() failed (Host 2)...\n");
    
    /* Attach IPSEC to END device. NOTE that this can be done
     * before setting up the above policies.
     */
    status = ipsecAttachIf (GW_2_ADDR);
    CATCH_ERR(status,"ipsecAttachIf() failed (Host 2)...\n");
  } /* end if(boardNum) */

  return(OK);
} /* end ipsecIKETunnelPhaseTwoInit() */



/***************************************************************
 *  ipsecMKMNullTransportInit() - set up IPsec for MKM transport
 *                                and NULL encryption
 *
 *  INPUT: int boardNum - 1 or 2, defines which endpoint this is
 *
 *  RETURN: STATUS - success code (0 for OK or -1 for failure)
 ***************************************************************/
STATUS ipsecMKMNullTransportInit(int boardNum)
    {
    STATUS status;

    if( (boardNum != 1) && (boardNum != 2) )
      {
      IPSEC_DEMO_PRINTF("ipsecMKMNullTransportInit() -> invalid board number...\n");
      IPSEC_DEMO_PRINTF("usage: ipsecMKMNullTransportInit(boardNum);\n");
      return(-1);
      }

#ifdef INET6
    /*
     * Give interface a known IPv6 address for tests.
     */
    if( boardNum == 1 )
      status = ifconfig(GW_1_IF " inet6 " GW_1_ADDR);
    else
      status = ifconfig(GW_2_IF " inet6 " GW_2_ADDR);

    CATCH_ERR(status,"ifconfig() failed...\n");

    taskDelay(100); /* give time for address re-configuration */
#endif /* INET6 */

#if !defined(PRJ_BUILD)
    status = ipsecComponentsInit();
    CATCH_ERR(status,"ipsecComponentsInit() failed...\n");
#endif /* PRJ_BUILD */

    /* Attach IPsec to primary interface */
    if( boardNum == 1)
      {
      status = ipsecAttachIf(GW_1_ADDR);
      }
    else
      {
      status = ipsecAttachIf(GW_2_ADDR);
      }
    CATCH_ERR(status,"ipsecAttachIf() failed...\n");

    /*
     * Set up MKM policies
     */
    status = spdSetESPXform("esp_null_md5_1,ESPNULL,MD5");
    CATCH_ERR(status,"spdSetESPXform() failed...\n");
    status = spdSetProp("prop_espnullmd5,esp_null_md5_1");
    CATCH_ERR(status,"spdSetProp() failed...\n");
    status = spdSetPropAttrib("prop_espnullmd5,DHGROUP,G1,ENCAP,TRANSPORT,HARDLIFETIME,1800,SOFTLIFETIME,1500");
    CATCH_ERR(status,"spdSetPropAttrib() failed...\n");
    status = spdSetSA("sa_espnullmd5,prop_espnullmd5,1");
    CATCH_ERR(status,"spdSetSA() failed...\n");

    if( boardNum == 1 )
      {
      status = spdAddTransport("ANY," GW_2_ADDR "," GW_1_ADDR ",OUT,PACKET,MANUAL,sa_espnullmd5");
      CATCH_ERR(status,"spdAddTransport() failed...\n");
      /* ESP bypass policy */
      status = spdAddBypass("50," GW_2_ADDR "," GW_1_ADDR ",OUT,MIRRORED");
      CATCH_ERR(status,"spdAddBypass() (ESP) failed...\n");
      /* AH bypass policy */
      status = spdAddBypass("51," GW_2_ADDR "," GW_1_ADDR ",OUT,MIRRORED");
      CATCH_ERR(status,"spdAddBypass() (AH) failed...\n");
      status = mkmAddTransport("4,ANY," GW_2_ADDR "," GW_1_ADDR ",OUT," GW_1_ADDR);
      CATCH_ERR(status,"mkmAddTransport() failed...\n");
      }
    else
      {
      status = spdAddTransport("ANY," GW_1_ADDR "," GW_2_ADDR ",OUT,PACKET,MANUAL,sa_espnullmd5");
      CATCH_ERR(status,"spdAddTransport() failed...\n");
      /* ESP bypass policy */
      status = spdAddBypass("50," GW_1_ADDR "," GW_2_ADDR ",OUT,MIRRORED");
      CATCH_ERR(status,"spdAddBypass() (ESP) failed...\n");
      /* AH bypass policy */
      status = spdAddBypass("51," GW_1_ADDR "," GW_2_ADDR ",OUT,MIRRORED");
      CATCH_ERR(status,"spdAddBypass() (AH) failed...\n");
      status = mkmAddTransport("4,ANY," GW_1_ADDR "," GW_2_ADDR ",OUT," GW_2_ADDR);
      CATCH_ERR(status,"mkmAddTransport() failed...\n");
      }

    status = mkmSetInboundESP("4,260,ESPNULL,AUTHALG,MD5,AUTHKEY,66306630663066306630663066302222");
    CATCH_ERR(status,"mkmSetInboundESP() failed...\n");
    status = mkmSetOutboundESP("4,260,ESPNULL,AUTHALG,MD5,AUTHKEY,66306630663066306630663066302222");
    CATCH_ERR(status,"mkmSetOutboundESP() failed...\n");
    status = mkmCommitAll();
    CATCH_ERR(status,"mkmCommitAll() failed...\n");

    /*
     * That's it! If all went well above we should be able to 'ping'
     * the other host over the "encrypted" channel.
     */

#ifdef IPSEC_DEMO_DEBUG
    printf ("\nIPsec configured for MKM transport with NULL encryption.\nTry pinging other peer.\n");
    ipsecShow();
    ipsecShowIf();
#endif

    return(OK);
    } /* end ipsecMKMNullTransportInit() */


/***************************************************************
 *  END OF FILE
 ***************************************************************/
