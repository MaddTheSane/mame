//###################################################################################################
//
//
//		Asgard6803.c
//		This file contains internal data and structures that were too cumbersome to include
//		in the main Asgard6800.h.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#ifndef __ASGARD6800INTERNALS__
#define __ASGARD6800INTERNALS__


#if A6800_COREDEBUG
#include "Asgard6800Debugger.h"
#endif


//###################################################################################################
//
// 	Macros to simplify expressions
//
//###################################################################################################

#if (A6800_CHIP == 6801 || A6800_CHIP == 6803 || A6800_CHIP == 63701)
	#define INTERNAL_REGISTERS		1
#else
	#define	INTERNAL_REGISTERS		0
#endif


//###################################################################################################
//
// 	Here is how the M6800_FLAGS internal register is mapped out
//
//###################################################################################################

enum
{
	kFlagsDirty				= 0x00000100,		// DIRTY = bit  23
	kFlagsCC				= 0x000000ff		// CC    = bits 24-31
};


//###################################################################################################
//
// 	These are the nonvolatile registers we use during execution
//
//###################################################################################################

#define	rPCAB				r31				/* holds the PC, A & B registers */
#define	rFlags				r30				/* holds the CC register plus pending interrupt flags */
#define	rSX					r29				/* holds the S & X registers */
#define	rICount				r28				/* holds the current number of cycles left to execute */
#define	rICountPtr 			r27				/* holds a pointer to the global that stores rICount */
#define	rCycleCount			r26				/* total cycle count for this instruction */
#define	rOpcodeROM			r25				/* holds the current base of the ROM space */
#define	rOpcodeROMPtr 		r24				/* holds a pointer to the global that stores rOpcodeROM */
#define	rArgumentROM		r23				/* holds the current base of the RAM space */
#define	rArgumentROMPtr 	r22				/* holds a pointer to the global that stores rArgumentROM */
#define	rOpcodeTable		r21				/* holds a pointer to the opcode lookup table */
#define	rEA					r20				/* holds the current effective address */
#define	rTempSave			r19				/* temporary holding register for a bunch of stuff */
#define	rTempSave2			r18				/* temporary nonvolatile register */
#define	rLinkSave			r17				/* temporary holding register for the PowerPC LR */
#define	rLinkSave2			r16				/* temporary holding register for the PowerPC LR */
#define	rCounter			r15				/* current value of the internal counter */
#define	rOutputCompareLeft	r14				/* current value of the output compare register */
#define	rLastNonVolatile	r14				/* the last non-volatible register we use */


//###################################################################################################
//
// 	Macros to transparently support bank updating
//
//###################################################################################################

#ifdef A6800_UPDATEBANK
	#define UPDATE_BANK		bl		updateBank;
#else
	#define UPDATE_BANK
#endif


//###################################################################################################
//
// 	Macros to transparently support internal memory debugging
//
//###################################################################################################

#if A6800_COREDEBUG
	#define READMEM			Asgard6800DebugRead
	#define WRITEMEM		Asgard6800DebugWrite
#else
	#define READMEM			A6800_READMEM
	#define WRITEMEM		A6800_WRITEMEM
#endif

#define READPORT			A6800_READPORT
#define WRITEPORT			A6800_WRITEPORT


//###################################################################################################
//
// 	These macros are used to extract the given 6800 register into the specified PPC register
//
//###################################################################################################

#define GET_X(r)			rlwinm	r,rSX,0,16,31;
#define GET_S(r)			rlwinm	r,rSX,16,16,31;
#define GET_PC(r)			rlwinm	r,rPCAB,16,16,31;
#define GET_A(r)			rlwinm	r,rPCAB,24,24,31;
#define GET_B(r)			rlwinm	r,rPCAB,0,24,31;
#define GET_D(r)			rlwinm	r,rPCAB,0,16,31;
#define GET_CC(r)			rlwinm	r,rFlags,0,24,31;


//###################################################################################################
//
// 	These macros are used to set the given 6800 register to the value in the specified PPC register
//
//###################################################################################################

