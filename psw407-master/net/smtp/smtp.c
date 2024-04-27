/**
 * \addtogroup exampleapps
 * @{
 */

/**
 * \defgroup smtp SMTP E-mail sender
 * @{
 *
 * The Simple Mail Transfer Protocol (SMTP) as defined by RFC821 is
 * the standard way of sending and transfering e-mail on the
 * Internet. This simple example implementation is intended as an
 * example of how to implement protocols in uIP, and is able to send
 * out e-mail but has not been extensively tested.
 */

/**
 * \file
 * SMTP example implementation
 * \author Adam Dunkels <adam@dunkels.com>
 */

/*
 * Copyright (c) 2002, Adam Dunkels.
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
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: smtp.c,v 1.1.2.7 2003/10/07 13:47:50 adam Exp $
 *
 */


#include <stdio.h>
#include <string.h>


#include "../uip/uip.h"
#include "task.h"
#include "semphr.h"
#include "../syslog/syslog.h"
#include "../events/events_handler.h"
#include "../dns/resolv.h"
#include "smtp.h"
#include "smtp-strings.h"
#include "../webserver/httpd.h"
#include "../uip/timer.h"
#include "board.h"
#include "names.h"
#include "selftest.h"
#include "settings.h"
#include "debug.h"

#define STATE_SEND_NONE         0
#define STATE_SEND_HELO         1
#define STATE_SEND_MAIL_FROM    2
#define STATE_SEND_RCPT_TO      3
#define STATE_SEND_DATA         4
#define STATE_SEND_DATA_HEADERS 5
#define STATE_SEND_DATA_MESSAGE 6
#define STATE_SEND_DATA_END     7
#define STATE_SEND_QUIT         8
#define STATE_SEND_DONE         9


static char *localhostname;
static uip_ipaddr_t smtpserver;

//флаг готовности к отправке
//1 - в данный момент ничего не отправляется и готовы отправить
//0 - пока занятыи и нужно подождать
u8 email_ready=0;

xTaskHandle xEMailTask;//задача отправки сообщений
xSemaphoreHandle xEmailSemaphore;

//очередь на отправку smtp сообщений
xQueueHandle SmtpQueue;

struct timer smtp_timeout_timer;

//timout 4 resolve dns server name
struct timer dns_timer;

u8 EmailSemaphore=0;

//char mail_text[512+256];
char mail_text[MESSAGE_LEN];

struct uip_conn *conn;

struct smtp_state st;
static struct smtp_state *s=&st;
//struct httpd_state *s;

#define ISO_nl 0x0a
#define ISO_cr 0x0d
#define ISO_el 0

#define ISO_period 0x2e

#define ISO_2  0x32
#define ISO_3  0x33
#define ISO_4  0x34
#define ISO_5  0x35

