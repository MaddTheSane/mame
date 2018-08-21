//###################################################################################################
//
//
//		AsgardZ80Internals.h
//		This file contains internal data and structures that were too cumbersome to include
//		in the main AsgardZ80.h.
//
//		See AsgardZ80Core.c for information about the AsgardZ80 system.
//
//
//###################################################################################################

#ifndef __ASGARDZ80INTERNALS__
#define __ASGARDZ80INTERNALS__


#if AZ80_COREDEBUG
#include "AsgardZ80Debugger.h"
#endif


//###################################################################################################
//
// 	Here is how the Flags internal register is mapped out
//
//###################################################################################################

enum
{
	kFlagsR					= 0xff000000,		// R		= bits 00-07
	kFlagsR2				= 0x00ff0000,		// R2		= bits 08-15
	kFlagsI					= 0x0000ff00,		// I		= bits 16-23
	kFlagsIFF1				= 0x00000080,		// IFF1		= bit  24
	kFlagsIFF2				= 0x00000040,		// IFF2		= bit  25
	kFlagsHALT				= 0x00000020,		// HALT		= bit  26
	kFlagsDirty				= 0x00000008,		// Dirty	= bit  28
	kFlagsIM				= 0x00000003		// IM		= bits 30-31
};


//###################################################################################################
//
// 	These macros can be used in __rlwinm instructions to extract the appropriate flag
//
//###################################################################################################

#define EXTRACT_R			 8,24,31
#define EXTRACT_R2			16,24,31
#define EXTRACT_I			24,24,31
#define EXTRACT_IFF1		25,31,31
#define EXTRACT_IFF2		26,31,31
#define EXTRACT_HALT		27,31,31
#define EXTRACT_DIRTY		29,31,31
#define EXTRACT_IM			 0,30,31


//###################################################################################################
//
// 	These macros can be used in __rlwimi instructions to insert the appropriate flag
//
//###################################################################################################

#define INSERT_R			24, 0, 7
#define INSERT_R2			16, 8,15
#define INSERT_I			 8,16,23
#define INSERT_IFF1			 7,24,24
#define INSERT_IFF2			 6,25,25
#define INSERT_HALT			 5,26,26
#define INSERT_IM			 0,30,31


//###################################################################################################
//
// 	These are the nonvolatile registers we use during execution
//
//###################################################################################################

#define	rPCAF				r31				/* holds the PC, A & F registers */
#define	rHLBC				r30				/* holds the H, L, B & C registers */
#define	rSPDE				r29				/* holds the SP, D & E registers */
#define	rIXIY				r28				/* holds the IX & IY registers */
#define	rAFDE2				r27				/* holds the A', F', D' & E' registers */
#define	rHLBC2				r26				/* holds the H', L', B' & C' registers */
#define	rFlags				r25				/* holds the internal flags */
#define	rICount				r24				/* holds the current number of cycles left to execute */
#define	rICountPtr			r23				/* holds a pointer to the global that stores rICount */
#define	rCycleCount			r22				/* total cycle count for this instruction */
#define	rTempSave2			r21				/* holds the current base of the ROM space */
#define	rTempSave3			r20				/* holds a pointer to the global that stores rOpcodeROM */
#define	rTempSave4			r19				/* holds the current base of the argument ROM space */
#define	rTempSave5			r18				/* holds a pointer to the global that stores rArgumentROM */
#define	rOpcodeTable		r17				/* holds a pointer to the opcode lookup table */
#define	rFlagTable			r16				/* holds a pointer to the flags lookup table */
#define	rEA					r15				/* holds the current effective address */
#define	rTempSave			r14				/* temporary holding register for a bunch of stuff */
#define	rLastNonVolatile	r14				/* the last non-volatible register we use */


//###################################################################################################
//
// 	Macros to transparently support bank updating
//
//###################################################################################################

#ifdef AZ80_UPDATEBANK
	#define UPDATE_BANK		bl		updateBank;
#else
	#define UPDATE_BANK
#endif


//###################################################################################################
//
// 	Macros to transparently support internal memory debugging
//
//###################################################################################################

#if AZ80_COREDEBUG
	#define READMEM			AsgardZ80DebugRead
	#define WRITEMEM		AsgardZ80DebugWrite
#else
	#define READMEM			AZ80_READMEM
	#define WRITEMEM		AZ80_WRITEMEM
#endif


//###################################################################################################
//
// 	These macros are used to extract the given Z80 register into the specified PPC register
//
//###################################################################################################

#define GET_A(r)			rlwinm	r,rPCAF,24,24,31;
#define GET_B(r)			rlwinm	r,rHLBC,24,24,31;
#define GET_C(r)			rlwinm	r,rHLBC,0,24,31;
#define GET_D(r)			rlwinm	r,rSPDE,24,24,31;
#define GET_E(r)			rlwinm	r,rSPDE,0,24,31;
#define GET_H(r)			rlwinm	r,rHLBC,8,24,31;
#define GET_L(r)			rlwinm	r,rHLBC,16,24,31;
#define GET_IXH(r)			rlwinm	r,rIXIY,8,24,31;
#define GET_IXL(r)			rlwinm	r,rIXIY,16,24,31;
#define GET_IYH(r)			rlwinm	r,rIXIY,24,24,31;
#define GET_IYL(r)			rlwinm	r,rIXIY,0,24,31;
#define GET_AF(r)			rlwinm	r,rPCAF,0,16,31;
#define GET_BC(r)			rlwinm	r,rHLBC,0,16,31;
#define GET_DE(r)			rlwinm	r,rSPDE,0,16,31;
#define GET_HL(r)			rlwinm	r,rHLBC,16,16,31;
#define GET_IX(r)			rlwinm	r,rIXIY,16,16,31;
#define GET_IY(r)			rlwinm	r,rIXIY,0,16,31;
#define GET_SP(r)			rlwinm	r,rSPDE,16,16,31;
#define GET_PC(r)			rlwinm	r,rPCAF,16,16,31;
#define GET_AF2(r)			rlwinm	r,rAFDE2,16,16,31;
#define GET_BC2(r)			rlwinm	r,rHLBC2,0,16,31;
#define GET_DE2(r)			rlwinm	r,rAFDE2,0,16,31;
#define GET_HL2(r)			rlwinm	r,rHLBC2,16,16,31;


