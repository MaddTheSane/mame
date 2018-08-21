//###################################################################################################
//
//
//		Asgard6502Core.c
//		See Asgard6502DefaultConfig.h for compile-time configuration and optimization.
//
//		A PowerPC assembly Rockwell 6502 emulation core written by Aaron Giles
//		This code is free for use in any emulation project as long as the following credit 
//		appears in the about box and documentation:
//
//			PowerPC-optimized 6502 emulation provided by Aaron Giles and the MAME project.
//
//		I would also appreciate a free copy of any project making use of this code.  Please take
//		the time to contact me if you are using this code or if you have any bugs to report;
//		It is quite possible that I have a newer version.  My email address is aaron@aarongiles.com
//
//		This file looks best when viewed with a tab size of 4 characters
//
//
//###################################################################################################
//
//
//		Revision history:
//
//		 3/02/00		1.0		First MAME version
//		 7/07/03		1.1		LBO - added Mach-O and Deco16 support
//		 5/09/04		1.2		LBO - added SO state, internalized 6502 sub-type setting
//
//
//###################################################################################################


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Asgard6502Core.h"
#include "Asgard6502Internals.h"

#if TARGET_RT_MAC_CFM
// we assume TOC-based data; this forces it on
#pragma toc_data on
#endif


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	// these flags indicate actions to be performed on exit from the core
	kExitActionGenerateIRQ		= 0x01,
	kExitActionGenerateNMI		= 0x02
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef struct
{
	// context variables containing the 6502 registers
	unsigned long 			fSAXY;						// S, A, X, Y registers
	unsigned long 			fPCP;						// PC and P registers

	// context variables describing the current 6502 interrupt state
	unsigned char 			fIRQState;					// current state of the IRQ line
	unsigned char 			fNMIState;					// current state of the NMI line
	unsigned char 			fSOState;					// current state of the software overflow
	Asgard6502IRQCallback 	fIRQCallback;				// callback routine for IRQ lines
	signed long				fInterruptCycleAdjust;		// number of cycles to adjust on exit
	int						fSubType;					// 6502 variant of this CPU instance
} Asgard6502Context;


//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// externally defined variables
extern unsigned char *			A6502_OPCODEROM;		// pointer to the ROM base
extern unsigned char *			A6502_ARGUMENTROM;		// pointer to the argument ROM base
extern int						A6502_ICOUNT;			// cycles remaining to execute

// other stuff
static signed long 				sRequestedCycles;		// the originally requested number of cycles
static signed long 				sExitActionCycles;		// number of cycles removed to force an exit action
static unsigned char			sExitActions;			// current exit actions
static unsigned char			sExecuting;				// true if we're currently executing
static unsigned char			sSubType;				// variant of 6502 we're currently emulating

// context variables containing the 6502 registers
static unsigned long 			sSAXY;					// S, A, X, Y registers
static unsigned long 			sPCP;					// PC and P registers
static unsigned long 			sOpcodePC;				// contains the PC of the current opcode

// local variables describing the current 6502 interrupt state
static unsigned char 			sIRQState;				// current state of the IRQ line
static unsigned char 			sNMIState;				// current state of the NMI line
static unsigned char 			sSOState;				// current state of the software overflow
static Asgard6502IRQCallback 	sIRQCallback;			// callback routine for IRQ lines
static signed long				sInterruptCycleAdjust;	// number of cycles to adjust on exit

//###################################################################################################
//	FUNCTION TABLES
//###################################################################################################

#if (A6502_CHIP != 65002 && A6502_CHIP != 658002)
#define bra		illegal
#define ora_zpi	illegal
#define and_zpi	illegal
#define eor_zpi	illegal
#define adc_zpi	illegal
#define sta_zpi	illegal
#define lda_zpi	illegal
#define cmp_zpi	illegal
#define sbc_zpi	illegal
#define tsb_zpg illegal
#define stz_zpg illegal
#define trb_zpg illegal
#define bit_zpx illegal
#define stz_zpx illegal
#define rmb0_zpg illegal
#define rmb1_zpg illegal
#define rmb2_zpg illegal
#define rmb3_zpg illegal
#define rmb4_zpg illegal
#define rmb5_zpg illegal
#define rmb6_zpg illegal
#define rmb7_zpg illegal
#define smb0_zpg illegal
#define smb1_zpg illegal
#define smb2_zpg illegal
#define smb3_zpg illegal
#define smb4_zpg illegal
#define smb5_zpg illegal
#define smb6_zpg illegal
#define smb7_zpg illegal
#define bit_imm illegal
#define ina illegal
#define dea illegal
#define phy illegal
#define ply illegal
#define phx illegal
#define plx illegal
#define tsb_abs illegal
#define trb_abs illegal
#define bit_abx illegal
#define jmp_iax illegal
#define stz_abs illegal
#define stz_abx illegal
#define bbr0_zpg illegal
#define bbr1_zpg illegal
#define bbr2_zpg illegal
#define bbr3_zpg illegal
#define bbr4_zpg illegal
#define bbr5_zpg illegal
#define bbr6_zpg illegal
#define bbr7_zpg illegal
#define bbs0_zpg illegal
#define bbs1_zpg illegal
#define bbs2_zpg illegal
#define bbs3_zpg illegal
#define bbs4_zpg illegal
#define bbs5_zpg illegal
#define bbs6_zpg illegal
#define bbs7_zpg illegal
#endif

