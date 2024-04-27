#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../deffines.h"

#include "mib-init.h"
#include "ber.h"
#include "utils.h"
#include "logging.h"
#include "dispatcher.h"
#include "snmpd.h"
#include "../controls/ctrls_deffines.h"
#include "debug.h"
#include "stm32f4x7_eth.h"
#include "board.h"
#include "settings.h"
#include "../snmp.h"
#include "../uip/uip.h"
#include "SMIApi.h"
#include "../webserver/httpd-cgi.h"

extern uint8_t dev_addr[6];
extern struct status_t status;

/* SNMPv2 system group */
static u8t ber_oid_system_sysDesc[]              = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00};
static ptr_t oid_system_sysDesc                  = {ber_oid_system_sysDesc, 8};

static u8t ber_oid_system_sysObjectId []         = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x02, 0x00};
static ptr_t oid_system_sysObjectId              = {ber_oid_system_sysObjectId, 8};

static u8t ber_oid_system_sysUpTime []           = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x03, 0x00};
static ptr_t oid_system_sysUpTime                = {ber_oid_system_sysUpTime, 8};

static u8t ber_oid_system_sysContact []          = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x04, 0x00};
static ptr_t oid_system_sysContact               = {ber_oid_system_sysContact, 8};

static u8t ber_oid_system_sysName []             = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x05, 0x00};
static ptr_t oid_system_sysName                  = {ber_oid_system_sysName, 8};

static u8t ber_oid_system_sysLocation []         = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x06, 0x00};
static ptr_t oid_system_sysLocation              = {ber_oid_system_sysLocation, 8};

static u8t ber_oid_system_sysServices []         = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x07, 0x00};
static ptr_t oid_system_sysServices              = {ber_oid_system_sysServices, 8};

static u8t ber_oid_system_sysORLastChange []     = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x08, 0x00};
static ptr_t oid_system_sysORLastChange          = {ber_oid_system_sysORLastChange, 8};

static u8t ber_oid_system_sysOREntry []          = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x09, 0x01};
static ptr_t oid_system_sysOREntry               = {ber_oid_system_sysOREntry, 8};

/* interfaces group*/
static u8t ber_oid_ifNumber[]                    = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x02, 0x01, 0x00};
static ptr_t oid_ifNumber                        = {ber_oid_ifNumber, 8};


static u8t ber_oid_ifEntry[]                     = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x02, 0x02, 0x01};
static ptr_t oid_ifEntry                         = {ber_oid_ifEntry, 8};

//at group
static u8t ber_oid_atEntry[]                     = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x03, 0x01, 0x01};
static ptr_t oid_atEntry                         = {ber_oid_atEntry, 8};

//ip group

static u8t ber_oid_ipForwarding []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x01, 0x00};
static ptr_t oid_ipForwarding                	 = {ber_oid_ipForwarding , 8};

static u8t ber_oid_ipDefaultTTL []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x02, 0x00};
static ptr_t oid_ipDefaultTTL                    = {ber_oid_ipDefaultTTL , 8};

static u8t ber_oid_ipInReceives []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x03, 0x00};
static ptr_t oid_ipInReceives                    = {ber_oid_ipInReceives , 8};

static u8t ber_oid_ipInHdrErrors []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x04, 0x00};
static ptr_t oid_ipInHdrErrors                    = {ber_oid_ipInHdrErrors , 8};

static u8t ber_oid_ipInAddrError []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x05, 0x00};
static ptr_t oid_ipInAddrError                    = {ber_oid_ipInAddrError , 8};

static u8t ber_oid_ipForwDatagrams []             = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x06, 0x00};
static ptr_t oid_ipForwDatagrams                  = {ber_oid_ipForwDatagrams , 8};

static u8t ber_oid_ipUnknownProtos []             = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x07, 0x00};
static ptr_t oid_ipUnknownProtos                  = {ber_oid_ipUnknownProtos , 8};

static u8t ber_oid_ipIpDiscards []             	  = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x08, 0x00};
static ptr_t oid_ipIpDiscards                     = {ber_oid_ipIpDiscards , 8};

static u8t ber_oid_ipInDelivers []             	  = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x09, 0x00};
static ptr_t oid_ipInDelivers                     = {ber_oid_ipInDelivers , 8};

static u8t ber_oid_ipOutRequests []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x0A, 0x00};
static ptr_t oid_ipOutRequests                    = {ber_oid_ipOutRequests , 8};

static u8t ber_oid_ipOutDiscards []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x0B, 0x00};
static ptr_t oid_ipOutDiscards                    = {ber_oid_ipOutDiscards , 8};

static u8t ber_oid_ipOutNoRoutes []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x0C, 0x00};
static ptr_t oid_ipOutNoRoutes                    = {ber_oid_ipOutNoRoutes , 8};

static u8t ber_oid_ipReasmTimeout []              = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x0D, 0x00};
static ptr_t oid_ipReasmTimeout                   = {ber_oid_ipReasmTimeout , 8};

static u8t ber_oid_ipReasmReqds []                = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x0E, 0x00};
static ptr_t oid_ipReasmReqds                     = {ber_oid_ipReasmReqds , 8};

static u8t ber_oid_ipReasmOKs []                  = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x0F, 0x00};
static ptr_t oid_ipReasmOKs                       = {ber_oid_ipReasmOKs , 8};

static u8t ber_oid_ipReasmFails []                = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x10, 0x00};
static ptr_t oid_ipReasmFails                     = {ber_oid_ipReasmFails , 8};

static u8t ber_oid_ipFragOKs []                	  = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x11, 0x00};
static ptr_t oid_ipFragOKs                        = {ber_oid_ipFragOKs , 8};

static u8t ber_oid_ipFragFails []                 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x12, 0x00};
static ptr_t oid_ipFragFails                      = {ber_oid_ipFragFails , 8};

static u8t ber_oid_ipFragCreates []               = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x13, 0x00};
static ptr_t oid_ipFragCreates                    = {ber_oid_ipFragCreates , 8};

//ip AdEntry table
static u8t ber_oid_ipAddrEntry[]				 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x14, 0x01};
static ptr_t oid_ipAddrEntry                     = {ber_oid_ipAddrEntry , 8};

//ip RouteEntry table
static u8t ber_oid_ipRouteEntry[]				 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x15, 0x01};
static ptr_t oid_ipRouteEntry                    = {ber_oid_ipRouteEntry , 8};

static u8t ber_oid_ipNetToMediaEntry[]			 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x16, 0x01};
static ptr_t oid_ipNetToMediaEntry               = {ber_oid_ipNetToMediaEntry , 8};

static u8t ber_oid_ipRoutingDiscards[]			 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x04, 0x17, 0x00};
static ptr_t oid_ipRoutingDiscards               = {ber_oid_ipRoutingDiscards , 8};


//dot 1 bridge
//dot1dBase
static u8t ber_oid_dot1dBase[] 					 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x01, 0x00};
static ptr_t oid_dot1dBase 			             = {ber_oid_dot1dBase  , 8};