//###################################################################################################
//
// 	These macros are used to set the given Z80 register to the value in the specified PPC register
//
//###################################################################################################

#define SET_A(r)			rlwimi	rPCAF,r,8,16,23;
#define SET_B(r)			rlwimi	rHLBC,r,8,16,23;
#define SET_C(r)			rlwimi	rHLBC,r,0,24,31;
#define SET_D(r)			rlwimi	rSPDE,r,8,16,23;
#define SET_E(r)			rlwimi	rSPDE,r,0,24,31;
#define SET_H(r)			rlwimi	rHLBC,r,24,0,7;
#define SET_L(r)			rlwimi	rHLBC,r,16,8,15;
#define SET_IXH(r)			rlwimi	rIXIY,r,24,0,7;
#define SET_IXL(r)			rlwimi	rIXIY,r,16,8,15;
#define SET_IYH(r)			rlwimi	rIXIY,r,8,16,23;
#define SET_IYL(r)			rlwimi	rIXIY,r,0,24,31;
#define SET_AF(r)			rlwimi	rPCAF,r,0,16,31;
#define SET_BC(r)			rlwimi	rHLBC,r,0,16,31;
#define SET_DE(r)			rlwimi	rSPDE,r,0,16,31;
#define SET_HL(r)			rlwimi	rHLBC,r,16,0,15;
#define SET_IX(r)			rlwimi	rIXIY,r,16,0,15;
#define SET_IY(r)			rlwimi	rIXIY,r,0,16,31;
#define SET_SP(r)			rlwimi	rSPDE,r,16,0,15;
#define SET_PC(r)			rlwimi	rPCAF,r,16,0,15;


//###################################################################################################
//
// 	These macros are used to save & load the current Z80 global state during execution
//
//###################################################################################################

// control flags are always saved/loaded
#define SAVE_FLAGS			_asm_set_global(rFlags,sFlags);
#define LOAD_FLAGS			_asm_get_global(rFlags,sFlags);

// the current execution cycle count is always saved/loaded
#define SAVE_ICOUNT			stw		rICount,0(rICountPtr);
#define LOAD_ICOUNT			lwz		rICount,0(rICountPtr);

// the OPCODEROM pointer is loaded only if we do bank switching
#if 0 // #ifdef AZ80_UPDATEBANK
	#define LOAD_ROM		lwz		rOpcodeROM,0(rOpcodeROMPtr);\
							lwz		rArgumentROM,0(rArgumentROMPtr);
#else
	#define LOAD_ROM
#endif

// the registers are optionally saved/loaded
#if AZ80_VOLATILEREGS
	#define SAVE_PCAF		_asm_set_global(rPCAF,sPCAF);
	#define LOAD_PCAF		_asm_get_global(rPCAF,sPCAF);
	#define SAVE_SPDE		_asm_set_global(rSPDE,sSPDE);
	#define LOAD_SPDE		_asm_get_global(rSPDE,sSPDE);
	#define SAVE_HLBC		_asm_set_global(rHLBC,sHLBC);
	#define LOAD_HLBC		_asm_get_global(rHLBC,sHLBC);
	#define SAVE_IXIY		_asm_set_global(rIXIY,sIXIY);
	#define LOAD_IXIY		_asm_get_global(rIXIY,sIXIY);
	#define SAVE_AFDE2		_asm_set_global(rAFDE2,sAFDE2);
	#define LOAD_AFDE2		_asm_get_global(rAFDE2,sAFDE2);
	#define SAVE_HLBC2		_asm_set_global(rHLBC2,sHLBC2);
	#define LOAD_HLBC2		_asm_get_global(rHLBC2,sHLBC2);
#else
	#define SAVE_PCAF
	#define LOAD_PCAF
	#define SAVE_SPDE
	#define LOAD_SPDE
	#define SAVE_HLBC
	#define LOAD_HLBC
	#define SAVE_IXIY
	#define LOAD_IXIY
	#define SAVE_AFDE2
	#define LOAD_AFDE2
	#define SAVE_HLBC2
	#define LOAD_HLBC2
#endif


//###################################################################################################
//
// 	Count cycles for this operation; should be done before registers are saved
//
//###################################################################################################

#define CYCLES(cycles)		addi	rCycleCount,rCycleCount,cycles;


//###################################################################################################
//
// 	EI flag - stored in a nonvolatile cr register for quick branching
//
//###################################################################################################

#define EI_FLAG_RESET		cmpwi	cr2,sp,0		// assumes stack is never 0 :-)
#define EI_FLAG_SET			cmpw	cr2,r0,r0
#define BRANCH_ON_EI(x)		beq-	cr2,x


//###################################################################################################
//
// 	Read an opcode from OPCODEROM to 'r' and increment the PC
//
//###################################################################################################

#if 1
#define READ_OPCODE(r) \
	GET_PC(r3) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
	SAVE_PCAF \
	SAVE_ICOUNT \
	SAVE_FLAGS \
	SAVE_SPDE\
	SAVE_HLBC\
	SAVE_IXIY\
	SAVE_AFDE2\
	SAVE_HLBC2\
	bl 		AZ80_READOP; \
	rlwinm	r3,r3,0,24,31;			/* keep the byte in range */\
	mr		r,r3; \
	bl		postExtern;				/* post-process */

#define READ_OPCODE_ARG(r) \
	GET_PC(r3) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
	SAVE_PCAF \
	SAVE_ICOUNT \
	SAVE_FLAGS \
	SAVE_SPDE\
	SAVE_HLBC\
	SAVE_IXIY\
	SAVE_AFDE2\
	SAVE_HLBC2\
	bl		AZ80_READOPARG; \
	rlwinm	r3,r3,0,24,31;			/* keep the byte in range */\
	mr		r,r3; \
	bl		postExtern;				/* post-process */
#else
#define READ_OPCODE(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
	lbzx	r,rOpcodeROM,r;			/* r = immediate byte value */

#define READ_OPCODE_ARG(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
	lbzx	r,rArgumentROM,r;		/* r = immediate byte value */
#endif


