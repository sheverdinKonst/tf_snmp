/*******************************************************************************
 *                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
 * OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
 * THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
 *                                                                              *
 * MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
 * MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
 * SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
 * (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
 *******************************************************************************/
/*******************************************************************************
 * mvUtils.c - utilities
 *
 * DESCRIPTION:
 *       None.
 *
 * DEPENDENCIES:
 *       None.
 *
 *******************************************************************************/

/* includes */

#include "vxWorks.h"
#include "config.h"
#include "sysLib.h"
#include "string.h"
#include "intLib.h"
#include "taskLib.h"
#include "vxLib.h"
#include "muxLib.h"
#include "cacheLib.h"
#include "usrConfig.h"
#include "mvCpu.h"
#include "mvPex.h"
#undef  PCI_AUTO_DEBUG
#include "drv/pci/pciIntLib.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciAutoConfigLib.h"
#include "drv/pci/pciConfigShow.h"
#include "ctrlEnv/sys/mvAhbToMbus.h"


#undef MV_DEBUG         
#ifdef MV_DEBUG         
#define DB(x) x
#else
#define DB(x)
#endif

/* typedefs */
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef struct _mvIsrEntryTable 
{
  MV_ISR_ENTRY *pexCauseArray;
	MV_ISR_ENTRY *pexErrCauseArray; 
	int  pexCauseCount;
	int  pexErrCauseCount;   
} MV_ISR_ENTRY_TABLE;

typedef struct COMM_AREA_TYPE
{
  uint vendor_id;
  uint dev_id;
  uint instance;
  uint b;
  uint d;
  uint f;
  uint *pp_bus;
  int  ret;
  int count;
} COMM_AREA;

IMPORT int pexIntHandlerCount;
IMPORT int pexIntErrHandlerCount;
IMPORT ulong intDemuxErrorCount;
IMPORT ULONG standTblSize;
IMPORT SYMBOL standTbl [];
IMPORT MV_ISR_ENTRY gpp7_0Array[2][8];
IMPORT MV_ISR_ENTRY gpp15_8Array[2][8];
IMPORT MV_ISR_ENTRY gpp23_16Array[2][8];
#if defined (MV88F6XXX)
IMPORT MV_ISR_ENTRY gpp31_24Array[2][8];
#endif
IMPORT MV_ISR_ENTRY_TABLE pexCauseArray_TABLE[8];

int dump_bytes = 64;

#define gc_isprint(a) ((a >=' ')&&(a <= '~'))

/* #define PEX_FIX_DEBUG */
#define PCI_ANY_ID 0xffffffff

#define MV_VENDOR _mv_vendor(b, d, f)
#define MV_DEVICE _mv_device(b, d, f)

#ifdef DB_MV78200_A_AMC
#define LION_BASE PCI4_MEM0_BASE /* spans 0x80000000 - 0x9fffffff */
#define IDT_VEN_ID 0x111d
#define LION_MAX_PIPE 8
/*
  PCI4_MEM_BASE and PCI4_MEM0_SIZE for DB_MV78200_A_AMC 
  are defined in mvSysHwConfig.h
*/
#endif

static UINT16 _mv_vendor(int b, int d, int f)
{
  UINT16 vendor;

  pciConfigInWord(b, d, f,  PCI_CFG_VENDOR_ID, &vendor);
  return vendor;  
}

static UINT16 _mv_device(int b, int d, int f)
{
  UINT16 device;
  pciConfigInWord(b, d, f,  PCI_CFG_DEVICE_ID, &device);
  return device;  
}

static void gc_dump(char *buf, int len, int offset)
{
  int offs, i;
  int j;

  for (offs = 0; offs < len; offs += 16) 
  {
    j = 1;
    printf("0x%08x : ", offs + offset);
    for (i = 0; i < 16 && offs + i < len; i++)
    {
      printf("%02x", (uchar)buf[offs + offset + i]);
      if (!((j++)%4) && (j < 16))
        printf(",");
    }
    for (; i < 16; i++)
      printf("   ");
    printf("  ");
    for (i = 0; i < 16 && offs + i < len; i++)
      printf("%c",
             gc_isprint(buf[offs + offset + i]) ? buf[offs + offset + i] : '.');
    printf("\n");
  }
  printf("\n");
}

