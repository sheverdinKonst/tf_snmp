#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_pwr.h"
#include "board.h"
#include "selftest.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_flash.h"
#include "misc.h"
#include "poe_ltc.h"
#include "i2c_hard.h"
#include "i2c_soft.h"
#include "names.h"
#include "eeprom.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../uip/uip.h"
#include "../stp/bridgestp.h"
#include "SMIApi.h"
#include "VLAN.h"
#include "../deffines.h"
#include "rc4.h"
#include "crc.h"
#include "spiflash.h"
#include "blbx.h"
#include "../events/events_handler.h"
#include "../snmp/snmp.h"
#include "../snmp/snmpd/snmpd-types.h"
#include "../smtp/smtp.h"
#include "../igmp/igmpv2.h"
#include "../dns/resolv.h"
#include "../tftp/tftpclient.h"
#include "../uip/timer.h"
#include "../inc/h/driver/gtDrvSwRegs.h"
#include "UPS_command.h"
#include "../webserver/httpd-cgi.h"
#include "settingsfile.h"
#include "../telnet/usb_shell.h"
#include "../command/command.h"
#include "Salsa2Regs.h"
#include "../plc/plc_def.h"
#include "../eth/stm32f4x7_eth.h"


#define SETT_BAD	-1//не корректные настройки
#define SETT_EQUAL	0//настройки не изменились
#define SETT_DIFF	1//настройки изменились

//группа set
int set_net_ip(uip_ipaddr_t ip);
int set_net_mask(uip_ipaddr_t mask);
int set_net_gate(uip_ipaddr_t gate);
int set_net_dns(uip_ipaddr_t dns);
int set_net_mac(u8 *mac);
int set_net_def_mac(u8 *mac);
int set_dhcp_mode(u8 mode);
int set_dhcp_server_addr(uip_ipaddr_t serv);
int set_dhcp_hops(u8 hops);
int set_dhcp_opt82(u8 state);
int set_gratuitous_arp_state(u8 state);
int set_port_state(u8 port,u8 state);
int set_port_speed_dplx(u8 port,u8 state);
int set_port_flow(u8 port,u8 state);
int set_port_wdt(u8 port,u8 state);
int set_port_wdt_ip(u8 port,uip_ipaddr_t ip);
int set_port_wdt_speed_up(u8 port,u16 speed);
int set_port_wdt_speed_down(u8 port,u16 speed);
int set_port_soft_start(u8 port, u8 state);
int set_port_poe(u8 port,u8 state);
int set_port_poe_b(u8 port,u8 state);
int set_port_pwr_lim_a(u8 port, u8 limit);
int set_port_pwr_lim_b(u8 port, u8 limit);
int set_port_sfp_mode(u8 port, u8 mode);
int set_interface_lang(u8 lang);
//int set_interface_login(char *paswd);
//int set_interface_passwd(char *paswd);
int set_interface_users_username(u8 num,char *login);
int set_interface_users_password(u8 num,char *paswd);
int set_interface_users_rule(u8 num,u8 rules);
void set_current_username(char *login);

