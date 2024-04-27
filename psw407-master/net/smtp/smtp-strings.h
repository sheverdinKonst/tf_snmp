/*
 * Copyright (c) 2004, Adam Dunkels.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: smtp-strings.h,v 1.3 2006/06/11 21:46:37 adam Exp $
 */
/*
extern const char smtp_220[4];
extern const char smtp_helo[6];
extern const char smtp_mail_from[12];
extern const char smtp_rcpt_to[10];
extern const char smtp_data[7];
extern const char smtp_to[5];
extern const char smtp_cc[5];
extern const char smtp_from[7];
extern const char smtp_subject[10];
extern const char smtp_quit[7];
extern const char smtp_crnl[3];
extern const char smtp_crnlperiodcrnl[6];
*/
const char smtp_220[4] =
/* "220" */
{'2', '2', '0', };
//{0x32, 0x32, 0x30, };

const char smtp_helo[6] =
/* "HELO " */
//{0x48, 0x45, 0x4c, 0x4f, 0x20, };
{'H', 'E', 'L', 'O', ' ', };

const char smtp_ehlo[6] =
/* "EHLO " */
//{ 0x45,0x48, 0x4c, 0x4f, 0x20, };
{'E', 'H', 'L', 'O', ' ', };


const char smtp_mail_from[11] =
/* "MAIL FROM: " */
//{0x4d, 0x41, 0x49, 0x4c, 0x20, 0x46, 0x52, 0x4f, 0x4d, 0x3a, 0x20, };
{'M','A','I','L',' ','F','R','O','M',':', };

const char smtp_rcpt_to[9] =
/* "RCPT TO: " */
//{0x52, 0x43, 0x50, 0x54, 0x20, 0x54, 0x4f, 0x3a, 0x20, };
{'R','C','P','T',' ','T','O',':',};

const char smtp_data[7] =
/* "DATA\r\n" */
//{0x44, 0x41, 0x54, 0x41, 0xd, 0xa, };
{'D','A','T','A',0xd, 0xa, };

const char smtp_to[5] =
/* "To: " */
//{0x54, 0x6f, 0x3a, 0x20, };
{'T','o',':',' ',};

const char smtp_cc[5] =
/* "Cc: " */
//{0x43, 0x63, 0x3a, 0x20, };
{'C', 'c', ':', ' ', };

const char smtp_from[7] =
/* "From: " */
//{0x46, 0x72, 0x6f, 0x6d, 0x3a, 0x20, };
{'F','r','o','m',':',' ', };

const char smtp_subject[10] =
/* "Subject: " */
//{0x53, 0x75, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x3a, 0x20, };
{'S','u','b','j','e','c','t',':',' ', };

const char smtp_quit[7] =
/* "QUIT\r\n" */
//{0x51, 0x55, 0x49, 0x54, 0xd, 0xa, };
{'Q','U','I','T','\r','\n',' ', };

const char smtp_crnl[3] =
/* "\r\n" */
{0xd, 0xa, };

const char smtp_cr[2] =
/* "\r\n" */
{0xd,  };

const char smtp_nl[2] =
/* "\r\n" */
{0xa,  };


const char smtp_crnlperiodcrnl[6] =
/* "\r\n.\r\n" */
{0xd, 0xa, 0x2e, 0xd, 0xa, };


const char smtp_250[4] = //sucess
{'2','5','0', };

const char smtp_235[4] = //auth sucess
{'2', '3', '5', };

const char smtp_auth[5] = //AUTH
{'A','U','T','H',};

const char smtp_plain[6] = //PLAIN
{'P','L','A','I','N', };

const char smtp_login[6] = //LOGIN
{'L','O','G','I','N', };

const char smtp_auth_login[11] = //AUTH LOGIN
{'A','U','T','H',' ','L','O','G','I','N',};

const char smtp_auth_plain[11] = //AUTH PLAIN
{'A','U','T','H',' ','P','L','A','I','N',};

const char smtp_334_username[17] =
/*334 VXNlcm5hbWU6*/
{'3','3','4',' ','V','X','N','l','c','m','5','h','b','W','U','6', };

const char smtp_334[4] =
{'3','3','4',};


const char smtp_334_password[17] =
/* 334 UGFzc3dvcmQ6 */
{'3','3','4',' ','U','G','F','z','c','3','d','v','c','m','Q','6', };

const char smtp_left_sk[2] =
{'<',};

const char smtp_right_sk[2] =
{'>',};
