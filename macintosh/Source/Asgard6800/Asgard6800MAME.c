//###################################################################################################
//
//
//		Asgard6800.c
//		Interface file to make Asgard6800 compile as a 6800/6802/6808 for MAME.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C 6800 core
#define PPC_M6800
#endif

//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6800 core
#define A6800_CHIP					6800
#define A6800_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6800_DEBUGHOOK				MAME_Debug
#endif

#define A6800_UPDATEBANK			MAME6800UpdateBank
#define A6800_READMEM				program_read_byte_8
#define A6800_WRITEMEM				program_write_byte_8
#define A6800_READPORT				io_read_byte_8
#define A6800_WRITEPORT				io_write_byte_8
#define A6800_OPCODEROM				opcode_base
#define A6800_ARGUMENTROM			opcode_arg_base
#define A6800_ICOUNT				m6800_ICount_ppc
#define A6800_VOLATILEREGS			1

Boolean using6800AsmCores = true;

//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "cpuintrf.h"

#ifndef PPC_M6800

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	#include "cpu/m6800/m6800.h"
	#include "Asgard6800Debugger.h"
	
	#include <string.h>

#ifdef __MWERKS__
	#pragma global_optimizer off
	#pragma peephole off
	#pragma auto_inline off
	#pragma dont_inline on
	#pragma scheduling off