/*---------------------------------------------------------------------------*/
static PT_THREAD(smtp_thread(void)){

static char *ptr;
char tempstr[128];
char tempstr2[64];
static u8 auth=0,word_auth=0,word_login=0,word_plain=0;
static u16 first_len=0,curr_len=0;



DEBUG_MSG(SMTP_DEBUG,"smtp_thread\r\n");

PSOCK_BEGIN(&s->sout);

PSOCK_READTO(&s->sout, ISO_nl);

DEBUG_MSG(SMTP_DEBUG,s->inputbuf);
DEBUG_MSG(SMTP_DEBUG,"\r\n");

if(strncmp(&s->inputbuf[0], smtp_220, 3) != 0){
	PSOCK_CLOSE(&s->sout);
	smtp_done(2);
	PSOCK_EXIT(&s->sout);
}

/*check if auth enabled*/

if(strlen(s->login)){
	if(strlen(s->pass))
		auth=1;
	else{
		DEBUG_MSG(SMTP_DEBUG,"1password :%s\r\n",s->pass);
		PSOCK_CLOSE(&s.psock);
		smtp_done(9);
		PSOCK_EXIT(&s->sout);
	}
}
else{
	auth=0;
}



if(auth){
	PSOCK_SEND_STR(&s->sout, (char *)smtp_ehlo);
	DEBUG_MSG(SMTP_DEBUG,"%s\r\n",smtp_ehlo);
}
else{
	PSOCK_SEND_STR(&s->sout, (char *)smtp_helo);
	DEBUG_MSG(SMTP_DEBUG,"%s\r\n",smtp_helo);
}


PSOCK_SEND_STR(&s->sout, /*s->login*/"psw");
DEBUG_MSG(SMTP_DEBUG,/*s->login*/"psw");
DEBUG_MSG(SMTP_DEBUG,"\r\n");

PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
//SMTP_DBG(smtp_crnl);
DEBUG_MSG(SMTP_DEBUG,"\r\n");






if(auth){
	/*принято ?*/
	/*получаем список методов*/
	DEBUG_MSG(SMTP_DEBUG,"get metods:\r\n");

auth=0;



while(1){

	DEBUG_MSG(SMTP_DEBUG,"hop \r\n");
	memset(s->inputbuf,0,512);

	if(auth==0){
		PSOCK_READTO(&s->sout, ISO_nl);
		first_len=uip_datalen();//get all str len
		curr_len=0;
		auth++;
	}



	if(curr_len>=first_len){

		//если приняли все аргументы
		if(((word_auth==1))&&((word_login==1)||(word_plain==1)))
			break;
		else{
			if(word_auth == 0){
				PSOCK_CLOSE(&s->sout);
				smtp_done(11);
				PSOCK_EXIT(&s->sout);
			}
			if((word_login == 0)&&(word_plain == 0)){
				PSOCK_CLOSE(&s->sout);
				smtp_done(12);
				PSOCK_EXIT(&s->sout);
			}
		}
	}
	else{
		if((auth!=0)&&(auth!=1))
			PSOCK_READTO(&s->sout, ISO_nl);
		curr_len+=strlen(s->inputbuf);//get len of current string

		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);

		//SMTP_DBG("uip len %d",first_len);
		//SMTP_DBG("str len %d",strlen(s->inputbuf));
		//SMTP_DBG("\r\n");

		if(strncmp(&s->inputbuf[0], smtp_250, 3) == 0) {
			//if 220
			//search "AUTH"
			strcpy(tempstr,s->inputbuf);

			ptr=strstr(tempstr,smtp_auth);
			if(ptr!=NULL){
				word_auth=1;
				DEBUG_MSG(SMTP_DEBUG,"AUTH finded\r\n");
			}
			//search LOGIN//
			ptr=strstr(tempstr,smtp_login);
			if(ptr!=NULL){
				word_login=1;
				DEBUG_MSG(SMTP_DEBUG,"LOGIN finded\r\n");
			}

			//search PLAIN//
			ptr=strstr(tempstr,smtp_plain);
			if(ptr!=NULL){
				word_plain=1;
				DEBUG_MSG(SMTP_DEBUG,"PLAIN finded\r\n");
			}

		}
		else
		{
				PSOCK_CLOSE(&s->sout);
				smtp_done(10);
				PSOCK_EXIT(&s->sout);
		}
	}
	auth++;



}

	/*autentification*/
	/*if login & plain -> use login*/
	if(word_login == 1){

		/*отправляем auth login*/
		PSOCK_SEND_STR(&s->sout,(char *)smtp_auth_login);

		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",smtp_auth_login);

		PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);

		PSOCK_READTO(&s->sout, ISO_nl);

		/*если получили "334 VXNlcm5hbWU6" - запрос Username   */
		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);

		if(strncmp(&s->inputbuf[0], smtp_334_username, 16) != 0) {
			PSOCK_CLOSE(&s->sout);
			smtp_done(13);
			PSOCK_EXIT(&s->sout);
		}

		/*передаем username*/
		encode64(s->login, tempstr);
		PSOCK_SEND_STR(&s->sout, (char *)tempstr);
		PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);

		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",tempstr);

		PSOCK_READTO(&s->sout, ISO_nl);

		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);

		/*если получли 334 password*/
		if(strncmp(&s->inputbuf[0], smtp_334_password, 16) != 0) {
			PSOCK_CLOSE(&s->sout);
			smtp_done(14);
			PSOCK_EXIT(&s->sout);
		}

		/*передаем password*/
		encode64(s->pass, tempstr);
		PSOCK_SEND_STR(&s->sout, (char *)tempstr);
		PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
		DEBUG_MSG(SMTP_DEBUG,"passw :%s",tempstr);
		DEBUG_MSG(SMTP_DEBUG,"\r\n");

		PSOCK_READTO(&s->sout, ISO_nl);
		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);

		DEBUG_MSG(SMTP_DEBUG,"1password :%s\r\n",s->pass);
		/*если 235 то все ок*/
		if(strncmp(&s->inputbuf[0], smtp_235, 3) != 0) {
			PSOCK_CLOSE(&s->sout);
			smtp_done(15);
			PSOCK_EXIT(&s->sout);
		}

	/*если только plain*/
	}else if(word_plain == 1){
		/*отправляем auth plain*/
		PSOCK_SEND_STR(&s->sout,(char *)smtp_auth_plain);
		PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);

		DEBUG_MSG(SMTP_DEBUG,"%s \r\n",smtp_auth_plain);

		PSOCK_READTO(&s->sout, ISO_nl);

		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);

		/*если получли 334*/
		if(strncmp(&s->inputbuf[0], smtp_334, 3) != 0) {
			PSOCK_CLOSE(&s->sout);
			smtp_done(16);
			PSOCK_EXIT(&s->sout);
		}

		/*передаем username + pass*/

		tempstr2[0]='\0';
		for(u8 i=0;i<=strlen(s->login);i++)
			tempstr2[i+1]=s->login[i];
		tempstr2[strlen(s->login)+1]='\0';
		for(u8 i=0;i<=strlen(s->pass); i++)
			tempstr2[i+strlen(s->login)+2]=s->pass[i];

		encode64len(tempstr2,tempstr,(strlen(s->login)+strlen(s->pass)+2));

		PSOCK_SEND_STR(&s->sout, (char *)tempstr);
		PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",tempstr);

		PSOCK_READTO(&s->sout, ISO_nl);
		DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);

		/*если 235 то все ок*/
		if(strncmp(&s->inputbuf[0], smtp_235, 3) != 0) {
			PSOCK_CLOSE(&s->sout);
			smtp_done(16);
			PSOCK_EXIT(&s->sout);
		}
	}

	/*переходим к обычной работе*/
}

