#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#undef ETH_HandleTxPkt
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_gpio.h"



#include "../uip/uip.h"
#include "../uip/timer.h"
#include "../uip/uip_arp.h"

#if UIP_SPLIT_HACK
#include "net/uip/uip-split.h"
#elif UIP_EMPTY_PACKET_HACK
#include "../uip/uip-empty-packet.h"
#endif 

#include "../command/command.h"
//#include "CommandProcessing.h"

#include "../webserver/httpd.h"
#include "../inc/SMIApi.h"
#include "../inc/VLAN.h"
#include "../inc/FlowCtrl.h"
#include "../inc/QoS.h"
#include "../inc/SpeedDuplex.h"
#include "../stp/bstp.h"
#include "../dhcp/dhcpc.h"
#include "../dhcp/dhcpr.h"
#include "../dhcp/dhcp.h"
#include "../sntp/sntp.h"
#include "../smtp/smtp.h"
#include "../snmp/snmp.h"
#include "../snmp/snmpd/snmpd.h"
#include "../snmp/snmpd/mib-init.h"
#include "../icmp/icmp.h"
#include "../psw_search/search.h"
#include "../syslog/syslog.h"
#include "../syslog/cfg_syslog.h"
#include "../poe/poe_ltc.h"
#include "../igmp/igmpv2.h"
#include "../igmp/igmp_mv.h"
#include "../dns/resolv.h"
#include "../telnet/telnetd.h"
#include "../telnet/usb_shell.h"
#include "../tftp/tftpserver.h"
#include "../tftp/tftpclient.h"
#include "../teleport/teleport.h"
#if LLDP_USE
	#include "../lldp/lldp.h"
#endif
#include "board.h"
#include "settings.h"
#include "debug.h"

//#include "../802.1x/eapol.h"
#include "marvel/inc/gtPortRateCtrl.h"

//static const u16_t broadcast_ipaddr[2] = {0xffff,0xffff};
uint8_t dev_addr[6]={0xC0,0x06,0x11,03,0,0};
uint8_t MyIP[4]={192,168,0,1};
uint8_t MyPORT[2]={0xAB,0xBA};

//extern uint8_t SS;
//extern u8 syslog_len;

//extern xQueueHandle IgmpQueue;

xSemaphoreHandle s_xSemaphore;
xQueueHandle xSettingsQueue;

//extern u8 test_processing_flag;

/* The time to block waiting for input. */
#define emacBLOCK_TIME_WAITING_FOR_INPUT	( ( portTickType ) 1000 )

int ntwk_wait_and_do=0;
struct timer ntwk_timer;

int update_state;
struct timer update_timer;

clock_time_t clock_time( void ){
	return xTaskGetTickCount();
}

static int initialized=0;

void ntwk_setip(void){
  uip_ipaddr_t xIPAddr;

  get_net_ip(&xIPAddr);
  uip_sethostaddr(xIPAddr);

  if((uip_ipaddr1(xIPAddr)==192)&&(uip_ipaddr2(xIPAddr)==168)&&(uip_ipaddr3(xIPAddr)==0)&&(uip_ipaddr4(xIPAddr)==1))
  	  set_led_default(ENABLE);
  else
	  set_led_default(DISABLE);

  get_net_mask(&xIPAddr);
  uip_setnetmask(xIPAddr);

  get_net_gate(&xIPAddr);
  uip_setdraddr(xIPAddr);

  get_net_dns(&xIPAddr);
  uip_ipaddr_copy(uip_dns_addr,xIPAddr);
}

void ntwk_sethaddr(uint8_t *haddr){
  int i;
  struct uip_eth_addr uNet_eth_address;
	for(i=0;i<6;i++) uNet_eth_address.addr[i]=haddr[i];
	    uip_setethaddr(uNet_eth_address);

}

