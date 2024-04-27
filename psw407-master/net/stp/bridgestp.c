
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bstp.h"
#include "../syslog/syslog.h"
#include "../syslog/cfg_syslog.h"
#include "../syslog/msg_build.h"
#include "../smtp/smtp.h"
#include "board.h"
#include "settings.h"
#include "../events/events_handler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"
#ifdef USE_STP



#define splnet() 0
#define splx(a)

#if BRIDGESTP_DEBUG
unsigned long deb_ticks;
int deb_tick;
int deb_trans;
int deb_src;
#endif

#ifdef NUM_BSTP_PORT
	#undef NUM_BSTP_PORT
	#define NUM_BSTP_PORT (ALL_PORT_NUM)
#endif


struct bstp_state bstp_state;
struct bstp_port bstp_ports[PORT_NUM];
struct portnet portnets[PORT_NUM];

const u_int8_t bstp_etheraddr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

void	bstp_transmit(struct bstp_state *, struct bstp_port *);
void	bstp_transmit_bpdu(struct bstp_state *, struct bstp_port *);
void	bstp_transmit_tcn(struct bstp_state *, struct bstp_port *);
void	bstp_set_port_role(struct bstp_port *, int);
void bstp_update_roles(struct bstp_state *bs, struct bstp_port *bp);
void bstp_input(struct bstp_state *bs, struct bstp_port *bp, struct ether_header *eh, uint8_t *m);
void bstp_update_state(struct bstp_state *bs, struct bstp_port *bp);

static void bstp_dump_(void){

#if STP_DEBUG
  int i;
  struct bstp_port *bp;
  struct portnet *ifp;
  uint8_t *ptr;
    DEBUG_MSG(STP_DEBUG,"\r\n");
    if (bstp_state.isroot)
    	DEBUG_MSG(STP_DEBUG,"ROOT Bridge\r\n");

    if (bstp_state.bs_root_port)
    	DEBUG_MSG(STP_DEBUG,"root port: %s(%d)\r\n", bstp_state.bs_root_port->bp_ifp->name,
    			bstp_state.bs_root_port->id);

#if STP_U64
    ptr = (uint8_t *)&bstp_state.bs_root_pv.pv_root_id;
#else
    ptr = (uint8_t *)bstp_state.bs_root_pv.pv_root_id;
#endif



    DEBUG_MSG(STP_DEBUG,"\trootid: ");
    for(int i=0;i<8;i++)
    	DEBUG_MSG(STP_DEBUG,"%02x", ptr[i]); DEBUG_MSG(STP_DEBUG,"\n\r");
    DEBUG_MSG(STP_DEBUG,"\r\npv_cost: %lu\r\n", bstp_state.bs_root_pv.pv_cost);
#if STP_U64
    ptr = (uint8_t *)&bstp_state.bs_root_pv.pv_dbridge_id;
#else
    ptr = (uint8_t *)bstp_state.bs_root_pv.pv_dbridge_id;
#endif
    DEBUG_MSG(STP_DEBUG,"\t dbridge_id: ");
    for(int i=0;i<8;i++)
    	DEBUG_MSG(STP_DEBUG,"%02x", ptr[i]); DEBUG_MSG(STP_DEBUG,"\n\r");
    DEBUG_MSG(STP_DEBUG,"\r\n\t dport_id: %04x\r\n", bstp_state.bs_root_pv.pv_dport_id);
    DEBUG_MSG(STP_DEBUG,"\t port_id: %04x\r\n", bstp_state.bs_root_pv.pv_port_id);

    DEBUG_MSG(STP_DEBUG,"\r\n");

    for (i=0;i<NUM_BSTP_PORT;i++){
      bp = &bstp_ports[i];
      if (bp->bp_active==0) continue;
      ifp = bp->bp_ifp;
      DEBUG_MSG(STP_DEBUG,"%s(%d):\r\n", bp->bp_ifp->name, bp->id);
      DEBUG_MSG(STP_DEBUG,"\t flags: ");
      if (ifp->if_link_state&LINK_STATE_DOWN) DEBUG_MSG(STP_DEBUG," LinkDown");
      if (ifp->if_link_state&LINK_STATE_FULL_DUPLEX) DEBUG_MSG(STP_DEBUG," FullDuplex");
      DEBUG_MSG(STP_DEBUG,"\r\n");

      DEBUG_MSG(STP_DEBUG,"\t port_id: %04x", bp->bp_port_id);
      DEBUG_MSG(STP_DEBUG,"\t priority: %d", bp->bp_priority);
      DEBUG_MSG(STP_DEBUG,"\t path_cost: %lu", bp->bp_path_cost);
      DEBUG_MSG(STP_DEBUG,"\r\n");

      DEBUG_MSG(STP_DEBUG,"\t state: ");
      switch(bp->bp_state){
        case BSTP_IFSTATE_FORWARDING: DEBUG_MSG(STP_DEBUG,"forwarding "); break;
        case BSTP_IFSTATE_DISABLED:   DEBUG_MSG(STP_DEBUG,"disabled  "); break;
        case BSTP_IFSTATE_LISTENING:  DEBUG_MSG(STP_DEBUG,"listn"); break;
        case BSTP_IFSTATE_LEARNING:   DEBUG_MSG(STP_DEBUG,"learn"); break;
        case BSTP_IFSTATE_BLOCKING:   DEBUG_MSG(STP_DEBUG,"block"); break;
        case BSTP_IFSTATE_DISCARDING: DEBUG_MSG(STP_DEBUG,"discarding "); break;
      }
      DEBUG_MSG(STP_DEBUG,"\t role: ");
      switch(bp->bp_role){
        case BSTP_ROLE_DISABLED:   DEBUG_MSG(STP_DEBUG,"disabled "); break;
        case BSTP_ROLE_ROOT:       DEBUG_MSG(STP_DEBUG,"root"); break;
        case BSTP_ROLE_DESIGNATED: DEBUG_MSG(STP_DEBUG,"dsgn"); break;
        case BSTP_ROLE_ALTERNATE:  DEBUG_MSG(STP_DEBUG,"alternate "); break;
        case BSTP_ROLE_BACKUP:     DEBUG_MSG(STP_DEBUG,"backup"); break;
      }
      DEBUG_MSG(STP_DEBUG,"  ");
      if (bp->bp_ptp_link){ DEBUG_MSG(STP_DEBUG," ptp"); }else {DEBUG_MSG(STP_DEBUG,"    ");}
      if (bp->bp_operedge){ DEBUG_MSG(STP_DEBUG," edge"); }else {DEBUG_MSG(STP_DEBUG,"     ");}
      if (bp->bp_sync){ DEBUG_MSG(STP_DEBUG," sync"); }else { DEBUG_MSG(STP_DEBUG,"     ");}
      if (bp->bp_synced){ DEBUG_MSG(STP_DEBUG," synced"); }else {DEBUG_MSG(STP_DEBUG,"       ");}
      if (bp->bp_proposing){ DEBUG_MSG(STP_DEBUG," proposing"); }else{ DEBUG_MSG(STP_DEBUG,"          ");}
      if (bp->bp_proposed){ DEBUG_MSG(STP_DEBUG," proposed"); }else {DEBUG_MSG(STP_DEBUG,"         ");}
      if (bp->bp_agree){ DEBUG_MSG(STP_DEBUG," agree"); }else {DEBUG_MSG(STP_DEBUG,"      ");}
      if (bp->bp_agreed){ DEBUG_MSG(STP_DEBUG," agreed");} else {DEBUG_MSG(STP_DEBUG,"       ");}

      DEBUG_MSG(STP_DEBUG,"\r\n");
      DEBUG_MSG(STP_DEBUG,"\t txcount: %3d", bp->bp_txcounter);
      DEBUG_MSG(STP_DEBUG,"\t rxcount: %3d", bp->bp_rxcounter);
      DEBUG_MSG(STP_DEBUG,"\r\n");
    }
#endif
}

void bstp_rawinput(uint8_t *ptr, int size){
  struct ether_header *eh;
  struct bstp_port *bp;
  int port = -1;
  int i;
  eh = (struct ether_header *)ptr;

  port = stp_ethhdr2port(ptr);
  DEBUG_MSG(STP_DEBUG,"bstp_read port = %d\r\n", port);
  for (i=0;i<NUM_BSTP_PORT;i++){
  	  bp = &bstp_ports[i];
  	  if (bp->id==port)
  		  break;
  }
  if (i==NUM_BSTP_PORT)
	  return;

  bstp_sem_take();
#if BRIDGESTP_DEBUG
  deb_src = 1;
  if (deb_tick<4||deb_trans){
    struct bstp_cbpdu *cpdu = (struct bstp_cbpdu *)(ptr+sizeof(*eh));
    DEBUG_MSG(STP_DEBUG,"\r\n");
    DEBUG_MSG(STP_DEBUG,"input %s(%d), flags: %02x!!! type = %d \r\n", bp->bp_ifp->name, bp->id, cpdu->cbu_flags, cpdu->cbu_bpdutype);
  }
#endif
  bp->bp_rxcounter++;
  bstp_input(&bstp_state, bp, eh, ptr+sizeof(*eh));
  bstp_sem_free();
}

int bstp_send(int port, uint8_t *buf, int len){
  return stp_send(port, buf, len);
}

void
bstp_timer_start(struct bstp_timer *t, u_int16_t v)
{
	t->value = v;
	t->active = 1;
	t->latched = 0;
}

void
bstp_timer_stop (struct bstp_timer *t)
{
	t->value = 0;
	t->active = 0;
	t->latched = 0;
}

void
bstp_timer_latch(struct bstp_timer *t)
{
	t->latched = 1;
	t->active = 1;
}

int
bstp_timer_expired(struct bstp_timer *t)
{
  if (t->active == 0 || t->latched)
    return (0);
//  t->value -= BSTP_TICK_VAL;
  if (t->value)
	  t->value --;
  if (t->value <= 0) {
    bstp_timer_stop(t);
    return (1);
  }
  return (0);
} 


int bstp_addr_cmp(const u_int8_t *a, const u_int8_t *b) {
  int i, d;

  for (i = 0, d = 0; i < ETHER_ADDR_LEN && d == 0; i++) { 
    d = ((int)a[i]) - ((int)b[i]);
  }

  return (d);
}

/*
 * compare the bridge address component of the bridgeid
 */
/*
 * uint64 vers
int bstp_same_bridgeid(u_int64_t id1, u_int64_t id2) {
  u_char addr1[ETHER_ADDR_LEN];
  u_char addr2[ETHER_ADDR_LEN];

  PV2ADDR(id1, addr1);
  PV2ADDR(id2, addr2);

  if (bstp_addr_cmp(addr1, addr2) == 0)
    return (1);

  return (0);
}*/

