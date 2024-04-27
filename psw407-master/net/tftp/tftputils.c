/* tftputils.c */

#include "stm32f4xx.h"
#include <string.h>
#include "tftputils.h"
#include "../uip/uip.h"
#include "debug.h"
/**
  * @brief  Extracts the opcode from a TFTP message
**/ 
tftp_opcode tftp_decode_op(char *buf)
{
  return (tftp_opcode)(buf[1]);
}

/**
  * @brief Extracts the block number from TFTP message
**/
u16 tftp_extract_block(char *buf)
{
  u16 *b = (u16*)buf;
  return ntohs(b[1]);
}



void tftp_extract_data(u8 *buff, u8 *outbuff){
	//memcpy(outbuff,buff,TFTP_DATA_LEN_MAX);
	outbuff = buff+4;
}

/**
  * @brief Extracts the filename from TFTP message
**/ 
void tftp_extract_filename(char *fname, char *buf)
{
  strcpy(fname, buf + 2);
  DEBUG_MSG(TFTP_DEBUG,"tftp_extract_filename %s\r\n",fname);
}


char *tftp_extract_options_tsize(char *buff){
char *ptr;
	DEBUG_MSG(TFTP_DEBUG,"tftp_extract_options_tsize, buff:%s\r\n",buff);
	ptr = strstr(buff,"tsize");
	if(ptr == NULL)
		return NULL;
	ptr = ptr + strlen(ptr) + 1;
	DEBUG_MSG(TFTP_DEBUG,"tftp_extract_options_tsize, tsize:%s\r\n",ptr);
	return ptr;
}

char *tftp_extract_options_blksize(char *buff){
char *ptr;
	DEBUG_MSG(TFTP_DEBUG,"tftp_extract_options_blksize, buff:%s\r\n",buff);
	ptr = strstr(buff,"blksize");
	if(ptr == NULL)
		return NULL;
	ptr = ptr + strlen(ptr) + 1;
	DEBUG_MSG(TFTP_DEBUG,"tftp_extract_options_blksize, blksize:%s\r\n",ptr);
	return ptr;
}




/**
  * @brief set the opcode in TFTP message: RRQ / WRQ / DATA / ACK / ERROR 
**/ 
void tftp_set_opcode(char *buffer, tftp_opcode opcode)
{

  buffer[0] = 0;
  buffer[1] = (u8)opcode;
}

void tftp_set_filename(char *buffer,char *fname)
{
  strcpy(buffer + 2,fname);
  DEBUG_MSG(TFTP_DEBUG,"tftp_set_filename %s\r\n",fname);
}

/**
  * @brief Set the errorcode in TFTP message
**/
void tftp_set_errorcode(char *buffer, tftp_errorcode errCode)
{

  buffer[2] = 0;
  buffer[3] = (u8)errCode;
}

/**
  * @brief Sets the error message
**/
void tftp_set_errormsg(char * buffer, char* errormsg)
{
  strcpy(buffer + 4, errormsg);
}

/**
  * @brief Sets the block number
**/
void tftp_set_block(char* packet, u16 block)
{

  u16 *p = (u16_t *)packet;
  p[1] = htons(block);
}

/**
  * @brief Set the data message
**/
void tftp_set_data_message(char* packet, char* buf, int buflen)
{
  memcpy(packet + 4, buf, buflen);
}

/**
  * @brief Check if the received acknowledgement is correct
**/
u32 tftp_is_correct_ack(char *buf, int block)
{
  /* first make sure this is a data ACK packet */
  if (tftp_decode_op(buf) != TFTP_ACK)
    return 0;

  /* then compare block numbers */
  if (block != tftp_extract_block(buf))
    return 0;

  return 1;
}

u8 *tftp_add_opcode(u8 *buf,tftp_opcode opcode){
	*buf++ = 0;
	*buf++ = (u8)opcode;
	return buf;
}

u8 *tftp_add_filename(u8 *buf, char *filename){
	memcpy(buf, filename, strlen(filename));
	buf += strlen(filename);
	*buf++ = 0;
	return buf;
}

u8 *tftp_add_type(u8 *buf,char *type){
	memcpy(buf, type, strlen(type));
	buf += strlen(type);
	*buf++ = 0;
	return buf;
}

u8 *tftp_add_blksize(u8 *buf,char* blksize){
	memcpy(buf, TFTP_BLKSIZE, strlen(TFTP_BLKSIZE));
	buf += strlen(TFTP_BLKSIZE);
	*buf++ = 0;
	memcpy(buf, blksize, strlen(blksize));
	buf += strlen(blksize);
	*buf++ = 0;
	return buf;
}

u8 *tftp_add_tsize(u8 *buf,char* tsize){
	memcpy(buf, TFTP_TSIZE, strlen(TFTP_TSIZE));
	buf += strlen(TFTP_TSIZE);
	*buf++ = 0;
	memcpy(buf, tsize, strlen(tsize));
	buf += strlen(tsize);
	*buf++ = 0;
	return buf;
}


