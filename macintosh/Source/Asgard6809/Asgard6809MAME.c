//###################################################################################################
//
//
//		Asgard6809MAME.c
//		Interface file to make Asgard6809 work with MAME.
//
//		See Asgard6809Core.c for information about the Asgard6809 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C 6809 core
#define PPC_M6809
#ifndef MESS
#define PPC_HD6309
#endif
#endif


//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6809 core
#define A6809_CHIP					6809
#define A6809_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6809_DEBUGHOOK				MAME_Debug
#endif

#define A6809_UPDATEBANK			MAME6809UpdateBank
#define A6809_READMEM				program_read_byte_8
#define A6809_WRITEMEM				program_write_byte_8
#define A6809_OPCODEROM				opcode_base
#define A6809_ARGUMENTROM			opcode_arg_base
#define A6809_ICOUNT				m6809_ICount_ppc
#define A6809_VOLATILEREGS			1


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "cpuintrf.h"

#ifndef PPC_M6809

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	#include "cpu/m6809/m6809.h"
	#include "Asgard6809Debugger.h"

	#if A6809_COREDEBUG
		#undef M6809_RDMEM
		#undef M6809_WRMEM
		#define M6809_RDMEM(A)    Asgard6809DebugRead(A)
		#define M6809_WRMEM(A,V)  Asgard6809DebugWrite(A,V)
	#endif
	
	#include "cpu/m6809/m6809.c"

	/*---------------------------------------------------------------------------

	To debug, insert these lines in m6809.c right after the #ifdef MAME_DEBUG:

	#if A6809_COREDEBUG
		Asgard6809MiniTrace ((xreg<<16)+yreg, (sreg<<16)+ureg, (pcreg<<16)+(areg<<8)+breg, (dpreg<<8)+cc, m6809_ICount);
	#endif

	---------------------------------------------------------------------------*/

	#if A6809_COREDEBUG
		#undef DIRECT
		#undef EXTENDED
		#include "Asgard6809Debugger.c"
	#endif

	/****************************************************************************
	 * HD6309 section
	 ****************************************************************************/
	void hd6309_init(void) { m6809_init(); }
	void hd6309_reset(void *param) { m6809_reset(param); }
	void hd6309_exit(void) { m6809_exit(); }
	int hd6309_execute(int cycles) { return m6809_execute(cycles); }
	void hd6309_get_context(void *dst) { m6809_get_context(dst); }
	void hd6309_set_context(void *src) { m6809_set_context(src); }
	unsigned hd6309_get_reg(int regnum) { return m6809_get_reg(regnum); }
	void hd6309_set_reg(int regnum, unsigned val) { m6809_set_reg(regnum,val); }
	void hd6309_set_irq_line(int irqline, int state) { m6809_set_irq_line(irqline,state); }
	void hd6309_set_irq_callback(int (*callback)(int irqline)) { m6809_set_irq_callback(callback); }
	void hd6309_state_save(void *file) { /*state_save(file, "hd6309");*/ }
	void hd6309_state_load(void *file) { /*state_load(file, "hd6309");*/ }
	void hd6309_get_info(UINT32 state, union cpuinfo *info)
	{
	    m6809_get_info(state,info);
	}

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

#ifdef __MWERKS__
	// LBO 5/4/05. We don't want the optimizer rescheduling the PPC assembly. It messes up the
	// parent set of Dragon Spirit.
	#pragma global_optimizer off
