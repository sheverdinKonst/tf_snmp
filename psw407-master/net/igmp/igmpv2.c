/*includes*/
#include "../deffines.h"
#include <string.h>
#if LWIP_IGMP

#include "board.h"
#include "stm32f4xx.h"
#include "stm32f4x7_eth.h"
#include "../stp/stp_oslayer_freertos.h"
#include "igmpv2.h"
#include "igmp_mv.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../uip/uip.h"
#include "SMIApi.h"
#include "selftest.h"
#include "../inc/Salsa2Regs.h"
#include "debug.h"
#include "settings.h"
#include "salsa2.h"
/*defines*/
/*
 * IGMP constants
 */
#define IGMP_TTL                       1
#define IGMP_MINLEN                    8
#define ROUTER_ALERT                   0x9404U
#define ROUTER_ALERTLEN                4

#define IGMP_QUEUE_SIZE				   64
/*
 * IGMP message types, including version number.
 */
#define IGMP_MEMB_QUERY                0x11 /* Membership query         */ // запрос
#define IGMP_V1_MEMB_REPORT            0x12 /* Ver. 1 membership report */	//отчет - вступление в группу
#define IGMP_V2_MEMB_REPORT            0x16 /* Ver. 2 membership report */
#define IGMP_V3_MEMB_REPORT            0x22 /* Ver. 3 membership report */
#define IGMP_LEAVE_GROUP               0x17 /* Leave-group message      */ //выход
/* Group  membership states */
#define IGMP_GROUP_NON_MEMBER          0//не член группы
#define IGMP_GROUP_DELAYING_MEMBER     1//временный член группы(переходит в 0 по истечению таймера)
#define IGMP_GROUP_IDLE_MEMBER         2//постоянный член группы


/*prototypes*/
static u8 igmp_mac_filter(u8 netif, uip_ipaddr_t groupaddr,u8 filter);
static int igmp_lookup_group(u8 ifp, uip_ipaddr_t addr);
void IgmpTask(void *pvParameters);



/*variable*/
static struct stats_igmp igmp_stat;
static struct igmp_group_t igmp_group_list[MAX_IGMP_GROUPS] __attribute__ ((section (".ccmram")));
static u8 igmp_group_num = 0;
static struct igmp_t igmp_st;
static struct igmp_t *igmp;
//static u8 igmp_querier = 0;//psw is querier if 1


//static const uip_ipaddr_t all_zeroes_addr =  {0x0000,0x0000};

//xQueueHandle IgmpQueue;
xTaskHandle	xIgmpTask;
uip_ipaddr_t     allsystems;
uip_ipaddr_t     allrouters;
extern uint16_t IDENTIFICATION;
//extern u8 dev_addr[6];


/*convert uip_ipadr_t 2 uint32_t (to compare)*/
static u32 ip_2_u32(uip_ipaddr_t ip){
u32 tmp;
	tmp =  (uip_ipaddr1(ip) <<24) | ( uip_ipaddr2(ip)<<16 ) | (uip_ipaddr3(ip)<<8 ) | (uip_ipaddr4(ip));
	return tmp;
}



static void make_igmp_addr(uint8_t *mac, uip_ipaddr_t groupaddr){
	mac[0] = 0x01;
	mac[1] = 0;
	mac[2] = 0x5E;
	mac[3] = (uip_ipaddr2(groupaddr)) & 0x7F;
	mac[4] = uip_ipaddr3(groupaddr);
	mac[5] = uip_ipaddr4(groupaddr);
}

void igmp_appcall(void){
  int port = -1;

  if(get_igmp_snooping_state() == ENABLE){
	  port = F2L_port_conv(stp_ethhdr2port(uip_buf));
	  struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
	  igmp_input(port,m->destipaddr);
  }
}


/**
 * Initialize the IGMP module
 * IGMP Queue
 */
void
igmp_init(void)
{
  DEBUG_MSG(IGMP_DEBUG,"igmp_init: initializing\r\n");
  //IgmpQueue = xQueueCreate(PORT_NUM,IGMP_QUEUE_SIZE);
  //if(IgmpQueue == NULL){
  //	  ADD_ALARM(ERROR_CREATE_IGMP_QUEUE);
  // }

  igmp = &igmp_st;

  igmp->state = IGMP_IDLE;
  igmp->state_querier = IGMP_NOQUERIER;


  uip_ipaddr(&allsystems, 224, 0, 0, 1);
  uip_ipaddr(&allrouters, 224, 0, 0, 2);
}



#if IGMP_DEBUG
/**
 * Dump global IGMP groups list
 */
