//###################################################################################################
//
//
//		Asgard68000OpcodeTable.c
//		This file contains the function prototypes and master opcode table.
//
//		See Asgard68000Core.c for information about the Asgard68000 system.
//
//
//###################################################################################################
//
//		x is the high register
//		y is the low register
//
//		EA modes:
//			iy    - immediate from 1-8
//			ry    - Dy/Ay
//			dy    - Dy
//			ay    - Ay
//			ay0   - (Ay)
//			ayp   - (Ay)+
//			aym   - -(Ay)
//			ayd   - (Ay,d16)
//			ayid  - (Ay,Xn,d16)
//			other - other forms (111)
//
//###################################################################################################

static void abcd_b_aym_axm(void);
static void abcd_b_dy_dx(void);

static void adda_w_ay0_ax(void);
static void adda_w_ayd_ax(void);
static void adda_w_ayid_ax(void);
static void adda_w_aym_ax(void);
static void adda_w_ayp_ax(void);
static void adda_w_ay_ax(void);
static void adda_w_dy_ax(void);
static void adda_w_other_ax(void);
static void adda_l_ay0_ax(void);
static void adda_l_ayd_ax(void);
static void adda_l_ayid_ax(void);
static void adda_l_aym_ax(void);
static void adda_l_ayp_ax(void);
static void adda_l_ay_ax(void);
static void adda_l_dy_ax(void);
static void adda_l_other_ax(void);

static void addi_b_imm_ay0(void);
static void addi_b_imm_ayd(void);
static void addi_b_imm_ayid(void);
static void addi_b_imm_aym(void);
static void addi_b_imm_ayp(void);
static void addi_b_imm_dy(void);
static void addi_b_imm_other(void);
static void addi_w_imm_ay0(void);
static void addi_w_imm_ayd(void);
static void addi_w_imm_ayid(void);
static void addi_w_imm_aym(void);
static void addi_w_imm_ayp(void);
static void addi_w_imm_dy(void);
static void addi_w_imm_other(void);
static void addi_l_imm_ay0(void);
static void addi_l_imm_ayd(void);
static void addi_l_imm_ayid(void);
static void addi_l_imm_aym(void);
static void addi_l_imm_ayp(void);
static void addi_l_imm_dy(void);
static void addi_l_imm_other(void);

static void addq_b_ix_ay(void);
static void addq_b_ix_ay0(void);
static void addq_b_ix_ayd(void);
static void addq_b_ix_ayid(void);
static void addq_b_ix_aym(void);
static void addq_b_ix_ayp(void);
static void addq_b_ix_dy(void);
static void addq_b_ix_other(void);
static void addq_w_ix_ay(void);
static void addq_w_ix_ay0(void);
static void addq_w_ix_ayd(void);
static void addq_w_ix_ayid(void);
static void addq_w_ix_aym(void);
static void addq_w_ix_ayp(void);
static void addq_w_ix_dy(void);
static void addq_w_ix_other(void);
static void addq_l_ix_ay(void);
static void addq_l_ix_ay0(void);
static void addq_l_ix_ayd(void);
static void addq_l_ix_ayid(void);
static void addq_l_ix_aym(void);
static void addq_l_ix_ayp(void);
static void addq_l_ix_dy(void);
static void addq_l_ix_other(void);

static void addx_b_aym_axm(void);
static void addx_b_dy_dx(void);
static void addx_w_aym_axm(void);
static void addx_w_dy_dx(void);
static void addx_l_aym_axm(void);
static void addx_l_dy_dx(void);

static void add_b_ay0_dx(void);
static void add_b_ayd_dx(void);
static void add_b_ayid_dx(void);
static void add_b_aym_dx(void);
static void add_b_ayp_dx(void);
static void add_b_dx_ay0(void);
static void add_b_dx_ayd(void);
static void add_b_dx_ayid(void);
static void add_b_dx_aym(void);
static void add_b_dx_ayp(void);
static void add_b_dx_other(void);
static void add_b_dy_dx(void);
static void add_b_other_dx(void);
static void add_w_ay0_dx(void);
static void add_w_ayd_dx(void);
static void add_w_ayid_dx(void);
static void add_w_aym_dx(void);
static void add_w_ayp_dx(void);
static void add_w_dx_ay0(void);
static void add_w_dx_ayd(void);
static void add_w_dx_ayid(void);
static void add_w_dx_aym(void);
static void add_w_dx_ayp(void);
static void add_w_dx_other(void);
static void add_w_other_dx(void);
static void add_w_ry_dx(void);
static void add_l_ay0_dx(void);
static void add_l_ayd_dx(void);
static void add_l_ayid_dx(void);
static void add_l_aym_dx(void);
static void add_l_ayp_dx(void);
static void add_l_dx_ay0(void);
static void add_l_dx_ayd(void);
static void add_l_dx_ayid(void);
static void add_l_dx_aym(void);
static void add_l_dx_ayp(void);
static void add_l_dx_other(void);
static void add_l_other_dx(void);
static void add_l_ry_dx(void);

static void andi_b_imm_ay0(void);
static void andi_b_imm_ayd(void);
static void andi_b_imm_ayid(void);
static void andi_b_imm_aym(void);
static void andi_b_imm_ayp(void);
static void andi_b_imm_dy(void);
static void andi_b_imm_other(void);
static void andi_w_imm_ay0(void);
static void andi_w_imm_ayd(void);
static void andi_w_imm_ayid(void);
static void andi_w_imm_aym(void);
static void andi_w_imm_ayp(void);
static void andi_w_imm_dy(void);
static void andi_w_imm_other(void);
static void andi_l_imm_ay0(void);
static void andi_l_imm_ayd(void);
static void andi_l_imm_ayid(void);
static void andi_l_imm_aym(void);
static void andi_l_imm_ayp(void);
static void andi_l_imm_dy(void);
static void andi_l_imm_other(void);

static void and_b_ay0_dx(void);
static void and_b_ayd_dx(void);
static void and_b_ayid_dx(void);
static void and_b_aym_dx(void);
static void and_b_ayp_dx(void);
static void and_b_dx_ay0(void);
static void and_b_dx_ayd(void);
static void and_b_dx_ayid(void);
static void and_b_dx_aym(void);
static void and_b_dx_ayp(void);
static void and_b_dx_other(void);
static void and_b_dy_dx(void);
static void and_b_other_dx(void);
static void and_w_ay0_dx(void);
static void and_w_ayd_dx(void);
static void and_w_ayid_dx(void);
static void and_w_aym_dx(void);
static void and_w_ayp_dx(void);
static void and_w_dx_ay0(void);
static void and_w_dx_ayd(void);
static void and_w_dx_ayid(void);
static void and_w_dx_aym(void);
static void and_w_dx_ayp(void);
static void and_w_dx_other(void);
static void and_w_dy_dx(void);
static void and_w_other_dx(void);
static void and_l_ay0_dx(void);
static void and_l_ayd_dx(void);
static void and_l_ayid_dx(void);
static void and_l_aym_dx(void);
static void and_l_ayp_dx(void);
static void and_l_dx_ay0(void);
static void and_l_dx_ayd(void);
static void and_l_dx_ayid(void);
static void and_l_dx_aym(void);
static void and_l_dx_ayp(void);
static void and_l_dx_other(void);
static void and_l_dy_dx(void);
static void and_l_other_dx(void);

static void asl_b_dx_dy(void);
static void asl_b_ix_dy(void);
static void asl_w_ay0(void);
static void asl_w_ayd(void);
static void asl_w_ayid(void);
static void asl_w_aym(void);
static void asl_w_ayp(void);
static void asl_w_dx_dy(void);
static void asl_w_ix_dy(void);
static void asl_w_other(void);
static void asl_l_dx_dy(void);
static void asl_l_ix_dy(void);

static void asr_b_dx_dy(void);
static void asr_b_ix_dy(void);
static void asr_w_ay0(void);
static void asr_w_ayd(void);
static void asr_w_ayid(void);
static void asr_w_aym(void);
static void asr_w_ayp(void);
static void asr_w_dx_dy(void);
static void asr_w_ix_dy(void);
static void asr_w_other(void);
static void asr_l_dx_dy(void);
static void asr_l_ix_dy(void);

static void bcc_cc(void);
static void bcc_cs(void);
static void bcc_eq(void);
static void bcc_ge(void);
static void bcc_gt(void);
static void bcc_hi(void);
static void bcc_le(void);
static void bcc_ls(void);
static void bcc_lt(void);
static void bcc_mi(void);
static void bcc_ne(void);
static void bcc_pl(void);
static void bcc_vc(void);
static void bcc_vs(void);

static void bchg_dx_ay0(void);
static void bchg_dx_ayd(void);
static void bchg_dx_ayid(void);
static void bchg_dx_aym(void);
static void bchg_dx_ayp(void);
static void bchg_dx_dy(void);
static void bchg_dx_other(void);
static void bchg_imm_ay0(void);
static void bchg_imm_ayd(void);
static void bchg_imm_ayid(void);
static void bchg_imm_aym(void);
static void bchg_imm_ayp(void);
static void bchg_imm_dy(void);
static void bchg_imm_other(void);

static void bclr_dx_ay0(void);
static void bclr_dx_ayd(void);
static void bclr_dx_ayid(void);
static void bclr_dx_aym(void);
static void bclr_dx_ayp(void);
static void bclr_dx_dy(void);
static void bclr_dx_other(void);
static void bclr_imm_ay0(void);
static void bclr_imm_ayd(void);
static void bclr_imm_ayid(void);
static void bclr_imm_aym(void);
static void bclr_imm_ayp(void);
static void bclr_imm_dy(void);
static void bclr_imm_other(void);

static void bfchg_dy(void);
static void bfchg_ay0(void);
static void bfchg_ayd(void);
static void bfchg_ayid(void);
static void bfchg_other(void);
static void bfclr_dy(void);
static void bfclr_ay0(void);
static void bfclr_ayd(void);
static void bfclr_ayid(void);
static void bfclr_other(void);
static void bfexts_dy_dx(void);
static void bfexts_ay0_dx(void);
static void bfexts_ayd_dx(void);
static void bfexts_ayid_dx(void);
static void bfexts_other_dx(void);
static void bfextu_dy_dx(void);
static void bfextu_ay0_dx(void);
static void bfextu_ayd_dx(void);
static void bfextu_ayid_dx(void);
static void bfextu_other_dx(void);
static void bfffo_dy_dx(void);
static void bfffo_ay0_dx(void);
static void bfffo_ayd_dx(void);
static void bfffo_ayid_dx(void);
static void bfffo_other_dx(void);
static void bfins_dx_dy(void);
static void bfins_dx_ay0(void);
static void bfins_dx_ayd(void);
static void bfins_dx_ayid(void);
static void bfins_dx_other(void);
static void bfset_dy(void);
static void bfset_ay0(void);
static void bfset_ayd(void);
static void bfset_ayid(void);
static void bfset_other(void);
static void bftst_dy(void);
static void bftst_ay0(void);
static void bftst_ayd(void);
static void bftst_ayid(void);
static void bftst_other(void);

static void bkpt(void);
static void bra(void);

static void bset_dx_ay0(void);
static void bset_dx_ayd(void);
static void bset_dx_ayid(void);
static void bset_dx_aym(void);
static void bset_dx_ayp(void);
static void bset_dx_dy(void);
static void bset_dx_other(void);
static void bset_imm_ay0(void);
static void bset_imm_ayd(void);
static void bset_imm_ayid(void);
static void bset_imm_aym(void);
static void bset_imm_ayp(void);
static void bset_imm_dy(void);
static void bset_imm_other(void);

static void bsr(void);

