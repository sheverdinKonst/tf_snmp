/* tftpserver.c */
#include "stm32f4xx.h"
#include "../deffines.h"
#include <string.h>
#include "../uip/uip.h"
#include "../uip/uip_arp.h"
#include "tftpserver.h"
#include "tftputils.h"
#include "board.h"
#include "spiflash.h"
#include "settings.h"
#include "debug.h"

#if TFTP_SERVER

tftp_connection_args args;

//FATFS filesystem;
//FIL file_SD, file_CR;
//DIR dir_1, dir_2;

#if defined   (__CC_ARM) /*!< ARM Compiler */
  __align(4)
  __IO uint8_t TempBuffer[600] = {0};

#elif defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4
  __IO uint8_t TempBuffer[600] = {0};

#elif defined (__GNUC__) /*!< GNU Compiler */
  __IO uint8_t TempBuffer[600] __attribute__ ((aligned (4)));

#elif defined  (__TASKING__) /*!< TASKING Compiler */
  __align(4)
  __IO uint8_t TempBuffer[600] = {0};
#endif /* __CC_ARM */


/* UDPpcb to be binded with port 69  */
struct uip_udp_conn *tftp_conn;
#endif
/* tftp_errorcode error strings */
char *tftp_errorcode_string[] = {
                                  "not defined",
                                  "file not found",
                                  "access violation",
                                  "disk full",
                                  "illegal operation",
                                  "unknown transfer id",
                                  "file already exists",
                                  "no such user",
                                  "incorrect options",
                                };



/**
  * @brief  sends a TFTP message
  * @param  upcb: pointer on a udp pcb
  * @param  to_ip: pointer on remote IP address
  * @param  to_port: pointer on remote port
  * @param buf: pointer on buffer where to create the message
  * @param err: error code of type tftp_errorcode
  * @retval error code
  */
err_t tftp_send_message(char *buf, int buflen)
{
  err_t err = 0;
  uip_ipaddr_t addr;

  //DEBUG_MSG(TFTP_DEBUG,"tftp_send_message %d , %s\r\n",buflen,buf);

  /* Sending packet by UDP protocol */
  struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];

  uip_ipaddr_copy(&addr,&m->srcipaddr);
  uip_ipaddr_copy(&m->srcipaddr,&m->destipaddr);
  uip_ipaddr_copy(&m->destipaddr,&addr);

  /*DEBUG_MSG(TFTP_DEBUG,"tftp_send_message m->srcipaddr %d.%d.%d.%d \r\n",
  						uip_ipaddr1(m->srcipaddr),uip_ipaddr2(m->srcipaddr),
  						uip_ipaddr3(m->srcipaddr),uip_ipaddr4(m->srcipaddr));

  DEBUG_MSG(TFTP_DEBUG,"tftp_send_message m->destipaddr %d.%d.%d.%d \r\n",
  						uip_ipaddr1(m->destipaddr),uip_ipaddr2(m->destipaddr),
  						uip_ipaddr3(m->destipaddr),uip_ipaddr4(m->destipaddr));
  */
  uip_udp_send(buflen);
  return err;
}


/**
  * @brief construct an error message into buf
  * @param buf: pointer on buffer where to create the message
  * @param err: error code of type tftp_errorcode
  * @retval
  */
int tftp_construct_error_message(char *buf, tftp_errorcode err)
{
  int errorlen;
  /* Set the opcode in the 2 first bytes */
  tftp_set_opcode(buf, TFTP_ERROR);
  /* Set the errorcode in the 2 second bytes  */
  tftp_set_errorcode(buf, err);
  /* Set the error message in the last bytes */
  tftp_set_errormsg(buf, tftp_errorcode_string[err]);
  /* Set the length of the error message  */
  errorlen = strlen(tftp_errorcode_string[err]);

  /* return message size */
  return 4 + errorlen + 1;
}


/**
  * @brief Sends a TFTP error message
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote port
  * @param  err: tftp error code
  * @retval error value
  */
int tftp_send_error_message(tftp_errorcode err)
{
  char *buf;
  int error_len;


  struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
  m->srcport = HTONS(get_tftp_port());
  buf = uip_appdata;

  DEBUG_MSG(TFTP_DEBUG,"tftp_send_error_message %d\r\n",err);
  /* construct error */
  error_len = tftp_construct_error_message(buf, err);
  /* send error message */
  return tftp_send_message(buf, error_len);
}


