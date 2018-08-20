//###################################################################################################
//
//
//		Asgard6800Debugger.h
//		Internal debugging system for tracking down incompatibilities with another 6800 core.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#ifndef __ASGARD6800DEBUGGER__
#define __ASGARD6800DEBUGGER__


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void			Asgard6800MiniTrace(unsigned long inSX, unsigned long inPCAB, unsigned long inFlags, int inICount, int inCounter, int inOutputCompare);
int 			Asgard6800DebugRead(int inAddress);
void 			Asgard6800DebugWrite(int inAddress, int inValue);

#endif
