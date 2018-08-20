//###################################################################################################
//
//
//		Asgard6803.c
//		Interface file to make Asgard6800 compile as a 6801/6803 for MAME.
//
//		See Asgard6800Core.c for information about the Asgard6800 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C 6803 core
#define PPC_M6803
#endif


//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 6800 core
#define A6800_CHIP					6803
#define A6800_COREDEBUG				0

#ifdef MAME_DEBUG
#define A6800_DEBUGHOOK				MAME_Debug
#endif

#define A6800_UPDATEBANK			MAME6803UpdateBank
#define A6800_READMEM				program_read_byte_8
#define A6800_WRITEMEM				program_write_byte_8
#define A6800_READPORT				io_read_byte_8
#define A6800_WRITEPORT				io_write_byte_8
#define A6800_OPCODEROM				opcode_base
#define A6800_ARGUMENTROM			opcode_arg_base
#define A6800_ICOUNT				m6803_ICount_ppc
#define A6800_VOLATILEREGS			1

extern Boolean using6800AsmCores;


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "cpuintrf.h"


#ifndef PPC_M6803

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
	#define Asgard6800Init				m6803_init_ppc
	#define Asgard6800Reset				m6803_reset_real_ppc
	#define Asgard6800SetContext		m6803_set_context_ppc
	#define Asgard6800SetReg			m6803_set_reg_ppc
	#define Asgard6800GetContext		m6803_get_context_ppc
	#define Asgard6800GetReg			m6803_get_reg_ppc
	#define Asgard6800SetIRQLine		m6803_set_irq_line_ppc
	#define Asgard6800SetIRQCallback	m6803_set_irq_callback_ppc
	#define Asgard6800GetIRQCallback	m6803_get_irq_callback_ppc
	#define Asgard6800Execute			m6803_execute_ppc
	#define Asgard6800InternalRead		m6803_internal_registers_r_ppc
	#define Asgard6800InternalWrite		m6803_internal_registers_w_ppc
	
	#include "Asgard6800.h"
	#include "Asgard6800Core.c"
	#include "Asgard6800Debugger.c"
	
	int m6803_ICount_ppc;

static READ8_HANDLER( m6803_internal_registers_r_ppc );
static WRITE8_HANDLER( m6803_internal_registers_w_ppc );

