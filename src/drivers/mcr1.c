/***************************************************************************

	Midway MCR-1 system

	Currently implemented:
		* Solar Fox
		* Kick

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	CPU #1
	========================================================================
	0000-6FFF   R     xxxxxxxx    Program ROM
	7000-77FF   R/W   xxxxxxxx    NVRAM
	F000-F1FF   R/W   xxxxxxxx    Sprite RAM
	F400-F41F     W   xxxxxxxx    Palette RAM blue/green
	F800-F81F     W   xxxxxxxx    Palette RAM red
	FC00-FFFF   R/W   xxxxxxxx    Background video RAM
	========================================================================
	0000        R     x-xxxxxx    Input ports
	            R     x-------    Service switch (active low)
	            R     --x-----    Tilt
	            R     ---xxx--    External inputs
	            R     ------x-    Right coin
	            R     -------x    Left coin
	0000        W     xxxxxxxx    Data latch OP0 (coin meters, 2 led's and cocktail 'flip')
	0001        R     xxxxxxxx    External inputs
	0002        R     xxxxxxxx    External inputs
	0003        R     xxxxxxxx    DIP switches
	0004        R     xxxxxxxx    External inputs
	0004        W     xxxxxxxx    Data latch OP4 (comm. with external hardware)
	0007        R     xxxxxxxx    Audio status
	001C-001F   W     xxxxxxxx    Audio latches 1-4
	00E0        W     --------    Watchdog reset
	00E8        W     xxxxxxxx    Unknown (written at initialization time)
	00F0-00F3   W     xxxxxxxx    CTC communications
	========================================================================
	Interrupts:
		NMI ???
		INT generated by CTC
	========================================================================


	========================================================================
	CPU #2 (Super Sound I/O)
	========================================================================
	0000-3FFF   R     xxxxxxxx    Program ROM
	8000-83FF   R/W   xxxxxxxx    Program RAM
	9000-9003   R     xxxxxxxx    Audio latches 1-4
	A000          W   xxxxxxxx    AY-8910 #1 control
	A001        R     xxxxxxxx    AY-8910 #1 status
	A002          W   xxxxxxxx    AY-8910 #1 data
	B000          W   xxxxxxxx    AY-8910 #2 control
	B001        R     xxxxxxxx    AY-8910 #2 status
	B002          W   xxxxxxxx    AY-8910 #2 data
	C000          W   xxxxxxxx    Audio status
	E000          W   xxxxxxxx    Unknown
	F000        R     xxxxxxxx    Audio board switches
	========================================================================
	Interrupts:
		NMI ???
		INT generated by external circuitry 780 times/second
	========================================================================

***************************************************************************/


#include "driver.h"
#include "machine/mcr.h"
#include "machine/z80fmly.h"
#include "sndhrdw/mcr.h"
#include "vidhrdw/generic.h"


/* video driver data & functions */
extern INT16 mcr1_spriteoffset;

void mcr1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0xf000, 0xf1ff, MRA_RAM },
	{ 0xfc00, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0xf000, 0xf1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf400, 0xf41f, paletteram_xxxxRRRRBBBBGGGG_split1_w, &paletteram },
	{ 0xf800, 0xf81f, paletteram_xxxxRRRRBBBBGGGG_split2_w, &paletteram_2 },
	{ 0xfc00, 0xffff, videoram_w, &videoram, &videoram_size },
	{ -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x00, 0x04, mcr_port_04_dispatch_r },
	{ 0x07, 0x07, ssio_status_r },
	{ 0x10, 0x10, mcr_port_04_dispatch_r },
	{ 0xf0, 0xf3, z80ctc_0_r },
	{ -1 }
};


