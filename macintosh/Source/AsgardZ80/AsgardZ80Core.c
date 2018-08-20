//###################################################################################################
//
//
//		AsgardZ80Core.c
//		See AsgardZ80DefaultConfig.h for compile-time configuration and optimization.
//
//		A PowerPC assembly Zilog Z80 emulation core written by Aaron Giles
//		This code is free for use in any emulation project as long as the following credit 
//		appears in the about box and documentation:
//
//			PowerPC-optimized Z80 emulation provided by Aaron Giles and the MAME project.
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
//		10/19/97		1.0		First MAME version
//		10/26/97		1.1		Added MAME support for the external previous PC
//		11/14/97		1.2		Fixed some bugs related to changes made in 1.01
//		 2/04/98		1.3		Added 16-bit I/O support for MAME
//		 1/07/99		1.4		LBO - Modified to work with Juergen's headers and the new interrupt system
//		 3/17/99		2.0		Rewrote extensively
//		 7/07/03		2.1		LBO - added Mach-O support
//
//
//###################################################################################################


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "state.h"

#include "AsgardZ80Core.h"
#include "AsgardZ80Internals.h"


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	// these flags indicate actions to be performed on exit from the core
	kExitActionGenerateNMI		= 0x01,
	kExitActionGenerateIRQ		= 0x02
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef struct
{
	// context variables containing the Z80 registers
	unsigned long 			fPCAF;						// PC, A, and F registers
	unsigned long 			fHLBC;						// H, L, B, and C registers
	unsigned long 			fSPDE;						// SP, D, and E registers
	unsigned long 			fIXIY;						// IX and IY reigsters
	unsigned long 			fAFDE2;						// A', F', D', and E' registers
	unsigned long 			fHLBC2;						// H', L', B', and C' registers
	unsigned long 			fFlags;						// internal flags and various

	// context variables describing the current Z80 interrupt state
	unsigned char 			fNMIState;					// current state of the NMI line
	unsigned char 			fIRQState;					// current state of the IRQ line
	unsigned char			fNMINestLevel;				// NMI nesting depth
	AsgardZ80IRQCallback 	fIRQCallback;				// callback routine for IRQ lines
	signed long				fInterruptCycleAdjust;		// number of cycles to adjust on exit

	// context variables describing the daisy chain
	unsigned char			fDaisyCount;				// number of daisy chained devices
	signed char				fDaisyIRQRequest;			// next daisy chain request device, or -1
	signed char 			fDaisyIRQService;			// next daisy chain RETI handling device, or -1
	unsigned char 			fDaisyState[kZ80MaxDaisy];	// current state of each daisy chain device
	AsgardZ80DaisyDevice	fDaisyDevice[kZ80MaxDaisy];	// list of daisy chain device callbacks
} AsgardZ80Context;
	

//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// externally defined variables
extern unsigned char *			AZ80_OPCODEROM;			// pointer to the ROM base
extern unsigned char *			AZ80_ARGUMENTROM;		// pointer to the argument ROM base
extern int						AZ80_ICOUNT;			// cycles remaining to execute

// other stuff
static signed long 				sRequestedCycles;		// the originally requested number of cycles
static signed long 				sExitActionCycles;		// number of cycles removed to force an exit action
static unsigned char			sExitActions;			// current exit actions
static unsigned char			sExecuting;				// true if we're currently executing

// context variables containing the Z80 registers
static unsigned long 			sPCAF;					// PC, A, and F registers
static unsigned long 			sHLBC;					// H, L, B, and C registers
static unsigned long 			sSPDE;					// SP, D, and E registers
static unsigned long 			sIXIY;					// IX and IY reigsters
static unsigned long 			sAFDE2;					// A', F', D', and E' registers
static unsigned long 			sHLBC2;					// H', L', B', and C' registers
static unsigned long 			sFlags;					// internal flags and various
static unsigned long 			sOpcodePC;				// contains the PC of the current opcode

// local variables describing the current Z80 interrupt state
static unsigned char 			sNMIState;				// current state of the NMI line
static unsigned char 			sIRQState;				// current state of the IRQ line
static unsigned char			sNMINestLevel;			// NMI nesting depth
static AsgardZ80IRQCallback 	sIRQCallback;			// callback routine for IRQ lines
static signed long				sInterruptCycleAdjust;	// number of cycles to adjust on exit

// context variables describing the daisy chain
static unsigned char			sDaisyCount;			// number of daisy chained devices
static signed char				sDaisyIRQRequest;		// next daisy chain request device, or -1
static signed char 				sDaisyIRQService;		// next daisy chain RETI handling device, or -1
static unsigned char 			sDaisyState[kZ80MaxDaisy];// current state of each daisy chain device
static AsgardZ80DaisyDevice		sDaisyDevice[kZ80MaxDaisy];// list of daisy chain device callbacks


//###################################################################################################
//	STATIC TABLES
//###################################################################################################

typedef struct
{
	unsigned char		fZSP[0x100];
	unsigned short		fDAA[0x800];
} LookupTables;

static LookupTables sLookupTables =
{
	{ 0 },
	{
		#include "AsgardZ80DAA.h"
	}
};


//###################################################################################################
//	FUNCTION TABLES
//###################################################################################################

typedef struct
{
	void *			fMain[0x100];
	void *			fDDCB[0x100];
	void *			fFDCB[0x100];
	void *			fCB[0x100];
	void *			fDD[0x100];
	void *			fED[0x100];
	void *			fFD[0x100];
} OpcodeTable;

static OpcodeTable sOpcodeTable =
{
	{
		nop,		ld_bc_word,	ld_xbc_a,	inc_bc,		inc_b,		dec_b,		ld_b_byte,	rlca,		/* 00 */
		ex_af_af,	add_hl_bc,	ld_a_xbc,	dec_bc,		inc_c,		dec_c,		ld_c_byte,	rrca,
		djnz,		ld_de_word,	ld_xde_a,	inc_de,		inc_d,		dec_d,		ld_d_byte,	rla,		/* 10 */
		jr,			add_hl_de,	ld_a_xde,	dec_de,		inc_e,		dec_e,		ld_e_byte,	rra,
		jr_nz,		ld_hl_word,	ld_xword_hl,inc_hl,		inc_h,		dec_h,		ld_h_byte,	daa,		/* 20 */
		jr_z,		add_hl_hl,	ld_hl_xword,dec_hl,		inc_l,		dec_l,		ld_l_byte,	cpl,
		jr_nc,		ld_sp_word,	ld_xbyte_a,	inc_sp,		inc_xhl,	dec_xhl,	ld_xhl_byte,scf,		/* 30 */
		jr_c,		add_hl_sp,	ld_a_xbyte,	dec_sp,		inc_a,		dec_a,		ld_a_byte,	ccf,
		ld_b_b,		ld_b_c,		ld_b_d,		ld_b_e,		ld_b_h,		ld_b_l,		ld_b_xhl,	ld_b_a,		/* 40 */
		ld_c_b,		ld_c_c,		ld_c_d,		ld_c_e,		ld_c_h,		ld_c_l,		ld_c_xhl,	ld_c_a,
		ld_d_b,		ld_d_c,		ld_d_d,		ld_d_e,		ld_d_h,		ld_d_l,		ld_d_xhl,	ld_d_a,		/* 50 */
		ld_e_b,		ld_e_c,		ld_e_d,		ld_e_e,		ld_e_h,		ld_e_l,		ld_e_xhl,	ld_e_a,
		ld_h_b,		ld_h_c,		ld_h_d,		ld_h_e,		ld_h_h,		ld_h_l,		ld_h_xhl,	ld_h_a,		/* 60 */
		ld_l_b,		ld_l_c,		ld_l_d,		ld_l_e,		ld_l_h,		ld_l_l,		ld_l_xhl,	ld_l_a,
		ld_xhl_b,	ld_xhl_c,	ld_xhl_d,	ld_xhl_e,	ld_xhl_h,	ld_xhl_l,	halt,		ld_xhl_a,	/* 70 */
		ld_a_b,		ld_a_c,		ld_a_d,		ld_a_e,		ld_a_h,		ld_a_l,		ld_a_xhl,	ld_a_a,
		add_a_b,	add_a_c,	add_a_d,	add_a_e,	add_a_h,	add_a_l,	add_a_xhl,	add_a_a,	/* 80 */
		adc_a_b,	adc_a_c,	adc_a_d,	adc_a_e,	adc_a_h,	adc_a_l,	adc_a_xhl,	adc_a_a,
		sub_b,		sub_c,		sub_d,		sub_e,		sub_h,		sub_l,		sub_xhl,	sub_a,		/* 90 */
		sbc_a_b,	sbc_a_c,	sbc_a_d,	sbc_a_e,	sbc_a_h,	sbc_a_l,	sbc_a_xhl,	sbc_a_a,
		and_b,		and_c,		and_d,		and_e,		and_h,		and_l,		and_xhl,	and_a,		/* a0 */
		xor_b,		xor_c,		xor_d,		xor_e,		xor_h,		xor_l,		xor_xhl,	xor_a,
		or_b,		or_c,		or_d,		or_e,		or_h,		or_l,		or_xhl,		or_a,		/* b0 */
		cp_b,		cp_c,		cp_d,		cp_e,		cp_h,		cp_l,		cp_xhl,		cp_a,
		ret_nz,		pop_bc,		jp_nz,		jp,			call_nz,	push_bc,	add_a_byte,	rst_00,		/* c0 */
		ret_z,		ret,		jp_z,		cb,			call_z,		call,		adc_a_byte,	rst_08,
		ret_nc,		pop_de,		jp_nc,		out_byte_a,	call_nc,	push_de,	sub_byte,	rst_10,		/* d0 */
		ret_c,		exx,		jp_c,		in_a_byte,	call_c,		dd,			sbc_a_byte,	rst_18,
		ret_po,		pop_hl,		jp_po,		ex_xsp_hl,	call_po,	push_hl,	and_byte,	rst_20,		/* e0 */
		ret_pe,		jp_hl,		jp_pe,		ex_de_hl,	call_pe,	ed,			xor_byte,	rst_28,
		ret_p,		pop_af,		jp_p,		di,			call_p,		push_af,	or_byte,	rst_30,		/* f0 */
		ret_m,		ld_sp_hl,	jp_m,		ei,			call_m,		fd,			cp_byte,	rst_38
	},
	{
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rlc_xix,	no_op_xx,	/* 00 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rrc_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rl_xix,		no_op_xx,	/* 10 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rr_xix,		no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	sla_xix,	no_op_xx,	/* 20 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	sra_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	sll_xix,	no_op_xx,	/* 30 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	srl_xix,	no_op_xx,
		bit_0_xix,	bit_0_xix,	bit_0_xix,	bit_0_xix,	bit_0_xix,	bit_0_xix,	bit_0_xix,	bit_0_xix,	/* 40 */
		bit_1_xix,	bit_1_xix,	bit_1_xix,	bit_1_xix,	bit_1_xix,	bit_1_xix,	bit_1_xix,	bit_1_xix,
		bit_2_xix,	bit_2_xix,	bit_2_xix,	bit_2_xix,	bit_2_xix,	bit_2_xix,	bit_2_xix,	bit_2_xix,	/* 50 */
		bit_3_xix,	bit_3_xix,	bit_3_xix,	bit_3_xix,	bit_3_xix,	bit_3_xix,	bit_3_xix,	bit_3_xix,
		bit_4_xix,	bit_4_xix,	bit_4_xix,	bit_4_xix,	bit_4_xix,	bit_4_xix,	bit_4_xix,	bit_4_xix,	/* 60 */
		bit_5_xix,	bit_5_xix,	bit_5_xix,	bit_5_xix,	bit_5_xix,	bit_5_xix,	bit_5_xix,	bit_5_xix,
		bit_6_xix,	bit_6_xix,	bit_6_xix,	bit_6_xix,	bit_6_xix,	bit_6_xix,	bit_6_xix,	bit_6_xix,	/* 70 */
		bit_7_xix,	bit_7_xix,	bit_7_xix,	bit_7_xix,	bit_7_xix,	bit_7_xix,	bit_7_xix,	bit_7_xix,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_0_xix,	no_op_xx,	/* 80 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_1_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_2_xix,	no_op_xx,	/* 90 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_3_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_4_xix,	no_op_xx,	/* a0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_5_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_6_xix,	no_op_xx,	/* b0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_7_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_0_xix,	no_op_xx,	/* c0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_1_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_2_xix,	no_op_xx,	/* d0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_3_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_4_xix,	no_op_xx,	/* e0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_5_xix,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_6_xix,	no_op_xx,	/* f0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_7_xix,	no_op_xx
	},
	{
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rlc_xiy,	no_op_xx,	/* 00 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rrc_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rl_xiy,		no_op_xx,	/* 10 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	rr_xiy,		no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	sla_xiy,	no_op_xx,	/* 20 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	sra_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	sll_xiy,	no_op_xx,	/* 30 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	srl_xiy,	no_op_xx,
		bit_0_xiy,	bit_0_xiy,	bit_0_xiy,	bit_0_xiy,	bit_0_xiy,	bit_0_xiy,	bit_0_xiy,	bit_0_xiy,	/* 40 */
		bit_1_xiy,	bit_1_xiy,	bit_1_xiy,	bit_1_xiy,	bit_1_xiy,	bit_1_xiy,	bit_1_xiy,	bit_1_xiy,
		bit_2_xiy,	bit_2_xiy,	bit_2_xiy,	bit_2_xiy,	bit_2_xiy,	bit_2_xiy,	bit_2_xiy,	bit_2_xiy,	/* 50 */
		bit_3_xiy,	bit_3_xiy,	bit_3_xiy,	bit_3_xiy,	bit_3_xiy,	bit_3_xiy,	bit_3_xiy,	bit_3_xiy,
		bit_4_xiy,	bit_4_xiy,	bit_4_xiy,	bit_4_xiy,	bit_4_xiy,	bit_4_xiy,	bit_4_xiy,	bit_4_xiy,	/* 60 */
		bit_5_xiy,	bit_5_xiy,	bit_5_xiy,	bit_5_xiy,	bit_5_xiy,	bit_5_xiy,	bit_5_xiy,	bit_5_xiy,
		bit_6_xiy,	bit_6_xiy,	bit_6_xiy,	bit_6_xiy,	bit_6_xiy,	bit_6_xiy,	bit_6_xiy,	bit_6_xiy,	/* 70 */
		bit_7_xiy,	bit_7_xiy,	bit_7_xiy,	bit_7_xiy,	bit_7_xiy,	bit_7_xiy,	bit_7_xiy,	bit_7_xiy,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_0_xiy,	no_op_xx,	/* 80 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_1_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_2_xiy,	no_op_xx,	/* 90 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_3_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_4_xiy,	no_op_xx,	/* a0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_5_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_6_xiy,	no_op_xx,	/* b0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	res_7_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_0_xiy,	no_op_xx,	/* c0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_1_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_2_xiy,	no_op_xx,	/* d0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_3_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_4_xiy,	no_op_xx,	/* e0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_5_xiy,	no_op_xx,
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_6_xiy,	no_op_xx,	/* f0 */
		no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	no_op_xx,	set_7_xiy,	no_op_xx
	},
	{
		rlc_b,		rlc_c,		rlc_d,		rlc_e,		rlc_h,		rlc_l,		rlc_xhl,	rlc_a,		/* 00 */
		rrc_b,		rrc_c,		rrc_d,		rrc_e,		rrc_h,		rrc_l,		rrc_xhl,	rrc_a,
		rl_b,		rl_c,		rl_d,		rl_e,		rl_h,		rl_l,		rl_xhl,		rl_a,		/* 10 */
		rr_b,		rr_c,		rr_d,		rr_e,		rr_h,		rr_l,		rr_xhl,		rr_a,
		sla_b,		sla_c,		sla_d,		sla_e,		sla_h,		sla_l,		sla_xhl,	sla_a,		/* 20 */
		sra_b,		sra_c,		sra_d,		sra_e,		sra_h,		sra_l,		sra_xhl,	sra_a,
		sll_b,		sll_c,		sll_d,		sll_e,		sll_h,		sll_l,		sll_xhl,	sll_a,		/* 30 */
		srl_b,		srl_c,		srl_d,		srl_e,		srl_h,		srl_l,		srl_xhl,	srl_a,
		bit_0_b,	bit_0_c,	bit_0_d,	bit_0_e,	bit_0_h,	bit_0_l,	bit_0_xhl,	bit_0_a,	/* 40 */
		bit_1_b,	bit_1_c,	bit_1_d,	bit_1_e,	bit_1_h,	bit_1_l,	bit_1_xhl,	bit_1_a,
		bit_2_b,	bit_2_c,	bit_2_d,	bit_2_e,	bit_2_h,	bit_2_l,	bit_2_xhl,	bit_2_a,	/* 50 */
		bit_3_b,	bit_3_c,	bit_3_d,	bit_3_e,	bit_3_h,	bit_3_l,	bit_3_xhl,	bit_3_a,
		bit_4_b,	bit_4_c,	bit_4_d,	bit_4_e,	bit_4_h,	bit_4_l,	bit_4_xhl,	bit_4_a,	/* 60 */
		bit_5_b,	bit_5_c,	bit_5_d,	bit_5_e,	bit_5_h,	bit_5_l,	bit_5_xhl,	bit_5_a,
		bit_6_b,	bit_6_c,	bit_6_d,	bit_6_e,	bit_6_h,	bit_6_l,	bit_6_xhl,	bit_6_a,	/* 70 */
		bit_7_b,	bit_7_c,	bit_7_d,	bit_7_e,	bit_7_h,	bit_7_l,	bit_7_xhl,	bit_7_a,
		res_0_b,	res_0_c,	res_0_d,	res_0_e,	res_0_h,	res_0_l,	res_0_xhl,	res_0_a,	/* 80 */
		res_1_b,	res_1_c,	res_1_d,	res_1_e,	res_1_h,	res_1_l,	res_1_xhl,	res_1_a,
		res_2_b,	res_2_c,	res_2_d,	res_2_e,	res_2_h,	res_2_l,	res_2_xhl,	res_2_a,	/* 90 */
		res_3_b,	res_3_c,	res_3_d,	res_3_e,	res_3_h,	res_3_l,	res_3_xhl,	res_3_a,
		res_4_b,	res_4_c,	res_4_d,	res_4_e,	res_4_h,	res_4_l,	res_4_xhl,	res_4_a,	/* a0 */
		res_5_b,	res_5_c,	res_5_d,	res_5_e,	res_5_h,	res_5_l,	res_5_xhl,	res_5_a,
		res_6_b,	res_6_c,	res_6_d,	res_6_e,	res_6_h,	res_6_l,	res_6_xhl,	res_6_a,	/* b0 */
		res_7_b,	res_7_c,	res_7_d,	res_7_e,	res_7_h,	res_7_l,	res_7_xhl,	res_7_a,
		set_0_b,	set_0_c,	set_0_d,	set_0_e,	set_0_h,	set_0_l,	set_0_xhl,	set_0_a,	/* c0 */
		set_1_b,	set_1_c,	set_1_d,	set_1_e,	set_1_h,	set_1_l,	set_1_xhl,	set_1_a,
		set_2_b,	set_2_c,	set_2_d,	set_2_e,	set_2_h,	set_2_l,	set_2_xhl,	set_2_a,	/* d0 */
		set_3_b,	set_3_c,	set_3_d,	set_3_e,	set_3_h,	set_3_l,	set_3_xhl,	set_3_a,
		set_4_b,	set_4_c,	set_4_d,	set_4_e,	set_4_h,	set_4_l,	set_4_xhl,	set_4_a,	/* e0 */
		set_5_b,	set_5_c,	set_5_d,	set_5_e,	set_5_h,	set_5_l,	set_5_xhl,	set_5_a,
		set_6_b,	set_6_c,	set_6_d,	set_6_e,	set_6_h,	set_6_l,	set_6_xhl,	set_6_a,	/* f0 */
		set_7_b,	set_7_c,	set_7_d,	set_7_e,	set_7_h,	set_7_l,	set_7_xhl,	set_7_a
	},
	{
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* 00 */
		no_op,		add_ix_bc,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* 10 */
		no_op,		add_ix_de,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		ld_ix_word,	ld_xword_ix,inc_ix,		inc_ixh,	dec_ixh,	ld_ixh_byte,no_op,		/* 20 */
		no_op,		add_ix_ix,	ld_ix_xword,dec_ix,		inc_ixl,	dec_ixl,	ld_ixl_byte,no_op,
		no_op,		no_op,		no_op,		no_op,		inc_xix,	dec_xix,	ld_xix_byte,no_op,		/* 30 */
		no_op,		add_ix_sp,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		ld_b_ixh,	ld_b_ixl,	ld_b_xix,	no_op,		/* 40 */
		no_op,		no_op,		no_op,		no_op,		ld_c_ixh,	ld_c_ixl,	ld_c_xix,	no_op,
		no_op,		no_op,		no_op,		no_op,		ld_d_ixh,	ld_d_ixl,	ld_d_xix,	no_op,		/* 50 */
		no_op,		no_op,		no_op,		no_op,		ld_e_ixh,	ld_e_ixl,	ld_e_xix,	no_op,
		ld_ixh_b,	ld_ixh_c,	ld_ixh_d,	ld_ixh_e,	ld_ixh_ixh,	ld_ixh_ixl,	ld_h_xix,	ld_ixh_a,	/* 60 */
		ld_ixl_b,	ld_ixl_c,	ld_ixl_d,	ld_ixl_e,	ld_ixl_ixh,	ld_ixl_ixl,	ld_l_xix,	ld_ixl_a,
		ld_xix_b,	ld_xix_c,	ld_xix_d,	ld_xix_e,	ld_xix_h,	ld_xix_l,	no_op,		ld_xix_a,	/* 70 */
		no_op,		no_op,		no_op,		no_op,		ld_a_ixh,	ld_a_ixl,	ld_a_xix,	no_op,
		no_op,		no_op,		no_op,		no_op,		add_a_ixh,	add_a_ixl,	add_a_xix,	no_op,		/* 80 */
		no_op,		no_op,		no_op,		no_op,		adc_a_ixh,	adc_a_ixl,	adc_a_xix,	no_op,
		no_op,		no_op,		no_op,		no_op,		sub_ixh,	sub_ixl,	sub_xix,	no_op,		/* 90 */
		no_op,		no_op,		no_op,		no_op,		sbc_a_ixh,	sbc_a_ixl,	sbc_a_xix,	no_op,
		no_op,		no_op,		no_op,		no_op,		and_ixh,	and_ixl,	and_xix,	no_op,		/* a0 */
		no_op,		no_op,		no_op,		no_op,		xor_ixh,	xor_ixl,	xor_xix,	no_op,
		no_op,		no_op,		no_op,		no_op,		or_ixh,		or_ixl,		or_xix,		no_op,		/* b0 */
		no_op,		no_op,		no_op,		no_op,		cp_ixh,		cp_ixl,		cp_xix,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* c0 */
		no_op,		no_op,		no_op,		dd_cb,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* d0 */
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		pop_ix,		no_op,		ex_xsp_ix,	no_op,		push_ix,	no_op,		no_op,		/* e0 */
		no_op,		jp_ix,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* f0 */
		no_op,		ld_sp_ix,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op
	},
	{
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* 00 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* 10 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* 20 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* 30 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		in_b_c,		out_c_b,	sbc_hl_bc,	ld_xword_bc,neg,		retn,		im_0,		ld_i_a,		/* 40 */
		in_c_c,		out_c_c,	adc_hl_bc,	ld_bc_xword,neg,		reti,		im_0,		ld_r_a,
		in_d_c,		out_c_d,	sbc_hl_de,	ld_xword_de,neg,		retn,		im_1,		ld_a_i,		/* 50 */
		in_e_c,		out_c_e,	adc_hl_de,	ld_de_xword,neg,		reti,		im_2,		ld_a_r,
		in_h_c,		out_c_h,	sbc_hl_hl,	ld_xword_hl,neg,		retn,		im_0,		rrd,		/* 60 */
		in_l_c,		out_c_l,	adc_hl_hl,	ld_hl_xword,neg,		reti,		im_0,		rld,
		in_0_c,		out_c_0,	sbc_hl_sp,	ld_xword_sp,neg,		retn,		im_1,		nop,		/* 70 */
		in_a_c,		out_c_a,	adc_hl_sp,	ld_sp_xword,neg,		reti,		im_2,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* 80 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* 90 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		ldi,		cpi,		ini,		outi,		nop,		nop,		nop,		nop,		/* a0 */
		ldd,		cpd,		ind,		outd,		nop,		nop,		nop,		nop,
		ldir,		cpir,		inir,		otir,		nop,		nop,		nop,		nop,		/* b0 */
		lddr,		cpdr,		indr,		otdr,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* c0 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* d0 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* e0 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop,		/* f0 */
		nop,		nop,		nop,		nop,		nop,		nop,		nop,		nop
	},
	{
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* 00 */
		no_op,		add_iy_bc,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* 10 */
		no_op,		add_iy_de,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		ld_iy_word,	ld_xword_iy,inc_iy,		inc_iyh,	dec_iyh,	ld_iyh_byte,no_op,		/* 20 */
		no_op,		add_iy_iy,	ld_iy_xword,dec_iy,		inc_iyl,	dec_iyl,	ld_iyl_byte,no_op,
		no_op,		no_op,		no_op,		no_op,		inc_xiy,	dec_xiy,	ld_xiy_byte,no_op,		/* 30 */
		no_op,		add_iy_sp,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		ld_b_iyh,	ld_b_iyl,	ld_b_xiy,	no_op,		/* 40 */
		no_op,		no_op,		no_op,		no_op,		ld_c_iyh,	ld_c_iyl,	ld_c_xiy,	no_op,
		no_op,		no_op,		no_op,		no_op,		ld_d_iyh,	ld_d_iyl,	ld_d_xiy,	no_op,		/* 50 */
		no_op,		no_op,		no_op,		no_op,		ld_e_iyh,	ld_e_iyl,	ld_e_xiy,	no_op,
		ld_iyh_b,	ld_iyh_c,	ld_iyh_d,	ld_iyh_e,	ld_iyh_iyh,	ld_iyh_iyl,	ld_h_xiy,	ld_iyh_a,	/* 60 */
		ld_iyl_b,	ld_iyl_c,	ld_iyl_d,	ld_iyl_e,	ld_iyl_iyh,	ld_iyl_iyl,	ld_l_xiy,	ld_iyl_a,
		ld_xiy_b,	ld_xiy_c,	ld_xiy_d,	ld_xiy_e,	ld_xiy_h,	ld_xiy_l,	no_op,		ld_xiy_a,	/* 70 */
		no_op,		no_op,		no_op,		no_op,		ld_a_iyh,	ld_a_iyl,	ld_a_xiy,	no_op,
		no_op,		no_op,		no_op,		no_op,		add_a_iyh,	add_a_iyl,	add_a_xiy,	no_op,		/* 80 */
		no_op,		no_op,		no_op,		no_op,		adc_a_iyh,	adc_a_iyl,	adc_a_xiy,	no_op,
		no_op,		no_op,		no_op,		no_op,		sub_iyh,	sub_iyl,	sub_xiy,	no_op,		/* 90 */
		no_op,		no_op,		no_op,		no_op,		sbc_a_iyh,	sbc_a_iyl,	sbc_a_xiy,	no_op,
		no_op,		no_op,		no_op,		no_op,		and_iyh,	and_iyl,	and_xiy,	no_op,		/* a0 */
		no_op,		no_op,		no_op,		no_op,		xor_iyh,	xor_iyl,	xor_xiy,	no_op,
		no_op,		no_op,		no_op,		no_op,		or_iyh,		or_iyl,		or_xiy,		no_op,		/* b0 */
		no_op,		no_op,		no_op,		no_op,		cp_iyh,		cp_iyl,		cp_xiy,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* c0 */
		no_op,		no_op,		no_op,		fd_cb,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* d0 */
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		pop_iy,		no_op,		ex_xsp_iy,	no_op,		push_iy,	no_op,		no_op,		/* e0 */
		no_op,		jp_iy,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,
		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		no_op,		/* f0 */
		no_op,		ld_sp_iy,	no_op,		no_op,		no_op,		no_op,		no_op,		no_op
	}
};


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
	sSPDE -= 0x00010000;
	WRITEMEM(sSPDE >> 16, inValue);
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
	static int done = false;
	int i;

	// only do it once
	if (done)
		return;
	done = true;
	
	// create the ZSP table
	for (i = 0; i < 256; ++i)
	{
		unsigned char zs = 0;
		unsigned char p = 0;

		if (i == 0)
			zs |= kZ80FlagZ;
		if (i & 0x80)
			zs |= kZ80FlagS;
			
		if (i & 1) ++p;
		if (i & 2) ++p;
		if (i & 4) ++p;
		if (i & 8) ++p;
		if (i & 16) ++p;
		if (i & 32) ++p;
		if (i & 64) ++p;
		if (i & 128) ++p;

		sLookupTables.fZSP[i] = zs | ((p & 1) ? 0 : kZ80FlagV);
	}
}