//###################################################################################################
//
// 	Read an opcode argument word from OPCODERAM to 'r' and increment the PC
//
//###################################################################################################

#if 1
#define READ_OPCODE_ARG_WORD(r) \
	GET_PC(rTempSave2) 				/* fetch the current PC */\
	addis	rPCAF,rPCAF,2;			/* increment & wrap the PC */\
	SAVE_PCAF \
	SAVE_ICOUNT \
	SAVE_FLAGS \
	SAVE_SPDE\
	SAVE_HLBC\
	SAVE_IXIY\
	SAVE_AFDE2\
	SAVE_HLBC2\
	rlwinm	r3,rTempSave2,0,16,31;	/* get the address in r3 */\
	bl		AZ80_READOPARG;			/* perform the read */\
	addi	r4,rTempSave2,1;		/* add one to the address */\
	rlwinm	rTempSave,r3,0,24,31;	/* save the result temporarily */\
	rlwinm	r3,r4,0,16,31;			/* keep it in range */\
	bl		AZ80_READOPARG;			/* perform the read */\
	rlwimi	rTempSave,r3,8,16,23;	/* combine the two bytes */\
	mr		r,rTempSave; \
	bl		postExtern;				/* post-process */
#else
#define READ_OPCODE_ARG_WORD(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,2;			/* increment & wrap the PC */\
	lhbrx	r,rArgumentROM,r;		/* r = immediate byte value */
#endif


//###################################################################################################
//
// 	Read a byte from memory address contained in 'reg' into r3, saving the registers as well
//
//###################################################################################################

#define READ_AT_REG_SAVE(reg) \
	SAVE_PCAF						/* save PC/AF */\
	GET_##reg(r3)					/* r3 = address */\
	SAVE_ICOUNT						/* save ICount */\
	bl		READMEM;				/* perform the read */\
	rlwinm	r3,r3,0,24,31;			/* keep the byte in range */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Read a byte from memory address contained in 'reg' into r3, saving the registers as well;
//	this version also puts a copy of the address into rEA
//
//###################################################################################################

#define READ_AT_REG_SAVE_EA(reg) \
	SAVE_PCAF						/* save PC/AF */\
	GET_##reg(r3)					/* rEA = address */\
	SAVE_ICOUNT						/* save ICount */\
	GET_##reg(rEA)					/* rEA = address */\
	bl		READMEM;				/* perform the read */\
	rlwinm	r3,r3,0,24,31;			/* keep the byte in range */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Write a byte from r4 to ('reg')
//
//###################################################################################################

#define WRITE_AT_REG(reg) \
	GET_##reg(r3)					/* r3 = address */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Write a byte from r4 to ('reg'), saving the registers as well
//
//###################################################################################################

#define WRITE_AT_REG_SAVE(reg) \
	SAVE_PCAF						/* save PC/AF */\
	GET_##reg(r3)					/* r3 = address */\
	SAVE_ICOUNT						/* save ICount */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Write a byte from r4 to ('reg'+(PC++)), saving the registers as well
//
//###################################################################################################

#define WRITE_AT_REG_OFFSET_SAVE(reg) \
	SAVE_ICOUNT						/* save ICount */\
	READ_OPCODE_ARG(r5)				/* r5 = current PC */\
	extsb	r5,r5;					/* sign extend the offset */\
	GET_##reg(rEA)					/* rEA = address */\
	add		rEA,rEA,r5;				/* rEA = address + offset */\
	SAVE_PCAF						/* save PC/AF */\
	rlwinm	r3,rEA,0,16,31;			/* keep EA in range */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Reads a word from memory address 'r' into rTempSave, saving the registers as well
//
//###################################################################################################

#define READ_WORD_SAVE(r) \
	SAVE_PCAF						/* save PC/AF */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	SAVE_ICOUNT						/* save ICount */\
	bl		READMEM;				/* perform the read */\
	addi	r4,r,1;					/* add one to the address */\
	rlwinm	rTempSave,r3,0,24,31;	/* save the result temporarily */\
	rlwinm	r3,r4,0,16,31;			/* keep it in range */\
	bl		READMEM;				/* perform the read */\
	rlwimi	rTempSave,r3,8,16,23;	/* combine the two bytes */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Writes a byte to memory address 'r' from register r4, saving the registers as well
//
//###################################################################################################

#define WRITE_BYTE_SAVE(r) \
	SAVE_PCAF						/* save PC/AF */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	SAVE_ICOUNT						/* save ICount */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Writes a word to memory address 'r' from the high/low 16 bits of register 'd'
//
//###################################################################################################

#define WRITE_WORD_HI(r,d) \
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	rlwinm	r4,d,16,24,31;			/* get the low byte of data in r4 */\
	bl		WRITEMEM;				/* perform the write */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r4,d,8,24,31;			/* get the high byte of data in r4 */\
	rlwinm	r3,r3,0,16,31;			/* keep it in range */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */

#define WRITE_WORD_LO(r,d) \
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	rlwinm	r4,d,0,24,31;			/* get the low byte of data in r4 */\
	bl		WRITEMEM;				/* perform the write */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r4,d,24,24,31;			/* get the high byte of data in r4 */\
	rlwinm	r3,r3,0,16,31;			/* keep it in range */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Writes a word to memory address 'r' from the high/low 16 bits of register 'd', 
//	saving the registers as well
//
//###################################################################################################

#define WRITE_WORD_HI_SAVE(r,d) \
	SAVE_PCAF						/* save PC/AF */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	rlwinm	r4,d,16,24,31;			/* get the low byte of data in r4 */\
	SAVE_ICOUNT						/* save ICount */\
	bl		WRITEMEM;				/* perform the write */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r4,d,8,24,31;			/* get the high byte of data in r4 */\
	rlwinm	r3,r3,0,16,31;			/* keep it in range */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */

#define WRITE_WORD_LO_SAVE(r,d) \
	SAVE_PCAF						/* save PC/AF */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	rlwinm	r4,d,0,24,31;			/* get the low byte of data in r4 */\
	SAVE_ICOUNT						/* save ICount */\
	bl		WRITEMEM;				/* perform the write */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r4,d,24,24,31;			/* get the high byte of data in r4 */\
	rlwinm	r3,r3,0,16,31;			/* keep it in range */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Clears bit 'n' in register 's', using offset 'o', and puts the result into 'd'