static void btst_dx_ay0(void);
static void btst_dx_ayd(void);
static void btst_dx_ayid(void);
static void btst_dx_aym(void);
static void btst_dx_ayp(void);
static void btst_dx_dy(void);
static void btst_dx_other(void);
static void btst_imm_ay0(void);
static void btst_imm_ayd(void);
static void btst_imm_ayid(void);
static void btst_imm_aym(void);
static void btst_imm_ayp(void);
static void btst_imm_dy(void);
static void btst_imm_other(void);

static void chk_w_ay0_dx(void);
static void chk_w_ayd_dx(void);
static void chk_w_ayid_dx(void);
static void chk_w_aym_dx(void);
static void chk_w_ayp_dx(void);
static void chk_w_dy_dx(void);
static void chk_w_other_dx(void);

static void chk_l_ay0_dx(void);
static void chk_l_ayd_dx(void);
static void chk_l_ayid_dx(void);
static void chk_l_aym_dx(void);
static void chk_l_ayp_dx(void);
static void chk_l_dy_dx(void);
static void chk_l_other_dx(void);

static void chk2_b_ay0_rx(void);
static void chk2_b_ayd_rx(void);
static void chk2_b_ayid_rx(void);
static void chk2_b_other_rx(void);
static void chk2_w_ay0_rx(void);
static void chk2_w_ayd_rx(void);
static void chk2_w_ayid_rx(void);
static void chk2_w_other_rx(void);
static void chk2_l_ay0_rx(void);
static void chk2_l_ayd_rx(void);
static void chk2_l_ayid_rx(void);
static void chk2_l_other_rx(void);

static void clr_b_ay0(void);
static void clr_b_ayd(void);
static void clr_b_ayid(void);
static void clr_b_aym(void);
static void clr_b_ayp(void);
static void clr_b_dy(void);
static void clr_b_other(void);
static void clr_w_ay0(void);
static void clr_w_ayd(void);
static void clr_w_ayid(void);
static void clr_w_aym(void);
static void clr_w_ayp(void);
static void clr_w_dy(void);
static void clr_w_other(void);
static void clr_l_ay0(void);
static void clr_l_ayd(void);
static void clr_l_ayid(void);
static void clr_l_aym(void);
static void clr_l_ayp(void);
static void clr_l_dy(void);
static void clr_l_other(void);

static void cmpa_w_ay0_ax(void);
static void cmpa_w_ayd_ax(void);
static void cmpa_w_ayid_ax(void);
static void cmpa_w_aym_ax(void);
static void cmpa_w_ayp_ax(void);
static void cmpa_w_ay_ax(void);
static void cmpa_w_dy_ax(void);
static void cmpa_w_other_ax(void);
static void cmpa_l_ay0_ax(void);
static void cmpa_l_ayd_ax(void);
static void cmpa_l_ayid_ax(void);
static void cmpa_l_aym_ax(void);
static void cmpa_l_ayp_ax(void);
static void cmpa_l_ay_ax(void);
static void cmpa_l_dy_ax(void);
static void cmpa_l_other_ax(void);

static void cmpi_b_imm_ay0(void);
static void cmpi_b_imm_ayd(void);
static void cmpi_b_imm_ayid(void);
static void cmpi_b_imm_aym(void);
static void cmpi_b_imm_ayp(void);
static void cmpi_b_imm_dy(void);
static void cmpi_b_imm_other(void);
static void cmpi_w_imm_ay0(void);
static void cmpi_w_imm_ayd(void);
static void cmpi_w_imm_ayid(void);
static void cmpi_w_imm_aym(void);
static void cmpi_w_imm_ayp(void);
static void cmpi_w_imm_dy(void);
static void cmpi_w_imm_other(void);
static void cmpi_l_imm_ay0(void);
static void cmpi_l_imm_ayd(void);
static void cmpi_l_imm_ayid(void);
static void cmpi_l_imm_aym(void);
static void cmpi_l_imm_ayp(void);
static void cmpi_l_imm_dy(void);
static void cmpi_l_imm_other(void);

static void cmpm_b_ayp_axp(void);
static void cmpm_l_ayp_axp(void);
static void cmpm_w_ayp_axp(void);

static void cmp_b_ay0_dx(void);
static void cmp_b_ayd_dx(void);
static void cmp_b_ayid_dx(void);
static void cmp_b_aym_dx(void);
static void cmp_b_ayp_dx(void);
static void cmp_b_dy_dx(void);
static void cmp_b_other_dx(void);
static void cmp_w_ay0_dx(void);
static void cmp_w_ayd_dx(void);
static void cmp_w_ayid_dx(void);
static void cmp_w_aym_dx(void);
static void cmp_w_ayp_dx(void);
static void cmp_w_other_dx(void);
static void cmp_w_ry_dx(void);
static void cmp_l_ay0_dx(void);
static void cmp_l_ayd_dx(void);
static void cmp_l_ayid_dx(void);
static void cmp_l_aym_dx(void);
static void cmp_l_ayp_dx(void);
static void cmp_l_other_dx(void);
static void cmp_l_ry_dx(void);

static void dbcc_cc_dy(void);
static void dbcc_cs_dy(void);
static void dbcc_eq_dy(void);
static void dbcc_f_dy(void);
static void dbcc_ge_dy(void);
static void dbcc_gt_dy(void);
static void dbcc_hi_dy(void);
static void dbcc_le_dy(void);
static void dbcc_ls_dy(void);
static void dbcc_lt_dy(void);
static void dbcc_mi_dy(void);
static void dbcc_ne_dy(void);
static void dbcc_pl_dy(void);
static void dbcc_t_dy(void);
static void dbcc_vc_dy(void);
static void dbcc_vs_dy(void);

static void divs_w_dy_dx(void);
static void divs_w_ay0_dx(void);
static void divs_w_ayp_dx(void);
static void divs_w_aym_dx(void);
static void divs_w_ayd_dx(void);
static void divs_w_ayid_dx(void);
static void divs_w_other_dx(void);
static void divu_w_dy_dx(void);
static void divu_w_ay0_dx(void);
static void divu_w_ayp_dx(void);
static void divu_w_aym_dx(void);
static void divu_w_ayd_dx(void);
static void divu_w_ayid_dx(void);
static void divu_w_other_dx(void);
static void div_l_dy_dx(void);
static void div_l_ay0_dx(void);
static void div_l_ayp_dx(void);
static void div_l_aym_dx(void);
static void div_l_ayd_dx(void);
static void div_l_ayid_dx(void);
static void div_l_other_dx(void);

static void eori_b_imm_ay0(void);
static void eori_b_imm_ayd(void);
static void eori_b_imm_ayid(void);
static void eori_b_imm_aym(void);
static void eori_b_imm_ayp(void);
static void eori_b_imm_dy(void);
static void eori_b_imm_other(void);
static void eori_w_imm_ay0(void);
static void eori_w_imm_ayd(void);
static void eori_w_imm_ayid(void);
static void eori_w_imm_aym(void);
static void eori_w_imm_ayp(void);
static void eori_w_imm_dy(void);
static void eori_w_imm_other(void);
static void eori_l_imm_ay0(void);
static void eori_l_imm_ayd(void);
static void eori_l_imm_ayid(void);
static void eori_l_imm_aym(void);
static void eori_l_imm_ayp(void);
static void eori_l_imm_dy(void);
static void eori_l_imm_other(void);

static void eor_b_dx_ay0(void);
static void eor_b_dx_ayd(void);
static void eor_b_dx_ayid(void);
static void eor_b_dx_aym(void);
static void eor_b_dx_ayp(void);
static void eor_b_dx_dy(void);
static void eor_b_dx_other(void);
static void eor_w_dx_ay0(void);
static void eor_w_dx_ayd(void);
static void eor_w_dx_ayid(void);
static void eor_w_dx_aym(void);
static void eor_w_dx_ayp(void);
static void eor_w_dx_dy(void);
static void eor_w_dx_other(void);
static void eor_l_dx_ay0(void);
static void eor_l_dx_ayd(void);
static void eor_l_dx_ayid(void);
static void eor_l_dx_aym(void);
static void eor_l_dx_ayp(void);
static void eor_l_dx_dy(void);
static void eor_l_dx_other(void);

static void exg_ax_ay(void);
static void exg_dx_ay(void);
static void exg_dx_dy(void);

static void ext_w_dy(void);
static void ext_l_dy(void);
static void extb_l_dy(void);

static void hodgepodge(void);

static void illegal(void);

static void jmp_ay0(void);
static void jmp_ayd(void);
static void jmp_ayid(void);
static void jmp_other(void);

static void jsr_ay0(void);
static void jsr_ayd(void);
static void jsr_ayid(void);
static void jsr_other(void);

static void lea_ay0_ax(void);
static void lea_ayd_ax(void);
static void lea_ayid_ax(void);
static void lea_other_ax(void);

static void link_imm_ay(void);

static void lsl_b_dx_dy(void);
static void lsl_b_ix_dy(void);
static void lsl_w_ay0(void);
static void lsl_w_ayd(void);
static void lsl_w_ayid(void);
static void lsl_w_aym(void);
static void lsl_w_ayp(void);
static void lsl_w_dx_dy(void);
static void lsl_w_ix_dy(void);
static void lsl_w_other(void);
static void lsl_l_dx_dy(void);
static void lsl_l_ix_dy(void);

static void lsr_b_dx_dy(void);
static void lsr_b_ix_dy(void);
static void lsr_w_ay0(void);
static void lsr_w_ayd(void);
static void lsr_w_ayid(void);
static void lsr_w_aym(void);
static void lsr_w_ayp(void);
static void lsr_w_dx_dy(void);
static void lsr_w_ix_dy(void);
static void lsr_w_other(void);
static void lsr_l_dx_dy(void);
static void lsr_l_ix_dy(void);

static void movea_w_ay0_ax(void);
static void movea_w_ayd_ax(void);
static void movea_w_ayid_ax(void);
static void movea_w_aym_ax(void);
static void movea_w_ayp_ax(void);
static void movea_w_other_ax(void);
static void movea_w_ry_ax(void);
static void movea_l_ay0_ax(void);
static void movea_l_ayd_ax(void);
static void movea_l_ayid_ax(void);
static void movea_l_aym_ax(void);
static void movea_l_ayp_ax(void);
static void movea_l_other_ax(void);
static void movea_l_ry_ax(void);

static void movec(void);

static void movem_w_ay0_reg(void);
static void movem_w_ayd_reg(void);
static void movem_w_ayid_reg(void);
static void movem_w_ayp_reg(void);
static void movem_w_other_reg(void);
static void movem_w_reg_ay0(void);
static void movem_w_reg_ayd(void);
static void movem_w_reg_ayid(void);
static void movem_w_reg_aym(void);
static void movem_w_reg_other(void);
static void movem_l_ay0_reg(void);
static void movem_l_ayd_reg(void);
static void movem_l_ayid_reg(void);
static void movem_l_ayp_reg(void);
static void movem_l_other_reg(void);
static void movem_l_reg_ay0(void);
static void movem_l_reg_ayd(void);
static void movem_l_reg_ayid(void);
static void movem_l_reg_aym(void);
static void movem_l_reg_other(void);

static void movep_w_ayd_dx(void);
static void movep_w_dx_ayd(void);
static void movep_l_ayd_dx(void);
static void movep_l_dx_ayd(void);

static void moveq_imm_dx(void);

