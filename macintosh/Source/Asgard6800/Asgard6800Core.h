//###################################################################################################
//
//
//		Asgard6800Core.h
//		See Asgard6800DefaultConfig.h for compile-time configuration and optimization.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#ifndef __ASGARD6800CORE__
#define __ASGARD6800CORE__


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include "Asgard6800DefaultConfig.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	k6800FlagH					= 0x20,		// Half (auxiliary) carry
	k6800FlagI					= 0x10,		// Inhibit IRQ
	k6800FlagN					= 0x08,		// Negative
	k6800FlagZ					= 0x04,		// Zero
	k6800FlagV					= 0x02,		// Overflow
	k6800FlagC					= 0x01,		// Carry
	
	k6800TCSRStateICF			= 0x80,		// Timer Control register bits
	k6800TCSRStateOCF			= 0x40,
	k6800TCSRStateTOF			= 0x20,
	k6800TCSRStateEICI			= 0x10,
	k6800TCSRStateEOCI			= 0x08,
	k6800TCSRStateETOI			= 0x04,
	k6800TCSRStateIEDG			= 0x02,
	k6800TCSRStateOLVL			= 0x01,

	k6800IRQStateClear			= 0,		// clear state for the IRQ lines
	k6800IRQStateAsserted		= 1,		// asserted state for the IRQ lines
	
	k6800PortDDR1				= 0,		// internal ports that are used by some 6800 variants
	k6800PortDDR2,
	k6800PortData1				= 256,
	k6800PortData2,

	k6800IRQLineIRQ				= 0,		// IRQ line for the IRQ signal
	k6800IRQLineTIN,						// P20/TIN input capture line (edge sensitive)
	k6800IRQLineCount,
	
	k6800RegisterIndexPC		= 1,		// index for GetReg()
	k6800RegisterIndexS,
	k6800RegisterIndexA,
	k6800RegisterIndexB,
	k6800RegisterIndexX,
	k6800RegisterIndexCC,
	k6800RegisterIndexWAIState,
	k6800RegisterIndexNMIState,
	k6800RegisterIndexIRQState,
	k6800RegisterIndexOCIState,
	k6800RegisterIndexTINState,
	
	k6800RegisterIndexOpcodePC	= -1		// special index for the opcode PC
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef int (*Asgard6800IRQCallback)(int inIRQLine);


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void 					Asgard6800Reset(void);

void 					Asgard6800SetContext(void *inContext);
void					Asgard6800SetReg(int inRegisterIndex, unsigned int inValue);

void		 			Asgard6800GetContext(void *outContext);
unsigned int			Asgard6800GetReg(int inRegisterIndex);

void 					Asgard6800SetNMILine(int inState);
void 					Asgard6800SetIRQLine(int inIRQLine, int inState);
void 					Asgard6800SetIRQCallback(Asgard6800IRQCallback inCallback);
Asgard6800IRQCallback	Asgard6800GetIRQCallback(void);

int 					Asgard6800Execute(register int inCycles);

UINT8					Asgard6800InternalRead(offs_t inOffset);
void					Asgard6800InternalWrite(offs_t inOffset, UINT8 inData);

#endif