//
//###################################################################################################

#define RES(d,s,n,o) \
	rlwinm 	d,s,0,(32-o-n)&31,(32-o-n-2)&31


//###################################################################################################
//
// 	Sets bit 'n' in register 's', using offset 'o', and puts the result into 'd'
//
//###################################################################################################

#define SET_HI(d,s,n,o) \
	oris 	d,s,(1<<(n+o))

#define SET_LO(d,s,n,o) \
	ori  	d,s,(1<<(n+o))


//###################################################################################################
//
// 	Performs a RL operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define RL(r,o) \
	rlwinm	r4,r,(1-o)&31,24,30;	/* r4 = r3 << 1 */\
	rlwimi	r4,rPCAF,0,31,31;		/* r4 = (r3 << 1) | C */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(25-o)&31,31,31;/* C = (r3 & 0x80) >> 7 */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a RLC operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define RLC(r,o) \
	rlwinm	r4,r,(1-o)&31,24,30;	/* r4 = r3 << 1 */\
	rlwimi	r4,r,(25-o)&31,31,31;	/* r4 = (r3 << 1) | (r3 >> 7) */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(25-o)&31,31,31;/* C = (r3 & 0x80) >> 7 */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a RR operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define RR(r,o) \
	rlwinm	r4,r,(31-o)&31,25,31;	/* r4 = r3 >> 1 */\
	rlwimi	r4,rPCAF,7,24,24;		/* r4 = (r3 >> 1) | (C << 7) */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(32-o)&31,31,31;/* C = (r3 & 1) */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a RRC operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define RRC(r,o) \
	rlwinm	r4,r,(31-o)&31,25,31;	/* r4 = r3 >> 1 */\
	rlwimi	r4,r,(7-o)&31,24,24;	/* r4 = (r3 >> 1) | (r3 << 7) */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(32-o)&31,31,31;/* C = (r3 & 1) */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a SLL operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define SLL(r,o) \
	rlwinm	r4,r,(1-o)&31,24,30;	/* r4 = r3 << 1 */\
	ori		r4,r4,1;				/* r4 = (r3 << 1) | 1 */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(25-o)&31,31,31;/* C = (r3 & 0x80) >> 7 */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a SLA operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define SLA(r,o) \
	rlwinm	r4,r,(1-o)&31,24,30;	/* r4 = r3 << 1 */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(25-o)&31,31,31;/* C = (r3 & 0x80) >> 7 */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a SRL operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define SRL(r,o) \
	rlwinm	r4,r,(31-o)&31,25,31;	/* r4 = r3 >> 1 */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(0-o)&31,31,31;	/* C = (r3 & 1) */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	Performs a SRA operation on the register 'r', byte at offset 'o'; result is in r4
//
//###################################################################################################

#define SRA(r,o) \
	rlwinm	r4,r,(31-o)&31,25,31;	/* r4 = r3 >> 1 */\
	rlwimi	r4,r,(0-o)&31,24,24;	/* r4 = (r3 >> 1) | (r3 & 0x80) */\
	lbzx	r5,rFlagTable,r4;		/* r5 = flag bits */\
	rlwimi	rPCAF,r,(0-o)&31,31,31;	/* C = (r3 & 1) */\
	rlwimi	rPCAF,r5,0,24,30;		/* set the other flags */


//###################################################################################################
//
// 	PUSHes the high or low part of the specified 'reg', optionally saving the registers as well
//
//###################################################################################################

#define PUSH_HI(reg) \
	subis	rSPDE,rSPDE,1;			/* decrement the sp */\
	rlwinm	r4,reg,8,24,31;			/* get the high part into r4 */\
	GET_SP(r3)						/* get the sp */\
	subis	rSPDE,rSPDE,1;			/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	rlwinm	r4,reg,16,24,31;		/* get the low part into r4 */\
	GET_SP(r3)						/* get the sp */\
	bl		WRITEMEM;				/* perform the write */\
	SAVE_SPDE						/* save the new sp */\
	bl		postExtern;				/* post-process */

#define PUSH_HI_SAVE(reg) \
	SAVE_PCAF						/* save PC/AF */\
	subis	rSPDE,rSPDE,1;			/* decrement the sp */\
	SAVE_ICOUNT						/* save ICOUNT */\
	rlwinm	r4,reg,8,24,31;			/* get the high part into r4 */\
	GET_SP(r3)						/* get the sp */\
	subis	rSPDE,rSPDE,1;			/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	rlwinm	r4,reg,16,24,31;		/* get the low part into r4 */\
	GET_SP(r3)						/* get the sp */\
	bl		WRITEMEM;				/* perform the write */\
	SAVE_SPDE						/* save the new sp */\
	bl		postExtern;				/* post-process */

#define PUSH_LO_SAVE(reg) \
	SAVE_PCAF						/* save PC/AF */\
	subis	rSPDE,rSPDE,1;			/* decrement the sp */\
	SAVE_ICOUNT						/* save ICOUNT */\
	rlwinm	r4,reg,24,24,31;		/* get the high part into r4 */\
	GET_SP(r3)						/* get the sp */\
	subis	rSPDE,rSPDE,1;			/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	rlwinm	r4,reg,0,24,31;			/* get the low part into r4 */\
	GET_SP(r3)						/* get the sp */\
	bl		WRITEMEM;				/* perform the write */\
	SAVE_SPDE						/* save the new sp */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	POPs the high or low part of the specified 'reg', saving the registers as well
//
//###################################################################################################

#define POP_HI_SAVE(reg) \
	SAVE_PCAF						/* save PC/AF */\
	GET_SP(r3)						/* get the sp */\
	addis	rSPDE,rSPDE,1;			/* increment the sp */\
	SAVE_ICOUNT						/* save ICOUNT */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,16,8,15;			/* extract the low bits */\
	GET_SP(r3)						/* get the sp */\
	addis	rSPDE,rSPDE,1;			/* increment the sp */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,24,0,7;			/* extract the high bits */\
	SAVE_SPDE						/* save the new sp */\
	bl		postExtern;				/* post-process */
	
