//###################################################################################################
//
//
//		Asgard6809Internals.h
//		This file contains internal data and structures that were too cumbersome to include
//		in the main Asgard6809.h.
//
//		See Asgard6809Core.c for information about the Asgard6809 system.
//
//
//###################################################################################################

#ifndef __ASGARD6809INTERNALS__
#define __ASGARD6809INTERNALS__


#if A6809_COREDEBUG
#include "Asgard6809Debugger.h"
#endif


//###################################################################################################
//
// 	Here is how the DPFlags internal register is mapped out
//
//###################################################################################################

enum
{
	kDPFlagsDirty			= 0x00010000,		// DIRTY = bit  15
	kDPFlagsDP				= 0x0000ff00,		// DP    = bits 16-23
	kDPFlagsCC				= 0x000000ff		// CC    = bits 24-31
};


//###################################################################################################
//
// 	These are the nonvolatile registers we use during execution
//
//###################################################################################################

#define	rPCAB				r31				/* holds the PC, A & B registers */
#define	rDPFlags			r30				/* holds the DP & CC register plus pending interrupt flags */
#define	rSU					r29				/* holds the S & U registers */
#define	rXY					r28				/* holds the X & Y registers */
#define	rICount				r27				/* holds the current number of cycles left to execute */
#define	rICountPtr		 	r26				/* holds a pointer to the global that stores rICount */
#define	rCycleCount			r25				/* total cycle count for this instruction */
#define	rOpcodeROM			r24				/* holds the current base of the ROM space */
#define	rOpcodeROMPtr		r23				/* holds a pointer to the global that stores rOpcodeROM */
#define	rArgumentROM		r22				/* holds the current base of the argument ROM space */
#define	rArgumentROMPtr		r21				/* holds a pointer to the global that stores rArgumentROM */
#define	rOpcodeTable		r20				/* holds a pointer to the opcode lookup table */
#define	rEA					r19				/* holds the current effective address */
#define	rTempSave			r18				/* temporary holding register for a bunch of stuff */
#define	rTempSave2			r17				/* temporary nonvolatile register */
#define	rLinkSave			r16				/* temporary holding register for the PowerPC LR */
#define	rLinkSave2			r15				/* temporary holding register for the PowerPC LR */
#define	rLastNonVolatile	r15				/* the last non-volatible register we use */


//###################################################################################################
//
// 	Macros to transparently support bank updating
//
//###################################################################################################

#ifdef A6809_UPDATEBANK
	#define UPDATE_BANK		bl		updateBank;
#else
	#define UPDATE_BANK
#endif


//###################################################################################################
//
// 	Macros to transparently support internal memory debugging
//
//###################################################################################################

#if A6809_COREDEBUG
	#define READMEM			Asgard6809DebugRead
	#define WRITEMEM		Asgard6809DebugWrite
#else
	#define READMEM			A6809_READMEM
	#define WRITEMEM		A6809_WRITEMEM
#endif


//###################################################################################################
//
// 	These macros are used to extract the given 6809 register into the specified PPC register
//
//###################################################################################################

#define GET_X(r)			rlwinm	r,rXY,16,16,31;
#define GET_Y(r)			rlwinm	r,rXY,0,16,31;
#define GET_S(r)			rlwinm	r,rSU,16,16,31;
#define GET_U(r)			rlwinm	r,rSU,0,16,31;
#define GET_PC(r)			rlwinm	r,rPCAB,16,16,31;
#define GET_A(r)			rlwinm	r,rPCAB,24,24,31;
#define GET_B(r)			rlwinm	r,rPCAB,0,24,31;
#define GET_D(r)			rlwinm	r,rPCAB,0,16,31;
#define GET_DP(r)			rlwinm	r,rDPFlags,24,24,31;
#define GET_CC(r)			rlwinm	r,rDPFlags,0,24,31;


//###################################################################################################
//
// 	These macros are used to set the given 6809 register to the value in the specified PPC register
//
//###################################################################################################

