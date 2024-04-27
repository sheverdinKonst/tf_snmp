#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include "stm32f10x_lib.h"
#include "stm32f4xx.h"
#include "board.h"
#include "i2c_soft.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "poe_ltc.h"
#include "../events/events_handler.h"
#include "UPS_command.h"
#include "../plc/plc_def.h"
#include "../plc/plc.h"
#include "settings.h"
#include "debug.h"




static uint8_t UPS_connected = 0;//плата IRP подключена
static uint8_t PLC_connected = 0;//плата PLC подключена

uint8_t UPS_rezerv;//переход на резерв 1- притание пропало
uint8_t UPS_AKB_detect;
uint8_t UPS_Low_Voltage;
uint8_t LinkState[5];
GPIO_InitTypeDef GPIO_InitStructure;
float voltage=0;
uint8_t RezervLastState=3;
uint8_t  LASTUPS_Low_Voltage=3;


#define MAX_NUM_10W 432
const u8 table10[MAX_NUM_10W]={
			170,160,157,155,154,153,153,153,154,153,154,153,
			154,154,154,154,154,153,153,153,153,154,153,153,153,153,153,153,
			153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
			153,153,152,152,152,152,152,152,152,152,151,151,151,151,151,151,
			151,151,151,150,150,150,150,150,150,150,150,150,150,149,149,149,
			149,149,149,149,148,148,148,148,148,148,148,148,147,147,147,147,
			147,147,147,147,147,147,147,147,146,146,146,146,146,145,146,145,
			145,145,145,144,144,144,144,144,144,144,144,144,144,144,144,142,
			142,142,142,142,142,142,142,142,142,141,141,141,140,140,140,140,
			140,140,140,139,140,139,139,139,139,139,138,139,138,138,138,138,
			138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
			138,138,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
			136,136,135,135,135,135,135,134,134,134,134,134,134,134,134,134,
			134,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,
			132,132,132,132,132,131,132,132,132,131,131,131,131,131,131,131,
			130,130,130,130,130,129,130,129,130,129,129,129,128,129,128,128,
			128,128,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
			127,127,127,126,126,126,126,126,125,126,126,125,125,125,125,124,
			124,124,124,124,124,124,124,124,124,124,124,124,124,123,123,123,
			123,122,122,122,122,122,121,121,121,121,121,120,121,120,120,120,
			120,120,120,120,119,119,119,119,119,119,119,119,119,119,118,118,
			118,118,118,117,117,117,117,117,116,116,116,116,116,115,115,115,
			115,115,115,115,115,115,115,114,114,114,114,114,113,113,113,113,
			113,112,112,112,112,112,112,112,112,111,111,111,110,110,110,110,
			109,109,109,109,109,108,108,107,108,107,106,106,106,106,106,106,
			105,105,104,104,104,103,103,103,102,101,101,100,100,99,98,98,
			96,96,95,94,93,93,92,91,90,90,89,88,87,87,85,82,
			78,75,70,67};

#define MAX_NUM_15W 339
const u8 table15[MAX_NUM_15W]=
		{181,175,170,168,167,166,166,165,165,165,165,165,165,165,164,
		163,163,163,163,162,162,162,162,162,162,161,161,161,161,160,
		160,160,160,160,159,159,159,159,158,158,158,157,157,157,157,
		156,156,156,156,156,156,156,156,155,155,155,155,154,154,154,
		154,153,154,153,153,153,153,153,153,152,153,152,152,151,151,
		151,151,150,150,150,150,149,150,149,149,149,149,148,148,148,
		148,148,147,147,147,147,147,147,147,147,147,146,146,146,146,
		145,145,145,144,144,144,144,144,144,144,144,144,144,144,142,
		142,142,142,142,142,142,142,141,141,141,141,140,141,141,140,
		140,140,140,140,139,139,138,139,139,138,138,138,138,138,138,
		136,136,136,136,136,136,136,136,136,136,136,136,136,135,135,
		134,134,134,134,134,134,134,133,133,133,133,133,133,133,133,
		132,133,132,132,132,132,131,131,131,130,130,130,130,129,129,
		129,128,129,128,128,128,127,128,127,127,127,127,127,127,127,
		126,126,126,126,126,125,125,125,125,124,124,124,124,124,124,
		123,123,123,123,123,122,122,122,122,122,121,121,121,120,120,
		120,119,119,119,119,119,119,119,118,118,117,118,117,117,116,
		116,115,115,115,115,115,115,115,114,114,114,114,113,113,112,
		112,112,112,112,112,112,111,111,110,110,110,110,109,108,108,
		108,107,107,107,106,106,106,106,106,105,105,104,104,103,103,
		103,103,102,102,101,101,100,100,100,100,99,98,97,97,96,
		96,95,95,94,93,93,92,92,90,90,89,87,87,86,85,
		84,83,81,80,78,75,74,72,69};


