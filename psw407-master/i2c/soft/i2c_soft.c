#include <stdio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_gpio.h>
#include "i2c_soft.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "settings.h"
#include "sfp_cmd.h"
#include "../debug.h"
#include "../plc/plc.h"
#include "../plc/em.h"

u8 curr_sfp;
static void i2c_start(void);
static uint8_t i2c_write_byte( uint8_t c );
void i2c_stop(void);

static void send_reset(void);
static void send_stop(void);
static void send_start(void);
static void send_ack(int ack);
static void I2C_INIT(void);


//#define I2CSOFT_DEBUG 1

GPIO_InitTypeDef GPIO_InitStructure;

//uint16_t delay;
uint8_t I2C_MODE;

uint8_t busy=0;

#define I2C_DELAY	if(I2C_MODE==IRP) 		\
						vTaskDelay(5*MSEC);	\
					else if(I2C_MODE==PLC) 	\
						vTaskDelay(1*MSEC);		\
					else 					\
						vTaskDelay(1);//sfp

void WaitIfBussy(void){
u8 TimeOut=100;
	while((busy)&&(TimeOut)){
		TimeOut--;
		vTaskDelay(1*MSEC);
	}
}

void SetI2CMode(uint8_t Mode){
	I2C_MODE=Mode;
	if(get_dev_type() == DEV_SWU16)
		sfp_set_addr(Mode);
}

static void SetI2CModeByAddr(uint8_t i2c_addr){
	if(i2c_addr == IRP_ADDR){
		SetI2CMode(IRP);
	}
	else if(i2c_addr == PLC_ADDR){
		SetI2CMode(PLC);
	}
}

u8 GetI2CMode(void){
	return (I2C_MODE);
}



void I2C_SCL(uint8_t x){
	if(I2C_MODE==SFP1){
		if (x) GPIO_SetBits(LINE_SFP1_GPIO, LINE_SFP1_SCL_PIN);
		else GPIO_ResetBits(LINE_SFP1_GPIO, LINE_SFP1_SCL_PIN);
	}
	else if(I2C_MODE==SFP2){
		if (x) GPIO_SetBits(LINE_SFP2_GPIO, LINE_SFP2_SCL_PIN);
		else GPIO_ResetBits(LINE_SFP2_GPIO, LINE_SFP2_SCL_PIN);
	}
	else if(I2C_MODE==IRP || I2C_MODE==PLC){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			if (x) GPIO_SetBits(LINE_IRP_GPIO_2, LINE_IRP_SCL_PIN_2);
			else GPIO_ResetBits(LINE_IRP_GPIO_2, LINE_IRP_SCL_PIN_2);
		}else{
			if (x) GPIO_SetBits(LINE_IRP_GPIO_1, LINE_IRP_SCL_PIN_1);
			else GPIO_ResetBits(LINE_IRP_GPIO_1, LINE_IRP_SCL_PIN_1);
		}
	}
	else if(get_dev_type() == DEV_SWU16){
		if (x)
			GPIO_SetBits(LINE_SFP1_GPIO, LINE_SFP1_SCL_PIN);
		else
			GPIO_ResetBits(LINE_SFP1_GPIO, LINE_SFP1_SCL_PIN);
	}
}