int set_interface_period(u8 period);
int set_interface_name(char *text);
int set_interface_location(char *text);
int set_interface_contact(char *text);
int set_port_descr(u8 port, char *text);
int set_ext_vendor_name(char *name);
int set_ext_vendor_name_flag(u8 state);
int set_smtp_state(u8 state);
int set_smtp_server(uip_ipaddr_t ip);
int set_smtp_to(char *to);
int set_smtp_to2(char *to);
int set_smtp_to3(char *to);
int set_smtp_from(char *to);
int set_smtp_subj(char *subj);
int set_smtp_login(char *login);
int set_smtp_pass(char *pass);
int set_smtp_port(u16 port);
int set_smtp_domain(char *domain);
int set_sntp_state(u8 state);
int set_sntp_serv(uip_ipaddr_t ip);
int set_sntp_serv_name(char *name);
int set_sntp_timezone(i8 timezone);
int set_sntp_period(u8 period);
int set_syslog_state(u8 state);
int set_syslog_serv(uip_ipaddr_t ip);
int set_event_base_s(u8 state,u8 level);
int set_event_port_s(u8 state,u8 level);
int set_event_vlan_s(u8 state,u8 level);
int set_event_stp_s(u8 state,u8 level);
int set_event_qos_s(u8 state,u8 level);
int set_event_other_s(u8 state,u8 level);
int set_event_port_link_t(u8 state,u8 level);
int set_event_port_poe_t(u8 state,u8 level);
int set_event_stp_t(u8 state,u8 level);
int set_event_spec_link_t(u8 state,u8 level);
int set_event_spec_ping_t(u8 state,u8 level);
int set_event_spec_speed_t(u8 state,u8 level);
int set_event_system_t(u8 state,u8 level);
int set_event_ups_t(u8 state,u8 level);
int set_event_alarm_t(u8 state,u8 level);
int set_event_mac_t(u8 state,u8 level);
int set_alarm_state(u8 num,u8 state);
int set_alarm_front(u8 num,u8 front);
int set_rate_limit_mode(u8 port,u8 mode);
int set_uc_rate_limit(u8 state);
int set_mc_rate_limit(u8 state);
int set_bc_rate_limit(u8 state);
int set_bc_limit(u8 limit);
int set_rate_limit_rx(u8 port,u32 limit);
int set_rate_limit_tx(u8 port,u32 limit);
int set_qos_port_cos_state(u8 port,u8 state);
int set_qos_port_tos_state(u8 port,u8 state);
int set_qos_port_rule(u8 port,u8 state);
int set_qos_port_def_pri(u8 port,u8 pri);
int set_qos_state(u8 state);
int set_qos_policy(u8 policy);
int set_qos_cos(u8 cos1,u8 queue);
int set_qos_tos(u8 tos,u8 queue);
int set_igmp_snooping_state(u8 state);
int set_igmp_query_mode(u8 mode);
int set_igmp_port_state(u8 port, u8 state);
int set_igmp_query_int(u8 time);
int set_igmp_max_resp_time(u8 time);
int set_igmp_group_membership_time(u8 time);
int set_igmp_other_querier_time(u8 time);



/*
int set_igmp_group_num(u8 num);
int set_igmp_group_list_active(u8 num,u8 state);
int set_igmp_group_list_ip(u8 num,uip_ipaddr_t ip);
int set_igmp_group_list_port(u8 num,u8 port_num,u8 state);
int set_igmp_group_delete(u8 num);
int set_igmp_group_add(uip_ipaddr_t ip,u8 *port_state);
*/
int set_pb_vlan_state(u8 state);
int set_pb_vlan_port(u8 rx_port,u8 tx_port,u8 state);
int set_pb_vlan_table(struct pb_vlan_t *pb);
int set_pb_vlan_swu_port(u8 port,u8 vid);
int set_vlan_sett_state(u8 state);
int set_vlan_trunk_state(u8 state);
int set_vlan_sett_mngt(u16 vid);
int set_vlan_sett_port_state(u8 port, u8 state);
int set_vlan_sett_dvid(u8 port, u16 dvid);
int set_vlan_sett_vlannum(u16 num);
int delete_vlan(u16 vid);
int set_vlan_state(u8 num,u8 state);
int set_vlan_vid(u8 num,u16 vid);
int set_vlan_name(u8 num, char *name);
int set_vlan_port(u8 num,u8 port,u8 state);
int set_stp_state(u8 state);
int set_stp_magic(u16 magic);
int set_stp_proto(u8 proto);
int set_stp_bridge_priority(u16 priority);
int set_stp_bridge_max_age(u8 mage);
int set_stp_bridge_htime(u8 htime);
int set_stp_bridge_fdelay(u8 delay);
int set_stp_bridge_mdelay(u8 delay);
int set_stp_txholdcount(u8 hcount);
int set_stp_port_enable(u8 port,u8 en);
int set_stp_port_state(u8 port,u8 state);
int set_stp_port_priority(u8 port,u8 priority);
int set_stp_port_cost(u8 port,u32 cost);
int set_stp_port_autocost(u8 port,u8 flag);
int set_stp_port_autoedge(u8 port,u8 flag);
int set_stp_port_edge(u8 port,u8 flag);
int set_stp_port_autoptp(u8 port,u8 flag);
int set_stp_port_ptp(u8 port,u8 flag);
int set_stp_port_flags(u8 port,u8 flag);
int set_stp_bpdu_fw(u8 state);
int set_callibrate_koef_1(u8 port, u16 koef);
int set_callibrate_koef_2(u8 port, u16 koef);
int set_callibrate_len(u8 port, u8 len);

