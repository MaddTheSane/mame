/****************************************************************************
Supported games:
	Twin Cobra (World)
	Twin Cobra (USA license)
	Kyukyoku Tiger (Japan license)

	Flying Shark (World)
	Sky Shark (USA license)
	Hishou Zame (Flying Shark Japan license)
	Flying Shark bootleg (USA license)

Difference between Twin Cobra and Kyukyoko Tiger:
	T.C. supports two simultaneous players.
	K.T. supports two players, but only one at a time.
		 for this reason, it also supports Table Top cabinets.
	T.C. stores 3 characters for high scores.
	K.T. stores 6 characters for high scores.
	T.C. heros are Red and Blue for player 1 and 2 respectively.
	K.T. heros are grey for both players.
	T.C. dead remains of ground tanks are circular.
	K.T. dead remains of ground tanks always vary in shape.
	T.C. does not use DSW1-1 and DSW2-8.
	K.T. uses DSW1-1 for cabinet type, and DSW2-8 for allow game continue.
	T.C. continues new hero and continued game at current position.
	K.T. continues new hero and continued game at predefined positions.
		 After dying, and your new hero appears, if you do not travel more
		 than your helicopter length forward, you are penalised and moved
		 back further when your next hero appears.
	K.T. Due to this difference in continue sequence, Kyukyoko Tiger is MUCH
		 harder, challenging, and nearly impossible to complete !

68000: Main CPU

00000-1FFFF ROM for Flying Shark
00000-2FFFF ROM for Twin Cobra
30000-33fff RAM shared with TMS320C10NL-14 protection microcontroller
40000-40fff RAM sprite display properties (co-ordinates, character, color - etc)
50000-50dff Palette RAM
7a000-7abff RAM shared with Z80; 16-bit on this side, 8-bit on Z80 side

read:
78001		DSW1 (Flying Shark)
78003		DSW2 (Flying Shark)

78005		Player 1 Joystick and Buttons input port
78007		Player 2 Joystick and Buttons input port
78009		bit 7 vblank, coin and control/service inputs (Flying shark)
				Flying Shark implements Tilt as 'freeze system' and also has
				a reset button, but its not implelemted here (not needed)

7e000-7e005 read data from video RAM (see below)

write:
60000-60003 CRT 6845 controller. 0 = register offset , 2 = register data
70000-70001 scroll   y   for character page (centre normally 0x01c9)
70002-70003 scroll < x > for character page (centre normally 0x00e2)
70004-70005 offset in character page to write character (7e000)

72000-72001 scroll   y   for foreground page (starts from     0x03c9)
72002-72003 scroll < x > for foreground page (centre normally 0x002a)
72004-72005 offset in character page to write character (7e002)

74000-74001 scroll   y   for background page (starts from     0x03c9)
74002-74003 scroll < x > for background page (centre normally 0x002a)
74004-74005 offset in character page to write character (7e004)

76000-76003 as above but for another layer maybe ??? (Not used here)
7800a		This activates INT line for Flying shark. (Not via 7800C)
			00		Activate INTerrupt line to the TMS320C10 DSP.
			01		Inhibit  INTerrupt line to the TMS320C10 DSP.

7800c		Control register (Byte write access).
			bits 7-4 always 0
			bits 3-1 select the control signal to drive.
			bit   0  is the value passed to the control signal.

			Value (hex):
			00-03	????
			04		Clear IPL2 line to 68000 inactive hi (Interrupt priority 4)
			05		Set   IPL2 line to 68000 active  low (Interrupt priority 4)
			06		Dont flip display
			07		Flip display
			08		Switch to background layer ram bank 0
			09		Switch to background layer ram bank 1
			0A		Switch to foreground layer rom bank 0
			0B		Switch to foreground layer rom bank 1
			0C		Activate INTerrupt line to the TMS320C10 DSP  (Twin Cobra)
			0D		Inhibit  INTerrupt line to the TMS320C10 DSP  (Twin Cobra)
			0E		Turn screen off
			0F		Turn screen on

7e000-7e001 data to write in text video RAM (70000)
7e002-7e003 data to write in bg video RAM (72004)
7e004-7e005 data to write in fg video RAM (74004)

Z80: Sound CPU
0000-7fff ROM
8000-87ff shared with 68000; 8-bit on this side, 16-bit on 68000 side

in:
00		  YM3812 status
10		  Coin inputs and control/service inputs (Twin Cobra)
40		  DSW1 (Twin Cobra)
50		  DSW2 (Twin Cobra)

out:
00		  YM3812 control
01		  YM3812 data
20		  Coin counters / Coin lockouts

TMS320C10 DSP: Harvard type architecture. RAM and ROM on seperate data buses.
0000-07ff ROM (words)
0000-0090 Internal RAM (words).	Moved to 8000-8120 for MAME compatibility.
								View this memory in the debugger at 4000h

in:
01		  data read from addressed 68K address space (Main RAM/Sprite RAM)

out:
00		  address of 68K to read/write to
01		  data to write to addressed 68K address space (Main RAM/Sprite RAM)
03		  bit 15 goes to BIO line of TMS320C10. BIO is a polled input line.


MCUs used with this hardware: (TMS320C10 in custom Toaplan/Taito disguise)

Twin Cobra					Sky Shark					Wardner
D70016U						D70012U  					D70012U
GXC-04						GXC-02						GXC-02
MCU (delta) 74000			MCU 71400					MCU (delta) 71900



68K writes the following to $30000 to tell DSP to do the following:
Twin  Kyukyoku
Cobra Tiger
00		00	 do nothing
01		0C	 run self test, and report DSP ROM checksum		from 68K PC:23CA6
02		07	 control all enemy shots						from 68K PC:23BFA
04		0b	 start the enemy helicopters					from 68K PC:23C66
05		08	 check for colision with enemy fire ???			from 68K PC:23C20
06		09	 check for colision with enemy ???				from 68K PC:23C44
07		01	 control enemy helicopter shots					from 68K PC:23AB2
08		02	 control all ground enemy shots
0A		04	 read hero position and send enemy to it ?		from 68K PC:23B58

03		0A	\
09		03	 \ These functions within the DSP never seem to be called ????
0B		05	 /
0C		06	/

68K writes the following to $30004 to tell DSP to do the following:
Flying	Hishou
Shark	Zame
00		00	 do nothing
03		0B	 Write sprite to sprite RAM
05		01	 Get angle
06		02	 Rotate towards direction
09		05	 Check collision between 2 spheres!??
0A		06	 Polar coordinates add
0B		07	 run self test, and report DSP ROM checksum

01		09	\
02		0A	 \
04		08	  > These functions within the DSP never seem to be called ????
07		03	 /
08		04	/
*****************************************************************************/



