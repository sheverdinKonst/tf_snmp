
#ifndef __TFTPSERVER_H_
#define __TFTPSERVER_H_

#include "../uip/uip.h"
#include "../deffines.h"

#define TFTP_OPCODE_LEN         2
#define TFTP_BLKNUM_LEN         2
#define TFTP_ERRCODE_LEN        2
#define TFTP_DATA_LEN_MAX       512//no modify
#define TFTP_DATA_PKT_HDR_LEN   (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_ERR_PKT_HDR_LEN    (TFTP_OPCODE_LEN + TFTP_ERRCODE_LEN)
#define TFTP_ACK_PKT_LEN        (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_DATA_PKT_LEN_MAX   (TFTP_DATA_PKT_HDR_LEN + TFTP_DATA_LEN_MAX)

/* TFTP opcodes as specified in RFC1350   */
typedef enum {
  TFTP_RRQ = 1,
  TFTP_WRQ = 2,
  TFTP_DATA = 3,
  TFTP_ACK = 4,
  TFTP_ERROR = 5,
  TFTP_OPTACK = 6
} tftp_opcode;

#define ERR_OK 	0

/* TFTP error codes as specified in RFC1350  */
typedef enum {
  TFTP_ERR_NOTDEFINED,
  TFTP_ERR_FILE_NOT_FOUND,
  TFTP_ERR_ACCESS_VIOLATION,
  TFTP_ERR_DISKFULL,
  TFTP_ERR_ILLEGALOP,
  TFTP_ERR_UKNOWN_TRANSFER_ID,
  TFTP_ERR_FILE_ALREADY_EXISTS,
  TFTP_ERR_NO_SUCH_USER,
  TFTP_ERR_INCORRECT_OPTIONS,
} tftp_errorcode;

typedef struct
{
  int op;    /* RRQ/WRQ */

  /* last block read */
  char data[TFTP_DATA_PKT_LEN_MAX];
  uint32_t  data_len;

  /* destination ip:port */
  uip_ipaddr_t to_ip;
  int to_port;

  /* next block number */
  int block;

  /* total number of bytes transferred */
  u32 tot_bytes;


  u8 use_options;

  /* timer interrupt count when last packet was sent */
  /* this should be used to resend packets on timeout */
  unsigned long long last_time;

}tftp_connection_args;

#if TFTP_SERVER
void tftps_init(void);
void tftps_appcall(void);
int tftp_process_write(char* FileName);
int tftp_process_read(char* FileName);
#endif

int tftp_send_error_message(tftp_errorcode err);
int tftp_send_data_packet(int block, char *buf, int buflen);
int tftp_send_ack_packet(int block);
#endif