static void move_b_ay0_ax0(void);
static void move_b_ay0_axd(void);
static void move_b_ay0_axid(void);
static void move_b_ay0_axm(void);
static void move_b_ay0_axp(void);
static void move_b_ay0_dx(void);
static void move_b_ay0_meml(void);
static void move_b_ay0_memw(void);
static void move_b_ayd_ax0(void);
static void move_b_ayd_axd(void);
static void move_b_ayd_axid(void);
static void move_b_ayd_axm(void);
static void move_b_ayd_axp(void);
static void move_b_ayd_dx(void);
static void move_b_ayd_meml(void);
static void move_b_ayd_memw(void);
static void move_b_ayid_ax0(void);
static void move_b_ayid_axd(void);
static void move_b_ayid_axid(void);
static void move_b_ayid_axm(void);
static void move_b_ayid_axp(void);
static void move_b_ayid_dx(void);
static void move_b_ayid_meml(void);
static void move_b_ayid_memw(void);
static void move_b_aym_ax0(void);
static void move_b_aym_axd(void);
static void move_b_aym_axid(void);
static void move_b_aym_axm(void);
static void move_b_aym_axp(void);
static void move_b_aym_dx(void);
static void move_b_aym_meml(void);
static void move_b_aym_memw(void);
static void move_b_ayp_ax0(void);
static void move_b_ayp_axd(void);
static void move_b_ayp_axid(void);
static void move_b_ayp_axm(void);
static void move_b_ayp_axp(void);
static void move_b_ayp_dx(void);
static void move_b_ayp_meml(void);
static void move_b_ayp_memw(void);
static void move_b_dy_ax0(void);
static void move_b_dy_axd(void);
static void move_b_dy_axid(void);
static void move_b_dy_axm(void);
static void move_b_dy_axp(void);
static void move_b_dy_dx(void);
static void move_b_dy_meml(void);
static void move_b_dy_memw(void);
static void move_b_other_ax0(void);
static void move_b_other_axd(void);
static void move_b_other_axid(void);
static void move_b_other_axm(void);
static void move_b_other_axp(void);
static void move_b_other_dx(void);
static void move_b_other_meml(void);
static void move_b_other_memw(void);

static void move_w_ay0_ax0(void);
static void move_w_ay0_axd(void);
static void move_w_ay0_axid(void);
static void move_w_ay0_axm(void);
static void move_w_ay0_axp(void);
static void move_w_ay0_ccr(void);
static void move_w_ay0_dx(void);
static void move_w_ay0_meml(void);
static void move_w_ay0_memw(void);
static void move_w_ay0_sr(void);
static void move_w_ayd_ax0(void);
static void move_w_ayd_axd(void);
static void move_w_ayd_axid(void);
static void move_w_ayd_axm(void);
static void move_w_ayd_axp(void);
static void move_w_ayd_ccr(void);
static void move_w_ayd_dx(void);
static void move_w_ayd_meml(void);
static void move_w_ayd_memw(void);
static void move_w_ayd_sr(void);
static void move_w_ayid_ax0(void);
static void move_w_ayid_axd(void);
static void move_w_ayid_axid(void);
static void move_w_ayid_axm(void);
static void move_w_ayid_axp(void);
static void move_w_ayid_ccr(void);
static void move_w_ayid_dx(void);
static void move_w_ayid_meml(void);
static void move_w_ayid_memw(void);
static void move_w_ayid_sr(void);
static void move_w_aym_ax0(void);
static void move_w_aym_axd(void);
static void move_w_aym_axid(void);
static void move_w_aym_axm(void);
static void move_w_aym_axp(void);
static void move_w_aym_ccr(void);
static void move_w_aym_dx(void);
static void move_w_aym_meml(void);
static void move_w_aym_memw(void);
static void move_w_aym_sr(void);
static void move_w_ayp_ax0(void);
static void move_w_ayp_axd(void);
static void move_w_ayp_axid(void);
static void move_w_ayp_axm(void);
static void move_w_ayp_axp(void);
static void move_w_ayp_ccr(void);
static void move_w_ayp_dx(void);
static void move_w_ayp_meml(void);
static void move_w_ayp_memw(void);
static void move_w_ayp_sr(void);
static void move_w_ccr_ay0(void);
static void move_w_ccr_ayd(void);
static void move_w_ccr_ayid(void);
static void move_w_ccr_aym(void);
static void move_w_ccr_ayp(void);
static void move_w_ccr_dy(void);
static void move_w_ccr_other(void);
static void move_w_dy_ccr(void);
static void move_w_dy_sr(void);
static void move_w_other_ax0(void);
static void move_w_other_axd(void);
static void move_w_other_axid(void);
static void move_w_other_axm(void);
static void move_w_other_axp(void);
static void move_w_other_ccr(void);
static void move_w_other_dx(void);
static void move_w_other_meml(void);
static void move_w_other_memw(void);
static void move_w_other_sr(void);
static void move_w_ry_ax0(void);
static void move_w_ry_axd(void);
static void move_w_ry_axid(void);
static void move_w_ry_axm(void);
static void move_w_ry_axp(void);
static void move_w_ry_dx(void);
static void move_w_ry_meml(void);
static void move_w_ry_memw(void);
static void move_w_sr_ay0(void);
static void move_w_sr_ayd(void);
static void move_w_sr_ayid(void);
static void move_w_sr_aym(void);
static void move_w_sr_ayp(void);
static void move_w_sr_dy(void);
static void move_w_sr_other(void);

static void move_l_ay_usp(void);
static void move_l_ay0_ax0(void);
static void move_l_ay0_axd(void);
static void move_l_ay0_axid(void);
static void move_l_ay0_axm(void);
static void move_l_ay0_axp(void);
static void move_l_ay0_dx(void);
static void move_l_ay0_meml(void);
static void move_l_ay0_memw(void);
static void move_l_ayd_ax0(void);
static void move_l_ayd_axd(void);
static void move_l_ayd_axid(void);
static void move_l_ayd_axm(void);
static void move_l_ayd_axp(void);
static void move_l_ayd_dx(void);
static void move_l_ayd_meml(void);
static void move_l_ayd_memw(void);
static void move_l_ayid_ax0(void);
static void move_l_ayid_axd(void);
static void move_l_ayid_axid(void);
static void move_l_ayid_axm(void);
static void move_l_ayid_axp(void);
static void move_l_ayid_dx(void);
static void move_l_ayid_meml(void);
static void move_l_ayid_memw(void);
static void move_l_aym_ax0(void);
static void move_l_aym_axd(void);
static void move_l_aym_axid(void);
static void move_l_aym_axm(void);
static void move_l_aym_axp(void);
static void move_l_aym_dx(void);
static void move_l_aym_meml(void);
static void move_l_aym_memw(void);
static void move_l_ayp_ax0(void);
static void move_l_ayp_axd(void);
static void move_l_ayp_axid(void);
static void move_l_ayp_axm(void);
static void move_l_ayp_axp(void);
static void move_l_ayp_dx(void);
static void move_l_ayp_meml(void);
static void move_l_ayp_memw(void);
static void move_l_other_ax0(void);
static void move_l_other_ax0(void);
static void move_l_other_axd(void);
static void move_l_other_axd(void);
static void move_l_other_axid(void);
static void move_l_other_axid(void);
static void move_l_other_axm(void);
static void move_l_other_axm(void);
static void move_l_other_axp(void);
static void move_l_other_axp(void);
static void move_l_other_dx(void);
static void move_l_other_dx(void);
static void move_l_other_meml(void);
static void move_l_other_memw(void);
static void move_l_ry_ax0(void);
static void move_l_ry_ax0(void);
static void move_l_ry_axd(void);
static void move_l_ry_axd(void);
static void move_l_ry_axid(void);
static void move_l_ry_axid(void);
static void move_l_ry_axm(void);
static void move_l_ry_axm(void);
static void move_l_ry_axp(void);
static void move_l_ry_axp(void);
static void move_l_ry_dx(void);
static void move_l_ry_dx(void);
static void move_l_ry_meml(void);
static void move_l_ry_meml(void);
static void move_l_ry_memw(void);
static void move_l_ry_memw(void);
static void move_l_usp_ay(void);

static void muls_w_dy_dx(void);
static void muls_w_ay0_dx(void);
static void muls_w_ayp_dx(void);
static void muls_w_aym_dx(void);
static void muls_w_ayd_dx(void);
static void muls_w_ayid_dx(void);
static void muls_w_other_dx(void);
static void mulu_w_dy_dx(void);
static void mulu_w_ay0_dx(void);
static void mulu_w_ayp_dx(void);
static void mulu_w_aym_dx(void);
static void mulu_w_ayd_dx(void);
static void mulu_w_ayid_dx(void);
static void mulu_w_other_dx(void);
static void mul_l_dy_dx(void);
static void mul_l_ay0_dx(void);
static void mul_l_ayp_dx(void);
static void mul_l_aym_dx(void);
static void mul_l_ayd_dx(void);
static void mul_l_ayid_dx(void);
static void mul_l_other_dx(void);

static void nbcd_b_ay0(void);
static void nbcd_b_ayd(void);
static void nbcd_b_ayid(void);
static void nbcd_b_aym(void);
static void nbcd_b_ayp(void);
static void nbcd_b_dy(void);
static void nbcd_b_other(void);

static void negx_b_ay0(void);
static void negx_b_ayd(void);
static void negx_b_ayid(void);
static void negx_b_aym(void);
static void negx_b_ayp(void);
static void negx_b_dy(void);
static void negx_b_other(void);
static void negx_w_ay0(void);
static void negx_w_ayd(void);
static void negx_w_ayid(void);
static void negx_w_aym(void);
static void negx_w_ayp(void);
static void negx_w_dy(void);
static void negx_w_other(void);
static void negx_l_ay0(void);
static void negx_l_ayd(void);
static void negx_l_ayid(void);
static void negx_l_aym(void);
static void negx_l_ayp(void);
static void negx_l_dy(void);
static void negx_l_other(void);

static void neg_b_ay0(void);
static void neg_b_ayd(void);
static void neg_b_ayid(void);
static void neg_b_aym(void);
static void neg_b_ayp(void);
static void neg_b_dy(void);
static void neg_b_other(void);
static void neg_w_ay0(void);
static void neg_w_ayd(void);
static void neg_w_ayid(void);
static void neg_w_aym(void);
static void neg_w_ayp(void);
static void neg_w_dy(void);
static void neg_w_other(void);
static void neg_l_ay0(void);
static void neg_l_ayd(void);
static void neg_l_ayid(void);
static void neg_l_aym(void);
static void neg_l_ayp(void);
static void neg_l_dy(void);
static void neg_l_other(void);

static void not_b_ay0(void);
static void not_b_ayd(void);
static void not_b_ayid(void);
static void not_b_aym(void);
static void not_b_ayp(void);
static void not_b_dy(void);
static void not_b_other(void);
static void not_w_ay0(void);
static void not_w_ayd(void);
static void not_w_ayid(void);
static void not_w_aym(void);
static void not_w_ayp(void);
static void not_w_dy(void);
static void not_w_other(void);
static void not_l_ay0(void);
static void not_l_ayd(void);
static void not_l_ayid(void);
static void not_l_aym(void);
static void not_l_ayp(void);
static void not_l_dy(void);
static void not_l_other(void);

static void ori_b_imm_ay0(void);
static void ori_b_imm_ayd(void);
static void ori_b_imm_ayid(void);
static void ori_b_imm_aym(void);
static void ori_b_imm_ayp(void);
static void ori_b_imm_dy(void);
static void ori_b_imm_other(void);
static void ori_w_imm_ay0(void);
static void ori_w_imm_ayd(void);
static void ori_w_imm_ayid(void);
static void ori_w_imm_aym(void);
static void ori_w_imm_ayp(void);
static void ori_w_imm_dy(void);
static void ori_w_imm_other(void);
static void ori_l_imm_ay0(void);
static void ori_l_imm_ayd(void);
static void ori_l_imm_ayid(void);
static void ori_l_imm_aym(void);
static void ori_l_imm_ayp(void);
static void ori_l_imm_dy(void);
static void ori_l_imm_other(void);

