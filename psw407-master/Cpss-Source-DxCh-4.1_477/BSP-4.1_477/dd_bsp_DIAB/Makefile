# Makefile - make rules for Marvell MV78100, MV78200 
#
# Copyright 1999-2001 Wind River Systems, Inc.
# Copyright 1999-2001 ARM Limited
# Copyright 2004-2007 Marvell
#
# modification history
# --------------------
# 01e,19Sep07,E_N  update for Disco Dou
# 01d,23may02,m_h  Reduce ROM_SIZE for boards with 32 meg RAM (77901)
# 01c,20mar02,rec  Increase ROM size, SPR 74146
# 01b,14nov01,dat  RELEASE update
# 01a,25sep01,rec  created
#
# DESCRIPTION
# This file contains rules for building VxWorks for the ARM INTEGRATOR AP
# fited with an ARM946ES core module.
#
# INCLUDES
#     makeTarget
#*/
# General definitions
VENDOR      = Marvell
CPU_ARCH    = ARM
CPU_FAMILY  = ARM
ifeq ($(findstring vxworks-6.,$(WIND_PLATFORM)),vxworks-6.)
WR_IDE      = Wb
TGT_DIR  = $(WIND_BASE)/target
SRC_PATH =  .
else
WR_IDE      = Pid
SRC_PATH =  .
TGT_DIR  = $(WIND_BASE)\target
endif


#SRC_PATH = $(TGT_DIR)/config/mrvl_db78xxx_gnu
ifeq ($(PROJECT), )    
PROJECT=DD
endif

ifeq ($(PROJECT), KW)    
include $(SRC_PATH)/VxBsp/ArchArm/88F6XXX/Makefile.user 
BOARD       = db88F6281f
endif
ifeq ($(PROJECT), 6183)    
include $(SRC_PATH)/VxBsp/ArchArm/88F6183/Makefile.user 
BOARD       = db886183
endif
ifeq ($(PROJECT), DD)    
include $(SRC_PATH)/VxBsp/ArchArm/78XX0/Makefile.user
ifeq ($(BOARD),LION_RD)
BOARD	= LION_RD
else
BOARD	= AMC
endif

endif


ifeq ($(BUILD_TOOL), VFP)
    ifeq ($(WR_IDE), Wb)
	MV_DEFINE  += -DMV_VFP -march=armv5t -mfpu=vfp -mhard-float  -mfloat-abi=softfp  -Wa,-mfpu=vfp -Wa,-march=armv5t
    else
	TOOL_PREFIX = vfp
	MV_DEFINE  += -DMV_VFP  -mhard-float 
    endif
endif

IP_ENDIAN = LITTLE_ENDIAN

ifeq ($(ENDIAN), BE)
TOOL_SUFFIX = be
IP_ENDIAN = BIG_ENDIAN
endif

ifeq ($(MV_CPU), 946)
MV_CPU_NAME = 946ES
EXTRA_DEFINE   += -DCPU_946E -DARMMMU=ARMMMU_946E -DARMCACHE=ARMCACHE_946E -DCPU_FAMILY=ARM
else
MV_CPU_NAME = 926EJ
EXTRA_DEFINE   += -DCPU_926E -DARMMMU=ARMMMU_926E -DARMCACHE=ARMCACHE_926E -DCPU_FAMILY=ARM
endif

CPU     = ARMARCH$(ARMARCH_NUM)

##
## for diab environment cpss must set the following variables
## ----------------------------------------------------------
##
## set COMILER=DIAB
## set DIABLIB=c:\TornadoARM\host\diab
## set PATH=c:\TornadoArm\host\diab\WIN32\bin;%PATH% 
##

ifeq ($(COMPILER),DIAB)
COMPILER=DIAB
else
COMPILER=GNU
endif

ifeq ($(COMPILER),GNU)
TOOL    = $(TOOL_PREFIX)gnu$(TOOL_SUFFIX)
TOOL_FAMILY=gnu
else
TOOL    = $(TOOL_PREFIX)diab$(TOOL_SUFFIX)
TOOL_FAMILY=diab
ADDED_CFLAGS+= -D_DIAB_ -Xlint=0x10
endif

include $(TGT_DIR)/h/make/defs.bsp

ADDED_CFLAGS+= -DMV78XX0 -DDB_MV78200_A_AMC
ADDED_CFLAGS+= -Os -g
#ADDED_CFLAGS+= -O0  -g
#CC_OPTIM = -O2 -g

##
## diab options to disable optimization
##
#ADDED_CFLAGS+= -Xkill-opt=0xffffffff -g -D__diab_optimization_is_off__

