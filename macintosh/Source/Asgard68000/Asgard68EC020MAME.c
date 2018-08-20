//###################################################################################################
//
//
//		Asgard68000MAME.c
//		Interface file to make Asgard68000 work with MAME.
//
//		See Asgard68000Core.c for information about the Asgard68000 system.
//
//
//###################################################################################################


#if __MWERKS__ // LBO - this will need to change when we have formal Xcode support
#define PPC_M68EC020
#endif

//###################################################################################################
//	CONFIGURATION
//###################################################################################################

// configure the assembly 68000 core
#define A68000_CHIP					68020
#define A68000_ADDRESSBITS			24
#define A68000_COREDEBUG			0

#ifdef MAME_DEBUG
#define A68000_DEBUGHOOK			MAME_Debug
#endif

#define A68000_UPDATEBANK			m68ec020_update_bank_ppc
#define A68000_READBYTE				program_read_byte_32be
#define A68000_WRITEBYTE			program_write_byte_32be
#define A68000_READWORD				readword_d32_ppc
#define A68000_WRITEWORD			writeword_d32_ppc
#define A68000_READLONG				readlong_d32_ppc
#define A68000_WRITELONG			writelong_d32_ppc
#define A68000_READBYTE_REL			m68k_read_pcrel_8_d32_ppc
#define A68000_READWORD_REL			m68k_read_pcrel_16_d32_ppc
#define A68000_READLONG_REL			m68k_read_pcrel_32_d32_ppc
#define A68000_OPCODEROM			opcode_base
#define A68000_ARGUMENTROM			opcode_arg_base
#define A68000_ICOUNT				m68ec020_ICount_ppc
#define A68000_VOLATILEREGS			1

extern Boolean usingAsmCores;
extern void m68ec020_get_info(UINT32 state, union cpuinfo *info);


//###################################################################################################
//	INCLUDES
//###################################################################################################

#include <stddef.h>
#include <stdio.h>

#include "cpuintrf.h"
#include "osd_cpu.h"

extern void m68ec020_get_info(UINT32 state, union cpuinfo *info);

#ifndef PPC_M68EC020

//###################################################################################################
//	C INTERFACE GLUE
//###################################################################################################

	// the C glue builds all three cores (68000/68010/68020), so we only need to include it in one
	// I've put it into the 68000 for now
#if !MAC_XCODE
	#include "Asgard68000Debugger.h"
	
	#if A68000_COREDEBUG

		#include "cpudefs.h"
		
		#undef get_byte
		#undef get_word
		#undef get_long
		#undef put_byte
		#undef put_word
		#undef put_long

		#define get_byte(a) Asgard68000DebugReadByte((a)&0xffffff)
		#define get_word(a) Asgard68000DebugReadWord((a)&0xffffff)
		#define get_long(a) Asgard68000DebugReadLong((a)&0xffffff)
		#define put_byte(a,b) Asgard68000DebugWriteByte((a)&0xffffff,b)
		#define put_word(a,b) Asgard68000DebugWriteWord((a)&0xffffff,b)
		#define put_long(a,b) Asgard68000DebugWriteLong((a)&0xffffff,b)
	
	#endif

	// We build the 68000 separate from the 68020, so if we're building the C 68k core
	// tell the C glue code that we're building the asm 68000 to keep it from being
	// included in this file. See Asgard68000MAME.c for the 68000 variant.
	#define A68K0

	#include "cpu/m68000/m68kmame.c"
	#include "cpu/m68000/m68kcpu.c"
	#include "cpu/m68000/m68kops.c"
	#include "cpu/m68000/m68kopac.c"
	#include "cpu/m68000/m68kopdm.c"
	#include "cpu/m68000/m68kopnz.c"

	/*---------------------------------------------------------------------------

	To debug, insert these lines in m68kcpu.c right after the CALL_MAME_DEBUG:

	#if A68000_COREDEBUG
		Asgard68000MiniTrace(&CPU_D[0], &CPU_A[0], m68k_peek_pc(), m68k_peek_sr(), m68ki_remaining_cycles);
	#endif

	---------------------------------------------------------------------------*/

	#if A68000_COREDEBUG
		#include "Asgard68000Debugger.c"
	#endif
