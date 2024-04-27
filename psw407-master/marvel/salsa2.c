#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "SMIApi.h"
#include "Salsa2Regs.h"
#include "salsa2.h"
#include "board.h"
#include "settings.h"
#include "deffines.h"
#include "debug.h"

//extern u8 dev_addr[6];

u8 get_port_phy_addr(u8 port){
	switch(port){
		case 0:return  0x04;
		case 1:return  0x05;
		case 2:return  0x06;
		case 3:return  0x07;
		case 4:return  0x08;
		case 5:return  0x09;
		case 6:return  0x0A;
		case 7:return  0x0B;
		case 8:return  0x0C;
		case 9:return  0x0D;
		case 10:return 0x0E;
		case 11:return 0x0F;
		case 12:return 0x04;
		case 13:return 0x05;
		case 14:return 0x06;
		case 15:return 0x07;
	}
	return 0;
}

void salsa2_config_processing(void){
u16 tmp_reg;
//vTaskDelay(2000*MSEC);

printf("config start\r\n\r\n");
//vTaskDelay(200*MSEC);

//	//LED CONTROL
//	for(u8 i=0;i<12;i++){
//		Salsa2_WritePhyReg(i,i,22,3);
//		Salsa2_WritePhyReg(i,i,16,1);
//	}
//
//	Salsa2_WritePhyReg(12,4,22,3);
//	Salsa2_WritePhyReg(12,4,16,1);
//
//	Salsa2_WritePhyReg(13,5,22,3);
//	Salsa2_WritePhyReg(13,5,16,1);
//
//	Salsa2_WritePhyReg(14,6,22,3);
//	Salsa2_WritePhyReg(14,6,16,1);
//
//	Salsa2_WritePhyReg(15,7,22,3);
//	Salsa2_WritePhyReg(15,7,16,1);

	//set_link_indication_mode(LINK_FORCED_ON);

	//0
	/*Salsa2_configPhyAddres(0,0x00);
	Salsa2_configPhyAddres(1,0x01);
	Salsa2_configPhyAddres(2,0x02);
	Salsa2_configPhyAddres(3,0x03);
	//1
	Salsa2_configPhyAddres(4,0x04);
	Salsa2_configPhyAddres(5,0x05);
	Salsa2_configPhyAddres(6,0x06);
	Salsa2_configPhyAddres(7,0x07);
	//2
	Salsa2_configPhyAddres(8,0x08);
	Salsa2_configPhyAddres(9,0x09);
	Salsa2_configPhyAddres(10,0x0A);
	Salsa2_configPhyAddres(11,0x0B);

	//3
	Salsa2_configPhyAddres(12,0x04);
	Salsa2_configPhyAddres(13,0x05);
	Salsa2_configPhyAddres(14,0x06);
	Salsa2_configPhyAddres(15,0x07);*/






	//all port reset
	for(u8 i=0;i<PORT_NUM;i++){
		Salsa2_WriteReg(0x10000008+0x400*i,0x0000C048);
	}



	//As EEPROM start processing
	Salsa2_WriteReg(0x00000000,0x0000600A);	// Disable Device Number 0


	Salsa2_WriteReg(0x04004200,0x00070000);// Change M_SMI_0 Configuration to non inverse mode, set SMI0 fast_mdc to div/64
	Salsa2_WriteReg(0x05004200,0x00070000);// Change M_SMI_1 Configuration to non inverse mode, set SMI1 fast_mdc to div/64
//	Salsa2_WriteReg(0x000000D4,0x0804CCCC);// Set M_SMI pad to 1.8V //у нас 3,3
	Salsa2_WriteReg(0x02040000,0x09000019);// Disable NA messages
	Salsa2_WriteReg(0x03000004,0x0000000E);// Disable BM aging

	Salsa2_WriteReg(0x06000000,0x00006261);// Assign strict priority to management accesses to FDB over the datapath and disable Age Recycle

	Salsa2_WriteReg(0x10000014,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10000414,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10000814,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10000C14,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10001014,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10001414,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10001814,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10001C14,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10002014,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10002414,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10002814,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10002C14,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10003014,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10003414,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10003814,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode
	Salsa2_WriteReg(0x10003C14,0x000008D0);// Port Serial Parameters Configuration Register change to avoid port hang in half-duplex mode

	Salsa2_WriteReg(0x000000E4,0x0000CCC9);// Set QSGMII bypass Configuration #qsgmii_bypass_en == 0 ?

	// Power up 5G SERDESs
	// Set SERDES ref clock register
	Salsa2_WriteReg(0x18000000,0x00003E80);
	Salsa2_WriteReg(0x18001000,0x00003E80);
	Salsa2_WriteReg(0x18002000,0x00003E80);
	Salsa2_WriteReg(0x18003000,0x00003E80);

	// Wait 10mSec
	vTaskDelay(10*MSEC);


	// Deactivate sd_reset //SDResetIn = 1
	Salsa2_WriteReg(0x18000004,0x00000008);
	Salsa2_WriteReg(0x18001004,0x00000008);
	Salsa2_WriteReg(0x18002004,0x00000008);
	Salsa2_WriteReg(0x18003004,0x00000008);

	// Wait for Calibration done (0x09800008 bit 3)
	vTaskDelay(10*MSEC);

	// Reference Ferquency select = 62.5MHz   ;Use registers bits to control speed configuration
	Salsa2_WriteReg(0x1800020C,0x0000800A);
	Salsa2_WriteReg(0x1800120C,0x0000800A);
	Salsa2_WriteReg(0x1800220C,0x0000800A);
	Salsa2_WriteReg(0x1800320C,0x0000800A);

	// Transmitter/Reciver Divider force, interpulator force; 1.25G: intpi = 25uA ,
	//VCO divided by 4 ; 2.5G: intpi = 25uA , VCO divided by 2  ;
	//3.125G: intpi = 30uA , VCO divided by 2 ; 3.75G: intpi = 20uA ,
	//VCO not divided; 6.25G: intpi = 30uA , VCO not divided; 5.15G: intpi = 25uA , VCO not divided
	Salsa2_WriteReg(0x18000210,0x00004414);
	Salsa2_WriteReg(0x18001210,0x00004414);
	Salsa2_WriteReg(0x18002210,0x00004414);
	Salsa2_WriteReg(0x18003210,0x00004414);

	// Force FbDiv/RfDiv
	Salsa2_WriteReg(0x18000214,0x0000A150);
	Salsa2_WriteReg(0x18001214,0x0000A150);
	Salsa2_WriteReg(0x18002214,0x0000A150);
	Salsa2_WriteReg(0x18003214,0x0000A150);

	// Force: PLL Speed, sel_v2i, loadcap_pll,sel_fplres
	Salsa2_WriteReg(0x18000218,0x0000BAAB);
	Salsa2_WriteReg(0x18001218,0x0000BAAB);
	Salsa2_WriteReg(0x18002218,0x0000BAAB);
	Salsa2_WriteReg(0x18003218,0x0000BAAB);

	// icp force
	Salsa2_WriteReg(0x1800021C,0x0000882C);
	Salsa2_WriteReg(0x1800121C,0x0000882C);
	Salsa2_WriteReg(0x1800221C,0x0000882C);
	Salsa2_WriteReg(0x1800321C,0x0000882C);

	//  0 = kvco-2
	Salsa2_WriteReg(0x180003CC,0x00002000);
	Salsa2_WriteReg(0x180013CC,0x00002000);
	Salsa2_WriteReg(0x180023CC,0x00002000);
	Salsa2_WriteReg(0x180033CC,0x00002000);

	// External TX/Rx Impedance changed from 6 to 0 while auto calibration results are used  -
	//based on lab measurments it seems that we need to force the auto imedance calibration values
	Salsa2_WriteReg(0x1800022C,0x00000000);
	Salsa2_WriteReg(0x1800122C,0x00000000);
	Salsa2_WriteReg(0x1800222C,0x00000000);
	Salsa2_WriteReg(0x1800322C,0x00000000);

	// Auto KVCO,  PLL is not forced to max speed during power up sequence -
	Salsa2_WriteReg(0x18000230,0x00000000);
	Salsa2_WriteReg(0x18001230,0x00000000);
	Salsa2_WriteReg(0x18002230,0x00000000);
	Salsa2_WriteReg(0x18003230,0x00000000);

	// Sampler OS Scale was changed from 5mV/Step to 3.3mV/Step; RX_IMP_VTHIMCAL was chnge from 3 to 0
	Salsa2_WriteReg(0x18000234,0x00004000);
	Salsa2_WriteReg(0x18001234,0x00004000);
	//Salsa2_WriteReg(0x18001234,0x00000000);//set 5mV/Step
	Salsa2_WriteReg(0x18002234,0x00004000);
	Salsa2_WriteReg(0x18003234,0x00004000);

	// Use value wiritten to register for process calibration instead of th eauto calibration;
	//Select process from register
	Salsa2_WriteReg(0x1800023C,0x00000018);
	Salsa2_WriteReg(0x1800123C,0x00000018);
	Salsa2_WriteReg(0x1800223C,0x00000018);
	Salsa2_WriteReg(0x1800323C,0x00000018);

	// DCC should be dissabled at baud 3.125 and below = 8060
	Salsa2_WriteReg(0x18000250,0x0000A0C0);
	Salsa2_WriteReg(0x18001250,0x0000A0C0);
	Salsa2_WriteReg(0x18002250,0x0000A0C0);
	Salsa2_WriteReg(0x18003250,0x0000A0C0);

	//wait 5ms
	vTaskDelay(5*MSEC);

	// DCC should be dissabled at baud 3.125 and below = 8060
	Salsa2_WriteReg(0x18000250,0x0000A060);
	Salsa2_WriteReg(0x18001250,0x0000A060);
	Salsa2_WriteReg(0x18002250,0x0000A060);
	Salsa2_WriteReg(0x18003250,0x0000A060);

	// PE Setting
	Salsa2_WriteReg(0x18000254,0x00005503);
	Salsa2_WriteReg(0x18001254,0x00005503);
	Salsa2_WriteReg(0x18002254,0x00005503);
	Salsa2_WriteReg(0x18003254,0x00005503);

	// PE Type
	Salsa2_WriteReg(0x18000258,0x00000000);
	Salsa2_WriteReg(0x18001258,0x00000000);
	Salsa2_WriteReg(0x18002258,0x00000000);
	Salsa2_WriteReg(0x18003258,0x00000000);

	// selmupi/mupf - low value for lower baud
	Salsa2_WriteReg(0x1800027C,0x000090AA);
	Salsa2_WriteReg(0x1800127C,0x000090AA);
	Salsa2_WriteReg(0x1800227C,0x000090AA);
	Salsa2_WriteReg(0x1800327C,0x000090AA);

	// DTL_FLOOP_EN = Dis
	Salsa2_WriteReg(0x18000280,0x00000800);
	Salsa2_WriteReg(0x18001280,0x00000800);
	Salsa2_WriteReg(0x18002280,0x00000800);
	Salsa2_WriteReg(0x18003280,0x00000800);

	// FFE Setting
	Salsa2_WriteReg(0x1800028C,0x00000344);
	Salsa2_WriteReg(0x1800128C,0x00000344);
	Salsa2_WriteReg(0x1800228C,0x00000344);
	Salsa2_WriteReg(0x1800328C,0x00000344);

	// Slicer Enable; Tx  Imp was changed from 50ohm to 43ohm
	Salsa2_WriteReg(0x1800035C,0x0000423F);
	Salsa2_WriteReg(0x1800135C,0x0000423F);
	Salsa2_WriteReg(0x1800235C,0x0000423F);
	Salsa2_WriteReg(0x1800335C,0x0000423F);

	// Not need to be configure - Same as default
	Salsa2_WriteReg(0x18000364,0x00005555);
	Salsa2_WriteReg(0x18001364,0x00005555);
	Salsa2_WriteReg(0x18002364,0x00005555);
	Salsa2_WriteReg(0x18003364,0x00005555);

	// Disable ana_clk_det
	Salsa2_WriteReg(0x1800036C,0x00000000);
	Salsa2_WriteReg(0x1800136C,0x00000000);
	Salsa2_WriteReg(0x1800236C,0x00000000);
	Salsa2_WriteReg(0x1800336C,0x00000000);

	// Configure rx_imp_vthimpcal to 0x0 (default value = 0x3);
	//Configure Sampler_os_scale to 3.3mV/step (default value = 5mV/step)
	Salsa2_WriteReg(0x18000234,0x00004000);
	Salsa2_WriteReg(0x18001234,0x00004000);//--old
//Salsa2_WriteReg(0x18001234,0x00000000);//--new
	Salsa2_WriteReg(0x18002234,0x00004000);
	Salsa2_WriteReg(0x18003234,0x00004000);

	//Configure IMP_VTHIMPCAL to 56.7ohm (default value = 53.3 ohm);
	//Configure cal_os_ph_rd to 0x60 (default value = 0x0);
	//Configure Cal_rxclkalign90_ext to use an external ovride value
	Salsa2_WriteReg(0x18000228,0x0000E0C0);
	Salsa2_WriteReg(0x18001228,0x0000E0C0);
	//Salsa2_WriteReg(0x18001228,0x0000E0A0);//Configure IMP_VTHIMPCAL to 50.0ohm
	Salsa2_WriteReg(0x18002228,0x0000E0C0);
	Salsa2_WriteReg(0x18003228,0x0000E0C0);

	// Reset dtl_rx ; Enable ana_clk_det
	Salsa2_WriteReg(0x1800036C,0x00008040);
	Salsa2_WriteReg(0x1800136C,0x00008040);
	Salsa2_WriteReg(0x1800236C,0x00008040);
	Salsa2_WriteReg(0x1800336C,0x00008040);

	// Un reset dtl_rx
	Salsa2_WriteReg(0x1800036C,0x00008000);
	Salsa2_WriteReg(0x1800136C,0x00008000);
	Salsa2_WriteReg(0x1800236C,0x00008000);
	Salsa2_WriteReg(0x1800336C,0x00008000);

	//wait 10ms
	vTaskDelay(10*MSEC);

	Salsa2_WriteReg(0x18000224,0x00000000);
	Salsa2_WriteReg(0x18001224,0x00000000);
	Salsa2_WriteReg(0x18002224,0x00000000);
	Salsa2_WriteReg(0x18003224,0x00000000);

	// CAL Start
	Salsa2_WriteReg(0x18000224,0x00008000);
	Salsa2_WriteReg(0x18001224,0x00008000);
	Salsa2_WriteReg(0x18002224,0x00008000);
	Salsa2_WriteReg(0x18003224,0x00008000);

	Salsa2_WriteReg(0x18000224,0x00000000);
	Salsa2_WriteReg(0x18001224,0x00000000);
	Salsa2_WriteReg(0x18002224,0x00000000);
	Salsa2_WriteReg(0x18003224,0x00000000);

	// Wait for RxClk_x2
	vTaskDelay(10*MSEC);

	// Set RxInit to 0x1 (remember that bit 3 is already set to 0x1)
	Salsa2_WriteReg(0x18000004,0x00000018);
	Salsa2_WriteReg(0x18001004,0x00000018);
	Salsa2_WriteReg(0x18002004,0x00000018);
	Salsa2_WriteReg(0x18003004,0x00000018);

	// Wait for p_clk = 1 and p_clk = 0
	vTaskDelay(10*MSEC);

	// Set RxInit to 0x0
	Salsa2_WriteReg(0x18000004,0x00000008);
	Salsa2_WriteReg(0x18001004,0x00000008);
	Salsa2_WriteReg(0x18002004,0x00000008);
	Salsa2_WriteReg(0x18003004,0x00000008);

	// Wait for ALL PHY_RDY = 1 (0x09800008 bit 0)
	vTaskDelay(10*MSEC);

	// RFResetIn
	Salsa2_WriteReg(0x18000004,0x00000028);
	Salsa2_WriteReg(0x18001004,0x00000028);
	Salsa2_WriteReg(0x18002004,0x00000028);
	Salsa2_WriteReg(0x18003004,0x00000028);

	vTaskDelay(10*MSEC);

	// Mac unreset;
	// In-band Auto-Negotiation mode
	Salsa2_WriteReg(0x10000008,0x0000C008);
	Salsa2_WriteReg(0x10000408,0x0000C008);
	Salsa2_WriteReg(0x10000808,0x0000C008);
	Salsa2_WriteReg(0x10000C08,0x0000C008);
	Salsa2_WriteReg(0x10001008,0x0000C008);
	Salsa2_WriteReg(0x10001408,0x0000C008);
	Salsa2_WriteReg(0x10001808,0x0000C008);
	Salsa2_WriteReg(0x10001C08,0x0000C008);
	Salsa2_WriteReg(0x10002008,0x0000C008);
	Salsa2_WriteReg(0x10002408,0x0000C008);
	Salsa2_WriteReg(0x10002808,0x0000C008);
	Salsa2_WriteReg(0x10002C08,0x0000C008);
	Salsa2_WriteReg(0x10003008,0x0000C008);
	Salsa2_WriteReg(0x10003408,0x0000C008);
	Salsa2_WriteReg(0x10003808,0x0000C008);
	Salsa2_WriteReg(0x10003C08,0x0000C008);


	//
	// Configuring PHY - 0
	//
	//led mode
	//0
	Salsa2_WriteReg(0x04004054,0x02C40003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x02040001);//set 1

	//1
	Salsa2_WriteReg(0x04004054,0x02C50003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x02050001);//set 1

	//2
	Salsa2_WriteReg(0x04004054,0x02C60003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x02060001);//set 1
	//3
	Salsa2_WriteReg(0x04004054,0x02C70003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x02070001);//set 1

	// 0 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02C400FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x01641C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02C40000);// Set Page #0
	// 1 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02C500FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x01651C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02C50000);// Set Page #0
	// 2 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02C600FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x01661C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02C60000);// Set Page #0
	// 3 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02C700FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x01671C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02C70000);// Set Page #0
	// 0 - Configure PHY to Cut Through Mode
	Salsa2_WriteReg(0x04004054,0x02C40010);// Set Page #16
	Salsa2_WriteReg(0x04004054,0x00240070);// Write to address 0x70
	Salsa2_WriteReg(0x04004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x00240870);// Write to address 0x870
	Salsa2_WriteReg(0x04004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x00241070);// Write to address 0x1070
	Salsa2_WriteReg(0x04004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x00241870); // Write to address 0x1870
	Salsa2_WriteReg(0x04004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x02C40000);// Set Page #0
	// Configure PHY to EEE Master Mode
	// Enable EEE AutoNeg
	Salsa2_WriteReg(0x04004054,0x02C00000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01A00007);
	Salsa2_WriteReg(0x04004054,0x01C0003C);
	Salsa2_WriteReg(0x04004054,0x01A04007);
	Salsa2_WriteReg(0x04004054,0x01C00006);
	Salsa2_WriteReg(0x04004054,0x02C10000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01A10007);
	Salsa2_WriteReg(0x04004054,0x01C1003C);
	Salsa2_WriteReg(0x04004054,0x01A14007);
	Salsa2_WriteReg(0x04004054,0x01C10006);
	Salsa2_WriteReg(0x04004054,0x02C20000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01A20007);
	Salsa2_WriteReg(0x04004054,0x01C2003C);
	Salsa2_WriteReg(0x04004054,0x01A24007);
	Salsa2_WriteReg(0x04004054,0x01C20006);
	Salsa2_WriteReg(0x04004054,0x02C30000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01A30007);
	Salsa2_WriteReg(0x04004054,0x01C3003C);
	Salsa2_WriteReg(0x04004054,0x01A34007);
	Salsa2_WriteReg(0x04004054,0x01C30006);
	// Enable EEE Master mode
	Salsa2_WriteReg(0x04004054,0x02C00010);// Change to page 16
	Salsa2_WriteReg(0x04004054,0x002003C1);
	Salsa2_WriteReg(0x04004054,0x00400001);
	Salsa2_WriteReg(0x04004054,0x00600000);
	Salsa2_WriteReg(0x04004054,0x00200BC1);
	Salsa2_WriteReg(0x04004054,0x00400001);
	Salsa2_WriteReg(0x04004054,0x00600000);
	Salsa2_WriteReg(0x04004054,0x002013C1);
	Salsa2_WriteReg(0x04004054,0x00400001);
	Salsa2_WriteReg(0x04004054,0x00600000);
	Salsa2_WriteReg(0x04004054,0x00201BC1);
	Salsa2_WriteReg(0x04004054,0x00400001);
	Salsa2_WriteReg(0x04004054,0x00600000);
	Salsa2_WriteReg(0x04004054,0x02C00000);// Change to page 0

	//
	// Configuring PHY - 1
	//
	//led mode
	//8
	Salsa2_WriteReg(0x04004054,0x02C80003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x02080001);//set 1

	//9
	Salsa2_WriteReg(0x04004054,0x02C90003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x02090001);//set 1

	//A
	Salsa2_WriteReg(0x04004054,0x02CA0003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x020A0001);//set 1
	//B
	Salsa2_WriteReg(0x04004054,0x02CB0003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x020B0001);//set 1

	Salsa2_WriteReg(0x04004054,0x02C80000);// Set Page #0
	// 8 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02C800FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x01681C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02C80000);// Set Page #0
	// 9 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02C900FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x01691C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02C90000);// Set Page #0
	// A - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02CA00FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x016A1C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02CA0000);// Set Page #0
	// B - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02CB00FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x016B1C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02CB0000);// Set Page #0
	// 8 - Configure PHY to Cut Through Mode
	Salsa2_WriteReg(0x04004054,0x02C80010);// Set Page #16
	Salsa2_WriteReg(0x04004054,0x00280070);// Write to address 0x70
	Salsa2_WriteReg(0x04004054,0x00480040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00680000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x00280870);// Write to address 0x870
	Salsa2_WriteReg(0x04004054,0x00480040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00680000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x00281070);// Write to address 0x1070
	Salsa2_WriteReg(0x04004054,0x00480040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00680000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x00281870); // Write to address 0x1870
	Salsa2_WriteReg(0x04004054,0x00480040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x00680000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x02C80000);// Set Page #0
	// Configure PHY to EEE Master Mode
	// Enable EEE AutoNeg
	Salsa2_WriteReg(0x04004054,0x02C80000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01A80007);
	Salsa2_WriteReg(0x04004054,0x01C8003C);
	Salsa2_WriteReg(0x04004054,0x01A84007);
	Salsa2_WriteReg(0x04004054,0x01C80006);
	Salsa2_WriteReg(0x04004054,0x02C90000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01A90007);
	Salsa2_WriteReg(0x04004054,0x01C9003C);
	Salsa2_WriteReg(0x04004054,0x01A94007);
	Salsa2_WriteReg(0x04004054,0x01C90006);
	Salsa2_WriteReg(0x04004054,0x02CA0000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01AA0007);
	Salsa2_WriteReg(0x04004054,0x01CA003C);
	Salsa2_WriteReg(0x04004054,0x01AA4007);
	Salsa2_WriteReg(0x04004054,0x01CA0006);
	Salsa2_WriteReg(0x04004054,0x02CB0000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01AB0007);
	Salsa2_WriteReg(0x04004054,0x01CB003C);
	Salsa2_WriteReg(0x04004054,0x01AB4007);
	Salsa2_WriteReg(0x04004054,0x01CB0006);
	// Enable EEE Master mode
	Salsa2_WriteReg(0x04004054,0x02C80010);// Change to page 16
	Salsa2_WriteReg(0x04004054,0x002803C1);
	Salsa2_WriteReg(0x04004054,0x00480001);
	Salsa2_WriteReg(0x04004054,0x00680000);
	Salsa2_WriteReg(0x04004054,0x00280BC1);
	Salsa2_WriteReg(0x04004054,0x00480001);
	Salsa2_WriteReg(0x04004054,0x00680000);
	Salsa2_WriteReg(0x04004054,0x002813C1);
	Salsa2_WriteReg(0x04004054,0x00480001);
	Salsa2_WriteReg(0x04004054,0x00680000);
	Salsa2_WriteReg(0x04004054,0x00281BC1);
	Salsa2_WriteReg(0x04004054,0x00480001);
	Salsa2_WriteReg(0x04004054,0x00680000);
	Salsa2_WriteReg(0x04004054,0x02C80000);// Change to page 0


	//
	// Configuring PHY - 2 - new addr
	//
	//led mode
	//C
	Salsa2_WriteReg(0x04004054,0x02CC0003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x020C0001);//set 1

	//D
	Salsa2_WriteReg(0x04004054,0x02CD0003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x020D0001);//set 1

	//E
	Salsa2_WriteReg(0x04004054,0x02CE0003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x020E0001);//set 1
	//F
	Salsa2_WriteReg(0x04004054,0x02CF0003);//set page to 3
	Salsa2_WriteReg(0x04004054,0x020F0001);//set 1

	Salsa2_WriteReg(0x04004054,0x02CC0000);// Set Page #0
	// C - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02CC00FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x016C1C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02CC0000);// Set Page #0
	// D - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02CD00FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x016DC190);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02CD0000);// Set Page #0
	// E - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02CE00FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x016E1C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02CE0000);// Set Page #0
	// F - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x04004054,0x02CF00FD);// Set the relevant page
	Salsa2_WriteReg(0x04004054,0x016E1C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x04004054,0x02CF0000);// Set Page #0
	// C - Configure PHY to Cut Through Mode
	Salsa2_WriteReg(0x04004054,0x02CC0010);// Set Page #16
	Salsa2_WriteReg(0x04004054,0x002C0070);// Write to address 0x70
	Salsa2_WriteReg(0x04004054,0x004C0040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x006C0000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x002C0870);// Write to address 0x870
	Salsa2_WriteReg(0x04004054,0x004C0040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x006C0000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x002C1070);// Write to address 0x1070
	Salsa2_WriteReg(0x04004054,0x004C0040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x006C0000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x002C1870); // Write to address 0x1870
	Salsa2_WriteReg(0x04004054,0x004C0040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x04004054,0x006C0000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x04004054,0x02CC0000);// Set Page #0
	// Configure PHY to EEE Master Mode
	// Enable EEE AutoNeg
	Salsa2_WriteReg(0x04004054,0x02CC0000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01AC0007);
	Salsa2_WriteReg(0x04004054,0x01CC003C);
	Salsa2_WriteReg(0x04004054,0x01AC4007);
	Salsa2_WriteReg(0x04004054,0x01CC0006);
	Salsa2_WriteReg(0x04004054,0x02CD0000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01AD0007);
	Salsa2_WriteReg(0x04004054,0x01CD003C);
	Salsa2_WriteReg(0x04004054,0x01AD4007);
	Salsa2_WriteReg(0x04004054,0x01CD0006);
	Salsa2_WriteReg(0x04004054,0x02CE0000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01AE0007);
	Salsa2_WriteReg(0x04004054,0x01CE003C);
	Salsa2_WriteReg(0x04004054,0x01AE4007);
	Salsa2_WriteReg(0x04004054,0x01CE0006);
	Salsa2_WriteReg(0x04004054,0x02CF0000);// Change to page 0
	Salsa2_WriteReg(0x04004054,0x01AF0007);
	Salsa2_WriteReg(0x04004054,0x01CF003C);
	Salsa2_WriteReg(0x04004054,0x01AF4007);
	Salsa2_WriteReg(0x04004054,0x01CF0006);
	// Enable EEE Master mode
	Salsa2_WriteReg(0x04004054,0x02CC0010);// Change to page 16
	Salsa2_WriteReg(0x04004054,0x002C03C1);
	Salsa2_WriteReg(0x04004054,0x004C0001);
	Salsa2_WriteReg(0x04004054,0x006C0000);
	Salsa2_WriteReg(0x04004054,0x002C0BC1);
	Salsa2_WriteReg(0x04004054,0x004C0001);
	Salsa2_WriteReg(0x04004054,0x006C0000);
	Salsa2_WriteReg(0x04004054,0x002C13C1);
	Salsa2_WriteReg(0x04004054,0x004C0001);
	Salsa2_WriteReg(0x04004054,0x006C0000);
	Salsa2_WriteReg(0x04004054,0x002C1BC1);
	Salsa2_WriteReg(0x04004054,0x004C0001);
	Salsa2_WriteReg(0x04004054,0x006C0000);
	Salsa2_WriteReg(0x04004054,0x02CC0000);// Change to page 0


	//
	// Configuring PHY - 3
	//
	//led mode
	//12
	Salsa2_WriteReg(0x05004054,0x02C40003);//set page to 3
	Salsa2_WriteReg(0x05004054,0x02040001);//set 1
	//13
	Salsa2_WriteReg(0x05004054,0x02C50003);//set page to 3
	Salsa2_WriteReg(0x05004054,0x02050001);//set 1
	//14
	Salsa2_WriteReg(0x05004054,0x02C60003);//set page to 3
	Salsa2_WriteReg(0x05004054,0x02060001);//set 1
	//15
	Salsa2_WriteReg(0x05004054,0x02C70003);//set page to 3
	Salsa2_WriteReg(0x05004054,0x02070001);//set 1

	// 4 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x05004054,0x02C400FD);// Set the relevant page
	Salsa2_WriteReg(0x05004054,0x01641C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x05004054,0x02C40000);// Set Page #0
	// 5 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x05004054,0x02C500FD);// Set the relevant page
	Salsa2_WriteReg(0x05004054,0x01651C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x05004054,0x02C50000);// Set Page #0
	// 6 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x05004054,0x02C600FD);// Set the relevant page
	Salsa2_WriteReg(0x05004054,0x01661C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x05004054,0x02C60000);// Set Page #0
	// 7 - QSGMII-88E1545 PHY FFE Adjustments
	Salsa2_WriteReg(0x05004054,0x02C700FD);// Set the relevant page
	Salsa2_WriteReg(0x05004054,0x01671C90);// PE = 1, AMP = 1
	Salsa2_WriteReg(0x05004054,0x02C70000);// Set Page #0
	// 4 - Configure PHY to Cut Through Mode
	Salsa2_WriteReg(0x05004054,0x02C40010);// Set Page #16
	Salsa2_WriteReg(0x05004054,0x00240070);// Write to address 0x70
	Salsa2_WriteReg(0x05004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x05004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x05004054,0x00240870);// Write to address 0x870
	Salsa2_WriteReg(0x05004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x05004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x05004054,0x00241070);// Write to address 0x1070
	Salsa2_WriteReg(0x05004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x05004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x05004054,0x00241870); // Write to address 0x1870
	Salsa2_WriteReg(0x05004054,0x00440040);// 16 LSB are 0x40
	Salsa2_WriteReg(0x05004054,0x00640000);// 16 MSB are 0x0
	Salsa2_WriteReg(0x05004054,0x02C40000);// Set Page #0
	// Configure PHY to EEE Master Mode
	// Enable EEE AutoNeg
	Salsa2_WriteReg(0x05004054,0x02C40000);// Change to page 0
	Salsa2_WriteReg(0x05004054,0x01A40007);//reg=13//val=7
	Salsa2_WriteReg(0x05004054,0x01C4003C);//reg=14//val=3C
	Salsa2_WriteReg(0x05004054,0x01A44007);//reg=13//val=7
	Salsa2_WriteReg(0x05004054,0x01C40006);//reg=14//val=6
	Salsa2_WriteReg(0x05004054,0x02C50000);// Change to page 0
	Salsa2_WriteReg(0x05004054,0x01A50007);
	Salsa2_WriteReg(0x05004054,0x01C5003C);
	Salsa2_WriteReg(0x05004054,0x01A54007);
	Salsa2_WriteReg(0x05004054,0x01C50006);
	Salsa2_WriteReg(0x05004054,0x02C60000);// Change to page 0
	Salsa2_WriteReg(0x05004054,0x01A60007);
	Salsa2_WriteReg(0x05004054,0x01C6003C);
	Salsa2_WriteReg(0x05004054,0x01A64007);
	Salsa2_WriteReg(0x05004054,0x01C60006);
	Salsa2_WriteReg(0x05004054,0x02C70000);// Change to page 0
	Salsa2_WriteReg(0x05004054,0x01A70007);
	Salsa2_WriteReg(0x05004054,0x01C7003C);
	Salsa2_WriteReg(0x05004054,0x01A74007);
	Salsa2_WriteReg(0x05004054,0x01C70006);
	// Enable EEE Master mode
	Salsa2_WriteReg(0x05004054,0x02C40010);// Change to page 16
	Salsa2_WriteReg(0x05004054,0x002403C1);
	Salsa2_WriteReg(0x05004054,0x00440001);
	Salsa2_WriteReg(0x05004054,0x00640000);
	Salsa2_WriteReg(0x05004054,0x00240BC1);
	Salsa2_WriteReg(0x05004054,0x00440001);
	Salsa2_WriteReg(0x05004054,0x00640000);
	Salsa2_WriteReg(0x05004054,0x002413C1);
	Salsa2_WriteReg(0x05004054,0x00440001);
	Salsa2_WriteReg(0x05004054,0x00640000);
	Salsa2_WriteReg(0x05004054,0x00241BC1);
	Salsa2_WriteReg(0x05004054,0x00440001);
	Salsa2_WriteReg(0x05004054,0x00640000);
	Salsa2_WriteReg(0x05004054,0x02C40000);// Change to page 0



	//phy0
	//Salsa2_WriteReg(0x04004054,0x02C40004);// RD ONLY: Set page for disable output clock
	//Salsa2_WriteReg(0x04004054,0x03643E80);// RD ONLY: Disable output clock of the right phy
	//Salsa2_WriteReg(0x04004054,0x02C40000);// RD ONLY: Return to page #0

	//phy 1
	Salsa2_WriteReg(0x04004054,0x02C80004);// RD ONLY: Set page for disable output clock
	Salsa2_WriteReg(0x04004054,0x03683E80);// RD ONLY: Disable output clock of the right phy
	Salsa2_WriteReg(0x04004054,0x02C80000);// RD ONLY: Return to page #0

	//phy 2
	Salsa2_WriteReg(0x04004054,0x02CC0004);// RD ONLY: Set page for disable output clock
	Salsa2_WriteReg(0x04004054,0x036C3E80);// RD ONLY: Disable output clock of the right phy
	Salsa2_WriteReg(0x04004054,0x02CC0000);// RD ONLY: Return to page #0

	//phy 3
	Salsa2_WriteReg(0x05004054,0x02C40004);// RD ONLY: Set page for disable output clock
	Salsa2_WriteReg(0x05004054,0x03643E80);// RD ONLY: Disable output clock of the right phy
	Salsa2_WriteReg(0x05004054,0x02C40000);// RD ONLY: Return to page #0




	//читаем статусные регистры портов - необходимо
	for(u8 i=0;i<FIBER_PORT_NUM;i++){
		Salsa2_WritePhyReg(i,Salsa2_get_phyAddr(i),22,1);
		Salsa2_ReadPhyReg(i,Salsa2_get_phyAddr(i),1);
	}

	//Restart ANEg - for fiber ports - не знаю нужно ли?
	/*for(u8 i=0;i<FIBER_PORT_NUM;i++){
		//set page 1
		Salsa2_WritePhyReg(i,Salsa2_get_phyAddr(i),22,1);
		tmp_reg = Salsa2_ReadPhyReg(i,Salsa2_get_phyAddr(i),0);
		tmp_reg |= 1<<9;//set Restart Fiber aneg bit
		Salsa2_WritePhyReg(i,Salsa2_get_phyAddr(i),0,tmp_reg);
	}*/

	//Restart ANEg - for cooper ports
	for(u8 i=FIBER_PORT_NUM;i<ALL_PORT_NUM;i++){
		//set page 0
		Salsa2_WritePhyReg(i,Salsa2_get_phyAddr(i),22,0);
		tmp_reg = Salsa2_ReadPhyReg(i,Salsa2_get_phyAddr(i),0);
		tmp_reg |= 1<<9;//set Restart Cooper aneg bit
		Salsa2_WritePhyReg(i,Salsa2_get_phyAddr(i),0,tmp_reg);
	}



	//config Phy addr
	for(u8 i=0;i<PORT_NUM;i++)
		Salsa2_configPhyAddres(i,get_port_phy_addr(i));


	//get phy`s ID
	//phy 0
	if(Salsa2_ReadPhyReg(0,get_port_phy_addr(0),2) != 0x0141)
		ADD_ALARM(ERROR_MARVEL_PHY0);

	//phy 1
	if(Salsa2_ReadPhyReg(4,get_port_phy_addr(4),2)!=0x0141)
		ADD_ALARM(ERROR_MARVEL_PHY1);

	//phy 2
	if(Salsa2_ReadPhyReg(8,get_port_phy_addr(8),2)!=0x0141)
		ADD_ALARM(ERROR_MARVEL_PHY2);

	//phy 3
	if(Salsa2_ReadPhyReg(12,get_port_phy_addr(12),2)!=0x0141)
		ADD_ALARM(ERROR_MARVEL_PHY3);





	//fix erranum 6.3
	Salsa2_WriteRegField(0x3000004,10,0,1);

	//fix erranum 6.4
	Salsa2_WriteRegField(0x02040000,19,1,1);
	Salsa2_WriteRegField(0x02040000,14,1,1);

	//fix erranum 6.5
	for(u8 i=0;i<16;i++){
		Salsa2_WriteRegField(0x10000014+0x400*i,1,0,1);
	}
	//Salsa2_WriteRegField(0x10000014+0x400*31,1,0,1);

	//fix erranum 6.6
	Salsa2_WriteRegField(0x6000000,7,0,1);

	//fix erranum 6.7 - mii mode
	for(u8 i=0;i<16;i++){
		Salsa2_WriteRegField(0x100000C8+0x400*i,7,0,1);
	}

	//fix erranum 6.8 - incorrect descr
	Salsa2_WriteRegField(0x01850000,0,1,1);//strict priority for unicast

	//fix erranum 6.9
	Salsa2_WriteRegField(0x01850000,27,1,1);

	//fix erranum 6.10
	Salsa2_WriteRegField(0x6000000,6,1,1);



	//experiments
//	Salsa2_WriteReg(0x03000000,0x7FCFF3FC);//lossless mode
//	Salsa2_WriteReg(0x01800004,0x039227F3);//TxQ config

//	Salsa2_WriteRegField(0x01850000,25,0,1);//disable HOL


	Salsa2_WriteReg(0x03000000,0x7FC69172);//bm comfiguration
	Salsa2_WriteReg(0x03000020,0x0780C826);//bp per port
	Salsa2_WriteReg(0x01800004,0x0392FBF3);//multi dest limit
	//Salsa2_WriteReg(0x01800C80,0x00000008);//txq desc limit per port
	Salsa2_WriteReg(0x01800C80,0x00003108);//txq desc limit per port

	Salsa2_WriteReg(0x01800C00,0x00003109);//txq per TC 0
	Salsa2_WriteReg(0x01800C10,0x00003109);//txq per TC 1
	Salsa2_WriteReg(0x01800C20,0x00003109);//txq per TC 2
	Salsa2_WriteReg(0x01800C30,0x00003109);//txq per TC 3



	// Enable Device Number 0
	//Salsa2_WriteReg(0x00000000,0x0000600B);
	//Salsa2_WriteReg(0x00000000,0x00006003);
	//printf("config done\r\n");
}

//set 100 mb full duplex to CPU port
//for Salsa 2
void config_cpu_port(void){
u32 temp;
	//PortReset = 0
	Salsa2_WriteRegField(CPU_PORT_CTRL_REG,30,0,1);

	//MRU = 1522
	Salsa2_WriteRegField(CPU_PORT_CTRL_REG,9,1,3);

	//VLAN Ethernet Type - default
//	Salsa2_WriteReg(CPU_PORT_SA_HI_REG,0x81008100);


	//SALow
	//Salsa2_WriteRegField(CPU_PORT_CTRL_REG,1,dev_addr[5],8);

	//SAMid
	//Salsa2_WriteRegField(CPU_PORT_SA_MID_REG,0,dev_addr[4],8);

	//SAHi
	//temp = (dev_addr[0]<<24)|(dev_addr[1]<<16)|(dev_addr[2]<<8) | dev_addr[3];
	//Salsa2_WriteReg(CPU_PORT_SA_HI_REG,temp);

	//printf("config_cpu_port %lX\r\n",temp);

	//set Default Trafic Class
	//Salsa2_WriteRegField(BRIDGE_CPU_PORT_CTRL,6,3,2);



	//link down
	Salsa2_WriteRegField(CPU_PORT_STATUS_REG,0,0,1);
//	//set 100 mbps full
	Salsa2_WriteRegField(CPU_PORT_STATUS_REG,2,0x6,3);
	//flow ctrl enable
	//Salsa2_WriteRegField(CPU_PORT_STATUS_REG,5,1,1);
	//link up
	Salsa2_WriteRegField(CPU_PORT_STATUS_REG,0,1,1);


	//set priority DSA Command=FROM_CPU
	Salsa2_WriteRegField(CPU2CODE_PRIO_REG,22,2,2);

	//enable unknown rate limit
	Salsa2_WriteRegField(UNKNOWN_RATE_LIMIT,0,1,1);

	//Forward No CPU Unknow Multicast
	//Salsa2_WriteRegField(EGRESS_BRIDGING_REG,3,3,2);

	//Forward No CPU Unknow Unicast
	//Salsa2_WriteRegField(EGRESS_BRIDGING_REG,1,3,2);

	//Enable CPU Broadcast Filter
	//Salsa2_WriteRegField(EGRESS_BRIDGING_REG,0,1,1);
}


/*Read MIB Counters
 * IN type
 * IN port
 * OUT *ret
 * return operation result*/
u8 Salsa2_Read_Counter(u8 type,u8 port,u32 *ret){
u32 port_modula;
u32 offset;
static u8 init[PORT_NUM];

	if(init[port] != 1){
		//set port
		port_modula = port%6;
		offset = MIB_CNT_CTRL_REG0_P0+0x800000*(port/6);
		Salsa2_WriteRegField(offset,1,port_modula,3);

		//Set DontClear bit
		Salsa2_WriteRegField(offset,4,1,1);

		//set trigger
		Salsa2_WriteRegField(offset,0,1,1);

		//wait
		while(Salsa2_ReadRegField(MIB_CNT_CTRL_REG0_P0+0x800000*(port/6),0,1)){}
		init[port] = 1;
	}

	//read counter
	offset = 0;
	CALC_MIB_CNT_OFFSET(port,offset);
	offset += type;
	*ret = Salsa2_ReadReg(offset);
	vTaskDelay(10);
	return GT_OK;
}



//vlan group

//set vid to internal vid pointer entry
u8 set_vlan_entry(u16 vid,u32 entry,u8 valid){
	if(vid%2 == 0){
		Salsa2_WriteRegField(VID_TO_INT_ENTRY0+0x4*(vid/2),0,entry,8);
		Salsa2_WriteRegField(VID_TO_INT_ENTRY0+0x4*(vid/2),8,valid,1);
	}
	else{
		Salsa2_WriteRegField(VID_TO_INT_ENTRY0+0x4*(vid/2),9,entry,8);
		Salsa2_WriteRegField(VID_TO_INT_ENTRY0+0x4*(vid/2),17,valid,1);
	}
	return 0;
}


void set_vlan_port_state(u8 vnum,u8 port,u8 state){
u8 pstate;

	switch(state){
		case 0: pstate = 0;break;
		case 1: pstate = 0;break;
		case 2: pstate = 1;break;
		case 3: pstate = 3;break;
		default: pstate = 0;
	}

	if(port<4){
		Salsa2_WriteRegField(VID0_ENTRY_WORD0+vnum*0x10,24+port*2,pstate,2);
	}
	else if(port<20){
		Salsa2_WriteRegField(VID0_ENTRY_WORD1+vnum*0x10,(port-4)*2,pstate,2);
	}
	else{
		Salsa2_WriteRegField(VID0_ENTRY_WORD2+vnum*0x10,(port-20)*2,pstate,2);
	}
}

void set_vlan_igmp_mode(u8 vnum,u8 state){
	Salsa2_WriteRegField(VID0_ENTRY_WORD0+vnum*0x10,12,state,1);
}

void set_vlan_stp_ptr(u8 vnum,u8 ptr){
	Salsa2_WriteRegField(VID0_ENTRY_WORD0+vnum*0x10,15,ptr,6);
}

void set_vlan_mgmt_mode(u8 vnum,u8 state){
	Salsa2_WriteRegField(VID0_ENTRY_WORD0+vnum*0x10,4,state,1);
}

void set_cpu_port_vid(u16 vid){
	Salsa2_WriteRegField(CPU_PORT_VID,0,vid,12);
}


void set_vlan_valid(u8 vnum,u8 state){
	//set valid bit
	Salsa2_WriteRegField(VID0_ENTRY_WORD0+vnum*0x10,0,state,1);

	//set learning enable bit
	Salsa2_WriteRegField(VID0_ENTRY_WORD0+vnum*0x10,1,state,1);
}

void set_port_default_vid(u8 port,u16 dvid){
	//enable port based vlan
//	Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+(port)*0x1000,18,ENABLE,1);

	//set vid for vlan
	if(port%2==0)
		Salsa2_WriteRegField(PORT0_VID_REG+(port/2)*0x2000,0,dvid,12);
	else
		Salsa2_WriteRegField(PORT0_VID_REG+(port/2)*0x2000,16,dvid,12);

	//Salsa2_WriteRegField(PORT0_VID_REG+port*0x1000,0,dvid,12);
}

u8 Salsa2_get_phyAddr(u8 port){
	switch(port){
		case 0: return  0x04;
		case 1: return  0x05;
		case 2: return  0x06;
		case 3: return  0x07;
		case 4: return  0x08;
		case 5: return  0x09;
		case 6: return  0x0A;
		case 7: return  0x0B;
		case 8: return  0x0C;
		case 9: return  0x0D;
		case 10: return 0x0E;
		case 11: return 0x0F;
		case 12: return 0x04;
		case 13: return 0x05;
		case 14: return 0x06;
		case 15: return 0x07;
	}
	return 0;
}

/*trunk - trunk ID 0-4, port, num - порядковый номер записи */
static void add_port_to_trunk(u8 trunk_id,u8 port, u8 num){
	//set DevNum 0
	Salsa2_WriteRegField(TRUNK1_MEMBERS_TABLE_WORD0+0x10*trunk_id+0x04*(num/2),(num%2)*10+5,0,5);
	//set port
	Salsa2_WriteRegField(TRUNK1_MEMBERS_TABLE_WORD0+0x10*trunk_id+0x04*(num/2),(num%2)*10,port,5);
}

//хеши по каждому транку
struct trunk_hash_t{
	u32 hash[8];
	u32 hash_all;
	u8  port_num;//число портов в транке
};

void SWU_link_aggregation_config(void){
u8 port,port_num;
u32 hash_notrunk;
u32 hash[8];
struct trunk_hash_t trunk_hash[LAG_MAX_ENTRIES];

	for(u8 id=0;id<LAG_MAX_ENTRIES;id++){
		if(get_lag_valid(id)){
			for(port = 0,port_num = 0;port<ALL_PORT_NUM;port++){
				if(get_lag_port(id,port) == 1){
					//set port Trunk GroupID
					Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+(port)*0x1000,0,get_lag_id(id),5);
					port_num++;
				}
			}

			//config trunk members num
			Salsa2_WriteRegField(TRUNK_TABLE_REG0,id*4,port_num,4);

			for(port = 0;port<ALL_PORT_NUM;port++){
				if(get_lag_port(id,port) == 1){
					//add members to trunk
					add_port_to_trunk(id,port,port_num);
				}
			}
		}
	}



  	//config hash
	Salsa2_WriteRegField(TRANSMIT_QUEUE_CTRL_REG,0,1,1);//hash based


	//set hash 0
	hash_notrunk = 0;
	//формируем суммарный хеш по каждому порту
	for(u8 id=0;id<LAG_MAX_ENTRIES;id++){
		trunk_hash[id].hash[0] = trunk_hash[id].hash[1] = trunk_hash[id].hash[2] =
		trunk_hash[id].hash[3] = trunk_hash[id].hash[4] = trunk_hash[id].hash[5] =
		trunk_hash[id].hash[6] = trunk_hash[id].hash[7] = 0;
		trunk_hash[id].port_num = 0;
		for(port = 0;port<ALL_PORT_NUM;port++){
			if(get_lag_port(id,port) ){
				trunk_hash[id].hash[trunk_hash[id].port_num%LAG_MAX_PORTS] |= (1<<port);
				trunk_hash[id].port_num++;
				hash_notrunk |= (1<<port);
			}
		}
		//заполнение повторяющимися данными
		if(trunk_hash[id].port_num<LAG_MAX_PORTS){
			for(u8 i=trunk_hash[id].port_num;i<LAG_MAX_PORTS;i++){
				trunk_hash[id].hash[i] = trunk_hash[id].hash[i-trunk_hash[id].port_num];
			}
		}

	}
	hash_notrunk = (~hash_notrunk) & 0xFFFF;//хеш всех портов, которые не входят ни в один транк
	DEBUG_MSG(DEBUG_QD,"hash_notrunk = 0x%lX\r\n",hash_notrunk);

	//формируем общие хеши
	for(u8 i=0;i<8;i++){
		hash[i] = hash_notrunk;
		for(u8 id=0;id<LAG_MAX_ENTRIES;id++){
			hash[i] |= trunk_hash[id].hash[i];
		}
		DEBUG_MSG(DEBUG_QD,"HASH[%d] = 0x%lX\r\n",i,hash[i]);
	}

	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH0,0,hash[0],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH1,0,hash[1],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH2,0,hash[2],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH3,0,hash[3],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH4,0,hash[4],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH5,0,hash[5],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH6,0,hash[6],24);
	Salsa2_WriteRegField(TRUNK_DESIGNATED_PORTS_HASH7,0,hash[7],24);


	for(u8 id=0;id<LAG_MAX_ENTRIES;id++){
		if(get_lag_valid(id)){
			for(port = 0,port_num = 0;port<ALL_PORT_NUM;port++){
				//set trunk non member reg
				//member - 0, no member - 1
				if(get_lag_port(id,port) == 1)
					Salsa2_WriteRegField(TRUNK0_NON_TRUNK_MEMB_REG+id*0x1000,port,0,1);
				else
					Salsa2_WriteRegField(TRUNK0_NON_TRUNK_MEMB_REG+id*0x1000,port,1,1);
			}
		}
	}
}

