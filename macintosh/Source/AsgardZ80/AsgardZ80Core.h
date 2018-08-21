//###################################################################################################
//
//
//		AsgardZ80Core.h
//		See AsgardZ80Core.c for information about the AsgardZ80 system.
//
//
//###################################################################################################

#ifndef __ASGARDZ80CORE__
#define __ASGARDZ80CORE__


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include "AsgardZ80DefaultConfig.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	kZ80MaxDaisy				= 4,		// maximum daisy chain devices
	
	kZ80FlagS					= 0x80, 	// Sign
	kZ80FlagZ					= 0x40,		// Zero
	kZ80FlagY					= 0x20,		// ???
	kZ80FlagH					= 0x10,		// Half Carry
	kZ80FlagX					= 0x08,		// Extended
	kZ80FlagP					= 0x04,		// Parity
	kZ80FlagV					= 0x04,		// Overflow
	kZ80FlagN					= 0x02,		// Negative
	kZ80FlagC					= 0x01,		// Carry

	kZ80IRQStateClear			= 0,		// clear state for the IRQ lines
	kZ80IRQStateAsserted		= 1,		// asserted state for the IRQ lines
	
	kZ80DaisyIRQStateREQ		= 0x01,
	kZ80DaisyIRQStateIEO		= 0x02,
	
	kZ80IRQLineIRQ				= 0,		// IRQ line for the IRQ signal
	kZ80IRQLineCount,
	
	kZ80RegisterIndexPC			= 1,		// index for GetReg()
	kZ80RegisterIndexSP,
	kZ80RegisterIndexAF,
	kZ80RegisterIndexBC,
	kZ80RegisterIndexDE,
	kZ80RegisterIndexHL,
	kZ80RegisterIndexIX,
	kZ80RegisterIndexIY,
	kZ80RegisterIndexAF2,
	kZ80RegisterIndexBC2,
	kZ80RegisterIndexDE2,
	kZ80RegisterIndexHL2,
	kZ80RegisterIndexR,
	kZ80RegisterIndexR2,
	kZ80RegisterIndexI,
	kZ80RegisterIndexIM,
	kZ80RegisterIndexIFF1,
	kZ80RegisterIndexIFF2,
	kZ80RegisterIndexHALT,
	kZ80RegisterIndexNMIState,
	kZ80RegisterIndexIRQState,
	kZ80RegisterIndexDaisyIntState0,
	kZ80RegisterIndexDaisyIntState1,
	kZ80RegisterIndexDaisyIntState2,
	kZ80RegisterIndexDaisyIntState3,
	kZ80RegisterIndexDaisyIntCount,
	kZ80RegisterIndexNMINestLevel,
	
	kZ80RegisterIndexOpcodePC	= -1		// special index for the opcode PC
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef int 	(*AsgardZ80IRQCallback)(int inIRQLine);

typedef void 	(*AsgardZ80DaisyReset)(int inParameter);
typedef int 	(*AsgardZ80DaisyEntry)(int inParameter);
typedef void 	(*AsgardZ80DaisyRETI)(int inParameter);


typedef struct
{
	AsgardZ80DaisyReset		fReset;
	AsgardZ80DaisyEntry		fEntry;
	AsgardZ80DaisyRETI		fRETI;
	int						fParameter;
} AsgardZ80DaisyDevice;


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void 					AsgardZ80Reset(AsgardZ80DaisyDevice *inDaisyList);

void 					AsgardZ80SetContext(void *inContext);
void					AsgardZ80SetReg(int inRegisterIndex, unsigned int inValue);

unsigned int 			AsgardZ80GetContext(void *outContext);
unsigned int			AsgardZ80GetReg(int inRegisterIndex);

void 					AsgardZ80SetNMILine(int inState);
void 					AsgardZ80SetIRQLine(int inIRQLine, int inState);
void 					AsgardZ80SetIRQCallback(AsgardZ80IRQCallback inCallback);
AsgardZ80IRQCallback	AsgardZ80GetIRQCallback(void);

int 					AsgardZ80Execute(register int inCycles);

#endif