/**
  * @brief  Sends TFTP data packet
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  block: block number
  * @param  buf: pointer on data buffer
  * @param  buflen: buffer length
  * @retval error value
  */
int tftp_send_data_packet(int block, char *buf, int buflen){
char *outbuf;
  DEBUG_MSG(TFTP_DEBUG,"tftp_send_data_packet: block %d, len %d\r\n",block,buflen);

  //struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
  //m->srcport = HTONS(get_tftp_port());
  outbuf = uip_appdata;

  /* Set the opcode 3 in the 2 first bytes */
  tftp_set_opcode(outbuf, TFTP_DATA);
  /* Set the block numero in the 2 second bytes */
  tftp_set_block(outbuf, block);
  /* Set the data message in the n last bytes */
  tftp_set_data_message(outbuf, buf, buflen);
  /* Send DATA packet */
  return tftp_send_message(outbuf, buflen + 4);
}

/**
  * @brief  Sends TFTP ACK packet
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  block: block number
  * @retval error value
  */
int tftp_send_ack_packet(int block)
{
  /* create the maximum possible size packet that a TFTP ACK packet can be */
  char *packet;

  packet = uip_appdata;

  DEBUG_MSG(TFTP_DEBUG,"tftp_send_ack_packet\r\n");

  /* define the first two bytes of the packet */
  tftp_set_opcode(packet, TFTP_ACK);

  /* Specify the block number being ACK'd.
   * If we are ACK'ing a DATA pkt then the block number echoes that of the DATA pkt being ACK'd (duh)
   * If we are ACK'ing a WRQ pkt then the block number is always 0
   * RRQ packets are never sent ACK pkts by the server, instead the server sends DATA pkts to the
   * host which are, obviously, used as the "acknowledgement" */
  tftp_set_block(packet, block);

  return tftp_send_message(packet, TFTP_ACK_PKT_LEN);
}

#if TFTP_SERVER
/**
  * @brief Cleanup after end of TFTP read operation
  * @param  args: pointer on a structure of type tftp_connection_args
  * @retval None
  */
void tftp_cleanup_rd(tftp_connection_args *args)
{
  /* close the filesystem */
  //f_close(&file_SD);
  //f_mount(0,NULL);
  /* Free the tftp_connection_args structure reserverd for */
  //mem_free(args);

  /* Disconnect the udp_pcb*/
  //udp_disconnect(upcb);

  /* close the connection */
  //udp_remove(upcb);

  //udp_recv(UDPpcb, recv_callback_tftp, NULL);

  //uip_udp_remove (tftp_conn);
}

/**
  * @brief Cleanup after end of TFTP write operation
  * @param  args: pointer on a structure of type tftp_connection_args
  * @retval None
  */
void tftp_cleanup_wr(tftp_connection_args *args)
{
  /* close the filesystem */
  //f_close(&file_CR);
  //f_mount(0,NULL);
  /* Free the tftp_connection_args structure reserverd for */
  //mem_free(args);

  /* Disconnect the udp_pcb*/
  //udp_disconnect(upcb);

  /* close the connection */
  //udp_remove(upcb);

  /* reset the callback function */
  //udp_recv(UDPpcb, recv_callback_tftp, NULL);

  //uip_udp_remove (tftp_conn);
}

/**
  * @brief  sends next data block during TFTP READ operation
  * @param  upcb: pointer on a udp pcb
  * @param  args: pointer on structure of type tftp_connection_args
  * @param  to_ip: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @retval None
  */
void tftp_send_next_block(tftp_connection_args *args)
{
  /* Function to read 512 bytes from the file to send (file_SD), put them
   * in "args->data" and return the number of bytes read */
   //f_read(&file_SD, (uint8_t*)args->data, TFTP_DATA_LEN_MAX, (UINT*)(&args->data_len));

  /*   NOTE: We need to send data packet even if args->data_len = 0*/

  /* sEndTransferthe data */
  tftp_send_data_packet(args->block, args->data, args->data_len);

}

/**
  * @brief  receive callback during tftp read operation (not on standard port 69)
  * @param  arg: pointer on argument passed to callback
  * @param  pkt_buf: pointer on the received pbuf
  * @param  addr: pointer on remote IP address
  * @param  port: pointer on remote udp port
  * @retval None
  */
