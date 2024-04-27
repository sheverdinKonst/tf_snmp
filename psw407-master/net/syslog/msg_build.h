#ifndef MSG_BUILD_H_
#define MSG_BUILD_H_

struct syslog_text_t {
 u8 level;
 char text[128];
} syslog_text,syslog_text2;

u8 make_syslog_string(u8 type,u8 group,u8 subgroup,u8 port,u8 variable,char *value);
void add(char *text);
void addmac(char *text);
void addip(char *text);
void addnum(char *text);
void addport(u8 port);
void addval(char *text);
void adddot(void);
u8 get_syslog_param(u8 type,u8 group,u8 subgroup,u8 port,u8 variable,char *value);
i8 if_need_send(u32 type,u8 *event_type);
#endif /* MSG_BUILD_H_ */