#define MAX_NUM_20W 249
const u8 table20[MAX_NUM_20W]={
		173,158,156,156,156,156,155,156,155,156,156,156,155,155,155,155,
		155,154,154,154,154,154,154,154,153,153,153,153,153,153,153,153,
		153,153,153,152,153,152,152,152,151,151,151,151,151,150,151,150,
		150,150,150,149,150,149,149,148,148,149,148,148,147,147,147,147,
		147,147,147,147,147,147,147,146,146,146,146,146,145,145,145,144,
		145,144,144,144,144,144,144,144,144,144,144,142,142,142,142,142,
		142,142,141,141,141,141,140,140,140,140,140,139,139,139,139,138,
		138,138,138,138,138,138,136,136,136,136,136,136,136,136,136,136,
		135,135,135,134,134,133,134,133,133,133,133,133,133,132,133,132,
		132,131,131,131,131,130,130,130,129,129,129,129,128,128,128,127,
		127,127,127,127,127,127,127,126,126,126,126,125,125,125,124,124,
		124,124,124,124,123,123,122,122,122,122,121,121,120,120,120,119,
		119,119,119,119,118,118,118,117,117,116,116,115,115,115,115,115,
		114,114,113,113,112,112,112,111,111,110,110,109,109,108,107,106,
		106,106,106,105,104,104,103,103,102,101,100,100,99,98,97,96,
		94,93,91,88,86,83,79,73,66};

#define MAX_NUM_25W 193
const u8 table25[MAX_NUM_25W]={
		171,155,154,154,154,154,154,154,154,154,154,153,153,153,153,153,
		153,153,153,153,152,152,151,151,151,151,151,150,150,150,150,149,
		149,149,148,149,148,148,148,147,147,147,147,147,147,147,147,146,
		146,146,146,145,145,145,145,144,144,144,144,144,144,142,142,142,
		142,142,142,142,142,141,141,141,141,140,140,140,139,139,139,139,
		138,138,138,138,138,138,136,136,136,136,136,135,135,135,135,134,
		134,133,133,133,133,133,133,133,132,132,131,131,130,130,130,130,
		129,129,128,128,128,127,127,127,127,126,126,126,125,125,125,124,
		124,124,124,124,123,122,122,122,121,120,120,119,119,119,119,118,
		118,117,117,117,116,115,115,115,115,114,113,113,112,112,112,111,
		111,110,110,108,108,107,107,106,106,105,104,103,103,102,101,100,
		100,99,98,96,96,95,93,93,91,89,87,86,84,81,78,75,
		70};

#define MAX_NUM_30W 155
const u8 table30[MAX_NUM_30W]={
		177,165,164,162,162,162,161,161,160,159,159,158,157,157,157,157,
		156,156,155,155,155,154,153,153,153,153,153,152,151,151,151,150,
		149,149,149,148,147,147,147,147,146,146,145,144,144,144,144,142,
		142,141,141,140,139,139,138,138,138,138,136,136,136,136,136,135,
		135,134,134,133,133,133,133,133,132,132,131,131,131,130,130,130,
		129,129,129,128,128,127,127,127,127,127,126,126,126,125,125,124,
		124,124,124,123,123,122,122,121,121,121,120,119,119,119,119,118,
		118,117,116,116,115,115,115,115,114,114,113,112,112,112,111,110,
		109,108,108,107,106,106,105,104,103,103,102,101,100,99,98,97,
		96,95,93,92,90,88,86,84,81,76,72};


