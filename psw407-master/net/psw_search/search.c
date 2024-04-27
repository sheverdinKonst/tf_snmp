/*
 * search.c
 *
 *  Created on: 17.03.2014
 *      Author: Alex
 */

#include <stdio.h>
#include <string.h>
#include "../uip/uip.h"
#include "stm32f4xx_rtc.h"
#include "board.h"
#include "settings.h"
#include "search.h"
#include "../snmp/snmpd/md5.h"
#include "../flash/spiflash.h"
#include "../webserver/httpd-cgi.h"
#include "settingsfile.h"
#include "debug.h"





struct uip_udp_conn *search_conn;



extern uip_ipaddr_t uip_hostaddr;
extern u8 dev_addr[6];
extern u32 image_version[1];

//init connection
void psw_search_init(void){

	if(search_conn){
		uip_udp_remove (search_conn);
	}

	search_conn = uip_udp_new(NULL, HTONS(PSW_PORT));
	if (search_conn!=NULL)
    	uip_udp_bind(search_conn, HTONS(PSW_PORT));
}

void psw_search_appcall(void){
	u8 *appdata;
	char text[128];
	MD5_CTX mdContext;
	char A1[128],HA1str[33],login[HTTPD_MAX_LEN_PASSWD],pswd[HTTPD_MAX_LEN_PASSWD],temp[5];
	u8 HA1[16];
	u32 esize;
	struct mac_entry_t entry;
	int start;

    if (uip_newdata()){
      DEBUG_MSG(PSWS_DEBUG,"psw_search_appcall\r\n");
      if (uip_datalen()){

    	  struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];

    	  DEBUG_MSG(PSWS_DEBUG,"input msg %d %d.%d.%d.%d\r\n",HTONS(m->destport),
    			  uip_ipaddr1(m->srcipaddr),uip_ipaddr2(m->srcipaddr),uip_ipaddr3(m->srcipaddr),uip_ipaddr4(m->srcipaddr));


    	  if(((m->srcipaddr[0] & uip_netmask[0]) == (uip_hostaddr[0] & uip_netmask[0])) &&
    		((m->srcipaddr[1] & uip_netmask[1]) == (uip_hostaddr[1] & uip_netmask[1]))) {
    		  DEBUG_MSG(PSWS_DEBUG,"us subnet\r\n");
    	  }
    	  else
    	  {
    		  m->srcipaddr[0] = 255<<8 | 255;
    		  m->srcipaddr[1] = 255<<8 | 255;
    		  DEBUG_MSG(PSWS_DEBUG,"no us subnet\r\n");
    	  }

    	  appdata = uip_appdata;


    	  struct search_out_msg_t *in = (struct search_out_msg_t *)appdata;
    	  DEBUG_MSG(PSWS_DEBUG,"input msg type %d\r\n",in->msg_type);

    	  if(m->srcport == HTONS(PSW_PORT)){
    		  if(in->msg_type == PSW_REQUEST){
    			  in->msg_type = PSW_RESPONSE;
    			  in->dev_type = get_dev_type();
    			  in->ip[0] = uip_ipaddr1(&uip_hostaddr);
    			  in->ip[1] = uip_ipaddr2(&uip_hostaddr);
    			  in->ip[2] = uip_ipaddr3(&uip_hostaddr);
    			  in->ip[3] = uip_ipaddr4(&uip_hostaddr);
    			  for(u8 i=0;i<6;i++){
    				  in->mac[i] = dev_addr[i];
    			  }
    			  get_interface_name(text);
    			  strcpy(in->dev_descr,text);

    			  get_interface_location(text);
    			  strcpy(in->dev_loc,text);

    			  in->uptime = RTC_GetCounter();

    			  in->firmware = image_version[0];

    			  in->mask[0] = uip_ipaddr1(&uip_netmask);
				  in->mask[1] = uip_ipaddr2(&uip_netmask);
				  in->mask[2] = uip_ipaddr3(&uip_netmask);
				  in->mask[3] = uip_ipaddr4(&uip_netmask);

    			  in->gate[0] = uip_ipaddr1(&uip_draddr);
				  in->gate[1] = uip_ipaddr2(&uip_draddr);
				  in->gate[2] = uip_ipaddr3(&uip_draddr);
				  in->gate[3] = uip_ipaddr4(&uip_draddr);


				  for(u8 i=0;i<PORT_NUM;i++){
					  in->mac_entry[i].mac[0]=\
					  in->mac_entry[i].mac[1]=\
					  in->mac_entry[i].mac[2]=\
					  in->mac_entry[i].mac[3]=\
					  in->mac_entry[i].mac[4]=\
					  in->mac_entry[i].mac[5]=0;

					  //для всех PoE портов
					  if(is_cooper(i) && (get_dev_type()!=DEV_SWU16)){
						  start = 1;
						  while(read_atu(0, start, &entry)==0){
							 start = 0;
							 if(!(entry.port_vect & (1<<L2F_port_conv(i))))
								continue;

							in->mac_entry[i].mac[0]=entry.mac[1];
							in->mac_entry[i].mac[1]=entry.mac[0];
							in->mac_entry[i].mac[2]=entry.mac[3];
							in->mac_entry[i].mac[3]=entry.mac[2];
							in->mac_entry[i].mac[4]=entry.mac[5];
							in->mac_entry[i].mac[5]=entry.mac[4];

							for(u8 j=0;j<UIP_ARPTAB_SIZE;j++){



								if(arp_table[j].ethaddr.addr[0]==in->mac_entry[i].mac[0] &&
								   arp_table[j].ethaddr.addr[1]==in->mac_entry[i].mac[1] &&
								   arp_table[j].ethaddr.addr[2]==in->mac_entry[i].mac[2] &&
								   arp_table[j].ethaddr.addr[3]==in->mac_entry[i].mac[3] &&
								   arp_table[j].ethaddr.addr[4]==in->mac_entry[i].mac[4] &&
								   arp_table[j].ethaddr.addr[5]==in->mac_entry[i].mac[5]){

								   DEBUG_MSG(PSWS_DEBUG,"search found (%d) %x:%x:%x:%x:%x:%x\r\n",i,
											arp_table[j].ethaddr.addr[0],
											arp_table[j].ethaddr.addr[1],
											arp_table[j].ethaddr.addr[2],
											arp_table[j].ethaddr.addr[3],
											arp_table[j].ethaddr.addr[4],
											arp_table[j].ethaddr.addr[5]);



									in->mac_entry[i].ip[0]= (u8)(arp_table[j].ipaddr[0]);
									in->mac_entry[i].ip[1]= (u8)(arp_table[j].ipaddr[0] >> 8);
									in->mac_entry[i].ip[2]= (u8)(arp_table[j].ipaddr[1] & 0xFF);
									in->mac_entry[i].ip[3]= (u8)(arp_table[j].ipaddr[1] >> 8);

									DEBUG_MSG(PSWS_DEBUG,"%d.%d.%d.%d\r\n",
									   in->mac_entry[i].ip[0],
									   in->mac_entry[i].ip[1],
									   in->mac_entry[i].ip[2],
									   in->mac_entry[i].ip[3]);
									break;
								}
								else{
									in->mac_entry[i].ip[0] = in->mac_entry[i].ip[1] = \
									in->mac_entry[i].ip[2]= in->mac_entry[i].ip[3] = 0;
								}
							}
							break;
						 }
					  }
				  }

       			  uip_udp_send(uip_len);
    		  }
    		  else if(in->msg_type == PSW_SETTINGS_REQUEST){
    			  struct search_sett_msg_t *in = (struct search_sett_msg_t *)appdata;

    			  //проверяем свой MAC
    			  if(dev_addr[0]==in->mac[0] && dev_addr[1]==in->mac[1] &&
    				 dev_addr[2]==in->mac[2] && dev_addr[3]==in->mac[3] &&
    				 dev_addr[4]==in->mac[4] && dev_addr[5]==in->mac[5]){
    				  in->msg_type = PSW_SETTINGS_RESPONSE;
    				  //проверяем MD5
    				  get_interface_users_username(0,login);
    				  get_interface_users_password(0,pswd);
    				  sprintf(A1,"%02X%02X%02X%02X%02X%02X+%s+%s",dev_addr[0],dev_addr[1],
    						  dev_addr[2],dev_addr[3],dev_addr[4],dev_addr[5], login,pswd);
    				  DEBUG_MSG(PSWS_DEBUG,"A1: %s\r\n",A1);
    				  MD5Init(&mdContext);
    				  MD5Update(&mdContext,(u8 *)A1,strlen(A1));
    				  MD5Final(&mdContext,HA1);
    				  DEBUG_MSG(PSWS_DEBUG,"HA1: %s\r\n",HA1);

    				  HA1str[0]=0;
    				  for(u8 i=0;i<16;i++){
    					  sprintf(temp,"%02x",HA1[i]);
    					  strcat(HA1str,temp);
    				  }
    				  HA1str[32]=0;

    				  DEBUG_MSG(PSWS_DEBUG,"HA1str: %s\r\n",HA1str);
    				  DEBUG_MSG(PSWS_DEBUG,"in->pass_md5: %s\r\n",in->pass_md5);

    				  if(strncmp(in->pass_md5,HA1str,32)==0){
    					  DEBUG_MSG(PSWS_DEBUG,"Auth OK!\r\n");
    					  DEBUG_MSG(PSWS_DEBUG,"in->buff len: %d\r\n",1024);
						  spi_flash_properties(NULL,NULL,&esize);
						  spi_flash_erase(FL_TMP_START,esize);
						  spi_flash_write(FL_TMP_START, 1024, in->buff);
						  DEBUG_MSG(PSWS_DEBUG,"in->buff: %s\r\n",in->buff);
						  set_from_search(1);
    					  if(parse_bak_file()==0){
    						  DEBUG_MSG(PSWS_DEBUG,"Settings OK!\r\n");
    						  settings_save();
    						 // send_events_u32(EVENT_SET_SEARCH,0);
							  in->pass_md5[0] = NO_ERROR;
							  uip_udp_send(uip_len);
							 //reboot(REBOOT_MCU_5S);
    					  }
    					  else{
    						  DEBUG_MSG(PSWS_DEBUG,"Settings Error!\r\n");
							  in->pass_md5[0] = ERROR_FILE;
							  uip_udp_send(uip_len);
    					  }
    					  set_from_search(0);
    				  }
    				  else{
    					  in->pass_md5[0] = ERROR_AUTH;
    					  uip_udp_send(uip_len);
    				  }

    			  }

    		  }
    	  }
      }
    }
}
