/* самодельные упрощенные Api функции для работы с Marvel 88E6095*/
#include <string.h>
#include "stm32f4x7_eth.h"
#include "board.h"
#include "SMIApi.h"
#include "FlowCtrl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "h/driver/gtDrvSwRegs.h"
#include "VLAN.h"
#include "names.h"
#include "settings.h"
#include "salsa2.h"
#include "debug.h"

/* The following macro converts a boolean   */
/* value to a binary one (of 1 bit).        */
/* GT_FALSE --> 0                           */
/* GT_TRUE --> 1                            */
#define BOOL_2_BIT(boolVal,binVal)                                  \
            (binVal) = (((boolVal) == GT_TRUE) ? 1 : 0)
#define GT_IS_PORT_SET(_portVec, _port)    \
            ((_portVec) & (0x1 << (_port)))



/*
typedef struct VLAN {
	uint16_t VID;
	char VLANNAme[20];
	uint8_t Ports;
}VLAN[MAXVlanNum];
*/



void PortBaseVLANSet(struct pb_vlan_t *pb){
//uint8_t a;
uint16_t Table[11];
uint16_t tmp;
//сначала заполняем 1
	for(uint8_t i=0;i<MV_PORT_NUM;i++)
		Table[i]=(~(1<<i))&0x7FF;

	if(pb->state==ENABLE){
		  for(uint8_t i=0;i<MV_PORT_NUM;i++){//заполняем таблицу из 11 строк (bin)
			Table[i] = (1<<CPU_PORT);
			for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
				if(i == L2F_port_conv(j))
					Table[i] |= pb->VLANTable[F2L_port_conv(i)][j]<<L2F_port_conv(j);
			}

			Table[i] &=0x7FF;
			tmp = ETH_ReadPHYRegister(0x10+i, 0x06);
			tmp&=~0x7FF;
			tmp|=Table[i];
			ETH_WritePHYRegister(0x10 +i,0x06,tmp);
		  }
	ETH_WritePHYRegister(0x1B,0x04,0x8000);//sw reset
	}
	else{
	//разрешаем работать всем портам
		  for(uint8_t i=0;i<MV_PORT_NUM;i++){
			Table[i]=(~(1<<i))&0x7FF;
			tmp = ETH_ReadPHYRegister(0x10+i, 0x06);
			tmp&=~0x7FF;
			tmp|=Table[i];
			ETH_WritePHYRegister(0x10+i,0x06,tmp);
		  }
	}
}

/*Port based VLAN Settings*/
void pbvlan_setup(void){
u16 tmp;
	if(get_pb_vlan_state()==1){
		for(u8 i=0;i<MV_PORT_NUM;i++){
			if(F2L_port_conv(i)!=-1){
				tmp = ETH_ReadPHYRegister(0x10+i,0x06);
				//printf("PB VLAN Read: Port %d,%X\r\n",i,tmp);
				tmp &=~0x7FF;
				for(u8 j=0;j<MV_PORT_NUM;j++){
					if(F2L_port_conv(j)!=-1){
						if(get_pb_vlan_port(F2L_port_conv(i),F2L_port_conv(j)))
							tmp |= 1<<j;
					}
				}
				//printf("PB VLAN mask: Port %d,%X\r\n",i,tmp);
				tmp &= ~(1<<i);
				tmp |= 1<<CPU_PORT;
				ETH_WritePHYRegister(0x10+i,0x06,tmp);
				//printf("PB VLAN Write: Port %d, %X\r\n",i,tmp);
			}
		}
		if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
			ETH_WritePHYRegister(0x1B,0x04,0x8000);//sw reset

	}
}

void VLANTableDecode(struct pb_vlan_t *pb,uint16_t *Table){
	for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if((Table[L2F_port_conv(i)] & (1<<L2F_port_conv(j))))
				pb->VLANTable[i][j]=1;
			else
				pb->VLANTable[i][j]=0;
		}
	}
}

//struct -> table
//make table
void VLANTableCode(struct pb_vlan_t *pb,uint16_t *Table){
	if(pb->state == DISABLE){
		for(u8 i=0;i<MV_PORT_NUM;i++){
			if(F2L_port_conv(i)!=-1){
				//if port exist
				Table[F2L_port_conv(i)] = 0x7FF & (~(1<<i));
			}
		}
	}
	else if(pb->state == ENABLE){
		for(u8 i=0;i<MV_PORT_NUM;i++){
			if(F2L_port_conv(i)!=-1){
				//if port exist
				Table[F2L_port_conv(i)] = 0x7FF & (~(1<<i));
				for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
					if(pb->VLANTable[i][j]==DISABLE){
						//disable selected
						Table[F2L_port_conv(i)] &= ~(1<<L2F_port_conv(j));
					}
				}
			}
		}
	}
}

uint8_t gvtuFlush(void){
	uint8_t retVal;
	retVal = vtuOperationPerform(FLUSH_ALL,NULL,NULL);
	return retVal;
}

