//###################################################################################################
//
//
//		Asgard6502Debugger.h
//		Internal debugging system for tracking down incompatibilities with another 6502 core.
//
//		See Asgard6502Core.c for information about the Asgard6502 system.
//
//
//###################################################################################################

#ifndef __ASGARD6502DEBUGGER__
#define __ASGARD6502DEBUGGER__


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void			Asgard6502MiniTrace(unsigned long inPCP, unsigned long inSAXY, int inICount);
int 			Asgard6502DebugRead(int inAddress);
void 			Asgard6502DebugWrite(int inAddress, int inValue);

#endif
