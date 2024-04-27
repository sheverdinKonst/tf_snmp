/*
 * eapol.h
 *
 *  Created on: 28 февр. 2018 г.
 *      Author: BelyaevAlex
 */

#ifndef NET_802_1X_EAPOL_H_
#define NET_802_1X_EAPOL_H_


#define EAPOL_ETHTYPE  0x888E

//статистика протокола EAPOL для порта
struct eapol_port_stat_t{
	u32 eapolFramesRx;//Количество корректных пакетов любого типа
	u32 eapolFramesTx;
	u32 eapolStartFramesRx;//Количество пакетов Start
	u32 eapolLogoffFramesRx;//Количество пакетов Logoff
	u32 eapolRespIdFramesRx;//Количество пакетов Resp/Id
	u32 eapolRespFramesRx;//Количество пакетов ответов (кроме Resp/Id)
	u32 eapolReqIdFramesTx;//Количество пакетов Resp/Id
	u32 eapolReqFramesTx;//Количество пакетов запросов (кроме Resp/Id)
	u32 invalidEapolFramesRx;//Количество пакетов протокола EAPOL с нераспознанным типом
	u32 eapLengthErrorFramesRx;//Количество пакетов протокола EAPOL с некорректной длиной
	u32 lastEapolFrameVersion;//Версия протокола EAPOL, принятая в самом последнем пакете.
	u32 lastEapolFrameSource[6];//MAC-адрес источника, принятый в самом последнем на данный момент пакете.
};



//структура пакета EAP
struct eap_packet_t{
	u8 	code;
	u8  identifier;
	u16	len;
	u8 *data;
};



void eap_config(void);
void eapol_input(void);
void ieee802_1x_init(void);













#endif /* NET_802_1X_EAPOL_H_ */
