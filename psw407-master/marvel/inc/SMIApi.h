
#ifndef SMIAPI_H_
#define SMIAPI_H_

#include "stm32f4xx.h"
#include "msApiDefs.h"

#define ADD_MAC_ENTRY		0
#define DEL_MAC_ENTRY		1

#define FORCED100M_MASK1 0x0001
#define FORCED100M_MASK2 0xFFFD

#define ENABLE_VCT_MASK 0x8000
#define RESULT_VCT_MASK 0x6000


/*port stat types*/
#define RX_GOOD 0x00
#define RX_BAD 0x02
#define TX_GOOD 0x0E
#define COLLISION 0x1E
#define STAT_INIT 0xFF
//unicast
#define RX_UNICAST	0x04
#define TX_UNICAST	0x10
//broadcast
#define RX_BROADCAST	0x06
#define TX_BROADCAST	0x13
//multicast
#define RX_MULTICAST	0x07
#define TX_MULTICAST	0x12

#define TX_FCSERR		0x03
#define DEFERRED		0x05
#define EXCESSIVE		0x11
#define SINGLE			0x14
#define TX_PAUSE		0x15
#define RX_PAUSE		0x16
#define MULTIPLE		0x17
#define RX_UNDERSIZE	0x18
#define RX_FARGMENTS	0x19
#define RX_OVERSIZE		0x1A
#define RX_JABBER		0x1B
#define RX_RXERR		0x1C
#define RX_FCSERR		0x1D
#define LATE			0x1F


//for salsa
#define RX_GOOD_FRAMES	100
#define TX_GOOD_FRAMES	101
#define TX_MAC_ERROR	102

//status 4 VCT
#define VCT_BAD 	0
#define VCT_SHORT 	1
#define VCT_OPEN	2
#define VCT_GOOD	3
//for swu
#define VCT_SAME_PAIR_SHORT 	4
#define VCT_CROSS_PAIR_SHORT 	5
#define VCT_PAIR_BUSY		 	6

#define VCT_TEST		1
#define VCT_CALLIBRATE	2

#define MAX_CPU_ENTRIES_NUM		50
struct mac_entry_t{
	u8 mac[6];
	u32 port_vect;
	u16 vid;
	u8 valid:1;
	u8 is_trunk:1;
};

u8 net_port_sett_change(void);
void net_port_sett_change_set(u8 state);

u8 set_cable_test(u8 state,u8 port,u8 length);
u8 get_cable_test(void);
u8 get_vct_port(void);
void vct_processing(void);

uint32_t simpleCableTest(uint8_t port);
uint8_t simplePktGen(uint8_t Port);
uint8_t PHYLoopControl(uint8_t Port,uint8_t State);
uint32_t InDiscardsFrameCount(uint8_t Port);
uint16_t InFilteredFrameCount(uint8_t Port);
uint16_t OutFilteredFrameCount(uint8_t Port);
uint8_t gprtGetPortCtr2(uint8_t port,GT_PORT_STAT2 *ctr);
uint8_t PortStateInfo(uint8_t Port);
uint8_t PortDuplexInfo(uint8_t Port);
uint8_t PortSpeedInfo(uint8_t Port);
uint8_t PortFlowControlInfo(uint8_t Port);
//uint8_t PortEnableDisableSet(uint8_t Port,uint8_t State);
//uint8_t PortForcedSpeedSet(uint8_t Port,uint8_t Speed);
//uint8_t PortDuplexSet(uint8_t Port,uint8_t State);
//uint8_t PortForcedLinkSet(uint8_t Port,uint8_t State);
//uint8_t PortForcedFlowControlSet(uint8_t Port,uint8_t State);
void LoadDefault(void);
//uint8_t PortLinkLEDSet(uint8_t Port,uint8_t State);
void SwitchPortSet(void);
int L2F_port_conv(uint8_t port);
int F2L_port_conv(uint8_t f_port);
uint8_t marvell_freeze_control(void);



void smi_allport_discard(void);
int read_atu(int num, int start, struct mac_entry_t *entry);
u32 read_atu_agetime(void);
u8 get_salsa2_fdb_entry(u16 num, struct mac_entry_t *entry);
//void read_all_atu(void);
u32 get_atu_port_vect(u8 *mac_s);
u32 read_stats_cnt(u8 port, u8 type);
void smi_set_port_dbnum_low(int port, int num);
void smi_flush_db(int dbnum);
void smi_flush_all(void);
u16 get_marvell_id(void);

void ppu_disable(void);
void wait_mv_ready(void);

void marvell_int_cfg(void);
void ETH_LINK_int_clear(void);

void port_statistics_processing(void);

uint16_t ETH_ReadIndirectPHYReg(uint16_t PHYAddress,uint16_t PHYPage, uint16_t PHYReg);
void ETH_WriteIndirectPHYReg(uint16_t PHYAddress,uint16_t PHYPage, uint16_t PHYReg, uint16_t Data);

GT_STATUS hwReadGlobalReg( IN  GT_U8    regAddr,  OUT GT_U16   *data);
GT_STATUS hwWriteGlobalReg(IN  GT_U8    regAddr,   IN  GT_U16   data);
GT_STATUS hwReadGlobal2Reg( IN  GT_U8    regAddr,  OUT GT_U16   *data);
GT_STATUS hwWriteGlobal2Reg(IN  GT_U8    regAddr,   IN  GT_U16   data);

GT_STATUS hwGetGlobal2RegField(IN GT_U8 regAddr,IN GT_U8 fieldOffset,IN GT_U8 fieldLength,OUT GT_U16 *data);

void * gtMemSet(void * start,int    symbol,GT_U32 size);

GT_STATUS gfdbGetAtuEntryNext(GT_ATU_ENTRY  *atuEntry);
GT_STATUS gfdbGetAtuEntryFirst(GT_ATU_ENTRY    *atuEntry);
GT_STATUS atuStateDevToApp(GT_BOOL unicast,GT_U32 state,GT_U32 *newOne);
GT_STATUS statsOperationPerform(GT_STATS_OPERATION statsOp,GT_U8 port,GT_STATS_COUNTERS counter, GT_VOID *statsData);
GT_STATUS statsCapture(GT_U8 port);
GT_STATUS statsReadCounter(GT_U32 counter,GT_U32 *statsData);
GT_STATUS statsReadRealtimeCounter(GT_U8  port,GT_U32 counter,GT_U32 *statsData);



u8 Salsa2_WriteReg(u32 addr,u32 data);
u32 Salsa2_ReadReg(u32 addr);
u8 Salsa2_WriteRegField(u32 addr,u8 offset, u32 data,u8 len);
u32 Salsa2_ReadRegField(u32 addr,u8 offset,u8 len);
u16 Salsa2_ReadPhyReg(u8 port,u8 PhyAddr, u8 PhyReg);
void Salsa2_WritePhyReg(u8 port,u8 PhyAddr, u8 PhyReg, u16 data);


void Salsa2_configPhyAddres(u8 port,u32 addr);
void SWU_start_process(void);

void set_mac_filtering(void);
GT_STATUS AddMacAddr(u8 *mac, u8 hw_port);
GT_STATUS DelMacAddr(u8 *mac, u8 hw_port);

void mac_filtring_processing(void);
void add_blocked_mac(u8 *mac,u8 port);
void del_blocked_mac(u8 *mac,u8 port);
#endif /* SMIAPI_H_ */
