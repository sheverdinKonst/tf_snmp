#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>



//#include "stm32f10x_lib.h"

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_rtc.h"
//#include "eeprom/eeprom.h"
//#include "usb.h"
#include "../i2c/soft/i2c_soft.h"
#include "../i2c/hard/i2c_hard.h"
#include "stm32f4xx_rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../uip/uip.h"
#include "../uip/psock.h"
#include "../uip/timer.h"
#include "../webserver/httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"
#include "../inc/SMIApi.h"
#include "../inc/VLAN.h"
#include "../inc/SpeedDuplex.h"
#include "../inc/FlowCtrl.h"
#include "../inc/FlowCtrl.h"
#include "../inc/QoS.h"
#include "../inc/salsa2.h"
#include "../uip/uip_arp.h"
#include "../syslog/syslog.h"
#include "../syslog/cfg_syslog.h"
#include "../syslog/msg_build.h"
#include "../sntp/sntp.h"
#include "../smtp/smtp.h"
#include "../snmp/snmp.h"
#include "../snmp/snmpd/snmpd.h"
#include "../snmp/snmpd/snmpd-types.h"
#include "../snmp/snmpd/mib.h"
#include "../dhcp/dhcpc.h"
#include "../dhcp/dhcpr.h"
#include "../stp/bstp.h"
#include "../UPS/UPS_command.h"
#include "../board/board.h"
#include "../eeprom/names.h"
#include "../eeprom/eeprom.h"
#include "../poe/poe_ltc.h"
#include "../poe/poe_tps.h"
#include "../autorestart/autorestart.h"
#include "../flash/blbx.h"
#include "../events/events_handler.h"
#include "../igmp/igmpv2.h"
#include "../dns/resolv.h"
#include "../board/selftest.h"
#include "../board/settings.h"
#include "../eeprom/settingsfile.h"
#include "../telnet/telnetd.h"
#include "../plc/plc_def.h"
#include "../plc/plc.h"
#include "../plc/em.h"
#include "../sfp/sfp_cmd.h"
#include "debug.h"
#include "../flash/s25fl128p.h"
#include "../net/lldp/lldp_node.h"
#include "../net/snmp/snmpd/ber.h"


#define PSEUDO_FLOAT(x,y) x/y,((x*100)/y-((x/y)*100))

extern u8 init_ok;
extern uint8_t dev_addr[6];
extern uint8_t MyIP[4];
extern uint16_t LedPeriod;
extern uint8_t OtherCommandsFlag;
extern xTaskHandle xOtherCommands, xPoE1,xPoE2,xPoE3,xPoE4, xEraseBBHandle;
extern u32 backup_file_len;
struct alarm_list_t alarm_list[ALARM_REC_NUM];

//uint8_t SS;
extern struct bstp_state bstp_state;

extern uint8_t RunTaskCAM;
extern xTaskHandle xPoECamControl;
//extern uint8_t UPS;
extern uint8_t UPS_rezerv;
extern uint8_t UPS_AKB_detect;
//extern float voltage;

extern u8 port_statistics_start;

extern uint16_t need_update_flag[4];
	//extern uint16_t last_reset_status;
extern struct status_t status;

extern struct mac_entry_t mac_entry[MAX_CPU_ENTRIES_NUM];

extern struct RemainingTimeUPS_t remtime;

u8 dry_contact_alarm;



char str[ETH_MAX_PACKET_SIZE+MAX_STR_LEN] __attribute__ ((section (".ccmram")));

char temp[128];
extern uint8_t SendICMPPingFlag;
extern uip_ipaddr_t IPDestPing;
xTaskHandle xEraseBBHandle;
//static struct QoS qos_tmp;
u32 i;


void MemFree(uint16_t Addr, uint8_t len);
void http_url_decode(char *in,char *out,uint8_t mx);

HTTPD_CGI_CALL(cgi_main,    		"main",    			run_main);
HTTPD_CGI_CALL(cgi_header_link,		"header_link",		run_header_link);
HTTPD_CGI_CALL(cgi_linkpoe_json,  	"header_json",		run_linkpoe_json);
HTTPD_CGI_CALL(cgi_header_name,		"header_name",		run_header_name);
HTTPD_CGI_CALL(cgi_index,  			"index",			run_index);
HTTPD_CGI_CALL(cgi_ntwk_settings, 	"ntwk_settings", 	run_ntwk_settings);
HTTPD_CGI_CALL(cgi_admin_settings, 	"admin_settings", 	run_admin_settings);
HTTPD_CGI_CALL(cgi_clear,  			"clear",			run_clear);
HTTPD_CGI_CALL(cgi_update,    		"update",    		run_update);
HTTPD_CGI_CALL(cgi_reboot,    		"reboot",    		run_reboot);
HTTPD_CGI_CALL(cgi_FEGE_set,    	"FEGE_set",    		run_FEGE_set);
HTTPD_CGI_CALL(cgi_FEGE_stat,    	"FEGE_stat",    	run_FEGE_stat);
HTTPD_CGI_CALL(cgi_VCT,  	  		"VCT",	    		run_VCT);
HTTPD_CGI_CALL(cgi_Descr,    		"Descr",	    	run_Descr);
HTTPD_CGI_CALL(cgi_FactoryDefaults, "default",	     	run_FactoryDefaults);
HTTPD_CGI_CALL(cgi_SaveSettings,    "savesett",	     	run_SaveSettings);
HTTPD_CGI_CALL(cgi_ComfortStart,  	"ComfortStart",		run_ComfortStart);
HTTPD_CGI_CALL(cgi_AutoRestart,  	"AutoRestart",		run_AutoRestart);
HTTPD_CGI_CALL(cgi_Ping,  			"ping",				run_Ping);
HTTPD_CGI_CALL(cgi_VLAN_perport,  	"VLAN_perport",		run_PortBaseVLAN);
HTTPD_CGI_CALL(cgi_VLAN_8021q,  	"VLAN_8021q",		run_8021QVLAN);
HTTPD_CGI_CALL(cgi_VLAN_trunk,  	"VLAN_trunk",		run_VLAN_trunk);
HTTPD_CGI_CALL(cgi_Port_stat,  		"Port_stat",		run_Port_stat);
HTTPD_CGI_CALL(cgi_ARP,  			"ARP",				run_ARP);
HTTPD_CGI_CALL(cgi_MAC,  			"MAC",				run_MAC);
HTTPD_CGI_CALL(cgi_RSTP,  			"RSTP",				run_RSTP);
HTTPD_CGI_CALL(cgi_RSTP_stat,  		"RSTP_stat",		run_RSTP_stat);
HTTPD_CGI_CALL(cgi_PoE_stat,  		"PoE_stat",			run_PoE_stat);
HTTPD_CGI_CALL(cgi_syslog_set,		"syslog_set",		run_syslog_set);
//HTTPD_CGI_CALL(cgi_service,		"service",			run_service);
HTTPD_CGI_CALL(cgi_log,				"log",				run_log);
HTTPD_CGI_CALL(cgi_ups,				"ups",				run_ups);
HTTPD_CGI_CALL(cgi_QoS_rate_limit,  "QoS_rate_limit",	run_QoS_rate_limit);
HTTPD_CGI_CALL(cgi_swu_rate_limit,  "swu_rate_limit",	run_swu_rate_limit);
HTTPD_CGI_CALL(cgi_QoS_general,		"QoS_general",		run_QoS_general);
HTTPD_CGI_CALL(cgi_QoS_cos,			"QoS_cos",			run_QoS_cos);
HTTPD_CGI_CALL(cgi_QoS_tos,			"QoS_tos",			run_QoS_tos);
//HTTPD_CGI_CALL(cgi_PortTrunk,		"port_trunk",		run_PortTrunk);
HTTPD_CGI_CALL(cgi_sntp_set,		"sntp_set",		    run_sntp_set);
HTTPD_CGI_CALL(cgi_smtp_set,		"smtp_set",		    run_smtp_set);
HTTPD_CGI_CALL(cgi_snmp_set,		"snmp_set",		    run_snmp_set);
HTTPD_CGI_CALL(cgi_eventlist,		"eventlist",		run_eventlist);
//HTTPD_CGI_CALL(cgi_files,			"files",			run_files);
HTTPD_CGI_CALL(cgi_test_web,		"selftest",			run_test_web);
HTTPD_CGI_CALL(cgi_test,			"test",				run_test);
HTTPD_CGI_CALL(cgi_managment,		"managment",		run_language);
HTTPD_CGI_CALL(cgi_igmp,			"igmp",				run_igmp);
HTTPD_CGI_CALL(cgi_igmp_groups,		"groups",			run_igmp_groups);
HTTPD_CGI_CALL(cgi_telnet,			"telnet",			run_telnet);
HTTPD_CGI_CALL(cgi_dns_stat,  		"dns_stat",			run_dns_stat);
HTTPD_CGI_CALL(cgi_devmode,  		"devmode",			run_devmode);
HTTPD_CGI_CALL(cgi_port_info,  		"port_info",		run_port_info);
HTTPD_CGI_CALL(cgi_mib_list,  		"miblist",			run_mib_list);
HTTPD_CGI_CALL(cgi_plc,  			"plc",				run_plc);
HTTPD_CGI_CALL(cgi_access,			"access",			run_access);
HTTPD_CGI_CALL(cgi_make_em_json,  	"make_em_json",		run_make_em_json);
HTTPD_CGI_CALL(cgi_mac_bind,  		"mac_bind",			run_mac_bind);
HTTPD_CGI_CALL(cgi_mac_blocked,  	"mac_blocked",		run_mac_blocked);
//HTTPD_CGI_CALL(cgi_aggregation,  	"aggregation",		run_aggregation);
HTTPD_CGI_CALL(cgi_mirror,  		"mirror",			run_mirror);
HTTPD_CGI_CALL(cgi_teleport,  		"teleport",			run_teleport);
HTTPD_CGI_CALL(cgi_devinfo,  		"devinfo",			run_devinfo);
HTTPD_CGI_CALL(cgi_cpuinfo,  		"cpuinfo",			run_cpuinfo);
HTTPD_CGI_CALL(cgi_lldpsett,		"lldpsett",			run_lldpsett);
HTTPD_CGI_CALL(cgi_lldpstat,		"lldpstat",			run_lldpstat);
HTTPD_CGI_CALL(cgi_lldpinfo,		"lldpinfo",			run_lldpinfo);

//http api section
HTTPD_CGI_CALL(cgi_getInput,  		"getInput",			run_getInput);
HTTPD_CGI_CALL(cgi_getLink,  		"getLink",			run_getLink);
HTTPD_CGI_CALL(cgi_getPoePower,  	"getPoePower",		run_getPoePower);
HTTPD_CGI_CALL(cgi_getPoe,  		"getPoe",			run_getPoe);
HTTPD_CGI_CALL(cgi_setPoe,  		"setPoe",			run_setPoe);
HTTPD_CGI_CALL(cgi_getPortNum,  	"getPortNum",		run_getPortNum);
HTTPD_CGI_CALL(cgi_isUps,  			"isUps",			run_isUps);
HTTPD_CGI_CALL(cgi_getUpsStatus,  	"getUpsStatus",		run_getUpsStatus);
HTTPD_CGI_CALL(cgi_getUpsVoltage,  	"getUpsVoltage",	run_getUpsVoltage);
HTTPD_CGI_CALL(cgi_getUpsEstimated, "getUpsEstimated",	run_getUpsEstimated);
HTTPD_CGI_CALL(cgi_getNetIp,  		"getNetIp",			run_getNetIp);
HTTPD_CGI_CALL(cgi_getNetMac,  		"getNetMac",		run_getNetMac);
HTTPD_CGI_CALL(cgi_getNetMask,  	"getNetMask",		run_getNetMask);
HTTPD_CGI_CALL(cgi_getNetGate,  	"getNetGate",		run_getNetGate);
HTTPD_CGI_CALL(cgi_getFwVersion,  	"getFwVersion",		run_getFwVersion);
HTTPD_CGI_CALL(cgi_getDevType,  	"getDevType",		run_getDevType);
HTTPD_CGI_CALL(cgi_getSerialNum,  	"getSerialNum",		run_getSerialNum);
HTTPD_CGI_CALL(cgi_getDevName,  	"getDevName",		run_getDevName);
HTTPD_CGI_CALL(cgi_getDevLocation,  "getDevLocation",	run_getDevLocation);
HTTPD_CGI_CALL(cgi_getDevContact,   "getDevContact",	run_getDevContact);
HTTPD_CGI_CALL(cgi_getPortMacList,  "getPortMacList",	run_getPortMacList);
HTTPD_CGI_CALL(cgi_cableTesterStart,"cableTesterStart",	run_cableTesterStart);
HTTPD_CGI_CALL(cgi_cableTesterStatus,"cableTesterStatus",run_cableTesterStatus);
HTTPD_CGI_CALL(cgi_getUptime,		"getUptime",		run_getUptime);
HTTPD_CGI_CALL(cgi_rebootAll,		"rebootPSW",		run_rebootAll);

/*функции для динамического формирования веб страниц*/
//добавление строки
void addstr(char *text){
	if((strlen(str)+strlen(text))<ETH_MAX_PACKET_SIZE)
		strcat(str,text);
}
//добавление мультиязычной строки
static void addstrl(char *eng, char *rus){
	if(get_interface_lang() == ENG)
		addstr(eng);
	else
		addstr(rus);

}


#define alert(string) temp[0]=0;sprintf(temp,"<SCRIPT LANGUAGE=\"javascript\">alert(\"%s\");</script>",string); PSOCK_SEND_STR(&s->sout,temp); temp[0]=0;


#define alertl(string_en,string_ru) temp[0]=0;if(get_interface_lang() == ENG)\
		sprintf(temp,"<SCRIPT LANGUAGE=\"javascript\">alert(\"%s\");</script>",string_en);\
		else sprintf(temp,"<SCRIPT LANGUAGE=\"javascript\">alert(\"%s\");</script>",string_ru);\
		PSOCK_SEND_STR(&s->sout,temp); temp[0]=0;


static void addhead(void){
	str[0]=0;
    addstr(
  	/*"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd\">"*/
    "<html>"
    "<head>"
    		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
    		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
    		"<!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/ui.css\">-->"
    		"<!--[if IE]>"
    		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/styleIE.css\">"
    		"<![endif]-->"
    		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/alarms.css\">"
    		"<script type=\"text/javascript\" src=\"/js/alarms.js\"></script>"
	"</head>");
}

static void add_lang(){
	if(get_interface_lang() == RUS){
		addstr("'ru'");
	}
	else{
		addstr("'en'");
	}
}

static const struct httpd_cgi_call *calls[] = {
	&cgi_main,
	&cgi_header_link,
	&cgi_linkpoe_json,
	&cgi_header_name,
	&cgi_index,
	&cgi_ntwk_settings,
	&cgi_admin_settings,
	&cgi_clear,
	&cgi_update,
	&cgi_rebootAll,
	&cgi_reboot,
	&cgi_FEGE_set,
	&cgi_FEGE_stat,
	&cgi_VCT,
	&cgi_Descr,
	&cgi_FactoryDefaults,
	&cgi_SaveSettings,
	&cgi_ComfortStart,
	&cgi_AutoRestart,
	&cgi_Ping,
	&cgi_VLAN_perport,
	&cgi_VLAN_8021q,
	&cgi_VLAN_trunk,
	&cgi_Port_stat,
	&cgi_ARP,
	&cgi_MAC,
	&cgi_RSTP_stat,
	&cgi_RSTP,
	&cgi_PoE_stat,
	&cgi_syslog_set,
	//&cgi_service,
	&cgi_log,
	&cgi_ups,
	&cgi_QoS_rate_limit,
	&cgi_swu_rate_limit,
	&cgi_QoS_general,
	&cgi_QoS_cos,
	&cgi_QoS_tos,
	//&cgi_PortTrunk,
	&cgi_sntp_set,
	&cgi_smtp_set,
	&cgi_snmp_set,
	&cgi_eventlist,
	&cgi_access,
	//&cgi_files,
	&cgi_test_web,
	&cgi_test,
	&cgi_managment,
	&cgi_igmp,
	&cgi_igmp_groups,
	&cgi_telnet,
	&cgi_dns_stat,
	&cgi_devmode,
	&cgi_port_info,
	&cgi_mib_list,
	&cgi_plc,
	&cgi_make_em_json,
	&cgi_mac_bind,
	&cgi_mac_blocked,
	//&cgi_aggregation,
	&cgi_mirror,
	&cgi_teleport,
	&cgi_devinfo,
	&cgi_cpuinfo,
	&cgi_lldpsett,
	&cgi_lldpstat,
	&cgi_lldpinfo,
	&cgi_getInput,
	&cgi_getLink,
	&cgi_getPoePower,
	&cgi_setPoe,
	&cgi_getPoe,
	&cgi_getPortNum,
	&cgi_isUps,
	&cgi_getUpsStatus,
	&cgi_getUpsVoltage,
	&cgi_getUpsEstimated,
	&cgi_getNetIp,
	&cgi_getNetMac,
	&cgi_getNetMask,
	&cgi_getNetGate,
	&cgi_getFwVersion,
	&cgi_getDevType,
	&cgi_getDevName,
	&cgi_getDevLocation,
	&cgi_getPortMacList,
	&cgi_cableTesterStart,
	&cgi_cableTesterStatus,
	&cgi_getSerialNum,
	&cgi_getDevContact,
	&cgi_getUptime,
    NULL
};



uint16_t http_get_parameters_parse(char *par,uint16_t mx){
  uint16_t count=0;
	uint16_t i=0;
  for(;par[i]&&i<mx;i++)
  {
    if(par[i]=='=')
    {
      count++;
      par[i]=0;
    } else if(par[i]=='&')
      par[i]=0;
  }
  return count;
}

char * http_get_parameter_name(char *par,uint16_t cnt,uint16_t mx){
  uint16_t i,j;
  cnt*=2;
  for(i=0,j=0;j<mx&&i<cnt;j++)
    if(!par[j]) i++;

  return j==mx?"":par+j;
}

char * http_get_parameter_value(char *par,uint16_t cnt,uint16_t mx){
  uint16_t i,j;
  cnt*=2;
  cnt++;
  for(i=0,j=0;j<mx&&i<cnt;j++)
    if(!par[j]) i++;

  return j==mx?"":par+j;
}


void http_url_decode(char *in,char *out,uint8_t mx){
  uint8_t i,j;
  char tmp[3]={0,0,0};

  for(i=0,j=0;((j<mx)&&(in[i]));i++)
	{
  	if(in[i]=='%'){
		tmp[0]=in[++i];
		tmp[1]=in[++i];
		out[j++]=(char)strtol(tmp,0,16);
	}else
		out[j++]=in[i];
	}
}

static void convert_str(char *in_text,char *out_text,u8 len){
	memset(out_text,0,len);
	http_url_decode(in_text,out_text,strlen(in_text));
	for(uint8_t i=0;i<strlen(out_text);i++){
		if(out_text[i]=='+') out_text[i] = ' ';
		if(out_text[i]=='%') out_text[i]=' ';
	}
}



static PT_THREAD(nullfunction(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  PSOCK_SEND_STR(&s->sout,"404<br>Not Implemented");
  PSOCK_END(&s->sout);
}

httpd_cgifunction httpd_cgi(char *name){
  const struct httpd_cgi_call **f;

  /* Find the matching name in the table, return the function. */
  for(f = calls; *f != NULL; ++f) {
    if(strncmp((*f)->name, name, strlen((*f)->name)) == 0) {
      return (*f)->function;
    }
  }
  return nullfunction;
}




static PT_THREAD(run_main(struct httpd_state *s, char *ptr))
{
	//NOTE:local variables are not preserved during the calls to proto socket functins
  u8 mac[6];
 extern const uint32_t image_version[1];
 static u8 first_run=0;
 char tmp[128],text_tmp[128];
 RTC_TimeTypeDef RTC_Time;
 RTC_DateTypeDef RTC_Date;
 u8 sstart=0;
 u8 wdt=0;
 uip_ipaddr_t addr;
 u16 tmp_voltage;

 str[0]=0;



  PSOCK_BEGIN(&s->sout);
  	addstr("<html>"
    "<head>"
  		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
  		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
  		"<!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/ui.css\">-->"
  		"<!--[if IE]>"
  		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/styleIE.css\">"
  		"<![endif]-->"
  		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/alarms.css\">"
  		"<script type=\"text/javascript\">"
  	       "function open_url(url,help_en,help_ru,lang)"
  		   "{"
  				"parent.ContentFrame.location.href= url;"
  				"if(lang=='ru'){"
  					"parent.HelpFrame.location.href = help_ru;"
				"}else{"
  					"parent.HelpFrame.location.href = help_en;"
  				"}"
  		   "}"
		   /*"function open_port_info(port)"
		   "{"
				"window.open('/info/port_info.shtml?port='+port,'','width=1000,height=800,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');"
		   "}"*/
  		   "function futu_alert(text, link_m,link_h, close, className) {"
    			"if (!document.getElementById('futu_alerts_holder')) {"
    				"if (document.getElementById('alerts_holder')) {"
    					"var futuAlertOuter = document.createElement('div');"
    					"futuAlertOuter.className = 'futu_alert_outer';"
    					"document.getElementById('alerts_holder').appendChild(futuAlertOuter);"
    				"}else{"
    					"var futuAlertOuter = document.createElement('div');"
    					"futuAlertOuter.className = 'futu_alert_outer';"
    					"document.body.appendChild(futuAlertOuter);"
    				"}"
    				"var futuAlertFrame = document.createElement('div');"
    				"futuAlertFrame.className = 'frame';"
    				"futuAlertOuter.appendChild(futuAlertFrame);"
    				"var futuAlertsHolder = document.createElement('div');"
    				"futuAlertsHolder.id = 'futu_alerts_holder';"
    				"futuAlertsHolder.className = 'futu_alerts_holder';"
    				"futuAlertFrame.appendChild(futuAlertsHolder);"
    			"}");
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

    			addstr("var futuAlert = document.createElement('div');"
    			"futuAlert.className = 'futu_alert ' + className;"
    			"document.getElementById('futu_alerts_holder').appendChild(futuAlert);"
    			"futuAlert.id = 'futu_alert';"
    			"if (close) {"
    				"var futuAlertCloseButton = document.createElement('a');"
    				"futuAlertCloseButton.href = '#';"
    				"futuAlertCloseButton.className = 'futu_alert_close_button';"
    				"futuAlertCloseButton.onclick = function(ev) {"
    					"if(!ev) {"
    						"ev=window.event;"
    					"}"
    					"if (!document.all) ev.preventDefault(); else ev.returnValue = false;"
    					"document.getElementById('futu_alerts_holder').removeChild(futuAlert);"
    				"};"
    				"futuAlert.appendChild(futuAlertCloseButton);"

    				"var futuAlertCloseButtonIcon = document.createElement('img');"
    				"futuAlertCloseButtonIcon.src = '/img/btn_close.gif';"
    				"futuAlertCloseButton.appendChild(futuAlertCloseButtonIcon);"
    			"}");
    			PSOCK_SEND_STR(&s->sout,str);
    			str[0]=0;
    			addstr("var futuAlertText = document.createElement('div');"
    			"futuAlertText.className = 'futu_alert_text';"
    			"futuAlert.appendChild(futuAlertText);"
    			"futuAlertText.innerHTML = text;"
    			"if(link_m && link_h){"
    				"var futuAlertBtn = document.createElement('div');"
    				"futuAlertBtn.className = 'futu_alert_btn';"
    				"futuAlert.appendChild(futuAlertBtn);"
    				"futuAlertBtn.innerHTML = \"<input type='button' onclick='open_url('+link_m'+,'+link_h+');' value='Set'>\";"
    			"}"
    			"futuAlert.style.position = 'relative';"
    			"futuAlert.style.top = '0';"
    			"futuAlert.style.display = 'block';"
    			"if (!close) {"
    				"setTimeout(function () { document.getElementById('futu_alerts_holder').removeChild(futuAlert); }, 3000);"
    			"}"
    		"}");
    		PSOCK_SEND_STR(&s->sout,str);
    		str[0]=0;
    		addstr("function menu_first_run_refresh(){");
			  if(get_dev_type() == DEV_SWU16){
				  if(get_interface_lang() == ENG){
					  if(get_current_user_rule()==ADMIN_RULE)
						  addstr("parent.LeftFrame.location.href =\"tree_admin_swu_en.html\";");
					  else
						  addstr("parent.LeftFrame.location.href =\"tree_user_swu_en.html\";");
				  }
				  else{
					  if(get_current_user_rule()==ADMIN_RULE)
						  addstr("parent.LeftFrame.location.href =\"tree_admin_swu_rus.html\";");
					  else
						  addstr("parent.LeftFrame.location.href =\"tree_user_swu_rus.html\";");
				  }
			  }
			  else{
				  if(get_interface_lang() == ENG){
					  if(get_current_user_rule()==ADMIN_RULE)
						  addstr("parent.LeftFrame.location.href =\"tree_admin_en.html\";");
					  else
						  addstr("parent.LeftFrame.location.href =\"tree_user_en.html\";");
				  }
				  else{
					  if(get_current_user_rule()==ADMIN_RULE)
						  addstr("parent.LeftFrame.location.href =\"tree_admin_rus.html\";");
					  else
						  addstr("parent.LeftFrame.location.href =\"tree_user_rus.html\";");
				  }
			  }
			  addstr("location.reload();}"
    	  //под SNTP
		  "var TimeoutID; "
		  "var Time; "
		  "function form2(v) {return(v<10?'0'+v:v);} "
		  "function showtime(){"
			"sec0=Time%60;"
			"min0=(Time%3600)/60|0;"
				"hour0=Time/3600|0;"
			  "document.getElementById('clock1').innerHTML = "
			  "form2(hour0)+':'+form2(min0)+':'+form2(sec0);"
			  "Time++;"
			  "window.setTimeout('showtime();',1000);"
		  "}"
		   "function inittime (hour,min,sec) { "
			"Time=hour*3600+min*60+sec;"
			"TimeoutID=window.setTimeout('showtime()', 1000);"
		  "}");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  //под operation time
		  addstr(
		  "var TimeoutID_;"
		  "var Time_;"
		  "function form_ (v) {return(v<10?'0'+v:v);} "
		  "function showtime_(){"
			"sec2=Time_%60;"
			"min2=(Time_%3600)/60|0;"
			"hour2=Time_/3600|0;"
			"document.getElementById('clock2').innerHTML = "
			"form_(hour2)+'h. '+form2(min2)+'m. '+form2(sec2)+'s.';"
			"Time_++;"
			"window.setTimeout('showtime_();',1000);"
		  "} "
		   "function inittime_(hour,min,sec){ "
			"Time_=hour*3600+min*60+sec;"
			"TimeoutID_=window.setTimeout('showtime_();', 1000);"
		  "} "
		  "</script>");

		  addstr("</head><body>");

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

	  addstrl("<legend>Main</legend>","<legend>Главная</legend>");
	  addstr(
			"<form method=\"post\" bgcolor=\"#808080\""
			"action=\"/main.shtml\">");

		if(first_run==0){
			addstr("<script type=\"text/javascript\">"
			"menu_first_run_refresh();"
			"</script>");
			first_run=1;
		}


	  //блок сообщений
	  addstr("<div id=\"alerts_holder\"></div><br>");

	  addstr("<table class = \"main_table\">"
		  "<tr>");


	  //если ненулевая строка то выводим имя устройства
	  get_interface_name(tmp);
	  tmp[127]=0;
	  if(strlen(tmp)){
		  memset(text_tmp,0,sizeof(text_tmp));
		  http_url_decode(tmp,text_tmp,strlen(tmp));
		  for(uint8_t i=0;i<strlen(text_tmp);i++){
		  	if(text_tmp[i]=='+') text_tmp[i] = ' ';
		  	if(text_tmp[i]=='%') text_tmp[i]=' ';
		  }
		  addstr("<td class=\"l_column\">");
		  addstrl("Device name", "Имя устройства");
		  addstr("</td><td class=\"r_column\">");
		  addstr(text_tmp);
		  addstr("</td>");
		  if(get_current_user_rule()==ADMIN_RULE){
		  	  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url('"
		  			  "/info/description.shtml',"
		  			  "'/help_en/description_help.html',"
		  			  "'/help_ru/description_help.html',");
		  	  add_lang();
		  	  addstr(")\"></td>");
		  }
		  addstr("</tr>");
	  }

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

	  get_interface_location(tmp);
	  tmp[127]=0;
	  if(strlen(tmp)){
		  memset(text_tmp,0,sizeof(text_tmp));
		  http_url_decode(tmp,text_tmp,strlen(tmp));
		  for(uint8_t i=0;i<strlen(text_tmp);i++){
		  	if(text_tmp[i]=='+') text_tmp[i] = ' ';
		  	if(text_tmp[i]=='%') text_tmp[i]=' ';
		  }
		  addstr("<td class=\"l_column\">");
		  addstrl("Device location","Местоположение устройства");
		  addstr("</td><td class=\"r_column\">");
		  addstr(text_tmp);
		  addstr("</td>");
		  if(get_current_user_rule()==ADMIN_RULE){
			  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" "
					  "onclick=\"open_url('/info/description.shtml',"
					  "'/help_en/description_help.html',"
					  "'/help_ru/description_help.html',");
			  add_lang();
			  addstr("\"></td>");
		  }
		  addstr("</tr>");
	  }

	  get_interface_contact(tmp);
	  tmp[127]=0;
	  if(strlen(tmp)){
		  memset(text_tmp,0,sizeof(text_tmp));
		  http_url_decode(tmp,text_tmp,strlen(tmp));
		  for(uint8_t i=0;i<strlen(text_tmp);i++){
		  	if(text_tmp[i]=='+') text_tmp[i] = ' ';
		  	if(text_tmp[i]=='%') text_tmp[i]=' ';
		  }
		  addstr("<td class=\"l_column\">");
		  addstrl("Contact information","Контактная информация");
		  addstr("</td><td class=\"r_column\">");
		  addstr(text_tmp);
		  addstr("</td>");
		  if(get_current_user_rule()==ADMIN_RULE){
				  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						  "'/info/description.shtml',"
						  "'/help_en/description_help.html',"
						  "'/help_ru/description_help.html',");
				  add_lang();
				  addstr(")\"></td>");
		  }
		  addstr("</tr>");
	  }
	  addstr("<tr><td>&nbsp;</td></tr>");
	  addstr("<td class=\"l_column\">");
	  addstrl("Serial number", "Серийный номер");
	  addstr("</td><td class=\"r_column\">");
	  get_net_def_mac(mac);
      sprintf(temp,"%d",(int)(mac[4]<<8 | (uint8_t)mac[5]));
	  addstr(temp);

	  addstr("</td>");
	  if(get_current_user_rule()==ADMIN_RULE){
		  addstr("<td class=\"a_column\">&nbsp;</td>");
	  }
	  addstr("</tr>"
		"<tr><td class=\"l_column\">");
	  addstrl("Firmware version","Версия прошивки");
	  addstr("</td><td class=\"r_column\">");
	  sprintf(temp,"%02X.%02X.%02X",
		 (int)(image_version[0]>>16)&0xff,
		 (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
	  addstr(temp);
#ifdef date_of_fv
	  addstr("<br>");
	  addstr(date_of_fv);
#endif

	  addstr("</td>");
	  if(get_current_user_rule()==ADMIN_RULE){
			  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Update\" onclick=\"open_url("
					  "'/mngt/update.shtml',"
					  "'/help_en/update_help.html',"
					  "'/help_ru/update_help.html',");
			  add_lang();
			  addstr( ")\"></td>");
	  }
	  addstr("</tr>");

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

	  addstr("<tr>"
	  "<td class=\"l_column\">");
	  addstrl("Bootloader version","Версия загрузчика");
	  addstr("</td><td class=\"r_column\">");
	  sprintf(temp,"%02X.%02X",(int)(bootloader_version>>8)&0xff,  (int)(bootloader_version)&0xff);
	  addstr(temp);
	  addstr("</td>");
	  if(get_current_user_rule()==ADMIN_RULE){
			 addstr("<td class=\"a_column\">&nbsp;</td>");
	  }
	  addstr("</tr>");

	  addstr("<tr>"
	  "<td class=\"l_column\">MAC</td>"
	  "<td class=\"r_column\">");
	  sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
			(int)dev_addr[0],(int)dev_addr[1],(int)dev_addr[2],
			(int)dev_addr[3],(int)dev_addr[4],(int)dev_addr[5]);
	  addstr(temp);
      addstr("</td>");
      if(get_current_user_rule()==ADMIN_RULE){
    		  addstr("<td class=\"a_column\">&nbsp;</td>");
      }
      addstr("</tr>");

      addstr("<tr><td class=\"l_column\">IP</td>"
      "<td class=\"r_column\">");
      sprintf(temp,"%d.%d.%d.%d",(u8)uip_hostaddr[0],(u8)(uip_hostaddr[0]>>8),(u8)uip_hostaddr[1],(u8)(uip_hostaddr[1]>>8));
	  addstr(temp);
	  addstr("</td>");
	  if(get_current_user_rule()==ADMIN_RULE){
			  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
					  "'/settings/settings_ntwk.shtml',"
					  "'/help_en/settings_ntwk_help.html',"
					  "'/help_ru/settings_ntwk_help.html',");
			  add_lang();
			  addstr(")\"></td>");
	  }
	  addstr("</tr>");

	  addstr("<tr><td class=\"l_column\">");
	  addstrl("Mask","Маска подсети");
	  addstr("</td><td class=\"r_column\">");
	  sprintf(temp,"%d.%d.%d.%d",(u8)uip_netmask[0],(u8)(uip_netmask[0]>>8), (u8)uip_netmask[1], (u8)(uip_netmask[1]>>8));
	  addstr(temp);
	  addstr("</td>");
	  if(get_current_user_rule()==ADMIN_RULE){
			  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
					  "'/settings/settings_ntwk.shtml',"
					  "'/help_en/settings_ntwk_help.html',"
					  "'/help_ru/settings_ntwk_help.html',");
			  add_lang();
			  addstr(")\"></td>");
	  }
	  addstr("</tr>");

	  if(((int)uip_ipaddr1(&uip_draddr)!=255)&&((int)uip_ipaddr2(&uip_draddr)!=255)&&(uip_ipaddr3(&uip_draddr)!=255)
			&&(uip_ipaddr4(&uip_draddr)!=255)){
		  addstr("<tr><td class=\"l_column\">");
		  addstrl("Gateway","Шлюз");
		  addstr("</td>");
		  addstr("</td><td class=\"r_column\">");
		  uip_getdraddr(&addr);
		  sprintf(temp,"%d.%d.%d.%d",(int)uip_ipaddr1(&uip_draddr),(int)uip_ipaddr2(&uip_draddr), uip_ipaddr3(&uip_draddr), uip_ipaddr4(&uip_draddr));
		  addstr(temp);
		  addstr("</td>");
		  if(get_current_user_rule()==ADMIN_RULE){
				  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						  "'/settings/settings_ntwk.shtml',"
						  "'/help_en/settings_ntwk_help.html',"
						  "'/help_ru/settings_ntwk_help.html',");
				  add_lang();
				  addstr(")\"></td>");
		  }
		  addstr("</tr>");
	  }

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

	  if(((int)uip_ipaddr1(&uip_dns_addr)!=255)&&((int)uip_ipaddr2(&uip_dns_addr)!=255)&&(uip_ipaddr3(&uip_dns_addr)!=255)
			&&(uip_ipaddr4(&uip_dns_addr)!=255)){
		  addstr("<tr><td class=\"l_column\">");
		  addstrl("DNS","DNS");
		  addstr("</td>");
		  addstr("</td><td class=\"r_column\">");
		  //uip_getdraddr(&addr);
		  sprintf(temp,"%d.%d.%d.%d",(int)uip_ipaddr1(&uip_dns_addr),(int)uip_ipaddr2(&uip_dns_addr), uip_ipaddr3(&uip_dns_addr), uip_ipaddr4(&uip_dns_addr));
		  addstr(temp);
		  addstr("</td>");
		  if(get_current_user_rule()==ADMIN_RULE){
				  addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url('"
						  "'/settings/settings_ntwk.shtml',"
						  "'/help_en/settings_ntwk_help.html',"
						  "'/help_ru/settings_ntwk_help.html',");
				  add_lang();
				  addstr(")\"></td>");
		  }
		  addstr("</tr>");
	  }

	  if(get_dhcp_mode()){
		     addstr("<tr><td class=\"l_column\">"
	  	 			 "DHCP</td><td class=\"r_column\">");
	  	 	 switch(get_dhcp_mode()){
	  	 	 	case 0:addstrl("Disabled","Нет");break;
	  	 	 	case 1:addstrl("Client","Клиент");break;
	  	 	 	case 2:addstrl("Relay","Ретранслятор");break;
	  	 	 	case 3:addstrl("Server","Сервер");break;
	  	 	 }
	  	 	 addstr("</td>");
	  	 	if(get_current_user_rule()==ADMIN_RULE){
	  	 		addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url('"
						  "'/settings/settings_ntwk.shtml',"
						  "'/help_en/settings_ntwk_help.html',"
						  "'/help_ru/settings_ntwk_help.html',");
				  add_lang();
				  addstr(")\"></td>");
	  	 	}
	  	 	addstr("</tr>");
	  }


	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;


	  if(get_telnet_state()){
		     addstr("<tr><td class=\"l_column\">"
	  	 			 "Telnet</td><td class=\"r_column\">");
	  	 	 addstrl("Enabled","Включено");
	  	  	 addstr("</td>");
	  	 	if(get_current_user_rule()==ADMIN_RULE){
	  	 		addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\""
	  	 				"open_url('/settings/telnet.shtml',"
	  	 				"'/help_en/telnet_help.html',"
	  	 				"'/help_ru/telnet_help.html',");
	  	 		add_lang();
	  	 		addstr(	")\"></td>");
	  	 	}
	  	 	addstr("</tr>");
	  }


	  addstr("<tr><td class=\"l_column\">");
	  addstrl("System Up Time","Время работы");
	  addstr("</td><td class=\"r_column\">");
	  RTC_GetTime(RTC_Format_BIN,&RTC_Time);
	  RTC_GetDate(RTC_Format_BCD,&RTC_Date);
	  sprintf(temp," %dd. ",(RTC_Date.RTC_Date));
	  addstr(temp);
	  addstr("<span id=\"clock2\"></span>&nbsp;"
	  "<script type=\"text/javascript\">"
	  "inittime_ (");
	  sprintf(temp,"%d,%d,%d",(RTC_Time.RTC_Hours),(RTC_Time.RTC_Minutes),(RTC_Time.RTC_Seconds));
	  addstr(temp);
	  addstr(");</script>");
	  addstr("</td>");
	  if(get_current_user_rule()==ADMIN_RULE){
		  addstr("<td class=\"a_column\">&nbsp;</td>");
	  }
	  addstr("</tr>");

	  if((get_sntp_state()==ENABLE)&&(date.year!=0)){
		   	addstr("<tr><td class=\"l_column\">");
			addstrl("Time","Время");
			addstr("</td><td class=\"r_column\">");
			addstr("<span id=\"clock1\"></span>&nbsp;"
			"<script type=\"text/javascript\">"
			"inittime (");
			sprintf(temp,"%d,%d,%d",date.hour,date.min,date.sec);
			addstr(temp);
			addstr(");</script>");
			addstr(
			"</td>");
			if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\">&nbsp;</td>");
			}
			addstr("</tr>"
			"<tr><td class=\"l_column\">");
			addstrl("Date","Дата");
			addstr("</td><td class=\"r_column\">");
			sprintf(temp,"%02d/%02d/%d",date.day,date.month,(date.year+2000));
			addstr(temp);
			addstr("</td>");
			if(get_current_user_rule()==ADMIN_RULE){
					addstr("<td class=\"a_column\">&nbsp;</td>");
			}
			addstr("</tr>");
			temp[0]=0;
	 }


	 PSOCK_SEND_STR(&s->sout,str);
	 str[0]=0;

	 if(is_ups_mode() == ENABLE){
		 addstr("<tr><td>&nbsp;</td></tr>");
		 addstr("<tr><td class=\"l_column\">");
		 addstrl("UPS","ИБП");
		 addstr("</td><td class=\"r_column\">Enable</td><td class=\"a_column\">&nbsp;</td></tr>");

		 addstr("<tr><td class=\"l_column\">");
		 addstrl("Battery voltage","Напряжение на АКБ");
		 addstr("</td><td class=\"r_column\">");
		 if(is_akb_detect()==1){
			 tmp_voltage = get_akb_voltage();
		 	 if(tmp_voltage<360){
				addstr("&nbsp;");
			 }
			 else{
				sprintf(temp,"%d.%dV",tmp_voltage/10,tmp_voltage%10);
				addstr(temp);
			 }
		}else{
			addstrl("Battery not connected","АКБ не подключена");
		}
		addstr("</td>");
		if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\">&nbsp;</td>");
		addstr("</tr>");

		addstr("<tr><td class=\"l_column\">");
		addstrl("Power source","Питание");
		addstr("</td><td class=\"r_column\">");
		if(is_ups_rezerv()==0){
			addstrl("VAC","Сетевое напряжение");
		}
		else{
			addstrl("Battery","АКБ");
		}
		addstr("</td>");
		if(get_current_user_rule()==ADMIN_RULE){
			addstr("<td class=\"a_column\">&nbsp;</td>");
		}
		addstr("</tr>");
		}
		addstr("<tr><td class=\"l_column\">");
		addstrl("Estimated battery time","Оценочное время работы на АКБ");
		addstr("</td><td class=\"r_column\">");
		if(remtime.valid){
			sprintf(temp,"%dh.%dm",remtime.hour,remtime.min);
			addstr(temp);
		}
		else{
			addstr("&nbsp;");
		}
		addstr("</td>");
		if(get_current_user_rule()==ADMIN_RULE){
			addstr("<td class=\"a_column\">&nbsp;</td>");
		}
		addstr("</tr>");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
    }//end if UPS



		//состояние протоколов
	 	 addstr("<tr><td>&nbsp;</td></tr>");

	 	 if(get_stp_state()){
			 addstr("<tr><td class=\"l_column\">STP/RSTP</td>"
					 "<td class=\"r_column\">");
			 if(get_stp_state()==1){
				 if(get_stp_proto()==2)
					 addstr("RSTP");
				 else
					 addstr("STP");
			 }
			 else{
				 addstrl("Disabled","Нет");
			 }
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
					 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
							 "'/STP/RSTP.shtml',"
							 "'/help_en/RSTP_help.html',"
							 "'/help_ru/RSTP_help.html',");
					 add_lang();
					 addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	 }

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

	 	if(!((get_vlan_sett_vlannum()==1)&&(get_vlan_sett_mngt()==1))){
			 addstr("<tr><td class=\"l_column\">VLAN</td>"
				"<td class=\"r_column\">");
			 if(get_vlan_sett_state()==1){
				 for(u8 i=0;i<get_vlan_sett_vlannum();i++){
					 sprintf(temp,"VLAN%d",get_vlan_vid(i));
					 addstr(temp);
					 if(i!=(get_vlan_sett_vlannum()-1))
						 addstr(", ");
				 }
			 }
			 else if(get_pb_vlan_state()==1){
				 addstr("Port Base VLAN");
			 }else
				 addstrl("Disabled","Нет");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
					addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
							"'/vlan/VLAN_8021q.shtml',"
							"'/help_en/VLAN_8021q.html',"
							"'/help_ru/VLAN_8021q.html',");
					add_lang();
					addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	}

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

	 	if(get_qos_state()){
			 addstr("<tr><td class=\"l_column\">QoS"
					 "</td><td class=\"r_column\">");
			 if(get_qos_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/qos/QoS_general.shtml',"
						"'/help_en/QoS_general_help.html',"
						"'/help_ru/QoS_general_help.html',");
				add_lang();
				addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	}

	 	if(get_igmp_snooping_state()){
			 addstr("<tr><td class=\"l_column\">IGMP Snooping"
					 "</td><td class=\"r_column\">");
			 if(get_igmp_snooping_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
					 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
							 "'/igmp/igmp.shtml',"
							 "'/help_en/QoS_general_help.html',"
							 "'/help_ru/QoS_general_help.html',");
					 add_lang();
					 addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	}




		 sstart = 0;
	 	 for(u8 k=0;k<COOPER_PORT_NUM;k++)
	 		if(get_port_sett_soft_start(k))
	 			sstart=1;

	 	 if(sstart){
			 addstr("<tr><td class=\"l_column\">");
			 addstrl("Comfort Start","Плавный старт");
			 addstr("</td><td class=\"r_column\">");
			 for(u8 i=0;i<COOPER_PORT_NUM;i++)
				 if(get_port_sett_soft_start(i)==1){
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1 ");break;
							case 1:addstr("FE#2 ");break;
							case 2:addstr("FE#3 ");break;
							case 3:addstr("FE#4 ");break;
							case 4:addstr("GE#1 ");break;
							case 5:addstr("GE#2 ");break;

						}
					}else{
						sprintf(temp,"%d ",i+1);
						addstr(temp);
					}
				 }
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
					addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
							"'/settings/ComfortStart.shtml',"
							"'/help_en/ComfortStart_help.html',"
							"'/help_ru/ComfortStart_help.html',");
					add_lang();
					addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	 }


	 	 wdt = 0;
		 for(u8 k=0;k<COOPER_PORT_NUM;k++){
			 if(get_port_sett_wdt(k))
				 wdt=1;
		 }

	 	 if(wdt){
			 addstr("<tr><td class=\"l_column\">");
			 addstrl("Auto Restart","Защита от зависания");
			 addstr("</td><td class=\"r_column\">");

			 for(u8 i=0;i<COOPER_PORT_NUM;i++){
				 if(get_port_sett_wdt(i)){
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1 ");break;
							case 1:addstr("FE#2 ");break;
							case 2:addstr("FE#3 ");break;
							case 3:addstr("FE#4 ");break;
							case 4:addstr("GE#1 ");break;
							case 5:addstr("GE#2 ");break;

						}
					}else{
						sprintf(temp,"%d ",i+1);
						addstr(temp);
					}
				 }
			 }
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						 "'/settings/AutoRestart.shtml',"
						 "'/help_en/AutoRestart_help.html',"
						 "'/help_ru/AutoRestart_help.html',");
				 add_lang();
				 addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	 }

	 	 if(get_syslog_state()){
			 addstr("<tr><td class=\"l_column\">SYSLOG");
			 addstr("</td><td class=\"r_column\">");
			 if(get_syslog_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						 "'/mngt/syslog.shtml',"
						 "'/help_en/syslog_help.html',"
						 "'/help_ru/syslog_help.html',");
				 add_lang();
				 addstr( ")\"></td>");
			 }
			 addstr("</tr>");
	 	 }

		 PSOCK_SEND_STR(&s->sout,str);
		 str[0]=0;
		 if(get_sntp_state()){
			 addstr("<tr><td class=\"l_column\">SNTP");
			 addstr("</td><td class=\"r_column\">");
			 if(get_sntp_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						 "'/mngt/sntp.shtml',"
						 "'/help_en/sntp_help.html',"
						 "'/help_ru/sntp_help.html',");
				 add_lang();
				 addstr(")\"></td>");
			 }
			 addstr("</tr>");
		 }

		 if(get_snmp_state()){
			 addstr("<tr><td class=\"l_column\">SNMP");
			 addstr("</td><td class=\"r_column\">");
			 if(get_snmp_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						 "'/mngt/snmp.shtml',"
						 "'/help_en/snmp_help.html',"
						 "'/help_ru/snmp_help.html',");
				 add_lang();
				 addstr( ")\"></td>");
			 }
			 addstr("</tr>");
		 }
		 if(get_smtp_state()){
			 addstr("<tr><td class=\"l_column\">SMTP");
			 addstr("</td><td class=\"r_column\">");
			 if(get_smtp_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/mngt/smtp.shtml',"
						"'/help_en/smtp_help.html',"
						"'/help_ru/smtp_help.html',");
				add_lang();
				addstr( ")\"></td>");
			 }
			 addstr("</tr>");
		 }
		 if(get_lldp_state()){
			 addstr("<tr><td class=\"l_column\">LLDP");
			 addstr("</td><td class=\"r_column\">");
			 if(get_lldp_state()==1)
				 addstrl("Enabled","Включено");
			 else
				addstrl("Disabled","Выключено");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						 "'/lldp/lldpsett.shtml',"
						 "'/help_en/lldpsett_help.html',"
						 "'/help_ru/lldpsett_help.html',");
				 add_lang();
				 addstr( ")\"></td>");
			 }
			 addstr("</tr>");
		 }

		 //mac filtering
	 	 wdt = 0;
		 for(u8 k=0;k<ALL_PORT_NUM;k++){
			 if(get_mac_filter_state(k)!=PORT_NORMAL)
				 wdt=1;
		 }
		 if(wdt){
			 addstr("<tr><td class=\"l_column\">");
			 addstrl("MAC Filtering","Фильтрация по MAC адресам");
			 addstr("</td><td class=\"r_column\">");
			 for(u8 i=0;i<ALL_PORT_NUM;i++){
				 if(get_mac_filter_state(i) != PORT_NORMAL){
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1 ");break;
							case 1:addstr("FE#2 ");break;
							case 2:addstr("FE#3 ");break;
							case 3:addstr("FE#4 ");break;
							case 4:addstr("GE#1 ");break;
							case 5:addstr("GE#2 ");break;

						}
					}else{
						sprintf(temp,"%d ",i+1);
						addstr(temp);
					}
				 }
			 }
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/settings/mac_bind.shtml',"
						"'/help_en/mac_bind_help.html',"
						"'/help_ru/mac_bind_help.html',");
				add_lang();
				addstr(	")\"></td>");
			 }
			 addstr("</tr>");
		 }

		 if(get_lag_entries_num() && get_dev_type()==DEV_SWU16){
			 addstr("<tr><td class=\"l_column\">");
			 addstrl("Link Aggregation","Агрегация портов");
			 addstr("</td><td class=\"r_column\">");
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/settings/aggregation.shtml',"
						"'/help_en/aggregation_help.html',"
						"'/help_ru/aggregation_help.html',");
				add_lang();
				addstr(	")\"></td>");
			 }
			 addstr("</tr>");
		 }

		 if(get_mirror_state() && get_dev_type()==DEV_SWU16){
			 addstr("<tr><td class=\"l_column\">");
			 addstrl("Port Mirroring","Зеркалирование портов");
			 addstr("</td>");
			 addstr("<td class=\"r_column\">");
			 for(u8 i=0;i<ALL_PORT_NUM;i++){
				 if(get_mirror_port(i)){
					 sprintf(temp,"%d ",i+1);
					 addstr(temp);
				 }
			 }
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/settings/mirror.shtml',"
						"'/help_en/mirror_help.html',"
						"'/help_ru/mirror_help.html',");
				add_lang();
				addstr(")\"></td>");
			 }
			 addstr("</tr>");
		 }


		 if(is_plc_connected()){
			 addstr("<tr><td class=\"l_column\">");
			 addstrl("Option board","Плата расширения");
			 addstr("</td><td class=\"r_column\">");
			 switch(get_plc_hw_vers()){
			 	 case PLC_01:
			 		 addstr("PLC-01");
			 		 break;
			 	 case PLC_02:
			 		 addstr("PLC-02");
			 		 break;
			 	 default:
			 		 addstr("-");
			 }
			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/mngt/access.shtml',"
						"'/help_en/access_help.html',"
						"'/help_ru/access_help.html',");
				add_lang();
				addstr(")\"></td>");
			 }
			 addstr("</tr>");
			 if((get_plc_hw_vers() == PLC_01 || get_plc_hw_vers() == PLC_02)&&(get_plc_em_model()!=0)){
				 addstr("<tr><td class=\"l_column\">");
				 addstrl("Energy Meter","Счётчик электроэнергии");
				 addstr("</td><td class=\"r_column\">");
				 get_plc_em_model_name(temp);
				 addstr(temp);
				 addstr("</td>");
				 if(get_current_user_rule()==ADMIN_RULE){
					 addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
							 "'/plc.shtml',"
							 "'/help_en/plc_help.html',"
							 "'/help_ru/plc_help.html',");
					 add_lang();
					 addstr(")\"></td>");
				 }
			 }
		 }

		 //teleport
		 if(get_remdev_num()){
			 addstr("<tr><td class=\"l_column\">");
			 addstr("Teleport");
			 addstr("</td><td class=\"r_column\">");

			 addstr("</td>");
			 if(get_current_user_rule()==ADMIN_RULE){
				addstr("<td class=\"a_column\"><input type=\"button\" value =\"Edit\" onclick=\"open_url("
						"'/settings/teleport.shtml',"
						"'/help_en/teleport_help.html',"
						"'/help_ru/teleport_help.html',");
				add_lang();
				addstr( ")\"></td>");
			 }
			 addstr("</tr>");
		 }

	 	 addstr("</table>"
	 			 "<br><br><input type=\"Submit\" value=\"Refresh\">");


	 	   /**************    предупреждения       *************************************/
		//если алармы по портам или по сухим контактам
		if(/*(ALL_PORT_ERROR)||(get_all_alarms_error())*/1){
			addstr("<script type=\"text/javascript\">");
			for(uint8_t i=0;i<(COOPER_PORT_NUM);i++){
				switch(get_port_error(i)){
					case 1:
						addstr("futu_alert(\"");
						addstrl("Warning! No link on port ","Внимание! Нет линка на порту ");
						if(get_dev_type()==DEV_PSW2GPLUS)
							addstr("FE");
						sprintf(temp,"%d",(int)(i+1));
						addstr(temp);
						addstr("\",false,false,true,'error');");
						break;

					case 2:
						addstr("futu_alert(\"");
						addstrl("Warning! No response to PING on port ","Внимание! Нет ответа на PING на порту ");
						if(get_dev_type()==DEV_PSW2GPLUS)
							addstr("FE");
						sprintf(temp,"%d",(int)(i+1));
						addstr(temp);
						addstr("\",false,false,true,'error');");
						break;

					case 3:
						addstr("futu_alert(\"");
						addstrl("Warning! Low Speed on port ","Внимание! Низкая скорость на порту ");
						if(get_dev_type()==DEV_PSW2GPLUS)
							addstr("FE");
						sprintf(temp,"%d",(int)(i+1));
						addstr(temp);
						addstr("\",false,false,true,'error');");
						break;
					case 4:
						addstr("futu_alert(\"");
						addstrl("Warning! High Speed on port ","Внимание! Высокая скорость на порту ");
						if(get_dev_type()==DEV_PSW2GPLUS)
							addstr("FE");
						sprintf(temp,"%d",(int)(i+1));
						addstr(temp);
						addstr("\",false,false,true,'error');");
						break;

				}
			}
			if(dev.alarm.dry_cont[0] == 1){
				addstr("futu_alert(\"");
				addstrl("Warning! Tamper is active", "Внимание! Датчик вскрытия активен");
				addstr("\",false,false,true,'error');");
			}
			if(dev.alarm.dry_cont[1] == 1){
				addstr("futu_alert(\"");
				addstrl("Warning! Dry contact 1 is active", "Внимание! Линия сухих контактов 1 активна");
				addstr("\",false,false,true,'error');");
			}
			if(dev.alarm.dry_cont[2] == 1){
				addstr("futu_alert(\"");
				addstrl("Warning! Dry contact 2 is active", "Внимание! Линия сухих контактов 2 активна");
				addstr("\",false,false,true,'error');");
			}


			if((dev.alarm.nopass == 1)&&(get_current_user_rule()==ADMIN_RULE)){
				addstr("futu_alert(\"");
				addstrl("Please, set admin password ",
						"Пожалуйста, установите пароль администратора ");
				addstr("\",false,false,true,'error');");
			}

			get_interface_name(tmp);
			if(!strlen(tmp)&&(get_current_user_rule()==ADMIN_RULE)){
				addstr("futu_alert(\"");
				addstrl("Device Name is not set","Имя устройства не заданно");
				addstr("\",false,false,true,'warning');");
			}

			get_interface_location(tmp);
			if(!strlen(tmp)&&(get_current_user_rule()==ADMIN_RULE)){
				addstr("futu_alert(\"");
				addstrl("Device Location is not set","Месторасположение устройства не заданно");
				addstr("\",false,false,true,'warning');");
			}
			addstr("</script>");
	  }

	  addstr("</body></html>");

  PSOCK_SEND_STR(&s->sout,str);
  str[0]=0;
  PSOCK_END(&s->sout);

}


static PT_THREAD(run_header_link(struct httpd_state *s, char *ptr)){
static u8 pcount;
	str[0]=0;
	RTC_TimeTypeDef RTC_Time;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		for(int i=0;i<pcount;i++){
		     char *pname,*pval;
		     pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		     pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			 if(!strcmp(pname,"Logout")){
				 if (!strncmp(pval, "L", 1)){
					  http_logout();
				 }
			}
		}
	}

	PSOCK_SEND_STR(&s->sout,
    	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd\">"
    	"<html>"
    	"<head>"
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
			"<!-- CSS -->"
			"<link rel=\"stylesheet\" type=\"text/css\" href=\"css/link.css\">"
			"<!-- JS ------------>"
			"<script type=\"text/javascript\">"
	  	       "function open_url(url,help_en,help_ru,lang)"
	  		   "{"
	  				"parent.ContentFrame.location.href= url;"
	  				"if(lang=='ru'){"
	  					"parent.HelpFrame.location.href = help_ru;"
					"}else{"
	  					"parent.HelpFrame.location.href = help_en;"
	  				"}"
	  		   "}"
				"function open_port_info(port)"
				"{"
					"window.open('/info/port_info.shtml?port='+port,'','width=1000,height=800,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');"
				"}"
			"</script>"
      	"</head>"
    	"<body background= \"img/bg.gif\">"
		"<form method=\"post\" action=\"/header_link.shtml\">");

		//main table
		addstr("<table width=\"100%\"><tr>"
		"<td>"
		"<a href=\"http://tfortis.ru\" target=\"_blank\"><IMG SRC= \"img/tfortis.gif\" ALIGN=left border=\"0\"></a>"
		"</td><td>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("<div id='header'>");

		if(get_dev_type() == DEV_SWU16){
			addstr("<table class=\"port_table\">"
			//1 строка
			"<tr><td class=\"port\">Port</td>");
			for(u8 i=0;i<(FIBER_PORT_NUM/2 + COOPER_PORT_NUM);i++){
				if(i<(FIBER_PORT_NUM/2)){
					addstr("<td id=\"port");
					sprintf(temp,"%d",i*2+2);
					addstr(temp);
					addstr("\" class=\"port\" onclick=\"open_port_info(");
					sprintf(temp,"%d",i*2+1);
					addstr(temp);
					addstr(")\">");
					sprintf(temp,"%d",i*2+2);
					addstr(temp);
					addstr("</td>");
				}
				else{
					addstr("<td></td>");
				}
			}
			addstr("</tr>"
			//2
			"<tr><td class=\"port\">Link</td>");
			for(u8 i=0;i<(FIBER_PORT_NUM/2 + COOPER_PORT_NUM);i++){
				if(i<(FIBER_PORT_NUM/2)){
					if(get_port_link(i*2+1)==1){
						addstr("<td id=\"link");
						sprintf(temp,"%d",i*2+2);
						addstr(temp);
						addstr("\" class=\"up\">Up");
						addstr("</td>");
					}
					else{
						addstr("<td id=\"link");
						sprintf(temp,"%d",i*2+2);
						addstr(temp);
						addstr("\" class=\"down\">Down");
						addstr("</td>");
					}
				}
				else{
					addstr("<td></td>");
				}
			}

			addstr("</tr>");


		    PSOCK_SEND_STR(&s->sout,str);
		    str[0]=0;

			//3
			addstr("<tr><td class=\"port\">Port</td>");
			for(u8 i=0;i<(FIBER_PORT_NUM/2);i++){
				addstr("<td id=\"port");
				sprintf(temp,"%d",i*2+1);
				addstr(temp);
				addstr("\" class=\"port\" onclick=\"open_port_info(");
				sprintf(temp,"%d",i*2);
				addstr(temp);
				addstr(")\">");
				sprintf(temp,"%d",i*2+1);
				addstr(temp);
				addstr("</td>");
			}
			for(u8 i=0;i<(COOPER_PORT_NUM);i++){
				addstr("<td id=\"port");
				sprintf(temp,"%d",FIBER_PORT_NUM+i+1);
				addstr(temp);
				addstr("\" class=\"port\" onclick=\"open_port_info(");
				sprintf(temp,"%d",FIBER_PORT_NUM+i);
				addstr(temp);
				addstr(")\">");
				sprintf(temp,"%d",FIBER_PORT_NUM+i+1);
				addstr(temp);
				addstr("</td>");
			}
			//4
			addstr("</tr>"
			"<tr><td class=\"port\">Link</td>");
			for(u8 i=0;i<(FIBER_PORT_NUM/2);i++){
				if(get_port_link(i*2)==1){
					addstr("<td id=\"link");
					sprintf(temp,"%d",i*2+1);
					addstr(temp);
					addstr("\" class=\"up\">Up");
					addstr("</td>");
				}
				else{
					addstr("<td id=\"link");
					sprintf(temp,"%d",i*2+1);
					addstr(temp);
					addstr("\" class=\"down\">Down");
					addstr("</td>");
				}
			}
			for(u8 i=0;i<(COOPER_PORT_NUM);i++){
				if(get_port_link(FIBER_PORT_NUM+i)==1){
					addstr("<td id=\"link");
					sprintf(temp,"%d",i+FIBER_PORT_NUM+1);
					addstr(temp);
					addstr("\" class=\"up\">Up");
					addstr("</td>");
				}
				else{
					addstr("<td id=\"link");
					sprintf(temp,"%d",i+FIBER_PORT_NUM+1);
					addstr(temp);
					addstr("\" class=\"down\">Down");
					addstr("</td>");
				}
			}
			addstr("</tr>");

		}
		else{
			//psw device
			addstr("<table class=\"port_table\">"
			"<tr><td class=\"port\">Port</td>");
			for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				addstr("<td id=\"port");
				sprintf(temp,"%d",i+1);
				addstr(temp);
				addstr("\" ");
				if(get_port_error(i))
					addstr("class = \"error\"");
				else
					addstr("class=\"port\"");
				addstr(" onclick=\"open_port_info(");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr(")\">");

				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:addstr("FE#1");break;
						case 1:addstr("FE#2");break;
						case 2:addstr("FE#3");break;
						case 3:addstr("FE#4");break;
						case 4:addstr("GE#1");break;
						case 5:addstr("GE#2");break;
					}
				}
				else{
					sprintf(temp,"%d",i+1);
					addstr(temp);
				}


				addstr("</td>");
			}

			addstr("</tr><tr onclick=\"open_url(\"info/FEGE_stat.shtml\",\"help/FEGE_stat_help.html\")\"><td class=\"port\">Link</td>");
			for(u8 i=0;i<(ALL_PORT_NUM);i++){
					if(get_port_link(i)==1){
						addstr("<td id=\"link");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\" class=\"up\">Up</td>");
					}
					else{
						addstr("<td id=\"link");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\" class=\"down\">Down</td>");
					}
			}

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

			RTC_GetTime(RTC_Format_BIN,&RTC_Time);
			addstr("</tr>");
			if(POE_PORT_NUM){
				addstr("<tr onclick=\"open_url("
						"'info/PoE_stat.shtml',"
						"'help_en/PoE_stat_help.html',"
						"'help_ru/PoE_stat_help.html',");
				add_lang();
				addstr(")\"><td class=\"port\">PoE</td>");
				for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
					if(i<POE_PORT_NUM){
						addstr("<td id=\"poe");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\" ");
						//для psw2g4f  и psw1G psw-2G8F+
						if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)
								||(get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS
								|| get_dev_type() == DEV_PSW2G8F)){
							if(get_port_poe_a(i)==1){
								addstr("class=\"up\">On");
							}
							else if(dev.port_stat[i].ss_process==1){
								sprintf(temp,"class=\"wait\">%d/%d",(u16)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes), (u16)(get_softstart_time()*60));
								addstr(temp);
							}
							else{
								addstr("class=\"down\">Off");
							}
						}

						//PSW 2G6F+ - 2 порта с PoE B
						if(get_dev_type() == DEV_PSW2G6F){
							if((get_port_poe_a(i)==1)||((i<POEB_PORT_NUM)&&(get_port_poe_b(i)==1))){
								addstr("class=\"up\">On");
							}
							else if(dev.port_stat[i].ss_process==1){
								sprintf(temp,"class=\"wait\">%d/%d",(u16)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes), (u16)(get_softstart_time()*60));
								addstr(temp);
							}
							else{
								addstr("class=\"down\">Off");
							}
						}

						//PSW 2G+, 2G2F+ - все порты с PoE B
						if((get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
							if((get_port_poe_a(i)==1)||(get_port_poe_b(i)==1))
								addstr("class=\"up\">On");
							else if(dev.port_stat[i].ss_process==1){
								sprintf(temp,"class=\"wait\">%d/%d",(u16)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes), (u16)(get_softstart_time()*60));
								addstr(temp);
							}
							else
								addstr("class=\"down\">Off");
						}
						addstr("</td>");
					}
					else
						addstr("<td class=\"down\">&nbsp</td>");
				}
				addstr("</tr>");
			}
		}
		addstr("</table>"
		"</div></td><td>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		//device name
		addstr("<table><tr align=\"center\">"
			"<td align=center><FONT SIZE=6 face=\"Arial\"  color=\"#003366\">"
			"<b>");
			get_dev_name_r(temp);
			addstr(temp);
			addstr("</b></FONT></td></tr>"
			"<tr><td align=\"right\" valign=\"bottom\">");
			if(http_passwd_enable){
				addstr(dev.user.current_username);
				addstr(",&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
				"<input type=\"Submit\" value=\"Logout\" name=\"Logout\"\">");
			}
			addstr(
			"<br><br>Language:&nbsp"
			"<img src= \"img/lang.gif\" ALIGN=right border=\"0\" "
			"onclick=\"open_url("
						"'mngt/managment.shtml',"
						"'help_en/managment_help.html',"
						"'help_ru/managment_help.html',");
						add_lang();
				addstr(")\">"
			"</td>"
			"</tr>"
		"</table>"
		"</td></tr></table>");


		addstr("<script>"
		"setInterval(function ajax_header(){"
			"var url='/jheader.shtml';"
			"var header = document.getElementById('header'); "
			"var http = createRequestObject();"
			"if( http ){"
				"http.open('get', url);"
				"http.onreadystatechange = function (){"
					"if(http.readyState == 4){"
						"header.innerHTML = http.responseText;"
					"}"
				"};"
				"http.send(null);"
			"}"
			"else"
			"{"
				"document.location = url;"
			"}"
		"},10000);"
		// создание ajax объекта
		"function createRequestObject()"
		"{"
			"try { return new XMLHttpRequest(); }"
			"catch(e)"
			"{"
				"try { return new ActiveXObject('Msxml2.XMLHTTP'); }  "
				"catch(e)"
				"{"
					"try { return new ActiveXObject('Microsoft.XMLHTTP'); }  "
					"catch(e) { return null; } "
				"}"
			"}"
		"}"
		"</script></form>"
		"</body>"
		"</html>");

    PSOCK_SEND_STR(&s->sout,str);
    str[0]=0;
    PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*        LINK AND POE JSON format														    	*/
/*******************************************************************************************************/
static PT_THREAD(run_linkpoe_json(struct httpd_state *s, char *ptr)){
	RTC_TimeTypeDef RTC_Time;
	RTC_GetTime(RTC_Format_BIN,&RTC_Time);
	str[0]=0;
	PSOCK_BEGIN(&s->sout);

	if(get_dev_type() == DEV_SWU16){
		addstr("<table class=\"port_table\">"
		//1 строка
		"<tr><td class=\"port\">Port</td>");
		for(u8 i=0;i<(FIBER_PORT_NUM/2 + COOPER_PORT_NUM);i++){
			if(i<(FIBER_PORT_NUM/2)){
				addstr("<td id=\"port");
				sprintf(temp,"%d",i*2+2);
				addstr(temp);
				addstr("\" class=\"port\" onclick=\"open_port_info(");
				sprintf(temp,"%d",i*2+1);
				addstr(temp);
				addstr(")\">");
				sprintf(temp,"%d",i*2+2);
				addstr(temp);
				addstr("</td>");
			}
			else{
				addstr("<td></td>");
			}
		}
		addstr("</tr>"
		//2
		"<tr><td class=\"port\">Link</td>");
		for(u8 i=0;i<(FIBER_PORT_NUM/2 + COOPER_PORT_NUM);i++){
			if(i<(FIBER_PORT_NUM/2)){
				if(get_port_link(i*2+1)==1){
					addstr("<td id=\"link");
					sprintf(temp,"%d",i*2+2);
					addstr(temp);
					addstr("\" class=\"up\">Up");
					addstr("</td>");
				}
				else{
					addstr("<td id=\"link");
					sprintf(temp,"%d",i*2+2);
					addstr(temp);
					addstr("\" class=\"down\">Down");
					addstr("</td>");
				}
			}
			else{
				addstr("<td></td>");
			}
		}

		addstr("</tr>");


	    PSOCK_SEND_STR(&s->sout,str);
	    str[0]=0;

		//3
		addstr("<tr><td class=\"port\">Port</td>");
		for(u8 i=0;i<(FIBER_PORT_NUM/2);i++){
			addstr("<td id=\"port");
			sprintf(temp,"%d",i*2+1);
			addstr(temp);
			addstr("\" class=\"port\" onclick=\"open_port_info(");
			sprintf(temp,"%d",i*2);
			addstr(temp);
			addstr(")\">");
			sprintf(temp,"%d",i*2+1);
			addstr(temp);
			addstr("</td>");
		}
		for(u8 i=0;i<(COOPER_PORT_NUM);i++){
			addstr("<td id=\"port");
			sprintf(temp,"%d",FIBER_PORT_NUM+i+1);
			addstr(temp);
			addstr("\" class=\"port\" onclick=\"open_port_info(");
			sprintf(temp,"%d",FIBER_PORT_NUM+i);
			addstr(temp);
			addstr(")\">");
			sprintf(temp,"%d",FIBER_PORT_NUM+i+1);
			addstr(temp);
			addstr("</td>");
		}
		//4
		addstr("</tr>"
		"<tr><td class=\"port\">Link</td>");
		for(u8 i=0;i<(FIBER_PORT_NUM/2);i++){
			if(get_port_link(i*2)==1){
				addstr("<td id=\"link");
				sprintf(temp,"%d",i*2+1);
				addstr(temp);
				addstr("\" class=\"up\">Up");
				addstr("</td>");
			}
			else{
				addstr("<td id=\"link");
				sprintf(temp,"%d",i*2+1);
				addstr(temp);
				addstr("\" class=\"down\">Down");
				addstr("</td>");
			}
		}
		for(u8 i=0;i<(COOPER_PORT_NUM);i++){
			if(get_port_link(FIBER_PORT_NUM+i)==1){
				addstr("<td id=\"link");
				sprintf(temp,"%d",i+FIBER_PORT_NUM+1);
				addstr(temp);
				addstr("\" class=\"up\">Up");
				addstr("</td>");
			}
			else{
				addstr("<td id=\"link");
				sprintf(temp,"%d",i+FIBER_PORT_NUM+1);
				addstr(temp);
				addstr("\" class=\"down\">Down");
				addstr("</td>");
			}
		}
		addstr("</tr>");

	}
	else{
		//psw device
		addstr("<table class=\"port_table\">"
		"<tr><td class=\"port\">Port</td>");
		for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			addstr("<td id=\"port");
			sprintf(temp,"%d",i+1);
			addstr(temp);
			addstr("\" ");
			if(get_port_error(i))
				addstr("class = \"error\"");
			else
				addstr("class=\"port\"");
			addstr(" onclick=\"open_port_info(");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr(")\">");

			if(get_dev_type() == DEV_PSW2GPLUS){
				switch(i){
					case 0:addstr("FE#1");break;
					case 1:addstr("FE#2");break;
					case 2:addstr("FE#3");break;
					case 3:addstr("FE#4");break;
					case 4:addstr("GE#1");break;
					case 5:addstr("GE#2");break;
				}
			}
			else{
				sprintf(temp,"%d",i+1);
				addstr(temp);
			}


			addstr("</td>");
		}

		addstr("</tr><tr onclick=\"open_url("
				"'info/FEGE_stat.shtml',"
				"'help_en/FEGE_stat_help.html',"
				"'help_ru/FEGE_stat_help.html',");
		add_lang();
		addstr(")\"><td class=\"port\">Link</td>");
		for(u8 i=0;i<(ALL_PORT_NUM);i++){
				if(get_port_link(i)==1){
					addstr("<td id=\"link");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\" class=\"up\">Up</td>");
				}
				else{
					addstr("<td id=\"link");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\" class=\"down\">Down</td>");
				}
		}

		RTC_GetTime(RTC_Format_BIN,&RTC_Time);
		addstr("</tr>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		if(POE_PORT_NUM){
			addstr("<tr onclick=\"open_url("
					"'info/PoE_stat.shtml',"
					"'help_en/PoE_stat_help.html',"
					"'help_ru/PoE_stat_help.html',");
			add_lang();
			addstr(")\"><td class=\"port\">PoE</td>");
			for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				if(i<POE_PORT_NUM){
					addstr("<td id=\"poe");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\" ");
					//для psw2g4f  и psw1G
					if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)
							||(get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)
							|| (get_dev_type() == DEV_PSW2G8F)){
						if(get_port_poe_a(i)==1){
							addstr("class=\"up\">On");
						}
						else if(dev.port_stat[i].ss_process==1){
							sprintf(temp,"class=\"wait\">%d/%d",(u16)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes), (u16)(get_softstart_time()*60));
							addstr(temp);
						}
						else{
							addstr("class=\"down\">Off");
						}
					}

					//PSW 2G6F+ - 2 порта с PoE B
					if(get_dev_type() == DEV_PSW2G6F){
						if((get_port_poe_a(i)==1)||((i<POEB_PORT_NUM)&&(get_port_poe_b(i)==1))){
							addstr("class=\"up\">On");
						}
						else if(dev.port_stat[i].ss_process==1){
							sprintf(temp,"class=\"wait\">%d/%d",(u16)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes), (u16)(get_softstart_time()*60));
							addstr(temp);
						}
						else{
							addstr("class=\"down\">Off");
						}
					}

					//PSW 2G+, 2G2F+ - все порты с PoE B
					if((get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
						if((get_port_poe_a(i)==1)||(get_port_poe_b(i)==1))
							addstr("class=\"up\">On");
						else if(dev.port_stat[i].ss_process==1){
							sprintf(temp,"class=\"wait\">%d/%d",(u16)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes), (u16)(get_softstart_time()*60));
							addstr(temp);
						}
						else
							addstr("class=\"down\">Off");
					}
					addstr("</td>");
				}
				else
					addstr("<td class=\"down\">&nbsp</td>");
			}
			addstr("</tr>");
		}
	}
	addstr("</table>");


	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}





static PT_THREAD(run_header_name(struct httpd_state *s, char *ptr)){
static uint16_t pcount;
	str[0]=0;
    PSOCK_BEGIN(&s->sout);

	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		for(int i=0;i<pcount;i++){
		     char *pname,*pval;
		     pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		     pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			 if(!strcmp(pname,"Logout")){
				 if (!strncmp(pval, "L", 1)){
					  http_logout();
				 }
			}
		}
	}

    addstr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd\">"
    "<html>"
    "<body background=\"img/bg.gif\">"
    "<form method=\"post\" bgcolor=\"#808080\""
      	"action=\"/header_name.shtml\"><br><br>"
    "<table width=\"100%\" height=\"100%\"><tr align=\"center\">"
    "<td align=center><FONT SIZE=6 face=\"Arial\"  color=\"#003366\">"
    "<b>");
    get_dev_name_r(temp);
    addstr(temp);
    addstr("</b></FONT></td></tr>"
    "<tr><td align=\"right\" valign=\"bottom\">");
	if(http_passwd_enable){
		addstr(dev.user.current_username);
		addstr(",&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
		"<input type=\"Submit\" value=\"Logout\" name=\"Logout\"\">");
	}
	addstr(
    "</td></tr>"
    "</table>"
    "</form>"
    "</body>"
    "</html>");
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
    PSOCK_END(&s->sout);
}


static PT_THREAD(run_index(struct httpd_state *s, char *ptr)){
	char text_tmp[128];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	//addstr("<!DOCTYPE  HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN" "http://www.w3.org/TR/html4/frameset.dtd\">"
	addstr("<!DOCTYPE html>"
			"<html>"
			"<HEAD>"
				"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
			"<TITLE>");
	get_interface_name(text_tmp);
	text_tmp[127]=0;
	if((text_tmp[0]==0)||(text_tmp[0]==0xFF)||(text_tmp[1]==0xFF)||(text_tmp[2]==0xFF)||(text_tmp[3]==0xFF)){
		memset(temp,0,64);
		get_dev_name(temp);
		addstr(temp);
	}
	else
	{
		memset(temp,0,128);
		http_url_decode(text_tmp,temp,strlen(text_tmp));
			for(uint8_t i=0;i<128;i++){
				if(temp[i]=='+') temp[i] = ' ';
				if(temp[i]=='%') temp[i]=0;
			}
		text_tmp[strlen(text_tmp)]=0;
		memset(text_tmp,0,128);
		sprintf(text_tmp,temp);
		addstr(temp);
	}
	addstr(
			"</TITLE>"
			"</head>"
			"<FRAMESET ROWS=\"142,*\" FRAMEBORDER=\"0\" BORDER=\"0\">");
			addstr("<frame src=\"header_link.shtml\" name=\"header_link\" scrolling=\"no\" noresize>");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;

		addstr(
			"<FRAMESET COLS=\"220,1.5%,*,25%\" FRAMEBORDER=\"0\" BORDER=\"0\">");
			if(get_dev_type()==DEV_SWU16){
				if(get_interface_lang() == ENG){
					if(get_current_user_rule()==ADMIN_RULE)
						addstr("<frame src=\"tree_admin_swu_en.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
					else
						addstr("<frame src=\"tree_user_swu_en.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
				}
				else{
					if(get_current_user_rule()==ADMIN_RULE)
						addstr("<frame src=\"tree_admin_swu_rus.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
					else
						addstr("<frame src=\"tree_user_swu_rus.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
				}
			}else{
				if(get_interface_lang() == ENG){
					if(get_current_user_rule()==ADMIN_RULE)
						addstr("<frame src=\"tree_admin_en.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
					else
						addstr("<frame src=\"tree_user_en.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
				}
				else{
					if(get_current_user_rule()==ADMIN_RULE)
						addstr("<frame src=\"tree_admin_rus.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
					else
						addstr("<frame src=\"tree_user_rus.html\"    name=\"LeftFrame\" scrolling=\"yes\" noresize>");
				}
			}
			addstr("<frame name=\"SpaseFrame\" scrolling=\"no\" noresize>"
			"<frame src=\"main.shtml\" name=\"ContentFrame\" scrolling=\"yes\" >");
			if(get_interface_lang() == RUS){
				addstr("<frame src=\"help_ru/info_help.html\" name=\"HelpFrame\" scrolling=\"yes\" >");
			}
			else{
				addstr("<frame src=\"help_en/info_help.html\" name=\"HelpFrame\" scrolling=\"yes\" >");
			}

			addstr("</FRAMESET>"
			"</FRAMESET>"
			"</html>");
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}





static PT_THREAD(run_ntwk_settings(struct httpd_state *s, char *ptr)){
  //NOTE:local variables are not preserved during the calls to proto socket functins
  static uint16_t pcount;
  PSOCK_BEGIN(&s->sout);
  uip_ipaddr_t addr,ipaddr,ip_;
  static uint8_t haddr[6], ip[4], mask[4], gw[4],ds[4],dns[4];
  static u16 DHCP_mode=0;
  static u8 hops,opt82,garp;

  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
    int result = 0;

    for(int i=0;i<pcount;i++){
      char *pname,*pval;
      result = 1;
      pname = http_get_parameter_name(s->param,i,sizeof(s->param));
      pval = http_get_parameter_value(s->param,i,sizeof(s->param));

      if(!strcmp(pname, "mac0")){
    	  haddr[0]=(uint8_t)strtoul(pval, NULL, 16);
      }
      if(!strcmp(pname, "mac1")){
    	  haddr[1]=(uint8_t)strtoul(pval, NULL, 16);
      }
      if(!strcmp(pname, "mac2")){
    	  haddr[2]=(uint8_t)strtoul(pval, NULL, 16);
      }
      if(!strcmp(pname, "mac3")){
    	  haddr[3]=(uint8_t)strtoul(pval, NULL, 16);
      }
      if(!strcmp(pname, "mac4")){
    	  haddr[4]=(uint8_t)strtoul(pval, NULL, 16);
      }
      if(!strcmp(pname, "mac5")){
    	  haddr[5]=(uint8_t)strtoul(pval, NULL, 16);
      }
      /********* IP  ********/
      if(!strcmp(pname, "ip0")){
    	  ip[0]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "ip1")){
    	  ip[1]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "ip2")){
    	  ip[2]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "ip3")){
    	  ip[3]=(uint8_t)strtoul(pval, NULL, 10);
      }
      /*****    Mask ********/


      if(!strcmp(pname, "mask0")){
    	  mask[0]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "mask1")){
    	  mask[1]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "mask2")){
    	  mask[2]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "mask3")){
    	  mask[3]=(uint8_t)strtoul(pval, NULL, 10);
      }

      /******** GAteway *******/
      uip_getdraddr(&addr);
      if(!strcmp(pname, "gw0")){
    	gw[0]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "gw1")){
    	gw[1]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "gw2")){
    	gw[2]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "gw3")){
    	gw[3]=(uint8_t)strtoul(pval, NULL, 10);
      }

      /******** DNS *******/
      if(!strcmp(pname, "dns0")){
    	  dns[0]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "dns1")){
    	  dns[1]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "dns2")){
    	  dns[2]=(uint8_t)strtoul(pval, NULL, 10);
      }
      if(!strcmp(pname, "dns3")){
    	  dns[3]=(uint8_t)strtoul(pval, NULL, 10);
      }


	  if(!strcmp(pname, "DHCP")){
		if (!strncmp(pval, "0", 1)) DHCP_mode=0;
		if (!strncmp(pval, "1", 1)) DHCP_mode=1;
		if (!strncmp(pval, "2", 1)) DHCP_mode=2;
	  }

	  if(!strcmp(pname, "ds0")){
		ds[0]=(uint8_t)strtoul(pval, NULL, 10);
	  }
	  if(!strcmp(pname, "ds1")){
		ds[1]=(uint8_t)strtoul(pval, NULL, 10);
	  }
	  if(!strcmp(pname, "ds2")){
		ds[2]=(uint8_t)strtoul(pval, NULL, 10);
	  }
	  if(!strcmp(pname, "ds3")){
		ds[3]=(uint8_t)strtoul(pval, NULL, 10);
	  }

	  if(!strcmp(pname, "hops")){
		hops=(uint8_t)strtoul(pval, NULL, 10);
	  }

	  if(!strcmp(pname, "opt82")){
		if (!strncmp(pval, "0", 1)) opt82=0;
		if (!strncmp(pval, "1", 1)) opt82=1;
	  }

	  if(!strcmp(pname, "garp")){
		if (!strncmp(pval, "0", 1)) garp=0;
		if (!strncmp(pval, "1", 1)) garp=1;
	  }
    }


    if (result==1) {
    	if(get_dhcp_mode()==DHCP_RELAY){
			if((ds[0]==0)||(ds[3]==0))
				result=0;
			if((hops==0)||(hops>16))
				result=0;
    	}
    	if(result == 0){
    		set_dhcp_default();
    		result=1;
    	}
    }

    if (result==1) {
    	if (ntwk_wait_and_do!=0){
    		PSOCK_SEND_STR(&s->sout,"<b>Device busy!</b>");
    		PSOCK_CLOSE(&s->sout);
    	} else {
    		uip_ipaddr(ipaddr,ip[0],ip[1],ip[2],ip[3]);
    		set_net_ip(ipaddr);

    		uip_ipaddr(ipaddr,mask[0],mask[1],mask[2],mask[3]);
    		set_net_mask(ipaddr);

    		uip_ipaddr(ipaddr,gw[0],gw[1],gw[2],gw[3]);
    		set_net_gate(ipaddr);

    		uip_ipaddr(ipaddr,dns[0],dns[1],dns[2],dns[3]);
    		set_net_dns(ipaddr);

    		set_net_mac(haddr);

    		uip_ipaddr(ipaddr,ds[0],ds[1],ds[2],ds[3]);
    		set_dhcp_server_addr(ipaddr);

    		set_dhcp_opt82(opt82);
    		set_dhcp_hops(hops);
    		set_dhcp_mode(DHCP_mode);

    		set_gratuitous_arp_state(garp);

    		settings_save();

	  		uip_ipaddr(ipaddr,ip[0],ip[1],ip[2],ip[3]);
	  		if(!(uip_ipaddr_cmp(&ipaddr,&uip_hostaddr))){
	  			//alert("Please enter new IP address in browser");
	  			alertl("Please enter new IP address in browser","Введите новый IP в адресной строке браузера");
	  		}

	   	    alertl("Parameters accepted and rebooting...","Настройки применились, перезагрузка...");

			timer_set(&ntwk_timer, configTICK_RATE_HZ*4);
			ntwk_wait_and_do = 1;
    	}
    	PSOCK_EXIT(&s->sout);
    } else {
   		alertl("Parameters incorrect!", "Неверные параметры!");


    }
  }

  get_dhcp_cfg();
str[0]=0;
		  addhead();
		   addstrl("<body>"
		  "<legend>Network Settings</legend>","<legend>Сетевые настройки</legend>");

		  if(get_current_user_rule()!=ADMIN_RULE){
			  addstr("Access denied");
		  }
		  else{
	 		  addstr("<form method=\"post\" bgcolor=\"#808080\""
			  "action=\"/settings/settings_ntwk.shtml\">"
			  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" width=\"60%\">"
			  "<tr class=\"g2\"><td>MAC</td><td>");
			for(uint8_t i=0;i<6;i++){
				addstr("<input type=\"text\"name=\"mac");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\" size=\"2\" maxlength=\"2\"value=\"");
				snprintf(temp,4,"%02X",(int)dev_addr[i]);
				addstr(temp);
				addstr("\">");
			}

			  //get_net_ip(&ip_);
			  addstr("</tr>"
			  "<tr class=\"g2\"><td>IP</td><td>"
			  "<input type=\"text\" name=\"ip0\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(int)uip_ipaddr1(&uip_hostaddr));
			  addstr(temp);
			  addstr(
			  "\"><input type=\"text\" name=\"ip1\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(int)uip_ipaddr2(&uip_hostaddr));
			  addstr(temp);
			  addstr(
			  "\"><input type=\"text\" name=\"ip2\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(int)uip_ipaddr3(&uip_hostaddr));
			  addstr(temp);
			  addstr("\"><input type=\"text\" name=\"ip3\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(int)uip_ipaddr4(&uip_hostaddr));
			  addstr(temp);
			  addstr("\"></td></tr>"
			  "<tr class=\"g2\">");
			  addstr("<td>");

			  PSOCK_SEND_STR(&s->sout,str);
			  str[0]=0;

			  addstrl("Mask","Маска");
			  addstr("</td><td>"
			  "<input type=\"text\" name=\"mask0\"size=\"3\"maxlength=\"3\"value=\"");
			  //get_net_mask(&ip_);
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr1(&uip_netmask));
			  addstr(temp);
			  addstr("\"><input type=\"text\" name=\"mask1\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr2(&uip_netmask));
			  addstr(temp);
			  addstr("\"><input type=\"text\" name=\"mask2\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr3(&uip_netmask));
			  addstr(temp);
			  addstr("\"><input type=\"text\" name=\"mask3\"size=\"3\"maxlength=\"3\"value=\"");
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr4(&uip_netmask));
			  addstr(temp);
			  addstr("\"></td></tr>");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;

			  addstr("<tr class=\"g2\"><td>");
			  addstrl("Gateway","Шлюз");
			  addstr("</td><td>"
			  "<input type=\"text\" name=\"gw0\" size=\"3\" maxlength=\"3\" value=\"");
			  //get_net_gate(&ip_);
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr1(&uip_draddr));
			  addstr(temp);
			  addstr( "\"><input type=\"text\" name=\"gw1\" size=\"3\" maxlength=\"3\" value=\"");
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr2(&uip_draddr));
			  addstr(temp);
			  addstr( "\"><input type=\"text\" name=\"gw2\" size=\"3\" maxlength=\"3\" value=\"");
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr3(&uip_draddr));
			  addstr(temp);
			  addstr("\"><input type=\"text\" name=\"gw3\" size=\"3\" maxlength=\"3\" value=\"");
			  snprintf(temp,sizeof(temp),"%d",(u8)uip_ipaddr4(&uip_draddr));
			  addstr(temp);
			  addstr("\"></td></tr>");

			  addstr("<tr class=\"g2\"><td>");
			  addstrl("DNS","DNS");
			  addstr("</td><td>"
			  "<input type=\"text\" name=\"dns0\" size=\"3\" maxlength=\"3\" value=\"");
			  sprintf(temp,"%d",(u8)uip_ipaddr1(&uip_dns_addr));
			  addstr(temp);
			  addstr( "\"><input type=\"text\" name=\"dns1\" size=\"3\" maxlength=\"3\" value=\"");
			  sprintf(temp,"%d",(u8)uip_ipaddr2(&uip_dns_addr));
			  addstr(temp);
			  addstr( "\"><input type=\"text\" name=\"dns2\" size=\"3\" maxlength=\"3\" value=\"");
			  sprintf(temp,"%d",(u8)uip_ipaddr3(&uip_dns_addr));
			  addstr(temp);
			  addstr("\"><input type=\"text\" name=\"dns3\" size=\"3\" maxlength=\"3\" value=\"");
			  sprintf(temp,"%d",(u8)uip_ipaddr4(&uip_dns_addr));
			  addstr(temp);
			  addstr("\"></td></tr>"

			  "<tr class=\"g2\"><td>");
			  addstrl("DHCP Mode","Режим DHCP");
			  addstr( "</td><td>"
			  "<select name=\"DHCP\" size=\"1\"><option");
			  if(get_dhcp_mode()==DISABLE)
				  addstr(" selected");
			  addstr(" value=\"0\">Disable</option><option");
			  if(get_dhcp_mode()==DHCP_CLIENT)
				  addstr(" selected");
			  addstr(" value=\"1\">Client</option>");

			  if((get_marvell_id() == DEV_88E097)){
				  addstr("<option");
				  if(get_dhcp_mode()==DHCP_RELAY)
					  addstr(" selected");
				  addstr(" value=\"2\">Relay</option>");
			  }

			  addstr("</select></td></tr>"
			  "<tr class=\"g2\"><td>");
			  addstrl("Gratuitous ARP","Самообращённые ARP");
			  addstr( "</td><td>"
			  "<select name=\"garp\" size=\"1\"><option");
			  if(get_gratuitous_arp_state()==DISABLE)
				  addstr(" selected");
			  addstr(" value=\"0\">Disable</option><option");
			  if(get_gratuitous_arp_state()==ENABLE)
				  addstr(" selected");
			  addstr(" value=\"1\">Enable</option>");
			  addstr("</select></td></tr>"
			  "</table>");

			  // DHCP Relay agent //

			  if((get_marvell_id() == DEV_88E097)){

				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;
				  addstr("<br><br><b>");
				  addstrl("DHCP Relay Settings","Настройки DHCP ретранслятора");
				  addstr("</b><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" width = \"50%\">"
				  "<tr class=\"g2\"><td>");
				  addstrl("DHCP Server","DHCP сервер");
				  addstr("</td><td>"
				  "<input type=\"text\" name=\"ds0\" size=\"3\" maxlength=\"3\" value=\"");
				  get_dhcp_server_addr(&ip_);
				  snprintf(temp,sizeof(temp),"%d",(uint8_t)uip_ipaddr1(ip_));
				  addstr(temp);
				  addstr("\"><input type=\"text\" name=\"ds1\" size=\"3\" maxlength=\"3\" value=\"");
				  snprintf(temp,sizeof(temp),"%d",(uint8_t)uip_ipaddr2(ip_));
				  addstr(temp);
				  addstr("\"><input type=\"text\" name=\"ds2\" size=\"3\" maxlength=\"3\" value=\"");
				  snprintf(temp,sizeof(temp),"%d",(uint8_t)uip_ipaddr3(ip_));
				  addstr(temp);
				  addstr("\"><input type=\"text\" name=\"ds3\" size=\"3\" maxlength=\"3\" value=\"");
				  snprintf(temp,sizeof(temp),"%d",(uint8_t)uip_ipaddr4(ip_));
				  addstr(temp);
				  addstr(
				  "\"></td></tr>"
				  "<tr class=\"g2\"><td>Hops limit</td><td>"
				  "<input type=\"text\" name=\"hops\" size=\"2\" maxlength=\"2\" value=\"");
				  snprintf(temp,sizeof(temp),"%d",(uint8_t)get_dhcp_hops());
				  addstr(temp);
				  addstr(
				  "\"></td></tr>"
				  "<tr class=\"g2\"><td>DHCP Option 82</td><td>"
				  "<select name=\"opt82\" size=\"1\"><option");
				  if(get_dhcp_opt82()==DISABLE)addstr(" selected");
				  addstr(" value=\"0\">Disable</option>"
				  "<option");
				  if(get_dhcp_opt82()==ENABLE)addstr(" selected");
				  addstr(" value=\"1\">Enable</option>"
				  "</select></td></tr>"
				  "</table>");
			  }
			  addstr("<br><br><input type=\"Submit\" value=\"Apply\" tabindex = \"1\">"
			  "</form></body></html>");

		 }
		 PSOCK_SEND_STR(&s->sout,str);
		 str[0]=0;

		 PSOCK_END(&s->sout);
}


static PT_THREAD(run_admin_settings(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  static  uint16_t pcount;
  int sc;
  static char tmp_pswd[HTTPD_MAX_LEN_PASSWD+1];
  static char curr_login[HTTPD_MAX_LEN_PASSWD+1];
  //static char curr_passwd[HTTPD_MAX_LEN_PASSWD+1];
  static char new_login[HTTPD_MAX_LEN_PASSWD+1];
  static char new_passwd[HTTPD_MAX_LEN_PASSWD+1];
  static char new_passwd_co[HTTPD_MAX_LEN_PASSWD+1];


  static u8 edit_add_user;
  static u8 user_num;
  static u8 delete;
  static u8 rule;
  static u8 apply;

  memset(new_login,0,HTTPD_MAX_LEN_PASSWD+1);
  memset(new_passwd,0,HTTPD_MAX_LEN_PASSWD+1);
  memset(new_passwd_co,0,HTTPD_MAX_LEN_PASSWD+1);

  PSOCK_BEGIN(&s->sout);

  //get_interface_login(curr_login);
  //get_interface_passwd(curr_passwd);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

		  	  memset(new_login,0,HTTPD_MAX_LEN_PASSWD+1);
		  	  memset(new_passwd,0,HTTPD_MAX_LEN_PASSWD+1);
		  	  memset(new_passwd_co,0,HTTPD_MAX_LEN_PASSWD+1);

			  for(int i=0;i<pcount;i++){
				  char *pname,*pval;
				  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
				  pval = http_get_parameter_value(s->param,i,sizeof(s->param));
				  if(!strcmp(pname, "user")){
					  strcpy(new_login,pval);
				  }

				  if(!strcmp(pname, "passwd")){
					  strcpy(new_passwd,pval);
				  }
				  if(!strcmp(pname, "passwd_confirm")){
					  strcpy(new_passwd_co,pval);
				  }

				  for(u8 j=0;j<MAX_USERS_NUM;j++){
					  sprintf(temp,"Edit%d",j);
					  if(!strcmp(pname,temp)){
						  if (!strncmp(pval, "E", 1)){
							  edit_add_user = 1;
							  user_num = j;
						  }
					  }
				  }

				  for(u8 j=0;j<MAX_USERS_NUM;j++){
					  sprintf(temp,"Add%d",j);
					  if(!strcmp(pname,temp)){
						  if (!strncmp(pval, "A", 1)){
							  edit_add_user = 1;
							  user_num = j;
						  }
					  }
				  }

				  for(u8 j=0;j<MAX_USERS_NUM;j++){
					  sprintf(temp,"Delete%d",j);
					  if(!strcmp(pname,temp)){
						  if (!strncmp(pval, "D", 1)){
							  delete = 1;
							  user_num = j;
						  }
					  }
				  }

				  if(!strcmp(pname,"rule")){
					  if (!strncmp(pval, "0", 1))rule = NO_RULE;
					  if (!strncmp(pval, "1", 1))rule = ADMIN_RULE;
					  if (!strncmp(pval, "2", 1))rule = USER_RULE;
				  }

				  if(!strcmp(pname,"Apply")){
					  if (!strncmp(pval, "A", 1)){
						  apply = 1;
					  }
				  }

				  if(!strcmp(pname,"Cancel")){
					  if (!strncmp(pval, "C", 1)){
						  apply = 0;
						  edit_add_user = 0;
					  }
				  }

				  if(!strcmp(pname,"Logout")){
					  if (!strncmp(pval, "L", 1)){
						  http_logout();
					  }
				  }

			  }
		  sc = 1;
		  if(apply == 1){
			  if(edit_add_user == 1){
				  if((new_login[0]==0)||(new_passwd[0]==0)) sc = 0;
				  if((strlen(new_login)>HTTPD_MAX_LEN_PASSWD)||(strlen(new_passwd)>HTTPD_MAX_LEN_PASSWD)) sc = 0;
				  if(strcmp(new_passwd,new_passwd_co)) sc = 0;

				  if(sc){
					  memset(tmp_pswd,0,HTTPD_MAX_LEN_PASSWD+1);
					  http_url_decode(new_login, tmp_pswd, HTTPD_MAX_LEN_PASSWD);
					  set_interface_users_username(user_num,tmp_pswd);

					  memset(tmp_pswd,0,HTTPD_MAX_LEN_PASSWD+1);
					  http_url_decode(new_passwd, tmp_pswd, HTTPD_MAX_LEN_PASSWD);
					  set_interface_users_password(user_num,tmp_pswd);

					  set_interface_users_rule(user_num,rule);
					  alertl("Parameters Accepted", "Настройки применились");
					  settings_save();
					  httpd_renew_passwd();
				  }else {
					  memset(new_login,0,HTTPD_MAX_LEN_PASSWD+1);
					  memset(new_passwd,0,HTTPD_MAX_LEN_PASSWD+1);
					  memset(new_passwd_co,0,HTTPD_MAX_LEN_PASSWD+1);
					  str[0]=0;
					  alertl("Parameters incorrect","Неверные параметры");
					  PSOCK_SEND_STR(&s->sout,str);
				  }
			  }
			  edit_add_user = 0;
			  apply = 0;
		  }
		  if(delete == 1){
			  memset(temp,0,64);
			  set_interface_users_username(user_num,temp);
			  set_interface_users_password(user_num,temp);
			  set_interface_users_rule(user_num,NO_RULE);
			  alertl("Parameters Accepted","Настройки применились");
			  settings_save();
			  delete = 0;
		  }


	  }

	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }
	  else{
		  addhead();
		  addstrl("<body><legend>User Accounts Settings</legend>","<legend>Настройки учетных записей пользователей</legend>");
		  addstr ("<form method=\"post\" bgcolor=\"#808080\""
				  "action=\"/settings/settings_admin.shtml\"><br><br><b>");
		  addstrl("Current user name","Текущее имя пользователя");
		  addstr("</b>: ");
		  if(http_passwd_enable){
			  addstr(dev.user.current_username);
		  }
		  addstr("<br><br>"
				  "<b>User list</b><br>"
				  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				  "<tr class=\"g2\"><td></td>");
		  addstrl("<td>User Name</td>","<td>Имя пользователя</td>");
		  addstrl("<td>Password</td>","<td>Пароль</td>");
		  addstrl("<td>Access Right</td>","<td>Права доступа</td>");
		  addstr("<td></td></tr>");
		  for(u8 i=0;i<MAX_USERS_NUM;i++){
			  addstr("<tr class=\"g2\"><td>");
			  sprintf(temp,"%d",i+1);
			  addstr(temp);
			  addstr("</td><td>");
			  get_interface_users_username(i,temp);
			  http_url_decode(temp, curr_login, HTTPD_MAX_LEN_PASSWD);
			  addstr(curr_login);
			  memset(curr_login,0,HTTPD_MAX_LEN_PASSWD);
			  addstr("</td><td>");
			  get_interface_users_password(i,temp);
			  if(strlen(temp)){
				  for(u8 j=0;j<strlen(temp);j++)
					  addstr("*");
			  }
			  addstr("</td><td>");
			  if(i==0){
				  addstr("Admin");
			  }
			  else{
				  switch(get_interface_users_rule(i)){
					  case NO_RULE:
						  break;
					  case ADMIN_RULE:
						  addstr("Admin");
						  break;
					  case USER_RULE:
						  addstr("User");
						  break;
				  }
			  }
			  addstr("</td><td>");
			  get_interface_users_username(i,temp);
			  if((strlen(temp) && (get_interface_users_rule(i)==ADMIN_RULE || get_interface_users_rule(i)==USER_RULE)) || (i==0)){
				  sprintf(temp,"%d",i);
				  addstr("<input type=\"Submit\" value=\"Edit\" name=\"Edit");
				  addstr(temp);
				  addstr("\">");
				  if(i!=0){
					  addstr("<input type=\"Submit\" value=\"Delete\" name=\"Delete");
					  addstr(temp);
					  addstr("\">");
				  }
				  addstr("</td></tr>");
			  }
			  else{
				  addstr("<input type=\"Submit\" value=\"Add New User\" name=\"Add");
				  sprintf(temp,"%d",i);
				  addstr(temp);
				  addstr("\">");
				  addstr("</td></tr>");
				  break;
			  }


		  }
		  addstr("</table><br>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  if((edit_add_user == 1)&&(user_num<MAX_USERS_NUM)){
			  addstr("<b>Add/Edit user</b><br>"
					  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
					  "<tr class=\"g2\">");
			  addstrl("<td>User Name</td>","<td>Новое имя пользователя</td>");
			  addstr( "<td><input type=\"text\" name=\"user\" size=\"12\" maxlength=\"10\" value=\"");
			  get_interface_users_username(user_num,temp);
			  addstr(temp);
			  addstr("\">");
			  addstr( "</td></tr>");
		  	  addstr("<tr class=\"g2\"><td>");
	  		  addstrl("New Password","Новый пароль");
	  		  addstr("</td><td>");
	  		  addstr("<input type=\"password\" name=\"passwd\" size=\"12\" maxlength=\"20\" value=\"");
			  addstr( "\"></td></tr>");
			  addstr( "<tr class=\"g2\"><td>");
			  addstrl("Password Confirm","Повторите пароль");
			  addstr("</td><td>");
			  addstr( "<input type=\"password\"name=\"passwd_confirm\"size=\"12\"maxlength=\"20\"value=\"");
			  addstr("\"></td></tr>"
					  "<tr class=\"g2\"><td>");
			  addstrl("Access Right","Права доступа");
			  addstr("</td>"
					 "<td><select ");
			  if(user_num==0)
				  addstr(" disabled ");
			  addstr("name=\"rule\" size=\"1\">"
					 "<option");
			  if((get_interface_users_rule(user_num)==ADMIN_RULE)||(user_num == 0))
				  addstr(" selected");
			  addstr(" value=\"1\">Admin</option>"
			  "<option");
			  if((get_interface_users_rule(user_num)==USER_RULE)&&(user_num))
				  addstr(" selected");
			  addstr(" value=\"2\">User</option>"
			  "</select></td>"
			  "</tr></table><br>");
			  addstr("<input type=\"Submit\" value=\"Apply\" name=\"Apply\">  ");
			  addstr("<input type=\"Submit\" value=\"Cancel\" name=\"Cancel\">");
		}

	  	addstr("</form></body></html>");
	  }

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}

static PT_THREAD(run_clear(struct httpd_state *s, char *ptr)){
	PSOCK_BEGIN(&s->sout);
	settings_add2queue(SQ_ERASE);
	PSOCK_END(&s->sout);
}



static PT_THREAD(run_update(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  uint16_t pcount;
  //static uint8_t ln, update_confirm = 0;;
  //static u8 uploading=0;
  extern const uint32_t image_version[1];
  static  uint32_t Tmp;
  int i;

	  PSOCK_BEGIN(&s->sout);
	  nosave();


	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(i=0;i<pcount;i++){
	  		char *pname,*pval;
	  		pname = http_get_parameter_name(s->param,i,sizeof(s->param));
	  		pval = http_get_parameter_value(s->param,i,sizeof(s->param));
	  		//sc = 0;
	  		if(!strcmp(pname,"Cancel")){
	  			if (!strncmp(pval, "C", 1)) {
	  				update_state=UPDATE_STATE_IDLE;
	  				ntwk_wait_and_do=0;
	  			}
	  		}

	  		if(!strcmp(pname, "Update")){
	  			if (!strncmp(pval, "U", 1)) {
	  				if(update_state==UPDATE_STATE_UPLOADED)
	  					update_state = UPDATE_STATE_UPDATING;
	  			}
	  		}

	  		if(!strcmp(pname, "Upload")){
	  			if (!strncmp(pval, "U", 1)) {
	  				if(update_state==UPDATE_STATE_IDLE)
	  					update_state=UPDATE_STATE_UPLOADING;
	  			}
	  		}

	  		if(!strcmp(pname, "Upload2")){
	  			if (!strncmp(pval, "U", 1)) {
	  				if(update_state==UPDATE_STATE_IDLE)
	  					update_state=UPDATE_STATE_UPLOADING;
	  			}
	  		}
		  }
	  }

	  if (update_state==UPDATE_STATE_IDLE){
		  str[0]=0;
		  addhead();
		  addstr("<body><script type=\"text/javascript\">"
		  "var TimeoutID;"
		  "var Time;"
		  "var FormS;"
		  "var Str;"
			  "function form () {"
			  "Str+=\" .\";"
			  "return(Str);"
			  "}"
			  "function submit_timeout() {"
				  "FormS.submit();"
			  "}"
			  "function submit_form (f) {"
		  	  	  "FormS = f;"
		  	  	  "window.setTimeout('submit_timeout();',10000);"
				  "var xhr = new XMLHttpRequest();"
				  "xhr.open('GET', '/clear.shtml', false);"
				  "xhr.send();"
			  "}"
			  "function showtime(){"
				  "document.getElementById('timer').innerHTML = '<br>Firmware loaded '+form();"
				  "Time++;"
				  "if(Time<40){"
				  	  "window.setTimeout('showtime();',1000);"
				  "}"
			  "}"
			  "function start_timer (){ "
				"Time=0;"
				"Str='';"
				"if(document.getElementById('file_id').value){"
				 	  "TimeoutID=window.setTimeout('showtime();', 1000);"
				"}"
			  "}"
		  "</script>");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  addstrl("<legend>Update Firmware</legend>","<legend>Обновление ПО</legend>");
		  if(get_current_user_rule()!=ADMIN_RULE){
			  addstr("Access denied");
		  }
		  else{
			  addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
			  "action=\"/mngt/update.shtml\" onSubmit=\"submit_form(this); return false\">");
			  addstrl("Current Firmware version: ","Текущая версия ПО: ");
			  snprintf(temp,sizeof(temp), "%02X.%02X.%02X",
					  (int)(image_version[0]>>16)&0xff,
					  (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
			  addstr(temp);
			  addstr("<br><br>");
			  addstr( "<table><tr><td>File:</td>"
			  "<td><input type=\"file\" name=\"updatefile\" id=\"file_id\" size=\"24\" value=\"\">"
			  "</td></tr></table>"
			  "<span id = \"timer\">"
			  "<br>"
			  "<input type=\"Submit\" value=\"Upload\" name=\"Upload\" onclick=\"start_timer()\"></span>");
			  addstr("</form>");
		  }
	  }
	  else if (update_state==UPDATE_STATE_UPLOADING){

		  str[0]=0;
		  addhead();
		  addstrl("<body><legend>Update Firmware</legend>","<body><legend>Обновление ПО</legend>");
		  if(get_current_user_rule()!=ADMIN_RULE){
			  addstr("Access denied");
		  }
		  else{
			  addstr("<form method=\"get\" bgcolor=\"#808080\""
				  "action=\"/mngt/update.shtml\">");
			  addstr("<meta http-equiv=\"refresh\" content=\"35; url=/mngt/update.shtml\">");
			  addstr("Firmware downloading error");
			  addstr("<br><br></form></body></html>");
		  }
		  update_state=UPDATE_STATE_IDLE;
	  }
	  else if (update_state==UPDATE_STATE_UPLOADED){
		  s->parse_state = 0;
		  str[0]=0;
		  addhead();
		  addstrl("<body><legend>Update Firmware</legend>","<body><legend>Обновление ПО</legend>");
		  if(get_current_user_rule()!=ADMIN_RULE){
			  addstr("Access denied");
		  }else{
			  addstr("<form method=\"get\" bgcolor=\"#808080\""
				  "action=\"/mngt/update.shtml\">");

			  Tmp=TestFirmware();

			  if(Tmp==0){
				  Tmp = get_new_fw_vers();
				  addstrl("Version of loaded firmware: ","Версия загруженной прошивки: ");
				  sprintf(temp,"%02X.%02X.%02X",(int)(Tmp>>16)&0xff,(int)(Tmp>>8)&0xff,(int)(Tmp)&0xff);
				  addstr(temp);
				  addstr("<br><br>"
					  "<tr align=\"justify\"><td colspan=2>"
					  "<input type=\"Submit\" value=\"Update\" name =\"Update\" id=\"Update\"></td>");
			  }else{
				  ntwk_wait_and_do=0;
				  update_state=0;
				  addstr("<font color=\"red\">");
				  addstrl("Firmware loaded incorrect, please try argain.","ПО загружено с ошибками, попробуйте еще раз");
				  addstr("</font><br>");
				  switch(Tmp){
					  case 1:addstr("<font color=\"red\">Version error</font><br>");break;
					  case 8:addstr("<font color=\"red\">Incorrect ID</font><br>");break;
					  case 9:addstr("<font color=\"red\">Incorrect length</font><br>");break;
					  case 10:addstr("<font color=\"red\">Incorrect CRC</font><br>");break;
					  default:
						  sprintf(temp,"<font color=\"red\">Error code %lu</font><br>",Tmp);
						  addstr(temp);
						  break;
				  }
			  }
			  addstr("<td colspan=2><input type=\"Submit\"value=\"Cancel\" name=\"Cancel\" onClick=\"cancel()\">"
			  "</td></tr>"
			  "</table>"
			  "<input type=hidden name=\"update\" value=\"1\" id=\"update\">"
			  "</form></body></html>");
		  }
	  }
	  else if (update_state==UPDATE_STATE_UPDATING){
			  //взводим флажок
			  need_update_flag[0] = NEED_UPDATE_YES;
			  //получаем версию новой прошивки
			  Tmp = get_new_fw_vers();
			  send_events_u32(EVENT_UPDATE_FIRMWARE,(u32)Tmp);
			  str[0]=0;
			  addhead();
			  addstr("<body><h3><b><font color=\"red\">");
			  addstrl("Updating firmware, please wait","Обновление ПО, пожалуйста подождите...");
			  addstr( "</font></b></h3>");
			  addstr("<meta http-equiv=\"refresh\" content=\"35; url=/main.shtml\"></body></html>");
			  timer_set(&ntwk_timer, 2000*MSEC);
			  ntwk_wait_and_do = 2;
		}

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}





static PT_THREAD(run_reboot(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  uint16_t pcount;
  int i;
  static  uint8_t AllR=0;
  str[0]=0;
	  PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  int sc;

		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			sc = 0;
			if(!strcmp(pname, "reboot")){
				if (!strncmp(pval, "Reboot", 1)) {sc = 1;AllR=1;}
			}
			if (sc){
				if (update_state==UPDATE_STATE_IDLE){
						alertl("Rebooted","Перезагрузка...");
						PSOCK_SEND_STR(&s->sout,
						"<meta http-equiv=\"refresh\" content=\"10; url=/main.shtml\">");
				    PSOCK_CLOSE(&s->sout);
				    timer_set(&ntwk_timer, 5000*MSEC);
				    if(AllR==1)
				    	ntwk_wait_and_do = 3;
				    else
				    	ntwk_wait_and_do = 2;
				    PSOCK_EXIT(&s->sout);
				} else{
					if(AllR==1){
						reboot(REBOOT_ALL);
					}
					break;
				}
			}
		  }
	  }

	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Reboot Device</legend>","<legend>Перезагрузка</legend>");
	  addstr("<form method=\"post\" bgcolor=\"#808080\""
	  	"action=\"/mngt/reboot.shtml\"><br><br>"
	   	"<input type=\"Submit\" name =\"reboot\" value=\"Reboot\">"
	  	"</form></body><html>"
	  );
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}

static PT_THREAD(run_FEGE_set(struct httpd_state *s, char *ptr)){
///////////////////////////////////////////////////////////////////////////////////////////
//NOTE:local variables are not preserved during the calls to proto socket functins
static uint16_t pcount,i;
///*static*/  uint8_t old[10];
static u8 apply=0;
static  uint8_t enable_state[PORT_NUM];
static  uint8_t enable_stateLast[PORT_NUM];

static  uint8_t Flow[PORT_NUM];
static  uint8_t FlowLast[PORT_NUM];

static  uint8_t speed[PORT_NUM];
static  uint8_t speedLast[PORT_NUM];

static  uint8_t poe[PORT_NUM];
//static  uint8_t poe_b[PORT_NUM];

static u8 pla[PORT_NUM];
static u8 plb[PORT_NUM];

static u8 sfp_mode[PORT_NUM];

str[0]=0;


PSOCK_BEGIN(&s->sout);

/*status*/
for(i=0;i<POE_PORT_NUM;i++){
	poe[i] = get_port_sett_poe(i);
	//poe_b[i] = get_port_sett_poe_b(i);
	pla[i] = get_port_sett_pwr_lim_a(i);
	plb[i] = get_port_sett_pwr_lim_b(i);
}

for(i=0;i<ALL_PORT_NUM;i++){
	enable_stateLast[i]=enable_state[i]=get_port_sett_state(i);
	speedLast[i]=speed[i] = get_port_sett_speed_dplx(i);
	FlowLast[i]=Flow[i] = get_port_sett_flow(i);
}


//fiber port only
if(get_dev_type() != DEV_SWU16){
	for(i=COOPER_PORT_NUM;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		sfp_mode[i-COOPER_PORT_NUM] = get_port_sett_sfp_mode(i);
	}
}



	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
	  		  for(u8 k=0;k<pcount;k++){
	  			static  char *pname,*pval;
	  	  		pname = http_get_parameter_name(s->param,k,sizeof(s->param));
	  	  		pval = http_get_parameter_value(s->param,k,sizeof(s->param));


				if(!strcmp(pname, "Apply")){
					if (!strncmp(pval, "A", 1)) apply = 1;
				}



				//state
	  	  		for(u8 j=0;j<(ALL_PORT_NUM);j++){
	  	  			sprintf(temp,"P%dE",j+1);
	  	  			if(!strcmp(pname, temp)){
	  	  				if (!strncmp(pval, "1", 1)) {enable_state[j] = 1;}
	  	  				if (!strncmp(pval, "0", 1)) {enable_state[j] = 0;}
	  	  			}
	  	  		}

	  	  		//speed
	  	  		for(u8 j=0;j<(ALL_PORT_NUM);j++){
	  	  			sprintf(temp,"P%dS",j+1);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1)) 	speed[j] = 0;//auto
						if (!strncmp(pval, "1", 1)) 	speed[j] = 1;//10half
						if (!strncmp(pval, "2", 1)) 	speed[j] = 2;//10full
						if (!strncmp(pval, "3", 1)) 	speed[j] = 3;//100half
						if (!strncmp(pval, "4", 1)) 	speed[j] = 4;//100full
						if (!strncmp(pval, "5", 1)) 	speed[j] = 5;//1000half
						if (!strncmp(pval, "6", 1)) 	speed[j] = 6;//1000full
					}
	  	  		}

	  	  		//flow
				for(u8 j=0;j<(ALL_PORT_NUM);j++){
					sprintf(temp,"P%dF",j+1);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1)) 	Flow[j] = 0;
						if (!strncmp(pval, "1", 1)) 	Flow[j] = 1;
						if (!strncmp(pval, "2", 1)) 	Flow[j] = 2;
					}

				}

				//poe a
				for(u8 j=0;j<(POE_PORT_NUM);j++){
					sprintf(temp,"P%dA",j+1);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1)) 	poe[j] = 0;
						if (!strncmp(pval, "1", 1)) 	poe[j] = 1;
						if (!strncmp(pval, "2", 1)) 	poe[j] = 2;
						if (!strncmp(pval, "3", 1)) 	poe[j] = 3;
						if (!strncmp(pval, "4", 1)) 	poe[j] = 4;
						if (!strncmp(pval, "5", 1)) 	poe[j] = 5;
						if (!strncmp(pval, "6", 1)) 	poe[j] = 6;
					}
				}


				//limit poe
				for(u8 j=0;j<(POE_PORT_NUM);j++){
					sprintf(temp,"P%dL",j+1);
					if(!strcmp(pname, temp)){
						pla[j]=(uint8_t)strtoul(pval, NULL, 10);
					}
				}

				//sfp mode
				for(u8 j=0;j<FIBER_PORT_NUM;j++){
					sprintf(temp,"SFP%d",j+1);
					if(!strcmp(pname, temp)){
						sfp_mode[j]=(uint8_t)strtoul(pval, NULL, 10);
					}
				}

	  		  }


	  		if(apply == 1){

	  			//if(get_dev_type() == DEV_PSW2G2FPLUSUPS || get_dev_type() == DEV_PSW2G2FPLUS){
	  				//if(poe[1]==1 && (poe[0]==1 || poe[0]==2 || poe[0]==3)){
	  				//	alertl("The power budget is exceeded!","Превышен бюджет мощности!");
	  				//	apply = 0;
	  				//	return;
	  				//}
	  			//}
	  			for(i=0;i<POE_PORT_NUM;i++){
					set_port_poe(i,poe[i]);
					set_port_pwr_lim_a(i,pla[i]);
					set_port_pwr_lim_b(i,plb[i]);
	  			}

				for(i=0;i<(ALL_PORT_NUM);i++){
					set_port_speed_dplx(i,speed[i]);
				}

				for(i=0;i<(ALL_PORT_NUM);i++){
					set_port_state(i,enable_state[i]);
					set_port_flow(i,Flow[i]);
				}

				if(get_dev_type() != DEV_SWU16){
					for(i=COOPER_PORT_NUM;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
						set_port_sfp_mode(i,sfp_mode[i-COOPER_PORT_NUM]);
					}
				}

				settings_save();
				for(i=0;i<ALL_PORT_NUM;i++){
					//if((enable_state[i]!=enable_stateLast[i])||(speed[i]!=speedLast[i])||(Flow[i]!=FlowLast[i])){
						settings_add2queue(SQ_PORT_1+i);
					//}
				}
				set_poe_init(0);
				alertl("Parameters accepted","Настройки применились");
				apply = 0;
	  		}
	  	   }

			addhead();
			addstr("<body>");
			addstrl("<legend>Port Settings</legend>","<legend>Настройки портов</legend>");
			if(get_current_user_rule()!=ADMIN_RULE){
				  addstr("Access denied");
			}
			else{
				addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/settings/FEGE_set.shtml\">");
				if(get_dev_type()==DEV_PSW2GPLUS){
					addstr("<FONT color = \"red\">Attention! Do not connect Non PoE-devices (e.g. PC) to FE port when PoE B in Passive mode"
					"<br>Внимание! Не подключайте устройство без поддержки PoE(например компьютер) к порту в "
					"режиме Passive PoE."
					"</font><br><br>");
				}

				//table head
				addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\"><td>");
				addstrl("Port","Порт");
				addstr("</td><td>");
				addstrl("State","Состояние");
				addstr("</td><td>");
				addstrl("Speed/Duplex","Скорость/Дуплекс");
				addstr("</td><td>");
				addstrl("Flow Control","Управление потоком");
				addstr("</td>");
				if(POE_PORT_NUM){
					addstr("<td>PoE</td>");
		#if POE_LIMIT
					addstr("<td>");
					addstrl("Power Limit PoE","Лимит PoE");
					addstr("</td>");
		#endif

				}
				if(get_dev_type() != DEV_SWU16){
					addstr("<td>");
					addstrl("SFP Link Mode","Режим SFP линка");
					addstr("</td>");
				}

				addstr("</tr>");
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				//table body
				for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
					//port
					addstr("<tr class=\"g2\"><td>");
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
							case 4:addstr("GE#1");break;
							case 5:addstr("GE#2");break;

						}
					}else{
						sprintf(temp,"%d",i+1);
						addstr(temp);
					}
					addstr("</td>");
					//state
					addstr("<td><select name=\"P");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("E\" size=\"1\"");
					addstr("><option");
					if(enable_state[i]==0)
						addstr(" selected");
					addstr(
					" value=\"0\">");
					addstrl("Disable","Выключено");
					addstr("</option>"
					"<option");
					if(enable_state[i]==1)
						addstr(" selected");
					addstr(" value=\"1\">");
					addstrl("Enable","Включено");
					addstr("</option>"
					"</select>"
					"</td>");

					//speed / duplex

					if(is_fiber(i))//for fiber ge ports
						speed[i]=6;

					addstr("<td><select name=\"P");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("S\" size=\"1\">");
					if(is_cooper(i)){
						addstr("<option");
						if(speed[i]==0)
							addstr(" selected");
						addstr(" value=\"0\">");
						addstrl("Auto","Авто");
						addstr("</option>"
						"<option");
						if(speed[i]==1)
							addstr(" selected");
						addstr(" value=\"1\">10M/Half</option>"
						"<option");
						if(speed[i]==2)
							addstr(" selected");
						addstr(" value=\"2\">10M/Full</option>"
						"<option");
						if(speed[i]==3)
							addstr(" selected");
						addstr(	" value=\"3\">100M/Half</option>"
						"<option");
						if(speed[i]==4)
							addstr(" selected");
						addstr(	" value=\"4\">100M/Full</option>");
						if(is_gigabit(i)){
							addstr("<option");
							if(speed[i]==6)
								addstr(" selected");
							addstr(	" value=\"6\">1000M/Full</option>");
						}
					}
					else{
						addstr("<option");
						addstr(	" value=\"6\">1000M/Full</option>");
					}
					addstr(	"</select></td>");
					//flow control
					addstr(	"<td><select name=\"");
					sprintf(temp,"P%dF",i+1);
					addstr(temp);
					addstr("\" size=\"1\"><option");
					if(Flow[i]==0)
						addstr(" selected");
					addstr(	" value=\"0\">");
					addstrl("Disable","Выключено");
					addstr("</option>"
					"<option");
					if(Flow[i]==1)
						addstr(" selected");
					addstr(	" value=\"1\">");
					addstrl("Enable","Включено");
					addstr("</option>"
					"</select>"
					"</td>");

					if(POE_PORT_NUM){
						//poe
						addstr("<td>");
						if(i<POE_PORT_NUM){
							addstr("<select name=\"");
							sprintf(temp,"P%dA",i+1);
							addstr(temp);
							addstr("\" size=\"1\">"
							"<option");
							if(poe[i]==POE_DISABLE)
								addstr(" selected");
							addstr(
							" value=\"0\">");
							addstrl("Disable","Выключено");
							addstr("</option>"

							"<option");
							if(poe[i]==POE_AUTO)
								addstr(" selected");
							addstr(
							" value=\"1\">");
							addstrl("Auto","Авто");
							addstr("</option>");


							if((get_dev_type() == DEV_PSW2GPLUS||get_dev_type() == DEV_PSW2G6F
								||get_dev_type() == DEV_PSW2G2FPLUS || get_dev_type() == DEV_PSW2G2FPLUSUPS)
								&& (i<POEB_PORT_NUM)){

								addstr("<option");
								if(poe[i]==POE_ULTRAPOE)
									addstr(" selected");
								addstr(
								" value=\"3\">");
								addstr("UltraPoE");
								addstr("</option>");

								addstr("<option");
								if(poe[i]==POE_ONLY_A)
									addstr(" selected");
								addstr(
								" value=\"4\">");
								addstrl("Only A","Только A");
								addstr("</option>");

								addstr("<option");
								if(poe[i]==POE_ONLY_B)
									addstr(" selected");
								addstr(
								" value=\"5\">");
								addstrl("Only B","Только B");
								addstr("</option>");

								addstr("<option");
								if(poe[i]==POE_MANUAL_EN)
									addstr(" selected");
								addstr(
								" value=\"2\">");
								addstrl("Passive B","Пассивное PoE B");
								addstr("</option>");

								addstr("<option");
								if(poe[i]==POE_FORCED_AB)
									addstr(" selected");
								addstr(
								" value=\"6\">");
								addstrl("Forced A+B","Форсированное A+B");
								addstr("</option>");

							}
							addstr("</select>");
						}
						addstr("</td>");

					}
		#if POE_LIMIT
					//power limit POE A
					addstr("<td>");
					if(i<POE_PORT_NUM){
						addstr("<input type=\"text\"name=\"");
						sprintf(temp,"P%dL",i+1);
						addstr(temp);
						addstr("\" size=\"3\" maxlength=\"3\"value=\"");
						if(pla[i]){
							sprintf(temp,"%d",(int)pla[i]);
							addstr(temp);
						}
						addstr("\">");
					}
#endif
					addstr("</td>");


					//sfp mode:
					//	*auto
					//	*manual
					if(get_dev_type()!=DEV_SWU16){
						addstr("<td>");
						if(is_fiber(i)){
							addstr("<select name=\"");
							sprintf(temp,"SFP%d",i-COOPER_PORT_NUM+1);
							addstr(temp);
							addstr("\" size=\"1\">"
							"<option");
							if(sfp_mode[i-COOPER_PORT_NUM]==0){
								addstr(" selected");
							}
							addstr(" value=\"0\">Forced</option>"
							"<option");
							if(sfp_mode[i-COOPER_PORT_NUM]==1){
								addstr(" selected");
							}
							addstr(" value=\"1\">Auto</option>"
							"</select>");
						}
						addstr("</td>");
					}
					addstr("</tr>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}

		  addstr("</table>"
		  "<br>"
		  "<input type=\"Submit\" value=\"Apply\" name=\"Apply\"><br><br><br>");
		  addstr("</form></body></html>");
 	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}

static PT_THREAD(run_FEGE_stat(struct httpd_state *s, char *ptr)){
static  uint8_t i=0;
u16 pcount;
static  struct sfp_state_t sfp;
static u8 download,fw_index;
static u8 port, sfp_info;
static u8 back;


	  if(port == 0)
		  SetI2CMode(SFP_NONE);

	  str[0]=0;
	  PSOCK_BEGIN(&s->sout);
		if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
				  int i;
		  		  for(i=0;i<pcount;i++){
		  			static  char *pname,*pval;
		  	  		pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		  	  		pval = http_get_parameter_value(s->param,i,sizeof(s->param));

		  	  		for(u8 j=0;j<ALL_PORT_NUM;j++){
		  	  			sprintf(temp,"sfp%d",j);
		  	  			if(!strcmp(pname, temp)){
			  	  			if (!strncmp(pval, "D", 1)){
			  	  				port = j;
			  	  				sfp_info = 1;
			  	  			}
		  	  			}
		  	  		}
		  	  		if(!strcmp(pname, "fw_index")){
		  	  			fw_index = (u8)strtol(pval,NULL,10);
		  	  		}

		  	  		if(!strcmp(pname, "Download")){
						if (!strncmp(pval, "D", 1)){
							download = 1;
						}
		  	  		}
		  	  		if(!strcmp(pname, "Back")){
						if (!strncmp(pval, "B", 1)){
							back = 1;
							port = 0;
						}
		  	  		}

					if(!strcmp(pname, "Upload")){
						if (!strncmp(pval, "U", 1)){
							tosave();
						}
					}

		  		  }

		  		  if(download){
		  			  if(sfp_reprog(port,fw_index)){
		  				alertl("Error firmware loading","Ошибка загрузки прошивки");
		  			  }
		  			  else{
		  				alertl("Firmware loadede successful","Прошивка загрузилась успешно");
		  			  }
		  			  download = 0;
		  		  }

		  		  if(back){
		  			sfp_info = 0;
		  			back = 0;
		  		  }
		 }


	  addhead();
	  addstr("<body>");
	  addstr("<script type=\"text/javascript\">"
		"function open_port_info(port)"
		"{"
			"window.open('/info/port_info.shtml?port='+port,'','width=1000,height=800,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');"
		"}"
		"</script>");
	  addstrl("<legend>Port Status</legend>","<legend>Информация по портам</legend>");
	  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/info/FEGE_stat.shtml\">");

if(sfp_info != 1){

	 addstr( "<br><br>"

	    		//table header
		  		"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\" align=center>"
				"<td>");
				addstrl("Port","Порт");
				addstr("</td><td>");
				addstrl("State","Состояние");
				addstr("</td><td>");
				addstrl("Link","Линк");
				addstr("</td><td>");
				addstrl("Speed","Скорость");
				addstr("</td><td>");
				addstrl("Duplex","Дуплекс");
				addstr("</td><td>");
				addstrl("Flow Control","Управление потоком");
				addstr("</td>");
				if(POE_PORT_NUM){
					addstr("<td>PoE</td>");
				}
				addstr("<td>SFP</td><td>Port Summary</td></tr>");

				//table body
				for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM) ;i++){
					addstr("<tr class=\"g2\"><td>");
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
							case 4:addstr("GE#1");break;
							case 5:addstr("GE#2");break;
						}
					}else{
						sprintf(temp,"%d",i+1);
						addstr(temp);
					}
					addstr("</td><td>");
					//state
					if(dev.port_stat[i].error_dis){
						addstr("Error-Disable");
					}
					else{
						if (PortStateInfo(L2F_port_conv(i))==1)
							addstrl("Enable","Включено");
						else if (PortStateInfo(L2F_port_conv(i))==2)
							addstrl("Learning","Learning");
						else if (PortStateInfo(L2F_port_conv(i))==3)
							addstrl("Blocked","Blocked");
						else
							addstrl("Disable","Выключено");
					}

					addstr("</td>");
					//link
					addstr("<td>");
					if (get_port_link(i))
						addstr("Up");
					else
						addstr("Down");
					addstr("</td>");
					//speed
					addstr("<td>");
					if(get_port_link(i)){
						switch(PortSpeedInfo(L2F_port_conv(i))){
							case 0:addstr("error");break;
							case 1:addstr("10M");break;
							case 2:addstr("100M");break;
							case 3:addstr("1000M");break;
						}
					}else addstr("-");
					addstr(  "</td>");
					//duplex
					addstr("<td>");
					if(get_port_link(i)){
						if (PortDuplexInfo(L2F_port_conv(i)))
							addstr("Full-duplex");
						else
							addstr("Half-duplex");
					}else
						addstr("-");
					addstr("</td>");
					//flow control
					addstr("<td>");
					if(get_port_link(i)){
						if (PortFlowControlInfo(L2F_port_conv(i)))
							addstrl("Enable","Включено");
						else
							addstrl("Disable","Выключено");
					}else addstr("-");
					addstr(	"</td>");

					if(POE_PORT_NUM){
						//poe
						addstr("<td>");
						if(i>=COOPER_PORT_NUM)
							addstr("-");
						else if (get_port_poe_a(i)||get_port_poe_b(i))
								addstrl("ON","Включено");
						else  addstrl("OFF","Выключено");
						addstr(	"</td>");
					}


					//sfp
					addstr("<td>");
					if(is_fiber(i)){
						addstr("<input type=\"Submit\" ");
						if(get_sfp_present(i)==0)
							addstr(" disabled ");
						addstr("name =\"");
						sprintf(temp,"sfp%d",i);
						addstr(temp);
						addstr("\" value=\"Detail\">");
					}

					addstr("</td><td>"
						"<input type=\"button\" value =\"Port info\" onclick=\"open_port_info(");
					sprintf(temp,"%d",i);
					addstr(temp);
					addstr(");\">");

					addstr("</td></tr>");
				  	PSOCK_SEND_STR(&s->sout,str);
				  	str[0]=0;
				}

				addstr("</table>"
				"<br><br>"
		  		"<br><br>"
		  		"<tr align=\"justify\"><td colspan=2>"
		  		"<input type=\"Submit\" value=\"Refresh\" width=\"200\"></tr></table>");

}
else //отображение инфы об sfp модулях
{
	  sfp_get_info(port,&sfp);
	  addstr("<input type=\"Submit\" name=\"Back\" value=\"Back\"><br><br>");
	  addstr("<table><tr><td>");
	  addstrl("Name","Имя");
	  addstr("</td><td>");
	  /*if(get_dev_type()==DEV_SWU16){
		  sprintf(temp,"SFP %d",GetI2CMode()+1);
	  }
	  else{
		  sprintf(temp,"SFP %d",GetI2CMode()-SFP1+1);
	  }*/
	  sprintf(temp,"SFP %d",port+1);
	  addstr(temp);
	  addstr("</td></tr>"
	  "<tr><td>");
	  addstrl("State","Состояние");
	  addstr("</td><td>");
	  if(sfp.state==1)
		  addstr("Enable");
	  else
		  addstr("Disable");
	  addstr("</td></tr>"
		"<tr><td>");
	  addstrl("Vendor","Производитель");
	  addstr("</td><td>");
		  if(sfp.state==1)
			  addstr(sfp.vendor);
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Vendor OUI","OUI производителя");
		  addstr("</td><td>");
		  if(sfp.state==1)
			  addstr(sfp.OUI);
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Vendor part number","Номер партии");
		  addstr("</td><td>");
		  if(sfp.state==1)
			  addstr(sfp.PN);
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Vendor Rev","Ревизия");
		  addstr("</td><td>");
		  if(sfp.state==1)
			  addstr(sfp.rev);
		  else
			  addstr("-");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Identifier","Идентификатор");
		  addstr("</td><td>");
		  switch(sfp.identifier){
			  case 0x01:addstr("GBIC");break;
			  case 0x03:addstr("SFP");break;
			  default: addstr("-");
		  }
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Connector","Разъем");
		  addstr("</td><td>");
		  switch(sfp.connector){
			  case 0:addstr("Unknown");break;
			  case 0x01:addstr("SC");break;
			  case 0x02:
			  case 0x03:addstr("Fiber Channel copper connector");break;
			  case 0x04:addstr("BNC/TNC");break;
			  case 0x05:addstr("Fiber Channel coaxial headers");break;
			  case 0x06:addstr("FiberJack");break;
			  case 0x07:addstr("LC");break;
			  case 0x08:addstr("MT-RJ");break;
			  case 0x09:addstr("MU");break;
			  case 0x0A:addstr("SG");break;
			  case 0x0B:addstr("Optical pigtail");break;
			  case 0x20:addstr("HSSDC II");break;
			  case 0x21:addstr("Copper Pigtail");break;
			  default: addstr("-");
		  }
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Ethernet Compliance Code","Код");
		  addstr("</td><td>");
		  if(sfp.state)
			  switch(sfp.type){
				  case 0x80:addstr("BASE-PX");break;
				  case 0x40:addstr("BASE-BX10");break;
				  case 0x20:addstr("100BASE-FX");break;
				  case 0x10:addstr("100BASE-LX/LX10");break;
				  case 0x08:addstr("1000BASE-T");break;
				  case 0x04:addstr("1000BASE-CX");break;
				  case 0x02:addstr("1000BASE-LX");break;
				  case 0x00:addstr("1000BASE-SX");break;
				  default: addstr("-");
			  }
			  else
				  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Link Length","Тип линка");
		  addstr("</td><td>");
		  switch(sfp.link_len){
			  case 0x80:addstr("very long distance (V)");break;
			  case 0x40:addstr("short distance (S)");break;
			  case 0x20:addstr("intermediate distance (I)");break;
			  case 0x10:addstr("long distance (L)");break;
			  case 0x08:addstr("medium distance (M)");break;
			  default: addstr("-");
		  }
		  addstr("</td></tr>"
		  "<tr><td>");
		  addstrl("Fibre Channel Technology","Оптическая технология");
		  addstr("</td><td>");
		  if(sfp.state)
			  switch(sfp.fibre_tech){
				  case 0x80:addstr("Electrical intra-enclosure (EL)");break;
				  case 0x40:addstr("Shortwave laser w/o OFC (SN)");break;
				  case 0x20:addstr("Shortwave laser with OFC (SL)");break;
				  case 0x10:addstr("Longwave laser (LL)");break;
				  case 0x04:addstr("Shortwave laser, linear Rx (SA)");break;
				  case 0x02:addstr("Longwave laser (LC)");break;
				  case 0x00:addstr("Electrical inter-enclosure (EL)");break;
				  default: addstr("-");
			  }
		  else
			  addstr("-");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  addstr("</td></tr>"
		  "<tr><td>Fibre Channel Transmission Media</td><td>");
		  if(sfp.state)
			  switch(sfp.media){
				  case 0x80:addstr("Twin Axial Pair (TW)");break;
				  case 0x40:addstr("Twisted Pair (TP)");break;
				  case 0x20:addstr("Miniature Coax (MI)");break;
				  case 0x10:addstr("Video Coax (TV)");break;
				  case 0x08:addstr("Multimode, 62.5um (M6)");break;
				  case 0x04:addstr("Multimode, 50um (M5, M5E)");break;
				  case 0x00:addstr("Single Mode (SM)");break;
				  default: addstr("-");
			  }
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>Fibre Channel Speed</td><td>");
		  if(sfp.state)
			  switch(sfp.speed){
				  case 0x80:addstr("1200 MBytes/sec");break;
				  case 0x40:addstr("800 MBytes/sec");break;
				  case 0x20:addstr("1600 MBytes/sec");break;
				  case 0x10:addstr("400 MBytes/sec");break;
				  case 0x04:addstr("200 MBytes/sec");break;
				  case 0x00:addstr("100 MBytes/sec");break;
				  default: addstr("-");
			  }
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>Nominal bitrate</td><td>");
		  if(sfp.state){
			  sprintf(temp,"%d MBit/sec",sfp.nbr);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>Encoding code</td><td>");
		  if(sfp.state)
			  switch(sfp.encoding){
				  case 0x01:addstr("8B/10B");break;
				  case 0x02:addstr("4B/5B");break;
				  case 0x03:addstr("NRZ");break;
				  case 0x04:addstr("Manchester");break;
				  case 0x05:addstr("SONET Scrambled");break;
				  case 0x06:addstr("64B/66B");break;
				  default: addstr("-");
			  }
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>TX Laser Wavelength </td><td>");
		  if(sfp.state){
			  sprintf(temp,"%d",sfp.wavelen);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>Link Length(for 9µm fiber)</td><td>");
		  if(sfp.state){
			  sprintf(temp,"%dm",sfp.len9);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>"
		  "<tr><td>Link Length(for 50µm fiber)</td><td>");
		  if(sfp.state){
			  sprintf(temp,"%dm",sfp.len50);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>"

		  "<tr><td>Link Length(for 62.5µm fiber)</td><td>");
		  if(sfp.state){
			  sprintf(temp,"%dm",sfp.len62);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>"

		  "<tr><td>Link Length(for cooper)</td><td>");
		  if(sfp.state){
			  sprintf(temp,"%dm",sfp.lenc);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>"

		  "<tr><td>Diagnostinc Monitoring Type</td><td>");
		  if(sfp.state && sfp.dm_type){
			  sprintf(temp,"%d",sfp.dm_type);
			  addstr(temp);
		  }
		  else
			  addstr("-");
		  addstr("</td></tr>");

		  if(sfp.dm_type){
			  //temperature
			  addstr("<tr><td>Temperature</td><td>");
			  if(sfp.state){
				  if(sfp.dm_temper>=0)
					  sprintf(temp,"+%d °C",(sfp.dm_temper/256)&0x7F);
				  else
					  sprintf(temp,"-%d °C",(sfp.dm_temper/256)&0x7F);
				  addstr(temp);
			  }
			  else
				  addstr("-");
			  addstr("</td></tr>");
			  //voltage
			  addstr("<tr><td>Supply Voltage</td><td>");
			  if(sfp.state){
				  sprintf(temp,"%d.%d V",((sfp.dm_voltage)/10000),((sfp.dm_voltage)/1000)%10);
				  addstr(temp);
			  }
			  else
				  addstr("-");
			  addstr("</td></tr>");
			  //current
			  addstr("<tr><td>TX bias current</td><td>");
			  if(sfp.state){
				  sprintf(temp,"%d.%d mA",((sfp.dm_current)/10000),((sfp.dm_current)/1000)%10);
				  addstr(temp);
			  }
			  else
				  addstr("-");
			  addstr("</td></tr>");
			  //tx power
			  if(sfp.state){
			  addstr("<tr><td>TX output optical power</td><td>");
				  sprintf(temp,"%d.%d mW",((sfp.dm_txpwr)/10000),((sfp.dm_txpwr)/1000)%10);
				  addstr(temp);
			  }
			  else
				  addstr("-");
			  addstr("</td></tr>");
			  //rx power
			  addstr("<tr><td>RX recieved optical power</td><td>");
			  if(sfp.state){
				  sprintf(temp,"%d.%d mW",((sfp.dm_rxpwr)/10000),((sfp.dm_rxpwr)/1000)%10);
				  addstr(temp);
			  }
			  else
				  addstr("-");
			  addstr("</td></tr>");
		  }

		  addstr("</table><br>");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;




		  addstrl("<legend>SFP Reprogramming</legend>","<legend>Перепрограммирование SFP</legend>");
		  addstr("<br><br>");
		  addstrl("<b>1. Firmware Dump</b>","<b>Сохранение прошивки SFP</b>");
		  addstr("<br>"
		  "<input type=\"button\" class=\"button\" value=\"SFP Dump\" onclick=\"location.href='/sfp_dump.bin'\"/>");

		  addstr("<br><br>");
		  addstrl("<b>2. Preloaded Firmware Download</b>","<b>2. Загрузка предустановленной прошивки</b>");
		  addstr("<br>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		  addstr("<tr class=\"g2\"><td>Preloaded Firmwares</td><td></td></tr>"
		  "<tr class=\"g2\"><td>"
		  "<select name=\"fw_index\" size=\"1\">");
		  for(u8 i=0;i<get_sfp_fw_num();i++){
				addstr("<option value=\"");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\">");
				get_sfp_fw_name(i,temp);
				addstr(temp);
				addstr("</option>");
		  }
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  addstr("</select>");
		  addstr("</td><td><input type=\"Submit\" name=\"Download\" value=\"Download\"></td></tr></table>");

		  addstr("<br><br>");
		  addstrl("<b>3. Custom Firmware Download</b>","<b>2. Загрузка сторонней прошивки</b>");
		  addstr("<br>"
		     	 "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		  addstr("<tr class=\"g2\"><td>Custom Firmware</td><td></td></tr>"
		 		  "<tr class=\"g2\"><td>"
				  "<input type=\"file\"name=\"updatefile\"size=\"24\" value=\"\"></td>"
				  "<td><input type=\"Submit\" value=\"Upload\" name=\"Upload\"></td></tr></table>"
				  "<br><br><br><br>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  if (update_state!=UPDATE_STATE_IDLE){
	  		  s->parse_state = 0;
	  		  update_state=UPDATE_STATE_IDLE;
			  if (update_state==UPDATE_STATE_UPLOADED){
				  if(sfp_reprog_file(GetI2CMode())){
					 alertl("Error firmware loading","Ошибка загрузки прошивки");
				  }
				  else{
					 alertl("Firmware loadede successful","Прошивка загрузилась успешно");
				  }
			  }
	  	  }

}

		  addstr("</form></body></html>");



		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  PSOCK_END(&s->sout);
}


static PT_THREAD(run_VCT(struct httpd_state *s, char *ptr)){
	    uint16_t pcount;
	    static  u8 port,i;
	    static  u8 Start,Calibrate,ActualLength;

	    PSOCK_BEGIN(&s->sout);

	    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
			  for(i=0;i<pcount;i++){
					char *pname,*pval,*End;
					pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					if(!strcmp(pname, "set1")){
						if (!strncmp(pval, "S", 1)) 	{Calibrate=1; port=0;}
					}
					if(!strcmp(pname, "set2")){
						if (!strncmp(pval, "S", 1)) 	{Calibrate=1; port=1;}
					}
					if(!strcmp(pname, "set3")){
						if (!strncmp(pval, "S", 1)) 	{Calibrate=1; port=2;}
					}

					if(!strcmp(pname, "set4")){
						if (!strncmp(pval, "S", 1)) 	{Calibrate=1; port=3;}
					}

					if(!strcmp(pname, "set5")){
						if (!strncmp(pval, "S", 1)) 	{Calibrate=1; port=4;}
					}

					if(!strcmp(pname, "set6")){
						if (!strncmp(pval, "S", 1)) 	{Calibrate=1; port=5;}
					}


					if(!strcmp(pname, "test1")){
						if (!strncmp(pval, "T", 1)) 	{Start=1; port=0;}
					}
					if(!strcmp(pname, "test2")){
						if (!strncmp(pval, "T", 1)) 	{Start=1; port=1;}
					}
					if(!strcmp(pname, "test3")){
						if (!strncmp(pval, "T", 1)) 	{Start=1; port=2;}
					}
					if(!strcmp(pname, "test4")){
						if (!strncmp(pval, "T", 1)) 	{Start=1; port=3;}
					}
					if(!strcmp(pname, "test5")){
						if (!strncmp(pval, "T", 1)) 	{Start=1; port=4;}
					}
					if(!strcmp(pname, "test6")){
						if (!strncmp(pval, "T", 1)) 	{Start=1; port=5;}
					}

					if(!strcmp(pname, "length1")){
						if(port == 0)
							ActualLength=strtol(pval,&End,10);
					}
					if(!strcmp(pname, "length2")){
						if(port == 1)
							ActualLength=strtol(pval,&End,10);
					}
					if(!strcmp(pname, "length3")){
						if(port == 2)
							ActualLength=strtol(pval,&End,10);
					}
					if(!strcmp(pname, "length4")){
						if(port == 3)
							ActualLength=strtol(pval,&End,10);
					}
					if(!strcmp(pname, "length5")){
						if(port == 4)
							ActualLength=strtol(pval,&End,10);
					}
					if(!strcmp(pname, "length6")){
						if(port == 5)
							ActualLength=strtol(pval,&End,10);
					}
			}

			if(Calibrate == 1){
				set_cable_test(VCT_CALLIBRATE,port,ActualLength);
			}
			if(Start == 1){
				set_cable_test(VCT_TEST,port,0);
			}
	    }


	      addhead();
	      addstr("<body>");
	      if(/*(get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)*/0){
	    	  //в psw-1g пока не подерживается кабельный тестер
	    	  addstr("This feature is not active in this firmvare version");
	      }
	      else{
			  addstr("<script type = \"text/javascript\">"
					  "var TimeoutID_;"
					  "var Time_;"
					  "var str;"
					  "var formid;"
					  "function form() {"
						"str+=\" .\";"
						"return(str);"
					  "}"
					  "function show(){"
						"document.getElementById(formid).innerHTML = 'Testing'+form();"
						"Time_++;"
						"window.setTimeout('show();',1000);"
					  "}"
					  "function start(id){ "
						"Time_=0;"
						"str='';"
						"formid=id;"
						"TimeoutID_=window.setTimeout('show();', 1000);"
					  "}"
			  "</script>");

			  PSOCK_SEND_STR(&s->sout,str);
			  str[0]=0;

			  addstrl("<legend>Virtual Cable Tester</legend>","<legend>Кабельный тестер</legend>");
			  addstr(
			  "<form method=\"post\" bgcolor=\"#808080 \"action=\"/tools/VCT.shtml\">");

			  //dot lines on Test buttons
			  if(Start == 1){
				  addstr("<script type=\"text/javascript\">"
				  "start('");
				  sprintf(temp,"t%d",port+1);// zB "t1"
				  addstr(temp);
				  addstr("')</script>");
				  addstr("<meta http-equiv=\"refresh\" content=\"5; url=\"/action=\"/tools/VCT.shtml\">");
				  Start = 0;
			  }

			  //dot lines on Set buttons
			  if(Calibrate == 1){
				  addstr("<script type=\"text/javascript\">"
				  "start('");
				  sprintf(temp,"s%d",port+1);// zB "s1"
				  addstr(temp);
				  addstr("')</script>");
				  addstr("<meta http-equiv=\"refresh\" content=\"5; url=\"/action=\"/tools/VCT.shtml\">");
				  Start = 0;
				  Calibrate = 0;
			  }

			  addstr(
				 "<BLOCKQUOTE><b>");
				  addstrl("Calibrate","Калибровка");
				  addstr("</b></BLOCKQUOTE>"
				 "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" >"

				 "<tr class=\"g2\"><td width=\"20%\">");
				 addstrl("Port","Порт");
				 addstr("</td><td width=\"200\">");
				 addstrl("Actual distance","Длина кабеля");
				 addstr("</td><td width=\"120\"></td></tr>");

				 //table body
				 for(i=0;i<COOPER_PORT_NUM;i++){
					addstr("<tr class=\"g2\"><td>");
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
							case 4:addstr("GE#1");break;
							case 5:addstr("GE#2");break;
						}
					}
					else if(get_dev_type() == DEV_SWU16){
						sprintf(temp,"%d",i+1+FIBER_PORT_NUM);
						addstr(temp);
					}
					else{
						sprintf(temp,"%d",i+1);
						addstr(temp);
					}
					addstr("</td>");
					//distance
					addstr("<td><input type=\"text\"name=\"length");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\"size=\"5\" maxlength=\"3\"value=\"");
					if(get_callibrate_len(i)){
						sprintf(temp,"%d",(int)get_callibrate_len(i));
						addstr(temp);
					}
					addstr("\"></td>");
					//set
					addstr("<td>"
					"<span id = \"s");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\">"
					"<input type=\"Submit\" name =\"set");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\" value=\"Set\"></span></td></tr>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				 }

				 addstr("</table><br>");

	//*********      таблица результатов тестирования            **************/
				 addstr(
				 "<br>"
				 "<BLOCKQUOTE><b>");
				 addstrl("Diagnostic","Тестирование");
				 addstr("</b></BLOCKQUOTE>"
				 "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				 "<tr class=\"g2\" ><td width=\"20%\" rowspan=\"2\">");//colspan=\"2\" //rowspan=\"2\"
				 addstrl("Port","Порт");
				 if(get_dev_type()==DEV_SWU16)
					 addstr("</td><td colspan=\"4\">");
				 else
					 addstr("</td><td colspan=\"2\">");
				 addstrl("Distance","Дистанция");
				 if(get_dev_type()==DEV_SWU16)
					 addstr("</td><td colspan=\"4\">");
				 else
					 addstr("</td><td colspan=\"2\">");
				 addstrl("Status","Статус");
				 addstr("</td><td rowspan=\"2\" width=\"120\"></td></tr>");

				 if(get_dev_type()==DEV_SWU16){
					 addstr("<tr class=\"g2\"><td>");
					 addstrl("Pair","Пара");
					 addstr(" 1-2</td><td>");
					 addstrl("Pair","Пара");
					 addstr(" 3-6</td>");
					 addstr("<td>");
					 addstrl("Pair","Пара");
					 addstr(" 4-5</td>");
					 addstr("<td>");
					 addstrl("Pair","Пара");
					 addstr(" 7-8</td>");
					 addstr("<td>");
					 addstrl("Pair","Пара");
					 addstr(" 1-2</td><td>");
					 addstrl("Pair","Пара");
					 addstr(" 3-6</td>");
					 addstr("<td>");
					 addstrl("Pair","Пара");
					 addstr(" 4-5</td>");
					 addstr("<td>");
					 addstrl("Pair","Пара");
					 addstr(" 7-8</td>");
					 addstr("</tr>");
				 }
				 else{
					 addstr("<tr class=\"g2\"><td>");
					 addstrl("Pair","Пара");
					 addstr(" 1-2</td><td>");
					 addstrl("Pair","Пара");
					 addstr(" 3-6</td>");
					 addstr("<td>");
					 addstrl("Pair","Пара");
					 addstr(" 1-2</td><td>");
					 addstrl("Pair","Пара");
					 addstr(" 3-6</td></tr>");
				 }



				 for(i=0;i<COOPER_PORT_NUM;i++){
					addstr("<tr class=\"g2\"><td>");
					//ports
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
							case 4:addstr("GE#1");break;
							case 5:addstr("GE#2");break;
						}
					}
					else if(get_dev_type() == DEV_SWU16){
						sprintf(temp,"%d",i+1+FIBER_PORT_NUM);
						addstr(temp);
					}
					else{
						sprintf(temp,"%d",i+1);
						addstr(temp);
					}
					addstr("</td>");
					if(get_dev_type() == DEV_SWU16){
						if(port==i){
							//pair 0-4 distance
							for(u8 j=0;j<4;j++){
								addstr("<td>");
								if((dev.port_stat[port].vct_status[j] == VCT_BAD)||
								  (dev.port_stat[port].vct_status[j] == VCT_GOOD)||
								  (dev.port_stat[port].vct_compleat_ok==0))
								{
									addstr("--");
								}
								else{
									if((get_callibrate_koef_1(port)<70)||(get_callibrate_koef_1(port)>150))
										set_callibrate_koef_1(port,100);
									sprintf(temp, "%d.%02d", (int)(dev.port_stat[port].vct_len[j]/100),(int)(dev.port_stat[port].vct_len[j]%100));
									addstr(temp);
								}
								addstr("</td>");
							}

							//pair 0-4 status
							for(u8 j=0;j<4;j++){
								addstr("<td>");
								if((port==i) && (dev.port_stat[port].vct_compleat_ok==1)){
									switch(dev.port_stat[port].vct_status[j]){
										case VCT_BAD:  addstrl("Measurement error","Ошибка измерения");break;
										case VCT_SHORT:addstrl("Short","Замыкание");break;
										case VCT_OPEN: addstrl("Open","Обрыв");break;
										case VCT_GOOD: addstrl("Good","Нет повреждений"); break;
										case VCT_SAME_PAIR_SHORT: addstrl("Same Pair Short","Замыкание пары");break;
										case VCT_CROSS_PAIR_SHORT: addstrl("Cross Pair Short","Замыкание с соседней парой");break;
									}
								}
								else
									addstr("---");
								addstr("</td>");
							}
							dev.port_stat[port].vct_compleat_ok = 0;
						}
						else{
							 addstr("<td>--</td><td>--</td><td>--</td><td>--</td><td>--</td><td>--</td>"
									 "<td>--</td><td>--</td>");
						}
						addstr("<td>"
						"<span id = \"t");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\"><input type=\"Submit\" name =\"test");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\" value=\"Test\"></span></td></tr>");
						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
					}
					else
					{
						//pair 1-2
						addstr("<td>");
						if(port==i){
							if((dev.port_stat[port].rx_status == VCT_BAD)||(dev.port_stat[port].rx_status == VCT_GOOD)||(dev.port_stat[port].vct_compleat_ok==0))
							{
								addstr("--");
							}
							else
							{
								if((get_callibrate_koef_1(port)<70)||(get_callibrate_koef_1(port)>150))
									set_callibrate_koef_1(port,100);
								sprintf(temp, "%d", (int)((dev.port_stat[port].rx_len)*get_callibrate_koef_1(port))/100);
								addstr(temp);
							}
						 }else{
							 addstr("--");
						 }
						 addstr("/");
						 if((get_callibrate_len(i)>0)&&(get_callibrate_len(i)<201)){
							 sprintf(temp,"%d",(int)get_callibrate_len(i));
							 addstr(temp);
						 }else{
							 addstr("--");
						 }
						 addstr("</td>");
						 //pair 3-6
						 addstr("<td>");
						 if(port==i){
							 //TX
							if((dev.port_stat[port].tx_status == VCT_BAD)||(dev.port_stat[port].tx_status == VCT_GOOD)||(dev.port_stat[port].vct_compleat_ok==0))
							{
								addstr("--");
							}
							else
							{
								if((get_callibrate_koef_2(port)<70)||(get_callibrate_koef_2(port)>150))
									set_callibrate_koef_2(port,100);
								sprintf(temp,"%d", (int)((dev.port_stat[i].tx_len)*get_callibrate_koef_2(port))/100);
								addstr(temp);
							}
						 }else{
							 addstr("--");
						 }
						 addstr("/");
						 if((get_callibrate_len(i)>0)&&(get_callibrate_len(i)<201)){
							 sprintf(temp,"%d",(int)get_callibrate_len(i));
							 addstr(temp);
						 }else{
							 addstr("--");
						 }
						 addstr("</td>");

						 //status
						 //pair 1-2
						addstr("<td>");
						if((port==i) && (dev.port_stat[port].vct_compleat_ok==1)){
							switch(dev.port_stat[port].rx_status){
								case VCT_BAD:  addstrl("Measurement error","Ошибка измерения");break;
								case VCT_SHORT:addstrl("Short","Замыкание");break;
								case VCT_OPEN: addstrl("Open","Обрыв");break;
								case VCT_GOOD: addstrl("Good","Нет повреждений"); break;
							}
						}
						else
							addstr("---");
						addstr("</td>");

						//pair 3-6
						addstr("<td>");
						if((port==i) && (dev.port_stat[port].vct_compleat_ok==1)){
							switch(dev.port_stat[port].tx_status){
								case VCT_BAD:  addstrl("Measurement error","Ошибка измерения");break;
								case VCT_SHORT:addstrl("Short","Замыкание");break;
								case VCT_OPEN: addstrl("Open","Обрыв");break;
								case VCT_GOOD: addstrl("Good","Нет повреждений");break;
							}
						}
						else
							addstr("---");
						addstr("</td><td>"
						"<span id = \"t");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\"><input type=\"Submit\" name =\"test");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\" value=\"Test\"></span></td></tr>");
						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
					}
				}
				addstr("</table>");
				addstr("</form></body></html>");
	    }
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
	PSOCK_END(&s->sout);
}



static PT_THREAD(run_Descr(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
uint16_t pcount;
static u8 j;
u8 apply=0;
char text_tmp[128];
char name_desc[128];
char cont_desc[128];
char loc_desc[128];
char port_descr[PORT_NUM][PORT_DESCR_LEN];

	  PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

			  for(int i=0;i<pcount;i++){
				  char *pname,*pval;
				  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
				  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

				  if(!strcmp(pname, "name"))
				  {
					  memset(name_desc,0,128);
					  if(strlen(pval)<128)
						  strcpy(name_desc,pval);

				  }
				  if(!strcmp(pname, "location"))
				  {
					  memset(loc_desc,0,128);
					  if(strlen(pval)<128)
						  strcpy(loc_desc,pval);
			  	  }
				  if(!strcmp(pname, "contact"))
				  {
					  memset(cont_desc,0,128);
					  if(strlen(pval)<128)
						  strcpy(cont_desc,pval);
				  }

				  for(j=0;j<ALL_PORT_NUM;j++){
					  sprintf(temp,"pd%d",j+1);
					  if(!strcmp(pname, temp)){
						  if(strlen(pval)<PORT_DESCR_LEN){
							  strcpy(port_descr[j],pval);
						  }
					  }
				  }

				  if(!strcmp(pname, "Apply")){
					  if(!strcmp(pval, "Apply")){
						  apply = 1;
					  }
				  }
			  }



			  if(apply){
				  set_interface_name(name_desc);
				  set_interface_location(loc_desc);
				  set_interface_contact(cont_desc);
				  for(j=0;j<ALL_PORT_NUM;j++){
					  set_port_descr(j,port_descr[j]);
				  }
				  settings_save();

				  alertl("Parameters accepted","Настройки применились");
			  }
	  }


	  get_interface_name(name_desc);
	  get_interface_location(loc_desc);
	  get_interface_contact(cont_desc);
	  name_desc[127]=0;
	  loc_desc[127]=0;
	  cont_desc[127]=0;


	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Device Description</legend>","<legend>Описание устройства</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{

		  addstr("<form method=\"post\" bgcolor=\"#808080\""
			  "action=\"/info/description.shtml\">");

		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\"><tr class=\"g2\">"
		/************  Device Name *******/
				  "<td width=\"150\">");
				  addstrl("Device Name","Имя устройства");
				  addstr("</td><td><input type=\"text\" name=\"name\" size=\"60\" maxlength=\"60\"value=\"");

	memset(text_tmp,0,sizeof(text_tmp));
	http_url_decode(name_desc,text_tmp,strlen(name_desc));
	for(uint8_t i=0;i<strlen(text_tmp);i++){
		if(text_tmp[i]=='+') text_tmp[i] = ' ';
		if(text_tmp[i]=='%') text_tmp[i]=' ';
	}
	addstr(text_tmp);
	/************** Device Location **********/
	addstr("\"></td></tr><tr class=\"g2\"><td>");
			  addstrl("Device Location","Местоположение устройства");

		  addstr( "</td>"
				  "<td><input type=\"text\" name=\"location\" size=\"60\" maxlength=\"60\"value=\"");


	memset(text_tmp,0,sizeof(text_tmp));
	http_url_decode(loc_desc,text_tmp,strlen(loc_desc));
	for(uint8_t i=0;i<strlen(text_tmp);i++){
		if(text_tmp[i]=='+') text_tmp[i] = ' ';
		if(text_tmp[i]=='%') text_tmp[i]=' ';
	}
	addstr(text_tmp);
	/************** Device Contact **********/
		addstr(
				  "\"></td></tr><tr class=\"g2\"><td>");
	  addstrl("Service Company","Обслуживающая организация");

		  addstr("</td><td><input type=\"text\" name=\"contact\" size=\"60\" maxlength=\"60\"value=\"");


	memset(text_tmp,0,sizeof(text_tmp));
	http_url_decode(cont_desc,text_tmp,strlen(cont_desc));
	for(uint8_t i=0;i<strlen(text_tmp);i++){
		if(text_tmp[i]=='+') text_tmp[i] = ' ';
		if(text_tmp[i]=='%') text_tmp[i]=' ';
	}
	addstr(text_tmp);

	addstr("\"></td></tr></table>");


	addstr("<br><br>");
	addstrl("<b>Port Description</b>","<b>Описание портов</b>");

	addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
	"<tr class=\"g2\"><td>");
	addstrl("Port","Порт");
	addstr("</td><td>");
	addstrl("Description","Описание");
	addstr("</td></tr>");
	for(j=0;j<ALL_PORT_NUM;j++){

		addstr("<tr class=\"g2\"><td>");
		sprintf(temp,"%d",j+1);
		addstr(temp);
		addstr("</td><td><input type=\"text\" name=\"pd");
		addstr(temp);
		addstr("\" size=\"32\" maxlength=\"32\"value=\"");
		get_port_descr(j,port_descr[j]);
		memset(text_tmp,0,sizeof(text_tmp));
		http_url_decode(port_descr[j],text_tmp,strlen(port_descr[j]));
		for(uint8_t i=0;i<strlen(text_tmp);i++){
			if(text_tmp[i]=='+') text_tmp[i] = ' ';
			if(text_tmp[i]=='%') text_tmp[i]=' ';
		}
		addstr(text_tmp);
		addstr("\"></td></tr>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
	}
	addstr("</table>");
	addstr( "<br><input type=\"Submit\" name = \"Apply\" value=\"Apply\"></form></body></html>");

	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}



static  PT_THREAD(run_FactoryDefaults(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	  /*static*/  uint16_t pcount;
	  extern const uint32_t image_version[1];
	  static  int keepPass=0,keepNtwk=0,Def_=0,keepSTP=0;
	  u8 flag=0;
	PSOCK_BEGIN(&s->sout);

	str[0]=0;
		  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

			  int i=0;
			  for(i=0;i<pcount;i++){
		  		char *pname,*pval;
		  		pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		  		pval = http_get_parameter_value(s->param,i,sizeof(s->param));
		  		Def_ = 0;
		  		if(!strcmp(pname, "Default")){
		  			if (!strncmp(pval, "Def", 3)) Def_ = 1;
		  		}
		  		if(!strcmp(pname, "kN")){
		  			if (!strncmp(pval, "1", 1)) keepNtwk = 1;
		  			else keepNtwk = 0;
		  		}
		  		if(!strcmp(pname, "kP")){
		  			if (!strncmp(pval, "1", 1)) keepPass = 1;
		  			else keepPass = 0;
		  		}
		  		if(!strcmp(pname, "kS")){
		  			if (!strncmp(pval, "1", 1)) keepSTP = 1;
		  			else keepSTP = 0;
		  		}
		  	}

				if(Def_){
					flag = DEFAULT_ALL;
					if(keepNtwk)	flag |= DEFAULT_KEEP_NTWK;
					if(keepPass)	flag |= DEFAULT_KEEP_PASS;
					if(keepSTP)		flag |= DEFAULT_KEEP_STP;
					settings_default(flag);
					alertl("Defaut accepted","Настройки сброшены");
					reboot(REBOOT_ALL);
				}
		  }

		addhead();
		addstr("<body>");
		addstrl("<legend>Factory Default</legend>","<legend>Сброс настроек</legend>");
		if(get_current_user_rule()!=ADMIN_RULE){
			  addstr("Access denied");
		}else{
			addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
				"action=\"/default.shtml\"><br>");

			addstr( "<table><tr><td>"
			"<input type=checkbox name=\"kN\" value=\"1\" tabindex = \"1\"></td><td>");
			addstrl("Keep current network settings","Сохранить сетевые настройки");

			addstr(" </td></tr>"
			"<tr><td><input type=checkbox name=\"kP\" value=\"1\"></td><td>");
			addstrl("Keep current username & password","Сохранить настройки учетной записи");

			addstr(" </td></tr>"
			"<tr><td><input type=checkbox name=\"kS\" value=\"1\"></td><td>");
			addstrl("Keep STP settings","Сохранить настройки STP");
			addstr("</td></tr></table><br><br><input type=\"Submit\" name = \"Default\" value=\"Default\">");

			addstr("<tr align=\"justify\"><td colspan=2></form></body></html>");
		}
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		PSOCK_END(&s->sout);
}

static PT_THREAD(run_SaveSettings(struct httpd_state *s, char *ptr)){
	  static  int save=0;
	  static char alert_ru[32];
	  static char alert_en[32];
	  u8 pcount;
	  u32 ret;
	  PSOCK_BEGIN(&s->sout);
	  tosave();
		  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
			  int i;
			  for(i=0;i<pcount;i++){
				char *pname,*pval;
				pname = http_get_parameter_name(s->param,i,sizeof(s->param));
				pval = http_get_parameter_value(s->param,i,sizeof(s->param));
				if(!strcmp(pname, "Upload")){
					if (!strncmp(pval, "U", 1)) tosave();
				}

		  		if(!strcmp(pname, "save")){
		  			if (!strncmp(pval, "C", 1)){
		  				save = 1;
		  			}
		  		}

			  }

			  if(save == 1){
				  backup_file_len = make_bak();
			  }
		  }
					  str[0]=0;
					  addhead();
					  addstr("<body>");
					  addstrl("<legend>Backup/Recovery</legend>","<legend>Восстановление и резервное копирование настроек</legend>");
					  if(get_current_user_rule()!=ADMIN_RULE){
						  addstr("Access denied");
					  }
					  else{
						  addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
						  "action=\"/savesett.shtml\"><br>"
						  "&nbsp;&nbsp;<b>1. ");
						  addstrl("Backup settings","Резервное копирование настроек");
						  addstr("</b><br><br>"
						  "&nbsp;");
						  addstrl("Download users settings as file:","Сохранение настроек в файл");
						  addstr("<br><br>");
						  //make file
						  if(save==0){
							  addstr("<input type=\"Submit\" name=\"save\" value=\"Create a file\"><br>");
						  }
						  else if(save==1){
							  addstr( "<input type=\"button\" class=\"button\" "
							  "value=\"Download a file\" onclick=\"location.href='/PSW_settings_backup.bak'\" />");
						  }

						  addstr("<br><br><br>"
						  "&nbsp;&nbsp;<b>2. ");
						  addstrl("Recovery settings","Восстановление настроек");
						  addstr("</b><br><br>");
						  addstrl("Upload settings file","Загрузка настроек из файла");
						  addstr("<br><input type=\"file\"name=\"updatefile\"size=\"24\" value=\"\">"
						  "<input type=\"Submit\" value=\"Upload\" name=\"Upload\">"
						  "</form>");
					  }

		  			if (update_state!=UPDATE_STATE_IDLE){
		  	  		if(!save){
						  s->parse_state = 0;
						  if (update_state==UPDATE_STATE_UPLOADED){
							  ret=parse_bak_file();
							  if(ret){
								  sprintf(alert_ru,"Ошибка файла (%lu)",ret);
								  sprintf(alert_en,"File Error(%lu)",ret);
								  alertl(alert_en,alert_ru);
								  settings_load();
								  nosave();
							  }
							  else{
								  alertl("Write successful and reboot...", "Успешно, перезагрузка..");
								  settings_save();
								  timer_set(&ntwk_timer, 5000*MSEC);
								  ntwk_wait_and_do = 3;//reboot
							  }
						  }
		  	  		}
		  	  }
		  addstr("</body><html>");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  save=0;
		  PSOCK_END(&s->sout);
	}

static PT_THREAD(run_ComfortStart(struct httpd_state *s, char *ptr)){
  static  uint16_t pcount;
  static uint16_t SSC1,SSC2,SSC3,SSC4,SSC5,SSC6,SSTimeOut;
  static  uint8_t poe1,poe2,poe3,poe4,poe5,poe6;
  static  uint8_t apply,i=0;
  RTC_TimeTypeDef RTC_Time;
  RTC_DateTypeDef RTC_Date;

  PSOCK_BEGIN(&s->sout);
	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  SSC1=SSC2=SSC3=SSC4=SSC5=SSC6=0;
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			/***************************/
			if(!strcmp(pname, "SS1")){
				if (!strncmp(pval, "1", 1)) SSC1 = 1;
			}
			if(!strcmp(pname, "SS2")){
				if (!strncmp(pval, "1", 1)) SSC2 = 1;
			}
			if(!strcmp(pname, "SS3")){
				if (!strncmp(pval, "1", 1)) SSC3 = 1;
			}
			if(!strcmp(pname, "SS4")){
				if (!strncmp(pval, "1", 1)) SSC4 = 1;
			}
			if(!strcmp(pname, "SS5")){
				if (!strncmp(pval, "1", 1)) SSC5 = 1;
			}
			if(!strcmp(pname, "SS6")){
				if (!strncmp(pval, "1", 1)) SSC6 = 1;
			}

			if(!strcmp(pname, "SST")){
				if (!strncmp(pval, "1", 1)) SSTimeOut=1;
				if (!strncmp(pval, "2", 1)) SSTimeOut=2;
			}

			/***************************/
			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}
			/***************************/
			if(!strcmp(pname, "poe1")){
				if (!strncmp(pval, "S", 1)) poe1=1;
			}
			if(!strcmp(pname, "poe2")){
				if (!strncmp(pval, "S", 1)) poe2=1;
			}
			if(!strcmp(pname, "poe3")){
				if (!strncmp(pval, "S", 1)) poe3=1;
			}
			if(!strcmp(pname, "poe4")){
				if (!strncmp(pval, "S", 1)) poe4=1;
			}
			if(!strcmp(pname, "poe5")){
				if (!strncmp(pval, "S", 1)) poe5=1;
			}
			if(!strcmp(pname, "poe6")){
				if (!strncmp(pval, "S", 1)) poe6=1;
			}

	  }

		  if(apply==1){
		  	  set_port_soft_start(0,SSC1);
		  	  set_port_soft_start(1,SSC2);
		  	  set_port_soft_start(2,SSC3);
		  	  set_port_soft_start(3,SSC4);
		  	  set_port_soft_start(4,SSC5);//for PSW-2G6F
		  	  set_port_soft_start(5,SSC6);//for PSW-2G6F
		  	  set_softstart_time(SSTimeOut);

			  settings_save();
			  alertl("Parameters accepted","Настройки применились");

		  }

		  if(poe1){
			  dev.port_stat[0].ss_process = 0;
			  set_poe_init(0);
			  poe1=0;
		  }
		  if(poe2){
			  dev.port_stat[1].ss_process = 0;
			  set_poe_init(0);
			  poe2=0;
		  }
		  if(poe3){
			  dev.port_stat[2].ss_process = 0;
			  set_poe_init(0);
			  poe3=0;
		  }
		  if(poe4){
			  dev.port_stat[3].ss_process = 0;
			  set_poe_init(0);
			  poe4=0;
		  }
		  if(poe5){
			  dev.port_stat[4].ss_process = 0;
			  set_poe_init(0);
			  poe5=0;
		  }
		  if(poe6){
			  dev.port_stat[5].ss_process = 0;
			  set_poe_init(0);
			  poe6=0;
		  }
	  }


	  	  SSC1 = get_port_sett_soft_start(0);
	  	  SSC2 = get_port_sett_soft_start(1);
	  	  SSC3 = get_port_sett_soft_start(2);
	  	  SSC4 = get_port_sett_soft_start(3);
	  	  SSC5 = get_port_sett_soft_start(4);
	  	  SSC6 = get_port_sett_soft_start(5);
	  	  SSTimeOut = get_softstart_time();

	  	  if(POE_PORT_NUM == 0){
	  		  addstrl("This feature is not activated in the device","Эта функция недоступна на этом устройстве");
	  	  }
	  	  else{
			  addhead();
			  addstr("<body>");
			  addstrl("<legend>Comfort Start</legend>","<legend>Плавный старт</legend>");
			  addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
					"action=\"/settings/ComfortStart.shtml\">");

			  if(get_current_user_rule()==ADMIN_RULE){
				  addstr("<br><table width=\"30%\" cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" >"
							"<tr class=\"g2\"><td width=\"120\">");

				  addstrl("Soft start time","Длительность плавного старта");
				  addstr("</td>"
							"<td><select name=\"SST\" size=\"1\">"
							"<option");
				  if(get_softstart_time()==1)
						addstr(" selected");
				  addstr(
					" value=\"1\">1 ");
				  addstrl("Hour","Час");
				  addstr("</option>"
					"<option");
				  if(get_softstart_time()==2)
						addstr(" selected");
				  addstr(" value=\"2\">2 ");
				  addstrl("Hours","Часа");
				  addstr("</option>"
					"</select></td></tr></table>"
					"<br><br>");
			  }

				addstr("<table width=\"65%\" cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" >"
				"<tr class=\"g2\"><td>");

				//table header
				addstrl("Port","Порт");
				addstr("</td><td>");
				if(get_current_user_rule()==ADMIN_RULE){
					addstrl("Comfort start","Плавный старт");
					addstr("</td><td>");
				}

				addstrl("PoE status","Статус PoE");
				addstr("</td><td>");
				addstrl("Manual start","Принудительный старт");
				addstr("</td></tr>");

				//table body
				for(i=0;i<POE_PORT_NUM;i++){
					addstr("<tr class=\"g2\"><td>");
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
						}
					}else{
						sprintf(temp,"%d",i+1);
						addstr(temp);
					}
					addstr("</td>");
					/********     Soft Start Cameras *********/
					if(get_current_user_rule()==ADMIN_RULE){
						addstr("<td><input type=checkbox name=\"SS");
						sprintf(temp,"%d",(i+1));
						addstr(temp);
						addstr("\" value=\"1\"");
						if (((SSC1==1)&&(i==0))||((SSC2==1)&&(i==1))||((SSC3==1)&&(i==2))||((SSC4==1)&&(i==3))
							||((SSC5==1)&&(i==4))||((SSC6==1)&&(i==5)))
							addstr( " CHECKED ");
						if(get_port_sett_poe(i)==0)
							addstr(" disabled=\"disabled\" ");
						addstr("></td>");
					}
					addstr("<td>");
					if (get_port_poe_a(i) == 1)
						addstr("ON");
					else{
						if(dev.port_stat[i].ss_process == 1){
							RTC_GetTime(RTC_Format_BIN,&RTC_Time);
							RTC_GetDate(RTC_Format_BCD,&RTC_Date);
							sprintf(temp,"Wait %d/%d min",(int)(RTC_Time.RTC_Hours*60 + RTC_Time.RTC_Minutes),(int)(get_softstart_time()*60));
							addstr(temp);
						}
						else
							addstr("OFF");
					}
					addstr("</td><td>"
					"<input type=\"Submit\" name=\"poe");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("\" value=\"Start\"");
					if(get_port_sett_poe(i)==0)
						addstr(" disabled=\"disabled\" ");
					addstr(" ></td></tr>");

					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}

				addstr("</table>"
				"<br>");
				if(get_current_user_rule()==ADMIN_RULE){
					addstr(	"<br><input type=\"Submit\" name=\"Apply\" value=\"Apply\" tabindex = \"1\"></form></body></html>");
				}
	  	  }
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_AutoRestart(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
 uint16_t pcount=0;
 static int i,j,p,l;
 static uint8_t reset[PORT_NUM];
 static u16 speed_up[PORT_NUM];
 static u16 speed_down[PORT_NUM];
 static uint8_t apply=0;
 static uip_ipaddr_t IP[PORT_NUM];
 static u8 tmp_ip[4];


	  PSOCK_BEGIN(&s->sout);


	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		 // WDT1=WDT2=WDT3=0;
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			/****************************/

			//wdt
			for(u8 j=0;j<(POE_PORT_NUM);j++){
				sprintf(temp,"wdt%d",j+1);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) set_port_wdt(j,0);
					if (!strncmp(pval, "1", 1)) set_port_wdt(j,1);
					if (!strncmp(pval, "2", 1)) set_port_wdt(j,2);
					if (!strncmp(pval, "3", 1)) set_port_wdt(j,3);
				}
			}


			/********* IP  ********/
			for(u8 j=0;j<(POE_PORT_NUM);j++){
				for(u8 k=0;k<4;k++){
					sprintf(temp,"fe%dip%d",j+1,k);
					if(!strcmp(pname, temp)){
						tmp_ip[k] = (uint8_t)strtoul(pval, NULL, 10);
						if(k==3)
							uip_ipaddr(&IP[j],tmp_ip[0],tmp_ip[1],tmp_ip[2],tmp_ip[3]);
					}
				}
			}

			//speed down
			for(u8 j=0;j<(POE_PORT_NUM);j++){
				sprintf(temp,"fe%ddsp",j+1);
				if(!strcmp(pname, temp)){
					speed_down[j] = (uint16_t)strtoul(pval, NULL, 10);
				}
			}

			//speed up
			for(u8 j=0;j<(POE_PORT_NUM);j++){
				sprintf(temp,"fe%dusp",j+1);
				if(!strcmp(pname, temp)){
					speed_up[j] = (uint16_t)strtoul(pval, NULL, 10);
				}
			}

			/***************************/
			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}

			for(u8 k=0;k<(POE_PORT_NUM);k++){
				sprintf(temp,"reset%d",k+1);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "R", 1))
						reset[k]=1;
				}
			}
	}

		  if(apply==1){
			  for(l=0;l<POE_PORT_NUM;l++){
				  //for autorestart: ping
				  if(get_port_sett_wdt(l)==2){
					  if((uip_ipaddr1(&IP[l])==0)||(uip_ipaddr4(&IP[l])==0)){
						  set_port_wdt(l,0);
						  alertl("Invalid IP addres", "Неверный IP адрес");
						  apply=0;
					  }
					  else
						  set_port_wdt_ip(l,IP[l]);
				  }
				  //for autorestart: speed
				  if(get_port_sett_wdt(l)==3){
					  if(speed_down[l]==0){
						  set_port_wdt(l,0);
						  alertl("Invalid speed", "Неверная скорость");
						  apply=0;
					  }
					  else{

						  if((speed_up[l])&&(speed_down[l]>=speed_up[l])){
							  set_port_wdt(l,0);
							  alertl("Invalid speed", "Неверная скорость");
							  apply=0;
						  }
						  else{
							  set_port_wdt_speed_up(l,speed_up[l]);
							  set_port_wdt_speed_down(l,speed_down[l]);
						  }
					  }
				  }
			  }

			  if(apply){
				  settings_save();
				  alertl("Parameters accepted","Настройки применились");
				  apply=0;
			  }
		  }
				for(p=0;p<POE_PORT_NUM;p++){
					if(reset[p]==1){
						if(OtherCommandsFlag==0){
							OtherCommandsFlag=p+1;
							vTaskResume(xOtherCommands);
							DEBUG_MSG(AUTORESTART_DBG,"reboot cam %d\r\n",p);
							alertl("Reboot...","Перезагрузка...");
						}
						else
						{
							alertl("PoE is reloaded! Try again please...","Порт уже перезагружается, повторите позже");
						}
						reset[p] = 0;
						break;
					}
				}
		  }


  	  	  if(POE_PORT_NUM == 0){
  	  		  addstrl("This feature is not activated in the device","Эта функция недоступна на этом устройстве");
  	  	  }
  	  	  else{

			  //get settings
			  for(u8 i=0;i<POE_PORT_NUM;i++){
				  get_port_sett_wdt_ip(i,&IP[i]);
				  speed_up[i] = get_port_sett_wdt_speed_up(i);
				  speed_down[i] = get_port_sett_wdt_speed_down(i);
			  }

					addhead();
					addstr("<body>");
					addstr("<script type=\"text/javascript\">"
						"function change_css(port){"
							"var element = document.getElementById('ar_click'+port);"
							"if(element){"
								"var dis = document.getElementById('ar_dis'+port);"
								"var el1 = document.getElementById('ar_ping'+port);"
								"var el2 = document.getElementById('ar_speed'+port);"
								"if((element.value == 0)||((element.value == 1)))"
								"{"
									"dis.style.display = \"table-cell\";"
								"}"
								"else"
								"{"
									"dis.style.display = \"none\";"
								"}"

								"if(element.value == 2){"
									"el1.style.display = \"table-cell\";"
								"}"
								"else"
								"{"
									"el1.style.display = \"none\";"
								"}"

								"if(element.value == 3){"
									"el2.style.display = \"table-cell\";"
								"}"
								"else{"
									"el2.style.display = \"none\""
								"}"
							"}"
						"}"
					"</script>");
					addstrl("<legend>Auto Restart</legend>","<legend>Контроль зависания</legend>");
					addstr("<form method=\"post\" bgcolor=\"#808080\""
					"action=\"/settings/AutoRestart.shtml\">");
					addstr("<br>"
					"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
					"<tr class=\"g2\"><td>");
					addstrl("Port","Порт");
					addstr("</td><td>");

					if(get_current_user_rule()==ADMIN_RULE){
						addstrl("Auto restart mode","Критерий зависания");
						addstr("</td>");
						addstr("<td>");
						addstrl("Value","Значение");
						addstr("</td><td>");
					}
					addstrl("Manual restart","Ручная перезагрузка");
					addstr("</td></tr>");

					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;

					for(i=0;i<POE_PORT_NUM;i++){
						/*ports*/
						addstr("<tr class=\"g2\"><td>");
						if(get_dev_type() == DEV_PSW2GPLUS){
							switch(i){
								case 0:addstr("FE#1");break;
								case 1:addstr("FE#2");break;
								case 2:addstr("FE#3");break;
								case 3:addstr("FE#4");break;
							}
						}else{
							sprintf(temp,"%d",i+1);
							addstr(temp);
						}
						addstr("</td>");
						if(get_current_user_rule()==ADMIN_RULE){
							/**********        Freeze Protection  Cam 1-6    **********/
							addstr("<td><select id=\"ar_click");
							sprintf(temp,"%d",i);
							addstr(temp);
							addstr("\"");

							addstr(" onchange=\"change_css(");
							addstr(temp);
							addstr(")\" ");

							if(get_port_sett_poe(i) == 0)
								addstr("disabled ");
							addstr(
							" name=\"wdt");
							sprintf(temp,"%d",(i+1));
							addstr(temp);
							addstr(	"\" size=\"1\">"
							"<option");
							if(get_port_sett_wdt(i)==0)
								addstr(" selected");
							addstr(
							" value=\"0\">");
							addstrl("Disable","Отключено");
							addstr("</option>"
							"<option");
							if(get_port_sett_wdt(i)==1)
								addstr(" selected");
							addstr(
							" value=\"1\">Link</option>"
							"<option");
							if(get_port_sett_wdt(i)==2)
								addstr(" selected");
							addstr(" value=\"2\">Ping</option>"
							"<option");
							if(get_port_sett_wdt(i)==3)
								addstr(" selected");
							addstr(
							" value=\"3\">Speed</option>"
							"</select>"
							"</td>");
							/*************    IP addressCam    **/////
							addstr("<td id=\"ar_ping");
							sprintf(temp,"%d\"",i);
							addstr(temp);
							addstr("><table><tr><td>IP: </td>");
							for(j=0;j<4;j++){
								addstr("<td><input ");
								if(get_port_sett_poe(i)==0)
									addstr(" disabled ");
								addstr(
								" type=\"text\"name=\"fe");
								sprintf(temp,"%d",(i+1));
								addstr(temp);
								addstr("ip");
								sprintf(temp,"%d",j);
								addstr(temp);
								addstr(
								"\"size=\"3\"maxlength=\"3\"value=\"");
								switch(j){
									case 0:sprintf(temp,"%d",uip_ipaddr1(&IP[i]));break;
									case 1:sprintf(temp,"%d",uip_ipaddr2(&IP[i]));break;
									case 2:sprintf(temp,"%d",uip_ipaddr3(&IP[i]));break;
									case 3:sprintf(temp,"%d",uip_ipaddr4(&IP[i]));break;
								}
								addstr(temp);
								addstr("\"></td>");
							}
							addstr("</tr></table></td>");

							//speed
							addstr("<td id=\"ar_speed");
							sprintf(temp,"%d\"",i);
							addstr(temp);
							addstr(">");
							addstrl("Speed: Min", "Cкорость: Мин");
							addstr("<input ");
							if(get_port_sett_poe(i)==0)
								addstr(" disabled ");
							addstr(
							" type=\"text\"name=\"fe");
							sprintf(temp,"%d",(i+1));
							addstr(temp);
							addstr("dsp");
							addstr(
							"\"size=\"3\"maxlength=\"6\"value=\"");
							sprintf(temp,"%d",speed_down[i]);
							addstr(temp);
							addstr("\">");
							addstrl("&nbsp;&nbsp;Max", "&nbsp;&nbsp;Макс");
							addstr("<input ");
							if(get_port_sett_poe(i)==0)
								addstr(" disabled ");
							addstr(
							" type=\"text\"name=\"fe");
							sprintf(temp,"%d",(i+1));
							addstr(temp);
							addstr("usp");
							addstr(
							"\"size=\"3\"maxlength=\"6\"value=\"");
							sprintf(temp,"%d",speed_up[i]);
							addstr(temp);
							addstr("\">");
							sprintf(temp," / %lu Kbps",dev.port_stat[i].rx_speed/128);
							addstr(temp);
							addstr("</td><td id=\"ar_dis");
							sprintf(temp,"%d\"",i);
							addstr(temp);
							addstr(">-</td>");
						}

						/*manual restart*/
						addstr("<td><input ");
						if(get_port_sett_poe(i)==0)
							addstr(" disabled ");
						addstr("type=\"Submit\" name=\"reset");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("\" value=\"Restart\"></td></tr>");

						addstr("<script type=\"text/javascript\">change_css(");
						sprintf(temp,"%d",i);
						addstr(temp);
						addstr(");</script>");
						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
					}

		  addstr("</table><br>");
		  if(get_current_user_rule()==ADMIN_RULE)
			  addstr("<br><input tabindex=\"1\" type=\"Submit\" name=\"Apply\" value=\"Apply\">");

		  addstr("</form></body></html>");
  	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}

static PT_THREAD(run_Ping(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  /*static*/  uint16_t pcount;
  int i;
  static  int PingStart;
  static  uint8_t cnt;
  static  int kk;
  static  u8 IP[4];



	  PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  //int sc;

		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			//sc = 0;

			/********* IP  ********/
			if(!strcmp(pname, "ip0")){
				IP[0]=(uint8_t)strtoul(pval, NULL, 10);
			}
			if(!strcmp(pname, "ip1")){
				IP[1]=(uint8_t)strtoul(pval, NULL, 10);
			}
			if(!strcmp(pname, "ip2")){
				IP[2]=(uint8_t)strtoul(pval, NULL, 10);
			}
			if(!strcmp(pname, "ip3")){
				IP[3]=(uint8_t)strtoul(pval, NULL, 10);
			}

			if(!strcmp(pname, "ping")){
				if(!strcmp(pval, "Ping")){
					cnt = 0;
					PingStart=1;
					SendICMPPingFlag = 0;
				}
			}
	  }

		  if((IP[0]==0)||(IP[3]==0)) {
			  PingStart=0;
			  alertl("Invalid IP address","Неверный IP адрес");

		  }

		  if(PingStart==1){
			  if(OtherCommandsFlag==0){
				  uip_ipaddr(IPDestPing,IP[0],IP[1],IP[2],IP[3]);
				  OtherCommandsFlag=PING_PROCESSING;
				  vTaskResume(xOtherCommands);
			  }
		  }
	  }
			  addhead();
			  addstr("<body>");
			  addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
			  "action=\"/tools/ping.shtml\">");
			  addstr("<legend>PING</legend>");
			  addstr("<br>"
					"<table cellpadding=\"3\">"
/*************    IP    **************************************/////
					"<tr><td>IP ");
					addstrl("address","адрес");
	  				addstr(" </td><td>"
			  		"<input type=\"text\"name=\"ip0\"size=\"3\"maxlength=\"3\"value=\"");
					snprintf(temp,sizeof(temp),"%d",uip_ipaddr1(IPDestPing));
					addstr(temp);
					addstr(
					"\"><input type=\"text\"name=\"ip1\"size=\"3\"maxlength=\"3\"value=\"");
					snprintf(temp,sizeof(temp),"%d",uip_ipaddr2(IPDestPing));
			  		addstr(temp);
			  		addstr(
					"\"><input type=\"text\"name=\"ip2\"size=\"3\"maxlength=\"3\"value=\"");
			  		snprintf(temp,sizeof(temp),"%d",uip_ipaddr3(IPDestPing));
			  		addstr(temp);
			  		addstr("\"><input type=\"text\"name=\"ip3\"size=\"3\"maxlength=\"3\"value=\"");
			  		snprintf(temp,sizeof(temp),"%d",uip_ipaddr4(IPDestPing));
			  		addstr(temp);

			  		addstr(
			  		"\"></td></tr></table>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
					addstr( "<br><input type=\"Submit\" name=\"ping\" value=\"Ping\"></form>");
					addstr(
			  		"<table>");
					PSOCK_SEND_STR(&s->sout,str);
					 str[0]=0;


			if(PingStart==1){//старт пинга


				if((OtherCommandsFlag == PING_PROCESSING)&&(cnt<5)){

						addstr(	 "<tr><td><br>Pinging ");
						snprintf(temp,sizeof(temp),"%d.%d.%d.%d", (int)uip_ipaddr1(IPDestPing),(int)uip_ipaddr2(IPDestPing),
								(int)uip_ipaddr3(IPDestPing),(int)uip_ipaddr4(IPDestPing));
						addstr(temp);
						addstr(
								" with 32 bytes of data, please wait...</td></tr><tr><td><br></td></tr>");
						addstr("<meta http-equiv=\"refresh\" content=\"3; url=/tools/ping.shtml\">");


					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;

					cnt++;
				}
				else{
					cnt=0;
					PingStart=2;
				}
			}

				if(PingStart==2){//отображение
					OtherCommandsFlag=0;
					if(SendICMPPingFlag>4)
						SendICMPPingFlag=4;
						addstr(	 "<br>Pinging ");
					sprintf(temp,"%d.%d.%d.%d", (int)uip_ipaddr1(IPDestPing),(int)uip_ipaddr2(IPDestPing),
							(int)uip_ipaddr3(IPDestPing),(int)uip_ipaddr4(IPDestPing));
					addstr(temp);
					addstr(	" with 32 bytes of data:<br><br>");

					for(kk = 1; kk <= SendICMPPingFlag; kk++ )
					{
							addstr( "<tr><td> Reply from ");
							snprintf(temp,sizeof(temp),"%d.%d.%d.%d", (int)uip_ipaddr1(IPDestPing),(int)uip_ipaddr2(IPDestPing),
									(int)uip_ipaddr3(IPDestPing),(int)uip_ipaddr4(IPDestPing));
							addstr(temp);
							temp[0]=0;
							addstr( ": bytes=32  seq=");
							snprintf(temp,sizeof(temp),"%d",kk);
							addstr(temp);
							temp[0]=0;
							addstr(	"</td></tr>");
							PSOCK_SEND_STR(&s->sout,str);
							str[0]=0;
					}

					addstr(	"<tr><td><br>Ping statistics for ");

					snprintf(temp,sizeof(temp),"%d.%d.%d.%d",
					(int)uip_ipaddr1(IPDestPing),(int)uip_ipaddr2(IPDestPing),(int)uip_ipaddr3(IPDestPing),(int)uip_ipaddr4(IPDestPing));
					addstr(temp);
					addstr(	"<br>Packets: Sent = 4");
					addstr(", Received = ");
					snprintf(temp,sizeof(temp),"%d",SendICMPPingFlag);
					addstr(temp);
					addstr(", Lost = ");
					snprintf(temp,sizeof(temp),"%d",4-SendICMPPingFlag);
					addstr(temp);
					addstr(
						" (");
					snprintf(temp,sizeof(temp),"%d",(((4-SendICMPPingFlag)*100)/4));
					addstr(temp);
					addstr(	"% loss)</td></tr>");
					SendICMPPingFlag=0;
					cnt=0;
					PingStart=0;
				}

	  addstr("</form></body></html>");
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}


//пока не работает
static PT_THREAD(run_PortBaseVLAN(struct httpd_state *s, char *ptr)){
	static  uint8_t pcount;
	static  uint8_t i,j,ok,select_all=0;
	static u8 row,col;
	static struct pb_vlan_t pbvlan;
	static u8 port_vid[PORT_NUM];
	PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  //clear checkboxs
		  for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
			for(uint8_t j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++)
				pbvlan.VLANTable[j][i]=0;
		  //
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "Apply", 1)) ok = 1;
			}

			if(!strcmp(pname, "SelectALL")){
				if (!strncmp(pval, "SelectALL", 1)) select_all = 1;
			}

			if(!strcmp(pname, "DeselectALL")){
				if (!strncmp(pval, "DeselectALL", 1)) select_all = 2;
			}

			for(int i=0;i<(ALL_PORT_NUM);i++){
				sprintf(temp,"P%d",i);
				if(!strcmp(pname, temp)){
					for(int j=0;j<(ALL_PORT_NUM);j++){
						port_vid[i]=(uint8_t)strtoul(pval, NULL, 10);
					}
				}
				for(int j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
					sprintf(temp,"T%02d%02d",i,j);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "1", 1))
							pbvlan.VLANTable[i][j] = 1;
						else
							pbvlan.VLANTable[i][j] = 0;
					}
				}
			}

			if(!strcmp(pname, "state")){
				if (!strncmp(pval, "0", 1)) pbvlan.state = 0;
				if (!strncmp(pval, "1", 1)) pbvlan.state = 1;
			}

			for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				for(uint8_t j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
					if((pbvlan.VLANTable[i][j]==1)){
						pbvlan.VLANTable[j][i]=1;
					}
				}
			}
		 }

		  //выбрать все
		  if(select_all==1){
			  for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				  for(uint8_t j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						pbvlan.VLANTable[j][i]=1;
				  }
			  }
			  set_pb_vlan_table(&pbvlan);
			  select_all=0;
		  }

		  //удалить все
		  if(select_all==2){
			for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				for(uint8_t j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
					pbvlan.VLANTable[j][i]=0;
				}
			}
			set_pb_vlan_table(&pbvlan);
			select_all=0;
		  }

		  if(ok){
			  if(get_dev_type()==DEV_SWU16){
				  for(i=0;i<(ALL_PORT_NUM);i++){
					  set_pb_vlan_swu_port(i,port_vid[i]);
				  }
			  }
			  else
			  {
				  set_pb_vlan_state(pbvlan.state);
				  set_pb_vlan_table(&pbvlan);
			  }

			  settings_add2queue(SQ_PBVLAN);
			  settings_save();
			  ok=0;
			  alertl("Parameters accepted","Настройки применились");
		  }
	  }

	  get_pb_vlan_table(&pbvlan);

	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Port Based VLAN Settings</legend>","<legend>VLAN на базе порта</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{

		  addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/vlan/VLAN_perport.shtml\">");


		  if(get_dev_type()==DEV_SWU16){
			  //table head
			  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			  "<tr class=\"g2\"><td>");
			  addstrl("Port","Порт");
			  addstr("</td><td>");
			  addstrl("VLAN","VLAN");
			  addstr("</td>");
			  addstr("</tr>");
			  PSOCK_SEND_STR(&s->sout,str);
			  str[0]=0;
			  //table body
			  for(i=0;i<ALL_PORT_NUM;i++){
				 //port
				 addstr("<tr class=\"g2\"><td>");
				 sprintf(temp,"%d",i+1);
				 addstr(temp);
				 addstr("</td>");
				 //vlan
				 addstr("<td><select name=\"");
				 sprintf(temp,"P%d",i);
				 addstr(temp);
				 addstr("\" size=\"1\">");
				 for(j=1;j<=ALL_PORT_NUM;j++){
					 addstr("<option");
					 if(get_pb_vlan_swu_port(i)==j)
						 addstr(" selected");
					 addstr(" value=\"");
					 sprintf(temp,"%d",j);
					 addstr(temp);
					 addstr("\">");
					 sprintf(temp,"VLAN%d",j);
					 addstr(temp);
					 addstr("</option>");
				 }
				 addstr("</select></td></tr>");
				 PSOCK_SEND_STR(&s->sout,str);
				 str[0]=0;
			  }

		  }
		  else{
			  addstr("<br>"
			  "<br>"
			  "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
			  addstrl("State:","Состояние:");
			  addstr("&nbsp;&nbsp;&nbsp;"
			  "<select name=\"state\" size=\"1\">"
			  "<option");
			  if(get_pb_vlan_state()==0)
				  addstr(" selected");
			  addstr(" value=\"0\">");
			  addstrl("Disable","Отключено");
			  addstr("</option>"
			  "<option");
			  if(get_pb_vlan_state()==1)
				  addstr(" selected");
			  addstr(" value=\"1\">");
			  addstrl("Enable","Включено");
			  addstr("</option>"
			  "</select>"
			  "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
			  "<input type=\"Submit\" name=\"SelectALL\" value=\"Select All\">"
			  "&nbsp;&nbsp;&nbsp;&nbsp;"
			  "<input type=\"Submit\" name=\"DeselectALL\" value=\"Deselect All\">"
			  "<br><br><br>"
			  "<table><tr><td>");
			  addstrl("Outgoing","Исходящие");
			  addstr("</td><td><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			  "<CAPTION>");
			  addstrl("Incoming","Входящие");
			  addstr("</caption>"
			  //table head
			  "<tr class=\"g2\"><td>\</td>");
			  for(int i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				addstr("<td>");
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:addstr("FE#1");break;
						case 1:addstr("FE#2");break;
						case 2:addstr("FE#3");break;
						case 3:addstr("FE#4");break;
						case 4:addstr("GE#1");break;
						case 5:addstr("GE#2");break;
					}
				}else{
					sprintf(temp,"%d ",i+1);
					addstr(temp);
				}
				addstr("</td>");
			  }
			  addstr("</tr>");

			  //table body
			  PSOCK_SEND_STR(&s->sout,str);
			  str[0]=0;


			  for(row = 0;row<(COOPER_PORT_NUM+FIBER_PORT_NUM);row++){
					addstr("<tr class=\"g2\"><td>");
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(row){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
							case 4:addstr("GE#1");break;
							case 5:addstr("GE#2");break;
						}
					}else{
						sprintf(temp,"%d ",row+1);
						addstr(temp);
					}
					addstr("</td>");
					for(col = 0;col<(COOPER_PORT_NUM+FIBER_PORT_NUM);col++){
						addstr("<td>"
						"<input type=checkbox name=\"");
						sprintf(temp,"T%02d%02d",row,col);
						addstr(temp);
						addstr("\" value=\"1\"");
						if ((pbvlan.VLANTable[row][col]==1)||(row==col))
							addstr( " CHECKED ");
						if(row==col)
							addstr( " disabled ");
						addstr(	"></td>");
					}
					addstr("</tr>");


					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
			  }
		  }


		addstr("</table>"
		"<br><br>"
		"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
		"<input type=\"Submit\" name=\"Apply\" value=\"Apply\">"
		"</form></body></html>");
	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


static  PT_THREAD(run_8021QVLAN(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	  static uint16_t pcount;
	  char TMp[20];
	  static  uint8_t AddNewVLAN=0,EditVLAN=0;
	  static  uint8_t i,k, ApplyEditAdd, DeleteVLAN, Cancel=0,Apply=0;
	  static  uint16_t CurrNum;
	  static  uint16_t VID_Temp,tmp;
	  static  u8 VlanState_Temp;
	  static  uint8_t StatePort[PORT_NUM];
	  static  char text[20];
	  static  u8 j=0,cnt_unt=0/*,cnt_t=0*/;


	  str[0] = 0;
	  temp[0] = 0;

	  PSOCK_BEGIN(&s->sout);
		  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
			  memset(TMp,0,17);
				  for(k=0;k<pcount;k++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,k,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,k,sizeof(s->param));

					  // Buttons

					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)) 	Apply = 1;
					  }

					  if(!strcmp(pname, "MVID")){
						  set_vlan_sett_mngt(strtoul(pval, NULL, 10));//vlan_cfg.mngvid=(uint16_t)strtoul(pval, NULL, 10);
					  }

					  if(!strcmp(pname, "Cancel")){
						  if (!strncmp(pval, "C", 1)) 	Cancel = 1;
					  }

					  if(!strcmp(pname, "ApplyEn")){
						  if (!strncmp(pval, "A", 1)){ 	/*ApplyState = 1;*/}
					  }

					  if(!strcmp(pname, "Add")){
						  if (!strncmp(pval, "A", 1)) 	AddNewVLAN = 1;
					  }

					  if(!strcmp(pname, "ApplyEditAdd")){
						  if (!strncmp(pval, "S", 1)) 	ApplyEditAdd = 1;
					  }

					  for(uint8_t p=0;p<MAXVlanNum;p++){
						  sprintf(temp,"Mod%d",p);
						  if(!strcmp(pname, temp)){
							  if (!strncmp(pval, "Ed", 2)){
								  CurrNum=p;
								  EditVLAN=1;
							  }
						  }
					  }

					  for(uint8_t p=0;p<MAXVlanNum;p++){
						  sprintf(temp,"Del%d",p);
						  if(!strcmp(pname, temp)){
							  if (!strncmp(pval, "Del", 1)){
								  CurrNum=p;
								  DeleteVLAN=1;
							  }
						  }
						  temp[0]=0;
					  }



					  /***** настройка или добавлние VLAN*/
					  if(!strcmp(pname, "VID")){
						  VID_Temp=(uint16_t)strtoul(pval, NULL, 10);
						  if((VID_Temp==0)||(VID_Temp>4094)){
							  //alertl("VID  incorrect!","Неверный VID");
							  VID_Temp=0;
							  //AddNewVLAN=0;
							  VlanState_Temp = 0;
						  }
					  }
					  if(!strcmp(pname, "VLANName")){
						  memcpy(TMp,pval,strlen(pval));
					  }


					  if(!strcmp(pname, "VSt")){
						  if (!strncmp(pval, "0", 1)) 	VlanState_Temp=0;
						  if (!strncmp(pval, "1", 1)) 	VlanState_Temp=1;
					  }


					  for(u8 p=0;p<ALL_PORT_NUM;p++){
							sprintf(temp,"State%d",p);
							if(!strcmp(pname, temp)){
								  if (!strncmp(pval, "0", 1)) 	StatePort[p]=0;
								  if (!strncmp(pval, "1", 1)) 	StatePort[p]=1;
								  if (!strncmp(pval, "2", 1)) 	StatePort[p]=2;
								  if (!strncmp(pval, "3", 1)) 	StatePort[p]=3;
							}
							temp[0]=0;
					  }
				  }


				  /*******************************************/
				  //применение параметров
				  if(Apply==1){

					  //проверяем, что 1 порт имеет не более 1 состяния untaged
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						  cnt_unt=0;
						  for(i=0;i<get_vlan_sett_vlannum();i++){
							  if((get_vlan_vid(i)==0)&&(get_vlan_vid(i)>4094)){

								  Apply=0;
							  }
							  if(get_vlan_port_state(i,j)==2){//port untagged
									cnt_unt++;
									set_vlan_sett_dvid(j,get_vlan_vid(i));//untagged port == default vid
							  }
						  }
						  //если >1 untagged port
						  if(cnt_unt>1){
							  Apply=0;
							  alertl("Bad param. 2 or more untagged port", "Порт является нетегированным в 2-х или более VLAN");
							  break;
						  }
					  }

					  if(Apply){
						  alertl("Parameters accepted","Настройки применились");
						  settings_add2queue(SQ_VLAN);
						  settings_add2queue(SQ_PBVLAN);
						  settings_save();
					  }

					  Apply=0;
					  AddNewVLAN=0;

					  ApplyEditAdd = 0;
					  EditVLAN = 0;
					  DeleteVLAN = 0;
				  }

	 		      if(ApplyEditAdd==1){
					   if(AddNewVLAN==1){
						   if(VID_Temp == 0 || VID_Temp>4094){
							   alertl("Bad param. No valid VID.", "Неверный VID");
							   AddNewVLAN = 0;
							   delete_vlan(VID_Temp);
						   }
						   else{
							   set_vlan_vid(get_vlan_sett_vlannum()-1,VID_Temp);
							   set_vlan_state(get_vlan_sett_vlannum()-1,VlanState_Temp);

							   if(strlen(TMp)>=1){
								   set_vlan_name(get_vlan_sett_vlannum()-1,TMp);
							   }
							   else
								   set_vlan_name(get_vlan_sett_vlannum()-1,"---");


							   for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
								   set_vlan_port(get_vlan_sett_vlannum()-1,j,StatePort[j]);
							   }


							   if(get_vlan_sett_vlannum()<MAXVlanNum){
								   tmp=get_vlan_sett_vlannum()-1;
							   }
							   else
								   addstr("<font color=red>VLAN overflow</font>");
						   }
						   AddNewVLAN=0;
					   }
					   ApplyEditAdd=0;

				  /*******************************************/
				  //редактируем VLAN
				   if(EditVLAN==1){
					   if(CurrNum==0){
						   set_vlan_vid(CurrNum,1);
						   set_vlan_name(CurrNum,"Default");
						   set_vlan_state(CurrNum,1);
					   }
					   else
					   {
						   if(VID_Temp == 0 || VID_Temp>4094){
						   		alertl("Bad param. No valid VID.", "Неверный VID");
						   		EditVLAN = 0;
						   }
						   else{
							   set_vlan_vid(CurrNum,VID_Temp);
							   set_vlan_state(CurrNum,VlanState_Temp);

							   if(strlen(TMp)>1){
								  set_vlan_name(CurrNum,TMp);
							   }
							   else
								   set_vlan_name(CurrNum,"---");
						   }
					   }
					   for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						   set_vlan_port(CurrNum,j,StatePort[j]);
					   }


				   	   }
					   EditVLAN=0;
				  }
				  /*******************************************/
				  //добавляем новый VLAN
				  if(AddNewVLAN==1){
					  set_vlan_sett_vlannum(get_vlan_sett_vlannum()+1);
					  AddNewVLAN=1;
					  CurrNum=get_vlan_sett_vlannum()-1;
				  }
				  /*******************************************/
				  //удаляем VLAN
				   if(DeleteVLAN==1){
					   if((get_vlan_sett_vlannum()>1)&&(CurrNum!=0)){
						  delete_vlan(get_vlan_vid(CurrNum));
						  tmp=CurrNum;
						  send_events_u32(EVENTS_VLAN_EDIT,(u32)CurrNum);
					   }
				   DeleteVLAN=0;
				   }
				   /***********************************************/
				   // отмена
				   if(Cancel==1){
					   temp[0]=0;
					   DeleteVLAN=0;
					   EditVLAN=0;
					   AddNewVLAN=0;
					   Cancel=0;
				   }
		  }




		  addhead();
		  addstr("<body>");
		  addstr("<form method=\"post\" bgcolor=\"#808080\""
				  "action=\"/vlan/VLAN_8021q.shtml\">");
		  addstrl("<legend>802.1Q VLAN Settings</legend>","<legend>Настройки 802.1Q VLAN</legend>");

		  if(get_current_user_rule()!=ADMIN_RULE){
			  //addstr("Access denied");
			  addstr("No implemented");
		  }else{
			  	  addstr(
				  "<br>"
				  "<table width=\"40%\">"
				  "<tr>"

// managment VLAN
				 "<td>Managment VLAN ID</td><td>"
				 "<input type=\"text\" name=\"MVID\"size=\"4\"maxlength=\"4\"value=\"");
					 sprintf(temp,"%d",get_vlan_sett_mngt());
					 addstr(temp);
					 addstr("\"></td></tr></table><br><br><b>");

/********    VLAN LIST  *********************************/
				  addstrl("VLAN List","Список VLAN");
				  addstr("</b><br>"
				  "<table width=\"95%\" class =\"g1\">"
				  "<tr class =\"g2\">"
				  "<td>VID</td><td>");
				  addstrl("State","Состояние");
				  addstr("</td><td>");
				  addstrl("VLAN Name","Имя VLAN");
				  addstr("</td><td>Tagged</td><td>Untagged</td></tr>");
				  for(i=0;i<get_vlan_sett_vlannum();i++){

					  if(!(i==(get_vlan_sett_vlannum()-1) && (AddNewVLAN))){//если не последний элемент и не идет редактирование
						  addstr("<tr bgcolor=\"#f4f4f4\"><td>");
						  sprintf(temp,"%d",get_vlan_vid(i));
						  addstr(temp);
						  addstr("</td><td>");
						  if(get_vlan_state(i)==1)
							  addstrl("Enabled","Включено");
						  else
							  addstrl("Disabled","Выключено");
						  addstr("</td><td>");
						  memset(text,0,17);
						  if(strlen(get_vlan_name(i))<16){
							 http_url_decode(get_vlan_name(i),text,strlen(get_vlan_name(i)));
							 for(uint8_t j=0;j<16;j++){
							   if(text[j]=='+') text[j] = ' ';
							   if(text[j]=='%') text[j]=0;
							 }
						  }
						  addstr(text);
						  addstr("</td><td>");
						  for(k=0;k<(COOPER_PORT_NUM+FIBER_PORT_NUM);k++){
							  if(get_vlan_port_state(i,k)==3){
									if(get_dev_type() == DEV_PSW2GPLUS){
										switch(k){
											case 0:addstr("FE#1 ");break;
											case 1:addstr("FE#2 ");break;
											case 2:addstr("FE#3 ");break;
											case 3:addstr("FE#4 ");break;
											case 4:addstr("GE#1 ");break;
											case 5:addstr("GE#2 ");break;
										}
									}else{
										sprintf(temp,"%d ",k+1);
										addstr(temp);
									}
							  }
						  }
						  addstr("</td><td>");
						  for(k=0;k<(COOPER_PORT_NUM+FIBER_PORT_NUM);k++){
							  if(get_vlan_port_state(i,k)==2){
									if(get_dev_type() == DEV_PSW2GPLUS){
										switch(k){
											case 0:addstr("FE#1 ");break;
											case 1:addstr("FE#2 ");break;
											case 2:addstr("FE#3 ");break;
											case 3:addstr("FE#4 ");break;
											case 4:addstr("GE#1 ");break;
											case 5:addstr("GE#2 ");break;
										}
									}else{
										sprintf(temp,"%d ",k+1);
										addstr(temp);
									}
							  }
						  }
						  addstr("</td></tr>");
						  PSOCK_SEND_STR(&s->sout,str);
						  str[0]=0;
					  }
				  }
				  addstr("</table><br><br>");

/********    Default VLAN PORT TABLE  ********************************/
/*				  addstr( "<b>Default VLAN Table</b><br>"
				  "<table width=\"95%\" class=\"g1\">"
				  "<tr class=\"g2\"><td>");
				  addstrl("Port","Порт");
				  addstr("</td>");
				  for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				  		addstr("<td>");
				  		if(get_dev_type() == DEV_PSW2GPLUS){
							switch(i){
								case 0:addstr("FE#1");break;
								case 1:addstr("FE#2");break;
								case 2:addstr("FE#3");break;
								case 3:addstr("FE#4");break;
								case 4:addstr("GE#1");break;
								case 5:addstr("GE#2");break;
							}
						}else{
							sprintf(temp,"%d",i+1);
							addstr(temp);
						}
						addstr("</td>");
				  }
				  addstr("</tr>"
				  "<tr class=\"g2\"><td>Default VID</td>");
				  //port list
				  for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
					  addstr("<td><input type=\"text\"name=\"dvid");
					  sprintf(temp,"%d",i);
					  addstr(temp);
					  addstr("\"size=\"5\"maxlength=\"5\"");
					  addstr(" value=\"");
					  sprintf(temp,"%d",get_vlan_sett_dvid(i));
					  addstr(temp);
					  addstr("\"></td>");
				  }
				  addstr("</tr></table><br><br>");

				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;
*/
/********    VLAN TABLE  ********************************/
				  addstr( "<b>VTU Table</b><br>"
				  "<table width=\"95%\" class=\"g1\">"
				  "<tr class=\"g2\"><td>VID</td><td>");
				  addstrl("State","Состояние");
				  addstr("</td><td>");
				  addstrl("VLAN Name","Имя VLAN");
				  addstr("</td>");
				  for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
					  addstr("<td>");
						if(get_dev_type() == DEV_PSW2GPLUS){
							switch(i){
								case 0:addstr("FE#1");break;
								case 1:addstr("FE#2");break;
								case 2:addstr("FE#3");break;
								case 3:addstr("FE#4");break;
								case 4:addstr("GE#1");break;
								case 5:addstr("GE#2");break;
							}
						}else{
							sprintf(temp,"%d",i+1);
							addstr(temp);
						}
						addstr("</td>");
				  }

				  addstr("<td width=\"50\"></td><td width=\"50\"></td></tr>");

				  if(get_vlan_sett_vlannum()==0)
					  addstr("<br>No record<br>");
				  i=0;
				while(i<get_vlan_sett_vlannum()){
					  /*если нажимали редактировать*/
					  if(((EditVLAN==1)&&(i==CurrNum))||((AddNewVLAN==1)&&(i==(get_vlan_sett_vlannum()-1)))){
						  addstr("<tr class=\"g2\">");
						  addstr( "<td><input type=\"text\"name=\"VID\"size=\"5\"maxlength=\"5\"");
						  if((CurrNum==0)&&(EditVLAN))
							 addstr(" disabled value=\"");
						  else
							 addstr(" value=\"");
						  if(EditVLAN){
							 sprintf(temp,"%d",(int)get_vlan_vid(CurrNum));
							 addstr(temp);
						  }
						  addstr("\"></td>"
					      "<td><select name= \"VSt\" size=\"1\"");
						  if(CurrNum==0)
							  addstr(" disabled ");
					      addstr("><option");
					if(get_vlan_state(CurrNum)==0) addstr(" selected");
						  addstr(" value=\"0\">");
						  addstrl("Disabled","Выключено");
						  addstr("</option>"
						  "<option");
					if(get_vlan_state(CurrNum)==1) addstr(" selected");
						  addstr(" value=\"1\">");
						  addstrl("Enabled","Включено");
						  addstr("</option>"
						  "</select></td>"
						  "<td><input type=\"text\"name=\"VLANName\"size=\"6\"maxlength=\"16\"");
						  if((CurrNum==0)&&(EditVLAN))
							 addstr(" disabled value=\"");
						  else
							 addstr(" value=\"");
						 //text[0]=0;
						 memset(text,0,16);
						 if(strlen(get_vlan_name(CurrNum))<16){
							 http_url_decode(get_vlan_name(CurrNum),text,strlen(get_vlan_name(CurrNum)));
							 for(j=0;j<16;j++){
						  	   if(text[j]=='+') text[j] = ' ';
						  	   if(text[j]=='%') text[j]=0;
						     }
						 }
						 //text[strlen(VLAN[CurrNum].VLANNAme)]=0;
						 //sprintf(text,text);
						 if(EditVLAN)
							 addstr(text);

						 addstr("\"></td>");

						 PSOCK_SEND_STR(&s->sout,str);
						 str[0]=0;

						 for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
							 addstr(
							 "<td>"
							 "<select name=\"State");
							 sprintf(temp,"%d",j);
							 addstr(temp);
							 addstr("\" size=\"1\">"
							 "<option");
							 if((get_vlan_port_state(CurrNum,j)==0)&&(EditVLAN==1)) addstr(" selected");
							 addstr(
							 " value=\"0\">N</option>"
							 "<option");
							 if((get_vlan_port_state(CurrNum,j)==2)&&(EditVLAN==1)) addstr(" selected");
							 addstr(
							 " value=\"2\">U</option><option");
							 if((get_vlan_port_state(CurrNum,j)==3)&&(EditVLAN==1)) addstr(" selected");
							 addstr(
							 " value=\"3\">T</option>"
							 "</select>"
							 "</td>");

							 if(j%9==0){
								 PSOCK_SEND_STR(&s->sout,str);
							 	 str[0]=0;
							 }
						 }
						 addstr("<td><input type=\"Submit\" name=\"ApplyEditAdd\" value=\"Set Vlan\"></td><td></td></tr>");
					  }
					  else
					  {
						  addstr("<tr bgcolor=\"#f4f4f4\"><td>");
						  if(get_vlan_vid(i)<=4095){
							  sprintf(temp,"%d",(int)(get_vlan_vid(i)));
							  addstr(temp);
						  }
						  addstr("</td><td>");
						  if(get_vlan_state(i) == 1)
							  addstrl("Enabled","Включено");
						  else
							  addstrl("Disabled","Выключено");

						  addstr("</td><td>");

						  memset(text,0,sizeof(text));
						  if(strlen(get_vlan_name(i))<16){
							  http_url_decode(get_vlan_name(i),text,strlen(get_vlan_name(i)));
							  for(k=0;k<16;k++){
								if(text[k]=='+') text[k] = ' ';
								if(text[k]=='%') text[k]=0;
							  }
							  addstr(text);
						  }

						  addstr("</td>");
						  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
							  addstr("<td>");
								  switch(get_vlan_port_state(i,j)){
									  case 0:addstr("N");break;
									  case 1:addstr("U");break;
									  case 2:addstr("U");break;
									  case 3:addstr("T");break;
									  default:addstr("-");break;
								  }
							addstr("</td>");
						  }
						  addstr("<td>"
						  "<input type=\"Submit\" ");
						  if((EditVLAN==1)||(AddNewVLAN==1))addstr( " disabled ");
						  addstr("name=\"Mod");
						  sprintf(temp,"%d",i);
						  addstr(temp);
						  addstr(
						  "\" value=\"Edit\"></td>"
						  "<td>"
						  "<input type=\"Submit\" ");
						  if((EditVLAN==1)||(AddNewVLAN==1) || (i == 0))addstr( " disabled ");
						  addstr("name=\"Del");
						  sprintf(temp,"%d",i);
						  addstr(temp);
						  addstr("\" value=\"Delete\"></td></tr>");
					  }
					  i++;
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				  }
				  //memset(temp,0,sizeof(temp));
				  addstr("</table><br>");
				  addstr( "<table width=\"100%\"><tr><td width=\"70%\">"
				  "<input type=\"Submit\"");
				  if((EditVLAN==1)||(AddNewVLAN==1))addstr( " disabled ");
				  addstr(" name=\"Apply\" value=\"Apply\"></td><td>"
				  "<input type=\"Submit\" name=\"Add\" ");
				  if((EditVLAN==1)||(AddNewVLAN==1))addstr( " disabled ");
				  addstr( "value=\"Add New VLAN\"></td></tr></table>");
		addstr("</form></body></html>");
		}//
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		temp[0]=0;
		PSOCK_END(&s->sout);
}



static PT_THREAD(run_VLAN_trunk(struct httpd_state *s, char *ptr)){
	static  uint8_t pcount;
	static  uint8_t i,ok;
	static u8 state = 0;
	static u8 port[PORT_NUM];

	PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  //clear checkboxs
		  for(uint8_t i=0;i<(ALL_PORT_NUM);i++)
			  port[i] = 0;

		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "Apply", 1)) ok = 1;
			}

			for(int i=0;i<(ALL_PORT_NUM);i++){
				sprintf(temp,"P%d",i);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "1", 1))
						port[i] = GT_FALLBACK;
					else
						port[i] = GT_SECURE;
				}
			}

			if(!strcmp(pname, "state")){
				if (!strncmp(pval, "0", 1)) state = 0;
				if (!strncmp(pval, "1", 1)) state = 1;
			}
		 }


		 if(ok){
			  set_vlan_trunk_state(state);
			  if(state){
				  for(u8 i=0;i<(ALL_PORT_NUM);i++){
					  set_vlan_sett_port_state(i,port[i]);
				  }
			  }
			  else{
				  for(u8 i=0;i<(ALL_PORT_NUM);i++){
					  set_vlan_sett_port_state(i,GT_SECURE);
				  }
			  }
			  settings_add2queue(SQ_VTRUNK);
			  settings_save();
			  alertl("Parameters accepted","Настройки применились");
			  ok=0;
		 }
	  }


	  addhead();
	  addstr("<body>");
	  addstrl("<legend>VLAN Trunking Settings</legend>","<legend>VLAN Trunking</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{

		  addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
		  "action=\"/vlan/VLAN_trunk.shtml\">");
		  addstr("<br>"
		  "<br>");
		  addstrl("State:","Состояние:");
		  addstr("&nbsp;&nbsp;&nbsp;"
		  "<select name=\"state\" size=\"1\">"
		  "<option");
		  if(get_vlan_trunk_state()==0)
			  addstr(" selected");
		  addstr(" value=\"0\">");
		  addstrl("Disable","Отключено");
		  addstr("</option>"
		  "<option");
		  if(get_vlan_trunk_state()==1)
			  addstr(" selected");
		  addstr(" value=\"1\">");
		  addstrl("Enable","Включено");
		  addstr("</option>"
		  "</select><br><br>"

		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td>");
		  addstrl("Port","Порт");
		  addstr("</td><td>");
		  addstrl("State","Состояние");
		  addstr("</td></tr>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  for(u8 i=0;i<(ALL_PORT_NUM);i++){
			addstr("<tr class=\"g2\"><td>");
			if(get_dev_type() == DEV_PSW2GPLUS){
				switch(i){
					case 0:addstr("FE#1");break;
					case 1:addstr("FE#2");break;
					case 2:addstr("FE#3");break;
					case 3:addstr("FE#4");break;
					case 4:addstr("GE#1");break;
					case 5:addstr("GE#2");break;
				}
			}else{
				sprintf(temp,"%d ",i+1);
				addstr(temp);
			}
			addstr("</td><td>"
			"<input type=checkbox name=\"");
			sprintf(temp,"P%d",i);
			addstr(temp);
			addstr("\" value=\"1\"");
			if (get_vlan_sett_port_state(i))
				addstr( " CHECKED ");
			addstr("></td></tr>");
		}

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

 		addstr("</table>"
		"<br><br>"
		"<input type=\"Submit\" name=\"Apply\" value=\"Apply\">"
		"</form></body></html>");
	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}




/*******************************************************************************************************/
/*        статистика по портам																	    	*/
/*******************************************************************************************************/
static PT_THREAD(run_Port_stat(struct httpd_state *s, char *ptr)){
static u8 i;
	str[0]=0;
	  PSOCK_BEGIN(&s->sout);
		addhead();
		addstr("<body>");
		addstr("<script type=\"text/javascript\">"
		"function open_port_info(port)"
		"{"
			"window.open('/info/port_info.shtml?port='+port,'','width=1000,height=800,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');"
		"}"
		"</script>");
		addstrl("<legend>Port Statistics</legend>","<legend>Статистика по портам</legend>");
	    addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/info/Port_stat.shtml\">");
	    addstr("<meta http-equiv=\"refresh\" content=\"15; url=/info/Port_stat.shtml\">");

	    addstr("<br><br>"
		  	  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			  "<tr class=\"g2\"><td>");

		addstrl("Port","Порт");
		addstr("</td><td>");
		addstrl("RX Speed, Kbps","Входящие данные, Кб/с");
		addstr("</td><td>");
		addstrl("TX Speed, Kbps","Исходящие данные, Кб/с");
		addstr("</td><td></td></tr>");

	  	PSOCK_SEND_STR(&s->sout,str);
	  	str[0]=0;

		//main table
		for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			  addstr("<tr class=\"g2\"><td>");
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:addstr("FE#1");break;
						case 1:addstr("FE#2");break;
						case 2:addstr("FE#3");break;
						case 3:addstr("FE#4");break;
						case 4:addstr("GE#1");break;
						case 5:addstr("GE#2");break;
					}
				}else{
					sprintf(temp,"%d",i+1);
					addstr(temp);
				}
				addstr("</td>");
				//rx good
				addstr("<td>");
				//dev.port_stat[i].rx_speed/128
				//sprintf(temp,"%d.%d",
				//		(int)(dev.port_stat[i].rx_speed/131072),
				//		(int)((dev.port_stat[i].rx_speed*10)/131072-(dev.port_stat[i].rx_speed/131072)*10));
				sprintf(temp,"%lu",dev.port_stat[i].rx_speed/128);
				addstr(temp);
				addstr("</td>");

				//tx good
				addstr("<td>");
				//sprintf(temp,"%d.%d",
				//		(int)(dev.port_stat[i].tx_speed/131072),
				//		(int)((dev.port_stat[i].tx_speed*10)/131072-(dev.port_stat[i].tx_speed/131072)*10));
				sprintf(temp,"%lu",dev.port_stat[i].tx_speed/128);
				addstr(temp);
				addstr("</td>"
				"<td><input type=\"button\" value =\"More info\" onclick=\"open_port_info(");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr(");\"></td>"
				"</tr>");

				if(i%8==0){
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}
		 }

			addstr("</table>"
			"<br><br>"
			"<br><br>"
			"<tr align=\"justify\"><td colspan=2>"
			"<input type=\"Submit\" value=\"Refresh\" width=\"200\"></tr></table></form></body></html>");
		  	PSOCK_SEND_STR(&s->sout,str);
		  	str[0]=0;
		  PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*        ARP																	    	*/
/*******************************************************************************************************/
static PT_THREAD(run_ARP(struct httpd_state *s, char *ptr)){
	/*static*/  uint16_t pcount;
	static  uint8_t Del=0,i/*,j*/;
	str[0]=0;
	  PSOCK_BEGIN(&s->sout);
	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  Del=0;
				  for(int i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  // Buttons
					  if(!strcmp(pname, "ArpD")){
						  if (!strncmp(pval, "C", 1)) 	Del = 1;
					  }
				  }

				  if(Del==1){
					  send_events_u32(EVENT_CLEAR_ARP,0);
					  vTaskDelay(2000*MSEC);
					  uip_arp_init();
					  vTaskDelay(1000*MSEC);
					  Del=0;
				  }
	  }
		  str[0]=0;
		  addhead();
		  addstr("<body>");
		  addstrl("<legend>ARP Table</legend>","<legend>ARP таблица</legend>");
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/info/ARP.shtml\">");

		  addstr( "<br><br>"
		  		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				  "<tr class=\"g2\"><td width=\"5%\">№</td><td width=\"25%\">IP ");
		  addstrl("address","адрес");
		  addstr("</td><td >MAC ");
		  addstrl("address","адрес");
		  addstr("</td>"
		  /*"<td width=\"10%\">");
		  addstrl("estimated time","время жизни");
		  addstr("</td>*/
		  "</tr>");

						for(i=0;i<UIP_ARPTAB_SIZE;i++){
						 if(arp_table[i].ipaddr[0]){
							addstr("<tr class=\"g2\"><td>");
							sprintf(temp,"%d",i+1);
							addstr(temp);
							addstr("</td><td>");
							sprintf(temp,"%d.%d.%d.%d",
									(uint8_t)(arp_table[i].ipaddr[0]),
									(uint8_t)((arp_table[i].ipaddr[0])>>8),
									(uint8_t)(arp_table[i].ipaddr[1]),
									(uint8_t)((arp_table[i].ipaddr[1])>>8));
							addstr(temp);
							addstr("</td><td>");
							sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
									arp_table[i].ethaddr.addr[0],
									arp_table[i].ethaddr.addr[1],
									arp_table[i].ethaddr.addr[2],
									arp_table[i].ethaddr.addr[3],
									arp_table[i].ethaddr.addr[4],
									arp_table[i].ethaddr.addr[5]);
							addstr(temp);
							//addstr("</td><td>");
							//sprintf(temp,"%d",get_arp_time(i));
							//addstr(temp);
							addstr("</td></tr>");
						 }
						 if((i%10)==0){
							PSOCK_SEND_STR(&s->sout,str);
							str[0]=0;
						 }

						}
					  	PSOCK_SEND_STR(&s->sout,str);
					  	str[0]=0;
					  addstr(
					  "</table>"
		  			"<br><br>"
		  			"<br><br>"
		  			"<table width=\"100%\"><tr><td>"
		  			"<input type=\"Submit\" value=\"Refresh\" width=\"200\"></td><td>");
					if(get_current_user_rule()==ADMIN_RULE){
						addstr("<input type=\"Submit\" name=\"ArpD\" value=\"Clear ARP table\">");
					}
					addstr("</td>"
		  			"</tr></table>");
	/***********************************************************************/
     				  addstr(
		  			"</form></body></html>");
		  	PSOCK_SEND_STR(&s->sout,str);
		  	str[0]=0;
		  PSOCK_END(&s->sout);
}

/*******************************************************************************************************/
/*        STATISTICS -> MAC ADDRESS	TABLE															    	*/
/*******************************************************************************************************/
static  PT_THREAD(run_MAC(struct httpd_state *s, char *ptr)){
	static u8 port;
	static u8 find=0;
	static u16 cnt;
	static  int i=0,start;
	struct mac_entry_t entry;
	uint16_t pcount;
	PSOCK_BEGIN(&s->sout);
    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		for(int i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			//if(!strcmp(pname, "Find")){
			//	if (!strncmp(pval, "F", 1)) 	find = 1;
			//}

			if(!strcmp(pname, "port")){
				for(u8 j=0;j<ALL_PORT_NUM;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp)){
						port = j;
						find = 1;
					}
				}
				if (!strcmp(pval, "All")){
					find = 0;
					port = 0xFF;
				}
			}
		}
    }

	str[0]=0;


		  addhead();
		  addstr("<body>");
		  addstrl("<legend>MAC Table</legend>","<legend>Таблица MAC адресов</legend>");
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/info/MAC.shtml\">");
		  addstr("<br><br>");
		  addstrl("MAC Address Ageing Time : ","Время жизни записей в MAC таблице: ");
		  sprintf(temp,"%lu",read_atu_agetime());
		  addstr(temp);
		  addstr("<br><br>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td>");
		  addstrl("Port","Порт");
		  addstr("</td><td>"
		  "<select name=\"port\" size=\"1\">");
		  addstr("<option value=\"All\" ");
		  addstr(" selected ");
		  addstr(">All</option>");
		  for(i=0;i<ALL_PORT_NUM;i++){
				addstr("<option value=\"");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\"");
			    if(port == i && find)
			    	addstr(" selected");
				addstr(">");
				sprintf(temp,"%d",i+1);
				addstr(temp);
				addstr("</option>");
		  }
		  addstr("</td><td>"
		  "<input type=\"Submit\" value=\"Find\" name=\"Find\" width=\"200\"></td></tr></table>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  addstr("<br><br>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td width=\"5%\">№</td><td width=\"60%\">MAC ");
		  addstrl("address","адрес");
		  addstr("</td><td>");

		  if(get_dev_type()==DEV_SWU16){
			  addstr("VLAN</td><td>");
		  }

		  addstrl("Port","Порт");
		  addstr("</td></tr>");

		 start = 1;
		 i=0;

		 if(get_dev_type() == DEV_SWU16){
			 cnt=0;
			 for(i=0;i<SALSA2_FDB_MAX;i++){
				 if(get_salsa2_fdb_entry(i,&entry)==1){

					 if(find == 1){
						 if(entry.port_vect != port)
							 continue;
					 }

					 addstr("<tr class=\"g2\"><td>");
					 sprintf(temp,"%d",cnt+1);
					 addstr(temp);
					 addstr("</td><td>");
					 sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",entry.mac[0],entry.mac[1],entry.mac[2],
							 entry.mac[3],entry.mac[4],entry.mac[5]);
					 addstr(temp);
					 addstr("</td><td>");
					 sprintf(temp,"%d",entry.vid);
					 addstr(temp);
					 addstr("</td><td>");
					 if(entry.is_trunk)
						 sprintf(temp,"trunk%lu",entry.port_vect);
					 else
						 sprintf(temp,"%lu",entry.port_vect+1);
					 addstr(temp);
					 addstr("</td></tr>");
					 if((cnt%8)==0){
						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
					 }
					 cnt++;
				 }
			 }

		 }
		 else{
			while(read_atu(0, start, &entry)==0){
				 start = 0;
				 if(find == 1){
					if(!(entry.port_vect & (1<<L2F_port_conv(port))))
						continue;
				 }
				 i++;

				 addstr("<tr class=\"g2\"><td>");
				 sprintf(temp,"%d",i);
				 addstr(temp);
				 addstr("</td><td>");
				 sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",(uint8_t)entry.mac[1],(uint8_t)entry.mac[0],(uint8_t)entry.mac[3],(uint8_t)entry.mac[2],
						 (uint8_t)entry.mac[5],(uint8_t)entry.mac[4]);
				 addstr(temp);
				 addstr("</td><td>");

				 if(get_dev_type() == DEV_SWU16){
					 sprintf(temp,"%d",entry.vid);
					 addstr(temp);
					 addstr("</td><td>");
				 }

				 if(get_dev_type() == DEV_PSW2GPLUS){
					 if(entry.port_vect & 1)
						 addstr("FE#1 ");
					 if(entry.port_vect & 4)
						 addstr("FE#2 ");
					 if(entry.port_vect & 16)
						 addstr("FE#3 ");
					 if(entry.port_vect & 64)
						 addstr("FE#4 ");
					 if(entry.port_vect & 256)
						 addstr("GE#1 ");
					 if(entry.port_vect & 512)
						 addstr("GE#2 ");
					 if(entry.port_vect & 1024)
						 addstr("CPU ");
				 } else if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)){
					 if(entry.port_vect & 1)
						 addstr("1 ");
					 if(entry.port_vect & 4)
						 addstr("2 ");
					 if(entry.port_vect & 16)
						 addstr("3 ");
					 if(entry.port_vect & 64)
						 addstr("4 ");
					 if(entry.port_vect & 256)
						 addstr("5 ");
					 if(entry.port_vect & 512)
						 addstr("6 ");
					 if(entry.port_vect & 1024)
						 addstr("CPU ");
				 }else if(get_dev_type() == DEV_PSW2G6F){
					 if(entry.port_vect & 1)
						 addstr("1 ");
					 if(entry.port_vect & 4)
						 addstr("2 ");
					 if(entry.port_vect & 16)
						 addstr("3 ");
					 if(entry.port_vect & 32)
						 addstr("4 ");
					 if(entry.port_vect & 64)
						 addstr("5 ");
					 if(entry.port_vect & 128)
						 addstr("6 ");
					 if(entry.port_vect & 256)
						 addstr("7 ");
					 if(entry.port_vect & 512)
						 addstr("8 ");
					 if(entry.port_vect & 1024)
						 addstr("CPU ");
				 }
				 else if(get_dev_type() == DEV_PSW2G8F){
					 if(entry.port_vect & 1)
						 addstr("1 ");
					 if(entry.port_vect & 2)
						 addstr("2 ");
					 if(entry.port_vect & 4)
						 addstr("3 ");
					 if(entry.port_vect & 8)
						 addstr("4 ");
					 if(entry.port_vect & 16)
						 addstr("5 ");
					 if(entry.port_vect & 32)
						 addstr("6 ");
					 if(entry.port_vect & 64)
						 addstr("7 ");
					 if(entry.port_vect & 128)
						 addstr("8 ");
					 if(entry.port_vect & 256)
						 addstr("9 ");
					 if(entry.port_vect & 512)
						 addstr("10 ");
					 if(entry.port_vect & 1024)
						 addstr("CPU ");
				 }
				 else if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
					 if(entry.port_vect & 1)
						 addstr("1 ");
					 if(entry.port_vect & 2)
						 addstr("2 ");
					 if(entry.port_vect & 4)
						 addstr("3 ");
					 if(entry.port_vect & 8)
						 addstr("4 ");
					 if(entry.port_vect & 16)
						 addstr("5 ");
					 if(entry.port_vect & 32)
						 addstr("6 ");
					 if(entry.port_vect & 64)
						 addstr("CPU ");
				 }
				 else if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
					 if(entry.port_vect & 16)
						 addstr("1 ");
					 if(entry.port_vect & 64)
						 addstr("2 ");
					 if(entry.port_vect & 256)
						 addstr("3 ");
					 if(entry.port_vect & 512)
						 addstr("4 ");
					 if(entry.port_vect & 1024)
						 addstr("CPU ");
				 }
				 addstr("</td></tr>");


				 if((i%8)==0){
				  	PSOCK_SEND_STR(&s->sout,str);
				  	str[0]=0;
				 }
			 }
		    }
			addstr("</table><br>"
		  	"<input type=\"Submit\" value=\"Refresh\" width=\"200\">"
			"</form></body></html>");
		  	PSOCK_SEND_STR(&s->sout,str);
		  	str[0]=0;
		  	i=0;
		  PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*        REDUNCY / RSTP															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_RSTP(struct httpd_state *s, char *ptr)){
	static  uint16_t pcount;
	static  uint8_t apply=0;
	static  unsigned int i=0,j=0;
	static  uint8_t selected;
	static  u8 advanced=0;
	static u8 stp_state = 0;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);

	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

				  if(advanced==1)
					  for(uint8_t j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++)
						  set_stp_port_autocost(j,1);


		  		  for(i=0;i<pcount;i++){
					  char *pname,*pval,*End;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));



					  // Buttons
					  if(!strcmp(pname, "Ap")){
						  if (!strncmp(pval, "A", 1)) 	apply = 1;
					  }

					  // Buttons
					  if(!strcmp(pname, "ad")){
						  if (!strncmp(pval, "A", 1)){
							  if(advanced==0)
								 advanced=1;
							  else
								 advanced=0;
						  }
					  }

					  if(!strcmp(pname, "R")){
						  if (!strncmp(pval, "0", 1)) 	{stp_state = 1;set_stp_proto(0);}
						  if (!strncmp(pval, "1", 1)) 	{stp_state = 0;}
						  if (!strncmp(pval, "2", 1)) 	{stp_state = 1;set_stp_proto(2);}
					  }

					  //Brige priority
					  if(!strcmp(pname, "P")){
						  set_stp_bridge_priority(strtol(pval,&End,10));
					  }

					  /*TxHoldCount*/
					  if(!strcmp(pname, "C")){
						  set_stp_txholdcount((u8)strtol(pval,&End,10));
					  }

					  //brige delay
					  if(!strcmp(pname, "D")){
						  set_stp_bridge_fdelay((u8)strtol(pval,&End,10));
					  }
					  //bridge max age
					  if(!strcmp(pname, "M")){
						  set_stp_bridge_max_age((u8)strtol(pval,&End,10));
					  }
					  //bridge Hello time
					  if(!strcmp(pname, "H")){
						  set_stp_bridge_htime((u8)strtol(pval,&End,10));
					  }

					  /*************************************************/
					  /* Port RSTP State   */
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						  sprintf(temp,"S%d",(j+1));
						  if(!strcmp(pname, temp)){
							  if (!strncmp(pval, "0", 1)) 	set_stp_port_enable(j,0);
							  if (!strncmp(pval, "1", 1)) 	set_stp_port_enable(j,1);
						  }
					  }

					  /*************************************************/
					  /*    Port Priority                              */
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						  sprintf(temp,"P%d",(j+1));
						  if(!strcmp(pname, temp)){
							  set_stp_port_priority(j,(u8)strtol(pval,&End,10));
						  }
					  }
					  /*************************************************/
					  /*     Port Cost*/
					  /*************************************************/
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						  sprintf(temp,"C%d",(j+1));
						  if(!strcmp(pname, temp)){
							  set_stp_port_cost(j,(u32)strtoul(pval,&End,10));
						  }
					  }
					   /*************************************************/
					  /*     Table -> Auto Cost*/
					  /*************************************************/
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						sprintf(temp,"%d",(j+1));
						if(!strcmp(pname, temp)){
							  if (!strncmp(pval, "1", 1))
								  set_stp_port_autocost(j,0);
							  if (!strncmp(pval, "0", 1))
								  set_stp_port_autocost(j,1);
						 }
					  }
					  /*************************************************/
					  /*     Table -> Edge*/
					  /*************************************************/
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						  sprintf(temp,"E%d",(j+1));
						  if(!strcmp(pname, temp)){
							  if (!strncmp(pval, "0", 1)){
								  set_stp_port_edge(j,0);
								  set_stp_port_autoedge(j,1);
							  }
							  if (!strncmp(pval, "1", 1)){
								  set_stp_port_edge(j,1);
								  set_stp_port_autoedge(j,0);
							  }
							  if (!strncmp(pval, "2", 1)){
								  set_stp_port_edge(j,0);
								  set_stp_port_autoedge(j,0);
							  }
						  }
					  }

					  /*************************************************/
					  /*     Table -> PTP*/
					  /*************************************************/
					  for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
						  sprintf(temp,"T%d",(j+1));
						  if(!strcmp(pname, temp)){
							  if (!strncmp(pval, "0", 1)){
								  set_stp_port_ptp(j,0);
								  set_stp_port_autoptp(j,1);
							  }
							  if (!strncmp(pval, "1", 1)){
								  set_stp_port_ptp(j,1);
								  set_stp_port_autoptp(j,0);
							  }
							  if (!strncmp(pval, "2", 1)){
								  set_stp_port_ptp(j,0);
								  set_stp_port_autoptp(j,0);
							  }
						  }
					  }

					  /*************************************************/
					  /* Port BPDU Forwarding   */
					  if(!strcmp("FB", temp)){
						  if (!strncmp(pval, "0", 1)) 	set_stp_bpdu_fw(0);
						  if (!strncmp(pval, "1", 1)) 	set_stp_bpdu_fw(1);
					  }


				  }

				  if(apply==1){
					  apply=0;
					  settings_save();
					  settings_add2queue(SQ_STP);
					  if(get_stp_state() != stp_state){
						  set_stp_state(stp_state);
						  alertl("Parameters accepted, reboot...","Настройки применились, перезагрузка...");
						  reboot(REBOOT_MCU_5S);
					  }
					  else{
						  alertl("Parameters accepted","Настройки применились");
					  }
				  }
	  }

	  str[0]=0;
	  addhead();
	  addstr("<body>");
	  addstr("<legend>RSTP</legend>");
	  addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/STP/RSTP.shtml\">");

	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{

	/********        RSTP State            */
		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				 "<caption><b>");
		  addstrl("Bridge settings","Настройки моста");
		  addstr("</b></caption>");


		  addstr("<tr class=\"g2\"><td width=\"50%\">");
		  addstrl("RSTP State","Состояние");
		  addstr("</td><td width=\"40%\">"
						  "<select name=\"R\" ");
						  addstr("size=\"1\"><option");
						  if(get_stp_state()!=1) addstr(" selected");
								addstr(
								" value=\"1\">");
								addstrl("Disable","Выключено");
								addstr("</option>"
								"<option");
						  if((get_stp_state()==1)&&(get_stp_proto()==0)) addstr(" selected");
								addstr(
								" value=\"0\">STP</option>"
								"<option");
						  if((get_stp_state()==1)&&(get_stp_proto()==2)) addstr(" selected");
								addstr(
								" value=\"2\">RSTP</option>"
								"</select>"
								"</td></tr>");

								PSOCK_SEND_STR(&s->sout,str);
								str[0]=0;

								//bpdu forwarding
								addstr("<tr class=\"g2\"><td width=\"50%\">");
								addstr("Forward BPDU");
								addstr("</td><td width=\"40%\">"
								"<select name=\"FB\" size=\"1\">"
								"<option");
								if(get_stp_bpdu_fw()==0){
									addstr(" selected");
								}
								addstr(" value=\"0\">");
								addstrl("Disable","Выключено");
								addstr("</option>"
								"<option");
								if(get_stp_bpdu_fw()){
									addstr(" selected");
								}
								addstr(" value=\"1\">");
								addstrl("Enable","Включено");
								addstr("</option>"
								"</select></td></tr>");



	/********        Brige Priority           */
								addstr("<tr class=\"g2\"><td>");
								addstrl("Bridge Priority","Приоритет моста");
								addstr(" (0-61440)</td><td>"
								"<select name=\"P\" size=\"1\">");
							 for(uint8_t i=0;i<=15;i++){
								 addstr("<option");
								 if(get_stp_bridge_priority()==(4096*i))
									 addstr(" selected");
								 addstr(
								 " value=\"");
								 sprintf(temp,"%d",(int)(4096*i));
								 addstr(temp);
								 addstr("\">");
								 sprintf(temp,"%d",(int)(4096*i));
								 addstr(temp);
								 addstr("</option>");
							 }
							 addstr("</select>");
							 addstr("</td></tr>");

							 PSOCK_SEND_STR(&s->sout,str);
							 str[0]=0;


					if(advanced == 1){
	 /********        Hold Count            */
							addstr("<tr class=\"g2\"><td>TX Hold Count (1-10)</td><td>"
							"<input type=\"text\"name=\"C\"size=\"5\" maxlength=\"2\"value=\"");
							 if(get_stp_txholdcount()<MAX_RSTP_HOLD_CNT){
								 sprintf(temp,"%d",(int)get_stp_txholdcount());
								 addstr(temp);
							 }
							 else{
								 sprintf(temp,"%d",(int)(BSTP_DEFAULT_HOLD_COUNT));
								 addstr(temp);
							 }
							 addstr(
							 "\"></td></tr>"
	/********        Bridge max age           */
							 "<tr class=\"g2\"><td>Bridge Max Age (6-40)</td><td>"
							"<input type=\"text\"name=\"M\"size=\"5\" maxlength=\"2\"value=\"");
							 if((get_stp_bridge_max_age()>=6)&&(get_stp_bridge_max_age()<=40)){
								 sprintf(temp,"%d",(int)get_stp_bridge_max_age());
								 addstr(temp);
							 }
							 else{
								 sprintf(temp,"%d",(int)BSTP_DEFAULT_MAX_AGE/4);
								 addstr(temp);
							 }

							 PSOCK_SEND_STR(&s->sout,str);
								 str[0]=0;

							 addstr(
							 "\"></td></tr>"
	 /********        Bridge Hi Time          */
							 "<tr class=\"g2\"><td>Bridge Hello Time (1-2)</td><td>"
							"<input type=\"text\"name=\"H\"size=\"5\" maxlength=\"1\"value=\"");
							 if((get_stp_bridge_htime()==1)||(get_stp_bridge_htime()==2)){
								 sprintf(temp,"%d",(int)get_stp_bridge_htime());
								 addstr(temp);
							 }
							 else{
								 sprintf(temp,"%d",(int)(BSTP_DEFAULT_HELLO_TIME/4));
								 addstr(temp);
							 }

							 addstr("\"></td></tr>"
	/********        Forward Delay          */
							 "<tr class=\"g2\"><td>Forward Delay Time (4-30)</td><td>"
							"<input type=\"text\"name=\"D\"size=\"5\" maxlength=\"2\"value=\"");
							 if((get_stp_bridge_fdelay()>=4)&&(get_stp_bridge_fdelay()<=30)){
								 snprintf(temp,4,"%d",(int)get_stp_bridge_fdelay());
								 addstr(temp);
							 }
							 else{
								 snprintf(temp,4,"%d",(int)(BSTP_DEFAULT_FORWARD_DELAY/4));
								 addstr(temp);
							 }
							 addstr(
							 "\"></td></tr></table><br><br>");

	 /********        BID Table          */
							PSOCK_SEND_STR(&s->sout,str);
							str[0]=0;
							addstr(
							 //"<b>BID Table</b>"
							 "<table width=\90%\" cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
							 "<caption><b>");
							 addstrl("Port settings","Настройки портов");
							 addstr("</b></caption>"
							 "<tr class=\"g2\"><td>");
							 //table head
							 addstrl("Port","Порт");
							 addstr("</td><td>");
							 addstrl("State","Состояние");
							 addstr("</td><td>");
							 addstrl("Port Priority","Приоритет порта");
							 addstr("</td><td>");
							 addstrl("Cost","Цена пути");
							 addstr("</td><td>");
							 addstrl("Auto cost","Авто. цена пути");
							 addstr("</td><td>");
							 addstr("Edge</td><td>P2P</td></tr>");

							 PSOCK_SEND_STR(&s->sout,str);
							 str[0]=0;

							 //table
							 for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
								//port
								addstr( "<tr class=\"g2\"><td>");
								if(get_dev_type() == DEV_PSW2GPLUS){
									switch(i){
										case 0:addstr("FE#1");break;
										case 1:addstr("FE#2");break;
										case 2:addstr("FE#3");break;
										case 3:addstr("FE#4");break;
										case 4:addstr("GE#1");break;
										case 5:addstr("GE#2");break;
									}
								}else{
									sprintf(temp,"%d",i+1);
									addstr(temp);
								}
								addstr("</td>");
								//state
								addstr("<td>"
								"<select name=\"S");
								sprintf(temp,"%d",i+1);
								addstr(temp);
								addstr("\" size=\"1\">"
									"<option");
								if(get_stp_port_enable(i)==0) addstr(" selected");
									addstr(
									" value=\"0\">");
									addstrl("Disable","Выключено");
									addstr("</option>"
									"<option");
								if(get_stp_port_enable(i)!=0) addstr(" selected");
									addstr(" value=\"1\">");
									addstrl("Enable","Включено");
									addstr("</option>"
								"</select>"
								"</td>");
								//priority
								addstr("<td>"
								"<select name=\"P");
								sprintf(temp,"%d",i+1);
								addstr(temp);
								addstr("\" size=\"1\">");
								for(uint8_t j=0;j<=15;j++){
									 addstr("<option");
									 if(get_stp_port_priority(i)==(16*j))
										 addstr(" selected");
									 addstr(" value=\"");
									 sprintf(temp,"%d",(int)(16*j));
									 addstr(temp);
									 addstr("\">");
									 sprintf(temp,"%d",(int)(16*j));
									 addstr(temp);
									 addstr("</option>");
								}
								addstr("</select>");
								addstr("</td>");
								//cost
								addstr("<td><input type=\"text\"name=\"C");
								sprintf(temp,"%d",i+1);
								addstr(temp);
								addstr("\"size=\"9\" maxlength=\"9\"value=\"");
								sprintf(temp,"%lu",(uint32_t)(get_stp_port_cost(i)));
								addstr(temp);
								addstr("\"></td>");
								//Autocost
								addstr("<td><input type=checkbox name=\"");
								sprintf(temp,"%d",i+1);
								addstr(temp);
								addstr("\" value=\"1\"");
								if (!(get_stp_port_autocost(i)))
									addstr( " CHECKED ");
								addstr("></td>");
								//Edge
								addstr("<td><select name=\"E");
								sprintf(temp,"%d",i+1);
								addstr(temp);
								addstr("\" size=\"1\"><option");
								if((get_stp_port_autoedge(i))&&(selected==0)) {
									addstr(" selected");
								}
								addstr(" value=\"0\">");
								addstrl("Auto","Авто");
								addstr("</option><option");
								if((get_stp_port_edge(i))&&(selected==0)){
									addstr(" selected");
								}
								addstr(" value=\"1\">");
								addstrl("Enable","Включено");
								addstr("</option><option");
								if(((!(get_stp_port_edge(i)))&&
										(!(get_stp_port_autoedge(i))))&&(selected==0)) {
									addstr(" selected");
								}
								addstr(" value=\"2\">");
								addstrl("Disable","Выключено");
								addstr("</option></select>"
								"</td>");
								//p2p
								addstr("<td><select name=\"T");
								sprintf(temp,"%d",i+1);
								addstr(temp);
								addstr("\" size=\"1\">"
									"<option");
								if((get_stp_port_autoptp(i))&&(selected==0)){
									addstr(" selected");
								}
								addstr(" value=\"0\">");
								addstrl("Auto","Авто");
								addstr("</option>"
								"<option");
								if((get_stp_port_ptp(i))&&(selected==0)){
									addstr(" selected");
								}
								addstr(" value=\"1\">");
								addstrl("Enable","Включено");
								addstr("</option>"
								"<option");
								if(((!(get_stp_port_ptp(i)))&&
										(!(get_stp_port_autoptp(i)))&&(selected==0))){
									addstr(" selected");
								}
								addstr(" value=\"2\">");
								addstrl("Disable","Выключено");
								addstr("</option>"
								"</select>"
								"</td></tr>");

								PSOCK_SEND_STR(&s->sout,str);
								str[0]=0;
							 }
					}//end of advanced

					addstr(	"</table><br><br>"
						"<table width =\"100%\"><tr>"
						"<td><input type=\"Submit\" name=\"Ap\" value=\"Apply\"></td>"
						"<td><input type=\"Submit\" name=\"ad\" value=\"");
					if(advanced==1)
						addstr("Advanced Settings Hide");
					else
						addstr("Advanced Settings Show");
					addstr("\"></td></tr></table>");
	  }
	  addstr("</form></body></html>");
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*        STATISTICS / RSTP															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_RSTP_stat(struct httpd_state *s, char *ptr)){
	uint16_t pcount;
	static  uint8_t i,j;
	static  bstp_bridge_stat_t  RstpStat;
	static  bstp_port_stat_t  RstpPortStat[PORT_NUM];



	bstp_get_bridge_stat(&RstpStat);
	for(j=0;j<(ALL_PORT_NUM);j++){
		bstp_get_port_stat(j,&RstpPortStat[j]);
	}

    PSOCK_BEGIN(&s->sout);


	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(j=0;j<pcount;i++){
			  char *pname,*pval;
			  pname = http_get_parameter_name(s->param,j,sizeof(s->param));
			  pval = http_get_parameter_value(s->param,j,sizeof(s->param));

			  // Buttons
			  if(!strcmp(pname, "Apply")){
				  if (!strncmp(pval, "A", 1)){ 	/*apply = 1;*/}
			  }
		  }
	  }
	  addhead();
	  addstr("<body>");
	  addstr("<legend>RSTP</legend>");
	  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/STP/RSTP_stat.shtml\">");
 	  addstr("<br><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<caption><b>Bridge status</b></caption>"
			"<tr class=\"g2\"><td>");
			addstrl("STP/RSTP state","Состояние STP/RSTP");
			addstr("</td><td>");
				 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) addstrl("Active","Активно");
				 else addstrl("Unactive","Неактивно");

			  addstr("</td></tr>"
			  "<tr class=\"g2\"><td>");
			  addstrl("Brige Root status","Статус корневого моста");
			  addstr("</td><td>");
			  if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ROOT) addstr("ROOT");
			  else addstr("---");
			  addstr("</td></tr>");

			 addstr("</tr><tr class=\"g2\">"
			"<td>");
			addstrl("Protocol","Протокол");
			addstr("</td>");
			 addstr("<td>");
				 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE){
					 if(bstp_state.bs_protover==0)
						 addstr("STP");
					 if(bstp_state.bs_protover==2)
						 addstr("RSTP");
				 }
				 else{
					 addstr("---");
				 }
			 addstr("</td>");

			 PSOCK_SEND_STR(&s->sout,str);
			 str[0]=0;

			addstr("</tr><tr class=\"g2\"><td>"
   		    "Root bridge MAC</td><td>");
			if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				  for(int j=0;j<6;j++) {
					 sprintf(temp,"%02X",(int)RstpStat.rootaddr[j]);
					 addstr(temp);
					 if (j!=5){
						 sprintf(temp,":");
						 addstr(temp);
					 }
				  }
			}
			else
				addstr("---");
			  addstr("</td></tr>"
			  "<tr class=\"g2\"><td>");
			 addstrl("Root bridge priority","Приоритет корневого моста");
			 addstr("</td><td>");
			  if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				  sprintf(temp,"%d",(int)(RstpStat.rootpri*256));
				  addstr(temp);
			  }else{
					 addstr("---");
			  }
			 addstr("</td></tr>");


			 if (!(RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ROOT)){
				 addstr("<tr class=\"g2\"><td>");
				 addstrl("Root port","Корневой порт");

				addstr("</td><td>");
				 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
					 if(get_dev_type() == DEV_PSW2GPLUS){
						 if(RstpStat.root_port==0)addstr("FE#1");
						 if(RstpStat.root_port==2)addstr("FE#2");
						 if(RstpStat.root_port==4)addstr("FE#3");
						 if(RstpStat.root_port==6)addstr("FE#4");
						 if(RstpStat.root_port==8)addstr("GE#1");
						 if(RstpStat.root_port==9)addstr("GE#2");
					 }
					 if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
						 if(RstpStat.root_port==0)addstr("1");
						 if(RstpStat.root_port==2)addstr("2");
						 if(RstpStat.root_port==4)addstr("3");
						 if(RstpStat.root_port==6)addstr("4");
						 if(RstpStat.root_port==8)addstr("5");
						 if(RstpStat.root_port==9)addstr("6");
					 }
					 if(get_dev_type() == DEV_PSW2G6F){
						 if(RstpStat.root_port==0)addstr("1");
						 if(RstpStat.root_port==2)addstr("2");
						 if(RstpStat.root_port==4)addstr("3");
						 if(RstpStat.root_port==5)addstr("4");
						 if(RstpStat.root_port==6)addstr("5");
						 if(RstpStat.root_port==7)addstr("6");
						 if(RstpStat.root_port==8)addstr("7");
						 if(RstpStat.root_port==9)addstr("8");
					 }
					 if(get_dev_type() == DEV_PSW2G8F){
						 if(RstpStat.root_port==0)addstr("1");
						 if(RstpStat.root_port==1)addstr("2");
						 if(RstpStat.root_port==2)addstr("3");
						 if(RstpStat.root_port==3)addstr("4");
						 if(RstpStat.root_port==4)addstr("5");
						 if(RstpStat.root_port==5)addstr("6");
						 if(RstpStat.root_port==6)addstr("7");
						 if(RstpStat.root_port==7)addstr("8");
						 if(RstpStat.root_port==8)addstr("9");
						 if(RstpStat.root_port==9)addstr("10");
					 }
				 }
				 else{
					 addstr("---");
				 }

				 addstr("</td></tr>");
			 }



     		 addstr("<tr class=\"g2\"><td>");
			 addstrl("Path cost to root","Путь до корневого коммутатора");
			 addstr("</td><td>");
			if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%d",(int)RstpStat.root_cost);
				 addstr(temp);
     		}
			else
				 addstr("---");
			 addstr("</td></tr><tr class=\"g2\"><td>");
			 addstrl("Designated bridge MAC","MAC назначенного коммутатора");
			 addstr("</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 for(int j=0;j<6;j++) {
					 sprintf(temp,"%02X",(int)RstpStat.desgaddr[j]);
					 addstr(temp);
					 if (j!=5) {
						 sprintf(temp,":");
						 addstr(temp);
					 }
				 }
			 }
			 else
				 addstr("---");
			 addstr("</td></tr><tr class=\"g2\"><td>");
			 addstrl("Designated bridge priority","Приоритет назначенного коммутатора");
			addstr("</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%d",(int)(RstpStat.desgpri*256));
				 addstr(temp);
			 }
			 else
				 addstr("---");
			 addstr("</td></tr><tr class=\"g2\"><td>"

			 "Brige max age</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%d",(int)bstp_state.bs_root_max_age/4);
				 addstr(temp);
			 }
			 else
				 addstr("---");

			 addstr("</td></tr><tr class=\"g2\"><td>"
			 "Brige Hello time</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%d",(int)bstp_state.bs_root_htime/4);
			 	addstr(temp);
			 }
			 else
				 addstr("---");
			 addstr("</td></tr><tr class=\"g2\"><td>"

			 "Forward Delay Time</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%d",(int)bstp_state.bs_root_fdelay/4);
				 addstr(temp);
			 }
			 else
				 addstr("---");
			 addstr("</td></tr><tr class=\"g2\"><td>");

			 addstrl("Time topology change","Время с последнего перестроения сети");
			 addstr("</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%lu",RstpStat.last_tc_time);
			 	addstr(temp);
			 }
			 else
				 addstr("---");
			 addstr("</td></tr><tr class=\"g2\"><td>");
			 addstrl("Topology changes count","Число перестроений сети");
			 addstr("</td><td>");
			 if (RstpStat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) {
				 sprintf(temp,"%d",(int)RstpStat.tc_count);
			 	addstr(temp);
			 }
			 else
				 addstr("---");
			 addstr("</td></tr></table><br><br>");
			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

//по портам

			addstr(  "<table width=\90%\" cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<caption><b>");
			addstrl("Port status","Статус портов");
			addstr("</b></caption>"
			"<tr class=\"g2\"><td>");
			addstrl("Port","Порт");
			addstr("</td><td>");
			addstrl("State","Состояние");
			addstr("</td><td>");
			addstrl("Link","Линк");
			addstr("</td><td>");
			addstrl("Baud rate/Duplex","Скорость/Дуплекс");
			addstr("</td><td>");
			addstrl("Port state","Состояние порта");
			addstr("</td><td>");
			addstrl("Port role","Роль порта");
			addstr("</td><td>");
			addstrl("Port priority","Приоритет порта");
			addstr("</td><td>");
			addstrl("Patch cost","Цена пути");
			addstr("</td>"
			"<td>P2P</td>"
			"<td>Edge</td>"
			"<td>Forward transitions</td></tr>");

			//main table
			for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				//port
				addstr( "<tr class=\"g2\"><td>");
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:addstr("FE#1");break;
						case 1:addstr("FE#2");break;
						case 2:addstr("FE#3");break;
						case 3:addstr("FE#4");break;
						case 4:addstr("GE#1");break;
						case 5:addstr("GE#2");break;
					}
				}else{
					sprintf(temp,"%d",i+1);
					addstr(temp);
				}
				addstr("</td>");
				//state
				addstr("<td>");
				if (RstpPortStat[i].flags.bp_active_==0)
					addstrl("Not active","Не активен");
				else
					addstrl("Active","Активен");
				addstr("</td>");
				//link
				addstr("<td>");
				 if (RstpPortStat[i].flags.bp_active_==0) {
					 addstr("----");
				 }
				 else{
					 if (RstpPortStat[i].if_link_state&LINK_STATE_DOWN){
						 addstr("Link Down");
					 } else
						 addstr("Link Up");
				 }
				 addstr("</td>");
				 //speed/duplex
				 addstr("<td>");
				 if ((RstpPortStat[i].flags.bp_active_==0)||(RstpPortStat[i].if_link_state&LINK_STATE_DOWN)) {
					 addstr("---");
				 } else{
					 sprintf(temp,"%luM/",(RstpPortStat[i].if_baudrate)/1000);
					 addstr(temp);
					 if (RstpPortStat[i].if_link_state&LINK_STATE_FULL_DUPLEX){
						 addstr("Full");
					 } else
						 addstr("Half");
				 }
				 addstr("</td>");
				 //port state
				 addstr("<td>");
				 if (RstpPortStat[i].flags.bp_active_==0) {
					 addstr("----");
				 }
				 else{
					 if(bstp_state.bs_protover==BSTP_PROTO_STP){
						 switch(RstpPortStat[i].state){
								case BSTP_IFSTATE_FORWARDING: addstr("Forwarding"); break;
								case BSTP_IFSTATE_DISABLED:   addstr("Disabled"); break;
								case BSTP_IFSTATE_LISTENING:  addstr("Listering"); break;
								case BSTP_IFSTATE_LEARNING:   addstr("Learning"); break;
								case BSTP_IFSTATE_BLOCKING:   addstr("Blocking"); break;
								case BSTP_IFSTATE_DISCARDING: addstr("Discarding"); break;
						 }
					 }
					 else{
						  switch(RstpPortStat[i].state){
								case BSTP_IFSTATE_FORWARDING: addstr("Forwarding"); break;
								case BSTP_IFSTATE_DISABLED:   addstr("Discarding"); break;
								case BSTP_IFSTATE_LISTENING:  addstr("Discarding"); break;
								case BSTP_IFSTATE_LEARNING:   addstr("Learning"); break;
								case BSTP_IFSTATE_BLOCKING:   addstr("Discarding"); break;
								case BSTP_IFSTATE_DISCARDING: addstr("Discarding"); break;
						 }
					 }
				 }
				 addstr("</td>");
				 //port role
				 addstr("<td>");
				 if (RstpPortStat[i].flags.bp_active_==0) {
					 addstr("----");
				 }
				 else{
					  switch(RstpPortStat[i].role){
						case BSTP_ROLE_DISABLED:   addstr("Disabled"); break;
						case BSTP_ROLE_ROOT:       addstr("Root"); break;
						case BSTP_ROLE_DESIGNATED: addstr("Designated"); break;
						case BSTP_ROLE_ALTERNATE:  addstr("Alternate"); break;
						case BSTP_ROLE_BACKUP:     addstr("Backup"); break;
					  }
				 }
				 addstr("</td>");
				 //port priority
				 addstr("<td>");
				 if ((RstpPortStat[i].flags.bp_active_==0)||(RstpPortStat[i].if_link_state&LINK_STATE_DOWN)) {
					 addstr("---");
				 }else{
					 if(!(RstpPortStat[i].if_link_state&LINK_STATE_DOWN)){
						 sprintf(temp,"%d",(int)RstpPortStat[i].priority);
						 addstr(temp);
					 }
				 }
				 addstr("</td>");
				 //path cost
				 addstr("<td>");
				 if((!(RstpPortStat[i].if_link_state&LINK_STATE_DOWN))&&((RstpPortStat[i].flags.bp_active_!=0))){
					 sprintf(temp,"%lu",RstpPortStat[i].path_cost);
					 addstr(temp);
				 }
				 else
				 {
					 addstr("---");
				 }
				 addstr("</td>");
				 //p2p
				 addstr("<td>");
				 if((!(RstpPortStat[i].if_link_state&LINK_STATE_DOWN))&&((RstpPortStat[i].flags.bp_active_!=0))){
					 if (RstpPortStat[i].flags.bp_ptp_link_)
						 addstr("P2P");
					 else
						 addstr("---");
				 }
				 else
					 addstr("---");
				 addstr("</td>");
				 //edge
				 addstr("<td>");
				 if((!(RstpPortStat[i].if_link_state&LINK_STATE_DOWN))&&((RstpPortStat[i].flags.bp_active_!=0))){
					 if (RstpPortStat[i].flags.bp_operedge_)
						 addstr("Edge");
					 else
						 addstr("---");
				 }
				 else
					 addstr("---");
				 addstr("</td>");
				 //forward transistion
				 addstr("<td>");
				 if((!(RstpPortStat[i].if_link_state&LINK_STATE_DOWN))&&((RstpPortStat[i].flags.bp_active_!=0))){
					sprintf(temp,"%d",(int)RstpPortStat[i].forward_transitions);
					addstr(temp);
				 }
				 else
					 addstr("---");
				 addstr("</td></tr>");

				 PSOCK_SEND_STR(&s->sout,str);
				 str[0]=0;
			}

			addstr("</table><br>"
			"<input type=\"Submit\" value=\"Refresh\" width=\"200\">"
  			"</form></body></html>");
		  	PSOCK_SEND_STR(&s->sout,str);
		  	str[0]=0;
		  PSOCK_END(&s->sout);
}





/*******************************************************************************************************/
/*        STATISTICS / PoE status													    	*/
/*******************************************************************************************************/
static PT_THREAD(run_PoE_stat(struct httpd_state *s, char *ptr)){
	/*static*/  uint16_t pcount;
	///*static*/  uint8_t apply=0;
	static  uint8_t i;
	/*static*/  uint32_t Temp;
	static u32 tot_pwr;


    PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  		  for(i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  // Buttons
					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)){ 	}
					  }
				  }
	  }
	  str[0]=0;
  	  if(POE_PORT_NUM == 0){
  		  addstrl("This feature is not activated in the device","Эта функция недоступна на этом устройстве");
  	  }
  	  else{

		  addhead();
		  addstr("<body>");
		  addstrl("<legend>PoE Status</legend>","<legend>Статистика PoE</legend>");
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/info/PoE_stat.shtml\">");

	//по портам
				addstr("<table width=\30%\" cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\"><td>");
				addstrl("Port","Порт");
				addstr("</td><td>");
				addstrl("Status","Статус");
				addstr("</td><td>");
				addstrl("Mode","Тип");
				addstr("</td><td>");
				addstrl("Power","Мощность");
				addstr("</td></tr>");
				tot_pwr = 0;
				//main table
				for(i=0;i<(POE_PORT_NUM);i++){
					//port
					addstr( "<tr class=\"g2\"><td>");
					if(get_dev_type() == DEV_PSW2GPLUS){
						switch(i){
							case 0:addstr("FE#1");break;
							case 1:addstr("FE#2");break;
							case 2:addstr("FE#3");break;
							case 3:addstr("FE#4");break;
							case 4:addstr("GE#1");break;
							case 5:addstr("GE#2");break;
						}
					}else{
						sprintf(temp,"%d",i+1);
						addstr(temp);
					}
					addstr("</td>");
					//state
					addstr("<td>");
					if(((get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G6F))&&(i<POEB_PORT_NUM)){
						if((get_port_poe_a(i))||(get_port_poe_b(i)))
							addstr("ON");
						else
							addstr("OFF");
					}else{
						if(get_port_poe_a(i))
							addstr("ON");
						else
							addstr("OFF");
					}
					addstr("</td>");
					//type
					addstr("<td>");
					if(get_port_poe_a(i)){
						addstr("PoE A ");
					}

					if(get_port_poe_b(i)){
						addstr("PoE B ");
					}
					addstr("</td>");
					//power
					addstr("<td>");
					if((get_port_poe_a(i))||((get_port_poe_b(i))&&((i<POEB_PORT_NUM)||(get_dev_type() == DEV_PSW2GPLUS)))){
						Temp = 0;
						if(get_port_poe_a(i)){
							Temp += get_poe_voltage(POE_A,i)*get_poe_current(POE_A,i)/1000;
						}
						if(get_port_poe_b(i)&&((i<POEB_PORT_NUM)||(get_dev_type() == DEV_PSW2GPLUS))){
							Temp += get_poe_voltage(POE_B,i)*get_poe_current(POE_B,i)/1000;
						}
						if(Temp){
							tot_pwr+=Temp;
							sprintf(temp,"%d.%d W",(int)(Temp/1000),(int)(Temp % 1000));
							addstr(temp);
						}
					}
					else{
						addstr("---");
					}
					addstr("</td></tr>");

					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}
				addstr("</table>");
				addstr("<br><br><b>");
				addstrl("Total Power","Суммарная мощность");
				sprintf(temp," %d.%d W",(int)(tot_pwr/1000),(int)(tot_pwr%1000));
				addstr(temp);
				addstr("</b><br><br>"
				"<input type=\"Submit\" value=\"Refresh\" width=\"200\">"
				"</form></body></html>");
  	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
      PSOCK_END(&s->sout);
}

/*******************************************************************************************************/
/*        Basic Settings / System log settings													    	*/
/*******************************************************************************************************/
static PT_THREAD(run_syslog_set(struct httpd_state *s, char *ptr)){
	static  uint16_t pcount;
	static  uint8_t Apply=0;
	static  uint16_t i=0;
	static  u8 IP[4];
	static  uip_ipaddr_t serv_ip,tmpip;


    PSOCK_BEGIN(&s->sout);

	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

	  		  for(i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));


					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)) 	Apply = 1;
					  }
					  if(!strcmp(pname, "state")){
						  if (!strncmp(pval, "0", 1)) 	set_syslog_state(0);
						  if (!strncmp(pval, "1", 1)) 	set_syslog_state(1);
					  }

				      if(!strcmp(pname, "ip1")){
				    	  IP[0]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip2")){
				    	  IP[1]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip3")){
				    	  IP[2]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip4")){
				    	  IP[3]=(uint8_t)strtoul(pval, NULL, 10);
				      }

			}

		  	if(Apply==1){
		  		set_syslog_param_init(0);
		  		uip_ipaddr(serv_ip,IP[0],IP[1],IP[2],IP[3]);
		  		set_syslog_serv(serv_ip);
		  		settings_add2queue(SQ_SYSLOG);
		  		settings_save();
		  		alertl("Parametrs accepted","Настройки применились");
		  		Apply=0;
		  	}

	  }

	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Syslog Settings</legend>","<legend>Настройки Syslog</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/mngt/syslog.shtml\">");
		  addstr("<br><br>"
				  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				  "<tr class=\"g2\">"
				  "<td width=\20%\">");
				  addstrl("State","Состояние");
				  addstr("</td><td>"
				  "<select name=\"state\" size=\"1\">"
				  "<option");
			if(get_syslog_state()==0)addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				  "<option ");
			if(get_syslog_state()==1)addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				  "</select>"
				  "</td></tr>"
				  "<tr class=\"g2\"><td>");
				 addstrl("Server IP address","IP адрес сервера");
				 addstr("</td><td>"
				 "<input type=\"text\"name=\"ip1\"size=\"3\" maxlength=\"3\"value=\"");

			 get_syslog_serv(&tmpip);

			 snprintf(temp,4,"%d",uip_ipaddr1(tmpip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip2\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",uip_ipaddr2(tmpip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip3\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",uip_ipaddr3(tmpip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip4\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",uip_ipaddr4(tmpip));
			 addstr(temp);
			 addstr("\"><br></td></tr>"
				"</table><br><br>"
				"<input type=\"Submit\" name=\"Apply\" value=\"Apply\" width=\"200\">"
				"</form></body></html>");
	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        LOG (BLACK BOX)												    	*/
/*******************************************************************************************************/
static PT_THREAD(run_log(struct httpd_state *s, char *ptr)){
	static  u32 i;
	static  uint32_t k;// k- номер записи
	static  uint16_t len1;
	static  uint16_t pcount;
	u16 type;
	char tmp[128];
	static u8 event_type,exit;

    PSOCK_BEGIN(&s->sout);
    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			/****************************/

		}
    }



		addstr("<html><head>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
		"<!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/ui.css\">-->"
		"<!--[if IE]>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/styleIE.css\">"
		"<![endif]-->"
				"<style type=\"text/css\">"
				".type_0{background-color:#E0E0E0;}"
				".type_1{background-color:#9AFF9A;}"
				".type_2{background-color:#FFEC8B;}"
				".type_3{background-color:#7EC0EE;}"
				".type_4{background-color:#00CD66;}"
				".type_5{background-color:#E3CF57;}"
				".type_6{background-color:#FFEBCD;}"
				".type_7{background-color:#FFFF00;}"
				".type_8{background-color:#FF6A6A;}"
				".type_9{background-color:#C67171;}"
				".no_color{background-color:#FFFFFF;}"
				".ctbl{display:none;}"
				"</style>"
				"<script type=\"text/javascript\">"
				"function checkbox(element) {"
					"var items = document.getElementById('log_holder').getElementsByClassName(element.name);"
					"if(element.checked) {"
						"for (var i=0; i<items.length; i++){"
							"items[i].style.display = \"block\";"
						"}"
					"}else{"
						"for (var i=0; i<items.length; i++){"
							"items[i].style.display = \"none\";"
						"}"
					"}"
				 "}");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

				addstr("function log_color(element) {"
					"var items = document.getElementById('log_holder').getElementsByTagName('td');"
					"var table_items = document.getElementsByClassName('ctbl');"
					"if(element.checked) {"
						"for(var j=0;j<table_items.length;j++){"
							"table_items[j].style.display = \"block\";"
						"}"
						"for (var i=0; i<items.length; i++){"
							"var ipos = items[i].className.indexOf(\"no_color\");"
							"if(ipos!=-1){"
								"var str = items[i].className;"
								"str = str.substr(0,ipos);"
								"items[i].className = str;"
							"}"
						"}");
		addstr("}else{"
						"for(var j=0;j<table_items.length;j++){"
							"table_items[j].style.display = \"none\";"
						"}"
						"for (var i=0; i<items.length; i++){"
							"var ipos = items[i].className.indexOf(\"no_color\");"
							"if(ipos==-1){"
								"items[i].className +=\" no_color\";"
							"}"
						"}"
					"}"
				 "}"
				"</script>"
		"</head><body>");



    	addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/log.shtml\">");
    	addstr("<legend>Log</legend>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

    	if(get_current_user_rule()!=ADMIN_RULE){
    		addstr("Access denied");
        }else{

			addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type\" value=\"1\" onchange=\"log_color(this)\"></td>"
			"<td>Highlight  events</td></tr></table>");
			addstr("<br><br>");
			addstr("<div class=\"ctbl\">"
			"<caption>Events filter</caption>"
			"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td>"
			"<input type=\"checkbox\" name=\"type_0\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_0\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Settings Save</td></tr>"

			"<tr class=\"g2\">");
			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;
			addstr("<td><input type=\"checkbox\" name=\"type_1\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_1\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Port.link</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_2\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_2\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Port.PoE</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_3\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_3\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>STP/RSTP</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_4\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_4\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Autorestart.Link</td></tr>"

			"<tr class=\"g2\">");
			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;
			addstr("<td><input type=\"checkbox\" name=\"type_5\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_0\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Autorestart.Ping</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_6\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_6\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Autorestart.Speed</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_7\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_7\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>System</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_8\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_8\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>Access control</td></tr>"

			"<tr class=\"g2\">"
			"<td><input type=\"checkbox\" name=\"type_9\" value=\"1\" onchange=\"checkbox(this)\" checked></td>"
			"<td class=\"type_9\">&nbsp;&nbsp;&nbsp;</td>"
			"<td>UPS</td></tr>"
			"</table></div><br><br>");



			addstr("<input type=\"button\" class=\"button\" value=\"Download log as file\" onclick=\"location.href='/pswlog.log'\"/>"

			"<br>"
			"<table id=\"log_holder\">");

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

			vTaskDelay(100*MSEC);
	#ifdef BB_LOG
			for(k=0;;k++){

				addstr("<tr><td class=\"type_");
				len1=BB_MSG_LEN;
				memset(tmp,0,sizeof(tmp));
				if(k==0){
					read_bb_first(1,0x0F,(u8 *)tmp,&len1);
					type = tmp[BB_MSG_LEN-1]<<8 | tmp[BB_MSG_LEN];
					if_need_send(type,&event_type);
					if(type != 0xFF){
						sprintf(temp,"%d",event_type);
						addstr(temp);
						addstr(" no_color\">");
						sprintf(temp,"%lu:   ",(u32)k);
						addstr(temp);
						addstr(tmp);
					}
				}else{
					if(read_bb_next(1,0x0F,(u8 *)tmp,&len1)){
						exit=1;
						addstr("[eof]");
					}
					else{
						if(type != 0xFF){
							type = tmp[BB_MSG_LEN-1]<<8 | tmp[BB_MSG_LEN];
							if_need_send(type,&event_type);
							sprintf(temp,"%d",event_type);
							addstr(temp);
							addstr(" no_color\">");
							sprintf(temp,"%lu:   ",(u32)k);
							addstr(temp);
							addstr(tmp);
						}
					}
				}
				addstr("</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				memset(str,0,sizeof(str));

			  if(exit){
				  exit=0;
				  break;
			  }
			}


	#else
				  PSOCK_SEND_STR(&s->sout,"Black Box not active");
	#endif

				  addstr("</table>"
				  "<br><br>"
				  "<input type=\"button\" class=\"button\" value=\"Download log as file\" onclick=\"location.href='/pswlog.log'\"/>");
				  addstr("</form></body></html>");
				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;
      }

      PSOCK_END(&s->sout);
}



/*QoS->Port Bandwith*/
static PT_THREAD(run_QoS_rate_limit(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  static uint16_t pcount;
  static  int i=0,j=0;
  static  char temp[20];
  static  uint8_t apply=0;

	  PSOCK_BEGIN(&s->sout);
	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			/****************************/

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}

			for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
				temp[0]=0;
				sprintf(temp,"P%dM",j+1);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) set_rate_limit_mode(j,0);
					if (!strncmp(pval, "1", 1)) set_rate_limit_mode(j,1);
					if (!strncmp(pval, "2", 1)) set_rate_limit_mode(j,2);
					if (!strncmp(pval, "3", 1)) set_rate_limit_mode(j,3);
				}

				temp[0]=0;
				sprintf(temp,"P%dR",j+1);
				if(!strcmp(pname, temp)){
					set_rate_limit_rx(j,(uint32_t)strtoul(pval, NULL, 10));
				}

				temp[0]=0;
				sprintf(temp,"P%dT",j+1);
				if(!strcmp(pname, temp)){
					set_rate_limit_tx(j,(uint32_t)strtoul(pval, NULL, 10));
				}
			}
		  }

		  if(apply==1){
			  alertl("Parametrs accepted","Настройки применились");
			  settings_add2queue(SQ_QOS);
			  settings_save();
			  apply=0;
		  }

		}

	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Port Bandwidth</legend>","<legend>Ограничение пропускной способности</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
					"action=\"/qos/QoS_rate_limit.shtml\">");

		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
					   "<tr class=\"g2\"><td>");
		  addstrl("Port","Порт");
		  addstr("</td><td>");
		  addstrl("RX limit","Ограничение входящего трафика");
		  addstr("</td><td>");
		  addstrl("TX limit","Ограничение исходящего трафика");
		  addstr( "</td></tr>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;


						for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
							addstr( "<tr class=\"g2\"><td>");
							if(get_dev_type() == DEV_PSW2GPLUS){
								switch(i){
									case 0:addstr("FE#1");break;
									case 1:addstr("FE#2");break;
									case 2:addstr("FE#3");break;
									case 3:addstr("FE#4");break;
									case 4:addstr("GE#1");break;
									case 5:addstr("GE#2");break;
								}
							}else{
								sprintf(temp,"%d",i+1);
								addstr(temp);
							}
							addstr("</td>");
							/*ограничение по приему (ingress)*/
							addstr("<td><input type=\"text\"name=\"");
							sprintf(temp,"P%dR",i+1);
							addstr(temp);
							addstr("\"size=\"8\" maxlength=\"8\"value=\"");
							sprintf(temp,"%lu",(uint32_t)get_rate_limit_rx(i));
							addstr(temp);
							addstr("\"");
							addstr("></td>");
							/*ограничение по передаче (egress)*/
							addstr("<td><input type=\"text\"name=\"");
							sprintf(temp,"P%dT",i+1);
							addstr(temp);
							addstr("\"size=\"8\" maxlength=\"8\"value=\"");
							sprintf(temp,"%lu",(uint32_t)get_rate_limit_tx(i));
							addstr(temp);
							addstr("\"></td>");
							addstr("</tr>");
							PSOCK_SEND_STR(&s->sout,str);
							str[0]=0;
						}
						addstr("</table>");
		 addstr("<br><input tabindex=\"1\" type=\"Submit\" name=\"Apply\" value=\"Apply\"></form></body></html>");
	 }
	 PSOCK_SEND_STR(&s->sout,str);
	 str[0]=0;
	 PSOCK_END(&s->sout);
}


/*QoS->SWU Rate Limit*/
static PT_THREAD(run_swu_rate_limit(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  static uint16_t pcount;
  static  int i=0,j=0;
  static  char temp[20];
  static  uint8_t apply=0;

	  PSOCK_BEGIN(&s->sout);
	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			/****************************/

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}

			if(!strcmp(pname, "UC")){
				if (!strncmp(pval, "0", 1)) set_uc_rate_limit(DISABLE);
				if (!strncmp(pval, "1", 1)) set_uc_rate_limit(ENABLE);
			}

			if(!strcmp(pname, "MC")){
				if (!strncmp(pval, "0", 1)) set_mc_rate_limit(DISABLE);
				if (!strncmp(pval, "1", 1)) set_mc_rate_limit(ENABLE);
			}

			if(!strcmp(pname, "BC")){
				if (!strncmp(pval, "0", 1)) set_bc_rate_limit(DISABLE);
				if (!strncmp(pval, "1", 1)) set_bc_rate_limit(ENABLE);
			}

			if(!strcmp(pname, "RT")){
				set_bc_limit((uint8_t)strtoul(pval, NULL, 10));
			}

			/*for(j=0;j<(ALL_PORT_NUM);j++){
				temp[0]=0;
				sprintf(temp,"P%d",j+1);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) set_rate_limit_mode(j,RL_DISABLED);
					if (!strncmp(pval, "1", 1)) set_rate_limit_mode(j,RL_BROADCAST);
					if (!strncmp(pval, "2", 1)) set_rate_limit_mode(j,RL_BRCST_MLTCST);
				}
			}*/
		  }

		  if(apply==1){
			  alertl("Parametrs accepted","Настройки применились");
			  settings_add2queue(SQ_PORT_ALL);
			  settings_save();
			  apply=0;
		  }

		}

	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Rate Limit</legend>","<legend>Ограничение пропускной способности</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
					"action=\"/qos/swu_rate_limit.shtml\">");

		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td>");
		  addstrl("Unknown Unicast Rate Limit","Ограничение неизвестного юникаста");
		  addstr("</td><td><select name=\"UC\" size=\"1\">"
		  "<option");
		  if(get_uc_rate_limit()==DISABLE)
			  addstr(" selected");
		  addstr(" value=\"0\">");
		  addstrl("Disable","Выключено");
		  addstr( "</option>"
		  "<option");
		  if(get_uc_rate_limit()==ENABLE)
			  addstr(" selected");
		  addstr(" value=\"1\">");
		  addstrl("Enable","Включено");
		  addstr("</option>"
		  "</select></td></tr>"
		  "<tr class=\"g2\"><td>");
		  addstrl("Unknown Multicast Rate Limit","Ограничение неизвестного мультикаста");
		  addstr("</td><td><select name=\"MC\" size=\"1\">"
		  "<option");
		  if(get_mc_rate_limit()==DISABLE)
			  addstr(" selected");
		  addstr(" value=\"0\">");
		  addstrl("Disable","Выключено");
		  addstr( "</option>"
		  "<option");
		  if(get_mc_rate_limit()==ENABLE)
			  addstr(" selected");
		  addstr(" value=\"1\">");
		  addstrl("Enable","Включено");
		  addstr("</option>"
		  "</select></td></tr>"
		  "<tr class=\"g2\"><td>");
		  addstrl("Broadcast Rate Limit","Ограничение бродкаста");
		  addstr("</td><td><select name=\"BC\" size=\"1\">"
		  "<option");
		  if(get_bc_rate_limit()==DISABLE)
			  addstr(" selected");
		  addstr(" value=\"0\">");
		  addstrl("Disable","Выключено");
		  addstr( "</option>"
		  "<option");
		  if(get_bc_rate_limit()==ENABLE)
			  addstr(" selected");
		  addstr(" value=\"1\">");
		  addstrl("Enable","Включено");
		  addstr("</option>"
		  "</select></td></tr>"
		  "<tr class=\"g2\"><td>");
		  addstrl("Rate Limit Threshold","Порог ограничения");
		  addstr("</td>");
		  addstr("<td><input type=\"text\"name=\"RT\"size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%d",get_bc_limit());
		  addstr(temp);
		  addstr("\"></td>");
		  addstr("</tr>"
		  "</td></tr>"
		  "</table>");
/*

		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
					   "<tr class=\"g2\"><td>");
		  addstrl("Port","Порт");
		  addstr("</td><td>");
		  addstrl("Limit","Ограничение");
		  addstr( "</td></tr>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  for(i=0;i<(ALL_PORT_NUM);i++){
			  addstr( "<tr class=\"g2\"><td>");
			  sprintf(temp,"%d",i+1);
			  addstr(temp);
			  addstr("</td><td>"
			  "<select name=\"");
			  sprintf(temp,"P%d",i+1);
			  addstr(temp);
			  addstr("\"><option");
			  if(get_rate_limit_mode(i)==RL_DISABLED)
				 addstr(" selected");
			  addstr(" value=\"0\">");
			  addstrl("Disable","Выключено");
			  addstr( "</option>"
			  "<option");
			  if(get_rate_limit_mode(i)==RL_BROADCAST)
				  addstr(" selected");
			  addstr(" value=\"1\">");
			  addstrl("Broadcast","Широковещательный");
			  addstr("</option>"
			  "<option");
			  if(get_rate_limit_mode(i)==RL_BRCST_MLTCST)
				  addstr(" selected");
			  addstr(" value=\"2\">");
			  addstrl("Broadcast and Multicast","Широковещательный и Многоадресный");
			  addstr("</option></select>");
			  addstr("</td></tr>");
			  PSOCK_SEND_STR(&s->sout,str);
			  str[0]=0;
		 }
		 addstr("</table>");
		 */
		 addstr("<br><input tabindex=\"1\" type=\"Submit\" name=\"Apply\" value=\"Apply\"></form></body></html>");
	 }
	 PSOCK_SEND_STR(&s->sout,str);
	 str[0]=0;
	 PSOCK_END(&s->sout);
}


static PT_THREAD(run_QoS_general(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  uint16_t pcount;
  static int i,j;
  static  uint8_t apply=0;

  str[0]=0;
	  PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}


			if(!strcmp(pname, "state")){
				if (!strncmp(pval, "0", 1)){ set_qos_state(0);}
				if (!strncmp(pval, "1", 1)) {set_qos_state(1);}
			}


			if(!strcmp(pname, "SM")){
				if (!strncmp(pval, "0", 1)) set_qos_policy(0);
				if (!strncmp(pval, "1", 1)) set_qos_policy(1);
			}
			for(j=0;j<ALL_PORT_NUM;j++){

				sprintf(temp,"QR%d",j+1);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) {
						set_qos_port_rule(j,COS_ONLY);
					}
					if (!strncmp(pval, "1", 1)){
						set_qos_port_rule(j,TOS_ONLY);;
					}
					if (!strncmp(pval, "2", 1)){
						set_qos_port_rule(j,TOS_AND_COS);
					}
				}
				switch(j){
					case 0:sprintf(temp,"P1");break;
					case 1:sprintf(temp,"P2");break;
					case 2:sprintf(temp,"P3");break;
					case 3:sprintf(temp,"P4");break;
					case 4:sprintf(temp,"P5");break;
					case 5:sprintf(temp,"P6");break;
					case 6:sprintf(temp,"P7");break;
					case 7:sprintf(temp,"P8");break;
				}
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) set_qos_port_def_pri(j,0);
					if (!strncmp(pval, "1", 1)) set_qos_port_def_pri(j,1);
					if (!strncmp(pval, "2", 1)) set_qos_port_def_pri(j,2);
					if (!strncmp(pval, "3", 1)) set_qos_port_def_pri(j,3);
					if (!strncmp(pval, "4", 1)) set_qos_port_def_pri(j,4);
					if (!strncmp(pval, "5", 1)) set_qos_port_def_pri(j,5);
					if (!strncmp(pval, "6", 1)) set_qos_port_def_pri(j,6);
					if (!strncmp(pval, "7", 1)) set_qos_port_def_pri(j,7);
				}

			}
		  }

		  //применение настроек
		  if (apply  == 1){
			  //need accept settings
			  alertl("Parametrs accepted","Настройки применились");
			  settings_add2queue(SQ_QOS);
			  settings_save();
			  apply = 0;
		  }
	  }


	  addhead();
	  addstr("<body>");
	  addstrl("<legend>QoS General settings</legend>","<legend>Общие настройки QoS</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/qos/QoS_general.shtml\">");


		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				  "<tr class=\"g2\"><td>");
				  addstrl("State","Состояние");
				  addstr("</td><td><select name=\"state\" size=\"1\">"
				  "<option");
				  if(get_qos_state()==0)addstr(" selected");
				  addstr(" value=\"0\">");
				  addstrl("Disable","Выключено");
				  addstr( "</option>"
				  "<option");
				  if(get_qos_state()==1)addstr(" selected");
				  addstr(" value=\"1\">");
				  addstrl("Enable","Включено");
				  addstr("</option></select></td></tr>"


				"<tr class=\"g2\"><td>");
				  addstrl("CoS Scheduling mechanism","тип работы планировщика");
				  addstr("</td><td>");
				  addstr("<select name=\"SM\" size=\"1\"><option");
				  if(get_qos_policy()==1)addstr(" selected");
				  addstr(" value=\"1\">");
				  addstrl("Strict priority","Строгая приоритезация");
				  addstr("</option>"
				  "<option");
				  if(get_qos_policy()==0)addstr(" selected");
				  addstr(" value=\"0\">");
				  addstrl("Weighted fair priority","Взвешенная приоритезация");
				  addstr("</option></select></td></tr></table><br><br>");


				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;

				  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				  "<tr class=\"g2\"><td>");
				  addstrl("Port","Порт");
				  addstr("</td><td>");
				  addstrl("Priority mode","Режим приоритета");
				  addstr("</td><td>");
				  addstrl("Default CoS priority","Приоритет по умолчанию для CoS");
				  addstr("</td></tr>");

				  for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
						//port
						addstr( "<tr class=\"g2\"><td>");
						if(get_dev_type() == DEV_PSW2GPLUS){
							switch(i){
								case 0:addstr("FE#1");break;
								case 1:addstr("FE#2");break;
								case 2:addstr("FE#3");break;
								case 3:addstr("FE#4");break;
								case 4:addstr("GE#1");break;
								case 5:addstr("GE#2");break;
							}
						}else{
							sprintf(temp,"%d",i+1);
							addstr(temp);
						}
						addstr("</td>");
						//priority mode
						addstr(	"<td><select name=\"");
						sprintf(temp,"QR%d",i+1);
						addstr(temp);
						addstr("\" size=\"1\"");
						addstr(" >"
						"<option");
						if(get_qos_port_rule(i)==COS_ONLY)
							addstr(" selected");
						addstr(" value=\"0\">");
						addstrl("CoS only","Только CoS");
						addstr("</option>"
						"<option");
						if(get_qos_port_rule(i)==TOS_ONLY)
							addstr(" selected");
						addstr(	" value=\"1\">");
						addstrl("ToS only","Только ToS");
						addstr("</option>"
						"<option");
						if(get_qos_port_rule(i)==TOS_AND_COS)
							addstr(" selected");
						addstr(	" value=\"2\">CoS & ToS</option>"
						"</select></td>");
						//default prior
						addstr("<td><select name=\"");
						sprintf(temp,"P%d",i+1);
						addstr(temp);
						addstr("\" size=\"1\">");
						for(u8 j = 0;j<8;j++){
							addstr("<option");
							if(get_qos_port_def_pri(i) == j)
								addstr(" selected");
							addstr(" value=\"");
							sprintf(temp,"%d",j);
							addstr(temp);
							addstrl("\">Priority ","\">Приоритет ");
							addstr(temp);
							addstr("</option>");
						}
						addstr("</select></td></tr>");
						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
				  }

		  addstr("</table>"
		  "<br><br><input type=\"Submit\" name=\"Apply\" value=\"Apply\"></form></body></html>");
	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}





//2-z версия тк первая зависает
static PT_THREAD(run_QoS_cos(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
  static  uint16_t pcount;
  static int i,j;
  static  uint8_t apply=0;

  str[0]=0;
	  PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}

			for(j=0;j<8;j++){
				temp[0]=0;
				sprintf(temp,"P%d",j);

				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) set_qos_cos(j,0);
					if (!strncmp(pval, "1", 1)) set_qos_cos(j,1);
					if (!strncmp(pval, "2", 1)) set_qos_cos(j,2);
					if (!strncmp(pval, "3", 1)) set_qos_cos(j,3);
				}
			}

		  }

		  if(apply==1){

			  alertl("Parametrs accepted","Настройки применились");
			  settings_add2queue(SQ_QOS);
			  settings_save();
			  apply=0;
		  }
	  }

	  addhead();
	  addstr("<body>");
	  addstr("<legend>Class of Service</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080\""
		  "action=\"/qos/QoS_cos.shtml\">");

		  addstr(  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		   "<tr class=\"g2\"><td>");
		  addstrl("Priority","Приоритет");
		  addstr("</td><td>");
		  addstrl("Queue","Очередь");
		  addstr("</td></tr>");
		  for(i=0;i<8;i++){
			  addstr(	"<tr class=\"g2\"><td>");
			  sprintf(temp,"%d",i);
			  addstrl("Priority ","Приоритет ");
			  addstr(temp);
			  addstr("</td><td><select name=\"");
			  sprintf(temp,"P%d",i);
			  addstr(temp);

				addstr("\" size=\"1\"><option");
				if(get_qos_cos_queue(i)==0) addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Queue 0","Очередь 0");
				addstr("</option>"
				"<option");
			   if(get_qos_cos_queue(i)==1) addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Queue 1","Очередь 1");
				addstr("</option>"
				"<option");
			   if(get_qos_cos_queue(i)==2) addstr(" selected");
				addstr(" value=\"2\">");
				addstrl("Queue 2","Очередь 2");
				addstr("</option>"
				"<option");
			   if(get_qos_cos_queue(i)==3) addstr(" selected");
				addstr(
				" value=\"3\">");
				addstrl("Queue 3","Очередь 3");
				addstr("</option></select></td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

		  }
		  addstr("</table>"
		  "<br><br><input type=\"Submit\" name=\"Apply\" value=\"Apply\"></form></body></html>");
	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}



//2-z версия тк первая зависает
static PT_THREAD(run_QoS_tos(struct httpd_state *s, char *ptr)){
//NOTE:local variables are not preserved during the calls to proto socket functins
static uint16_t pcount;
static int i;
static  uint8_t apply=0, from=0, to=0,val=0;

  str[0]=0;
	  PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		  for(i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply=1;
			}

			if(!strcmp(pname, "FROM")){
				from=(uint8_t)strtoul(pval, NULL, 10);
			}
			if(!strcmp(pname, "TO")){
				to=(uint8_t)strtoul(pval, NULL, 10);
			}
			if(!strcmp(pname, "VALUE")){
				if (!strncmp(pval, "0", 1)) val=0;
				if (!strncmp(pval, "1", 1)) val=1;
				if (!strncmp(pval, "2", 1)) val=2;
				if (!strncmp(pval, "3", 1)) val=3;
			}

		  }

		  if(apply==1){
			  if(to>=from){
				  for(i=from;i<=to;i++){
					  if(val<4){
						  set_qos_tos(i,val);
					  }
					  else{
						  alertl("Incorrect parametrs","Неверные параметры");
					  }
				  }
				  alertl("Parametrs accepted","Настройки применились");
				  settings_add2queue(SQ_QOS);
				  settings_save();
			  }
			  else
			  {
				  alertl("Incorrect parametrs","Неверные параметры");
			  }
			  apply=0;
		  }
	  }

		addhead();
		addstr("<body>");
		addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/qos/QoS_tos.shtml\">");
		addstr("<legend>Type of Service</legend>");
		if(get_current_user_rule()!=ADMIN_RULE){
			  addstr("Access denied");
		}else{

		addstr(  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		 "<tr class=\"g2\"><td>DSCP</td><td>");
		addstrl("From","С");
		addstr(": <select name=\"FROM\">");
		for(i=0;i<64;i++){
			  addstr("<option value=\"");
			  sprintf(temp,"%d",i);
			  addstr(temp);
			  addstr("\">");
			  addstr(temp);
			  addstr("</option>");
			  if(i%40==0){
				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;
			  }
		}

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("</td><td>");
		addstrl("To","До");
		addstr(": <select name=\"TO\">");
		for(i=0;i<64;i++){
			  addstr("<option value=\"");
			  temp[0]=0;
			  sprintf(temp,"%d",i);
			  addstr(temp);
			  addstr("\">");
			  addstr(temp);
			  addstr("</option>");
			  if(i%40==0){
				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;
			  }
		}

		 PSOCK_SEND_STR(&s->sout,str);
		 str[0]=0;

			addstr("</td><td><select name=\"VALUE\" size=\"1\">"
					"<option value=\"0\">");
					addstrl("Queue 0","Очередь 0");
					addstr("</option>"
					"<option value=\"1\">");
					addstrl("Queue 1","Очередь 1");
					addstr("</option>"
					"<option value=\"2\">");
					addstrl("Queue 2","Очередь 2");
					addstr("</option>"
					"<option value=\"3\">");
					addstrl("Queue 3","Очередь 3");
					addstr("</option></td></tr></table>");
			addstr("<br><input type=\"Submit\" name=\"Apply\" value=\"Apply\">");
		/*таблица состояния*/

			addstr(  "<br><br><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td>DSCP</td><td>");
			addstrl("Queue","Очередь");
			addstr("</td><td>DSCP</td><td>");
			addstrl("Queue","Очередь");
			addstr("</td><td>DSCP</td>"
			"<td>");
			addstrl("Queue","Очередь");
			addstr("</td><td>DSCP</td><td>");
			addstrl("Queue","Очередь");
			addstr("</td></tr>");
			 PSOCK_SEND_STR(&s->sout,str);
			 str[0]=0;
			 for(i=0;i<16;i++){
				 addstr("<tr class=\"g2\"><td>");

				 sprintf(temp,"%d",i);
				 addstr(temp);
				 addstr("</td><td>");
				 addstrl("Queue","Очередь");
				 sprintf(temp," %d",get_qos_tos_queue(i));
				 addstr(temp);
				 addstr("</td><td>");

				 sprintf(temp,"%d",(i+16));
				 addstr(temp);
				 addstr("</td><td>");
				 addstrl("Queue","Очередь");
				 sprintf(temp," %d",get_qos_tos_queue(i+16));
				 addstr(temp);
				 addstr("</td><td>");

				 sprintf(temp,"%d",(i+32));
				 addstr(temp);
				 addstr("</td><td>");
				 addstrl("Queue","Очередь");
				 sprintf(temp," %d",get_qos_tos_queue(i+32));
				 addstr(temp);
				 addstr("</td><td>");

				 sprintf(temp,"%d",(i+48));
				 addstr(temp);
				 addstr("</td><td>");
				 addstrl("Queue","Очередь");
				 sprintf(temp," %d",get_qos_tos_queue(i+48));
				 addstr(temp);
				 addstr("</td></tr>");

				 if(i%5==0){
				   PSOCK_SEND_STR(&s->sout,str);
				   str[0]=0;
				 }
			 }
		  addstr("</table></form></body></html>");
	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        Basic Settings / SNTP settings													    	*/
/*******************************************************************************************************/

static PT_THREAD(run_sntp_set(struct httpd_state *s, char *ptr)){
	static uint16_t pcount;
	static  uint8_t Apply=0,Synchronize=0;
	static  i8 i=0;
	static  u8 IP[4];
	static  uip_ipaddr_t serv_ip,ip;
	static char serv_name[64];
	/*for syslog:*/
	///*static*/  struct SNTP_Cfg sntp_cfg_tmp;

    PSOCK_BEGIN(&s->sout);

	//memcpy(&sntp_cfg_tmp,&sntp_cfg,sizeof(sntp_cfg));

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		 		  for(i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)) 	Apply = 1;
					  }

					  if(!strcmp(pname, "Synchronize")){
						  if (!strncmp(pval, "S", 1)) 	Synchronize = 1;
					  }
					  if(!strcmp(pname, "state")){
						  if (!strncmp(pval, "0", 1)) 	set_sntp_state(0);
						  if (!strncmp(pval, "1", 1)) 	set_sntp_state(1);
					  }
				      if(!strcmp(pname, "ip1")){
				    	  IP[0]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip2")){
				    	  IP[1]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip3")){
				    	  IP[2]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip4")){
				    	  IP[3]=(uint8_t)strtoul(pval, NULL, 10);
				      }

				      if(!strcmp(pname, "serv_name")){
				    	  strcpy(serv_name,pval);
				      }

				      if(!strcmp(pname, "timezone")){
				    	  if (!strncmp(pval, "-12", 3)) set_sntp_timezone(-12);
				    	  if (!strncmp(pval, "-11", 3)) set_sntp_timezone(-11);
				    	  if (!strncmp(pval, "-10", 3)) set_sntp_timezone(-10);
				    	  if (!strncmp(pval, "-9", 3)) 	set_sntp_timezone(-9);
				    	  if (!strncmp(pval, "-8", 3)) 	set_sntp_timezone(-8);
				    	  if (!strncmp(pval, "-7", 3)) 	set_sntp_timezone(-7);
				    	  if (!strncmp(pval, "-6", 3)) 	set_sntp_timezone(-6);
				    	  if (!strncmp(pval, "-5", 3)) 	set_sntp_timezone(-5);
				    	  if (!strncmp(pval, "-4", 3)) 	set_sntp_timezone(-4);
				    	  if (!strncmp(pval, "-3", 3)) 	set_sntp_timezone(-3);
				    	  if (!strncmp(pval, "-2", 3)) 	set_sntp_timezone(-2);
				    	  if (!strncmp(pval, "-1", 3)) 	set_sntp_timezone(-1);
				    	  if (!strncmp(pval, "0", 3)) 	set_sntp_timezone(0);
				    	  if (!strncmp(pval, "1", 3)) 	set_sntp_timezone(1);
				    	  if (!strncmp(pval, "2", 3)) 	set_sntp_timezone(2);
				    	  if (!strncmp(pval, "3", 3)) 	set_sntp_timezone(3);
				    	  if (!strncmp(pval, "4", 3)) 	set_sntp_timezone(4);
				    	  if (!strncmp(pval, "5", 3)) 	set_sntp_timezone(5);
				    	  if (!strncmp(pval, "6", 3)) 	set_sntp_timezone(6);
				    	  if (!strncmp(pval, "7", 3)) 	set_sntp_timezone(7);
				    	  if (!strncmp(pval, "8", 3)) 	set_sntp_timezone(8);
				    	  if (!strncmp(pval, "9", 3)) 	set_sntp_timezone(9);
				    	  if (!strncmp(pval, "10", 3)) 	set_sntp_timezone(10);
				    	  if (!strncmp(pval, "11", 3)) 	set_sntp_timezone(11);
				    	  if (!strncmp(pval, "12", 3)) 	set_sntp_timezone(12);
				    	  if (!strncmp(pval, "13", 3)) 	set_sntp_timezone(13);
				      }
				      if(!strcmp(pname, "period")){
				    	  if (!strncmp(pval, "1", 1)) set_sntp_period(1);
				    	  if (!strncmp(pval, "2", 1)) set_sntp_period(10);
				    	  if (!strncmp(pval, "3", 1)) set_sntp_period(60);
				    	  if (!strncmp(pval, "4", 1)) set_sntp_period(240);
				      }
				  }

		  	if(Apply==1){
		  		if(get_sntp_state()==1){
		  			uip_ipaddr(serv_ip,IP[0],IP[1],IP[2],IP[3]);
		  			set_sntp_serv(serv_ip);//server ip
		  			set_sntp_serv_name(serv_name);//server name
		  		}
		  		SNTP_config();
		  		if(get_sntp_state())
		  			addstr("<meta http-equiv=\"refresh\" content=\"5; url=/mngt/sntp.shtml\">");
		  		settings_save();
		  		alertl("Parametrs accepted","Настройки применились");
		  		Apply=0;
		  	}

		  	if(Synchronize==1){
		  		//SNTP_get_time();
		  		SNTP_config();
		  		Synchronize=0;
		  	}
	  }


	  addhead();
	  addstr("<body>");
	  addstr("<script type=\"text/javascript\">"
	  "var TimeoutID; "
	  "var Time; "
	  "function form2 (v) {return(v<10?'0'+v:v);} "
	  "function showtime(){"
	     "sec0=Time%60;"
	  	"min0=(Time%3600)/60|0;"
	     	"hour0=Time/3600|0;"
	      "document.getElementById('clock1').innerHTML = "
	      "form2(hour0)+':'+form2(min0)+':'+form2(sec0);"
	      "Time++;"
	      "window.setTimeout('showtime();',1000);"
	   "} "
	   "function inittime(hour,min,sec){ "
	    "Time=hour*3600+min*60+sec;"
	    "TimeoutID=window.setTimeout('showtime()', 1000);"
	  "} "
	  "</script>");

	  addstrl("<legend>SNTP Settings</legend>","<legend>Настройки SNTP</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/mngt/sntp.shtml\">");
		  addstr( "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\"><tr class=\"g2\">"
				  "<td width=\20%\">");
		  addstrl("State","Состояние");
		  addstr("</td><td>"
				  "<select name=\"state\" size=\"1\">"
				  "<option");
			if(get_sntp_state()==0)addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				  "<option ");
			if(get_sntp_state()==1)addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				  "</select>"
				  "</td></tr>"
				  "<tr class=\"g2\"><td>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				  addstrl("Server IP address","IP адрес сервера");
				  addstr("</td><td>"
				  "<input type=\"text\"name=\"ip1\"size=\"3\" maxlength=\"3\"value=\"");
			 get_sntp_serv(&ip);
			 snprintf(temp,4,"%d",(int)uip_ipaddr1(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip2\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",(int)uip_ipaddr2(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip3\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",(int)uip_ipaddr3(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip4\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",(int)uip_ipaddr4(ip));
			 addstr(temp);
			 addstr(
				  "\"></td></tr>"
			 "<tr class=\"g2\"><td>");
			 addstrl("Server Name","Имя сервера");
			 addstr("</td><td>"
			 "<input type=\"text\"name=\"serv_name\"size=\"32\" maxlength=\"32\"value=\"");
			 get_sntp_serv_name(temp);
			 addstr(temp);
			 addstr("\"></td><tr class=\"g2\"><td>");
			 addstrl("Time Zone","Часовой пояс");
			 addstr("</td><td><select name=\"timezone\" size=\"1\">");
			 PSOCK_SEND_STR(&s->sout,str);
			 str[0]=0;
			 for(i=-12;i<14;i++){
				 addstr("<option");
				 if(get_sntp_timezone()==i)addstr(" selected");
				 addstr(" value=\"");
				 sprintf(temp,"%i",i);
				 addstr(temp);
				 addstr( "\">");
				 if(i>0)
					 addstr("+");
				 addstr(temp);
				 addstr("</option>");
			 }
			addstr(
				"</select></td></tr>"
				"<tr class=\"g2\"><td>");
			addstrl("Period","Период");
			addstr("</td><td>"
				"<select name=\"period\" size=\"1\">"
				"<option");
				if(get_sntp_period()==1)addstr(" selected");
					addstr(" value=\"1\">1</option>"
					  "<option ");
				if(get_sntp_period()==10)addstr(" selected");
					addstr(" value=\"2\">10</option>"
				"<option ");
				if(get_sntp_period()==60)addstr(" selected");
					addstr(" value=\"3\">60</option>"
				"<option ");
				if(get_sntp_period()==240)addstr(" selected");
					addstr(" value=\"4\">240</option>"
				"</select>"
				"</td></tr>"
				"</table><br>"

				"<table width=\"80%\"><tr><td><input type=\"Submit\" name=\"Apply\" value=\"Apply\" width=\"200\"></td></tr><tr><td><br><br></td></tr>"
				"<tr><td>");
				if((get_sntp_state()==ENABLE)&&(date.year!=0)){
					addstrl("Time","Время");
					addstr(": "
					"<span id=\"clock1\"></span>&nbsp;"
					"<script type=\"text/javascript\">"
					"inittime (");
					sprintf(temp,"%d,%d,%d",date.hour,date.min,date.sec);
					addstr(temp);
					addstr(");</script>");
					addstr("</td></tr><tr><td>");
					addstrl("Date: ","Дата: ");
					sprintf(temp,"%02d/%02d/%d</td></tr><tr><td><br>",date.day,date.month,(date.year+2000));
					addstr(temp);
				}
				addstr(
				"<input type=\"Submit\" name=\"Synchronize\" value=\"Synchronize\" width=\"200\"></td></tr></table>"
				"</form></body></html>");
	  	  	}
		  	PSOCK_SEND_STR(&s->sout,str);
		  	str[0]=0;
		    PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        Basic Settings / SMTP settings													    	*/
/*******************************************************************************************************/

static  PT_THREAD(run_smtp_set(struct httpd_state *s, char *ptr)){
	static  uint16_t pcount;
	static  uint8_t Apply=0,Send=0;
	static  u8 i=0,IP[4];
	static  char subject[16],msg[64], domain[64];
	///*static*/  struct SMTP_cfg smtp_cfg_tmp;
	static  char tmptxt[64];
	static  uip_ipaddr_t serv_ip,ip;
	char to1[64],to2[64],to3[64],login[64],pass[64],subj[64],from[64];

    PSOCK_BEGIN(&s->sout);

    //memcpy(&smtp_cfg_tmp,&smtp_cfg,sizeof(smtp_cfg));

    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		 		  for(i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)) 	Apply = 1;
					  }
					  if(!strcmp(pname, "Send")){
						  if (!strncmp(pval, "S", 1)) 	Send=1;
					  }
					  if(!strcmp(pname, "state")){
						  if (!strncmp(pval, "0", 1)) 	set_smtp_state(0);
						  if (!strncmp(pval, "1", 1)) 	set_smtp_state(1);
					  }
				      if(!strcmp(pname, "ip1")){
				    	  IP[0]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip2")){
				    	  IP[1]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip3")){
				    	  IP[2]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip4")){
				    	  IP[3]=(uint8_t)strtoul(pval, NULL, 10);
				      }

				      if(!strcmp(pname, "port")){
				    	  set_smtp_port((uint16_t)strtoul(pval, NULL, 10));
				      }

					  if(!strcmp(pname, "from"))
					  {
						  strcpy(from,pval);
					  }

					  if(!strcmp(pname, "to"))
					  {
						  strcpy(to1,pval);
					  }
					  if(!strcmp(pname, "to2"))
					  {
						  strcpy(to2,pval);
					  }
					  if(!strcmp(pname, "to3"))
					  {
						  strcpy(to3,pval);
					  }

					  if(!strcmp(pname, "subj"))
					  {
						  strcpy(subj,pval);
					  }

					  if(!strcmp(pname, "domain"))
					  {
						  strcpy(domain,pval);
					  }


					  if(!strcmp(pname, "login"))
					  {
						  strcpy(login,pval);
					  }

					  if(!strcmp(pname, "pass"))
					  {
						  strcpy(pass,pval);
					  }


					  /*test message*/
					  if(!strcmp(pname, "subject"))
					  {
						  memset(subject,0,16);
						  strcat(pval,"\0");
						  memcpy(subject,pval,strlen(pval));
					  }
					  if(!strcmp(pname, "msg"))
					  {
						  memset(msg,0,64);
						  strcat(pval,"\0");
						  memcpy(msg,pval,strlen(pval));
					  }
				  }

		 	if(Send==1){
		 		subject[strlen(subject)]=0;
		 		msg[strlen(msg)]=0;
		 		mail_send(subject,msg);
		 		Send=0;
				alertl("Message sending...","Отправка...")
		 	}

		  	if(Apply==1){
		  		 if(get_smtp_state()==1){
		  			  //memcpy(settings.smtp_sett.serv_addr,IP,4);
		  			  uip_ipaddr(serv_ip,IP[0],IP[1],IP[2],IP[3]);
		  			  set_smtp_server(serv_ip);

					  //domain name
		  			 if(check_ip_addr(serv_ip)!=0){
		  				 if(strlen(domain)==0){
		  					 alertl("Invalid domain name","Некорректное доменое имя");
		  				  	 set_smtp_state(0);
		  				 }
		  			 }
					 set_smtp_domain(domain);

		  			  //from
					  memset(tmptxt,0,64);
		  			  if(strlen(from)==0){
		  				  alertl("Invalid sender e-mail address","Некорректный e-mail отправителя");
		  				  set_smtp_state(0);
			   		  }
					  http_url_decode(from,tmptxt,strlen(from));
					  for(uint8_t i=0;i<strlen(tmptxt);i++){
						if(tmptxt[i]=='+') tmptxt[i] = ' ';
						if(tmptxt[i]=='%') tmptxt[i]=' ';
					  }
					  set_smtp_from(tmptxt);

					  //to
					  memset(tmptxt,0,64);
					  if(strlen(to1)==0){
						  alertl("Invalid receiver e-mail address","Некорректный e-mail получателя");
	  				  	  set_smtp_state(0);
					  }
					  http_url_decode(to1,tmptxt,strlen(to1));
					  for(uint8_t i=0;i<strlen(tmptxt);i++){
						if(tmptxt[i]=='+') tmptxt[i] = ' ';
						if(tmptxt[i]=='%') tmptxt[i]=' ';
					  }
					  set_smtp_to(tmptxt);

					  //to2
					  memset(tmptxt,0,64);
					  http_url_decode(to2,tmptxt,strlen(to2));
					  for(uint8_t i=0;i<strlen(tmptxt);i++){
						if(tmptxt[i]=='+') tmptxt[i] = ' ';
						if(tmptxt[i]=='%') tmptxt[i]=' ';
					  }
					  set_smtp_to2(tmptxt);

					  //to3
					  memset(tmptxt,0,64);
					  http_url_decode(to3,tmptxt,strlen(to3));
					  for(uint8_t i=0;i<strlen(tmptxt);i++){
						if(tmptxt[i]=='+') tmptxt[i] = ' ';
						if(tmptxt[i]=='%') tmptxt[i]=' ';
					  }
					  set_smtp_to3(tmptxt);

					  //subject
		  			  if(strlen(subj)){
		  				  set_smtp_subj(subj);
		  			  }
		  			  else{
		  				  alertl("Invalid subject","Некорректная тема письма");
		  				  set_smtp_state(0);
			  		  }



					  //login
					  memset(tmptxt,0,32);
					  http_url_decode(login,tmptxt,strlen(login));
					  for(uint8_t i=0;i<strlen(tmptxt);i++){
						if(tmptxt[i]=='+') tmptxt[i] = ' ';
						if(tmptxt[i]=='%') tmptxt[i]=' ';
					  }
					  set_smtp_login(tmptxt);

					  //password
					  memset(tmptxt,0,32);
					  http_url_decode(pass,tmptxt,strlen(pass));
					  for(uint8_t i=0;i<strlen(tmptxt);i++){
						if(tmptxt[i]=='+') tmptxt[i] = ' ';
						if(tmptxt[i]=='%') tmptxt[i]=' ';
					  }
					  set_smtp_pass(tmptxt);

		  			  get_smtp_login(login);
		  			  get_smtp_pass(pass);
		  			  if(strlen(login)==0){
						  if(strlen(pass)!=0){
							  alertl("Invalid login","Некорректный login");
							  set_smtp_state(0);
							  goto not_cfg;
						  }
			  		  }


		  		  }

		  		smtp_config();
		  		//e_mail_init();
		  		settings_save();
		  		alertl("Parametrs accepted","Настройки применились");

not_cfg:
		  		 Apply=0;
		  	}



	  }


	  addhead();
	  addstr("<body>");
	  addstrl("<legend>SMTP Settings</legend>","<legend>Настройки SMTP</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/mngt/smtp.shtml\">");
		  addstr("<b>");
		  addstrl("SMTP server settings","Настройки SMTP сервера");
		  addstr("</b><br><br>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" width=\80%\">"
				  "<tr class=\"g2\">"
				  "<td width=\"30%\">");
		  addstrl("State","Состояние");
		  addstr("</td><td>"
				  "<select name=\"state\" size=\"1\">"
				  "<option");
			if(get_smtp_state()==0)addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				  "<option ");
			if(get_smtp_state()==1)addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				  "</select>"
				  "</td></tr>"

				  "<tr class=\"g2\"><td>");
				  addstrl("Server IP address","IP адрес сервера");
				  addstr("</td><td>"
				  "<input type=\"text\"name=\"ip1\"size=\"3\" maxlength=\"3\"value=\"");
				  get_smtp_server(&ip);
			 snprintf(temp,4,"%d",(int)uip_ipaddr1(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip2\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",(int)uip_ipaddr2(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip3\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",(int)uip_ipaddr3(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip4\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",(int)uip_ipaddr4(ip));
			 addstr(temp);
			 PSOCK_SEND_STR(&s->sout,str);
			 str[0]=0;
			 addstr(
				  "\"></td></tr>"
			 "<tr class=\"g2\"><td>");
			 addstrl("Server domain name","Доменое имя сервера");
			 addstr("</td><td>"
				  "<input type=\"text\"name=\"domain\"size=\"64\" maxlength=\"64\"value=\"");
			 memset(tmptxt,0,64);
			 get_smtp_domain(domain);
			 http_url_decode(domain,tmptxt,strlen(domain));
			 for(uint8_t i=0;i<strlen(domain);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
			 }
			 addstr(tmptxt);
			 addstr("\"></td></tr>"
				  "<tr class =\"g2\"><td>");
			 addstrl("Port","Порт");
			 addstr("</td><td>"
				  "<input type=\"text\" name=\"port\" size=\"16\" maxlength=\"5\"value=\"");
			 sprintf(tmptxt,"%d",get_smtp_port());
				addstr(tmptxt);
				addstr("\"></td></tr>"
				  "<tr class=\"g2\"><td>");
			 addstrl("Sender e-mail address","Адрес отправителя");
				  addstr("</td><td>"
				  "<input type=\"text\" name=\"from\" size=\"64\" maxlength=\"64\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_from(from);
				http_url_decode(from,tmptxt,strlen(from));
				 for(uint8_t i=0;i<strlen(from);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr>"
				"<tr class=\"g2\"><td>");
				addstrl("Receiver e-mail address 1","Адрес получателя 1");
				addstr("</td><td>"
				"<input type=\"text\" name=\"to\" size=\"64\" maxlength=\"32\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_to(to1);
				http_url_decode(to1,tmptxt,strlen(to1));
				 for(uint8_t i=0;i<strlen(to1);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr>"

				"<tr class=\"g2\"><td>");
				addstrl("Receiver e-mail address 2","Адрес получателя 2");
				addstr("</td><td>"
				"<input type=\"text\" name=\"to2\" size=\"64\" maxlength=\"32\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_to2(to2);
				http_url_decode(to2,tmptxt,strlen(to2));
				 for(uint8_t i=0;i<strlen(to2);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr>"
				"<tr class=\"g2\"><td>");
				addstrl("Receiver e-mail address 3","Адрес получателя 3");
				addstr("</td><td>"
				"<input type=\"text\" name=\"to3\" size=\"64\" maxlength=\"32\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_to3(to3);
				http_url_decode(to3,tmptxt,strlen(to3));
				 for(uint8_t i=0;i<strlen(to3);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr>"
				"<tr class=\"g2\"><td>");
				addstrl("Subject","Тема письма");
				addstr("</td><td>"
				"<input type=\"text\" name=\"subj\" size=\"64\" maxlength=\"16\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_subj(subj);
				http_url_decode(subj,tmptxt,strlen(subj));
				 for(uint8_t i=0;i<64;i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr>"
				"</table><br>"
				"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\" width=\80%\">"
				"<tr class=\"g2\">"
				"<td width=\"30%\">");
				addstrl("Login","Логин");
				addstr("</td><td><input type=\"text\" name=\"login\" size=\"64\" maxlength=\"31\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_login(login);
				http_url_decode(login,tmptxt,strlen(login));
				 for(uint8_t i=0;i<strlen(login);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr>"
					"<tr class=\"g2\"><td>");
				addstrl("Password","Пароль");
				addstr("</td><td><input type=\"password\" name=\"pass\" size=\"64\" maxlength=\"31\"value=\"");
				memset(tmptxt,0,64);
				get_smtp_pass(pass);
				http_url_decode(pass,tmptxt,strlen(pass));
				 for(uint8_t i=0;i<strlen(pass);i++){
					if(tmptxt[i]=='+') tmptxt[i] = ' ';
					if(tmptxt[i]=='%') tmptxt[i]=' ';
				}
				addstr(tmptxt);
				addstr("\"></td></tr></table><br>");
				addstr(
				"<input type=\"Submit\" name=\"Apply\" value=\"Apply\" width=\"200\">");
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

	//test e-mail send
				addstr("<br><br><br><b>");
				addstrl("Send test e-mail","Отправка тестового сообщения");
				addstr("</b><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\"><td>");
				addstrl("Subject","Тема письма");
				addstr("</td><td><input type=\"text\" name=\"subject\" size=\"16\" maxlength=\"16\"></td></tr>"
				"<tr class=\"g2\"><td>");
				addstrl("Message","Сообщение");
				addstr("</td><td><input type=\"text\" name=\"msg\" size=\"64\" maxlength=\"64\"></td></tr></table>"
				"<br><input type=\"Submit\" name=\"Send\" value=\"Send\" width=\"200\">"
				"</form></body></html>");
	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);
}




/*******************************************************************************************************/
/*        Basic Settings / SNMP settings													    	*/
/*******************************************************************************************************/

static PT_THREAD(run_snmp_set(struct httpd_state *s, char *ptr)){
	static  uint16_t pcount;
	static  uint8_t Apply=0;
	static  i8 i=0;
	static  u8 IP[4];
	static  char text[64],text_tmp[64];
	uip_ipaddr_t ip;
	char comm[64];
	static engine_id_t eid;

	PSOCK_BEGIN(&s->sout);

	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		 		  for(i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)) 	Apply = 1;
					  }

					  if(!strcmp(pname, "state")){
						  if (!strncmp(pval, "0", 1)) 	set_snmp_state(0);
						  if (!strncmp(pval, "1", 1)) 	set_snmp_state(1);
					  }

					  if(!strcmp(pname, "mode")){
						  if (!strncmp(pval, "0", 1)) 	set_snmp_mode(SNMP_R);
						  if (!strncmp(pval, "1", 1)) 	set_snmp_mode(SNMP_W);
						  if (!strncmp(pval, "2", 1)) 	set_snmp_mode(SNMP_RW);
					  }

					  if(!strcmp(pname, "vers")){
						  if (!strncmp(pval, "1", 1)) 	set_snmp_vers(1);
						  if (!strncmp(pval, "2", 1)) 	set_snmp_vers(2);
						  if (!strncmp(pval, "3", 1)) 	set_snmp_vers(3);
					  }

				      if(!strcmp(pname, "ip1")){
				    	  IP[0]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip2")){
				    	  IP[1]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip3")){
				    	  IP[2]=(uint8_t)strtoul(pval, NULL, 10);
				      }
				      if(!strcmp(pname, "ip4")){
				    	  IP[3]=(uint8_t)strtoul(pval, NULL, 10);
				      }

				      if(!strcmp(pname, "comm_r")){
				    	  set_snmp1_read_communitie(pval);
				      }

				      if(!strcmp(pname, "comm_w")){
				    	  set_snmp1_write_communitie(pval);
				      }

					  if(!strcmp(pname, "s3_lev")){
						  if (!strncmp(pval, "0", 1)) 	set_snmp3_level(0,NO_AUTH_NO_PRIV);
						  if (!strncmp(pval, "1", 1)) 	set_snmp3_level(0,AUTH_NO_PRIV);
						  if (!strncmp(pval, "2", 1)) 	set_snmp3_level(0,AUTH_PRIV);
					  }

					  if(!strcmp(pname, "s3_name")){
						  set_snmp3_user_name(0,pval);
					  }

					  if(!strcmp(pname, "s3_auth")){
						  set_snmp3_auth_pass(0,pval);
					  }

					  if(!strcmp(pname, "s3_priv")){
						  set_snmp3_priv_pass(0,pval);
					  }

					  if(!strcmp(pname, "s3_eid")){
						  str_to_engineid(pval,&eid);
						  set_snmp3_engine_id(&eid);
					  }

				  }

		  	if(Apply==1){
			    if(get_snmp_state()==1){
					  uip_ipaddr(ip,IP[0],IP[1],IP[2],IP[3]);
					  set_snmp_serv(ip);

				}
		  		snmp_trap_init();
		  		snmpd_init();
		  		settings_save();
		  		Apply=0;
		  		alertl("Parametrs accepted","Настройки применились");
		  	}
	  }


	  addhead();
	  addstr("<body>");
	  addstr("<script type=\"text/javascript\">"
			"function change_css(){"
				"var element = document.getElementById('click');"
				"if(element){"
					"var v1 = document.getElementsByName('v1');"
					"var v3 = document.getElementsByName('v3');"
			  	  	"for(var i = 0; i < v1.length; i++) {"
						"if(element.value == 1)"
						"{"
							"v1[i].style.display = \"table-row\";"
						"}"
						"else"
						"{"
							"v1[i].style.display = \"none\";"
						"}"
			  	  	"}"
			  	  	"for(var i = 0; i < v3.length; i++){"
						"if(element.value == 3){"
							"v3[i].style.display = \"table-row\";"
			     		"}"
						"else"
						"{"
							"v3[i].style.display = \"none\";"
						"}"
					"}"

			  	  "var lev = document.getElementById('lev');"
			  	  "var auth = document.getElementById('auth');"
			  	  "var priv = document.getElementById('priv');"

			  	  "if(lev.value == 0){"
			  		  "auth.disabled = true;"
			  		  "priv.disabled = true;"
			  	  "}"

				  "if(lev.value == 1){"
					  "auth.disabled = false;"
					  "priv.disabled = true;"
				  "}"

				  "if(lev.value == 2){"
					  "auth.disabled = false;"
					  "priv.disabled = false;"
				  "}"

				"}"
			"}"
	  "</script>");

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

	  addstrl("<legend>SNMP Settings</legend>","<legend>Настройки SNMP</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/mngt/snmp.shtml\">");
		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\"><tr class=\"g2\">"
				  "<td>");
				  addstrl("State","Состояние");
				  addstr("</td><td>"
				  "<select name=\"state\" size=\"1\">"
				  "<option");
			if(get_snmp_state()==0)addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				  "<option ");
			if(get_snmp_state()==1)addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				  "</select>"
				  "</td></tr>"

				  "<tr class=\"g2\">"
				  "<td>");
				  addstrl("Mode","Режим");
				  addstr("</td><td>"
				  "<select name=\"mode\" size=\"1\">"
				  "<option");
				  if(get_snmp_mode()==SNMP_R)addstr(" selected");
				  addstr(" value=\"0\">");
			      addstrl("Read Only","Только чтение");
			      addstr("</option>"
				  "<option ");
				  if(get_snmp_mode()==SNMP_W)addstr(" selected");
				  addstr(" value=\"1\">");
			      addstrl("Write Only","Только запись");
			      addstr("</option>"
				  "<option ");
				  if(get_snmp_mode()==SNMP_RW)addstr(" selected");
				  addstr(" value=\"2\">");
			      addstrl("Read Write","Чтение и запись");
			      addstr("</option>"
				  "</select>"
				  "</td></tr>"
				  "<tr class=\"g2\"><td>");
				  addstrl("Traps server IP address","Сервер для приема трапов");

				  PSOCK_SEND_STR(&s->sout,str);
				  str[0]=0;

				  addstr("</td><td>"
				  "<input type=\"text\"name=\"ip1\"size=\"3\" maxlength=\"3\"value=\"");
				  get_snmp_serv(&ip);
			 snprintf(temp,4,"%d",uip_ipaddr1(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip2\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",uip_ipaddr2(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip3\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",uip_ipaddr3(ip));
			 addstr(temp);
			 addstr(
				  "\"><input type=\"text\"name=\"ip4\"size=\"3\" maxlength=\"3\"value=\"");
			 snprintf(temp,4,"%d",uip_ipaddr4(ip));
			 addstr(temp);
			 addstr(
				  "\"></td></tr>"


				  "<tr class=\"g2\">"
				  "<td>");
				  addstrl("Version","Версия");
				  addstr("</td><td>"
				  "<select name=\"vers\" size=\"1\" id=\"click\" onchange=\"change_css();\">"
				  "<option");
				if(get_snmp_vers()==1)
					addstr(" selected");
				addstr(" value=\"1\">");
				addstr("SNMP v1");
				addstr("</option>"
				"<option ");
				if(get_snmp_vers()==3)
					addstr(" selected");
				addstr(" value=\"3\">");
				addstr("SNMP v3");
				addstr("</option>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				addstr(
				  "</select>"
				  "</td></tr>"
				//read communitie
				"<tr class=\"g2\" name=\"v1\">"
				"<td width=\"20%\">");
			addstrl("Read Community","Сообщество чтения");
			addstr("</td><td>"
				"<input type=\"text\"name=\"comm_r\"size=\"16\" maxlength=\"16\"value=\"");
			 memset(text,0,sizeof(text));
			 IWDG_ReloadCounter();
			 //get_snmp_communitie(comm);
			 get_snmp1_read_communitie(comm);
			 http_url_decode(comm,text,strlen(comm));
			 for(uint8_t i=0;i<strlen(comm);i++){
				if(text[i]=='+') text[i] = ' ';
				if(text[i]=='%') text[i]=' ';
			 }
			 memset(text_tmp,0,sizeof(text_tmp));
			 strcpy(text_tmp,text);
			 addstr(text_tmp);

			 addstr("\"></td></tr>"
    			//write communitie
				"<tr class=\"g2\" name=\"v1\"><td width=\"20%\">");
				addstrl("Write Community","Сообщество записи");
				addstr("</td><td>"
					"<input type=\"text\"name=\"comm_w\"size=\"16\" maxlength=\"16\"value=\"");
				 memset(text,0,sizeof(text));
				 IWDG_ReloadCounter();
				 //get_snmp_communitie(comm);
				 get_snmp1_write_communitie(comm);
				 http_url_decode(comm,text,strlen(comm));
				 for(uint8_t i=0;i<strlen(comm);i++){
					if(text[i]=='+') text[i] = ' ';
					if(text[i]=='%') text[i]=' ';
				 }
				 memset(text_tmp,0,sizeof(text_tmp));
				 strcpy(text_tmp,text);
				 addstr(text_tmp);
				 addstr("\"></td></tr>");

				 PSOCK_SEND_STR(&s->sout,str);
				 str[0]=0;

				//SNMP v3
				addstr("<tr class=\"g2\" name=\"v3\"><td>");
				addstrl("Security Level","Уровень безопасности");
				addstr("</td><td>"
				"<select name=\"s3_lev\" id=\"lev\" size=\"1\" onchange=\"change_css();\">"
				"<option");
				if(get_snmp3_level(0)==NO_AUTH_NO_PRIV)
					addstr(" selected");
				addstr(" value=\"0\">");
				addstr("NoAuth,NoPriv");
				addstr("</option>"
				"<option");
				if(get_snmp3_level(0)==AUTH_NO_PRIV)
					addstr(" selected");
				addstr(" value=\"1\">");
				addstr("Auth,NoPriv");
				addstr("</option>"
				"<option");
				if(get_snmp3_level(0)==AUTH_PRIV)
					addstr(" selected");
				addstr(" value=\"2\">");
				addstr("Auth,Priv");
				addstr("</option>"
				"</select></td></tr>"

				"<tr class=\"g2\" name=\"v3\"><td>");
				addstr("Engine ID</td>"
				"<td><input type=\"text\" name=\"s3_eid\" size=\"32\" maxlength=\"32\"value=\"");
				convert_ptr_to_eid(&eid, getEngineID());
				engineid_to_str(&eid,temp);
				addstr(temp);
				addstr("\"></td></tr>"
				"<tr class=\"g2\" name=\"v3\"><td>");
				addstrl("User Name","Имя пользователя");
				addstr("</td><td><input type=\"text\" name=\"s3_name\" size=\"16\" maxlength=\"16\"value=\"");
				get_snmp3_user_name(0,comm);
				addstr(comm);
				addstr("\"></td></tr>"
				"<tr class=\"g2\" name=\"v3\"><td>");
				addstr("Auth Password"
				"</td><td><input type=\"password\" id=\"auth\" name=\"s3_auth\" size=\"16\" maxlength=\"16\"value=\"");
				get_snmp3_auth_pass(0,comm);
				addstr(comm);
				addstr("\"></td></tr>"
				"<tr class=\"g2\" name=\"v3\"><td>");
				addstr("Priv Password"
				"</td><td><input type=\"password\" id=\"priv\" name=\"s3_priv\" size=\"16\" maxlength=\"16\"value=\"");
				get_snmp3_priv_pass(0,comm);
			    addstr(comm);
				addstr("\"></td></tr>"

				 "</table><br><br>"
			 "<table width=\"80%\"><tr><td><input type=\"Submit\" name=\"Apply\" value=\"Apply\" width=\"200\">"
			 "</td></tr>"
			 "</tr></table>"
			 "<script type=\"text/javascript\">change_css();</script>"
			 "</form>"
			 "<br><br><br>"
			 "<a href=\"\miblist.shtml\">View all OID entries</a>"
			 "</body></html>");
	  }
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  PSOCK_END(&s->sout);

}

/**********************************************************************************************************/
/*               EVENTS->EVENT LIST                                                                      **/
/**********************************************************************************************************/
static PT_THREAD(run_eventlist(struct httpd_state *s, char *ptr)){
	static  uint16_t pcount;
	static  uint8_t Apply=0;
	static  uint16_t i=0,cmd=0, st = 0, tmp=0,j=0;
	static  u8 param_cnt;

	u8 port_link_t=0;
	u8 port_poe_t=0;
	u8 stp_t=0;
	u8 spec_link_t=0;
	u8 spec_ping_t=0;
	u8 spec_speed_t=0;
	u8 system_t=0;
	u8 ups_t=0;
	u8 asc_t=0;
	u8 mac_filt_t = 0;

    PSOCK_BEGIN(&s->sout);

	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

		  port_link_t &=~SSTATE;
		  port_poe_t &=~SSTATE;
		  stp_t &=~SSTATE;
		  spec_link_t &=~SSTATE;
		  spec_ping_t &=~SSTATE;
		  spec_speed_t &=~SSTATE;
		  system_t &=~SSTATE;
		  ups_t &=~SSTATE;
		  asc_t &=~SSTATE;
		  mac_filt_t &=~SSTATE;


		  		  for(i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));


					  if(!strcmp(pname, "Apply")){
						  if (!strncmp(pval, "A", 1)) 	Apply = 1;
					  }

					  for(j=0;j<10;j++){
				    	  //state
				    	  sprintf(temp,"c%d",j);
				    	  if(!strcmp(pname, temp)){
				    		  if (!strncmp(pval, "1", 1)){
					    		  switch(j){
									  case 0:
										  port_link_t |=SSTATE;
										  break;
									  case 1:
										  port_poe_t |=SSTATE;
										  break;
									  case 2:
										  stp_t |=SSTATE;
										  break;
									  case 3:
										  spec_link_t |=SSTATE;
										  break;
									  case 4:
										  spec_ping_t |=SSTATE;
										  break;
									  case 5:
										  spec_speed_t |=SSTATE;
										  break;
									  case 6:
										  system_t |=SSTATE;
										  break;
									  case 7:
										  ups_t |=SSTATE;
										  break;
									  case 8:
										  asc_t |=SSTATE;
										  break;
									  case 9:
										  mac_filt_t |=SSTATE;
										  break;
					    		  }
				    		  }
				    	  }
				    	  //select
				    	  sprintf(temp,"s%d",j);
				    	  if(!strcmp(pname, temp)){
				    		  tmp=(uint8_t)strtoul(pval, NULL, 10);
				    		  switch(j){
								  case 0:  port_link_t &=~SMASK; port_link_t|=tmp; break;
								  case 1:  port_poe_t &=~SMASK; port_poe_t|=tmp;break;
								  case 2:  stp_t &=~SMASK; stp_t|=tmp; break;
								  case 3:  spec_link_t &=~SMASK; spec_link_t |=tmp; break;
								  case 4:  spec_ping_t &=~SMASK; spec_ping_t |=tmp;break;
								  case 5:  spec_speed_t &=~SMASK; spec_speed_t |=tmp;break;
								  case 6: system_t &=~SMASK; system_t|=tmp;break;
							      case 7: ups_t &=~SMASK; ups_t|=tmp;break;
							      case 8: asc_t &=~SMASK; asc_t|=tmp;break;
							      case 9: mac_filt_t &=~SMASK; mac_filt_t|=tmp;break;
				    		  }
				    	  }
				      }

				  }

		  	if(Apply==1){
		  		set_event_port_link_t((port_link_t>>3)&1,port_link_t&SMASK);
		  		set_event_port_poe_t((port_poe_t>>3)&1,port_poe_t&SMASK);
		  		set_event_stp_t((stp_t>>3)&1,stp_t&SMASK);
		  		set_event_spec_link_t((spec_link_t>>3)&1,spec_link_t&SMASK);
		  		set_event_spec_ping_t((spec_ping_t>>3)&1,spec_ping_t&SMASK);
		  		set_event_spec_speed_t((spec_speed_t>>3)&1,spec_speed_t&SMASK);
		  		set_event_system_t((system_t>>3)&1,system_t&SMASK);
		  		set_event_ups_t((ups_t>>3)&1,ups_t&SMASK);
		  		set_event_alarm_t((asc_t>>3)&1,asc_t&SMASK);
		  		set_event_mac_t((mac_filt_t>>3)&1,mac_filt_t&SMASK);

		  		Apply=0;
		  		settings_save();
		  		alertl("Parametrs accepted","Настройки применились");
		  	}

	  }


	  addhead();
	  addstr("<body>");
	  addstrl("<legend>Event List</legend>","<legend>Список событий</legend>");
	  if(get_current_user_rule()!=ADMIN_RULE){
		  addstr("Access denied");
	  }else{
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/mngt/eventlist.shtml\">");

		  addstr("<br>"
				//"<b>settings changes</b>"
				"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\"><td>");
				addstrl("Parametrs","Критерии");
				addstr("</td><td>");
				addstrl("State","Состояние");
				addstr("</td><td>");
				addstrl("Level","Уровень важности");
				addstr("</td></tr>");

				param_cnt=10;

				st=0;
				cmd=0;
				for(i=0;i<param_cnt;i++){
					addstr("<tr class=\"g2\"><td>");
					switch(i){
						case 0:addstrl("Port.link","Линк");
							cmd= get_event_port_link_t_level();
							st = get_event_port_link_t_st();
							break;
						case 1:addstrl("Port.PoE","PoE");
							cmd= get_event_port_poe_t_level();
							st = get_event_port_poe_t_st();
							break;
						case 2:addstr("STP/RSTP");
							cmd= get_event_port_stp_t_level();
							st = get_event_port_stp_t_st();
							break;
						case 3:addstrl("Autorestart.Link","Авторестарт.Линк");
							cmd= get_event_port_spec_link_t_level();
							st = get_event_port_spec_link_t_st();
							break;
						case 4:addstrl("Autorestart.Ping","Авторестарт.Ping");
							cmd= get_event_port_spec_ping_t_level();
							st = get_event_port_spec_ping_t_st();
							break;
						case 5:addstrl("Autorestart.Speed","Авторестарт.Скорость");
							cmd= get_event_port_spec_speed_t_level();
							st = get_event_port_spec_speed_t_st();
							break;
						case 6:addstrl("System","Системные");
							cmd= get_event_port_system_t_level();
							st = get_event_port_system_t_st();
							break;
						case 7:addstr("UPS");
							cmd= get_event_port_ups_t_level();
							st = get_event_port_ups_t_st();
							break;
						case 8:addstrl("Inputs/Outputs", "Входы/Выходы");
							cmd= get_event_port_alarm_t_level();
							st = get_event_port_alarm_t_st();
							break;
						case 9:addstrl("MAC Filtering", "Фильтрация по МАС");
							cmd= get_event_port_mac_t_level();
							st = get_event_port_mac_t_st();
							break;
					}
					addstr("</td><td>"
					"<input type=checkbox "
					"name=\"");
					sprintf(temp,"c%d",i);
					addstr(temp);
					addstr("\" value=\"1\"");


					if((i==1) && (POE_PORT_NUM==0)){
						addstr(" disabled ");
					}

					if((i==7)&&(is_ups_mode()==0))
						addstr(" disabled ");
					else if((i==8)&&(get_board_version() !=4))
						addstr(" disabled ");
					else if (st == 1) addstr( " CHECKED ");

					addstr("></td><td><select name=\"");
					sprintf(temp,"s%d",i);
					addstr(temp);
					addstr("\" ");

					if((i==7)&&(is_ups_mode()==0))
						addstr(" disabled ");
					if((i==8)&&(get_board_version() !=4))
						addstr(" disabled ");

					addstr("size=\"1\">"
					  "<option");
						if((cmd & SMASK)==0)addstr(" selected");
							addstr(" value=\"0\">(0) Emergency</option>"
							"<option ");
						if((cmd & SMASK)==1)addstr(" selected");
							addstr(" value=\"1\">(1) Alert</option>"
							"<option ");
						if((cmd & SMASK)==2)addstr(" selected");
							addstr(" value=\"2\">(2) Critical</option>"
							"<option ");
						if((cmd & SMASK)==3)addstr(" selected");
							addstr(" value=\"3\">(3) Error</option>"
							"<option ");
						if((cmd & SMASK)==4)addstr(" selected");
							addstr(" value=\"4\">(4) Warning</option>"
							"<option ");
						if((cmd & SMASK)==5)addstr(" selected");
							addstr(" value=\"5\">(5) Notice</option>"
							"<option ");
						if((cmd & SMASK)==6)addstr(" selected");
							addstr(" value=\"6\">(6) Informational</option>"
							"<option ");
						if((cmd & SMASK)==7)addstr(" selected");
							addstr(" value=\"7\">(7) Debug</option>"
						"</select></td></tr>");

						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
				}
				addstr(
				"</table><br>"
				"<input type=\"Submit\" name=\"Apply\" value=\"Apply\" width=\"200\">"
				"</form></body></html>");
	  	  }

		 PSOCK_SEND_STR(&s->sout,str);
		 str[0]=0;
		 PSOCK_END(&s->sout);
}



/**************************************************************************************/
/*        TEST Page / страница для стенда, парсится в программе		  */
/**************************************************************************************/

static PT_THREAD(run_test(struct httpd_state *s, char *ptr)){
	u8 tmp[6];
	static u8 i;
	u16 voltage_t,current_t;
	struct sfp_state_t sfp_state;
	extern const uint32_t image_version[1];

	 PSOCK_BEGIN(&s->sout);
	 str[0]=0;

	 	 	 	//не менять стоку ниже
				addstr("<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
						  "action=\"/test.shtml\">"
				//as XML struct
				"<br>"
				//не менять стоку выше
				"<!DOCTYPE settings>"
				"<settings>"
				"<selftest>");

				sprintf(temp,"<dev_type>%d</dev_type>",get_dev_type());
				addstr(temp);

				addstr("<cpu_id>");
				get_cpu_id_to_str(temp);
				addstr(temp);
				addstr("</cpu_id>");

				sprintf(temp,"<hw_vers>%d</hw_vers>",get_board_id());
				addstr(temp);


				sprintf(temp,"<init_ok>%d</init_ok>",init_ok);
				addstr(temp);

				sprintf(temp,"<firmvare_vers>%lx</firmvare_vers>",image_version[0]);
				addstr(temp);


				sprintf(temp,"<boot_vers>%x</boot_vers>",(bootloader_version));
				addstr(temp);


	    		get_net_def_mac(tmp);
			    sprintf(temp,"<default_mac>%02x:%02x:%02x:%02x:%02x:%02x</default_mac>",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
			    addstr(temp);


	    		for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
	    			sprintf(temp,"<link_%d>%d</link_%d>",i+1,get_port_link(i),i+1);
	    			addstr(temp);
	    		}


	    		for(i=0;i<(POE_PORT_NUM);i++){
	    			sprintf(temp,"<poe_a_%d_state>%d</poe_a_%d_state>",i+1,get_port_poe_a(i),i+1);
	    			addstr(temp);
	    		}


	    		//get poe voltage
	    		for(u8 i=0;i<POE_PORT_NUM;i++){
		    		voltage_t=get_poe_voltage(POE_A,i);
		    		sprintf(temp,"<poe_a_%d_v>%d.%d</poe_a_%d_v>",i+1,(int)(voltage_t)/1000,(int)(voltage_t % 1000),i+1);
		    		addstr(temp);
	    		}

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

	    		//get poe current
	    		for(u8 i=0;i<POE_PORT_NUM;i++){
		    		current_t=get_poe_current(POE_A,0);
		    		sprintf(temp,"<poe_a_%d_c>%d</poe_a_%d_c>",i+1,(int)(current_t),i+1);
		    		addstr(temp);
	    		}

	    		for(i=0;i<(POE_PORT_NUM);i++){
	    			sprintf(temp,"<poe_b_%d_state>%d</poe_b_%d_state>",i+1,get_port_poe_b(i),i+1);
	    			addstr(temp);
	    		}

	    		//get poe voltage
	    		for(u8 i=0;i<POE_PORT_NUM;i++){
		    		voltage_t=get_poe_voltage(POE_B,i);
		    		sprintf(temp,"<poe_b_%d_v>%d.%d</poe_b_%d_v>",i+1,(int)(voltage_t)/1000,(int)(voltage_t % 1000),i+1);
		    		addstr(temp);
	    		}


	    		//get poe current
	    		for(u8 i=0;i<POE_PORT_NUM;i++){
		    		current_t=get_poe_current(POE_B,0);
		    		sprintf(temp,"<poe_b_%d_c>%d</poe_b_%d_c>",i+1,(int)(current_t),i+1);
		    		addstr(temp);
	    		}

	    		//board version
	       		sprintf(temp,"<board_version>%d</board_version>",get_board_version());
	       		addstr(temp);

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

	    		//poe version
				if(POE_PORT_NUM){
					sprintf(temp,"<poe_controller>%d</poe_controller>",get_poe_id());
					addstr(temp);
				}

	    		//alarm state
	    		if(get_sensor_state(0)==Bit_RESET)
	    			addstr("<sensor_0>0</sensor_0>");
				else
					addstr("<sensor_0>1</sensor_0>");

				if(get_sensor_state(1)==Bit_RESET)
					addstr("<sensor_1>0</sensor_1>");
				else
					addstr("<sensor_1>1</sensor_1>");

	    		if(get_sensor_state(2)==Bit_RESET)
	    			addstr("<sensor_2>0</sensor_2>");
				else
					addstr("<sensor_2>1</sensor_2>");




	    		if(get_dev_type()==DEV_SWU16){
	    			for(u8 i=0;i<FIBER_PORT_NUM;i++){
	    				sprintf(temp,"<sfp_%d_pres>%d</sfp_%d_pres>",i,get_sfp_present(i),i);
	    				addstr(temp);
	    				if(get_sfp_present(i)){
							sfp_get_info(i,&sfp_state);
							sprintf(temp,"<sfp_%d_id>%d</sfp_%d_id>",i,sfp_state.identifier,i);
							addstr(temp);
						}
	    			}

	    		}
	    		else{
					//sfp present
					if(get_sfp_present(GE1)==0)
						addstr("<sfp_1_pres>0</sfp_1_pres>");
					else
						addstr("<sfp_1_pres>1</sfp_1_pres>");

					if(get_sfp_present(GE2)==0)
						addstr("<sfp_2_pres>0</sfp_2_pres>");
					else
						addstr("<sfp_2_pres>1</sfp_2_pres>");
					//SFP sd
					sprintf(temp,"<sfp_1_sd>%d</sfp_1_sd>",GPIO_ReadInputDataBit(GPIOD,SW_SD1));
					addstr(temp);

					sprintf(temp,"<sfp_2_sd>%d</sfp_2_sd>",GPIO_ReadInputDataBit(GPIOD,SW_SD2));
					addstr(temp);

					//i2c связь с Sfp модулями
					if(GPIO_ReadInputDataBit(GPIOD,SW_SD1)){
						//SetI2CMode(SFP1);
						sfp_get_info(GE1,&sfp_state);

						sprintf(temp,"<sfp_1_id>%d</sfp_1_id>",sfp_state.identifier);
						addstr(temp);
					}
					if(GPIO_ReadInputDataBit(GPIOD,SW_SD2)){
						//SetI2CMode(SFP2);
						sfp_get_info(GE2,&sfp_state);

						sprintf(temp,"<sfp_2_id>%d</sfp_2_id>",sfp_state.identifier);
						addstr(temp);
					}

	    		}



				//marvell id
				sprintf(temp,"<marvell_id>%d</marvell_id>",get_marvell_id());
				addstr(temp);


				//напряжение в контрольных точках
				//PC0 - 1.2 // ADC12_IN10
				//PA6 - 1.5	// ADC12_IN6
				//PA5 - 2.5 // ADC12_IN5
				if(get_dev_type()==DEV_PSW2GPLUS || get_dev_type()==DEV_PSW2G2FPLUSUPS || get_dev_type()==DEV_PSW2G2FPLUS){
					sprintf(temp,"<adc_1_2>%d</adc_1_2>",readADC1(10));
					addstr(temp);

					sprintf(temp,"<adc_1_5>%d</adc_1_5>",readADC1(6));
					addstr(temp);

					sprintf(temp,"<adc_2_5>%d</adc_2_5>",readADC1(5));
					addstr(temp);
				}



				if((get_dev_type()==DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type()==DEV_PSW2G6F)||(get_dev_type()==DEV_PSW2G8F)){
					sprintf(temp,"<adc_1_2>%d</adc_1_2>",readADC1(6));
					addstr(temp);

					sprintf(temp,"<adc_1_5>%d</adc_1_5>",readADC1(5));
					addstr(temp);

					sprintf(temp,"<adc_2_5>%d</adc_2_5>",readADC1(10));
					addstr(temp);
				}
				if((get_dev_type()==DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
					sprintf(temp,"<adc_1_0>%d</adc_1_0>",readADC1(5));
					addstr(temp);

					sprintf(temp,"<adc_1_8>%d</adc_1_8>",readADC1(10));
					addstr(temp);
				}

				//UPS state
				sprintf(temp,"<ups_det>%d</ups_det>",is_ups_mode());
				strcat(str,temp);
				if(is_ups_mode() == 1){
					//akb detect
					sprintf(temp,"<akb_det>%d</akb_det>",UPS_AKB_detect);
					addstr(temp);

					//AC detect
					sprintf(temp,"<ups_rez>%d</ups_rez>",UPS_rezerv);
					addstr(temp);

					//akb voltage
					SetI2CMode(IRP);
					voltage_t=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_READ);
					sprintf(temp,"<akb_voltage>%d.%d</akb_voltage>",(voltage_t+350)/10,(voltage_t+350)%10);
					addstr(temp);

					//charge voltage
					voltage_t=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_CHARGE_READ);
					sprintf(temp,"<akb_voltage_chg>%d.%d</akb_voltage_chg>",(voltage_t+350)/10,(voltage_t+350)%10);
					addstr(temp);
				}
				strcat(str,
				"</selftest>"
				"</settings>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				addstr("<br>ERROR CODES:");
				for(u8 i=0;i<ALARM_REC_NUM;i++){
					if(printf_alarm(i,temp)!=0){
						addstr(temp);
						addstr("<br>");
					}
				}

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

		  PSOCK_END(&s->sout);
}


/**************************************************************************************/
/*        TEST Page / страница для тестирования 									  */
/**************************************************************************************/

static PT_THREAD(run_test_web(struct httpd_state *s, char *ptr)){
	u8 tmp[6];
	static u8 i,apply,pcount;
	u16 voltage_t,current_t;
	struct sfp_state_t sfp_state;
	extern const uint32_t image_version[1];


	PSOCK_BEGIN(&s->sout);
	str[0]=0;
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		int i=0;
		for(i=0;i<pcount;i++){
			  char *pname,*pval;
			  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			  pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			  if(!strcmp(pname, "Apply")){
				  apply = 1;
			  }

			  for(u8 j=0;j<ALL_PORT_NUM;j++){
				  sprintf(temp,"lben%d",i);
				  if(!strcmp(pname, temp)){
					  dev.port_stat[i].port_loopback = ENABLE;
					  set_swu_port_loopback(i,ENABLE);
				  }

				  sprintf(temp,"lbdis%d",i);
				  if(!strcmp(pname, temp)){
					  dev.port_stat[i].port_loopback = DISABLE;
					  set_swu_port_loopback(i,DISABLE);
				  }

			  }
		}
		if(apply){
			set_swu_test_vlan(ENABLE);
			alert("Шлейф ВКЛЮЧЕН");
		}
    }




	 addhead();
	 addstr("<body>");

	 addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/selftest.shtml\">"
	 "<br>"
	 "<caption><b>Тест</b></caption><br><br>");

	 addstr("<br><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
	 "<tr class=\"g2\"><td>Тип устройства</td><td>");
	 get_dev_name(temp);
	 addstr(temp);
	 addstr("</td></tr>");

	 addstr("<tr class=\"g2\"><td>ID процессора</td><td>");
	 get_cpu_id_to_str(temp);
	 addstr(temp);
	 addstr("</td></tr>");

	 addstr("<tr class=\"g2\"><td>Аппаратная версия</td><td>");
	 sprintf(temp,"%d",get_board_id());
	 addstr(temp);
	 addstr("</td></tr>");

	 addstr("<tr class=\"g2\"><td>Версия встроенного ПО</td><td>");
	 sprintf(temp,"%02X.%02X.%02X",
		 (int)(image_version[0]>>16)&0xff,
		 (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
	 addstr(temp);
	 addstr("</td></tr>");

	 addstr("<tr class=\"g2\"><td>Версия загрузчика</td><td>");
	 sprintf(temp,"%02X.%02X",
		 (int)(bootloader_version>>8)&0xff,  (int)(bootloader_version)&0xff);
	 addstr(temp);
	 addstr("</td></tr>");

 	 addstr("<tr class=\"g2\"><td>MAC адрес</td><td>");
	 get_net_def_mac(tmp);
	 sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
			tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
	 addstr(temp);
	 addstr("</td></tr>");

 	 addstr("<tr class=\"g2\"><td>Switch-контроллер ID</td><td>");
	 sprintf(temp,"0x%X", get_marvell_id());
	 addstr(temp);
	 addstr("</td></tr>");
	 if((get_dev_type()!=DEV_SWU16)){
		 addstr("<tr class=\"g2\"><td>PoE-контроллер ID</td><td>");
		 sprintf(temp,"0x%X", get_poe_id());
		 addstr(temp);
		 addstr("</td></tr>");
	 }
	 PSOCK_SEND_STR(&s->sout,str);
	 str[0]=0;

	 //напряжение в контрольных точках
	//PC0 - 1.2 // ADC12_IN10
	//PA6 - 1.5	// ADC12_IN6
	//PA5 - 2.5 // ADC12_IN5
	if(get_dev_type()==DEV_PSW2GPLUS || get_dev_type()==DEV_PSW2G2FPLUSUPS || get_dev_type()==DEV_PSW2G2FPLUS){
		addstr("<tr class=\"g2\"><td>Напряжение 1.2В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(10),1241));
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение 1.5В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(6),1241));
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение 2.5В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(5),1241));
		addstr(temp);
		addstr("</td></tr>");
	}

	if((get_dev_type()==DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type()==DEV_PSW2G6F)||(get_dev_type()==DEV_PSW2G8F)){
		addstr("<tr class=\"g2\"><td>Напряжение 1.2В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(6),1241));
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение 1.5В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(5),1241));
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение 2.5В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(10),1241));
		addstr(temp);
		addstr("</td></tr>");
	}

	if((get_dev_type()==DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS) || (get_dev_type()==DEV_SWU16)){
		addstr("<tr class=\"g2\"><td>Напряжение 1.0В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(5),1241));
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение 1.8В</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(10),1241));
		addstr(temp);
		addstr("</td></tr>");
	}

	if(get_dev_type()==DEV_SWU16){
		addstr("<tr class=\"g2\"><td>Напряжение 3.3В (SFP)</td><td>");
		sprintf(temp,"%d.%d", PSEUDO_FLOAT(readADC1(6)*2,1241));
		addstr(temp);
		addstr("</td></tr>");
	}

	addstr("<tr class=\"g2\"><td>Модуль IRP(ИБП)</td><td>");
	if(is_ups_mode())
		addstr("Подключен");
	else
		addstr("Не подключен");
	addstr("</td></tr>");

	if(is_ups_mode()){
		addstr("<tr class=\"g2\"><td>АКБ</td><td>");
		if(UPS_AKB_detect)
			addstr("Подключена");
		else
			addstr("Не подключена");
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Источник питания</td><td>");
		if(UPS_rezerv)
			addstr("АКБ");
		else
			addstr("Сеть");
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение на АКБ</td><td>");
		if(UPS_AKB_detect){
			voltage_t=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_READ);
			sprintf(temp,"%d.%d",(voltage_t+350)/10,(voltage_t+350)%10);
			addstr(temp);
		}
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>Напряжение зарядки</td><td>");
		voltage_t=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_CHARGE_READ);
		sprintf(temp,"%d.%d",(voltage_t+350)/10,(voltage_t+350)%10);
		addstr(temp);
		addstr("</td></tr>");
	}//is ups mode


	addstr("<tr class=\"g2\"><td>Самотестирование</td>");
	if(init_ok && start_selftest()==0)
		addstr("<td class=\"goodtest\"><b>Успешно");
	else
		addstr("<td class=\"badtest\"><b>Ошибка");

	addstr("</b></td></tr>");
	addstr("</table>");


	if(init_ok==0){
	addstr("<br>Коды ошибок:<br>");
		for(u8 i=0;i<ALARM_REC_NUM;i++){
			if(alarm_list[i].alarm_code == ERROR_WRITE_BB)
				addstr("Ошибка теста FLASH<br>");
			if(alarm_list[i].alarm_code == ERROR_POE_INIT)
				addstr("Ошибка интерфейса с PoE контролером<br>");
			if(alarm_list[i].alarm_code == ERROR_I2C)
				addstr("Ошибка интерфейса I2C<br>");
			if(alarm_list[i].alarm_code == ERROR_MARVEL_START)
				addstr("Ошибка интерфейса с Switch контролером<br>");
		}
	}

	if(get_dev_type() == DEV_SWU16){
		if(apply == 0){
			addstr("<br><br><input type=\"Submit\" name=\"Apply\" value=\"Включить тестовый ШЛЕЙФ\"></form>");
		}
		else
		{
			addstr("<br><br><b color=\"red\">ШЛЕЙФ включен, выключение через перезагрузку<b></form>");
		}
	}


	addstr("<br><br>"
	"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
	"<tr class=\"g2\">"
	"<td>Порт</td><td>Линк</td><td>SFP</td><td>PoE</td><td>Tx/Rx</td>");
	if(get_dev_type()==DEV_SWU16){
		addstr("<td>Loopback</td>");
	}
	addstr("</tr>");
	//get_link_state();
	for(i=0;i<(ALL_PORT_NUM);i++){
		addstr("<tr class=\"g2\"><td>");
		sprintf(temp,"%d",i+1);
		addstr(temp);
		addstr("</td><td>");
		if(get_port_link(i)){
			switch(PortSpeedInfo(L2F_port_conv(i))){
				case 1:addstr("Up(10)");break;
				case 2:addstr("Up(100)");break;
				case 3:addstr("Up(1000)");break;
				default:addstr("Error");break;
			}
		}
		else
			addstr("Down");
	 	addstr("</td><td>");
	 	if(is_fiber(i)){
	 		if(get_sfp_present(i))
				addstr("PR ");
	 		if(get_sfp_los(i)==0)//sd = ~los
	 			addstr("SD ");
			sfp_get_info(i,&sfp_state);
			if(sfp_state.state)
				addstr("I2C ");
		}
	 	addstr("</td><td>");
	 	if(is_cooper(i) && i<POE_PORT_NUM){
	 		if(get_port_poe_a(i)){
				voltage_t=get_poe_voltage(POE_A,i);
				current_t=get_poe_current(POE_A,i);
				sprintf(temp,"PoE A(%d.%dV %dmA) ",
						(int)(voltage_t)/1000,(int)(voltage_t % 1000),(int)(current_t));
				addstr(temp);
	 		}
	 		if(get_port_poe_b(i)){
	 			voltage_t=get_poe_voltage(POE_B,i);
	 			current_t=get_poe_current(POE_B,i);
	 			sprintf(temp,"PoE B(%d.%dV %dmA)",
	 				(int)(voltage_t)/1000,(int)(voltage_t % 1000),(int)(current_t));
	 			addstr(temp);
	 		}
	 	}
	 	addstr("</td><td>");
	 	sprintf(temp,"%lu/%lu",dev.port_stat[i].rx_good,dev.port_stat[i].tx_good);
	 	addstr(temp);
	 	addstr("</td>");
	 	if(get_dev_type()==DEV_SWU16){
	 		addstr("<td>");
	 		if(dev.port_stat[i].port_loopback == ENABLE)
	 			addstr("Enable");
	 		else
	 			addstr("Disable");
	 		addstr("<input type=\"Submit\" name=\"");
	 		sprintf(temp,"lben%d",i);
	 		addstr(temp);
	 		addstr(	"\" value=\"Enable\">&nbsp;<input type=\"Submit\" name=\"");
	 		sprintf(temp,"lbdis%d",i);
	 		addstr(temp);
	 		addstr("\" value=\"Disable\"></td>");
	 	}
	 	addstr("</tr>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
	}

	addstr("</table>"
	"PR - сигнал PRESENT<br>"
	"SD - SignalDetect<br>"
	"I2C - шина I2C к SFP модулю<br>"
	"<br><br>");



	addstr("</body></html>");
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;


	//alarm state
	/*if(get_sensor_state(0)==Bit_RESET)
		addstr("<sensor_0>0</sensor_0>");
	else
		addstr("<sensor_0>1</sensor_0>");

	if(get_sensor_state(1)==Bit_RESET)
		addstr("<sensor_1>0</sensor_1>");
	else
		addstr("<sensor_1>1</sensor_1>");

	if(get_sensor_state(2)==Bit_RESET)
		addstr("<sensor_2>0</sensor_2>");
	else
		addstr("<sensor_2>1</sensor_2>");
	 */




	PSOCK_END(&s->sout);
}


/**************************************************************************************/
/*                            BASE SETTINGS -> Interface settings*/
/**************************************************************************************/


static PT_THREAD(run_language(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	static  uint16_t pcount;
	static  u8 apply=0,lang;

		  PSOCK_BEGIN(&s->sout);
		  str[0]=0;
		  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

			  int i=0;
			  for(i=0;i<pcount;i++){
		  		char *pname,*pval;
		  		pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		  		pval = http_get_parameter_value(s->param,i,sizeof(s->param));

		  		if(!strcmp(pname, "Apply")){
		  			if (!strncmp(pval, "A", 1)) apply = 1;
		  		}

		  		if(!strcmp(pname, "lang")){
		  			if (!strncmp(pval, "r", 1)) lang = RUS;
		  			if (!strncmp(pval, "e", 1)) lang = ENG;
		  		}

			  }

			  if(apply){
				  if(lang != get_interface_lang()){
					  set_interface_lang(lang);
					  settings_save();
					  apply = 0;
					  alertl("Parameters accepted, refresh page","Настройки применились, обновите страницу");
					  addstr("<script language=\"javascript\">location.reload();</script>");
				  }
				  else{
					  set_interface_lang(lang);
					  settings_save();
					  apply = 0;
					  alertl("Parameters accepted","Настройки применились");
				  }
			  }
		}

		lang =  get_interface_lang();

		addhead();
		addstr("<body>");
	    PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstrl("<legend>Language Settings</legend>","<legend>Язык интерфейса</legend>");

		addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
				  "action=\"/mngt/managment.shtml\"><br>");

		addstr("<br><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\">"
		"<td>");
		addstrl("Language","Язык");
		addstr("</td>"
		"<td><select name=\"lang\" size=\"1\">"
		"<option");
		if(lang == ENG) addstr(" selected");
		addstr(" value=\"e\">English</option>"
		"<option ");
		if(lang == RUS) addstr(" selected");
		addstr(" value=\"r\">Русский</option>"
		"</select></td></tr></table><br>"
		"<input type=\"Submit\" name=\"Apply\" value=\"Apply\"></form></body></html>");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		PSOCK_END(&s->sout);
}


/**************************************************************************************/
/*                            IGMP SETTINGS 										  */
/**************************************************************************************/

static PT_THREAD(run_igmp(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	 uint16_t pcount;
	static  u8 apply=0;
	static u8 state,i,k;
	static u8 send_query;
	static u8 port_st[PORT_NUM];
	static u8 advanced=0;
	u8 query_int=0;
	u8 query_resp_int=0;
	u8 group_memb_time=0;
	u8 querier_pres_int=0;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);

			  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
				  int j=0;

				  if(advanced==1)
					  for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
						  port_st[i] = 0;

				  for(j=0;j<pcount;j++){
					char *pname,*pval;
					pname = http_get_parameter_name(s->param,j,sizeof(s->param));
					pval = http_get_parameter_value(s->param,j,sizeof(s->param));

					if(!strcmp(pname, "Apply")){
						if (!strncmp(pval, "A", 1)) apply = 1;
					}

					if(!strcmp(pname, "ad")){
						if (!strncmp(pval, "A", 1)){
							if(advanced==0)
								advanced=1;
							else
								advanced=0;
						}
					}

					if(!strcmp(pname, "state")){
						if (!strncmp(pval, "0", 1)) state = 0;
						if (!strncmp(pval, "1", 1)) state = 1;
					}

					if(!strcmp(pname, "qi")){
						query_int=(uint8_t)strtoul(pval, NULL, 10);
					}

					if(!strcmp(pname, "sq")){
						if (!strncmp(pval, "0", 1)) send_query = 0;
						if (!strncmp(pval, "1", 1)) send_query = 1;
					}

					if(!strcmp(pname, "qri")){
						query_resp_int=(uint8_t)strtoul(pval, NULL, 10);
					}

					if(!strcmp(pname, "gmt")){
						group_memb_time=(uint8_t)strtoul(pval, NULL, 10);
					}
					if(!strcmp(pname, "gpi")){
						querier_pres_int=(uint8_t)strtoul(pval, NULL, 10);
					}



					for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
						sprintf(temp,"pstate%d",i);
						if(!strcmp(pname, temp)){
							if (!strncmp(pval, "1", 1)) port_st[i] = 1;
						}
					}
				  }

				  if(apply){
						set_igmp_snooping_state(state);
						if(advanced == 1){
							for(u8 port=0;port<(COOPER_PORT_NUM+FIBER_PORT_NUM);port++)
								set_igmp_port_state(port,port_st[port]);
							set_igmp_query_int(query_int);
							set_igmp_query_mode(send_query);
							set_igmp_max_resp_time(query_resp_int);
							set_igmp_group_membership_time(group_memb_time);
							set_igmp_other_querier_time(querier_pres_int);
						}
						apply=0;
						settings_add2queue(SQ_IGMP);
						settings_save();
						alertl("Parameters accepted","Настройки применились");
				  }
		  }
			state = get_igmp_snooping_state();
			send_query = get_igmp_query_mode();

			addhead();
			addstr("<body>");
			addstr("<legend>IGMP Snooping</legend>");
			if(get_current_user_rule()!=ADMIN_RULE){
				  addstr("Access denied");
			}else{

				addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
						  "action=\"/igmp/igmp.shtml\"><br>");

				addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\">"
				"<td>");
				addstrl("IGMP Snooping state","Состояние IGMP Snooping");
				addstr("</td>"
				"<td><select name=\"state\" size=\"1\">"
				"<option");
				if(state == 0) addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disabled","Выключено");
				addstr("</option>"
				"<option ");
				if(state == 1) addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enabled","Включено");
				addstr("</option>"
				"</select></td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				if(advanced == 1){
					//send query
					addstr("<tr class=\"g2\">"
					"<td>");
					addstrl("Send IGMP Query","Посылать IGMP Query");
					addstr("</td>"
					"<td><select name=\"sq\" size=\"1\">"
					"<option");
					if(send_query == 0) addstr(" selected");
					addstr(" value=\"0\">");
					addstrl("Disabled","Выключено");
					addstr("</option>"
					"<option ");
					if(send_query == 1) addstr(" selected");
					addstr(" value=\"1\">");
					addstrl("Enabled","Включено");
					addstr("</option>"
					"</select></td></tr>");
					//Query Interval
					addstr("<tr class=\"g2\"><td>");
					addstr("Query Interval");
					addstr("</td><td><input type=\"text\"name=\"qi\"size=\"3\" maxlength=\"3\"value=\"");
					sprintf(temp,"%d",get_igmp_query_int());
					addstr(temp);
					addstr("\"></td></tr>"
					"<tr class=\"g2\"><td>");
					addstr("Query Response Interval");
					addstr("</td><td><input type=\"text\"name=\"qri\"size=\"3\" maxlength=\"3\"value=\"");
					sprintf(temp,"%d",get_igmp_max_resp_time());
					addstr(temp);
					addstr("\"></td></tr>"
					"<tr class=\"g2\"><td>");
					addstr("Group Membership Time");
					addstr("</td><td><input type=\"text\"name=\"gmt\" size=\"3\" maxlength=\"3\"value=\"");
					sprintf(temp,"%d",get_igmp_group_membership_time());
					addstr(temp);
					addstr("\"></td></tr>"
					"<tr class=\"g2\"><td>");
					addstr("Other Querier Present Interval");
					addstr("</td><td><input type=\"text\"name=\"gpi\" size=\"3\" maxlength=\"3\"value=\"");
					sprintf(temp,"%d",get_igmp_other_querier_time());
					addstr(temp);
					addstr("\"></td></tr>"
					"</table><br><br>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
					//igmp table
					addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
					"<tr class=\"g2\"><td>");
					addstrl("Port","Порт");
					addstr("</td><td>");
					addstrl("Port State","Состояние порта");
					addstr("</td></tr>");

					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;

					for(k=0;k<(COOPER_PORT_NUM+FIBER_PORT_NUM);k++){
						//port
						addstr( "<tr class=\"g2\"><td>");
						if(get_dev_type() == DEV_PSW2GPLUS){
							switch(k){
								case 0:addstr("FE#1");break;
								case 1:addstr("FE#2");break;
								case 2:addstr("FE#3");break;
								case 3:addstr("FE#4");break;
								case 4:addstr("GE#1");break;
								case 5:addstr("GE#2");break;
							}
						}else{
							sprintf(temp,"%d",k+1);
							addstr(temp);
						}
						addstr("</td>");
						//state
						addstr("<td><input type=checkbox name=\"");
						sprintf(temp,"pstate%d",k);
						addstr(temp);
						addstr("\" value=\"1\"");
						if(get_igmp_port_state(k) == 1)
							addstr(" checked ");
						addstr("></td></tr>");

						PSOCK_SEND_STR(&s->sout,str);
						str[0]=0;
					}
			  }
			  addstr("</table>");
			  addstr("<br><br><table><tr><td>"
			  "<input type=\"Submit\" name=\"Apply\" value=\"Apply\"></td>"
			  "<td><input type=\"Submit\" name=\"ad\" value=\"");
			  if(advanced==1)
				  addstr("Advanced Settings Hide");
			  else
				  addstr("Advanced Settings Show");
			  addstr("\"></td></tr></table></form></body></html>");
		  }
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  PSOCK_END(&s->sout);
}


/**************************************************************************************/
/*                            IGMP GROUPS LIST			      						  */
/**************************************************************************************/

static PT_THREAD(run_igmp_groups(struct httpd_state *s, char *ptr)){
	static u8 i;
	uip_ipaddr_t groupaddr;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
			addhead();
			addstr("<body>");
			addstrl("<legend>IGMP Groups List</legend>","<legend>Список групп IGMP</legend>");

			addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
					  "action=\"/igmp/groups.shtml\"><br><br>");
			addstrl("Querier state: ","Состояние: ");
			if(get_igmp_querier())
				addstr("Querier");
			else
				addstr("Non querier");

			addstr("<br><br><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td>№</td><td>");
			addstrl("Group address", "Адрес группы");
			addstr("</td><td>");
			addstrl("Ports","Порты");
			addstr("</td></tr>");

			if(get_igmp_groupsnum() == 0){
				addstr("<tr class=\"g2\"><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td></tr>");
			}

			for(i = 0;i<get_igmp_groupsnum();i++){
				addstr("<tr class=\"g2\"><td>");
				sprintf(temp,"%d",i+1);
				addstr(temp);
				addstr("</td><td>");
				get_igmp_group(i,groupaddr);
				sprintf(temp,"%d.%d.%d.%d",
						uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),uip_ipaddr4(groupaddr));
				addstr(temp);
				addstr("</td><td>");
				for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
					if(get_igmp_group_port(i,j)==1){
						if(get_dev_type() == DEV_PSW2GPLUS){
							switch(j){
								case 0:addstr("FE#1 ");break;
								case 1:addstr("FE#2 ");break;
								case 2:addstr("FE#3 ");break;
								case 3:addstr("FE#4 ");break;
								case 4:addstr("GE#1 ");break;
								case 5:addstr("GE#2 ");break;
							}
						}else{
							sprintf(temp,"%d ",j+1);
							addstr(temp);
						}
					}
				}
				addstr("</td></tr>");
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;
			}

		  addstr("</table>"
		  "<br><br><table><tr><td>"
		  "<input type=\"Submit\" name=\"Refresh\" value=\"Refresh\">"
		  "</td></tr></table></form></body></html>");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  PSOCK_END(&s->sout);
}



/**************************************************************************************/
/*                            LLDP SETTINGS 										  */
/**************************************************************************************/

static PT_THREAD(run_lldpsett(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	 uint16_t pcount;
	static  u8 apply=0;
	static u8 state,i,k;
	static u8 port_st[PORT_NUM];
	static u8 lldp_ti,lldp_hm;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);

	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		int j=0;


		for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
		  port_st[i] = 0;

       for(j=0;j<pcount;j++){
    	   char *pname,*pval;
    	   pname = http_get_parameter_name(s->param,j,sizeof(s->param));
    	   pval = http_get_parameter_value(s->param,j,sizeof(s->param));

    	   if(!strcmp(pname, "Apply")){
    		   if (!strncmp(pval, "A", 1)) apply = 1;
    	   }

			if(!strcmp(pname, "state")){
				if (!strncmp(pval, "0", 1)) state = 0;
				if (!strncmp(pval, "1", 1)) state = 1;
			}



			//transmit interval
			if(!strcmp(pname, "tx_interval")){
				lldp_ti=(uint8_t)strtoul(pval, NULL, 10);
			}

			//hold multiplier
			if(!strcmp(pname, "tx_hold")){
				lldp_hm=(uint8_t)strtoul(pval, NULL, 10);
			}





			for(i=0;i<(ALL_PORT_NUM);i++){
				sprintf(temp,"pstate%d",i);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "1", 1)) port_st[i] = 1;
				}
			}
		}

		if(apply){

			if(lldp_ti<MIN_LLDP_TI || lldp_ti>MAX_LLDP_TI){
				alertl("Parameters incorrect!", "Неверные параметры!");
				apply = 0;
			}

			if(lldp_hm<MIN_LLDP_HM || lldp_hm>MAX_LLDP_HM){
				alertl("Parameters incorrect!", "Неверные параметры!");
				apply = 0;
			}

			if(apply){
				set_lldp_state(state);
				set_lldp_transmit_interval(lldp_ti);
				set_lldp_hold_multiplier(lldp_hm);
				for(i=0;i<ALL_PORT_NUM;i++)
					set_lldp_port_state(i,port_st[i]);
				apply=0;
				settings_add2queue(SQ_LLDP);
				settings_save();
				alertl("Parameters accepted","Настройки применились");
			}
		}
	}

	addhead();
	addstr("<body>");
	addstrl("<legend>LLDP Settings</legend>","<legend>Настройки LLDP</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstr("Access denied");
	}else{
		addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
				  "action=\"/lldp/lldpsett.shtml\"><br>");

				addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\">"
				"<td>");
				addstrl("LLDP state","Состояние LLDP");
				addstr("</td>"
				"<td><select name=\"state\" size=\"1\">"
				"<option");
				if(get_lldp_state() == 0) addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disabled","Выключено");
				addstr("</option>"
				"<option ");
				if(get_lldp_state() == 1) addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enabled","Включено");
				addstr("</option>"
				"</select></td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;


				//send query
				addstr("<tr class=\"g2\">"
				"<td>");
				addstrl("Message Transmit Interval","Интервал отправки сообщений");
				addstr("</td>"
				"<td>"
				"<input type=\"text\"name=\"tx_interval\"size=\"2\" maxlength=\"2\"value=\"");
				sprintf(temp,"%d",get_lldp_transmit_interval());
				addstr(temp);
				addstr("\"></td></tr>"


				"<tr class=\"g2\"><td>");
				addstrl("Tx Hold Multiplier","Множитель хранения");
				addstr("</td><td><input type=\"text\"name=\"tx_hold\" size=\"2\" maxlength=\"2\"value=\"");
				sprintf(temp,"%d",get_lldp_hold_multiplier());
				addstr(temp);
				addstr("\"></td></tr></table><br><br>");

				//igmp table
				addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\"><td>");
				addstrl("Port","Порт");
				addstr("</td><td>");
				addstrl("Port State","Состояние порта");
				addstr("</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				for(k=0;k<(ALL_PORT_NUM);k++){
					//port
					addstr( "<tr class=\"g2\"><td>");
					sprintf(temp,"%d",k+1);
					addstr(temp);
					addstr("</td>");
					//state
					addstr("<td><input type=checkbox name=\"");
					sprintf(temp,"pstate%d",k);
					addstr(temp);
					addstr("\" value=\"1\"");
					if(get_lldp_port_state(k) == 1)
						addstr(" checked ");
					addstr("></td></tr>");

					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}

				addstr("</table>");
				addstr("<br><br><input type=\"Submit\" name=\"Apply\" value=\"Apply\">");
		  }
		  addstr("</form></body></html>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  PSOCK_END(&s->sout);
}



/**************************************************************************************/
/*                            LLDP Status			      						  */
/**************************************************************************************/

static PT_THREAD(run_lldpstat(struct httpd_state *s, char *ptr)){
	static u8 i,k;
	uip_ipaddr_t ip;
	char text_tmp[128],tmp[128];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
			addhead();
			addstr("<body>");
			addstr("<script type=\"text/javascript\">"
			"function lldp_info(num)"
			"{"
				"window.open('/info/lldpinfo.shtml?num='+num,'','width=300,height=500,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');"
			"}"
			"</script>");
			addstrl("<legend>LLDP Statistics</legend>","<legend>Статистика LLDP</legend>");

			addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
					  "action=\"/lldp/lldpstat.shtml\"><br><br>");



			addstrl("<b>LLDP System Information</b>","<b>Информация о системе</b>");
			addstr("<br><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td>");
			addstrl("Chassis ID Subtype", "Подтип идентификатора шасси");
			addstr("</td><td>");
		 	addstrl("MAC Address", "MAC адрес");
		 	addstr("</td></tr>"
			"<tr class=\"g2\"><td>");
		    addstrl("Chassis ID", "Идентификатор шасси");
			addstr("</td><td>");
			sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
						(int)dev_addr[0],(int)dev_addr[1],(int)dev_addr[2],
						(int)dev_addr[3],(int)dev_addr[4],(int)dev_addr[5]);
			addstr(temp);
			addstr("</td></tr>"

			"<tr class=\"g2\"><td>");
			addstrl("System Name", "Имя устройства");
			addstr("</td><td>");
			get_interface_name(tmp);
			tmp[127]=0;
			if(strlen(tmp)){
				memset(text_tmp,0,sizeof(text_tmp));
				http_url_decode(tmp,text_tmp,strlen(tmp));
				for(uint8_t i=0;i<strlen(text_tmp);i++){
					if(text_tmp[i]=='+') text_tmp[i] = ' ';
				  	if(text_tmp[i]=='%') text_tmp[i]=' ';
				}
				addstr(text_tmp);
			}
			addstr("</td></tr>"

			"<tr class=\"g2\"><td>");
			addstrl("System Desrtiption", "Модель устройства");
			addstr("</td><td>");
			get_dev_name(text_tmp);
			addstr(text_tmp);
			addstr("</td></tr>"

			"<tr class=\"g2\"><td>");
			addstrl("System Capabilities","Системные возможности");
			addstr("</td><td>");
			addstrl("Repeater, Bridge", "Повторитель, Мост");
			addstr("</td></tr></table>");

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

			addstr("<br><br>");
			addstrl("<b>LLDP Entries</b>","<b>Записи LLDP</b>");
			addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td></td><td>");
			addstrl("Local Port","Локальный порт");
			addstr("</td><td>");
			addstrl("Chassis ID Subtype", "Подтип идентификатора шасси");
			addstr("</td><td>");
			addstrl("Chassis ID", "Идентификатор шасси");
			addstr("</td><td>");
			addstrl("Port ID Subtype", "Подтип идентификатора порта");
			addstr("</td><td>");
			addstrl("Port ID", "Идентификатор порта");
			addstr("</td><td>TTL</td><td></td></tr>");

			for(i=0,k=0;i<LLDP_ENTRY_NUM;i++,k++){
				if(lldp_port_entry[i].valid){
					addstr("<tr class=\"g2\"><td>");
					sprintf(temp,"%d",k+1);
					addstr(temp);
					addstr("</td><td>");
					sprintf(temp,"%d",lldp_port_entry[i].if_index+1);
					addstr(temp);
					addstr("</td><td>");
					convert_lldp_chasis_id_subtype(temp,lldp_port_entry[i].chassis_id_subtype);
					addstr(temp);
					addstr("</td><td>");
					convert_lldp_chasis_id(temp,
							lldp_port_entry[i].chassis_id_subtype,
							lldp_port_entry[i].chassis_id);
					addstr(temp);
					addstr("</td><td>");
					convert_lldp_port_id_subtype(temp,lldp_port_entry[i].portid_subtype);
					addstr(temp);
					addstr("</td><td>");
					convert_lldp_port_id(temp,
							lldp_port_entry[i].portid_subtype,
							lldp_port_entry[i].port_id);
					addstr(temp);
					addstr("</td><td>");
					sprintf(temp,"%d",lldp_port_entry[i].ttl);
					addstr(temp);
					addstr("</td>"
					"<td><input type=\"button\" value =\"More info\" onclick=\"lldp_info(");
					sprintf(temp,"%d",i);
					addstr(temp);
					addstr(");\"></td>"
					"</tr>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;

				}
			}
			addstr("</table><br><br>"
			"<input type=\"Submit\" name=\"Refresh\" value=\"Refresh\">"
			"</form></body></html>");
		    PSOCK_SEND_STR(&s->sout,str);
		    str[0]=0;
		    PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        LLDP -> LLDP Entry Info 															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_lldpinfo(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static u8 num=0xFF;
	u32 count;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "num")){
				num=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(num<LLDP_ENTRY_NUM){
	    addstr("<html><head>"
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
	    "<!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/ui.css\">-->"
		"<!--[if IE]>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/styleIE.css\">"
		"<![endif]-->");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
	    addstrl("<title>LLDP Enry Info</title>","<title>Информация о записи LLDP</title>");
		addstr("</head><body>");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("<legend>");
		addstrl("Entry #","Запись №");
		sprintf(temp,"%d",num+1);
		addstr(temp);
		addstr("</legend>");
		addstr("<form method=\"get\" bgcolor=\"#808080 \"action=\"/info/lldpinfo.shtml?num=");
		sprintf(temp,"%d",num);
		addstr(temp);
		addstr("\">"
		"<meta http-equiv=\"refresh\" content=\"5; url=/info/lldpinfo.shtml?num=");
		addstr(temp);
		addstr("\">");

		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\"><td>");
		addstrl("Local Port","Локальный порт");
		addstr("</td><td>");
		sprintf(temp,"%d",lldp_port_entry[num].if_index+1);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Chassis ID Subtype", "Подтип идентификатора шасси");
		addstr("</td><td>");
		convert_lldp_chasis_id_subtype(temp,lldp_port_entry[num].chassis_id_subtype);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Chassis ID", "Идентификатор шасси");
		addstr("</td><td>");
		convert_lldp_chasis_id(temp,
			lldp_port_entry[num].chassis_id_subtype,
			lldp_port_entry[num].chassis_id);
		addstr(temp);
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Port ID Subtype", "Подтип идентификатора порта");
		addstr("</td><td>");
		convert_lldp_port_id_subtype(temp,lldp_port_entry[num].portid_subtype);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Port ID", "Идентификатор порта");
		addstr("</td><td>");
		convert_lldp_port_id(temp,
			lldp_port_entry[num].portid_subtype,
			lldp_port_entry[num].port_id);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>"
		"TTL</td><td>");
		sprintf(temp,"%d",lldp_port_entry[num].ttl);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Port Description","Описание порта");
		addstr("</td><td>");
		if(lldp_port_entry[num].port_descr_len){
			addstr(lldp_port_entry[num].port_descr);
		}
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("System Name","Имя устройства");
		addstr("</td><td>");
		if(lldp_port_entry[num].sys_name_len){
			addstr(lldp_port_entry[num].sys_name);
		}
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("System Description","Описание устройства");
		addstr("</td><td>");
		if(lldp_port_entry[num].sys_descr_len){
			addstr(lldp_port_entry[num].sys_descr);
		}

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Supported capabilities","Поддерживаемые возможности");
		addstr("</td><td>");
		convert_lldp_cap(temp,lldp_port_entry[num].sys_cap,get_interface_lang());
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Enableded capabilities","Активные возможности");
		addstr("</td><td>");
		convert_lldp_cap(temp,lldp_port_entry[num].sys_cap_en,get_interface_lang());
		addstr(temp);
		addstr("</td></tr>");
		addstr("</table></form></body></html>");
	}
	else
		addstr("incorrect http request!");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}






/**************************************************************************************/
/*                            BASE SETTINGS -> Telnet*/
/**************************************************************************************/

static PT_THREAD(run_telnet(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	static  uint16_t pcount;
	static  u8 apply=0,state,echo_state;

		  PSOCK_BEGIN(&s->sout);
		  str[0]=0;
		  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

			  int i=0;
			  for(i=0;i<pcount;i++){
		  		char *pname,*pval;
		  		pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		  		pval = http_get_parameter_value(s->param,i,sizeof(s->param));

		  		if(!strcmp(pname, "Apply")){
		  			if (!strncmp(pval, "A", 1)) apply = 1;
		  		}

		  		if(!strcmp(pname, "state")){
		  			if (!strncmp(pval, "0", 1)) state = 0;
		  			if (!strncmp(pval, "1", 1)) state = 1;
		  		}

		  		if(!strcmp(pname, "echo")){
		  			if (!strncmp(pval, "0", 1)) echo_state = 0;
		  			if (!strncmp(pval, "1", 1)) echo_state = 1;
		  		}

		  		if(!strcmp(pname, "tftp_state")){
		  			if (!strncmp(pval, "0", 1)) set_tftp_state(0);
		  			if (!strncmp(pval, "1", 1)) set_tftp_state(1);
		  		}

		  		if(!strcmp(pname, "tftp_port")){
		  			set_tftp_port((u16)strtoul(pval, NULL, 10));
		  		}

			  }

			  if(apply){
				  	set_telnet_state(state);
				  	set_telnet_echo(echo_state);
					settings_save();
					telnetd_init();
					//reboot(REBOOT_MCU_5S);
					alertl("Parameters accepted","Настройки применились");
					apply=0;
			  	}
		  }


		  	state =  get_telnet_state();
			echo_state =  get_telnet_echo();

			addhead();
			addstr("<body>");
			addstrl("<legend>Telnet Settings</legend>","<legend>Настройка Telnet</legend>");
			if(get_current_user_rule()!=ADMIN_RULE){
				  addstr("Access denied");
		    }else{
				addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
						  "action=\"/settings/telnet.shtml\"><br>");

				addstr("<br>"
				"<caption><b>");
				addstrl("Telnet Settings","Настройки Telnet");
				addstr("</b></caption>"
				"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\">"
				"<td>");
				addstrl("State","Состояние");
				addstr("</td>"
				"<td><select name=\"state\" size=\"1\">"
				"<option");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;


				if(state == 0) addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				"<option ");
				if(state == 1) addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				"</select></td></tr>"
				"<tr class=\"g2\"><td>");
				addstrl("Option Echo","Опция Эхо");
				addstr("<td><select name=\"echo\" size=\"1\">"
				"<option");
				if(echo_state == 0) addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				"<option ");
				if(echo_state == 1) addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				"</select></td></tr>"
				"</table>"

				"<br><br><br>"
				"<caption><b>");
				addstrl("TFTP Settings","Настройки TFTP");
				addstr("</b></caption>"
				"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				"<tr class=\"g2\"><td>");
				addstrl("State","Состояние");
				addstr("</td><td>"
				"<select name=\"tftp_state\" size=\"1\">"
				"<option");
				if(get_tftp_state() == 0) addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				"<option ");
				if(get_tftp_state() == 1) addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option></td></tr>"
				"<tr class=\"g2\"><td>");
				addstrl("Port","Порт");
				addstr("</td><td>"
				"<input type=\"text\"name=\"tftp_port\" size=\"4\" maxlength=\"4\"value=\"");
				sprintf(temp,"%d",get_tftp_port());
				addstr(temp);
				addstr("\"></td></tr></table><br><br>"
				"<input type=\"Submit\" name=\"Apply\" value=\"Apply\">"
				"</form></body></html>");
		    }
			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

		  PSOCK_END(&s->sout);
}

/*******************************************************************************************************/
/*        STATICTICS -> DNS TABLE																	    	*/
/*******************************************************************************************************/
static PT_THREAD(run_dns_stat(struct httpd_state *s, char *ptr)){
	/*static*/  uint16_t pcount;
    static u8 i;
	char dn[32];
	uip_ipaddr_t ip;
	str[0]=0;
	  PSOCK_BEGIN(&s->sout);
	  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		   	  for(int i=0;i<pcount;i++){
					  char *pname,*pval;
					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  // Buttons
					  if(!strcmp(pname, "refresh")){
						  if (!strncmp(pval, "r", 1)) {}	//refre = 1;
					  }
		   	  }
	  }
		  str[0]=0;
		  addhead();
		  addstr("<body>");
		  addstrl("<legend>DNS Table</legend>","<legend>таблица DNS</legend>");
		  addstr("<form method=\"post\" bgcolor=\"#808080 \"action=\"/info/dns_stat.shtml\">");
		  //"<meta http-equiv=\"refresh\" content=\"15; url=/info/ARP.shtml\">");
		  addstr( "<br><br>"
		  		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
				  "<tr class=\"g2\"><td width=\"5%\">№</td><td width=\"25%\">IP ");
		  addstrl("address","адрес");
		  addstr("</td><td width=\"40%\">");
		  addstrl("Domain name","Доменное имя");
		  addstr("</td></tr>");

		  /* Walk through the list to see if the name is in there. If it is
		  not, we return NULL. */
		  for(i = 0; i < RESOLV_ENTRIES; ++i) {
			  resolv_list(i,dn,ip);
			  if((ip[0]!=0)&&(ip[1]!=0)){
				  addstr("<tr class=\"g2\"><td>");
				  sprintf(temp,"%d</td><td>%02d.%02d.%02d.%02d</td><td>%s</td></tr>",i+1,uip_ipaddr1(ip),
				  uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip),dn);
				  addstr(temp);
			  }
		  }
		  if(i==0){
			  addstr("<tr class=\"g2\"><td></td><td></td><td></td>");
		  }
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  addstr(
		  "</table>"
	      "<br><br>"
		  "<br><br>"
		  "<table width=\"100%\"><tr><td>"
		  "<input type=\"Submit\" value=\"Refresh\" width=\"200\"></td><td>"
		  "</tr></table>");
	/***********************************************************************/
     	  addstr(
		  "</form></body></html>");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  PSOCK_END(&s->sout);
}


#define ADDSCRIPT											\
	  addstr("<script type=\"text/javascript\">"			\
	  "function drawGraph (id, arr, border_h, border_w){"	\
		"var example = document.getElementById(id);"		\
		"var canvas  = example.getContext('2d');"			\
		"var max_val = 0;"									\
		"var resize_coeff  = 1.3;"							\
		"example.height = border_h+20;"						\
		"example.width  = border_w+40;"						\
		"for (var i = 0; i < arr.length; i++){"				\
		"if(arr[i]>max_val){"								\
			"max_val = arr[i];"								\
		"}"													\
		"}"													\
		"if(max_val==0){max_val=1;}");						\
		PSOCK_SEND_STR(&s->sout,str);						\
		str[0]=0;											\
	  addstr(												\
		"canvas.beginPath();"								\
		"canvas.strokeRect(0,0,border_w+40,border_h+20);"	\
		"canvas.strokeRect(30, 10, border_w, border_h);"	\
		"canvas.strokeStyle = '#7D7D7D';"					\
		"canvas.stroke();"									\
		"canvas.beginPath();"								\
		"for(var i=0;i<10;i++){"							\
		 "canvas.font = '12pt Calibri';"					\
		 "canvas.fillStyle = 'blue';"						\
		 "if(i == 0){"										\
			"/*canvas.fillText(\"Mb/s\",0,15);*/"				\
		 "}"												\
		 "else{"											\
			"canvas.fillText(parseFloat((max_val*resize_coeff-max_val*resize_coeff/10*i).toFixed(1)), 5,(border_h+10)/10*i+5);"\
		 "}"												\
		 "canvas.moveTo(30,(border_h+10)/10*i);"			\
		 "canvas.lineTo(border_w+30,(border_h+10)/10*i);"	\
		"}"													\
		"for(var i=0;i<5;i++){"								\
		   "canvas.moveTo((border_w)/5*i+30,10);"			\
		   "canvas.lineTo((border_w)/5*i+30,border_h+10);"	\
		"}"													\
		"canvas.strokeStyle = '#C9C9C9';"					\
		"canvas.stroke();"									\
		"canvas.beginPath();"								\
		"canvas.lineJoin = 'round';"						\
		"canvas.lineWidth = 3;"								\
		"canvas.strokeStyle = '#006400';"					\
		"canvas.moveTo(30, border_h-(arr[0])*border_h/(max_val*resize_coeff)+10);"\
		"for (var i = 0; i < arr.length; i++){"				\
			"canvas.lineTo(i*border_w/(arr.length-1)+30, border_h - (arr[i])*border_h/(max_val*resize_coeff));"\
		"}"													\
		"canvas.stroke();"									\
	   "}"													\
	   "function open_url(url)"								\
	   "{"													\
		 "parent.ContentFrame.location.href= url;"			\
	   "}"													\
	  "</script>");



/*******************************************************************************************************/
/*        BACKDOOR - Developers mode																	    	*/
/*******************************************************************************************************/
static PT_THREAD(run_devmode(struct httpd_state *s, char *ptr)){
   static  uint16_t pcount = 0;
   str[0]=0;
   static u8 authed = 1;
   static u8 apply;
   static u8 downshift;

   static uint8_t action=0;
   static uint8_t ID_=0;
   static uint8_t VAC=0;
   static uint8_t AKB_V=0,AKB_Z = 0;;
   static int8_t TEMP=0;
   static uint8_t CURR=0;
   static uint8_t VERS=0;
   static u8 sfp_mode[2];

   static u8 d_devaddr=0;
   static u8 d_regaddr=0;
   static u16 d_value=0;
   static u8 d_read;
   static u8 d_write;

   static u8 i_devaddr = 0;
   static u8 i_regaddr = 0;
   static u8 i_page = 0;
   static u16 i_value = 0;
   static u8 i_read;
   static u8 i_write;

   static u8 p_devaddr=0;
   static u8 p_regaddr=0;
   static u16 p_value=0;
   static u8 p_read;
   static u8 p_write;

   static u8 s_devaddr;
   static u8 s_regaddr;
   static u8 s_value[32];
   static u8 s_read_1,s_read_2;
   static u8 s_write;




   PSOCK_BEGIN(&s->sout);
   if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		   	  for(i=0;i<pcount;i++){
					  char *pname,*pval;

					  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
					  pval = http_get_parameter_value(s->param,i,sizeof(s->param));

					  if(!strcmp(pname, "pass_str")){//0p8kd5
						 // if (!strcmp(pval, dev_pass)) {
								authed = 1;
						 // }
					  }

					  if(authed == 1){

						  //sfp mode
						  for(u8 j=0;j<FIBER_PORT_NUM;j++){
							  sprintf(temp,"SFP%d",j+1);
							  if(!strcmp(pname, temp)){
								  sfp_mode[j]=(uint8_t)strtoul(pval, NULL, 10);
							  }
						  }

						  if(!strcmp(pname, "Apply")){
							  if (!strncmp(pval, "A", 1)) apply = 1;
						  }

						  if(!strcmp(pname, "downshift")){
							  if (!strncmp(pval, "0", 1))
								  downshift = 0;

							  if (!strncmp(pval, "1", 1))
								  downshift = 1;
						  }

						  if(!strcmp(pname, "Clear")){
							  if (!strncmp(pval, "C", 1)) {
									xTaskCreate( xEraseBB_Task, (void*)"erase_bb",256, NULL, DEFAULT_PRIORITY, &xEraseBBHandle );
									alert("Ок!");
							  }
						   }


						  if(!strcmp(pname, "refresh")){
							  if (!strncmp(pval, "r", 1)) {}	//refre = 1;
						  }

						  //ups
						  //кнопки //
						  if(!strcmp(pname, "rID")){
							  if (!strncmp(pval, "R", 1)) 	action=5;
						  }
						  if(!strcmp(pname, "rVAC")){
							  if (!strncmp(pval, "R", 1)) 	action=6;
						  }
						  if(!strcmp(pname, "rAKBv")){
							  if (!strncmp(pval, "R", 1)) 	action=8;
						  }
						  if(!strcmp(pname, "rTEMP")){
							  if (!strncmp(pval, "R", 1)) 	action=9;
						  }
						  if(!strcmp(pname, "rCURR")){
							  if (!strncmp(pval, "R", 1)) 	action=13;
						  }

						  if(!strcmp(pname, "rMCU")){
							  if (!strncmp(pval, "R", 1)) 	action=43;
						  }

						  if(!strcmp(pname, "rVE")){
							  if (!strncmp(pval, "S", 1)) 	read_byte_reg(IRP_ADDR,Ventil_Enable);
						  }

						  if(!strcmp(pname, "rVD")){
							  if (!strncmp(pval, "S", 1)) 	read_byte_reg(IRP_ADDR,Ventil_Disable);
						  }

						  if(!strcmp(pname, "rVERS")){
							  if (!strncmp(pval, "R", 1)) 	action = 44;
						  }

						  if(!strcmp(pname, "rAKBz")){
							  if (!strncmp(pval, "R", 1)) 	action=47;
						  }

						  //smi
						  if(!strcmp(pname, "d_devaddr")){
							  d_devaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "d_regaddr")){
							  d_regaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "d_value")){
							  d_value=(u16)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "d_read")){
							  if (!strncmp(pval, "R", 1)) 	d_read = 1;
						  }
						  if(!strcmp(pname, "d_write")){
							  if (!strncmp(pval, "W", 1)) 	d_write = 1;
						  }
						  //indirect
						  if(!strcmp(pname, "i_devaddr")){
							  i_devaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "i_regaddr")){
							  i_regaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "i_page")){
							  i_page=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "i_value")){
							  i_value=(u16)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "i_read")){
							  if (!strncmp(pval, "R", 1)) 	i_read = 1;
						  }
						  if(!strcmp(pname, "i_write")){
							  if (!strncmp(pval, "W", 1)) 	i_write = 1;
						  }
						  //poe i2c
						  if(!strcmp(pname, "p_devaddr")){
							  p_devaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "p_regaddr")){
							  p_regaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "p_value")){
							  p_value=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "p_read")){
							  if (!strncmp(pval, "R", 1)) 	p_read = 1;
						  }
						  if(!strcmp(pname, "p_write")){
							  if (!strncmp(pval, "W", 1)) 	p_write = 1;
						  }
						  //software i2c
						  if(!strcmp(pname, "s_devaddr")){
							  s_devaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "s_regaddr")){
							  s_regaddr=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "s_value")){
							  strcpy((char *)s_value,pval);
							  //s_value=(u8)strtoul(pval, NULL, 16);
						  }
						  if(!strcmp(pname, "s_read_1")){
							  if (!strncmp(pval, "R", 1)) 	s_read_1 = 1;
						  }
						  if(!strcmp(pname, "s_read_2")){
							  if (!strncmp(pval, "R", 1)) 	s_read_2 = 1;
						  }
						  if(!strcmp(pname, "s_write")){
							  if (!strncmp(pval, "W", 1)) 	s_write = 1;
						  }


					  }
		   	  }

	  		  //обработчик команд//
		   	  if(authed == 1){
				  switch(action){
					case 5:
						ID_=read_byte_reg(IRP_ADDR,ID_READ);
						break;
					case 6:
						VAC=read_byte_reg(IRP_ADDR,VAC_READ);
						break;
					case 8:
						AKB_V=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_READ);
						break;
					case 9:
						TEMP=read_byte_reg(IRP_ADDR,TEMPER_READ);
						break;
					case 13:
						CURR=read_byte_reg(IRP_ADDR,CURR_KEY_READ);
						break;
					case 43:
						read_byte_reg(IRP_ADDR,Reset_IPR);
						break;
					case 44:
						VERS = read_byte_reg(IRP_ADDR,Version_READ);
						break;
					case 47:
						AKB_Z=read_byte_reg(IRP_ADDR,AKB_VOLTAGE_CHARGE_READ);
						break;
				  }

				  if(apply == 1){
					  set_downshifting_mode(downshift);
					  for(u8 i=0;i<FIBER_PORT_NUM;i++){
						  set_port_sfp_mode(COOPER_PORT_NUM+i,sfp_mode[i]);
					  }
					  settings_save();
					  apply=0;
					  alertl("Parameters accepted","Настройки применились");

				  }

				  //smi
				  if(d_read == 1){
					  d_value = ETH_ReadPHYRegister(d_devaddr,d_regaddr);
					  d_read = 0;
				  }
				  if(d_write == 1){
					   ETH_WritePHYRegister(d_devaddr,d_regaddr,d_value);
					  d_write = 0;
				  }
				  if(i_read == 1){
					  i_value = ETH_ReadIndirectPHYReg(i_devaddr,i_page,i_regaddr);
					  i_read = 0;
				  }
				  if(i_write == 1){
					  ETH_WriteIndirectPHYReg(i_devaddr,i_page,i_regaddr,i_value);
					  i_write = 0;
				  }
				  if(p_read == 1){
					  p_value = I2c_ReadByte(p_devaddr,p_regaddr);
					  p_read = 0;
				  }
				  if(p_write == 1){
					  I2c_WriteByteData(p_devaddr,p_regaddr,p_value);
					  p_write = 0;
				  }

				  if(s_read_1 == 1){
					  i2c_buf_read(s_devaddr,s_regaddr,s_value,1);
					  s_read_1 = 0;
				  }
				  if(s_read_2 == 1){
					  i2c_buf_read(s_devaddr,s_regaddr,s_value,3);
					  s_read_2 = 0;
				  }
				  if(s_write == 1){
					  i2c_reg_write(s_devaddr,s_regaddr,s_value);
					  s_write = 0;
				  }

		   	  }
	  }

	  str[0]=0;
	  addstr("<head>"
			  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
			  "<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">");
	  ADDSCRIPT
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  addstr("</head><body>");
	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;

	  PSOCK_SEND_STR(&s->sout,str);
	  str[0]=0;
	  addstr("<legend>Developers page</legend>");


	  addstr( "<form method=\"post\" bgcolor=\"#808080\" enctype=\"multipart/form-data\""
				  "action=\"/devmode.shtml\">");

	  if(authed!=1 || get_current_user_rule()!=ADMIN_RULE){
		  addstr("<input type=\"text\" name=\"pass_str\" size=\"10\" maxlength=\"10\"value=\"\">");
		  addstr("<input type=\"Submit\" value=\"Enter\" width=\"200\">");
	  }else{

#if 1
		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		  if(get_dev_type()!=DEV_SWU16){
			  for(u8 i=0;i<FIBER_PORT_NUM;i++){
				  sprintf(temp,"SFP%d",i+1);
				  addstr("<tr class=\"g2\"><td>");
				  addstr(temp);
				  addstr("</td><td><select name=\"");
				  addstr(temp);
				  addstr("\" size=\"1\">"
				  "<option");
				  if(get_port_sett_sfp_mode(COOPER_PORT_NUM+i)==0){
					addstr(" selected");
				  }
				  addstr(" value=\"0\">Forced</option>"
				  "<option");
				  if( get_port_sett_sfp_mode(COOPER_PORT_NUM+i)==1){
					addstr(" selected");
				  }
				  addstr(" value=\"1\">Auto</option>"
				  "<option");
				  if( get_port_sett_sfp_mode(COOPER_PORT_NUM+i)==2){
					addstr(" selected");
				  }
				  addstr(" value=\"2\">Hard Forced</option>"
				  "</select>");

			  }
			  addstr("</td>");
		  }
		  addstr("<tr class=\"g2\"><td>");
		  //downshifting mode
		  addstr("Downshifting mode</td><td>"
		  "<select name=\"downshift\" size=\"1\">"
		  "<option");
		  if(get_downshifting_mode() == 0) addstr(" selected");
		  addstr(" value=\"0\">");
		  addstrl("Disable","Выключено");
		  addstr("</option>"
		  "<option ");
		  if(get_downshifting_mode()  == 1) addstr(" selected");
		  addstr(" value=\"1\">");
		  addstrl("Enable","Включено");
		  addstr("</option></td></tr>");
		  //clear log
		  addstr("<tr class=\"g2\"><td>Clear log</td>"
		  "<td><input type=\"Submit\" name=\"Clear\" value=\"Clear\"></td></tr>");

		  addstr("</table><br>");
		  addstr("<input type=\"Submit\" name=\"Apply\" value=\"Apply\" width=\"200\"><br><br>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  //ups
		  addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\">"
		  "<td>");
		  addstrl("1. Board HW ID","1. Идентификатор платы");
		  addstr("</td><td>");
		  sprintf(temp,"%d",ID_);
		  addstr(temp);
		  addstr("</td><td><input type=\"Submit\" name=\"rID\" value=\"Read\" width=\"100\"></td></tr>"
		  //наличие сетевого напряжения
		  "<tr class=\"g2\"><td>");
		  addstrl("2. VAC","2. Наличие сетевого напр.");
		  addstr("</td><td>");
		  if(VAC==1)
			addstrl("Yes","Eсть");
		  if(VAC==0)
			addstrl("No","Нет");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  addstr("</td><td><input type=\"Submit\" name=\"rVAC\" value=\"Read\" width=\"100\"></td></tr>"
	      //напряжение на АКБ
		  "<tr class=\"g2\"><td>");
		  addstrl("3. Barrery voltage","3. Напряжение на АКБ");
		  addstr("</td><td>");
		  sprintf(temp,"%d.%d",(35+AKB_V/10),(AKB_V%10));
		  addstr(temp);
		  addstr("</td><td><input type=\"Submit\" name=\"rAKBv\" value=\"Read\" width=\"100\"></td></tr>"
		  //Температура
		  "<tr class=\"g2\"><td>");
		  addstrl("4. Temper.","4. Температура");
		  addstr("</td><td>");
		  sprintf(temp,"%d",TEMP);
		  addstr(temp);
		  addstr("</td><td><input type=\"Submit\" name=\"rTEMP\" value=\"Read\" width=\"100\"></td></tr>"
		  "<tr class=\"g2\"><td>");
		  addstrl("5. Charge current","5. Ток заряда");
		  addstr("</td><td>");
		  if(CURR==1)
			 addstr("Hi");
		  else if(CURR==0)
			 addstr("Low");
		  else addstr("---");
		  addstr("</td><td><input type=\"Submit\" name=\"rCURR\" value=\"Read\" width=\"100\">"
		  "</td></tr>"

		  "<tr class=\"g2\"><td>");
		  addstrl("6. CPU reboot","6. Перезагрузка процессора");
		  addstr("</td><td>"
		  "</td><td><input type=\"Submit\" name=\"rMCU\" value=\"Reset\" width=\"100\"></td></tr>"

		  "<tr class=\"g2\"><td>");
		  addstrl("7. Fans ON","7. Вентиляторы Вкл.");
		  addstr("</td><td>"
		  "</td><td><input type=\"Submit\" name=\"rVE\" value=\"Set\" width=\"100\"></td></tr>"

		  "<tr class=\"g2\"><td>");
		  addstrl("8. Fans OFF","8. Вентиляторы Выкл.");
		  addstr("</td><td>"
		  "</td><td><input type=\"Submit\" name=\"rVD\" value=\"Set\" width=\"100\"></td></tr>"

		  //Версия ПО IRP
		  "<tr class=\"g2\"><td>");
		  addstrl("9. FW vers","9. Версия ПО");
		  addstr("</td><td>");
		  sprintf(temp,"%d",VERS);
		  addstr(temp);
		  addstr("</td><td><input type=\"Submit\" name=\"rVERS\" value=\"Read\" width=\"100\"></td></tr>"

		  //Напряжение зарядки АКБ
		  "<tr class=\"g2\"><td>");
		  addstrl("10. Charge voltage","10. Напряжение Зарядки");
		  addstr("</td><td>");
		  sprintf(temp,"%d.%d",(35+AKB_Z/10),(AKB_Z%10));
		  addstr(temp);
		  addstr("</td><td><input type=\"Submit\" name=\"rAKBz\" value=\"Read\" width=\"100\"></td></tr>"
		  "</table>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

		  //smi interface
		  addstr("<br><br>"
		  "SMI Interface<br>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td colspan=\"2\">Direct access</td></tr>"
		  "<tr class=\"g2\"><td>DevAddr</td><td>"
		  "<input type=\"text\"name=\"d_devaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",d_devaddr);
		  addstr(temp);
	      addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>RegAddr</td><td>"
		  "<input type=\"text\"name=\"d_regaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",d_regaddr);
		  addstr(temp);
	      addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>Value</td><td>"
		  "<input type=\"text\"name=\"d_value");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",d_value);
		  addstr(temp);
		  addstr("\"></td></tr>");
		  addstr("<tr class=\"g2\">"
				  "<td><input type=\"Submit\" name=\"d_read\" value=\"Read\" width=\"100\"></td>"
				  "<td><input type=\"Submit\" name=\"d_write\" value=\"Write\" width=\"100\"></td>"
		  "</tr>");
		  addstr("<tr class=\"g2\"><td colspan=\"2\"> </td></tr>");
		  //indirect access
		  addstr("<tr class=\"g2\"><td colspan=\"2\">Indirect access</td></tr>"
		  "<tr class=\"g2\"><td>DevAddr</td><td>"
		  "<input type=\"text\"name=\"i_devaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",i_devaddr);
		  addstr(temp);
	      addstr("\"></td></tr>"
	      "<tr class=\"g2\"><td>Page</td><td>"
		  "<input type=\"text\"name=\"i_page");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",i_page);
		  addstr(temp);
		  addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>RegAddr</td><td>"
		  "<input type=\"text\"name=\"i_regaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",i_regaddr);
		  addstr(temp);
	      addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>Value</td><td>"
		  "<input type=\"text\"name=\"i_value");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",i_value);
		  addstr(temp);
		  addstr("\"></td></tr>");
		  addstr("<tr class=\"g2\">"
				  "<td><input type=\"Submit\" name=\"i_read\" value=\"Read\" width=\"100\"></td>"
				  "<td><input type=\"Submit\" name=\"i_write\" value=\"Write\" width=\"100\"></td>"
		  "</tr></table>");

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  //i2c access
		  addstr("<br><br>I2C Interface<br>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td>DevAddr</td><td>"
		  "<input type=\"text\"name=\"p_devaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",p_devaddr);
		  addstr(temp);
		  addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>RegAddr</td><td>"
		  "<input type=\"text\"name=\"p_regaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",p_regaddr);
		  addstr(temp);
		  addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>Value</td><td>"
		  "<input type=\"text\"name=\"p_value");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",p_value);
		  addstr(temp);
		  addstr("\"></td></tr>");
		  addstr("<tr class=\"g2\">"
				  "<td><input type=\"Submit\" name=\"p_read\" value=\"Read\" width=\"100\"></td>"
				  "<td><input type=\"Submit\" name=\"p_write\" value=\"Write\" width=\"100\"></td>"
		  "</tr>");
		  addstr("</table>"
		  "<br><br>"
		  "<caption><b>Temperature</b></caption>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td>CPU</td><td>");
		  sprintf(temp,"%i",get_cpu_temper());
		  addstr(temp);
		  addstr("</td></tr>"
		  "<tr class=\"g2\"><td>"
		  "Marvell</td><td>");
		  sprintf(temp,"%i",get_marvell_temper());
		  addstr(temp);
		  addstr("</td></tr>"
		  "<tr class=\"g2\"><td>"
		  "PoE</td><td>");
		  sprintf(temp,"%d",get_poe_temper());
		  addstr(temp);
		  addstr("</td></tr>"
		  "</table>");

		  sprintf(temp,"<br>USB %lX<br>",(*(uint32_t*)0x50000000));
		  addstr(temp);

		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;

#endif

		  addstr("<caption><b>I2C Software</b></caption>"
		  "<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		  "<tr class=\"g2\"><td>DevAddr</td><td>"
		  "<input type=\"text\"name=\"s_devaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",s_devaddr);
		  addstr(temp);
		  addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>RegAddr</td><td>"
		  "<input type=\"text\"name=\"s_regaddr");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  sprintf(temp,"%X",s_regaddr);
		  addstr(temp);
		  addstr("\"></td></tr>"
		  "<tr class=\"g2\"><td>Value</td><td>"
		  "<input type=\"text\"name=\"s_value");
		  addstr("\" size=\"3\" maxlength=\"3\"value=\"");
		  //sprintf(temp,"%X",s_value);
		  addstr((char *)s_value);
		  addstr("\"></td></tr>");
		  addstr("<tr class=\"g2\"><td>Probe</td><td>");
		  //sprintf(temp,"%d",i2c_probe(s_devaddr));
		  addstr(temp);
		  addstr("</td></tr><tr class=\"g2\">"
		  "<td><input type=\"Submit\" name=\"s_read_1\" value=\"Read 1\" width=\"100\">"
		  "<input type=\"Submit\" name=\"s_read_2\" value=\"Read 3\" width=\"100\"></td>"
		  "<td><input type=\"Submit\" name=\"s_write\" value=\"Write\" width=\"100\"></td>"
		  "</tr>"
		  "</table>");

		  //poe status
		  addstr("<br><b>Block 1</b>");
		  sprintf(temp,"<br>IntStat %X <br>",I2c_ReadByte(Adress4271block1,INTSTAT));
		  addstr(temp);
		  sprintf(temp,"<br>Fault Event Reg %X <br>",I2c_ReadByte(Adress4271block1,FLTEVN_COR));
		  addstr(temp);
		  sprintf(temp,"<br>t_stat Event Reg %X <br>",I2c_ReadByte(Adress4271block1,TSEVN_COR));
		  addstr(temp);

		  addstr("<br><br><b>Block 2</b>");
		  sprintf(temp,"<br>IntStat %X <br>",I2c_ReadByte(Adress4271block2,INTSTAT));
		  addstr(temp);
		  sprintf(temp,"<br>Fault Event Reg %X <br>",I2c_ReadByte(Adress4271block2,FLTEVN_COR));
		  addstr(temp);
		  sprintf(temp,"<br>t_stat Event Reg %X <br>",I2c_ReadByte(Adress4271block2,TSEVN_COR));
		  addstr(temp);




		addstr("<br><input type=\"Submit\" name=\"Refresh\" value=\"Refresh\">");


		addstr("<br><br><b>Autorestart stat</b><br>");
		for(u8 i=0;i<POE_PORT_NUM;i++){
			sprintf(temp,"Port %d AR reset cnt %d<br>",i+1,dev.port_stat[i].rst_cnt);
			addstr(temp);
		}



	  }


	/***********************************************************************/
     	  addstr(
		  "</form></body></html>");
		  PSOCK_SEND_STR(&s->sout,str);
		  str[0]=0;
		  PSOCK_END(&s->sout);
}






static void addcoord(u8 port){
	  addstr("<script type=\"text/javascript\">"
	  "var coordArrayRx = [");
	  for(u8 i=0;i<SPEED_STAT_LEN;i++){
		  sprintf(temp,"%d",dev.port_stat[port].rx_speed_stat[i]);
		  addstr(temp);
		  if(i==(SPEED_STAT_LEN-1))
			  addstr("];");
		  else
			  addstr(",");
	  }
	  addstr("var coordArrayTx = [");
	  for(u8 i=0;i<SPEED_STAT_LEN;i++){
		  sprintf(temp,"%d",dev.port_stat[port].tx_speed_stat[i]);
		  addstr(temp);
		  if(i==(SPEED_STAT_LEN-1))
			  addstr("];");
		  else
			  addstr(",");
	  }
	  addstr("</script>");
}

/*******************************************************************************************************/
/*        STATICTICS -> Port  info 															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_port_info(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static u8 port=0xFF;
	u32 count;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(port<(COOPER_PORT_NUM+FIBER_PORT_NUM)){
	    addstr("<html><head>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
	    "<!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/ui.css\">-->"
		"<!--[if IE]>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/styleIE.css\">"
		"<![endif]-->");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		ADDSCRIPT
	    addstr("<title>Port Info</title>"
		"</head><body>");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		addcoord(port);
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		addstr("<legend>Port #");
		sprintf(temp,"%d",port+1);
		addstr(temp);
		addstr("</legend>");
		sprintf(temp,"%d",port);
		addstr("<form method=\"get\" bgcolor=\"#808080 \"action=\"/info/port_info.shtml?port=");
		addstr(temp);
		addstr("\">"
		"<meta http-equiv=\"refresh\" content=\"5; url=/info/port_info.shtml?port=");
		addstr(temp);
		addstr("\">"
		"<caption><b>Speed Statistics</b></caption>"
		"<table width=\"100%\" cellspacing=\"10\">"
		"<tr>"
			"<td>RX Speed</td>"
			"<td>TX Speed</td>"
		"</tr>"
		"<tr>"
			"<td><canvas id='canvasIdRx'>Извините, тег Canvas недоступен!</canvas></td>"
			"<td><canvas id='canvasIdTx'>Извините, тег Canvas недоступен!</canvas></td>"
		"</tr>"
		"</table>"
		"<script type=\"text/javascript\">"
		  "drawGraph('canvasIdRx', coordArrayRx,300,400);"
		  "drawGraph('canvasIdTx', coordArrayTx,300,400);"
		"</script>"
		"<br><br>"

		"<table width=\"100%\" cellspacing=\"10\">"
		"<tr>"
			"<td>"
			"<b>Port Statistics</b>"
			"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td></td><td>RX</td><td>TX</td></tr>");

			//all PSW`s
			if(get_dev_type() != DEV_SWU16){
				//total	good bytes
				addstr(
				"<tr class=\"g2\"><td>Good bytes</td>"
				"<td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].rx_good);
				addstr(temp);
				addstr("</td>"
				"<td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].tx_good);
				addstr(temp);
				addstr("</td></tr>"
				//bad bytes
				"<tr class=\"g2\">"
				"<td>Bad bytes</td>"
				"<td>");
				sprintf(temp,"%lu",(u32)(read_stats_cnt(port,RX_BAD)));
				addstr(temp);
				addstr("</td><td>-</td></tr>"

				//collision
				"<tr class=\"g2\"><td>Collision packets</td>"
				"<td>");
				sprintf(temp,"%lu",(u32)(read_stats_cnt(port,COLLISION)));
				addstr(temp);
				addstr("</td><td>-</td></tr>");


				addstr("<tr class=\"g2\"><td>Discards packets</td><td>");
				sprintf(temp,"%d",(uint16_t)(InDiscardsFrameCount(L2F_port_conv(port))));
				addstr(temp);
				addstr("</td><td>-</td></tr>"

				"<tr class=\"g2\"><td>Filtered packets</td><td>");
				sprintf(temp,"%d",(uint16_t)(InFilteredFrameCount(L2F_port_conv(port))));
				addstr(temp);
				addstr("</td><td>");
				sprintf(temp,"%d",(uint16_t)(OutFilteredFrameCount(L2F_port_conv(port))));
				addstr(temp);
				addstr("</td></tr>");


				addstr("<tr class=\"g2\"><td>Unicast  packets</td><td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].rx_unicast);
				addstr(temp);
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].tx_unicast);
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Broadcast  packets</td><td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].rx_broadcast);
				addstr(temp);
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].tx_broadcast);
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Multicast  packets</td><td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].rx_multicast);
				addstr(temp);
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)dev.port_stat[port].tx_multicast);
				addstr(temp);
				addstr("</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;


				//new
				addstr("<tr class=\"g2\"><td>FCS Errors</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_FCSERR));
				addstr(temp);
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,TX_FCSERR));
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Pause</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_PAUSE));
				addstr(temp);
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,TX_PAUSE));
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Undersize</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_UNDERSIZE));
				addstr(temp);
				addstr("</td><td>");
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Oversize</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_OVERSIZE));
				addstr(temp);
				addstr("</td><td>");
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Fragments</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_FARGMENTS));
				addstr(temp);
				addstr("</td><td>");
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Jabber</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_JABBER));
				addstr(temp);
				addstr("</td><td>");
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>MACRcvError</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,RX_RXERR));
				addstr(temp);
				addstr("</td><td>");
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Deferred</td><td>");
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,DEFERRED));
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Excessive</td><td>");
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,EXCESSIVE));
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Single</td><td>");
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,SINGLE));
				addstr(temp);
				addstr("</td></tr>");

				addstr("<tr class=\"g2\"><td>Multiple</td><td>");
				addstr("</td><td>");
				sprintf(temp,"%lu",(u32)read_stats_cnt(port,MULTIPLE));
				addstr(temp);
				addstr("</td></tr>");



			}else{

				//SWU-16

				//good bytes
				addstr("<tr class=\"g2\">"
				"<td>Good bytes</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_GOOD,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>");
				if(Salsa2_Read_Counter(SALSA2_TX_GOOD,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//bad bytes
				addstr("<tr class=\"g2\">"
				"<td>Bad bytes</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_BAD,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");


				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;


				//good frames
				addstr("<tr class=\"g2\">"
				"<td>Good frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_GOOD_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>");
				if(Salsa2_Read_Counter(SALSA2_TX_GOOD_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//bad frames received
				addstr("<tr class=\"g2\">"
				"<td>Bad frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_BAD_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//multicast frames received
				addstr("<tr class=\"g2\"><td>Multicast frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_MULTICAST,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>");
				if(Salsa2_Read_Counter(SALSA2_TX_MULTICAST,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;


				//broadcast
				addstr("<tr class=\"g2\"><td>Broadcast frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_BROADCAST,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>");
				if(Salsa2_Read_Counter(SALSA2_TX_BROADCAST,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Frames 0-64
				addstr("<tr class=\"g2\"><td>Frames Min - 64</td>"
				"<td colspan=\"2\">");
				if(Salsa2_Read_Counter(SALSA2_64_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Frames 65-127
				addstr("<tr class=\"g2\"><td>Frames 65 - 127</td>"
				"<td colspan=\"2\">");
				if(Salsa2_Read_Counter(SALSA2_65_127_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Frames 128-255
				addstr("<tr class=\"g2\"><td>Frames 128 - 255</td>"
				"<td colspan=\"2\">");
				if(Salsa2_Read_Counter(SALSA2_127_255_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Frames 256-511
				addstr("<tr class=\"g2\"><td>Frames 256 - 511</td>"
				"<td colspan=\"2\">");
				if(Salsa2_Read_Counter(SALSA2_256_511_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Frames 512 - 1023
				addstr("<tr class=\"g2\"><td>Frames 512 - 1023</td>"
				"<td colspan=\"2\">");
				if(Salsa2_Read_Counter(SALSA2_512_1023_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;


				//Frames 1024 - Max
				addstr("<tr class=\"g2\"><td>Frames 1024 - MAX</td>"
				"<td colspan=\"2\">");
				if(Salsa2_Read_Counter(SALSA2_1024_MAX_FRAMES,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Good FlowControl frames
				addstr("<tr class=\"g2\"><td>Good Flow Control frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_GOOD_FCR,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>");
				if(Salsa2_Read_Counter(SALSA2_TX_FCR,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

				//Bad FlowControl frames
				addstr("<tr class=\"g2\"><td>Bad Flow Control frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_BAD_FCR,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				//Unrecognized MAC Control
				addstr("<tr class=\"g2\">"
				"<td>Unrecognized MAC Control frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_UNRECOGN_MAC,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Undersize frames
				addstr("<tr class=\"g2\"><td>Undersize frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_UNDERSIZE,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Oversize frames
				addstr("<tr class=\"g2\"><td>Oversize frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_OVERSIZE,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Fragments
				addstr("<tr class=\"g2\"><td>Fragments</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_FRAGMENTS,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Jabber
				addstr("<tr class=\"g2\"><td>Jabber frames</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_JABBER,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//MAC Error
				addstr("<tr class=\"g2\"><td>MAC Errors</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_RX_MAC_ERR,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				//BAD CRC
				addstr("<tr class=\"g2\"><td>CRC Errors</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_BAD_CRC,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Collision
				addstr("<tr class=\"g2\"><td>Collision</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_COLLISION,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Late Collision
				addstr("<tr class=\"g2\"><td>Collision</td>"
				"<td>");
				if(Salsa2_Read_Counter(SALSA2_LATE_COLLISION,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td><td>-</td></tr>");

				//Excessive Collision
				addstr("<tr class=\"g2\">"
				"<td>Excessive Collision</td>"
				"<td>-</td><td>");
				if(Salsa2_Read_Counter(SALSA2_MAC_COLLISION,port,&count)==GT_OK){
					sprintf(temp,"%lu",count);
					addstr(temp);
				}
				addstr("</td></tr>");

			}



	    addstr("</table>"
		"</td>");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;


		if(POE_PORT_NUM){
			addstr("<td>"
			"<b>PoE Status</b>"
			"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\">"
			"<td></td><td>Type A</td>"
			"<td>Type B</td></tr>"

			"<tr class=\"g2\">"
			"<td>State</td><td>");
			if(get_port_poe_a(port))
				addstr("ON");
			else
				addstr("OFF");
			addstr("</td><td>");
			if(get_port_poe_b(port))
				addstr("ON");
			else
				addstr("OFF");
			addstr("</td></tr>"
			//voltage
			"<tr class=\"g2\"><td>Voltage</td><td>");
			if(get_port_poe_a(port)){
				sprintf(temp,"%d.%d V",get_poe_voltage(POE_A,port)/1000,get_poe_voltage(POE_A,port)%1000);
				addstr(temp);
			}
			addstr("</td>"
			"<td>");
			if(get_port_poe_b(port)){
				sprintf(temp,"%d.%d V",get_poe_voltage(POE_B,port)/1000,get_poe_voltage(POE_B,port)%1000);
				addstr(temp);
			}
			addstr("</td></tr>"
			//current
			"<tr class=\"g2\"><td>Current</td><td>");
			if(get_port_poe_a(port)){
				sprintf(temp,"%d mA",get_poe_current(POE_A,port));
				addstr(temp);
			}
			addstr("</td><td>");
			if(get_port_poe_b(port)){
				sprintf(temp,"%d mA",get_poe_current(POE_B,port));
				addstr(temp);
			}
			addstr("</td></tr>"
			//power
			"<tr class=\"g2\"><td>Power</td>"
			"<td>");
			if(get_port_poe_a(port)){
				sprintf(temp,"%d.%d W",(int)(get_poe_voltage(POE_A,port)*get_poe_current(POE_A,port)/1000000),
						(int)((get_poe_voltage(POE_A,port)*get_poe_current(POE_A,port)/1000) % 1000));
				addstr(temp);
			}
			addstr("</td><td>");
			if(get_port_poe_b(port)){
				sprintf(temp,"%d.%d W",(int)(get_poe_voltage(POE_B,port)*get_poe_current(POE_B,port)/1000000),
					(int)((get_poe_voltage(POE_B,port)*get_poe_current(POE_B,port)/1000) % 1000));
				addstr(temp);
			}
			addstr("</td></tr>");
			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;
			addstr("</table>");

			addstr("</td>");
			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;
		}

		addstr("</tr></table></form></body></html>");
	}
	else
		addstr("incorrect http request!");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}




/*******************************************************************************************************/
/*        MIB LIST															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_mib_list(struct httpd_state *s, char *ptr2)){
	static 	int i;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);

	extern mib_object_t* ptr;
	addhead();
	addstr("<body>");

	addstrl("<legend>OID Entries</legend>","<legend>OID записи</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstr("Access denied");
	}else{
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		i=0;
		mib_iterator_init();
		while (!mib_iterator_is_end()) {
		    addstr("<tr class=\"g2\"><td>");
			sprintf(temp,"%d",i+1);
			addstr(temp);
			addstr("</td><td>");
		    printf_oid(temp,ptr->varbind.oid_ptr);
			addstr(temp);
			addstr("</td><td>");
			if(ptr->attrs & FLAG_ACCESS_READONLY)
				addstrl("read-only","только чтение");
			else
				addstrl("read-write","чтение-запись");
			addstr("</td></tr>");
			i++;

			if(i%10 == 0){
				PSOCK_SEND_STR(&s->sout,str);
				str[0] = 0;
			}
		    mib_iterator_next();
		 }
		 addstr("</table></body></html>");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        PLC 														    	*/
/*******************************************************************************************************/
static PT_THREAD(run_plc(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	//static 	int i;
	static u8 apply;
	static u16 em;
	static u8 embr,emp,emdb,emsb;
	static u8 connect,connecting;
	static char id[PLC_EM_MAX_ID];
	static char pass[PLC_EM_MAX_PASS];

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
    if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
    	for(int i=0;i<pcount;i++){
			char *pname,*pval;

			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));



			if(!strcmp(pname, "em")){
				em = (u16)strtoul(pval, NULL, 10);
			}

			if(!strcmp(pname, "embr")){
				embr = (u8)strtoul(pval, NULL, 10);
			}

			if(!strcmp(pname, "emp")){
				emp = (u8)strtoul(pval, NULL, 10);
			}

			if(!strcmp(pname, "emdb")){
				emdb = (u8)strtoul(pval, NULL, 10);
			}

			if(!strcmp(pname, "emsb")){
				emsb = (u8)strtoul(pval, NULL, 10);
			}

			if(!strcmp(pname, "pass")){
				strcpy(pass,pval);
			}

			if(!strcmp(pname, "id")){
				strcpy(id,pval);
			}

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply = 1;
			}

			if(!strcmp(pname, "Set")){
				if (!strncmp(pval, "S", 1)) apply = 1;
			}

			if(!strcmp(pname, "Connect")){
				if (!strncmp(pval, "C", 1)) connect = 1;
			}
    	}

    	if(apply){
    		set_plc_em_model(em);
    		set_plc_em_rate(embr);
    		set_plc_em_parity(emp);
    		set_plc_em_databits(emdb);
    		set_plc_em_stopbits(emsb);
    		set_plc_em_pass(pass);
    		set_plc_em_id(id);
    		settings_save();
    		alertl("Parameters accepted","Настройки применились");
    		apply = 0;
    	}

    	//подключение к счетчику
    	if(connect){
    		if(get_plc_em_model()!=0){//если выбрана модель счетчика
				connecting = 1;//запускаем процесс подключения
				plc_em_start();
    		}
    		connect = 0;
    	}

    }




    str[0]=0;

	addhead();

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;

	addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/plc.shtml\">");
	addstr("<script type = \"text/javascript\">"
			  "var TimeoutID_;"
			  "var Time_;"
			  "var str;"
			  "var formid;"
			  "function form() {"
				"str+=\" .\";"
				"return(str);"
			  "}"
			  "function show(){"
				"document.getElementById(formid).innerHTML = '");
		addstrl("Connecting","Подключение");
		addstr("'+form();"
				"if(Time_<10)"
					"Time_++;"
				"window.setTimeout('show();',2000);"
			  "}"
			  "function start(id){ "
				"Time_=0;"
				"str='';"
				"formid=id;"
				"TimeoutID_=window.setTimeout('show();', 1000);"
			  "}"
			"</script>");


	//dot lines on connect button click
	if(connecting == 1){
		addstr("<script type=\"text/javascript\">"
					"start('status');"
				"</script>");
		addstr("<meta http-equiv=\"refresh\" content=\"20; url=\"/action=\"/plc.shtml\">");
		connecting = 0;
	}


	addstr("<body>");
	addstrl("<legend>RS485</legend>","<legend>RS485</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstr("Access denied");
	}else if(is_plc_connected()==0){
		addstrl("<b>PLC board is not connected!</b>","<b>Плата расширения не подключена!</b>");
	}
	else{

		addstrl("<b>RS485 Settings</b>","<b>Настройки RS485</b>");
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		//bitrate
		addstr("<tr class=\"g2\"><td>");
		addstrl("Baudrate","Скорость обмена");
		addstr("</td><td>");
		addstr("<select name=\"embr\" size=\"1\">");
		for(u8 i=0;i<7;i++){
			addstr("<option");
			if(get_plc_em_rate() == i){
				addstr(" selected");
			}
			addstr(" value=\"");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\">");
			switch(i){
				case 0: addstr("300");break;
				case 1: addstr("600");break;
				case 2: addstr("1200");break;
				case 3: addstr("2400");break;
				case 4: addstr("4800");break;
				case 5: addstr("9600");break;
				case 6: addstr("19200");break;
			}
			addstr("</option>");
		}
		addstr("</select></td></tr>");
		//parity
		addstr("<tr class=\"g2\"><td>");
		addstrl("Parity","Четность");
		addstr("</td><td>");
		addstr("<select name=\"emp\" size=\"1\">");
		addstr("<option");
		if(get_plc_em_parity() == 0){
			addstr(" selected");
		}
		addstr(" value=\"0\">");
		addstrl("Disable","Нет");
		addstr("</option>");
		addstr("<option");
		if(get_plc_em_parity() == 1){
			addstr(" selected");
		}
		addstr(" value=\"1\">");
		addstrl("Even","Чет");
		addstr("</option>");
		addstr("<option");
		if(get_plc_em_parity() == 2){
			addstr(" selected");
		}
		addstr(" value=\"2\">");
		addstrl("Odd","Нечет");
		addstr("</option></select></td></tr>");
		//data bits
		addstr("<tr class=\"g2\"><td>");
		addstrl("Data Bits","Биты данных");
		addstr("</td><td>");
		addstr("<select name=\"emdb\" size=\"1\">");
		for(u8 i=5;i<10;i++){
			addstr("<option");
			if(get_plc_em_databits() == i){
				addstr(" selected");
			}
			addstr(" value=\"");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\">");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("</option>");
		}

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("</select></td></tr>");
		//stop bits
		addstr("<tr class=\"g2\"><td>");
		addstrl("Stop Bits","Стоповые биты");
		addstr("</td><td>");
		addstr("<select name=\"emsb\" size=\"1\">");
		for(u8 i=1;i<3;i++){
			addstr("<option");
			if(get_plc_em_stopbits() == i){
				addstr(" selected");
			}
			addstr(" value=\"");
			sprintf(temp,"%d",i);
			addstr("\">");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("</option>");
		}
		addstr("</select></td></tr>");
		addstr("</table>"
		"<br>");


		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;


		addstrl("<b>Energy Meter Settings</b>","<b>Настройки cчетчика электроэнергии</b>");
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		//model
		addstr("<tr class=\"g2\"><td>");
		addstrl("Model","Модель");
		addstr("</td><td>");
		addstr("<select name=\"em\" size=\"1\">");
		for(u8 i=0;i<(EM_MODEL_NUM+1);i++){
			addstr("<option");
			if(get_plc_em_model() == i){
				addstr(" selected");
			}
			addstr(" value=\"");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\">");
			get_plc_em_model_name_list(i,temp);
			addstr(temp);
			addstr("</option>");
		}
		addstr("</select></td></tr>");
		//id
		addstr("<tr class=\"g2\"><td>");
		addstrl("Identification","Идентификатор устройства");
		addstr("</td><td>"
		"<input type=\"text\"name=\"id\" "
		"size=\"20\" maxlength=\"32\"value=\"");
		get_plc_em_id(temp);
		addstr(temp);
		addstr("\"></td></tr>"
		//pass
		"<tr class=\"g2\"><td>");
		addstrl("Password","Пароль доступа");
		addstr("</td><td>"
		"<input type=\"text\"name=\"pass\" "
		"size=\"10\" maxlength=\"16\"value=\"");
		get_plc_em_pass(temp);
		addstr(temp);
		addstr("\"></td></tr></table><br>");
		addstr("<input type=\"Submit\" name=\"Apply\" value=\"Apply\"><br><br><br>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstrl("<b>Energy Meter</b>","<b>Счетчик электроэнергии</b>");
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		//model
		addstr("<tr class=\"g2\"><td>");
		addstrl("Model","Модель");
		addstr("</td><td>");
		get_plc_em_model_name(temp);
		addstr(temp);
		addstr("</td></tr>");
		addstr("<tr class=\"g2\"><td>");
		addstrl("State","Состояние");
		addstr("</td><td>"
		"<span id = \"status\">");
		if(is_em_connected())
			addstrl("Connected","Подключен");
		else
			addstrl("Unconnected","Не подключен");
		addstr("</span></td></tr>");
		addstr("<tr class=\"g2\"><td>");
		addstrl("Total indications","Суммарные показания");
		addstr("</td><td>");
		if(is_em_connected()){
			get_em_total(temp);
			addstr(temp);
		}
		else
			addstr("-");
		addstr("</td></tr>");
		addstr("<tr class=\"g2\"><td>T1</td><td>");
		if(is_em_connected()){
			get_em_t1(temp);
			addstr(temp);
		}
		else
			addstr("-");
		addstr("</td></tr>");
		addstr("<tr class=\"g2\"><td>T2</td><td>");
		if(is_em_connected()){
			get_em_t2(temp);
			addstr(temp);
		}
		else
			addstr("-");
		addstr("</td></tr>");
		addstr("<tr class=\"g2\"><td>T3</td><td>");
		if(is_em_connected()){
			get_em_t3(temp);
			addstr(temp);
		}
		else
			addstr("-");
		addstr("</td></tr>");
		addstr("<tr class=\"g2\"><td>T4</td><td>");
		if(is_em_connected()){
			get_em_t4(temp);
			addstr(temp);
		}
		else
			addstr("-");
		addstr("</td></tr>"
		"</table><br>");

		addstr("<input type=\"Submit\" name=\"Connect\" value=\"Connect\">");

		addstr("</body></html>");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        PLC 														    	*/
/*******************************************************************************************************/
static PT_THREAD(run_make_em_json(struct httpd_state *s, char *ptr)){
	//plc_485_connect();
	//get_plc_em_indications();
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	addstr("{\"connected\":\"");
	if(is_em_connected())
		addstr("yes");
	else
		addstr("no");
	addstr("\",");
	addstr("\"model\":\"");
	get_plc_em_model_name_list(get_plc_em_model(),temp);
	addstr(temp);
	addstr("\",");
	addstr("\"total\":");
	if(is_em_connected()){
		get_em_total(temp);
		addstr(temp);
	}
	else
		addstr("0");
	addstr(",");
	addstr("\"t1\":");
	if(is_em_connected()){
		get_em_t1(temp);
		addstr(temp);
	}
	else
		addstr("0");
	addstr(",");
	addstr("\"t2\":");
	if(is_em_connected()){
		get_em_t2(temp);
		addstr(temp);
	}
	else
		addstr("0");
	addstr(",");
	addstr("\"t3\":");
	if(is_em_connected()){
		get_em_t3(temp);
		addstr(temp);
	}
	else
		addstr("0");
	addstr(",");
	addstr("\"t4\":");
	if(is_em_connected()){
		get_em_t4(temp);
		addstr(temp);
	}
	else
		addstr("0");
	addstr("}");


	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}




/**************************************************************************************/
/*                            EVENTS -> Access Control*/
/**************************************************************************************/

static PT_THREAD(run_access(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	static  uint16_t pcount;
	static  u8 apply_edit_ac=0,Read=0,i;
	//static char temp_rus[64];
	static u8 out_state[PLC_RELAY_OUT],out_reset[PLC_RELAY_OUT];
	static u8 out_event[PLC_RELAY_OUT][PLC_EVENTS];


		  PSOCK_BEGIN(&s->sout);
		  str[0]=0;

		  for(u8 i=0;i<PLC_RELAY_OUT;i++){
			for(u8 j=0;j<PLC_EVENTS;j++){
				//set_plc_out_event(i,j,0);
				out_event[i][j]=0;
			}
		  }

		  if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

			  int i=0;
			  for(i=0;i<pcount;i++){
		  		char *pname,*pval;
		  		pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		  		pval = http_get_parameter_value(s->param,i,sizeof(s->param));


		  		if(!strcmp(pname, "ApplyEdit")){
		  			if (!strncmp(pval, "A", 1)) apply_edit_ac = 1;
		  		}

		  		if(!strcmp(pname, "Refresh")){
		  			if (!strncmp(pval, "R", 1)) Read = 1;
		  		}

				if(!strcmp(pname, "state0")){
					  if (!strncmp(pval, "0", 1)) 	 set_alarm_state(0,0);
					  if (!strncmp(pval, "1", 1)) 	 set_alarm_state(0,1);
				}
				if(!strcmp(pname, "state1")){
					  if (!strncmp(pval, "0", 1)) 	 set_alarm_state(1,0);
					  if (!strncmp(pval, "1", 1)) 	 set_alarm_state(1,1);
				}
				if(!strcmp(pname, "state2")){
					  if (!strncmp(pval, "0", 1)) 	 set_alarm_state(2,0);
					  if (!strncmp(pval, "1", 1)) 	 set_alarm_state(2,1);
				}

				for(u8 i=0;i<NUM_ALARMS;i++){
					sprintf(temp,"edge%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "1", 1)) 	 set_alarm_front(i,1);
						if (!strncmp(pval, "2", 1)) 	 set_alarm_front(i,2);
						if (!strncmp(pval, "3", 1)) 	 set_alarm_front(i,3);
					}
				}




				for(u8 i=0;i<get_plc_output_num();i++){

					sprintf(temp,"out_state%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1)) out_state[i] = 0;//set_plc_out_state(i,0);
						if (!strncmp(pval, "1", 1)) out_state[i] = 1;//set_plc_out_state(i,1);
						if (!strncmp(pval, "2", 1)) out_state[i] = 2;//set_plc_out_state(i,2);
					}

					sprintf(temp,"reset%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "R", 1))
							out_reset[i] = 1;
					}

					for(u8 j=0;j<NUM_ALARMS;j++){
						sprintf(temp,"s_%d_%d",i,j);
						if(!strcmp(pname, temp)){
							if (!strncmp(pval, "1", 1))
								out_event[i][j] = 1;
						}
					}

					for(u8 j=0;j<get_plc_input_num();j++){
						sprintf(temp,"i_%d_%d",i,j);
						if(!strcmp(pname, temp))
							out_event[i][j+NUM_ALARMS] = 1;
					}

					sprintf(temp,"a_%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "1", 1))
							out_event[i][NUM_ALARMS+PLC_INPUTS]=1;
					}

					sprintf(temp,"action%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1))	set_plc_out_action(i,0);
						if (!strncmp(pval, "1", 1))	set_plc_out_action(i,1);
						if (!strncmp(pval, "2", 1))	set_plc_out_action(i,2);
					}
				}

				for(u8 i=0;i<get_plc_input_num();i++){
					sprintf(temp,"inp_state%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1)) 	 set_plc_in_state(i,0);
						if (!strncmp(pval, "1", 1)) 	 set_plc_in_state(i,1);
					}

					sprintf(temp,"inp_edge%d",i);
					if(!strcmp(pname, temp)){
						if (!strncmp(pval, "0", 1)) 	 set_plc_in_alarm_state(i,0);
						if (!strncmp(pval, "1", 1)) 	 set_plc_in_alarm_state(i,1);
						if (!strncmp(pval, "2", 1)) 	 set_plc_in_alarm_state(i,2);
					}
				}
		     }


			  if(apply_edit_ac==1){
				  	for(u8 i=0;i<get_plc_output_num();i++){
				  		if(out_state[i] == 0 || out_state[i] == 1)
				  			set_plc_relay_ee(i,out_state[i]);
				  		set_plc_out_state(i,out_state[i]);

				  		for(u8 j=0;j<PLC_EVENTS;j++){
				  			set_plc_out_event(i,j,out_event[i][j]);
						}
				  	}

					settings_save();
					alertl("Parameters accepted","Настройки применились");

					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
					apply_edit_ac=0;
			  }

			  //перезагрузка каналов
			  for(i=0;i<PLC_RELAY_OUT;i++){
				 if(out_reset[i]){
					 plc_relay_reset(i);
					 out_reset[i] = 0;
				 }
			  }
			  if(Read == 1){
				  //if(get_plc_hw_vers() == PLC_02)
				  //  dev.plc_status.new_event = 1;//получение состояний входа
				  Read=0;
			  }
		}

		//get out state
		for(u8 i=0;i<get_plc_output_num();i++){
			//my
			out_state[i] = get_plc_relay_ee(i);

		}
		//get current input state
		get_plc_inputs();


		addhead();
		addstr("<body>");
		addstrl("<legend>Inputs/Outputs</legend>","<legend>Входы/Выходы</legend>");
		if(get_dev_type() == DEV_SWU16){
			addstr("not active");
		}
		else if(get_current_user_rule()!=ADMIN_RULE){
			addstrl("Access denied","Доступ запрещён");
		}else{
			addstr( "<form method=\"post\" bgcolor=\"#808080\""
					  "action=\"/mngt/access.shtml\"><br>");

			addstr("<caption><b>");
			addstrl("Sensors (on main board)","Сенсоры (на главной плате)");
			addstr("<b></caption><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\">"
			"<td>&nbsp;");
			addstr("</td><td>");
			addstrl("State","Cостояние");
			addstr("</td><td>");
			addstrl("Alarm State","Аварийное состояние");
			addstr("</td><td>");
			addstrl("Current State","Текущее состояние");
			addstr("</td><td rowspan=\"");
			sprintf(temp,"%d",NUM_ALARMS+1);
			addstr(temp);
			addstr("\"><input type=\"Submit\" value=\"Refresh\"></td></tr>");


			//open sensor
			if((get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G6F)||(get_dev_type() == DEV_PSW2G8F)){
				addstr("<tr class=\"g2\"><td>");
				addstrl("Tamper","Датчик вскртия");
				addstr("</td><td>"
				"<select name=\"state0\" size=\"1\">"
				"<option");
				if(get_alarm_state(0)==0)addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				"<option ");
				if(get_alarm_state(0)==1)addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				"</select></td><td>"
				"<select disabled size=\"1\">"
				"<option selected >");
				addstrl("Opened","Разомкнутое");
				addstr("</option>"
				"</select></td><td>");
				if(get_sensor_state(0) == 0)
					addstrl("Alarm","Крышка вскрыта");
				else
					addstrl("Normal","Нормальное");
				addstr("</td></tr>");
			}

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

			for(i=1;i<NUM_ALARMS;i++){//начинаем с 1-го
				addstr("<tr class=\"g2\"><td>"
				"Sensor ");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("</td><td>"
				"<select name=\"state");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\" size=\"1\">"
				"<option");
				if(get_alarm_state(i)==0)
					addstr(" selected");
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>"
				"<option ");
				if(get_alarm_state(i)==1)
					addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option>"
				"</select></td><td>"

				"<select name=\"edge");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\" size=\"1\">"
				"<option");
				if(get_alarm_front(i)==1)
					addstr(" selected");
				addstr(" value=\"1\">");
				addstrl("Short","Замкнутое");
				addstr("</option>"
				"<option");
				if(get_alarm_front(i)==2)
					addstr(" selected");
				addstr(" value=\"2\">");
				addstrl("Open","Разомкнутое");
				addstr("</option>"
				"<option");
				if(get_alarm_front(i)==3)
					addstr(" selected");
				addstr(" value=\"3\">");
				addstrl("Any Change","Любое изменение");
				addstr("</option>"
				"</select></td><td>");
				if(get_sensor_state(i)==Bit_RESET)
					addstrl("Short", "Замкнутое");
				else
					addstrl("Open", "Разомкнутое");
				addstr("</td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;
			}
			addstr("</table>"
			"<br><br><br>");


			if(is_plc_connected()){
				if(get_plc_input_num()){
					addstr("<caption><b>");
					addstrl("Inputs (on option board)","Входы (на плате расширения)");
					addstr("<b></caption><table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
					"<tr class=\"g2\">"
					"<td>&nbsp;");
					addstr("</td><td>");
					addstrl("State","Cостояние");
					addstr("</td><td>");
					addstrl("Alarm State","Аварийное состояние");
					addstr("</td><td>");
					addstrl("Current State","Текущее состояние");
					addstr("</td><td rowspan=\"");
					sprintf(temp,"%d",get_plc_input_num()+1);
					addstr(temp);
					addstr("\"><input type=\"Submit\" value=\"Refresh\"></td></tr>");

					for(i=0;i<get_plc_input_num();i++){
						addstr("<tr class=\"g2\"><td>"
						"Input ");
						sprintf(temp,"%d",i+1);
						addstr(temp);
						addstr("</td><td>"
						"<select name=\"inp_state");
						sprintf(temp,"%d",i);
						addstr(temp);
						addstr("\" size=\"1\">"
						"<option");
						if(get_plc_in_state(i)==0)
							addstr(" selected");
						addstr(" value=\"0\">");
						addstrl("Disable","Выключено");
						addstr("</option>"
						"<option ");
						if(get_plc_in_state(i)==1)
							addstr(" selected");
						addstr(" value=\"1\">");
						addstrl("Enable","Включено");
						addstr("</option>"
						"</select></td><td>"
						"<select name=\"inp_edge");
						sprintf(temp,"%d",i);
						addstr(temp);
						addstr("\" size=\"1\">"
						"<option");
						if(get_plc_in_alarm_state(i)==PLC_ON_SHORT)
							addstr(" selected");
						addstr(" value=\"0\">");
						addstrl("Short","Замкнутое");
						addstr("</option>"
						"<option");
						if(get_plc_in_alarm_state(i)==PLC_ON_OPEN)
							addstr(" selected");
						addstr(" value=\"1\">");
						addstrl("Open","Разомкнутое");
						addstr("</option>"
						"<option");
						if(get_plc_in_alarm_state(i)==PLC_ANY_CHANGE)
							addstr(" selected");
						addstr(" value=\"2\">");
						addstrl("Any Change","Любое изменение");
						addstr("</option>"
						"</select></td><td>");
						if(dev.plc_status.in_state[i] == PLC_ON_OPEN)
							addstrl("Open", "Разомкнутое");
						else
							addstrl("Short", "Замкнутое");
						addstr("</td></tr>");
					}
					addstr("</table>"
					"<br><br><br>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}


				addstrl("<b>Outputs</b>","<b>Выходы</b>");
				addstr("<br>");
				addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
				addstr("<tr class=\"g2\"><td>");
				addstrl("Output","Выход");
				addstr("</td><td>");
				addstrl("Inital State","Начальное состояние");
				addstr("</td><td>");
				addstrl("Current State","Текущее состояние");
				addstr("</td>");
				if(get_plc_hw_vers() == PLC_01){
					addstr("<td>");
					addstrl("Manual Restart","Ручная перезагрузка");
					addstr("</td>");
				}
				addstr("</tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				for(i=0;i<get_plc_output_num();i++){
					addstr("<tr class=\"g2\"><td>");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("</td>"
					"<td>");
					addstr("<select name=\"");
					sprintf(temp,"out_state%d",i);
					addstr(temp);
					addstr("\" size=\"1\">"
					"<option");

					if(get_plc_out_state(i)==PLC_ACTION_SHORT){
						addstr(" selected");
					}
					addstr(" value=\"0\">");
					addstrl("Short","Замкнуто");
					addstr("</option>"
					"<option");
					if(get_plc_out_state(i)==PLC_ACTION_OPEN){
						addstr(" selected");
					}
					addstr(" value=\"1\">");
					addstrl("Open","Разомкнуто");
					addstr("</option>"

					"<option");

					if(get_plc_out_state(i) == LOGIC){
						addstr(" selected");
					}
					addstr(" value=\"2\">");
					addstrl("Logic","Логика");
					addstr("</option>"
					"</select></td>"
					"<td>");
					if(get_plc_relay(i)==PLC_ACTION_OPEN){
						addstrl("Open","Разомкнуто");
					}else{
						addstrl("Short","Замкнуто");
					}
					addstr("</td>");


					if(get_plc_hw_vers() == PLC_01){
						addstr("<td><input type=\"Submit\" name=\"reset");
						sprintf(temp,"%d",i);
						addstr(temp);
						addstr("\" value=\"Restart\"></td>");
					}
					addstr("</tr>");
				}
				addstr("</table><br><br><br>");
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;

				//ЛОГИКА
				addstrl("<b>Logic</b>","<b>Логика</b>");
				addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
				addstr("<tr class=\"g2\"><td>");
				addstrl("Output","Выход");
				addstr("</td><td>");
				addstrl("Event","Событие");
				addstr("</td><td>");
				addstrl("Action","Действие");
				addstr("</td></tr><tr>");
				for(i=0;i<get_plc_output_num();i++){
					addstr("<tr class=\"g2\"><td>");
					sprintf(temp,"%d",i+1);
					addstr(temp);
					addstr("</td><td>");
					//event
					addstr("<table>");
					for(u8 j=0;j<NUM_ALARMS;j++){
						if((get_dev_type() != DEV_PSW2GPLUS)&&(get_dev_type() != DEV_PSW2G6F)&&(get_dev_type() != DEV_PSW2G8F) &&(j==0)){
							continue;
						}
						addstr("<tr class=\"g2\"><td>"
						"<input type=checkbox name=\"");
						sprintf(temp,"s_%d_%d",i,j);
						addstr(temp);
						addstr("\" value=\"1\"");
						if(get_plc_out_event(i,j)==1)
							addstr(" checked ");
						addstr(">"
						"</td><td>");
						if((get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G6F)||(get_dev_type() == DEV_PSW2G8F)){
							addstrl("Tamper","Датчик вскрытия");
						}
						else{
							addstrl("Sensor ","Сенсор ");
							sprintf(temp,"%d",j);
							addstr(temp);
						}
						addstr("</td></tr>");
					}
					for(u8 j=0;j<get_plc_input_num();j++){
						addstr("<tr class=\"g2\"><td>"
						"<input type=checkbox name=\"");
						sprintf(temp,"i_%d_%d",i,j);
						addstr(temp);
						addstr("\" value=\"1\"");
						if(get_plc_out_event(i,j+NUM_ALARMS)==1){
							addstr(" checked ");
						}
						addstr(">"
						"</td><td>");
						addstrl("Input ","Вход ");
						sprintf(temp,"%d",j+1);
						addstr(temp);
						addstr("</td></tr>");
					}
					addstr("<tr class=\"g2\"><td>"
					"<input type=checkbox name=\"");
					sprintf(temp,"a_%d",i);
					addstr(temp);
					addstr("\" value=\"1\"");
					if(get_plc_out_event(i,NUM_ALARMS+PLC_INPUTS)==1){
						addstr(" checked ");
					}
					addstr(">"
					"</td><td>");
					addstrl("System Alarm","Системные события");
					addstr("</td></tr>");

					addstr("</table></td>");
					//action
					addstr("<td>");
					addstr("<select name=\"");
					sprintf(temp,"action%d",i);
					addstr(temp);
					addstr("\" size=\"1\">"
					"<option");
					if(get_plc_out_action(i)==PLC_ACTION_SHORT){
						addstr(" selected");
					}
					addstr(" value=\"0\">");
					addstrl("Short","Замкнуть");
					addstr("</option>"
					"<option");
					if(get_plc_out_action(i)==PLC_ACTION_OPEN){
						addstr(" selected");
					}
					addstr(" value=\"1\">");
					addstrl("Open","Разомкнуть");
					addstr("</option>"
					"<option");
					if(get_plc_out_action(i)==PLC_ACTION_IMPULSE){
						addstr(" selected");
					}
					addstr(" value=\"2\">");
					addstrl("Impulse","Импульс");
					addstr("</option>"
					"</select></td></tr>");
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}
				addstr("</table>");
			}

			addstr("<br><input type=\"Submit\" name=\"ApplyEdit\" value=\"Apply\">"
				"</form></body></html>");
		}
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        MAC Address Bind 														    				   */
/*******************************************************************************************************/
static PT_THREAD(run_mac_bind(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static 	int i;
	static u8 apply,lst[PORT_NUM],mac[6],port,delete,edit, entry_num,add;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
	    for(int i=0;i<pcount;i++){
			char *pname,*pval;

			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			for(u8 j=0;j<ALL_PORT_NUM;j++){
				sprintf(temp,"state%d",j);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1)) lst[j] = PORT_NORMAL;
					if (!strncmp(pval, "1", 1)) lst[j] = MAC_FILT;
					if (!strncmp(pval, "2", 1)) lst[j] = PORT_FILT;
					if (!strncmp(pval, "3", 1)) lst[j] = PORT_FILT_TEMP;
				}
			}

			for(u8 j=0;j<6;j++){
				sprintf(temp,"mac%d",j);
				if(!strcmp(pname, temp)){
					mac[j] = (u8)strtoul(pval, NULL, 16);
				}
			}

			if(!strcmp(pname, "port")){
				port = (u8)strtoul(pval, NULL, 10);
			}

			for(u8 j=0;j<MAC_BIND_MAX_ENTRIES;j++){
				sprintf(temp,"Del%d",j);
				if(!strcmp(pname, temp)){
					delete = 1;
					entry_num = j;
				}
			}

			for(u8 j=0;j<MAC_BIND_MAX_ENTRIES;j++){
				sprintf(temp,"Edit%d",j);
				if(!strcmp(pname, temp)){
					edit = 1;
					entry_num = j;
				}
			}

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply = 1;
			}

			if(!strcmp(pname, "Add")){
				if (!strncmp(pval, "A", 1)) add = 1;
			}

			if(!strcmp(pname, "Set")){
				if (!strncmp(pval, "S", 1)) add = 1;
			}
    	}

    	if(apply){
    		for(i=0;i<ALL_PORT_NUM;i++){
    			set_mac_filter_state(i,lst[i]);
      		}
    		//set_mac_learn_cpu(state_cpu);
    		settings_add2queue(SQ_MACFILR);
       		settings_save();
    		//alertl("Parameters accepted","Настройки применились");
    		alertl("Parameters accepted, reboot...","Настройки применились, перезагрузка...");
			reboot(REBOOT_MCU_5S);
    		apply = 0;
   	    }

		if(add){
			if(edit){
				del_mac_bind_entry(entry_num);
				add_mac_bind_entry(mac,port);
				edit = 0;
				add = 0;
				entry_num = 0;
			}
			else
				add_mac_bind_entry(mac,port);
    		add = 0;
    	}

		if(delete){
    		del_mac_bind_entry(entry_num);
    		entry_num = 0;
    		delete = 0;
		}




	}


    str[0]=0;

	addhead();


	addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/settings/mac_bind.shtml\">");

	addstr("<body>");
	addstrl("<legend>MAC Address Filtering</legend>","<legend>MAC Address Filtering</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstr("Access denied");
	}else if(get_dev_type() == DEV_SWU16){
		addstrl("Not Implemented", "Данная функция не реализована");
	}else{
		addstrl("<b>Ports Settings</b>","<b>Настройки портов</b>");
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		addstr("<tr class=\"g2\"><td>");
		addstrl("Port","Порт");
		addstr("</td><td>");
		addstrl("State","Состояние");
		addstr("</td></tr>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		for(i=0;i<ALL_PORT_NUM;i++){
			addstr("<tr class=\"g2\"><td>");
			sprintf(temp,"%d",i+1);
			addstr(temp);
			addstr("</td><td><select name=\"state");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\" size=\"1\">");
			//normal
			addstr("<option");
			if(get_mac_filter_state(i) == PORT_NORMAL){
				addstr(" selected");
			}
			addstr(" value=\"0\">");
			addstrl("Normal","Обычный");
			addstr("</option>");

			//mac filtering
			addstr("<option");
			if(get_mac_filter_state(i) == MAC_FILT){
				addstr(" selected");
			}
			addstr(" value=\"1\">");
			addstrl("Secure: MAC filtration","Защищённый: Фильтрация МАС адреса");
			addstr("</option>");

			//port shutdown
			addstr("<option");
			if(get_mac_filter_state(i) == PORT_FILT){
				addstr(" selected");
			}
			addstr(" value=\"2\">");
			addstrl("Secure: Port shutdown","Защищённый: Отключение порта");
			addstr("</option>");

			//temp port shutdown
			addstr("<option");
			if(get_mac_filter_state(i) == PORT_FILT_TEMP){
				addstr(" selected");
			}
			addstr(" value=\"3\">");
			addstrl("Secure: Temporary port shutdown","Защищённый: Временное отключение порта");
			addstr("</option></select></td></tr>");

			if(i%5 == 0){
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;
			}
		}

		addstr("</table><br><br>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		if(get_mac_bind_num()< MAC_BIND_MAX_ENTRIES){

			if(edit)
				addstrl("<b>Edit MAC Address</b>","<b>Редактировать MAC адрес</b>");
			else
				addstrl("<b>Add New MAC Address</b>","<b>Добавить новый разрешённый MAC адрес</b>");
			addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
			addstr("<tr class=\"g2\"><td>"
			"MAC"
			"</td><td>");
			addstrl("Port","Порт");
			addstr("</td><td></td></tr>");

			addstr("<tr class=\"g2\"><td>");
			for(uint8_t i=0;i<6;i++){
				addstr("<input type=\"text\"name=\"mac");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\" size=\"2\" maxlength=\"2\"value=\"");
				if(edit){
					sprintf(temp,"%X",get_mac_bind_entry_mac(entry_num,i));
					addstr(temp);
				}
				addstr("\">");
			}
			addstr("</td><td>"
			"<select name=\"port\" size=\"1\">");
			for(i=0;i<ALL_PORT_NUM;i++){
				addstr("<option value=\"");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\"");
				if(edit){
					if(i == get_mac_bind_entry_port(entry_num))
						addstr(" selected ");
				}
				addstr(">");
				addstrl("Port ","Порт ");
				sprintf(temp,"%d",i+1);
				addstr(temp);
				addstr("</option>");
			}
			addstr("</select></td><td>");
			if(edit)
				addstr("<input type=\"Submit\" name=\"Set\" value=\"Set\">");
			else
				addstr("<input type=\"Submit\" name=\"Add\" value=\"Add\">");

			addstr("</td></tr></table><br><br>");
		}


		addstrl("<b>MAC Address Table</b>","<b>Таблица MAC адресов</b>");
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		addstr("<tr class=\"g2\"><td><td>");
		addstrl("MAC Address","MAC адрес");
		addstr("</td><td>");
		addstrl("Port","Порт");
		addstr("</td><td></td></tr>");
		for(i=0;i<get_mac_bind_num();i++){
			addstr("<tr class=\"g2\"><td>");
			sprintf(temp,"%d",i+1);
			addstr(temp);
			addstr("</td><td>");
			sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",get_mac_bind_entry_mac(i,0),
					get_mac_bind_entry_mac(i,1),get_mac_bind_entry_mac(i,2),
					get_mac_bind_entry_mac(i,3),get_mac_bind_entry_mac(i,4),
					get_mac_bind_entry_mac(i,5));
			addstr(temp);
			addstr("</td><td>");
			sprintf(temp,"%d",get_mac_bind_entry_port(i)+1);
			addstr(temp);
			addstr("</td><td><input type=\"Submit\" name=\"Del");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\" value=\"");
			addstrl("Delete","Удалить");
			addstr("\">"
			"<input type=\"Submit\" name=\"Edit");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\" value=\"");
			addstrl("Edit","Редактировать");
			addstr("\"></td></tr>");

			if(i%9==0){
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;
			}
		}

		addstr("</table>"
		"<br><br><input type=\"Submit\" name=\"Apply\" value=\"Apply\">"
		"</form>");

	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*      Blocked MAC Address 														    				   */
/*******************************************************************************************************/
static PT_THREAD(run_mac_blocked(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static 	u8 i,j;
	static u8 apply;//,mac[6];//,port;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
	    for(int i=0;i<pcount;i++){
			char *pname,*pval;
			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply = 1;
			}
	    }
    	if(apply){
    		apply = 0;
   	    }
	}

    str[0]=0;
	addhead();
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/settings/mac_blocked.shtml\">");
	addstr("<body>");
	addstrl("<legend>Blocked MAC Address</legend>","<legend>Заблокированные МАС адреса</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstr("Access denied");
	}else if(get_dev_type() == DEV_SWU16){
		addstrl("Not Implemented", "Данная функция не реализована");
	}else{
		addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
		addstr("<tr class=\"g2\"><td></td><td>");
		addstrl("Port","Порт");
		addstr("</td><td>");
		addstrl("MAC address","МАС адрес");
		addstr("</td></tr>");
		j=0;
		for(i=0;i<MAX_BLOCKED_MAC;i++){
			if(dev.mac_blocked[i].age_time){
				addstr("<tr class=\"g2\"><td>");
				sprintf(temp,"%d",j+1);
				j++;
				addstr(temp);
				addstr("</td><td>");
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(dev.mac_blocked[i].port){
						case 0:addstr("FE#1 ");break;
						case 1:addstr("FE#2 ");break;
						case 2:addstr("FE#3 ");break;
						case 3:addstr("FE#4 ");break;
						case 4:addstr("GE#1 ");break;
						case 5:addstr("GE#2 ");break;

					}
				}else{
					sprintf(temp,"%d ",dev.mac_blocked[i].port+1);
					addstr(temp);
				}
				addstr("</td><td>");
				sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",dev.mac_blocked[i].mac[0],dev.mac_blocked[i].mac[1],
						dev.mac_blocked[i].mac[2],dev.mac_blocked[i].mac[3],dev.mac_blocked[i].mac[4],
						dev.mac_blocked[i].mac[5]);
				addstr(temp);
				addstr("</td></tr>");
				if(i%10==0){
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}
			}
		}
		addstr("</table>"
		"<br><input type=\"Submit\" name=\"Refresh\" value=\"Refresh\"><br><br><br>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("</form>");

	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}




/*******************************************************************************************************/
/*        UPS additional settings 														    				   */
/*******************************************************************************************************/
static PT_THREAD(run_ups(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	//static 	int i;
	static u8 apply, delay_start;
	u16 tmp_voltage;
	static u8 del_st,ups_vers;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
	    for(int i=0;i<pcount;i++){
			char *pname,*pval;

			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "delay_start")){
				if (!strncmp(pval, "0", 1)) delay_start = 0;
				if (!strncmp(pval, "1", 1)) delay_start = 1;
			}

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply = 1;
			}

    	}

    	if(apply){
    		//set_ups_delayed_start(delay_start);
    		//settings_save();

    		if(set_ups_delay_start(delay_start)==0){
    			alertl("Parameters accepted","Настройки применились");
    		}
    		else{
    			alertl("Parameters not accepted!","Настройки не применились!");
    		}
    		apply = 0;

   	    }
	}


	if(dev.ups_status.hw_vers>=VERS_11)
		del_st = get_ups_delay_start();


    str[0]=0;

	addhead();

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;



	addstr("<body>");
	addstrl("<legend>UPS</legend>","<legend>ИБП</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstr("Access denied");
	}else{
		if(is_ups_mode() == ENABLE){
			addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/ups.shtml\">");

			addstr("<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">");
			addstr("<tr class=\"g2\"><td>");
			addstrl("UPS","ИБП");
			addstr("</td><td>Enable</td></tr>");

			addstr("<tr class=\"g2\"><td>");
			addstrl("Battery voltage","Напряжение на АКБ");
			addstr("</td><td>");
			if(is_akb_detect()==1){
				 tmp_voltage = get_akb_voltage();
			 	 if(tmp_voltage<360){
					addstr("&nbsp;");
				 }
				 else{
					sprintf(temp,"%d.%dV",tmp_voltage/10,tmp_voltage%10);
					addstr(temp);
				 }
			}else{
				addstrl("Battery not connected","АКБ не подключена");
			}
			addstr("</td>");
			addstr("</tr>");

			addstr("<tr class=\"g2\"><td>");
			addstrl("Power source","Питание");
			addstr("</td><td>");
			if(is_ups_rezerv()==0){
				addstrl("VAC","Сетевое напряжение");
			}
			else{
				addstrl("Battery","АКБ");
			}
			addstr("</td>");
			addstr("</tr>");

			addstr("<tr class=\"g2\"><td>");
			addstrl("Estimated battery time","Оценочное время работы на АКБ");
			addstr("</td><td>");
			if(remtime.valid){
				sprintf(temp,"%dh.%dm",remtime.hour,remtime.min);
				addstr(temp);
			}
			else{
				addstr("&nbsp;");
			}
			addstr("</td>");
			addstr("</tr>");


			if(ups_vers>=VERS_11){
				addstr("<tr class=\"g2\"><td>");
				addstrl("Delayed start","Отложенный запуск");
				addstr("</td><td><select name=\"state\" size=\"1\">");
				addstr("<option");
				if(del_st == DISABLE){
					addstr(" selected");
				}
				addstr(" value=\"0\">");
				addstrl("Disable","Выключено");
				addstr("</option>");
				addstr("<option");
				if(del_st == ENABLE){
					addstr(" selected");
				}
				addstr(" value=\"1\">");
				addstrl("Enable","Включено");
				addstr("</option></select></td>"
				"</tr>");
			}
			addstr("</table>");
			if(dev.ups_status.hw_vers>=VERS_11){
				addstr("<br><input type=\"Submit\" name=\"Apply\" value=\"Apply\"><br><br><br>");
			}
			else{
				addstr("<br><input type=\"Submit\" name=\"Refresh\" value=\"Refresh\"><br><br><br>");
			}
			addstr("</form>");
		}
		else{
			addstrl("UPS is not connected","Модуль ИБП не подключен");
		}
	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*        Link Aggregation 														    				   */
/*******************************************************************************************************/
static PT_THREAD(run_aggregation(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static int i;
	static u8 apply;
	static u8 edit,delete,index,port[PORT_NUM];
	static u8 add_trunk,set_trunk,master,state;
	static u8 ok;


	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		for(u8 j=0;j<ALL_PORT_NUM;j++){
			port[j] = 0;
		}
	    for(int i=0;i<pcount;i++){
			char *pname,*pval;

			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

			if(!strcmp(pname, "Set")){
				if (!strncmp(pval, "S", 1)) set_trunk = 1;
			}

			if(!strcmp(pname, "Add")){
				if (!strncmp(pval, "A", 1)) add_trunk = 1;
			}

			for(u8 j=0;j<LAG_MAX_ENTRIES;j++){
				sprintf(temp,"edit%d",j);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "E", 1)){
						edit = 1;
						index = j;
					}
				}
				sprintf(temp,"del%d",j);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "D", 1)){
						delete = 1;
						index = j;
					}
				}
			}

			if(!strcmp(pname, "id")){
				if(set_trunk || add_trunk)
					index = (u8)strtoul(pval,NULL,10)-1;
			}

			if(!strcmp(pname, "state")){
				if (!strncmp(pval, "0", 1)) state = 0;
				if (!strncmp(pval, "1", 1)) state = 1;
			}

			for(u8 j=0;j<ALL_PORT_NUM;j++){
				sprintf(temp,"port%d",j);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "1", 1)){
						port[j] = 1;
					}
				}
			}

			if(!strcmp(pname, "master")){
				for(u8 j=0;j<ALL_PORT_NUM;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp)){
						master = j;
					}
				}
			}

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply = 1;
			}

    	}


		if(delete){
			del_lag_entry(index);
			delete = 0;
		}

		if(add_trunk || set_trunk){
			for(i=0,ok=0;i<ALL_PORT_NUM;i++){
				if(i == master)
					ok = 1;
			}
			if(ok==0){
				alertl("Bad Parameter: Master port is not in members ports","Неверный параметр: Мастер порт не принадлежит группе портов");
				apply = 0;
				add_trunk = 0;
				set_trunk = 0;
			}
			ok = 1;
			/*for(u8 k=0;k<LAG_MAX_ENTRIES;k++){
				if(state && get_lag_valid(k) && get_lag_state(k)){
					for(u8 j=0;j<ALL_PORT_NUM;j++){
						if(get_lag_port(k,j) == port[j])
							ok = 0;
					}
				}
			}

			if(ok==0){
				alertl("Bad Parameter: Port exists in several trunks","Неверный параметр: Порт существует в нескольких группах");
				apply = 0;
				add_trunk = 0;
				set_trunk = 0;
			}*/

			if(ok == 1) {
				if(add_trunk)
					index = get_lag_entries_num();
				set_lag_valid(index,1);
				set_lag_state(index,state);
				set_lag_master_port(index,master);
				for(i=0;i<ALL_PORT_NUM;i++){
					set_lag_port(index,i,port[i]);
				}
				add_trunk = 0;
				set_trunk = 0;
				edit = 0;
			}
		}

		if(apply){
			edit = 0;
			delete = 0;
			add_trunk = 0;
			set_trunk = 0;
			edit = 0;
			apply = 0;
			settings_add2queue(SQ_PORT_ALL);
			settings_save();
   			alertl("Parameters accepted","Настройки применились");
		}
	}

    str[0]=0;
	addhead();
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;


	addstr("<body>");
	addstrl("<legend>Link Aggregation</legend>","<legend>Агрегирование портов</legend>");
	if(get_current_user_rule()!=ADMIN_RULE || get_dev_type()!=DEV_SWU16){
		addstr("Access denied");
	}else if(get_dev_type() == DEV_SWU16){
		addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/settings/aggregation.shtml\">");
		if(edit){
			addstrl("<b>Edit Trunk</b>","<b>Редактировать транк</b>");
		}else if(edit == 0 && get_lag_entries_num() < LAG_MAX_ENTRIES){
			addstrl("<b>Add Trunk</b>","<b>Добавить транк</b>");
		}
		if(edit || (edit==0 && get_lag_entries_num() < (LAG_MAX_ENTRIES-1))){
			addstr("<br>");
			addstr("<table cellspacing=\"2\" cellpadding=\"6\" class=\"g1\">");
			addstr("<tr class=\"g2\">");
			addstr("<td>ID</td><td>");
			addstrl("State","Состояние");
			addstr("</td><td>");
			addstrl("Master Port","Мастер порт");
			addstr("</td>");
			for(u8 i=0;i<ALL_PORT_NUM;i++){
				sprintf(temp,"<td>%d</td>",i+1);
				addstr(temp);
			}
			addstr("</tr>");
			addstr("<tr class=\"g2\"><td>");
			/*"<input type=\"text\" name=\"id\"size=\"4\"maxlength=\"4\"value=\"");*/
			if(edit == 1){
				sprintf(temp,"%d",index+1);
				addstr(temp);
			}
			else if(edit == 0){
				sprintf(temp,"%d",get_lag_entries_num()+1);
				addstr(temp);
			}
			//addstr("\"></td>"
			addstr("</td>"
			"<td><select name= \"state\" size=\"1\"><option");
			if(get_lag_state(index)==0 && edit)
				addstr(" selected");
			addstr(" value=\"0\">");
			addstrl("Disabled","Выключено");
			addstr("</option>"
			 "<option");
			if(get_lag_state(index)==1 && edit)
				addstr(" selected");
			addstr(" value=\"1\">");
			addstrl("Enabled","Включено");
			addstr("</option>"
			"</select></td>"

			"<td><select name= \"master\" size=\"1\">");
			for(i=0;i<ALL_PORT_NUM;i++){
				addstr("<option");
				if(get_lag_master_port(index)==i && edit)
					addstr(" selected");
				addstr(" value=\"");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\">");
				sprintf(temp,"%d",i+1);
				addstr(temp);
				addstr("</option>");
			}
			addstr("</td>");

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

			for(i=0;i<ALL_PORT_NUM;i++){
				addstr("<td><input type=checkbox name=\"port");
				sprintf(temp,"%d",(i));
				addstr(temp);
				addstr("\" value=\"1\"");
				if (get_lag_port(index,i) && edit)
					addstr( " checked ");
				addstr("></td>");
			}
			addstr("</tr></table>");

			if(edit)
				addstr("<input type=\"Submit\" name=\"Set\" value=\"Set\">");
			else
				addstr("<input type=\"Submit\" name=\"Add\" value=\"Add\">");

			addstr("<br><br><br>");
		}

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstrl("<b>Trunk List</b>","<b>Список транков</b>");
		addstr("<table cellspacing=\"2\" cellpadding=\"6\" class=\"g1\">");
		addstr("<tr class=\"g2\"><td>Trunk ID</td><td>");
		addstrl("State","Состояние");
		addstr("</td><td>");
		addstrl("Master Port","Мастер порт");
		addstr("</td><td>");
		addstrl("Ports","Порты");
		addstr("</td><td></td></tr>");

		for(i=0;i<LAG_MAX_ENTRIES;i++){
			if(get_lag_valid(i)){
				addstr("<tr class=\"g2\"><td>");
				sprintf(temp,"%d",i+1);
				addstr(temp);
				addstr("</td><td>");
				if(get_lag_state(i)){
					addstrl("Enabled","Включено");
				}
				else{
					addstrl("Disabled","Выключено");
				}
				addstr("</td><td>");
				sprintf(temp,"%d",get_lag_master_port(i)+1);
				addstr(temp);
				addstr("</td><td>");

				for(u8 j=0;j<ALL_PORT_NUM;j++){
					if(get_lag_port(i,j)){
						sprintf(temp,"%d ",j+1);
						addstr(temp);
					}
				}
				addstr("</td><td>"
				"<input type=\"Submit\" ");
				addstr("name=\"edit");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\" value=\"Edit\">"
				"<input type=\"Submit\" ");
				addstr("name=\"del");
				sprintf(temp,"%d",i);
				addstr(temp);
				addstr("\" value=\"Delete\"></td></tr>");

				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;
			}
		}
		if(get_lag_entries_num()==0){
			addstr("<tr class=\"g2\"><td></td><td></td><td></td><td></td><td></td></tr>");
		}
		addstr("</table>"
		"<br><br><input type=\"Submit\" ");
		addstr("name=\"Apply\" value=\"Apply\">");
		addstr("</form>");
	}
	addstr("</body>");
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


/*******************************************************************************************************/
/*        PortMirroring													    				   */
/*******************************************************************************************************/
static PT_THREAD(run_mirror(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static int i;
	static u8 apply;
	static u8 port[PORT_NUM];
	static u8 target_port,state;
	static u8 ok;


	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
	    for(int i=0;i<pcount;i++){
			char *pname,*pval;

			pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));


			if(!strcmp(pname, "state")){
				if (!strncmp(pval, "0", 1)) state = 0;
				if (!strncmp(pval, "1", 1)) state = 1;
			}

			for(u8 j=0;j<ALL_PORT_NUM;j++){
				sprintf(temp,"port%d",j);
				if(!strcmp(pname, temp)){
					if (!strncmp(pval, "0", 1))	port[j] = 0;
					if (!strncmp(pval, "1", 1))	port[j] = 1;
					if (!strncmp(pval, "2", 1))	port[j] = 2;
					if (!strncmp(pval, "3", 1))	port[j] = 3;
				}
			}

			if(!strcmp(pname, "target")){
				for(u8 j=0;j<PORT_NUM;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp)){
						target_port = j;
					}
				}
			}

			if(!strcmp(pname, "Apply")){
				if (!strncmp(pval, "A", 1)) apply = 1;
			}

    	}


		if(apply){
			for(i=0,ok=0;i<ALL_PORT_NUM;i++){
				if(port[i]){
					if(i!= target_port)
						ok++;
				}
			}

			if(ok==0 && state){
				alertl("Bad Parameter: ports","Неверный параметр: порты");
				apply = 0;
			}
			else{
				set_mirror_state(state);
				set_mirror_target_port(target_port);
				for(i=0;i<ALL_PORT_NUM;i++){
					set_mirror_port(i,port[i]);
				}
				settings_add2queue(SQ_PORT_ALL);
				settings_save();
				apply = 0;
				alertl("Parameters accepted","Настройки применились");
			}
		}
	}



    str[0]=0;

	addhead();

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;



	addstr("<body>");
	addstrl("<legend>Port Mirroring</legend>","<legend>Зеркалирование портов</legend>");
	if(get_current_user_rule()!=ADMIN_RULE || get_dev_type()!=DEV_SWU16){
		addstr("Access denied");
	}else if(get_dev_type() == DEV_SWU16){
		addstr("<form method=\"post\" bgcolor=\"#808080\" action=\"/settings/mirror.shtml\">");

		addstr("<table cellspacing=\"2\" cellpadding=\"6\" class=\"g1\">");
		addstr("<tr class=\"g2\"><td>");
		addstrl("State","Состояние");
		addstr("</td><td><select name= \"state\" size=\"1\"><option");
		if(get_mirror_state()==0)
			addstr(" selected");
		addstr(" value=\"0\">");
		addstrl("Disabled","Выключено");
		addstr("</option>"
		 "<option");
		if(get_mirror_state()==1)
			addstr(" selected");
		addstr(" value=\"1\">");
		addstrl("Enabled","Включено");
		addstr("</option>"
		"</select></td></tr></table><br><br>");


		addstr("<table cellspacing=\"2\" cellpadding=\"6\" class=\"g1\">");
		addstr("<tr class=\"g2\"><td rowspan=\"2\">");
		addstrl("Target Port","Целевой порт");
		addstr("</td>");
		addstr("<td colspan=\"");
		sprintf(temp,"%d",ALL_PORT_NUM);
		addstr(temp);
		addstr("\">");
		addstrl("Source Ports","Порты-источники");
		addstr("</td></tr>");
		addstr("<tr class=\"g2\">");
		for(i=0;i<ALL_PORT_NUM;i++){
			sprintf(temp,"<td>%d</td>",i+1);
			addstr(temp);
		}
		addstr("</tr>");
		addstr("<tr class=\"g2\">"
		"<td><select name= \"target\" size=\"1\">");
		for(i=0;i<ALL_PORT_NUM;i++){
			addstr("<option");
			if(get_mirror_target_port()==i)
				addstr(" selected");
			addstr(" value=\"");
			sprintf(temp,"%d",i);
			addstr(temp);
			addstr("\">");
			sprintf(temp,"%d",i+1);
			addstr(temp);
			addstr("</option>");
		}
		addstr("</td>");
		for(i=0;i<ALL_PORT_NUM;i++){
			addstr("<td><select name= \"port");
			sprintf(temp,"%d",(i));
			addstr(temp);
			addstr("\" size=\"1\">"
			"<option");
			if(get_mirror_port(i)==0)
				addstr(" selected");
			addstr(" value=\"0\">N"
			"</option>"
			"<option");
			if(get_mirror_port(i)==1)
				addstr(" selected");
			addstr(" value=\"1\">R"
			"</option>"
			"<option");
			if(get_mirror_port(i)==2)
				addstr(" selected");
			addstr(" value=\"2\">T"
			"</option>"
			"<option");
			if(get_mirror_port(i)==3)
				addstr(" selected");
			addstr(" value=\"3\">B"
			"</option>"
			"</td>");

			if(i%5==0){
				PSOCK_SEND_STR(&s->sout,str);
				str[0]=0;
			}
		}
		addstr("</tr></table>"
		"<br><br><input type=\"Submit\" ");
		addstr("name=\"Apply\" value=\"Apply\">");
		addstr("</form>");
	}
	addstr("</body>");
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}



static PT_THREAD(run_teleport(struct httpd_state *s, char *ptr)){
	//NOTE:local variables are not preserved during the calls to proto socket functins
	static  uint16_t pcount;
	static  u8 edit,apply,edit_num,delete,apply_add;
	static uip_ipaddr_t ip_addr;
	static u8 ip[4],dev_type;
	static u8 active[MAX_INPUT_NUM],inverse[MAX_INPUT_NUM],output[MAX_INPUT_NUM],devlist[MAX_INPUT_NUM],port_err;
	static char descr[MAX_INPUT_NUM][32],name[64];

	static u8 e_active[MAX_TLP_EVENTS_NUM],
			  e_inverse[MAX_TLP_EVENTS_NUM],
			  e_output[MAX_TLP_EVENTS_NUM],
			  e_devlist[MAX_TLP_EVENTS_NUM];
	PSOCK_BEGIN(&s->sout);
	str[0]=0;



	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){

	  for(u8 i=0;i<MAX_INPUT_NUM;i++){
		  active[i] = 0;
		  inverse[i] = 0;
	  }

	  for(u8 i=0;i<MAX_TLP_EVENTS_NUM;i++){
		  e_active[i] = 0;
		  e_inverse[i] = 0;
	  }

	  int i=0;
	  for(i=0;i<pcount;i++){
		  char *pname,*pval;
		  pname = http_get_parameter_name(s->param,i,sizeof(s->param));
		  pval = http_get_parameter_value(s->param,i,sizeof(s->param));



		  for(u8 j=0;j<MAX_REMOTE_TLP;j++){
			  sprintf(temp,"edit%d",j);
			  if (!strcmp(pname, temp)){
				  if(!strncmp(pval, "E", 1)){
					  edit = 1;
					  edit_num = j;
				  }
			  }
			  sprintf(temp,"del%d",j);
			  if (!strcmp(pname, temp)){
				  if(!strncmp(pval, "D", 1)){
					  delete = 1;
					  edit_num = j;
				  }
			  }
		  }

		  for(u8 i=0;i<MAX_INPUT_NUM;i++){

			sprintf(temp,"a%d",i);
			if(!strcmp(pname, temp)){
				if (!strncmp(pval, "1", 1)){
					active[i] = 1;
				}
			}
			sprintf(temp,"inv%d",i);
			if(!strcmp(pname, temp)){
				if (!strncmp(pval, "1", 1)){
					inverse[i] = 1;
				}
			}


			sprintf(temp,"descr%d",i);
			if(!strcmp(pname, temp)){
				strcpy(descr[i],pval);
			}

			sprintf(temp,"devlist%d",i);
			if(!strcmp(pname, temp)){
				for(u8 j=0;j<MAX_REMOTE_TLP;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp))
						devlist[i] = j;
				}
			}

			sprintf(temp,"ports%d",i);
			if(!strcmp(pname, temp)){
				for(u8 j=0;j<MAX_OUTPUT_NUM;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp))
						output[i] = j;
				}
			}
		  }

		  for(u8 i=0;i<MAX_TLP_EVENTS_NUM;i++){

			sprintf(temp,"e_a%d",i);
			if(!strcmp(pname, temp)){
				if (!strncmp(pval, "1", 1)){
					e_active[i] = 1;
				}
			}
			sprintf(temp,"e_inv%d",i);
			if(!strcmp(pname, temp)){
				if (!strncmp(pval, "1", 1)){
					e_inverse[i] = 1;
				}
			}

			sprintf(temp,"e_devlist%d",i);
			if(!strcmp(pname, temp)){
				for(u8 j=0;j<MAX_REMOTE_TLP;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp))
						e_devlist[i] = j;
				}
			}

			sprintf(temp,"e_ports%d",i);
			if(!strcmp(pname, temp)){
				for(u8 j=0;j<MAX_OUTPUT_NUM;j++){
					sprintf(temp,"%d",j);
					if (!strcmp(pval, temp))
						e_output[i] = j;
				}
			}
		  }

		  if(!strcmp(pname, "Apply")){
			  if (!strncmp(pval, "A",1)){
				  apply = 1;
			  }
		  }

		  if(!strcmp(pname, "ApplyEdit")){
			  if (!strncmp(pval, "A",1) || !strncmp(pval, "E",1)){
				  apply_add = 1;
			  }
		  }


		  if(!strcmp(pname, "name")){
			  strcpy(name,pval);
		  }

		  //type
		  if(!strcmp(pname, "dev")){
			  dev_type = (u8)strtoul(pval, NULL, 10);
		  }

		  for(u8 j=0;j<4;j++){
			  //ip
			  sprintf(temp,"ip%d",j+1);
			  if(!strcmp(pname, temp)){
				  ip[j]=(uint8_t)strtoul(pval, NULL, 10);
			  }
		  }



	  }

	  if(delete){
		  delete_tlp_remdev(edit_num);
		  delete = 0;
		  settings_add2queue(SQ_TELEPORT);
		  settings_save();
		  alertl("Parameters Accepted", "Настройки применились");
	  }

	  if(apply_add){
		  if(edit == 0){
			  edit_num = get_tlp_remdev_last();//calculate last free space
		  }

		  set_tlp_remdev_valid(edit_num,1);
		  set_tlp_remdev_type(edit_num,dev_type);
		  set_tlp_remdev_name(edit_num,name);
		  uip_ipaddr(ip_addr,ip[0],ip[1],ip[2],ip[3]);
		  set_tlp_remdev_ip(edit_num,&ip_addr);
		  edit = 0;
		  apply_add = 0;
		  settings_add2queue(SQ_TELEPORT);
		  settings_save();
		  alertl("Parameters Accepted", "Настройки применились");
	  }

	  if(apply){
		  for(i=0;i<MAX_INPUT_NUM;i++){
			  if(active[i]){
				  if(devlist[i] == MAX_REMOTE_TLP){
					  apply = 0;
					  alertl("Remote Device not set","Укажите удалённое устройство");
					  break;
				  }
				  if(output[i] == MAX_OUTPUT_NUM){
					  apply = 0;
					  alertl("Remote Port not set","Укажите порт удалённого устройства");
					  break;
				  }
				  if(output[i]>=get_dev_outputs(get_dev_type_by_num(get_tlp_remdev_type(devlist[i])))){
					  apply = 0;
					  alertl("Illegal remote port","Некорректный порт удалённого устройства");
					  break;
				  }

				  for(u8 j=0;j<MAX_INPUT_NUM;j++){
					  if((i != j)&&(active[j] && active[i]) && (devlist[i] == devlist[j] && output[i]==output[j])){
						  apply = 0;
						  alertl("Remote port is already used","Порт удалённого устройства используется");
						  break;
					  }
				  }
				  for(u8 j=0;j<MAX_TLP_EVENTS_NUM;j++){
					  if((e_active[j] && active[i]) && (devlist[i] == e_devlist[j]) && (output[i]==e_output[j])){
						  apply = 0;
						  alertl("Remote port is already used","Порт удалённого устройства используется");
						  break;
					  }
				  }
			  }
			  for(u8 k=0;k<MAX_TLP_EVENTS_NUM;k++){

				  for(u8 j=0;j<MAX_TLP_EVENTS_NUM;j++){
					  if((k != j)&&(e_active[j] && e_active[k]) && (e_devlist[k] == e_devlist[j] && e_output[k]==e_output[j])){
						  apply = 0;
						  alertl("Remote port is already used","Порт удалённого устройства используется");
						  break;
					  }
				  }
			  }

		  }

		  if(apply){
			  for(u8 k=0;k<MAX_INPUT_NUM;k++){
				  set_input_state(k,active[k]);
				  set_input_inverse(k,inverse[k]);
				  set_input_name(k,descr[k]);
				  set_input_remdev(k,devlist[k]);
				  set_input_remport(k,output[k]);

				  printf("set_input_remport %d %d\r\n",k,output[k]);
			  }
			  for(u8 k=0;k<MAX_TLP_EVENTS_NUM;k++){
				  set_tlp_event_state(k,e_active[k]);
				  set_tlp_event_inverse(k,e_inverse[k]);
				  set_tlp_event_remdev(k,e_devlist[k]);
				  set_tlp_event_remport(k,e_output[k]);
			  }
			  settings_save();
			  settings_add2queue(SQ_TELEPORT);
			  apply = 0;
			  alertl("Parameters Accepted", "Настройки применились");
		  }
	  }
	}

	addhead();
	addstr("<script type=\"text/javascript\">"
		"function open_dev_info(num)"
		"{"
			"window.open('/info/devinfo.shtml?num='+num,'','width=400,height=400,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');"
		"}"
		"function change_css(port){"
			"var tlpD = document.getElementById('tlpD'+port);"
			"var tlpP = document.getElementById('tlpP'+port);"
			"var tlpI = document.getElementById('tlpI'+port);"
			"if(document.getElementById('active'+port).checked)"//active
			"{"
				"tlpD.style.display = \"table-cell\";"
				"tlpP.style.display = \"table-cell\";"
				"tlpI.style.display = \"table-cell\";"

			"}"
			"else"//standalone
			"{"
				"tlpD.style.display = \"none\";"
				"tlpP.style.display = \"none\";"
				"tlpI.style.display = \"none\";"
			"}"

		"}");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
		addstr("function e_change_css(port){"
			"var e_tlpD = document.getElementById('e_tlpD'+port);"
			"var e_tlpP = document.getElementById('e_tlpP'+port);"
			"var e_tlpI = document.getElementById('e_tlpI'+port);"
			"if(document.getElementById('e_active'+port).checked)"//active
			"{"
				"e_tlpD.style.display = \"table-cell\";"
				"e_tlpP.style.display = \"table-cell\";"
				"e_tlpI.style.display = \"table-cell\";"

			"}"
			"else"//standalone
			"{"
				"e_tlpD.style.display = \"none\";"
				"e_tlpP.style.display = \"none\";"
				"e_tlpI.style.display = \"none\";"
			"}"

		"}"

	"</script>");
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	addstr("<body>");
	addstrl("<legend>Remote Devices</legend>","<legend>Удалённые устройства</legend>");
	if(get_current_user_rule()!=ADMIN_RULE){
		addstrl("Access denied","Доступ запрещён");
	}else{
		addstr( "<form method=\"post\" bgcolor=\"#808080\""
		"action=\"/settings/teleport.shtml\"><br>");

		addstr("<caption><b>");
		addstrl("Devices List","Список устройств");
		addstr("<b></caption>"
		"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\">"
		"<td>&nbsp;");
		addstr("</td><td>");
		addstrl("Name","Имя");
		addstr("</td><td>");
		addstrl("Type","Тип");
		addstr("</td><td>");
		addstrl("IP Address","IP адрес");
		addstr("</td>");
		addstr("<td></td></tr>");
		if(get_remdev_num()){
			for(i=0;i<get_remdev_num();i++){
				addstr("<tr class=\"g2\">"
				"<td>");
				sprintf(temp,"%d",(u8)(i+1));
				addstr(temp);
				addstr("</td><td>");
				get_tlp_remdev_name(i,temp);
				convert_str(temp,name,64);
				addstr(name);
				addstr("</td><td>");
				get_dev_name_by_num(get_tlp_remdev_type(i),temp);
				addstr(temp);
				addstr("</td><td>");
				get_tlp_remdev_ip(i,&ip_addr);
				sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip_addr),uip_ipaddr2(ip_addr),
						uip_ipaddr3(ip_addr),uip_ipaddr4(ip_addr));
				addstr(temp);
				addstr("</td><td>"
				"<input type=\"Submit\" name=\"info");
				sprintf(temp,"%d",(u8)i);
				addstr(temp);
				addstr("\" value=\"Info\" onclick=\"open_dev_info(");
				sprintf(temp,"%d",(u8)i);
				addstr(temp);
				addstr(");\">&nbsp;"
				"<input type=\"Submit\" name=\"edit");
				sprintf(temp,"%d",(u8)i);
				addstr(temp);
				addstr("\" value=\"Edit\">&nbsp;"
				"<input type=\"Submit\" name=\"del");
				sprintf(temp,"%d",(u8)i);
				addstr(temp);
				addstr("\" value=\"Delete\">"
				"</td></tr>");

				if(i%2 == 0){
					PSOCK_SEND_STR(&s->sout,str);
					str[0]=0;
				}
			}
			addstr("</table>");
		}
		else{
			addstr("<tr class=\"g2\">"
			"<td></td><td></td><td></td><td></td><td></td></tr></table><br>");
			addstrl("Device list is empty","Список устройств пуст");
		}

		addstr("<br><br><br>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		if(get_remdev_num() < MAX_REMOTE_TLP || edit){
			addstr("<caption><b>");
			if(edit == 0){
				addstrl("Add New Remote Device","Добавить новое удалённое устройство");
			}
			else{
				addstrl("Edit Remote Device","Редактировать удалённое устройство");
			}
			addstr("<b></caption>"
			"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
			"<tr class=\"g2\"><td>");
			addstrl("Name","Имя");
			addstr("</td><td>");
			addstr("<input type=\"text\"name=\"name\" size=\"16\" maxlength=\"128\"value=\"");
			if(edit){
				get_tlp_remdev_name(edit_num,temp);
				convert_str(temp,name,64);
				addstr(name);
			}
			addstr("\"></td></tr>"

			"<tr class=\"g2\"><td>");
			addstrl("Type","Тип");
			addstr("</td><td>"
			"<select name=\"dev\" size=\"1\">");
			for(u8 i=0;i<get_dev_type_num();i++){
			  addstr("<option");
			  if(get_tlp_remdev_type(edit_num) == i && edit)
				addstr(" selected");
			  addstr(" value=\"");
			  sprintf(temp,"%d",i);
			  addstr(temp);
			  addstr("\">");
			  get_dev_name_by_num(i,temp);
			  addstr(temp);
			  addstr("</option>");
			}
			addstr("</select></td></tr>"

			"<tr class=\"g2\"><td>");
			addstrl("IP Address","IP адрес");
			addstr("</td><td>");
			addstr("<input type=\"text\"name=\"ip1\" size=\"3\" maxlength=\"3\"value=\"");
			get_tlp_remdev_ip(edit_num,&ip_addr);
			if(edit){
				sprintf(temp,"%d",uip_ipaddr1(ip_addr));
				addstr(temp);
			}
			addstr("\">"
			"<input type=\"text\"name=\"ip2\" size=\"3\" maxlength=\"3\"value=\"");
			if(edit){
				sprintf(temp,"%d",uip_ipaddr2(ip_addr));
				addstr(temp);
			}
			addstr("\">"
			"<input type=\"text\"name=\"ip3\" size=\"3\" maxlength=\"3\"value=\"");
			if(edit){
				sprintf(temp,"%d",uip_ipaddr3(ip_addr));
				addstr(temp);
			}
			addstr("\">"
			"<input type=\"text\"name=\"ip4\" size=\"3\" maxlength=\"3\"value=\"");
			if(edit){
				sprintf(temp,"%d",uip_ipaddr4(ip_addr));
				addstr(temp);
			}
			addstr("\"></td></tr>"
			"<tr class=\"g2\"><td></td><td>");
			addstr("<input type=\"Submit\" name=\"ApplyEdit\" value=\"");
			if(edit == 0)
				addstr("Add Device");
			else
				addstr("Edit Device");
			addstr("\"></td></tr>"
			"</table><br><br>");
		}


		addstr("<caption><b>");
		addstrl("Inputs","Входы");
		addstr("<b></caption>"
		"<br>"
		"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\">"
		"<td>&nbsp;");
		addstr("</td><td>");
		//addstrl("Active","Активность");
		addstrl("Translate","Транслировать");
		addstr("</td><td>");
		addstrl("Remote Device","Удалённое устройство");
		addstr("</td><td>");
		addstrl("Remote Port","Порт");
		addstr("</td><td>");
		addstrl("Inverse","Инвертировать");
		addstr("</td><td>");
		addstrl("Current State","Текущее состояние");
		addstr("</td></tr>");


		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;



		for(i = 0;i<MAX_INPUT_NUM;i++){
			if(i==0 && (get_dev_type() == DEV_PSW2GPLUS || get_dev_type() == DEV_PSW2G6F || get_dev_type() == DEV_PSW2G8F)){
				addstr("<tr class=\"g2\">"
				"<td>");
				addstrl("Tamper","Датчик вскртия");
			}
			else{
				if(i==0)
					continue;
				addstr("<tr class=\"g2\">"
				"<td>");
				sprintf(temp,"%d",(u8)i);
				addstr(temp);
			}
			addstr("</td><td>"
			"<input type=checkbox name=\"a");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" value=\"1\" ");
			if(get_input_state(i) && get_remdev_num())
				addstr(" checked ");
			addstr("id=\"active");
			addstr(temp);
			addstr("\" onchange=\"change_css(");
			addstr(temp);
			addstr(")\"></td><td>"
			"<select id=\"tlpD");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\"");
			addstr(" name=\"devlist");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" size=\"1\">");
			/*"<option ");
			if(get_remdev_num() == 0 || get_input_rem_dev(i) == MAX_REMOTE_TLP)
				addstr(" selected ");
			addstr(" value=\"");
			sprintf(temp,"%d",MAX_REMOTE_TLP);
			addstr(temp);
			addstr("\">");
			addstr("----------");
			addstr("</option>");*/
			for(u8 j=0;j<get_remdev_num();j++){
				addstr("<option");
				if(get_input_rem_dev(i) == j )
					addstr(" selected");
				addstr(" value=\"");
				sprintf(temp,"%d",j);
				addstr(temp);
				addstr("\">");
				get_tlp_remdev_name(j,temp);
				if(strlen(temp)){
					convert_str(temp,name,64);
					addstr(name);
				}
				else{
					get_tlp_remdev_ip(j,&ip_addr);
					sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip_addr),uip_ipaddr2(ip_addr),uip_ipaddr3(ip_addr),uip_ipaddr4(ip_addr));
					addstr(temp);
				}
				addstr("</option>");
			}

			addstr("</select></td>"
			"<td>"
			"<select id=\"tlpP");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\"");
			addstr(" name=\"ports");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" size=\"1\">");
			/*"<option ");
			if(get_remdev_num() == 0 || get_input_rem_port(i)==MAX_OUTPUT_NUM)
				addstr(" selected ");
			addstr(" value=\"");
			sprintf(temp,"%d",MAX_OUTPUT_NUM);
			addstr(temp);
			addstr("\">");
			addstr("----------");
			addstr("</option>");*/
			for(u8 j=0;j<MAX_OUTPUT_NUM;j++){
				addstr("<option");
				if(get_input_rem_port(i) == j)
					addstr(" selected");
				addstr(" value=\"");
				sprintf(temp,"%d",j);
				addstr(temp);
				addstr("\">");
				sprintf(temp,"Out %d",j+1);
				addstr(temp);
				addstr("</option>");
			}

			addstr("</select></td><td>"
			"<input type=checkbox name=\"inv");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" value=\"1\" ");
			if(get_input_inverse(i))
				addstr(" checked ");
			addstr("id=\"tlpI");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\"></td><td>");
			if(i == 0){
				if(get_sensor_state(i) == 0)
					addstrl("Alarm","Крышка вскрыта");
				else
					addstrl("Normal","Нормальное");
			}
			else{
				if(get_sensor_state(i)==Bit_RESET)
					addstrl("Short", "Замкнутое");
				else
					addstrl("Open", "Разомкнутое");
			}
			addstr("</td>");
			addstr("<script type=\"text/javascript\">change_css(");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr(");</script>"
			"</tr>");

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

		}
		addstr("</table>");


		addstr("<br><br><caption><b>");
		addstrl("Events","События");
		addstr("<b></caption>"
		"<br>"
		"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\">"
		"<td>&nbsp;");
		addstr("</td><td>");
		//addstrl("Active","Активность");
		addstrl("Translate","Транслировать");
		addstr("</td><td>");
		addstrl("Description","Описание");
		addstr("</td><td>");
		addstrl("Remote Device","Удалённое устройство");
		addstr("</td><td>");
		addstrl("Remote Port","Порт");
		addstr("</td><td>");
		addstrl("Inverse","Инвертировать");
		addstr("</td><td>");
		addstrl("Current State","Текущее состояние");
		addstr("</td></tr>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		for(i=0;i<MAX_TLP_EVENTS_NUM;i++){
			addstr("<tr class=\"g2\">"
			"<td>");
			sprintf(temp,"%d",(u8)i+1);
			addstr(temp);
			addstr("</td><td>"
			"<input type=checkbox name=\"e_a");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" value=\"1\" ");
			if(get_tlp_event_state(i) && get_remdev_num())
				addstr(" checked ");
			if(i==0 && is_ups_mode()==0)
				addstr(" disabled ");
			addstr("id=\"e_active");
			addstr(temp);
			addstr("\" onchange=\"e_change_css(");
			addstr(temp);
			addstr(")\"></td><td>");
			switch(i){
				case 0: addstrl("UPS","ИБП");break;
				case 1: addstrl("AutoRestart","Контроль зависания камер");break;
			}
			addstr("</td><td>"
			"<select id=\"e_tlpD");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\"");
			addstr(" name=\"e_devlist");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" size=\"1\">");
			/*"<option ");
			if(get_remdev_num() == 0 || get_tlp_event_rem_dev(i) == MAX_REMOTE_TLP)
				addstr(" selected ");
			addstr(" value=\"");
			sprintf(temp,"%d",MAX_REMOTE_TLP);
			addstr(temp);
			addstr("\">");
			addstr("----------");
			addstr("</option>");*/
			for(u8 j=0;j<get_remdev_num();j++){
				addstr("<option");
				if(get_tlp_event_rem_dev(i) == j )
					addstr(" selected");
				addstr(" value=\"");
				sprintf(temp,"%d",j);
				addstr(temp);
				addstr("\">");
				get_tlp_remdev_name(j,temp);
				if(strlen(temp)){
					convert_str(temp,name,64);
					addstr(name);
				}
				else{
					get_tlp_remdev_ip(j,&ip_addr);
					sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip_addr),uip_ipaddr2(ip_addr),uip_ipaddr3(ip_addr),uip_ipaddr4(ip_addr));
					addstr(temp);
				}
				addstr("</option>");
			}

			addstr("</select></td>"
			"<td >"
			"<select id=\"e_tlpP");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\"");
			addstr(" name=\"e_ports");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" size=\"1\">");
			/*"<option ");
			if(get_remdev_num() == 0 || get_tlp_event_rem_port(i)==MAX_OUTPUT_NUM)
				addstr(" selected ");
			addstr(" value=\"");
			sprintf(temp,"%d",MAX_OUTPUT_NUM);
			addstr(temp);
			addstr("\">");
			addstr("----------");
			addstr("</option>");*/
			for(u8 j=0;j<MAX_OUTPUT_NUM;j++){
				addstr("<option");
				if(get_tlp_event_rem_port(i) == j)
					addstr(" selected");
				addstr(" value=\"");
				sprintf(temp,"%d",j);
				addstr(temp);
				addstr("\">");
				sprintf(temp,"Out %d",j+1);
				addstr(temp);
				addstr("</option>");
			}

			addstr("</select></td><td>"
			"<input type=checkbox name=\"e_inv");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\" value=\"1\" ");
			if(get_tlp_event_inverse(i))
				addstr(" checked ");
			addstr("id=\"e_tlpI");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr("\"></td><td>");

			switch(i){
				case 0:
					if(is_ups_rezerv())
						addstrl("Short", "Замкнутое");
					else
						addstrl("Open", "Разомкнутое");
					break;
				case 1:
					port_err = 0;
					for(u8 k=0;k<COOPER_PORT_NUM;k++){
						if(get_port_error(k)){
							port_err = 1;
						}
					}
					if(port_err)
						addstrl("Short", "Замкнутое");
					else
						addstrl("Open", "Разомкнутое");
					break;
			}



			addstr("</td>");
			addstr("<script type=\"text/javascript\">e_change_css(");
			sprintf(temp,"%d",(u8)i);
			addstr(temp);
			addstr(");</script>"
			"</tr>");

			PSOCK_SEND_STR(&s->sout,str);
			str[0]=0;

		}
		addstr("</table>");

		addstr("<br><input type=\"Submit\" name=\"Apply\" value=\"Apply\">"
					"</form></body></html>");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}



/*******************************************************************************************************/
/*        STATICTICS -> Remote Device info 															    	*/
/*******************************************************************************************************/
static PT_THREAD(run_devinfo(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	static u8 num=0xFF;
	uip_ipaddr_t ip_addr;
	char name[64];

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "num")){
				num=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(num<MAX_REMOTE_TLP){
	    addstr("<html><head>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
	    "<!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/ui.css\">-->"
		"<!--[if IE]>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/styleIE.css\">"
		"<![endif]-->");

		addstr("<title>");
		addstrl("Device Info","Информация о устройстве");
		addstr("</title>"
		"</head><body>"
		"<form method=\"post\" bgcolor=\"#808080\""
		"action=\"/info/devinfo.shtml?num=");
		sprintf(temp,"%d",num);
		addstr(temp);
		addstr("\"><br>");
		addstr("<caption><b>");
		addstrl("Remote Device Info","Информация об удалённом устройстве");
		addstr("<b></caption>"

		"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\"><td>");
		addstrl("Name","Имя");
		addstr("</td><td>");
		get_tlp_remdev_name(num,temp);
		convert_str(temp,name,64);
		addstr(name);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Type","Тип устройства");
		addstr("</td><td>");
		get_dev_name_by_num(get_tlp_remdev_type(num),temp);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Inputs num","Число входов");
		addstr("</td><td>");
		sprintf(temp,"%d",get_dev_inputs(get_dev_type_by_num(get_tlp_remdev_type(num))));
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("Outputs num","Число выходов");
		addstr("</td><td>");
		sprintf(temp,"%d",get_dev_outputs(get_dev_type_by_num(get_tlp_remdev_type(num))));
		addstr(temp);
		addstr("</td></tr>"

		"<tr class=\"g2\"><td>");
		addstrl("IP Address","IP адрес");
		addstr("</td><td>");
		get_tlp_remdev_ip(num,&ip_addr);
		sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip_addr),uip_ipaddr2(ip_addr),
		uip_ipaddr3(ip_addr),uip_ipaddr4(ip_addr));
		addstr(temp);
		addstr("</td></tr>"

		"<tr class=\"g2\"><td>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstrl("Connection status","Статус подключения");
		addstr("</td><td>");
		if(dev.remdev[num].conn_state){
			addstrl("Connected","Подключено");
		}
		else{
			addstrl("Not connected","Не подключено");
		}
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("RX mngmt frames","Число входящих пакетов");
		addstr("</td><td>");
		sprintf(temp,"%lu",dev.remdev[num].rx_count);
		addstr(temp);
		addstr("</td></tr>"
		"<tr class=\"g2\"><td>");
		addstrl("TX mngmt frames","Число исходящих пакетов");
		addstr("</td><td>");
		sprintf(temp,"%lu",dev.remdev[num].tx_count);
		addstr(temp);
		addstr("</td></tr>"

		"</table>"
		"<br><br>"
		"<input type=\"Submit\" name=\"Refresh\" value=\"Refresh\">"
		"</form></body></html>");




		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

	}
	else
		addstr("incorrect http request!");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


static void add_cpu_stat(void){
	  addstr("<script type=\"text/javascript\">"
	  "var cpuArray = [");
	  for(u16 i=0;i<CPU_STAT_LEN;i++){
		  sprintf(temp,"%d",dev.cpu_stat.cpu_level[i]);
		  addstr(temp);
		  if(i==(CPU_STAT_LEN-1))
			  addstr("];");
		  else
			  addstr(",");
	  }
	  addstr("</script>");
}


/*******************************************************************************************************/
/*        STATICTICS -> Device Info / CPU Info													    	*/
/*******************************************************************************************************/
static PT_THREAD(run_cpuinfo(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	extern const uint32_t image_version[1];
	u8 tmp[8];

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));

		}
	}

	addhead();
	ADDSCRIPT
	addstr("<body><legend>");
	addstrl("Device Info","Информация о устройстве");
	addstr("</legend>");
    if(get_current_user_rule()!=ADMIN_RULE){
    	addstr("Access denied");
	}
	else{
		addstr("<form method=\"post\" bgcolor=\"#808080\""
		"action=\"/info/cpuinfo.shtml\"><br>"
		"<table cellspacing=\"2\" CELLPADDING=\"6\" class=\"g1\">"
		"<tr class=\"g2\"><td>");

		addstrl("Device Type","Тип устройства");
		addstr("</td><td>");
		get_dev_name(temp);
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>");
		addstrl("CPU ID","ID процессора");
		addstr("</td><td>");
		get_cpu_id_to_str(temp);
		addstr(temp);
		addstr("</td></tr>");

		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;

		addstr("<tr class=\"g2\"><td>");
		addstrl("HW version","Аппаратная версия");
		addstr("</td><td>");
		sprintf(temp,"%d",get_board_id());
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>");
		addstrl("Software version","Версия встроенного ПО");
		addstr("</td><td>");
		sprintf(temp,"%02X.%02X.%02X",
				(int)(image_version[0]>>16)&0xff,
				(int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
		addstr(temp);
		addstr("</td></tr>");



		addstr("<tr class=\"g2\"><td>");
		addstrl("Bootloader version","Версия загрузчика");
		addstr("</td><td>");
		sprintf(temp,"%02X.%02X",
				(int)(bootloader_version>>8)&0xff,  (int)(bootloader_version)&0xff);
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>");
		addstrl("MAC","MAC адрес");
		addstr("</td><td>");
		get_net_def_mac(tmp);
		sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
				tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>");
		addstrl("Switch ID","Switch-контроллер ID");
		addstr("</td><td>");
		sprintf(temp,"0x%X", get_marvell_id());
		addstr(temp);
		addstr("</td></tr>");
		if((get_dev_type()!=DEV_SWU16)){
			 addstr("<tr class=\"g2\"><td>");
			 addstrl("PoE ID","PoE-контроллер ID");
			 addstr("</td><td>");
			 sprintf(temp,"0x%X", get_poe_id());
			 addstr(temp);
			 addstr("</td></tr>");
		}

		addstr("<tr class=\"g2\"><td>");
		addstrl("SPI-Flash","SPI-Flash");
		addstr("</td><td>");
		if(spi_flash_manufacture()==SPANSION_FLASH)
			addstr("Spansion");
		if(spi_flash_manufacture()==MACRONYX_FLASH)
			addstr("Macronyx");
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>");
		addstrl("Heap free","Heap free");
		addstr("</td><td>");
		sprintf(temp,"%d",xPortGetFreeHeapSize());
		addstr(temp);
		addstr("</td></tr>");

		addstr("<tr class=\"g2\"><td>");
		addstrl("HW Errors","Аппаратные ошибки");
		addstr("</td><td>");
		if(init_ok){
			addstrl("No","Нет");
		}
		else{
			for(u8 i=0;i<ALARM_REC_NUM;i++){
				if(printf_alarm(i,temp)!=0){
					addstr(temp);
					addstr(", ");
				}
			}
		}

		addstr("</td></tr>");


		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;


		addstr("</table>"
		"<br><br>");
		add_cpu_stat();
		addstr("<b>");
		addstrl("CPU Utilization","Загрузка CPU");
		addstr("</b><br><canvas id='canvasCPU'>Извините, тег Canvas недоступен!</canvas>"
		"<script type=\"text/javascript\">"
		  "drawGraph('canvasCPU', cpuArray,300,400);"
		"</script>");

		addstr("<br><br><input type=\"Submit\" name=\"Refresh\" value=\"Refresh\">"
		"</form></body></html>");
	}


	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}





/*******************************************************************************************************/
/*        HTTP API										    	*/
/*******************************************************************************************************/

//input status
static PT_THREAD(run_getInput(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	u8 input=0xFF;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "input")){
				input=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(input<MAX_INPUT_NUM){
		if(get_sensor_state(input)==0)
			addstr("1");
		else
			addstr("0");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
	}
	else
		addstr("error");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//link status
static PT_THREAD(run_getLink(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	u8 port=0xFF;
	str[0]=0;

	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}

	if(port<ALL_PORT_NUM){
		if(get_port_link(port))
			addstr("up");
		else
			addstr("down");
	}
	else
		addstr("error");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//set poe state
static PT_THREAD(run_setPoe(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	u8 port=0xFF;
	u8 state=0xFF;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
			if(!strcmp(pname, "state")){
				state=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(port<POE_PORT_NUM){
		if(state==0 || state == 1){
			set_port_poe(port,state);
			set_poe_init(0);
			addstr("ok");
		}
		else{
			addstr("error");
		}
	}
	else
		addstr("error");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//poe status
static PT_THREAD(run_getPoe(struct httpd_state *s, char *ptr)){
	static u8 pcount = 0;
	u8 port=0xFF;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(port<POE_PORT_NUM){
		if(get_port_poe_a(port) || get_port_poe_b(port))
			addstr("up");
		else
			addstr("down");
		PSOCK_SEND_STR(&s->sout,str);
		str[0]=0;
	}
	else
		addstr("error");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//poe power
static PT_THREAD(run_getPoePower(struct httpd_state *s, char *ptr)){
static u8 pcount = 0;
u8 port=0xFF;
uint32_t Temp,tot_pwr;

	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}

	if(port<POE_PORT_NUM){
		if(get_port_poe_a(port)||get_port_poe_b(port)){
			Temp = 0;
			if(get_port_poe_a(port)){
				Temp += get_poe_voltage(POE_A,port)*get_poe_current(POE_A,port)/1000;
			}
			if(get_port_poe_b(port)){
				Temp += get_poe_voltage(POE_B,port)*get_poe_current(POE_B,port)/1000;
			}
			if(Temp){
				tot_pwr+=Temp;
				sprintf(temp,"%d.%d",(int)(Temp/1000),(int)(Temp % 1000));
				addstr(temp);
			}
		}
		else{
			addstr("0.0");
		}
	}
	else{
		addstr("error");
	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


//total port num
static PT_THREAD(run_getPortNum(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	sprintf(temp,"%d",ALL_PORT_NUM);
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//ups is connected
static PT_THREAD(run_isUps(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	sprintf(temp,"%d",is_ups_mode());
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//ups satus
static PT_THREAD(run_getUpsStatus(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(is_ups_mode()){
		if(is_ups_rezerv()==0){
			addstr("0");
		}
		else{
			addstr("1");
		}
	}
	else{
		addstr("0");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//ups voltage
static PT_THREAD(run_getUpsVoltage(struct httpd_state *s, char *ptr)){
u16 tmp_voltage;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(is_ups_mode()){
		if(is_akb_detect()==1){
			tmp_voltage = get_akb_voltage();
		 	if(tmp_voltage<360){
				addstr("0");
			}
			 else{
				sprintf(temp,"%d.%d",tmp_voltage/10,tmp_voltage%10);
				addstr(temp);
			 }
		}else{
			addstr("0");
		}
	}
	else{
		addstr("0");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//ups уыешьфеув
static PT_THREAD(run_getUpsEstimated(struct httpd_state *s, char *ptr)){
u16 tmp_voltage;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(is_ups_mode()|| is_akb_detect()==1 || is_ups_rezerv()==1){
		if(remtime.valid){
			sprintf(temp,"%d",remtime.hour*60+remtime.min);
			addstr(temp);
		}
		else{
			addstr("0");
		}
	}
	else{
		addstr("0");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


static PT_THREAD(run_getNetIp(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
    sprintf(temp,"%d.%d.%d.%d",(u8)uip_hostaddr[0],(u8)(uip_hostaddr[0]>>8),(u8)uip_hostaddr[1],(u8)(uip_hostaddr[1]>>8));
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getNetMac(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
			(int)dev_addr[0],(int)dev_addr[1],(int)dev_addr[2],
			(int)dev_addr[3],(int)dev_addr[4],(int)dev_addr[5]);
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getNetMask(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	sprintf(temp,"%d.%d.%d.%d",(u8)uip_netmask[0],(u8)(uip_netmask[0]>>8), (u8)uip_netmask[1], (u8)(uip_netmask[1]>>8));
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getNetGate(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(&uip_draddr),uip_ipaddr2(&uip_draddr), uip_ipaddr3(&uip_draddr), uip_ipaddr4(&uip_draddr));
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}


static PT_THREAD(run_getFwVersion(struct httpd_state *s, char *ptr)){
extern const uint32_t image_version[1];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	sprintf(temp,"%02X.%02X.%02X",
			 (int)(image_version[0]>>16)&0xff,
			 (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getDevType(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	get_dev_name_r(temp);
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getDevName(struct httpd_state *s, char *ptr)){
char text_tmp[127];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	get_interface_name(temp);
	temp[127]=0;
	if(strlen(temp)){
		memset(text_tmp,0,sizeof(text_tmp));
		http_url_decode(temp,text_tmp,strlen(temp));
		for(uint8_t i=0;i<strlen(text_tmp);i++){
			if(text_tmp[i]=='+') text_tmp[i] = ' ';
			if(text_tmp[i]=='%') text_tmp[i]=' ';
		}
		addstr(text_tmp);
	}
	else{
		addstr(" ");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getDevLocation(struct httpd_state *s, char *ptr)){
char text_tmp[127];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	get_interface_location(temp);
	temp[127]=0;
	if(strlen(temp)){
		memset(text_tmp,0,sizeof(text_tmp));
		http_url_decode(temp,text_tmp,strlen(temp));
		for(uint8_t i=0;i<strlen(text_tmp);i++){
			if(text_tmp[i]=='+') text_tmp[i] = ' ';
			if(text_tmp[i]=='%') text_tmp[i]=' ';
		}
		addstr(text_tmp);
	}
	else{
		addstr(" ");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//список МАС адресов на порту
static PT_THREAD(run_getPortMacList(struct httpd_state *s, char *ptr)){
struct mac_entry_t entry;
u16 cnt,i;
u8 start;
static u8 pcount = 0;
u8 port=0xFF;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(port<ALL_PORT_NUM){
		addstr("{");
		if(get_dev_type() == DEV_SWU16){
			cnt=0;
			for(i=0;i<SALSA2_FDB_MAX;i++){
				if(get_salsa2_fdb_entry(i,&entry)==1){
					if(entry.port_vect != port)
						continue;
				}
				sprintf(temp,"\"%d\" : \"%02X:%02X:%02X:%02X:%02X:%02X\",",
				 		cnt,entry.mac[0],entry.mac[1],entry.mac[2],
						entry.mac[3],entry.mac[4],entry.mac[5]);
				addstr(temp);
				cnt++;
			}
		}
		else{
			cnt=0;
			while(read_atu(0, start, &entry)==0){
				start = 0;
				if(!(entry.port_vect & (1<<L2F_port_conv(port))))
					continue;
				sprintf(temp,"\"%d\" : \"%02X:%02X:%02X:%02X:%02X:%02X\",",
				 		cnt,entry.mac[0],entry.mac[1],entry.mac[2],
						entry.mac[3],entry.mac[4],entry.mac[5]);
				addstr(temp);
				cnt++;
			}
		}

		if(cnt)
			str[strlen(str)-1]=0;

		addstr("}");
	}
	else{
		addstr("error");
	}

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//запуск кабельного тестера на порту
static PT_THREAD(run_cableTesterStart(struct httpd_state *s, char *ptr)){
static u8 pcount = 0;
u8 port=0xFF;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(is_cooper(port)){
		set_cable_test(VCT_TEST,port,0);
		addstr("ok");
	}
	else
		addstr("error");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}

//результат кабельного тестера на порту
static PT_THREAD(run_cableTesterStatus(struct httpd_state *s, char *ptr)){
static u8 pcount = 0;
u8 port=0xFF;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	if(s->param[0] && (pcount=http_get_parameters_parse(s->param,sizeof(s->param)))>0){
		char *pname,*pval;
		for(int i=0;i<pcount;i++){
 		    pname = http_get_parameter_name(s->param,i,sizeof(s->param));
			pval = http_get_parameter_value(s->param,i,sizeof(s->param));
			if(!strcmp(pname, "port")){
				port=(u8)strtoul(pval, NULL, 0);
			}
		}
	}
	if(is_cooper(port)){
		if(get_cable_test() == VCT_TEST){
			addstr("running");
		}
		else{
			//тест закончился
			if(dev.port_stat[port].vct_compleat_ok){
				if((dev.port_stat[port].rx_status == VCT_GOOD) && (dev.port_stat[port].tx_status == VCT_GOOD)){
					addstr("ok;normal");
				}
				else{
					if(dev.port_stat[port].rx_status == VCT_SHORT || dev.port_stat[port].tx_status == VCT_SHORT){
						if(dev.port_stat[port].rx_status == VCT_SHORT)
							sprintf(temp, "ok;short;%d", (int)(dev.port_stat[port].rx_len));
						else
							sprintf(temp, "ok;short;%d", (int)(dev.port_stat[port].tx_len));
					}
					if(dev.port_stat[port].rx_status == VCT_OPEN || dev.port_stat[port].tx_status == VCT_OPEN){
						if(dev.port_stat[port].rx_status == VCT_OPEN)
							sprintf(temp, "ok;open;%d", (int)(dev.port_stat[port].rx_len));
						else
							sprintf(temp, "ok;open;%d", (int)(dev.port_stat[port].tx_len));
					}
					addstr(temp);
					dev.port_stat[port].vct_compleat_ok = 0;
				}
			}
			else{
				addstr("ok");
			}
		}
	}
	else
		addstr("error");

	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}




static PT_THREAD(run_getDevContact(struct httpd_state *s, char *ptr)){
char text_tmp[127];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	get_interface_contact(temp);
	temp[127]=0;
	if(strlen(temp)){
		memset(text_tmp,0,sizeof(text_tmp));
		http_url_decode(temp,text_tmp,strlen(temp));
		for(uint8_t i=0;i<strlen(text_tmp);i++){
			if(text_tmp[i]=='+') text_tmp[i] = ' ';
			if(text_tmp[i]=='%') text_tmp[i]=' ';
		}
		addstr(text_tmp);
	}
	else{
		addstr(" ");
	}
	PSOCK_SEND_STR(&s->sout,str);
	str[0]=0;
	PSOCK_END(&s->sout);
}


static PT_THREAD(run_getSerialNum(struct httpd_state *s, char *ptr)){
u8 mac[6];
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	get_net_def_mac(mac);
	sprintf(temp,"%d",(int)(mac[4]<<8 | (uint8_t)mac[5]));
	addstr(temp);
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}

static PT_THREAD(run_getUptime(struct httpd_state *s, char *ptr)){
RTC_TimeTypeDef RTC_Time;
RTC_DateTypeDef RTC_Date;
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	RTC_GetTime(RTC_Format_BIN,&RTC_Time);
	RTC_GetDate(RTC_Format_BCD,&RTC_Date);

	sprintf(temp,"%lu",RTC_Date.RTC_Date*86400+RTC_Time.RTC_Hours*3600+RTC_Time.RTC_Minutes*60+RTC_Time.RTC_Seconds);
	addstr(temp);
	PSOCK_SEND_STR(&s->sout,temp);
	str[0]=0;
	PSOCK_END(&s->sout);
}



static PT_THREAD(run_rebootAll(struct httpd_state *s, char *ptr)){
	str[0]=0;
	PSOCK_BEGIN(&s->sout);
	PSOCK_SEND_STR(&s->sout,"ok");
	str[0]=0;
	reboot(REBOOT_MCU_5S);
	PSOCK_END(&s->sout);
}