#
## set this variable to NULL to skip linking appDemo
#
#USER_BASE=


## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.
#BOOTCONFIG=$(SRC_PATH)\bootConfig.c 
USRCONFIG= $(SRC_PATH)\usrConfig.c 
BOOTINIT = $(SRC_PATH)\bootInit.c

# Internal definitions

EXTRA_DEFINE += -Wcomment -DMV_VXWORKS -DMV$(CHIP) -DMV_$(CPU_ARCH) -DMV$(CHIP_DEFINE)  -DDB_$(CHIP) -DDB_MV$(CHIP_DEFINE)   \
                -DCPU_$(MV_CPU_NAME) -DMV_CPU_$(ENDIAN) $(MV_DEFINE) -DMV_SPI_BOOT -D$(BOARD)      \
                -DARMMMU=ARMMMU_$(MV_CPU)E -DARMCACHE=ARMCACHE_$(MV_CPU)E  -DIP_$(IP_ENDIAN) -DIP_PORT_VXWORKS=65 

#
# The constants ROM_TEXT_ADRS, ROM_SIZE, and RAM_HIGH_ADRS are defined
# in config.h and Makefile.
# All definitions for these constants must be identical.
#

ifeq ($(IMAGE), BOOTROM)
ROM_SIZE        = 00070004 # number of bytes of ROM space
else
ROM_SIZE        = 001f0004 # number of bytes of ROM space
endif

ROM_WARM_ADRS   = ffff0004 # ROM warm entry address
RAM_LOW_ADRS    = 00010000 # RAM text/data address
RAM_HIGH_ADRS   = 01800000 # RAM text/data address



# Main directory structure
COMMON_DIR      = $(SRC_PATH)/common
GND_DIR         = $(SRC_PATH)/common/gnd
GEN_ERRATA_DIR  = $(SRC_PATH)/common/errata
POOL_DIR        = $(SRC_PATH)/common/pool
OSSERVICES_BASE_DIR = $(SRC_PATH)/osServices
OSSERVICES_DIR  = $(SRC_PATH)/osServices/vxworks_oss
PSSSERVICES_DIR = $(SRC_PATH)/pssServices/vxWorks_oss
PSSSERVICES_BASE_DIR = $(SRC_PATH)/pssServices

SOC_UNITS     	= $(SRC_PATH)/Soc_units
VXBSP_DIR       = $(SRC_PATH)/VxBsp
 
# Marvell device- KW family components
SOC_BOARD_ENV_DIR    = $(MV_DEVICE_DIR)/boardEnv
SOC_ERRATA_DIR = $(MV_DEVICE_DIR)/boardEnv/errata
SOC_CPU_DIR          = $(MV_DEVICE_DIR)/cpu
SOC_DEVICE_DIR       = $(MV_DEVICE_DIR)/device
SOC_CTRL_ENV_DIR	 = $(MV_DEVICE_DIR)/ctrlEnv

# Marvell Soc units components
CESA_DIR	= $(SOC_UNITS)/cesa
CNTMR_DIR	= $(SOC_UNITS)/cntmr
ETH_DIR    	= $(SOC_UNITS)/eth
ETHPHY_DIR   	= $(SOC_UNITS)/eth-phy
GPP_DIR		= $(SOC_UNITS)/gpp
MFLASH_DIR    	= $(SOC_UNITS)/mflash
NFLASH_DIR   	= $(SOC_UNITS)/Nflash
NORFLASH_DIR 	= $(SOC_UNITS)/norflash
PCIIF_DIR	= $(SOC_UNITS)/pci-if
PEX_DIR		= $(SOC_UNITS)/pex
SATA_DIR	= $(SOC_UNITS)/sata
SFLASH_DIR      = $(SOC_UNITS)/sflash
SPI_DIR		= $(SOC_UNITS)/spi 
TWSI_DIR	= $(SOC_UNITS)/twsi
UART_DIR	= $(SOC_UNITS)/uart 
XOR_DIR		= $(SOC_UNITS)/xor
USB_DIR		= $(SOC_UNITS)/usb
IDMA_DIR       	= $(SOC_UNITS)/idma
PRESTERA_DIR    = $(SOC_UNITS)/prestera


# OS services
OSSERV_VXWORKS     = $(OSSERVICES_DIR)/VxWorks
OSSERV_ARCH_DIR    = $(OSSERVICES_DIR)/VxWorks/Arch$(CPU_ARCH)