//данные переменные закомментированы, т.к. их обработчики ещё не реализованы,
//не удалять
/*static u8t ber_oid_dot1dBasePortEntry[]			 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x01, 0x04, 0x01,0x00};
static ptr_t oid_dot1dBasePortEntry 			 = {ber_oid_dot1dBasePortEntry  , 10};

//dot1dStp
static u8t ber_oid_dot1dStpProtocolSpecification[]		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x01};
static ptr_t oid_dot1dStpProtocolSpecification  		= {ber_oid_dot1dStpProtocolSpecification  , 8};

static u8t ber_oid_dot1dStpPriority[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x02};
static ptr_t oid_dot1dStpPriority 						= {ber_oid_dot1dStpPriority  , 8};

static u8t ber_oid_dot1dStpTimeSinceTopologyChange[]	= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x03};
static ptr_t oid_dot1dStpTimeSinceTopologyChange		= {ber_oid_dot1dStpTimeSinceTopologyChange  , 8};

static u8t ber_oid_dot1dStpTopChanges[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x04};
static ptr_t oid_dot1dStpTopChanges						= {ber_oid_dot1dStpTopChanges  , 8};

static u8t ber_oid_dot1dStpDesignatedRoot[]				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x05};
static ptr_t oid_dot1dStpDesignatedRoot					= {ber_oid_dot1dStpDesignatedRoot  , 8};

static u8t ber_oid_dot1dStpRootCost[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x06};
static ptr_t oid_dot1dStpRootCost						= {ber_oid_dot1dStpRootCost  , 8};

static u8t ber_oid_dot1dStpRootPort[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x07};
static ptr_t oid_dot1dStpRootPort						= {ber_oid_dot1dStpRootPort  , 8};

static u8t ber_oid_dot1dStpMaxAge[]					 	= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x08};
static ptr_t oid_dot1dStpMaxAge							= {ber_oid_dot1dStpMaxAge  , 8};

static u8t ber_oid_dot1dStpHelloTime[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x09};
static ptr_t oid_dot1dStpHelloTime						= {ber_oid_dot1dStpHelloTime  , 8};

static u8t ber_oid_dot1dStpHoldTime[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0A};
static ptr_t oid_dot1dStpHoldTime						= {ber_oid_dot1dStpHoldTime  , 8};

static u8t ber_oid_dot1dStpForwardDelay[]				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0B};
static ptr_t oid_dot1dStpForwardDelay					= {ber_oid_dot1dStpForwardDelay  , 8};

static u8t ber_oid_dot1dStpBridgeMaxAge[]				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0C};
static ptr_t oid_dot1dStpBridgeMaxAge					= {ber_oid_dot1dStpBridgeMaxAge  , 8};

static u8t ber_oid_dot1dStpBridgeHelloTime[]			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0D};
static ptr_t oid_dot1dStpBridgeHelloTime				= {ber_oid_dot1dStpBridgeHelloTime  , 8};

static u8t ber_oid_dot1dStpBridgeForwardDelay[]			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0E};
static ptr_t oid_dot1dStpBridgeForwardDelay				= {ber_oid_dot1dStpBridgeForwardDelay  , 8};

//dot1dStpPortEntry table
static u8t ber_oid_dot1dStpPort[]						= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x01};
static ptr_t oid_dot1dStpPort							= {ber_oid_dot1dStpPort  , 10};

static u8t ber_oid_dot1dStpPortPriority[]				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x02};
static ptr_t oid_dot1dStpPortPriority					= {ber_oid_dot1dStpPortPriority  , 10};

static u8t ber_oid_dot1dStpPortState[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x03};
static ptr_t oid_dot1dStpPortState						= {ber_oid_dot1dStpPortState  , 10};

static u8t ber_oid_dot1dStpPortEnable[]					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x04};
static ptr_t oid_dot1dStpPortEnable						= {ber_oid_dot1dStpPortEnable  , 10};

static u8t ber_oid_dot1dStpPortPathCost[]				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x05};
static ptr_t oid_dot1dStpPortPathCost					= {ber_oid_dot1dStpPortPathCost  , 10};

static u8t ber_oid_dot1dStpPortDesignatedRoot[]			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x06};
static ptr_t oid_dot1dStpPortDesignatedRoot				= {ber_oid_dot1dStpPortDesignatedRoot  , 10};

static u8t ber_oid_dot1dStpPortDesignatedCost[]			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x07};
static ptr_t oid_dot1dStpPortDesignatedCost				= {ber_oid_dot1dStpPortDesignatedCost  , 10};

static u8t ber_oid_dot1dStpPortDesignatedBridge[]	    = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x08};
static ptr_t oid_dot1dStpPortDesignatedBridge		    = {ber_oid_dot1dStpPortDesignatedBridge  , 10};

static u8t ber_oid_dot1dStpPortDesignatedPort[]	    	= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x09};
static ptr_t oid_dot1dStpPortDesignatedPort		    	= {ber_oid_dot1dStpPortDesignatedPort  , 10};

static u8t ber_oid_dot1dStpPortForwardTransitions[]	    = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x02, 0x0F, 0x01, 0x0A};
static ptr_t oid_dot1dStpPortForwardTransitions	    	= {ber_oid_dot1dStpPortForwardTransitions  , 10};

//dot1dTp
static u8t ber_oid_dot1dTpLearnedEntryDiscards[]	    = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x01};
static ptr_t oid_dot1dTpLearnedEntryDiscards	    	= {ber_oid_dot1dTpLearnedEntryDiscards  , 8};

static u8t ber_oid_dot1dTpAgingTime[]	    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x02};
static ptr_t oid_dot1dTpAgingTime	    				= {ber_oid_dot1dTpAgingTime  , 8};

//dot1dTpFdbTable
static u8t ber_oid_dot1dTpFdbAddress[]	    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x03, 0x01, 0x01};
static ptr_t oid_dot1dTpFdbAddress	    				= {ber_oid_dot1dTpFdbAddress  , 10};

static u8t ber_oid_dot1dTpFdbPort[]	    				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x03, 0x01, 0x02};
static ptr_t oid_dot1dTpFdbPort	    					= {ber_oid_dot1dTpFdbPort  , 10};

static u8t ber_oid_dot1dTpFdbStatus[]	    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x03, 0x01, 0x03};
static ptr_t oid_dot1dTpFdbStatus    					= {ber_oid_dot1dTpFdbStatus  , 10};

//dot1dTpPortEntry
static u8t ber_oid_dot1dTpPort[]	    				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x04, 0x01, 0x01};
static ptr_t oid_dot1dTpPort    						= {ber_oid_dot1dTpPort  , 10};

static u8t ber_oid_dot1dTpPortMaxInfo[]	    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x04, 0x01, 0x02};
static ptr_t oid_dot1dTpPortMaxInfo    					= {ber_oid_dot1dTpPortMaxInfo  , 10};

static u8t ber_oid_dot1dTpPortInFrames[]	    		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x04, 0x01, 0x03};
static ptr_t oid_dot1dTpPortInFrames    				= {ber_oid_dot1dTpPortInFrames  , 10};

static u8t ber_oid_dot1dTpPortOutFrames[]	    		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x04, 0x01, 0x04};
static ptr_t oid_dot1dTpPortOutFrames    				= {ber_oid_dot1dTpPortOutFrames  , 10};

static u8t ber_oid_dot1dTpPortInDiscards[]	    		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x04, 0x04, 0x01, 0x05};
static ptr_t oid_dot1dTpPortInDiscards    				= {ber_oid_dot1dTpPortInDiscards  , 10};

//dot1dStaticEntry
static u8t ber_oid_dot1dStaticAddress[]	    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x05, 0x01, 0x01, 0x01};
static ptr_t oid_dot1dStaticAddress    					= {ber_oid_dot1dStaticAddress  , 10};

static u8t ber_oid_dot1dStaticReceivePort[]	    		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x05, 0x01, 0x01, 0x02};
static ptr_t oid_dot1dStaticReceivePort   				= {ber_oid_dot1dStaticReceivePort  , 10};

static u8t ber_oid_dot1dStaticAllowedToGoTo[]	    	= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x05, 0x01, 0x01, 0x03};
static ptr_t oid_dot1dStaticAllowedToGoTo   			= {ber_oid_dot1dStaticAllowedToGoTo  , 10};

static u8t ber_oid_dot1dStaticStatus[]	    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x05, 0x01, 0x01, 0x04};
static ptr_t oid_dot1dStaticStatus  					= {ber_oid_dot1dStaticStatus  , 10};
*/


//Q-Bridge / VLAN
static u8t ber_oid_dot1qVlanVersionNumber[]    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x01, 0x01};
static ptr_t oid_dot1qVlanVersionNumber  				= {ber_oid_dot1qVlanVersionNumber  , 10};

static u8t ber_oid_dot1qMaxVlanId[]    					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x01, 0x02};
static ptr_t oid_dot1qMaxVlanId  						= {ber_oid_dot1qMaxVlanId  , 10};