#define SET_X(r)			rlwimi	rSX,r,0,16,31;
#define SET_S(r)			rlwimi	rSX,r,16,0,15;
#define SET_PC(r)			rlwimi	rPCAB,r,16,0,15;
#define SET_A(r)			rlwimi	rPCAB,r,8,16,23;
#define SET_B(r)			rlwimi	rPCAB,r,0,24,31;
#define SET_D(r)			rlwimi	rPCAB,r,0,16,31;
#define SET_CC(r)			rlwimi	rFlags,r,0,24,31;


//###################################################################################################
//
// 	These macros are used to save/load the current M6800 global state during execution
//
//###################################################################################################

// control flags are always saved/loaded
#define SAVE_FLAGS			_asm_set_global(rFlags,sFlags);
#define LOAD_FLAGS			_asm_get_global(rFlags,sFlags);

// the current execution cycle count is always saved/loaded
#define SAVE_ICOUNT			stw		rICount,0(rICountPtr);
#define LOAD_ICOUNT			lwz		rICount,0(rICountPtr);

// the counter is always saved for chips that support it
#if (A6800_CHIP == 6801 || A6800_CHIP == 6803 || A6800_CHIP == 63701)
	#define LOAD_COUNTER	_asm_get_global_h(rCounter,sCounter);
	#define SAVE_COUNTER	_asm_set_global_h(rCounter,sCounter);
	#define LOAD_OCOMPARE	_asm_get_global(rOutputCompareLeft,sOutputCompareLeft);
	#define SAVE_OCOMPARE	_asm_set_global(rOutputCompareLeft,sOutputCompareLeft);
#else
	#define LOAD_COUNTER
	#define SAVE_COUNTER
	#define LOAD_OCOMPARE
	#define SAVE_OCOMPARE
#endif

// the OPCODEROM pointer is loaded only if we do bank switching
#ifdef A6800_UPDATEBANK
	#define LOAD_ROM		lwz		rOpcodeROM,0(rOpcodeROMPtr);\
							lwz		rArgumentROM,0(rArgumentROMPtr);
#else
	#define LOAD_ROM
#endif

// the registers are optionally saved/loaded
#if A6800_VOLATILEREGS
	#define SAVE_PCAB		_asm_set_global(rPCAB,sPCAB);
	#define LOAD_PCAB		_asm_get_global(rPCAB,sPCAB);
	#define SAVE_SX			_asm_set_global(rSX,sSX);
	#define LOAD_SX			_asm_get_global(rSX,sSX);
#else
	#define SAVE_PCAB
	#define LOAD_PCAB
	#define SAVE_SX
	#define LOAD_SX
#endif


//###################################################################################################
//
// 	Count cycles for this operation; should be done before registers are saved
//
//###################################################################################################

#if (A6800_CHIPTIMING == 6800 || A6800_CHIPTIMING == 6802 || A6800_CHIPTIMING == 6808 || A6800_CHIPTIMING == 8105)
	#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,a
#elif (A6800_CHIPTIMING == 6801 || A6800_CHIPTIMING == 6803)
	#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,b
#elif (A6800_CHIPTIMING == 63701)
	#define CYCLES(a,b,c) 	addi	rCycleCount,rCycleCount,c
#else
	#error You must define A6800_CHIPTIMING to be 6800, 6801, 6802, 6803, 6808, or 63701!
#endif


//###################################################################################################
//
// 	Update the cycle count after an opcode
//
//###################################################################################################

#if INTERNAL_REGISTERS
	#define COUNT_CYCLES(x)	add		r3,rCounter,rCycleCount;\
							sub		rOutputCompareLeft,rOutputCompareLeft,rCycleCount;\
							cmplwi	cr2,r3,0xffff;\
							rlwinm	rCounter,r3,0,16,31;\
							cmpwi	cr3,rOutputCompareLeft,0;\
							sub.	rICount,rICount,rCycleCount;\
							SAVE_COUNTER;\
							SAVE_ICOUNT;\
							SAVE_OCOMPARE;\
							bge-	cr2,x##wrapAround;\
							bge+	cr3,x##timerDone;\
						x##doOCI:\
							_asm_get_global_b(r3,sTCSR);\
							_asm_get_global_b(r4,sTCSRPending);\
						x##doOCIToo:\
							addis	rOutputCompareLeft,rOutputCompareLeft,1;\
							ori		r3,r3,k6800TCSRStateOCF;\
							ori		r4,r4,k6800TCSRStateOCF;\
						x##finishInt:\
							_asm_set_global_b(r3,sTCSR);\
							_asm_set_global_b(r4,sTCSRPending);\
							rlwinm	r4,r3,3,24,28;\
							and		r3,r3,r4;\
							_asm_set_global_b(r3,sIRQ2Flags);\
							bl		checkIRQLines;\
							cmpwi	rICount,0;\
							b		x##timerDone;\
						x##wrapAround:\
							_asm_get_global_b(r3,sTCSR);\
							_asm_get_global_b(r4,sTCSRPending);\
							ori		r3,r3,k6800TCSRStateTOF;\
							ori		r4,r4,k6800TCSRStateTOF;\
							blt-	cr3,x##doOCIToo;\
							b		x##finishInt;\
						x##timerDone:
