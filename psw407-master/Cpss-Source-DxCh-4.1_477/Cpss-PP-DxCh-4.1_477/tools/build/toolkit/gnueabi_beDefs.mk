#
# Make definitions for GNUEABI (big endian)
#

#start of include
FIS= -I
#end of include
FIE=
FD=-D
FO=-o
FC=-c
FG=-g
OBJ=o
ASM=s
LIBEXT=a

LDFLAGS=-EB -r -X -N
export LDFLAGS

CPU_FLAGS = -DARMEL -DCPU_$(ARM_CPU) -DARMMMU=ARMMMU_$(ARM_MMU) -DARMCACHE=ARMCACHE_$(ARM_CACHE)
CC_ARCH += -mbig-endian
#Do we compile for thumb ?
ifeq  ($(findstring _T,$(CPU)),_T)
  # definitions for Thumb mode
  CPU_FLAGS += -mthumb -mthumb-interwork
endif

CFLAGS1 += -B$(COMPILER_ROOT)

#Add a C definition to distiguish between different OS (Added for FreeBSD)
ifdef OS_TARGET
  CFLAGS1 =  -Wall $(CC_ARCH) -D$(OS_TARGET) -DOS_TARGET=$(OS_TARGET) -DOS_TARGET_RELEASE=$(OS_TARGET_RELEASE)
else
  CFLAGS1 =  -Wall $(CC_ARCH) -DOS_TARGET=$(OS_RUN)
endif

#CFLAGS2 = -ansi -pedantic -fvolatile -fno-builtin -funroll-loops
CFLAGS2 = -ansi -fno-builtin -funroll-loops
ifeq (3, $(GCCVER))
  CFLAGS2 += -fvolatile
endif

#Shared library approach requires Position-Independent Code
ifeq (1,$(SHARED_MEMORY))
  CFLAGS2 += -fPIC -DPIC
endif

CFLAGS_OPT = -O2 

BUILD_MAP_FILE= -Map 

