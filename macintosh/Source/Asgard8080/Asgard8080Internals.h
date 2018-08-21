//###################################################################################################
//
//
//		Asgard8080Internals.h
//		This file contains internal data and structures that were too cumbersome to include
//		in the main Asgard8080.h.
//
//		See Asgard8080Core.c for information about the Asgard8080 system.
//
//
//###################################################################################################

#ifndef __ASGARD8080INTERNALS__
#define __ASGARD8080INTERNALS__


#if A8080_COREDEBUG
#include "Asgard8080Debugger.h"
#endif


//###################################################################################################
//
// 	Here is how the Flags internal register is mapped out
//
//###################################################################################################

enum
{
	kFlagsRealIEN			= 0x00000400,		// RealIEN	= bit  21
	kFlagsDirty				= 0x00000200,		// Dirty	= bit  22
	kFlagsHALT				= 0x00000100,		// HALT		= bit  23
	kFlagsIM				= 0x000000ff,		// IM		= bits 24-31

	kFlagsSOD				= 0x00000080,		// SOD      = bit  24
	kFlagsSOE				= 0x00000040,		// SOE      = bit  25
	kFlagsR75				= 0x00000010,		// R75      = bit  27
	kFlagsMSE				= 0x00000008,		// MSE      = bit  28
	kFlagsM75				= 0x00000004,		// RST75    = bit  29
	kFlagsM65				= 0x00000002,		// RST65    = bit  30
	kFlagsM55				= 0x00000001,		// RST55    = bit  31

	kFlagsSID				= 0x00000080,		// SID      = bit  24
	kFlagsI75				= 0x00000040,		// I75      = bit  25
	kFlagsI65				= 0x00000020,		// I65      = bit  26
	kFlagsI55				= 0x00000010,		// I55      = bit  27
	kFlagsIEN				= 0x00000008		// IEN      = bit  28
};


//###################################################################################################
//
// 	These macros can be used in __rlwinm instructions to extract the appropriate flag
//
//###################################################################################################

#define EXTRACT_HALT		24,31,31
#define EXTRACT_DIRTY		23,31,31


//###################################################################################################
//
// 	These macros can be used in __rlwimi instructions to insert the appropriate flag
//
//###################################################################################################

#define INSERT_HALT			 8,23,23


//###################################################################################################
//
// 	These are the nonvolatile registers we use during execution
//
//###################################################################################################

#define	rPCAF				r31				/* holds the PC, A & F registers */
#define	rHLBC				r30				/* holds the H, L, B & C registers */
#define	rSPDE				r29				/* holds the SP, D & E registers */
#define	rFlags				r28				/* holds the internal flags */
#define	rICount				r27				/* holds the current number of cycles left to execute */
#define	rICountPtr			r26				/* holds a pointer to the global that stores rICount */
#define	rCycleCount			r25				/* total cycle count for this instruction */
#define	rOpcodeROM			r24				/* holds the current base of the ROM space */
#define	rOpcodeROMPtr		r23				/* holds a pointer to the global that stores rOpcodeROM */
#define	rArgumentROM		r22				/* holds the current base of the argument ROM space */
#define	rArgumentROMPtr		r21				/* holds a pointer to the global that stores rArgumentROM */
#define	rOpcodeTable		r20				/* holds a pointer to the opcode lookup table */
#define	rFlagTable			r19				/* holds a pointer to the flags lookup table */
#define	rEA					r18				/* holds the current effective address */
#define	rTempSave			r17				/* temporary holding register for a bunch of stuff */
#define	rLastNonVolatile	r17				/* the last non-volatible register we use */


//###################################################################################################
//
// 	Macros to transparently support bank updating
//
//###################################################################################################

#ifdef A8080_UPDATEBANK
	#define UPDATE_BANK		bl		updateBank;
#else
	#define UPDATE_BANK
#endif


//###################################################################################################
//
// 	Macros to transparently support internal memory debugging
//
//###################################################################################################

