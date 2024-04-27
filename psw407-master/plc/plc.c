#include <string.h>
#include <stdlib.h>
#include "plc_def.h"

#include "board.h"
#include <string.h>
#include "../i2c/soft/i2c_soft.h"
#include "../deffines.h"
#include "task.h"
#include "plc.h"
#include "em.h"
#include "../events/events_handler.h"
#include "settings.h"
#include "debug.h"

extern struct status_t status;

extern u8 plc_processing_flag;

u8 get_plc_relay(u8 num){
u8 temp;
	i2c_buf_read(PLC_ADDR,REALY1_GET_STATE+num,&temp,1);
	vTaskDelay(50*MSEC);
	i2c_buf_read(PLC_ADDR,REALY1_GET_STATE+num,&temp,1);
	return temp;
}

//сохранение состояния реле в eeprom
u8 set_plc_relay_ee(u8 num, u8 state){
u8 temp[33];
	memset(temp,0,33);
	temp[0] = state;
	i2c_reg_write(PLC_ADDR,REALY1_SET_STATE+num,temp);
	return 0;
}

//команды на управление реле
u8 set_plc_relay(u8 num,u8 state){
u8 temp[33];
	memset(temp,0,33);
	temp[0] = state;
	DEBUG_MSG(RS485_DEBUG,"set_plc_relay %d: %d\r\n",num,state);
	i2c_reg_write(PLC_ADDR,RELAY1+num,temp);
	return 0;
}


//чтение состояния реле из eeprom
u8 get_plc_relay_ee(u8 num){
u8 temp;
	i2c_buf_read(PLC_ADDR,REALY1_GET_EE_STATE+num,&temp,1);
	vTaskDelay(50*MSEC);
	i2c_buf_read(PLC_ADDR,REALY1_GET_EE_STATE+num,&temp,1);
	return temp;
}

//получить состояния всех входов
void get_plc_inputs(void){
u8 temp;
u8 temp1;
u8 temp2;
	DEBUG_MSG(RS485_DEBUG,"get_plc_inputs\r\n");
	if(i2c_buf_read(PLC_ADDR,INPUT_SUMMARY,&temp,1)!=0)
		return;
	vTaskDelay(100*MSEC);
	if(i2c_buf_read(PLC_ADDR,INPUT_SUMMARY,&temp1,1)!=0)//дубль
		return;
	if(temp != temp1){
		vTaskDelay(100*MSEC);
		if(i2c_buf_read(PLC_ADDR,INPUT_SUMMARY,&temp2,1)!=0)//дубль
			return;
		temp = temp2;
	}
	for(u8 i=0;i<PLC_INPUTS;i++){
		dev.plc_status.in_state[i] = (temp>>i) & 0x01;
	}
}



u8 plc_relay_reset(u8 num){
u8 temp;
	i2c_buf_read(PLC_ADDR,REALY1_RESET+num,&temp,1);
	return 0;
}

u8 get_plc_hw_vers(void){
	return dev.plc_status.hw_vers;
}

void set_plc_hw_vers(u8 vers){
	if(vers == PLC_01 || vers == PLC_02){
		dev.plc_status.hw_vers = vers;
	}
}

//число цифровых входов
u8 get_plc_input_num(void){
	switch(dev.plc_status.hw_vers){
		case PLC_01:
			return 0;
		case PLC_02:
			return 3;
		default:
			return 3;
	}
}

//число цифровых выходов
u8 get_plc_output_num(void){
	switch(dev.plc_status.hw_vers){
		case PLC_01:
			return 2;
		case PLC_02:
			return 1;
		default:
			return 1;
	}
}





//работа с платой PLC: опрос электросчетчика и события по входным датчикам
void plc_processing(void){
static u8 init = 0;
u8 state,action;

if(init == 0){
	dev.plc_status.new_event = 1;
	action = 0;
	for(u8 i=0;i<PLC_RELAY_OUT;i++){
		dev.plc_status.out_state[i] = 3;
	}
	init = 1;
}

	if(is_plc_connected()){
		if(plc_processing_flag){
			plc_485_connect();
			get_plc_em_indications();
			plc_processing_flag = 0;
		}

		if(dev.plc_status.new_event){
			dev.plc_status.ready4events = 0;
			vTaskDelay(500*MSEC);
			get_plc_inputs();
			for(u8 i=0;i<get_plc_input_num();i++){
				if((dev.plc_status.in_state[i] != dev.plc_status.in_state_last[i])&&
						((dev.plc_status.in_state[i] == get_plc_in_alarm_state(i))||
								(get_plc_in_alarm_state(i) == PLC_ANY_CHANGE))){
					state = dev.plc_status.in_state[i];
					send_events_u32(EVENTS_INPUT1+i,(u32)state);
				}
				dev.plc_status.in_state_last[i] = dev.plc_status.in_state[i];
			}
			dev.plc_status.new_event = 0;
			dev.plc_status.ready4events = 1;
		}

		//реализация логики
		for(u8 i=0;i<get_plc_output_num();i++){
			if(get_plc_out_state(i) == LOGIC){
				action = 0;//
				//for sensors
				for(u8 j=0;j<NUM_ALARMS;j++){
					if(get_alarm_state(j) && get_plc_out_event(i,j)){
						if(get_alarm_front(j)==(get_sensor_state(j)+1)){
							action = 1;
						}
					}
				}
				//for inputs
				for(u8 j=0;j<get_plc_input_num();j++){
					if(get_plc_in_state(j)){
						if(get_plc_out_event(i,j+NUM_ALARMS)){
							if(dev.plc_status.in_state[j] == get_plc_in_alarm_state(j)){
								action = 1;
							}
						}
					}
				}

				//событие произошло
				if(action){
					if(get_plc_out_action(i)==PLC_ACTION_SHORT || get_plc_out_action(i)==PLC_ACTION_OPEN){
						vTaskDelay(500*MSEC);
						if(dev.plc_status.out_state[i] != get_plc_out_action(i)){
							set_plc_relay(i,get_plc_out_action(i));
							dev.plc_status.out_state[i] = get_plc_out_action(i);
						}
					}
					else if(get_plc_out_action(i)==PLC_ACTION_IMPULSE){
						if(dev.plc_status.out_state[i]!= get_plc_out_action(i)){
							plc_relay_reset(i);
							dev.plc_status.out_state[i] = get_plc_out_action(i);
						}
					}

				}
				else{
					//если нет события
					if(get_plc_out_action(i)==PLC_ACTION_SHORT || get_plc_out_action(i)==PLC_ACTION_OPEN){
						vTaskDelay(500*MSEC);
						if(get_plc_out_action(i) == PLC_ACTION_SHORT){
							if(dev.plc_status.out_state[i] != PLC_ACTION_OPEN){
								set_plc_relay(i,PLC_ACTION_OPEN);
								dev.plc_status.out_state[i] = PLC_ACTION_OPEN;
							}
						}
						else{
							if(dev.plc_status.out_state[i] != PLC_ACTION_SHORT){
								set_plc_relay(i,PLC_ACTION_SHORT);
								dev.plc_status.out_state[i] = PLC_ACTION_SHORT;
							}
						}

					}
					else if(get_plc_out_action(i)==PLC_ACTION_IMPULSE){
						if(dev.plc_status.out_state[i] != PLC_ACTION_OPEN){
							set_plc_relay(i,PLC_ACTION_OPEN);
							dev.plc_status.out_state[i] = PLC_ACTION_OPEN;
						}
					}
				}
			}
			action = 0;
		}

	}

}
