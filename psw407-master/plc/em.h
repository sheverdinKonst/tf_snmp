#ifndef EM_H_
#define EM_H_


typedef struct {
	char total[16];
	char t1[16];
	char t2[16];
	char t3[16];
	char t4[16];
}plc_ind_t;

u8 is_plc_connected(void);
void set_em_connected(u8 state);

void plc_release(void);
void plc_take(void);

void get_plc_em_model_name(char *text);
void get_plc_em_model_name_list(u8 num, char *text);

u8 is_em_connected(void);

void get_em_total(char *text);
void get_em_t1(char *text);
void get_em_t2(char *text);
void get_em_t3(char *text);
void get_em_t4(char *text);

void plc_485_send(char *str, u8 len);
void plc_485_read(char *str, u8 len);
u8 set_plc_485_config(u8 rate,u8 parity, u8 databits, u8 stopbits);
u8 plc_485_connect(void);
void get_plc_em_indications(void);

#endif
