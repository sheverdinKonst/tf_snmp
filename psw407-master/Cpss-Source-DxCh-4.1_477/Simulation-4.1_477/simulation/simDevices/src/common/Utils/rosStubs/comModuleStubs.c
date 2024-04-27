/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* comModuleStubs.c
*
* DESCRIPTION:
*       This is API stub for the CM simulation.
*       When using there is no need for CM libraries.
*       Implementation is only for the function that
*       the simulation calls.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/

#include <comModule/CSClient_C.h>
#include <comModule/libsub.h>

/* -------------- stubs for CM ------------- */

#ifndef COM_MODULE_USED
    /* ------ Client Registration ------ */

    int  CS_RegisterClient(const char* sClientName){return 0;}
    void CS_UnregisterClient(){return;}

    /* ------ Connect & Disconnect To/From Adapter ------ */

    int  CS_ConnectToAdapter(unsigned long adapterId){return 0;}
    int  CS_DisconnectFromAdapter(unsigned long adapterId){return 0;}

    /* ------ Detected Adapters Description and Status ------ */

    int  CS_GetAdapterStatus(unsigned long adapterId, int* pStatus){return 0;}
    void CS_DetectAdapters(){return;}
    int  CS_GetDetectedAdaptersDescription(int* pNumOfAdapters, AdpDesc** ppAdpDesc){return 0;}

    /* ------ Lock & Unlock Adapters for individual usage ------ */

    int  CS_LockAdapter(unsigned long adapterId){return 0;}
    int  CS_UnlockAdapter(unsigned long adapterId){return 0;}

    /* ------ Transaction ------ */

    int  CS_StartTransaction( unsigned long adapterId){return 0;}
    int  CS_StopTransaction( unsigned long adapterId){return 0;}

    /* ------ SMI ------ */

    int  CS_WriteSMI( unsigned long adapterId,  unsigned long phyAddress,  unsigned long offset,  unsigned long data){return 0;}
    int  CS_ReadSMI( unsigned long adapterId,  unsigned long phyAddress,  unsigned long offset, unsigned long* pData){return 0;}

    int  CS_SetSMIConfigParams( unsigned long adapterId,  unsigned long frequency,  unsigned char bSlowMode){return 0;}
    int  CS_GetSMIConfigParams( unsigned long adapterId, unsigned long* pFrequency, unsigned char* pbSlowMode){return 0;}

    /* ------ I2C ------ */

    int  CS_WriteI2C(unsigned long adapterId,  char addr,  unsigned short nBytes, unsigned char* data,  unsigned char sendStop){return 0;}
    int  CS_ReadI2C( unsigned long adapterId,  char addr,  unsigned short nBytes, unsigned char** data,  unsigned char sendStop){return 0;}

    int  CS_SetI2CConfigParams( unsigned long adapterId, unsigned long frequency){return 0;}
    int  CS_GetI2CConfigParams( unsigned long adapterId, unsigned long* pFrequency){return 0;}

    /* ------ Serial ------ */

    int  CS_WriteSerial( unsigned long adapterId, unsigned char* sData){return 0;}
    int  CS_ReadSerial( unsigned long adapterId, unsigned char** sData){return 0;}

    int  CS_SetSerialConfigParams( unsigned long adapterId, unsigned long maxSpeed,  unsigned long parity,  unsigned long dataBits,  float stopBits){return 0;}
    int  CS_GetSerialConfigParams( unsigned long adapterId, unsigned long* pMaxSpeed, unsigned long* pParity, unsigned long* pDataBits, float* pStopBits){return 0;}

    /* ------ Loopback ------ */

    int  CS_WriteLoopback( unsigned long adapterId, unsigned char* sData){return 0;}
    int  CS_ReadLoopback( unsigned long adapterId, unsigned char** sData){return 0;}

    int  CS_SetLoopbackConfigParams( unsigned long adapterId, unsigned long speed){return 0;}
    int  CS_GetLoopbackConfigParams( unsigned long adapterId, unsigned long* pSpeed){return 0;}

    /* ------ Ethernet ------ */

    int  CS_ClientWriteEthernet( unsigned long adapterId, unsigned char* sData){return 0;}
    int  CS_ClientReadEthernet( unsigned long adapterId, unsigned char** sData){return 0;}

    int  CS_ClientSetEthernetConfigParams( unsigned long adapterId,  unsigned char* sIpAddress,  unsigned long portNumber,  unsigned long protocol){return 0;}
    int  CS_ClientGetEthernetConfigParams( unsigned long adapterId, unsigned char** sIpAddress, unsigned long* pPortNumber, unsigned long* pProtocol){return 0;}

#endif

