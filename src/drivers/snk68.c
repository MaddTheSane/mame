/***************************************************************************

	POW - Prisoners Of War (US)			A7008	SNK 1988
	POW - Prisoners Of War (Japan)		A7008	SNK 1988
	SAR - Search And Rescue	(US)		A8007	SNK 1989
	Street Smart (US version 1)			A8007	SNK 1989
	Street Smart (US version 2)			A7008	SNK 1989
	Street Smart (Japan version 1)		A8007	SNK 1989
	Ikari III	- The Rescue (US)		A7007	SNK 1989

	For some strange reason version 2 of Street Smart runs on Pow hardware!

	Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

void pow_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void pow_paletteram_w(int offset,int data);
void pow_flipscreen_w(int offset,int data);
int  pow_vh_start(void);
void pow_video_w(int offset,int data);
void searchar_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static unsigned char *pow_ram;

/******************************************************************************/

static int sound_cpu_r(int offset)
{
	return 0x0100;
}

static int pow_video_r(int offset)
{
	return READ_WORD(&videoram[offset]);
}
static int control_1_r(int offset)
{
	return (readinputport(0) + (readinputport(1) << 8));
}

static int control_2_r(int offset)
{
	return readinputport(2);
}

static int dip_1_r(int offset)
{
	return (readinputport(3) << 8);
}

static int dip_2_r(int offset)
{
	return (readinputport(4) << 8);
}

static int rotary_1_r(int offset)
{
	return (( ~(1 << (readinputport(5) * 12 / 256)) )<<8)&0xff00;
}

static int rotary_2_r(int offset)
{
	return (( ~(1 << (readinputport(6) * 12 / 256)) )<<8)&0xff00;
}

static int rotary_lsb_r(int offset)
{
	return ((( ~(1 << (readinputport(6) * 12 / 256))  ) <<4)&0xf000)
    	 + ((( ~(1 << (readinputport(5) * 12 / 256))  )    )&0x0f00);
}

static void sound_w(int offset, int data)
{
	soundlatch_w(0,(data>>8)&0xff);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}

/*******************************************************************************/

static struct MemoryReadAddress pow_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x040000, 0x043fff, MRA_BANK1 },
	{ 0x080000, 0x080001, control_1_r },
	{ 0x0c0000, 0x0c0001, control_2_r },
	{ 0x0e0000, 0x0e0001, MRA_NOP }, /* Watchdog or IRQ ack */
	{ 0x0e8000, 0x0e8001, MRA_NOP }, /* Watchdog or IRQ ack */
	{ 0x0f0000, 0x0f0001, dip_1_r },
	{ 0x0f0008, 0x0f0009, dip_2_r },
	{ 0x100000, 0x100fff, pow_video_r },
	{ 0x200000, 0x207fff, MRA_BANK3 },
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress pow_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x040000, 0x043fff, MWA_BANK1, &pow_ram },
	{ 0x080000, 0x080001, sound_w },
	{ 0x0c0000, 0x0c0001, pow_flipscreen_w },
	{ 0x0f0008, 0x0f0009, MWA_NOP },
	{ 0x100000, 0x100fff, pow_video_w, &videoram },
	{ 0x200000, 0x207fff, MWA_BANK3, &spriteram },
	{ 0x400000, 0x400fff, pow_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress searchar_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x040000, 0x043fff, MRA_BANK1 },
	{ 0x080000, 0x080001, input_port_0_r }, /* Player 1 */
	{ 0x080002, 0x080003, input_port_1_r }, /* Player 2 */
	{ 0x080004, 0x080005, input_port_2_r }, /* Coins */
	{ 0x0c0000, 0x0c0001, rotary_1_r }, /* Player 1 rotary */
	{ 0x0c8000, 0x0c8001, rotary_2_r }, /* Player 2 rotary */
	{ 0x0d0000, 0x0d0001, rotary_lsb_r }, /* Extra rotary bits */
	{ 0x0e0000, 0x0e0001, MRA_NOP },  /* Watchdog or IRQ ack */
	{ 0x0e8000, 0x0e8001, MRA_NOP },  /* Watchdog or IRQ ack */
	{ 0x0f0000, 0x0f0001, dip_1_r },
	{ 0x0f0008, 0x0f0009, dip_2_r },
	{ 0x0f8000, 0x0f8001, sound_cpu_r },
	{ 0x100000, 0x107fff, MRA_BANK3 },
	{ 0x200000, 0x200fff, pow_video_r },
	{ 0x300000, 0x33ffff, MRA_BANK8 }, /* Extra code bank */
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress searchar_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x040000, 0x043fff, MWA_BANK1, &pow_ram },
	{ 0x080000, 0x080001, sound_w },
	{ 0x080006, 0x080007, MWA_NOP }, /* Watchdog? */
	{ 0x0c0000, 0x0c0001, pow_flipscreen_w },
	{ 0x0f0000, 0x0f0001, MWA_NOP },
	{ 0x100000, 0x107fff, MWA_BANK3, &spriteram },
	{ 0x200000, 0x200fff, pow_video_w, &videoram },
	{ 0x201000, 0x201fff, pow_video_w }, /* Mirror used by Ikari 3 */
	{ 0x400000, 0x400fff, pow_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static void D7759_write_port_0_w(int offset, int data)
{
	UPD7759_reset_w (0,0);
	UPD7759_message_w(offset,data);
	UPD7759_start_w (0,0);
}

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ -1 }
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x20, 0x20, YM3812_write_port_0_w },
	{ 0x40, 0x40, D7759_write_port_0_w },
	{ 0x80, 0x80, MWA_NOP }, /* IRQ ack? */
	{ -1 }
};

