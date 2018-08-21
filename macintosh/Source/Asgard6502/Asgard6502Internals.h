//###################################################################################################
//
//
//		Asgard6502Internals.h
//		This file contains internal data and structures that were too cumbersome to include
//		in the main Asgard6502.h.
//
//		See Asgard6502Core.c for information about the Asgard6502 system.
//
//
//###################################################################################################

#ifndef __ASGARD6502INTERNALS__
#define __ASGARD6502INTERNALS__


#if A6502_COREDEBUG
#include "Asgard6502Debugger.h"
#endif


//###################################################################################################
//
// 	These are the nonvolatile registers we use during execution
//
//###################################################################################################

#define	rPCP				r31				/* holds the PC & P registers */
#define	rSAXY				r30				/* holds the S, A, X & Y registers */
#define	rICount				r29				/* holds the current number of cycles left to execute */
#define	rICountPtr			r28				/* holds a pointer to the global that stores rICount */
#define	rCycleCount			r27				/* total cycle count for this instruction */
#define	rOpcodeROM			r26				/* holds the current base of the ROM space */
#define	rOpcodeROMPtr		r25				/* holds a pointer to the global that stores rOpcodeROM */
#define	rArgumentROM		r24				/* holds the current base of the argument ROM space */
#define	rArgumentROMPtr		r23				/* holds a pointer to the global that stores rArgumentROM */
#define	rOpcodeTable		r22				/* holds a pointer to the opcode lookup table */
#define	rEA					r21				/* holds the current effective address */
#define	rTempSave			r20				/* temporary holding register for a bunch of stuff */
#define	rLastNonVolatile	r20				/* the last non-volatible register we use */


//###################################################################################################
//
// 	Macros to transparently support bank updating
//
//###################################################################################################

#ifdef A6502_UPDATEBANK
	#define UPDATE_BANK		bl		updateBank;
#else
	#define UPDATE_BANK
#endif


//###################################################################################################
//
// 	Macros to transparently support internal memory debugging
//
//###################################################################################################

#if A6502_COREDEBUG
	#define READMEM			Asgard6502DebugRead
	#define WRITEMEM		Asgard6502DebugWrite
#else
	#define READMEM			A6502_READMEM
	#define WRITEMEM		A6502_WRITEMEM
#endif


//###################################################################################################
//
// 	These macros are used to extract the given 6502 register into the specified PPC register
//
//###################################################################################################

#define GET_S(r)			rlwinm	r,rSAXY,8,24,31;
#define GET_A(r)			rlwinm	r,rSAXY,16,24,31;
#define GET_X(r)			rlwinm	r,rSAXY,24,24,31;
#define GET_Y(r)			rlwinm	r,rSAXY,0,24,31;
#define GET_PC(r)			rlwinm	r,rPCP,16,16,31;
#define GET_P(r)			rlwinm	r,rPCP,0,24,31;


//###################################################################################################
//
// 	These macros are used to set the given 6502 register to the value in the specified PPC register
//
//###################################################################################################

#define SET_S(r)			rlwimi	rSAXY,r,24,0,7;
#define SET_A(r)			rlwimi	rSAXY,r,16,8,15;
#define SET_X(r)			rlwimi	rSAXY,r,8,16,23;
#define SET_Y(r)			rlwimi	rSAXY,r,0,24,31;
#define SET_PC(r)			rlwimi	rPCP,r,16,0,15;
#define SET_P(r)			rlwimi	rPCP,r,0,24,31;


//###################################################################################################
//
// 	These macros are used to save & load the current 6502 global state during execution
//
//###################################################################################################

// the current execution cycle count is always saved/loaded
#define SAVE_ICOUNT			stw		rICount,0(rICountPtr);
#define LOAD_ICOUNT			lwz		rICount,0(rICountPtr);

// the OPCODEROM pointer is loaded only if we do bank switching
#ifdef A6502_UPDATEBANK
	#define LOAD_ROM		lwz		rOpcodeROM,0(rOpcodeROMPtr);\
							lwz		rArgumentROM,0(rArgumentROMPtr);
#else
	#define LOAD_ROM
#endif

// the registers are optionally saved/loaded
#if A6502_VOLATILEREGS
//	#define SAVE_PCP		stw		rPCP,sPCP(rtoc);
//	#define LOAD_PCP		lwz		rPCP,sPCP(rtoc);
//	#define SAVE_SAXY		stw		rSAXY,sSAXY(rtoc);
//	#define LOAD_SAXY		lwz		rSAXY,sSAXY(rtoc);
	#define SAVE_PCP		_asm_set_global(rPCP,sPCP);
	#define LOAD_PCP		_asm_get_global(rPCP,sPCP);
	#define SAVE_SAXY		_asm_set_global(rSAXY,sSAXY);
	#define LOAD_SAXY		_asm_get_global(rSAXY,sSAXY);
