//###################################################################################################
//
//
//		Asgard6800Core.c
//		See Asgard6800DefaultConfig.h for compile-time configuration and optimization.
//
//		A PowerPC assembly Motorola 6800-family emulation core written by Aaron Giles.
//		This code is free for use in any emulation project as long as the following credit 
//		appears in the about box and documentation:
//
//			PowerPC-optimized 6800 emulation provided by Aaron Giles and the MAME project.
//
//		I would also appreciate a free copy of any project making use of this code.  Please take
//		the time to contact me if you are using this code or if you have any bugs to report;
//		It is quite possible that I have a newer version.  My email address is aaron@aarongiles.com
//
//		This file looks best when viewed with a tab size of 4 characters
//
//		Warning: due to a bug in CWPro 2, you will need CWPro 3 or later to compile this
//
//
//###################################################################################################
//
//
//		Revision history:
//
//		??/??/??	1.0		First MAME version
//		03/15/99	2.0		Reformatted and rewrote to handle interrupts immediately
//							Changed the interface significantly
//							Renamed a bunch of internal variables to be more Mac-like
//							Moved the debugger to its own file
//		03/21/03	2.1		LBO - added adcx_im for the 8105 (used in Tube Panic)
//		07/07/03	2.2		LBO - added Mach-O support
//
//
//###################################################################################################


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Asgard6800Core.h"
#include "Asgard6800Internals.h"

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
	kInterruptStateWAI	  		= 0x01,		// set when WAI is waiting for an interrupt

	// these flags allow interrupts to generated from within an I/O callback
	kExitActionGenerateNMI		= 0x01,
	kExitActionGenerateIRQ		= 0x02
};


//###################################################################################################
//	TYPE AND STRUCTURE DEFINITIONS
//###################################################################################################

typedef struct
{
	unsigned long 			fSX;						// S and X registers
	unsigned long 			fPCAB;						// PC, A, and B registers
	unsigned char 			fFlags;						// CC and internal flags

	unsigned char 			fNMIState;					// current state of the NMI line
	unsigned char 			fIRQState;					// current state of the IRQ line
	unsigned char 			fInterruptState;			// current state of the internal SYNC/CWAI flags
	Asgard6800IRQCallback 	fIRQCallback;				// callback routine for IRQ lines
	signed long				fInterruptCycleAdjust;		// cycle count adjustment due to interrupts

#if INTERNAL_REGISTERS
	unsigned char 			fTINState;					// current state of the TIN line
	unsigned char 			fInputCaptureEdge;			// input capture edge, 0 = falling, 1 = rising
	unsigned char			fPort1DDR;					// data direction register for port 1
	unsigned char			fPort2DDR;					// data direction register for port 2
	unsigned char			fPort1Data;					// current data for port 1
	unsigned char			fPort2Data;					// current data for port 2
	unsigned char			fTCSR;						// TCSR register
	unsigned char			fTCSRPending;				// TCSR pending state
	unsigned char			fIRQ2Flags;					// IRQ2 flags
	unsigned char			fRAMControl;				// RAM control register
	unsigned short			fCounter;					// current counter value
	unsigned short			fCounterLatch;				// latched upper byte of counter
	unsigned short			fOutputCompare;				// current output compare value
	unsigned short			fInputCapture;				// current input capture value
	signed long				fOutputCompareLeft;			// remaining count until output compare
#endif

} Asgard6800Context;
	

//###################################################################################################
//	GLOBAL VARIABLES
//###################################################################################################

// externally defined variables
extern unsigned char *			A6800_OPCODEROM;		// pointer to the ROM base
extern unsigned char *			A6800_ARGUMENTROM;		// pointer to the argument ROM base
extern int						A6800_ICOUNT;			// cycles remaining to execute

// other stuff
static signed long 				sRequestedCycles;		// the originally requested number of cycles
static signed long 				sExitActionCycles;		// number of cycles removed to force an exit action
static unsigned char			sExitActions;			// current exit actions
static unsigned char			sExecuting;				// true if we're currently executing

// local variables containing the 6800 registers
static unsigned long 			sSX;					// S and X registers
static unsigned long 			sPCAB;					// PC, A, and B registers
static unsigned long 			sFlags;					// CC and internal flags
static unsigned long 			sOpcodePC;				// contains the PC of the current opcode

// local variables describing the current 6800 interrupt state
static unsigned char 			sNMIState;				// current state of the NMI line
static unsigned char 			sIRQState;				// current state of the IRQ line
static unsigned char 			sTINState;				// current state of the TIN line
static unsigned char 			sInputCaptureEdge;		// input capture edge, 0 = falling, 1 = rising
static unsigned char 			sInterruptState;		// current state of the internal SYNC/CWAI flags
static Asgard6800IRQCallback 	sIRQCallback;			// callback routine for IRQ lines
static signed long 				sInterruptCycleAdjust;	// cycle count adjustment due to interrupts

// local variables describing the current internal registers
#if INTERNAL_REGISTERS
static unsigned char			sPort1DDR;				// data direction register for port 1
static unsigned char			sPort2DDR;				// data direction register for port 2
static unsigned char			sPort1Data;				// current data for port 1
static unsigned char			sPort2Data;				// current data for port 2
static unsigned char			sTCSR;					// TCSR register
static unsigned char			sTCSRPending;			// TCSR pending state
static unsigned char			sIRQ2Flags;				// IRQ2 flags
static unsigned char			sRAMControl;			// RAM control register
static unsigned short			sCounter;				// current counter value
static unsigned short			sCounterLatch;			// latched upper byte of counter
static unsigned short			sOutputCompare;			// current output compare value
static unsigned short			sInputCapture;			// current input capture value
static signed long				sOutputCompareLeft;		// remaining count until output compare
#endif


//###################################################################################################
//	FUNCTION TABLES
//###################################################################################################

#if (A6800_CHIP == 6800 || A6800_CHIP == 6802 || A6800_CHIP == 6808)
	#define jsr_di  illegal
	#define brn     illegal
	#define adcx_im	illegal
#endif

#if (A6800_CHIP == 6800 || A6800_CHIP == 6802 || A6800_CHIP == 6808 || A6800_CHIP == 8105)
	#define lsrd    illegal
	#define asld    illegal
	#define pulx    illegal
	#define abx     illegal
	#define pshx    illegal
	#define mul     illegal
	#define addd_im illegal
	#define addd_di illegal
	#define addd_ix illegal
	#define addd_ex illegal
	#define subd_im illegal
	#define subd_di illegal
	#define subd_ix illegal
	#define subd_ex illegal
	#define ldd_im  illegal
	#define ldd_di  illegal
	#define ldd_ix  illegal
	#define ldd_ex  illegal
	#define std_im  illegal
	#define std_di  illegal
	#define std_ix  illegal
	#define std_ex  illegal
#endif

#if (A6800_CHIP != 63701)
	#define xgdx    illegal
	#define aim_ix  illegal
	#define oim_ix  illegal
	#define eim_ix  illegal
	#define tim_ix  illegal
	#define aim_di  illegal
	#define oim_di  illegal
	#define eim_di  illegal
	#define tim_di  illegal
	#define undoc1  illegal
	#define undoc2  illegal
#endif

#if (A6800_CHIP != 8105)
static void *sOpcodeTable[0x0100] =
{
	illegal, 	nop,		illegal, 	illegal, 	lsrd, 		asld,		tap,		tpa,		/*00*/
	inx,		dex,		clv,		sev,		clc,		sec,		cli,		sti,
	sba,		cba,		undoc1,		undoc2,		illegal, 	illegal, 	tab,		tba,		/*10*/
	xgdx,		daa,		illegal, 	aba,		illegal, 	illegal, 	illegal, 	illegal, 	
	bra,		brn,		bhi,		bls,		bcc,		bcs,		bne,		beq,		/*20*/
	bvc,		bvs,		bpl,		bmi,		bge,		blt,		bgt,		ble,
	tsx,		ins,		pula,		pulb,		des,		txs,		psha,		pshb,		/*30*/
	pulx,		rts,		abx,		rti,		pshx, 		mul,		wai,		swi,
	nega,		illegal, 	illegal, 	coma,		lsra,		illegal, 	rora,		asra,		/*40*/
	asla,		rola,		deca,		illegal, 	inca,		tsta,		illegal, 	clra,
	negb,		illegal, 	illegal, 	comb,		lsrb,		illegal, 	rorb,		asrb,		/*50*/
	aslb,		rolb,		decb,		illegal, 	incb,		tstb,		illegal, 	clrb,
	neg_ix,		aim_ix, 	oim_ix,	 	com_ix,		lsr_ix,		eim_ix,	 	ror_ix,		asr_ix,		/*60*/
	asl_ix,		rol_ix,		dec_ix,		tim_ix,	 	inc_ix,		tst_ix,		jmp_ix,		clr_ix,
	neg_ex,		aim_di,	 	oim_di,	 	com_ex,		lsr_ex,		eim_di,	 	ror_ex,		asr_ex,		/*70*/
	asl_ex,		rol_ex,		dec_ex,		tim_di,	 	inc_ex,		tst_ex,		jmp_ex,		clr_ex,
	suba_im,	cmpa_im,	sbca_im,	subd_im, 	anda_im,	bita_im,	lda_im,		illegal,	/*80*/
	eora_im,	adca_im,	ora_im,		adda_im,	cmpx_im,	bsr,		lds_im,		illegal,
	suba_di,	cmpa_di,	sbca_di,	subd_di, 	anda_di,	bita_di,	lda_di,		sta_di,		/*90*/
	eora_di,	adca_di,	ora_di,		adda_di,	cmpx_di,	jsr_di,		lds_di,		sts_di,
	suba_ix,	cmpa_ix,	sbca_ix,	subd_ix, 	anda_ix,	bita_ix,	lda_ix,		sta_ix,		/*A0*/
	eora_ix,	adca_ix,	ora_ix,		adda_ix,	cmpx_ix,	jsr_ix,		lds_ix,		sts_ix,
	suba_ex,	cmpa_ex,	sbca_ex,	subd_ex, 	anda_ex,	bita_ex,	lda_ex,		sta_ex,		/*B0*/
	eora_ex,	adca_ex,	ora_ex,		adda_ex,	cmpx_ex,	jsr_ex,		lds_ex,		sts_ex,
	subb_im,	cmpb_im,	sbcb_im,	addd_im,	andb_im,	bitb_im,	ldb_im,		illegal,	/*C0*/
	eorb_im,	adcb_im,	orb_im,		addb_im,	ldd_im,		illegal,	ldx_im,		illegal,
	subb_di,	cmpb_di,	sbcb_di,	addd_di,	andb_di,	bitb_di,	ldb_di,		stb_di,		/*D0*/
	eorb_di,	adcb_di,	orb_di,		addb_di,	ldd_di,		std_di,		ldx_di,		stx_di,
	subb_ix,	cmpb_ix,	sbcb_ix,	addd_ix,	andb_ix,	bitb_ix,	ldb_ix,		stb_ix,		/*E0*/
	eorb_ix,	adcb_ix,	orb_ix,		addb_ix,	ldd_ix,		std_ix,		ldx_ix,		stx_ix,
	subb_ex,	cmpb_ex,	sbcb_ex,	addd_ex,	andb_ex,	bitb_ex,	ldb_ex,		stb_ex,		/*F0*/
	eorb_ex,	adcb_ex,	orb_ex,		addb_ex,	ldd_ex,		std_ex,		ldx_ex,		stx_ex
};
#else
static void *sOpcodeTable[0x0100] =
{
	illegal, 	illegal, 	nop,		illegal, 	tap,		illegal,	tpa,		inx,		/*00*/
	inx,		clv,		dex,		sev,		clc,		cli,		sec,		sti,
	sba,		illegal,	cba,		illegal,	illegal,	tab,		illegal,	tba,		/*10*/
	illegal,	illegal,	daa,		aba,		illegal,	illegal,	illegal,	illegal,	
	bra,		bhi,		brn,		bls,		bcc,		bne,		bcs,		beq,		/*20*/
	bvc,		bpl,		bvs,		bmi,		bge,		bgt,		blt,		ble,
	tsx,		pula,		ins,		pulb,		des,		psha,		txs,		pshb,		/*30*/
	illegal,	illegal,	rts,		rti,		illegal,	wai,		illegal,	swi,
	suba_im,	sbca_im,	cmpa_im,	illegal,	anda_im,	lda_im,		bita_im,	illegal,	/*40*/
	eora_im,	ora_im,		adca_im,	adda_im,	cmpx_im,	lds_im,		bsr,		illegal,
	suba_di,	sbca_di,	cmpa_di,	illegal,	anda_di,	lda_di,		bita_di,	sta_di,		/*50*/
	eora_di,	ora_di,		adca_di,	adda_di,	cmpx_di,	lds_di,		jsr_di,		sts_di,
	suba_ix,	sbca_ix,	cmpa_ix,	illegal,	anda_ix,	lda_ix,		bita_ix,	sta_ix,		/*60*/
	eora_ix,	ora_ix,		adca_ix,	adda_ix,	cmpx_ix,	lds_ix,		jsr_ix,		sts_ix,
	suba_ex,	sbca_ex,	cmpa_ex,	illegal,	anda_ex,	lda_ex,		bita_ex,	sta_ex,		/*70*/
	eora_ex,	ora_ex,		adca_ex,	adda_ex,	cmpx_ex,	lds_ex,		jsr_ex,		sts_ex,
	nega,		illegal,	illegal,	coma,		lsra,		rora,		illegal,	asra,		/*80*/
	asla,		deca,		rola,		illegal,	inca,		illegal,	tsta,		clra,
	negb,		illegal,	illegal,	comb,		lsrb,		rorb,		illegal,	asrb,		/*90*/
	aslb,		decb,		rolb,		illegal,	incb,		illegal,	tstb,		clrb,
	neg_ix,		illegal,	illegal,	com_ix,		lsr_ix,		ror_ix,		illegal,	asr_ix,		/*A0*/
	asl_ix,		dec_ix,		rol_ix,		illegal,	inc_ix,		jmp_ix,		tst_ix,		clr_ix,
	neg_ex,		illegal,	illegal,	com_ex,		lsr_ex,		ror_ex,		illegal,	asr_ex,		/*B0*/
	asl_ex,		dec_ex,		rol_ex,		illegal,	inc_ex,		jmp_ex,		tst_ex,		clr_ex,
	subb_im,	sbcb_im,	cmpb_im,	illegal,	andb_im,	ldb_im,		bitb_im,	illegal,	/*C0*/
	eorb_im,	orb_im,		adcb_im,	addb_im,	illegal,	ldx_im,		illegal,	illegal,
	subb_di,	sbcb_di,	cmpb_di,	illegal,	andb_di,	ldb_di,		bitb_di,	stb_di,		/*D0*/
	eorb_di,	orb_di,		adcb_di,	addb_di,	illegal,	ldx_di,		illegal,	stx_di,
	subb_ix,	sbcb_ix,	cmpb_ix,	illegal,	andb_ix,	ldb_ix,		bitb_ix,	stb_ix,		/*E0*/
//	eorb_ix,	orb_ix,		adcb_ix,	addb_ix,	illegal,	ldx_ix,		illegal,	stx_ix,
	eorb_ix,	orb_ix,		adcb_ix,	addb_ix,	adcx_im,	ldx_ix,		illegal,	stx_ix,
	subb_ex,	sbcb_ex,	cmpb_ex,	illegal,	andb_ex,	ldb_ex,		bitb_ex,	stb_ex,		/*F0*/
	eorb_ex,	orb_ex,		adcb_ex,	addb_ex,	addx_ex,	ldx_ex,		illegal,	stx_ex
};
#endif

