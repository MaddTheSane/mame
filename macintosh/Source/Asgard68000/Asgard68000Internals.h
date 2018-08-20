//###################################################################################################
//
//
//		Asgard68000Internals.h
//		This file contains internal data and structures that were too cumbersome to include
//		in the main Asgard68000.h.
//
//		See Asgard68000Core.c for information about the Asgard68000 system.
//
//
//###################################################################################################

#ifndef __ASGARD68000INTERNALS__
#define __ASGARD68000INTERNALS__


#if A68000_COREDEBUG
#include "Asgard68000Debugger.h"
#endif


//###################################################################################################
//
// 	Here is how the MC68000_SR internal register is mapped out
//
//###################################################################################################

enum
{
	kSRFlagsSTOP			= 0x00020000,		// STOP				= bit  14
	kSRFlagsDirty			= 0x00010000,		// DIRTY			= bit  15
	kSRFlagsSR				= 0x0000ffff		// SR				= bits 16-31
};


//###################################################################################################
//
// 	These are the nonvolatile registers we use during execution
//
//###################################################################################################

#define	rPC					r31				/* holds the PC register */
#define	rSRFlags			r30				/* holds the SR register and flags */
#define	rLongRegs			r29				/* holds a pointer to the register array */
#define	rWordRegs			r28				/* holds a pointer to the register array + 2, for words */
#define	rByteRegs			r27				/* holds a pointer to the register array + 3, for bytes */
#define	rAddressRegs		r26				/* holds a pointer to the register array + 3, for bytes */
#define	rICount				r25				/* holds the current number of cycles left to execute */
#define	rICountPtr	 		r24				/* holds a pointer to the global that stores rICount */
#define	rCycleCount			r23				/* total cycle count for this instruction */
#define	rOpcodeROM			r22				/* holds the current base of the ROM space */
#define	rOpcodeROMPtr 		r21				/* holds a pointer to the global that stores rOpcodeROM */
#define	rOpcodeTable		r20				/* holds a pointer to the opcode lookup table */
#define	rEA					r19				/* holds the current effective address */
#define	rTempSave			r18				/* temporary holding register for a bunch of stuff */
#define	rTempSave2			r17				/* temporary holding register for the PowerPC LR */
#define	rRX					r16				/* Rx = SRC */
#define	rRY					r15				/* Ry = SRC */
#define	rLastNonVolatile	r15				/* the last non-volatible register we use */


//###################################################################################################
//
// 	Callback handling for changing the PC on jumps
//
//###################################################################################################

#ifdef A68000_UPDATEBANK
	#define UPDATE_BANK_SHORT
	#define UPDATE_BANK \
		GET_PC(r3);\
		bl		A68000_UPDATEBANK;\
		lwz		rOpcodeROM,0(rOpcodeROMPtr);
#else
	#define UPDATE_BANK_SHORT
	#define UPDATE_BANK
#endif


//###################################################################################################
//
// 	Macros to transparently support internal memory debugging
//
//###################################################################################################

#if A68000_COREDEBUG
	#define READBYTE		Asgard68000DebugReadByte
	#define READWORD		Asgard68000DebugReadWord
	#define READLONG		Asgard68000DebugReadLong
	#define WRITEBYTE		Asgard68000DebugWriteByte
	#define WRITEWORD		Asgard68000DebugWriteWord
	#define WRITELONG		Asgard68000DebugWriteLong
	#define READBYTEREL		Asgard68000DebugReadByte
	#define READWORDREL		Asgard68000DebugReadWord
	#define READLONGREL		Asgard68000DebugReadLong
#else
	#define READBYTE		A68000_READBYTE
	#define READWORD		A68000_READWORD
	#define READLONG		A68000_READLONG
	#define WRITEBYTE		A68000_WRITEBYTE
	#define WRITEWORD		A68000_WRITEWORD
	#define WRITELONG		A68000_WRITELONG
	#define READBYTEREL		A68000_READBYTE_REL
	#define READWORDREL		A68000_READWORD_REL
	#define READLONGREL		A68000_READLONG_REL
#endif


//###################################################################################################
//
// 	These macros are used for addressing
//
//###################################################################################################

#if (A68000_ADDRESSBITS == 32)
#define ADDRESS_MASK	0,31
#else
#define ADDRESS_MASK	8,31
#endif