static u8t ber_oid_dot1qMaxSupportedVlans[]    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x01, 0x03};
static ptr_t oid_dot1qMaxSupportedVlans  				= {ber_oid_dot1qMaxSupportedVlans  , 10};

static u8t ber_oid_dot1qNumVlans[]    					= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x01, 0x04};
static ptr_t oid_dot1qNumVlans  						= {ber_oid_dot1qNumVlans  , 10};

static u8t ber_oid_dot1qGvrpStatus[]    				= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x01, 0x05};
static ptr_t oid_dot1qGvrpStatus 						= {ber_oid_dot1qGvrpStatus  , 11};

//
static u8t ber_oid_dot1qVlanNumDeletes[]    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x01, 0};
static ptr_t oid_dot1qVlanNumDeletes 					= {ber_oid_dot1qVlanNumDeletes  , 11};

//vlan static table
static u8t ber_oid_dot1qVlanStaticTable[]                = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x03, 0x01};
static ptr_t oid_dot1qVlanStaticTable                    = {ber_oid_dot1qVlanStaticTable, 11};


/*
static u8t ber_oid_dot1qVlanStaticName[]    			= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x03, 0x01, 0x01, 0};
static ptr_t oid_dot1qVlanStaticName 					= {ber_oid_dot1qVlanStaticName  , 13};

static u8t ber_oid_dot1qVlanStaticEgressPorts[]    		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x03, 0x01, 0x02, 0};
static ptr_t oid_dot1qVlanStaticEgressPorts 			= {ber_oid_dot1qVlanStaticEgressPorts  , 13};

static u8t ber_oid_dot1qVlanForbiddenEgressPorts[]    	= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x03, 0x01, 0x03, 0};
static ptr_t oid_dot1qVlanForbiddenEgressPorts 			= {ber_oid_dot1qVlanForbiddenEgressPorts  , 13};

static u8t ber_oid_dot1qVlanStaticUntaggedPorts[]    	= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x03, 0x01, 0x04, 0};
static ptr_t oid_dot1qVlanStaticUntaggedPorts 			= {ber_oid_dot1qVlanStaticUntaggedPorts  , 13};

static u8t ber_oid_dot1qVlanStaticRowStatus[]    		= {0x2b, 0x06, 0x01, 0x02, 0x01, 0x11, 0x07, 0x01, 0x04, 0x03, 0x01, 0x05, 0};
static ptr_t oid_dot1qVlanStaticRowStatus 				= {ber_oid_dot1qVlanStaticRowStatus  , 13};
*/



/* SNMP group */
static u8t ber_oid_snmpInPkts[]                  = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x0B, 0x01, 0x00};
static ptr_t oid_snmpInPkts                      = {ber_oid_snmpInPkts, 8};

static u8t ber_oid_snmpInBadVersions[]           = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x0B, 0x03, 0x00};
static ptr_t oid_snmpInBadVersions               = {ber_oid_snmpInBadVersions, 8};

static u8t ber_oid_snmpInASNParseErrs[]          = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x0B, 0x06, 0x00};
static ptr_t oid_snmpInASNParseErrs              = {ber_oid_snmpInASNParseErrs, 8};

static u8t ber_oid_snmpEnableAuthenTraps[]       = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x0B, 0x1E, 0x00};
static ptr_t oid_snmpEnableAuthenTraps           = {ber_oid_snmpEnableAuthenTraps, 8};

static u8t ber_oid_snmpSilentDrops[]             = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x0B, 0x1F, 0x00};
static ptr_t oid_snmpSilentDrops                 = {ber_oid_snmpSilentDrops, 8};

static u8t ber_oid_snmpProxyDrops[]              = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x0B, 0x20, 0x00};
static ptr_t oid_snmpProxyDrops                  = {ber_oid_snmpProxyDrops, 8};

/* ifXTable */
static u8t ber_oid_ifXEntry[]                    = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x1f, 0x01, 0x01, 0x01};
static ptr_t oid_ifXEntry                        = {ber_oid_ifXEntry, 9};

/* entPhysicalEntry */
static u8t ber_oid_entPhyEntry[]                 = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x2f, 0x01, 0x01, 0x01, 0x01};
static ptr_t oid_entPhyEntry                     = {ber_oid_entPhyEntry, 10};

/* entPhySensorEntry */
static u8t ber_oid_entPhySensorEntry[]           = {0x2b, 0x06, 0x01, 0x02, 0x01, 0x63, 0x01, 0x01, 0x01};
static ptr_t oid_entPhySensorEntry               = {ber_oid_entPhySensorEntry, 9};

/* SNMPv2 snmpSet group */
static u8t ber_oid_snmpSetSerialNo[]             = {0x2b, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x06, 0x01, 0x00};
static ptr_t oid_snmpSetSerialNo                 = {ber_oid_snmpSetSerialNo, 10};




//enterprise specific - FORT-TELECOM
static u8t ber_oid_fT_comfortStartTime[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x01, 0x01, 0x01, 0};
ptr_t oid_fT_comfortStartTime     = {ber_oid_fT_comfortStartTime, 14};

static u8t ber_oid_fT_comfortStartEntry[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x01, 0x01, 0x02, 0x01};
ptr_t oid_fT_comfortStartEntry 			  = {ber_oid_fT_comfortStartEntry,14};

static u8t ber_oid_fT_autoReStartEntry[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x01, 0x02, 0x01, 0x01};
ptr_t oid_fT_autoReStartEntry 			  = {ber_oid_fT_autoReStartEntry,14};

static u8t ber_oid_fT_portPoeEntry[] = 	   {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x01, 0x03, 0x01, 0x01};
ptr_t oid_fT_portPoeEntry 			     = {ber_oid_fT_portPoeEntry,14};

//status group
//ups status
static u8t ber_oid_fT_upsModeAvalible[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x01, 0x01,0};
ptr_t oid_fT_upsModeAvalible     = 		  {ber_oid_fT_upsModeAvalible, 14};

static u8t ber_oid_fT_upsPwrSource[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x01, 0x02, 0x00};
ptr_t oid_fT_upsPwrSource     = 		  {ber_oid_fT_upsPwrSource, 14};

static u8t ber_oid_fT_upsBatteryVoltage[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x01, 0x03, 0x00};
ptr_t oid_fT_upsBatteryVoltage     = 		{ber_oid_fT_upsBatteryVoltage, 14};


//input status
static u8t ber_oid_fT_inputStatusEntry[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01};
ptr_t oid_fT_inputStatusEntry 		    =  {ber_oid_fT_inputStatusEntry,14};

//fw version
static u8t ber_oid_fT_fwVersion[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x03, 0x01, 0x00};
ptr_t oid_fT_fwVersion     = 		{ber_oid_fT_fwVersion, 14};

//Energy Meter
static u8t ber_oid_fT_emConnectionStatus[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x01, 0x00};
ptr_t oid_fT_emConnectionStatus     = 		 {ber_oid_fT_emConnectionStatus, 14};

static u8t ber_oid_fT_emResultTotal[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x02, 0x00};
ptr_t oid_fT_emResultTotal     = 		{ber_oid_fT_emResultTotal, 14};

static u8t ber_oid_fT_emResultT1[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x03, 0x00};
ptr_t oid_fT_emResultT1     = 	 	 {ber_oid_fT_emResultT1, 14};

static u8t ber_oid_fT_emResultT2[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x04, 0x00};
ptr_t oid_fT_emResultT2     = 	 	 {ber_oid_fT_emResultT2, 14};

static u8t ber_oid_fT_emResultT3[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x05, 0x00};
ptr_t oid_fT_emResultT3     = 	 	 {ber_oid_fT_emResultT3, 14};

static u8t ber_oid_fT_emResultT4[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x06, 0x00};
ptr_t oid_fT_emResultT4     = 	 	 {ber_oid_fT_emResultT4, 14};

static u8t ber_oid_fT_emPollingInterval[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x04, 0x07, 0x00};
ptr_t oid_fT_emPollingInterval     = 	 	{ber_oid_fT_emPollingInterval, 14};


