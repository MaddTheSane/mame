#define m68000_get_info					m68000_get_info_c
#define m68010_get_info					m68010_get_info_c
#define m68ec020_get_info				m68ec020_get_info_c
#define m68020_get_info					m68020_get_info_c

#define m68k_set_reset_instr_callback	m68k_set_reset_instr_callback_c
#define m68k_set_cmpild_instr_callback	m68k_set_cmpild_instr_callback_c
#define m68k_set_rte_instr_callback		m68k_set_rte_instr_callback_c
#define m68k_set_encrypted_opcode_range	m68k_set_encrypted_opcode_range_c

#include "cpu/m68000/m68kmame.c"
#include "cpu/m68000/m68kcpu.c"
#include "cpu/m68000/m68kops.c"
#include "cpu/m68000/m68kopac.c"
#include "cpu/m68000/m68kopdm.c"
#include "cpu/m68000/m68kopnz.c"

int m68000_ICount;