#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

/**************** Video stuff ******************/
int  twincobr_crtc_r(int offset);
void twincobr_crtc_w(int offset,int data);

void twincobr_txscroll_w(int offset,int data);
void twincobr_bgscroll_w(int offset,int data);
void twincobr_fgscroll_w(int offset,int data);
void twincobr_exscroll_w(int offset,int data);
int  twincobr_txoffs_r(void);
void twincobr_txoffs_w(int offset,int data);
void twincobr_bgoffs_w(int offset,int data);
void twincobr_fgoffs_w(int offset,int data);
int  twincobr_txram_r(int offset);
int  twincobr_bgram_r(int offset);
int  twincobr_fgram_r(int offset);
void twincobr_txram_w(int offset,int data);
void twincobr_bgram_w(int offset,int data);
void twincobr_fgram_w(int offset,int data);

int  twincobr_vh_start(void);
void twincobr_vh_stop(void);
void twincobr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern int twincobr_flip_screen;


/**************** Machine stuff ******************/
void fsharkbt_reset_8741_mcu(void);
int  fsharkbt_dsp_in(int offset);
void fshark_coin_dsp_w(int offset,int data);
int  twincobr_dsp_in(int offset);
void twincobr_dsp_out(int fnction,int data);
int  twincobr_68k_dsp_r(int offset);
void twincobr_68k_dsp_w(int offset,int data);
int  twincobr_7800c_r(int offset);
void twincobr_7800c_w(int offset,int data);
int  twincobr_sharedram_r(int offset);
void twincobr_sharedram_w(int offset,int data);

extern unsigned char *twincobr_68k_dsp_ram;
extern unsigned char *twincobr_sharedram;
extern int intenable;

/**************** High Score stuff ***************/
static int twinc_scores;
static int twinc_names;
static int twinc_levels;
static int twinc_end;
static int twinc_vid_test;


int twincobr_input_r(int offset)
{
	return readinputport(1 + (offset / 2));
}

int fshark_DSW_input_r(int offset)
{
	return readinputport(4 + (offset / 2));
}

int vblank_fshark_input_r(int offset)
{
	/* VBlank for both Twin Cobra and Flying Shark. */
	/* Also coin, start and system switches for Flying Shark */
	int read_two_ports = 0;
	read_two_ports = readinputport(0);
	read_two_ports |= readinputport(3);
	return read_two_ports;
}

int twincobr_interrupt(void)
{
	if (intenable) {
		intenable = 0;
		return MC68000_IRQ_4;
	}
	else return MC68000_INT_NONE;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x02ffff, MRA_ROM },
	{ 0x030000, 0x033fff, twincobr_68k_dsp_r, &twincobr_68k_dsp_ram },	/* 68K and DSP shared RAM */
	{ 0x040000, 0x040fff, MRA_BANK1 },				/* sprite ram data */
	{ 0x050000, 0x050dff, paletteram_word_r },
	{ 0x078000, 0x078003, fshark_DSW_input_r },		/* Flying Shark - DSW1 & 2 */
	{ 0x078004, 0x078007, twincobr_input_r },		/* Joystick inputs */
	{ 0x078008, 0x078009, vblank_fshark_input_r },  /* vblank & FShark coin/start */
	{ 0x07a000, 0x07abff, twincobr_sharedram_r },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x07e000, 0x07e001, twincobr_txram_r },		/* data from text video RAM */
	{ 0x07e002, 0x07e003, twincobr_bgram_r },		/* data from bg video RAM */
	{ 0x07e004, 0x07e005, twincobr_fgram_r },		/* data from fg video RAM */
	{ 0x080000, 0x0bffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0x030000, 0x033fff, twincobr_68k_dsp_w, &twincobr_68k_dsp_ram },	/* 68K and DSP shared RAM */
	{ 0x040000, 0x040fff, MWA_BANK1, &spriteram, &spriteram_size },		/* sprite ram data */
	{ 0x050000, 0x050dff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x060000, 0x060003, twincobr_crtc_w },		/* 6845 CRT controller */
	{ 0x070000, 0x070003, twincobr_txscroll_w },	/* text layer scroll */
	{ 0x070004, 0x070005, twincobr_txoffs_w },		/* offset in text video RAM */
	{ 0x072000, 0x072003, twincobr_bgscroll_w },	/* bg layer scroll */
	{ 0x072004, 0x072005, twincobr_bgoffs_w },		/* offset in bg video RAM */
	{ 0x074000, 0x074003, twincobr_fgscroll_w },	/* fg layer scroll */
	{ 0x074004, 0x074005, twincobr_fgoffs_w },		/* offset in fg video RAM */
	{ 0x076000, 0x076003, twincobr_exscroll_w },	/* Spare layer scroll */
	{ 0x07800a, 0x07800b, fshark_coin_dsp_w },		/* Flying Shark DSP Comms & coin stuff */
	{ 0x07800c, 0x07800d, twincobr_7800c_w },		/* Twin Cobra DSP Comms & system control */
	{ 0x07a000, 0x07afff, twincobr_sharedram_w },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x07e000, 0x07e001, twincobr_txram_w },		/* data for text video RAM */
	{ 0x07e002, 0x07e003, twincobr_bgram_w },		/* data for bg video RAM */
	{ 0x07e004, 0x07e005, twincobr_fgram_w },		/* data for fg video RAM */
	{ 0x080000, 0x0bffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &twincobr_sharedram },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ 0x10, 0x10, input_port_3_r },
	{ 0x40, 0x40, input_port_4_r },
	{ 0x50, 0x50, input_port_5_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ 0x20, 0x20, fshark_coin_dsp_w },		/* Twin Cobra coin count-lockout */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress DSP_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },	/* Really 0x0fff, Flying Shark bootleg requires this for ROM load */
	{ 0x8000, 0x811F, MRA_RAM },	/* The real DSP has this at address 0 */
									/* View this at 4000h in the debugger */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress DSP_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },	/* Really 0x0fff, Flying Shark bootleg requires this for ROM load */
	{ 0x8000, 0x811F, MWA_RAM },	/* The real DSP has this at address 0 */
									/* View this at 4000h in the debugger */
	{ -1 }	/* end of table */
};

