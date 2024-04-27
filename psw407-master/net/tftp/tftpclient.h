#ifndef TFTPCLIENT_H_
#define TFTPCLIENT_H_

#define TFTPC_QUEUE_LEN	2
#define TFTPC_MAX_LEN	128

#define TFTP_TYPE_OCTET 	"octet"

#define TFTP_BLKSIZE 		"blksize"
#define TFTP_TSIZE 			"tsize"

#define TFTP_BLK_512 		"512"

#define TFTP_0 				"0"

void tftpc_init(uip_ipaddr_t *addr);
void send_rrq(char *filename,uip_ipaddr_t *to);
void send_wrq(char *filename,uip_ipaddr_t *to, u32 tsize_);
u8 tftp_write_flash(u8* data,u16 block,u16 size);
void tftpc_appcall(void);
u32 make_block(u16 block_num,char *data);
#endif /* TFTPCLIENT_H_ */