//###################################################################################################
//
//	ProcessIRQ -- process an IRQ interrupt
//
//###################################################################################################

static void ProcessIRQ(void)
{
	int vector;

	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// mark the registers dirty
	sFlags |= kFlagsDirty;
	
	// see if the processor was halted, and unhalt it if so
	if (sFlags & kFlagsHALT)
	{
		sFlags &= ~kFlagsHALT;
		sPCAF += 0x00010000;
	}
	
	// if we're in daisy chain mode, get the vector from the daisy chain
	if (sDaisyCount != 0)
	{
		AsgardZ80DaisyDevice *device;

		// it seems like this should never happen, but if it does, just bail
		if (sDaisyIRQRequest < 0)
			return;
		device = &sDaisyDevice[sDaisyIRQRequest];

		// clear both interrupt flip flops and get the vector
		sFlags &= ~(kFlagsIFF1 | kFlagsIFF2);
		vector = (*device->fEntry)(device->fParameter);
		sDaisyIRQRequest = -1;
	}
	
	// otherwise, call the CPU interface to get the vector
	else
	{
		// clear both interrupt flip flops and get the vector			
		sFlags &= ~(kFlagsIFF1 | kFlagsIFF2);
		vector = (*sIRQCallback)(kZ80IRQLineIRQ);
	}
	
	// switch off the interrupt mode
	switch (sFlags & kFlagsIM)
	{
		// interrupt mode 2: call [Z80.I:databyte]
		case 2:
			vector = (vector & 0xff) | (sFlags & kFlagsI);
			PushWord(sPCAF >> 16);
			sPCAF = (READMEM(vector) << 16) + (READMEM(vector + 1) << 24) + (sPCAF & 0xffff);
			sInterruptCycleAdjust += 19;
			break;
		
		// interrupt mode 1: RST 38h
		case 1:
			PushWord(sPCAF >> 16);
			sPCAF = 0x00380000 + (sPCAF & 0xffff);
			sInterruptCycleAdjust += 11 + 2;				// RST $38 + 2 cycles
			break;
		
		// interrupt mode 0: execute opcode that is placed on the databus
		// we only handle CALLs and JMPs here
		case 0:
			
			// switch off the opcode
			switch (vector & 0xff0000)
			{
				case 0xcd0000:	// CALL
					PushWord(sPCAF >> 16);
					sInterruptCycleAdjust += 5;				// CALL $xxxx + 2 cycles
					
					// fall through...
				case 0xc30000:	// JMP
					sPCAF = (vector << 16) + (sPCAF & 0xffff);
					sInterruptCycleAdjust += 10 + 2;		// JMP $xxxx + 2 cycles
					break;
				
				default:		// assume RST
					PushWord(sPCAF >> 16);
					sPCAF = ((vector & 0x0038) << 16) + (sPCAF & 0xffff);
					sInterruptCycleAdjust += 11 + 2;		// RST $xx + 2 cycles
					break;
			}
	}
	
#ifdef AZ80_UPDATEBANK
	AZ80_UPDATEBANK(sPCAF >> 16);
#endif
}


//###################################################################################################
//
//	CheckIRQ -- see if we should generante an IRQ now
//
//###################################################################################################

static void CheckIRQ(void)
{
	if (sIRQState != kZ80IRQStateClear || sDaisyIRQRequest >= 0)
		ProcessIRQ();
}


//###################################################################################################
//
//	DaisyChainIRQ -- handle daisy chaining to another IRQ
//
//###################################################################################################

static void DaisyChainIRQ(void)
{
	if (sDaisyIRQService >= 0)
	{
		AsgardZ80DaisyDevice *device = &sDaisyDevice[sDaisyIRQService];
		device->fRETI(device->fParameter);
	}
}


//###################################################################################################
//
//	ProcessNMI -- process an NMI interrupt
//
//###################################################################################################

static void ProcessNMI(void)
{
	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// mark the registers dirty
	sFlags |= kFlagsDirty;
	
	// see if the processor was halted, and unhalt it if so
	if (sFlags & kFlagsHALT)
	{
		sFlags &= ~kFlagsHALT;
		sPCAF += 0x00010000;
	}
	
	// log nested NMIs
	++sNMINestLevel;
	
	// IFF1 is cleared to mask interrupts during the NMI
	sFlags &= ~kFlagsIFF1;
	
	PushWord(sPCAF >> 16);
	sPCAF = 0x00660000 + (sPCAF & 0xffff);
	sInterruptCycleAdjust += 11;
}


#pragma mark -
#pragma mark ¥ CORE IMPLEMENTATION

//###################################################################################################
//
//	AsgardZ80Init -- register the state variables
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state.
//
//###################################################################################################

void AsgardZ80Init(void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("z80", cpu, "AF", (UINT16 *) &sPCAF + 1, 1);
	state_save_register_UINT16("z80", cpu, "BC", (UINT16 *) &sHLBC + 1, 1);
	state_save_register_UINT16("z80", cpu, "DE", (UINT16 *) &sSPDE + 1, 1);
	state_save_register_UINT16("z80", cpu, "HL", (UINT16 *) &sHLBC, 1);
	state_save_register_UINT16("z80", cpu, "IX", (UINT16 *) &sIXIY, 1);
	state_save_register_UINT16("z80", cpu, "IY", (UINT16 *) &sIXIY + 1, 1);
	state_save_register_UINT16("z80", cpu, "PC", (UINT16 *) &sPCAF, 1);
	state_save_register_UINT16("z80", cpu, "SP", (UINT16 *) &sSPDE, 1);
	state_save_register_UINT16("z80", cpu, "AF2", (UINT16 *) &sAFDE2, 1);
	state_save_register_UINT16("z80", cpu, "BC2", (UINT16 *) &sHLBC2, 1);
	state_save_register_UINT16("z80", cpu, "DE2", (UINT16 *) &sAFDE2 + 1, 1);
	state_save_register_UINT16("z80", cpu, "HL2", (UINT16 *) &sHLBC2 + 1, 1);
#if 0
	state_save_register_UINT8("z80", cpu, "R", &Z80.R, 1);
	state_save_register_UINT8("z80", cpu, "R2", &Z80.R2, 1);
	state_save_register_UINT8("z80", cpu, "IFF1", &Z80.IFF1, 1);
	state_save_register_UINT8("z80", cpu, "IFF2", &Z80.IFF2, 1);
	state_save_register_UINT8("z80", cpu, "HALT", &Z80.HALT, 1);
	state_save_register_UINT8("z80", cpu, "IM", &Z80.IM, 1);
	state_save_register_UINT8("z80", cpu, "I", &Z80.I, 1);
	state_save_register_UINT8("z80", cpu, "irq_max", &Z80.irq_max, 1);
#endif
	state_save_register_INT8("z80", cpu, "request_irq", &sDaisyIRQRequest, 1);
	state_save_register_INT8("z80", cpu, "service_irq", &sDaisyIRQService, 1);
//	state_save_register_UINT8("z80", cpu, "int_state", Z80.int_state, 4);
	state_save_register_UINT8("z80", cpu, "nmi_state", &sNMIState, 1);
	state_save_register_UINT8("z80", cpu, "irq_state", &sIRQState, 1);
	/* daisy chain needs to be saved by z80ctc.c somehow */
}

//###################################################################################################
//
//	AsgardZ80Reset -- reset the Z80 processor
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state and create the tables
//
//###################################################################################################

void AsgardZ80Reset(AsgardZ80DaisyDevice *inDaisyList)
{
	// this reset is compatible with the MAME Z80 core, which has been
	// partially verified on a real Z80
	sPCAF = 0x00000000 | kZ80FlagZ;
#ifdef AZ80_UPDATEBANK
	AZ80_UPDATEBANK(sPCAF >> 16);
#endif
	sHLBC = 0x00000000;
	sSPDE = 0x00000000;
	sIXIY = 0xffffffff;
	sAFDE2 = 0x00000000;
	sHLBC2 = 0x00000000;
	sFlags = 0;
	
	// reset the interrupt states
	sIRQCallback = NULL;
	sNMINestLevel = 0;
	sInterruptCycleAdjust = 0;
	AsgardZ80SetIRQLine(INPUT_LINE_NMI, kZ80IRQStateClear);
	AsgardZ80SetIRQLine(kZ80IRQLineIRQ, kZ80IRQStateClear);

	// reset the daisy chain
	sDaisyCount = 0;
	sDaisyIRQRequest = -1;
	sDaisyIRQService = -1;
	memset(sDaisyState, 0, sizeof(sDaisyState));

	// parse the daisy chain list
	if (inDaisyList)
	{
		while (inDaisyList->fParameter != -1 && sDaisyCount < kZ80MaxDaisy)
		{
			AsgardZ80DaisyDevice *device = &sDaisyDevice[sDaisyCount];
			
			// copy in this item
			*device = *inDaisyList;
			
			// reset the device
			if (device->fReset)
				(*device->fReset)(device->fParameter);
			
			// advance
			sDaisyCount++;
			inDaisyList++;
		}
	}
	
	// initialize our internal tables
	InitTables();
}


//###################################################################################################
//
//	AsgardZ80SetContext -- set the contents of the Z80 registers
//
//	This function can unfortunately be called at any time to change the contents of the
//	Z80 registers.  Call AsgardZ80GetContext to get the original values before changing them.
//
//###################################################################################################

void AsgardZ80SetContext(void *inContext)
{
	// copy the context
	if (inContext)
	{
		AsgardZ80Context *context = inContext;
		int i;
		
		sPCAF = context->fPCAF;
		sHLBC = context->fHLBC;
		sSPDE = context->fSPDE;
		sIXIY = context->fIXIY;
		sAFDE2 = context->fAFDE2;
		sHLBC2 = context->fHLBC2;
		sFlags = context->fFlags | kFlagsDirty;

		sNMIState = context->fNMIState;
		sIRQState = context->fIRQState;
		sNMINestLevel = context->fNMINestLevel;
		sIRQCallback = context->fIRQCallback;
		sInterruptCycleAdjust = context->fInterruptCycleAdjust;

		sDaisyCount = context->fDaisyCount;
		sDaisyIRQRequest = context->fDaisyIRQRequest;
		sDaisyIRQService = context->fDaisyIRQService;
		for (i = 0; i < kZ80MaxDaisy; i++)
		{
			sDaisyState[i] = context->fDaisyState[i];
			sDaisyDevice[i] = context->fDaisyDevice[i];
		}

#ifdef AZ80_UPDATEBANK
		AZ80_UPDATEBANK(sPCAF >> 16);
#endif
	}
}


//###################################################################################################
//
//	AsgardZ80SetReg -- set the contents of one Z80 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	Z80 register.
//
//###################################################################################################