uint8_t vtuOperationPerform(uint16_t vtuOp, uint8_t *valid,GT_VTU_ENTRY *entry){

uint16_t          data;           /* Data to be set into the      */
                                /* register.                    */


    /* Wait until the VTU in ready. */
    data = 1;

    while(data == 1)
    {   hwGetGlobalRegField(QD_REG_VTU_OPERATION,15,1,&data);
    }


    /* Set the VTU data register    */
    /* There is no need to setup data reg. on flush, get next, or service violation */
    if((vtuOp != FLUSH_ALL) && (vtuOp != GET_NEXT_ENTRY) && (vtuOp != SERVICE_VIOLATIONS))
    {

        /****************** VTU DATA 1 REG *******************/

        /* get data and wirte to QD_REG_VTU_DATA1_REG (ports 0 to 3) */

        data =  (entry->vtuData.memberTagP[0] & 3)     	|
                ((entry->vtuData.memberTagP[1] & 3)<<4) |
                ((entry->vtuData.memberTagP[2] & 3)<<8);

        //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
        if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
            data |= ((entry->vtuData.portStateP[0] & 3)<<2)    |
                    ((entry->vtuData.portStateP[1] & 3)<<6) |
                    ((entry->vtuData.portStateP[2] & 3)<<10);
        }

        //if(dev->maxPorts > 3)
        if(1)
        {
            data |= ((entry->vtuData.memberTagP[3] & 3)<<12) ;
            //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
            if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
                data |= ((entry->vtuData.portStateP[3] & 3)<<14) ;
        }

        //retVal = hwWriteGlobalReg(dev,QD_REG_VTU_DATA1_REG,data);
        hwWriteGlobalReg(QD_REG_VTU_DATA1_REG,data);

        /****************** VTU DATA 2 REG *******************/

        /* get data and wirte to QD_REG_VTU_DATA2_REG (ports 4 to 7) */

        //if(dev->maxPorts > 4)
        if(1)
        {


            /* also need to set data register  ports 4 to 6 */

             data =  (entry->vtuData.memberTagP[4] & 3)   |
                     ((entry->vtuData.memberTagP[5] & 3) << 4);

             //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
             if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
                 data |= ((entry->vtuData.portStateP[4] & 3) << 2) |
                         ((entry->vtuData.portStateP[5] & 3) << 6);

             //if(dev->maxPorts > 6){
				 data |= ((entry->vtuData.memberTagP[6] & 3)<<8) ;
				 //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
				 if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
					 data |= ((entry->vtuData.portStateP[6] & 3)<<10) ;
        	//}

			 if(MV_PORT_NUM > 7){
				 data |= ((entry->vtuData.memberTagP[7] & 3)<<12) ;
				 //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
				 if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
					 data |= ((entry->vtuData.portStateP[7] & 3)<<14) ;
			 }


            //retVal = hwWriteGlobalReg(QD_REG_VTU_DATA2_REG,data);
            //ETH_WritePHYRegister(GlobalRegisters,QD_REG_VTU_DATA2_REG,data);
            hwWriteGlobalReg(QD_REG_VTU_DATA2_REG,data);
        }


        /****************** VTU DATA 3 REG *******************/

        /* get data and wirte to QD_REG_VTU_DATA3_REG (ports 8 to 10) */
        //if(dev->maxPorts > 7)
        if(MV_PORT_NUM>7){

            /* also need to set data register  ports 8 to 9 */

           data =  (entry->vtuData.memberTagP[8] & 3)   |
                    ((entry->vtuData.memberTagP[9] & 3) << 4);

            //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
           	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
                 data |= ((entry->vtuData.portStateP[8] & 3) << 2)    |
                        ((entry->vtuData.portStateP[9] & 3) << 6);

            //if(dev->maxPorts > 10)
            if(MV_PORT_NUM>10)
            {
                data |= (entry->vtuData.memberTagP[10] & 3) << 8;

                //if (IS_IN_DEV_GROUP(dev,DEV_802_1S))
                if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
                     data |= (entry->vtuData.portStateP[10] & 3) << 10;
            }


            //if (IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
            if(1)
            {
                if(entry->vidPriOverride == GT_TRUE)
                    data |= ((1 << 15) | ((entry->vidPriority & 0x7) << 12));
            }

            //retVal = hwWriteGlobalReg(dev,QD_REG_VTU_DATA3_REG,data);
            //ETH_WritePHYRegister(GlobalRegisters,QD_REG_VTU_DATA3_REG,data);
            hwWriteGlobalReg(QD_REG_VTU_DATA3_REG,data);


        }
        else
        	//if (IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
        	if(1)
			{
				if(entry->vidPriOverride == GT_TRUE)
					data = ((1 << 15) | ((entry->vidPriority & 0x7) << 12));
				else
					data = 0;

				//retVal = hwWriteGlobalReg(dev,QD_REG_VTU_DATA3_REG,data);
				//ETH_WritePHYRegister(GlobalRegisters,QD_REG_VTU_DATA3_REG,data);
				hwWriteGlobalReg(QD_REG_VTU_DATA3_REG,data);
			}
    }

    /* Set the VID register (QD_REG_VTU_VID_REG) */
    /* There is no need to setup VID reg. on flush and service violation */
    if((vtuOp != FLUSH_ALL) && (vtuOp != SERVICE_VIOLATIONS) )
    {
        data= ( (entry->vid) & 0xFFF ) | ( (*valid) << 12 );
        hwWriteGlobalReg((GT_U8)(QD_REG_VTU_VID_REG),data);
    }


    /* Set SID, FID, VIDPolicy, if it's Load operation */

    if((vtuOp == LOAD_PURGE_ENTRY) && (*valid == 1))
    {
        //if(IS_IN_DEV_GROUP(dev,DEV_802_1S_STU))
    	if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
        {
            data= (entry->sid) & 0x3F;
            hwWriteGlobalReg((GT_U8)(QD_REG_STU_SID_REG),data);
        }

        data = 0;

        //if(IS_IN_DEV_GROUP(dev,DEV_FID_REG))
        if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
        {
            //if(IS_IN_DEV_GROUP(dev,DEV_POLICY))
        	if(get_marvell_id() == DEV_88E6240)
            {
                data= entry->vidPolicy << 12;
            }

            data |= (entry->DBNum & 0xFFF);

            hwWriteGlobalReg((GT_U8)(QD_REG_VTU_FID_REG),data);

        }
    }




    /* Start the VTU Operation by defining the DBNum, vtuOp and VTUBusy    */
    /*
     * Flush operation will skip the above two setup (for data and vid), and
     * come to here directly
     */

    if(vtuOp == FLUSH_ALL)
        data = (1 << 15) | (vtuOp << 12);
    else
    {
    	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
    		data = (1 << 15) | (vtuOp << 12) | ((entry->DBNum & 0xF0) << 4) | (entry->DBNum & 0x0F);
    	else
    		data = (1 << 15) | (vtuOp << 12) | entry->DBNum;

    }

    hwWriteGlobalReg(QD_REG_VTU_OPERATION,data);
    //ETH_WritePHYRegister(GlobalRegisters,QD_REG_VTU_OPERATION,data);


    /* only two operations need to go through the mess below to get some data
     * after the operations -  service violation and get next entry
     */

    /* If the operation is to service violation operation wait for the response   */
    if(vtuOp == SERVICE_VIOLATIONS)
    {
        /* Wait until the VTU in ready. */
        data = 1;
        while(data == 1)
        {
            hwGetGlobalRegField(QD_REG_VTU_OPERATION,15,1,&data);
        	//data=(((ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_OPERATION))>>15) & 0x1);
        }


        /* get the Source Port ID that was involved in the violation */
        hwGetGlobalRegField(QD_REG_VTU_OPERATION,0,4,&data);
        //data=(ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_OPERATION) & 0x0F);

        entry->DBNum = (GT_U8)(data & 0xF);

        /* get the VID that was involved in the violation */

        hwReadGlobalReg(QD_REG_VTU_VID_REG,&data);
        //data=ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_VID_REG);

        /* Get the vid - bits 0-11 */
        entry->vid   = data & 0xFFF;


    } /* end of service violations */

    /* If the operation is a get next operation wait for the response   */
    if(vtuOp == GET_NEXT_ENTRY)
    {
        entry->vidExInfo.useVIDFPri = GT_FALSE;
        entry->vidExInfo.vidFPri = 0;

        entry->vidExInfo.useVIDQPri = GT_FALSE;
        entry->vidExInfo.vidQPri = 0;

        entry->vidExInfo.vidNRateLimit = GT_FALSE;

        entry->sid = 0;
           entry->vidPolicy = GT_FALSE;

        /* Wait until the VTU in ready. */
        data = 1;
        while(data == 1)
        {
        	hwGetGlobalRegField(QD_REG_VTU_OPERATION,15,1,&data);
        	//data=(((ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_OPERATION))>>15) & 0x1);
        }


        /****************** get the vid *******************/


        hwReadGlobalReg(QD_REG_VTU_VID_REG,&data);
        //hwGetGlobalRegField(QD_REG_VTU_OPERATION,0,16,&data);
        //data=ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_VID_REG);
        DEBUG_MSG(DEBUG_QD,"....Read from global register: regAddr 0x%x, data 0x%x.\r\n",QD_REG_VTU_VID_REG,data);

        /* the vid is bits 0-11 */
        entry->vid   = data & 0xFFF;

        /* the vid valid is bits 12 */
        *valid   = (data >> 12) & 1;

        if (*valid == 0)
        {
        	DEBUG_MSG(DEBUG_QD,"*valid == 0\r\n");
            return GT_OK;
        }
        /****************** get the SID *******************/
        //if(IS_IN_DEV_GROUP(dev,DEV_802_1S_STU))
        if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
        {
            hwReadGlobalReg((GT_U8)(QD_REG_STU_SID_REG),&data);
            entry->sid = data & 0x3F;
        }
        /****************** get the DBNum *******************/
        //if(IS_IN_DEV_GROUP(dev,DEV_FID_REG))
        if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
            hwReadGlobalReg((GT_U8)(QD_REG_VTU_FID_REG),&data);

            //if(IS_IN_DEV_GROUP(dev,DEV_POLICY))
            if(get_marvell_id() == DEV_88E6240)
            {
                entry->vidPolicy = (data >> 12) & 0x1;
            }
            entry->DBNum = data & 0xFFF;

        }else{
        	//6095
            hwGetGlobalRegField(QD_REG_VTU_OPERATION,0,4,&data);
			//data=((ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_OPERATION)) & 0x0F);
            entry->DBNum = data & 0xF;


            hwGetGlobalRegField(QD_REG_VTU_OPERATION,8,4,&data);
			//data=(((ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_OPERATION))>>8) & 0x0F);
            entry->DBNum |= ((data & 0xF) << 4);
        }




        /****************** get the MemberTagP *******************/
        //retVal = hwReadGlobalReg(dev,QD_REG_VTU_DATA1_REG,&data);
		data=ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_DATA1_REG);

        /* get data from data register for ports 0 to 2 */
        entry->vtuData.memberTagP[0]  =  data & 3 ;
        entry->vtuData.memberTagP[1]  = (data >> 4) & 3 ;
        entry->vtuData.memberTagP[2]  = (data >> 8) & 3 ;
        entry->vtuData.portStateP[0]  = (data >> 2) & 3 ;
        entry->vtuData.portStateP[1]  = (data >> 6) & 3 ;
        entry->vtuData.portStateP[2]  = (data >> 10) & 3 ;

        /****************** for the switch more than 3 ports *****************/
        //if(dev->maxPorts > 3)
        if(1)
        {
            /* fullsail has 3 ports, clippership has 7 prots */
            entry->vtuData.memberTagP[3]  = (data >>12) & 3 ;
            entry->vtuData.portStateP[3]  = (data >>14) & 3 ;

            /* get data from data register for ports 4 to 6 */
            hwReadGlobalReg(QD_REG_VTU_DATA2_REG,&data);
            //data=ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_DATA2_REG);


            entry->vtuData.memberTagP[4]  = data & 3 ;
            entry->vtuData.memberTagP[5]  = (data >> 4) & 3 ;
            entry->vtuData.portStateP[4]  = (data >> 2) & 3 ;
            entry->vtuData.portStateP[5]  = (data >> 6) & 3 ;

            //if(dev->maxPorts > 6)
                entry->vtuData.memberTagP[6]  = (data >> 8) & 3 ;
                entry->vtuData.portStateP[6]  = (data >> 10) & 3 ;



        }
        /****************** upto 7 port switch *******************/

        /****************** for the switch more than 7 ports *****************/

        //if(dev->maxPorts > 7)
        if(MV_PORT_NUM>7)
        {
            /* fullsail has 3 ports, clippership has 7 prots */
            entry->vtuData.memberTagP[7]  = (data >>12) & 3 ;
            entry->vtuData.portStateP[7]  = (data >>14) & 3 ;

            /* get data from data register for ports 4 to 6 */
            //hwReadGlobalReg(QD_REG_VTU_DATA3_REG,&data);
            data=ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_DATA3_REG);

            entry->vtuData.memberTagP[8]  = data & 3 ;
            entry->vtuData.memberTagP[9]  = (data >> 4) & 3 ;
            entry->vtuData.portStateP[8]  = (data >> 2) & 3 ;
            entry->vtuData.portStateP[9]  = (data >> 6) & 3 ;

            //if(dev->maxPorts > 10)
            if(1)
            {
                entry->vtuData.memberTagP[10]  = (data >> 8) & 3 ;
                entry->vtuData.portStateP[10]  = (data >> 10) & 3 ;
            }

            //if (IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
            if(1)
            {
                if (data & 0x8000)
                {
                    entry->vidPriOverride = GT_TRUE;
                    entry->vidPriority = (data >> 12) & 0x7;
                }
                else
                {
                    entry->vidPriOverride = GT_FALSE;
                    entry->vidPriority = 0;
                }
            }

        }
        else
        	//if (IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
        	if(1)
			{
				/* get data from data register for ports 4 to 6 */
			   //retVal = hwReadGlobalReg(dev,QD_REG_VTU_DATA3_REG,&data);
				data=ETH_ReadPHYRegister(GlobalRegisters,QD_REG_VTU_DATA3_REG);

				if (data & 0x8000)
				{
					entry->vidPriOverride = GT_TRUE;
					entry->vidPriority = (data >> 12) & 0x7;
				}
				else
				{
					entry->vidPriOverride = GT_FALSE;
					entry->vidPriority = 0;
				}

			}

        /****************** upto 11 ports switch *******************/

    } /* end of get next entry */


    return GT_OK;
}