#else
	#define SAVE_PCP
	#define LOAD_PCP
	#define SAVE_SAXY
	#define LOAD_SAXY
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

#define READ_OPCODE(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCP,rPCP,1;			/* increment & wrap the PC */\
	lbzx	r,rOpcodeROM,r;			/* r = immediate byte value */

#define READ_OPCODE_ARG(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCP,rPCP,1;			/* increment & wrap the PC */\
	lbzx	r,rArgumentROM,r;		/* r = immediate byte value */


//###################################################################################################
//
// 	Read an opcode argument word from OPCODERAM to 'r' and increment the PC
//
//###################################################################################################

#define READ_OPCODE_ARG_WORD(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCP,rPCP,2;			/* increment & wrap the PC */\
	lhbrx	r,rArgumentROM,r;		/* r = immediate byte value */


//###################################################################################################
//
// 	PUSHes the high or low part of the specified 'reg', optionally saving the registers as well
//
//###################################################################################################

#define PUSH_BYTE(reg,shift) \
	SAVE_PCP						/* save PC/P */\
	GET_S(r3)						/* extract the stack pointer */\
	rlwinm	r4,reg,(32-shift)&31,24,31;	/* extract the byte */\
	subis	rSAXY,rSAXY,0x100;		/* post-decrement the stack */\
	ori		r3,r3,0x100;			/* stack is always in the 100-1ff range */\
	SAVE_SAXY						/* save the updated stack */\
	bl		WRITEMEM;				/* perform the write */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	POPs the high or low part of the specified 'reg', saving the registers as well
//
//###################################################################################################

#define POP_BYTE(reg,shift) \
	addis	rSAXY,rSAXY,0x100;		/* pre-increment the stack */\
	SAVE_PCP						/* save PC/P */\
	GET_S(r3)						/* extract the stack pointer */\
	ori		r3,r3,0x100;			/* stack is always in the 100-1ff range */\
	SAVE_SAXY						/* save the updated stack */\
	bl		READMEM;				/* perform the read */\
	rlwimi	reg,r3,shift,(24-shift)&31,(31-shift)&31;/* extract the byte */\
	bl		postExtern;				/* post-process */


//###################################################################################################
//
// 	Effective address computations
//
//###################################################################################################

#define COMPUTE_EA_ABS(cycles) \
	READ_OPCODE_ARG_WORD(rEA) \
	CYCLES(cycles) \
	SAVE_PCP

#define COMPUTE_EA_ABX(cycles) \
	GET_X(r3) \
	READ_OPCODE_ARG_WORD(rEA) \
	CYCLES(cycles) \
	add		rEA,rEA,r3; \
	SAVE_PCP

#define COMPUTE_EA_ABY(cycles) \
	GET_Y(r3) \
	READ_OPCODE_ARG_WORD(rEA) \
	CYCLES(cycles) \
	add		rEA,rEA,r3; \
	SAVE_PCP

#define COMPUTE_EA_ZPG(cycles) \
	READ_OPCODE_ARG(rEA) \
	CYCLES(cycles) \
	SAVE_PCP

#define COMPUTE_EA_ZPX(cycles) \
	GET_X(r3) \
	READ_OPCODE_ARG(rEA) \
	SAVE_PCP \
	add		rEA,rEA,r3; \
	CYCLES(cycles) \
	rlwinm	rEA,rEA,0,24,31

#define COMPUTE_EA_ZPY(cycles) \
	GET_Y(r3) \
	READ_OPCODE_ARG(rEA) \
	SAVE_PCP \
	add		rEA,rEA,r3; \
	CYCLES(cycles) \
	rlwinm	rEA,rEA,0,24,31

#define COMPUTE_EA_ZPI(cycles) \
	READ_OPCODE_ARG(rTempSave) \
	SAVE_PCP \
	CYCLES(cycles) \
	rlwinm	r3,rTempSave,0,24,31; \
	bl		READMEM; \
	addi	rTempSave,rTempSave,1; \
	rlwinm	rEA,r3,0,24,31; \
	rlwinm	r3,rTempSave,0,24,31; \
	bl		READMEM; \
	rlwimi	rEA,r3,8,16,23; \
	bl		postExtern

