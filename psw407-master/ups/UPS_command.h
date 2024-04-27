#ifndef __UPS_COMMAND_H
#define __UPS_COMMAND_H

#define RIP_HW_ID 2

/*система команд*/
#define ID_READ 						35
#define VAC_READ 						1
#define AKB_PRESENT 					2
#define AKB_VOLTAGE_READ				3
#define TEMPER_READ 					4
#define VOUT_KEY_READ 					5
#define CURR_KEY_READ 					6
#define BAT_KEY_READ 					7
#define Up_Level_Voltage_Bat_READ 		8
#define Down_Level_Voltage_Bat_READ 	9
#define Safe_Down_Level_Voltage_Bat_READ  10
#define Hi_Current_Charge_READ 			11
#define LOW_Current_Charge_READ 		12
#define Safe_Null_Level_Voltage_Bat_READ 13
#define Temp_charge_READ 				14
#define Temp_delta_READ 				15
#define Delta_READ 						16
#define KoefDel_READ 					17
#define VOUT_KEY_WRITE 					18
#define CURR_KEY_WRITE 					19
#define BAT_KEY_WRITE 					20
#define Up_Level_Voltage_Bat_WRITE		21
#define Down_Level_Voltage_Bat_WRITE 	22
#define Safe_Down_Level_Voltage_Bat_WRITE 23
#define Hi_Current_Charge_WRITE 		24
#define LOW_Current_Charge_WRITE 		25
#define Safe_Null_Level_Voltage_Bat_WRITE 26
#define Temp_charge_WRITE 				27
#define Temp_delta_WRITE 				28
#define Delta_WRITE 					29
#define KoefDel_WRITE 					30
#define Short_Circuit_Delay_READ 		31
#define Short_Circuit_Delay_WRITE 		32
#define Safe_Up_Level_Voltage_Bat_READ	33
#define Safe_Up_Level_Voltage_Bat_WRITE 34
#define Reset_IPR 						36
#define VOUT_KEY_AUTO 					37
#define CURR_KEY_AUTO 					38
#define BAT_KEY_AUTO 					39
#define VentilTemper_READ 				40
#define VentilTemper_WRITE 				41
#define VentilTime_READ 				42
#define VentilTime_WRITE 				43
#define Version_READ					44
#define Ventil_Enable 					45
#define Ventil_Disable 					46
#define AKB_VOLTAGE_CHARGE_READ 		47
#define DELAYED_START_READ				50
#define DELAYED_START_WRITE				51



//начиная с этой версиии поддерживается сохранение отложенного старта
#define VERS_11	11

//начиная с этой версии обмен данными только через ADUM
#define VERS_8 8

struct RemainingTimeUPS_t {
	uint8_t min;
	uint8_t hour;
	u8 		valid;//valid data
 };

uint8_t UpsTime(struct RemainingTimeUPS_t *remtime);
void UPS_PLC_detect(void);
void UPS_procesing(void);

u8 is_ups_mode(void);
u8 is_plc_connected(void);
u8 is_akb_detect(void);
u8 is_ups_rezerv(void);
u16 get_akb_voltage(void);


int get_ups_delay_start(void);
u8 set_ups_delay_start(u8 state);




#endif //__UPS_COMMAND_H