#else
	#define COUNT_CYCLES(x)	sub.	rICount,rICount,rCycleCount;\
							SAVE_ICOUNT;
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
	SAVE_FLAGS						/* save flags */\
	bl		READMEM;				/* perform the read */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Read a word from memory address 'r' into rTempSave, saving the registers as well
//
//###################################################################################################

#define READ_WORD_SAVE(r) \
	SAVE_PCAB						/* save PC/AB */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	SAVE_FLAGS						/* save flags */\
	bl		READMEM;				/* perform the read */\
	addi	r4,r,1;					/* add one to the address */\
	rlwinm	rTempSave,r3,8,16,23;	/* save the result temporarily */\
	rlwinm	r3,r4,0,16,31;			/* keep it in range */\
	bl		READMEM;				/* perform the read */\
	rlwimi	rTempSave,r3,0,24,31;	/* combine the two bytes */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Write a word to memory address 'r' from the low 16 bits of register 'd', 
//		saving the registers as well
//
//###################################################################################################

#define WRITE_WORD_LO_SAVE(r,d) \
	SAVE_PCAB						/* save PC/AB */\
	rlwinm	r3,r,0,16,31;			/* get the address in r3 */\
	SAVE_FLAGS						/* save flags */\
	rlwinm	r4,d,24,24,31;			/* get the high byte of data in r4 */\
	bl		WRITEMEM;				/* perform the write */\
	addi	r3,r,1;					/* add one to the address */\
	rlwinm	r4,d,0,24,31;			/* get the low byte of data in r4 */\
	rlwinm	r3,r3,0,16,31;			/* keep it in range */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	EA Indexed: Fetches the next byte of the opcode and adds X
//
//###################################################################################################

#define INDEXED \
	READ_OPCODE_ARG(r3)				/* r3 = next opcode byte */\
	GET_X(rEA)						/* rEA = X */\
	add		rEA,rEA,r3;				/* rEA = X + M_RDOP() */


//###################################################################################################
//
// 	EA Direct: Fetches the next byte of the opcode
//
//###################################################################################################

#define DIRECT \
	READ_OPCODE_ARG(rEA)			/* rEA = next opcode byte */


//###################################################################################################
//
// 	EA Extended: Fetches the next word of the opcode as EA
//
//###################################################################################################

#define EXTENDED \
	READ_OPCODE_ARG_WORD(rEA)		/* rEA = next opcode word */
	

//###################################################################################################
//
// 	Performs a PUSH operation on the high/low half of register 'reg'
//
//###################################################################################################

#define PUSH_WORD_HI_SAVE(reg) \
	SAVE_PCAB						/* save PC/AB */\
	GET_S(r3)						/* get the sp */\
	rlwinm	r4,reg,16,24,31;		/* get the low part into r4 */\
	SAVE_FLAGS						/* save flags */\
	subis	rSX,rSX,1;				/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	GET_S(r3)						/* get the sp */\
	rlwinm	r4,reg,8,24,31;			/* get the high part into r4 */\
	subis	rSX,rSX,1;				/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	SAVE_SX							/* save the new sp */\
	bl			postExtern;			/* post-process */