//uint32 vers
#if STP_U64
int bstp_same_bridgeid(u_int64_t id1, u_int64_t id2) {
#else
int bstp_same_bridgeid(u_int32_t *id1, u_int32_t *id2) {
#endif
  u_char addr1[ETHER_ADDR_LEN];
  u_char addr2[ETHER_ADDR_LEN];

  PV2ADDR(id1, addr1);
  PV2ADDR(id2, addr2);

  if (bstp_addr_cmp(addr1, addr2) == 0)
    return (1);

  return (0);
}


int bstp_info_cmp(struct bstp_pri_vector *pv,  struct bstp_pri_vector *cpv) {


#if STP_U64
  if (cpv->pv_root_id < pv->pv_root_id)
	  return (INFO_BETTER);
  if (cpv->pv_root_id > pv->pv_root_id)
	  return (INFO_WORSE);
#else
  //замещение работы с 64битной переменной
   if (cpv->pv_root_id[1] < pv->pv_root_id[1]){
 	  return (INFO_BETTER);
   } else if(cpv->pv_root_id[1] ==  pv->pv_root_id[1]){
 	   if (cpv->pv_root_id[0] < pv->pv_root_id[0])
 		  return (INFO_BETTER);
   }

   if (cpv->pv_root_id[1] > pv->pv_root_id[1])
 	  return (INFO_WORSE);
   else if (cpv->pv_root_id[1] == pv->pv_root_id[1])
 	  if (cpv->pv_root_id[0] > pv->pv_root_id[0])
 		  return (INFO_WORSE);
#endif

  if (cpv->pv_cost < pv->pv_cost)
    return (INFO_BETTER);
  if (cpv->pv_cost > pv->pv_cost)
    return (INFO_WORSE);

#if STP_U64
  if (cpv->pv_dport_id < pv->pv_dport_id)
    return (INFO_BETTER);
  if (cpv->pv_dport_id > pv->pv_dport_id)
    return (INFO_WORSE);
#else
  if (cpv->pv_dbridge_id[1] < pv->pv_dbridge_id[1])
	  return (INFO_BETTER);
  else if (cpv->pv_dbridge_id[1] == pv->pv_dbridge_id[1]){
	  if (cpv->pv_dbridge_id[0] < pv->pv_dbridge_id[0])
		  return (INFO_BETTER);
  }

  if (cpv->pv_dbridge_id[1] > pv->pv_dbridge_id[1])
	  return (INFO_WORSE);
  else if (cpv->pv_dbridge_id[1] == pv->pv_dbridge_id[1]){
	  if (cpv->pv_dbridge_id[0] > pv->pv_dbridge_id[0])
		  return (INFO_WORSE);
  }
#endif

  return (INFO_SAME);
}

int bstp_pdu_bettersame(struct bstp_port *bp, int newinfo) {
  if (newinfo == BSTP_INFO_RECEIVED &&
      bp->bp_infois == BSTP_INFO_RECEIVED &&
      bstp_info_cmp(&bp->bp_port_pv, &bp->bp_msg_cu.cu_pv) >= INFO_SAME)
    return (1);

  if (newinfo == BSTP_INFO_MINE &&
      bp->bp_infois == BSTP_INFO_MINE &&
      bstp_info_cmp(&bp->bp_port_pv, &bp->bp_desg_pv) >= INFO_SAME)
    return (1);

  return (0);
}

/*
 * This message priority vector is superior to the port priority vector and
 * will replace it if, and only if, the message priority vector is better than
 * the port priority vector, or the message has been transmitted from the same
 * designated bridge and designated port as the port priority vector.
 */
int bstp_info_superior(struct bstp_pri_vector *pv,
    struct bstp_pri_vector *cpv) {
  if (bstp_info_cmp(pv, cpv) == INFO_BETTER ||
      (bstp_same_bridgeid(pv->pv_dbridge_id, cpv->pv_dbridge_id) &&
      (cpv->pv_dport_id & 0xfff) == (pv->pv_dport_id & 0xfff)))
    return (1);
  return (0);
}

void bstp_update_info(struct bstp_port *bp) {
  struct bstp_state *bs = bp->bp_bs;
  if (bs==NULL) return;

  bp->bp_proposing = 0;
  bp->bp_proposed = 0;

  if (bp->bp_agreed && !bstp_pdu_bettersame(bp, BSTP_INFO_MINE))
    bp->bp_agreed = 0;

  if (bp->bp_synced && !bp->bp_agreed /*&& !bp->bp_operedge*/) { //!!!!!
    bp->bp_synced = 0;
    bs->bs_allsynced = 0;
  }

  /* copy the designated pv to the port */
  bp->bp_port_pv = bp->bp_desg_pv;
  bp->bp_port_msg_age = bp->bp_desg_msg_age;
  bp->bp_port_max_age = bp->bp_desg_max_age;
  bp->bp_port_fdelay = bp->bp_desg_fdelay;
  bp->bp_port_htime = bp->bp_desg_htime;
  bp->bp_infois = BSTP_INFO_MINE;

  /* Set transmit flag but do not immediately send */
  bp->bp_flags |= BSTP_PORT_NEWINFO;
}

void bstp_assign_roles(struct bstp_state *bs) {
  struct bstp_port *bp, *rbp = NULL;
  struct bstp_pri_vector pv;

  /* default to our priority vector */
  bs->bs_root_pv = bs->bs_bridge_pv;
  bs->bs_root_msg_age = 0;
  bs->bs_root_max_age = bs->bs_bridge_max_age;
  bs->bs_root_fdelay = bs->bs_bridge_fdelay;
  bs->bs_root_htime = bs->bs_bridge_htime;
  bs->bs_root_port = NULL;
  bs->isroot = 1;

  /* check if any received info supersedes us */
  for (int i=0;i<NUM_BSTP_PORT;i++){
    if (bstp_ports[i].bp_infois!=BSTP_INFO_RECEIVED) continue;
    bp = &bstp_ports[i];
     
    pv = bp->bp_port_pv;
    pv.pv_cost += bp->bp_path_cost;

    /*
     * The root priority vector is the best of the set comprising
     * the bridge priority vector plus all root path priority
     * vectors whose bridge address is not equal to us.
     */
    if (bstp_same_bridgeid(pv.pv_dbridge_id,
        bs->bs_bridge_pv.pv_dbridge_id) == 0 &&
        bstp_info_cmp(&bs->bs_root_pv, &pv) == INFO_BETTER) {
      /* the port vector replaces the root */
      bs->bs_root_pv = pv;
      bs->bs_root_msg_age = bp->bp_port_msg_age +
          BSTP_MESSAGE_AGE_INCR;
      bs->bs_root_max_age = bp->bp_port_max_age;
      bs->bs_root_fdelay = bp->bp_port_fdelay;
      bs->bs_root_htime = bp->bp_port_htime;
      rbp = bp;
      bs->isroot = 0;
    }
  }

  for (int i=0;i<NUM_BSTP_PORT;i++){
    bp = &bstp_ports[i];
    if (bp->bp_active==0) continue;
    /* calculate the port designated vector */
#if STP_U64
    bp->bp_desg_pv.pv_root_id = bs->bs_root_pv.pv_root_id;
#else
    bp->bp_desg_pv.pv_root_id[0] = bs->bs_root_pv.pv_root_id[0];
    bp->bp_desg_pv.pv_root_id[1] = bs->bs_root_pv.pv_root_id[1];
#endif

    bp->bp_desg_pv.pv_cost = bs->bs_root_pv.pv_cost;
#if STP_U64
    bp->bp_desg_pv.pv_dbridge_id = bs->bs_bridge_pv.pv_dbridge_id;
#else
    bp->bp_desg_pv.pv_dbridge_id[0] = bs->bs_bridge_pv.pv_dbridge_id[0];
    bp->bp_desg_pv.pv_dbridge_id[1] = bs->bs_bridge_pv.pv_dbridge_id[1];
#endif



    bp->bp_desg_pv.pv_dport_id = bp->bp_port_id;
    bp->bp_desg_pv.pv_port_id = bp->bp_port_id;

    /* calculate designated times */
    bp->bp_desg_msg_age = bs->bs_root_msg_age;
    bp->bp_desg_max_age = bs->bs_root_max_age;
    bp->bp_desg_fdelay = bs->bs_root_fdelay;
    bp->bp_desg_htime = bs->bs_bridge_htime;


    switch (bp->bp_infois) {
    case BSTP_INFO_DISABLED:
      bstp_set_port_role(bp, BSTP_ROLE_DISABLED);
      break;

    case BSTP_INFO_AGED:
      bstp_set_port_role(bp, BSTP_ROLE_DESIGNATED);
      bstp_update_info(bp);
      break;

    case BSTP_INFO_MINE:
      bstp_set_port_role(bp, BSTP_ROLE_DESIGNATED);
      /* update the port info if stale */
#if 0
      if (bstp_info_cmp(&bp->bp_port_pv,
          &bp->bp_desg_pv) != INFO_SAME ||
          (rbp != NULL &&
          (bp->bp_port_msg_age != rbp->bp_port_msg_age ||
          bp->bp_port_max_age != rbp->bp_port_max_age ||
          bp->bp_port_fdelay != rbp->bp_port_fdelay ||
          bp->bp_port_htime != rbp->bp_port_htime)))
        bstp_update_info(bp);
#else
      if (bstp_info_cmp(&bp->bp_port_pv,
          &bp->bp_desg_pv) != INFO_SAME ||
          (rbp != NULL &&
          (bp->bp_port_msg_age != bp->bp_desg_msg_age ||
          bp->bp_port_max_age != bp->bp_desg_max_age ||
          bp->bp_port_fdelay != bp->bp_desg_fdelay ||
          bp->bp_port_htime != bp->bp_desg_htime)))
          bstp_update_info(bp);
#endif
      break;

    case BSTP_INFO_RECEIVED:
      if (bp == rbp) {
        /*
         * root priority is derived from this
         * port, make it the root port.
         */
        bstp_set_port_role(bp, BSTP_ROLE_ROOT);
        bs->bs_root_port = bp;
      } else if (bstp_info_cmp(&bp->bp_port_pv,
            &bp->bp_desg_pv) == INFO_BETTER) {
        /*
         * the port priority is lower than the root
         * port.
         */
        bstp_set_port_role(bp, BSTP_ROLE_DESIGNATED);
        bstp_update_info(bp);
      } else {
        if (bstp_same_bridgeid(
            bp->bp_port_pv.pv_dbridge_id,
            bs->bs_bridge_pv.pv_dbridge_id)) {
          /*
           * the designated bridge refers to
           * another port on this bridge.
           */
          bstp_set_port_role(bp,
              BSTP_ROLE_BACKUP);
        } else {
          /*
           * the port is an inferior path to the
           * root bridge.
           */
          bstp_set_port_role(bp,
              BSTP_ROLE_ALTERNATE);
        }
      }
      break;
    }
  }
}

/*
 * Calculate the path cost according to the link speed.
 */
/*4 Mbit/s	250	5,000,000
10 Mbit/s	100	2,000,000
16 Mbit/s	62	1,250,000
100 Mbit/s	19	200,000
1 Gbit/s	4	20,000
2 Gbit/s	3	10,000
10 Gbit/s	2	2,000*/

u_int32_t bstp_calc_path_cost(struct bstp_port *bp) {
  struct portnet *ifp = bp->bp_ifp;
  u_int32_t path_cost;

  //DEBUG_MSG(STP_DEBUG,"bstp_calc_path_cost1 %s\r\n", bp->bp_ifp->name);
  /* If the priority has been manually set then retain the value */
  if (bp->bp_flags & BSTP_PORT_ADMCOST)
	//DEBUG_MSG(STP_DEBUG,"bstp_calc_path_cost MANUAL\r\n");
    return bp->bp_path_cost;

  if (ifp->if_baudrate < 1000)
    return (BSTP_DEFAULT_PATH_COST);

  if(bp ->bp_protover==BSTP_PROTO_RSTP){
	//DEBUG_MSG(STP_DEBUG,"bstp_calc_path_cost2 %s: %lu\r\n", bp->bp_ifp->name, ifp->if_baudrate);
	  /* formula from section 17.14, IEEE Std 802.1D-2004 */
	  path_cost = 20000000UL / (ifp->if_baudrate / 1000);
	  //DEBUG_MSG(STP_DEBUG,"bstp_calc_path_cost3 %s: %lu\r\n", bp->bp_ifp->name, path_cost);
  }else{
	  // if STP
	  switch(ifp->if_baudrate){
		  case 10000: path_cost=100;break;
		  case 100000: path_cost=19;break;
		  case 1000000: path_cost=4;break;
		  default:path_cost=BSTP_DEFAULT_PATH_COST;
	  }
  }

  if (path_cost > BSTP_MAX_PATH_COST)
	path_cost = BSTP_MAX_PATH_COST;

  /* STP compat mode only uses 16 bits of the 32 */
  if (bp->bp_protover == BSTP_PROTO_STP && path_cost > 65535)
    path_cost = 65535;

  return (path_cost);
}

void bstp_hello_timer_expiry(struct bstp_state *bs, struct bstp_port *bp) {
  if ((bp->bp_flags & BSTP_PORT_NEWINFO) ||
      bp->bp_role == BSTP_ROLE_DESIGNATED ||
      (bp->bp_role == BSTP_ROLE_ROOT &&
       bp->bp_tc_timer.active == 1)) {
    bstp_timer_start(&bp->bp_hello_timer, bp->bp_desg_htime);
    bp->bp_flags |= BSTP_PORT_NEWINFO;
    bstp_transmit(bs, bp);
  }
}

void bstp_message_age_expiry(struct bstp_state *bs, struct bstp_port *bp) {
  DEBUG_MSG(STP_DEBUG,"%s %s(%d)\r\n", __FUNCTION__, bp->bp_ifp->name, bp->id);
  if (bp->bp_infois == BSTP_INFO_RECEIVED) {
    bp->bp_infois = BSTP_INFO_AGED;
    bstp_assign_roles(bs);
    DEBUG_MSG(STP_DEBUG,"aged info on %s(%d)\r\n", bp->bp_ifp->name, bp->id);
  }
}

void bstp_migrate_delay_expiry(struct bstp_state *bs, struct bstp_port *bp){
  DEBUG_MSG(STP_DEBUG,"%s %s(%d)\r\n", __FUNCTION__, bp->bp_ifp->name, bp->id);
  bp->bp_flags |= BSTP_PORT_CANMIGRATE;
}

void bstp_edge_delay_expiry(struct bstp_state *bs, struct bstp_port *bp){
  DEBUG_MSG(STP_DEBUG,"%s %s(%d)\r\n", __FUNCTION__, bp->bp_ifp->name, bp->id);
  DEBUG_MSG(STP_DEBUG,"flags: %lx, prop: %d, role: %d\r\n", bp->bp_flags, bp->bp_proposing, bp->bp_role);
  if ((bp->bp_flags & BSTP_PORT_AUTOEDGE) &&
      bp->bp_protover == BSTP_PROTO_RSTP && bp->bp_proposing &&
      bp->bp_role == BSTP_ROLE_DESIGNATED){
    bp->bp_operedge = 1;
    DEBUG_MSG(STP_DEBUG,"bp_operedge=1 %s(%d)\r\n", bp->bp_ifp->name, bp->id);
  }
}

uint8_t send_buf[512];

void bstp_send_bpdu(struct bstp_state *bs, struct bstp_port *bp, struct bstp_cbpdu *bpdu) {
  struct portnet *ifp = bp->bp_ifp;
  uint8_t *m;
  struct ether_header *eh;
  int s, len, error;

  s = splnet();
  if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0)
    goto done;
#if 0
#ifdef ALTQ
  if (!ALTQ_IS_ENABLED(&ifp->if_snd))
#endif
  if (IF_QFULL(&ifp->if_snd))
    goto done;
#endif
  
  m = send_buf;
  len = 0;
  eh = (struct ether_header *)m;

  bpdu->cbu_ssap = bpdu->cbu_dsap = LLC_8021D_LSAP;
  bpdu->cbu_ctl = LLC_UI;
  bpdu->cbu_protoid = htons(BSTP_PROTO_ID);

  get_haddr(eh->ether_shost);
  memcpy(eh->ether_dhost, bstp_etheraddr, ETHER_ADDR_LEN);

  switch (bpdu->cbu_bpdutype) {
  case BSTP_MSGTYPE_CFG:
    bpdu->cbu_protover = BSTP_PROTO_STP;
    len = sizeof(*eh) + BSTP_BPDU_STP_LEN; 
    eh->ether_type = htons(BSTP_BPDU_STP_LEN);
    memcpy(m + sizeof(*eh), bpdu, BSTP_BPDU_STP_LEN);
    break;
  case BSTP_MSGTYPE_RSTP:
    bpdu->cbu_protover = BSTP_PROTO_RSTP;
    bpdu->cbu_versionlen = htons(0);
    len = sizeof(*eh) + BSTP_BPDU_RSTP_LEN;
    eh->ether_type = htons(BSTP_BPDU_RSTP_LEN);
    memcpy(m + sizeof(*eh), bpdu, BSTP_BPDU_RSTP_LEN);
    break;
  /*default:
    panic("not implemented");*/
  }

  bp->bp_txcount++;
  bp->bp_txcounter++;

  error = bstp_send(bp->id, m, len);
/*  if (error == 0) {
    ifp->if_obytes += len;
    ifp->if_omcasts++;
  }*/
 done:
  splx(s);
}

