//###################################################################################################
//
//
//		Asgard6809DefaultConfig.h
//		This file contains default compile-time configuration and optimization flags.
//
//		See Asgard6809Core.c for information about the Asgard6809 system.
//
//
//###################################################################################################
//
//		WARNING: Do not modify this file! Look at Asgard6809MAME.c for an example of
//		how to configure your core.
//
//###################################################################################################

#ifndef __ASGARD6809DEFAULTCONFIG__
#define __ASGARD6809DEFAULTCONFIG__


//###################################################################################################
//
// 	Set A6809_CHIP to the number of the chip you wish to emulate. Currently supported
//	variants include:
//
//		Motorola M6809  == 6809
//		Konami 5000x	== 5000
//		Hitachi HD6309  == 6309     (note: full support is not implemented yet!)
//
//###################################################################################################

#ifndef A6809_CHIP
#define A6809_CHIP				6809
#endif


//###################################################################################################
//
// 	Set A6809_COREDEBUG to 1 to enable a very simple internal debugging system for debugging
//	the CPU core itself. You will need to make sure to link in Asgard6809Debugger.c as well.
//
//###################################################################################################

#ifndef A6809_COREDEBUG
#define A6809_COREDEBUG			0
#endif


//###################################################################################################
//
// 	Define A6809_DEBUGHOOK to the name of a function to be called once per instruction for
//	debugging. Its prototype should look like this:
//
//		void DebuggerHook(void)
//
//	Leave A6809_DEBUGHOOK undefined to get rid of the performance penalty of calling a
//	function once per emulated instruction.
//
//###################################################################################################

//#define A6809_DEBUGHOOK		DebuggerHook


//###################################################################################################
//
// 	Set A6809_UPDATEBANK to the name of a function that will check the PC after every 
//	significant branch and perform bankswitching by changing A6809_OPCODEROM and/or
//	A6809_ARGUMENTROM during execution. Its prototype should look like this:
//
//		void UpdateBank(int newpc)
//
//	Leave A6809_UPDATEBANK undefined if you don't need to do any bankswitching within
//	your emulator.
//
//###################################################################################################

//#define A6809_UPDATEBANK		UpdateBank


//###################################################################################################
//
// 	Define A6809_READMEM and A6809_WRITEMEM to the name of your memory read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadMemory(int addr)
//		void WriteMemory(int addr, int val)
//
//###################################################################################################

#ifndef A6809_READMEM
#define A6809_READMEM			ReadMemory
#endif

#ifndef A6809_WRITEMEM
#define A6809_WRITEMEM			WriteMemory
#endif


//###################################################################################################
//
// 	Define A6809_OPCODEROM to point to the base of ROM.
//
//	If your code is decrypted into separate opcode and argument areas, point 
//	A6809_ARGUMENTROM to the base of argument ROM.
//
//	These are assumed to be extern unsigned char *'s.
//
//###################################################################################################

#ifndef A6809_OPCODEROM
#define A6809_OPCODEROM			OpcodeROM
#endif

#ifndef A6809_ARGUMENTROM
#define A6809_ARGUMENTROM		A6809_OPCODEROM
#endif


//###################################################################################################
//
// 	Set A6809_ICOUNT to the name of a global variable that should maintain the current
//	cycle downcounter for this CPU. This field is required.
//
//	This variable is assumed to be extern int.
//
//###################################################################################################

#ifndef A6809_ICOUNT
#define A6809_ICOUNT			gICount
#endif


//###################################################################################################
//
// 	Set A6809_SETLINES to the name of a global function pointer that will be called whenever the
//	Konami setlines opcodes are executed to change the address lines A23-A16.
//
//	This variable is assumed to be extern void (*setlines)(int lines).
//
//###################################################################################################

#ifndef A6809_SETLINES
#define A6809_SETLINES			gSetLines
#endif


//###################################################################################################
//
// 	Set A6809_VOLATILEREGS to 1 if your emulator needs to examing or change the values of 
//	the PC, A, B, X, Y, S, or U registers from within an Asgard6809Execute call.
//
//###################################################################################################

#ifndef A6809_VOLATILEREGS
#define A6809_VOLATILEREGS		1
#endif


#endif