/******************************************************************************/


INPUT_PORTS_START( pow_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service button */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1, all active high */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurence" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPSETTING(    0x20, "1st & every 2nd" )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2, all active high */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k 50k" )
	PORT_DIPSETTING(    0x08, "40k 100k" )
	PORT_DIPSETTING(    0x04, "60k 150k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x30, "Freeze" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )
INPUT_PORTS_END


/* Identical to pow, but the Language dip switch has no effect */
INPUT_PORTS_START( powj_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service button */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1, all active high */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurence" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPSETTING(    0x20, "1st & every 2nd" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2, all active high */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k 50k" )
	PORT_DIPSETTING(    0x08, "40k 100k" )
	PORT_DIPSETTING(    0x04, "60k 150k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x30, "Freeze" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )
INPUT_PORTS_END


INPUT_PORTS_START( searchar_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* coin */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switches (Active high) */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Coin A & B" )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x00, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPSETTING(    0x40, "1st & every 2nd" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dip switches (Active high) */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50k 200k" )
	PORT_DIPSETTING(    0x08, "70k 270k" )
	PORT_DIPSETTING(    0x04, "90k 350k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x30, "Freeze" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 10, 0, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 10, 0, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( streetsm_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* coin */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switches (Active high) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A & B" )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPSETTING(    0x20, "1st & every 2nd" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dip switches (Active high) */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "200k 400k" )
	PORT_DIPSETTING(    0x08, "400k 600k" )
	PORT_DIPSETTING(    0x04, "600k 800k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x30, "Freeze" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START	/* player 1 12-way rotary control - not used in this game */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 2 12-way rotary control - not used in this game */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/* Same as streetsm, but Coinage is different */
INPUT_PORTS_START( streetsj_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* coin */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switches (Active high) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0c, "A 4/1 B 1/4" )
	PORT_DIPSETTING(    0x04, "A 3/1 B 1/3" )
	PORT_DIPSETTING(    0x08, "A 2/1 B 1/2" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPSETTING(    0x20, "1st & every 2nd" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dip switches (Active high) */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "200k 400k" )
	PORT_DIPSETTING(    0x08, "400k 600k" )
	PORT_DIPSETTING(    0x04, "600k 800k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x30, "Freeze" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START	/* player 1 12-way rotary control - not used in this game */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* player 2 12-way rotary control - not used in this game */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( ikari3_input_ports )
	PORT_START	/* Player 1 controls, maybe all are active_high? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2  )

	PORT_START	/* coin */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switches (Active high) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A & B" )
	PORT_DIPSETTING(    0x08, "First 2 Coins/1 Credit then 1/1" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, "First 1 Coin/2 Credits then 1/1" )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x00, "1st & 2nd only" )
	PORT_DIPSETTING(    0x20, "1st & every 2nd" )
	PORT_DIPNAME( 0x40, 0x00, "Blood" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dip switches (Active high) */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k 50k" )
	PORT_DIPSETTING(    0x08, "40k 100k" )
	PORT_DIPSETTING(    0x04, "60k 150k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x30, "Freeze" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 10, 0, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 10, 0, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	2048,
	4,		/* 4 bits per pixel  */
	{ 0, 4, 0x8000*8, (0x8000*8)+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	4096*4,
	4,		/* 4 bits per pixel */
	{ 0, 0x80000*8, 0x100000*8, 0x180000*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	16,16,
	0x6000,
	4,
	{ 0, 8,0x180000*8, 0x180000*8+8 },
	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
		7, 6, 5, 4, 3, 2, 1, 0
	},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	64*8
};

static struct GfxLayout spritelayout3 =
{
	16,16,	/* 16*16 sprites */
	4096*5,
	4,		/* 4 bits per pixel */
	{ 0x140000*8, 0, 0xa0000*8, 0x1e0000*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,   0, 128 },
	{ 1, 0x010000, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo searchar_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,   0,  16 },
	{ 1, 0x010000, &spritelayout2,0, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ikari3_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,   0,  16 },
	{ 1, 0x010000, &spritelayout3,0, 128 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	4000000,	/* 4 MHz */
	{ 255 }
};

static struct UPD7759_interface upd7759_interface =
{
	1,		/* number of chips */
	UPD7759_STANDARD_CLOCK,
	{ 50 }, /* volume */
	{ 3 },		/* memory region */
	UPD7759_STANDALONE_MODE,		/* chip mode */
	{0}
};

/******************************************************************************/

static struct MachineDriver ikari3_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M68000,
			10000000,	/* Accurate */
			0,
			searchar_readmem,searchar_writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* Accurate */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,3	/* ?? hand tuned */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	ikari3_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pow_vh_start,
	0,
	searchar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_UPD7759,
			&upd7759_interface
		}
	}
};

static struct MachineDriver pow_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M68000,
			10000000,	/* Accurate */
			0,
			pow_readmem,pow_writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* Accurate */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,3	/* ?? hand tuned */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pow_vh_start,
	0,
	pow_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_UPD7759,
			&upd7759_interface
		}
	}
};

static struct MachineDriver searchar_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M68000,
			12000000,
			0,
			searchar_readmem,searchar_writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,3	/* ?? hand tuned */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	searchar_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pow_vh_start,
	0,
	searchar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_UPD7759,
			&upd7759_interface
		}
	}
};