void igmp_dump_group_list(void)
{
  DEBUG_MSG(IGMP_DEBUG,"igmp_dump_group_list\r\n");
  for(u8 i=0;i<igmp_group_num;i++){
	DEBUG_MSG(IGMP_DEBUG,"  %d:  %d.%d.%d.%d ports:", i,uip_ipaddr1(&igmp_group_list[i].group_addr),uip_ipaddr2(igmp_group_list[i].group_addr),
    		uip_ipaddr3(&igmp_group_list[i].group_addr),uip_ipaddr4(&igmp_group_list[i].group_addr));
    for(u8 j = 0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
    	if(igmp_group_list[i].ports[j] == 1)
    		DEBUG_MSG(IGMP_DEBUG," %d",j);
    }
    DEBUG_MSG(IGMP_DEBUG," reports: ");
    for(u8 j = 0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
    	if(igmp_group_list[i].reports[j])
    		DEBUG_MSG(IGMP_DEBUG," %d(%d)",j,igmp_group_list[i].reports[j]);
    }

    DEBUG_MSG(IGMP_DEBUG," group state %d ",igmp_group_list[i].state);

    DEBUG_MSG(IGMP_DEBUG,"\r\n");
  }
  DEBUG_MSG(IGMP_DEBUG,"\r\n");
  if(igmp_group_num == 0)
	  DEBUG_MSG(IGMP_DEBUG,"empty group list\r\n");
}
#endif /* LWIP_DEBUG */

u8 get_igmp_groupsnum(void){
	return igmp_group_num;
}

u8 get_igmp_querier(void){
	return igmp_st.state_querier;
}

void get_igmp_group(u8 num, uip_ipaddr_t addr){
	uip_ipaddr_copy(addr,igmp_group_list[num].group_addr);
}

u8 get_igmp_group_port(u8 num,u8 port){
	return igmp_group_list[num].ports[port];
}

/*запуск igmp на интерфейсе:
 * **железные настройки Marvell`a
 * **добавление в группу allsystems*/
i8 igmp_start(u8 netif){
	u8 hw_port;
	u16 tmp;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		if((get_igmp_snooping_state() == ENABLE)&&(get_igmp_port_state(netif) == ENABLE)){
			//hardware init
			DEBUG_MSG(IGMP_DEBUG,"igmp_start: hw config if %d\r\n", netif);
			hw_port = L2F_port_conv(netif);
			//disable forwarding unknown multicast
			tmp = ETH_ReadPHYRegister(0x10+hw_port,0x08);
			tmp &=~0x40;
			ETH_WritePHYRegister(0x10+hw_port, 0x08, tmp);
			//igmp to cpu port
			tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
			tmp |=0x0400;
			ETH_WritePHYRegister(0x10+hw_port, 0x04, tmp);

			//sw init
			DEBUG_MSG(IGMP_DEBUG,"igmp_start: starting IGMP processing on if %d\r\n", netif);
			//igmp_lookup_group(netif, allsystems);

			//my uncomment later
			//igmp_mac_filter(netif, allsystems, IGMP_ADD_MAC_FILTER);
		}
	}
	else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		if((get_igmp_snooping_state() == ENABLE)&&(get_igmp_port_state(netif) == ENABLE)){
			//hardware init
			DEBUG_MSG(IGMP_DEBUG,"igmp_start: hw config if %d\r\n", netif);
			hw_port = L2F_port_conv(netif);
			//disable forwarding unknown multicast
			tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
			tmp &=~0x08;
			ETH_WritePHYRegister(0x10+hw_port, 0x04, tmp);
			//igmp to cpu port
			tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
			tmp |=0x0400;
			ETH_WritePHYRegister(0x10+hw_port, 0x04, tmp);

			//sw init
			DEBUG_MSG(IGMP_DEBUG,"igmp_start: starting IGMP processing on if %d\r\n", netif);
		}
	}
	if(get_marvell_id() == DEV_98DX316){
		//trap igmp andpass to CPU
		hw_port = L2F_port_conv(netif);
		Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG + hw_port*0x1000,14,1,1);
	}
	return 0;
}


/**
 * Search for a specific igmp group and create a new one if not found-
 *
 * @param ifp the network interface for which to look
 * @param addr the group ip address to search
 * return  group_num - number of existing group
 * 		   -1 - error of create

 */