static STATUS pciDumpHeader(int b, int d, int f, void *pArg /* ignored */)
{
  uchar buff[256];
  int addr;
  int nbytes = dump_bytes;

  memset(buff,0xff,dump_bytes);
  
  for (addr = 0 ; addr<dump_bytes ; addr++)
  {
    pciConfigInByte(b, d, f, addr, (buff+addr));
  }
  
  printf("Device at [%d,%d,%d]\n", b, d, f);

  gc_dump((char *)buff, nbytes, 0);
  return 0;
}

STATUS static _lspci1(int b, int d, int f, void *p /* ignored */)
{
  /* dump some of the Marvell PP's registers */
  
  UINT16 vendor, device;
  UINT32 bar0, bar2;
  UINT8 irq;

  vendor = MV_VENDOR;
  device = MV_DEVICE;

  if ((vendor == 0x11ab) && 
      (((device & 0xf000) == 0xc000) ||
       ((device & 0xf000) == 0xd000) ||
       ((device & 0xf000) == 0xe000))
      )
  {
    pciConfigInLong(b, d, f, PCI_CFG_BASE_ADDRESS_0, &bar0);
    bar0  &= ~ 0x0f;

    pciConfigInLong(b, d, f, PCI_CFG_BASE_ADDRESS_2, &bar2);
    bar2  &= ~ 0x0f;

    pciConfigInByte(b, d, f, PCI_CFG_DEV_INT_LINE, &irq);

    printf("Marvell Packet Processor at [%d,%d,%d]:\n", b, d, f);
    printf("  vendor=0x%04x, device=0x%04x, bar0=0x%08x, bar2=0x%08x, irq=%d\n",
           vendor, device, bar0, bar2, irq);
    printf("\n");

    printf("bar0=0x%08x\n", bar0);
    gc_dump((char *)(bar0), 128, 0x70000);

#ifdef MV_DEBUG
    DB(printf("pex information\n"));
    gc_dump((char *)(bar0), 128, 0x71a00);
#endif

    printf("bar2=0x%08x\n", bar2);
    gc_dump((char *)bar2, 128, 0);

    /* for interrupt detection technique */
    DB(printf("data at 0x%08x = 0x%08x\n", bar2+4, *(unsigned long *)(bar2+4)));


  }
  return 0;
}

static void _lspci(void)
{
  pciConfigForeachFunc(0, TRUE, pciDumpHeader, NULL);
  pciConfigForeachFunc(0, TRUE, _lspci1, NULL);
}

void lspci(void)
{
  dump_bytes = 64;
  _lspci();
}

void lspci_l(void)
{
  dump_bytes = 256;
  _lspci();
}

static int find_isr_name(ulong isr, char *name)
{
  int  j, sym_found;

  sym_found = 0;
  for (j = 0; j < standTblSize; j++)
	{
    if (standTbl[j].value == (void *)isr)
    {
      strcpy(name, standTbl[j].name);
      sym_found = 1;
      break;
    }
	}
  
  if (!sym_found)
    strcpy(name, "??");

  return sym_found;
}

static ulong getIntVecTable(void)
{
  ulong addr;
#ifndef DB_MV78200_A_AMC
  ulong isr;
  char name[128];
#endif
  static ulong staticIntVecTable = 0;
  int i;
  
  if (staticIntVecTable == 0)
  {
    staticIntVecTable = 0xffffffff;
    for (i = 4; i < 12; i += 4)
    {
      addr = (*(ulong *)((ulong)&intDemuxErrorCount + i));
      /*
        Black magic. intDemuxErrorCount is an external symbol which is 4 or 8
        prior to the hidden (static) intVecTable (depending on command_line or WB).
        do nm -n vxWorks.st
      */
      
      if (!addr)
        continue;

#ifdef DB_MV78200_A_AMC
			staticIntVecTable = addr;
			break;
#else
      isr = *(ulong *)addr;

      if (find_isr_name(isr, name) && 
          (strcmp(name, "vxAhb2MbusHandler") == 0))
      {
        staticIntVecTable = addr;
        break;
      }
#endif
    }
  }

  return staticIntVecTable;
}

