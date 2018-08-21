//###################################################################################################
//
//
//		Asgard63701.c
//		Interface file to make Asgard6800 compile as a 8105 for MAME.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C 8105 core
#define PPC_NSC8105
#endif


//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6800 core
#define A6800_CHIP					8105
#define A6800_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6800_DEBUGHOOK				MAME_Debug
#endif

#define A6800_UPDATEBANK			MAME8105UpdateBank
#define A6800_READMEM				program_read_byte_8
#define A6800_WRITEMEM				program_write_byte_8
#define A6800_READPORT				io_read_byte_8
#define A6800_WRITEPORT				io_write_byte_8
#define A6800_OPCODEROM				opcode_base
#define A6800_ARGUMENTROM			opcode_arg_base
#define A6800_ICOUNT				nsc8105_ICount
#define A6800_VOLATILEREGS			1

extern Boolean using6800AsmCores;

//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "Asgard6800.h"
#include "cpuintrf.h"


#ifndef PPC_NSC8105

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	// the C glue builds all four cores (6800/6803/63701/8105), so we only need to include it in one
	// I've put it into the 6800 for now

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	// #include the MAME definitions
	#include "m6800.h"

	// redefine some of our functions and variables to map to MAME's
	#define Asgard6800Init				nsc8105_init
	#define Asgard6800Reset				nsc8105_reset_real
	#define Asgard6800SetContext		nsc8105_set_context
	#define Asgard6800SetReg			nsc8105_set_reg
	#define Asgard6800GetContext		nsc8105_get_context
	#define Asgard6800GetReg			nsc8105_get_reg
	#define Asgard6800SetIRQLine		nsc8105_set_irq_line
	#define Asgard6800SetIRQCallback	nsc8105_set_irq_callback
	#define Asgard6800GetIRQCallback	nsc8105_get_irq_callback
	#define Asgard6800Execute			nsc8105_execute
	
	#include "Asgard6800Core.c"
	#include "Asgard6800Debugger.c"

	int nsc8105_ICount;

	// this function is called on any jump of significance
	void MAME8105UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void nsc8105_reset(void *)
	{
		// standard reset
		nsc8105_reset_real();
	}

	static offs_t nsc8105_dasm(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG 
		return Dasm680x(8105,buffer,pc); 
	#else 
		sprintf( buffer, "$%02X", program_read_byte_8(pc) ); 
		return 1; 
	#endif 
	}

	static void nsc8105_set_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	nsc8105_set_irq_line(M6800_IRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	nsc8105_set_irq_line(M6800_TIN_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	nsc8105_set_irq_line(INPUT_LINE_NMI, info->i);	break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			nsc8105_set_reg (k6800RegisterIndexPC, info->i); break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			nsc8105_set_reg (k6800RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_CC:			nsc8105_set_reg (k6800RegisterIndexCC, info->i); break;
			case CPUINFO_INT_REGISTER + M6800_A:			nsc8105_set_reg (k6800RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_B:			nsc8105_set_reg (k6800RegisterIndexB, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_X:			nsc8105_set_reg (k6800RegisterIndexX, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					nsc8105_set_irq_callback (info->irqcallback);	break;
		}
	}

	void nsc8105_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Asgard6800Context);	break;

			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 9;					break;

			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	info->i = nsc8105_get_reg(k6800RegisterIndexIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	info->i = nsc8105_get_reg(k6800RegisterIndexTINState);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = nsc8105_get_reg(k6800RegisterIndexNMIState);	break;

			case CPUINFO_INT_PREVIOUSPC: 					info->i = nsc8105_get_reg(k6800RegisterIndexOpcodePC);	break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			info->i = nsc8105_get_reg(k6800RegisterIndexPC);		break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			info->i = nsc8105_get_reg(k6800RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + M6800_CC:			info->i = nsc8105_get_reg(k6800RegisterIndexCC);		break;
			case CPUINFO_INT_REGISTER + M6800_A:			info->i = nsc8105_get_reg(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_B:			info->i = nsc8105_get_reg(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_X:			info->i = nsc8105_get_reg(k6800RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + M6800_WAI_STATE:	info->i = nsc8105_get_reg(k6800RegisterIndexWAIState);	break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = nsc8105_set_info;			break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = nsc8105_get_context;		break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = nsc8105_set_context;		break;
//			case CPUINFO_PTR_INIT:							info->init = nsc8105_init;					break;
			case CPUINFO_PTR_RESET:							info->reset = nsc8105_reset;				break;
//			case CPUINFO_PTR_EXIT:							info->exit = nsc8105_exit;					break;
			case CPUINFO_PTR_EXECUTE:						info->execute = nsc8105_execute;			break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;							break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = nsc8105_dasm;			break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = nsc8105_get_irq_callback();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &nsc8105_ICount;				break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "NSC8105"); break;

			case CPUINFO_STR_FLAGS:
			{
				Asgard6800Context r;
				nsc8105_get_context (&r);
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

			case CPUINFO_STR_REGISTER + M6800_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", nsc8105_get_reg(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", nsc8105_get_reg(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", nsc8105_get_reg(k6800RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M6800_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", nsc8105_get_reg(k6800RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + M6800_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", nsc8105_get_reg(k6800RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + M6800_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", nsc8105_get_reg(k6800RegisterIndexCC)); break;
			case CPUINFO_STR_REGISTER + M6800_WAI_STATE:	sprintf(info->s = cpuintrf_temp_str(), "WAI:%X", nsc8105_get_reg(k6800RegisterIndexWAIState)); break;

			default:
				m6800_get_info(state, info);
				break;
		}
	}
#endif
