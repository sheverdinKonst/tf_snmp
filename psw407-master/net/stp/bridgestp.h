#ifndef __BRIDGE_STP__
#define __BRIDGE_STP__

//#include "stm32f4xx.h"
#include <stdint.h>
#include "bstp.h"

#define u_int64_t uint64_t
#define u_int32_t uint32_t
#define u_int16_t uint16_t
#define u_int8_t uint8_t
#define u_char unsigned char

//#define BRIDGESTP_DEBUG

#define STP_U64 0//использование 64 битных переменных

//my
#define BSTP_TICK_VAL         (1 * 4)	/* in 256ths of a second */
//#define BSTP_TICK_VAL         (1 * 4)	/* in 256ths of a second */

#define BSTP_MESSAGE_AGE_INCR (1 * BSTP_TICK_VAL)	/* in 256ths of a second */
#define BSTP_LINK_TIMER       (BSTP_TICK_VAL * 15)

/* Protocol versions */
#define	BSTP_PROTO_ID		0x00
#define	BSTP_PROTO_STP		0x00
#define	BSTP_PROTO_RSTP		0x02
#define	BSTP_PROTO_MAX		BSTP_PROTO_RSTP

//use uint64
#if STP_U64
#define	PV2ADDR(pv, eaddr)	do {		\
	eaddr[0] = pv >> 40;			\
	eaddr[1] = pv >> 32;			\
	eaddr[2] = pv >> 24;			\
	eaddr[3] = pv >> 16;			\
	eaddr[4] = pv >> 8;			\
	eaddr[5] = pv >> 0;			\
} while (0)
#else
//use uint32[2]
#define	PV2ADDR(pv, eaddr)	do {		\
	eaddr[0] = pv[1] >> 8;			\
	eaddr[1] = pv[1] >> 0;			\
	eaddr[2] = pv[0] >> 24;			\
	eaddr[3] = pv[0] >> 16;			\
	eaddr[4] = pv[0] >> 8;			\
	eaddr[5] = pv[0] >> 0;			\
} while (0)
#endif

#define	INFO_BETTER	1
#define	INFO_SAME	0
#define	INFO_WORSE	-1

/* STP port states */
#define	BSTP_IFSTATE_DISABLED	0
#define	BSTP_IFSTATE_LISTENING	1
#define	BSTP_IFSTATE_LEARNING	2
#define	BSTP_IFSTATE_FORWARDING	3
#define	BSTP_IFSTATE_BLOCKING	4
#define	BSTP_IFSTATE_DISCARDING	5

#define	BSTP_TCSTATE_ACTIVE	1
#define	BSTP_TCSTATE_DETECTED	2
#define	BSTP_TCSTATE_INACTIVE	3
#define	BSTP_TCSTATE_LEARNING	4
#define	BSTP_TCSTATE_PROPAG	5
#define	BSTP_TCSTATE_ACK	6
#define	BSTP_TCSTATE_TC		7
#define	BSTP_TCSTATE_TCN	8

#define	BSTP_ROLE_DISABLED	0
#define	BSTP_ROLE_ROOT		1
#define	BSTP_ROLE_DESIGNATED	2
#define	BSTP_ROLE_ALTERNATE	3
#define	BSTP_ROLE_BACKUP	4

/* STP port flags */
#define	BSTP_PORT_CANMIGRATE	0x0001
#define	BSTP_PORT_NEWINFO	0x0002
#define	BSTP_PORT_DISPUTED	0x0004
#define	BSTP_PORT_ADMCOST	0x0008
#define	BSTP_PORT_AUTOEDGE	0x0010

/* BPDU priority */
#define	BSTP_PDU_SUPERIOR	1
#define	BSTP_PDU_REPEATED	2
#define	BSTP_PDU_INFERIOR	3
#define	BSTP_PDU_INFERIORALT	4
#define	BSTP_PDU_OTHER		5

/* BPDU flags */
#define	BSTP_PDU_PRMASK		0x0c		/* Port Role */
#define	BSTP_PDU_PRSHIFT	2		/* Port Role offset */
#define	BSTP_PDU_F_UNKN		0x00		/* Unknown port    (00) */
#define	BSTP_PDU_F_ALT		0x01		/* Alt/Backup port (01) */
#define	BSTP_PDU_F_ROOT		0x02		/* Root port       (10) */
#define	BSTP_PDU_F_DESG		0x03		/* Designated port (11) */