VXBSP_ROOT_DIR	= $(SRC_PATH)/vxBsp
VXBSP_ARCH_DIR	= $(VXBSP_ROOT_DIR)/arch$(CPU_ARCH)
VXBSP_CHIP_DIR  = $(VXBSP_ARCH_DIR)/$(CHIP_DEFINE)
VXBSP_DEBUG_DIR = $(VXBSP_ROOT_DIR)/Debug

# Marvell Sata Driver
MRVL_SATA_DIR   = $(VXBSP_ROOT_DIR)/mrvlSata

BSP_API_DIR   = $(VXBSP_ROOT_DIR)/expandedBSP
# GEI/CACHE/MMU/MPU device driver
VXBSP_WRFILES_DIR = $(VXBSP_ROOT_DIR)/WindRiverFiles

# debug vfp math path
VXBSP_DBG_MATH_DIR     = $(VXBSP_ROOT_DIR)/math

###############################################################################
# GEI PCI device driver
ifeq ($(GEI), yes)
MV_DEFINE  += 	-DINCLUDE_GEI_END
endif
###############################################################################
# FEI PCI device driver
ifeq ($(FEI), yes)
MV_DEFINE  += 	-DINCLUDE_FEI_END
endif
###############################################################################
ifeq ($(VX_BUS), yes)
MV_DEFINE      += -DINCLUDE_VXBUS
endif
###############################################################################
# Yukon PCI device driver
SKGEY2_DIR  = $(VXBSP_ROOT_DIR)/skGeY2
INC_VXBSP_SKGEY2_DIR = -I$(VXBSP_ROOT_DIR)/skGeY2

SKGEY2_OBJS     = skaddr.o skcsum.o skdim.o skgehwt.o skgeinit.o \
                  skgesirq.o sklm80.o skqueue.o skrlmt.o sktimer.o sktwsi.o skvpd.o \
                  skxmac2.o sky2le.o skGeY2End.o skY2End.o sysSkGeY2End.o 

ifeq ($(IMAGE), BOOTROM)
	ifeq ($(INCLUDE_SGI_INTO_BOOTROM), Y)
		YUKON = Y
	else
		YUKON = N
	endif
else
	ifeq ($(INCLUDE_SGI_INTO_IMAGE), Y)
		YUKON = Y
	else
		YUKON = N
	endif
endif

ifeq ($(YUKON), Y)
    MV_DEFINE  += 	-DINCLUDE_SYSKONNECT 
endif

ifeq ($(INCLUDE_ASF), Y)
    MV_DEFINE  += 	-DSK_ASF
ifeq ($(INCLUDE_ASF_DEBUG), Y)
        MV_DEFINE  += 	-DDEBUG -DSK_DEBUG_CHKMOD=0x00000400 -DSK_DEBUG_CHKCAT=0x00000003
endif
    SKGEY2_OBJS     += skfops.o skgeasf.o skgespi.o skgespilole.o 
endif
###############################################################################


# Set the tool chain object directory according to required endiannes.
BSP_OBJ_DIR = $(VXBSP_ROOT_DIR)/lib/$(TOOL_VER)/obj$(CPU)$(TOOL)


###############################################################################
# CPU 946ES compatible:
# Tornado 2.2.1 cache library for 946ES does not support the Marvell ARM 
# compatible CPU cache architecture (one way). This is way we need to 
# link agains our cache objects.
#
# CPU 926EJ compatible:
# Tornado 2.2.1 926EJ patch is installed only for ARMARCH5. This is why, 
# in case of big endian (only in ARMARCH4), we need to link against our 
# MMU and cache objects.

ifeq ($(WR_IDE), Wb)
    
       CPU_OBJS += $(BSP_OBJ_DIR)/cacheAimFeroceonALib.obj  \
                   $(BSP_OBJ_DIR)/cacheAimFeroceonLib.obj  
else
        CPU_OBJS += $(BSP_OBJ_DIR)/cache$(MV_CPU)eALib.obj \
                    $(BSP_OBJ_DIR)/cache$(MV_CPU)eLib.obj  \
		    $(BSP_OBJ_DIR)/mmu$(MV_CPU)eALib.obj   \
		    $(BSP_OBJ_DIR)/mmu$(MV_CPU)eLib.obj
ifeq ($(MV_CPU),946)
	CPU_OBJS += $(BSP_OBJ_DIR)/vmMpuLib.obj $(BSP_OBJ_DIR)/vmData.obj 
endif
endif