#define MAX_NUM_35W 136
const u8 table35[MAX_NUM_35W]=
		{173,153,153,153,153,153,153,153,152,152,152,152,151,151,151,
		151,150,150,149,149,149,149,148,148,148,148,147,147,147,147,
		147,147,146,145,145,145,144,144,144,144,144,144,142,142,142,
		142,141,141,140,140,139,139,139,138,138,138,138,138,136,136,
		136,136,136,135,135,134,133,133,133,133,132,132,132,131,131,
		130,129,129,129,128,127,127,127,127,126,126,125,125,124,124,
		124,123,123,122,122,121,121,120,119,119,119,118,118,117,116,
		116,115,115,114,113,112,112,112,111,110,109,108,107,106,106,
		105,103,103,102,100,100,98,97,96,93,92,89,86,84,79,
		74};

#define MAX_NUM_40W 136
const u8 table40[MAX_NUM_40W]={
		171,148,149,149,149,149,149,148,148,148,148,148,148,147,147,
		147,147,146,146,146,146,145,145,145,144,144,144,144,144,142,
		142,142,138,138,138,138,136,136,136,136,136,135,135,134,133,
		133,133,133,132,132,132,131,131,130,129,129,128,128,127,127,
		127,126,126,125,124,124,124,124,123,122,122,121,120,120,119,
		119,118,117,117,116,115,115,114,113,113,112,112,111,110,109,
		107,107,106,105,104,103,102,101,100,99,97,96,94,93,87,
		85,82,78,73};

#define MAX_NUM_45W 100
const u8 table45[MAX_NUM_45W]={
		183,158,157,156,156,156,155,154,153,153,153,152,151,151,150,150,
		149,149,148,147,147,147,147,146,146,145,145,144,144,144,142,142,
		142,141,141,140,139,139,138,138,138,136,136,136,136,135,134,134,
		133,133,133,132,131,131,130,129,128,128,127,127,127,126,125,124,
		124,124,123,122,121,120,120,119,118,117,116,115,115,114,113,112,
		112,110,109,107,106,106,104,103,102,100,99,97,95,93,90,87,
		84,80,74,66};

#define MAX_NUM_50W 90
const u8 table50[MAX_NUM_50W]={
		183,162,162,161,160,159,158,157,156,156,156,155,154,153,153,153,
		152,152,151,150,149,149,148,148,147,147,146,146,145,144,144,144,
		142,142,142,141,141,140,139,138,138,138,136,136,136,135,134,133,
		133,133,132,131,130,130,128,128,127,127,126,125,124,124,123,122,
		121,120,119,119,118,117,115,115,113,112,111,110,108,106,105,104,
		102,100,99,96,94,92,88,84,80,73};

#define MAX_NUM_55W 75
const u8 table55[MAX_NUM_55W]={
		180,154,153,152,152,151,150,149,148,148,147,147,146,146,145,
		144,144,142,142,141,140,139,138,138,138,136,136,136,135,134,
		133,133,133,133,131,131,130,129,128,127,127,127,126,125,124,
		124,123,122,121,120,119,119,118,117,116,115,114,113,112,111,
		109,108,106,105,104,102,101,99,97,95,92,88,84,80,72};


u16 voltage_u16;

u16 get_akb_voltage(void){
	return voltage_u16;
}


/*вычисление оставшегося времени работы от АКБ*/
uint8_t UpsTime(struct RemainingTimeUPS_t *remtime){

static uint16_t i=0;
static uint32_t MeasuringCnt=0;//счетчик измерений
//static uint32_t TimeWork=0;//время работы, прошедшее с момента отключения сетевого напряжения
static uint32_t TimeWorkStamp=0;//штамп время работы, прошедшее с момента отключения сетевого напряжения
static uint32_t RemainTime=0,RemainTimeLeft=0,RemainTimeRight=0;

uint8_t n=0;

u32 Power_u32;
u32 PSW_POWER_u32;
static u32 PoeVoltageMiddle_u32=0,PoeVoltage_u32=0,SumCurrMiddle_u32=0;
static u32 SumVoltageMid_u32=0,SumCurrMid_u32=0;
static u32 SummCurr_u32  = 0;


DEBUG_MSG(UPS_DEBUG,"---------------\r\n");
/*получаем информацию*/
//напряжение на АКБ

u8 tmp_buf;


tmp_buf=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_READ);
if(tmp_buf != 255){
	DEBUG_MSG(UPS_DEBUG,"Read voltage %d\r\n",tmp_buf);
	voltage_u16 = (tmp_buf+350);
}
else{
	//if incorrect voltage read
	//TimeWork=0;
	remtime->hour=0;
	remtime->min=0;
	remtime->valid = 0;
	return 1;
}


