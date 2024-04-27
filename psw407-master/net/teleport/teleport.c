#include "deffines.h"

#if IP_STACK == UIP

#include <stdio.h>
#include <string.h>


#include "../uip/uip.h"
#include "../uip/psock.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "settings.h"
#include "teleport.h"
#include "board.h"
#include "settings.h"
#include "crc.h"
#include "selftest.h"
#include "debug.h"
#include "net/events/events_handler.h"
#include "net/uip/uip_arp.h"



uip_ipaddr_t ip;
struct uip_conn *conn;
struct teleport_state tlp_st[MAX_REMOTE_TLP];
struct timer teleport_timer;

xQueueHandle TeleportQueue[MAX_REMOTE_TLP];//очереди для отправки сообщений


struct pkt_output pkt[MAX_REMOTE_TLP];

//struct uip_udp_conn *udp_conn_in[TLP_UDP_PORT_NUM];
//struct uip_udp_conn *udp_conn_in;
struct uip_udp_conn *udp_conn_out;

//init udp connection
void teleport_init(void){
uip_ipaddr_t ip;

	DEBUG_MSG(TELEPORT_DEBUG,"teleport_init\r\n");

	timer_set(&teleport_timer,  10000 * MSEC  );

	if(udp_conn_out){
		uip_udp_remove(udp_conn_out);
	}


	if(get_remdev_num()){
		get_tlp_remdev_ip(0,&ip);
		udp_conn_out = uip_udp_new(&ip,HTONS(TLP_UDP_PORT_OUT));
		if(udp_conn_out != NULL){
			uip_udp_bind(udp_conn_out,HTONS(TLP_UDP_PORT_IN));
		}

		for(u8 i=0;i<MAX_REMOTE_TLP;i++){
			tlp_st[i].connected = 0;
			dev.remdev[i].conn_state = 0;
			dev.remdev[i].conn_state_last = 0;
			dev.remdev[i].rx_count = 0;
			dev.remdev[i].tx_count = 0;
			timer_set(&dev.remdev[i].arp_timer,5000*MSEC);
			if(TeleportQueue[i] == NULL){
				TeleportQueue[i] = xQueueCreate(TLP_QUEUE_LEN,sizeof(struct pkt_output));
				if(TeleportQueue[i] == NULL){
					ADD_ALARM(ERROR_CREATE_TLP_QUEUE);
					DEBUG_MSG(TELEPORT_DEBUG,"ERROR_CREATE_QUEUE\r\n");
				}
			}
		}

	}
}

int send_teleport_input_msg(u8 input, u8 state){
struct pkt_output pkt;
u32 crc;
u8 num;

	if(input<MAX_INPUT_NUM){
		if(get_input_rem_dev(input)>=MAX_REMOTE_TLP)
			return -1;
		if(get_input_rem_port(input)>=MAX_OUTPUT_NUM)
			return -1;
	}else if(input < TLP_EVENTS_PREFIX || (input-TLP_EVENTS_PREFIX)>MAX_TLP_EVENTS_NUM){
		return -1;
	}



	DEBUG_MSG(TELEPORT_DEBUG,"send: input %d state %d \r\n",input,state);

	pkt.crc = 0;
	pkt.timestamp = RTC_GetCounter();
	pkt.input = input;
	pkt.type = 0;//inputs
	if(input<MAX_INPUT_NUM){
		pkt.out = get_input_rem_port(input);
	}
	if(input >= TLP_EVENTS_PREFIX && input < (TLP_EVENTS_PREFIX+MAX_TLP_EVENTS_NUM)){
		pkt.out = get_tlp_event_rem_port(input-TLP_EVENTS_PREFIX);
	}
	pkt.out_state = state;
	//calculate crc
	crc = BuffCrc((u8 *)&pkt,sizeof(struct pkt_output));
	pkt.crc = crc;
	if(input<MAX_INPUT_NUM){
		num = get_input_rem_dev(input);
		if(TeleportQueue[num] != NULL){
			if(xQueueSend(TeleportQueue[num],&pkt,0)!= pdPASS){
				DEBUG_MSG(TELEPORT_DEBUG,"add to queue FAIL %d\r\n",num);
			}
		}
		else
			DEBUG_MSG(TELEPORT_DEBUG,"queue is NULL %d\r\n",num);
	}
	else{
		num = get_tlp_event_rem_dev(input - TLP_EVENTS_PREFIX);
		if(TeleportQueue[num] != NULL){
			if(xQueueSend(TeleportQueue[num],&pkt,0)!= pdPASS){
				DEBUG_MSG(TELEPORT_DEBUG,"add to queue FAIL %d\r\n",num);
			}
		}
		else
			DEBUG_MSG(TELEPORT_DEBUG,"queue is NULL %d\r\n", num);
	}
	return 0;
}