//###################################################################################################
//	INLINE FUNCTIONS
//###################################################################################################

//###################################################################################################
//
//	PushByte -- push a byte onto the stack
//
//###################################################################################################

static inline void PushByte(unsigned char inValue)
{
	WRITEMEM(sSX >> 16, inValue);
	sSX -= 0x00010000;
}


//###################################################################################################
//
//	PushByte -- push a word onto the stack
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

static inline void ProcessInterrupt(int inInhibitWhich, int inPCOffset)
{
	// set the opcode PC to -1 during interrupts
	sOpcodePC = -1;

	// if the state has already been pushed by CWAI, just clear the CWAI condition
	if (sInterruptState & kInterruptStateWAI)
		sInterruptState &= ~kInterruptStateWAI;
		
	// otherwise, if we're pushing the entire state, set the E bit and push it all
	else
	{
		PushWord(sPCAB >> 16);
		PushWord(sSX & 0xffff);
		PushByte((sPCAB >> 8) & 0xff);
		PushByte(sPCAB & 0xff);
		PushByte(sFlags & 0xff);

		// count 8 additional cycles to account for this extra pushing
		sInterruptCycleAdjust += 8;
	}

	// inhibit the requested interrupts and mark the flags dirty
	sFlags |= inInhibitWhich | kFlagsDirty;
	
	// set the PC to the requested vector
	sPCAB = (READMEM(inPCOffset) << 24) + (READMEM(inPCOffset + 1) << 16) + (sPCAB & 0xffff);
#ifdef A6800_UPDATEBANK
	A6800_UPDATEBANK(sPCAB >> 16);
#endif

	// count 4 cycles to process the interrupt
	sInterruptCycleAdjust += 4;
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

static void InitTables (void)
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
//	ProcessNMI -- generates an NMI interrupt
//
//###################################################################################################

static void ProcessNMI(void)
{
	ProcessInterrupt(k6800FlagI, 0xfffc);
}


//###################################################################################################
//
//	ProcessIRQ -- generates an IRQ interrupt
//
//###################################################################################################

static void ProcessIRQ(void)
{
	ProcessInterrupt(k6800FlagI, 0xfff8);
	if (sIRQCallback)
		(*sIRQCallback)(k6800IRQLineIRQ);
}


#if INTERNAL_REGISTERS

//###################################################################################################
//
//	ProcessICI -- generates an ICI interrupt
//
//###################################################################################################

static void ProcessICI(void)
{
	ProcessInterrupt(k6800FlagI, 0xfff6);
	if (sIRQCallback)
		(*sIRQCallback)(k6800IRQLineTIN);
}


//###################################################################################################
//
//	ProcessOCI -- generates an OCI interrupt
//
//###################################################################################################

static void ProcessOCI(void)
{
	ProcessInterrupt(k6800FlagI, 0xfff4);
}


//###################################################################################################
//
//	ProcessTOI -- generates an TOI interrupt
//
//###################################################################################################

static void ProcessTOI(void)
{
	ProcessInterrupt(k6800FlagI, 0xfff2);
}

#endif


//###################################################################################################
//
//	ProcessSWI -- generates a software interrupt
//
//###################################################################################################

static void ProcessSWI(void)
{
	ProcessInterrupt(k6800FlagI, 0xfffa);
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
	// see if the IRQ flag is enabled
	if ((sFlags & k6800FlagI) == 0)
	{
		// if the IRQ line is asserted, process the interrupt and call the callback
		if (sIRQState != k6800IRQStateClear)
		{
			// if we're inside the execution loop, just set the state bit
			if (!inExecutingOkay && sExecuting)
			{
				sExitActions |= kExitActionGenerateIRQ;
				sExitActionCycles += A6800_ICOUNT;
				A6800_ICOUNT = 0;
			}
			
			// otherwise, process it right away
			else
				ProcessIRQ();
		}

#if INTERNAL_REGISTERS
		// if any of the IRQ2 flags are set, handle their interrupts
		else if (sIRQ2Flags & (k6800TCSRStateICF | k6800TCSRStateOCF | k6800TCSRStateTOF))
		{
			// check ICF
			if (sIRQ2Flags & k6800TCSRStateICF)
				ProcessICI();
			
			// check OCF
			if (sIRQ2Flags & k6800TCSRStateOCF)
				ProcessOCI();
			
			// check TOF
			if (sIRQ2Flags & k6800TCSRStateTOF)
				ProcessTOI();
		}
#endif
	}
}


#pragma mark -
#pragma mark ¥ INTERNAL REGISTERS

#if INTERNAL_REGISTERS

//###################################################################################################
//
//	UpdateIRQ2 -- Updates the internal state of the IRQ2 flags
//
//###################################################################################################

static inline void UpdateIRQ2(void)
{
	sIRQ2Flags = sTCSR & (sTCSR << 3);
}


//###################################################################################################
//
//	UpdateOutputCompareLeft -- Updates the internal state of the output compare left counter
//
//###################################################################################################

static inline void UpdateOutputCompareLeft(void)
{
	sOutputCompareLeft = (signed long)sOutputCompare - sCounter;
	if (sOutputCompareLeft <= 0)
		sOutputCompareLeft += 0x10000;
}


//###################################################################################################
//
//	Asgard6800InternalRead -- handles a read from an internal register
//
//	This function can be called by the client read handler to access the internal registers on the
//	CPUs that support them.
//
//###################################################################################################

UINT8 Asgard6800InternalRead(offs_t inOffset)
{
	// only some offsets are supported
	switch (inOffset)
	{
		case 0x00:
			return sPort1DDR;
			
		case 0x01:
			return sPort2DDR;
			
		case 0x02:
			return (READPORT(k6800PortData1) & (sPort1DDR ^ 0xff)) | (sPort1Data & sPort1DDR);
			
		case 0x03:
			return (READPORT(k6800PortData2) & (sPort2DDR ^ 0xff)) | (sPort2Data & sPort2DDR);
			
		case 0x08:
			sTCSRPending = 0;
			return sTCSR;
			
		case 0x09:
			if (!(sTCSRPending & k6800TCSRStateTOF))
			{
				sTCSR &= ~k6800TCSRStateTOF;
				UpdateIRQ2();
			}
			return (sCounter >> 8) & 0xff;
			
		case 0x0a:
			return sCounter & 0xff;
			
		case 0x0b:
			if (!(sTCSRPending & k6800TCSRStateOCF))
			{
				sTCSR &= ~k6800TCSRStateOCF;
				UpdateIRQ2();
			}
			return (sOutputCompare >> 8) & 0xff;
			
		case 0x0c:
			if (!(sTCSRPending & k6800TCSRStateOCF))
			{
				sTCSR &= ~k6800TCSRStateOCF;
				UpdateIRQ2();
			}
			return sOutputCompare & 0xff;
			
		case 0x0d:
			if (!(sTCSRPending & k6800TCSRStateICF))
			{
				sTCSR &= ~k6800TCSRStateICF;
				UpdateIRQ2();
			}
			return (sInputCapture >> 8) & 0xff;
			
		case 0x0e:
			return sInputCapture & 0xff;
			
		case 0x14:
			return sRAMControl;
		
		default:
			return 0;
	}
}


//###################################################################################################
//
//	Asgard6800InternalWrite -- handles a write to an internal register
//
//	This function can be called by the client write handler to access the internal registers on the
//	CPUs that support them.
//
//###################################################################################################

void Asgard6800InternalWrite(offs_t inOffset, UINT8 inData)
{
	switch (inOffset)
	{
		case 0x00:
			if (sPort1DDR != inData)
			{
				sPort1DDR = inData;
				if (sPort1DDR == 0xff)
					WRITEPORT(k6800PortData1, sPort1Data);
				else
					WRITEPORT(k6800PortData1, (sPort1Data & sPort1DDR)
						| (READPORT(k6800PortData1) & (sPort1DDR ^ 0xff)));
			}
			break;
			
		case 0x01:
			if (sPort2DDR != inData)
			{
				sPort2DDR = inData;
				if (sPort2DDR == 0xff)
					WRITEPORT(k6800PortData2, sPort2Data);
				else
					WRITEPORT(k6800PortData2, (sPort2Data & sPort2DDR)
						| (READPORT(k6800PortData2) & (sPort2DDR ^ 0xff)));
			}
			break;
		
		case 0x02:
			sPort1Data = inData;
			if (sPort1DDR == 0xff)
				WRITEPORT(k6800PortData1, sPort1Data);
			else
				WRITEPORT(k6800PortData1, (sPort1Data & sPort1DDR)
					| (READPORT(k6800PortData1) & (sPort1DDR ^ 0xff)));
			break;
				
		case 0x03:
			sPort2Data = inData;
			if (sPort2DDR == 0xff)
				WRITEPORT(k6800PortData2, sPort2Data);
			else
				WRITEPORT(k6800PortData2, (sPort2Data & sPort2DDR)
					| (READPORT(k6800PortData2) & (sPort2DDR ^ 0xff)));
			break;
		
		case 0x08:
			sTCSR = inData;
			sTCSRPending &= inData;
			UpdateIRQ2();
			break;
			
		case 0x09:
			sFlags |= kFlagsDirty;
			sCounterLatch = inData & 0xff;
			sCounter = 0xfff8;
			break;

		case 0x0a:
			sFlags |= kFlagsDirty;
			sCounter = (sCounterLatch << 8) | inData;
			break;
			
		case 0x0b:
			sFlags |= kFlagsDirty;
			sOutputCompare = (sOutputCompare & 0x00ff) | ((inData << 8) & 0xff00);
			UpdateOutputCompareLeft();
			break;
			
		case 0x0c:
			sFlags |= kFlagsDirty;
			sOutputCompare = (sOutputCompare & 0xff00) | (inData & 0x00ff);
			UpdateOutputCompareLeft();
			break;
			
		case 0x14:
			sRAMControl = inData;
			break;
	}
}

#endif


#pragma mark -
#pragma mark ¥ CORE IMPLEMENTATION

//###################################################################################################
//
//	Asgard6800Init -- init the 6800 state variables
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state
//
//###################################################################################################

void Asgard6800Init(void)
{
	int cpu = cpu_getactivecpu();
	state_save_register_UINT32("m680xppc", cpu, "PCAB", &sPCAB, 1);
	state_save_register_UINT32("m680xppc", cpu, "Flags", &sFlags, 1);
	state_save_register_UINT32("m680xppc", cpu, "SX", &sSX, 1);
	state_save_register_INT8("m680xppc", cpu, "NMI", &sNMIState, 1);
	state_save_register_INT8("m680xppc", cpu, "IRQ", &sIRQState, 1);
	state_save_register_UINT8("m680xppc", cpu, "Int", &sInterruptState, 1);
	state_save_register_INT32("m680xppc", cpu, "IntCycleAdjust", &sInterruptCycleAdjust, 1);
#if INTERNAL_REGISTERS
	// ¥¥¥ÊTODO.
#endif
}

//###################################################################################################
//
//	Asgard6800Reset -- reset the 6800 processor
//
//	This function must be called before starting the emulation, in order to initialize
//	the processor state and create the tables
//
//###################################################################################################

void Asgard6800Reset(void)
{
	// according to the 6800 datasheet, the only things changed on a reset
	// is setting the I flag
	sFlags |= k6800FlagI;
	
	// initialize the PC
	sPCAB = (READMEM(0xfffe) << 24) + (READMEM(0xffff) << 16);
#ifdef A6800_UPDATEBANK
	A6800_UPDATEBANK(sPCAB >> 16);
#endif

	// reset the internal interrupt state
	sNMIState = k6800IRQStateClear;
	sIRQState = k6800IRQStateClear;
	sInterruptState = 0;
	sInterruptCycleAdjust = 0;

	// reset the internal registers
#if INTERNAL_REGISTERS
	sTINState = k6800IRQStateClear;
	sInputCaptureEdge = 0;
	sPort1DDR = sPort2DDR = 0x00;
	sTCSR = sTCSRPending = 0x00;
	sIRQ2Flags = 0;
	sCounter = sCounterLatch = 0x0000;
	sOutputCompare = sOutputCompareLeft = 0xffff;
	sRAMControl |= 0x40;
#endif

	// make sure our tables have been created
	InitTables();
}


//###################################################################################################
//
//	Asgard6800SetContext -- set the contents of the 6800 registers
//
//	This function can unfortunately be called at any time to change the contents of the
//	6800 registers.  Call Asgard6800GetContext to get the original values before changing them.
//
//###################################################################################################

void Asgard6800SetContext(void *inContext)
{
	if (inContext)
	{
		Asgard6800Context *context = inContext;

		sSX = context->fSX;
		sPCAB = context->fPCAB;
		sFlags = context->fFlags | kFlagsDirty;

		sNMIState = context->fNMIState;
		sIRQState = context->fIRQState;
		sInterruptState = context->fInterruptState;
		sIRQCallback = context->fIRQCallback;
		sInterruptCycleAdjust = context->fInterruptCycleAdjust;
		
#if INTERNAL_REGISTERS
		sTINState = context->fTINState;
		sInputCaptureEdge = context->fInputCaptureEdge;
		sPort1DDR = context->fPort1DDR;
		sPort2DDR = context->fPort2DDR;
		sPort1Data = context->fPort1Data;
		sPort2Data = context->fPort2Data;
		sTCSR = context->fTCSR;
		sTCSRPending = context->fTCSRPending;
		sIRQ2Flags = context->fIRQ2Flags;
		sRAMControl = context->fRAMControl;
		sCounter = context->fCounter;
		sCounterLatch = context->fCounterLatch;
		sOutputCompare = context->fOutputCompare;
		sInputCapture = context->fInputCapture;
		sOutputCompareLeft = context->fOutputCompareLeft;
#endif

		CheckIRQLines(false);

#ifdef A6800_UPDATEBANK
		A6800_UPDATEBANK(sPCAB >> 16);
#endif
	}
}


//###################################################################################################
//
//	Asgard6800SetReg -- set the contents of one 6800 register
//
//	This function can unfortunately be called at any time to change the contents of a
//	6800 register.
//
//###################################################################################################

void Asgard6800SetReg(int inRegisterIndex, unsigned int inValue)
{
	sFlags |= kFlagsDirty;
	
	switch (inRegisterIndex)
	{
		case k6800RegisterIndexA:
			sPCAB = (sPCAB & 0xffff00ff) | ((inValue & 0xff) << 8);
			break;
		
		case k6800RegisterIndexB:
			sPCAB = (sPCAB & 0xffffff00) | (inValue & 0xff);
			break;
		
		case REG_PC:
		case k6800RegisterIndexPC:
			sPCAB = (sPCAB & 0x0000ffff) | ((inValue & 0xffff) << 16);

#ifdef A6800_UPDATEBANK
			A6800_UPDATEBANK(sPCAB >> 16);
#endif
			break;
		
		case REG_SP:
		case k6800RegisterIndexS:
			sSX = (sSX & 0x0000ffff) | ((inValue & 0xffff) << 16);
			break;
		
		case k6800RegisterIndexX:
			sSX = (sSX & 0xffff0000) | (inValue & 0xffff);
			break;

		case k6800RegisterIndexCC:
			sFlags = (sFlags & 0xffffff00) | (inValue & 0xff);
			CheckIRQLines(false);
			break;
		
		case k6800RegisterIndexWAIState:
			sInterruptState = (sInterruptState & ~kInterruptStateWAI) | ((inValue & kInterruptStateWAI) != 0);
			break;
		
		case k6800RegisterIndexNMIState:
			sNMIState = inValue;
			break;

		case k6800RegisterIndexIRQState:
			sIRQState = inValue;
			break;

		case k6800RegisterIndexOpcodePC:
			sOpcodePC = inValue;
			break;
	}
}


//###################################################################################################
//
//	Asgard6800GetContext -- examine the contents of the 6800 registers
//
//	This function can unfortunately be called at any time to examine the contents of the
//	6800 registers.
//
//###################################################################################################

void Asgard6800GetContext(void *outContext)
{
	if (outContext)
	{
		Asgard6800Context *context = outContext;

		context->fSX = sSX;
		context->fPCAB = sPCAB;
		context->fFlags = sFlags;

		context->fNMIState = sNMIState;
		context->fIRQState = sIRQState;
		context->fInterruptState = sInterruptState;
		context->fIRQCallback = sIRQCallback;
		context->fInterruptCycleAdjust = sInterruptCycleAdjust;

#if INTERNAL_REGISTERS
		context->fTINState = sTINState;
		context->fInputCaptureEdge = sInputCaptureEdge;
		context->fPort1DDR = sPort1DDR;
		context->fPort2DDR = sPort2DDR;
		context->fPort1Data = sPort1Data;
		context->fPort2Data = sPort2Data;
		context->fTCSR = sTCSR;
		context->fTCSRPending = sTCSRPending;
		context->fIRQ2Flags = sIRQ2Flags;
		context->fRAMControl = sRAMControl;
		context->fCounter = sCounter;
		context->fCounterLatch = sCounterLatch;
		context->fOutputCompare = sOutputCompare;
		context->fInputCapture = sInputCapture;
		context->fOutputCompareLeft = sOutputCompareLeft;
#endif
	}
//	return sizeof(Asgard6800Context);
}


//###################################################################################################
//
//	Asgard6800GetReg -- get the contents of one 6800 register
//
//	This function can unfortunately be called at any time to return the contents of a
//	6800 register.
//
//###################################################################################################

unsigned int Asgard6800GetReg(int inRegisterIndex)
{
	switch (inRegisterIndex)
	{
		case k6800RegisterIndexA:
			return (sPCAB >> 8) & 0xff;
		
		case k6800RegisterIndexB:
			return sPCAB & 0xff;
		
		case REG_PC:
		case k6800RegisterIndexPC:
			return (sPCAB >> 16) & 0xffff;
		
		case REG_SP:
		case k6800RegisterIndexS:
			return (sSX >> 16) & 0xffff;
		
		case k6800RegisterIndexX:
			return sSX & 0xffff;

		case k6800RegisterIndexCC:
			return sFlags & 0xff;

		case k6800RegisterIndexWAIState:
			return (sInterruptState & kInterruptStateWAI);
		
		case k6800RegisterIndexNMIState:
			return sNMIState;

		case k6800RegisterIndexIRQState:
			return sIRQState;

		case k6800RegisterIndexTINState:
			return sTINState;

		case k6800RegisterIndexOpcodePC:
			return sOpcodePC;
	}
	
	return 0;
}


//###################################################################################################
//
//	Asgard6800SetIRQLine -- sets the state of the IRQ/FIRQ lines
//
//###################################################################################################

void Asgard6800SetIRQLine(int inIRQLine, int inState)
{
	if (inIRQLine == INPUT_LINE_NMI)
	{
		// if the state is the same as last time, bail
		if (sNMIState == inState) 
			return;
		sNMIState = inState;

		// detect when the state goes non-clear
		if (inState != k6800IRQStateClear)
		{
			// if we're inside the execution loop, just set the state bit and force us to exit
			if (sExecuting)
			{
				sExitActions |= kExitActionGenerateNMI;
				sExitActionCycles += A6800_ICOUNT;
				A6800_ICOUNT = 0;
			}
		
			// else process it right away
			else
				ProcessNMI();
		}
		return;
	}

	// set the appropriate state for the IRQ line
	if (inIRQLine == k6800IRQLineIRQ)
	{
		sIRQState = inState;

		// if the state is non-clear, re-check the IRQ states
		if (inState != k6800IRQStateClear)
			CheckIRQLines(false);
	}

#if INTERNAL_REGISTERS
	// set the appropriate state for the TIN line
	else if (inIRQLine == k6800IRQLineTIN)
	{
		// if this wasn't the active edge, bail
		unsigned char edge = (inState == k6800IRQStateClear) ? k6800TCSRStateIEDG : 0;
		if (((sTCSR & k6800TCSRStateIEDG) ^ edge) == 0)
			return;
			
		// set the interrupt
		sTCSR |= k6800TCSRStateICF;
		sTCSRPending |= k6800TCSRStateICF;
		UpdateIRQ2();
		CheckIRQLines(false);
	}
#endif
}


//###################################################################################################
//
//	Asgard6800SetIRQCallback -- sets the function to be called when an interrupt is generated
//
//###################################################################################################

void Asgard6800SetIRQCallback(Asgard6800IRQCallback inCallback)
{
	sIRQCallback = inCallback;
}

//###################################################################################################
//
//	Asgard6800GetIRQCallback -- gets the function to be called when an interrupt is generated
//
//###################################################################################################

Asgard6800IRQCallback Asgard6800GetIRQCallback(void)
{
	return sIRQCallback;
}


//###################################################################################################
//
//	Asgard6800Execute -- run the CPU emulation
//
//	This function executes the 6800 for the specified number of cycles, returning the actual
//	number of cycles executed.
//
//###################################################################################################

asm int Asgard6800Execute(register int inCycles)
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
	li		r0,0
	_asm_get_global(rCycleCount,sInterruptCycleAdjust)
	mr		rICount,inCycles
	_asm_set_global(r0,sInterruptCycleAdjust)
	_asm_set_global(r0,sExitActionCycles)
	_asm_set_global(inCycles,sRequestedCycles)
	
	//	
	// 	initialize tables & goodies
	//
	_asm_get_global_ptr(rArgumentROMPtr,A6800_ARGUMENTROM)
	_asm_get_global_ptr(rOpcodeROMPtr,A6800_OPCODEROM)
	_asm_get_global_ptr(rICountPtr,A6800_ICOUNT)
	_asm_get_global_ptr(rOpcodeTable,sOpcodeTable)
	SAVE_ICOUNT
	lwz		rArgumentROM,0(rArgumentROMPtr)
	lwz		rOpcodeROM,0(rOpcodeROMPtr)

	//
	//	restore the state of the machine
	//
	_asm_get_global(rFlags,sFlags)
	_asm_get_global(rPCAB,sPCAB)
	rlwinm	rFlags,rFlags,0,24,22			// clear the dirty bit in rFlags
	_asm_get_global(rSX,sSX)
	SAVE_FLAGS
	LOAD_COUNTER
	LOAD_OCOMPARE

	//	
	// 	count any cycles from previous interrupts
	//
	COUNT_CYCLES(start)

	//
	//	mark that we're executing
	//
executeMore:	
	li		r9,0
	li		r0,1
	_asm_set_global_b(r9,sExitActions)
	_asm_set_global_b(r0,sExecuting)

	//
	//	if we're still in a WAI state, eat all cycles and bail
	//
	_asm_get_global_b(r0,sInterruptState)
	andi.	r0,r0,kInterruptStateWAI
	beq		executeLoop
	mr		rCycleCount,rICount
	b		executeLoopEnd

	//================================================================================================

	//
	// 	this is the heart of the M6800 execution loop; the process is basically this: load an 
	// 	opcode, look up the function, and branch to it
	//
executeLoop:

	//
	//	internal debugging hook
	//
#if A6800_COREDEBUG
	mr		r3,rSX
	mr		r4,rPCAB
	mr		r5,rFlags
	mr		r6,rICount
	mr		r7,rCounter
	mr		r8,rOutputCompareLeft
	bl		Asgard6800MiniTrace
#endif

	//
	//	external debugging hook
	//
#ifdef A6800_DEBUGHOOK
	_asm_get_global(r3,mame_debug)
#if TARGET_RT_MAC_CFM
	lwz		r3,0(r3)
#endif
	beq		executeLoopNoDebug
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rSX,sSX)
	SAVE_FLAGS
	SAVE_ICOUNT
	SAVE_COUNTER
	SAVE_OCOMPARE
	bl		A6800_DEBUGHOOK
	_asm_get_global(rPCAB,sPCAB)
	_asm_get_global(rSX,sSX)
	LOAD_FLAGS
	LOAD_ICOUNT
	LOAD_COUNTER
	LOAD_OCOMPARE
