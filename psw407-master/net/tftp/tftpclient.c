#include "stm32f4xx.h"
#include "../deffines.h"
#include <string.h>
#include <stdlib.h>
#include "../uip/uip.h"
#include "../uip/uip_arp.h"
#include "tftpclient.h"
#include "tftpserver.h"
#include "tftputils.h"
#include "board.h"
#include "settings.h"
#include "spiflash.h"
#include "queue.h"
#include "selftest.h"
#include "../flash/spiflash.h"
#include "stm32f4xx_iwdg.h"
#include "../telnet/telnetd.h"
#include "../telnet/shell.h"
#include "../events/events_handler.h"
#include "../webserver/httpd-cgi.h"
#include "settingsfile.h"
#include "debug.h"


#include "../uip/timer.h"
#include "eeprom.h"




xQueueHandle TftpcQueue;
tftp_connection_args args;

/* UDPpcb to be binded with port 69  */
struct uip_udp_conn *tftpc_conn;
u8 buff[TFTPC_MAX_LEN+10];
u16 bufflen;

extern struct tftp_proc_t tftp_proc;

extern int ntwk_wait_and_do;
extern struct timer ntwk_timer;
extern u8 it_is_save;

struct timer tftp_ack_timer;


static void start_updating(void);
static void to_telnet(char *text);
//static u32 make_block(u16 block_num,char *data);

