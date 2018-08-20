//###################################################################################################
//
//
//		Asgard68000Core.h
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
//		It is quite possible that I have a newer version.  My email address is agiles@sirius.com
//
//		This file looks best when viewed with a tab size of 4 characters
//
//		Warning: due to a bug in CWPro 2, you will need CWPro 3 or later to compile this
//
//
//###################################################################################################

#ifndef __ASGARD68000CORE__
#define __ASGARD68000CORE__


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include "Asgard68000DefaultConfig.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	k68000FlagT					= 0x8000,	// trace flag
	k68000FlagS					= 0x2000,	// supervisor mode
	k68000FlagIMask				= 0x0700,	// interrupt mask
	k68000FlagIShift			= 8,		// shift to get to the interrupt mask
	k68000FlagX					= 0x0010,	// extended carry flag
	k68000FlagN					= 0x0008,	// negative flag
	k68000FlagZ					= 0x0004,	// zero flag
	k68000FlagV					= 0x0002,	// overflow flag
	k68000FlagC					= 0x0001,	// carry flag

	k68000IRQStateClear			= 0,		// clear state for the IRQ lines
	k68000IRQStateAsserted		= 1,		// asserted state for the IRQ lines
	
	k68000IRQLineIRQ0			= 0,		// IRQ line for the IRQ signal
	k68000IRQLineIRQ1,
	k68000IRQLineIRQ2,
	k68000IRQLineIRQ3,
	k68000IRQLineIRQ4,
	k68000IRQLineIRQ5,
	k68000IRQLineIRQ6,
	k68000IRQLineCount,
	
	k68000RegisterIndexPC		= 1,		// index for GetReg()
	k68000RegisterIndexSP,
	k68000RegisterIndexISP,
	k68000RegisterIndexUSP,
	k68000RegisterIndexMSP,
	k68000RegisterIndexSR,
	k68000RegisterIndexVBR,
	k68000RegisterIndexSFC,
	k68000RegisterIndexDFC,
	k68000RegisterIndexCACR,
	k68000RegisterIndexCAAR,
	k68000RegisterIndexAddrPref,
	k68000RegisterIndexDataPref,
	k68000RegisterIndexD0,
	k68000RegisterIndexD1,
	k68000RegisterIndexD2,
	k68000RegisterIndexD3,
	k68000RegisterIndexD4,
	k68000RegisterIndexD5,
	k68000RegisterIndexD6,
	k68000RegisterIndexD7,
	k68000RegisterIndexA0,
	k68000RegisterIndexA1,
	k68000RegisterIndexA2,
	k68000RegisterIndexA3,
	k68000RegisterIndexA4,
	k68000RegisterIndexA5,
	k68000RegisterIndexA6,
	k68000RegisterIndexA7,
	
	k68000RegisterIndexOpcodePC = -1		// special index for the OpcodePC
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef int (*Asgard68000IRQCallback)(int inIRQLine);
typedef void (*Asgard68000ResetCallback)(void);
typedef void (*Asgard68000RTECallback)(void);
typedef void (*Asgard68000CMPILDCallback)(unsigned int, int);

//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void 			Asgard68000Reset(void);

void			Asgard68000SetReg(int inRegisterIndex, unsigned int inValue);
void 			Asgard68000SetContext(void *inContext);

unsigned int	Asgard68000GetReg(int inRegisterIndex);
unsigned int	Asgard68000GetContext(void *outContext);

void 			Asgard68000SetIRQLine(int inIRQLine, int inState);
void 			Asgard68000SetIRQCallback(Asgard68000IRQCallback inCallback);

void 			Asgard68000SetResetCallback(Asgard68000ResetCallback inCallback);
void 			Asgard68000SetRTECallback(Asgard68000RTECallback inCallback);
void 			Asgard68000SetCMPILDCallback(Asgard68000CMPILDCallback inCallback);

int				Asgard68000Execute(register int inCycles);

#endif