#endif

executeLoopNoDebug:
	//
	//	read the opcode and branch to the appropriate location
	//
	GET_PC(r4)									// fetch the current PC
	lbzx	r3,rOpcodeROM,r4					// load the opcode
	addis	rPCAB,rPCAB,1						// increment & wrap the PC
	rlwinm	r5,r3,2,0,29						// r5 = r3 << 2
	lwzx	r5,rOpcodeTable,r5					// r5 = rOpcodeTable[r3 << 2]
	li		rCycleCount,0						// cycles = 0
	mtctr	r5									// ctr = r5
	_asm_set_global(r4,sOpcodePC)				// save the PC
	bctr										// go for it

	//
	//	we get back here after any opcode that needs to store a byte result at (rEA)
	//
executeLoopEndWriteEA:
	SAVE_PCAB
	rlwinm	r3,rEA,0,16,31;						// r3 = address
	SAVE_FLAGS
	bl		WRITEMEM							// perform the write
	bl		postExtern							// post-process

	//
	//	we get back here after any other opcode
	//
executeLoopEnd:
	COUNT_CYCLES(loop)
	bgt		executeLoop							// loop if we're not done

	//================================================================================================

executeLoopExit:
	//
	// 	add back any exit action cycles and store the final icount
	//
	_asm_get_global(rCycleCount,sExitActionCycles)
	li		r0,0
	add		rICount,rICount,rCycleCount
	_asm_set_global(r0,sExitActionCycles)
	SAVE_ICOUNT

	//
	// 	save the final state of the machine
	//
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rSX,sSX)
	SAVE_FLAGS
	SAVE_COUNTER
	SAVE_OCOMPARE

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
	li		r0,0
	_asm_get_global(rCycleCount,sInterruptCycleAdjust)
	_asm_set_global(r0,sInterruptCycleAdjust)
	COUNT_CYCLES(end)

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
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rSX,sSX)
	SAVE_FLAGS
	SAVE_COUNTER
	SAVE_OCOMPARE
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
	LOAD_FLAGS									// restore the flags
	LOAD_ICOUNT									// rICount = A6800_ICOUNT
	andi.	r0,rFlags,kFlagsDirty			 	// extract the dirty flag
	LOAD_ROM									// reload the ROM pointer
	beqlr										// if the registers aren't dirty, return
	LOAD_SX										// get the new SX
	rlwinm	rFlags,rFlags,0,24,22				// clear the dirty bit
	LOAD_PCAB									// get the new PC/A/B
	LOAD_COUNTER
	LOAD_OCOMPARE
	blr											// return

	//================================================================================================

