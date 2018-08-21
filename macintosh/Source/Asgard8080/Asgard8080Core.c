//###################################################################################################
//
//
//		Asgard8080Core.c
//		See Asgard8080DefaultConfig.h for compile-time configuration and optimization.
//
//		A PowerPC assembly Intel 8080 emulation core written by Aaron Giles
//		This code is free for use in any emulation project as long as the following credit 
//		appears in the about box and documentation:
//
//			PowerPC-optimized I8080 emulation provided by Aaron Giles and the MAME project.
//
//		I would also appreciate a free copy of any project making use of this code.  Please take
//		the time to contact me if you are using this code or if you have any bugs to report;
//		It is quite possible that I have a newer version.  My email address is aaron@aarongiles.com
//
//		This file looks best when viewed with a tab size of 4 characters
//
//
//###################################################################################################
//
//
//		Revision history:
//
//		10/19/97		1.0		First MAME version
//		10/26/97		1.1		Added MAME support for the external previous PC
//		11/14/97		1.2		Fixed some bugs related to changes made in 1.01
//		 2/04/98		1.3		Added 16-bit I/O support for MAME
//		 1/07/99		1.4		LBO - Modified to work with Juergen's headers and the new interrupt system
//		 3/17/99		2.0		Rewrote extensively
//		 7/07/03		2.1		LBO - added Mach-O support
//
//
//###################################################################################################


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Asgard8080Core.h"
#include "Asgard8080Internals.h"

#if TARGET_RT_MAC_CFM
// we assume TOC-based data; this forces it on
#pragma toc_data on
#endif


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	// these flags indicate actions to be performed on exit from the core
	kExitActionGenerateTRAP		= 0x01,
	kExitActionGenerateINTR		= 0x02,
	kExitActionGenerateRST55	= 0x04,
	kExitActionGenerateRST65	= 0x08,
	kExitActionGenerateRST75	= 0x10
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef struct
{
	// context variables containing the 8080 registers
	unsigned long 			fPCAF;						// PC, A, and F registers
	unsigned long 			fHLBC;						// H, L, B, and C registers
	unsigned long 			fSPDE;						// SP, D, and E registers
	unsigned long 			fFlags;						// internal flags and various

	// context variables describing the current 8080 interrupt state
	unsigned char 			fTRAPState;					// current state of the TRAP line
	unsigned char 			fRST75State;				// current state of the RST7.5 line
	unsigned char			fRST75Latch;				// current state of the RST7.5 latch
	unsigned char 			fRST65State;				// current state of the RST6.5 line
	unsigned char 			fRST55State;				// current state of the RST5.5 line
	unsigned char 			fINTRState;					// current state of the IRQ line
	Asgard8080IRQCallback 	fIRQCallback;				// callback routine for IRQ lines
	Asgard8080SODCallback 	fSODCallback;				// callback routine for serial output
	signed long				fInterruptCycleAdjust;		// number of cycles to adjust on exit
} Asgard8080Context;


//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// externally defined variables
extern unsigned char *			A8080_OPCODEROM;		// pointer to the ROM base
extern unsigned char *			A8080_ARGUMENTROM;		// pointer to the argument ROM base
extern int						A8080_ICOUNT;			// cycles remaining to execute

// other stuff
static signed long 				sRequestedCycles;		// the originally requested number of cycles
static signed long 				sExitActionCycles;		// number of cycles removed to force an exit action
static unsigned char			sExitActions;			// current exit actions
static unsigned char			sExecuting;				// true if we're currently executing

// context variables containing the 8080 registers
static unsigned long 			sPCAF;					// PC, A, and F registers
static unsigned long 			sHLBC;					// H, L, B, and C registers
static unsigned long 			sSPDE;					// SP, D, and E registers
static unsigned long 			sFlags;					// internal flags and various
static unsigned long 			sOpcodePC;				// contains the PC of the current opcode

// local variables describing the current 8080 interrupt state
static unsigned char 			sTRAPState;				// current state of the TRAP line
static unsigned char 			sRST75State;			// current state of the RST7.5 line
static unsigned char 			sRST75Latch;			// current state of the RST7.5 latch
static unsigned char 			sRST65State;			// current state of the RST6.5 line
static unsigned char 			sRST55State;			// current state of the RST5.5 line
static unsigned char 			sINTRState;				// current state of the INTR line
static Asgard8080IRQCallback 	sIRQCallback;			// callback routine for IRQ lines
static Asgard8080SODCallback 	sSODCallback;			// callback routine for serial output
static signed long				sInterruptCycleAdjust;	// number of cycles to adjust on exit


//###################################################################################################
//	STATIC TABLES
//###################################################################################################

typedef struct
{
	unsigned char		fZSP[0x100];
	unsigned short		fDAA[0x800];
} LookupTables;

static LookupTables sLookupTables =
{
	{ 0 },
	{
		#include "Asgard8080DAA.h"
	}
};


//###################################################################################################
//	FUNCTION TABLES
//###################################################################################################

#if (A8080_CHIP == 8080)
#define rim		illegal
#define sim		illegal
#endif

static void *sOpcodeTable[0x100] =
{
	nop,		lxi_b,		stax_b,		inx_b,		inr_b,		dcr_b,		mvi_b,		rlc,		/* 00 */
	illegal,	dad_b,		ldax_b,		dcx_b,		inr_c,		dcr_c,		mvi_c,		rrc,
	illegal,	lxi_d,		stax_d,		inx_d,		inr_d,		dcr_d,		mvi_d,		ral,		/* 10 */
	illegal,	dad_d,		ldax_d,		dcx_d,		inr_e,		dcr_e,		mvi_e,		rar,
	rim,		lxi_h,		shld,		inx_h,		inr_h,		dcr_h,		mvi_h,		daa,		/* 20 */
	illegal,	dad_h,		lhld,		dcx_h,		inr_l,		dcr_l,		mvi_l,		cma,
	sim,		lxi_sp,		stax,		inx_sp,		inr_m,		dcr_m,		mvi_m,		stc,		/* 30 */
	illegal,	dad_sp,		ldax,		dcx_sp,		inr_a,		dcr_a,		mvi_a,		cmf,
	mov_b_b,	mov_b_c,	mov_b_d,	mov_b_e,	mov_b_h,	mov_b_l,	mov_b_m,	mov_b_a,	/* 40 */
	mov_c_b,	mov_c_c,	mov_c_d,	mov_c_e,	mov_c_h,	mov_c_l,	mov_c_m,	mov_c_a,
	mov_d_b,	mov_d_c,	mov_d_d,	mov_d_e,	mov_d_h,	mov_d_l,	mov_d_m,	mov_d_a,	/* 50 */
	mov_e_b,	mov_e_c,	mov_e_d,	mov_e_e,	mov_e_h,	mov_e_l,	mov_e_m,	mov_e_a,
	mov_h_b,	mov_h_c,	mov_h_d,	mov_h_e,	mov_h_h,	mov_h_l,	mov_h_m,	mov_h_a,	/* 60 */
	mov_l_b,	mov_l_c,	mov_l_d,	mov_l_e,	mov_l_h,	mov_l_l,	mov_l_m,	mov_l_a,
	mov_m_b,	mov_m_c,	mov_m_d,	mov_m_e,	mov_m_h,	mov_m_l,	halt,		mov_m_a,	/* 70 */
	mov_a_b,	mov_a_c,	mov_a_d,	mov_a_e,	mov_a_h,	mov_a_l,	mov_a_m,	mov_a_a,
	add_b,		add_c,		add_d,		add_e,		add_h,		add_l,		add_m,		add_a,		/* 80 */
	adc_b,		adc_c,		adc_d,		adc_e,		adc_h,		adc_l,		adc_m,		adc_a,
	sub_b,		sub_c,		sub_d,		sub_e,		sub_h,		sub_l,		sub_m,		sub_a,		/* 90 */
	sbb_b,		sbb_c,		sbb_d,		sbb_e,		sbb_h,		sbb_l,		sbb_m,		sbb_a,
	ana_b,		ana_c,		ana_d,		ana_e,		ana_h,		ana_l,		ana_m,		ana_a,		/* a0 */
	xra_b,		xra_c,		xra_d,		xra_e,		xra_h,		xra_l,		xra_m,		xra_a,
	ora_b,		ora_c,		ora_d,		ora_e,		ora_h,		ora_l,		ora_m,		ora_a,		/* b0 */
	cmp_b,		cmp_c,		cmp_d,		cmp_e,		cmp_h,		cmp_l,		cmp_m,		cmp_a,
	rnz,		pop_b,		jnz,		jmp,		cnz,		push_b,		adi,		rst_0,		/* c0 */
	rz,			ret,		jz,			illegal,	cz,			call,		aci,		rst_1,
	rnc,		pop_d,		jnc,		out,		cnc,		push_d,		sui,		rst_2,		/* d0 */
	rc,			illegal,	jc,			in,			cc,			illegal,	sbi,		rst_3,
	rpo,		pop_h,		jpo,		xthl,		cpe,		push_h,		ani,		rst_4,		/* e0 */
	rpe,		pchl,		jpe,		xchg,		cpo,		illegal,	xri,		rst_5,
	rp,			pop_a,		jp,			di,			cp,			push_a,		ori,		rst_6,		/* f0 */
	rm,			sphl,		jm,			ei,			cm,			illegal,	cpi,		rst_7
};


