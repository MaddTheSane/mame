/***************************************************************************

Scramble memory map (preliminary)

MAIN BOARD:
0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
5000-50ff Object RAM
5000-503f  screen attributes
5040-505f  sprites
5060-507f  bullets
5080-50ff  unused?

read:
7000      Watchdog Reset (Scramble)
7800      Watchdog Reset (Battle of Atlantis)
8100      IN0
8101      IN1
8102      IN2 (bits 5 and 7 used for protection check in Scramble)

write:
6801      interrupt enable
6802      coin counter
6803      ? (POUT1)
6804      stars on
6805      ? (POUT2)
6806      screen vertical flip
6807      screen horizontal flip
8200      To AY-3-8910 port A (commands for the audio CPU)
8201      bit 3 = interrupt trigger on audio CPU  bit 4 = AMPM (?)
8202      protection check control?


SOUND BOARD:
0000-17ff ROM
8000-83ff RAM

I/0 ports:
read:
20      8910 #2  read
80      8910 #1  read

write
10      8910 #2  control20      8910 #2  write
40      8910 #1  control
80      8910 #1  write

interrupts:
interrupt mode 1 triggered by the main CPU


Interesting tidbit:

There is a bug in Amidars. Look at the loop at 0x2715. It expects DE to be
saved during the call to 0x2726, but it can be destroyed, causing the loop
to read all kinds of bogus memory locations.


To Do:

- Mariner seems to have discrete sound circuits connected to the 8910's
  output ports


Notes:

- While Atlantis has a cabinet switch, it doesn't use the 2nd player controls
  in cocktail mode.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mariner_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void galaxian_flipx_w(int offset,int data);
void galaxian_flipy_w(int offset,int data);
void galaxian_attributes_w(int offset,int data);
void galaxian_stars_w(int offset,int data);
void scramble_background_w(int offset, int data);

int  scramble_vh_start(void);
int  mariner_vh_start(void);
int  ckongs_vh_start(void);

void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int  scramble_vh_interrupt(void);
int  mariner_vh_interrupt(void);
void scramble_filter_w(int offset, int data);

int scramble_portB_r(int offset);
void scramble_sh_irqtrigger_w(int offset,int data);

int frogger_portB_r(int offset);

/* protection stuff. in machine\scramble.c */
int scramble_input_port_2_r(int offset);
int scramble_protection_r(int offset);
int scramblk_protection_r(int offset);
int mariner_protection_1_r(int offset);
int mariner_protection_2_r(int offset);
int mariner_pip(int offset);
int mariner_pap(int offset);


static int ckongs_input_port_1_r(int offset)
{
	return (readinputport(1) & 0xfc) | ((readinputport(2) & 0x06) >> 1);
}

static int ckongs_input_port_2_r(int offset)
{
	return (readinputport(2) & 0xf9) | ((readinputport(1) & 0x03) << 1);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x4c00, 0x4fff, videoram_r },	/* mirror address */
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ 0x8100, 0x8100, input_port_0_r },	/* IN0 */
	{ 0x8101, 0x8101, input_port_1_r },	/* IN1 */
	{ 0x8102, 0x8102, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ckongs_readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6bff, MRA_RAM },				/* RAM */
	{ 0x7000, 0x7000, input_port_0_r },			/* IN0 */
	{ 0x7001, 0x7001, ckongs_input_port_1_r },	/* IN1 */
	{ 0x7002, 0x7002, ckongs_input_port_2_r },	/* IN2 */
	{ 0x9000, 0x93ff, MRA_RAM },				/* Video RAM */
	{ 0x9800, 0x987f, MRA_RAM },				/* screen attributes, sprites, bullets */
	{ 0xb000, 0xb000, watchdog_reset_r },
	{ -1 }	/* end of table */
};

