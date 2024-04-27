#ifndef SALSA2_H_
#define SALSA2_H_

void salsa2_config_processing(void);
void config_cpu_port(void);
u8 Salsa2_Read_Counter(u8 type,u8 port,u32 *ret);
u8 set_vlan_entry(u16 vid,u32 entry,u8 valid);
void set_vlan_port_state(u8 vnum,u8 port,u8 state);
void set_vlan_igmp_mode(u8 vnum,u8 state);
void set_vlan_stp_ptr(u8 vnum,u8 ptr);
void set_vlan_mgmt_mode(u8 vnum,u8 state);
void set_vlan_valid(u8 vnum,u8 state);
void set_cpu_port_vid(u16 vid);
void set_port_default_vid(u8 port,u16 dvid);
u8 Salsa2_get_phyAddr(u8 port);
void SWU_link_aggregation_config(void);
void SWU_port_mirroring_config(void);
void set_swu_test_vlan(u8 state);
int salsa2_mac_entry_ctrl(u8 port,u8 *mac, u16 vid, u8 trunk, u8 type);
void set_link_indication_mode(u8 mode);
void salsa2_dump(void);
void set_swu_port_loopback(u8 port,u8 state);
u8 get_port_phy_addr(u8 port);

#endif /* SALSA2_H_ */