int set_snmp_state(u8 state);
int set_snmp_mode(u8 mode);
int set_snmp_serv(uip_ipaddr_t addr);
int set_snmp_vers(u8 vers);
//int set_snmp_communitie(char *comm);
int set_snmp1_read_communitie(char *comm);
int set_snmp1_write_communitie(char *comm);
int set_snmp3_level(u8 usernum, u8 level);
int set_snmp3_user_name(u8 usernum, char *str);
int set_snmp3_auth_pass(u8 usernum, char *str);
int set_snmp3_priv_pass(u8 usernum, char *str);
int set_snmp3_engine_id(engine_id_t *eid);


int set_softstart_time(u16 time);
int set_telnet_state(u8 state);
int set_telnet_echo(u8 state);
int set_telnet_rn(u8 mode);
int set_tftp_state(u8 state);
int set_tftp_mode(u8 mode);
int set_tftp_port(u16 port);
int set_downshifting_mode(u8 state);

int set_plc_out_state(u8 channel,u8 state);
int set_plc_out_action(u8 channel, u8 action);
int set_plc_out_event(u8 channel, u8 event,u8 state);
int set_plc_in_state(u8 channel, u8 state);
int set_plc_in_alarm_state(u8 channel, u8 state);
int set_plc_em_model(u16 model);
int set_plc_em_rate(u8 rate);
int set_plc_em_parity(u8 parity);
int set_plc_em_databits(u8 bits);
int set_plc_em_stopbits(u8 bits);
int set_plc_em_pass(char *pass);
int set_plc_em_id(char *id);
int set_mac_filter_state(u8 port,u8 state);
int set_mac_learn_cpu(u8 state);
int set_mac_bind_entry_mac(u8 entry,u8 *mac);
int set_mac_bind_entry_port(u8 entry, u8 port);
int set_mac_bind_entry_active(u8 entry,u8 flag);
int add_mac_bind_entry(u8 *mac,u8 port);
int del_mac_bind_entry(u8 entry_num);
int set_ups_delayed_start(u8 state);

int set_lag_valid(u8 index,u8 state);
int set_lag_state(u8 index,u8 state);
int set_lag_master_port(u8 index,u8 port);
int set_lag_port(u8 index,u8 port,u8 state);
int del_lag_entry(u8 index);

int set_mirror_state(u8 state);
int set_mirror_target_port(u8 port);
int set_mirror_port(u8 port,u8 state);


int set_input_state(u8 channel, u8 state);
int set_input_name(u8 input,char *name);
int set_input_remdev(u8 input, u8 devnum);
int set_input_remport(u8 input, u8 port);
int set_input_inverse(u8 input, u8 inv_state);

int set_tlp_event_state(u8 num, u8 state);
int set_tlp_event_remdev(u8 num, u8 devnum);
int set_tlp_event_remport(u8 num, u8 port);
int set_tlp_event_inverse(u8 num, u8 inv_state);

int set_tlp_remdev_valid(u8 num,u8 valid);
int set_tlp_remdev_ip(u8 num,uip_ipaddr_t *ip);
int set_tlp_remdev_mask(u8 num,uip_ipaddr_t *mask);
int set_tlp_remdev_gate(u8 num,uip_ipaddr_t *gate);
int set_tlp_remdev_name(u8 num,char *name);
int set_tlp_remdev_type(u8 num,u8 type);
int delete_tlp_remdev(u8 num);

