//###################################################################################################
//
//
//		Asgard6809Debugger.h
//		Internal debugging system for tracking down incompatibilities with another 6809 core.
//
//		See Asgard6809Core.c for information about the Asgard6809 system.
//
//
//###################################################################################################

#ifndef __ASGARD6809DEBUGGER__
#define __ASGARD6809DEBUGGER__


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void			Asgard6809MiniTrace(unsigned long inXY, unsigned long inSU, unsigned long inPCAB, unsigned long inDPFlags, int inICount);
int 			Asgard6809DebugRead(int inAddress);
void 			Asgard6809DebugWrite(int inAddress, int inValue);

#endif