#define POP_LO_SAVE(reg) \
	SAVE_PCAF						/* save PC/AF */\
	GET_SP(r3)						/* get the sp */\
	addis	rSPDE,rSPDE,1;			/* increment the sp */\
	SAVE_ICOUNT						/* save ICOUNT */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,0,24,31;			/* extract the low bits */\
	GET_SP(r3)						/* get the sp */\
	addis	rSPDE,rSPDE,1;			/* increment the sp */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,8,16,23;			/* extract the high bits */\
	SAVE_SPDE						/* save the new sp */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

// externally defined functions
extern data8_t		AZ80_READMEM(offs_t address);
extern void 		AZ80_WRITEMEM(offs_t address, data8_t value);

// prototype for the debugger hook
#ifdef AZ80_DEBUGHOOK
extern void 		AZ80_DEBUGHOOK(void);
#endif

// prototype for the bank switcher
#ifdef AZ80_UPDATEBANK
extern void 		AZ80_UPDATEBANK(int newpc);
#endif

// prototypes for the opcode handlers
static void adc_a_xhl(void);
static void adc_a_xix(void);
static void adc_a_xiy(void);
static void adc_a_a(void);
static void adc_a_b(void);
static void adc_a_c(void);
static void adc_a_d(void);
static void adc_a_e(void);
static void adc_a_h(void);
static void adc_a_l(void);
static void adc_a_ixl(void);
static void adc_a_ixh(void);
static void adc_a_iyl(void);
static void adc_a_iyh(void);
static void adc_a_byte(void);

static void adc_hl_bc(void);
static void adc_hl_de(void);
static void adc_hl_hl(void);
static void adc_hl_sp(void);

static void add_a_xhl(void);
static void add_a_xix(void);
static void add_a_xiy(void);
static void add_a_a(void);
static void add_a_b(void);
static void add_a_c(void);
static void add_a_d(void);
static void add_a_e(void);
static void add_a_h(void);
static void add_a_l(void);
static void add_a_ixl(void);
static void add_a_ixh(void);
static void add_a_iyl(void);
static void add_a_iyh(void);
static void add_a_byte(void);

static void add_hl_bc(void);
static void add_hl_de(void);
static void add_hl_hl(void);
static void add_hl_sp(void);
static void add_ix_bc(void);
static void add_ix_de(void);
static void add_ix_ix(void);
static void add_ix_sp(void);
static void add_iy_bc(void);
static void add_iy_de(void);
static void add_iy_iy(void);
static void add_iy_sp(void);

static void and_xhl(void);
static void and_xix(void);
static void and_xiy(void);
static void and_a(void);
static void and_b(void);
static void and_c(void);
static void and_d(void);
static void and_e(void);
static void and_h(void);
static void and_l(void);
static void and_ixh(void);
static void and_ixl(void);
static void and_iyh(void);
static void and_iyl(void);
static void and_byte(void);

static void bit_0_xhl(void);
static void bit_0_xix(void);
static void bit_0_xiy(void);
static void bit_0_a(void);
static void bit_0_b(void);
static void bit_0_c(void);
static void bit_0_d(void);
static void bit_0_e(void);
static void bit_0_h(void);
static void bit_0_l(void);

static void bit_1_xhl(void);
static void bit_1_xix(void);
static void bit_1_xiy(void);
static void bit_1_a(void);
static void bit_1_b(void);
static void bit_1_c(void);
static void bit_1_d(void);
static void bit_1_e(void);
static void bit_1_h(void);
static void bit_1_l(void);

static void bit_2_xhl(void);
static void bit_2_xix(void);
static void bit_2_xiy(void);
static void bit_2_a(void);
static void bit_2_b(void);
static void bit_2_c(void);
static void bit_2_d(void);
static void bit_2_e(void);
static void bit_2_h(void);
static void bit_2_l(void);

static void bit_3_xhl(void);
static void bit_3_xix(void);
static void bit_3_xiy(void);
static void bit_3_a(void);
static void bit_3_b(void);
static void bit_3_c(void);
static void bit_3_d(void);
static void bit_3_e(void);
static void bit_3_h(void);
static void bit_3_l(void);

static void bit_4_xhl(void);
static void bit_4_xix(void);
static void bit_4_xiy(void);
static void bit_4_a(void);
static void bit_4_b(void);
static void bit_4_c(void);
static void bit_4_d(void);
static void bit_4_e(void);
static void bit_4_h(void);
static void bit_4_l(void);

static void bit_5_xhl(void);
static void bit_5_xix(void);
static void bit_5_xiy(void);
static void bit_5_a(void);
static void bit_5_b(void);
static void bit_5_c(void);
static void bit_5_d(void);
static void bit_5_e(void);
static void bit_5_h(void);
static void bit_5_l(void);

static void bit_6_xhl(void);
static void bit_6_xix(void);
static void bit_6_xiy(void);
static void bit_6_a(void);
static void bit_6_b(void);
static void bit_6_c(void);
static void bit_6_d(void);
static void bit_6_e(void);
static void bit_6_h(void);
static void bit_6_l(void);

static void bit_7_xhl(void);
static void bit_7_xix(void);
static void bit_7_xiy(void);
static void bit_7_a(void);
static void bit_7_b(void);
static void bit_7_c(void);
static void bit_7_d(void);
static void bit_7_e(void);
static void bit_7_h(void);
static void bit_7_l(void);

static void call_c(void);
static void call_m(void);
static void call_nc(void);
static void call_nz(void);
static void call_p(void);
static void call_pe(void);
static void call_po(void);
static void call_z(void);
static void call(void);

static void ccf(void);

static void cp_xhl(void);
static void cp_xix(void);
static void cp_xiy(void);
static void cp_a(void);
static void cp_b(void);
static void cp_c(void);
static void cp_d(void);
static void cp_e(void);
static void cp_h(void);
static void cp_l(void);
static void cp_ixh(void);
static void cp_ixl(void);
static void cp_iyh(void);
static void cp_iyl(void);
static void cp_byte(void);

static void cpd(void);
static void cpdr(void);
static void cpi(void);
static void cpir(void);
static void cpl(void);

static void daa(void);