else{
	if(s->inputbuf[0] != ISO_2) {
		PSOCK_CLOSE(&s.psock);
		smtp_done(3);
		PSOCK_EXIT(&s->sout);
	}
}


PSOCK_SEND_STR(&s->sout,(char *)smtp_mail_from);
DEBUG_MSG(SMTP_DEBUG,smtp_mail_from);
DEBUG_MSG(SMTP_DEBUG,"\r\n");

PSOCK_SEND_STR(&s->sout, (char *)smtp_left_sk);//"<"
PSOCK_SEND_STR(&s->sout, s->from);
PSOCK_SEND_STR(&s->sout, (char *)smtp_right_sk);//">"

DEBUG_MSG(SMTP_DEBUG,s->from);
DEBUG_MSG(SMTP_DEBUG,"\r\n");


PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);

PSOCK_READTO(&s->sout, ISO_nl);

DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);


if(s->inputbuf[0] != ISO_2) {
	PSOCK_CLOSE(&s->sout);
	smtp_done(4);
	PSOCK_EXIT(&s->sout);
}


PSOCK_SEND_STR(&s->sout, (char *)smtp_rcpt_to);
DEBUG_MSG(SMTP_DEBUG,smtp_rcpt_to);

PSOCK_SEND_STR(&s->sout, (char *)smtp_left_sk);//"<"
PSOCK_SEND_STR(&s->sout, s->to);
PSOCK_SEND_STR(&s->sout, (char *)smtp_right_sk);//">"

DEBUG_MSG(SMTP_DEBUG,s->to);
DEBUG_MSG(SMTP_DEBUG,"\r\n");
PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);


PSOCK_READTO(&s->sout, ISO_nl);

DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);
//SMTP_DBG("\r\n");

if(s->inputbuf[0] != ISO_2) {
	PSOCK_CLOSE(&s->sout);
	smtp_done(5);
	PSOCK_EXIT(&s->sout);
}