#if (A6502_CHIP == DECO16)
static void *sOpcodeTable[0x100] =
{
	brk,		ora_idx,	illegal,	illegal,	illegal,	ora_zpg,	asl_zpg,	illegal,	/* 00 */
	php,		ora_imm,	asl_a,		deco_unk,	illegal,	ora_abs,	asl_abs,	illegal,
	bpl,		ora_idy,	illegal,	deco_unk,	illegal,	ora_zpx,	asl_zpx,	illegal,	/* 10 */
	clc,		ora_aby,	illegal,	illegal,	illegal,	ora_abx,	asl_abx,	illegal,
	jsr,		and_idx,	illegal,	deco_unk,	bit_zpg,	and_zpg,	rol_zpg,	illegal,	/* 20 */
	plp,		and_imm,	rol_a,		illegal,	bit_abs,	and_abs,	rol_abs,	illegal,
	bmi,		and_idy,	illegal,	illegal,	illegal,	and_zpx,	rol_zpx,	illegal,	/* 30 */
	sec,		and_aby,	illegal,	illegal,	illegal,	and_abx,	rol_abx,	deco_unk,
	rti,		eor_idx,	illegal,	illegal,	illegal,	eor_zpg,	lsr_zpg,	illegal,	/* 40 */
	pha,		eor_imm,	lsr_a,		deco_unk,	jmp_abs,	eor_abs,	lsr_abs,	illegal,
	bvc,		eor_idy,	illegal,	illegal,	illegal,	eor_zpx,	lsr_zpx,	illegal,	/* 50 */
	cli,		eor_aby,	illegal,	illegal,	illegal,	eor_abx,	lsr_abx,	illegal,
	rts,		adc_idx,	illegal,	deco_unk,	illegal,	adc_zpg,	ror_zpg,	in_a_byte,	/* 60 */
	pla,		adc_imm,	ror_a,		illegal,	jmp_ind,	adc_abs,	ror_abs,	illegal,
	bvs,		adc_idy,	illegal,	illegal,	illegal,	adc_zpx,	ror_zpx,	illegal,	/* 70 */
	sei,		adc_aby,	illegal,	illegal,	illegal,	adc_abx,	ror_abx,	illegal,
	illegal,	sta_idx,	illegal,	illegal,	sty_zpg,	sta_zpg,	stx_zpg,	deco_unk,	/* 80 */
	dey,		illegal,	txa,		illegal,	sty_abs,	sta_abs,	stx_abs,	out_byte,
	bcc,		sta_idy,	illegal,	illegal,	sty_zpx,	sta_zpx,	stx_zpy,	illegal,	/* 90 */
	tya,		sta_aby,	txs,		illegal,	illegal,	sta_abx,	illegal,	illegal,
	ldy_imm,	lda_idx,	ldx_imm,	deco_unk,	ldy_zpg,	lda_zpg,	ldx_zpg,	illegal,	/* A0 */
	tay,		lda_imm,	tax,		illegal,	ldy_abs,	lda_abs,	ldx_abs,	illegal,
	bcs,		lda_idy,	illegal,	illegal,	ldy_zpx,	lda_zpx,	ldx_zpy,	illegal,	/* B0 */
	clv,		lda_aby,	tsx,		deco_unk,	ldy_abx,	lda_abx,	ldx_aby,	illegal,
	cpy_imm,	cmp_idx,	illegal,	illegal,	cpy_zpg,	cmp_zpg,	dec_zpg,	illegal,	/* C0 */
	iny,		cmp_imm,	dex,		illegal,	cpy_abs,	cmp_abs,	dec_abs,	illegal,
	bne,		cmp_idy,	illegal,	illegal,	illegal,	cmp_zpx,	dec_zpx,	illegal,	/* D0 */
	cld,		cmp_aby,	illegal,	illegal,	illegal,	cmp_abx,	dec_abx,	illegal,
	cpx_imm,	sbc_idx,	illegal,	illegal,	cpx_zpg,	sbc_zpg,	inc_zpg,	illegal,	/* E0 */
	inx,		sbc_imm,	nop,		illegal,	cpx_abs,	sbc_abs,	inc_abs,	illegal,
	beq,		sbc_idy,	illegal,	illegal,	illegal,	sbc_zpx,	inc_zpx,	illegal,	/* F0 */
	sed,		sbc_aby,	illegal,	illegal,	illegal,	sbc_abx,	inc_abx,	illegal
};
#elif (A6502_CHIP != 6510)
static void *sOpcodeTable[0x100] =
{
	brk,		ora_idx,	illegal,	illegal,	tsb_zpg,	ora_zpg,	asl_zpg,	rmb0_zpg,	/* 00 */
	php,		ora_imm,	asl_a,		illegal,	tsb_abs,	ora_abs,	asl_abs,	bbr0_zpg,
	bpl,		ora_idy,	ora_zpi,	illegal,	trb_zpg,	ora_zpx,	asl_zpx,	rmb1_zpg,	/* 10 */
	clc,		ora_aby,	ina,		illegal,	trb_abs,	ora_abx,	asl_abx,	bbr1_zpg,
	jsr,		and_idx,	illegal,	illegal,	bit_zpg,	and_zpg,	rol_zpg,	rmb2_zpg,	/* 20 */
	plp,		and_imm,	rol_a,		illegal,	bit_abs,	and_abs,	rol_abs,	bbr2_zpg,
	bmi,		and_idy,	and_zpi,	illegal,	bit_zpx,	and_zpx,	rol_zpx,	rmb3_zpg,	/* 30 */
	sec,		and_aby,	dea,		illegal,	bit_abx,	and_abx,	rol_abx,	bbr3_zpg,
	rti,		eor_idx,	illegal,	illegal,	illegal,	eor_zpg,	lsr_zpg,	rmb4_zpg,	/* 40 */
	pha,		eor_imm,	lsr_a,		illegal,	jmp_abs,	eor_abs,	lsr_abs,	bbr4_zpg,
	bvc,		eor_idy,	eor_zpi,	illegal,	illegal,	eor_zpx,	lsr_zpx,	rmb5_zpg,	/* 50 */
	cli,		eor_aby,	phy,		illegal,	illegal,	eor_abx,	lsr_abx,	bbr5_zpg,
	rts,		adc_idx,	illegal,	illegal,	stz_zpg,	adc_zpg,	ror_zpg,	rmb6_zpg,	/* 60 */
	pla,		adc_imm,	ror_a,		illegal,	jmp_ind,	adc_abs,	ror_abs,	bbr6_zpg,
	bvs,		adc_idy,	adc_zpi,	illegal,	stz_zpx,	adc_zpx,	ror_zpx,	rmb7_zpg,	/* 70 */
	sei,		adc_aby,	ply,		illegal,	jmp_iax,	adc_abx,	ror_abx,	bbr7_zpg,
	bra,		sta_idx,	illegal,	illegal,	sty_zpg,	sta_zpg,	stx_zpg,	smb0_zpg,	/* 80 */
	dey,		bit_imm,	txa,		illegal,	sty_abs,	sta_abs,	stx_abs,	bbs0_zpg,
	bcc,		sta_idy,	sta_zpi,	illegal,	sty_zpx,	sta_zpx,	stx_zpy,	smb1_zpg,	/* 90 */
	tya,		sta_aby,	txs,		illegal,	stz_abs,	sta_abx,	stz_abx,	bbs1_zpg,
	ldy_imm,	lda_idx,	ldx_imm,	illegal,	ldy_zpg,	lda_zpg,	ldx_zpg,	smb2_zpg,	/* A0 */
	tay,		lda_imm,	tax,		illegal,	ldy_abs,	lda_abs,	ldx_abs,	bbs2_zpg,
	bcs,		lda_idy,	lda_zpi,	illegal,	ldy_zpx,	lda_zpx,	ldx_zpy,	smb3_zpg,	/* B0 */
	clv,		lda_aby,	tsx,		illegal,	ldy_abx,	lda_abx,	ldx_aby,	bbs3_zpg,
	cpy_imm,	cmp_idx,	illegal,	illegal,	cpy_zpg,	cmp_zpg,	dec_zpg,	smb4_zpg,	/* C0 */
	iny,		cmp_imm,	dex,		illegal,	cpy_abs,	cmp_abs,	dec_abs,	bbs4_zpg,
	bne,		cmp_idy,	cmp_zpi,	illegal,	illegal,	cmp_zpx,	dec_zpx,	smb5_zpg,	/* D0 */
	cld,		cmp_aby,	phx,		illegal,	illegal,	cmp_abx,	dec_abx,	bbs5_zpg,
	cpx_imm,	sbc_idx,	illegal,	illegal,	cpx_zpg,	sbc_zpg,	inc_zpg,	smb6_zpg,	/* E0 */
	inx,		sbc_imm,	nop,		illegal,	cpx_abs,	sbc_abs,	inc_abs,	bbs6_zpg,
	beq,		sbc_idy,	sbc_zpi,	illegal,	illegal,	sbc_zpx,	inc_zpx,	smb7_zpg,	/* F0 */
	sed,		sbc_aby,	plx,		illegal,	illegal,	sbc_abx,	inc_abx,	bbs7_zpg
};
#else
static void *sOpcodeTable[0x100] =
{
	brk,		ora_idx,	illegal,	slo_idx,	dop,		ora_zpg,	asl_zpg,	slo_zpg,	/* 00 */
	php,		ora_imm,	asl_a,		anc_imm,	top,		ora_abs,	asl_abs,	slo_abs,
	bpl,		ora_idy,	illegal,	slo_idy,	dop,		ora_zpx,	asl_zpx,	slo_zpx,	/* 10 */
	clc,		ora_aby,	nop,		slo_aby,	top,		ora_abx,	asl_abx,	slo_abx,
	jsr,		and_idx,	illegal,	rla_idx,	bit_zpg,	and_zpg,	rol_zpg,	rla_zpg,	/* 20 */
	plp,		and_imm,	rol_a,		anc_imm,	bit_abs,	and_abs,	rol_abs,	rla_abs,
	bmi,		and_idy,	illegal,	rla_idy,	dop,		and_zpx,	rol_zpx,	rla_zpx,	/* 30 */
	sec,		and_aby,	nop,		rla_aby,	top,		and_abx,	rol_abx,	rla_abx,
	rti,		eor_idx,	illegal,	sre_idx,	dop,		eor_zpg,	lsr_zpg,	sre_zpg,	/* 40 */
	pha,		eor_imm,	lsr_a,		asr_imm,	jmp_abs,	eor_abs,	lsr_abs,	sre_abs,
	bvc,		eor_idy,	illegal,	sre_idy,	dop,		eor_zpx,	lsr_zpx,	sre_zpx,	/* 50 */
	cli,		eor_aby,	nop,		sre_aby,	top,		eor_abx,	lsr_abx,	sre_bx,
	rts,		adc_idx,	illegal,	rra_idx,	dop,		adc_zpg,	ror_zpg,	rra_zpg,	/* 60 */
	pla,		adc_imm,	ror_a,		arr_imm,	jmp_ind,	adc_abs,	ror_abs,	rra_abs,
	bvs,		adc_idy,	illegal,	rra_idy,	dop,		adc_zpx,	ror_zpx,	rra_zpx,	/* 70 */
	sei,		adc_aby,	nop,		rra_aby,	top,		adc_abx,	ror_abx,	rra_abx,
	dop,		sta_idx,	dop,		sax_idx,	sty_zpg,	sta_zpg,	stx_zpg,	sax_zpg,	/* 80 */
	dey,		dop,		txa,		axa_imm,	sty_abs,	sta_abs,	stx_abs,	sax_abs,
	bcc,		sta_idy,	illegal,	sax_idy,	sty_zpx,	sta_zpx,	stx_zpy,	sax_zpx,	/* 90 */
	tya,		sta_aby,	txs,		ssh_aby,	syh_abx,	sta_abx,	illegal,	sax_abx,
	ldy_imm,	lda_idx,	ldx_imm,	lax_idx,	ldy_zpg,	lda_zpg,	ldx_zpg,	lax_zpg,	/* A0 */
	tay,		lda_imm,	tax,		lax_imm,	ldy_abs,	lda_abs,	ldx_abs,	lax_abs,
	bcs,		lda_idy,	illegal,	lax_idy,	ldy_zpx,	lda_zpx,	ldx_zpy,	lax_zpx,	/* B0 */
	clv,		lda_aby,	tsx,		ast_aby,	ldy_abx,	lda_abx,	ldx_aby,	lax_abx,
	cpy_imm,	cmp_idx,	dop,		dcp_idx,	cpy_zpg,	cmp_zpg,	dec_zpg,	dcp_zpg,	/* C0 */
	iny,		cmp_imm,	dex,		asx_imm,	cpy_abs,	cmp_abs,	dec_abs,	dcp_abs,
	bne,		cmp_idy,	illegal,	dcp_idy,	dop,		cmp_zpx,	dec_zpx,	dcp_zpx,	/* D0 */
	cld,		cmp_aby,	nop,		dcp_aby,	top,		cmp_abx,	dec_abx,	dcp_abx,
	cpx_imm,	sbc_idx,	dop,		isb_idx,	cpx_zpg,	sbc_zpg,	inc_zpg,	isb_zpg,	/* E0 */
	inx,		sbc_imm,	nop,		sbc_imm,	cpx_abs,	sbc_abs,	inc_abs,	isb_abs,
	beq,		sbc_idy,	illegal,	isb_idy,	dop,		sbc_zpx,	inc_zpx,	isb_zpx,	/* F0 */
	sed,		sbc_aby,	nop,		isb_aby,	top,		sbc_abx,	inc_abx,	isb_abx
};
#endif


//###################################################################################################
//	INLINE FUNCTIONS
//###################################################################################################

#pragma mark ¥ INLINE FUNCTIONS

//###################################################################################################
//
//	PushByte -- push a byte onto the stack
//
//###################################################################################################

static inline void PushByte(unsigned char inValue)
{
	WRITEMEM((sSAXY >> 24) | 0x100, inValue);
	sSAXY -= 0x01000000;
}


//###################################################################################################
//
//	PushByte -- push a word onto the stack
//
//###################################################################################################

static inline void PushWord(unsigned short inValue)
{
	PushByte(inValue >> 8);
	PushByte(inValue & 0xff);
}


//###################################################################################################
//
//	ProcessInterrupt -- process an interrupt generically
//
//###################################################################################################

static inline void ProcessInterrupt(int inDoCallback, int inNewPC)
{
	int newPC;

	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// call the callback
	if (inDoCallback && sIRQCallback)
		(*sIRQCallback)(0);
	
	// push the current PC and flags
	PushWord(sPCP >> 16);
	PushByte(sPCP & 0xff & ~k6502FlagB);

	// load the vector
#if (A6502_CHIP == DECO16)
	newPC = READMEM(inNewPC + 1);
	newPC |= READMEM(inNewPC) << 8;
#else
	newPC = READMEM(inNewPC);
	newPC |= READMEM(inNewPC + 1) << 8;
#endif

	// set the new PC and flags
	sPCP = (newPC << 16) | (sPCP & 0xff & ~k6502FlagD) | k6502FlagI;
	sInterruptCycleAdjust += 7;
	
#ifdef A6502_UPDATEBANK
	A6502_UPDATEBANK(sPCP >> 16);
#endif
}


//###################################################################################################
//	IMPLEMENTATION
//###################################################################################################

#pragma mark -
#pragma mark ¥ LOCAL FUNCTIONS

//###################################################################################################
//
//	InitTables -- prepare all the internal tables for runtime
//
//	This function dereferences all our opcode function addresses (we don't need to use 
//	proper ptr_glue for these because they're all local, but CodeWarrior won't let us set 
//	up a table of local assembly labels, so we must resort to this hack)
//
//###################################################################################################

