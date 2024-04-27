#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "poe_ltc.h"
#include "../net/stp/bridgestp.h"
#include "../net/events/events_handler.h"
#include "SMIApi.h"
#include "selftest.h"


//extern struct status_t status;


void EXTI9_5_IRQHandler(void){



	if (EXTI_GetITStatus(EXTI_Line6) != RESET){
		if(GPIO_ReadInputDataBit(LINE_IRP_GPIO_1,LINE_IRP_SDA_PIN_1)==Bit_RESET){
			if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||
			(get_dev_type() == DEV_PSW2G6F)||(get_dev_type() == DEV_PSW2G2FPLUS)||
			(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
				if(dev.plc_status.ready4events){
					dev.plc_status.new_event = 1;
				}
			}
		}
		//else
		//	set_dry_contact_state(0,1);
		// Сброс флага прерывания
		EXTI_ClearITPendingBit(EXTI_Line6);
	}


	if (EXTI_GetITStatus(EXTI_Line7) != RESET){
		//GPIO_ToggleBits(GPIOE, GPIO_Pin_2);
		//прерывание либо по датчику вскрытия, либо по сигналу SCL
		/*if(GPIO_ReadInputDataBit(LINE_IRP_GPIO_1,LINE_IRP_SCL_PIN_1)==Bit_RESET){
			if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||
			(get_dev_type() == DEV_PSW2G6F)||(get_dev_type() == DEV_PSW2G2FPLUS)||
			(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
				//GPIO_ToggleBits(GPIOE, GPIO_Pin_2);//my
				//if(status.plc_status.ready4events)
					status.plc_status.new_event = 1;
			}
		}*/
		//status.plc_status.new_event = 1;
		// Сброс флага прерывания
		EXTI_ClearITPendingBit(EXTI_Line7);
	}

	if (EXTI_GetITStatus(EXTI_Line8) != RESET){
		//независимо от версии платы, если срабатывает по заднему фронту, значит аларм
		//set_dry_contact_state(1,1);
		//set_dry_contact_state(2,1);
		// Сброс флага прерывания
		EXTI_ClearITPendingBit(EXTI_Line8);
	}


	if (EXTI_GetITStatus(EXTI_Line9) != RESET){
		//независимо от версии платы, если срабатывает по заднему фронту, значит аларм
		//set_dry_contact_state(1,1);
		//set_dry_contact_state(2,1);
		// Сброс флага прерывания
		EXTI_ClearITPendingBit(EXTI_Line9);
	}

}



void EXTI2_IRQHandler(void){
	if (EXTI_GetITStatus(EXTI_Line2) != RESET){
		//прерывание по сигналу SCL
		//if(GPIO_ReadInputDataBit(LINE_IRP_GPIO_1,LINE_IRP_SCL_PIN_1)==Bit_RESET){
		//	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		//		if(status.plc_status.ready4events)
		//			status.plc_status.new_event = 1;
		//	}
		//}

		GPIO_ToggleBits(GPIOE, GPIO_Pin_2);

		EXTI_ClearITPendingBit(EXTI_Line2);
	}
}


void EXTI15_10_IRQHandler(void){
  portBASE_TYPE xSwitchRequired = pdFALSE;

if (EXTI_GetITStatus(EXTI_Line11) != RESET){


// Если прерывание точно на ноге SW_INT // потеря линка
	if (GPIO_ReadInputDataBit(GPIOE,LINE_SW_INT_PIN)==Bit_RESET) {
		//GPIO_ToggleBits(GPIOE, GPIO_Pin_2);//зажигаем лампочку
		//ETH_LINK_int_clear();
	}
// Сброс флага прерывания
EXTI_ClearITPendingBit(EXTI_Line11);
}


/* прерывание по обнаружению SFP модуля 1 */
if ((EXTI_GetITStatus(EXTI_Line14) != RESET)){
	dev.interrupts.sfp_sd_changed = 1;
	if (GPIO_ReadInputDataBit(GPIOD,LINE_SFPSD1_PIN)==Bit_SET) {

	}
	else
	{


	}

// Сброс флага прерывания
EXTI_ClearITPendingBit(EXTI_Line14);
}

/* прерывание по обнаружению SFP модуля 2 */
if ((EXTI_GetITStatus(EXTI_Line15) != RESET)){
	dev.interrupts.sfp_sd_changed = 1;
	if (GPIO_ReadInputDataBit(GPIOD,LINE_SFPSD2_PIN)==Bit_SET) {

	}
	else
	{

	}
// Сброс флага прерывания
EXTI_ClearITPendingBit(EXTI_Line15);
}

xSwitchRequired = bstp_link_change_i();
portEND_SWITCHING_ISR( xSwitchRequired );
}



/*прерывание по нажатию кнопки default*/
void EXTI1_IRQHandler(void){
	if (EXTI_GetITStatus(EXTI_Line1) != RESET){
		if(GPIO_ReadInputDataBit(LINE_IRP_GPIO_2,LINE_IRP_SDA_PIN_2)==Bit_RESET){
			if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
				if(dev.plc_status.ready4events)
					dev.plc_status.new_event = 1;
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}

//poe_int
void EXTI3_IRQHandler(void){
	if (EXTI_GetITStatus(EXTI_Line3) != RESET){
		//LedPeriod=300;
		//poe_int_clr();
		EXTI_ClearITPendingBit(EXTI_Line3);
	}
}
