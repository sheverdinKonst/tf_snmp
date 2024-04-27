/*
 * ctrls_deffines.h
 *
 *  Created on: 28.04.2014
 *      Author: Tsepelev
 */

#ifndef CTRLS_DEFFINES_H_
#define CTRLS_DEFFINES_H_

s8t getDot1qVlanVersionNumber(mib_object_t* object, u8t* oid, u8t len);
s8t getDot1qMaxVlanId(mib_object_t* object, u8t* oid, u8t len);
s8t getDot1qMaxSupportedVlans(mib_object_t* object, u8t* oid, u8t len);
s8t getDot1qNumVlans(mib_object_t* object, u8t* oid, u8t len);
s8t getDot1qGvrpStatus(mib_object_t* object, u8t* oid, u8t len);

s8t getDot1qVlanStaticName(mib_object_t* object, u8t* oid, u8t len);
s8t setDot1qVlanStaticName(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value);


s8t getDot1qVlanStaticTable(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextDot1qVlanStaticTable(mib_object_t* object, u8t* oid, u8t len);
s8t setDot1qVlanStaticTable(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value);





s8t getComfortStartTime(mib_object_t* object, u8t* oid, u8t len);
s8t setComfortStartTime(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value);
s8t getComfortStartEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextComfortStartEntry(mib_object_t* object, u8t* oid, u8t len);
s8t setComfortStartEntry(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value);

s8t getAutoReStartEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextAutoReStartEntry(mib_object_t* object, u8t* oid, u8t len);

s8t getPortPoeEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextPortPoeEntry(mib_object_t* object, u8t* oid, u8t len);
s8t setPortPoeEntry(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value);

s8t getUpsModeAvalible(mib_object_t* object, u8t* oid, u8t len);
s8t getUpsPwrSource(mib_object_t* object, u8t* oid, u8t len);
s8t getUpsBatteryVoltage(mib_object_t* object, u8t* oid, u8t len);
s8t getInputStatusEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextInputStatusEntry(mib_object_t* object, u8t* oid, u8t len);
s8t getFwVersion(mib_object_t* object, u8t* oid, u8t len);
s8t getEmConnectionStatus(mib_object_t* object, u8t* oid, u8t len);
s8t getEmResultTotal(mib_object_t* object, u8t* oid, u8t len);
s8t getEmResultT1(mib_object_t* object, u8t* oid, u8t len);
s8t getEmResultT2(mib_object_t* object, u8t* oid, u8t len);
s8t getEmResultT3(mib_object_t* object, u8t* oid, u8t len);
s8t getEmResultT4(mib_object_t* object, u8t* oid, u8t len);
s8t getEmPollingInterval(mib_object_t* object, u8t* oid, u8t len);
s8t getPortPoeStatusEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextPortPoeStatusEntry(mib_object_t* object, u8t* oid, u8t len);
s8t getAutoRestartErrorsEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextAutoRestartErrorsEntry(mib_object_t* object, u8t* oid, u8t len);
s8t getComfortStartStatusEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextComfortStartStatusEntry(mib_object_t* object, u8t* oid, u8t len);



//LLDP
s8t getLldpRemChassisIdSubtype(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextLldpRemChassisIdSubtype(mib_object_t* object, u8t* oid, u8t len);

s8t getLldpRemEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextLldpRemEntry(mib_object_t* object, u8t* oid, u8t len);


s8t getLldpMessageTxInterval(mib_object_t* object, u8t* oid, u8t len);
s8t getLldpTxHoldMultiplier(mib_object_t* object, u8t* oid, u8t len);
s8t getLldLocChassisIdSubtype(mib_object_t* object, u8t* oid, u8t len);
s8t getLldLocChassisId(mib_object_t* object, u8t* oid, u8t len);
s8t getLldLocSysName(mib_object_t* object, u8t* oid, u8t len);
s8t getLldLocSysDesc(mib_object_t* object, u8t* oid, u8t len);
s8t getLldLocSysCapSupported(mib_object_t* object, u8t* oid, u8t len);
s8t getLldLocSysCapEnabled(mib_object_t* object, u8t* oid, u8t len);
s8t getLldpPortConfigEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextLldpPortConfigEntry(mib_object_t* object, u8t* oid, u8t len);
s8t getLldpLocPortEntry(mib_object_t* object, u8t* oid, u8t len);
ptr_t* getNextLldpLocPortEntry(mib_object_t* object, u8t* oid, u8t len);

#endif /* CTRLS_DEFFINES_H_ */