static void dec_xhl(void);
static void dec_xix(void);
static void dec_xiy(void);
static void dec_a(void);
static void dec_b(void);
static void dec_c(void);
static void dec_d(void);
static void dec_e(void);
static void dec_h(void);
static void dec_l(void);
static void dec_ixh(void);
static void dec_ixl(void);
static void dec_iyh(void);
static void dec_iyl(void);

static void dec_bc(void);
static void dec_de(void);
static void dec_hl(void);
static void dec_ix(void);
static void dec_iy(void);
static void dec_sp(void);

static void di(void);

static void djnz(void);

static void ei(void);

static void ex_xsp_hl(void);
static void ex_xsp_ix(void);
static void ex_xsp_iy(void);
static void ex_af_af(void);
static void ex_de_hl(void);
static void exx(void);

static void halt(void);

static void im_0(void);
static void im_1(void);
static void im_2(void);

static void in_a_c(void);
static void in_b_c(void);
static void in_c_c(void);
static void in_d_c(void);
static void in_e_c(void);
static void in_h_c(void);
static void in_l_c(void);
static void in_0_c(void);

static void in_a_byte(void);

static void inc_xhl(void);
static void inc_xix(void);
static void inc_xiy(void);
static void inc_a(void);
static void inc_b(void);
static void inc_c(void);
static void inc_d(void);
static void inc_e(void);
static void inc_h(void);
static void inc_l(void);
static void inc_ixh(void);
static void inc_ixl(void);
static void inc_iyh(void);
static void inc_iyl(void);

static void inc_bc(void);
static void inc_de(void);
static void inc_hl(void);
static void inc_ix(void);
static void inc_iy(void);
static void inc_sp(void);

static void ind(void);

static void indr(void);
static void ini(void);

static void inir(void);

static void jp(void);
static void jp_hl(void);
static void jp_ix(void);
static void jp_iy(void);
static void jp_c(void);
static void jp_m(void);
static void jp_nc(void);
static void jp_nz(void);
static void jp_p(void);
static void jp_pe(void);
static void jp_po(void);
static void jp_z(void);

static void jr(void);
static void jr_c(void);
static void jr_nc(void);
static void jr_nz(void);
static void jr_z(void);

static void ld_xbc_a(void);
static void ld_xde_a(void);
static void ld_xhl_a(void);
static void ld_xhl_b(void);
static void ld_xhl_c(void);
static void ld_xhl_d(void);
static void ld_xhl_e(void);
static void ld_xhl_h(void);
static void ld_xhl_l(void);
static void ld_xhl_byte(void);
static void ld_xix_a(void);
static void ld_xix_b(void);
static void ld_xix_c(void);
static void ld_xix_d(void);
static void ld_xix_e(void);
static void ld_xix_h(void);
static void ld_xix_l(void);
static void ld_xix_byte(void);
static void ld_xiy_a(void);
static void ld_xiy_b(void);
static void ld_xiy_c(void);
static void ld_xiy_d(void);
static void ld_xiy_e(void);
static void ld_xiy_h(void);
static void ld_xiy_l(void);
static void ld_xiy_byte(void);
static void ld_xbyte_a(void);
static void ld_xword_bc(void);
static void ld_xword_de(void);
static void ld_xword_hl(void);
static void ld_xword_ix(void);
static void ld_xword_iy(void);
static void ld_xword_sp(void);
static void ld_a_xbc(void);
static void ld_a_xde(void);
static void ld_a_xhl(void);
static void ld_a_xix(void);
static void ld_a_xiy(void);
static void ld_a_xbyte(void);

static void ld_a_byte(void);
static void ld_b_byte(void);
static void ld_c_byte(void);
static void ld_d_byte(void);
static void ld_e_byte(void);
static void ld_h_byte(void);
static void ld_l_byte(void);
static void ld_ixh_byte(void);
static void ld_ixl_byte(void);
static void ld_iyh_byte(void);
static void ld_iyl_byte(void);

static void ld_b_xhl(void);
static void ld_c_xhl(void);
static void ld_d_xhl(void);
static void ld_e_xhl(void);
static void ld_h_xhl(void);
static void ld_l_xhl(void);
static void ld_b_xix(void);
static void ld_c_xix(void);
static void ld_d_xix(void);
static void ld_e_xix(void);
static void ld_h_xix(void);
static void ld_l_xix(void);
static void ld_b_xiy(void);
static void ld_c_xiy(void);
static void ld_d_xiy(void);
static void ld_e_xiy(void);
static void ld_h_xiy(void);
static void ld_l_xiy(void);
static void ld_a_a(void);
static void ld_a_b(void);
static void ld_a_c(void);
static void ld_a_d(void);
static void ld_a_e(void);
static void ld_a_h(void);
static void ld_a_l(void);
static void ld_a_ixh(void);
static void ld_a_ixl(void);
static void ld_a_iyh(void);
static void ld_a_iyl(void);
static void ld_b_b(void);
static void ld_b_a(void);
static void ld_b_c(void);
static void ld_b_d(void);
static void ld_b_e(void);
static void ld_b_h(void);
static void ld_b_l(void);
static void ld_b_ixh(void);
static void ld_b_ixl(void);
static void ld_b_iyh(void);
static void ld_b_iyl(void);
static void ld_c_c(void);
static void ld_c_a(void);
static void ld_c_b(void);
static void ld_c_d(void);
static void ld_c_e(void);
static void ld_c_h(void);
static void ld_c_l(void);
static void ld_c_ixh(void);
static void ld_c_ixl(void);
static void ld_c_iyh(void);
static void ld_c_iyl(void);
static void ld_d_d(void);
static void ld_d_a(void);
static void ld_d_c(void);
static void ld_d_b(void);
static void ld_d_e(void);
static void ld_d_h(void);
static void ld_d_l(void);
static void ld_d_ixh(void);
static void ld_d_ixl(void);
static void ld_d_iyh(void);
static void ld_d_iyl(void);
static void ld_e_e(void);
static void ld_e_a(void);
static void ld_e_c(void);
static void ld_e_b(void);
static void ld_e_d(void);
static void ld_e_h(void);
static void ld_e_l(void);
static void ld_e_ixh(void);
static void ld_e_ixl(void);
static void ld_e_iyh(void);
static void ld_e_iyl(void);
static void ld_h_h(void);
static void ld_h_a(void);
static void ld_h_c(void);
static void ld_h_b(void);
static void ld_h_e(void);
static void ld_h_d(void);
static void ld_h_l(void);
static void ld_l_l(void);
static void ld_l_a(void);
static void ld_l_c(void);
static void ld_l_b(void);
static void ld_l_e(void);
static void ld_l_d(void);
static void ld_l_h(void);
static void ld_ixh_a(void);
static void ld_ixh_b(void);
static void ld_ixh_c(void);
static void ld_ixh_d(void);
static void ld_ixh_e(void);
static void ld_ixh_ixh(void);
static void ld_ixh_ixl(void);
static void ld_ixl_a(void);
static void ld_ixl_b(void);
static void ld_ixl_c(void);
static void ld_ixl_d(void);
static void ld_ixl_e(void);
static void ld_ixl_ixh(void);
static void ld_ixl_ixl(void);
static void ld_iyh_a(void);
static void ld_iyh_b(void);
static void ld_iyh_c(void);
static void ld_iyh_d(void);
static void ld_iyh_e(void);
static void ld_iyh_iyh(void);
static void ld_iyh_iyl(void);
static void ld_iyl_a(void);
static void ld_iyl_b(void);
static void ld_iyl_c(void);
static void ld_iyl_d(void);
static void ld_iyl_e(void);
static void ld_iyl_iyh(void);
static void ld_iyl_iyl(void);
static void ld_bc_xword(void);
static void ld_bc_word(void);
static void ld_de_xword(void);
static void ld_de_word(void);
static void ld_hl_xword(void);
static void ld_hl_word(void);
static void ld_ix_xword(void);
static void ld_ix_word(void);
static void ld_iy_xword(void);
static void ld_iy_word(void);
static void ld_sp_xword(void);
static void ld_sp_word(void);
static void ld_sp_hl(void);
static void ld_sp_ix(void);
static void ld_sp_iy(void);
static void ld_a_i(void);
static void ld_i_a(void);
static void ld_a_r(void);
static void ld_r_a(void);

