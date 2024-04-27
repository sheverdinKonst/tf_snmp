//
//  $Id: sntp.h 331 2008-11-09 16:59:47Z jcw $
//  $Revision: 331 $
//  $Author: jcw $
//  $Date: 2008-11-09 11:59:47 -0500 (Sun, 09 Nov 2008) $
//  $HeadURL: http://tinymicros.com/svn_public/arm/lpc2148_demo/trunk/uip/apps/sntp/sntp.h $
//

#ifndef _SNTP_H_
#define _SNTP_H_

#define SNTP_SERVER_PORT 123
//#include <time.h>
//#include "net/uip/timer.h"
#include "../uip/uip.h"
//
//
//
#define SNTP_TIMEOUT 10
#define SNTP_RETRIES 10
//
//
//



//struct SNTP_Cfg sntp_cfg;

struct sntpState_t
{
  struct pt pt;
  struct uip_udp_conn *sntp_conn;
  struct timer timer;
  int retry;
  //void (*onSyncCallback) (time_t *epochSeconds);
} ;

typedef struct sntpState_t uip_udp_appstate_sntp;

struct date_t{
	u16 year;
	u8  month;
	u8  day_of_the_week;
	u8  day;
	u8 	hour;
	u8  min;
	u8  sec;
};

struct date_t date;


//
//
//
//void uipAutoSNTP (void);
int sntpSync (uip_ipaddr_t *ipaddr);
void sntp_appcall (void);
void SNTP_get_time(void);
void sntpUpdate (void);
void SNTP_config(void);
void sntp_task(void *p);
void SNTP_start_task(void);
void set_sntp_cfg_default(void);

#endif