static struct MachineDriver streets2_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M68000,
			10000000,	/* Accurate */
			0,
			pow_readmem,pow_writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* Accurate */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,3	/* ?? hand tuned */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	searchar_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	pow_vh_start,
	0,
	searchar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_UPD7759,
			&upd7759_interface
		}
	}
};

/******************************************************************************/


ROM_START( pow )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "dg1",   0x000000, 0x20000, 0x8e71a8af )
	ROM_LOAD_ODD ( "dg2",   0x000000, 0x20000, 0x4287affc )

	ROM_REGION_DISPOSE(0x210000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dg9",        0x000000, 0x08000, 0xdf864a08 )
	ROM_LOAD( "dg10",       0x008000, 0x08000, 0x9e470d53 )
	ROM_LOAD( "snk880.11a", 0x010000, 0x20000, 0xe70fd906 )
	ROM_LOAD( "snk880.12a", 0x030000, 0x20000, 0x628b1aed )
	ROM_LOAD( "snk880.13a", 0x050000, 0x20000, 0x19dc8868 )
	ROM_LOAD( "snk880.14a", 0x070000, 0x20000, 0x47cd498b )
	ROM_LOAD( "snk880.15a", 0x090000, 0x20000, 0x7a90e957 )
	ROM_LOAD( "snk880.16a", 0x0b0000, 0x20000, 0xe40a6c13 )
	ROM_LOAD( "snk880.17a", 0x0d0000, 0x20000, 0xc7931cc2 )
	ROM_LOAD( "snk880.18a", 0x0f0000, 0x20000, 0xeed72232 )
	ROM_LOAD( "snk880.19a", 0x110000, 0x20000, 0x1775b8dd )
	ROM_LOAD( "snk880.20a", 0x130000, 0x20000, 0xf8e752ec )
	ROM_LOAD( "snk880.21a", 0x150000, 0x20000, 0x27e9fffe )
	ROM_LOAD( "snk880.22a", 0x170000, 0x20000, 0xaa9c00d8 )
	ROM_LOAD( "snk880.23a", 0x190000, 0x20000, 0xadb6ad68 )
	ROM_LOAD( "snk880.24a", 0x1b0000, 0x20000, 0xdd41865a )
	ROM_LOAD( "snk880.25a", 0x1d0000, 0x20000, 0x055759ad )
	ROM_LOAD( "snk880.26a", 0x1f0000, 0x20000, 0x9bc261c5 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "dg8",        0x000000, 0x10000, 0xd1d61da3 )

	ROM_REGION(0x10000)	/* UPD7759 samples */
	ROM_LOAD( "dg7",        0x000000, 0x10000, 0xaba9a9d3 )
ROM_END


ROM_START( powj )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "1-2",   0x000000, 0x20000, 0x2f17bfb0 )
	ROM_LOAD_ODD ( "2-2",   0x000000, 0x20000, 0xbaa32354 )

	ROM_REGION_DISPOSE(0x210000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dg9",        0x000000, 0x08000, 0xdf864a08 )
	ROM_LOAD( "dg10",       0x008000, 0x08000, 0x9e470d53 )
	ROM_LOAD( "snk880.11a", 0x010000, 0x20000, 0xe70fd906 )
	ROM_LOAD( "snk880.12a", 0x030000, 0x20000, 0x628b1aed )
	ROM_LOAD( "snk880.13a", 0x050000, 0x20000, 0x19dc8868 )
	ROM_LOAD( "snk880.14a", 0x070000, 0x20000, 0x47cd498b )
	ROM_LOAD( "snk880.15a", 0x090000, 0x20000, 0x7a90e957 )
	ROM_LOAD( "snk880.16a", 0x0b0000, 0x20000, 0xe40a6c13 )
	ROM_LOAD( "snk880.17a", 0x0d0000, 0x20000, 0xc7931cc2 )
	ROM_LOAD( "snk880.18a", 0x0f0000, 0x20000, 0xeed72232 )
	ROM_LOAD( "snk880.19a", 0x110000, 0x20000, 0x1775b8dd )
	ROM_LOAD( "snk880.20a", 0x130000, 0x20000, 0xf8e752ec )
	ROM_LOAD( "snk880.21a", 0x150000, 0x20000, 0x27e9fffe )
	ROM_LOAD( "snk880.22a", 0x170000, 0x20000, 0xaa9c00d8 )
	ROM_LOAD( "snk880.23a", 0x190000, 0x20000, 0xadb6ad68 )
	ROM_LOAD( "snk880.24a", 0x1b0000, 0x20000, 0xdd41865a )
	ROM_LOAD( "snk880.25a", 0x1d0000, 0x20000, 0x055759ad )
	ROM_LOAD( "snk880.26a", 0x1f0000, 0x20000, 0x9bc261c5 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "dg8",        0x000000, 0x10000, 0xd1d61da3 )

	ROM_REGION(0x10000)	/* UPD7759 samples */
	ROM_LOAD( "dg7",        0x000000, 0x10000, 0xaba9a9d3 )
ROM_END


ROM_START( searchar )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "bh.2",  0x000000, 0x20000, 0xc852e2e2 )
	ROM_LOAD_ODD ( "bh.3",  0x000000, 0x20000, 0xbc04a4a1 )

	ROM_REGION_DISPOSE(0x310000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bh.7",       0x000000, 0x08000, 0xb0f1b049 )
	ROM_LOAD( "bh.8",       0x008000, 0x08000, 0x174ddba7 )

	ROM_LOAD( "bh.c1",      0x010000, 0x80000, 0x1fb8f0ae )
	ROM_LOAD( "bh.c3",      0x090000, 0x80000, 0xfd8bc407 )
	ROM_LOAD( "bh.c5",      0x110000, 0x80000, 0x1d30acc3 )
	ROM_LOAD( "bh.c2",      0x190000, 0x80000, 0x7c803767 )
	ROM_LOAD( "bh.c4",      0x210000, 0x80000, 0xeede7c43 )
	ROM_LOAD( "bh.c6",      0x290000, 0x80000, 0x9f785cd9 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "bh.5",       0x000000, 0x10000, 0x53e2fa76 )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "bh.v1",      0x000000, 0x20000, 0x07a6114b )

	ROM_REGION(0x40000) /* Extra code bank */
	ROM_LOAD_EVEN( "bh.1",  0x000000, 0x20000, 0xba9ca70b )
	ROM_LOAD_ODD ( "bh.4",  0x000000, 0x20000, 0xeabc5ddf )