int bstp_pdu_flags(struct bstp_port *bp) {
  int flags = 0;

  if (bp->bp_proposing && bp->bp_state != BSTP_IFSTATE_FORWARDING)
    flags |= BSTP_PDU_F_P;

  if (bp->bp_agree)
    flags |= BSTP_PDU_F_A;

  if (bp->bp_tc_timer.active)
    flags |= BSTP_PDU_F_TC;

  if (bp->bp_tc_ack)
    flags |= BSTP_PDU_F_TCA;

  switch (bp->bp_state) {
  case BSTP_IFSTATE_LEARNING:
    flags |= BSTP_PDU_F_L;
    break;
  case BSTP_IFSTATE_FORWARDING:
    flags |= (BSTP_PDU_F_L | BSTP_PDU_F_F);
    break;
  }

  switch (bp->bp_role) {
  case BSTP_ROLE_ROOT:
    flags |= (BSTP_PDU_F_ROOT << BSTP_PDU_PRSHIFT);
    break;
  case BSTP_ROLE_ALTERNATE:
  case BSTP_ROLE_BACKUP:
    flags |= (BSTP_PDU_F_ALT << BSTP_PDU_PRSHIFT);
    break;
  case BSTP_ROLE_DESIGNATED:
    flags |= (BSTP_PDU_F_DESG << BSTP_PDU_PRSHIFT);
    break;
  }

  /* Strip off unused flags in either mode */
  switch (bp->bp_protover) {
  case BSTP_PROTO_STP:
    flags &= BSTP_PDU_STPMASK;
    break;
  case BSTP_PROTO_RSTP:
    flags &= BSTP_PDU_RSTPMASK;
    break;
  }
  return (flags);
}

void bstp_transmit_tcn(struct bstp_state *bs, struct bstp_port *bp) {
  struct bstp_tbpdu bpdu;
  struct portnet *ifp = bp->bp_ifp;
  struct ether_header *eh;
  uint8_t *m;
  int s, len, error;

  if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0)
    return;
  DEBUG_MSG(STP_DEBUG,"bstp_transmit_tcn\r\n");
  m = send_buf;
  len = sizeof(*eh) + sizeof(bpdu);

  eh = (struct ether_header *)m;
  get_haddr(eh->ether_shost);
  memcpy(eh->ether_dhost, bstp_etheraddr, ETHER_ADDR_LEN);
  eh->ether_type = htons(sizeof(bpdu));

  bpdu.tbu_ssap = bpdu.tbu_dsap = LLC_8021D_LSAP;
  bpdu.tbu_ctl = LLC_UI;
  bpdu.tbu_protoid = 0;
  bpdu.tbu_protover = 0;
  bpdu.tbu_bpdutype = BSTP_MSGTYPE_TCN;
  memcpy(m + sizeof(*eh), &bpdu, sizeof(bpdu));

  s = splnet();
  bp->bp_txcount++;
  bp->bp_txcounter++;

  error = bstp_send(bp->id, m, len);
  /*if (error == 0) {
    ifp->if_obytes += len;
    ifp->if_omcasts++;
    if_start(ifp);
  }*/

  splx(s);
}

void bstp_transmit_bpdu(struct bstp_state *bs, struct bstp_port *bp) {
  struct bstp_cbpdu bpdu;
  struct portnet *ifp = bp->bp_ifp;

  if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0)
    return;
#if STP_U64
  bpdu.cbu_rootpri = htons(bp->bp_desg_pv.pv_root_id >> 48);
#else
  bpdu.cbu_rootpri = htons(bp->bp_desg_pv.pv_root_id[1] >> 16);
#endif



  PV2ADDR(bp->bp_desg_pv.pv_root_id, bpdu.cbu_rootaddr);

  bpdu.cbu_rootpathcost = htonl(bp->bp_desg_pv.pv_cost);

#if STP_U64
  bpdu.cbu_bridgepri = htons(bp->bp_desg_pv.pv_dbridge_id >> 48);
#else
  bpdu.cbu_bridgepri = htons(bp->bp_desg_pv.pv_dbridge_id[1] >> 16);
#endif

  PV2ADDR(bp->bp_desg_pv.pv_dbridge_id, bpdu.cbu_bridgeaddr);

  bpdu.cbu_portid = htons(bp->bp_port_id);
  bpdu.cbu_messageage = htons(bp->bp_desg_msg_age * (256/BSTP_TICK_VAL));
  bpdu.cbu_maxage = htons(bp->bp_desg_max_age * (256/BSTP_TICK_VAL));
  bpdu.cbu_hellotime = htons(bp->bp_desg_htime * (256/BSTP_TICK_VAL));
  bpdu.cbu_forwarddelay = htons(bp->bp_desg_fdelay * (256/BSTP_TICK_VAL));
  bpdu.cbu_flags = bstp_pdu_flags(bp);

  switch (bp->bp_protover) {
  case BSTP_PROTO_STP:
    bpdu.cbu_bpdutype = BSTP_MSGTYPE_CFG;
    break;
  case BSTP_PROTO_RSTP:
    bpdu.cbu_bpdutype = BSTP_MSGTYPE_RSTP;
    break;
  }

#if BRIDGESTP_DEBUG
  if (deb_tick<4||deb_trans){
    DEBUG_MSG(STP_DEBUG,"transmit_bpdu %s(%d): flags: %02x!!!\r\n", bp->bp_ifp->name, bp->id, bpdu.cbu_flags);
  }
#endif
  
  bstp_send_bpdu(bs, bp, &bpdu);
}

void bstp_transmit(struct bstp_state *bs, struct bstp_port *bp){
  if (/*(bs->bs_ifflags & IFF_RUNNING) == 0*/(bs->active==0)||bp==NULL)
    return;

  /*
   * a PDU can only be sent if we have tx quota left and the
   * hello timer is running.
   */
  if (bp->bp_hello_timer.active == 0) {
    /* Test if it needs to be reset */
    bstp_hello_timer_expiry(bs, bp);
    return;
  }
  if (bp->bp_txcount > bs->bs_txholdcount){
    /* Ran out of karma */
    DEBUG_MSG(STP_DEBUG,"%s(%d) Ran out of karma\r\n", bp->bp_ifp->name, bp->id);
    return;
  }

  if (bp->bp_protover == BSTP_PROTO_RSTP) {
    bstp_transmit_bpdu(bs, bp);
    bp->bp_tc_ack = 0;
  } else { /* STP */
    switch (bp->bp_role) {
    case BSTP_ROLE_DESIGNATED:
      bstp_transmit_bpdu(bs, bp);
      bp->bp_tc_ack = 0;
      break;

    case BSTP_ROLE_ROOT:
      bstp_transmit_tcn(bs, bp);
      break;
    }
  }
  bstp_timer_start(&bp->bp_hello_timer, bp->bp_desg_htime);
  bp->bp_flags &= ~BSTP_PORT_NEWINFO;
}

/* set tcprop on every port other than the caller */
void bstp_set_other_tcprop(struct bstp_port *bp) {
  struct bstp_port *bp2;

  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    bp2 = &bstp_ports[i];
    if (bp2 == bp) continue;
    bp2->bp_tc_prop = 1;
  }
}
#if 0
/* set tcprop on every port other than the caller */
void bstp_set_other_tcprop(struct bstp_port *bp){
  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    if (i==bp->id) continue;
    bstp_ports[i].bp_tc_prop = 1;
  }
}
#endif

void bstp_set_timer_tc(struct bstp_port *bp) {
  struct bstp_state *bs = bp->bp_bs;
  if (bs==NULL)
	  return;

  if (bp->bp_tc_timer.active)
    return;

  switch (bp->bp_protover) {
  case BSTP_PROTO_RSTP:
    bstp_timer_start(&bp->bp_tc_timer,
        bp->bp_desg_htime + BSTP_TICK_VAL);
    bp->bp_flags |= BSTP_PORT_NEWINFO;
    break;
  case BSTP_PROTO_STP:
    bstp_timer_start(&bp->bp_tc_timer,
        bs->bs_root_max_age + bs->bs_root_fdelay);
    break;
  }
}

void bstp_notify_rtage(void *arg, int pending){     //TAG: TODO
  struct bstp_port *bp = (struct bstp_port *)arg;
#if 0
  int age = 0;

//  splassert(IPL_NET);

  switch (bp->bp_protover) {
  case BSTP_PROTO_STP:
    /* convert to seconds */
    age = bp->bp_desg_fdelay / BSTP_TICK_VAL;
    break;
  case BSTP_PROTO_RSTP:
    age = 0;
    break;
  }

/*  if (bp->bp_active == 1)
    bridge_rtagenode(bp->bp_ifp, age);*/ //TAG: TODO
#endif
  /* flush is complete */
  DEBUG_MSG(STP_DEBUG,"%s(%d) -> FLUSHING\r\n", bp->bp_ifp->name, bp->id);
  flush_db(bp->id);
  bp->bp_fdbflush = 0;
}

