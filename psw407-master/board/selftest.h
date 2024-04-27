/*
 * selftest.h
 */

#ifndef SELFTEST_H_
#define SELFTEST_H_

//#define ALARM_TEXT_LEN		32
#define ALARM_REC_NUM		10




#define NO_ERROR 					0


#define ERROR_WRITE_BB				1
#define ERROR_POE_INIT				2
#define DEFAULT_TAST_NOSTART		3
#define ERROR_INIT_BB				4
#define ERROR_I2C					5
#define ERROR_CREATE_IGMP_QUEUE		6
#define ERROR_CREATE_IGMP_TASK		7
#define ERROR_CREATE_SMTP_QUEUE		8
#define ERROR_CREATE_SMTP_TASK		9
#define ERROR_CREATE_SNMP_QUEUE		10
#define ERROR_CREATE_SNTP_TASK		11
#define ERROR_CREATE_STP_QUEUE		12
#define ERROR_CREATE_STP_NOSEND		13
#define ERROR_CREATE_SYSLOG_QUEUE	14
#define ERROR_CREATE_AUTORESTART_TASK	15
#define ERROR_CREATE_POE_TASK		16
#define ERROR_CREATE_SS_TASK		17
#define ERROR_CREATE_OTHER_TASK		18
#define ERROR_CREATE_UIP_TASK		19
#define ERROR_CREATE_POECAM_TASK	20
#define ERROR_MARVEL_START			21
#define ERROR_CREATE_TFTP_QUEUE		22
#define ERROR_CREATE_SHELL_TASK		23
#define ERROR_CREATE_TLP_QUEUE		24
#define ERROR_MARVEL_PHY0			25
#define ERROR_MARVEL_PHY1			26
#define ERROR_MARVEL_PHY2			27
#define ERROR_MARVEL_PHY3			28
#define ERROR_MARVEL_FREEZE			29//детекция зависания marvell`a


struct alarm_list_t{
	u8 alarm_code;
};

void add_alarm(u8 alarm_code,u8 state);
int printf_alarm(u8 num,char *text);
u8 get_alarm_num(void);

void ADC_test_init(void);
u16 readADC1(u8 channel);

u8 start_selftest(void);
#define ADD_ALARM(code) add_alarm(code,1)
#define DEL_ALARM(code) add_alarm(code,0)



#endif /* SELFTEST_H_ */
