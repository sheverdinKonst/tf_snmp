#ifndef VLAN_H_
#define VLAN_H_



#include "msApiDefs.h"
#include "board.h"



void PortBaseVLANSet(struct pb_vlan_t *pb);
void pbvlan_setup(void);
void VLANTableDecode(struct pb_vlan_t *pb,uint16_t *Table);
void VLANTableCode(struct pb_vlan_t *pb,uint16_t *Table);

uint8_t vtuOperationPerform(uint16_t vtuOp, uint8_t *valid,GT_VTU_ENTRY *entry);
uint8_t gvlnSetPortVlanDot1qMode(uint8_t Port,GT_DOT1Q_MODE mode);
GT_U8 memberTagConversionForDev(GT_U8  tag);
GT_U8 memberTagConversionForApp(GT_U8  tag);
GT_STATUS gvtuFindVidEntry(GT_VTU_ENTRY  *vtuEntry,GT_BOOL *found);
GT_LPORT port2lport(GT_U8  hwPort);
GT_U8 lport2port(GT_LPORT     port);
GT_STATUS gvlnSetPortVid(GT_LPORT port,GT_U16 vid);
uint8_t gvtuFlush(void);
GT_STATUS sample802_1qSetup(void);
uint8_t VLAN1QPortConfig(void);
uint8_t VLANPortModeSet(void);
GT_STATUS VLAN_setup(void);
GT_STATUS sampleDisplayVIDTable(void);
GT_STATUS sampleAdmitOnlyTaggedFrame(GT_LPORT port);


GT_STATUS SWU_VLAN_setup(void);
GT_STATUS SWU_pbvlan_setup(void);

//struct VLN VLAN[MAXVlanNum];
//struct vlncfg vlan_cfg;

#endif /* VLAN_H_ */
