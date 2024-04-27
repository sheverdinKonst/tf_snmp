/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * $Id: telnetd.c,v 1.2 2006/06/07 09:43:54 adam Exp $
 *
 */
#include "../deffines.h"

#if TELNET_USE

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../uip/uip.h"
#include "telnetd.h"
#include "memb2.h"
#include "shell.h"
#include "board.h"
#include "settings.h"
#include "SMIApi.h"
#include "vcp/usbd_cdc_vcp.h"
#include "usb_shell.h"
#include "debug.h"



//строки для вывода
struct telnetd_line {
	char line[TELNETD_CONF_LINELEN];
};




//extern char cmd_last[TELNETD_CONF_LINELEN];
extern char SHELL_PROMPT[32];
extern struct tftp_proc_t tftp_proc;

extern u8 start_ping_flag;
extern u8 send_ping_flag;
extern u8 start_show_fdb;
extern u8 start_show_vlan;

extern uint8_t SendICMPPingFlag;
extern uip_ipaddr_t IPDestPing;
extern struct timer ping_timer;

extern u8 start_vct;
extern u8 port_vct;

extern u8 start_make_bak;
extern u32 offset_make_bak;
extern u32 backup_file_len;

//extern char cmd_last[TELNETD_CONF_LINELEN];

//extern struct status_t status;

MEMB1(linemem, struct telnetd_line, TELNETD_CONF_NUMLINES);




struct telnetd_state s_[TELNETD_USER_NUM] __attribute__ ((section (".ccmram")));


//struct command_queue_t queue;




/*---------------------------------------------------------------------------*/
static char *
alloc_line(void) {
	//DEBUG_MSG(TELNET_DEBUG,"alloc_line\r\n");
	return memb2_alloc(&linemem);
}
/*---------------------------------------------------------------------------*/
static void dealloc_line(char *line) {
	//DEBUG_MSG(TELNET_DEBUG,"dealloc_line\r\n");
	memb2_free(&linemem, line);
}
/*---------------------------------------------------------------------------*/
void shell_quit(struct telnetd_state *s,char *str) {
	s->state = STATE_CLOSE;
	s->entered_name = 0;
	s->entered_pass = 0;
	s->active = 0;
	s->telnetd_comm_mem.linenum = 0;
	s->user_rule = NO_RULE;
	if(s->usb){
		usb_shell_task_stop();
	}
	s->usb = 0;
	for(u8 i=0;i<TELNETD_MEM_SIZE;i++)
		memset(s->telnetd_comm_mem.line[i],0,TELNETD_CONF_LINELEN);

}
/*---------------------------------------------------------------------------*/
static void sendline(struct telnetd_state *s, char *line) {
	static unsigned int i;
	//DEBUG_MSG(TELNET_DEBUG,"sendline %s\r\n", line);
	for (i = 0; i < TELNETD_CONF_NUMLINES; ++i) {
		if (s->lines[i] == NULL) {
			s->lines[i] = line;
			break;
		}
	}
	if (i == TELNETD_CONF_NUMLINES) {
		dealloc_line(line);
	}
}
/*---------------------------------------------------------------------------*/
void shell_prompt(struct telnetd_state *s,char *str) {
	char *line;

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_prompt %s\r\n", str);

	if(s->usb){
		VCP_DataTx((u8 *)str,strlen(str));
	}
	else{
		line = alloc_line();
		if (line != NULL) {
			strncpy(line, str, TELNETD_CONF_LINELEN);
			sendline(s,line);
		}
	}
}
/*---------------------------------------------------------------------------*/
void shell_output(struct telnetd_state *s,char *str1, char *str2) {
	static unsigned len;
	char *line;

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_output %s\r\n", str1);

	if(s->usb){
		printf("%s%s\r\n",str1,str2);
	}
	else{
		line = alloc_line();
		if (line != NULL) {
			len = strlen(str1);
			strncpy(line, str1, TELNETD_CONF_LINELEN);
			if (len < TELNETD_CONF_LINELEN) {
				strncpy(line + len, str2, TELNETD_CONF_LINELEN - len);
			}
			len = strlen(line);
			if (len < TELNETD_CONF_LINELEN - 2) {
				//todo
				if(get_telnet_rn() == TELNET_RN){
					line[len] = KEY_CR;
					line[len + 1] = KEY_NL;
					line[len + 2] = 0;
				}
				else{
					line[len] = KEY_NL;
					line[len + 1] = KEY_CR;
					line[len + 2] = 0;
				}
			}
			/*    petsciiconv_toascii(line, TELNETD_CONF_LINELEN);*/
			sendline(s,line);
		}
	}
}
/*---------------------------------------------------------------------------*/
void telnetd_init(void) {
	if (get_telnet_state() == 1) {
		uip_listen(HTONS(23));
		memb2_init(&linemem);
		for(u8 i=0;i<TELNETD_USER_NUM;i++)
			shell_init(&s_[i]);
	}
	else
		uip_unlisten(HTONS(23));
}
/*---------------------------------------------------------------------------*/
static void acked(struct telnetd_state *s) {
	static unsigned int i;

	//DEBUG_MSG(TELNET_DEBUG,"acked\r\n");

	while (s->numsent > 0) {
		dealloc_line(s->lines[0]);
		for (i = 1; i < TELNETD_CONF_NUMLINES; ++i) {
			s->lines[i - 1] = s->lines[i];
		}
		s->lines[TELNETD_CONF_NUMLINES - 1] = NULL;
		--s->numsent;
	}
}
/*---------------------------------------------------------------------------*/
static void senddata(struct telnetd_state *s) {
	static char *bufptr, *lineptr;
	static int buflen, linelen;

	//DEBUG_MSG(TELNET_DEBUG,"senddata\r\n");

	bufptr = uip_appdata;
	buflen = 0;
	for (s->numsent = 0;
			s->numsent < TELNETD_CONF_NUMLINES && s->lines[s->numsent] != NULL;
			++s->numsent) {
		lineptr = s->lines[s->numsent];
		linelen = strlen(lineptr);
		if (linelen > TELNETD_CONF_LINELEN) {
			linelen = TELNETD_CONF_LINELEN;
		}
		if (buflen + linelen < uip_mss()) {
			memcpy(bufptr, lineptr, linelen);
			bufptr += linelen;
			buflen += linelen;
		} else {
			break;
		}
	}
	uip_send(uip_appdata, buflen);
}
/*---------------------------------------------------------------------------*/
static void closed(struct telnetd_state *s) {

	//static unsigned int i;

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"closed\r\n");

	s->entered_name = 0;
	s->entered_pass = 0;

	s->active = 0;

	s->telnetd_comm_mem.linenum = 0;

	s->user_rule = NO_RULE;
	for(u8 i=0;i<TELNETD_MEM_SIZE;i++)
		memset(s->telnetd_comm_mem.line[i],0,TELNETD_CONF_LINELEN);

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"closed conn addr %d.%d.%d.%d\r\n",
					uip_ipaddr1(s->ip),uip_ipaddr2(s->ip),uip_ipaddr3(s->ip),uip_ipaddr4(s->ip));
}

