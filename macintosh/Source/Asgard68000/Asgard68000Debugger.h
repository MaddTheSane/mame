//###################################################################################################
//
//
//		Asgard68000Debugger.h
//		Internal debugging system for tracking down incompatibilities with another 68000 core.
//
//		See Asgard68000Core.c for information about the Asgard68000 system.
//
//
//###################################################################################################

#ifndef __ASGARD68000DEBUGGER__
#define __ASGARD68000DEBUGGER__


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

void			Asgard68000MiniTrace(unsigned int *inD, unsigned int *inA, unsigned long inPC, unsigned long inSR, int inICount);
int 			Asgard68000DebugReadByte(int inAddress);
int 			Asgard68000DebugReadWord(int inAddress);
int 			Asgard68000DebugReadLong(int inAddress);
void 			Asgard68000DebugWriteByte(int inAddress, int inValue);
void 			Asgard68000DebugWriteWord(int inAddress, int inValue);
void 			Asgard68000DebugWriteLong(int inAddress, int inValue);

#endif