static void InitTables(void)
{
#if TARGET_RT_MAC_CFM
	static int done = false;
	int i;

	// only do it once
	if (done)
		return;
	done = true;
	
	// dereferences the opcodes
	for (i = 0; i < 0x100; ++i)
		sOpcodeTable[i] = *(void **)sOpcodeTable[i];
#endif
}


//###################################################################################################
//
//	ProcessIRQ -- process an IRQ interrupt
//
//###################################################################################################

static void ProcessIRQ(void)
{
#if (A6502_CHIP == DECO16)
	ProcessInterrupt(1, 0xfff2);
#else
	ProcessInterrupt(1, 0xfffe);
#endif
}


//###################################################################################################
//
//	ProcessNMI -- process an NMI interrupt
//
//###################################################################################################

static void ProcessNMI(void)
{
#if (A6502_CHIP == DECO16)
	ProcessInterrupt(0, 0xfff4);
#else
	ProcessInterrupt(0, 0xfffa);
#endif
}


//###################################################################################################
//
//	CheckIRQ -- see if we should generante an IRQ now
//
//###################################################################################################

static void CheckIRQ(int inProcessOK)
{
	// check the INTR state last
	if (sIRQState && !(sPCP & k6502FlagI))
	{
		if (!inProcessOK)
		{
			sExitActions |= kExitActionGenerateIRQ;
			sExitActionCycles += A6502_ICOUNT;
			A6502_ICOUNT = 0;
		}
		else
			ProcessIRQ();
	}
}


//###################################################################################################
//
//	SoftwareIRQ -- generate a software IRQ
//
//###################################################################################################

static void SoftwareIRQ(int inProcessOK)
{
	if (!inProcessOK)
	{
		sExitActions |= kExitActionGenerateIRQ;
		sExitActionCycles += A6502_ICOUNT;
		A6502_ICOUNT = 0;
	}
	else
		ProcessIRQ();
}


#pragma mark -
#pragma mark ¥ CORE IMPLEMENTATION

//###################################################################################################
//
//	Asgard6502Init -- set the 6502 state variables
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state variables
//
//###################################################################################################

void Asgard6502Init(void)
{
}

//###################################################################################################
//
//	Asgard6502Reset -- reset the 6502 processor
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state and create the tables
//
//###################################################################################################

void Asgard6502Reset(void)
{
	// reset the PC
#if (A6502_CHIP == DECO16)
	sPCP = READMEM(0xfff1) << 16;
	sPCP |= READMEM(0xfff0) << 24;
#else
	sPCP = READMEM(0xfffc) << 16;
	sPCP |= READMEM(0xfffd) << 24;
#endif
#ifdef A6502_UPDATEBANK
	A6502_UPDATEBANK(sPCP >> 16);
#endif
	sSAXY = 0xff000000;

	// reset the flags
	sPCP |= k6502FlagT | k6502FlagI | k6502FlagZ;

	// reset the interrupt states
	sNMIState = k6502IRQStateClear;
	sIRQState = k6502IRQStateClear;
	sInterruptCycleAdjust = 0;

	// initialize our internal tables
	InitTables();
}


//###################################################################################################
//
//	Asgard6502SetContext -- set the contents of the 6502 registers
//
//	This function can unfortunately be called at any time to change the contents of the
//	6502 registers.  Call Asgard6502GetContext to get the original values before changing them.
//
//###################################################################################################

void Asgard6502SetContext(void *inContext)
{
	// copy the context
	if (inContext)
	{
		Asgard6502Context *context = inContext;
		
		sSAXY = context->fSAXY;
		sPCP = context->fPCP;

		sIRQState = context->fIRQState;
		sNMIState = context->fNMIState;
		sSOState = context->fSOState;
		sIRQCallback = context->fIRQCallback;
		sInterruptCycleAdjust = context->fInterruptCycleAdjust;

		sSubType = context->fSubType;

#ifdef A6502_UPDATEBANK
		A6502_UPDATEBANK(sPCP >> 16);
#endif
	}
}


//###################################################################################################
//
//	Asgard6502SetReg -- set the contents of one 6502 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	6502 register.
//
//###################################################################################################