void bstp_set_port_tc(struct bstp_port *bp, int state){
  struct bstp_state *bs = bp->bp_bs;
  if (bs==NULL)
	  return;

  bp->bp_tcstate = state;

  /* initialise the new state */
  switch (bp->bp_tcstate) {
	  case BSTP_TCSTATE_ACTIVE:
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_ACTIVE\r\n", bp->bp_ifp->name, bp->id);
		/* nothing to do */
		break;

	  case BSTP_TCSTATE_INACTIVE:
		bstp_timer_stop(&bp->bp_tc_timer);
		/* flush routes on the parent bridge */
		bp->bp_fdbflush = 1;
		bstp_notify_rtage(bp, 0);
		bp->bp_tc_ack = 0;
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_INACTIVE\r\n", bp->bp_ifp->name, bp->id);
	#if 1
		if (bp->bp_operedge){
			bstp_set_timer_tc(bp);
			bstp_set_other_tcprop(bp);
		}
	#endif
		break;

	  case BSTP_TCSTATE_LEARNING:
		bp->bp_rcvdtc = 0;
		bp->bp_rcvdtcn = 0;
		bp->bp_rcvdtca = 0;
		bp->bp_tc_prop = 0;
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_LEARNING\r\n", bp->bp_ifp->name, bp->id);
		break;

	  case BSTP_TCSTATE_DETECTED:
		bstp_set_timer_tc(bp);
		bstp_set_other_tcprop(bp);
		/* send out notification */
		bp->bp_flags |= BSTP_PORT_NEWINFO;
		bstp_transmit(bs, bp);
		//getmicrotime(&bs->bs_last_tc_time);
		bs->bs_last_tc_time = 0;
		bs->bs_tc_count++;
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_DETECTED\r\n", bp->bp_ifp->name, bp->id);
		bp->bp_tcstate = BSTP_TCSTATE_ACTIVE; /* UCT */

		send_events_u32(EVENT_STP_REINIT,0);

		break;

	  case BSTP_TCSTATE_TCN:
		bstp_set_timer_tc(bp);
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_TCN\r\n", bp->bp_ifp->name, bp->id);

		/* FALLTHROUGH */
	  case BSTP_TCSTATE_TC:
		bp->bp_rcvdtc = 0;
		bp->bp_rcvdtcn = 0;
		if (bp->bp_role == BSTP_ROLE_DESIGNATED)
		  bp->bp_tc_ack = 1;
		bstp_set_other_tcprop(bp);
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_TC\r\n", bp->bp_ifp->name, bp->id);
		bp->bp_tcstate = BSTP_TCSTATE_ACTIVE; /* UCT */
		bp->bp_fdbflush = 1;
		bstp_notify_rtage(bp, 0);
		break;

	  case BSTP_TCSTATE_PROPAG:
		/* flush routes on the parent bridge */
		bp->bp_fdbflush = 1;
		bstp_notify_rtage(bp, 0);
		bp->bp_tc_prop = 0;
		bstp_set_timer_tc(bp);
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_PROPAG\r\n", bp->bp_ifp->name, bp->id);
		bp->bp_tcstate = BSTP_TCSTATE_ACTIVE; /* UCT */
		break;

	  case BSTP_TCSTATE_ACK:
		bstp_timer_stop(&bp->bp_tc_timer);
		bp->bp_rcvdtca = 0;
		DEBUG_MSG(STP_DEBUG,"%s(%d) -> TC_ACK\r\n", bp->bp_ifp->name, bp->id);
		bp->bp_tcstate = BSTP_TCSTATE_ACTIVE; /* UCT */
		break;
	  }
}

void bstp_update_tc(struct bstp_port *bp){
    switch (bp->bp_tcstate) {
    case BSTP_TCSTATE_ACTIVE:
      if ((bp->bp_role != BSTP_ROLE_DESIGNATED &&
          bp->bp_role != BSTP_ROLE_ROOT) || bp->bp_operedge)
        bstp_set_port_tc(bp, BSTP_TCSTATE_LEARNING);

      if (bp->bp_rcvdtcn)
        bstp_set_port_tc(bp, BSTP_TCSTATE_TCN);
      if (bp->bp_rcvdtc)
        bstp_set_port_tc(bp, BSTP_TCSTATE_TC);

      if (bp->bp_tc_prop && !bp->bp_operedge)
        bstp_set_port_tc(bp, BSTP_TCSTATE_PROPAG);

      if (bp->bp_rcvdtca)
        bstp_set_port_tc(bp, BSTP_TCSTATE_ACK);
      break;

    case BSTP_TCSTATE_INACTIVE:
      if ((bp->bp_state == BSTP_IFSTATE_LEARNING ||
          bp->bp_state == BSTP_IFSTATE_FORWARDING) &&
          bp->bp_fdbflush == 0)
        bstp_set_port_tc(bp, BSTP_TCSTATE_LEARNING);
      break;

    case BSTP_TCSTATE_LEARNING:
      if (bp->bp_rcvdtc || bp->bp_rcvdtcn || bp->bp_rcvdtca ||
          bp->bp_tc_prop)
        bstp_set_port_tc(bp, BSTP_TCSTATE_LEARNING);
      else if (bp->bp_role != BSTP_ROLE_DESIGNATED &&
         bp->bp_role != BSTP_ROLE_ROOT &&
         bp->bp_state == BSTP_IFSTATE_DISCARDING)
        bstp_set_port_tc(bp, BSTP_TCSTATE_INACTIVE);

      if ((bp->bp_role == BSTP_ROLE_DESIGNATED ||
          bp->bp_role == BSTP_ROLE_ROOT) &&
          bp->bp_state == BSTP_IFSTATE_FORWARDING &&
          !bp->bp_operedge)
        bstp_set_port_tc(bp, BSTP_TCSTATE_DETECTED);
      break;

    /* these are transient states and go straight back to ACTIVE */
    case BSTP_TCSTATE_DETECTED:
    case BSTP_TCSTATE_TCN:
    case BSTP_TCSTATE_TC:
    case BSTP_TCSTATE_PROPAG:
    case BSTP_TCSTATE_ACK:
      DEBUG_MSG(STP_DEBUG,"Invalid TC state for %s(%d)\r\n",
    		  bp->bp_ifp->name, bp->id);
      break;
    }
}

#define bstp_set_port_state(a, b) {DEBUG_MSG(STP_DEBUG,"bstp_set_port_state %s:%d\r\n", __FUNCTION__, __LINE__); bstp_set_port_state_(a,b);}

void bstp_set_port_state_(struct bstp_port *bp, int state){

  if (bp->bp_state == state)
    return;


  //для активных портов, состояние порта в лог
  if((bp->bp_ifp->if_link_state & LINK_STATE_DOWN) == 0)
  	  send_events_u32(EVENT_STP_PORT_1 + F2L_port_conv(bp->id), (u32)state);


  bp->bp_state = state;

  switch (bp->bp_state) {
  case BSTP_IFSTATE_DISCARDING:
    DEBUG_MSG(STP_DEBUG,"state changed to DISCARDING on %s(%d)\r\n",
    		bp->bp_ifp->name, bp->id);
    break;

  case BSTP_IFSTATE_LEARNING:
    DEBUG_MSG(STP_DEBUG,"state changed to LEARNING on %s(%d)\r\n",
    		bp->bp_ifp->name, bp->id);

    bstp_timer_start(&bp->bp_forward_delay_timer,
        bp->bp_protover == BSTP_PROTO_RSTP ?
        bp->bp_desg_htime : bp->bp_desg_fdelay);
    break;

  case BSTP_IFSTATE_FORWARDING:
    DEBUG_MSG(STP_DEBUG,"state changed to FORWARDING on %s(%d)\r\n",
    		bp->bp_ifp->name, bp->id);

    bstp_timer_stop(&bp->bp_forward_delay_timer);
    /* Record that we enabled forwarding */
    bp->bp_forward_transitions++;
    break;
  }

  stp_set_port_state(bp->id, state);

  bp->bp_bs->bs_changed = 1;
}


void bstp_set_port_role(struct bstp_port *bp, int role) {
  struct bstp_state *bs = bp->bp_bs;
  if (bs==NULL) return;

  if (bp->bp_role == role)
    return;

  /* perform pre-change tasks */
  switch (bp->bp_role) {
	  case BSTP_ROLE_DISABLED:
		bstp_timer_start(&bp->bp_forward_delay_timer,
			bp->bp_desg_max_age);
		break;

	  case BSTP_ROLE_BACKUP:
		bstp_timer_start(&bp->bp_recent_backup_timer,
			bp->bp_desg_htime * 2);
		/* FALLTHROUGH */
	  case BSTP_ROLE_ALTERNATE:
		bstp_timer_start(&bp->bp_forward_delay_timer,
			bp->bp_desg_fdelay);
		bp->bp_sync = 0;
		bp->bp_synced = 1;
		bp->bp_reroot = 0;
		break;

	  case BSTP_ROLE_ROOT:
		bstp_timer_start(&bp->bp_recent_root_timer,
			BSTP_DEFAULT_FORWARD_DELAY);
		break;
  }

  bp->bp_role = role;
  /* clear values not carried between roles */
  bp->bp_proposing = 0;
  bs->bs_allsynced = 0;

  /* initialise the new role */
  switch (bp->bp_role) {
	  case BSTP_ROLE_DISABLED:
	  case BSTP_ROLE_ALTERNATE:
	  case BSTP_ROLE_BACKUP:
		DEBUG_MSG(STP_DEBUG,"%s(%d) role -> ALT/BACK/DISABLED\r\n",
				bp->bp_ifp->name, bp->id);
		bstp_set_port_state(bp, BSTP_IFSTATE_DISCARDING);
		bstp_timer_stop(&bp->bp_recent_root_timer);
		bstp_timer_latch(&bp->bp_forward_delay_timer);
		bp->bp_sync = 0;
		bp->bp_synced = 1;
		bp->bp_reroot = 0;
		break;

	  case BSTP_ROLE_ROOT:
		DEBUG_MSG(STP_DEBUG,"%s(%d) role -> ROOT\r\n",
				bp->bp_ifp->name, bp->id);
		bstp_set_port_state(bp, BSTP_IFSTATE_DISCARDING);
		bstp_timer_latch(&bp->bp_recent_root_timer);
		bp->bp_proposing = 0;
		break;

	  case BSTP_ROLE_DESIGNATED:
		DEBUG_MSG(STP_DEBUG,"%s(%d) role -> DESIGNATED\r\n",
				bp->bp_ifp->name, bp->id);
		bstp_timer_start(&bp->bp_hello_timer,
			bp->bp_desg_htime);
		bp->bp_agree = 0;
		break;
  }

  /* let the TC state know that the role changed */
  bstp_update_tc(bp);

  bs->bs_changed = 1;
}

void bstp_set_port_proto(struct bstp_port *bp, int proto) {
  struct bstp_state *bs = bp->bp_bs;
  if (bs==NULL) return;

    DEBUG_MSG(STP_DEBUG,"%s(%d) set_port_proto %d\r\n", bp->bp_ifp->name, bp->id, proto);
    /* supported protocol versions */
    switch (proto) {
    case BSTP_PROTO_STP:
#if 0
      /* we can downgrade protocols only */
      bstp_timer_stop(&bp->bp_migrate_delay_timer);
#else
      bstp_timer_start(&bp->bp_migrate_delay_timer,
                bs->bs_migration_delay);
#endif
      /* clear unsupported features */
      bp->bp_operedge = 0;
      break;

    case BSTP_PROTO_RSTP:
      bstp_timer_start(&bp->bp_migrate_delay_timer,
          bs->bs_migration_delay);
      break;

    default:
      DEBUG_MSG(STP_DEBUG,"Unsupported STP version %d\r\n", proto);
      return;
    }

    bp->bp_protover = proto;
    bp->bp_flags &= ~BSTP_PORT_CANMIGRATE;
}

void bstp_set_timer_msgage(struct bstp_port *bp) {
  if (bp->bp_port_msg_age + BSTP_MESSAGE_AGE_INCR <=
      bp->bp_port_max_age) {
    bstp_timer_start(&bp->bp_message_age_timer,
        bp->bp_port_htime * 3);
  } else {
    /* expires immediately */
    bstp_timer_start(&bp->bp_message_age_timer, 0);
  }
}