static int igmp_lookup_group(u8 ifp, uip_ipaddr_t addr)
{
  int group_num;
  /* Search if the group already exists */
  group_num = igmp_lookfor_group(ifp, addr);
  if (group_num != -1) {
	// Group already exists.
	//присоединяем порт к группе
	if(group_num == -2){
		for(u8 i=0;i<igmp_group_num;i++){
			if ((uip_ipaddr_cmp(igmp_group_list[i].group_addr, addr))) {
				group_num = i;
			}
		}
	}
	igmp_group_list[group_num].ports[ifp] = 1;
	DEBUG_MSG(IGMP_DEBUG,"igmp_lookup_group: allocated a group with address ");
	DEBUG_MSG(IGMP_DEBUG,"%d.%d.%d.%d",uip_ipaddr1(addr),uip_ipaddr2(addr),
	  		uip_ipaddr3(addr),uip_ipaddr4(addr));
	DEBUG_MSG(IGMP_DEBUG," on if %d\r\n", ifp);
    return group_num;
  }

  //нет группы, создаем новую
  igmp_group_list[igmp_group_num].ports[ifp] = 1;
  uip_ipaddr_copy(igmp_group_list[igmp_group_num].group_addr, addr);

  //increment group num
  igmp_group_num++;

  DEBUG_MSG(IGMP_DEBUG,"igmp_lookup_group: allocated a group with address ");
  DEBUG_MSG(IGMP_DEBUG,"%d.%d.%d.%d",uip_ipaddr1(addr),uip_ipaddr2(addr),
  		uip_ipaddr3(addr),uip_ipaddr4(addr));
  DEBUG_MSG(IGMP_DEBUG," on if %d\r\n", ifp);

  //return
  return igmp_group_num-1;
}

/**
 * Search for a group in the global igmp_group_list
 *
 * @param ifp the network interface for which to look
 * @param addr the group ip address to search for
 * @return current number of group if the group has been found,
 *         NULL if the group wasn't found.
 */
int igmp_lookfor_group(u8  ifp, uip_ipaddr_t addr)
{
  for(u8 i=0;i<igmp_group_num;i++){
	if ((uip_ipaddr_cmp(igmp_group_list[i].group_addr, addr))) {
		//for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if(igmp_group_list[i].ports[ifp] == 1){
				DEBUG_MSG(IGMP_DEBUG,"Group and port exist on if %d\r\n",ifp);
				return i;
			}
		//}
	    DEBUG_MSG(IGMP_DEBUG,"Group exist, port not exist on if %d\r\n",ifp);
        return -2;
    }
  }
  DEBUG_MSG(IGMP_DEBUG,"Group dont exist\r\n");
  return -1;
}

/**
 * Remove a group in the global igmp_group_list
 *
 * @param group the group to remove from the global igmp_group_list
 * @return ERR_OK if group was removed from the list, an err_t otherwise
 */
static i8 igmp_remove_group(u16 group_num)
{
  if(group_num>igmp_group_num)
	  return -1;

  for(u8 i=group_num;i<igmp_group_num-1;i++){

	  if(uip_ipaddr_cmp(igmp_group_list[i].group_addr,allsystems)){
		  return 0;//dont delete allsystems group
	  }

	  DEBUG_MSG(IGMP_DEBUG,"IGMP remove group %d\r\n",group_num);

	  for(u8 j=0;j<(ALL_PORT_NUM);j++){
		  igmp_group_list[i].ports[j]=igmp_group_list[i+1].ports[j];
		  igmp_group_list[i].reports[j] = igmp_group_list[i+1].reports[j];
		  igmp_group_list[i].leaves[j] = igmp_group_list[i+1].leaves[j];
	  }
	  uip_ipaddr_copy(igmp_group_list[i].group_addr,igmp_group_list[i+1].group_addr);

  }
  igmp_group_num--;
  return 0;
}



/**
 * Remove a group in the global igmp_group_list
 *
 * @param group the group to remove from the global igmp_group_list
 * @return ERR_OK if group was removed from the list, an err_t otherwise
 */
static i8 igmp_remove_port(u16 group_num, u8 port)
{
  u8 j,port_mem=0;

  if(group_num>igmp_group_num)
	  return -1;

  DEBUG_MSG(IGMP_DEBUG,"igmp_remove_port: gr %d port %d\r\n",group_num,port);

  igmp_group_list[group_num].ports[port] = 0;

  //del ip addr from
  igmp_mac_filter(port,igmp_group_list[group_num].group_addr,IGMP_DEL_MAC_FILTER);

  //check if all ports is not member
  for(u8 i=0;i<igmp_group_num;i++){
	  for(j=0,port_mem=0;j<ALL_PORT_NUM;j++){
		  if(igmp_group_list[i].ports[j])
			  port_mem++;
	  }
	  if(port_mem == 0)
		  igmp_remove_group(i);
  }
  return 0;
}

