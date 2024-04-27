

#include <stdio.h>
#include "i2c_hard.h"
#include "poe_ltc.h"
#include "poe_tps.h"
#include "board.h"
#include "stm32f4xx.h"
#include "names.h"
#include "eeprom.h"
#include "FreeRTOS.h"
#include "task.h"
#include "arm_math.h"
#include "settings.h"

static u8 poe_id = 0;
static u8 poe_init=0;
//extern struct status_t status;

static void set_ltc_limit(u8 port,u8 type);

static struct timer poe_timer[PORT_NUM];

void set_poe_init(u8 x){
	poe_init = x;
}


/*static u8 lim_to_reg(u8 limit){
	//limit - power
	//R sense = 0.25 Ohm
	u8 cut;//cut in ma
	cut = limit*1000/2915;
	switch(cut){
		case 0:
		case 1:
		case 2:	return 0x88;
		case 3:
		case 4:	return 0x08;
		case 5:
		case 6:	return 0x89;
		case 7:
		case 8:	return 0x80;
		case 9:
		case 10: return 0x8A;
		case 11:
		case 12: return 0x90;
		case 13:
		case 14: return 0x9A;
		case 15:
		case 16: return 0xC0;
		case 17:
		case 18: return 0xCA;
		case 19:
		case 20: return 0xD0;
		default: return 0x80;
	}
}*/

/*обнаружение микросхемы poe контроллера*/
/*return value:
 * -1 error
 * 1 DEV_TPS2384
 * 2 DEV_LTC4271
 * */
 
i8 get_poe_id(void){
i8 reg_val;

	if((poe_id != DEV_TPS2384)&&(poe_id != DEV_LTC4271)){

	/*detection ltc4271*/
	 reg_val = I2c_ReadByte(Adress4271,ID);


     if((reg_val>>3) == 0x0C){
		  poe_id = DEV_LTC4271;
		  return poe_id;
	 }

     I2C_Configuration();

	 /*detection ti*/
	 reg_val = I2c_ReadByte(Address2384,0);

	  if((reg_val & 0x03) == 2){
		  poe_id = DEV_TPS2384;
		  return poe_id;
	  }
	  else
		  return -1;

	}
	return poe_id;
}

/*configure interrupt on POE controller*/
u8 poe_interrupt_cfg(void){
u8 reg;
	if(get_poe_id() == DEV_LTC4271){
		/*INT global enable*/
		reg = I2c_ReadByte(Adress4271block1,MCONF);
		reg |= INTEN;
		I2c_WriteByteData(Adress4271block1,MCONF,reg);
		I2c_WriteByteData(Adress4271block2,MCONF,reg);

		/*INT MASK*/
		reg = DIS_INT;
		I2c_WriteByteData(Adress4271block1,INTMASK,reg);
		I2c_WriteByteData(Adress4271block2,INTMASK,reg);
	}
	else
		return 1;
	return 0;
}

/* clear interrupt*/
//dont use
u8 poe_int_clr(void){
	u8 reg;
	if(get_poe_id() == DEV_LTC4271){
		reg = PINCLR_PB | INTCLR_PB;
		I2c_WriteByteData(Adress4271block1,RSTPB,reg);
		I2c_WriteByteData(Adress4271block2,RSTPB,reg);
	}
	else
		return 1;
	return 0;
}


u8 poe_port_cfg(u8 port){
	if(port >= POE_PORT_NUM)
		return 1;

	if(get_port_sett_poe(port)==POE_ULTRAPOE){
		set_poe_state(POE_ULTRAPOE,port,ENABLE);
	}
	else if(get_port_sett_poe(port)==POE_FORCED_AB){
		set_poe_state(POE_FORCED_AB,port,ENABLE);
	}
	else
	{
		//for poe a
		if(dev.port_stat[port].ss_process == 1)
			set_poe_state(POE_A,port,DISABLE);
		else{
			if(get_port_sett_poe(port)==POE_AUTO || get_port_sett_poe(port)==POE_ONLY_A || get_port_sett_poe(port)==POE_MANUAL_EN)
				set_poe_state(POE_A,port,POE_AUTO);
			else
				set_poe_state(POE_A,port,POE_DISABLE);
		}



		//for poe b
		if((get_dev_type()==DEV_PSW2GPLUS)||((get_dev_type()==DEV_PSW2G6F)&&(port<POEB_PORT_NUM))
			||(get_dev_type()==DEV_PSW2G2FPLUS)||(get_dev_type()==DEV_PSW2G2FPLUSUPS)){

			if(get_port_sett_poe(port)==POE_AUTO || get_port_sett_poe(port)==POE_ONLY_B)
				set_poe_state(POE_B,port,POE_AUTO);
			else if(get_port_sett_poe(port)==POE_MANUAL_EN){
				set_poe_state(POE_B,port,POE_MANUAL_EN);
			}
			else{
				set_poe_state(POE_B,port,POE_DISABLE);
			}
		}
	}
	return 0;
}