#endif // !MAC_XCODE

#else

//###################################################################################################
//	ASSEMBLY INTERFACE GLUE
//###################################################################################################

	#define A68K2
	#include "m68000.h"

	enum
	{
		M68K_CPU_TYPE_INVALID,
		M68K_CPU_TYPE_68000,
		M68K_CPU_TYPE_68010,
		M68K_CPU_TYPE_68EC020,
		M68K_CPU_TYPE_68020,
		M68K_CPU_TYPE_68030,	/* Supported by disassembler ONLY */
		M68K_CPU_TYPE_68040		/* Supported by disassembler ONLY */
	};

	/* redefine some of our functions and variables to map to MAME's */
	#define Asgard68000Init					m68ec020_init_ppc
	#define Asgard68000Reset				m68ec020_reset_real_ppc
	#define Asgard68000SetContext			m68ec020_set_context_ppc
	#define Asgard68000SetReg				m68ec020_set_reg_ppc
	#define Asgard68000GetContext			m68ec020_get_context_ppc
	#define Asgard68000GetReg				m68ec020_get_reg_ppc
	#define Asgard68000SetIRQLine			m68ec020_set_irq_line_ppc
	#define Asgard68000SetIRQCallback		m68ec020_set_int_ack_callback_ppc
	#define Asgard68000SetResetCallback		m68ec020_set_reset_instr_callback_ppc
	#define Asgard68000SetRTECallback		m68ec020_set_rte_instr_callback_ppc
	#define Asgard68000SetCMPILDCallback	m68ec020_set_cmpild_instr_callback_ppc
	#define Asgard68000Execute				m68ec020_execute_ppc

	#include "Asgard68000.h"
	#include "Asgard68000Core.c"
	#include "Asgard68000Debugger.c"

	int m68ec020_ICount_ppc;
	extern offs_t m68k_encrypted_opcode_start[MAX_CPU];
	extern offs_t m68k_encrypted_opcode_end[MAX_CPU];

	/* potentially misaligned 16-bit reads with a 32-bit data bus (and 24-bit address bus) */
	UINT16 readword_d32_ppc(offs_t address)
	{
		UINT16 result;

		if (!(address & 1))
			return program_read_word_32be(address);
		result = program_read_byte_32be(address) << 8;
		return result | program_read_byte_32be(address + 1);
	}

	/* potentially misaligned 16-bit writes with a 32-bit data bus (and 24-bit address bus) */
	void writeword_d32_ppc(offs_t address, UINT16 data)
	{
		if (!(address & 1))
		{
			program_write_word_32be(address, data);
			return;
		}
		program_write_byte_32be(address, data >> 8);
		program_write_byte_32be(address + 1, data);
	}

	/* potentially misaligned 32-bit reads with a 32-bit data bus (and 24-bit address bus) */
	UINT32 readlong_d32_ppc(offs_t address)
	{
		UINT32 result;

		if (!(address & 3))
			return program_read_dword_32be(address);
		else if (!(address & 1))
		{
			result = program_read_word_32be(address) << 16;
			return result | program_read_word_32be(address + 2);
		}
		result = program_read_byte_32be(address) << 24;
		result |= program_read_word_32be(address + 1) << 8;
		return result | program_read_byte_32be(address + 3);
	}

	/* potentially misaligned 32-bit writes with a 32-bit data bus (and 24-bit address bus) */
	void writelong_d32_ppc(offs_t address, UINT32 data)
	{
		if (!(address & 3))
		{
			program_write_dword_32be(address, data);
			return;
		}
		else if (!(address & 1))
		{
			program_write_word_32be(address, data >> 16);
			program_write_word_32be(address + 2, data);
			return;
		}
		program_write_byte_32be(address, data >> 24);
		program_write_word_32be(address + 1, data >> 8);
		program_write_byte_32be(address + 3, data);
	}

	UINT8 m68k_read_pcrel_8_d32_ppc(unsigned int address)
	{
		if (address >= m68k_encrypted_opcode_start[cpu_getactivecpu()] &&
				address < m68k_encrypted_opcode_end[cpu_getactivecpu()])
			return ((cpu_readop32(address&~1)>>(8*(1-(address & 1))))&0xff);
		else
			return program_read_byte_32be(address);
	}

	UINT16 m68k_read_pcrel_16_d32_ppc(unsigned int address)
	{
		if (address >= m68k_encrypted_opcode_start[cpu_getactivecpu()] &&
				address < m68k_encrypted_opcode_end[cpu_getactivecpu()])
			return cpu_readop32(address);
		else
			return readword_d32_ppc(address);
	}

	UINT32 m68k_read_pcrel_32_d32_ppc(unsigned int address)
	{
		if (address >= m68k_encrypted_opcode_start[cpu_getactivecpu()] &&
				address < m68k_encrypted_opcode_end[cpu_getactivecpu()])
			return ((cpu_readop32(address) << 16) | cpu_readop32((address)+2));
		else
			return readlong_d32_ppc(address);
	}

	// this function is called on any jump of significance
	void m68ec020_update_bank_ppc(offs_t pc)
	{
		change_pc(pc);
	}

	static void m68ec020_exit_ppc(void)
	{
		/* nothing to do ? */
	}
	
	static void m68ec020_reset_ppc(void *param)
	{
		m68ec020_reset_real_ppc();
	}
	
	static void m68ec020x_get_context_ppc(void *dst)
	{
		m68ec020_get_context_ppc(dst);
	}