void shell_push_command(struct telnetd_state *s){

	if(strlen(s->buf)<3)
		return;

	for(u8 i=TELNETD_MEM_SIZE-1;i>0;i--){
		strcpy(s->telnetd_comm_mem.line[i],s->telnetd_comm_mem.line[i-1]);
	}
	strcpy(s->telnetd_comm_mem.line[0],s->buf);
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_add_command:%s\r\n",s->buf);
	s->telnetd_comm_mem.linenum = 0;
}

void shell_pop_command_up(struct telnetd_state *s){

	strcpy(s->buf,s->telnetd_comm_mem.line[s->telnetd_comm_mem.linenum]);
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_pop_command (%d) %s\r\n",s->telnetd_comm_mem.linenum, s->buf);
	if(s->telnetd_comm_mem.linenum<(TELNETD_MEM_SIZE-1)){
		if(strlen(s->telnetd_comm_mem.line[s->telnetd_comm_mem.linenum+1]))
			s->telnetd_comm_mem.linenum++;
	}

}

void shell_pop_command_down(struct telnetd_state *s){
	strcpy(s->buf,s->telnetd_comm_mem.line[s->telnetd_comm_mem.linenum]);
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_pop_command (%d) %s\r\n",s->telnetd_comm_mem.linenum, s->buf);
	if(s->telnetd_comm_mem.linenum>0){
		if(strlen(s->telnetd_comm_mem.line[s->telnetd_comm_mem.linenum-1]))
			s->telnetd_comm_mem.linenum--;
	}
}
/*---------------------------------------------------------------------------*/
static void get_char(struct telnetd_state *s,u8_t c) {
	s->buf[(int) s->bufptr] = c;
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"get_char: 0x%02X\r\n", c);
	if(c==KEY_ESC || c == KEY_NL)
		return;

	timer_set(&s->timeout,TELNET_INPUT_TIMEOUT*1000*MSEC);

	//telnet echo - send char
	if(s->echo_flag == TELNET_ECHO && s->button == NO_KEY_PRESSED){
		if((isalnum(c))||(c=='.')||(c==',')||(c==' ')||(c=='_')||
				(c==':')||(c=='-')||(c=='/')||(c=='+')||(c=='*')||(c=='\\')||
				(c=='?')||(c=='=')||(c=='&')||(c=='!')||(c=='[')||(c=='{')||
				(c=='@')||(c=='#')||(c=='$')||(c=='%')||(c==']')||(c=='}')||
				(c=='(')||(c==')')||(c=='"')||(c=='<')||(c=='>')||(c=='^')){
			if((s->entered_name == 1)&&(s->entered_pass==0)){

			}
			else
				sendchar(s,c);
		}
	}


	//enter 0x0D
	if(s->buf[(int)s->bufptr] == KEY_CR){
		DEBUG_MSG(TELNET_DEBUG && !s->usb,"ENTER\r\n");
		if(s->echo_flag == TELNET_ECHO)
			shell_prompt(s,"\r\n");
		s->buf[(int)s->bufptr] = 0;
		if((s->entered_name == 1)&&(s->entered_pass==1))
			shell_push_command(s);//add to command history
		s->tab = 0;
		shell_input(s);
		s->bufptr = 0;
		return;
	}


	if((s->entered_name == 1)&&(s->entered_pass==1)){
		//up (0x5b + 0x41)
		if(s->button == KEY_PRESSED_UP){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"UP\r\n");
			s->buf[(int)s->bufptr] = 0;
			s->buf[(int)s->bufptr - 1] = 0;
			shell_pop_command_up(s);
			if(get_telnet_rn() == TELNET_RN)
				shell_prompt(s,"\r\n");
			promt_print(s);
			shell_prompt(s,s->buf);
			s->bufptr = strlen(s->buf);
			s->button = NO_KEY_PRESSED;
			return;
		}

		//down (0x5b + 0x42)
		if(s->button == KEY_PRESSED_DOWN){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"DOWN\r\n");
			s->buf[(int)s->bufptr] = 0;
			s->buf[(int)s->bufptr - 1] = 0;
			shell_pop_command_down(s);
			if(get_telnet_rn() == TELNET_RN)
				shell_prompt(s,"\r\n");
			promt_print(s);
			shell_prompt(s,s->buf);
			s->bufptr = strlen(s->buf);
			s->button = NO_KEY_PRESSED;
			return;
		}

		//left
		if((s->button == KEY_PRESSED_LEFT)&&(s->bufptr>0)){
			if(s->echo_flag == TELNET_ECHO){
				promt_print(s);
				shell_prompt(s,s->buf);
				shell_prompt(s,"\b");
			}
			else
				shell_prompt(s,"\b");
			s->button = NO_KEY_PRESSED;
		}


		//if backspase pressed
		if (s->button == KEY_PRESSED_BS) {
			if(s->bufptr>0){
				DEBUG_MSG(TELNET_DEBUG && !s->usb,"BS\r\n");
				s->bufptr--;
				s->buf[(int)s->bufptr] = 0;
				shell_prompt(s,"\r");
				promt_print(s);
				shell_prompt(s,s->buf);
				shell_prompt(s," \b");
			}
			else
				shell_prompt(s," ");
			s->button = NO_KEY_PRESSED;
			return;
		}

		if((s->button == KEY_PRESSED_TAB) && (s->bufptr>0)){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_TAB:%s\r\n",s->buf);
			s->buf[(int) s->bufptr] = 0;
			s->tab = 1;
			shell_input(s);
			s->button = NO_KEY_PRESSED;
			return;
		}
	}


	//если ничего не выбрано
	s->bufptr++;
}