void SWU_port_mirroring_config(void){
u8 is_rx_sniff,is_tx_sniff;

	is_rx_sniff = 0;
	is_tx_sniff = 0;



	for(u8 port=0; port< ALL_PORT_NUM; port++){
		//rx or both - rx sniffer
		if((get_mirror_port(port) == 1 || get_mirror_port(port) == 3 ) && (get_mirror_state() == ENABLE)){
			Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG + 0x1000*port,24,ENABLE,1);
			is_rx_sniff = 1;
		}
		else
		 	Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG + 0x1000*port,24,DISABLE,1);

		//tx or both - tx sniffer
		if((get_mirror_port(port) == 2 || get_mirror_port(port) == 3 ) && (get_mirror_state() == ENABLE)){
			Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG + 0x1000*port,10,ENABLE,1);
			is_tx_sniff = 1;
		}
		else
			Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG + 0x1000*port,10,DISABLE,1);
	}

	//set TX target port
	if(is_tx_sniff){
		//set dev num
		Salsa2_WriteRegField(TRANSMIT_SNIFFER_REG,0,0,5);
		//set port
		Salsa2_WriteRegField(TRANSMIT_SNIFFER_REG,7,get_mirror_target_port(),5);

		Salsa2_WriteRegField(TRANSMIT_SNIFFER_REG,19,2,11);//1/2 sniff rate
		Salsa2_WriteRegField(TRANSMIT_SNIFFER_REG,18,ENABLE,1);//enable statistical tx sniff
	}
	else{
		Salsa2_WriteRegField(TRANSMIT_SNIFFER_REG,18,DISABLE,1);
	}

	//set Rx target port
	if(is_rx_sniff){
		//set dev num
		Salsa2_WriteRegField(INGRESS_MIRRORING_REG,3,0,5);
		//set port
		Salsa2_WriteRegField(INGRESS_MIRRORING_REG,11,get_mirror_target_port(),5);

		//set sniff ratio
		Salsa2_WriteRegField(STATISTIC_SNIFF_REG,1,1,11);//all packet are mirrored
	}
}