#ifdef A6800_UPDATEBANK

	//
	//	post-PC change update: make sure the ROM bank hasn't been switched out from under us
	//
updateBank:
	mflr	rLinkSave2
	GET_PC(r3)
	bl		A6800_UPDATEBANK
	mtlr	rLinkSave2
	lwz		rOpcodeROM,0(rOpcodeROMPtr)			// restore the ROM pointer
	lwz		rArgumentROM,0(rArgumentROMPtr)		// restore the argument ROM pointer
	blr

#endif

	//================================================================================================

#if INTERNAL_REGISTERS

	//
	//	counter compare: we get called here any time the counter value passes the output compare
	//
counterOCI:
	_asm_get_global_b(r4,sTCSR)
	andi.	r0,r4,k6800TCSRStateEOCI
	ori		r4,r4,k6800TCSRStateOCF
	_asm_set_global_b(r4,sTCSR)
	bne		handleINT2
	cmpwi	rICount,0
	blr
	
	//
	//	counter overflow: we get called here any time the counter value overflows
	//
counterTOI:
	_asm_get_global_b(r4,sTCSR)
	andi.	r0,r4,k6800TCSRStateETOI
	ori		r4,r4,k6800TCSRStateTOF
	_asm_set_global_b(r4,sTCSR)
	bne		handleINT2
	cmpwi	rICount,0
	blr

handleINT2:
	mflr	rLinkSave
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rSX,sSX)
	SAVE_FLAGS
	bl		CheckIRQLines
	bl		postExtern
	mtlr	rLinkSave
	cmpwi	rICount,0
	blr

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
	//		SWI: software interrupt generator
	//
entry static swi
	_asm_set_global(rPCAB,sPCAB)
	_asm_set_global(rSX,sSX)
	SAVE_FLAGS
	SAVE_COUNTER
	SAVE_OCOMPARE
	bl		ProcessSWI
	bl		postExtern
	b		executeLoopEnd
	
	//================================================================================================

	//
	//		PSHA/PSHB: register push instructions
	//
entry static psha
	CYCLES(4,3,4)
	SAVE_PCAB
	GET_S(r3)
	SAVE_FLAGS
	GET_A(r4)
	subis	rSX,rSX,1
	bl		WRITEMEM
	SAVE_SX
	bl		postExtern
	b		executeLoopEnd

entry static pshb
	CYCLES(4,3,4)
	SAVE_PCAB
	GET_S(r3)
	SAVE_FLAGS
	GET_B(r4)
	subis	rSX,rSX,1
	bl		WRITEMEM
	SAVE_SX
	bl		postExtern
	b		executeLoopEnd

#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static pshx
	CYCLES(2,4,5)
	PUSH_WORD_LO_SAVE(rSX)
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		PULA/PULB: register pull instructions
	//
entry static pula
	CYCLES(4,4,3)
	SAVE_PCAB
	addis	rSX,rSX,1
	SAVE_FLAGS
	GET_S(r3)
	bl		READMEM
	SET_A(r3)
	bl		postExtern
	SAVE_SX
	b		executeLoopEnd

entry static pulb
	CYCLES(4,4,3)
	SAVE_PCAB
	addis	rSX,rSX,1
	SAVE_FLAGS
	GET_S(r3)
	bl		READMEM
	SET_B(r3)
	bl		postExtern
	SAVE_SX
	b		executeLoopEnd

#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static pulx
	CYCLES(2,5,4)
	PULL_WORD_LO_SAVE(rSX)
	b		executeLoopEnd

#endif

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
	CYCLES(3,3,3)
ldamemcommon:
	READ_BYTE_SAVE(rEA)
ldacommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in result
	rlwimi	rFlags,r3,28,28,28					// N = (r4 & 0x80)
	SET_A(r3)									// A = r3
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static lda_ix
	CYCLES(5,4,4)
	INDEXED
	b		ldamemcommon

entry static lda_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
ldbmemcommon:
	READ_BYTE_SAVE(rEA)
ldbcommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r3								// r7 = number of zeros in result
	rlwimi	rFlags,r3,28,28,28					// N = (r4 & 0x80)
	SET_B(r3)									// A = r3
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static ldb_ix
	CYCLES(5,4,4)
	INDEXED
	b		ldbmemcommon

entry static ldb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		ldbmemcommon

	//
	//		LDS
	//
entry static lds_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(3,3,3)
	b		ldscommon

entry static lds_di
	DIRECT
	CYCLES(4,4,4)
ldsmemcommon:
	READ_WORD_SAVE(rEA)
ldscommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_S(rTempSave)							// S = r3
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	SAVE_SX										// save the updated S
	b		executeLoopEnd

entry static lds_ix
	CYCLES(6,5,5)
	INDEXED
	b		ldsmemcommon

entry static lds_ex
	EXTENDED
	CYCLES(5,5,5)
	b		ldsmemcommon

	//
	//		LDX
	//
entry static ldx_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(3,3,3)
	b		ldxcommon

entry static ldx_di
	DIRECT
	CYCLES(4,4,4)
ldxmemcommon:
	READ_WORD_SAVE(rEA)
ldxcommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_X(rTempSave)							// X = r3
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	SAVE_SX										// save the updated S
	b		executeLoopEnd

entry static ldx_ix
	CYCLES(6,5,5)
	INDEXED
	b		ldxmemcommon

entry static ldx_ex
	EXTENDED
	CYCLES(5,5,5)
	b		ldxmemcommon

	//
	//		LDD
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static ldd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(2,3,3)
	b		lddcommon

entry static ldd_di
	DIRECT
	CYCLES(2,4,4)
lddmemcommon:
	READ_WORD_SAVE(rEA)
lddcommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	SET_D(rTempSave)							// X = r3
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static ldd_ix
	CYCLES(2,5,5)
	INDEXED
	b		lddmemcommon

entry static ldd_ex
	EXTENDED
	CYCLES(2,5,5)
	b		lddmemcommon

