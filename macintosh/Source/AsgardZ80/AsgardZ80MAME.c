//###################################################################################################
//
//
//		AsgardZ80MAME.c
//		Interface file to make AsgardZ80 work with MAME.
//
//		See AsgardZ80Core.c for information about the AsgardZ80 system.
//
//
//###################################################################################################

#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
// Undefine this to use the slower C Z80 core
// LBO 9/24/04. I need to revamp the PPC core to deal with reading/writing opcodes through
// the MAME routines cpu_readop and cpu_readop_arg so that they can deal with games that
// use mirrored memory banks properly, like Frogs. Work is almost done, but needs lots of testing.
//#define PPC_Z80
#endif


//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly Z80 core
#define AZ80_COREDEBUG				0

#ifdef MAME_DEBUG
#define AZ80_DEBUGHOOK				MAME_Debug
#endif

#define AZ80_UPDATEBANK				MAMEZ80UpdateBank
#define AZ80_READMEM				program_read_byte_8
#define AZ80_READOP					cpu_readop_ppc
#define AZ80_READOPARG				cpu_readop_arg_ppc
#define AZ80_WRITEMEM				program_write_byte_8
#define AZ80_READPORT				io_read_byte_8
#define AZ80_WRITEPORT				io_write_byte_8
#define AZ80_OPCODEROM				opcode_base
#define AZ80_ARGUMENTROM			opcode_arg_base
#define AZ80_ICOUNT					z80_ICount
#define AZ80_VOLATILEREGS			1


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "cpuintrf.h"
#include "memory.h"