ROM_END


ROM_START( streetsm )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "s2-1ver1.9c",  0x00000, 0x20000, 0xb59354c5 )
	ROM_LOAD_ODD ( "s2-2ver1.10c", 0x00000, 0x20000, 0xe448b68b )

	ROM_REGION_DISPOSE(0x310000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s2-7.15l",    0x000000, 0x08000, 0x22bedfe5 )
	ROM_LOAD( "s2-8.15m",    0x008000, 0x08000, 0x6a1c70ab )

	ROM_LOAD( "stsmart.900", 0x010000, 0x80000, 0xa8279a7e )
	ROM_LOAD( "stsmart.902", 0x090000, 0x80000, 0x2f021aa1 )
	ROM_LOAD( "stsmart.904", 0x110000, 0x80000, 0x167346f7 )
	ROM_LOAD( "stsmart.901", 0x190000, 0x80000, 0xc305af12 )
	ROM_LOAD( "stsmart.903", 0x210000, 0x80000, 0x73c16d35 )
	ROM_LOAD( "stsmart.905", 0x290000, 0x80000, 0xa5beb4e2 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "s2-5.16c",    0x000000, 0x10000, 0xca4b171e )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "s2-6.18d",    0x000000, 0x20000, 0x47db1605 )
ROM_END