void Uip_Task(void *pvParameters){
  static struct timer periodic_timer, arp_timer,igmp_timer;
  int i;
  //uint32_t status;
  uint16_t len;



  IWDG_ReloadCounter();


  if (initialized==0){



	DEBUG_MSG(UIP_DEBUG,"uip init start\r\n");


    timer_set( &periodic_timer, configTICK_RATE_HZ/2);
    timer_set( &arp_timer,  10*MSEC );//10
    timer_set( &igmp_timer, IGMP_TMR_INTERVAL*MSEC );


    get_net_mac(dev_addr);

    ntwk_sethaddr(dev_addr);

    ETH_MACAddressConfig(ETH_MAC_Address0, &dev_addr[0]);

    uip_init();

    if(get_dhcp_mode() == DHCP_CLIENT){
    	dhcp_client_config();//make xid
    	dhcpc_init(dev_addr,6);//new udp conn
    	dhcpc_request();//set myip to 0
    }
    else{
    	ntwk_setip();
    	httpd_init();
    	command_init();
    	snmp_trap_init();
    	syslog_init();
    	dns_resolv_init();//dns
       	SNTP_config();// NTP
    	psw_search_init();//
    	smtp_config();//email port config
    	e_mail_init();//email task create
    	mib_init();
    	snmpd_init();
    	telnetd_init();
    	teleport_init();

#if DHCPR
    	dhcpr_init();//dhcp relay init
#endif
    }

    if(get_gratuitous_arp_state()){
    	uip_grat_arp();
    	ETH_HandleTxPkt(uip_buf, uip_len);
    }

    initialized = 1;
    DEBUG_MSG(UIP_DEBUG,"uip init ok\r\n");
  }





      while(1){
      uint16_t type;
      struct uip_eth_hdr *xHeader;

		if (update_state!=UPDATE_STATE_IDLE){
			  if (timer_expired(&update_timer)) update_state = UPDATE_STATE_IDLE;
		}


		if (ntwk_wait_and_do){
			if (timer_expired(&ntwk_timer)){
				switch(ntwk_wait_and_do){
					 case 1:
						ntwk_setip();
						ETH_MACAddressConfig(ETH_MAC_Address0, &dev_addr[0]);
						ntwk_sethaddr(&dev_addr[0]);
						smi_allport_discard();
						reboot(REBOOT_MCU);
					   break;
					 case 2:
						smi_allport_discard();
						reboot(REBOOT_MCU);
						break;
					 case 3:
						smi_allport_discard();
						reboot(REBOOT_ALL);
						break;
				}
				ntwk_wait_and_do = 0;
			}
		}

		len=0;
      	if(/*s_xSemaphore != NULL*/1) {
    		if (/*xSemaphoreTake( s_xSemaphore, emacBLOCK_TIME_WAITING_FOR_INPUT)==pdTRUE*/1)
    		{
    			//len = low_level_input(uip_buf);
    			len = ETH_HandleRxPkt(uip_buf);

    	        if (len){
    	        	uip_len = len;
    	            xHeader = (struct uip_eth_hdr *) &uip_buf[ 0 ];
    	            type = xHeader->type;

    	            //for debug purposes
    	            //set_led_default(TOGLE_LED);

  				    xHeader->dsa_tag &= ~HTONL(0xDF07F000);
  				    xHeader->dsa_tag |=  HTONL(0x4100E000);

      	            if( type == htons( UIP_ETHTYPE_IP ) ){
    	              uip_arp_ipin();
    				  uip_input();
    	              if( uip_len > 0 ){
    	                uip_arp_out();
    	#if UIP_SPLIT_HACK
    			uip_split_output();
    	#elif UIP_EMPTY_PACKET_HACK
    					uip_emtpy_packet_output();
    	#else
    	                ETH_HandleTxPkt(uip_buf, uip_len);
    	#endif
    	              }
    	            } else if( type == htons( UIP_ETHTYPE_ARP ) ){
						  uip_arp_arpin();
						  if( uip_len > 0 ){
							 ETH_HandleTxPkt(uip_buf, uip_len);
						  }
    	            }
#if USE_STP
    	            //else if( type == htons( EAPOL_ETHTYPE ) ){
    	            //	printf("in type %x %lx\r\n",htons(type),HTONL(xHeader->dsa_tag));
    	            //	eapol_input();
    	            //}
    	            else{
    	            	extern const u_int8_t bstp_etheraddr[6];
    	            	if (memcmp(bstp_etheraddr, xHeader->dest.addr, 6)==0){
    	            		bstp_rawinput(uip_buf, len);
    	            	}
#if LLDP_USE
    	            	extern const uint8_t lldp_etheraddr[6];
    	            	if (memcmp(lldp_etheraddr, xHeader->dest.addr, 6)==0){
    	            		lldp_rawinput(uip_buf, len);
    	            	}
#endif

    	            }
#endif
    	      }
    		}
      	}

      if (len==0){
        if( ( timer_expired( &periodic_timer ) ) && ( uip_buf != NULL ) ){                          
          timer_reset( &periodic_timer );
          for( i = 0; i < UIP_CONNS; i++ ) {
            uip_periodic( i );
            /* If the above function invocation resulted in data that
            should be sent out on the network, the global variable
            uip_len is set to a value > 0. */
            if( uip_len > 0 ) {
            	uip_arp_out();
#if UIP_SPLIT_HACK
		uip_split_output();
#elif UIP_EMPTY_PACKET_HACK
			  uip_emtpy_packet_output();
#else 
              ETH_HandleTxPkt(uip_buf, uip_len);
#endif 
            }
          }
          for(i = 0; i < UIP_UDP_CONNS; i++) {
    		uip_udp_periodic(i);
			if(uip_len > 0) {
			  if(uip_arp_out()==1){
				  switch(uip_udp_conns[i].rport){
					  case HTONS(SYSLOG_PORT): syslog_ready(); break; // re send syslog
					  case HTONS(SNMP_PORT):
					  case HTONS(SNMP_TRAP_PORT):snmp_ready(); break;//re send snmp
				  }
			  }
			  ETH_HandleTxPkt(uip_buf, uip_len);
			}
          }

          //icmp request
          if(icmp_need_send() == 1){
        	 if(uip_hostaddr[0] != 0 || uip_hostaddr[1]!=0){
				  uip_process(UIP_SEND_ICMP);
				  if(uip_len > 0) {
					  uip_arp_out();//replace to ARP request if no found
					  ETH_HandleTxPkt(uip_buf, uip_len);
				  }
        	 }
          }


		  /* Call the ARP timer function every 10 seconds. */
		  if( timer_expired( &arp_timer ) ) {
			  timer_reset( &arp_timer );
			  uip_arp_timer();
			  if( uip_len > 0 ){
				 ETH_HandleTxPkt(uip_buf, uip_len);
			  }
		  }




        } else {
			/* We did not receive a packet, and there was no periodic
			processing to perform.  Block for a fixed period.  If a packet
			is received during this period we will be woken by the ISR
			giving us the Semaphore. */
			//xSemaphoreTake( xEMACSemaphore, configTICK_RATE_HZ / 2 );
        	vTaskDelay(1);
        }
      }
      break;
    }
}