//###################################################################################################
//	INLINE FUNCTIONS
//###################################################################################################

#pragma mark ¥ INLINE FUNCTIONS

//###################################################################################################
//
//	PushByte -- push a byte onto the stack
//
//###################################################################################################

static inline void PushByte(unsigned char inValue)
{
	sSPDE -= 0x00010000;
	WRITEMEM(sSPDE >> 16, inValue);
}


//###################################################################################################
//
//	PushByte -- push a word onto the stack
//
//###################################################################################################

static inline void PushWord(unsigned short inValue)
{
	PushByte(inValue >> 8);
	PushByte(inValue & 0xff);
}


//###################################################################################################
//
//	ProcessInterrupt -- process an interrupt generically
//
//###################################################################################################

static inline void ProcessInterrupt(int inLine, int inNewPC)
{
	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// mark the registers dirty
	sFlags |= kFlagsDirty;
	
	// see if the processor was halted, and unhalt it if so
	if (sFlags & kFlagsHALT)
	{
		sFlags &= ~kFlagsHALT;
		sPCAF += 0x00010000;
	}
	
	// get the vector and disable interrupts
	if (inLine >= 0)
	{
		(*sIRQCallback)(inLine);
		sFlags &= ~(kFlagsIEN | kFlagsRealIEN);
	}
	else
		sFlags &= ~kFlagsRealIEN;
	
	// set the new PC
	PushWord(sPCAF >> 16);
	sPCAF = (inNewPC << 16) + (sPCAF & 0xffff);
	sInterruptCycleAdjust += 11;			// RST $xx + 2 cycles
	
#ifdef A8080_UPDATEBANK
	A8080_UPDATEBANK(sPCAF >> 16);
#endif
}


//###################################################################################################
//	IMPLEMENTATION
//###################################################################################################

#pragma mark -
#pragma mark ¥ LOCAL FUNCTIONS

//###################################################################################################
//
//	InitTables -- prepare all the internal tables for runtime
//
//	This function dereferences all our opcode function addresses (we don't need to use 
//	proper ptr_glue for these because they're all local, but CodeWarrior won't let us set 
//	up a table of local assembly labels, so we must resort to this hack)
//
//###################################################################################################

static void InitTables(void)
{
	static int done = false;
	int i;

	// only do it once
	if (done)
		return;
	done = true;
	
	// create the ZSP table
	for (i = 0; i < 256; ++i)
	{
		unsigned char zs = 0;
		unsigned char p = 0;

		if (i == 0)
			zs |= k8080FlagZ;
		if (i & 0x80)
			zs |= k8080FlagS;
			
		if (i & 1) ++p;
		if (i & 2) ++p;
		if (i & 4) ++p;
		if (i & 8) ++p;
		if (i & 16) ++p;
		if (i & 32) ++p;
		if (i & 64) ++p;
		if (i & 128) ++p;

		sLookupTables.fZSP[i] = zs | ((p & 1) ? 0 : k8080FlagV);
	}

#if TARGET_RT_MAC_CFM
	// dereferences the opcodes
	for (i = 0; i < 0x100; ++i)
		sOpcodeTable[i] = *(void **)sOpcodeTable[i];
#endif
}


//###################################################################################################
//
//	ProcessINTR -- process an INTR interrupt
//
//###################################################################################################

static void ProcessINTR(void)
{
	int vector;

	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// mark the registers dirty
	sFlags |= kFlagsDirty;
	
	// see if the processor was halted, and unhalt it if so
	if (sFlags & kFlagsHALT)
	{
		sFlags &= ~kFlagsHALT;
		sPCAF += 0x00010000;
	}
	
	// disable interrupts
	sFlags &= ~(kFlagsIEN | kFlagsRealIEN);
	
	// get the vector
	vector = (*sIRQCallback)(k8080IRQLineINTR);
	
	// switch off the opcode
	switch (vector & 0xff0000)
	{
		case 0xcd0000:	// CALL
			PushWord(sPCAF >> 16);
			sInterruptCycleAdjust += 7;				// CALL $xxxx + 2 cycles
			
			// fall through...
		case 0xc30000:	// JMP
			sPCAF = (vector << 16) + (sPCAF & 0xffff);
			sInterruptCycleAdjust += 10;			// JMP $xxxx + 2 cycles
			break;
		
		default:		// assume RST
			PushWord(sPCAF >> 16);
			sPCAF = ((vector & 0x0038) << 16) + (sPCAF & 0xffff);
			sInterruptCycleAdjust += 11;			// RST $xx + 2 cycles
			break;
	}
	
#ifdef A8080_UPDATEBANK
	A8080_UPDATEBANK(sPCAF >> 16);
#endif
}


#if A8080_CHIP == 8085
//###################################################################################################
//
//	ProcessRST55 -- process an RST55 interrupt
//
//###################################################################################################

static void ProcessRST55(void)
{
	ProcessInterrupt(k8080IRQLineRST55, 0x002c);
}


//###################################################################################################
//
//	ProcessRST65 -- process an RST65 interrupt
//
//###################################################################################################

static void ProcessRST65(void)
{
	ProcessInterrupt(k8080IRQLineRST65, 0x0034);
}


//###################################################################################################
//
//	ProcessRST75 -- process an RST75 interrupt
//
//###################################################################################################

static void ProcessRST75(void)
{
	sRST75Latch = 0;
	ProcessInterrupt(k8080IRQLineRST75, 0x003c);
}
#endif


//###################################################################################################
//
//	ProcessTRAP -- process an RST75 interrupt
//
//###################################################################################################

static void ProcessTRAP(void)
{
	ProcessInterrupt(-1, 0x0024);
}


//###################################################################################################
//
//	CheckIRQ -- see if we should generante an IRQ now
//
//###################################################################################################

static void CheckIRQ(int inProcessOK)
{
#if A8080_CHIP == 8085
	// check the RST 7.5 latch first
	if (sRST75Latch && (sFlags & kFlagsRealIEN) && !(sFlags & kFlagsM75))
	{
		if (!inProcessOK)
		{
			sExitActions |= kExitActionGenerateRST75;
			sExitActionCycles += A8080_ICOUNT;
			A8080_ICOUNT = 0;
		}
		else
			ProcessRST75();
		return;
	}
	
	// check the RST 6.5 state next
	if (sRST65State && (sFlags & kFlagsRealIEN) && !(sFlags & kFlagsM65))
	{
		if (!inProcessOK)
		{
			sExitActions |= kExitActionGenerateRST65;
			sExitActionCycles += A8080_ICOUNT;
			A8080_ICOUNT = 0;
		}
		else
			ProcessRST65();
		return;
	}
	
	// check the RST 5.5 state next
	if (sRST55State && (sFlags & kFlagsRealIEN) && !(sFlags & kFlagsM55))
	{
		if (!inProcessOK)
		{
			sExitActions |= kExitActionGenerateRST55;
			sExitActionCycles += A8080_ICOUNT;
			A8080_ICOUNT = 0;
		}
		else
			ProcessRST55();
		return;
	}
#endif	

	// check the INTR state last
	if (sINTRState && (sFlags & kFlagsRealIEN))
	{
		if (!inProcessOK)
		{
			sExitActions |= kExitActionGenerateINTR;
			sExitActionCycles += A8080_ICOUNT;
			A8080_ICOUNT = 0;
		}
		else
			ProcessINTR();
	}
}


#pragma mark -
#pragma mark ¥ CORE IMPLEMENTATION

//###################################################################################################
//
//	Asgard8080Init -- init the 8080 processor state variables
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state variables
//
//###################################################################################################

void Asgard8080Init(void)
{
}

//###################################################################################################
//
//	Asgard8080Reset -- reset the 8080 processor
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state and create the tables
//
//###################################################################################################

void Asgard8080Reset(void)
{
	// reset the internal interrupt states
	sFlags = kFlagsM75 | kFlagsM65 | kFlagsM55;

	// reset the PC
	sPCAF &= 0xffff;
#ifdef A8080_UPDATEBANK
	A8080_UPDATEBANK(sPCAF >> 16);
#endif

	// reset the interrupt states
	sIRQCallback = NULL;
	sSODCallback = NULL;
	sInterruptCycleAdjust = 0;
	sRST75Latch = 0;
	Asgard8080SetIRQLine(k8080IRQLineNMI, k8080IRQStateClear);
	Asgard8080SetIRQLine(k8080IRQLineINTR, k8080IRQStateClear);
	Asgard8080SetIRQLine(k8080IRQLineRST75, k8080IRQStateClear);
	Asgard8080SetIRQLine(k8080IRQLineRST65, k8080IRQStateClear);
	Asgard8080SetIRQLine(k8080IRQLineRST55, k8080IRQStateClear);

	// initialize our internal tables
	InitTables();
}


//###################################################################################################
//
//	Asgard8080SetContext -- set the contents of the 8080 registers
//
//	This function can unfortunately be called at any time to change the contents of the
//	8080 registers.  Call Asgard8080GetContext to get the original values before changing them.
//
//###################################################################################################