//PoE Status
static u8t ber_oid_fT_portPoeStatusEntry[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x05, 0x01, 0x01};
ptr_t oid_fT_portPoeStatusEntry 		  =  {ber_oid_fT_portPoeStatusEntry,14};

//Special Function Status
//Auto Restart
static u8t ber_oid_fT_autoRestartErrorsEntry[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x06, 0x01, 0x01, 0x01};
ptr_t oid_fT_autoRestartErrorsEntry 	  =      {ber_oid_fT_autoRestartErrorsEntry,15};

//comfort Start
static u8t ber_oid_fT_comfortStartStatusEntry[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02, 0x02, 0x06, 0x02, 0x01, 0x01};
ptr_t oid_fT_comfortStartStatusEntry 	  =       {ber_oid_fT_comfortStartStatusEntry,15};


//LLDP
static u8t ber_oid_lldpRemEntry[] = 			{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01};
ptr_t oid_lldpRemEntry	  =     				{ber_oid_lldpRemEntry,10};

static u8t ber_oid_lldpMessageTxInterval[] =  	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x01,0x01,0};
ptr_t oid_lldpMessageTxInterval =				{ber_oid_lldpMessageTxInterval,10};

static u8t ber_oid_lldpMessageTxHoldMultiplier[] = 	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x01,0x02,0};
ptr_t oid_lldpMessageTxHoldMultiplier =				{ber_oid_lldpMessageTxHoldMultiplier,10};

static u8t ber_oid_lldpPortConfigEntry[]=		{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x01,0x06,0x01};
ptr_t oid_lldpPortConfigEntry=					{ber_oid_lldpPortConfigEntry,10};

static u8t ber_oid_lldpLocChassisIdSubtype[]=	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x01,0};
ptr_t oid_lldpLocChassisIdSubtype=				{ber_oid_lldpLocChassisIdSubtype,10};

static u8t ber_oid_lldpLocChassisId[]=			{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x02,0};
ptr_t oid_lldpLocChassisId=						{ber_oid_lldpLocChassisId,10};

static u8t ber_oid_lldpLocSysName[]=			{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x03,0};
ptr_t oid_lldpLocSysName=						{ber_oid_lldpLocSysName,10};

static u8t ber_oid_lldpLocSysDesc[]=			{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x04,0};
ptr_t oid_lldpLocSysDesc=						{ber_oid_lldpLocSysDesc,10};

static u8t ber_oid_lldpLocSysCapSupported[]=	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x05,0};
ptr_t oid_lldpLocSysCapSupported=				{ber_oid_lldpLocSysCapSupported,10};

static u8t ber_oid_lldpLocSysCapEnabled[]=		{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x06,0};
ptr_t oid_lldpLocSysCapEnabled=					{ber_oid_lldpLocSysCapEnabled,10};

static u8t ber_oid_lldpLocPortEntry[]=			{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x03,0x07,0x01};
ptr_t oid_lldpLocPortEntry=						{ber_oid_lldpLocPortEntry,10};

/*
static u8t ber_oid_lldpRemChassisIdSubtype[] = {0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x04};
ptr_t oid_lldpRemChassisIdSubtype	  =        {ber_oid_lldpRemChassisIdSubtype,11};

static u8t ber_oid_lldpRemChassisId[] = 	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x05};
ptr_t oid_lldpRemChassisId	  =        		{ber_oid_lldpRemChassisId,11};

static u8t ber_oid_lldpRemPortIdSubtype[] = {0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x06};
ptr_t oid_lldpRemPortIdSubtype	  =        	{ber_oid_lldpRemPortIdSubtype,11};

static u8t ber_oid_lldpRemPortId[] = 		{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x07};
ptr_t oid_lldpRemPortId	  =        			{ber_oid_lldpRemPortId,11};

static u8t ber_oid_lldpRemPortDesc[] 	= 	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x08};
ptr_t oid_lldpRemPortDesc	  =       		{ber_oid_lldpRemPortDesc,11};

static u8t ber_oid_lldpRemSysName[] 	= 	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x09};
ptr_t oid_lldpRemSysName	  =       		{ber_oid_lldpRemSysName,11};

static u8t ber_oid_lldpRemSysDesc[] 	= 	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x0A};
ptr_t oid_lldpRemSysDesc	  =       		{ber_oid_lldpRemSysDesc,11};

static u8t ber_oid_lldpRemSysCapSupported[]={0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x0B};
ptr_t oid_lldpRemSysCapSupported	  =     {ber_oid_lldpRemSysCapSupported,11};

static u8t ber_oid_lldpRemSysCapEnabled[]=	{0x28,0xC4,0x62,0x01,0x01,0x02,0x01,0x04,0x01,0x01,0x0C};
ptr_t oid_lldpRemSysCapEnabled	  =    		{ber_oid_lldpRemSysCapEnabled,11};
*/









/*void http_url_decode(char *in,char *out,uint8_t mx){
  uint8_t i,j;
  char tmp[3]={0,0,0};
  for(i=0,j=0;((j<mx)&&(in[i]));i++)
	{
  	if(in[i]=='%'){
		tmp[0]=in[++i];
		tmp[1]=in[++i];
		out[j++]=(char)strtol(tmp,0,16);
	}else
		out[j++]=in[i];
	}
}*/

ptr_t* handleTableNextOid2(u8t* oid, u8t len, u8t* columns, u8t columnNumber, u8t rowNumber) {
    ptr_t* ret = 0;
    u32t oid_el1, oid_el2;
    u8t i;

    DEBUG_MSG(SNMP_DEBUG,"handleTableNextOid2\r\n");

    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);
    for (i = 0; i < columnNumber; i++) {
        if (oid_el1 < columns[i] || (oid_el1 == columns[i] && oid_el2 < rowNumber)) {
            ret = oid_create();
            CHECK_PTR_U(ret);
            ret->len = 2;
            ret->ptr = pvPortMalloc(2);
            CHECK_PTR_U(ret->ptr);
            ret->ptr[0] = columns[i];
            if (oid_el1 < columns[i]) {
                ret->ptr[1] = 1;
            } else {
                ret->ptr[1] = oid_el2 + 1;
            }
            break;
        }
    }
    return ret;
}

u8t displayCounter=0;

ptr_t* handleTableNextOid2_workAround(u8t* oid, u8t len, u8t* columns, u8t columnNumber, u8t rowNumber) {
    ptr_t* ret = 0;
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    //char text[50];
    DEBUG_MSG(SNMP_DEBUG,"%lu %lu\r\n", oid_el1, oid_el2);

    //raven_lcd_show_text(text);

    for (i = 0; i < columnNumber; i++) {
      if ((oid_el1 < columns[i]) || (oid_el1 == columns[i] && oid_el2 < rowNumber)) {
	    ret = oid_create();
            CHECK_PTR_U(ret);
            ret->len = 2;
            ret->ptr = pvPortMalloc(2);
            CHECK_PTR_U(ret->ptr);
            ret->ptr[0] = columns[i];
	    if (oid_el1 < columns[i]) {
                ret->ptr[1] = 5;
            } else {
	      if(len == 1) {
		displayCounter++;
		DEBUG_MSG(SNMP_DEBUG,"5ing %d\r\n", displayCounter);


                ret->ptr[1] = 5;
	      }
	      else
	        ret->ptr[1] = oid_el2 + 1;
            }
            break;
        }
    }
    return ret;
}

/*
 *  "system" group
 */
s8t getTimeTicks(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = getSysUpTime();
    return 0;
}

/* ----------------   sysORTable    -------------------------- */

#define sysORID         2
#define sysORDescr      3
#define sysORUpTime     4

u8t sysORTableColumns[] = {sysORID, sysORDescr, sysORUpTime};

#define ORTableSize     1

static u8t ber_oid_mib2[]              = {0x2b, 0x06, 0x01, 0x06, 0x03, 0x01};
static ptr_t oid_mib2                  = {ber_oid_mib2, 6};