ifeq ($(WR_IDE), Wb)
#IP_COM
IPNET2_DIR        = $(WIND_HOME)/components/ip_net2-6.5
IPAPPL_CFG_DIR    = $(IPNET2_DIR)/ipappl/config
IPAPPL_INC_DIR    = $(IPNET2_DIR)/ipappl/include
IPCOM_CFG_DIR     = $(IPNET2_DIR)/ipcom/config
IPCOM_INC_DIR     = $(IPNET2_DIR)/ipcom/include
IPCOM_VXCFG_DIR   = $(IPNET2_DIR)/ipcom/port/vxworks/config
IPCOM_VXINC_DIR   = $(IPNET2_DIR)/ipcom/port/vxworks/include
IPDHCP6_CFG_DIR   = $(IPNET2_DIR)/ipdhcpc6/config
IPDHCP6_INC_DIR   = $(IPNET2_DIR)/ipdhcpc6/include
IPDHCPR_CFG_DIR   = $(IPNET2_DIR)/ipdhcpr/config
IPDHCPR_INC_DIR   = $(IPNET2_DIR)/ipdhcpr/include
IPDHCPS_CFG_DIR   = $(IPNET2_DIR)/ipdhcps/config
IPDHCPS_INC_DIR   = $(IPNET2_DIR)/ipdhcps/include
IPDHCPS6_CFG_DIR  = $(IPNET2_DIR)/ipdhcps6/config
IPDHCPS6_INC_DIR  = $(IPNET2_DIR)/ipdhcps6/include
IPPPP_CFG_DIR     = $(IPNET2_DIR)/ipppp/config
IPPPP_INC_DIR     = $(IPNET2_DIR)/ipppp/include
IPRIP_CFG_DIR     = $(IPNET2_DIR)/iprip/config
IPRIP_INC_DIR     = $(IPNET2_DIR)/iprip/include
IPRIPNG_CFG_DIR   = $(IPNET2_DIR)/ipripng/config
IPIRPNG_INC_DIR   = $(IPNET2_DIR)/ipripng/include
IPSNTP_CFG_DIR    = $(IPNET2_DIR)/ipsntp/config
IPSNTP_INC_DIR    = $(IPNET2_DIR)/ipsntp/include
IPTCP_CFG_DIR     = $(IPNET2_DIR)/iptcp/config
IPTCP_INC_DIR     = $(IPNET2_DIR)/iptcp/include
IPNET2_INC_DIR    = $(IPNET2_DIR)/ipnet2/include
IPNET2_CFG_DIR    = $(IPNET2_DIR)/ipnet2/config
IPNET2_SRC_DIR    = $(IPNET2_DIR)/ipnet2/src
WIND_VXCONFIG     = $(IPNET2_DIR)/osconfig/vxworks/src

IPNET2_PATH     = -I$(IPNET2_SRC_DIR) -I$(IPNET2_INC_DIR) -I$(IPNET2_CFG_DIR)  \
                  -I$(IPCOM_CFG_DIR)  -I$(IPCOM_INC_DIR)  -I$(IPCOM_VXCFG_DIR) \
                  -I$(IPPPP_CFG_DIR)  -I$(IPPPP_INC_DIR)  -I$(IPCOM_VXINC_DIR) \
                  -I$(WIND_VXCONFIG)
else
IPNET2_PATH     =			  
endif 				  


# Internal include path
FAMILY_DEVICE_PATH     =    -I$(SOC_BOARD_ENV_DIR) -I$(SOC_CPU_DIR) \
				-I$(SOC_DEVICE_DIR) -I$(SOC_CTRL_ENV_DIR) \
				-I$(SOC_CTRL_ENV_DIR)/sys  -I$(SOC_ERRATA_DIR)   

ifeq ($(CHIP),78200)
FAMILY_DEVICE_PATH    += -I$(SOC_MV_78200_DIR)
endif

COMMON_PATH     = -I$(COMMON_DIR) -I$(GND_DIR) -I$(POOL_DIR) -I$(GEN_ERRATA_DIR)