void I2C_SDA(uint8_t x){
	if(I2C_MODE==SFP1){
		if (x) GPIO_SetBits(LINE_SFP1_GPIO, LINE_SFP1_SDA_PIN);
		else GPIO_ResetBits(LINE_SFP1_GPIO, LINE_SFP1_SDA_PIN);
	}
	else if(I2C_MODE==SFP2){
		if (x) GPIO_SetBits(LINE_SFP2_GPIO, LINE_SFP2_SDA_PIN);
		else GPIO_ResetBits(LINE_SFP2_GPIO, LINE_SFP2_SDA_PIN);
	}
	else if(I2C_MODE==IRP || I2C_MODE==PLC){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			if (x) GPIO_SetBits(LINE_IRP_GPIO_2, LINE_IRP_SDA_PIN_2);
			else GPIO_ResetBits(LINE_IRP_GPIO_2, LINE_IRP_SDA_PIN_2);
		}
		else{
			//psw 2g4f
			if (x) GPIO_SetBits(LINE_IRP_GPIO_1, LINE_IRP_SDA_PIN_1);
			else GPIO_ResetBits(LINE_IRP_GPIO_1, LINE_IRP_SDA_PIN_1);
		}
	}
	else if(get_dev_type() == DEV_SWU16){
		if (x)
			GPIO_SetBits(LINE_SFP1_GPIO, LINE_SFP1_SDA_PIN);
		else
			GPIO_ResetBits(LINE_SFP1_GPIO, LINE_SFP1_SDA_PIN);
	}
}

void I2C_TRISTATE(void){
	if(I2C_MODE==SFP1){
		GPIO_InitStructure.GPIO_Pin = LINE_SFP1_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_SFP1_GPIO, &GPIO_InitStructure);
	}
	else if(I2C_MODE==SFP2){
		GPIO_InitStructure.GPIO_Pin = LINE_SFP2_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_SFP2_GPIO, &GPIO_InitStructure);
	}
	else if(I2C_MODE==IRP || I2C_MODE==PLC){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_2;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
			GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);
		}else{
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_1;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
			GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);
		}
	}
	else if(get_dev_type() == DEV_SWU16){
		GPIO_InitStructure.GPIO_Pin = LINE_SFP1_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_SFP1_GPIO, &GPIO_InitStructure);

		sfp_set_write(ENABLE);
		//GPIO_SetBits(GPIOA, GPIO_Pin_8);
	}
}

void I2C_SCL_TRISTATE(void){

	if(get_dev_type() != DEV_SWU16){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			//SCL-in - для приема прерываний
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_2;//7
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
			GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);//pc
		}
		else{
			//SCL-in - для приема прерываний
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_1;//7
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
			GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);//pc
		}
	}
}

void I2C_SCL_ACTIVE(void){

	if(get_dev_type() != DEV_SWU16){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			//scl
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_2;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
			GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
			GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);
		}
		else{
			//scl
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_1;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
			GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
			GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);

			//SCL-out
			/*GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_2;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
			GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
			GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);
			GPIO_ResetBits(LINE_IRP_GPIO_2, LINE_IRP_SCL_PIN_2);*/
		}
	}
}

void I2C_ACTIVE(void){
	if(I2C_MODE==SFP1){
		GPIO_InitStructure.GPIO_Pin = LINE_SFP1_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(LINE_SFP1_GPIO, &GPIO_InitStructure);
	}
	else if(I2C_MODE==SFP2){
		GPIO_InitStructure.GPIO_Pin = LINE_SFP2_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(LINE_SFP2_GPIO, &GPIO_InitStructure);
	}
	else if(I2C_MODE==IRP || I2C_MODE==PLC){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_2;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
			GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
			GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);


		}else{
			GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_1;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
			GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
			GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);


		}
	}
	else if(get_dev_type() == DEV_SWU16){
		GPIO_InitStructure.GPIO_Pin = LINE_SFP1_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(LINE_SFP1_GPIO, &GPIO_InitStructure);

		sfp_set_write(DISABLE);
		//GPIO_ResetBits(GPIOA, GPIO_Pin_8);
	}

}

uint8_t I2C_READ(void){
uint8_t Ret=3;
	if(I2C_MODE==SFP1){
		Ret=GPIO_ReadInputDataBit(LINE_SFP1_GPIO,LINE_SFP1_SDA_PIN);
	}
	else if(I2C_MODE==SFP2){
		Ret=GPIO_ReadInputDataBit(LINE_SFP2_GPIO,LINE_SFP2_SDA_PIN);
	}
	else if(I2C_MODE==IRP || I2C_MODE==PLC){
		if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS))
			Ret=GPIO_ReadInputDataBit(LINE_IRP_GPIO_2,LINE_IRP_SDA_PIN_2);
		else
			Ret=GPIO_ReadInputDataBit(LINE_IRP_GPIO_1,LINE_IRP_SDA_PIN_1);
	}
	else{
		Ret=GPIO_ReadInputDataBit(LINE_SFP1_GPIO,LINE_SFP1_SDA_PIN);
	}
	return Ret;
}