#define SET_X(r)			rlwimi	rXY,r,16,0,15;
#define SET_Y(r)			rlwimi	rXY,r,0,16,31;
#define SET_S(r)			rlwimi	rSU,r,16,0,15;
#define SET_U(r)			rlwimi	rSU,r,0,16,31;
#define SET_PC(r)			rlwimi	rPCAB,r,16,0,15;
#define SET_A(r)			rlwimi	rPCAB,r,8,16,23;
#define SET_B(r)			rlwimi	rPCAB,r,0,24,31;
#define SET_D(r)			rlwimi	rPCAB,r,0,16,31;
#define SET_DP(r)			rlwimi	rDPFlags,r,8,16,23;
#define SET_CC(r)			rlwimi	rDPFlags,r,0,24,31;


//###################################################################################################
//
// 	These macros are used to save/load the current 6809 global state during execution
//
//###################################################################################################

// DP & control flags are always saved/loaded
#define SAVE_DPFLAGS		_asm_set_global(rDPFlags,sDPFlags);
#define LOAD_DPFLAGS		_asm_get_global(rDPFlags,sDPFlags);

// the current execution cycle count is always saved/loaded
#define SAVE_ICOUNT			stw		rICount,0(rICountPtr);
#define LOAD_ICOUNT			lwz		rICount,0(rICountPtr);

// the OPCODEROM pointer is loaded only if we do bank switching
#ifdef A6809_UPDATEBANK
	#define LOAD_ROM		lwz		rOpcodeROM,0(rOpcodeROMPtr);\
							lwz		rArgumentROM,0(rArgumentROMPtr);
#else
	#define LOAD_ROM
#endif

// the registers are optionally saved/loaded
#if A6809_VOLATILEREGS
	#define SAVE_PCAB		_asm_set_global(rPCAB,sPCAB);
	#define LOAD_PCAB		_asm_get_global(rPCAB,sPCAB);
	#define SAVE_XY			_asm_set_global(rXY,sXY);
	#define LOAD_XY			_asm_get_global(rXY,sXY);
	#define SAVE_SU			_asm_set_global(rSU,sSU);
	#define LOAD_SU			_asm_get_global(rSU,sSU);
#else
	#define SAVE_PCAB
	#define LOAD_PCAB
	#define SAVE_XY
	#define LOAD_XY
	#define SAVE_SU
	#define LOAD_SU
#endif


//###################################################################################################
//
// 	Count cycles for this operation; should be done before registers are saved
//
//###################################################################################################

#if (A6809_CHIP == 6809)
	#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,a
#elif (A6809_CHIP == 6309)
	#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,b
#elif (A6809_CHIP == 5000)
	#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,c
#else
	#error You must define A6809_CHIP to be 6809, 6309, or 5000!
#endif


//###################################################################################################
//
// 	Read an opcode from ROM to 'r' and increment the PC
//
//###################################################################################################

#define READ_OPCODE(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAB,rPCAB,1;			/* increment & wrap the PC */\
	lbzx	r,rOpcodeROM,r;			/* r = immediate byte value */

#define READ_OPCODE_ARG(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAB,rPCAB,1;			/* increment & wrap the PC */\
	lbzx	r,rArgumentROM,r;		/* r = immediate byte value */


//###################################################################################################
//
// 	Read an opcode word from ROM to 'r' and increment the PC
//
//###################################################################################################

#define READ_OPCODE_ARG_WORD(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAB,rPCAB,2;			/* increment & wrap the PC */\
	lhzx	r,rArgumentROM,r;		/* r = immediate byte value */


//###################################################################################################
//
// 	Read a byte from memory address 'r' into r3, saving the registers as well
//
//###################################################################################################

#define READ_BYTE_SAVE(r) \
	SAVE_PCAB						/* save PC/AB */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	SAVE_DPFLAGS					/* save DP & flags */\
	bl		READMEM;				/* perform the read */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Read a word from memory address 'r' into rTempSave, saving the registers as well
//
//###################################################################################################

#define READ_WORD_SAVE(r) \
	SAVE_PCAB						/* save PC/AB */\
	rlwinm	r3,r,0,16,31;			/* keep it in range */\
	SAVE_DPFLAGS					/* save DP & flags */\
	bl		READMEM;				/* perform the read */\
	rlwinm	rTempSave,r3,8,16,23;	/* save the result temporarily */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r3,r3,0,16,31;			/* get the address in r3 */\
	bl		READMEM;				/* perform the read */\
	rlwimi	rTempSave,r3,0,24,31;	/* combine the two bytes */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Write a word to memory address 'r' from the low 16 bits of register 'd', 