SOC_UNITS_PATH =  -I$(SOC_UNITS)  -I$(MV_DEVICE_DIR)						    	\
		  -I$(CESA_DIR)   -I$(CNTMR_DIR) -I$(DRAM_DIR)   -I$(ETH_DIR)   -I$(ETHPHY_DIR) 	\
		  -I$(GPP_DIR)   -I$(MFLASH_DIR) -I$(NFLASH_DIR) -I$(NORFLASH_DIR)   			\
		  -I$(PCIIF_DIR)  -I$(PEX_DIR)   -I$(RTC_DIR)    -I$(SATA_DIR)   -I$(SFLASH_DIR)   	\
		  -I$(SPI_DIR)    -I$(TWSI_DIR)  -I$(UART_DIR)   -I$(XOR_DIR)    -I$(USB_DIR)      	\
		  -I$(CESA_DIR)/AES                     \
		  -I$(DRAM_DIR)/spd                     \
		  -I$(SATA_DIR)/CoreDriver 		\
		  -I$(ETH_DIR)/gbe                    \
		  -I$(TWSI_DIR)/ArchArm                 \
		  -I$(USB_DIR)/device      		\
		  -I$(USB_DIR)/api      		\
		  -I$(PRESTERA_DIR)                     \
		  -I$(USB_DIR)/common 


OSSERVICES_PATH = -I$(OSSERVICES_DIR) -I$(OSSERV_VXWORKS) -I$(OSSERV_ARCH_DIR) \
                  -I$(OSSERVICES_BASE_DIR)
PSSSERVICES_PATH= -I$(PSSSERVICES_DIR) -I$(PSSSERVICES_BASE_DIR)

VXBSP_PATH      = -I$(VXBSP_ROOT_DIR) -I$(VXBSP_CHIP_DIR) #-I$(TGT_DIR)/h/nfs


# Working Dirs
FAMILY_DEVICE_DIRS = $(SOC_BOARD_ENV_DIR) $(SOC_CPU_DIR) \
			$(SOC_DEVICE_DIR) $(SOC_CTRL_ENV_DIR) \
			$(SOC_CTRL_ENV_DIR)/sys $(SOC_ERRATA_DIR)   

ifeq ($(CHIP),78200)
FAMILY_DEVICE_DIRS    += $(SOC_MV_78200_DIR)
endif

COMMON_DIRS     = $(COMMON_DIR) $(GND_DIR) $(POOL_DIR) $(GEN_ERRATA_DIR)

SOC_UNITS_DIRS =  $(CESA_DIR)   $(CNTMR_DIR) $(DRAM_DIR)   $(ETH_DIR)  $(ETHPHY_DIR)   \
                  $(GPP_DIR)    $(MFLASH_DIR) $(NFLASH_DIR) $(NORFLASH_DIR) \
		  $(PCIIF_DIR)  $(PEX_DIR)   $(RTC_DIR)    $(SATA_DIR)   $(SFLASH_DIR)   \
		  $(SPI_DIR)    $(TWSI_DIR)  $(UART_DIR)   $(XOR_DIR)    $(USB_DIR)      \
		  $(CESA_DIR)/AES		\
		  $(DRAM_DIR)/spd 		\
		  $(SATA_DIR)/CoreDriver 	\
		  $(ETH_DIR)/gbe 		\
		  $(TWSI_DIR)/ArchArm        	\
		  $(USB_DIR)/device 		\
		  $(USB_DIR)/examples		\
		  $(USB_DIR)/common             \
		  $(PRESTERA_DIR)

ifeq ($(PROJECT), DD)		#DiscoDou
SOC_UNITS_DIRS +=  $(IDMA_DIR)
SOC_UNITS_PATH +=  -I$(IDMA_DIR)
endif

OSSERVICES_DIRS = $(OSSERVICES_DIR) $(OSSERV_VXWORKS) $(OSSERV_ARCH_DIR) \
                  $(OSSERVICES_BASE_DIR)
PSSSERVICES_DIRS= $(PSSSERVICES_DIR) $(PSSSERVICES_BASE_DIR)

VXBSP_DIRS      = $(VXBSP_ROOT_DIR) $(VXBSP_CHIP_DIR) 

###############################################################################
COMMON_OBJS     = mvDebug.o mvCommon.o mvUtils.o
###############################################################################
MV_DEVICE_OBJ   = mvCtrlEnvLib.o mvCtrlEnvAddrDec.o mvBoardEnvLib.o mvBoardEnvSpec.o \
		  mvAhbToMbus.o mvCpuIf.o mvSysGbe.o mvSysPex.o mvSysXor.o 

ifeq ($(CHIP),78200)
MV_DEVICE_OBJ   += mvSemaphore.o mvSocUnitMap.o
endif

ifeq ($(PROJECT), 6183)		
	MV_DEVICE_OBJ   += mvCpuArm.o mvCtrlEnvPadCalibration.o
else
	MV_DEVICE_OBJ   += mvCpu.o 