static void ldd(void);
static void lddr(void);
static void ldi(void);
static void ldir(void);
static void neg(void);

static void nop(void);

static void or_xhl(void);
static void or_xix(void);
static void or_xiy(void);
static void or_a(void);
static void or_b(void);
static void or_c(void);
static void or_d(void);
static void or_e(void);
static void or_h(void);
static void or_l(void);
static void or_ixh(void);
static void or_ixl(void);
static void or_iyh(void);
static void or_iyl(void);
static void or_byte(void);

static void outd(void);
static void otdr(void);
static void outi(void);
static void otir(void);

static void out_c_a(void);
static void out_c_b(void);
static void out_c_c(void);
static void out_c_d(void);
static void out_c_e(void);
static void out_c_h(void);
static void out_c_l(void);
static void out_c_0(void);
static void out_byte_a(void);

static void pop_af(void);
static void pop_bc(void);
static void pop_de(void);
static void pop_hl(void);
static void pop_ix(void);
static void pop_iy(void);

static void push_af(void);
static void push_bc(void);
static void push_de(void);
static void push_hl(void);
static void push_ix(void);
static void push_iy(void);

static void res_0_xhl(void);
static void res_0_xix(void);
static void res_0_xiy(void);
static void res_0_a(void);
static void res_0_b(void);
static void res_0_c(void);
static void res_0_d(void);
static void res_0_e(void);
static void res_0_h(void);
static void res_0_l(void);

static void res_1_xhl(void);
static void res_1_xix(void);
static void res_1_xiy(void);
static void res_1_a(void);
static void res_1_b(void);
static void res_1_c(void);
static void res_1_d(void);
static void res_1_e(void);
static void res_1_h(void);
static void res_1_l(void);

static void res_2_xhl(void);
static void res_2_xix(void);
static void res_2_xiy(void);
static void res_2_a(void);
static void res_2_b(void);
static void res_2_c(void);
static void res_2_d(void);
static void res_2_e(void);
static void res_2_h(void);
static void res_2_l(void);

static void res_3_xhl(void);
static void res_3_xix(void);
static void res_3_xiy(void);
static void res_3_a(void);
static void res_3_b(void);
static void res_3_c(void);
static void res_3_d(void);
static void res_3_e(void);
static void res_3_h(void);
static void res_3_l(void);

static void res_4_xhl(void);
static void res_4_xix(void);
static void res_4_xiy(void);
static void res_4_a(void);
static void res_4_b(void);
static void res_4_c(void);
static void res_4_d(void);
static void res_4_e(void);
static void res_4_h(void);
static void res_4_l(void);

static void res_5_xhl(void);
static void res_5_xix(void);
static void res_5_xiy(void);
static void res_5_a(void);
static void res_5_b(void);
static void res_5_c(void);
static void res_5_d(void);
static void res_5_e(void);
static void res_5_h(void);
static void res_5_l(void);

static void res_6_xhl(void);
static void res_6_xix(void);
static void res_6_xiy(void);
static void res_6_a(void);
static void res_6_b(void);
static void res_6_c(void);
static void res_6_d(void);
static void res_6_e(void);
static void res_6_h(void);
static void res_6_l(void);

static void res_7_xhl(void);
static void res_7_xix(void);
static void res_7_xiy(void);
static void res_7_a(void);
static void res_7_b(void);
static void res_7_c(void);
static void res_7_d(void);
static void res_7_e(void);
static void res_7_h(void);
static void res_7_l(void);

static void ret(void);
static void ret_c(void);
static void ret_m(void);
static void ret_nc(void);
static void ret_nz(void);
static void ret_p(void);
static void ret_pe(void);
static void ret_po(void);
static void ret_z(void);

static void reti(void);
static void retn(void);

static void rl_xhl(void);
static void rl_xix(void);
static void rl_xiy(void);
static void rl_a(void);
static void rl_b(void);
static void rl_c(void);
static void rl_d(void);
static void rl_e(void);
static void rl_h(void);
static void rl_l(void);
static void rla(void) ;