static struct IOWritePort writeport[] =
{
	{ 0x00, 0x01, mcr_port_01_w },
	{ 0x04, 0x07, mcr_port_47_dispatch_w },
	{ 0x1c, 0x1f, ssio_data_w },
	{ 0xe0, 0xe0, watchdog_reset_w },
	{ 0xe8, 0xe8, mcr_unknown_w },
	{ 0xf0, 0xf3, z80ctc_0_w },
	{ -1 }	/* end of table */
};



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( solarfox_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( kick_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 -- this is the Kick spinner input.  */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 10, 50, 5, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Music" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

MCR_CHAR_LAYOUT(charlayout, 256);
MCR_SPRITE_LAYOUT(spritelayout, 64);

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 1 },	/* colors 0-15 */
	{ 1, 0x2000, &spritelayout, 16, 1 },	/* colors 16-31 */
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,	/* 2.5 Mhz */
			0,
			readmem,writemem,readport,writeport,
			mcr_interrupt,1,
			0,0,mcr_daisy_chain
		},
		SOUND_CPU_SSIO(2)
	},
	30, DEFAULT_REAL_30HZ_VBLANK_DURATION,
	1,
	mcr_init_machine,

	/* video hardware */
	32*16, 32*16, { 0, 32*16-1, 0, 30*16-1 },
	gfxdecodeinfo,
	32,32,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		SOUND_SSIO
	}
};



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void solarfox_init(void)
{
	static const UINT8 hiscore_init[] = { 0,0,1,1,1,1,1,3,3,3,7,0,0,0,0,0 };

	MCR_CONFIGURE_HISCORE(0x7000, 0x800, hiscore_init);
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	MCR_CONFIGURE_DEFAULT_PORTS;

	mcr1_spriteoffset = 3;
}