/**
 * Called from ip_input() if a new IGMP packet is received.
 *
 * @param p received igmp packet, p->payload pointing to the igmp header
 * @param inp network interface on which the packet was received
 * @param dest destination ip address of the igmp packet
 */
void igmp_input(u8 inp, uip_ipaddr_t dest)
{
int num;
uip_ipaddr_t ip_tmp;
u32 rec_ip,my_ip;
uip_ipaddr_t groupaddr;

  IGMP_STATS_INC(igmp_stat.recv);

  /* Note that the length CAN be greater than 8 but only 8 are used - All are included in the checksum */
  if (uip_len < IGMP_MINLEN) {
    IGMP_STATS_INC(igmp_stat.lenerr);
    DEBUG_MSG(IGMP_DEBUG,"igmp_input: length error\r\n");
    return;
  }

  struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];


  if (m->ipchksum == uip_ipchksum()) {
    IGMP_STATS_INC(igmp_stat.chkerr);
    DEBUG_MSG(IGMP_DEBUG,"igmp_input: ip checksum error\r\n");
    return;
  }

  /* Now calculate and check the checksum */
  struct igmp_msg *igmp;
  if(m->vhl == 0x45)
	  igmp = (struct igmp_msg *)&uip_buf[UIP_IPH_LEN + UIP_LLH_LEN];
  else
	  //vhl = 0x46 - used router alert options (4 bytes)
	  igmp = (struct igmp_msg *)&uip_buf[UIP_IPH_LEN + UIP_LLH_LEN+4];



  //приходит пакет

  /* Packet is ok so find an existing group */
  //num = igmp_lookfor_group(inp, dest); /* use the destination IP address of incoming packet */

  //смотрим его тип
  switch (igmp->igmp_msgtype) {
  	  //пришёл
  	  case IGMP_MEMB_QUERY: {

		  uip_ipaddr_copy(groupaddr, igmp->igmp_group_address);

		  DEBUG_MSG(IGMP_DEBUG,"igmp_input:port %d %d.%d.%d.%d IGMP_MEMB_QUERY \r\n",
		  				  inp,uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),uip_ipaddr4(groupaddr));


  		  //нужно для выбора Qerier`a
  		  if ((uip_ipaddr_cmp(dest, allsystems)) && ((groupaddr[0]==0)&&(groupaddr[1]==0))) {

  			  DEBUG_MSG(IGMP_DEBUG,"igmp_input: GENERAL QUERY\r\n");

  			  //get src ip
  			  uip_ipaddr_copy(ip_tmp,m->srcipaddr);
  			  if((ip_tmp[0] != 0)&&(ip_tmp[1] != 0)){
				  rec_ip = ip_2_u32(ip_tmp);
				  my_ip = ip_2_u32(uip_hostaddr);

				  DEBUG_MSG(IGMP_DEBUG,"Querier: receive %lu, my ip %lu\r\n",rec_ip,my_ip);
				  //compare ip`s
				  if(my_ip < rec_ip){
					  igmp_st.state_querier = IGMP_QUERIER;
					  DEBUG_MSG(IGMP_DEBUG,"Querier\r\n");
				  }
				  else{
					  igmp_st.state_querier = IGMP_NOQUERIER;
					  igmp_st.state = IGMP_START_WAIT_REPORTS_MRT;//запускаем механизм ожидания
					  DEBUG_MSG(IGMP_DEBUG,"Non Querier\r\n");
				  }
  			  }
  		  }
  		  break;
  	  }

  	  //пришёл ответ:
  	  //1. - клиент желает присоединиться - его еще нет в списке групп
  	  //2. - пришел ответ от группы клиентов в ответ на запрос general queue
  	  case IGMP_V2_MEMB_REPORT: {

  		  //ignore if no querier
  		  if(igmp_st.state_querier != IGMP_QUERIER)
  			  break;

  		  uip_ipaddr_copy(groupaddr, igmp->igmp_group_address);

  		  DEBUG_MSG(IGMP_DEBUG,"igmp_input:port %d %d.%d.%d.%d IGMP_V2_MEMB_REPORT \r\n",
  		  				  inp,uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),uip_ipaddr4(groupaddr));

  		  IGMP_STATS_INC(igmp_stat.rx_report);


  		  //получили новый запрос
  		  //ищем вхождение в группы
  		  num = igmp_lookfor_group(inp, dest);
  		  if(num<0){
				//если группа не существует или порт не добавлен к группе
  				DEBUG_MSG(IGMP_DEBUG,"create new group\r\n");

				if(!uip_ipaddr_cmp(groupaddr,allsystems)){
					num = igmp_lookup_group(inp, groupaddr);
					if(num>=0){
						/* Allow the igmp messages at the MAC level */
						DEBUG_MSG(IGMP_DEBUG,"igmp_start: igmp_mac_filter(ADD %d.%d.%d.%d ) on if %d\r\n",
								uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),uip_ipaddr4(groupaddr),
								inp);
						igmp_mac_filter(inp, groupaddr, IGMP_ADD_MAC_FILTER);
						igmp_group_list[num].reports[inp] = 3;//при создании сразу ставим в 3
					}
				}
  		  }
  		  else if(num>=0){
  			  //группа уже существует, и если пришел report, значит он был в ответ на query
  			  //мах 3
  			  if(igmp_group_list[num].reports[inp]<3)
  				  igmp_group_list[num].reports[inp]++;
  		  }
  		break;
  	  }
  	  case IGMP_LEAVE_GROUP:{
  		  //ignore message if no querier
  		  if(igmp_st.state_querier != IGMP_QUERIER)
  			  break;
  		  uip_ipaddr_copy(groupaddr, igmp->igmp_group_address);
  		  DEBUG_MSG(IGMP_DEBUG,"igmp_input:port %d %d.%d.%d.%d IGMP_LEAVE_GROUP \r\n",
  				  inp,uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),uip_ipaddr4(groupaddr));

  		  IGMP_STATS_INC(igmp_stat.rx_leave);
  		  num = igmp_lookfor_group(inp, dest);
  		  if(num >= 0){
  			//если группа уже подключена
  			//и пришел запрос на отключение
  			igmp_group_list[num].state = IGMP_LEAVE;
  			igmp_group_list[num].leaves[inp]++;
  			DEBUG_MSG(IGMP_DEBUG,"igmp_input: leave our group\r\n");
  		  }
  		  break;
  	  }
  }
}