void Asgard6502SetReg(int inRegisterIndex, unsigned int inValue)
{
	switch (inRegisterIndex)
	{
		case k6502RegisterIndexPC:
			sPCP = (sPCP & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case k6502RegisterIndexS:
			sSAXY = (sSAXY & 0x00ffffff) | ((inValue & 0xff) << 24);
			break;

		case k6502RegisterIndexP:
			sPCP = (sPCP & 0xffffff00) | (inValue & 0xff);
			break;

		case k6502RegisterIndexA:
			sSAXY = (sSAXY & 0xff00ffff) | ((inValue & 0xff) << 16);
			break;

		case k6502RegisterIndexX:
			sSAXY = (sSAXY & 0xffff00ff) | ((inValue & 0xff) << 8);
			break;

		case k6502RegisterIndexY:
			sSAXY = (sSAXY & 0xffffff00) | (inValue & 0xff);
			break;

		case k6502RegisterIndexEA:
			break;

		case k6502RegisterIndexZP:
			break;

		case k6502RegisterIndexIRQState:
			sIRQState = inValue;
			break;

		case k6502RegisterIndexNMIState:
			sNMIState = inValue;
			break;
			
		case k6502RegisterIndexOpcodePC:
			sOpcodePC = inValue;
			break;

		case k6502RegisterIndexSubType:
			sSubType = inValue;
			break;
			
	}
}
		

//###################################################################################################
//
//	Asgard6502GetContext -- examine the contents of the 6502 registers
//
//	This function can unfortunately be called at any time to return the contents of the
//	6502 registers.
//
//###################################################################################################

void Asgard6502GetContext(void *outContext)
{
	// copy the context
	if (outContext)
	{
		Asgard6502Context *context = outContext;
		
		context->fSAXY = sSAXY;
		context->fPCP = sPCP;

		context->fIRQState = sIRQState;
		context->fNMIState = sNMIState;
		context->fSOState = sSOState;
		context->fIRQCallback = sIRQCallback;
		context->fInterruptCycleAdjust = sInterruptCycleAdjust;
		
		context->fSubType = sSubType;
	}
}


//###################################################################################################
//
//	Asgard6502GetReg -- return the contents of one 6502 register
//
//	This function can unfortunately be called at any time to return the contents of a
//	6502 register.
//
//###################################################################################################

unsigned int Asgard6502GetReg(int inRegisterIndex)
{
	switch (inRegisterIndex)
	{
		case k6502RegisterIndexPC:
			return sPCP >> 16;
		
		case k6502RegisterIndexS:
			return sSAXY >> 24;

		case k6502RegisterIndexP:
			return sPCP & 0xff;

		case k6502RegisterIndexA:
			return (sSAXY >> 16) & 0xff;

		case k6502RegisterIndexX:
			return (sSAXY >> 8) & 0xff;

		case k6502RegisterIndexY:
			return sSAXY & 0xff;

		case k6502RegisterIndexEA:
			return 0;

		case k6502RegisterIndexZP:
			return 0;

		case k6502RegisterIndexIRQState:
			return sIRQState;

		case k6502RegisterIndexNMIState:
			return sNMIState;
			
		case k6502RegisterIndexSOState:
			return sSOState;
			
		case k6502RegisterIndexOpcodePC:
			return sOpcodePC;

		case k6502RegisterIndexSubType:
			return sSubType;
	}
	return 0;
}
		

//###################################################################################################
//
//	Asgard6502SetIRQLine -- sets the state of the IRQ/FIRQ lines
//
//###################################################################################################

void Asgard6502SetIRQLine(int inIRQLine, int inState)
{
	if (inIRQLine == INPUT_LINE_NMI)
	{
		// if the state is the same as last time, bail
		if (sNMIState == inState) 
			return;
		sNMIState = inState;

		// detect when the state goes non-clear
		if (inState != k6502IRQStateClear)
		{
			// if we're inside the execution loop, just set the state bit and force us to exit
			if (sExecuting)
			{
				sExitActions |= kExitActionGenerateNMI;
				sExitActionCycles += A6502_ICOUNT;
				A6502_ICOUNT = 0;
			}
		
			// else process it right away
			else
				ProcessNMI();
		}
	}
	else
	{
		if (inIRQLine == M6502_SET_OVERFLOW)
		{
			if (sSOState && !inState)
			{
				// Set the overflow flag
				sPCP |= 0x40;
			}
			sSOState = inState;
			return;
		}
		
		sIRQState = inState;

		// bail if the new state is clear
		if (inState == k6502IRQStateClear)
			return;
		
		// check for IRQs
		CheckIRQ(!sExecuting);
	}
}


//###################################################################################################
//
//	Asgard6502SetIRQCallback -- sets the function to be called when an interrupt is generated
//
//###################################################################################################

void Asgard6502SetIRQCallback(Asgard6502IRQCallback inCallback)
{
	sIRQCallback = inCallback;
}

//###################################################################################################
//
//	Asgard6502GetIRQCallback -- gets the function to be called when an interrupt is generated
//
//###################################################################################################

Asgard6502IRQCallback Asgard6502GetIRQCallback(void)
{
	return sIRQCallback;
}


//###################################################################################################
//
//	Asgard6502Execute -- run the CPU emulation
//
//	This function executes the 6502 for the specified number of cycles, returning the actual
//	number of cycles executed.
//
//###################################################################################################

asm int Asgard6502Execute(register int inCycles)
{
#if (__MWERKS__ >= 0x2300)
	nofralloc
#endif

	//================================================================================================

	//
	// 	standard function prolog; we use almost every nonvolatile register
	//
	mflr	r0
	stmw	rLastNonVolatile,-76(sp)
	stw		r0,8(sp)
	stwu	sp,-128(sp)
	
	//	
	// 	load the cycle count, accounting for any extra interrupt cycles
	//	also, clear the exit action cycle count
	//
//	lwz		r4,sInterruptCycleAdjust(rtoc)
	_asm_get_global(r4,sInterruptCycleAdjust)
	li		r0,0
	sub		rICount,inCycles,r4
//	stw		r0,sInterruptCycleAdjust(rtoc)
//	stw		inCycles,sRequestedCycles(rtoc)
//	stw		r0,sExitActionCycles(rtoc)
	_asm_set_global(r0,sInterruptCycleAdjust)
	_asm_set_global(inCycles,sRequestedCycles)
	_asm_set_global(r0,sExitActionCycles)
	
	//	
	// 	initialize tables & goodies
	//
//	lwz		rArgumentROMPtr,A6502_ARGUMENTROM(rtoc)
//	lwz		rOpcodeROMPtr,A6502_OPCODEROM(rtoc)
//	lwz		rICountPtr,A6502_ICOUNT(rtoc)
//	lwz		rOpcodeTable,sOpcodeTable(rtoc)
	_asm_get_global_ptr(rArgumentROMPtr,A6502_ARGUMENTROM)
	_asm_get_global_ptr(rOpcodeROMPtr,A6502_OPCODEROM)
	_asm_get_global_ptr(rICountPtr,A6502_ICOUNT)
	_asm_get_global_ptr(rOpcodeTable,sOpcodeTable)
	SAVE_ICOUNT
	lwz		rArgumentROM,0(rArgumentROMPtr)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)

	//
	//	restore the state of the machine
	//
//	lwz		rPCP,sPCP(rtoc)
//	lwz		rSAXY,sSAXY(rtoc)
	_asm_get_global(rPCP,sPCP)
	_asm_get_global(rSAXY,sSAXY)
	EI_FLAG_RESET

executeMore:
	//
	//	mark that we're executing and clear the exit actions
	//
	li		r0,1
	li		r9,0
//	stb		r0,sExecuting(rtoc)
//	stb		r9,sExitActions(rtoc)
	_asm_set_global_b(r0,sExecuting)
	_asm_set_global_b(r9,sExitActions)

	//================================================================================================

	//
	// 	this is the heart of the 6502 execution loop; the process is basically this: load an opcode,
	// 	look up the function, and call it
	//
executeLoop:

	//
	//	internal debugging hook
	//
executeLoopEI:
#if A6502_COREDEBUG
	mr		r3,rPCP
	mr		r4,rSAXY
	mr		r5,rICount
	bl		Asgard6502MiniTrace
#endif

	//
	//	external debugging hook
	//
#ifdef A6502_DEBUGHOOK
	_asm_get_global(r3,mame_debug)
#if TARGET_RT_MAC_CFM
	lwz		r3,0(r3)
#endif
	cmpwi	r3,0
	beq		executeLoopNoDebug
//	stw		rPCP,sPCP(rtoc)
//	stw		rSAXY,sSAXY(rtoc)
	_asm_set_global(rPCP,sPCP)
	_asm_set_global(rSAXY,sSAXY)
	stw		rICount,0(rICountPtr)
	bl		A6502_DEBUGHOOK
//	lwz		rPCP,sPCP(rtoc)
//	lwz		rSAXY,sSAXY(rtoc)
	_asm_get_global(rPCP,sPCP)
	_asm_get_global(rSAXY,sSAXY)
	lwz		rICount,0(rICountPtr)
#endif

executeLoopNoDebug:
	//
	//	read the opcode and branch to the appropriate location
	//
	GET_PC(r4)									// fetch the current PC
	lbzx	r3,rOpcodeROM,r4					// load the opcode
	addis	rPCP,rPCP,1							// increment & wrap the PC
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	lwzx	r5,rOpcodeTable,r5					// r5 = rOpcodeTable[r3 << 2]
	li		rCycleCount,0
	mtctr	r5									// ctr = r5
//	stw		r4,sOpcodePC(rtoc)					// store the PC
	_asm_set_global(r4,sOpcodePC)				// save the PC
	SAVE_ICOUNT
	bctr										// go for it

	//
	//	we get back here after any opcode that needs to store a byte result at (rEA)
	//
executeLoopEndWriteEA:
	SAVE_PCP
	rlwinm	r3,rEA,0,16,31						// r3 = address
	bl		WRITEMEM							// perform the write
	bl		postExtern							// post-process

	//
	//	we get back here after any other opcode
	//
executeLoopEnd:
	sub.	rICount,rICount,rCycleCount			// decrement the cycle count
	BRANCH_ON_EI(eiContinue)
	bgt		executeLoop							// loop if we're not done

	//================================================================================================

executeLoopExit:
	//
	// 	add back any exit action cycles and store the final icount
	//
//	lwz		r3,sExitActionCycles(rtoc)
	_asm_get_global(r3,sExitActionCycles)
	li		r0,0
	add		rICount,rICount,r3
//	stw		r0,sExitActionCycles(rtoc)
	_asm_set_global(r0,sExitActionCycles)
	stw		rICount,0(rICountPtr)

	//
	// 	save the final state of the machine
	//
//	stw		rPCP,sPCP(rtoc)
//	stw		rSAXY,sSAXY(rtoc)
	_asm_set_global(rPCP,sPCP)
	_asm_set_global(rSAXY,sSAXY)

	//
	//	mark that we're no longer executing
	//
	li		r0,0
//	stb		r0,sExecuting(rtoc)
	_asm_set_global_b(r0,sExecuting)
	
	//
	//	see if there are interrupts pending
	//
//	lbz		rTempSave,sExitActions(rtoc)
	_asm_get_global_b(rTempSave,sExitActions)
	andi.	r0,rTempSave,kExitActionGenerateNMI
	beq		noPendingNMI
	bl		ProcessNMI
	bl		postExtern
noPendingNMI:
	andi.	r0,rTempSave,kExitActionGenerateIRQ
	beq		noPendingIRQ
	bl		ProcessIRQ
	bl		postExtern
noPendingIRQ:
	
	//
	// 	account for any interrupt cycles and store the final cycle count
	//
//	lwz		r3,sInterruptCycleAdjust(rtoc)
	_asm_get_global(r3,sInterruptCycleAdjust)
	li		r0,0
	sub.	rICount,rICount,r3
//	stw		r0,sInterruptCycleAdjust(rtoc)
	_asm_set_global(r0,sInterruptCycleAdjust)
	stw		rICount,0(rICountPtr)

	//
	//	if there's time left on the clock, try again
	//
	bgt		executeMore

	//
	// 	standard function epilogue; we also compute the number of cycles processed in r3 for return
	//
//	lwz		r3,sRequestedCycles(rtoc)
	_asm_get_global(r3,sRequestedCycles)
	lwz		r0,136(sp)
	addi	sp,sp,128
	mtlr	r0
	sub		r3,r3,rICount
	lmw		rLastNonVolatile,-76(sp)
	blr

	//================================================================================================

	//
	// 	post-external call update: because an external function can modify the ICount, registers,
	//	etc., this function must be called after every external call
	//
postExtern:
	LOAD_PCP
	LOAD_SAXY
	LOAD_ICOUNT									// rICount = sICount
	LOAD_ROM									// reload the ROM pointers
	blr											// return

	//================================================================================================

#ifdef A6502_UPDATEBANK

	//
	//	post-PC change update: make sure the ROM bank hasn't been switched out from under us
	//
updateBank:
	mflr	rTempSave
	GET_PC(r3)
	bl		A6502_UPDATEBANK
	mtlr	rTempSave
	lwz		rOpcodeROM,0(rOpcodeROMPtr)			// restore the ROM pointer
	lwz		rArgumentROM,0(rArgumentROMPtr)		// restore the argument ROM pointer
	blr

#endif

	//================================================================================================

	//
	//		Illegal opcode handler
	//
entry static illegal
	CYCLES(2)
#if MAME_DEBUG
//	rfi
#endif
	b		executeLoopEnd

	//================================================================================================

	//
	//		ADC: add with carry to A
	//
entry static adc_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		adc_common
	
entry static adc_abs
	COMPUTE_EA_ABS(4)
	b		adc_common_mem

entry static adc_abx
	COMPUTE_EA_ABX(4)
	b		adc_common_mem

entry static adc_aby
	COMPUTE_EA_ABY(4)
	b		adc_common_mem

entry static adc_idx
	COMPUTE_EA_IDX(6)
	b		adc_common_mem
	
entry static adc_idy
	COMPUTE_EA_IDY(5)
	b		adc_common_mem

entry static adc_zpx
	COMPUTE_EA_ZPX(4)
	b		adc_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static adc_zpi
	COMPUTE_EA_ZPI(3)
	b		adc_common_mem
#endif

entry static adc_zpg
	COMPUTE_EA_ZPG(3)
adc_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
adc_common:
#if (A6502_CHIP != 2403)
	andi.	r0,rPCP,k6502FlagD		// check D flag
	GET_A(r5)						// r5 = A
	rlwinm	r6,rPCP,0,31,31			// r6 = C
	bne-	adc_common_bcd			// if D flag set, do it with BCD
#else
	GET_A(r5)						// r5 = A
	rlwinm	r6,rPCP,0,31,31			// r6 = C
#endif
	
	add		r4,r5,r3				// r4 = A + val
	xor		r8,r5,r3				// r8 = A ^ val
	add		r4,r4,r6				// r4 = A + val + C
	rlwimi	rPCP,r4,24,31,31		// set C
	xor		r9,r5,r4				// r9 = A ^ res
	SET_A(r4)						// set A
	andc	r8,r9,r8				// r8 = ~(A ^ val) & (A ^ res)
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r8,31,25,25		// set V
	SAVE_SAXY
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEnd

#if (A6502_CHIP != 2403)
adc_common_bcd:
	rlwinm	r9,r3,0,28,31			// r9  = lo(val)
	rlwinm	r11,r5,0,28,31			// r11 = lo(A)
	rlwinm	r10,r3,0,24,27			// r10 = hi(val)
	rlwinm	r12,r5,0,24,27			// r12 = hi(A)
	add		r11,r11,r9				// r11 = lo(A) + lo(val)
	add		r12,r12,r10				// r12 = hi(A) + hi(val)
	add		r11,r11,r6				// r11 = lo(A) + lo(val) + C
	cmpwi	r11,9					// is lo > 9?
	xor		r8,r5,r3				// r8 = A ^ val
	ble		adc_no_overflow_lo		// skip if no overflow in the low nibble
	addi	r12,r12,0x10			// r12 += 0x10
	addi	r11,r11,0x06			// r11 += 0x06
adc_no_overflow_lo:
	cmpwi	r12,0x90				// if hi > 0x90?
	xor		r9,r5,r12				// r9 = A ^ hi
	rlwinm	r4,r11,0,28,31			// r4 = r12 & 0x0f
	ble		adc_no_overflow_hi		// skip if no overflow in the high nibble
	addi	r12,r12,0x60			// r12 += 0x60
	rlwinm	r0,r12,31,23,23			// r0 = (r12 & 0x200) >> 1
	or		r12,r12,r0				// r12 |= (r12 & 0x200) >> 1 (keeps carry in the 0x100 bit)
adc_no_overflow_hi:
	andc	r8,r9,r8				// r8 = ~(A ^ val) & (A ^ res)
	rlwimi	r4,r12,0,24,27			// r4 = (r12 & 0xf0) | (r11 & 0x0f)
	rlwimi	rPCP,r12,24,31,31		// set C
	SET_A(r4)						// set A
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r8,31,25,25		// set V
	SAVE_SAXY
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		AND: logical AND with A
	//
entry static and_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		and_common
	
entry static and_abs
	COMPUTE_EA_ABS(4)
	b		and_common_mem

entry static and_abx
	COMPUTE_EA_ABX(4)
	b		and_common_mem

entry static and_aby
	COMPUTE_EA_ABY(4)
	b		and_common_mem

entry static and_idx
	COMPUTE_EA_IDX(6)
	b		and_common_mem
	
entry static and_idy
	COMPUTE_EA_IDY(5)
	b		and_common_mem

entry static and_zpx
	COMPUTE_EA_ZPX(4)
	b		and_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static and_zpi
	COMPUTE_EA_ZPI(3)
	b		and_common_mem
#endif

entry static and_zpg
	COMPUTE_EA_ZPG(3)
and_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
and_common:
	GET_A(r5)						// r5 = A
	and		r4,r3,r5				// r4 = A & val
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		ASL: arithmetic left shift
	//
entry static asl_a
	GET_A(r4)						// r4 = A
	rlwimi	rPCP,rSAXY,9,31,31		// C = upper bit
	rlwinm	r4,r4,1,24,30			// r4 <<= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static asl_abs
	COMPUTE_EA_ABS(6)
	b		asl_common_mem

entry static asl_abx
	COMPUTE_EA_ABX(7)
	b		asl_common_mem

entry static asl_zpx
	COMPUTE_EA_ZPX(6)
	b		asl_common_mem

entry static asl_zpg
	COMPUTE_EA_ZPG(5)
asl_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
asl_common:
	rlwimi	rPCP,r3,25,31,31		// C = upper bit
	rlwinm	r4,r3,1,24,30			// r4 = r3 << 1
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		BIT: bit test
	//
#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static bit_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		bit_common

entry static bit_abx
	COMPUTE_EA_ABX(4)
	b		bit_common_mem

entry static bit_zpx
	COMPUTE_EA_ZPX(4)
	b		bit_common_mem
#endif

entry static bit_abs
	COMPUTE_EA_ABS(4)
	b		bit_common_mem

entry static bit_zpg
	COMPUTE_EA_ZPG(3)
bit_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
bit_common:
	GET_A(r5)
	and		r4,r3,r5
	rlwimi	rPCP,r3,0,24,25			// set N/V
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEnd

	//================================================================================================

	//
	//		Bcc: conditional branches
	//
#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static bra
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)
	// fall through
