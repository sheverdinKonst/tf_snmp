#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "stm32f4xx_rtc.h"
#include "../deffines.h"
#include "../uip/uipopt.h"
#include "../uip/uip.h"
#include "../uip/uip_arp.h"
#include "../sntp/sntp.h"
#include "snmp.h"
#include "board.h"
#include "settings.h"
#include "../events/events_handler.h"
#include "queue.h"
#include "../uip/uip_arp.h"
#include "selftest.h"
#include "snmpd/ber.h"
#include "debug.h"


//struct uip_udp_conn *snmp_conn; //snmp connection for control
struct uip_udp_conn *snmp_trap_conn; //snmp connection for traps

xQueueHandle SnmpTrapQueue;//очередь для snmp сообщений

struct timer snmp_arp_timer;

static u16 request_id=0;

//extern uint8_t MyIP[4];


/*---------------------------------------------------------------------------*/
static uint8_t *add_stage1(uint8_t *optptr)
{
  *optptr++ = 0x30;
  *optptr++ = 0x82;
  *optptr++ = 0x00;
   return optptr;
}
static uint8_t *add_summ_len(uint8_t *optptr, uint8_t len){
	*optptr++ = len;
	return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_version(uint8_t *optptr, uint8_t version)
{
  *optptr++ = 1;
  *optptr++ = version;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_type(uint8_t *optptr)
{
  // 0x04 - trap only
  *optptr++ = 0x04;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_community(uint8_t *optptr, char *community, u8 len)
{
  *optptr++ = len;
  for(u8 i=0;i<len;i++){
	  *optptr++ = community[i];
  }
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_stage3_v1(uint8_t *optptr)
{
  *optptr++ = 0xA4;
  *optptr++ = 0x82;
  *optptr++ = 0x00;
  return optptr;
}

/*---------------------------------------------------------------------------*/
static uint8_t *add_stage3_v2(uint8_t *optptr)
{
  *optptr++ = 0xA7;
  *optptr++ = 0x82;
  *optptr++ = 0x00;
  return optptr;
}

/*---------------------------------------------------------------------------*/
static uint8_t *add_stage3_len(uint8_t *optptr, u8 s3_len)
{
  *optptr++ = s3_len;
  return optptr;
}

/*---------------------------------------------------------------------------*/
static uint8_t *add_enterprise(uint8_t *optptr)
{
  *optptr++ = 10;
  *optptr++ = 0x2b;
  *optptr++ = 0x06;
  *optptr++ = 0x01;
  *optptr++ = 0x04;
  *optptr++ = 0x01;
  *optptr++ = 0x82;//
  *optptr++ = 0xc8;//OID == 42019
  *optptr++ = 0x23;//
  *optptr++ = 3;
  *optptr++ = 2;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_stage4(uint8_t *optptr)
{
  *optptr++ = 0x40;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_agent_ip(uint8_t *optptr, uip_ipaddr_t ip)
{
  *optptr++ = 4;

  *optptr++ = uip_ipaddr1(ip);
  *optptr++ = uip_ipaddr2(ip);
  *optptr++ = uip_ipaddr3(ip);
  *optptr++ = uip_ipaddr4(ip);

  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_02(uint8_t *optptr)
{
  *optptr++ = 0x02;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_06(uint8_t *optptr)
{
  *optptr++ = 0x06;
  return optptr;
}

/*---------------------------------------------------------------------------*/
static uint8_t *add_generic_trap(uint8_t *optptr, u8 generic_trap, u8 len)
{
  *optptr++ = len;
  *optptr++ = generic_trap;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_stage5(uint8_t *optptr)
{
  *optptr++ = 0x43;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_specific_trap(uint8_t *optptr, u8 *specific_trap, u8 len)
{
  *optptr++ = len;
  for(u8 i=0;i<len;i++){
	  *optptr++ = specific_trap[i];
  }
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_time_stamp(uint8_t *optptr, u8 *ts_len)
{
  static u8 len;
  u32 time_stamp=RTC_GetCounter();
  len=0;
  if(time_stamp>0xFF)
	  len=2;
  else
	  len=1;

  *optptr++ = len;

  for(u8 i=len;i>0;i--){
	  *optptr++ = (u8)(time_stamp>>(8*(i-1)));
  }
  *ts_len=len;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_stage6(uint8_t *optptr)
{
  *optptr++ = 0x30;
  *optptr++ = 0x82;
  *optptr++ = 0x00;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_variable_bindings(uint8_t *optptr, u8 *var, u8 len)
{
  if(len){
	  *optptr++ = len;
	  for(u8 i=0;i<len;i++){
		  *optptr++ = var[i];
	  }
  }
  return optptr;
}

/*---------------------------------------------------------------------------*/
static u8 get_strap_len(struct snmp_msg2_t *snmp_msg){
	u8 len=0;
		for(u8 i=0;i<16;i++){
			if(snmp_msg->spectrap[i]==0){
				len = i;
				break;
			}
		}
		if(len)
			return len;
		else
			return 1;
}
/*---------------------------------------------------------------------------*/
static u8 make_varbind_list(struct snmp_msg2_t *snmp_msg,u8 *arr){
u8 i,ptr=0,len1=0;//obj len
u8 len2=0;//val len

	for(i=0;i<SNMP_BIND_MAXNUM;i++){
		if(snmp_msg->varbind[i].flag == 1){
			len1=0;
			len2=0;
			arr[ptr] = 0x30;
			//arr[ptr+1] = 0x0c;//0x0f;
			arr[ptr+2] = 0x06;

			len1=snmp_msg->varbind[i].len;
			if(len1>16)
				len1 = 16;

			for(u8 j=0;j<len1;j++){
				arr[ptr+4+j] = snmp_msg->varbind[i].objname[j];
			}

			if(len1==0){
				len1=1;
				arr[ptr+4]=0;
			}
			arr[ptr+3]=len1;
			if(snmp_msg->varbind[i].type == BER_TYPE_OCTET_STRING){
				arr[ptr+len1+4]=0x04;
				arr[ptr+len1+5]=snmp_msg->varbind[i].value_str_len;
				for(u8 j=0;j<snmp_msg->varbind[i].value_str_len;j++){
					arr[ptr+len1+6+j] = snmp_msg->varbind[i].value_str[j];
				}
				len2 = snmp_msg->varbind[i].value_str_len;
				arr[ptr+1] = 4+len1+len2;//0x0f;
				ptr = ptr + (len1+len2+6);
			}
			else{
			//type integer
				arr[ptr+len1+4]=0x02;
				  if(snmp_msg->varbind[i].value > 0xFFFFFF){
					  len2=4;
					  arr[ptr+len1+5]=len2;
					  arr[ptr+len1+6]=(u8)(snmp_msg->varbind[i].value >> 24);
					  arr[ptr+len1+7]=(u8)(snmp_msg->varbind[i].value >> 16);
					  arr[ptr+len1+8]=(u8)(snmp_msg->varbind[i].value >> 8);
					  arr[ptr+len1+9]=(u8)(snmp_msg->varbind[i].value );
				  }
				  else if(snmp_msg->varbind[i].value > 0xFFFF){
					  len2=3;
					  arr[ptr+len1+5]=len2;
					  arr[ptr+len1+6]=(u8)(snmp_msg->varbind[i].value >> 16);
					  arr[ptr+len1+7]=(u8)(snmp_msg->varbind[i].value >> 8);
					  arr[ptr+len1+8]=(u8)(snmp_msg->varbind[i].value );
				  }
				  else if(snmp_msg->varbind[i].value > 0xFF){
					  len2=2;
					  arr[ptr+len1+5]=len2;
					  arr[ptr+len1+6]=(u8)(snmp_msg->varbind[i].value >> 8);
					  arr[ptr+len1+7]=(u8)(snmp_msg->varbind[i].value );
				  }
				  else{
					  len2=1;
					  arr[ptr+len1+5]=len2;
					  arr[ptr+len1+6]=(u8)(snmp_msg->varbind[i].value );
				  }
				  arr[ptr+1] = 4+len1+len2;//0x0f;
				  ptr = ptr + (len1+len2+6);
			}
		}
		else{
			return ptr;
		}
	}
	return ptr;
}


/*add request id*/
static uint8_t *add_request_id(uint8_t *optptr, u16 id)
{
  *optptr++ = 0x02;
  *optptr++ = (u8)(id>>8);
  *optptr++ = (u8)id;
  return optptr;
}


/*add error staus*/
static uint8_t *add_error_status(uint8_t *optptr, u8 status)
{
  *optptr++ = 0x02;
  *optptr++ = 0x01;
  *optptr++ = status;
  return optptr;
}

/*add error id*/
static uint8_t *add_error_index(uint8_t *optptr, u8 index)
{
  *optptr++ = 0x02;
  *optptr++ = 0x01;
  *optptr++ = index;
  return optptr;
}
/*
static u8 get_version(u8 *buff){
	for(u8 i=4;i<uip_datalen();i++){
		if((buff[i-4]==0x30)&&(buff[i-2]==0x02)&&(buff[i-1]==0x01)){
			if((buff[i] == SNMP__VERSION_1)||(buff[i] == SNMP__VERSION_2C)||(buff[i] == SNMP__VERSION_3)){
				return buff[i];
			}
		}
	}
	return -1;
}
*/

//static u16 get_msg_len(u8 *buff){
//	return buff[1];
//}

//return ptr to end of communitie string
static u8 check_communitie(char *buff,char *commun,u8 len){
u8 ptr=0;
	for(u8 i=0;i<len;i++){
		if(buff[i] == commun[ptr]){
			ptr++;
			if(ptr == strlen(commun))
				return i;//нашли
		}
		else
			ptr = 0;
	}
	return 0;
}

static u8 get_pdu_type(u8 *buff,u8 *ptr,u8 len){
u8 ret_val;
	if(len>(*ptr+2)){
		ret_val = buff[*ptr+1];
		*ptr+=2;
		return ret_val;
	}
	else
		return -1;
}

static u16 get_request_id(u8 *buff,u8 *ptr,u8 len){
	if(len>(*ptr+4)){
		ptr++;
		if(buff[*ptr] == 0x02){
			ptr++;
			if(buff[*ptr]==0x01){
				ptr++;
				return buff[*ptr];
			}else if(buff[*ptr]==0x01){
				ptr+=2;
				return 	(buff[*ptr-1]<<8 | buff[*ptr]);
			}
		}
	}
	return -1;
}

static u16 get_error_status(u8 *buff,u8 *ptr,u8 len){
	if(len>(*ptr+4)){
		ptr++;
		if(buff[*ptr] == 0x02){
			ptr++;
			if(buff[*ptr]==0x01){
				ptr++;
				return buff[*ptr];
			}else if(buff[*ptr]==0x01){
				ptr+=2;
				return 	(buff[*ptr-1]<<8 | buff[*ptr]);
			}
		}
	}
	return -1;
}

/*---------------------------------------------------------------------------*/


u8 snmp_trap_init(void){
	uip_ipaddr_t addr;
	get_snmp_serv(&addr);

	if(get_snmp_state() == ENABLE && uip_ipaddr1(&addr) != 0){

		  if(SnmpTrapQueue == NULL){
			 //создаём очередь для отправки сообщений посредством трапов
			 //глубина очереди MSG_QUEUE_LEN
			 SnmpTrapQueue = xQueueCreate(MSG_QUEUE_LEN,sizeof(snmp_msg));
			 if(SnmpTrapQueue == NULL){
				 ADD_ALARM(ERROR_CREATE_SNMP_QUEUE);
				return 1;
			 }
		  }

		  if (snmp_trap_conn){
		    uip_udp_remove (snmp_trap_conn);
		    DEBUG_MSG(SNMP_DEBUG,"remove udp conn snmp traps\r\n");
		  }

		  get_snmp_serv(&addr);


		  if ((snmp_trap_conn = uip_udp_new (&addr, HTONS (SNMP_TRAP_PORT))))
		  {
			  uip_udp_bind (snmp_trap_conn, HTONS (SNMP_PORT));
		  }

		  timer_set(&snmp_arp_timer,5000*MSEC);
		  return 0;
	}
	return 1;
}

void snmp_appcall(void){
uip_ipaddr_t addr;
	get_snmp_serv(&addr);

	if(uip_ipaddr1(&addr)==0)
		return;

	if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
		return;
	//ecли в очереди что-то есть
	if(uxQueueMessagesWaiting(SnmpTrapQueue)){
		//и запись в arp есть
		if(uip_arp_out_check(/*snmp_conn*/snmp_trap_conn) == 0){
			if(xQueueReceive(SnmpTrapQueue,&snmp_msg,0) == pdPASS ){
				DEBUG_MSG(SNMP_DEBUG,"snmp send trap\r\n");
				snmp_send_trap(&snmp_msg);
			}
		}else{
			//send arp
			if(timer_expired(&snmp_arp_timer)){
				uip_udp_send(uip_len);
				DEBUG_MSG(SNMP_DEBUG,"snmp not send, arp msg\r\n");
				timer_reset(&snmp_arp_timer);
			}
		}
	}
}


void snmp_msg_appcall(u8 *buff,u16  len){
	char comm[17];
	u8 ptr;
	u8 type;


	//get_snmp_communitie(comm);
	get_snmp1_read_communitie(comm);

	ptr = check_communitie((char *)buff,comm,len);
	if(ptr){
		//вхождение в сообщество
		type = get_pdu_type(buff,&ptr,len);
		switch(type & 0x0F){
			case SNMP_GET_REQUEST:{
				DEBUG_MSG(SNMP_DEBUG,"snmp SNMP_GET_REQUEST\r\n");
				break;
			}
			case SNMP_GET_NEXT_REQUEST:{
				DEBUG_MSG(SNMP_DEBUG,"snmp SNMP_GET_NEXT_REQUEST\r\n");
				DEBUG_MSG(SNMP_DEBUG,"snmp request id %d\r\n",get_request_id(buff,&ptr,len));
				DEBUG_MSG(SNMP_DEBUG,"snmp error status %d\r\n",get_error_status(buff,&ptr,len));
				break;
			}
		}
	}
}

i8 snmp_send_trap(struct snmp_msg2_t *snmp_msg)
{
  u8 varbind_arr[64];
  u8 total_len,s3_len;
  DEBUG_MSG(SNMP_DEBUG,"start snmp_send_trap\r\n");

  u8 *tot_len_ptr, *end, *s3_ptr;
  u8 *buff = (u8 *)uip_appdata;

  u8 ts_len,varbind_len,strap_len;


  //default value
  //version
  if(get_snmp_vers() == 2)
	  snmp_msg->version=SNMP__VERSION_2C;
  else
	  snmp_msg->version=SNMP__VERSION_1;

  //communitie
  //get_snmp_communitie(snmp_msg->community);
  get_snmp1_read_communitie(snmp_msg->community);


/*make strings*/

  buff = add_stage1(buff);
  tot_len_ptr=buff;
  total_len=0;
  buff = add_summ_len(buff,total_len);
  buff = add_02(buff);
  buff = add_version(buff, snmp_msg->version);
  buff = add_type(buff);
  buff = add_community(buff,snmp_msg->community,strlen(snmp_msg->community));

  if(snmp_msg->version == SNMP__VERSION_2C)
	  buff = add_stage3_v2(buff);
  else
	  //vers v1
	  buff = add_stage3_v1(buff);

  s3_ptr = buff;//запоминаем ссылку
  buff = add_stage3_len(buff,0);


  if(snmp_msg->version == SNMP__VERSION_2C)
	  buff = add_02(buff);
  else
	  buff = add_06(buff);

  if(snmp_msg->version == SNMP__VERSION_2C){
	  buff = add_request_id(buff,request_id);
	  buff = add_error_status(buff,NOERROR);
	  buff = add_error_index(buff,0);
	  request_id++;
  }
  else {
	  buff = add_enterprise(buff);
	  buff = add_stage4(buff);
	  buff = add_agent_ip(buff,uip_hostaddr);
	  buff = add_02(buff);
	  buff = add_generic_trap(buff,snmp_msg->gentrap,1);
	  buff = add_02(buff);
	  strap_len = get_strap_len(snmp_msg);
	  buff = add_specific_trap(buff,snmp_msg->spectrap,strap_len);
	  buff = add_stage5(buff);
	  buff = add_time_stamp(buff,&ts_len);
  }

  varbind_len = make_varbind_list(snmp_msg,varbind_arr);

  if(varbind_len)
	  buff = add_stage6(buff);

  buff = add_variable_bindings(buff,varbind_arr,varbind_len);
  end = buff;
  total_len=(int)(end-tot_len_ptr - 1);
  add_summ_len(tot_len_ptr,total_len);
  s3_len=(int)(end - s3_ptr-1);
  add_stage3_len(s3_ptr,s3_len);

  uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));

  DEBUG_MSG(SNMP_DEBUG,"stop snmp_send_trap\r\n");
  return 0;
}