#endif

	// #define Get/SetRegs away so our definitions don't conflict
	#include "cpu/m6809/m6809.h"

	// redefine some of our functions and variables to map to MAME's
	#define Asgard6809Init				m6809_init_ppc
	#define Asgard6809Reset				m6809_reset_real_ppc
	#define Asgard6809SetContext		m6809_set_context_ppc
	#define Asgard6809SetReg			m6809_set_reg_ppc
	#define Asgard6809GetContext		m6809_get_context_ppc
	#define Asgard6809GetReg			m6809_get_reg_ppc
	#define Asgard6809SetIRQLine		m6809_set_irq_line_ppc
	#define Asgard6809SetIRQCallback	m6809_set_irq_callback_ppc
	#define Asgard6809GetIRQCallback	m6809_get_irq_callback_ppc
	#define Asgard6809Execute			m6809_execute_ppc
	
	#include "Asgard6809.h"
	#include "Asgard6809Core.c"
	#include "Asgard6809Debugger.c"

	int m6809_ICount_ppc;

	// this function is called on any jump of significance
	void MAME6809UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void m6809_reset_ppc(void *)
	{
		// standard reset
		m6809_reset_real_ppc();
	}
	
	static void m6809_exit_ppc(void)
	{
	}
	
	static void m6809_set_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + M6809_IRQ_LINE:	m6809_set_irq_line_ppc(M6809_IRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + M6809_FIRQ_LINE:	m6809_set_irq_line_ppc(M6809_FIRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	m6809_set_irq_line_ppc(INPUT_LINE_NMI, info->i);	break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6809_PC:			m6809_set_reg_ppc (k6809RegisterIndexPC, info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6809_S:			m6809_set_reg_ppc (k6809RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_CC:			m6809_set_reg_ppc (k6809RegisterIndexCC, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_U:			m6809_set_reg_ppc (k6809RegisterIndexU, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_A:			m6809_set_reg_ppc (k6809RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_B:			m6809_set_reg_ppc (k6809RegisterIndexB, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_X:			m6809_set_reg_ppc (k6809RegisterIndexX, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_Y:			m6809_set_reg_ppc (k6809RegisterIndexY, info->i);	break;
			case CPUINFO_INT_REGISTER + M6809_DP:			m6809_set_reg_ppc (k6809RegisterIndexDP, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					m6809_set_irq_callback_ppc (info->irqcallback);		break;
		}
	}

	static UINT8 m6809_reg_layout_ppc[] =
	{
		M6809_PC, M6809_S, M6809_CC, M6809_A, M6809_B, M6809_X, -1,
		M6809_Y, M6809_U, M6809_DP, 0
	};

	// Layout of the debugger windows x,y,w,h
	static UINT8 m6809_win_layout_ppc[] =
	{
		27, 0,53, 4,	// register window (top, right rows)
		 0, 0,26,22,	// disassembler window (left colums)
		27, 5,53, 8,	// memory #1 window (right, upper middle)
		27,14,53, 8,	// memory #2 window (right, lower middle)
		 0,23,80, 1,	// command line window (bottom rows)
	};

// еее LBO 12/27/05. The prototype in m6809.h is outdated and needs to be fixed.
extern offs_t m6809_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes);

	void m6809_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Asgard6809Context);	break;
			case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
			case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
			case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
			case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
			case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
			case CPUINFO_INT_MIN_CYCLES:					info->i = 2;							break;
			case CPUINFO_INT_MAX_CYCLES:					info->i = 19;							break;
			
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

			case CPUINFO_INT_INPUT_STATE + M6809_IRQ_LINE:	info->i = m6809_get_reg_ppc(k6809RegisterIndexIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + M6809_FIRQ_LINE:	info->i = m6809_get_reg_ppc(k6809RegisterIndexFIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = m6809_get_reg_ppc(k6809RegisterIndexNMIState);	break;

			case CPUINFO_INT_PREVIOUSPC: 					info->i = m6809_get_reg_ppc(k6809RegisterIndexOpcodePC);	break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6809_PC:			info->i = m6809_get_reg_ppc(k6809RegisterIndexPC);			break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6809_S:			info->i = m6809_get_reg_ppc(k6809RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + M6809_CC:			info->i = m6809_get_reg_ppc(k6809RegisterIndexCC);			break;
			case CPUINFO_INT_REGISTER + M6809_U:			info->i = m6809_get_reg_ppc(k6809RegisterIndexU);			break;
			case CPUINFO_INT_REGISTER + M6809_A:			info->i = m6809_get_reg_ppc(k6809RegisterIndexA);			break;
			case CPUINFO_INT_REGISTER + M6809_B:			info->i = m6809_get_reg_ppc(k6809RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6809_X:			info->i = m6809_get_reg_ppc(k6809RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + M6809_Y:			info->i = m6809_get_reg_ppc(k6809RegisterIndexY);			break;
			case CPUINFO_INT_REGISTER + M6809_DP:			info->i = m6809_get_reg_ppc(k6809RegisterIndexDP);			break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = m6809_set_info_ppc;					break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m6809_get_context_ppc;			break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m6809_set_context_ppc;			break;
			case CPUINFO_PTR_INIT:							info->init = m6809_init_ppc;						break;
			case CPUINFO_PTR_RESET:							info->reset = m6809_reset_ppc;						break;
			case CPUINFO_PTR_EXIT:							info->exit = m6809_exit_ppc;						break;
			case CPUINFO_PTR_EXECUTE:						info->execute = m6809_execute_ppc;					break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;									break;
#ifdef MAME_DEBUG
			case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = m6809_dasm;					break;
#endif /* MAME_DEBUG */
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = m6809_get_irq_callback_ppc();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m6809_ICount_ppc;					break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m6809_reg_layout_ppc;						break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m6809_win_layout_ppc;						break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6809"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola 6809"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.1"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) Aaron Giles, 1997-1999"); break;

			case CPUINFO_STR_FLAGS:
			{
				int flags = m6809_get_reg_ppc(k6809RegisterIndexCC);
				sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
					(flags & 0x80) ? 'E':'.',
					(flags & 0x40) ? 'F':'.',
					(flags & 0x20) ? 'H':'.',
					(flags & 0x10) ? 'I':'.',
					(flags & 0x08) ? 'N':'.',
					(flags & 0x04) ? 'Z':'.',
					(flags & 0x02) ? 'V':'.',
					(flags & 0x01) ? 'C':'.');
	            break;
			}
			
			case CPUINFO_STR_REGISTER + M6809_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m6809_get_reg_ppc(k6809RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M6809_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", m6809_get_reg_ppc(k6809RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + M6809_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", m6809_get_reg_ppc(k6809RegisterIndexCC)); break;
			case CPUINFO_STR_REGISTER + M6809_U:			sprintf(info->s = cpuintrf_temp_str(), "U:%04X", m6809_get_reg_ppc(k6809RegisterIndexU)); break;
			case CPUINFO_STR_REGISTER + M6809_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m6809_get_reg_ppc(k6809RegisterIndexA)); break;
			case CPUINFO_STR_REGISTER + M6809_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", m6809_get_reg_ppc(k6809RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6809_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", m6809_get_reg_ppc(k6809RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + M6809_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%04X", m6809_get_reg_ppc(k6809RegisterIndexY)); break;
			case CPUINFO_STR_REGISTER + M6809_DP:			sprintf(info->s = cpuintrf_temp_str(), "DP:%02X", m6809_get_reg_ppc(k6809RegisterIndexDP)); break;
		}
	}

	void m6809e_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 4;							break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6809E"); break;

			default:
				m6809_get_info(state, info);
				break;
		}
	}

#ifdef PPC_HD6309
	/****************************************************************************
	 * HD6309 section
	 ****************************************************************************/
	int hd6309_ICount_ppc;

	static void hd6309_init_ppc(void) { m6809_init_ppc(); }

	void hd6309_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_INIT:							info->init = hd6309_init_ppc;					break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "HD6309"); break;

			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &hd6309_ICount_ppc;				break;

			default:
				m6809_get_info(state, info);
				break;
		}
	}
#endif

#endif