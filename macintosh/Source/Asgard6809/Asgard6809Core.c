//###################################################################################################
//
//
//		Asgard6809Core.c
//		See Asgard6809DefaultConfig.h for compile-time configuration and optimization.
//
//		A PowerPC assembly Motorola 6809 emulation core written by Aaron Giles
//		This code is free for use in any emulation project as long as the following credit 
//		appears in the about box and documentation:
//
//			PowerPC-optimized 6809 emulation provided by Aaron Giles and the MAME project.
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
//		10/19/97	1.0		First MAME version
//		11/19/97	1.1		Added support for separating opcode reads from opcode arg reads
//		01/10/99	1.2		LBO - modified for new interrupt system
//		03/07/99	1.3		LBO - skank fix to work with Juergen's no-pending stuff
//		03/12/99	2.0		Reformatted and rewrote to handle interrupts immediately
//							Changed the interface significantly
//		03/12/99	2.1		Changed reset to only clear DP and set the I and F flags
//							Added NMI inhibiting until S is set
//							Renamed a bunch of internal variables to be more Mac-like
//							Moved the debugger to its own file
//		03/15/99	2.2		Added GetReg/SetReg interface
//		07/07/03	2.1		LBO - added Mach-O support
//
//
//###################################################################################################


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Asgard6809Core.h"
#include "Asgard6809Internals.h"

#if TARGET_RT_MAC_CFM
// we assume TOC-based data; this forces it on
#pragma toc_data on
#endif


//###################################################################################################
//	CONSTANTS
//###################################################################################################

enum
{
	// these flags mimic MAME behavior
	kInterruptStateCWAI	  		= 0x01,		// set when CWAI is waiting for an interrupt
	kInterruptStateSYNC			= 0x02,		// set when SYNC is waiting for an interrupt
	kInterruptStateInhibitNMI	= 0x04,		// set to inhibit the NMI interrupt