/*-----------------------------------------------------------------------
 * Send a reset sequence consisting of 9 clocks with the data signal high
 * to clock any confused device back into an idle state.  Also send a
 * <stop> at the end of the sequence for belts & suspenders.
 */

void I2C_INIT(void){
	/*SFP1*/
	//SDA-in
	GPIO_InitStructure.GPIO_Pin = LINE_SFP1_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(LINE_SFP1_GPIO, &GPIO_InitStructure);
	//SCL-out
	GPIO_InitStructure.GPIO_Pin = LINE_SFP1_SCL_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(LINE_SFP1_GPIO, &GPIO_InitStructure);
	GPIO_ResetBits(LINE_SFP1_GPIO, LINE_SFP1_SCL_PIN);

	/*SFP2*/
	//SDA-in
	GPIO_InitStructure.GPIO_Pin = LINE_SFP2_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(LINE_SFP2_GPIO, &GPIO_InitStructure);

	//SCL-out
	GPIO_InitStructure.GPIO_Pin = LINE_SFP2_SCL_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(LINE_SFP2_GPIO, &GPIO_InitStructure);
	GPIO_ResetBits(LINE_SFP2_GPIO, LINE_SFP2_SCL_PIN);

	/*IRP*/


	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		//SDA-in
		GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);
		//SCL-out
		GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);
		GPIO_ResetBits(LINE_IRP_GPIO_2, LINE_IRP_SCL_PIN_2);
	}
	else{
		//SDA-in
		GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_1;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);
		//SCL-out
		GPIO_InitStructure.GPIO_Pin = LINE_IRP_SCL_PIN_1;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
		GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);
		GPIO_ResetBits(LINE_IRP_GPIO_1, LINE_IRP_SCL_PIN_1);
	}
}



static void send_reset(void){
	int j;

	I2C_SCL(1);
	I2C_SDA(1);
	I2C_TRISTATE();
	for(j = 0; j < 9; j++) {
		I2C_SCL(0);
		I2C_DELAY;
		I2C_DELAY;
		I2C_SCL(1);
		I2C_DELAY;
		I2C_DELAY;
	}
	send_stop();
	I2C_TRISTATE();
}

static void send_start(void){
	plc_take();//заняли, запретили прерывания
	I2C_DELAY;
	I2C_SDA(1);
	I2C_ACTIVE();
	I2C_DELAY;
	I2C_SCL(1);
	I2C_DELAY;
	I2C_SDA(0);
	I2C_DELAY;
}

static void send_stop(void){

	I2C_SCL(0);
	I2C_DELAY;
	I2C_SDA(0);
	I2C_ACTIVE();
	I2C_DELAY;
	I2C_SCL(1);
	I2C_DELAY;
	I2C_SDA(1);
	I2C_DELAY;
	I2C_TRISTATE();
	plc_release();//заняли, запретили прерывания
	//I2C_SCL_TRISTATE();//my
}

static void send_ack(int ack){

	I2C_SCL(0);
	I2C_DELAY;
	I2C_ACTIVE();
	I2C_SDA(ack);
	I2C_DELAY;
	I2C_SCL(1);
	I2C_DELAY;
	I2C_DELAY;
	I2C_SCL(0);
	I2C_DELAY;
}

