//###################################################################################################
//
//
//		Asgard8085MAME.c
//		Interface file to make Asgard8080 work with MAME.
//
//		See Asgard8080Core.c for information about the Asgard8080 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C 8085 core

// Blasto (in the blockade.c driver) doesn't work with the PPC 8080 core. 1/2/06 LBO
//#define PPC_8085
#endif


//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 8085 core
#define A8080_CHIP					8085
#define A8080_COREDEBUG				0

#ifdef MAME_DEBUG
#define A8080_DEBUGHOOK				MAME_Debug
#endif

#define A8080_UPDATEBANK			MAME8085UpdateBank
#define A8080_READMEM				program_read_byte_8
#define A8080_WRITEMEM				program_write_byte_8
#define A8080_READPORT				io_read_byte_8
#define A8080_WRITEPORT				io_write_byte_8
#define A8080_OPCODEROM				opcode_base
#define A8080_ARGUMENTROM			opcode_arg_base
#define A8080_ICOUNT				i8085_ICount
#define A8080_VOLATILEREGS			1


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "cpuintrf.h"
#include "memory.h"


#ifndef PPC_8085

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	// the C glue builds both cores (8080/8085), so we only need to include it in one
	// I've put it into the 8080 for now

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	// #define Get/SetRegs away so our definitions don't conflict
	#include "i8085.h"

	// redefine some of our functions and variables to map to MAME's
	#define Asgard8080Init				i8085_init
	#define Asgard8080SetContext		i8085_set_context
	#define Asgard8080SetReg			i8085_set_reg
	#define Asgard8080GetContext		i8085_get_context
	#define Asgard8080GetReg			i8085_get_reg
	#define Asgard8080SetIRQLine		i8085_set_irq_line
	#define Asgard8080SetIRQCallback	i8085_set_irq_callback
	#define Asgard8080GetIRQCallback	i8085_get_irq_callback
	#define Asgard8080Execute			i8085_execute
	#define Asgard8080Reset				i8085_reset_real
	
	#include "Asgard8080Core.h"
	#include "Asgard8080Core.c"
	#include "Asgard8080Debugger.c"
	
	int i8085_ICount;

	// this function is called on any jump of significance
	void MAME8085UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void i8085_reset(void *)
	{
		// standard reset
		i8085_reset_real();
	}

	static void i8085x_get_context(void *dst)
	{
		i8085_get_context(dst);
	}

	// this function cleans up after the core
	static void i8085_exit(void)
	{
	}

	static offs_t i8085_dasm(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG
	    return Dasm8085(buffer,pc);
	#else
		sprintf( buffer, "$%02X", cpu_readop(pc) );
		return 1;
	#endif
	}

	// Layout of the registers in the debugger
	static UINT8 i8085_reg_layout[] =
	{
		I8085_PC,I8085_SP,I8085_AF,I8085_BC,I8085_DE,I8085_HL, -1,
		I8085_HALT,I8085_IM,I8085_IREQ,I8085_ISRV,I8085_VECTOR,	0 };

	// Layout of the debugger windows x,y,w,h
	static UINT8 i8085_win_layout[] =
	{
		25, 0,55, 3,	// register window (top, right rows)
		 0, 0,24,22,	// disassembler window (left colums)
		25, 4,55, 9,	// memory #1 window (right, upper middle)
		25,14,55, 8,	// memory #2 window (right, lower middle)
		 0,23,80, 1,	// command line window (bottom rows)
	};

	static void i8085_set_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + I8085_INTR_LINE:	i8085_set_irq_line(k8080IRQLineINTR, info->i); break;
			case CPUINFO_INT_INPUT_STATE + I8085_RST55_LINE:	i8085_set_irq_line(k8080IRQLineRST55, info->i); break;
			case CPUINFO_INT_INPUT_STATE + I8085_RST65_LINE:	i8085_set_irq_line(k8080IRQLineRST65, info->i); break;
			case CPUINFO_INT_INPUT_STATE + I8085_RST75_LINE:	i8085_set_irq_line(k8080IRQLineRST75, info->i); break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		i8085_set_irq_line(INPUT_LINE_NMI, info->i); break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + I8085_PC:			i8085_set_reg(k8080RegisterIndexPC,info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + I8085_SP:			i8085_set_reg(k8080RegisterIndexSP,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_AF:			i8085_set_reg(k8080RegisterIndexAF,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_BC:			i8085_set_reg(k8080RegisterIndexBC,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_DE:			i8085_set_reg(k8080RegisterIndexDE,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_HL:			i8085_set_reg(k8080RegisterIndexHL,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_IM:			i8085_set_reg(k8080RegisterIndexIM,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_HALT:			i8085_set_reg(k8080RegisterIndexHALT,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_IREQ:			i8085_set_reg(k8080RegisterIndexIREQ,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_ISRV:			i8085_set_reg(k8080RegisterIndexISRV,info->i);	break;
			case CPUINFO_INT_REGISTER + I8085_VECTOR:		i8085_set_reg(k8080RegisterIndexVector,info->i);	break;

			case CPUINFO_INT_I8085_SID:						Asgard8080SetSIDLine(info->i); break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					Asgard8080SetIRQCallback(info->irqcallback);		break;
			case CPUINFO_PTR_I8085_SOD_CALLBACK:			Asgard8080SetSODCallback((Asgard8080SODCallback)info->p);break;
		}
	}

	void i8085_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = i8085_get_context(NULL);		break;
			case CPUINFO_INT_INPUT_LINES:					info->i = 4;							break;
			case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0xff;							break;
			case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
			case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
			case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 3;							break;
			case CPUINFO_INT_MIN_CYCLES:					info->i = 4;							break;
			case CPUINFO_INT_MAX_CYCLES:					info->i = 16;							break;
			
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

			case CPUINFO_INT_INPUT_STATE + I8085_INTR_LINE:	info->i = i8085_get_reg(k8080RegisterIndexINTRState); break;
			case CPUINFO_INT_INPUT_STATE + I8085_RST55_LINE:	info->i = i8085_get_reg(k8080RegisterIndexRST55State); break;
			case CPUINFO_INT_INPUT_STATE + I8085_RST65_LINE:	info->i = i8085_get_reg(k8080RegisterIndexRST65State); break;
			case CPUINFO_INT_INPUT_STATE + I8085_RST75_LINE:	info->i = i8085_get_reg(k8080RegisterIndexRST75State); break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = i8085_get_reg(k8080RegisterIndexTRAPState); break;

			case CPUINFO_INT_PREVIOUSPC:					info->i = i8085_get_reg(k8080RegisterIndexOpcodePC);	break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + I8085_PC:			info->i = i8085_get_reg(k8080RegisterIndexPC);		break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + I8085_SP:			info->i = i8085_get_reg(k8080RegisterIndexSP);		break;
			case CPUINFO_INT_REGISTER + I8085_AF:			info->i = i8085_get_reg(k8080RegisterIndexAF);		break;
			case CPUINFO_INT_REGISTER + I8085_BC:			info->i = i8085_get_reg(k8080RegisterIndexBC);		break;
			case CPUINFO_INT_REGISTER + I8085_DE:			info->i = i8085_get_reg(k8080RegisterIndexDE);		break;
			case CPUINFO_INT_REGISTER + I8085_HL:			info->i = i8085_get_reg(k8080RegisterIndexHL);		break;
			case CPUINFO_INT_REGISTER + I8085_IM:			info->i = i8085_get_reg(k8080RegisterIndexIM);		break;
			case CPUINFO_INT_REGISTER + I8085_HALT:			info->i = i8085_get_reg(k8080RegisterIndexHALT);	break;
			case CPUINFO_INT_REGISTER + I8085_IREQ:			info->i = i8085_get_reg(k8080RegisterIndexIREQ);	break;
			case CPUINFO_INT_REGISTER + I8085_ISRV:			info->i = i8085_get_reg(k8080RegisterIndexISRV);	break;
			case CPUINFO_INT_REGISTER + I8085_VECTOR:		info->i = i8085_get_reg(k8080RegisterIndexVector);	break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = i8085_set_info;			break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = i8085x_get_context;	break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = i8085_set_context;	break;
			case CPUINFO_PTR_INIT:							info->init = i8085_init;				break;
			case CPUINFO_PTR_RESET:							info->reset = i8085_reset;				break;
			case CPUINFO_PTR_EXIT:							info->exit = i8085_exit;				break;
			case CPUINFO_PTR_EXECUTE:						info->execute = i8085_execute;			break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = i8085_dasm;			break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = i8085_get_irq_callback();		break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &i8085_ICount;			break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i8085_reg_layout;				break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i8085_win_layout;				break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "8085A"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 8080"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.1"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) Aaron Giles, 1997-1999"); break;

			case CPUINFO_STR_FLAGS:
			{
				int flags = i8085_get_reg(k8080RegisterIndexAF);
				sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
					flags & 0x80 ? 'S':'.',
					flags & 0x40 ? 'Z':'.',
					flags & 0x20 ? '?':'.',
					flags & 0x10 ? 'H':'.',
					flags & 0x08 ? '?':'.',
					flags & 0x04 ? 'P':'.',
					flags & 0x02 ? 'N':'.',
					flags & 0x01 ? 'C':'.');
				break;
			}

			case CPUINFO_STR_REGISTER + I8085_AF:			sprintf(info->s = cpuintrf_temp_str(), "AF:%04X", i8085_get_reg(k8080RegisterIndexAF)); break;
			case CPUINFO_STR_REGISTER + I8085_BC:			sprintf(info->s = cpuintrf_temp_str(), "BC:%04X", i8085_get_reg(k8080RegisterIndexBC)); break;
			case CPUINFO_STR_REGISTER + I8085_DE:			sprintf(info->s = cpuintrf_temp_str(), "DE:%04X", i8085_get_reg(k8080RegisterIndexDE)); break;
			case CPUINFO_STR_REGISTER + I8085_HL:			sprintf(info->s = cpuintrf_temp_str(), "HL:%04X", i8085_get_reg(k8080RegisterIndexHL)); break;
			case CPUINFO_STR_REGISTER + I8085_SP:			sprintf(info->s = cpuintrf_temp_str(), "SP:%04X", i8085_get_reg(k8080RegisterIndexSP)); break;
			case CPUINFO_STR_REGISTER + I8085_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", i8085_get_reg(k8080RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + I8085_IM:			sprintf(info->s = cpuintrf_temp_str(), "IM:%02X", i8085_get_reg(k8080RegisterIndexIM)); break;
			case CPUINFO_STR_REGISTER + I8085_HALT:			sprintf(info->s = cpuintrf_temp_str(), "HALT:%d", i8085_get_reg(k8080RegisterIndexHALT)); break;
			case CPUINFO_STR_REGISTER + I8085_IREQ:			sprintf(info->s = cpuintrf_temp_str(), "IREQ:%02X", i8085_get_reg(k8080RegisterIndexIREQ)); break;
			case CPUINFO_STR_REGISTER + I8085_ISRV:			sprintf(info->s = cpuintrf_temp_str(), "ISRV:%02X", i8085_get_reg(k8080RegisterIndexISRV)); break;
			case CPUINFO_STR_REGISTER + I8085_VECTOR:		sprintf(info->s = cpuintrf_temp_str(), "VEC:%02X", i8085_get_reg(k8080RegisterIndexVector)); break;
		}
	}

#endif