/* -------------- stubs for SUB-20 ------------- */
#ifdef STUB_SUB20_USED
int  sub_errno = 0;
int  sub_i2c_status = 0;

int sub_get_mode( sub_handle hndl, int *boot ) {return 0;}

/*debugging */
int sub_set_debug_level( int level ){return 0;}

sub_device sub_find_devices( sub_device first ){return 0;}
sub_handle sub_open( sub_device dev ){return 0;}
int sub_close( sub_handle hndl ){return 0;}
const struct sub_version* sub_get_version( sub_handle hndl ){return 0;}
const struct sub_cfg_vpd* sub_get_cfg_vpd( sub_handle hndl ){return 0;}

/* I2C */
int sub_i2c_freq( sub_handle hndl, sub_int32_t *freq ){return 0;}
int sub_i2c_config( sub_handle hndl, int sa, int flags ){return 0;}
int sub_i2c_start( sub_handle hndl ){return 0;}
int sub_i2c_stop( sub_handle hndl ){return 0;}
int sub_i2c_scan( sub_handle hndl, int* slave_cnt, char* slave_buf ){return 0;}
int sub_i2c_read( sub_handle hndl, int sa, int ma, int ma_sz, char* buf, int sz ){return 0;}
int sub_i2c_write( sub_handle hndl, int sa, int ma, int ma_sz, char* buf, int sz ){return 0;}

/* SPI Master */
int sub_spi_config( sub_handle hndl, int cfg_set, int* cfg_get ){return 0;}
int sub_spi_transfer( sub_handle hndl, char* out_buf, char* in_buf, int sz, int ss_config ){return 0;}
int sub_sdio_transfer( sub_handle hndl, char* out_buf, char* in_buf, int out_sz, int in_sz, int ss_config ){return 0;}

/* MDIO */
int sub_mdio22( sub_handle hndl, int op, int phyad, int regad, int data, int* content ){return 0;}
int sub_mdio45( sub_handle hndl, int op, int prtad, int devad, int data, int* content ){return 0;}
int sub_mdio_xfer( sub_handle hndl, int count, union sub_mdio_frame* mdios ){return 0;}

/* GPIO */
int sub_gpio_config( sub_handle hndl, sub_int32_t set, sub_int32_t* get, sub_int32_t mask ){return 0;}
int sub_gpio_read( sub_handle hndl, sub_int32_t* get ){return 0;}
int sub_gpio_write( sub_handle hndl, sub_int32_t set, sub_int32_t* get, sub_int32_t mask ){return 0;}

/* Fast PWM */
int sub_fpwm_config( sub_handle hndl, double freq_hz, int flags ){return 0;}
int sub_fpwm_set( sub_handle hndl, int pwm_n, double duty ){return 0;}

/* PWM */
int sub_pwm_config( sub_handle hndl, int res_us, int limit ){return 0;}
int sub_pwm_set( sub_handle hndl, int pwm_n, int duty ){return 0;}


/* ADC */
int sub_adc_config( sub_handle hndl, int flags ){return 0;}
int sub_adc_single( sub_handle hndl, int* data, int mux ){return 0;}
int sub_adc_read( sub_handle hndl, int* data, int* mux, int reads ){return 0;}

/* LCD */
int sub_lcd_write( sub_handle hndl, char* str ){return 0;}

/* RS232/RS485 */
int sub_rs_set_config( sub_handle hndl, int config, int baud ){return 0;}
int sub_rs_get_config( sub_handle hndl, int* config, int* baud ){return 0;}
int sub_rs_timing( sub_handle hndl,
                int flags, int tx_space_us, int rx_msg_us, int rx_byte_us ){return 0;}
int sub_rs_xfer( sub_handle hndl,
                char* tx_buf, int tx_sz, char* rx_buf, int rx_sz ){return 0;}

/* FIFO */
int sub_fifo_write( sub_handle hndl, char* buf, int sz, int to_ms ){return 0;}
int sub_fifo_read( sub_handle hndl, char* buf, int sz, int to_ms ){return 0;}
int sub_fifo_config( sub_handle hndl, int config ){return 0;}

/* USB */
int usb_transaction( sub_handle hndl,
                             char* out_buf, int out_sz,
                             char*  in_buf, int in_sz,
                             int timeout ){return 0;}

/* product identification */
int sub_control_request( sub_handle hndl,
                            int type, int request, int value, int index,
                            char* buf, int sz, int timeout
                        ){return 0;}
int sub_get_serial_number( sub_handle hndl, char *buf, int sz){return 0;}
int sub_get_product_id( sub_handle hndl, char *buf, int sz){return 0;}


char* sub_strerror( int errnum ){return 0;}

#endif /*NO_SUB20_USED*/