void showirq_pex(int pexIf)
{
  int i;
  MV_ISR_ENTRY *intCauseArray = pexCauseArray_TABLE[pexIf].pexCauseArray;
  MV_ISR_ENTRY *intErrCauseArray = pexCauseArray_TABLE[pexIf].pexErrCauseArray;
  ulong isr0 = 0;
  ulong isr1 = 0;
  int count0;
  int count1;
  char name0[128];
  char name1[128];

  printf("pex interface %d\n", pexIf);
  for (i = 0; i < 8; i++)
  {
    isr0 = (ulong)intCauseArray[i].userISR;
    count0 = intCauseArray[i].counter;

    isr1 = (ulong)intErrCauseArray[i].userISR;
    count1 = intCauseArray[i].counter;

    if (isr0)
    {
      find_isr_name(isr0, name0);
      printf(" irq %d, cause %d: count=%d, \thandler=0x%08lx, %s\n", 
             (pexIf*4 + i + 96), i, count0, isr0, name0);
    }

    if (isr1)
    {
      find_isr_name(isr1, name1);
      if (strcmp(name1, "pexErrShow") != 0)
        printf(" irq %d errcause %d: count=%d, \thandler=0x%08lx, %s\n", 
               (pexIf*4 + i + 96), i, count1, isr1, name1);    
    }
  }
}

void showirq_gpio(int arrayNum)
{
  int i;
  ulong isr0 = 0;
  ulong isr1 = 0;
  char name[128];
  char name1[128];

  printf("gpio array %d\n", arrayNum);
  for (i = 0; i < 8; i++)
  {
    switch ((arrayNum))
    {
    case 0:
      isr0 = (ulong)gpp7_0Array[0][i].userISR;
      isr1 = (ulong)gpp7_0Array[1][i].userISR;
      break;
    case 1:
      isr0 = (ulong)gpp15_8Array[0][i].userISR;
      isr1 = (ulong)gpp15_8Array[1][i].userISR;
      break;
    case 2:
      isr0 = (ulong)gpp23_16Array[0][i].userISR;
      isr1 = (ulong)gpp23_16Array[1][i].userISR;
      break;
#if defined (MV88F6XXX)
    case 3:
      isr0 = (ulong)gpp31_24Array[0][i].userISR;
      isr1 = (ulong)gpp31_24Array[1][i].userISR;
#endif
      
    }
    
    find_isr_name(isr0, name);
    find_isr_name(isr1, name1);

    if (isr0)
      printf("    group 0 entry %d: count=%d, \thandler=0x%08lx, %s\n", 
             i, -1, isr0, name);
    
    if (isr1)
      printf("    group 1 entry=%d: count=%d, \thandler1=0x%08lx, %s\n", 
             i, -1, isr1, name1);
  } 
}

/* place holder for 10 mv symbols */

int mv_symbol0;
int mv_symbol1;
int mv_symbol2;
int mv_symbol3;
int mv_symbol4;
int mv_symbol5;
int mv_symbol6;
int mv_symbol7;
int mv_symbol8;
int mv_symbol9;

void mv_symAdd(char *name, void *value)
{
  int i;
  char *p;

  for (i = 0; i < standTblSize; i++)
	{
    if (strncmp(standTbl[i].name, "mv_symbol", 9) == 0)
    {
      p = malloc(strlen(name) + 1);
      strcpy(p, name);
      standTbl[i].name =  p;
      standTbl[i].value = (char *)value;
      standTbl[i].type = SYM_TEXT | SYM_DATA | SYM_GLOBAL;
      break;
    }
  }
  
}