/* Extra ROM and protection locations */
static struct MemoryReadAddress mariner_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x4c00, 0x4fff, videoram_r },	/* mirror address */
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x5800, 0x67ff, MRA_ROM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8100, 0x8100, input_port_0_r },	/* IN0 */
	{ 0x8101, 0x8101, input_port_1_r },	/* IN1 */
	{ 0x8102, 0x8102, input_port_2_r },	/* IN2 */
	{ 0x9008, 0x9008, mariner_protection_2_r },
	{ 0xb401, 0xb401, mariner_protection_1_r },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x503f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x6801, 0x6801, interrupt_enable_w },
	{ 0x6802, 0x6802, coin_counter_w },
	{ 0x6803, 0x6803, scramble_background_w },
	{ 0x6804, 0x6804, galaxian_stars_w },
	{ 0x6806, 0x6806, galaxian_flipx_w },
	{ 0x6807, 0x6807, galaxian_flipy_w },
	{ 0x8200, 0x8200, soundlatch_w },
	{ 0x8201, 0x8201, scramble_sh_irqtrigger_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress triplep_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4c00, 0x4fff, videoram_w },	/* mirror address */
	{ 0x5000, 0x503f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x6801, 0x6801, interrupt_enable_w },
	{ 0x6802, 0x6802, coin_counter_w },
	{ 0x6803, 0x6803, MWA_NOP },   /* ??? (it's NOT a background enable) */
	{ 0x6804, 0x6804, galaxian_stars_w },
	{ 0x6806, 0x6806, galaxian_flipx_w },
	{ 0x6807, 0x6807, galaxian_flipy_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ckongs_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6bff, MWA_RAM },
	{ 0x7800, 0x7800, soundlatch_w },
	{ 0x7801, 0x7801, scramble_sh_irqtrigger_w },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa802, 0xa802, coin_counter_w },
	{ 0xa804, 0xa804, galaxian_stars_w },
	{ 0xa806, 0xa806, galaxian_flipx_w },
	{ 0xa807, 0xa807, galaxian_flipy_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort triplep_readport[] =
{
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x02, 0x02, mariner_pip },
	{ 0x03, 0x03, mariner_pap },
	{ -1 }	/* end of table */
};

static struct IOWritePort triplep_writeport[] =
{
	{ 0x01, 0x01, AY8910_control_port_0_w },
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x17ff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x17ff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9000, 0x9fff, scramble_filter_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress froggers_sound_readmem[] =
{
	{ 0x0000, 0x17ff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress froggers_sound_writemem[] =
{
	{ 0x0000, 0x17ff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
  //{ 0x6000, 0x6fff, scramble_filter_w },  /* There is probably a filter here,	 */
							  	            /* but it can't possibly be the same */
	{ -1 }	/* end of table */				/* as the one in Scramble. One 8910 only */
};


static struct IOReadPort sound_readport[] =
{
	{ 0x20, 0x20, AY8910_read_port_1_r },
	{ 0x80, 0x80, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x20, 0x20, AY8910_write_port_1_w },
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort froggers_sound_readport[] =
{
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort froggers_sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ 0x80, 0x80, AY8910_control_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( scramble_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1  B 2/1  C 1/1" )
	PORT_DIPSETTING(    0x02, "A 1/2  B 1/1  C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3  B 3/1  C 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/4  B 4/1  C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
INPUT_PORTS_END

INPUT_PORTS_START( atlantis_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/3  B 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/6  B 1/1" )
	PORT_DIPSETTING(    0x04, "A 1/99 B 1/99")
	/* all the other combos give 99 credits */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* same as scramble, dip switches are different */
INPUT_PORTS_START( theend_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
INPUT_PORTS_END

INPUT_PORTS_START( froggers_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot2 - unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(    0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 1/6 C 1/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( amidars_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( triplep_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/2 B 1/1 C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1 C 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 2/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1 C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BITX(    0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ckongs_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout mariner_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout mariner_spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* it's just 1 pixel, but we use 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 3, 0, 0, 0, 0, 0, 0 },	/* I "know" that this bit of the */
	{ 0 },						/* graphics ROMs is 1 */
	0	/* no use */
};
static struct GfxLayout theend_bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* 4*1 line, I think - 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 2, 2, 2, 2, 0, 0, 0 },	/* I "know" that this bit of the */
	{ 0 },						/* graphics ROMs is 1 */
	0	/* no use */
};
static struct GfxLayout backgroundlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	8,8,
	32,	/* one for each column */
	7,	/* 128 colors max */
	{ 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	8*8*8	/* each character takes 64 bytes */
};


static struct GfxDecodeInfo scramble_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout, 8*4, 1 },	/* 1 color code instead of 2, so all */
											/* shots will be yellow */
	{ 0, 0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo theend_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0000, &theend_bulletlayout, 8*4, 2 },
	{ 0, 0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo mariner_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mariner_charlayout,     0, 8 },
	{ 1, 0x0000, &mariner_spritelayout,   0, 8 },
	{ 1, 0x0000, &bulletlayout,         8*4, 1 },	/* 1 color code instead of 2, so all */
													/* shots will be yellow */
	{ 0, 0x0000, &backgroundlayout, 8*4+2*2, 1 },	/* this will be dynamically created */
	{ -1 } /* end of array */
};



/* this is NOT the original color PROM - it's the Scramble one */
static unsigned char wrong_color_prom[] =
{
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
};



static struct AY8910interface scramble_ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 MHz */
	{ 0x30ff, 0x30ff },
	{ soundlatch_r },
	{ scramble_portB_r },
	{ 0 },
	{ 0 }
};

static struct AY8910interface frogger_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ 0x30ff },
	{ soundlatch_r },
	{ frogger_portB_r },
	{ 0 },
	{ 0 }
};

static struct AY8910interface triplep_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ 0x30ff },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver scramble_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	scramble_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scramble_ay8910_interface
		}
	}
};


/* same as Scramble, the only difference is gfxdecodeinfo */
static struct MachineDriver theend_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	theend_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scramble_ay8910_interface
		}
	}
};