static void rlc_xhl(void);
static void rlc_xix(void);
static void rlc_xiy(void);
static void rlc_a(void);
static void rlc_b(void);
static void rlc_c(void);
static void rlc_d(void);
static void rlc_e(void);
static void rlc_h(void);
static void rlc_l(void);
static void rlca(void) ;

static void rld(void);

static void rr_xhl(void);
static void rr_xix(void);
static void rr_xiy(void);
static void rr_a(void);
static void rr_b(void);
static void rr_c(void);
static void rr_d(void);
static void rr_e(void);
static void rr_h(void);
static void rr_l(void);
static void rra(void) ;

static void rrc_xhl(void);
static void rrc_xix(void);
static void rrc_xiy(void);
static void rrc_a(void);
static void rrc_b(void);
static void rrc_c(void);
static void rrc_d(void);
static void rrc_e(void);
static void rrc_h(void);
static void rrc_l(void);
static void rrca(void) ;

static void rrd(void);

static void rst_00(void);
static void rst_08(void);
static void rst_10(void);
static void rst_18(void);
static void rst_20(void);
static void rst_28(void);
static void rst_30(void);
static void rst_38(void);

static void sbc_a_byte(void);
static void sbc_a_xhl(void);
static void sbc_a_xix(void);
static void sbc_a_xiy(void);
static void sbc_a_a(void);
static void sbc_a_b(void);
static void sbc_a_c(void);
static void sbc_a_d(void);
static void sbc_a_e(void);
static void sbc_a_h(void);
static void sbc_a_l(void);
static void sbc_a_ixh(void);
static void sbc_a_ixl(void);
static void sbc_a_iyh(void);
static void sbc_a_iyl(void);

static void sbc_hl_bc(void);
static void sbc_hl_de(void);
static void sbc_hl_hl(void);
static void sbc_hl_sp(void);

static void scf(void);

static void set_0_xhl(void);
static void set_0_xix(void);
static void set_0_xiy(void);
static void set_0_a(void);
static void set_0_b(void);
static void set_0_c(void);
static void set_0_d(void);
static void set_0_e(void);
static void set_0_h(void);
static void set_0_l(void);

static void set_1_xhl(void);
static void set_1_xix(void);
static void set_1_xiy(void);
static void set_1_a(void);
static void set_1_b(void);
static void set_1_c(void);
static void set_1_d(void);
static void set_1_e(void);
static void set_1_h(void);
static void set_1_l(void);

static void set_2_xhl(void);
static void set_2_xix(void);
static void set_2_xiy(void);
static void set_2_a(void);
static void set_2_b(void);
static void set_2_c(void);
static void set_2_d(void);
static void set_2_e(void);
static void set_2_h(void);
static void set_2_l(void);

static void set_3_xhl(void);
static void set_3_xix(void);
static void set_3_xiy(void);
static void set_3_a(void);
static void set_3_b(void);
static void set_3_c(void);
static void set_3_d(void);
static void set_3_e(void);
static void set_3_h(void);
static void set_3_l(void);

static void set_4_xhl(void);
static void set_4_xix(void);
static void set_4_xiy(void);
static void set_4_a(void);
static void set_4_b(void);
static void set_4_c(void);
static void set_4_d(void);
static void set_4_e(void);
static void set_4_h(void);
static void set_4_l(void);

static void set_5_xhl(void);
static void set_5_xix(void);
static void set_5_xiy(void);
static void set_5_a(void);
static void set_5_b(void);
static void set_5_c(void);
static void set_5_d(void);
static void set_5_e(void);
static void set_5_h(void);
static void set_5_l(void);

static void set_6_xhl(void);
static void set_6_xix(void);
static void set_6_xiy(void);
static void set_6_a(void);
static void set_6_b(void);
static void set_6_c(void);
static void set_6_d(void);
static void set_6_e(void);
static void set_6_h(void);
static void set_6_l(void);

static void set_7_xhl(void);
static void set_7_xix(void);
static void set_7_xiy(void);
static void set_7_a(void);
static void set_7_b(void);
static void set_7_c(void);
static void set_7_d(void);
static void set_7_e(void);
static void set_7_h(void);
static void set_7_l(void);

static void sla_xhl(void);
static void sla_xix(void);
static void sla_xiy(void);
static void sla_a(void);
static void sla_b(void);
static void sla_c(void);
static void sla_d(void);
static void sla_e(void);
static void sla_h(void);
static void sla_l(void);

static void sll_xhl(void);
static void sll_xix(void);
static void sll_xiy(void);
static void sll_a(void);
static void sll_b(void);
static void sll_c(void);
static void sll_d(void);
static void sll_e(void);
static void sll_h(void);
static void sll_l(void);

static void sra_xhl(void);
static void sra_xix(void);
static void sra_xiy(void);
static void sra_a(void);
static void sra_b(void);
static void sra_c(void);
static void sra_d(void);
static void sra_e(void);
static void sra_h(void);
static void sra_l(void);

static void srl_xhl(void);
static void srl_xix(void);
static void srl_xiy(void);
static void srl_a(void);
static void srl_b(void);
static void srl_c(void);
static void srl_d(void);
static void srl_e(void);
static void srl_h(void);
static void srl_l(void);

static void sub_xhl(void);
static void sub_xix(void);
static void sub_xiy(void);
static void sub_a(void);
static void sub_b(void);
static void sub_c(void);
static void sub_d(void);
static void sub_e(void);
static void sub_h(void);
static void sub_l(void);
static void sub_ixh(void);
static void sub_ixl(void);
static void sub_iyh(void);
static void sub_iyl(void);
static void sub_byte(void);

static void xor_xhl(void);
static void xor_xix(void);
static void xor_xiy(void);
static void xor_a(void);
static void xor_b(void);
static void xor_c(void);
static void xor_d(void);
static void xor_e(void);
static void xor_h(void);
static void xor_l(void);
static void xor_ixh(void);
static void xor_ixl(void);
static void xor_iyh(void);
static void xor_iyl(void);
static void xor_byte(void);

static void no_op(void);
static void no_op_xx(void);
static void patch(void);
static void dd_cb(void);
static void fd_cb(void);
static void cb(void);
static void dd(void);
static void ed(void);
static void fd(void);

#endif