int set_lldp_state(u8 state);
int set_lldp_transmit_interval(u8 ti);
int set_lldp_hold_multiplier(u8 hm);
int set_lldp_port_state(u8 port,u8 state);



//группа get относится к настройкам
void get_net_ip(uip_ipaddr_t *ip);
void get_net_mask(uip_ipaddr_t *ip);
void get_net_gate(uip_ipaddr_t *ip);
void get_net_dns(uip_ipaddr_t *ip);
void get_net_mac(u8 *mac);
void get_net_def_mac(u8 *mac);
u8 get_dhcp_mode(void);
void get_dhcp_server_addr(uip_ipaddr_t *ip);
u8 get_dhcp_hops(void);
u8 get_dhcp_opt82(void);
u8 get_dhcpr_state(void);
u8 get_dhcpr_state(void);
u8 get_dhcpr_hops(void);
u8 get_dhcpr_opt(void);

u8 get_gratuitous_arp_state(void);
u8 get_port_sett_state(u8 port);
u8 get_port_sett_speed_dplx(u8 port);
u8 get_port_sett_flow(u8 port);
u8 get_port_sett_wdt(u8 port);
void get_port_sett_wdt_ip(u8 port,uip_ipaddr_t *ip);
u16 get_port_sett_wdt_speed_up(u8 port);
u16 get_port_sett_wdt_speed_down(u8 port);
u8 get_port_sett_soft_start(u8 port);
u8 get_port_sett_poe(u8 port);
u8 get_port_sett_poe_b(u8 port);
u8 get_port_sett_pwr_lim_a(u8 port);
u8 get_port_sett_pwr_lim_b(u8 port);
u8 get_port_sett_sfp_mode(u8 port);
u8 get_interface_lang(void);
//void get_interface_login(char *str);
//void get_interface_passwd(char *str);
void get_interface_users_username(u8 num,char *str);
void get_interface_users_password(u8 num,char *str);
u8 get_interface_users_rule(u8 num);

u8 get_interface_period(void);
void get_interface_name(char *str);
void get_interface_location(char *str);
void get_interface_contact(char *str);
void get_port_descr(u8 port,char *str);
void get_ext_vendor_name(char *str);
u8 ext_vendor_name_flag(void);
u8 get_smtp_state(void);
void get_smtp_server(uip_ipaddr_t *ip);
void get_smtp_to(char *str);
void get_smtp_to2(char *str);
void get_smtp_to3(char *str);
void get_smtp_from(char *str);
void get_smtp_subj(char *str);
void get_smtp_login(char *str);
void get_smtp_pass(char *str);
u16 get_smtp_port(void);
void get_smtp_domain(char *str);
u8 get_sntp_state(void);
void get_sntp_serv(uip_ipaddr_t *ip);
void get_sntp_serv_name(char *name);
i8 get_sntp_timezone(void);
u8 get_sntp_period(void);
u8 get_syslog_state(void);
//u16 *get_syslog_serv(void);
void get_syslog_serv(uip_ipaddr_t *ip);
u8 get_event_base_s_st(void);
u8 get_event_base_s_level(void);
u8 get_event_port_s_st(void);
u8 get_event_port_s_level(void);
u8 get_event_vlan_s_st(void);
u8 get_event_vlan_s_level(void);
u8 get_event_stp_s_st(void);
u8 get_event_stp_s_level(void);
u8 get_event_qos_s_st(void);
u8 get_event_qos_s_level(void);
u8 get_event_other_s_st(void);
u8 get_event_other_s_level(void);
u8 get_event_port_link_t_st(void);
u8 get_event_port_link_t_level(void);
u8 get_event_port_poe_t_st(void);
u8 get_event_port_poe_t_level(void);
u8 get_event_port_stp_t_st(void);
u8 get_event_port_stp_t_level(void);
u8 get_event_port_spec_link_t_st(void);
u8 get_event_port_spec_link_t_level(void);
u8 get_event_port_spec_ping_t_st(void);
u8 get_event_port_spec_ping_t_level(void);
u8 get_event_port_spec_speed_t_st(void);
u8 get_event_port_spec_speed_t_level(void);
u8 get_event_port_system_t_st(void);
u8 get_event_port_system_t_level(void);
u8 get_event_port_ups_t_st(void);
u8 get_event_port_ups_t_level(void);
u8 get_event_port_alarm_t_st(void);
u8 get_event_port_alarm_t_level(void);
u8 get_event_port_mac_t_st(void);
u8 get_event_port_mac_t_level(void);