static u8t ber_oid_jacobs_raven[]      = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x81, 0xf2, 0x06, 0x01, 0x01};
static ptr_t oid_jacobs_raven          = {ber_oid_jacobs_raven, 10};

s8t getOREntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case sysORID: 
            object->varbind.value_type = BER_TYPE_OID;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.p_value.ptr = oid_mib2.ptr;
                    object->varbind.value.p_value.len = oid_mib2.len;
                    break;
                default:
                    return -1;
            }
            break;
        case sysORDescr:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.p_value.ptr = (u8t*)"The MIB module for SNMP entities";
                    object->varbind.value.p_value.len = strlen((char*)object->varbind.value.p_value.ptr);
                    break;
                default:
                    return -1;
            }
            break;
        case sysORUpTime:
            object->varbind.value_type = BER_TYPE_TIME_TICKS;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.u_value = 0;
                    break;
                default:
                    return -1;
            }
            break;
        default:
            return -1;
    }
    return 0;
}

ptr_t* getNextOREntry(mib_object_t* object, u8t* oid, u8t len)
{
    return handleTableNextOid2(oid, len, sysORTableColumns, 3, ORTableSize);
}

/*
 * interfaces group
 */
#define ifIndex             1
#define ifDescr             2
#define ifType              3
#define ifMtu               4
#define ifSpeed             5
#define ifPhysAddress       6
#define ifAdminStatus       7
#define ifOperStatus        8
#define ifLastChange        9
#define ifInOctets          10
#define ifInUcastPkts       11
#define ifInErrors          14
#define ifInUnknownProtos   15
#define ifOutOctets         16
#define ifOutUcastPkts      17
#define ifOutErrors         20
#define ifOutQLen           21
/*
#if CONTIKI_TARGET_AVR_RAVEN
u8t phyAddress[8];

void getPhyAddress(varbind_value_t* value) {
    radio_get_extended_address(phyAddress);
    value->p_value.ptr = phyAddress;
    value->p_value.len = 8;
}

extern uint8_t rf230_last_rssi;
extern uint16_t RF230_sendpackets,RF230_receivepackets,RF230_sendfail,RF230_receivefail;
//extern uint16_t RF230_sendBroadcastPkts, RF230_receiveBroadcastPkts;
extern uint32_t RF230_sendOctets, RF230_receiveOctets;
#define RF230_sendBroadcastPkts 0
#define RF230_receiveBroadcastPkts 0
#else
#define RF230_sendpackets       0
#define RF230_receivepackets    0
#define RF230_sendBroadcastPkts 0
#define RF230_receiveBroadcastPkts 0
#define RF230_sendOctets 0
#define RF230_receiveOctets 0
#define RF230_sendfail 0
#define RF230_receivefail 0
#endif
*/

s8t getIfEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getIfEntry\r\n");
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case ifIndex:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
				object->varbind.value.i_value = oid_el2;
			}else
				return -1;
            break;
        case ifDescr:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
				object->varbind.value.p_value.ptr = (u8t*)getIfName(oid_el2-1);
				object->varbind.value.p_value.len = strlen(getIfName(oid_el2-1));
			}else
				 return -1;

            break;
        case ifType:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
				object->varbind.value.i_value = 6;
			}else
				return -1;
            break;            
        case ifMtu:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
				object->varbind.value.i_value = MAX_ETH_PAYLOAD;
			}
			else
				return -1;
            break;

        case ifSpeed:
            object->varbind.value_type = BER_TYPE_GAUGE;
			if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
				if(dev.port_stat[oid_el2-1].link){
					switch(PortSpeedInfo(L2F_port_conv(oid_el2-1))){
						case 1:object->varbind.value.u_value = 10000000;  break;
						case 2:object->varbind.value.u_value = 100000000; break;
						case 3:object->varbind.value.u_value = 1000000000;break;
					}
				}
				else
					object->varbind.value.u_value = 0;
			else
				return -1;
            break;

	    
        case ifPhysAddress:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
                object->varbind.value.p_value.ptr = (u8t*)dev_addr;
                object->varbind.value.p_value.len = 6;
            }
            else
            	return -1;
            break;
	    

        case ifAdminStatus:
            object->varbind.value_type = BER_TYPE_INTEGER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
            	if(get_port_sett_state(oid_el2-1))
            		object->varbind.value.u_value = SNMP_UP;
            	else
            		object->varbind.value.u_value = SNMP_DOWN;
            }else
            	return -1;
            break;

        case ifOperStatus:
            object->varbind.value_type = BER_TYPE_INTEGER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
            	if(get_port_link(oid_el2-1))
            		object->varbind.value.u_value = SNMP_UP;
            	else
            		object->varbind.value.u_value = SNMP_DOWN;
            }else
            	return -1;
            break;

        case ifLastChange:
            object->varbind.value_type = BER_TYPE_TIME_TICKS;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                 object->varbind.value.u_value = 0;
            else
                 return -1;
            break;

	    
        case ifInOctets:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                 object->varbind.value.u_value = (uint32_t)getReceivedOctets(oid_el2-1);
            else
            	return -1;

            break;

        case ifInUcastPkts:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = getReceivedPackets(oid_el2-1);
            else
            	return -1;
            break;
	    

        case ifInErrors:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                 object->varbind.value.u_value = getFailReceived(oid_el2-1);
            else
                 return -1;
            break;

        case ifInUnknownProtos:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = 0;
            else
                return -1;
            break;

	    
        case ifOutOctets:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = (uint32_t)getSentOctets(oid_el2-1);
            else
                return -1;
            break;
	        
        case ifOutUcastPkts:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = getSentUnicastPackets(oid_el2-1);
            else
                return -1;
            break;
	    

        case ifOutErrors:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                 object->varbind.value.u_value = getFailSent(oid_el2);
            else
                 return -1;
            break;

        case ifOutQLen:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                 object->varbind.value.u_value = 0;
            else
                 return -1;
            break;


        default:
            return -1;
    }
    return 0;
}

u8t ifTableColumns[] = {ifIndex, ifDescr, ifType, ifMtu, ifSpeed, ifPhysAddress, ifAdminStatus, ifOperStatus,
                        ifLastChange, ifInOctets, ifInUcastPkts, ifInErrors, ifInUnknownProtos, ifOutOctets, ifOutUcastPkts, ifOutErrors, ifOutQLen };

//#define ifTableSize     1
u8 ifTableSize;



ptr_t* getNextIfEntry(mib_object_t* object, u8t* oid, u8t len)
{
	ifTableSize = (COOPER_PORT_NUM+FIBER_PORT_NUM);
	DEBUG_MSG(SNMP_DEBUG,"getNextIfEntry \r\n");
    return handleTableNextOid2(oid, len, ifTableColumns, 15, ifTableSize);
}


// at group // not support (null value)

#define atIfIndex 		1
#define atPhysAddress 	2
#define atNetAddress	3

s8t getAtEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getAtEntry\r\n");
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case atIfIndex:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 0;
			break;
        case atPhysAddress:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 0;
			break;
        case atNetAddress:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 0;
            break;

        default:
        	return -1;
    }
    return 0;
}

u8t atTableColumns[] = {atIfIndex, atPhysAddress, atNetAddress};


ptr_t* getNextAtEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextAtEntry \r\n");
    return handleTableNextOid2(oid, len, atTableColumns, 3, 1);
}


/*ip group*/
#define ipForwarding	1
#define ipDefaultTTL	2
#define ipInReceives	3
#define ipInHdrErrors	4
#define ipInAddrError	5
#define ipForwDatagrams	6
#define ipUnknownProtos	7
#define ipIpDiscards	8
#define ipInDelivers	9
#define ipOutRequests	10
#define ipOutDiscards	11
#define ipOutNoRoutes	12
#define ipReasmTimeout	13
#define ipReasmReqds	14
#define ipReasmOKs		15
#define ipReasmFails	16
#define ipFragOKs		17
#define ipFragFails		18
#define ipFragCreates	19



#define ipAdEntAddr			1
#define ipAdEntIfIndex		2
#define ipAdEntNetMask		3
#define ipAdEntBcastAddr	4
#define ipAdEntReasmMaxSize	5