void PoEControlTask(void *pvParam){
u8 port;
//init
if(get_poe_id() == DEV_TPS2384){
	poe_alt_ab_init();
}

for(port=0;port<POE_PORT_NUM;port++){
	poe_port_cfg(port);
}

	while(1){
		for(port=0;port<POE_PORT_NUM;port++){


			//get poe state
			dev.port_stat[port].poe_a=get_poe_state(POE_A,port);

			if((get_dev_type()==DEV_PSW2GPLUS)||((get_dev_type()==DEV_PSW2G6F)&&(port<POEB_PORT_NUM))||
			  (((get_dev_type()==DEV_PSW2G2FPLUS)||(get_dev_type()==DEV_PSW2G2FPLUSUPS))&&(port<POEB_PORT_NUM)))
				dev.port_stat[port].poe_b=get_poe_state(POE_B,port);


			vTaskDelay(100*MSEC);


			//periodicaly task only for manual mode
			if((get_port_sett_poe(port) == POE_MANUAL_EN && get_port_poe_b(port) == DISABLE)

			|| ((get_port_sett_poe(port) == POE_ULTRAPOE)&&
				((get_port_poe_a(port) == DISABLE)||(get_port_poe_b(port) == DISABLE)))

			|| ((get_port_sett_poe(port) == POE_FORCED_AB)&&
				((get_port_poe_a(port) == DISABLE)||(get_port_poe_b(port) == DISABLE)))){

				poe_port_cfg(port);
			}


			if (poe_init == 0){
				//if auto mode & disabled mode
				poe_port_cfg(port);
			}
		}
		set_poe_init(1);
		vTaskDelay(5000*MSEC);
	}
}





u8 if_poe_state_changed(u8 port){

	if(port >= POE_PORT_NUM)
		return 1;

	if(/*dev.port_stat[port].poe_a != dev.port_stat[port].poe_a_last*/1){
		//generate
		//event();
		//dev.port_stat[port].poe_a_last = dev.port_stat[port].poe_a;
	}
	if(/*dev.port_stat[port].poe_b != dev.port_stat[port].poe_b_last*/1){
		//generate
		//event();
		//dev.port_stat[port].poe_b_last = dev.port_stat[port].poe_b;
	}

	return 0;

}

u8 calculate_addr_bank(u8 port){
	if(get_dev_type() == DEV_PSW2GPLUS){
		//выбор адреса
		//порт 0-1 банк 1
		//порт 2-3 банк 2
		if(port < 2)
			return Adress4271block1;
		else
			return Adress4271block2;
	}
	else if(get_dev_type() == DEV_PSW2G6F){
		//выбор адреса
		//порт 0-1 банк 1
		//порт 2-7 банк 2
		if(port < 2)
			return Adress4271block1;
		else
			return Adress4271block2;
	}
	else if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
		return Adress4271block2;
	}
	else if(get_dev_type() == DEV_PSW2G8F){
		//выбор адреса
		//порт 0-3 банк 1
		//порт 4-7 банк 2
		if(port < 4)
			return Adress4271block1;
		else
			return Adress4271block2;
	}

	return 0;
}

//расчет смещения
u8 calculate_offset(u8 type,u8 port){
	if(get_dev_type() == DEV_PSW2GPLUS){
		if(type == POE_A){
			if(port == 0 || port == 2)
				return 0;
			else
				return 2;
		}else if(type == POE_B){
			if(port == 0 || port == 2)
				return 1;
			else
				return 3;
		}
	}
	else if(get_dev_type() == DEV_PSW2G6F){
		if(type == POE_A){
			switch(port){
				case 0: return 0;
				case 1: return 2;
				case 2: return 0;
				case 3: return 1;
				case 4: return 2;
				case 5: return 3;
				default: return 0;
			}

		}else if(type == POE_B){
			if(port == 0)
				return 1;
			else if(port == 1)
				return 3;

		}
	}
	else if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
		if(port == 0){
			if(type == POE_A){
				return 2;
			}
			if(type == POE_B){
				return 3;
			}
		}
		else
			return 1;
	}
	else if(get_dev_type() == DEV_PSW2G8F){
		if(type == POE_A){
			switch(port){
				case 0: return 0;
				case 1: return 1;
				case 2: return 2;
				case 3: return 3;
				case 4: return 0;
				case 5: return 1;
				case 6: return 2;
				case 7: return 3;
				default: return 0;
			}
		}
		else
			return 0;
	}

	return 0;
}


