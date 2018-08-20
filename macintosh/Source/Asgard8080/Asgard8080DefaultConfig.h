//###################################################################################################
//
//
//		Asgard8080DefaultConfig.h
//		This file contains default compile-time configuration and optimization flags.
//
//		See Asgard8080Core.c for information about the Asgard8080 system.
//
//
//###################################################################################################
//
//		WARNING: Do not modify this file! Look at Asgard8080MAME.c for an example of
//		how to configure your core.
//
//###################################################################################################

#ifndef __ASGARD8080DEFAULTCONFIG__
#define __ASGARD8080DEFAULTCONFIG__


//###################################################################################################
//
// 	Set A8080_CHIP to the number of the chip you wish to emulate. Currently supported
//	variants include:
//
//		Intel 8080  == 8080
//		Intel 8085A	== 8085
//
//	Set A8080_CHIPTIMING to the timing model you wish to use. Leave it undefined to default
//	to the same timing as the time you're emulating (seems reasonable enough, doesn't it?)
//
//###################################################################################################

#ifndef A8080_CHIP
#define A8080_CHIP				8080
#endif

#ifndef A8080_CHIPTIMING
#define A8080_CHIPTIMING		A8080_CHIP
#endif


//###################################################################################################
//
// 	Set A8080_COREDEBUG to 1 to enable a very simple internal debugging system for debugging
//	the CPU core itself. You will need to make sure to link in Asgard8080Debugger.c as well.
//
//###################################################################################################

#ifndef A8080_COREDEBUG
#define A8080_COREDEBUG			0
#endif


//###################################################################################################
//
// 	Define A8080_DEBUGHOOK to the name of a function to be called once per instruction for
//	debugging. Its prototype should look like this:
//
//		void DebuggerHook(void)
//
//	Leave A8080_DEBUGHOOK undefined to get rid of the performance penalty of calling a
//	function once per emulated instruction.
//
//###################################################################################################

//#define A8080_DEBUGHOOK		DebuggerHook


//###################################################################################################
//
// 	Set A8080_UPDATEBANK to the name of a function that will check the PC after every 
//	significant branch and perform bankswitching by changing A8080_OPCODEROM and/or
//	A8080_ARGUMENTROM during execution. Its prototype should look like this:
//
//		void UpdateBank(int newpc)
//
//	Leave A8080_UPDATEBANK undefined if you don't need to do any bankswitching within
//	your emulator.
//
//###################################################################################################

//#define A8080_UPDATEBANK		UpdateBank


//###################################################################################################
//
// 	Define A8080_READMEM and A8080_WRITEMEM to the name of your memory read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadMemory(int addr)
//		void WriteMemory(int addr, int val)
//
//###################################################################################################

#ifndef A8080_READMEM
#define A8080_READMEM			ReadMemory
#endif

#ifndef A8080_WRITEMEM
#define A8080_WRITEMEM			WriteMemory
#endif


//###################################################################################################
//
// 	Define A8080_READPORT and A8080_WRITEPORT to the name of your I/O port read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadPort(int addr)
//		void WritePort(int addr, int val)
//
//###################################################################################################

#ifndef A8080_READPORT
#define A8080_READPORT			ReadPort
#endif

#ifndef A8080_WRITEPORT
#define A8080_WRITEPORT			WritePort
#endif


//###################################################################################################
//
// 	Define A8080_OPCODEROM to point to the base of ROM.
//
//	If your code is decrypted into separate opcode and argument areas, point 
//	A8080_ARGUMENTROM to the base of argument ROM.
//
//	These are assumed to be extern unsigned char *'s.
//
//###################################################################################################

#ifndef A8080_OPCODEROM
#define A8080_OPCODEROM			gOpcodeROM
#endif

#ifndef A8080_ARGUMENTROM
#define A8080_ARGUMENTROM		A8080_OPCODEROM
#endif


//###################################################################################################
//
// 	Set A8080_VOLATILEREGS to 1 if your emulator needs to examing or change the values of 
//	the PC, A, B, X, Y, S, or U registers from within an Asgard8080Execute call.
//
//###################################################################################################

#ifndef A8080_VOLATILEREGS
#define A8080_VOLATILEREGS		1
#endif


#endif