//udp msg output parts
//* IN: num - dev num in list**/
void teleport_out_udp_appcall(void){
struct pkt_output *tlp = (struct pkt_output *)uip_appdata;


	for(u8 num=0;num<MAX_REMOTE_TLP;num++){
		if(get_tlp_remdev_valid(num)==1){
			if(uxQueueMessagesWaiting(TeleportQueue[num])){
				//и запись в arp есть
				if(uip_arp_out_check(udp_conn_out)==0){
					if(xQueueReceive(TeleportQueue[num],tlp,0)==pdPASS){
						DEBUG_MSG(TELEPORT_DEBUG,"TELEPORT SEND dev=%d\r\n",num);
						uip_len = sizeof(struct pkt_output);

						uip_udp_send(uip_len);

						dev.remdev[num].tx_count++;
						dev.remdev[num].conn_state = 1;//устройство подключено
						if(dev.remdev[num].conn_state != dev.remdev[num].conn_state_last){
							send_events_u32(EVENTS_TLP1_CONN + num,(u32)(dev.remdev[num].conn_state));
						}
						dev.remdev[num].conn_state_last = dev.remdev[num].conn_state;
					}
				}else{
					//send arp
					if(timer_expired(&dev.remdev[num].arp_timer)){
						uip_udp_send(uip_len);

						DEBUG_MSG(TELEPORT_DEBUG,"tlp not send, arp msg: dev=%d len=%d\r\n",num, uip_len);
						timer_reset(&dev.remdev[num].arp_timer);
						dev.remdev[num].conn_state = 0;//устройство не подключено

						if(dev.remdev[num].conn_state != dev.remdev[num].conn_state_last){
							send_events_u32(EVENTS_TLP1_CONN + num,(u32)(dev.remdev[num].conn_state));
						}
						dev.remdev[num].conn_state_last = dev.remdev[num].conn_state;
					}
				}
			}
		}
	}
}

//при входящих udp
void teleport_in_udp_appcall(void){
	if (uip_newdata()) {
		//newdata_teleport();
		return;
	}
}




void teleport_processing(void){
static u8 last_state[3]={0,0,0};
static u8 last_state_event[3]={0,0,0};
static u8 init = 0;
u8 state;
u8 port_err;


	if(init == 0){
		last_state[0] = get_sensor_state(0);
		last_state[1] = get_sensor_state(1);
		last_state[2] = get_sensor_state(2);
		init++;
	}


	//при наступлении события, сразу же отправляем сообщение
	for(u8 i=0;i<MAX_INPUT_NUM;i++){
		if(get_input_state(i)){
			if(last_state[i] != get_sensor_state(i)){
				vTaskDelay(50*MSEC);
				if(last_state[i] != get_sensor_state(i)){
					state = get_sensor_state(i);
					if(get_input_inverse(i) == 0){
						if(state){
							state = 0;
						}
						else
							state = 1;
					}
					send_teleport_input_msg(i,state);
				}
			}
			last_state[i] = get_sensor_state(i);
		}
	}
	for(u8 i=0;i<MAX_TLP_EVENTS_NUM;i++){
		if(get_tlp_event_state(i)){
			switch(i){
				case 0:
					if(last_state_event[i] != is_ups_rezerv())
						send_teleport_input_msg(i+TLP_EVENTS_PREFIX,is_ups_rezerv());
					last_state_event[i] = is_ups_rezerv();
					break;
				case 1:
					port_err = 0;
					for(u8 k=0;k<COOPER_PORT_NUM;k++){
						if(get_port_error(k)){
							port_err = 1;
						}
					}
					if(last_state_event[i] != port_err)
						send_teleport_input_msg(i+TLP_EVENTS_PREFIX,port_err);
					last_state_event[i] = port_err;
					break;
			}
		}
	}

	//периодическая посылка состояний
	if(timer_expired(&teleport_timer)){
		for(u8 i=0;i<MAX_INPUT_NUM;i++){
			if(get_input_state(i)){
				state = get_sensor_state(i);
				if(get_input_inverse(i) == 0){
					if(state){
						state = 0;
					}
					else
						state = 1;
				}
				send_teleport_input_msg(i,state);
			}
		}
		for(u8 i=0;i<MAX_TLP_EVENTS_NUM;i++){
			if(get_tlp_event_state(i)){
				switch(i){
					case 0:
						state = is_ups_rezerv();
						if(get_tlp_event_inverse(i)){
							if(state)
								state = 0;
							else
								state = 1;
						}
						send_teleport_input_msg(i+TLP_EVENTS_PREFIX,state);
						break;
					case 1:
						port_err = 0;
						for(u8 k=0;k<COOPER_PORT_NUM;k++){
							if(get_port_error(k)){
								port_err = 1;
							}
						}
						if(get_tlp_event_inverse(i)){
							if(port_err)
								port_err = 0;
							else
								port_err = 1;
						}
						send_teleport_input_msg(i+TLP_EVENTS_PREFIX,port_err);
						break;
				}
			}
		}
		timer_set(&teleport_timer,  10000 * MSEC  );
	}

}

#endif

