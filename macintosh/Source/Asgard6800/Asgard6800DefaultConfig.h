//###################################################################################################
//
//
//		Asgard6800DefaultConfig.h
//		This file contains default compile-time configuration and optimization flags.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################
//
//		WARNING: Do not modify this file! Look at Asgard6800MAME.c for an example of
//		how to configure your core.
//
//###################################################################################################

#ifndef __ASGARD6800DEFAULTCONFIG__
#define __ASGARD6800DEFAULTCONFIG__


//###################################################################################################
//
// 	Set A6800_CHIP to the number of the chip you wish to emulate. Currently supported
//	variants include:
//
//		Motorola M6800  == 6800
//		Motorola M6801  == 6801
//		Motorola M6802  == 6802
//		Motorola M6803  == 6803
//		Motorola M6808  == 6808
//		NSC8105         == 8105
//		Hitachi HD63701 == 63701
//
//	Set A6800_CHIPTIMING to the timing model you wish to use. Leave it undefined to default
//	to the same timing as the time you're emulating (seems reasonable enough, doesn't it?)
//
//###################################################################################################

#ifndef A6800_CHIP
#define A6800_CHIP				6800
#endif

#ifndef A6800_CHIPTIMING
#define A6800_CHIPTIMING		A6800_CHIP
#endif


//###################################################################################################
//
// 	Set A6800_COREDEBUG to 1 to enable a very simple internal debugging system for debugging
//	the CPU core itself. You will need to make sure to link in Asgard6800Debugger.c as well.
//
//###################################################################################################

#ifndef A6800_COREDEBUG
#define A6800_COREDEBUG			0
#endif


//###################################################################################################
//
// 	Define A6800_DEBUGHOOK to the name of a function to be called once per instruction for
//	debugging. Its prototype should look like this:
//
//		void DebuggerHook(void)
//
//	Leave A6800_DEBUGHOOK undefined to get rid of the performance penalty of calling a
//	function once per emulated instruction.
//
//###################################################################################################

//#define A6800_DEBUGHOOK		DebuggerHook


//###################################################################################################
//
// 	Set A6800_UPDATEBANK to the name of a function that will check the PC after every 
//	significant branch and perform bankswitching by changing A6800_OPCODEROM and/or
//	A6800_ARGUMENTROM during execution. Its prototype should look like this:
//
//		void UpdateBank(int newpc)
//
//	Leave A6800_UPDATEBANK undefined if you don't need to do any bankswitching within
//	your emulator.
//
//###################################################################################################

//#define A6800_UPDATEBANK		UpdateBank


//###################################################################################################
//
// 	Define A6800_READMEM and A6800_WRITEMEM to the name of your memory read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadMemory(int addr)
//		void WriteMemory(int addr, int val)
//
//###################################################################################################

#ifndef A6800_READMEM
#define A6800_READMEM			ReadMemory
#endif

#ifndef A6800_WRITEMEM
#define A6800_WRITEMEM			WriteMemory
#endif


//###################################################################################################
//
// 	Define A6800_READPORT and A6800_WRITEPORT to the name of your I/O port read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadPort(int addr)
//		void WritePort(int addr, int val)
//
//###################################################################################################

#ifndef A6800_READPORT
#define A6800_READPORT			ReadPort
#endif

#ifndef A6800_WRITEPORT
#define A6800_WRITEPORT			WritePort
#endif


//###################################################################################################
//
// 	Define A6800_OPCODEROM to point to the base of ROM.
//
//	If your code is decrypted into separate opcode and argument areas, point 
//	A6800_ARGUMENTROM to the base of argument ROM.
//
//	These are assumed to be extern unsigned char *'s.
//
//###################################################################################################

#ifndef A6800_OPCODEROM
#define A6800_OPCODEROM			OpcodeROM
#endif

#ifndef A6800_ARGUMENTROM
#define A6800_ARGUMENTROM		A6800_OPCODEROM
#endif


//###################################################################################################
//
// 	Set A6800_ICOUNT to the name of a global variable that should maintain the current
//	cycle downcounter for this CPU. This field is required.
//
//	This variable is assumed to be extern int.
//
//###################################################################################################

#ifndef A6800_ICOUNT
#define A6800_ICOUNT			gICount
#endif


//###################################################################################################
//
// 	Set A6800_VOLATILEREGS to 1 if your emulator needs to examing or change the values of 
//	the PC, A, B, X, or Y registers from within an Asgard6800Execute call.
//
//###################################################################################################

#ifndef A6800_VOLATILEREGS
#define A6800_VOLATILEREGS		1
#endif

#endif