u8 get_poe_state(u8 type,u8 port){
u8 reg;
u8 addres_bank;
u8 offset;
	if(get_poe_id() == DEV_LTC4271){
		addres_bank = calculate_addr_bank(port);
		offset = calculate_offset(type,port);
		reg = I2c_ReadByte(addres_bank,STATPWR);
		if(reg & (1<<offset))
			return 1;
		else
			return 0;
	}
	if(get_poe_id() == DEV_TPS2384){
		if(PoEStateRead(port) == 2)
			return 1;
		else
			return 0;
	}
	return 0;
}

//port 0..3
//state 0 - disable poe
//		1 - auto poe
//		2 - hard enable

void set_poe_state(u8 type,uint8_t port, uint8_t state){
u8 reg;
u8 addres_bank;
u8 offset;
#if POE_NO_LIMIT
u8 limit=0;
if(type == POE_A)
	limit = get_port_sett_pwr_lim_a(port);
if(type == POE_B)
	limit = get_port_sett_pwr_lim_b(port);
#endif

if(get_poe_id() == DEV_LTC4271){

	if(type == POE_A || type == POE_B){
		addres_bank = calculate_addr_bank(port);
		offset = calculate_offset(type,port);
		switch(state){
			case POE_DISABLE:
				//0x12 opmd[opmd1]=00 (disable)
				reg = I2c_ReadByte(addres_bank,OPMD);
				reg &= ~(0x3<<(offset*2));
				I2c_WriteByteData(addres_bank,OPMD,reg);
				break;
			case POE_AUTO:
				//set_poe_plus_pwr();
				//0x12 opmd[opmd1]=11 (auto)
				reg = I2c_ReadByte(addres_bank,OPMD);
				reg |= (0x3<<(offset*2));
				I2c_WriteByteData(addres_bank,OPMD,reg);
				//0x14 detena[cls1]=1
				//0x14 detena[det1]=1
				reg = I2c_ReadByte(addres_bank,DETENA);
				reg |= (0x11<<(offset));
				I2c_WriteByteData(addres_bank,DETENA,reg);
				break;

			case POE_MANUAL_EN://passive poe
				//cut1[cut]= 0x22 - set to class4 (default class 3 - TH-03 don`t work)
				if(type == POE_B){
					reg = 0xE2;
					switch(port){
						case 0:
						case 2:
							I2c_WriteByteData(addres_bank,CUT2,reg);
							break;
						case 1:
						case 3:
							I2c_WriteByteData(addres_bank,CUT4,reg);
							break;
					}
				}

				//0x12 opmd[opmd1]=01 (manual)
				reg = I2c_ReadByte(addres_bank,OPMD);
				reg &= ~(0x3<<(offset*2));
				reg |= (0x01<<(offset*2));
				I2c_WriteByteData(addres_bank,OPMD,reg);


				//pwrpb[det]=1
				reg=0;
				reg = 1<<(offset);
				I2c_WriteByteData(addres_bank,DETPB,reg);
				vTaskDelay(5*MSEC);
				//read stat

				//printf("manual det %d\r\n",port);

				reg = I2c_ReadByte(addres_bank,STATP1+offset);
				if(((reg & 0x07) != 0x01)&&((reg & 0x07) != 0x04))
					break;
				//Idis = 2460мА
				set_ltc_limit(port,TYPE_UNLIMIT);
				vTaskDelay(10*MSEC);

				//printf("manual pb on %d\r\n",port);

				//0x19 pwrpb[on1]=1
				reg=0;
				reg = 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);
				break;

	#if POE_NO_LIMIT
			case POE_NO_LIM:
				//disable detection

				reg = I2c_ReadByte(addres_bank,DETENA);
				reg &= ~(0x01<<(offset));
				I2c_WriteByteData(addres_bank,DETENA,reg);


				if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
					I2c_WriteByteData(addres_bank,CUT3,/*0xFF*/0xE2);
					I2c_WriteByteData(addres_bank,LIM3,0x10);
					I2c_WriteByteData(addres_bank,CUT4,/*0xFF*/0xE2);
					I2c_WriteByteData(addres_bank,LIM4,0x10);
				}
				else{
					if(type == POE_A){
						switch(port){
							case 0:
							case 2:
								I2c_WriteByteData(addres_bank,CUT1,0xFF);
								I2c_WriteByteData(addres_bank,LIM1,0xDA);
								break;
							case 1:
							case 3:
								I2c_WriteByteData(addres_bank,CUT3,0xFF);
								I2c_WriteByteData(addres_bank,LIM3,0xDA);
								break;
						}
					}
				}
				//pwrpb[det]=1
				reg=0;
				reg = 1<<(offset);
				I2c_WriteByteData(addres_bank,DETPB,reg);
				//vTaskDelay(5*MSEC);
				//read stat
				reg = I2c_ReadByte(addres_bank,STATP1+offset);
				if(((reg & 0x07) != 0x01)&&((reg & 0x07) != 0x04)){
					//disable detection
					reg = I2c_ReadByte(addres_bank,DETENA);
					reg &= ~(0x01<<(offset));
					I2c_WriteByteData(addres_bank,DETENA,reg);
					break;
				}
				reg=I2c_ReadByte(addres_bank,PWRPB);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);

				break;
	#endif
		}
	}
	else if(type == POE_ULTRAPOE){
		addres_bank = calculate_addr_bank(port);

		//0x12 opmd[opmd1]=01 (manual)
		offset = calculate_offset(POE_A,port);
		reg = I2c_ReadByte(addres_bank,OPMD);
		reg &= ~(0x3<<(offset*2));
		reg |= (0x01<<(offset*2));

		//0x12 opmd[opmd1]=01 (manual)
		offset = calculate_offset(POE_B,port);
		reg &= ~(0x3<<(offset*2));
		reg |= (0x01<<(offset*2));
		I2c_WriteByteData(addres_bank,OPMD,reg);

		//если PoE нет
		if(!get_poe_state(POE_A,port) && !get_poe_state(POE_B,port)){
			//start detection & classification Type B
			//detpb[det]=1  detpb[cls]=1
			offset = calculate_offset(POE_B,port);
			reg = 0x11<<(offset);
			I2c_WriteByteData(addres_bank,DETPB,reg);
			vTaskDelay(100*MSEC);
			//read stat
			//det = 4 - ok
			//class == 0..4
			reg = I2c_ReadByte(addres_bank,STATP1+offset);

			if(((reg & 0x07) == 0x04) && ((((reg>>4)&0x07)<5)||((reg>>4)&0x07)==6)){
				//Idis = 2460мА
				set_ltc_limit(port,TYPE_UNLIMIT);
				vTaskDelay(10*MSEC);
				//Type B up.
				reg=I2c_ReadByte(addres_bank,PWRPB);
				offset = calculate_offset(POE_B,port);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);
				vTaskDelay(10*MSEC);
				//type A up
				reg=I2c_ReadByte(addres_bank,PWRPB);
				offset = calculate_offset(POE_A,port);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);

				//wait 1 min
				timer_set(&poe_timer[port],60000*MSEC);
			}
		}

		//если PoE на одном из каналов, повторно запитываем
		if(get_poe_state(POE_A,port) != get_poe_state(POE_B,port) && timer_expired(&poe_timer[port])){
			//Idis = 2460мА
			set_ltc_limit(port,TYPE_UNLIMIT);
			vTaskDelay(10*MSEC);
			//Type B up.
			reg=I2c_ReadByte(addres_bank,PWRPB);
			offset = calculate_offset(POE_B,port);
			reg |= 1<<(offset);
			I2c_WriteByteData(addres_bank,PWRPB,reg);
			vTaskDelay(10*MSEC);
			//type A up
			reg=I2c_ReadByte(addres_bank,PWRPB);
			offset = calculate_offset(POE_A,port);
			reg |= 1<<(offset);
			I2c_WriteByteData(addres_bank,PWRPB,reg);

			//wait 1 min
			timer_set(&poe_timer[port],60000*MSEC);
		}
	}

	else if(type == POE_FORCED_AB){
			addres_bank = calculate_addr_bank(port);

			//0x12 opmd[opmd1]=01 (manual)
			offset = calculate_offset(POE_A,port);
			reg = I2c_ReadByte(addres_bank,OPMD);
			reg &= ~(0x3<<(offset*2));
			reg |= (0x01<<(offset*2));

			//0x12 opmd[opmd1]=01 (manual)
			offset = calculate_offset(POE_B,port);
			reg &= ~(0x3<<(offset*2));
			reg |= (0x01<<(offset*2));
			I2c_WriteByteData(addres_bank,OPMD,reg);

			//если PoE нет
			//if(!get_poe_state(POE_A,port) && !get_poe_state(POE_B,port)){
				//Idis = 2460мА
				set_ltc_limit(port,TYPE_UNLIMIT);
				vTaskDelay(10*MSEC);
				//Type B up.
				reg=I2c_ReadByte(addres_bank,PWRPB);
				offset = calculate_offset(POE_B,port);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);
				vTaskDelay(10*MSEC);
				//type A up
				reg=I2c_ReadByte(addres_bank,PWRPB);
				offset = calculate_offset(POE_A,port);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);

				//wait 1 min
				//timer_set(&poe_timer[port],60000*MSEC);
			//}

			//если PoE на одном из каналов, повторно запитываем
			/*if(get_poe_state(POE_A,port) != get_poe_state(POE_B,port) && timer_expired(&poe_timer[port])){
				//Idis = 2460мА
				set_ltc_limit(port,TYPE_UNLIMIT);
				vTaskDelay(10*MSEC);
				//Type B up.
				reg=I2c_ReadByte(addres_bank,PWRPB);
				offset = calculate_offset(POE_B,port);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);
				vTaskDelay(10*MSEC);
				//type A up
				reg=I2c_ReadByte(addres_bank,PWRPB);
				offset = calculate_offset(POE_A,port);
				reg |= 1<<(offset);
				I2c_WriteByteData(addres_bank,PWRPB,reg);

				//wait 1 min
				timer_set(&poe_timer[port],60000*MSEC);
			}*/
		}
	}

	if(get_poe_id() == DEV_TPS2384){
		if(type == POE_A){
			//disable port over/undervoltage fault
			switch(port){
				case 0: I2c_WriteByteData(Address2384,8, (1<<2));break;
				case 1: I2c_WriteByteData(Address2384,9, (1<<2));break;
				case 2: I2c_WriteByteData(Address2384,10,(1<<2));break;
				case 3: I2c_WriteByteData(Address2384,11,(1<<2));break;
			}

			state = (!state) & 0x01;
			switch(port){
				case 0:I2c_WriteByteData(Address2384,24,(state<<4));break;
				case 1:I2c_WriteByteData(Address2384,25,(state<<4));break;
				case 2:I2c_WriteByteData(Address2384,26,(state<<4));break;
				case 3:I2c_WriteByteData(Address2384,27,(state<<4));break;
			}
			set_poe_alt_ab(0);
			vTaskDelay(20*MSEC);
			set_poe_alt_ab(1);
		}
	}
}


