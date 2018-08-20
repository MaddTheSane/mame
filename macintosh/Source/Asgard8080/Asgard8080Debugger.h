//###################################################################################################
//
//
//		Asgard8080Debugger.h
//		Internal debugging system for tracking down incompatibilities with another 8080 core.
//
//		See Asgard8080Core.c for information about the Asgard8080 system.
//
//
//###################################################################################################

#ifndef __ASGARD8080DEBUGGER__
#define __ASGARD8080DEBUGGER__


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void			Asgard8080MiniTrace(unsigned long inPCAF, unsigned long inSPDE, unsigned long inHLBC, unsigned long inFlags, int inICount);
int 			Asgard8080DebugRead(int inAddress);
void 			Asgard8080DebugWrite(int inAddress, int inValue);

#endif
