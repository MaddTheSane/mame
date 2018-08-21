//###################################################################################################
//
//
//		Asgard8080Core.h
//		See Asgard8080Core.c for information about the Asgard8080 system.
//
//
//###################################################################################################

#ifndef __ASGARD8080CORE__
#define __ASGARD8080CORE__


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include "Asgard8080DefaultConfig.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	k8080FlagS					= 0x80, 	// Sign
	k8080FlagZ					= 0x40,		// Zero
	k8080FlagY					= 0x20,		// ???
	k8080FlagH					= 0x10,		// Half Carry
	k8080FlagX					= 0x08,		// Extended
	k8080FlagP					= 0x04,		// Parity
	k8080FlagV					= 0x04,		// Overflow
	k8080FlagN					= 0x02,		// Negative
	k8080FlagC					= 0x01,		// Carry

	k8080IRQStateClear			= 0,		// clear state for the IRQ lines
	k8080IRQStateAsserted		= 1,		// asserted state for the IRQ lines
	
	k8080IRQLineINTR			= 0,		// IRQ line for the IRQ signal
	k8080IRQLineRST55,						// IRQ line for the RST55 signal
	k8080IRQLineRST65,						// IRQ line for the RST65 signal
	k8080IRQLineRST75,						// IRQ line for the RST75 signal
	k8080IRQLineCount,
	k8080IRQLineNMI				= 127,		// IRQ line for the NMI signal
	
	k8080RegisterIndexPC		= 1,		// index for GetReg()
	k8080RegisterIndexSP,
	k8080RegisterIndexAF,
	k8080RegisterIndexBC,
	k8080RegisterIndexDE,
	k8080RegisterIndexHL,
	k8080RegisterIndexHALT,
	k8080RegisterIndexIM,
	k8080RegisterIndexIREQ,
	k8080RegisterIndexISRV,
	k8080RegisterIndexVector,
	k8080RegisterIndexTRAPState,
	k8080RegisterIndexINTRState,
	k8080RegisterIndexRST55State,
	k8080RegisterIndexRST65State,
	k8080RegisterIndexRST75State,
	
	k8080RegisterIndexOpcodePC = -1			// special index
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef int 	(*Asgard8080IRQCallback)(int inIRQLine);
typedef void 	(*Asgard8080SODCallback)(int inIRQLine);


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void 					Asgard8080Reset(void);

void 					Asgard8080SetContext(void *inContext);
void					Asgard8080SetReg(int inRegisterIndex, unsigned int inValue);

unsigned int 			Asgard8080GetContext(void *outContext);
unsigned int			Asgard8080GetReg(int inRegisterIndex);

void 					Asgard8080SetNMILine(int inState);
void 					Asgard8080SetIRQLine(int inIRQLine, int inState);
void 					Asgard8080SetIRQCallback(Asgard8080IRQCallback inCallback);
Asgard8080IRQCallback 	Asgard8080GetIRQCallback(void);

void					Asgard8080SetSIDLine(int inState);
void					Asgard8080SetSODCallback(Asgard8080SODCallback inCallback);

int 					Asgard8080Execute(register int inCycles);

#endif
