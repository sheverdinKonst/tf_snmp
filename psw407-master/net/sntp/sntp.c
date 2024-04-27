//
//  $Id: sntp.c 331 2008-11-09 16:59:47Z jcw $
//  $Revision: 331 $
//  $Author: jcw $
//  $Date: 2008-11-09 11:59:47 -0500 (Sun, 09 Nov 2008) $
//  $HeadURL: http://tinymicros.com/svn_public/arm/lpc2148_demo/trunk/uip/apps/sntp/sntp.c $
//
#include <string.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4xx_rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../uip/uip.h"
#include "../stp/stp_oslayer_freertos.h"
#include "../uip/timer.h"
#include "../uip/pt.h"
#include "sntp.h"
#include "../uip/timer.h"
#include "names.h"
#include "board.h"
#include "settings.h"
#include "selftest.h"
#include "../uip/timer.h"
#include "../dns/resolv.h"
#include "debug.h"




#define JAN_1970 0x83aa7e80 /* 2208988800 1970 - 1900 in seconds */

#define NTP_TO_UNIX(n,u) do {  u = n - JAN_1970; } while (0)

#define NTOHL_FP(n, h)  do { (h)->l_ui = ntohl((n)->l_ui); \
                             (h)->l_uf = ntohs((n)->l_uf); } while (0)

//
//
//
#define CLOCK_SECOND		( 100  )

typedef enum
{
  LEAPINDICATOR_NO_WARNING = 0,
  LEAPINDICATOR_61_SECOND_DAY,
  LEAPINDICATOR_59_SECOND_DAY,
  LEAPINDICATOR_ALARM,
}
leapIndicator_e;

typedef enum
{
  MODE_RESERVED = 0,
  MODE_SYMMETRIC_ACTIVE,
  MODE_SYMMETRIC_PASSIVE,
  MODE_CLIENT,
  MODE_SERVER,
  MODE_BROADCAST,
  MODE_NTP_CONTROL_MSG,
  MODE_PRIVATE,
}
mode_e;

//
//  RFC1305
//
#define l_ui Ul_i.Xl_ui /* unsigned integral part */
#define l_i  Ul_i.Xl_i  /* signed integral part */
#define l_uf Ul_f.Xl_uf /* unsigned fractional part */
#define l_f  Ul_f.Xl_f  /* signed fractional part */

uip_ipaddr_t uip_sntpaddr;
const u32 uip_timeoffset = 0;

#define uip_getsntpaddr(addr) uip_ipaddr_copy((addr), uip_sntpaddr)
#define uip_gettimeoffset(addr) *addr=uip_timeoffset

static const uip_ipaddr_t all_zeroes_addr = {0x0000,0x0000};
static u8 sntp_run=0;//0 - задача sntp не запущена
//static u8 sntp_busy=0;
u32 TimeStamp;

xTaskHandle xsntp_task;

struct timer sntp_dns_timer;

typedef struct 
{
  union 
  {
    u32 Xl_ui;
    u32 Xl_i;
  } 
  Ul_i;

  union 
  {
    u32 Xl_uf;
    u32 Xl_f;
  } 
  Ul_f;
} 
__attribute__ ((packed)) l_fp;

typedef struct sntpHeader_s
{
  u16 mode          : 3;
  u16 versionNumber : 3;
  u16 leapIndicator : 2;
  u16 stratum       : 8;
  u16 poll          : 8;
  u16 precision     : 8;
  u32 rootDelay;
  u32 rootDispersion;
  u32 refID;
  l_fp  refTimeStamp;
  l_fp  orgTimeStamp;
  l_fp  rxTimeStamp;
  l_fp  txTimeStamp;
}
__attribute__ ((packed)) sntpHeader_t;

//
//
//
struct  sntpState_t sntpState;

//struct uip_udp_conn *sntp_conn;

const uint8_t month_day[] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const uint16_t week_day[] =
{ 0x4263, 0xA8BD, 0x42BF, 0x4370, 0xABBF, 0xA8BF, 0x43B2};


