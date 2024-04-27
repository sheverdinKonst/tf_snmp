//#define USE_STP
//#include "stp_oslayer_tos32.h"
#include "stp_oslayer_freertos.h"

//void stp_task_sleep(uint32_t ticks);
void get_haddr(uint8_t *mac);
int stp_send(int port, uint8_t *buf, int len);
void stp_set_port_state(int port, int state);
int stp_ethhdr2port(uint8_t *ptr);

int bstp_sem_take(void);
int bstp_sem_free(void);

typedef struct{
  uint8_t id;
#define BSTP_EVENT_STATE_CHANGE 1
#define BSTP_EVENT_REINIT       2
#define BSTP_EVENT_DUMP         3
#define BSTP_EVENT_CHANGED      4
#define BSTP_EVENT_LINK_CHANGE  5
  uint8_t port;
} bstp_event_t;

#define BSTP_EVENT STATE_CHANGE
int bstp_send_event(bstp_event_t *event);
int bstp_send_event_i(bstp_event_t *event);
int bstp_recv_event(bstp_event_t *event, unsigned long tout);

void flush_db(int port);
void stp_if_poll(int port, int *link, int *duplex, uint32_t *baudrate);

unsigned long bstp_getticks(void);
unsigned long bstp_getdeltatick(unsigned long ticks);