	// these flags allow interrupts to generated from within an I/O callback
	kExitActionGenerateNMI		= 0x01,
	kExitActionGenerateIRQ		= 0x02,
	kExitActionGenerateFIRQ		= 0x04
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef struct
{
	// context variables containing the 6809 registers
	unsigned long 			fXY;					// X and Y registers
	unsigned long 			fSU;					// S and U registers
	unsigned long 			fPCAB;					// PC, A, and B registers
	unsigned long 			fDPFlags;				// DP, CC, and internal flags

	// context variables describing the current 6809 interrupt state
	unsigned char 			fNMIState;				// current state of the NMI line
	unsigned char 			fIRQState;				// current state of the IRQ line
	unsigned char 			fFIRQState;				// current state of the FIRQ line
	unsigned char 			fInterruptState;		// current state of the internal SYNC/CWAI flags
	Asgard6809IRQCallback 	fIRQCallback;			// callback routine for IRQ lines
	signed long				fInterruptCycleAdjust;	// cycle count adjustment due to interrupts
} Asgard6809Context;
	

//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// externally defined variables
extern unsigned char *			A6809_OPCODEROM;		// pointer to the ROM base
extern unsigned char *			A6809_ARGUMENTROM;		// pointer to the argument ROM base
extern int						A6809_ICOUNT;			// cycles remaining to execute

#if (A6809_CHIP == 5000)
extern void 					(*A6809_SETLINES)(int);	// callback called when A16-A23 are set
#endif

// local variables for tracking cycles
static signed long 				sRequestedCycles;		// the originally requested number of cycles
static signed long 				sExitActionCycles;		// number of cycles removed to force an exit action
static unsigned char			sExitActions;			// current exit actions
static unsigned char			sExecuting;				// true if we're currently executing

// local variables containing the 6809 registers
static unsigned long 			sXY;					// X and Y registers
static unsigned long 			sSU;					// S and U registers
static unsigned long 			sPCAB;					// PC, A, and B registers
static unsigned long 			sDPFlags;				// DP, CC, and internal flags
static unsigned long 			sOpcodePC;				// contains the PC of the current opcode

// local variables describing the current 6809 interrupt state
static signed char 				sNMIState;				// current state of the NMI line
static signed char 				sIRQState;				// current state of the IRQ line
static signed char 				sFIRQState;				// current state of the FIRQ line
static unsigned char 			sInterruptState;		// current state of the internal SYNC/CWAI flags
static Asgard6809IRQCallback 	sIRQCallback;			// callback routine for IRQ lines
static signed long 				sInterruptCycleAdjust;	// cycle count adjustment due to interrupts


//###################################################################################################
//	FUNCTION TABLES
//###################################################################################################

typedef struct
{
	void *			fMain[0x100];
	void *			fEA[0x100];
#if (A6809_CHIP != 5000)
	void *			fExt10[0x100];
	void *			fExt11[0x100];
#endif
} OpcodeTable;

#if (A6809_CHIP != 5000)
static OpcodeTable sOpcodeTable =
{
	{
		neg_di, 	illegal, 	illegal, 	com_di, 	lsr_di, 	illegal, 	ror_di, 	asr_di, 	/*00*/
		asl_di, 	rol_di, 	dec_di, 	illegal, 	inc_di, 	tst_di, 	jmp_di, 	clr_di, 
		op_10, 		op_11,	 	nop, 		sync, 		illegal, 	illegal, 	lbra, 		lbsr, 		/*10*/
		illegal, 	daa, 		orcc, 		illegal, 	andcc, 		sex, 		exg, 		tfr, 
		bra, 		brn, 		bhi, 		bls, 		bcc, 		bcs, 		bne, 		beq, 		/*20*/
		bvc, 		bvs, 		bpl, 		bmi, 		bge, 		blt, 		bgt, 		ble, 
		leax, 		leay, 		leas, 		leau, 		pshs, 		puls, 		pshu, 		pulu, 		/*30*/
		illegal, 	rts, 		abx, 		rti, 		cwai, 		mul, 		illegal, 	swi, 
		nega, 		illegal, 	illegal, 	coma, 		lsra, 		illegal, 	rora, 		asra, 		/*40*/
		asla, 		rola, 		deca, 		illegal, 	inca, 		tsta, 		illegal, 	clra, 
		negb, 		illegal, 	illegal, 	comb, 		lsrb, 		illegal, 	rorb, 		asrb, 		/*50*/
		aslb, 		rolb, 		decb, 		illegal, 	incb, 		tstb,		illegal, 	clrb, 
		neg_ix, 	illegal, 	illegal, 	com_ix, 	lsr_ix, 	illegal, 	ror_ix, 	asr_ix, 	/*60*/
		asl_ix, 	rol_ix, 	dec_ix, 	illegal, 	inc_ix, 	tst_ix, 	jmp_ix, 	clr_ix, 
		neg_ex, 	illegal, 	illegal, 	com_ex, 	lsr_ex, 	illegal, 	ror_ex, 	asr_ex, 	/*70*/
		asl_ex, 	rol_ex, 	dec_ex, 	illegal, 	inc_ex, 	tst_ex, 	jmp_ex, 	clr_ex, 
		suba_im, 	cmpa_im, 	sbca_im, 	subd_im, 	anda_im, 	bita_im, 	lda_im, 	sta_im,		/*80*/
		eora_im, 	adca_im, 	ora_im, 	adda_im, 	cmpx_im, 	bsr, 		ldx_im, 	stx_im,
		suba_di, 	cmpa_di, 	sbca_di, 	subd_di, 	anda_di, 	bita_di, 	lda_di, 	sta_di, 	/*90*/
		eora_di, 	adca_di, 	ora_di, 	adda_di, 	cmpx_di, 	jsr_di, 	ldx_di, 	stx_di, 
		suba_ix, 	cmpa_ix, 	sbca_ix, 	subd_ix, 	anda_ix, 	bita_ix, 	lda_ix, 	sta_ix, 	/*A0*/
		eora_ix, 	adca_ix, 	ora_ix, 	adda_ix, 	cmpx_ix, 	jsr_ix, 	ldx_ix, 	stx_ix, 
		suba_ex, 	cmpa_ex, 	sbca_ex, 	subd_ex, 	anda_ex, 	bita_ex, 	lda_ex, 	sta_ex, 	/*B0*/
		eora_ex, 	adca_ex, 	ora_ex, 	adda_ex, 	cmpx_ex, 	jsr_ex, 	ldx_ex, 	stx_ex, 
		subb_im, 	cmpb_im, 	sbcb_im, 	addd_im, 	andb_im, 	bitb_im, 	ldb_im, 	stb_im,		/*C0*/
		eorb_im, 	adcb_im, 	orb_im, 	addb_im, 	ldd_im, 	std_im,		ldu_im, 	stu_im,
		subb_di, 	cmpb_di, 	sbcb_di, 	addd_di, 	andb_di, 	bitb_di, 	ldb_di, 	stb_di, 	/*D0*/
		eorb_di, 	adcb_di, 	orb_di, 	addb_di, 	ldd_di, 	std_di, 	ldu_di, 	stu_di, 
		subb_ix, 	cmpb_ix, 	sbcb_ix, 	addd_ix, 	andb_ix, 	bitb_ix, 	ldb_ix, 	stb_ix, 	/*E0*/
		eorb_ix, 	adcb_ix, 	orb_ix, 	addb_ix, 	ldd_ix, 	std_ix, 	ldu_ix, 	stu_ix, 
		subb_ex, 	cmpb_ex, 	sbcb_ex, 	addd_ex, 	andb_ex,	bitb_ex, 	ldb_ex, 	stb_ex, 	/*F0*/
		eorb_ex, 	adcb_ex, 	orb_ex, 	addb_ex, 	ldd_ex, 	std_ex, 	ldu_ex, 	stu_ex
	},
	{
		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		/*00*/
		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,	
		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		/*10*/
		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,		eaxpk,
		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		/*20*/
		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,	
		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		/*30*/
		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,		eaypk,	
		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		/*40*/
		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,	
		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		/*50*/
		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,		eaupk,	
		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		/*60*/
		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,	
		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		/*70*/
		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,		easpk,	
		eaxi,		eaxii,		eaxd,		eaxdd,		eax,		eaxpb,		eaxpa,		eaz,		/*80*/
		eaxp8,		eaxp16,		eaz,		eaxpd,		eapcp8,		eapcp16,	eaz,		ea16,	
		eaeaxi,		eaeaxii,	eaeaxd,		eaeaxdd,	eaeax,		eaeaxpb,	eaeaxpa,	eaz,		/*90*/
		eaeaxp8,	eaeaxp16,	eaz,		eaeaxpd,	eaeapcp8,	eaeapcp16,	eaz,		eaea16,	
		eayi,		eayii,		eayd,		eaydd,		eay,		eaypb,		eaypa,		eaz,		/*A0*/
		eayp8,		eayp16,		eaz,		eaypd,		eapcp8,		eapcp16,	eaz,		ea16,	
		eaeayi,		eaeayii,	eaeayd,		eaeaydd,	eaeay,		eaeaypb,	eaeaypa,	eaz,		/*B0*/
		eaeayp8,	eaeayp16,	eaz,		eaeaypd,	eaeapcp8,	eaeapcp16,	eaz,		eaea16,	
		eaui,		eauii,		eaud,		eaudd,		eau,		eaupb,		eaupa,		eaz,		/*C0*/
		eaup8,		eaup16,		eaz,		eaupd,		eapcp8,		eapcp16,	eaz,		ea16,	
		eaeaui,		eaeauii,	eaeaud,		eaeaudd,	eaeau,		eaeaupb,	eaeaupa,	eaz,		/*D0*/
		eaeaup8,	eaeaup16,	eaz,		eaeaupd,	eaeapcp8,	eaeapcp16,	eaz,		eaea16,	
		easi,		easii,		easd,		easdd,		eas,		easpb,		easpa,		eaz,		/*E0*/
		easp8,		easp16,		eaz,		easpd,		eapcp8,		eapcp16,	eaz,		ea16,	
		eaeasi,		eaeasii,	eaeasd,		eaeasdd,	eaeas,		eaeaspb,	eaeaspa,	eaz,		/*F0*/
		eaeasp8,	eaeasp16,	eaz,		eaeaspd,	eaeapcp8,	eaeapcp16,	eaz,		eaea16,	
	},
	{
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*00*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*10*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	lbrn,		lbhi,		lbls,		lbcc,		lbcs,		lbne,		lbeq,		/*20*/
		lbvc,		lbvs,		lbpl,		lbmi,		lbge,		lblt,		lbgt,		lble,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*30*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	swi2,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*40*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*50*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*60*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*70*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	cmpd_im,	illegal,	illegal,	illegal,	illegal,	/*80*/
		illegal,	illegal,	illegal,	illegal,	cmpy_im,	illegal,	ldy_im,		sty_im,
		illegal,	illegal,	illegal,	cmpd_di,	illegal,	illegal,	illegal,	illegal,	/*90*/
		illegal,	illegal,	illegal,	illegal,	cmpy_di,	illegal,	ldy_di,		sty_di,
		illegal,	illegal,	illegal,	cmpd_ix,	illegal,	illegal,	illegal,	illegal,	/*A0*/
		illegal,	illegal,	illegal,	illegal,	cmpy_ix,	illegal,	ldy_ix,		sty_ix,
		illegal,	illegal,	illegal,	cmpd_ex,	illegal,	illegal,	illegal,	illegal,	/*B0*/
		illegal,	illegal,	illegal,	illegal,	cmpy_ex,	illegal,	ldy_ex,		sty_ex,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*C0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	lds_im,		sts_im,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*D0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	lds_di,		sts_di,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*E0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	lds_ix,		sts_ix,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*F0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	lds_ex,		sts_ex
	},
	{
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*00*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*10*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*20*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*30*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	swi3,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*40*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*50*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*60*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*70*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	cmpu_im,	illegal,	illegal,	illegal,	illegal,	/*80*/
		illegal,	illegal,	illegal,	illegal,	cmps_im,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	cmpu_di,	illegal,	illegal,	illegal,	illegal,	/*90*/
		illegal,	illegal,	illegal,	illegal,	cmps_di,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	cmpu_ix,	illegal,	illegal,	illegal,	illegal,	/*A0*/
		illegal,	illegal,	illegal,	illegal,	cmps_ix,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	cmpu_ex,	illegal,	illegal,	illegal,	illegal,	/*B0*/
		illegal,	illegal,	illegal,	illegal,	cmps_ex,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*C0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*D0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*E0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	/*F0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal
	}
};
#else
static OpcodeTable sOpcodeTable =
{
	{
		illegal, 	illegal, 	illegal, 	illegal, 	illegal, 	illegal, 	illegal, 	illegal, 	/*00*/
		leax, 		leay,	 	leau,	 	leas,	 	pshs, 		pshu,	 	puls,	 	pulu, 
		lda_im,		ldb_im,	 	lda_ix,		ldb_ix, 	adda_im, 	addb_im, 	adda_ix,	addb_ix,	/*10*/
		adca_im, 	adcb_im, 	adca_ix,	adcb_ix, 	suba_im, 	subb_im, 	suba_ix, 	subb_ix, 
		sbca_im, 	sbcb_im, 	sbca_ix,	sbcb_ix,	anda_im,	andb_im, 	anda_ix,	andb_ix,	/*20*/
		bita_im, 	bitb_im, 	bita_ix, 	bitb_ix, 	eora_im,	eorb_im, 	eora_ix,	eorb_ix, 
		ora_im,		orb_im, 	ora_ix,		orb_ix, 	cmpa_im,	cmpb_im, 	cmpa_ix, 	cmpb_ix,	/*30*/
		setline_im,	setline_ix,	sta_ix, 	stb_ix, 	andcc,		orcc, 		exg,	 	tfr, 
		ldd_im,		ldd_ix, 	ldx_im, 	ldx_ix, 	ldy_im, 	ldy_ix, 	ldu_im, 	ldu_ix,		/*40*/
		lds_im,		lds_ix, 	cmpd_im,	cmpd_ix, 	cmpx_im, 	cmpx_ix, 	cmpy_im, 	cmpy_ix, 
		cmpu_im, 	cmpu_ix, 	cmps_im, 	cmps_ix, 	addd_im,	addd_ix, 	subd_im, 	subd_ix,	/*50*/
		std_ix,		stx_ix, 	sty_ix,		stu_ix, 	sts_ix,		illegal,	illegal, 	illegal, 
		bra, 		bhi,	 	bcc, 		bne,	 	bvc,	 	bpl,	 	bge, 		bgt,	 	/*60*/
		lbra,	 	lbhi,	 	lbcc,	 	lbne,	 	lbvc,	 	lbpl,	 	lbge,	 	lbgt, 
		brn,	 	bls,	 	bcs, 		beq, 		bvs, 		bmi,	 	blt,	 	ble, 		/*70*/
		lbrn,	 	lbls,	 	lbcs,	 	lbeq, 		lbvs, 		lbmi,	 	lblt,	 	lble, 
		clra,	 	clrb,	 	clr_ix, 	coma,	 	comb,	 	com_ix, 	nega,	 	negb,		/*80*/
		neg_ix, 	inca,	 	incb,	 	inc_ix, 	deca,	 	decb, 		dec_ix, 	rts,
		tsta, 		tstb, 		tst_ix, 	lsra,	 	lsrb,	 	lsr_ix, 	rora,	 	rorb,	 	/*90*/
		ror_ix, 	asra, 		asrb, 		asr_ix, 	asla,	 	aslb,	 	asl_ix, 	rti, 
		rola,	 	rolb,	 	rol_ix, 	lsrw_ix, 	rorw_ix, 	asrw_ix, 	aslw_ix, 	rolw_ix, 	/*A0*/
		jmp_ix, 	jsr_ix, 	bsr, 		lbsr,	 	decbjnz, 	decxjnz, 	nop,	 	illegal, 
		abx, 		daa, 		sex, 		mul,	 	lmul,	 	divx,	 	bmove,	 	move,	 	/*B0*/
		lsrd_im, 	lsrd_ix, 	rord_im, 	rord_ix, 	asrd_im, 	asrd_ix, 	asld_im, 	asld_ix, 
		rold_im,	rold_ix, 	clrd,	 	clrw_ix, 	negd,	 	negw_ix, 	incd,	 	incw_ix,	/*C0*/
		decd, 		decw_ix, 	tstd, 		tstw_ix, 	absa, 		absb,		absd, 		bset,
		bset2, 		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal, 	/*D0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal, 
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal, 	/*E0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal, 
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal, 	/*F0*/
		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal
	},
	{
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaex,		/*00*/
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaeaex,	
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		/*10*/
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,	
		eaxi,		eaxii,		eaxd,		eaxdd,		eaxp8,		eaxp16,		eax,		eaz,		/*20*/
		eaeaxi,		eaeaxii,	eaeaxd,		eaeaxdd,	eaeaxp8,	eaeaxp16,	eaeax,		eaz,	
		eayi,		eayii,		eayd,		eaydd,		eayp8,		eayp16,		eay,		eaz,		/*30*/
		eaeayi,		eaeayii,	eaeayd,		eaeaydd,	eaeayp8,	eaeayp16,	eaeay,		eaz,	
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		/*40*/
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,	
		eaui,		eauii,		eaud,		eaudd,		eaup8,		eaup16,		eau,		eaz,		/*50*/
		eaeaui,		eaeauii,	eaeaud,		eaeaudd,	eaeaup8,	eaeaup16,	eaeau,		eaz,	
		easi,		easii,		easd,		easdd,		easp8,		easp16,		eas,		eaz,		/*60*/
		eaeasi,		eaeasii,	eaeasd,		eaeasdd,	eaeasp8,	eaeasp16,	eaeas,		eaz,	
		eapci,		eapcii,		eapcd,		eapcdd,		eapcp8,		eapcp16,	eapc,		eaz,		/*70*/
		eaeapci,	eaeapcii,	eaeapcd,	eaeapcdd,	eaeapcp8,	eaeapcp16,	eaeapc,		eaz,	
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		/*80*/
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,	
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		/*90*/
		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,		eaz,	
		eaxpa,		eaxpb,		eaz,		eaz,		eaz,		eaz,		eaz,		eaxpd,		/*A0*/
		eaeaxpa,	eaeaxpb,	eaz,		eaz,		eaz,		eaz,		eaz,		eaeaxpd,	
		eaypa,		eaypb,		eaz,		eaz,		eaz,		eaz,		eaz,		eaypd,		/*B0*/
		eaeaypa,	eaeaypb,	eaz,		eaz,		eaz,		eaz,		eaz,		eaeaypd,	
		eaz,		eaz,		eaz,		eaz,		eadi,		eaz,		eaz,		eaz,		/*C0*/
		eaz,		eaz,		eaz,		eaz,		eaeadi,		eaz,		eaz,		eaz,	
		eaupa,		eaupb,		eaz,		eaz,		eaz,		eaz,		eaz,		eaupd,		/*D0*/
		eaeaupa,	eaeaupb,	eaz,		eaz,		eaz,		eaz,		eaz,		eaeaupd,	
		easpa,		easpb,		eaz,		eaz,		eaz,		eaz,		eaz,		easpd,		/*E0*/
		eaeaspa,	eaeaspb,	eaz,		eaz,		eaz,		eaz,		eaz,		eaeaspd,	
		eapcpa,		eapcpb,		eaz,		eaz,		eaz,		eaz,		eaz,		eapcpd,		/*F0*/
		eaeapcpa,	eaeapcpb,	eaz,		eaz,		eaz,		eaz,		eaz,		eaeapcpd	
	}
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
	sSU -= 0x00010000;
	WRITEMEM(sSU >> 16, inValue);
}


//###################################################################################################
//
//	PushWord -- push a word onto the stack
//
//###################################################################################################

static inline void PushWord(unsigned short inValue)
{
	PushByte(inValue & 0xff);
	PushByte(inValue >> 8);
}


//###################################################################################################
//
//	ProcessInterrupt -- push the state of the machine onto the stack, inhibit the requested
//						interrupts, and change the PC to the requested vector
//
//###################################################################################################

static inline void ProcessInterrupt(int inEntireState, int inInhibitWhich, int inPCOffset)
{
	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// if the state has already been pushed by CWAI, just clear the CWAI condition
	if (sInterruptState & kInterruptStateCWAI)
		sInterruptState &= ~kInterruptStateCWAI;
		
	// otherwise, if we're pushing the entire state, set the E bit and push it all
	else if (inEntireState)
	{
		sDPFlags |= k6809FlagE;
		PushWord(sPCAB >> 16);
		PushWord(sSU & 0xffff);
		PushWord(sXY & 0xffff);
		PushWord(sXY >> 16);
		PushByte((sDPFlags >> 8) & 0xff);
		PushByte(sPCAB & 0xff);
		PushByte((sPCAB >> 8) & 0xff);
		PushByte(sDPFlags & 0xff);
		sInterruptCycleAdjust += 12;
	}
	
	// otherwise, clear the E bit and push the short state
	else
	{
		sDPFlags &= ~k6809FlagE;
		PushWord(sPCAB >> 16);
		PushByte(sDPFlags & 0xff);
		sInterruptCycleAdjust += 3;
	}

	// inhibit the requested interrupts and mark the flags dirty
	sDPFlags |= inInhibitWhich | kDPFlagsDirty;
	
	// set the PC to the requested vector
	sPCAB = (READMEM(inPCOffset) << 24) + (READMEM(inPCOffset + 1) << 16) + (sPCAB & 0xffff);
#ifdef A6809_UPDATEBANK
	A6809_UPDATEBANK(sPCAB >> 16);
#endif

	// count 10 cycles to process the interrupt
	sInterruptCycleAdjust += 7;
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
	{
		sOpcodeTable.fMain[i] = *(void **)sOpcodeTable.fMain[i];
		sOpcodeTable.fEA[i] = *(void **)sOpcodeTable.fEA[i];
#if (A6809_CHIP != 5000)
		sOpcodeTable.fExt10[i] = *(void **)sOpcodeTable.fExt10[i];
		sOpcodeTable.fExt11[i] = *(void **)sOpcodeTable.fExt11[i];
#endif
	}
#endif
}


//###################################################################################################
//
//	ProcessNMI -- generates an NMI interrupt
//
//###################################################################################################

static void ProcessNMI(void)
{
	// NMI inhibits both IRQ and FIRQ
	ProcessInterrupt(true, k6809FlagF | k6809FlagI, 0xfffc);
}


//###################################################################################################
//
//	ProcessIRQ -- generates an IRQ interrupt
//
//###################################################################################################

static void ProcessIRQ(void)
{
	// IRQ inhibits IRQ only
	ProcessInterrupt(true, k6809FlagI, 0xfff8);
	if (sIRQCallback)
		(*sIRQCallback)(k6809IRQLineIRQ);
}


//###################################################################################################
//
//	ProcessFIRQ -- generates an FIRQ interrupt
//
//###################################################################################################

static void ProcessFIRQ(void)
{
	// FIRQ inhibits both IRQ and FIRQ
	ProcessInterrupt(false, k6809FlagF | k6809FlagI, 0xfff6);
	if (sIRQCallback)
		(*sIRQCallback)(k6809IRQLineFIRQ);
}


//###################################################################################################
//
//	ProcessSWI1 -- generates a software interrupt type 1
//
//###################################################################################################

static void ProcessSWI1(void)
{
	// SWI1 inhibits both IRQ and FIRQ
	ProcessInterrupt(true, k6809FlagF | k6809FlagI, 0xfffa);
}


//###################################################################################################
//
//	ProcessSWI2 -- generates a software interrupt type 2
//
//###################################################################################################

static void ProcessSWI2(void)
{
	// SWI2 inhibits nothing
	ProcessInterrupt(true, 0, 0xfff4);
}


//###################################################################################################
//
//	ProcessSWI3 -- generates a software interrupt type 3
//
//###################################################################################################

static void ProcessSWI3(void)
{
	// SWI3 inhibits nothing
	ProcessInterrupt(true, 0, 0xfff2);
}


//###################################################################################################
//
//	CheckIRQLines -- check the state of the IRQ lines
//
//	This function examines the current state of the IRQ lines and generates an interrupt
//	condition if all the proper conditions are met.
//
//###################################################################################################

static void CheckIRQLines(int inExecutingOkay)
{
	// see if the FIRQ line is asserted
	if (sFIRQState != k6809IRQStateClear)
	{
		sInterruptState &= ~kInterruptStateSYNC;
		
		// if FIRQ is enabled, process the interrupt and call the callback
		if ((sDPFlags & k6809FlagF) == 0)
		{
			// if we're inside the execution loop, just set the state bit
			if (!inExecutingOkay && sExecuting)
			{
				sExitActions |= kExitActionGenerateFIRQ;
				sExitActionCycles += A6809_ICOUNT;
				A6809_ICOUNT = 0;
			}
			
			// otherwise, process it right away
			else
				ProcessFIRQ();
		}
	}

	// see if the IRQ line is asserted
	if (sIRQState != k6809IRQStateClear)
	{
		sInterruptState &= ~kInterruptStateSYNC;

		// if IRQ is enabled, process the interrupt and call the callback
		if ((sDPFlags & k6809FlagI) == 0)
		{
			// if we're inside the execution loop, just set the state bit
			if (!inExecutingOkay && sExecuting)
			{
				sExitActions |= kExitActionGenerateIRQ;
				sExitActionCycles += A6809_ICOUNT;
				A6809_ICOUNT = 0;
			}
			
			// otherwise, process it right away
			else
				ProcessIRQ();
		}
	}
}


#pragma mark -
#pragma mark ¥ CORE IMPLEMENTATION

//###################################################################################################
//
//	Asgard6809Init -- init the 6809 processor state
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state variables
//
//###################################################################################################

void Asgard6809Init(void)
{
	int cpu = cpu_getactivecpu();
	state_save_register_UINT32("m6809ppc", cpu, "PCAB", &sPCAB, 1);
	state_save_register_UINT32("m6809ppc", cpu, "DPFlags", &sDPFlags, 1);
	state_save_register_UINT32("m6809ppc", cpu, "SU", &sSU, 1);
	state_save_register_UINT32("m6809ppc", cpu, "XY", &sXY, 1);
	state_save_register_INT8("m6809ppc", cpu, "NMI", &sNMIState, 1);
	state_save_register_INT8("m6809ppc", cpu, "IRQ", &sIRQState, 1);
	state_save_register_INT8("m6809ppc", cpu, "FIRQ", &sFIRQState, 1);
	state_save_register_UINT8("m6809ppc", cpu, "Int", &sInterruptState, 1);
	state_save_register_INT32("m6809ppc", cpu, "IntCycleAdjust", &sInterruptCycleAdjust, 1);
}

//###################################################################################################
//
//	Asgard6809Reset -- reset the 6809 processor
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state and create the tables
//
//###################################################################################################

void Asgard6809Reset(void)
{
	// according to the 6809 manual, the only things changed on a reset
	// are setting DP to 0 and setting the I and F flags
	sDPFlags &= 0xffff00ff;
	sDPFlags |= k6809FlagI | k6809FlagF;
	
	// initialize the PC
	sPCAB = (READMEM(0xfffe) << 24) + (READMEM(0xffff) << 16);
#ifdef A6809_UPDATEBANK
	A6809_UPDATEBANK(sPCAB >> 16);
#endif

	// reset the interrupt states
	Asgard6809SetIRQLine(k6809IRQLineNMI, k6809IRQStateClear);
	Asgard6809SetIRQLine(k6809IRQLineIRQ, k6809IRQStateClear);
	Asgard6809SetIRQLine(k6809IRQLineFIRQ, k6809IRQStateClear);
	
	// by default, we inhibit NMIs until the S register is set
	sInterruptState = kInterruptStateInhibitNMI;
	sInterruptCycleAdjust = 0;
	
	// make sure our tables have been created
	InitTables();
}


//###################################################################################################
//
//	Asgard6809SetContext -- set the contents of the 6809 registers
//
//	This function can unfortunately be called at any time to change the contents of the
//	6809 registers.  Call Asgard6809GetContext to get the original values before changing them.
//
//###################################################################################################

void Asgard6809SetContext(void *inContext)
{
	if (inContext)
	{
		Asgard6809Context *context = inContext;

		sSU = context->fSU;
		sXY = context->fXY;
		sPCAB = context->fPCAB;
		sDPFlags = context->fDPFlags | kDPFlagsDirty;

		sNMIState = context->fNMIState;
		sIRQState = context->fIRQState;
		sFIRQState = context->fFIRQState;
		sInterruptState = context->fInterruptState;
		sIRQCallback = context->fIRQCallback;
		sInterruptCycleAdjust = context->fInterruptCycleAdjust;

#ifdef A6809_UPDATEBANK
		A6809_UPDATEBANK(sPCAB >> 16);
#endif

		CheckIRQLines(false);
	}
}


//###################################################################################################
//
//	Asgard6809SetReg -- set the contents of one 6809 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	6809 register.
//
//###################################################################################################

void Asgard6809SetReg(int inRegisterIndex, unsigned int inValue)
{
	sDPFlags |= kDPFlagsDirty;
	
	switch (inRegisterIndex)
	{
		case k6809RegisterIndexA:
			sPCAB = (sPCAB & 0xffff00ff) | ((inValue & 0xff) << 8);
			break;
		
		case k6809RegisterIndexB:
			sPCAB = (sPCAB & 0xffffff00) | (inValue & 0xff);
			break;
		
		case k6809RegisterIndexPC:
			sPCAB = (sPCAB & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case k6809RegisterIndexS:
			sSU = (sSU & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case k6809RegisterIndexU:
			sSU = (sSU & 0xffff0000) | (inValue & 0xffff);
			break;

		case k6809RegisterIndexX:
			sXY = (sXY & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case k6809RegisterIndexY:
			sXY = (sXY & 0xffff0000) | (inValue & 0xffff);
			break;

		case k6809RegisterIndexCC:
			sDPFlags = (sDPFlags & 0xffffff00) | (inValue & 0xff);
			CheckIRQLines(false);
			break;
		
		case k6809RegisterIndexDP:
			sDPFlags = (sDPFlags & 0xffff00ff) | ((inValue & 0xff) << 8);
			break;

		case k6809RegisterIndexNMIState:
			sNMIState = inValue;
			break;

		case k6809RegisterIndexIRQState:
			sIRQState = inValue;
			break;

		case k6809RegisterIndexFIRQState:
			sFIRQState = inValue;
			break;
			
		case k6809RegisterIndexOpcodePC:
			sOpcodePC = inValue;
			break;
	}
}


//###################################################################################################
//
//	Asgard6809GetContext -- examine the contents of the 6809 registers
//
//	This function can unfortunately be called at any time to examine the contents of the
//	6809 registers.
//
//###################################################################################################

void Asgard6809GetContext(void *outContext)
{
	if (outContext)
	{
		Asgard6809Context *context = outContext;

		context->fSU = sSU;
		context->fXY = sXY;
		context->fPCAB = sPCAB;
		context->fDPFlags = sDPFlags;

		context->fNMIState = sNMIState;
		context->fIRQState = sIRQState;
		context->fFIRQState = sFIRQState;
		context->fInterruptState = sInterruptState;
		context->fIRQCallback = sIRQCallback;
		context->fInterruptCycleAdjust = sInterruptCycleAdjust;
	}
}


//###################################################################################################
//
//	Asgard6809GetReg -- get the contents of one 6809 register
//
//	This function can unfortunately be called at any time to return the contents of a
//	6809 register.
//
//###################################################################################################

unsigned int Asgard6809GetReg(int inRegisterIndex)
{
	switch (inRegisterIndex)
	{
		case k6809RegisterIndexA:
			return (sPCAB >> 8) & 0xff;
		
		case k6809RegisterIndexB:
			return sPCAB & 0xff;
		
		case k6809RegisterIndexPC:
			return (sPCAB >> 16) & 0xffff;
		
		case k6809RegisterIndexS:
			return (sSU >> 16) & 0xffff;
		
		case k6809RegisterIndexU:
			return sSU & 0xffff;

		case k6809RegisterIndexX:
			return (sXY >> 16) & 0xffff;
		
		case k6809RegisterIndexY:
			return sXY & 0xffff;
		
		case k6809RegisterIndexCC:
			return sDPFlags & 0xff;
		
		case k6809RegisterIndexDP:
			return (sDPFlags >> 8) & 0xff;

		case k6809RegisterIndexNMIState:
			return sNMIState;

		case k6809RegisterIndexIRQState:
			return sIRQState;

		case k6809RegisterIndexFIRQState:
			return sFIRQState;
		
		case k6809RegisterIndexOpcodePC:
			return sOpcodePC;
	}
	
	return 0;
}


//###################################################################################################
//
//	Asgard6809SetIRQLine -- sets the state of the IRQ/FIRQ lines
//
//###################################################################################################

void Asgard6809SetIRQLine(int inIRQLine, int inState)
{
	if (inIRQLine == INPUT_LINE_NMI)
	{
		// if the state is the same as last time, bail
		if (sNMIState == inState) 
			return;
		sNMIState = inState;

		// detect when the state goes non-clear
		if (inState != k6809IRQStateClear)
		{
			sInterruptState &= ~kInterruptStateSYNC;
		
			// generate the interrupt
			if (!(sInterruptState & kInterruptStateInhibitNMI))
			{
				// if we're inside the execution loop, just set the state bit and force us to exit
				if (sExecuting)
				{
					sExitActions |= kExitActionGenerateNMI;
					sExitActionCycles += A6809_ICOUNT;
					A6809_ICOUNT = 0;
				}
			
				// else process it right away
				else
					ProcessNMI();
			}
		}
	}
	else
	{
		// set the appropriate state
		if (inIRQLine == k6809IRQLineIRQ)
			sIRQState = inState;
		else if (inIRQLine == k6809IRQLineFIRQ)
			sFIRQState = inState;

		// if the state is non-clear, re-check the IRQ states
		if (inState != k6809IRQStateClear)
			CheckIRQLines(false);
	}
}


//###################################################################################################
//
//	Asgard6809SetIRQCallback -- sets the function to be called when an interrupt is generated
//
//###################################################################################################

void Asgard6809SetIRQCallback(Asgard6809IRQCallback inCallback)
{
	sIRQCallback = inCallback;
}

//###################################################################################################
//
//	Asgard6809GetIRQCallback -- gets the function to be called when an interrupt is generated
//
//###################################################################################################

Asgard6809IRQCallback Asgard6809GetIRQCallback(void)
{
	return sIRQCallback;
}


//###################################################################################################
//
//	Asgard6809Execute -- run the CPU emulation
//
//	This function executes the 6809 for the specified number of cycles, returning the actual
//	number of cycles executed.
//
//###################################################################################################

asm int Asgard6809Execute(register int inCycles)
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
	_asm_get_global(r4,sInterruptCycleAdjust)
	li		r0,0
	sub		rICount,inCycles,r4
	_asm_set_global(r0,sInterruptCycleAdjust)
	_asm_set_global(inCycles,sRequestedCycles)
	_asm_set_global(r0,sExitActionCycles)
	
	//	
	// 	initialize tables & goodies
	//
	_asm_get_global_ptr(rArgumentROMPtr,A6809_ARGUMENTROM)
	_asm_get_global_ptr(rOpcodeROMPtr,A6809_OPCODEROM)
	_asm_get_global_ptr(rICountPtr,A6809_ICOUNT)
	_asm_get_global_ptr(rOpcodeTable,sOpcodeTable)
	SAVE_ICOUNT
	lwz		rArgumentROM,0(rArgumentROMPtr)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)

	//
	//	restore the state of the machine
	//
	_asm_get_global(rDPFlags,sDPFlags)
	_asm_get_global(rPCAB,sPCAB)
	rlwinm	rDPFlags,rDPFlags,0,16,14			// clear the dirty bit in rDPFlags
	_asm_get_global(rXY,sXY)
	SAVE_DPFLAGS
	_asm_get_global(rSU,sSU)
	
	//
	//	mark that we're executing
	//
executeMore:
	li		r0,1
	li		r9,0
	_asm_set_global_b(r0,sExecuting)
	_asm_set_global_b(r9,sExitActions)

	//
	//	if we're still in a SYNC or CWAI state, eat all cycles and bail
	//
	_asm_get_global_b(r0,sInterruptState)
	andi.	r0,r0,(kInterruptStateCWAI | kInterruptStateSYNC)
	beq		executeLoop
	li		rICount,0
	b		executeLoopExit

	//================================================================================================

	//
	// 	this is the heart of the 6809 execution loop; the process is basically this: load an 
	// 	opcode, look up the function, and branch to it
	//
executeLoop:

	//
	//	internal debugging hook
	//
#if A6809_COREDEBUG
	mr		r3,rXY
	mr		r4,rSU
	mr		r5,rPCAB
	mr		r6,rDPFlags
	mr		r7,rICount
	bl		Asgard6809MiniTrace
#endif

	//
	//	external debugging hook
	//
#ifdef A6809_DEBUGHOOK
	_asm_get_global(r3,mame_debug)
#if TARGET_RT_MAC_CFM
	lwz		r3,0(r3)
#endif
	cmpwi	r3,0
	beq		executeLoopNoDebug
	_asm_set_global(rDPFlags,sDPFlags)
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rXY,sXY)
	_asm_set_global(rSU,sSU)
	stw		rICount,0(rICountPtr)
	bl		A6809_DEBUGHOOK
	_asm_get_global(rDPFlags,sDPFlags)
	_asm_get_global(rPCAB,sPCAB)
	_asm_get_global(rXY,sXY)
	_asm_get_global(rSU,sSU)
	lwz		rICount,0(rICountPtr)
#endif

executeLoopNoDebug:
	//
	//	read the opcode and branch to the appropriate location
	//
	GET_PC(r4)									// fetch the current PC
	lbzx	r3,rOpcodeROM,r4					// get the opcode
	addis	rPCAB,rPCAB,1						// increment & wrap the PC
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	lwzx	r5,rOpcodeTable,r5					// r5 = rOpcodeTable[r3 << 2]
	li		rCycleCount,0						// cycles = 0
	mtctr	r5									// ctr = r5
	_asm_set_global(r4,sOpcodePC)				// save the PC
	bctr										// go for it

#if (A6809_CHIP == 5000)
	//
	//	we get back here after any opcode that needs to store a word result at (rEA)
	//
executeLoopEndWriteEAWord:
	mr		rTempSave,r4
	SAVE_PCAB
	rlwinm	r3,rEA,0,16,31						// r3 = address
	SAVE_DPFLAGS
	rlwinm	r4,r4,24,24,31						// r4 = high byte
	addi	rEA,rEA,1
	bl		WRITEMEM							// perform the write
	rlwinm	r4,rTempSave,0,24,31				// r4 = low byte
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM							// perform the write
	bl		postExtern							// post-process
	b		executeLoopEnd
#endif

	//
	//	we get back here after any opcode that needs to store a byte result at (rEA)
	//
executeLoopEndWriteEA:
	SAVE_PCAB
	rlwinm	r3,rEA,0,16,31						// r3 = address
	SAVE_DPFLAGS
	bl		WRITEMEM							// perform the write
	bl		postExtern							// post-process

	//
	//	we get back here after any other opcode
	//
executeLoopEnd:
	sub.	rICount,rICount,rCycleCount			// decrement the cycle count
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
	_asm_set_global(rDPFlags,sDPFlags)
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rXY,sXY)
	_asm_set_global(rSU,sSU)

