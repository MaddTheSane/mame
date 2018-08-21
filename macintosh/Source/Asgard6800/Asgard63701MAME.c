//###################################################################################################
//
//
//		Asgard63701.c
//		Interface file to make Asgard6800 compile as a 63701 for MAME.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C 63701 core
#define PPC_HD63701
// еее LBO - need to fix issue with Namco System 1 DAC and PPC core
#endif

//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6800 core
#define A6800_CHIP					63701
#define A6800_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6800_DEBUGHOOK				MAME_Debug
#endif

#define A6800_UPDATEBANK			MAME63701UpdateBank
#define A6800_READMEM				program_read_byte_8
#define A6800_WRITEMEM				program_write_byte_8
#define A6800_READPORT				io_read_byte_8
#define A6800_WRITEPORT				io_write_byte_8
#define A6800_OPCODEROM				opcode_base
#define A6800_ARGUMENTROM			opcode_arg_base
#define A6800_ICOUNT				hd63701_ICount_ppc
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


#ifndef PPC_HD63701

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
	#define Asgard6800Init				hd63701_init_ppc
	#define Asgard6800Reset				hd63701_reset_real_ppc
	#define Asgard6800SetContext		hd63701_set_context_ppc
	#define Asgard6800SetReg			hd63701_set_reg_ppc
	#define Asgard6800GetContext		hd63701_get_context_ppc
	#define Asgard6800GetReg			hd63701_get_reg_ppc
	#define Asgard6800SetIRQLine		hd63701_set_irq_line_ppc
	#define Asgard6800SetIRQCallback	hd63701_set_irq_callback_ppc
	#define Asgard6800GetIRQCallback	hd63701_get_irq_callback_ppc
	#define Asgard6800Execute			hd63701_execute_ppc
	#define Asgard6800InternalRead		hd63701_internal_registers_r_ppc
	#define Asgard6800InternalWrite		hd63701_internal_registers_w_ppc
	
	#include "Asgard6800Core.c"
	#include "Asgard6800Debugger.c"

	int hd63701_ICount_ppc;

	// this function is called on any jump of significance
	void MAME63701UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void hd63701_reset_ppc(void *)
	{
		// standard reset
		hd63701_reset_real_ppc();
	}

	static offs_t hd63701_dasm_ppc(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG 
		return Dasm680x(63701,buffer,pc); 
	#else 
		sprintf( buffer, "$%02X", program_read_byte_8(pc) ); 
		return 1; 
	#endif 
	}

	static void hd63701_set_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	hd63701_set_irq_line_ppc(M6800_IRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	hd63701_set_irq_line_ppc(M6800_TIN_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	hd63701_set_irq_line_ppc(INPUT_LINE_NMI, info->i);	break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			hd63701_set_reg_ppc (k6800RegisterIndexPC, info->i); break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			hd63701_set_reg_ppc (k6800RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_CC:			hd63701_set_reg_ppc (k6800RegisterIndexCC, info->i); break;
			case CPUINFO_INT_REGISTER + M6800_A:			hd63701_set_reg_ppc (k6800RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_B:			hd63701_set_reg_ppc (k6800RegisterIndexB, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_X:			hd63701_set_reg_ppc (k6800RegisterIndexX, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					hd63701_set_irq_callback_ppc (info->irqcallback);	break;
		}
	}

	void hd63701_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Asgard6800Context);	break;

			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 9;					break;

			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	info->i = hd63701_get_reg_ppc(k6800RegisterIndexIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	info->i = hd63701_get_reg_ppc(k6800RegisterIndexTINState);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = hd63701_get_reg_ppc(k6800RegisterIndexNMIState);	break;

			case CPUINFO_INT_PREVIOUSPC: 					info->i = hd63701_get_reg_ppc(k6800RegisterIndexOpcodePC);	break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			info->i = hd63701_get_reg_ppc(k6800RegisterIndexPC);		break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			info->i = hd63701_get_reg_ppc(k6800RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + M6800_CC:			info->i = hd63701_get_reg_ppc(k6800RegisterIndexCC);		break;
			case CPUINFO_INT_REGISTER + M6800_A:			info->i = hd63701_get_reg_ppc(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_B:			info->i = hd63701_get_reg_ppc(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_X:			info->i = hd63701_get_reg_ppc(k6800RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + M6800_WAI_STATE:	info->i = hd63701_get_reg_ppc(k6800RegisterIndexWAIState);	break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = hd63701_set_info_ppc;				break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = hd63701_get_context_ppc;			break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = hd63701_set_context_ppc;			break;
//			case CPUINFO_PTR_INIT:							info->init = hd63701_init_ppc;						break;
			case CPUINFO_PTR_RESET:							info->reset = hd63701_reset_ppc;					break;
//			case CPUINFO_PTR_EXIT:							info->exit = hd63701_exit_ppc;						break;
			case CPUINFO_PTR_EXECUTE:						info->execute = hd63701_execute_ppc;				break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;									break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = hd63701_dasm_ppc;				break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = hd63701_get_irq_callback_ppc();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &hd63701_ICount_ppc;					break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "HD63701"); break;

			case CPUINFO_STR_FLAGS:
			{
				Asgard6800Context r;
				hd63701_get_context_ppc (&r);
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

			case CPUINFO_STR_REGISTER + M6800_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", hd63701_get_reg_ppc(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", hd63701_get_reg_ppc(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", hd63701_get_reg_ppc(k6800RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M6800_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", hd63701_get_reg_ppc(k6800RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + M6800_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", hd63701_get_reg_ppc(k6800RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + M6800_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", hd63701_get_reg_ppc(k6800RegisterIndexCC)); break;
			case CPUINFO_STR_REGISTER + M6800_WAI_STATE:	sprintf(info->s = cpuintrf_temp_str(), "WAI:%X", hd63701_get_reg_ppc(k6800RegisterIndexWAIState)); break;

			default:
				m6800_get_info_ppc(state, info);
				break;
		}
	}


	void hd63701_get_info(UINT32 state, union cpuinfo *info)
	{
		if (using6800AsmCores)
			hd63701_get_info_ppc (state, info);
		else
			hd63701_get_info_c (state, info);
	}
	
#endif
