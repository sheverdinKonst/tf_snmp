
 #ifndef CFG_SYSLOG_H
 #define CFG_SYSLOG_H
 
 #define CONFIG_SYSLOG_NET     1//передавать syslog сообщения по сети
 
 #define CONFIG_SYSLOG_SERIAL  0//передавать syslog в USB
 
#define SYSLOG_PORT		     514///порт

#define SYSLOG_MAX_LEN		128


#define S_EMERGENCY 		0 //система неработоспособна
#define S_ALERT 			1 //система требует немедленного вмешательства
#define S_CRITICAL 			2 //состояние системы критическое
#define S_ERROR 			3 //сообщения о возникших ошибках
#define S_WARNING 			4 //предупреждения о возможных проблемах
#define S_NOTICE  			5 //сообщения о нормальных, но важных событиях
#define S_INFORMATIONAL 	6 //информационные сообщения
#define S_DEBUG 			7 //отладочные сообщения

#endif /* CFG_SYSLOG_H */