if(strlen(s->to2)>1){
	PSOCK_SEND_STR(&s->sout, (char *)smtp_rcpt_to);
	PSOCK_SEND_STR(&s->sout, (char *)smtp_left_sk);//"<"
	PSOCK_SEND_STR(&s->sout, s->to2);
	PSOCK_SEND_STR(&s->sout, (char *)smtp_right_sk);//">"
	PSOCK_SEND_STR(&s->sout, (char *)smtp_nl);
	PSOCK_READTO(&s->sout, ISO_nl);
	DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);
	if(s->inputbuf[0] != ISO_2) {
		PSOCK_CLOSE(&s->sout);
		smtp_done(6);
		PSOCK_EXIT(&s->sout);
	}
}
if(strlen(s->to3)>1){
	PSOCK_SEND_STR(&s->sout, (char *)smtp_rcpt_to);
	PSOCK_SEND_STR(&s->sout, (char *)smtp_left_sk);//"<"
	PSOCK_SEND_STR(&s->sout, s->to3);
	PSOCK_SEND_STR(&s->sout, (char *)smtp_right_sk);//">"
	PSOCK_SEND_STR(&s->sout, (char *)smtp_nl);
	PSOCK_READTO(&s->sout, ISO_nl);
	DEBUG_MSG(SMTP_DEBUG,"%s\r\n",s->inputbuf);
	if(s->inputbuf[0] != ISO_2) {
		PSOCK_CLOSE(&s->sout);
		smtp_done(6);
		PSOCK_EXIT(&s->sout);
	}
}


PSOCK_SEND_STR(&s->sout, (char *)smtp_data);
PSOCK_READTO(&s->sout, ISO_nl);

if(s->inputbuf[0] != ISO_3) {
	PSOCK_CLOSE(&s->sout);
	smtp_done(7);
	PSOCK_EXIT(&s->sout);
}
PSOCK_SEND_STR(&s->sout, (char *)smtp_to);
PSOCK_SEND_STR(&s->sout, s->to);
PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);

if(strlen(s->to2)){
	PSOCK_SEND_STR(&s->sout, (char *)smtp_to);
	PSOCK_SEND_STR(&s->sout, s->to2);
	PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
}

if(strlen(s->to3)){
	PSOCK_SEND_STR(&s->sout, (char *)smtp_to);
	PSOCK_SEND_STR(&s->sout, s->to3);
	PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
}


PSOCK_SEND_STR(&s->sout, (char *)smtp_from);
PSOCK_SEND_STR(&s->sout, s->from);
PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
PSOCK_SEND_STR(&s->sout, (char *)smtp_subject);
PSOCK_SEND_STR(&s->sout, s->subject);
PSOCK_SEND_STR(&s->sout, (char *)/*smtp_crnl*/smtp_nl);
PSOCK_SEND_STR(&s->sout, (char *)smtp_crnl);

//PSOCK_SEND_STR(&s->sout, (char *)smtp_crnl);//my
PSOCK_SEND(&s->sout, s->msg, s->msglen);
PSOCK_SEND_STR(&s->sout, (char *)smtp_crnlperiodcrnl);
PSOCK_READTO(&s->sout, ISO_nl);
if(s->inputbuf[0] != ISO_2) {
	PSOCK_CLOSE(&s->sout);
	smtp_done(8);
	PSOCK_EXIT(&s->sout);
}

PSOCK_SEND_STR(&s->sout, (char *)smtp_quit);
smtp_done(SMTP_ERR_OK);

DEBUG_MSG(SMTP_DEBUG,"send end\r\n");

memset(mail_text,0,sizeof(mail_text));

//освобождаем
email_ready = 1;


PSOCK_END(&s->sout);

}
/*----------------------------------------------------------------------------------------*/
 void smtp_appcall(void) {
	 extern u8 uip_flags;

	if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
		return;


DEBUG_MSG(SMTP_DEBUG,"smtp_appcall\r\n");

  if(uip_closed()) {
	  s->connected = 0;
	  return;
  }
  if(uip_aborted() || uip_timedout()) {
	  s->connected = 0;
	  smtp_done(1);
	  return;
  }
  smtp_thread();
}

/*-----------------------------------------------------------------------------------*/
/**
 * Send an e-mail.
 *
 * \param to The e-mail address of the receiver of the e-mail.
 * \param from The e-mail address of the sender of the e-mail.
 * \param subject The subject of the e-mail.
 * \param msg The actual e-mail message.
 * \param msglen The length of the e-mail message.
 */