void Asgard8080SetContext(void *inContext)
{
	// copy the context
	if (inContext)
	{
		Asgard8080Context *context = inContext;
		
		sPCAF = context->fPCAF;
		sHLBC = context->fHLBC;
		sSPDE = context->fSPDE;
		sFlags = context->fFlags | kFlagsDirty;

		sTRAPState = context->fTRAPState;
		sRST75State = context->fRST75State;
		sRST75Latch = context->fRST75Latch;
		sRST65State = context->fRST65State;
		sRST55State = context->fRST55State;
		sINTRState = context->fINTRState;
		sIRQCallback = context->fIRQCallback;
		sSODCallback = context->fSODCallback;
		sInterruptCycleAdjust = context->fInterruptCycleAdjust;

#ifdef A8080_UPDATEBANK
		A8080_UPDATEBANK(sPCAF >> 16);
#endif
	}
}


//###################################################################################################
//
//	Asgard8080SetReg -- set the contents of one 8080 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	8080 register.
//
//###################################################################################################

void Asgard8080SetReg(int inRegisterIndex, unsigned int inValue)
{
	sFlags |= kFlagsDirty;
	
	switch (inRegisterIndex)
	{
		case k8080RegisterIndexPC:
			sPCAF = (sPCAF & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case k8080RegisterIndexSP:
			sSPDE = (sSPDE & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;

		case k8080RegisterIndexBC:
			sHLBC = (sHLBC & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case k8080RegisterIndexDE:
			sSPDE = (sSPDE & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case k8080RegisterIndexHL:
			sHLBC = (sHLBC & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
			
		case k8080RegisterIndexAF:
			sPCAF = (sPCAF & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case k8080RegisterIndexHALT:
			sFlags = __rlwimi(sFlags, inValue, INSERT_HALT);
			break;
		
		case k8080RegisterIndexIM:
			break;
		
		case k8080RegisterIndexIREQ:
			break;
		
		case k8080RegisterIndexISRV:
			break;
		
		case k8080RegisterIndexVector:
			break;
		
		case k8080RegisterIndexTRAPState:
			sTRAPState = inValue;
			break;
			
		case k8080RegisterIndexINTRState:
			sINTRState = inValue;
			break;

		case k8080RegisterIndexRST55State:
			sRST55State = inValue;
			break;

		case k8080RegisterIndexRST65State:
			sRST65State = inValue;
			break;

		case k8080RegisterIndexRST75State:
			sRST75State = inValue;
			break;
			
		case k8080RegisterIndexOpcodePC:
			sOpcodePC = inValue;
			break;
	}
}
		

//###################################################################################################
//
//	Asgard8080GetContext -- examine the contents of the 8080 registers
//
//	This function can unfortunately be called at any time to return the contents of the
//	8080 registers.
//
//###################################################################################################

unsigned int Asgard8080GetContext(void *outContext)
{
	// copy the context
	if (outContext)
	{
		Asgard8080Context *context = outContext;
		
		context->fPCAF = sPCAF;
		context->fHLBC = sHLBC;
		context->fSPDE = sSPDE;
		context->fFlags = sFlags;

		context->fTRAPState = sTRAPState;
		context->fRST75State = sRST75State;
		context->fRST75Latch = sRST75Latch;
		context->fRST65State = sRST65State;
		context->fRST55State = sRST55State;
		context->fINTRState = sINTRState;
		context->fIRQCallback = sIRQCallback;
		context->fSODCallback = sSODCallback;
		context->fInterruptCycleAdjust = sInterruptCycleAdjust;
	}
	
	// return the size
	return sizeof(Asgard8080Context);
}


//###################################################################################################
//
//	Asgard8080GetReg -- return the contents of one 8080 register
//
//	This function can unfortunately be called at any time to return the contents of a
//	8080 register.
//
//###################################################################################################

unsigned int Asgard8080GetReg(int inRegisterIndex)
{
	switch (inRegisterIndex)
	{
		case k8080RegisterIndexPC:
			return sPCAF >> 16;
		
		case k8080RegisterIndexSP:
			return sSPDE >> 16;

		case k8080RegisterIndexBC:
			return sHLBC & 0xffff;
			
		case k8080RegisterIndexDE:
			return sSPDE & 0xffff;
			
		case k8080RegisterIndexHL:
			return sHLBC >> 16;
			
		case k8080RegisterIndexAF:
			return sPCAF & 0xffff;
			
		case k8080RegisterIndexHALT:
			return __rlwinm(sFlags, EXTRACT_HALT);
		
		case k8080RegisterIndexIM:
			return 0;
		
		case k8080RegisterIndexIREQ:
			return 0;
		
		case k8080RegisterIndexISRV:
			return 0;
		
		case k8080RegisterIndexVector:
			return 0;

		case k8080RegisterIndexTRAPState:
			return sTRAPState;
			
		case k8080RegisterIndexINTRState:
			return sINTRState;

		case k8080RegisterIndexRST55State:
			return sRST55State;

		case k8080RegisterIndexRST65State:
			return sRST65State;

		case k8080RegisterIndexRST75State:
			return sRST75State;
		
		case k8080RegisterIndexOpcodePC:
			return sOpcodePC;
	}
	
	return 0;
}
		

//###################################################################################################
//
//	Asgard8080SetIRQLine -- sets the state of the IRQ/FIRQ lines
//
//###################################################################################################

void Asgard8080SetIRQLine(int inIRQLine, int inState)
{
	switch (inIRQLine)
	{
		case INPUT_LINE_NMI:
			// if the state is the same as last time, bail
			if (sTRAPState == inState) 
				return;
			sTRAPState = inState;

			// detect when the state goes non-clear
			if (inState != k8080IRQStateClear)
			{
				// if we're inside the execution loop, just set the state bit and force us to exit
				if (sExecuting)
				{
					sExitActions |= kExitActionGenerateTRAP;
					sExitActionCycles += A8080_ICOUNT;
					A8080_ICOUNT = 0;
				}
				// else process it right away
				else
					ProcessTRAP();
			}
			break;
			
		case k8080IRQLineRST75:
			// check for a rising edge
			if (inState && !sRST75State)
				sRST75Latch = 1;
			sRST75State = inState;
			break;

		case k8080IRQLineRST65:
			sRST65State = inState;
			break;

		case k8080IRQLineRST55:
			sRST55State = inState;
			break;

		case k8080IRQLineINTR:
			sINTRState = inState;
			break;
	}

	// bail if the new state is clear
	if (inState == k8080IRQStateClear)
		return;
		
	// check for IRQs
	CheckIRQ(!sExecuting);
}


//###################################################################################################
//
//	Asgard8080SetIRQCallback -- sets the function to be called when an interrupt is generated
//
//###################################################################################################

void Asgard8080SetIRQCallback(Asgard8080IRQCallback inCallback)
{
	sIRQCallback = inCallback;
}

//###################################################################################################
//
//	Asgard8080GetIRQCallback -- gets the function to be called when an interrupt is generated
//
//###################################################################################################

Asgard8080IRQCallback Asgard8080GetIRQCallback(void)
{
	return sIRQCallback;
}


#if A8080_CHIP == 8085
//###################################################################################################
//
//	Asgard8080SetSODCallback -- sets the function to be called when a serial bit is output
//
//###################################################################################################

void Asgard8080SetSODCallback(Asgard8080SODCallback inCallback)
{
	sSODCallback = inCallback;
}


//###################################################################################################
//
//	Asgard8080SetSIDLine -- sets the state of the serial input line
//
//###################################################################################################

void Asgard8080SetSIDLine(int inState)
{
	if (inState)
		sFlags |= kFlagsSID;
	else
		sFlags &= ~kFlagsSID;
}
#endif


//###################################################################################################
//
//	Asgard8080Execute -- run the CPU emulation
//
//	This function executes the 8080 for the specified number of cycles, returning the actual
//	number of cycles executed.
//
//###################################################################################################

asm int Asgard8080Execute(register int inCycles)
{
#if (__MWERKS__ >= 0x2300)
	nofralloc
#endif

	//================================================================================================

	//
	// 	standard function prolog; we use almost every nonvolatile register
	//
	mflr	r0
	stmw	rLastNonVolatile,-76(sp)
	stw		r0,8(sp)
	stwu	sp,-128(sp)
	
	//	
	// 	load the cycle count, accounting for any extra interrupt cycles
	//	also, clear the exit action cycle count
	//
	_asm_get_global(r4,sInterruptCycleAdjust)
	li		r0,0
	sub		rICount,inCycles,r4
	_asm_set_global(r0,sInterruptCycleAdjust)
	_asm_set_global(inCycles,sRequestedCycles)
	_asm_set_global(r0,sExitActionCycles)
	
	//	
	// 	initialize tables & goodies
	//
	_asm_get_global_ptr(rArgumentROMPtr,A8080_ARGUMENTROM)
	_asm_get_global_ptr(rOpcodeROMPtr,A8080_OPCODEROM)
	_asm_get_global_ptr(rICountPtr,A8080_ICOUNT)
	_asm_get_global_ptr(rFlagTable,sLookupTables)
	_asm_get_global_ptr(rOpcodeTable,sOpcodeTable)
	SAVE_ICOUNT
	lwz		rArgumentROM,0(rArgumentROMPtr)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)

	//
	//	restore the state of the machine
	//
	_asm_get_global(rFlags,sFlags)
	_asm_get_global(rPCAF,sPCAF)
	_asm_get_global(rHLBC,sHLBC)
	rlwinm	rFlags,rFlags,0,23,21			// clear the dirty bit in rFlags
	_asm_get_global(rSPDE,sSPDE)
	SAVE_FLAGS
	EI_FLAG_RESET

executeMore:
	//
	//	mark that we're executing and clear the exit actions
	//
	li		r0,1
	li		r9,0
	_asm_set_global_b(r0,sExecuting)
	_asm_set_global_b(r9,sExitActions)

	//================================================================================================

	//
	// 	this is the heart of the 8080 execution loop; the process is basically this: load an opcode,
	// 	increment the R register, look up the function, and call it
	//
executeLoop:

	//
	//	internal debugging hook
	//
executeLoopEI:
#if A8080_COREDEBUG
	mr		r3,rPCAF
	mr		r4,rSPDE
	mr		r5,rHLBC
	mr		r6,rFlags
	mr		r7,rICount
	bl		Asgard8080MiniTrace
#endif

	//
	//	external debugging hook
	//
#ifdef A8080_DEBUGHOOK
	_asm_get_global(r3,mame_debug)
#if TARGET_RT_MAC_CFM
	lwz		r3,0(r3)
#endif
	cmpwi	r3,0
	beq		executeLoopNoDebug
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	stw		rICount,0(rICountPtr)
	bl		A8080_DEBUGHOOK
	_asm_get_global(rFlags,sFlags)
	_asm_get_global(rPCAF,sPCAF)
	_asm_get_global(rSPDE,sSPDE)
	_asm_get_global(rHLBC,sHLBC)
	lwz		rICount,0(rICountPtr)
#endif

executeLoopNoDebug:
	//
	//	read the opcode and branch to the appropriate location
	//
	GET_PC(r4)									// fetch the current PC
	lbzx	r3,rOpcodeROM,r4					// load the opcode
	addis	rPCAF,rPCAF,1						// increment & wrap the PC
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	lwzx	r5,rOpcodeTable,r5					// r5 = rOpcodeTable[r3 << 2]
	li		rCycleCount,0
	mtctr	r5									// ctr = r5
	_asm_set_global(r4,sOpcodePC)				// save the PC
	bctr										// go for it

	//
	//	we get back here after any opcode that needs to store a byte result at (rEA)
	//
executeLoopEndWriteEA:
	rlwinm	r3,rEA,0,16,31						// r3 = address
	bl		WRITEMEM							// perform the write
	bl		postExtern							// post-process

	//
	//	we get back here after any other opcode
	//
executeLoopEnd:
	sub.	rICount,rICount,rCycleCount			// decrement the cycle count
	BRANCH_ON_EI(eiContinue)
	SAVE_ICOUNT
	bgt		executeLoop							// loop if we're not done

	//================================================================================================

executeLoopExit:
	//
	// 	add back any exit action cycles and store the final icount
	//
	_asm_get_global(r3,sExitActionCycles)
	li		r0,0
	add		rICount,rICount,r3
	_asm_set_global(r0,sExitActionCycles)
	stw		rICount,0(rICountPtr)

	//
	// 	save the final state of the machine
	//
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)

	//
	//	mark that we're no longer executing
	//
	li		r0,0
	_asm_set_global_b(r0,sExecuting)
	
	//
	//	see if there are interrupts pending
	//
	_asm_get_global_b(rTempSave,sExitActions)
	andi.	r0,rTempSave,kExitActionGenerateTRAP
	beq		noPendingTRAP
	bl		ProcessTRAP
	bl		postExtern
noPendingTRAP:

#if A8080_CHIP == 8085
	andi.	r0,rTempSave,kExitActionGenerateRST75
	beq		noPendingRST75
	bl		ProcessRST75
	bl		postExtern
noPendingRST75:
	andi.	r0,rTempSave,kExitActionGenerateRST65
	beq		noPendingRST65
	bl		ProcessRST65
	bl		postExtern
noPendingRST65:
	andi.	r0,rTempSave,kExitActionGenerateRST55
	beq		noPendingRST55
	bl		ProcessRST55
	bl		postExtern
noPendingRST55:
#endif

	andi.	r0,rTempSave,kExitActionGenerateINTR
	beq		noPendingINTR
	bl		ProcessINTR
	bl		postExtern
noPendingINTR:
	
	//
	// 	account for any interrupt cycles and store the final cycle count
	//
	_asm_get_global(r3,sInterruptCycleAdjust)
	li		r0,0
	sub.	rICount,rICount,r3
	_asm_set_global(r0,sInterruptCycleAdjust)
	stw		rICount,0(rICountPtr)

	//
	//	if there's time left on the clock, try again
	//
	bgt		executeMore

	//
	// 	standard function epilogue; we also compute the number of cycles processed in r3 for return
	//
	_asm_get_global(r3,sRequestedCycles)
	lwz		r0,136(sp)
	addi	sp,sp,128
	mtlr	r0
	sub		r3,r3,rICount
	lmw		rLastNonVolatile,-76(sp)
	blr

	//================================================================================================

	//
	// 	post-external call update: because an external function can modify the ICount, registers,
	//	etc., this function must be called after every external call
	//
postExtern:
	LOAD_FLAGS
	LOAD_ICOUNT									// rICount = sICount
	andi.	r0,rFlags,kFlagsDirty				// extract the dirty flag
	LOAD_ROM									// reload the ROM pointers
	beqlr										// if the registers aren't dirty, return
	LOAD_PCAF									// get the new PC/AF
	LOAD_HLBC									// get the new HL/BC
	rlwinm	rFlags,rFlags,0,23,21				// clear the dirty bit
	LOAD_SPDE									// get the new SP/DE
	SAVE_FLAGS									// save the updated flags
	blr											// return

	//================================================================================================

#ifdef A8080_UPDATEBANK

	//
	//	post-PC change update: make sure the ROM bank hasn't been switched out from under us
	//
updateBank:
	mflr	rTempSave
	GET_PC(r3)
	bl		A8080_UPDATEBANK
	mtlr	rTempSave
	lwz		rOpcodeROM,0(rOpcodeROMPtr)			// restore the ROM pointer
	lwz		rArgumentROM,0(rArgumentROMPtr)		// restore the argument ROM pointer
	blr

#endif

	//================================================================================================

	//
	//		Illegal opcode handler
	//
entry static illegal
	CYCLES(4)
	b		executeLoopEnd

	//================================================================================================

	//
	//		ADC_A: add with carry to A
	//
entry static adc_a
	GET_A(r3)
adc4:
	CYCLES(4)
adc0:
	GET_A(r4)									// r4 = A
	rlwinm	r5,rPCAF,0,31,31					// r5 = C
	xor		r7,r3,r4							// r7 = r3 ^ A
	add		r6,r3,r4							// r6 = r3 + A
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	add		r6,r6,r5							// r6 = A' = r3 + A + C
	xori	r8,r7,0x80							// r8 = r3 ^ A ^ 0x80
	rlwimi	rPCAF,r6,24,31,31					// C = (A' & 0x100)
	xor		r9,r3,r6							// r9 = r3 ^ A'
	rlwimi	rPCAF,r6,0,24,24					// S = (A' & 0x80)
	xor		r7,r7,r6							// r7 = r3 ^ A ^ A'
	rlwinm	r6,r6,0,24,31						// r6 = A' &= 0xff
	rlwimi	rPCAF,r7,0,27,27					// H = (r3 ^ A ^ A') & 0x10
	and		r8,r8,r9							// r8 = (r3 ^ A ^ 0x80) & (r3 ^ A')
	SET_A(r6)									// A = A'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3 ^ 0x80) & (r3 ^ A') & 0x80
	rlwimi	rPCAF,r7,1,25,25					// Z = (A' == 0)
	b		executeLoopEnd

entry static adc_b
	GET_B(r3)
	b		adc4

entry static adc_c
	GET_C(r3)
	b		adc4

entry static adc_d
	GET_D(r3)
	b		adc4

entry static adc_e
	GET_E(r3)
	b		adc4

entry static adc_h
	GET_H(r3)
	b		adc4

entry static adc_l
	GET_L(r3)
	b		adc4

entry static adc_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		adc0

entry static aci
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		adc0

	//================================================================================================

	//
	//		ADD_A: add to A
	//
entry static add_a
	GET_A(r3)
add4:
	CYCLES(4)
add0:
	GET_A(r4)									// r4 = A
	xor		r7,r3,r4							// r7 = A ^ r3
	add		r6,r3,r4							// r6 = A' = A + r3
	rlwinm	rPCAF,rPCAF,0,0,23					// N = 0 (actually all = 0)
	xori	r8,r7,0x80							// r8 = A ^ r3 ^ 0x80
	rlwimi	rPCAF,r6,24,31,31					// C = (A' & 0x100)
	xor		r9,r3,r6							// r9 = r3 ^ A'
	rlwimi	rPCAF,r6,0,24,24					// S = (A' & 0x80)
	xor		r7,r7,r6							// r7 = A ^ r3 ^ A'
	rlwinm	r6,r6,0,24,31						// r6 = A' &= 0xff
	rlwimi	rPCAF,r7,0,27,27					// H = (A ^ r3 ^ A') & 0x10
	and		r8,r8,r9							// r8 = (A ^ r3 ^ 0x80) & (r3 ^ A')
	SET_A(r6)									// A = A'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3 ^ 0x80) & (r3 ^ A') & 0x80
	rlwimi	rPCAF,r7,1,25,25					// Z = (A' == 0)
	b		executeLoopEnd

entry static add_b
	GET_B(r3)
	b		add4

entry static add_c
	GET_C(r3)
	b		add4

entry static add_d
	GET_D(r3)
	b		add4

entry static add_e
	GET_E(r3)
	b		add4

entry static add_h
	GET_H(r3)
	b		add4

entry static add_l
	GET_L(r3)
	b		add4

entry static add_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		add0

entry static adi
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		add0

	//================================================================================================

	//
	//		ADD_HL: add to HL
	//
entry static dad_b
	GET_BC(r3)
addhl11:
	GET_HL(r4)								// r4 = HL
	andi.	r5,rPCAF,k8080FlagS|k8080FlagZ|k8080FlagV|k8080FlagN 	// keep only S/Z/V
	xor		r7,r3,r4						// r7 = HL ^ r3
	add		r6,r3,r4						// r6 = HL' = HL + r3
	rlwimi	rPCAF,r5,0,24,31				// S = S, Z = Z, V = V, N = 0
	xor		r7,r7,r6						// r7 = HL ^ r3 ^ HL'
	rlwimi	rPCAF,r6,16,31,31				// C = (HL' & 0x10000)
	SET_HL(r6)								// HL = HL'
	rlwimi	rPCAF,r7,24,27,27				// H = (HL ^ r3 ^ HL') & 0x1000
	CYCLES(11)
	SAVE_HLBC
	b		executeLoopEnd

entry static dad_d
	GET_DE(r3)
	b		addhl11

entry static dad_h
	GET_HL(r3)
	b		addhl11

entry static dad_sp
	GET_SP(r3)
	b		addhl11

	//================================================================================================

	//
	//		AND_A: ana with A
	//
entry static ana_a
	GET_A(r3)
ana4:
	CYCLES(4)
ana0:
	GET_A(r4)									// r4 = R.AF.B.h
	and		r4,r4,r3							// r4 = R.AF.B.h&Reg
	lbzx	r5,rFlagTable,r4					// r5 = flag bits
	SET_A(r4)									// R.AF.B.h = q
	rlwimi	rPCAF,r5,0,24,31					// set the flags
	ori		rPCAF,rPCAF,k8080FlagH				// also set the H flag
	b		executeLoopEnd

entry static ana_b
	GET_B(r3)
	b		ana4

entry static ana_c
	GET_C(r3)
	b		ana4

entry static ana_d
	GET_D(r3)
	b		ana4

entry static ana_e
	GET_E(r3)
	b		ana4

entry static ana_h
	GET_H(r3)
	b		ana4

entry static ana_l
	GET_L(r3)
	b		ana4

entry static ana_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		ana0

entry static ani
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		ana0

	//================================================================================================

	//
	//		CALL: call a subroutine
	//
entry static cc
	rlwinm.	r0,rPCAF,0,31,31
	beq		SkipCall

entry static call
DoCall:
	CYCLES(17)
	READ_OPCODE_ARG_WORD(rTempSave)				// load the calling address
	PUSH_HI_SAVE(rPCAF)							// push the current PC
	SET_PC(rTempSave)							// set the new PC
	UPDATE_BANK
	b		executeLoopEnd

SkipCall:
	CYCLES(10)
	addis	rPCAF,rPCAF,2						// skip the address
	b		executeLoopEnd
	
entry static cm
	rlwinm.	r0,rPCAF,0,24,24
	beq		SkipCall
	b		DoCall

entry static cnc
	rlwinm.	r0,rPCAF,0,31,31
	bne		SkipCall
	b		DoCall

entry static cnz
	rlwinm.	r0,rPCAF,0,25,25
	bne		SkipCall
	b		DoCall

entry static cp
	rlwinm.	r0,rPCAF,0,24,24
	bne		SkipCall
	b		DoCall

entry static cpe
	rlwinm.	r0,rPCAF,0,29,29
	beq		SkipCall
	b		DoCall

entry static cpo
	rlwinm.	r0,rPCAF,0,29,29
	bne		SkipCall
	b		DoCall

entry static cz
	rlwinm.	r0,rPCAF,0,25,25
	beq		SkipCall
	b		DoCall

	//================================================================================================

	//
	//		CMF: complement the carry flag
	//
entry static cmf
	rlwinm	rPCAF,rPCAF,0,31,29					// clear the N flag
	CYCLES(4)
	rlwimi	rPCAF,rPCAF,4,27,27					// H = C
	xori	rPCAF,rPCAF,1						// complement C
	b		executeLoopEnd

	//================================================================================================

	//
	//		CMP: compare to A
	//
entry static cmp_a
	GET_A(r3)
cmp4:
	CYCLES(4)
cmp0:
	GET_A(r4)									// r4 = A
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	xor		r8,r3,r4							// r8 = A ^ r3
	ori		rPCAF,rPCAF,k8080FlagN				// N = 1
	sub		r6,r4,r3							// r6 = tmp = A - r3
	rlwimi	rPCAF,r6,24,31,31					// C = tmp & 0x100
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,0,24,24					// S = tmp & 0x80
	xor		r7,r8,r6							// r7 = A ^ r3 ^ tmp
	rlwinm	r6,r6,0,24,31						// r6 = tmp &= 0xff
	rlwimi	rPCAF,r7,0,27,27					// H = (A ^ r3 ^ tmp) & 0x10
	and		r8,r8,r9							// r8 = (A ^ r3) & (r3 ^ tmp)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3) & (r3 ^ tmp) & 0x80
	rlwimi	rPCAF,r7,1,25,25					// Z = (tmp == 0)
	b		executeLoopEnd

entry static cmp_b
	GET_B(r3)
	b		cmp4

entry static cmp_c
	GET_C(r3)
	b		cmp4

entry static cmp_d
	GET_D(r3)
	b		cmp4

entry static cmp_e
	GET_E(r3)
	b		cmp4

entry static cmp_h
	GET_H(r3)
	b		cmp4

entry static cmp_l
	GET_L(r3)
	b		cmp4

entry static cmp_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		cmp0

entry static cpi
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		cmp0

	//================================================================================================

	//
	//		CMA: complement accumulator
	//
entry static cma
	xori	rPCAF,rPCAF,0xff00					// complement A
	CYCLES(4)									// count the cycles
	ori		rPCAF,rPCAF,k8080FlagH|k8080FlagN		// set the H,N flags
	b		executeLoopEnd						// continue

	//================================================================================================

	//
	//		DAA: adjust for BCD arithmetic
	//
entry static daa
	rlwinm	r3,rPCAF,25,23,30					// r3 = A << 1
	rlwimi	r3,rPCAF,9,22,22					// r3 |= k8080FlagC
	addi	r4,rFlagTable,LookupTables.fDAA		// r4 -> DAA table
	rlwimi	r3,rPCAF,6,21,21					// r3 |= k8080FlagH
	rlwimi	r3,rPCAF,10,20,20					// r3 |= k8080FlagN
	lhzx	r5,r4,r3							// r5 = DAA[r3]
	CYCLES(4)									// count the cycles
	rlwimi	rPCAF,r5,0,16,31					// set A and the flags
	b		executeLoopEnd						// continue

	//================================================================================================

	//
	//		DEC: decrement an 8-bit value
	//
dec0:
	subi	r4,r3,1								// r4 = r3 - 1
	rlwinm	rPCAF,rPCAF,0,31,23					// clear out all but the carry
	rlwinm	r4,r4,0,24,31						// keep result in range
	ori		rPCAF,rPCAF,k8080FlagN				// set the N flag
	rlwinm	r5,r3,0,28,31						// get low 4 bits of original value in r5
	subi	r3,r3,0x80							// subtract 0x80 from the original value
	rlwimi	rPCAF,r4,0,24,24					// set the S flag
	cntlzw	r6,r4								// r6 = number of zeros in result
	cntlzw	r5,r5								// r5 = number of zeros in low 4 bits
	rlwimi	rPCAF,r6,1,25,25					// set the Z flag
	cntlzw	r3,r3								// r3 = 32 if the original value was 0x80
	rlwimi	rPCAF,r5,31,27,27					// set the H flag
	rlwimi	rPCAF,r3,29,29,29					// set the V flag
	blr
	
entry static dcr_a
	GET_A(r3)
	bl		dec0
	SET_A(r4)
	CYCLES(4)
	b		executeLoopEnd
	
entry static dcr_b
	GET_B(r3)
	bl		dec0
	SET_B(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dcr_c
	GET_C(r3)
	bl		dec0
	SET_C(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dcr_d
	GET_D(r3)
	bl		dec0
	SET_D(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static dcr_e
	GET_E(r3)
	bl		dec0
	SET_E(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static dcr_h
	GET_H(r3)
	bl		dec0
	SET_H(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dcr_l
	GET_L(r3)
	bl		dec0
	SET_L(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dcr_m
	CYCLES(11)
	READ_AT_REG_SAVE_EA(HL)
	bl		dec0
	b		executeLoopEndWriteEA
	
	//================================================================================================

	//
	//		DCX: decrement a 16-bit value
	//
entry static dcx_b
	subi	r3,rHLBC,1
	rlwimi	rHLBC,r3,0,16,31
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dcx_d
	subi	r3,rSPDE,1
	rlwimi	rSPDE,r3,0,16,31
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static dcx_h
	subis	rHLBC,rHLBC,1
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dcx_sp
	subis	rSPDE,rSPDE,1
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		DI: disable interrupts
	//
entry static di
	rlwinm	rFlags,rFlags,0,22,20				// clear RealIEN
	CYCLES(4)
	rlwinm	rFlags,rFlags,0,29,27				// clear IEN
	SAVE_FLAGS
	b		executeLoopEnd

	//================================================================================================

	//
	//		EI: enable interrupts
	//
entry static ei
	ori		rFlags,rFlags,kFlagsRealIEN | kFlagsIEN
	SAVE_FLAGS
	subi	rICount,rICount,4
	EI_FLAG_SET
	SAVE_ICOUNT
	b		executeLoopEI

eiContinue:
	SAVE_ICOUNT
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	li		r3,1
	bl		CheckIRQ
	li		rCycleCount,0
	bl		postExtern
	EI_FLAG_RESET
	b		executeLoopEnd

	//================================================================================================

	//
	//		HALT: halt processing until an interrupt
	//
entry static halt
	ori		rFlags,rFlags,kFlagsHALT			// set the HALT flag
	SAVE_FLAGS									// update the flags
	li		rCycleCount,4						// CYCLES(4)
	cmpwi	rICount,0
	subis	rPCAF,rPCAF,1						// back up the PC so we stay here
	ble		executeLoopEnd
	li		rICount,0
	b		executeLoopEnd

	//================================================================================================

	//
	//		IN: read from an input port
	//
entry static in
	SAVE_PCAF									// save PC/AF
	READ_OPCODE_ARG(r3)							// read the port number from PC
	CYCLES(11)
	bl		A8080_READPORT						// perform the read
	SET_A(r3)
	bl		postExtern							// post-process
	b		executeLoopEnd

	//================================================================================================

	//
	//		INR: increment an 8-bit value
	//
inc0:
	addi	r4,r3,1								// r4 = r3 + 1
	rlwinm	rPCAF,rPCAF,0,31,23					// clear out all but the carry
	rlwinm	r4,r4,0,24,31						// keep result in range
	subi	r3,r4,0x80							// subtract 0x80 from the result
	rlwinm	r5,r4,0,28,31						// get low 4 bits in r5
	rlwimi	rPCAF,r4,0,24,24					// set the S flag
	cntlzw	r6,r4								// r6 = number of zeros in result
	cntlzw	r5,r5								// r5 = number of zeros in low 4 bits
	rlwimi	rPCAF,r6,1,25,25					// set the Z flag
	cntlzw	r3,r3								// r3 = 32 if the original value was 0x80
	rlwimi	rPCAF,r5,31,27,27					// set the H flag
	rlwimi	rPCAF,r3,29,29,29					// set the V flag
	blr
	
entry static inr_a
	GET_A(r3)
	bl		inc0
	SET_A(r4)
	CYCLES(4)
	b		executeLoopEnd
	
entry static inr_b
	GET_B(r3)
	bl		inc0
	SET_B(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inr_c
	GET_C(r3)
	bl		inc0
	SET_C(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inr_d
	GET_D(r3)
	bl		inc0
	SET_D(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static inr_e
	GET_E(r3)
	bl		inc0
	SET_E(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static inr_h
	GET_H(r3)
	bl		inc0
	SET_H(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inr_l
	GET_L(r3)
	bl		inc0
	SET_L(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inr_m
	CYCLES(11)
	READ_AT_REG_SAVE_EA(HL)
	bl		inc0
	b		executeLoopEndWriteEA
	
	//================================================================================================

	//
	//		INX: increment a 16-bit value
	//
entry static inx_b
	addi	r3,rHLBC,1
	SET_BC(r3)
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inx_d
	addi	r3,rSPDE,1
	SET_DE(r3)
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static inx_h
	addis	rHLBC,rHLBC,1
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inx_sp
	addis	rSPDE,rSPDE,1
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		JMP: jump to an absolute address
	//
entry static jc
	andi.	r0,rPCAF,k8080FlagC
	bne		DoJMP
SkipJMP:
	CYCLES(10)
	addis	rPCAF,rPCAF,2						// skip the address
	b		executeLoopEnd

entry static jmp
DoJMP:
	READ_OPCODE_ARG_WORD(r3)					// load the calling address
	subis	rEA,rPCAF,2
	SET_PC(r3)									// set the new PC
	UPDATE_BANK
	xor.	r0,rEA,rPCAF
	CYCLES(10)
	bne+	executeLoopEnd
	cmpwi	rICount,0
	ble		executeLoopEnd
	li		rICount,0
	b		executeLoopEnd

entry static jm
	andi.	r0,rPCAF,k8080FlagS
	bne		DoJMP
	b		SkipJMP

entry static jnc
	andi.	r0,rPCAF,k8080FlagC
	beq		DoJMP
	b		SkipJMP

entry static jnz
	andi.	r0,rPCAF,k8080FlagZ
	beq		DoJMP
	b		SkipJMP

entry static jp
	andi.	r0,rPCAF,k8080FlagS
	beq		DoJMP
	b		SkipJMP

entry static jpe
	andi.	r0,rPCAF,k8080FlagV
	bne		DoJMP
	b		SkipJMP

entry static jpo
	andi.	r0,rPCAF,k8080FlagV
	beq		DoJMP
	b		SkipJMP

entry static jz
	andi.	r0,rPCAF,k8080FlagZ
	bne		DoJMP
	b		SkipJMP

	//================================================================================================

	//
	//		LDAX: load A from memory
	//
entry static ldax_b
	CYCLES(7)
	READ_AT_REG_SAVE(BC)
	SET_A(r3)
	b		executeLoopEnd

entry static ldax_d
	CYCLES(7)
	READ_AT_REG_SAVE(DE)
	SET_A(r3)
	b		executeLoopEnd

entry static ldax
	READ_OPCODE_ARG_WORD(r3)
	SAVE_PCAF									// save PC/AF
	CYCLES(13)
	bl		READMEM								// perform the read
	SET_A(r3)
	bl		postExtern							// post-process
	b		executeLoopEnd

	//================================================================================================

	//
	//		LHLD: load HL from memory
	//
entry static lhld
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(16)
	READ_WORD_SAVE(rEA)
	SET_HL(rTempSave)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		LXI: load immediate 16-bit value to a register
	//
entry static lxi_b
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_BC(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static lxi_d
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_DE(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static lxi_h
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_HL(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static lxi_sp
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_SP(r3)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		MOV: transfer from register to register
	//

	//
	//		MOV A,r
	//
entry static mov_a_a
	CYCLES(4)
	b		executeLoopEnd

entry static mov_a_b
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,0,16,23
	b		executeLoopEnd

entry static mov_a_c
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,8,16,23
	b		executeLoopEnd

entry static mov_a_d
	CYCLES(4)
	rlwimi	rPCAF,rSPDE,0,16,23
	b		executeLoopEnd

entry static mov_a_e
	CYCLES(4)
	rlwimi	rPCAF,rSPDE,8,16,23
	b		executeLoopEnd

entry static mov_a_h
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,16,16,23
	b		executeLoopEnd

entry static mov_a_l
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,24,16,23
	b		executeLoopEnd

entry static mov_a_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_A(r3)
	b		executeLoopEnd

	//
	//		MOV B,r
	//
entry static mov_b_a
	rlwimi	rHLBC,rPCAF,0,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_b_b
	CYCLES(4)
	b		executeLoopEnd

entry static mov_b_c
	rlwimi	rHLBC,rHLBC,8,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_b_d
	rlwimi	rHLBC,rSPDE,0,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_b_e
	rlwimi	rHLBC,rSPDE,8,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_b_h
	rlwimi	rHLBC,rHLBC,16,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_b_l
	rlwimi	rHLBC,rHLBC,24,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_b_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_B(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		MOV C,r
	//
entry static mov_c_a
	rlwimi	rHLBC,rPCAF,24,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_c_b
	rlwimi	rHLBC,rHLBC,24,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_c_c
	CYCLES(4)
	b		executeLoopEnd

entry static mov_c_d
	rlwimi	rHLBC,rSPDE,24,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_c_e
	rlwimi	rHLBC,rSPDE,0,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_c_h
	rlwimi	rHLBC,rHLBC,8,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_c_l
	rlwimi	rHLBC,rHLBC,16,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_c_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_C(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		MOV D,r
	//
entry static mov_d_a
	rlwimi	rSPDE,rPCAF,0,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_d_b
	rlwimi	rSPDE,rHLBC,0,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_d_c
	rlwimi	rSPDE,rHLBC,8,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_d_d
	CYCLES(4)
	b		executeLoopEnd

entry static mov_d_e
	rlwimi	rSPDE,rSPDE,8,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_d_h
	rlwimi	rSPDE,rHLBC,16,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_d_l
	rlwimi	rSPDE,rHLBC,24,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_d_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_D(r3)
	SAVE_SPDE
	b		executeLoopEnd

	//
	//		MOV E,r
	//
entry static mov_e_a
	rlwimi	rSPDE,rPCAF,24,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_e_b
	rlwimi	rSPDE,rHLBC,24,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_e_c
	rlwimi	rSPDE,rHLBC,0,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_e_d
	rlwimi	rSPDE,rSPDE,24,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_e_e
	CYCLES(4)
	b		executeLoopEnd

entry static mov_e_h
	rlwimi	rSPDE,rHLBC,8,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_e_l
	rlwimi	rSPDE,rHLBC,16,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static mov_e_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_E(r3)
	SAVE_SPDE
	b		executeLoopEnd

	//
	//		MOV H,r
	//
entry static mov_h_a
	rlwimi	rHLBC,rPCAF,16,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_h_b
	rlwimi	rHLBC,rHLBC,16,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_h_c
	rlwimi	rHLBC,rHLBC,24,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_h_d
	rlwimi	rHLBC,rSPDE,16,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_h_e
	rlwimi	rHLBC,rSPDE,24,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_h_h
	CYCLES(4)
	b		executeLoopEnd

entry static mov_h_l
	rlwimi	rHLBC,rHLBC,8,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_h_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_H(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		MOV L,r
	//
entry static mov_l_a
	rlwimi	rHLBC,rPCAF,8,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_l_b
	rlwimi	rHLBC,rHLBC,8,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_l_c
	rlwimi	rHLBC,rHLBC,16,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_l_d
	rlwimi	rHLBC,rSPDE,8,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_l_e
	rlwimi	rHLBC,rSPDE,16,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_l_h
	rlwimi	rHLBC,rHLBC,24,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static mov_l_l
	CYCLES(4)
	b		executeLoopEnd

entry static mov_l_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_L(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		MOV (HL),r
	//
entry static mov_m_a
	GET_A(r4)
wrhl7:
	CYCLES(7)
	WRITE_AT_REG_SAVE(HL)
	b		executeLoopEnd

entry static mov_m_b
	GET_B(r4)
	b		wrhl7

entry static mov_m_c
	GET_C(r4)
	b		wrhl7

entry static mov_m_d
	GET_D(r4)
	b		wrhl7

entry static mov_m_e
	GET_E(r4)
	b		wrhl7

entry static mov_m_h
	GET_H(r4)
	b		wrhl7

entry static mov_m_l
	GET_L(r4)
	b		wrhl7

	//================================================================================================

	//
	//		MVI: transfer immediate data to a register
	//
entry static mvi_a
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	SET_A(r3)
	b		executeLoopEnd

entry static mvi_b
	READ_OPCODE_ARG(r3)
	SET_B(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static mvi_c
	READ_OPCODE_ARG(r3)
	SET_C(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static mvi_d
	READ_OPCODE_ARG(r3)
	SET_D(r3)
	CYCLES(7)
	SAVE_SPDE
	b		executeLoopEnd

entry static mvi_e
	READ_OPCODE_ARG(r3)
	SET_E(r3)
	CYCLES(7)
	SAVE_SPDE
	b		executeLoopEnd

entry static mvi_h
	READ_OPCODE_ARG(r3)
	SET_H(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static mvi_l
	READ_OPCODE_ARG(r3)
	SET_L(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static mvi_m
	READ_OPCODE_ARG(r4)
	CYCLES(10)
	WRITE_AT_REG_SAVE(HL)
	b		executeLoopEnd

	//================================================================================================

	//
	//		NOP: do nothing
	//
entry static nop
	CYCLES(4)
	b		executeLoopEnd

	//================================================================================================

	//
	//		OR: logical OR with A
	//
entry static ora_a
	GET_A(r3)
ora4:
	CYCLES(4)
ora0:
	GET_A(r4)									// r4 = R.AF.B.h
	or			r4,r4,r3						// r4 = R.AF.B.h|Reg
	lbzx	r5,rFlagTable,r4					// r5 = flag bits
	SET_A(r4)									// R.AF.B.h = q
	rlwimi	rPCAF,r5,0,24,31					// set the flags
	b		executeLoopEnd

entry static ora_b
	GET_B(r3)
	b		ora4

entry static ora_c
	GET_C(r3)
	b		ora4

entry static ora_d
	GET_D(r3)
	b		ora4

entry static ora_e
	GET_E(r3)
	b		ora4

entry static ora_h
	GET_H(r3)
	b		ora4

entry static ora_l
	GET_L(r3)
	b		ora4

entry static ora_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		ora0

entry static ori
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		ora0

	//================================================================================================

	//
	//		OUT: output a byte
	//
entry static out
	CYCLES(11)
	READ_OPCODE_ARG(r3)							// read the port number from PC
	SAVE_PCAF									// save PC/AF
	GET_A(r4)									// get the value from A
	bl		A8080_WRITEPORT	 					// perform the write
	bl		postExtern							// post-process
	b		executeLoopEnd

	//================================================================================================

	//
	//		PCHL: copy HL to the PC
	//
entry static pchl
	rlwimi	rPCAF,rHLBC,0,0,15
	CYCLES(4)
	UPDATE_BANK
	b		executeLoopEnd

	//================================================================================================

	//
	//		POP: pop a value from the stack
	//
entry static pop_a
	CYCLES(10)
	POP_LO_SAVE(rPCAF)
	b		executeLoopEnd
	
entry static pop_b
	CYCLES(10)
	POP_LO_SAVE(rHLBC)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static pop_d
	CYCLES(10)
	POP_LO_SAVE(rSPDE)
//	SAVE_SPDE -- already saved because of SP change
	b		executeLoopEnd
	
entry static pop_h
	CYCLES(10)
	POP_HI_SAVE(rHLBC)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		PUSH: push a value onto the stack
	//
entry static push_a
	CYCLES(11)
	PUSH_LO_SAVE(rPCAF)
	b		executeLoopEnd
	
entry static push_b
	CYCLES(11)
	PUSH_LO_SAVE(rHLBC)
	b		executeLoopEnd
	
entry static push_d
	CYCLES(11)
	PUSH_LO_SAVE(rSPDE)
	b		executeLoopEnd
	
entry static push_h
	CYCLES(11)
	PUSH_HI_SAVE(rHLBC)
	b		executeLoopEnd
	
	//================================================================================================

	//
	//		RAL: rotate a value left
	//
entry static ral
	rlwinm	r4,rPCAF,1,16,22					// r4 = A << 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,8,23,23					// r4 = (A << 1) | (F & 1)
	rlwimi	r3,rPCAF,17,31,31					// r3 = (rPCAF & 0xec) | (A >> 7)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RAR: rotate a value right
	//
entry static rar
	rlwinm	r4,rPCAF,31,17,23					// r4 = A >> 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,15,16,16					// r4 = (A >> 1) | ((F & 1) << 7)
	rlwimi	r3,rPCAF,24,31,31					// r3 = (rPCAF & 0xec) | (A & 1)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RET: return from a subroutine
	//
entry static ret
	CYCLES(10)
	b		DoRetCore

entry static rc
	andi.	r0,rPCAF,k8080FlagC
	bne		DoRet

SkipRet:
	CYCLES(5)
	b		executeLoopEnd

DoRet:
	CYCLES(11)
DoRetCore:
	POP_HI_SAVE(rPCAF)							// pop the PC
	UPDATE_BANK
	b		executeLoopEnd

entry static rm
	andi.	r0,rPCAF,k8080FlagS
	bne		DoRet
	b		SkipRet

entry static rnc
	andi.	r0,rPCAF,k8080FlagC
	beq		DoRet
	b		SkipRet

entry static rnz
	andi.	r0,rPCAF,k8080FlagZ
	beq		DoRet
	b		SkipRet

entry static rp
	andi.	r0,rPCAF,k8080FlagS
	beq		DoRet
	b		SkipRet

entry static rpe
	andi.	r0,rPCAF,k8080FlagV
	bne		DoRet
	b		SkipRet

entry static rpo
	andi.	r0,rPCAF,k8080FlagV
	beq		DoRet
	b		SkipRet

entry static rz
	andi.	r0,rPCAF,k8080FlagZ
	bne		DoRet
	b		SkipRet

	//================================================================================================

#if A8080_CHIP == 8085

	//
	//		RIM: read interrupt state
	//
entry static rim
	rlwimi	rPCAF,rFlags,8,20,23				// set IEN/M7.5/M6.5/M5.5
	rlwimi	rPCAF,rFlags,8,16,16				// set SID
	_asm_get_global_b(r4,sRST75State)
	_asm_get_global_b(r5,sRST65State)
	_asm_get_global_b(r6,sRST55State)
	cntlzw	r4,r4
	cntlzw	r5,r5
	cntlzw	r6,r6
	xori	r4,r4,0x20
	xori	r5,r5,0x20
	xori	r6,r6,0x20
	rlwimi	rPCAF,r4,9,17,17
	rlwimi	rPCAF,r5,8,18,18
	rlwimi	rPCAF,r6,7,19,19
	CYCLES(7)
	rlwimi	rFlags,rFlags,25,28,28				// IEN = RealIEN
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		RLC: rotate a value left through the carry
	//
entry static rlc
	rlwinm	r4,rPCAF,1,16,22					// r4 = A << 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,25,23,23					// r4 = (A << 1) | (A >> 7)
	rlwimi	r3,rPCAF,17,31,31					// r3 = (rPCAF & 0xec) | (A >> 7)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RRC: rotate a value right through the carry
	//
entry static rrc
	rlwinm	r4,rPCAF,31,17,23					// r4 = A >> 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,7,16,16					// r4 = (A >> 1) | (A << 7)
	rlwimi	r3,rPCAF,24,31,31					// r3 = (rPCAF & 0xec) | (A & 1)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RST: software interrupt
	//
entry static rst_0
	li		rTempSave,0x00
rst:
	CYCLES(11)
	PUSH_HI_SAVE(rPCAF)
	SET_PC(rTempSave)
	UPDATE_BANK
	b		executeLoopEnd
	
entry static rst_1
	li		rTempSave,0x08
	b		rst
	
entry static rst_2
	li		rTempSave,0x10
	b		rst
	
entry static rst_3
	li		rTempSave,0x18
	b		rst
	
entry static rst_4
	li		rTempSave,0x20
	b		rst
	
entry static rst_5
	li		rTempSave,0x28
	b		rst
	
entry static rst_6
	li		rTempSave,0x30
	b		rst
	
entry static rst_7
	li		rTempSave,0x38
	b		rst

	//================================================================================================

	//
	//		SBB: subtract with carry from A
	//
entry static sbb_a
	GET_A(r3)
sbb4:
	CYCLES(4)
sbb0:
	GET_A(r4)									// r4 = R.AF.B.h
	rlwinm	r5,rPCAF,0,31,31					// r5 = R.AF.B.l&1
	rlwinm	rPCAF,rPCAF,0,0,23					// clear all flags
	xor		r7,r3,r4							// r7 = Reg^R.AF.B.h
	sub		r6,r4,r3							// r6 = R.AF.B.h-Reg
	ori		rPCAF,rPCAF,k8080FlagN				// set the N flag
	sub		r6,r6,r5							// r6 = q = R.AF.B.h-Reg-(R.AF.B.l&1)
	rlwimi	rPCAF,r6,24,31,31					// set the C flag
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,0,24,24					// set the S flag
	xor		r8,r7,r6							// r8 = Reg^R.AF.B.h^q
	rlwinm	r6,r6,0,24,31						// r6 &= 0xff
	rlwimi	rPCAF,r8,0,27,27					// set the H flag
	and		r8,r7,r9							// r8 = (Reg^R.AF.B.h)&(Reg^q)
	SET_A(r6)									// R.AF.B.h = q
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// set the V flag
	rlwimi	rPCAF,r7,1,25,25					// set the Z flag
	b		executeLoopEnd

entry static sbb_b
	GET_B(r3)
	b		sbb4

entry static sbb_c
	GET_C(r3)
	b		sbb4

entry static sbb_d
	GET_D(r3)
	b		sbb4

entry static sbb_e
	GET_E(r3)
	b		sbb4

entry static sbb_h
	GET_H(r3)
	b		sbb4

entry static sbb_l
	GET_L(r3)
	b		sbb4

entry static sbb_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		sbb0

entry static sbi
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		sbb0

	//================================================================================================

	//
	//		SHLD: store HL to memory
	//
entry static shld
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(16)
	WRITE_WORD_HI_SAVE(rEA,rHLBC)
	b		executeLoopEnd

	//================================================================================================

#if A8080_CHIP == 8085

	//
	//		SIM: store interrupt state
	//
entry static sim
	andi.	r0,rPCAF,kFlagsMSE << 8				// mask set?
	beq		rimSkipMask
	rlwimi	rFlags,rPCAF,24,29,31
	SAVE_FLAGS
rimSkipMask:
	andi.	r0,rPCAF,kFlagsR75 << 8				// reset R7.5?
	beq		rimSkipReset
	_asm_get_global_b(r3,sRST75State);			// assume it will be immediately set again if the
	_asm_set_global_b(r3,sRST75Latch);			// state is high
rimSkipReset:
	andi.	r0,rPCAF,kFlagsSOE << 8				// set serial data?
	CYCLES(7)
	beq		executeLoopEnd
	_asm_get_global_ptr(r4,sSODCallback);
	rlwinm	r3,rPCAF,17,31,31
	lwz		r4,0(r4)
	cmpwi	r4,0
	beq		executeLoopEnd
	mtctr	r4
	bctrl
	bl		postExtern
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		SPHL: set SP to HL
	//
entry static sphl
	rlwimi	rSPDE,rHLBC,0,0,15
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		STAX: store A to memory
	//
entry static stax_b
	GET_A(r4)
	CYCLES(7)
	WRITE_AT_REG_SAVE(BC)
	b		executeLoopEnd

entry static stax_d
	GET_A(r4)
	CYCLES(7)
	WRITE_AT_REG_SAVE(DE)
	b		executeLoopEnd

entry static stax
	GET_A(r4)
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(13)
	WRITE_BYTE_SAVE(r3)
	b		executeLoopEnd

	//================================================================================================

	//
	//		STC: set the carry flag
	//
entry static stc
	andi.	r3,rPCAF,0xec
	ori		rPCAF,rPCAF,k8080FlagC
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,30
	b		executeLoopEnd

	//================================================================================================

	//
	//		SUB: subtract a value from A
	//
entry static sub_a
	GET_A(r3)
sub4:
	CYCLES(4)
sub0:
	GET_A(r4)									// r4 = A
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	xor		r7,r3,r4							// r8 = A ^ r3
	ori		rPCAF,rPCAF,k8080FlagN				// N = 1
	sub		r6,r4,r3							// r6 = A' = A - r3
	mr		r8,r7								// r8 = A ^ r3
	rlwimi	rPCAF,r6,24,31,31					// C = A' & 0x100
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,0,24,24					// S = A' & 0x80
	xor		r8,r7,r6							// r8 = r3 ^ A ^ A'
	rlwinm	r6,r6,0,24,31						// r6 = A' &= 0xff
	rlwimi	rPCAF,r8,0,27,27					// set the H flag
	and		r8,r7,r9							// r8 = (r3 ^ A) & (r3 ^ A')
	SET_A(r6)									// A = A'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3) & (r3 ^ A') & 0x80
	rlwimi	rPCAF,r7,1,25,25					// set the Z flag
	b		executeLoopEnd

entry static sub_b
	GET_B(r3)
	b		sub4

entry static sub_c
	GET_C(r3)
	b		sub4

entry static sub_d
	GET_D(r3)
	b		sub4

entry static sub_e
	GET_E(r3)
	b		sub4

entry static sub_h
	GET_H(r3)
	b		sub4

entry static sub_l
	GET_L(r3)
	b		sub4

entry static sub_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		sub0

entry static sui
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		sub0

	//================================================================================================

	//
	//		XCHG: exchange HL with DE
	//
entry static xchg
	GET_DE(r3)
	rlwimi	rSPDE,rHLBC,16,16,31
	SET_HL(r3)
	SAVE_SPDE
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		XTHL: exchange HL with the top of the stack
	//
entry static xthl
	GET_SP(rEA)
	CYCLES(19)
	READ_WORD_SAVE(rEA)
	WRITE_WORD_HI(rEA,rHLBC)
	SET_HL(rTempSave)
	SAVE_HLBC
	b		executeLoopEnd
	
	//================================================================================================

	//
	//		XRA: logical exclusive OR with A
	//
entry static xra_a
	GET_A(r3)
xra4:
	CYCLES(4)
xra0:
	GET_A(r4)									// r4 = R.AF.B.h
	xor		r4,r4,r3							// r4 = R.AF.B.h^Reg
	lbzx	r5,rFlagTable,r4					// r5 = flag bits
	SET_A(r4)									// R.AF.B.h = q
	rlwimi	rPCAF,r5,0,24,31					// set the flags
	b		executeLoopEnd

entry static xra_b
	GET_B(r3)
	b		xra4

entry static xra_c
	GET_C(r3)
	b		xra4

entry static xra_d
	GET_D(r3)
	b		xra4

entry static xra_e
	GET_E(r3)
	b		xra4

entry static xra_h
	GET_H(r3)
	b		xra4

entry static xra_l
	GET_L(r3)
	b		xra4

entry static xra_m
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		xra0

entry static xri
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		xra0
}