static ADDRESS_MAP_START(m6803_mem_ppc, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(m6803_internal_registers_r_ppc, m6803_internal_registers_w_ppc)
	AM_RANGE(0x0020, 0x007f) AM_NOP        /* unused */
	AM_RANGE(0x0080, 0x00ff) AM_RAM        /* 6803 internal RAM */
ADDRESS_MAP_END

	// this function is called on any jump of significance
	void MAME6803UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	static void m6803_reset_ppc(void *)
	{
		// standard reset
		m6803_reset_real_ppc();
	}

	static offs_t m6803_dasm_ppc(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG
	    return Dasm680x(6803,buffer,pc);
	#else
		sprintf( buffer, "$%02X", program_read_byte_8(pc) );
		return 1;
	#endif
	}

	static void m6803_set_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	m6803_set_irq_line_ppc(M6800_IRQ_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	m6803_set_irq_line_ppc(M6800_TIN_LINE, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	m6803_set_irq_line_ppc(INPUT_LINE_NMI, info->i);	break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			m6803_set_reg_ppc (k6800RegisterIndexPC, info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			m6803_set_reg_ppc (k6800RegisterIndexS, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_CC:			m6803_set_reg_ppc (k6800RegisterIndexCC, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_A:			m6803_set_reg_ppc (k6800RegisterIndexA, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_B:			m6803_set_reg_ppc (k6800RegisterIndexB, info->i);	break;
			case CPUINFO_INT_REGISTER + M6800_X:			m6803_set_reg_ppc (k6800RegisterIndexX, info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					m6803_set_irq_callback_ppc (info->irqcallback);		break;
		}
	}

	void m6803_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(Asgard6800Context);	break;

			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 9;					break;

			case CPUINFO_INT_INPUT_STATE + M6800_IRQ_LINE:	info->i = m6803_get_reg_ppc(k6800RegisterIndexIRQState);	break;
			case CPUINFO_INT_INPUT_STATE + M6800_TIN_LINE:	info->i = m6803_get_reg_ppc(k6800RegisterIndexTINState);	break;
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:	info->i = m6803_get_reg_ppc(k6800RegisterIndexNMIState);	break;

			case CPUINFO_INT_PREVIOUSPC: 					info->i = m6803_get_reg_ppc(k6800RegisterIndexOpcodePC);	break;
			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + M6800_PC:			info->i = m6803_get_reg_ppc(k6800RegisterIndexPC);			break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M6800_S:			info->i = m6803_get_reg_ppc(k6800RegisterIndexS);			break;
			case CPUINFO_INT_REGISTER + M6800_CC:			info->i = m6803_get_reg_ppc(k6800RegisterIndexCC);			break;
			case CPUINFO_INT_REGISTER + M6800_A:			info->i = m6803_get_reg_ppc(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_B:			info->i = m6803_get_reg_ppc(k6800RegisterIndexB);			break;
			case CPUINFO_INT_REGISTER + M6800_X:			info->i = m6803_get_reg_ppc(k6800RegisterIndexX);			break;
			case CPUINFO_INT_REGISTER + M6800_WAI_STATE:	info->i = m6803_get_reg_ppc(k6800RegisterIndexWAIState);	break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = m6803_set_info_ppc;					break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m6803_get_context_ppc;			break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m6803_set_context_ppc;			break;
//			case CPUINFO_PTR_INIT:							info->init = m6803_init_ppc;						break;
			case CPUINFO_PTR_RESET:							info->reset = m6803_reset_ppc;						break;
//			case CPUINFO_PTR_EXIT:							info->exit = m6803_exit_ppc;						break;
			case CPUINFO_PTR_EXECUTE:						info->execute = m6803_execute_ppc;					break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;									break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6803_dasm_ppc;					break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = m6803_get_irq_callback_ppc();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m6803_ICount_ppc;					break;

			case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = construct_map_m6803_mem_ppc; break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6803"); break;

			case CPUINFO_STR_FLAGS:
			{
				Asgard6800Context r;
				m6803_get_context_ppc (&r);
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

			case CPUINFO_STR_REGISTER + M6800_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%02X", m6803_get_reg_ppc(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", m6803_get_reg_ppc(k6800RegisterIndexB)); break;
			case CPUINFO_STR_REGISTER + M6800_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m6803_get_reg_ppc(k6800RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M6800_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", m6803_get_reg_ppc(k6800RegisterIndexS)); break;
			case CPUINFO_STR_REGISTER + M6800_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", m6803_get_reg_ppc(k6800RegisterIndexX)); break;
			case CPUINFO_STR_REGISTER + M6800_CC:			sprintf(info->s = cpuintrf_temp_str(), "CC:%02X", m6803_get_reg_ppc(k6800RegisterIndexCC)); break;
			case CPUINFO_STR_REGISTER + M6800_WAI_STATE:	sprintf(info->s = cpuintrf_temp_str(), "WAI:%X", m6803_get_reg_ppc(k6800RegisterIndexWAIState)); break;

			default:
				m6800_get_info(state, info);
				break;
		}
	}

	/****************************************************************************
	 * M6801 almost (fully?) equal to the M6803
	 ****************************************************************************/
	static void m6801_init_ppc(void) { m6803_init_ppc(); }
	static int  m6801_execute_ppc(int cycles) { return m6803_execute_ppc(cycles); }

	static offs_t m6801_dasm_ppc(char *buffer, unsigned pc)
	{
	#ifdef MAME_DEBUG
	    return Dasm680x(6801,buffer,pc);
	#else
		sprintf( buffer, "$%02X", program_read_byte_8(pc) );
		return 1;
	#endif
	}

	void m6801_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 9;					break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_INIT:							info->init = m6801_init_ppc;				break;
			case CPUINFO_PTR_EXECUTE:						info->execute = m6801_execute_ppc;			break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m6801_dasm_ppc;			break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "M6801"); break;

			default:
				m6803_get_info_ppc(state, info);
				break;
		}
	}

	void m6801_get_info(UINT32 state, union cpuinfo *info)
	{
		if (using6800AsmCores)
			m6801_get_info_ppc (state, info);
		else
			m6801_get_info_c (state, info);
	}
	
	void m6803_get_info(UINT32 state, union cpuinfo *info)
	{
		if (using6800AsmCores)
			m6803_get_info_ppc (state, info);
		else
			m6803_get_info_c (state, info);
	}
	
#endif