#define PUSH_WORD_LO_SAVE(reg) \
	SAVE_PCAB						/* save PC/AB */\
	GET_S(r3)						/* get the sp */\
	rlwinm	r4,reg,0,24,31;			/* get the low part into r4 */\
	SAVE_FLAGS						/* save flags */\
	subis	rSX,rSX,1;				/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	GET_S(r3)						/* get the sp */\
	rlwinm	r4,reg,24,24,31;		/* get the high part into r4 */\
	subis	rSX,rSX,1;				/* decrement the sp */\
	bl		WRITEMEM;				/* perform the write */\
	SAVE_SX							/* save the new sp */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Performs a PULL operation on the high/low half of register 'reg'
//
//###################################################################################################

#define PULL_WORD_HI_SAVE(reg) \
	SAVE_PCAB						/* save PC/AB */\
	addis	rSX,rSX,1;				/* increment the sp */\
	SAVE_FLAGS						/* save flags */\
	GET_S(r3)						/* get the sp */\
	bl		READMEM;				/* perform the read */\
	addis	rSX,rSX,1;				/* increment the sp */\
	rlwimi	reg,r3,24,0,7;			/* extract the high bits */\
	GET_S(r3)						/* get the sp */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,16,8,15;			/* extract the low bits */\
	SAVE_SX							/* save the new sp */\
	bl		postExtern;				/* post-process */

#define PULL_WORD_LO_SAVE(reg) \
	SAVE_PCAB						/* save PC/AB */\
	addis	rSX,rSX,1;				/* increment the sp */\
	SAVE_FLAGS						/* save flags */\
	GET_S(r3)						/* get the sp */\
	bl		READMEM;				/* perform the read */\
	addis	rSX,rSX,1;				/* increment the sp */\
	rlwimi	reg,r3,8,16,23;			/* extract the high bits */\
	GET_S(r3)						/* get the sp */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,0,24,31;			/* extract the low bits */\
	SAVE_SX							/* save the new sp */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

// externally defined functions
extern UINT8		A6800_READMEM(offs_t address);
extern void 		A6800_WRITEMEM(offs_t address, UINT8 value);

// prototype for the debugger hook
#ifdef A6800_DEBUGHOOK
extern void 		A6800_DEBUGHOOK(void);
#endif

// prototype for the bank switcher
#ifdef A6800_UPDATEBANK
extern void 		A6800_UPDATEBANK(int newpc);
#endif