#endif

	//================================================================================================

	//
	//		ST: register store operations
	//

	//
	//		STA
	//
entry static sta_di
	DIRECT
	CYCLES(4,3,3)
stamemcommon:
	GET_A(r4)
st8memcommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static sta_ix
	CYCLES(6,4,4)
	INDEXED
	b		stamemcommon

entry static sta_ex
	EXTENDED
	CYCLES(5,4,4)
	b		stamemcommon

	//
	//		STB
	//
entry static stb_di
	DIRECT
	CYCLES(4,3,3)
	GET_B(r4)
	b		st8memcommon

entry static stb_ix
	CYCLES(6,4,4)
	INDEXED
	GET_B(r4)
	b		st8memcommon

entry static stb_ex
	EXTENDED
	CYCLES(5,4,4)
	GET_B(r4)
	b		st8memcommon

	//
	//		STS
	//
entry static sts_di
	DIRECT
	CYCLES(5,4,4)
stsmemcommon:
	GET_S(rTempSave)
st16memcommon:
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,rTempSave						// r7 = number of zeros in result
	rlwimi	rFlags,rTempSave,20,28,28			// N = (r4 & 0x8000)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	WRITE_WORD_LO_SAVE(rEA,rTempSave)
	b		executeLoopEnd

entry static sts_ix
	CYCLES(7,5,5)
	INDEXED
	b		stsmemcommon

entry static sts_ex
	EXTENDED
	CYCLES(6,5,5)
	b		stsmemcommon

	//
	//		STX
	//
entry static stx_di
	DIRECT
	CYCLES(5,4,4)
	GET_X(rTempSave)
	b		st16memcommon

entry static stx_ix
	CYCLES(7,5,5)
	INDEXED
	GET_X(rTempSave)
	b		st16memcommon

entry static stx_ex
	EXTENDED
	CYCLES(6,5,5)
	GET_X(rTempSave)
	b		st16memcommon

	//
	//		STD
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static std_di
	DIRECT
	CYCLES(2,4,4)
	GET_D(rTempSave)
	b		st16memcommon

entry static std_ix
	CYCLES(2,5,5)
	INDEXED
	GET_D(rTempSave)
	b		st16memcommon

entry static std_ex
	EXTENDED
	CYCLES(2,5,5)
	GET_D(rTempSave)
	b		st16memcommon

#endif

	//================================================================================================

	//
	//		TAP/TPA/TBA/TAB/TSX/TXS: register transfer instructions
	//
entry static tap
	rlwimi	rFlags,rPCAB,24,24,31
	CYCLES(2,2,1)
	b		executeLoopEnd

entry static tpa
	rlwimi	rPCAB,rFlags,8,16,23
	CYCLES(2,2,1)
	b		executeLoopEnd

entry static tba
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	GET_B(r3)									// r3 = B
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,28,28,28				// N = (B & 0x80)
	cntlzw	r7,r3								// r7 = number of zeros in the result
	SET_A(r3)									// A = B
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0)
	b		executeLoopEnd

entry static tab
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	GET_A(r3)									// r3 = A
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,20,28,28				// N = (A & 0x80)
	cntlzw	r7,r3								// r7 = number of zeros in the result
	SET_B(r3)									// B = A
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0)
	b		executeLoopEnd

entry static tsx
	GET_S(r3)
	addi	r3,r3,1
	SET_X(r3)
	CYCLES(4,3,1)
	SAVE_SX
	b		executeLoopEnd

entry static txs
	GET_X(r3)
	subi	r3,r3,1
	SET_S(r3)
	CYCLES(4,3,1)
	SAVE_SX
	b		executeLoopEnd

	//================================================================================================

	//
	//		CLR: register/memory clear operations
	//

	//
	//		Memory clear
	//
entry static clr_ix
	CYCLES(7,6,5)
	INDEXED
	li		r0,k6800FlagZ						// r0 = (N=V=C=0, Z=1)
	li		r4,0								// r4 = 0
	rlwimi	rFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEndWriteEA

entry static clr_ex
	CYCLES(6,6,5)
	EXTENDED
	li		r0,k6800FlagZ						// r0 = (N=V=C=0, Z=1)
	li		r4,0								// r4 = 0
	rlwimi	rFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEndWriteEA

	//
	//		Register (A/B) clear
	//
entry static clra
	li		r0,k6800FlagZ						// r0 = (N=V=C=0, Z=1)
	rlwinm	rPCAB,rPCAB,0,24,15					// A = 0
	CYCLES(2,2,1)
	rlwimi	rFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEnd

entry static clrb
	li		r0,k6800FlagZ						// r0 = (N=V=C=0, Z=1)
	rlwinm	rPCAB,rPCAB,0,0,23					// B = 0
	CYCLES(2,2,1)
	rlwimi	rFlags,r0,0,28,31					// N=V=C=0, Z=1
	b		executeLoopEnd

	//================================================================================================

	//
	//		ADD: register/memory addition operations
	//

	//
	//		ADDA
	//
entry static aba
	GET_B(r3)
	CYCLES(2,2,1)
	b		addacommon

entry static adda_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	b		addacommon

entry static adda_di
	DIRECT
	CYCLES(3,3,3)
addamemcommon:
	READ_BYTE_SAVE(rEA)
