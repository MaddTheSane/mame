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
// Undefine this to use the slower C Konami core
#define PPC_KONAMI
#endif


//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6809 core
#define A6809_CHIP					5000
#define A6809_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6809_DEBUGHOOK				MAME_Debug
#endif

#define A6809_UPDATEBANK			MAMEKonamiUpdateBank
#define A6809_READMEM				program_read_byte_8
#define A6809_WRITEMEM				program_write_byte_8
#define A6809_OPCODEROM				opcode_base
#define A6809_ARGUMENTROM			opcode_arg_base
#define A6809_ICOUNT				konami_ICount
#define A6809_SETLINES				konami_cpu_setlines_callback
#define A6809_VOLATILEREGS			1


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "cpuintrf.h"


#ifndef PPC_KONAMI

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	#include "cpu/konami/konami.h"
	#include "Asgard6809Debugger.h"

	#if A6809_COREDEBUG
		#undef KONAMI_RDMEM
		#undef KONAMI_WRMEM
		#define KONAMI_RDMEM(A)    Asgard6809DebugRead(A)
		#define KONAMI_WRMEM(A,V)  Asgard6809DebugWrite(A,V)
	#endif
	
	#include "cpu/konami/konami.c"

	/*---------------------------------------------------------------------------

	To debug, insert these lines in m6809.c right after the #ifdef MAME_DEBUG:

	#if A6809_COREDEBUG
		Asgard6809MiniTrace ((xreg<<16)+yreg, (sreg<<16)+ureg, (pcreg<<16)+(areg<<8)+breg, (dpreg<<8)+cc, konami_ICount);
	#endif

	---------------------------------------------------------------------------*/

	#if A6809_COREDEBUG
		#undef DIRECT
		#undef EXTENDED
		#include "Asgard6809Debugger.c"
	#endif

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	// #define Get/SetRegs away so our definitions don't conflict
	#include "konami.h"

	// redefine some of our functions and variables to map to MAME's
	#define Asgard6809Init				konami_init
	#define Asgard6809Reset				konami_reset_real
	#define Asgard6809SetContext		konami_set_context
	#define Asgard6809SetReg			konami_set_reg
	#define Asgard6809GetContext		konami_get_context
	#define Asgard6809GetReg			konami_get_reg
	#define Asgard6809SetIRQLine		konami_set_irq_line
	#define Asgard6809SetIRQCallback	konami_set_irq_callback
	#define Asgard6809GetIRQCallback	konami_get_irq_callback
	#define Asgard6809Execute			konami_execute
	
	#include "Asgard6809.h"
	#include "Asgard6809Core.c"
	#include "Asgard6809Debugger.c"

	int konami_ICount;
	void (*konami_cpu_setlines_callback)(int);

	// this function is called on any jump of significance
	void MAMEKonamiUpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	// this reset function verifies that our structure matches MAME's
	void konami_reset(void *)
	{
		// standard reset
		konami_reset_real();
	}
	
	void konami_exit(void)
	{
	}
	
	static offs_t konami_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
	{
	#ifdef MAME_DEBUG
	    return Dasmknmi(buffer, pc, oprom, opram, bytes);
	#else
		sprintf( buffer, "$%02X", cpu_readop(pc) );
		return 1;
	#endif
	}

	static void konami_set_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + KONAMI_IRQ_LINE:	konami_set_irq_line(KONAMI_IRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + KONAMI_FIRQ_LINE:	konami_set_irq_line(KONAMI_FIRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		konami_set_irq_line(INPUT_LINE_NMI, info->i);		break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + KONAMI_PC:			konami_set_reg (k6809RegisterIndexPC, info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + KONAMI_S:			konami_set_reg (k6809RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_CC:			konami_set_reg (k6809RegisterIndexCC, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_U:			konami_set_reg (k6809RegisterIndexU, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_A:			konami_set_reg (k6809RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_B:			konami_set_reg (k6809RegisterIndexB, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_X:			konami_set_reg (k6809RegisterIndexX, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_Y:			konami_set_reg (k6809RegisterIndexY, info->i);	break;
			case CPUINFO_INT_REGISTER + KONAMI_DP:			konami_set_reg (k6809RegisterIndexDP, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					konami_set_irq_callback (info->irqcallback);	break;
			case CPUINFO_PTR_KONAMI_SETLINES_CALLBACK:		konami_cpu_setlines_callback = (void (*)(int))info->p; break;
		}
	}

	static UINT8 konami_reg_layout[] =
	{
		KONAMI_PC, KONAMI_S, KONAMI_CC, KONAMI_A, KONAMI_B, KONAMI_X, -1,
		KONAMI_Y, KONAMI_U, KONAMI_DP, 0
	};

	// Layout of the debugger windows x,y,w,h
	static UINT8 konami_win_layout[] =
	{
		27, 0,53, 4,	// register window (top, right rows)
		 0, 0,26,22,	// disassembler window (left colums)
		27, 5,53, 8,	// memory #1 window (right, upper middle)
		27,14,53, 8,	// memory #2 window (right, lower middle)
		 0,23,80, 1,	// command line window (bottom rows)
	};

	void konami_get_info(UINT32 state, union cpuinfo *info)
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

			case CPUINFO_INT_INPUT_STATE + KONAMI_IRQ_LINE:	info->i = konami_get_reg(k6809RegisterIndexIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + KONAMI_FIRQ_LINE:	info->i = konami_get_reg(k6809RegisterIndexFIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = konami_get_reg(k6809RegisterIndexNMIState);	break;

			case CPUINFO_INT_PREVIOUSPC:					info->i = konami_get_reg(k6809RegisterIndexOpcodePC);	break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + KONAMI_PC:			info->i = konami_get_reg(k6809RegisterIndexPC);			break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + KONAMI_S:			info->i = konami_get_reg(k6809RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + KONAMI_CC:			info->i = konami_get_reg(k6809RegisterIndexCC);			break;
			case CPUINFO_INT_REGISTER + KONAMI_U:			info->i = konami_get_reg(k6809RegisterIndexU);			break;
			case CPUINFO_INT_REGISTER + KONAMI_A:			info->i = konami_get_reg(k6809RegisterIndexA);			break;
			case CPUINFO_INT_REGISTER + KONAMI_B:			info->i = konami_get_reg(k6809RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + KONAMI_X:			info->i = konami_get_reg(k6809RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + KONAMI_Y:			info->i = konami_get_reg(k6809RegisterIndexY);			break;
			case CPUINFO_INT_REGISTER + KONAMI_DP:			info->i = konami_get_reg(k6809RegisterIndexDP);			break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = konami_set_info;				break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = konami_get_context;			break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = konami_set_context;			break;
			case CPUINFO_PTR_INIT:							info->init = konami_init;						break;
			case CPUINFO_PTR_RESET:							info->reset = konami_reset;						break;
			case CPUINFO_PTR_EXIT:							info->exit = konami_exit;						break;
			case CPUINFO_PTR_EXECUTE:						info->execute = konami_execute;					break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;								break;
//			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = konami_dasm;				break;
			case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = konami_dasm;			break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = konami_get_irq_callback();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &konami_ICount;					break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = konami_reg_layout;					break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = konami_win_layout;					break;
			case CPUINFO_PTR_KONAMI_SETLINES_CALLBACK:		info->p = (void *)konami_cpu_setlines_callback;	break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "KONAMI"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "KONAMI 5000x"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) Aaron Giles, 1997-1999"); break;

			case CPUINFO_STR_FLAGS:
			{
				int flags = konami_get_reg(k6809RegisterIndexCC);
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
			
			case CPUINFO_STR_REGISTER + KONAMI_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", konami_get_reg(k6809RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + KONAMI_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", konami_get_reg(k6809RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + KONAMI_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", konami_get_reg(k6809RegisterIndexCC)); break;
			case CPUINFO_STR_REGISTER + KONAMI_U:			sprintf(info->s = cpuintrf_temp_str(), "U:%04X", konami_get_reg(k6809RegisterIndexU)); break;
			case CPUINFO_STR_REGISTER + KONAMI_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", konami_get_reg(k6809RegisterIndexA)); break;
			case CPUINFO_STR_REGISTER + KONAMI_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", konami_get_reg(k6809RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + KONAMI_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", konami_get_reg(k6809RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + KONAMI_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%04X", konami_get_reg(k6809RegisterIndexY)); break;
			case CPUINFO_STR_REGISTER + KONAMI_DP:			sprintf(info->s = cpuintrf_temp_str(), "DP:%02X", konami_get_reg(k6809RegisterIndexDP)); break;
		}
	}

#endif