#define	BSTP_PDU_STPMASK	0x81		/* strip unused STP flags */
#define	BSTP_PDU_RSTPMASK	0x7f		/* strip unused RSTP flags */
#define	BSTP_PDU_F_TC		0x01		/* Topology change */
#define	BSTP_PDU_F_P		0x02		/* Proposal flag */
#define	BSTP_PDU_F_L		0x10		/* Learning flag */
#define	BSTP_PDU_F_F		0x20		/* Forwarding flag */
#define	BSTP_PDU_F_A		0x40		/* Agreement flag */
#define	BSTP_PDU_F_TCA		0x80		/* Topology change ack */

/*
 * Spanning tree defaults.
 */
#define	BSTP_DEFAULT_MAX_AGE		(20 * BSTP_TICK_VAL)
#define	BSTP_DEFAULT_HELLO_TIME		(2 * BSTP_TICK_VAL)
#define	BSTP_DEFAULT_FORWARD_DELAY	(15 * BSTP_TICK_VAL)
#define	BSTP_DEFAULT_HOLD_TIME		(1 * BSTP_TICK_VAL)
#define	BSTP_DEFAULT_MIGRATE_DELAY	(3 * BSTP_TICK_VAL)
#define	BSTP_DEFAULT_HOLD_COUNT		6
#define	BSTP_DEFAULT_BRIDGE_PRIORITY	61440//0x8000//my
#define	BSTP_DEFAULT_PORT_PRIORITY	0x80
#define	BSTP_DEFAULT_PATH_COST		200000

#define	BSTP_MIN_HELLO_TIME		(1 * BSTP_TICK_VAL)
#define	BSTP_MIN_MAX_AGE		(6 * BSTP_TICK_VAL)
#define	BSTP_MIN_FORWARD_DELAY		(4 * BSTP_TICK_VAL)
#define	BSTP_MIN_HOLD_COUNT		1
#define	BSTP_MAX_HELLO_TIME		(2 * BSTP_TICK_VAL)
#define	BSTP_MAX_MAX_AGE		(40 * BSTP_TICK_VAL)
#define	BSTP_MAX_FORWARD_DELAY		(30 * BSTP_TICK_VAL)
#define	BSTP_MAX_HOLD_COUNT		10
#define	BSTP_MAX_PRIORITY		32768
#define	BSTP_MAX_PORT_PRIORITY		240
#define	BSTP_MAX_PATH_COST		200000000

/* BPDU message types */
#define	BSTP_MSGTYPE_CFG	0x00		/* Configuration */
#define	BSTP_MSGTYPE_RSTP	0x02		/* Rapid STP */
#define	BSTP_MSGTYPE_TCN	0x80		/* Topology chg notification */

#define	BSTP_INFO_RECEIVED	1
#define	BSTP_INFO_MINE		2
#define	BSTP_INFO_AGED		3
#define	BSTP_INFO_DISABLED	4

/* STP port states */
#define	BSTP_IFSTATE_DISABLED	0
#define	BSTP_IFSTATE_LISTENING	1
#define	BSTP_IFSTATE_LEARNING	2
#define	BSTP_IFSTATE_FORWARDING	3
#define	BSTP_IFSTATE_BLOCKING	4
#define	BSTP_IFSTATE_DISCARDING	5

#define	BSTP_TCSTATE_ACTIVE	1
#define	BSTP_TCSTATE_DETECTED	2
#define	BSTP_TCSTATE_INACTIVE	3
#define	BSTP_TCSTATE_LEARNING	4
#define	BSTP_TCSTATE_PROPAG	5
#define	BSTP_TCSTATE_ACK	6
#define	BSTP_TCSTATE_TC		7
#define	BSTP_TCSTATE_TCN	8

#define	BSTP_ROLE_DISABLED	0
#define	BSTP_ROLE_ROOT		1
#define	BSTP_ROLE_DESIGNATED	2
#define	BSTP_ROLE_ALTERNATE	3
#define	BSTP_ROLE_BACKUP	4

