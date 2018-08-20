//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
//
//
//		Asgard68000StarScream.c
//		Interface file to make Asgard68000 look like StarScream, for porting games using Neill
//		Corlett's assembly engine on the PC.
//
//		A PowerPC assembly MC68000 emulation core written by Aaron Giles
//		Special thanks to Brian Verre for his work in generating the cycle counting table
//
//		This code is free for use in any emulation project as long as the following credit appears 
//		in the about box and documentation:
//
//			PowerPC-optimized MC68000 emulation provided by Aaron Giles.
//
//		I would also appreciate a free copy of any project making use of this code.  Please take
//		the time to contact me if you are using this code or if you have any bugs to report;
//		It is quite possible that I have a newer version.  My email address is agiles@sirius.com
//
//		This file looks best when viewed with a tab size of 3 characters
//
//


/* 
 *	68000.h c stubfile
 */

#include "Asgard68000.h"

#include "rccore/global.h"

// StarScream interface
#include "StarCPU.h"


typedef UINT32 (*ReadHandler)(UINT32 address, struct STARSCREAM_MEMORYIO *read);
typedef void (*WriteHandler)(UINT32 address, UINT32 data, struct STARSCREAM_MEMORYIO *write);


static UINT32 odometer;
static UINT32 cyclesrunning;


unsigned char *s68000OpcodeBase;
struct STARSCREAM_MEMORYFETCH *currentOpcodeBase;
struct S68000CONTEXT s68000context;


void AsgardContextToS68000Context (void);
void S68000ContextToAsgardContext (void);
void *s68000ChangePC (UINT32 newPC);


int s68000init (void)
{
	odometer = 0;
	memset (&s68000context, 0, sizeof (s68000context));
}


void s68000reset (void)
{
	Asgard68000Reset ();
	if (s68000context.resethandler) (*s68000context.resethandler) ();
	AsgardContextToS68000Context ();
	s68000ChangePC (s68000context.pc);
}


UINT32 s68000exec (UINT32 cycles)
{
	Asgard68000 context;

	cyclesrunning = cycles;
	Asgard68000Execute (cycles);
	odometer += cyclesrunning - Asgard68000ICount;
	cyclesrunning = Asgard68000ICount = 0;

	return 0x80000000;
}


UINT32 s68000interrupt (UINT32 a)
{
	Asgard68000AssertIRQ (1 << a);
	return 0;
}


UINT32 s68000GetContextSize (void)
{
	return sizeof (struct S68000CONTEXT);
}


void s68000GetContext (void *c)
{
	AsgardContextToS68000Context ();
	*(struct S68000CONTEXT *)c = s68000context;
}


void s68000SetContext (void *c)
{
	s68000context = *(struct S68000CONTEXT *)c;
	S68000ContextToAsgardContext ();
}


UINT32 s68000controlOdometer (UINT32 reset)
{
	UINT32 cyclesleft = Asgard68000ICount;
	UINT32 cyclesran = cyclesrunning - cyclesleft;
	UINT32 result = odometer + cyclesran;
	
	if (reset)
	{
		odometer = 0;
		cyclesrunning = cyclesleft;
	}
	
	return result;
}


void s68000releaseTimeslice (void)
{
	UINT32 cyclesleft = Asgard68000ICount;
	UINT32 cyclesran = cyclesrunning - cyclesleft;

	odometer += cyclesran;
	cyclesrunning = Asgard68000ICount = 0;
}


void AsgardContextToS68000Context (void)
{
	Asgard68000 context;
	int i;
	
	Asgard68000GetRegs (&context);
	
	for (i = 0; i < 8; i++)
	{
		s68000context.dreg[i] = context.d[i];
		s68000context.areg[i] = context.a[i];
	}
	s68000context.pc = context.pc;
	s68000context.sr = context.sr;
	s68000context.asp = context.isp;
	s68000context.io_edi = context.usp;
	s68000context.stopped = context.irq & ASGARD68000_STOP;
	s68000context.execinfo = context.irq >> 24;
}


void S68000ContextToAsgardContext (void)
{
	Asgard68000 context;
	int i;

	for (i = 0; i < 8; i++)
	{
		context.d[i] = s68000context.dreg[i];
		context.a[i] = s68000context.areg[i];
	}
	context.pc = s68000context.pc;
	context.sr = s68000context.sr;
	context.isp = s68000context.asp;
	context.usp = s68000context.io_edi;
	context.vbr = 0;
	context.sfc = 0;
	context.dfc = 0;
	context.irq = (s68000context.stopped & ASGARD68000_STOP) | (s68000context.execinfo << 24);

	Asgard68000SetRegs (&context);
	s68000ChangePC (s68000context.pc);
}




void *s68000ChangePC (UINT32 newPC)
{
	struct STARSCREAM_MEMORYFETCH *mf;
	UINT32 low;

	if (newPC >= currentOpcodeBase->lowaddr && newPC <= currentOpcodeBase->highaddr)
		return s68000OpcodeBase;
	
	for (mf = s68000context.memoryfetch; (low = mf->lowaddr) != 0xffffffff; mf++)
		if (newPC >= low && newPC <= mf->highaddr)
		{
			currentOpcodeBase = mf;
			return s68000OpcodeBase = (unsigned char *)mf->off;
		}
}


UINT32 s68000ReadByte (UINT32 address)
{
	struct STARSCREAM_MEMORYIO *mr;
	UINT32 low;

	for (mr = s68000context.memoryreadbyte; (low = mr->lowaddr) != 0xffffffff; mr++)
		if (address >= low && address <= mr->highaddr)
		{
			if (mr->memorycall)
				return ((ReadHandler)mr->memorycall) (address, mr);
			else
				return *(UINT8 *)((UINT32)mr->userdata + address - low);
		}
	return 0xff;
}


UINT32 s68000ReadWord (UINT32 address)
{
	struct STARSCREAM_MEMORYIO *mr;
	UINT32 low;

	for (mr = s68000context.memoryreadword; (low = mr->lowaddr) != 0xffffffff; mr++)
		if (address >= low && address <= mr->highaddr)
		{
			if (mr->memorycall)
				return ((ReadHandler)mr->memorycall) (address, mr);
			else
				return *(UINT16 *)((UINT32)mr->userdata + address - low);
		}
	return 0xffff;
}


UINT32 s68000ReadLong (UINT32 address)
{
	return (s68000ReadWord (address) << 16) + s68000ReadWord (address + 2);
}


void s68000WriteByte (UINT32 address, UINT32 data)
{
	struct STARSCREAM_MEMORYIO *mr;
	UINT32 low;

	for (mr = s68000context.memorywritebyte; (low = mr->lowaddr) != 0xffffffff; mr++)
		if (address >= low && address <= mr->highaddr)
		{
			if (mr->memorycall)
				((WriteHandler)mr->memorycall) (address, data, mr);
			else
				*(UINT8 *)((UINT32)mr->userdata + address - low) = data;
		}
}


void s68000WriteWord (UINT32 address, UINT32 data)
{
	struct STARSCREAM_MEMORYIO *mr;
	UINT32 low;

	for (mr = s68000context.memorywriteword; (low = mr->lowaddr) != 0xffffffff; mr++)
		if (address >= low && address <= mr->highaddr)
		{
			if (mr->memorycall)
				((WriteHandler)mr->memorycall) (address, data, mr);
			else
				*(UINT16 *)((UINT32)mr->userdata + address - low) = data;
		}
}


UINT32 s68000WriteLong (UINT32 address, UINT32 data)
{
	s68000WriteWord (address, data >> 16);
	s68000WriteWord (address + 2, data & 0xffff);
}