//	saving the registers as well
//
//###################################################################################################

#define WRITE_WORD_LO_SAVE(r,d) \
	SAVE_PCAB						/* save PC/AB */\
	rlwinm	r4,d,24,24,31;			/* get the high byte of data in r4 */\
	SAVE_DPFLAGS					/* save DP & flags */\
	rlwinm	r3,r,0,16,31;			/* keep it in range */\
	bl		WRITEMEM;				/* perform the write */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r4,d,0,24,31;			/* get the low byte of data in r4 */\
	rlwinm	r3,r3,0,16,31;			/* get the address in r3 */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	EA Indexed: Fetches the next byte of the opcode and calls the appropriate eaddr subfunction
//
//###################################################################################################

#define INDEXED \
	READ_OPCODE_ARG(r3)				/* r3 = next opcode byte */\
	addi	r4,rOpcodeTable,OpcodeTable.fEA; /* r4 = address of EA table */\
	rlwinm	r5,r3,2,0,31;			/* r5 = r3 << 2 */\
	lwzx	r5,r4,r5;				/* r5 = opcodes[r5] */\
	mtctr	r5;						/* ctr = r5 */\
	bctrl;							/* branch and link */
	

//###################################################################################################
//
// 	EA Direct: Fetches the next byte of the opcode and ors in the DP
//
//###################################################################################################

#define DIRECT \
	READ_OPCODE_ARG(rEA)			/* rEA = next opcode byte */\
	rlwimi	rEA,rDPFlags,0,16,23;	/* EA |= DP<<8 */


//###################################################################################################
//
// 	EA Extended: Fetches the next word of the opcode as EA
//
//###################################################################################################

#define EXTENDED \
	READ_OPCODE_ARG_WORD(rEA)		/* rEA = next opcode word */
	

//###################################################################################################
//
// 	Performs a PUSH operation on the high half of register 'reg'
//
//###################################################################################################

#define PUSH_WORD_HI_SAVE(reg) \
	SAVE_PCAB						/* save PC/AB */\
	subis	rSU,rSU,1;				/* decrement the sp */\
	SAVE_DPFLAGS					/* save DP & flags */\
	rlwinm	r4,reg,16,24,31;		/* get the low part into r4 */\
	GET_S(r3)						/* get the sp */\
	bl		WRITEMEM;				/* perform the write */\
	subis	rSU,rSU,1;				/* decrement the sp */\
	rlwinm	r4,reg,8,24,31;			/* get the high part into r4 */\
	GET_S(r3)						/* get the sp */\
	bl		WRITEMEM;				/* perform the write */\
	SAVE_SU							/* save the new sp */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Performs a PULL operation on the high half of register 'reg'
//
//###################################################################################################

#define PULL_WORD_HI_SAVE(reg) \
	SAVE_PCAB						/* save PC/AB */\
	GET_S(r3)						/* get the sp */\
	SAVE_DPFLAGS					/* save DP & flags */\
	addis	rSU,rSU,1;				/* increment the sp */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,24,0,7;			/* extract the high bits */\
	GET_S(r3)						/* get the sp */\
	addis	rSU,rSU,1;				/* increment the sp */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,16,8,15;			/* extract the low bits */\
	SAVE_SU							/* save the new sp */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Expands the bits starting at 'bit' in the CR into a register selector; local labels are 
//	generated by using 'label'.  The selected register is retrieved into 'res'
//
//###################################################################################################

#if (A6809_CHIP != 5000)

#define EXPAND_REG_GET(bit,label,res,ret)	\
	bc		12,bit+0,label##1xxx;	/* if bit 0 is set, it must be 1xxx */\
	bc		12,bit+1,label##01xx;	/* if bit 1 is set, it must be 01xx */\
	bc		12,bit+2,label##001x;	/* if bit 2 is set, it must be 001x */\
	bc		12,bit+3,label##0001;	/* if bit 3 is set, it must be 0001 */\