addacommon:
	GET_A(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
setflagsha:
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	rlwimi	rFlags,r8,1,26,26					// H = (r5 ^ r3 ^ r4) & 0x10
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static adda_ix
	CYCLES(5,4,4)
	INDEXED
	b		addamemcommon

entry static adda_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
addbmemcommon:
	READ_BYTE_SAVE(rEA)
addbcommon:
	GET_B(r5)									// r5 = A
	add		r4,r5,r3							// r4 = r5 + r3
setflagshb:
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	rlwimi	rFlags,r8,1,26,26					// H = (r5 ^ r3 ^ r4) & 0x10
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_B(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static addb_ix
	CYCLES(5,4,4)
	INDEXED
	b		addbmemcommon

entry static addb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		addbmemcommon

	//
	//		ADDD
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static addd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(2,4,3)
	b		adddcommon

entry static addd_di
	DIRECT
	CYCLES(2,5,4)
adddmemcommon:
	READ_WORD_SAVE(rEA)
adddcommon:
	GET_D(r5)									// r5 = D
	add		r4,r5,rTempSave						// r4 = r5 + rTempSave
setflagsd:
	xor		r8,r5,rTempSave						// r8 = r5 ^ rTempSave
	rlwimi	rFlags,r4,16,31,31					// C = (r4 & 0x10000)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,20,28,28					// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,18,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x8000
	SET_D(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static addd_ix
	CYCLES(2,6,5)
	INDEXED
	b		adddmemcommon

entry static addd_ex
	EXTENDED
	CYCLES(2,6,5)
	b		adddmemcommon

#endif

	//
	//		ADDX
	//
#if (A6800_CHIP == 8105)

entry static addx_ex
	EXTENDED
	CYCLES(2,6,5)
	READ_WORD_SAVE(rEA)
	GET_X(r5)									// r5 = D
	add		r4,r5,rTempSave						// r4 = r5 + rTempSave
	xor		r8,r5,rTempSave						// r8 = r5 ^ rTempSave
	rlwimi	rFlags,r4,16,31,31					// C = (r4 & 0x10000)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,20,28,28					// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,18,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x8000
	SET_X(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		ADC: register/memory addition with carry operations
	//

	//
	//		ADCA
	//
entry static adca_im
	READ_OPCODE_ARG(r3)
	GET_A(r5)									// r5 = A
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	add		r4,r5,r3							// r4 = r5 + r3
	CYCLES(2,2,2)
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagsha							// same as ADD

entry static adca_di
	DIRECT
	CYCLES(3,3,3)
adcamemcommon:
	READ_BYTE_SAVE(rEA)
	GET_A(r5)									// r5 = A
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	add		r4,r5,r3							// r4 = r5 + r3
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagsha							// same as ADD

entry static adca_ix
	CYCLES(5,4,4)
	INDEXED
	b		adcamemcommon

entry static adca_ex
	EXTENDED
	CYCLES(4,4,4)
	b		adcamemcommon

	//
	//		ADCB
	//
entry static adcb_im
	READ_OPCODE_ARG(r3)
	GET_B(r5)									// r5 = B
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	add		r4,r5,r3							// r4 = r5 + r3
	CYCLES(2,2,2)
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagshb							// same as ADD

entry static adcb_di
	DIRECT
	CYCLES(3,3,3)
adcbmemcommon:
	READ_BYTE_SAVE(rEA)
	GET_B(r5)									// r5 = B
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	add		r4,r5,r3							// r4 = r5 + r3
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagshb							// same as ADD

entry static adcb_ix
	CYCLES(5,4,4)
	INDEXED
	b		adcbmemcommon

entry static adcb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		adcbmemcommon

#if (A6800_CHIP == 8105) // NSC8105 only. Flags are a guess, copied from ADCB
setflagshx:
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	rlwimi	rFlags,r8,1,26,26					// H = (r5 ^ r3 ^ r4) & 0x10
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_X(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

	//
	//		ADCX
	//
entry static adcx_im
	READ_OPCODE_ARG(r3)
	GET_X(r5)									// r5 = X
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	add		r4,r5,r3							// r4 = r5 + r3
	CYCLES(2,2,2)
	add		r4,r4,r10							// r4 = r5 + r3 + C
	b		setflagshx							// same as ADD
#endif

	//================================================================================================

	//
	//		ABX: add B to X operation
	//

#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static abx
	GET_B(r3)
	GET_X(r4)
	add		r4,r4,r3
	CYCLES(0,3,1)
	SET_X(r4)
	SAVE_SX
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		SUB: subtract two values

	//
	//
	//		SUBA
	//
entry static sba
	GET_B(r3)
	GET_A(r5)
	CYCLES(2,2,1)
	sub		r4,r5,r3
setflagsa:										// same as ADD, but don't change H
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd
	
entry static suba_im
	READ_OPCODE_ARG(r3)
	GET_A(r5)									// r5 = A
	CYCLES(2,2,2)
	sub		r4,r5,r3							// r4 = r5 - r3
	b		setflagsa							// same as ADD

entry static suba_di
	DIRECT
	CYCLES(3,3,3)
subamemcommon:
	READ_BYTE_SAVE(rEA)
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	b		setflagsa							// same as ADD

entry static suba_ix
	CYCLES(5,4,4)
	INDEXED
	b		subamemcommon

entry static suba_ex
	EXTENDED
	CYCLES(4,4,4)
	b		subamemcommon

	//
	//		SUBB
	//
entry static subb_im
	READ_OPCODE_ARG(r3)
	GET_B(r5)									// r5 = A
	CYCLES(2,2,2)
	sub		r4,r5,r3							// r4 = r5 - r3
setflagsb:										// same as ADD but don't change H
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	SET_B(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static subb_di
	DIRECT
	CYCLES(3,3,3)
subbmemcommon:
	READ_BYTE_SAVE(rEA)
	GET_B(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	b		setflagsb							// same as ADD

entry static subb_ix
	CYCLES(5,4,4)
	INDEXED
	b		subbmemcommon

entry static subb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		subbmemcommon

	//
	//		SUBD
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static subd_im
	READ_OPCODE_ARG_WORD(rTempSave)
	GET_D(r5)									// r5 = A
	CYCLES(0,4,3)
	sub		r4,r5,rTempSave						// r4 = r5 - rTempSave
	b		setflagsd							// same as ADD

entry static subd_di
	DIRECT
	CYCLES(0,5,4)
subdmemcommon:
	READ_WORD_SAVE(rEA)
	GET_D(r5)									// r5 = A
	sub		r4,r5,rTempSave						// r4 = r5 - rTempSave
	b		setflagsd							// same as ADD

entry static subd_ix
	CYCLES(0,6,5)
	INDEXED
	b		subdmemcommon

entry static subd_ex
	EXTENDED
	CYCLES(0,6,5)
	b		subdmemcommon

#endif

	//================================================================================================

	//
	//		SBC: register/memory subtract with borrow operations
	//

	//
	//		SBCA
	//
entry static sbca_im
	READ_OPCODE_ARG(r3)
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	CYCLES(2,2,2)
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsa							// same as ADD

entry static sbca_di
	DIRECT
	CYCLES(3,3,3)
sbcamemcommon:
	READ_BYTE_SAVE(rEA)
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	GET_A(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsa							// same as ADD

entry static sbca_ix
	CYCLES(5,4,4)
	INDEXED
	b		sbcamemcommon

entry static sbca_ex
	EXTENDED
	CYCLES(4,4,4)
	b		sbcamemcommon

	//
	//		SBCB
	//
entry static sbcb_im
	READ_OPCODE_ARG(r3)
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	GET_B(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	CYCLES(2,2,2)
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsb							// same as ADD

entry static sbcb_di
	DIRECT
	CYCLES(3,3,3)
sbcbmemcommon:
	READ_BYTE_SAVE(rEA)
	rlwinm	r10,rFlags,0,31,31					// r10 = C
	GET_B(r5)									// r5 = A
	sub		r4,r5,r3							// r4 = r5 - r3
	sub		r4,r4,r10							// r4 = r5 - r3 - C
	b		setflagsb							// same as ADD

entry static sbcb_ix
	CYCLES(5,4,4)
	INDEXED
	b		sbcbmemcommon

entry static sbcb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		sbcbmemcommon

	//================================================================================================

	//
	//		NEG: register/memory negate operations
	//

	//
	//		Memory negate
	//
entry static neg_ix
	CYCLES(7,6,6)
	INDEXED
negcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	li			r5,0
	sub		r4,r5,r3							// r4 = 0 - r3
setflagsmem:
	xor		r8,r5,r3							// r8 = r5 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static neg_ex
	CYCLES(6,6,6)
	EXTENDED
	b		negcommon

	//
	//		Register (A/B) negate
	//
entry static nega
	GET_A(r3)
	li			r5,0
	CYCLES(2,2,1)
	sub		r4,r5,r3
	b		setflagsa

entry static negb
	GET_B(r3)
	li			r5,0
	CYCLES(2,2,1)
	sub		r4,r5,r3
	b		setflagsb

	//================================================================================================

	//
	//		MUL: multiply A & B
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static mul
	GET_A(r3)									// r3 = A
	GET_B(r5)									// r5 = B
	mullw	r4,r3,r5							// r4 = r3 * r5
	CYCLES(0,10,7)
	rlwimi	rFlags,r4,25,31,31					// r0 = (r4 & 0x80)
	SET_D(r4)									// D = r4
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		INC: increment a value
	//
entry static inc_ix
	INDEXED
	CYCLES(7,6,6)
inccommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	xori	r6,r3,0x7f							// r6 = r3 ^ 0x7f
	xori	r7,r3,0xff							// r7 = r3 ^ 0xff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0xff)
	rlwimi	rFlags,r6,28,30,30					// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	rlwimi	rFlags,r4,28,28,28 					// N = (r4 & 0x80)
	b		executeLoopEndWriteEA

entry static inc_ex
	EXTENDED
	CYCLES(6,6,6)
	b		inccommon

entry static inca
	GET_A(r3)									// r3 = A
	CYCLES(2,2,1)
	xori	r6,r3,0x7f							// r6 = r3 ^ 0x7f
	xori	r7,r3,0xff							// r7 = r3 ^ 0xff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rFlags,r6,28,30,30					// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r4,28,28,28 					// N = (r4 & 0x80)
	b		executeLoopEnd

entry static incb
	GET_B(r3)									// r3 = B
	CYCLES(2,2,1)
	xori	r6,r3,0x7f							// r6 = r3 ^ 0x7f
	xori	r7,r3,0xff							// r7 = r3 ^ 0xff
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rFlags,r6,28,30,30					// V = (r3 == 0x7f)
	addi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r4,28,28,28 					// N = (r4 & 0x80)
	b		executeLoopEnd

entry static inx
	GET_X(r3)									// r3 = X
	CYCLES(4,3,1)
	xori	r7,r3,0xffff						// r7 = r3 ^ 0xffff
	addi	r4,r3,1								// r4 = result = r3 + 1
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	SET_X(r4)									// X = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	SAVE_SX
	b		executeLoopEnd

entry static ins
	addis	rSX,rSX,1							// S += 1
	CYCLES(4,3,1)
	SAVE_SX
	b		executeLoopEnd

	//================================================================================================

	//
	//		DEC: decrement a value
	//
entry static dec_ix
	INDEXED
	CYCLES(7,6,6)
deccommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0xff)
	rlwimi	rFlags,r6,28,30,30					// V = (r3 == 0x7f)
	subi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	rlwimi	rFlags,r4,28,28,28 					// N = (r4 & 0x80)
	b		executeLoopEndWriteEA

entry static dec_ex
	EXTENDED
	CYCLES(6,6,6)
	b		deccommon

entry static deca
	GET_A(r3)									// r3 = A
	CYCLES(2,2,1)
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rFlags,r6,28,30,30					// V = (r3 == 0x7f)
	subi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r4,28,28,28 					// N = (r4 & 0x80)
	b		executeLoopEnd

entry static decb
	GET_B(r3)									// r3 = B
	CYCLES(2,2,1)
	xori	r6,r3,0x80							// r6 = r3 ^ 0x80
	xori	r7,r3,0x01							// r7 = r3 ^ 0x01
	cntlzw	r6,r6								// r6 = number of zeros in (r3 ^ 0x80)
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	rlwimi	rFlags,r6,28,30,30					// V = (r3 == 0x7f)
	subi	r4,r3,1								// r4 = result = r3 + 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r4,28,28,28 					// N = (r4 & 0x80)
	b		executeLoopEnd

entry static dex
	GET_X(r3)									// r3 = X
	CYCLES(4,3,1)
	xori	r7,r3,0x0001						// r7 = r3 ^ 0x0001
	subi	r4,r3,1								// r4 = result = r3 + 1
	cntlzw	r7,r7								// r7 = number of zeros in (r3 ^ 0x01)
	SET_X(r4)									// X = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0xff)
	SAVE_SX
	b		executeLoopEnd

entry static des
	subis	rSX,rSX,1							// S -= 1
	CYCLES(4,3,1)
	SAVE_SX
	b		executeLoopEnd

	//================================================================================================

	//
	//		CMP: register/memory compare operations
	//

	//
	//		CMPA
	//
entry static cba
	GET_B(r3)
	CYCLES(2,2,1)
	GET_A(rTempSave2)
	b		cmp8common
	
entry static cmpa_im
	READ_OPCODE_ARG(r3)
	CYCLES(2,2,2)
	GET_A(rTempSave2)
	b		cmp8common

entry static cmpa_di
	DIRECT
	CYCLES(3,3,3)
cmpamemcommon:
	GET_A(rTempSave2)
cmp8memcommon:
	READ_BYTE_SAVE(rEA)
cmp8common:
	sub		r4,rTempSave2,r3					// r4 = rTempSave2 - r3
	xor		r8,rTempSave2,r3					// r8 = rTempSave2 ^ r3
	rlwimi	rFlags,r4,24,31,31					// C = (r4 & 0x100)
	xor		r8,r8,r4							// r8 = r5 ^ r3 ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwinm	r6,r4,0,24,31						// r6 = r4 & 0xff
	xor		r9,r9,r8							// r9 = r5 ^ r3 ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,26,30,30					// V = (r5 ^ r3 ^ r4 ^ (r4 >> 1)) & 0x80
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static cmpa_ix
	CYCLES(5,4,4)
	INDEXED
	b		cmpamemcommon

entry static cmpa_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
	GET_B(rTempSave2)
	b		cmp8memcommon

entry static cmpb_ix
	CYCLES(5,4,4)
	INDEXED
	GET_B(rTempSave2)
	b		cmp8memcommon

entry static cmpb_ex
	EXTENDED
	CYCLES(4,4,4)
	GET_B(rTempSave2)
	b		cmp8memcommon

	//
	//		CMPX
	//
entry static cmpx_im
	READ_OPCODE_ARG_WORD(rTempSave)
	CYCLES(3,4,3)
	GET_X(rTempSave2)
	b		cmp16common

entry static cmpx_di
	CYCLES(4,5,4)
	DIRECT
cmpxmemcommon:
	GET_X(rTempSave2)
	READ_WORD_SAVE(rEA)
cmp16common:
	sub		r4,rTempSave2,rTempSave				// r4 = rTempSave2 - rTempSave
	xor		r8,rTempSave2,rTempSave				// r8 = rTempSave2 ^ rTempSave
	rlwimi	rFlags,r4,16,31,31					// C = (r4 & 0x10000)
	xor		r8,r8,r4							// r8 = rTempSave2 ^ rTempSave ^ r4
	rlwinm	r9,r4,31,1,31						// r9 = r4 >> 1
	rlwimi	rFlags,r4,20,28,28					// N = (r4 & 0x8000)
	rlwinm	r6,r4,0,16,31						// r6 = r4 & 0xffff
	xor		r9,r9,r8							// r9 = rTempSave2 ^ rTempSave ^ r4 ^ (r4 >> 1)
	cntlzw	r7,r6								// r7 = number of zeros in result
	rlwimi	rFlags,r9,18,30,30					// V = (rTempSave2 ^ rTempSave ^ r4 ^ (r4 >> 1)) & 0x8000
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static cmpx_ix
	CYCLES(6,6,5)
	INDEXED
	b		cmpxmemcommon

entry static cmpx_ex
	EXTENDED
	CYCLES(5,6,5)
	b		cmpxmemcommon

	//================================================================================================

	//
	//		TST: register/memory test operations
	//

	//
	//		Memory test
	//
entry static tst_ix
	CYCLES(7,6,4)
	INDEXED
tstcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	rFlags,rFlags,0,0,29				// clear V,C
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rFlags,r3,28,28,28					// N = (r3 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0)
	b		executeLoopEnd

entry static tst_ex
	CYCLES(6,6,4)
	EXTENDED
	b		tstcommon

	//
	//		Register (A/B) test
	//
entry static tsta
	GET_A(r3)									// r3 = A
	rlwinm	rFlags,rFlags,0,0,29				// clear V,C
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rFlags,r3,28,28,28					// N = (r3 & 0x80)
	CYCLES(2,2,1)
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0)
	b		executeLoopEnd

entry static tstb
	GET_B(r3)									// r3 = B
	rlwinm	rFlags,rFlags,0,0,29				// clear V,C
	cntlzw	r7,r3								// r7 = number of zeros in r3
	rlwimi	rFlags,r3,28,28,28					// N = (r3 & 0x80)
	CYCLES(2,2,1)
	rlwimi	rFlags,r7,29,29,29					// Z = (r3 == 0)
	b		executeLoopEnd

	//================================================================================================

	//
	//		LSR: register/memory logical right shift operations
	//

	//
	//		Memory logical right shift
	//
entry static lsr_ix
	CYCLES(7,6,6)
	INDEXED
lsrcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,31,25,31						// r4 = result = r3 >> 1
	rlwimi	rFlags,r3,0,31,31					// C = r3 & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rFlags,rFlags,0,29,27				// N = 0
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static lsr_ex
	CYCLES(6,6,6)
	EXTENDED
	b		lsrcommon

	//
	//		Register (A/B/D) logical right shift
	//
entry static lsra
	rlwinm	r4,rPCAB,23,25,31					// r4 = result = A >> 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,24,31,31				// C = A & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rFlags,rFlags,0,29,27				// N = 0
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static lsrb
	rlwinm	r4,rPCAB,31,25,31					// r4 = result = B >> 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,0,31,31				// C = B & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rFlags,rFlags,0,29,27				// N = 0
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static lsrd
	rlwinm	r4,rPCAB,31,17,31					// r4 = result = D >> 1
	CYCLES(0,3,1)
	rlwimi	rFlags,rPCAB,0,31,31				// C = D & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwinm	rFlags,rFlags,0,29,27				// N = 0
	SET_D(r4)									// D = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		ROR: register/memory right rotate operations
	//

	//
	//		Memory right rotate
	//
entry static ror_ix
	CYCLES(7,6,6)
	INDEXED
rorcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,31,25,31						// r4 = result = r3 >> 1
	rlwimi	rFlags,rFlags,3,28,28 				// N = C
	rlwimi	r4,rFlags,7,24,24					// r4 = result = (r3 >> 1) | (C << 7)
	rlwimi	rFlags,r3,0,31,31					// C = r3 & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static ror_ex
	CYCLES(6,6,6)
	EXTENDED
	b		rorcommon

	//
	//		Register (A/B) right rotate
	//
entry static rora
	rlwinm	r4,rPCAB,23,25,31					// r4 = result = A >> 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rFlags,3,28,28 				// N = C
	rlwimi	r4,rFlags,7,24,24					// r4 = result = (r3 >> 1) | (C << 7)
	rlwimi	rFlags,rPCAB,24,31,31				// C = A & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static rorb
	rlwinm	r4,rPCAB,31,25,31					// r4 = result = B >> 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rFlags,3,28,28 				// N = C
	rlwimi	r4,rFlags,7,24,24					// r4 = result = (r3 >> 1) | (C << 7)
	rlwimi	rFlags,rPCAB,0,31,31				// C = B & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

	//================================================================================================

	//
	//		ASR: register/memory arithmetic right shift operations
	//

	//
	//		Memory arithmetic right shift
	//
entry static asr_ix
	CYCLES(7,6,6)
	INDEXED
asrcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,31,25,31						// r4 = result = r3 >> 1
	rlwimi	rFlags,r3,28,28,28 					// N = (r3 & 0x80)
	rlwimi	r4,r3,0,24,24						// r4 = result = (r3 >> 1) | (r3 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r3,0,31,31					// C = r3 & 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static asr_ex
	CYCLES(6,6,6)
	EXTENDED
	b		asrcommon

	//
	//		Register (A/B) arithmetic right shift
	//
entry static asra
	rlwinm	r4,rPCAB,23,25,31					// r4 = result = A >> 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,20,28,28				// N = (A & 0x80)
	rlwimi	r4,rPCAB,24,24,24					// r4 = result = (A >> 1) | (A & 0x80)
	rlwimi	rFlags,rPCAB,24,31,31				// C = A & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static asrb
	rlwinm	r4,rPCAB,31,25,31					// r4 = result = B >> 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,28,28,28				// N = (B & 0x80)
	rlwimi	r4,rPCAB,0,24,24					// r4 = result = (B >> 1) | (B & 0x80)
	rlwimi	rFlags,rPCAB,0,31,31				// C = B & 1
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

	//================================================================================================

	//
	//		ASL: register/memory arithmetic left shift operations
	//

	//
	//		Memory arithmetic left shift
	//
entry static asl_ix
	CYCLES(7,6,6)
	INDEXED
aslcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,1,24,30						// r4 = result = r3 << 1
	rlwimi	rFlags,r3,29,28,28 					// N = (r3 & 0x40)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r3,25,31,31					// C = r3 & 0x80
	xor		r9,r3,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,26,30,30					// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEndWriteEA

entry static asl_ex
	CYCLES(6,6,6)
	EXTENDED
	b		aslcommon

	//
	//		Register (A/B) arithmetic left shift
	//
entry static asla
	rlwinm	r4,rPCAB,25,24,30					// r4 = result = A << 1
	CYCLES(2,2,1)
	GET_A(r5)									// r5 = A
	rlwimi	rFlags,rPCAB,21,28,28				// N = (A & 0x40)
	xor		r9,r5,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rFlags,rPCAB,17,31,31				// C = A & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,26,30,30					// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

entry static aslb
	rlwinm	r4,rPCAB,1,24,30					// r4 = result = B << 1
	CYCLES(2,2,1)
	rlwimi	rFlags,rPCAB,29,28,28				// N = (B & 0x40)
	xor		r9,rPCAB,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rFlags,rPCAB,25,31,31				// C = B & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,26,30,30					// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808 && A6800_CHIP != 8105)

entry static asld
	rlwinm	r4,rPCAB,1,16,30					// r4 = result = D << 1
	CYCLES(0,3,1)
	rlwimi	rFlags,rPCAB,21,28,28				// N = (D & 0x4000)
	xor		r9,rPCAB,r4							// r9 = r4 ^ (r4 >> 1)
	rlwimi	rFlags,rPCAB,17,31,31				// C = B & 0x8000
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_D(r4)									// D = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,18,30,30					// V = (r4 ^ (r4 >> 1)) & 0x8000
	b		executeLoopEnd

#endif

	//================================================================================================

	//
	//		ROL: register/memory left rotate operations
	//

	//
	//		Memory left rotate
	//
entry static rol_ix
	CYCLES(7,6,6)
	INDEXED
rolcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	rlwinm	r4,r3,1,24,30						// r4 = result = r3 << 1
	rlwimi	r4,rFlags,0,31,31					// r4 = result = (r3 << 1) | C
	rlwimi	rFlags,r3,29,28,28 					// N = (r3 & 0x40)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r3,25,31,31					// C = r3 & 0x80
	xor		r9,r3,r4							// r9 = r4 ^ r3 = r4 ^ (r4 >> 1)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,26,30,30					// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEndWriteEA

entry static rol_ex
	CYCLES(6,6,6)
	EXTENDED
	b		rolcommon

	//
	//		Register (A/B) left rotate
	//
entry static rola
	rlwinm	r4,rPCAB,25,24,30					// r4 = result = A << 1
	CYCLES(2,2,1)
	rlwimi	r4,rFlags,0,31,31					// r4 = result = (A << 1) | C
	GET_A(r3)									// r3 = A
	rlwimi	rFlags,rPCAB,21,28,28				// N = (A & 0x40)
	xor		r9,r3,r4							// r9 = r4 ^ r3 = r4 ^ (r4 >> 1)
	rlwimi	rFlags,rPCAB,17,31,31				// C = A & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,26,30,30					// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

entry static rolb
	rlwinm	r4,rPCAB,1,24,30					// r4 = result = B << 1
	CYCLES(2,2,1)
	rlwimi	r4,rFlags,0,31,31					// r4 = result = (B << 1) | C
	rlwimi	rFlags,rPCAB,29,28,28				// N = (B & 0x40)
	xor		r9,rPCAB,r4							// r9 = r4 ^ rPCAB = r4 ^ (r4 >> 1)
	rlwimi	rFlags,rPCAB,25,31,31				// C = B & 0x80
	cntlzw	r7,r4								// r7 = number of zeros in result
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	rlwimi	rFlags,r9,26,30,30					// V = (r4 ^ (r4 >> 1)) & 0x80
	b		executeLoopEnd

	//================================================================================================

	//
	//		COM: register/memory logical complement
	//

	//
	//		Memory logical complement
	//
entry static com_ix
	CYCLES(7,6,6)
	INDEXED
comcommon:
	READ_BYTE_SAVE(rEA)							// r3 = byte at rEA
	xori	r4,r3,0xff							// r4 = r3 ^ 0xff
	li		r0,k6800FlagC						// r0 = k6800FlagC
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r0,0,30,31					// V = 0, C = 1
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static com_ex
	CYCLES(6,6,6)
	EXTENDED
	b		comcommon

	//
	//		Register (A/B) logical complement
	//
entry static coma
	GET_A(r3)									// r3 = A
	CYCLES(2,2,1)
	xori	r4,r3,0xff							// r4 = result = r3 ^ 0xff
	li		r0,k6800FlagC						// r0 = k6800FlagC
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r0,0,30,31					// V = 0, C = 1
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static comb
	GET_B(r3)									// r3 = B
	CYCLES(2,2,1)
	xori	r4,r3,0xff							// r4 = result = r3 ^ 0xff
	li		r0,k6800FlagC						// r0 = k6800FlagC
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r0,0,30,31					// V = 0, C = 1
	SET_B(r4)									// B = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
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
	CYCLES(3,3,3)
andamemcommon:
	READ_BYTE_SAVE(rEA)
andacommon:
	GET_A(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static anda_ix
	CYCLES(5,4,4)
	INDEXED
	b		andamemcommon

entry static anda_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
andbmemcommon:
	READ_BYTE_SAVE(rEA)
andbcommon:
	GET_B(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	SET_B(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static andb_ix
	CYCLES(5,4,4)
	INDEXED
	b		andbmemcommon

entry static andb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		andbmemcommon

	//
	//		AIM (HD63701YO only)
	//
#if (A6800_CHIP == 63701)

entry static aim_ix
	READ_OPCODE_ARG(rTempSave)
	INDEXED
	CYCLES(0,0,7)
aimcommon:
	READ_BYTE_SAVE(rEA)
	and		r4,r3,rTempSave						// r4 = r3 & rTempSave
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static aim_di
	READ_OPCODE_ARG(rTempSave)
	DIRECT
	CYCLES(0,0,6)
	b		aimcommon

#endif

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
	CYCLES(3,3,3)
oramemcommon:
	READ_BYTE_SAVE(rEA)
oracommon:
	GET_A(r5)									// r5 = A
	or		r4,r5,r3							// r4 = r5 | r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static ora_ix
	CYCLES(5,4,4)
	INDEXED
	b		oramemcommon

entry static ora_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
orbmemcommon:
	READ_BYTE_SAVE(rEA)
orbcommon:
	GET_B(r5)									// r5 = A
	or		r4,r5,r3							// r4 = r5 | r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	SET_B(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static orb_ix
	CYCLES(5,4,4)
	INDEXED
	b		orbmemcommon

entry static orb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		orbmemcommon

	//
	//		OIM (HD63701YO only)
	//
#if (A6800_CHIP == 63701)

entry static oim_ix
	READ_OPCODE_ARG(rTempSave)
	INDEXED
	CYCLES(0,0,7)
oimcommon:
	READ_BYTE_SAVE(rEA)
	or		r4,r3,rTempSave						// r4 = r3 | rTempSave
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static oim_di
	READ_OPCODE_ARG(rTempSave)
	DIRECT
	CYCLES(0,0,6)
	b		oimcommon

#endif

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
	CYCLES(3,3,3)
eoramemcommon:
	READ_BYTE_SAVE(rEA)
eoracommon:
	GET_A(r5)									// r5 = A
	xor		r4,r5,r3							// r4 = r5 ^ r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static eora_ix
	CYCLES(5,4,4)
	INDEXED
	b		eoramemcommon

entry static eora_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
eorbmemcommon:
	READ_BYTE_SAVE(rEA)
eorbcommon:
	GET_B(r5)									// r5 = A
	xor		r4,r5,r3							// r4 = r5 ^ r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	SET_B(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static eorb_ix
	CYCLES(5,4,4)
	INDEXED
	b		eorbmemcommon

entry static eorb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		eorbmemcommon

	//
	//		EIM (HD63701YO only)
	//
#if (A6800_CHIP == 63701)

entry static eim_ix
	READ_OPCODE_ARG(rTempSave)
	INDEXED
	CYCLES(0,0,7)
eimcommon:
	READ_BYTE_SAVE(rEA)
	xor		r4,r3,rTempSave						// r4 = r3 ^ rTempSave
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEndWriteEA

entry static eim_di
	READ_OPCODE_ARG(rTempSave)
	DIRECT
	CYCLES(0,0,6)
	b		eimcommon

#endif

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
	CYCLES(3,3,3)
bitamemcommon:
	READ_BYTE_SAVE(rEA)
bitacommon:
	GET_A(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static bita_ix
	CYCLES(5,4,4)
	INDEXED
	b		bitamemcommon

entry static bita_ex
	EXTENDED
	CYCLES(4,4,4)
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
	CYCLES(3,3,3)
bitbmemcommon:
	READ_BYTE_SAVE(rEA)
bitbcommon:
	GET_B(r5)									// r5 = A
	and		r4,r5,r3							// r4 = r5 & r3
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static bitb_ix
	CYCLES(5,4,4)
	INDEXED
	b		bitbmemcommon

entry static bitb_ex
	EXTENDED
	CYCLES(4,4,4)
	b		bitbmemcommon

	//
	//		TIM (HD63701YO only)
	//
#if (A6800_CHIP == 63701)

entry static tim_ix
	READ_OPCODE_ARG(rTempSave)
	INDEXED
	CYCLES(0,0,5)
timcommon:
	READ_BYTE_SAVE(rEA)
	and		r4,r3,rTempSave						// r4 = r3 & rTempSave
	rlwinm	rFlags,rFlags,0,31,29				// V = 0
	cntlzw	r7,r4								// r7 = number of zeros in result
	rlwimi	rFlags,r4,28,28,28					// N = (r4 & 0x80)
	rlwimi	rFlags,r7,29,29,29					// Z = (r4 == 0)
	b		executeLoopEnd

entry static tim_di
	READ_OPCODE_ARG(rTempSave)
	DIRECT
	CYCLES(0,0,4)
	b		timcommon

#endif

	//================================================================================================

	//
	//		CLV/SEV/CLC/SEC/CLI/STI: modify the flags directly
	//
entry static clv
	rlwinm	rFlags,rFlags,0,31,29
	CYCLES(2,2,1)
	b		executeLoopEnd

entry static sev
	ori		rFlags,rFlags,k6800FlagV
	CYCLES(2,2,1)
	b		executeLoopEnd

entry static clc
	rlwinm	rFlags,rFlags,0,0,30
	CYCLES(2,2,1)
	b		executeLoopEnd

entry static sec
	ori		rFlags,rFlags,k6800FlagC
	CYCLES(2,2,1)
	b		executeLoopEnd

entry static cli
	rlwinm	rFlags,rFlags,0,28,26
	CYCLES(2,2,1)
	bl		checkIRQLines
	b		executeLoopEnd

entry static sti
	ori		rFlags,rFlags,k6800FlagI
	CYCLES(2,2,1)
	b		executeLoopEnd

	//================================================================================================

	//
	//		BRA: branch relative
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
	CYCLES(4,3,3)
	bne+	executeLoopEnd						// end if we're jumping somewhere different
	cmpwi	rICount,0							// are we already 0?
	ble		executeLoopEnd						// if so, just continue
	add		rCycleCount,rCycleCount,rICount		// else add all the remaining cycles to the icount
	b		executeLoopEnd						// all done

	//================================================================================================

	//
	//		BSR: branch to subroutine relative
	//
entry static bsr
	READ_OPCODE_ARG(rEA)						// r3 = offset
	CYCLES(8,6,5)
	extsb	rEA,rEA								// sign-extend
	rlwinm	rEA,rEA,16,0,15						// rotate high
	PUSH_WORD_HI_SAVE(rPCAB)
	add		rPCAB,rPCAB,rEA						// adjust the PC
	UPDATE_BANK
	b		executeLoopEnd						// all done

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
	CYCLES(10,10,10)
	SAVE_PCAB
	mflr	rLinkSave
	addis	rSX,rSX,1
	SAVE_FLAGS

	// pull CC
	GET_S(r3)
	bl		READMEM
	addis	rSX,rSX,1
	rlwimi	rFlags,r3,0,24,31

	// pull B
	GET_S(r3)
	bl		READMEM
	addis	rSX,rSX,1
	rlwimi	rPCAB,r3,0,24,31
	
	// pull A
	GET_S(r3)
	bl		READMEM
	addis	rSX,rSX,1
	rlwimi	rPCAB,r3,8,16,23
	
	// pull X
	GET_S(r3)
	bl		READMEM
	addis	rSX,rSX,1
	rlwimi	rSX,r3,8,16,23
	GET_S(r3)
	bl		READMEM
	addis	rSX,rSX,1
	rlwimi	rSX,r3,0,24,31
	
	// pull PC
	GET_S(r3)
	bl		READMEM
	addis	rSX,rSX,1
	rlwimi	rPCAB,r3,24,0,7
	GET_S(r3)
	bl		READMEM
	rlwimi	rPCAB,r3,16,8,15

	// clean up
	SAVE_FLAGS
	SAVE_PCAB
	mtlr	rLinkSave
	SAVE_SX
	bl		postExtern

	UPDATE_BANK
	bl		checkIRQLines
	b		executeLoopEnd

	//================================================================================================

	//
	//		BRN: branch, but don't really branch -- huh?
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808)

entry static brn
	addis	rPCAB,rPCAB,1
	CYCLES(0,3,3)
	b		executeLoopEnd
	
#endif

	//================================================================================================

	//
	//		Bxx: branch conditionally relative
	//

	//
	//		BHI
	//
entry static bhi
	andi.	r0,rFlags,5
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BLS
	//
entry static bls
	andi.	r0,rFlags,5
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BCC
	//
entry static bcc
	andi.	r0,rFlags,1
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BCS
	//
entry static bcs
	andi.	r0,rFlags,1
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BNE
	//
entry static bne
	andi.	r0,rFlags,4
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BEQ
	//
entry static beq
	andi.	r0,rFlags,4
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BVC
	//
entry static bvc
	andi.	r0,rFlags,2
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BVS
	//
entry static bvs
	andi.	r0,rFlags,2
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BPL
	//
entry static bpl
	andi.	r0,rFlags,8
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BMI
	//
entry static bmi
	andi.	r0,rFlags,8
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BGE
	//
entry static bge
	rlwinm	r3,rFlags,29,31,31
	rlwinm	r4,rFlags,31,31,31
	xor.	r0,r3,r4
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BLT
	//
entry static blt
	rlwinm	r3,rFlags,29,31,31
	rlwinm	r4,rFlags,31,31,31
	xor.	r0,r3,r4
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BGT
	//
entry static bgt
	rlwinm	r3,rFlags,29,31,31
	rlwinm	r4,rFlags,31,31,31
	rlwinm	r5,rFlags,30,31,31
	xor		r3,r3,r4
	or.		r0,r3,r5
	beq		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//
	//		BLE
	//
entry static ble
	rlwinm	r3,rFlags,29,31,31
	rlwinm	r4,rFlags,31,31,31
	rlwinm	r5,rFlags,30,31,31
	xor		r3,r3,r4
	or.		r0,r3,r5
	bne		takebra
	CYCLES(4,3,3)
	addis	rPCAB,rPCAB,1
	b		executeLoopEnd

	//================================================================================================

	//
	//		JMP: jump to effective address
	//
entry static jmp_ix
	CYCLES(4,3,3)
	INDEXED
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

entry static jmp_ex
	EXTENDED
	CYCLES(3,3,3)
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

	//================================================================================================

	//
	//		JSR: jump to subroutine at effective address
	//
#if (A6800_CHIP != 6800 && A6800_CHIP != 6802 && A6800_CHIP != 6808)

entry static jsr_di
	DIRECT
	CYCLES(0,5,5)

#endif

jsrcommon:
	PUSH_WORD_HI_SAVE(rPCAB)
	SET_PC(rEA)
	UPDATE_BANK
	b		executeLoopEnd

entry static jsr_ix
	CYCLES(8,6,5)
	INDEXED
	b		jsrcommon

entry static jsr_ex
	EXTENDED
	CYCLES(9,6,6)
	b		jsrcommon

	//================================================================================================

	//
	//		WAI: processor timing/delay instruction
	//
entry static wai
	SAVE_PCAB
	SAVE_FLAGS

	// push PC
	GET_S(r3)
	rlwinm	r4,rPCAB,16,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM
	GET_S(r3)
	rlwinm	r4,rPCAB,8,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM

	// push X
	GET_S(r3)
	rlwinm	r4,rSX,0,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM
	GET_S(r3)
	rlwinm	r4,rSX,24,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM

	// push A
	GET_S(r3)
	rlwinm	r4,rPCAB,24,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM

	// push B
	GET_S(r3)
	rlwinm	r4,rPCAB,0,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM

	// push CC
	GET_S(r3)
	rlwinm	r4,rFlags,0,24,31
	subis	rSX,rSX,1
	bl		WRITEMEM
	bl		postExtern
	SAVE_SX

	_asm_get_global_b(r0,sInterruptState)
	CYCLES(9,9,9)
	ori		r0,r0,kInterruptStateWAI			// set the WAI bit
	_asm_set_global_b(r0,sInterruptState)
	bl		checkIRQLines						// check for interrupts
	_asm_get_global_b(r0,sInterruptState)
	andi.	r0,r0,kInterruptStateWAI			// see if the WAI bit is still on
	cmpwi	cr1,rICount,0
	beq		executeLoopEnd
	ble		cr1,executeLoopEnd
	add		rCycleCount,rCycleCount,rICount		// else add all the remaining cycles to the icount
	b		executeLoopEnd

	//================================================================================================

	//
	//		DAA: adjust accumulator
	//

	//
	//		DAA
	//
entry static daa
	GET_A(r4)
	rlwinm	r5,rPCAB,24,28,31					// r5 = lsn = A & 0xf
	rlwinm	r6,rPCAB,20,28,31					// r6 = msn = A >> 4
	cmpwi	cr1,r5,9							// compare lsn to 9
	andi.	r0,rFlags,k6800FlagH				// test the H flag
	cmpwi	cr2,r6,9							// compare msn to 9
	rlwinm	rFlags,rFlags,0,31,29 				// V = 0
	bgt		cr1,daa_add06						// if (lsn > 9), we need to add 6
	beq		daa_dontadd06						// if (lsn < 9 && !(cc & k6800FlagH)), skip
daa_add06:
	addi	r4,r4,0x06							// r4 += 0x06
daa_dontadd06:
	andi.	r0,rFlags,k6800FlagC				// test the C flag
	blt		cr2,daa_checkc						// if msn < 9, check the C flag
	bgt		cr2,daa_add60						// if msn > 9, add
	ble		cr1,daa_checkc						// if (msn == 9 && lsn <= 9), the C flag is our only hope
daa_add60:
	addi	r4,r4,0x60							// r4 += 0x60
	b		daa_finish							// skip to the finish
daa_checkc:
	bne		daa_add60							// if (cc & k6800FlagC), add anyhow
daa_finish:
	rlwinm	r7,r4,0,24,31						// r7 = r4 & 0xff
	rlwinm	r3,r4,24,31,31						// r3 = C bit
	cntlzw	r7,r7								// r7 = number of zeros in r7
	rlwimi	rFlags,r4,28,28,28					// N = (A & 0x80)
	SET_A(r4)									// A = r4
	rlwimi	rFlags,r7,29,29,29					// Z = (A == 0)
	CYCLES(2,2,2)
	or		rFlags,rFlags,r3					// C |= (r4 & 0x100)
	b		executeLoopEnd

	//================================================================================================

	//
	//		NOP: no-op
	//

	//
	//		NOP
	//
entry static nop
	CYCLES(2,2,1)
	b		executeLoopEnd

	//================================================================================================

	//
	//		XGDX: special to the HD63701YO
	//
#if (A6800_CHIP == 63701)

entry static xgdx
	GET_X(r3)
	GET_D(r4)
	SET_D(r3)
	SET_X(r4)
	CYCLES(0,0,2)
	b		executeLoopEnd

	//
	//		UNDOC1 (HD63701YO only)
	//
entry static undoc1
    CYCLES(0,0,0)
    GET_S(r3)
    addi	r3,r3,1
    SAVE_FLAGS
    bl		READMEM
    GET_X(r4) 									// r4 = X
    add		r4,r3,r4							// r4 = result = r3 + r4
    SET_X(r4)									// X = r4
    SAVE_SX
    bl		postExtern
    b		executeLoopEnd

	//
	//		UNDOC2 (HD63701YO only)
	//
entry static undoc2
    CYCLES(0,0,0)
    GET_S(r3)
    addi	r3,r3,1
    SAVE_FLAGS
    bl		READMEM
    GET_X(r4)									// r4 = X
    add		r4,r3,r4							// r4 = result = r3 + r4
    SET_X(r4)									// X = r4
    SAVE_SX
    bl		postExtern
    b		executeLoopEnd
#endif
}
