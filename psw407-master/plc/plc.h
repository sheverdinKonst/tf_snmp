#ifndef PLC_H_
#define PLC_H_


u8 set_plc_relay(u8 num,u8 state);
u8 get_plc_relay(u8 num);
u8 plc_relay_reset(u8 num);

u8 set_plc_relay_ee(u8 num, u8 state);
u8 get_plc_relay_ee(u8 num);


void get_plc_inputs(void);


void plc_em_start(void);
void plc_processing(void);

u8 get_plc_hw_vers(void);
void set_plc_hw_vers(u8 vers);

u8 get_plc_input_num(void);
u8 get_plc_output_num(void);




#endif /* PLC_H_ */