u8 get_alarm_state(u8 num);
u8 get_alarm_front(u8 num);
u8 get_rate_limit_mode(u8 port);
u8 get_uc_rate_limit(void);
u8 get_mc_rate_limit(void);
u8 get_bc_rate_limit(void);
u8 get_bc_limit(void);
u32 get_rate_limit_rx(u8 port);
u32 get_rate_limit_tx(u8 port);
u8 get_qos_port_cos_state(u8 port);
u8 get_qos_port_tos_state(u8 port);
u8 get_qos_port_rule(u8 port);
u8 get_qos_port_def_pri(u8 port);
u8 get_qos_state(void);
u8 get_qos_policy(void);
u8 get_qos_cos_queue(u8 cos1);
u8 get_qos_tos_queue(u8 tos);

u8 get_igmp_snooping_state(void);
u8 get_igmp_query_mode(void);
u8 get_igmp_port_state(u8 port);
u16 get_igmp_query_int(void);
u8 get_igmp_max_resp_time(void);
u8 get_igmp_group_membership_time(void);
u16 get_igmp_other_querier_time(void);

/*
 *
u8 get_igmp_group_num(void);
u8 get_igmp_group_list_active(u8 num);
void get_igmp_group_list_ip(u8 num,uip_ipaddr_t *ip);
u8 get_igmp_group_list_port(u8 num,u8 port_num);
*/
u8 get_pb_vlan_state(void);
u8 get_pb_vlan_port(u8 rx_port, u8 tx_port);
void get_pb_vlan_table(struct pb_vlan_t *pb);

u8 get_pb_vlan_swu_port(u8 port);
u8 get_vlan_sett_state(void);
u16 get_vlan_sett_mngt(void);
u8 get_vlan_trunk_state(void);
u8 get_vlan_sett_port_state(u8 port);
u16 get_vlan_sett_dvid(u8 port);
u16 get_vlan_sett_vlannum(void);
u8 get_vlan_state(u8 num);
u16 get_vlan_vid(u8 num);
int vlan_vid2num(u16 vid);
char *get_vlan_name(u8 num);
u8 get_vlan_port_state(u8 num,u8 port);
u8 get_stp_state(void);
u16 get_stp_magic(void);
u8 get_stp_proto(void);
u16 get_stp_bridge_priority(void);
u8 get_stp_bridge_max_age(void);
u8 get_stp_bridge_htime(void);
u8 get_stp_bridge_fdelay(void);
u8 get_stp_bridge_mdelay(void);
u8 get_stp_txholdcount(void);
u8 get_stp_port_enable(u8 port);
u8 get_stp_port_state(u8 port);
u8 get_stp_port_priority(u8 port);
u32 get_stp_port_cost(u8 port);
u8 get_stp_port_autocost(u8 port);
u8 get_stp_port_autoedge(u8 port);
u8 get_stp_port_edge(u8 port);
u8 get_stp_port_autoptp(u8 port);
u8 get_stp_port_ptp(u8 port);
u8 get_stp_port_flags(u8 port);
u8 get_stp_bpdu_fw(void);

u16 get_callibrate_koef_1(u8 port);
u16 get_callibrate_koef_2(u8 port);
u8 get_callibrate_len(u8 port);