label##0000:\
	GET_D(res)						/* 0000: operate on D */\
	b		ret;					/* done */\
label##0001:\
	GET_X(res)						/* 0001: operate on X */\
	b		ret;					/* done */\
label##001x:\
	bc		12,bit+3,label##0011;	/* if bit 3 is set, it must be 0011 */\
label##0010:\
	GET_Y(res)						/* 0010: operate on Y */\
	b		ret;					/* done */\
label##0011:\
	GET_U(res)						/* 0011: operate on U */\
	b		ret;					/* done */\
label##01xx:\
	bc		12,bit+3,label##0101;	/* if bit 3 is set, it must be 0101 (011x are invalid!) */\
label##0100:\
	GET_S(res)						/* 0100: operate on S */\
	b		ret;					/* done */\
label##0101:\
	GET_PC(res)						/* 0101: operate on PC */\
	b		ret;					/* done */\
label##1xxx:\
	bc		12,bit+2,label##101x;	/* if bit 2 is set, it must be 101x (11xx are invalid!) */\
	bc		12,bit+3,label##1001;	/* if bit 3 is set, it must be 1001 */\
label##1000:\
	GET_A(res)						/* 1000: operate on A */\
	b		ret;					/* done */\
label##1001:\
	GET_B(res)						/* 1001: operate on B */\
	b		ret;					/* done */\
label##101x:\
	bc		12,bit+3,label##1011;	/* if bit 3 is set, it must be 1011 */\
label##1010:\
	GET_CC(res)						/* 1010: operate on CC */\
	b		ret;					/* done */\
label##1011:\
	GET_DP(res)						/* 1011: operate on DP */

#else

#define EXPAND_REG_GET(bit,label,res,ret)	\
/*	bc		12,bit+0,label##1xxx;	/* if bit 0 is set, it must be 1xxx */\
	bc		12,bit+1,label##01xx;	/* if bit 1 is set, it must be 01xx */\
	bc		12,bit+2,label##001x;	/* if bit 2 is set, it must be 001x */\
	bc		12,bit+3,label##0001;	/* if bit 3 is set, it must be 0001 */\
label##0000:\
	GET_A(res)						/* 0000: operate on A */\
	b		ret;					/* done */\
label##0001:\
	GET_B(res)						/* 0001: operate on B */\
	b		ret;					/* done */\
label##001x:\
	bc		12,bit+3,label##0011;	/* if bit 3 is set, it must be 0011 */\
label##0010:\
	GET_X(res)						/* 0010: operate on X */\
	b		ret;					/* done */\
label##0011:\
	GET_Y(res)						/* 0011: operate on Y */\
	b		ret;					/* done */\
label##01xx:\
	bc		12,bit+3,label##0101;	/* if bit 3 is set, it must be 0101 (011x are invalid!) */\
label##0100:\
	GET_S(res)						/* 0100: operate on S */\
	b		ret;					/* done */\
label##0101:\
	GET_U(res)						/* 0101: operate on U */\
	b		ret;					/* done */\
label##1xxx:\
	li		res,0xff;				/* unknown case: just use 0xff */\

#endif

//###################################################################################################
//
// 	Expands the bits starting at 'bit' in the CR into a register selector; local labels are 
//		generated by using 'label'.  The selected register is retrieved into 'res'
//
//###################################################################################################

#if (A6809_CHIP != 5000)

#define EXPAND_REG_SET(bit,label,res,ret)	\
	bc		12,bit+0,label##1xxx;	/* if bit 0 is set, it must be 1xxx */\
	bc		12,bit+1,label##01xx;	/* if bit 1 is set, it must be 01xx */\
	bc		12,bit+2,label##001x;	/* if bit 2 is set, it must be 001x */\
	bc		12,bit+3,label##0001;	/* if bit 3 is set, it must be 0001 */\
label##0000:\
	SET_D(res)						/* 0000: operate on D */\
	b		ret;					/* done */\
label##0001:\
	SET_X(res)						/* 0001: operate on X */\
	SAVE_XY							/* save the result */\
	b		ret;					/* done */\