void tftpc_appcall(void){
  u8 *appdata;
  uint16_t ln;
  char *tsize;
  char *blksize;
  u32 tsize_;
  u16 op;


  //DEBUG_MSG(TFTP_DEBUG,"tftpc_appcall\r\n");

  if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
	  return;

  if (uip_newdata()){
	  if (uip_datalen()){
		  appdata = uip_appdata;
		  ln = uip_datalen();
		  DEBUG_MSG(TFTP_DEBUG,"cmdtftp_appcall new data, len = %d\r\n",ln);

		  op = (u16)tftp_decode_op((char *)appdata);
		  switch((u16)op){
		  	  case TFTP_ACK:{
		  		  DEBUG_MSG(TFTP_DEBUG,"input TFTP_ACK\r\n");
			  	  //ответ на запрос
			  	  if(args.op == TFTP_WRQ){
						if(tftp_extract_block((char *)appdata) == args.block){
								//send data
								make_block(args.block,args.data);
								args.data_len = args.tot_bytes - args.block*TFTP_DATA_LEN_MAX;
								if(args.data_len>=TFTP_DATA_LEN_MAX)
									args.data_len=TFTP_DATA_LEN_MAX;
								args.block++;
								if((args.data_len!=0)&&(args.data_len<=TFTP_DATA_LEN_MAX)){
									tftp_send_data_packet(args.block,args.data,args.data_len);
									args.op = TFTP_DATA;
									break;
								}
						}
			  	  }

		  		  if(args.op == TFTP_DATA){
						if(tftp_extract_block((char *)appdata) == args.block){
							if(((args.block)*TFTP_DATA_LEN_MAX)<=args.tot_bytes){
								//send data
								make_block(args.block,args.data);
								args.data_len = args.tot_bytes - args.block*TFTP_DATA_LEN_MAX;
								if(args.data_len>=TFTP_DATA_LEN_MAX)
									args.data_len=TFTP_DATA_LEN_MAX;
								if((args.data_len!=0)&&(args.data_len<=TFTP_DATA_LEN_MAX)){
									tftp_send_data_packet((args.block+1),args.data,args.data_len);
									args.op = TFTP_DATA;
								}
								args.block++;
							}else{
								//shell_prompt(s_[0],"\r\nUploaded compleat.");
								tftp_proc.start = 0;
								tftp_proc.opcode = 0;
							}
						}
		  		  }
		  	  	  }
		  		  break;

		  	  case TFTP_OPTACK:{
		  		args.block 	 = 0;
		  		//ответ на запрос
		  		if(args.op == TFTP_WRQ){
		  			//можем начинать передавать
					//extract options
					blksize = (char *)(appdata+2);
					blksize = tftp_extract_options_blksize(blksize);
					if(strncmp(blksize,TFTP_BLK_512,strlen(TFTP_BLK_512))!=0){
						tftp_send_error_message(TFTP_ERR_INCORRECT_OPTIONS);
						return;
					}
					tsize = blksize+strlen(blksize)+1;
					tsize = tftp_extract_options_tsize(tsize);
					if(strlen(tsize)){
						tsize_ = strtoul(tsize,NULL,10);
						if(tsize_ != args.tot_bytes){
							tftp_send_error_message(TFTP_ERR_INCORRECT_OPTIONS);
							return;
						}
					}else
					{
						tftp_send_error_message(TFTP_ERR_INCORRECT_OPTIONS);
						return;
					}
					//send data
					make_block(args.block,args.data);
					args.data_len = args.tot_bytes - args.block*TFTP_DATA_LEN_MAX;
					if(args.data_len>=TFTP_DATA_LEN_MAX)
						args.data_len=TFTP_DATA_LEN_MAX;
					if((args.data_len!=0)&&(args.data_len<=TFTP_DATA_LEN_MAX)){
						tftp_send_data_packet((args.block+1),args.data,args.data_len);
						args.op = TFTP_DATA;
					}

		  		}
		  		else
		  		{
					args.op = TFTP_OPTACK;
					//extract options
					blksize = (char *)(appdata+2);
					blksize = tftp_extract_options_blksize(blksize);
					if(strncmp(blksize,TFTP_BLK_512,strlen(TFTP_BLK_512))!=0){
						tftp_send_error_message(TFTP_ERR_INCORRECT_OPTIONS);
						return;
					}
					tsize = blksize+strlen(blksize)+1;
					tsize = tftp_extract_options_tsize(tsize);
					if(strlen(tsize)){
						tsize_ = strtoul(tsize,NULL,10);
						if(tsize_ > (FL_TMP_END-FL_TMP_START)){
							tftp_send_error_message(TFTP_ERR_DISKFULL);
							return;
						}
						args.tot_bytes = tsize_;
					}else
					{
						tftp_send_error_message(TFTP_ERR_INCORRECT_OPTIONS);
						return;
					}
			  		//send ack
			  		tftp_send_ack_packet(args.block);
			  		args.block++;
		  		}
		  	    }
		  		break;

			  case TFTP_DATA:{
				//if correct data block
				args.op = TFTP_DATA;
				if(tftp_extract_block((char *)appdata) == args.block){
					//if normal len
					if((ln-4) <= TFTP_DATA_LEN_MAX){
						//write to flash
						tftp_write_flash(appdata+4,args.block,(ln-4));
						//send ACK
						tftp_send_ack_packet(args.block);
						args.block++;
						return;
					}
					else{
						//tftp_send_error_message(TFTP_ERR_UKNOWN_TRANSFER_ID);
						DEBUG_MSG(TFTP_DEBUG,"TFTP block sise > 512\r\n");
						return;
					}
				}
				else{
					tftp_send_error_message(TFTP_ERR_UKNOWN_TRANSFER_ID);
					DEBUG_MSG(TFTP_DEBUG,"TFTP block is incorrect\r\n");
					return;
				}
			    }
				break;

			  case TFTP_ERROR:
				  args.op = TFTP_ERROR;
				  DEBUG_MSG(TFTP_DEBUG,"input TFTP_ERROR\r\n");
				  tftp_proc.opcode = TFTP_ERROR_FILE;
				  break;

		  }
	  }
  }else{
	  if(TftpcQueue){
		  if(uip_arp_out_check(tftpc_conn) == 0){
		  	  if(xQueueReceive(TftpcQueue,buff,0) == pdPASS ){
				  struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
				  m->srcport = HTONS(get_tftp_port());
				  DEBUG_MSG(TFTP_DEBUG,"TFTP send msg to %d.%d.%d.%d, srcport %d,dstport %d\r\n",
						uip_ipaddr1(tftpc_conn->ripaddr),uip_ipaddr2(tftpc_conn->ripaddr),
						uip_ipaddr3(tftpc_conn->ripaddr),uip_ipaddr4(tftpc_conn->ripaddr),
						tftpc_conn->lport,tftpc_conn->rport);
				  uip_send(buff,bufflen);
		  	  }
		  	  else{
		  		  if(args.op == TFTP_RRQ || args.op == TFTP_WRQ){
		  			  if(timer_expired(&tftp_ack_timer)){
		  				  tftp_proc.opcode = TFTP_ERROR_FILE;
		  				  tftp_proc.start = 1;
		  				  tftp_send_error_message(TFTP_ERR_INCORRECT_OPTIONS);
		  				  args.op = 0;
		  			  }
		  		  }
		  	  }
		  }else{
				//send arp
				uip_udp_send(uip_len);
				DEBUG_MSG(TFTP_DEBUG,"tftp not send to %d.%d.%d.%d ,send arp msg\r\n",
					uip_ipaddr1(tftpc_conn->ripaddr),uip_ipaddr2(tftpc_conn->ripaddr),
					uip_ipaddr3(tftpc_conn->ripaddr),uip_ipaddr4(tftpc_conn->ripaddr));
				tftp_proc.opcode = TFTP_ERROR_FILE;
		  }
	  }

  }

}