void showirq(void)
{
  int i;
  ulong intVecTable = getIntVecTable();
  ulong isr;
  char name[128];
  int count;
  
  if (intVecTable == 0 || intVecTable == 0xffffffff)
  {
    printf("inVecTable not found\n");
    return;
  }
  printf("inVecTable    is at 0x%08lx\n", intVecTable);
  printf("gpp7_0Array   is at 0x%08lx\n", (ulong)gpp7_0Array);
  printf("gpp15_8Array  is at 0x%08lx\n", (ulong)gpp15_8Array);
  printf("gpp23_16Array is at 0x%08lx\n", (ulong)gpp23_16Array);
#if defined (MV88F6XXX)
  printf("gpp31_24Array is at 0x%08lx\n", (ulong)gpp31_24Array);
#endif
  printf("\n");

  for (i = 0; i < 150; i++ )
  {
    isr = *(ulong *)(intVecTable+(i*8));
    find_isr_name(isr, name);

		if (strcmp(name, "??") == 0)
			continue;

    /*  
    if (strcmp(name, "pexIntHandler") == 0)
      count = pexIntHandlerCount;
    else if (strcmp(name, "pexIntErrHandler") == 0)
      count = pexIntErrHandlerCount;
    else
    */
      count=-1;

    if ((strcmp(name, "gpp7_0Int") == 0))
      showirq_gpio(0);
    else if ((strcmp(name, "gpp15_8Int") == 0))
      showirq_gpio(1);
    else if ((strcmp(name, "gpp23_16Int") == 0))
      showirq_gpio(2);

#if defined (MV88F6XXX)
    else if ((strcmp(name, "gpp31_24Int") == 0))
      showirq_gpio(3);
#endif
    else if ((strcmp(name, "pexIntHandler") == 0))
      showirq_pex(i-32);    
    else 
      printf("irq=%i, count=%d, \thandler=0x%08lx, %s\n", 
             i, count, isr, name);    
  } 

  printf("\n");
}

static int _mv_finddev(int b, int d, int f, void *arg)
{
  COMM_AREA *pcomm = (COMM_AREA *)arg;

  /* do lspci and find the devices that are not pps or bridges !, Giora */
  uint exclude_dev[4] = {0x8888, 0x7820, 0x7810, 0x0};
  int i;
  int ret = 0;
  UINT16 device = 0;
  
  DB(printf(">>>> in _mv_findev, pcomm->dev_id = 0x%04x, b=%d, d=%d, f=%d\n",
             pcomm->dev_id, b, d, f));

  if (((pcomm->vendor_id == PCI_ANY_ID) || (MV_VENDOR == pcomm->vendor_id )) && 
      ((pcomm->dev_id    == PCI_ANY_ID) || ((device = MV_DEVICE) == pcomm->dev_id)))
  {
    if (!device)
      device = MV_DEVICE; /* need to read it in case PCI_ANY_ID */

    for (i = 0; i < sizeof(exclude_dev)/sizeof(uint); i++)
      if (device == exclude_dev[i])
        goto out;

    if (pcomm->count++ == pcomm->instance)
    {
      ret = 1; /* found the device, stop the iteration */
      pcomm->b = b;
      pcomm->d = d;
      pcomm->f = f;
      pcomm->ret = 1;      
      goto out;
    }
  }
  
 out:
	return ret;  
}

static int mv_finddev(uint vendor_id, 
                   uint dev_id, 
                   uint instance,
                   uint *b,
                   uint *d,
                   uint *f)
{

	int ret = 0;
  COMM_AREA comm;

  comm.dev_id    = dev_id;
  comm.vendor_id = vendor_id;
  comm.instance  = instance;  
  comm.count     = 0;
  comm.ret       = 0;

  DB(printf(">>>>> before foreach, comm.dev_id = 0x%04x\n", comm.dev_id));

  pciConfigForeachFunc(0, TRUE, _mv_finddev, (void *)&comm);
  if (comm.ret)
  {

    DB(printf(">>>>>> after foreach, comm.dev_id=0x%04x, b=%d, d=%d, f=%d\n", 
               comm.dev_id, comm.b, comm.d, comm.f));

    if (b)
      *b = comm.b;
    if (d)
      *d = comm.d;
    if (f)
      *f = comm.f;
    ret = 1;
  }
  
  return ret;
}