int write_byte(unsigned char data){
	int j;
	int nack=3;
	unsigned char data_old;
	data_old = data;

	I2C_ACTIVE();
	for(j = 0; j < 8; j++) {
		I2C_SCL(0);
		I2C_DELAY;
		if (data & 0x80)
			I2C_SDA(1);
        else
        	I2C_SDA(0);
		I2C_DELAY;
		I2C_SCL(1);
		I2C_DELAY;
		data <<= 1;
	}

	/*
	* Look for an <ACK>(negative logic) and return it.
	*/
	I2C_SCL(0);
	I2C_DELAY;
	I2C_SDA(1);
	I2C_TRISTATE();
	I2C_DELAY;
	I2C_SCL(1);
	I2C_DELAY;

	if (I2C_READ()==Bit_RESET)
		nack=0;
	else
		nack = 1;


	I2C_SCL(0);
	I2C_DELAY;
	I2C_ACTIVE();

	//I2C_SCL(0);
	//I2C_ACTIVE();
	//I2C_SDA(nack);//my
	//I2C_DELAY;
	//I2C_ACTIVE();//comment my
	//DEBUG_MSG(I2CSOFT_DEBUG,"write_byte %X, nack: %d\r\n",data_old,nack);
	return(nack);	/* not a nack is an ack */
}

static unsigned char read_byte(int ack){
	int  data;
	int  j;
	/*
	 * Read 8 bits, MSB first.
	 */
	I2C_TRISTATE();
	data = 0;
	for(j = 0; j < 8; j++){
		I2C_SCL(0);
		I2C_DELAY;
		I2C_SCL(1);
		I2C_DELAY;
		data <<= 1;
		if (I2C_READ()==Bit_RESET) {data &=0xFE;} else data |= 1;
		I2C_DELAY;
	}
	send_ack(ack);

	//DEBUG_MSG(I2CSOFT_DEBUG,"read_byte %d\r\n",data);
	return(data);
}


/*=====================================================================*/
/*                         Public Functions                            */
/*=====================================================================*/

void i2c_init (void){
	WaitIfBussy();
	busy=1;
	I2C_INIT();
	I2C_SCL(1);
	I2C_DELAY;
	I2C_SDA(1);
	I2C_TRISTATE();
	send_reset();
	busy=0;
}

int i2c_probe(unsigned char addr){
	WaitIfBussy();
	busy=1;
	int rc;

	//set mode
	SetI2CModeByAddr(addr);

	if((GetI2CMode()!=IRP)&&(GetI2CMode()!=PLC))
		addr=addr>>1;
	send_start();
	rc = write_byte ((addr << 1) | 0);
	send_stop();
	busy=0;
	//I2C_SCL_TRISTATE();
	//printf("i2c_probe rc=%d Addr=%d\r\n",rc,GetI2CMode());
	return (rc ? 1 : 0);
}

/*-----------------------------------------------------------------------
 * Read bytes
 */
int  i2c_read(unsigned char chip, unsigned char Addr, int alen, unsigned char *buffer, int len){
	int shift;
	//WaitIfBussy();
	busy=1;


	if(alen > 0) {

		shift = (alen-1) * 8;
		send_start();
		//I2C_DELAY;

		if(write_byte((chip << 1) | (0))) {	/* write cycle */ //
			send_stop();
			/*PRINTD("i2c_read, no chip responded %02X\n", chip);*/
			DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, no chip addr responded \r\n");
			busy=0;
			return(1);
		}
		if(write_byte(Addr)) {
			DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, address high byte not <ACK>\r\n");
			busy=0;
			return(1);
		}

		I2C_DELAY;
		send_stop();
		DEBUG_MSG(I2CSOFT_DEBUG,"mi2c_read address right\r\n");
		busy=0;
		return(0);
  }

  else {

	  send_start();
	  //I2C_DELAY;
	  //////////////////////////////////////////////////////
	  if(write_byte(((chip << 1))|(1))){
	    send_stop();
	    DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, no chip addr responded \r\n");
	    busy=0;
	    return(1);
	  }
	  for (shift=0;shift<len;shift++)
	   if(shift<len-1)
		   buffer[shift]=read_byte(0);
	   else
		   buffer[shift]=read_byte(1);
	  send_stop();
/////////////////////////////////////////////////
  }
	I2C_DELAY;
	busy=0;
	return(0);
}