int bstp_pdu_rcvtype(struct bstp_port *bp, struct bstp_config_unit *cu) {
  int type;

  /* default return type */
  type = BSTP_PDU_OTHER;

  switch (cu->cu_role) {
  case BSTP_ROLE_DESIGNATED:
    if (bstp_info_superior(&bp->bp_port_pv, &cu->cu_pv))
      /* bpdu priority is superior */
      type = BSTP_PDU_SUPERIOR;
    else if (bstp_info_cmp(&bp->bp_port_pv, &cu->cu_pv) ==
        INFO_SAME) {
      if (bp->bp_port_msg_age != cu->cu_message_age ||
          bp->bp_port_max_age != cu->cu_max_age ||
          bp->bp_port_fdelay != cu->cu_forward_delay ||
          bp->bp_port_htime != cu->cu_hello_time)
        /* bpdu priority is equal and timers differ */
        type = BSTP_PDU_SUPERIOR;
      else
        /* bpdu is equal */
        type = BSTP_PDU_REPEATED;
    } else
      /* bpdu priority is worse */
      type = BSTP_PDU_INFERIOR;

    break;

  case BSTP_ROLE_ROOT:
  case BSTP_ROLE_ALTERNATE:
  case BSTP_ROLE_BACKUP:
    if (bstp_info_cmp(&bp->bp_port_pv, &cu->cu_pv) <= INFO_SAME)
      /*
       * not a designated port and priority is the same or
       * worse
       */
      type = BSTP_PDU_INFERIORALT;
    break;
  }

  return (type);
}

void bstp_received_tcn(struct bstp_state *bs, struct bstp_port *bp,
    struct bstp_tcn_unit *tcn) {
  bp->bp_rcvdtcn = 1;
  bstp_update_tc(bp);
}

void bstp_update_state(struct bstp_state *bs, struct bstp_port *bp) {
	struct bstp_port *bp2;
	int synced;

	/* check if all the ports have synchronized again */
	if (!bs->bs_allsynced) {
		synced = 1;
		for (int i=0;i<NUM_BSTP_PORT;i++){
			if (bstp_ports[i].bp_active==0) continue;
            bp2 = &bstp_ports[i];
			if (!(bp2->bp_synced ||
			     bp2->bp_role == BSTP_ROLE_ROOT)) {
				synced = 0;
				break;
			}
		}
		bs->bs_allsynced = synced;
	}

	bstp_update_roles(bs, bp);
	bstp_update_tc(bp);
}

void bstp_received_bpdu(struct bstp_state *bs, struct bstp_port *bp,
    struct bstp_config_unit *cu) {
  int type;

  /* We need to have transitioned to INFO_MINE before proceeding */
  switch (bp->bp_infois) {
	  case BSTP_INFO_DISABLED:
	  case BSTP_INFO_AGED:
		return;
  }

  type = bstp_pdu_rcvtype(bp, cu);

  switch (type) {
  case BSTP_PDU_SUPERIOR:
    bs->bs_allsynced = 0;
    bp->bp_agreed = 0;
    bp->bp_proposing = 0;

    if (cu->cu_proposal && cu->cu_forwarding == 0)
      bp->bp_proposed = 1;
    if (cu->cu_topology_change)
      bp->bp_rcvdtc = 1;
    if (cu->cu_topology_change_ack)
      bp->bp_rcvdtca = 1;

    if (bp->bp_agree &&
        !bstp_pdu_bettersame(bp, BSTP_INFO_RECEIVED))
      bp->bp_agree = 0;

    /* copy the received priority and timers to the port */
    bp->bp_port_pv = cu->cu_pv;
    bp->bp_port_msg_age = cu->cu_message_age;
    bp->bp_port_max_age = cu->cu_max_age;
    bp->bp_port_fdelay = cu->cu_forward_delay;
    bp->bp_port_htime =
        (cu->cu_hello_time > BSTP_MIN_HELLO_TIME ?
         cu->cu_hello_time : BSTP_MIN_HELLO_TIME);

    /* set expiry for the new info */
    bstp_set_timer_msgage(bp);

    bp->bp_infois = BSTP_INFO_RECEIVED;
    bstp_assign_roles(bs);
    break;

  case BSTP_PDU_REPEATED:
    if (cu->cu_proposal && cu->cu_forwarding == 0)
      bp->bp_proposed = 1;
    if (cu->cu_topology_change)
      bp->bp_rcvdtc = 1;
    if (cu->cu_topology_change_ack)
      bp->bp_rcvdtca = 1;

    /* rearm the age timer */
    bstp_set_timer_msgage(bp);
    break;

  case BSTP_PDU_INFERIOR:
	DEBUG_MSG(STP_DEBUG,"BSTP_PDU_INFERIOR %d\r\n", bp->id);
    if (cu->cu_learning) {
      DEBUG_MSG(STP_DEBUG,"bp_agreed = 1 (1) %d\r\n", bp->id);
      bp->bp_agreed = 1;
      bp->bp_proposing = 0;
    }
    break;

  case BSTP_PDU_INFERIORALT:
	DEBUG_MSG(STP_DEBUG,"BSTP_PDU_INFERIORALT %d\r\n", bp->id);
    /*
     * only point to point links are allowed fast
     * transitions to forwarding.
     */
    if (cu->cu_agree && bp->bp_ptp_link) {
      DEBUG_MSG(STP_DEBUG,"bp_agreed = 1 (2)\r\n");
      bp->bp_agreed = 1;
      bp->bp_proposing = 0;
    } else
      bp->bp_agreed = 0;

    if (cu->cu_topology_change)
      bp->bp_rcvdtc = 1;
    if (cu->cu_topology_change_ack)
      bp->bp_rcvdtca = 1;
    break;

  case BSTP_PDU_OTHER:
    return;	/* do nothing */
  }

  /* update the state machines with the new data */
  bstp_update_state(bs, bp);
  if (bs->bs_changed){
    bstp_event_t event;
	  bs->bs_changed = 0;
	  event.id = BSTP_EVENT_CHANGED;
	  bstp_send_event(&event);
  }
}

void bstp_decode_bpdu(struct bstp_port *bp, struct bstp_cbpdu *cpdu,
    struct bstp_config_unit *cu) {
  int flags;

#if STP_U64
  cu->cu_pv.pv_root_id =
      (((u_int64_t)ntohs(cpdu->cbu_rootpri)) << 48) |
      (((u_int64_t)cpdu->cbu_rootaddr[0]) << 40) |
      (((u_int64_t)cpdu->cbu_rootaddr[1]) << 32) |
      (((u_int64_t)cpdu->cbu_rootaddr[2]) << 24) |
      (((u_int64_t)cpdu->cbu_rootaddr[3]) << 16) |
      (((u_int64_t)cpdu->cbu_rootaddr[4]) << 8) |
      (((u_int64_t)cpdu->cbu_rootaddr[5]) << 0);
  DEBUG_MSG(STP_DEBUG,"bstp_decode_bpdu pv_root_id 0x%llX \r\n", cu->cu_pv.pv_root_id);
#else
  cu->cu_pv.pv_root_id[1] = ((((u_int32_t)ntohs(cpdu->cbu_rootpri)) << 16) |
	      (((u_int32_t)cpdu->cbu_rootaddr[0]) << 8) |
	      ((u_int32_t)cpdu->cbu_rootaddr[1]));
  cu->cu_pv.pv_root_id[0] =
		  ((((u_int32_t)cpdu->cbu_rootaddr[2]) << 24) |
	      (((u_int32_t)cpdu->cbu_rootaddr[3]) << 16) |
	      (((u_int32_t)cpdu->cbu_rootaddr[4]) << 8) |
	      (((u_int32_t)cpdu->cbu_rootaddr[5]) << 0));
  DEBUG_MSG(STP_DEBUG,"bstp_decode_bpdu pv_root_id 0x%lX 0x%lX \r\n", cu->cu_pv.pv_root_id[1], cu->cu_pv.pv_root_id[0]);
#endif



#if STP_U64
  cu->cu_pv.pv_dbridge_id =
      (((u_int64_t)ntohs(cpdu->cbu_bridgepri)) << 48) |
      (((u_int64_t)cpdu->cbu_bridgeaddr[0]) << 40) |
      (((u_int64_t)cpdu->cbu_bridgeaddr[1]) << 32) |
      (((u_int64_t)cpdu->cbu_bridgeaddr[2]) << 24) |
      (((u_int64_t)cpdu->cbu_bridgeaddr[3]) << 16) |
      (((u_int64_t)cpdu->cbu_bridgeaddr[4]) << 8) |
      (((u_int64_t)cpdu->cbu_bridgeaddr[5]) << 0);
  DEBUG_MSG(STP_DEBUG,"bstp_decode_bpdu pv_root_id 0x%llX \r\n", cu->cu_pv.pv_dbridge_id);
#else
  cu->cu_pv.pv_dbridge_id[1] = ((((u_int32_t)ntohs(cpdu->cbu_bridgepri)) << 16) |
	      (((u_int32_t)cpdu->cbu_bridgeaddr[0]) << 8) |
	      ((u_int32_t)cpdu->cbu_bridgeaddr[1]));
  cu->cu_pv.pv_dbridge_id[0] =
		  ((((u_int32_t)cpdu->cbu_bridgeaddr[2]) << 24) |
	      (((u_int32_t)cpdu->cbu_bridgeaddr[3]) << 16) |
	      (((u_int32_t)cpdu->cbu_bridgeaddr[4]) << 8) |
	      (((u_int32_t)cpdu->cbu_bridgeaddr[5]) << 0));
  DEBUG_MSG(STP_DEBUG,"bstp_decode_bpdu pv_root_id 0x%lX 0x%lX\r\n", cu->cu_pv.pv_dbridge_id[1], cu->cu_pv.pv_dbridge_id[0]);
#endif


  cu->cu_pv.pv_cost = ntohl(cpdu->cbu_rootpathcost);
  cu->cu_message_age = ntohs(cpdu->cbu_messageage) / (256/BSTP_TICK_VAL);
  cu->cu_max_age = ntohs(cpdu->cbu_maxage) / (256/BSTP_TICK_VAL);
  cu->cu_hello_time = ntohs(cpdu->cbu_hellotime) / (256/BSTP_TICK_VAL);
  cu->cu_forward_delay = ntohs(cpdu->cbu_forwarddelay) / (256/BSTP_TICK_VAL);
  cu->cu_pv.pv_dport_id = ntohs(cpdu->cbu_portid);
  cu->cu_pv.pv_port_id = bp->bp_port_id;
  cu->cu_message_type = cpdu->cbu_bpdutype;

  /* Strip off unused flags in STP mode */
  flags = cpdu->cbu_flags;
  switch (cpdu->cbu_protover) {
  case BSTP_PROTO_STP:
    flags &= BSTP_PDU_STPMASK;
    /* A STP BPDU explicitly conveys a Designated Port */
    cu->cu_role = BSTP_ROLE_DESIGNATED;
    break;
  case BSTP_PROTO_RSTP:
    flags &= BSTP_PDU_RSTPMASK;
    break;
  }

  cu->cu_topology_change_ack =
    (flags & BSTP_PDU_F_TCA) ? 1 : 0;
  cu->cu_proposal =
    (flags & BSTP_PDU_F_P) ? 1 : 0;
  cu->cu_agree =
    (flags & BSTP_PDU_F_A) ? 1 : 0;
  cu->cu_learning =
    (flags & BSTP_PDU_F_L) ? 1 : 0;
  cu->cu_forwarding =
    (flags & BSTP_PDU_F_F) ? 1 : 0;
  cu->cu_topology_change =
    (flags & BSTP_PDU_F_TC) ? 1 : 0;

  switch ((flags & BSTP_PDU_PRMASK) >> BSTP_PDU_PRSHIFT) {
	  case BSTP_PDU_F_ROOT:
		cu->cu_role = BSTP_ROLE_ROOT;
		break;
	  case BSTP_PDU_F_ALT:
		cu->cu_role = BSTP_ROLE_ALTERNATE;
		break;
	  case BSTP_PDU_F_DESG:
		cu->cu_role = BSTP_ROLE_DESIGNATED;
		break;
  }
}

void bstp_received_stp(struct bstp_state *bs, struct bstp_port *bp,
    uint8_t *mp, struct bstp_tbpdu *tpdu) {
  struct bstp_cbpdu cpdu;
  struct bstp_config_unit *cu = &bp->bp_msg_cu;
  struct bstp_tcn_unit tu;

  switch (tpdu->tbu_bpdutype) {
  case BSTP_MSGTYPE_TCN:
    tu.tu_message_type = tpdu->tbu_bpdutype;
    bstp_received_tcn(bs, bp, &tu);
    break;
  case BSTP_MSGTYPE_CFG:
    memcpy(&cpdu, mp, BSTP_BPDU_STP_LEN);

    bstp_decode_bpdu(bp, &cpdu, cu);
    bstp_received_bpdu(bs, bp, cu);
    break;
  }
}