/*установка тестовой конфигурации ШЛЕЙФ*/
void set_swu_test_vlan(u8 state){

	if(state == ENABLE){


		//set vlans enable
		set_vlan_entry(1,0,ENABLE);
		set_vlan_entry(2,1,ENABLE);
		set_vlan_entry(3,2,ENABLE);
		set_vlan_entry(4,3,ENABLE);
		set_vlan_entry(5,4,ENABLE);
		set_vlan_entry(6,5,ENABLE);
		set_vlan_entry(7,6,ENABLE);
		set_vlan_entry(8,7,ENABLE);

		//v1 2-15 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 1 || i== 14)
				set_vlan_port_state(0,i,2);
			else
				set_vlan_port_state(0,i,0);
		}


		//v2 1-3 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 0 || i== 2)
				set_vlan_port_state(1,i,2);
			else
				set_vlan_port_state(1,i,0);
		}

		//v3 4-6 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 3 || i== 5)
				set_vlan_port_state(2,i,2);
			else
				set_vlan_port_state(2,i,0);
		}


		//v4 5-7 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 4 || i== 6)
				set_vlan_port_state(3,i,2);
			else
				set_vlan_port_state(3,i,0);
		}


		//v5 8-10 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 7 || i== 9)
				set_vlan_port_state(4,i,2);
			else
				set_vlan_port_state(4,i,0);
		}


		//v6 9-11 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 8 || i== 10)
				set_vlan_port_state(5,i,2);
			else
				set_vlan_port_state(5,i,0);
		}

		//v7 12-13 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 11 || i== 12)
				set_vlan_port_state(6,i,2);
			else
				set_vlan_port_state(6,i,0);
		}

		//v8 14-16 - untagged
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(i == 13 || i== 15)
				set_vlan_port_state(7,i,2);
			else
				set_vlan_port_state(7,i,0);
		}

		set_vlan_mgmt_mode(0,DISABLE);
		set_vlan_mgmt_mode(1,DISABLE);
		set_vlan_mgmt_mode(2,DISABLE);
		set_vlan_mgmt_mode(3,DISABLE);
		set_vlan_mgmt_mode(4,DISABLE);
		set_vlan_mgmt_mode(5,DISABLE);
		set_vlan_mgmt_mode(6,DISABLE);
		//v8 - port 16 - managment VLAN
		set_vlan_mgmt_mode(7,ENABLE);
		set_cpu_port_vid(8);

		set_vlan_valid(0,ENABLE);
		set_vlan_valid(1,ENABLE);
		set_vlan_valid(2,ENABLE);
		set_vlan_valid(3,ENABLE);
		set_vlan_valid(4,ENABLE);
		set_vlan_valid(5,ENABLE);
		set_vlan_valid(6,ENABLE);
		set_vlan_valid(7,ENABLE);




		//set default VLAN
		set_port_default_vid(0,2);//1
		set_port_default_vid(1,1);//2
		set_port_default_vid(2,2);//3
		set_port_default_vid(3,3);//4
		set_port_default_vid(4,4);//5
		set_port_default_vid(5,3);//6
		set_port_default_vid(6,4);//7
		set_port_default_vid(7,5);//8
		set_port_default_vid(8,6);//9
		set_port_default_vid(9,5);//10
		set_port_default_vid(10,6);//11
		set_port_default_vid(11,7);//12
		set_port_default_vid(12,7);//13
		set_port_default_vid(13,8);//14
		set_port_default_vid(14,1);//15
		set_port_default_vid(15,8);//16

	}
}