#if A8080_COREDEBUG
	#define READMEM			Asgard8080DebugRead
	#define WRITEMEM		Asgard8080DebugWrite
#else
	#define READMEM			A8080_READMEM
	#define WRITEMEM		A8080_WRITEMEM
#endif


//###################################################################################################
//
// 	These macros are used to extract the given 8080 register into the specified PPC register
//
//###################################################################################################

#define GET_A(r)			rlwinm	r,rPCAF,24,24,31;
#define GET_B(r)			rlwinm	r,rHLBC,24,24,31;
#define GET_C(r)			rlwinm	r,rHLBC,0,24,31;
#define GET_D(r)			rlwinm	r,rSPDE,24,24,31;
#define GET_E(r)			rlwinm	r,rSPDE,0,24,31;
#define GET_H(r)			rlwinm	r,rHLBC,8,24,31;
#define GET_L(r)			rlwinm	r,rHLBC,16,24,31;
#define GET_AF(r)			rlwinm	r,rPCAF,0,16,31;
#define GET_BC(r)			rlwinm	r,rHLBC,0,16,31;
#define GET_DE(r)			rlwinm	r,rSPDE,0,16,31;
#define GET_HL(r)			rlwinm	r,rHLBC,16,16,31;
#define GET_SP(r)			rlwinm	r,rSPDE,16,16,31;
#define GET_PC(r)			rlwinm	r,rPCAF,16,16,31;


//###################################################################################################
//
// 	These macros are used to set the given 8080 register to the value in the specified PPC register
//
//###################################################################################################

#define SET_A(r)			rlwimi	rPCAF,r,8,16,23;
#define SET_B(r)			rlwimi	rHLBC,r,8,16,23;
#define SET_C(r)			rlwimi	rHLBC,r,0,24,31;
#define SET_D(r)			rlwimi	rSPDE,r,8,16,23;
#define SET_E(r)			rlwimi	rSPDE,r,0,24,31;
#define SET_H(r)			rlwimi	rHLBC,r,24,0,7;
#define SET_L(r)			rlwimi	rHLBC,r,16,8,15;
#define SET_AF(r)			rlwimi	rPCAF,r,0,16,31;
#define SET_BC(r)			rlwimi	rHLBC,r,0,16,31;
#define SET_DE(r)			rlwimi	rSPDE,r,0,16,31;
#define SET_HL(r)			rlwimi	rHLBC,r,16,0,15;
#define SET_SP(r)			rlwimi	rSPDE,r,16,0,15;
#define SET_PC(r)			rlwimi	rPCAF,r,16,0,15;


//###################################################################################################
//
// 	These macros are used to save & load the current 8080 global state during execution
//
//###################################################################################################

// control flags are always saved/loaded
#define SAVE_FLAGS			_asm_set_global(rFlags,sFlags);
#define LOAD_FLAGS			_asm_get_global(rFlags,sFlags);

// the current execution cycle count is always saved/loaded
#define SAVE_ICOUNT			stw		rICount,0(rICountPtr);
#define LOAD_ICOUNT			lwz		rICount,0(rICountPtr);

// the OPCODEROM pointer is loaded only if we do bank switching
#ifdef A8080_UPDATEBANK
	#define LOAD_ROM		lwz		rOpcodeROM,0(rOpcodeROMPtr);\
							lwz		rArgumentROM,0(rArgumentROMPtr);
#else
	#define LOAD_ROM
#endif

// the registers are optionally saved/loaded
#if A8080_VOLATILEREGS
	#define SAVE_PCAF		_asm_set_global(rPCAF,sPCAF);
	#define LOAD_PCAF		_asm_get_global(rPCAF,sPCAF);
	#define SAVE_SPDE		_asm_set_global(rSPDE,sSPDE);
	#define LOAD_SPDE		_asm_get_global(rSPDE,sSPDE);
	#define SAVE_HLBC		_asm_set_global(rHLBC,sHLBC);
	#define LOAD_HLBC		_asm_get_global(rHLBC,sHLBC);