#endif

branchTaken:
	GET_PC(r4)						// r4 = PC
	extsb	r3,r3					// r3 = sign-extended offset
	CYCLES(1)						// one more cycle if taken
	add		r3,r3,r4				// r3 = PC + offset
	xor		r4,r4,r3				// r4 = newPC ^ oldPC
	SET_PC(r3)						// set the new PC
	rlwinm	r4,r4,24,31,31			// r4 = ((newPC ^ oldPC) >> 8) & 1
	add		rCycleCount,rCycleCount,r4	// one more cycle if crossing a page boundary
	UPDATE_BANK						// update the bank
	b		executeLoopEnd

entry static bcc
	andi.	r0,rPCP,k6502FlagC		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bcs
	andi.	r0,rPCP,k6502FlagC		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
entry static beq
	andi.	r0,rPCP,k6502FlagZ		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
entry static bmi
	andi.	r0,rPCP,k6502FlagN		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
entry static bne
	andi.	r0,rPCP,k6502FlagZ		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
entry static bpl
	andi.	r0,rPCP,k6502FlagN		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
entry static bvc
	andi.	r0,rPCP,k6502FlagV		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
entry static bvs
	andi.	r0,rPCP,k6502FlagV		// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(2)						// minimum 2 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
	//================================================================================================

	//
	//		BRK: software interrupt
	//
entry static brk
	addis	rPCP,rPCP,1				// increment PC
//	stw		rPCP,sPCP(rtoc)
//	stw		rSAXY,sSAXY(rtoc)
	_asm_set_global(rPCP,sPCP)
	_asm_set_global(rSAXY,sSAXY)
	li		r3,1
	bl		SoftwareIRQ
	li		r0,0
//	lwz		rCycleCount,sInterruptCycleAdjust(rtoc)
//	stw		r0,sInterruptCycleAdjust(rtoc)
	_asm_get_global(rCycleCount,sInterruptCycleAdjust)
	_asm_set_global(r0,sInterruptCycleAdjust)
	bl		postExtern
	b		executeLoopEnd

	//================================================================================================

	//
	//		CLx: flag clearing
	//
entry static clc
	rlwinm	rPCP,rPCP,0,0,30		// clear C
	CYCLES(2)
	SAVE_PCP
	b		executeLoopEnd
	
entry static cld
	rlwinm	rPCP,rPCP,0,29,27		// clear D
	CYCLES(2)
	SAVE_PCP
	b		executeLoopEnd
	
entry static cli
	andi.	r0,rPCP,k6502FlagI
	rlwinm	rPCP,rPCP,0,30,28		// clear I
	CYCLES(2)
	SAVE_PCP
	beq		executeLoop
	sub		rICount,rICount,rCycleCount
	EI_FLAG_SET						// set the EI flag
	SAVE_ICOUNT
	b		executeLoopEI			// and execute the next instruction
	
eiContinue:
	SAVE_ICOUNT
//	stw		rPCP,sPCP(rtoc)
//	stw		rSAXY,sSAXY(rtoc)
	_asm_set_global(rPCP,sPCP)
	_asm_set_global(rSAXY,sSAXY)
	li		r3,1
	bl		CheckIRQ
	li		r0,0
//	lwz		rCycleCount,sInterruptCycleAdjust(rtoc)
//	stw		r0,sInterruptCycleAdjust(rtoc)
	_asm_get_global(rCycleCount,sInterruptCycleAdjust)
	_asm_set_global(r0,sInterruptCycleAdjust)
	bl		postExtern
	EI_FLAG_RESET
	b		executeLoopEnd

entry static clv
	rlwinm	rPCP,rPCP,0,26,24		// clear V
	CYCLES(2)
	SAVE_PCP
	b		executeLoopEnd
	
	//================================================================================================

	//
	//		CMP: compare to A
	//
entry static cmp_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		cmp_common
	
entry static cmp_abs
	COMPUTE_EA_ABS(4)
	b		cmp_common_mem

entry static cmp_abx
	COMPUTE_EA_ABX(4)
	b		cmp_common_mem

entry static cmp_aby
	COMPUTE_EA_ABY(4)
	b		cmp_common_mem

entry static cmp_idx
	COMPUTE_EA_IDX(6)
	b		cmp_common_mem
	
entry static cmp_idy
	COMPUTE_EA_IDY(5)
	b		cmp_common_mem

entry static cmp_zpx
	COMPUTE_EA_ZPX(4)
	b		cmp_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static cmp_zpi
	COMPUTE_EA_ZPI(3)
	b		cmp_common_mem
#endif

entry static cmp_zpg
	COMPUTE_EA_ZPG(3)
cmp_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
cmp_common:
	GET_A(r5)						// r5 = A
	sub		r4,r5,r3				// r4 = A - val
	rlwimi	rPCP,r4,24,31,31		// set C
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r7,28,30,30		// set Z
	xori	rPCP,rPCP,1				// invert the sense of C
	b		executeLoopEnd

	//================================================================================================

	//
	//		CPX: compare to X
	//
entry static cpx_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		cpx_common
	
entry static cpx_abs
	COMPUTE_EA_ABS(4)
	b		cpx_common_mem

entry static cpx_zpg
	COMPUTE_EA_ZPG(3)
cpx_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
cpx_common:
	GET_X(r5)						// r5 = X
	sub		r4,r5,r3				// r4 = X - val
	rlwimi	rPCP,r4,24,31,31		// set C
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r7,28,30,30		// set Z
	xori	rPCP,rPCP,1				// invert the sense of C
	b		executeLoopEnd

	//================================================================================================

	//
	//		CPY: compare to Y
	//
entry static cpy_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		cpy_common
	
entry static cpy_abs
	COMPUTE_EA_ABS(4)
	b		cpy_common_mem

entry static cpy_zpg
	COMPUTE_EA_ZPG(3)
cpy_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
cpy_common:
	GET_Y(r5)						// r5 = Y
	sub		r4,r5,r3				// r4 = X - val
	rlwimi	rPCP,r4,24,31,31		// set C
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r7,28,30,30		// set Z
	xori	rPCP,rPCP,1				// invert the sense of C
	b		executeLoopEnd

	//================================================================================================

	//
	//		DEC: decrement
	//
#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static dea
	GET_A(r4)						// r4 = A
	subi	r4,r4,1					// r4 -= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd
#endif

entry static dex
	GET_X(r4)						// r4 = X
	subi	r4,r4,1					// r4 -= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_X(r4)						// set X
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static dey
	GET_Y(r4)						// r4 = Y
	subi	r4,r4,1					// r4 -= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_Y(r4)						// set Y
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static dec_abs
	COMPUTE_EA_ABS(6)
	b		dec_common_mem

entry static dec_abx
	COMPUTE_EA_ABX(7)
	b		dec_common_mem

entry static dec_zpx
	COMPUTE_EA_ZPX(6)
	b		dec_common_mem

entry static dec_zpg
	COMPUTE_EA_ZPG(5)
dec_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
dec_common:
	subi	r4,r3,1					// r4 = r3 - 1
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		EOR: logical XOR with A
	//
entry static eor_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		eor_common
	
entry static eor_abs
	COMPUTE_EA_ABS(4)
	b		eor_common_mem

entry static eor_abx
	COMPUTE_EA_ABX(4)
	b		eor_common_mem

entry static eor_aby
	COMPUTE_EA_ABY(4)
	b		eor_common_mem

entry static eor_idx
	COMPUTE_EA_IDX(6)
	b		eor_common_mem
	
entry static eor_idy
	COMPUTE_EA_IDY(5)
	b		eor_common_mem

entry static eor_zpx
	COMPUTE_EA_ZPX(4)
	b		eor_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static eor_zpi
	COMPUTE_EA_ZPI(3)
	b		eor_common_mem
#endif

entry static eor_zpg
	COMPUTE_EA_ZPG(3)
eor_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
eor_common:
	GET_A(r5)						// r5 = A
	xor		r4,r3,r5				// r4 = A ^ val
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		INC: increment
	//
#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static ina
	GET_A(r4)						// r4 = A
	addi	r4,r4,1					// r4 += 1
	CYCLES(2)
	rlwinm	r7,r4,0,24,31			// r7 = r4 & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd
#endif

entry static inx
	GET_X(r4)						// r4 = X
	addi	r4,r4,1					// r4 += 1
	CYCLES(2)
	rlwinm	r7,r4,0,24,31			// r7 = r4 & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	SET_X(r4)						// set X
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static iny
	GET_Y(r4)						// r4 = Y
	addi	r4,r4,1					// r4 += 1
	CYCLES(2)
	rlwinm	r7,r4,0,24,31			// r7 = r4 & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	SET_Y(r4)						// set Y
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static inc_abs
	COMPUTE_EA_ABS(6)
	b		inc_common_mem