//add/delete mac entry
int salsa2_mac_entry_ctrl(u8 port,u8 *mac, u16 vid, u8 trunk, u8 type){
u32 temp;
	if(port<PORT_NUM){
		if(type == ADD_MAC_ENTRY){
			//word 0
			temp = (mac[4] << 24) | (mac[5] << 16) | (0 << 15) | (0x01 << 7) | (NEW_ADDR_MSG << 4) | 0x02;
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG0,temp);
			//word 1
			temp = mac[0] << 24 | mac[1] << 16 | mac[2] << 8 | mac[3];
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG1,temp);
			//word 2
			if(trunk)
				temp = (trunk<< 24) | (1<<16) | (1<<14) | (vid);	//trunk member
			else
				temp = (port << 24) | (1<<16) | (0<<14) | (vid);	//port
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG2,temp);
			//word 3
			temp = 1<<18 | 1<<31;//static
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG3,temp);
			//wait
			while(temp & (1<<31)){
				temp = Salsa2_ReadReg(MESSAGE_FROM_CPU_REG3);
			}

			return 0;

		}
		else if(type == DEL_MAC_ENTRY){
			//word 0
			temp = (mac[4] << 24) | (mac[5] << 16) | (0 << 15) | (0x01 << 7) | (NEW_ADDR_MSG << 4) | 0x02;
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG0,temp);
			//word 1
			temp = mac[0] << 24 | mac[1] << 16 | mac[2] << 8 | mac[3];
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG1,temp);
			//word 2
			if(trunk)
				temp = (trunk<< 24) | (1<<16) | (1<<14) | (1<<12) | (vid) ;	//trunk member
			else
				temp = (port << 24) | (1<<16) | (0<<14) | (1<<12) | (vid);	//port
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG2,temp);
			//word 3
			temp = 1<<18 | 1<<31;//static
			Salsa2_WriteReg(MESSAGE_FROM_CPU_REG3,temp);
			//wait
			while(temp & (1<<31)){
				temp = Salsa2_ReadReg(MESSAGE_FROM_CPU_REG3);
			}
			//DEBUG_MSG(IGMP_DEBUG,"wait r3 %lX\r\n",temp);

			//temp = Salsa2_ReadReg(MESSAGE_FROM_CPU_REG0);
			//DEBUG_MSG(IGMP_DEBUG,"r1 %lX\r\n",temp);

			return 0;
		}
	}
	else{
		return -1;
	}
	return 0;
}