static void set_ltc_limit(u8 port,u8 type){
u8	addres_bank;

	addres_bank = calculate_addr_bank(port);
	if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
		if(type == TYPE_NORMAL){
			I2c_WriteByteData(addres_bank,CUT3,0xE2);
			I2c_WriteByteData(addres_bank,LIM3,0x10);
			I2c_WriteByteData(addres_bank,CUT4,0xE2);
			I2c_WriteByteData(addres_bank,LIM4,0x10);
		}
		if(type == TYPE_UNLIMIT){
			I2c_WriteByteData(addres_bank,CUT3,0xBF);
			I2c_WriteByteData(addres_bank,LIM3,0x50);
			I2c_WriteByteData(addres_bank,CUT4,0xBF);
			I2c_WriteByteData(addres_bank,LIM4,0x50);
		}
	}
	else if(get_dev_type() == DEV_PSW2GPLUS){
		switch(port){
			case 0:
			case 2:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT1,0xE2);
					I2c_WriteByteData(addres_bank,LIM1,0x10);
					I2c_WriteByteData(addres_bank,CUT2,0xE2);
					I2c_WriteByteData(addres_bank,LIM2,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT1,0xBF);
					I2c_WriteByteData(addres_bank,LIM1,0x50);
					I2c_WriteByteData(addres_bank,CUT2,0xBF);
					I2c_WriteByteData(addres_bank,LIM2,0x50);
				}
				break;
			case 1:
			case 3:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT3,0xE2);
					I2c_WriteByteData(addres_bank,LIM3,0x10);
					I2c_WriteByteData(addres_bank,CUT4,0xE2);
					I2c_WriteByteData(addres_bank,LIM4,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT3,0xBF);
					I2c_WriteByteData(addres_bank,LIM3,0x50);
					I2c_WriteByteData(addres_bank,CUT4,0xBF);
					I2c_WriteByteData(addres_bank,LIM4,0x50);
				}
				break;
		}
	}
	else if(get_dev_type() == DEV_PSW2G6F){
		switch(port){
			case 0:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT1,0xE2);
					I2c_WriteByteData(addres_bank,LIM1,0x10);
					I2c_WriteByteData(addres_bank,CUT2,0xE2);
					I2c_WriteByteData(addres_bank,LIM2,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT1,0xBF);
					I2c_WriteByteData(addres_bank,LIM1,0x50);
					I2c_WriteByteData(addres_bank,CUT2,0xBF);
					I2c_WriteByteData(addres_bank,LIM2,0x50);
				}
				break;
			case 1:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT3,0xE2);
					I2c_WriteByteData(addres_bank,LIM3,0x10);
					I2c_WriteByteData(addres_bank,CUT4,0xE2);
					I2c_WriteByteData(addres_bank,LIM4,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT3,0xBF);
					I2c_WriteByteData(addres_bank,LIM3,0x50);
					I2c_WriteByteData(addres_bank,CUT4,0xBF);
					I2c_WriteByteData(addres_bank,LIM4,0x50);
				}
				break;
			case 2:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT1,0xE2);
					I2c_WriteByteData(addres_bank,LIM1,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT1,0xBF);
					I2c_WriteByteData(addres_bank,LIM1,0x50);
				}
				break;
			case 3:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT2,0xE2);
					I2c_WriteByteData(addres_bank,LIM2,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT2,0xBF);
					I2c_WriteByteData(addres_bank,LIM2,0x50);
				}
				break;
			case 4:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT3,0xE2);
					I2c_WriteByteData(addres_bank,LIM3,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT3,0xBF);
					I2c_WriteByteData(addres_bank,LIM3,0x50);
				}
				break;
			case 5:
				if(type == TYPE_NORMAL){
					I2c_WriteByteData(addres_bank,CUT4,0xE2);
					I2c_WriteByteData(addres_bank,LIM4,0x10);
				}
				if(type == TYPE_UNLIMIT){
					I2c_WriteByteData(addres_bank,CUT4,0xBF);
					I2c_WriteByteData(addres_bank,LIM4,0x50);
				}
				break;
		}
	}
}