ROM_START( streets2 )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "s2-1ver2.14h", 0x00000, 0x20000, 0x655f4773 )
	ROM_LOAD_ODD ( "s2-2ver2.14k", 0x00000, 0x20000, 0xefae4823 )

	ROM_REGION_DISPOSE(0x310000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s2-9.25l",    0x000000, 0x08000, 0x09b6ac67 )
	ROM_LOAD( "s2-10.25m",   0x008000, 0x08000, 0x89e4ee6f )

	ROM_LOAD( "stsmart.900", 0x010000, 0x80000, 0xa8279a7e )
	ROM_LOAD( "stsmart.902", 0x090000, 0x80000, 0x2f021aa1 )
	ROM_LOAD( "stsmart.904", 0x110000, 0x80000, 0x167346f7 )
	ROM_LOAD( "stsmart.901", 0x190000, 0x80000, 0xc305af12 )
	ROM_LOAD( "stsmart.903", 0x210000, 0x80000, 0x73c16d35 )
	ROM_LOAD( "stsmart.905", 0x290000, 0x80000, 0xa5beb4e2 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "s2-5.16c",    0x000000, 0x10000, 0xca4b171e )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "s2-6.18d",    0x000000, 0x20000, 0x47db1605 )
ROM_END


ROM_START( streetsj )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "s2v1j_01.bin", 0x00000, 0x20000, 0xf031413c )
	ROM_LOAD_ODD ( "s2v1j_02.bin", 0x00000, 0x20000, 0xe403a40b )

	ROM_REGION_DISPOSE(0x310000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s2-7.15l",    0x000000, 0x08000, 0x22bedfe5 )
	ROM_LOAD( "s2-8.15m",    0x008000, 0x08000, 0x6a1c70ab )

	ROM_LOAD( "stsmart.900", 0x010000, 0x80000, 0xa8279a7e )
	ROM_LOAD( "stsmart.902", 0x090000, 0x80000, 0x2f021aa1 )
	ROM_LOAD( "stsmart.904", 0x110000, 0x80000, 0x167346f7 )
	ROM_LOAD( "stsmart.901", 0x190000, 0x80000, 0xc305af12 )
	ROM_LOAD( "stsmart.903", 0x210000, 0x80000, 0x73c16d35 )
	ROM_LOAD( "stsmart.905", 0x290000, 0x80000, 0xa5beb4e2 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "s2-5.16c",    0x000000, 0x10000, 0xca4b171e )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "s2-6.18d",    0x000000, 0x20000, 0x47db1605 )