	//
	//	mark that we're no longer executing
	//
	li		r0,0
	_asm_set_global_b(r0,sExecuting)
	
	//
	//	see if there are interrupts pending
	//
	_asm_get_global_b(rTempSave,sExitActionCycles)
	andi.	r0,rTempSave,kExitActionGenerateNMI
	beq		noPendingNMI
	bl		ProcessNMI
	bl		postExtern
noPendingNMI:
	andi.	r0,rTempSave,kExitActionGenerateFIRQ
	beq		noPendingFIRQ
	bl		ProcessFIRQ
	bl		postExtern
noPendingFIRQ:
	andi.	r0,rTempSave,kExitActionGenerateIRQ
	beq		noPendingIRQ
	bl		ProcessIRQ
	bl		postExtern
noPendingIRQ:
	
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
	// 	IRQ line check routine: any time the state of the IRQ lines change, or anytime we futz
	//	with the interrupt enable bits, we need to re-check for interrupts
	//
checkIRQLines:
	mflr	rLinkSave2
	_asm_set_global(rDPFlags,sDPFlags)
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rXY,sXY)
	_asm_set_global(rSU,sSU)
	li		r3,true
	bl		CheckIRQLines
	mtlr	rLinkSave2
	
	// 	deliberate fall through here...

	//================================================================================================

	//
	// 	post-external call update: because an external function can modify the ICount, registers,
	//	etc., this function must be called after every external call
	//
postExtern:
	LOAD_DPFLAGS								// restore the flags
	LOAD_ICOUNT									// rICount = A6809_ICOUNT
	andis.	r0,rDPFlags,(kDPFlagsDirty>>16) 	// extract the dirty flag
	LOAD_ROM									// reload the ROM pointer
	beqlr										// if the registers aren't dirty, return
	LOAD_XY										// get the new XY
	LOAD_SU										// get the new SU
	rlwinm	rDPFlags,rDPFlags,0,16,14			// clear the dirty bit
	LOAD_PCAB									// get the new PC/A/B
	blr											// return

	//================================================================================================

#ifdef A6809_UPDATEBANK

	//
	//	post-PC change update: make sure the ROM bank hasn't been switched out from under us
	//
updateBank:
	mflr	rLinkSave2
	GET_PC(r3)
	bl		A6809_UPDATEBANK
	mtlr	rLinkSave2
	lwz		rOpcodeROM,0(rOpcodeROMPtr)			// restore the ROM pointer
	lwz		rArgumentROM,0(rArgumentROMPtr)		// restore the argument ROM pointer
	blr

#endif

	//================================================================================================

#if (A6809_CHIP != 5000)
	//
	// 	extended opcodes: load another byte and look up the function later in the table
	//
entry static op_10
	READ_OPCODE(r3)								// r3 = opcode
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	addi	r6,rOpcodeTable,OpcodeTable.fExt10	// point to the right table
	lwzx	r5,r6,r5							// r5 = rOpcodeTable[r3 << 2]
	mtctr	r5									// ctr = r5
	bctr										// go for it

entry static op_11
	READ_OPCODE(r3)								// r3 = opcode
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	addi	r6,rOpcodeTable,OpcodeTable.fExt11	// point to the right table
	lwzx	r5,r6,r5							// r5 = rOpcodeTable[r3 << 2]
	mtctr	r5									// ctr = r5
	bctr										// go for it
#endif

	//================================================================================================

	//
	// 	illegal opcode: ignore
	//
entry static illegal
	CYCLES(2,2,2)
	b		executeLoopEnd

	//================================================================================================

	//
	//		SWI/SWI2/SWI3: software interrupt generators
	//
entry static swi
	_asm_set_global(rDPFlags,sDPFlags)
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rXY,sXY)
	_asm_set_global(rSU,sSU)
	bl		ProcessSWI1
	bl		postExtern
	b		executeLoopEnd

entry static swi2
	_asm_set_global(rDPFlags,sDPFlags)
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rXY,sXY)
	_asm_set_global(rSU,sSU)
	bl		ProcessSWI2
	bl		postExtern
	CYCLES(1,1,1)					// 1 extra cycle for SWI2/SWI3
	b		executeLoopEnd

entry static swi3
	_asm_set_global(rDPFlags,sDPFlags)
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rXY,sXY)
	_asm_set_global(rSU,sSU)
	bl		ProcessSWI3
	bl		postExtern
	CYCLES(1,1,1)					// 1 extra cycle for SWI2/SWI3
	b		executeLoopEnd

	//================================================================================================

	//
	//		PSHS/PSHU: stack push operations
	//
pushcommon:
	SAVE_PCAB
	mflr	rLinkSave
	
	// push PC
	andi.	r0,rTempSave,0x80
	SAVE_DPFLAGS
	beq+	pushnopc
	subi	rEA,rEA,1
	rlwinm	r4,rPCAB,16,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	subi	rEA,rEA,1
	rlwinm	r4,rPCAB,8,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(2,2,2)
pushnopc:

	// push U
	andi.	r0,rTempSave,0x40
	beq+	pushnou
	subi	rEA,rEA,1
	rlwinm	r4,rSU,0,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	subi	rEA,rEA,1
	rlwinm	r4,rSU,24,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(2,2,2)
pushnou:

	// push S
	andi.	r0,rTempSave,0x100
	beq+	pushnos
	subi	rEA,rEA,1
	rlwinm	r4,rSU,16,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	subi	rEA,rEA,1
	rlwinm	r4,rSU,8,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(2,2,2)
pushnos:

	// push Y
	andi.	r0,rTempSave,0x20
	beq+	pushnoy
	subi	rEA,rEA,1
	rlwinm	r4,rXY,0,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	subi	rEA,rEA,1
	rlwinm	r4,rXY,24,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(2,2,2)
pushnoy:

	// push X
	andi.	r0,rTempSave,0x10
	beq+	pushnox
	subi	rEA,rEA,1
	rlwinm	r4,rXY,16,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	subi	rEA,rEA,1
	rlwinm	r4,rXY,8,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(2,2,2)
pushnox:

	// push DP
	andi.	r0,rTempSave,0x08
	beq+	pushnodp
	subi	rEA,rEA,1
	rlwinm	r4,rDPFlags,24,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(1,1,1)
pushnodp:

	// push B
	andi.	r0,rTempSave,0x04
	beq+	pushnob
	subi	rEA,rEA,1
	rlwinm	r4,rPCAB,0,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(1,1,1)
pushnob:

	// push A
	andi.	r0,rTempSave,0x02
	beq+	pushnoa
	subi	rEA,rEA,1
	rlwinm	r4,rPCAB,24,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(1,1,1)
pushnoa:

	// push CC
	andi.	r0,rTempSave,0x01
	beq+	pushnocc
	subi	rEA,rEA,1
	rlwinm	r4,rDPFlags,0,24,31
	rlwinm	r3,rEA,0,16,31
	bl		WRITEMEM
	CYCLES(1,1,1)
pushnocc:
	mtlr	rLinkSave
	b		postExtern
	
	//
	//		PSHS
	//
entry static pshs
	CYCLES(5,5,5)
	READ_OPCODE_ARG(rTempSave)
	GET_S(rEA)
	bl		pushcommon
	SET_S(rEA)
	SAVE_SU
	b		executeLoopEnd
	
	//
	//		PSHU
	//
entry static pshu
	CYCLES(5,5,5)
	READ_OPCODE_ARG(rTempSave)
	rlwimi	rTempSave,rTempSave,2,23,23			// move bit 0x40 up to 0x100 (we push S, not U)
	GET_U(rEA)
	rlwinm	rTempSave,rTempSave,0,26,24			// clear bit 0x40
	bl		pushcommon
	SET_U(rEA)
	SAVE_SU
	b		executeLoopEnd

	//================================================================================================

	//
	//		PULS/PULU: stack pull operations
	//
pullcommon:
	SAVE_PCAB
	mflr	rLinkSave
	
	// pull CC
	andi.	r0,rTempSave,0x01
	SAVE_DPFLAGS
	beq+	pullnocc
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rDPFlags,r3,0,24,31
	CYCLES(1,1,1)
pullnocc:

	// pull A
	andi.	r0,rTempSave,0x02
	beq+	pullnoa
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rPCAB,r3,8,16,23
	CYCLES(1,1,1)
pullnoa:
	
	// pull B
	andi.	r0,rTempSave,0x04
	beq+	pullnob
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rPCAB,r3,0,24,31
	CYCLES(1,1,1)
pullnob:
	
	// pull DP
	andi.	r0,rTempSave,0x08
	beq+	pullnodp
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rDPFlags,r3,8,16,23
	CYCLES(1,1,1)
pullnodp:
	
	// pull X
	andi.	r0,rTempSave,0x10
	beq+	pullnox
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rXY,r3,24,0,7
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rXY,r3,16,8,15
	CYCLES(2,2,2)
pullnox:
	
	// pull Y
	andi.	r0,rTempSave,0x20
	beq+	pullnoy
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rXY,r3,8,16,23
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rXY,r3,0,24,31
	CYCLES(2,2,2)
pullnoy:
	
	// pull U
	andi.	r0,rTempSave,0x40
	beq+	pullnou
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rSU,r3,8,16,23
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rSU,r3,0,24,31
	CYCLES(2,2,2)
pullnou:
	
	// pull S
	andi.	r0,rTempSave,0x100
	beq+	pullnos
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rSU,r3,24,0,7
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	_asm_get_global_b(r0,sInterruptState)
	rlwimi	rSU,r3,16,8,15
	andi.	r0,r0,~kInterruptStateInhibitNMI & 0xffff
	CYCLES(2,2,2)
	_asm_set_global_b(r0,sInterruptState)
pullnos:
	
	// pull PC
	andi.	r0,rTempSave,0x80
	SAVE_DPFLAGS
	beq+	pullnopc
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rPCAB,r3,24,0,7
	rlwinm	r3,rEA,0,16,31
	addi	rEA,rEA,1
	bl		READMEM
	rlwimi	rPCAB,r3,16,8,15
	CYCLES(2,2,2)
	UPDATE_BANK

pullnopc:
	SAVE_XY
	SAVE_PCAB
	mtlr	rLinkSave
	b		postExtern

	//
	//		PULS
	//
entry static puls
	CYCLES(5,5,5)
	READ_OPCODE_ARG(rTempSave)
	GET_S(rEA)
	bl		pullcommon
	andi.	r0,rTempSave,0x01
	SET_S(rEA)
	SAVE_SU
	beq		skipIRQCheck1
	bl		checkIRQLines
skipIRQCheck1:
	b		executeLoopEnd
	
	//
	//		PULU
	//
entry static pulu
	CYCLES(5,5,5)
	READ_OPCODE_ARG(rTempSave)
	rlwimi	rTempSave,rTempSave,2,23,23			// move bit 0x40 up to 0x100 (we push S, not U)
	GET_U(rEA)
	rlwinm	rTempSave,rTempSave,0,26,24			// clear bit 0x40
	bl		pullcommon
	andi.	r0,rTempSave,0x01
	SET_U(rEA)
	SAVE_SU
	beq		skipIRQCheck2
	bl		checkIRQLines