entry static inc_abx
	COMPUTE_EA_ABX(7)
	b		inc_common_mem

entry static inc_zpx
	COMPUTE_EA_ZPX(6)
	b		inc_common_mem

entry static inc_zpg
	COMPUTE_EA_ZPG(5)
inc_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
inc_common:
	addi	r4,r3,1					// r4 = r3 + 1
	rlwinm	r7,r4,0,24,31			// r7 = r4 & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		JMP/JSR: jumps
	//
entry static jmp_ind
	COMPUTE_EA_IND(5)
	b		jmp_common

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static jmp_iax
	COMPUTE_EA_IAX(2)
	b		jmp_common
#endif

entry static jmp_abs
	COMPUTE_EA_ABS(3)
jmp_common:
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

entry static jsr
	COMPUTE_EA_ABS(6)
	GET_PC(rTempSave)				// get the updated PC				
	subi	rTempSave,rTempSave,1	// we push PC-1
	
	GET_S(r3)						// r3 = S
	rlwinm	r4,rTempSave,24,24,31	// r4 = PC (upper)
	subis	rSAXY,rSAXY,0x100		// post-decrement the stack
	ori		r3,r3,0x100				// r3 = S | 0x100
	SAVE_SAXY
	bl		WRITEMEM				// write the byte
	
	GET_S(r3)						// r3 = S
	rlwinm	r4,rTempSave,0,24,31	// r4 = PC (lower)
	subis	rSAXY,rSAXY,0x100		// post-decrement the stack
	ori		r3,r3,0x100				// r3 = S | 0x100
	SAVE_SAXY
	bl		WRITEMEM				// write the byte
	bl		postExtern
	
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

	//================================================================================================

	//
	//		LDA: load into A
	//
entry static lda_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		lda_common
	
entry static lda_abs
	COMPUTE_EA_ABS(4)
	b		lda_common_mem

entry static lda_abx
	COMPUTE_EA_ABX(4)
	b		lda_common_mem

entry static lda_aby
	COMPUTE_EA_ABY(4)
	b		lda_common_mem

entry static lda_idx
	COMPUTE_EA_IDX(6)
	b		lda_common_mem
	
entry static lda_idy
	COMPUTE_EA_IDY(5)
	b		lda_common_mem

entry static lda_zpx
	COMPUTE_EA_ZPX(4)
	b		lda_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static lda_zpi
	COMPUTE_EA_ZPI(3)
	b		lda_common_mem
#endif

entry static lda_zpg
	COMPUTE_EA_ZPG(3)
lda_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
lda_common:
	rlwinm	r4,r3,0,24,31			// r4 = final value
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		LDX: load into X
	//
entry static ldx_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		ldx_common
	
entry static ldx_abs
	COMPUTE_EA_ABS(4)
	b		ldx_common_mem

entry static ldx_aby
	COMPUTE_EA_ABY(4)
	b		ldx_common_mem

entry static ldx_zpy
	COMPUTE_EA_ZPY(4)
	b		ldx_common_mem

entry static ldx_zpg
	COMPUTE_EA_ZPG(3)