static struct MachineDriver froggers_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			0,
			readmem,writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			3,	/* memory region #3 */
			froggers_sound_readmem,froggers_sound_writemem,froggers_sound_readport,froggers_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	scramble_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&frogger_ay8910_interface
		}
	}
};

/* Triple Punch and Mariner are different - only one CPU, one 8910 */
static struct MachineDriver triplep_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			0,
			readmem,triplep_writemem,triplep_readport,triplep_writeport,
			scramble_vh_interrupt,1
		}
	},
	60, 2500,/* ? */	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	scramble_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&triplep_ay8910_interface
		}
	}
};

static struct MachineDriver mariner_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			0,
			mariner_readmem,triplep_writemem,triplep_readport,triplep_writeport,
			mariner_vh_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	mariner_gfxdecodeinfo,
	32+64+10,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 10 for background */
	mariner_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mariner_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&triplep_ay8910_interface
		}
	}
};

static struct MachineDriver ckongs_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			ckongs_readmem,ckongs_writemem,0,0,
			scramble_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 Mhz */
			3,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	mariner_gfxdecodeinfo,
	32+64+1,8*4+2*2+128*1,	/* 32 for the characters, 64 for the stars, 1 for background */
	galaxian_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	ckongs_vh_start,
	generic_vh_stop,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scramble_ay8910_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scramble_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2d",           0x0000, 0x0800, 0xb89207a1 )
	ROM_LOAD( "2e",           0x0800, 0x0800, 0xe9b4b9eb )
	ROM_LOAD( "2f",           0x1000, 0x0800, 0xa1f14f4c )
	ROM_LOAD( "2h",           0x1800, 0x0800, 0x591bc0d9 )
	ROM_LOAD( "2j",           0x2000, 0x0800, 0x22f11b6b )
	ROM_LOAD( "2l",           0x2800, 0x0800, 0x705ffe49 )
	ROM_LOAD( "2m",           0x3000, 0x0800, 0xea26c35c )
	ROM_LOAD( "2p",           0x3800, 0x0800, 0x94d8f5e3 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x5f30311a )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0x516e029e )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )
ROM_END

ROM_START( scramblk_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2d.k",         0x0000, 0x0800, 0xea35ccaa )
	ROM_LOAD( "2e.k",         0x0800, 0x0800, 0xe7bba1b3 )
	ROM_LOAD( "2f.k",         0x1000, 0x0800, 0x12d7fc3e )
	ROM_LOAD( "2h.k",         0x1800, 0x0800, 0xb59360eb )
	ROM_LOAD( "2j.k",         0x2000, 0x0800, 0x4919a91c )
	ROM_LOAD( "2l.k",         0x2800, 0x0800, 0x26a4547b )
	ROM_LOAD( "2m.k",         0x3000, 0x0800, 0x0bb49470 )
	ROM_LOAD( "2p.k",         0x3800, 0x0800, 0x6a5740e5 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "5f.k",         0x0000, 0x0800, 0x4708845b )
	ROM_LOAD( "5h.k",         0x0800, 0x0800, 0x11fd2887 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )
ROM_END

ROM_START( atlantis_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x0800, 0x0e485b9a )
	ROM_LOAD( "2e",           0x0800, 0x0800, 0xc1640513 )
	ROM_LOAD( "2f",           0x1000, 0x0800, 0xeec265ee )
	ROM_LOAD( "2h",           0x1800, 0x0800, 0xa5d2e442 )
	ROM_LOAD( "2j",           0x2000, 0x0800, 0x45f7cf34 )
	ROM_LOAD( "2l",           0x2800, 0x0800, 0xf335b96b )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x57f9c6b9 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0xe989f325 )

	ROM_REGION(0x0020)	/* color prom */
	/* missing in action */
	ROM_LOAD( "atlantis.clr", 0x0000, 0x0020, 0x00000000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )
ROM_END

ROM_START( theend_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic13",         0x0000, 0x0800, 0x90e5ab14 )
	ROM_LOAD( "ic14",         0x0800, 0x0800, 0x950f0a07 )
	ROM_LOAD( "ic15",         0x1000, 0x0800, 0x6786bcf5 )
	ROM_LOAD( "ic16",         0x1800, 0x0800, 0x380a0017 )
	ROM_LOAD( "ic17",         0x2000, 0x0800, 0xaf067b7f )
	ROM_LOAD( "ic18",         0x2800, 0x0800, 0xa0411b93 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "ic30",         0x0000, 0x0800, 0x527fd384 )
	ROM_LOAD( "ic31",         0x0800, 0x0800, 0xaf6d09b6 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, 0x24652bc4 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ic56",         0x0000, 0x0800, 0x3b2c2f70 )
	ROM_LOAD( "ic55",         0x0800, 0x0800, 0xe0429e50 )
ROM_END

ROM_START( froggers_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_d2.bin",   0x0000, 0x0800, 0xc103066e )
	ROM_LOAD( "vid_e2.bin",   0x0800, 0x0800, 0xf08bc094 )
	ROM_LOAD( "vid_f2.bin",   0x1000, 0x0800, 0x637a2ff8 )
	ROM_LOAD( "vid_h2.bin",   0x1800, 0x0800, 0x04c027a5 )
	ROM_LOAD( "vid_j2.bin",   0x2000, 0x0800, 0xfbdfbe74 )
	ROM_LOAD( "vid_l2.bin",   0x2800, 0x0800, 0x8a4389e1 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "epr-1036.1k",  0x0000, 0x0800, 0x658745f8 )
	ROM_LOAD( "frogger.607",  0x0800, 0x0800, 0x05f7d883 )

	ROM_REGION(0x0020)	/* color PROMs */
	ROM_LOAD( "vid_e6.bin",   0x0000, 0x0020, 0x0b878b54 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, 0xe8ab0256 )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, 0x7380a48f )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, 0x31d7eb27 )
ROM_END

ROM_START( amidars_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "am2d",         0x0000, 0x0800, 0x24b79547 )
	ROM_LOAD( "am2e",         0x0800, 0x0800, 0x4c64161e )
	ROM_LOAD( "am2f",         0x1000, 0x0800, 0xb3987a72 )
	ROM_LOAD( "am2h",         0x1800, 0x0800, 0x29873461 )
	ROM_LOAD( "am2j",         0x2000, 0x0800, 0x0fdd54d8 )
	ROM_LOAD( "am2l",         0x2800, 0x0800, 0x5382f7ed )
	ROM_LOAD( "am2m",         0x3000, 0x0800, 0x1d7109e9 )
	ROM_LOAD( "am2p",         0x3800, 0x0800, 0xc9163ac6 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "2716.a6",      0x0000, 0x0800, 0x2082ad0a )   /* Same graphics ROMs as Amigo */
	ROM_LOAD( "2716.a5",      0x0800, 0x0800, 0x3029f94f )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "amidar.clr",   0x0000, 0x0020, 0xf940dcc3 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "amidarus.5c",  0x0000, 0x1000, 0x8ca7b750 )
	ROM_LOAD( "amidarus.5d",  0x1000, 0x1000, 0x9b5bdc0a )
ROM_END

ROM_START( triplep_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "triplep.2g",   0x0000, 0x1000, 0xc583a93d )
	ROM_LOAD( "triplep.2h",   0x1000, 0x1000, 0xc03ddc49 )
	ROM_LOAD( "triplep.2k",   0x2000, 0x1000, 0xe83ca6b5 )
	ROM_LOAD( "triplep.2l",   0x3000, 0x1000, 0x982cc3b9 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics */
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, 0xd51cbd6f )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, 0xf21c0059 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, 0x624f75df )
ROM_END