label##001x:\
	bc		12,bit+3,label##0011;	/* if bit 3 is set, it must be 0011 */\
label##0010:\
	SET_Y(res)						/* 0010: operate on Y */\
	SAVE_XY							/* save the result */\
	b		ret;					/* done */\
label##0011:\
	SET_U(res)						/* 0011: operate on U */\
	SAVE_SU							/* save the result */\
	b		ret;					/* done */\
label##01xx:\
	bc		12,bit+3,label##0101;	/* if bit 3 is set, it must be 0101 (011x are invalid!) */\
label##0100:\
	_asm_get_global_b(r0,sInterruptState);\
	SET_S(res)						/* 0100: operate on S */\
	andi.	r0,r0,~kInterruptStateInhibitNMI & 0xffff;\
	SAVE_SU							/* save the result */\
	_asm_set_global_b(r0,sInterruptState);\
	b		ret;					/* done */\
label##0101:\
	SET_PC(res)						/* 0101: operate on PC */\
	ori		rTempSave,rTempSave,2;	/* signal that we need to update the banking */\
	b		ret;					/* done */\
label##1xxx:\
	bc		12,bit+2,label##101x;	/* if bit 2 is set, it must be 101x (11xx are invalid!) */\
	bc		12,bit+3,label##1001;	/* if bit 3 is set, it must be 1001 */\
label##1000:\
	SET_A(res)						/* 1000: operate on A */\
	b		ret;					/* done */\
label##1001:\
	SET_B(res)						/* 1001: operate on B */\
	b		ret;					/* done */\
label##101x:\
	bc		12,bit+3,label##1011;	/* if bit 3 is set, it must be 1011 */\
label##1010:\
	SET_CC(res)						/* 1010: operate on CC */\
	ori		rTempSave,rTempSave,1;	/* signal that we need to check the IRQ lines */\
	b		ret;					/* done */\
label##1011:\
	SET_DP(res)						/* 1011: operate on DP */

#else

#define EXPAND_REG_SET(bit,label,res,ret)	\
/*	bc		12,bit+0,label##1xxx;	/* if bit 0 is set, it must be 1xxx */\
	bc		12,bit+1,label##01xx;	/* if bit 1 is set, it must be 01xx */\
	bc		12,bit+2,label##001x;	/* if bit 2 is set, it must be 001x */\
	bc		12,bit+3,label##0001;	/* if bit 3 is set, it must be 0001 */\
label##0000:\
	SET_A(res)						/* 0000: operate on A */\
	b		ret;					/* done */\
label##0001:\
	SET_B(res)						/* 0001: operate on B */\
	b		ret;					/* done */\
label##001x:\
	bc		12,bit+3,label##0011;	/* if bit 3 is set, it must be 0011 */\
label##0010:\
	SET_X(res)						/* 0010: operate on X */\
	SAVE_XY							/* save the result */\
	b		ret;					/* done */\
label##0011:\
	SET_Y(res)						/* 0011: operate on Y */\
	SAVE_XY							/* save the result */\
	b		ret;					/* done */\
label##01xx:\
	bc		12,bit+3,label##0101;	/* if bit 3 is set, it must be 0101 (011x are invalid!) */\
label##0100:\
	_asm_get_global_b(r0,sInterruptState);\
	SET_S(res)						/* 0100: operate on S */\
	andi.	r0,r0,~kInterruptStateInhibitNMI & 0xffff;\
	SAVE_SU							/* save the result */\
	_asm_set_global_b(r0,sInterruptState);\
	b		ret;					/* done */\
label##0101:\
	SET_U(res)						/* 0101: operate on U */\
	SAVE_SU							/* save the result */\
label##1xxx:\

#endif

//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

// externally defined functions
extern UINT8		A6809_READMEM(offs_t address);
extern void 		A6809_WRITEMEM(offs_t address, UINT8 value);

// prototype for the debugger hook
#ifdef A6809_DEBUGHOOK
extern void 		A6809_DEBUGHOOK(void);
#endif

// prototype for the bank switcher
#ifdef A6809_UPDATEBANK
extern void 		A6809_UPDATEBANK(int newpc);
#endif