/* STP port flags */
#define	BSTP_PORT_CANMIGRATE	0x0001
#define	BSTP_PORT_NEWINFO	0x0002
#define	BSTP_PORT_DISPUTED	0x0004
#define	BSTP_PORT_ADMCOST	0x0008//manual cost
#define	BSTP_PORT_AUTOEDGE	0x0010
#define	BSTP_PORT_AUTOPTP	0x0020


/*
 * Unnumbered LLC format commands
 */
#define LLC_UI		0x3
#define LLC_UI_P	0x13
#define LLC_DISC	0x43
#define	LLC_DISC_P	0x53
#define LLC_UA		0x63
#define LLC_UA_P	0x73
#define LLC_TEST	0xe3
#define LLC_TEST_P	0xf3
#define LLC_FRMR	0x87
#define	LLC_FRMR_P	0x97
#define LLC_DM		0x0f
#define	LLC_DM_P	0x1f
#define LLC_XID		0xaf
#define LLC_XID_P	0xbf
#define LLC_SABME	0x6f
#define LLC_SABME_P	0x7f

/*
 * ISO PDTR 10178 contains among others
 */
#define LLC_8021D_LSAP	0x42
#define LLC_X25_LSAP	0x7e
#define LLC_SNAP_LSAP	0xaa
#define LLC_ISO_LSAP	0xfe

/*
 * Structure of a 10Mb/s Ethernet header.
 */

struct bstp_pri_vector {
#if STP_U64
  u_int64_t	pv_root_id;
  u_int64_t	pv_dbridge_id;
#else
  u_int32_t pv_root_id[2];
  u_int32_t pv_dbridge_id[2];
#endif
  u_int32_t	pv_cost;
  u_int16_t	pv_dport_id;
  u_int16_t	pv_port_id;
};

struct bstp_config_unit {
  struct bstp_pri_vector  cu_pv;
  u_int16_t cu_message_age;
  u_int16_t cu_max_age;
  u_int16_t cu_forward_delay;
  u_int16_t cu_hello_time;
  u_int8_t  cu_message_type;
  u_int8_t  cu_topology_change_ack;
  u_int8_t  cu_topology_change;
  u_int8_t  cu_proposal;
  u_int8_t  cu_agree;
  u_int8_t  cu_learning;
  u_int8_t  cu_forwarding;
  u_int8_t  cu_role;
};

struct bstp_tcn_unit {
  u_int8_t	tu_message_type;
};

struct bstp_timer {
  u_int16_t value;
  u_int8_t latched;
  u_int8_t active;
};

struct portnet {
  char name[16];
  unsigned long if_baudrate;
  unsigned char if_flags;
#define IFF_UP (1<<0)
#define IFF_RUNNING (1<<1)
  unsigned char if_link_state;
#define LINK_STATE_DOWN (1<<0)
#define LINK_STATE_FULL_DUPLEX (1<<1)
};

typedef struct {
	uint16_t flags;
	struct{
		uint16_t bp_tc_ack_;
		uint16_t bp_tc_prop_;
		uint16_t bp_fdbflush_;
		uint16_t bp_ptp_link_;
		uint16_t bp_agree_;
		uint16_t bp_agreed_;
		uint16_t bp_sync_;
		uint16_t bp_synced_;
		uint16_t bp_proposing_;
		uint16_t bp_proposed_;
		uint16_t bp_operedge_;
		uint16_t bp_reroot_;
		uint16_t bp_rcvdtc_;
		uint16_t bp_rcvdtca_;
		uint16_t bp_rcvdtcn_;
		uint16_t bp_active_;
	};
} bp_flags_t;

struct bstp_port {
  uint8_t id;
  u_int8_t    bp_protover;
  u_int16_t   bp_port_id;

  struct portnet  *bp_ifp;
  struct bstp_state *bp_bs;

