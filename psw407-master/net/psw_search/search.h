/*
 * search.h
 *
 *  Created on: 17.03.2014
 *      Author: Alex
 */

#ifndef SEARCH_H_
#define SEARCH_H_



#define PSW_PORT 	6123

//msg type
#define PSW_REQUEST     		0xE0
#define PSW_RESPONSE   		 	0xE1
#define PSW_SETTINGS_REQUEST	0xE2
#define PSW_SETTINGS_RESPONSE	0xE3


/*
#ifndef DEV_TYPE
	#define DEV_TYPE DEV_PSW2GPLUS
#endif
*/

#define NO_ERROR	0
#define ERROR_AUTH	1
#define ERROR_FILE	2

struct search_mac_entry_t {
	u8 mac[6];
	u8 ip[4];
};

struct search_out_msg_t {
	u8 msg_type;
	u8 dev_type;
	u8 ip[4];
	u8 mac[6];
	char dev_descr[128];
	char dev_loc[128];
	u32 uptime;
	u32 firmware;
	u8 mask[4];
	u8 gate[4];
	struct search_mac_entry_t mac_entry[PORT_NUM];
};

struct search_sett_msg_t {
	u8 msg_type;
	u8 mac[6];
	char pass_md5[32];//str(md5(mac+username+pass))
	char buff[1024];
};

void psw_search_init(void);
void psw_search_appcall(void);

#endif /* SEARCH_H_ */