s8t getIpAtEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u8 ip_addr[5];
    static u8 add_oid[16];
    u8 add_len=0;

	DEBUG_MSG(SNMP_DEBUG,"getIpAtEntry len = %d\r\n",len);
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"oid_el1 %lu, oid_el2 = %lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {
        case ipAdEntAddr:
            object->varbind.value_type = BER_TYPE_OCTET_STRING; //BER_TYPE_INTEGER;//BER_TYPE_OCTET_STRING;//BER_TYPE_IPADDRESS;

            if(oid_el2==1){
				ip_addr[0] = uip_ipaddr1(uip_hostaddr);
				ip_addr[1] = uip_ipaddr2(uip_hostaddr);
				ip_addr[2] = uip_ipaddr3(uip_hostaddr);
				ip_addr[3] = uip_ipaddr4(uip_hostaddr);

				add_oid[0] = 0x01;
				add_len = 1;
				for(u8 l=0;l<4;l++){
					if(ip_addr[l]>128){
						add_oid[add_len] = 0x81;
						add_oid[add_len+1] = ip_addr[l]-128;
						add_len+=2;
					}
					else{
						add_oid[add_len] = ip_addr[l];
						add_len++;
					}
				}

				for(u8 i=0;i<add_len;i++)
					object->varbind.oid_ptr->ptr[object->varbind.oid_ptr->len + i] = add_oid[i];
				object->varbind.oid_ptr->len += (add_len);

				print_oid(object->varbind.oid_ptr);

				object->varbind.value.p_value.ptr = (u8 *)ip_addr;
				object->varbind.value.p_value.len = 4;

				DEBUG_MSG(SNMP_DEBUG,"ipAdEntAddr len:%d %d.%d.%d.%d.%d.%d.%d.%d\r\n",add_len,add_oid[0],add_oid[1],add_oid[2],
						add_oid[3],add_oid[4],add_oid[5],add_oid[6],add_oid[7]);
            }
            else
            	return -1;
            break;

        case ipAdEntIfIndex:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=1){
				object->varbind.value.i_value = 123/*oid_el2*/;
			}else
				return -1;
            break;

        case ipAdEntNetMask:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=1){
				object->varbind.value.i_value = oid_el2;
			}else
				return -1;
            break;

        case ipAdEntBcastAddr:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=1){
				object->varbind.value.i_value = oid_el2;
			}else
				return -1;
            break;

        case ipAdEntReasmMaxSize:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=1){
				object->varbind.value.i_value = oid_el2;
			}else
				return -1;
            break;

        default:
        	return -1;
    }
    return 0;
}

u8t ipAtTableColumns[] = {ipAdEntAddr, ipAdEntIfIndex, ipAdEntNetMask, ipAdEntBcastAddr, ipAdEntReasmMaxSize};


ptr_t* getNextIpAtEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextAtEntry \r\n");
    return handleTableNextOid2(oid, len, ipAtTableColumns, 5, 1);
}

//ip route table
#define ipRouteDest			1
#define ipRouteIfIndex		2
#define ipRouteMetric1		3
#define ipRouteMetric2		4
#define ipRouteMetric3		5
#define ipRouteMetric4		6
#define ipRouteNextHop		7
#define ipRouteType			8
#define ipRouteProto		9
#define ipRouteAge			10
#define ipRouteMask			11
#define ipRouteMetric5		12
#define ipRouteInfo			13



s8t getIpRouteEntry(mib_object_t* object, u8t* oid, u8t len)
{

	static u8 ip_addr[4];
	DEBUG_MSG(SNMP_DEBUG,"getIpAtEntry\r\n");
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case ipRouteDest:
            object->varbind.value_type = BER_TYPE_IPADDRESS;
            ip_addr[0] = uip_ipaddr1(uip_hostaddr);
            ip_addr[1] = uip_ipaddr2(uip_hostaddr);
            ip_addr[2] = uip_ipaddr3(uip_hostaddr);
            ip_addr[3] = uip_ipaddr4(uip_hostaddr);


            object->varbind.value.p_value.ptr = (u8 *)ip_addr;
		    object->varbind.value.p_value.len = 4;
            break;


        case ipRouteIfIndex:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = oid_el2;
            break;

        case ipRouteMetric1:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 1;
		    break;

        case ipRouteMetric2:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = -1;
		    break;

        case ipRouteMetric3:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = -1;
		    break;

        case ipRouteMetric4:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = -1;
		    break;
        case ipRouteMetric5:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = -1;
		    break;

        case ipRouteNextHop:
            object->varbind.value_type = BER_TYPE_IPADDRESS;
	     	object->varbind.value.i_value = oid_el2;
	        break;

        case ipRouteType:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 3;//3 - direct
		    break;

        case ipRouteProto:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 2;//2 - local
		    break;

        case ipRouteAge:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 0;
		    break;

        case ipRouteMask:
            object->varbind.value_type = BER_TYPE_IPADDRESS;
			object->varbind.value.i_value = 0;
		    break;


        default:
        	return -1;
    }
    return 0;
}

u8t ipRouteTableColumns[] = {ipRouteDest, ipRouteIfIndex, ipRouteMetric1, ipRouteMetric2, ipRouteMetric3,
		ipRouteMetric4,ipRouteNextHop,ipRouteType,ipRouteProto,ipRouteAge,ipRouteMask,ipRouteMetric5,ipRouteInfo};


ptr_t* getNextIpRouteEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextRouteEntry \r\n");
    return handleTableNextOid2(oid, len, ipRouteTableColumns, 13, 1);
}

//ip net to media table
#define ipNetToMediaIfIndex			1
#define ipNetToMediaPhysAddress		2
#define ipNetToMediaNetAddress		3
#define ipNetToMediaType			4



s8t getIpNetToMediaEntry(mib_object_t* object, u8t* oid, u8t len)
{

	DEBUG_MSG(SNMP_DEBUG,"getIpNetToMediaEntry\r\n");
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case ipNetToMediaIfIndex:
            object->varbind.value_type = BER_TYPE_INTEGER;//BER_TYPE_IPADDRESS;

            break;

        case ipNetToMediaPhysAddress:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
			object->varbind.value.i_value = 0;
	        break;

        case ipNetToMediaNetAddress:
            object->varbind.value_type = BER_TYPE_IPADDRESS;
			object->varbind.value.i_value = oid_el2;
            break;

        case ipNetToMediaType:
            object->varbind.value_type = BER_TYPE_INTEGER;
			object->varbind.value.i_value = 3;//3 - dynamic
		    break;

        default:
        	return -1;
    }
    return 0;
}

u8t ipNetToMediaTableColumns[] = {ipNetToMediaIfIndex, ipNetToMediaPhysAddress, ipNetToMediaNetAddress,ipNetToMediaType};

ptr_t* getNextIpNetToMediaEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextRouteEntry \r\n");
    return handleTableNextOid2(oid, len, ipNetToMediaTableColumns, 4, get_arp_table_size());
}

// 802.1d STP table

#define dot1dBaseBridgeAddress		1
#define dot1dBaseNumPorts			2
#define dot1dBaseType				3

s8t getDot1dBase(mib_object_t* object, u8t* oid, u8t len)
{
    //static u8 ip_addr[5];
    //static u8 add_oid[16];

	DEBUG_MSG(SNMP_DEBUG,"getIpNetToMediaEntry\r\n");
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case dot1dBaseBridgeAddress:
            object->varbind.value_type = BER_TYPE_INTEGER;//BER_TYPE_IPADDRESS;

            break;

        case dot1dBaseNumPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
			object->varbind.value.i_value = 0;
	        break;

        case dot1dBaseType:
            object->varbind.value_type = BER_TYPE_IPADDRESS;
			object->varbind.value.i_value = oid_el2;
            break;

        default:
        	return -1;
    }
    return 0;
}

u8t dot1dBaseTableColumns[] = {dot1dBaseBridgeAddress, dot1dBaseNumPorts, dot1dBaseType};