/**
 * Send an igmp packet to a specific group.
 *
 * @param group the group to which to send the packet
 * @param type the type of igmp packet to send
 */
static void igmp_send(u8 port, uip_ipaddr_t groupaddr, u8_t type)
{
u8 hw_port;
	struct uip_eth_hdr   *eth = (struct uip_eth_hdr *)uip_buf;
    struct uip_udpip_hdr *m = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
	struct igmp_msg 	 *igmp = (struct igmp_msg *)&uip_buf[UIP_IPH_LEN + UIP_LLH_LEN];

	if(get_igmp_query_mode() == ENABLE){
		//если разрешено посылать IGMP Query

		hw_port = L2F_port_conv(port);

		switch(type){
			case IGMP_MEMB_QUERY:
				if(uip_ipaddr_cmp(groupaddr,allsystems)){
					IGMP_STATS_INC(igmp_stat.tx_general);
					DEBUG_MSG(IGMP_DEBUG,"send General Queue\r\n");
					make_igmp_addr(eth->dest.addr,groupaddr);
					get_haddr(eth->src.addr);
	#if DSA_TAG_ENABLE
					if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
						(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
						eth->dsa_tag = HTONL(0x40000000|(hw_port<<19)|(7<<13));
					}
					else if(get_marvell_id() == DEV_98DX316){
						eth->dsa_tag = HTONL(0x41000000|(hw_port<<19)|(7<<13));
					}
	#endif
					//type
					eth->type = HTONS(UIP_ETHTYPE_IP);
					m->vhl = 0x45;
					m->tos = 0;
					m->len[0] = 0x00;
					m->len[1] = 28;
					IDENTIFICATION++;
					m->ipid[0] = (u8)(IDENTIFICATION>>8);
					m->ipid[1] = (u8)(IDENTIFICATION);
					m->ipoffset[0] = 0;
					m->ipoffset[1] = 0;
					m->ttl = 1;
					m->proto = UIP_PROTO_IGMP;
					m->ipchksum = 0;
					uip_ipaddr_copy(m->srcipaddr,uip_hostaddr);
					uip_ipaddr_copy(m->destipaddr,groupaddr);
					m->ipchksum = 0;
					m->ipchksum = ~(uip_ipchksum());
					igmp->igmp_msgtype = IGMP_MEMB_QUERY;
					igmp->igmp_maxresp = get_igmp_max_resp_time()*10;
					uip_ipaddr(igmp->igmp_group_address,0,0,0,0);
					igmp->igmp_checksum = 0;
					igmp->igmp_checksum = ~uip_chksum((u16 *)&uip_buf[UIP_IPH_LEN + UIP_LLH_LEN],IGMP_MINLEN);
					//xQueueSend(IgmpQueue,uip_buf,0);
					uip_len = 42+4;
					ETH_HandleTxPkt(uip_buf, uip_len);
				}
				else
				{
					IGMP_STATS_INC(igmp_stat.tx_query);
					DEBUG_MSG(IGMP_DEBUG,"send Specific Group Queue\r\n");
					make_igmp_addr(eth->dest.addr,groupaddr);
					get_haddr(eth->src.addr);
#if DSA_TAG_ENABLE
				if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
					(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
					eth->dsa_tag = HTONL(0x40000000|(hw_port<<19)|(7<<13));
				}
				else if(get_marvell_id() == DEV_98DX316){
					eth->dsa_tag = HTONL(0x41000000|(hw_port<<19)|(7<<13));
				}
#endif
					//type
					eth->type = HTONS(UIP_ETHTYPE_IP);
					m->vhl = 0x45;
					m->tos = 0;
					m->len[0] = 0x00;
					m->len[1] = 28;
					IDENTIFICATION++;
					m->ipid[0] = (u8)(IDENTIFICATION>>8);
					m->ipid[1] = (u8)(IDENTIFICATION);
					m->ipoffset[0] = 0;
					m->ipoffset[1] = 0;
					m->ttl = 1;
					m->proto = UIP_PROTO_IGMP;
					m->ipchksum = 0;//заполнить ниже
					uip_ipaddr_copy(m->srcipaddr,uip_hostaddr);
					uip_ipaddr_copy(m->destipaddr,groupaddr);
					m->ipchksum = 0;
					m->ipchksum = ~(uip_ipchksum());

					igmp->igmp_msgtype = IGMP_MEMB_QUERY;
					igmp->igmp_maxresp = get_igmp_max_resp_time()*10;
					uip_ipaddr_copy(igmp->igmp_group_address,groupaddr);
					igmp->igmp_checksum = 0;
					igmp->igmp_checksum = ~uip_chksum((u16 *)&uip_buf[UIP_IPH_LEN + UIP_LLH_LEN],IGMP_MINLEN);
					//xQueueSend(IgmpQueue,uip_buf,0);
					uip_len = 42+4;
					ETH_HandleTxPkt(uip_buf, uip_len);
				}
				break;
			default :
				DEBUG_MSG(IGMP_DEBUG,"undefined command");

		}
	}

}