ROM_END


ROM_START( ikari3 )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "ik3-2.bin", 0x000000, 0x20000, 0xa7b34dcd )
	ROM_LOAD_ODD ( "ik3-3.bin", 0x000000, 0x20000, 0x50f2b83d )

	ROM_REGION_DISPOSE(0x290000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ik3-7.bin",  0x000000, 0x08000, 0x0b4804df )
	ROM_LOAD( "ik3-8.bin",  0x008000, 0x08000, 0x10ab4e50 )

	ROM_LOAD( "ik3-13.bin", 0x010000, 0x20000, 0x9a56bd32 )
	ROM_LOAD( "ik3-12.bin", 0x030000, 0x20000, 0x0ce6a10a )
	ROM_LOAD( "ik3-11.bin", 0x050000, 0x20000, 0xe4e2be43 )
	ROM_LOAD( "ik3-10.bin", 0x070000, 0x20000, 0xac222372 )
	ROM_LOAD( "ik3-9.bin",  0x090000, 0x20000, 0xc33971c2 )

	ROM_LOAD( "ik3-14.bin", 0x0b0000, 0x20000, 0x453bea77 )
	ROM_LOAD( "ik3-15.bin", 0x0d0000, 0x20000, 0x781a81fc )
	ROM_LOAD( "ik3-16.bin", 0x0f0000, 0x20000, 0x80ba400b )
	ROM_LOAD( "ik3-17.bin", 0x110000, 0x20000, 0x0cc3ce4a )
	ROM_LOAD( "ik3-18.bin", 0x130000, 0x20000, 0xba106245 )

	ROM_LOAD( "ik3-23.bin", 0x150000, 0x20000, 0xd0fd5c77 )
	ROM_LOAD( "ik3-22.bin", 0x170000, 0x20000, 0x4878d883 )
	ROM_LOAD( "ik3-21.bin", 0x190000, 0x20000, 0x50d0fbf0 )
	ROM_LOAD( "ik3-20.bin", 0x1b0000, 0x20000, 0x9a851efc )
	ROM_LOAD( "ik3-19.bin", 0x1d0000, 0x20000, 0x4ebdba89 )

	ROM_LOAD( "ik3-24.bin", 0x1f0000, 0x20000, 0xe9b26d68 )
	ROM_LOAD( "ik3-25.bin", 0x210000, 0x20000, 0x073b03f1 )
	ROM_LOAD( "ik3-26.bin", 0x230000, 0x20000, 0x9c613561 )
	ROM_LOAD( "ik3-27.bin", 0x250000, 0x20000, 0x16dd227e )
	ROM_LOAD( "ik3-28.bin", 0x270000, 0x20000, 0x711715ae )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "ik3-5.bin",  0x000000, 0x10000, 0xce6706fc )

	ROM_REGION(0x20000)	/* UPD7759 samples */
	ROM_LOAD( "ik3-6.bin",  0x000000, 0x20000, 0x59d256a4 )

	ROM_REGION(0x40000) /* Extra code bank */
	ROM_LOAD_EVEN( "ik3-1.bin",  0x000000, 0x10000, 0x47e4d256 )
	ROM_LOAD_ODD ( "ik3-4.bin",  0x000000, 0x10000, 0xa43af6b5 )
ROM_END

/******************************************************************************/

static int pow_cycle_r(int offset)
{
	int c=READ_WORD(&pow_ram[0x3e7c]);
	if (cpu_get_pc()==0x628 && (c&0xff)!=0xf) { cpu_spinuntil_int(); return 0xf; }
	return c;
}

