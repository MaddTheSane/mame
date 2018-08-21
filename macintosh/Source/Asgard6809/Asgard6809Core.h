//###################################################################################################
//
//
//		Asgard6809Core.h
//		See Asgard6809Core.c for information about the Asgard6809 system.
//
//
//###################################################################################################

#ifndef __ASGARD6809CORE__
#define __ASGARD6809CORE__


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include "Asgard6809DefaultConfig.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	k6809FlagE					= 0x80, 	// entire state pushed
	k6809FlagF					= 0x40,		// Inhibit FIRQ
	k6809FlagH					= 0x20,		// Half (auxiliary) carry
	k6809FlagI					= 0x10,		// Inhibit IRQ
	k6809FlagN					= 0x08,		// Negative
	k6809FlagZ					= 0x04,		// Zero
	k6809FlagV					= 0x02,		// Overflow
	k6809FlagC					= 0x01,		// Carry

	k6809IRQStateClear			= 0,		// clear state for the IRQ lines
	k6809IRQStateAsserted		= 1,		// asserted state for the IRQ lines
	
	k6809IRQLineIRQ				= 0,		// IRQ line for the IRQ signal
	k6809IRQLineFIRQ,						// IRQ line for the FIRQ signal
	k6809IRQLineCount,
	k6809IRQLineNMI				= 127,		// IRQ line for the NMI signal
	
	k6809RegisterIndexPC		= 1,		// index for GetReg()
	k6809RegisterIndexS,
	k6809RegisterIndexCC,
	k6809RegisterIndexA,
	k6809RegisterIndexB,
	k6809RegisterIndexU,
	k6809RegisterIndexX,
	k6809RegisterIndexY,
	k6809RegisterIndexDP,
	k6809RegisterIndexNMIState,
	k6809RegisterIndexIRQState,
	k6809RegisterIndexFIRQState,
	
	k6809RegisterIndexOpcodePC	= -1		// special index for the opcode PC
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef int (*Asgard6809IRQCallback)(int inIRQLine);


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void 					Asgard6809Reset(void);

void					Asgard6809SetReg(int inRegisterIndex, unsigned int inValue);
void 					Asgard6809SetContext(void *inContext);

unsigned int			Asgard6809GetReg(int inRegisterIndex);
void					Asgard6809GetContext(void *outContext);

void 					Asgard6809SetNMILine(int inState);
void 					Asgard6809SetIRQLine(int inIRQLine, int inState);
void 					Asgard6809SetIRQCallback(Asgard6809IRQCallback inCallback);
Asgard6809IRQCallback	Asgard6809GetIRQCallback(void);

int 					Asgard6809Execute(register int inCycles);

#endif