// prototypes for the opcode handlers
static void illegal(void);
static void swi(void);
static void psha(void);
static void pshb(void);
static void pula(void);
static void pulb(void);
static void lda_im(void);
static void lda_di(void);
static void lda_ix(void);
static void lda_ex(void);
static void ldb_im(void);
static void ldb_di(void);
static void ldb_ix(void);
static void ldb_ex(void);
static void lds_im(void);
static void lds_di(void);
static void lds_ix(void);
static void lds_ex(void);
static void ldx_im(void);
static void ldx_di(void);
static void ldx_ix(void);
static void ldx_ex(void);
static void sta_im(void);
static void stb_im(void);
static void sta_di(void);
static void sta_ix(void);
static void sta_ex(void);
static void stb_di(void);
static void stb_ix(void);
static void stb_ex(void);
static void sts_im(void);
static void stx_im(void);
static void sts_di(void);
static void sts_ix(void);
static void sts_ex(void);
static void stx_di(void);
static void stx_ix(void);
static void stx_ex(void);
static void tap(void);
static void tpa(void);
static void tba(void);
static void tab(void);
static void tsx(void);
static void txs(void);
static void clr_ix(void);
static void clr_ex(void);
static void clra(void);
static void clrb(void);
static void aba(void);
static void adda_im(void);
static void adda_di(void);
static void adda_ix(void);
static void adda_ex(void);
static void addb_im(void);
static void addb_di(void);
static void addb_ix(void);
static void addb_ex(void);
static void adca_im(void);
static void adca_di(void);
static void adca_ix(void);
static void adca_ex(void);
static void adcb_im(void);
static void adcb_di(void);
static void adcb_ix(void);
static void adcb_ex(void);
static void abx(void);
static void sba(void);
static void suba_im(void);
static void suba_di(void);
static void suba_ix(void);
static void suba_ex(void);
static void subb_im(void);
static void subb_di(void);
static void subb_ix(void);
static void subb_ex(void);
static void sbca_im(void);
static void sbca_di(void);
static void sbca_ix(void);
static void sbca_ex(void);
static void sbcb_im(void);
static void sbcb_di(void);
static void sbcb_ix(void);
static void sbcb_ex(void);
static void neg_ix(void);
static void neg_ex(void);
static void nega(void);
static void negb(void);
static void inc_ix(void);
static void inc_ex(void);
static void inca(void);
static void incb(void);
static void inx(void);
static void ins(void);
static void dec_ix(void);
static void dec_ex(void);
static void deca(void);
static void decb(void);
static void dex(void);
static void des(void);
static void cba(void);
static void cmpa_im(void);
static void cmpa_di(void);
static void cmpa_ix(void);
static void cmpa_ex(void);
static void cmpb_im(void);
static void cmpb_di(void);
static void cmpb_ix(void);
static void cmpb_ex(void);
static void cmpx_im(void);
static void cmpx_di(void);
static void cmpx_ix(void);
static void cmpx_ex(void);
static void tst_ix(void);
static void tst_ex(void);
static void tsta(void);
static void tstb(void);
static void lsr_ix(void);
static void lsr_ex(void);
static void lsra(void);
static void lsrb(void);
static void ror_ix(void);
static void ror_ex(void);
static void rora(void);
static void rorb(void);
static void asr_ix(void);
static void asr_ex(void);
static void asra(void);
static void asrb(void);
static void asl_ix(void);
static void asl_ex(void);
static void asla(void);
static void aslb(void);
static void rol_ix(void);
static void rol_ex(void);
static void rola(void);
static void rolb(void);
static void com_ix(void);
static void com_ex(void);
static void coma(void);
static void comb(void);
static void anda_im(void);
static void anda_di(void);
static void anda_ix(void);
static void anda_ex(void);
static void andb_im(void);
static void andb_di(void);
static void andb_ix(void);
static void andb_ex(void);
static void ora_im(void);
static void ora_di(void);
static void ora_ix(void);
static void ora_ex(void);
static void orb_im(void);
static void orb_di(void);
static void orb_ix(void);
static void orb_ex(void);
static void eora_im(void);
static void eora_di(void);
static void eora_ix(void);
static void eora_ex(void);
static void eorb_im(void);
static void eorb_di(void);
static void eorb_ix(void);
static void eorb_ex(void);
static void bita_im(void);
static void bita_di(void);
static void bita_ix(void);
static void bita_ex(void);
static void bitb_im(void);
static void bitb_di(void);
static void bitb_ix(void);
static void bitb_ex(void);
static void clv(void);
static void sev(void);
static void clc(void);
static void sec(void);
static void cli(void);
static void sti(void);
static void bra(void);
static void bsr(void);
static void rts(void);
static void rti(void);
static void brn(void);
static void bhi(void);
static void bls(void);
static void bcc(void);
static void bcs(void);
static void bne(void);
static void beq(void);
static void bvc(void);
static void bvs(void);
static void bpl(void);
static void bmi(void);
static void bge(void);
static void blt(void);
static void bgt(void);
static void ble(void);
static void jmp_ix(void);
static void jmp_ex(void);
static void jsr_di(void);
static void jsr_ix(void);
static void jsr_ex(void);
static void wai(void);
static void daa(void);
static void nop(void);

extern void lsrd(void);
extern void asld(void);
extern void addd_im(void);
extern void addd_di(void);
extern void addd_ix(void);
extern void addd_ex(void);
extern void ldd_im(void);
extern void ldd_di(void);
extern void ldd_ix(void);
extern void ldd_ex(void);
extern void mul(void);
extern void pshx(void);
extern void pulx(void);
extern void std_im(void);
extern void std_di(void);
extern void std_ix(void);
extern void std_ex(void);
extern void subd_im(void);
extern void subd_di(void);
extern void subd_ix(void);
extern void subd_ex(void);

extern void aim_ix(void);
extern void oim_ix(void);
extern void eim_ix(void);
extern void tim_ix(void);
extern void aim_di(void);
extern void oim_di(void);
extern void eim_di(void);
extern void tim_di(void);
extern void xgdx(void);
extern void undoc1(void);
extern void undoc2(void);

extern void addx_ex(void);
extern void adcx_im(void);

#endif