#ifdef MAME_DEBUG
	/* UGLY HACK */
	unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type);
#endif

	static offs_t m68ec020_dasm_ppc(char *buffer, unsigned pc)
	{
		change_pc(pc);
	#ifdef MAME_DEBUG
		return m68k_disassemble (buffer, pc, M68K_CPU_TYPE_68EC020);
	#else
		sprintf( buffer, "$%04X", cpu_readop16(pc) );
		return 2;
	#endif
	}

	static UINT8 m68ec020_reg_layout_ppc[] = 
	{
		M68K_PC,  M68K_MSP, -1,
		M68K_SR,  M68K_ISP, -1,
		M68K_SFC, M68K_USP, -1,
		M68K_DFC, M68K_VBR, -1,
		M68K_D0,  M68K_A0, -1,
		M68K_D1,  M68K_A1, -1,
		M68K_D2,  M68K_A2, -1,
		M68K_D3,  M68K_A3, -1,
		M68K_D4,  M68K_A4, -1,
		M68K_D5,  M68K_A5, -1,
		M68K_D6,  M68K_A6, -1,
		M68K_D7,  M68K_A7, 0
	};

	static UINT8 m68ec020_win_layout_ppc[] =
	{
		48, 0,32,13,	// register window (top right)
		 0, 0,47,13,	// disassembler window (top left)
		 0,14,47, 8,	// memory #1 window (left, middle)
		48,14,32, 8,	// memory #2 window (right, middle)
		 0,23,80, 1 	// command line window (bottom rows)
	};

	static void m68ec020_set_info_ppc(UINT32 state, union cpuinfo *info)
	{
		switch (state)
		{
			/* --- the following bits of info are set as 64-bit signed integers --- */
			case CPUINFO_INT_INPUT_STATE + 0:				m68ec020_set_irq_line_ppc(0, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 1:				m68ec020_set_irq_line_ppc(1, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 2:				m68ec020_set_irq_line_ppc(2, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 3:				m68ec020_set_irq_line_ppc(3, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 4:				m68ec020_set_irq_line_ppc(4, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 5:				m68ec020_set_irq_line_ppc(5, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 6:				m68ec020_set_irq_line_ppc(6, info->i);					break;
			case CPUINFO_INT_INPUT_STATE + 7:				m68ec020_set_irq_line_ppc(7, info->i);					break;

			case CPUINFO_INT_PC:						m68ec020_set_reg_ppc(k68000RegisterIndexPC, info->i&0x00ffffff);		break;
			case CPUINFO_INT_REGISTER + M68K_PC:  		m68ec020_set_reg_ppc(k68000RegisterIndexPC, info->i);		break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M68K_SP:  		m68ec020_set_reg_ppc(k68000RegisterIndexSP, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_ISP: 		m68ec020_set_reg_ppc(k68000RegisterIndexISP, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_USP: 		m68ec020_set_reg_ppc(k68000RegisterIndexUSP, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_SR:  		m68ec020_set_reg_ppc(k68000RegisterIndexSR, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D0:  		m68ec020_set_reg_ppc(k68000RegisterIndexD0, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D1:  		m68ec020_set_reg_ppc(k68000RegisterIndexD1, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D2:  		m68ec020_set_reg_ppc(k68000RegisterIndexD2, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D3:  		m68ec020_set_reg_ppc(k68000RegisterIndexD3, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D4:  		m68ec020_set_reg_ppc(k68000RegisterIndexD4, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D5:  		m68ec020_set_reg_ppc(k68000RegisterIndexD5, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D6:  		m68ec020_set_reg_ppc(k68000RegisterIndexD6, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_D7:  		m68ec020_set_reg_ppc(k68000RegisterIndexD7, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A0:  		m68ec020_set_reg_ppc(k68000RegisterIndexA0, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A1:  		m68ec020_set_reg_ppc(k68000RegisterIndexA1, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A2:  		m68ec020_set_reg_ppc(k68000RegisterIndexA2, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A3:  		m68ec020_set_reg_ppc(k68000RegisterIndexA3, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A4:  		m68ec020_set_reg_ppc(k68000RegisterIndexA4, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A5:  		m68ec020_set_reg_ppc(k68000RegisterIndexA5, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A6:  		m68ec020_set_reg_ppc(k68000RegisterIndexA6, info->i);		break;
			case CPUINFO_INT_REGISTER + M68K_A7:  		m68ec020_set_reg_ppc(k68000RegisterIndexA7, info->i);		break;
			
			case CPUINFO_INT_REGISTER + M68K_VBR:  		m68ec020_set_reg_ppc(k68000RegisterIndexVBR, info->i);		break; /* 68010+ */
			case CPUINFO_INT_REGISTER + M68K_SFC:  		m68ec020_set_reg_ppc(k68000RegisterIndexSFC, info->i);		break; /* 68010+ */
			case CPUINFO_INT_REGISTER + M68K_DFC:  		m68ec020_set_reg_ppc(k68000RegisterIndexDFC, info->i);		break; /* 68010+ */
			case CPUINFO_INT_REGISTER + M68K_MSP:		m68ec020_set_reg_ppc(k68000RegisterIndexMSP, info->i);		break; /* 68020+ */
			case CPUINFO_INT_REGISTER + M68K_CACR:		m68ec020_set_reg_ppc(k68000RegisterIndexCACR, info->i);		break; /* 68020+ */
			case CPUINFO_INT_REGISTER + M68K_CAAR:		m68ec020_set_reg_ppc(k68000RegisterIndexCAAR, info->i);		break; /* 68020+ */

			/* --- the following bits of info are set as pointers to data or functions --- */
			case CPUINFO_PTR_IRQ_CALLBACK:				m68ec020_set_int_ack_callback_ppc(info->irqcallback); break;
			case CPUINFO_PTR_M68K_RESET_CALLBACK:		m68ec020_set_reset_instr_callback_ppc(info->f);		break;
			case CPUINFO_PTR_M68K_CMPILD_CALLBACK:		m68ec020_set_cmpild_instr_callback_ppc((void (*)(unsigned int,int))(info->f));		break;
			case CPUINFO_PTR_M68K_RTE_CALLBACK:			m68ec020_set_rte_instr_callback_ppc(info->f);		break;
		}
	}

	void m68ec020_get_info_ppc(UINT32 state, union cpuinfo *info)
	{
		int sr;

		switch (state)
		{
			/* --- the following bits of info are returned as 64-bit signed integers --- */
			case CPUINFO_INT_CONTEXT_SIZE:					info->i = m68ec020_get_context_ppc(NULL);	break;
			case CPUINFO_INT_INPUT_LINES:					info->i = 8;							break;
			case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = -1;							break;
			case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
			case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
			case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
			case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 10;							break;
			case CPUINFO_INT_MIN_CYCLES:					info->i = 4;							break;
			case CPUINFO_INT_MAX_CYCLES:					info->i = 158;							break;
			
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 24;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
			case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
			case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

			case CPUINFO_INT_INPUT_STATE + 0:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 1:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 2:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 3:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 4:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 5:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 6:					info->i = 0;	/* fix me */			break;
			case CPUINFO_INT_INPUT_STATE + 7:					info->i = 0;	/* fix me */			break;

			case CPUINFO_INT_PREVIOUSPC:					info->i = m68ec020_get_reg_ppc(k68000RegisterIndexOpcodePC); break;

			case CPUINFO_INT_PC:							info->i = m68ec020_get_reg_ppc(k68000RegisterIndexPC)&0x00ffffff; break;
			case CPUINFO_INT_REGISTER + M68K_PC:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexPC); break;
			case CPUINFO_INT_SP:
			case CPUINFO_INT_REGISTER + M68K_SP:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexSP); break;
			case CPUINFO_INT_REGISTER + M68K_ISP:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexISP); break;
			case CPUINFO_INT_REGISTER + M68K_USP:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexUSP); break;
			case CPUINFO_INT_REGISTER + M68K_SR:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexSR); break;
			case CPUINFO_INT_REGISTER + M68K_D0:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD0); break;
			case CPUINFO_INT_REGISTER + M68K_D1:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD1); break;
			case CPUINFO_INT_REGISTER + M68K_D2:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD2); break;
			case CPUINFO_INT_REGISTER + M68K_D3:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD3); break;
			case CPUINFO_INT_REGISTER + M68K_D4:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD4); break;
			case CPUINFO_INT_REGISTER + M68K_D5:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD5); break;
			case CPUINFO_INT_REGISTER + M68K_D6:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD6); break;
			case CPUINFO_INT_REGISTER + M68K_D7:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexD7); break;
			case CPUINFO_INT_REGISTER + M68K_A0:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA0); break;
			case CPUINFO_INT_REGISTER + M68K_A1:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA1); break;
			case CPUINFO_INT_REGISTER + M68K_A2:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA2); break;
			case CPUINFO_INT_REGISTER + M68K_A3:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA3); break;
			case CPUINFO_INT_REGISTER + M68K_A4:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA4); break;
			case CPUINFO_INT_REGISTER + M68K_A5:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA5); break;
			case CPUINFO_INT_REGISTER + M68K_A6:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA6); break;
			case CPUINFO_INT_REGISTER + M68K_A7:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexA7); break;
// ���			case CPUINFO_INT_REGISTER + M68K_PREF_ADDR:		info->i = m68ec020_get_reg_ppc(M68K_REG_PREF_ADDR); break;
// ���			case CPUINFO_INT_REGISTER + M68K_PREF_DATA:		info->i = m68ec020_get_reg_ppc(M68K_REG_PREF_DATA); break;

			case CPUINFO_INT_REGISTER + M68K_VBR:  			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexVBR); /* 68010+ */ break;
			case CPUINFO_INT_REGISTER + M68K_SFC:  			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexSFC); /* 68010+ */ break;
			case CPUINFO_INT_REGISTER + M68K_DFC:  			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexDFC); /* 68010+ */ break;
			case CPUINFO_INT_REGISTER + M68K_MSP:			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexMSP); /* 68020+ */ break;
			case CPUINFO_INT_REGISTER + M68K_CACR: 			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexCACR); /* 68020+ */ break;
			case CPUINFO_INT_REGISTER + M68K_CAAR: 			info->i = m68ec020_get_reg_ppc(k68000RegisterIndexCAAR); /* 68020+ */ break;

			/* --- the following bits of info are returned as pointers to data or functions --- */
			case CPUINFO_PTR_SET_INFO:						info->setinfo = m68ec020_set_info_ppc;			break;
			case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m68ec020x_get_context_ppc;	break;
			case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m68ec020_set_context_ppc;	break;
			case CPUINFO_PTR_INIT:							info->init = m68ec020_init_ppc;					break;
			case CPUINFO_PTR_RESET:							info->reset = m68ec020_reset_ppc;				break;
			case CPUINFO_PTR_EXIT:							info->exit = m68ec020_exit_ppc;					break;
			case CPUINFO_PTR_EXECUTE:						info->execute = m68ec020_execute_ppc;			break;
			case CPUINFO_PTR_BURN:							info->burn = NULL;							break;
			case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m68ec020_dasm_ppc;			break;
			case CPUINFO_PTR_IRQ_CALLBACK:					/* fix me */								break;
			case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m68ec020_ICount_ppc;			break;
			case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m68ec020_reg_layout_ppc;				break;
			case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m68ec020_win_layout_ppc;				break;

			/* --- the following bits of info are returned as NULL-terminated strings --- */
			case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "68EC020"); break;
			case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola 68K"); break;
			case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "2.0a"); break;
			case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
			case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright 1998-2000 Aaron Giles.  All rights reserved."); break;

			case CPUINFO_STR_FLAGS:
				sr = m68ec020_get_reg_ppc(k68000RegisterIndexSR);
				sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					sr & 0x8000 ? 'T':'.',
					sr & 0x4000 ? 't':'.',
					sr & 0x2000 ? 'S':'.',
					sr & 0x1000 ? 'M':'.',
					sr & 0x0800 ? '?':'.',
					sr & 0x0400 ? 'I':'.',
					sr & 0x0200 ? 'I':'.',
					sr & 0x0100 ? 'I':'.',
					sr & 0x0080 ? '?':'.',
					sr & 0x0040 ? '?':'.',
					sr & 0x0020 ? '?':'.',
					sr & 0x0010 ? 'X':'.',
					sr & 0x0008 ? 'N':'.',
					sr & 0x0004 ? 'Z':'.',
					sr & 0x0002 ? 'V':'.',
					sr & 0x0001 ? 'C':'.');
				break;

			case CPUINFO_STR_REGISTER + M68K_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexPC)); break;
			case CPUINFO_STR_REGISTER + M68K_SR:  			sprintf(info->s = cpuintrf_temp_str(), "SR :%04X", m68ec020_get_reg_ppc(k68000RegisterIndexSR)); break;
			case CPUINFO_STR_REGISTER + M68K_SP:  			sprintf(info->s = cpuintrf_temp_str(), "SP :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexSP)); break;
			case CPUINFO_STR_REGISTER + M68K_ISP: 			sprintf(info->s = cpuintrf_temp_str(), "ISP:%08X", m68ec020_get_reg_ppc(k68000RegisterIndexISP)); break;
			case CPUINFO_STR_REGISTER + M68K_USP: 			sprintf(info->s = cpuintrf_temp_str(), "USP:%08X", m68ec020_get_reg_ppc(k68000RegisterIndexUSP)); break;
			case CPUINFO_STR_REGISTER + M68K_D0:			sprintf(info->s = cpuintrf_temp_str(), "D0 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD0)); break;
			case CPUINFO_STR_REGISTER + M68K_D1:			sprintf(info->s = cpuintrf_temp_str(), "D1 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD1)); break;
			case CPUINFO_STR_REGISTER + M68K_D2:			sprintf(info->s = cpuintrf_temp_str(), "D2 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD2)); break;
			case CPUINFO_STR_REGISTER + M68K_D3:			sprintf(info->s = cpuintrf_temp_str(), "D3 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD3)); break;
			case CPUINFO_STR_REGISTER + M68K_D4:			sprintf(info->s = cpuintrf_temp_str(), "D4 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD4)); break;
			case CPUINFO_STR_REGISTER + M68K_D5:			sprintf(info->s = cpuintrf_temp_str(), "D5 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD5)); break;
			case CPUINFO_STR_REGISTER + M68K_D6:			sprintf(info->s = cpuintrf_temp_str(), "D6 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD6)); break;
			case CPUINFO_STR_REGISTER + M68K_D7:			sprintf(info->s = cpuintrf_temp_str(), "D7 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexD7)); break;
			case CPUINFO_STR_REGISTER + M68K_A0:			sprintf(info->s = cpuintrf_temp_str(), "A0 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA0)); break;
			case CPUINFO_STR_REGISTER + M68K_A1:			sprintf(info->s = cpuintrf_temp_str(), "A1 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA1)); break;
			case CPUINFO_STR_REGISTER + M68K_A2:			sprintf(info->s = cpuintrf_temp_str(), "A2 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA2)); break;
			case CPUINFO_STR_REGISTER + M68K_A3:			sprintf(info->s = cpuintrf_temp_str(), "A3 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA3)); break;
			case CPUINFO_STR_REGISTER + M68K_A4:			sprintf(info->s = cpuintrf_temp_str(), "A4 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA4)); break;
			case CPUINFO_STR_REGISTER + M68K_A5:			sprintf(info->s = cpuintrf_temp_str(), "A5 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA5)); break;
			case CPUINFO_STR_REGISTER + M68K_A6:			sprintf(info->s = cpuintrf_temp_str(), "A6 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA6)); break;
			case CPUINFO_STR_REGISTER + M68K_A7:			sprintf(info->s = cpuintrf_temp_str(), "A7 :%08X", m68ec020_get_reg_ppc(k68000RegisterIndexA7)); break;
// ���			case CPUINFO_STR_REGISTER + M68K_PREF_ADDR:		sprintf(info->s = cpuintrf_temp_str(), "PAR:%08X", m68ec020_get_reg_ppc(M68K_REG_PREF_ADDR)); break;
// ���			case CPUINFO_STR_REGISTER + M68K_PREF_DATA:		sprintf(info->s = cpuintrf_temp_str(), "PDA:%08X", m68ec020_get_reg_ppc(M68K_REG_PREF_DATA)); break;
			case CPUINFO_STR_REGISTER + M68K_SFC:			sprintf(info->s = cpuintrf_temp_str(), "SFC:%X",   m68ec020_get_reg_ppc(k68000RegisterIndexSFC)); break;
			case CPUINFO_STR_REGISTER + M68K_DFC:			sprintf(info->s = cpuintrf_temp_str(), "DFC:%X",   m68ec020_get_reg_ppc(k68000RegisterIndexDFC)); break;
			case CPUINFO_STR_REGISTER + M68K_VBR:			sprintf(info->s = cpuintrf_temp_str(), "VBR:%08X", m68ec020_get_reg_ppc(k68000RegisterIndexVBR)); break;
			case CPUINFO_STR_REGISTER + M68K_MSP:			sprintf(info->s = cpuintrf_temp_str(), "MSP:%08X", m68ec020_get_reg_ppc(k68000RegisterIndexMSP)); break;
			case CPUINFO_STR_REGISTER + M68K_CACR:			sprintf(info->s = cpuintrf_temp_str(), "CCR:%08X", m68ec020_get_reg_ppc(k68000RegisterIndexCACR)); break;
			case CPUINFO_STR_REGISTER + M68K_CAAR:			sprintf(info->s = cpuintrf_temp_str(), "CAR:%08X", m68ec020_get_reg_ppc(k68000RegisterIndexCAAR)); break;
		}
	}

	void m68ec020_get_info(UINT32 state, union cpuinfo *info)
	{
		// LBO 9/23/04. Always use the C 68ec020 core for now. This works around
		// problems in the PPC core when run against Salamander 2 and various Taito F3 games.
		if (0)
			m68ec020_get_info_ppc (state, info);
		else
			m68ec020_get_info_c (state, info);
	}
	
#endif