void stats_display_igmp(void)
{
	DEBUG_MSG(IGMP_DEBUG,"IGMP Statistics\r\n");
	DEBUG_MSG(IGMP_DEBUG,"xmit: %lu\r\n", igmp_stat.xmit);
	DEBUG_MSG(IGMP_DEBUG,"recv: %lu\r\n", igmp_stat.recv);
	DEBUG_MSG(IGMP_DEBUG,"drop: %lu\r\n", igmp_stat.drop);
	DEBUG_MSG(IGMP_DEBUG,"chkerr: %lu\r\n", igmp_stat.chkerr);
	DEBUG_MSG(IGMP_DEBUG,"lenerr: %lu\r\n", igmp_stat.lenerr);
	DEBUG_MSG(IGMP_DEBUG,"memerr: %lu\r\n", igmp_stat.memerr);
	DEBUG_MSG(IGMP_DEBUG,"proterr: %lu\r\n", igmp_stat.proterr);
	DEBUG_MSG(IGMP_DEBUG,"rx_v1: %lu\r\n", igmp_stat.rx_v1);
	DEBUG_MSG(IGMP_DEBUG,"rx_group: %lu\r\n", igmp_stat.rx_group);
	DEBUG_MSG(IGMP_DEBUG,"rx_general: %lu\r\n", igmp_stat.rx_general);
	DEBUG_MSG(IGMP_DEBUG,"rx_leave: %lu\r\n", igmp_stat.rx_leave);
	DEBUG_MSG(IGMP_DEBUG,"rx_report: %lu\r\n", igmp_stat.rx_report);
	DEBUG_MSG(IGMP_DEBUG,"tx_join: %lu\r\n", igmp_stat.tx_join);
	DEBUG_MSG(IGMP_DEBUG,"tx_leave: %lu\r\n", igmp_stat.tx_leave);
	DEBUG_MSG(IGMP_DEBUG,"tx_report: %lu\r\n", igmp_stat.tx_report);
}


static void decr_reports_allport(struct igmp_group_t *group){
	for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		if(group->reports[i])
			group->reports[i]--;//decrement value of reports
	}
}


/** This function could be called to add or delete an entry in the multicast
    filter table of the ethernet MAC.*/