//udp appcall
void udp_appcall(void){
		  if (uip_udp_conn->lport==(MyPORT[0]|(MyPORT[1]<<8))) cmd_appcall();
		  if (uip_udp_conn->lport==HTONS(SNTP_SERVER_PORT)) sntp_appcall();
		  if (uip_udp_conn->lport==HTONS(SYSLOG_PORT)) syslog_appcall();
		  if ((uip_udp_conn->lport==HTONS(DHCPS_CLIENT_PORT))||(uip_udp_conn->lport==HTONS(DHCPS_SERVER_PORT)))
			    dhcp_appcall();
		  if (uip_udp_conn->lport==HTONS(PSW_PORT)) psw_search_appcall();
		  if(uip_udp_conn->rport == HTONS(DNS_PORT))  resolv_appcall();

		  if(uip_udp_conn->lport==HTONS(SNMP_PORT))
			  snmpd_appcall(); //snmp manage
		  if (uip_udp_conn->rport==HTONS(SNMP_TRAP_PORT))
			  snmp_appcall();//snmp traps
		  if (uip_udp_conn->lport==HTONS(get_tftp_port()) || uip_udp_conn->rport==HTONS(get_tftp_port()))
			  tftpc_appcall();//tftp client
		  //teleport
		  if (uip_udp_conn->rport==HTONS(TLP_UDP_PORT_OUT)){
			  teleport_out_udp_appcall();
		  }
}

//tcp appcall
void dispatch_appcall(void){
		if((uip_conn->rport==HTONS(SMTP_SERVER_PORT))||(uip_conn->rport==HTONS(get_smtp_port())))
			smtp_appcall();
		else if(uip_conn->lport == HTONS(23))
			telnetd_appcall(uip_conn);
		else
			httpd_appcall();
}


/*переинициализация сетевых настроек*/
void network_sett_reinit(u8 flag){
	if(flag)
		initialized = 0;
}