/*-----------------------------------------------------------------------------------*/
unsigned char smtp_send(char *to,char *to2,char *to3, char *from,char *cc, char *subject, char *msg, u16_t msglen,char *login, char *pass){
  if(strlen(msg)==0)
	  return 1;
  //флаг занятости
  email_ready = 0;

  //стартуем таймер
  timer_set(&smtp_timeout_timer, SMTP_TIMEOUT * MSEC  );

  if((smtpserver[0] == 0)&&(smtpserver[1]==0))
	  return 1;

  conn = uip_connect(&smtpserver, HTONS(get_smtp_port()));
  if(conn == NULL) {
    return 1;
  }
  s->state = STATE_SEND_NONE;
  s->sentlen = s->sendptr = s->textlen = 0;
  s->connected = 1;
  strcpy(s->to,to);
  strcpy(s->to2,to2);
  strcpy(s->to3,to3);
  strcpy(s->cc,cc);
  strcpy(s->from,from);
  strcpy(s->subject,subject);
  strcpy(s->msg,msg);
  s->msglen = msglen;
  strcpy(s->login,login);
  strcpy(s->pass,pass);

  //SMTP_DBG("smtp ip %d.%d.%d.%d  port %d\r\n",uip_ipaddr1(&smtpserver),uip_ipaddr2(&smtpserver),uip_ipaddr3(&smtpserver),uip_ipaddr4(&smtpserver),port);
  DEBUG_MSG(SMTP_DEBUG,"to:");DEBUG_MSG(SMTP_DEBUG,s->to);DEBUG_MSG(SMTP_DEBUG,"\r\n");
  DEBUG_MSG(SMTP_DEBUG,"from:");DEBUG_MSG(SMTP_DEBUG,s->from);DEBUG_MSG(SMTP_DEBUG,"\r\n");
  DEBUG_MSG(SMTP_DEBUG,"subj: ");DEBUG_MSG(SMTP_DEBUG,s->subject);DEBUG_MSG(SMTP_DEBUG,"\r\n");
  DEBUG_MSG(SMTP_DEBUG,"msg: ");DEBUG_MSG(SMTP_DEBUG,s->msg);DEBUG_MSG(SMTP_DEBUG,"msg len %d",strlen(s->msg));DEBUG_MSG(SMTP_DEBUG,"\r\n");
  DEBUG_MSG(SMTP_DEBUG,"login: ");DEBUG_MSG(SMTP_DEBUG,s->login);DEBUG_MSG(SMTP_DEBUG,"\r\n");
  DEBUG_MSG(SMTP_DEBUG,"passwd: ");DEBUG_MSG(SMTP_DEBUG,s->pass);DEBUG_MSG(SMTP_DEBUG,"\r\n");

  PSOCK_INIT(&s->sout, s->inputbuf, sizeof(s->inputbuf));

  return 0;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Specificy an SMTP server and hostname.
 *
 * This function is used to configure the SMTP module with an SMTP
 * server and the hostname of the host.
 *
 * \param lhostname The hostname of the uIP host.
 *
 * \param server A pointer to a 4-byte array representing the IP
 * address of the SMTP server to be configured.
 */
/*-----------------------------------------------------------------------------------*/
void smtp_configure(char *lhostname, u16_t *server)
{
    localhostname = lhostname;
	uip_ipaddr_copy(smtpserver, server);
	DEBUG_MSG(SMTP_DEBUG,"smtp_configure: ip addr %d.%d.%d.%d\r\n",
			uip_ipaddr1(server),uip_ipaddr2(server),uip_ipaddr3(server),uip_ipaddr4(server));
}
/*---------------------------------------------------------------------------*/
void smtp_init(void)
{
	DEBUG_MSG(SMTP_DEBUG,"smtp_init \r\n");
	s->connected = 0;
}
/*-----------------------------------------------------------------------------------*/
/** @} */
void smtp_done(unsigned char error){
	DEBUG_MSG(SMTP_DEBUG,"smtp_done: %d\r\n",error);
}

void smtp_config(void){
uip_ipaddr_t addr;
uip_ipaddr_t tmp_ip,dm_ip;
char dm[32];

	if(get_smtp_state() == 1){
		DEBUG_MSG(SMTP_DEBUG,"SMTP Queue create\r\n");
		//create smtp queue
		if(SmtpQueue==0){
			SmtpQueue = xQueueCreate(MSG_QUEUE_LEN,SMTP_MAX_LEN);
			if(SmtpQueue){
				//создаём очередь для отправки сообщений
				//глубина очереди MSG_QUEUE_LEN
				DEBUG_MSG(SMTP_DEBUG,"SMTP Queue created\r\n");
			}
			else{
				DEBUG_MSG(SMTP_DEBUG,"SMTP Queue alarm\r\n");
				ADD_ALARM(ERROR_CREATE_SMTP_QUEUE);
				return;
			}
		}


		smtp_init();
		get_smtp_server(&addr);
		get_smtp_domain(dm);

		//get server ip
		get_smtp_server(&tmp_ip);

		if((tmp_ip[0]==0)&&(tmp_ip[1]==0)){
			//static ip is not set, use dns
			resolv_lookup(dm,&dm_ip);
			DEBUG_MSG(SMTP_DEBUG,"smtp server dns\r\n");
			if((dm_ip[0]==0)&&(dm_ip[1]==0)){
				DEBUG_MSG(SMTP_DEBUG,"smtp server don`t exist, dns query\r\n");
				resolv_query(dm);
				timer_set(&dns_timer, 10000 * MSEC  );
			}
			else
				smtp_configure(dm,dm_ip);
		}
		else{
			DEBUG_MSG(SMTP_DEBUG,"smtp server static ip\r\n");
			smtp_configure(dm,addr);
		}
	}
}

u8 mail_send(char *subject,char *msg){
char to1[64],to2[64],to3[64],login[32],pass[32],from[64];
if(get_smtp_state()==1){
	/*адрес 1 должен быть всегда, остальных может и не быть*/
	get_smtp_to(to1);
	get_smtp_to2(to2);
	get_smtp_to3(to3);
	get_smtp_login(login);
	get_smtp_from(from);
	get_smtp_pass(pass);

	to1[strlen(to1)]=0;
	to2[strlen(to2)]=0;
	to3[strlen(to3)]=0;
	from[strlen(from)]=0;
	login[strlen(login)]=0;
	pass[strlen(pass)]=0;

	if(!strlen(to1))
		return 1;
	DEBUG_MSG(SMTP_DEBUG,"mail_send\r\n");
	if(smtp_send(to1,to2,to3,from,NULL,subject,msg,strlen(msg),login,pass)==0)
		return 0;

	DEBUG_MSG(SMTP_DEBUG,"mail_send error\r\n");
}
return 1;
}






void e_mail_init(void){

	if(get_smtp_state()==1){
	  //если очередь создалась, создаем задачу, которая будет отправлять
	  if(SmtpQueue == 0){
		  DEBUG_MSG(SMTP_DEBUG,"SMTP Queue not created!!!\r\n");
		  ADD_ALARM(ERROR_CREATE_SMTP_QUEUE);
		  return;
	  }

	  if(email_ready ==0){
		  if(xTaskCreate( EMailTask, (void*)"smtp",128*5, NULL, DEFAULT_PRIORITY, &xEMailTask )
					  ==errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
			  ADD_ALARM(ERROR_CREATE_SMTP_TASK);
		  }
	  }
  }
}

static char subj[64];
void EMailTask(void *pvParameters){
email_ready = 1;
	while (1){
		//получаем из очереди сообщение
		if((email_ready==1)&&(SmtpQueue)){
			if(xQueueReceive(SmtpQueue,mail_text,0) == pdPASS ){
				DEBUG_MSG(SMTP_DEBUG,"msg from queue recieve\r\n");
				if(strlen(mail_text)>0){
					get_smtp_subj(subj);
					subj[strlen(subj)]=0;
					mail_text[strlen(mail_text)] = 0;
					mail_send(subj,mail_text);//дублируем через e-mail
				}
			}
		}

		//если время вышло, разрешаем отправку в любом случае
		if((timer_expired(&smtp_timeout_timer))&&(email_ready == 0)){
			email_ready = 1;
			memset(mail_text,0,sizeof(mail_text));
			smtp_init();
			DEBUG_MSG(SMTP_DEBUG,"msg not send: timeout\r\n");
		}
		vTaskDelay(100*MSEC);
	}
	vTaskDelete(NULL);
}