ldx_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
ldx_common:
	rlwinm	r4,r3,0,24,31			// r4 = final value
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_X(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		LDY: load into Y
	//
entry static ldy_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		ldy_common
	
entry static ldy_abs
	COMPUTE_EA_ABS(4)
	b		ldy_common_mem

entry static ldy_abx
	COMPUTE_EA_ABX(4)
	b		ldy_common_mem

entry static ldy_zpx
	COMPUTE_EA_ZPX(4)
	b		ldy_common_mem

entry static ldy_zpg
	COMPUTE_EA_ZPG(3)
ldy_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
ldy_common:
	rlwinm	r4,r3,0,24,31			// r4 = final value
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_Y(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		LSR: logical right shift
	//
entry static lsr_a
	GET_A(r4)						// r4 = A
	rlwimi	rPCP,rSAXY,16,31,31		// C = lower bit
	rlwinm	r4,r4,31,25,31			// r4 >>= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static lsr_abs
	COMPUTE_EA_ABS(6)
	b		lsr_common_mem

entry static lsr_abx
	COMPUTE_EA_ABX(7)
	b		lsr_common_mem

entry static lsr_zpx
	COMPUTE_EA_ZPX(6)
	b		lsr_common_mem

entry static lsr_zpg
	COMPUTE_EA_ZPG(5)
lsr_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
lsr_common:
	rlwimi	rPCP,r3,0,31,31			// C = lower bit
	rlwinm	r4,r3,31,25,31			// r4 >>= 1
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		NOP: no operation
	//
entry static nop
	CYCLES(2)
	b		executeLoopEnd

	//================================================================================================

	//
	//		ORA: logical OR with A
	//
entry static ora_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		ora_common
	
entry static ora_abs
	COMPUTE_EA_ABS(4)
	b		ora_common_mem

entry static ora_abx
	COMPUTE_EA_ABX(4)
	b		ora_common_mem

entry static ora_aby
	COMPUTE_EA_ABY(4)
	b		ora_common_mem

entry static ora_idx
	COMPUTE_EA_IDX(6)
	b		ora_common_mem
	
entry static ora_idy
	COMPUTE_EA_IDY(5)
	b		ora_common_mem

entry static ora_zpx
	COMPUTE_EA_ZPX(4)
	b		ora_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static ora_zpi
	COMPUTE_EA_ZPI(3)
	b		ora_common_mem
#endif

entry static ora_zpg
	COMPUTE_EA_ZPG(3)
ora_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
ora_common:
	GET_A(r5)						// r5 = A
	or		r4,r3,r5				// r4 = A | val
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		PHx: push register
	//
entry static pha
	PUSH_BYTE(rSAXY,16)
	CYCLES(2)
	b		executeLoopEnd

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static phx
	PUSH_BYTE(rSAXY,8)
	CYCLES(3)
	b		executeLoopEnd

entry static phy
	PUSH_BYTE(rSAXY,0)
	CYCLES(3)
	b		executeLoopEnd
#endif

entry static php
	PUSH_BYTE(rPCP,0)
	CYCLES(2)
	b		executeLoopEnd

	//================================================================================================

	//
	//		PLx: pull register
	//
entry static pla
	SAVE_PCP
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwinm	r4,r3,0,24,31			// r4 = final value
	SET_A(r3)						// set A
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SAVE_SAXY						// save the updated stack
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_PCP
	bl		postExtern
	CYCLES(2)
	b		executeLoopEnd

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static plx
	SAVE_PCP
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwinm	r4,r3,0,24,31			// r4 = final value
	SET_X(r3)						// set X
	SAVE_SAXY						// save the updated stack
	SAVE_PCP
	bl		postExtern
	CYCLES(4)
	b		executeLoopEnd

entry static ply
	SAVE_PCP
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwinm	r4,r3,0,24,31			// r4 = final value
	SET_Y(r3)						// set Y
	SAVE_SAXY						// save the updated stack
	bl		postExtern
	CYCLES(4)
	b		executeLoopEnd
#endif

entry static plp
	mr		rTempSave,rPCP
	SAVE_PCP
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwimi	rPCP,r3,0,24,31			// insert P
	SAVE_SAXY						// save the updated stack
	SAVE_PCP
	bl		postExtern
	CYCLES(2)
	xor		rTempSave,rTempSave,rPCP// get the difference in flags

	andi.	r0,rTempSave,k6502FlagI	// if the I flag didn't change
	beq		executeLoopEnd			// return
	andi.	r0,rPCP,k6502FlagI		// if the I flag is set
	bne		executeLoopEnd			// return
	sub		rICount,rICount,rCycleCount
	EI_FLAG_SET						// set the EI flag
	SAVE_ICOUNT
	b		executeLoopEI			// and execute the next instruction

	//================================================================================================

	//
	//		ROL: logical left rotate
	//
entry static rol_a
	GET_A(r4)						// r4 = A
	rlwimi	r4,rPCP,31,0,0			// r4 = A with C
	rlwimi	rPCP,rSAXY,9,31,31		// C = upper bit
	rlwinm	r4,r4,1,24,31			// r4 <<= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static rol_abs
	COMPUTE_EA_ABS(6)
	b		rol_common_mem

entry static rol_abx
	COMPUTE_EA_ABX(7)
	b		rol_common_mem

entry static rol_zpx
	COMPUTE_EA_ZPX(6)
	b		rol_common_mem

entry static rol_zpg
	COMPUTE_EA_ZPG(5)
rol_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
rol_common:
	rlwimi	r3,rPCP,31,0,0			// r3 = val with C
	rlwimi	rPCP,r3,25,31,31		// C = upper bit
	rlwinm	r4,r3,1,24,31			// r4 <<= 1
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		ROR: logical right rotate
	//
entry static ror_a
	GET_A(r4)						// r4 = A
	rlwimi	r4,rPCP,8,23,23			// r4 = A with C
	rlwimi	rPCP,rSAXY,16,31,31		// C = lower bit
	rlwinm	r4,r4,31,24,31			// r4 >>= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static ror_abs
	COMPUTE_EA_ABS(6)
	b		ror_common_mem

entry static ror_abx
	COMPUTE_EA_ABX(7)
	b		ror_common_mem

entry static ror_zpx
	COMPUTE_EA_ZPX(6)
	b		ror_common_mem

entry static ror_zpg
	COMPUTE_EA_ZPG(5)
ror_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
ror_common:
	rlwimi	r3,rPCP,8,23,23			// r3 = val with C
	rlwimi	rPCP,r3,0,31,31			// C = lower bit
	rlwinm	r4,r3,31,24,31			// r4 >>= 1
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		RTI: return from interrupt
	//
entry static rti
	SAVE_PCP						// save PC/P
	CYCLES(6)
	mr		rTempSave,rPCP
	
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwimi	rPCP,r3,0,24,31			// insert P
	
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwimi	rPCP,r3,16,8,15			// insert PC lower
	
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwimi	rPCP,r3,24,0,7			// insert PC upper

	ori		rPCP,rPCP,k6502FlagT	// set the T flag
	SAVE_PCP
	bl		postExtern				// cleanup
	xor		rEA,rTempSave,rPCP		// get the difference in flags
	UPDATE_BANK

	andi.	r0,rEA,k6502FlagI		// if the I flag didn't change
	beq		executeLoopEnd			// return
	andi.	r0,rPCP,k6502FlagI		// if the I flag is set
	bne		executeLoopEnd			// return
	sub		rICount,rICount,rCycleCount
	EI_FLAG_SET						// set the EI flag
	SAVE_ICOUNT
	b		executeLoopEI			// and execute the next instruction

	//================================================================================================

	//
	//		RTS: return from subroutine
	//
entry static rts
	SAVE_PCP						// save PC/P
	CYCLES(6)
	mr		rTempSave,rPCP
	
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwimi	rPCP,r3,16,8,15			// insert PC lower
	
	addis	rSAXY,rSAXY,0x100		// pre-increment the stack
	GET_S(r3)						// extract the stack pointer
	SAVE_SAXY						// save the updated stack
	ori		r3,r3,0x100				// stack is always in the 100-1ff range
	bl		READMEM					// perform the read
	rlwimi	rPCP,r3,24,0,7			// insert PC upper

	addis	rPCP,rPCP,1				// increment the final PC
	SAVE_PCP
	bl		postExtern				// cleanup
	UPDATE_BANK
	b		executeLoopEnd			// return

	//================================================================================================

	//
	//		SBC: subtract with carry from A
	//
entry static sbc_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	b		sbc_common
	
entry static sbc_abs
	COMPUTE_EA_ABS(4)
	b		sbc_common_mem

entry static sbc_abx
	COMPUTE_EA_ABX(4)
	b		sbc_common_mem

entry static sbc_aby
	COMPUTE_EA_ABY(4)
	b		sbc_common_mem

entry static sbc_idx
	COMPUTE_EA_IDX(6)
	b		sbc_common_mem
	
entry static sbc_idy
	COMPUTE_EA_IDY(5)
	b		sbc_common_mem

entry static sbc_zpx
	COMPUTE_EA_ZPX(4)
	b		sbc_common_mem

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static sbc_zpi
	COMPUTE_EA_ZPI(3)
	b		sbc_common_mem
#endif

entry static sbc_zpg
	COMPUTE_EA_ZPG(3)
sbc_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
sbc_common:
#if (A6502_CHIP != 2403)
	andi.	r0,rPCP,k6502FlagD		// check D flag
	rlwinm	r6,rPCP,0,31,31			// r6 = C
	GET_A(r5)						// r5 = A
	xori	r6,r6,1					// r6 = ~C
	bne-	sbc_common_bcd			// if D flag set, do it with BCD
#else
	rlwinm	r6,rPCP,0,31,31			// r6 = C
	GET_A(r5)						// r5 = A
	xori	r6,r6,1					// r6 = ~C
#endif
	
	sub		r4,r5,r3				// r4 = A - val
	xor		r8,r5,r3				// r8 = A ^ val
	sub		r4,r4,r6				// r4 = A - val - C
	rlwimi	rPCP,r4,24,31,31		// set C
	xor		r9,r5,r4				// r9 = A ^ res
	SET_A(r4)						// set A
	xori	rPCP,rPCP,k6502FlagC	// invert C
	and		r8,r9,r8				// r8 = (A ^ val) & (A ^ res)
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r8,31,25,25		// set V
	SAVE_SAXY
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEnd

#if (A6502_CHIP != 2403)
sbc_common_bcd:
	sub		r8,r5,r3				// r8 = A - val
	rlwinm	r9,r3,0,28,31			// r9  = lo(val)
	rlwinm	r11,r5,0,28,31			// r11 = lo(A)
	rlwinm	r10,r3,0,24,27			// r10 = hi(val)
	rlwinm	r12,r5,0,24,27			// r12 = hi(A)
	sub		r8,r8,r6				// r8 = A - val - C
	sub		r11,r11,r9				// r11 = lo(A) - lo(val)
	sub		r12,r12,r10				// r12 = hi(A) - hi(val)
	sub		r11,r11,r6				// r11 = lo(A) - lo(val) + C
	andi.	r0,r11,0xf0				// did lo underflow?
	xor		r9,r5,r8				// r9 = A ^ sum
	xor		r8,r5,r3				// r8 = A ^ val
	beq		sbc_no_underflow_lo		// skip if no underflow in the low nibble
	subi	r11,r11,0x06			// r11 -= 0x06
	andi.	r0,r11,0x80				// did lo underflow again?
	beq		sbc_no_underflow_lo
	subi	r12,r12,0x10			// r12 -= 0x10
sbc_no_underflow_lo:
	andi.	r0,r12,0xf00			// did hi underflow?
	rlwinm	r4,r11,0,28,31			// r4 = r12 & 0x0f
	beq		sbc_no_underflow_hi		// skip if no underflow in the high nibble
	subi	r12,r12,0x60			// r12 -= 0x60
	rlwinm	r0,r12,31,23,23			// r0 = (r12 & 0x200) >> 1
	or		r12,r12,r0				// r12 |= (r12 & 0x200) >> 1 (keeps carry in the 0x100 bit)
sbc_no_underflow_hi:
	and		r8,r9,r8				// r8 = (A ^ val) & (A ^ sum)
	rlwimi	r4,r12,0,24,27			// r4 = (r12 & 0xf0) | (r11 & 0x0f)
	rlwimi	rPCP,r12,24,31,31		// set C
	SET_A(r4)						// set A
	xori	rPCP,rPCP,k6502FlagC	// invert C
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r8,31,25,25		// set V
	SAVE_SAXY
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		SEx: flag clearing
	//
entry static sec
	ori		rPCP,rPCP,k6502FlagC	// set C
	CYCLES(2)
	SAVE_PCP
	b		executeLoopEnd
	
entry static sed
	ori		rPCP,rPCP,k6502FlagD	// set D
	CYCLES(2)
	SAVE_PCP
	b		executeLoopEnd
	
entry static sei
	ori		rPCP,rPCP,k6502FlagI	// set I
	CYCLES(2)
	SAVE_PCP
	b		executeLoopEnd			// and execute the next instruction
	
	//================================================================================================

	//
	//		STA: store from A
	//
entry static sta_abs
	COMPUTE_EA_ABS(4)
	GET_A(r4)
	b		executeLoopEndWriteEA

entry static sta_abx
	COMPUTE_EA_ABX(5)
	GET_A(r4)
	b		executeLoopEndWriteEA

entry static sta_aby
	COMPUTE_EA_ABY(5)
	GET_A(r4)
	b		executeLoopEndWriteEA

entry static sta_idx
	COMPUTE_EA_IDX(6)
	GET_A(r4)
	b		executeLoopEndWriteEA
	
entry static sta_idy
	COMPUTE_EA_IDY(6)
	GET_A(r4)
	b		executeLoopEndWriteEA

entry static sta_zpx
	COMPUTE_EA_ZPX(4)
	GET_A(r4)
	b		executeLoopEndWriteEA

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)
entry static sta_zpi
	COMPUTE_EA_ZPI(4)
	GET_A(r4)
	b		executeLoopEndWriteEA
#endif

entry static sta_zpg
	COMPUTE_EA_ZPG(3)
	GET_A(r4)
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		STX: store from X
	//
entry static stx_abs
	COMPUTE_EA_ABS(5)
	GET_X(r4)
	b		executeLoopEndWriteEA

entry static stx_zpy
	COMPUTE_EA_ZPY(4)
	GET_X(r4)
	b		executeLoopEndWriteEA

entry static stx_zpg
	COMPUTE_EA_ZPG(3)
	GET_X(r4)
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		STY: store from Y
	//
entry static sty_abs
	COMPUTE_EA_ABS(4)
	GET_Y(r4)
	b		executeLoopEndWriteEA

entry static sty_zpx
	COMPUTE_EA_ZPX(4)
	GET_Y(r4)
	b		executeLoopEndWriteEA

entry static sty_zpg
	COMPUTE_EA_ZPG(3)
	GET_Y(r4)
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		Txx: transfer instructions
	//
entry static tax
	GET_A(r4)
	CYCLES(2)
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_X(r4)
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static tay
	GET_A(r4)
	CYCLES(2)
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_Y(r4)
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static tsx
	GET_S(r4)
	CYCLES(2)
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_X(r4)
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static txa
	GET_X(r4)
	CYCLES(2)
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

entry static txs
	rlwimi	rSAXY,rSAXY,16,0,7
	CYCLES(2)
	SAVE_SAXY
	b		executeLoopEnd

entry static tya
	GET_Y(r4)
	CYCLES(2)
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)
	cntlzw	r7,r4
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd


	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//=================================                              =================================
	//=================================     65C02-SPECIFIC OPCODES   =================================
	//=================================                              =================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================

#if (A6502_CHIP == 65002 || A6502_CHIP == 658002)

	//================================================================================================

	//
	//		BBR: branch if bit reset
	//
entry static bbr0_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 0)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr1_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 1)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr2_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 2)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr3_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 3)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr4_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 4)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr5_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 5)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr6_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 6)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbr7_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 7)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	beq		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top
	
	//================================================================================================

	//
	//		BBS: branch if bit set
	//
entry static bbs0_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 0)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs1_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 1)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs2_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 2)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs3_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 3)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs4_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 4)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs5_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 5)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs6_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 6)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

entry static bbs7_zpg
	COMPUTE_EA_ZPG(3)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	andi.	r0,r3,(1 << 7)			// test condition
	READ_OPCODE_ARG(r3)				// fetch branch amount
	CYCLES(7)						// minimum 2+5 cycles if not taken
	bne		branchTaken				// if taken, skip ahead
	b		executeLoopEnd			// otherwise, back to the top

	//================================================================================================

	//
	//		RMBx: reset memory bit
	//
entry static rmb0_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 0) & 0xff
	b		executeLoopEndWriteEA

entry static rmb1_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 1) & 0xff
	b		executeLoopEndWriteEA

entry static rmb2_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 2) & 0xff
	b		executeLoopEndWriteEA

entry static rmb3_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 3) & 0xff
	b		executeLoopEndWriteEA

entry static rmb4_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 4) & 0xff
	b		executeLoopEndWriteEA

entry static rmb5_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 5) & 0xff
	b		executeLoopEndWriteEA

entry static rmb6_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 6) & 0xff
	b		executeLoopEndWriteEA

entry static rmb7_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	andi.	r4,r3,~(1 << 7) & 0xff
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		SMBx: set memory bit
	//
entry static smb0_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 0)
	b		executeLoopEndWriteEA

entry static smb1_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 1)
	b		executeLoopEndWriteEA

entry static smb2_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 2)
	b		executeLoopEndWriteEA

entry static smb3_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 3)
	b		executeLoopEndWriteEA

entry static smb4_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 4)
	b		executeLoopEndWriteEA

entry static smb5_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 5)
	b		executeLoopEndWriteEA

entry static smb6_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 6)
	b		executeLoopEndWriteEA