void AsgardZ80SetReg(int inRegisterIndex, unsigned int inValue)
{
	sFlags |= kFlagsDirty;
	
	switch (inRegisterIndex)
	{
		case kZ80RegisterIndexAF:
			sPCAF = (sPCAF & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case kZ80RegisterIndexBC:
			sHLBC = (sHLBC & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case kZ80RegisterIndexDE:
			sSPDE = (sSPDE & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case kZ80RegisterIndexHL:
			sHLBC = (sHLBC & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;

		case kZ80RegisterIndexSP:
			sSPDE = (sSPDE & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;

		case kZ80RegisterIndexPC:
			sPCAF = (sPCAF & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case kZ80RegisterIndexIX:
			sIXIY = (sIXIY & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
			
		case kZ80RegisterIndexIY:
			sIXIY = (sIXIY & 0xffff0000) | (inValue & 0xffff);
			break;
		
		case kZ80RegisterIndexAF2:
			sIXIY = (sAFDE2 & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
			
		case kZ80RegisterIndexBC2:
			sHLBC = (sHLBC2 & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case kZ80RegisterIndexDE2:
			sSPDE = (sAFDE2 & 0xffff0000) | (inValue & 0xffff);
			break;
			
		case kZ80RegisterIndexHL2:
			sHLBC = (sHLBC2 & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
			
		case kZ80RegisterIndexR:
			sFlags = __rlwimi(sFlags, inValue, INSERT_R);
			break;
			
		case kZ80RegisterIndexR2:
			sFlags = __rlwimi(sFlags, inValue, INSERT_R2);
			break;
			
		case kZ80RegisterIndexI:
			sFlags = __rlwimi(sFlags, inValue, INSERT_I);
			break;
			
		case kZ80RegisterIndexIM:
			sFlags = __rlwimi(sFlags, inValue, INSERT_IM);
			break;
			
		case kZ80RegisterIndexIFF1:
			sFlags = __rlwimi(sFlags, inValue, INSERT_IFF1);
			break;
			
		case kZ80RegisterIndexIFF2:
			sFlags = __rlwimi(sFlags, inValue, INSERT_IFF2);
			break;
			
		case kZ80RegisterIndexHALT:
			sFlags = __rlwimi(sFlags, inValue, INSERT_HALT);
			break;
		
		case kZ80RegisterIndexNMIState:
			sNMIState = inValue;
			break;
			
		case kZ80RegisterIndexIRQState:
			sIRQState = inValue;
			break;
			
		case kZ80RegisterIndexDaisyIntState0:
			sDaisyState[0] = inValue;
			break;
			
		case kZ80RegisterIndexDaisyIntState1:
			sDaisyState[1] = inValue;
			break;
			
		case kZ80RegisterIndexDaisyIntState2:
			sDaisyState[2] = inValue;
			break;
			
		case kZ80RegisterIndexDaisyIntState3:
			sDaisyState[3] = inValue;
			break;
		
		case kZ80RegisterIndexNMINestLevel:
			sNMINestLevel = inValue;
			break;
			
		case kZ80RegisterIndexOpcodePC:
			sOpcodePC = inValue;
			break;
			
#if MAME_DEBUG
		default:
			Debugger();
			break;
#endif
	}
}
		

//###################################################################################################
//
//	AsgardZ80GetContext -- examine the contents of the Z80 registers
//
//	This function can unfortunately be called at any time to return the contents of the
//	Z80 registers.
//
//###################################################################################################

unsigned int AsgardZ80GetContext(void *outContext)
{
	// copy the context
	if (outContext)
	{
		AsgardZ80Context *context = outContext;
		int i;
		
		context->fPCAF = sPCAF;
		context->fHLBC = sHLBC;
		context->fSPDE = sSPDE;
		context->fIXIY = sIXIY;
		context->fAFDE2 = sAFDE2;
		context->fHLBC2 = sHLBC2;
		context->fFlags = sFlags;

		context->fNMIState = sNMIState;
		context->fIRQState = sIRQState;
		context->fNMINestLevel = sNMINestLevel;
		context->fIRQCallback = sIRQCallback;
		context->fInterruptCycleAdjust = sInterruptCycleAdjust;

		context->fDaisyCount = sDaisyCount;
		context->fDaisyIRQRequest = sDaisyIRQRequest;
		context->fDaisyIRQService = sDaisyIRQService;
		for (i = 0; i < kZ80MaxDaisy; i++)
		{
			context->fDaisyState[i] = sDaisyState[i];
			context->fDaisyDevice[i] = sDaisyDevice[i];
		}
	}
	
	// return the size
	return sizeof(AsgardZ80Context);
}


//###################################################################################################
//
//	AsgardZ80GetReg -- return the contents of one Z80 register
//
//	This function can unfortunately be called at any time to return the contents of a
//	Z80 register.
//
//###################################################################################################

unsigned int AsgardZ80GetReg(int inRegisterIndex)
{
	switch (inRegisterIndex)
	{
		case kZ80RegisterIndexAF:
			return sPCAF & 0xffff;
			
		case kZ80RegisterIndexBC:
			return sHLBC & 0xffff;
			
		case kZ80RegisterIndexDE:
			return sSPDE & 0xffff;
			
		case kZ80RegisterIndexHL:
			return sHLBC >> 16;
			
		case kZ80RegisterIndexSP:
			return sSPDE >> 16;

		case kZ80RegisterIndexPC:
			return sPCAF >> 16;
		
		case kZ80RegisterIndexIX:
			return sIXIY >> 16;
			
		case kZ80RegisterIndexIY:
			return sIXIY & 0xffff;
		
		case kZ80RegisterIndexAF2:
			return sAFDE2 & 0xffff;
			
		case kZ80RegisterIndexBC2:
			return sHLBC2 & 0xffff;
			
		case kZ80RegisterIndexDE2:
			return sAFDE2 & 0xffff;
			
		case kZ80RegisterIndexHL2:
			return sHLBC2 >> 16;
			
		case kZ80RegisterIndexR:
			return __rlwinm(sFlags, EXTRACT_R);
			
		case kZ80RegisterIndexR2:
			return __rlwinm(sFlags, EXTRACT_R2);
			
		case kZ80RegisterIndexI:
			return __rlwinm(sFlags, EXTRACT_I);
			
		case kZ80RegisterIndexIM:
			return __rlwinm(sFlags, EXTRACT_IM);
			
		case kZ80RegisterIndexIFF1:
			return __rlwinm(sFlags, EXTRACT_IFF1);
			
		case kZ80RegisterIndexIFF2:
			return __rlwinm(sFlags, EXTRACT_IFF2);
			
		case kZ80RegisterIndexHALT:
			return __rlwinm(sFlags, EXTRACT_HALT);
		
		case kZ80RegisterIndexNMIState:
			return sNMIState;
			
		case kZ80RegisterIndexIRQState:
			return sIRQState;
			
		case kZ80RegisterIndexDaisyIntState0:
			return sDaisyState[0];
			
		case kZ80RegisterIndexDaisyIntState1:
			return sDaisyState[1];
			
		case kZ80RegisterIndexDaisyIntState2:
			return sDaisyState[2];
			
		case kZ80RegisterIndexDaisyIntState3:
			return sDaisyState[3];
		
		case kZ80RegisterIndexDaisyIntCount:
			return sDaisyCount;
		
		case kZ80RegisterIndexNMINestLevel:
			return sNMINestLevel;
		
		case kZ80RegisterIndexOpcodePC:
			return sOpcodePC;

#if MAME_DEBUG
		default:
			Debugger();
			break;
#endif
	}
	
	return 0;
}
		

//###################################################################################################
//
//	AsgardZ80SetIRQLine -- sets the state of the IRQ/FIRQ lines
//
//###################################################################################################

void AsgardZ80SetIRQLine(int inIRQLine, int inState)
{
	if (inIRQLine == INPUT_LINE_NMI)
	{
		// if the state is the same as last time, bail
		if (sNMIState == inState) 
			return;
		sNMIState = inState;

		// detect when the state goes non-clear
		if (inState != kZ80IRQStateClear)
		{
			// if we're inside the execution loop, just set the state bit and force us to exit
			if (sExecuting)
			{
				sExitActions |= kExitActionGenerateNMI;
				sExitActionCycles += AZ80_ICOUNT;
				AZ80_ICOUNT = 0;
			}
		
			// else process it right away
			else
				ProcessNMI();
		}
		return;
	}

	// set the state and quit if clear
	sIRQState = inState;
	if (sIRQState == kZ80IRQStateClear)
		return;

	// if we're daisy chaining, do the hard work
	if (sDaisyCount != 0)
	{
		int device, interruptState;
		
		// call the generic handler to get the device ID
		interruptState = (*sIRQCallback)(inIRQLine);
		
		// extract it
		device = interruptState >> 8;
		interruptState &= 0xff;

		// only process further if the resulting state is an actual change
		if (sDaisyState[device] == interruptState)
			return;

		// set the new state and reset the daisy chain IRQ states
		sDaisyState[device] = interruptState;
		sDaisyIRQRequest = sDaisyIRQService = -1;

        // search the device list for either IEO or REQ states
		for (device = 0; device < sDaisyCount; device++)
		{
			// if this device's state has IEO set, disable any earlier interrupts
			if (sDaisyState[device] & kZ80DaisyIRQStateIEO)
			{
				sDaisyIRQRequest = -1;
				sDaisyIRQService = device;
			}
			
			// if this device's state has REQ set, request an interrupt there
			if (sDaisyState[device] & kZ80DaisyIRQStateREQ)
				sDaisyIRQRequest = device;
		}

		// bail if we didn't get any requests
		if (sDaisyIRQRequest < 0)
			return;
	}

	// make sure we're allowed to do this
	if ((sFlags & kFlagsIFF1) == 0)
		return;

	// if we're inside the execution loop, just set the state bit and force us to exit
	if (sExecuting)
	{
		sExitActions |= kExitActionGenerateIRQ;
		sExitActionCycles += AZ80_ICOUNT;
		AZ80_ICOUNT = 0;
	}
	
	// else process it right away
	else
		ProcessIRQ();
}


//###################################################################################################
//
//	AsgardZ80SetIRQCallback -- sets the function to be called when an interrupt is generated
//
//###################################################################################################

void AsgardZ80SetIRQCallback(AsgardZ80IRQCallback inCallback)
{
	sIRQCallback = inCallback;
}

//###################################################################################################
//
//	AsgardZ80GetIRQCallback -- gets the function to be called when an interrupt is generated
//
//###################################################################################################

AsgardZ80IRQCallback AsgardZ80GetIRQCallback(void)
{
	return sIRQCallback;
}


//###################################################################################################
//
//	AsgardZ80Execute -- run the CPU emulation
//
//	This function executes the Z80 for the specified number of cycles, returning the actual
//	number of cycles executed.
//
//###################################################################################################

asm int AsgardZ80Execute(register int inCycles)
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
	stwu	sp,-132(sp)
	
	//	
	// 	load the cycle count, accounting for any extra interrupt cycles
	//	also, clear the exit action cycle count
	//
	_asm_get_global(r4,sInterruptCycleAdjust)
	li		r0,0
	sub		rICount,inCycles,r4
	_asm_set_global(r0,sInterruptCycleAdjust)
	_asm_set_global(inCycles,sRequestedCycles)
	_asm_set_global(r0,sExitActionCycles)
	
	//	
	// 	initialize tables & goodies
	//
//	_asm_get_global_ptr(rArgumentROMPtr,AZ80_ARGUMENTROM)
//	_asm_get_global_ptr(rOpcodeROMPtr,AZ80_OPCODEROM)
	_asm_get_global_ptr(rICountPtr,AZ80_ICOUNT)
	_asm_get_global_ptr(rFlagTable,sLookupTables)
	_asm_get_global_ptr(rOpcodeTable,sOpcodeTable)
	SAVE_ICOUNT
//	lwz		rArgumentROM,0(rArgumentROMPtr)
//	lwz		rOpcodeROM,0(rOpcodeROMPtr)

	//
	//	restore the state of the machine
	//
	_asm_get_global(rFlags,sFlags)
	_asm_get_global(rPCAF,sPCAF)
	_asm_get_global(rHLBC,sHLBC)
	rlwinm	rFlags,rFlags,0,29,27			// clear the dirty bit in rFlags
	_asm_get_global(rSPDE,sSPDE)
	_asm_get_global(rIXIY,sIXIY)
	_asm_get_global(rAFDE2,sAFDE2)
	SAVE_FLAGS
	_asm_get_global(rHLBC2,sHLBC2)
	EI_FLAG_RESET

executeMore:
	//
	//	mark that we're executing and clear the exit actions
	//
	li		r0,1
	li		r9,0
	_asm_set_global_b(r0,sExecuting)
	_asm_set_global_b(r9,sExitActions)

	//================================================================================================

	//
	// 	this is the heart of the Z80 execution loop; the process is basically this: load an opcode,
	// 	increment the R register, look up the function, and call it
	//
executeLoop:

	//
	//	internal debugging hook
	//
#if AZ80_COREDEBUG
	mr		r3,rPCAF
	mr		r4,rSPDE
	mr		r5,rHLBC
	mr		r6,rIXIY
	mr		r7,rAFDE2
	mr		r8,rHLBC2
	mr		r9,rFlags
	mr		r10,rICount
	bl		AsgardZ80MiniTrace
#endif

	//
	//	external debugging hook
	//
executeLoopEI:
#ifdef AZ80_DEBUGHOOK
	_asm_get_global(r3,mame_debug)
#if TARGET_RT_MAC_CFM
	lwz		r3,0(r3)
#endif
	cmpwi	r3,0
	beq		executeLoopNoDebug
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	_asm_set_global(rIXIY,sIXIY)
	_asm_set_global(rAFDE2,sAFDE2)
	_asm_set_global(rHLBC2,sHLBC2)
	stw		rICount,0(rICountPtr)
	bl		AZ80_DEBUGHOOK
	_asm_get_global(rFlags,sFlags)
	_asm_get_global(rPCAF,sPCAF)
	_asm_get_global(rSPDE,sSPDE)
	_asm_get_global(rHLBC,sHLBC)
	_asm_get_global(rIXIY,sIXIY)
	_asm_get_global(rAFDE2,sAFDE2)
	_asm_get_global(rHLBC2,sHLBC2)
	lwz		rICount,0(rICountPtr)
#endif

executeLoopNoDebug:
	//
	//	read the opcode and branch to the appropriate location
	//
	READ_OPCODE(r3)
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	lwzx	r5,rOpcodeTable,r5					// r5 = rOpcodeTable[r3 << 2]
	li		rCycleCount,0
	mtctr	r5									// ctr = r5
	_asm_set_global(r4,sOpcodePC)				// save the PC
	addis	rFlags,rFlags,0x0100				// ++R
	bctr										// go for it

	//
	//	we get back here after any opcode that needs to store a byte result at (rEA)
	//
executeLoopEndWriteEA:
	rlwinm	r3,rEA,0,16,31						// r3 = address
	bl		WRITEMEM							// perform the write
	bl		postExtern							// post-process

	//
	//	we get back here after any other opcode
	//
executeLoopEnd:
	sub.	rICount,rICount,rCycleCount			// decrement the cycle count
	BRANCH_ON_EI(eiContinue)
	SAVE_ICOUNT
	bgt		executeLoop							// loop if we're not done

	//================================================================================================

executeLoopExit:
	//
	// 	add back any exit action cycles and store the final icount
	//
	_asm_get_global(r3,sExitActionCycles)
	li		r0,0
	add		rICount,rICount,r3
	_asm_set_global(r0,sExitActionCycles)
	stw		rICount,0(rICountPtr)

	//
	// 	save the final state of the machine
	//
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	_asm_set_global(rIXIY,sIXIY)
	_asm_set_global(rAFDE2,sAFDE2)
	_asm_set_global(rHLBC2,sHLBC2)

	//
	//	mark that we're no longer executing
	//
	li		r0,0
	_asm_set_global_b(r0,sExecuting)

	//
	//	see if there are interrupts pending
	//
	_asm_get_global_b(rTempSave,sExitActions)
	andi.	r0,rTempSave,kExitActionGenerateNMI
	beq		noPendingPendingNMI
	bl		ProcessNMI
	bl		postExtern
noPendingPendingNMI:
	andi.	r0,rTempSave,kExitActionGenerateIRQ
	beq		noPendingPendingIRQ
	bl		ProcessIRQ
	bl		postExtern
noPendingPendingIRQ:
	
	//
	// 	account for any interrupt cycles and store the final cycle count
	//
	_asm_get_global(r3,sInterruptCycleAdjust)
	li		r0,0
	sub.	rICount,rICount,r3
	_asm_set_global(r0,sInterruptCycleAdjust)
	stw		rICount,0(rICountPtr)

	//
	//	if there's time left on the clock, try again
	//
	bgt		executeMore

#if AZ80_COREDEBUG
	mr		r3,rPCAF
	mr		r4,rSPDE
	mr		r5,rHLBC
	mr		r6,rIXIY
	mr		r7,rAFDE2
	mr		r8,rHLBC2
	mr		r9,rFlags
	mr		r10,rICount
	bl		AsgardZ80MiniTrace
#endif

	//
	// 	standard function epilogue; we also compute the number of cycles processed in r3 for return
	//
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
	_asm_get_global(r12,sFlags)					// r5 = sFlags
	LOAD_ICOUNT									// rICount = sICount
	rlwimi	rFlags,r12,0,16,31					// only keep the low bits of the new flags
	andi.	r12,r12,kFlagsDirty					// extract the dirty flag
	LOAD_ROM									// reload the ROM pointers
	beqlr										// if the registers aren't dirty, return
	LOAD_PCAF									// get the new PC/AF
	LOAD_HLBC									// get the new HL/BC
	rlwinm	rFlags,rFlags,0,29,27				// clear the dirty bit
	LOAD_SPDE									// get the new SP/DE
	LOAD_IXIY									// get the new IX/IY
	LOAD_AFDE2									// get the new AF2/DE2
	LOAD_HLBC2									// get the new HL/BC2
	SAVE_FLAGS									// save the updated flags
	blr											// return

	//================================================================================================

#ifdef AZ80_UPDATEBANK

	//
	//	post-PC change update: make sure the ROM bank hasn't been switched out from under us
	//
updateBank:
	mflr	rTempSave
	GET_PC(r3)
	bl		AZ80_UPDATEBANK
	mtlr	rTempSave
//	lwz		rOpcodeROM,0(rOpcodeROMPtr)			// restore the ROM pointer
//	lwz		rArgumentROM,0(rArgumentROMPtr)		// restore the argument ROM pointer
	blr

#endif

	//================================================================================================

	//
	// 	extended opcodes: load another byte and look up the function later in the table
	//
entry static cb
	READ_OPCODE(r4)								// read the next opcode into r4
	addi	r6,rOpcodeTable,OpcodeTable.fCB		// point to these opcodes
	rlwinm	r5,r4,2,0,29						// r5 = r4 << 2
	lwzx	r5,r6,r5							// r5 = rOpcodeTable[r3 << 2]
	mtctr	r5									// ctr = r5
	addis	rFlags,rFlags,0x0100				// ++R
	bctr										// go for it

entry static dd
	READ_OPCODE(r4)								// read the next opcode into r4
	addi	r6,rOpcodeTable,OpcodeTable.fDD		// point to these opcodes
	rlwinm	r5,r4,2,0,29						// r5 = r4 << 2
	lwzx	r5,r6,r5							// r5 = rOpcodeTable[r3 << 2]
	mtctr	r5									// ctr = r5
	addis	rFlags,rFlags,0x0100				// ++R
	bctr										// go for it

entry static ed
	READ_OPCODE(r4)								// read the next opcode into r4
	addi	r6,rOpcodeTable,OpcodeTable.fED		// point to these opcodes
	rlwinm	r5,r4,2,0,29						// r5 = r4 << 2
	lwzx	r5,r6,r5							// r5 = rOpcodeTable[r3 << 2]
	mtctr	r5									// ctr = r5
	addis	rFlags,rFlags,0x0100				// ++R
	bctr										// go for it

entry static fd
	READ_OPCODE(r4)								// read the next opcode into r4
	addi	r6,rOpcodeTable,OpcodeTable.fFD 	// point to these opcodes
	rlwinm	r5,r4,2,0,29						// r5 = r4 << 2
	lwzx	r5,r6,r5							// r5 = rOpcodeTable[r3 << 2]
	mtctr	r5									// ctr = r5
	addis	rFlags,rFlags,0x0100				// ++R
	bctr										// go for it

	//
	// 	DDCB opcodes are in the form DDCBjjxx, where jj is the offset (IX+jj) and xx is the subopcode
	//
entry static dd_cb
	READ_OPCODE_ARG(r3)							// r3 = offset
	CYCLES(20)									// all of these take at least 20 cycles
	READ_OPCODE_ARG(r5)							// r5 = subopcode
	extsb	r3,r3								// r3 = sign-extended offset
	andi.	r0,r5,0xc0							// r0 = subop & 0xc0
	GET_IX(r4)									// r4 = IX
	cmpwi	r0,0x40								// see if (subop & 0xc0 == 0x40) -- subop >= 0x40 && subop < 0x80
	add		rEA,r3,r4							// rEA = IX + offset
	addi	r6,rOpcodeTable,OpcodeTable.fDDCB	// point to these opcodes
	SAVE_PCAF									// save PC/AF
	beq		dd_cb_bit							// skip if this is a BIT opcode
	CYCLES(3)									// an extra 3 cycles for everything else
dd_cb_bit:
	rlwinm	r5,r5,2,0,29						// r5 = r5 << 2
	lwzx	rTempSave,r6,r5						// rTempSave = rOpcodeTable[r3 << 2]
	rlwinm	r3,rEA,0,16,31						// keep EA in range
	bl		READMEM								// perform the read
	mtctr	rTempSave							// ctr = opcode address
	bl		postExtern							// post-process
	bctr										// go for it
	
	//
	// 	FDCB opcodes are in the form FDCBjjxx, where jj is the offset (IY+jj) and xx is the subopcode
	//
entry static fd_cb
	READ_OPCODE_ARG(r3)							// r3 = offset
	CYCLES(20)									// all of these take at least 20 cycles
	READ_OPCODE_ARG(r5)							// r5 = subopcode
	extsb	r3,r3								// r3 = sign-extended offset
	andi.	r0,r5,0xc0							// r0 = subop & 0xc0
	GET_IY(r4)									// r4 = IY
	cmpwi	r0,0x40								// see if (subop & 0xc0 == 0x40) -- subop >= 0x40 && subop < 0x80
	add		rEA,r3,r4							// rEA = IY + offset
	addi	r6,rOpcodeTable,OpcodeTable.fFDCB	// point to these opcodes
	SAVE_PCAF									// save PC/AF
	beq		fd_cb_bit							// skip if this is a BIT opcode
	CYCLES(3)									// an extra 3 cycles for everything else
fd_cb_bit:
	rlwinm	r5,r5,2,0,29						// r5 = r5 << 2
	lwzx	rTempSave,r6,r5						// rTempSave = rOpcodeTable[r3 << 2]
	rlwinm	r3,rEA,0,16,31						// keep EA in range
	bl		READMEM								// perform the read
	mtctr	rTempSave							// ctr = opcode address
	bl		postExtern 							// post-process
	bctr										// go for it

	//================================================================================================

	//
	// 	local subroutines: compute the EA for addressing forms like (IX+nn) and (IY+nn)
	//
ixplusoffset_save:
	CYCLES(19)									// count 19 cycles
//	GET_PC(r4)									// r4 = current PC
//	lbzx	r4,rArgumentROM,r4					// r4 = immediate byte value
	READ_OPCODE_ARG(r4)
	mflr	rTempSave							// remember the LR
	extsb	r4,r4								// sign extend the offset
	GET_IX(rEA)									// rEA = address
//	addis	rPCAF,rPCAF,1						// increment & wrap the PC
	add		rEA,rEA,r4							// rEA = address + offset
	SAVE_PCAF									// save PC/AF
	rlwinm	r3,rEA,0,16,31						// keep EA in range
	bl		READMEM								// perform the read
	mtlr	rTempSave							// restore the LR
	b		postExtern							// post-process

iyplusoffset_save:
	CYCLES(19)									// count 19 cycles
//	GET_PC(r4)									// r4 = current PC
//	lbzx	r4,rArgumentROM,r4					// r4 = immediate byte value
	READ_OPCODE_ARG(r4)
	mflr	rTempSave							// remember the LR
	extsb	r4,r4								// sign extend the offset
	GET_IY(rEA)									// rEA = address
//	addis	rPCAF,rPCAF,1						// increment & wrap the PC
	add		rEA,rEA,r4							// rEA = address + offset
	SAVE_PCAF									// save PC/AF
	rlwinm	r3,rEA,0,16,31						// keep EA in range
	bl		READMEM								// perform the read
	mtlr	rTempSave							// restore the LR
	b		postExtern							// post-process

	//================================================================================================

	//
	//		special case opcodes
	//
entry static no_op
	subis	rPCAF,rPCAF,1						// decrement PC, to account for the prefix byte
	b		executeLoopEnd

entry static no_op_xx
	CYCLES(-23)									// take back the cycles we tried to claim before
	b		executeLoopEnd

	//================================================================================================

	//
	//		ADC_A: add with carry to A
	//
entry static adc_a_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		adc0

entry static adc_a_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		adc0

entry static adc_a_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		adc0

entry static adc_a_a
	GET_A(r3)
adc4:
	CYCLES(4)
adc0:
	GET_A(r4)									// r4 = A
	rlwinm	r5,rPCAF,0,31,31					// r5 = C
	xor		r7,r3,r4							// r7 = r3 ^ A
	add		r6,r3,r4							// r6 = r3 + A
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	add		r6,r6,r5							// r6 = A' = r3 + A + C
	xori	r8,r7,0x80							// r8 = r3 ^ A ^ 0x80
	rlwimi	rPCAF,r6,24,31,31					// C = (A' & 0x100)
	xor		r9,r3,r6							// r9 = r3 ^ A'
	rlwimi	rPCAF,r6,0,24,24					// S = (A' & 0x80)
	xor		r7,r7,r6							// r7 = r3 ^ A ^ A'
	rlwinm	r6,r6,0,24,31						// r6 = A' &= 0xff
	rlwimi	rPCAF,r7,0,27,27					// H = (r3 ^ A ^ A') & 0x10
	and		r8,r8,r9							// r8 = (r3 ^ A ^ 0x80) & (r3 ^ A')
	SET_A(r6)									// A = A'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3 ^ 0x80) & (r3 ^ A') & 0x80
	rlwimi	rPCAF,r7,1,25,25					// Z = (A' == 0)
	b		executeLoopEnd

entry static adc_a_b
	GET_B(r3)
	b		adc4

entry static adc_a_c
	GET_C(r3)
	b		adc4

entry static adc_a_d
	GET_D(r3)
	b		adc4

entry static adc_a_e
	GET_E(r3)
	b		adc4

entry static adc_a_h
	GET_H(r3)
	b		adc4

entry static adc_a_l
	GET_L(r3)
	b		adc4

entry static adc_a_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		adc0

entry static adc_a_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		adc0

entry static adc_a_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		adc0

entry static adc_a_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		adc0

entry static adc_a_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		adc0

	//================================================================================================

	//
	//		ADC_HL: add with carry to HL
	//
entry static adc_hl_bc
	GET_BC(r3)
adchl15:
	GET_HL(r4)									// r4 = HL
	rlwinm	r5,rPCAF,0,31,31					// r5 = C
	xor		r7,r3,r4							// r7 = r3 ^ HL
	add		r6,r3,r4							// r6 = r3 + HL
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	add		r6,r6,r5							// r6 = HL' = r3 + HL + C
	xori	r8,r7,0x8000						// r8 = r3 ^ HL ^ 0x8000
	rlwimi	rPCAF,r6,16,31,31					// C = (HL' & 0x10000)
	xor		r9,r3,r6							// r9 = r3 ^ HL'
	rlwimi	rPCAF,r6,24,24,24					// S = (HL' & 0x8000)
	xor		r7,r7,r6							// r7 = r3 ^ HL ^ HL'
	rlwinm	r6,r6,0,16,31						// r6 = HL' &= 0xffff
	rlwimi	rPCAF,r7,24,27,27					// H = (r3 ^ HL ^ HL') & (r3 ^ HL') & 0x1000
	and		r8,r8,r9							// r8 = (r3 ^ HL ^ 0x8000) & (r3 ^ HL')
	SET_HL(r6)									// HL = HL'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,19,29,29					// V = (HL ^ r3 ^ 0x8000) & (r3 ^ HL') & 0x8000
	rlwimi	rPCAF,r7,1,25,25					// Z = (HL' == 0)
	CYCLES(15)
	SAVE_HLBC
	b		executeLoopEnd

entry static adc_hl_de
	GET_DE(r3)
	b		adchl15

entry static adc_hl_hl
	GET_HL(r3)
	b		adchl15

entry static adc_hl_sp
	GET_SP(r3)
	b		adchl15

	//================================================================================================

	//
	//		ADD_A: add to A
	//
entry static add_a_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		add0

entry static add_a_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		add0

entry static add_a_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		add0

entry static add_a_a
	GET_A(r3)
add4:
	CYCLES(4)
add0:
	GET_A(r4)									// r4 = A
	xor		r7,r3,r4							// r7 = A ^ r3
	add		r6,r3,r4							// r6 = A' = A + r3
	rlwinm	rPCAF,rPCAF,0,0,23					// N = 0 (actually all = 0)
	xori	r8,r7,0x80							// r8 = A ^ r3 ^ 0x80
	rlwimi	rPCAF,r6,24,31,31					// C = (A' & 0x100)
	xor		r9,r3,r6							// r9 = r3 ^ A'
	rlwimi	rPCAF,r6,0,24,24					// S = (A' & 0x80)
	xor		r7,r7,r6							// r7 = A ^ r3 ^ A'
	rlwinm	r6,r6,0,24,31						// r6 = A' &= 0xff
	rlwimi	rPCAF,r7,0,27,27					// H = (A ^ r3 ^ A') & 0x10
	and		r8,r8,r9							// r8 = (A ^ r3 ^ 0x80) & (r3 ^ A')
	SET_A(r6)									// A = A'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3 ^ 0x80) & (r3 ^ A') & 0x80
	rlwimi	rPCAF,r7,1,25,25					// Z = (A' == 0)
	b		executeLoopEnd

entry static add_a_b
	GET_B(r3)
	b		add4

entry static add_a_c
	GET_C(r3)
	b		add4

entry static add_a_d
	GET_D(r3)
	b		add4

entry static add_a_e
	GET_E(r3)
	b		add4

entry static add_a_h
	GET_H(r3)
	b		add4

entry static add_a_l
	GET_L(r3)
	b		add4

entry static add_a_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		add0

entry static add_a_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		add0

entry static add_a_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		add0

entry static add_a_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		add0

entry static add_a_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		add0

	//================================================================================================

	//
	//		ADD_HL: add to HL
	//
entry static add_hl_bc
	GET_BC(r3)
addhl11:
	GET_HL(r4)								// r4 = HL
	andi.	r5,rPCAF,kZ80FlagS|kZ80FlagZ|kZ80FlagV 	// keep only S/Z/V
	xor		r7,r3,r4						// r7 = HL ^ r3
	add		r6,r3,r4						// r6 = HL' = HL + r3
	rlwimi	rPCAF,r5,0,24,31				// S = S, Z = Z, V = V, N = 0
	xor		r7,r7,r6						// r7 = HL ^ r3 ^ HL'
	rlwimi	rPCAF,r6,16,31,31				// C = (HL' & 0x10000)
	SET_HL(r6)								// HL = HL'
	rlwimi	rPCAF,r7,24,27,27				// H = (HL ^ r3 ^ HL') & 0x1000
	CYCLES(11)
	SAVE_HLBC
	b		executeLoopEnd

entry static add_hl_de
	GET_DE(r3)
	b		addhl11

entry static add_hl_hl
	GET_HL(r3)
	b		addhl11

entry static add_hl_sp
	GET_SP(r3)
	b		addhl11

	//================================================================================================

	//
	//		ADD_IX: add to IX
	//
entry static add_ix_bc
	GET_BC(r3)
addix15:
	GET_IX(r4)									// r4 = IX
	andi.	r5,rPCAF,kZ80FlagS|kZ80FlagZ|kZ80FlagV 		// keep only S/Z/V
	xor		r7,r3,r4							// r7 = IX ^ r3
	add		r6,r3,r4							// r6 = IX' = IX + r3
	rlwimi	rPCAF,r5,0,24,31					// S = S, Z = Z, V = V, N = 0
	xor		r7,r7,r6							// r7 = IX ^ r3 ^ IX'
	rlwimi	rPCAF,r6,16,31,31					// C = (IX' & 0x10000)
	SET_IX(r6)									// IX = IX'
	rlwimi	rPCAF,r7,24,27,27					// H = (IX ^ r3 ^ IX') & 0x1000
	CYCLES(15)
	SAVE_IXIY
	b		executeLoopEnd

entry static add_ix_de
	GET_DE(r3)
	b		addix15

entry static add_ix_ix
	GET_IX(r3)
	b		addix15

entry static add_ix_sp
	GET_SP(r3)
	b		addix15

	//================================================================================================

	//
	//		ADD_IY: add to IY
	//
entry static add_iy_bc
	GET_BC(r3)
addiy15:
	GET_IY(r4)									// r4 = IY
	andi.	r5,rPCAF,kZ80FlagS|kZ80FlagZ|kZ80FlagV		// keep only S/Z/V
	xor		r7,r3,r4							// r7 = IY ^ r3
	add		r6,r3,r4							// r6 = IY' = IY + r3
	rlwimi	rPCAF,r5,0,24,31					// S = S, Z = Z, V = V, N = 0
	xor		r7,r7,r6							// r7 = IY ^ r3 ^ IY'
	rlwimi	rPCAF,r6,16,31,31					// C = (IY' & 0x10000)
	SET_IY(r6)									// IY = IY'
	rlwimi	rPCAF,r7,24,27,27					// H = (IY ^ r3 ^ IY') & 0x1000
	CYCLES(15)
	SAVE_IXIY
	b		executeLoopEnd

entry static add_iy_de
	GET_DE(r3)
	b		addiy15

entry static add_iy_iy
	GET_IY(r3)
	b		addiy15

entry static add_iy_sp
	GET_SP(r3)
	b		addiy15

	//================================================================================================

	//
	//		AND_A: and with A
	//
entry static and_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		and0

entry static and_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		and0

entry static and_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		and0

entry static and_a
	GET_A(r3)
and4:
	CYCLES(4)
and0:
	GET_A(r4)									// r4 = R.AF.B.h
	and		r4,r4,r3							// r4 = R.AF.B.h&Reg
	lbzx	r5,rFlagTable,r4					// r5 = flag bits
	SET_A(r4)									// R.AF.B.h = q
	rlwimi	rPCAF,r5,0,24,31					// set the flags
	ori		rPCAF,rPCAF,kZ80FlagH				// also set the H flag
	b		executeLoopEnd

entry static and_b
	GET_B(r3)
	b		and4

entry static and_c
	GET_C(r3)
	b		and4

entry static and_d
	GET_D(r3)
	b		and4

entry static and_e
	GET_E(r3)
	b		and4

entry static and_h
	GET_H(r3)
	b		and4

entry static and_l
	GET_L(r3)
	b		and4

entry static and_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		and0

entry static and_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		and0

entry static and_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		and0

entry static and_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		and0

entry static and_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		and0

	//================================================================================================

	//
	//		BIT_0: test bit 0
	//
entry static bit_0_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit00

entry static bit_0_a
	GET_A(r3)
bit08:
	CYCLES(8)

entry static bit_0_xix
entry static bit_0_xiy
	// note: setup is already handled in the opcode handler
bit00:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,6,25,25					// set the Z flag
	rlwimi	rPCAF,r4,2,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_0_b
	GET_B(r3)
	b		bit08

entry static bit_0_c
	GET_C(r3)
	b		bit08

entry static bit_0_d
	GET_D(r3)
	b		bit08

entry static bit_0_e
	GET_E(r3)
	b		bit08

entry static bit_0_h
	GET_H(r3)
	b		bit08

entry static bit_0_l
	GET_L(r3)
	b		bit08

	//================================================================================================

	//
	//		BIT_1: test bit 1
	//
entry static bit_1_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit10

entry static bit_1_a
	GET_A(r3)
bit18:
	CYCLES(8)

entry static bit_1_xix
entry static bit_1_xiy
	// note: setup is already handled in the opcode handler
bit10:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,5,25,25					// set the Z flag
	rlwimi	rPCAF,r4,1,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_1_b
	GET_B(r3)
	b		bit18

entry static bit_1_c
	GET_C(r3)
	b		bit18

entry static bit_1_d
	GET_D(r3)
	b		bit18

entry static bit_1_e
	GET_E(r3)
	b		bit18

entry static bit_1_h
	GET_H(r3)
	b		bit18

entry static bit_1_l
	GET_L(r3)
	b		bit18

	//================================================================================================

	//
	//		BIT_2: test bit 2
	//
entry static bit_2_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit20

entry static bit_2_a
	GET_A(r3)
bit28:
	CYCLES(8)

entry static bit_2_xix
entry static bit_2_xiy
	// note: setup is already handled in the opcode handler
bit20:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,4,25,25					// set the Z flag
	rlwimi	rPCAF,r4,0,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_2_b
	GET_B(r3)
	b		bit28

entry static bit_2_c
	GET_C(r3)
	b		bit28

entry static bit_2_d
	GET_D(r3)
	b		bit28

entry static bit_2_e
	GET_E(r3)
	b		bit28

entry static bit_2_h
	GET_H(r3)
	b		bit28

entry static bit_2_l
	GET_L(r3)
	b		bit28

	//================================================================================================

	//
	//		BIT_3: test bit 3
	//
entry static bit_3_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit30

entry static bit_3_a
	GET_A(r3)
bit38:
	CYCLES(8)

entry static bit_3_xix
entry static bit_3_xiy
	// note: setup is already handled in the opcode handler
bit30:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,3,25,25					// set the Z flag
	rlwimi	rPCAF,r4,31,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_3_b
	GET_B(r3)
	b		bit38

entry static bit_3_c
	GET_C(r3)
	b		bit38

entry static bit_3_d
	GET_D(r3)
	b		bit38

entry static bit_3_e
	GET_E(r3)
	b		bit38

entry static bit_3_h
	GET_H(r3)
	b		bit38

entry static bit_3_l
	GET_L(r3)
	b		bit38

	//================================================================================================

	//
	//		BIT_4: test bit 4
	//
entry static bit_4_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit40

entry static bit_4_a
	GET_A(r3)
bit48:
	CYCLES(8)

entry static bit_4_xix
entry static bit_4_xiy
	// note: setup is already handled in the opcode handler
bit40:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,2,25,25					// set the Z flag
	rlwimi	rPCAF,r4,30,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_4_b
	GET_B(r3)
	b		bit48

entry static bit_4_c
	GET_C(r3)
	b		bit48

entry static bit_4_d
	GET_D(r3)
	b		bit48

entry static bit_4_e
	GET_E(r3)
	b		bit48

entry static bit_4_h
	GET_H(r3)
	b		bit48

entry static bit_4_l
	GET_L(r3)
	b		bit48

	//================================================================================================

	//
	//		BIT_5: test bit 5
	//
entry static bit_5_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit50

entry static bit_5_a
	GET_A(r3)
bit58:
	CYCLES(8)

entry static bit_5_xix
entry static bit_5_xiy
	// note: setup is already handled in the opcode handler
bit50:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,1,25,25					// set the Z flag
	rlwimi	rPCAF,r4,29,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_5_b
	GET_B(r3)
	b		bit58

entry static bit_5_c
	GET_C(r3)
	b		bit58

entry static bit_5_d
	GET_D(r3)
	b		bit58

entry static bit_5_e
	GET_E(r3)
	b		bit58

entry static bit_5_h
	GET_H(r3)
	b		bit58

entry static bit_5_l
	GET_L(r3)
	b		bit58

	//================================================================================================

	//
	//		BIT_6: test bit 6
	//
entry static bit_6_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit60

entry static bit_6_a
	GET_A(r3)
bit68:
	CYCLES(8)

entry static bit_6_xix
entry static bit_6_xiy
	// note: setup is already handled in the opcode handler
bit60:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,0,25,25					// set the Z flag
	rlwimi	rPCAF,r4,28,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_6_b
	GET_B(r3)
	b		bit68

entry static bit_6_c
	GET_C(r3)
	b		bit68

entry static bit_6_d
	GET_D(r3)
	b		bit68

entry static bit_6_e
	GET_E(r3)
	b		bit68

entry static bit_6_h
	GET_H(r3)
	b		bit68

entry static bit_6_l
	GET_L(r3)
	b		bit68

	//================================================================================================

	//
	//		BIT_7: test bit 7
	//
entry static bit_7_xhl
	CYCLES(12)
	READ_AT_REG_SAVE(HL)
	b		bit70

entry static bit_7_a
	GET_A(r3)
bit78:
	CYCLES(8)

entry static bit_7_xix
entry static bit_7_xiy
	// note: setup is already handled in the opcode handler
bit70:
	rlwinm	rPCAF,rPCAF,0,31,23					// zero out all flag bits but the carry
	not		r4,r3								// r4 = ~r3
	ori		rPCAF,rPCAF,kZ80FlagH				// set the H flag
	rlwimi	rPCAF,r4,31,25,25					// set the Z flag
	rlwimi	rPCAF,r3,0,24,24					// set the S flag
	rlwimi	rPCAF,r4,27,29,29					// set the P flag
	b		executeLoopEnd

entry static bit_7_b
	GET_B(r3)
	b		bit78

entry static bit_7_c
	GET_C(r3)
	b		bit78

entry static bit_7_d
	GET_D(r3)
	b		bit78

entry static bit_7_e
	GET_E(r3)
	b		bit78

entry static bit_7_h
	GET_H(r3)
	b		bit78

entry static bit_7_l
	GET_L(r3)
	b		bit78

	//================================================================================================

	//
	//		CALL: call a subroutine
	//
entry static call_c
	rlwinm.	r0,rPCAF,0,31,31
	beq		SkipCall

entry static call
DoCall:
	CYCLES(17)
	READ_OPCODE_ARG_WORD(rTempSave)				// load the calling address
	PUSH_HI_SAVE(rPCAF)							// push the current PC
	SET_PC(rTempSave)							// set the new PC
	UPDATE_BANK
	b		executeLoopEnd

SkipCall:
	CYCLES(10)
	addis	rPCAF,rPCAF,2						// skip the address
	b		executeLoopEnd
	
entry static call_m
	rlwinm.	r0,rPCAF,0,24,24
	beq		SkipCall
	b		DoCall

entry static call_nc
	rlwinm.	r0,rPCAF,0,31,31
	bne		SkipCall
	b		DoCall

entry static call_nz
	rlwinm.	r0,rPCAF,0,25,25
	bne		SkipCall
	b		DoCall

entry static call_p
	rlwinm.	r0,rPCAF,0,24,24
	bne		SkipCall
	b		DoCall

entry static call_pe
	rlwinm.	r0,rPCAF,0,29,29
	beq		SkipCall
	b		DoCall

entry static call_po
	rlwinm.	r0,rPCAF,0,29,29
	bne		SkipCall
	b		DoCall

entry static call_z
	rlwinm.	r0,rPCAF,0,25,25
	beq		SkipCall
	b		DoCall

	//================================================================================================

	//
	//		CCF: complement the carry flag
	//
entry static ccf
	rlwinm	rPCAF,rPCAF,0,31,29					// clear the N flag
	CYCLES(4)
	rlwimi	rPCAF,rPCAF,4,27,27					// H = C
	xori	rPCAF,rPCAF,1						// complement C
	b		executeLoopEnd

	//================================================================================================

	//
	//		CP_A: compare to A
	//
entry static cp_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		cp0

entry static cp_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		cp0

entry static cp_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		cp0

entry static cp_a
	GET_A(r3)
cp4:
	CYCLES(4)
cp0:
	GET_A(r4)									// r4 = A
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	xor		r8,r3,r4							// r8 = A ^ r3
	ori		rPCAF,rPCAF,kZ80FlagN				// N = 1
	sub		r6,r4,r3							// r6 = tmp = A - r3
	rlwimi	rPCAF,r6,24,31,31					// C = tmp & 0x100
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,0,24,24					// S = tmp & 0x80
	xor		r7,r8,r6							// r7 = A ^ r3 ^ tmp
	rlwinm	r6,r6,0,24,31						// r6 = tmp &= 0xff
	rlwimi	rPCAF,r7,0,27,27					// H = (A ^ r3 ^ tmp) & 0x10
	and		r8,r8,r9							// r8 = (A ^ r3) & (r3 ^ tmp)
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3) & (r3 ^ tmp) & 0x80
	rlwimi	rPCAF,r7,1,25,25					// Z = (tmp == 0)
	b		executeLoopEnd

entry static cp_b
	GET_B(r3)
	b		cp4

entry static cp_c
	GET_C(r3)
	b		cp4

entry static cp_d
	GET_D(r3)
	b		cp4

entry static cp_e
	GET_E(r3)
	b		cp4

entry static cp_h
	GET_H(r3)
	b		cp4

entry static cp_l
	GET_L(r3)
	b		cp4

entry static cp_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		cp0

entry static cp_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		cp0

entry static cp_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		cp0

entry static cp_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		cp0

entry static cp_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		cp0

	//================================================================================================

	//
	//		CPD/CPI: compare and advance
	//
cpxcommon:
	CYCLES(16)
	mflr	rEA
	READ_AT_REG_SAVE(HL)						// r3 = (HL) = i
	GET_A(r4)									// r4 = R.AF.B.h
	rlwinm	rPCAF,rPCAF,0,31,24					// clear all flags but the carry
	xor		r6,r4,r3							// r6 = R.AF.B.h^i
	sub		r5,r4,r3							// r5 = j = R.AF.B.h-i
	mtlr	rEA
	subi	r10,rHLBC,1							// --R.BC.W.l
	xor		r7,r6,r5							// r7 = R.AF.B.h^i^j
	SET_BC(r10)									// keep BC in range
	rlwinm	r5,r5,0,24,31						// r5 = j&0xff
	ori		rPCAF,rPCAF,kZ80FlagN|kZ80FlagV		// set the N+V flags
	add		rHLBC,rHLBC,rTempSave				// increment/decrement HL
	cntlzw	r8,r5								// r8 = number of zeros in r5
	rlwimi	rPCAF,r7,0,27,27					// set the H flag
	rlwinm.	r0,r10,0,16,31						// see if BC==0
	cmpwi	cr1,r5,0							// is the result 0?
	rlwimi	rPCAF,r8,1,25,25					// set the Z flag
	SAVE_HLBC									// update HL/BC
	rlwimi	rPCAF,r5,0,24,24					// set the S flag
	blr

entry static cpd
	lis		rTempSave,-1						// decrement
	bl		cpxcommon							// do the standard stuff
	bne		executeLoopEnd						// skip if BC!=0
	rlwinm	rPCAF,rPCAF,0,30,28					// clear the V flag
	b		executeLoopEnd						// continue

entry static cpdr
	lis		rTempSave,-1						// decrement
	bl		cpxcommon							// do the standard stuff
	bne+	cpdrRep								// skip if BC!=0
	rlwinm	rPCAF,rPCAF,0,30,28					// clear the V flag
	b		executeLoopEnd						// continue
cpdrRep:
	beq-	cr1,executeLoopEnd					// if the result was zero, the loop is done
	subis	rPCAF,rPCAF,2						// point back to the start of this opcode
	CYCLES(5)									// count some extra cycles
	b		executeLoopEnd						// continue

entry static cpi
	lis		rTempSave,1							// increment
	bl		cpxcommon							// do the standard stuff
	bne		executeLoopEnd						// skip if BC!=0
	rlwinm	rPCAF,rPCAF,0,30,28					// clear the V flag
	b		executeLoopEnd						// continue

entry static cpir
	lis		rTempSave,1							// increment
	bl		cpxcommon							// do the standard stuff
	bne+	cpirRep								// skip if BC!=0
	rlwinm	rPCAF,rPCAF,0,30,28					// clear the V flag
	b		executeLoopEnd						// continue
cpirRep:
	beq-	cr1,executeLoopEnd					// if the result was zero, the loop is done
	subis	rPCAF,rPCAF,2						// point back to the start of this opcode
	CYCLES(5)									// count some extra cycles
	b		executeLoopEnd						// continue

	//================================================================================================

	//
	//		CPL: complement accumulator
	//
entry static cpl
	xori	rPCAF,rPCAF,0xff00					// complement A
	CYCLES(4)									// count the cycles
	ori		rPCAF,rPCAF,kZ80FlagH|kZ80FlagN		// set the H,N flags
	rlwimi	rPCAF,rPCAF,24,28,28
	rlwimi	rPCAF,rPCAF,24,26,26
	b		executeLoopEnd						// continue

	//================================================================================================

	//
	//		DAA: adjust for BCD arithmetic
	//
entry static daa
	rlwinm	r3,rPCAF,25,23,30					// r3 = A << 1
	rlwimi	r3,rPCAF,9,22,22					// r3 |= kZ80FlagC
	addi	r4,rFlagTable,LookupTables.fDAA		// r4 -> DAA table
	rlwimi	r3,rPCAF,6,21,21					// r3 |= kZ80FlagH
	rlwimi	r3,rPCAF,10,20,20					// r3 |= kZ80FlagN
	lhzx	r5,r4,r3							// r5 = DAA[r3]
	CYCLES(4)									// count the cycles
	rlwimi	rPCAF,r5,0,16,31					// set A and the flags
	b		executeLoopEnd						// continue

	//================================================================================================

	//
	//		DEC: decrement an 8-bit value
	//
dec0:
	subi	r4,r3,1								// r4 = r3 - 1
	rlwinm	rPCAF,rPCAF,0,31,23					// clear out all but the carry
	rlwinm	r4,r4,0,24,31						// keep result in range
	ori		rPCAF,rPCAF,kZ80FlagN				// set the N flag
	rlwinm	r5,r3,0,28,31						// get low 4 bits of original value in r5
	subi	r3,r3,0x80							// subtract 0x80 from the original value
	rlwimi	rPCAF,r4,0,24,24					// set the S flag
	cntlzw	r6,r4								// r6 = number of zeros in result
	cntlzw	r5,r5								// r5 = number of zeros in low 4 bits
	rlwimi	rPCAF,r6,1,25,25					// set the Z flag
	cntlzw	r3,r3								// r3 = 32 if the original value was 0x80
	rlwimi	rPCAF,r5,31,27,27					// set the H flag
	rlwimi	rPCAF,r3,29,29,29					// set the V flag
	blr
	
entry static dec_xhl
	CYCLES(11)
	READ_AT_REG_SAVE_EA(HL)
	bl		dec0
	b		executeLoopEndWriteEA
	
entry static dec_xix
	CYCLES(4)
	bl		ixplusoffset_save					// automatically adds 19 cycles
	bl		dec0
	b		executeLoopEndWriteEA
	
entry static dec_xiy
	CYCLES(4)
	bl		iyplusoffset_save					// automatically adds 19 cycles
	bl		dec0
	b		executeLoopEndWriteEA

entry static dec_a
	GET_A(r3)
	bl		dec0
	SET_A(r4)
	CYCLES(4)
	b		executeLoopEnd
	
entry static dec_b
	GET_B(r3)
	bl		dec0
	SET_B(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dec_c
	GET_C(r3)
	bl		dec0
	SET_C(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dec_d
	GET_D(r3)
	bl		dec0
	SET_D(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static dec_e
	GET_E(r3)
	bl		dec0
	SET_E(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static dec_h
	GET_H(r3)
	bl		dec0
	SET_H(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dec_l
	GET_L(r3)
	bl		dec0
	SET_L(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dec_ixh
	GET_IXH(r3)
	bl		dec0
	SET_IXH(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static dec_ixl
	GET_IXL(r3)
	bl		dec0
	SET_IXL(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static dec_iyh
	GET_IYH(r3)
	bl		dec0
	SET_IYH(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static dec_iyl
	GET_IYL(r3)
	bl		dec0
	SET_IYL(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
	//================================================================================================

	//
	//		DEC: decrement a 16-bit value
	//
entry static dec_bc
	subi	r3,rHLBC,1
	rlwimi	rHLBC,r3,0,16,31
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dec_de
	subi	r3,rSPDE,1
	rlwimi	rSPDE,r3,0,16,31
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static dec_hl
	subis	rHLBC,rHLBC,1
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static dec_ix
	subis	rIXIY,rIXIY,1
	CYCLES(10)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static dec_iy
	subi	r3,rIXIY,1
	rlwimi	rIXIY,r3,0,16,31
	CYCLES(10)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static dec_sp
	subis	rSPDE,rSPDE,1
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		DI: disable interrupts
	//
entry static di
	rlwinm	rFlags,rFlags,0,26,23				// clear IFF1 & IFF2
	CYCLES(4)
	SAVE_FLAGS
	b		executeLoopEnd

	//================================================================================================

	//
	//		DJNZ: decrement and jump if non-zero
	//
entry static djnz
	GET_B(r3)
	subic.	r4,r3,1
	SET_B(r4)
	beq		SkipJR1
DoJR1:
	READ_OPCODE_ARG(r3)							// load the jump offset
	CYCLES(13)
	extsb	r3,r3								// sign extend the byte
	rlwinm	r4,r3,16,0,15						// shift it high
	add		rPCAF,rPCAF,r4						// update the PC
	SAVE_HLBC
	b		executeLoopEnd
SkipJR1:
	CYCLES(8)
	addis	rPCAF,rPCAF,1;						// skip the offset
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		EI: enable interrupts
	//
entry static ei
	_asm_get_global_b(r9,sNMINestLevel)
	rlwinm.	r0,rFlags,EXTRACT_IFF1				// check IFF1
	cmpwi	cr1,r9,0
	subi	r9,r9,1
	beq		cr1,einodec
	_asm_set_global_b(r9,sNMINestLevel)
einodec:
	beq		eitricky							// if IFF1==0, it gets a little hairy
	ori		rFlags,rFlags,kFlagsIFF2			// set IFF2
	SAVE_FLAGS									// update the flags
	CYCLES(4)
	b		executeLoopEnd
eitricky:
	ori		rFlags,rFlags,kFlagsIFF1|kFlagsIFF2 // set both IFF's
	EI_FLAG_SET
	SAVE_FLAGS
	b		executeLoopEI						// jump to the top of the loop

eiContinue:
	SAVE_ICOUNT
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	_asm_set_global(rIXIY,sIXIY)
	_asm_set_global(rAFDE2,sAFDE2)
	_asm_set_global(rHLBC2,sHLBC2)
	bl		CheckIRQ
	li		rCycleCount,4
	bl		postExtern
	EI_FLAG_RESET
	b		executeLoopEnd

	//================================================================================================

	//
	//		EX: exchange two values
	//
entry static ex_xsp_hl
	GET_SP(rEA)
	CYCLES(19)
	READ_WORD_SAVE(rEA)
	WRITE_WORD_HI(rEA,rHLBC)
	SET_HL(rTempSave)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static ex_xsp_ix
	GET_SP(rEA)
	CYCLES(23)
	READ_WORD_SAVE(rEA)
	WRITE_WORD_HI(rEA,rIXIY)
	SET_IX(rTempSave)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static ex_xsp_iy
	GET_SP(rEA)
	CYCLES(23)
	READ_WORD_SAVE(rEA)
	WRITE_WORD_LO(rEA,rIXIY)
	SET_IY(rTempSave)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static ex_af_af
	GET_AF2(r3)
	rlwimi	rAFDE2,rPCAF,16,0,15
	SET_AF(r3)
	SAVE_AFDE2
	CYCLES(4)
	b		executeLoopEnd

entry static ex_de_hl
	GET_DE(r3)
	rlwimi	rSPDE,rHLBC,16,16,31
	SET_HL(r3)
	SAVE_SPDE
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static exx
	mr		r3,rHLBC2
	GET_DE2(r4)
	mr		rHLBC2,rHLBC
	rlwimi	rAFDE2,rSPDE,0,16,31
	SAVE_HLBC2
	mr		rHLBC,r3
	SAVE_AFDE2
	SET_DE(r4)
	SAVE_HLBC
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		HALT: halt processing until an interrupt
	//
entry static halt
	ori		rFlags,rFlags,kFlagsHALT			// set the HALT flag
	SAVE_FLAGS									// update the flags
	li		rCycleCount,4						// CYCLES(4)
	cmpwi	rICount,0
	subis	rPCAF,rPCAF,1						// back up the PC so we stay here
	ble		executeLoopEnd
	li		rICount,0
	b		executeLoopEnd

	//================================================================================================

	//
	//		IM: set the interrupt mode
	//
entry static im_0
	li		r3,0
	rlwimi	rFlags,r3,0,30,31
	CYCLES(8)
	SAVE_FLAGS									// update the flags
	b		executeLoopEnd
	
entry static im_1
	li		r3,1
	rlwimi	rFlags,r3,0,30,31
	CYCLES(8)
	SAVE_FLAGS									// update the flags
	b		executeLoopEnd
	
entry static im_2
	li		r3,2
	rlwimi	rFlags,r3,0,30,31
	CYCLES(8)
	SAVE_FLAGS									// update the flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		IN: read from an input port
	//
entry static in_a_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_A(r3)
	b		executeLoopEnd
	
entry static in_b_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_B(r3)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static in_c_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_C(r3)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static in_d_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_D(r3)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static in_e_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_E(r3)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static in_h_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_H(r3)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static in_l_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	SET_L(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static in_0_c
	SAVE_PCAF									// save PC/AF
	GET_BC(r3)
	CYCLES(12)
	bl		AZ80_READPORT						// perform the read
	rlwinm	r3,r3,0,24,31						// keep it in range
	lbzx	rTempSave,rFlagTable,r3				// look up the ZSP flags
	bl		postExtern							// post-process
	rlwimi	rPCAF,rTempSave,0,24,30				// set all but the carry flag
	b		executeLoopEnd

entry static in_a_byte
	READ_OPCODE_ARG(r3)							// read the port number from PC
	SAVE_PCAF									// save PC/AF
	CYCLES(11)
	rlwimi	r3,rPCAF,0,16,23					// port = (A << 8) | imm8 (according to nicola's code!!)
	bl		AZ80_READPORT						// perform the read
	bl		postExtern							// post-process
	SET_A(r3)
	b		executeLoopEnd

	//================================================================================================

	//
	//		INC: increment an 8-bit value
	//
inc0:
	addi	r4,r3,1								// r4 = r3 + 1
	rlwinm	rPCAF,rPCAF,0,31,23					// clear out all but the carry
	rlwinm	r4,r4,0,24,31						// keep result in range
	subi	r3,r4,0x80							// subtract 0x80 from the result
	rlwinm	r5,r4,0,28,31						// get low 4 bits in r5
	rlwimi	rPCAF,r4,0,24,24					// set the S flag
	cntlzw	r6,r4								// r6 = number of zeros in result
	cntlzw	r5,r5								// r5 = number of zeros in low 4 bits
	rlwimi	rPCAF,r6,1,25,25					// set the Z flag
	cntlzw	r3,r3								// r3 = 32 if the original value was 0x80
	rlwimi	rPCAF,r5,31,27,27					// set the H flag
	rlwimi	rPCAF,r3,29,29,29					// set the V flag
	blr
	
entry static inc_xhl
	CYCLES(11)
	READ_AT_REG_SAVE_EA(HL)
	bl		inc0
	b		executeLoopEndWriteEA
	
entry static inc_xix
	CYCLES(4)
	bl		ixplusoffset_save					// automatically adds 19 cycles
	bl		inc0
	b		executeLoopEndWriteEA
	
entry static inc_xiy
	CYCLES(4)
	bl		iyplusoffset_save					// automatically adds 19 cycles
	bl		inc0
	b		executeLoopEndWriteEA

entry static inc_a
	GET_A(r3)
	bl		inc0
	SET_A(r4)
	CYCLES(4)
	b		executeLoopEnd
	
entry static inc_b
	GET_B(r3)
	bl		inc0
	SET_B(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inc_c
	GET_C(r3)
	bl		inc0
	SET_C(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inc_d
	GET_D(r3)
	bl		inc0
	SET_D(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static inc_e
	GET_E(r3)
	bl		inc0
	SET_E(r4)
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static inc_h
	GET_H(r3)
	bl		inc0
	SET_H(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inc_l
	GET_L(r3)
	bl		inc0
	SET_L(r4)
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inc_ixh
	GET_IXH(r3)
	bl		inc0
	SET_IXH(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static inc_ixl
	GET_IXL(r3)
	bl		inc0
	SET_IXL(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static inc_iyh
	GET_IYH(r3)
	bl		inc0
	SET_IYH(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static inc_iyl
	GET_IYL(r3)
	bl		inc0
	SET_IYL(r4)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd
	
	//================================================================================================

	//
	//		INC: increment a 16-bit value
	//
entry static inc_bc
	addi	r3,rHLBC,1
	SET_BC(r3)
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inc_de
	addi	r3,rSPDE,1
	SET_DE(r3)
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static inc_hl
	addis	rHLBC,rHLBC,1
	CYCLES(6)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static inc_ix
	addis	rIXIY,rIXIY,1
	CYCLES(10)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static inc_iy
	addi	r3,rIXIY,1
	SET_IY(r3)
	CYCLES(10)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static inc_sp
	addis	rSPDE,rSPDE,1
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		IND/INI: in and advance
	//
indcommon:
	CYCLES(16)
	subi	rTempSave,rHLBC,0x0100
	SAVE_PCAF									// save PC/AF
	mflr	rEA
	GET_BC(r3)
	bl		AZ80_READPORT						// perform the read
	bl		postExtern							// post-process
	rlwimi	rHLBC,rTempSave,0,16,23
	rlwinm	r4,r3,0,24,31
	WRITE_AT_REG(HL)
	rlwinm.	rTempSave,rTempSave,0,16,23
	mtlr	rEA
	cntlzw	r3,rTempSave
	ori		rPCAF,rPCAF,kZ80FlagN
	subis	rHLBC,rHLBC,1
	rlwimi	rPCAF,r3,1,25,25
	SAVE_HLBC
	blr

inicommon:
	CYCLES(16)
	subi	rTempSave,rHLBC,0x0100
	SAVE_PCAF									// save PC/AF
	mflr	rEA
	GET_BC(r3)
	bl		AZ80_READPORT						// perform the read
	bl		postExtern							// post-process
	rlwimi	rHLBC,rTempSave,0,16,23
	rlwinm	r4,r3,0,24,31
	WRITE_AT_REG(HL)
	rlwinm.	rTempSave,rTempSave,0,16,23
	mtlr	rEA
	cntlzw	r3,rTempSave
	ori		rPCAF,rPCAF,kZ80FlagN
	addis	rHLBC,rHLBC,1
	rlwimi	rPCAF,r3,1,25,25
	SAVE_HLBC
	blr

entry static ind
	bl		indcommon
	b		executeLoopEnd

entry static indr
	bl		indcommon
	beq-	executeLoopEnd
	subis	rPCAF,rPCAF,2
	CYCLES(5)
	b		executeLoopEnd
	
entry static ini
	bl		inicommon
	b		executeLoopEnd

entry static inir
	bl		inicommon
	beq-	executeLoopEnd
	subis	rPCAF,rPCAF,2
	CYCLES(5)
	b		executeLoopEnd

	//================================================================================================

	//
	//		JP: jump to an absolute address
	//
entry static jp_hl
	rlwimi	rPCAF,rHLBC,0,0,15
	CYCLES(4)
	UPDATE_BANK
	b		executeLoopEnd

entry static jp_ix
	rlwimi	rPCAF,rIXIY,0,0,15
	CYCLES(8)
	UPDATE_BANK
	b		executeLoopEnd

entry static jp_iy
	rlwimi	rPCAF,rIXIY,16,0,15
	CYCLES(8)
	UPDATE_BANK
	b		executeLoopEnd

entry static jp_c
	andi.	r0,rPCAF,kZ80FlagC
	bne		DoJP
SkipJP:
	CYCLES(10)
	addis	rPCAF,rPCAF,2						// skip the address
	b		executeLoopEnd

entry static jp
DoJP:
	READ_OPCODE_ARG_WORD(r3)					// load the calling address
	subis	rEA,rPCAF,2
	SET_PC(r3)									// set the new PC
	UPDATE_BANK
	xor.	r0,rEA,rPCAF
	CYCLES(10)
	bne+	executeLoopEnd
	cmpwi	rICount,0
	ble		executeLoopEnd
	li		rICount,0
	b		executeLoopEnd

entry static jp_m
	andi.	r0,rPCAF,kZ80FlagS
	bne		DoJP
	b		SkipJP

entry static jp_nc
	andi.	r0,rPCAF,kZ80FlagC
	beq		DoJP
	b		SkipJP

entry static jp_nz
	andi.	r0,rPCAF,kZ80FlagZ
	beq		DoJP
	b		SkipJP

entry static jp_p
	andi.	r0,rPCAF,kZ80FlagS
	beq		DoJP
	b		SkipJP

entry static jp_pe
	andi.	r0,rPCAF,kZ80FlagV
	bne		DoJP
	b		SkipJP

entry static jp_po
	andi.	r0,rPCAF,kZ80FlagV
	beq		DoJP
	b		SkipJP

entry static jp_z
	andi.	r0,rPCAF,kZ80FlagZ
	bne		DoJP
	b		SkipJP

	//================================================================================================

	//
	//		JR: jump to a relative address
	//
entry static jr_c
	andi.	r0,rPCAF,kZ80FlagC
	bne		DoJR
SkipJR:
	CYCLES(7)
	addis	rPCAF,rPCAF,1						// skip the offset
	b		executeLoopEnd

entry static jr
DoJR:
	READ_OPCODE_ARG(r3)							// load the jump offset
	subis	rEA,rPCAF,2
	extsb	r3,r3;								// sign extend the byte
	rlwinm	r4,r3,16,0,15;						// shift it high
	add		rPCAF,rPCAF,r4;						// update the PC
	UPDATE_BANK
	xor.	r0,rEA,rPCAF
	CYCLES(12)
	bne+	executeLoopEnd
	cmpwi	rICount,0
	ble		executeLoopEnd
	li		rICount,0
	b		executeLoopEnd

entry static jr_nc
	andi.	r0,rPCAF,kZ80FlagC
	beq		DoJR
	b		SkipJR

entry static jr_nz
	andi.	r0,rPCAF,kZ80FlagZ
	beq		DoJR
	b		SkipJR

entry static jr_z
	andi.	r0,rPCAF,kZ80FlagZ
	bne		DoJR
	b		SkipJR

	//================================================================================================

	//
	//		LD: transfer data between register and memory
	//

	//
	//		LD (rr),A
	//
entry static ld_xbc_a
	GET_A(r4)
	CYCLES(7)
	WRITE_AT_REG_SAVE(BC)
	b		executeLoopEnd

entry static ld_xde_a
	GET_A(r4)
	CYCLES(7)
	WRITE_AT_REG_SAVE(DE)
	b		executeLoopEnd

	//
	//		LD (HL),r
	//
entry static ld_xhl_a
	GET_A(r4)
wrhl7:
	CYCLES(7)
	WRITE_AT_REG_SAVE(HL)
	b		executeLoopEnd

entry static ld_xhl_b
	GET_B(r4)
	b		wrhl7

entry static ld_xhl_c
	GET_C(r4)
	b		wrhl7

entry static ld_xhl_d
	GET_D(r4)
	b		wrhl7

entry static ld_xhl_e
	GET_E(r4)
	b		wrhl7

entry static ld_xhl_h
	GET_H(r4)
	b		wrhl7

entry static ld_xhl_l
	GET_L(r4)
	b		wrhl7

entry static ld_xhl_byte
	READ_OPCODE_ARG(r4)
	CYCLES(10)
	WRITE_AT_REG_SAVE(HL)
	b		executeLoopEnd

	//
	//		LD (IX+n),r
	//
entry static ld_xix_a
	GET_A(r4)
wrix19:
	CYCLES(19)
	WRITE_AT_REG_OFFSET_SAVE(IX)
	b		executeLoopEnd

entry static ld_xix_b
	GET_B(r4)
	b		wrix19

entry static ld_xix_c
	GET_C(r4)
	b		wrix19

entry static ld_xix_d
	GET_D(r4)
	b		wrix19

entry static ld_xix_e
	GET_E(r4)
	b		wrix19

entry static ld_xix_h
	GET_H(r4)
	b		wrix19

entry static ld_xix_l
	GET_L(r4)
	b		wrix19

entry static ld_xix_byte
	READ_OPCODE_ARG(r3)							// r3 = offset
	GET_IX(r5)									// r5 = IX
	extsb	r3,r3								// sign extend the offset
	READ_OPCODE_ARG(r4)							// r4 = immediate value
	add		r3,r3,r5							// r3 = IX + offset
	CYCLES(19)
	WRITE_BYTE_SAVE(r3)							// write the value
	b		executeLoopEnd

	//
	//		LD (IY+n),r
	//
entry static ld_xiy_a
	GET_A(r4)
wriy19:
	CYCLES(19)
	WRITE_AT_REG_OFFSET_SAVE(IY)
	b		executeLoopEnd

entry static ld_xiy_b
	GET_B(r4)
	b		wriy19

entry static ld_xiy_c
	GET_C(r4)
	b		wriy19

entry static ld_xiy_d
	GET_D(r4)
	b		wriy19

entry static ld_xiy_e
	GET_E(r4)
	b		wriy19

entry static ld_xiy_h
	GET_H(r4)
	b		wriy19

entry static ld_xiy_l
	GET_L(r4)
	b		wriy19

entry static ld_xiy_byte
	READ_OPCODE_ARG(r3)							// r3 = offset
	GET_IY(r5)									// r5 = IY
	extsb	r3,r3								// sign extend the offset
	READ_OPCODE_ARG(r4)							// r4 = immediate value
	add		r3,r3,r5							// r3 = IY + offset
	CYCLES(19)
	WRITE_BYTE_SAVE(r3)							// write the value
	b		executeLoopEnd

	//
	//		LD (nn),A
	//
entry static ld_xbyte_a
	GET_A(r4)
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(13)
	WRITE_BYTE_SAVE(r3)
	b		executeLoopEnd

	//
	//		LD (nn),rr
	//
entry static ld_xword_bc
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	WRITE_WORD_LO_SAVE(rEA,rHLBC)
	b		executeLoopEnd

entry static ld_xword_de
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	WRITE_WORD_LO_SAVE(rEA,rSPDE)
	b		executeLoopEnd

entry static ld_xword_hl
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(16)
	WRITE_WORD_HI_SAVE(rEA,rHLBC)
	b		executeLoopEnd

entry static ld_xword_ix
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	WRITE_WORD_HI_SAVE(rEA,rIXIY)
	b		executeLoopEnd

entry static ld_xword_iy
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	WRITE_WORD_LO_SAVE(rEA,rIXIY)
	b		executeLoopEnd

entry static ld_xword_sp
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	WRITE_WORD_HI_SAVE(rEA,rSPDE)
	b		executeLoopEnd

	//
	//		LD A,(rr)
	//
entry static ld_a_xbc
	CYCLES(7)
	READ_AT_REG_SAVE(BC)
	SET_A(r3)
	b		executeLoopEnd

entry static ld_a_xde
	CYCLES(7)
	READ_AT_REG_SAVE(DE)
	SET_A(r3)
	b		executeLoopEnd

entry static ld_a_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_A(r3)
	b		executeLoopEnd

entry static ld_a_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_A(r3)
	b		executeLoopEnd

entry static ld_a_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_A(r3)
	b		executeLoopEnd

entry static ld_a_xbyte
	READ_OPCODE_ARG_WORD(r3)
	SAVE_PCAF									// save PC/AF
	CYCLES(13)
	bl		READMEM								// perform the read
	SET_A(r3)
	bl		postExtern							// post-process
	b		executeLoopEnd

	//
	//		LD r,(HL)
	//
entry static ld_b_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_B(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_C(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_d_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_D(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_E(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_h_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_H(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	SET_L(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD r,(IX+n)
	//
entry static ld_b_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_B(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_C(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_d_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_D(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_E(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_h_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_H(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	SET_L(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD r,(IY+n)
	//
entry static ld_b_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_B(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_C(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_d_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_D(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_E(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_h_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_H(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	SET_L(r3)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD rr,(nn)
	//
entry static ld_bc_xword
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	READ_WORD_SAVE(rEA)
	SET_BC(rTempSave)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_bc_word
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_BC(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_de_xword
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	READ_WORD_SAVE(rEA)
	SET_DE(rTempSave)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_de_word
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_DE(r3)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_hl_xword
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(16)
	READ_WORD_SAVE(rEA)
	SET_HL(rTempSave)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_hl_word
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_HL(r3)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_ix_xword
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	READ_WORD_SAVE(rEA)
	SET_IX(rTempSave)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ix_word
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(14)
	SET_IX(r3)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iy_xword
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	READ_WORD_SAVE(rEA)
	SET_IY(rTempSave)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iy_word
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(14)
	SET_IY(r3)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_sp_xword
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(20)
	READ_WORD_SAVE(rEA)
	SET_SP(rTempSave)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_sp_word
	READ_OPCODE_ARG_WORD(r3)
	CYCLES(10)
	SET_SP(r3)
	SAVE_SPDE
	b		executeLoopEnd

	//================================================================================================

	//
	//		LD: transfer immediate data to a register
	//
entry static ld_a_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	SET_A(r3)
	b		executeLoopEnd

entry static ld_b_byte
	READ_OPCODE_ARG(r3)
	SET_B(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_byte
	READ_OPCODE_ARG(r3)
	SET_C(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_d_byte
	READ_OPCODE_ARG(r3)
	SET_D(r3)
	CYCLES(7)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_byte
	READ_OPCODE_ARG(r3)
	SET_E(r3)
	CYCLES(7)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_h_byte
	READ_OPCODE_ARG(r3)
	SET_H(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_byte
	READ_OPCODE_ARG(r3)
	SET_L(r3)
	CYCLES(7)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_ixh_byte
	READ_OPCODE_ARG(r3)
	SET_IXH(r3)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_byte
	READ_OPCODE_ARG(r3)
	SET_IXL(r3)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyh_byte
	READ_OPCODE_ARG(r3)
	SET_IYH(r3)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_byte
	READ_OPCODE_ARG(r3)
	SET_IYL(r3)
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

	//================================================================================================

	//
	//		LD: transfer from register to register
	//

	//
	//		LD A,r
	//
entry static ld_a_a
	CYCLES(4)
	b		executeLoopEnd

entry static ld_a_b
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,0,16,23
	b		executeLoopEnd

entry static ld_a_c
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,8,16,23
	b		executeLoopEnd

entry static ld_a_d
	CYCLES(4)
	rlwimi	rPCAF,rSPDE,0,16,23
	b		executeLoopEnd

entry static ld_a_e
	CYCLES(4)
	rlwimi	rPCAF,rSPDE,8,16,23
	b		executeLoopEnd

entry static ld_a_h
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,16,16,23
	b		executeLoopEnd

entry static ld_a_l
	CYCLES(4)
	rlwimi	rPCAF,rHLBC,24,16,23
	b		executeLoopEnd

entry static ld_a_ixh
	CYCLES(9)
	rlwimi	rPCAF,rIXIY,16,16,23
	b		executeLoopEnd

entry static ld_a_ixl
	CYCLES(9)
	rlwimi	rPCAF,rIXIY,24,16,23
	b		executeLoopEnd

entry static ld_a_iyh
	CYCLES(9)
	rlwimi	rPCAF,rIXIY,0,16,23
	b		executeLoopEnd

entry static ld_a_iyl
	CYCLES(9)
	rlwimi	rPCAF,rIXIY,8,16,23
	b		executeLoopEnd

	//
	//		LD B,r
	//
entry static ld_b_b
	CYCLES(4)
	b		executeLoopEnd

entry static ld_b_a
	rlwimi	rHLBC,rPCAF,0,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_c
	rlwimi	rHLBC,rHLBC,8,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_d
	rlwimi	rHLBC,rSPDE,0,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_e
	rlwimi	rHLBC,rSPDE,8,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_h
	rlwimi	rHLBC,rHLBC,16,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_l
	rlwimi	rHLBC,rHLBC,24,16,23
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_ixh
	rlwimi	rHLBC,rIXIY,16,16,23
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_ixl
	rlwimi	rHLBC,rIXIY,24,16,23
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_iyh
	rlwimi	rHLBC,rIXIY,0,16,23
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_b_iyl
	rlwimi	rHLBC,rIXIY,8,16,23
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD C,r
	//
entry static ld_c_c
	CYCLES(4)
	b		executeLoopEnd

entry static ld_c_a
	rlwimi	rHLBC,rPCAF,24,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_b
	rlwimi	rHLBC,rHLBC,24,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_d
	rlwimi	rHLBC,rSPDE,24,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_e
	rlwimi	rHLBC,rSPDE,0,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_h
	rlwimi	rHLBC,rHLBC,8,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_l
	rlwimi	rHLBC,rHLBC,16,24,31
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_ixh
	rlwimi	rHLBC,rIXIY,8,24,31
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_ixl
	rlwimi	rHLBC,rIXIY,16,24,31
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_iyh
	rlwimi	rHLBC,rIXIY,24,24,31
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_c_iyl
	rlwimi	rHLBC,rIXIY,0,24,31
	CYCLES(9)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD D,r
	//
entry static ld_d_d
	CYCLES(4)
	b		executeLoopEnd

entry static ld_d_a
	rlwimi	rSPDE,rPCAF,0,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_b
	rlwimi	rSPDE,rHLBC,0,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_c
	rlwimi	rSPDE,rHLBC,8,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_e
	rlwimi	rSPDE,rSPDE,8,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_h
	rlwimi	rSPDE,rHLBC,16,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_l
	rlwimi	rSPDE,rHLBC,24,16,23
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_ixh
	rlwimi	rSPDE,rIXIY,16,16,23
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_ixl
	rlwimi	rSPDE,rIXIY,24,16,23
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_iyh
	rlwimi	rSPDE,rIXIY,0,16,23
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_d_iyl
	rlwimi	rSPDE,rIXIY,8,16,23
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

	//
	//		LD E,r
	//
entry static ld_e_e
	CYCLES(4)
	b		executeLoopEnd

entry static ld_e_a
	rlwimi	rSPDE,rPCAF,24,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_b
	rlwimi	rSPDE,rHLBC,24,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_c
	rlwimi	rSPDE,rHLBC,0,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_d
	rlwimi	rSPDE,rSPDE,24,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_h
	rlwimi	rSPDE,rHLBC,8,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_l
	rlwimi	rSPDE,rHLBC,16,24,31
	CYCLES(4)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_ixh
	rlwimi	rSPDE,rIXIY,8,24,31
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_ixl
	rlwimi	rSPDE,rIXIY,16,24,31
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_iyh
	rlwimi	rSPDE,rIXIY,24,24,31
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_e_iyl
	rlwimi	rSPDE,rIXIY,0,24,31
	CYCLES(9)
	SAVE_SPDE
	b		executeLoopEnd

	//
	//		LD H,r
	//
entry static ld_h_h
	CYCLES(4)
	b		executeLoopEnd

entry static ld_h_a
	rlwimi	rHLBC,rPCAF,16,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_h_b
	rlwimi	rHLBC,rHLBC,16,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_h_c
	rlwimi	rHLBC,rHLBC,24,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_h_d
	rlwimi	rHLBC,rSPDE,16,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_h_e
	rlwimi	rHLBC,rSPDE,24,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_h_l
	rlwimi	rHLBC,rHLBC,8,0,7
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD L,r
	//
entry static ld_l_l
	CYCLES(4)
	b		executeLoopEnd

entry static ld_l_a
	rlwimi	rHLBC,rPCAF,8,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_b
	rlwimi	rHLBC,rHLBC,8,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_c
	rlwimi	rHLBC,rHLBC,16,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_d
	rlwimi	rHLBC,rSPDE,8,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_e
	rlwimi	rHLBC,rSPDE,16,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

entry static ld_l_h
	rlwimi	rHLBC,rHLBC,24,8,15
	CYCLES(4)
	SAVE_HLBC
	b		executeLoopEnd

	//
	//		LD IXH,r
	//
entry static ld_ixh_a
	rlwimi	rIXIY,rPCAF,16,0,7
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixh_b
	rlwimi	rIXIY,rHLBC,16,0,7
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixh_c
	rlwimi	rIXIY,rHLBC,24,0,7
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixh_d
	rlwimi	rIXIY,rSPDE,16,0,7
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixh_e
	rlwimi	rIXIY,rSPDE,24,0,7
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixh_ixh
	CYCLES(9)
	b		executeLoopEnd

entry static ld_ixh_ixl
	rlwimi	rIXIY,rIXIY,8,0,7
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

	//
	//		LD IXL,r
	//
entry static ld_ixl_a
	rlwimi	rIXIY,rPCAF,8,8,15
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_b
	rlwimi	rIXIY,rHLBC,8,8,15
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_c
	rlwimi	rIXIY,rHLBC,16,8,15
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_d
	rlwimi	rIXIY,rSPDE,8,8,15
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_e
	rlwimi	rIXIY,rSPDE,16,8,15
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_ixh
	rlwimi	rIXIY,rIXIY,16,8,15
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_ixl_ixl
	CYCLES(9)
	b		executeLoopEnd

	//
	//		LD IYH,r
	//
entry static ld_iyh_a
	rlwimi	rIXIY,rPCAF,0,16,23
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyh_b
	rlwimi	rIXIY,rHLBC,0,16,23
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyh_c
	rlwimi	rIXIY,rHLBC,8,16,23
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyh_d
	rlwimi	rIXIY,rSPDE,0,16,23
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyh_e
	rlwimi	rIXIY,rSPDE,8,16,23
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyh_iyh
	CYCLES(9)
	b		executeLoopEnd

entry static ld_iyh_iyl
	rlwimi	rIXIY,rIXIY,8,16,23
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

	//
	//		LD IYL,r
	//
entry static ld_iyl_a
	rlwimi	rIXIY,rPCAF,24,24,31
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_b
	rlwimi	rIXIY,rHLBC,24,24,31
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_c
	rlwimi	rIXIY,rHLBC,0,24,31
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_d
	rlwimi	rIXIY,rSPDE,24,24,31
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_e
	rlwimi	rIXIY,rSPDE,0,24,31
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_iyh
	rlwimi	rIXIY,rIXIY,24,24,31
	CYCLES(9)
	SAVE_IXIY
	b		executeLoopEnd

entry static ld_iyl_iyl
	CYCLES(9)
	b		executeLoopEnd

	//
	//		LD SP,rr
	//
entry static ld_sp_hl
	rlwimi	rSPDE,rHLBC,0,0,15
	CYCLES(6)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_sp_ix
	rlwimi	rSPDE,rIXIY,0,0,15
	CYCLES(10)
	SAVE_SPDE
	b		executeLoopEnd

entry static ld_sp_iy
	rlwimi	rSPDE,rIXIY,16,0,15
	CYCLES(10)
	SAVE_SPDE
	b		executeLoopEnd

	//
	//		LD A,I
	//
entry static ld_a_i
	rlwinm	rPCAF,rPCAF,0,31,23					// clear all flags but the carry
	rlwinm	r3,rFlags,24,24,31					// r3 = I
	rlwimi	rPCAF,rFlags,0,16,23				// A = I
	cntlzw	r4,r3								// r4 = 32 if I==0
	rlwimi	rPCAF,r3,0,24,24					// set the S flag if I<0
	CYCLES(9)
	rlwimi	rPCAF,r4,1,25,25					// set the Z flag if I==0
	rlwimi	rPCAF,rFlags,28,29,29				// set the V flag (==IFF2)
	b		executeLoopEnd

	//
	//		LD I,A
	//
entry static ld_i_a
	rlwimi	rFlags,rPCAF,0,16,23
	CYCLES(9)
	SAVE_FLAGS									// update the flags
	b		executeLoopEnd

	//
	//		LD A,R
	//
entry static ld_a_r
	rlwinm	r3,rFlags,8,25,31					// r3 = R & 127
	rlwinm	rPCAF,rPCAF,0,31,23					// clear all flags but the carry
	rlwimi	r3,rFlags,16,24,24					// r3 = (R&127)|(R2&128)
	CYCLES(9)
	SET_A(r3)									// A = (R&127)|(R2&128)
	cntlzw	r4,r3								// r4 = 32 if R==0
	rlwimi	rPCAF,r3,0,24,24					// set the S flag if R<0
	rlwimi	rPCAF,r4,1,25,25					// set the Z flag if R==0
	rlwimi	rPCAF,rFlags,28,29,29				// set the V flag (==IFF2)
	b		executeLoopEnd

	//
	//		LD R,A
	//
entry static ld_r_a
	rlwimi	rFlags,rPCAF,16,0,7					// R = A
	CYCLES(9)
	rlwimi	rFlags,rPCAF,8,8,15					// R2 = A
	SAVE_FLAGS									// update the flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		LDI/LDD: copy and repeat
	//
lddcommon:
	CYCLES(16)
	mflr	rEA
	READ_AT_REG_SAVE(HL)						// r3 = value at (HL)
	rlwinm	r4,r3,0,24,31						// r4 = r3
	rlwinm	rPCAF,rPCAF,0,31,26					// clear N,V,H
	WRITE_AT_REG(DE)							// write value to (DE)
	subi	r3,rHLBC,1							// r3 = BC-1
	mtlr	rEA
	subi	r4,rSPDE,1							// r4 = DE-1
	rlwinm.	r5,r3,0,16,31						// r5 = (BC-1)&0xffff
	subis	rHLBC,rHLBC,1						// HL--
	cntlzw	r5,r5								// r5 = 32 if (BC-1)==0
	SET_BC(r3)									// BC = BC-1
	not		r5,r5								// invert bit 0x20
	SAVE_HLBC
	SET_DE(r4)									// DE = DE-1
	rlwimi	rPCAF,r5,29,29,29					// set the V flag if BC != 0
	SAVE_SPDE
	blr

ldicommon:
	CYCLES(16)
	mflr	rEA
	READ_AT_REG_SAVE(HL)						// r3 = value at (HL)
	rlwinm	r4,r3,0,24,31						// r4 = r3
	rlwinm	rPCAF,rPCAF,0,31,26					// clear N,V,H
	WRITE_AT_REG(DE)							// write value to (DE)
	subi	r3,rHLBC,1							// r3 = BC-1
	mtlr	rEA
	addi	r4,rSPDE,1							// r4 = DE+1
	rlwinm.	r5,r3,0,16,31						// r5 = (BC-1)&0xffff
	addis	rHLBC,rHLBC,1						// HL++
	cntlzw	r5,r5								// r5 = 32 if (BC-1)==0
	SET_BC(r3)									// BC = BC-1
	not		r5,r5								// invert bit 0x20
	SAVE_HLBC
	SET_DE(r4)									// DE = DE-1
	rlwimi	rPCAF,r5,29,29,29					// set the V flag if BC != 0
	SAVE_SPDE
	blr

entry static ldd
	bl		lddcommon
	b		executeLoopEnd

entry static lddr
	bl		lddcommon
	beq-	executeLoopEnd
	subis	rPCAF,rPCAF,2
	CYCLES(5)
	b		executeLoopEnd

entry static ldi
	bl		ldicommon
	b		executeLoopEnd

entry static ldir
	bl		ldicommon
	beq-	executeLoopEnd
	subis	rPCAF,rPCAF,2
	CYCLES(5)
	b		executeLoopEnd

	//================================================================================================

	//
	//		NEG: negate A
	//
entry static neg
	GET_A(r3)
	rlwinm	rPCAF,rPCAF,0,24,15
	CYCLES(8)
	b		sub0

	//================================================================================================

	//
	//		NOP: do nothing
	//
entry static nop
	CYCLES(4)
	b		executeLoopEnd

	//================================================================================================

	//
	//		OR: logical OR with A
	//
entry static or_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		or0

entry static or_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		or0

entry static or_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		or0

entry static or_a
	GET_A(r3)
or4:
	CYCLES(4)
or0:
	GET_A(r4)									// r4 = R.AF.B.h
	or			r4,r4,r3						// r4 = R.AF.B.h|Reg
	lbzx	r5,rFlagTable,r4					// r5 = flag bits
	SET_A(r4)									// R.AF.B.h = q
	rlwimi	rPCAF,r5,0,24,31					// set the flags
	b		executeLoopEnd

entry static or_b
	GET_B(r3)
	b		or4

entry static or_c
	GET_C(r3)
	b		or4

entry static or_d
	GET_D(r3)
	b		or4

entry static or_e
	GET_E(r3)
	b		or4

entry static or_h
	GET_H(r3)
	b		or4

entry static or_l
	GET_L(r3)
	b		or4

entry static or_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		or0

entry static or_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		or0

entry static or_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		or0

entry static or_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		or0

entry static or_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		or0

	//================================================================================================

	//
	//		OUTD/OUTI: out and repeat
	//
outdcommon:
	CYCLES(16)
	mflr	rEA
	subi	rTempSave,rHLBC,0x0100
	READ_AT_REG_SAVE(HL)
	rlwimi	rHLBC,rTempSave,0,16,23
	mr		r4,r3
	GET_BC(r3)									// get the register
	bl		AZ80_WRITEPORT						// perform the write
	bl		postExtern							// post-process
	rlwinm.	rTempSave,rTempSave,0,16,23
	mtlr	rEA
	cntlzw	r3,rTempSave
	ori		rPCAF,rPCAF,kZ80FlagN
	subis	rHLBC,rHLBC,1
	rlwimi	rPCAF,r3,1,25,25
	SAVE_HLBC
	blr

outicommon:
	CYCLES(16)
	mflr	rEA
	subi	rTempSave,rHLBC,0x0100
	READ_AT_REG_SAVE(HL)
	rlwimi	rHLBC,rTempSave,0,16,23
	mr		r4,r3
	GET_BC(r3)									// get the register
	bl		AZ80_WRITEPORT						// perform the write
	bl		postExtern							// post-process
	rlwinm.	rTempSave,rTempSave,0,16,23
	mtlr	rEA
	cntlzw	r3,rTempSave
	ori		rPCAF,rPCAF,kZ80FlagN
	addis	rHLBC,rHLBC,1
	rlwimi	rPCAF,r3,1,25,25
	SAVE_HLBC
	blr

entry static outd
	bl		outdcommon
	b		executeLoopEnd

entry static otdr
	bl		outdcommon
	beq-	executeLoopEnd
	subis	rPCAF,rPCAF,2
	CYCLES(5)
	b		executeLoopEnd
	
entry static outi
	bl		outicommon
	b		executeLoopEnd

entry static otir
	bl		outicommon
	beq-	executeLoopEnd
	subis	rPCAF,rPCAF,2
	CYCLES(5)
	b		executeLoopEnd

	//================================================================================================

	//
	//		OUT: output a byte
	//
entry static out_c_a
	GET_A(r4)
out12:
	SAVE_PCAF									// save PC/AF
	CYCLES(12)
	GET_BC(r3)									// get the register
	bl		AZ80_WRITEPORT						// perform the write
	bl		postExtern							// post-process
	b		executeLoopEnd
	
entry static out_c_b
	GET_B(r4)
	b		out12
	
entry static out_c_c
	GET_C(r4)
	b		out12
	
entry static out_c_d
	GET_D(r4)
	b		out12
	
entry static out_c_e
	GET_E(r4)
	b		out12
	
entry static out_c_h
	GET_H(r4)
	b		out12
	
entry static out_c_l
	GET_L(r4)
	b		out12

entry static out_c_0
	li		r4,0
	b		out12

entry static out_byte_a
	CYCLES(11)
	READ_OPCODE_ARG(r3)							// read the port number from PC
	SAVE_PCAF									// save PC/AF
	GET_A(r4)									// get the value from A
	rlwimi	r3,r4,8,16,23						// port = (A << 8) | imm8 (according to Nicola's code!)
	bl		AZ80_WRITEPORT	 					// perform the write
	bl		postExtern							// post-process
	b		executeLoopEnd

	//================================================================================================

	//
	//		POP: pop a value from the stack
	//
entry static pop_af
	CYCLES(10)
	POP_LO_SAVE(rPCAF)
	b		executeLoopEnd
	
entry static pop_bc
	CYCLES(10)
	POP_LO_SAVE(rHLBC)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static pop_de
	CYCLES(10)
	POP_LO_SAVE(rSPDE)
//	SAVE_SPDE -- already saved because of SP change
	b		executeLoopEnd
	
entry static pop_hl
	CYCLES(10)
	POP_HI_SAVE(rHLBC)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static pop_ix
	CYCLES(14)
	POP_HI_SAVE(rIXIY)
	SAVE_IXIY
	b		executeLoopEnd
	
entry static pop_iy
	CYCLES(14)
	POP_LO_SAVE(rIXIY)
	SAVE_IXIY
	b		executeLoopEnd

	//================================================================================================

	//
	//		PUSH: push a value onto the stack
	//
entry static push_af
	CYCLES(11)
	PUSH_LO_SAVE(rPCAF)
	b		executeLoopEnd
	
entry static push_bc
	CYCLES(11)
	PUSH_LO_SAVE(rHLBC)
	b		executeLoopEnd
	
entry static push_de
	CYCLES(11)
	PUSH_LO_SAVE(rSPDE)
	b		executeLoopEnd
	
entry static push_hl
	CYCLES(11)
	PUSH_HI_SAVE(rHLBC)
	b		executeLoopEnd
	
entry static push_ix
	CYCLES(15)
	PUSH_HI_SAVE(rIXIY)
	b		executeLoopEnd
	
entry static push_iy
	CYCLES(15)
	PUSH_LO_SAVE(rIXIY)
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_0: clear bit 0 of a value
	//
entry static res_0_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_0_xix
entry static res_0_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,0,0)
	b		executeLoopEndWriteEA

entry static res_0_a
	RES(rPCAF,rPCAF,0,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_0_b
	RES(rHLBC,rHLBC,0,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_0_c
	RES(rHLBC,rHLBC,0,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_0_d
	RES(rSPDE,rSPDE,0,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_0_e
	RES(rSPDE,rSPDE,0,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_0_h
	RES(rHLBC,rHLBC,0,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_0_l
	RES(rHLBC,rHLBC,0,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_1: clear bit 1 of a value
	//
entry static res_1_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_1_xix
entry static res_1_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,1,0)
	b		executeLoopEndWriteEA

entry static res_1_a
	RES(rPCAF,rPCAF,1,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_1_b
	RES(rHLBC,rHLBC,1,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_1_c
	RES(rHLBC,rHLBC,1,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_1_d
	RES(rSPDE,rSPDE,1,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_1_e
	RES(rSPDE,rSPDE,1,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_1_h
	RES(rHLBC,rHLBC,1,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_1_l
	RES(rHLBC,rHLBC,1,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_2: clear bit 2 of a value
	//
entry static res_2_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_2_xix
entry static res_2_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,2,0)
	b		executeLoopEndWriteEA

entry static res_2_a
	RES(rPCAF,rPCAF,2,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_2_b
	RES(rHLBC,rHLBC,2,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_2_c
	RES(rHLBC,rHLBC,2,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_2_d
	RES(rSPDE,rSPDE,2,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_2_e
	RES(rSPDE,rSPDE,2,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_2_h
	RES(rHLBC,rHLBC,2,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_2_l
	RES(rHLBC,rHLBC,2,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_3: clear bit 3 of a value
	//
entry static res_3_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_3_xix
entry static res_3_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,3,0)
	b		executeLoopEndWriteEA

entry static res_3_a
	RES(rPCAF,rPCAF,3,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_3_b
	RES(rHLBC,rHLBC,3,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_3_c
	RES(rHLBC,rHLBC,3,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_3_d
	RES(rSPDE,rSPDE,3,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_3_e
	RES(rSPDE,rSPDE,3,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_3_h
	RES(rHLBC,rHLBC,3,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_3_l
	RES(rHLBC,rHLBC,3,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_4: clear bit 4 of a value
	//
entry static res_4_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_4_xix
entry static res_4_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,4,0)
	b		executeLoopEndWriteEA

entry static res_4_a
	RES(rPCAF,rPCAF,4,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_4_b
	RES(rHLBC,rHLBC,4,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_4_c
	RES(rHLBC,rHLBC,4,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_4_d
	RES(rSPDE,rSPDE,4,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_4_e
	RES(rSPDE,rSPDE,4,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_4_h
	RES(rHLBC,rHLBC,4,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_4_l
	RES(rHLBC,rHLBC,4,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_5: clear bit 5 of a value
	//
entry static res_5_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_5_xix
entry static res_5_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,5,0)
	b		executeLoopEndWriteEA

entry static res_5_a
	RES(rPCAF,rPCAF,5,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_5_b
	RES(rHLBC,rHLBC,5,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_5_c
	RES(rHLBC,rHLBC,5,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_5_d
	RES(rSPDE,rSPDE,5,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_5_e
	RES(rSPDE,rSPDE,5,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_5_h
	RES(rHLBC,rHLBC,5,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_5_l
	RES(rHLBC,rHLBC,5,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_6: clear bit 6 of a value
	//
entry static res_6_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_6_xix
entry static res_6_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,6,0)
	b		executeLoopEndWriteEA

entry static res_6_a
	RES(rPCAF,rPCAF,6,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_6_b
	RES(rHLBC,rHLBC,6,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_6_c
	RES(rHLBC,rHLBC,6,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_6_d
	RES(rSPDE,rSPDE,6,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_6_e
	RES(rSPDE,rSPDE,6,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_6_h
	RES(rHLBC,rHLBC,6,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_6_l
	RES(rHLBC,rHLBC,6,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RES_7: clear bit 7 of a value
	//
entry static res_7_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static res_7_xix
entry static res_7_xiy
	// note: setup is already handled in the opcode handler
	RES(r4,r3,7,0)
	b		executeLoopEndWriteEA

entry static res_7_a
	RES(rPCAF,rPCAF,7,8)
	CYCLES(8)
	b		executeLoopEnd

entry static res_7_b
	RES(rHLBC,rHLBC,7,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_7_c
	RES(rHLBC,rHLBC,7,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_7_d
	RES(rSPDE,rSPDE,7,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_7_e
	RES(rSPDE,rSPDE,7,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static res_7_h
	RES(rHLBC,rHLBC,7,24)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static res_7_l
	RES(rHLBC,rHLBC,7,16)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		RET: return from a subroutine
	//
entry static ret
	CYCLES(10)
	b		DoRetCore

entry static ret_c
	andi.	r0,rPCAF,kZ80FlagC
	bne		DoRet

SkipRet:
	CYCLES(5)
	b		executeLoopEnd

DoRet:
	CYCLES(11)
DoRetCore:
	POP_HI_SAVE(rPCAF)							// pop the PC
	UPDATE_BANK
	b		executeLoopEnd

entry static ret_m
	andi.	r0,rPCAF,kZ80FlagS
	bne		DoRet
	b		SkipRet

entry static ret_nc
	andi.	r0,rPCAF,kZ80FlagC
	beq		DoRet
	b		SkipRet

entry static ret_nz
	andi.	r0,rPCAF,kZ80FlagZ
	beq		DoRet
	b		SkipRet

entry static ret_p
	andi.	r0,rPCAF,kZ80FlagS
	beq		DoRet
	b		SkipRet

entry static ret_pe
	andi.	r0,rPCAF,kZ80FlagV
	bne		DoRet
	b		SkipRet

entry static ret_po
	andi.	r0,rPCAF,kZ80FlagV
	beq		DoRet
	b		SkipRet

entry static ret_z
	andi.	r0,rPCAF,kZ80FlagZ
	bne		DoRet
	b		SkipRet

	//================================================================================================

	//
	//		RETI/RETN: return from an interrupt
	//
entry static reti
	CYCLES(14)
	POP_HI_SAVE(rPCAF)							// pop the PC
	UPDATE_BANK
	_asm_set_global(rFlags,sFlags)
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	_asm_set_global(rIXIY,sIXIY)
	_asm_set_global(rAFDE2,sAFDE2)
	_asm_set_global(rHLBC2,sHLBC2)
	bl		DaisyChainIRQ
	bl		postExtern
	b		executeLoopEnd
	
entry static retn
	POP_HI_SAVE(rPCAF)							// pop the PC
	UPDATE_BANK
	_asm_get_global_b(r9,sNMINestLevel)
	rlwinm.	r0,rFlags,EXTRACT_IFF1				// check IFF1
	cmpwi	cr1,r9,0
	rlwimi	rFlags,rFlags,1,24,24				// IFF1 = IFF2
	subi	r9,r9,1
	SAVE_FLAGS									// update the flags
	beq		cr1,retnnodec
	_asm_set_global_b(r9,sNMINestLevel)
retnnodec:
	bne		retnskip							// if !IFF1 && IFF2, we check for pending IRQs
	rlwinm.	r0,rFlags,EXTRACT_IFF2
	beq		retnskip
	_asm_set_global(rPCAF,sPCAF)
	_asm_set_global(rSPDE,sSPDE)
	_asm_set_global(rHLBC,sHLBC)
	_asm_set_global(rIXIY,sIXIY)
	_asm_set_global(rAFDE2,sAFDE2)
	_asm_set_global(rHLBC2,sHLBC2)
	bl		CheckIRQ
	bl		postExtern
retnskip:
	CYCLES(14)
	b		executeLoopEnd

	//================================================================================================

	//
	//		RL: rotate a value left
	//
entry static rl_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static rl_xix
entry static rl_xiy
	// note: setup is already handled in the opcode handler
	RL(r3,0)
	b		executeLoopEndWriteEA

entry static rl_a
	RL(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static rl_b
	RL(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rl_c
	RL(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rl_d
	RL(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rl_e
	RL(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rl_h
	RL(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rl_l
	RL(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static rla
	rlwinm	r4,rPCAF,1,16,22					// r4 = A << 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,8,23,23					// r4 = (A << 1) | (F & 1)
	rlwimi	r3,rPCAF,17,31,31					// r3 = (rPCAF & 0xec) | (A >> 7)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RLC: rotate a value left through the carry
	//
entry static rlc_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static rlc_xix
entry static rlc_xiy
	// note: setup is already handled in the opcode handler
	RLC(r3,0)
	b		executeLoopEndWriteEA

entry static rlc_a
	RLC(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static rlc_b
	RLC(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rlc_c
	RLC(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rlc_d
	RLC(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rlc_e
	RLC(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rlc_h
	RLC(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rlc_l
	RLC(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static rlca
	rlwinm	r4,rPCAF,1,16,22					// r4 = A << 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,25,23,23					// r4 = (A << 1) | (A >> 7)
	rlwimi	r3,rPCAF,17,31,31					// r3 = (rPCAF & 0xec) | (A >> 7)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RLD: highly strange instruction
	//
entry static rld
	CYCLES(18)
	READ_AT_REG_SAVE_EA(HL)						// r3 = (HL)
	rlwinm	r4,rPCAF,24,28,31					// r4 = A & 15
	rlwimi	rPCAF,r3,4,20,23					// A = (A & 0xf0) | (r3 >> 4)
	rlwimi	r4,r3,4,24,27						// r4 = (r3 << 4) | (A & 15)
	GET_A(r3)									// r3 = A
	lbzx	r5,rFlagTable,r3					// r5 = ZSP[A]
	rlwimi	rPCAF,r5,0,24,30					// insert all but the carry
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		RR: rotate a value right
	//
entry static rr_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static rr_xix
entry static rr_xiy
	// note: setup is already handled in the opcode handler
	RR(r3,0)
	b		executeLoopEndWriteEA

entry static rr_a
	RR(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static rr_b
	RR(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rr_c
	RR(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rr_d
	RR(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rr_e
	RR(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rr_h
	RR(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rr_l
	RR(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static rra
	rlwinm	r4,rPCAF,31,17,23					// r4 = A >> 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,15,16,16					// r4 = (A >> 1) | ((F & 1) << 7)
	rlwimi	r3,rPCAF,24,31,31					// r3 = (rPCAF & 0xec) | (A & 1)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RRC: rotate a value right through the carry
	//
entry static rrc_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static rrc_xix
entry static rrc_xiy
	// note: setup is already handled in the opcode handler
	RRC(r3,0)
	b		executeLoopEndWriteEA

entry static rrc_a
	RRC(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static rrc_b
	RRC(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rrc_c
	RRC(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rrc_d
	RRC(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rrc_e
	RRC(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static rrc_h
	RRC(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static rrc_l
	RRC(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static rrca
	rlwinm	r4,rPCAF,31,17,23					// r4 = A >> 1
	andi.	r3,rPCAF,0xec						// r3 = rPCAF & 0xec
	rlwimi	r4,rPCAF,7,16,16					// r4 = (A >> 1) | (A << 7)
	rlwimi	r3,rPCAF,24,31,31					// r3 = (rPCAF & 0xec) | (A & 1)
	rlwimi	rPCAF,r4,0,16,23					// set A
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,31					// set the new flags
	b		executeLoopEnd

	//================================================================================================

	//
	//		RRD: another highly strange instruction
	//
entry static rrd
	CYCLES(18)
	READ_AT_REG_SAVE_EA(HL)						// r3 = (HL)
	rlwinm	r4,rPCAF,28,24,27					// r4 = A << 4
	rlwimi	rPCAF,r3,8,20,23					// A = (A & 0xf0) | (r3 & 15)
	rlwimi	r4,r3,28,28,31						// r4 = (r3 >> 4) | (A << 4)
	GET_A(r3)									// r3 = A
	lbzx	r5,rFlagTable,r3					// r5 = ZSP[A]
	rlwimi	rPCAF,r5,0,24,30					// insert all but the carry
	b		executeLoopEndWriteEA

	//================================================================================================

	//
	//		RST: software interrupt
	//
entry static rst_00
	li		rTempSave,0x00
rst:
	CYCLES(11)
	PUSH_HI_SAVE(rPCAF)
	SET_PC(rTempSave)
	UPDATE_BANK
	b		executeLoopEnd
	
entry static rst_08
	li		rTempSave,0x08
	b		rst
	
entry static rst_10
	li		rTempSave,0x10
	b		rst
	
entry static rst_18
	li		rTempSave,0x18
	b		rst
	
entry static rst_20
	li		rTempSave,0x20
	b		rst
	
entry static rst_28
	li		rTempSave,0x28
	b		rst
	
entry static rst_30
	li		rTempSave,0x30
	b		rst
	
entry static rst_38
	li		rTempSave,0x38
	b		rst

	//================================================================================================

	//		SBC_A: subtract with carry from A
	//
entry static sbc_a_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		sbc0

entry static sbc_a_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		sbc0

entry static sbc_a_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		sbc0

entry static sbc_a_a
	GET_A(r3)
sbc4:
	CYCLES(4)
sbc0:
	GET_A(r4)									// r4 = R.AF.B.h
	rlwinm	r5,rPCAF,0,31,31					// r5 = R.AF.B.l&1
	rlwinm	rPCAF,rPCAF,0,0,23					// clear all flags
	xor		r7,r3,r4							// r7 = Reg^R.AF.B.h
	sub		r6,r4,r3							// r6 = R.AF.B.h-Reg
	ori		rPCAF,rPCAF,kZ80FlagN				// set the N flag
	sub		r6,r6,r5							// r6 = q = R.AF.B.h-Reg-(R.AF.B.l&1)
	rlwimi	rPCAF,r6,24,31,31					// set the C flag
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,0,24,24					// set the S flag
	xor		r8,r7,r6							// r8 = Reg^R.AF.B.h^q
	rlwinm	r6,r6,0,24,31						// r6 &= 0xff
	rlwimi	rPCAF,r8,0,27,27					// set the H flag
	and		r8,r7,r9							// r8 = (Reg^R.AF.B.h)&(Reg^q)
	SET_A(r6)									// R.AF.B.h = q
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// set the V flag
	rlwimi	rPCAF,r7,1,25,25					// set the Z flag
	b		executeLoopEnd

entry static sbc_a_b
	GET_B(r3)
	b		sbc4

entry static sbc_a_c
	GET_C(r3)
	b		sbc4

entry static sbc_a_d
	GET_D(r3)
	b		sbc4

entry static sbc_a_e
	GET_E(r3)
	b		sbc4

entry static sbc_a_h
	GET_H(r3)
	b		sbc4

entry static sbc_a_l
	GET_L(r3)
	b		sbc4

entry static sbc_a_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		sbc0

entry static sbc_a_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		sbc0

entry static sbc_a_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		sbc0

entry static sbc_a_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		sbc0

entry static sbc_a_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		sbc0

	//================================================================================================

	//
	//		SBC_HL: subtract with carry from HL
	//
entry static sbc_hl_bc
	GET_BC(r3)
sbchl15:
	GET_HL(r4)									// r4 = R.AF.B.h
	rlwinm	r5,rPCAF,0,31,31					// r5 = R.AF.B.l&1
	rlwinm	rPCAF,rPCAF,0,0,23					// clear all flags
	xor		r7,r3,r4							// r7 = Reg^R.AF.B.h
	sub		r6,r4,r3							// r6 = R.AF.B.h-Reg
	ori		rPCAF,rPCAF,kZ80FlagN				// set the N flag
	sub		r6,r6,r5							// r6 = q = R.AF.B.h-Reg-(R.AF.B.l&1)
	rlwimi	rPCAF,r6,16,31,31					// set the C flag
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,24,24,24					// set the S flag
	xor		r8,r7,r6							// r8 = Reg^R.AF.B.h^q
	rlwinm	r6,r6,0,16,31						// r6 &= 0xff
	rlwimi	rPCAF,r8,24,27,27					// set the H flag
	and		r8,r7,r9							// r8 = (Reg^R.AF.B.h)&(Reg^q)
	SET_HL(r6)									// R.AF.B.h = q
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,19,29,29					// set the V flag
	CYCLES(15)
	rlwimi	rPCAF,r7,1,25,25					// set the Z flag
	SAVE_HLBC
	b		executeLoopEnd

entry static sbc_hl_de
	GET_DE(r3)
	b		sbchl15

entry static sbc_hl_hl
	GET_HL(r3)
	b		sbchl15

entry static sbc_hl_sp
	GET_SP(r3)
	b		sbchl15

	//================================================================================================

	//
	//		SCF: set the carry flag
	//
entry static scf
	andi.	r3,rPCAF,0xec
	ori		rPCAF,rPCAF,kZ80FlagC
	CYCLES(4)
	rlwimi	rPCAF,r3,0,24,30
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_0: set bit 0 of a value
	//
entry static set_0_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_0_xix
entry static set_0_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,0,0)
	b		executeLoopEndWriteEA

entry static set_0_a
	SET_LO(rPCAF,rPCAF,0,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_0_b
	SET_LO(rHLBC,rHLBC,0,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_0_c
	SET_LO(rHLBC,rHLBC,0,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_0_d
	SET_LO(rSPDE,rSPDE,0,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_0_e
	SET_LO(rSPDE,rSPDE,0,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_0_h
	SET_HI(rHLBC,rHLBC,0,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_0_l
	SET_HI(rHLBC,rHLBC,0,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_1: set bit 1 of a value
	//
entry static set_1_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_1_xix
entry static set_1_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,1,0)
	b		executeLoopEndWriteEA

entry static set_1_a
	SET_LO(rPCAF,rPCAF,1,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_1_b
	SET_LO(rHLBC,rHLBC,1,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_1_c
	SET_LO(rHLBC,rHLBC,1,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_1_d
	SET_LO(rSPDE,rSPDE,1,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_1_e
	SET_LO(rSPDE,rSPDE,1,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_1_h
	SET_HI(rHLBC,rHLBC,1,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_1_l
	SET_HI(rHLBC,rHLBC,1,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_2: set bit 2 of a value
	//
entry static set_2_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_2_xix
entry static set_2_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,2,0)
	b		executeLoopEndWriteEA

entry static set_2_a
	SET_LO(rPCAF,rPCAF,2,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_2_b
	SET_LO(rHLBC,rHLBC,2,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_2_c
	SET_LO(rHLBC,rHLBC,2,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_2_d
	SET_LO(rSPDE,rSPDE,2,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_2_e
	SET_LO(rSPDE,rSPDE,2,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_2_h
	SET_HI(rHLBC,rHLBC,2,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_2_l
	SET_HI(rHLBC,rHLBC,2,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_3: set bit 3 of a value
	//
entry static set_3_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_3_xix
entry static set_3_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,3,0)
	b		executeLoopEndWriteEA

entry static set_3_a
	SET_LO(rPCAF,rPCAF,3,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_3_b
	SET_LO(rHLBC,rHLBC,3,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_3_c
	SET_LO(rHLBC,rHLBC,3,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_3_d
	SET_LO(rSPDE,rSPDE,3,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_3_e
	SET_LO(rSPDE,rSPDE,3,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_3_h
	SET_HI(rHLBC,rHLBC,3,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_3_l
	SET_HI(rHLBC,rHLBC,3,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_4: set bit 4 of a value
	//
entry static set_4_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_4_xix
entry static set_4_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,4,0)
	b		executeLoopEndWriteEA

entry static set_4_a
	SET_LO(rPCAF,rPCAF,4,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_4_b
	SET_LO(rHLBC,rHLBC,4,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_4_c
	SET_LO(rHLBC,rHLBC,4,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_4_d
	SET_LO(rSPDE,rSPDE,4,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_4_e
	SET_LO(rSPDE,rSPDE,4,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_4_h
	SET_HI(rHLBC,rHLBC,4,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_4_l
	SET_HI(rHLBC,rHLBC,4,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_5: set bit 5 of a value
	//
entry static set_5_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_5_xix
entry static set_5_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,5,0)
	b		executeLoopEndWriteEA

entry static set_5_a
	SET_LO(rPCAF,rPCAF,5,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_5_b
	SET_LO(rHLBC,rHLBC,5,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_5_c
	SET_LO(rHLBC,rHLBC,5,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_5_d
	SET_LO(rSPDE,rSPDE,5,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_5_e
	SET_LO(rSPDE,rSPDE,5,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_5_h
	SET_HI(rHLBC,rHLBC,5,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_5_l
	SET_HI(rHLBC,rHLBC,5,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_6: set bit 6 of a value
	//
entry static set_6_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_6_xix
entry static set_6_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,6,0)
	b		executeLoopEndWriteEA

entry static set_6_a
	SET_LO(rPCAF,rPCAF,6,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_6_b
	SET_LO(rHLBC,rHLBC,6,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_6_c
	SET_LO(rHLBC,rHLBC,6,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_6_d
	SET_LO(rSPDE,rSPDE,6,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_6_e
	SET_LO(rSPDE,rSPDE,6,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_6_h
	SET_HI(rHLBC,rHLBC,6,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_6_l
	SET_HI(rHLBC,rHLBC,6,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SET_7: set bit 7 of a value
	//
entry static set_7_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static set_7_xix
entry static set_7_xiy
	// note: setup is already handled in the opcode handler
	SET_LO(r4,r3,7,0)
	b		executeLoopEndWriteEA

entry static set_7_a
	SET_LO(rPCAF,rPCAF,7,8)
	CYCLES(8)
	b		executeLoopEnd

entry static set_7_b
	SET_LO(rHLBC,rHLBC,7,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_7_c
	SET_LO(rHLBC,rHLBC,7,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_7_d
	SET_LO(rSPDE,rSPDE,7,8)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_7_e
	SET_LO(rSPDE,rSPDE,7,0)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd

entry static set_7_h
	SET_HI(rHLBC,rHLBC,7,8)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

entry static set_7_l
	SET_HI(rHLBC,rHLBC,7,0)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SLA: shift a value left arithmetically
	//
entry static sla_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static sla_xix
entry static sla_xiy
	// note: setup is already handled in the opcode handler
	SLA(r3,0)
	b		executeLoopEndWriteEA

entry static sla_a
	SLA(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static sla_b
	SLA(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sla_c
	SLA(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sla_d
	SLA(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static sla_e
	SLA(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static sla_h
	SLA(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sla_l
	SLA(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SLL: shift a value left logically
	//
entry static sll_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static sll_xix
entry static sll_xiy
	// note: setup is already handled in the opcode handler
	SLL(r3,0)
	b		executeLoopEndWriteEA

entry static sll_a
	SLL(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static sll_b
	SLL(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sll_c
	SLL(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sll_d
	SLL(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static sll_e
	SLL(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static sll_h
	SLL(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sll_l
	SLL(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SRA: shift a value right arithmetically
	//
entry static sra_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static sra_xix
entry static sra_xiy
	// note: setup is already handled in the opcode handler
	SRA(r3,0)
	b		executeLoopEndWriteEA

entry static sra_a
	SRA(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static sra_b
	SRA(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sra_c
	SRA(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sra_d
	SRA(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static sra_e
	SRA(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static sra_h
	SRA(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static sra_l
	SRA(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SRL: shift a value right logically
	//
entry static srl_xhl
	CYCLES(15)
	READ_AT_REG_SAVE_EA(HL)

entry static srl_xix
entry static srl_xiy
	// note: setup is already handled in the opcode handler
	SRL(r3,0)
	b		executeLoopEndWriteEA

entry static srl_a
	SRL(rPCAF,8)
	SET_A(r4)
	CYCLES(8)
	b		executeLoopEnd
	
entry static srl_b
	SRL(rHLBC,8)
	SET_B(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static srl_c
	SRL(rHLBC,0)
	SET_C(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static srl_d
	SRL(rSPDE,8)
	SET_D(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static srl_e
	SRL(rSPDE,0)
	SET_E(r4)
	CYCLES(8)
	SAVE_SPDE
	b		executeLoopEnd
	
entry static srl_h
	SRL(rHLBC,24)
	SET_H(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd
	
entry static srl_l
	SRL(rHLBC,16)
	SET_L(r4)
	CYCLES(8)
	SAVE_HLBC
	b		executeLoopEnd

	//================================================================================================

	//
	//		SUB_A: subtract a value from A
	//
entry static sub_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		sub0

entry static sub_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		sub0

entry static sub_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		sub0

entry static sub_a
	GET_A(r3)
sub4:
	CYCLES(4)
sub0:
	GET_A(r4)									// r4 = A
	rlwinm	rPCAF,rPCAF,0,0,23					// N = S = Z = H = V = C = 0
	xor		r7,r3,r4							// r8 = A ^ r3
	ori		rPCAF,rPCAF,kZ80FlagN				// N = 1
	sub		r6,r4,r3							// r6 = A' = A - r3
	mr		r8,r7								// r8 = A ^ r3
	rlwimi	rPCAF,r6,24,31,31					// C = A' & 0x100
	xor		r9,r4,r6							// r9 = A ^ tmp
	rlwimi	rPCAF,r6,0,24,24					// S = A' & 0x80
	xor		r8,r7,r6							// r8 = r3 ^ A ^ A'
	rlwinm	r6,r6,0,24,31						// r6 = A' &= 0xff
	rlwimi	rPCAF,r8,0,27,27					// set the H flag
	and		r8,r7,r9							// r8 = (r3 ^ A) & (r3 ^ A')
	SET_A(r6)									// A = A'
	cntlzw	r7,r6								// r7 = number of zeros in r6
	rlwimi	rPCAF,r8,27,29,29					// V = (A ^ r3) & (r3 ^ A') & 0x80
	rlwimi	rPCAF,r7,1,25,25					// set the Z flag
	b		executeLoopEnd

entry static sub_b
	GET_B(r3)
	b		sub4

entry static sub_c
	GET_C(r3)
	b		sub4

entry static sub_d
	GET_D(r3)
	b		sub4

entry static sub_e
	GET_E(r3)
	b		sub4

entry static sub_h
	GET_H(r3)
	b		sub4

entry static sub_l
	GET_L(r3)
	b		sub4

entry static sub_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		sub0

entry static sub_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		sub0

entry static sub_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		sub0

entry static sub_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		sub0

entry static sub_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		sub0

	//================================================================================================

	//
	//		XOR: logical exclusive OR with A
	//
entry static xor_xhl
	CYCLES(7)
	READ_AT_REG_SAVE(HL)
	b		xor0

entry static xor_xix
	bl		ixplusoffset_save					// automatically adds 19 cycles
	b		xor0

entry static xor_xiy
	bl		iyplusoffset_save					// automatically adds 19 cycles
	b		xor0

entry static xor_a
	GET_A(r3)
xor4:
	CYCLES(4)
xor0:
	GET_A(r4)									// r4 = R.AF.B.h
	xor		r4,r4,r3							// r4 = R.AF.B.h^Reg
	lbzx	r5,rFlagTable,r4					// r5 = flag bits
	SET_A(r4)									// R.AF.B.h = q
	rlwimi	rPCAF,r5,0,24,31					// set the flags
	b		executeLoopEnd

entry static xor_b
	GET_B(r3)
	b		xor4

entry static xor_c
	GET_C(r3)
	b		xor4

entry static xor_d
	GET_D(r3)
	b		xor4

entry static xor_e
	GET_E(r3)
	b		xor4

entry static xor_h
	GET_H(r3)
	b		xor4

entry static xor_l
	GET_L(r3)
	b		xor4

entry static xor_ixh
	GET_IXH(r3)
	CYCLES(9)
	b		xor0

entry static xor_ixl
	GET_IXL(r3)
	CYCLES(9)
	b		xor0

entry static xor_iyh
	GET_IYH(r3)
	CYCLES(9)
	b		xor0

entry static xor_iyl
	GET_IYL(r3)
	CYCLES(9)
	b		xor0

entry static xor_byte
	READ_OPCODE_ARG(r3)
	CYCLES(7)
	b		xor0
}