#else
	#define SAVE_PCAF
	#define LOAD_PCAF
	#define SAVE_SPDE
	#define LOAD_SPDE
	#define SAVE_HLBC
	#define LOAD_HLBC
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
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
	lbzx	r,rOpcodeROM,r;			/* r = immediate byte value */

#define READ_OPCODE_ARG(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
	lbzx	r,rArgumentROM,r;		/* r = immediate byte value */


//###################################################################################################
//
// 	Read an opcode argument word from OPCODERAM to 'r' and increment the PC
//
//###################################################################################################

#define READ_OPCODE_ARG_WORD(r) \
	GET_PC(r) 						/* fetch the current PC */\
	addis	rPCAF,rPCAF,2;			/* increment & wrap the PC */\
	lhbrx	r,rArgumentROM,r;		/* r = immediate byte value */


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
	GET_PC(r5)						/* r5 = current PC */\
	SAVE_ICOUNT						/* save ICount */\
	lbzx	r5,rArgumentROM,r5;		/* r5 = immediate byte value */\
	addis	rPCAF,rPCAF,1;			/* increment & wrap the PC */\
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
// 	PUSHes the high or low part of the specified 'reg', optionally saving the registers as well
//
//###################################################################################################

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
extern UINT8		A8080_READMEM(offs_t address);
extern void 		A8080_WRITEMEM(offs_t address, UINT8 value);

// prototype for the debugger hook
#ifdef A8080_DEBUGHOOK
extern void 		A8080_DEBUGHOOK(void);
#endif

// prototype for the bank switcher
#ifdef A8080_UPDATEBANK
extern void 		A8080_UPDATEBANK(int newpc);
#endif