GT_STATUS gvtuGetEntryFirst
(
    OUT GT_VTU_ENTRY    *vtuEntry
)
{
    GT_U8               valid;
    GT_STATUS           retVal;
    GT_U8               port;
    GT_LPORT               lport;
    GT_VTU_ENTRY        entry;

    DEBUG_MSG(DEBUG_QD,"gvtuGetEntryFirst Called.\r\n");

    entry.vid = 0xFFF;
    entry.DBNum = 0;

    retVal = vtuOperationPerform(GET_NEXT_ENTRY,&valid, &entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed (vtuOperationPerform returned GT_FAIL).\r\n");
        return retVal;
    }

    /* retrive the value from the operation */

    if((entry.vid == 0xFFF) && (valid == 0)){
    	DEBUG_MSG(DEBUG_QD,"Failed (vtuOperationPerform returned GT_NO_SUCH).\r\n");
        return GT_NO_SUCH;
    }

    vtuEntry->DBNum = entry.DBNum;
    vtuEntry->vid   = entry.vid;

    vtuEntry->vidPriOverride = entry.vidPriOverride;
    vtuEntry->vidPriority = entry.vidPriority;

    vtuEntry->vidPolicy = entry.vidPolicy;
    vtuEntry->sid = entry.sid;

    vtuEntry->vidExInfo.useVIDFPri = entry.vidExInfo.useVIDFPri;
    vtuEntry->vidExInfo.vidFPri = entry.vidExInfo.vidFPri;
    vtuEntry->vidExInfo.useVIDQPri = entry.vidExInfo.useVIDQPri;
    vtuEntry->vidExInfo.vidQPri = entry.vidExInfo.vidQPri;
    vtuEntry->vidExInfo.vidNRateLimit = entry.vidExInfo.vidNRateLimit;

    for(lport=0; lport<11; lport++)
    {
        port = lport2port(lport);
        vtuEntry->vtuData.memberTagP[lport]=memberTagConversionForApp(entry.vtuData.memberTagP[port]);
        vtuEntry->vtuData.portStateP[lport]=entry.vtuData.portStateP[port];
        DEBUG_MSG(DEBUG_QD,"memberTagConversionForApp\r\n");
    }

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


uint8_t gvlnSetPortVlanDot1qMode(uint8_t Port,GT_DOT1Q_MODE mode){
	GT_U8           phyPort;        /* Physical port.               */
	DEBUG_MSG(DEBUG_QD,"gvlnSetPortVlanDot1qMode Called.\r\n");
	phyPort = lport2port(Port);
	/* check if device supports this feature */
	hwSetPortRegField(phyPort,QD_REG_PORT_CONTROL2,10,2,(GT_U16)mode );

	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
	return GT_OK;
}



/*******************************************************************************
* gvtuAddEntry
*
* DESCRIPTION:
*       Creates the new entry in VTU table based on user input.
*
* INPUTS:
*       vtuEntry    - vtu entry to insert to the VTU.
*
*******************************************************************************/
GT_STATUS gvtuAddEntry(GT_VTU_ENTRY *vtuEntry){
    GT_U8               valid;
    GT_STATUS           retVal;
    GT_U8           	port;
    GT_VTU_ENTRY     	tmpVtuEntry;
    GT_VTU_ENTRY        entry;
    GT_BOOL             found;
    int                	count = 5000;
    uint8_t 			lport;

    DEBUG_MSG(DEBUG_QD,"gvtuAddEntry Called.\r\n");

    entry.DBNum = vtuEntry->DBNum;
    entry.vid   = vtuEntry->vid;

	entry.vidPriOverride = vtuEntry->vidPriOverride;
	entry.vidPriority = vtuEntry->vidPriority;

	entry.vidPolicy = GT_FALSE;

	if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
		entry.sid = vtuEntry->sid;
	else
		entry.sid = 0;

	entry.vidExInfo.useVIDFPri = 0;
	entry.vidExInfo.vidFPri = 0;
	entry.vidExInfo.useVIDQPri = 0;
	entry.vidExInfo.vidQPri = 0;
	entry.vidExInfo.vidNRateLimit = 0;


    valid = 1; /* for load operation */

    for(port=0; port<MV_PORT_NUM; port++)
    {
        //lport = GT_PORT_2_LPORT(port);
    	lport = port2lport(port);
        if(lport == GT_INVALID_PORT)
        {
            entry.vtuData.memberTagP[port] = memberTagConversionForDev(NOT_A_MEMBER);
            entry.vtuData.portStateP[port] = 0;
        }
        else
        {
        	entry.vtuData.memberTagP[port] = memberTagConversionForDev(vtuEntry->vtuData.memberTagP[lport]);
        	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
        		entry.vtuData.portStateP[port] = vtuEntry->vtuData.portStateP[lport];
        	else
        		entry.vtuData.portStateP[port] = 0;
        }
    }
//??
    retVal = vtuOperationPerform(LOAD_PURGE_ENTRY,&valid, &entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed (vtuOperationPerform returned GT_FAIL).\r\n");
        return retVal;
    }

    /* verify that the given entry has been added */
    tmpVtuEntry.vid = vtuEntry->vid;
    tmpVtuEntry.DBNum = vtuEntry->DBNum;

    if((retVal = gvtuFindVidEntry(&tmpVtuEntry,&found)) != GT_OK)
    {
        while(count--);
        if((retVal = gvtuFindVidEntry(&tmpVtuEntry,&found)) != GT_OK)
        {
        	DEBUG_MSG(DEBUG_QD,"Added entry cannot be found\r\n");
            return retVal;
        }
    }
    if(found == GT_FALSE)
    {
    	DEBUG_MSG(DEBUG_QD,"Added entry cannot be found\r\n");
        return GT_FAIL;
    }

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}

GT_U8 memberTagConversionForDev(GT_U8  tag){
GT_U8 convTag=tag;
    /* check if memberTag needs to be converted */
//    if (!((IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)) ||
//        (IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
//		(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))))
//        return tag;
if(1){
    switch(tag)
    {
        case MEMBER_EGRESS_UNMODIFIED:
                convTag = 0;
                break;
        case NOT_A_MEMBER:
                convTag = 3;
                break;
        case MEMBER_EGRESS_UNTAGGED:
                convTag = 1;
                break;
        case MEMBER_EGRESS_TAGGED:
                convTag = 2;
                break;
        default:
        		DEBUG_MSG(DEBUG_QD,"Unknown Tag (%#x) from App. !!!.\r\n",tag);
                convTag = 0xFF;
                break;
    }
}
    return convTag;
}
GT_U8 memberTagConversionForApp(GT_U8  tag){
    GT_U8 convTag;

    /* check if memberTag needs to be converted */
//    if (!((IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)) ||
//        (IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
//		(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))))
//        return tag;

    switch(tag)
    {
        case 0:
                convTag = MEMBER_EGRESS_UNMODIFIED;
                break;
        case 1:
                convTag = MEMBER_EGRESS_UNTAGGED;
                break;
        case 2:
                convTag = MEMBER_EGRESS_TAGGED;
                break;
        case 3:
                convTag = NOT_A_MEMBER;
                break;
        default:
        		DEBUG_MSG(DEBUG_QD,"Unknown Tag (%#x) from Device !!!.\r\n",tag);
                convTag = 0xFF;
                break;

    }

    return convTag;
}


GT_STATUS gvtuFindVidEntry(GT_VTU_ENTRY  *vtuEntry,GT_BOOL *found)
{
    GT_U8               valid;
    GT_STATUS           retVal;
    GT_U8               port;
    GT_LPORT            lport;
    GT_VTU_ENTRY        entry;

    DEBUG_MSG(DEBUG_QD,"gvtuFindVidEntry Called.\r\n");

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,1, DEV_802_1Q)) != GT_OK)
    //  return retVal;

    *found = GT_FALSE;

    /* Decrement 1 from vid    */
    entry.vid   = vtuEntry->vid-1;
    valid = 0; /* valid is not used as input in this operation */
    entry.DBNum = vtuEntry->DBNum;

    retVal = vtuOperationPerform(GET_NEXT_ENTRY,&valid, &entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed (vtuOperationPerform returned GT_FAIL).\r\n");
        return retVal;
    }

    /* retrive the value from the operation */

    if( (entry.vid !=vtuEntry->vid) | (valid !=1) )
    {
    	 DEBUG_MSG(DEBUG_QD,"entry.vid %d\r\n",entry.vid);
    	 DEBUG_MSG(DEBUG_QD,"vtuEntry->vid %d\r\n",vtuEntry->vid);
    	 DEBUG_MSG(DEBUG_QD,"valid %d\r\n",valid);
    	 DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
         return GT_NO_SUCH;
    }

    vtuEntry->DBNum = entry.DBNum;

    vtuEntry->vidPriOverride = entry.vidPriOverride;
    vtuEntry->vidPriority = entry.vidPriority;

    vtuEntry->vidPolicy = entry.vidPolicy;
    vtuEntry->sid = entry.sid;

    vtuEntry->vidExInfo.useVIDFPri = entry.vidExInfo.useVIDFPri;
    vtuEntry->vidExInfo.vidFPri = entry.vidExInfo.vidFPri;
    vtuEntry->vidExInfo.useVIDQPri = entry.vidExInfo.useVIDQPri;
    vtuEntry->vidExInfo.vidQPri = entry.vidExInfo.vidQPri;
    vtuEntry->vidExInfo.vidNRateLimit = entry.vidExInfo.vidNRateLimit;

    for(lport=0; lport<MV_PORT_NUM; lport++)
    {
        port = lport2port(lport);
        //vtuEntry->vtuData.memberTagP[lport]=MEMBER_TAG_CONV_FOR_APP(dev,entry.vtuData.memberTagP[port]);
    	vtuEntry->vtuData.memberTagP[lport]=memberTagConversionForApp(entry.vtuData.memberTagP[port]);
        vtuEntry->vtuData.portStateP[lport]=entry.vtuData.portStateP[port];
    }

    *found = GT_TRUE;

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}
GT_U8 lport2port(GT_LPORT     port){

	GT_U16    portVec=(1 << MV_PORT_NUM) - 1;
    GT_U8    hwPort, tmpPort;

    tmpPort = hwPort = 0;

    while (portVec)
    {
        if(portVec & 0x1)
        {
            if((GT_LPORT)tmpPort == port)
                break;
            tmpPort++;
        }
        hwPort++;
        portVec >>= 1;
    }

    if (!portVec)
        hwPort = GT_INVALID_PORT;

    return hwPort;
}

