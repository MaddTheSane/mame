//###################################################################################################
//
//
//		Asgard6502MAME.c
//		Interface file to make Asgard6502 work with MAME.
//
//		See Asgard6502Core.c for information about the Asgard6502 system.
//
//
//###################################################################################################

// Undefine this to use the slower C 6502 core
#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
#define PPC_6502
#endif

//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6502 core
#define A6502_CHIP					6502
#define A6502_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6502_DEBUGHOOK				MAME_Debug
#endif

#define A6502_UPDATEBANK			MAME6502UpdateBank
#define A6502_READMEM				program_read_byte_8
#define A6502_WRITEMEM				program_write_byte_8
#define A6502_OPCODEROM				opcode_base
#define A6502_ARGUMENTROM			opcode_arg_base
#define A6502_ICOUNT				m6502_ICount
#define A6502_VOLATILEREGS			1


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "cpuintrf.h"
#include "memory.h"

#ifndef PPC_6502

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	#include "m6502.h"
	#include "Asgard6502Debugger.h"

	#if A6502_COREDEBUG
		#define cpu_readmem16 Asgard6502DebugRead
		#define cpu_writemem16 Asgard6502DebugWrite
	#endif
	
	#include "cpu/m6502/m6502.c"

	#if A6502_COREDEBUG
		#undef cpu_readmem16
		#undef cpu_writemem16
	#endif

	/*---------------------------------------------------------------------------

	To debug, insert these lines in m6502.c right after the CALL_MAME_DEBUG:

	#if A6502_COREDEBUG
		Asgard6502MiniTrace((PCW<<16)+P, (S<<24)+(A<<16)+(X<<8)+Y, m6502_ICount);
	#endif
	
	---------------------------------------------------------------------------*/

	#if A6502_COREDEBUG
		#include "Asgard6502Debugger.c"
	#endif

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	// #define Get/SetRegs away so our definitions don't conflict
	#include "m6502.h"

	// redefine some of our functions and variables to map to MAME's
	#define Asgard6502Init				m6502_init_real
	#define Asgard6502SetContext		m6502_set_context
	#define Asgard6502SetReg			m6502_set_reg
	#define Asgard6502GetContext		m6502_get_context
	#define Asgard6502GetReg			m6502_get_reg
	#define Asgard6502SetIRQLine		m6502_set_irq_line
	#define Asgard6502SetIRQCallback	m6502_set_irq_callback
	#define Asgard6502GetIRQCallback	m6502_get_irq_callback
	#define Asgard6502Execute			m6502_execute
	#define Asgard6502Reset				m6502_reset_real
	
	#include "Asgard6502Core.h"
	#include "Asgard6502Core.c"
	#include "Asgard6502Debugger.c"
	
	int m6502_ICount;

	// this function is called on any jump of significance
	void MAME6502UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void m6502_init(void)
	{
		m6502_init_real();
		m6502_set_reg(k6502RegisterIndexSubType, SUBTYPE_6502);
	}

	static void m6502_reset(void *)
	{
		// standard reset
		m6502_reset_real();
	}

	// this function cleans up after the core
	static void m6502_exit(void)
	{
	}
	
	// Layout of the registers in the debugger
	static UINT8 m6502_reg_layout[] =
	{
		M6502_PC, M6502_S, M6502_P, M6502_A, M6502_X, M6502_Y, -1,
		M6502_EA, M6502_ZP, 0
	};

	// Layout of the debugger windows x,y,w,h
	static UINT8 m6502_win_layout[] =
	{
		25, 0,55, 2,	// register window (top, right rows)
		 0, 0,24,22,	// disassembler window (left colums)
		25, 3,55, 9,	// memory #1 window (right, upper middle)
		25,13,55, 9,	// memory #2 window (right, lower middle)
		 0,23,80, 1,	// command line window (bottom rows)
	};

	static void m6502_set_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + M6502_IRQ_LINE:	m6502_set_irq_line(M6502_IRQ_LINE, info->i); break;
			case CPUINFO_INT_INPUT_STATE + M6502_SET_OVERFLOW:m6502_set_irq_line(M6502_SET_OVERFLOW, info->i); break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		m6502_set_irq_line(INPUT_LINE_NMI, info->i); break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6502_PC:			m6502_set_reg (k6502RegisterIndexPC, info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6502_S:			m6502_set_reg (k6502RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + M6502_P:			m6502_set_reg (k6502RegisterIndexP, info->i);	break;
			case CPUINFO_INT_REGISTER + M6502_A:			m6502_set_reg (k6502RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6502_X:			m6502_set_reg (k6502RegisterIndexX, info->i);	break;
			case CPUINFO_INT_REGISTER + M6502_Y:			m6502_set_reg (k6502RegisterIndexY, info->i);	break;
			case CPUINFO_INT_REGISTER + M6502_EA:			m6502_set_reg (k6502RegisterIndexEA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6502_ZP:			m6502_set_reg (k6502RegisterIndexZP, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					m6502_set_irq_callback(info->irqcallback);	break;
		}
	}

	void m6502_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Asgard6502Context);	break;
			case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
			case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
			case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
			case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
			case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 3;							break;
			case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
			case CPUINFO_INT_MAX_CYCLES:					info->i = 10;							break;
			
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

			case CPUINFO_INT_INPUT_STATE + M6502_IRQ_LINE:	info->i = m6502_get_reg (k6502RegisterIndexIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + M6502_SET_OVERFLOW:info->i = m6502_get_reg (k6502RegisterIndexSOState);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = m6502_get_reg (k6502RegisterIndexNMIState);	break;

			case CPUINFO_INT_PREVIOUSPC:					info->i = m6502_get_reg (k6502RegisterIndexOpcodePC);	break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6502_PC:			info->i = m6502_get_reg (k6502RegisterIndexPC);			break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6502_S:			info->i = m6502_get_reg (k6502RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + M6502_P:			info->i = m6502_get_reg (k6502RegisterIndexP);			break;
			case CPUINFO_INT_REGISTER + M6502_A:			info->i = m6502_get_reg (k6502RegisterIndexA);			break;
			case CPUINFO_INT_REGISTER + M6502_X:			info->i = m6502_get_reg (k6502RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + M6502_Y:			info->i = m6502_get_reg (k6502RegisterIndexY);			break;
			case CPUINFO_INT_REGISTER + M6502_EA:			info->i = m6502_get_reg (k6502RegisterIndexEA);			break;
			case CPUINFO_INT_REGISTER + M6502_ZP:			info->i = m6502_get_reg (k6502RegisterIndexZP);			break;
			case CPUINFO_INT_REGISTER + M6502_SUBTYPE:		info->i = m6502_get_reg (k6502RegisterIndexSubType);	break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = m6502_set_info;			break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m6502_get_context;	break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m6502_set_context;	break;
			case CPUINFO_PTR_INIT:							info->init = m6502_init;				break;
			case CPUINFO_PTR_RESET:							info->reset = m6502_reset;				break;
			case CPUINFO_PTR_EXIT:							info->exit = m6502_exit;				break;
			case CPUINFO_PTR_EXECUTE:						info->execute = m6502_execute;			break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
			case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = m6502_dasm;		break;
#endif
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = m6502_get_irq_callback();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m6502_ICount;			break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m6502_reg_layout;				break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m6502_win_layout;				break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6502"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Mostek 6502"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.2"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) 2000 Aaron Giles, all rights reserved."); break;

			case CPUINFO_STR_FLAGS:
			{
				int flags = m6502_get_reg(k6502RegisterIndexP);
				sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
					flags & 0x80 ? 'N':'.',
					flags & 0x40 ? 'V':'.',
					flags & 0x20 ? 'R':'.',
					flags & 0x10 ? 'B':'.',
					flags & 0x08 ? 'D':'.',
					flags & 0x04 ? 'I':'.',
					flags & 0x02 ? 'Z':'.',
					flags & 0x01 ? 'C':'.');
				break;
			}

			case CPUINFO_STR_REGISTER + M6502_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m6502_get_reg (k6502RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M6502_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%02X", m6502_get_reg (k6502RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + M6502_P:			sprintf(info->s = cpuintrf_temp_str(), "P:%02X", m6502_get_reg (k6502RegisterIndexP)); break;
			case CPUINFO_STR_REGISTER + M6502_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m6502_get_reg (k6502RegisterIndexA)); break;
			case CPUINFO_STR_REGISTER + M6502_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%02X", m6502_get_reg (k6502RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + M6502_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%02X", m6502_get_reg (k6502RegisterIndexY)); break;
			case CPUINFO_STR_REGISTER + M6502_EA:			sprintf(info->s = cpuintrf_temp_str(), "EA:%04X", m6502_get_reg (k6502RegisterIndexEA)); break;
			case CPUINFO_STR_REGISTER + M6502_ZP:			sprintf(info->s = cpuintrf_temp_str(), "ZP:%03X", m6502_get_reg (k6502RegisterIndexZP)); break;
		}
	}

#endif