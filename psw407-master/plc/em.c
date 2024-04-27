#include <string.h>
#include <stdlib.h>
#include "plc_def.h"
#include "board.h"
#include <string.h>
#include "../i2c/soft/i2c_soft.h"
#include "../deffines.h"
#include "task.h"
#include "em.h"
#include "settings.h"
#include "debug.h"





static u8 em_connected = 0;
plc_ind_t plc_ind;
extern struct status_t status;

u8 plc_processing_flag = 0;

//запуск подключения
void plc_em_start(void){
	plc_processing_flag = 1;
	em_connected = 0;
}

//освободить
void plc_release(void){
	dev.plc_status.ready4events = 1;
}

//занять
void plc_take(void){
	dev.plc_status.ready4events = 0;
}

//получение имени модели счетчика
void get_plc_em_model_name(char *text){
	get_plc_em_model_name_list(get_plc_em_model(),text);
}

//получение имени счетчика по индексу из списка доступных
void get_plc_em_model_name_list(u8 num, char *text){
	switch(num){
		case EM_CE102M:
			strcpy(text,"Energomera CE102M");
			break;
		default:
			strcpy(text,"---");
	}
}

//счетчик подключен
u8 is_em_connected(void){
	return em_connected;
}

//суммарные показания
void get_em_total(char *text){
	if(em_connected){
		strcpy(text,plc_ind.total);
	}
	else
		text[0] = 0;
}

void get_em_t1(char *text){
	if(em_connected){
		strcpy(text,plc_ind.t1);
	}
	else
		text[0] = 0;
}

void get_em_t2(char *text){
	if(em_connected){
		strcpy(text,plc_ind.t2);
	}
	else
		text[0] = 0;
}

void get_em_t3(char *text){
	if(em_connected){
		strcpy(text,plc_ind.t3);
	}
	else
		text[0] = 0;
}

void get_em_t4(char *text){
	if(em_connected){
		strcpy(text,plc_ind.t4);
	}
	else
		text[0] = 0;
}


void set_em_connected(u8 state){
	em_connected = state;
}


//send string
void plc_485_send(char *str,u8 len){
u8 temp[33];
	memset(temp,0,33);
	temp[0] = len;
	for(u8 i=0;i<len;i++){
		temp[i+1] = str[i];
	}
	DEBUG_MSG(RS485_DEBUG,"rs485 write:(%d)%s\r\n",len,str);
	i2c_reg_write(PLC_ADDR,WRITE_485,temp);
}

//recieve string
void plc_485_read(char *str,u8 len){
	i2c_buf_read(PLC_ADDR,READ_485,(unsigned char *)str,len);
}


//команды 485
//настройка 485
u8 set_plc_485_config(u8 rate,u8 parity, u8 databits, u8 stopbits){
u8 reg[33];
	memset(reg,0,33);
	reg[0] = rate;
	reg[1] = parity;
	reg[2] = databits;
	reg[3] = stopbits;
	i2c_reg_write(PLC_ADDR,CONFIG_485,reg);
	return 0;
}

static u8 plc_em_decode(char *str,u8 len){
char  *ptr1, *ptr2;
//char val_str[32];


	//find etope+total
	ptr1 = strstr(str,"ET0PE(");
	ptr1+=strlen("ET0PE(");
	if(ptr1 == NULL)
		return 1;
	ptr2 = strstr(ptr1,")");
	if(ptr2 == NULL)
		return 1;
	//val = strtof(ptr1,&ptr2,10);
	strncpy(plc_ind.total,ptr1,ptr2-ptr1);
	//printf("total %s\r\n",plc_ind.total);

	//t1
	ptr1 = strstr(ptr2,"(");
	ptr1++;
	ptr2 = strstr(ptr1,")");
	strncpy(plc_ind.t1,ptr1,ptr2-ptr1);
	//printf("t1 %s\r\n",plc_ind.t1);

	//t2
	ptr1 = strstr(ptr2,"(");
	ptr1++;
	ptr2 = strstr(ptr1,")");
	strncpy(plc_ind.t2,ptr1,ptr2-ptr1);
	//printf("t2 %s\r\n",plc_ind.t2);

	//t3
	ptr1 = strstr(ptr2,"(");
	ptr1++;
	ptr2 = strstr(ptr1,")");
	strncpy(plc_ind.t3,ptr1,ptr2-ptr1);
	//printf("t3 %s\r\n",plc_ind.t3);

	//t4
	ptr1 = strstr(ptr2,"(");
	ptr1++;
	ptr2 = strstr(ptr1,")");
	strncpy(plc_ind.t4,ptr1,ptr2-ptr1);
	//printf("t4 %s\r\n",plc_ind.t4);


	return 0;
}