endif
###############################################################################
SOC_UNITS_OBJS =  mvCntmr.o  mvEthPhy.o mvXor.o \
                  mvDevice.o mvDramIf.o mvGpp.o mvPex.o\
                  mvTwsi.o  mvUart.o  \
                  $(PCI_OBJ) $(DRAM_OBJ) mvPciIf.o \
		  mvFlash.o mvAmdFlash.o mvIntelFlash.o mvFlashCom.o \
		  mvSpi.o mvSFlash.o mvSpiCmnd.o \
		  mvEth.o mvEthDebug.o 
ifneq ($(BOARD),LION_RD)
SOC_UNITS_OBJS += mvVrtBrgPex.o
endif
###############################################################################
OSSERVICES_OBJS = mvVxwArm.o mvOsVxw.o
###############################################################################
VXBSP_OBJS      = vxCntmrIntCtrl.o  vxXorIntCtrl.o  mvCtrlInit.o mvCtrlInit2.o \
		  $(CPU_OBJS) sysmap.o mgiEnd.o	 \
		  vxPexIntCtrl.o vxGppIntCtrl.o vxEthErrIntCtrl.o sysPciIntCtrl.o 
################################################################################
# Switch stuff
#
ifeq ($(MV_SWITCH_PORTS), YES)
SOC_UNITS_DIRS += $(PRESTERA_DIR)

SOC_UNITS_OBJS +=                                                              \
                mvPrestera.o                                                   \

endif
###############################################################################
ifeq ($(RTC),EXT_RTC)
RTC_DIR      	= $(SOC_UNITS)/rtc/integ_rtc
SOC_UNITS_DIRS +=    $(RTC_DIR)
SOC_UNITS_OBJS + = mvRtc.o
endif
###############################################################################
ifeq ($(RTC),INTEG_RTC)
RTC_DIR      	= $(SOC_UNITS)/rtc/ext_rtc
SOC_UNITS_DIRS +=    $(RTC_DIR)
SOC_UNITS_OBJS + = mvDS133x.o 
endif
###############################################################################
ifeq ($(DRAM),ddr2)
SOC_UNITS_DIRS +=  $(DRAM_DIR)/spd
SOC_UNITS_OBJS +=  mvSpd.o mvSysDram.o  
endif
###############################################################################
ifeq ($(INTERRUPT_STATISTIC),Y)
VXBSP_OBJS      += $(BSP_OBJ_DIR)/intArchLib.obj
                  
endif
###############################################################################
ifeq ($(MV_CESA), Y)
VXBSP_OBJS	+= vxCciMarvell.o ipsecInit.o 
MV_DEVICE_OBJ   += mvSysCesa.o 
MV_DEFINE       += -DMV_INCLUDE_CESA 
SOC_UNITS_OBJS += mvCesa.o mvCesaDebug.o mvMD5.o mvSHA1.o mvAesApi.o mvAesAlg.o mvCesaTest.o 
endif
###############################################################################
ifeq ($(SATA), no)
MRVL_SATA_OBJS	= 
else 
EXTRA_INCLUDE   += -I$(MRVL_SATA_DIR)
MV_DEFINE     += -DMV_INCLUDE_INTEG_SATA -DMV_SATA_SUPPORT -DMV_INCLUDE_SATA   
MRVL_SATA_OBJS	= mvIALCommon.o mvIALCommonUtils.o mvLog.o mvSata.o    \
		  mvScsiAtaLayer.o mvStorageDev.o mrvlSataLib.o sysMrvlSata.o	\
		  scsiDiskPerf.o mvOsSVxw.o sysScsi.o sysScsiConfigDisk.o mvSysSata.o
endif
###############################################################################
#DiscoDou 
ifeq ($(PROJECT), DD)		
VXBSP_OBJS      += vxDramIntCtrl.o 
ifeq ($(CHIP),78100)
BOOT_EXTRA      += mvDramIfBasicInit.o mvTwsiEeprom.o mvDramIfConfig.o 
endif
endif
###############################################################################
USB_DEV_DEMO_OBJS = mouse.o disk.o 

USB_DEV_OBJS  = vxUsbDevs.o  mvUsbDevMain.o mvUsbDevRecv.o mvUsbDevSend.o mvUsbDevUtl.o  \
	        mvUsbHsDevCncl.o mvUsbHsDevMain.o mvUsbHsDevUtl.o mvUsbDevCh9.o