uint8_t read_I2C(uint8_t chip){
uint8_t byte;
	//I2C_DELAY;
	send_start();
	write_byte((chip << 1)|(1));
	byte=read_byte(1);
	send_stop();
	return byte;
}
uint8_t write_I2C(uint8_t chip,uint8_t data){
uint8_t byte=0;
	//I2C_DELAY;
	send_start();
	write_byte((chip << 1)|(0));
	write_byte(data);
	send_stop();
	return byte;
}

uint8_t write_byte_reg(uint8_t chip,uint8_t reg,uint8_t data){
uint8_t  nack=0;
	WaitIfBussy();
	busy=1;

	//set mode
	SetI2CModeByAddr(chip);

	if(GetI2CMode()==IRP){
		nack=write_I2C(chip,reg);
		vTaskDelay(50*MSEC);
		nack=write_I2C(chip,data);
		busy=0;
		return 0;
	}
	else{
		//если sfp
		i2c_start();


		if(/*i2c_*/write_byte( chip << 0 | 0 )){
			i2c_stop();
			DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, no chip responded\r\n");
			return 1;
		}
		if(/*i2c_*/write_byte( reg )){
			DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, address not <ACK>ed\r\n");
			return 1;
		}
		/*i2c_*/write_byte( data);
		i2c_stop();
		busy=0;
		return 0;
	}
}


uint8_t read_byte_reg(uint8_t chip,uint8_t reg){
uint8_t tmp=0;
WaitIfBussy();
busy=1;

	//set mode
	SetI2CModeByAddr(chip);

	if(GetI2CMode()==IRP){
		if(chip==IRP_ADDR){
			vTaskDelay(10*MSEC);
			write_I2C(chip,reg);
			vTaskDelay(1*MSEC);
			tmp=read_I2C(chip);
			//DEBUG_MSG(I2CSOFT_DEBUG,"i2c read: reg=%d, val=%d\r\n",reg,tmp);
			vTaskDelay(10*MSEC);
		}
	}
	else{
		if (i2c_read((chip>>1),reg,1,0,0)==0){
			if (i2c_read((chip>>1),0,0,&tmp,1)){
				busy=0;
				return 255;
			}
		}
	}
	busy=0;
	DEBUG_MSG(I2CSOFT_DEBUG,"i2c read: addr=%d, reg=%d, val=%d\r\n",chip,reg,tmp);
	return tmp;
}


/*-----------------------------------------------------------------------
 * Write bytes
 */
int  i2c_write(unsigned char chip, unsigned int addr, int alen, unsigned char *buffer, int len)
{
WaitIfBussy();

	//set mode
	SetI2CModeByAddr(chip);

	int shift, failures = 0;

	DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write: chip %02X addr %02X alen %d buffer[0] %d len %d\r\n",
		chip, addr, alen, buffer[0], len);


	send_start();
	if(write_byte(chip << 1)) {	/* write cycle */
		send_stop();
		/*PRINTD("i2c_write, no chip responded %02X\n", chip);*/
		//xUsb_SendString("i2c_write, no chip responded \r\n");
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, no chip responded\r\n");
		busy=0;
		return(1);
	}
	shift = (alen-1) * 8;
	while(alen-- > 0) {
		if(write_byte(addr >> shift)) {
			/*PRINTD("i2c_write, address not <ACK>ed\n");*/
			DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, address not <ACK>ed\r\n");
			busy=0;
			return(1);
		}
		shift -= 8;
	}

	while(len-- > 0) {
		if(write_byte(*buffer++)) {
			failures++;
		}
	}
	send_stop();
	busy=0;
	return(failures);
}



/*-----------------------------------------------------------------------
 * Read a register
 */