entry static smb7_zpg
	COMPUTE_EA_ZPG(5)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	ori		r4,r3,(1 << 7)
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		STZ: store zero
	//
entry static stz_abs
	COMPUTE_EA_ABS(4)
	li		r4,0
	b		executeLoopEndWriteEA

entry static stz_abx
	COMPUTE_EA_ABX(5)
	li		r4,0
	b		executeLoopEndWriteEA

entry static stz_zpx
	COMPUTE_EA_ZPX(4)
	li		r4,0
	b		executeLoopEndWriteEA

entry static stz_zpg
	COMPUTE_EA_ZPG(3)
	li		r4,0
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		TRB: test and set bits
	//
entry static trb_abs
	COMPUTE_EA_ABS(3)
	b		trb_common_mem

entry static trb_zpg
	COMPUTE_EA_ZPG(2)
trb_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
trb_common:
	GET_A(r5)						// r5 = A
	and		r6,r3,r5				// r6 = val & A
	andc	r4,r3,r5				// r4 = val & ~A
	cntlzw	r7,r6
	rlwimi	rPCP,r4,0,24,24			// set N
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		TSB: test and set bits
	//
entry static tsb_abs
	COMPUTE_EA_ABS(3)
	b		tsb_common_mem

entry static tsb_zpg
	COMPUTE_EA_ZPG(2)
tsb_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
tsb_common:
	GET_A(r5)						// r5 = A
	and		r6,r3,r5				// r6 = val & A
	or		r4,r3,r5				// r4 = val | A
	cntlzw	r7,r6
	rlwimi	rPCP,r4,0,24,24			// set N
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

#endif


	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//=================================                              =================================
	//=================================     6510-SPECIFIC OPCODES    =================================
	//=================================                              =================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================

#if (A6502_CHIP == 6510)

	//================================================================================================

	//
	//		ANC: logical AND with A, set carry from MSB of A
	//
entry static anc_imm
	READ_OPCODE_ARG(r3)
	CYCLES(2)
	GET_A(r5)						// r5 = A
	and		r4,r3,r5				// r4 = A & val
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	rlwimi	rPCP,r4,25,31,31		// set C from MSB of A
	b		executeLoopEnd

	//================================================================================================

	//
	//		ASR: and then perform logical right shift
	//
entry static asr_imm
	READ_OPCODE_ARG(r3)
	GET_A(r4)						// r4 = A
	CYCLES(2)
	and		r4,r4,r3				// r4 = A & immediate
	rlwimi	rPCP,rSAXY,16,31,31		// C = lower bit
	rlwinm	r4,r4,31,25,31			// r4 >>= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		AST: set A and X to (S & val)
	//
entry static ast_aby
	COMPUTE_EA_ABY(4)
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
	GET_S(r5)
	and		r4,r3,r5				// r4 = S & val
	cntlzw	r7,r4
	SET_S(r4)						// set S
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_X(r4)						// set X
	rlwimi	rPCP,r7,28,30,30		// set Z
	SET_A(r4)						// set A
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		ARR: and then perform logical right rotate
	//
entry static asr_imm
	READ_OPCODE_ARG(r3)
	GET_A(r4)						// r4 = A
	CYCLES(2)
	and		r4,r4,r3				// r4 = A & immediate
	rlwimi	r4,rPCP,8,23,23			// r4 = A with C
	rlwimi	rPCP,rSAXY,16,31,31		// C = lower bit
	rlwinm	r4,r4,31,24,31			// r4 >>= 1
	CYCLES(2)
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		ASX: X = (X & A) - immediate
	//
entry static asx_imm
	GET_A(r6)						// r6 = A
	READ_OPCODE_ARG(r3)
	GET_X(r5)						// r5 = X
	CYCLES(2)
	and		r5,r5,r6				// r5 = X & A
	sub		r4,r5,r3				// r4 = (X & A) - val
	rlwimi	rPCP,r4,24,31,31		// set C
	SET_X(r4)						// set X
	xori	rPCP,rPCP,k6502FlagC	// invert C
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	SAVE_SAXY
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEnd

	//================================================================================================

	//
	//		AXA: set A to (X & immedate)
	//
entry static axa_imm
	READ_OPCODE_ARG(r3)
	GET_X(r5)
	and		r4,r3,r5				// r4 = X & val
	cntlzw	r7,r4
	rlwimi	rPCP,r4,0,24,24			// set N
	SET_A(r4)						// set A
	rlwimi	rPCP,r7,28,30,30		// set Z
	SAVE_SAXY
	b		executeLoopEnd

	//================================================================================================

	//
	//		DCP: decrement and compare
	//
entry static dcp_abs
	COMPUTE_EA_ABS(6)
	b		dcp_common_mem

entry static dcp_abx
	COMPUTE_EA_ABX(7)
	b		dcp_common_mem

entry static dcp_aby
	COMPUTE_EA_ABY(6)
	b		dcp_common_mem

entry static dcp_idx
	COMPUTE_EA_IDX(7)
	b		dcp_common_mem
	
entry static dcp_idy
	COMPUTE_EA_IDY(6)
	b		dcp_common_mem

entry static dcp_zpx
	COMPUTE_EA_ZPX(6)
	b		dcp_common_mem

entry static dcp_zpg
	COMPUTE_EA_ZPG(5)
dcp_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	bl		postExtern
dcp_common:
	subi	r4,r3,1					// r4 = val - 1
	rlwimi	rPCP,r4,24,31,31		// set C
	xori	rPCP,rPCP,k6502FlagC	// invert C
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		DOP: double no-op
	//
entry static dop
	CYCLES(2)
	addis	rPCP,rPCP,1				// increment PC again
	b		executeLoopEnd

	//================================================================================================

	//
	//		ISB: increment and subtract with carry
	//
entry static isb_abs
	COMPUTE_EA_ABS(6)
	b		isb_common_mem

entry static isb_abx
	COMPUTE_EA_ABX(7)
	b		isb_common_mem

entry static isb_aby
	COMPUTE_EA_ABY(6)
	b		isb_common_mem

entry static isb_idx
	COMPUTE_EA_IDX(7)
	b		isb_common_mem
	
entry static isb_idy
	COMPUTE_EA_IDY(6)
	b		isb_common_mem

entry static isb_zpx
	COMPUTE_EA_ZPX(6)
	b		isb_common_mem

entry static isb_zpg
	COMPUTE_EA_ZPG(5)
isb_common_mem:
	rlwinm	r3,rEA,0,16,31
	bl		READMEM
	addi	r3,r3,1
	bl		postExtern
	andi.	r3,r3,0xff
isb_common:
	andi.	r0,rPCP,k6502FlagD		// check D flag
	rlwinm	r6,rPCP,0,31,31			// r6 = C
	GET_A(r5)						// r5 = A
	xori	r6,r6,1					// r6 = ~C
	bne-	isb_common_bcd			// if D flag set, do it with BCD
	
	sub		r4,r5,r3				// r4 = A - val
	xor		r8,r5,r3				// r8 = A ^ val
	sub		r4,r4,r6				// r4 = A - val - C
	rlwimi	rPCP,r4,24,31,31		// set C
	xor		r9,r5,r4				// r9 = A ^ res
	xori	rPCP,rPCP,k6502FlagC	// invert C
	and		r8,r9,r8				// r8 = (A ^ val) & (A ^ res)
	rlwinm	r7,r4,0,24,31			// r7 = res & 0xff
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r7
	rlwimi	rPCP,r8,31,25,25		// set V
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

isb_common_bcd:
	sub		r8,r5,r3				// r8 = A - val
	rlwinm	r9,r3,0,28,31			// r9  = lo(val)
	rlwinm	r11,r5,0,28,31			// r11 = lo(A)
	rlwinm	r10,r3,0,24,27			// r10 = hi(val)
	rlwinm	r12,r5,0,24,27			// r12 = hi(A)
	sub		r8,r8,r6				// r8 = A - val - C
	sub		r11,r11,r9				// r11 = lo(A) - lo(val)
	sub		r12,r12,r10				// r12 = hi(A) - hi(val)
	sub		r11,r11,r6				// r11 = lo(A) - lo(val) + C
	andi.	r0,r11,0xf0				// did lo underflow?
	xor		r9,r5,r8				// r9 = A ^ sum
	xor		r8,r5,r3				// r8 = A ^ val
	beq		isb_no_underflow_lo		// skip if no underflow in the low nibble
	subi	r11,r11,0x06			// r11 -= 0x06
	andi.	r0,r11,0x80				// did lo underflow again?
	beq		isb_no_underflow_lo
	subi	r12,r12,0x10			// r12 -= 0x10
isb_no_underflow_lo:
	andi.	r0,r12,0xf00			// did hi underflow?
	rlwinm	r4,r11,0,28,31			// r4 = r12 & 0x0f
	beq		isb_no_underflow_hi		// skip if no underflow in the high nibble
	subi	r12,r12,0x60			// r12 -= 0x60
	rlwinm	r0,r12,31,23,23			// r0 = (r12 & 0x200) >> 1
	or		r12,r12,r0				// r12 |= (r12 & 0x200) >> 1 (keeps carry in the 0x100 bit)
isb_no_underflow_hi:
	and		r8,r9,r8				// r8 = (A ^ val) & (A ^ sum)
	rlwimi	r4,r12,0,24,27			// r4 = (r12 & 0xf0) | (r11 & 0x0f)
	rlwimi	rPCP,r12,24,31,31		// set C
	xori	rPCP,rPCP,k6502FlagC	// invert C
	rlwimi	rPCP,r4,0,24,24			// set N
	cntlzw	r7,r4
	rlwimi	rPCP,r8,31,25,25		// set V
	rlwimi	rPCP,r7,28,30,30		// set Z
	b		executeLoopEndWriteEA

#endif
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//=================================                              =================================
	//=================================    DECO16-SPECIFIC OPCODES   =================================
	//=================================                              =================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================
	//================================================================================================

#if (A6502_CHIP == DECO16)

entry static in_a_byte
	CYCLES(2)
	READ_OPCODE_ARG(r3)							// read an opcode arg - unused?
	li		r3, 0								// port = 0
	SAVE_PCP
	bl		A6502_READPORT						// perform the read
	bl		postExtern							// post-process
	SET_A(r3)
	b		executeLoopEnd

entry static out_byte
	CYCLES(3)
	READ_OPCODE_ARG(r4)							// value to write to port
	li		r3, 0								// port = 0
	SAVE_PCP
	bl		A6502_WRITEPORT						// perform the write
	bl		postExtern							// post-process
	b		executeLoopEnd

entry static deco_unk
	CYCLES(3)
	READ_OPCODE_ARG(r3)							// read an opcode arg - unused?
	b		executeLoopEnd

#endif

}