ifeq ($(USB_object),yes)
USB_OBJS        = $(BSP_OBJ_DIR)/usbDevLib.obj  $(BSP_OBJ_DIR)/usbDevDemo.obj
VXBSP_OBJS      += vxUsbErrIntCtrl.o 
MV_DEVICE_OBJ   += mvSysUsb.o 
SOC_UNITS_OBJS  += mvUsb.o  
SOC_UNITS_DIRS +=   $(USB_DIR) 
endif
###############################################################################
ifeq ($(IDMA), yes)
MV_DEFINE     += -DMV_INCLUDE_IDMA 
	SOC_UNITS_DIRS +=  $(IDMA_DIR)
	SOC_UNITS_OBJS += mvIdma.o 
	VXBSP_OBJS     += vxIdmaIntCtrl.o
	MV_DEVICE_OBJ  += mvSysIdma.o 
endif
###############################################################################
ifeq ($(AUDIO),YES)
SOC_UNITS_OBJS += mvSysAudio.o  mvAudio.o
endif		  
###############################################################################
PSSSERVICES_OBJS = pssBspApis.o
###############################################################################
MACH_EXTRA      = $(MV_DEVICE_OBJ) $(SOC_UNITS_OBJS)  \
                  $(OSSERVICES_OBJS) $(VXBSP_OBJS) $(COMMON_OBJS)  \
		  $(BSP_USER_OBJS) $(PSSSERVICES_OBJS)

# for Tornado project facility
EXTRA_INCLUDE   +=  -I$(SRC_PATH) $(COMMON_PATH) $(FAMILY_DEVICE_PATH) $(SOC_UNITS_PATH) \
	            $(OSSERVICES_PATH) $(VXBSP_PATH) $(IPNET2_PATH) \
                   $(PSSSERVICES_PATH)

EXTRA_INCLUDE   += -I$(TGT_DIR)/h/nfs -I$(TGT_DIR)/h/drv/xbd
VPATH           = $(FAMILY_DEVICE_DIRS) $(COMMON_DIRS) $(SOC_UNITS_DIRS)  \
		  $(OSSERVICES_DIRS) $(VXBSP_DIRS) $(SKGEY2_DIR)   \
		  $(PSSSERVICES_DIRS) $(PSSSERVICES_BASE_DIR) $(MRVL_SATA_DIR) $($(SATA_DIR)/CoreDriver) \
		  $(VXBSP_WRFILES_DIR) $(VXBSP_VXW_DIR) $(VXBSP_DBG_MATH_DIR) $(IPNET2_SRC_DIR)
		  
###############################################################################
ifeq ($(GEI),yes)
MACH_EXTRA      +=  $(INTEL_NET_OBJS) 
endif
###############################################################################
ifeq ($(YUKON), Y)
MACH_EXTRA      += $(BSP_OBJ_DIR)/yukonLib.obj
EXTRA_INCLUDE += $(INC_VXBSP_SKGEY2_DIR) #-I$(USB_DIR)/device -I$(USB_DIR)/api -I$(USB_DIR)/common
endif                  
###############################################################################
ifeq ($(USB_HOST_DEV),Y)
VPATH         += $(USER_SRC_DIRS) $(USB_DIR)/device   $(USB_DIR)/api   $(USB_DIR)/common  $(USB_DIR)/examples
EXTRA_INCLUDE += $(USER_INC_PATH)
endif
###############################################################################
ifeq ($(BUILD), _dbg)
VPATH           +=   $(VXBSP_DEBUG_DIR) 
MACH_EXTRA  	+=  $(DBG_OBJ)
EXTRA_INCLUDE   += -I$(VXBSP_DEBUG_DIR) 
endif
###############################################################################
ifeq ($(INTERRUPT_STATISTIC), Y)
VPATH           += $(BSP_API_DIR)
EXTRA_INCLUDE   += $(BSP_API_DIR)
endif
###############################################################################
ifdef USER_BASE
ifdef USER_MAKE
# for Tornado 2.x project facility
	EXTRA_MODULES += pssBspApis.o

	include $(USER_BASE)/$(USER_MAKE)
	EXTRA_DEFINE	+= -DINCLUDE_USER_APPL -DPSS_MODE
endif
endif
###############################################################################
# For disassembly 
disa :
	objdumparm -x -s -S --show-raw-insn -l -D bootrom > bootrom.dis

disatmp :
	objdumparm -x -s -S --show-raw-insn -l -D tmp.o > tmp.dis

disavxw :
	objdumparm -x -s -S --show-raw-insn -l -D vxworks.st > vxworks.st.dis
disabin :
	objdumparm -x -s -S --show-raw-insn -l -D vxworks.st_rom > vxworks.st_rom.dis

        
################################################################################
# For Marvell internal use.