static uint8_t calendar_check_leap(uint16_t year)
{
	if ((year % 400) == 0)
	{
		return TRUE;
	}
	else if ((year % 100) == 0)
	{
		return FALSE;
	}
	else if ((year % 4) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void DateConv(u32 *time_stamp,struct date_t *date){
int16_t tmp=0;

	//printf("time_stamp1 %lu\r\n",*time_stamp);
	*time_stamp = (*time_stamp - 3155673600/* + get_sntp_timezone()*3600*/);
	//printf("time_stamp2 %lu\r\n",*time_stamp);

	date->year= *time_stamp/(365 * 24 * 3600);
	tmp = ((*time_stamp % (365 * 24 * 3600)) / (24 * 3600)) - (date->year / 4);
	if ((tmp <= 0) && (date->year > 3))
	{
		date->year--;
		if (calendar_check_leap(date->year + 2000))
		{
			tmp += 366;
		}
		else
		{
			tmp += 365;
		}
	}

	if (calendar_check_leap(date->year + 2000)){
		tmp++;
	}

	date->month = 0;
	while (tmp > 0)
	{
		date->day = tmp;
		if (date->month++ == 2)
		{
			if (calendar_check_leap(date->year + 2000))
				tmp -= month_day[date->month - 1] + 1;
			else
				tmp -= month_day[date->month - 1];
		}
		else
		{
			tmp -= month_day[date->month - 1];
		}
	}

	date->day_of_the_week = 0;//calendar_week_day(2000 + date->year,date->month, date->day);
	date->hour = ((*time_stamp % (365 * 24 * 3600)) % (24 * 3600)) / 3600 + get_sntp_timezone();
	if(date->hour == 24)
		date->hour = 0;
	date->min = (((*time_stamp % (365 * 24 * 3600)) % (24 * 3600)) % 3600)/ 60;
	date->sec = (((*time_stamp % (365 * 24 * 3600)) % (24 * 3600)) % 3600)% 60;
}


static u16_t uip_iszeroaddr (uip_ipaddr_t *addr)
{
  return uip_ipaddr_cmp (addr, all_zeroes_addr);
}


/*
void uipAutoSNTP (void)
{
  uip_ipaddr_t addr;

  uip_getsntpaddr (&addr);

  if (!uip_iszeroaddr (&addr))
    //sntpSync (&addr, uipAutoSNTPTimeSynced);
    sntpSync (&addr);
}
*/

int sntpSync (uip_ipaddr_t *ipaddr)
{
  DEBUG_MSG(SNTP_DEBUG,"sntpSync()\r\n");
  if (sntpState.sntp_conn != NULL){
    uip_udp_remove (sntpState.sntp_conn);
    DEBUG_MSG(SNTP_DEBUG,"remove udp conn\r\n");
  }

  DEBUG_MSG(SNTP_DEBUG,"Server IP %d.%d.%d.%d\r\n",uip_ipaddr1(ipaddr),uip_ipaddr2(ipaddr),uip_ipaddr3(ipaddr),uip_ipaddr4(ipaddr));

  sntpState.sntp_conn = uip_udp_new (ipaddr, HTONS (SNTP_SERVER_PORT));
  if (sntpState.sntp_conn != NULL){
	  //sntpState = (struct sntpState_t *) &sntp_conn->appstate;
	  //sntpState->conn = sntp_conn;
	  sntpState.retry = SNTP_RETRIES;
	  uip_udp_bind (sntpState.sntp_conn, HTONS (SNTP_SERVER_PORT));
	  DEBUG_MSG(SNTP_DEBUG,"conn created\r\n");
  }
  PT_INIT (&sntpState.pt);
  return 1;
}
//
//
//
void sntpUpdate (void)
{
  DEBUG_MSG(SNTP_DEBUG,"sntpUpdate\r\n");
  sntpHeader_t *hdr = (sntpHeader_t *) uip_appdata;
  memset (hdr, 0, sizeof (sntpHeader_t));
  hdr->leapIndicator = LEAPINDICATOR_ALARM;
  hdr->versionNumber = 4;
  hdr->mode = MODE_CLIENT;
  uip_udp_send (sizeof (sntpHeader_t));
  //DEBUG_MSG(SNTP_DEBUG,"stop sntpUpdate\r\n");
}

static void sntpConvertToTm (sntpHeader_t *h, time_t *epochSeconds)
{
  sntpHeader_t temp;
  NTOHL_FP (&h->txTimeStamp, &temp.txTimeStamp);
  NTP_TO_UNIX (temp.txTimeStamp.Ul_i.Xl_ui, *epochSeconds);
  DEBUG_MSG(SNTP_DEBUG,"sntpConvertToTm %lu  %lu\r\n",(u32)&epochSeconds,(u32)temp.txTimeStamp.Ul_i.Xl_ui);
  TimeStamp=temp.txTimeStamp.Ul_i.Xl_ui;
  DateConv(&TimeStamp,&date);
  DEBUG_MSG(SNTP_DEBUG,"time %d:%d:%d\r\n",date.hour,date.min,date.sec);
  DEBUG_MSG(SNTP_DEBUG,"date %d %d %d %d\r\n",date.day, date.month, date.year, date.day_of_the_week);
}

static PT_THREAD (sntpHandler (void))
{
  DEBUG_MSG(SNTP_DEBUG,"start sntpHandler\r\n");
  PT_BEGIN (&sntpState.pt);
  do
  {
	sntpUpdate ();
    DEBUG_MSG(SNTP_DEBUG,"sntpUpdate retry=%d\r\n",sntpState.retry);
    timer_set (&sntpState.timer, (SNTP_TIMEOUT * CLOCK_SECOND));

     PT_WAIT_UNTIL (&sntpState.pt, (!uip_poll () && uip_newdata ()) || timer_expired (&sntpState.timer));
     sntpState.retry--;
  }
  while ((uip_poll ()) && (sntpState.retry>0));

  if (timer_expired (&sntpState.timer))
  {
	DEBUG_MSG(SNTP_DEBUG,"uip_udp_remove\r\n");
    uip_udp_remove (sntpState.sntp_conn);
  }
  else
  {
    if (((sntpHeader_t *) uip_appdata)->refID)
    {
      time_t epochSeconds;
      DEBUG_MSG(SNTP_DEBUG,"sntpConvertToTm\r\n");
      sntpConvertToTm ((sntpHeader_t *) uip_appdata, &epochSeconds);
      uip_udp_remove (sntpState.sntp_conn);
      DEBUG_MSG(SNTP_DEBUG,"uip_udp_remove\r\n");
    }
  }
  while (1)
	 PT_YIELD (&sntpState.pt);
  PT_END (&sntpState.pt);
}



void sntp_appcall (void)
{

  if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
	  return;

  DEBUG_MSG(SNTP_DEBUG,"start sntp_appcall\r\n");
  sntpHandler ();

}

void SNTP_get_time(void){
	if(get_sntp_state()==ENABLE){
		if(uip_arp_out_check(sntpState.sntp_conn) == 0)
			sntp_appcall();
	}
}

void sntp_task(void *p){
static u32 Last,Now;
sntp_run=1;
DEBUG_MSG(SNTP_DEBUG,"sntp_task start\r\n");
//vTaskDelay(3000*MSEC);

Last=RTC_GetCounter();

	while(1){
		if((get_sntp_state()==ENABLE)){
			if((RTC_GetCounter()-Last)>(get_sntp_period()*60)){
				//если прошел период, синхронизируемся с часами
				Last=RTC_GetCounter();
				DEBUG_MSG(SNTP_DEBUG,"SNTP_get_time\r\n");
				//SNTP_get_time();
				sntpSync(&uip_sntpaddr);
			}
			else{
				//иначе работаем по встроенным RTC часам
				Now=TimeStamp+RTC_GetCounter()-Last+3155673600;
				DateConv(&Now,&date);
			}
		}
		vTaskDelay(1000*MSEC);
	}
}

void SNTP_start_task(void){
	if(xTaskCreate( sntp_task, (void*)"SNTP",128*2, NULL, DEFAULT_PRIORITY, &xsntp_task)
			==errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
		 ADD_ALARM(ERROR_CREATE_SNTP_TASK);
	}
}

void SNTP_config(void){
uip_ipaddr_t tmp_ip,dm_ip;
char serv_name[64];
	if(get_sntp_state()==ENABLE){

		get_sntp_serv(&tmp_ip);
		get_sntp_serv_name(serv_name);
		if((tmp_ip[0]==0)&&(tmp_ip[1]==0)){
			//static ip is not set, use dns
			resolv_lookup(serv_name,&dm_ip);
			DEBUG_MSG(SNTP_DEBUG,"sntp server dns\r\n");
			if((dm_ip[0]==0)&&(dm_ip[1]==0)){
				DEBUG_MSG(SNTP_DEBUG,"sntp server don`t exist, dns query\r\n");
				resolv_query(serv_name);
				timer_set(&sntp_dns_timer, 10000 * MSEC  );
			}
			else
				uip_ipaddr_copy(uip_sntpaddr,dm_ip);
		}
		else{
			DEBUG_MSG(SNTP_DEBUG,"sntp server static ip\r\n");
			uip_ipaddr_copy(uip_sntpaddr,tmp_ip);
		}



		DEBUG_MSG(SNTP_DEBUG,"start sntp client. SNTP server addr %d.%d.%d.%d\r\n",
			uip_ipaddr1(&uip_sntpaddr),uip_ipaddr2(&uip_sntpaddr),uip_ipaddr3(&uip_sntpaddr),uip_ipaddr4(&uip_sntpaddr));

		sntpSync (&uip_sntpaddr);

		if(sntp_run==0){
			SNTP_start_task();
		}
	}
}

void set_sntp_cfg_default(void){
	uip_ipaddr_t ip;
	uip_ipaddr(ip,0,0,0,0);

	set_sntp_state(0);
	set_sntp_serv(ip);
	set_sntp_period(10);
	set_sntp_timezone(0);

	//Write_Eeprom(SntpCfgE, &sntp_cfg, 8);

	/*EeSetVar(SNTP_SETT_SERV,get_sntp_serv());
	EeSetVar(SNTP_PERIOD,get_sntp_period());
	EeSetVar(SNTP_STATE,get_sntp_state());
	EeSetVar(SNTP_TIMEZONE,get_sntp_timezone());
	*/


}