static u8 igmp_mac_filter(u8 netif, uip_ipaddr_t groupaddr,u8 filter){
int g_num;

u8 mac[6];
u8 ports[PORT_NUM];//={0,0,0,0,0,0};

	//make mac
	make_igmp_addr(mac,groupaddr);

	memset(ports,0,sizeof(ports));

	if(filter == IGMP_ADD_MAC_FILTER){
		DEBUG_MSG(IGMP_DEBUG,"add mac filter if %d ip %d.%d.%d.%d state = %d \r\n",netif,
				uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),
				uip_ipaddr4(groupaddr),filter);

		//add to table
		ports[netif] = 1;
		if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
			(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
			if((uip_ipaddr_cmp(groupaddr,allsystems))||(uip_ipaddr_cmp(groupaddr,allrouters)))
				sampleAddMulticastAddr(mac,ports,1);
			else
				sampleAddMulticastAddr(mac,ports,0);
		}
		else if(get_marvell_id() == DEV_98DX316){
			salsa2_mac_entry_ctrl(netif,mac,1,0,ADD_MAC_ENTRY);//todo add vlan
		}
	}
	else{
		//dell to table
		g_num = igmp_lookfor_group(netif, groupaddr);
		if(g_num>=0){
			DEBUG_MSG(IGMP_DEBUG,"del mac filter if %d ip %d.%d.%d.%d state = %d \r\n",netif,
							uip_ipaddr1(groupaddr),uip_ipaddr2(groupaddr),uip_ipaddr3(groupaddr),
							uip_ipaddr4(groupaddr),filter);

			ports[netif] = 0;
			if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
				(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
				if((uip_ipaddr_cmp(groupaddr,allsystems))||(uip_ipaddr_cmp(groupaddr,allrouters)))
					sampleAddMulticastAddr(mac,ports,1);
				else
					sampleDelMulticastAddr(mac,ports,0);
			}
			else if(get_marvell_id() == DEV_98DX316){
				//salsa2_mac_entry_ctrl(netif,mac,1,0,DEL_MAC_ENTRY);//todo add vlan
			}
		}
	}
	return 0;
}


void igmp_task_start(void){
	DEBUG_MSG(IGMP_DEBUG,"start igmp\r\n");
	if(xTaskCreate( IgmpTask, ( signed char * ) "igmp", 256, NULL, DEFAULT_PRIORITY, &xIgmpTask) == -1)
		ADD_ALARM(ERROR_CREATE_IGMP_TASK);
}