//###################################################################################################
//
// 	These macros are used to extract the given MC68000 register into the specified PPC register
//
//###################################################################################################

#define GET_PC(r)		rlwinm	r,rPC,0,ADDRESS_MASK;
#define GET_SR(r)		rlwinm	r,rSRFlags,0,16,31;


//###################################################################################################
//
// 	These macros are used to set the given MC68000 register to the value in the specified PPC register
//
//###################################################################################################

#define SET_PC(r)		mr		rPC,r;
#define SET_SR(r)		rlwimi	rSRFlags,r,0,16,31;


//###################################################################################################
//
// 	These macros are used to save/load the current MC68000 global state during execution
//
//###################################################################################################

#define SAVE_SR		/*	stw		rSRFlags,sSRFlags(rtoc);*/
#define LOAD_SR		/*	lwz		rSRFlags,sSRFlags(rtoc);*/

//#define SAVE_PC			stw		rPC,sPC(rtoc);
#define SAVE_PC			_asm_set_global(rPC,sPC);
//#define LOAD_PC		/*	lwz		rPC,sPC(rtoc);*/
#define LOAD_PC		/*	_asm_get_global(rPC,sPC);*/

#define SAVE_ICOUNT		stw		rICount,0(rICountPtr);
#define LOAD_ICOUNT		lwz		rICount,0(rICountPtr);


//###################################################################################################
//
// 	Count cycles for this operation; should be done before registers are saved
//
//###################################################################################################

#if A68000_CHIPTIMING == 68000
#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,a
#elif A68000_CHIPTIMING == 68010
#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,b
#elif A68000_CHIPTIMING == 68020
//#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,c*2		/* best case might be too much! */
#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,c		/* best case might be too much! */
#else
#error Must define A68000_CHIPTIMING to be 68000, 68010, or 68020!
#endif


//###################################################################################################
//
// 	Read an opcode from ROM to 'r' and increment the PC
//
//###################################################################################################

#define READ_OPCODE(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addi	rPC,rPC,2;				/* increment & wrap the PC */\
	lhzx	r,rOpcodeROM,r;			/* r = immediate word value */

#define READ_OPCODE_ARG(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addi	rPC,rPC,2;				/* increment & wrap the PC */\
	lhzx	r,rOpcodeROM,r;			/* r = immediate word value */

#define READ_OPCODE_ARG_BYTE(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addi	r,r,1;					/* r += 1 */\
	addi	rPC,rPC,2;				/* increment & wrap the PC */\
	lbzx	r,rOpcodeROM,r;			/* r = immediate word value */

#define READ_OPCODE_ARG_EXT(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addi	rPC,rPC,2;				/* increment & wrap the PC */\
	lhax	r,rOpcodeROM,r;			/* r = sign-extended immediate word value */

#define READ_OPCODE_ARG_LONG(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addi	rPC,rPC,4;				/* increment & wrap the PC */\
	lwzx	r,rOpcodeROM,r;			/* r = immediate word value */


//###################################################################################################
//
// 	These macros are used for flag manipulation
//
//###################################################################################################

#define CLR_C 				rlwinm	rSRFlags,rSRFlags,0,0,30;		/* C = 0 */
#define CLR_V 				rlwinm	rSRFlags,rSRFlags,0,31,29;		/* V = 0 */
#define CLR_Z 				rlwinm	rSRFlags,rSRFlags,0,30,28;		/* Z = 0 */
#define CLR_N 				rlwinm	rSRFlags,rSRFlags,0,29,27;		/* N = 0 */
#define CLR_VC				rlwinm	rSRFlags,rSRFlags,0,0,29;		/* C = V = 0 */

#define SET_N				ori		rSRFlags,rSRFlags,k68000FlagN;	/* N = 1 */
#define SET_Z_1				ori		rSRFlags,rSRFlags,k68000FlagZ;	/* Z = 1 */
#define SET_C				ori		rSRFlags,rSRFlags,k68000FlagC;	/* C = 1 */

#define INVERT_C			xori	rSRFlags,rSRFlags,k68000FlagC;

#define GET_X(r)			rlwinm	r,rSRFlags,28,31,31;			/* r = X */

#define GET_A7(r)			lwz		r,7*4(rAddressRegs)
#define SET_A7(r)			stw		r,7*4(rAddressRegs)