GT_LPORT port2lport(GT_U8  hwPort){

	GT_U16    portVec=(1 << MV_PORT_NUM) - 1;
    GT_U8        tmpPort,port;

    port = 0;

    if (hwPort == GT_INVALID_PORT)
        return (GT_LPORT)hwPort;

    if (!GT_IS_PORT_SET(portVec, hwPort))
        return (GT_LPORT)GT_INVALID_PORT;

    for (tmpPort = 0; tmpPort <= hwPort; tmpPort++)
    {
        if(portVec & 0x1)
        {
            port++;
        }
        portVec >>= 1;
    }

    return (GT_LPORT)port-1;
}

GT_STATUS gvlnSetPortVid(GT_LPORT port,GT_U16 vid){
    GT_U8           phyPort;        /* Physical port.               */

    DEBUG_MSG(DEBUG_QD,"gvlnSetPortVid Called.\r\n");
    phyPort = lport2port(port);

    hwSetPortRegField(phyPort,QD_REG_PVID,0,12, vid);
    //ETH_WritePHYRegister(phyPort,QD_REG_PVID,(uint16_t)(vid & 0x0FFF));

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}

/*******************************************************************************
* gvtuGetEntryNext
*
* DESCRIPTION:
*       Gets next lexicographic VTU entry from the specified VID.
*
* INPUTS:
*       vtuEntry - the VID to start the search.
*
* OUTPUTS:
*       vtuEntry - match VTU  entry.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - on error or entry does not exist.
*       GT_NO_SUCH - no more entries.
*
* COMMENTS:
*       Search starts from the VID specified by the user.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gvtuGetEntryNext
(
    INOUT GT_VTU_ENTRY  *vtuEntry
)
{
    GT_U8               valid;
    GT_STATUS           retVal;
    GT_U8               port;
    GT_LPORT            lport;
    GT_VTU_ENTRY        entry;

    DEBUG_MSG(DEBUG_QD,"gvtuGetEntryNext Called.\r\n");

    entry.DBNum = vtuEntry->DBNum;
    entry.vid   = vtuEntry->vid;
    valid = 0;

    retVal = vtuOperationPerform(GET_NEXT_ENTRY,&valid, &entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed (vtuOperationPerform returned GT_FAIL).\r\n");
        return retVal;
    }

    /* retrieve the value from the operation */

    if((entry.vid == 0xFFF) && (valid == 0)){
    	DEBUG_MSG(DEBUG_QD,"Failed (vtuOperationPerform returned GT_NO_SUCH).\r\n");
        return GT_NO_SUCH;
    }

    vtuEntry->DBNum = entry.DBNum;
    vtuEntry->vid   = entry.vid;

    vtuEntry->vidPriOverride = entry.vidPriOverride;
    vtuEntry->vidPriority = entry.vidPriority;

    vtuEntry->vidPolicy = entry.vidPolicy;
    vtuEntry->sid = entry.sid;

    vtuEntry->vidExInfo.useVIDFPri = entry.vidExInfo.useVIDFPri;
    vtuEntry->vidExInfo.vidFPri = entry.vidExInfo.vidFPri;
    vtuEntry->vidExInfo.useVIDQPri = entry.vidExInfo.useVIDQPri;
    vtuEntry->vidExInfo.vidQPri = entry.vidExInfo.vidQPri;
    vtuEntry->vidExInfo.vidNRateLimit = entry.vidExInfo.vidNRateLimit;

    for(lport=0; lport<11; lport++)
    {
        port = lport2port(lport);
        vtuEntry->vtuData.memberTagP[lport]=memberTagConversionForApp(entry.vtuData.memberTagP[port]);
        vtuEntry->vtuData.portStateP[lport]=entry.vtuData.portStateP[port];
    }

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}