/* возвращает значение тока текущего через порт в мА */
uint16_t get_poe_current(u8 type,u8 port){
u8 lsb=0,msb=0;
uint16_t current=0;
u8 lsb_addr=0,msb_addr=0;
u8 addres_bank,offset;

if(get_poe_id() == DEV_LTC4271){
	addres_bank = calculate_addr_bank(port);
	offset = calculate_offset(type,port);

	lsb_addr = IP1LSB+offset*4;
	msb_addr = IP1MSB+offset*4;

	lsb = I2c_ReadByte(addres_bank,lsb_addr);
	msb = I2c_ReadByte(addres_bank,msb_addr);

	current = ((msb&0x3F)<<8) | lsb;
	return (u16)(current * CLSB);
}

if(get_poe_id() == DEV_TPS2384){
	if(type == POE_A)
		return PoECurrentRead(port);
	return 0;
}

return 0;
}

/* возвращает значение напряжения на порту в mV */
uint16_t get_poe_voltage(u8 type,u8 port){
uint16_t voltage=0;
u8 lsb=0,msb=0;
u8 addres_bank,offset;
u8 lsb_addr=0,msb_addr=0;


if(get_poe_id() == DEV_LTC4271){

	addres_bank = calculate_addr_bank(port);

	offset = calculate_offset(type,port);

	lsb_addr = VP1LSB+offset*4;
	msb_addr = VP1MSB+offset*4;

	lsb = I2c_ReadByte(addres_bank,lsb_addr);
	msb = I2c_ReadByte(addres_bank,msb_addr);

	voltage = ((msb&0x3F)<<8) | lsb;
	return (u16)(voltage * VLSB);
}

if(get_poe_id() == DEV_TPS2384){
	if(type == POE_A)
		return PoEVoltageRead(port);
	return 0;
}

return 0;
}