skipIRQCheck2:
	b		executeLoopEnd

	//================================================================================================

	//
	//		LD: register load operations
	//

	//
	//		LDA
	//
entry static lda_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		ldacommon

entry static lda_di
	DIRECT
	CYCLES(4,4,4)
ldamemcommon:
	READ_BYTE_SAVE(rEA)
ldacommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in result
	rlwimi	rDPFlags,r3,28,28,28				// N = (r4 & 0x80)
	SET_A(r3)									// A = r3
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static lda_ix
	CYCLES(4,4,4)
	INDEXED
	b		ldamemcommon

entry static lda_ex
	EXTENDED
	CYCLES(5,5,5)
	b		ldamemcommon

	//
	//		LDB
	//
entry static ldb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		ldbcommon

entry static ldb_di
	DIRECT
	CYCLES(4,4,4)
ldbmemcommon:
	READ_BYTE_SAVE(rEA)
ldbcommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in result
	rlwimi	rDPFlags,r3,28,28,28				// N = (r4 & 0x80)
	SET_B(r3)									// A = r3
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static ldb_ix
	CYCLES(4,4,4)
	INDEXED
	b		ldbmemcommon

entry static ldb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		ldbmemcommon

	//
	//		LDD
	//
entry static ldd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(3,3,3)
	b		lddcommon

entry static ldd_di
	DIRECT
	CYCLES(5,5,5)
lddmemcommon:
	READ_WORD_SAVE(rEA)
lddcommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_D(rTempSave)							// X = r3
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static ldd_ix
	CYCLES(5,5,5)
	INDEXED
	b		lddmemcommon

entry static ldd_ex
	EXTENDED
	CYCLES(6,6,6)
	b		lddmemcommon

	//
	//		LDX
	//
entry static ldx_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(3,3,3)
	b		ldxcommon

entry static ldx_di
	DIRECT
	CYCLES(5,5,5)
ldxmemcommon:
	READ_WORD_SAVE(rEA)
ldxcommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_X(rTempSave)							// X = r3
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	SAVE_XY
	b		executeLoopEnd

entry static ldx_ix
	CYCLES(5,5,5)
	INDEXED
	b		ldxmemcommon

entry static ldx_ex
	EXTENDED
	CYCLES(6,6,6)
	b		ldxmemcommon

	//
	//		LDY
	//
entry static ldy_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(4,4,4)
	b		ldycommon

entry static ldy_di
	DIRECT
	CYCLES(6,6,6)
ldymemcommon:
	READ_WORD_SAVE(rEA)
ldycommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_Y(rTempSave)							// X = r3
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	SAVE_XY
	b		executeLoopEnd

entry static ldy_ix
	CYCLES(6,6,6)
	INDEXED
	b		ldymemcommon

entry static ldy_ex
	EXTENDED
	CYCLES(7,7,7)
	b		ldymemcommon

	//
	//		LDS
	//
entry static lds_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(4,4,4)
	b		ldscommon

entry static lds_di
	DIRECT
	CYCLES(6,6,6)
ldsmemcommon:
	READ_WORD_SAVE(rEA)
ldscommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	_asm_get_global_b(r0,sInterruptState)
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	andi.	r0,r0,~kInterruptStateInhibitNMI & 0xffff
	SET_S(rTempSave)							// S = r3
	_asm_set_global_b(r0,sInterruptState)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	SAVE_SU										// save the updated S
	b		executeLoopEnd

entry static lds_ix
	CYCLES(6,6,6)
	INDEXED
	b		ldsmemcommon

entry static lds_ex
	EXTENDED
	CYCLES(7,7,7)
	b		ldsmemcommon

	//
	//		LDU
	//
entry static ldu_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(3,3,3)
	b		lducommon

entry static ldu_di
	DIRECT
	CYCLES(5,5,5)
ldumemcommon:
	READ_WORD_SAVE(rEA)
lducommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_U(rTempSave)							// X = r3
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	SAVE_SU										// save the updated S
	b		executeLoopEnd

entry static ldu_ix
	CYCLES(5,5,5)
	INDEXED
	b		ldumemcommon

entry static ldu_ex
	EXTENDED
	CYCLES(6,6,6)
	b		ldumemcommon

	//================================================================================================

	//
	//		ST: register store operations
	//

	//
	//		STA
	//
entry static sta_im
entry static stb_im
	READ_OPCODE_ARG(r3)
	b		executeLoopEnd						// shouldn't do anything....

entry static sta_di
	DIRECT
	CYCLES(4,4,4)
stamemcommon:
	GET_A(r4)
st8memcommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static sta_ix
	CYCLES(4,4,4)
	INDEXED
	b		stamemcommon

entry static sta_ex
	EXTENDED
	CYCLES(5,5,5)
	b		stamemcommon

	//
	//		STB
	//
entry static stb_di
	DIRECT
	CYCLES(4,4,4)
	GET_B(r4)
	b		st8memcommon

entry static stb_ix
	CYCLES(4,4,4)
	INDEXED
	GET_B(r4)
	b		st8memcommon

entry static stb_ex
	EXTENDED
	CYCLES(5,5,5)
	GET_B(r4)
	b		st8memcommon

	//
	//		STD
	//
entry static std_im
entry static stx_im
entry static sty_im
entry static sts_im
entry static stu_im
	READ_OPCODE_ARG_WORD(rTempSave)
	b		executeLoopEnd						// shouldn't do anything....

entry static std_di
	DIRECT
	CYCLES(5,5,5)
stdmemcommon:
	GET_D(rTempSave)
st16memcommon:
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	WRITE_WORD_LO_SAVE(rEA,rTempSave)
	b		executeLoopEnd

entry static std_ix
	CYCLES(5,5,5)
	INDEXED
	b		stdmemcommon

entry static std_ex
	EXTENDED
	CYCLES(6,6,6)
	b		stdmemcommon

	//
	//		STX
	//
entry static stx_di
	DIRECT
	CYCLES(5,5,5)
	GET_X(rTempSave)
	b		st16memcommon

entry static stx_ix
	CYCLES(5,5,5)
	INDEXED
	GET_X(rTempSave)
	b		st16memcommon

entry static stx_ex
	EXTENDED
	CYCLES(6,6,6)
	GET_X(rTempSave)
	b		st16memcommon

	//
	//		STY
	//
entry static sty_di
	DIRECT
	CYCLES(6,6,6)
	GET_Y(rTempSave)
	b		st16memcommon

entry static sty_ix
	CYCLES(6,6,6)
	INDEXED
	GET_Y(rTempSave)
	b		st16memcommon

entry static sty_ex
	EXTENDED
	CYCLES(7,7,7)
	GET_Y(rTempSave)
	b		st16memcommon

	//
	//		STS
	//
entry static sts_di
	DIRECT
	CYCLES(6,6,6)
	GET_S(rTempSave)
	b		st16memcommon

entry static sts_ix
	CYCLES(6,6,6)
	INDEXED
	GET_S(rTempSave)
	b		st16memcommon

entry static sts_ex
	EXTENDED
	CYCLES(7,7,7)
	GET_S(rTempSave)
	b		st16memcommon

	//
	//		STU
	//
entry static stu_di
	DIRECT
	CYCLES(5,5,5)
	GET_U(rTempSave)
	b		st16memcommon

entry static stu_ix
	CYCLES(5,5,5)
	INDEXED
	GET_U(rTempSave)
	b		st16memcommon

entry static stu_ex
	EXTENDED
	CYCLES(6,6,6)
	GET_U(rTempSave)
	b		st16memcommon

	//================================================================================================

	//
	//		CLR: register/memory clear operations
	//

	//
	//		Memory clear
	//
entry static clr_di
	DIRECT
	CYCLES(6,6,6)
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	li		r4,0								// r4 = 0
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEndWriteEA

entry static clr_ix
	CYCLES(6,6,6)
	INDEXED
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	li		r4,0								// r4 = 0
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEndWriteEA

entry static clr_ex
	CYCLES(7,7,7)
	EXTENDED
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	li		r4,0								// r4 = 0
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEndWriteEA

	//
	//		Register (A/B) clear
	//
entry static clra
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	rlwinm	rPCAB,rPCAB,0,24,15					// A = 0
	CYCLES(2,2,2)
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEnd

entry static clrb
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	rlwinm	rPCAB,rPCAB,0,0,23					// B = 0
	CYCLES(2,2,2)
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory clear 16-bits
	//
entry static clrw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	li		r4,0								// r4 = 0
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEndWriteEAWord

	//
	//		Register (D) clear
	//
entry static clrd	// not yet
	li		r0,k6809FlagZ						// r0 = (N=V=C=0, Z=1)
	rlwinm	rPCAB,rPCAB,0,0,15					// D = 0
	CYCLES(2,2,2)
	rlwimi	rDPFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		ADD: register/memory addition operations
	//

	//
	//		ADDA
	//
entry static adda_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		addacommon

entry static adda_di
	DIRECT
	CYCLES(4,4,4)
addamemcommon:
	READ_BYTE_SAVE(rEA)