unsigned char i2c_reg_read(unsigned char i2c_addr, unsigned char reg){
unsigned char buf;

	//set mode
	SetI2CModeByAddr(i2c_addr);

	send_start();
	//write chip addr
	if(write_byte(i2c_addr << 1 | (0))) {
		send_stop();
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, no chip responded\r\n");
		return 1;
	}
	//write reg addr
	if(write_byte(reg)) {
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, address not <ACK>ed\r\n");
		return(1);
	}
	I2C_DELAY;//?
	send_stop();

	send_start();
	//write chip addr
	if(write_byte(i2c_addr << 1 | (1))) {
		send_stop();
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, no chip responded\r\n");
		return 1;
	}

	//read
	buf=read_byte(1);
	send_stop();

	DEBUG_MSG(I2CSOFT_DEBUG,"i2c_reg_read: chip %02X, addr %02X, data %d\r\n",i2c_addr, reg, buf);

	return buf;
}






void i2c_writebit( uint8_t c )
{
	I2C_ACTIVE();

	if ( c > 0 ) {
        I2C_SDA(1);
    } else {
    	I2C_SDA(0);
    }


    I2C_SCL(1);
    I2C_DELAY


    I2C_SCL(0);
    I2C_DELAY

    if ( c > 0 ) {
        I2C_SDA(0);
    }
    I2C_DELAY
}

//
uint8_t i2c_readbit(void){
uint8_t c;

	I2C_TRISTATE();

    I2C_SDA(1);
    I2C_SCL(1);
    I2C_DELAY

    c = I2C_READ();

    I2C_SCL(0);
    I2C_DELAY


    return  c;
}

// Inits bitbanging port, must be called before using the functions below
//
/*void i2c_init(void)
{
    I2C_SCL(1);
    I2C_SDA(1);
    I2C_DELAY
}*/

// Send a START Condition
//
static void i2c_start(void)
{
	plc_take();//заняли, запретили прерывания
	//I2C_SCL_ACTIVE();

	I2C_SCL(1);
	I2C_ACTIVE();
    I2C_SDA(1);

    I2C_DELAY

    I2C_SDA(0);
    I2C_DELAY

    I2C_SCL(0);
    I2C_DELAY
}


// Send a STOP Condition
//
void i2c_stop(void)
{
	I2C_ACTIVE();
	I2C_SDA(0);
	I2C_DELAY
	I2C_SCL(1);
	I2C_DELAY
    I2C_SDA(1);
    I2C_DELAY

    //I2C_SCL_TRISTATE();
    I2C_TRISTATE();
    plc_release();//освободили, готовы принимать события
}

// write a byte to the I2C slave device
//
static uint8_t i2c_write_byte( uint8_t c )
{
    for ( uint8_t i=0;i<8;i++) {
        i2c_writebit( c & 128 );
        c<<=1;
    }

    return i2c_readbit();
}

// read a byte from the I2C slave device
//
uint8_t i2c_read_byte( uint8_t ack )
{
    uint8_t res = 0;

    for ( uint8_t i=0;i<8;i++) {
        res <<= 1;
        res |= i2c_readbit();
    }

    if ( ack )
        i2c_writebit( 0 );
    else
        i2c_writebit( 1 );

    //I2C_DELAY

    return res;
}



u8 i2c_reg_write(unsigned char i2c_addr, unsigned char reg, unsigned char *val){

	//set mode
	SetI2CModeByAddr(i2c_addr);

	DEBUG_MSG(I2CSOFT_DEBUG,"i2c_reg_write: chip %02X, addr %02X\r\n",	i2c_addr, reg);
	//printf_arr(TYPE_CHAR,val,33);

	i2c_start();
	if(i2c_write_byte( i2c_addr << 1 | 0 )){
		i2c_stop();
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, no chip responded\r\n");
		return 1;
	}

	if(i2c_write_byte( reg )){
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, address not <ACK>ed\r\n");
		return 1;
	}

	for(u8 i=0;i<33;i++){
		if(i2c_write_byte( val[i] )){
			DEBUG_MSG(I2CSOFT_DEBUG,"i2c_write, address not <ACK>ed\r\n");
			break;
		}
		else{
			//printf("%d:%X[%c] ",i,val[i],val[i]);
		}
	}
	//printf("\r\n");
	if(I2CSOFT_DEBUG)
		printf_arr(TYPE_CHAR,val,33);
    i2c_stop();
    return 0;
}


