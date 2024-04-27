#ifndef DEBUG_H_
#define DEBUG_H_

#include "stm32f4xx.h"
#include "telnet/usb_shell.h"

#define TYPE_CHAR			1
#define TYPE_HEX			2
#define MAX_PRINTF_SIZE		256


#define DEBUG_MSG(lev,fmt, arg...) if(lev && usb_shell_state()==0){printf(fmt, ##arg);for(u16 i=0;i<10000;i++){asm("nop");}}


void printf_arr(u8 type,u8 *buff,u32 size);


#endif /* DEBUG_H_ */
