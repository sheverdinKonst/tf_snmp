
#include "stm32f4xx.h"
#include "FreeRtos.h"
#include "task.h"
#include "../net/uip/uip.h"
#include "autorestart.h"
#include "board.h"
#include "settings.h"
#include "../net/syslog/msg_build.h"
#include "../net/smtp/smtp.h"
#include "poe_ltc.h"
#include "../net/icmp/icmp.h"
#include "stm32f4xx_rtc.h"
#include "../deffines.h"
#include "eeprom.h"

extern struct status_t status;


void SoftStartTask(void *pvParameters){
static struct timer ss_timer;
RTC_TimeTypeDef RTC_Time;

	for(u8 i = 0;i<POE_PORT_NUM;i++){
		dev.port_stat[i].ss_process = 0;
	}
	//если время прогрева еще не завершилось
	RTC_GetTime(RTC_Format_BIN,&RTC_Time);
	if(/*(reboot_flag == POWER_RESET)&&*/
		((RTC_Time.RTC_Hours*3600 + RTC_Time.RTC_Minutes*60 + RTC_Time.RTC_Seconds)< get_softstart_time()*3600)){
		for(u8 i = 0;i<POE_PORT_NUM;i++){
			if(get_port_sett_soft_start(i) == 1){
				dev.port_stat[i].ss_process = 1;
			}
			set_poe_init(0);
		}

		//ждем время прогрева
		timer_set(&ss_timer, ((get_softstart_time()*3600 - (RTC_Time.RTC_Hours*3600 + RTC_Time.RTC_Minutes*60 + RTC_Time.RTC_Seconds)))*MSEC*1000);

		while(timer_expired(&ss_timer)==0){
			vTaskDelay(1000*MSEC);
		}
	}
	for(u8 i = 0;i<POE_PORT_NUM;i++){
		dev.port_stat[i].ss_process = 0;
	}
	set_poe_init(0);
	vTaskDelete( NULL);
}