int i2c_buf_read(unsigned char i2c_addr, unsigned char reg, unsigned char *val, u8 len){

	//set mode
	SetI2CModeByAddr(i2c_addr);

	DEBUG_MSG(I2CSOFT_DEBUG,"i2c_reg_read: chip %02X, addr %02X\r\n",i2c_addr, reg);
	i2c_start();
	if(i2c_write_byte( i2c_addr << 1 | 0 )){
		i2c_stop();
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, no chip responded\r\n");
		return -1;
	}

	if(i2c_write_byte( reg )){
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, address not <ACK>ed\r\n");
		i2c_stop();
		return -1;
	}
	i2c_stop();
	I2C_DELAY
	i2c_start();
	if(i2c_write_byte( i2c_addr << 1 | 1 )){
		i2c_stop();
		DEBUG_MSG(I2CSOFT_DEBUG,"i2c_read, no chip responded\r\n");
		return -1;
	}
	for(u8 i=0;i<len;i++){
		if(i<len-1)
			val[i] = i2c_read_byte(1);//не последняя
		else
			val[i] = i2c_read_byte(0);

		//DEBUG_MSG(I2CSOFT_DEBUG,"%d:%X[%c]\r\n",i,val[i],val[i]);
	}
	if(I2CSOFT_DEBUG)
		printf_arr(TYPE_CHAR,val,len);
    i2c_stop();
    return 0;
}

//настройка прерывания по Scl
void i2c_int_config(void){
	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		//SDA-in
		GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_2;//pd1
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_IRP_GPIO_2, &GPIO_InitStructure);

		EXTI->IMR &= ~EXTI_IMR_MR1;
		EXTI->EMR &= ~EXTI_EMR_MR1;

		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;//RCC_APB2ENR_AFIOEN;
		// Конфигурирование PD1 для внешнего прерывания
		SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI1_PD;
		SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PD;

		// Конфигурирование внешнего прерывания по заднему фронту
		EXTI->RTSR |= EXTI_RTSR_TR1;
		EXTI->FTSR |= EXTI_FTSR_TR1;

		// Установка линии прерывания(EXTI_Line2)
		EXTI->IMR |= EXTI_IMR_MR1;

		// Установка приоритета группы
		NVIC_SetPriority(EXTI1_IRQn, 15);
		// Разрешение прерывания порты 1
		NVIC_EnableIRQ(EXTI1_IRQn);
	}
	else{
		//sda
		GPIO_InitStructure.GPIO_Pin = LINE_IRP_SDA_PIN_1;//6
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(LINE_IRP_GPIO_1, &GPIO_InitStructure);//pc

		EXTI->IMR &= ~EXTI_IMR_MR6;
		EXTI->EMR &= ~EXTI_EMR_MR6;

		//RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;//RCC_APB2ENR_AFIOEN;
		// Конфигурирование PC7 для внешнего прерывания
		SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR2_EXTI6_PC;
		SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI6_PC;

		// Конфигурирование внешнего прерывания по заднему фронту
		EXTI->FTSR |= EXTI_FTSR_TR6;

		// Установка линии прерывания(EXTI_Line6)
		EXTI->IMR |= EXTI_IMR_MR6;

		// Установка приоритета группы
		NVIC_SetPriority(EXTI9_5_IRQn, 15);
		// Разрешение прерывания порты 5-9
		NVIC_EnableIRQ(EXTI9_5_IRQn);

	}
}

//разрешить прерывание по SDA
void i2c_sda_sei(void){

}
//запрещение прерывания по SDA
void i2c_sda_cli(){

}
