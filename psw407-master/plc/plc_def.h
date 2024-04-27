#ifndef PLC_DEF_H_
#define PLC_DEF_H_




typedef enum
{
  BR_300 = 0,
  BR_600,
  BR_1200,
  BR_2400,
  BR_4800,
  BR_9600,
  BR_19200,
}
em_baudrate_t;



//число поддерживаемых счетчиков
#define EM_MODEL_NUM	1

//поддерживаемые модели счетчиков
#define EM_CE102M		1


//команды
#define RESET_PLC				1
#define SW_VERSION				2
#define HW_VERSION				3
#define READ_485				6
#define WRITE_485				7
#define CONFIG_485				8

#define RELAY1					101
#define RELAY2					102
#define RELAY3					103
#define RELAY4					104
#define RELAY_OPT				105

#define REALY1_RESET			110
#define REALY2_RESET			111
#define REALY3_RESET			112
#define REALY4_RESET			113
#define RELAY_OPT_RESET			114


#define REALY1_SET_STATE		120
#define REALY2_SET_STATE		121
#define REALY3_SET_STATE		122
#define REALY4_SET_STATE		123


#define REALY1_GET_EE_STATE		130
#define REALY2_GET_EE_STATE		131
#define REALY3_GET_EE_STATE		132
#define REALY4_GET_EE_STATE		133


#define	INPUT1					140
#define INPUT2					141
#define INPUT3					142

#define INPUT_SUMMARY			150

#define REALY1_GET_STATE		160
#define REALY2_GET_STATE		161
#define REALY3_GET_STATE		162
#define REALY4_GET_STATE		163


#endif /* PLC_DEF_H_ */