  u_int32_t   bp_flags;
  u_int32_t   bp_path_cost;
  u_int16_t   bp_port_msg_age;
  u_int16_t   bp_port_max_age;
  u_int16_t   bp_port_fdelay;
  u_int16_t   bp_port_htime;
  u_int16_t   bp_desg_msg_age;
  u_int16_t   bp_desg_max_age;
  u_int16_t   bp_desg_fdelay;
  u_int16_t   bp_desg_htime;
  struct bstp_timer bp_edge_delay_timer;
  struct bstp_timer bp_forward_delay_timer;
  struct bstp_timer bp_hello_timer;
  struct bstp_timer bp_message_age_timer;
  struct bstp_timer bp_migrate_delay_timer;
  struct bstp_timer bp_recent_backup_timer;
  struct bstp_timer bp_recent_root_timer;
  struct bstp_timer bp_tc_timer;
  struct bstp_config_unit bp_msg_cu;
  struct bstp_pri_vector  bp_desg_pv;
  struct bstp_pri_vector  bp_port_pv;

  u_int8_t    bp_state;
  u_int8_t    bp_tcstate;
  u_int8_t    bp_role;
  u_int8_t    bp_infois;
  bp_flags_t  bp_stflags;
#define bp_active bp_stflags.bp_active_
#define bp_tc_ack bp_stflags.bp_tc_ack_
#define bp_tc_prop bp_stflags.bp_tc_prop_
#define bp_fdbflush bp_stflags.bp_fdbflush_
#define bp_ptp_link bp_stflags.bp_ptp_link_
#define bp_agree bp_stflags.bp_agree_
#define bp_agreed bp_stflags.bp_agreed_
#define bp_sync bp_stflags.bp_sync_
#define bp_synced bp_stflags.bp_synced_
#define bp_proposing bp_stflags.bp_proposing_
#define bp_proposed bp_stflags.bp_proposed_
#define bp_operedge bp_stflags.bp_operedge_
#define bp_reroot bp_stflags.bp_reroot_
#define bp_rcvdtc bp_stflags.bp_rcvdtc_
#define bp_rcvdtca bp_stflags.bp_rcvdtca_
#define bp_rcvdtcn bp_stflags.bp_rcvdtcn_
/*	  u_int8_t    bp_tc_ack;
	  u_int8_t    bp_tc_prop;
	  u_int8_t    bp_fdbflush;
	  u_int8_t    bp_ptp_link;
	  u_int8_t    bp_agree;
	  u_int8_t    bp_agreed;
	  u_int8_t    bp_sync;
	  u_int8_t    bp_synced;
	  u_int8_t    bp_proposing;
	  u_int8_t    bp_proposed;
	  u_int8_t    bp_operedge;
	  u_int8_t    bp_reroot;
	  u_int8_t    bp_rcvdtc;
	  u_int8_t    bp_rcvdtca;
	  u_int8_t    bp_rcvdtcn;*/
  u_int32_t   bp_forward_transitions;
  u_int8_t    bp_priority;
  u_int8_t    bp_txcount;
  u_int8_t    bp_rxcounter;
  u_int8_t    bp_txcounter;
};


struct bstp_state{
  uint8_t inited;
  uint8_t active;
  struct bstp_pri_vector	bs_bridge_pv;
  struct bstp_pri_vector	bs_root_pv;
  struct bstp_port	*bs_root_port;
  u_int8_t    bs_protover;
  u_int16_t   bs_migration_delay;
  u_int16_t   bs_edge_delay;
  u_int16_t   bs_bridge_max_age;
  u_int16_t   bs_bridge_fdelay;
  u_int16_t   bs_bridge_htime;
  u_int16_t   bs_root_msg_age;
  u_int16_t   bs_root_max_age;
  u_int16_t   bs_root_fdelay;
  u_int16_t   bs_root_htime;
  u_int16_t   bs_hold_time;
  u_int16_t   bs_bridge_priority;
  u_int8_t    bs_txholdcount;
  u_int8_t    bs_allsynced;
  
  u_int8_t    bs_bstptimeout;
  struct bstp_timer bs_link_timer;
  uint32_t    bs_last_tc_time;
  uint32_t    bs_tc_count;

  uint8_t     isroot;
  u_int8_t    bs_changed;
  
  u_int16_t   sec_cnt;
};

/*
 * Because BPDU's do not make nicely aligned structures, two different
 * declarations are used: bstp_?bpdu (wire representation, packed) and
 * bstp_*_unit (internal, nicely aligned version).
 */
