//###################################################################################################
//
//
//		AsgardZ80Debugger.h
//		Internal debugging system for tracking down incompatibilities with another Z80 core.
//
//		See AsgardZ80Core.c for information about the AsgardZ80 system.
//
//
//###################################################################################################

#ifndef __ASGARDZ80DEBUGGER__
#define __ASGARDZ80DEBUGGER__


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void			AsgardZ80MiniTrace(unsigned long inPCAF, unsigned long inSPDE, unsigned long inHLBC, unsigned long inIXIY, unsigned long inAFDE2, unsigned long inHLBC2, unsigned long inFlags, int inICount);
int 			AsgardZ80DebugRead(int inAddress);
void 			AsgardZ80DebugWrite(int inAddress, int inValue);

#endif