#define COMPUTE_EA_IDX(cycles) \
	GET_X(r3) \
	READ_OPCODE_ARG(rTempSave) \
	SAVE_PCP \
	add		rTempSave,rTempSave,r3; \
	CYCLES(cycles) \
	rlwinm	r3,rTempSave,0,24,31; \
	bl		READMEM; \
	addi	rTempSave,rTempSave,1; \
	rlwinm	rEA,r3,0,24,31; \
	rlwinm	r3,rTempSave,0,24,31; \
	bl		READMEM; \
	rlwimi	rEA,r3,8,16,23; \
	bl		postExtern

#define COMPUTE_EA_IDY(cycles) \
	READ_OPCODE_ARG(rTempSave) \
	SAVE_PCP \
	CYCLES(cycles) \
	rlwinm	r3,rTempSave,0,24,31; \
	bl		READMEM; \
	addi	rTempSave,rTempSave,1; \
	rlwinm	rEA,r3,0,24,31; \
	rlwinm	r3,rTempSave,0,24,31; \
	bl		READMEM; \
	rlwimi	rEA,r3,8,16,23; \
	GET_Y(r3) \
	mr		rTempSave,rEA; \
	add		rEA,rEA,r3; \
	xor		rTempSave,rTempSave,rEA; \
	bl		postExtern; \
	rlwinm	rTempSave,rTempSave,24,31,31; \
	add		rCycleCount,rCycleCount,rTempSave /* add a cycle for page crossing */

#define COMPUTE_EA_IND(cycles) \
	READ_OPCODE_ARG_WORD(rTempSave) \
	CYCLES(cycles) \
	SAVE_PCP \
	rlwinm	r3,rTempSave,0,16,31; \
	bl		READMEM; \
	rlwinm	rEA,r3,0,24,31; \
	addi	r3,rTempSave,1; \
	rlwimi	r3,rTempSave,0,16,23; /* keep on same page! */\
	bl		READMEM; \
	rlwimi	rEA,r3,8,16,23; \
	bl		postExtern

#define COMPUTE_EA_IAX(cycles) \
	READ_OPCODE_ARG_WORD(rTempSave) \
	CYCLES(cycles) \
	SAVE_PCP \
	rlwinm	r3,rTempSave,0,16,31; \
	bl		READMEM; \
	rlwinm	rEA,r3,0,24,31; \
	addi	r3,rTempSave,1; \
	rlwimi	r3,rTempSave,0,16,23; /* keep on same page! */\
	bl		READMEM; \
	rlwimi	rEA,r3,8,16,23; \
	bl		postExtern; \
	GET_X(r3) \
	mr		rTempSave,rEA; \
	add		rEA,rEA,r3; \
	xor		rTempSave,rTempSave,rEA; \
	rlwinm	rTempSave,rTempSave,24,31,31; \
	add		rCycleCount,rCycleCount,rTempSave /* add a cycle for page crossing */


//###################################################################################################
//	FUNCTION PROTOTYPES
//###################################################################################################

// externally defined functions
extern UINT8		A6502_READMEM(offs_t address);
extern void 		A6502_WRITEMEM(offs_t address, UINT8 value);

// prototype for the debugger hook
#ifdef A6502_DEBUGHOOK
extern void 		A6502_DEBUGHOOK(void);
#endif

// prototype for the bank switcher
#ifdef A6502_UPDATEBANK
extern void 		A6502_UPDATEBANK(int newpc);
#endif

// prototypes for the opcode handlers
static void illegal(void);

static void adc_abs(void);
static void adc_abx(void);
static void adc_aby(void);
static void adc_idx(void);
static void adc_idy(void);
static void adc_imm(void);
static void adc_zpg(void);
static void adc_zpx(void);
static void adc_zpi(void);

static void and_abs(void);
static void and_abx(void);
static void and_aby(void);
static void and_idx(void);
static void and_idy(void);
static void and_imm(void);
static void and_zpg(void);
static void and_zpx(void);
static void and_zpi(void);

static void asl_a(void);
static void asl_abs(void);
static void asl_abx(void);
static void asl_zpg(void);
static void asl_zpx(void);

static void bit_abs(void);
static void bit_abx(void);
static void bit_imm(void);
static void bit_zpg(void);
static void bit_zpx(void);

static void bra	(void);
static void bcc(void);
static void bcs(void);
static void beq(void);
static void bmi(void);
static void bne(void);
static void bpl(void);
static void brk(void);
static void bvc(void);
static void bvs(void);

static void bbr0_zpg(void);
static void bbr1_zpg(void);
static void bbr2_zpg(void);
static void bbr3_zpg(void);
static void bbr4_zpg(void);
static void bbr5_zpg(void);
static void bbr6_zpg(void);
static void bbr7_zpg(void);

static void bbs0_zpg(void);
static void bbs1_zpg(void);
static void bbs2_zpg(void);
static void bbs3_zpg(void);
static void bbs4_zpg(void);
static void bbs5_zpg(void);
static void bbs6_zpg(void);
static void bbs7_zpg(void);