/*---------------------------------------------------------------------------*/
void sendopt(struct telnetd_state *s,u8_t option, u8_t value) {
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"sendopt %X %X\r\n", option, value);

	char *line;
	line = alloc_line();
	if (line != NULL) {
		line[0] = TELNET_IAC;
		line[1] = option;
		line[2] = value;
		line[3] = 0;
		sendline(s,line);
	}
}

//send char
void sendchar(struct telnetd_state *s,char c) {
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"sendchar %c\r\n",c);
	char *line;

	if(s->usb){
		VCP_DataTx((u8 *)&c,1);
	}
	else{
		line = alloc_line();
		if (line != NULL) {
			line[0] = c;
			line[1] = 0;
			sendline(s,line);
		}
	}
}

//получаем опции
static u16 get_telnet_option(struct telnetd_state *s, u8 *buff,u16 len){
u16 ptr;
u16 len1;
u8 c;
u8 *buff2;

	buff2 = buff;
	len1 = len;

	//printf_arr(TYPE_HEX,buff,len);
	s->option_num = 0;
	while (len1 > 0 && s->bufptr < sizeof(s->buf)) {
		c = *buff2;
		if(c == TELNET_IAC){
			ptr = 1;
		}
		else if(ptr == 1 && (c == TELNET_DO || c == TELNET_WILL)){
			ptr = 2;
			s->options[s->option_num].cmd = c;
		}
		else if(c != TELNET_IAC && c != TELNET_DO && c != TELNET_WILL && ptr == 2){
			ptr = 0;
			s->options[s->option_num].opt = c;
			s->option_num++;
		}
		else
		{
			//если последовательность сбилась, значит опции закончились, и идут даные,
			//возвращаем указатель на них
			return len - len1;
		}
		++buff2;
		--len1;
	}

	return len - len1;//возвращаем длину
}