static void or_b_ay0_dx(void);
static void or_b_ayd_dx(void);
static void or_b_ayid_dx(void);
static void or_b_aym_dx(void);
static void or_b_ayp_dx(void);
static void or_b_dx_ay0(void);
static void or_b_dx_ayd(void);
static void or_b_dx_ayid(void);
static void or_b_dx_aym(void);
static void or_b_dx_ayp(void);
static void or_b_dx_other(void);
static void or_b_dy_dx(void);
static void or_b_other_dx(void);
static void or_w_ay0_dx(void);
static void or_w_ayd_dx(void);
static void or_w_ayid_dx(void);
static void or_w_aym_dx(void);
static void or_w_ayp_dx(void);
static void or_w_dx_ay0(void);
static void or_w_dx_ayd(void);
static void or_w_dx_ayid(void);
static void or_w_dx_aym(void);
static void or_w_dx_ayp(void);
static void or_w_dx_other(void);
static void or_w_dy_dx(void);
static void or_w_other_dx(void);
static void or_l_ay0_dx(void);
static void or_l_ayd_dx(void);
static void or_l_ayid_dx(void);
static void or_l_aym_dx(void);
static void or_l_ayp_dx(void);
static void or_l_dx_ay0(void);
static void or_l_dx_ayd(void);
static void or_l_dx_ayid(void);
static void or_l_dx_aym(void);
static void or_l_dx_ayp(void);
static void or_l_dx_other(void);
static void or_l_dy_dx(void);
static void or_l_other_dx(void);

static void pea_ay0(void);
static void pea_ayd(void);
static void pea_ayid(void);
static void pea_other(void);

static void rol_b_dx_dy(void);
static void rol_b_ix_dy(void);
static void rol_w_ay0(void);
static void rol_w_ayd(void);
static void rol_w_ayid(void);
static void rol_w_aym(void);
static void rol_w_ayp(void);
static void rol_w_dx_dy(void);
static void rol_w_ix_dy(void);
static void rol_w_other(void);
static void rol_l_dx_dy(void);
static void rol_l_ix_dy(void);

static void ror_b_dx_dy(void);
static void ror_b_ix_dy(void);
static void ror_w_ay0(void);
static void ror_w_ayd(void);
static void ror_w_ayid(void);
static void ror_w_aym(void);
static void ror_w_ayp(void);
static void ror_w_dx_dy(void);
static void ror_w_ix_dy(void);
static void ror_w_other(void);
static void ror_l_dx_dy(void);
static void ror_l_ix_dy(void);

static void roxl_b_dx_dy(void);
static void roxl_b_ix_dy(void);
static void roxl_w_ay0(void);
static void roxl_w_ayd(void);
static void roxl_w_ayid(void);
static void roxl_w_aym(void);
static void roxl_w_ayp(void);
static void roxl_w_dx_dy(void);
static void roxl_w_dx_dy(void);
static void roxl_w_ix_dy(void);
static void roxl_w_other(void);
static void roxl_l_dx_dy(void);
static void roxl_l_ix_dy(void);

static void roxr_b_dx_dy(void);
static void roxr_b_ix_dy(void);
static void roxr_w_ay0(void);
static void roxr_w_ayd(void);
static void roxr_w_ayid(void);
static void roxr_w_aym(void);
static void roxr_w_ayp(void);
static void roxr_w_dx_dy(void);
static void roxr_w_ix_dy(void);
static void roxr_w_other(void);
static void roxr_l_dx_dy(void);
static void roxr_l_ix_dy(void);

static void sbcd_b_aym_axm(void);
static void sbcd_b_dy_dx(void);

static void scc_cc_ay0(void);
static void scc_cc_ayd(void);
static void scc_cc_ayid(void);
static void scc_cc_aym(void);
static void scc_cc_ayp(void);
static void scc_cc_dy(void);
static void scc_cc_other(void);
static void scc_cs_ay0(void);
static void scc_cs_ayd(void);
static void scc_cs_ayid(void);
static void scc_cs_aym(void);
static void scc_cs_ayp(void);
static void scc_cs_dy(void);
static void scc_cs_other(void);
static void scc_eq_ay0(void);
static void scc_eq_ayd(void);
static void scc_eq_ayid(void);
static void scc_eq_aym(void);
static void scc_eq_ayp(void);
static void scc_eq_dy(void);
static void scc_eq_other(void);
static void scc_f_ay0(void);
static void scc_f_ayd(void);
static void scc_f_ayid(void);
static void scc_f_aym(void);
static void scc_f_ayp(void);
static void scc_f_dy(void);
static void scc_f_other(void);
static void scc_ge_ay0(void);
static void scc_ge_ayd(void);
static void scc_ge_ayid(void);
static void scc_ge_aym(void);
static void scc_ge_ayp(void);
static void scc_ge_dy(void);
static void scc_ge_other(void);
static void scc_gt_ay0(void);
static void scc_gt_ayd(void);
static void scc_gt_ayid(void);
static void scc_gt_aym(void);
static void scc_gt_ayp(void);
static void scc_gt_dy(void);
static void scc_gt_other(void);
static void scc_hi_ay0(void);
static void scc_hi_ayd(void);
static void scc_hi_ayid(void);
static void scc_hi_aym(void);
static void scc_hi_ayp(void);
static void scc_hi_dy(void);
static void scc_hi_other(void);
static void scc_le_ay0(void);
static void scc_le_ayd(void);
static void scc_le_ayid(void);
static void scc_le_aym(void);
static void scc_le_ayp(void);
static void scc_le_dy(void);
static void scc_le_other(void);
static void scc_ls_ay0(void);
static void scc_ls_ayd(void);
static void scc_ls_ayid(void);
static void scc_ls_aym(void);
static void scc_ls_ayp(void);
static void scc_ls_dy(void);
static void scc_ls_other(void);
static void scc_lt_ay0(void);
static void scc_lt_ayd(void);
static void scc_lt_ayid(void);
static void scc_lt_aym(void);
static void scc_lt_ayp(void);
static void scc_lt_dy(void);
static void scc_lt_other(void);
static void scc_mi_ay0(void);
static void scc_mi_ayd(void);
static void scc_mi_ayid(void);
static void scc_mi_aym(void);
static void scc_mi_ayp(void);
static void scc_mi_dy(void);
static void scc_mi_other(void);
static void scc_ne_ay0(void);
static void scc_ne_ayd(void);
static void scc_ne_ayid(void);
static void scc_ne_aym(void);
static void scc_ne_ayp(void);
static void scc_ne_dy(void);
static void scc_ne_other(void);
static void scc_pl_ay0(void);
static void scc_pl_ayd(void);
static void scc_pl_ayid(void);
static void scc_pl_aym(void);
static void scc_pl_ayp(void);
static void scc_pl_dy(void);
static void scc_pl_other(void);
static void scc_t_ay0(void);
static void scc_t_ayd(void);
static void scc_t_ayid(void);
static void scc_t_aym(void);
static void scc_t_ayp(void);
static void scc_t_dy(void);
static void scc_t_other(void);
static void scc_vc_ay0(void);
static void scc_vc_ayd(void);
static void scc_vc_ayid(void);
static void scc_vc_aym(void);
static void scc_vc_ayp(void);
static void scc_vc_dy(void);
static void scc_vc_other(void);
static void scc_vs_ay0(void);
static void scc_vs_ayd(void);
static void scc_vs_ayid(void);
static void scc_vs_aym(void);
static void scc_vs_ayp(void);
static void scc_vs_dy(void);
static void scc_vs_other(void);

static void suba_w_ay0_ax(void);
static void suba_w_ayd_ax(void);
static void suba_w_ayid_ax(void);
static void suba_w_aym_ax(void);
static void suba_w_ayp_ax(void);
static void suba_w_ay_ax(void);
static void suba_w_dy_ax(void);
static void suba_w_other_ax(void);
static void suba_l_ay0_ax(void);
static void suba_l_ayd_ax(void);
static void suba_l_ayid_ax(void);
static void suba_l_aym_ax(void);
static void suba_l_ayp_ax(void);
static void suba_l_ay_ax(void);
static void suba_l_dy_ax(void);
static void suba_l_other_ax(void);

static void subi_b_imm_ay0(void);
static void subi_b_imm_ayd(void);
static void subi_b_imm_ayid(void);
static void subi_b_imm_aym(void);
static void subi_b_imm_ayp(void);
static void subi_b_imm_dy(void);
static void subi_b_imm_other(void);
static void subi_w_imm_ay0(void);
static void subi_w_imm_ayd(void);
static void subi_w_imm_ayid(void);
static void subi_w_imm_aym(void);
static void subi_w_imm_ayp(void);
static void subi_w_imm_dy(void);
static void subi_w_imm_other(void);
static void subi_l_imm_ay0(void);
static void subi_l_imm_ayd(void);
static void subi_l_imm_ayid(void);
static void subi_l_imm_aym(void);
static void subi_l_imm_ayp(void);
static void subi_l_imm_dy(void);
static void subi_l_imm_other(void);

static void subq_b_ix_ay(void);
static void subq_b_ix_ay0(void);
static void subq_b_ix_ayd(void);
static void subq_b_ix_ayid(void);
static void subq_b_ix_aym(void);
static void subq_b_ix_ayp(void);
static void subq_b_ix_dy(void);
static void subq_b_ix_other(void);
static void subq_w_ix_ay(void);
static void subq_w_ix_ay0(void);
static void subq_w_ix_ayd(void);
static void subq_w_ix_ayid(void);
static void subq_w_ix_aym(void);
static void subq_w_ix_ayp(void);
static void subq_w_ix_dy(void);
static void subq_w_ix_other(void);
static void subq_l_ix_ay(void);
static void subq_l_ix_ay0(void);
static void subq_l_ix_ayd(void);
static void subq_l_ix_ayid(void);
static void subq_l_ix_aym(void);
static void subq_l_ix_ayp(void);
static void subq_l_ix_dy(void);
static void subq_l_ix_other(void);

static void subx_b_aym_axm(void);
static void subx_b_dy_dx(void);
static void subx_w_aym_axm(void);
static void subx_w_dy_dx(void);
static void subx_l_aym_axm(void);
static void subx_l_dy_dx(void);

static void sub_b_ay0_dx(void);
static void sub_b_ayd_dx(void);
static void sub_b_ayid_dx(void);
static void sub_b_aym_dx(void);
static void sub_b_ayp_dx(void);
static void sub_b_dx_ay0(void);
static void sub_b_dx_ayd(void);
static void sub_b_dx_ayid(void);
static void sub_b_dx_aym(void);
static void sub_b_dx_ayp(void);
static void sub_b_dx_other(void);
static void sub_b_dy_dx(void);
static void sub_b_other_dx(void);
static void sub_w_ay0_dx(void);
static void sub_w_ayd_dx(void);
static void sub_w_ayid_dx(void);
static void sub_w_aym_dx(void);
static void sub_w_ayp_dx(void);
static void sub_w_dx_ay0(void);
static void sub_w_dx_ayd(void);
static void sub_w_dx_ayid(void);
static void sub_w_dx_aym(void);
static void sub_w_dx_ayp(void);
static void sub_w_dx_other(void);
static void sub_w_other_dx(void);
static void sub_w_ry_dx(void);
static void sub_l_ay0_dx(void);
static void sub_l_ayd_dx(void);
static void sub_l_ayid_dx(void);
static void sub_l_aym_dx(void);
static void sub_l_ayp_dx(void);
static void sub_l_dx_ay0(void);
static void sub_l_dx_ayd(void);
static void sub_l_dx_ayid(void);
static void sub_l_dx_aym(void);
static void sub_l_dx_ayp(void);
static void sub_l_dx_other(void);
static void sub_l_other_dx(void);
static void sub_l_ry_dx(void);

static void swap_dy(void);

static void tas_b_ay0(void);
static void tas_b_ayd(void);
static void tas_b_ayid(void);
static void tas_b_aym(void);
static void tas_b_ayp(void);
static void tas_b_dy(void);
static void tas_b_other(void);

