/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack
 *
 * $Id: telnetd.h,v 1.2 2006/06/07 09:43:54 adam Exp $
 *
 */
#ifndef __TELNETD_H__
#define __TELNETD_H__

#include "../deffines.h"
#include "../uip/uip.h"
#include "queue.h"

//key code
#define KEY_NL		0x0A// \n
#define KEY_CR		0x0D// \r
#define KEY_UP		0x26
#define KEY_DOWN	0x28
#define KEY_BS		0x08
#define KEY_TAB		0x09
#define KEY_ESC		0x1B

#define KEY_UP_5B	0x5B
#define KEY_UP_41	0x41

#define KEY_DOWN_5B	0x5B
#define KEY_DOWN_42	0x42

#define KEY_LEFT_5B	0x5B
#define KEY_LEFT_44	0x44

#define KEY_RIGHT_43 0x43

#define KEY_BS2		0x7F



#define KEY_CTRLC 0x03


#define NO_KEY_PRESSED		0
#define KEY_PRESSED_ENTER	1
#define KEY_PRESSED_UP		2
#define KEY_PRESSED_DOWN	3
#define KEY_PRESSED_LEFT	3
#define KEY_PRESSED_RIGHT	4
#define KEY_PRESSED_TAB		5
#define	KEY_PRESSED_BS		6//bacspase
#define KEY_PRESSED_CTRLC	7//control + c




//tftp processing
#define TFTP_IDLE			0
#define TFTP_UPLOADING		1
#define TFTP_DOWNLOADING	2
#define TFTP_UPDATING		3
#define TFTP_ERROR_FILE 	4
#define TFTP_BACKUPING		5
#define TFTP_UPLOADED		6

#define TFTP_MAX_WAIT 	1000
struct tftp_proc_t{
	u8 start;
	u8 opcode;
	u8 wait_time;
	u8 errorcode;
};

#define TELNET_SERVER_PORT 23

void telnetd_appcall(struct uip_conn *uip_conn);
void telnetd_init(void);


#define STATE_START	 		0
#define STATE_NORMAL 		1
#define STATE_IAC    		2
#define STATE_WILL   		3
#define STATE_WONT   		4
#define STATE_DO	     	5
#define STATE_DO_AHEAD     	6
#define STATE_DONT   		7
#define STATE_CLOSE  		8


#define TELNET_IAC   0xFF
#define TELNET_WILL  0xFB//fb
#define TELNET_WONT  0xFC//fc
#define TELNET_DO    0xFD//fd
#define TELNET_DONT  0xFE//fe

#define TELNET_ECHO	 1
#define TELNET_AHEAD 3

#define TELNET_TRANSMIT_BINARY	0




#ifndef TELNETD_CONF_LINELEN
#define TELNETD_CONF_LINELEN 80 //длина строки
#endif
#ifndef TELNETD_CONF_NUMLINES
#define TELNETD_CONF_NUMLINES 64//100//64 //число строк
#endif

#define TELNET_MAX_AUTH_FAIL 30
#define TELNET_FAIL_TIMEOUT	 3600 // 1 hour

#define TELNET_INPUT_TIMEOUT 300//5 минут

#define FDB_PAGE_ENTRIES 	 50//размер МАС таблицы // на один экран

#define TELNETD_USER_NUM	 2//максимальное число сессий//было 5

//память команд, для up, down
#define TELNETD_MEM_SIZE 	 5//размер

#define SHELL_QUEUE_LEN		 100//задач в очереди





//память команд, для up, down
struct telnetd_comm_mem_t{
	char line[TELNETD_MEM_SIZE][TELNETD_CONF_LINELEN];
	u8 linenum; //0 - текущая строка. 1-предыдущая команда, 2 - перед ней и т.д.
};
typedef struct{
	u8 cmd;
	u8 opt;
}telnet_options_t;

struct telnetd_state {
  char *lines[TELNETD_CONF_NUMLINES];
  char buf[TELNETD_CONF_LINELEN];
  char bufptr;
  u8 numsent;
  u8 state;
  u8 echo_flag;//echo on/off
  u8 up_pressed;//нажата кнопка вверх
  u8 entered_name;//имя введено корректно
  u8 entered_pass;//пароль введен корректно
  uip_ipaddr_t ip;//remote ip - net shell
  u8 usb;//usb shell
  u16 rport;//remote port
  u8 active; //флаг занятости
  u8 tab;//need autocompleat comand
  char username[20];
  char password[20];
  u8 user_rule;//права доступа текущего пользователя
  //структура
  struct telnetd_comm_mem_t telnetd_comm_mem;//история команд

  telnet_options_t options[10];
  u8 option_num;

  u8 button;//

  struct timer timeout;//таймаут на ввод данных,
  //если не было обращений в течении определённого времени,
  //то соединение рвётся принудительно
};




void sendopt(struct telnetd_state *s,u8_t option, u8_t value);
void sendchar(struct telnetd_state *s,char c);

void shell_push_command(struct telnetd_state *s);
void shell_pop_command_up(struct telnetd_state *s);
void shell_pop_command_down(struct telnetd_state *s);


#ifndef UIP_APPSTATE_SIZE
#define UIP_APPSTATE_SIZE (sizeof(struct telnetd_state))
#endif

#endif /* __TELNETD_H__ */
