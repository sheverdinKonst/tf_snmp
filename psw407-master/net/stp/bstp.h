#ifndef __STP_H__
#define __STP_H__

//#define BRIDGESTP_DEBUG

//#include "settings.h"
#include "deffines.h"
#define NUM_BSTP_PORT PORT_NUM

#include "bstp_config.h"
#include "bridgestp.h"
#include "stp_oslayer.h"

#define DSA_TAG_PORT(a) ((HTONL(a)&0x00f80000)>>19)

void bstp_main_task(void *pvParameters);

#endif