//применение отложенных настроек из очереди
void settings_queue_processing(void){
u8 flag;
u32 esize;
port_sett_t port_sett;

	//обработка очереди для проведения отложенной настройки только после сохранения настроек
	if(need_save_settings())
		return;

	if(xSettingsQueue){
		if(xQueueReceive(xSettingsQueue,&flag,0) == pdPASS ){
			switch(flag){
				case SQ_NETWORK:
					ntwk_setip();
					break;
				case SQ_SYSLOG:
					syslog_init();
					break;

				case SQ_SNTP:
					SNTP_config();
					break;

				case SQ_STP:
					bstp_reinit();
					break;

				case SQ_IGMP:
					igmp_init();
					igmp_task_start();
					//add broadcast to atu table all ports
					add_broadcast_to_atu();
					//sample on fe port
					for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
						igmp_start(i);
					break;

				case SQ_SMTP:
			    	smtp_config();//email port config
			    	e_mail_init();//email task create
					break;

				case SQ_SNMP:
					snmpd_init();
					snmp_trap_init();
					//mib_init();
					break;

				case SQ_VLAN:
					if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
					   (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
						VLAN_setup();
					}
					else if	(get_marvell_id() == DEV_98DX316){
						SWU_VLAN_setup();
						//SWU_pbvlan_setup();
					}
					break;

				case SQ_SS:
					break;

				case SQ_AR:
					break;

				case SQ_DRYCONT:
					break;

				case SQ_USERS:
					httpd_renew_passwd();
					break;

				case SQ_TFTP:
					break;

				case SQ_EVENTS:
					break;

				case SQ_QOS:
					if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
					   (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
						if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
							RateLimitConfig();
						if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
							PSW1G_RateLimitConfig();
						qos_set();
					}
					else if	(get_marvell_id() == DEV_98DX316){
						SWU_qos_set();
					}
					break;

				case SQ_PORT_ALL:
					SwitchPortSet();
					break;

				case SQ_POE:
					set_poe_init(0);
					break;

				case SQ_PBVLAN:
					if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
					   (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
						pbvlan_setup();
					}
					break;

				case SQ_MACFILR:
					//SwitchPortSet();
					set_mac_filtering();
					break;

				case SQ_PORT_1:
				case SQ_PORT_2:
				case SQ_PORT_3:
				case SQ_PORT_4:
				case SQ_PORT_5:
				case SQ_PORT_6:
				case SQ_PORT_7:
				case SQ_PORT_8:
				case SQ_PORT_9:
				case SQ_PORT_10:
				case SQ_PORT_11:
				case SQ_PORT_12:
				case SQ_PORT_13:
				case SQ_PORT_14:
				case SQ_PORT_15:
				case SQ_PORT_16:
				case SQ_PORT_17:
				case SQ_PORT_18:
				case SQ_PORT_19:
				case SQ_PORT_20:
					get_port_config(flag-SQ_PORT_1,&port_sett);
					switch_port_config(&port_sett);
					break;

				case SQ_TELEPORT:
					teleport_init();
					break;

				case SQ_ERASE:
					  spi_flash_properties(NULL,NULL,&esize);
					  for(u32 i=FL_TMP_START;i<=1048576;i+=esize){
						  spi_flash_erase(i,esize);
					      IWDG_ReloadCounter();
					      //printf("erased %lu\r\n",i);
					  }

					  break;

				default:
					return;

			}
		}
	}
}

//добавление в очередь на применение настроек
void settings_add2queue(u8 flag){
u8 tmp;
	DEBUG_MSG(PSWS_DEBUG,"add2queue %d\r\n",flag);

	if(!flag)
		return;

	if(xSettingsQueue){
		if(uxQueueMessagesWaiting(xSettingsQueue)){
			if( xQueuePeek( xSettingsQueue, &tmp, ( portTickType ) 10 ) ){
				if(tmp != flag){
					xQueueSend(xSettingsQueue,&flag,0);
					DEBUG_MSG(PSWS_DEBUG,"settings_add2queue %d\r\n",flag);
				}
			}
		}
		else{
			xQueueSend(xSettingsQueue,&flag,0);
			DEBUG_MSG(PSWS_DEBUG,"settings_add2queue %d\r\n",flag);
		}
	}
}