#pragma pack(push, 1)
/* configuration bridge protocol data unit */
struct bstp_cbpdu {
	u_int8_t	cbu_dsap;		/* LLC: destination sap */
	u_int8_t	cbu_ssap;		/* LLC: source sap */
	u_int8_t	cbu_ctl;		/* LLC: control */
	u_int16_t	cbu_protoid;		/* protocol id */
	u_int8_t	cbu_protover;		/* protocol version */
	u_int8_t	cbu_bpdutype;		/* message type */
	u_int8_t	cbu_flags;		/* flags (below) */

	/* root id */
	u_int16_t	cbu_rootpri;		/* root priority */
	u_int8_t	cbu_rootaddr[6];	/* root address */

	u_int32_t	cbu_rootpathcost;	/* root path cost */

	/* bridge id */
	u_int16_t	cbu_bridgepri;		/* bridge priority */
	u_int8_t	cbu_bridgeaddr[6];	/* bridge address */

	u_int16_t	cbu_portid;		/* port id */
	u_int16_t	cbu_messageage;		/* current message age */
	u_int16_t	cbu_maxage;		/* maximum age */
	u_int16_t	cbu_hellotime;		/* hello time */
	u_int16_t	cbu_forwarddelay;	/* forwarding delay */
	u_int8_t	cbu_versionlen;		/* version 1 length */
} __attribute__ ((packed));

#define	BSTP_BPDU_STP_LEN	(3 + 35)	/* LLC + STP pdu */
#define	BSTP_BPDU_RSTP_LEN	(3 + 36)	/* LLC + RSTP pdu */

/* topology change notification bridge protocol data unit */
struct bstp_tbpdu {
	u_int8_t	tbu_dsap;		/* LLC: destination sap */
	u_int8_t	tbu_ssap;		/* LLC: source sap */
	u_int8_t	tbu_ctl;		/* LLC: control */
	u_int16_t	tbu_protoid;		/* protocol id */
	u_int8_t	tbu_protover;		/* protocol version */
	u_int8_t	tbu_bpdutype;		/* message type */
} __attribute__ ((packed));

#pragma pack(pop)

typedef struct{
  uint8_t flags;
#define BSTP_BRIDGE_STAT_FLAG_INITED (1<<0)
#define BSTP_BRIDGE_STAT_FLAG_ACTIVE (1<<1)
#define BSTP_BRIDGE_STAT_FLAG_ROOT (1<<2)
  uint8_t root_port;
  u_int16_t	rootpri;		/* root priority */
  u_int8_t	rootaddr[6];	/* root address */
  u_int32_t root_cost;
  u_int16_t	desgpri;
  u_int8_t	desgaddr[6];
  u_int16_t dport_id;
  u_int16_t root_port_id;
  u_int32_t last_tc_time;
  u_int32_t tc_count;
} bstp_bridge_stat_t;

typedef struct{
  unsigned long if_baudrate;
  unsigned char if_flags;
  unsigned char if_link_state;
  u_int8_t protover;
  u_int16_t port_id;
  u_int8_t priority;
  u_int32_t path_cost;
  u_int8_t state;
  u_int8_t role;
  bp_flags_t flags;
  u_int8_t rx_counter;
  u_int8_t tx_counter;
  u_int32_t   forward_transitions;
  u_int16_t	desgpri;
  u_int8_t	desgaddr[6];
} bstp_port_stat_t;

extern struct bstp_port bstp_ports[NUM_BSTP_PORT];
extern struct bstp_state bstp_state;
extern struct portnet portnets[NUM_BSTP_PORT];

void bstp_create(void);
void bstp_task_start(void);
void bstp_reinit(void);

void bstp_rawinput(uint8_t *ptr, int size);

int bstp_add_port(char *name, int port, int id, unsigned long baudrate);
int bstp_del_port(int port, int reinit);

void bstp_set_portstate(int port, int state);
void bstp_set_portlink(int port, int link, int duplex, uint32_t baudrate);
void bstp_link_change(void);
int bstp_link_change_i(void);

void bstp_get_bridge_stat(bstp_bridge_stat_t *stat);
void bstp_get_port_stat(int port, bstp_port_stat_t *stat);
void bstp_dump(void);

#endif /*__BRIDGE_STP__*/