#define SET_N_BYTE(r) 		rlwimi	rSRFlags,r,28,28,28;			/* N = (r & 0x80) */
#define SET_N_WORD(r) 		rlwimi	rSRFlags,r,20,28,28;			/* N = (r & 0x8000) */
#define SET_N_LONG(r) 		rlwimi	rSRFlags,r,4,28,28;				/* N = (r & 0x80000000) */

#define SET_C_BYTE(r) 		rlwimi	rSRFlags,r,24,31,31;			/* C = (r & 0x100) */
#define SET_C_WORD(r) 		rlwimi	rSRFlags,r,16,31,31;			/* C = (r & 0x10000) */
#define SET_C_LONG(r) 		addze	rSRFlags,rSRFlags				/* C = (EXT) -- must clear first */

#define SET_X_FROM_C		rlwimi	rSRFlags,rSRFlags,4,27,27;		/* X = C */

#define SET_Z(r) 			rlwimi	rSRFlags,r,29,29,29;			/* Z = (r4 == 0) */

#define SET_V_BYTE(r)		rlwimi	rSRFlags,r,26,30,30;			/* V = (r & 0x80) */
#define SET_V_WORD(r)		rlwimi	rSRFlags,r,18,30,30;			/* V = (r & 0x8000) */
#define SET_V_LONG(r)		rlwimi	rSRFlags,r,2,30,30;				/* V = (r & 0x80000000) */


//###################################################################################################
//
// 	These macros are used for byte/word truncation
//
//###################################################################################################

#define TRUNC_BYTE(r)		rlwinm	r,r,0,24,31;
#define TRUNC_BYTE_TO(d,r)	rlwinm	d,r,0,24,31;
#define TRUNC_WORD(r)		rlwinm	r,r,0,16,31;
#define TRUNC_WORD_TO(d,r)	rlwinm	d,r,0,16,31;


//###################################################################################################
//
// 	These macros are used for to load/store register values
//
//###################################################################################################

#define READ_B_REG(r,v)		lbzx	r,rByteRegs,v;
#define READ_W_REG(r,v)		lhzx	r,rWordRegs,v;
#define READ_W_REG_EXT(r,v) lhax	r,rWordRegs,v;
#define READ_L_REG(r,v)		lwzx	r,rLongRegs,v;
#define READ_L_AREG(r,v)	lwzx	r,rAddressRegs,v;

#define WRITE_B_REG(r,v)	stbx	r,rByteRegs,v;
#define WRITE_W_REG(r,v)	sthx	r,rWordRegs,v;
#define WRITE_L_REG(r,v)	stwx	r,rLongRegs,v;
#define WRITE_L_AREG(r,v)	stwx	r,rAddressRegs,v;

#define REG_OFFSET_LO_4(r)	rlwinm	r,r11,2,26,29;		/* get the 4-bit register in the low part */


//###################################################################################################
//
// 	These macros are used for to load/store data to memory
//
//###################################################################################################

#define READ_B_AT(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		READBYTE;

#define READ_W_AT(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		READWORD;

#define READ_L_AT(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		READLONG;

#define READ_W_AT_REL(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		READWORDREL;

#define READ_L_AT_REL(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		READLONGREL;

#define WRITE_B_AT(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		WRITEBYTE;

#define WRITE_W_AT(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		WRITEWORD;

#define WRITE_L_AT(r) \
	rlwinm	r3,r,0,ADDRESS_MASK;\
	bl		WRITELONG;


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

// externally defined functions
extern UINT8		A68000_READBYTE(offs_t address);
extern UINT16		A68000_READWORD(offs_t address);
extern UINT32		A68000_READLONG(offs_t address);
extern void 		A68000_WRITEBYTE(offs_t address, UINT8 value);
extern void 		A68000_WRITEWORD(offs_t address, UINT16 value);
extern void 		A68000_WRITELONG(offs_t address, UINT32 value);
extern UINT8		A68000_READBYTE_REL(offs_t address);
extern UINT16		A68000_READWORD_REL(offs_t address);
extern UINT32		A68000_READLONG_REL(offs_t address);

// prototype for the debugger hook
#ifdef A68000_DEBUGHOOK
extern void 		A68000_DEBUGHOOK(void);
#endif

// prototype for the bank switcher
#ifdef A68000_UPDATEBANK
extern void 		A68000_UPDATEBANK(offs_t newpc);
#endif

#endif