void bstp_received_rstp(struct bstp_state *bs, struct bstp_port *bp,
    uint8_t *mp, struct bstp_tbpdu *tpdu) {
  struct bstp_cbpdu cpdu;
  struct bstp_config_unit *cu = &bp->bp_msg_cu;

  if (tpdu->tbu_bpdutype != BSTP_MSGTYPE_RSTP)
    return;

  memcpy(&cpdu, mp, BSTP_BPDU_RSTP_LEN);

  bstp_decode_bpdu(bp, &cpdu, cu);
  bstp_received_bpdu(bs, bp, cu);
}

void bstp_input(struct bstp_state *bs, struct bstp_port *bp, struct ether_header *eh, uint8_t *m) {
  struct bstp_tbpdu tpdu;
  u_int16_t len;

  if (bs == NULL || bp == NULL || bp->bp_active == 0)
    goto out;

  len = ntohs(eh->ether_type);
  if (len < sizeof(tpdu))
    goto out;
  memcpy(&tpdu, m, sizeof(tpdu));

  if (tpdu.tbu_dsap != LLC_8021D_LSAP ||
      tpdu.tbu_ssap != LLC_8021D_LSAP ||
      tpdu.tbu_ctl != LLC_UI)
    goto out;
  if (tpdu.tbu_protoid != BSTP_PROTO_ID)
    goto out;

  /*
   * We can treat later versions of the PDU as the same as the maximum
   * version we implement. All additional parameters/flags are ignored.
   */
  if (tpdu.tbu_protover > BSTP_PROTO_MAX)
    tpdu.tbu_protover = BSTP_PROTO_MAX;

  if (tpdu.tbu_protover != bp->bp_protover) {
    /*
     * Wait for the migration delay timer to expire before changing
     * protocol version to avoid flip-flops.
     */
	  DEBUG_MSG(STP_DEBUG,"%s(%d) protover\r\n", bp->bp_ifp->name, bp->id);
    if (bp->bp_flags & BSTP_PORT_CANMIGRATE)
      bstp_set_port_proto(bp, tpdu.tbu_protover);
    else
      goto out;
  }

  /* Clear operedge upon receiving a PDU on the port */
  bp->bp_operedge = 0;
  bstp_timer_start(&bp->bp_edge_delay_timer,
      BSTP_DEFAULT_MIGRATE_DELAY);

  switch (tpdu.tbu_protover) {
	  case BSTP_PROTO_STP:
		bstp_received_stp(bs, bp, m, &tpdu);
		break;
	  case BSTP_PROTO_RSTP:
		bstp_received_rstp(bs, bp, m, &tpdu);
		break;
  }
  out:
    return;
}

void bstp_stop(struct bstp_state *bs) {
  struct bstp_port *bp;

  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    bp = &bstp_ports[i];
    bstp_set_port_state(bp, BSTP_IFSTATE_DISCARDING);
  }

  if (bs->bs_bstptimeout) bs->bs_bstptimeout = 0;
    
}

void bstp_set_all_reroot(struct bstp_state *bs) {
  struct bstp_port *bp;



  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    bp = &bstp_ports[i];
    bp->bp_reroot = 1;
  }
}

void bstp_set_all_sync(struct bstp_state *bs) {
  struct bstp_port *bp;

  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    bp = &bstp_ports[i];
    bp->bp_sync = 1;
    bp->bp_synced = 0;	/* Not explicit in spec */
  }

  bs->bs_allsynced = 0;
}

int bstp_rerooted(struct bstp_state *bs, struct bstp_port *bp) {
  struct bstp_port *bp2;
  int rr_set = 0;

  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    bp2 = &bstp_ports[i];
    if (bp2 == bp)
      continue;
    if (bp2->bp_recent_root_timer.active) {
      rr_set = 1;
      break;
    }
  }
  return (!rr_set);
}

void bstp_update_roles(struct bstp_state *bs, struct bstp_port *bp) {
//  DEBUG_MSG(STP_DEBUG,"bstp_update_roles\r\n");

  switch (bp->bp_role) {
  case BSTP_ROLE_DISABLED:
    /* Clear any flags if set */
    if (bp->bp_sync || !bp->bp_synced || bp->bp_reroot) {
      bp->bp_sync = 0;
      bp->bp_synced = 1;
      bp->bp_reroot = 0;
    }
    break;

  case BSTP_ROLE_ALTERNATE:
  case BSTP_ROLE_BACKUP:
    if ((bs->bs_allsynced && !bp->bp_agree) ||
        (bp->bp_proposed && bp->bp_agree)) {
      bp->bp_proposed = 0;
      bp->bp_agree = 1;
      bp->bp_flags |= BSTP_PORT_NEWINFO;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ALTERNATE_AGREED\r\n",
    		  bp->bp_ifp->name, bp->id);
    }

    if (bp->bp_proposed && !bp->bp_agree) {
      DEBUG_MSG(STP_DEBUG,"set_all_sync (1)\r\n");
      bstp_set_all_sync(bs);
      bp->bp_proposed = 0;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ALTERNATE_PROPOSED\r\n",
    		  bp->bp_ifp->name, bp->id);
    }

    /* Clear any flags if set */
    if (bp->bp_sync || !bp->bp_synced || bp->bp_reroot) {
      bp->bp_sync = 0;
      bp->bp_synced = 1;
      bp->bp_reroot = 0;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ALTERNATE_PORT\r\n", bp->bp_ifp->name, bp->id);
    }
    break;

  case BSTP_ROLE_ROOT:
    if (bp->bp_state != BSTP_IFSTATE_FORWARDING && !bp->bp_reroot) {
      bstp_set_all_reroot(bs);
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ROOT_REROOT\r\n", bp->bp_ifp->name, bp->id);
    }

    if ((bs->bs_allsynced && !bp->bp_agree) ||
        (bp->bp_proposed && bp->bp_agree)) {
      bp->bp_proposed = 0;
      bp->bp_sync = 0;
      bp->bp_agree = 1;
      bp->bp_flags |= BSTP_PORT_NEWINFO;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ROOT_AGREED\r\n", bp->bp_ifp->name, bp->id);
    }

    if (bp->bp_proposed && !bp->bp_agree) {
      DEBUG_MSG(STP_DEBUG,"set_all_sync (2)\r\n");
      bstp_set_all_sync(bs);
      bp->bp_proposed = 0;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ROOT_PROPOSED\r\n", bp->bp_ifp->name, bp->id);
    }

    if (bp->bp_state != BSTP_IFSTATE_FORWARDING &&
        (bp->bp_forward_delay_timer.active == 0 ||
        (bstp_rerooted(bs, bp) &&
        bp->bp_recent_backup_timer.active == 0 &&
        bp->bp_protover == BSTP_PROTO_RSTP))) {
      switch (bp->bp_state) {
      case BSTP_IFSTATE_DISCARDING:
        bstp_set_port_state(bp, BSTP_IFSTATE_LEARNING);
        break;
      case BSTP_IFSTATE_LEARNING:
        bstp_set_port_state(bp, BSTP_IFSTATE_FORWARDING);
        break;
      }
    }

    if (bp->bp_state == BSTP_IFSTATE_FORWARDING && bp->bp_reroot) {
      bp->bp_reroot = 0;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> ROOT_REROOTED\r\n", bp->bp_ifp->name, bp->id);
    }
    break;

  case BSTP_ROLE_DESIGNATED:
    if (bp->bp_recent_root_timer.active == 0 && bp->bp_reroot) {
      bp->bp_reroot = 0;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> DESIGNATED_RETIRED\r\n",
    		  bp->bp_ifp->name, bp->id);
    }

    if ((bp->bp_state == BSTP_IFSTATE_DISCARDING &&
        !bp->bp_synced) || (bp->bp_agreed && !bp->bp_synced) ||
        (bp->bp_operedge && !bp->bp_synced) ||
        (bp->bp_sync && bp->bp_synced)) {
/*DEBUG_MSG(STP_DEBUG,"bp_state=%d\r\n", bp->bp_state);
DEBUG_MSG(STP_DEBUG,"bp_synced=%d\r\n", bp->bp_synced);
DEBUG_MSG(STP_DEBUG,"bp_agreed=%d\r\n", bp->bp_agreed);
DEBUG_MSG(STP_DEBUG,"bp_operedge=%d\r\n", bp->bp_operedge);
DEBUG_MSG(STP_DEBUG,"bp_sync=%d\r\n", bp->bp_sync);*/
      bstp_timer_stop(&bp->bp_recent_root_timer);
      bp->bp_synced = 1;
      bp->bp_sync = 0;
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> DESIGNATED_SYNCED\r\n",
    		  bp->bp_ifp->name, bp->id);
    }

    if (bp->bp_state != BSTP_IFSTATE_FORWARDING &&
        !bp->bp_agreed && !bp->bp_proposing &&
        !bp->bp_operedge) {
      bp->bp_proposing = 1;
      bp->bp_flags |= BSTP_PORT_NEWINFO;
      bstp_timer_start(&bp->bp_edge_delay_timer,
          (bp->bp_ptp_link ? BSTP_DEFAULT_MIGRATE_DELAY :
           bp->bp_desg_max_age));
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> DESIGNATED_PROPOSE\r\n",
    		  bp->bp_ifp->name, bp->id);
    }

    if (bp->bp_state != BSTP_IFSTATE_FORWARDING &&
        (bp->bp_forward_delay_timer.active == 0 || bp->bp_agreed ||
        bp->bp_operedge) &&
        (bp->bp_recent_root_timer.active == 0 || !bp->bp_reroot) &&
        !bp->bp_sync) {
      if (bp->bp_agreed)
        DEBUG_MSG(STP_DEBUG,"%s(%d) -> AGREED\r\n", bp->bp_ifp->name, bp->id);
      /*
       * If agreed|operedge then go straight to forwarding,
       * otherwise follow discard -> learn -> forward.
       */
      if (bp->bp_agreed || bp->bp_operedge ||
          bp->bp_state == BSTP_IFSTATE_LEARNING) {
        bstp_set_port_state(bp,BSTP_IFSTATE_FORWARDING);
        bp->bp_agreed = bp->bp_protover;
      } else if (bp->bp_state == BSTP_IFSTATE_DISCARDING)
        bstp_set_port_state(bp, BSTP_IFSTATE_LEARNING);
    }

    if (((bp->bp_sync && !bp->bp_synced) ||
        (bp->bp_reroot && bp->bp_recent_root_timer.active) ||
        (bp->bp_flags & BSTP_PORT_DISPUTED)) && !bp->bp_operedge &&
        bp->bp_state != BSTP_IFSTATE_DISCARDING) {
      bstp_set_port_state(bp, BSTP_IFSTATE_DISCARDING);
      bp->bp_flags &= ~BSTP_PORT_DISPUTED;
      bstp_timer_start(&bp->bp_forward_delay_timer,
          bp->bp_protover == BSTP_PROTO_RSTP ?
          bp->bp_desg_htime : bp->bp_desg_fdelay);
      DEBUG_MSG(STP_DEBUG,"%s(%d) -> DESIGNATED_DISCARD\r\n",
    		  bp->bp_ifp->name, bp->id);
    }
    break;
  }

  if (bp->bp_flags & BSTP_PORT_NEWINFO)
    bstp_transmit(bs, bp);
}

void bstp_enable_port(struct bstp_state *bs, struct bstp_port *bp) {
  bp->bp_infois = BSTP_INFO_AGED;
  bstp_assign_roles(bs);
}

void bstp_disable_port(struct bstp_state *bs, struct bstp_port *bp) {
  bp->bp_infois = BSTP_INFO_DISABLED;
  bstp_set_port_state(bp, BSTP_IFSTATE_DISCARDING);
  bstp_assign_roles(bs);
}

void bstp_ifupdstatus(struct bstp_state *bs, struct bstp_port *bp) {
  struct portnet *ifp = bp->bp_ifp;
  if (ifp == NULL)
    return;

  bp->bp_path_cost = bstp_calc_path_cost(bp);
  if ((ifp->if_flags & IFF_UP) &&
      (ifp->if_link_state&LINK_STATE_DOWN)==0) {
    if (bp->bp_flags & BSTP_PORT_AUTOPTP) {
      /* A full-duplex link is assumed to be ptp */
    	if (ifp->if_link_state&LINK_STATE_FULL_DUPLEX) bp->bp_ptp_link = 1;
			else bp->bp_ptp_link = 0;
    }

    if (bp->bp_infois == BSTP_INFO_DISABLED){
      bp->bp_operedge = 0;
      bstp_enable_port(bs, bp);
    }
  } else {
    if (bp->bp_infois != BSTP_INFO_DISABLED){
      bstp_disable_port(bs, bp);
    }
  }
}