ROM_START( mariner_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "tp1",          0x0000, 0x1000, 0xdac1dfd0 )
	ROM_LOAD( "tm2",          0x1000, 0x1000, 0xefe7ca28 )
	ROM_LOAD( "tm3",          0x2000, 0x1000, 0x027881a6 )
	ROM_LOAD( "tm4",          0x3000, 0x1000, 0xa0fde7dc )
	ROM_LOAD( "tm5",          0x6000, 0x0800, 0xd7ebcb8e )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION_DISPOSE(0x2000)      /* temporary space for graphics */
	ROM_LOAD( "tm8",          0x0000, 0x1000, 0x70ae611f )
	ROM_LOAD( "tm9",          0x1000, 0x1000, 0x8e4e999e )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "tm.t4",        0x0000, 0x0020, 0xca42b6dd )
ROM_END

ROM_START( ckongs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x1000, 0x49a8c234 )
	ROM_LOAD( "vid_2e.bin",   0x1000, 0x1000, 0xf1b667f1 )
	ROM_LOAD( "vid_2f.bin",   0x2000, 0x1000, 0xb194b75d )
	ROM_LOAD( "vid_2h.bin",   0x3000, 0x1000, 0x2052ba8a )
	ROM_LOAD( "vid_2j.bin",   0x4000, 0x1000, 0xb377afd0 )
	ROM_LOAD( "vid_2l.bin",   0x5000, 0x1000, 0xfe65e691 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_5f.bin",   0x0000, 0x1000, 0x7866d2cb )
	ROM_LOAD( "vid_5h.bin",   0x1000, 0x1000, 0x7311a101 )

	ROM_REGION(0x20)	/* color prom */
	ROM_LOAD( "vid_6e.bin",   0x0000, 0x0020, 0x5039af97 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c",  0x0000, 0x1000, 0xf0c30f9a )
	ROM_LOAD( "snd_5d.bin",   0x1000, 0x1000, 0x892c9547 )
ROM_END



static void scramble_driver_init(void)
{
	install_mem_read_handler(0, 0x8102, 0x8102, scramble_input_port_2_r);
	install_mem_read_handler(0, 0x8202, 0x8202, scramble_protection_r);
}

static void scramblk_driver_init(void)
{
	install_mem_read_handler(0, 0x8202, 0x8202, scramblk_protection_r);
}


static void froggers_decode(void)
{
	int A;
	unsigned char *RAM;


	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	for (A = 0;A < 0x0800;A++)
		RAM[A] = (RAM[A] & 0xfc) | ((RAM[A] & 1) << 1) | ((RAM[A] & 2) >> 1);
}



static int scramble_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
    if ((memcmp(&RAM[0x4200],"\x00\x00\x01",3) == 0) &&
		(memcmp(&RAM[0x421B],"\x00\x00\x01",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4200],0x1E);
			/* copy high score */
			memcpy(&RAM[0x40A8],&RAM[0x4200],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void scramble_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4200],0x1E);
		osd_fclose(f);
	}

}


static int atlantis_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x403D],"\x00\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x403D],4*11);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void atlantis_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x403D],4*11);
		osd_fclose(f);
	}

}


static int theend_hiload(void)
{
	static int loop = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (loop == 0)
	{
		memset(&RAM[0x43c0],0xff,3*5);
		loop = 1;
	}

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x43c0],"\x00\x00\x00",3) == 0 &&
		memcmp(&RAM[0x43cc],"\x00\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* This seems to have more than 5 scores in memory. */
			/* If this DISPLAYS more than 5 scores, change 3*5 to 3*10 or */
			/* however many it should be. */
			osd_fread(f,&RAM[0x43c0],3*5);
			/* copy high score */
			memcpy(&RAM[0x40a8],&RAM[0x43c0],3);
			osd_fclose(f);
		}

		loop = 0;
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void theend_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* This seems to have more than 5 scores in memory. */
		/* If this DISPLAYS more than 5 scores, change 3*5 to 3*10 or */
		/* however many it should be. */
		osd_fwrite(f,&RAM[0x43C0],3*5);
		osd_fclose(f);
	}

}


static int froggers_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x43f1],"\x63\x04",2) == 0 &&
		memcmp(&RAM[0x43f8],"\x27\x01",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x43f1],2*5);
			RAM[0x43ef] = RAM[0x43f1];
			RAM[0x43f0] = RAM[0x43f2];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void froggers_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x43f1],2*5);
		osd_fclose(f);
	}
}