#if 0
/*******************************************************************************
* gprtSetDiscardUntagged
*
* DESCRIPTION:
*        When this bit is set to a one, all non-MGMT frames that are processed as
*        Untagged will be discarded as they enter this switch port. Priority only
*        tagged frames (with a VID of 0x000) are considered tagged.
*
* INPUTS:
*        port - the logical port number.
*        mode - GT_TRUE to discard untagged frame, GT_FALSE otherwise
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtSetDiscardUntagged
(
    IN GT_LPORT     port,
    IN GT_BOOL      mode
)
{
    GT_U16          data;
    GT_STATUS       retVal=0;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */


    DBG_INFO(("gprtSetDiscardUntagged Called.\r\n"));

    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if the given Switch supports this feature. */


    /* translate BOOL to binary */
    BOOL_2_BIT(mode, data);


    /* Set DiscardUnTagged. */
    retVal = hwSetPortRegField(hwPort, QD_REG_PORT_CONTROL2, 8, 1, data);
    /*tmp=ETH_ReadPHYRegister(hwPort,QD_REG_PORT_CONTROL2);
    tmp &=~0x100;
    tmp |=((data<<8) & 0x100);
    ETH_WritePHYRegister(hwPort,QD_REG_PORT_CONTROL2,tmp);*/


    DBG_INFO(("OK.\r\n"));

    return retVal;
}
#endif