static void trap_imm(void);

static void tst_b_ay0(void);
static void tst_b_ayd(void);
static void tst_b_ayid(void);
static void tst_b_aym(void);
static void tst_b_ayp(void);
static void tst_b_dy(void);
static void tst_b_other(void);
static void tst_w_ay0(void);
static void tst_w_ayd(void);
static void tst_w_ayid(void);
static void tst_w_aym(void);
static void tst_w_ayp(void);
static void tst_w_ay(void);
static void tst_w_dy(void);
static void tst_w_other(void);
static void tst_l_ay0(void);
static void tst_l_ayd(void);
static void tst_l_ayid(void);
static void tst_l_aym(void);
static void tst_l_ayp(void);
static void tst_l_ay(void);
static void tst_l_dy(void);
static void tst_l_other(void);

static void unlk_ay(void);


static void *opcodes[0x2000] =
{
	//***************************************************************************************************
	//
	// 0000 - 00FF									
	ori_b_imm_dy,	illegal,		ori_b_imm_ay0,	ori_b_imm_ayp,	ori_b_imm_aym,	ori_b_imm_ayd,	ori_b_imm_ayid,	ori_b_imm_other,
	ori_w_imm_dy,	illegal,		ori_w_imm_ay0,	ori_w_imm_ayp,	ori_w_imm_aym,	ori_w_imm_ayd,	ori_w_imm_ayid,	ori_w_imm_other,
	ori_l_imm_dy,	illegal,		ori_l_imm_ay0,	ori_l_imm_ayp,	ori_l_imm_aym,	ori_l_imm_ayd,	ori_l_imm_ayid,	ori_l_imm_other,
	illegal,		illegal,		chk2_b_ay0_rx,	illegal,		illegal,		chk2_b_ayd_rx,	chk2_b_ayid_rx,	chk2_b_other_rx,		
	// 0100 - 01FF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0200 - 02FF
	andi_b_imm_dy,	illegal,		andi_b_imm_ay0,	andi_b_imm_ayp,	andi_b_imm_aym,	andi_b_imm_ayd,	andi_b_imm_ayid,andi_b_imm_other,
	andi_w_imm_dy,	illegal,		andi_w_imm_ay0,	andi_w_imm_ayp,	andi_w_imm_aym,	andi_w_imm_ayd,	andi_w_imm_ayid,andi_w_imm_other,
	andi_l_imm_dy,	illegal,		andi_l_imm_ay0,	andi_l_imm_ayp,	andi_l_imm_aym,	andi_l_imm_ayd,	andi_l_imm_ayid,andi_l_imm_other,
	illegal,		illegal,		chk2_w_ay0_rx,	illegal,		illegal,		chk2_w_ayd_rx,	chk2_w_ayid_rx,	chk2_w_other_rx,		
	// 0300 - 03FF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0400 - 04FF
	subi_b_imm_dy,	illegal,		subi_b_imm_ay0,	subi_b_imm_ayp,	subi_b_imm_aym,	subi_b_imm_ayd,	subi_b_imm_ayid,subi_b_imm_other,
	subi_w_imm_dy,	illegal,		subi_w_imm_ay0,	subi_w_imm_ayp,	subi_w_imm_aym,	subi_w_imm_ayd,	subi_w_imm_ayid,subi_w_imm_other,
	subi_l_imm_dy,	illegal,		subi_l_imm_ay0,	subi_l_imm_ayp,	subi_l_imm_aym,	subi_l_imm_ayd,	subi_l_imm_ayid,subi_l_imm_other,
	illegal,		illegal,		chk2_l_ay0_rx,	illegal,		illegal,		chk2_l_ayd_rx,	chk2_l_ayid_rx,	chk2_l_other_rx,		
	// 0500 - 05FF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0600 - 06FF
	addi_b_imm_dy,	illegal,		addi_b_imm_ay0,	addi_b_imm_ayp,	addi_b_imm_aym,	addi_b_imm_ayd,	addi_b_imm_ayid,addi_b_imm_other,
	addi_w_imm_dy,	illegal,		addi_w_imm_ay0,	addi_w_imm_ayp,	addi_w_imm_aym,	addi_w_imm_ayd,	addi_w_imm_ayid,addi_w_imm_other,
	addi_l_imm_dy,	illegal,		addi_l_imm_ay0,	addi_l_imm_ayp,	addi_l_imm_aym,	addi_l_imm_ayd,	addi_l_imm_ayid,addi_l_imm_other,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	// 0700 - 07FF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0800 - 08FF
	btst_imm_dy,	illegal,		btst_imm_ay0,	btst_imm_ayp,	btst_imm_aym,	btst_imm_ayd,	btst_imm_ayid,	btst_imm_other,	
	bchg_imm_dy,	illegal,		bchg_imm_ay0,	bchg_imm_ayp,	bchg_imm_aym,	bchg_imm_ayd,	bchg_imm_ayid,	bchg_imm_other,	
	bclr_imm_dy,	illegal,		bclr_imm_ay0,	bclr_imm_ayp,	bclr_imm_aym,	bclr_imm_ayd,	bclr_imm_ayid,	bclr_imm_other,	
	bset_imm_dy,	illegal,		bset_imm_ay0,	bset_imm_ayp,	bset_imm_aym,	bset_imm_ayd,	bset_imm_ayid,	bset_imm_other,	
	// 0900 - 09FF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0A00 - 0AFF
	eori_b_imm_dy,	illegal,		eori_b_imm_ay0,	eori_b_imm_ayp,	eori_b_imm_aym,	eori_b_imm_ayd,	eori_b_imm_ayid,eori_b_imm_other,
	eori_w_imm_dy,	illegal,		eori_w_imm_ay0,	eori_w_imm_ayp,	eori_w_imm_aym,	eori_w_imm_ayd,	eori_w_imm_ayid,eori_w_imm_other,
	eori_l_imm_dy,	illegal,		eori_l_imm_ay0,	eori_l_imm_ayp,	eori_l_imm_aym,	eori_l_imm_ayd,	eori_l_imm_ayid,eori_l_imm_other,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	// 0B00 - 0BFF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0C00 - 0CFF
	cmpi_b_imm_dy,	illegal,		cmpi_b_imm_ay0,	cmpi_b_imm_ayp,	cmpi_b_imm_aym,	cmpi_b_imm_ayd,	cmpi_b_imm_ayid,cmpi_b_imm_other,
	cmpi_w_imm_dy,	illegal,		cmpi_w_imm_ay0,	cmpi_w_imm_ayp,	cmpi_w_imm_aym,	cmpi_w_imm_ayd,	cmpi_w_imm_ayid,cmpi_w_imm_other,
	cmpi_l_imm_dy,	illegal,		cmpi_l_imm_ay0,	cmpi_l_imm_ayp,	cmpi_l_imm_aym,	cmpi_l_imm_ayd,	cmpi_l_imm_ayid,cmpi_l_imm_other,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	// 0D00 - 0DFF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,
	// 0E00 - 0EFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		
	// 0F00 - 0FFF
	btst_dx_dy,		movep_w_ayd_dx,	btst_dx_ay0,	btst_dx_ayp,	btst_dx_aym,	btst_dx_ayd,	btst_dx_ayid,	btst_dx_other,
	bchg_dx_dy,		movep_l_ayd_dx,	bchg_dx_ay0,	bchg_dx_ayp,	bchg_dx_aym,	bchg_dx_ayd,	bchg_dx_ayid,	bchg_dx_other,
	bclr_dx_dy,		movep_w_dx_ayd,	bclr_dx_ay0,	bclr_dx_ayp,	bclr_dx_aym,	bclr_dx_ayd,	bclr_dx_ayid,	bclr_dx_other,
	bset_dx_dy,		movep_l_dx_ayd,	bset_dx_ay0,	bset_dx_ayp,	bset_dx_aym,	bset_dx_ayd,	bset_dx_ayid,	bset_dx_other,

	//***************************************************************************************************
	//
	// 1000 - 10FF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1100 - 11FF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	move_b_dy_memw,	illegal,		move_b_ay0_memw,move_b_ayp_memw,move_b_aym_memw,move_b_ayd_memw,move_b_ayid_memw,move_b_other_memw,
	// 1200 - 12FF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1300 - 13FF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	move_b_dy_meml,	illegal,		move_b_ay0_meml,move_b_ayp_meml,move_b_aym_meml,move_b_ayd_meml,move_b_ayid_meml,move_b_other_meml,
	// 1400 - 14FF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1500 - 15FF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 1600 - 16FF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1700 - 17FF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 1800 - 18FF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1900 - 19FF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 1A00 - 1AFF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1B00 - 1BFF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 1C00 - 1CFF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1D00 - 1DFF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 1E00 - 1EFF
	move_b_dy_dx,	illegal,		move_b_ay0_dx,	move_b_ayp_dx,	move_b_aym_dx,	move_b_ayd_dx,	move_b_ayid_dx,	move_b_other_dx,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	move_b_dy_ax0,	illegal,		move_b_ay0_ax0,	move_b_ayp_ax0,	move_b_aym_ax0,	move_b_ayd_ax0,	move_b_ayid_ax0,move_b_other_ax0,
	move_b_dy_axp,	illegal,		move_b_ay0_axp,	move_b_ayp_axp,	move_b_aym_axp,	move_b_ayd_axp,	move_b_ayid_axp,move_b_other_axp,
	// 1F00 - 1FFF
	move_b_dy_axm,	illegal,		move_b_ay0_axm,	move_b_ayp_axm,	move_b_aym_axm,	move_b_ayd_axm,	move_b_ayid_axm,move_b_other_axm,
	move_b_dy_axd,	illegal,		move_b_ay0_axd,	move_b_ayp_axd,	move_b_aym_axd,	move_b_ayd_axd,	move_b_ayid_axd,move_b_other_axd,
	move_b_dy_axid,	illegal,		move_b_ay0_axid,move_b_ayp_axid,move_b_aym_axid,move_b_ayd_axid,move_b_ayid_axid,move_b_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	

	//***************************************************************************************************
	//
	// 2000 - 20FF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2100 - 21FF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	move_l_ry_memw,	move_l_ry_memw,	move_l_ay0_memw,move_l_ayp_memw,move_l_aym_memw,move_l_ayd_memw,move_l_ayid_memw,move_l_other_memw,
	// 2200 - 22FF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2300 - 23FF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	move_l_ry_meml,	move_l_ry_meml,	move_l_ay0_meml,move_l_ayp_meml,move_l_aym_meml,move_l_ayd_meml,move_l_ayid_meml,move_l_other_meml,
	// 2400 - 24FF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2500 - 25FF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 2600 - 26FF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2700 - 27FF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 2800 - 28FF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2900 - 29FF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 2A00 - 2AFF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2B00 - 2BFF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 2C00 - 2CFF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2D00 - 2DFF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 2E00 - 2EFF
	move_l_ry_dx,	move_l_ry_dx,	move_l_ay0_dx,	move_l_ayp_dx,	move_l_aym_dx,	move_l_ayd_dx,	move_l_ayid_dx,	move_l_other_dx,
	movea_l_ry_ax,	movea_l_ry_ax,	movea_l_ay0_ax,	movea_l_ayp_ax,	movea_l_aym_ax,	movea_l_ayd_ax,	movea_l_ayid_ax,movea_l_other_ax,
	move_l_ry_ax0,	move_l_ry_ax0,	move_l_ay0_ax0,	move_l_ayp_ax0,	move_l_aym_ax0,	move_l_ayd_ax0,	move_l_ayid_ax0,move_l_other_ax0,
	move_l_ry_axp,	move_l_ry_axp,	move_l_ay0_axp,	move_l_ayp_axp,	move_l_aym_axp,	move_l_ayd_axp,	move_l_ayid_axp,move_l_other_axp,
	// 2F00 - 2FFF
	move_l_ry_axm,	move_l_ry_axm,	move_l_ay0_axm,	move_l_ayp_axm,	move_l_aym_axm,	move_l_ayd_axm,	move_l_ayid_axm,move_l_other_axm,
	move_l_ry_axd,	move_l_ry_axd,	move_l_ay0_axd,	move_l_ayp_axd,	move_l_aym_axd,	move_l_ayd_axd,	move_l_ayid_axd,move_l_other_axd,
	move_l_ry_axid,	move_l_ry_axid,	move_l_ay0_axid,move_l_ayp_axid,move_l_aym_axid,move_l_ayd_axid,move_l_ayid_axid,move_l_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	

	//***************************************************************************************************
	//
	// 3000 - 30FF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3100 - 31FF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	move_w_ry_memw,	move_w_ry_memw,	move_w_ay0_memw,move_w_ayp_memw,move_w_aym_memw,move_w_ayd_memw,move_w_ayid_memw,move_w_other_memw,
	// 3200 - 32FF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3300 - 33FF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	move_w_ry_meml,	move_w_ry_meml,	move_w_ay0_meml,move_w_ayp_meml,move_w_aym_meml,move_w_ayd_meml,move_w_ayid_meml,move_w_other_meml,
	// 3400 - 34FF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3500 - 35FF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 3600 - 36FF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3700 - 37FF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 3800 - 38FF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3900 - 39FF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 3A00 - 3AFF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3B00 - 3BFF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 3C00 - 3CFF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3D00 - 3DFF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	// 3E00 - 3EFF
	move_w_ry_dx,	move_w_ry_dx,	move_w_ay0_dx,	move_w_ayp_dx,	move_w_aym_dx,	move_w_ayd_dx,	move_w_ayid_dx,	move_w_other_dx,
	movea_w_ry_ax,	movea_w_ry_ax,	movea_w_ay0_ax,	movea_w_ayp_ax,	movea_w_aym_ax,	movea_w_ayd_ax,	movea_w_ayid_ax,movea_w_other_ax,
	move_w_ry_ax0,	move_w_ry_ax0,	move_w_ay0_ax0,	move_w_ayp_ax0,	move_w_aym_ax0,	move_w_ayd_ax0,	move_w_ayid_ax0,move_w_other_ax0,
	move_w_ry_axp,	move_w_ry_axp,	move_w_ay0_axp,	move_w_ayp_axp,	move_w_aym_axp,	move_w_ayd_axp,	move_w_ayid_axp,move_w_other_axp,
	// 3F00 - 3FFF
	move_w_ry_axm,	move_w_ry_axm,	move_w_ay0_axm,	move_w_ayp_axm,	move_w_aym_axm,	move_w_ayd_axm,	move_w_ayid_axm,move_w_other_axm,
	move_w_ry_axd,	move_w_ry_axd,	move_w_ay0_axd,	move_w_ayp_axd,	move_w_aym_axd,	move_w_ayd_axd,	move_w_ayid_axd,move_w_other_axd,
	move_w_ry_axid,	move_w_ry_axid,	move_w_ay0_axid,move_w_ayp_axid,move_w_aym_axid,move_w_ayd_axid,move_w_ayid_axid,move_w_other_axid,
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	

	//***************************************************************************************************
	//
	// 4000 - 40FF
	negx_b_dy,		illegal,		negx_b_ay0,		negx_b_ayp,		negx_b_aym,		negx_b_ayd,		negx_b_ayid,	negx_b_other,
	negx_w_dy,		illegal,		negx_w_ay0,		negx_w_ayp,		negx_w_aym,		negx_w_ayd,		negx_w_ayid,	negx_w_other,
	negx_l_dy,		illegal,		negx_l_ay0,		negx_l_ayp,		negx_l_aym,		negx_l_ayd,		negx_l_ayid,	negx_l_other,
	move_w_sr_dy,	illegal,		move_w_sr_ay0,	move_w_sr_ayp,	move_w_sr_aym,	move_w_sr_ayd,	move_w_sr_ayid,	move_w_sr_other,
	// 4100 - 41FF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4200 - 42FF
	clr_b_dy,		illegal,		clr_b_ay0,		clr_b_ayp,		clr_b_aym,		clr_b_ayd,		clr_b_ayid,		clr_b_other,
	clr_w_dy,		illegal,		clr_w_ay0,		clr_w_ayp,		clr_w_aym,		clr_w_ayd,		clr_w_ayid,		clr_w_other,
	clr_l_dy,		illegal,		clr_l_ay0,		clr_l_ayp,		clr_l_aym,		clr_l_ayd,		clr_l_ayid,		clr_l_other,
	move_w_ccr_dy,	illegal,		move_w_ccr_ay0,	move_w_ccr_ayp,	move_w_ccr_aym,	move_w_ccr_ayd,	move_w_ccr_ayid,move_w_ccr_other,
	// 4300 - 43FF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4400 - 44FF
	neg_b_dy,		illegal,		neg_b_ay0,		neg_b_ayp,		neg_b_aym,		neg_b_ayd,		neg_b_ayid,		neg_b_other,
	neg_w_dy,		illegal,		neg_w_ay0,		neg_w_ayp,		neg_w_aym,		neg_w_ayd,		neg_w_ayid,		neg_w_other,
	neg_l_dy,		illegal,		neg_l_ay0,		neg_l_ayp,		neg_l_aym,		neg_l_ayd,		neg_l_ayid,		neg_l_other,
	move_w_dy_ccr,	illegal,		move_w_ay0_ccr,	move_w_ayp_ccr,	move_w_aym_ccr,	move_w_ayd_ccr,	move_w_ayid_ccr,move_w_other_ccr,
	// 4500 - 45FF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4600 - 46FF
	not_b_dy,		illegal,		not_b_ay0,		not_b_ayp,		not_b_aym,		not_b_ayd,		not_b_ayid,		not_b_other,
	not_w_dy,		illegal,		not_w_ay0,		not_w_ayp,		not_w_aym,		not_w_ayd,		not_w_ayid,		not_w_other,
	not_l_dy,		illegal,		not_l_ay0,		not_l_ayp,		not_l_aym,		not_l_ayd,		not_l_ayid,		not_l_other,
	move_w_dy_sr,	illegal,		move_w_ay0_sr,	move_w_ayp_sr,	move_w_aym_sr,	move_w_ayd_sr,	move_w_ayid_sr,	move_w_other_sr,
	// 4700 - 47FF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4800 - 48FF
	nbcd_b_dy,		illegal,		nbcd_b_ay0,		nbcd_b_ayp,		nbcd_b_aym,		nbcd_b_ayd,		nbcd_b_ayid,	nbcd_b_other,
	swap_dy,		bkpt,			pea_ay0,		illegal,		illegal,		pea_ayd,		pea_ayid,		pea_other,
	ext_w_dy,		illegal,		movem_w_reg_ay0,illegal,		movem_w_reg_aym,movem_w_reg_ayd,movem_w_reg_ayid,movem_w_reg_other,
	ext_l_dy,		illegal,		movem_l_reg_ay0,illegal,		movem_l_reg_aym,movem_l_reg_ayd,movem_l_reg_ayid,movem_l_reg_other,
	// 4900 - 49FF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	extb_l_dy,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4A00 - 4AFF
	tst_b_dy,		illegal,		tst_b_ay0,		tst_b_ayp,		tst_b_aym,		tst_b_ayd,		tst_b_ayid,		tst_b_other,
	tst_w_dy,		tst_w_ay,		tst_w_ay0,		tst_w_ayp,		tst_w_aym,		tst_w_ayd,		tst_w_ayid,		tst_w_other,
	tst_l_dy,		tst_l_ay,		tst_l_ay0,		tst_l_ayp,		tst_l_aym,		tst_l_ayd,		tst_l_ayid,		tst_l_other,
	tas_b_dy,		illegal,		tas_b_ay0,		tas_b_ayp,		tas_b_aym,		tas_b_ayd,		tas_b_ayid,		tas_b_other,
	// 4B00 - 4BFF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4C00 - 4CFF
	mul_l_dy_dx,	illegal,		mul_l_ay0_dx,	mul_l_ayp_dx,	mul_l_aym_dx,	mul_l_ayd_dx,	mul_l_ayid_dx,	mul_l_other_dx,
	div_l_dy_dx,	illegal,		div_l_ay0_dx,	div_l_ayp_dx,	div_l_aym_dx,	div_l_ayd_dx,	div_l_ayid_dx,	div_l_other_dx,
	illegal,		illegal,		movem_w_ay0_reg,movem_w_ayp_reg,illegal,		movem_w_ayd_reg,movem_w_ayid_reg,movem_w_other_reg,
	illegal,		illegal,		movem_l_ay0_reg,movem_l_ayp_reg,illegal,		movem_l_ayd_reg,movem_l_ayid_reg,movem_l_other_reg,
	// 4D00 - 4DFF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,
	// 4E00 - 4EFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	trap_imm,		trap_imm,		link_imm_ay,	unlk_ay,		move_l_ay_usp,	move_l_usp_ay,	hodgepodge,		movec,		
	illegal,		illegal,		jsr_ay0,		illegal,		illegal,		jsr_ayd,		jsr_ayid,		jsr_other,
	illegal,		illegal,		jmp_ay0,		illegal,		illegal,		jmp_ayd,		jmp_ayid,		jmp_other,
	// 4F00 - 4FFF
	chk_l_dy_dx,	illegal,		chk_l_ay0_dx,	chk_l_ayp_dx,	chk_l_aym_dx,	chk_l_ayd_dx,	chk_l_ayid_dx,	chk_l_other_dx,	
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,	
	chk_w_dy_dx,	illegal,		chk_w_ay0_dx,	chk_w_ayp_dx,	chk_w_aym_dx,	chk_w_ayd_dx,	chk_w_ayid_dx,	chk_w_other_dx,
	illegal,		illegal,		lea_ay0_ax,		illegal,		illegal,		lea_ayd_ax,		lea_ayid_ax,	lea_other_ax,

	//***************************************************************************************************
	//
	// 5000 - 50FF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_t_dy,		dbcc_t_dy,		scc_t_ay0,		scc_t_ayp,		scc_t_aym,		scc_t_ayd,		scc_t_ayid,		scc_t_other,
	// 5100 - 51FF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_f_dy,		dbcc_f_dy,		scc_f_ay0,		scc_f_ayp,		scc_f_aym,		scc_f_ayd,		scc_f_ayid,		scc_f_other,
	// 5200 - 52FF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_hi_dy,		dbcc_hi_dy,		scc_hi_ay0,		scc_hi_ayp,		scc_hi_aym,		scc_hi_ayd,		scc_hi_ayid,	scc_hi_other,
	// 5300 - 53FF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_ls_dy,		dbcc_ls_dy,		scc_ls_ay0,		scc_ls_ayp,		scc_ls_aym,		scc_ls_ayd,		scc_ls_ayid,	scc_ls_other,
	// 5400 - 54FF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_cc_dy,		dbcc_cc_dy,		scc_cc_ay0,		scc_cc_ayp,		scc_cc_aym,		scc_cc_ayd,		scc_cc_ayid,	scc_cc_other,
	// 5500 - 55FF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_cs_dy,		dbcc_cs_dy,		scc_cs_ay0,		scc_cs_ayp,		scc_cs_aym,		scc_cs_ayd,		scc_cs_ayid,	scc_cs_other,
	// 5600 - 56FF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_ne_dy,		dbcc_ne_dy,		scc_ne_ay0,		scc_ne_ayp,		scc_ne_aym,		scc_ne_ayd,		scc_ne_ayid,	scc_ne_other,
	// 5700 - 57FF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_eq_dy,		dbcc_eq_dy,		scc_eq_ay0,		scc_eq_ayp,		scc_eq_aym,		scc_eq_ayd,		scc_eq_ayid,	scc_eq_other,
	// 5800 - 58FF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_vc_dy,		dbcc_vc_dy,		scc_vc_ay0,		scc_vc_ayp,		scc_vc_aym,		scc_vc_ayd,		scc_vc_ayid,	scc_vc_other,
	// 5900 - 59FF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_vs_dy,		dbcc_vs_dy,		scc_vs_ay0,		scc_vs_ayp,		scc_vs_aym,		scc_vs_ayd,		scc_vs_ayid,	scc_vs_other,
	// 5A00 - 5AFF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_pl_dy,		dbcc_pl_dy,		scc_pl_ay0,		scc_pl_ayp,		scc_pl_aym,		scc_pl_ayd,		scc_pl_ayid,	scc_pl_other,
	// 5B00 - 5BFF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_mi_dy,		dbcc_mi_dy,		scc_mi_ay0,		scc_mi_ayp,		scc_mi_aym,		scc_mi_ayd,		scc_mi_ayid,	scc_mi_other,
	// 5C00 - 5CFF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_ge_dy,		dbcc_ge_dy,		scc_ge_ay0,		scc_ge_ayp,		scc_ge_aym,		scc_ge_ayd,		scc_ge_ayid,	scc_ge_other,
	// 5D00 - 5DFF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_lt_dy,		dbcc_lt_dy,		scc_lt_ay0,		scc_lt_ayp,		scc_lt_aym,		scc_lt_ayd,		scc_lt_ayid,	scc_lt_other,
	// 5E00 - 5EFF
	addq_b_ix_dy,	illegal,		addq_b_ix_ay0,	addq_b_ix_ayp,	addq_b_ix_aym,	addq_b_ix_ayd,	addq_b_ix_ayid,	addq_b_ix_other,
	addq_w_ix_dy,	addq_w_ix_ay,	addq_w_ix_ay0,	addq_w_ix_ayp,	addq_w_ix_aym,	addq_w_ix_ayd,	addq_w_ix_ayid,	addq_w_ix_other,
	addq_l_ix_dy,	addq_l_ix_ay,	addq_l_ix_ay0,	addq_l_ix_ayp,	addq_l_ix_aym,	addq_l_ix_ayd,	addq_l_ix_ayid,	addq_l_ix_other,
	scc_gt_dy,		dbcc_gt_dy,		scc_gt_ay0,		scc_gt_ayp,		scc_gt_aym,		scc_gt_ayd,		scc_gt_ayid,	scc_gt_other,
	// 5F00 - 5FFF
	subq_b_ix_dy,	illegal,		subq_b_ix_ay0,	subq_b_ix_ayp,	subq_b_ix_aym,	subq_b_ix_ayd,	subq_b_ix_ayid,	subq_b_ix_other,
	subq_w_ix_dy,	subq_w_ix_ay,	subq_w_ix_ay0,	subq_w_ix_ayp,	subq_w_ix_aym,	subq_w_ix_ayd,	subq_w_ix_ayid,	subq_w_ix_other,
	subq_l_ix_dy,	subq_l_ix_ay,	subq_l_ix_ay0,	subq_l_ix_ayp,	subq_l_ix_aym,	subq_l_ix_ayd,	subq_l_ix_ayid,	subq_l_ix_other,
	scc_le_dy,		dbcc_le_dy,		scc_le_ay0,		scc_le_ayp,		scc_le_aym,		scc_le_ayd,		scc_le_ayid,	scc_le_other,

	//***************************************************************************************************
	//
	// 6000 - 60FF
	bra,			bra,			bra,			bra,			bra,			bra,			bra,			bra,				
	bra,			bra,			bra,			bra,			bra,			bra,			bra,			bra,				
	bra,			bra,			bra,			bra,			bra,			bra,			bra,			bra,				
	bra,			bra,			bra,			bra,			bra,			bra,			bra,			bra,				
	// 6100 - 61FF
	bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,				
	bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,				
	bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,				
	bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,			bsr,				
	// 6200 - 62FF
	bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			
	bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			
	bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			
	bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			bcc_hi,			
	// 6300 - 63FF
	bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			
	bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			
	bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			
	bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			bcc_ls,			
	// 6400 - 64FF
	bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			
	bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			
	bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			
	bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			bcc_cc,			
	// 6500 - 65FF
	bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,				
	bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,				
	bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,				
	bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,			bcc_cs,				
	// 6600 - 66FF
	bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			
	bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			
	bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			
	bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			bcc_ne,			
	// 6700 - 67FF
	bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,				
	bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,				
	bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,				
	bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,			bcc_eq,				
	// 6800 - 68FF
	bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			
	bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			
	bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			
	bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			bcc_vc,			
	// 6900 - 69FF
	bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			
	bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			
	bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			
	bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			bcc_vs,			
	// 6A00 - 6AFF
	bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			
	bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			
	bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			
	bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			bcc_pl,			
	// 6B00 - 6BFF
	bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			
	bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			
	bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			
	bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			bcc_mi,			
	// 6C00 - 6CFF
	bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			
	bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			
	bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			
	bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			bcc_ge,			
	// 6D00 - 6DFF
	bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			
	bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			
	bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			
	bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			bcc_lt,			
	// 6E00 - 6EFF
	bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			
	bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			
	bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			
	bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			bcc_gt,			
	// 6F00 - 6FFF
	bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			
	bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			
	bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			
	bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			bcc_le,			

	//***************************************************************************************************
	//
	// 7000 - 70FF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7100 - 71FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7200 - 72FF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7300 - 73FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7400 - 74FF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7500 - 75FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7600 - 76FF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7700 - 77FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7800 - 78FF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7900 - 79FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7A00 - 7AFF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7B00 - 7BFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7C00 - 7CFF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7D00 - 7DFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// 7E00 - 7EFF
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	moveq_imm_dx,	
	// 7F00 - 7FFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			

	//***************************************************************************************************
	//
	// 8000 - 80FF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8100 - 81FF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal,		or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal,		or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8200 - 82FF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8300 - 83FF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal,	 	or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal,	 	or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8400 - 84FF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8500 - 85FF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal, 		or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal, 		or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8600 - 86FF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8700 - 87FF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal,	 	or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal,	 	or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8800 - 88FF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8900 - 89FF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal, 		or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal, 		or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8A00 - 8AFF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8B00 - 8BFF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal,		or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal,		or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8C00 - 8CFF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8D00 - 8DFF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal, 		or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal, 		or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,
	// 8E00 - 8EFF
	or_b_dy_dx,		illegal,		or_b_ay0_dx,	or_b_ayp_dx,	or_b_aym_dx,	or_b_ayd_dx,	or_b_ayid_dx,	or_b_other_dx,
	or_w_dy_dx,		illegal,		or_w_ay0_dx,	or_w_ayp_dx,	or_w_aym_dx,	or_w_ayd_dx,	or_w_ayid_dx,	or_w_other_dx,
	or_l_dy_dx,		illegal,		or_l_ay0_dx,	or_l_ayp_dx,	or_l_aym_dx,	or_l_ayd_dx,	or_l_ayid_dx,	or_l_other_dx,
	divu_w_dy_dx,	illegal,		divu_w_ay0_dx,	divu_w_ayp_dx,	divu_w_aym_dx,	divu_w_ayd_dx,	divu_w_ayid_dx,	divu_w_other_dx,
	// 8F00 - 8FFF
	sbcd_b_dy_dx,	sbcd_b_aym_axm,	or_b_dx_ay0,	or_b_dx_ayp,	or_b_dx_aym,	or_b_dx_ayd,	or_b_dx_ayid,	or_b_dx_other,
	illegal,	 	illegal,		or_w_dx_ay0,	or_w_dx_ayp,	or_w_dx_aym,	or_w_dx_ayd,	or_w_dx_ayid,	or_w_dx_other,
	illegal,	 	illegal,		or_l_dx_ay0,	or_l_dx_ayp,	or_l_dx_aym,	or_l_dx_ayd,	or_l_dx_ayid,	or_l_dx_other,
	divs_w_dy_dx,	illegal,		divs_w_ay0_dx,	divs_w_ayp_dx,	divs_w_aym_dx,	divs_w_ayd_dx,	divs_w_ayid_dx,	divs_w_other_dx,

	//***************************************************************************************************
	//
	// 9000 - 90FF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9100 - 91FF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9200 - 92FF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9300 - 93FF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9400 - 94FF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9500 - 95FF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9600 - 96FF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9700 - 97FF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9800 - 98FF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9900 - 99FF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9A00 - 9AFF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9B00 - 9BFF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9C00 - 9CFF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9D00 - 9DFF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,
	// 9E00 - 9EFF
	sub_b_dy_dx,	illegal,		sub_b_ay0_dx,	sub_b_ayp_dx,	sub_b_aym_dx,	sub_b_ayd_dx,	sub_b_ayid_dx,	sub_b_other_dx,
	sub_w_ry_dx,	sub_w_ry_dx,	sub_w_ay0_dx,	sub_w_ayp_dx,	sub_w_aym_dx,	sub_w_ayd_dx,	sub_w_ayid_dx,	sub_w_other_dx,
	sub_l_ry_dx,	sub_l_ry_dx,	sub_l_ay0_dx,	sub_l_ayp_dx,	sub_l_aym_dx,	sub_l_ayd_dx,	sub_l_ayid_dx,	sub_l_other_dx,
	suba_w_dy_ax,	suba_w_ay_ax,	suba_w_ay0_ax,	suba_w_ayp_ax,	suba_w_aym_ax,	suba_w_ayd_ax,	suba_w_ayid_ax,	suba_w_other_ax,
	// 9F00 - 9FFF
	subx_b_dy_dx,	subx_b_aym_axm,	sub_b_dx_ay0,	sub_b_dx_ayp,	sub_b_dx_aym,	sub_b_dx_ayd,	sub_b_dx_ayid,	sub_b_dx_other,
	subx_w_dy_dx,	subx_w_aym_axm,	sub_w_dx_ay0,	sub_w_dx_ayp,	sub_w_dx_aym,	sub_w_dx_ayd,	sub_w_dx_ayid,	sub_w_dx_other,
	subx_l_dy_dx,	subx_l_aym_axm,	sub_l_dx_ay0,	sub_l_dx_ayp,	sub_l_dx_aym,	sub_l_dx_ayd,	sub_l_dx_ayid,	sub_l_dx_other,
	suba_l_dy_ax,	suba_l_ay_ax,	suba_l_ay0_ax,	suba_l_ayp_ax,	suba_l_aym_ax,	suba_l_ayd_ax,	suba_l_ayid_ax,	suba_l_other_ax,

	//***************************************************************************************************
	//
	// A000 - A0FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A100 - A1FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A200 - A2FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A300 - A3FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A400 - A4FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A500 - A5FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A600 - A6FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A700 - A7FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A800 - A8FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// A900 - A9FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// AA00 - AAFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// AB00 - ABFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// AC00 - ACFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// AD00 - ADFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// AE00 - AEFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// AF00 - AFFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			

	//***************************************************************************************************
	//
	// B000 - B0FF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// B100 - B1FF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// B200 - B2FF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// B300 - B3FF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// B400 - B4FF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// B500 - B5FF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// B600 - B6FF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// B700 - B7FF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// B800 - B8FF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// B900 - B9FF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// BA00 - BAFF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// BB00 - BBFF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// BC00 - BCFF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// BD00 - BDFF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,
	// BE00 - BEFF
	cmp_b_dy_dx,	illegal,		cmp_b_ay0_dx,	cmp_b_ayp_dx,	cmp_b_aym_dx,	cmp_b_ayd_dx,	cmp_b_ayid_dx,	cmp_b_other_dx,
	cmp_w_ry_dx,	cmp_w_ry_dx,	cmp_w_ay0_dx,	cmp_w_ayp_dx,	cmp_w_aym_dx,	cmp_w_ayd_dx,	cmp_w_ayid_dx,	cmp_w_other_dx,
	cmp_l_ry_dx,	cmp_l_ry_dx,	cmp_l_ay0_dx,	cmp_l_ayp_dx,	cmp_l_aym_dx,	cmp_l_ayd_dx,	cmp_l_ayid_dx,	cmp_l_other_dx,
	cmpa_w_dy_ax,	cmpa_w_ay_ax,	cmpa_w_ay0_ax,	cmpa_w_ayp_ax,	cmpa_w_aym_ax,	cmpa_w_ayd_ax,	cmpa_w_ayid_ax,	cmpa_w_other_ax,
	// BF00 - BFFF
	eor_b_dx_dy,	cmpm_b_ayp_axp,	eor_b_dx_ay0,	eor_b_dx_ayp,	eor_b_dx_aym,	eor_b_dx_ayd,	eor_b_dx_ayid,	eor_b_dx_other,
	eor_w_dx_dy,	cmpm_w_ayp_axp,	eor_w_dx_ay0,	eor_w_dx_ayp,	eor_w_dx_aym,	eor_w_dx_ayd,	eor_w_dx_ayid,	eor_w_dx_other,
	eor_l_dx_dy,	cmpm_l_ayp_axp,	eor_l_dx_ay0,	eor_l_dx_ayp,	eor_l_dx_aym,	eor_l_dx_ayd,	eor_l_dx_ayid,	eor_l_dx_other,
	cmpa_l_dy_ax,	cmpa_l_ay_ax,	cmpa_l_ay0_ax,	cmpa_l_ayp_ax,	cmpa_l_aym_ax,	cmpa_l_ayd_ax,	cmpa_l_ayid_ax,	cmpa_l_other_ax,

	//***************************************************************************************************
	//
	// C000 - C0FF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// C100 - C1FF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,		and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,		and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// C200 - C2FF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// C300 - C3FF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,		and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,		and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// C400 - C4FF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// C500 - C5FF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay, 		and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay, 		and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// C600 - C6FF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// C700 - C7FF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,		and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,		and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// C800 - C8FF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// C900 - C9FF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,	 	and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,	 	and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// CA00 - CAFF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// CB00 - CBFF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,		and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,	 	and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// CC00 - CCFF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// CD00 - CDFF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,	 	and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,	 	and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,
	// CE00 - CEFF
	and_b_dy_dx,	illegal,		and_b_ay0_dx,	and_b_ayp_dx,	and_b_aym_dx,	and_b_ayd_dx,	and_b_ayid_dx,	and_b_other_dx,
	and_w_dy_dx,	illegal,		and_w_ay0_dx,	and_w_ayp_dx,	and_w_aym_dx,	and_w_ayd_dx,	and_w_ayid_dx,	and_w_other_dx,
	and_l_dy_dx,	illegal,		and_l_ay0_dx,	and_l_ayp_dx,	and_l_aym_dx,	and_l_ayd_dx,	and_l_ayid_dx,	and_l_other_dx,
	mulu_w_dy_dx,	illegal,		mulu_w_ay0_dx,	mulu_w_ayp_dx,	mulu_w_aym_dx,	mulu_w_ayd_dx,	mulu_w_ayid_dx,	mulu_w_other_dx,
	// CF00 - CFFF
	abcd_b_dy_dx,	abcd_b_aym_axm,	and_b_dx_ay0,	and_b_dx_ayp,	and_b_dx_aym,	and_b_dx_ayd,	and_b_dx_ayid,	and_b_dx_other,
	exg_dx_dy,	 	exg_ax_ay,	 	and_w_dx_ay0,	and_w_dx_ayp,	and_w_dx_aym,	and_w_dx_ayd,	and_w_dx_ayid,	and_w_dx_other,
	illegal,		exg_dx_ay,	 	and_l_dx_ay0,	and_l_dx_ayp,	and_l_dx_aym,	and_l_dx_ayd,	and_l_dx_ayid,	and_l_dx_other,
	muls_w_dy_dx,	illegal,		muls_w_ay0_dx,	muls_w_ayp_dx,	muls_w_aym_dx,	muls_w_ayd_dx,	muls_w_ayid_dx,	muls_w_other_dx,

	//***************************************************************************************************
	//
	// D000 - D0FF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// D100 - D1FF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// D200 - D2FF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// D300 - D3FF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// D400 - D4FF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// D500 - D5FF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// D600 - D6FF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// D700 - D7FF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// D800 - D8FF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// D900 - D9FF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// DA00 - DAFF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// DB00 - DBFF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// DC00 - DCFF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// DD00 - DDFF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,
	// DE00 - DEFF
	add_b_dy_dx,	illegal,		add_b_ay0_dx,	add_b_ayp_dx,	add_b_aym_dx,	add_b_ayd_dx,	add_b_ayid_dx,	add_b_other_dx,
	add_w_ry_dx,	add_w_ry_dx,	add_w_ay0_dx,	add_w_ayp_dx,	add_w_aym_dx,	add_w_ayd_dx,	add_w_ayid_dx,	add_w_other_dx,
	add_l_ry_dx,	add_l_ry_dx,	add_l_ay0_dx,	add_l_ayp_dx,	add_l_aym_dx,	add_l_ayd_dx,	add_l_ayid_dx,	add_l_other_dx,
	adda_w_dy_ax,	adda_w_ay_ax,	adda_w_ay0_ax,	adda_w_ayp_ax,	adda_w_aym_ax,	adda_w_ayd_ax,	adda_w_ayid_ax,	adda_w_other_ax,
	// DF00 - DFFF
	addx_b_dy_dx,	addx_b_aym_axm,	add_b_dx_ay0,	add_b_dx_ayp,	add_b_dx_aym,	add_b_dx_ayd,	add_b_dx_ayid,	add_b_dx_other,
	addx_w_dy_dx,	addx_w_aym_axm,	add_w_dx_ay0,	add_w_dx_ayp,	add_w_dx_aym,	add_w_dx_ayd,	add_w_dx_ayid,	add_w_dx_other,
	addx_l_dy_dx,	addx_l_aym_axm,	add_l_dx_ay0,	add_l_dx_ayp,	add_l_dx_aym,	add_l_dx_ayd,	add_l_dx_ayid,	add_l_dx_other,
	adda_l_dy_ax,	adda_l_ay_ax,	adda_l_ay0_ax,	adda_l_ayp_ax,	adda_l_aym_ax,	adda_l_ayd_ax,	adda_l_ayid_ax,	adda_l_other_ax,

	//***************************************************************************************************
	//
	// E000 - E0FF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	illegal,		illegal,		asr_w_ay0,		asr_w_ayp,		asr_w_aym,		asr_w_ayd,		asr_w_ayid,		asr_w_other,
	// E100 - E1FF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	illegal,		illegal,		asl_w_ay0,		asl_w_ayp,		asl_w_aym,		asl_w_ayd,		asl_w_ayid,		asl_w_other,
	// E200 - E2FF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	illegal,		illegal,		lsr_w_ay0,		lsr_w_ayp,		lsr_w_aym,		lsr_w_ayd,		lsr_w_ayid,		lsr_w_other,
	// E300 - E3FF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	illegal,		illegal,		lsl_w_ay0,		lsl_w_ayp,		lsl_w_aym,		lsl_w_ayd,		lsl_w_ayid,		lsl_w_other,
	// E400 - E4FF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	illegal,		illegal,		roxr_w_ay0,		roxr_w_ayp,		roxr_w_aym,		roxr_w_ayd,		roxr_w_ayid,	roxr_w_other,
	// E500 - E5FF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	illegal,		illegal,		roxl_w_ay0,		roxl_w_ayp,		roxl_w_aym,		roxl_w_ayd,		roxl_w_ayid,	roxl_w_other,
	// E600 - E6FF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	illegal,		illegal,		ror_w_ay0,		ror_w_ayp,		ror_w_aym,		ror_w_ayd,		ror_w_ayid,		ror_w_other,
	// E700 - E7FF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	illegal,		illegal,		rol_w_ay0,		rol_w_ayp,		rol_w_aym,		rol_w_ayd,		rol_w_ayid,		rol_w_other,
	// E800 - E8FF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	bftst_dy,		illegal,		bftst_ay0,		illegal,		illegal,		bftst_ayd,		bftst_ayid,		bftst_other,
	// E900 - E9FF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	bfextu_dy_dx,	illegal,		bfextu_ay0_dx,	illegal,		illegal,		bfextu_ayd_dx,	bfextu_ayid_dx,	bfextu_other_dx,
	// EA00 - EAFF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	bfchg_dy,		illegal,		bfchg_ay0,		illegal,		illegal,		bfchg_ayd,		bfchg_ayid,		bfchg_other,
	// EB00 - EBFF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	bfexts_dy_dx,	illegal,		bfexts_ay0_dx,	illegal,		illegal,		bfexts_ayd_dx,	bfexts_ayid_dx,	bfexts_other_dx,
	// EC00 - ECFF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	bfclr_dy,		illegal,		bfclr_ay0,		illegal,		illegal,		bfclr_ayd,		bfclr_ayid,		bfclr_other,
	// ED00 - EDFF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	bfffo_dy_dx,	illegal,		bfffo_ay0_dx,	illegal,		illegal,		bfffo_ayd_dx,	bfffo_ayid_dx,	bfffo_other_dx,
	// EE00 - EEFF
	asr_b_ix_dy,	lsr_b_ix_dy,	roxr_b_ix_dy,	ror_b_ix_dy,	asr_b_dx_dy,	lsr_b_dx_dy,	roxr_b_dx_dy,	ror_b_dx_dy,
	asr_w_ix_dy,	lsr_w_ix_dy,	roxr_w_ix_dy,	ror_w_ix_dy,	asr_w_dx_dy,	lsr_w_dx_dy,	roxr_w_dx_dy,	ror_w_dx_dy,
	asr_l_ix_dy,	lsr_l_ix_dy,	roxr_l_ix_dy,	ror_l_ix_dy,	asr_l_dx_dy,	lsr_l_dx_dy,	roxr_l_dx_dy,	ror_l_dx_dy,
	bfset_dy,		illegal,		bfset_ay0,		illegal,		illegal,		bfset_ayd,		bfset_ayid,		bfset_other,
	// EF00 - EFFF
	asl_b_ix_dy,	lsl_b_ix_dy,	roxl_b_ix_dy,	rol_b_ix_dy,	asl_b_dx_dy,	lsl_b_dx_dy,	roxl_b_dx_dy,	rol_b_dx_dy,
	asl_w_ix_dy,	lsl_w_ix_dy,	roxl_w_ix_dy,	rol_w_ix_dy,	asl_w_dx_dy,	lsl_w_dx_dy,	roxl_w_dx_dy,	rol_w_dx_dy,
	asl_l_ix_dy,	lsl_l_ix_dy,	roxl_l_ix_dy,	rol_l_ix_dy,	asl_l_dx_dy,	lsl_l_dx_dy,	roxl_l_dx_dy,	rol_l_dx_dy,
	bfins_dx_dy,	illegal,		bfins_dx_ay0,	illegal,		illegal,		bfins_dx_ayd,	bfins_dx_ayid,	bfins_dx_other,
	
	//***************************************************************************************************
	//
	// F000 - F0FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F100 - F1FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F200 - F2FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F300 - F3FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F400 - F4FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F500 - F5FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F600 - F6FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F700 - F7FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F800 - F8FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// F900 - F9FF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// FA00 - FAFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// FB00 - FBFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// FC00 - FCFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// FD00 - FDFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// FE00 - FEFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	// FF00 - FFFF
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,			
	illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal,		illegal
};