if((voltage_u16<400)||(voltage_u16>560)){
	voltage_u16 = get_akb_voltage();
	if((voltage_u16<400)||(voltage_u16>560))
		voltage_u16 = 500;
}

/*если притание от АКБ считаем прошедшее время работы*/
if(UPS_rezerv){

	if(TimeWorkStamp==0){}
	//	TimeWorkStamp=RTC_GetCounter();
	//TimeWork=RTC_GetCounter()-TimeWorkStamp;


/*1. напряжение на АКБ*/



DEBUG_MSG(UPS_DEBUG,"AKB Voltage = %d.%d\r\n", (voltage_u16/10),(voltage_u16)%10);



SummCurr_u32=0;
PoeVoltage_u32=0;
n=0;
/*2. суммарный потребляемый ток в мА*/
/*2. напряжение в мВ*/
for(u8 i=0;i<(POE_PORT_NUM);i++){
	if(get_port_poe_a(i)){
		SummCurr_u32 += get_poe_current(POE_A,i);
		PoeVoltage_u32 += get_poe_voltage(POE_A,i);
		n++;
	}
	if(get_port_poe_b(i)){
		SummCurr_u32 += get_poe_current(POE_B,i);
		PoeVoltage_u32 += get_poe_voltage(POE_B,i);
		n++;
	}
}


/*усреднение осуществляется на основании 10 замеров*/
//if(MeasuringCnt>20){
//	SumCurrMiddle=0;
///	PoeVoltageMiddle=0;
//	MeasuringCnt=0;
//	SumVoltageMid=0;
//	SumCurrMid=0;
//}


MeasuringCnt++;

if(n){
	SumVoltageMid_u32 += (PoeVoltage_u32/(n*1000));//в вольтах
	SumCurrMid_u32 += SummCurr_u32;//в мА
}
else{
	SumVoltageMid_u32=0;
	SumCurrMid_u32=0;
}


//если подключена нагрузка
if(n){
	if(MeasuringCnt){
		SumCurrMiddle_u32 = (SumCurrMid_u32/MeasuringCnt);//усредняем значение потребляемого тока по всем измерениям
		DEBUG_MSG(UPS_DEBUG,"SumCurrMiddle = %d\r\n", (int)SumCurrMiddle_u32);

		PoeVoltageMiddle_u32 = SumVoltageMid_u32/MeasuringCnt;
		DEBUG_MSG(UPS_DEBUG,"MidleVoltagePoe = %d\r\n", (int)PoeVoltageMiddle_u32);
	}
}
else{
	SumCurrMiddle_u32=0;
	PoeVoltageMiddle_u32=0;
}

	/*мощность, потребляемая PSW в мВт*/
	PSW_POWER_u32=5200;
	for(n=0;n<COOPER_PORT_NUM;n++){
		if(get_port_link(n))
			PSW_POWER_u32+=200;
	}
	for(n=COOPER_PORT_NUM;n<(COOPER_PORT_NUM+FIBER_PORT_NUM);n++){
		if(get_port_link(n)==1)
			PSW_POWER_u32+=800;
	}

	/*суммарная потребляемая мощность в мВт*/
	Power_u32=(PoeVoltageMiddle_u32*SumCurrMiddle_u32*100/RIP_EFFECTIVITY)+PSW_POWER_u32;
	DEBUG_MSG(UPS_DEBUG,"Total power = %lu\r\n", (Power_u32));


	/*определяем таблицы, по которым апроксимируем мощность*/
	if(Power_u32<=10000){
		Power_u32=10000;
		i=0;
		while(voltage_u16<(table10[i]+350)){
			i++;
		}
		RemainTime=MAX_NUM_10W-i;
		goto ret;
	}
	else{
		if(Power_u32>=55000){
			Power_u32=55000;
			i=0;
			while(voltage_u16<(table55[i]+350)){
				i++;
			}
			RemainTime=MAX_NUM_55W-i;
		}
		else{
				if(Power_u32<15000){
						i=0;
						while(voltage_u16<(table10[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_10W-i;
						i=0;
						while(voltage_u16<(table15[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_15W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						//RemainTime=RemainTimeLeft+(Power-10)*(RemainTimeRight-RemainTimeLeft)/5;
						RemainTime=RemainTimeLeft-((Power_u32-10000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}

				if(Power_u32<20000){
						i=0;
						while(voltage_u16<(table15[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_15W-i;
						i=0;
						while(voltage_u16<(table20[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_20W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						RemainTime=RemainTimeLeft-((Power_u32-15000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}

				if(Power_u32<25000){
						i=0;
						while(voltage_u16<(table20[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_20W-i;
						i=0;
						while(voltage_u16<(table25[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_25W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						RemainTime=RemainTimeLeft-((Power_u32-20000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}
				if(Power_u32<30000){
						i=0;
						while(voltage_u16<(table25[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_25W-i;
						i=0;
						while(voltage_u16<(table30[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_30W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						//RemainTime=RemainTimeLeft+(Power-25)*(RemainTimeRight-RemainTimeLeft)/5;
						RemainTime=RemainTimeLeft-((Power_u32-25000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}
				if(Power_u32<35000){
						i=0;
						while(voltage_u16<(table30[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_30W-i;
						i=0;
						while(voltage_u16<(table35[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_35W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						//RemainTime=RemainTimeLeft+(Power-30)*(RemainTimeRight-RemainTimeLeft)/5;
						RemainTime=RemainTimeLeft-((Power_u32-30000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}
				if(Power_u32<40000){
						i=0;
						while(voltage_u16<(table35[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_35W-i;
						i=0;
						while(voltage_u16<(table40[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_40W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						RemainTime=RemainTimeLeft-((Power_u32-35000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}
				if(Power_u32<45000){
						i=0;
						while(voltage_u16<(table40[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_40W-i;
						i=0;
						while(voltage_u16<(table45[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_45W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						RemainTime=RemainTimeLeft-((Power_u32-40000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}
				if(Power_u32<50000){
						i=0;
						while(voltage_u16<(table45[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_45W-i;
						i=0;
						while(voltage_u16<(table50[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_50W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						RemainTime=RemainTimeLeft-((Power_u32-45000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}
				if(Power_u32<55000){
						i=0;
						while(voltage_u16<(table50[i]+350)){
							i++;
						}
						RemainTimeLeft=MAX_NUM_50W-i;
						i=0;
						while(voltage_u16<(table55[i]+350)){
							i++;
						}
						RemainTimeRight=MAX_NUM_55W-i;
						if(RemainTimeLeft<RemainTimeRight)
							RemainTimeLeft=RemainTimeRight;
						RemainTime=RemainTimeLeft-((Power_u32-50000)/5000)*(RemainTimeLeft-RemainTimeRight);
						goto ret;
				}


			DEBUG_MSG(UPS_DEBUG,"RemainTimeLeft = %d\r\n", (int)RemainTimeLeft);
			DEBUG_MSG(UPS_DEBUG,"RemainTimeRight = %d\r\n", (int)RemainTimeRight);
		}
	}

	ret:
/*потребляемая мощность в Вт*/
//Power=(PoeVoltage*SumCurrMiddle+PSW_POWER)/1000;
/*считаем оставшееся время работы(без учета напряжения) в секундах*/
//if(PoeVoltage>52)//когда АКБ полностью заряжена
//RemainTime=((-13.2*Power/24+11503)*60-TimeWork);
//RemainTime=UPS_CAPACITY/((float)((PoeVoltage*SumCurrMiddle+PSW_POWER)*100/RIP_EFFECTIVITY))-TimeWork;

DEBUG_MSG(UPS_DEBUG,"RemainTime = %d\r\n", (int)RemainTime);
DEBUG_MSG(UPS_DEBUG,"MeasuringCnt = %d\r\n", (int)MeasuringCnt);

//to log
#if UPS_DEBUG
	//sprintf(temp,"RemainTime = %d, Voltage=%d",(int)RemainTime,(int)(voltage));
	//write_log_bb(temp);
#endif


if(RemainTime>1200)
	RemainTime=600;
if(RemainTime==0)
	RemainTime=1;
remtime->hour=RemainTime/60;
remtime->min=RemainTime%60;
remtime->valid = 1;
return 0;

}else{
	MeasuringCnt=0;
	SumCurrMiddle_u32=0;
	PoeVoltageMiddle_u32=0;
	SumVoltageMid_u32=0;
	SumCurrMid_u32=0;

	//TimeWork=0;
	remtime->hour=0;
	remtime->min=0;
	remtime->valid = 0;
	return 1;
}
}


void UPS_PLC_detect(void){
static uint8_t init=0;
u8 temp;
if(init == 0){
	/*НА ПЛАТЕ PSW-1G есть особенность: другое расположение линй в инфю шнуре к плате IRP*/
	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_DET_PIN_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_DET_GPIO_2, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_VAC_PIN_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_VAC_GPIO_2, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_VAKB_PIN_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_VAKB_GPIO_2, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_VOUT_PIN_2;//достаточный заряд АКБ
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_VOUT_GPIO_2, &GPIO_InitStructure);

	}
	else
	{
		//для всех других плат, все остаётся по-старому
		GPIO_InitStructure.GPIO_Pin = LINE_UPS_DET_PIN_1;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_DET_GPIO_1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_VAC_PIN_1;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_VAC_GPIO_1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_VAKB_PIN_1;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_VAKB_GPIO_1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = LINE_UPS_VOUT_PIN_1;//достаточный заряд АКБ
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_UPS_VOUT_GPIO_1, &GPIO_InitStructure);
	}


	if((((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS))
		&& (GPIO_ReadInputDataBit(LINE_UPS_DET_GPIO_2,LINE_UPS_DET_PIN_2)==Bit_RESET))||
		((get_dev_type() != DEV_PSW1G4F)&&(get_dev_type() != DEV_PSW1G4FUPS)&&
		(GPIO_ReadInputDataBit(LINE_UPS_DET_GPIO_1,LINE_UPS_DET_PIN_1)==Bit_RESET))	)
	{
			if(i2c_probe(PLC_ADDR)==0){
				i2c_buf_read(PLC_ADDR,HW_VERSION,&temp,1);
				vTaskDelay(200*MSEC);
				if(i2c_buf_read(PLC_ADDR,HW_VERSION,&temp,1) == 0){
					PLC_connected = 1;
					set_plc_hw_vers(temp);
					dev.plc_status.new_event = 1;
				}
			}
			vTaskDelay(200*MSEC);

			//ups detect
			if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS) ||
			  (get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)){
				UPS_connected = 1;
				if(i2c_probe(IRP_ADDR)==0){
					temp = read_byte_reg(IRP_ADDR,Version_READ);
					dev.ups_status.hw_vers = temp;
				}
			}
			else if(get_dev_type() == DEV_PSW2G2FPLUS || get_dev_type() ==DEV_PSW2G2FPLUSUPS){
				if(i2c_probe(IRP_ADDR)==0){
					temp = read_byte_reg(IRP_ADDR,Version_READ);
					UPS_connected = 1;
					dev.ups_status.hw_vers = temp;
				}
			}
			init=1;
		}else{
			UPS_connected=0;
			PLC_connected = 0;
			init=1;
			return;
		}
}


/******************проверяем инф. линии************************/
if(UPS_connected==1){
	//для psw-2g2gf+ или для IRP c FW >= v8 связь только по i2c
	if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)||
	  dev.ups_status.hw_vers>=VERS_8){
		//наличие АКБ
		read_byte_reg(IRP_ADDR,AKB_PRESENT);
		vTaskDelay(100*MSEC);
		read_byte_reg(IRP_ADDR,AKB_PRESENT);
		vTaskDelay(100*MSEC);
		UPS_AKB_detect = read_byte_reg(IRP_ADDR,AKB_PRESENT);
		if(UPS_AKB_detect!=0 && UPS_AKB_detect!=1)
			UPS_AKB_detect = 0;
		vTaskDelay(100*MSEC);
		//тип питания
		read_byte_reg(IRP_ADDR,VAC_READ);
		vTaskDelay(100*MSEC);
		UPS_rezerv = read_byte_reg(IRP_ADDR,VAC_READ);
		if(UPS_rezerv != 0 && UPS_rezerv != 1){
			UPS_rezerv = 0;
		}
		else{
			UPS_rezerv = ~UPS_rezerv & 1;
		}

		UPS_Low_Voltage = 1;
		if(UPS_rezerv && UPS_AKB_detect){
			if(get_akb_voltage() < 440)
				UPS_Low_Voltage = 0;
		}
	}
	else{
		//для всех остальных
		/*аккумуляторы подключены*/
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			if (GPIO_ReadInputDataBit(LINE_UPS_VAKB_GPIO_2,LINE_UPS_VAKB_PIN_2)==Bit_RESET) {
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAKB_GPIO_2,LINE_UPS_VAKB_PIN_2)==Bit_RESET)
					UPS_AKB_detect=1;
			}else{
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAKB_GPIO_2,LINE_UPS_VAKB_PIN_2)==Bit_SET)
					UPS_AKB_detect=0;
			}
		}
		else{
			if (GPIO_ReadInputDataBit(LINE_UPS_VAKB_GPIO_1,LINE_UPS_VAKB_PIN_1)==Bit_RESET) {
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAKB_GPIO_1,LINE_UPS_VAKB_PIN_1)==Bit_RESET)
					UPS_AKB_detect=1;
			}else{
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAKB_GPIO_1,LINE_UPS_VAKB_PIN_1)==Bit_SET)
					UPS_AKB_detect=0;
			}
		}

		/*переключение на резерв*/
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			if (GPIO_ReadInputDataBit(LINE_UPS_VAC_GPIO_2,LINE_UPS_VAC_PIN_2)==Bit_RESET) {
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAC_GPIO_2,LINE_UPS_VAC_PIN_2)==Bit_RESET)
					UPS_rezerv=1;
			}else{
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAC_GPIO_2,LINE_UPS_VAC_PIN_2)==Bit_SET)
					UPS_rezerv=0;
			}
		}else{
			if (GPIO_ReadInputDataBit(LINE_UPS_VAC_GPIO_1,LINE_UPS_VAC_PIN_1)==Bit_RESET) {
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAC_GPIO_1,LINE_UPS_VAC_PIN_1)==Bit_RESET)
					UPS_rezerv=1;
			}else{
				vTaskDelay(50*MSEC);
				if (GPIO_ReadInputDataBit(LINE_UPS_VAC_GPIO_1,LINE_UPS_VAC_PIN_1)==Bit_SET)
					UPS_rezerv=0;
			}
		}



	/*достаточный заряд акб*/
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			if (GPIO_ReadInputDataBit(LINE_UPS_VOUT_GPIO_2,LINE_UPS_VOUT_PIN_2)==Bit_RESET) {
				UPS_Low_Voltage=1;//достаточный
			}else{
				UPS_Low_Voltage=0;//низкий заряд
			}
		}else{
			if (GPIO_ReadInputDataBit(LINE_UPS_VOUT_GPIO_1,LINE_UPS_VOUT_PIN_1)==Bit_RESET) {
				UPS_Low_Voltage=1;//достаточный
			}else{
				UPS_Low_Voltage=0;//низкий заряд
			}
		}




	}


	//при изменении статуса
	if(RezervLastState!=UPS_rezerv){
		if((UPS_rezerv==1)||(UPS_rezerv==0)){
			send_events_u32(EVENTS_UPS_REZERV,(u32)UPS_rezerv);
		}
		RezervLastState=UPS_rezerv;
	}

	if((LASTUPS_Low_Voltage!=UPS_Low_Voltage)&&(UPS_rezerv==1)){
		if((UPS_Low_Voltage==0)){
			send_events_u32(EVENTS_UPS_LOW_VOLTAGE,(u32)UPS_Low_Voltage);
		}
		LASTUPS_Low_Voltage=UPS_Low_Voltage;
	}


}
else{
	UPS_rezerv=0;
	UPS_Low_Voltage=1;
	UPS_AKB_detect=1;
}
}

/*return current status  ups/ no ups  */
u8 is_ups_mode(void){
	return UPS_connected;
}


/*return akb precision*/
u8 is_akb_detect(void){
	return UPS_AKB_detect;
}

/*return 1 - UPS powered
 * return 0 - VAC powered*/
u8 is_ups_rezerv(void){
	return UPS_rezerv;
}


/*return current status  plc/ no plc  */
//детекция подключения платы PLC
u8 is_plc_connected(void){
	return PLC_connected;
}


//сохранение состояния функции отложенного запуска UPS
u8 set_ups_delay_start(u8 state){
u8  temp[33];
	memset(temp,0,33);
	temp[0] = state;
	i2c_reg_write(IRP_ADDR,DELAYED_START_WRITE,temp);
	return 0;
}

int get_ups_delay_start(void){
u8 temp;
	i2c_buf_read(IRP_ADDR,DELAYED_START_READ,&temp,1);
	vTaskDelay(50*MSEC);
	i2c_buf_read(IRP_ADDR,DELAYED_START_READ,&temp,1);
	if(temp == ENABLE || temp == DISABLE)
		return temp;
	else
		return -1;
}



