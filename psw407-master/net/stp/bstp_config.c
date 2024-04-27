#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "stm32f10x_lib.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#include "bstp.h"
#include "board.h"
#include "settings.h"
#ifdef USE_STP



void bstp_cfg_reset(void){
  //bstp_port_cfg_t port_cfg;//my
  //uint32_t CompRSTP;

	set_stp_magic(BSTP_CFG_MAGIC);//bstp_cfg.magic = BSTP_CFG_MAGIC;
	set_stp_proto(BSTP_PROTO_RSTP);//bstp_cfg.proto = BSTP_PROTO_RSTP;
	set_stp_bridge_priority(BSTP_DEFAULT_BRIDGE_PRIORITY);//bstp_cfg.bridge_priority = BSTP_DEFAULT_BRIDGE_PRIORITY;
	set_stp_bridge_max_age(BSTP_DEFAULT_MAX_AGE / BSTP_TICK_VAL);//	bstp_cfg.bridge_max_age = BSTP_DEFAULT_MAX_AGE / BSTP_TICK_VAL;
	set_stp_bridge_htime(BSTP_DEFAULT_HELLO_TIME / BSTP_TICK_VAL);//bstp_cfg.bridge_htime = BSTP_DEFAULT_HELLO_TIME / BSTP_TICK_VAL;
	set_stp_bridge_fdelay(BSTP_DEFAULT_FORWARD_DELAY / BSTP_TICK_VAL);//bstp_cfg.bridge_fdelay = BSTP_DEFAULT_FORWARD_DELAY / BSTP_TICK_VAL;
	set_stp_txholdcount(BSTP_DEFAULT_HOLD_COUNT);//bstp_cfg.txholdcount = BSTP_DEFAULT_HOLD_COUNT;
	set_stp_bridge_mdelay(BSTP_DEFAULT_MIGRATE_DELAY / BSTP_TICK_VAL);//bstp_cfg.bridge_mdelay = BSTP_DEFAULT_MIGRATE_DELAY / BSTP_TICK_VAL;




	//port_cfg.state = 1;
	//port_cfg.flags = 0;
	//port_cfg.flags = BSTP_PORTCFG_FLAG_AUTOEDGE | BSTP_PORTCFG_FLAG_AUTOPTP;
	//port_cfg.priority = BSTP_DEFAULT_PORT_PRIORITY;

	for(int i=0;i<NUM_BSTP_PORT;i++){
		if((i==GE1)||(i==GE2))
			set_stp_port_cost(i,BSTP_DEFAULT_PATH_COST/10);
		else
			set_stp_port_cost(i,BSTP_DEFAULT_PATH_COST);

		set_stp_port_enable(i,1);
		set_stp_port_state(i,1);
		set_stp_port_priority(i,BSTP_DEFAULT_PORT_PRIORITY);
		set_stp_port_autoedge(i,1);
		set_stp_port_autoptp(i,1);
	}

	bstp_sem_free();
}

#endif /*USE_STP*/
