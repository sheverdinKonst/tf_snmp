/*
 * Copyright (c) 2001-2005, Adam Dunkels.
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
 * $Id: httpd.h,v 1.2 2006/06/11 21:46:38 adam Exp $
 *
 */

#ifndef __HTTPD_H__
#define __HTTPD_H__

#include "stm32f4xx.h"

#include "../uip/psock.h"
#include "httpd-fs.h"

#define HTTP_GET 1
#define HTTP_POST 2
#define HTTP_SEND_503 3

struct httpd_state {
  uint16_t timer;
  struct psock       sin, sout;
  char request_type;
  char state;
  char inputbuf[2*256];   // для обновления ПО через web должен быть равен размеру страницы флэш, или перекачивать через промежуточный буфер
  struct pt outputpt, scriptpt;
  char   filename[50];
  char   param[700];//размер параметров
  unsigned short param_ind;
  unsigned long content_length;
  char is_authorized; 
  struct httpd_fs_file file;
  int    len;
  char *scriptptr;
  int scriptlen;
  char contenttype;
#define HTTP_CONTENT_NONE       0
#define HTTP_CONTENT_FORMDATA   1
#define HTTP_CONTENT_URLENCODED 2
  char boundary[60];
  char parse_state;
#define HTTP_PARSE_IDLE             0
#define HTTP_PARSE_WAIT_DISPOSITION 1
#define HTTP_PARSE_WAIT_NL1         2
#define HTTP_PARSE_WAIT_PARAM       3
#define HTTP_PARSE_WAIT_OCTET_STREM 4
#define HTTP_PARSE_WAIT_UPLOAD      5
#define HTTP_PARSE_WAIT_UPLOADED    6
};

extern char http_passwd_enable;
void httpd_renew_passwd(void);

void httpd_init(void);
void httpd_appcall(void);

void tosave(void);

void nosave(void);

void set_progress(u32 offset);
u32 get_progress(void);


void encode64( const char *instr, char *outstr);
void encode64len( const char *instr, char *outstr, int length);
PT_THREAD(send_bak(struct httpd_state *s));
PT_THREAD(send_file(struct httpd_state *s));
PT_THREAD(send_part_of_file(struct httpd_state *s));

void http_logout(void);

#endif /* __HTTPD_H__ */