void bstp_initialization(struct bstp_state *bs) {



  struct bstp_port *bp;
//  struct portnet *mif = NULL;
  u_char e_addr[6];
  DEBUG_MSG(STP_DEBUG,"bstp_initialization\r\n");

  int flag = 0;
  for (int i=0;i<NUM_BSTP_PORT;i++){
    if (bstp_ports[i].bp_active) {
      flag = 1;
      break;
    }
  }
  if (flag==0) {
    bstp_stop(bs);
    bs->active = 0;
    DEBUG_MSG(STP_DEBUG,"bstp stop\r\n");
    return;
  }
  bs->active = 1;
  DEBUG_MSG(STP_DEBUG,"bstp active %d\r\n", bs->bs_protover);
  get_haddr(e_addr);

#if STP_U64
  bs->bs_bridge_pv.pv_dbridge_id =
       (((u_int64_t)bs->bs_bridge_priority) << 48) |
       (((u_int64_t)e_addr[0]) << 40) |
       (((u_int64_t)e_addr[1]) << 32) |
       (((u_int64_t)e_addr[2]) << 24) |
       (((u_int64_t)e_addr[3]) << 16) |
       (((u_int64_t)e_addr[4]) << 8) |
       (((u_int64_t)e_addr[5]));

   bs->bs_bridge_pv.pv_root_id = bs->bs_bridge_pv.pv_dbridge_id;
#else
  bs->bs_bridge_pv.pv_dbridge_id[1] = ((u_int32_t)(bs->bs_bridge_priority) << 16) |
		  ((u_int32_t)(e_addr[0]) << 8) | (u_int32_t)(e_addr[1]);

  bs->bs_bridge_pv.pv_dbridge_id[0] = ((u_int32_t)(e_addr[2]) << 24) | ((u_int32_t)(e_addr[3]) << 16) |
		  ((u_int32_t)(e_addr[4]) << 8) | (u_int32_t)(e_addr[5]);

  bs->bs_bridge_pv.pv_root_id[0] = bs->bs_bridge_pv.pv_dbridge_id[0];
  bs->bs_bridge_pv.pv_root_id[1] = bs->bs_bridge_pv.pv_dbridge_id[1];
#endif



  bs->bs_bridge_pv.pv_cost = 0;
  bs->bs_bridge_pv.pv_dport_id = 0;
  bs->bs_bridge_pv.pv_port_id = 0;
  bs->isroot = 1;
  bs->sec_cnt = 0;

  if (bs->bs_bstptimeout==0) bs->bs_bstptimeout = 1;

  for (int i=0;i<NUM_BSTP_PORT;i++){
    bp = &bstp_ports[i];
    if (bp->bp_active==0)
    	continue;
    DEBUG_MSG(STP_DEBUG,"bstp reinit port %s(%d), priority %d\r\n", bp->bp_ifp->name, bp->id, bp->bp_priority);
    bp->bp_port_id = (bp->bp_priority << 8) |
        (bp->id & 0xfff);
    bstp_ifupdstatus(bs, bp);
  }

  bstp_assign_roles(bs);
  bstp_timer_start(&bs->bs_link_timer, BSTP_LINK_TIMER);
}

void bstp_create(void){
	if (bstp_state.inited==0){
		bstp_state.bs_bridge_max_age = BSTP_DEFAULT_MAX_AGE;
		bstp_state.bs_bridge_htime = BSTP_DEFAULT_HELLO_TIME;
		bstp_state.bs_bridge_fdelay = BSTP_DEFAULT_FORWARD_DELAY;
		bstp_state.bs_bridge_priority = BSTP_DEFAULT_BRIDGE_PRIORITY;
		bstp_state.bs_hold_time = BSTP_DEFAULT_HOLD_TIME;
		bstp_state.bs_migration_delay = BSTP_DEFAULT_MIGRATE_DELAY;
		bstp_state.bs_txholdcount = BSTP_DEFAULT_HOLD_COUNT;
		bstp_state.bs_protover = BSTP_PROTO_RSTP;	/* STP instead of RSTP? */

		//getmicrotime(&bstp_state.bs_last_tc_time);
		bstp_state.bs_last_tc_time = 0;
		bstp_state.bs_tc_count = 0;
		bstp_state.inited  = 1;

	}
}

static int bstp_add(int port, int id, struct portnet *ifp){
  struct bstp_port *bp;

  DEBUG_MSG(STP_DEBUG,"bstp_add (%d) start\r\n",id);



    if (port>=NUM_BSTP_PORT) return -1;
    bp = &bstp_ports[port];
    ifp = &portnets[port];
    if (bp->bp_active) return -1;

    bp->bp_bs = &bstp_state;
	bp->bp_ifp = ifp;
//	bp->id = id;

    bp->bp_priority = BSTP_DEFAULT_PORT_PRIORITY;
    bp->bp_txcount = 0;
    
      /* Init state */
    bp->bp_infois = BSTP_INFO_DISABLED;
    bp->bp_flags = BSTP_PORT_AUTOEDGE | BSTP_PORT_AUTOPTP;



    bstp_set_port_state(bp, BSTP_IFSTATE_DISCARDING);

    bstp_set_port_proto(bp, bstp_state.bs_protover);
    bstp_set_port_role(bp, BSTP_ROLE_DISABLED);

    bstp_set_port_tc(bp, BSTP_TCSTATE_INACTIVE);
    bp->bp_path_cost = bstp_calc_path_cost(bp);

    bp->bp_active = 1;
    bp->bp_flags |= BSTP_PORT_NEWINFO;

    bstp_initialization(&bstp_state);

    bstp_update_roles(&bstp_state, bp);

    DEBUG_MSG(STP_DEBUG,"bstp_add %s(%d), cost %lu, priority %d\r\n", bp->bp_ifp->name, bp->id, bp->bp_path_cost,bp->bp_priority);

    return 0;
}

int bstp_delete(int port){
    struct bstp_port *bp;

    if (port>=NUM_BSTP_PORT) return -1;

    bp = &bstp_ports[port];
    bstp_set_port_state(bp, BSTP_IFSTATE_FORWARDING);
    if (!bp->bp_active) return -1;
    bp->bp_bs = NULL;
    bp->bp_active = 0;



    bstp_initialization(&bstp_state);
    DEBUG_MSG(STP_DEBUG,"bstp_delete %s(%d)\r\n", bp->bp_ifp->name, bp->id);
    return 0;
}

void bstp_tick(void) {
  struct bstp_state *bs = &bstp_state;
  struct bstp_port *bp;
  int s;

  s = splnet();
  if (bs->active==0) {
    splx(s);
    return;
  }

  //DEBUG_MSG(STP_DEBUG,"bstp_tick %lu\r\n",bstp_state.bs_last_tc_time);



  /* slow timer to catch missed link events */
  if (bstp_timer_expired(&bs->bs_link_timer)) {
    for (int i=0;i<NUM_BSTP_PORT;i++){
      if (bstp_ports[i].bp_active==0) continue;
      bp = &bstp_ports[i];
      bstp_ifupdstatus(bs, bp);
    }
    bstp_timer_start(&bs->bs_link_timer, BSTP_LINK_TIMER);
  }

  for (int i=0;i<NUM_BSTP_PORT;i++){
	if (bstp_ports[i].bp_active==0) continue;
    bp = &bstp_ports[i];
    /* no events need to happen for these */
    bstp_timer_expired(&bp->bp_tc_timer);
    bstp_timer_expired(&bp->bp_recent_root_timer);
    bstp_timer_expired(&bp->bp_forward_delay_timer);
    bstp_timer_expired(&bp->bp_recent_backup_timer);

    if (bstp_timer_expired(&bp->bp_hello_timer))
      bstp_hello_timer_expiry(bs, bp);

    if (bstp_timer_expired(&bp->bp_message_age_timer))
      bstp_message_age_expiry(bs, bp);

    if (bstp_timer_expired(&bp->bp_migrate_delay_timer))
      bstp_migrate_delay_expiry(bs, bp);

    if (bstp_timer_expired(&bp->bp_edge_delay_timer))
      bstp_edge_delay_expiry(bs, bp);

    /* update the various state machines for the port */
    bstp_update_state(bs, bp);
  }

  bs->sec_cnt++;
  if (bs->sec_cnt>=BSTP_TICK_VAL){
	bs->sec_cnt = 0;

	bstp_state.bs_last_tc_time++;
	for (int i=0;i<NUM_BSTP_PORT;i++){
		if (bstp_ports[i].bp_active==0) continue;
		bp = &bstp_ports[i];
		if (bp->bp_txcount > 0) bp->bp_txcount--;
	}
  }

  splx(s);
}

void if_poll(void){
  int link, duplex, changed;
  uint32_t baudrate;
  uint8_t flags;
  struct bstp_port *bp;

  for (int i=0;i<NUM_BSTP_PORT;i++){
    if (bstp_ports[i].bp_active==0)
    	continue;
    bp = &bstp_ports[i];
	  stp_if_poll(bp->id, &link, &duplex, &baudrate);
	  flags = 0;
	  changed = 0;
	  if (link==0) flags|=LINK_STATE_DOWN;
	  if (duplex)  flags|=LINK_STATE_FULL_DUPLEX;

	  if ((baudrate)&&(bp->bp_ifp->if_baudrate!=baudrate)){
		  DEBUG_MSG(STP_DEBUG,"%s(%d): baudrate changed %lu \r\n",bp->bp_ifp->name, bp->id,baudrate);

		  //stp_if_poll(bp->id, &link, &duplex, &baudrate);
		  //DEBUG_MSG(STP_DEBUG,"%s(%d): baudrate changed %lu \r\n",bp->bp_ifp->name, bp->id,baudrate);

		  changed = 1;
	  }
	  if (bp->bp_ifp->if_link_state!=flags){
		  DEBUG_MSG(STP_DEBUG,"%s(%d): link/duplex changed %d \r\n",bp->bp_ifp->name, bp->id,flags);
		  //stp_if_poll(bp->id, &link, &duplex, &baudrate);
		  //if (link==0) flags|=LINK_STATE_DOWN;
		  //if (duplex)  flags|=LINK_STATE_FULL_DUPLEX;
		  //DEBUG_MSG(STP_DEBUG,"%s(%d): link/duplex changed %d \r\n",bp->bp_ifp->name, bp->id,flags);

		  changed = 1;
	  }
	  bp->bp_ifp->if_link_state = flags;
	  if (baudrate) bp->bp_ifp->if_baudrate = baudrate;
	  if (changed){
		DEBUG_MSG(STP_DEBUG,"%s(%d): ifp changed \r\n", bp->bp_ifp->name, bp->id);
		bstp_ifupdstatus(&bstp_state, bp);
		bstp_update_state(&bstp_state, bp);
	  }
  }
}

void bstp_ifstate(struct portnet *ifp){
  struct bstp_state *bs = &bstp_state;
  struct bstp_port *bp;
  int i;
	for (i=0;i<NUM_BSTP_PORT;i++){
      if (bstp_ports[i].bp_active==0) continue;
	  bp = &bstp_ports[i];
	  if (bp->bp_ifp==ifp) break;
	}
	if (i==NUM_BSTP_PORT) return;

	bstp_ifupdstatus(bs, bp);
	bstp_update_state(bs, bp);
}

static void bstp_cfgflags(int port, uint8_t flags){
	struct bstp_port *bp;
	struct bstp_state *bs;
	bp = &bstp_ports[port];
	bs = bp->bp_bs;
	if (bs==NULL) return;

	if (flags&BSTP_PORTCFG_FLAG_ADMCOST) bp->bp_flags|=BSTP_PORT_ADMCOST;
	  else bp->bp_flags&=~BSTP_PORT_ADMCOST;
	/*
	 * Set edge status
	 */
	if (flags & BSTP_PORTCFG_FLAG_AUTOEDGE) {
		if ((bp->bp_flags & BSTP_PORT_AUTOEDGE) == 0) {
			bp->bp_flags |= BSTP_PORT_AUTOEDGE;

			/* we may be able to transition straight to edge */
			if (bp->bp_edge_delay_timer.active == 0)
				bstp_edge_delay_expiry(bs, bp);
		}
	} else
		bp->bp_flags &= ~BSTP_PORT_AUTOEDGE;

	if (flags & BSTP_PORTCFG_FLAG_EDGE)
		bp->bp_operedge = 1;
	else
		bp->bp_operedge = 0;

	/*
	 * Set point to point status
	 */
	if (flags & BSTP_PORTCFG_FLAG_AUTOPTP) {
		if ((bp->bp_flags & BSTP_PORT_AUTOPTP) == 0) {
			bp->bp_flags |= BSTP_PORT_AUTOPTP;

			bstp_ifupdstatus(bs, bp);
		}
	} else
		bp->bp_flags &= ~BSTP_PORT_AUTOPTP;

	if (flags & BSTP_PORTCFG_FLAG_PTP)
		bp->bp_ptp_link = 1;
	else
		bp->bp_ptp_link = 0;
}

