#*******************************************************************************
#                   Copyright 2002, GALILEO TECHNOLOGY, LTD.                   *
# THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
# NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
# OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
# DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
# THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
# IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
#                                                                              *
# MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
# MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
# SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
# (MJKK), GALILEO TECHNOLOGY LTD. (GTL) ,GALILEO TECHNOLOGY, INC. (GTI). and   *
# RADLAN COMPUTER COMMUNICATIONS LTD. (RND).
#*******************************************************************************
# Makefile for Windows Visual C++ (Future Borland)
#
# DESCRIPTION:
#       This file contains rules for building CPSS for
#           WIN32 on the specified board using the tool chain environment.
#*******************************************************************************
# FILE VERSION: 11
#*******************************************************************************

## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.
#

EXTRA_DEFINE    += /D$(BOARD_TYPE)

RELEASE         = application

#specify extra components & include path
EXTRA_INCLUDE =
LIB_EXTRA =
MACH_EXTRA =


include $(USER_BASE)/cpssCommon.mk

#flag simulation on application side
NO_ASIC=FALSE
#flag for 'full application' or just cppEnabler (no mainPpDrv , no galtisWrapper ..)
FULL_APP=TRUE

ifeq (APPLICATION_SIDE_ONLY, $(DISTRIBUTED_SIMULATION_ROLE))
  NO_ASIC=TRUE
endif
ifeq (BROKER_ONLY, $(DISTRIBUTED_SIMULATION_ROLE))
  NO_ASIC=TRUE
  FULL_APP=FALSE
endif

#Convert Unix path to DOS
PROJ_APP_DIR = $(subst /,\,$(PROJ_BSP_DIR))

#simulation lib
#SIM_LIB = $(wildcard $(COMPILATION_ROOT_FIX)\simulation\libs\$(CPU_DIR)\simulation.lib)

ifeq (visual, $(TOOL))
CFLAGS  =  /debug /nologo /incremental:no /SUBSYSTEM:CONSOLE
  ifeq (YES,$(IS_64BIT_OS))
    # do nothing now
  else
	CFLAGS += /entry:mainCRTStartup /pdbtype:sept /machine:I386
  endif
else
## -q - nologo
CFLAGS  =  -c -DI486 -DWIN32 -5 -Od -o -r- -X -a1 -R -v -pc -y -q -w- -wbef -wbig -wccc -wcpr -wdpu -wdup -wdsz -weas -weff -wias -wext \
			-whid -wibc -will -winl -wlin -wlvc -wmpc -wmpd -wnak -wnci -wnfc -wnod -wnst -wntd -wnvf -wobi -wofp -wovl -wpch -wpia \
			-wpro -wrch -wret -wrng -wrpt -wrvl -wsus -wvoi -wzdi
endif


ifneq ($(VC_VER),10)
ADDITIONAL_LIBS = libcmtd.lib
else
ADDITIONAL_LIBS = /NODEFAULTLIB:LIBC.lib
endif

#/entry:main
#/entry:mainCRTStartup
ifeq (visual, $(TOOL))
 ifeq (YES,$(IS_64BIT_OS))
  LDFLAGS = kernel32.lib user32.lib Ws2_32.lib winmm.lib gdi32.lib \
			/NODEFAULTLIB:libcmt.lib libcmtd.lib
 else
  LDFLAGS = kernel32.lib user32.lib gdi32.lib winspool.lib \
        comdlg32.lib advapi32.lib shell32.lib ole32.lib \
        oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib \
        user32.lib gdi32.lib winspool.lib comdlg32.lib \
        advapi32.lib shell32.lib ole32.lib oleaut32.lib \
        uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib winmm.lib imagehlp.lib \
        $(ADDITIONAL_LIBS) /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib
 endif
else
#  LDFLAGS = /Gn /q /c /v /m /M /s c0x32  
  LDFLAGS = /q /c /v /m /M /s c0x32  
  STDLIBS = import32.lib cw32.lib
endif

ifeq (builder, $(TOOL))
  SIM_EXTRA_FIX = $(subst /,\,$(SIM_EXTRA))
  #SIM_LIB = $(wildcard $(COMPILATION_ROOT_FIX)\simulation\libs\$(CPU_DIR)\simulation.lib)
  SIM_LIB_FIX = $(subst /,\,$(wildcard $(COMPILATION_ROOT_FIX)\simulation\libs\$(CPU_DIR)\simulation.lib))
  #SIM_LIB_FIX = $(subst /,\,$(SIM_LIB))
  LIB_EXTRA_FIX = $(subst /,\,$(LIB_EXTRA))
  MACH_EXTRA_FIX = $(subst /,\,$(MACH_EXTRA))
endif

MACH_EXTRA += $(CPSS_ENABLER_FILES_EXT) \
				$(GALTIS_FILES_EXT)

all : cpss application

cpss :
	gmake -f $(USER_BASE)\presteraTopMake full

application : appDemoSim.exe