u8 get_snmp_state(void);
u8 get_snmp_mode(void);
void get_snmp_serv(uip_ipaddr_t *ip);
u8 get_snmp_vers(void);
//void get_snmp_communitie(char *str);
void get_snmp1_read_communitie(char *str);
void get_snmp1_write_communitie(char *str);
int get_snmp3_level(u8 usernum);
int get_snmp3_user_name(u8 usernum, char *str);
int get_snmp3_auth_pass(u8 usernum, char *str);
int get_snmp3_priv_pass(u8 usernum, char *str);
int get_snmp3_engine_id(engine_id_t *eid);

u16 get_softstart_time(void);
u8 get_telnet_state(void);
u8 get_telnet_echo(void);
u8 get_telnet_rn(void);
u8 get_tftp_state(void);
u8 get_tftp_mode(void);
u16 get_tftp_port(void);

u8 get_downshifting_mode(void);

u8 get_plc_out_state(u8 channel);
u8 get_plc_out_action(u8 channel);
u8 get_plc_out_event(u8 channel, u8 event);
u8 get_plc_in_state(u8 channel);
u8 get_plc_in_alarm_state(u8 channel);
u16 get_plc_em_model(void);
u8 get_plc_em_rate(void);
u8 get_plc_em_parity(void);
u8 get_plc_em_databits(void);
u8 get_plc_em_stopbits(void);
void get_plc_em_pass(char *pass);
void get_plc_em_id(char *id);
u8 get_mac_filter_state(u8 port);
u8 get_mac_learn_cpu(void);
u8 get_mac_bind_num(void);
u8 get_mac_bind_entry_mac(u8 entry,u8 position);
u8 get_mac_bind_entry_port(u8 entry);
u8 get_mac_bind_entry_active(u8 entry);
u8 get_ups_delayed_start(void);

u8 get_lag_id(u8 index);
u8 get_lag_valid(u8 index);
u8 get_lag_state(u8 index);
u8 get_lag_master_port(u8 index);
u8 get_lag_port(u8 index,u8 port);
u8 get_lag_entries_num(void);

u8 get_mirror_state(void);
u8 get_mirror_target_port(void);
u8 get_mirror_port(u8 port);

u8 get_input_state(u8 num);
u8 get_input_rem_dev(u8 num);
u8 get_input_rem_port(u8 num);
u8 get_input_inverse(u8 input);
u8 get_tlp_event_state(u8 num);
u8 get_tlp_event_rem_dev(u8 num);
u8 get_tlp_event_rem_port(u8 num);
u8 get_tlp_event_inverse(u8 num);
u8 get_tlp_remdev_valid(u8 num);
void get_tlp_remdev_ip(u8 num,uip_ipaddr_t *ip);
void get_tlp_remdev_mask(u8 num,uip_ipaddr_t *mask);
void get_tlp_remdev_mask_default(uip_ipaddr_t *mask);
void get_tlp_remdev_gate(u8 num,uip_ipaddr_t *gate);
void get_tlp_remdev_gate_default(uip_ipaddr_t *gate);
void get_tlp_remdev_name(u8 num,char *name);
u8 get_tlp_remdev_type(u8 num);
u8 get_tlp_remdev_last(void);
u8 get_remdev_num(void);
u8 get_mv_freeze_ctrl_state(void);

u8 get_lldp_state(void);
u8 get_lldp_transmit_interval(void);
u8 get_lldp_hold_multiplier();
u8 get_lldp_port_state(u8 port);




int settings_save(void);
int settings_load(void);



u8 need_load_settings(void);
void need_load_settings_set(u8 state);


u8 need_save_settings(void);
void need_save_settings_set(u8 state);


void settings_default(u8 flag);

int settings_struct_initialization(u8 flag);

//ret 1 if settings is loaded
u8 settings_is_loaded(void);
//set settings loaded flag
void settings_loaded(u8 state);


#endif /* SETTINGS_H_ */
