#ifndef ICMP_H_
#define ICMP_H_


void ping_init(void);
void set_icmp_need_send(u8 mode);
u8 icmp_need_send(void);
#endif /* ICMP_H_ */