// prototypes for the opcode handlers
static void adc_a(void);
static void adc_b(void);
static void adc_c(void);
static void adc_d(void);
static void adc_e(void);
static void adc_h(void);
static void adc_l(void);
static void adc_m(void);
static void aci(void);
static void add_a(void);
static void add_b(void);
static void add_c(void);
static void add_d(void);
static void add_e(void);
static void add_h(void);
static void add_l(void);
static void add_m(void);
static void adi(void);
static void dad_b(void);
static void dad_d(void);
static void dad_h(void);
static void dad_sp(void);
static void ana_a(void);
static void ana_b(void);
static void ana_c(void);
static void ana_d(void);
static void ana_e(void);
static void ana_h(void);
static void ana_l(void);
static void ana_m(void);
static void ani(void);
static void cc(void);
static void call(void);
static void cm(void);
static void cnc(void);
static void cnz(void);
static void cp(void);
static void cpe(void);
static void cpo(void);
static void cz(void);
static void cmf(void);
static void cmp_a(void);
static void cmp_b(void);
static void cmp_c(void);
static void cmp_d(void);
static void cmp_e(void);
static void cmp_h(void);
static void cmp_l(void);
static void cmp_m(void);
static void cpi(void);
static void cma(void);
static void daa(void);
static void dcr_a(void);
static void dcr_b(void);
static void dcr_c(void);
static void dcr_d(void);
static void dcr_e(void);
static void dcr_h(void);
static void dcr_l(void);
static void dcr_m(void);
static void dcx_b(void);
static void dcx_d(void);
static void dcx_h(void);
static void dcx_sp(void);
static void di(void);
static void ei(void);
static void halt(void);
static void in(void);
static void inr_a(void);
static void inr_b(void);
static void inr_c(void);
static void inr_d(void);
static void inr_e(void);
static void inr_h(void);
static void inr_l(void);
static void inr_m(void);
static void inx_b(void);
static void inx_d(void);
static void inx_h(void);
static void inx_sp(void);
static void jc(void);
static void jmp(void);
static void jm(void);
static void jnc(void);
static void jnz(void);
static void jp(void);
static void jpe(void);
static void jpo(void);
static void jz(void);
static void illegal(void);
static void ldax_b(void);
static void ldax_d(void);
static void ldax(void);
static void lhld(void);
static void lxi_b(void);
static void lxi_d(void);
static void lxi_h(void);
static void lxi_sp(void);
static void mov_a_a(void);
static void mov_a_b(void);
static void mov_a_c(void);
static void mov_a_d(void);
static void mov_a_e(void);
static void mov_a_h(void);
static void mov_a_l(void);
static void mov_a_m(void);
static void mov_b_a(void);
static void mov_b_b(void);
static void mov_b_c(void);
static void mov_b_d(void);
static void mov_b_e(void);
static void mov_b_h(void);
static void mov_b_l(void);
static void mov_b_m(void);
static void mov_c_a(void);
static void mov_c_b(void);
static void mov_c_c(void);
static void mov_c_d(void);
static void mov_c_e(void);
static void mov_c_h(void);
static void mov_c_l(void);
static void mov_c_m(void);
static void mov_d_a(void);
static void mov_d_b(void);
static void mov_d_c(void);
static void mov_d_d(void);
static void mov_d_e(void);
static void mov_d_h(void);
static void mov_d_l(void);
static void mov_d_m(void);
static void mov_e_a(void);
static void mov_e_b(void);
static void mov_e_c(void);
static void mov_e_d(void);
static void mov_e_e(void);
static void mov_e_h(void);
static void mov_e_l(void);
static void mov_e_m(void);
static void mov_h_a(void);
static void mov_h_b(void);
static void mov_h_c(void);
static void mov_h_d(void);
static void mov_h_e(void);
static void mov_h_h(void);
static void mov_h_l(void);
static void mov_h_m(void);
static void mov_l_a(void);
static void mov_l_b(void);
static void mov_l_c(void);
static void mov_l_d(void);
static void mov_l_e(void);
static void mov_l_h(void);
static void mov_l_l(void);
static void mov_l_m(void);
static void mov_m_a(void);
static void mov_m_b(void);
static void mov_m_c(void);
static void mov_m_d(void);
static void mov_m_e(void);
static void mov_m_h(void);
static void mov_m_l(void);
static void mvi_a(void);
static void mvi_b(void);
static void mvi_c(void);
static void mvi_d(void);
static void mvi_e(void);
static void mvi_h(void);
static void mvi_l(void);
static void mvi_m(void);
static void nop(void);
static void ora_a(void);
static void ora_b(void);
static void ora_c(void);
static void ora_d(void);
static void ora_e(void);
static void ora_h(void);
static void ora_l(void);
static void ora_m(void);
static void ori(void);
static void out(void);
static void pchl(void);
static void pop_a(void);
static void pop_b(void);
static void pop_d(void);
static void pop_h(void);
static void push_a(void);
static void push_b(void);
static void push_d(void);
static void push_h(void);
static void ral(void);
static void rar(void);
static void ret(void);
static void rc(void);
static void rm(void);
static void rnc(void);
static void rnz(void);
static void rp(void);
static void rpe(void);
static void rpo(void);
static void rz(void);
static void rim(void);
static void rlc(void);
static void rrc(void);
static void rst_0(void);
static void rst_1(void);
static void rst_2(void);
static void rst_3(void);
static void rst_4(void);
static void rst_5(void);
static void rst_6(void);
static void rst_7(void);
static void sbb_a(void);
static void sbb_b(void);
static void sbb_c(void);
static void sbb_d(void);
static void sbb_e(void);
static void sbb_h(void);
static void sbb_l(void);
static void sbb_m(void);
static void sbi(void);
static void shld(void);
static void sim(void);
static void sphl(void);
static void stax_b(void);
static void stax_d(void);
static void stax(void);
static void stc(void);
static void sub_a(void);
static void sub_b(void);
static void sub_c(void);
static void sub_d(void);
static void sub_e(void);
static void sub_h(void);
static void sub_l(void);
static void sub_m(void);
static void sui(void);
static void xchg(void);
static void xthl(void);
static void xra_a(void);
static void xra_b(void);
static void xra_c(void);
static void xra_d(void);
static void xra_e(void);
static void xra_h(void);
static void xra_l(void);
static void xra_m(void);
static void xri(void);

#endif
