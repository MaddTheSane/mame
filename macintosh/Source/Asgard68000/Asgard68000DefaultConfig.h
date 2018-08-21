//###################################################################################################
//
//
//		Asgard68000DefaultConfig.h
//		This file contains default compile-time configuration and optimization flags.
//
//		See Asgard68000Core.c for information about the Asgard68000 system.
//
//
//###################################################################################################
//
//		WARNING: Do not modify this file! Look at Asgard68000MAME.c for an example of
//		how to configure your core.
//
//###################################################################################################

#ifndef __ASGARD68000DEFAULTCONFIG__
#define __ASGARD68000DEFAULTCONFIG__


//###################################################################################################
//
// 	Set A68000_CHIP to the number of the chip you wish to emulate. Currently supported
//	variants include:
//
//		Motorola MC68000    == 68000
//		Motorola MC68010    == 68010
//		Motorola MC68020    == 68020
//
//	Set A68000_CHIPTIMING to the timing model you wish to use. Leave it undefined to default
//	to the same timing as the time you're emulating (seems reasonable enough, doesn't it?)
//
//	Set A68000_ADDRESSBITS to the number of address lines on the chip. The main use for this
//	is to create an MC68EC020 variant of the 68000 by specifying only 32 bits.
//
//###################################################################################################

#ifndef A68000_CHIP
#define A68000_CHIP				68000
#endif

#ifndef A68000_CHIPTIMING
#define A68000_CHIPTIMING		A68000_CHIP
#endif

#ifndef A68000_ADDRESSBITS
#if (A68000_CHIP >= 68020)
#define A68000_ADDRESSBITS	32
#else
#define A68000_ADDRESSBITS	24
#endif
#endif


//###################################################################################################
//
// 	Set A68000_COREDEBUG to 1 to enable a very simple internal debugging system for debugging
//	the CPU core itself. You will need to make sure to link in Asgard68000Debugger.c as well.
//
//###################################################################################################

#ifndef A68000_COREDEBUG
#define A68000_COREDEBUG		0
#endif


//###################################################################################################
//
// 	Define A68000_DEBUGHOOK to the name of a function to be called once per instruction for
//	debugging. Its prototype should look like this:
//
//		void DebuggerHook(void)
//
//	Leave A68000_DEBUGHOOK undefined to get rid of the performance penalty of calling a
//	function once per emulated instruction.
//
//###################################################################################################

//#define A68000_DEBUGHOOK		DebuggerHook


//###################################################################################################
//
// 	Set A68000_UPDATEBANK to the name of a function that will check the PC after every 
//	significant branch and perform bankswitching by changing A68000_OPCODEROM and/or
//	A68000_ARGUMENTROM during execution. Its prototype should look like this:
//
//		void UpdateBank(int newpc)
//
//	Leave A68000_UPDATEBANK undefined if you don't need to do any bankswitching within
//	your emulator.
//
//###################################################################################################

//#define A68000_UPDATEBANK		UpdateBank


//###################################################################################################
//
// 	Define A68000_READxxx and A68000_WRITExxx to the name of your memory read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadByte(int addr)
//		int ReadWord(int addr)
//		int ReadLong(int addr)
//		void WriteByte(int addr, int val)
//		void WriteWord(int addr, int val)
//		void WriteLong(int addr, int val)
//
//###################################################################################################

#ifndef A68000_READBYTE
#define A68000_READBYTE			ReadByte
#endif

#ifndef A68000_READWORD
#define A68000_READWORD			ReadWord
#endif

#ifndef A68000_READLONG
#define A68000_READLONG			ReadLong
#endif

#ifndef A68000_WRITEBYTE
#define A68000_WRITEBYTE		WriteByte
#endif

#ifndef A68000_WRITEWORD
#define A68000_WRITEWORD		WriteWord
#endif

#ifndef A68000_WRITELONG
#define A68000_WRITELONG		WriteLong
#endif


//###################################################################################################
//
// 	Define A68000_OPCODEROM to point to the base of ROM.
//
//	This is assumed to be extern unsigned char *'s.
//
//###################################################################################################

#ifndef A68000_OPCODEROM
#define A68000_OPCODEROM		OpcodeROM
#endif


//###################################################################################################
//
// 	Set A68000_ICOUNT to the name of a global variable that should maintain the current
//	cycle downcounter for this CPU. This field is required.
//
//	This variable is assumed to be extern int.
//
//###################################################################################################

#ifndef A68000_ICOUNT
#define A68000_ICOUNT			gICount
#endif


//###################################################################################################
//
// 	Set A68000_OPCODEPC to the name of a global variable that should receive the PC of the
//	start of the current opcode on every instruction.
//
//	This variable is assumed to be extern int.
//
//	Leave A68000_OPCODEPC undefined if you don't need to know this value, as it adds an extra
//	two instructions per opcode.
//
//###################################################################################################

//#define A68000_OPCODEPC		gOpcodePC


//###################################################################################################
//
// 	Set A68000_VOLATILEREGS to 1 if your emulator needs to examine or change the values of 
//	the PC, A, B, X, Y, S, or U registers from within an Asgard68000Execute call.
//
//###################################################################################################

#ifndef A68000_VOLATILEREGS
#define A68000_VOLATILEREGS		1
#endif


#endif