static void bstp_reinitialization(struct bstp_state *bs){
 // bstp_cfg_t bstp_cfg;
 // bstp_port_cfg_t port_cfg;

	DEBUG_MSG(STP_DEBUG,"bstp_reinitialization\r\n");
	if (get_stp_magic()!=BSTP_CFG_MAGIC){
		DEBUG_MSG(STP_DEBUG,"bstp_reinitialization reset cfg\r\n");
		bstp_cfg_reset();
	}
#if 1
	bstp_state.bs_bridge_max_age = get_stp_bridge_max_age()* BSTP_TICK_VAL;
	bstp_state.bs_bridge_htime = get_stp_bridge_htime() * BSTP_TICK_VAL;
	bstp_state.bs_bridge_fdelay = get_stp_bridge_fdelay() * BSTP_TICK_VAL;
	bstp_state.bs_bridge_priority = get_stp_bridge_priority();
	bstp_state.bs_migration_delay = get_stp_bridge_mdelay()* BSTP_TICK_VAL;
	bstp_state.bs_txholdcount = get_stp_txholdcount();
	bstp_state.bs_protover = get_stp_proto();
//	bstp_state.active = get_stp_state();
#endif
	for(int i=0;i<NUM_BSTP_PORT;i++){
#if 1
		bstp_ports[i].bp_priority = get_stp_port_priority(i);
		bstp_ports[i].bp_path_cost = get_stp_port_cost(i);

		//если stp отключен, то отключаем его и на всех портах
		if (get_stp_port_enable(i)==0 || get_stp_state() == 0){
			bstp_delete(i);
		} else {
			if (get_stp_port_state(i)){
				portnets[i].if_flags = IFF_UP | IFF_RUNNING;
			}
			else
				portnets[i].if_flags = 0;

			bstp_add(i, bstp_ports[i].id, &portnets[i]);
			bstp_cfgflags(i, get_stp_port_flags(i));
		}

#else
		portnets[i].if_flags = IFF_UP | IFF_RUNNING;
		bstp_add(i, bstp_ports[i].id, &portnets[i]);
#endif
	}

	DEBUG_MSG(STP_DEBUG,"bstp_reinitialization END\r\n");

	bstp_initialization(bs);
}

void bstp_main_task(void *pvParameters){
  bstp_event_t event;
  uint32_t tick_tt, tout, tick_tout;
  int sc, state_change_cnt;

  DEBUG_MSG(STP_DEBUG,"bstp task started\r\n");

  tick_tt = bstp_getticks();
  tick_tout = tout = configTICK_RATE_HZ/BSTP_TICK_VAL;
  state_change_cnt = 0;

//  bstp_state.active = get_stp_state();

  while(1){



	  //DEBUG_MSG(STP_DEBUG,"+tick\r\n");

	  if (bstp_state.bs_changed) {
		  //send_events(EVENT_STP_REINIT,0);
		  DEBUG_MSG(STP_DEBUG,"State changed\r\n");
		  bstp_state.bs_changed = 0;
		  state_change_cnt++;
		  if (state_change_cnt==1)
			  tout = 0;
		  else if (state_change_cnt<3)
			  tout = 1;
		  else if (state_change_cnt<4)
			  tout = 2;
		  else if (state_change_cnt<6)
			  tout = 10;
	  } else
		  state_change_cnt = 0;


	  if (tout)
		  sc = bstp_recv_event(&event, tout);
	  else
		  sc = 1;


      bstp_sem_take();
#if BRIDGESTP_DEBUG
        deb_src = 3;
#endif
      if (sc==0){
		  DEBUG_MSG(STP_DEBUG,"bstp task recv event %d\r\n", event.id);
		  /*if (bstp_state.active)*/{
		  switch(event.id){
			case BSTP_EVENT_STATE_CHANGE:
				if (bstp_state.active){
					DEBUG_MSG(STP_DEBUG,"BSTP_EVENT_STATE_CHANGE %d\r\n", event.port);
					bstp_ifstate(&portnets[event.port]);
				}
			  break;
			case BSTP_EVENT_REINIT:
				DEBUG_MSG(STP_DEBUG,"BSTP_EVENT_REINIT\r\n");
				bstp_reinitialization(&bstp_state);
			  break;
			case BSTP_EVENT_DUMP:
				bstp_dump_();
			  break;
			case BSTP_EVENT_CHANGED:
				DEBUG_MSG(STP_DEBUG,"BSTP_EVENT_CHANGED\r\n");

				for (int i=0;i<NUM_BSTP_PORT;i++){
					if (bstp_ports[i].bp_active==0) continue;
					bstp_update_state(&bstp_state, &bstp_ports[i]);
				}
			  break;
			case BSTP_EVENT_LINK_CHANGE:
				DEBUG_MSG(STP_DEBUG,"BSTP_EVENT_LINK_CHANGE\r\n");
				if_poll();
			  break;
		  }
		}
	  }



	  tout = bstp_getdeltatick(tick_tt);
	  //DEBUG_MSG(STP_DEBUG,"bstp_getdeltatick %lu %lu\r\n",tout,tick_tout);

	  if (tout>=tick_tout){
#if BRIDGESTP_DEBUG
	  deb_src = 2;
	  if ((deb_tick<4)){
		DEBUG_MSG(STP_DEBUG,"\r\n");
		DEBUG_MSG(STP_DEBUG,"sleeped %lu\r\n", tout);
		DEBUG_MSG(STP_DEBUG,"tick!!!\r\n");
		deb_tick++;
	  }
#endif
	    if_poll();
	    bstp_tick();

	    //DEBUG_MSG(STP_DEBUG,"tick!!!+++++++\r\n");


	    tout = bstp_getdeltatick(tick_tt);
	    tick_tt = bstp_getticks();

	    //todo
	    //if(configTICK_RATE_HZ/BSTP_TICK_VAL-(tout - tick_tout)>0)
	    	tout = configTICK_RATE_HZ/BSTP_TICK_VAL - (tout - tick_tout);
	    //else
	    //	tout = configTICK_RATE_HZ/BSTP_TICK_VAL;

	  	tick_tout = tout;

	  	//DEBUG_MSG(STP_DEBUG,"tick_tout = %lu\r\n",tick_tout);
	  }
	  else {
		 if (sc!=0) {
			DEBUG_MSG(STP_DEBUG,"Update changed state\r\n");
			for (int i=0;i<NUM_BSTP_PORT;i++){
				if (bstp_ports[i].bp_active==0) continue;
				bstp_update_state(&bstp_state, &bstp_ports[i]);
			}
		 }
		 tout = tick_tout - tout;
	  }
	  bstp_sem_free();

	  vTaskDelay(100);
  }
  vTaskDelete(NULL);
}

void bstp_set_portstate(int port, int state){
  uint8_t flags;
  int changed = 0;
  	if (port>=NUM_BSTP_PORT)
		return;
	flags = 0;
	if (state)
		flags = IFF_UP | IFF_RUNNING;
	if (portnets[port].if_flags!=flags)
		changed = 1;

	portnets[port].if_flags = flags;
	if (changed){
	    bstp_event_t event;
	    //xUsb_SendString("state changed\r\r\n");
	    event.id = BSTP_EVENT_STATE_CHANGE;
	    event.port = port;
		bstp_send_event(&event);
	}
}

void bstp_set_portlink(int port, int link, int duplex, uint32_t baudrate){
  uint8_t flags;
  int changed = 0;
//if (link==0) return;

	if (port>=NUM_BSTP_PORT)
		return;
	flags = 0;
    if (link==0)
    	flags|=LINK_STATE_DOWN;
    if (duplex)
    	flags|=LINK_STATE_FULL_DUPLEX;
    if ((baudrate)&&(portnets[port].if_baudrate!=baudrate))
    	changed = 1;
    if (portnets[port].if_link_state!=flags)
    	changed = 1;
    portnets[port].if_link_state = flags;
    if (baudrate)
    	portnets[port].if_baudrate = baudrate;
	if (changed){
	    bstp_event_t event;
	    event.id = BSTP_EVENT_STATE_CHANGE;
	    event.port = port;
		bstp_send_event(&event);
	}
}

int bstp_link_change_i(void){
    bstp_event_t event;
	event.id = BSTP_EVENT_LINK_CHANGE;
	return bstp_send_event_i(&event);
}

void bstp_link_change(void){
  bstp_event_t event;
	event.id = BSTP_EVENT_LINK_CHANGE;
	bstp_send_event(&event);
}

void bstp_reinit(void){
  bstp_event_t event;
    event.id = BSTP_EVENT_REINIT;
	bstp_send_event(&event);
}

int bstp_add_port(char *name, int port, int id, unsigned long baudrate){
  struct bstp_port *bp;
  struct portnet *ifp;



	if (port>=NUM_BSTP_PORT)
		return -1;
	bp = &bstp_ports[port];
	if (bp->bp_ifp!=NULL)
		return -1;
	ifp = &portnets[port];
	bp->bp_ifp = ifp;

	strcpy(ifp->name,name);
	ifp->if_baudrate = baudrate;
	ifp->if_flags = IFF_UP | IFF_RUNNING;
	ifp->if_link_state = LINK_STATE_DOWN;

	bstp_ports[port].id = id;
	return 0;
}

void bstp_dump(void){
  bstp_event_t event;
    event.id = BSTP_EVENT_DUMP;
	bstp_send_event(&event);
}

void bstp_get_bridge_stat(bstp_bridge_stat_t *stat){
	bstp_sem_take();
	stat->flags = 0;
	if (bstp_state.isroot) stat->flags|=BSTP_BRIDGE_STAT_FLAG_ROOT;
	if (bstp_state.inited) stat->flags|=BSTP_BRIDGE_STAT_FLAG_INITED;
	if (bstp_state.active) stat->flags|=BSTP_BRIDGE_STAT_FLAG_ACTIVE;
	stat->root_port = bstp_state.bs_root_port->id;

#if STP_U64
	stat->rootpri = htons(bstp_state.bs_root_pv.pv_root_id>>48);
#else
	stat->rootpri = htons(bstp_state.bs_root_pv.pv_root_id[1]>>16);
#endif
	PV2ADDR(bstp_state.bs_root_pv.pv_root_id, stat->rootaddr);
	stat->root_cost = bstp_state.bs_root_pv.pv_cost;
#if STP_U64
	stat->desgpri = htons(bstp_state.bs_root_pv.pv_dbridge_id>>48);
#else
	stat->desgpri = htons(bstp_state.bs_root_pv.pv_dbridge_id[1]>>16);
#endif

	PV2ADDR(bstp_state.bs_root_pv.pv_dbridge_id, stat->desgaddr);
	stat->dport_id = bstp_state.bs_root_pv.pv_dport_id;
	stat->root_port_id = bstp_state.bs_root_pv.pv_port_id;
	stat->last_tc_time = bstp_state.bs_last_tc_time;
	stat->tc_count = bstp_state.bs_tc_count;
	bstp_sem_free();
}

void bstp_get_port_stat(int port, bstp_port_stat_t *stat){
	struct bstp_port *bp;
	bp = &bstp_ports[port];

	bstp_sem_take();

	stat->if_baudrate = bp->bp_ifp->if_baudrate;
	stat->if_flags = bp->bp_ifp->if_flags;
	stat->if_link_state = bp->bp_ifp->if_link_state;
	stat->port_id = bp->bp_port_id;
	stat->priority = bp->bp_priority;
	stat->path_cost = bp->bp_path_cost;
	stat->state = bp->bp_state;
	stat->role = bp->bp_role;
	stat->flags = bp->bp_stflags;
	stat->rx_counter = bp->bp_rxcounter;
	stat->tx_counter = bp->bp_txcounter;
	stat->protover = bp->bp_protover;
	stat->forward_transitions = bp->bp_forward_transitions;

#if STP_U64
	stat->desgpri = htons(bp->bp_desg_pv.pv_dbridge_id>>48);
#else
	stat->desgpri = htons(bp->bp_desg_pv.pv_dbridge_id[1]>>16);
#endif

	PV2ADDR(bp->bp_desg_pv.pv_dbridge_id, stat->desgaddr);

	bstp_sem_free();
}

#endif /*USE_STP*/


