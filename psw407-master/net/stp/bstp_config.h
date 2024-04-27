
#define BSTP_CFG_MAGIC 0xFEC1
/*
 * это продублированов структуре settings
 *
typedef struct{
  unsigned char enable;
  unsigned char state;
  unsigned char priority;
  unsigned long path_cost;
  unsigned char flags;
#define BSTP_PORTCFG_FLAG_ADMCOST  (1<<0)
#define BSTP_PORTCFG_FLAG_AUTOEDGE (1<<1)
#define BSTP_PORTCFG_FLAG_EDGE     (1<<2)
#define BSTP_PORTCFG_FLAG_AUTOPTP  (1<<3)
#define BSTP_PORTCFG_FLAG_PTP      (1<<4)
}bstp_port_cfg_t;

typedef struct{
  unsigned short magic;
  unsigned char  proto;
  unsigned short bridge_priority;
  unsigned short bridge_max_age;
  unsigned short bridge_htime;
  unsigned short bridge_fdelay;
  unsigned short bridge_mdelay;
  unsigned char  txholdcount;
}bstp_cfg_t;
*/

void bstp_cfg_reset(void);

void bstp_load_cfg(/*bstp_cfg_t *cfg*/void);
void bstp_save_cfg(/*bstp_cfg_t *cfg*/void);
void bstp_load_portcfg(/*int port, bstp_port_cfg_t *cfg*/void);
void bstp_save_portcfg(/*int port, bstp_port_cfg_t *cfg*/void);