void rrq_recv_callback(void *arg, u8 *buff)
{
  /* Get our connection state  */
  tftp_connection_args *args = (tftp_connection_args *)arg;

  DEBUG_MSG(TFTP_DEBUG,"rrq_recv_callback\r\n");

  if (tftp_is_correct_ack((char *)buff, args->block))
  {
    /* increment block # */
    args->block++;
  }
  else
  {
    /* we did not receive the expected ACK, so
       do not update block #. This causes the current block to be resent. */
  }

  /* if the last read returned less than the requested number of bytes
   * (i.e. TFTP_DATA_LEN_MAX), then we've sent the whole file and we can quit
   */
  if (args->data_len < TFTP_DATA_LEN_MAX)
  {
    /* Clean the connection*/
    //tftp_cleanup_rd(args);

    //pbuf_free(p);
  }

  /* if the whole file has not yet been sent then continue  */
  tftp_send_next_block(args);

  //pbuf_free(p);
}

/**
  * @brief  receive callback during tftp write operation (not on standard port 69)
  * @param  arg: pointer on argument passed to callback
  * @param  udp_pcb: pointer on the udp pcb
  * @param  pkt_buf: pointer on the received pbuf
  * @param  addr: pointer on remote IP address
  * @param  port: pointer on remote port
  * @retval None
  */
void wrq_recv_callback(u8 *pkt_buf,void *arg)
{
  tftp_connection_args *args = (tftp_connection_args *)arg;
  int n = 0;

  DEBUG_MSG(TFTP_DEBUG,"wrq_recv_callback\r\n");

  /* we expect to receive only one pbuf (pbuf size should be
     configured > max TFTP frame size */
/*  if (pkt_buf->len != pkt_buf->tot_len)
  {
    return;
  }
*/
  /* Does this packet have any valid data to write? */
  if ((uip_len > TFTP_DATA_PKT_HDR_LEN) &&
      (tftp_extract_block((char *)pkt_buf) == (args->block + 1)))
  {
    /* write the received data to the file */
    memcpy((char*)TempBuffer, (char*)pkt_buf + TFTP_DATA_PKT_HDR_LEN, uip_len - TFTP_DATA_PKT_HDR_LEN);
    //f_write(&file_CR, (char*)TempBuffer, pkt_buf->len - TFTP_DATA_PKT_HDR_LEN, (UINT*)&n);
	//TFTP2Flash();
    if (n <= 0)
    {
      //tftp_send_error_message(addr, port, TFTP_ERR_FILE_NOT_FOUND);
      /* close the connection */
      tftp_cleanup_wr(args); /* close the connection */
    }

    /* update our block number to match the block number just received */
    args->block++;

    /* update total bytes  */
    (args->tot_bytes) += (uip_len - TFTP_DATA_PKT_HDR_LEN);
  }
  else if (tftp_extract_block((char *)pkt_buf/*->payload*/) == (args->block + 1))
  {
    /* update our block number to match the block number just received  */
    args->block++;
  }

  /* Send the appropriate ACK pkt (the block number sent in the ACK pkt echoes
   * the block number of the DATA pkt we just received - see RFC1350)
   * NOTE!: If the DATA pkt we received did not have the appropriate block
   * number, then the args->block (our block number) is never updated and
   * we simply send "duplicate ACK" which has the same block number as the
   * last ACK pkt we sent.  This lets the host know that we are still waiting
   * on block number args->block+1.
   */
  tftp_send_ack_packet(args->block);

  /* If the last write returned less than the maximum TFTP data pkt length,
   * then we've received the whole file and so we can quit (this is how TFTP
   * signals the end of a transfer!)
   */
  if (uip_len < TFTP_DATA_PKT_LEN_MAX)
  {

  }
  else
  {
    return;
  }
}


/**
  * @brief  processes tftp read operation
  * @param  upcb: pointer on udp pcb
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  FileName: pointer on filename to be read
  * @retval error code
  */
