//###################################################################################################
//
//
//		AsgardZ80DefaultConfig.h
//		This file contains default compile-time configuration and optimization flags.
//
//		See AsgardZ80Core.c for information about the AsgardZ80 system.
//
//
//###################################################################################################
//
//		WARNING: Do not modify this file! Look at AsgardZ80MAME.c for an example of
//		how to configure your core.
//
//###################################################################################################

#ifndef __ASGARDZ80DEFAULTCONFIG__
#define __ASGARDZ80DEFAULTCONFIG__


//###################################################################################################
//
// 	Set AZ80_COREDEBUG to 1 to enable a very simple internal debugging system for debugging
//	the CPU core itself. You will need to make sure to link in AsgardZ80Debugger.c as well.
//
//###################################################################################################

#ifndef AZ80_COREDEBUG
#define AZ80_COREDEBUG			0
#endif


//###################################################################################################
//
// 	Define AZ80_DEBUGHOOK to the name of a function to be called once per instruction for
//	debugging. Its prototype should look like this:
//
//		void DebuggerHook(void)
//
//	Leave AZ80_DEBUGHOOK undefined to get rid of the performance penalty of calling a
//	function once per emulated instruction.
//
//###################################################################################################

//#define AZ80_DEBUGHOOK		DebuggerHook


//###################################################################################################
//
// 	Set AZ80_UPDATEBANK to the name of a function that will check the PC after every 
//	significant branch and perform bankswitching by changing AZ80_OPCODEROM and/or
//	AZ80_ARGUMENTROM during execution. Its prototype should look like this:
//
//		void UpdateBank(int newpc)
//
//	Leave AZ80_UPDATEBANK undefined if you don't need to do any bankswitching within
//	your emulator.
//
//###################################################################################################

//#define AZ80_UPDATEBANK		UpdateBank


//###################################################################################################
//
// 	Define AZ80_READMEM and AZ80_WRITEMEM to the name of your memory read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadMemory(int addr)
//		void WriteMemory(int addr, int val)
//
//###################################################################################################

#ifndef AZ80_READMEM
#define AZ80_READMEM			ReadMemory
#endif

#ifndef AZ80_WRITEMEM
#define AZ80_WRITEMEM			WriteMemory
#endif


//###################################################################################################
//
// 	Define AZ80_READPORT and AZ80_WRITEPORT to the name of your I/O port read and write
//	functions.  Their prototypes should look like this:
//
//		int ReadPort(int addr)
//		void WritePort(int addr, int val)
//
//###################################################################################################

#ifndef AZ80_READPORT
#define AZ80_READPORT			ReadPort
#endif

#ifndef AZ80_WRITEPORT
#define AZ80_WRITEPORT			WritePort
#endif


//###################################################################################################
//
// 	Define AZ80_OPCODEROM to point to the base of ROM.
//
//	If your code is decrypted into separate opcode and argument areas, point 
//	AZ80_ARGUMENTROM to the base of argument ROM.
//
//	These are assumed to be extern unsigned char *'s.
//
//###################################################################################################

#ifndef AZ80_OPCODEROM
#define AZ80_OPCODEROM			gOpcodeROM
#endif

#ifndef AZ80_ARGUMENTROM
#define AZ80_ARGUMENTROM		AZ80_OPCODEROM
#endif


//###################################################################################################
//
// 	Set AZ80_ICOUNT to the name of a global variable that should maintain the current
//	cycle downcounter for this CPU. This field is required.
//
//	This variable is assumed to be extern int.
//
//###################################################################################################

#ifndef AZ80_ICOUNT
#define AZ80_ICOUNT				gICount
#endif


//###################################################################################################
//
// 	Set AZ80_VOLATILEREGS to 1 if your emulator needs to examing or change the values of 
//	the PC, A, B, X, Y, S, or U registers from within an AsgardZ80Execute call.
//
//###################################################################################################

#ifndef AZ80_VOLATILEREGS
#define AZ80_VOLATILEREGS		1
#endif


#endif