// prototypes for the opcode handlers
static void			neg_di(void);
static void			illegal(void);
static void			com_di(void);
static void			lsr_di(void);
static void			ror_di(void);
static void			asr_di(void);
static void			asl_di(void);
static void			rol_di(void);
static void			dec_di(void);
static void			inc_di(void);
static void			tst_di(void);
static void			jmp_di(void);
static void			clr_di(void);
static void			nop(void);
static void			sync(void);
static void			lbra(void);
static void			lbsr(void);
static void			daa(void);
static void			orcc(void);
static void			andcc(void);
static void			sex(void);
static void			exg(void);
static void			tfr(void);
static void			bra(void);
static void			brn(void);
static void			bhi(void);
static void			bls(void);
static void			bcc(void);
static void			bcs(void);
static void			bne(void);
static void			beq(void);
static void			bvc(void);
static void			bvs(void);
static void			bpl(void);
static void			bmi(void);
static void			bge(void);
static void			blt(void);
static void			bgt(void);
static void			ble(void);
static void			leax(void);
static void			leay(void);
static void			leas(void);
static void			leau(void);
static void			pshs(void);
static void			puls(void);
static void			pshu(void);
static void			pulu(void);
static void			rts(void);
static void			abx(void);
static void			rti(void);
static void			cwai(void);
static void			mul(void);
static void			swi(void);
static void			nega(void);
static void			coma(void);
static void			lsra(void);
static void			rora(void);
static void			asra(void);
static void			asla(void);
static void			rola(void);
static void			deca(void);
static void			inca(void);
static void			tsta(void);
static void			clra(void);
static void			negb(void);
static void			comb(void);
static void			lsrb(void);
static void			rorb(void);
static void			asrb(void);
static void			aslb(void);
static void			rolb(void);
static void			decb(void);
static void			incb(void);
static void			tstb(void);
static void			clrb(void);
static void			neg_ix(void);
static void			com_ix(void);
static void			lsr_ix(void);
static void			ror_ix(void);
static void			asr_ix(void);
static void			asl_ix(void);
static void			rol_ix(void);
static void			dec_ix(void);
static void			inc_ix(void);
static void			tst_ix(void);
static void			jmp_ix(void);
static void			clr_ix(void);
static void			neg_ex(void);
static void			com_ex(void);
static void			lsr_ex(void);
static void			ror_ex(void);
static void			asr_ex(void);
static void			asl_ex(void);
static void			rol_ex(void);
static void			dec_ex(void);
static void			inc_ex(void);
static void			tst_ex(void);
static void			jmp_ex(void);
static void			clr_ex(void);
static void			suba_im(void);
static void			cmpa_im(void);
static void			sbca_im(void);
static void			subd_im(void);
static void			anda_im(void);
static void			bita_im(void);
static void			lda_im(void);
static void			sta_im(void); /* ILLEGAL? */
static void			eora_im(void);
static void			adca_im(void);
static void			ora_im(void);
static void			adda_im(void);
static void			cmpx_im(void);
static void			bsr(void);
static void			ldx_im(void);
static void			stx_im(void); /* ILLEGAL? */
static void			suba_di(void);
static void			cmpa_di(void);
static void			sbca_di(void);
static void			subd_di(void);
static void			anda_di(void);
static void			bita_di(void);
static void			lda_di(void);
static void			sta_di(void);
static void			eora_di(void);
static void			adca_di(void);
static void			ora_di(void);
static void			adda_di(void);
static void			cmpx_di(void);
static void			jsr_di(void);
static void			ldx_di(void);
static void			stx_di(void);
static void			suba_ix(void);
static void			cmpa_ix(void);
static void			sbca_ix(void);
static void			subd_ix(void);
static void			anda_ix(void);
static void			bita_ix(void);
static void			lda_ix(void);
static void			sta_ix(void);
static void			eora_ix(void);
static void			adca_ix(void);
static void			ora_ix(void);
static void			adda_ix(void);
static void			cmpx_ix(void);
static void			jsr_ix(void);
static void			ldx_ix(void);
static void			stx_ix(void);
static void			suba_ex(void);
static void			cmpa_ex(void);
static void			sbca_ex(void);
static void			subd_ex(void);
static void			anda_ex(void);
static void			bita_ex(void);
static void			lda_ex(void);
static void			sta_ex(void);
static void			eora_ex(void);
static void			adca_ex(void);
static void			ora_ex(void);
static void			adda_ex(void);
static void			cmpx_ex(void);
static void			jsr_ex(void);
static void			ldx_ex(void);
static void			stx_ex(void);
static void			subb_im(void);
static void			cmpb_im(void);
static void			sbcb_im(void);
static void			addd_im(void);
static void			andb_im(void);
static void			bitb_im(void);
static void			ldb_im(void);
static void			stb_im(void); /* ILLEGAL? */
static void			eorb_im(void);
static void			adcb_im(void);
static void			orb_im(void);
static void			addb_im(void);
static void			ldd_im(void);
static void			std_im(void); /* ILLEGAL? */
static void			ldu_im(void);
static void			stu_im(void); /* ILLEGAL? */
static void			subb_di(void);
static void			cmpb_di(void);
static void			sbcb_di(void);
static void			addd_di(void);
static void			andb_di(void);
static void			bitb_di(void);
static void			ldb_di(void);
static void			stb_di(void);
static void			eorb_di(void);
static void			adcb_di(void);
static void			orb_di(void);
static void			addb_di(void);
static void			ldd_di(void);
static void			std_di(void);
static void			ldu_di(void);
static void			stu_di(void);
static void			subb_ix(void);
static void			cmpb_ix(void);
static void			sbcb_ix(void);
static void			addd_ix(void);
static void			andb_ix(void);
static void			bitb_ix(void);
static void			ldb_ix(void);
static void			stb_ix(void);
static void			eorb_ix(void);
static void			adcb_ix(void);
static void			orb_ix(void);
static void			addb_ix(void);
static void			ldd_ix(void);
static void			std_ix(void);
static void			ldu_ix(void);
static void			stu_ix(void);
static void			subb_ex(void);
static void			cmpb_ex(void);
static void			sbcb_ex(void);
static void			addd_ex(void);
static void			andb_ex(void);
static void			bitb_ex(void);
static void			ldb_ex(void);
static void			stb_ex(void);
static void			eorb_ex(void);
static void			adcb_ex(void);
static void			orb_ex(void);
static void			addb_ex(void);
static void			ldd_ex(void);
static void			std_ex(void);
static void			ldu_ex(void);
static void			stu_ex(void);