ptr_t* getNextDot1dBase(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextRouteEntry \r\n");
    return handleTableNextOid2(oid, len, dot1dBaseTableColumns, 3, 1);
}



/*
 * ifXTable group
 */
#define ifName                  1
#define ifInMulticastPkts       2
#define ifInBroadcastPkts       3
#define ifOutMulticastPkts      4
#define ifOutBroadcastPkts      5
#define ifLinkUpDownTrapEnable  14
#define ifHighSpeed             15
#define ifPromiscuousMode       16
#define ifConnectorPresent      17

s8t getIfXEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case ifName:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
				object->varbind.value.p_value.ptr = (u8t*)getIfName(oid_el2-1);
				object->varbind.value.p_value.len = strlen(getIfName(oid_el2-1));
            }else
                return -1;
            break;

        case ifInMulticastPkts:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM)){
            	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

	    
        case ifInBroadcastPkts:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = getInBroadcastPkt(oid_el2 - 1);//RF230_receiveBroadcastPkts;
            else
                return -1;
            break;
	    

        case ifOutMulticastPkts:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = getOutMulticastPkt(oid_el2 - 1);
            else
                return -1;
            break;

        case ifOutBroadcastPkts:
            object->varbind.value_type = BER_TYPE_COUNTER;
            if(oid_el2<=(COOPER_PORT_NUM+FIBER_PORT_NUM))
                object->varbind.value.u_value = getOutBroadkastPkt(oid_el2 - 1);//RF230_sendBroadcastPkts;
            else
                 return -1;
            break;

        case ifLinkUpDownTrapEnable:
            object->varbind.value_type = BER_TYPE_INTEGER;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.i_value = 1; // enabled
                    break;
                default:
                    return -1;
            }
            break;
        case ifHighSpeed:
            object->varbind.value_type = BER_TYPE_GAUGE;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.u_value = 0; // 0 + 50000
                    break;
                default:
                    return -1;
            }
            break;

        case ifPromiscuousMode:
            object->varbind.value_type = BER_TYPE_INTEGER;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.u_value = 2; // false
                    break;
                default:
                    return -1;
            }
            break;

        case ifConnectorPresent:
            object->varbind.value_type = BER_TYPE_INTEGER;
            switch (oid_el2) {
                case 1:
                    object->varbind.value.u_value = 1; // true : 1; false : 0
                    break;
                default:
                    return -1;
            }
            break;

        default:
            return -1;
    }
    return 0;
}



u8t ifXTableColumns[] = {ifName, ifInMulticastPkts, ifInBroadcastPkts, ifOutMulticastPkts, ifOutBroadcastPkts,
                            ifLinkUpDownTrapEnable, ifHighSpeed, ifPromiscuousMode, ifConnectorPresent};

ptr_t* getNextIfXEntry(mib_object_t* object, u8t* oid, u8t len)
{
	ifTableSize = (COOPER_PORT_NUM+FIBER_PORT_NUM);
	return handleTableNextOid2(oid, len, ifXTableColumns, 9, ifTableSize);
}





/*
 * SNMP group
 */
s8t getMIBSnmpInPkts(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = getSnmpInPkts();
    return 0;
}

s8t getMIBSnmpInBadVersions(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = getSnmpInBadVersions();
    return 0;
}

s8t getMIBSnmpInASNParseErrs(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = getSnmpInASNParseErrs();
    return 0;
}

s8t getMIBSnmpSilentDrops(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = getSnmpSilentDrops();
    return 0;
}


//ip table
s8t getIpForvarding(mib_object_t* object, u8t* oid, u8t len){
    object->varbind.value.u_value = getIpForvardingF();
    return 0;
}


static char sysDesc[DESCRIPT_LEN];
static char sysContact[DESCRIPT_LEN];
static char sysName[DESCRIPT_LEN];
static char sysLocation[DESCRIPT_LEN];

/*-----------------------------------------------------------------------------------*/
/*
 * Initialize the MIB.
 */