//задача IGMP
void IgmpTask(void *pvParameters){
	//struct timer general_queue_timer;
	//struct timer max_resp_timer;
	//struct timer group_membership_timer;

	struct igmp_group_t *group;

	//uip_ipaddr_t specaddr;
	u8 port,i,j;

	//start
	//set QueryInterval
	//timer_set(&general_queue_timer, get_igmp_query_int()*MSEC*1000);
	//timer_set(&max_resp_timer, get_igmp_max_resp_time()*MSEC*1000);
	//timer_set(&group_membership_timer, get_igmp_group_membership_time()*MSEC*1000);


	//inital state
	//мы querier, и отправляем general query
	igmp->state_querier = IGMP_QUERIER;
	igmp->state = IGMP_IDLE;

	//send general query on all ports
	/*for(port = 0;port<(COOPER_PORT_NUM+FIBER_PORT_NUM);port++){
		igmp_send(port,allsystems,IGMP_MEMB_QUERY);
	}

	//wait Max resp time
	vTaskDelay(get_igmp_max_resp_time()*1000*MSEC);
	//
*/



	while(1){

		if(uip_hostaddr[0]==0 && uip_hostaddr[1] == 0){
			vTaskDelay(1000*MSEC);
			continue;
		}

		//igmp snooping querier processing
		if(igmp->state_querier == IGMP_QUERIER){
			//управление глобальное
			switch(igmp->state){
				case IGMP_IDLE:
					igmp->state = IGMP_SEND_GEN_QUERY;
					break;

				case IGMP_SEND_GEN_QUERY:
					DEBUG_MSG(IGMP_DEBUG,"IGMP Send General Query\r\n");
					for(i=0;i<igmp_group_num;i++){
						group = &igmp_group_list[i];
						decr_reports_allport(group);
					}
					//send general query on all ports
					for(port = 0;port<(COOPER_PORT_NUM+FIBER_PORT_NUM);port++){
						igmp_send(port,allsystems,IGMP_MEMB_QUERY);
					}
					igmp->state = IGMP_START_WAIT_REPORTS_MRT;
					break;

				case IGMP_START_WAIT_REPORTS_MRT:
					igmp->state = IGMP_WAIT_REPORTS_MRT;
					timer_set(&igmp->timer_mrt, get_igmp_max_resp_time()*MSEC*1000);
					break;

				case IGMP_WAIT_REPORTS_MRT:
					//ждем max response time
					//wait if timer exp
					if(timer_expired(&igmp->timer_mrt)){
						//if no reports on port, delete port
						for(i=0;i<igmp_group_num;i++){
							for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
								//for all ports
								if(igmp_group_list[i].ports[j]){
									if(igmp_group_list[i].reports[j] == 0){
										igmp_remove_port(i,j);
									}
								}
								//если в группе больше не осталось портов, то группа удаляется
							}
						}
						//переходим в цикл ожидания
						igmp->state = IGMP_START_WAIT_QI;
					}
					break;

				case IGMP_START_WAIT_QI:
					igmp->state = IGMP_WAIT_QI;
					timer_set(&igmp->timer_qi, get_igmp_query_int()*MSEC*1000);
					break;

				case IGMP_WAIT_QI:
					if(timer_expired(&igmp->timer_qi)){
						DEBUG_MSG(IGMP_DEBUG,"QueryInterval timer expired\r\n");
						igmp->state = IGMP_IDLE;
					}
					break;
			}

			//управление по группам // запросы на удаление
			for(i=0;i<igmp_group_num;i++){
				group = &igmp_group_list[i];
				switch(group->state){
					case IGMP_IDLE:
						break;

					case IGMP_LEAVE:
						for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
							if(group->leaves[j]){
								group->leaves[j] = 0;
								group->reports[j] = 0;

								group->state = IGMP_SEND_GSPEC_QUERY;
								DEBUG_MSG(IGMP_DEBUG,"Send Specific Group Query\r\n");
								//send specific query on all ports // 2 sends
								for(port = 0;port<(COOPER_PORT_NUM+FIBER_PORT_NUM);port++){
									if(group->ports[port])
										igmp_send(port,group->group_addr,IGMP_MEMB_QUERY);
								}
								for(port = 0;port<(COOPER_PORT_NUM+FIBER_PORT_NUM);port++){
									if(group->ports[port])
										igmp_send(port,group->group_addr,IGMP_MEMB_QUERY);
								}
								timer_set(&group->timer, get_igmp_group_membership_time()*MSEC*1000);
								group->state = IGMP_WAIT_REPORTS_GS;
							}
							break;
						}

					case IGMP_WAIT_REPORTS_GS:
						if(timer_expired(&group->timer)){
							DEBUG_MSG(IGMP_DEBUG,"IGMP_WAIT_REPORTS_GS timer_expired\r\n");
							for(j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
								//if no reports - no hosts on group - del group
								if(group->reports[j] == 0){
									igmp_remove_port(i,j);
								}
							}
							group->state = IGMP_IDLE;
						}
						break;
				}
			}

		}
		else if(igmp->state_querier == IGMP_NOQUERIER){
			//igmp snooping noquerier processing
			switch(igmp->state){
				case IGMP_IDLE:
					igmp->state = IGMP_SEND_GEN_QUERY;
					break;
				case IGMP_SEND_GEN_QUERY:
					//send general query on all ports
					for(port = 0;port<(COOPER_PORT_NUM+FIBER_PORT_NUM);port++){
						igmp_send(port,allsystems,IGMP_MEMB_QUERY);
					}
					igmp->state = IGMP_START_WAIT_REPORTS_MRT;
					break;

				case IGMP_START_WAIT_REPORTS_MRT:
					igmp->state = IGMP_WAIT_REPORTS_MRT;
					timer_set(&igmp->timer_qi, get_igmp_max_resp_time()*MSEC*1000);
					break;

				case IGMP_WAIT_REPORTS_MRT:
					//ждем max response time
					//если нет ответа запускаем таймер other querier interval
					//wait if timer exp
					if(timer_expired(&igmp->timer_qi)){
						igmp->state = IGMP_WAIT_QPI;
						timer_set(&igmp->timer_qpi, get_igmp_other_querier_time()*MSEC*1000);
					}
					break;


				case IGMP_WAIT_QPI:
					//wait if timer exp
					if(timer_expired(&igmp->timer_qpi)){
						//если таймер истёк, и мы здесь, значит не было ответа general query (в функции igmp_input)
						//то мы считаем себя querier`om
						igmp->state_querier = IGMP_QUERIER;
						igmp->state = IGMP_IDLE;
					}
					break;
			}
		}
		vTaskDelay(1000*MSEC);
	}
	vTaskDelete(NULL);
}

#endif