void tftpc_init(uip_ipaddr_t *addr)
{
  u16 port = 69;

  //uip_ipaddr_t addr;
  //uip_ipaddr(&addr,192,168,0,104);

  port = get_tftp_port();

  //erase all temp place
  /*spi_flash_properties(NULL,NULL,&esize);
  for(u8 i=0;i<(FL_TMP_END/(esize*2));i++){
	  spi_flash_erase(FL_TMP_START+esize*i,esize);
	  IWDG_ReloadCounter();
  }
  */

  /* create a new UDP connection  */
  if(get_tftp_state() == ENABLE){
	  if(tftpc_conn){
		uip_udp_remove(tftpc_conn);
		DEBUG_MSG(TFTP_DEBUG,"remove udp conn tftp\r\n");
	  }
	  tftpc_conn = uip_udp_new (addr, HTONS(/*port*/0));//dst port
	  if (tftpc_conn!=NULL)
	  {
		  uip_udp_bind (tftpc_conn, HTONS(port/*50001*/));
		  DEBUG_MSG(TFTP_DEBUG,"tftp conn created OK\r\n");
		  //create queue if need
		  if(TftpcQueue == 0){
			  TftpcQueue = xQueueCreate(TFTPC_QUEUE_LEN,TFTPC_MAX_LEN);
			  if(TftpcQueue){
				 DEBUG_MSG(TFTP_DEBUG,"TFTP Queue created\r\n");
			  }
			  else{
				 DEBUG_MSG(TFTP_DEBUG,"TFTP Queue alarm\r\n");
				 ADD_ALARM(ERROR_CREATE_TFTP_QUEUE);
				 return;
			  }
		  }
	  }
  }
}



//download
void send_rrq(char *filename,uip_ipaddr_t *to){
u32 esize;
	DEBUG_MSG(TFTP_DEBUG,"TFTP make rrq\r\n");

	//init udp connection
	tftpc_init(to);

	//erase 1mb
	spi_flash_properties(NULL,NULL,&esize);
	for(u8 i=0;i<(FL_TMP_END/(esize*2));i++){
	  spi_flash_erase(FL_TMP_START+esize*i,esize);
	  IWDG_ReloadCounter();
	}

	uip_ipaddr_copy(args.to_ip,to);
	args.op = TFTP_RRQ;
	args.to_port   = get_tftp_port();
	args.block 	 = 0;
	args.tot_bytes = 0;

	args.use_options=0;//dont use options

	uint8_t *end;
	end = buff;
	end = tftp_add_opcode(end,TFTP_RRQ);
	end = tftp_add_filename(end,filename);
	end = tftp_add_type(end,TFTP_TYPE_OCTET);
	if(args.use_options==0){
		//если не используются опции, то blok=1
		args.block = 1;
	}

	if(args.use_options==1){
		//если используются опции
		end = tftp_add_blksize(end,TFTP_BLK_512);
		end = tftp_add_tsize(end,TFTP_0);
	}

	if(TftpcQueue)
		xQueueSend(TftpcQueue,buff,0);

	timer_set(&tftp_ack_timer, 5000*MSEC);

	bufflen = (u16)(end - buff);
}