#ifndef PPC_Z80

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	#include "cpu/z80/Z80.h"
	#include "AsgardZ80Debugger.h"

	#include "cpu/z80/Z80.c"

	/*---------------------------------------------------------------------------

	To debug, insert these lines in mZ80.c right after the #ifdef MAME_DEBUG:

	You will also need to modify the main loop to count cycles *after* executing
	the opcode.

	#if AZ80_COREDEBUG
		AsgardZ80MiniTrace((_PC<<16)+_AF, (_SP<<16)+_DE, (_HL<<16)+_BC, (_IX<<16)+_IY, 
				(Z80.AF2.w.l<<16)+Z80.DE2.w.l, (Z80.HL2.w.l<<16)+Z80.BC2.w.l, 
				(_IM & 3) | ((_HALT & 1) << 5) | ((_IFF2 & 1) << 6) | ((_IFF1 & 1) << 7) | ((_I & 0xff) << 8) | ((_R2 & 0xff) << 16) | ((_R & 0xff) << 24), 
				z80_ICount);
	#endif
	
	---------------------------------------------------------------------------*/

	#if AZ80_COREDEBUG
		#undef DIRECT
		#undef EXTENDED
		#include "AsgardZ80Debugger.c"
	#endif

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	// #define Get/SetRegs away so our definitions don't conflict
	#include "Z80.h"

	// redefine some of our functions and variables to map to MAME's
	#define AsgardZ80Init				z80_init
	#define AsgardZ80SetContext			z80_set_context
	#define AsgardZ80SetReg				z80_set_reg
	#define AsgardZ80GetContext			z80_get_context
	#define AsgardZ80GetReg				z80_get_reg
	#define AsgardZ80SetIRQLine			z80_set_irq_line
	#define AsgardZ80SetIRQCallback		z80_set_irq_callback
	#define AsgardZ80GetIRQCallback		z80_get_irq_callback
	#define AsgardZ80Execute			z80_execute
	
	UINT8  cpu_readop_ppc(offs_t A)
	{
		return cpu_readop(A);
	}

	UINT8  cpu_readop_arg_ppc(offs_t A)
	{
		return cpu_readop_arg(A);
	}

	#include "AsgardZ80Core.h"
	#include "AsgardZ80Core.c"
	#include "AsgardZ80Debugger.c"
	
	int z80_ICount;

	// this function is called on any jump of significance
	void MAMEZ80UpdateBank(int pc)
	{
		change_pc(pc);
	}
	
	void z80_reset(void *daisy_chain)
	{
		// standard reset
		AsgardZ80Reset(daisy_chain);
	}

	static void z80x_get_context(void *dst)
	{
		z80_get_context(dst);
	}

	// this function cleans up after the core
	static void z80_exit(void)
	{
	}
	
	static void z80_burn(int cycles)
	{
		if( cycles > 0 )
		{
			/* NOP takes 4 cycles per instruction */
			int n = (cycles + 3) / 4;
			sFlags += n << 24;
			z80_ICount -= 4 * n;
		}
	}
	
	static offs_t z80_dasm( char *buffer, unsigned pc )
	{
	#ifdef MAME_DEBUG
	    return DasmZ80( buffer, pc );
	#else
		sprintf( buffer, "$%02X", cpu_readop(pc) );
		return 1;
	#endif
	}

	static void z80_set_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		z80_set_irq_line(INPUT_LINE_NMI, info->i);	break;
			case CPUINFO_INT_INPUT_STATE + 0:					z80_set_irq_line(0, info->i);				break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + Z80_PC:				z80_set_reg(kZ80RegisterIndexPC,info->i);	break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + Z80_SP:				z80_set_reg(kZ80RegisterIndexSP,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_AF:				z80_set_reg(kZ80RegisterIndexAF,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_BC:				z80_set_reg(kZ80RegisterIndexBC,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_DE:				z80_set_reg(kZ80RegisterIndexDE,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_HL:				z80_set_reg(kZ80RegisterIndexHL,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_IX:				z80_set_reg(kZ80RegisterIndexIX,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_IY:				z80_set_reg(kZ80RegisterIndexIY,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_R:				z80_set_reg(kZ80RegisterIndexR,info->i); z80_set_reg(kZ80RegisterIndexR2,info->i & 0x80);	break;
			case CPUINFO_INT_REGISTER + Z80_I:				z80_set_reg(kZ80RegisterIndexI,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_AF2:			z80_set_reg(kZ80RegisterIndexAF2,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_BC2:			z80_set_reg(kZ80RegisterIndexBC2,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_DE2:			z80_set_reg(kZ80RegisterIndexDE2,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_HL2:			z80_set_reg(kZ80RegisterIndexHL2,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_IM:				z80_set_reg(kZ80RegisterIndexIM,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_IFF1:			z80_set_reg(kZ80RegisterIndexIFF1,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_IFF2:			z80_set_reg(kZ80RegisterIndexIFF2,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_HALT:			z80_set_reg(kZ80RegisterIndexHALT,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_DC0:			z80_set_reg(kZ80RegisterIndexDaisyIntState0,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_DC1:			z80_set_reg(kZ80RegisterIndexDaisyIntState1,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_DC2:			z80_set_reg(kZ80RegisterIndexDaisyIntState2,info->i);	break;
			case CPUINFO_INT_REGISTER + Z80_DC3:			z80_set_reg(kZ80RegisterIndexDaisyIntState3,info->i);	break;
			
			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:					z80_set_irq_callback(info->irqcallback);	break;
#if 0 // еее LBO - 5/9/04 AsgardZ80 doesn't support altered cycle count tables
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_op: cc[Z80_TABLE_op] = info->p;			break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_cb: cc[Z80_TABLE_cb] = info->p;			break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_ed: cc[Z80_TABLE_ed] = info->p;			break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_xy: cc[Z80_TABLE_xy] = info->p;			break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_xycb: cc[Z80_TABLE_xycb] = info->p;		break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_ex: cc[Z80_TABLE_ex] = info->p;			break;
#endif
		}
	}

	static UINT8 z80_reg_layout[] =
	{
		Z80_PC, Z80_SP, Z80_AF, Z80_BC, Z80_DE, Z80_HL, -1,
		Z80_IX, Z80_IY, Z80_AF2,Z80_BC2,Z80_DE2,Z80_HL2,-1,
		Z80_R,	Z80_I,	Z80_IM, Z80_IFF1,Z80_IFF2, -1,
		Z80_DC0,Z80_DC1,Z80_DC2,Z80_DC3, 0
	};

	static UINT8 z80_win_layout[] =
	{
		27, 0,53, 4,	// register window (top rows)
		 0, 0,26,22,	// disassembler window (left colums)
		27, 5,53, 8,	// memory #1 window (right, upper middle)
		27,14,53, 8,	// memory #2 window (right, lower middle)
		 0,23,80, 1,	// command line window (bottom rows)
	};

	void z80_get_info(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = z80_get_context(NULL);		break;
			case CPUINFO_INT_INPUT_LINES:					info->i = 1;							break;
			case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0xff;							break;
			case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
			case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
			case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
			case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
			case CPUINFO_INT_MAX_CYCLES:					info->i = 16;							break;
			
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 16;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

			case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:		info->i = z80_get_reg(kZ80RegisterIndexNMIState);	break;
			case CPUINFO_INT_INPUT_STATE + 0:					info->i = z80_get_reg(kZ80RegisterIndexIRQState);	break;

			case CPUINFO_INT_PREVIOUSPC: 					info->i = z80_get_reg(kZ80RegisterIndexOpcodePC);		break;

			case CPUINFO_INT_PC:
			case CPUINFO_INT_REGISTER + Z80_PC:				info->i = z80_get_reg(kZ80RegisterIndexPC);		break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + Z80_SP:				info->i = z80_get_reg(kZ80RegisterIndexSP);		break;
			case CPUINFO_INT_REGISTER + Z80_AF:				info->i = z80_get_reg(kZ80RegisterIndexAF);		break;
			case CPUINFO_INT_REGISTER + Z80_BC:				info->i = z80_get_reg(kZ80RegisterIndexBC);		break;
			case CPUINFO_INT_REGISTER + Z80_DE:				info->i = z80_get_reg(kZ80RegisterIndexDE);		break;
			case CPUINFO_INT_REGISTER + Z80_HL:				info->i = z80_get_reg(kZ80RegisterIndexHL);		break;
			case CPUINFO_INT_REGISTER + Z80_IX:				info->i = z80_get_reg(kZ80RegisterIndexIX);		break;
			case CPUINFO_INT_REGISTER + Z80_IY:				info->i = z80_get_reg(kZ80RegisterIndexIY);		break;
			case CPUINFO_INT_REGISTER + Z80_R:				info->i = (z80_get_reg(kZ80RegisterIndexR) & 0x7f) | (z80_get_reg(kZ80RegisterIndexR2) & 0x80);		break;
			case CPUINFO_INT_REGISTER + Z80_I:				info->i = z80_get_reg(kZ80RegisterIndexI);		break;
			case CPUINFO_INT_REGISTER + Z80_AF2:			info->i = z80_get_reg(kZ80RegisterIndexAF2);	break;
			case CPUINFO_INT_REGISTER + Z80_BC2:			info->i = z80_get_reg(kZ80RegisterIndexBC2);	break;
			case CPUINFO_INT_REGISTER + Z80_DE2:			info->i = z80_get_reg(kZ80RegisterIndexDE2);	break;
			case CPUINFO_INT_REGISTER + Z80_HL2:			info->i = z80_get_reg(kZ80RegisterIndexHL2);	break;
			case CPUINFO_INT_REGISTER + Z80_IM:				info->i = z80_get_reg(kZ80RegisterIndexIM);		break;
			case CPUINFO_INT_REGISTER + Z80_IFF1:			info->i = z80_get_reg(kZ80RegisterIndexIFF1);	break;
			case CPUINFO_INT_REGISTER + Z80_IFF2:			info->i = z80_get_reg(kZ80RegisterIndexIFF2);	break;
			case CPUINFO_INT_REGISTER + Z80_HALT:			info->i = z80_get_reg(kZ80RegisterIndexHALT);	break;
			case CPUINFO_INT_REGISTER + Z80_DC0:			info->i = z80_get_reg(kZ80RegisterIndexDaisyIntState0);	break;
			case CPUINFO_INT_REGISTER + Z80_DC1:			info->i = z80_get_reg(kZ80RegisterIndexDaisyIntState1);	break;
			case CPUINFO_INT_REGISTER + Z80_DC2:			info->i = z80_get_reg(kZ80RegisterIndexDaisyIntState2);	break;
			case CPUINFO_INT_REGISTER + Z80_DC3:			info->i = z80_get_reg(kZ80RegisterIndexDaisyIntState3);	break;
			case REG_PREVIOUSPC: return ;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = z80_set_info;			break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = z80x_get_context;	break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = z80_set_context;		break;
			case CPUINFO_PTR_INIT:							info->init = z80_init;					break;
			case CPUINFO_PTR_RESET:							info->reset = z80_reset;				break;
			case CPUINFO_PTR_EXIT:							info->exit = z80_exit;					break;
			case CPUINFO_PTR_EXECUTE:						info->execute = z80_execute;			break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = z80_dasm;			break;
			case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = z80_get_irq_callback();	break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &z80_ICount;				break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = z80_reg_layout;				break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = z80_win_layout;				break;
#if 0 // еее LBO - 5/9/04 AsgardZ80 doesn't support altered cycle count tables
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_op: info->p = (void *)cc[Z80_TABLE_op];	break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_cb: info->p = (void *)cc[Z80_TABLE_cb];	break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_ed: info->p = (void *)cc[Z80_TABLE_ed];	break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_xy: info->p = (void *)cc[Z80_TABLE_xy];	break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_xycb: info->p = (void *)cc[Z80_TABLE_xycb]; break;
			case CPUINFO_PTR_Z80_CYCLE_TABLE + Z80_TABLE_ex: info->p = (void *)cc[Z80_TABLE_ex];	break;
#endif

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "Z80"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Zilog Z80"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.5"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) Aaron Giles, 1997-1999"); break;

			case CPUINFO_STR_FLAGS:
			{
				int flags = z80_get_reg(kZ80RegisterIndexAF);
				sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
					flags & 0x80 ? 'S':'.',
					flags & 0x40 ? 'Z':'.',
					flags & 0x20 ? '5':'.',
					flags & 0x10 ? 'H':'.',
					flags & 0x08 ? '3':'.',
					flags & 0x04 ? 'P':'.',
					flags & 0x02 ? 'N':'.',
					flags & 0x01 ? 'C':'.');
				break;
			}

			case CPUINFO_STR_REGISTER + Z80_PC:				sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", z80_get_reg(kZ80RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + Z80_SP:				sprintf(info->s = cpuintrf_temp_str(), "SP:%04X", z80_get_reg(kZ80RegisterIndexSP)); break;
			case CPUINFO_STR_REGISTER + Z80_AF:				sprintf(info->s = cpuintrf_temp_str(), "AF:%04X", z80_get_reg(kZ80RegisterIndexAF)); break;
			case CPUINFO_STR_REGISTER + Z80_BC:				sprintf(info->s = cpuintrf_temp_str(), "BC:%04X", z80_get_reg(kZ80RegisterIndexBC)); break;
			case CPUINFO_STR_REGISTER + Z80_DE:				sprintf(info->s = cpuintrf_temp_str(), "DE:%04X", z80_get_reg(kZ80RegisterIndexDE)); break;
			case CPUINFO_STR_REGISTER + Z80_HL:				sprintf(info->s = cpuintrf_temp_str(), "HL:%04X", z80_get_reg(kZ80RegisterIndexHL)); break;
			case CPUINFO_STR_REGISTER + Z80_IX:				sprintf(info->s = cpuintrf_temp_str(), "IX:%04X", z80_get_reg(kZ80RegisterIndexIX)); break;
			case CPUINFO_STR_REGISTER + Z80_IY:				sprintf(info->s = cpuintrf_temp_str(), "IY:%04X", z80_get_reg(kZ80RegisterIndexIY)); break;
			case CPUINFO_STR_REGISTER + Z80_R:				sprintf(info->s = cpuintrf_temp_str(), "R:%02X", (z80_get_reg(kZ80RegisterIndexR) & 0x7f) | (z80_get_reg(kZ80RegisterIndexR2) & 0x80)); break;
			case CPUINFO_STR_REGISTER + Z80_I:				sprintf(info->s = cpuintrf_temp_str(), "I:%02X", z80_get_reg(kZ80RegisterIndexI)); break;
			case CPUINFO_STR_REGISTER + Z80_AF2:			sprintf(info->s = cpuintrf_temp_str(), "AF'%04X", z80_get_reg(kZ80RegisterIndexAF2)); break;
			case CPUINFO_STR_REGISTER + Z80_BC2:			sprintf(info->s = cpuintrf_temp_str(), "BC'%04X", z80_get_reg(kZ80RegisterIndexBC2)); break;
			case CPUINFO_STR_REGISTER + Z80_DE2:			sprintf(info->s = cpuintrf_temp_str(), "DE'%04X", z80_get_reg(kZ80RegisterIndexDE2)); break;
			case CPUINFO_STR_REGISTER + Z80_HL2:			sprintf(info->s = cpuintrf_temp_str(), "HL'%04X", z80_get_reg(kZ80RegisterIndexHL2)); break;
			case CPUINFO_STR_REGISTER + Z80_IM:				sprintf(info->s = cpuintrf_temp_str(), "IM:%X", z80_get_reg(kZ80RegisterIndexIM)); break;
			case CPUINFO_STR_REGISTER + Z80_IFF1:			sprintf(info->s = cpuintrf_temp_str(), "IFF1:%X", z80_get_reg(kZ80RegisterIndexIFF1)); break;
			case CPUINFO_STR_REGISTER + Z80_IFF2:			sprintf(info->s = cpuintrf_temp_str(), "IFF2:%X", z80_get_reg(kZ80RegisterIndexIFF2)); break;
			case CPUINFO_STR_REGISTER + Z80_HALT:			sprintf(info->s = cpuintrf_temp_str(), "HALT:%X", z80_get_reg(kZ80RegisterIndexHALT)); break;
			case CPUINFO_STR_REGISTER + Z80_DC0:			sprintf(info->s = cpuintrf_temp_str(), "DC0:%X", z80_get_reg(kZ80RegisterIndexDaisyIntCount) >= 1 && z80_get_reg(kZ80RegisterIndexDaisyIntState0)); break;
			case CPUINFO_STR_REGISTER + Z80_DC1:			sprintf(info->s = cpuintrf_temp_str(), "DC1:%X", z80_get_reg(kZ80RegisterIndexDaisyIntCount) >= 2 && z80_get_reg(kZ80RegisterIndexDaisyIntState1)); break;
			case CPUINFO_STR_REGISTER + Z80_DC2:			sprintf(info->s = cpuintrf_temp_str(), "DC2:%X", z80_get_reg(kZ80RegisterIndexDaisyIntCount) >= 3 && z80_get_reg(kZ80RegisterIndexDaisyIntState2)); break;
			case CPUINFO_STR_REGISTER + Z80_DC3:			sprintf(info->s = cpuintrf_temp_str(), "DC3:%X", z80_get_reg(kZ80RegisterIndexDaisyIntCount) >= 4 && z80_get_reg(kZ80RegisterIndexDaisyIntState3)); break;
		}
	}

#endif