static void clc(void);
static void cld(void);
static void cli(void);
static void clv(void);

static void cmp_abs(void);
static void cmp_abx(void);
static void cmp_aby(void);
static void cmp_idx(void);
static void cmp_idy(void);
static void cmp_imm(void);
static void cmp_zpg(void);
static void cmp_zpx(void);
static void cmp_zpi(void);

static void cpx_abs(void);
static void cpx_imm(void);
static void cpx_zpg(void);

static void cpy_abs(void);
static void cpy_imm(void);
static void cpy_zpg(void);

static void dec_abs(void);
static void dec_abx(void);
static void dec_zpg(void);
static void dec_zpx(void);
static void dea(void);
static void dex(void);
static void dey(void);

static void eor_abs(void);
static void eor_abx(void);
static void eor_aby(void);
static void eor_idx(void);
static void eor_idy(void);
static void eor_imm(void);
static void eor_zpg(void);
static void eor_zpx(void);
static void eor_zpi(void);

static void inc_abs(void);
static void inc_abx(void);
static void inc_zpg(void);
static void inc_zpx(void);
static void ina(void);
static void inx(void);
static void iny(void);

static void jmp_abs(void);
static void jmp_ind(void);
static void jmp_iax(void);
static void jsr(void);

static void lda_abs(void);
static void lda_abx(void);
static void lda_aby(void);
static void lda_idx(void);
static void lda_idy(void);
static void lda_imm(void);
static void lda_zpg(void);
static void lda_zpx(void);
static void lda_zpi(void);

static void ldx_abs(void);
static void ldx_aby(void);
static void ldx_imm(void);
static void ldx_zpg(void);
static void ldx_zpy(void);

static void ldy_abs(void);
static void ldy_abx(void);
static void ldy_imm(void);
static void ldy_zpg(void);
static void ldy_zpx(void);

static void lsr_a(void);
static void lsr_abs(void);
static void lsr_abx(void);
static void lsr_zpg(void);
static void lsr_zpx(void);

static void nop(void);

static void ora_abs(void);
static void ora_abx(void);
static void ora_aby(void);
static void ora_idx(void);
static void ora_idy(void);
static void ora_imm(void);
static void ora_zpg(void);
static void ora_zpx(void);
static void ora_zpi(void);

static void pha(void);
static void phy(void);
static void phx(void);
static void php(void);

static void pla(void);
static void plp(void);
static void plx(void);
static void ply(void);

static void rmb0_zpg(void);
static void rmb1_zpg(void);
static void rmb2_zpg(void);
static void rmb3_zpg(void);
static void rmb4_zpg(void);
static void rmb5_zpg(void);
static void rmb6_zpg(void);
static void rmb7_zpg(void);

static void rol_a(void);
static void rol_abs(void);
static void rol_abx(void);
static void rol_zpg(void);
static void rol_zpx(void);

static void ror_a(void);
static void ror_abs(void);
static void ror_abx(void);
static void ror_zpg(void);
static void ror_zpx(void);

static void rti(void);
static void rts(void);

static void sbc_abs(void);
static void sbc_abx(void);
static void sbc_aby(void);
static void sbc_idx(void);
static void sbc_idy(void);
static void sbc_imm(void);
static void sbc_zpg(void);
static void sbc_zpx(void);
static void sbc_zpi(void);

static void sec(void);
static void sed(void);
static void sei(void);

static void smb0_zpg(void);
static void smb1_zpg(void);
static void smb2_zpg(void);
static void smb3_zpg(void);
static void smb4_zpg(void);
static void smb5_zpg(void);
static void smb6_zpg(void);
static void smb7_zpg(void);

static void sta_abs(void);
static void sta_abx(void);
static void sta_aby(void);
static void sta_idx(void);
static void sta_idy(void);
static void sta_zpg(void);
static void sta_zpx(void);
static void sta_zpi(void);

static void stx_abs(void);
static void stx_zpg(void);
static void stx_zpy(void);

static void sty_abs(void);
static void sty_zpg(void);
static void sty_zpx(void);

static void stz_abs(void);
static void stz_abx(void);
static void stz_zpg(void);
static void stz_zpx(void);

static void trb_abs(void);
static void trb_zpg(void);

static void tsb_abs(void);
static void tsb_zpg(void);

static void tax(void);
static void tay(void);
static void tsx(void);
static void txa(void);
static void txs(void);
static void tya(void);

// Deco16-specific opcodes
static void deco_unk(void);
static void in_a_byte(void);
static void out_byte(void);

#endif
