#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "shell.h"
#include "settings.h"
#include "telnetd.h"
#include "usb_shell.h"
#include "vcp/usbd_cdc_vcp.h"
#include "task.h"
#include "semphr.h"
#include "selftest.h"
#include "debug.h"

struct telnetd_state *s;
u8 usb_shell_state_;

extern struct telnetd_state s_[TELNETD_USER_NUM];

static struct telnetd_state *alloc_telnetd_state();

xQueueHandle UsbShellQueue;


void usb_shell_init(void) {
	//memb2_init(&linemem);
	for(u8 i=0;i<TELNETD_USER_NUM;i++)
		shell_init(&s_[i]);
}

void set_char(char c){
	s->buf[(int) s->bufptr] = c;

	if(c==KEY_ESC || c == KEY_NL)
		return;

	//enter 0x0D
	if(s->buf[(int)s->bufptr] == KEY_CR || s->buf[(int)s->bufptr] == KEY_NL){
		DEBUG_MSG(TELNET_DEBUG,"ENTER\r\n");
		printf("\r\n");
		s->buf[(int)s->bufptr] = 0;
		if((s->entered_name == 1)&&(s->entered_pass==1))
			shell_push_command(s);//add to command history
		s->tab = 0;
		shell_input(s);
		s->bufptr = 0;
		return;
	}



	if((s->entered_name == 1)&&(s->entered_pass==1)){
		if(s->buf[(int) s->bufptr] == KEY_UP){
			DEBUG_MSG(TELNET_DEBUG,"UP\r\n");
			s->buf[(int)s->bufptr] = 0;
			shell_pop_command_up(s);
			shell_prompt(s,"\r\n");
			promt_print(s);
			shell_prompt(s,s->buf);
			s->bufptr = strlen(s->buf);
			return;
		}



		//down (0x27)
		if(s->buf[(int) s->bufptr] == KEY_DOWN){
			DEBUG_MSG(TELNET_DEBUG,"DOWN\r\n");
			s->buf[(int)s->bufptr] = 0;
			shell_pop_command_down(s);
			shell_prompt(s,"\r\n");
			promt_print(s);
			shell_prompt(s,s->buf);
			s->bufptr = strlen(s->buf);
			return;
		}


		//if backspase pressed
		if (c == KEY_BS) {
			if(s->bufptr>0){
				DEBUG_MSG(TELNET_DEBUG,"BS\r\n");
				s->bufptr--;
				s->buf[(int)s->bufptr] = 0;
				shell_prompt(s,"\r");
				promt_print(s);
				shell_prompt(s,s->buf);
				shell_prompt(s," \b");
			}
			else
				shell_prompt(s," ");

			return;
		}

		if((s->buf[(int) s->bufptr] == KEY_TAB) && (s->bufptr>0)){
			DEBUG_MSG(TELNET_DEBUG,"KEY_TAB:%s\r\n",s->buf);
			s->buf[(int) s->bufptr] = 0;
			s->tab = 1;
			shell_input(s);
			return;
		}
	}




	//telnet echo - send char
	//ввод пароля
	if((s->entered_name == 1)&&(s->entered_pass==0)){
		if(c != KEY_CR && c != KEY_NL ){
			c = '*';
			sendchar(s,c);
		}
	}else{
		if(c!=KEY_DOWN_5B && c!=KEY_UP_5B && c!=KEY_CR && c!=KEY_TAB){
			sendchar(s,c);
		}
	}

	//если ничего не выбрано
	s->bufptr++;
}

void usb_shell_task(void ){
char c;
	if(UsbShellQueue == NULL)
		return;

	if(xQueueReceive(UsbShellQueue,&c,0) == pdPASS ){
		set_char(c);
	}
}

//запуск usb консоли
void usb_shell_task_start(void){
	usb_shell_state_ = 1;
	usb_shell_init();
	if(UsbShellQueue == NULL)
		UsbShellQueue = xQueueCreate(5,1);
	s = alloc_telnetd_state();
}

void usb_shell_task_stop(void){
	usb_shell_state_ = 0;
}


static struct telnetd_state *alloc_telnetd_state(){
	for(u8 i=0;i<TELNETD_USER_NUM;i++){
		if(s_[i].active){
			if(s_[i].usb){
				return &s_[i];
			}
		}
	}
	for(u8 i=0;i<TELNETD_USER_NUM;i++){
		if(s_[i].active == 0){
			//если запись не нашли, но нашли свободное место
			s_[i].usb = 1;
			s_[i].active = 1;
			shell_start(&s_[i]);
			return &s_[i];
		}
	}
	return NULL;
}

u8 usb_shell_state(void){
	return usb_shell_state_;
}