//управление миганием светодиодов на панели
void set_link_indication_mode(u8 mode){
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		Salsa2_WritePhyReg(i,get_port_phy_addr(i),22,3);
		Salsa2_WritePhyReg(i,get_port_phy_addr(i),16,mode);
	}
}

void salsa2_dump(void){
	for(u32 i=0;i<=0xE4;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x00400000;i<=0x0040001C;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x01800000;i<=0x01801F04;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x01810500;i<=0x01810C80;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x01840000;i<=0x0184001C;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x01850000;i<=0x01850014;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x02000000;i<=0x02018000;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x02040000;i<=0x0204027C;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}


	for(u32 i=0x03000000;i<=0x03000044;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x04004000;i<=0x04005114;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x04010000;i<=0x040102FC;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x04810000;i<=0x048102FC;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x04804000;i<=0x04805114;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x05004000;i<=0x05005114;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x05010000;i<=0x050102FC;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x05804000;i<=0x0580410C;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x05810000;i<=0x058102FC;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x06000000;i<=0x060002EC;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x06100000;i<=0x0610FFF8;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x06810000;i<=0x06817FFF;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x07018000;i<=0x0701FFFF;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}

	for(u32 i=0x0A000000;i<=0x0A0060FC;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x10000000;i<=0x10005CD0;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
	for(u32 i=0x18000000;i<=0x180053E0;i+=4){
		printf("offset\t0x%08lX\t%08lX\r\n",i,Salsa2_ReadReg(i));
		IWDG_ReloadCounter();
		vTaskDelay(1);
	}
}


void set_swu_port_loopback(u8 port,u8 state){

}