static int ikari3_cycle_r(int offset)
{
	int c=READ_WORD(&pow_ram[0x0]);
	if (cpu_get_pc()==0x107a && (c&0xff00)==0) { cpu_spinuntil_int(); return 0xf; }
	return c;
}

static int sar_cycle_r(int offset)
{
	int c=READ_WORD(&pow_ram[0x2]);
	int p=cpu_get_pc();
	if ((p==0x427c || p==0x4294 || p==0x42a6 || p==0xc9e) && c==0x800a) { cpu_spinuntil_int(); return 0xa; }
	return c;
}

static int streetsm_cycle_r(int offset)
{
	int c=READ_WORD(&pow_ram[0x3e02]);
	if (cpu_get_pc()==0x1240 && (c&0xff)!=0xf) { cpu_spinuntil_int(); return 0xf; }
	return c;
}

static void custom_memory(void)
{
	if (!strcmp(Machine->gamedrv->name,"pow")) install_mem_read_handler(0, 0x43e7c, 0x43e7d, pow_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"powj")) install_mem_read_handler(0, 0x43e7c, 0x43e7d, pow_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"searchar")) install_mem_read_handler(0, 0x40002, 0x40003, sar_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"streetsm")) install_mem_read_handler(0, 0x43e02, 0x43e03, streetsm_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ikari3")) install_mem_read_handler(0, 0x40000, 0x40001, ikari3_cycle_r);

}

static void searchar_memory(void)
{
	cpu_setbank(8, Machine->memory_region[4]);
	custom_memory();
}

static void streetsm_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	WRITE_WORD (&RAM[0x107d0],0x4245); /* Clear D5 (Sprite ram error!?) */
}

static void streetsj_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	WRITE_WORD (&RAM[0x10710],0x4245); /* Clear D5 (Sprite ram error!?) */
}

/******************************************************************************/

struct GameDriver pow_driver =
{
	__FILE__,
	0,
	"pow",
	"P.O.W. - Prisoners of War (US)",
	"1988",
	"SNK",
	"Bryan McPhail",
	0,
	&pow_machine_driver,
	custom_memory,

	pow_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pow_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver powj_driver =
{
	__FILE__,
	&pow_driver,
	"powj",
	"Datsugoku - Prisoners of War (Japan)",
	"1988",
	"SNK",
	"Bryan McPhail",
	0,
	&pow_machine_driver,
	custom_memory,

	powj_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	powj_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver searchar_driver =
{
	__FILE__,
	0,
	"searchar",
	"SAR - Search And Rescue (US)",
	"1989",
	"SNK",
	"Bryan McPhail",
	0,
	&searchar_machine_driver,
	searchar_memory,

	searchar_rom,
	0, 0,
	0,
	0,

	searchar_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver streetsm_driver =
{
	__FILE__,
	0,
	"streetsm",
	"Street Smart (US version 1)",
	"1989",
	"SNK",
	"Bryan McPhail",
	0,
	&searchar_machine_driver,
	custom_memory,

	streetsm_rom,
	streetsm_patch, 0,
	0,
	0,

	streetsm_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver streets2_driver =
{
	__FILE__,
	&streetsm_driver,
	"streets2",
	"Street Smart (US version 2)",
	"1989",
	"SNK",
	"Bryan McPhail",
	0,
	&streets2_machine_driver,
	custom_memory,

	streets2_rom,
	0, 0,
	0,
	0,

	streetsm_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver streetsj_driver =
{
	__FILE__,
	&streetsm_driver,
	"streetsj",
	"Street Smart (Japan version 1)",
	"1989",
	"SNK",
	"Bryan McPhail",
	0,
	&searchar_machine_driver,
	custom_memory,

	streetsj_rom,
	streetsj_patch, 0,
	0,
	0,

	streetsj_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};


struct GameDriver ikari3_driver =
{
	__FILE__,
	0,
	"ikari3",
	"Ikari III - The Rescue",
	"1989",
	"SNK",
	"Bryan McPhail",
	0,
	&ikari3_machine_driver,
	searchar_memory,

	ikari3_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	ikari3_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