ifeq (visual, $(TOOL))
SYMTBL = $(subst \,/,$(PROJ_BSP_DIR))/symtable
appDemoSim.exe ::
	- @ $(MKDIR) -p $(subst \,/,$(PROJ_APP_DIR))
	$(AR) /nologo /out:$(PROJ_APP_DIR)/sim.lib $(SIM_EXTRA) $(SIM_LIB) 
	$(AR) /nologo /out:$(PROJ_APP_DIR)/appdemo.lib $(LIB_EXTRA) /IGNORE:4006
	# generate map file
	# 1. Generate list
	- rm -f $(SYMTBL).*
	echo "" >$(SYMTBL).list
	-dumpbin /symbols $(subst \,/,$(PROJ_APP_DIR))/appdemo.lib >>$(SYMTBL).list
	-dumpbin /symbols $(subst \,/,$(PROJ_APP_DIR))/sim.lib >>$(SYMTBL).list
	-for lib in $(subst \,/,$(MACH_EXTRA)); do dumpbin /symbols $$lib;done >>$(SYMTBL).list
	# 2. Create .c file and compile it
	awk -f $(subst \,/,$(USER_BASE))/tools/bin/vc_mksymtbl.awk $(SYMTBL).list >$(SYMTBL).c
	$(CC) /c $(SYMTBL).c /Fo$(SYMTBL).obj
	# now link
	$(LD) /opt:noref $(CFLAGS) $(MACH_EXTRA) $(PROJ_APP_DIR)/appdemo.lib $(PROJ_APP_DIR)/sim.lib $(SYMTBL).obj $(LDFLAGS) $< /OUT:$(PROJ_APP_DIR)/$@ /PDB:$(PROJ_APP_DIR)/$(@:.exe=.pdb) /map:$(PROJ_APP_DIR)/$(@:.exe=.map)
else
SYMTBL = $(subst \,/,$(PROJ_BSP_DIR))/symtable
SYMTBL_FIX = $(PROJ_APP_DIR)\symtable
CAT = awk "{print}"
appDemoSim.exe ::
	- @ $(MKDIR) -p $(subst \,/,$(PROJ_APP_DIR))
##must delete those libs before call tlib, because neither -+ operation for every lib, nor options /u /d of tlib doesn't work corretly
ifneq ( , $(wildcard $(PROJ_APP_DIR)/*.lib))
	$(RM) $(subst /,\,$(wildcard $(PROJ_APP_DIR)/*.lib))
endif
	$(AR) $(PROJ_APP_DIR)\sim.lib $(SIM_LIB_FIX) $(SIM_EXTRA_FIX)
	$(AR) $(PROJ_APP_DIR)\appdemo.lib $(LIB_EXTRA_FIX)
	# generate map file
	# 1. Generate list
	- rm -f $(SYMTBL).*
	echo >$(SYMTBL).list
	-$(AR) $(PROJ_APP_DIR)\sim.lib ,$(SYMTBL_FIX).tmp
	-$(CAT) $(SYMTBL).tmp >> $(SYMTBL).list
	-$(AR) $(PROJ_APP_DIR)\appdemo.lib ,$(SYMTBL_FIX).tmp
	-$(CAT) $(SYMTBL).tmp >> $(SYMTBL).list
	-for lib in $(addprefix ",$(addsuffix ",$(MACH_EXTRA_FIX))); do $(AR) "$$lib" ,"$(SYMTBL_FIX).tmp";$(CAT) $(SYMTBL).tmp;done >>$(SYMTBL).list
	# 2. Create .c file and compile it
	awk -f $(subst \,/,$(USER_BASE))/tools/bin/bc_mksymtbl.awk $(SYMTBL).list >$(SYMTBL).c
	$(CC) -c -o$(SYMTBL_FIX).obj $(SYMTBL_FIX).c
	# now link
	$(LD) $(LDFLAGS), $(PROJ_APP_DIR)\$@, , $(MACH_EXTRA_FIX) $(PROJ_APP_DIR)\appdemo.lib $(PROJ_APP_DIR)\sim.lib $(SYMTBL_FIX).obj $(STDLIBS), ,
endif
	@echo "===== appDemoSim.exe created ====="

clean_all:  clean
clean:
	rm -f appDemo*
	rm -f *.gdb
	find . -name '*~' -exec rm -f {} \;
	find . -name '*.o' -exec rm -f {} \;
	find . -name '*.lib' -exec rm -f {} \;
	find . -name '*.dep' -exec rm -f {} \;

unix:
	find . -name '*.c' -exec dos2unix -q {} \;
	find . -name '*.cpp' -exec dos2unix -q {} \;
	find . -name '*.h' -exec dos2unix -q {} \;
	find . -name '*.C' -exec dos2unix -q {} \;
	find . -name '*.CPP' -exec dos2unix -q {} \;
	find . -name '*.H' -exec dos2unix -q {} \;
	find . -name Makefile -exec dos2unix -q {} \;
	find . -name '*.mk' -exec dos2unix -q {} \;
	find . -name presteraTopMake -exec dos2unix -q {} \;
	find . -name gtTopMake -exec dos2unix -q {} \;
	find . -name gtBuild -exec dos2unix -q {} \;

dos:
	find . -name '*.c' -exec unix2dos -q {} \;
	find . -name '*.cpp' -exec unix2dos -q {} \;
	find . -name '*.h' -exec unix2dos -q {} \;
	find . -name '*.C' -exec unix2dos -q {} \;
	find . -name '*.CPP' -exec unix2dos -q {} \;
	find . -name '*.H' -exec unix2dos -q {} \;
	find . -name Makefile -exec unix2dos -q {} \;
	find . -name '*.mk' -exec unix2dos -q {} \;
	find . -name presteraTopMake -exec unix2dos -q {} \;
	find . -name gtTopMake -exec unix2dos -q {} \;
	find . -name gtBuild -exec unix2dos -q {} \;