static void get_telnet_key(struct telnetd_state *s, u8 *buff,u16 len){
u16 ptr;
u8 tmp[5];
u16 len1;
u8 c;
u8 *buff2;
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"get_telnet_key\r\n");
	buff2 = buff;
	len1 = len;
	//printf_arr(TYPE_HEX,buff,len);
	s->option_num = 0;
	tmp[0] = 0;
	ptr = 0;

	while (len1 > 0 && s->bufptr < sizeof(s->buf) && ptr<sizeof(tmp)) {
		c = *buff2;
		if(ptr == 0 && c == KEY_UP){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_UP\r\n");
			s->button = KEY_PRESSED_UP;
			ptr++;
			break;
		}

		if(ptr == 0 && c == KEY_DOWN){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_DOWN\r\n");
			s->button = KEY_PRESSED_DOWN;
			ptr++;
			break;
		}

		if(ptr == 0 && c == 0x1B){
			tmp[0] = c;
			ptr++;
		}

		if(ptr == 1 && c == KEY_UP_5B && tmp[0]==0x1B){
			tmp[1] = c;
			ptr++;
		}

		if(ptr == 2 && c == KEY_UP_41 && tmp[1] == KEY_UP_5B && tmp[0]==0x1B){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_UP\r\n");
			tmp[2] = c;
			s->button = KEY_PRESSED_UP;
			ptr++;
			break;
		}
		if(ptr == 2 && c == KEY_DOWN_42 && tmp[1] == KEY_UP_5B && tmp[0]==0x1B){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_DOWN\r\n");
			s->button = KEY_PRESSED_DOWN;
			ptr++;
			break;
		}
		if(ptr == 2 && c == KEY_LEFT_44 && tmp[1] == KEY_UP_5B && tmp[0]==0x1B){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_LEFT\r\n");
			s->button = KEY_PRESSED_LEFT;
			ptr++;
			break;
		}

		if(ptr == 2 && c == KEY_RIGHT_43 && tmp[1] == KEY_UP_5B && tmp[0]==0x1B){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_RIGHT\r\n");
			s->button = KEY_PRESSED_RIGHT;
			ptr++;
			break;
		}

		if(ptr == 0 && (c == KEY_BS || c == KEY_BS2)){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_BS\r\n");
			s->button = KEY_PRESSED_BS;
			ptr++;
			break;
		}
		if(ptr == 0 && c == KEY_TAB){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_TAB\r\n");
			s->button = KEY_PRESSED_TAB;
			ptr++;
			break;
		}

		if(ptr == 0 && c == KEY_CTRLC){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"KEY_PRESSED_CTRLC\r\n");
			s->button = KEY_PRESSED_CTRLC;
			ptr++;
			break;
		}

		++buff2;
		--len1;
	}

	if(ptr){
		for(u8 i=1;i<=ptr;i++){
			*(buff2+i-1) = ' ';
		}
	}
	else{
	//если ничего не нажато
		s->button = NO_KEY_PRESSED;
	}
}

