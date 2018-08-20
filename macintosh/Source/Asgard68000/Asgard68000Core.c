//###################################################################################################
//
//
//		Asgard68000Core.c
//		See Asgard68000DefaultConfig.h for compile-time configuration and optimization.
//
//		A PowerPC assembly Motorola 68000 emulation core written by Aaron Giles
//		This code is free for use in any emulation project as long as the following credit 
//		appears in the about box and documentation:
//
//			PowerPC-optimized 68000 emulation provided by Aaron Giles and the MAME project.
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
//		10/19/97	1.0		First MAME version
//		03/23/99	2.0		Cleaned up and rewrote portions
//		01/26/01	2.1		LBO - added CHK.L, CHK2/CMP2, fixed EXTB for 020+ targets
//							Opcodes still missing: CALLM/RETM, PACK/UNPK, cp*, CAS/CAS2, LINK.L
//		xx/xx/02	2.2		Kent Miller - fixed a number of '020 issues
//		06/11/02	2.3		LBO - fixed exception handling
//		03/20/03	2.4		LBO - MOVEM.L, .W wasn't using pc-relative memory read handlers
//		07/07/03	2.5		LBO - added Mach-O support
//		09/23/04	2.6		LBO - Added reset callbacks when in supervisor mode.
//
//###################################################################################################


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Asgard68000Core.h"
#include "Asgard68000Internals.h"

#if TARGET_RT_MAC_CFM
// we assume TOC-based data; this forces it on
#pragma toc_data on
#endif
#pragma global_optimizer off

#define USE_SYS16_CALLBACKS 0

//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	// these flags allow interrupts to generated from within an I/O callback
	kExitActionGenerateINT0		= 0x01,
	kExitActionGenerateINT1		= 0x02,
	kExitActionGenerateINT2		= 0x04,
	kExitActionGenerateINT3		= 0x08,
	kExitActionGenerateINT4		= 0x10,
	kExitActionGenerateINT5		= 0x20,
	kExitActionGenerateINT6		= 0x40,
	kExitActionGenerateINT7		= 0x80,
	
	// Possible pseudo-vectors returned by the IRQ ackknowledge handlers
	kIRQAck_Autovector = -1,
	kIRQAck_Spurious = -2,
	
	// these are the various types of exceptions; note that we don't generate them all
	kExceptionReset				= 0,
	kExceptionReset2,
	kExceptionBusError,
	kExceptionAddressError,
	kExceptionIllegalInstruction,
	kExceptionZeroDivide,
	kExceptionCHK,
	kExceptionTRAPV,
	kExceptionPrivilegeViolation,
	kExceptionTrace,
	kExceptionLine1010,
	kExceptionLine1111,
	kExceptionReserved1,
	kExceptionReserved2,
	kExceptionFormatError,
	
	kExceptionSpuriousInterrupt = 24,
	kExceptionInterrupt = 24,
	kExceptionLevel1Interrupt,
	kExceptionLevel2Interrupt,
	kExceptionLevel3Interrupt,
	kExceptionLevel4Interrupt,
	kExceptionLevel5Interrupt,
	kExceptionLevel6Interrupt,
	kExceptionLevel7Interrupt,
	kExceptionTrap00,
	kExceptionTrap01,
	kExceptionTrap02,
	kExceptionTrap03,
	kExceptionTrap04,
	kExceptionTrap05,
	kExceptionTrap06,
	kExceptionTrap07,
	kExceptionTrap08,
	kExceptionTrap09,
	kExceptionTrap0A,
	kExceptionTrap0B,
	kExceptionTrap0C,
	kExceptionTrap0D,
	kExceptionTrap0E,
	kExceptionTrap0F
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef struct
{
	// context variables containing the 68000 registers
	unsigned long			fRegs[2][8];
	unsigned long			fSRFlags;
	unsigned long			fPC;
	unsigned long			fUSP;
	unsigned long			fISP;

	// context variables containing the 68010 registers
	unsigned long			fVBR;
	unsigned long			fSFC;
	unsigned long			fDFC;

	// context variables containing the 68020 registers
	unsigned long			fCACR;
	unsigned long			fCAAR;
	unsigned long			fMSP;

	// context variables describing the current 68000 interrupt state
	unsigned char 			fIRQState;
	Asgard68000IRQCallback 	fIRQCallback;
	signed long				fInterruptCycleAdjust;

	// Misc callbacks
	Asgard68000ResetCallback	fResetCallback;
	Asgard68000CMPILDCallback	fCMPILDCallback;
	Asgard68000RTECallback		fRTECallback;
} Asgard68000Context;


//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// externally defined variables
extern unsigned char *			A68000_OPCODEROM;		// pointer to the ROM base
extern unsigned char *			A68000_ARGUMENTROM;		// pointer to the argument ROM base
extern int						A68000_ICOUNT;			// cycles remaining to execute

// local variables for tracking cycles
static signed long 				sRequestedCycles;		// the originally requested number of cycles
static signed long 				sExitActionCycles;		// number of cycles removed to force an exit action
static unsigned char			sExitVector;			// current exit vector
static unsigned char			sExecuting;				// true if we're currently executing

// local variables containing the 68000 registers
static unsigned long 			sRegs[2][8];			// D and A registers
static unsigned long 			sSRFlags;				// SR and flags
static unsigned long 			sPC;					// PC
static unsigned long 			sUSP;					// User Stack Pointer
static unsigned long 			sISP;					// Interrupt Stack Pointer
static unsigned long 			sOpcodePC;				// contains the PC of the current opcode

// local variables containing the 68010 registers
static unsigned long 			sVBR;					// Vector Base Register
static unsigned long 			sSFC;					// Source Function Code
static unsigned long 			sDFC;					// Destination Function Code

// local variables containing the 68020 registers
static unsigned long 			sCACR;					// Cache Control Register
static unsigned long 			sCAAR;					// Cache Address Register
static unsigned long 			sMSP;					// Master Stack Pointer

// local variables describing the current 68000 interrupt state
static unsigned char			sIRQState;				// current state of the IRQ line
static Asgard68000IRQCallback 	sIRQCallback;			// callback routine for IRQ lines
static signed long 				sInterruptCycleAdjust;	// cycle count adjustment due to interrupts

// local variables for misc callbacks
static Asgard68000ResetCallback		sResetCallback;			// callback routine for Reset instruction
static Asgard68000RTECallback 		sRTECallback;			// callback routine for RTE instruction
static Asgard68000CMPILDCallback 	sCMPILDCallback;		// callback routine for CMPI.L, Dx instruction


// pre-dereferenced pointers to the byte/word/long read functions
static void *					sReadByte;
static void *					sReadWord;
static void *					sReadLong;
static void *					sReadBytePCRel;
static void *					sReadWordPCRel;
static void *					sReadLongPCRel;


//###################################################################################################
//	FUNCTION TABLES
//###################################################################################################

#if (A68000_CHIP < 68010)
#define movec 				illegal
#define move_w_ccr_dy 		illegal
#define move_w_ccr_ay0 		illegal
#define move_w_ccr_ayp 		illegal
#define move_w_ccr_aym 		illegal
#define move_w_ccr_ayd		illegal
#define move_w_ccr_ayid		illegal
#define move_w_ccr_other	illegal
#endif

#if (A68000_CHIP < 68020)
#define extb_l_dy			illegal
#define tst_w_ay 			illegal
#define tst_l_ay 			illegal
#define mul_l_dy_dx			illegal
#define mul_l_ay0_dx		illegal
#define mul_l_ayp_dx		illegal
#define mul_l_aym_dx		illegal
#define mul_l_ayd_dx		illegal
#define mul_l_ayid_dx		illegal
#define mul_l_other_dx		illegal
#define div_l_dy_dx			illegal
#define div_l_ay0_dx		illegal
#define div_l_ayp_dx		illegal
#define div_l_aym_dx		illegal
#define div_l_ayd_dx		illegal
#define div_l_ayid_dx		illegal
#define div_l_other_dx		illegal
#define bfchg_dy			illegal
#define bfchg_ay0			illegal
#define bfchg_ayd			illegal
#define bfchg_ayid			illegal
#define bfchg_other			illegal
#define bfclr_dy			illegal
#define bfclr_ay0			illegal
#define bfclr_ayd			illegal
#define bfclr_ayid			illegal
#define bfclr_other			illegal
#define bfexts_dy_dx		illegal
#define bfexts_ay0_dx		illegal
#define bfexts_ayd_dx		illegal
#define bfexts_ayid_dx		illegal
#define bfexts_other_dx		illegal
#define bfextu_dy_dx		illegal
#define bfextu_ay0_dx		illegal
#define bfextu_ayd_dx		illegal
#define bfextu_ayid_dx		illegal
#define bfextu_other_dx		illegal
#define bfffo_dy_dx			illegal
#define bfffo_ay0_dx		illegal
#define bfffo_ayd_dx		illegal
#define bfffo_ayid_dx		illegal
#define bfffo_other_dx		illegal
#define bfins_dx_dy			illegal
#define bfins_dx_ay0		illegal
#define bfins_dx_ayd		illegal
#define bfins_dx_ayid		illegal
#define bfins_dx_other		illegal
#define bfset_dy			illegal
#define bfset_ay0			illegal
#define bfset_ayd			illegal
#define bfset_ayid			illegal
#define bfset_other			illegal
#define bftst_dy			illegal
#define bftst_ay0			illegal
#define bftst_ayd			illegal
#define bftst_ayid			illegal
#define bftst_other			illegal
#define chk_l_ay0_dx		illegal
#define chk_l_ayd_dx		illegal
#define chk_l_ayid_dx		illegal
#define chk_l_aym_dx		illegal
#define chk_l_ayp_dx		illegal
#define chk_l_dy_dx			illegal
#define chk_l_other_dx		illegal
#define chk2_b_ay0_rx		illegal
#define chk2_b_ayd_rx		illegal
#define chk2_b_ayid_rx		illegal
#define chk2_b_other_rx		illegal
#define chk2_w_ay0_rx		illegal
#define chk2_w_ayd_rx		illegal
#define chk2_w_ayid_rx		illegal
#define chk2_w_other_rx		illegal
#define chk2_l_ay0_rx		illegal
#define chk2_l_ayd_rx		illegal
#define chk2_l_ayid_rx		illegal
#define chk2_l_other_rx		illegal
#endif

#include "Asgard68000OpcodeTable.c"


//###################################################################################################
//	INLINE FUNCTIONS
//###################################################################################################

#pragma mark ¥ INLINE FUNCTIONS

//###################################################################################################
//
//	PushWord -- push a word onto the stack
//
//###################################################################################################

static inline void PushWord(unsigned short inValue)
{
	sRegs[1][7] -= 2;
#if (A68000_ADDRESSBITS == 32)			// AK 2003-07-31
	WRITEWORD(sRegs[1][7], inValue);		// AK 2003-07-31
#else								// AK 2003-07-31
	WRITEWORD(sRegs[1][7] & 0xffffff, inValue);
#endif							// AK 2003-07-31
}


//###################################################################################################
//
//	PushLong -- push a long onto the stack
//
//###################################################################################################

static inline void PushLong(unsigned long inValue)
{
	sRegs[1][7] -= 4;
#if (A68000_ADDRESSBITS == 32)				// AK 2003-07-31
	WRITELONG(sRegs[1][7], inValue);			// AK 2003-07-31
#else			// AK 2003-07-31
	WRITELONG(sRegs[1][7] & 0xffffff, inValue);
#endif								// AK 2003-07-31
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

static void InitTables (void)
{
#if TARGET_RT_MAC_CFM
	static int done = false;
	int i;

	// only do it once
	if (done)
		return;
	done = true;
	
	// dereferences the opcodes
	for (i = 0; i < 0x2000; ++i)
		opcodes[i] = *(void **)opcodes[i];
	
	// dereferences byte/word/load reads
	sReadByte = *(void **)READBYTE;
	sReadWord = *(void **)READWORD;
	sReadLong = *(void **)READLONG;
	sReadBytePCRel = *(void **)READBYTEREL;
	sReadWordPCRel = *(void **)READWORDREL;
	sReadLongPCRel = *(void **)READLONGREL;
#else
	// dereferences byte/word/load reads
	sReadByte = READBYTE;
	sReadWord = READWORD;
	sReadLong = READLONG;
	sReadBytePCRel = READBYTEREL;
	sReadWordPCRel = READWORDREL;
	sReadLongPCRel = READLONGREL;
#endif
}


//###################################################################################################
//
//	GenerateException -- generates an exception of the given type
//
//###################################################################################################

static void GenerateException(int inType)
{
#if (A68000_CHIPTIMING == 68000)
	static const unsigned char sExceptionCycles[] =
	{
		 4,  4, 50, 50, 34, 38, 40, 34, 34, 34,  4,  4,  4,  4,  4, 44,
		 4,  4,  4,  4,  4,  4,  4,  4, 44, 44, 44, 44, 44, 44, 44, 44, 
		34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
		 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4
	};
#elif (A68000_CHIPTIMING == 68010)
	static const unsigned char sExceptionCycles[] =
	{
		 4,  4,126,126, 38, 44, 44, 34, 38, 38,  4,  4,  4,  4,  4, 44,
		 4,  4,  4,  4,  4,  4,  4,  4, 46, 46, 46, 46, 46, 46, 46, 46, 
		38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38,
		 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4
	};
#else
	static const unsigned char sExceptionCycles[] =
	{
		 4,  4, 50, 50, 20, 38, 40, 20, 34, 25, 20, 20,  4,  4,  4, 30,
		 4,  4,  4,  4,  4,  4,  4,  4, 30, 30, 30, 30, 30, 30, 30, 30, 
		20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
		 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4
	};
#endif
	
	unsigned long vector;
	int exception = inType;
	UInt16 stackSR;
	
	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// if we're not in supervisor mode, swap stacks
	if ((sSRFlags & k68000FlagS) == 0)
	{
		sUSP = sRegs[1][7];
		sRegs[1][7] = sISP;
	}
	
	stackSR = sSRFlags & 0xffff;
	
	// set supervisor mode, and clear the trace bit
	sSRFlags |= k68000FlagS;
	sSRFlags &= ~k68000FlagT;

//logerror ("680x0 exception, tyoe: %02x\n", inType);

	// if we're an interrupt, set the interrupt mode as well
	if (inType >= kExceptionInterrupt && inType <= kExceptionLevel7Interrupt)
	{
		sSRFlags = (sSRFlags & ~k68000FlagIMask) | ((inType - kExceptionInterrupt) << k68000FlagIShift);

		// Call IRQ callback if this isn't an NMI
		if ((sIRQCallback) && (inType != kExceptionLevel7Interrupt))
			exception = (*sIRQCallback)(inType - kExceptionInterrupt);
		
		// If it's a normal IRQ, use the regular autovector pointers
		if (exception == kIRQAck_Autovector)
			exception = inType;
		
		// If it's a spurious interrupt, use that vector instead
		else if (exception == kIRQAck_Spurious)
			exception = kExceptionSpuriousInterrupt;

		// also clear any stop state
		sSRFlags &= ~kSRFlagsSTOP;
	}
	
	vector = exception * 4;

	// Set up the stack frame
#if (A68000_CHIP >= 68010)
	// push extra word indicating exception stack frame size; we only generate small ones
	PushWord(0x0000 | (vector & 0x0fff));
		
	// add the VBR to the vector
	vector += sVBR;
#endif
	
	// push the remaining state
	PushLong(sPC);
	PushWord(stackSR);
	
	// set the PC to the requested vector
#if (A68000_ADDRESSBITS == 32)			// AK 2003-07-31
	sPC = READLONG(vector);			// AK 2003-07-31
#else								// AK 2003-07-31
	sPC = READLONG(vector & 0xffffff);
#endif							// AK 2003-07-31
#ifdef A68000_UPDATEBANK
	A68000_UPDATEBANK(sPC);
#endif

	// count cycles to process the interrupt
	sInterruptCycleAdjust += sExceptionCycles[inType];
}


//###################################################################################################
//
//	CheckIRQLines -- checks the current interrupt state
//
//###################################################################################################

static void CheckIRQLines(void)
{
	int currentLevel = (sSRFlags & k68000FlagIMask) >> k68000FlagIShift;
	int state = sIRQState;

	// don't check NMI's -- they are edge-sensitive and always generated immediately
	if (state > currentLevel)
	{
		GenerateException(kExceptionInterrupt + state);
		return;
	}
}


//###################################################################################################
//
//	ResetCallback -- calls through to our reset callback
//
//###################################################################################################

static void ResetCallback(void)
{
	if (sResetCallback)
		(*sResetCallback)();
}


//###################################################################################################
//
//	RTECallback -- calls through to our RTE callback
//
//###################################################################################################

static void RTECallback(void)
{
	if (sRTECallback)
		(*sRTECallback)();
}


//###################################################################################################
//
//	CMPILDCallback -- calls through to our CMPILD callback
//
//###################################################################################################

static void CMPILDCallback(unsigned int val, int reg)
{
	if (sCMPILDCallback)
		(*sCMPILDCallback)(val, reg & 0x07);
}


#pragma mark -
#pragma mark ¥ CORE IMPLEMENTATION

//###################################################################################################
//
//	Asgard68000Init -- set up the 68000 state
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state variables
//
//###################################################################################################

void Asgard68000Init(void)
{
	sResetCallback = NULL;
	sRTECallback = NULL;
	sCMPILDCallback = NULL;

	int cpu = cpu_getactivecpu();

	state_save_register_UINT32("m68kppc", cpu, "D"         , sRegs[0], 8);
	state_save_register_UINT32("m68kppc", cpu, "A"         , sRegs[1], 8);
	state_save_register_UINT32("m68kppc", cpu, "PC"        , &sPC, 1);
	state_save_register_UINT32("m68kppc", cpu, "USP"       , &sUSP, 1);
	state_save_register_UINT32("m68kppc", cpu, "ISP"       , &sISP, 1);
	state_save_register_UINT32("m68kppc", cpu, "MSP"       , &sMSP, 1);
	state_save_register_UINT32("m68kppc", cpu, "VBR"       , &sVBR, 1);
	state_save_register_UINT32("m68kppc", cpu, "SFC"       , &sSFC, 1);
	state_save_register_UINT32("m68kppc", cpu, "DFC"       , &sDFC, 1);
	state_save_register_UINT32("m68kppc", cpu, "CACR"      , &sCACR, 1);
	state_save_register_UINT32("m68kppc", cpu, "CAAR"      , &sCAAR, 1);
	state_save_register_UINT32("m68kppc", cpu, "SRFlags"   , &sSRFlags, 1);
	state_save_register_UINT32("m68kppc", cpu, "IRQState" , &sIRQState, 1);
	state_save_register_UINT32("m68kppc", cpu, "IRQCycleAdjust", &sInterruptCycleAdjust, 1);
}

//###################################################################################################
//
//	Asgard68000Reset -- reset the 68000 processor
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state and create the tables
//
//###################################################################################################

void Asgard68000Reset(void)
{
	// Clear any stop state
	sSRFlags &= ~kSRFlagsSTOP;
	
	// set the interrupt level to 7 and turn on supervisor mode
	sSRFlags |= k68000FlagIMask | k68000FlagS;
	
	// clear the VBR
	sVBR = 0;
	
	// set the initial SP and PC (directly from ROM, since these are fetched
	// the same was as standard opcodes are)
	sRegs[1][7] = *(unsigned long *)&A68000_OPCODEROM[0];
	sPC = *(unsigned long *)&A68000_OPCODEROM[4];
#ifdef A68000_UPDATEBANK
	A68000_UPDATEBANK(sPC);
#endif
	
	// reset the interrupt states
	sIRQState = 0;
	
	// reset the cycles
	sInterruptCycleAdjust = 0;
	
	// make sure our tables have been created
	InitTables();
}


//###################################################################################################
//
//	Asgard68000SetContext -- set the contents of the 68000 registers
//
//	This function can unfortunately be called at any time to change the contents of the
//	68000 registers.  Call Asgard68000GetContext to get the original values before changing them.
//
//###################################################################################################

void Asgard68000SetContext(void *inContext)
{
	if (inContext)
	{
		Asgard68000Context *context = inContext;

		sRegs[0][0] = context->fRegs[0][0];
		sRegs[0][1] = context->fRegs[0][1];
		sRegs[0][2] = context->fRegs[0][2];
		sRegs[0][3] = context->fRegs[0][3];
		sRegs[0][4] = context->fRegs[0][4];
		sRegs[0][5] = context->fRegs[0][5];
		sRegs[0][6] = context->fRegs[0][6];
		sRegs[0][7] = context->fRegs[0][7];

		sRegs[1][0] = context->fRegs[1][0];
		sRegs[1][1] = context->fRegs[1][1];
		sRegs[1][2] = context->fRegs[1][2];
		sRegs[1][3] = context->fRegs[1][3];
		sRegs[1][4] = context->fRegs[1][4];
		sRegs[1][5] = context->fRegs[1][5];
		sRegs[1][6] = context->fRegs[1][6];
		sRegs[1][7] = context->fRegs[1][7];
		
		sSRFlags = context->fSRFlags;
		sPC = context->fPC;
		sISP = context->fISP;
		sUSP = context->fUSP;
		
		sVBR = context->fVBR;
		sSFC = context->fSFC;
		sDFC = context->fDFC;
		
		sCACR = context->fCACR;
		sCAAR = context->fCAAR;
		sMSP = context->fMSP;
		
		sIRQState = context->fIRQState;
		sIRQCallback = context->fIRQCallback;
		sInterruptCycleAdjust = context->fInterruptCycleAdjust;

		sResetCallback = context->fResetCallback;
		sRTECallback = context->fRTECallback;
		sCMPILDCallback = context->fCMPILDCallback;

		CheckIRQLines();

#ifdef A68000_UPDATEBANK
		A68000_UPDATEBANK(sPC);
#endif
	}
}


//###################################################################################################
//
//	Asgard68000SetReg -- set the contents of one 68000 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	68000 register.
//
//###################################################################################################

void Asgard68000SetReg(int inRegisterIndex, unsigned int inValue)
{
	sSRFlags |= kSRFlagsDirty;
	
	switch (inRegisterIndex)
	{
		case k68000RegisterIndexD0:
		case k68000RegisterIndexD1:
		case k68000RegisterIndexD2:
		case k68000RegisterIndexD3:
		case k68000RegisterIndexD4:
		case k68000RegisterIndexD5:
		case k68000RegisterIndexD6:
		case k68000RegisterIndexD7:
			sRegs[0][inRegisterIndex - k68000RegisterIndexD0] = inValue;
			break;
		
		case k68000RegisterIndexA0:
		case k68000RegisterIndexA1:
		case k68000RegisterIndexA2:
		case k68000RegisterIndexA3:
		case k68000RegisterIndexA4:
		case k68000RegisterIndexA5:
		case k68000RegisterIndexA6:
		case k68000RegisterIndexA7:
			sRegs[1][inRegisterIndex - k68000RegisterIndexA0] = inValue;
			break;

		case k68000RegisterIndexSR:
			sSRFlags = (sSRFlags & 0xffff0000) | (inValue & 0x0000ffff);
			break;
		
		case k68000RegisterIndexPC:
			sPC = inValue;
			break;
		
		case k68000RegisterIndexSP:
			sRegs[1][7] = inValue;
			break;
		
		case k68000RegisterIndexISP:
			sISP = inValue;
			break;
		
		case k68000RegisterIndexUSP:
			sUSP = inValue;
			break;
		
		case k68000RegisterIndexVBR:
			sVBR = inValue;
			break;
		
		case k68000RegisterIndexSFC:
			sSFC = inValue;
			break;
		
		case k68000RegisterIndexDFC:
			sDFC = inValue;
			break;
		
		case k68000RegisterIndexCACR:
			sCACR = inValue;
			break;
		
		case k68000RegisterIndexCAAR:
			sCAAR = inValue;
			break;
		
		case k68000RegisterIndexMSP:
			sMSP = inValue;
			break;
			
		case k68000RegisterIndexOpcodePC:
			sOpcodePC = inValue;
			break;
	}
}


//###################################################################################################
//
//	Asgard68000GetContext -- examine the contents of the 68000 registers
//
//	This function can unfortunately be called at any time to examine the contents of the
//	68000 registers.
//
//###################################################################################################

unsigned int Asgard68000GetContext(void *outContext)
{
	if (outContext)
	{
		Asgard68000Context *context = outContext;

		context->fRegs[0][0] = sRegs[0][0];
		context->fRegs[0][1] = sRegs[0][1];
		context->fRegs[0][2] = sRegs[0][2];
		context->fRegs[0][3] = sRegs[0][3];
		context->fRegs[0][4] = sRegs[0][4];
		context->fRegs[0][5] = sRegs[0][5];
		context->fRegs[0][6] = sRegs[0][6];
		context->fRegs[0][7] = sRegs[0][7];

		context->fRegs[1][0] = sRegs[1][0];
		context->fRegs[1][1] = sRegs[1][1];
		context->fRegs[1][2] = sRegs[1][2];
		context->fRegs[1][3] = sRegs[1][3];
		context->fRegs[1][4] = sRegs[1][4];
		context->fRegs[1][5] = sRegs[1][5];
		context->fRegs[1][6] = sRegs[1][6];
		context->fRegs[1][7] = sRegs[1][7];
		
		context->fSRFlags = sSRFlags;
		context->fPC = sPC;
		context->fISP = sISP;
		context->fUSP = sUSP;
		
		context->fVBR = sVBR;
		context->fSFC = sSFC;
		context->fDFC = sDFC;
		
		context->fCACR = sCACR;
		context->fCAAR = sCAAR;
		context->fMSP = sMSP;
		
		context->fIRQState = sIRQState;
		context->fIRQCallback = sIRQCallback;
		context->fInterruptCycleAdjust = sInterruptCycleAdjust;

		context->fResetCallback = sResetCallback;
		context->fRTECallback = sRTECallback;
		context->fCMPILDCallback = sCMPILDCallback;
	}
	return sizeof(Asgard68000Context);
}


//###################################################################################################
//
//	Asgard68000GetReg -- set the contents of one 68000 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	68000 register.
//
//###################################################################################################

unsigned int Asgard68000GetReg(int inRegisterIndex)
{
	switch (inRegisterIndex)
	{
		case k68000RegisterIndexD0:
		case k68000RegisterIndexD1:
		case k68000RegisterIndexD2:
		case k68000RegisterIndexD3:
		case k68000RegisterIndexD4:
		case k68000RegisterIndexD5:
		case k68000RegisterIndexD6:
		case k68000RegisterIndexD7:
			return sRegs[0][inRegisterIndex - k68000RegisterIndexD0];
		
		case k68000RegisterIndexA0:
		case k68000RegisterIndexA1:
		case k68000RegisterIndexA2:
		case k68000RegisterIndexA3:
		case k68000RegisterIndexA4:
		case k68000RegisterIndexA5:
		case k68000RegisterIndexA6:
		case k68000RegisterIndexA7:
			return sRegs[1][inRegisterIndex - k68000RegisterIndexA0];

		case k68000RegisterIndexSR:
			return sSRFlags & 0xffff;
		
		case k68000RegisterIndexPC:
			return sPC;
		
		case k68000RegisterIndexSP:
			return sRegs[1][7];
		
		case k68000RegisterIndexISP:
			return sISP;
			
		case k68000RegisterIndexUSP:
			return sUSP;
		
		case k68000RegisterIndexVBR:
			return sVBR;
		
		case k68000RegisterIndexSFC:
			return sSFC;
		
		case k68000RegisterIndexDFC:
			return sDFC;
		
		case k68000RegisterIndexCACR:
			return sCACR;
		
		case k68000RegisterIndexCAAR:
			return sCAAR;
		
		case k68000RegisterIndexMSP:
			return sMSP;
			
		case k68000RegisterIndexOpcodePC:
			return sOpcodePC;
	}
	
	return 0;
}


//###################################################################################################
//
//	Asgard68000SetIRQLine -- sets the state of the IRQ line
//
//###################################################################################################

void Asgard68000SetIRQLine(int inIRQLine, int inState)
{
	unsigned char oldState = sIRQState;
	
	if (inIRQLine == INPUT_LINE_NMI)
		inIRQLine = 7;
	
	// set/clear the appropriate lines
	sIRQState = 0;		// remove me eventually
	if (inState == k68000IRQStateClear)
		sIRQState &= ~inIRQLine;
	else
		sIRQState |= inIRQLine;
	
	// check for edge-sensitive NMIs
	if (sIRQState == 7)
	{
		if (oldState != 7)
			GenerateException(kExceptionLevel7Interrupt);
	}
	
	// else if the state is non-clear, re-check the IRQ states
	if (inState != k68000IRQStateClear)
		CheckIRQLines();
}


//###################################################################################################
//
//	Asgard68000SetIRQCallback -- sets the function to be called when an interrupt is generated
//
//###################################################################################################

void Asgard68000SetIRQCallback(Asgard68000IRQCallback inCallback)
{
	sIRQCallback = inCallback;
}


//###################################################################################################
//
//	Asgard68000SetResetCallback -- sets the function to be called when a RESET is executed
//
//###################################################################################################

void Asgard68000SetResetCallback(Asgard68000ResetCallback inCallback)
{
	sResetCallback = inCallback;
}


//###################################################################################################
//
//	Asgard68000SetRTECallback -- sets the function to be called when a RTE is executed
//
//###################################################################################################

void Asgard68000SetRTECallback(Asgard68000RTECallback inCallback)
{
	sRTECallback = inCallback;
}


//###################################################################################################
//
//	Asgard68000SetCMPILDCallback -- sets the function to be called when a CMPILD is executed
//
//###################################################################################################

void Asgard68000SetCMPILDCallback(Asgard68000CMPILDCallback inCallback)
{
	sCMPILDCallback = inCallback;
}


//###################################################################################################
//
//	Asgard68000Execute -- run the CPU emulation
//
//	This function executes the 68000 for the specified number of cycles, returning the actual
//	number of cycles executed.
//
//###################################################################################################

asm int Asgard68000Execute (register int inCycles)
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
//	lwz		r4,sInterruptCycleAdjust(rtoc)
	_asm_get_global(r4,sInterruptCycleAdjust)
	li		r0,0
	sub		rICount,inCycles,r4
//	stw		r0,sInterruptCycleAdjust(rtoc)
//	stw		inCycles,sRequestedCycles(rtoc)
//	stw		r0,sExitActionCycles(rtoc)
	_asm_set_global(r0,sInterruptCycleAdjust)
	_asm_set_global(inCycles,sRequestedCycles)
	_asm_set_global(r0,sExitActionCycles)
	
	//	
	// 	initialize tables & goodies
	//
//	lwz		rOpcodeROMPtr,A68000_OPCODEROM(rtoc)
//	lwz		rICountPtr,A68000_ICOUNT(rtoc)
//	lwz		rOpcodeTable,opcodes(rtoc)
//	lwz		rLongRegs,sRegs(rtoc)
	_asm_get_global_ptr(rOpcodeROMPtr,A68000_OPCODEROM)
	_asm_get_global_ptr(rICountPtr,A68000_ICOUNT)
	_asm_get_global_ptr(rOpcodeTable,opcodes)
	_asm_get_global_ptr(rLongRegs,sRegs)
	SAVE_ICOUNT
	lwz		rOpcodeROM,0(rOpcodeROMPtr)
	addi	rWordRegs,rLongRegs,2
	addi	rByteRegs,rLongRegs,3
	addi	rAddressRegs,rLongRegs,8*4

	//
	//	restore the state of the machine
	//
//	lwz		rSRFlags,sSRFlags(rtoc)
//	lwz		rPC,sPC(rtoc)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)

	//
	//	mark that we're executing
	//
executeMore:
	li		r0,1
	li		r9,0
//	stb		r0,sExecuting(rtoc)
//	stb		r9,sExitVector(rtoc)
	_asm_set_global_b(r0,sExecuting)
	_asm_set_global_b(r9,sExitVector)

	//
	//	if we're still in a STOP state, eat all cycles and bail
	//
	andis.	r0,rSRFlags,kSRFlagsSTOP>>16
	beq		executeLoop
	li		rICount,0
	b		executeLoopExit

	//================================================================================================

	//
	// 	this is the heart of the MC68000 execution loop; the process is basically this: load an 
	// 	opcode, look up the function, and branch to it
	//
executeLoop:

	//
	//	internal debugging hook
	//
#if A68000_COREDEBUG
	mr		r3,rLongRegs
	mr		r4,rAddressRegs
	mr		r5,rPC
	mr		r6,rSRFlags
	mr		r7,rICount
	bl		Asgard68000MiniTrace
#endif
	
	//
	//	external debugging hook
	//
#ifdef A68000_DEBUGHOOK
	_asm_get_global(r3,mame_debug)
#if TARGET_RT_MAC_CFM
	lwz		r3,0(r3)
#endif
	cmpwi	r3,0
	beq		executeLoopNoDebug
//	stw		rSRFlags,sSRFlags(rtoc)
//	stw		rPC,sPC(rtoc)
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)
	stw		rICount,0(rICountPtr)
	bl		A68000_DEBUGHOOK
//	lwz		rSRFlags,sSRFlags(rtoc)
//	lwz		rPC,sPC(rtoc)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)
	lwz		rICount,0(rICountPtr)
#endif

executeLoopNoDebug:
	//
	//	read the opcode and branch to the appropriate location
	//
	GET_PC(r3)									// fetch the current PC
	lhzx	r11,rOpcodeROM,r3					// load the opcode
	addi	rPC,rPC,2							// increment the PC
	rlwinm	r5,r11,31,17,29						// r5 = (opcode >> 3) << 2
	lwzx	r5,rOpcodeTable,r5					// r5 = rOpcodeTable[opcode << 2]
	rlwinm	rRY,r11,2,27,29						// rRY = 3 bit register offset from low 3 bits -- (opcode & 0x07) * sizeof (UInt32)
	li		rCycleCount,0						// reset the cycle count
	mtctr	r5									// ctr = r5
	rlwinm	rRX,r11,25,27,29					// rRX = 3 bit register offset from higher 3 bits
#if MAME_DEBUG
//lwz		r10,sOpcodePC(rtoc)
_asm_get_global(r10,sOpcodePC)
#endif
//	stw		r3,sOpcodePC(rtoc)					// save the PC
	_asm_set_global(r3,sOpcodePC)				// save the PC
	bctr										// go for it

	//================================================================================================

	//
	//	we get back here after any opcode that modifies the high byte of the SR
	//	new SR value should be in r3
	//
executeLoopEndUpdateSR:
	xor		r4,r3,rSRFlags						// take the xor between the old and new values
	andi.	r0,r4,k68000FlagS					// did supervisor mode change?
	rlwimi	rSRFlags,r3,0,16,31					// copy in the new value
	beq		updateSROk							// if not, skip
	andi.	r0,rSRFlags,k68000FlagS				// are we moving into supervisor mode?
	GET_A7(r5)									// r5 = A7
	beq		updateSRToUser						// if not, skip
	
	//
	//	user mode -> supervisor mode
	//
//	stw		r5,sUSP(rtoc)						// save as the USP
//	lwz		r6,sISP(rtoc)						// get the ISP
	_asm_set_global(r5,sUSP)					// save as the USP
	_asm_get_global(r6,sISP)					// get the ISP
	b		updateSRContinue

	//
	//	supervisor mode -> user mode
	//
updateSRToUser:
//	stw		r5,sISP(rtoc)						// save as the ISP
//	lwz		r6,sUSP(rtoc)						// get the USP
	_asm_set_global(r5,sISP)					// save as the ISP
	_asm_get_global(r6,sUSP)					// get the USP

	//
	//	check our interrupt state
	//
updateSRContinue:
	SET_A7(r6)									// A7 = new SP
updateSROk:
	stw		rICount,0(rICountPtr)
//	stw		rSRFlags,sSRFlags(rtoc)
//	stw		rPC,sPC(rtoc)
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)
	bl		CheckIRQLines
	_asm_get_global_ptr(rOpcodeROMPtr,A68000_OPCODEROM)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)
	lwz		rICount,0(rICountPtr)
//	lwz		rSRFlags,sSRFlags(rtoc)
//	lwz		rPC,sPC(rtoc)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)

	//
	//	update cycles and loop
	//
	sub.	rICount,rICount,rCycleCount			// update the cycle count
	SAVE_ICOUNT
	bgt		executeLoop							// loop if we're not done
	b		executeLoopExit

	//================================================================================================

	//
	//	we get back here after any opcode that writes to memory
	//
executeLoopEndRestore:
	LOAD_SR
executeLoopEndRestoreNoSR:
	LOAD_ICOUNT
	LOAD_PC

	//================================================================================================

	//
	//	we get back here after any other opcode
	//
executeLoopEnd:
	sub.	rICount,rICount,rCycleCount			// update the cycle count
	SAVE_ICOUNT
	bgt		executeLoop							// loop if we're not done

	//================================================================================================

executeLoopExit:
	//
	// 	add back any exit action cycles and store the final icount
	//
//	lwz		r3,sExitActionCycles(rtoc)
	_asm_get_global(r3,sExitActionCycles)
	li		r0,0
	add		rICount,rICount,r3
//	stw		r0,sExitActionCycles(rtoc)
	_asm_set_global(r0,sExitActionCycles)
	stw		rICount,0(rICountPtr)

	//
	// 	save the final state of the machine
	//
//	stw		rSRFlags,sSRFlags(rtoc)
//	stw		rPC,sPC(rtoc)
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)

	//
	//	mark that we're no longer executing
	//
	li		r0,0
//	stb		r0,sExecuting(rtoc)
	_asm_set_global_b(r0,sExecuting)
	
	//
	//	see if there is an exception pending
	//
//	lbz		r3,sExitVector(rtoc)
	_asm_get_global_b(r3,sExitVector)
	cmpwi	r3,0
	beq		noPendingException
	bl		GenerateException
	lwz		rOpcodeROM,0(rOpcodeROMPtr)
	lwz		rICount,0(rICountPtr)
//	lwz		rSRFlags,sSRFlags(rtoc)
//	lwz		rPC,sPC(rtoc)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)
noPendingException:
	
	//
	// 	account for any interrupt cycles and store the final cycle count
	//
//	lwz		r3,sInterruptCycleAdjust(rtoc)
	_asm_get_global(r3,sInterruptCycleAdjust)
	li		r0,0
	sub.	rICount,rICount,r3
//	stw		r0,sInterruptCycleAdjust(rtoc)
	_asm_set_global(r0,sInterruptCycleAdjust)
	stw		rICount,0(rICountPtr)

	//
	//	if there's time left on the clock, try again
	//
	bgt		executeMore

	//
	// 	standard function epilogue; we also compute the number of cycles processed in r3 for return
	//
//	lwz		r3,sRequestedCycles(rtoc)
	_asm_get_global(r3,sRequestedCycles)
	lwz		r0,136(sp)
	addi	sp,sp,128
	mtlr	r0
	sub		r3,r3,rICount
	lmw		rLastNonVolatile,-76(sp)
	blr

	//================================================================================================

	//
	//	this computes the effective address for a source operand in mode 111 (7)
	//	it assumes that rRY contains the "register" field shifted left 2 bits
	//	it also only adds cycles above the immediate case (i.e., it adds 0 cycles
	//		for an immediate)
	//
fetchEAByte111:
	cmpwi	cr1,rRY,4<<2						// compare against 4
	cmpwi	cr2,rRY,2<<2						// compare against 2
//	lwz		r3,sReadByte(rtoc)
//	lwz		r4,sReadBytePCRel(rtoc)
	_asm_get_global(r3,sReadByte)
	_asm_get_global(r4,sReadBytePCRel)
	bne		cr1,fetchEACommon111
	READ_OPCODE_ARG_BYTE(r3)					// r3 = immediate value
#if (A68000_CHIP >= 68020)
	CYCLES(0,0,1)
#endif
	blr

fetchEAWord111:
	cmpwi	cr1,rRY,4<<2						// compare against 4
	cmpwi	cr2,rRY,2<<2						// compare against 2
//	lwz		r3,sReadWord(rtoc)
//	lwz		r4,sReadWordPCRel(rtoc)
	_asm_get_global(r3,sReadWord)
	_asm_get_global(r4,sReadWordPCRel)
	bne		cr1,fetchEACommon111
	READ_OPCODE_ARG(r3)							// r3 = immediate value
#if (A68000_CHIP >= 68020)
	CYCLES(0,0,1)
#endif
	blr
	
fetchEALong111_plus2:
	CYCLES(2,2,0)
fetchEALong111:
	cmpwi	cr1,rRY,4<<2						// compare against 4
	cmpwi	cr2,rRY,2<<2						// compare against 2
//	lwz		r3,sReadLong(rtoc)
//	lwz		r4,sReadLongPCRel(rtoc)
	_asm_get_global(r3,sReadLong)
	_asm_get_global(r4,sReadLongPCRel)
	bne		cr1,fetchEACommon111
	CYCLES(-2,-2,1)
	READ_OPCODE_ARG_LONG(r3)					// r3 = immediate value
	blr
	
computeEA111:
	cmpwi	cr1,rRY,4<<2						// compare against 4
	cmpwi	cr2,rRY,2<<2						// compare against 2
	mflr	r3
	mflr	r4
#if !TARGET_RT_MAC_MACHO
	bge		cr1,illegal							// handle illegal cases (reg >= 4)
#else
	bge		cr1,illegal1						// handle illegal cases (reg >= 4)
#endif

	// common code
fetchEACommon111:
	andi.	r0,rRY,1<<2							// test the low bit
	SAVE_SR										// save the SR, since all other modes require a read
	bge		cr2,fetchEACommon111_pc				// handle pc-relative modes (reg == 2 or reg == 3)
	mtctr	r3
	bne		fetchEACommon111_absl				// handle long absolute mode (reg == 1)

	//	(xxx).W mode (reg == 0)
fetchEACommon111_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(4,4,3)
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	bctr										// read the byte and return
	
	//	(xxx).L mode (reg == 1)
fetchEACommon111_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long word
	SAVE_PC
	CYCLES(8,8,3)
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	bctr										// read the byte and return

	//	PC-relative modes
fetchEACommon111_pc:
	mtctr	r4
	GET_PC(rEA)									// rEA = PC
#if !TARGET_RT_MAC_MACHO
	bge		cr1,illegal							// if reg >= 4, this is illegal
#else
	bge		cr1,illegal1						// handle illegal cases (reg >= 4)
#endif
	READ_OPCODE_ARG_EXT(r9)						// r9 = sign-extended word
#if (A68000_CHIP < 68020)
	bne		fetchEACommon111_pcid				// handle indexed PC mode
#else
	bne		fetchEACommon020
#endif

	//	(d16,PC) mode (reg == 2)
fetchEACommon111_pcd:
	add		rEA,rEA,r9							// rEA = PC + d16
	SAVE_PC
	CYCLES(4,4,3)
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	bctr										// read the byte and return

	//	(d8,PC,Xn) mode (reg == 3)
#if (A68000_CHIP < 68020)
fetchEACommon111_pcid:
	rlwinm	r8,r9,22,26,29						// r8 = register offset of Xn
	extsb	r7,r9								// r7 = sign-extended low byte
	andi.	r0,r9,0x0800						// word or long?
	READ_L_REG(r8,r8)							// r8 = Xn
	add		rEA,rEA,r7							// rEA += r7
	bne		fetchEACommon111_pcid_long			// skip if this is a long
	extsh	r8,r8								// sign-extend
fetchEACommon111_pcid_long:
	SAVE_PC
	add		rEA,rEA,r8							// rEA += r8
	CYCLES(6,6,0)
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	bctr										// read the byte and return
#endif
	
	//================================================================================================

	//
	//	this computes the effective address for a source operand in mode 110 (6)
	//	it assumes that rRY contains the "register" field shifted left 2 bits
	//	modifies r0,r7,r8,r9
	//
#if (A68000_CHIP < 68020)
computeEA110RX:
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	b		computeEA110Common

computeEA110:
	READ_L_AREG(rEA,rRY)						// rEA = Ay

	// common code
computeEA110Common:
	READ_OPCODE_ARG(r9)							// r9 = opcode
	SAVE_SR
	rlwinm	r8,r9,22,26,29						// r8 = register offset of Xn
	andi.	r0,r9,0x0800						// word or long?
	extsb	r7,r9								// r7 = sign-extended low byte
	READ_L_REG(r8,r8)							// r8 = Xn
	add		rEA,rEA,r7							// rEA += r7
	bne		computeEA110_long					// skip if this is a long
	extsh	r8,r8								// sign-extend
computeEA110_long:
	SAVE_PC
	add		rEA,rEA,r8							// rEA += r8
	blr
#else
computeEA110RX:
	mflr	r0
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	READ_OPCODE_ARG(r9)							// r9 = opcode
	mtctr	r0
	SAVE_SR
	b		fetchEACommon020

computeEA110:
	mflr	r0
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	READ_OPCODE_ARG(r9)							// r9 = opcode
	mtctr	r0
	SAVE_SR
	b		fetchEACommon020
#endif

	//================================================================================================

	//
	//	this computes the extended effective address for the new 68020 addressing modes
	//

#if (A68000_CHIP >= 68020)
	//	first compute the scale in r6 and the sign-extended index in r8
fetchEACommon020:
	rlwinm	r8,r9,22,26,29						// r8 = register offset of Xn
	mtcrf	0xff,r9								// pop r9 into the CR
	READ_L_REG(r8,r8)							// r8 = Xn
	rlwinm	r6,r9,23,30,31						// r6 = SCALE
	mr		r7,rEA								// r7 = base register
	bt		20,fetchEACommon020_long			// skip if this is a long (if bit 20 is set)
	extsh	r8,r8								// sign-extend r8 as halfword
fetchEACommon020_long:

	//	apply the scale
	slw		r8,r8,r6							// r8 = Xn*SCALE
	bt		23,fetchEACommon020_full			// handle the full case later (if bit 23 is set)

	//	simple case: only a brief extension word is present
	add		rEA,r8,r7							// EA = Ay + Xn.SIZE*SCALE
	extsb	r9,r9								// sign-extend the displacement
	CYCLES(0,0,4)
	add		rEA,rEA,r9							// EA = Ay + Xn.SIZE*SCALE + d8
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	bctr

	//	complex case: a full extension word
fetchEACommon020_full:

	//	compute the base displacement in rEA, or 0 if none
	li		rEA,0								// rEA = 0
	bf		26,fetchEACommon020_bdready			// skip if there's no base displacement (if bit 26 is zero)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended base displacement
	CYCLES(0,0,2)
	bf		27,fetchEACommon020_bdready			// skip if we don't need another word (if bit 27 is zero)
	READ_OPCODE_ARG(r4)							// r4 = low order 16 bits
	rlwinm	rEA,rEA,16,0,15						// shift what we read up high
	CYCLES(0,0,4)
	rlwimi	rEA,r4,0,16,31						// insert the extra bits
fetchEACommon020_bdready:

	//	compute the outer displacement in rRY, or 0 if none
	li		rRY,0								// rRY = 0
	bf		30,fetchEACommon020_odready			// skip if there's no outer displacement (if bit 30 is zero)
	READ_OPCODE_ARG_EXT(rRY)					// rRY = sign-extended base displacement
	CYCLES(0,0,2)
	bf		31,fetchEACommon020_odready			// skip if we don't need another word (if bit 31 is zero)
	READ_OPCODE_ARG(r4)							// r4 = low order 16 bits
	rlwinm	rRY,rRY,16,0,15						// shift what we read up high
	rlwimi	rRY,r4,0,16,31						// insert the extra bits
fetchEACommon020_odready:

	//	add in the contribution from the base register (r7)
	bt		24,fetchEACommon020_bs				// skip this next step if base register is suppressed
	add		rEA,rEA,r7							// rEA = Ay + bd
fetchEACommon020_bs:

	//	add in the contribution from the index register (r8), either pre or post
	bt		25,fetchEACommon020_is				// skip this next step if index register is suppressed
	bt		29,fetchEACommon020_postindex		// select pre or post index
	add		rEA,rEA,r8							// rEA += Xn.SIZE*SCALE
	b		fetchEACommon020_is
fetchEACommon020_postindex:
	add		rRY,rRY,r8							// rRY += Xn.SIZE*SCALE
fetchEACommon020_is:

	//	if we're in no-indirect mode, return with what we have
	andi.	r0,r9,7								// check the I/IS bits for all-zero
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	beqctr										// if all-zero, this is the final address

	//	read the final address and add the outer displacement/index to it
	CYCLES(0,0,5)
	mflr	rTempSave2
	READ_L_AT(rEA)								// r3 = long at (rEA)
	mtlr	rTempSave2
	add		rEA,r3,rRY							// final EA = (rEA) + rRY
	rlwinm	r3,rEA,0,ADDRESS_MASK				// strip the address
	bctr
#endif

#if TARGET_RT_MAC_MACHO
illegal1:
	b		illegal
#endif

	//================================================================================================
	//
	//		ABCD -- Add BCD with carry
	//
	//================================================================================================
	
	//
	// 		ABCD.B -(Ay),-(Ax)
	//
entry static abcd_b_aym_axm
	CYCLES(18,18,14)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	sub		rTempSave,rTempSave,r12				// rEA -= r12 (to keep A7 word aligned!)
	subi	rTempSave,rTempSave,1				// Ay = Ay - 1
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_B_AT(rTempSave)						// r3 = byte(Ay)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rEA,rEA,1							// Ax = Ax + 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_B_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	mr		r4,r3									
	mr		r3,rRX
	LOAD_PC
abcd_b_aym_axm_core:
	GET_X(r7)									// r7 = X
	LOAD_ICOUNT
	rlwinm	r5,r3,0,28,31						// r5 = r3 & 0x0f
	rlwinm	r6,r3,0,24,27						// r6 = r3 & 0xf0
	rlwinm	r8,r4,0,28,31						// r8 = r4 & 0x0f
	rlwinm	r9,r4,0,24,27						// r9 = r4 & 0xf0
	add		r5,r5,r8							// r5 += r8
	add		r6,r6,r9							// r6 += r9
	add		r5,r5,r7							// r5 += X
abcd_b_aym_axm_adjust:
	cmplwi	r5,9								// do we need to adjust the low nibble?
	CLR_C										// clear carry
	ble		abcd_b_aym_axm_adjust_a				// if not, skip
	addi	r6,r6,0x10							// carry to the high nibble
	addi	r5,r5,6								// bring back in range
abcd_b_aym_axm_adjust_a:
	cmplwi	r6,0x90								// do we need to adjust the high nibble?
	rlwinm	r4,r5,0,28,31						// r4 = final low nibble
	ble		abcd_b_aym_axm_adjust_b				// if not, skip
	addi	r6,r6,0x60							// bring back in range
	SET_C										// carry
abcd_b_aym_axm_adjust_b:
	rlwimi	r4,r6,0,24,27						// insert the high nibble
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_X_FROM_C								// X = C
	rlwinm	r7,r7,29,29,29						// make a Z bit in the proper location
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// only clear Z; never set it
	SAVE_SR										// only need to save SR; PC & ICOUNT will not have changed
	WRITE_B_AT(rEA)								// write the new value back to rEA
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	// 		ABCD.B Dy,Dx
	//
entry static abcd_b_dy_dx
	CYCLES(6,6,4)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(r4,rRX)							// r4 = Dx
abcd_b_dy_dx_core:
	GET_X(r7)									// r7 = X
	rlwinm	r5,r3,0,28,31						// r5 = r3 & 0x0f
	rlwinm	r6,r3,0,24,27						// r6 = r3 & 0xf0
	rlwinm	r8,r4,0,28,31						// r8 = r4 & 0x0f
	rlwinm	r9,r4,0,24,27						// r9 = r4 & 0xf0
	add		r5,r5,r8							// r5 += r8
	add		r6,r6,r9							// r6 += r9
	add		r5,r5,r7							// r5 += X
abcd_b_dy_dx_adjust:
	cmplwi	r5,9								// do we need to adjust the low nibble?
	CLR_C										// clear carry
	ble		abcd_b_dy_dx_adjust_a				// if not, skip
	addi	r6,r6,0x10							// carry to the high nibble
	addi	r5,r5,6								// bring back in range
abcd_b_dy_dx_adjust_a:
	cmplwi	r6,0x90								// do we need to adjust the high nibble?
	rlwinm	r4,r5,0,28,31						// r4 = final low nibble
	ble		abcd_b_dy_dx_adjust_b				// if not, skip
	addi	r6,r6,0x60							// bring back in range
	SET_C										// carry
abcd_b_dy_dx_adjust_b:
	rlwimi	r4,r6,0,24,27						// insert the high nibble
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_X_FROM_C								// X = C
	rlwinm	r7,r7,29,29,29						// make a Z bit in the proper location
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// only clear Z; never set it
	WRITE_B_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		ADDA -- Add to address register
	//
	//================================================================================================
	
	//
	//		ADDA.W Dy,Ax
	//
entry static adda_w_dy_ax
	CYCLES(8,8,1)
	READ_W_REG_EXT(r3,rRY)						// r3 = Dy (sign-extended)
	READ_L_AREG(r4,rRX)							// r4 = Ax
	add		r4,r4,r3							// r4 += r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDA.W Ay,Ax
	//
entry static adda_w_ay_ax
	CYCLES(8,8,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	READ_L_AREG(r4,rRX)							// r4 = Ax
	extsh	r3,r3								// r3 = ext(r3)
	add		r4,r4,r3							// r4 += r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDA.W (Ay),Ax
	//
entry static adda_w_ay0_ax
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
adda_w_ea_ax:
	READ_W_AT(rEA)								// r3 = word(rEA)
adda_w_ea_ax_post:
	READ_L_AREG(r4,rRX)							// r4 = Ax
	extsh	r3,r3								// r3 = ext(r3)
	add		r4,r4,r3							// r4 += r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDA.W -(Ay),Ax
	//
entry static adda_w_aym_ax
	CYCLES(14,14,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		adda_w_ea_ax
	
	//================================================================================================

	//
	//		ADDA.W (Ay)+,Ax
	//
entry static adda_w_ayp_ax
	CYCLES(12,12,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		adda_w_ea_ax
	
	//================================================================================================

	//
	//		ADDA.W (Ay,d16),Ax
	//
entry static adda_w_ayd_ax
	CYCLES(16,16,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		adda_w_ea_ax
	
	//================================================================================================

	//
	//		ADDA.W (Ay,Xn,d8),Ax
	//
entry static adda_w_ayid_ax
	bl		computeEA110
	CYCLES(18,18,0)
	b		adda_w_ea_ax
	
	//================================================================================================

	//
	//		ADDA.W (xxx).W,Ax
	//		ADDA.W (xxx).L,Ax
	//		ADDA.W (d16,PC),Ax
	//		ADDA.W (d8,PC,Xn),Ax
	//
entry static adda_w_other_ax
	bl		fetchEAWord111
	CYCLES(12,12,0)
	b		adda_w_ea_ax_post

	//================================================================================================
	//================================================================================================

	//
	//		ADDA.L Dy,Ax
	//
entry static adda_l_dy_ax
	CYCLES(8,8,1)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_AREG(r4,rRX)							// r4 = Ax
	add		r4,r4,r3							// r4 += r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDA.L Ay,Ax
	//
entry static adda_l_ay_ax
	CYCLES(8,8,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	READ_L_AREG(r4,rRX)							// r4 = Ax
	add		r4,r4,r3							// r4 += r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDA.L (Ay),Ax
	//
entry static adda_l_ay0_ax
	CYCLES(14,14,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
adda_l_ea_ax:
	READ_L_AT(rEA)								// r3 = long(rEA)
adda_l_ea_ax_post:
	READ_L_AREG(r4,rRX)							// r4 = Ax
	add		r4,r4,r3							// r4 += r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDA.L -(Ay),Ax
	//
entry static adda_l_aym_ax
	CYCLES(16,16,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		adda_l_ea_ax
	
	//================================================================================================

	//
	//		ADDA.L (Ay)+,Ax
	//
entry static adda_l_ayp_ax
	CYCLES(14,14,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		adda_l_ea_ax
	
	//================================================================================================

	//
	//		ADDA.L (Ay,d16),Ax
	//
entry static adda_l_ayd_ax
	CYCLES(18,18,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		adda_l_ea_ax
	
	//================================================================================================

	//
	//		ADDA.L (Ay,Xn,d8),Ax
	//
entry static adda_l_ayid_ax
	bl		computeEA110
	CYCLES(20,20,0)
	b		adda_l_ea_ax
	
	//================================================================================================

	//
	//		ADDA.L (xxx).W,Ax
	//		ADDA.L (xxx).L,Ax
	//		ADDA.L (d16,PC),Ax
	//		ADDA.L (d8,PC,Xn),Ax
	//
entry static adda_l_other_ax
	bl		fetchEALong111
	CYCLES(16,16,0)
	b		adda_l_ea_ax_post

	//================================================================================================
	//
	//		ADDI -- Add immediate to effective address
	//
	//================================================================================================

	//
	//		ADDI.B #xx,Dy
	//
entry static addi_b_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	READ_B_REG(r3,rRY)							// r3 = Dy
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDI.B #xx,(Ay)
	//
entry static addi_b_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
addi_b_imm_ea:
	READ_B_AT(rEA)								// r3 = byte(rEA)
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDI.B #xx,-(Ay)
	//
entry static addi_b_imm_aym
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		addi_b_imm_ea
	
	//================================================================================================

	//
	//		ADDI.B #xx,(Ay)+
	//
entry static addi_b_imm_ayp
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		addi_b_imm_ea
	
	//================================================================================================

	//
	//		ADDI.B #xx,(Ay,d16)
	//
entry static addi_b_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		addi_b_imm_ea
	
	//================================================================================================

	//
	//		ADDI.B #xx,(Ay,Xn,d8)
	//
entry static addi_b_imm_ayid
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		addi_b_imm_ea
	
	//================================================================================================

	//
	//		ADDI.B #xx,(xxx).W
	//		ADDI.B #xx,(xxx).L
	//
entry static addi_b_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	bne		addi_b_imm_absl						// handle long absolute mode
addi_b_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		addi_b_imm_ea
addi_b_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		addi_b_imm_ea

	//================================================================================================
	//================================================================================================

	//
	//		ADDI.W #xx,Dy
	//
entry static addi_w_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	READ_W_REG(r3,rRY)							// r3 = Dy
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDI.W #xx,(Ay)
	//
entry static addi_w_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
addi_w_imm_ea:
	READ_W_AT(rEA)								// r3 = byte(rEA)
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDI.W #xx,-(Ay)
	//
entry static addi_w_imm_aym
	CYCLES(18,18,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		addi_w_imm_ea
	
	//================================================================================================

	//
	//		ADDI.W #xx,(Ay)+
	//
entry static addi_w_imm_ayp
	CYCLES(16,16,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		addi_w_imm_ea
	
	//================================================================================================

	//
	//		ADDI.W #xx,(Ay,d16)
	//
entry static addi_w_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		addi_w_imm_ea
	
	//================================================================================================

	//
	//		ADDI.W #xx,(Ay,Xn,d8)
	//
entry static addi_w_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		addi_w_imm_ea
	
	//================================================================================================

	//
	//		ADDI.W #xx,(xxx).W
	//		ADDI.W #xx,(xxx).L
	//
entry static addi_w_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	bne		addi_w_imm_absl						// handle long absolute mode
addi_w_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		addi_w_imm_ea
addi_w_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		addi_w_imm_ea

	//================================================================================================
	//================================================================================================

	//
	//		ADDI.L #xx,Dy
	//
entry static addi_l_imm_dy
	CYCLES(16,14,1)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	READ_L_REG(r3,rRY)							// r3 = Dy
	addc	r4,r3,rRX							// r4 = r3 + rRX
	CLR_C										// C = 0
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDI.L #xx,(Ay)
	//
entry static addi_l_imm_ay0
	CYCLES(28,28,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
addi_l_imm_ea:
	READ_L_AT(rEA)								// r3 = byte(rEA)
	addc	r4,r3,rRX							// r4 = r3 + rRX
	CLR_C										// C = 0
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDI.L #xx,-(Ay)
	//
entry static addi_l_imm_aym
	CYCLES(30,30,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		addi_l_imm_ea
	
	//================================================================================================

	//
	//		ADDI.L #xx,(Ay)+
	//
entry static addi_l_imm_ayp
	CYCLES(28,28,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		addi_l_imm_ea
	
	//================================================================================================

	//
	//		ADDI.L #xx,(Ay,d16)
	//
entry static addi_l_imm_ayd
	CYCLES(32,32,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		addi_l_imm_ea
	
	//================================================================================================

	//
	//		ADDI.L #xx,(Ay,Xn,d8)
	//
entry static addi_l_imm_ayid
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(34,34,4)
	b		addi_l_imm_ea
	
	//================================================================================================

	//
	//		ADDI.L #xx,(xxx).W
	//		ADDI.L #xx,(xxx).L
	//
entry static addi_l_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	bne		addi_l_imm_absl						// handle long absolute mode
addi_l_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(32,32,7)
	b		addi_l_imm_ea
addi_l_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(36,36,7)
	b		addi_l_imm_ea

	//================================================================================================
	//
	//		ADDQ -- Add 3-bit immediate to effective address
	//
	//================================================================================================

	//
	//		ADDQ.B #xx,Dy
	//
entry static addq_b_ix_dy
	CYCLES(4,4,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDQ.B #xx,(Ay)
	//
entry static addq_b_ix_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
addq_b_ix_ea:
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	READ_B_AT(rEA)								// r3 = byte(rEA)
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDQ.B #xx,-(Ay)
	//
entry static addq_b_ix_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		addq_b_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.B #xx,(Ay)+
	//
entry static addq_b_ix_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		addq_b_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.B #xx,(Ay,d16)
	//
entry static addq_b_ix_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		addq_b_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.B #xx,(Ay,Xn,d8)
	//
entry static addq_b_ix_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		addq_b_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.B #xx,(xxx).W
	//		ADDQ.B #xx,(xxx).L
	//
entry static addq_b_ix_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		addq_b_ix_absl						// handle long absolute mode
addq_b_ix_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		addq_b_ix_ea
addq_b_ix_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		addq_b_ix_ea

	//================================================================================================
	//================================================================================================

	//
	//		ADDQ.W #xx,Dy
	//
entry static addq_w_ix_dy
	CYCLES(4,4,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDQ.W #xx,Ay
	//
entry static addq_w_ix_ay
	CYCLES(4,4,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	add		r4,r3,rRX							// r4 = r3 + rRX
	WRITE_L_AREG(r4,rRY)						// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDQ.W #xx,(Ay)
	//
entry static addq_w_ix_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
addq_w_ix_ea:
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	READ_W_AT(rEA)								// r3 = byte(rEA)
	add		r4,r3,rRX							// r4 = r3 + rRX
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDQ.W #xx,-(Ay)
	//
entry static addq_w_ix_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		addq_w_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.W #xx,(Ay)+
	//
entry static addq_w_ix_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		addq_w_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.W #xx,(Ay,d16)
	//
entry static addq_w_ix_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		addq_w_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.W #xx,(Ay,Xn,d8)
	//
entry static addq_w_ix_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		addq_w_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.W #xx,(xxx).W
	//		ADDQ.W #xx,(xxx).L
	//
entry static addq_w_ix_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		addq_w_ix_absl						// handle long absolute mode
addq_w_ix_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		addq_w_ix_ea
addq_w_ix_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		addq_w_ix_ea

	//================================================================================================
	//================================================================================================

	//
	//		ADDQ.L #xx,Dy
	//
entry static addq_l_ix_dy
	CYCLES(8,8,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	addc	r4,r3,rRX							// r4 = r3 + rRX
	CLR_C										// C = 0
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDQ.L #xx,Ay
	//
entry static addq_l_ix_ay
	CYCLES(8,8,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	add		r4,r3,rRX							// r4 = r3 + rRX
	WRITE_L_AREG(r4,rRY)						// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADDQ.L #xx,(Ay)
	//
entry static addq_l_ix_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
addq_l_ix_ea:
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	READ_L_AT(rEA)								// r3 = byte(rEA)
	addc	r4,r3,rRX							// r4 = r3 + rRX
	CLR_C										// C = 0
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADDQ.L #xx,-(Ay)
	//
entry static addq_l_ix_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		addq_l_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.L #xx,(Ay)+
	//
entry static addq_l_ix_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		addq_l_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.L #xx,(Ay,d16)
	//
entry static addq_l_ix_ayd
	CYCLES(24,24,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		addq_l_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.L #xx,(Ay,Xn,d8)
	//
entry static addq_l_ix_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		addq_l_ix_ea
	
	//================================================================================================

	//
	//		ADDQ.L (xxx).W,Ax
	//		ADDQ.L (xxx).L,Ax
	//
entry static addq_l_ix_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		addq_l_ix_absl						// handle long absolute mode
addq_l_ix_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		addq_l_ix_ea
addq_l_ix_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		addq_l_ix_ea

	//================================================================================================
	//
	//		ADDX -- Add extended
	//
	//================================================================================================

	//
	//		ADDX.B -(Ay),-(Ax)
	//
entry static addx_b_aym_axm
	CYCLES(18,18,10)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rTempSave,rTempSave,1				// Ay = Ay - 1
	sub		rTempSave,rTempSave,r12				// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_B_AT(rTempSave)						// r3 = byte(Ay)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rEA,rEA,1							// Ax = Ax + 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_B_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
addx_b_aym_axm_core:
	GET_X(r7)									// r7 = X
	LOAD_ICOUNT
	add		r7,r7,rRX							// r7 = X + rRX
	add		r4,r3,r7							// r4 = r3 + rRX + X
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	// 		ADDX.B Dy,Dx
	//
entry static addx_b_dy_dx
	CYCLES(4,4,2)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRY,rRX)							// rRY = Dx
addx_b_dy_dx_core:
	GET_X(r7)									// r7 = X
	add		r7,r7,rRY							// r7 = X + rRY
	add		r4,r3,r7							// r4 = r3 + rRX + X
	eqv		r5,r3,rRY							// r5 = ~(r3 ^ rRY)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRY							// r6 = r4 ^ rRY
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	WRITE_B_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ADDX.W -(Ay),-(Ax)
	//
entry static addx_w_aym_axm
	CYCLES(18,18,10)
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	subi	rTempSave,rTempSave,2				// Ay = Ay - 2
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_W_AT(rTempSave)						// r3 = byte(Ay)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	subi	rEA,rEA,2							// Ax = Ax + 2
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_W_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
addx_w_aym_axm_core:
	GET_X(r7)									// r7 = X
	LOAD_ICOUNT
	add		r7,r7,rRX							// r7 = X + rRX
	add		r4,r3,r7							// r4 = r3 + rRX + X
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	// 		ADDX.W Dy,Dx
	//
entry static addx_w_dy_dx
	CYCLES(4,4,2)
	READ_W_REG(r3,rRY)							// r3 = Dy
	READ_W_REG(rRY,rRX)							// rRY = Dx
addx_w_dy_dx_core:
	GET_X(r7)									// r7 = X
	add		r7,r7,rRY							// r7 = X + rRY
	add		r4,r3,r7							// r4 = r3 + rRX + X
	eqv		r5,r3,rRY							// r5 = ~(r3 ^ rRY)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRY							// r6 = r4 ^ rRY
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	WRITE_W_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ADDX.L -(Ay),-(Ax)
	//
entry static addx_l_aym_axm
	CYCLES(30,30,10)
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	subi	rTempSave,rTempSave,4				// Ay = Ay - 4
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_L_AT(rTempSave)						// r3 = byte(Ay)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	subi	rEA,rEA,4							// Ax = Ax + 4
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_L_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
addx_l_aym_axm_core:
	GET_X(r7)									// r7 = X
	CLR_C										// C = 0
	addc	r4,r3,rRX							// r4 = r3 + rRX
	LOAD_ICOUNT
	SET_C_LONG(r4)								// set carry
	addc	r4,r4,r7							// r4 = r3 + rRX + X
	eqv		r5,r3,rRX							// r5 = ~(r3 ^ rRX)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRX							// r6 = r4 ^ rRX
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	// 		ADDX.L Dy,Dx
	//
entry static addx_l_dy_dx
	CYCLES(8,6,2)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_REG(rRY,rRX)							// rRY = Dx
addx_l_dy_dx_core:
	GET_X(r7)									// r7 = X
	CLR_C										// C = 0
	addc	r4,r3,rRY							// r4 = r3 + rRY
	SET_C_LONG(r4)								// set carry
	addc	r4,r4,r7							// r4 = r3 + rRY + X
	eqv		r5,r3,rRY							// r5 = ~(r3 ^ rRY)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRY							// r6 = r4 ^ rRY
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	WRITE_L_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		ADD -- Add two values
	//
	//================================================================================================

	//
	//		ADD.B Dy,Dx
	//
entry static add_b_dy_dx
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRY,rRX)							// rRY = Dx
	add		r4,rRY,r3							// r4 = rRY + r3
	eqv		r5,rRY,r3							// r5 = ~(rRY ^ r3)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,r3							// r6 = r4 ^ r3
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADD.B (Ay),Dx
	//
entry static add_b_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
add_b_ea_dx:
	READ_B_AT(rEA)								// r3 = byte(rEA)
add_b_ea_dx_post:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	add		r4,rRY,r3							// r4 = rRY + r3
	eqv		r5,rRY,r3							// r5 = ~(rRY ^ r3)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,r3							// r6 = r4 ^ r3
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		ADD.B -(Ay),Dx
	//
entry static add_b_aym_dx
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		add_b_ea_dx
	
	//================================================================================================

	//
	//		ADD.B (Ay)+,Dx
	//
entry static add_b_ayp_dx
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		add_b_ea_dx
	
	//================================================================================================

	//
	//		ADD.B (Ay,d16),Dx
	//
entry static add_b_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		add_b_ea_dx
	
	//================================================================================================

	//
	//		ADD.B (Ay,Xn,d8),Dx
	//
entry static add_b_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		add_b_ea_dx
	
	//================================================================================================

	//
	//		ADD.B (xxx).W,Dx
	//		ADD.B (xxx).L,Dx
	//		ADD.B (d16,PC),Dx
	//		ADD.B (d8,PC,Xn),Dx
	//
entry static add_b_other_dx
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		add_b_ea_dx_post

	//================================================================================================

	//
	//		ADD.B Dx,(Ay)
	//
entry static add_b_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
add_b_dx_ea:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	READ_B_AT(rEA)								// r3 = byte(rEA)
	add		r4,r3,rRY							// r4 = r3 + rRY
	eqv		r5,r3,rRY							// r5 = ~(r3 ^ rRY)
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRY							// r6 = r4 ^ rRY
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (rRY ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADD.B Dx,-(Ay)
	//
entry static add_b_dx_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		add_b_dx_ea
	
	//================================================================================================

	//
	//		ADD.B Dx,(Ay)+
	//
entry static add_b_dx_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		add_b_dx_ea
	
	//================================================================================================

	//
	//		ADD.B Dx,(Ay,d16)
	//
entry static add_b_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		add_b_dx_ea
	
	//================================================================================================

	//
	//		ADD.B Dx,(Ay,Xn,d8)
	//
entry static add_b_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		add_b_dx_ea
	
	//================================================================================================

	//
	//		ADD.B Dx,(xxx).W
	//		ADD.B Dx,(xxx).L
	//
entry static add_b_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		add_b_dx_absl						// handle long absolute mode
add_b_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		add_b_dx_ea
add_b_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		add_b_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		ADD.W Ry,Dx
	//
entry static add_w_ry_dx
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	READ_W_REG(r3,rRY)							// r3 = Ry
	READ_W_REG(rRY,rRX)							// rRY = Dx
	add		r4,rRY,r3							// r4 = rRY + r3
	eqv		r5,rRY,r3							// r5 = ~(rRY ^ r3)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,r3							// r6 = r4 ^ r3
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADD.W (Ay),Dx
	//
entry static add_w_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
add_w_ea_dx:
	READ_W_AT(rEA)								// r3 = byte(rEA)
add_w_ea_dx_post:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	add		r4,rRY,r3							// r4 = rRY + r3
	eqv		r5,rRY,r3							// r5 = ~(rRY ^ r3)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,r3							// r6 = r4 ^ r3
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		ADD.W -(Ay),Dx
	//
entry static add_w_aym_dx
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		add_w_ea_dx
	
	//================================================================================================

	//
	//		ADD.W (Ay)+,Dx
	//
entry static add_w_ayp_dx
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		add_w_ea_dx
	
	//================================================================================================

	//
	//		ADD.W (Ay,d16),Dx
	//
entry static add_w_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		add_w_ea_dx
	
	//================================================================================================

	//
	//		ADD.W (Ay,Xn,d8),Dx
	//
entry static add_w_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		add_w_ea_dx

	//================================================================================================

	//
	//		ADD.W (xxx).W,Dx
	//		ADD.W (xxx).L,Dx
	//		ADD.W (d16,PC),Dx
	//		ADD.W (d8,PC,Xn),Dx
	//
entry static add_w_other_dx
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		add_w_ea_dx_post

	//================================================================================================

	//
	//		ADD.W Dx,(Ay)
	//
entry static add_w_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
add_w_dx_ea:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	READ_W_AT(rEA)								// r3 = byte(rEA)
	add		r4,r3,rRY							// r4 = r3 + rRY
	eqv		r5,r3,rRY							// r5 = ~(r3 ^ rRY)
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRY							// r6 = r4 ^ rRY
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (rRY ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADD.W Dx,-(Ay)
	//
entry static add_w_dx_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		add_w_dx_ea
	
	//================================================================================================

	//
	//		ADD.W Dx,(Ay)+
	//
entry static add_w_dx_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		add_w_dx_ea
	
	//================================================================================================

	//
	//		ADD.W Dx,(Ay,d16)
	//
entry static add_w_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		add_w_dx_ea
	
	//================================================================================================

	//
	//		ADD.W Dx,(Ay,Xn,d8)
	//
entry static add_w_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		add_w_dx_ea
	
	//================================================================================================

	//
	//		ADD.W Dx,(xxx).W
	//		ADD.W Dx,(xxx).L
	//
entry static add_w_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		add_w_dx_absl						// handle long absolute mode
add_w_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		add_w_dx_ea
add_w_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		add_w_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		ADD.L Ry,Dx
	//
entry static add_l_ry_dx
	CYCLES(8,8,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_REG(rRY,rRX)							// rRY = Dx
	addc	r4,rRY,r3							// r4 = rRY + r3
	CLR_C										// C = 0
	eqv		r5,rRY,r3							// r5 = ~(rRY ^ r3)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,r3							// r6 = r4 ^ r3
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	SET_V_LONG(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ADD.L (Ay),Dx
	//
entry static add_l_ay0_dx
	CYCLES(14,14,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
add_l_ea_dx:
	READ_L_AT(rEA)								// r3 = byte(rEA)
add_l_ea_dx_post:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	addc	r4,rRY,r3							// r4 = rRY + r3
	CLR_C										// C = 0
	eqv		r5,rRY,r3							// r5 = ~(rRY ^ r3)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,r3							// r6 = r4 ^ r3
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	SET_V_LONG(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		ADD.L -(Ay),Dx
	//
entry static add_l_aym_dx
	CYCLES(16,16,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		add_l_ea_dx
	
	//================================================================================================

	//
	//		ADD.L (Ay)+,Dx
	//
entry static add_l_ayp_dx
	CYCLES(14,14,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		add_l_ea_dx
	
	//================================================================================================

	//
	//		ADD.L (Ay,d16),Dx
	//
entry static add_l_ayd_dx
	CYCLES(18,18,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		add_l_ea_dx
	
	//================================================================================================

	//
	//		ADD.L (Ay,Xn,d8),Dx
	//
entry static add_l_ayid_dx
	bl		computeEA110
	CYCLES(20,20,0)
	b		add_l_ea_dx
	
	//================================================================================================

	//
	//		ADD.L (xxx).W,Dx
	//		ADD.L (xxx).L,Dx
	//		ADD.L (d16,PC),Dx
	//		ADD.L (d8,PC,Xn),Dx
	//
entry static add_l_other_dx
	bl		fetchEALong111
	CYCLES(16,16,0)
	b		add_l_ea_dx_post

	//================================================================================================

	//
	//		ADD.L Dx,(Ay)
	//
entry static add_l_dx_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
add_l_dx_ea:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	READ_L_AT(rEA)								// r3 = byte(rEA)
	addc	r4,r3,rRY							// r4 = r3 + rRY
	CLR_C										// C = 0
	eqv		r5,r3,rRY							// r5 = ~(r3 ^ rRY)
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r4,rRY							// r6 = r4 ^ rRY
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (rRY ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ADD.L Dx,-(Ay)
	//
entry static add_l_dx_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		add_l_dx_ea
	
	//================================================================================================

	//
	//		ADD.L Dx,(Ay)+
	//
entry static add_l_dx_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		add_l_dx_ea
	
	//================================================================================================

	//
	//		ADD.L Dx,(Ay,d16)
	//
entry static add_l_dx_ayd
	CYCLES(24,24,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		add_l_dx_ea
	
	//================================================================================================

	//
	//		ADD.L Dx,(Ay,Xn,d8)
	//
entry static add_l_dx_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		add_l_dx_ea
	
	//================================================================================================

	//
	//		ADD.L Dx,(xxx).W
	//		ADD.L Dx,(xxx).L
	//
entry static add_l_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		add_l_dx_absl						// handle long absolute mode
add_l_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		add_l_dx_ea
add_l_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		add_l_dx_ea

	//================================================================================================
	//
	//		ANDI -- And immediate with effective address
	//
	//================================================================================================

	//
	//		ANDI.B #xx,Dy
	//
entry static andi_b_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	READ_B_REG(r3,rRY)							// r3 = Dy
	and		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ANDI.B #xx,(Ay)
	//
entry static andi_b_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
andi_b_imm_ea:
	READ_B_AT(rEA)								// r3 = byte(rEA)
	and		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ANDI.B #xx,-(Ay)
	//
entry static andi_b_imm_aym
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		andi_b_imm_ea
	
	//================================================================================================

	//
	//		ANDI.B #xx,(Ay)+
	//
entry static andi_b_imm_ayp
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		andi_b_imm_ea
	
	//================================================================================================

	//
	//		ANDI.B #xx,(Ay,d16)
	//
entry static andi_b_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		andi_b_imm_ea
	
	//================================================================================================

	//
	//		ANDI.B #xx,(Ay,Xn,d8)
	//
entry static andi_b_imm_ayid
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		andi_b_imm_ea
	
	//================================================================================================

	//
	//		ANDI.B (xxx).W,Ax
	//		ANDI.B (xxx).L,Ax
	//
entry static andi_b_imm_other
	cmpwi	cr1,rRY,4<<2						// is this the CCR case?
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	beq		cr1,andi_b_imm_ccr					// handle CCR case
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		andi_b_imm_absl						// handle long absolute mode
andi_b_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		andi_b_imm_ea
andi_b_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		andi_b_imm_ea
andi_b_imm_ccr:
	and		r3,rSRFlags,rRX						// r3 = rSRFlags & rRX
	rlwimi	rSRFlags,r3,0,24,31					// keep only the low byte
	CYCLES(20,16,9)
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ANDI.W #xx,Dy
	//
entry static andi_w_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	READ_W_REG(r3,rRY)							// r3 = Dy
	and		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ANDI.W #xx,(Ay)
	//
entry static andi_w_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
andi_w_imm_ea:
	READ_W_AT(rEA)								// r3 = byte(rEA)
	and		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ANDI.W #xx,-(Ay)
	//
entry static andi_w_imm_aym
	CYCLES(18,18,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		andi_w_imm_ea
	
	//================================================================================================

	//
	//		ANDI.W #xx,(Ay)+
	//
entry static andi_w_imm_ayp
	CYCLES(16,16,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		andi_w_imm_ea
	
	//================================================================================================

	//
	//		ANDI.W #xx,(Ay,d16)
	//
entry static andi_w_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		andi_w_imm_ea
	
	//================================================================================================

	//
	//		ANDI.W #xx,(Ay,Xn,d8)
	//
entry static andi_w_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		andi_w_imm_ea
	
	//================================================================================================

	//
	//		ANDI.W (xxx).W,Ax
	//		ANDI.W (xxx).L,Ax
	//
entry static andi_w_imm_other
	cmpwi	cr1,rRY,4<<2						// is this the SR case?
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	beq		cr1,andi_w_imm_sr					// handle SR case
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		andi_w_imm_absl						// handle long absolute mode
andi_w_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		andi_w_imm_ea
andi_w_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		andi_w_imm_ea
andi_w_imm_sr:
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, exception time
	and		r3,rSRFlags,rRX						// r3 = rSRFlags & rRX
	CYCLES(20,16,9)
	b		executeLoopEndUpdateSR				// all done

	//================================================================================================
	//================================================================================================

	//
	//		ANDI.L #xx,Dy
	//
entry static andi_l_imm_dy
	CYCLES(14,14,1)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	READ_L_REG(r3,rRY)							// r3 = Dy
	and		r4,r3,rRX							// r4 = r3 & rRX
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ANDI.L #xx,(Ay)
	//
entry static andi_l_imm_ay0
	CYCLES(28,28,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
andi_l_imm_ea:
	READ_L_AT(rEA)								// r3 = byte(rEA)
	and		r4,r3,rRX							// r4 = r3 & rRX
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ANDI.L #xx,-(Ay)
	//
entry static andi_l_imm_aym
	CYCLES(30,30,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		andi_l_imm_ea
	
	//================================================================================================

	//
	//		ANDI.L #xx,(Ay)+
	//
entry static andi_l_imm_ayp
	CYCLES(28,28,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		andi_l_imm_ea
	
	//================================================================================================

	//
	//		ANDI.L #xx,(Ay,d16)
	//
entry static andi_l_imm_ayd
	CYCLES(32,32,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		andi_l_imm_ea
	
	//================================================================================================

	//
	//		ANDI.L #xx,(Ay,Xn,d8)
	//
entry static andi_l_imm_ayid
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(34,34,4)
	b		andi_l_imm_ea
	
	//================================================================================================

	//
	//		ANDI.L (xxx).W,Ax
	//		ANDI.L (xxx).L,Ax
	//
entry static andi_l_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	bne		andi_l_imm_absl						// handle long absolute mode
andi_l_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(32,32,7)
	b		andi_l_imm_ea
andi_l_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(36,36,7)
	b		andi_l_imm_ea

	//================================================================================================
	//
	//		AND -- And two values
	//
	//================================================================================================

	//
	//		AND.B Dy,Dx
	//
entry static and_b_dy_dx
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRY,rRX)							// rRY = Dx
	and		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		AND.B (Ay),Dx
	//
entry static and_b_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
and_b_ea_dx:
	READ_B_AT(rEA)								// r3 = byte(rEA)
and_b_ea_dx_post:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	and		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		AND.B -(Ay),Dx
	//
entry static and_b_aym_dx
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		and_b_ea_dx
	
	//================================================================================================

	//
	//		AND.B (Ay)+,Dx
	//
entry static and_b_ayp_dx
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		and_b_ea_dx
	
	//================================================================================================

	//
	//		AND.B (Ay,d16),Dx
	//
entry static and_b_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		and_b_ea_dx
	
	//================================================================================================

	//
	//		AND.B (Ay,Xn,d8),Dx
	//
entry static and_b_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		and_b_ea_dx
	
	//================================================================================================

	//
	//		AND.B (xxx).W,Dx
	//		AND.B (xxx).L,Dx
	//		AND.B (d16,PC),Dx
	//		AND.B (d8,PC,Xn),Dx
	//
entry static and_b_other_dx
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		and_b_ea_dx_post

	//================================================================================================

	//
	//		AND.B Dx,(Ay)
	//
entry static and_b_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
and_b_dx_ea:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	READ_B_AT(rEA)								// r3 = byte(rEA)
	and		r4,r3,rRY							// r4 = r3 + rRY
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		AND.B Dx,-(Ay)
	//
entry static and_b_dx_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		and_b_dx_ea
	
	//================================================================================================

	//
	//		AND.B Dx,(Ay)+
	//
entry static and_b_dx_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		and_b_dx_ea
	
	//================================================================================================

	//
	//		AND.B Dx,(Ay,d16)
	//
entry static and_b_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		and_b_dx_ea
	
	//================================================================================================

	//
	//		AND.B Dx,(Ay,Xn,d8)
	//
entry static and_b_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		and_b_dx_ea
	
	//================================================================================================

	//
	//		AND.B Dx,(xxx).W
	//		AND.B Dx,(xxx).L
	//
entry static and_b_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		and_b_dx_absl						// handle long absolute mode
and_b_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		and_b_dx_ea
and_b_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		and_b_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		AND.W Dy,Dx
	//
entry static and_w_dy_dx
	CYCLES(4,4,1)
	READ_W_REG(r3,rRY)							// r3 = Ry
	READ_W_REG(rRY,rRX)							// rRY = Dx
	and		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		AND.W (Ay),Dx
	//
entry static and_w_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
and_w_ea_dx:
	READ_W_AT(rEA)								// r3 = byte(rEA)
and_w_ea_dx_post:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	and		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		AND.W -(Ay),Dx
	//
entry static and_w_aym_dx
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		and_w_ea_dx
	
	//================================================================================================

	//
	//		AND.W (Ay)+,Dx
	//
entry static and_w_ayp_dx
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		and_w_ea_dx
	
	//================================================================================================

	//
	//		AND.W (Ay,d16),Dx
	//
entry static and_w_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		and_w_ea_dx
	
	//================================================================================================

	//
	//		AND.W (Ay,Xn,d8),Dx
	//
entry static and_w_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		and_w_ea_dx

	//================================================================================================

	//
	//		AND.W (xxx).W,Dx
	//		AND.W (xxx).L,Dx
	//		AND.W (d16,PC),Dx
	//		AND.W (d8,PC,Xn),Dx
	//
entry static and_w_other_dx
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		and_w_ea_dx_post

	//================================================================================================

	//
	//		AND.W Dx,(Ay)
	//
entry static and_w_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
and_w_dx_ea:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	READ_W_AT(rEA)								// r3 = byte(rEA)
	and		r4,r3,rRY							// r4 = r3 + rRY
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		AND.W Dx,-(Ay)
	//
entry static and_w_dx_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		and_w_dx_ea
	
	//================================================================================================

	//
	//		AND.W Dx,(Ay)+
	//
entry static and_w_dx_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		and_w_dx_ea
	
	//================================================================================================

	//
	//		AND.W Dx,(Ay,d16)
	//
entry static and_w_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		and_w_dx_ea
	
	//================================================================================================

	//
	//		AND.W Dx,(Ay,Xn,d8)
	//
entry static and_w_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		and_w_dx_ea
	
	//================================================================================================

	//
	//		AND.W Dx,(xxx).W
	//		AND.W Dx,(xxx).L
	//
entry static and_w_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		and_w_dx_absl						// handle long absolute mode
and_w_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		and_w_dx_ea
and_w_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		and_w_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		AND.L Dy,Dx
	//
entry static and_l_dy_dx
	CYCLES(8,8,1)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_REG(rRY,rRX)							// rRY = Dx
	and		r4,rRY,r3							// r4 = rRY + r3
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		AND.L (Ay),Dx
	//
entry static and_l_ay0_dx
	CYCLES(14,14,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
and_l_ea_dx:
	READ_L_AT(rEA)								// r3 = byte(rEA)
and_l_ea_dx_post:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	and		r4,rRY,r3							// r4 = rRY + r3
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		AND.L -(Ay),Dx
	//
entry static and_l_aym_dx
	CYCLES(16,16,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		and_l_ea_dx
	
	//================================================================================================

	//
	//		AND.L (Ay)+,Dx
	//
entry static and_l_ayp_dx
	CYCLES(14,14,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		and_l_ea_dx
	
	//================================================================================================

	//
	//		AND.L (Ay,d16),Dx
	//
entry static and_l_ayd_dx
	CYCLES(18,18,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		and_l_ea_dx
	
	//================================================================================================

	//
	//		AND.L (Ay,Xn,d8),Dx
	//
entry static and_l_ayid_dx
	bl		computeEA110
	CYCLES(20,20,0)
	b		and_l_ea_dx

	//================================================================================================

	//
	//		AND.L (xxx).W,Dx
	//		AND.L (xxx).L,Dx
	//		AND.L (d16,PC),Dx
	//		AND.L (d8,PC,Xn),Dx
	//
entry static and_l_other_dx
	bl		fetchEALong111
	CYCLES(16,16,0)
	b		and_l_ea_dx_post

	//================================================================================================

	//
	//		AND.L Dx,(Ay)
	//
entry static and_l_dx_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
and_l_dx_ea:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	READ_L_AT(rEA)								// r3 = byte(rEA)
	and		r4,r3,rRY							// r4 = r3 + rRY
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		AND.L Dx,-(Ay)
	//
entry static and_l_dx_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		and_l_dx_ea
	
	//================================================================================================

	//
	//		AND.L Dx,(Ay)+
	//
entry static and_l_dx_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		and_l_dx_ea
	
	//================================================================================================

	//
	//		AND.L Dx,(Ay,d16)
	//
entry static and_l_dx_ayd
	CYCLES(24,24,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		and_l_dx_ea
	
	//================================================================================================

	//
	//		AND.L Dx,(Ay,Xn,d8)
	//
entry static and_l_dx_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		and_l_dx_ea

	//================================================================================================

	//
	//		AND.L Dx,(xxx).W
	//		AND.L Dx,(xxx).L
	//
entry static and_l_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		and_l_dx_absl						// handle long absolute mode
and_l_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		and_l_dx_ea
and_l_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		and_l_dx_ea

	//================================================================================================
	//
	//		ASL -- Arithmetic shift left (same as logical)
	//
	//================================================================================================

	//
	//		ASL.B Dx,Dy
	//
entry static asl_b_dx_dy
	CYCLES(6,6,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	add		r0,rRX,rRX
	li		r8,0x7f								// r8 = 0x7f
	add		rCycleCount,rCycleCount,r0
	slw		r4,r3,rRX							// r4 = r3 << rRX
	srw		r8,r8,rRX							// r8 = 0x7f >> rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	xori	r8,r8,0xff							// r8 = (0x7f >> rRX) ^ 0xff
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	and		r9,r3,r8							// r9 = r3 & r8
	cntlzw	r7,r5								// r7 = number of zeros in r5
	xor		r8,r9,r8							// r8 = (r3 & r8) ^ r8
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	cntlzw	r8,r8								// r8 = number of zeros
	cntlzw	r9,r9								// r9 = number of zeros
	WRITE_B_REG(r5,rRY)							// write the final register value
	nor		r8,r8,r9							// r8 = ~(r8 | r9)
	SET_Z(r7)									// Z = (r5 == 0)
	rlwimi	rSRFlags,r8,28,30,30				// V = the result
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASL.B Ix,Dy
	//
entry static asl_b_ix_dy
	CYCLES(6,6,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	li		r8,0x7f								// r8 = 0x7f
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	srw		r8,r8,rRX							// r8 = 0x7f >> rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	xori	r8,r8,0xff							// r8 = (0x7f >> rRX) ^ 0xff
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	and		r9,r3,r8							// r9 = r3 & r8
	cntlzw	r7,r5								// r7 = number of zeros in r5
	xor		r8,r9,r8							// r8 = (r3 & r8) ^ r8
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	cntlzw	r8,r8								// r8 = number of zeros
	WRITE_B_REG(r5,rRY)							// write the final register value
	cntlzw	r9,r9								// r9 = number of zeros
	SET_Z(r7)									// Z = (r5 == 0)
	nor		r8,r8,r9							// r8 = ~(r8 | r9)
	SET_X_FROM_C								// X = C
	rlwimi	rSRFlags,r8,28,30,30				// V = the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ASL.W Dx,Dy
	//
entry static asl_w_dx_dy
	CYCLES(6,6,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	li		r8,0x7fff							// r8 = 0x7f
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	srw		r8,r8,rRX							// r8 = 0x7f >> rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	xori	r8,r8,0xffff						// r8 = (0x7f >> rRX) ^ 0xff
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	and		r9,r3,r8							// r9 = r3 & r8
	cntlzw	r7,r5								// r7 = number of zeros in r5
	xor		r8,r9,r8							// r8 = (r3 & r8) ^ r8
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	cntlzw	r8,r8								// r8 = number of zeros
	cntlzw	r9,r9								// r9 = number of zeros
	WRITE_W_REG(r5,rRY)							// write the final register value
	nor		r8,r8,r9							// r8 = ~(r8 | r9)
	SET_Z(r7)									// Z = (r5 == 0)
	rlwimi	rSRFlags,r8,28,30,30				// V = the result
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASL.W Ix,Dy
	//
entry static asl_w_ix_dy
	CYCLES(6,6,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	li		r8,0x7fff							// r8 = 0x7f
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	srw		r8,r8,rRX							// r8 = 0x7f >> rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	xori	r8,r8,0xffff						// r8 = (0x7f >> rRX) ^ 0xff
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	and		r9,r3,r8							// r9 = r3 & r8
	cntlzw	r7,r5								// r7 = number of zeros in r5
	xor		r8,r9,r8							// r8 = (r3 & r8) ^ r8
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	cntlzw	r8,r8								// r8 = number of zeros
	WRITE_W_REG(r5,rRY)							// write the final register value
	cntlzw	r9,r9								// r9 = number of zeros
	SET_Z(r7)									// Z = (r5 == 0)
	nor		r8,r8,r9							// r8 = ~(r8 | r9)
	SET_X_FROM_C								// X = C
	rlwimi	rSRFlags,r8,28,30,30				// V = the result
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASL.W #1,(Ay)
	//
entry static asl_w_ay0
	CYCLES(12,12,9)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
asl_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,1,16,30						// r4 = r3 << 1
	rlwimi	rSRFlags,r3,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	xor		r8,r3,r4							// r8 = r3 ^ r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SET_X_FROM_C								// X = C
	rlwimi	rSRFlags,r8,18,30,30				// V = (r3 ^ r4) & 0x8000
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ASL.W #1,-(Ay)
	//
entry static asl_w_aym
	CYCLES(14,14,9)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		asl_w_ea
	
	//================================================================================================

	//
	//		ASL.W #1,(Ay)+
	//
entry static asl_w_ayp
	CYCLES(12,12,10)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		asl_w_ea
	
	//================================================================================================

	//
	//		ASL.W #1,(Ay,d16)
	//
entry static asl_w_ayd
	CYCLES(16,16,9)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		asl_w_ea
	
	//================================================================================================

	//
	//		ASL.W #1,(Ay,Xn,d8)
	//
entry static asl_w_ayid
	bl		computeEA110
	CYCLES(18,18,6)
	b		asl_w_ea

	//================================================================================================

	//
	//		ASL.W #1,(xxx).W
	//		ASL.W #1,(xxx).L
	//
entry static asl_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		asl_w_absl							// handle long absolute mode
asl_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,9)
	b		asl_w_ea
asl_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,9)
	b		asl_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		ASL.L Dx,Dy
	//
entry static asl_l_dx_dy
	CYCLES(8,8,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	add		r0,rRX,rRX
	subi	rRX,rRX,1							// RX -= 1
	add		rCycleCount,rCycleCount,r0
	lis		r8,0x4000							// r8 = 0x4000000
	beq		asl_l_dx_dy_0						// special case for zero
	subi	r8,r8,1								// r8 = 0x3ffffff
	slw		r4,r3,rRX							// r4 = r3 << rRX
	srw		r8,r8,rRX							// r8 = 0x3f >> rRX
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	not		r8,r8								// r8 = (0x3f >> rRX) ^ 0xff
	rlwinm	r4,r4,1,0,30						// shift the last bit out
	and		r9,r3,r8							// r9 = r3 & r8
	cntlzw	r7,r4								// r7 = number of zeros in r5
	xor		r8,r9,r8							// r8 = (r3 & r8) ^ r8
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	cntlzw	r8,r8								// r8 = number of zeros
	WRITE_L_REG(r4,rRY)							// write the final register value
	cntlzw	r9,r9								// r9 = number of zeros
	SET_Z(r7)									// Z = (r5 == 0)
	nor		r8,r8,r9							// r8 = ~(r8 | r9)
	SET_X_FROM_C								// X = C
	rlwimi	rSRFlags,r8,28,30,30				// V = the result
	b		executeLoopEnd						// all done
asl_l_dx_dy_0:
	CLR_VC										// C = V = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASL.L Ix,Dy
	//
entry static asl_l_ix_dy
	CYCLES(10,10,5)
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	subi	rRX,rRX,1							// rRX -= 1
	READ_L_REG(r3,rRY)							// r3 = Dy
	lis		r8,0x4000							// r8 = 0x4000000
	andi.	rRX,rRX,7							// rRX = (rRX - 1) & 7
	subi	r8,r8,1								// r8 = 0x3ffffff
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	srw		r8,r8,rRX							// r8 = 0x3f >> rRX
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	not		r8,r8								// r8 = (0x3f >> rRX) ^ 0xff
	rlwinm	r4,r4,1,0,30						// shift the last bit out
	and		r9,r3,r8							// r9 = r3 & r8
	cntlzw	r7,r4								// r7 = number of zeros in r5
	xor		r8,r9,r8							// r8 = (r3 & r8) ^ r8
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	cntlzw	r8,r8								// r8 = number of zeros
	WRITE_L_REG(r4,rRY)							// write the final register value
	cntlzw	r9,r9								// r9 = number of zeros
	SET_Z(r7)									// Z = (r5 == 0)
	nor		r8,r8,r9							// r8 = ~(r8 | r9)
	SET_X_FROM_C								// X = C
	rlwimi	rSRFlags,r8,28,30,30				// V = the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		ASR -- Arithmetic shift right
	//
	//================================================================================================

	//
	//		ASR.B Dx,Dy
	//
entry static asr_b_dx_dy
	CYCLES(6,6,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwinm	r3,r3,24,0,7						// rotate the value high
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	sraw	r4,r3,rRX							// r4 = r3 >> rRX
	rlwinm	r5,r4,8,24,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,9,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASR.B Ix,Dy
	//
entry static asr_b_ix_dy
	CYCLES(6,6,3)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwinm	r3,r3,24,0,7						// rotate the value high
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	sraw	r4,r3,rRX							// r4 = r3 >> rRX
	rlwinm	r5,r4,8,24,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,9,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ASR.W Dx,Dy
	//
entry static asr_w_dx_dy
	CYCLES(6,6,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwinm	r3,r3,16,0,15						// rotate the value high
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	sraw	r4,r3,rRX							// r4 = r3 >> rRX
	rlwinm	r5,r4,16,16,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASR.W Ix,Dy
	//
entry static asr_w_ix_dy
	CYCLES(6,6,3)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwinm	r3,r3,16,0,15						// rotate the value high
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	sraw	r4,r3,rRX							// r4 = r3 >> rRX
	rlwinm	r5,r4,16,16,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASR.W #1,(Ay)
	//
entry static asr_w_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
asr_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,31,17,31						// r4 = r3 >> 1
	CLR_V										// V = 0
	rlwimi	r4,r4,1,16,16						// copy the high bit
	rlwimi	rSRFlags,r3,0,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SET_X_FROM_C								// X = C
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ASR.W #1,-(Ay)
	//
entry static asr_w_aym
	CYCLES(14,14,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		asr_w_ea
	
	//================================================================================================

	//
	//		ASR.W #1,(Ay)+
	//
entry static asr_w_ayp
	CYCLES(12,12,9)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		asr_w_ea
	
	//================================================================================================

	//
	//		ASR.W #1,(Ay,d16)
	//
entry static asr_w_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		asr_w_ea
	
	//================================================================================================

	//
	//		ASR.W #1,(Ay,Xn,d8)
	//
entry static asr_w_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		asr_w_ea

	//================================================================================================

	//
	//		ASR.W #1,(xxx).W
	//		ASR.W #1,(xxx).L
	//
entry static asr_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		asr_w_absl							// handle long absolute mode
asr_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		asr_w_ea
asr_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		asr_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		ASR.L Dx,Dy
	//
entry static asr_l_dx_dy
	CYCLES(8,8,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	add		r0,rRX,rRX
	subi	rRX,rRX,1							// RX -= 1
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	beq		asr_l_dx_dy_0						// special case for zero
	sraw	r4,r3,rRX							// r4 = r3 << rRX
	rlwimi	rSRFlags,r4,0,31,31					// C = copy of the last bit shifted out
	srawi	r4,r4,1								// shift the last bit out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
asr_l_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ASR.L Ix,Dy
	//
entry static asr_l_ix_dy
	CYCLES(10,10,3)
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	subi	rRX,rRX,1							// rRX -= 1
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,7							// rRX = (rRX - 1) & 7
	CLR_V										// V = 0
	add		r0,rRX,rRX
	sraw	r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	rlwimi	rSRFlags,r4,0,31,31					// C = copy of the last bit shifted out
	srawi	r4,r4,1								// shift the last bit out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		BCC -- Branch conditional
	//
	//================================================================================================

	//
	//		cc == HI == ~C & ~Z = ~(C | Z)
	//
entry static bcc_hi
	rlwinm	r3,rSRFlags,30,31,31				// r3 = Z shifted to match C position
	nor		r3,r3,rSRFlags						// r3 = ~(r3 | C)
	extsb	rEA,r11								// rEA = sign-extended opcode
	rlwinm.	r3,r3,0,31,31						// r3 = ~(C | Z) & 1

bcc_common:
	cmpwi	cr1,rEA,0							// cr1 = (rEA == 0)
#if (A68000_CHIP >= 68020)
	cmpwi	cr2,rEA,-1							// cr2 = (rEA == -1)
#endif
	bne		bcc_doit							// if condition is true, branch
	CYCLES(8,6,1)
	beq		cr1,bcc_dont_doit_w					// if rEA == 0, we need to read an extra displacement
#if (A68000_CHIP >= 68020)
	beq		cr2,bcc_dont_doit_l					// if rEA == 0xff, we need to read an extra displacement
#endif
	b		executeLoopEnd						// all done

bcc_doit:
	CYCLES(10,10,2)
	beq		cr1,bcc_doit_w						// if rEA == 0, we need to read an extra displacement
#if (A68000_CHIP >= 68020)
	beq		cr2,bcc_doit_l						// if rEA == 0xff, we need to read an extra displacement
#endif
	add		rPC,rPC,rEA							// rPC += rEA
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

bcc_doit_w:
	rlwinm	r3,rPC,0,ADDRESS_MASK				// r3 = address
	lhax	rEA,rOpcodeROM,r3					// rEA = sign-extended displacement
	add		rPC,rPC,rEA							// rPC += rEA
	UPDATE_BANK
	b		executeLoopEnd						// all done
bcc_dont_doit_w:
	addi	rPC,rPC,2							// else skip the 16-bit displacement
	CYCLES(4,4,2)
	b		executeLoopEnd						// all done

#if (A68000_CHIP >= 68020)
bcc_doit_l:
	rlwinm	r3,rPC,0,ADDRESS_MASK				// r3 = address
	lwzx	rEA,rOpcodeROM,r3					// rEA = 32-bit displacement
	add		rPC,rPC,rEA							// rPC += rEA
	UPDATE_BANK
	b		executeLoopEnd						// all done
bcc_dont_doit_l:
	addi	rPC,rPC,4							// else skip the 32-bit displacement
	CYCLES(0,0,2)
	b		executeLoopEnd						// all done
#endif

	//================================================================================================

	//
	//		cc == LS == C | Z
	//
entry static bcc_ls
	rlwinm	r3,rSRFlags,30,31,31				// r3 = Z shifted to match C position
	extsb	rEA,r11								// rEA = sign-extended opcode
	or		r3,r3,rSRFlags						// r3 = (r3 | C)
	rlwinm.	r3,r3,0,31,31						// r3 = (C | Z) & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == CC = ~C
	//
entry static bcc_cc
	extsb	rEA,r11								// rEA = sign-extended opcode
	not		r3,rSRFlags							// r3 = ~C
	rlwinm.	r0,r3,0,31,31						// r0 = ~C & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == CS = C
	//
entry static bcc_cs
	extsb	rEA,r11								// rEA = sign-extended opcode
	rlwinm.	r0,rSRFlags,0,31,31					// r0 = C & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == NE = ~Z
	//
entry static bcc_ne
	extsb	rEA,r11								// rEA = sign-extended opcode
	not		r3,rSRFlags							// r3 = ~Z
	rlwinm.	r0,r3,0,29,29						// r0 = ~Z & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == EQ = Z
	//
entry static bcc_eq
	extsb	rEA,r11								// rEA = sign-extended opcode
	rlwinm.	r0,rSRFlags,0,29,29					// r0 = Z & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == VC = ~V
	//
entry static bcc_vc
	extsb	rEA,r11								// rEA = sign-extended opcode
	not		r3,rSRFlags							// r3 = ~V
	rlwinm.	r0,r3,0,30,30						// r0 = ~V & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == VS = V
	//
entry static bcc_vs
	extsb	rEA,r11								// rEA = sign-extended opcode
	rlwinm.	r0,rSRFlags,0,30,30					// r0 = V & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == PL = ~N
	//
entry static bcc_pl
	extsb	rEA,r11								// rEA = sign-extended opcode
	not		r3,rSRFlags							// r3 = ~N
	rlwinm.	r0,r3,0,28,28						// r0 = ~N & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == MI = N
	//
entry static bcc_mi
	extsb	rEA,r11								// rEA = sign-extended opcode
	rlwinm.	r0,rSRFlags,0,28,28					// r0 = N & 1
	b		bcc_common

	//================================================================================================

	//
	//		cc == GE = ~(N ^ V)
	//
entry static bcc_ge
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	extsb	rEA,r11								// rEA = sign-extended opcode
	eqv		r3,r3,rSRFlags						// r3 = ~(N ^ V)
	rlwinm.	r0,r3,0,30,30						// r0 = ~(N ^ V) & 2
	b		bcc_common

	//================================================================================================

	//
	//		cc == LT = (N ^ V)
	//
entry static bcc_lt
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	extsb	rEA,r11								// rEA = sign-extended opcode
	xor		r3,r3,rSRFlags						// r3 = (N ^ V)
	rlwinm.	r0,r3,0,30,30						// r0 = (N ^ V) & 2
	b		bcc_common

	//================================================================================================

	//
	//		cc == GE = ~(N ^ V) & ~Z = ~((N ^ V) | Z)
	//
entry static bcc_gt
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	rlwinm	r4,rSRFlags,31,30,30				// r4 = Z shifted to match V position
	extsb	rEA,r11								// rEA = sign-extended opcode
	xor		r3,r3,rSRFlags						// r3 = (N ^ V)
	nor		r3,r3,r4							// r3 = ~((N ^ V) | Z)
	rlwinm.	r0,r3,0,30,30						// r0 = ~((N ^ V) | Z) & 2
	b		bcc_common

	//================================================================================================

	//
	//		cc == LE = (N ^ V) | Z
	//
entry static bcc_le
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	rlwinm	r4,rSRFlags,31,30,30				// r4 = Z shifted to match V position
	extsb	rEA,r11								// rEA = sign-extended opcode
	xor		r3,r3,rSRFlags						// r3 = (N ^ V)
	or		r3,r3,r4							// r4 = ((N ^ V) | Z)
	rlwinm.	r0,r3,0,30,30						// r0 = ((N ^ V) | Z) & 2
	b		bcc_common

	//================================================================================================
	//
	//		BCHG -- Bit test and invert
	//
	//================================================================================================

	//
	//		BCHG.L Dx,Dy
	//
entry static bchg_dx_dy
	CYCLES(8,8,1)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	xor		r4,r3,r5							// r4 = Dy ^ (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	WRITE_L_REG(r4,rRY)							// Dy = r4
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BCHG.B Dx,(Ay)
	//
entry static bchg_dx_ay0
	CYCLES(12,12,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bchg_dx_ea:
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_AT(rEA)								// r3 = (Ay)
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	xor		r4,r3,r5							// r4 = Dy ^ (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		BCHG.B Dx,-(Ay)
	//
entry static bchg_dx_aym
	CYCLES(14,14,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		bchg_dx_ea
	
	//================================================================================================

	//
	//		BCHG.B Dx,(Ay)+
	//
entry static bchg_dx_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		bchg_dx_ea
	
	//================================================================================================

	//
	//		BCHG.B Dx,(Ay,d16)
	//
entry static bchg_dx_ayd
	CYCLES(16,16,7)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bchg_dx_ea
	
	//================================================================================================

	//
	//		BCHG.B Dx,(Ay,Xn,d8)
	//
entry static bchg_dx_ayid
	bl		computeEA110
	CYCLES(18,18,4)
	b		bchg_dx_ea

	//================================================================================================

	//
	//		BCHG.B Dx,(xxx).W
	//		BCHG.B Dx,(xxx).L
	//
entry static bchg_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bchg_dx_absl						// handle long absolute mode
bchg_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,7)
	b		bchg_dx_ea
bchg_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,7)
	b		bchg_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		BCHG.L #xx,Dy
	//
entry static bchg_imm_dy
	CYCLES(12,12,1)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	xor		r4,r3,r5							// r4 = Dy ^ (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	WRITE_L_REG(r4,rRY)							// Dy = r4
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BCHG.B #xx,(Ay)
	//
entry static bchg_imm_ay0
	CYCLES(16,16,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_OPCODE_ARG(rRX)						// rRX = Ix
bchg_imm_ea:
	READ_B_AT(rEA)								// r3 = (Ay)
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	xor		r4,r3,r5							// r4 = Dy ^ (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		BCHG.B #xx,-(Ay)
	//
entry static bchg_imm_aym
	CYCLES(18,18,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		bchg_imm_ea
	
	//================================================================================================

	//
	//		BCHG.B #xx,(Ay)+
	//
entry static bchg_imm_ayp
	CYCLES(16,16,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		bchg_imm_ea
	
	//================================================================================================

	//
	//		BCHG.B #xx,(Ay,d16)
	//
entry static bchg_imm_ayd
	CYCLES(20,20,7)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bchg_imm_ea
	
	//================================================================================================

	//
	//		BCHG.B #xx,(Ay,Xn,d8)
	//
entry static bchg_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bl		computeEA110
	CYCLES(22,22,4)
	b		bchg_imm_ea

	//================================================================================================

	//
	//		BCHG.B #xx,(xxx).W
	//		BCHG.B #xx,(xxx).L
	//
entry static bchg_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bne		bchg_imm_absl						// handle long absolute mode
bchg_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,7)
	b		bchg_imm_ea
bchg_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,7)
	b		bchg_imm_ea

	//================================================================================================
	//
	//		BCLR -- Bit test and clear
	//
	//================================================================================================

	//
	//		BCLR.L Dx,Dy
	//
entry static bclr_dx_dy
	CYCLES(10,10,1)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	andc	r4,r3,r5							// r4 = Dy & ~(1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	WRITE_L_REG(r4,rRY)							// Dy = r4
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BCLR.B Dx,(Ay)
	//
entry static bclr_dx_ay0
	CYCLES(12,14,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bclr_dx_ea:
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_AT(rEA)								// r3 = (Ay)
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	andc	r4,r3,r5							// r4 = Dy & ~(1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		BCLR.B Dx,-(Ay)
	//
entry static bclr_dx_aym
	CYCLES(14,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		bclr_dx_ea
	
	//================================================================================================

	//
	//		BCLR.B Dx,(Ay)+
	//
entry static bclr_dx_ayp
	CYCLES(12,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		bclr_dx_ea
	
	//================================================================================================

	//
	//		BCLR.B Dx,(Ay,d16)
	//
entry static bclr_dx_ayd
	CYCLES(16,18,7)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bclr_dx_ea
	
	//================================================================================================

	//
	//		BCLR.B Dx,(Ay,Xn,d8)
	//
entry static bclr_dx_ayid
	bl		computeEA110
	CYCLES(18,20,4)
	b		bclr_dx_ea

	//================================================================================================

	//
	//		BCLR.B Dx,(xxx).W
	//		BCLR.B Dx,(xxx).L
	//
entry static bclr_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bclr_dx_absl						// handle long absolute mode
bclr_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,18,7)
	b		bclr_dx_ea
bclr_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,22,7)
	b		bclr_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		BCLR.L #xx,Dy
	//
entry static bclr_imm_dy
	CYCLES(14,14,1)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	andc	r4,r3,r5							// r4 = Dy & ~(1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	WRITE_L_REG(r4,rRY)							// Dy = r4
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BCLR.B #xx,(Ay)
	//
entry static bclr_imm_ay0
	CYCLES(16,16,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_OPCODE_ARG(rRX)						// rRX = Ix
bclr_imm_ea:
	READ_B_AT(rEA)								// r3 = (Ay)
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	andc	r4,r3,r5							// r4 = Dy & ~(1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		BCLR.B #xx,-(Ay)
	//
entry static bclr_imm_aym
	CYCLES(18,18,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		bclr_imm_ea
	
	//================================================================================================

	//
	//		BCLR.B #xx,(Ay)+
	//
entry static bclr_imm_ayp
	CYCLES(16,16,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		bclr_imm_ea
	
	//================================================================================================

	//
	//		BCLR.B #xx,(Ay,d16)
	//
entry static bclr_imm_ayd
	CYCLES(20,20,7)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bclr_imm_ea
	
	//================================================================================================

	//
	//		BCLR.B #xx,(Ay,Xn,d8)
	//
entry static bclr_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bl		computeEA110
	CYCLES(22,22,4)
	b		bclr_imm_ea

	//================================================================================================

	//
	//		BCLR.B #xx,(xxx).W
	//		BCLR.B #xx,(xxx).L
	//
entry static bclr_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bne		bclr_imm_absl						// handle long absolute mode
bclr_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,7)
	b		bclr_imm_ea
bclr_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,7)
	b		bclr_imm_ea


	//================================================================================================
	//
	//		BFCHG -- Bit field test and complement
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFCHG Dy{o:w}
	//
entry static bfchg_dy
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,9)
	bl		ExtractBitfieldInfo
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	SET_N_LONG(r4)								// N = upper bit of bitfield
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	xor		r4,r3,r9							// r4 = r3 ^ bitmask
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFCHG (Ay){o:w}
	//
entry static bfchg_ay0
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,18)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfchg_ea:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	CLR_VC										// V = C = 0
	bgt		cr7,bfchg_multiword					// handle multiword case
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	SET_N_LONG(r4)								// N = upper bit of bitfield
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	xor		r4,r3,r9							// r4 = r3 ^ bitmask
	SAVE_SR
	WRITE_L_AT(rEA)
	b		executeLoopEndRestore

bfchg_multiword:
	rfi

	//================================================================================================

	//
	//		BFCHG (Ay,d16){o:w}
	//
entry static bfchg_ayd
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,18)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfchg_ea
	
	//================================================================================================

	//
	//		BFCHG (Ay,Xn,d8){o:w}
	//
entry static bfchg_ayid
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,16)
	b		bfchg_ea
	
	//================================================================================================

	//
	//		BFCHG (xxx).W{o:w}
	//		BFCHG (xxx).L{o:w}
	//
entry static bfchg_other
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bfchg_absl						// handle long absolute mode
bfchg_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(0,0,18)
	b		bfchg_ea
bfchg_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(0,0,17)
	b		bfchg_ea
#endif


	//================================================================================================
	//
	//		BFCLR -- Bit field test and clear
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFCLR Dy{o:w}
	//
entry static bfclr_dy
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,9)
	bl		ExtractBitfieldInfo
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	SET_N_LONG(r4)								// N = upper bit of bitfield
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	andc	r4,r3,r9							// r4 = r3 & ~bitmask
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFCLR (Ay){o:w}
	//
entry static bfclr_ay0
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,18)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfclr_ea:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	CLR_VC										// V = C = 0
	bgt		cr7,bfclr_multiword					// handle multiword case
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	SET_N_LONG(r4)								// N = upper bit of bitfield
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	andc	r4,r3,r9							// r4 = r3 & ~bitmask
	SAVE_SR
	WRITE_L_AT(rEA)
	b		executeLoopEndRestore

bfclr_multiword:
	rfi

	//================================================================================================

	//
	//		BFCLR (Ay,d16){o:w}
	//
entry static bfclr_ayd
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,18)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfclr_ea
	
	//================================================================================================

	//
	//		BFCLR (Ay,Xn,d8){o:w}
	//
entry static bfclr_ayid
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,16)
	b		bfclr_ea
	
	//================================================================================================

	//
	//		BFCLR (xxx).W{o:w}
	//		BFCLR (xxx).L{o:w}
	//
entry static bfclr_other
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bfclr_absl						// handle long absolute mode
bfclr_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(0,0,18)
	b		bfclr_ea
bfclr_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(0,0,17)
	b		bfclr_ea
#endif


	//================================================================================================
	//
	//		BFEXTS -- Bit field extract signed
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFEXTS Dy{o:w},Dx
	//
entry static bfexts_dy_dx
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,5)
	bl		ExtractBitfieldInfo
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	slw		r4,r3,rTempSave2
	SET_N_LONG(r4)								// N = upper bit of bitfield
	sraw	r4,r4,r10							// r4 = bitfield shifted into place
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	SET_Z(r7)									// set the Z bit
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFEXTS (Ay){o:w},Dx
	//
entry static bfexts_ay0_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,15)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfexts_ea_dx:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	CLR_VC										// V = C = 0
	bgt		cr7,bfexts_multiword				// handle multiword case
	slw		r4,r3,rTempSave2
	SET_N_LONG(r4)								// N = upper bit of bitfield
	sraw	r4,r4,r10							// r4 = bitfield shifted into place
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	SET_Z(r7)									// set the Z bit
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd

bfexts_multiword:
	rfi

	//================================================================================================

	//
	//		BFEXTS (Ay,d16){o:w},Dx
	//
entry static bfexts_ayd_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,15)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfexts_ea_dx
	
	//================================================================================================

	//
	//		BFEXTS (Ay,Xn,d8){o:w},Dx
	//
entry static bfexts_ayid_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,13)
	b		bfexts_ea_dx
	
	//================================================================================================

	//
	//		BFEXTS (xxx).W{o:w},Dx
	//		BFEXTS (xxx).L{o:w},Dx
	//		BFEXTS (PC,d16){o:w},Dx
	//		BFEXTS (PC,Xn,d8){o:w},Dx
	//
entry static bfexts_other_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA111
	CYCLES(0,0,13)
	b		bfexts_ea_dx
#endif


	//================================================================================================
	//
	//		BFEXTU -- Bit field extract unsigned
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFEXTU Dy{o:w},Dx
	//
entry static bfextu_dy_dx
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,5)
	bl		ExtractBitfieldInfo
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	slw		r4,r3,rTempSave2
	SET_N_LONG(r4)								// N = upper bit of bitfield
	srw		r4,r4,r10							// r4 = bitfield shifted into place
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	SET_Z(r7)									// set the Z bit
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFEXTU (Ay){o:w},Dx
	//
entry static bfextu_ay0_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,15)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfextu_ea_dx:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	CLR_VC										// V = C = 0
	bgt		cr7,bfextu_multiword				// handle multiword case
	slw		r4,r3,rTempSave2
	SET_N_LONG(r4)								// N = upper bit of bitfield
	srw		r4,r4,r10							// r4 = bitfield shifted into place
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	SET_Z(r7)									// set the Z bit
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd

bfextu_multiword:
	rfi

	//================================================================================================

	//
	//		BFEXTU (Ay,d16){o:w},Dx
	//
entry static bfextu_ayd_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,15)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfextu_ea_dx
	
	//================================================================================================

	//
	//		BFEXTU (Ay,Xn,d8){o:w},Dx
	//
entry static bfextu_ayid_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,13)
	b		bfextu_ea_dx
	
	//================================================================================================

	//
	//		BFEXTU (xxx).W{o:w},Dx
	//		BFEXTU (xxx).L{o:w},Dx
	//		BFEXTU (PC,d16){o:w},Dx
	//		BFEXTU (PC,Xn,d8){o:w},Dx
	//
entry static bfextu_other_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA111
	CYCLES(0,0,13)
	b		bfextu_ea_dx
#endif


	//================================================================================================
	//
	//		BFFFO -- Bit field extract unsigned
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFFFO Dy{o:w},Dx
	//
entry static bfffo_dy_dx
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,15)
	bl		ExtractBitfieldInfo
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	slw		r4,r3,rTempSave2
	SET_N_LONG(r4)								// N = upper bit of bitfield
	srw		r5,r4,r10							// r4 = bitfield shifted into place
	cntlzw	r4,r4
	cntlzw	r7,r5								// r7 = number of zeros in bitfield
	add		r4,r4,rTempSave2
	SET_Z(r7)									// set the Z bit
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFFFO (Ay){o:w},Dx
	//
entry static bfffo_ay0_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,26)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfffo_ea_dx:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	CLR_VC										// V = C = 0
	bgt		cr7,bfffo_multiword					// handle multiword case
	slw		r4,r3,rTempSave2
	SET_N_LONG(r4)								// N = upper bit of bitfield
	srw		r5,r4,r10							// r4 = bitfield shifted into place
	cntlzw	r4,r4
	cntlzw	r7,r5								// r7 = number of zeros in bitfield
	add		r4,r4,rTempSave2
	SET_Z(r7)									// set the Z bit
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd

bfffo_multiword:
	rfi

	//================================================================================================

	//
	//		BFFFO (Ay,d16){o:w},Dx
	//
entry static bfffo_ayd_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,26)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfffo_ea_dx
	
	//================================================================================================

	//
	//		BFFFO (Ay,Xn,d8){o:w},Dx
	//
entry static bfffo_ayid_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,24)
	b		bfffo_ea_dx
	
	//================================================================================================

	//
	//		BFFFO (xxx).W{o:w},Dx
	//		BFFFO (xxx).L{o:w},Dx
	//		BFFFO (PC,d16){o:w},Dx
	//		BFFFO (PC,Xn,d8){o:w},Dx
	//
entry static bfffo_other_dx
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA111
	CYCLES(0,0,23)
	b		bfffo_ea_dx
#endif


	//================================================================================================
	//
	//		BFINS -- Bit field insert
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	// 		Extract bitfield info
	//			rRX 		= register
	//			rTempSave 	= width
	//			rTempSave2 	= offset
ExtractBitfieldInfo:
	mtcrf	0xff,rRX
	rlwinm.	rTempSave,rRX,0,27,31
	bf		26,WidthIsConstant
	rlwinm	rTempSave,rRX,2,27,29
	READ_B_REG(rTempSave,rTempSave)
	rlwinm.	rTempSave,rTempSave,0,27,31
WidthIsConstant:
	bne		WidthIsNonZero
	li		rTempSave,32
WidthIsNonZero:
	rlwinm	rTempSave2,rRX,26,27,31
	bf		20,OffsetIsConstant
	rlwinm	rTempSave2,rRX,28,27,29
	READ_L_REG(rTempSave2,rTempSave2)
OffsetIsConstant:
	rlwinm	rRX,rRX,22,27,29
	blr

	//
	//		BFINS Dx,Dy{o:w}
	//
entry static bfins_dx_dy
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,7)
	bl		ExtractBitfieldInfo
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	READ_L_REG(rRX,rRX)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	slw		rRX,rRX,r10
	slw		r9,r9,r10
	SET_N_LONG(rRX)								// N = upper bit of bitfield
	cntlzw	r7,rRX								// r7 = number of zeros in bitfield
	srw		rRX,rRX,rTempSave2					// rRX = bitfield shifted into place
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	andc	r4,r3,r9							// r4 = r3 & ~r9
	or		r4,r4,rRX							// r4 = (r3 & ~r9) | rRX
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFINS Dx,(Ay){o:w}
	//
entry static bfins_dx_ay0
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,16)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfins_dx_ea:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	READ_L_REG(rRX,rRX)
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	CLR_VC										// V = C = 0
	bgt		cr7,bfins_multiword					// handle multiword case
	slw		rRX,rRX,r10
	slw		r9,r9,r10
	SET_N_LONG(rRX)								// N = upper bit of bitfield
	cntlzw	r7,rRX								// r7 = number of zeros in bitfield
	srw		rRX,rRX,rTempSave2					// rRX = bitfield shifted into place
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	andc	r4,r3,r9							// r4 = r3 & ~r9
	or		r4,r4,rRX							// r4 = (r3 & ~r9) | rRX
	SAVE_SR
	WRITE_L_AT(rEA)
	b		executeLoopEndRestore

bfins_multiword:
	rfi

	//================================================================================================

	//
	//		BFINS Dx,(Ay,d16){o:w}
	//
entry static bfins_dx_ayd
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,16)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfins_dx_ea
	
	//================================================================================================

	//
	//		BFINS Dx,(Ay,Xn,d8){o:w}
	//
entry static bfins_dx_ayid
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,14)
	b		bfins_dx_ea
	
	//================================================================================================

	//
	//		BFINS Dx,(xxx).W{o:w}
	//		BFINS Dx,(xxx).L{o:w}
	//
entry static bfins_dx_other
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bfins_dx_absl						// handle long absolute mode
bfins_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(0,0,16)
	b		bfins_dx_ea
bfins_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(0,0,15)
	b		bfins_dx_ea
#endif


	//================================================================================================
	//
	//		BFSET -- Bit field test and set
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFSET Dy{o:w}
	//
entry static bfset_dy
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,9)
	bl		ExtractBitfieldInfo
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	SET_N_LONG(r4)								// N = upper bit of bitfield
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	or		r4,r3,r9							// r4 = r3 | bitmask
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFSET (Ay){o:w}
	//
entry static bfset_ay0
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,18)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bfset_ea:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	CLR_VC										// V = C = 0
	bgt		cr7,bfset_multiword					// handle multiword case
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	SET_N_LONG(r4)								// N = upper bit of bitfield
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	srw		r9,r9,rTempSave2					// r9 = mask for bitfield
	SET_Z(r7)									// set the Z bit
	or		r4,r3,r9							// r4 = r3 | bitmask
	SAVE_SR
	WRITE_L_AT(rEA)
	b		executeLoopEndRestore

bfset_multiword:
	rfi

	//================================================================================================

	//
	//		BFSET (Ay,d16){o:w}
	//
entry static bfset_ayd
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,18)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bfset_ea
	
	//================================================================================================

	//
	//		BFSET (Ay,Xn,d8){o:w}
	//
entry static bfset_ayid
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,16)
	b		bfset_ea
	
	//================================================================================================

	//
	//		BFSET (xxx).W{o:w}
	//		BFSET (xxx).L{o:w}
	//
entry static bfset_other
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bfset_absl						// handle long absolute mode
bfset_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(0,0,18)
	b		bfset_ea
bfset_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(0,0,17)
	b		bfset_ea
#endif


	//================================================================================================
	//
	//		BFTST -- Bit field test
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		BFTST Dy{o:w}
	//
entry static bftst_dy
	READ_OPCODE_ARG(rRX)
	CYCLES(0,0,3)
	bl		ExtractBitfieldInfo
	READ_L_REG(r3,rRY)
	CLR_VC										// V = C = 0
	rlwinm	rTempSave2,rTempSave2,0,27,31		// keep the offset 0-31 (register case only)
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	SET_N_LONG(r4)								// N = upper bit of bitfield
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		BFTST (Ay){o:w}
	//
entry static bftst_ay0
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,13)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bftst_ea:
//	rlwinm	r4,rTempSave2,0,0,28				// r4 = number of bytes in the offset
// KM bugfix
	rlwinm	r4,rTempSave2,29,3,31				// r4 = number of bytes in the offset
	rlwinm	rTempSave2,rTempSave2,0,29,31		// offset is now just the bit offset
	add		rEA,rEA,r4							// adjust the EA for the byte offset
	add		rRY,rTempSave2,rTempSave			// rRY = offset + width
	READ_L_AT(rEA)								// fetch the data to be modified
	cmpwi	cr7,rRY,32							// above 32?
	subfic	r10,rTempSave,32					// r10 = 32 - width
	li		r9,-1								// r9 = -1
	CLR_VC										// V = C = 0
	bgt		cr7,bftst_multiword					// handle multiword case
	slw		r4,r3,rTempSave2
	slw		r9,r9,r10
	and		r4,r4,r9
	cntlzw	r7,r4								// r7 = number of zeros in bitfield
	SET_N_LONG(r4)								// N = upper bit of bitfield
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd

bftst_multiword:
	rfi

	//================================================================================================

	//
	//		BFTST (Ay,d16){o:w}
	//
entry static bftst_ayd
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	CYCLES(0,0,13)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bftst_ea
	
	//================================================================================================

	//
	//		BFTST (Ay,Xn,d8){o:w}
	//
entry static bftst_ayid
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA110
	CYCLES(0,0,11)
	b		bftst_ea
	
	//================================================================================================

	//
	//		BFTST (xxx).W{o:w}
	//		BFTST (xxx).L{o:w}
	//		BFTST (PC,d16){o:w}
	//		BFTST (PC,Xn,d8){o:w}
	//
entry static bftst_other
	READ_OPCODE_ARG(rRX)
	bl		ExtractBitfieldInfo
	bl		computeEA111
	CYCLES(0,0,11)
	b		bftst_ea
#endif


	//================================================================================================
	//
	//		BKPT -- Breakpoint
	//
	//================================================================================================

entry static bkpt
	b		executeLoopEnd
	

	//================================================================================================
	//
	//		BRA -- Branch
	//
	//================================================================================================

entry static bra
	extsb.	r3,r11								// r3 = sign-extended offset
#if (A68000_CHIP >= 68020)
	cmpwi	cr2,r3,-1							// cr2 = (r3 == -1)
#endif
	CYCLES(10,10,3)
	beq		bra_w								// if r3 == 0, this is a 16-bit offset
#if (A68000_CHIP >= 68020)
	beq		cr2,bra_l							// if rEA == 0xff, we need to read an extra displacement
#endif
	add		rPC,rPC,r3							// rPC += r3
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

bra_w:
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = address
	lhax	r3,rOpcodeROM,rRX					// r3 = sign-extended 16-bit displacement
	add		rPC,rPC,r3							// rPC += r3
	UPDATE_BANK
	b		executeLoopEnd						// all done

#if (A68000_CHIP >= 68020)
bra_l:
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = address
	lwzx	r3,rOpcodeROM,rRX					// r3 = 32-bit displacement
	add		rPC,rPC,r3							// rPC += r3
	UPDATE_BANK
	b		executeLoopEnd						// all done
#endif


	//================================================================================================
	//
	//		BSET -- Bit test and clear
	//
	//================================================================================================

	//
	//		BSET.L Dx,Dy
	//
entry static bset_dx_dy
	CYCLES(8,8,1)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	or		r4,r3,r5							// r4 = Dy | (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	WRITE_L_REG(r4,rRY)							// Dy = r4
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BSET.B Dx,(Ay)
	//
entry static bset_dx_ay0
	CYCLES(12,12,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
bset_dx_ea:
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_AT(rEA)								// r3 = (Ay)
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	or		r4,r3,r5							// r4 = Dy | (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		BSET.B Dx,-(Ay)
	//
entry static bset_dx_aym
	CYCLES(14,14,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		bset_dx_ea
	
	//================================================================================================

	//
	//		BSET.B Dx,(Ay)+
	//
entry static bset_dx_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		bset_dx_ea
	
	//================================================================================================

	//
	//		BSET.B Dx,(Ay,d16)
	//
entry static bset_dx_ayd
	CYCLES(16,16,7)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bset_dx_ea
	
	//================================================================================================

	//
	//		BSET.B Dx,(Ay,Xn,d8)
	//
entry static bset_dx_ayid
	bl		computeEA110
	CYCLES(18,18,4)
	b		bset_dx_ea

	//================================================================================================

	//
	//		BSET.B Dx,(xxx).W
	//		BSET.B Dx,(xxx).L
	//
entry static bset_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		bset_dx_absl						// handle long absolute mode
bset_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,7)
	b		bset_dx_ea
bset_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,7)
	b		bset_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		BSET.L #xx,Dy
	//
entry static bset_imm_dy
	CYCLES(12,12,1)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	or		r4,r3,r5							// r4 = Dy | (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	WRITE_L_REG(r4,rRY)							// Dy = r4
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BSET.B #xx,(Ay)
	//
entry static bset_imm_ay0
	CYCLES(16,16,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_OPCODE_ARG(rRX)						// rRX = Ix
bset_imm_ea:
	READ_B_AT(rEA)								// r3 = (Ay)
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	or		r4,r3,r5							// r4 = Dy | (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		BSET.B #xx,-(Ay)
	//
entry static bset_imm_aym
	CYCLES(18,18,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		bset_imm_ea
	
	//================================================================================================

	//
	//		BSET.B #xx,(Ay)+
	//
entry static bset_imm_ayp
	CYCLES(16,16,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		bset_imm_ea
	
	//================================================================================================

	//
	//		BSET.B #xx,(Ay,d16)
	//
entry static bset_imm_ayd
	CYCLES(20,20,7)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		bset_imm_ea
	
	//================================================================================================

	//
	//		BSET.B #xx,(Ay,Xn,d8)
	//
entry static bset_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bl		computeEA110
	CYCLES(22,22,4)
	b		bset_imm_ea

	//================================================================================================

	//
	//		BSET.B #xx,(xxx).W
	//		BSET.B #xx,(xxx).L
	//
entry static bset_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bne		bset_imm_absl						// handle long absolute mode
bset_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,7)
	b		bset_imm_ea
bset_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,7)
	b		bset_imm_ea

	//================================================================================================
	//
	//		BSR -- Branch to subroutine
	//
	//================================================================================================

entry static bsr
	CYCLES(18,18,5)
	GET_A7(rEA)									// rEA = A7
	extsb.	r3,r11								// r3 = sign-extended offset
#if (A68000_CHIP >= 68020)
	cmpwi	cr2,r3,-1							// cr2 = (r3 == -1)
#endif
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_SR
	SET_A7(rEA)									// A7 = rEA
	beq		bsr_w								// if r3 == 0, this is a 16-bit offset
#if (A68000_CHIP >= 68020)
	beq		cr2,bsr_l							// if rEA == 0xff, we need to read an extra displacement
#endif
	mr		r4,rPC								// r4 = rPC
	add		rPC,rPC,r3							// rPC += r3
	SAVE_PC
	WRITE_L_AT(rEA)								// 
	UPDATE_BANK_SHORT
	b		executeLoopEndRestore				// return

bsr_w:
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = address
	lhax	r3,rOpcodeROM,rRX					// r3 = sign-extended 16-bit displacement
	addi	r4,rPC,2							// r4 = rPC + 2
	add		rPC,rPC,r3							// rPC += r3
	SAVE_PC
	WRITE_L_AT(rEA)								// 
	UPDATE_BANK
	b		executeLoopEndRestore				// return

#if (A68000_CHIP >= 68020)
bsr_l:
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = address
	lwzx	r3,rOpcodeROM,rRX					// r3 = 32-bit displacement
	addi	r4,rPC,2							// r4 = rPC + 2
	add		rPC,rPC,r3							// rPC += r3
	SAVE_PC
	WRITE_L_AT(rEA)								// 
	UPDATE_BANK
	b		executeLoopEndRestore				// return
#endif

	//================================================================================================
	//
	//		BTST -- Bit test and clear
	//
	//================================================================================================

	//
	//		BTST.L Dx,Dy
	//
entry static btst_dx_dy
	CYCLES(6,6,1)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BTST.B Dx,(Ay)
	//
entry static btst_dx_ay0
	CYCLES(8,8,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
btst_dx_ea:
	READ_B_AT(rEA)								// r3 = (Ay)
btst_dx_ea_post:
	READ_B_REG(rRX,rRX)							// rRX = Dx
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	b		executeLoopEndRestoreNoSR

	//================================================================================================

	//
	//		BTST.B Dx,-(Ay)
	//
entry static btst_dx_aym
	CYCLES(10,10,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		btst_dx_ea
	
	//================================================================================================

	//
	//		BTST.B Dx,(Ay)+
	//
entry static btst_dx_ayp
	CYCLES(8,8,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		btst_dx_ea
	
	//================================================================================================

	//
	//		BTST.B Dx,(Ay,d16)
	//
entry static btst_dx_ayd
	CYCLES(12,12,7)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		btst_dx_ea
	
	//================================================================================================

	//
	//		BTST.B Dx,(Ay,Xn,d8)
	//
entry static btst_dx_ayid
	bl		computeEA110
	CYCLES(14,14,4)
	b		btst_dx_ea

	//================================================================================================

	//
	//		BTST.B Dx,(xxx).W
	//		BTST.B Dx,(xxx).L
	//		BTST.B Dx,(PC,d16)
	//		BTST.B Dx,(PC,Xn,d8)
	//
entry static btst_dx_other
	bl		fetchEAByte111
	CYCLES(8,8,7)
	b		btst_dx_ea_post

	//================================================================================================
	//================================================================================================

	//
	//		BTST.L #xx,Dy
	//
entry static btst_imm_dy
	CYCLES(10,10,1)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	READ_L_REG(r3,rRY)							// r3 = Dy
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,31							// only 31 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		BTST.B #xx,(Ay)
	//
entry static btst_imm_ay0
	CYCLES(12,12,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_OPCODE_ARG(rRX)						// rRX = Ix
btst_imm_ea:
	READ_B_AT(rEA)								// r3 = (Ay)
btst_imm_ea_post:
	li		r5,1								// r5 = 1
	andi.	rRX,rRX,7							// only 7 bits to test
	rlwnm	r5,r5,rRX,0,31						// r5 = 1 << rRX
	and		r6,r3,r5							// r6 = Dy & (1 << rRX)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	SET_Z(r7)									// set the Z bit
	b		executeLoopEndRestoreNoSR

	//================================================================================================

	//
	//		BTST.B #xx,-(Ay)
	//
entry static btst_imm_aym
	CYCLES(14,14,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		btst_imm_ea
	
	//================================================================================================

	//
	//		BTST.B #xx,(Ay)+
	//
entry static btst_imm_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		btst_imm_ea
	
	//================================================================================================

	//
	//		BTST.B #xx,(Ay,d16)
	//
entry static btst_imm_ayd
	CYCLES(16,16,7)
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		btst_imm_ea
	
	//================================================================================================

	//
	//		BTST.B #xx,(Ay,Xn,d8)
	//
entry static btst_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bl		computeEA110
	CYCLES(18,18,4)
	b		btst_imm_ea

	//================================================================================================

	//
	//		BTST.B #xx,(xxx).W
	//		BTST.B #xx,(xxx).L
	//		BTST.B #xx,(PC,d16)
	//		BTST.B #xx,(PC,Xn,d8)
	//
entry static btst_imm_other
	READ_OPCODE_ARG(rRX)						// rRX = Ix
	bl		fetchEAByte111
	CYCLES(12,12,4)
	b		btst_imm_ea_post

	//================================================================================================
	//
	//		CHK -- Check register against bounds
	//
	//================================================================================================

	//
	//		CHK.W Dy,Dx
	//
entry static chk_w_dy_dx
	CYCLES(10,8,8)
	READ_W_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	cmpwi	rRX,0								// compare to zero
	cmpw	cr1,rRX,r3							// compare to the upper bound
	li		r3,6								// exception 6
	blt		chk_w_dy_dx_low						// if less than zero, we're low
	ble		cr1,executeLoopEnd					// if not greater, we're done
	CLR_N										// we're high; set N = 0
	b		generateException					// generate an exception
chk_w_dy_dx_low:
	SET_N										// we're low; set N = 1
	b		generateException					// generate an exception
	
	//================================================================================================

	//
	//		CHK.W (Ay),Dx
	//
entry static chk_w_ay0_dx
	CYCLES(14,14,11)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
chk_w_ea_dx:
	READ_W_AT(rEA)								// r3 = (Ay)
chk_w_ea_dx_post:
	READ_W_REG(rRX,rRX)							// rRX = Dx
	cmpwi	rRX,0								// compare to zero
	cmpw	cr1,rRX,r3							// compare to the upper bound
	li		r3,6								// exception 6
	blt		chk_w_ea_dx_low						// if less than zero, we're low
	ble		cr1,executeLoopEndRestore			// if not greater, we're done
	CLR_N										// we're high; set N = 0
	b		generateException					// generate an exception
chk_w_ea_dx_low:
	SET_N										// we're low; set N = 1
	b		generateException					// generate an exception

	//================================================================================================

	//
	//		CHK.W -(Ay),Dx
	//
entry static chk_w_aym_dx
	CYCLES(16,16,11)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		chk_w_ea_dx
	
	//================================================================================================

	//
	//		CHK.W (Ay)+,Dx
	//
entry static chk_w_ayp_dx
	CYCLES(14,14,12)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		chk_w_ea_dx
	
	//================================================================================================

	//
	//		CHK.W (Ay,d16),Dx
	//
entry static chk_w_ayd_dx
	CYCLES(18,18,11)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		chk_w_ea_dx
	
	//================================================================================================

	//
	//		CHK.W (Ay,Xn,d8),Dx
	//
entry static chk_w_ayid_dx
	bl		computeEA110
	CYCLES(20,20,8)
	b		chk_w_ea_dx

	//================================================================================================

	//
	//		CHK.W (xxx).W,Dx
	//		CHK.W (xxx).L,Dx
	//		CHK.W (PC,d16),Dx
	//		CHK.W (PC,Xn,d8),Dx
	//
entry static chk_w_other_dx
	bl		fetchEAWord111
	CYCLES(14,14,8)
	b		chk_w_ea_dx_post

	//================================================================================================

	//
	//		CHK.L Dy,Dx
	//
#if (A68000_CHIP >= 68020)
entry static chk_l_dy_dx
	CYCLES(10,8,8)
	READ_L_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	cmpwi	rRX,0								// compare to zero
	cmpw	cr1,rRX,r3							// compare to the upper bound
	li		r3,6								// exception 6
	blt		chk_l_dy_dx_low						// if less than zero, we're low
	ble		cr1,executeLoopEnd					// if not greater, we're done
	CLR_N										// we're high; set N = 0
	b		generateException					// generate an exception
chk_l_dy_dx_low:
	SET_N										// we're low; set N = 1
	b		generateException					// generate an exception
	
	//================================================================================================

	//
	//		CHK.L (Ay),Dx
	//
entry static chk_l_ay0_dx
	CYCLES(14,14,11)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
chk_l_ea_dx:
	READ_L_AT(rEA)								// r3 = (Ay)
chk_l_ea_dx_post:
	READ_L_REG(rRX,rRX)							// rRX = Dx
	cmpwi	rRX,0								// compare to zero
	cmpw	cr1,rRX,r3							// compare to the upper bound
	li		r3,6								// exception 6
	blt		chk_l_ea_dx_low						// if less than zero, we're low
	ble		cr1,executeLoopEndRestore			// if not greater, we're done
	CLR_N										// we're high; set N = 0
	b		generateException					// generate an exception
chk_l_ea_dx_low:
	SET_N										// we're low; set N = 1
	b		generateException					// generate an exception

	//================================================================================================

	//
	//		CHK.L -(Ay),Dx
	//
entry static chk_l_aym_dx
	CYCLES(16,16,11)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		chk_l_ea_dx
	
	//================================================================================================

	//
	//		CHK.L (Ay)+,Dx
	//
entry static chk_l_ayp_dx
	CYCLES(14,14,12)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		chk_l_ea_dx
	
	//================================================================================================

	//
	//		CHK.L (Ay,d16),Dx
	//
entry static chk_l_ayd_dx
	CYCLES(18,18,11)
	SAVE_SR
	READ_OPCODE_ARG_LONG(r3)					// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		chk_l_ea_dx
	
	//================================================================================================

	//
	//		CHK.L (Ay,Xn,d8),Dx
	//
entry static chk_l_ayid_dx
	bl		computeEA110
	CYCLES(20,20,8)
	b		chk_l_ea_dx

	//================================================================================================

	//
	//		CHK.L (xxx).W,Dx
	//		CHK.L (xxx).L,Dx
	//		CHK.L (PC,d16),Dx
	//		CHK.L (PC,Xn,d8),Dx
	//
entry static chk_l_other_dx
	bl		fetchEALong111
	CYCLES(14,14,8)
	b		chk_l_ea_dx_post
#endif

	//================================================================================================

	//
	//		CHK2.B (Ay),Rx
	//
#if (A68000_CHIP >= 68020)
entry static chk2_b_ay0_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	CYCLES(14,14,11)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
chk2_b_ea_rx:
	READ_B_AT(rEA)								// r3 = (Ay)
chk2_b_ea_rx_post:
	mr		rTempSave2,r3						// rTempSave2 = (Ay) = lower bounds
	addi	rEA,rEA,1
	READ_B_AT(rEA)								// r3 = (Ay + 1) = upper bounds
	READ_L_REG(rRX,rRX)							// rRX = Rx
	andi.	r4,rTempSave,0x8000					// is this a data register?
	bne		chk2_b_ea_rx_data					// no, address, don't sign extend it
	extsb	rRX,rRX								// make signed
	extsb	r3,r3								// make signed
	extsb	rTempSave2,rTempSave2				// make signed
	cmpw	cr2,rRX,rTempSave2					// compare to the lower bound
	cmpw	cr1,rRX,r3							// compare to the upper bound
	b		chk2_b_ea_rx_done
chk2_b_ea_rx_data:
	cmplw	cr2,rRX,rTempSave2					// compare to the lower bound
	cmplw	cr1,rRX,r3							// compare to the upper bound
chk2_b_ea_rx_done:
	li		r3,6								// exception 6
	blt		cr2,chk2_b_ea_rx_bad				// if less than lower bound, we fail the test
	beq		cr2,chk2_b_ea_rx_equal				// if we're equal to the lower bounds, branch
	beq		cr1,chk2_b_ea_rx_equal				// if we're equal to the upper bounds, branch
	ble		cr1,chk2_b_ea_rx_inequal			// if not greater, we're done
chk2_b_ea_rx_bad:
	SET_C										// we're out of range; set C = 1
	CLR_Z										// not equal to one of the bounds, clear Z
	andi.	rTempSave,rTempSave,0x0800			// check to see if this is chk2 or cmp2
	beq		executeLoopEndRestore				// bit is clear, this is cmp2, no exception
	b		generateException					// generate an exception
chk2_b_ea_rx_equal:
	SET_Z_1										// we're equal to one of the bounds, set Z
chk2_b_ea_rx_inequal:
	CLR_C										// we're in range, clear C
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		CHK2.B (Ay,d16),Rx
	//
entry static chk2_b_ayd_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	CYCLES(18,18,11)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		chk2_b_ea_rx
	
	//================================================================================================

	//
	//		CHK2.B (Ay,Xn,d8),Rx
	//
entry static chk2_b_ayid_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	bl		computeEA110
	CYCLES(20,20,8)
	b		chk2_b_ea_rx

	//================================================================================================

	//
	//		CHK2.B (xxx).W,Rx
	//		CHK2.B (xxx).L,Rx
	//		CHK2.B (PC,d16),Rx
	//		CHK2.B (PC,Xn,d8),Rx
	//
entry static chk2_b_other_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	bl		fetchEAByte111
	CYCLES(14,14,8)
	b		chk2_b_ea_rx_post
#endif

	//================================================================================================

	//
	//		CHK2.W (Ay),Rx
	//
#if (A68000_CHIP >= 68020)
entry static chk2_w_ay0_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	CYCLES(14,14,11)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
chk2_w_ea_rx:
	READ_W_AT(rEA)								// r3 = (Ay)
chk2_w_ea_rx_post:
	mr		rTempSave2,r3						// rTempSave2 = (Ay) = lower bounds
	addi	rEA,rEA,2
	READ_W_AT(rEA)								// r3 = (Ay + 2) = upper bounds
	READ_L_REG(rRX,rRX)							// rRX = Rx
	andi.	r4,rTempSave,0x8000					// is this a data register?
	bne		chk2_w_ea_rx_data					// no, address, don't sign extend it
	extsh	rRX,rRX								// make signed
	extsh	r3,r3								// make signed
	extsh	rTempSave2,rTempSave2				// make signed
	cmpw	cr2,rRX,rTempSave2					// compare to the lower bound
	cmpw	cr1,rRX,r3							// compare to the upper bound
	b		chk2_w_ea_rx_done
chk2_w_ea_rx_data:
	cmplw	cr2,rRX,rTempSave2					// compare to the lower bound
	cmplw	cr1,rRX,r3							// compare to the upper bound
chk2_w_ea_rx_done:
	li		r3,6								// exception 6
	blt		cr2,chk2_w_ea_rx_bad				// if less than lower bound, we fail the test
	beq		cr2,chk2_w_ea_rx_equal				// if we're equal to the lower bounds, branch
	beq		cr1,chk2_w_ea_rx_equal				// if we're equal to the upper bounds, branch
	ble		cr1,chk2_w_ea_rx_inequal			// if not greater, we're done
chk2_w_ea_rx_bad:
	SET_C										// we're out of range; set C = 1
	CLR_Z										// not equal to one of the bounds, clear Z
	andi.	rTempSave,rTempSave,0x0800			// check to see if this is chk2 or cmp2
	beq		executeLoopEndRestore				// bit is clear, this is cmp2, no exception
	b		generateException					// generate an exception
chk2_w_ea_rx_equal:
	SET_Z_1										// we're equal to one of the bounds, set Z
chk2_w_ea_rx_inequal:
	CLR_C										// we're in range, clear C
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		CHK2.W (Ay,d16),Rx
	//
entry static chk2_w_ayd_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	CYCLES(18,18,11)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		chk2_w_ea_rx
	
	//================================================================================================

	//
	//		CHK2.W (Ay,Xn,d8),Rx
	//
entry static chk2_w_ayid_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	bl		computeEA110
	CYCLES(20,20,8)
	b		chk2_w_ea_rx

	//================================================================================================

	//
	//		CHK2.W (xxx).W,Rx
	//		CHK2.W (xxx).L,Rx
	//		CHK2.W (PC,d16),Rx
	//		CHK2.W (PC,Xn,d8),Rx
	//
entry static chk2_w_other_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	bl		fetchEAWord111
	CYCLES(14,14,8)
	b		chk2_w_ea_rx_post
#endif

	//================================================================================================

	//
	//		CHK2.L (Ay),Rx
	//
#if (A68000_CHIP >= 68020)
entry static chk2_l_ay0_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	CYCLES(14,14,11)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
chk2_l_ea_rx:
	READ_L_AT(rEA)								// r3 = (Ay)
chk2_l_ea_rx_post:
	mr		rTempSave2,r3						// r4 = (Ay) = lower bounds
	addi	rEA,rEA,4
	READ_L_AT(rEA)								// r3 = (Ay + 4) = upper bounds
	READ_L_REG(rRX,rRX)							// rRX = Dx
	cmpw	cr2,rRX,rTempSave2					// compare to the lower bound
	cmpw	cr1,rRX,r3							// compare to the upper bound
	li		r3,6								// exception 6
	blt		cr2,chk2_l_ea_rx_bad				// if less than lower bound, we fail the test
	beq		cr2,chk2_l_ea_rx_equal				// if we're equal to the lower bounds, branch
	beq		cr1,chk2_l_ea_rx_equal				// if we're equal to the upper bounds, branch
	ble		cr1,chk2_l_ea_rx_inequal			// if not greater, we're done
chk2_l_ea_rx_bad:
	SET_C										// we're out of range; set C = 1
	CLR_Z										// not equal to one of the bounds, clear Z
	andi.	rTempSave,rTempSave,0x0800			// check to see if this is chk2 or cmp2
	beq		executeLoopEndRestore				// bit is clear, this is cmp2, no exception
	b		generateException					// generate an exception
chk2_l_ea_rx_equal:
	SET_Z_1										// we're equal to one of the bounds, set Z
chk2_l_ea_rx_inequal:
	CLR_C										// we're in range, clear C
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		CHK2.L (Ay,d16),Rx
	//
entry static chk2_l_ayd_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	CYCLES(18,18,11)
	SAVE_SR
	READ_OPCODE_ARG_LONG(r3)					// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		chk2_l_ea_rx
	
	//================================================================================================

	//
	//		CHK2.L (Ay,Xn,d8),Rx
	//
entry static chk2_l_ayid_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	bl		computeEA110
	CYCLES(20,20,8)
	b		chk2_l_ea_rx

	//================================================================================================

	//
	//		CHK2.L (xxx).W,Rx
	//		CHK2.L (xxx).L,Rx
	//		CHK2.L (PC,d16),Rx
	//		CHK2.L (PC,Xn,d8),Rx
	//
entry static chk2_l_other_rx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,26,29				// determine the destination register
	bl		fetchEALong111
	CYCLES(14,14,8)
	b		chk2_l_ea_rx_post
#endif

	//================================================================================================
	//
	//		CLR -- Bit test and clear
	//
	//================================================================================================

	//
	//		CLR.B Dy
	//
entry static clr_b_dy
	CYCLES(4,4,1)
	li		r5,k68000FlagZ						// r5 = Z
	li		r4,0								// r4 = 0
	rlwimi	rSRFlags,r5,0,28,31					// N=V=C=0, Z=1
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		CLR.B (Ay)
	//
entry static clr_b_ay0
	CYCLES(12,8,5)
	SAVE_PC
	READ_L_AREG(rEA,rRY)						// rEA = Ay
clr_b_ea:
	li		r5,k68000FlagZ						// r5 = Z
	li		r4,0								// r4 = 0
	rlwimi	rSRFlags,r5,0,28,31					// N=V=C=0, Z=1
	SAVE_SR
	WRITE_B_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		CLR.B -(Ay)
	//
entry static clr_b_aym
	CYCLES(14,10,5)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		clr_b_ea
	
	//================================================================================================

	//
	//		CLR.B (Ay)+
	//
entry static clr_b_ayp
	CYCLES(12,8,5)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		clr_b_ea
	
	//================================================================================================

	//
	//		CLR.B (Ay,d16)
	//
entry static clr_b_ayd
	CYCLES(16,12,5)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		clr_b_ea
	
	//================================================================================================

	//
	//		CLR.B (Ay,Xn,d8)
	//
entry static clr_b_ayid
	bl		computeEA110
	CYCLES(18,16,3)
	b		clr_b_ea

	//================================================================================================

	//
	//		CLR.B (xxx).W
	//		CLR.B (xxx).L
	//
entry static clr_b_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		clr_b_absl							// handle long absolute mode
clr_b_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,12,5)
	b		clr_b_ea
clr_b_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,16,4)
	b		clr_b_ea

	//================================================================================================
	//================================================================================================

	//
	//		CLR.W Dy
	//
entry static clr_w_dy
	CYCLES(4,4,1)
	li		r5,k68000FlagZ						// r5 = Z
	li		r4,0								// r4 = 0
	rlwimi	rSRFlags,r5,0,28,31					// N=V=C=0, Z=1
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		CLR.W (Ay)
	//
entry static clr_w_ay0
	CYCLES(12,8,5)
	SAVE_PC
	READ_L_AREG(rEA,rRY)						// rEA = Ay
clr_w_ea:
	li		r5,k68000FlagZ						// r5 = Z
	li		r4,0								// r4 = 0
	rlwimi	rSRFlags,r5,0,28,31					// N=V=C=0, Z=1
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		CLR.W -(Ay)
	//
entry static clr_w_aym
	CYCLES(14,10,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		clr_w_ea
	
	//================================================================================================

	//
	//		CLR.W (Ay)+
	//
entry static clr_w_ayp
	CYCLES(12,8,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		clr_w_ea
	
	//================================================================================================

	//
	//		CLR.W (Ay,d16)
	//
entry static clr_w_ayd
	CYCLES(16,12,5)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		clr_w_ea
	
	//================================================================================================

	//
	//		CLR.W (Ay,Xn,d8)
	//
entry static clr_w_ayid
	bl		computeEA110
	CYCLES(18,16,3)
	b		clr_w_ea

	//================================================================================================

	//
	//		CLR.W (xxx).W
	//		CLR.W (xxx).L
	//
entry static clr_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		clr_w_absl							// handle long absolute mode
clr_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,12,5)
	b		clr_w_ea
clr_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,16,4)
	b		clr_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		CLR.L Dy
	//
entry static clr_l_dy
	CYCLES(6,6,1)
	li		r5,k68000FlagZ						// r5 = Z
	li		r4,0								// r4 = 0
	rlwimi	rSRFlags,r5,0,28,31					// N=V=C=0, Z=1
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		CLR.L (Ay)
	//
entry static clr_l_ay0
	CYCLES(20,12,5)
	SAVE_PC
	READ_L_AREG(rEA,rRY)						// rEA = Ay
clr_l_ea:
	li		r5,k68000FlagZ						// r5 = Z
	li		r4,0								// r4 = 0
	rlwimi	rSRFlags,r5,0,28,31					// N=V=C=0, Z=1
	SAVE_SR
	WRITE_L_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		CLR.L -(Ay)
	//
entry static clr_l_aym
	CYCLES(22,14,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		clr_l_ea
	
	//================================================================================================

	//
	//		CLR.L (Ay)+
	//
entry static clr_l_ayp
	CYCLES(20,12,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		clr_l_ea
	
	//================================================================================================

	//
	//		CLR.L (Ay,d16)
	//
entry static clr_l_ayd
	CYCLES(24,16,5)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		clr_l_ea
	
	//================================================================================================

	//
	//		CLR.L (Ay,Xn,d8)
	//
entry static clr_l_ayid
	bl		computeEA110
	CYCLES(26,20,3)
	b		clr_l_ea

	//================================================================================================

	//
	//		CLR.L (xxx).W
	//		CLR.L (xxx).L
	//
entry static clr_l_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		clr_l_absl							// handle long absolute mode
clr_l_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,16,5)
	b		clr_l_ea
clr_l_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,20,4)
	b		clr_l_ea

	//================================================================================================
	//
	//		CMPA -- Compare to address register
	//
	//================================================================================================

	//
	//		CMPA.W Dy,Ax
	//
entry static cmpa_w_dy_ax
	CYCLES(6,6,1)
	READ_W_REG_EXT(r3,rRY)						// r3 = Dy (sign-extended)
	READ_L_AREG(rRX,rRX)						// rRX = Ax
	CLR_C										// C = 0
	subc	r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100000000)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		CMPA.W Ay,Ax
	//
entry static cmpa_w_ay_ax
	CYCLES(6,6,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	READ_L_AREG(rRX,rRX)						// rRX = Ax
	extsh	r3,r3								// r3 = ext(r3)
	CLR_C										// C = 0
	subc	r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100000000)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		CMPA.W (Ay),Ax
	//
entry static cmpa_w_ay0_ax
	CYCLES(10,10,4)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmpa_w_ea_ax:
	READ_W_AT(rEA)								// r3 = word(rEA)
cmpa_w_ea_ax_post:
	READ_L_AREG(rRX,rRX)						// rRX = Ax
	extsh	r3,r3								// r3 = ext(r3)
	CLR_C										// C = 0
	subc	r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100000000)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPA.W -(Ay),Ax
	//
entry static cmpa_w_aym_ax
	CYCLES(12,12,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmpa_w_ea_ax
	
	//================================================================================================

	//
	//		CMPA.W (Ay)+,Ax
	//
entry static cmpa_w_ayp_ax
	CYCLES(10,10,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmpa_w_ea_ax
	
	//================================================================================================

	//
	//		CMPA.W (Ay,d16),Ax
	//
entry static cmpa_w_ayd_ax
	CYCLES(14,14,4)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmpa_w_ea_ax
	
	//================================================================================================

	//
	//		CMPA.W (Ay,Xn,d8),Ax
	//
entry static cmpa_w_ayid_ax
	bl		computeEA110
	CYCLES(16,16,1)
	b		cmpa_w_ea_ax

	//================================================================================================

	//
	//		CMPA.W (xxx).W,Ax
	//		CMPA.W (xxx).L,Ax
	//		CMPA.W (d16,PC),Ax
	//		CMPA.W (d8,PC,Xn),Ax
	//
entry static cmpa_w_other_ax
	bl		fetchEAWord111
	CYCLES(10,10,1)
	b		cmpa_w_ea_ax_post

	//================================================================================================
	//================================================================================================

	//
	//		CMPA.L Dy,Ax
	//
entry static cmpa_l_dy_ax
	CYCLES(6,6,1)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_AREG(rRX,rRX)						// rRX = Ax
	CLR_C										// C = 0
	subc	r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100000000)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		CMPA.L Ay,Ax
	//
entry static cmpa_l_ay_ax
	CYCLES(6,6,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	READ_L_AREG(rRX,rRX)						// rRX = Ax
	CLR_C										// C = 0
	subc	r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100000000)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		CMPA.L (Ay),Ax
	//
entry static cmpa_l_ay0_ax
	CYCLES(14,14,4)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmpa_l_ea_ax:
	READ_L_AT(rEA)								// r3 = long(rEA)
cmpa_l_ea_ax_post:
	READ_L_AREG(rRX,rRX)						// rRX = Ax
	CLR_C										// C = 0
	subc	r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100000000)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPA.L -(Ay),Ax
	//
entry static cmpa_l_aym_ax
	CYCLES(16,16,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmpa_l_ea_ax
	
	//================================================================================================

	//
	//		CMPA.L (Ay)+,Ax
	//
entry static cmpa_l_ayp_ax
	CYCLES(14,14,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmpa_l_ea_ax
	
	//================================================================================================

	//
	//		CMPA.L (Ay,d16),Ax
	//
entry static cmpa_l_ayd_ax
	CYCLES(18,18,4)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmpa_l_ea_ax
	
	//================================================================================================

	//
	//		CMPA.L (Ay,Xn,d8),Ax
	//
entry static cmpa_l_ayid_ax
	bl		computeEA110
	CYCLES(20,20,1)
	b		cmpa_l_ea_ax

	//================================================================================================

	//
	//		CMPA.L (xxx).W,Ax
	//		CMPA.L (xxx).L,Ax
	//		CMPA.L (d16,PC),Ax
	//		CMPA.L (d8,PC,Xn),Ax
	//
entry static cmpa_l_other_ax
	bl		fetchEALong111_plus2
	CYCLES(14,14,1)
	b		cmpa_l_ea_ax_post

	//================================================================================================
	//
	//		CMPI -- Compare immediate to effective address
	//
	//================================================================================================

	//
	//		CMPI.B #xx,Dy
	//
entry static cmpi_b_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	READ_B_REG(r3,rRY)							// r3 = Dy
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	TRUNC_BYTE(r4)								// keep to a byte
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		CMPI.B #xx,(Ay)
	//
entry static cmpi_b_imm_ay0
	CYCLES(12,12,3)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmpi_b_imm_ea:
	READ_B_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	TRUNC_BYTE(r4)								// keep to a byte
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPI.B #xx,-(Ay)
	//
entry static cmpi_b_imm_aym
	CYCLES(14,14,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmpi_b_imm_ea
	
	//================================================================================================

	//
	//		CMPI.B #xx,(Ay)+
	//
entry static cmpi_b_imm_ayp
	CYCLES(12,12,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmpi_b_imm_ea
	
	//================================================================================================

	//
	//		CMPI.B #xx,(Ay,d16)
	//
entry static cmpi_b_imm_ayd
	CYCLES(16,16,3)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmpi_b_imm_ea
	
	//================================================================================================

	//
	//		CMPI.B #xx,(Ay,Xn,d8)
	//
entry static cmpi_b_imm_ayid
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(18,18,0)
	b		cmpi_b_imm_ea

	//================================================================================================

	//
	//		CMPI.B (xxx).W,Ax
	//		CMPI.B (xxx).L,Ax
	//
entry static cmpi_b_imm_other
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA111
	CYCLES(12,12,0)
	b		cmpi_b_imm_ea

	//================================================================================================
	//================================================================================================

	//
	//		CMPI.W #xx,Dy
	//
entry static cmpi_w_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	READ_W_REG(r3,rRY)							// r3 = Dy
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	TRUNC_WORD(r4)								// keep to a byte
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		CMPI.W #xx,(Ay)
	//
entry static cmpi_w_imm_ay0
	CYCLES(12,12,3)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmpi_w_imm_ea:
	READ_W_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	TRUNC_WORD(r4)								// keep to a byte
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPI.W #xx,-(Ay)
	//
entry static cmpi_w_imm_aym
	CYCLES(14,14,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmpi_w_imm_ea
	
	//================================================================================================

	//
	//		CMPI.W #xx,(Ay)+
	//
entry static cmpi_w_imm_ayp
	CYCLES(12,12,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmpi_w_imm_ea
	
	//================================================================================================

	//
	//		CMPI.W #xx,(Ay,d16)
	//
entry static cmpi_w_imm_ayd
	CYCLES(16,16,3)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmpi_w_imm_ea
	
	//================================================================================================

	//
	//		CMPI.W #xx,(Ay,Xn,d8)
	//
entry static cmpi_w_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA110
	CYCLES(18,18,0)
	b		cmpi_w_imm_ea

	//================================================================================================

	//
	//		CMPI.W (xxx).W,Ax
	//		CMPI.W (xxx).L,Ax
	//
entry static cmpi_w_imm_other
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA111
	CYCLES(12,12,0)
	b		cmpi_w_imm_ea

	//================================================================================================
	//================================================================================================

	//
	//		CMPI.L #xx,Dy
	//
entry static cmpi_l_imm_dy
	CYCLES(14,12,1)
#if USE_SYS16_CALLBACKS
	READ_OPCODE_ARG_LONG(r3)					// r3 = immediate value
	mr		r4,r11								// r4 = opcode
	stw		rICount,0(rICountPtr)
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)
	bl		CMPILDCallback
	_asm_get_global_ptr(rOpcodeROMPtr,A68000_OPCODEROM)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)
	lwz		rICount,0(rICountPtr)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)
#endif
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	READ_L_REG(r3,rRY)							// r3 = Dy
	CLR_C										// C = 0
	subc	r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		CMPI.L #xx,(Ay)
	//
entry static cmpi_l_imm_ay0
	CYCLES(20,20,4)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmpi_l_imm_ea:
	READ_L_AT(rEA)								// r3 = byte(rEA)
	CLR_C										// C = 0
	subc	r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPI.L #xx,-(Ay)
	//
entry static cmpi_l_imm_aym
	CYCLES(22,22,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmpi_l_imm_ea
	
	//================================================================================================

	//
	//		CMPI.L #xx,(Ay)+
	//
entry static cmpi_l_imm_ayp
	CYCLES(20,20,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmpi_l_imm_ea
	
	//================================================================================================

	//
	//		CMPI.L #xx,(Ay,d16)
	//
entry static cmpi_l_imm_ayd
	CYCLES(24,24,4)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmpi_l_imm_ea
	
	//================================================================================================

	//
	//		CMPI.L #xx,(Ay,Xn,d8)
	//
entry static cmpi_l_imm_ayid
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(26,26,1)
	b		cmpi_l_imm_ea

	//================================================================================================

	//
	//		CMPI.L (xxx).W,Ax
	//		CMPI.L (xxx).L,Ax
	//
entry static cmpi_l_imm_other
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA111
	CYCLES(20,20,1)
	b		cmpi_l_imm_ea

	//================================================================================================
	//
	//		CMPM -- Compare memory to memory
	//
	//================================================================================================

	//
	//		CMPM.B (Ay)+,(Ax)+
	//
entry static cmpm_b_ayp_axp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	addi	r4,rTempSave,1						// r4 = Ay + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// put back the updated values
	SAVE_SR
	READ_B_AT(rTempSave)						// r3 = byte(Ay)
	addi	r12,rRX,1*4							// r4 = rRY + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	addi	r4,rEA,1							// r4 = Ax + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	WRITE_L_AREG(r4,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_B_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
cmpm_b_ayp_axp_core:
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	TRUNC_BYTE(r4)								// keep to a byte
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPM.W (Ay)+,(Ax)+
	//
entry static cmpm_w_ayp_axp
	CYCLES(12,12,8)
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	addi	r4,rTempSave,2						// r4 = Ay + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// put back the updated values
	SAVE_SR
	READ_W_AT(rTempSave)						// r3 = byte(Ay)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	addi	r4,rEA,2							// r4 = Ax + 2
	WRITE_L_AREG(r4,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_W_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
cmpm_w_ayp_axp_core:
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	TRUNC_WORD(r4)								// keep to a byte
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMPM.L (Ay)+,(Ax)+
	//
entry static cmpm_l_ayp_axp
	CYCLES(20,20,8)
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	addi	r4,rTempSave,4						// r4 = Ay + 1
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// put back the updated values
	SAVE_SR
	READ_L_AT(rTempSave)						// r3 = byte(Ay)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	addi	r4,rEA,4							// r4 = Ax + 1
	WRITE_L_AREG(r4,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_L_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
cmpm_l_ayp_axp_core:
	subc	r4,r3,rRX							// r4 = r3 - rRX
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================
	//
	//		CMP -- Compare to data register
	//
	//================================================================================================

	//
	//		CMP.B Dy,Dx
	//
entry static cmp_b_dy_dx
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRX,rRX)							// rRX = Dx
	sub		r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	TRUNC_BYTE(r4)								// keep it byte-sized
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_BYTE(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		CMP.B (Ay),Dx
	//
entry static cmp_b_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmp_b_ea_dx:
	READ_B_AT(rEA)								// r3 = word(rEA)
cmp_b_ea_dx_post:
	READ_B_REG(rRX,rRX)							// rRX = Dx
	sub		r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	TRUNC_BYTE(r4)								// keep it byte-sized
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_BYTE(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMP.B -(Ay),Dx
	//
entry static cmp_b_aym_dx
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmp_b_ea_dx
	
	//================================================================================================

	//
	//		CMP.B (Ay)+,Dx
	//
entry static cmp_b_ayp_dx
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmp_b_ea_dx
	
	//================================================================================================

	//
	//		CMP.B (Ay,d16),Dx
	//
entry static cmp_b_ayd_dx
	CYCLES(12,12,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmp_b_ea_dx
	
	//================================================================================================

	//
	//		CMP.B (Ay,Xn,d8),Dx
	//
entry static cmp_b_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		cmp_b_ea_dx

	//================================================================================================

	//
	//		CMP.B (xxx).W,Dx
	//		CMP.B (xxx).L,Dx
	//		CMP.B (d16,PC),Dx
	//		CMP.B (d8,PC,Xn),Dx
	//
entry static cmp_b_other_dx
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		cmp_b_ea_dx_post

	//================================================================================================
	//================================================================================================

	//
	//		CMP.W Ry,Dx
	//
entry static cmp_w_ry_dx
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	READ_W_REG(r3,rRY)							// r3 = Ry
	READ_W_REG(rRX,rRX)							// rRX = Dx
	sub		r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	TRUNC_WORD(r4)								// keep it byte-sized
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_WORD(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		CMP.W (Ay),Dx
	//
entry static cmp_w_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmp_w_ea_dx:
	READ_W_AT(rEA)								// r3 = word(rEA)
cmp_w_ea_dx_post:
	READ_W_REG(rRX,rRX)							// rRX = Dx
	sub		r4,rRX,r3							// r4 = rRX - r3
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	TRUNC_WORD(r4)								// keep it byte-sized
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_WORD(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMP.W -(Ay),Dx
	//
entry static cmp_w_aym_dx
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmp_w_ea_dx
	
	//================================================================================================

	//
	//		CMP.W (Ay)+,Dx
	//
entry static cmp_w_ayp_dx
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmp_w_ea_dx
	
	//================================================================================================

	//
	//		CMP.W (Ay,d16),Dx
	//
entry static cmp_w_ayd_dx
	CYCLES(12,12,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmp_w_ea_dx
	
	//================================================================================================

	//
	//		CMP.W (Ay,Xn,d8),Dx
	//
entry static cmp_w_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		cmp_w_ea_dx

	//================================================================================================

	//
	//		CMP.W (xxx).W,Dx
	//		CMP.W (xxx).L,Dx
	//		CMP.W (d16,PC),Dx
	//		CMP.W (d8,PC,Xn),Dx
	//
entry static cmp_w_other_dx
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		cmp_w_ea_dx_post

	//================================================================================================
	//================================================================================================

	//
	//		CMP.L Ry,Dx
	//
entry static cmp_l_ry_dx
	CYCLES(6,6,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	READ_L_REG(r3,rRY)							// r3 = Ry
	READ_L_REG(rRX,rRX)							// rRX = Dx
	subc	r4,rRX,r3							// r4 = rRX - r3
	CLR_C										// C = 0
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		CMP.L (Ay),Dx
	//
entry static cmp_l_ay0_dx
	CYCLES(14,14,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
cmp_l_ea_dx:
	READ_L_AT(rEA)								// r3 = word(rEA)
cmp_l_ea_dx_post:
	READ_L_REG(rRX,rRX)							// rRX = Dx
	subc	r4,rRX,r3							// r4 = rRX - r3
	CLR_C										// C = 0
	xor		r5,rRX,r3							// r5 = rRX ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	xor		r6,rRX,r4							// r6 = rRX ^ r4
	INVERT_C									// invert the carry sense
	and		r5,r6,r5							// r5 = (rRX ^ r4) & (rRX ^ r3)
	SET_V_LONG(r5)								// V = (rRX ^ r4) & (rRX ^ r3)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		CMP.L -(Ay),Dx
	//
entry static cmp_l_aym_dx
	CYCLES(16,16,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		cmp_l_ea_dx
	
	//================================================================================================

	//
	//		CMP.L (Ay)+,Dx
	//
entry static cmp_l_ayp_dx
	CYCLES(14,14,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		cmp_l_ea_dx
	
	//================================================================================================

	//
	//		CMP.L (Ay,d16),Dx
	//
entry static cmp_l_ayd_dx
	CYCLES(18,18,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		cmp_l_ea_dx
	
	//================================================================================================

	//
	//		CMP.L (Ay,Xn,d8),Dx
	//
entry static cmp_l_ayid_dx
	bl		computeEA110
	CYCLES(20,20,0)
	b		cmp_l_ea_dx

	//================================================================================================

	//
	//		CMP.L (xxx).W,Dx
	//		CMP.L (xxx).L,Dx
	//		CMP.L (d16,PC),Dx
	//		CMP.L (d8,PC,Xn),Dx
	//
entry static cmp_l_other_dx
	bl		fetchEALong111_plus2
	CYCLES(14,14,0)
	b		cmp_l_ea_dx_post

	//================================================================================================
	//
	//		DBCC -- Decrement and branch
	//
	//================================================================================================

	//
	//		DBcc.W dy,<d>
	//
entry static dbcc_t_dy
	CYCLES(12,10,3)
	addi	rPC,rPC,2							// rPC += 2
	b		executeLoopEnd						// if the condition is true, it's a no op

	//================================================================================================

	//
	//		cc == F == 0
	//
entry static dbcc_f_dy
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(14,16,7)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == HI == ~C & ~Z = ~(C | Z)
	//
entry static dbcc_hi_dy
	rlwinm	r3,rSRFlags,30,31,31				// r3 = Z shifted to match C position
	nor		r3,r3,rSRFlags						// r3 = ~(r3 | C)
	rlwinm.	r3,r3,0,31,31						// r3 = ~(C | Z) & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == LS == C | Z
	//
entry static dbcc_ls_dy
	rlwinm	r3,rSRFlags,30,31,31				// r3 = Z shifted to match C position
	or		r3,r3,rSRFlags						// r3 = (r3 | C)
	rlwinm.	r3,r3,0,31,31						// r3 = (C | Z) & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == CC = ~C
	//
entry static dbcc_cc_dy
	not		r3,rSRFlags							// r3 = ~C
	rlwinm.	r0,r3,0,31,31						// r0 = ~C & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == CS = C
	//
entry static dbcc_cs_dy
	rlwinm.	r0,rSRFlags,0,31,31					// r0 = C & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == NE = ~Z
	//
entry static dbcc_ne_dy
	not		r3,rSRFlags							// r3 = ~Z
	rlwinm.	r0,r3,0,29,29						// r0 = ~Z & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == EQ = Z
	//
entry static dbcc_eq_dy
	rlwinm.	r0,rSRFlags,0,29,29					// r0 = Z & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == VC = ~V
	//
entry static dbcc_vc_dy
	not		r3,rSRFlags							// r3 = ~V
	rlwinm.	r0,r3,0,30,30						// r0 = ~V & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == VS = V
	//
entry static dbcc_vs_dy
	rlwinm.	r0,rSRFlags,0,30,30					// r0 = V & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == PL = ~N
	//
entry static dbcc_pl_dy
	not		r3,rSRFlags							// r3 = ~N
	rlwinm.	r0,r3,0,28,28						// r0 = ~N & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == MI = N
	//
entry static dbcc_mi_dy
	rlwinm.	r0,rSRFlags,0,28,28					// r0 = N & 1
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == GE = ~(N ^ V)
	//
entry static dbcc_ge_dy
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	eqv		r3,r3,rSRFlags						// r3 = ~(N ^ V)
	rlwinm.	r0,r3,0,30,30						// r0 = ~(N ^ V) & 2
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == LT = (N ^ V)
	//
entry static dbcc_lt_dy
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	xor		r3,r3,rSRFlags						// r3 = (N ^ V)
	rlwinm.	r0,r3,0,30,30						// r0 = (N ^ V) & 2
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == GE = ~(N ^ V) & ~Z = ~((N ^ V) | Z)
	//
entry static dbcc_gt_dy
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	rlwinm	r4,rSRFlags,31,30,30				// r4 = Z shifted to match V position
	xor		r3,r3,rSRFlags						// r3 = (N ^ V)
	nor		r3,r3,r4							// r3 = ~((N ^ V) | Z)
	rlwinm.	r0,r3,0,30,30						// r0 = ~((N ^ V) | Z) & 2
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		cc == LE = (N ^ V) | Z
	//
entry static dbcc_le_dy
	rlwinm	r3,rSRFlags,30,30,30				// r3 = N shifted to match V position
	rlwinm	r4,rSRFlags,31,30,30				// r4 = Z shifted to match V position
	xor		r3,r3,rSRFlags						// r3 = (N ^ V)
	or		r3,r3,r4							// r4 = ((N ^ V) | Z)
	rlwinm.	r0,r3,0,30,30						// r0 = ((N ^ V) | Z) & 2
	rlwinm	rRX,rPC,0,ADDRESS_MASK				// rRX = rPC & 0xffffff
	addi	rPC,rPC,2							// rPC += 2
	CYCLES(12,10,3)
	bne		executeLoopEnd						// if the condition is true, it's a no op
	lhax	rRX,rOpcodeROM,rRX					// rRX = sign-extended offset in ROM
	READ_W_REG(r3,rRY)							// r3 = register to decrement
	subi	rRX,rRX,2							// adjust the offset to compensate for the PC
	subic.	r3,r3,1								// r3 -= 1
	WRITE_W_REG(r3,rRY)							// save the result back to the register
	CYCLES(2,6,4)
	blt		executeLoopEnd						// if we're less than zero, it's the end
	add		rPC,rPC,rRX							// PC += rRX
	CYCLES(-4,-6,-4)
	UPDATE_BANK_SHORT
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		DIVS/DIVU -- Divide
	//
	//================================================================================================

	//
	//		DIVS.W Dy,Dx
	//
entry static divs_w_dy_dx
	CYCLES(158,122,54)
	READ_W_REG_EXT(r3,rRY)						// r3 = word-sized rRY
divs_w_ea_common:
	READ_L_REG(r4,rRX)							// r4 = long-sized rRX
	cmpwi	r3,0								// check for divide by zero
	CLR_VC										// clear V & C
	beq		div_exception						// if zero, generate an exception
	divw	r5,r4,r3							// r5 = r4 / r3
	rlwinm.	r8,r5,0,0,16						// r8 = r5 & 0xffff8000
	mullw	r6,r5,r3							// r6 = r5 * r3
	beq		divs_w_dy_dx_nooverflow				// skip if no overflow
	not		r8,r5								// r8 = ~r5
	rlwinm.	r8,r8,0,0,16						// r8 = r5 & 0xffff8000
	bne		div_overflow						// if non-zero this time, it's a real overflow
divs_w_dy_dx_nooverflow:
	sub		r6,r4,r6							// r6 = r4 - r5 * r3
	cntlzw	r7,r5								// count the zeros in the result
	SET_N_WORD(r5)								// N = (r5 & 0x8000)
	rlwimi	r5,r6,16,0,15						// pop remainder into high part of the word
	SET_Z(r7)									// Z = (r5 == 0)
	WRITE_L_REG(r5,rRX)							// long-sized rRX = result
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		DIVS.W (Ay),Dx
	//
entry static divs_w_ay0_dx
	CYCLES(162,126,57)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_W_AT(rEA)								// r3 = word(rEA)
	extsh	r3,r3
	b		divs_w_ea_common

	//================================================================================================

	//
	//		DIVS.W -(Ay),Dx
	//
entry static divs_w_aym_dx
	CYCLES(164,128,57)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	READ_W_AT(rEA)								// r3 = word(rEA)
	extsh	r3,r3
	b		divs_w_ea_common
	
	//================================================================================================

	//
	//		DIVS.W (Ay)+,Dx
	//
entry static divs_w_ayp_dx
	CYCLES(162,126,58)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	READ_W_AT(rEA)								// r3 = word(rEA)
	extsh	r3,r3
	b		divs_w_ea_common
	
	//================================================================================================

	//
	//		DIVS.W (Ay,d16),Dx
	//
entry static divs_w_ayd_dx
	CYCLES(166,130,57)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	READ_W_AT(rEA)								// r3 = word(rEA)
	extsh	r3,r3
	b		divs_w_ea_common
	
	//================================================================================================

	//
	//		DIVS.W (Ay,Xn,d8),Dx
	//
entry static divs_w_ayid_dx
	bl		computeEA110
	SAVE_SR
	READ_W_AT(rEA)								// r3 = word(rEA)
	SAVE_PC
	CYCLES(168,132,54)
	extsh	r3,r3
	b		divs_w_ea_common

	//================================================================================================

	//
	//		DIVS.W (xxx).W,Dx
	//		DIVS.W (xxx).L,Dx
	//		DIVS.W (d16,PC),Dx
	//		DIVS.W (d8,PC,Xn),Dx
	//
entry static divs_w_other_dx
	bl		fetchEAWord111
	CYCLES(162,126,54)
	extsh	r3,r3
	b		divs_w_ea_common

	//================================================================================================
	//================================================================================================

	//
	//		DIVU.W Dy,Dx
	//
entry static divu_w_dy_dx
	CYCLES(140,108,42)
	READ_W_REG(r3,rRY)							// r3 = word-sized rRY
divu_w_ea_common:
	READ_L_REG(r4,rRX)							// r4 = long-sized rRX
	cmpwi	r3,0								// check for divide by zero
	CLR_VC										// clear V & C
	beq		div_exception						// if zero, generate an exception
	divwu	r5,r4,r3							// r5 = r4 / r3
	rlwinm.	r0,r5,0,0,15						// r0 = result & 0xffff
	mullw	r6,r5,r3							// r6 = r5 * r3
	bne		div_overflow						// skip if overflow
	cntlzw	r7,r5								// count the zeros in the result
	SET_N_WORD(r5)								// N = (r5 & 0x8000)
	sub		r6,r4,r6							// r6 = r4 - r5 * r3
	SET_Z(r7)									// Z = (r5 == 0)
	rlwimi	r5,r6,16,0,15						// pop remainder into high part of the word
	WRITE_L_REG(r5,rRX)							// long-sized rRX = result
	b		executeLoopEnd						// all done
div_overflow:
	ori		rSRFlags,rSRFlags,k68000FlagV|k68000FlagN // V = N = 1
	b		executeLoopEnd						// all done
div_exception:
	li		r3,5								// vector 5
	b		generateException					// generate an exception
//	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		DIVU.W (Ay),Dx
	//
entry static divu_w_ay0_dx
	CYCLES(144,112,45)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_W_AT(rEA)								// r3 = word(rEA)
	b		divu_w_ea_common

	//================================================================================================

	//
	//		DIVU.W -(Ay),Dx
	//
entry static divu_w_aym_dx
	CYCLES(146,114,45)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	READ_W_AT(rEA)								// r3 = word(rEA)
	b		divu_w_ea_common
	
	//================================================================================================

	//
	//		DIVU.W (Ay)+,Dx
	//
entry static divu_w_ayp_dx
	CYCLES(144,112,46)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	READ_W_AT(rEA)								// r3 = word(rEA)
	b		divu_w_ea_common
	
	//================================================================================================

	//
	//		DIVU.W (Ay,d16),Dx
	//
entry static divu_w_ayd_dx
	CYCLES(148,116,45)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	READ_W_AT(rEA)								// r3 = word(rEA)
	b		divu_w_ea_common
	
	//================================================================================================

	//
	//		DIVU.W (Ay,Xn,d8),Dx
	//
entry static divu_w_ayid_dx
	bl		computeEA110
	SAVE_SR
	READ_W_AT(rEA)								// r3 = word(rEA)
	SAVE_PC
	CYCLES(150,118,42)
	b		divu_w_ea_common
	
	//================================================================================================

	//
	//		DIVU.W (xxx).W,Dx
	//		DIVU.W (xxx).L,Dx
	//		DIVU.W (d16,PC),Dx
	//		DIVU.W (d8,PC,Xn),Dx
	//
entry static divu_w_other_dx
	bl		fetchEAWord111
	CYCLES(144,112,42)
	b		divu_w_ea_common


	//================================================================================================
	//
	//		DIVS.L/DIVU.L -- Divide long
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	// performs r5:r4 / r3
	// returns remainder in r5, quotient in r6
divide64BitSigned:
	cmpwi	cr5,r3,0							// positive divisor? (cr5)
	xor		r0,r5,r3							// r0 = final sign of quotient
	cmpwi	cr6,r5,0							// positive dividend? (cr6)
	cmpwi	cr7,r0,0							// positive quotient? (cr7)
	bge		cr5,divisorPositive					// skip if divisor positive
	neg		r3,r3								// make divisor positive
divisorPositive:
	mflr	r7									// save the original return address
	bge		cr6,dividendPositive				// skip if dividend positive
	not		r4,r4								// make dividend positive (~dividend + 1)
	not		r5,r5
	addic	r4,r4,1
	addze	r5,r5
dividendPositive:
	bl		divide64BitUnsigned					// do the unsigned divide
	mtlr	r7									// copy r7 to the link register
	lis		r0,0x8000							// get the maximum
	cmplw	r6,r0								// compare against the max
	bge		cr7,quotientPositive				// skip if quotient positive
quotientNegative:
	bgt		div_overflow						// overflow if |quotient| > 0x80000000
	neg		r6,r6								// make the quotient negative
	bgelr	cr6									// return if remainder positive
	neg		r5,r5								// make the remainder negative
	blr											// return
quotientPositive:
	bge		div_overflow						// overflow if |quotient| >= 0x80000000
	bgelr	cr6									// return if remainder positive
	neg		r5,r5								// make the remainder negative
	blr											// return

divide64BitUnsigned:
	cmplw	r5,r3
	li		r0,32
	li		r6,0
	mtctr	r0
	bge		div_overflow
divide64Loop:
	rlwinm.	r0,r5,0,0,0
	rlwinm	r5,r5,1,0,30
	rlwinm	r6,r6,1,0,30
	rlwimi	r5,r4,1,31,31
	rlwinm	r4,r4,1,0,30
	cmplw	cr4,r3,r5				// AK - was cr7
	bne		divide64DoIt
	bgt		cr4,divide64SkipIt		// AK - was cr7
divide64DoIt:
	ori		r6,r6,1
	sub		r5,r5,r3
divide64SkipIt:
	bdnz	divide64Loop
	blr

	//
	//		DIVS.L/DIVU.L Dy,Dx
	//
entry static div_l_dy_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_L_REG(r3,rRY)							// r3 = rRY
div_l_ea_common:
	andi.	r9,rTempSave,0x0800					// see if this is signed or unsigned
	cmpwi	cr7,r3,0							// check for divide by zero
	READ_L_REG(r4,rRX)							// r4 = long-sized rRX
	CLR_VC										// clear V & C
	beq		cr7,div_exception					// if zero, generate an exception
	beq		divu_l_common

divs_l_common:
	andi.	r0,rTempSave,0x0400					// see if this is 32-bit or 64-bit
	rlwinm	rRY,rTempSave,2,27,29				// determine the upper destination register
	CYCLES(0,0,89)
	READ_L_REG(r5,rRY)							// r5 = long-sized additional register
	bne		divs_l_64bit
	
divs_l_32bit:
	divwo	r6,r4,r3							// r6 = r4 / r3
	mcrxr	cr0									// get overflow bit
	mullw	r5,r6,r3							// r5 = r6 * r3
	bt		1,div_overflow						// handle overflow case
	sub		r5,r4,r5							// r5 = r4 - r6 * r3
	cntlzw	r7,r6								// count the zeros in the result
	WRITE_L_REG(r5,rRY)							// write the remainder (must be first)
	SET_N_WORD(r6)								// N = (r5 & 0x8000)
	SET_Z(r7)									// Z = (r5 == 0)
	WRITE_L_REG(r6,rRX)							// write the quotient
	b		executeLoopEnd						// all done

divs_l_64bit:
	bl		divide64BitSigned
	cntlzw	r7,r6								// count the zeros in the result
	WRITE_L_REG(r5,rRY)							// write the remainder (must be first)
	SET_N_WORD(r6)								// N = (r5 & 0x8000)
	SET_Z(r7)									// Z = (r5 == 0)
	WRITE_L_REG(r6,rRX)							// write the quotient
	b		executeLoopEnd						// all done

divu_l_common:
	andi.	r0,rTempSave,0x0400					// see if this is 32-bit or 64-bit
	rlwinm	rRY,rTempSave,2,27,29				// determine the upper destination register
	CYCLES(0,0,77)
	READ_L_REG(r5,rRY)							// r5 = long-sized additional register
	bne		divu_l_64bit
	
divu_l_32bit:
	divwuo	r6,r4,r3							// r6 = r4 / r3
	mcrxr	cr0									// get overflow bit
	mullw	r5,r6,r3							// r5 = r6 * r3
	bt		1,div_overflow						// handle overflow case
	sub		r5,r4,r5							// r5 = r4 - r6 * r3
	cntlzw	r7,r6								// count the zeros in the result
	WRITE_L_REG(r5,rRY)							// write the remainder (must be first)
	SET_N_WORD(r6)								// N = (r5 & 0x8000)
	SET_Z(r7)									// Z = (r5 == 0)
	WRITE_L_REG(r6,rRX)							// write the quotient
	b		executeLoopEnd						// all done

divu_l_64bit:
	bl		divide64BitUnsigned
	cntlzw	r7,r6								// count the zeros in the result
	WRITE_L_REG(r5,rRY)							// write the remainder (must be first)
	SET_N_WORD(r6)								// N = (r5 & 0x8000)
	SET_Z(r7)									// Z = (r5 == 0)
	WRITE_L_REG(r6,rRX)							// write the quotient
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		DIVS.L/DIVU.L (Ay),Dx
	//
entry static div_l_ay0_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,3)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_L_AT(rEA)								// r3 = long(rEA)
	b		div_l_ea_common

	//================================================================================================

	//
	//		DIVS.L/DIVU.L -(Ay),Dx
	//
entry static div_l_aym_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,3)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	READ_L_AT(rEA)								// r3 = long(rEA)
	b		div_l_ea_common
	
	//================================================================================================

	//
	//		DIVS.L/DIVU.L (Ay)+,Dx
	//
entry static div_l_ayp_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,4)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	READ_L_AT(rEA)								// r3 = long(rEA)
	b		div_l_ea_common
	
	//================================================================================================

	//
	//		DIVS.L/DIVU.L (Ay,d16),Dx
	//
entry static div_l_ayd_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,3)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	READ_L_AT(rEA)								// r3 = long(rEA)
	b		div_l_ea_common
	
	//================================================================================================

	//
	//		DIVS.L/DIVU.L (Ay,Xn,d8),Dx
	//
entry static div_l_ayid_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	bl		computeEA110
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	SAVE_SR
	READ_L_AT(rEA)								// r3 = long(rEA)
	SAVE_PC
	b		div_l_ea_common

	//================================================================================================

	//
	//		DIVS.L/DIVU.L (xxx).W,Dx
	//		DIVS.L/DIVU.L (xxx).L,Dx
	//		DIVS.L/DIVU.L (d16,PC),Dx
	//		DIVS.L/DIVU.L (d8,PC,Xn),Dx
	//
entry static div_l_other_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	bl		fetchEALong111
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	CYCLES(0,0,3)
	b		div_l_ea_common
#endif


	//================================================================================================
	//
	//		EORI -- And immediate with effective address
	//
	//================================================================================================

	//
	//		EORI.B #xx,Dy
	//
entry static eori_b_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	READ_B_REG(r3,rRY)							// r3 = Dy
	xor		r4,r3,rRX							// r4 = r3 ^ rRX
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EORI.B #xx,(Ay)
	//
entry static eori_b_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
eori_b_imm_ea:
	READ_B_AT(rEA)								// r3 = byte(rEA)
	xor		r4,r3,rRX							// r4 = r3 ^ rRX
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		EORI.B #xx,-(Ay)
	//
entry static eori_b_imm_aym
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		eori_b_imm_ea
	
	//================================================================================================

	//
	//		EORI.B #xx,(Ay)+
	//
entry static eori_b_imm_ayp
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		eori_b_imm_ea
	
	//================================================================================================

	//
	//		EORI.B #xx,(Ay,d16)
	//
entry static eori_b_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		eori_b_imm_ea
	
	//================================================================================================

	//
	//		EORI.B #xx,(Ay,Xn,d8)
	//
entry static eori_b_imm_ayid
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		eori_b_imm_ea

	//================================================================================================

	//
	//		EORI.B (xxx).W,Ax
	//		EORI.B (xxx).L,Ax
	//
entry static eori_b_imm_other
	cmpwi	cr1,rRY,4<<2						// is this the CCR case?
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	beq		cr1,eori_b_imm_ccr					// handle CCR case
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		eori_b_imm_absl						// handle long absolute mode
eori_b_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		eori_b_imm_ea
eori_b_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		eori_b_imm_ea
eori_b_imm_ccr:
	xor		r3,rSRFlags,rRX						// r3 = rSRFlags ^ rRX
	rlwimi	rSRFlags,r3,0,24,31					// keep only the low byte
	CYCLES(20,16,9)
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		EORI.W #xx,Dy
	//
entry static eori_w_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	READ_W_REG(r3,rRY)							// r3 = Dy
	xor		r4,r3,rRX							// r4 = r3 ^ rRX
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EORI.W #xx,(Ay)
	//
entry static eori_w_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
eori_w_imm_ea:
	READ_W_AT(rEA)								// r3 = byte(rEA)
	xor		r4,r3,rRX							// r4 = r3 ^ rRX
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		EORI.W #xx,-(Ay)
	//
entry static eori_w_imm_aym
	CYCLES(18,18,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		eori_w_imm_ea
	
	//================================================================================================

	//
	//		EORI.W #xx,(Ay)+
	//
entry static eori_w_imm_ayp
	CYCLES(16,16,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		eori_w_imm_ea
	
	//================================================================================================

	//
	//		EORI.W #xx,(Ay,d16)
	//
entry static eori_w_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		eori_w_imm_ea
	
	//================================================================================================

	//
	//		EORI.W #xx,(Ay,Xn,d8)
	//
entry static eori_w_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		eori_w_imm_ea

	//================================================================================================

	//
	//		EORI.W (xxx).W,Ax
	//		EORI.W (xxx).L,Ax
	//
entry static eori_w_imm_other
	cmpwi	cr1,rRY,4<<2						// is this the SR case?
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	beq		cr1,eori_w_imm_sr					// handle SR case
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		eori_w_imm_absl						// handle long absolute mode
eori_w_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		eori_w_imm_ea
eori_w_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		eori_w_imm_ea
eori_w_imm_sr:
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, exception time
	xor		r3,rSRFlags,rRX						// r3 = rSRFlags ^ rRX
	CYCLES(20,16,9)
	b		executeLoopEndUpdateSR				// all done

	//================================================================================================
	//================================================================================================

	//
	//		EORI.L #xx,Dy
	//
entry static eori_l_imm_dy
	CYCLES(16,14,1)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	READ_L_REG(r3,rRY)							// r3 = Dy
	xor		r4,r3,rRX							// r4 = r3 ^ rRX
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EORI.L #xx,(Ay)
	//
entry static eori_l_imm_ay0
	CYCLES(28,28,4)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
eori_l_imm_ea:
	READ_L_AT(rEA)								// r3 = byte(rEA)
	xor		r4,r3,rRX							// r4 = r3 ^ rRX
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		EORI.L #xx,-(Ay)
	//
entry static eori_l_imm_aym
	CYCLES(30,30,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		eori_l_imm_ea
	
	//================================================================================================

	//
	//		EORI.L #xx,(Ay)+
	//
entry static eori_l_imm_ayp
	CYCLES(28,28,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		eori_l_imm_ea
	
	//================================================================================================

	//
	//		EORI.L #xx,(Ay,d16)
	//
entry static eori_l_imm_ayd
	CYCLES(32,32,4)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		eori_l_imm_ea
	
	//================================================================================================

	//
	//		EORI.L #xx,(Ay,Xn,d8)
	//
entry static eori_l_imm_ayid
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(34,34,4)
	b		eori_l_imm_ea

	//================================================================================================

	//
	//		EORI.L (xxx).W,Ax
	//		EORI.L (xxx).L,Ax
	//
entry static eori_l_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	bne		eori_l_imm_absl						// handle long absolute mode
eori_l_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(32,32,4)
	b		eori_l_imm_ea
eori_l_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(36,36,4)
	b		eori_l_imm_ea

	//================================================================================================
	//
	//		EOR -- Exclusive OR two values
	//
	//================================================================================================

	//
	//		EOR.B Dx,Dy
	//
entry static eor_b_dx_dy
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRX,rRX)							// rRX = Dx
	xor		r4,rRX,r3							// r4 = rRX ^ r3
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EOR.B (Ax),Dy
	//
entry static eor_b_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
eor_b_dx_ea:
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_AT(rEA)								// r3 = byte(rEA)
	xor		r4,rRX,r3							// r4 = rRX ^ r3
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// (rEA) = result
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		EOR.B -(Ax),Dy
	//
entry static eor_b_dx_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		eor_b_dx_ea
	
	//================================================================================================

	//
	//		EOR.B (Ax)+,Dy
	//
entry static eor_b_dx_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		eor_b_dx_ea
	
	//================================================================================================

	//
	//		EOR.B (Ax,d16),Dy
	//
entry static eor_b_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		eor_b_dx_ea
	
	//================================================================================================

	//
	//		EOR.B (Ax,Xn,d8),Dy
	//
entry static eor_b_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		eor_b_dx_ea

	//================================================================================================

	//
	//		EOR.B (xxx).W,Dy
	//		EOR.B (xxx).L,Dy
	//
entry static eor_b_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		eor_b_dx_absl						// handle long absolute mode
eor_b_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		eor_b_dx_ea
eor_b_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		eor_b_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		EOR.W Dx,Dy
	//
entry static eor_w_dx_dy
	CYCLES(4,4,1)
	READ_W_REG(r3,rRY)							// r3 = Dy
	READ_W_REG(rRX,rRX)							// rRX = Dx
	xor		r4,rRX,r3							// r4 = rRX ^ r3
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EOR.W (Ax),Dy
	//
entry static eor_w_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
eor_w_dx_ea:
	READ_W_REG(rRX,rRX)							// rRX = Dx
	READ_W_AT(rEA)								// r3 = byte(rEA)
	xor		r4,rRX,r3							// r4 = rRX ^ r3
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// (rEA) = result
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		EOR.W -(Ax),Dy
	//
entry static eor_w_dx_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		eor_w_dx_ea
	
	//================================================================================================

	//
	//		EOR.W (Ax)+,Dy
	//
entry static eor_w_dx_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		eor_w_dx_ea
	
	//================================================================================================

	//
	//		EOR.W (Ax,d16),Dy
	//
entry static eor_w_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		eor_w_dx_ea
	
	//================================================================================================

	//
	//		EOR.W (Ax,Xn,d8),Dy
	//
entry static eor_w_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		eor_w_dx_ea

	//================================================================================================

	//
	//		EOR.W (xxx).W,Dy
	//		EOR.W (xxx).L,Dy
	//
entry static eor_w_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		eor_w_dx_absl						// handle long absolute mode
eor_w_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		eor_w_dx_ea
eor_w_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		eor_w_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		EOR.L Dx,Dy
	//
entry static eor_l_dx_dy
	CYCLES(8,6,1)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_REG(rRX,rRX)							// rRX = Dx
	xor		r4,rRX,r3							// r4 = rRX ^ r3
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EOR.L (Ax),Dy
	//
entry static eor_l_dx_ay0
	CYCLES(20,18,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
eor_l_dx_ea:
	READ_L_REG(rRX,rRX)							// rRX = Dx
	READ_L_AT(rEA)								// r3 = byte(rEA)
	xor		r4,rRX,r3							// r4 = rRX ^ r3
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// (rEA) = result
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		EOR.L -(Ax),Dy
	//
entry static eor_l_dx_aym
	CYCLES(22,20,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		eor_l_dx_ea
	
	//================================================================================================

	//
	//		EOR.L (Ax)+,Dy
	//
entry static eor_l_dx_ayp
	CYCLES(20,18,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		eor_l_dx_ea
	
	//================================================================================================

	//
	//		EOR.L (Ax,d16),Dy
	//
entry static eor_l_dx_ayd
	CYCLES(24,22,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		eor_l_dx_ea
	
	//================================================================================================

	//
	//		EOR.L (Ax,Xn,d8),Dy
	//
entry static eor_l_dx_ayid
	bl		computeEA110
	CYCLES(26,24,3)
	b		eor_l_dx_ea

	//================================================================================================

	//
	//		EOR.L (xxx).W,Dy
	//		EOR.L (xxx).L,Dy
	//
entry static eor_l_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		eor_l_dx_absl						// handle long absolute mode
eor_l_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,22,6)
	b		eor_l_dx_ea
eor_l_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,26,6)
	b		eor_l_dx_ea

	//================================================================================================
	//
	//		EXG -- Exchange two registers
	//
	//================================================================================================

	//
	//		EXG Ax,Ay
	//
entry static exg_ax_ay
	CYCLES(6,6,1)
	READ_L_AREG(r3,rRX)
	READ_L_AREG(r4,rRY)
	WRITE_L_AREG(r3,rRY)
	WRITE_L_AREG(r4,rRX)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EXG Dx,Ay
	//
entry static exg_dx_ay
	CYCLES(6,6,1)
	READ_L_REG(r3,rRX)
	READ_L_AREG(r4,rRY)
	WRITE_L_AREG(r3,rRY)
	WRITE_L_REG(r4,rRX)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		EXG Dx,Dy
	//
entry static exg_dx_dy
	CYCLES(6,6,1)
	READ_L_REG(r3,rRX)
	READ_L_REG(r4,rRY)
	WRITE_L_REG(r3,rRY)
	WRITE_L_REG(r4,rRX)
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		EXT -- Sign-extend a register
	//
	//================================================================================================

	//
	//		EXT.W Dx
	//
entry static ext_w_dy
	CYCLES(4,4,1)
	CLR_VC										// V = C = 0
	READ_B_REG(r3,rRY)							// r3 = value of Dx
	extsb	r4,r3								// r4 = word extend(r3)
	cntlzw	r7,r3								// r7 = number of zeros in r7
	SET_N_WORD(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r3 == 0)
	WRITE_W_REG(r4,rRY)							// store the new value
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		EXT.L Dx
	//
entry static ext_l_dy
	CYCLES(4,4,1)
	CLR_VC										// V = C = 0
	READ_W_REG_EXT(r4,rRY)						// r3 = value of Dx
	cntlzw	r7,r4								// r7 = number of zeros in r7
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r3 == 0)
	WRITE_L_REG(r4,rRY)							// store the new value
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		EXTB.L Dx
	//
#if (A68000_CHIP >= 68020)
entry static extb_l_dy
	CYCLES(4,4,1)
	CLR_VC										// V = C = 0
	READ_B_REG(r3,rRY)							// r3 = value of Dx
	extsb	r4,r3								// r4 = word extend(r3)
	cntlzw	r7,r3								// r7 = number of zeros in r7
	SET_N_LONG(r4)								// N = (r4 & 0x80000000)
	SET_Z(r7)									// Z = (r3 == 0)
	WRITE_L_REG(r4,rRY)							// store the new value
	b		executeLoopEnd						// return
#endif

	//================================================================================================
	//
	//		HODGEPODGE -- Several miscellaneous instructions
	//
	//================================================================================================

entry static hodgepodge
	cmpwi	cr1,rRY,4<<2						// compare against 4
	andi.	r0,rRY,1<<2							// check the low bit
	bge		cr1,hodgepodge_1xx					// skip if bit 2 is set
	cmpwi	cr2,rRY,2<<2						// compare against 2 now
	bne		hodgepodge_0x1						// skip if bit 0 is set
	bge		cr2,hodgepodge_010_stop				// skip if bit 1 is set

	//================================================================================================

	//
	//		RESET
	//
hodgepodge_000_reset:
	andi.	r0,rSRFlags,k68000FlagS				// check for supervisor mode
	beq		supervisorException					// if not in supervisor mode, generate an exception
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)
	bl		ResetCallback
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)
	CYCLES(132,130,518)
	b		executeLoopEnd						// return

hodgepodge_0x1:
	bge		cr2,hodgepodge_011_rte				// skip if bit 1 is set

	//================================================================================================

	//
	//		NOP
	//
hodgepodge_001_nop:
	CYCLES(4,4,1)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		STOP
	//
hodgepodge_010_stop:
	andi.	r0,rSRFlags,k68000FlagS				// check for supervisor mode
	beq		supervisorException					// if not in supervisor mode, generate an exception
	READ_OPCODE_ARG(r3)							// r3 = immediate value
	CYCLES(4,4,8)
	oris	rSRFlags,rSRFlags,(kSRFlagsSTOP>>16)// set the stop bit in the flags register
	li		rICount,0							// set icount to 0
	b		executeLoopEndUpdateSR				// copy r3 into the SR and update

	//================================================================================================

	//
	//		RTE/RTR
	//
hodgepodge_011_rte:
	andi.	r0,rSRFlags,k68000FlagS				// check for supervisor mode
	beq		supervisorException					// if not in supervisor mode, generate an exception
#if USE_SYS16_CALLBACKS
	stw		rICount,0(rICountPtr)
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)
	bl		RTECallback
	_asm_get_global_ptr(rOpcodeROMPtr,A68000_OPCODEROM)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)
	lwz		rICount,0(rICountPtr)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)
#endif
	CYCLES(20,20,20)
	SAVE_PC
	GET_A7(rRX)									// rRX = A7
	SAVE_SR
	READ_W_AT(rRX)								// r3 = (rRX)
	addi	rRX,rRX,2							// rRX += 2
	mr		rRY,r3								// rRY = new SR value
	READ_L_AT(rRX)								// r3 = (rRX)
	addi	rRX,rRX,4							// rRX += 4
	mr		rPC,r3								// rPC = r3
#if (A68000_CHIP >= 68010)
	READ_W_AT(rRX)								// r3 = format word (currently ignored)
	addi	rRX,rRX,2							// rRX += 2
#endif
	SET_A7(rRX)									// A7 = rRX
	LOAD_ICOUNT
	UPDATE_BANK
	mr		r3,rRY								// r3 = new SR value
	b		executeLoopEndUpdateSR				// return and update the SR

hodgepodge_111_rtr:
	CYCLES(20,20,13)
	SAVE_PC
	GET_A7(rRX)									// rRX = A7
	SAVE_SR
	READ_W_AT(rRX)								// r3 = (rRX)
	addi	rRX,rRX,2							// rRX += 2
	rlwimi	rSRFlags,r3,0,24,31					// or in the low 8 bits of the SR
	READ_L_AT(rRX)								// r3 = (rRX)
	addi	rRX,rRX,4							// rRX += 4
	mr		rPC,r3								// rPC = r3
	SET_A7(rRX)									// A7 = rRX
	LOAD_ICOUNT
	UPDATE_BANK
	b		executeLoopEnd						// return

hodgepodge_1xx:
	cmpwi	cr2,rRY,6<<2						// compare against 6 now
	bne		hodgepodge_1x1						// skip if bit 0 is set
	bge		cr2,hodgepodge_110_trapv			// skip if bit 1 is set

	//================================================================================================


#if (A68000_CHIP >= 68010)
	//
	//		RTD
	//
hodgepodge_011_rtd:
	CYCLES(0,16,9)
	GET_A7(rRX)									// rRX = A7
	SAVE_PC
	READ_OPCODE_ARG_EXT(rRY)					// rRY = offset
	SAVE_SR
	READ_L_AT(rRX)								// r3 = (rRX)
	addi	rRX,rRX,4							// rRX += 4
	LOAD_SR
	add		rRX,rRX,rRY							// rRX += offset
	mr		rPC,r3								// rPC = r3
	SET_A7(rRX)									// A7 = rRX
	LOAD_ICOUNT
	UPDATE_BANK
	b		executeLoopEnd						// return
#else
	b		illegal
#endif

hodgepodge_1x1:
	bge		cr2,hodgepodge_111_rtr				// skip if bit 1 is set

	//================================================================================================

	//
	//		RTS
	//
hodgepodge_101_rts:
	CYCLES(16,16,9)
	GET_A7(rRX)									// rRX = A7
	SAVE_PC
	SAVE_SR
	READ_L_AT(rRX)								// r3 = (rRX)
	addi	rRX,rRX,4							// rRX += 4
	LOAD_SR
	mr		rPC,r3								// rPC = r3
	SET_A7(rRX)									// A7 = rRX
	LOAD_ICOUNT
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		TRAPV
	//
hodgepodge_110_trapv:
	andi.	r0,rSRFlags,k68000FlagV				// test the V flag
	CYCLES(4,4,23)
	li		r3,7								// _SRC = vector 7
	bne		generateException					// if set, generate an exception
	CYCLES(0,0,-22)
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		JMP -- Jump to effective address
	//
	//================================================================================================

	//
	//		JMP (Ay)
	//
entry static jmp_ay0
	CYCLES(8,8,1)
	READ_L_AREG(rPC,rRY)						// rPC = Ay
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		JMP (Ay,d16)
	//
entry static jmp_ayd
	CYCLES(10,10,2)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rPC,rRY)						// rPC = Ay
	add		rPC,rPC,r3							// rPC += r3
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		JMP (Ay,Xn,d8)
	//
entry static jmp_ayid
	bl		computeEA110
	CYCLES(14,14,1)
	mr		rPC,rEA								// rPC = rEA
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		JMP (xxx).W
	//		JMP (xxx).L
	//		JMP (d16,PC)
	//		JMP (d8,PC,Xn)
	//
entry static jmp_other
	bl		computeEA111
	CYCLES(6,6,1)
	mr		rPC,rEA								// rPC = rEA
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		JSR -- Jump to effective address
	//
	//================================================================================================

	//
	//		JSR (Ay)
	//
entry static jsr_ay0
	CYCLES(16,16,3)
	GET_A7(rRX)									// rRX = A7
	subi	rRX,rRX,4							// rRX -= 4
	SAVE_SR
	mr		r4,rPC								// r4 = rPC
	WRITE_L_AT(rRX)								// write the PC
	LOAD_ICOUNT
	SET_A7(rRX)									// A7 = rRX
	LOAD_SR
	READ_L_AREG(rPC,rRY)						// rPC = Ay
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		JSR (Ay,d16)
	//
entry static jsr_ayd
	CYCLES(18,18,4)
	GET_A7(rRX)									// rRX = A7
	subi	rRX,rRX,4							// rRX -= 4
	SAVE_SR
	addi	r4,rPC,2							// r4 = rPC + 2
	WRITE_L_AT(rRX)								// write the PC
	LOAD_ICOUNT
	SET_A7(rRX)									// A7 = rRX
	LOAD_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rPC,rRY)						// rPC = Ay
	add		rPC,rPC,r3							// rPC += r3
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		JSR (Ay,Xn,d8)
	//
entry static jsr_ayid
	GET_A7(rRX)									// rRX = A7
	subi	rRX,rRX,4							// rRX -= 4
	SAVE_SR
	LOAD_ICOUNT
	SET_A7(rRX)									// A7 = rRX
	LOAD_SR
	bl		computeEA110
	mr		r4,rPC								// r4 = rPC
	CYCLES(22,22,3)
	WRITE_L_AT(rRX)								// write the PC
	mr		rPC,rEA								// rPC = rEA
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		JSR (xxx).W
	//		JSR (xxx).L
	//		JSR (d16,PC)
	//		JSR (d8,PC,Xn)
	//
entry static jsr_other
	GET_A7(rRX)									// rRX = A7
	subi	rRX,rRX,4							// rRX -= 4
	SAVE_SR
	LOAD_ICOUNT
	SET_A7(rRX)									// A7 = rRX
	LOAD_SR
	bl		computeEA111
	mr		r4,rPC								// r4 = rPC
	CYCLES(14,14,3)
	WRITE_L_AT(rRX)								// write the PC
	mr		rPC,rEA								// rPC = rEA
	UPDATE_BANK
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		LEA -- Load effective address
	//
	//================================================================================================

	//
	//		LEA (Ay),Ax
	//
entry static lea_ay0_ax
	CYCLES(4,4,2)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	WRITE_L_AREG(rEA,rRX)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		LEA (Ay,d16),Ax
	//
entry static lea_ayd_ax
	CYCLES(8,8,4)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	add		rEA,rEA,r3							// rEA += r3
	WRITE_L_AREG(rEA,rRX)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		LEA (Ay,Xn,d8),Ax
	//
entry static lea_ayid_ax
	bl		computeEA110
	CYCLES(12,12,2)
	WRITE_L_AREG(rEA,rRX)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		LEA (xxx).W,Ax
	//		LEA (xxx).L,Ax
	//		LEA (d16,PC),Ax
	//		LEA (d8,PC,Xn),Ax
	//
entry static lea_other_ax
	bl		computeEA111
	CYCLES(4,4,2)
	WRITE_L_AREG(rEA,rRX)
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		LINK -- Allocate stack space
	//
	//================================================================================================

entry static link_imm_ay
	CYCLES(16,16,3)
	GET_A7(rEA)									// rEA = A7
	SAVE_SR
	READ_L_AREG(r4,rRY)							// r4 = Ay
	subi	rEA,rEA,4							// rEA -= 4
	READ_OPCODE_ARG_EXT(rRX)					// rRX = sign-extended offset
	SAVE_PC
	WRITE_L_AT(rEA)								// write Ax
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	add		rEA,rEA,rRX							// rEA += rRX
	SET_A7(rEA)									// A7 = rEA
	b		executeLoopEndRestore				// return

	//================================================================================================
	//
	//		LSL -- Logical shift left
	//
	//================================================================================================

	//
	//		LSL.B Dx,Dy
	//
entry static lsl_b_dx_dy
	CYCLES(6,6,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	CLR_V										// V = 0
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSL.B Ix,Dy
	//
entry static lsl_b_ix_dy
	CYCLES(6,6,1)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	CLR_V										// V = 0
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		LSL.W Dx,Dy
	//
entry static lsl_w_dx_dy
	CYCLES(6,6,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	CLR_V										// V = 0
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSL.W Ix,Dy
	//
entry static lsl_w_ix_dy
	CYCLES(6,6,1)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	CLR_V										// V = 0
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSL.W #1,(Ay)
	//
entry static lsl_w_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
lsl_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	CLR_V										// V = 0
	rlwinm	r4,r3,1,16,30						// r4 = r3 << 1
	rlwimi	rSRFlags,r3,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SET_X_FROM_C								// X = C
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		LSL.W #1,-(Ay)
	//
entry static lsl_w_aym
	CYCLES(14,14,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		lsl_w_ea
	
	//================================================================================================

	//
	//		LSL.W #1,(Ay)+
	//
entry static lsl_w_ayp
	CYCLES(12,12,9)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		lsl_w_ea
	
	//================================================================================================

	//
	//		LSL.W #1,(Ay,d16)
	//
entry static lsl_w_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		lsl_w_ea
	
	//================================================================================================

	//
	//		LSL.W #1,(Ay,Xn,d8)
	//
entry static lsl_w_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		lsl_w_ea

	//================================================================================================

	//
	//		LSL.W #1,(xxx).W
	//		LSL.W #1,(xxx).L
	//
entry static lsl_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		lsl_w_absl							// handle long absolute mode
lsl_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		lsl_w_ea
lsl_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		lsl_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		LSL.L Dx,Dy
	//
entry static lsl_l_dx_dy
	CYCLES(8,8,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	CLR_V										// V = 0
	add		r0,rRX,rRX
	subi	rRX,rRX,1							// RX -= 1
	add		rCycleCount,rCycleCount,r0
	beq		lsl_l_dx_dy_0						// special case for zero
	slw		r4,r3,rRX							// r4 = r3 << rRX
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	rlwinm	r4,r4,1,0,30						// shift the last bit out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
lsl_l_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSL.L Ix,Dy
	//
entry static lsl_l_ix_dy
	CYCLES(10,10,1)
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	subi	rRX,rRX,1							// rRX -= 1
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,7							// rRX = (rRX - 1) & 7
	CLR_V										// V = 0
	add		r0,rRX,rRX
	slw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	rlwinm	r4,r4,1,0,30						// shift the last bit out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		LSR -- Arithmetic shift right
	//
	//================================================================================================

	//
	//		LSR.B Dx,Dy
	//
entry static lsr_b_dx_dy
	CYCLES(6,6,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwinm	r3,r3,24,0,7						// rotate the value high
	add		r0,rRX,rRX
	srw		r4,r3,rRX							// r4 = r3 >> rRX
	add		rCycleCount,rCycleCount,r0
	rlwinm	r5,r4,8,24,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,9,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSR.B Ix,Dy
	//
entry static lsr_b_ix_dy
	CYCLES(6,6,1)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwinm	r3,r3,24,0,7						// rotate the value high
	add		r0,rRX,rRX
	srw		r4,r3,rRX							// r4 = r3 >> rRX
	add		rCycleCount,rCycleCount,r0
	rlwinm	r5,r4,8,24,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,9,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		LSR.W Dx,Dy
	//
entry static lsr_w_dx_dy
	CYCLES(6,6,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwinm	r3,r3,16,0,15						// rotate the value high
	add		r0,rRX,rRX
	srw		r4,r3,rRX							// r4 = r3 >> rRX
	add		rCycleCount,rCycleCount,r0
	rlwinm	r5,r4,16,16,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	beq		executeLoopEnd						// X is unaffected for shift count of zero
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSR.W Ix,Dy
	//
entry static lsr_w_ix_dy
	CYCLES(6,6,1)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwinm	r3,r3,16,0,15						// rotate the value high
	add		r0,rRX,rRX
	srw		r4,r3,rRX							// r4 = r3 >> rRX
	add		rCycleCount,rCycleCount,r0
	rlwinm	r5,r4,16,16,31						// get the byte result in r5
	rlwimi	rSRFlags,r4,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSR.W #1,(Ay)
	//
entry static lsr_w_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
lsr_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,31,17,31						// r4 = r3 >> 1
	rlwimi	rSRFlags,r3,0,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SET_X_FROM_C								// X = C
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		LSR.W #1,-(Ay)
	//
entry static lsr_w_aym
	CYCLES(14,14,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		lsr_w_ea
	
	//================================================================================================

	//
	//		LSR.W #1,(Ay)+
	//
entry static lsr_w_ayp
	CYCLES(12,12,9)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		lsr_w_ea
	
	//================================================================================================

	//
	//		LSR.W #1,(Ay,d16)
	//
entry static lsr_w_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		lsr_w_ea
	
	//================================================================================================

	//
	//		LSR.W #1,(Ay,Xn,d8)
	//
entry static lsr_w_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		lsr_w_ea

	//================================================================================================

	//
	//		LSR.W #1,(xxx).W
	//		LSR.W #1,(xxx).L
	//
entry static lsr_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		lsr_w_absl							// handle long absolute mode
lsr_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		lsr_w_ea
lsr_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		lsr_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		LSR.L Dx,Dy
	//
entry static lsr_l_dx_dy
	CYCLES(10,10,3)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	subi	rRX,rRX,1							// RX -= 1
	beq		lsr_l_dx_dy_0						// special case for zero
	add		r0,rRX,rRX
	srw		r4,r3,rRX							// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	rlwimi	rSRFlags,r4,0,31,31					// C = copy of the last bit shifted out
	rlwinm	r4,r4,31,1,31						// shift the last bit out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
lsr_l_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		LSR.L Ix,Dy
	//
entry static lsr_l_ix_dy
	CYCLES(10,10,1)
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	subi	rRX,rRX,1							// rRX -= 1
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,7							// rRX = (rRX - 1) & 7
	srw		r4,r3,rRX							// r4 = r3 << rRX
	add		r0,rRX,rRX
	rlwimi	rSRFlags,r4,0,31,31					// C = copy of the last bit shifted out
	add		rCycleCount,rCycleCount,r0
	rlwinm	r4,r4,31,1,31						// shift the last bit out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================

	//
	//		illegal instruction handler
	//
	//		this is stuck in the middle of all the opcodes so that we can reach it from anywhere
	//
entry static illegal

	rlwinm	r0,r11,20,28,31						// r0 = _OPCODE >> 12
//	lwz		rPC,sOpcodePC(rtoc)					// get the original PC
	_asm_get_global(rPC,sOpcodePC)				// get the original PC
	cmpwi	r0,0x0f								// is it an 0xf000 opcode?
	cmpwi	cr1,r0,0x0a							// is it an 0xa000 opcode?
	li		r3,0x0b								// start with the F-trap exception
	beq		generateException					// do it if it was an 0xf000 opcode
	li		r3,0x0a								// now on to the A-trap exception
	beq		cr1,generateException				// do it if it was an 0xa000 opcode
	li		r3,0x04								// default to exception vector 4
#if (A68000_CHIP >= 68020)
#ifdef MAME_DEBUG
	// use this illegal instruction to find unimplemented opcodes
	rfi
#endif
#endif

	// deliberate fall through...

	//================================================================================================

	//
	//		generate an exception
	//
generateException:
	stw		rICount,0(rICountPtr)
//	stw		rSRFlags,sSRFlags(rtoc)
//	stw		rPC,sPC(rtoc)
	_asm_set_global(rSRFlags,sSRFlags)
	_asm_set_global(rPC,sPC)
	bl		GenerateException
	lwz		rOpcodeROM,0(rOpcodeROMPtr)
	lwz		rICount,0(rICountPtr)
//	lwz		rSRFlags,sSRFlags(rtoc)
//	lwz		rPC,sPC(rtoc)
	_asm_get_global(rSRFlags,sSRFlags)
	_asm_get_global(rPC,sPC)
	b		executeLoopEnd

	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================

	//================================================================================================
	//
	//		MOVEA -- Move to address register
	//
	//================================================================================================

	//
	//		MOVEA.W Ry,Ax
	//
entry static movea_w_ry_ax
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit offset
	READ_W_REG_EXT(r3,rRY)						// r3 = Dy (sign-extended)
	WRITE_L_AREG(r3,rRX)						// Ax = r3
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MOVEA.W (Ay),Ax
	//
entry static movea_w_ay0_ax
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movea_w_ea_ax:
	READ_W_AT(rEA)								// r3 = word(rEA)
movea_w_ea_ax_post:
	extsh	r3,r3								// r3 = ext(r3)
	WRITE_L_AREG(r3,rRX)						// Ax = r3
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MOVEA.W -(Ay),Ax
	//
entry static movea_w_aym_ax
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		movea_w_ea_ax
	
	//================================================================================================

	//
	//		MOVEA.W (Ay)+,Ax
	//
entry static movea_w_ayp_ax
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		movea_w_ea_ax
	
	//================================================================================================

	//
	//		MOVEA.W (Ay,d16),Ax
	//
entry static movea_w_ayd_ax
	CYCLES(12,12,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		movea_w_ea_ax
	
	//================================================================================================

	//
	//		MOVEA.W (Ay,Xn,d8),Ax
	//
entry static movea_w_ayid_ax
	bl		computeEA110
	CYCLES(14,14,0)
	b		movea_w_ea_ax

	//================================================================================================

	//
	//		MOVEA.W (xxx).W,Ax
	//		MOVEA.W (xxx).L,Ax
	//		MOVEA.W (d16,PC),Ax
	//		MOVEA.W (d8,PC,Xn),Ax
	//
entry static movea_w_other_ax
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		movea_w_ea_ax_post

	//================================================================================================
	//================================================================================================

	//
	//		MOVEA.L Ry,Ax
	//
entry static movea_l_ry_ax
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit offset
	READ_L_REG(r3,rRY)							// r3 = Dy
	WRITE_L_AREG(r3,rRX)						// Ax = r3
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MOVEA.L (Ay),Ax
	//
entry static movea_l_ay0_ax
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movea_l_ea_ax:
	READ_L_AT(rEA)								// r3 = long(rEA)
movea_l_ea_ax_post:
	WRITE_L_AREG(r3,rRX)						// Ax = r3
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MOVEA.L -(Ay),Ax
	//
entry static movea_l_aym_ax
	CYCLES(14,14,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		movea_l_ea_ax
	
	//================================================================================================

	//
	//		MOVEA.L (Ay)+,Ax
	//
entry static movea_l_ayp_ax
	CYCLES(12,12,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		movea_l_ea_ax
	
	//================================================================================================

	//
	//		MOVEA.L (Ay,d16),Ax
	//
entry static movea_l_ayd_ax
	CYCLES(16,16,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		movea_l_ea_ax
	
	//================================================================================================

	//
	//		MOVEA.L (Ay,Xn,d8),Ax
	//
entry static movea_l_ayid_ax
	bl		computeEA110
	CYCLES(18,18,0)
	b		movea_l_ea_ax

	//================================================================================================

	//
	//		MOVEA.L (xxx).W,Ax
	//		MOVEA.L (xxx).L,Ax
	//		MOVEA.L (d16,PC),Ax
	//		MOVEA.L (d8,PC,Xn),Ax
	//
entry static movea_l_other_ax
	bl		fetchEALong111_plus2
	CYCLES(12,12,0)
	b		movea_l_ea_ax_post

	//================================================================================================
	//
	//		MOVEC -- Move to control register
	//
	//================================================================================================

#if (A68000_CHIP >= 68010)

entry static movec
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not, exception time
	READ_OPCODE_ARG(rTempSave)					// register number == temp save
	andi.	r0,r11,1							// check direction
	rlwinm	rRX,rTempSave,22,26,29				// get register index in rRX
	rlwinm	rTempSave,rTempSave,0,20,31			// control register index in rTempSave
	cmpwi	cr1,rTempSave,0x000					// SFC register?
	cmpwi	cr2,rTempSave,0x001					// DFC register?
	cmpwi	cr3,rTempSave,0x800					// USP register?
	cmpwi	cr4,rTempSave,0x801					// VBR register?
	beq		movec_to_reg						// move to a general-purpose register
	CYCLES(0,10,3)
	READ_L_REG(r3,rRX)							// get value to store
	beq		cr1,movec_to_sfc
	beq		cr2,movec_to_dfc
	beq		cr3,movec_to_usp
	beq		cr4,movec_to_vbr

#if (A68000_CHIP >= 68020)
	cmpwi	cr1,rTempSave,0x002					// CACR register?
	cmpwi	cr2,rTempSave,0x802					// CAAR register?
	cmpwi	cr3,rTempSave,0x803					// MSP register?
	cmpwi	cr4,rTempSave,0x804					// ISP register?
	beq		cr1,movec_to_cacr
	beq		cr2,movec_to_caar
	beq		cr3,movec_to_msp
	beq		cr4,movec_to_isp
#endif

	b		illegal

movec_to_sfc:
//	stw		r3,sSFC(rtoc)
	_asm_set_global(r3,sSFC)
	b		executeLoopEnd
movec_to_dfc:
//	stw		r3,sDFC(rtoc)
	_asm_set_global(r3,sDFC)
	b		executeLoopEnd
movec_to_usp:
//	stw		r3,sUSP(rtoc)
	_asm_set_global(r3,sUSP)
	b		executeLoopEnd
movec_to_vbr:
//	stw		r3,sVBR(rtoc)
	_asm_set_global(r3,sVBR)
	b		executeLoopEnd

#if (A68000_CHIP >= 68020)
movec_to_cacr:
//	stw		r3,sCACR(rtoc)
	_asm_set_global(r3,sCACR)
	b		executeLoopEnd
movec_to_caar:
//	stw		r3,sCAAR(rtoc)
	_asm_set_global(r3,sCAAR)
	b		executeLoopEnd
movec_to_msp:
//	stw		r3,sMSP(rtoc)
	_asm_set_global(r3,sMSP)
	b		executeLoopEnd
movec_to_isp:
//	stw		r3,sISP(rtoc)
	_asm_set_global(r3,sISP)
	b		executeLoopEnd
#endif

movec_to_reg:
	CYCLES(0,12,9)
	beq		cr1,movec_from_sfc
	beq		cr2,movec_from_dfc
	beq		cr3,movec_from_usp
	beq		cr4,movec_from_vbr

#if (A68000_CHIP >= 68020)
	cmpwi	cr1,rTempSave,0x002					// CACR register?
	cmpwi	cr2,rTempSave,0x802					// CAAR register?
	cmpwi	cr3,rTempSave,0x803					// MSP register?
	cmpwi	cr4,rTempSave,0x804					// ISP register?
	beq		cr1,movec_from_cacr
	beq		cr2,movec_from_caar
	beq		cr3,movec_from_msp
	beq		cr4,movec_from_isp
#endif

	b		illegal

movec_from_sfc:
//	lwz		r3,sSFC(rtoc)
	_asm_get_global(r3,sSFC)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
movec_from_dfc:
//	lwz		r3,sDFC(rtoc)
	_asm_get_global(r3,sDFC)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
movec_from_usp:
//	lwz		r3,sUSP(rtoc)
	_asm_get_global(r3,sUSP)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
movec_from_vbr:
//	lwz		r3,sVBR(rtoc)
	_asm_get_global(r3,sVBR)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd

#if (A68000_CHIP >= 68020)
movec_from_cacr:
//	lwz		r3,sCACR(rtoc)
	_asm_get_global(r3,sCACR)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
movec_from_caar:
//	lwz		r3,sCAAR(rtoc)
	_asm_get_global(r3,sCAAR)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
movec_from_msp:
//	lwz		r3,sMSP(rtoc)
	_asm_get_global(r3,sMSP)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
movec_from_isp:
//	lwz		r3,sISP(rtoc)
	_asm_get_global(r3,sISP)
	WRITE_L_REG(r3,rRX)
	b		executeLoopEnd
#endif

#endif

	//================================================================================================
	//
	//		MOVEM -- Move multiple registers
	//
	//================================================================================================

	//
	//		MOVEM.W (Ay),regs
	//
entry static movem_w_ay0_reg
	CYCLES(12,12,10)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_w_ea_reg:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_w_ea_reg_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_w_ea_reg_skip					// if zero, skip
	READ_W_AT(rEA)								// read from EA
//	READ_W_AT_REL(rEA)							// read from EA
	addi	rEA,rEA,2							// rEA += 2
	extsh	r4,r3								// sign-extend the result
	stw		r4,0(rTempSave2)					// register value = r4
	CYCLES(4,4,4)								// 4 cycles per register
movem_w_ea_reg_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_w_ea_reg_loop					// loop over all registers
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.W (Ay)+,regs
	//
entry static movem_w_ayp_reg
	CYCLES(12,12,11)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_w_ayp_reg:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_w_ayp_reg_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_w_ayp_reg_skip				// if zero, skip
	READ_W_AT(rEA)								// read from EA
	addi	rEA,rEA,2							// rEA += 2
	extsh	r4,r3								// sign-extend the result
	stw		r4,0(rTempSave2)					// register value = r4
	CYCLES(4,4,4)								// 4 cycles per register
movem_w_ayp_reg_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_w_ayp_reg_loop				// loop over all registers
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.W (Ay,d16),regs
	//
entry static movem_w_ayd_reg
	CYCLES(16,16,10)
	READ_OPCODE_ARG(rRX)						// get the register mask
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		movem_w_ea_reg
	
	//================================================================================================

	//
	//		MOVEM.W (Ay,Xn,d8),regs
	//
entry static movem_w_ayid_reg
	READ_OPCODE_ARG(rRX)						// get the register mask
	bl		computeEA110
	CYCLES(18,18,8)
	b		movem_w_ea_reg

	//================================================================================================

	//
	//		MOVEM.W (xxx).W,regs
	//		MOVEM.W (xxx).L,regs
	//		MOVEM.W (d16,PC),regs
	//		MOVEM.W (d8,PC,Xn),regs
	//
entry static movem_w_other_reg
	READ_OPCODE_ARG(rRX)						// get the register mask
	bl		computeEA111
	CYCLES(12,12,10)
//	b		movem_w_ea_reg

movem_w_ea_reg_pcrel:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_w_ea_reg_loop_pcrel:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_w_ea_reg_skip_pcrel			// if zero, skip
//	READ_W_AT(rEA)								// read from EA
	READ_W_AT_REL(rEA)							// read from EA
	addi	rEA,rEA,2							// rEA += 2
	extsh	r4,r3								// sign-extend the result
	stw		r4,0(rTempSave2)					// register value = r4
	CYCLES(4,4,4)								// 4 cycles per register
movem_w_ea_reg_skip_pcrel:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_w_ea_reg_loop_pcrel			// loop over all registers
	b		executeLoopEndRestore				// return


	//================================================================================================
	//================================================================================================

	//
	//		MOVEM.L (Ay),regs
	//
entry static movem_l_ay0_reg
	CYCLES(12,12,10)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_l_ea_reg:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_l_ea_reg_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_l_ea_reg_skip					// if zero, skip
	READ_L_AT(rEA)								// read from EA
//	READ_L_AT_REL(rEA)							// read from EA
	addi	rEA,rEA,4							// rEA += 4
	stw		r3,0(rTempSave2)					// register value = r3
	CYCLES(8,8,4)								// 8 cycles per register
movem_l_ea_reg_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_l_ea_reg_loop					// loop over all registers
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.L (Ay)+,regs
	//
entry static movem_l_ayp_reg
	CYCLES(12,12,11)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_l_ayp_reg:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_l_ayp_reg_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_l_ayp_reg_skip				// if zero, skip
	READ_L_AT(rEA)								// read from EA
	addi	rEA,rEA,4							// rEA += 4
	stw		r3,0(rTempSave2)					// register value = r4
	CYCLES(8,8,4)								// 8 cycles per register
movem_l_ayp_reg_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_l_ayp_reg_loop				// loop over all registers
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.L (Ay,d16),regs
	//
entry static movem_l_ayd_reg
	CYCLES(16,16,10)
	READ_OPCODE_ARG(rRX)						// get the register mask
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		movem_l_ea_reg
	
	//================================================================================================

	//
	//		MOVEM.L (Ay,Xn,d8),regs
	//
entry static movem_l_ayid_reg
	READ_OPCODE_ARG(rRX)						// get the register mask
	bl		computeEA110
	CYCLES(18,18,8)
	b		movem_l_ea_reg

	//================================================================================================

	//
	//		MOVEM.L (xxx).W,regs
	//		MOVEM.L (xxx).L,regs
	//		MOVEM.L (d16,PC),regs
	//		MOVEM.L (d8,PC,Xn),regs
	//
entry static movem_l_other_reg
	READ_OPCODE_ARG(rRX)						// get the register mask
	bl		computeEA111
	CYCLES(12,12,8)
//	b		movem_l_ea_reg

movem_l_ea_reg_pcrel:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_l_ea_reg_loop_pcrel:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_l_ea_reg_skip_pcrel			// if zero, skip
//	READ_L_AT(rEA)								// read from EA
	READ_L_AT_REL(rEA)							// read from EA
	addi	rEA,rEA,4							// rEA += 4
	stw		r3,0(rTempSave2)					// register value = r3
	CYCLES(8,8,4)								// 8 cycles per register
movem_l_ea_reg_skip_pcrel:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_l_ea_reg_loop_pcrel			// loop over all registers
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVEM.W regs,(Ay)
	//
entry static movem_w_reg_ay0
	CYCLES(8,8,6)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_w_reg_ea:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_w_reg_ea_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_w_reg_ea_skip					// if zero, skip
	lwz		r4,0(rTempSave2)					// r4 = register value
	WRITE_W_AT(rEA)								// write to EA
	addi	rEA,rEA,2							// rEA += 2
	CYCLES(4,4,3)								// 4 cycles per register
movem_w_reg_ea_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_w_reg_ea_loop					// loop over all registers
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.W regs,-(Ay)
	//
entry static movem_w_reg_aym
	CYCLES(8,8,6)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_w_reg_aym:
	li		rTempSave,16						// rTempSave = 16
	addi	rTempSave2,rWordRegs,15*4			// rTempSave2 = pointer to A7
movem_w_reg_aym_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_w_reg_aym_skip				// if zero, skip
	lhz		r4,0(rTempSave2)					// r4 = register value
	subi	rEA,rEA,2							// rEA -= 2
	WRITE_W_AT(rEA)								// read from EA
	CYCLES(4,4,3)								// 4 cycles per register
movem_w_reg_aym_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	subi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_w_reg_aym_loop				// loop over all registers
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.W regs,(Ay,d16)
	//
entry static movem_w_reg_ayd
	CYCLES(12,12,6)
	READ_OPCODE_ARG(rRX)						// get the register mask
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		movem_w_reg_ea
	
	//================================================================================================

	//
	//		MOVEM.W regs,(Ay,Xn,d8)
	//
entry static movem_w_reg_ayid
	READ_OPCODE_ARG(rRX)						// get the register mask
	bl		computeEA110
	CYCLES(14,14,4)
	b		movem_w_reg_ea

	//================================================================================================

	//
	//		MOVEM.W regs,(xxx).W
	//		MOVEM.W regs,(xxx).L
	//
entry static movem_w_reg_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
movem_w_reg_abs:
	bne		movem_w_reg_absl					// handle long absolute mode
movem_w_reg_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(12,12,6)
	b		movem_w_reg_ea
movem_w_reg_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(16,16,6)
	b		movem_w_reg_ea

	//================================================================================================
	//================================================================================================

	//
	//		MOVEM.L regs,(Ay)
	//
entry static movem_l_reg_ay0
	CYCLES(8,8,6)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_l_reg_ea:
	li		rTempSave,16						// rTempSave = 16
	mr		rTempSave2,rLongRegs				// rTempSave2 = pointer to D0
movem_l_reg_ea_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_l_reg_ea_skip					// if zero, skip
	lwz		r4,0(rTempSave2)					// r4 = register value
	WRITE_L_AT(rEA)								// write to EA
	addi	rEA,rEA,4							// rEA += 4
	CYCLES(8,8,3)								// 8 cycles per register
movem_l_reg_ea_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	addi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_l_reg_ea_loop					// loop over all registers
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.L regs,-(Ay)
	//
entry static movem_l_reg_aym
	CYCLES(8,8,6)
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
movem_l_reg_aym:
	li		rTempSave,16						// rTempSave = 16
	addi	rTempSave2,rLongRegs,15*4			// rTempSave2 = pointer to A7
movem_l_reg_aym_loop:
	andi.	r0,rRX,1							// test the low bit
	rlwinm	rRX,rRX,31,0,31						// shift right one bit
	beq		movem_l_reg_aym_skip				// if zero, skip
	lwz		r4,0(rTempSave2)					// r4 = register value
	subi	rEA,rEA,4							// rEA -= 4
	WRITE_L_AT(rEA)								// read from EA
	CYCLES(8,8,3)								// 8 cycles per register
movem_l_reg_aym_skip:
	subic.	rTempSave,rTempSave,1				// decrement the counter
	subi	rTempSave2,rTempSave2,4				// point to next register
	bne		movem_l_reg_aym_loop				// loop over all registers
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEM.L regs,(Ay,d16)
	//
entry static movem_l_reg_ayd
	CYCLES(12,12,6)
	READ_OPCODE_ARG(rRX)						// get the register mask
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		movem_l_reg_ea
	
	//================================================================================================

	//
	//		MOVEM.L regs,(Ay,Xn,d8)
	//
entry static movem_l_reg_ayid
	READ_OPCODE_ARG(rRX)						// get the register mask
	bl		computeEA110
	CYCLES(14,14,4)
	b		movem_l_reg_ea

	//================================================================================================

	//
	//		MOVEM.L regs,(xxx).W
	//		MOVEM.L regs,(xxx).L
	//
entry static movem_l_reg_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG(rRX)						// get the register mask
	SAVE_SR
movem_l_reg_abs:
	bne		movem_l_reg_absl					// handle long absolute mode
movem_l_reg_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(12,12,6)
	b		movem_l_reg_ea
movem_l_reg_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(16,16,6)
	b		movem_l_reg_ea

	//================================================================================================
	//
	//		MOVEP -- Move peripheral data
	//
	//================================================================================================

	//
	//		MOVEP.W (Ay,d16),Dx
	//
entry static movep_w_ayd_dx
	CYCLES(16,16,10)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = ay
	SAVE_PC
	READ_OPCODE_ARG_EXT(r6)						// r6 = sign-extended offset
	add		rEA,rEA,r6							// rEA += r6
	READ_B_AT(rEA)								// r3 = byte(rEA)
	addi	rEA,rEA,2							// rEA += 2
	rlwinm	rTempSave,r3,8,16,23				// high byte = r3
	READ_B_AT(rEA)								// r3 = byte(rEA)
	rlwimi	rTempSave,r3,0,24,31				// rTempSave |= low byte
	WRITE_W_REG(rTempSave,rRX)					// Dx = rTempSave
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEP.W Dx,(Ay,d16)
	//
entry static movep_w_dx_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = ay
	SAVE_PC
	READ_OPCODE_ARG_EXT(r6)						// r6 = sign-extended offset
	READ_W_REG(rTempSave,rRX)					// rTempSave = Dx
	add		rEA,rEA,r6							// rEA += r6
	rlwinm	r4,rTempSave,24,24,31				// r4 = high 8 bits
	WRITE_B_AT(rEA)								// write it
	addi	rEA,rEA,2							// rEA += 2
	rlwinm	r4,rTempSave,0,24,31				// r4 = low 8 bits
	WRITE_B_AT(rEA)								// write it
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEP.L (Ay,d16),Dx
	//
entry static movep_l_ayd_dx
	CYCLES(24,24,16)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = ay
	SAVE_PC
	READ_OPCODE_ARG_EXT(r6)						// r6 = sign-extended offset
	add		rEA,rEA,r6							// rEA += r6
	READ_B_AT(rEA)								// r3 = byte(rEA)
	addi	rEA,rEA,2							// rEA += 2
	rlwinm	rTempSave,r3,24,0,7					// high byte = r3
	READ_B_AT(rEA)								// r3 = byte(rEA)
	addi	rEA,rEA,2							// rEA += 2
	rlwimi	rTempSave,r3,16,8,15				// rTempSave |= low byte
	READ_B_AT(rEA)								// r3 = byte(rEA)
	addi	rEA,rEA,2							// rEA += 2
	rlwimi	rTempSave,r3,8,16,23				// high byte = r3
	READ_B_AT(rEA)								// r3 = byte(rEA)
	rlwimi	rTempSave,r3,0,24,31				// rTempSave |= low byte
	WRITE_L_REG(rTempSave,rRX)					// Dx = rTempSave
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVEP.L Dx,(Ay,d16)
	//
entry static movep_l_dx_ayd
	CYCLES(24,24,14)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = ay
	SAVE_PC
	READ_OPCODE_ARG_EXT(r6)						// r6 = sign-extended offset
	READ_L_REG(rTempSave,rRX)					// rTempSave = Dx
	add		rEA,rEA,r6							// rEA += r6
	rlwinm	r4,rTempSave,8,24,31				// r4 = high 8 bits
	WRITE_B_AT(rEA)								// write it
	addi	rEA,rEA,2							// rEA += 2
	rlwinm	r4,rTempSave,16,24,31				// r4 = high 8 bits
	WRITE_B_AT(rEA)								// write it
	addi	rEA,rEA,2							// rEA += 2
	rlwinm	r4,rTempSave,24,24,31				// r4 = high 8 bits
	WRITE_B_AT(rEA)								// write it
	addi	rEA,rEA,2							// rEA += 2
	rlwinm	r4,rTempSave,0,24,31				// r4 = low 8 bits
	WRITE_B_AT(rEA)								// write it
	b		executeLoopEndRestore				// return

	//================================================================================================
	//
	//		MOVEQ -- Move 8-bit immediate to Dx
	//
	//================================================================================================

entry static moveq_imm_dx
	CYCLES(4,4,1)
	extsb	r3,r11								// r3 = sign-extended value
	CLR_VC										// V = C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r3 & 0x80)
	SET_Z(r7)									// Z = (r3 == 0)
	WRITE_L_REG(r3,rRX)							// write the result
	b		executeLoopEnd						// return
	
	//================================================================================================

	//
	//		supervisor mode exception generator
	//		here so that all routines can jump relative to it
	//
supervisorException:
	li		r3,8
	subi	rPC,rPC,2							// rPC -= 2
	b		generateException
	
	//================================================================================================
	//
	//		MOVE.B -- Move byte-sized data between two EA's
	//
	//================================================================================================

	//
	//		MOVE.B Dy,Dx
	//
entry static move_b_dy_dx
	CYCLES(4,4,1)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		MOVE.B Dy,(Ax)
	//
entry static move_b_dy_ax0
	CYCLES(8,8,3)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B Dy,-(Ax)
	//
entry static move_b_dy_axm
	CYCLES(8,8,3)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	WRITE_L_AREG(rEA,rRX)						// Ax = rEA
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B Dy,(Ax)+
	//
entry static move_b_dy_axp
	CYCLES(8,8,4)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// rEA += 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	WRITE_L_AREG(r3,rRX)						// Ax = rEA
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B Dy,(Ax,d16)
	//
entry static move_b_dy_axd
	CYCLES(12,12,3)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended offset
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	add		rEA,rEA,r3							// rEA += r3
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B Dy,(Ax,Xn,d8)
	//
entry static move_b_dy_axid
	CYCLES(14,14,0)
	READ_B_REG(rTempSave,rRY)					// rTempSave = Dy
	bl		computeEA110RX
	mr		r4,rTempSave						// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B Dy,(xxx).W
	//
entry static move_b_dy_memw
	CYCLES(12,12,3)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B Dy,(xxx).L
	//
entry static move_b_dy_meml
	CYCLES(16,16,5)
	READ_B_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.B (Ay),Dx
	//
entry static move_b_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_dx:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_dx_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),(Ax)
	//
entry static move_b_ay0_ax0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_ax0:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_ax0_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),-(Ax)
	//
entry static move_b_ay0_axm
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_axm:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_axm_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	WRITE_L_AREG(rEA,rRX)						// Ax = rEA
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),(Ax)+
	//
entry static move_b_ay0_axp
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_axp:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_axp_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// rEA += 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	WRITE_L_AREG(r3,rRX)						// Ax = rEA
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),(Ax,d16)
	//
entry static move_b_ay0_axd
	CYCLES(16,16,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_axd:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_axd_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended offset
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	add		rEA,rEA,r3							// rEA += r3
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),(Ax,Xn,d8)
	//
entry static move_b_ay0_axid
	CYCLES(18,18,3)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_axid:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_axid_post:
	TRUNC_BYTE_TO(rTempSave,r3)					// rTempSave = byte(r3)
	bl		computeEA110RX
	mr		r4,rTempSave
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),(xxx).W
	//
entry static move_b_ay0_memw
	CYCLES(16,16,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_memw:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_memw_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.B (Ay),(xxx).L
	//
entry static move_b_ay0_meml
	CYCLES(20,20,8)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_b_ea_meml:
	READ_B_AT(r3)								// r3 = (Ay)
move_b_ea_meml_post:
	TRUNC_BYTE_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_SR
	WRITE_B_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.B -(Ay),Dx
	//
entry static move_b_aym_dx
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_dx

	//================================================================================================

	//
	//		MOVE.B -(Ay),(Ax)
	//
entry static move_b_aym_ax0
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_ax0

	//================================================================================================

	//
	//		MOVE.B -(Ay),-(Ax)
	//
entry static move_b_aym_axm
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_axm

	//================================================================================================

	//
	//		MOVE.B -(Ay),(Ax)+
	//
entry static move_b_aym_axp
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_axp

	//================================================================================================

	//
	//		MOVE.B -(Ay),(Ax,d16)
	//
entry static move_b_aym_axd
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_axd

	//================================================================================================

	//
	//		MOVE.B -(Ay),(Ax,Xn,d8)
	//
entry static move_b_aym_axid
	CYCLES(20,20,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_axid

	//================================================================================================

	//
	//		MOVE.B -(Ay),(xxx).W
	//
entry static move_b_aym_memw
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_memw

	//================================================================================================

	//
	//		MOVE.B -(Ay),(xxx).L
	//
entry static move_b_aym_meml
	CYCLES(22,22,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	r3,r3,1								// r3 -= 1
	sub		r3,r3,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_b_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.B (Ay)+,Dx
	//
entry static move_b_ayp_dx
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_dx

	//================================================================================================

	//
	//		MOVE.B (Ay)+,(Ax)
	//
entry static move_b_ayp_ax0
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_ax0

	//================================================================================================

	//
	//		MOVE.B (Ay)+,-(Ax)
	//
entry static move_b_ayp_axm
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_axm

	//================================================================================================

	//
	//		MOVE.B (Ay)+,(Ax)+
	//
entry static move_b_ayp_axp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_axp

	//================================================================================================

	//
	//		MOVE.B (Ay)+,(Ax,d16)
	//
entry static move_b_ayp_axd
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_axd

	//================================================================================================

	//
	//		MOVE.B (Ay)+,(Ax,Xn,d8)
	//
entry static move_b_ayp_axid
	CYCLES(18,18,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_axid

	//================================================================================================

	//
	//		MOVE.B (Ay)+,(xxx).W
	//
entry static move_b_ayp_memw
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_memw

	//================================================================================================

	//
	//		MOVE.B (Ay)+,(xxx).L
	//
entry static move_b_ayp_meml
	CYCLES(20,20,9)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r4,r3,1								// r4 = r3 + 1
	add		r4,r4,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_b_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.B (Ay,d16),Dx
	//
entry static move_b_ayd_dx
	CYCLES(12,12,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_dx

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),(Ax)
	//
entry static move_b_ayd_ax0
	CYCLES(16,16,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_ax0

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),-(Ax)
	//
entry static move_b_ayd_axm
	CYCLES(16,16,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_axm

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),(Ax)+
	//
entry static move_b_ayd_axp
	CYCLES(16,16,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_axp

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),(Ax,d16)
	//
entry static move_b_ayd_axd
	CYCLES(20,20,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_axd

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),(Ax,Xn,d8)
	//
entry static move_b_ayd_axid
	CYCLES(22,22,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_axid

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),(xxx).W
	//
entry static move_b_ayd_memw
	CYCLES(20,20,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_memw

	//================================================================================================

	//
	//		MOVE.B (Ay,d16),(xxx).L
	//
entry static move_b_ayd_meml
	CYCLES(24,24,8)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_b_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),Dx
	//
entry static move_b_ayid_dx
	bl		computeEA110
	CYCLES(14,14,1)
	mr		r3,rEA
	b		move_b_ea_dx

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),(Ax)
	//
entry static move_b_ayid_ax0
	bl		computeEA110
	CYCLES(18,18,4)
	mr		r3,rEA
	b		move_b_ea_ax0

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),-(Ax)
	//
entry static move_b_ayid_axm
	bl		computeEA110
	CYCLES(18,18,4)
	mr		r3,rEA
	b		move_b_ea_axm

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),(Ax)+
	//
entry static move_b_ayid_axp
	bl		computeEA110
	CYCLES(18,18,4)
	mr		r3,rEA
	b		move_b_ea_axp

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),(Ax,d16)
	//
entry static move_b_ayid_axd
	bl		computeEA110
	CYCLES(22,22,4)
	mr		r3,rEA
	b		move_b_ea_axd

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),(Ax,Xn,d8)
	//
entry static move_b_ayid_axid
	bl		computeEA110
	CYCLES(24,24,1)
	mr		r3,rEA
	b		move_b_ea_axid

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),(xxx).W
	//
entry static move_b_ayid_memw
	bl		computeEA110
	CYCLES(22,22,4)
	mr		r3,rEA
	b		move_b_ea_memw

	//================================================================================================

	//
	//		MOVE.B (Ay,Xn,d8),(xxx).L
	//
entry static move_b_ayid_meml
	bl		computeEA110
	CYCLES(26,26,6)
	mr		r3,rEA
	b		move_b_ea_meml

	//================================================================================================

	//
	//		MOVE.B (xxx).W,Dx
	//		MOVE.B (xxx).L,Dx
	//		MOVE.B #xxx,Dx
	//		MOVE.B (PC,d16),Dx
	//		MOVE.B (PC,Xn,d8),Dx
	//
entry static move_b_other_dx
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		move_b_ea_dx_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,(Ax)
	//		MOVE.B (xxx).L,(Ax)
	//		MOVE.B #xxx,(Ax)
	//		MOVE.B (PC,d16),(Ax)
	//		MOVE.B (PC,Xn,d8),(Ax)
	//
entry static move_b_other_ax0
	bl		fetchEAByte111
	CYCLES(12,12,3)
	b		move_b_ea_ax0_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,-(Ax)
	//		MOVE.B (xxx).L,-(Ax)
	//		MOVE.B #xxx,-(Ax)
	//		MOVE.B (PC,d16),-(Ax)
	//		MOVE.B (PC,Xn,d8),-(Ax)
	//
entry static move_b_other_axm
	bl		fetchEAByte111
	CYCLES(12,12,3)
	b		move_b_ea_axm_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,(Ax)+
	//		MOVE.B (xxx).L,(Ax)+
	//		MOVE.B #xxx,(Ax)+
	//		MOVE.B (PC,d16),(Ax)+
	//		MOVE.B (PC,Xn,d8),(Ax)+
	//
entry static move_b_other_axp
	bl		fetchEAByte111
	CYCLES(12,12,3)
	b		move_b_ea_axp_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,(Ax,d16)
	//		MOVE.B (xxx).L,(Ax,d16)
	//		MOVE.B #xxx,(Ax,d16)
	//		MOVE.B (PC,d16),(Ax,d16)
	//		MOVE.B (PC,Xn,d8),(Ax,d16)
	//
entry static move_b_other_axd
	bl		fetchEAByte111
	CYCLES(16,16,3)
	b		move_b_ea_axd_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,(Ax,Xn,d8)
	//		MOVE.B (xxx).L,(Ax,Xn,d8)
	//		MOVE.B #xxx,(Ax,Xn,d8)
	//		MOVE.B (PC,d16),(Ax,Xn,d8)
	//		MOVE.B (PC,Xn,d8),(Ax,Xn,d8)
	//
entry static move_b_other_axid
	bl		fetchEAByte111
	CYCLES(18,18,0)
	b		move_b_ea_axid_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,(xxx).W
	//		MOVE.B (xxx).L,(xxx).W
	//		MOVE.B #xxx,(xxx).W
	//		MOVE.B (PC,d16),(xxx).W
	//		MOVE.B (PC,Xn,d8),(xxx).W
	//
entry static move_b_other_memw
	bl		fetchEAByte111
	CYCLES(16,16,3)
	b		move_b_ea_memw_post

	//================================================================================================

	//
	//		MOVE.B (xxx).W,(xxx).L
	//		MOVE.B (xxx).L,(xxx).L
	//		MOVE.B #xxx,(xxx).L
	//		MOVE.B (PC,d16),(xxx).L
	//		MOVE.B (PC,Xn,d8),(xxx).L
	//
entry static move_b_other_meml
	bl		fetchEAByte111
	CYCLES(20,20,5)
	b		move_b_ea_meml_post

	//================================================================================================
	//
	//		MOVE.W -- Move word-sized data between two EA's
	//
	//================================================================================================

	//
	//		MOVE.W Dy,Dx
	//
entry static move_w_ry_dx
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		MOVE.W Dy,(Ax)
	//
entry static move_w_ry_ax0
	CYCLES(8,8,3)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W Dy,-(Ax)
	//
entry static move_w_ry_axm
	CYCLES(8,8,3)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	WRITE_L_AREG(rEA,rRX)						// Ax = rEA
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W Dy,(Ax)+
	//
entry static move_w_ry_axp
	CYCLES(8,8,3)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	addi	r3,rEA,2							// rEA += 2
	WRITE_L_AREG(r3,rRX)						// Ax = rEA
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W Dy,(Ax,d16)
	//
entry static move_w_ry_axd
	CYCLES(12,12,3)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended offset
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	add		rEA,rEA,r3							// rEA += r3
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W Dy,(Ax,Xn,d8)
	//
entry static move_w_ry_axid
	CYCLES(14,14,0)
	REG_OFFSET_LO_4(rTempSave)					// get the full 4-bit offset
	bl		computeEA110RX
	READ_W_REG(r4,rTempSave)					// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W Dy,(xxx).W
	//
entry static move_w_ry_memw
	CYCLES(12,12,3)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W Dy,(xxx).L
	//
entry static move_w_ry_meml
	CYCLES(16,16,5)
	REG_OFFSET_LO_4(rRY)						// get the full 4-bit offset
	READ_W_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.W (Ay),Dx
	//
entry static move_w_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_dx:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_dx_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),(Ax)
	//
entry static move_w_ay0_ax0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_ax0:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_ax0_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),-(Ax)
	//
entry static move_w_ay0_axm
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_axm:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_axm_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	WRITE_L_AREG(rEA,rRX)						// Ax = rEA
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),(Ax)+
	//
entry static move_w_ay0_axp
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_axp:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_axp_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	addi	r3,rEA,2							// rEA += 2
	WRITE_L_AREG(r3,rRX)						// Ax = rEA
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),(Ax,d16)
	//
entry static move_w_ay0_axd
	CYCLES(16,16,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_axd:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_axd_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended offset
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	add		rEA,rEA,r3							// rEA += r3
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),(Ax,Xn,d8)
	//
entry static move_w_ay0_axid
	CYCLES(18,18,3)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_axid:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_axid_post:
	TRUNC_WORD_TO(rTempSave,r3)					// rTempSave = byte(r3)
	bl		computeEA110RX
	mr		r4,rTempSave
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),(xxx).W
	//
entry static move_w_ay0_memw
	CYCLES(16,16,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_memw:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_memw_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.W (Ay),(xxx).L
	//
entry static move_w_ay0_meml
	CYCLES(20,20,8)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_w_ea_meml:
	READ_W_AT(r3)								// r3 = (Ay)
move_w_ea_meml_post:
	TRUNC_WORD_TO(r4,r3)						// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_SR
	WRITE_W_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.W -(Ay),Dx
	//
entry static move_w_aym_dx
	CYCLES(10,10,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_dx

	//================================================================================================

	//
	//		MOVE.W -(Ay),(Ax)
	//
entry static move_w_aym_ax0
	CYCLES(14,14,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_ax0

	//================================================================================================

	//
	//		MOVE.W -(Ay),-(Ax)
	//
entry static move_w_aym_axm
	CYCLES(14,14,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_axm

	//================================================================================================

	//
	//		MOVE.W -(Ay),(Ax)+
	//
entry static move_w_aym_axp
	CYCLES(14,14,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_axp

	//================================================================================================

	//
	//		MOVE.W -(Ay),(Ax,d16)
	//
entry static move_w_aym_axd
	CYCLES(18,18,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_axd

	//================================================================================================

	//
	//		MOVE.W -(Ay),(Ax,Xn,d8)
	//
entry static move_w_aym_axid
	CYCLES(20,20,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_axid

	//================================================================================================

	//
	//		MOVE.W -(Ay),(xxx).W
	//
entry static move_w_aym_memw
	CYCLES(18,18,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_memw

	//================================================================================================

	//
	//		MOVE.W -(Ay),(xxx).L
	//
entry static move_w_aym_meml
	CYCLES(22,22,8)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,2								// r3 -= 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.W (Ay)+,Dx
	//
entry static move_w_ayp_dx
	CYCLES(8,8,4)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_dx

	//================================================================================================

	//
	//		MOVE.W (Ay)+,(Ax)
	//
entry static move_w_ayp_ax0
	CYCLES(12,12,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_ax0

	//================================================================================================

	//
	//		MOVE.W (Ay)+,-(Ax)
	//
entry static move_w_ayp_axm
	CYCLES(12,12,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_axm

	//================================================================================================

	//
	//		MOVE.W (Ay)+,(Ax)+
	//
entry static move_w_ayp_axp
	CYCLES(12,12,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_axp

	//================================================================================================

	//
	//		MOVE.W (Ay)+,(Ax,d16)
	//
entry static move_w_ayp_axd
	CYCLES(16,16,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_axd

	//================================================================================================

	//
	//		MOVE.W (Ay)+,(Ax,Xn,d8)
	//
entry static move_w_ayp_axid
	CYCLES(18,18,4)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_axid

	//================================================================================================

	//
	//		MOVE.W (Ay)+,(xxx).W
	//
entry static move_w_ayp_memw
	CYCLES(16,16,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_memw

	//================================================================================================

	//
	//		MOVE.W (Ay)+,(xxx).L
	//
entry static move_w_ayp_meml
	CYCLES(20,20,9)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,2								// r4 = r3 + 2
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_w_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.W (Ay,d16),Dx
	//
entry static move_w_ayd_dx
	CYCLES(12,12,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_dx

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),(Ax)
	//
entry static move_w_ayd_ax0
	CYCLES(16,16,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_ax0

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),-(Ax)
	//
entry static move_w_ayd_axm
	CYCLES(16,16,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_axm

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),(Ax)+
	//
entry static move_w_ayd_axp
	CYCLES(16,16,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_axp

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),(Ax,d16)
	//
entry static move_w_ayd_axd
	CYCLES(20,20,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_axd

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),(Ax,Xn,d8)
	//
entry static move_w_ayd_axid
	CYCLES(22,22,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_axid

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),(xxx).W
	//
entry static move_w_ayd_memw
	CYCLES(20,20,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_memw

	//================================================================================================

	//
	//		MOVE.W (Ay,d16),(xxx).L
	//
entry static move_w_ayd_meml
	CYCLES(24,24,8)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_w_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),Dx
	//
entry static move_w_ayid_dx
	bl		computeEA110
	CYCLES(14,14,1)
	mr		r3,rEA
	b		move_w_ea_dx

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),(Ax)
	//
entry static move_w_ayid_ax0
	bl		computeEA110
	CYCLES(18,18,4)
	mr		r3,rEA
	b		move_w_ea_ax0

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),-(Ax)
	//
entry static move_w_ayid_axm
	bl		computeEA110
	CYCLES(18,18,4)
	mr		r3,rEA
	b		move_w_ea_axm

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),(Ax)+
	//
entry static move_w_ayid_axp
	bl		computeEA110
	CYCLES(18,18,4)
	mr		r3,rEA
	b		move_w_ea_axp

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),(Ax,d16)
	//
entry static move_w_ayid_axd
	bl		computeEA110
	CYCLES(22,22,4)
	mr		r3,rEA
	b		move_w_ea_axd

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),(Ax,Xn,d8)
	//
entry static move_w_ayid_axid
	bl		computeEA110
	CYCLES(24,24,1)
	mr		r3,rEA
	b		move_w_ea_axid

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),(xxx).W
	//
entry static move_w_ayid_memw
	bl		computeEA110
	CYCLES(22,22,4)
	mr		r3,rEA
	b		move_w_ea_memw

	//================================================================================================

	//
	//		MOVE.W (Ay,Xn,d8),(xxx).L
	//
entry static move_w_ayid_meml
	bl		computeEA110
	CYCLES(26,26,6)
	mr		r3,rEA
	b		move_w_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.W (xxx).W,Dx
	//		MOVE.W (xxx).L,Dx
	//		MOVE.W #xxx,Dx
	//		MOVE.W (PC,d16),Dx
	//		MOVE.W (PC,Xn,d8),Dx
	//
entry static move_w_other_dx
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		move_w_ea_dx_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,(Ax)
	//		MOVE.W (xxx).L,(Ax)
	//		MOVE.W #xxx,(Ax)
	//		MOVE.W (PC,d16),(Ax)
	//		MOVE.W (PC,Xn,d8),(Ax)
	//
entry static move_w_other_ax0
	bl		fetchEAWord111
	CYCLES(12,12,3)
	b		move_w_ea_ax0_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,-(Ax)
	//		MOVE.W (xxx).L,-(Ax)
	//		MOVE.W #xxx,-(Ax)
	//		MOVE.W (PC,d16),-(Ax)
	//		MOVE.W (PC,Xn,d8),-(Ax)
	//
entry static move_w_other_axm
	bl		fetchEAWord111
	CYCLES(12,12,3)
	b		move_w_ea_axm_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,(Ax)+
	//		MOVE.W (xxx).L,(Ax)+
	//		MOVE.W #xxx,(Ax)+
	//		MOVE.W (PC,d16),(Ax)+
	//		MOVE.W (PC,Xn,d8),(Ax)+
	//
entry static move_w_other_axp
	bl		fetchEAWord111
	CYCLES(12,12,3)
	b		move_w_ea_axp_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,(Ax,d16)
	//		MOVE.W (xxx).L,(Ax,d16)
	//		MOVE.W #xxx,(Ax,d16)
	//		MOVE.W (PC,d16),(Ax,d16)
	//		MOVE.W (PC,Xn,d8),(Ax,d16)
	//
entry static move_w_other_axd
	bl		fetchEAWord111
	CYCLES(16,16,3)
	b		move_w_ea_axd_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,(Ax,Xn,d8)
	//		MOVE.W (xxx).L,(Ax,Xn,d8)
	//		MOVE.W #xxx,(Ax,Xn,d8)
	//		MOVE.W (PC,d16),(Ax,Xn,d8)
	//		MOVE.W (PC,Xn,d8),(Ax,Xn,d8)
	//
entry static move_w_other_axid
	bl		fetchEAWord111
	CYCLES(18,18,0)
	b		move_w_ea_axid_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,(xxx).W
	//		MOVE.W (xxx).L,(xxx).W
	//		MOVE.W #xxx,(xxx).W
	//		MOVE.W (PC,d16),(xxx).W
	//		MOVE.W (PC,Xn,d8),(xxx).W
	//
entry static move_w_other_memw
	bl		fetchEAWord111
	CYCLES(16,16,3)
	b		move_w_ea_memw_post

	//================================================================================================

	//
	//		MOVE.W (xxx).W,(xxx).L
	//		MOVE.W (xxx).L,(xxx).L
	//		MOVE.W #xxx,(xxx).L
	//		MOVE.W (PC,d16),(xxx).L
	//		MOVE.W (PC,Xn,d8),(xxx).L
	//
entry static move_w_other_meml
	bl		fetchEAWord111
	CYCLES(20,20,5)
	b		move_w_ea_meml_post

	//================================================================================================
	//
	//		MOVE.L -- Move long-sized data between two EA's
	//
	//================================================================================================

	//
	//		MOVE.L Dy,Dx
	//
entry static move_l_ry_dx
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		MOVE.L Dy,(Ax)
	//
entry static move_l_ry_ax0
	CYCLES(12,12,3)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L Dy,-(Ax)
	//
entry static move_l_ry_axm
	CYCLES(12,12,3)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	WRITE_L_AREG(rEA,rRX)						// Ax = rEA
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L Dy,(Ax)+
	//
entry static move_l_ry_axp
	CYCLES(12,12,3)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	addi	r3,rEA,4							// rEA += 4
	WRITE_L_AREG(r3,rRX)						// Ax = rEA
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L Dy,(Ax,d16)
	//
entry static move_l_ry_axd
	CYCLES(16,16,3)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended offset
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	add		rEA,rEA,r3							// rEA += r3
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L Dy,(Ax,Xn,d8)
	//
entry static move_l_ry_axid
	CYCLES(14,14,0)
	REG_OFFSET_LO_4(rTempSave)					// get the full 4-bit offset
	bl		computeEA110RX
	READ_L_REG(r4,rTempSave)					// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L Dy,(xxx).W
	//
entry static move_l_ry_memw
	CYCLES(16,16,3)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L Dy,(xxx).L
	//
entry static move_l_ry_meml
	CYCLES(20,20,5)
	REG_OFFSET_LO_4(rRY)						// get full 4-bit register
	READ_L_REG(r4,rRY)							// r4 = Dy
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.L (Ay),Dx
	//
entry static move_l_ay0_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_dx:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_dx_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),(Ax)
	//
entry static move_l_ay0_ax0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_ax0:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_ax0_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),-(Ax)
	//
entry static move_l_ay0_axm
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_axm:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_axm_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	WRITE_L_AREG(rEA,rRX)						// Ax = rEA
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),(Ax)+
	//
entry static move_l_ay0_axp
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_axp:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_axp_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	addi	r3,rEA,4							// rEA += 4
	WRITE_L_AREG(r3,rRX)						// Ax = rEA
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),(Ax,d16)
	//
entry static move_l_ay0_axd
	CYCLES(24,24,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_axd:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_axd_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended offset
	SET_Z(r7)									// Z = (r4 == 0)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	SAVE_SR
	add		rEA,rEA,r3							// rEA += r3
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),(Ax,Xn,d8)
	//
entry static move_l_ay0_axid
	CYCLES(26,26,3)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_axid:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_axid_post:
	mr		rTempSave,r3						// rTempSave = r3
	bl		computeEA110RX
	mr		r4,rTempSave
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SAVE_PC
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),(xxx).W
	//
entry static move_l_ay0_memw
	CYCLES(24,24,6)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_memw:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_memw_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		MOVE.L (Ay),(xxx).L
	//
entry static move_l_ay0_meml
	CYCLES(28,28,8)
	SAVE_SR
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_PC
move_l_ea_meml:
	READ_L_AT(r3)								// r3 = (Ay)
move_l_ea_meml_post:
	mr		r4,r3								// r4 = byte(r3)
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_SR
	WRITE_L_AT(rEA)								// write the result
	b		executeLoopEndRestore				// return

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.L -(Ay),Dx
	//
entry static move_l_aym_dx
	CYCLES(14,14,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_dx

	//================================================================================================

	//
	//		MOVE.L -(Ay),(Ax)
	//
entry static move_l_aym_ax0
	CYCLES(22,22,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_ax0

	//================================================================================================

	//
	//		MOVE.L -(Ay),-(Ax)
	//
entry static move_l_aym_axm
	CYCLES(22,22,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_axm

	//================================================================================================

	//
	//		MOVE.L -(Ay),(Ax)+
	//
entry static move_l_aym_axp
	CYCLES(22,22,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_axp

	//================================================================================================

	//
	//		MOVE.L -(Ay),(Ax,d16)
	//
entry static move_l_aym_axd
	CYCLES(26,26,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_axd

	//================================================================================================

	//
	//		MOVE.L -(Ay),(Ax,Xn,d8)
	//
entry static move_l_aym_axid
	CYCLES(28,28,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_axid

	//================================================================================================

	//
	//		MOVE.L -(Ay),(xxx).W
	//
entry static move_l_aym_memw
	CYCLES(26,26,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_memw

	//================================================================================================

	//
	//		MOVE.L -(Ay),(xxx).L
	//
entry static move_l_aym_meml
	CYCLES(30,30,8)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	subi	r3,r3,4								// r3 -= 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_l_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.L (Ay)+,Dx
	//
entry static move_l_ayp_dx
	CYCLES(12,12,4)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_dx

	//================================================================================================

	//
	//		MOVE.L (Ay)+,(Ax)
	//
entry static move_l_ayp_ax0
	CYCLES(20,20,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_ax0

	//================================================================================================

	//
	//		MOVE.L (Ay)+,-(Ax)
	//
entry static move_l_ayp_axm
	CYCLES(20,20,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_axm

	//================================================================================================

	//
	//		MOVE.L (Ay)+,(Ax)+
	//
entry static move_l_ayp_axp
	CYCLES(20,20,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_axp

	//================================================================================================

	//
	//		MOVE.L (Ay)+,(Ax,d16)
	//
entry static move_l_ayp_axd
	CYCLES(24,24,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_axd

	//================================================================================================

	//
	//		MOVE.L (Ay)+,(Ax,Xn,d8)
	//
entry static move_l_ayp_axid
	CYCLES(26,26,4)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_axid

	//================================================================================================

	//
	//		MOVE.L (Ay)+,(xxx).W
	//
entry static move_l_ayp_memw
	CYCLES(24,24,7)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_memw

	//================================================================================================

	//
	//		MOVE.L (Ay)+,(xxx).L
	//
entry static move_l_ayp_meml
	CYCLES(28,28,9)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	addi	r4,r3,4								// r4 = r3 + 4
	SAVE_PC
	WRITE_L_AREG(r4,rRY)						// Ay = r3
	b		move_l_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.L (Ay,d16),Dx
	//
entry static move_l_ayd_dx
	CYCLES(16,16,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_dx

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),(Ax)
	//
entry static move_l_ayd_ax0
	CYCLES(24,24,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_ax0

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),-(Ax)
	//
entry static move_l_ayd_axm
	CYCLES(24,24,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_axm

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),(Ax)+
	//
entry static move_l_ayd_axp
	CYCLES(24,24,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_axp

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),(Ax,d16)
	//
entry static move_l_ayd_axd
	CYCLES(28,28,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_axd

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),(Ax,Xn,d8)
	//
entry static move_l_ayd_axid
	CYCLES(30,30,3)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_axid

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),(xxx).W
	//
entry static move_l_ayd_memw
	CYCLES(28,28,6)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_memw

	//================================================================================================

	//
	//		MOVE.L (Ay,d16),(xxx).L
	//
entry static move_l_ayd_meml
	CYCLES(32,32,8)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	SAVE_SR
	READ_OPCODE_ARG_EXT(r4)						// r4 = sign-extended word
	SAVE_PC
	add		r3,r3,r4							// r3 = Ay + d16
	b		move_l_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),Dx
	//
entry static move_l_ayid_dx
	bl		computeEA110
	CYCLES(18,18,1)
	mr		r3,rEA
	b		move_l_ea_dx

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),(Ax)
	//
entry static move_l_ayid_ax0
	bl		computeEA110
	CYCLES(26,26,4)
	mr		r3,rEA
	b		move_l_ea_ax0

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),-(Ax)
	//
entry static move_l_ayid_axm
	bl		computeEA110
	CYCLES(26,26,4)
	mr		r3,rEA
	b		move_l_ea_axm

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),(Ax)+
	//
entry static move_l_ayid_axp
	bl		computeEA110
	CYCLES(26,26,4)
	mr		r3,rEA
	b		move_l_ea_axp

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),(Ax,d16)
	//
entry static move_l_ayid_axd
	bl		computeEA110
	CYCLES(30,30,4)
	mr		r3,rEA
	b		move_l_ea_axd

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),(Ax,Xn,d8)
	//
entry static move_l_ayid_axid
	bl		computeEA110
	CYCLES(32,32,1)
	mr		r3,rEA
	b		move_l_ea_axid

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),(xxx).W
	//
entry static move_l_ayid_memw
	bl		computeEA110
	CYCLES(30,30,4)
	mr		r3,rEA
	b		move_l_ea_memw

	//================================================================================================

	//
	//		MOVE.L (Ay,Xn,d8),(xxx).L
	//
entry static move_l_ayid_meml
	bl		computeEA110
	CYCLES(34,34,7)
	mr		r3,rEA
	b		move_l_ea_meml

	//================================================================================================
	//================================================================================================

	//
	//		MOVE.L (xxx).W,Dx
	//		MOVE.L (xxx).L,Dx
	//		MOVE.L #xxx,Dx
	//		MOVE.L (PC,d16),Dx
	//		MOVE.L (PC,Xn,d8),Dx
	//
entry static move_l_other_dx
	bl		fetchEALong111_plus2
	CYCLES(12,12,0)
	b		move_l_ea_dx_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,(Ax)
	//		MOVE.L (xxx).L,(Ax)
	//		MOVE.L #xxx,(Ax)
	//		MOVE.L (PC,d16),(Ax)
	//		MOVE.L (PC,Xn,d8),(Ax)
	//
entry static move_l_other_ax0
	bl		fetchEALong111_plus2
	CYCLES(20,20,3)
	b		move_l_ea_ax0_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,-(Ax)
	//		MOVE.L (xxx).L,-(Ax)
	//		MOVE.L #xxx,-(Ax)
	//		MOVE.L (PC,d16),-(Ax)
	//		MOVE.L (PC,Xn,d8),-(Ax)
	//
entry static move_l_other_axm
	bl		fetchEALong111_plus2
	CYCLES(20,20,3)
	b		move_l_ea_axm_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,(Ax)+
	//		MOVE.L (xxx).L,(Ax)+
	//		MOVE.L #xxx,(Ax)+
	//		MOVE.L (PC,d16),(Ax)+
	//		MOVE.L (PC,Xn,d8),(Ax)+
	//
entry static move_l_other_axp
	bl		fetchEALong111_plus2
	CYCLES(20,20,3)
	b		move_l_ea_axp_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,(Ax,d16)
	//		MOVE.L (xxx).L,(Ax,d16)
	//		MOVE.L #xxx,(Ax,d16)
	//		MOVE.L (PC,d16),(Ax,d16)
	//		MOVE.L (PC,Xn,d8),(Ax,d16)
	//
entry static move_l_other_axd
	bl		fetchEALong111_plus2
	CYCLES(24,24,3)
	b		move_l_ea_axd_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,(Ax,Xn,d8)
	//		MOVE.L (xxx).L,(Ax,Xn,d8)
	//		MOVE.L #xxx,(Ax,Xn,d8)
	//		MOVE.L (PC,d16),(Ax,Xn,d8)
	//		MOVE.L (PC,Xn,d8),(Ax,Xn,d8)
	//
entry static move_l_other_axid
	bl		fetchEALong111_plus2
	CYCLES(26,26,0)
	b		move_l_ea_axid_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,(xxx).W
	//		MOVE.L (xxx).L,(xxx).W
	//		MOVE.L #xxx,(xxx).W
	//		MOVE.L (PC,d16),(xxx).W
	//		MOVE.L (PC,Xn,d8),(xxx).W
	//
entry static move_l_other_memw
	bl		fetchEALong111_plus2
	CYCLES(24,24,3)
	b		move_l_ea_memw_post

	//================================================================================================

	//
	//		MOVE.L (xxx).W,(xxx).L
	//		MOVE.L (xxx).L,(xxx).L
	//		MOVE.L #xxx,(xxx).L
	//		MOVE.L (PC,d16),(xxx).L
	//		MOVE.L (PC,Xn,d8),(xxx).L
	//
entry static move_l_other_meml
	bl		fetchEALong111_plus2
	CYCLES(28,28,5)
	b		move_l_ea_meml_post

	//================================================================================================
	//
	//		MOVE to/from CCR -- Move data into/out of the CCR
	//
	//================================================================================================

	//
	//		MOVE Dy,CCR
	//
entry static move_w_dy_ccr
	CYCLES(12,12,4)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit offset
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rSRFlags,r3,0,24,31					// insert the low 8 bits
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MOVE (Ay),CCR
	//
entry static move_w_ay0_ccr
	CYCLES(16,16,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
move_w_ea_ccr:
	READ_W_AT(rEA)								// r3 = word(rEA)
move_w_ea_ccr_post:
	rlwimi	rSRFlags,r3,0,24,31					// insert the low 8 bits
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MOVE -(Ay),CCR
	//
entry static move_w_aym_ccr
	CYCLES(18,18,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		move_w_ea_ccr
	
	//================================================================================================

	//
	//		MOVE (Ay)+,CCR
	//
entry static move_w_ayp_ccr
	CYCLES(16,16,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_ccr
	
	//================================================================================================

	//
	//		MOVE (Ay,d16),CCR
	//
entry static move_w_ayd_ccr
	CYCLES(20,20,7)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		move_w_ea_ccr
	
	//================================================================================================

	//
	//		MOVE (Ay,Xn,d8),CCR
	//
entry static move_w_ayid_ccr
	bl		computeEA110
	CYCLES(22,22,4)
	b		move_w_ea_ccr

	//================================================================================================

	//
	//		MOVE (xxx).W,CCR
	//		MOVE (xxx).L,CCR
	//		MOVE (d16,PC),CCR
	//		MOVE (d8,PC,Xn),CCR
	//
entry static move_w_other_ccr
	bl		fetchEAWord111
	CYCLES(16,16,4)
	b		move_w_ea_ccr_post

	//================================================================================================
	//================================================================================================

#if (A68000_CHIP >= 68010)

	//
	//		MOVE CCR,Dy
	//
entry static move_w_ccr_dy
	CYCLES(0,4,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit offset
	rlwinm	r4,rSRFlags,0,24,31					// r4 = CCR
	WRITE_W_REG(r4,rRY)							// r3 = Dy
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MOVE CCR,(Ay)
	//
entry static move_w_ccr_ay0
	CYCLES(0,12,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
move_w_ccr_ea:
	rlwinm	r4,rSRFlags,0,24,31					// r4 = CCR
	WRITE_W_AT(rEA)								// r3 = word(rEA)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MOVE CCR,-(Ay)
	//
entry static move_w_ccr_aym
	CYCLES(0,14,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		move_w_ccr_ea
	
	//================================================================================================

	//
	//		MOVE CCR,(Ay)+
	//
entry static move_w_ccr_ayp
	CYCLES(0,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ccr_ea
	
	//================================================================================================

	//
	//		MOVE CCR,(Ay,d16)
	//
entry static move_w_ccr_ayd
	CYCLES(0,16,7)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		move_w_ccr_ea
	
	//================================================================================================

	//
	//		MOVE CCR,(Ay,Xn,d8)
	//
entry static move_w_ccr_ayid
	bl		computeEA110
	CYCLES(0,18,5)
	b		move_w_ccr_ea

	//================================================================================================

	//
	//		MOVE CCR,(xxx).W
	//		MOVE CCR,(xxx).L
	//
entry static move_w_ccr_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
move_w_ccr_abs:
	bne		move_w_ccr_absl						// handle long absolute mode
move_w_ccr_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(0,16,7)
	b		move_w_ccr_ea
move_w_ccr_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(0,20,7)
	b		move_w_ccr_ea

#endif

	//================================================================================================
	//
	//		MOVE to/from SR -- Move data into/out of the SR
	//
	//================================================================================================

	//
	//		MOVE Dy,SR
	//
entry static move_w_dy_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	CYCLES(12,12,8)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit offset
	READ_W_REG(r3,rRY)							// r3 = Dy
	b		executeLoopEndUpdateSR				// all done

	//================================================================================================

	//
	//		MOVE (Ay),SR
	//
entry static move_w_ay0_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	CYCLES(16,16,11)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
move_w_ea_sr:
	READ_W_AT(rEA)								// r3 = word(rEA)
	b		executeLoopEndUpdateSR				// all done

	//================================================================================================

	//
	//		MOVE -(Ay),SR
	//
entry static move_w_aym_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	CYCLES(18,18,11)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		move_w_ea_sr
	
	//================================================================================================

	//
	//		MOVE (Ay)+,SR
	//
entry static move_w_ayp_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	CYCLES(16,16,12)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_ea_sr
	
	//================================================================================================

	//
	//		MOVE (Ay,d16),SR
	//
entry static move_w_ayd_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	CYCLES(20,20,11)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		move_w_ea_sr
	
	//================================================================================================

	//
	//		MOVE (Ay,Xn,d8),SR
	//
entry static move_w_ayid_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	bl		computeEA110
	CYCLES(22,22,8)
	b		move_w_ea_sr

	//================================================================================================

	//
	//		MOVE (xxx).W,SR
	//		MOVE (xxx).L,SR
	//		MOVE (d16,PC),SR
	//		MOVE (d8,PC,Xn),SR
	//
entry static move_w_other_sr
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	bl		fetchEAWord111
	CYCLES(16,16,8)
	b		executeLoopEndUpdateSR

	//================================================================================================
	//================================================================================================

	//
	//		MOVE SR,Dy
	//
entry static move_w_sr_dy
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(6,6,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit offset
	rlwinm	r4,rSRFlags,0,16,31					// r4 = SR
	WRITE_W_REG(r4,rRY)							// r3 = Dy
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MOVE SR,(Ay)
	//
entry static move_w_sr_ay0
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(12,12,7)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
move_w_sr_ea:
	rlwinm	r4,rSRFlags,0,16,31					// r4 = SR
	WRITE_W_AT(rEA)								// r3 = word(rEA)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MOVE SR,-(Ay)
	//
entry static move_w_sr_aym
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(14,14,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		move_w_sr_ea
	
	//================================================================================================

	//
	//		MOVE SR,(Ay)+
	//
entry static move_w_sr_ayp
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		move_w_sr_ea
	
	//================================================================================================

	//
	//		MOVE SR,(Ay,d16)
	//
entry static move_w_sr_ayd
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(16,16,7)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		move_w_sr_ea
	
	//================================================================================================

	//
	//		MOVE SR,(Ay,Xn,d8)
	//
entry static move_w_sr_ayid
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	bl		computeEA110
	CYCLES(18,18,5)
	b		move_w_sr_ea

	//================================================================================================

	//
	//		MOVE SR,(xxx).W
	//		MOVE SR,(xxx).L
	//
entry static move_w_sr_other
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
move_w_sr_abs:
	bne		move_w_sr_absl						// handle long absolute mode
move_w_sr_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,7)
	b		move_w_sr_ea
move_w_sr_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,7)
	b		move_w_sr_ea

	//================================================================================================
	//
	//		MOVE to/from USP -- Move data into/out of the USP
	//
	//================================================================================================

	//
	//		MOVE USP,Ay
	//
entry static move_l_usp_ay
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(4,6,1)
//	lwz		r3,sUSP(rtoc)						// r3 = USP
	_asm_get_global(r3,sUSP)					// r3 = USP
	WRITE_L_AREG(r3,rRY)						// Ay = USP
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		MOVE Ay,USP
	//
entry static move_l_ay_usp
#if (A68000_CHIP >= 68010)
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
#endif
	CYCLES(4,6,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
//	stw		r3,sUSP(rtoc)						// USP = Ay
	_asm_set_global(r3,sUSP)					// USP = Ay
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		MULS/MULU -- Multiply
	//
	//================================================================================================

	//
	//		MULS.W Dy,Dx
	//
entry static muls_w_dy_dx
	CYCLES(54,42,25)
	READ_W_REG_EXT(r3,rRY)						// r3 = word-sized rRY
	READ_W_REG_EXT(r4,rRX)						// r4 = long-sized _DST
	CLR_VC										// V = C = 0
	mullw		r5,r3,r4						// r5 = r3 * r4
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_LONG(r5)								// N = (r5 & 0x80000000)
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MULS.W (Ay),Dx
	//
entry static muls_w_ay0_dx
	CYCLES(58,46,28)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
muls_w_ea_dx:
	READ_W_AT(rEA)								// r3 = word(rEA)
muls_w_ea_dx_post:
	READ_W_REG_EXT(r4,rRX)						// r4 = long-sized _DST
	extsh	r3,r3								// sign-extend
	CLR_VC										// V = C = 0
	mullw		r5,r3,r4						// r5 = r3 * r4
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_LONG(r5)								// N = (r5 & 0x80000000)
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MULS.W -(Ay),Dx
	//
entry static muls_w_aym_dx
	CYCLES(60,48,28)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		muls_w_ea_dx
	
	//================================================================================================

	//
	//		MULS.W (Ay)+,Dx
	//
entry static muls_w_ayp_dx
	CYCLES(58,46,29)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		muls_w_ea_dx
	
	//================================================================================================

	//
	//		MULS.W (Ay,d16),Dx
	//
entry static muls_w_ayd_dx
	CYCLES(62,50,28)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		muls_w_ea_dx
	
	//================================================================================================

	//
	//		MULS.W (Ay,Xn,d8),Dx
	//
entry static muls_w_ayid_dx
	bl		computeEA110
	CYCLES(64,52,25)
	b		muls_w_ea_dx

	//================================================================================================

	//
	//		MULS.W (xxx).W,Dx
	//		MULS.W (xxx).L,Dx
	//		MULS.W (d16,PC),Dx
	//		MULS.W (d8,PC,Xn),Dx
	//
entry static muls_w_other_dx
	bl		fetchEAWord111
	CYCLES(58,46,28)
	extsh	r3,r3
	b		muls_w_ea_dx_post

	//================================================================================================
	//================================================================================================

	//
	//		MULU.W Dy,Dx
	//
entry static mulu_w_dy_dx
	CYCLES(54,40,25)
	READ_W_REG(r3,rRY)							// r3 = word-sized rRY
	READ_W_REG(r4,rRX)							// r4 = long-sized _DST
	CLR_VC										// V = C = 0
	mullw		r5,r3,r4						// r5 = r3 * r4
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_LONG(r5)								// N = (r5 & 0x80000000)
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MULU.W (Ay),Dx
	//
entry static mulu_w_ay0_dx
	CYCLES(58,44,28)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
mulu_w_ea_dx:
	READ_W_AT(rEA)								// r3 = word(rEA)
mulu_w_ea_dx_post:
	READ_W_REG(r4,rRX)							// r4 = long-sized _DST
	CLR_VC										// V = C = 0
	mullw		r5,r3,r4						// r5 = r3 * r4
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_LONG(r5)								// N = (r5 & 0x80000000)
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		MULU.W -(Ay),Dx
	//
entry static mulu_w_aym_dx
	CYCLES(60,46,28)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		mulu_w_ea_dx
	
	//================================================================================================

	//
	//		MULU.W (Ay)+,Dx
	//
entry static mulu_w_ayp_dx
	CYCLES(58,44,29)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		mulu_w_ea_dx
	
	//================================================================================================

	//
	//		MULU.W (Ay,d16),Dx
	//
entry static mulu_w_ayd_dx
	CYCLES(62,48,28)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		mulu_w_ea_dx
	
	//================================================================================================

	//
	//		MULU.W (Ay,Xn,d8),Dx
	//
entry static mulu_w_ayid_dx
	bl		computeEA110
	CYCLES(64,50,25)
	b		mulu_w_ea_dx

	//================================================================================================

	//
	//		MULU.W (xxx).W,Dx
	//		MULU.W (xxx).L,Dx
	//		MULU.W (d16,PC),Dx
	//		MULU.W (d8,PC,Xn),Dx
	//
entry static mulu_w_other_dx
	bl		fetchEAWord111
	CYCLES(58,44,28)
	b		mulu_w_ea_dx_post

	//================================================================================================
	//
	//		MULS.L/MULU.L -- Multiply long
	//
	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		MULS.L Dy,Dx
	//
entry static mul_l_dy_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,41)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_L_REG(r3,rRY)							// r3 = long-sized rRY
mul_l_common:
	andi.	r9,rTempSave,0x0800					// see if this is signed or unsigned
	READ_L_REG(r4,rRX)							// r4 = long-sized rRX
	CLR_VC										// V = C = 0
	mullw	r5,r3,r4							// r5 = r3 * r4
	beq		mulu_l_common

muls_l_common:
	andi.	r0,rTempSave,0x0400					// see if this is 32-bit or 64-bit
	mulhw	r6,r3,r4							// r6 = r3 * r4 (high)
	cntlzw	r7,r5								// r7 = number of zeros in r5
	cntlzw	r8,r6								// r8 = number of zeros in r6
// KM	SET_N_LONG(r6)								// N = (r6 & 0x80000000)
	add		r7,r7,r8							// r7 = total number of zeros
	bne		muls_l_64bit						// perform a 64-bit multiply

muls_l_32bit:
// KM bugfix
	SET_N_LONG(r5)								// N = (r5 & 0x80000000)
	srawi	r8,r5,31							// r8 = sign extension of r5	
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	cmpw	cr1,r8,r6							// is r6 == sign extension of result?
	rlwimi	rSRFlags,r7,28,29,29				// Z = (r5:r6) == 0
	beq		cr1,executeLoopEnd					// all done if no overflow
	ori		rSRFlags,rSRFlags,k68000FlagV		// set the V flag
	b		executeLoopEnd						// all done

muls_l_64bit:
// KM bugfix
	SET_N_LONG(r6)								// N = (r6 & 0x80000000)
	rlwinm	rRY,rTempSave,2,27,29				// get high destination
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	rlwimi	rSRFlags,r7,28,29,29				// Z = (r5:r6) == 0
	WRITE_L_REG(r6,rRY)							// long-sized _DST = result
	b		executeLoopEnd						// all done

mulu_l_common:
	andi.	r0,rTempSave,0x0400					// see if this is 32-bit or 64-bit
	mulhwu	r6,r3,r4							// r6 = r3 * r4 (high)
	cntlzw	r7,r5								// r7 = number of zeros in r5
	cntlzw	r8,r6								// r8 = number of zeros in r6
	SET_N_LONG(r6)								// N = (r6 & 0x80000000)
	add		r7,r7,r8							// r7 = total number of zeros
	WRITE_L_REG(r5,rRX)							// long-sized _DST = result
	bne		mulu_l_64bit						// perform a 64-bit multiply

mulu_l_32bit:
	cmpwi	cr1,r6,0							// is r6 == sign extension of result?
	rlwimi	rSRFlags,r7,28,29,29				// Z = (r5:r6) == 0
	beq		cr1,executeLoopEnd					// all done if no overflow
	ori		rSRFlags,rSRFlags,k68000FlagV		// set the V flag
	b		executeLoopEnd						// all done

mulu_l_64bit:
	rlwinm	rRY,rTempSave,2,27,29				// get high destination
	rlwimi	rSRFlags,r7,28,29,29				// Z = (r5:r6) == 0
	WRITE_L_REG(r6,rRY)							// long-sized _DST = result
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		MULS.L (Ay),Dx
	//
entry static mul_l_ay0_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,44)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	READ_L_AT(rEA)								// r3 = word(rEA)
	b		mul_l_common

	//================================================================================================

	//
	//		MULS.L -(Ay),Dx
	//
entry static mul_l_aym_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,44)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	READ_L_AT(rEA)								// r3 = word(rEA)
	b		mul_l_common
	
	//================================================================================================

	//
	//		MULS.L (Ay)+,Dx
	//
entry static mul_l_ayp_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,45)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	READ_L_AT(rEA)								// r3 = word(rEA)
	b		mul_l_common
	
	//================================================================================================

	//
	//		MULS.L (Ay,d16),Dx
	//
entry static mul_l_ayd_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	CYCLES(0,0,44)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	READ_L_AT(rEA)								// r3 = word(rEA)
	b		mul_l_common
	
	//================================================================================================

	//
	//		MULS.L (Ay,Xn,d8),Dx
	//
entry static mul_l_ayid_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	bl		computeEA110
	CYCLES(0,0,41)
	READ_L_AT(rEA)								// r3 = word(rEA)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	b		mul_l_common

	//================================================================================================

	//
	//		MULS.L (xxx).W,Dx
	//		MULS.L (xxx).L,Dx
	//		MULS.L (d16,PC),Dx
	//		MULS.L (d8,PC,Xn),Dx
	//
entry static mul_l_other_dx
	READ_OPCODE_ARG(rTempSave)					// fetch the second word
	bl		fetchEALong111
	CYCLES(0,0,41)
	rlwinm	rRX,rTempSave,22,27,29				// determine the destination register
	b		mul_l_common
#endif

	//================================================================================================
	//
	//		NBCD -- BCD negate
	//
	//================================================================================================

	//
	//		NBCD.B Dy
	//
entry static nbcd_b_dy
	CYCLES(6,6,6)
	READ_B_REG(r3,rRY)							// r3 = rRY
	GET_X(r7)									// r7 = X
	rlwinm	r5,r3,0,28,31						// r5 = r3 & 0x0f
	rlwinm	r6,r3,0,24,27						// r6 = r3 & 0xf0
	neg		r5,r5								// r5 = -(r3 & 0x0f)
	neg		r6,r6								// r6 = -(r3 & 0xf0)
	sub		r5,r5,r7							// r5 = -(r3 & 0x0f) - X
	CLR_C										// clear carry
	cmplwi	r5,9								// do we need to adjust the low nibble?
	ble		nbcd_b_dy_adjust_a					// if not, skip
	subi	r6,r6,0x10							// borrow from the high nibble
	subi	r5,r5,6								// bring back in range
nbcd_b_dy_adjust_a:
	rlwinm	r4,r5,0,28,31						// r4 = final low nibble
	cmplwi	r6,0x90								// do we need to adjust the high nibble?
	ble		nbcd_b_dy_adjust_b					// if not, skip
	subi	r6,r6,0x60							// bring back in range
	SET_C										// carry
nbcd_b_dy_adjust_b:
	rlwimi	r4,r6,0,24,27						// insert the high nibble
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_X_FROM_C								// X = C
	rlwinm	r7,r7,29,29,29						// make a Z bit in the proper location
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// only clear Z; never set it
	WRITE_B_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NBCD.B (Ay)
	//
entry static nbcd_b_ay0
	CYCLES(12,12,12)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
nbcd_b_ea:
	READ_B_AT(rEA)								// r3 = (rEA)
	GET_X(r7)									// r7 = X
	rlwinm	r5,r3,0,28,31						// r5 = r3 & 0x0f
	rlwinm	r6,r3,0,24,27						// r6 = r3 & 0xf0
	neg		r5,r5								// r5 = -(r3 & 0x0f)
	neg		r6,r6								// r6 = -(r3 & 0xf0)
	sub		r5,r5,r7							// r5 = -(r3 & 0x0f) - X
	CLR_C										// clear carry
	cmplwi	r5,9								// do we need to adjust the low nibble?
	ble		nbcd_b_ea_adjust_a					// if not, skip
	subi	r6,r6,0x10							// borrow from the high nibble
	subi	r5,r5,6								// bring back in range
nbcd_b_ea_adjust_a:
	rlwinm	r4,r5,0,28,31						// r4 = final low nibble
	cmplwi	r6,0x90								// do we need to adjust the high nibble?
	ble		nbcd_b_ea_adjust_b					// if not, skip
	subi	r6,r6,0x60							// bring back in range
	SET_C										// carry
nbcd_b_ea_adjust_b:
	rlwimi	r4,r6,0,24,27						// insert the high nibble
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_X_FROM_C								// X = C
	rlwinm	r7,r7,29,29,29						// make a Z bit in the proper location
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// only clear Z; never set it
	SAVE_SR
	WRITE_B_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NBCD.B -(Ay)
	//
entry static nbcd_b_aym
	CYCLES(14,14,11)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		nbcd_b_ea
	
	//================================================================================================

	//
	//		NBCD.B (Ay)+
	//
entry static nbcd_b_ayp
	CYCLES(12,12,13)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		nbcd_b_ea
	
	//================================================================================================

	//
	//		NBCD.B (Ay,d16)
	//
entry static nbcd_b_ayd
	CYCLES(16,16,12)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		nbcd_b_ea
	
	//================================================================================================

	//
	//		NBCD.B (Ay,Xn,d8)
	//
entry static nbcd_b_ayid
	bl		computeEA110
	CYCLES(18,18,9)
	b		nbcd_b_ea

	//================================================================================================

	//
	//		NBCD.B (xxx).W
	//		NBCD.B (xxx).L
	//
entry static nbcd_b_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
nbcd_b_abs:
	bne		nbcd_b_absl							// handle long absolute mode
nbcd_b_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,12)
	b		nbcd_b_ea
nbcd_b_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,12)
	b		nbcd_b_ea

	//================================================================================================
	//
	//		NEGX -- negate with extend
	//
	//================================================================================================

	//
	//		NEGX.B Dy
	//
entry static negx_b_dy
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = rRY
	GET_X(r5)									// r5 = X
	add		rRX,r3,r5							// rRX = r3 + X
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	rlwinm	r7,r7,29,29,29						// shift Z into place
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if Z is set to 0
	WRITE_B_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NEGX.B (Ay)
	//
entry static negx_b_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
negx_b_ea:
	READ_B_AT(rEA)								// r3 = (rEA)
	GET_X(r5)									// r5 = X
	add		rRX,r3,r5							// rRX = r3 + X
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	rlwinm	r7,r7,29,29,29						// shift Z into place
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if Z is set to 0
	SAVE_SR
	WRITE_B_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NEGX.B -(Ay)
	//
entry static negx_b_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		negx_b_ea
	
	//================================================================================================

	//
	//		NEGX.B (Ay)+
	//
entry static negx_b_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		negx_b_ea
	
	//================================================================================================

	//
	//		NEGX.B (Ay,d16)
	//
entry static negx_b_ayd
	CYCLES(16,16,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		negx_b_ea
	
	//================================================================================================

	//
	//		NEGX.B (Ay,Xn,d8)
	//
entry static negx_b_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		negx_b_ea

	//================================================================================================

	//
	//		NEGX.B (xxx).W
	//		NEGX.B (xxx).L
	//
entry static negx_b_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
negx_b_abs:
	bne		negx_b_absl							// handle long absolute mode
negx_b_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		negx_b_ea
negx_b_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		negx_b_ea

	//================================================================================================
	//================================================================================================

	//
	//		NEGX.W Dy
	//
entry static negx_w_dy
	CYCLES(4,4,1)
	READ_W_REG(r3,rRY)							// r3 = rRY
	GET_X(r5)									// r5 = X
	add		rRX,r3,r5							// rRX = r3 + X
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	rlwinm	r7,r7,29,29,29						// shift Z into place
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if Z is set to 0
	WRITE_W_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NEGX.W (Ay)
	//
entry static negx_w_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
negx_w_ea:
	READ_W_AT(rEA)								// r3 = (rEA)
	GET_X(r5)									// r5 = X
	add		rRX,r3,r5							// rRX = r3 + X
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	rlwinm	r7,r7,29,29,29						// shift Z into place
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if Z is set to 0
	SAVE_SR
	WRITE_W_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NEGX.W -(Ay)
	//
entry static negx_w_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		negx_w_ea
	
	//================================================================================================

	//
	//		NEGX.W (Ay)+
	//
entry static negx_w_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		negx_w_ea
	
	//================================================================================================

	//
	//		NEGX.W (Ay,d16)
	//
entry static negx_w_ayd
	CYCLES(16,16,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		negx_w_ea
	
	//================================================================================================

	//
	//		NEGX.W (Ay,Xn,d8)
	//
entry static negx_w_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		negx_w_ea

	//================================================================================================

	//
	//		NEGX.W (xxx).W
	//		NEGX.W (xxx).L
	//
entry static negx_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
negx_w_abs:
	bne		negx_w_absl							// handle long absolute mode
negx_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		negx_w_ea
negx_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		negx_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		NEGX.L Dy
	//
entry static negx_l_dy
	CYCLES(6,6,1)
	READ_L_REG(rRX,rRY)							// rRX = rRY
	li		r3,0								// r3 = 0
	GET_X(r5)									// r5 = X
	CLR_VC										// C = V = 0
	subc	r4,r3,rRX							// r4 = 0 - rRX
	SET_C_LONG(r4)								// set carry
	subc	r4,r4,r5							// r4 -= X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100) -- important: must set V afterwards
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	rlwinm	r7,r7,29,29,29						// shift Z into place
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if Z is set to 0
	WRITE_L_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NEGX.L (Ay)
	//
entry static negx_l_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
negx_l_ea:
	READ_L_AT(rEA)								// r3 = (rEA)
	mr		rRX,r3								// rRX = r3
	GET_X(r5)									// r5 = X
	li		r3,0								// r3 = 0
	CLR_VC										// C = V = 0
	subc	r4,r3,rRX							// r4 = 0 - rRX
	SET_C_LONG(r4)								// set carry
	subc	r4,r4,r5							// r4 = 0 - rRX - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100) -- important: must set V afterwards
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	rlwinm	r7,r7,29,29,29						// shift Z into place
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if Z is set to 0
	SAVE_SR
	WRITE_L_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NEGX.L -(Ay)
	//
entry static negx_l_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		negx_l_ea
	
	//================================================================================================

	//
	//		NEGX.L (Ay)+
	//
entry static negx_l_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		negx_l_ea
	
	//================================================================================================

	//
	//		NEGX.L (Ay,d16)
	//
entry static negx_l_ayd
	CYCLES(24,24,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		negx_l_ea
	
	//================================================================================================

	//
	//		NEGX.L (Ay,Xn,d8)
	//
entry static negx_l_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		negx_l_ea

	//================================================================================================

	//
	//		NEGX.L (xxx).W
	//		NEGX.L (xxx).L
	//
entry static negx_l_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
negx_l_abs:
	bne		negx_l_absl							// handle long absolute mode
negx_l_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		negx_l_ea
negx_l_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		negx_l_ea

	//================================================================================================
	//
	//		NEG -- negate
	//
	//================================================================================================

	//
	//		NEG.B Dy
	//
entry static neg_b_dy
	CYCLES(4,4,1)
	READ_B_REG(rRX,rRY)							// rRX = rRY
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NEG.B (Ay)
	//
entry static neg_b_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
neg_b_ea:
	READ_B_AT(rEA)								// r3 = (rEA)
	TRUNC_BYTE_TO(rRX,r3)						// rRX = r3
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NEG.B -(Ay)
	//
entry static neg_b_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		neg_b_ea
	
	//================================================================================================

	//
	//		NEG.B (Ay)+
	//
entry static neg_b_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		neg_b_ea
	
	//================================================================================================

	//
	//		NEG.B (Ay,d16)
	//
entry static neg_b_ayd
	CYCLES(16,16,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		neg_b_ea
	
	//================================================================================================

	//
	//		NEG.B (Ay,Xn,d8)
	//
entry static neg_b_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		neg_b_ea

	//================================================================================================

	//
	//		NEG.B (xxx).W
	//		NEG.B (xxx).L
	//
entry static neg_b_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
neg_b_abs:
	bne		neg_b_absl							// handle long absolute mode
neg_b_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		neg_b_ea
neg_b_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		neg_b_ea

	//================================================================================================
	//================================================================================================

	//
	//		NEG.W Dy
	//
entry static neg_w_dy
	CYCLES(4,4,1)
	READ_W_REG(rRX,rRY)							// rRX = rRY
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NEG.W (Ay)
	//
entry static neg_w_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
neg_w_ea:
	READ_W_AT(rEA)								// r3 = (rEA)
	TRUNC_WORD_TO(rRX,r3)						// rRX = r3
	li		r3,0								// r3 = 0
	sub		r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NEG.W -(Ay)
	//
entry static neg_w_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		neg_w_ea
	
	//================================================================================================

	//
	//		NEG.W (Ay)+
	//
entry static neg_w_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		neg_w_ea
	
	//================================================================================================

	//
	//		NEG.W (Ay,d16)
	//
entry static neg_w_ayd
	CYCLES(16,16,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		neg_w_ea
	
	//================================================================================================

	//
	//		NEG.W (Ay,Xn,d8)
	//
entry static neg_w_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		neg_w_ea

	//================================================================================================

	//
	//		NEG.W (xxx).W
	//		NEG.W (xxx).L
	//
entry static neg_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
neg_w_abs:
	bne		neg_w_absl							// handle long absolute mode
neg_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		neg_w_ea
neg_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		neg_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		NEG.L Dy
	//
entry static neg_l_dy
	CYCLES(6,6,1)
	READ_L_REG(rRX,rRY)							// rRX = rRY
	li		r3,0								// r3 = 0
	subc	r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NEG.L (Ay)
	//
entry static neg_l_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
neg_l_ea:
	READ_L_AT(rEA)								// r3 = (rEA)
	mr		rRX,r3								// rRX = r3
	li		r3,0								// r3 = 0
	subc	r4,r3,rRX							// r4 = 0 - rRX = 0 - (r3 + X) = 0 - r3 - X
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r3 ^ r4) & (r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NEG.L -(Ay)
	//
entry static neg_l_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		neg_l_ea
	
	//================================================================================================

	//
	//		NEG.L (Ay)+
	//
entry static neg_l_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		neg_l_ea
	
	//================================================================================================

	//
	//		NEG.L (Ay,d16)
	//
entry static neg_l_ayd
	CYCLES(24,24,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		neg_l_ea
	
	//================================================================================================

	//
	//		NEG.L (Ay,Xn,d8)
	//
entry static neg_l_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		neg_l_ea

	//================================================================================================

	//
	//		NEG.L (xxx).W
	//		NEG.L (xxx).L
	//
entry static neg_l_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
neg_l_abs:
	bne		neg_l_absl							// handle long absolute mode
neg_l_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		neg_l_ea
neg_l_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(30,30,6)
	b		neg_l_ea

	//================================================================================================
	//
	//		NOT -- logical NOT
	//
	//================================================================================================

	//
	//		NOT.B Dy
	//
entry static not_b_dy
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	xori	r4,r3,0xff							// r4 = ~rRX
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NOT.B (Ay)
	//
entry static not_b_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
not_b_ea:
	READ_B_AT(rEA)								// r3 = (rEA)
	TRUNC_BYTE(r3)								// rRX = r3
	CLR_VC										// V = C = 0
	xori	r4,r3,0xff							// r4 = ~rRX
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NOT.B -(Ay)
	//
entry static not_b_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		not_b_ea
	
	//================================================================================================

	//
	//		NOT.B (Ay)+
	//
entry static not_b_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		not_b_ea
	
	//================================================================================================

	//
	//		NOT.B (Ay,d16)
	//
entry static not_b_ayd
	CYCLES(16,16,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		not_b_ea
	
	//================================================================================================

	//
	//		NOT.B (Ay,Xn,d8)
	//
entry static not_b_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		not_b_ea

	//================================================================================================

	//
	//		NOT.B (xxx).W
	//		NOT.B (xxx).L
	//
entry static not_b_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
not_b_abs:
	bne		not_b_absl							// handle long absolute mode
not_b_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		not_b_ea
not_b_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		not_b_ea

	//================================================================================================
	//================================================================================================

	//
	//		NOT.W Dy
	//
entry static not_w_dy
	CYCLES(4,4,1)
	READ_W_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	xori	r4,r3,0xffff						// r4 = ~r3
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NOT.W (Ay)
	//
entry static not_w_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
not_w_ea:
	READ_W_AT(rEA)								// r3 = (rEA)
	TRUNC_WORD(r3)								// rRX = r3
	CLR_VC										// V = C = 0
	xori	r4,r3,0xffff						// r4 = ~rRX
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NOT.W -(Ay)
	//
entry static not_w_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		not_w_ea
	
	//================================================================================================

	//
	//		NOT.W (Ay)+
	//
entry static not_w_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		not_w_ea
	
	//================================================================================================

	//
	//		NOT.W (Ay,d16)
	//
entry static not_w_ayd
	CYCLES(16,16,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		not_w_ea
	
	//================================================================================================

	//
	//		NOT.W (Ay,Xn,d8)
	//
entry static not_w_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		not_w_ea

	//================================================================================================

	//
	//		NOT.W (xxx).W
	//		NOT.W (xxx).L
	//
entry static not_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
not_w_abs:
	bne		not_w_absl							// handle long absolute mode
not_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		not_w_ea
not_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		not_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		NOT.L Dy
	//
entry static not_l_dy
	CYCLES(6,6,1)
	READ_L_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	not		r4,r3								// r4 = ~rRX
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		NOT.L (Ay)
	//
entry static not_l_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
not_l_ea:
	READ_L_AT(rEA)								// r3 = (rEA)
	CLR_VC										// V = C = 0
	not		r4,r3								// r4 = ~rRX
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		NOT.L -(Ay)
	//
entry static not_l_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		not_l_ea
	
	//================================================================================================

	//
	//		NOT.L (Ay)+
	//
entry static not_l_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		not_l_ea
	
	//================================================================================================

	//
	//		NOT.L (Ay,d16)
	//
entry static not_l_ayd
	CYCLES(24,24,6)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		not_l_ea
	
	//================================================================================================

	//
	//		NOT.L (Ay,Xn,d8)
	//
entry static not_l_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		not_l_ea

	//================================================================================================

	//
	//		NOT.L (xxx).W
	//		NOT.L (xxx).L
	//
entry static not_l_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
not_l_abs:
	bne		not_l_absl							// handle long absolute mode
not_l_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		not_l_ea
not_l_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		not_l_ea

	//================================================================================================
	//
	//		ORI -- Or immediate with effective address
	//
	//================================================================================================

	//
	//		ORI.B #xx,Dy
	//
entry static ori_b_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	READ_B_REG(r3,rRY)							// r3 = Dy
	or		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ORI.B #xx,(Ay)
	//
entry static ori_b_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
ori_b_imm_ea:
	READ_B_AT(rEA)								// r3 = byte(rEA)
	or		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ORI.B #xx,-(Ay)
	//
entry static ori_b_imm_aym
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		ori_b_imm_ea
	
	//================================================================================================

	//
	//		ORI.B #xx,(Ay)+
	//
entry static ori_b_imm_ayp
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		ori_b_imm_ea
	
	//================================================================================================

	//
	//		ORI.B #xx,(Ay,d16)
	//
entry static ori_b_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		ori_b_imm_ea
	
	//================================================================================================

	//
	//		ORI.B #xx,(Ay,Xn,d8)
	//
entry static ori_b_imm_ayid
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		ori_b_imm_ea

	//================================================================================================

	//
	//		ORI.B (xxx).W,Ax
	//		ORI.B (xxx).L,Ax
	//
entry static ori_b_imm_other
	cmpwi	cr1,rRY,4<<2						// is this the CCR case?
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	beq		cr1,ori_b_imm_ccr					// handle CCR case
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		ori_b_imm_absl						// handle long absolute mode
ori_b_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		ori_b_imm_ea
ori_b_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		ori_b_imm_ea
ori_b_imm_ccr:
	or		r3,rSRFlags,rRX						// r3 = rSRFlags & rRX
	rlwimi	rSRFlags,r3,0,24,31					// keep only the low byte
	CYCLES(20,20,9)
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ORI.W #xx,Dy
	//
entry static ori_w_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	READ_W_REG(r3,rRY)							// r3 = Dy
	or		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ORI.W #xx,(Ay)
	//
entry static ori_w_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
ori_w_imm_ea:
	READ_W_AT(rEA)								// r3 = byte(rEA)
	or		r4,r3,rRX							// r4 = r3 & rRX
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ORI.W #xx,-(Ay)
	//
entry static ori_w_imm_aym
	CYCLES(18,18,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		ori_w_imm_ea
	
	//================================================================================================

	//
	//		ORI.W #xx,(Ay)+
	//
entry static ori_w_imm_ayp
	CYCLES(16,16,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		ori_w_imm_ea
	
	//================================================================================================

	//
	//		ORI.W #xx,(Ay,d16)
	//
entry static ori_w_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		ori_w_imm_ea
	
	//================================================================================================

	//
	//		ORI.W #xx,(Ay,Xn,d8)
	//
entry static ori_w_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		ori_w_imm_ea

	//================================================================================================

	//
	//		ORI.W (xxx).W,Ax
	//		ORI.W (xxx).L,Ax
	//
entry static ori_w_imm_other
	cmpwi	cr1,rRY,4<<2						// is this the SR case?
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	beq		cr1,ori_w_imm_sr					// handle SR case
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	bne		ori_w_imm_absl						// handle long absolute mode
ori_w_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		ori_w_imm_ea
ori_w_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		ori_w_imm_ea
ori_w_imm_sr:
	andi.	r0,rSRFlags,k68000FlagS				// supervisor mode?
	beq		supervisorException					// if not in supervisor mode, generate an exception
	or		r3,rSRFlags,rRX						// r3 = rSRFlags | rRX
	CYCLES(20,20,9)
	b		executeLoopEndUpdateSR				// all done

	//================================================================================================
	//================================================================================================

	//
	//		ORI.L #xx,Dy
	//
entry static ori_l_imm_dy
	CYCLES(16,14,1)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	READ_L_REG(r3,rRY)							// r3 = Dy
	or		r4,r3,rRX							// r4 = r3 & rRX
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ORI.L #xx,(Ay)
	//
entry static ori_l_imm_ay0
	CYCLES(28,28,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
ori_l_imm_ea:
	READ_L_AT(rEA)								// r3 = byte(rEA)
	or		r4,r3,rRX							// r4 = r3 & rRX
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		ORI.L #xx,-(Ay)
	//
entry static ori_l_imm_aym
	CYCLES(30,30,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		ori_l_imm_ea
	
	//================================================================================================

	//
	//		ORI.L #xx,(Ay)+
	//
entry static ori_l_imm_ayp
	CYCLES(28,28,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		ori_l_imm_ea
	
	//================================================================================================

	//
	//		ORI.L #xx,(Ay,d16)
	//
entry static ori_l_imm_ayd
	CYCLES(32,32,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		ori_l_imm_ea
	
	//================================================================================================

	//
	//		ORI.L #xx,(Ay,Xn,d8)
	//
entry static ori_l_imm_ayid
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(34,34,4)
	b		ori_l_imm_ea

	//================================================================================================

	//
	//		ORI.L (xxx).W,Ax
	//		ORI.L (xxx).L,Ax
	//
entry static ori_l_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	bne		ori_l_imm_absl						// handle long absolute mode
ori_l_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(32,32,7)
	b		ori_l_imm_ea
ori_l_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(36,36,7)
	b		ori_l_imm_ea

	//================================================================================================
	//
	//		OR -- Or two values
	//
	//================================================================================================

	//
	//		OR.B Dy,Dx
	//
entry static or_b_dy_dx
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRY,rRX)							// rRY = Dx
	or		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		OR.B (Ay),Dx
	//
entry static or_b_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
or_b_ea_dx:
	READ_B_AT(rEA)								// r3 = byte(rEA)
or_b_ea_dx_post:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	or		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		OR.B -(Ay),Dx
	//
entry static or_b_aym_dx
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		or_b_ea_dx
	
	//================================================================================================

	//
	//		OR.B (Ay)+,Dx
	//
entry static or_b_ayp_dx
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		or_b_ea_dx
	
	//================================================================================================

	//
	//		OR.B (Ay,d16),Dx
	//
entry static or_b_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		or_b_ea_dx
	
	//================================================================================================

	//
	//		OR.B (Ay,Xn,d8),Dx
	//
entry static or_b_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		or_b_ea_dx

	//================================================================================================

	//
	//		OR.B (xxx).W,Dx
	//		OR.B (xxx).L,Dx
	//		OR.B (d16,PC),Dx
	//		OR.B (d8,PC,Xn),Dx
	//
entry static or_b_other_dx
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		or_b_ea_dx_post

	//================================================================================================

	//
	//		OR.B Dx,(Ay)
	//
entry static or_b_dx_ay0
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
or_b_dx_ea:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	READ_B_AT(rEA)								// r3 = byte(rEA)
	or		r4,r3,rRY							// r4 = r3 + rRY
	TRUNC_BYTE(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		OR.B Dx,-(Ay)
	//
entry static or_b_dx_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		or_b_dx_ea
	
	//================================================================================================

	//
	//		OR.B Dx,(Ay)+
	//
entry static or_b_dx_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		or_b_dx_ea
	
	//================================================================================================

	//
	//		OR.B Dx,(Ay,d16)
	//
entry static or_b_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		or_b_dx_ea
	
	//================================================================================================

	//
	//		OR.B Dx,(Ay,Xn,d8)
	//
entry static or_b_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		or_b_dx_ea

	//================================================================================================

	//
	//		OR.B Dx,(xxx).W
	//		OR.B Dx,(xxx).L
	//
entry static or_b_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		or_b_dx_absl						// handle long absolute mode
or_b_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		or_b_dx_ea
or_b_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		or_b_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		OR.W Dy,Dx
	//
entry static or_w_dy_dx
	CYCLES(4,4,1)
	READ_W_REG(r3,rRY)							// r3 = Ry
	READ_W_REG(rRY,rRX)							// rRY = Dx
	or		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		OR.W (Ay),Dx
	//
entry static or_w_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
or_w_ea_dx:
	READ_W_AT(rEA)								// r3 = byte(rEA)
or_w_ea_dx_post:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	or		r4,rRY,r3							// r4 = rRY + r3
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		OR.W -(Ay),Dx
	//
entry static or_w_aym_dx
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		or_w_ea_dx
	
	//================================================================================================

	//
	//		OR.W (Ay)+,Dx
	//
entry static or_w_ayp_dx
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		or_w_ea_dx
	
	//================================================================================================

	//
	//		OR.W (Ay,d16),Dx
	//
entry static or_w_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		or_w_ea_dx
	
	//================================================================================================

	//
	//		OR.W (Ay,Xn,d8),Dx
	//
entry static or_w_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		or_w_ea_dx

	//================================================================================================

	//
	//		OR.W (xxx).W,Dx
	//		OR.W (xxx).L,Dx
	//		OR.W (d16,PC),Dx
	//		OR.W (d8,PC,Xn),Dx
	//
entry static or_w_other_dx
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		or_w_ea_dx_post

	//================================================================================================

	//
	//		OR.W Dx,(Ay)
	//
entry static or_w_dx_ay0
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
or_w_dx_ea:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	READ_W_AT(rEA)								// r3 = byte(rEA)
	or		r4,r3,rRY							// r4 = r3 + rRY
	TRUNC_WORD(r4)								// keep the result byte-sized
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		OR.W Dx,-(Ay)
	//
entry static or_w_dx_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		or_w_dx_ea
	
	//================================================================================================

	//
	//		OR.W Dx,(Ay)+
	//
entry static or_w_dx_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		or_w_dx_ea
	
	//================================================================================================

	//
	//		OR.W Dx,(Ay,d16)
	//
entry static or_w_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		or_w_dx_ea
	
	//================================================================================================

	//
	//		OR.W Dx,(Ay,Xn,d8)
	//
entry static or_w_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		or_w_dx_ea

	//================================================================================================

	//
	//		OR.W Dx,(xxx).W
	//		OR.W Dx,(xxx).L
	//
entry static or_w_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		or_w_dx_absl						// handle long absolute mode
or_w_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		or_w_dx_ea
or_w_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		or_w_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		OR.L Dy,Dx
	//
entry static or_l_dy_dx
	CYCLES(8,8,1)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_REG(rRY,rRX)							// rRY = Dx
	or		r4,rRY,r3							// r4 = rRY + r3
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		OR.L (Ay),Dx
	//
entry static or_l_ay0_dx
	CYCLES(14,14,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
or_l_ea_dx:
	READ_L_AT(rEA)								// r3 = byte(rEA)
or_l_ea_dx_post:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	or		r4,rRY,r3							// r4 = rRY + r3
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		OR.L -(Ay),Dx
	//
entry static or_l_aym_dx
	CYCLES(16,16,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		or_l_ea_dx
	
	//================================================================================================

	//
	//		OR.L (Ay)+,Dx
	//
entry static or_l_ayp_dx
	CYCLES(14,14,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		or_l_ea_dx
	
	//================================================================================================

	//
	//		OR.L (Ay,d16),Dx
	//
entry static or_l_ayd_dx
	CYCLES(18,18,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		or_l_ea_dx
	
	//================================================================================================

	//
	//		OR.L (Ay,Xn,d8),Dx
	//
entry static or_l_ayid_dx
	bl		computeEA110
	CYCLES(20,20,0)
	b		or_l_ea_dx

	//================================================================================================

	//
	//		OR.L (xxx).W,Dx
	//		OR.L (xxx).L,Dx
	//		OR.L (d16,PC),Dx
	//		OR.L (d8,PC,Xn),Dx
	//
entry static or_l_other_dx
	bl		fetchEALong111
	CYCLES(16,16,0)
	b		or_l_ea_dx_post

	//================================================================================================

	//
	//		OR.L Dx,(Ay)
	//
entry static or_l_dx_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
or_l_dx_ea:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	READ_L_AT(rEA)								// r3 = byte(rEA)
	or		r4,r3,rRY							// r4 = r3 + rRY
	CLR_VC										// V = C = 0
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		OR.L Dx,-(Ay)
	//
entry static or_l_dx_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		or_l_dx_ea
	
	//================================================================================================

	//
	//		OR.L Dx,(Ay)+
	//
entry static or_l_dx_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		or_l_dx_ea
	
	//================================================================================================

	//
	//		OR.L Dx,(Ay,d16)
	//
entry static or_l_dx_ayd
	CYCLES(24,24,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		or_l_dx_ea
	
	//================================================================================================

	//
	//		OR.L Dx,(Ay,Xn,d8)
	//
entry static or_l_dx_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		or_l_dx_ea

	//================================================================================================

	//
	//		OR.L Dx,(xxx).W
	//		OR.L Dx,(xxx).L
	//
entry static or_l_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		or_l_dx_absl						// handle long absolute mode
or_l_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		or_l_dx_ea
or_l_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		or_l_dx_ea

	//================================================================================================
	//
	//		PEA -- Load effective address
	//
	//================================================================================================

	//
	//		PEA (Ay),Ax
	//
entry static pea_ay0
	CYCLES(12,12,5)
	SAVE_SR
	GET_A7(rTempSave)
	SAVE_PC
	READ_L_AREG(r4,rRY)							// r4 = Ay
	subi	rTempSave,rTempSave,4
	SET_A7(rTempSave)
	WRITE_L_AT(rTempSave)
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		PEA (Ay,d16),Ax
	//
entry static pea_ayd
	CYCLES(16,16,5)
	SAVE_SR
	GET_A7(rTempSave)
	SAVE_PC
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	subi	rTempSave,rTempSave,4
	SET_A7(rTempSave)
	add		r4,rEA,r3							// r4 = rEA + r3
	WRITE_L_AT(rTempSave)
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	//		PEA (Ay,Xn,d8),Ax
	//
entry static pea_ayid
	SAVE_SR
	GET_A7(rTempSave)
	SAVE_PC
	subi	rTempSave,rTempSave,4
	bl		computeEA110
	mr		r4,rEA
	SET_A7(rTempSave)
	WRITE_L_AT(rTempSave)
	CYCLES(20,20,3)
	b		executeLoopEndRestore				// return*/

	//================================================================================================

	//
	//		PEA (xxx).W,Ax
	//		PEA (xxx).L,Ax
	//		PEA (d16,PC),Ax
	//		PEA (d8,PC,Xn),Ax
	//
entry static pea_other
	SAVE_SR
	GET_A7(rTempSave)
	SAVE_PC
	subi	rTempSave,rTempSave,4
	bl		computeEA111
	mr		r4,rEA
	SET_A7(rTempSave)
	WRITE_L_AT(rTempSave)
	CYCLES(12,12,3)
	b		executeLoopEndRestore				// return

	//================================================================================================
	//
	//		ROL -- Rotate left
	//
	//================================================================================================

	//
	//		ROL.B Dx,Dy
	//
entry static rol_b_dx_dy
	CYCLES(6,6,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,8,16,23						// copy the byte
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		rol_b_dx_dy_0						// skip if zero
	rlwimi	r3,r3,16,0,15						// copy the word
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done
rol_b_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_BYTE(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROL.B Ix,Dy
	//
entry static rol_b_ix_dy
	CYCLES(6,6,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,8,16,23						// copy the byte
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwimi	r3,r3,16,0,15						// copy the word
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ROL.W Dx,Dy
	//
entry static rol_w_dx_dy
	CYCLES(6,6,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,16,0,15						// copy the word
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		rol_w_dx_dy_0						// skip if zero
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done
rol_w_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_WORD(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROL.W Ix,Dy
	//
entry static rol_w_ix_dy
	CYCLES(6,6,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,16,0,15						// copy the word
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROL.W #1,(Ay)
	//
entry static rol_w_ay0
	CYCLES(12,12,10)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
rol_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,1,16,30						// r4 = r3 << 1
	CLR_V										// V = 0
	rlwimi	r4,r3,17,31,31						// copy the low bit
	rlwimi	rSRFlags,r3,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ROL.W #1,-(Ay)
	//
entry static rol_w_aym
	CYCLES(14,14,10)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		rol_w_ea
	
	//================================================================================================

	//
	//		ROL.W #1,(Ay)+
	//
entry static rol_w_ayp
	CYCLES(12,12,11)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		rol_w_ea
	
	//================================================================================================

	//
	//		ROL.W #1,(Ay,d16)
	//
entry static rol_w_ayd
	CYCLES(16,16,10)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		rol_w_ea
	
	//================================================================================================

	//
	//		ROL.W #1,(Ay,Xn,d8)
	//
entry static rol_w_ayid
	bl		computeEA110
	CYCLES(18,18,7)
	b		rol_w_ea

	//================================================================================================

	//
	//		ROL.W #1,(xxx).W
	//		ROL.W #1,(xxx).L
	//
entry static rol_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		rol_w_absl							// handle long absolute mode
rol_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,10)
	b		rol_w_ea
rol_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,10)
	b		rol_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		ROL.L Dx,Dy
	//
entry static rol_l_dx_dy
	CYCLES(8,8,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		rol_l_dx_dy_0						// special case for zero
	rlwimi	rSRFlags,r4,0,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done
rol_l_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROL.L Ix,Dy
	//
entry static rol_l_ix_dy
	CYCLES(8,8,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	CLR_V										// V = 0
	add		r0,rRX,rRX
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	add		rCycleCount,rCycleCount,r0
	rlwimi	rSRFlags,r4,0,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		ROR -- Rotate right
	//
	//================================================================================================

	//
	//		ROR.B Dx,Dy
	//
entry static ror_b_dx_dy
	CYCLES(6,6,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,8,16,23						// copy the byte
	add		r0,rRX,rRX
	subfic	rRX,rRX,64							// rRX = 64 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	beq		ror_b_dx_dy_0						// skip if zero
	rlwimi	r3,r3,16,0,15						// copy the word
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done
ror_b_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_BYTE(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROR.B Ix,Dy
	//
entry static ror_b_ix_dy
	CYCLES(6,6,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,8,16,23						// copy the byte
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwimi	r3,r3,16,0,15						// copy the word
	add		r0,rRX,rRX
	subfic	rRX,rRX,64							// rRX = 64 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ROR.W Dx,Dy
	//
entry static ror_w_dx_dy
	CYCLES(6,6,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,16,0,15						// copy the word
	add		r0,rRX,rRX
	subfic	rRX,rRX,64							// rRX = 64 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	beq		ror_w_dx_dy_0						// skip if zero
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done
ror_w_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_WORD(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROR.W Ix,Dy
	//
entry static ror_w_ix_dy
	CYCLES(6,6,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,16,0,15						// copy the word
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	add		r0,rRX,rRX
	subfic	rRX,rRX,64							// rRX = 64 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROR.W #1,(Ay)
	//
entry static ror_w_ay0
	CYCLES(12,12,10)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
ror_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,31,17,31						// r4 = r3 << 1
	CLR_V										// V = 0
	rlwimi	r4,r3,15,16,16						// copy the high bit
	rlwimi	rSRFlags,r3,0,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ROR.W #1,-(Ay)
	//
entry static ror_w_aym
	CYCLES(14,14,10)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		ror_w_ea
	
	//================================================================================================

	//
	//		ROR.W #1,(Ay)+
	//
entry static ror_w_ayp
	CYCLES(12,12,11)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		ror_w_ea
	
	//================================================================================================

	//
	//		ROR.W #1,(Ay,d16)
	//
entry static ror_w_ayd
	CYCLES(16,16,10)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		ror_w_ea
	
	//================================================================================================

	//
	//		ROR.W #1,(Ay,Xn,d8)
	//
entry static ror_w_ayid
	bl		computeEA110
	CYCLES(18,18,7)
	b		ror_w_ea

	//================================================================================================

	//
	//		ROR.W #1,(xxx).W
	//		ROR.W #1,(xxx).L
	//
entry static ror_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		ror_w_absl							// handle long absolute mode
ror_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,10)
	b		ror_w_ea
ror_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,10)
	b		ror_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		ROR.L Dx,Dy
	//
entry static ror_l_dx_dy
	CYCLES(8,8,5)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	add		r0,rRX,rRX
	subfic	rRX,rRX,64							// rRX = 64 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	beq		ror_l_dx_dy_0						// special case for zero
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done
ror_l_dx_dy_0:
	CLR_C										// C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROR.L Ix,Dy
	//
entry static ror_l_ix_dy
	CYCLES(8,8,5)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	add		r0,rRX,rRX
	subfic	rRX,rRX,64							// rRX = 64 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	rlwimi	rSRFlags,r4,1,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		ROXL -- Rotate left through extend
	//
	//================================================================================================

	//
	//		ROXL.B Dx,Dy
	//
entry static roxl_b_dx_dy
	CYCLES(6,6,9)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,23,1,8						// copy the byte, leaving room for the extend bit
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		roxl_b_dx_dy_0						// skip if zero
	rlwimi	r3,rSRFlags,27,0,0					// copy the X bit between the two copies of the byte
roxl_b_dx_dy_again:
	cmpwi	rRX,9								// greater than 9 bits?
	ble+	roxl_b_dx_dy_done					// if not, skip
	subi	rRX,rRX,9							// rRX -= 9
	b		roxl_b_dx_dy_again					// check again
roxl_b_dx_dy_done:
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
roxl_b_dx_dy_0:
	rlwimi	rSRFlags,rSRFlags,28,31,31			// C = X
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_BYTE(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXL.B Ix,Dy
	//
entry static roxl_b_ix_dy
	CYCLES(6,6,9)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,23,1,8						// copy the byte, leaving room for the extend bit
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwimi	r3,rSRFlags,27,0,0					// copy the X bit between the two copies of the byte
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ROXL.W Dx,Dy
	//
entry static roxl_w_dx_dy
	CYCLES(6,6,9)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,16,0,15						// copy the word
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		roxl_w_dx_dy_0						// skip if zero
roxl_w_dx_dy_again:
	cmpwi	rRX,17								// greater than 17 bits?
	ble+	roxl_w_dx_dy_done					// if not, skip
	subi	rRX,rRX,17							// rRX -= 17
	b		roxl_w_dx_dy_again					// check again
roxl_w_dx_dy_done:
	rlwimi	r3,r3,1,16,30						// pre-shift by 1 bit to make room for X
	subi	rRX,rRX,1							// rRX -= 1, to account for the pre-shifted bit
	rlwimi	r3,rSRFlags,28,31,31				// copy the X bit in between the two copies of r3
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
roxl_w_dx_dy_0:
	rlwimi	rSRFlags,rSRFlags,28,31,31			// C = X
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_WORD(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXL.W Ix,Dy
	//
entry static roxl_w_ix_dy
	CYCLES(6,6,9)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,15,1,15						// copy the word, leaving room for the extend bit
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwimi	r3,rSRFlags,27,0,0					// copy the X bit between the two copies of the word
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXL.W #1,(Ay)
	//
entry static roxl_w_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
roxl_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,1,16,30						// r4 = r3 << 1
	CLR_V										// V = 0
	rlwimi	r4,rSRFlags,0,31,31					// copy the low bit
	rlwimi	rSRFlags,r3,17,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SET_X_FROM_C								// X = C
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ROXL.W #1,-(Ay)
	//
entry static roxl_w_aym
	CYCLES(14,14,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		roxl_w_ea
	
	//================================================================================================

	//
	//		ROXL.W #1,(Ay)+
	//
entry static roxl_w_ayp
	CYCLES(12,12,9)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		roxl_w_ea
	
	//================================================================================================

	//
	//		ROXL.W #1,(Ay,d16)
	//
entry static roxl_w_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		roxl_w_ea
	
	//================================================================================================

	//
	//		ROXL.W #1,(Ay,Xn,d8)
	//
entry static roxl_w_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		roxl_w_ea

	//================================================================================================

	//
	//		ROXL.W #1,(xxx).W
	//		ROXL.W #1,(xxx).L
	//
entry static roxl_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		roxl_w_absl							// handle long absolute mode
roxl_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		roxl_w_ea
roxl_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		roxl_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		ROXL.L Dx,Dy
	//
entry static roxl_l_dx_dy
	CYCLES(8,8,9)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		roxl_l_dx_dy_0						// special case for zero
roxl_l_dx_dy_again:
	cmpwi	rRX,33								// greater than 33 bits?
	ble+	roxl_w_dx_dy_done					// if not, skip
	subi	rRX,rRX,33							// rRX -= 33
roxl_l_dx_dy_done:
	subfic	r9,rRX,33							// r9 = 33 - count
	rlwinm	r6,rSRFlags,28,31,31				// r6 = X in the low bit
	subi	rRX,rRX,1							// rRX -= 1
	slw		r8,r6,rRX							// r8 = (X << (count - 1))
	srw		r9,r3,r9							// r9 = (val >> (33 - count))
	slw		r4,r3,rRX							// r4 = (val << (count - 1))
	or		r8,r8,r9							// r8 = (val >> (33 - count)) | (X << (count - 1))
	rlwimi	rSRFlags,r4,1,31,31					// C = carry bit
	rlwinm	r4,r4,1,0,30						// r4 = (val << count)
	or		r4,r4,r8							// r4 = (val << count) | (val >> (33 - count)) | (X << (count - 1))
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
roxl_l_dx_dy_0:
	rlwimi	rSRFlags,rSRFlags,28,31,31			// C = X
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXL.L Ix,Dy
	//
entry static roxl_l_ix_dy
	CYCLES(8,8,9)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	subfic	r9,rRX,33							// r9 = 33 - count
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	rlwinm	r6,rSRFlags,28,31,31				// r6 = X in the low bit
	subi	rRX,rRX,1							// rRX -= 1
	slw		r8,r6,rRX							// r8 = (X << (count - 1))
	srw		r9,r3,r9							// r9 = (val >> (33 - count))
	slw		r4,r3,rRX							// r4 = (val << (count - 1))
	or		r8,r8,r9							// r8 = (val >> (33 - count)) | (X << (count - 1))
	rlwimi	rSRFlags,r4,1,31,31					// C = carry bit
	rlwinm	r4,r4,1,0,30						// r4 = (val << count)
	or		r4,r4,r8							// r4 = (val << count) | (val >> (33 - count)) | (X << (count - 1))
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		ROXR -- Rotate right through extend
	//
	//================================================================================================

	//
	//		ROXR.B Dx,Dy
	//
entry static roxr_b_dx_dy
	CYCLES(6,6,9)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_B_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,9,15,22						// copy the byte, leaving room for the extend bit
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		roxr_b_dx_dy_0						// skip if zero
	rlwimi	r3,rSRFlags,4,23,23					// copy the X bit between the two copies of the byte
roxr_b_dx_dy_again:
	cmpwi	rRX,9								// greater than 9 bits?
	ble+	roxr_b_dx_dy_done					// if not, skip
	subi	rRX,rRX,9							// rRX -= 9
	b		roxr_b_dx_dy_again					// check again
roxr_b_dx_dy_done:
	subfic	rRX,rRX,32							// rRX = 32 - rRX
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
roxr_b_dx_dy_0:
	rlwimi	rSRFlags,rSRFlags,28,31,31			// C = X
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_BYTE(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXR.B Ix,Dy
	//
entry static roxr_b_ix_dy
	CYCLES(6,6,9)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,9,15,22						// copy the byte, leaving room for the extend bit
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwimi	r3,rSRFlags,4,23,23					// copy the X bit between the two copies of the byte
	add		r0,rRX,rRX
	subfic	rRX,rRX,32							// rRX = 32 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_BYTE_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,24,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_BYTE(r5)								// N = (r5 & 0x80)
	WRITE_B_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		ROXR.W Dx,Dy
	//
entry static roxr_w_dx_dy
	CYCLES(6,6,9)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_W_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	rlwimi	r3,r3,16,0,15						// copy the word
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		roxr_w_dx_dy_0						// skip if zero
roxr_w_dx_dy_again:
	cmpwi	rRX,17								// greater than 17 bits?
	ble+	roxr_w_dx_dy_done					// if not, skip
	subi	rRX,rRX,17							// rRX -= 17
	b		roxr_w_dx_dy_again					// check again
roxr_w_dx_dy_done:
	rlwimi	r3,r3,31,17,31						// pre-shift by 1 bit to make room for X
	subi	rRX,rRX,1							// rRX -= 1, to account for the pre-shifted bit
	rlwimi	r3,rSRFlags,11,16,16				// copy the X bit in between the two copies of r3
	subfic	rRX,rRX,32							// rRX = 32 - rRX
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
roxr_w_dx_dy_0:
	rlwimi	rSRFlags,rSRFlags,28,31,31			// C = X
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_WORD(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXR.W Ix,Dy
	//
entry static roxr_w_ix_dy
	CYCLES(6,6,9)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	rlwimi	r3,r3,17,0,14						// copy the word, leaving room for the extend bit
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	rlwimi	r3,rSRFlags,12,15,15				// copy the X bit between the two copies of the word
	add		r0,rRX,rRX
	subfic	rRX,rRX,32							// rRX = 32 - rRX
	add		rCycleCount,rCycleCount,r0
	CLR_V										// V = 0
	rlwnm	r4,r3,rRX,0,31						// r4 = r3 << rRX
	TRUNC_WORD_TO(r5,r4)						// get the byte result in r5
	rlwimi	rSRFlags,r4,16,31,31				// C = copy of the last bit shifted out
	cntlzw	r7,r5								// r7 = number of zeros in r5
	SET_N_WORD(r5)								// N = (r5 & 0x80)
	WRITE_W_REG(r5,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXR.W #1,(Ay)
	//
entry static roxr_w_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
roxr_w_ea:
	READ_W_AT(rEA)								// r3 = (Ay)
	rlwinm	r4,r3,31,17,31						// r4 = r3 << 1
	CLR_V										// V = 0
	rlwimi	r4,rSRFlags,15,16,16				// copy the low bit
	rlwimi	rSRFlags,r3,0,31,31					// C = copy of the last bit shifted out
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SET_X_FROM_C								// X = C
	SAVE_SR
	WRITE_W_AT(rEA)
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ROXR.W #1,-(Ay)
	//
entry static roxr_w_aym
	CYCLES(14,14,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		roxr_w_ea
	
	//================================================================================================

	//
	//		ROXR.W #1,(Ay)+
	//
entry static roxr_w_ayp
	CYCLES(12,12,9)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		roxr_w_ea
	
	//================================================================================================

	//
	//		ROXR.W #1,(Ay,d16)
	//
entry static roxr_w_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		roxr_w_ea
	
	//================================================================================================

	//
	//		ROXR.W #1,(Ay,Xn,d8)
	//
entry static roxr_w_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		roxr_w_ea

	//================================================================================================

	//
	//		ROXR.W #1,(xxx).W
	//		ROXR.W #1,(xxx).L
	//
entry static roxr_w_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		roxr_w_absl							// handle long absolute mode
roxr_w_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		roxr_w_ea
roxr_w_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		roxr_w_ea

	//================================================================================================
	//================================================================================================

	//
	//		ROXR.L Dx,Dy
	//
entry static roxr_l_dx_dy
	CYCLES(8,8,9)
	READ_B_REG(rRX,rRX)							// rRX = Dx
	READ_L_REG(r3,rRY)							// r3 = Dy
	andi.	rRX,rRX,0x3f						// everything is mod 64
	add		r0,rRX,rRX
	CLR_V										// V = 0
	add		rCycleCount,rCycleCount,r0
	beq		roxr_l_dx_dy_0						// special case for zero
roxr_l_dx_dy_again:
	cmpwi	rRX,33								// greater than 33 bits?
	ble+	roxr_w_dx_dy_done					// if not, skip
	subi	rRX,rRX,33							// rRX -= 33
roxr_l_dx_dy_done:
	subfic	r8,rRX,32							// r8 = 32 - count
	subfic	r9,rRX,33							// r9 = 33 - count
	rlwinm	r6,rSRFlags,28,31,31				// r6 = X in the low bit
	subi	rRX,rRX,1							// rRX -= 1
	slw		r8,r6,r8							// r8 = (X << (32 - count))
	slw		r9,r3,r9							// r9 = (val << (33 - count))
	srw		r4,r3,rRX							// r4 = (val >> (count - 1))
	or		r8,r8,r9							// r8 = (val << (33 - count)) | (X << (32 - count))
	rlwimi	rSRFlags,r4,0,31,31					// C = carry bit
	rlwinm	r4,r4,31,1,31						// r4 = (val >> count)
	or		r4,r4,r8							// r4 = (val >> count) | (val << (33 - count)) | (X << (32 - count))
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done
roxr_l_dx_dy_0:
	rlwimi	rSRFlags,rSRFlags,28,31,31			// C = X
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r5 & 0x80)
	SET_Z(r7)									// Z = (r5 == 0)
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ROXR.L Ix,Dy
	//
entry static roxr_l_ix_dy
	CYCLES(8,8,9)
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_REG(r3,rRY)							// r3 = Dy
	rlwinm	rRX,rRX,30,29,31					// rotate back to 0-7
	CLR_V										// V = 0
	rlwimi	rRX,r7,30,28,28						// convert a 0 count to 8
	subfic	r8,rRX,32							// r8 = 32 - count
	add		r0,rRX,rRX
	subfic	r9,rRX,33							// r9 = 33 - count
	add		rCycleCount,rCycleCount,r0
	rlwinm	r6,rSRFlags,28,31,31				// r6 = X in the low bit
	subi	rRX,rRX,1							// rRX -= 1
	slw		r8,r6,r8							// r8 = (X << (32 - count))
	slw		r9,r3,r9							// r9 = (val << (33 - count))
	srw		r4,r3,rRX							// r4 = (val >> (count - 1))
	or		r8,r8,r9							// r8 = (val << (33 - count)) | (X << (32 - count))
	rlwimi	rSRFlags,r4,0,31,31					// C = carry bit
	rlwinm	r4,r4,31,1,31						// r4 = (val >> count)
	or		r4,r4,r8							// r4 = (val >> count) | (val << (33 - count)) | (X << (32 - count))
	cntlzw	r7,r4								// r7 = number of zeros in r5
	SET_N_LONG(r4)								// N = (r5 & 0x80)
	WRITE_L_REG(r4,rRY)							// write the final register value
	SET_Z(r7)									// Z = (r5 == 0)
	SET_X_FROM_C								// X = C
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		SBCD -- Subtract BCD with carry
	//
	//================================================================================================

	//
	// 		SBCD.B -(Ay),-(Ax)
	//
entry static sbcd_b_aym_axm
	CYCLES(18,18,14)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rTempSave,rTempSave,1				// Ay = Ay - 1
	sub		rTempSave,rTempSave,r12				// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_B_AT(rTempSave)						// r3 = byte(Ay)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rEA,rEA,1							// Ax = Ax + 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_B_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	mr		r4,rRX
	LOAD_PC
sbcd_b_aym_axm_core:
	GET_X(r7)									// r7 = X
	LOAD_ICOUNT
	rlwinm	r5,r3,0,28,31						// r5 = r3 & 0x0f
	rlwinm	r6,r3,0,24,27						// r6 = r3 & 0xf0
	rlwinm	r8,r4,0,28,31						// r8 = r4 & 0x0f
	rlwinm	r9,r4,0,24,27						// r9 = r4 & 0xf0
	sub		r5,r5,r8							// r5 -= r8
	sub		r6,r6,r9							// r6 -= r9
	sub		r5,r5,r7							// r5 -= X
sbcd_b_aym_axm_adjust:
	cmplwi	r5,9								// do we need to adjust the low nibble?
	CLR_C										// clear carry
	ble		sbcd_b_aym_axm_adjust_a				// if not, skip
	subi	r6,r6,0x10							// borrow from the high nibble
	subi	r5,r5,6								// bring back in range
sbcd_b_aym_axm_adjust_a:
	cmplwi	r6,0x90								// do we need to adjust the high nibble?
	rlwinm	r4,r5,0,28,31						// r4 = final low nibble
	ble		sbcd_b_aym_axm_adjust_b				// if not, skip
	subi	r6,r6,0x60							// bring back in range
	SET_C										// carry
sbcd_b_aym_axm_adjust_b:
	rlwimi	r4,r6,0,24,27						// insert the high nibble
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_X_FROM_C								// X = C
	rlwinm	r7,r7,29,29,29						// make a Z bit in the proper location
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// only clear Z; never set it
	SAVE_SR										// only need to save SR; PC & ICOUNT will not have changed
	WRITE_B_AT(rEA)								// write the new value back to rEA
	b		executeLoopEndRestore				// return

	//================================================================================================

	//
	// 		SBCD.B Dy,Dx
	//
entry static sbcd_b_dy_dx
	CYCLES(6,6,4)
	READ_B_REG(r3,rRX)							// r3 = Dy
	READ_B_REG(r4,rRY)							// r4 = Dx
sbcd_b_dy_dx_core:
	GET_X(r7)									// r7 = X
	rlwinm	r5,r3,0,28,31						// r5 = r3 & 0x0f
	rlwinm	r6,r3,0,24,27						// r6 = r3 & 0xf0
	rlwinm	r8,r4,0,28,31						// r8 = r4 & 0x0f
	rlwinm	r9,r4,0,24,27						// r9 = r4 & 0xf0
	sub		r5,r5,r8							// r5 -= r8
	sub		r6,r6,r9							// r6 -= r9
	sub		r5,r5,r7							// r5 -= X
sbcd_b_dy_dx_adjust:
	cmplwi	r5,9								// do we need to adjust the low nibble?
	CLR_C										// clear carry
	ble		sbcd_b_dy_dx_adjust_a				// if not, skip
	subi	r6,r6,0x10							// borrow from the high nibble
	subi	r5,r5,6								// bring back in range
sbcd_b_dy_dx_adjust_a:
	cmplwi	r6,0x90								// do we need to adjust the high nibble?
	rlwinm	r4,r5,0,28,31						// r4 = final low nibble
	ble		sbcd_b_dy_dx_adjust_b				// if not, skip
	subi	r6,r6,0x60							// bring back in range
	SET_C										// carry
sbcd_b_dy_dx_adjust_b:
	rlwimi	r4,r6,0,24,27						// insert the high nibble
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_X_FROM_C								// X = C
	rlwinm	r7,r7,29,29,29						// make a Z bit in the proper location
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// only clear Z; never set it
	WRITE_B_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// return

	//================================================================================================
	//
	//		SCC -- Set conditional
	//
	//================================================================================================

	//
	//		cc == T == 1
	//
	
	//================================================================================================

	//
	//		ST Dy
	//
entry static scc_t_dy
	CYCLES(6,4,1)
	li		r4,-1								// r4 = -1
	WRITE_B_REG(r4,rRY)							// write to the register
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		ST (Ay)
	//
entry static scc_t_ay0
	CYCLES(12,10,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_t_ea:
	li		r4,-1								// r4 = -1
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		ST -(Ay)
	//
entry static scc_t_aym
	CYCLES(14,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_t_ea
	
	//================================================================================================

	//
	//		ST (Ay)+
	//
entry static scc_t_ayp
	CYCLES(12,10,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_t_ea
	
	//================================================================================================

	//
	//		ST (Ay,d16)
	//
entry static scc_t_ayd
	CYCLES(16,14,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_t_ea
	
	//================================================================================================

	//
	//		ST (Ay,Xn,d8)
	//
entry static scc_t_ayid
	bl		computeEA110
	CYCLES(18,16,5)
	b		scc_t_ea

	//================================================================================================

	//
	//		ST (xxx).W
	//		ST (xxx).L
	//
entry static scc_t_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_t_absl							// handle long absolute mode
scc_t_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,14,8)
	b		scc_t_ea
scc_t_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,18,8)
	b		scc_t_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == F == 0
	//
	
	//================================================================================================

	//
	//		SF Dy
	//
entry static scc_f_dy
	CYCLES(4,4,1)
	li		r4,0								// r4 = 0
	WRITE_B_REG(r4,rRY)							// write to the register
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SF (Ay)
	//
entry static scc_f_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_f_ea:
	li		r4,0								// r4 = 0
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SF -(Ay)
	//
entry static scc_f_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_f_ea
	
	//================================================================================================

	//
	//		SF (Ay)+
	//
entry static scc_f_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_f_ea
	
	//================================================================================================

	//
	//		SF (Ay,d16)
	//
entry static scc_f_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_f_ea
	
	//================================================================================================

	//
	//		SF (Ay,Xn,d8)
	//
entry static scc_f_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_f_ea

	//================================================================================================

	//
	//		SF (xxx).W
	//		SF (xxx).L
	//
entry static scc_f_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_f_absl							// handle long absolute mode
scc_f_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_f_ea
scc_f_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_f_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == HI == ~C & ~Z = ~(C | Z)
	//
	
	//================================================================================================

	//
	//		SHI Dy
	//
entry static scc_hi_dy
	CYCLES(4,4,1)
	rlwinm	r4,rSRFlags,30,31,31				// r4 = Z shifted to match C position
	nor		r4,r4,rSRFlags						// r4 = ~(r3 | C)
	rlwinm	r4,r4,0,31,31						// r4 = ~(C | Z) & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SHI (Ay)
	//
entry static scc_hi_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_hi_ea:
	rlwinm	r4,rSRFlags,30,31,31				// r4 = Z shifted to match C position
	nor		r4,r4,rSRFlags						// r4 = ~(r3 | C)
	rlwinm	r4,r4,0,31,31						// r4 = ~(C | Z) & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SHI -(Ay)
	//
entry static scc_hi_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_hi_ea
	
	//================================================================================================

	//
	//		SHI (Ay)+
	//
entry static scc_hi_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_hi_ea
	
	//================================================================================================

	//
	//		SHI (Ay,d16)
	//
entry static scc_hi_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_hi_ea
	
	//================================================================================================

	//
	//		SHI (Ay,Xn,d8)
	//
entry static scc_hi_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_hi_ea

	//================================================================================================

	//
	//		SHI (xxx).W
	//		SHI (xxx).L
	//
entry static scc_hi_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_hi_absl							// handle long absolute mode
scc_hi_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_hi_ea
scc_hi_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_hi_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == LS == ~C & ~Z = ~(C | Z)
	//
	
	//================================================================================================

	//
	//		SLS Dy
	//
entry static scc_ls_dy
	CYCLES(4,4,1)
	rlwinm	r4,rSRFlags,30,31,31				// r4 = Z shifted to match C position
	or		r4,r4,rSRFlags						// r4 = (r4 | C)
	rlwinm	r4,r4,0,31,31						// r4 = (C | Z) & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SLS (Ay)
	//
entry static scc_ls_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_ls_ea:
	rlwinm	r4,rSRFlags,30,31,31				// r4 = Z shifted to match C position
	or		r4,r4,rSRFlags						// r4 = (r4 | C)
	rlwinm	r4,r4,0,31,31						// r4 = (C | Z) & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SLS -(Ay)
	//
entry static scc_ls_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_ls_ea
	
	//================================================================================================

	//
	//		SLS (Ay)+
	//
entry static scc_ls_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_ls_ea
	
	//================================================================================================

	//
	//		SLS (Ay,d16)
	//
entry static scc_ls_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_ls_ea
	
	//================================================================================================

	//
	//		SLS (Ay,Xn,d8)
	//
entry static scc_ls_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_ls_ea

	//================================================================================================

	//
	//		SLS (xxx).W
	//		SLS (xxx).L
	//
entry static scc_ls_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_ls_absl							// handle long absolute mode
scc_ls_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_ls_ea
scc_ls_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_ls_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == CC == ~C
	//
	
	//================================================================================================

	//
	//		SCC Dy
	//
entry static scc_cc_dy
	CYCLES(4,4,1)
	not		r4,rSRFlags							// r4 = ~C
	rlwinm	r4,r4,0,31,31						// r4 = ~C & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SCC (Ay)
	//
entry static scc_cc_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_cc_ea:
	not		r4,rSRFlags							// r4 = ~C
	rlwinm	r4,r4,0,31,31						// r4 = ~C & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SCC -(Ay)
	//
entry static scc_cc_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_cc_ea
	
	//================================================================================================

	//
	//		SCC (Ay)+
	//
entry static scc_cc_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_cc_ea
	
	//================================================================================================

	//
	//		SCC (Ay,d16)
	//
entry static scc_cc_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_cc_ea
	
	//================================================================================================

	//
	//		SCC (Ay,Xn,d8)
	//
entry static scc_cc_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_cc_ea

	//================================================================================================

	//
	//		SCC (xxx).W
	//		SCC (xxx).L
	//
entry static scc_cc_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_cc_absl							// handle long absolute mode
scc_cc_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_cc_ea
scc_cc_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_cc_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == CS == C
	//
	
	//================================================================================================

	//
	//		SCS Dy
	//
entry static scc_cs_dy
	CYCLES(4,4,1)
	rlwinm	r4,rSRFlags,0,31,31					// r4 = C & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SCS (Ay)
	//
entry static scc_cs_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_cs_ea:
	rlwinm	r4,rSRFlags,0,31,31					// r4 = C & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SCS -(Ay)
	//
entry static scc_cs_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_cs_ea
	
	//================================================================================================

	//
	//		SCS (Ay)+
	//
entry static scc_cs_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_cs_ea
	
	//================================================================================================

	//
	//		SCS (Ay,d16)
	//
entry static scc_cs_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_cs_ea
	
	//================================================================================================

	//
	//		SCS (Ay,Xn,d8)
	//
entry static scc_cs_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_cs_ea

	//================================================================================================

	//
	//		SCS (xxx).W
	//		SCS (xxx).L
	//
entry static scc_cs_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_cs_absl							// handle long absolute mode
scc_cs_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_cs_ea
scc_cs_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_cs_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == NE == ~Z
	//
	
	//================================================================================================

	//
	//		SNE Dy
	//
entry static scc_ne_dy
	CYCLES(4,4,1)
	not		r4,rSRFlags							// r4 = ~Z
	rlwinm	r4,r4,30,31,31						// r4 = ~Z & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SNE (Ay)
	//
entry static scc_ne_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_ne_ea:
	not		r4,rSRFlags							// r4 = ~Z
	rlwinm	r4,r4,30,31,31						// r4 = ~Z & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SNE -(Ay)
	//
entry static scc_ne_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_ne_ea
	
	//================================================================================================

	//
	//		SNE (Ay)+
	//
entry static scc_ne_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_ne_ea
	
	//================================================================================================

	//
	//		SNE (Ay,d16)
	//
entry static scc_ne_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_ne_ea
	
	//================================================================================================

	//
	//		SNE (Ay,Xn,d8)
	//
entry static scc_ne_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_ne_ea

	//================================================================================================

	//
	//		SNE (xxx).W
	//		SNE (xxx).L
	//
entry static scc_ne_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_ne_absl							// handle long absolute mode
scc_ne_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_ne_ea
scc_ne_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_ne_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == EQ == Z
	//
	
	//================================================================================================

	//
	//		SEQ Dy
	//
entry static scc_eq_dy
	CYCLES(4,4,1)
	rlwinm	r4,rSRFlags,30,31,31				// r4 = Z & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SEQ (Ay)
	//
entry static scc_eq_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_eq_ea:
	rlwinm	r4,rSRFlags,30,31,31				// r4 = Z & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SEQ -(Ay)
	//
entry static scc_eq_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_eq_ea
	
	//================================================================================================

	//
	//		SEQ (Ay)+
	//
entry static scc_eq_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_eq_ea
	
	//================================================================================================

	//
	//		SEQ (Ay,d16)
	//
entry static scc_eq_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_eq_ea
	
	//================================================================================================

	//
	//		SEQ (Ay,Xn,d8)
	//
entry static scc_eq_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_eq_ea

	//================================================================================================

	//
	//		SEQ (xxx).W
	//		SEQ (xxx).L
	//
entry static scc_eq_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_eq_absl							// handle long absolute mode
scc_eq_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_eq_ea
scc_eq_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_eq_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == VC == ~V
	//
	
	//================================================================================================

	//
	//		SVC Dy
	//
entry static scc_vc_dy
	CYCLES(4,4,1)
	not		r4,rSRFlags							// r4 = ~V
	rlwinm	r4,r4,31,31,31						// r4 = ~V & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SVC (Ay)
	//
entry static scc_vc_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_vc_ea:
	not		r4,rSRFlags							// r4 = ~V
	rlwinm	r4,r4,31,31,31						// r4 = ~V & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SVC -(Ay)
	//
entry static scc_vc_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_vc_ea
	
	//================================================================================================

	//
	//		SVC (Ay)+
	//
entry static scc_vc_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_vc_ea
	
	//================================================================================================

	//
	//		SVC (Ay,d16)
	//
entry static scc_vc_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_vc_ea
	
	//================================================================================================

	//
	//		SVC (Ay,Xn,d8)
	//
entry static scc_vc_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_vc_ea

	//================================================================================================

	//
	//		SVC (xxx).W
	//		SVC (xxx).L
	//
entry static scc_vc_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_vc_absl							// handle long absolute mode
scc_vc_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_vc_ea
scc_vc_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_vc_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == VS == C
	//
	
	//================================================================================================

	//
	//		SVS Dy
	//
entry static scc_vs_dy
	CYCLES(4,4,1)
	rlwinm	r4,rSRFlags,31,31,31				// r4 = V & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SVS (Ay)
	//
entry static scc_vs_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_vs_ea:
	rlwinm	r4,rSRFlags,31,31,31				// r4 = V & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SVS -(Ay)
	//
entry static scc_vs_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_vs_ea
	
	//================================================================================================

	//
	//		SVS (Ay)+
	//
entry static scc_vs_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_vs_ea
	
	//================================================================================================

	//
	//		SVS (Ay,d16)
	//
entry static scc_vs_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_vs_ea
	
	//================================================================================================

	//
	//		SVS (Ay,Xn,d8)
	//
entry static scc_vs_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_vs_ea

	//================================================================================================

	//
	//		SVS (xxx).W
	//		SVS (xxx).L
	//
entry static scc_vs_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_vs_absl							// handle long absolute mode
scc_vs_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_vs_ea
scc_vs_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_vs_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == PL == ~N
	//
	
	//================================================================================================

	//
	//		SPL Dy
	//
entry static scc_pl_dy
	CYCLES(4,4,1)
	not		r4,rSRFlags							// r4 = ~N
	rlwinm	r4,r4,29,31,31						// r4 = ~N & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SPL (Ay)
	//
entry static scc_pl_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_pl_ea:
	not		r4,rSRFlags							// r4 = ~N
	rlwinm	r4,r4,29,31,31						// r4 = ~N & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SPL -(Ay)
	//
entry static scc_pl_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_pl_ea
	
	//================================================================================================

	//
	//		SPL (Ay)+
	//
entry static scc_pl_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_pl_ea
	
	//================================================================================================

	//
	//		SPL (Ay,d16)
	//
entry static scc_pl_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_pl_ea
	
	//================================================================================================

	//
	//		SPL (Ay,Xn,d8)
	//
entry static scc_pl_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_pl_ea

	//================================================================================================

	//
	//		SPL (xxx).W
	//		SPL (xxx).L
	//
entry static scc_pl_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_pl_absl							// handle long absolute mode
scc_pl_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_pl_ea
scc_pl_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_pl_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == MI == N
	//
	
	//================================================================================================

	//
	//		SMI Dy
	//
entry static scc_mi_dy
	CYCLES(4,4,1)
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SMI (Ay)
	//
entry static scc_mi_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_mi_ea:
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N & 1
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SMI -(Ay)
	//
entry static scc_mi_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_mi_ea
	
	//================================================================================================

	//
	//		SMI (Ay)+
	//
entry static scc_mi_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_mi_ea
	
	//================================================================================================

	//
	//		SMI (Ay,d16)
	//
entry static scc_mi_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_mi_ea
	
	//================================================================================================

	//
	//		SMI (Ay,Xn,d8)
	//
entry static scc_mi_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_mi_ea

	//================================================================================================

	//
	//		SMI (xxx).W
	//		SMI (xxx).L
	//
entry static scc_mi_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_mi_absl							// handle long absolute mode
scc_mi_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_mi_ea
scc_mi_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_mi_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == GE = ~(N ^ V)
	//
	
	//================================================================================================

	//
	//		SGE Dy
	//
entry static scc_ge_dy
	CYCLES(4,4,1)
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	eqv		r4,r4,r3							// r4 = ~(N ^ V)
	rlwinm	r4,r4,0,31,31						// only the low bit
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SGE (Ay)
	//
entry static scc_ge_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_ge_ea:
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	eqv		r4,r4,r3							// r4 = ~(N ^ V)
	rlwinm	r4,r4,0,31,31						// only the low bit
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SGE -(Ay)
	//
entry static scc_ge_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_ge_ea
	
	//================================================================================================

	//
	//		SGE (Ay)+
	//
entry static scc_ge_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_ge_ea
	
	//================================================================================================

	//
	//		SGE (Ay,d16)
	//
entry static scc_ge_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_ge_ea
	
	//================================================================================================

	//
	//		SGE (Ay,Xn,d8)
	//
entry static scc_ge_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_ge_ea

	//================================================================================================

	//
	//		SGE (xxx).W
	//		SGE (xxx).L
	//
entry static scc_ge_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_ge_absl							// handle long absolute mode
scc_ge_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_ge_ea
scc_ge_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_ge_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == LT = (N ^ V)
	//
	
	//================================================================================================

	//
	//		SLT Dy
	//
entry static scc_lt_dy
	CYCLES(4,4,1)
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	xor		r4,r4,r3							// r4 = (N ^ V)
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SLT (Ay)
	//
entry static scc_lt_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_lt_ea:
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	xor		r4,r4,r3							// r4 = (N ^ V)
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SLT -(Ay)
	//
entry static scc_lt_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_lt_ea
	
	//================================================================================================

	//
	//		SLT (Ay)+
	//
entry static scc_lt_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_lt_ea
	
	//================================================================================================

	//
	//		SLT (Ay,d16)
	//
entry static scc_lt_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_lt_ea
	
	//================================================================================================

	//
	//		SLT (Ay,Xn,d8)
	//
entry static scc_lt_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_lt_ea

	//================================================================================================

	//
	//		SLT (xxx).W
	//		SLT (xxx).L
	//
entry static scc_lt_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_lt_absl							// handle long absolute mode
scc_lt_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_lt_ea
scc_lt_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_lt_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == GT = ~(N ^ V) & ~Z
	//
	
	//================================================================================================

	//
	//		SGT Dy
	//
entry static scc_gt_dy
	CYCLES(4,4,1)
	rlwinm	r5,rSRFlags,30,31,31				// r5 = Z
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	xori	r5,r5,1								// r5 = ~Z
	eqv		r4,r4,r3							// r4 = ~(N ^ V)
	and		r4,r4,r5							// r4 = ~(N ^ V) & ~Z
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SGT (Ay)
	//
entry static scc_gt_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_gt_ea:
	rlwinm	r5,rSRFlags,30,31,31				// r5 = Z
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	xori	r5,r5,1								// r5 = ~Z
	eqv		r4,r4,r3							// r4 = ~(N ^ V)
	and		r4,r4,r5							// r4 = ~(N ^ V) & ~Z
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SGT -(Ay)
	//
entry static scc_gt_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_gt_ea
	
	//================================================================================================

	//
	//		SGT (Ay)+
	//
entry static scc_gt_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_gt_ea
	
	//================================================================================================

	//
	//		SGT (Ay,d16)
	//
entry static scc_gt_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_gt_ea
	
	//================================================================================================

	//
	//		SGT (Ay,Xn,d8)
	//
entry static scc_gt_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_gt_ea

	//================================================================================================

	//
	//		SGT (xxx).W
	//		SGT (xxx).L
	//
entry static scc_gt_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_gt_absl							// handle long absolute mode
scc_gt_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_gt_ea
scc_gt_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_gt_ea

	//================================================================================================
	//================================================================================================

	//
	//		cc == LE = (N ^ V) | Z
	//
	
	//================================================================================================

	//
	//		SLE Dy
	//
entry static scc_le_dy
	CYCLES(4,4,1)
	rlwinm	r5,rSRFlags,30,31,31				// r5 = Z
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	xor		r4,r4,r3							// r4 = (N ^ V)
	or		r4,r4,r5							// r4 = (N ^ V) | Z
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_REG(r4,rRY)							// write to the register
#if (A68000_CHIP == 68000)
	sub		rCycleCount,rCycleCount,r4
	sub		rCycleCount,rCycleCount,r4
#endif
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SLE (Ay)
	//
entry static scc_le_ay0
	CYCLES(12,12,8)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
scc_le_ea:
	rlwinm	r5,rSRFlags,30,31,31				// r5 = Z
	rlwinm	r3,rSRFlags,31,31,31				// r3 = V
	rlwinm	r4,rSRFlags,29,31,31				// r4 = N
	xor		r4,r4,r3							// r4 = (N ^ V)
	or		r4,r4,r5							// r4 = (N ^ V) | Z
	neg		r4,r4								// r4 = -r4 (0 -> 0, 1 -> -1)
	WRITE_B_AT(rEA)								// write to memory
	b		executeLoopEndRestore

	//================================================================================================

	//
	//		SLE -(Ay)
	//
entry static scc_le_aym
	CYCLES(14,14,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 2
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		scc_le_ea
	
	//================================================================================================

	//
	//		SLE (Ay)+
	//
entry static scc_le_ayp
	CYCLES(12,12,8)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		scc_le_ea
	
	//================================================================================================

	//
	//		SLE (Ay,d16)
	//
entry static scc_le_ayd
	CYCLES(16,16,8)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		scc_le_ea
	
	//================================================================================================

	//
	//		SLE (Ay,Xn,d8)
	//
entry static scc_le_ayid
	bl		computeEA110
	CYCLES(18,18,5)
	b		scc_le_ea

	//================================================================================================

	//
	//		SLE (xxx).W
	//		SLE (xxx).L
	//
entry static scc_le_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		scc_le_absl							// handle long absolute mode
scc_le_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,8)
	b		scc_le_ea
scc_le_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,8)
	b		scc_le_ea

	//================================================================================================
	//
	//		SUBA -- Subtract from address register
	//
	//================================================================================================

	//
	//		SUBA.W Dy,Ax
	//
entry static suba_w_dy_ax
	CYCLES(8,8,1)
	READ_W_REG_EXT(r3,rRY)						// r3 = Dy (sign-extended)
	READ_L_AREG(r4,rRX)							// r4 = Ax
	sub		r4,r4,r3							// r4 -= r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBA.W Ay,Ax
	//
entry static suba_w_ay_ax
	CYCLES(8,8,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	READ_L_AREG(r4,rRX)							// r4 = Ax
	extsh	r3,r3								// r3 = ext(r3)
	sub		r4,r4,r3							// r4 -= r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBA.W (Ay),Ax
	//
entry static suba_w_ay0_ax
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
suba_w_ea_ax:
	READ_W_AT(rEA)								// r3 = word(rEA)
suba_w_ea_ax_post:
	READ_L_AREG(r4,rRX)							// r4 = Ax
	extsh	r3,r3								// r3 = ext(r3)
	sub		r4,r4,r3							// r4 -= r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBA.W -(Ay),Ax
	//
entry static suba_w_aym_ax
	CYCLES(14,14,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		suba_w_ea_ax
	
	//================================================================================================

	//
	//		SUBA.W (Ay)+,Ax
	//
entry static suba_w_ayp_ax
	CYCLES(12,12,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		suba_w_ea_ax
	
	//================================================================================================

	//
	//		SUBA.W (Ay,d16),Ax
	//
entry static suba_w_ayd_ax
	CYCLES(16,16,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		suba_w_ea_ax
	
	//================================================================================================

	//
	//		SUBA.W (Ay,Xn,d8),Ax
	//
entry static suba_w_ayid_ax
	bl		computeEA110
	CYCLES(18,18,0)
	b		suba_w_ea_ax

	//================================================================================================

	//
	//		SUBA.W (xxx).W,Ax
	//		SUBA.W (xxx).L,Ax
	//		SUBA.W (d16,PC),Ax
	//		SUBA.W (d8,PC,Xn),Ax
	//
entry static suba_w_other_ax
	bl		fetchEAWord111
	CYCLES(12,12,0)
	extsh	r3,r3
	b		suba_w_ea_ax_post

	//================================================================================================
	//================================================================================================

	//
	//		SUBA.L Dy,Ax
	//
entry static suba_l_dy_ax
	CYCLES(8,8,1)
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_AREG(r4,rRX)							// r4 = Ax
	sub		r4,r4,r3							// r4 -= r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBA.L Ay,Ax
	//
entry static suba_l_ay_ax
	CYCLES(8,8,1)
	READ_L_AREG(r3,rRY)							// r3 = Ay
	READ_L_AREG(r4,rRX)							// r4 = Ax
	sub		r4,r4,r3							// r4 -= r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBA.L (Ay),Ax
	//
entry static suba_l_ay0_ax
	CYCLES(10,10,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
suba_l_ea_ax:
	READ_L_AT(rEA)								// r3 = long(rEA)
suba_l_ea_ax_post:
	READ_L_AREG(r4,rRX)							// r4 = Ax
	sub		r4,r4,r3							// r4 -= r3
	WRITE_L_AREG(r4,rRX)						// Ax = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBA.L -(Ay),Ax
	//
entry static suba_l_aym_ax
	CYCLES(12,12,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		suba_l_ea_ax
	
	//================================================================================================

	//
	//		SUBA.L (Ay)+,Ax
	//
entry static suba_l_ayp_ax
	CYCLES(10,10,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		suba_l_ea_ax
	
	//================================================================================================

	//
	//		SUBA.L (Ay,d16),Ax
	//
entry static suba_l_ayd_ax
	CYCLES(14,14,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		suba_l_ea_ax
	
	//================================================================================================

	//
	//		SUBA.L (Ay,Xn,d8),Ax
	//
entry static suba_l_ayid_ax
	bl		computeEA110
	CYCLES(16,16,0)
	b		suba_l_ea_ax

	//================================================================================================

	//
	//		SUBA.L (xxx).W,Ax
	//		SUBA.L (xxx).L,Ax
	//		SUBA.L (d16,PC),Ax
	//		SUBA.L (d8,PC,Xn),Ax
	//
entry static suba_l_other_ax
	bl		fetchEALong111
	CYCLES(12,12,0)
	b		suba_l_ea_ax_post

	//================================================================================================
	//
	//		SUBI -- Subtract immediate from effective address
	//
	//================================================================================================

	//
	//		SUBI.B #xx,Dy
	//
entry static subi_b_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	READ_B_REG(r3,rRY)							// r3 = Dy
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBI.B #xx,(Ay)
	//
entry static subi_b_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
subi_b_imm_ea:
	READ_B_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBI.B #xx,-(Ay)
	//
entry static subi_b_imm_aym
	CYCLES(18,18,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	subi	rEA,rEA,1							// rEA -= 1
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		subi_b_imm_ea
	
	//================================================================================================

	//
	//		SUBI.B #xx,(Ay)+
	//
entry static subi_b_imm_ayp
	CYCLES(16,16,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	addi	r3,rEA,1							// r3 = rEA + 1
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		subi_b_imm_ea
	
	//================================================================================================

	//
	//		SUBI.B #xx,(Ay,d16)
	//
entry static subi_b_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		subi_b_imm_ea
	
	//================================================================================================

	//
	//		SUBI.B #xx,(Ay,Xn,d8)
	//
entry static subi_b_imm_ayid
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		subi_b_imm_ea

	//================================================================================================

	//
	//		SUBI.B (xxx).W,Ax
	//		SUBI.B (xxx).L,Ax
	//
entry static subi_b_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_BYTE(rRX)					// rRX = immediate value
	SAVE_SR
	bne		subi_b_imm_absl						// handle long absolute mode
subi_b_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		subi_b_imm_ea
subi_b_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		subi_b_imm_ea

	//================================================================================================
	//================================================================================================

	//
	//		SUBI.W #xx,Dy
	//
entry static subi_w_imm_dy
	CYCLES(8,8,1)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	READ_W_REG(r3,rRY)							// r3 = Dy
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBI.W #xx,(Ay)
	//
entry static subi_w_imm_ay0
	CYCLES(16,16,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
subi_w_imm_ea:
	READ_W_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBI.W #xx,-(Ay)
	//
entry static subi_w_imm_aym
	CYCLES(18,18,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		subi_w_imm_ea
	
	//================================================================================================

	//
	//		SUBI.W #xx,(Ay)+
	//
entry static subi_w_imm_ayp
	CYCLES(16,16,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		subi_w_imm_ea
	
	//================================================================================================

	//
	//		SUBI.W #xx,(Ay,d16)
	//
entry static subi_w_imm_ayd
	CYCLES(20,20,6)
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		subi_w_imm_ea
	
	//================================================================================================

	//
	//		SUBI.W #xx,(Ay,Xn,d8)
	//
entry static subi_w_imm_ayid
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	bl		computeEA110
	CYCLES(22,22,3)
	b		subi_w_imm_ea

	//================================================================================================

	//
	//		SUBI.W (xxx).W,Ax
	//		SUBI.W (xxx).L,Ax
	//
entry static subi_w_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG(rRX)						// rRX = immediate value
	SAVE_SR
	bne		subi_w_imm_absl						// handle long absolute mode
subi_w_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(20,20,6)
	b		subi_w_imm_ea
subi_w_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(24,24,6)
	b		subi_w_imm_ea

	//================================================================================================
	//================================================================================================

	//
	//		SUBI.L #xx,Dy
	//
entry static subi_l_imm_dy
	CYCLES(16,14,1)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	READ_L_REG(r3,rRY)							// r3 = Dy
	subc	r4,r3,rRX							// r4 = r3 - rRX
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBI.L #xx,(Ay)
	//
entry static subi_l_imm_ay0
	CYCLES(28,28,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
subi_l_imm_ea:
	READ_L_AT(rEA)								// r3 = byte(rEA)
	subc	r4,r3,rRX							// r4 = r3 - rRX
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBI.L #xx,-(Ay)
	//
entry static subi_l_imm_aym
	CYCLES(30,30,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		subi_l_imm_ea
	
	//================================================================================================

	//
	//		SUBI.L #xx,(Ay)+
	//
entry static subi_l_imm_ayp
	CYCLES(28,28,8)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		subi_l_imm_ea
	
	//================================================================================================

	//
	//		SUBI.L #xx,(Ay,d16)
	//
entry static subi_l_imm_ayd
	CYCLES(32,32,7)
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		subi_l_imm_ea
	
	//================================================================================================

	//
	//		SUBI.L #xx,(Ay,Xn,d8)
	//
entry static subi_l_imm_ayid
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	bl		computeEA110
	CYCLES(34,34,4)
	b		subi_l_imm_ea

	//================================================================================================

	//
	//		SUBI.L (xxx).W,Ax
	//		SUBI.L (xxx).L,Ax
	//
entry static subi_l_imm_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	READ_OPCODE_ARG_LONG(rRX)					// rRX = immediate value
	SAVE_SR
	bne		subi_l_imm_absl						// handle long absolute mode
subi_l_imm_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(32,32,7)
	b		subi_l_imm_ea
subi_l_imm_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(36,36,7)
	b		subi_l_imm_ea

	//================================================================================================
	//
	//		SUBQ -- Subtract 3-bit immediate from effective address
	//
	//================================================================================================

	//
	//		SUBQ.B #xx,Dy
	//
entry static subq_b_ix_dy
	CYCLES(4,4,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_B_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBQ.B #xx,(Ay)
	//
entry static subq_b_ix_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
subq_b_ix_ea:
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	READ_B_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBQ.B #xx,-(Ay)
	//
entry static subq_b_ix_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		subq_b_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.B #xx,(Ay)+
	//
entry static subq_b_ix_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		subq_b_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.B #xx,(Ay,d16)
	//
entry static subq_b_ix_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		subq_b_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.B #xx,(Ay,Xn,d8)
	//
entry static subq_b_ix_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		subq_b_ix_ea

	//================================================================================================

	//
	//		SUBQ.B (xxx).W,Ax
	//		SUBQ.B (xxx).L,Ax
	//
entry static subq_b_ix_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		subq_b_ix_absl						// handle long absolute mode
subq_b_ix_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		subq_b_ix_ea
subq_b_ix_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		subq_b_ix_ea

	//================================================================================================
	//================================================================================================

	//
	//		SUBQ.W #xx,Dy
	//
entry static subq_w_ix_dy
	CYCLES(4,4,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_W_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBQ.W #xx,Ay
	//
entry static subq_w_ix_ay
	CYCLES(8,8,6)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	sub		r4,r3,rRX							// r4 = r3 - rRX
	WRITE_L_AREG(r4,rRY)						// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBQ.W #xx,(Ay)
	//
entry static subq_w_ix_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
subq_w_ix_ea:
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	READ_W_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRX							// r4 = r3 - rRX
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result word-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBQ.W #xx,-(Ay)
	//
entry static subq_w_ix_aym
	CYCLES(14,14,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		subq_w_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.W #xx,(Ay)+
	//
entry static subq_w_ix_ayp
	CYCLES(12,12,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		subq_w_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.W #xx,(Ay,d16)
	//
entry static subq_w_ix_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		subq_w_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.W #xx,(Ay,Xn,d8)
	//
entry static subq_w_ix_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		subq_w_ix_ea

	//================================================================================================

	//
	//		SUBQ.W (xxx).W,Ax
	//		SUBQ.W (xxx).L,Ax
	//
entry static subq_w_ix_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		subq_w_ix_absl						// handle long absolute mode
subq_w_ix_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		subq_w_ix_ea
subq_w_ix_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		subq_w_ix_ea

	//================================================================================================
	//================================================================================================

	//
	//		SUBQ.L #xx,Dy
	//
entry static subq_l_ix_dy
	CYCLES(8,8,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_REG(r3,rRY)							// r3 = Dy
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	subc	r4,r3,rRX							// r4 = r3 - rRX
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRY)							// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBQ.L #xx,Ay
	//
entry static subq_l_ix_ay
	CYCLES(8,8,1)
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	READ_L_AREG(r3,rRY)							// r3 = Ay
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	sub		r4,r3,rRX							// r4 = r3 - rRX
	WRITE_L_AREG(r4,rRY)						// Dy = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUBQ.L #xx,(Ay)
	//
entry static subq_l_ix_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
subq_l_ix_ea:
	rlwinm	rRX,r11,23,29,31					// rRX = immediate value
	cntlzw	r7,rRX								// r7 = number of zeros in rRX
	rlwimi	rRX,r7,30,28,28						// convert zero to 8
	READ_L_AT(rEA)								// r3 = byte(rEA)
	subc	r4,r3,rRX							// r4 = r3 - rRX
	CLR_C										// C = 0
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUBQ.L #xx,-(Ay)
	//
entry static subq_l_ix_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		subq_l_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.L #xx,(Ay)+
	//
entry static subq_l_ix_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		subq_l_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.L #xx,(Ay,d16)
	//
entry static subq_l_ix_ayd
	CYCLES(24,24,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		subq_l_ix_ea
	
	//================================================================================================

	//
	//		SUBQ.L #xx,(Ay,Xn,d8)
	//
entry static subq_l_ix_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		subq_l_ix_ea

	//================================================================================================

	//
	//		SUBQ.L (xxx).W,Ax
	//		SUBQ.L (xxx).L,Ax
	//
entry static subq_l_ix_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		subq_l_ix_absl						// handle long absolute mode
subq_l_ix_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		subq_l_ix_ea
subq_l_ix_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		subq_l_ix_ea

	//================================================================================================
	//
	//		SUBX -- Subtract extended
	//
	//================================================================================================

	//
	//		SUBX.B -(Ay),-(Ax)
	//
entry static subx_b_aym_axm
	CYCLES(18,18,10)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rTempSave,rTempSave,1				// Ay = Ay - 1
	sub		rTempSave,rTempSave,r12				// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_B_AT(rTempSave)						// r3 = byte(Ay)
	addi	r12,rRX,1*4							// r12 = rRX + 4
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	subi	rEA,rEA,1							// Ax = Ax + 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_B_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
subx_b_aym_axm_core:
	GET_X(r7)									// r7 = X
	LOAD_ICOUNT
	add		r7,r7,rRX							// r7 = X + rRX
	sub		r4,r3,r7							// r4 = r3 - rRX - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	SAVE_SR
	WRITE_B_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	// 		SUBX.B Dy,Dx
	//
entry static subx_b_dy_dx
	CYCLES(4,4,2)
	READ_B_REG(rRY,rRY)							// rRY = Dy
	READ_B_REG(r3,rRX)							// r3 = Dx
subx_b_dy_dx_core:
	GET_X(r7)									// r7 = X
	add		r7,r7,rRY							// r7 = X + rRY
	sub		r4,r3,r7							// r4 = r3 - rRX - X
	xor		r5,r3,rRY 		// AK 2003-07-31	 (was rRx)	// r5 = r3 ^ rRX
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	WRITE_B_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		SUBX.W -(Ay),-(Ax)
	//
entry static subx_w_aym_axm
	CYCLES(18,18,10)
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	subi	rTempSave,rTempSave,2				// Ay = Ay - 1
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_W_AT(rTempSave)						// r3 = byte(Ay)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	subi	rEA,rEA,2							// Ax = Ax + 1
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_W_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
subx_w_aym_axm_core:
	GET_X(r7)									// r7 = X
	LOAD_ICOUNT
	add		r7,r7,rRX							// r7 = X + rRX
	sub		r4,r3,r7							// r4 = r3 - rRX - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	SAVE_SR
	WRITE_W_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	// 	SUBX.W Dy,Dx
	//
entry static subx_w_dy_dx
	CYCLES(4,4,2)
	READ_W_REG(rRY,rRY)							// rRY = Dy
	READ_W_REG(r3,rRX)							// r3 = Dx
subx_w_dy_dx_core:
	GET_X(r7)									// r7 = X
	add		r7,r7,rRY							// r7 = X + rRY
	sub		r4,r3,r7							// r4 = r3 - rRX - X
	xor		r5,r3,rRY 		// AK 2003-07-31 (was rRX)	// r5 = r3 ^ rRX
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7							// clear Z only if it's zero now
	WRITE_W_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//================================================================================================

	//
	//		SUBX.L -(Ay),-(Ax)
	//
entry static subx_l_aym_axm
	CYCLES(30,30,10)
	READ_L_AREG(rTempSave,rRY)					// rTempSave = Ay
	subi	rTempSave,rTempSave,4				// Ay = Ay - 1
	SAVE_PC
	WRITE_L_AREG(rTempSave,rRY)					// put back the updated values
	SAVE_SR
	READ_L_AT(rTempSave)						// r3 = byte(Ay)
	READ_L_AREG(rEA,rRX)						// rEA = Ax
	subi	rEA,rEA,4							// Ax = Ax + 1
	WRITE_L_AREG(rEA,rRX)						// put back the updated values
	mr		rRX,r3								// rRX = (Ay)
	READ_L_AT(rEA)								// r3 = byte(Ax)
	LOAD_SR
	LOAD_PC
subx_l_aym_axm_core:
	GET_X(r7)									// r7 = X
	CLR_VC										// C = V = 0
	subc	r4,r3,rRX							// r4 = r3 - rRX
	LOAD_ICOUNT
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	subc	r4,r4,r7							// r4 = r3 - rRX - X
	xor		r5,r3,rRX							// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100) -- important: must set V afterwards
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRX) & ~(r3 ^ rRX)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	SAVE_SR
	WRITE_L_AT(rEA)								// byte(rEA) = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	// 	SUBX.L Dy,Dx
	//
entry static subx_l_dy_dx
	CYCLES(8,6,2)
	READ_L_REG(rRY,rRY)							// rRY = Dy
	READ_L_REG(r3,rRX)							// r3 = Dx
subx_l_dy_dx_core:
	GET_X(r7)									// r7 = X
	CLR_VC										// C = V = 0
	subc	r4,r3,rRY							// r4 = r3 - rRY
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	subc	r4,r4,r7							// r4 = r3 - rRX - X
	xor		r5,r3,rRY 		// AK 2003-07-31 (was rRX)	// r5 = r3 ^ rRX
	SET_C_LONG(r4)								// C = (r4 & 0x100) -- important: must set V afterwards
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	rlwinm	r7,r7,29,29,29						// shift Z into place
	xori	r7,r7,k68000FlagZ					// need to invert the Z sense before andc
	andc	rSRFlags,rSRFlags,r7				// clear Z only if it's zero now
	WRITE_L_REG(r4,rRX)							// save the result
	b		executeLoopEnd						// all done

	//================================================================================================
	//
	//		SUB -- Subtract two values
	//
	//================================================================================================

	//
	//		SUB.B Dy,Dx
	//
entry static sub_b_dy_dx
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = Dy
	READ_B_REG(rRY,rRX)							// rRY = Dx
	sub		r4,rRY,r3							// r4 = rRY - r3
	xor		r5,rRY,r3							// r5 = rRY ^ r3
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,rRY,r4							// r6 = rRY ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUB.B (Ay),Dx
	//
entry static sub_b_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
sub_b_ea_dx:
	READ_B_AT(rEA)								// r3 = byte(rEA)
sub_b_ea_dx_post:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	sub		r4,rRY,r3							// r4 = rRY - r3
	xor		r5,rRY,r3							// r5 = rRY ^ r3
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,rRY,r4							// r6 = rRY ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		SUB.B -(Ay),Dx
	//
entry static sub_b_aym_dx
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		sub_b_ea_dx
	
	//================================================================================================

	//
	//		SUB.B (Ay)+,Dx
	//
entry static sub_b_ayp_dx
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		sub_b_ea_dx
	
	//================================================================================================

	//
	//		SUB.B (Ay,d16),Dx
	//
entry static sub_b_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		sub_b_ea_dx
	
	//================================================================================================

	//
	//		SUB.B (Ay,Xn,d8),Dx
	//
entry static sub_b_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		sub_b_ea_dx

	//================================================================================================

	//
	//		SUB.B (xxx).W,Dx
	//		SUB.B (xxx).L,Dx
	//		SUB.B (d16,PC),Dx
	//		SUB.B (d8,PC,Xn),Dx
	//
entry static sub_b_other_dx
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		sub_b_ea_dx_post

	//================================================================================================

	//
	//		SUB.B Dx,(Ay)
	//
entry static sub_b_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
sub_b_dx_ea:
	READ_B_REG(rRY,rRX)							// rRY = Dx
	READ_B_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRY							// r4 = r3 - rRY
	xor		r5,r3,rRY							// r5 = r3 ^ rRY
	SET_C_BYTE(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_BYTE(r4)								// keep the result byte-sized
	SET_V_BYTE(r5)								// V = (r3 ^ r4) & (rRY ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUB.B Dx,-(Ay)
	//
entry static sub_b_dx_aym
	CYCLES(14,14,6)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		sub_b_dx_ea
	
	//================================================================================================

	//
	//		SUB.B Dx,(Ay)+
	//
entry static sub_b_dx_ayp
	CYCLES(12,12,7)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		sub_b_dx_ea
	
	//================================================================================================

	//
	//		SUB.B Dx,(Ay,d16)
	//
entry static sub_b_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		sub_b_dx_ea
	
	//================================================================================================

	//
	//		SUB.B Dx,(Ay,Xn,d8)
	//
entry static sub_b_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		sub_b_dx_ea

	//================================================================================================

	//
	//		SUB.B Dx,(xxx).W
	//		SUB.B Dx,(xxx).L
	//
entry static sub_b_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		sub_b_dx_absl						// handle long absolute mode
sub_b_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		sub_b_dx_ea
sub_b_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		sub_b_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		SUB.W Ry,Dx
	//
entry static sub_w_ry_dx
	CYCLES(4,4,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	READ_W_REG(r3,rRY)							// r3 = Ry
	READ_W_REG(rRY,rRX)							// rRY = Dx
	sub		r4,rRY,r3							// r4 = rRY - r3
	xor		r5,rRY,r3							// r5 = rRY ^ r3
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,rRY,r4							// r6 = rRY ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUB.W (Ay),Dx
	//
entry static sub_w_ay0_dx
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
sub_w_ea_dx:
	READ_W_AT(rEA)								// r3 = byte(rEA)
sub_w_ea_dx_post:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	sub		r4,rRY,r3							// r4 = rRY - r3
	xor		r5,rRY,r3							// r5 = rRY ^ r3
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,rRY,r4							// r6 = rRY ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_W_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		SUB.W -(Ay),Dx
	//
entry static sub_w_aym_dx
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		sub_w_ea_dx
	
	//================================================================================================

	//
	//		SUB.W (Ay)+,Dx
	//
entry static sub_w_ayp_dx
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		sub_w_ea_dx
	
	//================================================================================================

	//
	//		SUB.W (Ay,d16),Dx
	//
entry static sub_w_ayd_dx
	CYCLES(12,12,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		sub_w_ea_dx
	
	//================================================================================================

	//
	//		SUB.W (Ay,Xn,d8),Dx
	//
entry static sub_w_ayid_dx
	bl		computeEA110
	CYCLES(14,14,0)
	b		sub_w_ea_dx

	//================================================================================================

	//
	//		SUB.W (xxx).W,Dx
	//		SUB.W (xxx).L,Dx
	//		SUB.W (d16,PC),Dx
	//		SUB.W (d8,PC,Xn),Dx
	//
entry static sub_w_other_dx
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		sub_w_ea_dx_post

	//================================================================================================

	//
	//		SUB.W Dx,(Ay)
	//
entry static sub_w_dx_ay0
	CYCLES(12,12,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
sub_w_dx_ea:
	READ_W_REG(rRY,rRX)							// rRY = Dx
	READ_W_AT(rEA)								// r3 = byte(rEA)
	sub		r4,r3,rRY							// r4 = r3 - rRY
	xor		r5,r3,rRY							// r5 = r3 ^ rRY
	SET_C_WORD(r4)								// C = (r4 & 0x100)
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	TRUNC_WORD(r4)								// keep the result byte-sized
	SET_V_WORD(r5)								// V = (r3 ^ r4) & (rRY ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_W_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUB.W Dx,-(Ay)
	//
entry static sub_w_dx_aym
	CYCLES(14,14,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		sub_w_dx_ea
	
	//================================================================================================

	//
	//		SUB.W Dx,(Ay)+
	//
entry static sub_w_dx_ayp
	CYCLES(12,12,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		sub_w_dx_ea
	
	//================================================================================================

	//
	//		SUB.W Dx,(Ay,d16)
	//
entry static sub_w_dx_ayd
	CYCLES(16,16,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		sub_w_dx_ea
	
	//================================================================================================

	//
	//		SUB.W Dx,(Ay,Xn,d8)
	//
entry static sub_w_dx_ayid
	bl		computeEA110
	CYCLES(18,18,3)
	b		sub_w_dx_ea

	//================================================================================================

	//
	//		SUB.W Dx,(xxx).W
	//		SUB.W Dx,(xxx).L
	//
entry static sub_w_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		sub_w_dx_absl						// handle long absolute mode
sub_w_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(16,16,6)
	b		sub_w_dx_ea
sub_w_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(20,20,6)
	b		sub_w_dx_ea

	//================================================================================================
	//================================================================================================

	//
	//		SUB.L Ry,Dx
	//
entry static sub_l_ry_dx
	CYCLES(8,8,1)
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	READ_L_REG(r3,rRY)							// r3 = Dy
	READ_L_REG(rRY,rRX)							// rRY = Dx
	subc	r4,rRY,r3							// r4 = rRY - r3
	CLR_C										// C = 0
	xor		r5,rRY,r3							// r5 = rRY ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,rRY,r4							// r6 = rRY ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	SET_V_LONG(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		SUB.L (Ay),Dx
	//
entry static sub_l_ay0_dx
	CYCLES(14,14,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
sub_l_ea_dx:
	READ_L_AT(rEA)								// r3 = byte(rEA)
sub_l_ea_dx_post:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	subc	r4,rRY,r3							// r4 = rRY - r3
	CLR_C										// C = 0
	xor		r5,rRY,r3							// r5 = rRY ^ r3
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,rRY,r4							// r6 = rRY ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ r3) & ~(rRY ^ r3)
	SET_V_LONG(r5)								// V = (rRY ^ r4) & (r3 ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_L_REG(r4,rRX)							// Dx = r4
	b		executeLoopEndRestoreNoSR			// all done

	//================================================================================================

	//
	//		SUB.L -(Ay),Dx
	//
entry static sub_l_aym_dx
	CYCLES(16,16,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		sub_l_ea_dx
	
	//================================================================================================

	//
	//		SUB.L (Ay)+,Dx
	//
entry static sub_l_ayp_dx
	CYCLES(14,14,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		sub_l_ea_dx
	
	//================================================================================================

	//
	//		SUB.L (Ay,d16),Dx
	//
entry static sub_l_ayd_dx
	CYCLES(18,18,3)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		sub_l_ea_dx
	
	//================================================================================================

	//
	//		SUB.L (Ay,Xn,d8),Dx
	//
entry static sub_l_ayid_dx
	bl		computeEA110
	CYCLES(20,20,0)
	b		sub_l_ea_dx

	//================================================================================================

	//
	//		SUB.L (xxx).W,Dx
	//		SUB.L (xxx).L,Dx
	//		SUB.L (d16,PC),Dx
	//		SUB.L (d8,PC,Xn),Dx
	//
entry static sub_l_other_dx
	bl		fetchEALong111
	CYCLES(16,16,0)
	b		sub_l_ea_dx_post

	//================================================================================================

	//
	//		SUB.L Dx,(Ay)
	//
entry static sub_l_dx_ay0
	CYCLES(20,20,6)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
sub_l_dx_ea:
	READ_L_REG(rRY,rRX)							// rRY = Dx
	READ_L_AT(rEA)								// r3 = byte(rEA)
	subc	r4,r3,rRY							// r4 = r3 + rRY
	CLR_C										// C = 0
	xor		r5,r3,rRY							// r5 = r3 ^ rRY
	SET_C_LONG(r4)								// C = (r4 & 0x100)
	INVERT_C									// invert the carry sense
	xor		r6,r3,r4							// r6 = r3 ^ r4
	SET_X_FROM_C								// X = C
	and		r5,r6,r5							// r5 = (r4 ^ rRY) & ~(r3 ^ rRY)
	SET_V_LONG(r5)								// V = (r3 ^ r4) & (rRY ^ rRX)
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_L_AT(rEA)								// Dx = r4
	b		executeLoopEndRestore				// all done

	//================================================================================================

	//
	//		SUB.L Dx,-(Ay)
	//
entry static sub_l_dx_aym
	CYCLES(22,22,6)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		sub_l_dx_ea
	
	//================================================================================================

	//
	//		SUB.L Dx,(Ay)+
	//
entry static sub_l_dx_ayp
	CYCLES(20,20,7)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		sub_l_dx_ea
	
	//================================================================================================

	//
	//		SUB.L Dx,(Ay,d16)
	//
entry static sub_l_dx_ayd
	CYCLES(24,24,6)
	SAVE_SR
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		sub_l_dx_ea
	
	//================================================================================================

	//
	//		SUB.L Dx,(Ay,Xn,d8)
	//
entry static sub_l_dx_ayid
	bl		computeEA110
	CYCLES(26,26,3)
	b		sub_l_dx_ea

	//================================================================================================

	//
	//		SUB.L Dx,(xxx).W
	//		SUB.L Dx,(xxx).L
	//
entry static sub_l_dx_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
	bne		sub_l_dx_absl						// handle long absolute mode
sub_l_dx_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(24,24,6)
	b		sub_l_dx_ea
sub_l_dx_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(28,28,6)
	b		sub_l_dx_ea

	//================================================================================================
	//
	//		SWAP -- Swap register halves
	//
	//================================================================================================

entry static swap_dy
	CYCLES(4,4,1)
	READ_L_REG(r3,rRY)							// r3 = register
	cntlzw	r7,r3								// count zeros ahead of time
	CLR_VC										// V = C = 0
	rlwinm	r4,r3,16,0,31						// swap halves
	SET_Z(r7)									// Z = (r3 == 0)
	SET_N_LONG(r4)								// N = (r4 & 0x80)
	WRITE_L_REG(r4,rRY)							// save the register
	b		executeLoopEnd						// done

	//================================================================================================
	//
	//		TAS -- test and set
	//
	//================================================================================================

	//
	//		TAS.B Dy
	//
entry static tas_b_dy
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	TRUNC_BYTE_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	ori		r4,r4,0x80							// set the high bit
	SET_Z(r7)									// Z = (r4 == 0)
	WRITE_B_REG(r4,rRY)							// store the register
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		TAS.B (Ay)
	//
entry static tas_b_ay0
	CYCLES(18,18,14)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
tas_b_ea:
	READ_B_AT(rEA)								// r3 = (rEA)
	CLR_VC										// V = C = 0
	TRUNC_BYTE_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	ori		r4,r4,0x80							// set the high bit
	SET_Z(r7)									// Z = (r4 == 0)
	SAVE_SR
	WRITE_B_AT(rEA)								// store the register
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		TAS.B -(Ay)
	//
entry static tas_b_aym
	CYCLES(20,20,14)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		tas_b_ea
	
	//================================================================================================

	//
	//		TAS.B (Ay)+
	//
entry static tas_b_ayp
	CYCLES(18,18,14)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		tas_b_ea
	
	//================================================================================================

	//
	//		TAS.B (Ay,d16)
	//
entry static tas_b_ayd
	CYCLES(22,22,14)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		tas_b_ea
	
	//================================================================================================

	//
	//		TAS.B (Ay,Xn,d8)
	//
entry static tas_b_ayid
	bl		computeEA110
	CYCLES(24,24,12)
	b		tas_b_ea

	//================================================================================================

	//
	//		TAS.B (xxx).W
	//		TAS.B (xxx).L
	//
entry static tas_b_other
	cmpwi	cr2,rRY,1<<2
	andi.	r0,rRY,1<<2							// test the low bit
#if TARGET_RT_MAC_MACHO
	bgt		cr2,illegal1
#else
	bgt		cr2,illegal
#endif
	SAVE_SR
tas_b_abs:
	bne		tas_b_absl							// handle long absolute mode
tas_b_absw:
	READ_OPCODE_ARG_EXT(rEA)					// rEA = sign-extended word
	SAVE_PC
	CYCLES(22,22,14)
	b		tas_b_ea
tas_b_absl:
	READ_OPCODE_ARG_LONG(rEA)					// rEA = long
	SAVE_PC
	CYCLES(26,26,14)
	b		tas_b_ea

	//================================================================================================
	//
	//		TRAP -- Trap
	//
	//================================================================================================

entry static trap_imm
	rlwinm	r3,r11,0,28,31						// r3 = trap #
	addi	r3,r3,32							// r3 = trap + 32
	b		generateException

	//================================================================================================
	//
	//		TST.B -- test and set
	//
	//================================================================================================

	//
	//		TST.B Dy
	//
entry static tst_b_dy
	CYCLES(4,4,1)
	READ_B_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	TRUNC_BYTE_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

	//
	//		TST.B (Ay)
	//
entry static tst_b_ay0
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
tst_b_ea:
	READ_B_AT(rEA)								// r3 = (rEA)
tst_b_ea_post:
	CLR_VC										// V = C = 0
	TRUNC_BYTE_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_BYTE(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		TST.B -(Ay)
	//
entry static tst_b_aym
	CYCLES(10,10,3)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	subi	rEA,rEA,1							// rEA -= 1
	sub		rEA,rEA,r12							// rEA -= r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		tst_b_ea
	
	//================================================================================================

	//
	//		TST.B (Ay)+
	//
entry static tst_b_ayp
	CYCLES(8,8,4)
	addi	r12,rRY,1*4							// r12 = rRY + 4
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	rlwinm	r12,r12,27,31,31					// r12 = (rRY + 4) >> 5
	SAVE_SR
	addi	r3,rEA,1							// r3 = rEA + 1
	add		r3,r3,r12							// r3 += r12 (to keep A7 word aligned!)
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		tst_b_ea
	
	//================================================================================================

	//
	//		TST.B (Ay,d16)
	//
entry static tst_b_ayd
	CYCLES(12,12,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		tst_b_ea
	
	//================================================================================================

	//
	//		TST.B (Ay,Xn,d8)
	//
entry static tst_b_ayid
	bl		computeEA110
	CYCLES(14,14,0)
	b		tst_b_ea

	//================================================================================================

	//
	//		TST.B (xxx).W
	//		TST.B (xxx).L
	//
entry static tst_b_other
	bl		fetchEAByte111
	CYCLES(8,8,0)
	b		tst_b_ea_post

	//================================================================================================
	//================================================================================================

	//
	//		TST.W Dy
	//
entry static tst_w_dy
	CYCLES(4,4,1)
	READ_W_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	TRUNC_WORD_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		TST.W Ay
	//
entry static tst_w_ay
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	CYCLES(0,0,1)
	READ_W_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	TRUNC_WORD_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return
#endif

	//================================================================================================

	//
	//		TST.W (Ay)
	//
entry static tst_w_ay0
	CYCLES(8,8,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
tst_w_ea:
	READ_W_AT(rEA)								// r3 = (rEA)
tst_w_ea_post:
	CLR_VC										// V = C = 0
	TRUNC_WORD_TO(r4,r3)						// keep the result byte-sized
	cntlzw	r7,r4								// r7 = number of zeros in r4
	SET_N_WORD(r4)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		TST.W -(Ay)
	//
entry static tst_w_aym
	CYCLES(10,10,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,2							// rEA -= 2
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		tst_w_ea
	
	//================================================================================================

	//
	//		TST.W (Ay)+
	//
entry static tst_w_ayp
	CYCLES(8,8,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,2							// r3 = rEA + 2
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		tst_w_ea
	
	//================================================================================================

	//
	//		TST.W (Ay,d16)
	//
entry static tst_w_ayd
	CYCLES(12,12,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		tst_w_ea
	
	//================================================================================================

	//
	//		TST.W (Ay,Xn,d8)
	//
entry static tst_w_ayid
	bl		computeEA110
	CYCLES(14,14,0)
	b		tst_w_ea

	//================================================================================================

	//
	//		TST.W (xxx).W
	//		TST.W (xxx).L
	//
entry static tst_w_other
	bl		fetchEAWord111
	CYCLES(8,8,0)
	b		tst_w_ea_post

	//================================================================================================
	//================================================================================================

	//
	//		TST.L Dy
	//
entry static tst_l_dy
	CYCLES(4,4,1)
	READ_L_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return

	//================================================================================================

#if (A68000_CHIP >= 68020)
	//
	//		TST.L Ay
	//
entry static tst_l_ay
	REG_OFFSET_LO_4(rRY)						// rRY = 4-bit register offset
	CYCLES(0,0,1)
	READ_L_REG(r3,rRY)							// r3 = rRY
	CLR_VC										// V = C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEnd						// return
#endif

	//================================================================================================

	//
	//		TST.L (Ay)
	//
entry static tst_l_ay0
	CYCLES(12,12,3)
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
tst_l_ea:
	READ_L_AT(rEA)								// r3 = (rEA)
tst_l_ea_post:
	CLR_VC										// V = C = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_N_LONG(r3)								// N = (r4 & 0x80)
	SET_Z(r7)									// Z = (r4 == 0)
	b		executeLoopEndRestoreNoSR			// return

	//================================================================================================

	//
	//		TST.L -(Ay)
	//
entry static tst_l_aym
	CYCLES(14,14,3)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	subi	rEA,rEA,4							// rEA -= 4
	SAVE_PC
	WRITE_L_AREG(rEA,rRY)						// Ay = rEA
	b		tst_l_ea
	
	//================================================================================================

	//
	//		TST.L (Ay)+
	//
entry static tst_l_ayp
	CYCLES(12,12,4)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	addi	r3,rEA,4							// r3 = rEA + 4
	SAVE_PC
	WRITE_L_AREG(r3,rRY)						// Ay = r3
	b		tst_l_ea
	
	//================================================================================================

	//
	//		TST.L (Ay,d16)
	//
entry static tst_l_ayd
	CYCLES(16,16,3)
	READ_OPCODE_ARG_EXT(r3)						// r3 = sign-extended opcode
	SAVE_SR
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_PC
	add		rEA,rEA,r3							// rEA += r3
	b		tst_l_ea
	
	//================================================================================================

	//
	//		TST.L (Ay,Xn,d8)
	//
entry static tst_l_ayid
	bl		computeEA110
	CYCLES(18,18,0)
	b		tst_l_ea

	//================================================================================================

	//
	//		TST.L (xxx).W
	//		TST.L (xxx).L
	//
entry static tst_l_other
	bl		fetchEALong111
	CYCLES(12,12,0)
	b		tst_l_ea_post

	//================================================================================================
	//
	//		UNLK -- Decallocate stack space
	//
	//================================================================================================

entry static unlk_ay
	CYCLES(12,12,5)
	READ_L_AREG(rEA,rRY)						// rEA = Ay
	SAVE_SR
	SAVE_PC
	READ_L_AT(rEA)								// r3 = (rEA)
	addi	rEA,rEA,4							// rEA += 4
	WRITE_L_AREG(r3,rRY)						// Ay = (rEA)
	SET_A7(rEA)									// A7 = rEA
	b		executeLoopEndRestore				// return
}