#if 0
/*****************************************************************************
* sampleAdmitOnlyTaggedFrame
*
* DESCRIPTION:
*        This routine will show how to configure a port to accept only vlan
*        tagged frames.
*        This routine assumes that 802.1Q has been enabled for the given port.
*
* INPUTS:
*       port - logical port to be configured.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*
* COMMENTS:
*        Some device support Discard Untagged feature. If so, gprtSetDiscardUntagged
*        function will do the work.
*
*******************************************************************************/
GT_STATUS sampleAdmitOnlyTaggedFrame(GT_LPORT port)
{
    GT_STATUS status;
    GT_VTU_ENTRY vtuEntry;
    int i;

    /*
     *    0) If device support gprtSetDiscardUntagged, call the function.
    */
    status = gprtSetDiscardUntagged(port, GT_TRUE);
    switch (status)
    {
        case GT_OK:
        	DBG_INFO(("Done.\r\n"));
            return status;
        case GT_NOT_SUPPORTED:
        	DBG_INFO(("Try other method.\r\n"));
            break;
        default:
        	DBG_INFO(("Failure accessing device.\r\n"));
            return status;
    }


    /*
     *    1) Add VLAN ID 0xFFF with the given port as a member.
    */
    gtMemSet(&vtuEntry,0,sizeof(GT_VTU_ENTRY));
    vtuEntry.DBNum = 0;
    vtuEntry.vid = 0xFFF;
    for(i=0; i<11; i++)
    {
        vtuEntry.vtuData.memberTagP[i] = NOT_A_MEMBER;
    }
    vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_TAGGED;

    if((status = gvtuAddEntry(&vtuEntry)) != GT_OK)
    {
    	DBG_INFO(("gvtuAddEntry returned fail.\r\n"));
        return status;
    }

    /*
     *    2) Configure the default vid for the given port with VID 0xFFF
    */
    if((status = gvlnSetPortVid(port,0xFFF)) != GT_OK)
    {
    	DBG_INFO(("gvlnSetPortVid returned fail.\r\n"));
        return status;
    }

    return GT_OK;

}

