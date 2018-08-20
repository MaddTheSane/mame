//###################################################################################################
//
//
//		Asgard6502Core.h
//		See Asgard6502Core.c for information about the Asgard6502 system.
//
//
//###################################################################################################

#ifndef __ASGARD6502CORE__
#define __ASGARD6502CORE__


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include "Asgard6502DefaultConfig.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	k6502FlagN					= 0x80, 	// Negative
	k6502FlagV					= 0x40,		// Overflow
	k6502FlagT					= 0x20,		// ???
	k6502FlagB					= 0x10,		// ???
	k6502FlagD					= 0x08,		// Decimal
	k6502FlagI					= 0x04,		// Interrupt
	k6502FlagZ					= 0x02,		// Zero
	k6502FlagC					= 0x01,		// Carry

	k6502IRQStateClear			= 0,		// clear state for the IRQ lines
	k6502IRQStateAsserted		= 1,		// asserted state for the IRQ lines
	
	k6502IRQLine				= 0,		// IRQ line for the IRQ signal
	k6502IRQLineCount,
	
	k6502RegisterIndexPC		= 1,		// index for GetReg()
	k6502RegisterIndexS,
	k6502RegisterIndexP,
	k6502RegisterIndexA,
	k6502RegisterIndexX,
	k6502RegisterIndexY,
	k6502RegisterIndexEA,
	k6502RegisterIndexZP,
	k6502RegisterIndexNMIState,
	k6502RegisterIndexIRQState,
	k6502RegisterIndexSOState,
	k6502RegisterIndexSubType,
	
	k6502RegisterIndexOpcodePC = -1			// special index
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef int 	(*Asgard6502IRQCallback)(int inIRQLine);


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void 					Asgard6502Reset(void);

void 					Asgard6502SetContext(void *inContext);
void					Asgard6502SetReg(int inRegisterIndex, unsigned int inValue);

void		 			Asgard6502GetContext(void *outContext);
unsigned int			Asgard6502GetReg(int inRegisterIndex);

void 					Asgard6502SetNMILine(int inState);
void 					Asgard6502SetIRQLine(int inIRQLine, int inState);
void 					Asgard6502SetIRQCallback(Asgard6502IRQCallback inCallback);
Asgard6502IRQCallback	Asgard6502GetIRQCallback(void);

int 					Asgard6502Execute(register int inCycles);

#endif
