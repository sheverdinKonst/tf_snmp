
#ifndef __TFTP_UTILS_H_
#define __TFTP_UTILS_H_

#include "tftpserver.h"
#include "tftpclient.h"

typedef  int err_t;

tftp_opcode tftp_decode_op(char *buf);
void tftp_extract_filename(char *fname, char *buf);
u16 tftp_extract_block(char *buf);
void tftp_extract_data(u8 *buff, u8 *outbuff);
char *tftp_extract_options_tsize(char *buff);
char *tftp_extract_options_blksize(char *buff);

void tftp_set_opcode(char *buffer, tftp_opcode opcode);
void tftp_set_errorcode(char *buffer, tftp_errorcode errCode);
void tftp_set_errormsg(char * buffer, char* errormsg);
u32 tftp_is_correct_ack(char *buf, int block);
void tftp_set_data_message(char* packet, char* buf, int buflen);
void tftp_set_block(char* packet, u16 block);

u8 *tftp_add_opcode(u8 *buf,tftp_opcode opcode);
u8 *tftp_add_filename(u8 *buf, char *filename);
u8 *tftp_add_type(u8 *buf,char *type);
u8 *tftp_add_blksize(u8 *buf,char* blksize);
u8 *tftp_add_tsize(u8 *buf,char* tsize);

#endif