#endif
	
	#if A6800_COREDEBUG
		#undef M6800_RDMEM
		#undef M6800_WRMEM
		#define M6800_RDMEM(A)    Asgard6800DebugRead(A)
		#define M6800_WRMEM(A,V)  Asgard6800DebugWrite(A,V)
	#endif
	
	#include "cpu/m6800/m6800.c"

	/*---------------------------------------------------------------------------

	To debug, insert these lines in m6800.c right after the #ifdef MAME_DEBUG:

	#if A6800_COREDEBUG
		Asgard6800MiniTrace((m6800.s.w.l<<16)+m6800.x.w.l, (m6800.pc.w.l<<16)+m6800.d.w.l, m6800.cc, m6800_ICount, m6800.counter, m6800.output_compare);
	#endif

	---------------------------------------------------------------------------*/

	#if A6800_COREDEBUG
		#undef DIRECT
		#undef INDEXED
		#undef EXTENDED
		#include "Asgard6800Debugger.c"
	#endif

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	// #include the MAME definitions
	#include "cpu/m6800/m6800.h"

	#include "Asgard6800.h"
	#include "Asgard6800Core.c"
	#include "Asgard6800Debugger.c"
	
	int m6800_ICount_ppc;

	// this function is called on any jump of significance
	void MAME6800UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void m6800_reset_ppc(void *param)
	{
		Asgard6800Reset();
	}

	// this function cleans up after the core
	static void m6800_exit_ppc(void)
	{
	}
	
	static offs_t m6800_dasm_ppc(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG
	    return Dasm680x(6800,buffer,pc);
	#else
		sprintf( buffer, "$%02X", program_read_byte_8(pc) );
		return 1;
	#endif
	}

	static void m6800_set_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	Asgard6800SetIRQLine(M6800_IRQ_LINE, info->i);		break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	Asgard6800SetIRQLine(M6800_TIN_LINE, info->i);		break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	Asgard6800SetIRQLine(INPUT_LINE_NMI, info->i);		break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			Asgard6800SetReg (k6800RegisterIndexPC, info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			Asgard6800SetReg (k6800RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_CC:			Asgard6800SetReg (k6800RegisterIndexCC, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_A:			Asgard6800SetReg (k6800RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_B:			Asgard6800SetReg (k6800RegisterIndexB, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_X:			Asgard6800SetReg (k6800RegisterIndexX, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					Asgard6800SetIRQCallback (info->irqcallback);		break;
		}
	}

	// Layout of the registers in the debugger
	static UINT8 m6800_reg_layout_ppc[] =
	{
		M6800_PC, M6800_S, M6800_CC, M6800_A, M6800_B, M6800_X, -1,
		M6800_WAI_STATE, 0
	};

	// Layout of the debugger windows x,y,w,h
	static UINT8 m6800_win_layout_ppc[] =
	{
		27, 0,53, 4,	/* register window (top rows) */
		 0, 0,26,22,	/* disassembler window (left colums) */
		27, 5,53, 8,	/* memory #1 window (right, upper middle) */
		27,14,53, 8,	/* memory #2 window (right, lower middle) */
		 0,23,80, 1,	/* command line window (bottom rows) */
	};

	void m6800_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Asgard6800Context);	break;
			case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
			case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
			case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
			case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
			case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
			case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
			case CPUINFO_INT_MAX_CYCLES:					info->i = 12;							break;
			
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	info->i = Asgard6800GetReg(k6800RegisterIndexIRQState);		break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	info->i = Asgard6800GetReg(k6800RegisterIndexTINState);		break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = Asgard6800GetReg(k6800RegisterIndexNMIState);		break;

			case CPUINFO_INT_PREVIOUSPC: 					info->i = Asgard6800GetReg(k6800RegisterIndexOpcodePC);		break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			info->i = Asgard6800GetReg(k6800RegisterIndexPC);			break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			info->i = Asgard6800GetReg(k6800RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + M6800_CC:			info->i = Asgard6800GetReg(k6800RegisterIndexCC);			break;
			case CPUINFO_INT_REGISTER + M6800_A:			info->i = Asgard6800GetReg(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_B:			info->i = Asgard6800GetReg(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_X:			info->i = Asgard6800GetReg(k6800RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + M6800_WAI_STATE:	info->i = Asgard6800GetReg(k6800RegisterIndexWAIState);		break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = m6800_set_info_ppc;				break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = Asgard6800GetContext;		break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = Asgard6800SetContext;		break;
			case CPUINFO_PTR_INIT:							info->init = Asgard6800Init;					break;
			case CPUINFO_PTR_RESET:							info->reset = m6800_reset_ppc;					break;
			case CPUINFO_PTR_EXIT:							info->exit = m6800_exit_ppc;					break;
			case CPUINFO_PTR_EXECUTE:						info->execute = Asgard6800Execute;				break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;								break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6800_dasm_ppc;				break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = Asgard6800GetIRQCallback();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m6800_ICount_ppc;				break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m6800_reg_layout_ppc;					break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m6800_win_layout_ppc;					break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6800"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola 6800"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.1"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) Aaron Giles, 1997-1999"); break;

			case CPUINFO_STR_FLAGS:
			{
				Asgard6800Context r;
				Asgard6800GetContext (&r);
				sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
					(r.fFlags & 0x80) ? '?':'.',
					(r.fFlags & 0x40) ? '?':'.',
					(r.fFlags & 0x20) ? 'H':'.',
					(r.fFlags & 0x10) ? 'I':'.',
					(r.fFlags & 0x08) ? 'N':'.',
					(r.fFlags & 0x04) ? 'Z':'.',
					(r.fFlags & 0x02) ? 'V':'.',
					(r.fFlags & 0x01) ? 'C':'.');
				break;
			}

			case CPUINFO_STR_REGISTER + M6800_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", Asgard6800GetReg(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", Asgard6800GetReg(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", Asgard6800GetReg(k6800RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M6800_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", Asgard6800GetReg(k6800RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + M6800_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", Asgard6800GetReg(k6800RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + M6800_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", Asgard6800GetReg(k6800RegisterIndexCC)); break;
			case CPUINFO_STR_REGISTER + M6800_WAI_STATE:	sprintf(info->s = cpuintrf_temp_str(), "WAI:%X", Asgard6800GetReg(k6800RegisterIndexWAIState)); break;
		}
	}

	void m6800_get_info(UINT32 state, union cpuinfo *info)
	{
		if (using6800AsmCores)
			m6800_get_info_ppc (state, info);
		else
			m6800_get_info_c (state, info);
	}
	
	/****************************************************************************
	 * M6802 almost (fully?) equal to the M6800
	 ****************************************************************************/
	static offs_t m6802_dasm_ppc(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG
	    return Dasm680x(6802,buffer,pc);
	#else
		sprintf( buffer, "$%02X", program_read_byte_8(pc) );
		return 1;
	#endif
	}

	void m6802_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6802_dasm_ppc;			break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6802"); break;

			default:
				m6800_get_info_ppc(state, info);
				break;
		}
	}

	void m6802_get_info(UINT32 state, union cpuinfo *info)
	{
		if (using6800AsmCores)
			m6802_get_info_ppc (state, info);
		else
			m6802_get_info_c (state, info);
	}
	
	/****************************************************************************
	 * M6808 almost (fully?) equal to the M6800
	 ****************************************************************************/
	static offs_t m6808_dasm_ppc(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG 
		return Dasm680x(6808,buffer,pc); 
	#else 
		sprintf( buffer, "$%02X", program_read_byte_8(pc) ); 
		return 1; 
	#endif 
	} 

	void m6808_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6808_dasm_ppc;			break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6808"); break;

			default:
				m6800_get_info_ppc(state, info);
				break;
		}
	}

	void m6808_get_info(UINT32 state, union cpuinfo *info)
	{
		if (using6800AsmCores)
			m6808_get_info_ppc (state, info);
		else
			m6808_get_info_c (state, info);
	}
	
#endif
