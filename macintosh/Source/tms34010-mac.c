#if defined (MAME_DEBUG) || defined(MAC_XCODE)
#define USE_MAC_ASM 0
#else
#define USE_MAC_ASM 0
#endif

#if USE_MAC_ASM
#define tms34010_reset _tms34010_reset
#define tms34010_execute _tms34010_execute

#define tms34020_reset _tms34020_reset
#define tms34020_execute _tms34020_execute
#endif

#include "cpu/tms34010/tms34010.c"

#if USE_MAC_ASM
#undef tms34010_execute
#undef tms34010_reset
#undef tms34020_execute
#undef tms34020_reset

static asm void execute_core(void);

void tms34010_reset(void *param)
{
#if TARGET_RT_MAC_CFM
	static int done = 0;
	if (!done)
	{
		int i;
		for (i = 0; i < (0x10000 >> 4); i++)
			opcode_table[i] = *(void **)opcode_table[i];
		done = 1;
	}
#endif
	_tms34010_reset(param);
}

int tms34010_execute(int cycles)
{
	/* Get out if CPU is halted. Absolutely no interrupts must be taken!!! */
	if (IOREG(REG_HSTCTLH) & 0x8000)
		return cycles;

	/* if the CPU's reset was deferred, do it now */
	if (state.reset_deferred)
	{
		state.reset_deferred = 0;
		PC = RLONG(0xffffffe0);
	}

	/* prepare to run */
	tms34010_ICount = cycles;
	change_pc(TOBYTE(PC));

	/* do it */
	execute_core();
	
	return cycles - tms34010_ICount;
}

void tms34020_reset(void *param)
{
	tms34010_reset(param);
	state.is_34020 = 1;
}

// Should be identical to tms34010_execute
int tms34020_execute(int cycles)
{
#if 1
	return tms34010_execute(cycles);
#else
	/* Get out if CPU is halted. Absolutely no interrupts must be taken!!! */
	if (IOREG(REG_HSTCTLH) & 0x8000)
		return cycles;

	/* if the CPU's reset was deferred, do it now */
	if (state.reset_deferred)
	{
		state.reset_deferred = 0;
		PC = RLONG(0xffffffe0);
	}

	/* prepare to run */
	tms34010_ICount = cycles;
	cpu_executing = cpu_getactivecpu();
	change_pc(TOBYTE(PC));

	/* do it */
	execute_core();
	
	cpu_executing = -1;

	return cycles - tms34010_ICount;
#endif
}

static asm void execute_core(void)
{
#define rROM		r28
#define rICount		r29
#define rState		r30
#define rOpTable	r31

#if (__MWERKS__ >= 0x2300)
	nofralloc
#endif

	mflr	r0
	stmw	r28,-16(sp)
	stw		r0,8(sp)
	stwu	sp,-96(sp)
	
	_asm_get_global_ptr(rROM,OP_ROM)
	_asm_get_global_ptr(rICount,tms34010_ICount)
	_asm_get_global_ptr(rState,state)
	_asm_get_global_ptr(rOpTable,opcode_table)

MainLoop:
	lwz		r3,tms34010_regs.pc(rState)			// r3 = current PC
	lwz		r6,0(rROM)							// r6 = ROM base
	rlwinm	r5,r3,32-3,3,31						// r5 = PC >> 3
	lhzx	r7,r6,r5							// r7 = opcode
	addi	r4,r3,0x10							// r4 = PC + 10
	rlwinm	r8,r7,32-2,18,29					// r8 = (opcode >> 4) << 2
	lwzx	r9,rOpTable,r8						// r9 = opcode handler address
	stw		r4,tms34010_regs.pc(rState)			// save new PC
	mtctr	r9									// store to counter
	stw		r7,tms34010_regs.op(rState)			// save opcode
	bctrl
	
	lwz		r3,tms34010_regs.pc(rState)			// r3 = current PC
	lwz		r6,0(rROM)							// r6 = ROM base
	rlwinm	r5,r3,32-3,3,31						// r5 = PC >> 3
	lhzx	r7,r6,r5							// r7 = opcode
	addi	r4,r3,0x10							// r4 = PC + 10
	rlwinm	r8,r7,32-2,18,29					// r8 = (opcode >> 4) << 2
	lwzx	r9,rOpTable,r8						// r9 = opcode handler address
	stw		r4,tms34010_regs.pc(rState)			// save new PC
	mtctr	r9									// store to counter
	stw		r7,tms34010_regs.op(rState)			// save opcode
	bctrl
	
	lwz		r3,tms34010_regs.pc(rState)			// r3 = current PC
	lwz		r6,0(rROM)							// r6 = ROM base
	rlwinm	r5,r3,32-3,3,31						// r5 = PC >> 3
	lhzx	r7,r6,r5							// r7 = opcode
	addi	r4,r3,0x10							// r4 = PC + 10
	rlwinm	r8,r7,32-2,18,29					// r8 = (opcode >> 4) << 2
	lwzx	r9,rOpTable,r8						// r9 = opcode handler address
	stw		r4,tms34010_regs.pc(rState)			// save new PC
	mtctr	r9									// store to counter
	stw		r7,tms34010_regs.op(rState)			// save opcode
	bctrl
	
	lwz		r3,tms34010_regs.pc(rState)			// r3 = current PC
	lwz		r6,0(rROM)							// r6 = ROM base
	rlwinm	r5,r3,32-3,3,31						// r5 = PC >> 3
	lhzx	r7,r6,r5							// r7 = opcode
	addi	r4,r3,0x10							// r4 = PC + 10
	rlwinm	r8,r7,32-2,18,29					// r8 = (opcode >> 4) << 2
	lwzx	r9,rOpTable,r8						// r9 = opcode handler address
	stw		r4,tms34010_regs.pc(rState)			// save new PC
	mtctr	r9									// store to counter
	stw		r7,tms34010_regs.op(rState)			// save opcode
	bctrl
	
	lwz		r3,0(rICount)						// r3 = ICount
	cmpwi	r3,0								// done?
	bgt		MainLoop							// if not, loop
	
	lwz		r0,104(sp)
	addi	sp,sp,96
	mtlr	r0
	lmw		r28,-16(sp)
	blr
}
#endif