/*---------------------------------------------------------------------------*/
static void newdata(struct telnetd_state *s) {
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"newdata()\r\n");

	u16 len;
	u16 len_opt;
	u8 	c;
	u8 	*dataptr;

	len = uip_datalen();
	dataptr = (u8 *)uip_appdata;



	if(s->state == STATE_START){
		if(get_telnet_echo()){
			s->state = STATE_DO_AHEAD;
			sendopt(s,TELNET_DO,TELNET_AHEAD);
			return;
		}
		else{
			s->state = STATE_NORMAL;
			shell_start(s);
		}
	}

	//find option
	len_opt = get_telnet_option(s,dataptr,len);
	len -=len_opt;
	if(s->state == STATE_START || s->state == STATE_DO_AHEAD){
		for(u8 i=0;i<s->option_num;i++){
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"OPTION %X, %X\r\n",s->options[i].cmd,s->options[i].opt);
		}
		//если есть опции, отвечаем на них
		if(s->option_num){
			for(u8 i=0;i<s->option_num;i++){
				if(s->options[i].cmd == TELNET_DO && s->options[i].opt == TELNET_AHEAD){
					sendopt(s,TELNET_WILL, TELNET_AHEAD);
				}
				if(s->options[i].cmd == TELNET_DO && s->options[i].opt == TELNET_ECHO){
					sendopt(s,TELNET_WILL, TELNET_ECHO);
					s->echo_flag = 1;
				}
				if(s->options[i].cmd == TELNET_WILL){
					if(s->options[i].opt == TELNET_AHEAD)
						//sendopt(s,TELNET_DO, TELNET_AHEAD);
						sendopt(s,TELNET_WILL, TELNET_AHEAD);
					else
						sendopt(s,TELNET_DONT, s->options[i].opt);
				}
			}
			s->option_num = 0;
			shell_start(s);
			s->state = STATE_NORMAL;
			return;
		}
	}

	if(s->state == STATE_NORMAL){
		//если было нажатие на кнопки
		get_telnet_key(s,dataptr,len);
	}

	while (len > 0 && s->bufptr < sizeof(s->buf)) {
		c = *dataptr;
		++dataptr;
		--len;
		if(s->state == STATE_NORMAL){
			get_char(s,c);
		}

	}
}






static struct telnetd_state *alloc_telnetd_state(struct uip_conn *conn){
	for(u8 i=0;i<TELNETD_USER_NUM;i++){
		if(s_[i].active){
			if(uip_ipaddr_cmp(conn->ripaddr,s_[i].ip)){
				if(conn->rport == s_[i].rport)
					return &s_[i];
			}
		}
	}

	for(u8 i=0;i<TELNETD_USER_NUM;i++){
		if(s_[i].active == 0){
			//если запись не нашли, но нашли свободное место
			uip_ipaddr_copy(s_[i].ip,conn->ripaddr);
			s_[i].rport = conn->rport;
			s_[i].active = 1;
			DEBUG_MSG(TELNET_DEBUG,"create addr %d.%d.%d.%d port:%d\r\n",
				uip_ipaddr1(conn->ripaddr),uip_ipaddr2(conn->ripaddr),uip_ipaddr3(conn->ripaddr),uip_ipaddr4(conn->ripaddr),
				conn->rport);

			timer_set(&s_[i].timeout,TELNET_INPUT_TIMEOUT*1000*MSEC);
			return &s_[i];
		}
	}
	return NULL;
}

