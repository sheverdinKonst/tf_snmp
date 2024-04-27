/**
 * \addtogroup smtp
 * @{
 */


/**
 * \file
 * SMTP header file
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
 * $Id: smtp.h,v 1.4 2006/06/11 21:46:37 adam Exp $
 *
 */
#ifndef __SMTP_H__
#define __SMTP_H__

//#include "uipopt.h"
#include "stm32f4xx.h"
#include "../uip/uip.h"
#include "../uip/psock.h"
#include "../webserver/httpd-fs.h"

/**
 * Error number that signifies a non-error condition.
 */
#define SMTP_ERR_OK 0

#define SMTP_SERVER_PORT 25

//макс длина сообщения
#define SMTP_MAX_LEN 128


//таймаут на отправку сообщений
#define SMTP_TIMEOUT 10000//10сек


/**
 * Callback function that is called when an e-mail transmission is
 * done.
 *
 * This function must be implemented by the module that uses the SMTP
 * module.
 *
 * \param error The number of the error if an error occured, or
 * SMTP_ERR_OK.
 */
void smtp_done(unsigned char error);

void smtp_init(void);

/* Functions. */
void smtp_configure(char *localhostname, u16_t *smtpserver);

unsigned char smtp_send(char *to,char *to2,char *to3,char *cc, char *from, char *subject, char *msg, u16_t msglen,char *login, char *pass);

/*#define SMTP_SEND(to,to2,to3, cc, from, subject, msg)\
       smtp_send(to,to2,to3, cc, from, subject, msg, strlen(msg))*/

void smtp_appcall(void);

struct smtp_state {
  struct psock  sout;	//my
  char inputbuf[512];//my
  char cc[64];//my
  u8 connected;//my
  u8_t state;
  char to[64];
  char to2[64];
  char to3[64];
  char from[64];
  char subject[64];
  char msg[256];
  u16_t msglen;

  char login[32];
  char pass[32];

  u16_t sentlen, textlen;
  u16_t sendptr;
};



//struct SMTP_cfg smtp_cfg;
//sizeof 214+32=246

#ifndef UIP_APPCALL
#define UIP_APPCALL     smtp_appcall
#endif
//typedef struct smtp_state uip_tcp_appstate_t;


void smtp_config(void);
u8 mail_send(char *subject,char *msg);
void smtp_cfg_test(void);
void set_smtp_cfg_default(void);
void EMailTask(void *pvParameters);
void e_mail_init(void);
//void tomail(void);



#endif /* __SMTP_H__ */

/** @} */