static void			lbrn(void);
static void			lbhi(void);
static void			lbls(void);
static void			lbcc(void);
static void			lbcs(void);
static void			lbne(void);
static void			lbeq(void);
static void			lbvc(void);
static void			lbvs(void);
static void			lbpl(void);
static void			lbmi(void);
static void			lbge(void);
static void			lblt(void);
static void			lbgt(void);
static void			lble(void);
static void			swi2(void);
static void			cmpd_im(void);
static void			cmpy_im(void);
static void			ldy_im(void);
static void			sty_im(void); /* ILLEGAL? */
static void			cmpd_di(void);
static void			cmpy_di(void);
static void			ldy_di(void);
static void			sty_di(void);
static void			cmpd_ix(void);
static void			cmpy_ix(void);
static void			ldy_ix(void);
static void			sty_ix(void);
static void			cmpd_ex(void);
static void			cmpy_ex(void);
static void			ldy_ex(void);
static void			sty_ex(void);
static void			lds_im(void);
static void			sts_im(void); /* ILLEGAL? */
static void			lds_di(void);
static void			sts_di(void);
static void			lds_ix(void);
static void			sts_ix(void);
static void			lds_ex(void);
static void			sts_ex(void);
static void			swi3(void);
static void			cmpu_im(void);
static void			cmps_im(void);
static void			cmpu_di(void);
static void			cmps_di(void);
static void			cmpu_ix(void);
static void			cmps_ix(void);
static void			cmpu_ex(void);
static void			cmps_ex(void);

static void			op_10(void);
static void			op_11(void);