s8t mib_init()
{
    s32t defaultServiceValue = 78;
    s32t defaultSnmpEnableAuthenTraps = 2;

    s32t ifNumber = (COOPER_PORT_NUM+FIBER_PORT_NUM);

    u32 ttl = UIP_TTL;

    //get name of device
    get_dev_name(sysDesc);

    get_interface_name(sysName);
	http_url_decode(sysName,sysName,strlen(sysName));

    get_interface_location(sysLocation);
    http_url_decode(sysLocation,sysLocation,strlen(sysLocation));

    get_interface_contact(sysContact);
    http_url_decode(sysContact,sysContact,strlen(sysContact));


    //lldp
    if(add_scalar(&oid_lldpMessageTxInterval,FLAG_ACCESS_READONLY,BER_TYPE_INTEGER,0,&getLldpMessageTxInterval,0)==-1||
       add_scalar(&oid_lldpMessageTxHoldMultiplier,FLAG_ACCESS_READONLY,BER_TYPE_INTEGER,0,&getLldpTxHoldMultiplier,0)==-1){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_lldp\r\n");
        return -1;
    }
    if (add_table(&oid_lldpPortConfigEntry, &getLldpPortConfigEntry, &getNextLldpPortConfigEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_lldpPortConfigEntry\r\n");
        return -1;
    }

	if(add_scalar(&oid_lldpLocChassisIdSubtype,FLAG_ACCESS_READONLY,BER_TYPE_INTEGER,0,&getLldLocChassisIdSubtype,0)==-1 ||
	   add_scalar(&oid_lldpLocChassisId,FLAG_ACCESS_READONLY,BER_TYPE_OCTET_STRING,0,&getLldLocChassisId,0)==-1 ||
	   add_scalar(&oid_lldpLocSysName,FLAG_ACCESS_READONLY,BER_TYPE_OCTET_STRING,0,&getLldLocSysName,0)==-1 ||
	   add_scalar(&oid_lldpLocSysDesc,FLAG_ACCESS_READONLY,BER_TYPE_OCTET_STRING,0,&getLldLocSysDesc,0)==-1 ||
	   add_scalar(&oid_lldpLocSysCapSupported,FLAG_ACCESS_READONLY,BER_TYPE_OCTET_STRING,0,&getLldLocSysCapSupported,0)==-1 ||
	   add_scalar(&oid_lldpLocSysCapEnabled,FLAG_ACCESS_READONLY,BER_TYPE_OCTET_STRING,0,&getLldLocSysCapEnabled,0)==-1){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid lldp group\r\n");
        return -1;
    }

    if (add_table(&oid_lldpLocPortEntry, &getLldpLocPortEntry, &getNextLldpLocPortEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_lldpLocPortEntry\r\n");
        return -1;
    }

    if (add_table(&oid_lldpRemEntry, &getLldpRemEntry, &getNextLldpRemEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_lldpRemEntry\r\n");
        return -1;
    }



    // system group
    if (add_scalar(&oid_system_sysDesc, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, sysDesc, 0, 0) == -1 ||
        add_scalar(&oid_system_sysObjectId, FLAG_ACCESS_READONLY, BER_TYPE_OID, &oid_jacobs_raven, 0, 0) == -1 ||
        add_scalar(&oid_system_sysUpTime, FLAG_ACCESS_READONLY, BER_TYPE_TIME_TICKS, 0, &getTimeTicks, 0) == -1 ||
        add_scalar(&oid_system_sysContact, 0, BER_TYPE_OCTET_STRING, sysContact, 0, 0) == -1 ||
        add_scalar(&oid_system_sysName, 0, BER_TYPE_OCTET_STRING, sysName, 0, 0) == -1 ||
        add_scalar(&oid_system_sysLocation, 0, BER_TYPE_OCTET_STRING, sysLocation, 0, 0) == -1 ||
        add_scalar(&oid_system_sysServices, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, &defaultServiceValue, 0, 0) == -1 ||
        add_scalar(&oid_system_sysORLastChange, FLAG_ACCESS_READONLY, BER_TYPE_TIME_TICKS, 0, 0, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error system group\r\n");
    	return -1;
    }
    if (add_table(&oid_system_sysOREntry, &getOREntry, &getNextOREntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_system_sysOREntry\r\n");
    	return -1;
    }


    // interfaces
    if (add_scalar(&oid_ifNumber, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, &ifNumber, 0, 0) != ERR_NO_ERROR) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_ifNumber\r\n");
    	return -1;
    }
    if (add_table(&oid_ifEntry, &getIfEntry, &getNextIfEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_ifEntry\r\n");
    	return -1;
    }

    //at (arp cash) // no support
    if (add_table(&oid_atEntry, &getAtEntry, &getNextAtEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_atEntry\r\n");
    	return -1;
    }

    //ip
    if (add_scalar(&oid_ipForwarding, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0, &getIpForvarding, 0) == -1 ||
        add_scalar(&oid_ipDefaultTTL, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, &ttl, 0, 0) == -1 ||
        add_scalar(&oid_ipInReceives, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipInHdrErrors, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipInAddrError, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipForwDatagrams, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipUnknownProtos, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipIpDiscards, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipInDelivers, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipOutRequests, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipOutDiscards, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipOutNoRoutes, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipReasmTimeout, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipReasmReqds, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipReasmOKs, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipReasmFails, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipFragOKs, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1  ||
        add_scalar(&oid_ipFragFails, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1 ||
        add_scalar(&oid_ipFragCreates, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, 0, 0) == -1
    ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error ip\r\n");
    	return -1;
    }
    if (add_table(&oid_ipAddrEntry, &getIpAtEntry, &getNextIpAtEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_ipAddrEntry\r\n");
    	return -1;
    }
    if (add_table(&oid_ipRouteEntry, &getIpRouteEntry, &getNextIpRouteEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_ipRouteEntry\r\n");
    	return -1;
    }
    if (add_table(&oid_ipNetToMediaEntry, &getIpNetToMediaEntry, &getNextIpNetToMediaEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_ipNetToMediaEntry\r\n");
    	return -1;
    }

    // snmp group
    if (add_scalar(&oid_snmpInPkts, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, &getMIBSnmpInPkts, 0) != ERR_NO_ERROR ||
        add_scalar(&oid_snmpInBadVersions, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, &getMIBSnmpInBadVersions, 0) != ERR_NO_ERROR ||
        add_scalar(&oid_snmpInASNParseErrs, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0, &getMIBSnmpInASNParseErrs, 0) != ERR_NO_ERROR ||
        add_scalar(&oid_snmpEnableAuthenTraps, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, &defaultSnmpEnableAuthenTraps, 0, 0) != ERR_NO_ERROR ||
        add_scalar(&oid_snmpSilentDrops, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0,  &getMIBSnmpSilentDrops, 0) != ERR_NO_ERROR ||
        add_scalar(&oid_snmpProxyDrops, FLAG_ACCESS_READONLY, BER_TYPE_COUNTER, 0,  0, 0) != ERR_NO_ERROR) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error snmp group\r\n");
    	return -1;
    }

    //dot1dBridge
    //dot2dBase
    if (add_table(&oid_dot1dBase, &getDot1dBase, &getNextDot1dBase, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_dot1dBase\r\n");
    	return -1;
    }

    //vlan ctrl group
    if (add_scalar(&oid_dot1qVlanVersionNumber, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0, &getDot1qVlanVersionNumber, 0) != ERR_NO_ERROR ||
    	add_scalar(&oid_dot1qMaxVlanId, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0, &getDot1qMaxVlanId, 0) != ERR_NO_ERROR ||
    	add_scalar(&oid_dot1qMaxSupportedVlans, FLAG_ACCESS_READONLY, BER_TYPE_UNSIGNED32, 0, &getDot1qMaxSupportedVlans, 0) != ERR_NO_ERROR ||
    	add_scalar(&oid_dot1qNumVlans, FLAG_ACCESS_READONLY, BER_TYPE_UNSIGNED32, 0, &getDot1qNumVlans, 0) != ERR_NO_ERROR ||
    	add_scalar(&oid_dot1qGvrpStatus, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0, &getDot1qGvrpStatus, 0) != ERR_NO_ERROR
    ) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error vlan ctrl group\r\n");
    	return -1;
    }

    //dot1qVlanStaticTable
    if (add_table(&oid_dot1qVlanStaticTable, &getDot1qVlanStaticTable, &getNextDot1qVlanStaticTable, &setDot1qVlanStaticTable) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error dot1qVlanStaticTable\r\n");
    	return -1;
    }

    // ifXTable
    if (add_table(&oid_ifXEntry, &getIfXEntry, &getNextIfXEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error ifXTable\r\n");
    	return -1;
    }




    //enterprise specific OID - FORT-TELECOM
    // comfort start
    if (add_scalar(&oid_fT_comfortStartTime, 0, BER_TYPE_INTEGER, 0,&getComfortStartTime, &setComfortStartTime) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error comfort start\r\n");
    	return -1;
    }
    if (add_table(&oid_fT_comfortStartEntry, &getComfortStartEntry, &getNextComfortStartEntry, &setComfortStartEntry) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error comfort start\r\n");
    	return -1;
    }

    //autorestart
    if (add_table(&oid_fT_autoReStartEntry, &getAutoReStartEntry, &getNextAutoReStartEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error autorestart\r\n");
    	return -1;
    }

    //port poe
    if (add_table(&oid_fT_portPoeEntry, &getPortPoeEntry, &getNextPortPoeEntry, &setPortPoeEntry) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error port poe\r\n");
    	return -1;
    }

//Status Grop
    //ups
    if (add_scalar(&oid_fT_upsModeAvalible, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0,&getUpsModeAvalible, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_upsModeAvalible\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_upsPwrSource, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0,&getUpsPwrSource, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_upsPwrSource\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_upsBatteryVoltage,FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0,&getUpsBatteryVoltage, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_upsBatteryVoltage\r\n");
    	return -1;
    }

    //inputs Status
    if (add_table(&oid_fT_inputStatusEntry, &getInputStatusEntry, &getNextInputStatusEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_inputStatusEntry\r\n");
    	return -1;
    }
    //fw vers
    if (add_scalar(&oid_fT_fwVersion, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, 0,&getFwVersion, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_fwVersion\r\n");
    	return -1;
    }

    //energy meter
    if (add_scalar(&oid_fT_emConnectionStatus, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0,&getEmConnectionStatus, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emConnectionStatus\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_emResultTotal, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, 0,&getEmResultTotal, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emResultTotal\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_emResultT1, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, 0,&getEmResultT1, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emResultT1\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_emResultT2, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, 0,&getEmResultT2, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emResultT2\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_emResultT3, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, 0,&getEmResultT3, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emResultT3\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_emResultT4, FLAG_ACCESS_READONLY, BER_TYPE_OCTET_STRING, 0,&getEmResultT4, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emResultT4\r\n");
    	return -1;
    }
    if (add_scalar(&oid_fT_emPollingInterval, FLAG_ACCESS_READONLY, BER_TYPE_INTEGER, 0,&getEmPollingInterval, 0) == -1 ){
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_emPollingInterval\r\n");
    	return -1;
    }

    //poe status
    if (add_table(&oid_fT_portPoeStatusEntry, &getPortPoeStatusEntry, &getNextPortPoeStatusEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_portPoeStatusEntry\r\n");
    	return -1;
    }
    //autorestart errors
    if (add_table(&oid_fT_autoRestartErrorsEntry, &getAutoRestartErrorsEntry, &getNextAutoRestartErrorsEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_autoRestartErrorsEntry\r\n");
    	return -1;
    }
    //Comfort Start Status
    if (add_table(&oid_fT_comfortStartStatusEntry, &getComfortStartStatusEntry, &getNextComfortStartStatusEntry, 0) == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"add table error oid_fT_comfortStartStatusEntry\r\n");
    	return -1;
    }



    //if (add_table(&oid_ifEntry, &getIfEntry, &getNextIfEntry, 0) == -1) {
    //    return -1;
    //}



    return 0;
}