/*****************************************************************************
* sample802_1qSetup
*
* DESCRIPTION:
*        This routine will show how to configure the switch device so that it
*        can be a Home Gateway. This example assumes that all the frames are not
*        VLAN-Tagged.
*        1) to clear VLAN ID Table,
*         2) to enable 802.1Q in SECURE mode for each port except CPU port,
*        3) to enable 802.1Q in FALL BACK mode for the CPU port.
*        4) to add VLAN ID 1 with member port 0 and CPU port
*        (untagged egress),
*        5) to add VLAN ID 2 with member the rest of the ports and CPU port
*        (untagged egress),
*        6) to configure the default vid of each port:
*        Port 0 have PVID 1, CPU port has PVID 3, and the rest ports have PVID 2.
*        Note: CPU port's PVID should be unknown VID, so that QuarterDeck can use
*        VlanTable (header info) for TX.
*
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*
* COMMENTS:
*        WARNING!!
*        If you create just two VLAN for this setup, Trailer mode or Header mode
*        for the CPU port has to be enabled and Ethernet driver which connects to
*        CPU port should understand VLAN-TAGGING, Trailer mode, or Header mode.
*
*******************************************************************************/
GT_STATUS sample802_1qSetup(void){
    GT_STATUS status;
    GT_DOT1Q_MODE mode;
    GT_VTU_ENTRY vtuEntry;
    GT_U16 vid;
    GT_LPORT port;
    int i;

    /*
     *    1) Clear VLAN ID Table
    */
    if((status = gvtuFlush()) != GT_OK)
    {
    	MSG_PRINT("gvtuFlush returned fail.\r\n");
    	return status;
    }

    /*
     *    2) Enable 802.1Q for each port as GT_SECURE mode except CPU port.
    */
    mode = GT_SECURE;
    for(i=0; i<11; i++)
    {
        port = i;
        if (port == 10)
            continue;

        if((status = gvlnSetPortVlanDot1qMode(port, mode)) != GT_OK)
        {
        	MSG_PRINT("gvlnSetPortVlanDot1qMode return Failed\r\n");
            return status;
        }
    }

    /*
     *    3) Enable 802.1Q for CPU port as GT_FALLBACK mode
    */
    if((status = gvlnSetPortVlanDot1qMode(10, GT_FALLBACK)) != GT_OK)
    {
    	MSG_PRINT("gvlnSetPortVlanDot1qMode return Failed\r\n");
        return status;
    }

    /*
     *    4) Add VLAN ID 1 with Port 0 and CPU Port as members of the Vlan.
    */
    /*порт 1, порт 2 принадлежит VLAN 1 */
    gtMemSet(&vtuEntry,0,sizeof(GT_VTU_ENTRY));
    vtuEntry.DBNum = 0;
    vtuEntry.vid = 1;//1
    for(i=0; i<11; i++)
    {
        port = i;
        if((i==0) || (i==2) || (port == 10))//
            vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_UNTAGGED;
        else
            vtuEntry.vtuData.memberTagP[port] = NOT_A_MEMBER;
    }

    if((status = gvtuAddEntry(&vtuEntry)) != GT_OK)
    {
    	MSG_PRINT("gvtuAddEntry returned fail.\r\n");
        return status;
    }
    /*
     *    5) Add VLAN ID 1 with the rest of the Ports and CPU Port as members of
     *    the Vlan.
    */
    /*остальные порты принадлежат vlan2 */
    gtMemSet(&vtuEntry,0,sizeof(GT_VTU_ENTRY));
    vtuEntry.DBNum = 0;
    vtuEntry.vid = 2;
    for(i=0; i<11; i++)
    {
        port = i;
        if((port == 0)||(port == 2))
            vtuEntry.vtuData.memberTagP[port] = NOT_A_MEMBER;
        else
        	if (port==8)
        		vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_TAGGED;
        	else
        		vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_UNTAGGED;
    }

    if((status = gvtuAddEntry(&vtuEntry)) != GT_OK)
    {
    	MSG_PRINT("gvtuAddEntry returned fail.\r\n");
        return status;
    }
    /*
     *    6) Configure the default vid for each port.
     *    Port 0 has PVID 1, CPU port has PVID 3, and the rest ports have PVID 2.
    */
    for(i=0; i<11; i++)
    {
        port = i;
        if(i==0)
            vid = 1;
        else if(port == 10)
            vid = 1;
        else if(port == 2)
            vid = 1;
        else
           	vid = 2;

        if((status = gvlnSetPortVid(port,vid)) != GT_OK)
        {
        	MSG_PRINT("gvlnSetPortVid returned fail.\r\n");
        	return status;
        }
    }
    return GT_OK;
}
#endif


/*****************************************************************************
* VLAN SETUP
*******************************************************************************/
GT_STATUS VLAN_setup(void){

    GT_STATUS status;
    GT_VTU_ENTRY vtuEntry;
    GT_U16 vid;
    GT_LPORT port;
    int lport;
    u16 data;
    int i,j;
    u8 cnt_unt;

    DEBUG_MSG(DEBUG_QD,"start vlan setup\r\n");



	  if(get_vlan_sett_state()==0)
		  return GT_OK;

	  if((get_vlan_sett_vlannum()>MAXVlanNum)||(get_vlan_sett_vlannum() == 0)){
		  set_vlan_sett_vlannum(1);
	  }


	  //проверяем, что 1 порт имеет не более 1 состяния untaged
	  for(j=0;j<(ALL_PORT_NUM);j++){
		  cnt_unt=0;
		  for(i=0;i<get_vlan_sett_vlannum();i++){
			  if(get_vlan_port_state(i,j)==2){//port untagged
				  cnt_unt++;
				  set_vlan_sett_dvid(j,get_vlan_vid(i));//untagged port == default vid
			  }
		  }
	  }


/*
 *
 *    1) Clear VLAN ID Table
*/
    if((status = gvtuFlush()) != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"gvtuFlush returned fail.\r\n");
    	return status;
    }

    /*
     *    2) Enable 802.1Q for each port as GT_SECURE mode except CPU port.
    */
    VLAN1QPortConfig();




/*ADD VLAN to VTU Table */

//Marvell bug:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//Agate rev A0 and A1 devices do not allow VTU entries
//unless a valid dummy STU entry is made to the STU Table
if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
    /* Wait until the VTU in ready. */
    data = 1;
    while(data == 1)
      hwGetGlobalRegField(QD_REG_VTU_OPERATION,15,1,&data);
    //set SID = 0
    data = 0;
    hwWriteGlobalReg(QD_REG_STU_SID_REG,data);
    //set Valid bit
    data = 0x1000;
    hwWriteGlobalReg(QD_REG_VTU_VID_REG,data);
    //set port state (disabled)
    data = 0;
    hwWriteGlobalReg(QD_REG_VTU_DATA1_REG,data);
    hwWriteGlobalReg(QD_REG_VTU_DATA2_REG,data);
    //dont owerride
    hwWriteGlobalReg(QD_REG_VTU_DATA3_REG,data);
    //VTU op - VTU Busy + STU Load
    data = 0xD000;
    hwWriteGlobalReg(QD_REG_VTU_OPERATION,data);
}


for(uint16_t k=0;k<get_vlan_sett_vlannum();k++){

    gtMemSet(&vtuEntry,0,sizeof(GT_VTU_ENTRY));
    vtuEntry.DBNum = 0;
    vtuEntry.vid = get_vlan_vid(k);
    for(i=0; i<MV_PORT_NUM; i++)
    {
        port = i;
        if((port==CPU_PORT)&&(get_vlan_sett_mngt()==get_vlan_vid(k))){
        	vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_UNTAGGED;
        }else{
        	lport = F2L_port_conv(i);
        	if(lport != -1){
				switch(get_vlan_port_state(k,(u8)lport)){
					case 2:
						vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_UNTAGGED;
						break;
					case 1:
						vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_UNMODIFIED;
						break;
					case 3:
						vtuEntry.vtuData.memberTagP[port] = MEMBER_EGRESS_TAGGED;
						break;
					default:
						vtuEntry.vtuData.memberTagP[port] = NOT_A_MEMBER;
						break;
			   }
        	}
        }
    }

    if((status = gvtuAddEntry(&vtuEntry)) != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"gvtuAddEntry returned fail.\r\n");
        return status;
    }
}



    /*
     *    6) Configure the default vid for each port.
     *    Port 0 has PVID 1, CPU port has PVID 3, and the rest ports have PVID 2.
    */
    for(i=0; i<MV_PORT_NUM; i++)
    {
        port = i;
        vid = 1;
        if(port==CPU_PORT)
        	vid=get_vlan_sett_mngt();
        else{
        	lport = F2L_port_conv(i);
        	if(lport != -1)
        		vid =  get_vlan_sett_dvid(lport);//default VLAN
        }

         if((status = gvlnSetPortVid(port,vid)) != GT_OK)
        {
        	DEBUG_MSG(DEBUG_QD,"gvlnSetPortVid returned fail.\r\n");
        	return status;
        }
    }
    DEBUG_MSG(DEBUG_QD,"vlan configuration compleat succesfull.\r\n");
    return GT_OK;
}




