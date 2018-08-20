//###################################################################################################
//
//
//		Asgard6502DefaultConfig.h
//		This file contains default compile-time configuration and optimization flags.
//
//		See Asgard6502Core.c for information about the Asgard6502 system.
//
//
//###################################################################################################
//
//		WARNING: Do not modify this file! Look at Asgard6502MAME.c for an example of
//		how to configure your core.
//
//###################################################################################################

#ifndef __ASGARD6502DEFAULTCONFIG__
#define __ASGARD6502DEFAULTCONFIG__


//###################################################################################################
//
// 	Set A6502_CHIP to the number of the chip you wish to emulate. Currently supported
//	variants include:
//
//		Rockwell 6502   == 6502
//		Rockwell 65C02  == 65002
//		Rockwell 65SC02 == 658002
//		Rockwell 6510   == 6510
//		Nintendo 2A03   == 2403
//		Deco 16		    == DECO16
//
//	Set A6502_CHIPTIMING to the timing model you wish to use. Leave it undefined to default
//	to the same timing as the time you're emulating (seems reasonable enough, doesn't it?)
//
//###################################################################################################

#ifndef A6502_CHIP
#define A6502_CHIP				6502
#endif

#ifndef A6502_CHIPTIMING
#define A6502_CHIPTIMING		A6502_CHIP
#endif


//###################################################################################################
//
// 	Set A6502_COREDEBUG to 1 to enable a very simple internal debugging system for debugging
//	the CPU core itself. You will need to make sure to link in Asgard6502Debugger.c as well.
//
//###################################################################################################

#ifndef A6502_COREDEBUG
#define A6502_COREDEBUG			0
#endif


//###################################################################################################
//
// 	Define A6502_DEBUGHOOK to the name of a function to be called once per instruction for
//	debugging. Its prototype should look like this:
//
//		void DebuggerHook(void)
//
//	Leave A6502_DEBUGHOOK undefined to get rid of the performance penalty of calling a
//	function once per emulated instruction.
//
//###################################################################################################

//#define A6502_DEBUGHOOK		DebuggerHook


//###################################################################################################
//
// 	Set A6502_UPDATEBANK to the name of a function that will check the PC after every 
//	significant branch and perform bankswitching by changing A6502_OPCODEROM and/or
//	A6502_ARGUMENTROM during execution. Its prototype should look like this:
//
//		void UpdateBank(int newpc)
//
//	Leave A6502_UPDATEBANK undefined if you don't need to do any bankswitching within
//	your emulator.
//
//###################################################################################################

//#define A6502_UPDATEBANK		UpdateBank


//###################################################################################################
//
// 	Define A6502_READMEM and A6502_WRITEMEM to the name of your memory read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadMemory(int addr)
//		void WriteMemory(int addr, int val)
//
//###################################################################################################

#ifndef A6502_READMEM
#define A6502_READMEM			ReadMemory
#endif

#ifndef A6502_WRITEMEM
#define A6502_WRITEMEM			WriteMemory
#endif


//###################################################################################################
//
// 	Define A6502_OPCODEROM to point to the base of ROM.
//
//	If your code is decrypted into separate opcode and argument areas, point 
//	A6502_ARGUMENTROM to the base of argument ROM.
//
//	These are assumed to be extern unsigned char *'s.
//
//###################################################################################################

#ifndef A6502_OPCODEROM
#define A6502_OPCODEROM			gOpcodeROM
#endif

#ifndef A6502_ARGUMENTROM
#define A6502_ARGUMENTROM		A6502_OPCODEROM
#endif


//###################################################################################################
//
// 	Set A6502_VOLATILEREGS to 1 if your emulator needs to examing or change the values of 
//	the PC, A, X, Y, or S registers from within an Asgard6502Execute call.
//
//###################################################################################################

#ifndef A6502_VOLATILEREGS
#define A6502_VOLATILEREGS		1
#endif


#endif