static int _mv_finddevCount(int b, int d, int f, void *arg)
{
  COMM_AREA *pcomm = (COMM_AREA *)arg;

  /* do lspci and find the devices that are not pps or bridges !, Giora */
  uint exclude_dev[4] = {0x8888, 0x7820, 0x7810, 0x0};
  int i;
  int ret = 0;
  UINT16 device = 0;
  
  if (((pcomm->vendor_id == PCI_ANY_ID) || (MV_VENDOR == pcomm->vendor_id )) && 
      ((pcomm->dev_id    == PCI_ANY_ID) || ((device = MV_DEVICE) == pcomm->dev_id)))
  {
    if (!device)
      device = MV_DEVICE; /* need to read it in case PCI_ANY_ID */

    for (i = 0; i < sizeof(exclude_dev)/sizeof(uint); i++)
      if (device == exclude_dev[i])
        goto out;

	pcomm->pp_bus[pcomm->count] = b;
    pcomm->count++;
    pcomm->b = b;
    pcomm->d = d;
    pcomm->f = f;
    pcomm->ret = 1;      
    goto out;
  }
  
 out:
	return ret;  
}

static int mv_find_pp_instances(UINT16 *dev_id, uint *pp_bus)
{
  int count = 0;
  uint b, d, f;
  COMM_AREA comm;

  comm.dev_id    = PCI_ANY_ID;
  comm.vendor_id = MARVELL_VEN_ID;
  comm.count     = 0;
  comm.ret       = 0;
  comm.pp_bus    = pp_bus;

  pciConfigForeachFunc(0, TRUE, _mv_finddevCount, (void *)&comm);
  if (comm.ret)
  {
     b = comm.b;
     d = comm.d;
     f = comm.f;

	 *dev_id = MV_DEVICE;

	 count = comm.count;
	 pp_bus[count] = comm.b;

  }
  
  return count;
}

static int _MV_REG_WRITE(uint a, uint b)
{
#ifdef  PEX_FIX_DEBUG
  mvOsPrintf(">>> calling MV_REG_WRITE(0x%08x, 0x%08x)\n", a, b);
#endif
  return MV_REG_WRITE(a, b);
}

static int _pciConfigOutLong(uint a, uint b, uint c, uint d, uint e)
{
#ifdef  PEX_FIX_DEBUG
  mvOsPrintf(">>> calling pciConfigOutLong(0x%08x, 0x%08x, 0x%08x, 0x%08x, "
         "0x%08x\n", a, b, c, d, e);
#endif
  return pciConfigOutLong(a, b, c, d, e);
}

static int _pciConfigOutByte(uint a, uint b, uint c, uint d, uint e)
{
#ifdef  PEX_FIX_DEBUG
  mvOsPrintf(">>> calling pciConfigOutByte(0x%08x, 0x%08x, 0x%08x, 0x%08x, "
         "0x%08x\n", a, b, c, d, e);
#endif
  return pciConfigOutByte(a, b, c, d, e);
}

static int mvGetBarSize(uint a, uint b, uint c, uint d)
{
	uint  bar_size,bar_response;

	/* get the bar size by tickling the bar (the formal method, not the guesswork) */
	pciConfigOutLong(a,b,c, d, 0xffffffff);
	pciConfigInLong(a,b,c,d,&bar_response);
	bar_size = ~(bar_response & ~0xf) + 1;

	/* ensure memory is enabled */
	pciConfigOutLong(a,b,c, 0x04, 0x00100146);

	return bar_size;

}

int    lion_board_type = 0;
int    have_idt_switch  = 0;
UINT16 lion_dev_id      = 0;