$(GEI_NAME).obj:        $(GEI_NAME).o
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) $(GEI_NAME).o -o $(BSP_OBJ_DIR)/$@     

intArchLib.obj:  intArchLib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@     
	$(LD_PARTIAL)  $(MV_DEFINE)  intArchLib.o -o $(BSP_OBJ_DIR)/$@     

yukonLib.obj:  $(SKGEY2_OBJS)
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) $(SKGEY2_OBJS) -o $(BSP_OBJ_DIR)/$@
        
usbDevLib.obj:  $(USB_DEV_OBJS)
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) $(USB_DEV_OBJS) -o $(BSP_OBJ_DIR)/$@
        
usbDevDemo.obj:  $(USB_DEV_DEMO_OBJS)
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) $(USB_DEV_DEMO_OBJS) -o $(BSP_OBJ_DIR)/$@
        
cache$(MV_CPU)eALib.obj:  cache$(MV_CPU)eALib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@     
	$(LD_PARTIAL) $(MV_DEFINE) cache$(MV_CPU)eALib.o -o $(BSP_OBJ_DIR)/$@     
        
cache$(MV_CPU)eLib.obj:  cache$(MV_CPU)eLib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) cache$(MV_CPU)eLib.o -o $(BSP_OBJ_DIR)/$@     
        
mmu$(MV_CPU)eALib.obj:  mmu$(MV_CPU)eALib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@     
	$(LD_PARTIAL) $(MV_DEFINE) mmu$(MV_CPU)eALib.o -o $(BSP_OBJ_DIR)/$@     
        
mmu$(MV_CPU)eLib.obj:  mmu$(MV_CPU)eLib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) mmu$(MV_CPU)eLib.o -o $(BSP_OBJ_DIR)/$@     
        

cacheAimFeroceonLib.obj:  cacheAimFeroceonLib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) cacheAimFeroceonLib.o -o $(BSP_OBJ_DIR)/$@     
        
cacheAimFeroceonALib.obj:  cacheAimFeroceonALib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@
	$(LD_PARTIAL) $(MV_DEFINE) cacheAimFeroceonALib.o -o $(BSP_OBJ_DIR)/$@     

vmMpuLib.obj:  vmMpuLib.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@     
	$(LD_PARTIAL) $(MV_DEFINE) vmMpuLib.o -o $(BSP_OBJ_DIR)/$@     
        
vmData.obj:  vmData.o
	- @ $(RM) $(BSP_OBJ_DIR)/$@     
	$(LD_PARTIAL)  $(MV_DEFINE)  vmData.o -o $(BSP_OBJ_DIR)/$@     
        

################################################################################
VMA_START	= 0x$(RAM_LOW_ADRS)

# Binary version of VxWorks ROM images, suitable for programming
# into Flash using tools provided by ARM.  If other ROM images need to
# be put into Flash, add similar rules here.

bootrom.bin: bootrom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary bootrom $@

bootrom_res.bin: bootrom_res
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary bootrom_res $@

bootrom_uncmp.bin: bootrom_uncmp
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary bootrom_uncmp $@

vxWorks_rom.bin: vxWorks_rom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks_rom $@

vxWorks.st_rom.bin: vxWorks.st_rom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks.st_rom $@

vxWorks.res_rom.bin: vxWorks.res_rom
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks.res_rom $@

vxWorks.res_rom_nosym.bin: vxWorks.res_rom_nosym
	- @ $(RM) $@
	$(EXTRACT_BIN) -O binary vxWorks.res_rom_nosym $@

printConfig:
	@echo Build tool = $(BUILD_TOOL)
	@echo IDE = $(WR_IDE)
	@echo PROJECT = $(PROJECT)
	@echo TOOL = $(TOOL_PREFIX)
	@echo TOOL = $(TOOL_SUFFIX)
   

#MACH_EXTRA	= 
#LIB_EXTRA	= 

## Only redefine make definitions above this point, or the expansion of
## makefile target dependencies may be incorrect.

include $(TGT_DIR)/h/make/rules.bsp
zzz :
	@echo $(EXTRA_INCLUDE)
yyy:
	@echo $(MACH_EXTRA)

xxx :
	@echo $(VPATH)
##clean   : clean_cmdLine
##	- $(RM) $(wildcard vxWorks*) $(wildcard bootrom*) \
##	$(wildcard bootapp*) $(wildcard depend.$(BSP_NAME))
##	- $(RM) depend*
##	
.S.o :
	@ $(RM) $@
	$(CC) $(CFLAGS_AS) $(OPTION_OBJECT_ONLY) -o $@ $< 


