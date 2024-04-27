#include <string.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "board.h"
#include "names.h"
#include "settings.h"
#include "../net/uip/psock.h"
#include "../net/webserver/httpd.h"
#include "../net/webserver/httpd-cgi.h"
#include "settingsfile.h"
#include "spiflash.h"
#include "blbx.h"
#include "stm32f4xx_iwdg.h"
#include "../i2c/soft/i2c_soft.h"
#include "debug.h"

u32 backup_file_len;


#define ADD2FLASH if(strlen(str)>1024){\
					spi_flash_write(offset,strlen(str),str);\
					offset+=strlen(str);\
					memset(str,0,sizeof(str));\
				  }


#define IF_EQUAL(X) if(strcmp(var_name,X)==0)

#define ADD_COMMENT(comment) addstr("------");addstr(comment);addstr("\r\n");DEBUG_MSG(SETTINGS_DBG,"%s\r\n",comment);ADD2FLASH

#define ADD_U8(var_name,x) sprintf(temp,"#%s=[%d]\r\n",var_name,x);addstr(temp);ADD2FLASH

#define ADD_U16(var_name,x) sprintf(temp,"#%s=[%d]\r\n",var_name,x);addstr(temp);ADD2FLASH

#define ADD_U32(var_name,x) sprintf(temp,"#%s=[%lu]\r\n",var_name,x);addstr(temp);ADD2FLASH

#define ADD_STR(var_name,x) sprintf(temp,"#%s=[%s]\r\n",var_name,x);addstr(temp);ADD2FLASH

#define ADD_IP(var_name,x) sprintf(temp,"#%s=[%d.%d.%d.%d]\r\n", \
	var_name,uip_ipaddr1(x),uip_ipaddr2(x),uip_ipaddr3(x),uip_ipaddr4(x));addstr(temp);ADD2FLASH

#define ADD_MAC(var_name,x) sprintf(temp,"#%s=[%x:%x:%x:%x:%x:%x]\r\n", var_name,x[0],x[1],x[2],x[3],x[4],x[5]);addstr(temp);ADD2FLASH

#define ADD_I8(var_name,x) ADD_U8(var_name,x);ADD2FLASH

#define ADD_ARR(var_name,x,len) sprintf(temp,"#%s=[",var_name); \
								for(u8 i=0;i<len;i++){			\
									sprintf(q,"[%d]",x[i]);		\
									strcat(temp,q);				\
								}								\
								sprintf(q,"]\r\n");strcat(temp,q); \
								addstr(temp);			\
								ADD2FLASH


#define ADD_VLANREC(var_name,i){sprintf(temp,"#%s=[",var_name); 							  	\
								sprintf(q,"[%d]",get_vlan_state(i));strcat(temp,q); 	\
								sprintf(q,"[%d]",get_vlan_vid(i));strcat(temp,q); 		\
								sprintf(q,"[%s]",get_vlan_name(i));strcat(temp,q); 				\
								for(u8 j=0;j<ALL_PORT_NUM;j++){							\
									sprintf(q,"[%d]",get_vlan_port_state(i,j));strcat(temp,q);\
								}													\
								strcat(temp,"]\r\n");addstr(temp);						\
								}													\
								ADD2FLASH


#define ADD_STPREC(var_name,i) {sprintf(temp,"#%s=[",var_name);									\
								sprintf(q,"[%d]",get_stp_port_enable(i));strcat(temp,q); 		\
								sprintf(q,"[%d]",get_stp_port_state(i));strcat(temp,q); 		\
								sprintf(q,"[%d]",get_stp_port_priority(i));strcat(temp,q); 		\
								sprintf(q,"[%lu]",get_stp_port_cost(i));strcat(temp,q); 		\
								sprintf(q,"[%d]",get_stp_port_flags(i));strcat(temp,q); 		\
								strcat(temp,"]\r\n");addstr(temp);								\
								}\
								ADD2FLASH



#define IF_CMP(x) if(strcmp(var_name,x)==0)

extern char str[ETH_MAX_PACKET_SIZE+MAX_STR_LEN];

HTTPD_CGI_CALL(cgi_make_bak,  		"make_bak",			run_make_bak);
HTTPD_CGI_CALL(cgi_make_log,  		"make_log",			run_make_log);
HTTPD_CGI_CALL(cgi_sfp_dump,  		"sfp_dump",			run_sfp_dump);

static const struct httpd_cgi_call *calls_bak[] = {
	&cgi_make_bak,
	&cgi_make_log,
	&cgi_sfp_dump,
    NULL
};