/***устанавливаем режим работы порта****/
uint8_t VLAN1QPortConfig(void){
 /*
 *    2) Enable 802.1Q for each port as GT_SECURE mode except CPU port.
*/
int lport;
uint8_t status;


/*GT_DISABLE = 0,
GT_FALLBACK,
GT_CHECK,
GT_SECURE
*/

if(get_vlan_sett_state()==1){

	for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		//add all ports
		lport = L2F_port_conv(i);
		//если включен VLAN Trunk
		if(get_vlan_trunk_state()){
			if((status = gvlnSetPortVlanDot1qMode(lport, get_vlan_sett_port_state(i))) != GT_OK)
				return status;
		}
		else{
			if((status = gvlnSetPortVlanDot1qMode(lport, GT_SECURE)) != GT_OK)
				return status;
		}
	}

   //3) Enable 802.1Q for CPU port as GT_FALLBACK mode
   if((status = gvlnSetPortVlanDot1qMode(CPU_PORT, GT_FALLBACK)) != GT_OK)
   	  return status;

   return 0;
}
else
    return 1;

return 0;
}




/*****************************************************************************
* sampleDisplayVIDTable
*
* DESCRIPTION:
*        This routine will show how to enumerate each vid entry in the VTU table
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
* COMMENTS:
*
*******************************************************************************/
GT_STATUS sampleDisplayVIDTable(void){
    GT_STATUS status;
    GT_VTU_ENTRY vtuEntry;
    GT_LPORT port;
    int portIndex;
    u16 mode;

    gtMemSet(&vtuEntry,0,sizeof(GT_VTU_ENTRY));
    vtuEntry.vid = 0xfff;
    if((status = gvtuGetEntryFirst(&vtuEntry)) != GT_OK)
    {
    	DEBUG_MSG(PRINTF_DEB,"gvtuGetEntryCount returned fail.\r\n");
        return status;
    }

    DEBUG_MSG(PRINTF_DEB,"DBNum:%d, VID:%i \r\n",vtuEntry.DBNum,vtuEntry.vid);

    for(portIndex=0; portIndex<MV_PORT_NUM; portIndex++)
    {
        port = portIndex;

        DEBUG_MSG(PRINTF_DEB,"Tag %lu:%x  ",port,vtuEntry.vtuData.memberTagP[port]);
    }

    DEBUG_MSG(PRINTF_DEB,"\r\n");

    while(1)
    {
        if((status = gvtuGetEntryNext(&vtuEntry)) != GT_OK)
        {
        	DEBUG_MSG(PRINTF_DEB,"gvtuGetEntryNext break");
            break;
        }

        DEBUG_MSG(PRINTF_DEB,"DBNum:%d, VID:%i \r\n",vtuEntry.DBNum,vtuEntry.vid);

        for(portIndex=0; portIndex<11; portIndex++)
        {
            port = portIndex;

            DEBUG_MSG(PRINTF_DEB,"Tag %lu:%x  ",port,vtuEntry.vtuData.memberTagP[port]);
        }

        DEBUG_MSG(PRINTF_DEB,"\r\n");
    }

    for(portIndex=0; portIndex<11; portIndex++){
    	mode = 0;
    	hwGetPortRegField((u8)portIndex+0x10,QD_REG_PORT_CONTROL2,10,2,&mode);
        DEBUG_MSG(PRINTF_DEB,"Port %d:%x  ",portIndex,mode);
    }
    DEBUG_MSG(PRINTF_DEB,"\r\n");

    return GT_OK;
}



GT_STATUS SWU_VLAN_setup(void){
    /*GT_STATUS status;
    GT_VTU_ENTRY vtuEntry;
    GT_U16 vid;
    GT_LPORT port;
    int lport;
    u16 data;*/
    int i,j;
    //u8 cnt_unt;
    u8 port_state;
	DEBUG_MSG(DEBUG_QD,"start vlan setup\r\n");

	//set VLAN Ether Type
	Salsa2_WriteReg(VLAN_ETHER_TYPE_REG,0x81008100);

	if((get_vlan_sett_vlannum()>MAXVlanNum)||(get_vlan_sett_vlannum() == 0)){
		set_vlan_sett_vlannum(1);
	}


	//проверяем, что 1 порт имеет не более 1 состяния untaged
	for(j=0;j<ALL_PORT_NUM;j++){
		for(i=0;i<get_vlan_sett_vlannum();i++){
		  if(get_vlan_port_state(i,j)==2){//port untagged
			  set_vlan_sett_dvid(j,get_vlan_vid(i));//untagged port == default vid
		  }
		}
	}



	//настройка
	for(j=0;j<(ALL_PORT_NUM);j++){
		set_port_default_vid(j,get_vlan_sett_dvid(j));
	}

	for(i=0;i<get_vlan_sett_vlannum();i++){
		//set pointer for each vlan
		set_vlan_entry(get_vlan_vid(i),i,get_vlan_state(i));

		//config each entry
		//set port
		for(j=0;j<ALL_PORT_NUM;j++){
			port_state = get_vlan_port_state(i,j);
			set_vlan_port_state(i,j,port_state);
		}

		//enable IGMP Snooping acording to VLAN
		//set_vlan_igmp_mode(i,ENABLE);

		//set STP entry - 0
		set_vlan_stp_ptr(i,0);

		//set managment VLAN -- CPU is a member of this vlan
		if(get_vlan_vid(i) == get_vlan_sett_mngt()){
			set_vlan_mgmt_mode(i,ENABLE);
			set_cpu_port_vid(get_vlan_sett_mngt());
		}
		else{
			set_vlan_mgmt_mode(i,DISABLE);
		}

		//set vlan state
		set_vlan_valid(i,get_vlan_state(i));
	}

	//настройка port based vlan
	for(j=0;j<ALL_PORT_NUM;j++){
		set_port_default_vid(j,get_vlan_sett_dvid(j));
	}

	return GT_OK;
}

GT_STATUS SWU_pbvlan_setup(void){
	for(u8 j=0;j<(ALL_PORT_NUM);j++){
		set_port_default_vid(j,get_pb_vlan_swu_port(j));
	}
	return GT_OK;
}