static void kick_init(void)
{
	MCR_CONFIGURE_HISCORE(0x7000, 0x800, NULL);
	MCR_CONFIGURE_SOUND(MCR_SSIO);
	MCR_CONFIGURE_DEFAULT_PORTS;

	mcr1_spriteoffset = -3;
}



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( solarfox )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sfcpu.3b",     0x0000, 0x1000, 0x8c40f6eb )
	ROM_LOAD( "sfcpu.4b",     0x1000, 0x1000, 0x4d47bd7e )
	ROM_LOAD( "sfcpu.5b",     0x2000, 0x1000, 0xb52c3bd5 )
	ROM_LOAD( "sfcpu.4d",     0x3000, 0x1000, 0xbd5d25ba )
	ROM_LOAD( "sfcpu.5d",     0x4000, 0x1000, 0xdd57d817 )
	ROM_LOAD( "sfcpu.6d",     0x5000, 0x1000, 0xbd993cd9 )
	ROM_LOAD( "sfcpu.7d",     0x6000, 0x1000, 0x8ad8731d )

	ROM_REGION_DISPOSE(0x0a000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sfcpu.4g",     0x0000, 0x1000, 0xba019a60 )
	ROM_LOAD( "sfcpu.5g",     0x1000, 0x1000, 0x7ff0364e )
	ROM_LOAD( "sfvid.1a",     0x2000, 0x2000, 0x9d9b5d7e )
	ROM_LOAD( "sfvid.1b",     0x4000, 0x2000, 0x78801e83 )
	ROM_LOAD( "sfvid.1d",     0x6000, 0x2000, 0x4d8445cf )
	ROM_LOAD( "sfvid.1e",     0x8000, 0x2000, 0x3da25495 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sfsnd.7a",     0x0000, 0x1000, 0xcdecf83a )
	ROM_LOAD( "sfsnd.8a",     0x1000, 0x1000, 0xcb7788cb )
	ROM_LOAD( "sfsnd.9a",     0x2000, 0x1000, 0x304896ce )
ROM_END


ROM_START( kick )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1200a-v2.b3",  0x0000, 0x1000, 0x65924917 )
	ROM_LOAD( "1300b-v2.b4",  0x1000, 0x1000, 0x27929f52 )
	ROM_LOAD( "1400c-v2.b5",  0x2000, 0x1000, 0x69107ce6 )
	ROM_LOAD( "1500d-v2.d4",  0x3000, 0x1000, 0x04a23aa1 )
	ROM_LOAD( "1600e-v2.d5",  0x4000, 0x1000, 0x1d2834c0 )
	ROM_LOAD( "1700f-v2.d6",  0x5000, 0x1000, 0xddf84ce1 )

	ROM_REGION_DISPOSE(0x0a000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1800g-v2.g4",  0x0000, 0x1000, 0xb4d120f3 )
	ROM_LOAD( "1900h-v2.g5",  0x1000, 0x1000, 0xc3ba4893 )
	ROM_LOAD( "2600a-v2.1e",  0x2000, 0x2000, 0x2c5d6b55 )
	ROM_LOAD( "2700b-v2.1d",  0x4000, 0x2000, 0x565ea97d )
	ROM_LOAD( "2800c-v2.1b",  0x6000, 0x2000, 0xf3be56a1 )
	ROM_LOAD( "2900d-v2.1a",  0x8000, 0x2000, 0x77da795e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "4200-a.a7",    0x0000, 0x1000, 0x9e35c02e )
	ROM_LOAD( "4300-b.a8",    0x1000, 0x1000, 0xca2b7c28 )
	ROM_LOAD( "4400-c.a9",    0x2000, 0x1000, 0xd1901551 )
	ROM_LOAD( "4500-d.a10",   0x3000, 0x1000, 0xd36ddcdc )
ROM_END


ROM_START( kicka )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1200-a.b3",    0x0000, 0x1000, 0x22fa42ed )
	ROM_LOAD( "1300-b.b4",    0x1000, 0x1000, 0xafaca819 )
	ROM_LOAD( "1400-c.b5",    0x2000, 0x1000, 0x6054ee56 )
	ROM_LOAD( "1500-d.d4",    0x3000, 0x1000, 0x263af0f3 )
	ROM_LOAD( "1600-e.d5",    0x4000, 0x1000, 0xeaaa78a7 )
	ROM_LOAD( "1700-f.d6",    0x5000, 0x1000, 0xc06c880f )

	ROM_REGION_DISPOSE(0x0a000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1000-g.g4",    0x0000, 0x1000, 0xacdae4f6 )
	ROM_LOAD( "1100-h.g5",    0x1000, 0x1000, 0xdbb18c96 )
	ROM_LOAD( "2600-a.1e",    0x2000, 0x2000, 0x74b409d7 )
	ROM_LOAD( "2700-b.1d",    0x4000, 0x2000, 0x78eda36c )
	ROM_LOAD( "2800-c.1b",    0x6000, 0x2000, 0xc93e0170 )
	ROM_LOAD( "2900-d.1a",    0x8000, 0x2000, 0x91e59383 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "4200-a.a7",    0x0000, 0x1000, 0x9e35c02e )
	ROM_LOAD( "4300-b.a8",    0x1000, 0x1000, 0xca2b7c28 )
	ROM_LOAD( "4400-c.a9",    0x2000, 0x1000, 0xd1901551 )
	ROM_LOAD( "4500-d.a10",   0x3000, 0x1000, 0xd36ddcdc )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

struct GameDriver solarfox_driver =
{
	__FILE__,
	0,
	"solarfox",
	"Solar Fox",
	"1981",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&machine_driver,
	solarfox_init,

	solarfox_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	solarfox_input_ports,

	0, 0, 0,
	ORIENTATION_SWAP_XY,

	mcr_hiload,mcr_hisave
};


struct GameDriver kick_driver =
{
	__FILE__,
	0,
	"kick",
	"Kick (mirror version)",
	"1981",
	"Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver\nJohn Butler",
	0,
	&machine_driver,
	kick_init,

	kick_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	kick_input_ports,

	0, 0, 0,
	ORIENTATION_SWAP_XY,

	mcr_hiload,mcr_hisave
};


struct GameDriver kicka_driver =
{
	__FILE__,
	&kick_driver,
	"kicka",
	"Kick (upright version)",
	"1981",
	"bootleg?",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver\nJohn Butler",
	0,
	&machine_driver,
	kick_init,

	kicka_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	kick_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	mcr_hiload,mcr_hisave
};