static struct IOReadPort DSP_readport[] =
{
	{ 0x01, 0x01, twincobr_dsp_in },
	{ 0x02, 0x02, fsharkbt_dsp_in },
	{ -1 }	/* end of table */
};

static struct IOWritePort DSP_writeport[] =
{
	{ 0x00, 0x03, twincobr_dsp_out },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( twincobr_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50K, then every 150K" )
	PORT_DIPSETTING(	0x04, "70K, then every 200K" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( twincobu_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50K, then every 150K" )
	PORT_DIPSETTING(	0x04, "70K, then every 200K" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ktiger_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "70K, then every 200K" )
	PORT_DIPSETTING(	0x04, "50K, then every 150K" )
	PORT_DIPSETTING(	0x08, "100000" )
	PORT_DIPSETTING(	0x0c, "No Extend" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( fshark_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )		/* FS tilt causes freeze */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* FS reset button */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50K, then every 150K" )
	PORT_DIPSETTING(	0x04, "70K, then every 200K" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "1" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
INPUT_PORTS_END


INPUT_PORTS_START( skyshark_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )		/* FS tilt causes freeze */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* FS reset button */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_2C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50K, then every 150K" )
	PORT_DIPSETTING(	0x04, "70K, then every 200K" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "1" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( hishouza_input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )		/* FS tilt causes freeze */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* FS reset button */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )

	PORT_START		/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50K, then every 150K" )
	PORT_DIPSETTING(	0x04, "70K, then every 200K" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "1" )
	PORT_DIPSETTING(	0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0*2048*8*8, 1*2048*8*8, 2*2048*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every char takes 8 consecutive bytes */
};

static struct GfxLayout fgtilelayout =
{
	8,8,	/* 8*8 tiles */
	8192,	/* 8192 tiles */
	4,		/* 4 bits per pixel */
	{ 0*8192*8*8, 1*8192*8*8, 2*8192*8*8, 3*8192*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout bgtilelayout =
{
	8,8,	/* 8*8 tiles */
	4096,	/* 4096 tiles */
	4,		/* 4 bits per pixel */
	{ 0*4096*8*8, 1*4096*8*8, 2*4096*8*8, 3*4096*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every tile takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,		/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   1536, 32 },		/* colors 1536-1791 */
	{ 1, 0x0c000, &fgtilelayout, 1280, 16 },		/* colors 1280-1535 */
	{ 1, 0x4c000, &bgtilelayout, 1024, 16 },		/* colors 1024-1279 */
	{ 1, 0x6c000, &spritelayout,    0, 64 },		/* colors    0-1023 */
	{ -1 } /* end of array */
};



/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip  */
	3500000,		/* 3.5 MHz */
	{ 100 },		/* volume */
	{ irqhandler },
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			28000000/4,			/* 7.0 MHz - Main board Crystal is 28Mhz */
			0,
			readmem,writemem,0,0,
			twincobr_interrupt,1
		},
		{
			CPU_Z80,
			28000000/8,			/* 3.5 MHz */
			2,					/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		},
		{
			CPU_TMS320C10,
			28000000/8,			/* 3.5 MHz */
			3,					/* memory region #3 */
			DSP_readmem,DSP_writemem,DSP_readport,DSP_writeport,
			ignore_interrupt,0	/* IRQs are caused by 68000 */
		},
	},
	56, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	10,								/* 10 CPU slices per frame */
	fsharkbt_reset_8741_mcu,		/* Reset fshark bootleg 8741 MCU data */

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1792, 1792,
	0,	/* No color PROM decode */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	twincobr_vh_start,
	twincobr_vh_stop,
	twincobr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	},
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( twincobr )
	ROM_REGION(0x30000)		/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",		0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",		0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "tc15",		0x20000, 0x08000, 0x3a646618 )
	ROM_LOAD_ODD ( "tc13",		0x20000, 0x08000, 0xd7d1e317 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",			0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",			0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",			0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",			0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",			0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc06",			0x2c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc05",			0x3c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc07",			0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",			0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",			0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",			0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",			0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",			0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",			0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",			0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)		/* 32k for second CPU */
	ROM_LOAD( "tc12",			0x00000, 0x08000, 0xe37b3c44 )	/* slightly different from the other two sets */

	ROM_REGION(0x10000)		/* For TI TMS320C10 Microcontroller */
	ROM_LOAD_EVEN( "tc1b",		0x0000, 0x0800, 0x1757cc33 )
	ROM_LOAD_ODD ( "tc2a",		0x0000, 0x0800, 0xd6d878c9 )
ROM_END

ROM_START( twincobu )
	ROM_REGION(0x30000)		/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",			0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",			0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "tcbra26.bin",	0x20000, 0x08000, 0xbdd00ba4 )
	ROM_LOAD_ODD ( "tcbra27.bin",	0x20000, 0x08000, 0xed600907 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics */
	ROM_LOAD( "tc11",			0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",			0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",			0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",			0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",			0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc06",			0x2c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc05",			0x3c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc07",			0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",			0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",			0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",			0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",			0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",			0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",			0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",			0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)		/* 32k for second CPU */
	ROM_LOAD( "b30-05",				0x00000, 0x08000, 0x1a8f1e10 )

	ROM_REGION(0x10000)		/* For TI TMS320C10 Microcontroller */
	ROM_LOAD_EVEN( "dsp_22.bin",	0x0000, 0x0800, 0x79389a71 )
	ROM_LOAD_ODD ( "dsp_21.bin",	0x0000, 0x0800, 0x2d135376 )
ROM_END

ROM_START( ktiger )
	ROM_REGION(0x30000)		/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",		0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",		0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "b30-02",	0x20000, 0x08000, 0x1d63e9c4 )
	ROM_LOAD_ODD ( "b30-04",	0x20000, 0x08000, 0x03957a30 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics */
	ROM_LOAD( "tc11",			0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",			0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",			0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",			0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",			0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc06",			0x2c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc05",			0x3c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc07",			0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",			0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",			0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",			0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",			0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",			0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",			0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",			0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)		/* 32k for second CPU */
	ROM_LOAD( "b30-05",			0x00000, 0x08000, 0x1a8f1e10 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp-22",	0x0000, 0x0800, BADCRC( 0x8a1d48d9 ) )
	ROM_LOAD_ODD ( "dsp-21",	0x0000, 0x0800, BADCRC( 0x33d99bc2 ) )
ROM_END

ROM_START( fshark )
	ROM_REGION(0x20000)		/* 2*64k for code */
	ROM_LOAD_EVEN( "b02_18-1.rom",	0x00000, 0x10000, 0x04739e02 )
	ROM_LOAD_ODD ( "b02_17-1.rom",	0x00000, 0x10000, 0xfd6ef7a8 )

	ROM_REGION_DISPOSE(0xac000)		/* temporary space for graphics */
	ROM_LOAD( "b02_07-1.rom",	0x00000, 0x04000, 0xe669f80e )	/* chars */
	ROM_LOAD( "b02_06-1.rom",	0x04000, 0x04000, 0x5e53ae47 )
	ROM_LOAD( "b02_05-1.rom",	0x08000, 0x04000, 0xa8b05bd0 )
	ROM_LOAD( "b02_12.rom",		0x0c000, 0x08000, 0x733b9997 )	/* fg tiles */
		/* 14000-1bfff not used */
	ROM_LOAD( "b02_15.rom",		0x1c000, 0x08000, 0x8b70ef32 )
		/* 24000-2bfff not used */
	ROM_LOAD( "b02_14.rom",		0x2c000, 0x08000, 0xf711ba7d )
		/* 34000-3bfff not used */
	ROM_LOAD( "b02_13.rom",		0x3c000, 0x08000, 0x62532cd3 )
		/* 44000-4bfff not used */
	ROM_LOAD( "b02_08.rom",		0x4c000, 0x08000, 0xef0cf49c )	/* bg tiles */
	ROM_LOAD( "b02_11.rom",		0x54000, 0x08000, 0xf5799422 )
	ROM_LOAD( "b02_10.rom",		0x5c000, 0x08000, 0x4bd099ff )
	ROM_LOAD( "b02_09.rom",		0x64000, 0x08000, 0x230f1582 )
	ROM_LOAD( "b02_01.512",		0x6c000, 0x10000, 0x2234b424 )	/* sprites */
	ROM_LOAD( "b02_02.512",		0x7c000, 0x10000, 0x30d4c9a8 )
	ROM_LOAD( "b02_03.512",		0x8c000, 0x10000, 0x64f3d88f )
	ROM_LOAD( "b02_04.512",		0x9c000, 0x10000, 0x3b23a9fc )

	ROM_REGION(0x10000)		/* 64k for second CPU */
	ROM_LOAD( "b02_16.rom",		0x0000, 0x8000, 0xcdd1a153 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp.lsb",	0x0000, 0x0800, BADCRC( 0xacb21beb ) )
	ROM_LOAD_ODD ( "dsp.msb",	0x0000, 0x0800, BADCRC( 0x963ce23d ) )
ROM_END

ROM_START( skyshark )
	ROM_REGION(0x20000)		/* 2*64k for code */
	ROM_LOAD_EVEN( "18-2",		0x00000, 0x10000, 0x888e90f3 )
	ROM_LOAD_ODD ( "17-2",		0x00000, 0x10000, 0x066d67be )

	ROM_REGION_DISPOSE(0xac000)		/* temporary space for graphics */
	ROM_LOAD( "7-2",			0x00000, 0x04000, 0xaf48c4e6 )	/* chars */
	ROM_LOAD( "6-2",			0x04000, 0x04000, 0x9a29a862 )
	ROM_LOAD( "5-2",			0x08000, 0x04000, 0xfb7cad55 )
	ROM_LOAD( "b02_12.rom",		0x0c000, 0x08000, 0x733b9997 )	/* fg tiles */
		/* 14000-1bfff not used */
	ROM_LOAD( "b02_15.rom",		0x1c000, 0x08000, 0x8b70ef32 )
		/* 24000-2bfff not used */
	ROM_LOAD( "b02_14.rom",		0x2c000, 0x08000, 0xf711ba7d )
		/* 34000-3bfff not used */
	ROM_LOAD( "b02_13.rom",		0x3c000, 0x08000, 0x62532cd3 )
		/* 44000-4bfff not used */
	ROM_LOAD( "b02_08.rom",		0x4c000, 0x08000, 0xef0cf49c )	/* bg tiles */
	ROM_LOAD( "b02_11.rom",		0x54000, 0x08000, 0xf5799422 )
	ROM_LOAD( "b02_10.rom",		0x5c000, 0x08000, 0x4bd099ff )
	ROM_LOAD( "b02_09.rom",		0x64000, 0x08000, 0x230f1582 )
	ROM_LOAD( "b02_01.512",		0x6c000, 0x10000, 0x2234b424 )	/* sprites */
	ROM_LOAD( "b02_02.512",		0x7c000, 0x10000, 0x30d4c9a8 )
	ROM_LOAD( "b02_03.512",		0x8c000, 0x10000, 0x64f3d88f )
	ROM_LOAD( "b02_04.512",		0x9c000, 0x10000, 0x3b23a9fc )

	ROM_REGION(0x10000)		/* 64k for second CPU */
	ROM_LOAD( "b02_16.rom",		0x0000, 0x8000, 0xcdd1a153 )

	ROM_REGION(0x10000)		/* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "dsp.lsb",	0x0000, 0x0800, BADCRC( 0xacb21beb ) )
	ROM_LOAD_ODD ( "dsp.msb",	0x0000, 0x0800, BADCRC( 0x963ce23d ) )
ROM_END

ROM_START( hishouza )
	ROM_REGION(0x20000)		/* 2*64k for code */
	ROM_LOAD_EVEN( "b02-18.rom",	0x00000, 0x10000, 0x4444bb94 )
	ROM_LOAD_ODD ( "b02-17.rom",	0x00000, 0x10000, 0xcdac7228 )

	ROM_REGION_DISPOSE(0xac000)		/* temporary space for graphics */
	ROM_LOAD( "b02-07.rom",		0x00000, 0x04000, 0xc13a775e )	/* chars */
	ROM_LOAD( "b02-06.rom",		0x04000, 0x04000, 0xad5f1371 )
	ROM_LOAD( "b02-05.rom",		0x08000, 0x04000, 0x85a7bff6 )
	ROM_LOAD( "b02_12.rom",		0x0c000, 0x08000, 0x733b9997 )	/* fg tiles */
		/* 14000-1bfff not used */
	ROM_LOAD( "b02_15.rom",		0x1c000, 0x08000, 0x8b70ef32 )
		/* 24000-2bfff not used */
	ROM_LOAD( "b02_14.rom",		0x2c000, 0x08000, 0xf711ba7d )
		/* 34000-3bfff not used */
	ROM_LOAD( "b02_13.rom",		0x3c000, 0x08000, 0x62532cd3 )
		/* 44000-4bfff not used */
	ROM_LOAD( "b02_08.rom",		0x4c000, 0x08000, 0xef0cf49c )	/* bg tiles */
	ROM_LOAD( "b02_11.rom",		0x54000, 0x08000, 0xf5799422 )
	ROM_LOAD( "b02_10.rom",		0x5c000, 0x08000, 0x4bd099ff )
	ROM_LOAD( "b02_09.rom",		0x64000, 0x08000, 0x230f1582 )
	ROM_LOAD( "b02_01.512",		0x6c000, 0x10000, 0x2234b424 )	/* sprites */
	ROM_LOAD( "b02_02.512",		0x7c000, 0x10000, 0x30d4c9a8 )
	ROM_LOAD( "b02_03.512",		0x8c000, 0x10000, 0x64f3d88f )
	ROM_LOAD( "b02_04.512",		0x9c000, 0x10000, 0x3b23a9fc )

	ROM_REGION(0x10000)		/* 64k for second CPU */
	ROM_LOAD( "b02_16.rom",		0x0000, 0x8000, 0xcdd1a153 )

	ROM_REGION(0x10000)		/* For TI TMS320C10 Microcontroller */
#ifndef LSB_FIRST
	ROM_LOAD_NIB_HIGH( "dsp-a3.bpr", 0x1000, 0x0400, 0xdf88e79b ) /* lsb */
	ROM_LOAD_NIB_LOW ( "dsp-a4.bpr", 0x1000, 0x0400, 0xa2094a7f )
	ROM_LOAD_NIB_HIGH( "dsp-b7.bpr", 0x1400, 0x0400, 0xe87540cd )
	ROM_LOAD_NIB_LOW ( "dsp-b8.bpr", 0x1400, 0x0400, 0xd3c16c5c )
	ROM_LOAD_NIB_HIGH( "dsp-a1.bpr", 0x1800, 0x0400, 0x45d4d1b1 ) /* msb */
	ROM_LOAD_NIB_LOW ( "dsp-a2.bpr", 0x1800, 0x0400, 0xedd227fa )
	ROM_LOAD_NIB_HIGH( "dsp-b5.bpr", 0x1c00, 0x0400, 0x85ca5d47 )
	ROM_LOAD_NIB_LOW ( "dsp-b6.bpr", 0x1c00, 0x0400, 0x81816b2c )
#else
	ROM_LOAD_NIB_HIGH( "dsp-a1.bpr", 0x1000, 0x0400, 0x45d4d1b1 ) /* lsb */
	ROM_LOAD_NIB_LOW ( "dsp-a2.bpr", 0x1000, 0x0400, 0xedd227fa )
	ROM_LOAD_NIB_HIGH( "dsp-b5.bpr", 0x1400, 0x0400, 0x85ca5d47 )
	ROM_LOAD_NIB_LOW ( "dsp-b6.bpr", 0x1400, 0x0400, 0x81816b2c )
	ROM_LOAD_NIB_HIGH( "dsp-a3.bpr", 0x1800, 0x0400, 0xdf88e79b ) /* msb */
	ROM_LOAD_NIB_LOW ( "dsp-a4.bpr", 0x1800, 0x0400, 0xa2094a7f )
	ROM_LOAD_NIB_HIGH( "dsp-b7.bpr", 0x1c00, 0x0400, 0xe87540cd )
	ROM_LOAD_NIB_LOW ( "dsp-b8.bpr", 0x1c00, 0x0400, 0xd3c16c5c )
#endif
ROM_END

ROM_START( fsharkbt )
	ROM_REGION(0x20000)		/* 2*64k for code */
	ROM_LOAD_EVEN( "r18",		0x00000, 0x10000, 0xef30f563 )
	ROM_LOAD_ODD ( "r17",		0x00000, 0x10000, 0x0e18d25f )

	ROM_REGION_DISPOSE(0xac000)		/* temporary space for graphics */
	ROM_LOAD( "b02_07-1.rom",	0x00000, 0x04000, 0xe669f80e )	/* chars */
	ROM_LOAD( "b02_06-1.rom",	0x04000, 0x04000, 0x5e53ae47 )
	ROM_LOAD( "b02_05-1.rom",	0x08000, 0x04000, 0xa8b05bd0 )
	ROM_LOAD( "b02_12.rom",		0x0c000, 0x08000, 0x733b9997 )	/* fg tiles */
		/* 14000-1bfff not used */
	ROM_LOAD( "b02_15.rom",		0x1c000, 0x08000, 0x8b70ef32 )
		/* 24000-2bfff not used */
	ROM_LOAD( "b02_14.rom",		0x2c000, 0x08000, 0xf711ba7d )
		/* 34000-3bfff not used */
	ROM_LOAD( "b02_13.rom",		0x3c000, 0x08000, 0x62532cd3 )
		/* 44000-4bfff not used */
	ROM_LOAD( "b02_08.rom",		0x4c000, 0x08000, 0xef0cf49c )	/* bg tiles */
	ROM_LOAD( "b02_11.rom",		0x54000, 0x08000, 0xf5799422 )
	ROM_LOAD( "b02_10.rom",		0x5c000, 0x08000, 0x4bd099ff )
	ROM_LOAD( "b02_09.rom",		0x64000, 0x08000, 0x230f1582 )
	ROM_LOAD( "b02_01.512",		0x6c000, 0x10000, 0x2234b424 )	/* sprites */
	ROM_LOAD( "b02_02.512",		0x7c000, 0x10000, 0x30d4c9a8 )
	ROM_LOAD( "b02_03.512",		0x8c000, 0x10000, 0x64f3d88f )
	ROM_LOAD( "b02_04.512",		0x9c000, 0x10000, 0x3b23a9fc )

	ROM_REGION(0x10000)		/* 64k for second CPU */
	ROM_LOAD( "b02_16.rom",		0x0000, 0x8000, 0xcdd1a153 )

	ROM_REGION(0x10000)		/* For TI TMS320C10 Microcontroller */
#ifndef LSB_FIRST
	ROM_LOAD_NIB_HIGH( "mcu-3.bpr",  0x1000, 0x0400, 0xdf88e79b ) /* lsb */
	ROM_LOAD_NIB_LOW ( "mcu-4.bpr",  0x1000, 0x0400, 0xa2094a7f )
	ROM_LOAD_NIB_HIGH( "mcu-7.bpr",  0x1400, 0x0400, 0x0cd30d49 )
	ROM_LOAD_NIB_LOW ( "mcu-8.bpr",  0x1400, 0x0400, 0x3379bbff )
	ROM_LOAD_NIB_HIGH( "mcu-1.bpr",  0x1800, 0x0400, 0x45d4d1b1 ) /* msb */
	ROM_LOAD_NIB_LOW ( "mcu-2.bpr",  0x1800, 0x0400, 0x651336d1 )
	ROM_LOAD_NIB_HIGH( "mcu-5.bpr",  0x1c00, 0x0400, 0xf97a58da )
	ROM_LOAD_NIB_LOW ( "mcu-6.bpr",  0x1c00, 0x0400, 0xffcc422d )
#else
	ROM_LOAD_NIB_HIGH( "mcu-1.bpr",  0x1000, 0x0400, 0x45d4d1b1 ) /* msb */
	ROM_LOAD_NIB_LOW ( "mcu-2.bpr",  0x1000, 0x0400, 0x651336d1 )
	ROM_LOAD_NIB_HIGH( "mcu-5.bpr",  0x1400, 0x0400, 0xf97a58da )
	ROM_LOAD_NIB_LOW ( "mcu-6.bpr",  0x1400, 0x0400, 0xffcc422d )
	ROM_LOAD_NIB_HIGH( "mcu-3.bpr",  0x1800, 0x0400, 0xdf88e79b ) /* lsb */
	ROM_LOAD_NIB_LOW ( "mcu-4.bpr",  0x1800, 0x0400, 0xa2094a7f )
	ROM_LOAD_NIB_HIGH( "mcu-7.bpr",  0x1c00, 0x0400, 0x0cd30d49 )
	ROM_LOAD_NIB_LOW ( "mcu-8.bpr",  0x1c00, 0x0400, 0x3379bbff )
#endif
ROM_END


static void twincobr_decode(void)
{
	/* TMS320C10 roms have address lines A0 and A1 swapped. Decode it. */
	int A;
	unsigned char D1;
	unsigned char D2;
	unsigned char *DSP_ROMS;

	DSP_ROMS = Machine->memory_region[Machine->drv->cpu[2].memory_region];

	for (A = 2;A <= 0x0ffa;A+=8) {
		D1 = DSP_ROMS[A];
		D2 = DSP_ROMS[A+1];
		DSP_ROMS[A] = DSP_ROMS[A+2];
		DSP_ROMS[A+1] = DSP_ROMS[A+3];
		DSP_ROMS[A+2] = D1;
		DSP_ROMS[A+3] = D2;
	}
}


static void fsharkbt_decode(void)
{
	int A;
	unsigned char datamsb;
	unsigned char datalsb;
	unsigned char *DSP_ROMS;

	DSP_ROMS = Machine->memory_region[Machine->drv->cpu[2].memory_region];

	/* The ROM loader fixes the nibble images. Here we fix the byte ordering. */

	for (A = 0;A < 0x0800;A++)
	{
		datamsb = DSP_ROMS[0x1000 + A];
		datalsb = DSP_ROMS[0x1800 + A];
		DSP_ROMS[A*2]   = datamsb;
		DSP_ROMS[A*2+1] = datalsb;
	}
		for (A = 0x1000;A < 0x2000;A++)
	{
		DSP_ROMS[A] = 00;
	}
}

void twincobr_hiscore_init_tests(void)
{
	/* Initialise hi score variables. Tables are contiguous */
	/* Memory is cleared out in the machine driver before hi score is tested */

	twinc_scores = 0x15a2;  	/* 54 * first four bytes are the top of screen */
                                /*      screen hi score. The hi score table */
								/*		then starts after that */
	twinc_names  = 0x15f6;		/* f0 * start of names table */
	twinc_levels = 0x16e6;		/* 28 * start of level acheived table */
	twinc_end    = 0x170e;		/* 		end of table */
	twinc_vid_test = 0x5803;	/* green digit 3 in 30000 from screen memory to test */
}
void ktiger_hiscore_init_tests(void)
{
	twinc_scores = 0x1280;
	twinc_names  = 0x12d4;
	twinc_levels = 0x13c4;
	twinc_end    = 0x13ec;
	twinc_vid_test = 0x3803;	/* red 3 */
}
void fshark_hiscore_init_tests(void)
{
	twinc_scores = 0x016a;
	twinc_names  = 0x01be;
	twinc_levels = 0x02ae;
	twinc_end    = 0x02d6;
	twinc_vid_test = 0x0003;	/* white 3 */
}


static int twincobr_hiload(void)
{
	/* check if the hi score table has already been initialized */

	int twinc_tx_offset;
	int twinc_tx_data;
	int twinc_scrn1=0x466;	/* video ram locations of high score */
	int twinc_scrn2=0x426;
	int twinc_scrn3=0x3e6;
	int twinc_scrn4=0x3a6;
	int twinc_scrn5=0x366;
	int twinc_scrn6=0x326;
	int twinc_scrn7=0x2e6;

	twinc_tx_offset = twincobr_txoffs_r();	/* save current video offset */
	twincobr_txoffs_w(0,twinc_scrn4);
	twinc_tx_data = twincobr_txram_r(0);


	if ((READ_WORD(&twincobr_68k_dsp_ram[(twinc_scores+2)])==0x3000) && /* start of table */
		(READ_WORD(&twincobr_68k_dsp_ram[(twinc_scores+6)])==0x3000) &&
		(READ_WORD(&twincobr_68k_dsp_ram[(twinc_end-4)])==0x0001) &&
		(READ_WORD(&twincobr_68k_dsp_ram[(twinc_end-2)])==0x0001) && /* end of table */
		(twinc_tx_data == twinc_vid_test))						   /* video layer initialised ? */
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int twinc_hiscore;
			int twinc_zero_flag = 0;
			int twinc_digit1, twinc_digit2, twinc_digit3, twinc_digit4;

			osd_fread_msbfirst(f,&twincobr_68k_dsp_ram[twinc_scores],
				twinc_names - twinc_scores);					/* score values */
			osd_fread_msbfirst(f,&twincobr_68k_dsp_ram[twinc_names],
				twinc_levels - twinc_names);					/* initials */
			osd_fread_msbfirst(f,&twincobr_68k_dsp_ram[twinc_levels],
				twinc_end - twinc_levels);					/* levels */

			WRITE_WORD(&twincobr_68k_dsp_ram[twinc_scores],		/* update high score on top of screen */
				(READ_WORD(&twincobr_68k_dsp_ram[(twinc_scores+4)])));
			WRITE_WORD(&twincobr_68k_dsp_ram[(twinc_scores+2)],
				(READ_WORD(&twincobr_68k_dsp_ram[(twinc_scores+6)])));

			twinc_hiscore = READ_WORD(&twincobr_68k_dsp_ram[twinc_scores]);
			twinc_digit1 =  twinc_hiscore & 0xf;
			twinc_digit2 = (twinc_hiscore >>  4) & 0xf;
			twinc_digit3 = (twinc_hiscore >>  8) & 0xf;

			if (twinc_digit3)
			{
				twincobr_txoffs_w(0,twinc_scrn7);
				twinc_tx_data = twincobr_txram_r(0) & 0xf800;
				twincobr_txram_w(0,(twinc_tx_data | twinc_digit3));
				twinc_zero_flag = 1;
			}

			if ((twinc_digit2) || (twinc_zero_flag))
			{
				twincobr_txoffs_w(0,twinc_scrn6);
				twinc_tx_data = twincobr_txram_r(0) & 0xf800;
				twincobr_txram_w(0,(twinc_tx_data | twinc_digit2));
				twinc_zero_flag = 1;
			}

			if ((twinc_digit1) || (twinc_zero_flag))
			{
				twincobr_txoffs_w(0,twinc_scrn5);
				twinc_tx_data = twincobr_txram_r(0) & 0xf800;
				twincobr_txram_w(0,(twinc_tx_data | twinc_digit1));
			}

			twinc_hiscore = READ_WORD(&twincobr_68k_dsp_ram[(twinc_scores+2)]);
			twinc_digit4 = (twinc_hiscore >> 12) & 0xf;
			twinc_digit3 = (twinc_hiscore >>  8) & 0xf;
			twinc_digit2 = (twinc_hiscore >>  4) & 0xf;
			twinc_digit1 =  twinc_hiscore & 0xf;

			twincobr_txoffs_w(0,twinc_scrn4);
			twinc_tx_data = twincobr_txram_r(0) & 0xf800;
			twincobr_txram_w(0,(twinc_tx_data | twinc_digit4));

			twincobr_txoffs_w(0,twinc_scrn3);
			twinc_tx_data = twincobr_txram_r(0) & 0xf800;
			twincobr_txram_w(0,(twinc_tx_data | twinc_digit3));

			twincobr_txoffs_w(0,twinc_scrn2);
			twinc_tx_data = twincobr_txram_r(0) & 0xf800;
			twincobr_txram_w(0,(twinc_tx_data | twinc_digit2));

			twincobr_txoffs_w(0,twinc_scrn1);
			twinc_tx_data = twincobr_txram_r(0) & 0xf800;
			twincobr_txram_w(0,(twinc_tx_data | twinc_digit1));

			osd_fclose(f);
		}
		twincobr_txoffs_w(0,twinc_tx_offset);
		return 1;
	}
	else
	{
		twincobr_txoffs_w(0,twinc_tx_offset);	/* restore video offset */
		return 0;			/* we can't load the hi scores yet */
	}
}

static void twincobr_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,&twincobr_68k_dsp_ram[twinc_scores],
			twinc_end - twinc_scores);						/* HS table */
		osd_fclose(f);
	}
}


struct GameDriver twincobr_driver =
{
	__FILE__,
	0,
	"twincobr",
	"Twin Cobra (World)",
	"1987",
	"Taito Corporation",
	"Quench\nNicola Salmoria",
	0,
	&machine_driver,
	twincobr_hiscore_init_tests,
	twincobr_rom,
	twincobr_decode, 0,
	0,
	0,
	twincobr_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	twincobr_hiload, twincobr_hisave
};

struct GameDriver twincobu_driver =
{
	__FILE__,
	&twincobr_driver,
	"twincobu",
	"Twin Cobra (US)",
	"1987",
	"Taito America Corporation (Romstar license)",
	"Quench\nNicola Salmoria",
	0,
	&machine_driver,
	twincobr_hiscore_init_tests,
	twincobu_rom,
	0, 0,
	0,
	0,
	twincobu_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver ktiger_driver =
{
	__FILE__,
	&twincobr_driver,
	"ktiger",
	"Kyukyoku Tiger (Japan)",
	"1987",
	"Taito Corporation",
	"Quench\nNicola Salmoria\nCarl-Henrik (HW Advice)",
	0,
	&machine_driver,
	ktiger_hiscore_init_tests,
	ktiger_rom,
	0, 0,
	0,
	0,
	ktiger_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	twincobr_hiload, twincobr_hisave
};

struct GameDriver fshark_driver =
{
	__FILE__,
	0,
	"fshark",
	"Flying Shark (World)",
	"1987",
	"Taito Corporation",
	"Quench\nNicola Salmoria\nCarl-Henrik Starstedt - DSC (HW info)\nMagnus Danielsson - DSC (HW info)\nRuben Panossian (HW info)",
	0,
	&machine_driver,
	fshark_hiscore_init_tests,
	fshark_rom,
	0, 0,
	0,
	0,
	fshark_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	twincobr_hiload, twincobr_hisave
};

struct GameDriver skyshark_driver =
{
	__FILE__,
	&fshark_driver,
	"skyshark",
	"Sky Shark (US)",
	"1987",
	"Taito America Corporation (Romstar license)",
	"Quench\nNicola Salmoria\nCarl-Henrik Starstedt - DSC (HW info)\nMagnus Danielsson - DSC (HW info)\nRuben Panossian (HW info)",
	0,
	&machine_driver,
	fshark_hiscore_init_tests,
	skyshark_rom,
	0, 0,
	0,
	0,
	skyshark_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	twincobr_hiload, twincobr_hisave
};

struct GameDriver hishouza_driver =
{
	__FILE__,
	&fshark_driver,
	"hishouza",
	"Hishou Zame (Japan)",
	"1987",
	"Taito Corporation",
	"Quench\nNicola Salmoria\nCarl-Henrik Starstedt - DSC (HW info)\nMagnus Danielsson - DSC (HW info)\nRuben Panossian (HW info)",
	0,
	&machine_driver,
	fshark_hiscore_init_tests,
	hishouza_rom,
	fsharkbt_decode, 0,
	0,
	0,
	hishouza_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	twincobr_hiload, twincobr_hisave
};

struct GameDriver fsharkbt_driver =
{
	__FILE__,
	&fshark_driver,
	"fsharkbt",
	"Flying Shark (bootleg)",
	"1987",
	"bootleg",
	"Quench\nNicola Salmoria\nCarl-Henrik Starstedt - DSC (HW info)\nMagnus Danielsson - DSC (HW info)\nRuben Panossian (HW info)",
	0,
	&machine_driver,
	fshark_hiscore_init_tests,
	fsharkbt_rom,
	fsharkbt_decode, 0,
	0,
	0,
	skyshark_input_ports,
	0, 0, 0,
	ORIENTATION_ROTATE_270,
	twincobr_hiload, twincobr_hisave
};
