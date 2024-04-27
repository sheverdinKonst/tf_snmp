#ifndef TELEPORT_H_
#define TELEPORT_H_

//#define TLP_UDP_PORT 6124

#define TLP_UDP_PORT_IN 	6124
#define TLP_UDP_PORT_OUT  	6125


#define TLP_RS485_MSG	128//максимальный размер фрейма по rs-485

#define TLP_UDP_PORT_NUM	MAX_REMOTE_TLP//число зарезервированных UDP портов

struct teleport_state {
  struct psock  sout;	//my
  char inputbuf[64];
  u8 connected;//флаг подключения
  //struct uip_conn *conn;//указатель на структуру подключения по tcp
  //struct uip_udp_conn *udp_conn;//указатель на подключение по UDP
  u8_t state;
  uip_ipaddr_t ip;
  u16 net_port;


//  char msg[256+10];
  char *msg;//
  u16_t msglen;
  u16_t sentlen, textlen;
  u16_t sendptr;
};

//структура, описывающая управляющий пакет
struct pkt_output{
	u32 crc;
	u8 type;//тип сообщения 0- input, 1 - rs485
	u32 timestamp;//метка времени
	u8 input;//номер входа
	u8 out;//номер выхода удалённого устройства
	u8 out_state;//состояние выхода
	u16 len;//длина данных
	u8  data[TLP_RS485_MSG];//данные для отправки по rs485
};

void teleport_out_udp_appcall(void);
void teleport_in_udp_appcall(void);

void teleport_init(void);
void  teleport_send(struct teleport_state *s);
int send_teleport_input_msg(u8 input, u8 state);

void newdata_teleport(void);
void teleport_task(void *pvParameters);

//u8 get_tlp_num_by_port(u16 rport);
void teleport_processing(void);

#endif /* TELEPORT_H_ */