static int ckongs_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    /* check if the hi score table has already been initialized */
    /* NOTE : 60b8 + 3 */
	if (memcmp(&RAM[0x6109],"\x07\x06\x05",3) == 0 && memcmp(&RAM[0x61a0],"\xfd\xfd\xfd",3) == 0  )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			int hi;
        	osd_fread(f,&RAM[0x6107],155);
			osd_fclose(f);

			hi = (RAM[0x610b] &0x0f) * 0x10 +
				 (RAM[0x610c] &0x0f);
			RAM[0x60b8] = hi;
			hi = (RAM[0x6109] &0x0f) * 0x10 +
				 (RAM[0x610a] &0x0f);
			RAM[0x60b9] = hi;
			hi = (RAM[0x6107] &0x0f) * 0x10 +
				 (RAM[0x6108] &0x0f);
			RAM[0x60ba] = hi;
		}
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void ckongs_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x6107],155);
        osd_fclose(f);
    }
}



struct GameDriver scramble_driver =
{
	__FILE__,
	0,
	"scramble",
	"Scramble",
	"1981",
	"Konami",
	"Nicola Salmoria",
	0,
	&scramble_machine_driver,
	scramblk_driver_init,

	scramblk_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	scramble_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scramble_hiload, scramble_hisave
};

struct GameDriver scrambls_driver =
{
	__FILE__,
	&scramble_driver,
	"scrambls",
	"Scramble (Stern)",
	"1981",
	"[Konami] (Stern license)",
	"Nicola Salmoria",
	0,
	&scramble_machine_driver,
	scramble_driver_init,

	scramble_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	scramble_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scramble_hiload, scramble_hisave
};

struct GameDriver atlantis_driver =
{
	__FILE__,
	0,
	"atlantis",
	"Battle of Atlantis",
	"1981",
	"Comsoft",
	"Nicola Salmoria\nMike Balfour",
	GAME_WRONG_COLORS,
	&scramble_machine_driver,
	0,

	atlantis_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	atlantis_input_ports,

	wrong_color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	atlantis_hiload, atlantis_hisave
};

struct GameDriver theend_driver =
{
	__FILE__,
	0,
	"theend",
	"The End",
	"1980",
	"[Konami] (Stern license)",
	"Nicola Salmoria\nVille Laitinen\nMike Balfour",
	0,
	&theend_machine_driver,
	0,

	theend_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	theend_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	theend_hiload, theend_hisave
};

extern struct GameDriver frogger_driver;
struct GameDriver froggers_driver =
{
	__FILE__,
	&frogger_driver,
	"froggers",
	"Frog",
	"1981",
	"bootleg",
	"Nicola Salmoria",
	0,
	&froggers_machine_driver,
	0,

	froggers_rom,
	froggers_decode, 0,
	0,
	0,	/* sound_prom */

	froggers_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	froggers_hiload, froggers_hisave
};

extern struct GameDriver amidar_driver;
struct GameDriver amidars_driver =
{
	__FILE__,
	&amidar_driver,
	"amidars",
	"Amidar (Scramble hardware)",
	"1982",
	"Konami",
	"Nicola Salmoria\nMike Coates",
	0,
	&scramble_machine_driver,
	0,

	amidars_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	amidars_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0,0
};

struct GameDriver triplep_driver =
{
	__FILE__,
	0,
	"triplep",
	"Triple Punch",
	"1982",
	"KKI",
	"Nicola Salmoria",
	0,
	&triplep_machine_driver,
	0,

	triplep_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	triplep_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scramble_hiload, scramble_hisave
};

struct GameDriver mariner_driver =
{
	__FILE__,
	0,
	"mariner",
	"Mariner",
	"1981",
	"Amenip",
	"Zsolt Vasvari\nGerald Coy",
	0,
	&mariner_machine_driver,
	0,

	mariner_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	scramble_input_ports, /* seems to be the same as Scramble */

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	scramble_hiload, scramble_hisave
};

extern struct GameDriver ckong_driver;
struct GameDriver ckongs_driver =
{
	__FILE__,
	&ckong_driver,
	"ckongs",
	"Crazy Kong (Scramble hardware)",
	"1981",
	"bootleg",
	"Nicola Salmoria",
	0,
	&ckongs_machine_driver,
	0,

	ckongs_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	ckongs_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	ckongs_hiload, ckongs_hisave
};