static PT_THREAD(nullfunction(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  PSOCK_SEND_STR(&s->sout,"Not Implemented");
  PSOCK_END(&s->sout);
}

//for settings backup make
httpd_cgifunction bak_cgi(char *name){
  const struct httpd_cgi_call **f;

  /* Find the matching name in the table, return the function. */
  for(f = calls_bak; *f != NULL; ++f) {
    if(strncmp((*f)->name, name, strlen((*f)->name)) == 0) {
      return (*f)->function;
    }
  }
  return nullfunction;
}

/*
u8 send_to_tftp;
static u8 to_tftp(void){
	return send_to_tftp;
}
void make_log_test(void){
	struct httpd_state s;
	send_to_tftp = 1;
	DEBUG_MSG(TFTP_DEBUG,"make_log_test %d\r\n",run_make_test(&s,NULL));
}
static void to_flash_init(void){
	DEBUG_MSG(TFTP_DEBUG,"to_flash_init\r\n");
}

static void to_flash_write(char *text,u16 len){
	DEBUG_MSG(TFTP_DEBUG,"to_flash_write %d\r\n",len);
}
*/

static u8 is_sett;

static u8 is_from_search(){
	return is_sett;
}

void set_from_search(u8 state){
	is_sett = state;
}


/*******************************************************************************************************/
/*       save mcu log as txt file 															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_make_log(struct httpd_state *s, char *ptr)){
static  uint32_t m = 0;//m - номер записи
static  uint16_t len1;
static u8 exit = 0;
char temp[128];
	  str[0] = 0;
	  PSOCK_BEGIN(&s->sout);


	  for(m=0;;m++){
		len1=BB_MSG_LEN;
		memset(temp,0,sizeof(temp));
		if(m==0){
			sprintf(temp,"%lu:   ",(u32)m);
			addstr(temp);
			read_bb_first(1,0x0F,(u8 *)temp,&len1);
			addstr(temp);

		}else{
			sprintf(temp,"%lu:   ",(u32)m);
			addstr(temp);
			if(read_bb_next(1,0x0F,(u8 *)temp,&len1)){
				exit=1;
				addstr("[eof]");
			}
			else{
				addstr(temp);
			}
		}
		addstr("\r\n");
		if(m%10==0){
		  IWDG_ReloadCounter();
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		}
		if(exit){
		  IWDG_ReloadCounter();
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  exit=0;
		  break;
		}
	  }
	  PSOCK_END(&s->sout);
}

/*******************************************************************************************************/
/*       save mcu log to flash														    	*/
/* return value - total len	*/
/*******************************************************************************************************/
u32 make_log(void){
char temp[128];
uint32_t m=0;//m - номер записи
uint16_t len1;
u8 exit = 0;
u32 offset = 0;
u32 esize;
	  str[0] = 0;
	  //erase all temp place  - 1MB
	  spi_flash_properties(NULL,NULL,&esize);
	  for(u8 i=0;i<(FL_TMP_END/(esize*2));i++){
		  spi_flash_erase(FL_TMP_START+esize*i,esize);
	      IWDG_ReloadCounter();
	  }
	  while(1){
		  sprintf(temp,"%lu:   ",(u32)m);
		  addstr(temp);
		  len1=BB_MSG_LEN;
		  memset(temp,0,sizeof(temp));
		  if(m==0){
			  read_bb_first(1,0x0F,(u8 *)temp,&len1);
			  addstr(temp);
		  }else{
			  if(read_bb_next(1,0x0F,(u8 *)temp,&len1)){
					exit=1;
					addstr("[eof]");
			  }
			  else{
				  temp[len1]=0;
				  addstr(temp);
			  }
		  }
		  m++;
		  addstr("\r\n");
		  if(m%10==0){
			  ////////////////////////////////////////////
			  spi_flash_write(offset,strlen(str),str);
			  offset+=strlen(str);
			  str[0] = 0;
			  ////////////////////////////////////////////
		  }
		  if(exit){
			  exit=0;
			  ////////////////////////////////////////////
			  spi_flash_write(offset,strlen(str),str);
			  offset+=strlen(str);
			  str[0] = 0;
			  ////////////////////////////////////////////
			  m = 0;
			  break;
		  }

		  if(m>100000){
			  m = 0;
			  break;
		  }
	  }
	  m = 0;
	  exit=0;
	  return offset;
}




/*******************************************************************************************************/
/*       MAKE BACKUP FILE (SAVED SETTINGS) 				 													    	*/
/*******************************************************************************************************/


static PT_THREAD(run_make_bak(struct httpd_state *s, char *ptr)){

static u32 offset;

	  str[0] = 0;
	  PSOCK_BEGIN(&s->sout);

	  while(1){
		  memset(str,0,sizeof(str));
		  spi_flash_read(offset,BLOCK_SIZE,str);
		  PSOCK_SEND_STR(&s->sout,str);

		  offset+=BLOCK_SIZE;

		  if(offset>=backup_file_len)
			  break;
	  }

	  offset = 0;
	  PSOCK_END(&s->sout);
}
/*******************************************************************************************************/
/*       MAKE BACKUP FILE (SAVED SETTINGS)	сохраняем файл на флеш - предварительная подготовка												    	*/
/*******************************************************************************************************/
u32 make_bak(void){
char temp[256];
u8 tmp[256];
char q[256];
u8 i;
u32 temp_u32;

uip_ipaddr_t ip;
	  str[0] = 0;
u32 offset = 0;
u32 esize;


	  //erase temp place //64K
	  spi_flash_properties(NULL,NULL,&esize);
	  spi_flash_erase(FL_TMP_START,esize);

	  offset = FL_TMP_START;

	  ADD_COMMENT("Network settings");

	  get_net_ip(&ip);
	  ADD_IP(IPADDRESS_NAME,ip);

	  get_net_mask(&ip);
	  ADD_IP(NETMASK_NAME,ip);

	  get_net_gate(&ip);
	  ADD_IP(GATEWAY_NAME,ip);

	  get_net_mac(tmp);
	  ADD_MAC(USER_MAC_NAME,tmp);

	  get_dhcp_server_addr(&ip);
	  ADD_IP(DHCPRELAYIP_NAME,ip);

	  get_net_dns(&ip);
	  ADD_IP(DNS_NAME,ip);

	  ADD_U8(DHCPMODE_NAME,get_dhcp_mode())

	  ADD_U8(DHCP_MAX_HOPS_NAME,get_dhcp_hops());

	  ADD_U8(DHCP_OPT82_NAME,get_dhcp_opt82());

	  ADD_COMMENT("Interface settings");

	  ADD_U8(LANG_NAME,get_interface_lang());

	  //get_interface_login(q);
	  get_interface_users_username(0,q);
	  ADD_STR(HTTP_USERNAME_NAME,q);

	  //get_interface_passwd(q);
	  get_interface_users_password(0,q);
	  ADD_STR(HTTP_PASSWD_NAME,q);

	  get_interface_users_username(1,q);
	  ADD_STR(USER1_USERNAME_NAME,q);
	  get_interface_users_password(1,q);
	  ADD_STR(USER1_PASSWD_NAME,q);
	  ADD_U8(USER1_RULE_NAME,get_interface_users_rule(1));

	  get_interface_users_username(2,q);
	  ADD_STR(USER2_USERNAME_NAME,q);
	  get_interface_users_password(2,q);
	  ADD_STR(USER2_PASSWD_NAME,q);
	  ADD_U8(USER2_RULE_NAME,get_interface_users_rule(2));

	  get_interface_users_username(3,q);
	  ADD_STR(USER3_USERNAME_NAME,q);
	  get_interface_users_password(3,q);
	  ADD_STR(USER3_PASSWD_NAME,q);
	  ADD_U8(USER3_RULE_NAME,get_interface_users_rule(3));

	  get_interface_name(q);
	  ADD_STR(SYSTEM_NAME_NAME,q);

	  get_interface_location(q);
	  ADD_STR(SYSTEM_LOCATION_NAME,q);

	  get_interface_contact(q);
	  ADD_STR(SYSTEM_CONTACT_NAME,q);

	  get_port_descr(0,q);
	  ADD_STR(PORT1_DESCR_NAME,q);
	  get_port_descr(1,q);
	  ADD_STR(PORT2_DESCR_NAME,q);
	  get_port_descr(2,q);
	  ADD_STR(PORT3_DESCR_NAME,q);
	  get_port_descr(3,q);
	  ADD_STR(PORT4_DESCR_NAME,q);
	  get_port_descr(4,q);
	  ADD_STR(PORT5_DESCR_NAME,q);
	  get_port_descr(5,q);
	  ADD_STR(PORT6_DESCR_NAME,q);
	  get_port_descr(6,q);
	  ADD_STR(PORT7_DESCR_NAME,q);
	  get_port_descr(7,q);
	  ADD_STR(PORT8_DESCR_NAME,q);
	  get_port_descr(8,q);
	  ADD_STR(PORT9_DESCR_NAME,q);
	  get_port_descr(9,q);
	  ADD_STR(PORT10_DESCR_NAME,q);
	  get_port_descr(10,q);
	  ADD_STR(PORT11_DESCR_NAME,q);
	  get_port_descr(11,q);
	  ADD_STR(PORT12_DESCR_NAME,q);
	  get_port_descr(12,q);
	  ADD_STR(PORT13_DESCR_NAME,q);
	  get_port_descr(13,q);
	  ADD_STR(PORT14_DESCR_NAME,q);
	  get_port_descr(14,q);
	  ADD_STR(PORT15_DESCR_NAME,q);
	  get_port_descr(15,q);
	  ADD_STR(PORT16_DESCR_NAME,q);


	  ADD_COMMENT("Port settings");

	  ADD_U8(PORT1_STATE_NAME,get_port_sett_state(0));
	  ADD_U8(PORT2_STATE_NAME,get_port_sett_state(1));
	  ADD_U8(PORT3_STATE_NAME,get_port_sett_state(2));
	  ADD_U8(PORT4_STATE_NAME,get_port_sett_state(3));
	  ADD_U8(PORT5_STATE_NAME,get_port_sett_state(4));
	  ADD_U8(PORT6_STATE_NAME,get_port_sett_state(5));
	  ADD_U8(PORT7_STATE_NAME,get_port_sett_state(6));
	  ADD_U8(PORT8_STATE_NAME,get_port_sett_state(7));
	  ADD_U8(PORT9_STATE_NAME,get_port_sett_state(8));
	  ADD_U8(PORT10_STATE_NAME,get_port_sett_state(9));
	  ADD_U8(PORT11_STATE_NAME,get_port_sett_state(10));
	  ADD_U8(PORT12_STATE_NAME,get_port_sett_state(11));
	  ADD_U8(PORT13_STATE_NAME,get_port_sett_state(12));
	  ADD_U8(PORT14_STATE_NAME,get_port_sett_state(13));
	  ADD_U8(PORT15_STATE_NAME,get_port_sett_state(14));
	  ADD_U8(PORT16_STATE_NAME,get_port_sett_state(15));

	  ADD_U8(PORT1_SPEEDDPLX_NAME,get_port_sett_speed_dplx(0));
	  ADD_U8(PORT2_SPEEDDPLX_NAME,get_port_sett_speed_dplx(1));
	  ADD_U8(PORT3_SPEEDDPLX_NAME,get_port_sett_speed_dplx(2));
	  ADD_U8(PORT4_SPEEDDPLX_NAME,get_port_sett_speed_dplx(3));
	  ADD_U8(PORT5_SPEEDDPLX_NAME,get_port_sett_speed_dplx(4));
	  ADD_U8(PORT6_SPEEDDPLX_NAME,get_port_sett_speed_dplx(5));
	  ADD_U8(PORT7_SPEEDDPLX_NAME,get_port_sett_speed_dplx(6));
	  ADD_U8(PORT8_SPEEDDPLX_NAME,get_port_sett_speed_dplx(7));
	  ADD_U8(PORT9_SPEEDDPLX_NAME,get_port_sett_speed_dplx(8));
	  ADD_U8(PORT10_SPEEDDPLX_NAME,get_port_sett_speed_dplx(9));
	  ADD_U8(PORT11_SPEEDDPLX_NAME,get_port_sett_speed_dplx(10));
	  ADD_U8(PORT12_SPEEDDPLX_NAME,get_port_sett_speed_dplx(11));
	  ADD_U8(PORT13_SPEEDDPLX_NAME,get_port_sett_speed_dplx(12));
	  ADD_U8(PORT14_SPEEDDPLX_NAME,get_port_sett_speed_dplx(13));
	  ADD_U8(PORT15_SPEEDDPLX_NAME,get_port_sett_speed_dplx(14));
	  ADD_U8(PORT16_SPEEDDPLX_NAME,get_port_sett_speed_dplx(15));

	  ADD_U8(PORT1_FLOWCTRL_NAME,get_port_sett_flow(0));
	  ADD_U8(PORT2_FLOWCTRL_NAME,get_port_sett_flow(1));
	  ADD_U8(PORT3_FLOWCTRL_NAME,get_port_sett_flow(2));
	  ADD_U8(PORT4_FLOWCTRL_NAME,get_port_sett_flow(3));
	  ADD_U8(PORT5_FLOWCTRL_NAME,get_port_sett_flow(4));
	  ADD_U8(PORT6_FLOWCTRL_NAME,get_port_sett_flow(5));
	  ADD_U8(PORT7_FLOWCTRL_NAME,get_port_sett_flow(6));
	  ADD_U8(PORT8_FLOWCTRL_NAME,get_port_sett_flow(7));
	  ADD_U8(PORT9_FLOWCTRL_NAME,get_port_sett_flow(8));
	  ADD_U8(PORT10_FLOWCTRL_NAME,get_port_sett_flow(9));
	  ADD_U8(PORT11_FLOWCTRL_NAME,get_port_sett_flow(10));
	  ADD_U8(PORT12_FLOWCTRL_NAME,get_port_sett_flow(11));
	  ADD_U8(PORT13_FLOWCTRL_NAME,get_port_sett_flow(12));
	  ADD_U8(PORT14_FLOWCTRL_NAME,get_port_sett_flow(13));
	  ADD_U8(PORT15_FLOWCTRL_NAME,get_port_sett_flow(14));
	  ADD_U8(PORT16_FLOWCTRL_NAME,get_port_sett_flow(15));

	  ADD_U8(PORT1_WDT_NAME,get_port_sett_wdt(0));
	  ADD_U8(PORT2_WDT_NAME,get_port_sett_wdt(1));
	  ADD_U8(PORT3_WDT_NAME,get_port_sett_wdt(2));
	  ADD_U8(PORT4_WDT_NAME,get_port_sett_wdt(3));
	  ADD_U8(PORT5_WDT_NAME,get_port_sett_wdt(4));
	  ADD_U8(PORT6_WDT_NAME,get_port_sett_wdt(5));
	  ADD_U8(PORT7_WDT_NAME,get_port_sett_wdt(6));
	  ADD_U8(PORT8_WDT_NAME,get_port_sett_wdt(7));
	  ADD_U8(PORT9_WDT_NAME,get_port_sett_wdt(8));
	  ADD_U8(PORT10_WDT_NAME,get_port_sett_wdt(9));
	  ADD_U8(PORT11_WDT_NAME,get_port_sett_wdt(10));
	  ADD_U8(PORT12_WDT_NAME,get_port_sett_wdt(11));
	  ADD_U8(PORT13_WDT_NAME,get_port_sett_wdt(12));
	  ADD_U8(PORT14_WDT_NAME,get_port_sett_wdt(13));
	  ADD_U8(PORT15_WDT_NAME,get_port_sett_wdt(14));
	  ADD_U8(PORT16_WDT_NAME,get_port_sett_wdt(15));


	  get_port_sett_wdt_ip(0,&ip);
	  ADD_IP(PORT1_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(1,&ip);
	  ADD_IP(PORT2_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(2,&ip);
	  ADD_IP(PORT3_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(3,&ip);
	  ADD_IP(PORT4_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(4,&ip);
	  ADD_IP(PORT5_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(5,&ip);
	  ADD_IP(PORT6_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(6,&ip);
	  ADD_IP(PORT7_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(7,&ip);
	  ADD_IP(PORT8_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(8,&ip);
	  ADD_IP(PORT9_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(9,&ip);
	  ADD_IP(PORT10_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(10,&ip);
	  ADD_IP(PORT11_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(11,&ip);
	  ADD_IP(PORT12_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(12,&ip);
	  ADD_IP(PORT13_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(13,&ip);
	  ADD_IP(PORT14_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(14,&ip);
	  ADD_IP(PORT15_IPADDR_NAME,ip);
	  get_port_sett_wdt_ip(15,&ip);
	  ADD_IP(PORT16_IPADDR_NAME,ip);

	  ADD_U16(PORT1_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(0));
	  ADD_U16(PORT2_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(1));
	  ADD_U16(PORT3_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(2));
	  ADD_U16(PORT4_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(3));
	  ADD_U16(PORT5_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(4));
	  ADD_U16(PORT6_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(5));
	  ADD_U16(PORT7_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(6));
	  ADD_U16(PORT8_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(7));
	  ADD_U16(PORT9_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(8));
	  ADD_U16(PORT10_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(9));
	  ADD_U16(PORT11_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(10));
	  ADD_U16(PORT12_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(11));
	  ADD_U16(PORT13_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(12));
	  ADD_U16(PORT14_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(13));
	  ADD_U16(PORT15_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(14));
	  ADD_U16(PORT16_WDT_SPEED_NAME,get_port_sett_wdt_speed_down(15));


	  ADD_U16(PORT1_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(0));
	  ADD_U16(PORT2_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(1));
	  ADD_U16(PORT3_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(2));
	  ADD_U16(PORT4_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(3));
	  ADD_U16(PORT5_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(4));
	  ADD_U16(PORT6_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(5));
	  ADD_U16(PORT7_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(6));
	  ADD_U16(PORT8_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(7));
	  ADD_U16(PORT9_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(8));
	  ADD_U16(PORT10_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(9));
	  ADD_U16(PORT11_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(10));
	  ADD_U16(PORT12_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(11));
	  ADD_U16(PORT13_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(12));
	  ADD_U16(PORT14_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(13));
	  ADD_U16(PORT15_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(14));
	  ADD_U16(PORT16_WDT_SPEED_UP_NAME,get_port_sett_wdt_speed_up(15));


	  ADD_U8(PORT_SOFTSTART_TIME_NAME,get_softstart_time());

	  ADD_U8(PORT1_SOFTSTART_NAME,get_port_sett_soft_start(0));
	  ADD_U8(PORT2_SOFTSTART_NAME,get_port_sett_soft_start(1));
	  ADD_U8(PORT3_SOFTSTART_NAME,get_port_sett_soft_start(2));
	  ADD_U8(PORT4_SOFTSTART_NAME,get_port_sett_soft_start(3));
	  ADD_U8(PORT5_SOFTSTART_NAME,get_port_sett_soft_start(4));
	  ADD_U8(PORT6_SOFTSTART_NAME,get_port_sett_soft_start(5));
	  ADD_U8(PORT7_SOFTSTART_NAME,get_port_sett_soft_start(6));
	  ADD_U8(PORT8_SOFTSTART_NAME,get_port_sett_soft_start(7));
	  ADD_U8(PORT9_SOFTSTART_NAME,get_port_sett_soft_start(8));
	  ADD_U8(PORT10_SOFTSTART_NAME,get_port_sett_soft_start(9));
	  ADD_U8(PORT11_SOFTSTART_NAME,get_port_sett_soft_start(10));
	  ADD_U8(PORT12_SOFTSTART_NAME,get_port_sett_soft_start(11));
	  ADD_U8(PORT13_SOFTSTART_NAME,get_port_sett_soft_start(12));
	  ADD_U8(PORT14_SOFTSTART_NAME,get_port_sett_soft_start(13));
	  ADD_U8(PORT15_SOFTSTART_NAME,get_port_sett_soft_start(14));
	  ADD_U8(PORT16_SOFTSTART_NAME,get_port_sett_soft_start(15));



	  ADD_U8(PORT1_POE_NAME,(u16)(get_port_sett_poe_b(0)<<8 | get_port_sett_poe(0)));
	  ADD_U8(PORT2_POE_NAME,(u16)(get_port_sett_poe_b(1)<<8 | get_port_sett_poe(1)));
	  ADD_U8(PORT3_POE_NAME,(u16)(get_port_sett_poe_b(2)<<8 | get_port_sett_poe(2)));
	  ADD_U8(PORT4_POE_NAME,(u16)(get_port_sett_poe_b(3)<<8 | get_port_sett_poe(3)));
	  ADD_U8(PORT5_POE_NAME,(u16)(get_port_sett_poe_b(4)<<8 | get_port_sett_poe(4)));
	  ADD_U8(PORT6_POE_NAME,(u16)(get_port_sett_poe_b(5)<<8 | get_port_sett_poe(5)));
	  ADD_U8(PORT7_POE_NAME,(u16)(get_port_sett_poe_b(6)<<8 | get_port_sett_poe(6)));
	  ADD_U8(PORT8_POE_NAME,(u16)(get_port_sett_poe_b(7)<<8 | get_port_sett_poe(7)));
	  ADD_U8(PORT9_POE_NAME,(u16)(get_port_sett_poe_b(8)<<8 | get_port_sett_poe(8)));
	  ADD_U8(PORT10_POE_NAME,(u16)(get_port_sett_poe_b(9)<<8 | get_port_sett_poe(9)));
	  ADD_U8(PORT11_POE_NAME,(u16)(get_port_sett_poe_b(10)<<8 | get_port_sett_poe(10)));
	  ADD_U8(PORT12_POE_NAME,(u16)(get_port_sett_poe_b(11)<<8 | get_port_sett_poe(11)));
	  ADD_U8(PORT13_POE_NAME,(u16)(get_port_sett_poe_b(12)<<8 | get_port_sett_poe(12)));
	  ADD_U8(PORT14_POE_NAME,(u16)(get_port_sett_poe_b(13)<<8 | get_port_sett_poe(13)));
	  ADD_U8(PORT15_POE_NAME,(u16)(get_port_sett_poe_b(14)<<8 | get_port_sett_poe(14)));
	  ADD_U8(PORT16_POE_NAME,(u16)(get_port_sett_poe_b(15)<<8 | get_port_sett_poe(15)));

	  ADD_U8(PORT1_POE_LIM_NAME,get_port_sett_pwr_lim_a(0));
	  ADD_U8(PORT2_POE_LIM_NAME,get_port_sett_pwr_lim_a(1));
	  ADD_U8(PORT3_POE_LIM_NAME,get_port_sett_pwr_lim_a(2));
	  ADD_U8(PORT4_POE_LIM_NAME,get_port_sett_pwr_lim_a(3));
	  ADD_U8(PORT5_POE_LIM_NAME,get_port_sett_pwr_lim_a(4));
	  ADD_U8(PORT6_POE_LIM_NAME,get_port_sett_pwr_lim_a(5));
	  ADD_U8(PORT7_POE_LIM_NAME,get_port_sett_pwr_lim_a(6));
	  ADD_U8(PORT8_POE_LIM_NAME,get_port_sett_pwr_lim_a(7));
	  ADD_U8(PORT9_POE_LIM_NAME,get_port_sett_pwr_lim_a(8));
	  ADD_U8(PORT10_POE_LIM_NAME,get_port_sett_pwr_lim_a(9));
	  ADD_U8(PORT11_POE_LIM_NAME,get_port_sett_pwr_lim_a(10));
	  ADD_U8(PORT12_POE_LIM_NAME,get_port_sett_pwr_lim_a(11));
	  ADD_U8(PORT13_POE_LIM_NAME,get_port_sett_pwr_lim_a(12));
	  ADD_U8(PORT14_POE_LIM_NAME,get_port_sett_pwr_lim_a(13));
	  ADD_U8(PORT15_POE_LIM_NAME,get_port_sett_pwr_lim_a(14));
	  ADD_U8(PORT16_POE_LIM_NAME,get_port_sett_pwr_lim_a(15));

	  ADD_U8(SFP1_MODE_NAME,get_port_sett_sfp_mode(GE1));
	  ADD_U8(SFP2_MODE_NAME,get_port_sett_sfp_mode(GE2));


	  ADD_COMMENT("SMTP settings");

	  ADD_U8(SMTP_STATE_NAME,get_smtp_state());

	  get_smtp_server(&ip);
	  ADD_IP(SMTP_SERV_IP_NAME,ip);

	  get_smtp_to(q);
	  ADD_STR(SMTP_TO1_NAME,q);

	  get_smtp_to2(q);
	  ADD_STR(SMTP_TO2_NAME,q);

	  get_smtp_to3(q);
	  ADD_STR(SMTP_TO3_NAME,q);

	  get_smtp_from(q);
	  ADD_STR(SMTP_FROM_NAME,q);

	  get_smtp_subj(q);
	  ADD_STR(SMTP_SUBJ_NAME,q);

	  get_smtp_login(q);
	  ADD_STR(SMTP_LOGIN_NAME,q);

	  get_smtp_pass(q);
	  ADD_STR(SMTP_PASS_NAME,q);

	  ADD_U8(SMTP_PORT_NAME,get_smtp_port());

	  get_smtp_domain(q);
	  ADD_STR(SMTP_DOMAIN_NAME_NAME,q);

	  ADD_COMMENT("SNTP settings");

	  ADD_U8(SNTP_STATE_NAME,get_sntp_state());

	  get_sntp_serv(&ip);
	  ADD_IP(SNTP_SETT_SERV_NAME,ip);

	  get_sntp_serv_name(q);
	  ADD_STR(SNTP_SERV_NAME_NAME,q);


	  ADD_I8(SNTP_TIMEZONE_NAME,get_sntp_timezone());

	  ADD_U8(SNTP_PERIOD_NAME,get_sntp_period());

	  ADD_COMMENT("SYSLOG settings");

	  ADD_U8(SYSLOG_STATE_NAME,get_syslog_state());

	  get_syslog_serv(&ip);
	  ADD_IP(SYSLOG_SERV_IP_NAME,ip);



	  ADD_COMMENT("Events List settings");

	  ADD_U8(EVENT_LIST_BASE_S_NAME,(get_event_base_s_st()<<3 | get_event_base_s_level()));

	  ADD_U8(EVENT_LIST_PORT_S_NAME,(get_event_port_s_st()<<3 | get_event_port_s_level()));

	  ADD_U8(EVENT_LIST_VLAN_S_NAME,(get_event_vlan_s_st()<<3 | get_event_vlan_s_level()));

	  ADD_U8(EVENT_LIST_STP_S_NAME,(get_event_stp_s_st()<<3 | get_event_stp_s_level()));

	  ADD_U8(EVENT_LIST_QOS_S_NAME,(get_event_qos_s_st()<<3 | get_event_qos_s_level()));

	  ADD_U8(EVENT_LIST_OTHER_S_NAME,(get_event_other_s_st()<<3 | get_event_other_s_level()));

	  ADD_U8(EVENT_LIST_LINK_T_NAME,(get_event_port_link_t_st()<<3 | get_event_port_link_t_level()));

	  ADD_U8(EVENT_LIST_POE_T_NAME,(get_event_port_poe_t_st()<<3 | get_event_port_poe_t_level()));

	  ADD_U8(EVENT_LIST_STP_T_NAME,(get_event_port_stp_t_st()<<3 | get_event_port_stp_t_level()));

	  ADD_U8(EVENT_LIST_ARLINK_T_NAME,(get_event_port_spec_link_t_st()<<3 | get_event_port_spec_link_t_level()));

	  ADD_U8(EVENT_LIST_ARPING_T_NAME,(get_event_port_spec_ping_t_st()<<3 | get_event_port_spec_ping_t_level()));

	  ADD_U8(EVENT_LIST_ARSPEED_T_NAME,(get_event_port_spec_speed_t_st()<<3 | get_event_port_spec_speed_t_level()));

	  ADD_U8(EVENT_LIST_SYSTEM_T_NAME,(get_event_port_system_t_st()<<3 | get_event_port_system_t_level()));

	  ADD_U8(EVENT_LIST_UPS_T_NAME,(get_event_port_ups_t_st()<<3 | get_event_port_ups_t_level()));

	  ADD_U8(EVENT_LIST_ACCESS_T_NAME,(get_event_port_alarm_t_st()<<3 | get_event_port_alarm_t_level()));

	  ADD_U8(EVENT_LIST_MAC_T_NAME,(get_event_port_mac_t_st()<<3 | get_event_port_mac_t_level()));


	  ADD_COMMENT("Dry contact settings");

	  ADD_U8(DRY_CONT0_STATE_NAME,get_alarm_state(0));

	  ADD_U8(DRY_CONT1_STATE_NAME,get_alarm_state(1));
	  ADD_U8(DRY_CONT1_LEVEL_NAME,get_alarm_front(1));

	  ADD_U8(DRY_CONT2_STATE_NAME,get_alarm_state(2));
	  ADD_U8(DRY_CONT2_LEVEL_NAME,get_alarm_front(2));

	  ADD_COMMENT("QoS settings");

	  ADD_U32(PORT1_RATE_LIMIT_RX_NAME,get_rate_limit_rx(0));
	  ADD_U32(PORT2_RATE_LIMIT_RX_NAME,get_rate_limit_rx(1));
	  ADD_U32(PORT3_RATE_LIMIT_RX_NAME,get_rate_limit_rx(2));
	  ADD_U32(PORT4_RATE_LIMIT_RX_NAME,get_rate_limit_rx(3));
	  ADD_U32(PORT5_RATE_LIMIT_RX_NAME,get_rate_limit_rx(4));
	  ADD_U32(PORT6_RATE_LIMIT_RX_NAME,get_rate_limit_rx(5));
	  ADD_U32(PORT7_RATE_LIMIT_RX_NAME,get_rate_limit_rx(6));
	  ADD_U32(PORT8_RATE_LIMIT_RX_NAME,get_rate_limit_rx(7));
	  ADD_U32(PORT9_RATE_LIMIT_RX_NAME,get_rate_limit_rx(8));
	  ADD_U32(PORT10_RATE_LIMIT_RX_NAME,get_rate_limit_rx(9));
	  ADD_U32(PORT11_RATE_LIMIT_RX_NAME,get_rate_limit_rx(10));
	  ADD_U32(PORT12_RATE_LIMIT_RX_NAME,get_rate_limit_rx(11));
	  ADD_U32(PORT13_RATE_LIMIT_RX_NAME,get_rate_limit_rx(12));
	  ADD_U32(PORT14_RATE_LIMIT_RX_NAME,get_rate_limit_rx(13));
	  ADD_U32(PORT15_RATE_LIMIT_RX_NAME,get_rate_limit_rx(14));
	  ADD_U32(PORT16_RATE_LIMIT_RX_NAME,get_rate_limit_rx(15));


	  ADD_U32(PORT1_RATE_LIMIT_TX_NAME,get_rate_limit_tx(0));
	  ADD_U32(PORT2_RATE_LIMIT_TX_NAME,get_rate_limit_tx(1));
	  ADD_U32(PORT3_RATE_LIMIT_TX_NAME,get_rate_limit_tx(2));
	  ADD_U32(PORT4_RATE_LIMIT_TX_NAME,get_rate_limit_tx(3));
	  ADD_U32(PORT5_RATE_LIMIT_TX_NAME,get_rate_limit_tx(4));
	  ADD_U32(PORT6_RATE_LIMIT_TX_NAME,get_rate_limit_tx(5));
	  ADD_U32(PORT7_RATE_LIMIT_TX_NAME,get_rate_limit_tx(6));
	  ADD_U32(PORT8_RATE_LIMIT_TX_NAME,get_rate_limit_tx(7));
	  ADD_U32(PORT9_RATE_LIMIT_TX_NAME,get_rate_limit_tx(8));
	  ADD_U32(PORT10_RATE_LIMIT_TX_NAME,get_rate_limit_tx(9));
	  ADD_U32(PORT11_RATE_LIMIT_TX_NAME,get_rate_limit_tx(10));
	  ADD_U32(PORT12_RATE_LIMIT_TX_NAME,get_rate_limit_tx(11));
	  ADD_U32(PORT13_RATE_LIMIT_TX_NAME,get_rate_limit_tx(12));
	  ADD_U32(PORT14_RATE_LIMIT_TX_NAME,get_rate_limit_tx(13));
	  ADD_U32(PORT15_RATE_LIMIT_TX_NAME,get_rate_limit_tx(14));
	  ADD_U32(PORT16_RATE_LIMIT_TX_NAME,get_rate_limit_tx(15));

	  ADD_U8(QOS_STATE_NAME,get_qos_state());

	  ADD_U8(QOS_POLICY_NAME,get_qos_policy());

	  for(u8 i=0;i<8;i++){
		  tmp[i] = get_qos_cos_queue(i);
	  }
	  ADD_ARR(QOS_COS_NAME,tmp,8);

	  for(u8 i=0;i<64;i++){
		  tmp[i] = get_qos_tos_queue(i);
	  }
	  ADD_ARR(QOS_TOS_NAME,tmp,64);


	  ADD_U8(PORT1_COS_STATE_NAME,get_qos_port_cos_state(0));
	  ADD_U8(PORT2_COS_STATE_NAME,get_qos_port_cos_state(1));
	  ADD_U8(PORT3_COS_STATE_NAME,get_qos_port_cos_state(2));
	  ADD_U8(PORT4_COS_STATE_NAME,get_qos_port_cos_state(3));
	  ADD_U8(PORT5_COS_STATE_NAME,get_qos_port_cos_state(4));
	  ADD_U8(PORT6_COS_STATE_NAME,get_qos_port_cos_state(5));
	  ADD_U8(PORT7_COS_STATE_NAME,get_qos_port_cos_state(6));
	  ADD_U8(PORT8_COS_STATE_NAME,get_qos_port_cos_state(7));
	  ADD_U8(PORT9_COS_STATE_NAME,get_qos_port_cos_state(8));
	  ADD_U8(PORT10_COS_STATE_NAME,get_qos_port_cos_state(9));
	  ADD_U8(PORT11_COS_STATE_NAME,get_qos_port_cos_state(10));
	  ADD_U8(PORT12_COS_STATE_NAME,get_qos_port_cos_state(11));
	  ADD_U8(PORT13_COS_STATE_NAME,get_qos_port_cos_state(12));
	  ADD_U8(PORT14_COS_STATE_NAME,get_qos_port_cos_state(13));
	  ADD_U8(PORT15_COS_STATE_NAME,get_qos_port_cos_state(14));
	  ADD_U8(PORT16_COS_STATE_NAME,get_qos_port_cos_state(15));


	  ADD_U8(PORT1_TOS_STATE_NAME,get_qos_port_tos_state(0));
	  ADD_U8(PORT2_TOS_STATE_NAME,get_qos_port_tos_state(1));
	  ADD_U8(PORT3_TOS_STATE_NAME,get_qos_port_tos_state(2));
	  ADD_U8(PORT4_TOS_STATE_NAME,get_qos_port_tos_state(3));
	  ADD_U8(PORT5_TOS_STATE_NAME,get_qos_port_tos_state(4));
	  ADD_U8(PORT6_TOS_STATE_NAME,get_qos_port_tos_state(5));
	  ADD_U8(PORT7_TOS_STATE_NAME,get_qos_port_tos_state(6));
	  ADD_U8(PORT8_TOS_STATE_NAME,get_qos_port_tos_state(7));
	  ADD_U8(PORT9_TOS_STATE_NAME,get_qos_port_tos_state(8));
	  ADD_U8(PORT10_TOS_STATE_NAME,get_qos_port_tos_state(9));
	  ADD_U8(PORT11_TOS_STATE_NAME,get_qos_port_tos_state(10));
	  ADD_U8(PORT12_TOS_STATE_NAME,get_qos_port_tos_state(11));
	  ADD_U8(PORT13_TOS_STATE_NAME,get_qos_port_tos_state(12));
	  ADD_U8(PORT14_TOS_STATE_NAME,get_qos_port_tos_state(13));
	  ADD_U8(PORT15_TOS_STATE_NAME,get_qos_port_tos_state(14));
	  ADD_U8(PORT16_TOS_STATE_NAME,get_qos_port_tos_state(15));


	  ADD_U8(PORT1_QOS_RULE_NAME,get_qos_port_rule(0));
	  ADD_U8(PORT2_QOS_RULE_NAME,get_qos_port_rule(1));
	  ADD_U8(PORT3_QOS_RULE_NAME,get_qos_port_rule(2));
	  ADD_U8(PORT4_QOS_RULE_NAME,get_qos_port_rule(3));
	  ADD_U8(PORT5_QOS_RULE_NAME,get_qos_port_rule(4));
	  ADD_U8(PORT6_QOS_RULE_NAME,get_qos_port_rule(5));
	  ADD_U8(PORT7_QOS_RULE_NAME,get_qos_port_rule(6));
	  ADD_U8(PORT8_QOS_RULE_NAME,get_qos_port_rule(7));
	  ADD_U8(PORT9_QOS_RULE_NAME,get_qos_port_rule(8));
	  ADD_U8(PORT10_QOS_RULE_NAME,get_qos_port_rule(9));
	  ADD_U8(PORT11_QOS_RULE_NAME,get_qos_port_rule(10));
	  ADD_U8(PORT12_QOS_RULE_NAME,get_qos_port_rule(11));
	  ADD_U8(PORT13_QOS_RULE_NAME,get_qos_port_rule(12));
	  ADD_U8(PORT14_QOS_RULE_NAME,get_qos_port_rule(13));
	  ADD_U8(PORT15_QOS_RULE_NAME,get_qos_port_rule(14));
	  ADD_U8(PORT16_QOS_RULE_NAME,get_qos_port_rule(15));

	  ADD_U8(PORT1_COS_PRI_NAME,get_qos_port_def_pri(0));
	  ADD_U8(PORT2_COS_PRI_NAME,get_qos_port_def_pri(1));
	  ADD_U8(PORT3_COS_PRI_NAME,get_qos_port_def_pri(2));
	  ADD_U8(PORT4_COS_PRI_NAME,get_qos_port_def_pri(3));
	  ADD_U8(PORT5_COS_PRI_NAME,get_qos_port_def_pri(4));
	  ADD_U8(PORT6_COS_PRI_NAME,get_qos_port_def_pri(5));
	  ADD_U8(PORT7_COS_PRI_NAME,get_qos_port_def_pri(6));
	  ADD_U8(PORT8_COS_PRI_NAME,get_qos_port_def_pri(7));
	  ADD_U8(PORT9_COS_PRI_NAME,get_qos_port_def_pri(8));
	  ADD_U8(PORT10_COS_PRI_NAME,get_qos_port_def_pri(9));
	  ADD_U8(PORT11_COS_PRI_NAME,get_qos_port_def_pri(10));
	  ADD_U8(PORT12_COS_PRI_NAME,get_qos_port_def_pri(11));
	  ADD_U8(PORT13_COS_PRI_NAME,get_qos_port_def_pri(12));
	  ADD_U8(PORT14_COS_PRI_NAME,get_qos_port_def_pri(13));
	  ADD_U8(PORT15_COS_PRI_NAME,get_qos_port_def_pri(14));
	  ADD_U8(PORT16_COS_PRI_NAME,get_qos_port_def_pri(15));


	  ADD_COMMENT("Port Based VLAN settings");
	  ADD_U8(PBVLAN_STATE_NAME,get_pb_vlan_state());

	  temp_u32  =0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(0,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT1_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(1,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT2_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(2,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT3_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(3,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT4_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(4,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT5_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(5,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT6_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(6,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT7_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(7,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT8_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(8,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT9_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(9,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT10_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(10,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT11_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(11,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT12_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(12,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT13_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(13,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT14_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(14,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT15_NAME,temp_u32);

	  temp_u32 = 0;
	  for(u8 i=0;i<(ALL_PORT_NUM);i++){
		  temp_u32 |= get_pb_vlan_port(15,i) << i;
	  }
	  ADD_U32(PBVLAN_PORT16_NAME,temp_u32);

	  ADD_COMMENT("PortBased VLAN SWU settings");
	  ADD_U8(PORT1_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(0));
	  ADD_U8(PORT2_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(1));
	  ADD_U8(PORT3_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(2));
	  ADD_U8(PORT4_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(3));
	  ADD_U8(PORT5_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(4));
	  ADD_U8(PORT6_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(5));
	  ADD_U8(PORT7_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(6));
	  ADD_U8(PORT8_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(7));
	  ADD_U8(PORT9_PB_VLAN_SWU_NAME,  get_pb_vlan_swu_port(8));
	  ADD_U8(PORT10_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(9));
	  ADD_U8(PORT11_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(10));
	  ADD_U8(PORT12_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(11));
	  ADD_U8(PORT13_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(12));
	  ADD_U8(PORT14_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(13));
	  ADD_U8(PORT15_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(14));
	  ADD_U8(PORT16_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(15));
	  ADD_U8(PORT17_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(16));
	  ADD_U8(PORT18_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(17));
	  ADD_U8(PORT19_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(18));
	  ADD_U8(PORT20_PB_VLAN_SWU_NAME, get_pb_vlan_swu_port(19));

	  ADD_COMMENT("VLAN settings");

	  ADD_U8(VLAN_MVID_NAME,get_vlan_sett_mngt());

	  ADD_U8(VLAN_NUM_NAME,get_vlan_sett_vlannum());

	  for(i=0;i<get_vlan_sett_vlannum();i++){
		  switch(i){
		  	  case 0:ADD_VLANREC(VLAN1_NAME,0);break;
		  	  case 1:ADD_VLANREC(VLAN2_NAME,1);break;
		  	  case 2:ADD_VLANREC(VLAN3_NAME,2);break;
		  	  case 3:ADD_VLANREC(VLAN4_NAME,3);break;
		  	  case 4: ADD_VLANREC(VLAN5_NAME,4);break;
		  	  case 5:ADD_VLANREC(VLAN6_NAME,5);break;
		  	  case 6:ADD_VLANREC(VLAN7_NAME,6);break;
		  	  case 7:ADD_VLANREC(VLAN8_NAME,7);break;
		  	  case 8:ADD_VLANREC(VLAN9_NAME,8);break;
		  	  case 9:ADD_VLANREC(VLAN10_NAME,9);break;
		  	  case 10:ADD_VLANREC(VLAN11_NAME,10);break;
		  	  case 11:ADD_VLANREC(VLAN12_NAME,11);break;
		  	  case 12:ADD_VLANREC(VLAN13_NAME,12);break;
		  	  case 13:ADD_VLANREC(VLAN14_NAME,13);break;
		  	  case 14:ADD_VLANREC(VLAN15_NAME,14);break;
		  	  case 15:ADD_VLANREC(VLAN16_NAME,15);break;
		  	  case 16:ADD_VLANREC(VLAN17_NAME,16);break;
		  	  case 17:ADD_VLANREC(VLAN18_NAME,17);break;
		  	  case 18:ADD_VLANREC(VLAN19_NAME,18);break;
		  	  case 19:ADD_VLANREC(VLAN20_NAME,19);break;
			  case 20:ADD_VLANREC(VLAN21_NAME,20);break;
			  case 21:ADD_VLANREC(VLAN22_NAME,21);break;
			  case 22:ADD_VLANREC(VLAN23_NAME,22);break;
			  case 23:ADD_VLANREC(VLAN24_NAME,23);break;
			  case 24:ADD_VLANREC(VLAN25_NAME,24);break;
			  case 25:ADD_VLANREC(VLAN26_NAME,25);break;
			  case 26:ADD_VLANREC(VLAN27_NAME,26);break;
			  case 27:ADD_VLANREC(VLAN28_NAME,27);break;
			  case 28:ADD_VLANREC(VLAN29_NAME,28);break;
			  case 29:ADD_VLANREC(VLAN30_NAME,29);break;
			  case 30:ADD_VLANREC(VLAN31_NAME,30);break;
			  case 31:ADD_VLANREC(VLAN32_NAME,31);break;
			  case 32:ADD_VLANREC(VLAN33_NAME,32);break;
			  case 33:ADD_VLANREC(VLAN34_NAME,33);break;
			  case 34:ADD_VLANREC(VLAN35_NAME,34);break;
			  case 35:ADD_VLANREC(VLAN36_NAME,35);break;
			  case 36:ADD_VLANREC(VLAN37_NAME,36);break;
			  case 37:ADD_VLANREC(VLAN38_NAME,37);break;
			  case 38:ADD_VLANREC(VLAN39_NAME,38);break;
			  case 39:ADD_VLANREC(VLAN40_NAME,39);break;
		  }
	  }

	  ADD_COMMENT("VLAN Trunking settings");
	  ADD_U8(VLAN_TRUNK_STATE_NAME,get_vlan_trunk_state());
	  ADD_U8(VLAN_PORT1_STATE_NAME,get_vlan_sett_port_state(0));
	  ADD_U8(VLAN_PORT2_STATE_NAME,get_vlan_sett_port_state(1));
	  ADD_U8(VLAN_PORT3_STATE_NAME,get_vlan_sett_port_state(2));
	  ADD_U8(VLAN_PORT4_STATE_NAME,get_vlan_sett_port_state(3));
	  ADD_U8(VLAN_PORT5_STATE_NAME,get_vlan_sett_port_state(4));
	  ADD_U8(VLAN_PORT6_STATE_NAME,get_vlan_sett_port_state(5));
	  ADD_U8(VLAN_PORT7_STATE_NAME,get_vlan_sett_port_state(6));
	  ADD_U8(VLAN_PORT8_STATE_NAME,get_vlan_sett_port_state(7));
	  ADD_U8(VLAN_PORT9_STATE_NAME,get_vlan_sett_port_state(8));
	  ADD_U8(VLAN_PORT10_STATE_NAME,get_vlan_sett_port_state(9));
	  ADD_U8(VLAN_PORT11_STATE_NAME,get_vlan_sett_port_state(10));
	  ADD_U8(VLAN_PORT12_STATE_NAME,get_vlan_sett_port_state(11));
	  ADD_U8(VLAN_PORT13_STATE_NAME,get_vlan_sett_port_state(12));
	  ADD_U8(VLAN_PORT14_STATE_NAME,get_vlan_sett_port_state(13));
	  ADD_U8(VLAN_PORT15_STATE_NAME,get_vlan_sett_port_state(14));
	  ADD_U8(VLAN_PORT16_STATE_NAME,get_vlan_sett_port_state(15));


	  ADD_COMMENT("STP/RSTP settings");

	  ADD_U8(STP_STATE_NAME,get_stp_state());

	  ADD_U8(STP_MAGIC_NAME,get_stp_magic());

	  ADD_U8(STP_PROTO_NAME,get_stp_proto());

	  ADD_U8(STP_BRIDGE_PRIOR_NAME,get_stp_bridge_priority());

	  ADD_U8(STP_MAX_AGE_NAME,get_stp_bridge_max_age());

	  ADD_U8(STP_HELLO_TIME_NAME,get_stp_bridge_htime());

	  ADD_U8(STP_FORW_DELAY_NAME,get_stp_bridge_fdelay());

	  ADD_U8(STP_MIGRATE_DELAY_NAME,get_stp_bridge_mdelay());

	  ADD_U8(STP_TX_HCOUNT_NAME,get_stp_txholdcount());

	  ADD_STPREC(STP_PORT1_CFG_NAME,0);
	  ADD_STPREC(STP_PORT2_CFG_NAME,1);
	  ADD_STPREC(STP_PORT3_CFG_NAME,2);
	  ADD_STPREC(STP_PORT4_CFG_NAME,3);
	  ADD_STPREC(STP_PORT5_CFG_NAME,4);
	  ADD_STPREC(STP_PORT6_CFG_NAME,5);
	  ADD_STPREC(STP_PORT7_CFG_NAME,6);
	  ADD_STPREC(STP_PORT8_CFG_NAME,7);
	  ADD_STPREC(STP_PORT9_CFG_NAME,8);
	  ADD_STPREC(STP_PORT10_CFG_NAME,9);
	  ADD_STPREC(STP_PORT11_CFG_NAME,10);
	  ADD_STPREC(STP_PORT12_CFG_NAME,11);
	  ADD_STPREC(STP_PORT13_CFG_NAME,12);
	  ADD_STPREC(STP_PORT14_CFG_NAME,13);
	  ADD_STPREC(STP_PORT15_CFG_NAME,14);
	  ADD_STPREC(STP_PORT16_CFG_NAME,15);

	  ADD_U8(BPDU_FORWARD_NAME,get_stp_bpdu_fw());



	  ADD_COMMENT("Virtual cable tester settings");
	  ADD_U8(PORT1_VCT_ADJ_NAME,get_callibrate_koef_1(0));
	  ADD_U8(PORT2_VCT_ADJ_NAME,get_callibrate_koef_1(1));
	  ADD_U8(PORT3_VCT_ADJ_NAME,get_callibrate_koef_1(2));
	  ADD_U8(PORT4_VCT_ADJ_NAME,get_callibrate_koef_1(3));
	  ADD_U8(PORT5_VCT_ADJ_NAME,get_callibrate_koef_1(4));
	  ADD_U8(PORT6_VCT_ADJ_NAME,get_callibrate_koef_1(5));
	  ADD_U8(PORT7_VCT_ADJ_NAME,get_callibrate_koef_1(6));
	  ADD_U8(PORT8_VCT_ADJ_NAME,get_callibrate_koef_1(7));
	  ADD_U8(PORT9_VCT_ADJ_NAME,get_callibrate_koef_1(8));
	  ADD_U8(PORT10_VCT_ADJ_NAME,get_callibrate_koef_1(9));
	  ADD_U8(PORT11_VCT_ADJ_NAME,get_callibrate_koef_1(10));
	  ADD_U8(PORT12_VCT_ADJ_NAME,get_callibrate_koef_1(11));
	  ADD_U8(PORT13_VCT_ADJ_NAME,get_callibrate_koef_1(12));
	  ADD_U8(PORT14_VCT_ADJ_NAME,get_callibrate_koef_1(13));
	  ADD_U8(PORT15_VCT_ADJ_NAME,get_callibrate_koef_1(14));
	  ADD_U8(PORT16_VCT_ADJ_NAME,get_callibrate_koef_1(15));

	  ADD_U8(PORT1_VCT_LEN_NAME,get_callibrate_len(0));
	  ADD_U8(PORT2_VCT_LEN_NAME,get_callibrate_len(1));
	  ADD_U8(PORT3_VCT_LEN_NAME,get_callibrate_len(2));
	  ADD_U8(PORT4_VCT_LEN_NAME,get_callibrate_len(3));
	  ADD_U8(PORT5_VCT_LEN_NAME,get_callibrate_len(4));
	  ADD_U8(PORT6_VCT_LEN_NAME,get_callibrate_len(5));
	  ADD_U8(PORT7_VCT_LEN_NAME,get_callibrate_len(6));
	  ADD_U8(PORT8_VCT_LEN_NAME,get_callibrate_len(7));
	  ADD_U8(PORT9_VCT_LEN_NAME,get_callibrate_len(8));
	  ADD_U8(PORT10_VCT_LEN_NAME,get_callibrate_len(9));
	  ADD_U8(PORT11_VCT_LEN_NAME,get_callibrate_len(10));
	  ADD_U8(PORT12_VCT_LEN_NAME,get_callibrate_len(11));
	  ADD_U8(PORT13_VCT_LEN_NAME,get_callibrate_len(12));
	  ADD_U8(PORT14_VCT_LEN_NAME,get_callibrate_len(13));
	  ADD_U8(PORT15_VCT_LEN_NAME,get_callibrate_len(14));
	  ADD_U8(PORT16_VCT_LEN_NAME,get_callibrate_len(15));


	  ADD_COMMENT("SNMP settings");
	  ADD_U8(SNMP_STATE_NAME,get_snmp_state());

	  get_snmp_serv(&ip);
	  ADD_IP(SNMP_SERVER_NAME,ip);

	  ADD_U8(SNMP_VERS_NAME,get_snmp_vers());

	  get_snmp1_read_communitie(q);
	  ADD_STR(SNMP_COMMUNITY1_NAME,q);

	  get_snmp1_write_communitie(q);
	  ADD_STR(SNMP_COMMUNITY2_NAME,q);

	  ADD_U8(SNMPV3_USER1_LEVEL,get_snmp3_level(0));
	  get_snmp3_user_name(0,q);
	  ADD_STR(SNMPV3_USER1_USER_NAME,q);
	  get_snmp3_auth_pass(0,q);
	  ADD_STR(SNMPV3_USER1_AUTH_PASS,q);
	  get_snmp3_priv_pass(0,q);
	  ADD_STR(SNMPV3_USER1_PRIV_PASS,q);

	  if(NUM_USER == 2){
		  ADD_U8(SNMPV3_USER2_LEVEL,get_snmp3_level(1));
		  get_snmp3_user_name(1,q);
		  ADD_STR(SNMPV3_USER2_USER_NAME,q);
		  get_snmp3_auth_pass(1,q);
		  ADD_STR(SNMPV3_USER2_AUTH_PASS,q);
		  get_snmp3_priv_pass(1,q);
		  ADD_STR(SNMPV3_USER2_PRIV_PASS,q);
	  }

	  if(NUM_USER == 3){
		  ADD_U8(SNMPV3_USER3_LEVEL,get_snmp3_level(2));
		  get_snmp3_user_name(2,q);
		  ADD_STR(SNMPV3_USER3_USER_NAME,q);
		  get_snmp3_auth_pass(2,q);
		  ADD_STR(SNMPV3_USER3_AUTH_PASS,q);
		  get_snmp3_priv_pass(2,q);
		  ADD_STR(SNMPV3_USER3_PRIV_PASS,q);
	  }

	  engine_id_t eid;
	  get_snmp3_engine_id(&eid);
	  engineid_to_str(&eid,q);
	  ADD_STR(SNMP3_ENGINE_ID_NAME,q);


	  ADD_COMMENT("IGMP settings");
	  ADD_U8(IGMP_STATE_NAME,get_igmp_snooping_state());

	  ADD_U8(IGMP_PORT_1_STATE_NAME,get_igmp_port_state(0));
	  ADD_U8(IGMP_PORT_2_STATE_NAME,get_igmp_port_state(1));
	  ADD_U8(IGMP_PORT_3_STATE_NAME,get_igmp_port_state(2));
	  ADD_U8(IGMP_PORT_4_STATE_NAME,get_igmp_port_state(3));
	  ADD_U8(IGMP_PORT_5_STATE_NAME,get_igmp_port_state(4));
	  ADD_U8(IGMP_PORT_6_STATE_NAME,get_igmp_port_state(5));
	  ADD_U8(IGMP_PORT_7_STATE_NAME,get_igmp_port_state(6));
	  ADD_U8(IGMP_PORT_8_STATE_NAME,get_igmp_port_state(7));
	  ADD_U8(IGMP_PORT_9_STATE_NAME,get_igmp_port_state(8));
	  ADD_U8(IGMP_PORT_10_STATE_NAME,get_igmp_port_state(9));
	  ADD_U8(IGMP_PORT_11_STATE_NAME,get_igmp_port_state(10));
	  ADD_U8(IGMP_PORT_12_STATE_NAME,get_igmp_port_state(11));
	  ADD_U8(IGMP_PORT_13_STATE_NAME,get_igmp_port_state(12));
	  ADD_U8(IGMP_PORT_14_STATE_NAME,get_igmp_port_state(13));
	  ADD_U8(IGMP_PORT_15_STATE_NAME,get_igmp_port_state(14));
	  ADD_U8(IGMP_PORT_16_STATE_NAME,get_igmp_port_state(15));

	  ADD_U8(IGMP_QUERY_INTERVAL_NAME,get_igmp_query_int());
	  ADD_U8(IGMP_QUERY_MODE_NAME,get_igmp_query_mode());

	  ADD_U8(IGMP_QUERY_RESP_INTERVAL_NAME,get_igmp_max_resp_time());

	  ADD_U8(IGMP_GROUP_MEMB_TIME_NAME,get_igmp_group_membership_time());

	  ADD_U8(IGMP_OTHER_QUERIER_NAME,get_igmp_other_querier_time());

	  ADD_COMMENT("Telnet settings");
	  ADD_U8(TELNET_STATE_NAME,get_telnet_state());


	  ADD_COMMENT("Downshifting settings");
	  ADD_U8(DOWNSHIFT_STATE_NAME,get_downshifting_mode());

	  ADD_COMMENT("TFTP settings");
	  ADD_U8(TFTP_STATE_NAME,get_tftp_state());
	  ADD_U8(TFTP_MODE_NAME,get_tftp_mode());
	  ADD_U16(TFTP_PORT_NAME,get_tftp_port());

	  ADD_COMMENT("PLC settings (option)");
	  ADD_U16(PLC_EM_MODEL_NAME,get_plc_em_model());
	  ADD_U8(PLC_EM_BAUDRATE_NAME,get_plc_em_rate());
	  ADD_U8(PLC_EM_PARITY_NAME,get_plc_em_parity());
	  ADD_U8(PLC_EM_DATABITS_NAME,get_plc_em_databits());
	  ADD_U8(PLC_EM_STOPBITS_NAME,get_plc_em_stopbits());

	  get_plc_em_pass(q);
	  ADD_STR(PLC_EM_PASS_NAME,q);

	  get_plc_em_id(q);
	  ADD_STR(PLC_EM_ID_NAME,q);

	  ADD_U8(PLC_OUT1_STATE_NAME,get_plc_out_state(0));
	  ADD_U8(PLC_OUT2_STATE_NAME,get_plc_out_state(1));
	  ADD_U8(PLC_OUT3_STATE_NAME,get_plc_out_state(3));
	  ADD_U8(PLC_OUT4_STATE_NAME,get_plc_out_state(4));

	  ADD_U8(PLC_OUT1_ACTION_NAME,get_plc_out_action(0));
	  ADD_U8(PLC_OUT2_ACTION_NAME,get_plc_out_action(1));
	  ADD_U8(PLC_OUT3_ACTION_NAME,get_plc_out_action(2));
	  ADD_U8(PLC_OUT4_ACTION_NAME,get_plc_out_action(3));

	  ADD_U8(PLC_OUT1_EVENT1_NAME,get_plc_out_event(0,0));
	  ADD_U8(PLC_OUT1_EVENT2_NAME,get_plc_out_event(0,1));
	  ADD_U8(PLC_OUT1_EVENT3_NAME,get_plc_out_event(0,2));
	  ADD_U8(PLC_OUT1_EVENT4_NAME,get_plc_out_event(0,3));
	  ADD_U8(PLC_OUT1_EVENT5_NAME,get_plc_out_event(0,4));
	  ADD_U8(PLC_OUT1_EVENT6_NAME,get_plc_out_event(0,5));
	  ADD_U8(PLC_OUT1_EVENT7_NAME,get_plc_out_event(0,6));

	  ADD_U8(PLC_OUT2_EVENT1_NAME,get_plc_out_event(1,0));
	  ADD_U8(PLC_OUT2_EVENT2_NAME,get_plc_out_event(1,1));
	  ADD_U8(PLC_OUT2_EVENT3_NAME,get_plc_out_event(1,2));
	  ADD_U8(PLC_OUT2_EVENT4_NAME,get_plc_out_event(1,3));
	  ADD_U8(PLC_OUT2_EVENT5_NAME,get_plc_out_event(1,4));
	  ADD_U8(PLC_OUT2_EVENT6_NAME,get_plc_out_event(1,5));
	  ADD_U8(PLC_OUT2_EVENT7_NAME,get_plc_out_event(1,6));

	  ADD_U8(PLC_OUT3_EVENT1_NAME,get_plc_out_event(2,0));
	  ADD_U8(PLC_OUT3_EVENT2_NAME,get_plc_out_event(2,1));
	  ADD_U8(PLC_OUT3_EVENT3_NAME,get_plc_out_event(2,2));
	  ADD_U8(PLC_OUT3_EVENT4_NAME,get_plc_out_event(2,3));
	  ADD_U8(PLC_OUT3_EVENT5_NAME,get_plc_out_event(2,4));
	  ADD_U8(PLC_OUT3_EVENT6_NAME,get_plc_out_event(2,5));
	  ADD_U8(PLC_OUT3_EVENT7_NAME,get_plc_out_event(2,6));

	  ADD_U8(PLC_OUT4_EVENT1_NAME,get_plc_out_event(3,0));
	  ADD_U8(PLC_OUT4_EVENT2_NAME,get_plc_out_event(3,1));
	  ADD_U8(PLC_OUT4_EVENT3_NAME,get_plc_out_event(3,2));
	  ADD_U8(PLC_OUT4_EVENT4_NAME,get_plc_out_event(3,3));
	  ADD_U8(PLC_OUT4_EVENT5_NAME,get_plc_out_event(3,4));
	  ADD_U8(PLC_OUT4_EVENT6_NAME,get_plc_out_event(3,5));
	  ADD_U8(PLC_OUT4_EVENT7_NAME,get_plc_out_event(3,6));

	  ADD_COMMENT("MAC Filtering");
	  ADD_U8(PORT1_MACFILT_NAME,get_mac_filter_state(0));
	  ADD_U8(PORT2_MACFILT_NAME,get_mac_filter_state(1));
	  ADD_U8(PORT3_MACFILT_NAME,get_mac_filter_state(2));
	  ADD_U8(PORT4_MACFILT_NAME,get_mac_filter_state(3));
	  ADD_U8(PORT5_MACFILT_NAME,get_mac_filter_state(4));
	  ADD_U8(PORT6_MACFILT_NAME,get_mac_filter_state(5));
	  ADD_U8(PORT7_MACFILT_NAME,get_mac_filter_state(6));
	  ADD_U8(PORT8_MACFILT_NAME,get_mac_filter_state(7));
	  ADD_U8(PORT9_MACFILT_NAME,get_mac_filter_state(8));
	  ADD_U8(PORT10_MACFILT_NAME,get_mac_filter_state(9));
	  ADD_U8(PORT11_MACFILT_NAME,get_mac_filter_state(10));
	  ADD_U8(PORT12_MACFILT_NAME,get_mac_filter_state(11));
	  ADD_U8(PORT13_MACFILT_NAME,get_mac_filter_state(12));
	  ADD_U8(PORT14_MACFILT_NAME,get_mac_filter_state(13));
	  ADD_U8(PORT15_MACFILT_NAME,get_mac_filter_state(14));
	  ADD_U8(PORT16_MACFILT_NAME,get_mac_filter_state(15));


	  ADD_COMMENT("MAC Address Binding");
	  ADD_U8(MAC_BIND_ENRTY1_ACTIVE_NAME,get_mac_bind_entry_active(0));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(0,k);
	  ADD_MAC(MAC_BIND_ENRTY1_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY1_PORT_NAME,get_mac_bind_entry_port(0));

	  ADD_U8(MAC_BIND_ENRTY2_ACTIVE_NAME,get_mac_bind_entry_active(1));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(1,k);
	  ADD_MAC(MAC_BIND_ENRTY2_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY2_PORT_NAME,get_mac_bind_entry_port(1));

	  ADD_U8(MAC_BIND_ENRTY3_ACTIVE_NAME,get_mac_bind_entry_active(2));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(2,k);
	  ADD_MAC(MAC_BIND_ENRTY3_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY3_PORT_NAME,get_mac_bind_entry_port(2));

	  ADD_U8(MAC_BIND_ENRTY4_ACTIVE_NAME,get_mac_bind_entry_active(3));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(3,k);
	  ADD_MAC(MAC_BIND_ENRTY4_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY4_PORT_NAME,get_mac_bind_entry_port(3));

	  ADD_U8(MAC_BIND_ENRTY5_ACTIVE_NAME,get_mac_bind_entry_active(4));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(4,k);
	  ADD_MAC(MAC_BIND_ENRTY5_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY5_PORT_NAME,get_mac_bind_entry_port(4));

	  ADD_U8(MAC_BIND_ENRTY6_ACTIVE_NAME,get_mac_bind_entry_active(5));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(5,k);
	  ADD_MAC(MAC_BIND_ENRTY6_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY6_PORT_NAME,get_mac_bind_entry_port(5));

	  ADD_U8(MAC_BIND_ENRTY7_ACTIVE_NAME,get_mac_bind_entry_active(6));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(6,k);
	  ADD_MAC(MAC_BIND_ENRTY7_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY7_PORT_NAME,get_mac_bind_entry_port(6));

	  ADD_U8(MAC_BIND_ENRTY8_ACTIVE_NAME,get_mac_bind_entry_active(7));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(7,k);
	  ADD_MAC(MAC_BIND_ENRTY8_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY8_PORT_NAME,get_mac_bind_entry_port(7));

	  ADD_U8(MAC_BIND_ENRTY9_ACTIVE_NAME,get_mac_bind_entry_active(8));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(8,k);
	  ADD_MAC(MAC_BIND_ENRTY9_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY9_PORT_NAME,get_mac_bind_entry_port(8));

	  ADD_U8(MAC_BIND_ENRTY10_ACTIVE_NAME,get_mac_bind_entry_active(9));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(9,k);
	  ADD_MAC(MAC_BIND_ENRTY10_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY10_PORT_NAME,get_mac_bind_entry_port(9));

	  ADD_U8(MAC_BIND_ENRTY11_ACTIVE_NAME,get_mac_bind_entry_active(10));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(10,k);
	  ADD_MAC(MAC_BIND_ENRTY11_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY11_PORT_NAME,get_mac_bind_entry_port(10));

	  ADD_U8(MAC_BIND_ENRTY12_ACTIVE_NAME,get_mac_bind_entry_active(11));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(11,k);
	  ADD_MAC(MAC_BIND_ENRTY12_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY12_PORT_NAME,get_mac_bind_entry_port(11));

	  ADD_U8(MAC_BIND_ENRTY13_ACTIVE_NAME,get_mac_bind_entry_active(12));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(12,k);
	  ADD_MAC(MAC_BIND_ENRTY13_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY13_PORT_NAME,get_mac_bind_entry_port(12));

	  ADD_U8(MAC_BIND_ENRTY14_ACTIVE_NAME,get_mac_bind_entry_active(13));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(13,k);
	  ADD_MAC(MAC_BIND_ENRTY14_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY14_PORT_NAME,get_mac_bind_entry_port(13));

	  ADD_U8(MAC_BIND_ENRTY15_ACTIVE_NAME,get_mac_bind_entry_active(14));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(14,k);
	  ADD_MAC(MAC_BIND_ENRTY15_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY15_PORT_NAME,get_mac_bind_entry_port(14));

	  ADD_U8(MAC_BIND_ENRTY16_ACTIVE_NAME,get_mac_bind_entry_active(15));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(15,k);
	  ADD_MAC(MAC_BIND_ENRTY16_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY16_PORT_NAME,get_mac_bind_entry_port(15));

	  ADD_U8(MAC_BIND_ENRTY17_ACTIVE_NAME,get_mac_bind_entry_active(16));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(16,k);
	  ADD_MAC(MAC_BIND_ENRTY17_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY17_PORT_NAME,get_mac_bind_entry_port(16));

	  ADD_U8(MAC_BIND_ENRTY18_ACTIVE_NAME,get_mac_bind_entry_active(17));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(17,k);
	  ADD_MAC(MAC_BIND_ENRTY18_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY18_PORT_NAME,get_mac_bind_entry_port(17));

	  ADD_U8(MAC_BIND_ENRTY19_ACTIVE_NAME,get_mac_bind_entry_active(18));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(18,k);
	  ADD_MAC(MAC_BIND_ENRTY19_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY19_PORT_NAME,get_mac_bind_entry_port(18));

	  ADD_U8(MAC_BIND_ENRTY20_ACTIVE_NAME,get_mac_bind_entry_active(19));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(19,k);
	  ADD_MAC(MAC_BIND_ENRTY20_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY20_PORT_NAME,get_mac_bind_entry_port(19));


	  ADD_U8(MAC_BIND_ENRTY21_ACTIVE_NAME,get_mac_bind_entry_active(20));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(20,k);
	  ADD_MAC(MAC_BIND_ENRTY21_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY21_PORT_NAME,get_mac_bind_entry_port(20));

	  ADD_U8(MAC_BIND_ENRTY22_ACTIVE_NAME,get_mac_bind_entry_active(21));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(21,k);
	  ADD_MAC(MAC_BIND_ENRTY22_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY22_PORT_NAME,get_mac_bind_entry_port(21));

	  ADD_U8(MAC_BIND_ENRTY23_ACTIVE_NAME,get_mac_bind_entry_active(22));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(22,k);
	  ADD_MAC(MAC_BIND_ENRTY23_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY23_PORT_NAME,get_mac_bind_entry_port(22));

	  ADD_U8(MAC_BIND_ENRTY24_ACTIVE_NAME,get_mac_bind_entry_active(23));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(23,k);
	  ADD_MAC(MAC_BIND_ENRTY24_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY24_PORT_NAME,get_mac_bind_entry_port(23));

	  ADD_U8(MAC_BIND_ENRTY25_ACTIVE_NAME,get_mac_bind_entry_active(24));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(24,k);
	  ADD_MAC(MAC_BIND_ENRTY25_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY25_PORT_NAME,get_mac_bind_entry_port(24));

	  ADD_U8(MAC_BIND_ENRTY26_ACTIVE_NAME,get_mac_bind_entry_active(25));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(25,k);
	  ADD_MAC(MAC_BIND_ENRTY26_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY26_PORT_NAME,get_mac_bind_entry_port(25));

	  ADD_U8(MAC_BIND_ENRTY27_ACTIVE_NAME,get_mac_bind_entry_active(26));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(26,k);
	  ADD_MAC(MAC_BIND_ENRTY27_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY27_PORT_NAME,get_mac_bind_entry_port(26));

	  ADD_U8(MAC_BIND_ENRTY28_ACTIVE_NAME,get_mac_bind_entry_active(27));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(27,k);
	  ADD_MAC(MAC_BIND_ENRTY28_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY28_PORT_NAME,get_mac_bind_entry_port(27));

	  ADD_U8(MAC_BIND_ENRTY29_ACTIVE_NAME,get_mac_bind_entry_active(28));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(28,k);
	  ADD_MAC(MAC_BIND_ENRTY29_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY29_PORT_NAME,get_mac_bind_entry_port(28));

	  ADD_U8(MAC_BIND_ENRTY30_ACTIVE_NAME,get_mac_bind_entry_active(29));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(29,k);
	  ADD_MAC(MAC_BIND_ENRTY30_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY30_PORT_NAME,get_mac_bind_entry_port(29));

	  ADD_U8(MAC_BIND_ENRTY31_ACTIVE_NAME,get_mac_bind_entry_active(30));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(30,k);
	  ADD_MAC(MAC_BIND_ENRTY31_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY31_PORT_NAME,get_mac_bind_entry_port(30));

	  ADD_U8(MAC_BIND_ENRTY32_ACTIVE_NAME,get_mac_bind_entry_active(31));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(31,k);
	  ADD_MAC(MAC_BIND_ENRTY32_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY32_PORT_NAME,get_mac_bind_entry_port(31));

	  //
	  ADD_U8(MAC_BIND_ENRTY33_ACTIVE_NAME,get_mac_bind_entry_active(32));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(32,k);
	  ADD_MAC(MAC_BIND_ENRTY33_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY33_PORT_NAME,get_mac_bind_entry_port(32));

	  ADD_U8(MAC_BIND_ENRTY34_ACTIVE_NAME,get_mac_bind_entry_active(33));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(33,k);
	  ADD_MAC(MAC_BIND_ENRTY34_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY34_PORT_NAME,get_mac_bind_entry_port(33));

	  ADD_U8(MAC_BIND_ENRTY35_ACTIVE_NAME,get_mac_bind_entry_active(34));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(34,k);
	  ADD_MAC(MAC_BIND_ENRTY35_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY35_PORT_NAME,get_mac_bind_entry_port(34));

	  ADD_U8(MAC_BIND_ENRTY36_ACTIVE_NAME,get_mac_bind_entry_active(35));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(35,k);
	  ADD_MAC(MAC_BIND_ENRTY36_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY36_PORT_NAME,get_mac_bind_entry_port(35));

	  ADD_U8(MAC_BIND_ENRTY37_ACTIVE_NAME,get_mac_bind_entry_active(36));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(36,k);
	  ADD_MAC(MAC_BIND_ENRTY37_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY37_PORT_NAME,get_mac_bind_entry_port(36));

	  ADD_U8(MAC_BIND_ENRTY38_ACTIVE_NAME,get_mac_bind_entry_active(37));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(37,k);
	  ADD_MAC(MAC_BIND_ENRTY38_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY38_PORT_NAME,get_mac_bind_entry_port(37));

	  ADD_U8(MAC_BIND_ENRTY39_ACTIVE_NAME,get_mac_bind_entry_active(38));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(38,k);
	  ADD_MAC(MAC_BIND_ENRTY39_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY39_PORT_NAME,get_mac_bind_entry_port(38));

	  ADD_U8(MAC_BIND_ENRTY40_ACTIVE_NAME,get_mac_bind_entry_active(39));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(39,k);
	  ADD_MAC(MAC_BIND_ENRTY40_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY40_PORT_NAME,get_mac_bind_entry_port(39));

	  ADD_U8(MAC_BIND_ENRTY41_ACTIVE_NAME,get_mac_bind_entry_active(40));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(40,k);
	  ADD_MAC(MAC_BIND_ENRTY41_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY41_PORT_NAME,get_mac_bind_entry_port(40));

	  ADD_U8(MAC_BIND_ENRTY42_ACTIVE_NAME,get_mac_bind_entry_active(41));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(41,k);
	  ADD_MAC(MAC_BIND_ENRTY42_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY42_PORT_NAME,get_mac_bind_entry_port(41));

	  ADD_U8(MAC_BIND_ENRTY43_ACTIVE_NAME,get_mac_bind_entry_active(42));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(42,k);
	  ADD_MAC(MAC_BIND_ENRTY43_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY43_PORT_NAME,get_mac_bind_entry_port(42));

	  ADD_U8(MAC_BIND_ENRTY44_ACTIVE_NAME,get_mac_bind_entry_active(43));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(43,k);
	  ADD_MAC(MAC_BIND_ENRTY44_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY44_PORT_NAME,get_mac_bind_entry_port(43));

	  ADD_U8(MAC_BIND_ENRTY45_ACTIVE_NAME,get_mac_bind_entry_active(44));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(44,k);
	  ADD_MAC(MAC_BIND_ENRTY45_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY45_PORT_NAME,get_mac_bind_entry_port(44));

	  ADD_U8(MAC_BIND_ENRTY46_ACTIVE_NAME,get_mac_bind_entry_active(45));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(45,k);
	  ADD_MAC(MAC_BIND_ENRTY46_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY46_PORT_NAME,get_mac_bind_entry_port(45));

	  ADD_U8(MAC_BIND_ENRTY47_ACTIVE_NAME,get_mac_bind_entry_active(46));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(46,k);
	  ADD_MAC(MAC_BIND_ENRTY47_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY47_PORT_NAME,get_mac_bind_entry_port(46));

	  ADD_U8(MAC_BIND_ENRTY48_ACTIVE_NAME,get_mac_bind_entry_active(47));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(47,k);
	  ADD_MAC(MAC_BIND_ENRTY48_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY48_PORT_NAME,get_mac_bind_entry_port(47));

	  ADD_U8(MAC_BIND_ENRTY49_ACTIVE_NAME,get_mac_bind_entry_active(48));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(48,k);
	  ADD_MAC(MAC_BIND_ENRTY49_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY49_PORT_NAME,get_mac_bind_entry_port(48));

	  ADD_U8(MAC_BIND_ENRTY50_ACTIVE_NAME,get_mac_bind_entry_active(49));
	  for(u8 k=0;k<6;k++)tmp[k]=get_mac_bind_entry_mac(49,k);
	  ADD_MAC(MAC_BIND_ENRTY50_MAC_NAME,tmp);
	  ADD_U8(MAC_BIND_ENRTY50_PORT_NAME,get_mac_bind_entry_port(49));





	  ADD_COMMENT("UPS Settings");
	  ADD_U8(UPS_DELAYED_START_NAME,get_ups_delayed_start());

	  ADD_COMMENT("Teleport Settings");
	  ADD_U8(INPUT1_STATE_NAME,get_input_state(0));
	  ADD_U8(INPUT2_STATE_NAME,get_input_state(1));
	  ADD_U8(INPUT3_STATE_NAME,get_input_state(2));


	  ADD_U8(INPUT1_INVERSE_NAME,get_input_inverse(0));
	  ADD_U8(INPUT2_INVERSE_NAME,get_input_inverse(1));
	  ADD_U8(INPUT3_INVERSE_NAME,get_input_inverse(2));


	  ADD_U8(INPUT1_REMDEV_NAME,get_input_rem_dev(0));
	  ADD_U8(INPUT2_REMDEV_NAME,get_input_rem_dev(1));
	  ADD_U8(INPUT3_REMDEV_NAME,get_input_rem_dev(2));

	  ADD_U8(INPUT1_REMPORT_NAME,get_input_rem_port(0));
	  ADD_U8(INPUT2_REMPORT_NAME,get_input_rem_port(1));
	  ADD_U8(INPUT3_REMPORT_NAME,get_input_rem_port(2));

	  ADD_U8(TLP_EVENT1_STATE_NAME,get_tlp_event_state(0));
	  ADD_U8(TLP_EVENT2_STATE_NAME,get_tlp_event_state(1));

	  ADD_U8(TLP_EVENT1_INVERSE_NAME,get_tlp_event_inverse(0));
	  ADD_U8(TLP_EVENT2_INVERSE_NAME,get_tlp_event_inverse(1));

	  ADD_U8(TLP_EVENT1_REMDEV_NAME,get_tlp_event_rem_dev(0));
	  ADD_U8(TLP_EVENT2_REMDEV_NAME,get_tlp_event_rem_dev(1));

	  ADD_U8(TLP_EVENT1_REMPORT_NAME,get_tlp_event_rem_port(0));
	  ADD_U8(TLP_EVENT2_REMPORT_NAME,get_tlp_event_rem_port(1));

	  ADD_U8(TLP1_VALID_NAME,get_tlp_remdev_valid(0));
	  ADD_U8(TLP2_VALID_NAME,get_tlp_remdev_valid(1));

	  ADD_U8(TLP1_TYPE_NAME,get_tlp_remdev_type(0));
	  ADD_U8(TLP2_TYPE_NAME,get_tlp_remdev_type(1));

	  get_tlp_remdev_name(0,q);
	  ADD_STR(TLP1_DESCR_NAME,q);

	  get_tlp_remdev_name(1,q);
	  ADD_STR(TLP2_DESCR_NAME,q);


	  get_tlp_remdev_ip(0,&ip);
	  ADD_IP(TLP1_IP_NAME,ip);

	  get_tlp_remdev_ip(1,&ip);
	  ADD_IP(TLP2_IP_NAME,ip);

	  ADD_U8(LLDP_STATE_NAME,get_lldp_state());
	  ADD_U8(LLDP_TX_INT_NAME,get_lldp_transmit_interval());
	  ADD_U8(LLDP_HOLD_TIME_NAME,get_lldp_hold_multiplier());
	  ADD_U8(LLDP_PORT1_STATE_NAME,get_lldp_port_state(0));
	  ADD_U8(LLDP_PORT2_STATE_NAME,get_lldp_port_state(1));
	  ADD_U8(LLDP_PORT3_STATE_NAME,get_lldp_port_state(2));
	  ADD_U8(LLDP_PORT4_STATE_NAME,get_lldp_port_state(3));
	  ADD_U8(LLDP_PORT5_STATE_NAME,get_lldp_port_state(4));
	  ADD_U8(LLDP_PORT6_STATE_NAME,get_lldp_port_state(5));
	  ADD_U8(LLDP_PORT7_STATE_NAME,get_lldp_port_state(6));
	  ADD_U8(LLDP_PORT8_STATE_NAME,get_lldp_port_state(7));
	  ADD_U8(LLDP_PORT9_STATE_NAME,get_lldp_port_state(8));
	  ADD_U8(LLDP_PORT10_STATE_NAME,get_lldp_port_state(9));
	  ADD_U8(LLDP_PORT11_STATE_NAME,get_lldp_port_state(10));
	  ADD_U8(LLDP_PORT12_STATE_NAME,get_lldp_port_state(11));
	  ADD_U8(LLDP_PORT13_STATE_NAME,get_lldp_port_state(12));
	  ADD_U8(LLDP_PORT14_STATE_NAME,get_lldp_port_state(13));
	  ADD_U8(LLDP_PORT15_STATE_NAME,get_lldp_port_state(14));
	  ADD_U8(LLDP_PORT16_STATE_NAME,get_lldp_port_state(15));

	  spi_flash_write(offset,strlen(str),str);
	  offset+=strlen(str);


	  return offset;
}




#define VAR_MAX_LEN	32
#define IP_MAX_LEN 18
#define MAC_MAX_LEN 18
#define U16_MAX_LEN	10
#define U32_MAX_LEN 16
#define STR_MAX_LEN 128
#define BKP_FILE_SIZE 20000
#define MIN_VAR_NUM 50

static u8 find_ip_raw(char *str,uip_ipaddr_t ip){
	// atoi(str)
	char *ptr1;
	u32 tip[4];
	//1
	ptr1 = strstr(str,".");
	if(ptr1 == NULL)
		return 0;
	ptr1-=3;
	tip[0] = strtol(ptr1,&ptr1,10);
	if((tip[0]==4294967295))
		return 0;
	//2
	ptr1 = strstr(ptr1,".");
	tip[1] = strtol(ptr1+1,&ptr1,10);
	if((tip[1]==4294967295))
		return 0;
	//3
	ptr1 = strstr(ptr1,".");
	tip[2] = strtol(ptr1+1,&ptr1,10);
	if((tip[2]==4294967295))
		return 0;
	//4
	ptr1 = strstr(ptr1,".");
	tip[3] = strtol(ptr1+1,&ptr1,10);
	if((tip[3]==4294967295))
		return 0;
	uip_ipaddr(ip,(u8)tip[0],(u8)tip[1],(u8)tip[2],(u8)tip[3]);
	DEBUG_MSG(SETTINGS_DBG,"str2ip convert %lu.%lu.%lu.%lu\r\n",tip[0],tip[1],tip[2],tip[3]);
	return 1;
}


static u8 find_mac_raw(char *str,u32 *mac){
	char *ptr_end;
	char *ptr_st;
	//1
	ptr_st = str;
	for(u8 i=0;i<6;i++){
		mac[i] = strtol(ptr_st,&ptr_end,16);
		if((mac[i]==4294967295))
				return 0;
		ptr_st = ptr_end+1;
	}
	return 1;
}

//поиск имени переменной
static u8 find_var_name(char *inpbuf,char *outbuf, u8 *incrptr){
u8 inptr = 0;
u8 outptr = 0;
	while(inpbuf[inptr]!='#'){
		inptr++;
		if(inptr>VAR_MAX_LEN){
			//DEBUG_MSG(SETTINGS_DBG,"nofind #\r\n");
			*incrptr = inptr;
			return 1;
		}
	}
	inptr++;
	while(inpbuf[inptr]!='='){
		outbuf[outptr] = inpbuf[inptr];
		outptr++;
		inptr++;
		*incrptr = inptr;
		if(outptr>VAR_MAX_LEN*2){
			return 1;
		}
		if(inptr>VAR_MAX_LEN*2){
			return 1;
		}
	}
	outbuf[outptr] = 0;
	return 0;
}


static u8 find_ip(char *text,uip_ipaddr_t ip){
	u8 inptr = 0;
	u8 ipptr = 0;
	char ipbuff[IP_MAX_LEN+4];
	while(text[inptr]!='['){
		inptr++;
		if(inptr>IP_MAX_LEN)
			return 1;
	}
	inptr++;
	ipptr = 0;
	while(text[inptr]!=']'){
		ipbuff[ipptr]=text[inptr];
		inptr++;
		ipptr++;
		if(ipptr>IP_MAX_LEN)
			return 1;
	}
	if(find_ip_raw(ipbuff,ip)){
		DEBUG_MSG(SETTINGS_DBG,"ip %d.%d.%d.%d\r\n",uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
		return 0;
	}
	return 1;
}

static u8 find_mac(char *text,u8 *mac){
	u8 inptr = 0;
	u8 macptr = 0;
	char macbuff[MAC_MAX_LEN+4];
	u32 mac_tmp[6];

	while(text[inptr]!='['){
		inptr++;
		if(inptr>MAC_MAX_LEN)
			return 1;
	}
	inptr++;
	macptr = 0;
	while(text[inptr]!=']'){
		macbuff[macptr]=text[inptr];
		inptr++;
		macptr++;
		if(macptr>MAC_MAX_LEN)
			return 1;
	}
	DEBUG_MSG(SETTINGS_DBG,"mac buff %s\r\n",macbuff);
	if(find_mac_raw(macbuff,mac_tmp)){
		for(u8 i=0;i<6;i++)
			mac[i] = (u8)mac_tmp[i];
		DEBUG_MSG(SETTINGS_DBG,"mac %X:%X:%X:%X:%X:%X\r\n",
				mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		return 0;
	}
	return 1;
}

static u8 find_u32(char *text,u32 *val){
	u8 inptr = 0;
	u8 ptr = 0;
	char buff[U32_MAX_LEN+4];
	while(text[inptr]!='['){
		inptr++;
		if(inptr>U32_MAX_LEN)
			return 1;
	}
	inptr++;
	ptr = 0;
	while(text[inptr]!=']'){
		buff[ptr]=text[inptr];
		inptr++;
		ptr++;
		if(ptr>U32_MAX_LEN)
			return 1;
	}
	buff[ptr] = 0;

	if(strtoul(buff,NULL,10) == 4294967295)
		return 1;
	*val = strtoul(buff,NULL,10);
	//DEBUG_MSG(SETTINGS_DBG,"varu32 %lu\r\n",*val);
	return 0;
}

static u8 find_str(char *instr,char *outstr){
u8 inptr = 0;
u8 outptr = 0;

	inptr = 0;
	while(instr[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	//memset(outstr,0,sizeof(outstr));
	while(instr[inptr]!=']'){
		outstr[outptr] = instr[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	outstr[outptr] = 0;
	//DEBUG_MSG(SETTINGS_DBG,"str %s\r\n",outstr);
	return 0;
}

static u8 find_arr(char *buff,u8 *arr,u8 size){
u8 inptr = 0;
u8 outptr = 0;
char tmp[16];
	inptr = 0;
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN*2)
			return 1;
	}
	inptr++;
	for(u8 i=0;i<size;i++){
		outptr = 0;
		while(buff[inptr]!='['){
			inptr++;
			if(inptr>STR_MAX_LEN*2)
				return 1;
		}
		inptr++;
		while(buff[inptr]!=']'){
			tmp[outptr] = buff[inptr];
			inptr++;
			outptr++;
			if(outptr>15)
				return 1;
		}
		tmp[outptr] = 0;
		if(strtoul(tmp,NULL,10) == 4294967295)
			return 1;
		arr[i] = (u8)strtoul(tmp,NULL,10);
		DEBUG_MSG(SETTINGS_DBG,"arr[%d] = %d ",i,arr[i]);
	}
	inptr++;
	if(buff[inptr]!=']')
		return 1;
	return 0;
}

static u8 find_vlan(char *buff,u8 num){
u8 inptr = 0;
u8 outptr = 0;
char tmp[128];
inptr = 0;
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_vlan_state(num,(u8)strtoul(tmp,NULL,10));
	DEBUG_MSG(SETTINGS_DBG,"vlan st %d\r\n",get_vlan_state(num));
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_vlan_vid(num,(u16)strtoul(tmp,NULL,10));
	DEBUG_MSG(SETTINGS_DBG,"vlan vid %d\r\n",get_vlan_vid(num));
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	set_vlan_name(num,tmp);
	DEBUG_MSG(SETTINGS_DBG,"vlan name %s\r\n",tmp);

	for(u8 j=0;j<ALL_PORT_NUM;j++){
		// [
		while(buff[inptr]!='['){
			inptr++;
			if(inptr>STR_MAX_LEN)
				return 1;
		}
		inptr++;
		outptr = 0;
		while(buff[inptr]!=']'){
			tmp[outptr] = buff[inptr];
			inptr++;
			outptr++;
			if(outptr>STR_MAX_LEN)
				return 1;
		}
		tmp[outptr] = 0;
		if(strtoul(tmp,NULL,10) == 4294967295)
			return 1;
		set_vlan_port(num,j,(u16)strtoul(tmp,NULL,10));
		DEBUG_MSG(SETTINGS_DBG,"[%d]",get_vlan_port_state(num,j));
	}
	DEBUG_MSG(SETTINGS_DBG,"\r\n");
	inptr++;
	if(buff[inptr]!=']')
		return 1;
	return 0;
}

static u8 find_stp(char *buff,u8 num){
u8 inptr = 0;
u8 outptr = 0;
char tmp[16];
	inptr = 0;
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_stp_port_enable(num,(u8)strtoul(tmp,NULL,10));
	DEBUG_MSG(SETTINGS_DBG,"stp p en %d\r\n",get_stp_port_enable(num));
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_stp_port_state(num,(u8)strtoul(tmp,NULL,10));
	DEBUG_MSG(SETTINGS_DBG,"stp p st %d\r\n",get_stp_port_state(num));
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_stp_port_state(num,(u8)strtoul(tmp,NULL,10));
	//DEBUG_MSG(SETTINGS_DBG,"stp p pri %d\r\n",get_stp_port_priority(num));
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_stp_port_cost(num,strtoul(tmp,NULL,10));
	//DEBUG_MSG(SETTINGS_DBG,"stp p cost %lu\r\n",get_stp_port_cost(num));
	// [
	while(buff[inptr]!='['){
		inptr++;
		if(inptr>STR_MAX_LEN)
			return 1;
	}
	inptr++;
	outptr = 0;
	while(buff[inptr]!=']'){
		tmp[outptr] = buff[inptr];
		inptr++;
		outptr++;
		if(outptr>STR_MAX_LEN)
			return 1;
	}
	tmp[outptr] = 0;
	if(strtoul(tmp,NULL,10) == 4294967295)
		return 1;
	set_stp_port_flags(num,(u8)strtoul(tmp,NULL,10));
	inptr++;
	if(buff[inptr]!=']')
		return 1;
	//DEBUG_MSG(SETTINGS_DBG,"stp p cost %d\r\n",get_stp_port_flags(num));
	return 0;
}


#define GET_IP(buff,funct,sett_queue) {\
		if(find_ip(buff,ip)==0){\
			if(funct == SETT_DIFF){\
				sett_queue;\
			}valid_var++;\
		}\
	}

#define GET_MAC(buff,funct,sett_queue){if(find_mac(buff,mac)==0){if(funct == SETT_DIFF){sett_queue;}valid_var++;}}

#define GET_U32(buff,funct,sett_queue){if(find_u32(buff,&var_u32)==0){if(funct == SETT_DIFF){sett_queue;}valid_var++;}}
#define GET_U32_PB(buff,funct) {if(find_u32(buff,&var_u32)==0){funct;valid_var++;}}
#define GET_U8 GET_U32
#define GET_STR(buff,funct,sett_queue){if(find_str(buff,tmpstr)==0){if(funct == SETT_DIFF){sett_queue;}valid_var++;}}
#define GET_ENGINE_ID(buff,funct1,funct2,sett_queue){if(find_str(buff,tmpstr)==0){funct1;if(funct2 == SETT_DIFF){sett_queue;}valid_var++;}}
#define GET_ARR(buff,arr,size); {if(find_arr(buff,arr,size)==0){valid_var++;}}
#define GET_VLANREC(buff,num) {if(find_vlan(buff,num)==0){valid_var++;}}
#define GET_STP(buff,num) {if(find_stp(buff,num)==0){valid_var++;}}

u32 parse_bak_file(void){
char tmp[256];
char var_name[VAR_MAX_LEN];
u32 curr_ptr;//указатель на текущую позицию
u8 incrptr;
u8 i;
u32 valid_var = 0;//инкремент если верная переменная

uip_ipaddr_t ip;
u8 mac[6];
u32 var_u32;
char tmpstr[256];
u8 cos[64];


	curr_ptr = 0;

	while(curr_ptr<BKP_FILE_SIZE){

		curr_ptr -= curr_ptr%2;//делаем кратным 2
		spi_flash_read(curr_ptr,VAR_MAX_LEN*2,tmp);

		//vTaskDelay(100*MSEC);
		IWDG_ReloadCounter();

		incrptr = 0;
		if(find_var_name(tmp,var_name,&incrptr)==0){
			if(strlen(var_name))
				DEBUG_MSG(SETTINGS_DBG,"var = %s \r\n",var_name);
			curr_ptr+=(incrptr);
			//get val
			curr_ptr -= curr_ptr%2;//делаем кратным 2
			spi_flash_read(curr_ptr,256,tmp);//read 256

			//netsettings
			IF_CMP(IPADDRESS_NAME){GET_IP(tmp,set_net_ip(ip),settings_add2queue(SQ_NETWORK))}


			IF_CMP(NETMASK_NAME){GET_IP(tmp,set_net_mask(ip),settings_add2queue(SQ_NETWORK))}

			IF_CMP(GATEWAY_NAME){GET_IP(tmp,set_net_gate(ip),settings_add2queue(SQ_NETWORK))}

			IF_CMP(USER_MAC_NAME){GET_MAC(tmp,set_net_mac(mac),settings_add2queue(SQ_NETWORK))}

			IF_CMP(DHCPMODE_NAME){GET_U8(tmp,set_dhcp_mode((u8)var_u32),settings_add2queue(SQ_NETWORK))}

			IF_CMP(DHCPRELAYIP_NAME){GET_IP(tmp,set_dhcp_server_addr(ip),settings_add2queue(SQ_NETWORK))}

			IF_CMP(DNS_NAME){GET_IP(tmp,set_net_dns(ip),settings_add2queue(SQ_NETWORK))}

			IF_CMP(DHCP_MAX_HOPS_NAME){GET_U8(tmp,set_dhcp_hops((u8)var_u32),settings_add2queue(SQ_NETWORK))}

			IF_CMP(DHCP_OPT82_NAME){GET_U8(tmp,set_dhcp_opt82((u8)var_u32),settings_add2queue(SQ_NETWORK))}

			//interface settings
			IF_CMP(LANG_NAME){GET_U8(tmp,set_dhcp_opt82((u8)var_u32),settings_add2queue(SQ_NETWORK))}

			IF_CMP(HTTP_USERNAME_NAME){GET_STR(tmp,set_interface_users_username(0,tmpstr),settings_add2queue(SQ_CAP))}
			IF_CMP(HTTP_PASSWD_NAME){GET_STR(tmp,set_interface_users_password(0,tmpstr),settings_add2queue(SQ_CAP));}

			IF_CMP(USER1_USERNAME_NAME){GET_STR(tmp,set_interface_users_username(1,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(USER1_PASSWD_NAME){GET_STR(tmp,set_interface_users_password(1,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(USER1_RULE_NAME){GET_U8(tmp,set_interface_users_rule(1,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(USER2_USERNAME_NAME){GET_STR(tmp,set_interface_users_username(2,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(USER2_PASSWD_NAME){GET_STR(tmp,set_interface_users_password(2,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(USER2_RULE_NAME){GET_U8(tmp,set_interface_users_rule(2,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(USER3_USERNAME_NAME){GET_STR(tmp,set_interface_users_username(3,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(USER3_PASSWD_NAME){GET_STR(tmp,set_interface_users_password(3,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(USER3_RULE_NAME){GET_U8(tmp,set_interface_users_rule(3,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(SYSTEM_NAME_NAME){GET_STR(tmp,set_interface_name(tmpstr),settings_add2queue(SQ_CAP));}

			IF_CMP(SYSTEM_LOCATION_NAME){GET_STR(tmp,set_interface_location(tmpstr),settings_add2queue(SQ_CAP));}

			IF_CMP(SYSTEM_CONTACT_NAME){GET_STR(tmp,set_interface_contact(tmpstr),settings_add2queue(SQ_CAP));}


			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_DESCR_NAME){GET_STR(tmp,set_port_descr(0,tmpstr),settings_add2queue(SQ_CAP));}



			//port settings
			IF_CMP(PORT1_STATE_NAME){GET_U8(tmp,set_port_state(0,(u8)var_u32),settings_add2queue(SQ_PORT_1))}
			IF_CMP(PORT2_STATE_NAME){GET_U8(tmp,set_port_state(1,(u8)var_u32),settings_add2queue(SQ_PORT_2))}
			IF_CMP(PORT3_STATE_NAME){GET_U8(tmp,set_port_state(2,(u8)var_u32),settings_add2queue(SQ_PORT_3))}
			IF_CMP(PORT4_STATE_NAME){GET_U8(tmp,set_port_state(3,(u8)var_u32),settings_add2queue(SQ_PORT_4))}
			IF_CMP(PORT5_STATE_NAME){GET_U8(tmp,set_port_state(4,(u8)var_u32),settings_add2queue(SQ_PORT_5))}
			IF_CMP(PORT6_STATE_NAME){GET_U8(tmp,set_port_state(5,(u8)var_u32),settings_add2queue(SQ_PORT_6))}
			IF_CMP(PORT7_STATE_NAME){GET_U8(tmp,set_port_state(6,(u8)var_u32),settings_add2queue(SQ_PORT_7))}
			IF_CMP(PORT8_STATE_NAME){GET_U8(tmp,set_port_state(7,(u8)var_u32),settings_add2queue(SQ_PORT_8))}
			IF_CMP(PORT9_STATE_NAME){GET_U8(tmp,set_port_state(8,(u8)var_u32),settings_add2queue(SQ_PORT_9))}
			IF_CMP(PORT10_STATE_NAME){GET_U8(tmp,set_port_state(9,(u8)var_u32),settings_add2queue(SQ_PORT_10))}
			IF_CMP(PORT11_STATE_NAME){GET_U8(tmp,set_port_state(10,(u8)var_u32),settings_add2queue(SQ_PORT_11))}
			IF_CMP(PORT12_STATE_NAME){GET_U8(tmp,set_port_state(11,(u8)var_u32),settings_add2queue(SQ_PORT_12))}
			IF_CMP(PORT13_STATE_NAME){GET_U8(tmp,set_port_state(12,(u8)var_u32),settings_add2queue(SQ_PORT_13))}
			IF_CMP(PORT14_STATE_NAME){GET_U8(tmp,set_port_state(13,(u8)var_u32),settings_add2queue(SQ_PORT_14))}
			IF_CMP(PORT15_STATE_NAME){GET_U8(tmp,set_port_state(14,(u8)var_u32),settings_add2queue(SQ_PORT_15))}
			IF_CMP(PORT16_STATE_NAME){GET_U8(tmp,set_port_state(15,(u8)var_u32),settings_add2queue(SQ_PORT_16))}

			IF_CMP(PORT1_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(0,(u8)var_u32),settings_add2queue(SQ_PORT_1))}
			IF_CMP(PORT2_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(1,(u8)var_u32),settings_add2queue(SQ_PORT_2))}
			IF_CMP(PORT3_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(2,(u8)var_u32),settings_add2queue(SQ_PORT_3))}
			IF_CMP(PORT4_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(3,(u8)var_u32),settings_add2queue(SQ_PORT_4))}
			IF_CMP(PORT5_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(4,(u8)var_u32),settings_add2queue(SQ_PORT_5))}
			IF_CMP(PORT6_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(5,(u8)var_u32),settings_add2queue(SQ_PORT_6))}
			IF_CMP(PORT7_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(6,(u8)var_u32),settings_add2queue(SQ_PORT_7))}
			IF_CMP(PORT8_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(7,(u8)var_u32),settings_add2queue(SQ_PORT_8))}
			IF_CMP(PORT9_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(8,(u8)var_u32),settings_add2queue(SQ_PORT_9))}
			IF_CMP(PORT10_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(9,(u8)var_u32),settings_add2queue(SQ_PORT_10))}
			IF_CMP(PORT11_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(10,(u8)var_u32),settings_add2queue(SQ_PORT_11))}
			IF_CMP(PORT12_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(11,(u8)var_u32),settings_add2queue(SQ_PORT_12))}
			IF_CMP(PORT13_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(12,(u8)var_u32),settings_add2queue(SQ_PORT_13))}
			IF_CMP(PORT14_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(13,(u8)var_u32),settings_add2queue(SQ_PORT_14))}
			IF_CMP(PORT15_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(14,(u8)var_u32),settings_add2queue(SQ_PORT_15))}
			IF_CMP(PORT16_SPEEDDPLX_NAME){GET_U8(tmp,set_port_speed_dplx(15,(u8)var_u32),settings_add2queue(SQ_PORT_16))}

			IF_CMP(PORT1_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(0,(u8)var_u32),settings_add2queue(SQ_PORT_1))}
			IF_CMP(PORT2_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(1,(u8)var_u32),settings_add2queue(SQ_PORT_2))}
			IF_CMP(PORT3_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(2,(u8)var_u32),settings_add2queue(SQ_PORT_3))}
			IF_CMP(PORT4_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(3,(u8)var_u32),settings_add2queue(SQ_PORT_4))}
			IF_CMP(PORT5_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(4,(u8)var_u32),settings_add2queue(SQ_PORT_5))}
			IF_CMP(PORT6_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(5,(u8)var_u32),settings_add2queue(SQ_PORT_6))}
			IF_CMP(PORT7_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(6,(u8)var_u32),settings_add2queue(SQ_PORT_7))}
			IF_CMP(PORT8_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(7,(u8)var_u32),settings_add2queue(SQ_PORT_8))}
			IF_CMP(PORT9_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(8,(u8)var_u32),settings_add2queue(SQ_PORT_9))}
			IF_CMP(PORT10_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(9,(u8)var_u32),settings_add2queue(SQ_PORT_10))}
			IF_CMP(PORT11_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(10,(u8)var_u32),settings_add2queue(SQ_PORT_11))}
			IF_CMP(PORT12_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(11,(u8)var_u32),settings_add2queue(SQ_PORT_12))}
			IF_CMP(PORT13_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(12,(u8)var_u32),settings_add2queue(SQ_PORT_13))}
			IF_CMP(PORT14_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(13,(u8)var_u32),settings_add2queue(SQ_PORT_14))}
			IF_CMP(PORT15_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(14,(u8)var_u32),settings_add2queue(SQ_PORT_15))}
			IF_CMP(PORT16_FLOWCTRL_NAME){GET_U8(tmp,set_port_flow(15,(u8)var_u32),settings_add2queue(SQ_PORT_16))}

			IF_CMP(PORT1_WDT_NAME){GET_U8(tmp,set_port_wdt(0,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT2_WDT_NAME){GET_U8(tmp,set_port_wdt(1,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT3_WDT_NAME){GET_U8(tmp,set_port_wdt(2,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT4_WDT_NAME){GET_U8(tmp,set_port_wdt(3,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT5_WDT_NAME){GET_U8(tmp,set_port_wdt(4,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT6_WDT_NAME){GET_U8(tmp,set_port_wdt(5,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT7_WDT_NAME){GET_U8(tmp,set_port_wdt(6,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT8_WDT_NAME){GET_U8(tmp,set_port_wdt(7,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT9_WDT_NAME){GET_U8(tmp,set_port_wdt(8,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT10_WDT_NAME){GET_U8(tmp,set_port_wdt(9,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT11_WDT_NAME){GET_U8(tmp,set_port_wdt(10,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT12_WDT_NAME){GET_U8(tmp,set_port_wdt(11,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT13_WDT_NAME){GET_U8(tmp,set_port_wdt(12,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT14_WDT_NAME){GET_U8(tmp,set_port_wdt(13,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT15_WDT_NAME){GET_U8(tmp,set_port_wdt(14,(u8)var_u32),settings_add2queue(SQ_CAP))}
			IF_CMP(PORT16_WDT_NAME){GET_U8(tmp,set_port_wdt(15,(u8)var_u32),settings_add2queue(SQ_CAP))}


			IF_CMP(SFP1_MODE_NAME){GET_U8(tmp,set_port_sfp_mode(GE1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(SFP2_MODE_NAME){GET_U8(tmp,set_port_sfp_mode(GE2,(u8)var_u32),settings_add2queue(SQ_CAP));}


			IF_CMP(PORT1_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(0,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT2_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(1,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT3_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(2,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT4_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(3,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(4,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT6_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(5,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT7_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(6,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT8_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(7,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT9_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(8,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT10_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(9,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT11_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(10,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT12_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(11,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT13_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(12,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT14_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(13,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT15_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(14,ip),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT16_IPADDR_NAME){GET_IP(tmp,set_port_wdt_ip(15,ip),settings_add2queue(SQ_CAP));}

			IF_CMP(PORT1_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT2_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT3_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT4_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT6_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT7_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(6,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT8_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(7,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT9_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(8,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT10_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(9,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT11_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(10,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT12_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(11,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT13_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(12,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT14_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(13,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT15_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(14,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT16_WDT_SPEED_NAME){GET_U8(tmp,set_port_wdt_speed_down(15,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PORT1_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT2_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT3_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT4_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT6_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT7_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(6,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT8_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(7,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT9_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(8,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT10_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(9,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT11_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(10,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT12_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(11,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT13_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(12,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT14_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(13,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT15_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(14,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT16_WDT_SPEED_UP_NAME){GET_U8(tmp,set_port_wdt_speed_up(15,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PORT_SOFTSTART_TIME_NAME){GET_U8(tmp,set_softstart_time((u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT1_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT2_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT3_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT4_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT6_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT7_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(6,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT8_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(7,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT9_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(8,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT10_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(9,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT11_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(10,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT12_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(11,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT13_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(12,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT14_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(13,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT15_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(14,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT16_SOFTSTART_NAME){GET_U8(tmp,set_port_soft_start(15,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PORT1_POE_NAME){ GET_U8(tmp,set_port_poe(0,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT2_POE_NAME){ GET_U8(tmp,set_port_poe(1,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT3_POE_NAME){ GET_U8(tmp,set_port_poe(2,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT4_POE_NAME){ GET_U8(tmp,set_port_poe(3,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT5_POE_NAME){ GET_U8(tmp,set_port_poe(4,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT6_POE_NAME){ GET_U8(tmp,set_port_poe(5,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT7_POE_NAME){ GET_U8(tmp,set_port_poe(6,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT8_POE_NAME){ GET_U8(tmp,set_port_poe(7,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT9_POE_NAME){ GET_U8(tmp,set_port_poe(8,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT10_POE_NAME){GET_U8(tmp,set_port_poe(9,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT11_POE_NAME){GET_U8(tmp,set_port_poe(10,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT12_POE_NAME){GET_U8(tmp,set_port_poe(11,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT13_POE_NAME){GET_U8(tmp,set_port_poe(12,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT14_POE_NAME){GET_U8(tmp,set_port_poe(13,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT15_POE_NAME){GET_U8(tmp,set_port_poe(14,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT16_POE_NAME){GET_U8(tmp,set_port_poe(15,(u8)var_u32),settings_add2queue(SQ_POE));}

			IF_CMP(PORT1_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(0,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT2_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(1,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT3_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(2,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT4_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(3,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT5_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(4,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT6_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(5,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT7_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(6,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT8_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(7,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT9_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(8,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT10_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(9,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT11_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(10,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT12_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(11,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT13_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(12,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT14_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(13,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT15_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(14,(u8)var_u32),settings_add2queue(SQ_POE));}
			IF_CMP(PORT16_POE_LIM_NAME){ GET_U8(tmp,set_port_pwr_lim_a(15,(u8)var_u32),settings_add2queue(SQ_POE));}

			//smtp settings
			IF_CMP(SMTP_STATE_NAME) {GET_U8(tmp,set_smtp_state((u8)var_u32),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_SERV_IP_NAME) {GET_IP(tmp,set_smtp_server(ip),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_TO1_NAME) {GET_STR(tmp,set_smtp_to(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_TO2_NAME) {GET_STR(tmp,set_smtp_to2(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_TO3_NAME) {GET_STR(tmp,set_smtp_to3(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_FROM_NAME) {GET_STR(tmp,set_smtp_from(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_SUBJ_NAME) {GET_STR(tmp,set_smtp_subj(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_LOGIN_NAME) {GET_STR(tmp,set_smtp_login(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_PASS_NAME) {GET_STR(tmp,set_smtp_pass(tmpstr),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_PORT_NAME) {GET_U8(tmp,set_smtp_port((u16)var_u32),settings_add2queue(SQ_SMTP))}

			IF_CMP(SMTP_DOMAIN_NAME_NAME) {GET_STR(tmp,set_smtp_domain(tmpstr),settings_add2queue(SQ_SMTP))}


			//SNTP settings
			IF_CMP(SNTP_STATE_NAME) {GET_U8(tmp,set_sntp_state((u8)var_u32),settings_add2queue(SQ_SNTP))}

			IF_CMP(SNTP_SETT_SERV_NAME) {GET_IP(tmp,set_sntp_serv(ip),settings_add2queue(SQ_SNTP))}

			IF_CMP(SNTP_SERV_NAME_NAME) {GET_STR(tmp,set_sntp_serv_name(tmpstr),settings_add2queue(SQ_SNTP))}

			IF_CMP(SNTP_TIMEZONE_NAME) {GET_U8(tmp,set_sntp_timezone((i8)var_u32),settings_add2queue(SQ_SNTP))}

			IF_CMP(SNTP_PERIOD_NAME) {GET_U8(tmp,set_sntp_period((u8)var_u32),settings_add2queue(SQ_SNTP))}


			//syslog settings
			IF_CMP(SYSLOG_STATE_NAME) {GET_U8(tmp,set_syslog_state((u8)var_u32),settings_add2queue(SQ_SYSLOG))}
			IF_CMP(SYSLOG_SERV_IP_NAME) {GET_IP(tmp,set_syslog_serv(ip),settings_add2queue(SQ_SYSLOG))}

			//eventlist
			IF_CMP(EVENT_LIST_BASE_S_NAME) {GET_U8(tmp,set_event_base_s((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_PORT_S_NAME) {GET_U8(tmp,set_event_port_s((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_VLAN_S_NAME) {GET_U8(tmp,set_event_vlan_s((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_STP_S_NAME) {GET_U8(tmp,set_event_stp_s((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_QOS_S_NAME) {GET_U8(tmp,set_event_qos_s((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_OTHER_S_NAME) {GET_U8(tmp,set_event_other_s((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_LINK_T_NAME) {GET_U8(tmp,set_event_port_link_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_POE_T_NAME) {GET_U8(tmp,set_event_port_poe_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_STP_T_NAME) {GET_U8(tmp,set_event_stp_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_ARLINK_T_NAME) {GET_U8(tmp,set_event_spec_link_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_ARPING_T_NAME) {GET_U8(tmp,set_event_spec_ping_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_ARSPEED_T_NAME) {GET_U8(tmp,set_event_spec_speed_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_SYSTEM_T_NAME) {GET_U8(tmp,set_event_system_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_UPS_T_NAME) {GET_U8(tmp,set_event_ups_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}
			IF_CMP(EVENT_LIST_ACCESS_T_NAME) {GET_U8(tmp,set_event_alarm_t((u8)(var_u32>>3),(u8)(var_u32&0x07)),settings_add2queue(SQ_CAP));}


			//dry contact
			IF_CMP(DRY_CONT0_STATE_NAME) {GET_U8(tmp,set_alarm_state(0,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(DRY_CONT1_STATE_NAME) {GET_U8(tmp,set_alarm_state(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(DRY_CONT1_LEVEL_NAME) {GET_U8(tmp,set_alarm_front(1,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(DRY_CONT2_STATE_NAME) {GET_U8(tmp,set_alarm_state(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(DRY_CONT2_LEVEL_NAME) {GET_U8(tmp,set_alarm_front(2,(u8)var_u32),settings_add2queue(SQ_CAP));}


			//QoS settings
			IF_CMP(PORT1_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(0,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT2_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(1,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT3_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(2,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT4_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(3,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT5_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(4,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT6_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(5,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT7_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(6,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT8_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(7,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT9_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(8,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT10_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(9,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT11_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(10,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT12_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(11,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT13_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(12,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT14_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(13,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT15_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(14,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT16_RATE_LIMIT_RX_NAME) {GET_U32(tmp,set_rate_limit_rx(15,var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(PORT1_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(0,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT2_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(1,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT3_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(2,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT4_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(3,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT5_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(4,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT6_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(5,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT7_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(6,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT8_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(7,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT9_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(8,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT10_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(9,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT11_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(10,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT12_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(11,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT13_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(12,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT14_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(13,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT15_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(14,var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT16_RATE_LIMIT_TX_NAME) {GET_U32(tmp,set_rate_limit_tx(15,var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(QOS_STATE_NAME) {GET_U8(tmp,set_qos_state((u8)var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(QOS_POLICY_NAME) {GET_U8(tmp,set_qos_policy((u8)var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(QOS_COS_NAME) {
				GET_ARR(tmp,cos,8);
				for(i=0;i<8;i++)
					set_qos_cos(i,cos[i] );
				settings_add2queue(SQ_QOS);
			}


			IF_CMP(QOS_TOS_NAME) {
				GET_ARR(tmp,cos,64);
				for(i=0;i<64;i++)
					set_qos_tos(i,cos[i]);
				settings_add2queue(SQ_QOS);
			}


			IF_CMP(PORT1_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(0,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT2_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(1,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT3_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(2,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT4_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(3,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT5_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(4,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT6_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(5,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT7_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(6,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT8_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(7,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT9_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(8,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT10_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(9,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT11_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(10,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT12_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(11,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT13_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(12,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT14_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(13,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT15_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(14,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT16_COS_STATE_NAME) {GET_U8(tmp,set_qos_port_cos_state(15,(u8)var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(PORT1_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(0,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT2_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(1,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT3_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(2,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT4_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(3,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT5_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(4,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT6_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(5,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT7_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(6,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT8_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(7,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT9_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(8,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT10_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(9,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT11_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(10,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT12_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(11,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT13_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(12,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT14_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(13,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT15_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(14,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT16_TOS_STATE_NAME) {GET_U8(tmp,set_qos_port_tos_state(15,(u8)var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(PORT1_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(0,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT2_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(1,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT3_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(2,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT4_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(3,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT5_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(4,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT6_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(5,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT7_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(6,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT8_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(7,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT9_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(8,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT10_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(9,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT11_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(10,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT12_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(11,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT13_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(12,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT14_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(13,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT15_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(14,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT16_QOS_RULE_NAME) {GET_U8(tmp,set_qos_port_rule(15,(u8)var_u32),settings_add2queue(SQ_QOS));}

			IF_CMP(PORT1_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(0,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT2_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(1,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT3_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(2,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT4_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(3,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT5_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(4,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT6_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(5,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT7_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(6,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT8_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(7,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT9_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(8,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT10_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(9,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT11_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(10,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT12_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(11,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT13_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(12,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT14_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(13,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT15_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(14,(u8)var_u32),settings_add2queue(SQ_QOS));}
			IF_CMP(PORT16_COS_PRI_NAME) {GET_U8(tmp,set_qos_port_def_pri(15,(u8)var_u32),settings_add2queue(SQ_QOS));}

			//port based vlan
			IF_CMP(PBVLAN_STATE_NAME) {GET_U8(tmp,set_pb_vlan_state((u8)var_u32),settings_add2queue(SQ_PBVLAN));}

			IF_CMP(PBVLAN_PORT1_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(0,i,1);else set_pb_vlan_port(0,i,0);)}
			IF_CMP(PBVLAN_PORT2_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(1,i,1);else set_pb_vlan_port(1,i,0);)}
			IF_CMP(PBVLAN_PORT3_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(2,i,1);else set_pb_vlan_port(2,i,0);)}
			IF_CMP(PBVLAN_PORT4_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(3,i,1);else set_pb_vlan_port(3,i,0);)}
			IF_CMP(PBVLAN_PORT5_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(4,i,1);else set_pb_vlan_port(4,i,0);)}
			IF_CMP(PBVLAN_PORT6_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(5,i,1);else set_pb_vlan_port(5,i,0);)}
			IF_CMP(PBVLAN_PORT7_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(6,i,1);else set_pb_vlan_port(6,i,0);)}
			IF_CMP(PBVLAN_PORT8_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(7,i,1);else set_pb_vlan_port(7,i,0);)}
			IF_CMP(PBVLAN_PORT9_NAME) {GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(8,i,1);else set_pb_vlan_port(8,i,0);)}
			IF_CMP(PBVLAN_PORT10_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(9,i,1);else set_pb_vlan_port(9,i,0);)}
			IF_CMP(PBVLAN_PORT11_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(10,i,1);else set_pb_vlan_port(10,i,0);)}
			IF_CMP(PBVLAN_PORT12_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(11,i,1);else set_pb_vlan_port(11,i,0);)}
			IF_CMP(PBVLAN_PORT13_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(12,i,1);else set_pb_vlan_port(12,i,0);)}
			IF_CMP(PBVLAN_PORT14_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(13,i,1);else set_pb_vlan_port(13,i,0);)}
			IF_CMP(PBVLAN_PORT15_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(14,i,1);else set_pb_vlan_port(14,i,0);)}
			IF_CMP(PBVLAN_PORT16_NAME){GET_U32_PB(tmp,for(u8 i=0;i<ALL_PORT_NUM;i++)if(var_u32 & (1<<i)) set_pb_vlan_port(15,i,1);else set_pb_vlan_port(15,i,0);)}

			//port based vlan for swu
			IF_CMP(PORT1_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(0,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT2_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(1,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT3_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(2,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT4_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(3,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT5_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(4,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT6_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(5,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT7_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(6,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT8_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(7,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT9_PB_VLAN_SWU_NAME) {GET_U32(tmp,set_pb_vlan_swu_port(8,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT10_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(9,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT11_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(10,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT12_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(11,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT13_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(12,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT14_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(13,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT15_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(14,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT16_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(15,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT17_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(16,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT18_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(17,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT19_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(18,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}
			IF_CMP(PORT20_PB_VLAN_SWU_NAME){GET_U32(tmp,set_pb_vlan_swu_port(19,(u8)var_u32),settings_add2queue(SQ_PBVLAN));}


			//vlan settings
			IF_CMP(VLAN_MVID_NAME) {GET_U8(tmp,set_vlan_sett_mngt((u16)var_u32),settings_add2queue(SQ_VLAN));}

			IF_CMP(VLAN_NUM_NAME) {GET_U8(tmp,set_vlan_sett_vlannum((u16)var_u32),settings_add2queue(SQ_VLAN));}

			for(i=0;i<get_vlan_sett_vlannum();i++){
				switch(i){
					case 0:IF_CMP(VLAN1_NAME){GET_VLANREC(tmp,0);}break;
					case 1:IF_CMP(VLAN2_NAME){GET_VLANREC(tmp,1);}break;
					case 2:IF_CMP(VLAN3_NAME){GET_VLANREC(tmp,2);}break;
					case 3:IF_CMP(VLAN4_NAME){GET_VLANREC(tmp,3);}break;
					case 4:IF_CMP(VLAN5_NAME){GET_VLANREC(tmp,4);}break;
					case 5:IF_CMP(VLAN6_NAME){GET_VLANREC(tmp,5);}break;
					case 6:IF_CMP(VLAN7_NAME){GET_VLANREC(tmp,6);}break;
					case 7:IF_CMP(VLAN8_NAME){GET_VLANREC(tmp,7);}break;
					case 8:IF_CMP(VLAN9_NAME){GET_VLANREC(tmp,8);}break;
					case 9:IF_CMP(VLAN10_NAME){GET_VLANREC(tmp,9);}break;
					case 10:IF_CMP(VLAN11_NAME){GET_VLANREC(tmp,10);}break;
					case 11:IF_CMP(VLAN12_NAME){GET_VLANREC(tmp,11);}break;
					case 12:IF_CMP(VLAN13_NAME){GET_VLANREC(tmp,12);}break;
					case 13:IF_CMP(VLAN14_NAME){GET_VLANREC(tmp,13);}break;
					case 14:IF_CMP(VLAN15_NAME){GET_VLANREC(tmp,14);}break;
					case 15:IF_CMP(VLAN16_NAME){GET_VLANREC(tmp,15);}break;
					case 16:IF_CMP(VLAN17_NAME){GET_VLANREC(tmp,16);}break;
					case 17:IF_CMP(VLAN18_NAME){GET_VLANREC(tmp,17);}break;
					case 18:IF_CMP(VLAN19_NAME){GET_VLANREC(tmp,18);}break;
					case 19:IF_CMP(VLAN20_NAME){GET_VLANREC(tmp,19);}break;
					case 20:IF_CMP(VLAN21_NAME){GET_VLANREC(tmp,20);}break;
					case 21:IF_CMP(VLAN22_NAME){GET_VLANREC(tmp,21);}break;
					case 22:IF_CMP(VLAN23_NAME){GET_VLANREC(tmp,22);}break;
					case 23:IF_CMP(VLAN24_NAME){GET_VLANREC(tmp,23);}break;
					case 24:IF_CMP(VLAN25_NAME){GET_VLANREC(tmp,24);}break;
					case 25:IF_CMP(VLAN26_NAME){GET_VLANREC(tmp,25);}break;
					case 26:IF_CMP(VLAN27_NAME){GET_VLANREC(tmp,26);}break;
					case 27:IF_CMP(VLAN28_NAME){GET_VLANREC(tmp,27);}break;
					case 28:IF_CMP(VLAN29_NAME){GET_VLANREC(tmp,28);}break;
					case 29:IF_CMP(VLAN30_NAME){GET_VLANREC(tmp,29);}break;
					case 30:IF_CMP(VLAN31_NAME){GET_VLANREC(tmp,30);}break;
					case 31:IF_CMP(VLAN32_NAME){GET_VLANREC(tmp,31);}break;
					case 32:IF_CMP(VLAN33_NAME){GET_VLANREC(tmp,32);}break;
					case 33:IF_CMP(VLAN34_NAME){GET_VLANREC(tmp,33);}break;
					case 34:IF_CMP(VLAN35_NAME){GET_VLANREC(tmp,34);}break;
					case 35:IF_CMP(VLAN36_NAME){GET_VLANREC(tmp,35);}break;
					case 36:IF_CMP(VLAN37_NAME){GET_VLANREC(tmp,36);}break;
					case 37:IF_CMP(VLAN38_NAME){GET_VLANREC(tmp,37);}break;
					case 38:IF_CMP(VLAN39_NAME){GET_VLANREC(tmp,38);}break;
					case 39:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,39);}break;
					case 40:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,40);}break;
					case 41:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,41);}break;
					case 42:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,42);}break;
					case 43:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,43);}break;
					case 44:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,44);}break;
					case 45:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,45);}break;
					case 46:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,46);}break;
					case 47:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,47);}break;
					case 48:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,48);}break;
					case 49:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,49);}break;
					case 50:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,50);}break;
					case 51:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,51);}break;
					case 52:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,52);}break;
					case 53:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,53);}break;
					case 54:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,54);}break;
					case 55:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,55);}break;
					case 56:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,56);}break;
					case 57:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,57);}break;
					case 58:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,58);}break;
					case 59:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,59);}break;
					case 60:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,60);}break;
					case 61:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,61);}break;
					case 62:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,62);}break;
					case 63:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,63);}break;
					case 64:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,64);}break;
					case 65:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,65);}break;
					case 66:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,66);}break;
					case 67:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,67);}break;
					case 68:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,68);}break;
					case 69:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,69);}break;
					case 70:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,70);}break;
					case 71:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,71);}break;
					case 72:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,72);}break;
					case 73:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,73);}break;
					case 74:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,74);}break;
					case 75:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,75);}break;
					case 76:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,76);}break;
					case 77:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,77);}break;
					case 78:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,78);}break;
					case 79:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,79);}break;
					case 80:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,80);}break;
					case 81:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,81);}break;
					case 82:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,82);}break;
					case 83:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,83);}break;
					case 84:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,84);}break;
					case 85:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,85);}break;
					case 86:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,86);}break;
					case 87:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,87);}break;
					case 88:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,88);}break;
					case 89:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,89);}break;
					case 90:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,90);}break;
					case 91:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,91);}break;
					case 92:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,92);}break;
					case 93:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,93);}break;
					case 94:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,94);}break;
					case 95:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,95);}break;
					case 96:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,96);}break;
					case 97:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,97);}break;
					case 98:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,98);}break;
					case 99:IF_CMP(VLAN40_NAME){GET_VLANREC(tmp,99);}break;
				}
			}

			//VLAN Trunking settings
			IF_CMP(VLAN_TRUNK_STATE_NAME) {GET_U8(tmp,set_vlan_trunk_state((u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT1_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(0,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT2_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(1,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT3_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(2,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT4_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(3,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT5_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(4,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT6_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(5,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT7_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(6,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT8_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(7,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT9_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(8,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT10_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(9,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT11_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(10,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT12_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(11,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT13_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(12,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT14_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(13,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT15_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(14,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}
			IF_CMP(VLAN_PORT16_STATE_NAME) {GET_U8(tmp,set_vlan_sett_port_state(15,(u8)var_u32),settings_add2queue(SQ_VTRUNK));}




			//rstp settings
			IF_CMP(STP_STATE_NAME) {GET_U8(tmp,set_stp_state((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_MAGIC_NAME) {GET_U8(tmp,set_stp_magic((u16)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_PROTO_NAME) {GET_U8(tmp,set_stp_proto((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_BRIDGE_PRIOR_NAME) {GET_U8(tmp,set_stp_bridge_priority((u16)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_MAX_AGE_NAME) {GET_U8(tmp,set_stp_bridge_max_age((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_HELLO_TIME_NAME) {GET_U8(tmp,set_stp_bridge_htime((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_FORW_DELAY_NAME) {GET_U8(tmp,set_stp_bridge_fdelay((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_MIGRATE_DELAY_NAME) {GET_U8(tmp,set_stp_bridge_mdelay((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_TX_HCOUNT_NAME) {GET_U8(tmp,set_stp_txholdcount((u8)var_u32),settings_add2queue(SQ_STP));}

			IF_CMP(STP_PORT1_CFG_NAME) GET_STP(tmp,0);
			IF_CMP(STP_PORT2_CFG_NAME) GET_STP(tmp,1);
			IF_CMP(STP_PORT3_CFG_NAME) GET_STP(tmp,2);
			IF_CMP(STP_PORT4_CFG_NAME) GET_STP(tmp,3);
			IF_CMP(STP_PORT5_CFG_NAME) GET_STP(tmp,4);
			IF_CMP(STP_PORT6_CFG_NAME) GET_STP(tmp,5);
			IF_CMP(STP_PORT7_CFG_NAME) GET_STP(tmp,6);
			IF_CMP(STP_PORT8_CFG_NAME) GET_STP(tmp,7);
			IF_CMP(STP_PORT9_CFG_NAME) GET_STP(tmp,8);
			IF_CMP(STP_PORT10_CFG_NAME)GET_STP(tmp,9);
			IF_CMP(STP_PORT11_CFG_NAME)GET_STP(tmp,10);
			IF_CMP(STP_PORT12_CFG_NAME)GET_STP(tmp,11);
			IF_CMP(STP_PORT13_CFG_NAME)GET_STP(tmp,12);
			IF_CMP(STP_PORT14_CFG_NAME)GET_STP(tmp,13);
			IF_CMP(STP_PORT15_CFG_NAME)GET_STP(tmp,14);
			IF_CMP(STP_PORT16_CFG_NAME)GET_STP(tmp,15);

			IF_CMP(BPDU_FORWARD_NAME) {GET_U8(tmp,set_stp_bpdu_fw((u8)var_u32),settings_add2queue(SQ_STP));}


			//virtual cable tester settings
			IF_CMP(PORT1_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(0,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT2_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(1,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT3_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(2,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT4_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(3,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(4,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(5,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT7_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(6,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT8_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(7,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT9_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(8,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT10_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(9,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT11_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(10,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT12_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(11,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT13_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(12,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT14_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(13,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT15_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(14,(u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT16_VCT_ADJ_NAME) {GET_U8(tmp,set_callibrate_koef_1(15,(u16)var_u32),settings_add2queue(SQ_CAP));}


			IF_CMP(PORT1_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT2_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT3_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT4_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT5_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT6_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT7_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(6,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT8_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(7,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT9_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(8,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT10_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(9,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT11_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(10,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT12_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(11,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT13_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(12,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT14_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(13,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT15_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(14,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PORT16_VCT_LEN_NAME) {GET_U8(tmp,set_callibrate_len(15,(u8)var_u32),settings_add2queue(SQ_CAP));}


			//snmp settings
			IF_CMP(SNMP_STATE_NAME) {GET_U8(tmp,set_snmp_state((u8)var_u32),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMP_SERVER_NAME) {GET_IP(tmp,set_snmp_serv(ip),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMP_VERS_NAME) {GET_U8(tmp,set_snmp_vers((u8)var_u32),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMP_COMMUNITY1_NAME) {GET_STR(tmp,set_snmp1_read_communitie(tmpstr),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMP_COMMUNITY2_NAME) {GET_STR(tmp,set_snmp1_write_communitie(tmpstr),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMPV3_USER1_LEVEL)     {GET_U8(tmp,set_snmp3_level(0,(u8)var_u32),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER1_USER_NAME) {GET_STR(tmp,set_snmp3_user_name(0,tmpstr),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER1_AUTH_PASS) {GET_STR(tmp,set_snmp3_auth_pass(0,tmpstr),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER1_PRIV_PASS) {GET_STR(tmp,set_snmp3_priv_pass(0,tmpstr),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMPV3_USER2_LEVEL)     {GET_U8(tmp,set_snmp3_level(1,(u8)var_u32),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER2_USER_NAME) {GET_STR(tmp,set_snmp3_user_name(1,tmpstr),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER2_AUTH_PASS) {GET_STR(tmp,set_snmp3_auth_pass(1,tmpstr),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER2_PRIV_PASS) {GET_STR(tmp,set_snmp3_priv_pass(1,tmpstr),settings_add2queue(SQ_SNMP));}

			IF_CMP(SNMPV3_USER3_LEVEL)     {GET_U8(tmp,set_snmp3_level(2,(u8)var_u32),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER3_USER_NAME) {GET_STR(tmp,set_snmp3_user_name(2,tmpstr),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER3_AUTH_PASS) {GET_STR(tmp,set_snmp3_auth_pass(2,tmpstr),settings_add2queue(SQ_SNMP));}
			IF_CMP(SNMPV3_USER3_PRIV_PASS) {GET_STR(tmp,set_snmp3_priv_pass(2,tmpstr),settings_add2queue(SQ_SNMP));}

			engine_id_t eid;
			IF_CMP(SNMP3_ENGINE_ID_NAME) {GET_ENGINE_ID(tmp,str_to_engineid(tmpstr,&eid),set_snmp3_engine_id(&eid),settings_add2queue(SQ_SNMP));}

			//igmp settings
			IF_CMP(IGMP_PORT_1_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(0,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_2_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(1,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_3_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(2,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_4_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(3,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_5_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(4,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_6_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(5,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_7_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(6,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_8_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(7,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_9_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(8,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_10_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(9,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_11_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(10,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_12_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(11,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_13_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(12,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_14_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(13,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_15_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(14,(u8)var_u32),settings_add2queue(SQ_IGMP));}
			IF_CMP(IGMP_PORT_16_STATE_NAME) {GET_U8(tmp,set_igmp_port_state(15,(u8)var_u32),settings_add2queue(SQ_IGMP));}

			IF_CMP(IGMP_QUERY_INTERVAL_NAME) {GET_U8(tmp,set_igmp_query_int((u8)var_u32),settings_add2queue(SQ_IGMP));}

			IF_CMP(IGMP_QUERY_MODE_NAME) {GET_U8(tmp,set_igmp_query_mode((u8)var_u32),settings_add2queue(SQ_IGMP));}

			IF_CMP(IGMP_QUERY_RESP_INTERVAL_NAME) {GET_U8(tmp,set_igmp_max_resp_time((u8)var_u32),settings_add2queue(SQ_IGMP));}

			IF_CMP(IGMP_GROUP_MEMB_TIME_NAME) {GET_U8(tmp,set_igmp_group_membership_time((u8)var_u32),settings_add2queue(SQ_IGMP));}

			IF_CMP(IGMP_OTHER_QUERIER_NAME) {GET_U8(tmp,set_igmp_other_querier_time((u8)var_u32),settings_add2queue(SQ_IGMP));}

			//telnet
			IF_CMP(TELNET_STATE_NAME) {GET_U8(tmp,set_telnet_state((u8)var_u32),settings_add2queue(SQ_TELNET));}

			//downshifting
			IF_CMP(DOWNSHIFT_STATE_NAME) {GET_U8(tmp,set_downshifting_mode((u8)var_u32),settings_add2queue(SQ_CAP));}

			//tftp
			IF_CMP(TFTP_STATE_NAME) {GET_U8(tmp,set_tftp_state((u8)var_u32),settings_add2queue(SQ_TELNET));}
			IF_CMP(TFTP_MODE_NAME) {GET_U8(tmp,set_tftp_mode((u8)var_u32),settings_add2queue(SQ_TELNET));}
			IF_CMP(TFTP_PORT_NAME) {GET_U32(tmp,set_tftp_port((u16)var_u32),settings_add2queue(SQ_TELNET));}


			//PLC settings (option)
			IF_CMP(PLC_OUT1_STATE_NAME) {GET_U8(tmp,set_plc_out_state(0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_STATE_NAME) {GET_U8(tmp,set_plc_out_state(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_STATE_NAME) {GET_U8(tmp,set_plc_out_state(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_STATE_NAME) {GET_U8(tmp,set_plc_out_state(3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_EM_MODEL_NAME) 	{GET_U32(tmp,set_plc_em_model((u16)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_EM_BAUDRATE_NAME){GET_U8(tmp,set_plc_em_rate((u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_EM_PARITY_NAME) 	{GET_U8(tmp,set_plc_em_parity((u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_EM_DATABITS_NAME){GET_U8(tmp,set_plc_em_databits((u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_EM_STOPBITS_NAME){GET_U8(tmp,set_plc_em_stopbits((u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PLC_EM_PASS_NAME){GET_STR(tmp,set_plc_em_pass(tmpstr),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_EM_ID_NAME){GET_STR(tmp,set_plc_em_id(tmpstr),settings_add2queue(SQ_CAP));}

			IF_CMP(PLC_OUT1_ACTION_NAME) {GET_U8(tmp,set_plc_out_action(0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_ACTION_NAME) {GET_U8(tmp,set_plc_out_action(1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_ACTION_NAME) {GET_U8(tmp,set_plc_out_action(2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_ACTION_NAME) {GET_U8(tmp,set_plc_out_action(3,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PLC_OUT1_EVENT1_NAME) {GET_U8(tmp,set_plc_out_event(0,0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT1_EVENT2_NAME) {GET_U8(tmp,set_plc_out_event(0,1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT1_EVENT3_NAME) {GET_U8(tmp,set_plc_out_event(0,2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT1_EVENT4_NAME) {GET_U8(tmp,set_plc_out_event(0,3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT1_EVENT5_NAME) {GET_U8(tmp,set_plc_out_event(0,4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT1_EVENT6_NAME) {GET_U8(tmp,set_plc_out_event(0,5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT1_EVENT7_NAME) {GET_U8(tmp,set_plc_out_event(0,6,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PLC_OUT2_EVENT1_NAME) {GET_U8(tmp,set_plc_out_event(1,0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_EVENT2_NAME) {GET_U8(tmp,set_plc_out_event(1,1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_EVENT3_NAME) {GET_U8(tmp,set_plc_out_event(1,2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_EVENT4_NAME) {GET_U8(tmp,set_plc_out_event(1,3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_EVENT5_NAME) {GET_U8(tmp,set_plc_out_event(1,4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_EVENT6_NAME) {GET_U8(tmp,set_plc_out_event(1,5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT2_EVENT7_NAME) {GET_U8(tmp,set_plc_out_event(1,6,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PLC_OUT3_EVENT1_NAME) {GET_U8(tmp,set_plc_out_event(2,0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_EVENT2_NAME) {GET_U8(tmp,set_plc_out_event(2,1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_EVENT3_NAME) {GET_U8(tmp,set_plc_out_event(2,2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_EVENT4_NAME) {GET_U8(tmp,set_plc_out_event(2,3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_EVENT5_NAME) {GET_U8(tmp,set_plc_out_event(2,4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_EVENT6_NAME) {GET_U8(tmp,set_plc_out_event(2,5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT3_EVENT7_NAME) {GET_U8(tmp,set_plc_out_event(2,6,(u8)var_u32),settings_add2queue(SQ_CAP));}

			IF_CMP(PLC_OUT4_EVENT1_NAME) {GET_U8(tmp,set_plc_out_event(3,0,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_EVENT2_NAME) {GET_U8(tmp,set_plc_out_event(3,1,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_EVENT3_NAME) {GET_U8(tmp,set_plc_out_event(3,2,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_EVENT4_NAME) {GET_U8(tmp,set_plc_out_event(3,3,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_EVENT5_NAME) {GET_U8(tmp,set_plc_out_event(3,4,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_EVENT6_NAME) {GET_U8(tmp,set_plc_out_event(3,5,(u8)var_u32),settings_add2queue(SQ_CAP));}
			IF_CMP(PLC_OUT4_EVENT7_NAME) {GET_U8(tmp,set_plc_out_event(3,6,(u8)var_u32),settings_add2queue(SQ_CAP));}


			//mac learning
			IF_CMP(PORT1_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(0,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT2_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(1,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT3_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(2,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT4_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(3,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT5_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(4,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT6_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(5,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT7_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(6,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT8_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(7,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT9_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(8,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT10_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(9,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT11_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(10,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT12_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(11,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT13_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(12,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT14_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(13,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT15_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(14,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(PORT16_MACFILT_NAME) {GET_U8(tmp,set_mac_filter_state(15,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			//mac filtering
			IF_CMP(MAC_BIND_ENRTY1_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(0,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY1_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(0,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY1_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(0,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY2_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(1,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY2_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(1,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY2_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(1,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY3_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(2,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY3_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(2,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY3_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(2,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY4_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(3,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY4_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(3,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY4_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(3,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY5_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(4,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY5_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(4,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY5_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(4,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY6_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(5,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY6_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(5,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY6_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(5,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY7_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(6,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY7_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(6,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY7_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(6,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY8_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(7,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY8_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(7,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY8_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(7,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY9_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(8,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY9_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(8,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY9_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(8,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY10_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(9,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY10_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(9,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY10_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(9,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY11_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(10,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY11_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(10,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY11_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(10,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY12_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(11,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY12_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(11,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY12_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(11,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY13_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(12,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY13_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(12,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY13_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(12,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY14_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(13,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY14_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(13,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY14_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(13,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY15_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(14,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY15_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(14,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY15_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(14,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY16_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(15,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY16_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(15,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY16_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(15,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY17_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(16,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY17_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(16,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY17_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(16,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY18_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(17,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY18_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(17,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY18_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(17,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY19_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(18,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY19_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(18,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY19_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(18,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY20_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(19,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY20_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(19,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY20_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(19,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY21_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(20,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY21_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(20,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY21_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(20,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY22_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(21,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY22_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(21,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY22_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(21,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY23_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(22,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY23_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(22,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY23_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(22,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY24_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(23,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY24_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(23,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY24_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(23,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY25_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(24,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY25_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(24,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY25_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(24,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY26_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(25,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY26_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(25,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY26_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(25,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY27_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(26,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY27_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(26,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY27_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(26,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY28_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(27,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY28_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(27,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY28_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(27,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY29_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(28,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY29_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(28,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY29_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(28,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY30_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(29,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY30_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(29,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY30_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(29,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY31_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(30,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY31_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(30,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY31_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(30,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY32_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(31,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY32_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(31,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY32_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(31,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
//
			IF_CMP(MAC_BIND_ENRTY33_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(32,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY33_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(32,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY33_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(32,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY34_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(33,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY34_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(33,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY34_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(33,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY35_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(34,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY35_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(34,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY35_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(34,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY36_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(35,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY36_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(35,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY36_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(35,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY37_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(36,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY37_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(36,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY37_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(36,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY38_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(37,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY38_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(37,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY38_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(37,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY39_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(38,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY39_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(38,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY39_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(38,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY40_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(39,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY40_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(39,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY40_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(39,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY41_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(40,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY41_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(40,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY41_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(40,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY42_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(41,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY42_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(41,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY42_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(41,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY43_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(42,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY43_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(42,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY43_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(42,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY44_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(43,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY44_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(43,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY44_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(43,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY45_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(44,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY45_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(44,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY45_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(44,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY46_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(45,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY46_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(45,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY46_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(45,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY47_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(46,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY47_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(46,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY47_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(46,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY48_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(47,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY48_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(47,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY48_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(47,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY49_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(48,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY49_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(48,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY49_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(48,(u8)var_u32),settings_add2queue(SQ_MACFILR));}

			IF_CMP(MAC_BIND_ENRTY50_ACTIVE_NAME) {GET_U8(tmp,set_mac_bind_entry_active(49,(u8)var_u32),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY50_MAC_NAME){GET_MAC(tmp,set_mac_bind_entry_mac(49,mac),settings_add2queue(SQ_MACFILR));}
			IF_CMP(MAC_BIND_ENRTY50_PORT_NAME) {GET_U8(tmp,set_mac_bind_entry_port(49,(u8)var_u32),settings_add2queue(SQ_MACFILR));}


			IF_CMP(UPS_DELAYED_START_NAME) {GET_U8(tmp,set_ups_delayed_start((u8)var_u32),settings_add2queue(SQ_CAP));}

			//teleport settings
			IF_CMP(INPUT1_STATE_NAME) {GET_U8(tmp,set_input_state(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT2_STATE_NAME) {GET_U8(tmp,set_input_state(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT2_STATE_NAME) {GET_U8(tmp,set_input_state(2,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(INPUT1_INVERSE_NAME) {GET_U8(tmp,set_input_inverse(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT2_INVERSE_NAME) {GET_U8(tmp,set_input_inverse(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT3_INVERSE_NAME) {GET_U8(tmp,set_input_inverse(2,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(INPUT1_REMDEV_NAME) {GET_U8(tmp,set_input_remdev(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT2_REMDEV_NAME) {GET_U8(tmp,set_input_remdev(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT3_REMDEV_NAME) {GET_U8(tmp,set_input_remdev(2,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(INPUT1_REMPORT_NAME) {GET_U8(tmp,set_input_remport(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT2_REMPORT_NAME) {GET_U8(tmp,set_input_remport(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(INPUT3_REMPORT_NAME) {GET_U8(tmp,set_input_remport(2,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP_EVENT1_STATE_NAME) {GET_U8(tmp,set_tlp_event_state(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP_EVENT2_STATE_NAME) {GET_U8(tmp,set_tlp_event_state(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP_EVENT1_INVERSE_NAME) {GET_U8(tmp,set_tlp_event_inverse(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP_EVENT2_INVERSE_NAME) {GET_U8(tmp,set_tlp_event_inverse(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP_EVENT1_REMDEV_NAME) {GET_U8(tmp,set_tlp_event_remdev(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP_EVENT2_REMDEV_NAME) {GET_U8(tmp,set_tlp_event_remdev(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP_EVENT1_REMPORT_NAME) {GET_U8(tmp,set_tlp_event_remport(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP_EVENT2_REMPORT_NAME) {GET_U8(tmp,set_tlp_event_remport(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP1_VALID_NAME) {GET_U8(tmp,set_tlp_remdev_valid(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP2_VALID_NAME) {GET_U8(tmp,set_tlp_remdev_valid(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP1_TYPE_NAME) {GET_U8(tmp,set_tlp_remdev_type(0,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP2_TYPE_NAME) {GET_U8(tmp,set_tlp_remdev_type(1,(u8)var_u32),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(TLP1_DESCR_NAME){GET_STR(tmp,set_tlp_remdev_name(0,tmpstr),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP2_DESCR_NAME){GET_STR(tmp,set_tlp_remdev_name(1,tmpstr),settings_add2queue(SQ_TELEPORT));}


			IF_CMP(TLP1_IP_NAME) {GET_IP(tmp,set_tlp_remdev_ip(0,&ip),settings_add2queue(SQ_TELEPORT));}
			IF_CMP(TLP2_IP_NAME) {GET_IP(tmp,set_tlp_remdev_ip(1,&ip),settings_add2queue(SQ_TELEPORT));}

			IF_CMP(LLDP_STATE_NAME) {GET_U8(tmp,set_lldp_state((u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_TX_INT_NAME) {GET_U8(tmp,set_lldp_transmit_interval((u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_HOLD_TIME_NAME) {GET_U8(tmp,set_lldp_hold_multiplier((u8)var_u32),settings_add2queue(SQ_LLDP));}

			IF_CMP(LLDP_PORT1_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(0,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT2_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(1,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT3_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(2,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT4_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(3,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT5_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(4,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT6_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(5,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT7_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(6,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT8_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(7,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT9_STATE_NAME) {GET_U8(tmp,set_lldp_port_state(8,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT10_STATE_NAME){GET_U8(tmp,set_lldp_port_state(9,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT11_STATE_NAME){GET_U8(tmp,set_lldp_port_state(10,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT12_STATE_NAME){GET_U8(tmp,set_lldp_port_state(11,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT13_STATE_NAME){GET_U8(tmp,set_lldp_port_state(12,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT14_STATE_NAME){GET_U8(tmp,set_lldp_port_state(13,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT15_STATE_NAME){GET_U8(tmp,set_lldp_port_state(14,(u8)var_u32),settings_add2queue(SQ_LLDP));}
			IF_CMP(LLDP_PORT16_STATE_NAME){GET_U8(tmp,set_lldp_port_state(15,(u8)var_u32),settings_add2queue(SQ_LLDP));}

		}else
			//если не нашли
			curr_ptr+=VAR_MAX_LEN;

		memset(tmp,0,sizeof(tmp));
		memset(var_name,0,sizeof(var_name));
	}

	//if((valid_var<MIN_VAR_NUM)||(!is_from_search()))
	//	return curr_ptr;




	if(is_from_search() && valid_var){
		return 0;
	}
	else if(valid_var>MIN_VAR_NUM){
		return 0;
	}

	return 1;
}








/*******************************************************************************************************/
/*       save mcu log as txt file 															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_sfp_dump(struct httpd_state *s, char *ptr)){
static char temp[128];
	  str[0] = 0;
	  PSOCK_BEGIN(&s->sout);

	  i2c_probe(0xA0);

	  for(u8 i=0;i<128;i++){
		temp[i] = read_byte_reg(0xA0,i);
		printf("%d:%02X\r\n",i,temp[i]);
      }
	  PSOCK_SEND(&s->sout,temp,128);
	  PSOCK_END(&s->sout);
}