static void			setline_im(void);
static void			setline_ix(void);
static void			lsrw_ix(void);	
static void			rorw_ix(void);
static void			asrw_ix(void);
static void			aslw_ix(void);
static void			rolw_ix(void);
static void			decbjnz(void);
static void			decxjnz(void);
static void			bmove(void);
static void			move(void);
static void			lmul(void);
static void			divx(void);
static void			lsrd_im(void);
static void			lsrd_ix(void);
static void			asrd_im(void);
static void			asrd_ix(void);
static void			rord_im(void);
static void			rord_ix(void);
static void			rold_im(void);
static void			rold_ix(void);
static void			asld_im(void);
static void			asld_ix(void);
static void			clrd(void);
static void			clrw_ix(void);
static void			negd(void);
static void			negw_ix(void);
static void			decd(void);
static void			decw_ix(void);
static void			incd(void);
static void			incw_ix(void);
static void			tstd(void);
static void			tstw_ix(void);
static void			absa(void);
static void			absb(void);
static void			absd(void);
static void			bset(void);
static void			bset2(void);

static void			eaz(void);
static void			eadi(void);
static void			eaex(void);
static void			ea16(void);
static void			eaeadi(void);
static void			eaeaex(void);
static void			eaea16(void);

static void			eaxpk(void);
static void			eaxi(void);
static void			eaxii(void);
static void			eaxd(void);
static void			eaxdd(void);
static void			eax(void);
static void			eaxpb(void);
static void			eaxpa(void);
static void			eaxp8(void);
static void			eaxp16(void);
static void			eaxpd(void);
static void			eaeaxi(void);
static void			eaeaxii(void);
static void			eaeaxd(void);
static void			eaeaxdd(void);
static void			eaeax(void);
static void			eaeaxpb(void);
static void			eaeaxpa(void);
static void			eaeaxp8(void);
static void			eaeaxp16(void);
static void			eaeaxpd(void);

static void			eaypk(void);
static void			eayi(void);
static void			eayii(void);
static void			eayd(void);
static void			eaydd(void);
static void			eay(void);
static void			eaypb(void);
static void			eaypa(void);
static void			eayp8(void);
static void			eayp16(void);
static void			eaypd(void);
static void			eaeayi(void);
static void			eaeayii(void);
static void			eaeayd(void);
static void			eaeaydd(void);
static void			eaeay(void);
static void			eaeaypb(void);
static void			eaeaypa(void);
static void			eaeayp8(void);
static void			eaeayp16(void);
static void			eaeaypd(void);

static void			eaupk(void);
static void			eaui(void);
static void			eauii(void);
static void			eaud(void);
static void			eaudd(void);
static void			eau(void);
static void			eaupb(void);
static void			eaupa(void);
static void			eaup8(void);
static void			eaup16(void);
static void			eaupd(void);
static void			eaeaui(void);
static void			eaeauii(void);
static void			eaeaud(void);
static void			eaeaudd(void);
static void			eaeau(void);
static void			eaeaupb(void);
static void			eaeaupa(void);
static void			eaeaup8(void);
static void			eaeaup16(void);
static void			eaeaupd(void);

static void			easpk(void);
static void			easi(void);
static void			easii(void);
static void			easd(void);
static void			easdd(void);
static void			eas(void);
static void			easpb(void);
static void			easpa(void);
static void			easp8(void);
static void			easp16(void);
static void			easpd(void);
static void			eaeasi(void);
static void			eaeasii(void);
static void			eaeasd(void);
static void			eaeasdd(void);
static void			eaeas(void);
static void			eaeaspb(void);
static void			eaeaspa(void);
static void			eaeasp8(void);
static void			eaeasp16(void);
static void			eaeaspd(void);

static void			eapci(void);
static void			eapcii(void);
static void			eapcd(void);
static void			eapcdd(void);
static void			eapc(void);
static void			eapcpb(void);
static void			eapcpa(void);
static void			eapcp8(void);
static void			eapcp16(void);
static void			eapcpd(void);
static void			eaeapci(void);
static void			eaeapcii(void);
static void			eaeapcd(void);
static void			eaeapcdd(void);
static void			eaeapc(void);
static void			eaeapcpb(void);
static void			eaeapcpa(void);
static void			eaeapcp8(void);
static void			eaeapcp16(void);
static void			eaeapcpd(void);

#endif