int tftp_process_read(char* FileName)
{

  DEBUG_MSG(TFTP_DEBUG,"tftp_process_read\r\n");

  /* If Could not open the file which will be transmitted  */
  if (/*f_open(&file_SD, (const TCHAR*)FileName, FA_READ) != FR_OK*/0)
  {
    tftp_send_error_message(TFTP_ERR_FILE_NOT_FOUND);

    //tftp_cleanup_rd(args);

    return 0;
  }

  //args = mem_malloc(sizeof *args);

  /* If we aren't able to allocate memory for a "tftp_connection_args" */
  if (/*!args*/0)
  {
    /* unable to allocate memory for tftp args  */
    tftp_send_error_message(TFTP_ERR_NOTDEFINED);

    /* no need to use tftp_cleanup_rd because no "tftp_connection_args" struct has been malloc'd   */
    //tftp_cleanup_rd(args);

    return 0;
  }

  /* initialize connection structure  */
  args.op = TFTP_RRQ;
  //uip_ipaddr_copy(args.to_ip,to);
  args.to_port = 69;//to_port;
  args.block = 1; /* block number starts at 1 (not 0) according to RFC1350  */
  args.tot_bytes = 0;


  /* set callback for receives on this UDP PCB  */
  //udp_recv(rrq_recv_callback, args);

  /* initiate the transaction by sending the first block of data,
    further blocks will be sent when ACKs are received */

  tftp_send_next_block(&args);

  return 1;
}

/**
  * @brief  processes tftp write operation
  * @param  upcb: pointer on upd pcb
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  FileName: pointer on filename to be written
  * @retval error code
  */
int tftp_process_write(char *FileName)
{
  //tftp_connection_args *args = NULL;

  DEBUG_MSG(TFTP_DEBUG,"tftp_process_write\r\n");

  /* Can not create file */
  if (/*f_open(&file_CR, (const TCHAR*)FileName, FA_CREATE_ALWAYS|FA_WRITE) != FR_OK*/0)
  {
    tftp_send_error_message(TFTP_ERR_NOTDEFINED);

    //tftp_cleanup_wr(args);

    return 0;
  }

  //args = mem_malloc(sizeof *args);
  if (/*!args*/0)
  {
    tftp_send_error_message(TFTP_ERR_NOTDEFINED);
    tftp_cleanup_wr(&args);
    return 0;
  }

  args.op = TFTP_WRQ;
  //uip_ipaddr_copy(&args.to_ip,to);
  //args.to_port = to_port;
  /* the block # used as a positive response to a WRQ is _always_ 0!!! (see RFC1350)  */
  args.block = 0;
  args.tot_bytes = 0;

  /* set callback for receives on this UDP PCB  */
  //udp_recv(wrq_recv_callback, args);

  /* initiate the write transaction by sending the first ack */
  tftp_send_ack_packet(args.block);

  return 0;
}


/**
  * @brief  processes the tftp request on port 69
  * @param  pkt_buf: pointer on received pbuf
  * @param  ip_addr: pointer on source IP address
  * @param  port: pointer on source udp port
  * @retval None
  */
void process_tftp_request(u8 *pkt_buf)
{
  tftp_opcode op = tftp_decode_op((char *)pkt_buf);
  char FileName[30];
  //struct udp_pcb *upcb;
  //err_t err;

  DEBUG_MSG(TFTP_DEBUG,"process_tftp_request\r\n");

  /* create new UDP PCB structure */
  //upcb = udp_new();
  if (/*!upcb*/0)
  {
    /* Error creating PCB. Out of Memory  */
    return;
  }

  /* bind to port 0 to receive next available free port */
  /* NOTE:  This is how TFTP works.  There is a UDP PCB for the standard port
   * 69 which al transactions begin communication on, however all subsequent
   * transactions for a given "stream" occur on another port!  */

  //err = udp_bind(upcb, IP_ADDR_ANY, 0);
  if (/*err != ERR_OK*/0)
  {
    /* Unable to bind to port   */
    return;
  }

  switch (op)
  {
    case TFTP_RRQ:/* TFTP RRQ (read request)  */
    {
      /* Read the name of the file asked by the client to be sent from the SD card */
      tftp_extract_filename(FileName, (char *)pkt_buf/*->payload*/);

      /* could not open filesystem */
      if (/*f_mount(0, &filesystem) != FR_OK*/0)
      {
        return;
      }
      /* could not open the selected directory */
      if (/*f_opendir(&dir_1, "/") != FR_OK*/0)
      {
        return;
      }
      /* Start the TFTP read mode*/
      tftp_process_read(FileName);

      break;
    }

    case TFTP_WRQ:    /* TFTP WRQ (write request)   */
    {
      /* Read the name of the file asked by the client to be received and writen in the SD card */
      tftp_extract_filename(FileName, (char *)pkt_buf);

      /* Could not open filesystem */
      if (/*f_mount(0, &filesystem) != FR_OK*/0)
      {
        return;
      }

      /* If Could not open the selected directory */
      if (/*f_opendir(&dir_2, "/") != FR_OK*/0)
      {
        return;
      }

      /* Start the TFTP write mode*/
      tftp_process_write(FileName);
      break;
    }
    default: /* TFTP unknown request op */
      /* send generic access violation message */
      tftp_send_error_message(TFTP_ERR_ACCESS_VIOLATION);
      //udp_remove(upcb);
      break;
  }
}