/*---------------------------------------------------------------------------*/
void telnetd_appcall(struct uip_conn *uip_conn) {
char temp[256];
struct telnetd_state *s;

	#if(TELNET_DEBUG)
	extern u8 test_processing_flag;
	if(test_processing_flag==7){
		DEBUG_MSG(TELNET_DEBUG,"TELNET CONNECTION:%d uip_close\r\n",htons(uip_conn->lport));
		test_processing_flag = 0;
		uip_close();
		//return;
	}
	#endif


	//выделяем место для IP
	s = alloc_telnetd_state(uip_conn);
	if(s == NULL){
		s->state = STATE_CLOSE;
		//закрываем сессию, если нет места
		return;
	}



	static unsigned int i;

	if(get_telnet_state() != 1)
		return;

	if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
		return;


	if (uip_connected()) {
		//set prompr line
		get_dev_name_r(SHELL_PROMPT);
		DEBUG_MSG(TELNET_DEBUG && !s->usb,"uip_connected()\r\n");
		s->state = STATE_START;
		for (i = 0; i < TELNETD_CONF_NUMLINES; ++i) {
			s->lines[i] = NULL;
		}
		s->bufptr = 0;

		if(get_telnet_echo()){
			s->state = STATE_DO_AHEAD;
			sendopt(s,TELNET_DO,TELNET_AHEAD);
			return;
		}
		else{
			s->state = STATE_NORMAL;
			shell_start(s);

		}
	}

	if (s->state == STATE_CLOSE) {
		DEBUG_MSG(TELNET_DEBUG && !s->usb,"STATE_CLOSE\r\n");
		s->state = STATE_NORMAL;
		uip_close();
		return;
	}

	if (uip_closed() || uip_aborted() || uip_timedout()) {
		if(uip_closed())
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"uip_closed\r\n");
		if(uip_aborted())
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"uip_aborted\r\n");
		if(uip_timedout())
			DEBUG_MSG(TELNET_DEBUG && !s->usb,"uip_timedout\r\n");
		closed(s);
	}

	if (uip_acked()) {
		acked(s);
	}

	if (uip_newdata()) {
		newdata(s);
	}

	if (uip_rexmit() || uip_newdata() || uip_acked() || uip_connected()
			|| uip_poll()) {
		senddata(s);
	}

	if(timer_expired(&s->timeout)){
		uip_close();
	}



	//for VCT
	if(start_vct){
		if(get_cable_test()==0){
			shell_prompt(s,"\r\nPort\tType\tLink Status\tTest Result\tCable Length (M)\r\n");
			shell_prompt(s,"----\t----\t----------\t-----------\t----------------\r\n");
			sprintf(temp,"%d\tFE\t",port_vct+1);
			if(get_port_link(port_vct))
				strcat(temp,"Link Up\t");
			else
				strcat(temp,"Link Down");
			shell_prompt(s,temp);
			sprintf(temp,"\tpair 1-2: ");
			switch(dev.port_stat[port_vct].rx_status){
				case VCT_BAD:
					strcat(temp,"Bad test\t");
					break;
				case VCT_SHORT:
					strcat(temp,"Short\t");
					break;
				case VCT_OPEN:
					strcat(temp,"Open\t");
					break;
				case VCT_GOOD:
					strcat(temp,"Good\t");
					break;
				default:
					strcat(temp,"Bad test\t");
					break;
			}
			shell_prompt(s,temp);
			if((dev.port_stat[port_vct].rx_status==VCT_OPEN)||
					(dev.port_stat[port_vct].rx_status==VCT_SHORT)){
				sprintf(temp,"%d M",dev.port_stat[port_vct].rx_len);
				shell_prompt(s,temp);
			}

			sprintf(temp,"\r\n\t\t\t\tpair 3-6: ");
			switch(dev.port_stat[port_vct].tx_status){
				case VCT_BAD:
					strcat(temp,"Bad test\t");
					break;
				case VCT_SHORT:
					strcat(temp,"Short\t");
					break;
				case VCT_OPEN:
					strcat(temp,"Open\t");
					break;
				case VCT_GOOD:
					strcat(temp,"Good\t");
					break;
				default:
					strcat(temp,"Bad test\t");
					break;
			}
			shell_prompt(s,temp);
			if((dev.port_stat[port_vct].tx_status==VCT_OPEN)||
					(dev.port_stat[port_vct].rx_status==VCT_SHORT)){
				sprintf(temp,"%d M",dev.port_stat[port_vct].tx_len);
				shell_prompt(s,temp);
			}
			shell_prompt(s,"\r\n");
			shell_prompt(s,SHELL_PROMPT);
			start_vct = 0;
		}
		else{
			shell_prompt(s,".");
		}
	}

	//для генерации файла бекапа
	if(start_make_bak){


		memset(temp,0,sizeof(temp));
		spi_flash_read(offset_make_bak,TELNETD_CONF_LINELEN,temp);
		shell_output(s,temp,"");
		offset_make_bak+=TELNETD_CONF_LINELEN;
		if(offset_make_bak>=backup_file_len){
			start_make_bak = 0;
			offset_make_bak = 0;
			backup_file_len = 0;
			shell_prompt(s,"\r\n");
			shell_prompt(s,SHELL_PROMPT);
		}
	}

	//for ping processing
	if(start_ping_flag){
		if(timer_expired(&ping_timer)){
			sprintf(temp,"Ping statistics for %d.%d.%d.%d\r\n",
					(int)uip_ipaddr1(IPDestPing),(int)uip_ipaddr2(IPDestPing),(int)uip_ipaddr3(IPDestPing),(int)uip_ipaddr4(IPDestPing));
			shell_prompt(s,temp);
			shell_prompt(s,"Packets: Sent = 4 ");
			sprintf(temp,"Received = %d, Lost = %d (%d %% loss)",SendICMPPingFlag,4-SendICMPPingFlag,(int)(((4-SendICMPPingFlag)*100)/4));
			shell_prompt(s,temp);
			shell_prompt(s,"\r\n");
			promt_print(s);
			SendICMPPingFlag = 0;
			start_ping_flag = 0;
		}else{
			if((SendICMPPingFlag != send_ping_flag)&&(SendICMPPingFlag > send_ping_flag)){
				send_ping_flag = SendICMPPingFlag;
				sprintf(temp,"\r\nReply from %d.%d.%d.%d bytes=32 seq=%d",
		  		  uip_ipaddr1(&IPDestPing),uip_ipaddr2(&IPDestPing),uip_ipaddr3(&IPDestPing),uip_ipaddr4(&IPDestPing),
		  		  SendICMPPingFlag);
				shell_prompt(s,temp);

				if(SendICMPPingFlag>=4){
					sprintf(temp,"\r\nPing statistics for %d.%d.%d.%d\r\n",
							(int)uip_ipaddr1(IPDestPing),(int)uip_ipaddr2(IPDestPing),(int)uip_ipaddr3(IPDestPing),(int)uip_ipaddr4(IPDestPing));
					shell_prompt(s,temp);
					shell_prompt(s,"Packets: Sent = 4 ");
					sprintf(temp,"Received = %d, Lost = %d (%d %% loss)",SendICMPPingFlag,4-SendICMPPingFlag,(int)(((4-SendICMPPingFlag)*100)/4));
					shell_prompt(s,temp);
					shell_prompt(s,"\r\n");
					promt_print(s);
					start_ping_flag = 0;
				}
			}
		}
	}

	if(start_show_fdb){
		show_fdb(s);

	}

	if(start_show_vlan){
		show_vlan_all(s);
	}




	if(tftp_proc.start){
		if(tftp_proc.wait_time<TFTP_MAX_WAIT){
			switch(tftp_proc.opcode){
				case TFTP_IDLE:
					break;

				case TFTP_UPLOADING:
					if(tftp_proc.wait_time==0){
						shell_prompt(s,"\r\nUploading");
					}
					if(tftp_proc.wait_time%50 == 0){
						shell_prompt(s,".");
					}
					break;

				case TFTP_DOWNLOADING:
					if(tftp_proc.wait_time==0){
						shell_prompt(s,"\r\nDownloading");
					}
					if(tftp_proc.wait_time%50 == 0){
						shell_prompt(s,".");
					}
					break;

				case TFTP_UPDATING:
					DEBUG_MSG(TELNET_DEBUG && !s->usb,"TFTP_UPDATING\r\n");
					tftp_proc.start = 0;
					tftp_proc.wait_time = 0;

					if(ntwk_wait_and_do)
						s->state = STATE_CLOSE;//close connection
					break;

				case TFTP_ERROR_FILE:
					shell_prompt(s,"\r\nFile is incorrect");
					tftp_proc.start = 0;
					tftp_proc.wait_time = 0;
					break;

				case TFTP_BACKUPING:
					shell_prompt(s,"\r\nDownloading compleat.");
					shell_prompt(s,"\r\nRecovery config from file, wait, reboot...");
					tftp_proc.start = 0;
					tftp_proc.wait_time = 0;
					break;

				case TFTP_UPLOADED:
					shell_prompt(s,"\r\nUploaded compleat.");
					shell_prompt(s,"\r\nRecovery config from file, wait, reboot...");
					tftp_proc.start = 0;
					tftp_proc.wait_time = 0;
					break;
			}
			tftp_proc.wait_time++;
		}
		else{
			tftp_proc.start = 0;
			tftp_proc.wait_time = 0;
		}
	}
}
/*---------------------------------------------------------------------------*/

#endif