void mv_fix_pex(void)
{
#ifdef DB_MV78200_A_AMC
  
  /*
  // Use huristics to fix the pex chain and address decoding windows for accessing
  // the packet-processors
  */
  

  UINT16 dev_id = 0;
  MV_TARGET_ATTRIB targetAttrib;

  /*
  // ul = uplink
  // dl = downlink
  */
  uint idt_ul_bus;
  uint idt_dl_bus;

  uint pp_bus[LION_MAX_PIPE] = {0};
  uint lion_irq[LION_MAX_PIPE] = {0};

  uint idt_dl_dev[LION_MAX_PIPE] = {0};
  ushort datas;
  int i, ppInstance, data, winSize0, winSize1, totalSize, tmp, num;

  mvOsPrintf("\nPEX: Marvell device fixup\n");

  /* as long as we are here, fix the nor flash adw */
	_MV_REG_WRITE(AHB_TO_MBUS_WIN_CTRL_REG(0,13), 0x007f1f11);

  /* determine if we have idt switch  */
	have_idt_switch  = mv_finddev(IDT_VEN_ID, PCI_ANY_ID, 0, NULL, NULL, NULL);
  	
  /* determine the type of configuration that we have */

    ppInstance = mv_find_pp_instances(&dev_id,pp_bus);
	lion_dev_id = dev_id;

	if (ppInstance >= 4)
	  lion_board_type = 1;

	if (ppInstance == 8)
	  lion_board_type = 2;


#ifndef LION_RD
	mvOsPrintf("Detected configuration with %d Marvell devices 0x%04x\n",
		   ppInstance, 
		   dev_id);

	mvOsPrintf("idt bridge %s\n",
		   (have_idt_switch)  ? "present":
		   "not present");
#endif 
	if (have_idt_switch)
	{
		mv_finddev(IDT_VEN_ID, PCI_ANY_ID, 0, &idt_ul_bus, NULL, NULL);
		mv_finddev(IDT_VEN_ID, PCI_ANY_ID, 1, &idt_dl_bus, NULL, NULL);

		for (num = 0, i = 0; num < ppInstance; i++)
		{
			/*
			// locate the active downlink ports
			// the idt switch downlink ports are 2 - (2 + ppInstance). 
			// ports 0 - 1 are for the IDT switch
			// Lets find which of them are connected to the pps.
			*/
			pciConfigInWord(idt_dl_bus, i + 2, 0, 0x52, &datas);
			if (datas == 0x3011)
			{
				idt_dl_dev[num++] = i + 2;
			}
		}

		for (i = 0; i < ppInstance; i++)
		{
			/*
			  The PP cores always generate int_a (interrupt pin 1)
			  but the switch translates (shifts, maps, ...) them 
			  according to this formula:

			  port x inta -> int x % 4

			  see page 51 of "PEX sw 89PES8T5 Device User Manual.pdf"

			  first pin of pex interface 4: 96 + 16 = 112  
			  therefore we swizzle the interrupts accoring to the formula:

			  irq_num = 112 + (device nume)%4;

			*/
			lion_irq[i] = 112 + idt_dl_dev[i]%4;          
		}

		mvOsPrintf("Fixing pex interfce 4 and adw for Marvell lion devices "
				   "with pex bridge\n");

		mvOsPrintf("idt uplink   port  at %02x:01.0\n", idt_ul_bus);
		mvOsPrintf("idt downlink ports at ");
		for (i = 0; i < ppInstance; i++)
		  mvOsPrintf("%02x:%02x.0%s", idt_dl_bus, idt_dl_dev[i], (i<(ppInstance - 1))?", ":"");
		mvOsPrintf("\n");

		mvOsPrintf("Marvell devices = 0x%04x, at ", dev_id);
		for (i = 0; i < ppInstance; i++)
		  mvOsPrintf("%02x:00.0%s", pp_bus[i], (i<(ppInstance - 1))?", ":"");      
		mvOsPrintf("\n");


		/* allocate memory windows */
		totalSize = 0;
		for (i = 0; i < ppInstance; i++)
		{
			/* allocate memory 0 */
			winSize0 = mvGetBarSize(pp_bus[i], 0, 0,PCI_CFG_BASE_ADDRESS_0);
			_pciConfigOutLong(pp_bus[i], 0, 0, PCI_CFG_BASE_ADDRESS_0, 
										   (totalSize + LION_BASE)|0x0000000c);

			if ((winSize0 % _64M) != 0)
			{
				/* align memory size to 64M */
				winSize0 = (winSize0 / _64M) + _64M;
			}
			/* allocate memory 1 */
			winSize1 = mvGetBarSize(pp_bus[i], 0, 0,PCI_CFG_BASE_ADDRESS_2);
			_pciConfigOutLong(pp_bus[i], 0, 0, PCI_CFG_BASE_ADDRESS_2, 
										   (totalSize + winSize0 + LION_BASE) | 
											0x0000000c);

			if ((winSize1 % _64M) != 0)
			{
				/* align memory size to 64M */
				winSize1 = (winSize1 / _64M) + _64M;
			}


			_pciConfigOutLong(idt_dl_bus, idt_dl_dev[i], 0, PCI_CFG_MEM_BASE, 
				 ((totalSize + LION_BASE) | 
							  (((winSize0 + winSize1) - 1) & 0xFFFF0000)) | 
							  ((totalSize + LION_BASE) >> 16));

			totalSize += (winSize0 + winSize1); 

			/* write irq number in pci config spaces */
			_pciConfigOutByte(pp_bus[i] ,0, 0, PCI_CFG_DEV_INT_LINE, lion_irq[i]);

		}
		
		mvOsPrintf("Allocating %dMB memory at 0x%08x\n",totalSize / (1024*1024) ,LION_BASE);
		

		_pciConfigOutLong(idt_ul_bus, 1, 0, PCI_CFG_MEM_BASE,
				 (LION_BASE|((totalSize - 1) & 0xFFFF0000)) | 
						  ((LION_BASE|0x00000000) >> 16));

		/* fix the adw - window 5 */
		data = 0;
		data |= ATMWCR_WIN_ENABLE;/* enable window */
		mvCtrlAttribGet(PCI4_MEM0, &targetAttrib); /* get attribute and attribute */
		data |= targetAttrib.targetId << ATMWCR_WIN_TARGET_OFFS; /* set target */
		data |= targetAttrib.attrib << ATMWCR_WIN_ATTR_OFFS;/* set attributes */

		/* need to alligned the allocated size */
		tmp = (totalSize / (64 * 1024) - 1);
		if (tmp > 0)
		{
			/* find the MSB set bit */
			for (i = 16; i > 0; i--)
			{
				if ((tmp >> (i - 1)) != 0)
				{
					break;
				}
			}
			/* alligned the size */
			tmp = 1 << i;
			tmp -= 1;
		}

		data |= tmp << ATMWCR_WIN_SIZE_OFFS; /* set size */

		_MV_REG_WRITE(AHB_TO_MBUS_WIN_CTRL_REG(0,5), data); 
		_MV_REG_WRITE(AHB_TO_MBUS_WIN_BASE_REG(0,5), LION_BASE); 
		_MV_REG_WRITE(AHB_TO_MBUS_WIN_REMAP_LOW_REG(0,5), LION_BASE); 
		_MV_REG_WRITE(AHB_TO_MBUS_WIN_REMAP_HIGH_REG(0,5), 0x00000000); 

    }
	else
	{
#ifdef LION_RD

        _pciConfigOutByte(0 ,1, 0, PCI_CFG_DEV_INT_LINE, INT_LVL_PEX00_INTA);
        _pciConfigOutByte(1 ,1, 0, PCI_CFG_DEV_INT_LINE, INT_LVL_PEX01_INTA);
        _pciConfigOutByte(2 ,1, 0, PCI_CFG_DEV_INT_LINE, INT_LVL_PEX02_INTA);
        _pciConfigOutByte(3 ,1, 0, PCI_CFG_DEV_INT_LINE, INT_LVL_PEX03_INTA);

#endif 
	}
 
  mvOsPrintf("\n");

#endif
}