//подключаемся по 485
//return:
//	1 - подключились
//  0 - не подключились
u8 plc_485_connect(void){
u8 rate,parity,databits,stopbits;
char str[64];
char pass[64];
char id[64];
	rate = get_plc_em_rate();
	parity = get_plc_em_parity();
	databits = get_plc_em_databits();
	stopbits = get_plc_em_stopbits();
	set_plc_485_config(rate,parity,databits,stopbits);
	vTaskDelay(300*MSEC);


	get_plc_em_id(id);
	if(strlen(id)){
		str[0] = '/';
		str[1] = '?';
		for(u8 i=0;i<strlen(id);i++){
			str[2+i] = id[i];
		}
		str[2+strlen(id)] = '!';
		str[3+strlen(id)] = 0x0D;
		str[4+strlen(id)] = 0x0A;
		plc_485_send(str,5+strlen(id));
	}
	else{
		plc_485_send("/?!\r\n",5);
	}
	vTaskDelay(1000*MSEC);

	plc_485_read(str, 20);
	if(strstr(str+5,"CE102M")!=NULL){
		str[0] = 0x06;
		str[1] = '0';
		str[2] = '5';
		str[3] = '1';
		str[4] = 0x0D;
		str[5] = 0x0A;
		plc_485_send(str,6);
		vTaskDelay(300*MSEC);
		plc_485_read(str, 20);
		if(strstr(str,"P0")==NULL){
			em_connected = 1;
			return 1;
		}

		get_plc_em_pass(pass);
		if(strlen(pass)){
			str[0] = 0x01;
			str[1] = 'P';
			str[2] = '1';
			str[3] = 0x02;
			str[4] = '(';
			for(u8 i=0;i<strlen(pass);i++){
				str[5+i] = pass[i];
			}
			str[5+strlen(pass)] = ')';
			str[6+strlen(pass)] = 0x03;
			str[7+strlen(pass)] = '!';
			plc_485_send(str,8+strlen(pass));
			vTaskDelay(400*MSEC);
			plc_485_read(str, 20);
			DEBUG_MSG(RS485_DEBUG,"rs485 read: %s\r\n",str);

			str[0] = 0x01;
			str[1] = 'B';
			str[2] = '0';
			str[3] = 0x03;
			str[4] = 'u';
			plc_485_send(str,5);
			vTaskDelay(400*MSEC);
		}
		em_connected = 1;
		return 1;
	}
	else{
		em_connected = 0;
		return 0;
	}
}

void get_plc_em_indications(void){
char str[256];
char pass[64];
char id[64];

	if(em_connected == 0)
		return;

	get_plc_em_pass(pass);
	if(strlen(pass)){
		str[0] = '/';
		str[1] = '?';
		get_plc_em_id(id);
		for(u8 i=0;i<strlen(id);i++){
			str[2 + i] = id[i];
		}
		str[2 + strlen(id)] = '!';
		str[3 + strlen(id)] = 0x0D;
		str[4 + strlen(id)] = 0x0A;
		plc_485_send(str,5+strlen(id));
		vTaskDelay(400*MSEC);

		plc_485_read(str, 20);

		str[0] = 0x06;
		str[1] = '0';
		str[2] = '5';
		str[3] = '1';
		str[4] = 0x0D;
		str[5] = 0x0A;
		plc_485_send(str,6);
		vTaskDelay(100*MSEC);
		get_plc_em_pass(pass);
		if(strlen(pass)){
			str[0] = 0x01;
			str[1] = 'P';
			str[2] = '1';
			str[3] = 0x02;
			str[4] = '(';
			for(u8 i=0;i<strlen(pass);i++){
				str[5+i] = pass[i];
			}
			str[5+strlen(pass)] = ')';
			str[6+strlen(pass)] = 0x03;
			str[7+strlen(pass)] = '!';
			plc_485_send(str,8+strlen(pass));
			vTaskDelay(300*MSEC);
			//plc_485_read(str, 20);
			//DEBUG_MSG(RS485_DEBUG,"rs485 read: %s\r\n",str);
		}
	}
	str[0] = 0x01;
	str[1] = 'R';
	str[2] = '1';
	str[3] = 0x02;
	str[4] = 'E';
	str[5] = 'T';
	str[6] = '0';
	str[7] = 'P';
	str[8] = 'E';
	str[9] = '(';
	str[10] = ')';
	str[11] = 0x03;
	str[12] = '7';
	plc_485_send(str,13);
	vTaskDelay(300*MSEC);
	plc_485_read(str,100);
	if(plc_em_decode(str,100)==1)
		em_connected = 0;//были ошибки при декодировании


	str[0] = 0x01;
	str[1] = 'B';
	str[2] = '0';
	str[3] = 0x03;
	str[4] = 'u';
	plc_485_send(str,5);
}