//upload
void send_wrq(char *filename,uip_ipaddr_t *to, u32 tsize_){
	//char temp[10];
	DEBUG_MSG(TFTP_DEBUG,"TFTP make wrq, len = %lu\r\n",tsize_);

	//init udp connection
	tftpc_init(to);

	uip_ipaddr_copy(args.to_ip,to);
	args.op = TFTP_WRQ;
	args.to_port   = get_tftp_port();
	args.block 	 = 0;
	args.tot_bytes = tsize_;

	uint8_t *end;
	end = buff;
	end = tftp_add_opcode(end,TFTP_WRQ);
	end = tftp_add_filename(end,filename);
	end = tftp_add_type(end,TFTP_TYPE_OCTET);
	/*end = tftp_add_blksize(end,TFTP_BLK_512);
	sprintf(temp,"%lu",tsize_);
	end = tftp_add_tsize(end,temp);*/

	if(TftpcQueue)
		xQueueSend(TftpcQueue,buff,0);

	//start timer
	timer_set(&tftp_ack_timer, 5000*MSEC);

	bufflen = (u16)(end - buff);
}


u8 tftp_write_flash(u8* data,u16 block,u16 size){
u32 esize;
u32 ret;

	if(size<=TFTP_DATA_LEN_MAX)
		spi_flash_write(FL_TMP_START+(block-1)*TFTP_DATA_LEN_MAX, size, data);


	if(((args.use_options)&&((FL_TMP_START+(block-1)*TFTP_DATA_LEN_MAX+size) >= args.tot_bytes))||
			((args.use_options==0)&&(size<TFTP_DATA_LEN_MAX))){

		tftp_proc.wait_time = 0;

		//download config
		if(it_is_save==1){
			it_is_save=0;
			ret=parse_bak_file();
			DEBUG_MSG(TFTP_DEBUG,"valid_var %lu\r\n",ret);

			if(ret){
				tftp_proc.opcode = TFTP_ERROR_FILE;
				DEBUG_MSG(TFTP_DEBUG,"TFTP_ERROR_FILE\r\n");
				settings_load();
				nosave();
			}
		    else{
				tftp_proc.opcode = TFTP_BACKUPING;
				DEBUG_MSG(TFTP_DEBUG,"TFTP_BACKUPING\r\n");
				tftp_proc.wait_time = 0;
				settings_save();
				timer_set(&ntwk_timer, 5000*MSEC);
				ntwk_wait_and_do = 3;
			}
		}
		else{
		//download firmware
			esize = TestFirmware();
			//DEBUG_MSG(TFTP_DEBUG,"TestFirmware %lu\r\n",esize);
			if(esize == 0){
				start_updating();
				tftp_proc.opcode = TFTP_UPDATING;
				tftp_proc.wait_time = 0;
				tftp_proc.start = 1;
			}else{
				tftp_proc.opcode = TFTP_ERROR_FILE;
			}
			//DEBUG_MSG(TFTP_DEBUG,"TestFirmware %lu\r\n",esize);
		}
	}
	return 0;
}


static void start_updating(void){
u32 Tmp;
	 //взводим флажок
	  need_update_flag[0] = NEED_UPDATE_YES;
	  //получаем версию новой прошивки
	  Tmp = get_new_fw_vers();
	  send_events_u32(EVENT_UPDATE_FIRMWARE,(u32)Tmp);
	  timer_set(&ntwk_timer, 10000*MSEC);
	  ntwk_wait_and_do = 2;
	  DEBUG_MSG(TFTP_DEBUG,"start_updating\r\n");
	  to_telnet("\r\nDownloading compleat.");
	  to_telnet("\r\nUpdating start, wait...");
}

static void to_telnet(char *text){
	//if(get_telnet_state())
	//	shell_prompt(text);
}

u32 make_block(u16 block_num,char *data){
u32 offset;
	DEBUG_MSG(TFTP_DEBUG,"make_block:start\r\n");
	offset = FL_TMP_START + block_num*TFTP_DATA_LEN_MAX;
	//spi_flash_properties(NULL,NULL,&esize);
	//DEBUG_MSG(TFTP_DEBUG,"make_block:opsize %lu\r\n",esize);
	spi_flash_read(offset,TFTP_DATA_LEN_MAX,data);
	DEBUG_MSG(TFTP_DEBUG,"make_block: [%d] [%lu],%s \r\n",block_num,offset,data);
	return TFTP_DATA_LEN_MAX;
}