/**
  * @brief  tftp server receive callback on port 69
  * @param  arg: pointer on argument passed to callback (not used here)
  * @param  upcb: pointer on udp pcb
  * @param  pkt_buf: pointer on received pbuf
  * @param  ip_addr: pointer on source IP address
  * @param  port: pointer on source udp port
  * @retval None
  */
void tftps_appcall(void){
  unsigned char *appdata;
  uint16_t ln;
  //struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
  uip_ipaddr_t to;
  char FileName[64];


  if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
	  return;

  //uip_ipaddr_copy()

  if (uip_newdata()){
	 if (uip_datalen()){
		if(/*uip_arp_out_check(tftp_conn) == 0*/1){
			appdata = uip_appdata;
			ln = uip_datalen();
			DEBUG_MSG(TFTP_DEBUG,"cmdtftp_appcall new data, len = %d\r\n",ln);
			//process_tftp_request(appdata,(uip_ipaddr_t *)(m->srcipaddr),m->srcport);

			//get TFTP op code
			tftp_opcode op = tftp_decode_op((char *)appdata);
			switch (op){
				case TFTP_RRQ:/* TFTP RRQ (read request)  */
					tftp_extract_filename(FileName, (char *)appdata);
					args.op = TFTP_RRQ;
					uip_ipaddr_copy(args.to_ip,to);
					args.to_port = /*to_port*/69;
					args.block = 1; /* block number starts at 1 (not 0) according to RFC1350  */
					args.tot_bytes = 0;
					/* initiate the transaction by sending the first block of data,
					further blocks will be sent when ACKs are received */
					tftp_send_next_block(&args);
					break;
				case TFTP_WRQ:    /* TFTP WRQ (write request)   */
					/* Read the name of the file asked by the client to be received and writen in the SD card */
					tftp_extract_filename(FileName, (char *)appdata);

					args.op = TFTP_WRQ;
					uip_ipaddr_copy(&args.to_ip,to);
					args.to_port = 69;//to_port;
					/* the block # used as a positive response to a WRQ is _always_ 0!!! (see RFC1350)  */
					args.block = 0;
					args.tot_bytes = 0;
					tftp_send_ack_packet(args.block);
				    break;

				case TFTP_DATA:
					switch(args.op){
						case TFTP_WRQ:
							wrq_recv_callback(appdata,&args);
							break;
						case TFTP_RRQ:
							rrq_recv_callback(&args,appdata);
							break;
					}
					break;

				default: /* TFTP unknown request op */
				      /* send generic access violation message */
					  DEBUG_MSG(TFTP_DEBUG,"cmdtftp_appcall opcode %d\r\n",op);
				      tftp_send_error_message(TFTP_ERR_ACCESS_VIOLATION);
				      break;
			 }

		}else{
			//send arp
			uip_udp_send(uip_len);
			DEBUG_MSG(TFTP_DEBUG,"tftp not send, arp msg\r\n");
		}
	 }
  }
}



/**
  * @brief  Initializes the udp pcb for TFTP
  * @param  None
  * @retval None
  */
void tftps_init(void)
{
  u16 port = 69;
  uip_ipaddr_t addr;
  uip_ipaddr(&addr,192,168,0,104);

  port = get_tftp_port();

  /* create a new UDP connection  */
  if(get_tftp_state() == ENABLE){
	  if(tftp_conn){
		uip_udp_remove (tftp_conn);
		DEBUG_MSG(TFTP_DEBUG,"remove udp conn tftp\r\n");
	  }
	  tftp_conn = uip_udp_new (&addr/*NULL*/, 0);
	  if (tftp_conn!=NULL)
	  {
		  uip_udp_bind (tftp_conn, HTONS (port));
		  DEBUG_MSG(TFTP_DEBUG,"tftp conn created OK\r\n");
	  }
  }
}
#endif
