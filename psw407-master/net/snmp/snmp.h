#ifndef _SNMP_H_
#define _SNMP_H_

#ifndef SNMP_TRAP_PORT
#define SNMP_TRAP_PORT 162
#endif
#define SNMP_PORT 161


#define SNMP_GET_REQUEST		0
#define SNMP_GET_NEXT_REQUEST	1
#define SNMP_SET_REQUEST		3
#define SNMP_GET_RESPONSE		2
#define SNMP_TRAP				7

#define SNMP_ES_NOERROR 0
#define SNMP_ES_TOOBIG 1
#define SNMP_ES_NOSUCHNAME 2
#define SNMP_ES_BADVALUE 3
#define SNMP_ES_READONLY 4
#define SNMP_ES_GENERROR 5

#define SNMP_GENTRAP_COLDSTART 	0
#define SNMP_GENTRAP_WARMSTART 	1
#define SNMP_GENTRAP_LINKDOWN 	2
#define SNMP_GENTRAP_LINKUP 	3
#define SNMP_GENTRAP_AUTHFAIL 	4
#define SNMP_GENTRAP_ENTERPRISESPC 6

#define SNMP_UP		1
#define	SNMP_DOWN	2

/** fixed maximum length for object identifier type */
#define LWIP_SNMP_OBJ_ID_LEN 32

#define SNMP_TRAP_DESTINATIONS 1

//рока поддерживаем snmp v1 trap
#define SNMP__VERSION_1 		0
#define SNMP__VERSION_2C 		1
#define SNMP__VERSION_3 		2


//snmp v2
#define NOERROR		0

#define ERR_ARG -1



#define SNMP_BIND_MAXNUM 5 //max num of variable bindings

#define IPADDR_ANY NULL
#define uip_ipaddr_isany(addr1) ((addr1) == IPADDR_ANY)

typedef  int err_t;

#define if_snmp_ready()  snmp_send == 1
#define snmp_ready()  snmp_send = 1
#define snmp_empty()  snmp_send = 0


/*
   ----------------------------------
   ---------- SNMP options ----------
   ----------------------------------
*/

/**
 * SNMP_CONCURRENT_REQUESTS: Number of concurrent requests the module will
 * allow. At least one request buffer is required.
 * Does not have to be changed unless external MIBs answer request asynchronously
 */
#ifndef SNMP_CONCURRENT_REQUESTS
#define SNMP_CONCURRENT_REQUESTS        1
#endif

/**
 * SNMP_TRAP_DESTINATIONS: Number of trap destinations. At least one trap
 * destination is required
 */
#ifndef SNMP_TRAP_DESTINATIONS
#define SNMP_TRAP_DESTINATIONS          1
#endif

/**
 * SNMP_PRIVATE_MIB:
 * When using a private MIB, you have to create a file 'private_mib.h' that contains
 * a 'struct mib_array_node mib_private' which contains your MIB.
 */
#ifndef SNMP_PRIVATE_MIB
#define SNMP_PRIVATE_MIB                0
#endif

/**
 * Only allow SNMP write actions that are 'safe' (e.g. disabeling netifs is not
 * a safe action and disabled when SNMP_SAFE_REQUESTS = 1).
 * Unsafe requests are disabled by default!
 */
#ifndef SNMP_SAFE_REQUESTS
#define SNMP_SAFE_REQUESTS              1
#endif

/**
 * The maximum length of strings used. This affects the size of
 * MEMP_SNMP_VALUE elements.
 */
#ifndef SNMP_MAX_OCTET_STRING_LEN
#define SNMP_MAX_OCTET_STRING_LEN       127
#endif

/**
 * The maximum depth of the SNMP tree.
 * With private MIBs enabled, this depends on your MIB!
 * This affects the size of MEMP_SNMP_VALUE elements.
 */
#ifndef SNMP_MAX_TREE_DEPTH
#define SNMP_MAX_TREE_DEPTH             15
#endif

/**
 * The size of the MEMP_SNMP_VALUE elements, normally calculated from
 * SNMP_MAX_OCTET_STRING_LEN and SNMP_MAX_TREE_DEPTH.
 */
//#ifndef SNMP_MAX_VALUE_SIZE
//#define SNMP_MAX_VALUE_SIZE             LWIP_MAX((SNMP_MAX_OCTET_STRING_LEN)+1, sizeof(s32_t)*(SNMP_MAX_TREE_DEPTH))
//#endif





struct varbind2_t {
	u8 flag;
	u8 objname[16];
	u8 len;
	u32 value;
	u8 type;
	u8 value_str[/*16*/32];
	u8 value_str_len;
};




struct snmp_msg2_t{
	u8 version;
	char community[32];
	u8 enterprise[32];
	u8 gentrap;
	u8 spectrap[16];
	struct varbind2_t varbind[SNMP_BIND_MAXNUM];
};







//struct snmp_cfg_t snmp_cfg;

struct snmp_msg2_t snmp_msg;
u8 snmp_string[16];
u32 snmp_var;
u8 snmp_send;



i8 snmp_send_trap(struct snmp_msg2_t *snmp_msg);
u8 snmp_trap_init(void);
void snmp_appcall(void);
u8 check_msg_before_send (struct snmp_msg2_t *snmp_msg);
u8 set_snmp_cfg(void);
u8 get_snmp_cfg(void);
u8 set_snmp_default(void);
u8 make_snmp_msg(u8 *trap,u32 value);

void snmp_msg_appcall(u8 *buff,u16 len);

#endif