addacommon:
	GET_A(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
setflagsha:
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r8,1,26,26					// H = (r5 ^ r3 ^ r4) & 0x10
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static adda_ix
	CYCLES(4,4,4)
	INDEXED
	b		addamemcommon

entry static adda_ex
	EXTENDED
	CYCLES(5,5,5)
	b		addamemcommon

	//
	//		ADDB
	//
entry static addb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		addbcommon

entry static addb_di
	DIRECT
	CYCLES(4,4,4)
addbmemcommon:
	READ_BYTE_SAVE(rEA)
addbcommon:
	GET_B(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
setflagshb:
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r8,1,26,26					// H = (r5 ^ r3 ^ r4) & 0x10
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_B(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static addb_ix
	CYCLES(4,4,4)
	INDEXED
	b		addbmemcommon

entry static addb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		addbmemcommon

	//
	//		ADDD
	//
entry static addd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(4,4,4)
	b		adddcommon

entry static addd_di
	DIRECT
	CYCLES(6,6,6)
adddmemcommon:
	READ_WORD_SAVE(rEA)
adddcommon:
	GET_D(r5)									// r5 = D
	add		r4,r5,rTempSave						// r4 = r5 + rTempSave
setflagsd:
	xor		r8,r5,rTempSave						// r8 = r5 ^ rTempSave
	rlwimi	rDPFlags,r4,16,31,31				// C = (r4 & 0x10000)
	xor		r8,r8,r4							// r8 = r5 ^ rTempSave ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,20,28,28				// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = r5 ^ rTempSave ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,18,30,30				// V = (r5 ^ rTempSave ^ r4 ^ (r4 >> 1)) & 0x8000
	SET_D(r4)									// D = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static addd_ix
	CYCLES(6,6,6)
	INDEXED
	b		adddmemcommon

entry static addd_ex
	EXTENDED
	CYCLES(7,7,7)
	b		adddmemcommon

	//================================================================================================

	//
	//		ADC: register/memory addition with carry operations
	//

	//
	//		ADCA
	//
entry static adca_im
	READ_OPCODE_ARG(r3)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_A(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
	CYCLES(2,2,2)
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagsha							// same as ADD

entry static adca_di
	DIRECT
	CYCLES(4,4,4)
adcamemcommon:
	READ_BYTE_SAVE(rEA)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_A(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagsha							// same as ADD

entry static adca_ix
	CYCLES(4,4,4)
	INDEXED
	b		adcamemcommon

entry static adca_ex
	EXTENDED
	CYCLES(5,5,5)
	b		adcamemcommon

	//
	//		ADCB
	//
entry static adcb_im
	READ_OPCODE_ARG(r3)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_B(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
	CYCLES(2,2,2)
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagshb							// same as ADD

entry static adcb_di
	DIRECT
	CYCLES(4,4,4)
adcbmemcommon:
	READ_BYTE_SAVE(rEA)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_B(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagshb							// same as ADD

entry static adcb_ix
	CYCLES(4,4,4)
	INDEXED
	b		adcbmemcommon

entry static adcb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		adcbmemcommon

	//================================================================================================

	//
	//		ABX: add B to X operation
	//
entry static abx
	rlwinm	r3,rPCAB,16,8,15
	add		rXY,rXY,r3
	CYCLES(3,3,3)
	SAVE_XY
	b		executeLoopEnd

	//================================================================================================

	//
	//		SUB: register/memory subtract operations
	//

	//
	//		SUBA
	//
entry static suba_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
setflagsa:										// same as ADD, but don't change H
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static suba_di
	DIRECT
	CYCLES(4,4,4)
subamemcommon:
	READ_BYTE_SAVE(rEA)
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	b		setflagsa							// same as ADD

entry static suba_ix
	CYCLES(4,4,4)
	INDEXED
	b		subamemcommon

entry static suba_ex
	EXTENDED
	CYCLES(5,5,5)
	b		subamemcommon

	//
	//		SUBB
	//
entry static subb_im
	READ_OPCODE_ARG(r3)
	GET_B(r5)									// r5 = A
	CYCLES(2,2,2)
	sub		r4,r5,r3							// r4 = r5 - r3
setflagsb:										// same as ADD, but don't change H
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_B(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static subb_di
	DIRECT
	CYCLES(4,4,4)
subbmemcommon:
	READ_BYTE_SAVE(rEA)
subbcommon:
	GET_B(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	b		setflagsb							// same as ADD

entry static subb_ix
	CYCLES(4,4,4)
	INDEXED
	b		subbmemcommon

entry static subb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		subbmemcommon

	//
	//		SUBD
	//
entry static subd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	GET_D(r5)									// r5 = A
	CYCLES(4,4,4)
	sub		r4,r5,rTempSave						// r4 = r5 - rTempSave
	b		setflagsd							// same as ADD

entry static subd_di
	DIRECT
	CYCLES(6,6,6)
subdmemcommon:
	READ_WORD_SAVE(rEA)
	GET_D(r5)									// r5 = A
	sub		r4,r5,rTempSave						// r4 = r5 - rTempSave
	b		setflagsd							// same as ADD

entry static subd_ix
	CYCLES(6,6,6)
	INDEXED
	b		subdmemcommon

entry static subd_ex
	EXTENDED
	CYCLES(7,7,7)
	b		subdmemcommon

	//================================================================================================

	//
	//		SBC: register/memory subtract with borrow operations
	//

	//
	//		SBCA
	//
entry static sbca_im
	READ_OPCODE_ARG(r3)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	CYCLES(2,2,2)
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsa							// same as ADD

entry static sbca_di
	DIRECT
	CYCLES(4,4,4)
sbcamemcommon:
	READ_BYTE_SAVE(rEA)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsa							// same as ADD

entry static sbca_ix
	CYCLES(4,4,4)
	INDEXED
	b		sbcamemcommon

entry static sbca_ex
	EXTENDED
	CYCLES(5,5,5)
	b		sbcamemcommon

	//
	//		SBCB
	//
entry static sbcb_im
	READ_OPCODE_ARG(r3)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_B(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	CYCLES(2,2,2)
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsb							// same as ADD

entry static sbcb_di
	DIRECT
	CYCLES(4,4,4)
sbcbmemcommon:
	READ_BYTE_SAVE(rEA)
	rlwinm	r10,rDPFlags,0,31,31				// r10 = C
	GET_B(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsb							// same as ADD

entry static sbcb_ix
	CYCLES(4,4,4)
	INDEXED
	b		sbcbmemcommon

entry static sbcb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		sbcbmemcommon

	//================================================================================================

	//
	//		MUL: register multiply operation
	//

entry static mul
	GET_A(r3)									// r3 = A
	GET_B(r5)									// r5 = B
	mullw	r4,r3,r5							// r4 = r3 * r5
	CYCLES(11,11,11)
	cntlzw	r7,r4								// r7 = number of zeros in product
	rlwimi	rDPFlags,r4,25,31,31				// C = (r4 & 0x80)
	SET_D(r4)									// D = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Long (16-bit) multiply
	//
entry static lmul	// not yet
	GET_X(r3)									// r3 = X
	GET_Y(r5)									// r5 = Y
	mullw	rXY,r3,r5							// rXY = r3 * r5
	CYCLES(11,11,11)
	cntlzw	r7,rXY								// r7 = number of zeros in product
	rlwimi	rDPFlags,rXY,17,31,31				// C = (r4 & 0x8000)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

	//
	//		Divide
	//
entry static divx	// not yet
	rlwinm.	r3,rPCAB,0,24,31					// r3 = B
	GET_X(r5)									// r5 = X
	beq		divx_by_zero
	divwu	r4,r5,r3							// r4 = X / B
	CYCLES(11,11,11)
	cntlzw	r7,r4								// r7 = number of zeros in quotient
	mullw	r6,r4,r3							// r6 = (X / B) * B
divx_common:
	SET_X(r4)
	sub		r6,r5,r6							// r6 = X - (X / B) * B
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	SET_B(r6)
	rlwimi	rDPFlags,r4,25,31,31				// C = (r4 & 0x80)
	b		executeLoopEnd
divx_by_zero:
	li		r4,0
	li		r6,0
	cntlzw	r7,r4								// r7 = number of zeros in quotient
	b		divx_common
#endif

	//================================================================================================

	//
	//		NEG: register/memory negate operations
	//

	//
	//		Memory negate
	//
entry static neg_di
	DIRECT
	CYCLES(6,6,6)
negcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	neg		r4,r3								// r4 = result = -r3
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x80
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static neg_ix
	CYCLES(6,6,6)
	INDEXED
	b		negcommon

entry static neg_ex
	CYCLES(7,7,7)
	EXTENDED
	b		negcommon

	//
	//		Register (A/B) negate
	//
entry static nega
	GET_A(r3)									// r3 = A
	CYCLES(2,2,2)
	neg		r4,r3								// r4 = result = -r3
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_A(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static negb
	GET_B(r3)									// r3 = A
	CYCLES(2,2,2)
	neg		r4,r3								// r4 = result = -r3
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_B(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory negate 16-bit
	//
entry static negw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = byte at rEA
	neg		r4,rTempSave						// r4 = result = -rTempSave
	xor		r8,rTempSave,r4						// r8 = rTempSave ^ r4
	rlwimi	rDPFlags,r4,16,31,31				// C = (r4 & 0x10000)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,20,28,28				// N = (r4 & 0x8000)
	rlwinm	r4,r4,0,16,31						// r4 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = rTempSave ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,18,30,30				// V = (rTempSave ^ r4 ^ (r4 >> 1)) & 0x8000
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEAWord

	//
	//		Register (D) negate
	//
entry static negd	// not yet
	GET_D(r3)									// r3 = D
	CYCLES(2,2,2)
	neg		r4,r3								// r4 = result = -r3
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,16,31,31				// C = (r4 & 0x10000)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,20,28,28				// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,18,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x8000
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		INC: register/memory increment operations
	//

	//
	//		Memory increment
	//
entry static inc_di
	DIRECT
	CYCLES(6,6,6)
inccommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	xori	r6,r3,0x7f							// r6 = r3 ^ 0x7f
	xori	r7,r3,0xff							// r7 = r3 ^ 0xff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0xff)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0xff)
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)
	b		executeLoopEndWriteEA

entry static inc_ix
	CYCLES(6,6,6)
	INDEXED
	b		inccommon

entry static inc_ex
	CYCLES(7,7,7)
	EXTENDED
	b		inccommon

	//
	//		Register (A/B) increment
	//
entry static inca
	GET_A(r3)									// r3 = A
	CYCLES(2,2,2)
	xori	r6,r3,0x7f							// r6 = r3 ^ 0x7f
	xori	r7,r3,0xff							// r7 = r3 ^ 0xff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0xff)
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)
	b		executeLoopEnd

entry static incb
	GET_B(r3)									// r3 = B
	CYCLES(2,2,2)
	xori	r6,r3,0x7f							// r6 = r3 ^ 0x7f
	xori	r7,r3,0xff							// r7 = r3 ^ 0xff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0xff)
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory increment 16-bit
	//
entry static incw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = byte at rEA
	xori	r6,rTempSave,0x7fff					// r6 = rTempSave ^ 0x7fff
	xori	r7,rTempSave,0xffff					// r7 = rTempSave ^ 0xffff
	cntlzw	r6,r6								// r6 = number of zeros in (rTempSave ^ 0x8000)
	cntlzw	r7,r7								// r7 = number of zeros in (rTempSave ^ 0xffff)
	rlwimi	rDPFlags,r6,28,30,30				// V = (rTempSave == 0x7fff)
	addi	r4,rTempSave,1						// r4 = result = rTempSave + 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (rTempSave == 0xffff)
	rlwimi	rDPFlags,r4,20,28,28 				// N = (r4 & 0x8000)
	b		executeLoopEndWriteEAWord


	//
	//		Register (D) increment
	//
entry static incd	// not yet
	GET_D(r3)									// r3 = D
	CYCLES(2,2,2)
	xori	r6,r3,0x7fff						// r6 = r3 ^ 0x7fff
	xori	r7,r3,0xffff						// r7 = r3 ^ 0xffff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x8000)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0xffff)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0xff)
	SET_D(r4)									// A = r4
	rlwimi	rDPFlags,r4,20,28,28 				// N = (r4 & 0x8000)
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		DEC: register/memory decrement operations
	//

	//
	//		Memory decrement
	//
entry static dec_di
	DIRECT
	CYCLES(6,6,6)
deccommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x80)
	subi	r4,r3,1								// r4 = result = r3 - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0x01)
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)
	b		executeLoopEndWriteEA

entry static dec_ix
	CYCLES(6,6,6)
	INDEXED
	b		deccommon

entry static dec_ex
	CYCLES(7,7,7)
	EXTENDED
	b		deccommon

	//
	//		Register (A/B) decrement
	//
entry static deca
	GET_A(r3)									// r3 = A
	CYCLES(2,2,2)
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x80)
	subi	r4,r3,1								// r4 = result = r3 - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0x01)
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)
	b		executeLoopEnd

entry static decb
	GET_B(r3)									// r3 = B
	CYCLES(2,2,2)
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x80)
	subi	r4,r3,1								// r4 = result = r3 - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0x01)
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory decrement
	//
entry static decw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	xori	r6,rTempSave,0x8000					// r6 = rTempSave ^ 0x8000
	xori	r7,rTempSave,0x0001					// r7 = rTempSave ^ 0x0001
	cntlzw	r6,r6								// r6 = number of zeros in (rTempSave ^ 0x8000)
	cntlzw	r7,r7								// r7 = number of zeros in (rTempSave ^ 0x0001)
	rlwimi	rDPFlags,r6,28,30,30				// V = (rTempSave == 0x8000)
	subi	r4,rTempSave,1						// r4 = result = rTempSave - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (rTempSave == 0x0001)
	rlwimi	rDPFlags,r4,20,28,28 				// N = (r4 & 0x8000)
	b		executeLoopEndWriteEAWord

	//
	//		Register (D) decrement
	//
entry static decd	// not yet
	GET_D(r3)									// r3 = D
	CYCLES(2,2,2)
	xori	r6,r3,0x8000						// r6 = r3 ^ 0x8000
	xori	r7,r3,0x0001						// r7 = r3 ^ 0x0001
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x8000)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x0001)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x8000)
	subi	r4,r3,1								// r4 = result = r3 - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0x0001)
	SET_D(r4)									// D = r4
	rlwimi	rDPFlags,r4,20,28,28 				// N = (r4 & 0x8000)
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		CMP: register/memory compare operations
	//

	//
	//		CMPA
	//
entry static cmpa_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	GET_A(rTempSave2)
	b		cmp8common

entry static cmpa_di
	DIRECT
	CYCLES(4,4,4)
cmpamemcommon:
	GET_A(rTempSave2)
cmp8memcommon:
	READ_BYTE_SAVE(rEA)
cmp8common:
	sub		r4,rTempSave2,r3					// r4 = rTempSave2 - r3
	xor		r8,rTempSave2,r3					// r8 = rTempSave2 ^ r3
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = rTempSave2 ^ r3 ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = rTempSave2 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (rTempSave2 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static cmpa_ix
	CYCLES(4,4,4)
	INDEXED
	b		cmpamemcommon

entry static cmpa_ex
	EXTENDED
	CYCLES(5,5,5)
	b		cmpamemcommon

	//
	//		CMPB
	//
entry static cmpb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	GET_B(rTempSave2)
	b		cmp8common

entry static cmpb_di
	DIRECT
	CYCLES(4,4,4)
	GET_B(rTempSave2)
	b		cmp8memcommon

entry static cmpb_ix
	CYCLES(4,4,4)
	INDEXED
	GET_B(rTempSave2)
	b		cmp8memcommon

entry static cmpb_ex
	EXTENDED
	CYCLES(5,5,5)
	GET_B(rTempSave2)
	b		cmp8memcommon

	//
	//		CMPD
	//
entry static cmpd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(5,5,5)
	GET_D(rTempSave2)
	b		cmp16common

entry static cmpd_di
	DIRECT
	CYCLES(7,7,7)
cmpdmemcommon:
	GET_D(rTempSave2)
cmp16memcommon:
	READ_WORD_SAVE(rEA)
cmp16common:
	sub		r4,rTempSave2,rTempSave				// r4 = rTempSave2 - rTempSave
	xor		r8,rTempSave2,rTempSave				// r8 = rTempSave2 ^ rTempSave
	rlwimi	rDPFlags,r4,16,31,31				// C = (r4 & 0x10000)
	xor		r8,r8,r4							// r8 = rTempSave2 ^ rTempSave ^ r4
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,20,28,28				// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = rTempSave2 ^ rTempSave ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,18,30,30				// V = (rTempSave2 ^ rTempSave ^ r4 ^ (r4 >> 1)) & 0x8000
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static cmpd_ix
	CYCLES(7,7,7)
	INDEXED
	b		cmpdmemcommon

entry static cmpd_ex
	EXTENDED
	CYCLES(8,8,8)
	b		cmpdmemcommon

	//
	//		CMPX
	//
entry static cmpx_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(4,4,4)
	GET_X(rTempSave2)
	b		cmp16common

entry static cmpx_di
	DIRECT
	CYCLES(6,6,6)
	GET_X(rTempSave2)
	b		cmp16memcommon

entry static cmpx_ix
	CYCLES(6,6,6)
	INDEXED
	GET_X(rTempSave2)
	b		cmp16memcommon

entry static cmpx_ex
	EXTENDED
	CYCLES(7,7,7)
	GET_X(rTempSave2)
	b		cmp16memcommon

	//
	//		CMPY
	//
entry static cmpy_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(5,5,5)
	GET_Y(rTempSave2)
	b		cmp16common

entry static cmpy_di
	DIRECT
	CYCLES(7,7,7)
	GET_Y(rTempSave2)
	b		cmp16memcommon

entry static cmpy_ix
	CYCLES(7,7,7)
	INDEXED
	GET_Y(rTempSave2)
	b		cmp16memcommon

entry static cmpy_ex
	EXTENDED
	CYCLES(8,8,8)
	GET_Y(rTempSave2)
	b		cmp16memcommon

	//
	//		CMPS
	//
entry static cmps_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(5,5,5)
	GET_S(rTempSave2)
	b		cmp16common

entry static cmps_di
	DIRECT
	CYCLES(7,7,7)
	GET_S(rTempSave2)
	b		cmp16memcommon

entry static cmps_ix
	CYCLES(7,7,7)
	INDEXED
	GET_S(rTempSave2)
	b		cmp16memcommon

entry static cmps_ex
	EXTENDED
	CYCLES(8,8,8)
	GET_S(rTempSave2)
	b		cmp16memcommon

	//
	//		CMPU
	//
entry static cmpu_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(5,5,5)
	GET_U(rTempSave2)
	b		cmp16common

entry static cmpu_di
	DIRECT
	CYCLES(7,7,7)
	GET_U(rTempSave2)
	b		cmp16memcommon

entry static cmpu_ix
	CYCLES(7,7,7)
	INDEXED
	GET_U(rTempSave2)
	b		cmp16memcommon

entry static cmpu_ex
	EXTENDED
	CYCLES(8,8,8)
	GET_U(rTempSave2)
	b		cmp16memcommon

	//================================================================================================

	//
	//		TST: register/memory test operations
	//

	//
	//		Memory test
	//
entry static tst_di
	DIRECT
	CYCLES(6,6,6)
tstcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rDPFlags,r3,28,28,28				// N = (r3 & 0x80)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0)
	b		executeLoopEnd

entry static tst_ix
	CYCLES(6,6,6)
	INDEXED
	b		tstcommon

entry static tst_ex
	CYCLES(7,7,7)
	EXTENDED
	b		tstcommon

	//
	//		Register (A/B) test
	//
entry static tsta
	GET_A(r3)									// r3 = A
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rDPFlags,r3,28,28,28				// N = (r3 & 0x80)
	CYCLES(2,2,2)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0)
	b		executeLoopEnd

entry static tstb
	GET_B(r3)									// r3 = B
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rDPFlags,r3,28,28,28				// N = (r3 & 0x80)
	CYCLES(2,2,2)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory test 16-bit
	//
entry static tstw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in rTempSave
	rlwimi	rDPFlags,rTempSave,20,28,28			// N = (rTempSave & 0x8000)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (rTempSave == 0)
	b		executeLoopEnd

	//
	//		Register (D) test
	//
entry static tstd	// not yet
	GET_D(r3)									// r3 = D
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rDPFlags,r3,20,28,28				// N = (r3 & 0x8000)
	CYCLES(2,2,2)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0)
	b		executeLoopEnd
#endif

	//================================================================================================

	//
	//		LSR: register/memory logical right shift operations
	//

	//
	//		Memory logical right shift
	//
entry static lsr_di
	DIRECT
	CYCLES(6,6,6)
lsrcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,31,25,31						// r4 = result = r3 >> 1
	rlwimi	rDPFlags,r3,0,31,31					// C = r3 & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rDPFlags,rDPFlags,0,29,27			// N = 0
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static lsr_ix
	CYCLES(6,6,6)
	INDEXED
	b		lsrcommon

entry static lsr_ex
	CYCLES(7,7,7)
	EXTENDED
	b		lsrcommon

	//
	//		Register (A/B) logical right shift
	//
entry static lsra
	rlwinm	r4,rPCAB,23,25,31					// r4 = result = A >> 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rPCAB,24,31,31				// C = A & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rDPFlags,rDPFlags,0,29,27			// N = 0
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static lsrb
	rlwinm	r4,rPCAB,31,25,31					// r4 = result = B >> 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rPCAB,0,31,31				// C = B & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rDPFlags,rDPFlags,0,29,27			// N = 0
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory logical right shift 16-bit
	//
entry static lsrw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	rlwinm	r4,rTempSave,31,17,31				// r4 = result = rTempSave >> 1
	rlwimi	rDPFlags,rTempSave,0,31,31			// C = rTempSave & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rDPFlags,rDPFlags,0,29,27			// N = 0
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEAWord

	//
	//		Register logical right shift 16-bit variable
	//
entry static lsrd_im	// not yet
	CYCLES(2,2,2)
	READ_OPCODE_ARG(r6)							// get shift count in r6
lsrd_common:
	GET_D(r3)									// r3 = D
	subic.	r7,r6,1								// subtract by 1
	cmpwi	cr7,r6,15
	blt		lsrd_noshift						// handle 0 count
	bgt		cr7,lsrd_overshift					// handle >15 count
	srw		r3,r3,r7							// pre-shift r3 by count-1
	rlwinm	r4,r3,31,17,31						// r4 = result = r3 >> 1
	rlwimi	rDPFlags,r3,0,31,31					// C = r3 & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rDPFlags,rDPFlags,0,29,27			// N = 0
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd
lsrd_overshift:
	rlwinm	rPCAB,rPCAB,0,0,15
lsrd_noshift:
	b		executeLoopEnd

entry static lsrd_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	mr		r6,r3								// r6 = byte
	b		lsrd_common
#endif

	//================================================================================================

	//
	//		ROR: register/memory right rotate operations
	//

	//
	//		Memory right rotate
	//
entry static ror_di
	DIRECT
	CYCLES(6,6,6)
rorcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,31,25,31						// r4 = result = r3 >> 1
	rlwimi	rDPFlags,rDPFlags,3,28,28			// N = C
	rlwimi	r4,rDPFlags,7,24,24					// r4 = result = (r3 >> 1) | (C << 7)
	rlwimi	rDPFlags,r3,0,31,31					// C = r3 & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static ror_ix
	CYCLES(6,6,6)
	INDEXED
	b		rorcommon

entry static ror_ex
	CYCLES(7,7,7)
	EXTENDED
	b		rorcommon

	//
	//		Register (A/B) right rotate
	//
entry static rora
	rlwinm	r4,rPCAB,23,25,31					// r4 = result = A >> 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rDPFlags,3,28,28 			// N = C
	rlwimi	r4,rDPFlags,7,24,24					// r4 = result = (r3 >> 1) | (C << 7)
	rlwimi	rDPFlags,rPCAB,24,31,31				// C = A & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static rorb
	rlwinm	r4,rPCAB,31,25,31					// r4 = result = B >> 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rDPFlags,3,28,28 			// N = C
	rlwimi	r4,rDPFlags,7,24,24					// r4 = result = (r3 >> 1) | (C << 7)
	rlwimi	rDPFlags,rPCAB,0,31,31				// C = B & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory right rotate 16-bit
	//
entry static rorw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	rlwinm	r4,rTempSave,31,17,31				// r4 = result = rTempSave >> 1
	rlwimi	rDPFlags,rDPFlags,3,28,28			// N = C
	rlwimi	r4,rDPFlags,15,16,16				// r4 = result = (rTempSave >> 1) | (C << 15)
	rlwimi	rDPFlags,rTempSave,0,31,31			// C = rTempSave & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEAWord

	//
	//		Register right rotate 16-bit variable
	//
entry static rord_im	// not yet
	CYCLES(2,2,2)
	READ_OPCODE_ARG(r6)							// get shift count in r6
rord_common:
	cmpwi	r6,0
	GET_D(r4)
	beq		rord_done
rord_loop:
	mr		r3,r4
	subic.	r6,r6,1
	rlwinm	r4,r3,31,17,31						// r4 = result = r3 >> 1
	rlwimi	rDPFlags,rDPFlags,3,28,28			// N = C
	rlwimi	r4,rDPFlags,15,16,16				// r4 = result = (r3 >> 1) | (C << 15)
	rlwimi	rDPFlags,r3,0,31,31					// C = r3 & 1
	bne		rord_loop
rord_done:
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static rord_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	mr		r6,r3								// r6 = byte at rEA
	b		rord_common
#endif

	//================================================================================================

	//
	//		ASR: register/memory arithmetic right shift operations
	//

	//
	//		Memory arithmetic right shift
	//
entry static asr_di
	DIRECT
	CYCLES(6,6,6)
asrcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,31,25,31						// r4 = result = r3 >> 1
	rlwimi	rDPFlags,r3,28,28,28 				// N = (r3 & 0x80)
	rlwimi	r4,r3,0,24,24						// r4 = result = (r3 >> 1) | (r3 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r3,0,31,31					// C = r3 & 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static asr_ix
	CYCLES(6,6,6)
	INDEXED
	b		asrcommon

entry static asr_ex
	CYCLES(7,7,7)
	EXTENDED
	b		asrcommon

	//
	//		Register (A/B) arithmetic right shift
	//
entry static asra
	rlwinm	r4,rPCAB,23,25,31					// r4 = result = A >> 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rPCAB,20,28,28				// N = (A & 0x80)
	rlwimi	r4,rPCAB,24,24,24					// r4 = result = (A >> 1) | (A & 0x80)
	rlwimi	rDPFlags,rPCAB,24,31,31				// C = A & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static asrb
	rlwinm	r4,rPCAB,31,25,31					// r4 = result = B >> 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rPCAB,28,28,28				// N = (B & 0x80)
	rlwimi	r4,rPCAB,0,24,24					// r4 = result = (B >> 1) | (B & 0x80)
	rlwimi	rDPFlags,rPCAB,0,31,31				// C = B & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory arithmetic right shift 16 bits
	//
entry static asrw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	rlwinm	r4,rTempSave,31,17,31				// r4 = result = rTempSave >> 1
	rlwimi	rDPFlags,rTempSave,20,28,28 		// N = (rTempSave & 0x8000)
	rlwimi	r4,rTempSave,0,16,16				// r4 = result = (rTempSave >> 1) | (r3 & 0x8000)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,0,31,31			// C = rTempSave & 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEAWord

	//
	//		Register arithmetic right shift 16 bits variable
	//
entry static asrd_im	// not yet
	CYCLES(2,2,2)
	READ_OPCODE_ARG(r6)							// get shift count in r6
asrd_common:
	subic.	r7,r6,1								// subtract by 1
	cmpwi	cr7,r6,15
	extsh	r3,rPCAB							// sign-extend D into r3
	blt		asrd_noshift						// handle 0 count
	bgt		cr7,asrd_overshift					// handle >15 count
	sraw	r3,r3,r7							// pre-shift r3 by count-1
	rlwinm	r4,r3,31,17,31						// r4 = result = r3 >> 1
	rlwimi	rDPFlags,r3,20,28,28 				// N = (r3 & 0x8000)
	rlwimi	r4,r3,0,16,16						// r4 = result = (r3 >> 1) | (r3 & 0x8000)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r3,0,31,31					// C = r3 & 1
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd
asrd_overshift:
	rlwimi	rPCAB,r3,16,16,31
asrd_noshift:
	b		executeLoopEnd

entry static asrd_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	mr		r6,r3								// r6 = byte at rEA
	b		asrd_common
#endif

	//================================================================================================

	//
	//		ASL: register/memory arithmetic left shift operations
	//

	//
	//		Memory arithmetic left shift
	//
entry static asl_di
	DIRECT
	CYCLES(6,6,6)
aslcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,1,24,30						// r4 = result = r3 << 1
	rlwimi	rDPFlags,r3,29,28,28 				// N = (r3 & 0x40)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r3,25,31,31				// C = r3 & 0x80
	xor		r9,r3,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,26,30,30				// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEndWriteEA

entry static asl_ix
	CYCLES(6,6,6)
	INDEXED
	b		aslcommon

entry static asl_ex
	CYCLES(7,7,7)
	EXTENDED
	b		aslcommon

	//
	//		Register (A/B) arithmetic left shift
	//
entry static asla
	rlwinm	r4,rPCAB,25,24,30					// r4 = result = A << 1
	CYCLES(2,2,2)
	GET_A(r5)									// r5 = A
	rlwimi	rDPFlags,rPCAB,21,28,28				// N = (A & 0x40)
	xor		r9,r5,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,rPCAB,17,31,31				// C = A & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,26,30,30				// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

entry static aslb
	rlwinm	r4,rPCAB,1,24,30					// r4 = result = B << 1
	CYCLES(2,2,2)
	rlwimi	rDPFlags,rPCAB,29,28,28				// N = (B & 0x40)
	xor		r9,rPCAB,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,rPCAB,25,31,31				// C = B & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,26,30,30				// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory arithmetic left shift
	//
entry static aslw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	rlwinm	r4,rTempSave,1,16,30				// r4 = result = rTempSave << 1
	rlwimi	rDPFlags,rTempSave,21,28,28 		// N = (rTempSave & 0x4000)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,17,31,31			// C = rTempSave & 0x8000
	xor		r9,rTempSave,r4						// r9 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,18,30,30				// V = (r4 ^ (r4 >> 1)) & 0x8000
	b		executeLoopEndWriteEAWord

	//
	//		Register logical left shift 16-bit variable
	//
entry static asld_im	// not yet
	CYCLES(2,2,2)
	READ_OPCODE_ARG(r6)							// get shift count in r6
asld_common:
	GET_D(r3)									// r3 = D
	subic.	r7,r6,1								// subtract by 1
	cmpwi	cr7,r6,15
	blt		asld_noshift						// handle 0 count
	bgt		cr7,asld_overshift					// handle >15 count
	slw		r3,r3,r7							// pre-shift r3 by count-1
	rlwinm	r4,r3,1,16,30						// r4 = result = r3 << 1
	rlwimi	rDPFlags,r3,17,31,31				// C = r3 & 0x8000
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rDPFlags,rDPFlags,0,29,27			// N = 0
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd
asld_overshift:
	rlwinm	rPCAB,rPCAB,0,0,15
asld_noshift:
	b		executeLoopEnd

entry static asld_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	mr		r6,r3
	b		asld_common
#endif

	//================================================================================================

	//
	//		ROL: register/memory left rotate operations
	//

	//
	//		Memory left rotate
	//
entry static rol_di
	DIRECT
	CYCLES(6,6,6)
rolcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,1,24,30						// r4 = result = r3 << 1
	rlwimi	r4,rDPFlags,0,31,31					// r4 = result = (r3 << 1) | C
	rlwimi	rDPFlags,r3,29,28,28 				// N = (r3 & 0x40)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r3,25,31,31				// C = r3 & 0x80
	xor		r9,r3,r4							// r9 = r4 ^ r3 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,26,30,30				// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEndWriteEA

entry static rol_ix
	CYCLES(6,6,6)
	INDEXED
	b		rolcommon

entry static rol_ex
	CYCLES(7,7,7)
	EXTENDED
	b		rolcommon

	//
	//		Register (A/B) left rotate
	//
entry static rola
	rlwinm	r4,rPCAB,25,24,30					// r4 = result = A << 1
	CYCLES(2,2,2)
	rlwimi	r4,rDPFlags,0,31,31					// r4 = result = (A << 1) | C
	GET_A(r3)									// r3 = A
	rlwimi	rDPFlags,rPCAB,21,28,28				// N = (A & 0x40)
	xor		r9,r3,r4							// r9 = r4 ^ r3 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,rPCAB,17,31,31				// C = A & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,26,30,30				// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

entry static rolb
	rlwinm	r4,rPCAB,1,24,30					// r4 = result = B << 1
	CYCLES(2,2,2)
	rlwimi	r4,rDPFlags,0,31,31					// r4 = result = (B << 1) | C
	rlwimi	rDPFlags,rPCAB,29,28,28				// N = (B & 0x40)
	xor		r9,rPCAB,r4							// r9 = r4 ^ rPCAB = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,rPCAB,25,31,31				// C = B & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,26,30,30				// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

#if (A6809_CHIP == 5000)
	//
	//		Memory left rotate
	//
entry static rolw_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_WORD_SAVE(rEA)							// rTempSave = word at rEA
	rlwinm	r4,rTempSave,1,16,30				// r4 = result = rTempSave << 1
	rlwimi	r4,rDPFlags,0,31,31					// r4 = result = (rTempSave << 1) | C
	rlwimi	rDPFlags,rTempSave,21,28,28 		// N = (rTempSave & 0x4000)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,rTempSave,17,31,31			// C = rTempSave & 0x8000
	xor		r9,rTempSave,r4						// r9 = r4 ^ rTempSave = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	rlwimi	rDPFlags,r9,18,30,30				// V = (r4 ^ (r4 >> 1)) & 0x8000
	b		executeLoopEndWriteEAWord

	//
	//		Register left rotate 16-bit variable
	//
entry static rold_im	// not yet
	CYCLES(2,2,2)
	READ_OPCODE_ARG(r6)							// get shift count in r6
rold_common:
	cmpwi	r6,0
	GET_D(r4)
	beq		rold_done
rold_loop:
	mr		r3,r4
	subic.	r6,r6,1
	rlwinm	r4,r3,1,16,30						// r4 = result = r3 << 1
	rlwimi	r4,rDPFlags,0,31,31					// r4 = result = (r3 << 1) | C
	rlwimi	rDPFlags,r3,21,28,28 				// N = (r3 & 0x4000)
	rlwimi	rDPFlags,r3,17,31,31				// C = r3 & 0x8000
	xor		r9,r3,r4							// r9 = r4 ^ r3 = r4 ^ (r4 >> 1)
	rlwimi	rDPFlags,r9,18,30,30				// V = (r4 ^ (r4 >> 1)) & 0x8000
	bne		rold_loop
rold_done:
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static rold_ix	// not yet
	CYCLES(6,6,6)
	INDEXED
	READ_BYTE_SAVE(rEA)							// r3 = word at rEA
	mr		r6,r3
	b		rold_common
#endif

	//================================================================================================

	//
	//		COM: register/memory logical complement
	//

	//
	//		Memory logical complement
	//
entry static com_di
	DIRECT
	CYCLES(6,6,6)
comcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	xori	r4,r3,0xff							// r4 = r3 ^ 0xff
	li		r0,k6809FlagC						// r0 = k6809FlagC
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r0,0,30,31					// V = 0, C = 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static com_ix
	CYCLES(6,6,6)
	INDEXED
	b		comcommon

entry static com_ex
	CYCLES(7,7,7)
	EXTENDED
	b		comcommon

	//
	//		Register (A/B) logical complement
	//
entry static coma
	GET_A(r3)									// r3 = A
	CYCLES(2,2,2)
	xori	r4,r3,0xff							// r4 = result = r3 ^ 0xff
	li		r0,k6809FlagC						// r0 = k6809FlagC
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r0,0,30,31					// V = 0, C = 1
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static comb
	GET_B(r3)									// r3 = B
	CYCLES(2,2,2)
	xori	r4,r3,0xff							// r4 = result = r3 ^ 0xff
	li		r0,k6809FlagC						// r0 = k6809FlagC
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r0,0,30,31					// V = 0, C = 1
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

	//================================================================================================

	//
	//		AND: register/memory logical AND
	//

	//
	//		ANDA
	//
entry static anda_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		andacommon

entry static anda_di
	DIRECT
	CYCLES(4,4,4)
andamemcommon:
	READ_BYTE_SAVE(rEA)
andacommon:
	GET_A(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static anda_ix
	CYCLES(4,4,4)
	INDEXED
	b		andamemcommon

entry static anda_ex
	EXTENDED
	CYCLES(5,5,5)
	b		andamemcommon

	//
	//		ANDB
	//
entry static andb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		andbcommon

entry static andb_di
	DIRECT
	CYCLES(4,4,4)
andbmemcommon:
	READ_BYTE_SAVE(rEA)
andbcommon:
	GET_B(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	SET_B(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static andb_ix
	CYCLES(4,4,4)
	INDEXED
	b		andbmemcommon

entry static andb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		andbmemcommon

	//================================================================================================

	//
	//		OR: register/memory logical OR
	//

	//
	//		ORA
	//
entry static ora_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		oracommon

entry static ora_di
	DIRECT
	CYCLES(4,4,4)
oramemcommon:
	READ_BYTE_SAVE(rEA)
oracommon:
	GET_A(r5)									// r5 = A
	or		r4,r5,r3							// r4 = r5 | r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static ora_ix
	CYCLES(4,4,4)
	INDEXED
	b		oramemcommon

entry static ora_ex
	EXTENDED
	CYCLES(5,5,5)
	b		oramemcommon

	//
	//		ORB
	//
entry static orb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		orbcommon

entry static orb_di
	DIRECT
	CYCLES(4,4,4)
orbmemcommon:
	READ_BYTE_SAVE(rEA)
orbcommon:
	GET_B(r5)									// r5 = A
	or		r4,r5,r3							// r4 = r5 | r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	SET_B(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static orb_ix
	CYCLES(4,4,4)
	INDEXED
	b		orbmemcommon

entry static orb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		orbmemcommon

	//================================================================================================

	//
	//		EOR: register/memory logical exclusive-OR
	//

	//
	//		EORA
	//
entry static eora_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		eoracommon

entry static eora_di
	DIRECT
	CYCLES(4,4,4)
eoramemcommon:
	READ_BYTE_SAVE(rEA)
eoracommon:
	GET_A(r5)									// r5 = A
	xor		r4,r5,r3							// r4 = r5 ^ r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static eora_ix
	CYCLES(4,4,4)
	INDEXED
	b		eoramemcommon

entry static eora_ex
	EXTENDED
	CYCLES(5,5,5)
	b		eoramemcommon

	//
	//		EORB
	//
entry static eorb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		eorbcommon

entry static eorb_di
	DIRECT
	CYCLES(4,4,4)
eorbmemcommon:
	READ_BYTE_SAVE(rEA)
eorbcommon:
	GET_B(r5)									// r5 = A
	xor		r4,r5,r3							// r4 = r5 ^ r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	SET_B(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static eorb_ix
	CYCLES(4,4,4)
	INDEXED
	b		eorbmemcommon

entry static eorb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		eorbmemcommon

	//================================================================================================

	//
	//		BIT: register/memory logical bit test
	//

	//
	//		BITA
	//
entry static bita_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		bitacommon

entry static bita_di
	DIRECT
	CYCLES(4,4,4)
bitamemcommon:
	READ_BYTE_SAVE(rEA)
bitacommon:
	GET_A(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static bita_ix
	CYCLES(4,4,4)
	INDEXED
	b		bitamemcommon

entry static bita_ex
	EXTENDED
	CYCLES(5,5,5)
	b		bitamemcommon

	//
	//		BITB
	//
entry static bitb_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		bitbcommon

entry static bitb_di
	DIRECT
	CYCLES(4,4,4)
bitbmemcommon:
	READ_BYTE_SAVE(rEA)
bitbcommon:
	GET_B(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rDPFlags,rDPFlags,0,31,29			// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static bitb_ix
	CYCLES(4,4,4)
	INDEXED
	b		bitbmemcommon

entry static bitb_ex
	EXTENDED
	CYCLES(5,5,5)
	b		bitbmemcommon

	//================================================================================================

	//
	//		JMP: jump to effective address
	//
entry static jmp_di
	DIRECT
	CYCLES(3,3,3)
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

entry static jmp_ix
	CYCLES(3,3,3)
	INDEXED
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

entry static jmp_ex
	EXTENDED
	CYCLES(4,4,4)
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

	//================================================================================================

	//
	//		JSR: jump to subroutine at effective address
	//
entry static jsr_di
	DIRECT
	CYCLES(7,7,7)
jsrcommon:
	PUSH_WORD_HI_SAVE(rPCAB)
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

entry static jsr_ix
	CYCLES(7,7,7)
	INDEXED
	b		jsrcommon

entry static jsr_ex
	EXTENDED
	CYCLES(8,8,8)
	b		jsrcommon

	//================================================================================================

	//
	//		BRA/LBRA: branch relative
	//
entry static bra
takebra:
	READ_OPCODE_ARG(r3)							// r3 = offset
	subis	rTempSave,rPCAB,2					// rTempSave = original PC
	extsb	r3,r3								// sign-extend
	rlwinm	r3,r3,16,0,15						// rotate high
	add		rPCAB,rPCAB,r3						// adjust the PC
	UPDATE_BANK
	xor.	r0,rTempSave,rPCAB					// see if anything's changed
	CYCLES(3,3,3)
	bne+	executeLoopEnd						// end if we're jumping somewhere different
	li		rICount,0							// snap to zero
	b		executeLoopEnd						// all done

entry static lbra
takelbra:
	READ_OPCODE_ARG_WORD(rEA)
	subis	rTempSave,rPCAB,3					// rTempSave = original PC
	rlwinm	rEA,rEA,16,0,15
	add		rPCAB,rPCAB,rEA
	UPDATE_BANK
	xor.	r0,rTempSave,rPCAB					// see if anything's changed
	CYCLES(5,5,5)
	bne+	executeLoopEnd						// end if we're jumping somewhere different
	li		rICount,0							// snap to zero
	b		executeLoopEnd

	//================================================================================================

	//
	//		BSR/LBSR: branch to subroutine relative
	//
entry static bsr
	READ_OPCODE_ARG(rEA)						// r3 = offset
	CYCLES(7,7,7)
	extsb	rEA,rEA								// sign-extend
	rlwinm	rEA,rEA,16,0,15						// rotate high
bsrcommon:
	PUSH_WORD_HI_SAVE(rPCAB)
	add		rPCAB,rPCAB,rEA						// adjust the PC
	UPDATE_BANK
	b		executeLoopEnd						// all done

entry static lbsr
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(9,9,9)
	rlwinm	rEA,rEA,16,0,15
	b		bsrcommon

	//================================================================================================

	//
	//		RTS/RTI: return from subroutine/interrupt
	//

	//
	//		RTS
	//
entry static rts
	PULL_WORD_HI_SAVE(rPCAB)
	CYCLES(5,5,5)
	UPDATE_BANK
	b		executeLoopEnd

	//
	//		RTI
	//
entry static rti
	SAVE_PCAB
	GET_S(r3)
	SAVE_DPFLAGS
	addis	rSU,rSU,1
	bl		READMEM
	rlwimi	rDPFlags,r3,0,24,31
	SAVE_SU
	CYCLES(4,4,4)
	// no post extern here; it's done in the pullcommon routine, below
	andi.	r0,rDPFlags,k6809FlagE
	li		rTempSave,0x80
	beq		rtinomore
	li		rTempSave,0xfe
rtinomore:
	GET_S(rEA)
	bl		pullcommon
	SET_S(rEA)
	SAVE_SU
	bl		checkIRQLines
	b		executeLoopEnd

	//================================================================================================

	//
	//		BRN/LBRN: branch, but don't really branch -- huh?
	//
entry static brn
	addis	rPCAB,rPCAB,1
	CYCLES(3,3,3)
	b		executeLoopEnd

entry static lbrn
	addis	rPCAB,rPCAB,2
	CYCLES(5,5,5)
	b		executeLoopEnd

	//================================================================================================

	//
	//		Bxx/LBxx: branch conditionally relative
	//

	//
	//		BHI
	//
entry static bhi
	andi.	r0,rDPFlags,5
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbhi
	andi.	r0,rDPFlags,5
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd
takelbra6:
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(6,6,6)
	rlwinm	rEA,rEA,16,0,15
	add		rPCAB,rPCAB,rEA
	UPDATE_BANK
	b		executeLoopEnd

	//
	//		BLS
	//
entry static bls
	andi.	r0,rDPFlags,5
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbls
	andi.	r0,rDPFlags,5
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BCC
	//
entry static bcc
	andi.	r0,rDPFlags,1
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbcc
	andi.	r0,rDPFlags,1
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BCS
	//
entry static bcs
	andi.	r0,rDPFlags,1
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbcs
	andi.	r0,rDPFlags,1
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BNE
	//
entry static bne
	andi.	r0,rDPFlags,4
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbne
	andi.	r0,rDPFlags,4
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BEQ
	//
entry static beq
	andi.	r0,rDPFlags,4
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbeq
	andi.	r0,rDPFlags,4
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BVC
	//
entry static bvc
	andi.	r0,rDPFlags,2
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbvc
	andi.	r0,rDPFlags,2
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BVS
	//
entry static bvs
	andi.	r0,rDPFlags,2
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbvs
	andi.	r0,rDPFlags,2
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BPL
	//
entry static bpl
	andi.	r0,rDPFlags,8
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbpl
	andi.	r0,rDPFlags,8
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BMI
	//
entry static bmi
	andi.	r0,rDPFlags,8
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbmi
	andi.	r0,rDPFlags,8
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BGE
	//
entry static bge
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	xor.	r0,r3,r4
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbge
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	xor.	r0,r3,r4
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BLT
	//
entry static blt
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	xor.	r0,r3,r4
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lblt
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	xor.	r0,r3,r4
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BGT
	//
entry static bgt
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	rlwinm	r5,rDPFlags,30,31,31
	xor		r3,r3,r4
	or.		r0,r3,r5
	beq		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lbgt
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	rlwinm	r5,rDPFlags,30,31,31
	xor		r3,r3,r4
	or.		r0,r3,r5
	beq		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd

	//
	//		BLE
	//
entry static ble
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	rlwinm	r5,rDPFlags,30,31,31
	xor		r3,r3,r4
	or.		r0,r3,r5
	bne		takebra
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

entry static lble
	rlwinm	r3,rDPFlags,29,31,31
	rlwinm	r4,rDPFlags,31,31,31
	rlwinm	r5,rDPFlags,30,31,31
	xor		r3,r3,r4
	or.		r0,r3,r5
	bne		takelbra6
	CYCLES(5,5,5)
	addis	rPCAB,rPCAB,2
	b		executeLoopEnd


	//================================================================================================

#if (A6809_CHIP == 5000)
	//
	//		DECJNZ: decrement & jump
	//
entry static decbjnz	// hit at start
	GET_B(r3)									// r3 = B
	CYCLES(2,2,2)
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rDPFlags,r6,28,30,30				// V = (r3 == 0x80)
	subi	r4,r3,1								// r4 = result = r3 - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0x01)
	SET_B(r4)									// B = r4
	rlwimi	rDPFlags,r4,28,28,28 				// N = (r4 & 0x80)

	andi.	r0,rDPFlags,4
	beq		takebrasafe
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

takebrasafe:
	READ_OPCODE_ARG(r3)							// r3 = offset
	subis	rTempSave,rPCAB,2					// rTempSave = original PC
	extsb	r3,r3								// sign-extend
	rlwinm	r3,r3,16,0,15						// rotate high
	add		rPCAB,rPCAB,r3						// adjust the PC
	UPDATE_BANK
	CYCLES(3,3,3)
	b		executeLoopEnd						// all done

entry static decxjnz	// not yet
	GET_X(r3)									// r3 = X
	CYCLES(3,3,3)
	xori	r7,r3,0x0001						// r7 = r3 ^ 0x0001
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	subi	r4,r3,1								// r4 = result = r3 - 1
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r3 == 0x0001)
	SET_X(r4)									// B = r4
	rlwimi	rDPFlags,r4,20,28,28 				// N = (r4 & 0x8000)

	andi.	r0,rDPFlags,4
	beq		takebrasafe
	CYCLES(3,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd
#endif


	//================================================================================================

	//
	//		SEX/DAA: adjust accumulator
	//

	//
	//		SEX
	//
entry static sex
	GET_B(r3)									// r3 = B
	rlwinm	rDPFlags,rDPFlags,0,31,29 			// V = 0
	CYCLES(2,2,2)
	extsb	r4,r3								// r4 = B sign extended
	rlwimi	rDPFlags,r3,28,28,28				// N = (B & 0x80)
	cntlzw	r7,r3								// r7 = number of zeros in r3
	SET_D(r4)									// D = sign extended B
	rlwimi	rDPFlags,r7,29,29,29				// Z = (B == 0)
	b		executeLoopEnd
	
	//
	//		DAA
	//
entry static daa
	GET_A(r4)
	rlwinm	r5,rPCAB,24,28,31					// r5 = lsn = A & 0xf
	rlwinm	r6,rPCAB,20,28,31					// r6 = msn = A >> 4
	cmpwi	cr1,r5,9							// compare lsn to 9
	andi.	r0,rDPFlags,k6809FlagH				// test the H flag
	cmpwi	cr2,r6,9							// compare msn to 9
	rlwinm	rDPFlags,rDPFlags,0,31,29 			// V = 0
	bgt		cr1,daa_add06						// if (lsn > 9), we need to add 6
	beq		daa_dontadd06						// if (lsn < 9 && !(cc & k6809FlagH)), skip
daa_add06:
	addi	r4,r4,0x06							// r4 += 0x06
daa_dontadd06:
	andi.	r0,rDPFlags,k6809FlagC				// test the C flag
	blt		cr2,daa_checkc						// if msn < 9, check the C flag
	bgt		cr2,daa_add60						// if msn > 9, add
	ble		cr1,daa_checkc						// if (msn == 9 && lsn <= 9), the C flag is our only hope
daa_add60:
	addi	r4,r4,0x60							// r4 += 0x60
	b		daa_finish							// skip to the finish
daa_checkc:
	bne		daa_add60							// if (cc & k6809FlagC), add anyhow
daa_finish:
	rlwinm	r7,r4,0,24,31						// r7 = r4 & 0xff
	rlwinm	r3,r4,24,31,31						// r3 = C bit
	cntlzw	r7,r7								// r7 = number of zeros in r7
	rlwimi	rDPFlags,r4,28,28,28				// N = (A & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rDPFlags,r7,29,29,29				// Z = (A == 0)
	CYCLES(2,2,2)
	or		rDPFlags,rDPFlags,r3				// C |= (r4 & 0x100)
	b		executeLoopEnd

	//================================================================================================

	//
	//		EXG/TFR: exchange/transfer between registers
	//

	//
	//		EXG
	//
entry static exg
	READ_OPCODE_ARG(r3)							// r3 = registers
	mtcrf	0xff,r3								// copy into CR
	li		rTempSave,0
	CYCLES(8,8,8)									// count the cycles
	EXPAND_REG_GET(24,gethi,r4,gethidone)		// get the first register in r4
gethidone:
	EXPAND_REG_GET(28,getlo,r3,getlodone)		// get the second register in r3
getlodone:
	EXPAND_REG_SET(24,sethi,r3,sethidone)		// set the first register to r3
sethidone:
	EXPAND_REG_SET(28,setlo,r4,setlodone) 		// set the second register to r4
setlodone:
	cmpwi	cr1,rTempSave,0						// do we need to check for IRQs?
	beq		cr1,executeLoopEnd					// if not, bail
	andi.	r0,rTempSave,2						// do we need to update the PC?
	beq		nopcupdate
	UPDATE_BANK
	andi.	r0,rTempSave,1
	beq		executeLoopEnd
nopcupdate:
	bl		checkIRQLines
	b		executeLoopEnd						// last case always falls through

	//
	//		TFR
	//
#if (A6809_CHIP != 5000)

entry static tfr
	READ_OPCODE_ARG(r3)							// r3 = registers
	mtcrf	0xff,r3								// copy into CR
	li		r5,0
	CYCLES(6,6,6)								// count the cycles
	EXPAND_REG_GET(24,gethi2,r4,sethidone)		// get the first register in r4
	b		sethidone							// last case always falls through

#else

entry static tfr
	READ_OPCODE_ARG(r3)							// r3 = registers
	mtcrf	0xff,r3								// copy into CR
	li		r5,0
	CYCLES(6,6,6)								// count the cycles
	EXPAND_REG_GET(28,gethi2,r4,tfrgetdone)		// get the first register in r4
tfrgetdone:
	EXPAND_REG_SET(24,setlo2,r4,setlodone) 		// set the second register to r4
	b		setlodone							// last case always falls through

#endif

	//================================================================================================

	//
	//		NOP/SYNC/CWAI: processor timing/delay instructions
	//

	//
	//		NOP
	//
entry static nop
	CYCLES(2,2,2)
	b		executeLoopEnd

	//
	//		SYNC
	//
entry static sync
	_asm_get_global_b(r0,sInterruptState)
	ori		r0,r0,kInterruptStateSYNC			// set the SYNC bit
	_asm_set_global_b(r0,sInterruptState)
	bl		checkIRQLines						// check for interrupts
	_asm_get_global_b(r0,sInterruptState)
	CYCLES(4,4,4)
	andi.	r0,r0,kInterruptStateSYNC			// see if the SYNC bit is still on
	cmpwi	cr1,rICount,0
	beq		executeLoopEnd
	ble		cr1,executeLoopEnd
	li		rICount,0							// if so, wait for an interrupt to occur
	b		executeLoopEnd

	//
	//		CWAI
	//
entry static cwai
	READ_OPCODE_ARG(r3)
	and		r3,rDPFlags,r3						// mask CC with the constant
	SET_CC(r3)
	ori		rDPFlags,rDPFlags,k6809FlagE		// set the E bit as well
	li		rTempSave,0xff
	GET_S(rEA)
	bl		pushcommon							// push the entire CPU state
	SET_S(rEA)
	_asm_get_global_b(r0,sInterruptState)
	CYCLES(20-12,20-12,20-12)	// pushcommon counts 12 cycles for 12 bytes
	ori		r0,r0,kInterruptStateCWAI			// set the CWAI bit
	_asm_set_global_b(r0,sInterruptState)
	bl		checkIRQLines						// check for interrupts
	_asm_get_global_b(r0,sInterruptState)
	andi.	r0,r0,kInterruptStateCWAI			// see if the CWAI bit is still on
	cmpwi	cr1,rICount,0
	beq		executeLoopEnd
	ble		cr1,executeLoopEnd
	li		rICount,0							// if so, wait for an interrupt to occur
	b		executeLoopEnd

	//================================================================================================

	//
	//		ORCC/ANDCC: modify the CC register
	//

	//
	//		ORCC
	//
entry static orcc
	READ_OPCODE_ARG(r3)
	CYCLES(3,3,3)
	or		rDPFlags,rDPFlags,r3
	b		executeLoopEnd

	//
	//		ANDCC
	//
entry static andcc
	READ_OPCODE_ARG(r3)
	CYCLES(3,3,3)
	and		r4,rDPFlags,r3
	SET_CC(r4)
	bl		checkIRQLines
	b		executeLoopEnd

	//================================================================================================

	//
	//		LEA: compute effective address and load
	//
entry static leax
	INDEXED
	rlwinm	r3,rEA,0,16,31
	SET_X(rEA)
	cntlzw	r3,r3
	CYCLES(4,4,4)
	rlwimi	rDPFlags,r3,29,29,29
	SAVE_XY
	b		executeLoopEnd

entry static leay
	INDEXED
	rlwinm	r3,rEA,0,16,31
	SET_Y(rEA)
	cntlzw	r3,r3
	CYCLES(4,4,4)
	rlwimi	rDPFlags,r3,29,29,29
	SAVE_XY
	b		executeLoopEnd

entry static leas
	INDEXED
	SET_S(rEA)
	_asm_get_global_b(r0,sInterruptState)
	CYCLES(4,4,4)
	andi.	r0,r0,~kInterruptStateInhibitNMI & 0xffff
	SAVE_SU
	_asm_set_global_b(r0,sInterruptState)
	b		executeLoopEnd

entry static leau
	INDEXED
	SET_U(rEA)
	CYCLES(4,4,4)
	SAVE_SU
	b		executeLoopEnd

	//================================================================================================

#if (A6809_CHIP == 5000)

	//
	//		ABS: absolute value of register
	//
entry static absa	// not yet
	rlwinm.	r0,rPCAB,0,16,16
	GET_A(r3)									// r3 = A
	CYCLES(2,2,2)
	mr		r4,r3
	beq		absa_dont
	neg		r4,r3								// r4 = result = -r3
absa_dont:
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_A(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd

entry static absb	// not yet
	rlwinm.	r0,rPCAB,0,24,24
	GET_B(r3)									// r3 = A
	CYCLES(2,2,2)
	mr		r4,r3
	beq		absb_dont
	neg		r4,r3								// r4 = result = -r3
absb_dont:
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,24,31,31				// C = (r4 & 0x100)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,28,28,28				// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,26,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_B(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd
	
entry static absd	// not yet
	rlwinm.	r0,rPCAB,0,16,16
	GET_D(r3)									// r3 = D
	CYCLES(2,2,2)
	mr		r4,r3
	beq		absd_dont
	neg		r4,r3								// r4 = result = -r3
absd_dont:
	xor		r8,r3,r4							// r8 = r3 ^ r4
	rlwimi	rDPFlags,r4,16,31,31				// C = (r4 & 0x10000)
	rlwinm	r9,r4,31,0,31						// r9 = r4 >> 1
	rlwimi	rDPFlags,r4,20,28,28				// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rDPFlags,r9,18,30,30				// V = (r3 ^ r4 ^ (r4 >> 1)) & 0x8000
	SET_D(r4)
	rlwimi	rDPFlags,r7,29,29,29				// Z = (r4 == 0)
	b		executeLoopEnd
	
#endif

	//================================================================================================

#if (A6809_CHIP == 5000)

	//
	//		BSET: block set
	//
entry static bset	// not yet
	rlwinm.	rTempSave,rSU,0,16,31
	beq		bset_done
	SAVE_PCAB
	SAVE_ICOUNT
bset_loop:
	GET_X(r3)
	GET_A(r4)
	addis	rXY,rXY,1
	bl		WRITEMEM
	subic.	rTempSave,rTempSave,1
	CYCLES(2,2,2)
	bne		bset_loop
bset_done:
	SET_U(rTempSave)
	bl		postExtern
	b		executeLoopEnd

	//
	//		BSET2: block set words
	//
entry static bset2	// not yet
	rlwinm.	rTempSave,rSU,0,16,31
	beq		bset2_done
	SAVE_PCAB
	SAVE_ICOUNT
bset2_loop:
	GET_X(r3)
	GET_A(r4)
	addis	rXY,rXY,1
	bl		WRITEMEM
	GET_X(r3)
	GET_B(r4)
	addis	rXY,rXY,1
	bl		WRITEMEM
	subic.	rTempSave,rTempSave,1
	CYCLES(3,3,3)
	bne		bset2_loop
bset2_done:
	SET_U(rTempSave)
	bl		postExtern
	b		executeLoopEnd
	
#endif

	//================================================================================================

#if (A6809_CHIP == 5000)

	//
	//		MOVE: move memory
	//
entry static move	// not yet
	SAVE_PCAB
	SAVE_ICOUNT
	GET_Y(r3)
	addi	r4,r3,1
	SET_Y(r4)
	bl		READMEM
	rlwinm	r4,r3,0,24,31
	GET_X(r3)
	addis	rXY,rXY,1
	bl		WRITEMEM
	GET_U(r6)
	subi	r6,r6,1
	SET_U(r6)
	CYCLES(2,2,2)
	b		executeLoopEnd

	//
	//		BMOVE: block move memory
	//
entry static bmove	// not yet
	rlwinm.	rTempSave,rSU,0,16,31
	beq		bmove_done
	SAVE_PCAB
	SAVE_ICOUNT
bmove_loop:
	GET_Y(r3)
	addi	r4,r3,1
	SET_Y(r4)
	bl		READMEM
	rlwinm	r4,r3,0,24,31
	GET_X(r3)
	addis	rXY,rXY,1
	bl		WRITEMEM
	subic.	rTempSave,rTempSave,1
	CYCLES(2,2,2)
	bne		bmove_loop
bmove_done:
	SET_U(rTempSave)
	bl		postExtern
	b		executeLoopEnd
	
#endif

	//================================================================================================

#if (A6809_CHIP == 5000)

	//
	//		SETLINE: set address lines A23-A16
	//
entry static setline_im		// hit during startup
	_asm_get_global(rTempSave,A6809_SETLINES)
	READ_OPCODE_ARG(r3)
#if TARGET_RT_MAC_CFM
	lwz		rTempSave,0(rTempSave)
#endif
	b		setline_common
	
entry static setline_ix		// hit during startup
	_asm_get_global(rTempSave,A6809_SETLINES)
	INDEXED
#if TARGET_RT_MAC_CFM
	lwz		rTempSave,0(rTempSave)
#endif
	READ_BYTE_SAVE(rEA)
setline_common:
	cmpwi	rTempSave,0
#if TARGET_RT_MAC_CFM
	lwz		rTempSave,0(rTempSave)
#endif
	mtctr	rTempSave
	beq		setline_dontdoit
	bctrl
setline_dontdoit:
	b		executeLoopEnd
	
#endif

	//================================================================================================

	//
	// 	effective address handlers: compute the effective address and store it in rEA
	//
entry static eaz
	li		rEA,0
	blr

#if (A6809_CHIP == 5000)
entry static eadi
	DIRECT
	CYCLES(1,1,1)
	blr

entry static eaex
	EXTENDED
	CYCLES(2,2,2)
	blr
#endif

#if (A6809_CHIP != 5000)
	//
	//		X/Y/U/S + 5 bit offset
	//
entry static eaxpk
	rlwinm	r3,r3,27,0,4
	GET_X(rEA)
	srawi	r3,r3,27
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaypk
	rlwinm	r3,r3,27,0,4
	GET_Y(rEA)
	srawi	r3,r3,27
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaupk
	rlwinm	r3,r3,27,0,4
	GET_U(rEA)
	srawi	r3,r3,27
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static easpk
	rlwinm	r3,r3,27,0,4
	GET_S(rEA)
	srawi	r3,r3,27
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr
#endif

	//
	//		X/Y/U/S/PC+
	//
entry static eaxi
	GET_X(rEA)
	addis	rXY,rXY,1
	SAVE_XY
	CYCLES(2,2,2)
	blr

entry static eayi
	addi	r3,rXY,1
	GET_Y(rEA)
	SET_Y(r3)
	SAVE_XY
	CYCLES(2,2,2)
	blr

entry static eaui
	addi	r3,rSU,1
	GET_U(rEA)
	SET_U(r3)
	SAVE_SU
	CYCLES(2,2,2)
	blr

entry static easi
	GET_S(rEA)
	addis	rSU,rSU,1
	SAVE_SU
	CYCLES(2,2,2)
	blr

#if (A6809_CHIP == 5000)
entry static eapci
	GET_PC(rEA)
	addis	rPCAB,rPCAB,1
	SAVE_PCAB
	CYCLES(2,2,2)
	blr
#endif

	//
	//		X/Y/U/S/PC++
	//
entry static eaxii
	GET_X(rEA)
	addis	rXY,rXY,2
	SAVE_XY
	CYCLES(3,3,3)
	blr

entry static eayii
	addi	r3,rXY,2
	GET_Y(rEA)
	SET_Y(r3)
	SAVE_XY
	CYCLES(3,3,3)
	blr

entry static eauii
	addi	r3,rSU,2
	GET_U(rEA)
	SET_U(r3)
	SAVE_SU
	CYCLES(3,3,3)
	blr

entry static easii
	GET_S(rEA)
	addis	rSU,rSU,2
	SAVE_SU
	CYCLES(3,3,3)
	blr

#if (A6809_CHIP == 5000)
entry static eapcii
	GET_PC(rEA)
	addis	rPCAB,rPCAB,2
	SAVE_PCAB
	CYCLES(3,3,3)
	blr
#endif

	//
	//		-X/Y/U/S/PC
	//
entry static eaxd
	subis	rXY,rXY,1
	GET_X(rEA)
	SAVE_XY
	CYCLES(2,2,2)
	blr

entry static eayd
	GET_Y(rEA)
	subi	rEA,rEA,1
	SET_Y(rEA)
	SAVE_XY
	CYCLES(2,2,2)
	blr

entry static eaud
	GET_U(rEA)
	subi	rEA,rEA,1
	SET_U(rEA)
	SAVE_SU
	CYCLES(2,2,2)
	blr

entry static easd
	subis	rSU,rSU,1
	GET_S(rEA)
	SAVE_SU
	CYCLES(2,2,2)
	blr

#if (A6809_CHIP == 5000)
entry static eapcd
	subis	rPCAB,rPCAB,1
	GET_PC(rEA)
	SAVE_PCAB
	CYCLES(2,2,2)
	blr
#endif

	//
	//		--X/Y/U/S/PC
	//
entry static eaxdd
	subis	rXY,rXY,2
	GET_X(rEA)
	SAVE_XY
	CYCLES(3,3,3)
	blr

entry static eaydd
	GET_Y(rEA)
	subi	rEA,rEA,2
	SET_Y(rEA)
	SAVE_XY
	CYCLES(3,3,3)
	blr

entry static eaudd
	GET_U(rEA)
	subi	rEA,rEA,2
	SET_U(rEA)
	SAVE_SU
	CYCLES(3,3,3)
	blr

entry static easdd
	subis	rSU,rSU,2
	GET_S(rEA)
	SAVE_SU
	CYCLES(3,3,3)
	blr

#if (A6809_CHIP == 5000)
entry static eapcdd
	subis	rPCAB,rPCAB,2
	GET_PC(rEA)
	SAVE_PCAB
	CYCLES(3,3,3)
	blr
#endif

	//
	//		X/Y/U/S/PC
	//
entry static eax
	GET_X(rEA)
	blr

entry static eay
	GET_Y(rEA)
	blr

entry static eau
	GET_U(rEA)
	blr

entry static eas
	GET_S(rEA)
	blr

#if (A6809_CHIP == 5000)
entry static eapc
	GET_PC(rEA)
	blr
#endif

	//
	//		X/Y/U/S/PC + B
	//
entry static eaxpb
	GET_B(r3)
	extsb	r3,r3
	GET_X(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaypb
	GET_B(r3)
	extsb	r3,r3
	GET_Y(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaupb
	GET_B(r3)
	extsb	r3,r3
	GET_U(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static easpb
	GET_B(r3)
	extsb	r3,r3
	GET_S(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

#if (A6809_CHIP == 5000)
entry static eapcpb
	GET_B(r3)
	extsb	r3,r3
	GET_PC(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr
#endif

	//
	//		X/Y/U/S/PC + A
	//
entry static eaxpa
	GET_A(r3)
	extsb	r3,r3
	GET_X(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaypa
	GET_A(r3)
	extsb	r3,r3
	GET_Y(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaupa
	GET_A(r3)
	extsb	r3,r3
	GET_U(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static easpa
	GET_A(r3)
	extsb	r3,r3
	GET_S(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

#if (A6809_CHIP == 5000)
entry static eapcpa
	GET_A(r3)
	extsb	r3,r3
	GET_PC(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr
#endif

	//
	//		X/Y/U/S/PC + 8 bit offset
	//
entry static eaxp8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_X(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eayp8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_Y(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eaup8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_U(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static easp8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_S(rEA)
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

entry static eapcp8
#if (A6809_CHIP != 5000)
	READ_OPCODE_ARG(r3)
	GET_PC(rEA)
#else
	GET_PC(rEA)
	READ_OPCODE_ARG(r3)
#endif
	extsb	r3,r3
	add		rEA,rEA,r3
	CYCLES(1,1,1)
	blr

	//
	//		X/Y/U/S/PC + 16 bit offset
	//
entry static eaxp16
	READ_OPCODE_ARG_WORD(r3)
	GET_X(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static eayp16
	READ_OPCODE_ARG_WORD(r3)
	GET_Y(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static eaup16
	READ_OPCODE_ARG_WORD(r3)
	GET_U(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static easp16
	READ_OPCODE_ARG_WORD(r3)
	GET_S(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static eapcp16
#if (A6809_CHIP != 5000)
	READ_OPCODE_ARG_WORD(r3)
	GET_PC(rEA)
#else
	GET_PC(rEA)
	READ_OPCODE_ARG_WORD(r3)
#endif
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

	//
	//		X/Y/U/S/PC + D
	//
entry static eaxpd
	GET_D(r3)
	GET_X(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static eaypd
	GET_D(r3)
	GET_Y(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static eaupd
	GET_D(r3)
	GET_U(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

entry static easpd
	GET_D(r3)
	GET_S(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr

#if (A6809_CHIP == 5000)
entry static eapcpd
	GET_D(r3)
	GET_PC(rEA)
	add		rEA,rEA,r3
	CYCLES(4,4,4)
	blr
#endif

	//
	//		16 bit absolute
	//
entry static ea16
	READ_OPCODE_ARG_WORD(rEA)
	CYCLES(5,5,5)
	blr

#if (A6809_CHIP == 5000)
entry static eaeadi
	READ_OPCODE_ARG(rTempSave)
	rlwimi	rTempSave,rDPFlags,0,16,23
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaex
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(4,4,4)
	b		eaderef
#endif

	//
	//		(X/Y/U/S/PC+)
	//
entry static eaeaxi
	GET_X(rTempSave)
	addis	rXY,rXY,1
	SAVE_XY
	CYCLES(5,5,5)
eaderef:
	SAVE_DPFLAGS
	mflr	rLinkSave							// save the LR
	SAVE_PCAB									// save PC/AB
	rlwinm	r3,rTempSave,0,16,31				// get the address in r3
	bl		READMEM								// perform the read
	addi	r4,rTempSave,1						// add one to the address
	rlwinm	rEA,r3,8,16,23						// save the result
	rlwinm	r3,r4,0,16,31						// keep it in range
	bl		READMEM								// perform the read
	bl		postExtern							// post-process
	mtlr	rLinkSave							// restore link reg
	rlwimi	rEA,r3,0,24,31						// combine the two bytes
	blr

entry static eaeayi
	addi	r3,rXY,1
	GET_Y(rTempSave)
	SET_Y(r3)
	SAVE_XY
	CYCLES(5,5,5)
	b		eaderef

entry static eaeaui
	addi	r3,rSU,1
	GET_U(rTempSave)
	SET_U(r3)
	SAVE_SU
	CYCLES(5,5,5)
	b		eaderef

entry static eaeasi
	GET_S(rTempSave)
	addis	rSU,rSU,1
	SAVE_SU
	CYCLES(5,5,5)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapci
	GET_PC(rTempSave)
	addis	rPCAB,rPCAB,1
	SAVE_PCAB
	CYCLES(5,5,5)
	b		eaderef
#endif

	//
	//		(X/Y/U/S/PC++)
	//
entry static eaeaxii
	GET_X(rTempSave)
	addis	rXY,rXY,2
	SAVE_XY
	CYCLES(6,6,6)
	b		eaderef

entry static eaeayii
	addi	r3,rXY,2
	GET_Y(rTempSave)
	SET_Y(r3)
	SAVE_XY
	CYCLES(6,6,6)
	b		eaderef

entry static eaeauii
	addi	r3,rSU,2
	GET_U(rTempSave)
	SET_U(r3)
	SAVE_SU
	CYCLES(6,6,6)
	b		eaderef

entry static eaeasii
	GET_S(rTempSave)
	addis	rSU,rSU,2
	SAVE_SU
	CYCLES(6,6,6)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapcii
	GET_PC(rTempSave)
	addis	rPCAB,rPCAB,2
	SAVE_PCAB
	CYCLES(6,6,6)
	b		eaderef
#endif

	//
	//		(-X/Y/U/S/PC)
	//
entry static eaeaxd
	GET_X(rTempSave)
	subi	rTempSave,rTempSave,1
	SET_X(rTempSave)
	SAVE_XY
	CYCLES(5,5,5)
	b		eaderef

entry static eaeayd
	GET_Y(rTempSave)
	subi	rTempSave,rTempSave,1
	SET_Y(rTempSave)
	SAVE_XY
	CYCLES(5,5,5)
	b		eaderef

entry static eaeaud
	GET_U(rTempSave)
	subi	rTempSave,rTempSave,1
	SET_U(rTempSave)
	SAVE_SU
	CYCLES(5,5,5)
	b		eaderef

entry static eaeasd
	GET_S(rTempSave)
	subi	rTempSave,rTempSave,1
	SET_S(rTempSave)
	SAVE_SU
	CYCLES(5,5,5)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapcd
	GET_PC(rTempSave)
	subi	rTempSave,rTempSave,1
	SET_PC(rTempSave)
	SAVE_PCAB
	CYCLES(5,5,5)
	b		eaderef
#endif

	//
	//		(--X/Y/U/S/PC)
	//
entry static eaeaxdd
	GET_X(rTempSave)
	subi	rTempSave,rTempSave,2
	SET_X(rTempSave)
	SAVE_XY
	CYCLES(6,6,6)
	b		eaderef

entry static eaeaydd
	GET_Y(rTempSave)
	subi	rTempSave,rTempSave,2
	SET_Y(rTempSave)
	SAVE_XY
	CYCLES(6,6,6)
	b		eaderef

entry static eaeaudd
	GET_U(rTempSave)
	subi	rTempSave,rTempSave,2
	SET_U(rTempSave)
	SAVE_SU
	CYCLES(6,6,6)
	b		eaderef

entry static eaeasdd
	GET_S(rTempSave)
	subi	rTempSave,rTempSave,2
	SET_S(rTempSave)
	SAVE_SU
	CYCLES(6,6,6)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapcdd
	GET_PC(rTempSave)
	subi	rTempSave,rTempSave,2
	SET_PC(rTempSave)
	SAVE_PCAB
	CYCLES(6,6,6)
	b		eaderef
#endif

	//
	//		(X/Y/U/S/PC)
	//
entry static eaeax
	GET_X(rTempSave)
	CYCLES(3,3,3)
	b		eaderef

entry static eaeay
	GET_Y(rTempSave)
	CYCLES(3,3,3)
	b		eaderef

entry static eaeau
	GET_U(rTempSave)
	CYCLES(3,3,3)
	b		eaderef

entry static eaeas
	GET_S(rTempSave)
	CYCLES(3,3,3)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapc
	GET_PC(rTempSave)
	CYCLES(3,3,3)
	b		eaderef
#endif

	//
	//		(X/Y/U/S/PC + B)
	//
entry static eaeaxpb
	GET_B(r3)
	extsb	r3,r3
	GET_X(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaypb
	GET_B(r3)
	extsb	r3,r3
	GET_Y(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaupb
	GET_B(r3)
	extsb	r3,r3
	GET_U(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaspb
	GET_B(r3)
	extsb	r3,r3
	GET_S(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapcpb
	GET_B(r3)
	extsb	r3,r3
	GET_PC(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef
#endif

	//
	//		(X/Y/U/S/PC + A)
	//
entry static eaeaxpa
	GET_A(r3)
	extsb	r3,r3
	GET_X(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaypa
	GET_A(r3)
	extsb	r3,r3
	GET_Y(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaupa
	GET_A(r3)
	extsb	r3,r3
	GET_U(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaspa
	GET_A(r3)
	extsb	r3,r3
	GET_S(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapcpa
	GET_A(r3)
	extsb	r3,r3
	GET_PC(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef
#endif

	//
	//		(X/Y/U/S/PC + 8 bit offset)
	//
entry static eaeaxp8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_X(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeayp8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_Y(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeaup8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_U(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeasp8
	READ_OPCODE_ARG(r3)
	extsb	r3,r3
	GET_S(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

entry static eaeapcp8
#if (A6809_CHIP != 5000)
	READ_OPCODE_ARG(r3)
	GET_PC(rTempSave)
#else
	GET_PC(rTempSave)
	READ_OPCODE_ARG(r3)
#endif
	extsb	r3,r3
	add		rTempSave,rTempSave,r3
	CYCLES(4,4,4)
	b		eaderef

	//
	//		(X/Y/U/S/PC + 16 bit offset)
	//
entry static eaeaxp16
	READ_OPCODE_ARG_WORD(r3)
	GET_X(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeayp16
	READ_OPCODE_ARG_WORD(r3)
	GET_Y(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeaup16
	READ_OPCODE_ARG_WORD(r3)
	GET_U(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeasp16
	READ_OPCODE_ARG_WORD(r3)
	GET_S(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeapcp16
#if (A6809_CHIP != 5000)
	READ_OPCODE_ARG_WORD(r3)
	GET_PC(rTempSave)
#else
	GET_PC(rTempSave)
	READ_OPCODE_ARG_WORD(r3)
#endif
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

	//
	//		(X/Y/U/S/PC + D)
	//
entry static eaeaxpd
	GET_D(r3)
	GET_X(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeaypd
	GET_D(r3)
	GET_Y(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeaupd
	GET_D(r3)
	GET_U(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

entry static eaeaspd
	GET_D(r3)
	GET_S(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef

#if (A6809_CHIP == 5000)
entry static eaeapcpd
	GET_D(r3)
	GET_PC(rTempSave)
	add		rTempSave,rTempSave,r3
	CYCLES(7,7,7)
	b		eaderef
#endif
	
	//
	//		(16 bit absolute)
	//
entry static eaea16
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(8,8,8)
	b		eaderef
}
